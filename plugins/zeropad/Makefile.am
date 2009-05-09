# Create a shared library libZeroPAD
AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libZeroPAD.a
INCLUDES = -I@srcdir@/../../common/include

libZeroPAD_a_CXXFLAGS = $(shell pkg-config --cflags gtk+-2.0)
libZeroPAD_a_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)

if X86_64
libZeroPAD_a_CXXFLAGS += -fPIC
libZeroPAD_a_CFLAGS += -fPIC
endif

# Create a shared object by faking an exe (thanks to ODE makefiles)
traplibdir=$(prefix)

if DEBUGBUILD
preext=d
endif

EXEEXT=$(preext)@so_ext@

traplib_PROGRAMS=libZeroPAD
libZeroPAD_SOURCES=
libZeroPAD_DEPENDENCIES = libZeroPAD.a
libZeroPAD_LDFLAGS= @SHARED_LDFLAGS@
libZeroPAD_LDFLAGS+=-Wl,-soname,@ZEROPAD_SONAME@
libZeroPAD_LDADD=$(libZeroPAD_a_OBJECTS)

libZeroPAD_a_SOURCES = analog.cpp analog.h zeropad.cpp zeropad.h \
Linux/gui.cpp Linux/linux.cpp Linux/joystick.cpp Linux/support.c Linux/interface.c
