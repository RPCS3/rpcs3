
PLUGIN  = libPADwin.so
CFLAGS  = -fPIC -Wall -O2 -fomit-frame-pointer -D__LINUX__
OBJS    = PAD.o Conf.o Linux.o 
OBJS   += interface.o support.o
LIBS    = $(shell pkg-config --libs gtk+-2.0)
CFLAGS += $(shell pkg-config --cflags gtk+-2.0)

DEPS:= $(OBJS:.o=.d)

all: plugin
install: all

plugin: ${OBJS}
	rm -f ${PLUGIN}
	gcc -shared -Wl,-soname,${PLUGIN} ${CFLAGS} ${OBJS} -o ${PLUGIN} ${LIBS}
	strip --strip-unneeded --strip-debug ${PLUGIN}

.PHONY: clear plugin

clean: 
	rm -f ${OBJS} ${DEPS} ${PLUGIN}

-include ${DEPS}

