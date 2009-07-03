# Create a shared library libUSBnull
AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libUSBnull.a
INCLUDES = -I@srcdir@/../../common/include -I@srcdir@/../../3rdparty -I@srcdir@/Linux

libUSBnull_a_CXXFLAGS = $(shell pkg-config --cflags gtk+-2.0)
libUSBnull_a_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)

# Create a shared object by faking an exe (thanks to ODE makefiles)
traplibdir=$(prefix)

if DEBUGBUILD
preext=d
endif

EXEEXT=$(preext)@so_ext@

traplib_PROGRAMS=libUSBnull
libUSBnull_SOURCES=
libUSBnull_DEPENDENCIES = libUSBnull.a
libUSBnull_LDFLAGS= @SHARED_LDFLAGS@
libUSBnull_LDFLAGS+=-Wl,-soname,@libUSBnull_SONAME@
libUSBnull_LDADD=$(libUSBnull_a_OBJECTS)

libUSBnull_a_SOURCES = USB.cpp Linux/Config.cpp Linux/Linux.cpp \
USB.h Linux/Config.h Linux/Linux.h

libUSBnull_a_SOURCES += \
Linux/interface.h Linux/support.c  \
Linux/interface.c Linux/support.h \
Linux/callbacks.h 

#SUBDIRS = Linux