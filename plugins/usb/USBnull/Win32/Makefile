#
# USBnull Makefile for MINGW32
#


all: plugin

CPU = ix86
PLUGIN = USBnull.dll

CC = gcc
NASM = nasmw
RM = rm -f
AR = ar
STRIP = strip
RC = windres

OPTIMIZE = -O2 -fomit-frame-pointer -finline-functions -ffast-math -fno-strict-aliasing -m128bit-long-double
FLAGS = -D__WIN32__ -D__MINGW32__ # -DENABLE_NLS -DPACKAGE=\"pcsx2\"
ASMFLAGS = -D__WIN32__ -i.. -i.# -DENABLE_NLS -DPACKAGE=\"pcsx2\"
RC1FLAGS = -d__MINGW32__
LIBS = -L./ -lcomctl32 -lwsock32 -lwinmm -lgdi32 -lcomdlg32 
RESOBJ = res.o

OBJS = ../USB.o
OBJS+= Config.o Win32.o ${RESOBJ}

DEPS:= $(OBJS:.o=.d)

CFLAGS = -Wall ${OPTIMIZE} -I. -I.. -I/usr/local/include ${FLAGS}
ASMFLAGS = -f elf ${FLAGS} -i./ -i../

plugin: ${OBJS}
	dllwrap --def plugin.def -o ${PLUGIN} ${OBJS}
#	${CC} -shared -Wl,--kill-at ${CFLAGS} ${OBJS} -o ${PLUGIN} ${LIBS}
	${STRIP} ${PLUGIN}

.PHONY: clean plugin

clean:
	${RM} ${OBJS} ${DEPS} ${PLUGIN}

%.o: %.asm
	${NASM} ${ASMFLAGS} -o $@ $<

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $< -MD -MF $(patsubst %.o,%.d,$@)

${RESOBJ}: USBnull.rc
	    ${RC} -D__MINGW32__ -I rc -O coff -o $@ -i $<

-include ${DEPS}

