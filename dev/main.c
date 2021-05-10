#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>


#include "jsmn.h"
#include "msg.h"
#include "thread.h"

#include "net/emcute.h"
#include "net/ipv6/addr.h"

#include "timex.h"
#include "xtimer.h"
#include "shell.h"
#include "lpsxxx.h"
#include "lpsxxx_params.h"
#include "isl29020.h"
#include "isl29020_params.h"
#include "periph/gpio.h"

#define 	CONFIG_EMCUTE_DEFAULT_PORT   (1883U)

#define JSMN_HEADER

#define LIGHT_ITER 5 /* Light measurement - number of iterations */

#define DELAY (60000LU * US_PER_MS) /* 1 minute - Delay between main_loop() iterations */
//60000LU = 1 minute ; 300000LU = 5 minutes

#define LIGHT_SLEEP_TIME 1 /* Determines the sleep time (60 seconds) between subsequent iterations in the measure_light() loop */

#define TEMP_TOO_LOW 1
#define TEMP_TOO_HIGH 0
#define TEMP_OK 2

#define EMCUTE_PRIO (THREAD_PRIORITY_MAIN - 1)

/* MQTT SECTION */
#ifndef EMCUTE_ID
#define EMCUTE_ID ("power_saver_0")
#endif

#define _IPV6_DEFAULT_PREFIX_LEN (64U)

#define NUMOFSUBS (16U)
#define TOPIC_MAXLEN (64U)

#define MQTT_TOKENS 6 /* The number of tokens (key-value) that will be received by the IoT Core */


#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

/* [Emcute - MQTT] Stack and  vars */
static char stack_emcute[THREAD_STACKSIZE_DEFAULT];
//static msg_t queue[8];

static emcute_sub_t subscriptions[NUMOFSUBS];
static char topics[NUMOFSUBS][TOPIC_MAXLEN];


/* [Sensors] Stacks for multi-threading & tids placeholders*/
char stack_loop[THREAD_STACKSIZE_MAIN];
char stack_lux[THREAD_STACKSIZE_MAIN];
char stack_temp[THREAD_STACKSIZE_MAIN];

/* Threads' IDs */
kernel_pid_t tmain, t1, t2;

/* Light and temperature sensors */
static lpsxxx_t lpsxxx;
isl29020_t dev;

static void *emcute_thread(void *arg) {
    (void)arg;
    emcute_run(CONFIG_EMCUTE_DEFAULT_PORT, EMCUTE_ID);
    return NULL; /* should never be reached */
}

int parse_val(jsmntok_t key, char *command) {
    unsigned int length = key.end - key.start;
    char keyString[length + 1];
    memcpy(keyString, &command[key.start], length);
    keyString[length] = '\0';
    //printf("Val: %s\n", keyString);

    int val = atoi(keyString);

    return val;
}

int toggle_rgbled(int code);


/* Parse the reply from the IoT Core*/
int parse_command(char *command) {
    jsmn_parser parser;
    jsmntok_t tokens[MQTT_TOKENS];

    jsmn_init(&parser);

    int r = jsmn_parse(&parser, command, strlen(command), tokens, 10);

    if (r < 0) {
        printf("Failed to parse JSON: %d\n", r);
        return 1;
    }
    if (r < 1 || tokens[0].type != JSMN_OBJECT) {
        printf("Object expected\n");
        return 2;
    }


    int activations = 0;
    int acts = 0;

    // JSON STRUCT: {"acts":"1|2"", [lux":"0|1"], ["led":"0|1|2"]} ('[]' mean optional)
    for (int i = 1; i < MQTT_TOKENS; i += 2) {
        jsmntok_t key = tokens[i];
        unsigned int length = key.end - key.start;
        char keyString[length + 1];
        memcpy(keyString, &command[key.start], length);
        keyString[length] = '\0';
        //printf("Key: %s\n", keyString);

        if (strcmp(keyString, "acts") == 0) {
            int val = parse_val(tokens[i + 1], command);

            if (val < 1 || val > 2) {
                printf("An invalid number of actuator commands was passed: %d", val);
                return 7;
            }

            activations = val;
        }
        else if (strcmp(keyString, "lux") == 0) {
            int val = parse_val(tokens[i + 1], command);

            if (val < 0 || val > 1) {
                printf("An invalid value was supplied for lux: %d", val);
                return 4;
            }

            //toggle_lamp(val);

            acts++;
            if (acts == activations) {
                //puts("LuBreak");
                break;
            }
        } else if (strcmp(keyString, "led") == 0) {
            int val = parse_val(tokens[i + 1], command);

            if (val < 0 || val > 2) {
                printf("An invalid value was supplied for temp: %d", val);
                return 5;
            }

            toggle_rgbled(val);

            acts++;
            if (acts == activations) {
                //puts("LeBreak");
                break;
            }
        } else {
            printf("Key not recognized: %s\n", keyString);
        }
    }
 
    return 0;
}

static void on_pub(const emcute_topic_t *topic, void *data, size_t len) {
    char *in = (char *)data;

    char comm[len + 1];

    printf("### got publication for topic '%s' [%i] ###\n",
           topic->name, (int)topic->id);
    for (size_t i = 0; i < len; i++) {
        //printf("%c", in[i]);

        comm[i] = in[i];
    }
    comm[len] = '\0';

    printf("%s\n", comm);

    parse_command(comm);
    //puts("");
}

static int pub(char *topic, const char *data, int qos) {
    emcute_topic_t t;
    unsigned flags = EMCUTE_QOS_0;

    switch (qos) {
        case 1:
            flags |= EMCUTE_QOS_1;
            break;
        case 2:
            flags |= EMCUTE_QOS_2;
            break;
        default:
            flags |= EMCUTE_QOS_0;
            break;
    }

    t.name = MQTT_TOPIC;
    if (emcute_reg(&t) != EMCUTE_OK) {
        puts("[MQTT] PUB ERROR: Unable to obtain Topic ID");
        return 1;
    }
    if (emcute_pub(&t, data, strlen(data), flags) != EMCUTE_OK) {
        printf("[MQTT] PUB ERROR: unable to publish data to topic '%s [%i]'\n", t.name, (int)t.id);
        return 1;
    }

    printf("[MQTT] PUB SUCCESS: Published %s on topic %s\n", data, topic);
    return 0;
}

int setup_mqtt(void) {
    /* initialize our subscription buffers */
    memset(subscriptions, 0, (NUMOFSUBS * sizeof(emcute_sub_t)));

    /* start the emcute thread */
    thread_create(stack_emcute, sizeof(stack_emcute), EMCUTE_PRIO, 0, emcute_thread, NULL, "emcute");
    //Adding address to network interface
    //netif_add("4", "2001:0db8:0:f101::2");
    //netif_add("4", NUCLEO_ADDR);
    // connect to MQTT-SN broker

    printf("Connecting to MQTT-SN broker %s port %d.\n", SERVER_ADDR, SERVER_PORT);

    sock_udp_ep_t gw = {
        .family = AF_INET6,
        .port = SERVER_PORT};

    char *message = "connected";
    size_t len = strlen(message);

    /* parse address */
    if (ipv6_addr_from_str((ipv6_addr_t *)&gw.addr.ipv6, SERVER_ADDR) == NULL) {
        printf("error parsing IPv6 address\n");
        return 1;
    }

    if (emcute_con(&gw, true, MQTT_TOPIC, message, len, 0) != EMCUTE_OK) {
        printf("error: unable to connect to [%s]:%i\n", SERVER_ADDR, (int)gw.port);
        return 1;
    }

    printf("Successfully connected to gateway at [%s]:%i\n", SERVER_ADDR, (int)gw.port);

    // setup subscription to topic

    unsigned flags = EMCUTE_QOS_0;
    subscriptions[0].cb = on_pub;
    strcpy(topics[0], MQTT_TOPIC_IN);
    subscriptions[0].topic.name = MQTT_TOPIC_IN;

    if (emcute_sub(&subscriptions[0], flags) != EMCUTE_OK) {
        printf("error: unable to subscribe to %s\n", MQTT_TOPIC_IN);
        return 1;
    }

    printf("Now subscribed to %s\n", MQTT_TOPIC_IN);

    return 0;
}

int curr_lux = -1;

int curr_led = -1;
int toggle_rgbled(int code) {
    /* Avoid re-triggering the same action */
    if (curr_led == code)
        return 0;
    curr_led = code;

    /* Clear the colors before setting them again */
    gpio_clear(LED0_PIN);
    gpio_clear(LED1_PIN);
    gpio_clear(LED2_PIN);

    switch (code) {
        case TEMP_TOO_HIGH:  // Red
            gpio_set(LED0_PIN);
            break;
        case TEMP_TOO_LOW:  // Orange [Blue - not present in IoT Lab]
            gpio_set(LED1_PIN);
            break;
        case TEMP_OK:  // Green         /* IDEA: might use Yellow for major inconsistencies */
            gpio_set(LED2_PIN);
            break;
    }

    return 0;
}

int init_actuators(void) {
    gpio_init(LED0_PIN, GPIO_OUT);
    gpio_init(LED1_PIN, GPIO_OUT);
    gpio_init(LED2_PIN, GPIO_OUT);
    gpio_set(LED0_PIN);
    gpio_set(LED1_PIN);
    gpio_set(LED2_PIN);

    return 0;
}

int last_lux = -1;
void *measure_light(void *arg) {
    (void)arg;

    int lux = 0;

    int avg = 0;
    const int iterations = LIGHT_ITER;
    int i = 0;

    printf("Sampling light...\n");
    while (i < iterations) {
        lux += isl29020_read(&dev);

        avg += lux;

        i++;
        xtimer_sleep(LIGHT_SLEEP_TIME);
    }

    avg /= iterations;
    avg = round(avg);
    uint32_t uavg = avg;

    last_lux = avg;

    printf("Average lux after %d iterations at a %d-second(s) interval: %lu\n", iterations, LIGHT_SLEEP_TIME, uavg);

    msg_t msg;
    /* Signal to the main thread that this thread's execution has finished */
    msg.content.value = uavg;
    msg_send(&msg, tmain);

    return NULL;
}

int last_temp = -1;
void *measure_temp(void *arg) {
    (void)arg;

    int16_t dtemp = 0;
    lpsxxx_read_temp(&lpsxxx, &dtemp);    

    dtemp = dtemp/100;

    uint32_t utemp;
    if (dtemp <= 0) { /* Treat negative temperatures as inadmissible (room temperature <= 0 is LOW) */
        utemp = 0;
    } else {
        utemp = dtemp;
    }

    last_temp = dtemp;

    /* Signal to the main thread that this thread's execution has finished */
    msg_t msg;
    msg.content.value = utemp;
    msg_send(&msg, tmain);

    return NULL;

}

int started = -1;

void *main_loop(void *arg) {
    (void)arg;
    tmain = thread_getpid();

    xtimer_ticks32_t last = xtimer_now();

    while (1) {
        /* Measure the ambient light on a different thead in order to concurrently
   * measure the temperature and use the corresponding actuator(s). */
        t1 = thread_create(stack_lux, sizeof(stack_lux), THREAD_PRIORITY_MAIN - 1,
                           THREAD_CREATE_STACKTEST, measure_light, NULL, "light_check");

        /* Measure the ambient temperature on a different thead in order to
   * concurrently measure the temperature and use the corresponding actuator(s). */
        t2 = thread_create(stack_temp, sizeof(stack_temp), THREAD_PRIORITY_MAIN - 3,
                           THREAD_CREATE_STACKTEST, measure_temp, NULL, "temp_check");

        //puts("THREADS CREATED");
        //printf("%hd\t%hd\t%hd\n", tmain, t1, t2);

        msg_t msg1, msg2;

        uint32_t lux = 0;
        uint32_t temp = 0;
        // Wait for the 2 threads to finish their execution
        msg_receive(&msg1);
        if (msg1.sender_pid == t1) {  //Message coming from light measurer
            lux = msg1.content.value;
        } else {
            temp = msg1.content.value;
        }

        //puts("msg1 received\n");
        msg_receive(&msg2);
        if (msg2.sender_pid == t2) {  // Message coming from temperature measurer
            temp = msg2.content.value;
        } else {
            lux = msg2.content.value;
        }

        
        printf("LUX: %lu\n", lux);
        printf("TEMP: %lu\n", temp);
        //puts("msg2 received\n");
        
        char core_str[40];
        sprintf(core_str, "{\"id\":\"%s\",\"lux\":\"%lu\",\"temp\":\"%lu\",\"lamp\":\"%d\",\"led\":\"%d\"}", EMCUTE_ID, lux, temp, curr_lux, curr_led);
        pub(MQTT_TOPIC, core_str, 0);

        xtimer_periodic_wakeup(&last, DELAY);
    }
}

static void _lpsxxx_usage(char *cmd)
{
    printf("usage: %s <temperature|pressure>\n", cmd);
}


static int isl29020_handler(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("Light value: %5i LUX\n", isl29020_read(&dev));

    return 0;
} 

static int lpsxxx_handler(int argc, char *argv[]) {
    if (argc < 2) {
        _lpsxxx_usage(argv[0]);
        return -1;
    }

    if (!strcmp(argv[1], "temperature")) {
        int16_t temp = 0;
        lpsxxx_read_temp(&lpsxxx, &temp);
        printf("Temperature: %i.%u°C\n", (temp / 100), (temp % 100));
    }
    else if (!strcmp(argv[1], "pressure")) {
        uint16_t pres = 0;
        lpsxxx_read_pres(&lpsxxx, &pres);
        printf("Pressure: %uhPa\n", pres);
    }
    else {
        _lpsxxx_usage(argv[0]);
        return -1;
    }

    return 0;
}

static int cmd_toggle_led(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Please provide the led code: {0, 1, 2}");
        return -1;
    }

    int code = atoi(argv[1]);
    printf("Setting led to: %d\n", code);
    toggle_rgbled(code);

    return 0;
}

int init_sensors(void) {
    int res = 0;

    lpsxxx_init(&lpsxxx, &lpsxxx_params[0]);

    if (isl29020_init(&dev, &isl29020_params[0]) != 0) {
        res = 1;
    }

    return res;
}

static int _board_handler(int argc, char **argv)
{
    /* These parameters are not used, avoid a warning during build */
    (void)argc;
    (void)argv;

    puts(RIOT_BOARD);

    return 0;
}

static int _cpu_handler(int argc, char **argv)
{
    /* These parameters are not used, avoid a warning during build */
    (void)argc;
    (void)argv;

    puts(RIOT_CPU);

    return 0;
}

static int cmd_status(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("%d %d %d\n", last_lux, last_temp, curr_led);

    return 0;
} 

static const shell_command_t shell_commands[] = {
    { "status", "get a status report", cmd_status },
    { "led", "toggle led", cmd_toggle_led },
    { "isl", "read the isl29020 values", isl29020_handler },
    { "lps", "read the lps331ap values", lpsxxx_handler },
    { "board", "Print the board name", _board_handler },
    { "cpu", "Print the cpu name", _cpu_handler },
    { NULL, NULL, NULL }
};

int main(void) {

    puts("Setting up ethos and emcute");
    setup_mqtt();

    printf("Initializing sensors\n");
    int sensors_status = init_sensors();

    if (sensors_status == 0)
        printf("All sensors initialized successfully!\n");
    else {
        printf("An error occurred while initializing some sensors, error code: %d\n", sensors_status);
        return 1;
    }

    printf("Initializing actuators\n");
    int actuators_status = init_actuators();
    

    if (actuators_status == 0)
        puts("All actuators initialized successfully!");
    else {
        printf("An error occurred while initializing some actuators, error code: %d\n", actuators_status);
        return 2;
    }

    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("RIOT network stack example application");
    

    puts("Starting main_loop thread...");
    /* Perform sensor readings on a separate thread in order to host a shell on the main thread*/
    thread_create(stack_loop, sizeof(stack_loop), EMCUTE_PRIO, 0, main_loop, NULL, "main_loop");
    puts("Thread started successfully!");    

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}