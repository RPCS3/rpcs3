INCLUDES = -I@srcdir@/.. -I@srcdir@/../../
noinst_LIBRARIES = libix86.a

libix86_a_SOURCES = ix86_3dnow.c  ix86.c  ix86_cpudetect.c  ix86_fpu.c  ix86.h ix86_sse.c

if X86_64
else
libix86_a_SOURCES += ix86_mmx.c
endif
