# Create a shared library libOnePAD
AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libOnePAD.a
INCLUDES = -I@srcdir@/../../common/include

libOnePAD_a_CXXFLAGS = $(shell pkg-config --cflags gtk+-2.0)
libOnePAD_a_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)

# Create a shared object by faking an exe (thanks to ODE makefiles)
traplibdir=$(prefix)

if DEBUGBUILD
preext=d
endif

EXEEXT=$(preext)@so_ext@

traplib_PROGRAMS=libOnePAD
libOnePAD_SOURCES=
libOnePAD_DEPENDENCIES = libOnePAD.a
libOnePAD_LDFLAGS= @SHARED_LDFLAGS@
libOnePAD_LDFLAGS+=-Wl,-soname,@ZEROPAD_SONAME@
libOnePAD_LDADD=$(libOnePAD_a_OBJECTS)

libOnePAD_a_SOURCES = joystick.cpp analog.cpp analog.h onepad.cpp onepad.h controller.cpp controller.h \
Linux/gui.cpp Linux/linux.cpp  Linux/support.c Linux/interface.c Linux/ini.cpp keyboard.cpp keyboard.h
