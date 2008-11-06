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


EECFLAGS  = -O2 -fomit-frame-pointer -mips3 -EL -nostartfiles -G0
EEINCLUDES = -I. -I$(PS2LIB)/common/include -I$(PS2LIB)/ee/include

IOPCFLAGS = -O2 -fomit-frame-pointer -nostartfiles -G0
IOPINCLUDES = -I. -I$(PS2LIB)/common/include -I$(PS2LIB)/iop/include
IOPCOMPILE = $(IOPCC) $(IOPINCLUDES) $(IOPCFLAGS)
IOPLINK = $(IOPLD) -dc


#  ------------------------------------------------------------------------
#  PROJECTS TO BUILD

all: start


#  ------------------------------------------------------------------------
#  KERNEL BUILD INSTRUCTIONS

OBJS = eestart.o iopstart.o start.o romdir.o
DIRS = eeload iopload

start: $(OBJS)
	for i in $(DIRS); do \
		(cd $$i; make; cd ..) \
	done; 
	$(EELD) -Wl,--oformat,binary -T linkfile $(EECFLAGS) $(OBJS) -o ../build/RESET

iopstart.o: iopstart.c
	$(IOPCC) $(IOPINCLUDES) $(IOPCFLAGS) -o $@ -c $<

eestart.o: eestart.c
	$(EECC) $(EEINCLUDES) $(EECFLAGS) -o $@ -c $<

romdir.o: romdir.c
	$(IOPCC) $(IOPINCLUDES) $(IOPCFLAGS) -o $@ -c $<

start.o: start.c
	$(IOPCC) $(IOPINCLUDES) $(IOPCFLAGS) -o $@ -c $<



clean:
	for i in $(DIRS); do \
		(cd $$i; make clean; cd ..) \
	done; 
	rm -f $(OBJS) start



