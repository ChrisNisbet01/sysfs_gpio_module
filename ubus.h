#ifndef __UBUS_H__
#define __UBUS_H__

#include <libubus.h>

#include <stdbool.h>

struct ubus_context *
gpio_ubus_initialise(char const * const path);

void
gpio_ubus_done(void);


#endif /* __UBUS_H__ */
