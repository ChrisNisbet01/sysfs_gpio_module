LIBS=\
	-ljson-c \
	-lubus \
	-lubox

CFLAGS=-D_GNU_SOURCE

SRCS=\
	configuration.c \
	ubus.c \
	ubus_server.c \
	daemonize.c \
	main.c \
	sysfs_gpio_module.c

OBJS = ${SRCS:.c=.o}

TARGET=sysfs_gpio_module

.PHONY: ${TARGET}
${TARGET}: ${OBJS}
	${CC} ${OBJS} ${LIBS} -o $@

.PHONY: clean
clean:
	rm -rf *.o ${TARGET}

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} ${SRCS} >> .depend

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

include: .depend
