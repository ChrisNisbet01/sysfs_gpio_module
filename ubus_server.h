#ifndef __UBUS_SERVER_H__
#define __UBUS_SERVER_H__

#include "configuration.h"

#include <libubus.h>

bool
gpio_ubus_server_initialise(
    struct ubus_context * const ctx,
    configuration_st const * const configuration_in);

void
gpio_ubus_server_done(void);


#endif /* __UBUS_SERVER_H__ */
