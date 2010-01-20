#  _____     ___ ____
#   ____|   |    ____|      PSX2 OpenSource Project
#  |     ___|   |____       (C)2002, David Ryan ( Oobles@hotmail.com )
#  ------------------------------------------------------------------------

# Generated automatically from Makefile.in by configure.
#.SUFFIXES: .S .c .o .s .elf .irx

IOP_BIN  = pcsx2hostfs.irx
IOP_OBJS = net_fsys.o net_fio.o ps2link.o tty.o nprintf.o excepHandler.o imports.o

IOP_INCS += -I../include
IOP_LIBS += 
IOP_LDFLAGS += 
IOP_CFLAGS += -Wall

# Enable zero-copy on fileio writes.
ifeq ($(ZEROCOPY),1)
IOP_CFLAGS += -DZEROCOPY
endif

# Enable debug mode
ifeq ($(DEBUG),1)
IOP_CFLAGS += -DDEBUG
endif

ifeq ($(PWOFFONRESET),1)
IOP_CFLAGS += -DPWOFFONRESET
endif

# Enable screenshot functionality
ifeq ($(SCREENSHOTS),1)
IOP_CFLAGS += -DSCREENSHOTS
endif


all: $(IOP_BIN)

clean:
	-rm -f $(IOP_OBJS) $(IOP_BIN)

include $(PS2SDK)/Defs.make
include Rules.make
