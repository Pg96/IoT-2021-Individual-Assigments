#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>


#include "jsmn.h"
#include "msg.h"
#include "thread.h"

#include "timex.h"
#include "xtimer.h"
#include "shell.h"
#include "lpsxxx.h"
#include "lpsxxx_params.h"
#include "isl29020.h"
#include "isl29020_params.h"

#define LIGHT_ITER 5 /* Light measurement - number of iterations */

#define DELAY (60000LU * US_PER_MS) /* 1 minute - Delay between main_loop() iterations */
//60000LU = 1 minute ; 300000LU = 5 minutes

#define LIGHT_SLEEP_TIME 1 /* Determines the sleep time (60 seconds) between subsequent iterations in the measure_light() loop */

#define EMCUTE_PRIO (THREAD_PRIORITY_MAIN - 1)

/* MQTT SECTION */
#ifndef EMCUTE_ID
#define EMCUTE_ID ("power_saver_0")

//TODO: add missing stuff

#endif

/* [Sensors] Stacks for multi-threading & tids placeholders*/
char stack_loop[THREAD_STACKSIZE_MAIN];
char stack_lux[THREAD_STACKSIZE_MAIN];
char stack_temp[THREAD_STACKSIZE_MAIN];

/* Threads' IDs */
kernel_pid_t tmain, t1, t2;

/* Light and temperature sensors */
static lpsxxx_t lpsxxx;
isl29020_t dev;

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

        //char core_str[40];
        //sprintf(core_str, "{\"id\":\"%s\",\"lux\":\"%lu\",\"temp\":\"%lu\",\"lamp\":\"%d\",\"led\":\"%d\"}", EMCUTE_ID, lux, temp, curr_lux, curr_led);
        // TODO: enable once MQTT works
        //pub(MQTT_TOPIC, core_str, 0);

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

    printf("%d %d %d\n", started, last_lux, last_temp);

    return 0;
} 

static const shell_command_t shell_commands[] = {
    { "status", "get a status report", cmd_status },
    { "isl", "read the isl29020 values", isl29020_handler },
    { "lps", "read the lps331ap values", lpsxxx_handler },
    { "board", "Print the board name", _board_handler },
    { "cpu", "Print the cpu name", _cpu_handler },
    { NULL, NULL, NULL }
};

int main(void) {

    printf("Initializing sensors\n");
    int sensors_status = init_sensors();

    if (sensors_status == 0)
        printf("All sensors initialized successfully!\n");
    else {
        printf("An error occurred while initializing some sensors, error code: %d\n", sensors_status);
        return 1;
    }

    // TODO: init actuators

    puts("Starting main_loop thread...");
    /* Perform sensor readings on a separate thread in order to host a shell on the main thread*/
    thread_create(stack_loop, sizeof(stack_loop), EMCUTE_PRIO, 0, main_loop, NULL, "main_loop");
    puts("Thread started successfully!");    

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}