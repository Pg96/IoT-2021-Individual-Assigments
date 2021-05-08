/**
 * @{
 *
 * @brief       Light & Temperature measurer. Sensor readings are sent to the cloud (aws IoT Core)
 *              which will command the activation of the actuators (lamp and rgb led+buzzer)
 *
 * @author      Giacomo Priamo <priamo.1701568@studenti.uniroma1.it>
 *
 * @}
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "analog_util.h"
#include "dht.h"
#include "dht_params.h"
#include "fmt.h"
#include "jsmn.h"
#include "msg.h"
#include "net/emcute.h"
#include "net/ipv6/addr.h"
#include "periph/adc.h"
#include "periph/gpio.h"
#include "periph/pm.h"
#include "periph/rtc.h"
#include "shell.h"
#include "thread.h"
#include "xtimer.h"

#define JSMN_HEADER

/* Sensors Section */
#define ADC_IN_USE ADC_LINE(0)
#define ADC_RES ADC_RES_12BIT

#define LIGHT_ITER 5 /* Light measurement - number of iterations */

// #define PM_MODE 0  /* Power  Management mode */
// #define PM_DELAY 5 /* Power Management Wake-up delay */

#define DELAY (60000LU * US_PER_MS) /* 1 minute - Delay between main_loop() iterations */
//60000LU = 1 minute ; 300000LU = 5 minutes

#define TEMP_SLEEP_TIME 2  /* Determines the duration of the buzzer's sound */
#define LIGHT_SLEEP_TIME 1 /* Determines the sleep time (60 seconds) between subsequent iterations in the measure_light() loop */

#define TEMP_TOO_LOW 1
#define TEMP_TOO_HIGH 0
#define TEMP_OK 2

#define LAMP_ON 1
#define LAMP_OFF 0

/* MQTT SECTION */
#ifndef EMCUTE_ID
#define EMCUTE_ID ("power_saver_0")
#endif
#define EMCUTE_PRIO (THREAD_PRIORITY_MAIN - 1)

#define _IPV6_DEFAULT_PREFIX_LEN (64U)

#define NUMOFSUBS (16U)
#define TOPIC_MAXLEN (64U)

#define MQTT_TOKENS 6 /* The number of tokens (key-value) that will be received by the IoT Core */

/* [Sensors] Stacks for multi-threading & tids placeholders*/
char stack_loop[THREAD_STACKSIZE_MAIN];

#if NUCLEO == 1
char stack_lux[THREAD_STACKSIZE_MAIN];
char stack_temp[THREAD_STACKSIZE_MAIN];

/* Actuators' pins */
const gpio_t pin_org = GPIO_PIN(PORT_B, 6);  // R
const gpio_t pin_yel = GPIO_PIN(PORT_C, 7);  // G
const gpio_t pin_blu = GPIO_PIN(PORT_A, 9);  // B

const gpio_t lamp_pin = GPIO_PIN(PORT_A, 8);

const gpio_t buzz_pin = GPIO_PIN(PORT_B, 5);

/* DHT11 device */
dht_t dev;
#endif 

/* Threads' IDs */
kernel_pid_t tmain, t1, t2;

#if NUCLEO == 1
/* [Emcute - MQTT] Stack and  vars */
static char stack_emcute[THREAD_STACKSIZE_DEFAULT];
//static msg_t queue[8];

static emcute_sub_t subscriptions[NUMOFSUBS];
static char topics[NUMOFSUBS][TOPIC_MAXLEN];
#endif

#if NUCLEO == 1
static void *emcute_thread(void *arg) {
    (void)arg;
    emcute_run(CONFIG_EMCUTE_DEFAULT_PORT, EMCUTE_ID);
    return NULL; /* should never be reached */
}
#endif

#if NUCLEO == 1
int toggle_lamp(int code);
int toggle_rgbled(int code);
#endif 

int parse_val(jsmntok_t key, char *command) {
    unsigned int length = key.end - key.start;
    char keyString[length + 1];
    memcpy(keyString, &command[key.start], length);
    keyString[length] = '\0';
    //printf("Val: %s\n", keyString);

    int val = atoi(keyString);

    return val;
}

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

#if NUCLEO == 1
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

            toggle_lamp(val);

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
#endif 
    return 0;
}

#if NUCLEO == 1
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
#endif 

#if NUCLEO == 1
static uint8_t get_prefix_len(char *addr) {
    int prefix_len = ipv6_addr_split_int(addr, '/', _IPV6_DEFAULT_PREFIX_LEN);

    if (prefix_len < 1) {
        prefix_len = _IPV6_DEFAULT_PREFIX_LEN;
    }

    return prefix_len;
}

static int netif_add(char *iface_name, char *addr_str) {
    netif_t *iface = netif_get_by_name(iface_name);
    if (!iface) {
        puts("error: invalid interface given");
        return 1;
    }
    enum {
        _UNICAST = 0,
        _ANYCAST
    } type = _UNICAST;

    ipv6_addr_t addr;
    uint16_t flags = GNRC_NETIF_IPV6_ADDRS_FLAGS_STATE_VALID;
    uint8_t prefix_len;

    prefix_len = get_prefix_len(addr_str);

    if (ipv6_addr_from_str(&addr, addr_str) == NULL) {
        puts("error: unable to parse IPv6 address.");
        return 1;
    }

    if (ipv6_addr_is_multicast(&addr)) {
        if (netif_set_opt(iface, NETOPT_IPV6_GROUP, 0, &addr,
                          sizeof(addr)) < 0) {
            printf("error: unable to join IPv6 multicast group\n");
            return 1;
        }
    } else {
        if (type == _ANYCAST) {
            flags |= GNRC_NETIF_IPV6_ADDRS_FLAGS_ANYCAST;
        }
        flags |= (prefix_len << 8U);
        if (netif_set_opt(iface, NETOPT_IPV6_ADDR, flags, &addr,
                          sizeof(addr)) < 0) {
            printf("error: unable to add IPv6 address\n");
            return 1;
        }
    }

    printf("success: added %s/%d to interface ", addr_str, prefix_len);
    printf("\n");

    return 0;
}
#endif 

#if NUCLEO == 1
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
    netif_add("4", NUCLEO_ADDR);
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
#endif 
//static void callback_rtc(void *arg) { puts(arg); }

#if NUCLEO == 1
int init_actuators(void) {
    /* Initialize lamp pin */
    if (gpio_init(lamp_pin, GPIO_OUT)) {
        printf("An error occurred while trying to initialize GPIO_PIN(%d %d)\n", PORT_B, 5);
        return 1;
    }

    /* Initialize buzzer pin */
    if (gpio_init(buzz_pin, GPIO_OUT)) {
        printf("An error occurred while trying to initialize GPIO_PIN(%d %d)\n", PORT_B, 5);
        return 2;
    }

    /* Initialize RGB led pins */

    printf("Trying to initialize leds\n");  // With the current conf (while on main), might even move this away

    if (gpio_init(pin_org, GPIO_OUT)) {
        printf("An error occurred while trying to initialize GPIO_PIN(%d %d)\n", PORT_B, 6);
        return 3;
    }
    if (gpio_init(pin_yel, GPIO_OUT)) {
        printf("An error occurred while trying to initialize GPIO_PIN(%d %d)\n", PORT_C, 7);
        return 3;
    }
    if (gpio_init(pin_blu, GPIO_OUT)) {
        printf("An error occurred while trying to initialize GPIO_PIN(%d %d)\n", PORT_A, 9);
        return 3;
    }

    return 0;
}

int curr_lux = 0;
/* Sensors & Actuators */
int toggle_lamp(int code) {
    /* Avoid re-triggering the current action */
    if (curr_lux == code)
        return 0;

    if (code == LAMP_ON) 
        gpio_set(lamp_pin);
    else
        gpio_clear(lamp_pin);
    
    curr_lux = code;

    return 0;
}
#endif 

# if NUCLEO == 1
void *measure_light(void *arg) {
    (void)arg;

    int sample = 0;
    int lux = 0;

    int avg = 0;
    const int iterations = LIGHT_ITER;
    int i = 0;

    printf("Sampling light...\n");
    while (i < iterations) {
        sample = adc_sample(ADC_IN_USE, ADC_RES);
        lux = adc_util_map(sample, ADC_RES, 10, 100);
        if (sample < 0) {
            printf("ADC_LINE(%u): selected resolution not applicable\n", ADC_IN_USE);
        } else {
            //printf("ADC_LINE(%u): raw value: %i, lux: %i\n", ADC_IN_USE, sample, lux);

            avg += lux;
        }
        i++;
        xtimer_sleep(LIGHT_SLEEP_TIME);
    }

    avg /= iterations;
    avg = round(avg);
    uint32_t uavg = avg;

    printf("Average lux after %d iterations at a %d-second(s) interval: %lu\n", iterations, LIGHT_SLEEP_TIME, uavg);

    msg_t msg;
    /* Signal to the main thread that this thread's execution has finished */
    msg.content.value = uavg;
    msg_send(&msg, tmain);

    return NULL;
}
#endif 

# if NUCLEO == 1
int toggle_buzzer(void) {
    // gpio_t pin_out = GPIO_PIN(PORT_B, 5);
    // if (gpio_init(pin_out, GPIO_OUT)) {
    //     printf("An error occurred while trying to initialize GPIO_PIN(%d %d)\n", PORT_B, 5);
    //     return -1;
    // }

    // Turn on the buzzer
    gpio_set(buzz_pin);

    xtimer_sleep(TEMP_SLEEP_TIME);

    // Turn off the buzzer
    gpio_clear(buzz_pin);

    return 0;
}
#endif

#if NUCLEO == 1
int curr_led = -1;
int toggle_rgbled(int code) {
    /* Avoid re-triggering the same action */
    if (curr_led == code)
        return 0;
    curr_led = code;

    /* Clear the colors before setting them again */
    gpio_clear(pin_org);
    gpio_clear(pin_yel);
    gpio_clear(pin_blu);

    switch (code) {
        case TEMP_TOO_HIGH:  // Red
            gpio_set(pin_org);
            toggle_buzzer();
            break;
        case TEMP_TOO_LOW:  // Blue
            gpio_set(pin_blu);
            toggle_buzzer();
            break;
        case TEMP_OK:  // Green         /* IDEA: might use Yellow for major inconsistencies */
            gpio_set(pin_yel);
            break;
    }

    return 0;
}
#endif 

#if NUCLEO == 1
void *measure_temp(void *arg) {
    (void)arg;

    /* Retrieve sensor reading */
    int16_t temp, hum;

    if (dht_read(&dev, &temp, &hum) != DHT_OK) {
        printf("An error occurred while reading values\n");
    }

    /* Extract + format temperature from sensor reading */
    char temp_s[10];
    size_t n = fmt_s16_dfp(temp_s, temp, -1);
    temp_s[n] = '\0';

    int dtemp = atoi(temp_s);
    uint32_t utemp;
    if (dtemp <= 0) { /* Treat negative temperatures as inadmissible (room temperature <= 0 is LOW) */
        utemp = 0;
    } else {
        utemp = dtemp;
    }
    /* Extract + format humidity from sensor reading */
    char hum_s[10];
    n = fmt_s16_dfp(hum_s, hum, -1);
    hum_s[n] = '\0';

    printf("DHT values - temp: %s°C - relative humidity: %s%%\n", temp_s, hum_s);

    /* Signal to the main thread that this thread's execution has finished */
    msg_t msg;
    msg.content.value = utemp;
    msg_send(&msg, tmain);

    return NULL;
}
#endif 


#if NUCLEO == 1
int init_sensors(void) {
    int res = 0;
    /* initialize the ADC line */
    if (adc_init(ADC_IN_USE) < 0) {
        printf("The attempt to initialize ADC_LINE(%u) failed\n", ADC_IN_USE);
        res |= 1;
    } else {
        printf("Successfully initialized ADC_LINE(%u)\n", ADC_IN_USE);
    }

    /* Fix port parameter for digital sensor */
    dht_params_t my_params;
    my_params.pin = GPIO_PIN(PORT_A, 10);
    my_params.type = DHT11;
    my_params.in_mode = DHT_PARAM_PULL;

    /* Initialize digital sensor */
    if (dht_init(&dev, &my_params) == DHT_OK) {
        printf("DHT sensor connected\n");
    } else {
        printf("Failed to connect to DHT sensor\n");
        res |= 2;
    }
    return res;
}
#endif 

#if NUCLEO == 1
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

        /* TODO: Reactivate once MQTT is set up
        char core_str[40];
        sprintf(core_str, "{\"id\":\"%s\",\"lux\":\"%lu\",\"temp\":\"%lu\",\"lamp\":\"%d\",\"led\":\"%d\"}", EMCUTE_ID, lux, temp, curr_lux, curr_led);
        pub(MQTT_TOPIC, core_str, 0);
        */

        xtimer_periodic_wakeup(&last, DELAY);
    }
}
#endif 

static int cmd_board(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("test\n");
    //const char* s2 = DEVICE;    
    //printf("DEVICE :%s\n",(DEVICE == 1)? s2 : "var is NULL");
    #if NUCLEO == 1
    printf("USING NUCLEO\n");
    #else 
    printf("USING M3\n");
    #endif
    return 0;
}

static int cmd_h(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("hello\n");
}

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];


static const shell_command_t shell_commands[] = {
    //    { "will", "register a last will", cmd_will },
    {"h", "send help", cmd_h},
    {"board", "gets BOARD env var value", cmd_board},
    {NULL, NULL, NULL}};

int main(void) {
    /** IoT 2021 -- Individual assigment
     *  Measures the intensity of ambient light using a photocell
     *  and the ambient temperature using a DHT11 sensor.
     */

    #if NUCLEO == 1
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
    #endif

    #if NUCLEO == 1
    printf("Initializing actuators\n");
    int actuators_status = init_actuators();
    

    if (actuators_status == 0)
        puts("All actuators initialized successfully!");
    else {
        printf("An error occurred while initializing some actuators, error code: %d\n", actuators_status);
        return 2;
    }
    #endif

    #if NUCLEO == 1
    puts("Starting main_loop thread...");
    /* Perform sensor readings on a separate thread in order to host a shell on the main thread*/
    thread_create(stack_loop, sizeof(stack_loop), EMCUTE_PRIO, 0, main_loop, NULL, "main_loop");
    puts("Thread started successfully!");
    #endif 

    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("RIOT network stack example application");
    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}