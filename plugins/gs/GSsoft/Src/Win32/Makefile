#
# GSsoft Makefile for MINGW32
#


all: gssoft

PLUGIN = GSsoft.dll

CC = gcc
NASM = nasmw
RM = rm -f
AR = ar
STRIP = strip
RC = windres

OPTIMIZE = -O2 -fomit-frame-pointer -finline-functions -ffast-math -fno-strict-aliasing -m128bit-long-double
FLAGS = -D__WIN32__ -D__MINGW32__ $(shell sdl-config --cflags)# -DENABLE_NLS -DPACKAGE=\"pcsx2\"
ASMFLAGS = -D__WIN32__ -i.. -i.# -DENABLE_NLS -DPACKAGE=\"pcsx2\"
RC1FLAGS = -d__MINGW32__
LIBS = -L./ -lcomctl32 -lwsock32 -lwinmm -lgdi32 -lcomdlg32 $(shell sdl-config --libs) #-lintl
RESOBJ = gssoft.o

OBJS = ../GS.o ../Cache.o ../Color.o ../Draw.o ../Mem.o \
	../Page.o ../Prim.o ../Rec.o ../Regs.o ../SDL.o \
	../Soft.o ../Texts.o ../Transfer.o \
	../scale2x.o ../SDL_gfxPrimitives.o 
OBJS+= Conf.o Win32.o ${RESOBJ}

DEPS:= $(OBJS:.o=.d)

CFLAGS = -Wall ${OPTIMIZE} -I. -I.. -I/usr/local/include -I../ffmpeg ${FLAGS}
ASMFLAGS = -f elf ${FLAGS} -i./ -i../

FLAGS+= -I../x86
OBJS+= ../x86/ix86_cpudetect.o

gssoft: ${OBJS}
	dllwrap --def plugin.def -o ${PLUGIN} ${OBJS} ${LIBS}
#	${CC} -shared -Wl,--kill-at,--output-def,plugin.def ${CFLAGS} ${OBJS} -o ${PLUGIN} ${LIBS}
	${STRIP} ${PLUGIN}

.PHONY: clean gssoft

clean:
	${RM} ${OBJS} ${DEPS} ${PCSX2}

%.o: %.asm
	${NASM} ${ASMFLAGS} -o $@ $<

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $< -MD -MF $(patsubst %.o,%.d,$@)

${RESOBJ}: GSsoftdx.rc
	    ${RC} -D__MINGW32__ -I rc -O coff -o $@ -i $<

-include ${DEPS}

