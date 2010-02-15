# Compilation variables

# Set this to 1 to enable debug mode
DEBUG = 0

# Set this to 1 to build a highloading version, 0 for normal low version
LOADHIGH = 1

# Set this to 1 to build ps2link with all the needed IRX builtins
BUILTIN_IRXS = 1

# Set this to 1 to enable caching of config files
CACHED_CFG = 1

# Set this to 1 to enable zero-copy on fileio writes.
ZEROCOPY = 0

# Set this to 1 to power off the ps2 when the reset button is tapped
# otherwise it will try and reset ps2link
PWOFFONRESET = 1

# Set this to 1 to hook the kernel CreateThread/DeleteThread calls.
# Note that this will cause problems when loading PS2LINK.ELF from PS2LINK...
HOOK_THREADS = 0

# Set this to 1 to enable screenshots.
# Note that this adds a dependency with libgraph and libdma
SCREENSHOTS = 0

# Set to the path where ps2eth is located
PS2ETH = $(PS2DEV)/ps2eth

SHELL=/usr/bin/env bash
BIN2O=$(PS2SDK)/bin/bin2o
RM=rm -f

#
# You shouldn't need to modify anything below this point
#

include $(PS2SDK)/Defs.make

EEFILES=ee/pcsx2hostfs_ldr.elf

IRXFILES=iop/pcsx2hostfs.irx $(PS2SDK)/iop/irx/ioptrap.irx

VARIABLES=DEBUG=$(DEBUG) LOADHIGH=$(LOADHIGH) BUILTIN_IRXS=$(BUILTIN_IRXS) ZEROCOPY=$(ZEROCOPY) PWOFFONRESET=$(PWOFFONRESET) CACHED_CFG=$(CACHED_CFG) HOOK_THREADS=$(HOOK_THREADS) SCREENSHOTS=$(SCREENSHOTS)

ifeq ($(BUILTIN_IRXS),1)
TARGETS = iop builtins ee
else
TARGETS = ee iop
endif

all: $(TARGETS)
ifneq ($(BUILTIN_IRXS),1)
	@for file in $(IRXFILES); do \
		new=`echo $${file/*\//}|tr "[:lower:]" "[:upper:]"`; \
		cp $$file bin/$$new; \
	done;
endif
	@for file in $(EEFILES); do \
		new=`echo $${file/*\//}|tr "[:lower:]" "[:upper:]"`; \
		cp $$file bin/$$new; \
	done;

ee:
	$(VARIABLES) $(MAKE) -C ee

iop:
	$(VARIABLES) $(MAKE) -C iop

clean:
	$(MAKE) -C ee clean
	$(MAKE) -C iop clean

check:
	$(VARIABLES) $(MAKE) -C ee check

# Creates a zip from what you have
dist: all
	@rm -rf dist
	@mkdir -p dist/ps2link
ifneq ($(BUILTIN_IRXS),1)
	@for file in $(IRXFILES); do \
		new=`echo $${file/*\//}|tr "[:lower:]" "[:upper:]"`; \
		cp $$file dist/ps2link/$$new; \
	done;
endif
	@for file in $(EEFILES); do \
		new=`echo $${file/*\//}|tr "[:lower:]" "[:upper:]"`; \
		cp $$file dist/ps2link/$$new; \
	done;
	cd dist; \
	tar -jcf ps2link.tar.bz2 ps2link/

RELEASE_FILES=bin/*IRX bin/*DAT bin/*cnf bin/*ELF LICENSE README
#
# Creates zip with iso and all necessary files of last release
release:
	@rm -rf RELEASE
	@mkdir -p RELEASE
	@VERSION=`cvs log Makefile | grep -A 1 symbolic | tail -1 | awk '{print substr($$1, 0, length($$1)-1)}'`; \
	cd RELEASE; \
	cvs co -r $$VERSION ps2link; \
	cd ps2link; \
	make; \
	make check; \
	mkdir -p bin; \
	for file in $(IRXFILES); do \
		new=`echo $${file/*\//}|tr "[:lower:]" "[:upper:]"`; \
		cp $$file bin/$$new; \
	done; \
	for file in $(EEFILES); do \
		new=`echo $${file/*\//}|tr "[:lower:]" "[:upper:]"`; \
		cp $$file bin/$$new; \
	done; \
	dd if=/dev/zero of=bin/dummy bs=1024 count=28672; \
	ps2mkisofs -o ps2link_$$VERSION.iso bin/; \
	rm bin/dummy; \
	tar -jcf ps2link_$$VERSION.tbz $(RELEASE_FILES) ps2link_$$VERSION.iso

docs:
	doxygen doxy.conf

builtins:
	@for file in $(IRXFILES); do \
		basefile=$${file/*\//}; \
		basefile=$${basefile/\.*/}; \
		echo "Embedding IRX file $$basefile"; \
		$(BIN2O) $$file ee/$${basefile}_irx.o _binary_$${basefile}_irx; \
	done;

.PHONY: iop ee
