# Create a shared library libZeroGSogl
AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libZeroGSogl.a
INCLUDES = -I@srcdir@/../../../common/include -I@srcdir@/../../../3rdparty

libZeroGSogl_a_CPPFLAGS = $(shell pkg-config --cflags gtk+-2.0)
libZeroGSogl_a_CXXFLAGS = $(shell pkg-config --cflags gtk+-2.0)

# Create a shared object by faking an exe (thanks to ODE makefiles)
traplibdir=$(prefix)

if RELEASE_TO_PUBLIC
preext=r
endif

EXEEXT=$(preext)@so_ext@

traplib_PROGRAMS=libZeroGSogl
libZeroGSogl_SOURCES=
libZeroGSogl_DEPENDENCIES = libZeroGSogl.a
libZeroGSogl_LDFLAGS= @SHARED_LDFLAGS@
libZeroGSogl_LDFLAGS+=-Wl,-soname,@ZEROGS_SONAME@
libZeroGSogl_LDADD=$(libZeroGSogl_a_OBJECTS)

libZeroGSogl_a_SOURCES = \
GSmain.cpp  memcpy_amd.cpp  Regs.cpp     x86.cpp     zpipe.cpp \
Mem.cpp     rasterfont.cpp  targets.cpp  zerogs.cpp GifTransfer.cpp

libZeroGSogl_a_SOURCES += x86-32.S

if SSE2
CCASFLAGS+= -DZEROGS_SSE2
endif

libZeroGSogl_a_DEPENDENCIES = Linux/libZeroGSLinux.a
libZeroGSogl_LDADD += Linux/libZeroGSLinux.a

SUBDIRS = Linux .
