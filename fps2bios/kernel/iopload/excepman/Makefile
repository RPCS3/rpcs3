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
IOPINCLUDES = -I. -I../include -I$(PS2LIB)/common/include
IOPCOMPILE = $(IOPCC) $(IOPINCLUDES) $(IOPCFLAGS)
IOPLINK = $(IOPLD) -dc -nostartfiles


#  ------------------------------------------------------------------------
#  PROJECTS TO BUILD

all: excepman


#  ------------------------------------------------------------------------
#  KERNEL BUILD INSTRUCTIONS

LDFLAGS = 
LDADD   = 
OBJECTS = excepman.o ../libkernel/iop_loadcore.o ex_handler.o ../iopdebug.o

excepman: $(OBJECTS)
	$(IOPLINK) $(OBJECTS) $(LDFLAGS) $(LDADD) -o ../../../build/EXCEPMAN

%.o: %.c
	$(IOPCC) $(IOPINCLUDES) $(IOPCFLAGS) -o $@ -c $<

%.o : %.s
	$(IOPAS) $(IOPASFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS)



