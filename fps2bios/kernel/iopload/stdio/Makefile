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
IOPINCLUDES = -I. -Iinclude -I$(PS2LIB)/common/include -I$(NEWLIB)/include #-I$(PS2LIB)/iop/include 
IOPCOMPILE = $(IOPCC) $(IOPINCLUDES) $(IOPCFLAGS)
IOPLINK = $(IOPLD) -dc -nostartfiles


#  ------------------------------------------------------------------------
#  PROJECTS TO BUILD

all: stdio


#  ------------------------------------------------------------------------
#  KERNEL BUILD INSTRUCTIONS

LDFLAGS = -L$(PS2LIB)/iop/lib -L$(NEWLIB)/lib
LDADD   = 
OBJECTS = stdio.o

stdio: $(OBJECTS)
	$(IOPLINK) $(OBJECTS) $(LDFLAGS) $(LDADD) -o ../../../build/STDIO

%.o: %.c
	$(IOPCC) $(IOPINCLUDES) $(IOPCFLAGS) -o $@ -c $<


clean:
	rm -f $(OBJECTS)



