
CC = gcc

PLUGIN = libSPU2null.so
CFLAGS+= -fPIC -Wall -O2 -fomit-frame-pointer -D__LINUX__
OBJS = SPU2.o
DEPS:= $(OBJS:.o=.d)
LIBS = $(shell pkg-config --libs gtk+-2.0)
CFLAGS+= $(shell pkg-config --cflags gtk+-2.0)

all: plugin
install: all

plugin: ${OBJS}
	rm -f ${PLUGIN}
	gcc -shared -Wl,-soname,${PLUGIN} ${CFLAGS} ${OBJS} -o ${PLUGIN} ${LIBS}
	strip --strip-unneeded --strip-debug ${PLUGIN}

clean:
	rm -f ${OBJS} ${DEPS}

%.o: %.cpp
	${CC} ${CFLAGS} -c -o $@ $< -MD -MF $(patsubst %.o,%.d,$@)

#-include ${DEPS}
