# Create a shared library libZeroSPU2
AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libZeroSPU2.a

libZeroSPU2_a_CXXFLAGS = $(shell pkg-config --cflags gtk+-2.0)
libZeroSPU2_a_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)

if X86_64
libZeroSPU2_a_CXXFLAGS += -fPIC
libZeroSPU2_a_CFLAGS += -fPIC
endif

# Create a shared object by faking an exe (thanks to ODE makefiles)
traplibdir=$(prefix)

if DEBUGBUILD
preext=d
endif

EXEEXT=$(preext)@so_ext@

traplib_PROGRAMS=libZeroSPU2
libZeroSPU2_SOURCES=
libZeroSPU2_DEPENDENCIES = libZeroSPU2.a SoundTouch/libSoundTouch.a
libZeroSPU2_LDFLAGS= @SHARED_LDFLAGS@
libZeroSPU2_LDFLAGS+=-Wl,-soname,@ZEROSPU2_SONAME@
libZeroSPU2_LDADD=$(libZeroSPU2_a_OBJECTS) SoundTouch/libSoundTouch.a

libZeroSPU2_a_SOURCES = zerospu2.cpp Linux.cpp

SUBDIRS = SoundTouch .