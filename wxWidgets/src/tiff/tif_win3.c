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
 * TIFF Library Windows 3.x-specific Routines.
 */
#include "tiffiop.h"
#if defined(__WATCOMC__) || defined(__BORLANDC__) || defined(_MSC_VER)
#include <io.h>		/* for open, close, etc. function prototypes */
#endif

#include <windows.h>
#include <windowsx.h>
#include <memory.h>

static tsize_t 
_tiffReadProc(thandle_t fd, tdata_t buf, tsize_t size)
{
	return (_hread(fd, buf, size));
}

static tsize_t
_tiffWriteProc(thandle_t fd, tdata_t buf, tsize_t size)
{
	return (_hwrite(fd, buf, size));
}

static toff_t
_tiffSeekProc(thandle_t fd, toff_t off, int whence)
{
	return (_llseek(fd, (off_t) off, whence));
}

static int
_tiffCloseProc(thandle_t fd)
{
	return (_lclose(fd));
}

#include <sys/stat.h>

static toff_t
_tiffSizeProc(thandle_t fd)
{
	struct stat sb;
	return (fstat((int) fd, &sb) < 0 ? 0 : sb.st_size);
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
	int m, fd;
	OFSTRUCT of;
	int mm = 0;

	m = _TIFFgetMode(mode, module);
	if (m == -1)
		return ((TIFF*)0);
	if (m & O_CREAT) {
		if ((m & O_TRUNC) || OpenFile(name, &of, OF_EXIST) != HFILE_ERROR)
			mm |= OF_CREATE;
	}
	if (m & O_WRONLY)
		mm |= OF_WRITE;
	if (m & O_RDWR)
		mm |= OF_READWRITE;
	fd = OpenFile(name, &of, mm);
	if (fd < 0) {
		TIFFError(module, "%s: Cannot open", name);
		return ((TIFF*)0);
	}
	return (TIFFFdOpen(fd, name, mode));
}

tdata_t
_TIFFmalloc(tsize_t s)
{
	return (tdata_t) GlobalAllocPtr(GHND, (DWORD) s);
}

void
_TIFFfree(tdata_t p)
{
	GlobalFreePtr(p);
}

tdata_t
_TIFFrealloc(tdata_t p, tsize_t s)
{
	return (tdata_t) GlobalReAllocPtr(p, (DWORD) s, GHND);
}

void
_TIFFmemset(tdata_t p, int v, tsize_t c)
{
	char* pp = (char*) p;

	while (c > 0) {
		tsize_t chunk = 0x10000 - ((uint32) pp & 0xffff);/* What's left in segment */
		if (chunk > 0xff00)				/* No more than 0xff00 */
			chunk = 0xff00;
		if (chunk > c)					/* No more than needed */
			chunk = c;
		memset(pp, v, chunk);
		pp = (char*) (chunk + (char huge*) pp);
		c -= chunk;
	}
}

void
_TIFFmemcpy(tdata_t d, const tdata_t s, tsize_t c)
{
	if (c > 0xFFFF)
		hmemcpy((void _huge*) d, (void _huge*) s, c);
	else
		(void) memcpy(d, s, (size_t) c);
}

int
_TIFFmemcmp(const tdata_t d, const tdata_t s, tsize_t c)
{
	char* dd = (char*) d;
	char* ss = (char*) s;
	tsize_t chunks, chunkd, chunk;
	int result;

	while (c > 0) {
		chunks = 0x10000 - ((uint32) ss & 0xffff);	/* What's left in segment */
		chunkd = 0x10000 - ((uint32) dd & 0xffff);	/* What's left in segment */
		chunk = c;					/* Get the largest of     */
		if (chunk > chunks)				/*   c, chunks, chunkd,   */
			chunk = chunks;				/*   0xff00               */
		if (chunk > chunkd)
			chunk = chunkd;
		if (chunk > 0xff00)
			chunk = 0xff00;
		result = memcmp(dd, ss, chunk);
		if (result != 0)
			return (result);
		dd = (char*) (chunk + (char huge*) dd);
		ss = (char*) (chunk + (char huge*) ss);
		c -= chunk;
	}
	return (0);
}

static void
win3WarningHandler(const char* module, const char* fmt, va_list ap)
{
	char e[512] = { '\0' };
	if (module != NULL)
		strcat(strcpy(e, module), ":");
	vsprintf(e+strlen(e), fmt, ap);
	strcat(e, ".");
	MessageBox(GetActiveWindow(), e, "LibTIFF Warning",
	    MB_OK|MB_ICONEXCLAMATION);
}
TIFFErrorHandler _TIFFwarningHandler = win3WarningHandler;

static void
win3ErrorHandler(const char* module, const char* fmt, va_list ap)
{
	char e[512] = { '\0' };
	if (module != NULL)
		strcat(strcpy(e, module), ":");
	vsprintf(e+strlen(e), fmt, ap);
	strcat(e, ".");
	MessageBox(GetActiveWindow(), e, "LibTIFF Error", MB_OK|MB_ICONSTOP);
}
TIFFErrorHandler _TIFFerrorHandler = win3ErrorHandler;
