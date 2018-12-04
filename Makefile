LIBS=\
	-ljson-c \
	-lubus \
	-lubox \
	-lubusgpio

CFLAGS=-D_GNU_SOURCE

ifneq ($(LIB_PREFIX),)
LFLAGS += -Wl,-L$(LIB_PREFIX)/lib -Wl,-rpath=$(LIB_PREFIX)/lib
CFLAGS += -I$(LIB_PREFIX)/include
endif

SRCS=\
	configuration.c \
	ubus.c \
	daemonize.c \
	main.c \
	sysfs_gpio_module.c

OBJS = ${SRCS:.c=.o}

TARGET=sysfs_gpio_module

.PHONY: ${TARGET}
${TARGET}: ${OBJS}
	${CC} ${OBJS} ${LFLAGS} ${LIBS} -o $@

.PHONY: clean
clean:
	rm -rf *.o ${TARGET}

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} ${SRCS} >> .depend

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

include: .depend
