#  _____     ___ ____
#   ____|   |    ____|      PSX2 OpenSource Project
#  |     ___|   |____       (C)2002, David Ryan ( Oobles@hotmail.com )
#  ------------------------------------------------------------------------

# Generated automatically from Makefile.in by configure.
#.SUFFIXES: .S .c .o .s .elf .irx

#  ------------------------------------------------------------------------
#  COMPILERS

IOPCC = iop-gcc
IOPAR = iop-ar
IOPLD = iop-ld
IOPAS = iop-as
EECC  = ee-gcc
EEAR  = ee-ar
EELD  = ee-gcc


#  ------------------------------------------------------------------------
#  DIRECTORY PATHS & FLAGS


EECFLAGS  = -O2 -fomit-frame-pointer -mips3 -EL -nostartfiles 
EEINCLUDES = -I. -Iinclude -I$(PS2LIB)/common/include -I$(PS2LIB)/ee/include

IOPCFLAGS = -O2 -fomit-frame-pointer -nostartfiles -G0
IOPINCLUDES = -I. -Iinclude -I$(PS2LIB)/common/include -I$(PS2LIB)/iop/include
IOPCOMPILE = $(IOPCC) $(IOPINCLUDES) $(IOPCFLAGS)
IOPLINK = $(IOPLD) -dc


#  ------------------------------------------------------------------------
#  PROJECTS TO BUILD

all: iopload


#  ------------------------------------------------------------------------
#  KERNEL BUILD INSTRUCTIONS

LDFLAGS = -nostartfiles -L$(PS2LIB)/iop/lib
LDADD   =
OBJECTS = romdir.o iopelf.o iopdebug.o
DIRS    = iopboot sysmem loadcore excepman intrman stdio timrman \
		  dmacman sifman sifcmd ssbusc sysclib heaplib \
		  vblank threadman sio2man

iopload: $(OBJECTS)
	for i in $(DIRS); do \
		(cd $$i; make; cd ..) \
	done; 


%.o: %.c
	$(IOPCC) $(IOPINCLUDES) $(IOPCFLAGS) -o $@ -c $<


clean:
	for i in $(DIRS); do \
		(cd $$i; make clean; cd ..) \
	done; 
	rm -f $(OBJECTS)



