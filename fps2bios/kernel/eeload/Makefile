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


EECFLAGS  = -O2 -fomit-frame-pointer -mips3 -EL -nostartfiles -G0 -D_EE --save-temps
EEINCLUDES = -I. -Iinclude -I$(PS2LIB)/common/include -I$(PS2LIB)/ee/include

IOPCFLAGS = -O2 -fomit-frame-pointer -nostartfiles 
IOPINCLUDES = -I. -Iinclude -I$(PS2LIB)/common/include -I$(PS2LIB)/iop/include
IOPCOMPILE = $(IOPCC) $(IOPINCLUDES) $(IOPCFLAGS)
IOPLINK = $(IOPLD) -dc


#  ------------------------------------------------------------------------
#  PROJECTS TO BUILD

all: eeload


#  ------------------------------------------------------------------------
#  KERNEL BUILD INSTRUCTIONS

LDFLAGS = -L$(PS2LIB)/ee/lib
LDADD   = -lmc -lpad -lc -lkernel
OBJECTS = eeirq.o eedata.o eekernel.o eeinit.o eeload.o eeelf.o eedebug.o romdir.o

eeload: $(OBJECTS)
	$(EELD) -Wl,--oformat,binary,--Map,eeload.map -T linkfile $(EECFLAGS) $(OBJECTS) $(LDFLAGS) $(LDADD) -o ../../build/EELOAD

# restrict all but the kernel registers
eeirq.o: eeirq.c
	$(EECC) $(EEINCLUDES) $(EECFLAGS) -ffixed-2 -ffixed-3 -ffixed-4 -ffixed-5 -ffixed-6 -ffixed-7 -ffixed-8 -ffixed-9 -ffixed-10 -ffixed-11 -ffixed-12 -ffixed-13 -ffixed-14 -ffixed-15 -ffixed-16 -ffixed-17 -ffixed-18 -ffixed-19 -ffixed-20 -ffixed-21 -ffixed-22 -ffixed-23 -ffixed-24 -ffixed-25 -fcall-used-26 -fcall-used-27 -ffixed-30 -o $@ -c $<
%.o: %.c
	$(EECC) $(EEINCLUDES) $(EECFLAGS) -o $@ -c $<


clean:
	rm -f $(OBJECTS) eeload



