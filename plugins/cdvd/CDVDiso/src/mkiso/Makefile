
CC = gcc

MKISO = mkiso
CFLAGS = -fPIC -Wall -O2 -fomit-frame-pointer -I.. -I. -D__LINUX__ -I../zlib
OBJS = mkiso.o ../libiso.o
LIBS = 
OBJS+= ../zlib/adler32.o ../zlib/compress.o ../zlib/crc32.o ../zlib/gzio.o ../zlib/uncompr.o ../zlib/deflate.o ../zlib/trees.o \
	../zlib/zutil.o ../zlib/inflate.o ../zlib/infback.o ../zlib/inftrees.o ../zlib/inffast.o

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
