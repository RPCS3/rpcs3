/* "$Header$" */

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
 * TIFF Library ATARI-specific Routines.
 */
#include "tiffiop.h"
#if defined(__TURBOC__)
#include <tos.h>
#include <stdio.h>
#else
#include <osbind.h>
#include <fcntl.h>
#endif

#ifndef O_ACCMODE
#define O_ACCMODE 3
#endif

#include <errno.h>

#define AEFILNF   -33

static tsize_t
_tiffReadProc(thandle_t fd, tdata_t buf, tsize_t size)
{
	long r;

	r = Fread((int) fd, size, buf);
	if (r < 0) {
		errno = (int)-r;
		r = -1;
	}
	return r;
}

static tsize_t
_tiffWriteProc(thandle_t fd, tdata_t buf, tsize_t size)
{
	long r;

	r = Fwrite((int) fd, size, buf);
	if (r < 0) {
		errno = (int)-r;
		r = -1;
	}
	return r;
}

static toff_t
_tiffSeekProc(thandle_t fd, off_t off, int whence)
{
	char buf[256];
	long current_off, expected_off, new_off;

	if (whence == SEEK_END || off <= 0)
		return Fseek(off, (int) fd, whence);
	current_off = Fseek(0, (int) fd, SEEK_CUR); /* find out where we are */
	if (whence == SEEK_SET)
		expected_off = off;
	else
		expected_off = off + current_off;
	new_off = Fseek(off, (int) fd, whence);
	if (new_off == expected_off)
		return new_off;
	/* otherwise extend file -- zero filling the hole */
	if (new_off < 0)            /* error? */
		new_off = Fseek(0, (int) fd, SEEK_END); /* go to eof */
	_TIFFmemset(buf, 0, sizeof(buf));
	while (expected_off > new_off) {
		off = expected_off - new_off;
		if (off > sizeof(buf))
			off = sizeof(buf);
		if ((current_off = Fwrite((int) fd, off, buf)) != off)
			return (current_off > 0) ?
			    new_off + current_off : new_off;
		new_off += off;
	}
	return new_off;
}

static int
_tiffCloseProc(thandle_t fd)
{
	long r;

	r = Fclose((int) fd);
	if (r < 0) {
		errno = (int)-r;
		r = -1;
	}
	return (int)r;
}

static toff_t
_tiffSizeProc(thandle_t fd)
{
	long pos, eof;

	pos = Fseek(0, (int) fd, SEEK_CUR);
	eof = Fseek(0, (int) fd, SEEK_END);
	Fseek(pos, (int) fd, SEEK_SET);
	return eof;
}

static int
_tiffMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
	return (0);
}

static void
_tiffUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
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
		_tiffReadProc, _tiffWriteProc, _tiffSeekProc, _tiffCloseProc,
		_tiffSizeProc, _tiffMapProc, _tiffUnmapProc);
	if (tif)
		tif->tif_fd = fd;
	return (tif);
}

/*
* Open a TIFF file for read/writing.
*/
TIFF*
TIFFOpen(const char* name, const char* mode)
{
	static const char module[] = "TIFFOpen";
	int m;
	long fd;

	m = _TIFFgetMode(mode, module);
	if (m == -1)
		return ((TIFF*)0);
	if (m & O_TRUNC) {
		fd = Fcreate(name, 0);
	} else {
		fd = Fopen(name, m & O_ACCMODE);
		if (fd == AEFILNF && m & O_CREAT)
			fd = Fcreate(name, 0);
	}
	if (fd < 0)
		errno = (int)fd;
	if (fd < 0) {
		TIFFError(module, "%s: Cannot open", name);
		return ((TIFF*)0);
	}
	return (TIFFFdOpen(fd, name, mode));
}

#include <stdlib.h>

tdata_t
_TIFFmalloc(tsize_t s)
{
	return (malloc((size_t) s));
}

void
_TIFFfree(tdata_t p)
{
	free(p);
}

tdata_t
_TIFFrealloc(tdata_t p, tsize_t s)
{
	return (realloc(p, (size_t) s));
}

void
_TIFFmemset(tdata_t p, int v, size_t c)
{
	memset(p, v, (size_t) c);
}

void
_TIFFmemcpy(tdata_t d, const tdata_t s, size_t c)
{
	memcpy(d, s, (size_t) c);
}

int
_TIFFmemcmp(const tdata_t p1, const tdata_t p2, tsize_t c)
{
	return (memcmp(p1, p2, (size_t) c));
}

static void
atariWarningHandler(const char* module, const char* fmt, va_list ap)
{
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	fprintf(stderr, "Warning, ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
}
TIFFErrorHandler _TIFFwarningHandler = atariWarningHandler;

static void
atariErrorHandler(const char* module, const char* fmt, va_list ap)
{
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
}
TIFFErrorHandler _TIFFerrorHandler = atariErrorHandler;
