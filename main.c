#include "daemonize.h"
#include "sysfs_gpio_module.h"
#include "ubus.h"
#include "ubus_server.h"
#include "configuration.h"
#include "debug.h"

#include <libubox/uloop.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>

static void usage(char const * const program_name)
{
    fprintf(stdout, "Usage: %s [options] <listening_socket_name>\n", program_name);
    fprintf(stdout, "\n");
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "  -d %-21s %s\n", "", "Run as a daemon");
    fprintf(stdout, "  -s %-21s %s\n", "ubus_socket", "UBUS socket name");
    fprintf(stdout, "  -c %-21s %s\n", "config", "Configuration filename");
}

int main(int argc, char * * argv)
{
    bool daemonise = false;
    int daemonise_result;
    int exit_code;
    unsigned int args_remaining;
    int option;
    char const * path = NULL;
    char const * configuration_filename = NULL;

    while ((option = getopt(argc, argv, "c:s:?d")) != -1)
    {
        switch (option)
        {
            case 'c':
                configuration_filename = optarg;
                break;
            case 's':
                path = optarg;
                break;
            case 'd':
                daemonise = true;
                break;
            case '?':
                usage(basename(argv[0]));
                exit_code = EXIT_SUCCESS;
                goto done;
        }
    }

    if (daemonise)
    {
        daemonise_result = daemonize(NULL, NULL, NULL);
        if (daemonise_result < 0)
        {
            fprintf(stderr, "Failed to daemonise. Exiting\n");
            exit_code = EXIT_FAILURE;
            goto done;
        }
        if (daemonise_result == 0)
        {
            /* This is the parent process, which can exit now. */
            exit_code = EXIT_SUCCESS;
            goto done;
        }
    }

    if (configuration_filename == NULL)
    {
        fprintf(stderr, "Configuration filename must be specified\n");
        exit_code = EXIT_FAILURE;
        goto done;
    }

    configuration_st const * const configuration =
        configuration_load(configuration_filename);

    if (configuration == NULL)
    {
        DPRINTF("Unable to load configuration file: %s\n", configuration_filename);
        exit_code = EXIT_FAILURE;
        goto done;
    }

    enable_gpio_pins(configuration);

    struct ubus_context * const ubus_ctx = gpio_ubus_initialise(path);

    if (ubus_ctx == NULL)
    {
        DPRINTF("\r\nfailed to initialise UBUS\n");
        exit_code = EXIT_FAILURE;
        goto done;
    }

    bool const ubus_server_initialised = gpio_ubus_server_initialise(ubus_ctx, configuration);

    if (!ubus_server_initialised)
    {
        DPRINTF("\r\nfailed to initialise UBUS server\n");
        exit_code = EXIT_FAILURE;
        goto done;
    }

    uloop_run();

    uloop_done(); 

    gpio_ubus_done();
    gpio_ubus_server_done();

    disable_gpio_pins(configuration);

    configuration_free(configuration);

    exit_code = EXIT_SUCCESS;

done:
    exit(exit_code);
}
