#
# Makefile for MINGW32
#


all: cdvdiso

PLUGIN = CDVDiso.dll

CC = gcc
NASM = nasmw
RM = rm -f
AR = ar
STRIP = strip
RC = windres

OPTIMIZE = -O2 -fomit-frame-pointer -finline-functions -ffast-math -fno-strict-aliasing
FLAGS = -D__WIN32__ -D__MINGW32__ # -DENABLE_NLS -DPACKAGE=\"pcsx2\"
RC1FLAGS = -d__MINGW32__
LIBS = -L./ -lcomctl32 -lwsock32 -lwinmm -lgdi32 -lcomdlg32 #-lintl
RESOBJ = cdvdiso.o

OBJS = ../CDVDiso.o ../libiso.o
OBJS+= Config.o Win32.o ${RESOBJ}
OBJS+= ../zlib/adler32.o ../zlib/compress.o ../zlib/crc32.o ../zlib/gzio.o ../zlib/uncompr.o ../zlib/deflate.o ../zlib/trees.o \
	../zlib/zutil.o ../zlib/inflate.o ../zlib/infback.o ../zlib/inftrees.o ../zlib/inffast.o
OBJS+= ../bzip2/blocksort.o ../bzip2/bzlib.o \
	   ../bzip2/compress.o ../bzip2/crctable.o \
	   ../bzip2/decompress.o ../bzip2/huffman.o \
	   ../bzip2/randtable.o

DEPS:= $(OBJS:.o=.d)

CFLAGS = -Wall ${OPTIMIZE} -I. -I.. -I/usr/local/include -I../zlib -I../bzip2 ${FLAGS}

cdvdiso: ${OBJS}
	dllwrap --def plugin.def -o ${PLUGIN} ${OBJS} ${LIBS}
#	${CC} -shared -Wl,--kill-at,--output-def,plugin.def ${CFLAGS} ${OBJS} -o ${PLUGIN} ${LIBS}
	${STRIP} ${PLUGIN}

.PHONY: clean cdvdiso

clean:
	${RM} ${OBJS} ${DEPS} ${PCSX2}

%.o: %.asm
	${NASM} ${ASMFLAGS} -o $@ $<

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $< -MD -MF $(patsubst %.o,%.d,$@)

${RESOBJ}: CDVDiso.rc
	    ${RC} -D__MINGW32__ -I rc -O coff -o $@ -i $<

-include ${DEPS}

	