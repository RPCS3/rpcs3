# makefile for libpng
# Copyright (C) 1998 Tim Wegner
# For conditions of distribution and use, see copyright notice in png.h
# Assumes that zlib.lib, zconf.h, and zlib.h have been copied to ..\zlib
# To use, do "nmake /f scripts\makefile.vcwin32"

# -------- Microsoft Visual C++ 2.0 and later, no assembler code --------
# If you want to use assembler (MMX) code, use makefile.vcawin32 instead.

# Compiler, linker, librarian, and other tools
CC = cl
LD = link
AR = lib
CFLAGS  = -nologo -MD -O2 -W3 -I..\zlib
LDFLAGS = -nologo
ARFLAGS = -nologo
RM = del

# File extensions
O=.obj

#uncomment next to put error messages in a file
#ERRFILE= >> pngerrs.log

# Variables
OBJS1 = png$(O) pngerror$(O) pngget$(O) pngmem$(O) pngpread$(O)
OBJS2 = pngread$(O) pngrio$(O) pngrtran$(O) pngrutil$(O) pngset$(O)
OBJS3 = pngtrans$(O) pngwio$(O) pngwrite$(O) pngwtran$(O) pngwutil$(O)
OBJS  = $(OBJS1) $(OBJS2) $(OBJS3)

# Targets
all: libpng.lib

png$(O): png.h pngconf.h
	$(CC) -c $(CFLAGS) $*.c $(ERRFILE)

pngset$(O): png.h pngconf.h
	$(CC) -c $(CFLAGS) $*.c $(ERRFILE)

pngget$(O): png.h pngconf.h
	$(CC) -c $(CFLAGS) $*.c $(ERRFILE)

pngread$(O): png.h pngconf.h
	$(CC) -c $(CFLAGS) $*.c $(ERRFILE)

pngpread$(O): png.h pngconf.h
	$(CC) -c $(CFLAGS) $*.c $(ERRFILE)

pngrtran$(O): png.h pngconf.h
	$(CC) -c $(CFLAGS) $*.c $(ERRFILE)

pngrutil$(O): png.h pngconf.h
	$(CC) -c $(CFLAGS) $*.c $(ERRFILE)

pngerror$(O): png.h pngconf.h
	$(CC) -c $(CFLAGS) $*.c $(ERRFILE)

pngmem$(O): png.h pngconf.h
	$(CC) -c $(CFLAGS) $*.c $(ERRFILE)

pngrio$(O): png.h pngconf.h
	$(CC) -c $(CFLAGS) $*.c $(ERRFILE)

pngwio$(O): png.h pngconf.h
	$(CC) -c $(CFLAGS) $*.c $(ERRFILE)

pngtest$(O): png.h pngconf.h
	$(CC) -c $(CFLAGS) $*.c $(ERRFILE)

pngtrans$(O): png.h pngconf.h
	$(CC) -c $(CFLAGS) $*.c $(ERRFILE)

pngwrite$(O): png.h pngconf.h
	$(CC) -c $(CFLAGS) $*.c $(ERRFILE)

pngwtran$(O): png.h pngconf.h
	$(CC) -c $(CFLAGS) $*.c $(ERRFILE)

pngwutil$(O): png.h pngconf.h
	$(CC) -c $(CFLAGS) $*.c $(ERRFILE)

libpng.lib: $(OBJS)
	-$(RM) $@
	$(AR) $(ARFLAGS) -out:$@ $(OBJS) $(ERRFILE)

pngtest.exe: pngtest$(O) libpng.lib
	$(LD) $(LDFLAGS) -out:$@ pngtest$(O) libpng.lib ..\zlib\zlib.lib $(ERRFILE)

test: pngtest.exe
	pngtest

clean:
	-$(RM) *$(O)
	-$(RM) libpng.lib
	-$(RM) pngtest.exe
	-$(RM) pngout.png

# End of makefile for libpng

