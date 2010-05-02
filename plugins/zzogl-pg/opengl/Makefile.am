# Create a shared library libzzoglpg
AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libzzoglpg.a
INCLUDES = -I@srcdir@/../../../common/include -I@srcdir@/common  -I@srcdir@/../../../3rdparty/

libzzoglpg_a_CPPFLAGS = $(shell pkg-config --cflags gtk+-2.0)
libzzoglpg_a_CXXFLAGS = $(shell pkg-config --cflags gtk+-2.0)

# Create a shared object by faking an exe (thanks to ODE makefiles)
traplibdir=$(prefix)


if RELEASE_TO_PUBLIC
preext=r
endif

EXEEXT=$(preext)@so_ext@

traplib_PROGRAMS=libzzoglpg
libzzoglpg_SOURCES=
libzzoglpg_DEPENDENCIES = libzzoglpg.a ../../../deps/release/libUtilities.a
libzzoglpg_LDFLAGS= @SHARED_LDFLAGS@
libzzoglpg_LDFLAGS+=-Wl,-soname,@ZEROGS_SONAME@
libzzoglpg_LDADD=$(libzzoglpg_a_OBJECTS)

libzzoglpg_a_SOURCES = \
GSmain.cpp GLWinX11.cpp GifTransfer.cpp memcpy_amd.cpp  Regs.cpp x86.cpp zpipe.cpp Mem.cpp Profile.cpp Profile.h     \
rasterfont.cpp  targets.cpp  zerogs.cpp  ZZoglVB.cpp  ZZoglShoots.cpp	ZZoglCreate.cpp	\
ZZoglShaders.cpp	ZZoglCRTC.cpp	ZZoglSave.cpp	ZZoglFlush.cpp \
Mem_Swizzle.h Mem_Tables.cpp Mem_Transmit.h Mem_Swizzle.cpp


libzzoglpg_a_SOURCES += x86-32.S

if SSE2
CCASFLAGS+= -DZEROGS_SSE2
endif

libzzoglpg_a_DEPENDENCIES = Linux/libZeroGSLinux.a ../../../deps/release/libUtilities.a
libzzoglpg_LDADD += Linux/libZeroGSLinux.a ../../../deps/release/libUtilities.a

SUBDIRS = Linux ZeroGSShaders .
