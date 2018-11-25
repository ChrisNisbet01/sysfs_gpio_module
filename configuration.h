#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include <stddef.h>
#include <stdbool.h>

typedef struct configuration_st configuration_st;

configuration_st * configuration_load(char const * const filename);
void configuration_free(configuration_st const * const configuration);

size_t configuration_num_inputs(configuration_st const * const configuration);

size_t configuration_num_outputs(configuration_st const * const configuration);

bool configuration_input_gpio_number(
    configuration_st const * const configuration,
    size_t const input_number,
    size_t * const gpio_number);

bool configuration_output_gpio_number(
    configuration_st const * const configuration,
    size_t const output_number,
    size_t * const gpio_number);


#endif /* __CONFIGURATION_H__ */
