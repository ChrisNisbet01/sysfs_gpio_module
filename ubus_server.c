#include "ubus_server.h"
#include "ubus_private.h"
#include "debug.h"

#include "sysfs_gpio_module.h"

#include <libubox/blobmsg.h>

#include <string.h>
#include <stdio.h>

static char const gpio_object_name[] = "sysfs.gpio";
static char const gpio_object_type_name[] = "gpio";
static char const gpio_get_method_name[] = "get";
static char const gpio_set_method_name[] = "set";
static char const gpio_io_type_bi[] = "bi";
static char const gpio_io_type_bo[] = "bo"; 


static char const pin_str[] = "pin";
static char const state_str[] = "state";
static char const result_str[] = "result";

static char const gpio_counts_str[] = "counts";
static char const gpio_count_str[] = "count";
static char const gpios_str[] = "gpios";
static char const results_str[] = "results";
static char const instance_str[] = "instance";
static char const value_str[] = "value";
static char const error_str[] = "error";
static char const gpio_io_type_str[] = "io type";
static char const gpio_io_type_binary_input[] = "binary-input";
static char const gpio_io_type_binary_output[] = "binary-output"; 


struct ubus_context * ubus_ctx;
configuration_st const * configuration;

static int
gpio_set_multiple_handler(
    struct ubus_context * ctx,
    struct ubus_request_data * req,
    struct blob_attr * array_blob)
{
    int result;
    struct blob_buf b;

    if (blobmsg_check_array(array_blob, BLOBMSG_TYPE_TABLE) < 0)
    {
        /* The array needs to contain a set of tables, and it doesn't.*/ 
        result = UBUS_STATUS_INVALID_ARGUMENT;
        goto done;
    }

    local_blob_buf_init(&b, 0);

    void * const a = blobmsg_open_array(&b, results_str);

    struct blob_attr * cur;
    int rem;
    blobmsg_for_each_attr(cur, array_blob, rem)
    {
        enum
        {
            GPIO_TYPE,
                GPIO_INSTANCE,
                GPIO_VALUE,
                __GPIO_MAX
        };
        static struct blobmsg_policy const gpio_policy[__GPIO_MAX] = {
            [GPIO_TYPE] = { .name = gpio_io_type_str, .type = BLOBMSG_TYPE_STRING },
            [GPIO_INSTANCE] = { .name = instance_str, .type = BLOBMSG_TYPE_INT32 },
            [GPIO_VALUE] = { .name = value_str, .type = BLOBMSG_TYPE_BOOL }
        };
        struct blob_attr * tb[__GPIO_MAX];

        blobmsg_parse(
            gpio_policy,
            ARRAY_SIZE(gpio_policy),
            tb,
            blobmsg_data(cur),
            blobmsg_data_len(cur));

        if (tb[GPIO_TYPE] == NULL
            || tb[GPIO_INSTANCE] == NULL
            || tb[GPIO_VALUE] == NULL)
        {
            result = UBUS_STATUS_INVALID_ARGUMENT;
            goto buf_free_done;
        }

        bool const io_type_is_binary_output =
            strcmp(blobmsg_get_string(tb[GPIO_TYPE]), gpio_io_type_binary_output) == 0;
        uint32_t const gpio_instance = blobmsg_get_u32(tb[GPIO_INSTANCE]);
        bool wrote_gpio;

        void * const table_out = blobmsg_open_table(&b, NULL);

        if (!io_type_is_binary_output)
        {
            blobmsg_add_string(&b, error_str, "Can't write this IO type");
            wrote_gpio = false;
        }
        else
        {
            bool const state = blobmsg_get_bool(tb[GPIO_VALUE]);

            wrote_gpio = GPIOWrite(gpio_instance, state) == 0;
        }

        blobmsg_add_string(&b, gpio_io_type_str, blobmsg_get_string(tb[GPIO_TYPE]));
        blobmsg_add_u32(&b, instance_str, gpio_instance);
        blobmsg_add_u8(&b, result_str, wrote_gpio);

        blobmsg_close_table(&b, table_out);
    }

    blobmsg_close_array(&b, a);

    ubus_send_reply(ctx, req, b.head);

    result = UBUS_STATUS_OK;

buf_free_done:
    blob_buf_free(&b);

done:
    return result;
}

enum
{
    GPIO_SET_PIN,
    GPIO_SET_STATE,
    GPIO_SET_GPIOS,
    __GPIO_SET_MAX
};

static struct blobmsg_policy const gpio_set_policy[__GPIO_SET_MAX] = {
    [GPIO_SET_PIN] = { .name = pin_str, .type = BLOBMSG_TYPE_INT32 },
    [GPIO_SET_STATE] = { .name = state_str, .type = BLOBMSG_TYPE_BOOL },
    [GPIO_SET_GPIOS] = { .name = gpios_str, .type = BLOBMSG_TYPE_ARRAY }
};

static int
gpio_set_handler(
    struct ubus_context * ctx,
    struct ubus_object * obj,
    struct ubus_request_data * req,
    const char * method,
    struct blob_attr * msg)
{
    int result;
    struct blob_attr * tb[__GPIO_SET_MAX];
    struct blob_buf b; 

    blobmsg_parse(gpio_set_policy,
                  ARRAY_SIZE(gpio_set_policy),
                  tb,
                  blob_data(msg),
                  blob_len(msg));

    if (tb[GPIO_SET_GPIOS] != NULL)
    {
        result = gpio_set_multiple_handler(ctx, req, tb[GPIO_SET_GPIOS]);
        goto done;
    }

    if (tb[GPIO_SET_PIN] == NULL || tb[GPIO_SET_STATE] == NULL)
    {
        result = UBUS_STATUS_INVALID_ARGUMENT;
        goto done;
    }

    uint32_t const pin = blobmsg_get_u32(tb[GPIO_SET_PIN]);
    bool const state = blobmsg_get_bool(tb[GPIO_SET_STATE]);

    bool const success = GPIOWrite(pin, state) == 0;

    local_blob_buf_init(&b, 0);

    blobmsg_add_u32(&b, pin_str, pin);
    blobmsg_add_u8(&b, result_str, success);

    ubus_send_reply(ctx, req, b.head);

    blob_buf_free(&b);

    result = 0;

done:
    return result;
}

static int
gpio_get_multiple_handler(
    struct ubus_context * ctx,
    struct ubus_request_data * req,
    struct blob_attr * array_blob)
{
    int result;
    struct blob_buf b;

    if (blobmsg_check_array(array_blob, BLOBMSG_TYPE_TABLE) < 0)
    {
        /* The array needs to contain a set of tables, and it doesn't.*/ 
        result = UBUS_STATUS_INVALID_ARGUMENT;
        goto done;
    }

    local_blob_buf_init(&b, 0);

    void * const a = blobmsg_open_array(&b, results_str);

    struct blob_attr * cur;
    int rem;
    blobmsg_for_each_attr(cur, array_blob, rem)
    {
        enum
        {
            GPIO_TYPE,
            GPIO_INSTANCE,
            __GPIO_MAX
        };
        static struct blobmsg_policy const gpio_policy[__GPIO_MAX] = {
            [GPIO_TYPE] = { .name = gpio_io_type_str, .type = BLOBMSG_TYPE_STRING },
            [GPIO_INSTANCE] = { .name = instance_str, .type = BLOBMSG_TYPE_INT32 },
        };
        struct blob_attr * tb[__GPIO_MAX];

        blobmsg_parse(
            gpio_policy,
            ARRAY_SIZE(gpio_policy),
            tb,
            blobmsg_data(cur),
            blobmsg_data_len(cur));

        if (tb[GPIO_TYPE] == NULL
            || tb[GPIO_INSTANCE] == NULL)
        {
            result = UBUS_STATUS_INVALID_ARGUMENT;
            goto buf_free_done;
        }

        bool io_type_is_binary_input =
            strcmp(blobmsg_get_string(tb[GPIO_TYPE]), gpio_io_type_binary_input) == 0;
        uint32_t const gpio_instance = blobmsg_get_u32(tb[GPIO_INSTANCE]);
        bool read_gpio;
        void * table_out = blobmsg_open_table(&b, NULL);

        if (!io_type_is_binary_input)
        {
            blobmsg_add_string(&b, error_str, "Can't read this IO type");
            read_gpio = false;
        }
        else
        {
            bool state;

            read_gpio = GPIORead(gpio_instance, &state) == 0;

            if (read_gpio)
            {
                blobmsg_add_u8(&b, value_str, state);
            }
        }

        blobmsg_add_string(&b, gpio_io_type_str, blobmsg_get_string(tb[GPIO_TYPE]));
        blobmsg_add_u32(&b, instance_str, gpio_instance);
        blobmsg_add_u8(&b, result_str, read_gpio);

        blobmsg_close_table(&b, table_out);
    }

    blobmsg_close_array(&b, a);

    ubus_send_reply(ctx, req, b.head);

    result = UBUS_STATUS_OK;

buf_free_done:
    blob_buf_free(&b);

done:
    return result;
}

enum
{
    GPIO_GET_PIN,
    GPIO_GET_GPIOS,
    __GPIO_GET_MAX
};

static struct blobmsg_policy const gpio_get_policy[__GPIO_GET_MAX] = {
    [GPIO_GET_PIN] = { .name = pin_str, .type = BLOBMSG_TYPE_INT32 },
    [GPIO_GET_GPIOS] = { .name = gpios_str, .type = BLOBMSG_TYPE_ARRAY }
};

static int
gpio_get_handler(
    struct ubus_context * ctx,
    struct ubus_object * obj,
    struct ubus_request_data * req,
    const char * method,
    struct blob_attr * msg)
{
    int result;
    struct blob_attr * tb[__GPIO_GET_MAX];
    struct blob_buf b; 

    blobmsg_parse(gpio_get_policy,
                  ARRAY_SIZE(gpio_get_policy),
                  tb,
                  blob_data(msg),
                  blob_len(msg));

    if (tb[GPIO_GET_GPIOS] != NULL)
    {
        result = gpio_get_multiple_handler(ctx, req, tb[GPIO_GET_GPIOS]);
        goto done;
    }

    if (tb[GPIO_GET_PIN] == NULL)
    {
        result = UBUS_STATUS_INVALID_ARGUMENT;
        goto done;
    }

    uint32_t const pin = blobmsg_get_u32(tb[GPIO_GET_PIN]);
    bool state;

    bool const success = GPIORead(pin, &state) == 0;

    local_blob_buf_init(&b, 0);

    blobmsg_add_u8(&b, result_str, success);
    if (success)
    {
        blobmsg_add_u8(&b, state_str, state);
    }

    ubus_send_reply(ctx, req, b.head);

    blob_buf_free(&b); 

    result = 0;

done:
    return result;
}

enum
{
    GPIO_COUNT_TYPE,
    __GPIO_COUNT_MAX
};

static struct blobmsg_policy const gpio_count_policy[__GPIO_COUNT_MAX] = {
    [GPIO_COUNT_TYPE] = { .name = gpio_io_type_str, .type = BLOBMSG_TYPE_STRING }
};

static int
gpio_count_handler(
    struct ubus_context * ctx,
    struct ubus_object * obj,
    struct ubus_request_data * req,
    const char * method,
    struct blob_attr * msg)
{
    int result;
    struct blob_attr * tb[__GPIO_COUNT_MAX];
    struct blob_buf b;

    blobmsg_parse(gpio_count_policy,
                  ARRAY_SIZE(gpio_count_policy),
                  tb,
                  blob_data(msg),
                  blob_len(msg));

    if (tb[GPIO_COUNT_TYPE] == NULL)
    {
        result = UBUS_STATUS_INVALID_ARGUMENT;
        goto done;
    }

    char const * const io_type = blobmsg_get_string(tb[GPIO_COUNT_TYPE]);
    int count;

    if (strcmp(io_type, gpio_io_type_bi) == 0)
    {
        count = configuration_num_inputs(configuration);
    }
    else if (strcmp(io_type, gpio_io_type_bo) == 0)
    {
        count = configuration_num_outputs(configuration);
    }
    else
    {
        count = 0;
    }

    local_blob_buf_init(&b, 0);

    blobmsg_add_u32(&b, io_type, count);

    ubus_send_reply(ctx, req, b.head);

    blob_buf_free(&b); 

    result = 0;

done:
    return result;
}

static int
gpio_counts_handler(
    struct ubus_context * ctx,
    struct ubus_object * obj,
    struct ubus_request_data * req,
    const char * method,
    struct blob_attr * msg)
{
    int result;
    struct blob_buf b;

    local_blob_buf_init(&b, 0);

    void * const a = blobmsg_open_array(&b, gpio_counts_str);
    void * table_out;

    table_out = blobmsg_open_table(&b, NULL); 

    blobmsg_add_string(&b, gpio_io_type_str, gpio_io_type_binary_input);
    blobmsg_add_u32(&b, gpio_count_str, configuration_num_inputs(configuration));

    blobmsg_close_table(&b, table_out); 

    table_out = blobmsg_open_table(&b, NULL);

    blobmsg_add_string(&b, gpio_io_type_str, gpio_io_type_binary_output);
    blobmsg_add_u32(&b, gpio_count_str, configuration_num_outputs(configuration));

    blobmsg_close_table(&b, table_out); 

    blobmsg_close_array(&b, a);

    ubus_send_reply(ctx, req, b.head);

    blob_buf_free(&b); 

    return 0;
}

static bool
gpio_add_object(
    struct ubus_context * const ctx,
    struct ubus_object * const obj)
{
    int const ret = ubus_add_object(ctx, obj);

    if (ret != UBUS_STATUS_OK)
    {
        DPRINTF("Failed to publish object '%s': %s\n",
                obj->name,
                ubus_strerror(ret));
    }

    return ret == UBUS_STATUS_OK;
}

static struct ubus_method gpio_object_methods[] = {
    UBUS_METHOD(gpio_get_method_name, gpio_get_handler, gpio_get_policy),
    UBUS_METHOD(gpio_set_method_name, gpio_set_handler, gpio_set_policy),
    UBUS_METHOD(gpio_count_str, gpio_count_handler, gpio_count_policy),
    UBUS_METHOD_NOARG(gpio_counts_str, gpio_counts_handler)
};

static struct ubus_object_type gpio_object_type =
    UBUS_OBJECT_TYPE(gpio_object_type_name, gpio_object_methods);

static struct ubus_object gpio_object =
{
    .name = gpio_object_name,
    .type = &gpio_object_type,
    .methods = gpio_object_methods,
    .n_methods = ARRAY_SIZE(gpio_object_methods)
};

bool
gpio_ubus_server_initialise(
    struct ubus_context * const ctx,
    configuration_st const * const configuration_in)
{
    ubus_ctx = ctx;
    configuration = configuration_in;

    return gpio_add_object(ubus_ctx, &gpio_object);
}

void
gpio_ubus_server_done(void)
{
    ubus_ctx = NULL;
    configuration = NULL;
}


