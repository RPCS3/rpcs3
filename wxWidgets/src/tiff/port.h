/*
 * Warning, this file was automatically created by the TIFF configure script
 *   Actually, it has been modified to use wxWidgets' values.
 * VERSION:	 v3.5.2
 * DATE:	 Son Nov 28 15:15:07 GMT 1999
 * TARGET:	 
 * CCOMPILER:	 
 */
#ifndef _PORT_
#define _PORT_ 1

#ifdef __cplusplus
extern "C" {
#endif

#include "wx/setup.h"

#include <sys/types.h>
#define HOST_FILLORDER FILLORDER_LSB2MSB

/* wxWidgets defines WORDS_BIGENDIAN */
#ifdef WORDS_BIGENDIAN
#define HOST_BIGENDIAN	1
#else
#define HOST_BIGENDIAN	0
#endif

/* wxWidgets defines its own reading and writing routines */
/* #define HAVE_MMAP 1 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

typedef double dblparam_t;

#ifdef __STRICT_ANSI__
#define	INLINE	__inline__
#else
#define	INLINE	inline
#endif

#define GLOBALDATA(TYPE,NAME)	extern TYPE NAME

/* wxWidgets defines wxUSE_LIBJPEG */
#if wxUSE_LIBJPEG
#define JPEG_SUPPORT
#endif

/* wxWidgets defines wxUSE_ZLIB */
#if wxUSE_ZLIB
#define ZIP_SUPPORT
#endif

#ifdef __cplusplus
}
#endif

#endif
