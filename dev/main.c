/**
 * @{
 *
 * @brief       Light & Temperature measurer
 *
 * @author      Giacomo Priamo <priamo.1701568@studenti.uniroma1.it>
 *
 * @}
 */

#include <stdio.h>

#include "analog_util.h"
#include "dht.h"
#include "dht_params.h"
#include "fmt.h"
#include "msg.h"
#include "periph/adc.h"
#include "periph/gpio.h"
#include "periph/pm.h"
#include "periph/rtc.h"
#include "shell.h"
#include "thread.h"
#include "xtimer.h"

#define ADC_IN_USE ADC_LINE(0)
#define ADC_RES ADC_RES_12BIT

#define PM_MODE 0
#define PM_DELAY 5

char stack_lux[THREAD_STACKSIZE_MAIN];
char stack_temp[THREAD_STACKSIZE_MAIN];

kernel_pid_t tmain, t1, t2;

dht_t dev;

static void callback_rtc(void *arg) { puts(arg); }

void *measure_light(void *arg) {
    (void)arg;

    int sample = 0;
    int lux = 0;

    sample = adc_sample(ADC_IN_USE, ADC_RES);
    lux = adc_util_map(sample, ADC_RES, 10, 100);
    printf("Sampling");
    if (sample < 0) {
        printf("ADC_LINE(%u): selected resolution not applicable\n", ADC_IN_USE);
    } else {
        printf("ADC_LINE(%u): raw value: %i, lux: %i\n", ADC_IN_USE, sample, lux);
    }

    //puts("THREAD 1 end\n");
    msg_t msg;
    msg_send(&msg, tmain);

    return NULL;
}

void *measure_temp(void *arg) {
    (void)arg;

    /* Retrieve sensor reading */
    int16_t temp, hum;

    if (dht_read(&dev, &temp, &hum) != DHT_OK) {
        printf("Error reading values\n");
    }

    /* Extract + format temperature from sensor reading */
    char temp_s[10];
    size_t n = fmt_s16_dfp(temp_s, temp, -1);
    temp_s[n] = '\0';

    /* Extract + format humidity from sensor reading */
    char hum_s[10];
    n = fmt_s16_dfp(hum_s, hum, -1);
    hum_s[n] = '\0';

    printf("DHT values - temp: %s°C - relative humidity: %s%%\n", temp_s, hum_s);

    //puts("THREAD 2 end\n");
    msg_t msg;
    msg_send(&msg, tmain);

    return NULL;
}

int init_sensors(void) {
    int res = 0;

    /* initialize the ADC line */
    if (adc_init(ADC_IN_USE) < 0) {
        printf("Initialization of ADC_LINE(%u) failed\n", ADC_IN_USE);
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



int main(void) {
    /** IoT 2021 -- Individual assigment
     *  Measures the intensity of ambient light using a photocell
     *  and the ambient temperature using a DHT11 sensor.
     */

    printf("Initializing sensors\n");
    int sensors_status = init_sensors();

    if (sensors_status == 0)
        printf("All sensors initialized successfully!\n");
    else {
        printf("An error occurred while initializing some sensors, error code: %d\n", sensors_status);
        return 1;
    }

    tmain = thread_getpid();

    /* Measure the ambient light on a different thead in order to concurrently
   * measure the temperature. */
    t1 = thread_create(stack_lux, sizeof(stack_lux), THREAD_PRIORITY_MAIN - 1,
                       THREAD_CREATE_STACKTEST, measure_light, NULL, "light_check");

    /* Measure the ambient temperature on a different thead in order to
   * concurrently measure the temperature. */
    t2 = thread_create(stack_temp, sizeof(stack_temp), THREAD_PRIORITY_MAIN - 3,
                       THREAD_CREATE_STACKTEST, measure_temp, NULL, "temp_check");

    puts("THREADS CREATED");
    //printf("%hd\t%hd\t%hd\n", tmain, t1, t2);

    msg_t msg1, msg2;

    // Wait for the 2 threads to return
    msg_receive(&msg1);
    //puts("msg1 received\n");
    msg_receive(&msg2);
    //puts("msg2 received\n");

    /* Set an RTC-based alarm to trigger wakeup */
    const int mode = PM_MODE;
    const int delay = PM_DELAY;

    printf("Setting wakeup from mode %d in %d seconds.\n", mode, delay);
    fflush(stdout);

    struct tm time;
    rtc_get_time(&time);
    time.tm_sec += delay;
    rtc_set_alarm(&time, callback_rtc, "Wakeup alarm");

    /* Enter deep sleep mode */
    pm_set(mode);

    return 0;
}
