# Create a shared library libDEV9null
AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libDEV9null.a
INCLUDES = -I@srcdir@/../../common/include -I@srcdir@/../../3rdparty -I@srcdir@/Linux

libDEV9null_a_CXXFLAGS = $(shell pkg-config --cflags gtk+-2.0)
libDEV9null_a_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)

# Create a shared object by faking an exe (thanks to ODE makefiles)
traplibdir=$(prefix)

if DEBUGBUILD
preext=d
endif

EXEEXT=$(preext)@so_ext@

traplib_PROGRAMS=libDEV9null
libDEV9null_SOURCES=
libDEV9null_DEPENDENCIES = libDEV9null.a
libDEV9null_LDFLAGS= @SHARED_LDFLAGS@
libDEV9null_LDFLAGS+=-Wl,-soname,@libDEV9null_SONAME@
libDEV9null_LDADD=$(libDEV9null_a_OBJECTS)

libDEV9null_a_SOURCES = DEV9.cpp DEV9.h Linux/Config.cpp

#SUBDIRS = Linux