/***
* errno.h - system wide error numbers (set by system calls)
*
*       Copyright (c) 1985-1997, Microsoft Corporation. All rights reserved.
*
* Purpose:
*       This file defines the system-wide error numbers (set by
*       system calls).  Conforms to the XENIX standard.  Extended
*       for compatibility with Uniforum standard.
*       [System V]
*
*       [Public]
*
****/

#if     _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_ERRNO
#define _INC_ERRNO

#if     !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif

#include <winsock.h>

#ifdef  __cplusplus
extern "C" {
#endif



/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif

/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if	_MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI1 __cdecl
#else
#define _CRTAPI1
#endif
#endif


/* declare reference to errno */

#if     (defined(_MT) || defined(_MD) || defined(_DLL)) && !defined(_MAC)
_CRTIMP extern int * __cdecl _errno(void);
#define errno   (*_errno())
#else   /* ndef _MT && ndef _MD && ndef _DLL */
_CRTIMP extern int errno;
#endif  /* _MT || _MD || _DLL */

/* Error Codes */

#define EPERM           1
#define ENOENT          2
#define ESRCH           3
#define EINTR           4
#define EIO             5
#define ENXIO           6
#define E2BIG           7
#define ENOEXEC         8
#define EBADF           9
#define ECHILD          10
#define EAGAIN          11
#define ENOMEM          12
#define EACCES          13
#define EFAULT          14
#define EBUSY           16
#define EEXIST          17
#define EXDEV           18
#define ENODEV          19
#define ENOTDIR         20
#define EISDIR          21
#define EINVAL          22
#define ENFILE          23
#define EMFILE          24
#define ENOTTY          25
#define EFBIG           27
#define ENOSPC          28
#define ESPIPE          29
#define EROFS           30
#define EMLINK          31
#define EPIPE           32
#define EDOM            33
#define ERANGE          34
#define EDEADLK         36

/* defined differently in winsock.h on WinCE */
#ifndef ENAMETOOLONG
#define ENAMETOOLONG    38
#endif

#define ENOLCK          39
#define ENOSYS          40

/* defined differently in winsock.h on WinCE */
#ifndef ENOTEMPTY
#define ENOTEMPTY       41
#endif

#define EILSEQ          42

/*
 * Support EDEADLOCK for compatibiity with older MS-C versions.
 */
#define EDEADLOCK       EDEADLK

#ifdef  __cplusplus
}
#endif

#endif  /* _INC_ERRNO */
