#include "daemonize.h"
#include "sysfs_gpio_module.h"
#include "ubus.h"
#include "configuration.h"
#include "debug.h"
#include <libubusgpio/libubusgpio.h>

#include <libubox/uloop.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>

configuration_st const * configuration;

static void usage(char const * const program_name)
{
    fprintf(stdout, "Usage: %s [options] <listening_socket_name>\n", program_name);
    fprintf(stdout, "\n");
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "  -d %-21s %s\n", "", "Run as a daemon");
    fprintf(stdout, "  -s %-21s %s\n", "ubus_socket", "UBUS socket name");
    fprintf(stdout, "  -c %-21s %s\n", "config", "Configuration filename");
}

static bool get_callback(
    void * const callback_ctx,
    char const * const io_type,
    size_t const instance,
    bool * const state)
{
    bool read_io;

    if (strcmp(io_type, "binary-input") != 0)
    {
        read_io = false;
        goto done;
    }

    if (instance >= configuration_num_inputs(configuration))
    {
        read_io = false;
        goto done;
    }

    size_t gpio_number;

    if (!configuration_input_gpio_number(
            configuration,
            instance,
            &gpio_number))
    {
        read_io = false;
        goto done;
    }

    read_io = GPIORead(gpio_number, state) == 0;

done:
    return read_io;
}

static bool set_callback(
    void * const callback_ctx,
    char const * const io_type,
    size_t const instance,
    bool const state)
{
    bool wrote_io;

    if (strcmp(io_type, "binary-output") != 0)
    {
        wrote_io = false;
        goto done;
    }

    if (instance >= configuration_num_outputs(configuration))
    {
        wrote_io = false;
        goto done;
    }

    size_t gpio_number;

    if (!configuration_output_gpio_number(
            configuration,
            instance,
            &gpio_number))
    {
        wrote_io = false;
        goto done;
    }

    wrote_io = GPIOWrite(gpio_number, state) == 0;

done:
    return wrote_io;
}

static void count_callback(
    void * const callback,
    append_count_callback_fn const append_callback,
    void * const append_ctx)
{
    append_callback(append_ctx, "binary-input", configuration_num_inputs(configuration));
    append_callback(append_ctx, "binary-output", configuration_num_outputs(configuration));
}

static ubus_gpio_server_handlers_st const ubus_gpio_server_handlers =
{
    .count_callback = count_callback,
    .get =
    {
        .get_callback = get_callback
    },
    .set =
    {
        .set_callback = set_callback
    }
};

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

    configuration =
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

    ubus_gpio_server_ctx_st * ubus_server_ctx =
        ubus_gpio_server_initialise(
            ubus_ctx,
            "sysfs.gpio",
            &ubus_gpio_server_handlers,
            NULL);

    uloop_run();

    ubus_gpio_server_done(ubus_ctx, ubus_server_ctx);

    uloop_done(); 

    gpio_ubus_done();

    disable_gpio_pins(configuration);

    configuration_free(configuration);

    exit_code = EXIT_SUCCESS;

done:
    exit(exit_code);
}
