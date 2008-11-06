#
# Makefile for MINGW32
#


all: dev9linuz

PLUGIN = DEV9linuz.dll

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
LIBS = -L./ -lcomctl32 -lwsock32 -lwinmm -lgdi32 -lcomdlg32 -lPacket
RESOBJ = dev9linuz.o

OBJS = ../DEV9.o
OBJS+= socks.o Config.o Win32.o ${RESOBJ}

DEPS:= $(OBJS:.o=.d)

CFLAGS = -Wall ${OPTIMIZE} -I. -I.. -I/usr/local/include ${FLAGS}
ASMFLAGS = -f elf ${FLAGS} -i./ -i../

dev9linuz: ${OBJS}
	dllwrap --def plugin.def -o ${PLUGIN} ${OBJS} ${LIBS}
#	${CC} -shared -Wl,--kill-at,--output-def,plugin.def ${CFLAGS} ${OBJS} -o ${PLUGIN} ${LIBS}
#	${STRIP} ${PLUGIN}

.PHONY: clean dev9linuz

clean:
	${RM} ${OBJS} ${DEPS} ${PCSX2}

%.o: %.asm
	${NASM} ${ASMFLAGS} -o $@ $<

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $< -MD -MF $(patsubst %.o,%.d,$@)

${RESOBJ}: DEV9linuz.rc
	    ${RC} -D__MINGW32__ -I rc -O coff -o $@ -i $<

-include ${DEPS}

