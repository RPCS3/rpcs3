
CC = gcc

MKISO = mkiso
CFLAGS = -fPIC -Wall -O2 -fomit-frame-pointer -I.. -I. -D__LINUX__ -I../3rdparty/zlib
OBJS = mkiso.o ../libiso.o
LIBS = 
OBJS+= ../3rdparty/zlib/adler32.o ../3rdparty/zlib/compress.o ../3rdparty/zlib/crc32.o ../3rdparty/zlib/gzio.o ../3rdparty/zlib/uncompr.o \
../3rdparty/zlib/deflate.o ../3rdparty/zlib/trees.o ../3rdparty/zlib/zutil.o ../3rdparty/zlib/inflate.o ../3rdparty/zlib/infback.o ../3rdparty/zlib/inftrees.o \
../3rdparty/zlib/inffast.o

DEPS:= $(OBJS:.o=.d)

all: mkiso

mkiso: ${OBJS}
	rm -f ${MKISO}
	${CC} ${CFLAGS} ${OBJS} -o ${MKISO} ${LIBS}
	strip ${MKISO}

clean: 
	rm -f ${OBJS} ${DEPS} ${MKISO}

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $< -MD -MF $(patsubst %.o,%.d,$@)

-include ${DEPS}
