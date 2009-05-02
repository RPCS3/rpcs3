# Create a shared library libFWnull
AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libFWnull.a
INCLUDES = -I@srcdir@/../../common/include -I@srcdir@/../../3rdparty -I@srcdir@/Linux

libFWnull_a_CXXFLAGS = $(shell pkg-config --cflags gtk+-2.0)
libFWnull_a_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)

# Create a shared object by faking an exe (thanks to ODE makefiles)
traplibdir=$(prefix)

if DEBUGBUILD
preext=d
endif

EXEEXT=$(preext)@so_ext@

traplib_PROGRAMS=libFWnull
libFWnull_SOURCES=
libFWnull_DEPENDENCIES = libFWnull.a
libFWnull_LDFLAGS= @SHARED_LDFLAGS@
libFWnull_LDFLAGS+=-Wl,-soname,@libFWnull_SONAME@
libFWnull_LDADD=$(libFWnull_a_OBJECTS)

libFWnull_a_SOURCES = FW.cpp Linux/Config.cpp Linux/Linux.cpp \
FW.h Linux/Config.h Linux/Linux.h

libFWnull_a_SOURCES += \
Linux/interface.h Linux/support.c  \
Linux/interface.c Linux/support.h \
Linux/callbacks.c Linux/callbacks.h 

#SUBDIRS = Linux