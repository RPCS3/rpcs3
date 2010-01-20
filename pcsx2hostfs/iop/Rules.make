# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004.
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.


IOP_CC_VERSION := $(shell $(IOP_CC) -v 2>&1 | sed -n 's/^.*version //p')

ASFLAGS_TARGET = -mcpu=r3000

ifeq ($(IOP_CC_VERSION),3.2.2)
CFLAGS_TARGET  = -miop
ASFLAGS_TARGET = -march=r3000
LDFLAGS_TARGET = -miop
endif

IOP_INCS := $(IOP_INCS) -I$(PS2SDK)/iop/include -I$(PS2SDK)/common/include 

IOP_CFLAGS  := $(CFLAGS_TARGET) -O2 -G0 -c $(IOP_INCS) $(IOP_CFLAGS)
IOP_ASFLAGS := $(ASFLAGS_TARGET) -EL -G0 $(IOP_ASFLAGS)
IOP_LDFLAGS := $(LDFLAGS_TARGET) -nostdlib $(IOP_LDFLAGS)

# Externally defined variables: IOP_BIN, IOP_OBJS, IOP_LIB

%.o : %.c
	$(IOP_CC) $(IOP_CFLAGS) $< -o $@

%.o : %.s
	$(IOP_AS) $(IOP_ASFLAGS) $< -o $@

# A rule to build imports.lst.
%.o : %.lst
	echo "#include \"irx_imports.h\"" > build-imports.c
	cat $< >> build-imports.c
	$(IOP_CC) $(IOP_CFLAGS) build-imports.c -o $@
	-rm -f build-imports.c

# A rule to build exports.tab.
%.o : %.tab
	echo "#include \"irx.h\"" > build-exports.c
	cat $< >> build-exports.c
	$(IOP_CC) $(IOP_CFLAGS) build-exports.c -o $@
	-rm -f build-exports.c


$(IOP_BIN) : $(IOP_OBJS)
	$(IOP_CC) $(IOP_LDFLAGS) -o $(IOP_BIN) $(IOP_OBJS) $(IOP_LIBS)

$(IOP_LIB) : $(IOP_OBJS)
	$(IOP_AR) cru $(IOP_LIB) $(IOP_OBJS)

