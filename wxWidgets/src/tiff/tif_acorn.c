/* $Header$ */

/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library RISC OS specific Routines.
 * Developed out of the Unix version.
 * Peter Greenham, May 1995
 */
#include "tiffiop.h"
#include <stdio.h>
#include <stdlib.h>

/*
Low-level file handling
~~~~~~~~~~~~~~~~~~~~~~~
The functions in osfcn.h are unavailable when compiling under C, as it's a
C++ header. Therefore they have been implemented here.

Now, why have I done it this way?

The definitive API library for RISC OS is Jonathan Coxhead's OSLib, which
uses heavily optimised ARM assembler or even plain inline SWI calls for
maximum performance and minimum runtime size. However, I don't want to make
LIBTIFF need that to survive. Therefore I have also emulated the functions
using macros to _swi() and _swix() defined in the swis.h header, and
borrowing types from kernel.h, which is less efficient but doesn't need any
third-party libraries.
 */

#ifdef INCLUDE_OSLIB

#include "osfile.h"
#include "osgbpb.h"
#include "osargs.h"
#include "osfind.h"

#else

/* OSLIB EMULATION STARTS */

#include "kernel.h"
#include "swis.h"

/* From oslib:types.h */
typedef unsigned int                            bits;
typedef unsigned char                           byte;
#ifndef TRUE
#define TRUE                                    1
#endif
#ifndef FALSE
#define FALSE                                   0
#endif
#ifndef NULL
#define NULL                                    0
#endif
#ifndef SKIP
#define SKIP                                    0
#endif

/* From oslib:os.h */
typedef _kernel_oserror os_error;
typedef byte os_f;

/* From oslib:osfile.h */
#undef  OS_File
#define OS_File                                 0x8

/* From oslib:osgbpb.h */
#undef  OS_GBPB
#define OS_GBPB                                 0xC
#undef  OSGBPB_Write
#define OSGBPB_Write                            0x2
#undef  OSGBPB_Read
#define OSGBPB_Read                             0x4

extern os_error *xosgbpb_write (os_f file,
      byte *data,
      int size,
      int *unwritten);
extern int osgbpb_write (os_f file,
      byte *data,
      int size);

#define	xosgbpb_write(file, data, size, unwritten) \
	(os_error*) _swix(OS_GBPB, _IN(0)|_IN(1)|_IN(2)|_IN(3)|_IN(4)|_OUT(3), \
		OSGBPB_WriteAt, \
		file, \
		data, \
		size, \
		unwritten)

#define	osgbpb_write(file, data, size) \
	_swi(OS_GBPB, _IN(0)|_IN(1)|_IN(2)|_IN(3)|_RETURN(3), \
		OSGBPB_Write, \
		file, \
		data, \
		size)

extern os_error *xosgbpb_read (os_f file,
      byte *buffer,
      int size,
      int *unread);
extern int osgbpb_read (os_f file,
      byte *buffer,
      int size);

#define	xosgbpb_read(file, buffer, size, unread) \
	(os_error*) _swix(OS_GBPB, _IN(0)|_IN(1)|_IN(2)|_IN(3)|_OUT(3), \
		OSGBPB_Read, \
		file, \
		buffer, \
		size, \
		unread)

#define	osgbpb_read(file, buffer, size) \
	_swi(OS_GBPB, _IN(0)|_IN(1)|_IN(2)|_IN(3)|_RETURN(3), \
		OSGBPB_Read, \
		file, \
		buffer, \
		size)

/* From oslib:osfind.h */
#undef  OS_Find
#define OS_Find                                 0xD
#undef  OSFind_Openin
#define OSFind_Openin                           0x40
#undef  OSFind_Openout
#define OSFind_Openout                          0x80
#undef  OSFind_Openup
#define OSFind_Openup                           0xC0
#undef  OSFind_Close
#define OSFind_Close                            0x0

#define	xosfind_open(reason, file_name, path, file) \
	(os_error*) _swix(OS_Find, _IN(0)|_IN(1)|_IN(2)|_OUT(0), \
		reason, file_name, path, file)

#define	osfind_open(reason, file_name, path) \
	(os_f) _swi(OS_Find, _IN(0)|_IN(1)|_IN(2)|_RETURN(0), \
		reason, file_name, path)

extern os_error *xosfind_openin (bits flags,
      char *file_name,
      char *path,
      os_f *file);
extern os_f osfind_openin (bits flags,
      char *file_name,
      char *path);

#define	xosfind_openin(flags, file_name, path, file) \
	xosfind_open(flags | OSFind_Openin, file_name, path, file)

#define	osfind_openin(flags, file_name, path) \
	osfind_open(flags | OSFind_Openin, file_name, path)

extern os_error *xosfind_openout (bits flags,
      char *file_name,
      char *path,
      os_f *file);
extern os_f osfind_openout (bits flags,
      char *file_name,
      char *path);

#define	xosfind_openout(flags, file_name, path, file) \
	xosfind_open(flags | OSFind_Openout, file_name, path, file)

#define	osfind_openout(flags, file_name, path) \
	osfind_open(flags | OSFind_Openout, file_name, path)

extern os_error *xosfind_openup (bits flags,
      char *file_name,
      char *path,
      os_f *file);
extern os_f osfind_openup (bits flags,
      char *file_name,
      char *path);

#define	xosfind_openup(flags, file_name, path, file) \
	xosfind_open(flags | OSFind_Openup, file_name, path, file)

#define	osfind_openup(flags, file_name, path) \
	osfind_open(flags | OSFind_Openup, file_name, path)

extern os_error *xosfind_close (os_f file);
extern void osfind_close (os_f file);

#define	xosfind_close(file) \
	(os_error*) _swix(OS_Find, _IN(0)|_IN(1), \
		OSFind_Close, \
		file)

#define	osfind_close(file) \
	(void) _swi(OS_Find, _IN(0)|_IN(1), \
		OSFind_Close, \
		file)

/* From oslib:osargs.h */
#undef  OS_Args
#define OS_Args                                 0x9
#undef  OSArgs_ReadPtr
#define OSArgs_ReadPtr                          0x0
#undef  OSArgs_SetPtr
#define OSArgs_SetPtr                           0x1
#undef  OSArgs_ReadExt
#define OSArgs_ReadExt                          0x2

extern os_error *xosargs_read_ptr (os_f file,
      int *ptr);
extern int osargs_read_ptr (os_f file);

#define	xosargs_read_ptr(file, ptr) \
	(os_error*) _swix(OS_Args, _IN(0)|_IN(1)|_OUT(2), \
		OSArgs_ReadPtr, \
		file, \
		ptr)

#define	osargs_read_ptr(file) \
	_swi(OS_Args, _IN(0)|_IN(1)|_RETURN(2), \
		OSArgs_ReadPtr, \
		file)

extern os_error *xosargs_set_ptr (os_f file,
      int ptr);
extern void osargs_set_ptr (os_f file,
      int ptr);

#define	xosargs_set_ptr(file, ptr) \
	(os_error*) _swix(OS_Args, _IN(0)|_IN(1)|_IN(2), \
		OSArgs_SetPtr, \
		file, \
		ptr)

#define	osargs_set_ptr(file, ptr) \
	(void) _swi(OS_Args, _IN(0)|_IN(1)|_IN(2), \
		OSArgs_SetPtr, \
		file, \
		ptr)

extern os_error *xosargs_read_ext (os_f file,
      int *ext);
extern int osargs_read_ext (os_f file);

#define	xosargs_read_ext(file, ext) \
	(os_error*) _swix(OS_Args, _IN(0)|_IN(1)|_OUT(2), \
		OSArgs_ReadExt, \
		file, \
		ext)

#define	osargs_read_ext(file) \
	_swi(OS_Args, _IN(0)|_IN(1)|_RETURN(2), \
		OSArgs_ReadExt, \
		file)

/* OSLIB EMULATION ENDS */

#endif

#ifndef __osfcn_h
/* Will be set or not during tiffcomp.h */
/* You get this to compile under C++? Please say how! */

extern int open(const char* name, int flags, int mode)
{
	/* From what I can tell, should return <0 for failure */
	os_error* e = (os_error*) 1; /* Cheeky way to use a pointer eh? :-) */
	os_f file = (os_f) -1;

	flags = flags;

	switch(mode)
	{
		case O_RDONLY:
		{
			e = xosfind_openin(SKIP, name, SKIP, &file);
			break;
		}
		case O_WRONLY:
		case O_RDWR|O_CREAT:
		case O_RDWR|O_CREAT|O_TRUNC:
		{
			e = xosfind_openout(SKIP, name, SKIP, &file);
			break;
		}
		case O_RDWR:
		{
			e = xosfind_openup(SKIP, name, SKIP, &file);
			break;
		}
	}
	if (e)
	{
		file = (os_f) -1;
	}
	return (file);
}

extern int close(int fd)
{
	return ((int) xosfind_close((os_f) fd));
}

extern int write(int fd, const char *buf, int nbytes)
{
	/* Returns number of bytes written */
	return (nbytes - osgbpb_write((os_f) fd, (const byte*) buf, nbytes));
}

extern int read(int fd, char *buf, int nbytes)
{
	/* Returns number of bytes read */
	return (nbytes - osgbpb_read((os_f) fd, (byte*) buf, nbytes));
}

extern off_t lseek(int fd, off_t offset, int whence)
{
	int absolute = 0;

	switch (whence)
	{
		case SEEK_SET:
		{
			absolute = (int) offset;
			break;
		}
		case SEEK_CUR:
		{
			absolute = osargs_read_ptr((os_f) fd) + (int) offset;
			break;
		}
		case SEEK_END:
		{
			absolute = osargs_read_ext((os_f) fd) + (int) offset;
			break;
		}
	}

	osargs_set_ptr((os_f) fd, absolute);

	return ((off_t) osargs_read_ptr((os_f) fd));
}
#endif

static tsize_t
_tiffReadProc(thandle_t fd, tdata_t buf, tsize_t size)
{
	return ((tsize_t) read((int) fd, buf, (size_t) size));
}

static tsize_t
_tiffWriteProc(thandle_t fd, tdata_t buf, tsize_t size)
{
	return ((tsize_t) write((int) fd, buf, (size_t) size));
}

static toff_t
_tiffSeekProc(thandle_t fd, toff_t off, int whence)
{
	return ((toff_t) lseek((int) fd, (off_t) off, whence));
}

static int
_tiffCloseProc(thandle_t fd)
{
	return (close((int) fd));
}

static toff_t
_tiffSizeProc(thandle_t fd)
{
	return (lseek((int) fd, SEEK_END, SEEK_SET));
}

#ifdef HAVE_MMAP
#error "I didn't know Acorn had that!"
#endif

/* !HAVE_MMAP */
static int
_tiffMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
	(void) fd; (void) pbase; (void) psize;
	return (0);
}

static void
_tiffUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
	(void) fd; (void) base; (void) size;
}

/*
 * Open a TIFF file descriptor for read/writing.
 */
TIFF*
TIFFFdOpen(int fd, const char* name, const char* mode)
{
	TIFF* tif;

	tif = TIFFClientOpen(name, mode,
		(thandle_t) fd,
		_tiffReadProc, _tiffWriteProc,
		_tiffSeekProc, _tiffCloseProc, _tiffSizeProc,
		_tiffMapProc, _tiffUnmapProc);
	if (tif)
	{
		tif->tif_fd = fd;
	}
	return (tif);
}

/*
 * Open a TIFF file for read/writing.
 */
TIFF*
TIFFOpen(const char* name, const char* mode)
{
	static const char module[] = "TIFFOpen";
	int m, fd;

	m = _TIFFgetMode(mode, module);

	if (m == -1)
	{
		return ((TIFF*) 0);
	}

	fd = open(name, 0, m);

	if (fd < 0)
	{
		TIFFError(module, "%s: Cannot open", name);
		return ((TIFF *)0);
	}
	return (TIFFFdOpen(fd, name, mode));
}

void*
_TIFFmalloc(tsize_t s)
{
	return (malloc((size_t) s));
}

void
_TIFFfree(tdata_t p)
{
	free(p);
}

void*
_TIFFrealloc(tdata_t p, tsize_t s)
{
	return (realloc(p, (size_t) s));
}

void
_TIFFmemset(tdata_t p, int v, tsize_t c)
{
	memset(p, v, (size_t) c);
}

void
_TIFFmemcpy(tdata_t d, const tdata_t s, tsize_t c)
{
	memcpy(d, s, (size_t) c);
}

int
_TIFFmemcmp(const tdata_t p1, const tdata_t p2, tsize_t c)
{
	return (memcmp(p1, p2, (size_t) c));
}

static void
acornWarningHandler(const char* module, const char* fmt, va_list ap)
{
	if (module != NULL)
	{
		fprintf(stderr, "%s: ", module);
	}
	fprintf(stderr, "Warning, ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
}
TIFFErrorHandler _TIFFwarningHandler = acornWarningHandler;

static void
acornErrorHandler(const char* module, const char* fmt, va_list ap)
{
	if (module != NULL)
	{
		fprintf(stderr, "%s: ", module);
	}
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
}
TIFFErrorHandler _TIFFerrorHandler = acornErrorHandler;
