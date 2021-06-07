#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "net/loramac.h"     /* core loramac definitions */
#include "semtech_loramac.h" /* package API */
#include "hts221.h"
#include "hts221_params.h"

#include "jsmn.h"
#include "msg.h"
#include "periph/gpio.h"
#include "shell.h"
#include "thread.h"
#include "timex.h"
#include "xtimer.h"

#define CONFIG_EMCUTE_DEFAULT_PORT (1883U)

#define JSMN_HEADER

#define LIGHT_ITER 5 /* Light measurement - number of iterations */

#define DELAY (30000LU * US_PER_MS) /* 1 minute - Delay between main_loop() iterations */
//60000LU = 1 minute ; 300000LU = 5 minutes ; 180000LU

#define LIGHT_SLEEP_TIME 1 /* Determines the sleep time (60 seconds) between subsequent iterations in the measure_light() loop */

#define TEMP_TOO_LOW 1
#define TEMP_TOO_HIGH 0
#define TEMP_OK 2

#define MQTT_TOKENS 8 /* The number of tokens (key-value) that will be received by the IoT Core */

#define RECV_MSG_QUEUE (4U)
static msg_t _recv_queue[RECV_MSG_QUEUE];
static char _recv_stack[THREAD_STACKSIZE_DEFAULT];

static semtech_loramac_t loramac; /* The loramac stack descriptor */

static hts221_t hts221; /* The HTS221 device descriptor */

static const uint8_t deveui[LORAMAC_DEVEUI_LEN] = {0x98, 0x76, 0x54, 0x33, 0x22, 0x11, 0x98, 0x76};
static const uint8_t appeui[LORAMAC_APPEUI_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t appkey[LORAMAC_APPKEY_LEN] = {0x79, 0x9C, 0x10, 0x94, 0x37, 0xCC, 0xF6, 0xDF, 0x3E, 0x8D, 0xA5, 0x2A, 0xBF, 0x54, 0x5C, 0x78};

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static int mod_table[] = {0, 2, 1};

char *base64_encode(const char *data,
                    size_t input_length,
                    size_t *output_length) {
    *output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = malloc(*output_length);
    if (encoded_data == NULL) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}

static void *_recv(void *arg) {
    msg_init_queue(_recv_queue, RECV_MSG_QUEUE);
    (void)arg;
    while (1) {
        /* blocks until a message is received */
        semtech_loramac_recv(&loramac);
        loramac.rx_data.payload[loramac.rx_data.payload_len] = 0;
        printf("Data received: %s, port: %d\n",
               (char *)loramac.rx_data.payload, loramac.rx_data.port);

        /*
        char * msg = (char *)loramac.rx_data.payload;
        size_t inl = strlen(msg);
        printf("Trying to decode the message (len: %u )...\n", inl);
        size_t outl;
        //unsigned char* decoded =  base64_decode(msg, inl, &outl);
        
        printf("Result: ( %u )\n", outl);
        */
    }
    return NULL;
}

void send(char[] message) {
    //printf("VALUE: %d\n", val);
    /**
    char message[50];
    sprintf(message, "{\"id\":\"%s\",\"filling\":\"%d\"}", TTN_DEV_ID, val);
    */

    printf("Sending message '%s'\n", message);

    size_t inl = strlen(message);
    size_t outl;
    printf("Trying to encode the message (len: %u )...\n", inl);
    char *encoded = base64_encode(message, inl, &outl);
    printf("Result: %s (%u )\n", encoded, outl);
    /* send the message here */
    if (semtech_loramac_send(&loramac,
                             (uint8_t *)message, strlen(message)) != SEMTECH_LORAMAC_TX_DONE) {
        printf("Cannot send message '%s'\n", message);
    } else {
        printf("Message '%s' sent\n", message);
    }
}

int lora_init(void) {
    /* initialize the loramac stack */
    semtech_loramac_init(&loramac);

    /* configure the device parameters */
    semtech_loramac_set_deveui(&loramac, deveui);
    semtech_loramac_set_appeui(&loramac, appeui);
    semtech_loramac_set_appkey(&loramac, appkey);

    /* change datarate to DR5 (SF7/BW125kHz) */
    semtech_loramac_set_dr(&loramac, 5);

    /* start the OTAA join procedure */
    if (semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA) != SEMTECH_LORAMAC_JOIN_SUCCEEDED) {
        puts("Join procedure failed");
        return 1;
    }
    puts("Join procedure succeeded");

    puts("Starting recv thread");
    thread_create(_recv_stack, sizeof(_recv_stack),
               THREAD_PRIORITY_MAIN - 1, 0, _recv, NULL, "recv thread"); 


    return 0;
}

/* [Sensors] Stacks for multi-threading & tids placeholders*/
char stack_loop[THREAD_STACKSIZE_MAIN];
char stack_lux[THREAD_STACKSIZE_MAIN];
char stack_temp[THREAD_STACKSIZE_MAIN];

/* Threads' IDs */
kernel_pid_t tmain, t1, t2;

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

int dev_id = -1;
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

    // JSON STRUCT: {"id":"<k>", acts":"1|2"", [lux":"0|1"], ["led":"0|1|2"]} ('[]' mean optional)
    for (int i = 1; i < MQTT_TOKENS; i += 2) {
        jsmntok_t key = tokens[i];
        unsigned int length = key.end - key.start;
        char keyString[length + 1];
        memcpy(keyString, &command[key.start], length);
        keyString[length] = '\0';
        printf("Key: %s\n", keyString);

        if (strcmp(keyString, "id") == 0) {
            int val = parse_val(tokens[i + 1], command);

            if (val != dev_id) {
                printf("This message is not meant for me: %d", val);
                return 0;
            }
        }

        if (strcmp(keyString, "acts") == 0) {
            int val = parse_val(tokens[i + 1], command);

            if (val < 1 || val > 2) {
                printf("An invalid number of actuator commands was passed: %d", val);
                return 7;
            }

            activations = val;
        } else if (strcmp(keyString, "lux") == 0) {
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

    int max = 50;
    int min = 0;

    printf("Sampling light...\n");
    while (i < iterations) {
        //lux += isl29020_read(&dev);

        // TODO: deal with this in a better way
        lux = (rand() % (max+1-min)) + min;

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

    uint16_t humidity = 0;
    int16_t temperature = 0;
    if (hts221_read_humidity(&hts221, &humidity) != HTS221_OK) {
        puts("Cannot read humidity!");
    }
    if (hts221_read_temperature(&hts221, &temperature) != HTS221_OK) {
        puts("Cannot read temperature!");
    }

    /**
    char message[64];
    sprintf(message, "H: %d.%d%%, T:%d.%dC",
            (humidity / 10), (humidity % 10),
            (temperature / 10), (temperature % 10));
    printf("Sending message '%s'\n", message);
    */

    int16_t dtemp = temperature / 10;
    

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

uint32_t temp_high_threshold = -1;
uint32_t temp_low_threshold = -1;
uint32_t lux_threshold = -1;

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

        // TODO: may need to split these 2 (due to limited data that can be sent) [ PROB NOT ]
        if ((temp > temp_high_threshold || temp < temp_low_threshold) || lux >= lux_threshold) {
            sprintf(core_str, "{\"id\":\"%s\",\"lux\":\"%lu\",\"temp\":\"%lu\",\"lamp\":\"%d\",\"led\":\"%d\"}", TTN_DEV_ID, lux, temp, curr_lux, curr_led);
            
            send(core_str);

            printf("%s (%d)\n", core_str, strlen(core_str));
        }

        xtimer_periodic_wakeup(&last, DELAY);
    }
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

    if (hts221_init(&hts221, &hts221_params[0]) != HTS221_OK) {
        puts("Sensor initialization failed");
        return 1;
    }

    if (hts221_power_on(&hts221) != HTS221_OK) {
        puts("Sensor initialization power on failed");
        return 1;
    }

    if (hts221_set_rate(&hts221, hts221.p.rate) != HTS221_OK) {
        puts("Sensor continuous mode setup failed");
        return 1;
    }

    return res;
}

static int _board_handler(int argc, char **argv) {
    /* These parameters are not used, avoid a warning during build */
    (void)argc;
    (void)argv;

    puts(RIOT_BOARD);

    return 0;
}

static int _cpu_handler(int argc, char **argv) {
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
    {"status", "get a status report", cmd_status},
    {"led", "toggle led", cmd_toggle_led},
    {"board", "Print the board name", _board_handler},
    {"cpu", "Print the cpu name", _cpu_handler},
    {NULL, NULL, NULL}};

int main(void) {
    srand(time(NULL));

    printf("%s\n", TTN_DEV_ID);

    puts("Initializing lora");
    lora_init();

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

    puts("Starting main_loop thread...");
    /* Perform sensor readings on a separate thread in order to host a shell on the main thread*/
    thread_create(stack_loop, sizeof(stack_loop), THREAD_PRIORITY_MAIN, 0, main_loop, NULL, "main_loop");

    puts("Thread started successfully!");

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}