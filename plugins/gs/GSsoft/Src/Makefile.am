# Create a shared library libGSSoft
AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libGSSoft.a

libGSSoft_a_CPPFLAGS = $(shell pkg-config --cflags gtk+-2.0) $(shell pkg-config --cflags sdl)
libGSSoft_a_CXXFLAGS = $(shell pkg-config --cflags gtk+-2.0) $(shell pkg-config --cflags sdl)

if X86_64
libGSSoft_a_CPPFLAGS += -fPIC
libGSSoft_a_CXXFLAGS += -fPIC
CCASFLAGS += -fPIC
endif

# Create a shared object by faking an exe (thanks to ODE makefiles)
traplibdir=$(prefix)

if RELEASE_TO_PUBLIC
preext=r
endif

EXEEXT=$(preext)@so_ext@

traplib_PROGRAMS=libGSSoft
libGSSoft_SOURCES=
libGSSoft_DEPENDENCIES = libGSSoft.a
libGSSoft_LDFLAGS= @SHARED_LDFLAGS@
libGSSoft_LDFLAGS+=-Wl,-soname,@GSSOFT_SONAME@
libGSSoft_LDADD=$(libGSSoft_a_OBJECTS)

libGSSoft_a_SOURCES = \
Cache.c Color.c Draw.c GS.c Mem.c Page.c Prim.c Rec.c Regs.c \
scale2x.c SDL.c SDL_gfxPrimitives.c Soft.c Texts.c Transfer.c \
Linux/Conf.c Linux/interface.c Linux/Linux.c \
Linux/support.c x86/ix86_cpudetect.c 

