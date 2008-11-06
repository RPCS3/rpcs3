
PLUGIN = libDEV9linuz.so
CFG = cfgDEV9linuz
CFLAGS+= -fPIC -Wall -I. -I.. -O3 -fomit-frame-pointer 
OBJS = ../DEV9.o ../dvr.o ../flash.o
OBJS+= Linux.o Config.o socks.o
CFGOBJS = conf.o interface.o support.o Config.o
DEV9NETOBJS = dev9net.o
DEPS:= $(OBJS:.o=.d)
LIBS = -lpthread
CFLAGS+= $(shell gtk-config --cflags) -D__LINUX__
CFGLIBS = $(shell gtk-config --libs) 

CC = gcc

all: plugin cfg dev9net

plugin: ${OBJS}
	rm -f ${PLUGIN}
	${CC} -shared -Wl,-soname,${PLUGIN} ${CFLAGS} ${OBJS} -o ${PLUGIN} ${LIBS}
	strip --strip-unneeded --strip-debug ${PLUGIN}

cfg: ${CFGOBJS}
	rm -f ${CFG}
	${CC} ${CFLAGS} ${CFGOBJS} -o ${CFG} ${CFGLIBS}
	strip ${CFG}

dev9net:
	${CC} -O2 -Wall -fno-strict-aliasing -D__KERNEL__ -DMODULE=1 -I/usr/src/linux/include -c -o dev9net.o dev9net.c

clean: 
	rm -f ${OBJS} ${DEPS} ${PLUGIN} ${CFG}

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $< -MD -MF $(patsubst %.o,%.d,$@)

-include ${DEPS}
