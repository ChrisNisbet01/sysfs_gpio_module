/* Taken from https://elinux.org/RPi_GPIO_Code_Samples#sysfs */
#include "sysfs_gpio_module.h"
#include "ubus_common.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct
{
    bool outgoing;
    size_t gpio;
} gpio_definition_st;

static gpio_definition_st const gpio_definitions[] =
{
    {
        .outgoing = false,
        .gpio = 0
    },
    {
        .outgoing = false,
        .gpio = 1
    },
    {
        .outgoing = false,
        .gpio = 2
    },
    {
        .outgoing = false,
        .gpio = 3
    },
    {
        .outgoing = false,
        .gpio = 4
    },
    {
        .outgoing = false,
        .gpio = 5
    },
    {
        .outgoing = false,
        .gpio = 6
    },
    {
        .outgoing = false,
        .gpio = 7
    },
    {
        .outgoing = true,
        .gpio = 8
    },
    {
        .outgoing = true,
        .gpio = 9
    },
    {
        .outgoing = true,
        .gpio = 10
    },
    {
        .outgoing = true,
        .gpio = 11
    },
    {
        .outgoing = true,
        .gpio = 12
    },
    {
        .outgoing = true,
        .gpio = 13
    },
    {
        .outgoing = true,
        .gpio = 14
    },
    {
        .outgoing = true,
        .gpio = 15
    }
};
#define NUM_GPIO_DEFINITIONS ARRAY_SIZE(gpio_definitions)

#define GPIO_BASE_PATH "/sys/class/gpio"

static size_t const first_output_pin = 8;

#define BUFFER_MAX 10
static int
GPIOExport(int const pin)
{
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

    fd = open(GPIO_BASE_PATH "/export", O_WRONLY);
	if (-1 == fd) 
    {
		fprintf(stderr, "Failed to open export for writing!\n");
		return -1;
	}

	snprintf(buffer, sizeof buffer, "%d", pin);
	write(fd, buffer, strlen(buffer));
	close(fd);

	return 0;
}

static int
GPIOUnexport(int const pin)
{
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

    fd = open(GPIO_BASE_PATH "/unexport", O_WRONLY);
	if (-1 == fd) 
    {
		fprintf(stderr, "Failed to open unexport for writing!\n");
		return -1;
	}

    snprintf(buffer, sizeof buffer, "%d", pin);
    write(fd, buffer, strlen(buffer));
	close(fd);

    return 0;
}

static int
GPIODirection(int pin, bool const outgoing)
{
#define DIRECTION_MAX 35
	char path[DIRECTION_MAX];
	int fd;
    int result;

    snprintf(path, DIRECTION_MAX, GPIO_BASE_PATH "/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) 
    {
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		result = -1;
        goto done;
	}

    char const * const direction_str = outgoing ? "out" : "in";

    if (-1 == write(fd, direction_str, strlen(direction_str)))
    {
        fprintf(stderr, "Failed to set direction!\n");
        result = -1;
        goto done;
    }

    result = 0;

done:
    if (fd >= 0)
    {
        close(fd);
    }

	return result;
}

int
GPIORead(int const pin, bool * const state)
{
#define VALUE_MAX 30
	char path[VALUE_MAX];
	char value_str[3];
	int fd;
    int result;

    snprintf(path, VALUE_MAX, GPIO_BASE_PATH "/gpio%d/value", pin);
	fd = open(path, O_RDONLY);
	if (fd < 0) 
    {
		fprintf(stderr, "Failed to open gpio value for reading!\n");
        result = -1;
        goto done;
	}

	if (read(fd, value_str, sizeof value_str) < 0) 
    {
		fprintf(stderr, "Failed to read value!\n");
        result = -1;
        goto done;
    }

    *state = atoi(value_str);
    result = 0;

done:
    if (fd >= 0)
    {
        close(fd);
    }

	return result;
}

int
GPIOWrite(int const pin, bool const high)
{
	char path[VALUE_MAX];
	int fd;
    int result;

    snprintf(path, VALUE_MAX, GPIO_BASE_PATH "/gpio%d/value", pin + first_output_pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) 
    {
		fprintf(stderr, "Failed to open gpio value for writing!\n");
        result = -1;
        goto done;
	}

	if (1 != write(fd, high ? "1" : "0", 1)) 
    {
		fprintf(stderr, "Failed to write value!\n");
        result = -1;
        goto done;
    }

    result = 0;

done:
    if (fd >= 0)
    {
        close(fd);
    }

	return result;
}

static bool 
configure_gpio(size_t const gpio_number, bool const outgoing)
{
    bool success;

    if (GPIOExport(gpio_number) < 0)
    {
        success = false;
        goto done;
    }

    /*
     * Set GPIO direction.
     */
    if (GPIODirection(gpio_number, outgoing) < 0)
    {
        success = false;
        goto done;
    }

    success = true;

done:
    return success;
}

static bool 
enable_inputs(configuration_st const * const configuration)
{
    bool success;

    for (size_t index = 0; index < configuration_num_inputs(configuration); index++)
    {
        size_t gpio_number;

        if (!configuration_input_gpio_number(configuration, index, &gpio_number))
        {
            success = false;
            goto done;
        }
        configure_gpio(gpio_number, false);
    }

    success = true;

done:
    return success;
}

static void
disable_inputs(configuration_st const * const configuration)
{
    for (size_t index = 0; index < configuration_num_inputs(configuration); index++)
    {
        size_t gpio_number;

        if (!configuration_input_gpio_number(configuration, index, &gpio_number))
        {
            continue;
        }
        GPIOUnexport(gpio_number);
    }
}

static bool 
enable_outputs(configuration_st const * const configuration)
{
    bool success;

    for (size_t index = 0; index < configuration_num_outputs(configuration); index++)
    {
        size_t gpio_number;

        if (!configuration_output_gpio_number(configuration, index, &gpio_number))
        {
            success = false;
            goto done;
        }
        configure_gpio(gpio_number, true);
    }

    success = true;

done:
    return success;
}

static void
disable_outputs(configuration_st const * const configuration)
{
    for (size_t index = 0; index < configuration_num_outputs(configuration); index++)
    {
        size_t gpio_number;

        if (!configuration_output_gpio_number(configuration, index, &gpio_number))
        {
            continue;
        }
        GPIOUnexport(gpio_number);
    }
}

bool enable_gpio_pins(configuration_st const * const configuration)
{
    bool success;

    if (!enable_inputs(configuration))
    {
        success = false;
        goto done;
    }

    if (!enable_outputs(configuration))
    {
        success = false;
        goto done;
    }

    success = true;

done:
    return success;
}

void disable_gpio_pins(configuration_st const * const configuration)
{
    disable_inputs(configuration);
    disable_outputs(configuration);
}

int gpio_pin_from_index(size_t const index)
{
    int gpio_pin;

    if (index >= NUM_GPIO_DEFINITIONS)
    {
        gpio_pin = -1;
    }

    gpio_definition_st const * const gpio_definition = &gpio_definitions[index]; 

    return gpio_definition->gpio;
}
