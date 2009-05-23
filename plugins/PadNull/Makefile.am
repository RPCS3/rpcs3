# Create a shared library libPadnull
AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libPadnull.a
INCLUDES = -I@srcdir@/../../common/include -I@srcdir@/../../3rdparty -I@srcdir@/Linux

libPadnull_a_CXXFLAGS = $(shell pkg-config --cflags gtk+-2.0)
libPadnull_a_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)

# Create a shared object by faking an exe (thanks to ODE makefiles)
traplibdir=$(prefix)

if DEBUGBUILD
preext=d
endif

EXEEXT=$(preext)@so_ext@

traplib_PROGRAMS=libPadnull
libPadnull_SOURCES=
libPadnull_DEPENDENCIES = libPadnull.a
libPadnull_LDFLAGS= @SHARED_LDFLAGS@
libPadnull_LDFLAGS+=-Wl,-soname,@libPadnull_SONAME@
libPadnull_LDADD=$(libPadnull_a_OBJECTS)

libPadnull_a_SOURCES = Pad.cpp Pad.h Linux/Config.cpp Linux/Config.h \
Linux/PadLinux.cpp Linux/PadLinux.h

libPadnull_a_SOURCES += \
Linux/interface.h Linux/support.c  \
Linux/interface.c Linux/support.h \
Linux/callbacks.h 