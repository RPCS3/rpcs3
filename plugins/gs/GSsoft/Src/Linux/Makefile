

CPU = $(shell gcc -dumpmachine)

OPTIMIZE = -O2 -fomit-frame-pointer -ffast-math -fno-exceptions -m128bit-long-double
#OPTIMIZE = -g -O0


PLUGIN = libGSsoft.so
CFLAGS+= -fPIC -Wall ${OPTIMIZE} -I. -I.. -I../x86
OBJS = ../GS.o ../Prim.o ../Soft.o ../Draw.o \
	   ../Texts.o ../Color.o ../Mem.o ../Rec.o \
	   ../Page.o ../Transfer.o ../Cache.o \
	   ../SDL.o ../scale2x.o ../Regs.o ../SDL_gfxPrimitives.o
OBJS+= Conf.o interface.o support.o Linux.o
OBJS+= ../x86/ix86_cpudetect.o

DEPS:= $(OBJS:.o=.d)
LIBS = $(shell gtk-config --libs) $(shell sdl-config --libs) -ldl
CFLAGS+= $(shell gtk-config --cflags) $(shell sdl-config --cflags) -D__LINUX__



CC = gcc
NASM = nasm

all: plugin

plugin: ${OBJS}
	rm -f ${PLUGIN}
	${CC} -shared -Wl,-soname,${PLUGIN} ${CFLAGS} ${OBJS} -o ${PLUGIN} ${LIBS}
#	strip --strip-unneeded --strip-debug ${PLUGIN}

test: ../test.o ../i386.o
	${CC} ${CFLAGS} ../test.o ../i386.o -o test ${LIBS}

clean: 
	rm -f ${OBJS} ${DEPS} ${PLUGIN}

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $< -MD -MF $(patsubst %.o,%.d,$@)

../i386.o: ../i386.asm
	${NASM} ${ASMFLAGS} -o $@ $<

-include ${DEPS}
