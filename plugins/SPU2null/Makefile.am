# Create a shared library libSPU2null
AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libSPU2null.a
INCLUDES = -I@srcdir@/../../common/include -I@srcdir@/../../3rdparty -I@srcdir@/Linux

libSPU2null_a_CXXFLAGS = $(shell pkg-config --cflags gtk+-2.0)
libSPU2null_a_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)

# Create a shared object by faking an exe (thanks to ODE makefiles)
traplibdir=$(prefix)

if DEBUGBUILD
preext=d
endif

EXEEXT=$(preext)@so_ext@

traplib_PROGRAMS=libSPU2null
libSPU2null_SOURCES=
libSPU2null_DEPENDENCIES = libSPU2null.a
libSPU2null_LDFLAGS= @SHARED_LDFLAGS@
libSPU2null_LDFLAGS+=-Wl,-soname,@libSPU2null_SONAME@
libSPU2null_LDADD=$(libSPU2null_a_OBJECTS)

libSPU2null_a_SOURCES = SPU2.cpp SPU2.h Linux/Config.cpp

#SUBDIRS = Linux