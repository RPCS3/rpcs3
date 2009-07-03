# Create a shared library libCDVDiso
AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libCDVDiso.a
INCLUDES = $(shell pkg-config --cflags gtk+-2.0) -I@srcdir@/../../../common/include -I@srcdir@/../../../3rdparty -I@srcdir@/Linux -I../../../../3rdparty/zlib

# Create a shared object by faking an exe (thanks to ODE makefiles)
traplibdir=$(prefix)
 
if DEBUGBUILD
preext=d
endif

EXEEXT=$(preext)@so_ext@

traplib_PROGRAMS=libCDVDiso
libCDVDiso_SOURCES=
libCDVDiso_DEPENDENCIES = libCDVDiso.a
libCDVDiso_LDFLAGS= @SHARED_LDFLAGS@
libCDVDiso_LDFLAGS+=-Wl,-soname,@libCDVDiso_SONAME@
libCDVDiso_LDADD=$(libCDVDiso_a_OBJECTS)

libCDVDiso_a_SOURCES = CDVDisop.cpp CDVDiso.h libiso.cpp libiso.h mkiso/mkiso.cpp

libCDVDiso_a_SOURCES += \
Linux/Config.cpp Linux/Config.h Linux/Linux.cpp \
Linux/interface.h Linux/support.c  \
Linux/interface.c Linux/support.h \
Linux/callbacks.h 
#Linux/callbacks.c

#SUBDIRS = Linux