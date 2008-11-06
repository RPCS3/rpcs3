
PLUGIN = libFWnull.so
CFG = cfgFWnull
CFLAGS+= -fPIC -Wall -I. -I.. -O3 -fomit-frame-pointer -fno-strict-aliasing
OBJS = ../FW.o
OBJS+= Linux.o Config.o
CFGOBJS = conf.o interface.o support.o Config.o
DEPS:= $(OBJS:.o=.d)
CFGDEPS:= $(CFGOBJS:.o=.d)
LIBS = -lpthread
CFLAGS+= $(shell pkg-config --cflags gtk+-2.0)    -D__LINUX__
CFGLIBS = $(shell pkg-config --libs gtk+-2.0)

CC = gcc

all: plugin cfg
install: all

plugin: ${OBJS}
	rm -f ${PLUGIN}
	${CC} -shared -Wl,-soname,${PLUGIN} ${CFLAGS} ${OBJS} -o ${PLUGIN} ${LIBS}
	strip --strip-unneeded --strip-debug ${PLUGIN}

cfg: ${CFGOBJS}
	rm -f ${CFG}
	${CC} ${CFLAGS} ${CFGOBJS} -o ${CFG} ${CFGLIBS}
	strip ${CFG}

clean: 
	rm -f ${OBJS} ${DEPS} ${CFGOBJS} ${CFGDEPS} ${PLUGIN} ${CFG}

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $< -MD -MF $(patsubst %.o,%.d,$@)

-include ${DEPS}
