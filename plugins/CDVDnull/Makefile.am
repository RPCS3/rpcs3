# Create a shared library libCDVDnull
AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libCDVDnull.a
INCLUDES = -I@srcdir@/../../common/include -I@srcdir@/../../3rdparty -I@srcdir@/Linux

libCDVDnull_a_CXXFLAGS = $(shell pkg-config --cflags gtk+-2.0)
libCDVDnull_a_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)

# Create a shared object by faking an exe (thanks to ODE makefiles)
traplibdir=$(prefix)

if DEBUGBUILD
preext=d
endif

EXEEXT=$(preext)@so_ext@

traplib_PROGRAMS=libCDVDnull
libCDVDnull_SOURCES=
libCDVDnull_DEPENDENCIES = libCDVDnull.a
libCDVDnull_LDFLAGS= @SHARED_LDFLAGS@
libCDVDnull_LDFLAGS+=-Wl,-soname,@libCDVDnull_SONAME@
libCDVDnull_LDADD=$(libCDVDnull_a_OBJECTS)

libCDVDnull_a_SOURCES = CDVD.cpp CDVD.h Linux/Config.cpp

#SUBDIRS = Linux