#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "timex.h"
#include "xtimer.h"
#include "shell.h"
#include "lpsxxx.h"
#include "lpsxxx_params.h"

static lpsxxx_t lpsxxx;


static void _lpsxxx_usage(char *cmd)
{
    printf("usage: %s <temperature|pressure>\n", cmd);
}

static int lpsxxx_handler(int argc, char *argv[])
{
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

static const shell_command_t shell_commands[] = {
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

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}