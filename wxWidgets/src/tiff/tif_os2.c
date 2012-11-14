/* $Header$ */

/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * modifed for use with OS/2 by David Webster
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
 * TIFF Library OS/2-specific Routines.  Adapted from tif_win32.c 2/16/00 by
 * David Webster (dwebster@bhmi.com), Baldwin, Hackett, and Meeks, Inc., Omaha, NE USA
 */
#define INCL_PM
#define INCL_BASE
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include "tiffiop.h"

/* Some windows datatypes */

typedef ULONG DWORD;
typedef ULONG HANDLE;
typedef PCHAR LPTSTR;
typedef const PCHAR LPCTSTR;
typedef CHAR TCHAR;
#define GlobalAlloc(a,b) malloc(b)
#define LocalAlloc(a,b) malloc(b)
#define GlobalFree(b) free(b)
#define LocalFree(b) free(b)
#define GlobalReAlloc(p, s, t) realloc(p, s)
#define CopyMemory(p, v, s) memcpy(p, v, s)
#define FillMemory(p, c, s) memset(p, c, s)
#define GlobalSize(p) sizeof(p)
#define wsprintf sprintf
#define wvsprintf vsprintf
#define lstrlen strlen

static tsize_t LINKAGEMODE
_tiffReadProc(thandle_t fd, tdata_t buf, tsize_t size)
{
	DWORD dwSizeRead;
	if (!DosRead((HFILE)fd, buf, size, &dwSizeRead))
		return(0);
	return ((tsize_t) dwSizeRead);
}

static tsize_t LINKAGEMODE
_tiffWriteProc(thandle_t fd, tdata_t buf, tsize_t size)
{
	DWORD dwSizeWritten;
	if (!DosWrite((HFILE)fd, buf, size, &dwSizeWritten))
		return(0);
	return ((tsize_t) dwSizeWritten);
}

static toff_t LINKAGEMODE
_tiffSeekProc(thandle_t fd, toff_t off, int whence)
{
	DWORD dwMoveMethod;
    ULONG ibActual;
	switch(whence)
	{
	case 0:
		dwMoveMethod = FILE_BEGIN;
		break;
	case 1:
		dwMoveMethod = FILE_CURRENT;
		break;
	case 2:
		dwMoveMethod = FILE_END;
		break;
	default:
		dwMoveMethod = FILE_BEGIN;
		break;
	}
	DosSetFilePtr((HFILE)fd, off, dwMoveMethod, &ibActual);
 return((toff_t)ibActual);
}

static int LINKAGEMODE
_tiffCloseProc(thandle_t fd)
{
	return (DosClose((HFILE)fd) ? 0 : -1);
}

static toff_t LINKAGEMODE
_tiffSizeProc(thandle_t fd)
{
   FILESTATUS3    vStatus;

   DosQueryFileInfo((HFILE)fd, FIL_STANDARD, &vStatus, sizeof(FILESTATUS3));
   return (vStatus.cbFile);
}

static int LINKAGEMODE
_tiffDummyMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
	return (0);
}

/*
 * From "Hermann Josef Hill" <lhill@rhein-zeitung.de>:
 *
 * Windows uses both a handle and a pointer for file mapping,
 * but according to the SDK documentation and Richter's book
 * "Advanced Windows Programming" it is safe to free the handle
 * after obtaining the file mapping pointer
 *
 * This removes a nasty OS dependency and cures a problem
 * with Visual C++ 5.0
 */
static int LINKAGEMODE
_tiffMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
    return(0);
}

static void LINKAGEMODE
_tiffDummyUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
}

static void LINKAGEMODE
_tiffUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
}

/*
 * Open a TIFF file descriptor for read/writing.
 * Note that TIFFFdOpen and TIFFOpen recognise the character 'u' in the mode
 * string, which forces the file to be opened unmapped.
 */
TIFF*
TIFFFdOpen(int ifd, const char* name, const char* mode)
{
	TIFF* tif;
	BOOL fSuppressMap = (mode[1] == 'u' || mode[2] == 'u');

	tif = TIFFClientOpen(name, mode,
		 (thandle_t)ifd,
	    _tiffReadProc, _tiffWriteProc,
	    _tiffSeekProc, _tiffCloseProc, _tiffSizeProc,
		 fSuppressMap ? _tiffDummyMapProc : _tiffMapProc,
		 fSuppressMap ? _tiffDummyUnmapProc : _tiffUnmapProc);
	if (tif)
		tif->tif_fd = ifd;
	return (tif);
}

/*
 * Open a TIFF file for read/writing.
 */
TIFF*
TIFFOpen(const char* name, const char* mode)
{
	static const char module[] = "TIFFOpen";
	thandle_t fd;
	int m;
	DWORD dwOpenMode;
 DWORD dwOpenFlags;
 DWORD dwAction;
 APIRET ulrc;

	m = _TIFFgetMode(mode, module);

	switch(m)
	{
	case O_RDONLY:
		dwOpenMode = OPEN_ACCESS_READONLY;
		dwOpenFlags = OPEN_ACTION_FAIL_IF_NEW;
		break;
	case O_RDWR:
		dwOpenMode = OPEN_ACCESS_READWRITE;
		dwOpenFlags = OPEN_ACTION_FAIL_IF_NEW;
		break;
	case O_RDWR|O_CREAT:
		dwOpenMode = OPEN_ACCESS_READWRITE;
		dwOpenFlags = OPEN_ACTION_CREATE_IF_NEW;
		break;
	case O_RDWR|O_TRUNC:
		dwOpenMode = OPEN_ACCESS_READWRITE;
		dwOpenFlags = OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS;
		break;
	case O_RDWR|O_CREAT|O_TRUNC:
		dwOpenMode = OPEN_ACCESS_READWRITE;
		dwOpenFlags = OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_REPLACE_IF_EXISTS;
		break;
	default:
		return ((TIFF*)0);
	}
 ulrc = DosOpen( name, (HFILE*)&fd, &dwAction, FILE_ARCHIVED|FILE_NORMAL
                ,1000L, dwOpenFlags, dwOpenMode, NULL
               );
	if (fd != NO_ERROR) {
		TIFFError(module, "%s: Cannot open", name);
		return ((TIFF *)0);
	}
	return (TIFFFdOpen((int)fd, name, mode));
}

tdata_t
_TIFFmalloc(tsize_t s)
{
	return ((tdata_t)GlobalAlloc(GMEM_FIXED, s));
}

void
_TIFFfree(tdata_t p)
{
	GlobalFree(p);
	return;
}

tdata_t
_TIFFrealloc(tdata_t p, tsize_t s)
{
	void* pvTmp;
	if ((pvTmp = GlobalReAlloc(p, s, 0)) == NULL) {
		if ((pvTmp = GlobalAlloc(sGMEM_FIXED, s)) != NULL) {
			CopyMemory(pvTmp, p, GlobalSize(p));
			GlobalFree(p);
		}
	}
	return ((tdata_t)pvTmp);
}

void
_TIFFmemset(void* p, int v, tsize_t c)
{
	FillMemory(p, c, (BYTE)v);
}

void
_TIFFmemcpy(void* d, const tdata_t s, tsize_t c)
{
	CopyMemory(d, s, c);
}

int
_TIFFmemcmp(const tdata_t p1, const tdata_t p2, tsize_t c)
{
	register const *pb1 = (const int*)p1;
	register const *pb2 = (const int*)p2;
	register DWORD dwTmp = c;
	register int iTmp;
	for (iTmp = 0; dwTmp-- && !iTmp; iTmp = (int)*pb1++ - (int)*pb2++)
		;
	return (iTmp);
}

static void LINKAGEMODE
Os2WarningHandler(const char* module, const char* fmt, va_list ap)
{
#ifndef TIF_PLATFORM_CONSOLE
	LPTSTR szTitle;
	LPTSTR szTmp;
	LPCTSTR szTitleText = "%s Warning";
	LPCTSTR szDefaultModule = "TIFFLIB";
	szTmp = (module == NULL) ? (LPTSTR)szDefaultModule : (LPTSTR)module;
	if ((szTitle = (LPTSTR)LocalAlloc(LMEM_FIXED, (lstrlen(szTmp) +
			lstrlen(szTitleText) + lstrlen(fmt) + 128)*sizeof(TCHAR))) == NULL)
		return;
	wsprintf(szTitle, szTitleText, szTmp);
	szTmp = szTitle + (lstrlen(szTitle)+2)*sizeof(TCHAR);
	wvsprintf(szTmp, fmt, ap);
	WinMessageBox( HWND_DESKTOP
               ,WinQueryFocus(HWND_DESKTOP)
               ,szTmp
               ,szTitle
               ,0
               ,MB_OK | MB_INFORMATION
              );
	LocalFree(szTitle);
	return;
#else
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	fprintf(stderr, "Warning, ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
#endif
}
TIFFErrorHandler _TIFFwarningHandler = Os2WarningHandler;

static void LINKAGEMODE
Os2ErrorHandler(const char* module, const char* fmt, va_list ap)
{
#ifndef TIF_PLATFORM_CONSOLE
	LPTSTR szTitle;
	LPTSTR szTmp;
	LPCTSTR szTitleText = "%s Error";
	LPCTSTR szDefaultModule = "TIFFLIB";
	szTmp = (module == NULL) ? (LPTSTR)szDefaultModule : (LPTSTR)module;
	if ((szTitle = (LPTSTR)LocalAlloc(LMEM_FIXED, (lstrlen(szTmp) +
			lstrlen(szTitleText) + lstrlen(fmt) + 128)*sizeof(TCHAR))) == NULL)
		return;
	wsprintf(szTitle, szTitleText, szTmp);
	szTmp = szTitle + (lstrlen(szTitle)+2)*sizeof(TCHAR);
	wvsprintf(szTmp, fmt, ap);
	WinMessageBox( HWND_DESKTOP
               ,WinQueryFocus(HWND_DESKTOP)
               ,szTmp
               ,szTitle
               ,0
               ,MB_OK | MB_ICONEXCLAMATION
              );
	LocalFree(szTitle);
	return;
#else
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
#endif
}
TIFFErrorHandler _TIFFerrorHandler = Os2ErrorHandler;

