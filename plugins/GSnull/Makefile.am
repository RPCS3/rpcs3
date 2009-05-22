# Create a shared library libGSnull
AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libGSnull.a
INCLUDES = -I@srcdir@/../../common/include -I@srcdir@/../../3rdparty -I@srcdir@/Linux

libGSnull_a_CXXFLAGS = $(shell pkg-config --cflags gtk+-2.0)
libGSnull_a_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)

# Create a shared object by faking an exe (thanks to ODE makefiles)
traplibdir=$(prefix)

if DEBUGBUILD
preext=d
endif

EXEEXT=$(preext)@so_ext@

traplib_PROGRAMS=libGSnull
libGSnull_SOURCES=
libGSnull_DEPENDENCIES = libGSnull.a
libGSnull_LDFLAGS= @SHARED_LDFLAGS@
libGSnull_LDFLAGS+=-Wl,-soname,@libGSnull_SONAME@
libGSnull_LDADD=$(libGSnull_a_OBJECTS)

libGSnull_a_SOURCES = \
GS.cpp \
Linux/Config.cpp Linux/Linux.cpp Linux/GSLinux.cpp
Linux/GSLinux.h Linux/Config.h Linux/Linux.h \
GifTransfer.cpp GifTransfer.h \
null/GSnull.cpp null/GSnull.h 

libGSnull_a_SOURCES += \
Linux/interface.h Linux/support.c  \
Linux/interface.c Linux/support.h \
Linux/callbacks.h