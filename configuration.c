#include "configuration.h"
#include "debug.h"

#include <json-c/json.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct gpio_input_st
{
    size_t gpio_number;
} gpio_input_st;

typedef struct gpio_input_context_st
{
    size_t num_gpio;
    gpio_input_st * gpios;
} gpio_input_context_st;

typedef struct gpio_output_st
{
    size_t gpio_number;
} gpio_output_st;

typedef struct gpio_output_context_st
{
    size_t num_gpio;
    gpio_output_st * gpios;
} gpio_output_context_st;

struct configuration_st
{
    struct json_object * json;
    gpio_input_context_st inputs;
    gpio_output_context_st outputs;
};

static struct json_object * get_object_by_name(
    struct json_object * const parent,
    char const * const name)
{
    struct json_object * object;

    if (!json_object_object_get_ex(
            parent,
            name,
            &object))
    {
        object = NULL;
        goto done;
    }

done:
    return object;
}

static bool
parse_inputs(
    gpio_input_context_st * const inputs_context,
    struct json_object * gpio_object)
{
    bool success;

    static char const gpio_inputs_name[] = "inputs";
    struct json_object * const inputs =
        get_object_by_name(gpio_object, gpio_inputs_name);

    if (inputs == NULL || !json_object_is_type(inputs, json_type_array))
    {
        success = false;
        goto done;
    }

    inputs_context->num_gpio = json_object_array_length(inputs);
    inputs_context->gpios = calloc(inputs_context->num_gpio, sizeof *inputs_context->gpios);
    if (inputs_context->gpios == NULL)
    {
        success = false;
        goto done;
    }

    for (size_t index = 0; index < inputs_context->num_gpio; index++)
    {
        gpio_input_st * const gpio_input = &inputs_context->gpios[index];
        struct json_object * const input_object =
            json_object_array_get_idx(inputs, index);
        struct json_object * const gpio = get_object_by_name(input_object, "gpio");

        if (gpio == NULL)
        {
            success = false;
            goto done;
        }

        gpio_input->gpio_number = json_object_get_int(gpio);

        DPRINTF("input: %d gpio %d\n", index, gpio_input->gpio_number);
    }

    success = true;

done:
    return success;
}

static bool
parse_outputs(
    gpio_output_context_st * const outputs_context,
    struct json_object * gpio_object)
{
    bool success;

    static char const gpio_outputs_name[] = "outputs";
    struct json_object * const outputs =
        get_object_by_name(gpio_object, gpio_outputs_name);

    if (outputs == NULL || !json_object_is_type(outputs, json_type_array))
    {
        success = false;
        goto done;
    }

    outputs_context->num_gpio = json_object_array_length(outputs);
    outputs_context->gpios = calloc(outputs_context->num_gpio, sizeof *outputs_context->gpios);
    if (outputs_context->gpios == NULL)
    {
        success = false;
        goto done;
    }

    for (size_t index = 0; index < outputs_context->num_gpio; index++)
    {
        gpio_output_st * const gpio_output = &outputs_context->gpios[index];
        struct json_object * const output_object =
            json_object_array_get_idx(outputs, index);
        struct json_object * const gpio = get_object_by_name(output_object, "gpio");

        if (gpio == NULL)
        {
            success = false;
            goto done;
        }

        gpio_output->gpio_number = json_object_get_int(gpio);

        DPRINTF("output: %d gpio %d\n", index, gpio_output->gpio_number);
    }

    success = true;

done:
    return success;
}

static bool
parse_configuration(
    configuration_st * const configuration,
    struct json_object * json_root)
{
    bool success;

    static char const gpio_object_name[] = "gpio";
    struct json_object * const gpio_object = 
        get_object_by_name(json_root, gpio_object_name);

    if (gpio_object == NULL)
    {
        success = false;
        goto done;
    }

    if (!parse_inputs(&configuration->inputs, gpio_object))
    {
        success = false;
        goto done;
    }

    if (!parse_outputs(&configuration->outputs, gpio_object))
    {
        success = false;
        goto done;
    }

    success = true;

done:
    return success;
}

void configuration_free(configuration_st const * const configuration)
{
    if (configuration == NULL)
    {
        goto done;
    }

    json_object_put(configuration->json);
    free(configuration->inputs.gpios);
    free(configuration->outputs.gpios); 

    free((void *)configuration);

done:
    return;
}

configuration_st * configuration_load(char const * const filename)
{
    configuration_st * configuration = 
        calloc(1, sizeof *configuration);

    if (configuration == NULL)
    {
        goto done;
    }

    configuration->json = json_object_from_file(filename);
    if (configuration->json == NULL)
    {
        configuration_free(configuration);
        configuration = NULL;
        goto done;
    }

    if (!parse_configuration(configuration, configuration->json))
    {
        configuration_free(configuration);
        configuration = NULL;
        goto done;
    }

done:
    return configuration;
}

size_t configuration_num_inputs(configuration_st const * const configuration)
{
    gpio_input_context_st const * const inputs = &configuration->inputs;

    return inputs->num_gpio;
}

size_t configuration_num_outputs(configuration_st const * const configuration)
{
    gpio_output_context_st const * const outputs = &configuration->outputs;

    return outputs->num_gpio;
}

bool configuration_input_gpio_number(
    configuration_st const * const configuration,
    size_t const input_number,
    size_t * const gpio_number)
{
    bool success;
    gpio_input_context_st const * const inputs = &configuration->inputs; 

    if (input_number >= inputs->num_gpio)
    {
        success = false;
        goto done;
    }

    gpio_input_st const * const input =
        &inputs->gpios[input_number];

    *gpio_number = input->gpio_number;
    success = true;

done:
    return success;
}

bool configuration_output_gpio_number(
    configuration_st const * const configuration,
    size_t const output_number,
    size_t * const gpio_number)
{
    bool success;
    gpio_output_context_st const * const outputs = &configuration->outputs;

    if (output_number >= outputs->num_gpio)
    {
        success = false;
        goto done;
    }

    gpio_output_st const * const output =
        &outputs->gpios[output_number];

    *gpio_number = output->gpio_number;
    success = true;

done:
    return success;
}


