#
# Makefile for MINGW32
#


all: cdvdnull
install: all

PLUGIN = libCDVDnull.so

CC = gcc
NASM = nasmw
RM = rm -f
AR = ar
STRIP = strip
RC = windres

OPTIMIZE = -O2 -fomit-frame-pointer -finline-functions -ffast-math -fno-strict-aliasing
FLAGS = -DENABLE_NLS -DPACKAGE=\"pcsx2\"
RC1FLAGS =
LIBS = 
RESOBJ = cdvdnull.o

OBJS = CDVD.o


DEPS:= $(OBJS:.o=.d)

CFLAGS = -Wall ${OPTIMIZE} -I. -I/usr/local/include ${FLAGS} -fPIC

cdvdnull: ${OBJS}
#	dllwrap --def plugin.def -o ${PLUGIN} ${OBJS} ${LIBS}
	${CC} -shared -Wl,-soname,${PLUGIN} ${CFLAGS} ${OBJS} -o ${PLUGIN} ${LIBS}
	${STRIP} ${PLUGIN}

.PHONY: clean cdvdnull

clean:
	${RM} ${OBJS} ${DEPS} ${PCSX2}

%.o: %.asm
	${NASM} ${ASMFLAGS} -o $@ $<

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $< -MD -MF $(patsubst %.o,%.d,$@)

${RESOBJ}: CDVDnull.rc
	    ${RC} -D__MINGW32__ -I rc -O coff -o $@ -i $<

-include ${DEPS}
