EE_BIN  = pcsx2hostfs_ldr.elf
EE_OBJS = ps2link.o excepHandler.o exceptions.o
EE_INCS += -I../include

# This is for the sbv patch
SBVLITE = $(PS2SDK)/sbv
EE_INCS += -I$(SBVLITE)/include
EE_LDFLAGS += -L$(SBVLITE)/lib
EE_LIBS += -lpatches -ldebug
ifeq ($(SCREENSHOTS),1)
EE_LIBS += -lpacket -ldma
endif

# Normal low loading version
ifeq ($(BUILTIN_IRXS),1)
LOADADDR  = 0xa8000
else
LOADADDR  = 0xc0000
endif
STACKADDR = 0x100000

# This is to build a highloading version
ifeq ($(LOADHIGH),1)
ifeq ($(BUILTIN_IRXS),1)
LOADADDR  = 0x1ee8000
else
LOADADDR  = 0x1f00000
endif
STACKADDR = 0x1f40000
EE_CFLAGS += -D_LOADHIGHVER
endif

# This is to builtin the IRXs into ps2link
ifeq ($(BUILTIN_IRXS),1)
EE_CFLAGS += -DBUILTIN_IRXS
EE_LDFLAGS += pcsx2hostfs_irx.o ioptrap_irx.o
endif

ifeq ($(BUILTIN_IRXS),1)
ifeq ($(CACHED_CFG),1)
EE_CFLAGS += -DUSE_CACHED_CFG
endif
endif

# This is to enable the debug mode into ps2link
ifeq ($(DEBUG),1)
EE_CFLAGS += -DDEBUG -g
endif

# Enable screenshot functionality
ifeq ($(SCREENSHOTS),1)
EE_CFLAGS += -DSCREENSHOTS
endif

LDPARAMS := -Wl,--defsym -Wl,_stack_size=0x8000 -Wl,--defsym -Wl,_stack=$(STACKADDR)
EE_LDFLAGS += -Wl,-Ttext -Wl,$(LOADADDR) $(LDPARAMS)

ifeq ($(DEBUG),1)
EE_LDFLAGS += -g
else
EE_LDFLAGS += -s
endif

all: $(EE_BIN)

clean:
	-rm -f $(EE_OBJS) $(EE_BIN)

check: $(EE_BIN)
	@ee-readelf -l $(EE_BIN) | awk ' /LOAD/ { if ((spare = (-($$4 + $$6) + ("'$(STACKADDR)'" - "0x8000"))) <= 0) { printf("PS2Link is too big, %i (0x%x) bytes missing\n", -spare, -spare); exit -1; } else { printf("PS2Link has %i (0x%x) spare bytes\n", spare, spare) } } '

include $(PS2SDK)/Defs.make
include Rules.make
