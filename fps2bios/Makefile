#
#

all: ps2romgen_exe romdir_exe romver_exe fps2bios

VERSION = 0
BUILD   = 1

CC = gcc
RM = rm -f
STRIP = strip

OPTIMIZE = -O2 -fomit-frame-pointer -finline-functions -ffast-math
CFLAGS = -Wall ${OPTIMIZE} -I.
DIRS = kernel intro loader
FILES = RESET ROMDIR EXTINFO ROMVER IOPBOOT EELOAD \
		SYSMEM LOADCORE EXCEPMAN INTRMAN SSBUSC DMACMAN \
		TIMRMAN SYSCLIB HEAPLIB THREADMAN VBLANK STDIO \
		SIFMAN SIFCMD SIO2MAN LOADER INTRO IOPBTCONF FP2BLOGO \
		IOMAN MODLOAD ROMDRV IGREETING REBOOT LOADFILE CDVDMAN \
		CDVDFSV SIFINIT FILEIO SECRMAN EESYNC

ps2romgen_exe: ps2romgen.o
	${CC} ${CFLAGS} ps2romgen.o -o build/ps2romgen_exe

romdir_exe: romdir.o
	${CC} ${CFLAGS} romdir.o -o build/romdir_exe

romver_exe: romver.o
	${CC} ${CFLAGS} romver.o -o build/romver_exe

fps2bios:
	for i in $(DIRS); do \
		(cd $$i; make; cd ..) \
	done;
	cp -f used/* build
	cp -f FP2BLOGO build
	cp -f IOPBTCONF build/
	(cd build; \
	./romver_exe $(VERSION) $(BUILD); \
	./romdir_exe $(FILES); \
	./ps2romgen_exe fps2bios; \
	cd ..)
	cp build/fps2bios ../bin/bios

.PHONY: clean ps2romgen_exe romdir_exe fps2bios

clean:
	${RM} *.o build/*
	for i in $(DIRS); do \
		(cd $$i; make clean; cd ..) \
	done; 

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $<


