#ifndef __SYSFS_GPIO_MODULE_H__
#define __SYSFS_GPIO_MODULE_H__

#include "configuration.h"

#include <stdbool.h>
#include <stddef.h>

int
GPIORead(int const pin, bool * const state);

int
GPIOWrite(int const pin, bool const high);

bool enable_gpio_pins(configuration_st const * const configuration);
void disable_gpio_pins(configuration_st const * const configuration);


#endif /* __SYSFS_GPIO_MODULE_H__ */
