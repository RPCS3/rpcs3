noinst_LIBRARIES = libZeroGSLinux.a

if X86_64
libZeroGSLinux_a_CPPFLAGS = -fPIC
libZeroGSLinux_a_CXXFLAGS = -fPIC
libZeroGSLinux_a_CFLAGS = -fPIC
endif

INCLUDES = $(shell pkg-config --cflags gtk+-2.0) -I@srcdir@/../ -I@srcdir@/../common
libZeroGSLinux_a_SOURCES = callbacks.c Conf.cpp interface.c Linux.cpp support.c
