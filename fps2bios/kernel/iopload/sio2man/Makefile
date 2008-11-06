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
EEINCLUDES = -I. -Iinclude -I$(PS2LIB)/common/include -I$(PS2LIB)/ee/include -I$(NEWLIB)/include

IOPCFLAGS = -O2 -fomit-frame-pointer -nostartfiles -G0
IOPINCLUDES = -I. -I../include -I$(PS2LIB)/common/include -I$(PS2LIB)/iop/include
IOPCOMPILE = $(IOPCC) $(IOPINCLUDES) $(IOPCFLAGS)
IOPLINK = $(IOPLD) -dc -nostartfiles
IOPASFLAGS := -EL -G0


#  ------------------------------------------------------------------------
#  PROJECTS TO BUILD

all: sio2man


#  ------------------------------------------------------------------------
#  KERNEL BUILD INSTRUCTIONS

LDFLAGS = -L$(PS2LIB)/iop/lib -L$(NEWLIB)/lib
LDADD   =
OBJECTS = sio2man.o ../iopdebug.o \
		  ../libkernel/iop_loadcore.o ../libkernel/iop_intrman.o \
		  ../libkernel/iop_dmacman.o ../libkernel/iop_thbase.o \
		  ../libkernel/iop_thevent.o ../libkernel/iop_stdio.o \
		  ../libkernel/iop_sysclib.o

sio2man: $(OBJECTS)
	$(IOPLINK) $(OBJECTS) $(LDFLAGS) $(LDADD) -o ../../../build/SIO2MAN

%.o: %.c
	$(IOPCC) $(IOPINCLUDES) $(IOPCFLAGS) -o $@ -c $<

%.o : %.s
	$(IOPAS) $(IOPASFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS)

