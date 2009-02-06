
RC = windres
STRIP = strip

PLUGIN = FWnull.dll
CFLAGS = -Wall -O2 -fomit-frame-pointer -I.. -D__WIN32__ -D__MINGW32__
LIBS   = -lcomctl32 -lwsock32 -lwinmm -lgdi32 -lcomdlg32
RESOBJ = FireWireNull.o
OBJS   = ../FW.o Config.o Win32.o ${RESOBJ}

DEPS:= $(OBJS:.o=.d)

all: plugin

plugin: ${OBJS}
	dllwrap --def plugin.def -o ${PLUGIN} ${OBJS} ${LIBS}
#	${CC} -shared -Wl,--kill-at,--output-def,plugin.def ${CFLAGS} ${OBJS} -o ${PLUGIN} ${LIBS}
	${STRIP} ${PLUGIN}

.PHONY: clear plugin

clean: 
	rm -f ${OBJS} ${DEPS} ${PLUGIN}

${RESOBJ}: FireWireNull.rc
	${RC} -D__MINGW32__ -I rc -O coff -o $@ -i $<

-include ${DEPS}

