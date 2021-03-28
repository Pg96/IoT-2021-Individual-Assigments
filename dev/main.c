/**
 * @{
 *
 * @brief       Light & Temperature measurer
 *
 * @author      Giacomo Priamo <priamo.1701568@studenti.uniroma1.it>
 *
 * @}
 */

#include <math.h>
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

#define LIGHT_ITER 5

#define PM_MODE 0
#define PM_DELAY 5

#define DELAY1 (1000LU * US_PER_MS) /* 100 ms */
#define DELAY2 (3000LU * US_PER_MS) /* 100 ms */

char stack_lux[THREAD_STACKSIZE_MAIN];
char stack_temp[THREAD_STACKSIZE_MAIN];

kernel_pid_t tmain, t1, t2;

dht_t dev;

static void callback_rtc(void *arg) { puts(arg); }

int enable_led(void) {
    gpio_t pin_out = GPIO_PIN(PORT_B, 5);
    if (gpio_init(pin_out, GPIO_OUT)) {
        printf("Error to initialize GPIO_PIN(%d %d)\n", PORT_B, 5);
        return -1;
    }

    printf("Set pin to HIGH\n");
    gpio_set(pin_out);
    xtimer_sleep(2);
    printf("Set pin to LOW\n");
    gpio_clear(pin_out);

    return 0;
}

int enable_rgbled(int code) {
    gpio_t pin_org = GPIO_PIN(PORT_B, 6);  // R
    gpio_t pin_yel = GPIO_PIN(PORT_C, 7);  // G
    gpio_t pin_blu = GPIO_PIN(PORT_A, 9);  // B

    printf("Trying to initialize leds\n");

    if (gpio_init(pin_org, GPIO_OUT)) {
        printf("Error to initialize GPIO_PIN(%d %d)\n", PORT_B, 6);
        return -1;
    }
    if (gpio_init(pin_yel, GPIO_OUT)) {
        printf("Error to initialize GPIO_PIN(%d %d)\n", PORT_C, 7);
        return -1;
    }
    if (gpio_init(pin_blu, GPIO_OUT)) {
        printf("Error to initialize GPIO_PIN(%d %d)\n", PORT_A, 9);
        return -1;
    }

    switch (code) {
        case 0:  // Red
            gpio_set(pin_org);
            xtimer_sleep(2);
            gpio_clear(pin_org);
            break;
        case 1:  // Yellow
            gpio_set(pin_org);
            gpio_set(pin_yel);
            xtimer_sleep(2);
            gpio_clear(pin_org);
            gpio_clear(pin_yel);
            break;
        case 2:
            gpio_set(pin_yel);
            xtimer_sleep(2);
            //gpio_clear(pin_yel);
            break;
    }

    return 0;
}

void *measure_light(void *arg) {
    (void)arg;

    int sample = 0;
    int lux = 0;

    xtimer_ticks32_t last = xtimer_now();

    int avg = 0;
    const int iterations = LIGHT_ITER;
    int i = 0;

    while (i < iterations) {
        sample = adc_sample(ADC_IN_USE, ADC_RES);
        lux = adc_util_map(sample, ADC_RES, 10, 100);
        printf("Sampling");
        if (sample < 0) {
            printf("ADC_LINE(%u): selected resolution not applicable\n", ADC_IN_USE);
        } else {
            printf("ADC_LINE(%u): raw value: %i, lux: %i\n", ADC_IN_USE, sample, lux);

            avg += lux;
        }
        i++;
        xtimer_periodic_wakeup(&last, DELAY1);
    }

    avg /= iterations;
    avg = round(avg);
    printf("Avg: %d\n", avg);

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

    enable_rgbled(2);

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
   * measure the temperature and use the corresponding actuator(s). */
    t1 = thread_create(stack_lux, sizeof(stack_lux), THREAD_PRIORITY_MAIN - 1,
                       THREAD_CREATE_STACKTEST, measure_light, NULL, "light_check");

    /* Measure the ambient temperature on a different thead in order to
   * concurrently measure the temperature and use the corresponding actuator(s). */
    t2 = thread_create(stack_temp, sizeof(stack_temp), THREAD_PRIORITY_MAIN - 3,
                       THREAD_CREATE_STACKTEST, measure_temp, NULL, "temp_check");

    puts("THREADS CREATED");
    //printf("%hd\t%hd\t%hd\n", tmain, t1, t2);

    msg_t msg1, msg2;

    // Wait for the 2 threads to finish their execution
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
