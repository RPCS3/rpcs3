##############################################################################
# MAKEFILE FOR THE PEOPS OSS SPU2... just run "make"
##############################################################################

##############################################################################
# 1. SETS (CCFLAGS3 is used)
##############################################################################
all: spu2PeopsOSS
install: all

# Set to TRUE to build the ALSA support
USEALSA = FALSE
# Set to TRUE to disable the thread library support - helpful for some Linux distros
NOTHREADLIB = FALSE

##############################################################################

CC = gcc
CCFLAGS1 = -fPIC -c -Wall -O3 -m32 
CCFLAGS2 = -fPIC -c -Wall -O2 -ffast-math -m32 
CCFLAGS3 = -fPIC -c -Wall -O3 -ffast-math -fomit-frame-pointer -m32 

INCLUDE =
LINK = gcc
OBJ =   spu.o dma.o freeze.o registers.o
LIB = -lc -lm

ifeq ($(USEALSA), TRUE)
	OBJ+= alsa.o
	LIB+= -lasound
	LINKFLAGS = -shared -Wl,-soname,libspu2PeopsALSA.so -o libspu2PeopsALSA.so.1.0.3
	CCFLAGS3+= -DUSEALSA
else
	OBJ+= oss.o
	LINKFLAGS = -shared -Wl,-soname,libspu2PeopsOSS.so.1.6 -fPIC -fomit-frame-pointer -o libspu2PeopsOSS.so.1.6 -m32 
endif

ifeq ($(NOTHREADLIB), TRUE)
	CCFLAGS3+= -DNOTHREADLIB
else
	LIB+= -lpthread
endif



##############################################################################
# 2. MAIN RULE
##############################################################################

spu2PeopsOSS :	$(OBJ)
		$(LINK) $(LINKFLAGS) $(OBJ) $(LIB)

##############################################################################
# 3. GENERAL RULES
##############################################################################

%.o     : %.c
	$(CC) $(CCFLAGS3) $(INCLUDE) $<

##############################################################################
# 4. SPECIFIC RULES
##############################################################################

spu.o  : spu.c stdafx.h externals.h cfg.h dsoundoss.h regs.h debug.h xa.c reverb.c adsr.c
cfg.o  : stdafx.h externals.h
dma.o : dma.c stdafx.h externals.h
freeze.o : freeze.c stdafx.h externals.h registers.h spu.h regs.h
oss.o : oss.c stdafx.h externals.h
alsa.o : alsa.h stdafx.h externals.h
registers.o : registers.c stdafx.h externals.h registers.h regs.h reverb.h

.PHONY: clean spu2PeopsOSS

clean:
	rm -f *.o *.so.*
