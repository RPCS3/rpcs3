/* This test added by JACS as a quick fix. What should we do
 * to make it work with configure?
 */

#if defined(_MSC_VER) || defined(__BORLANDC__) || defined (__DMC__)
#include "jconfig.vc"
#else

/* jconfig.h.  Generated automatically by configure.  */
/* jconfig.cfg --- source file edited by configure script */
/* see jconfig.doc for explanations */

/* If using MetroWerks on Mac define __WXMAC__ if it isn't already
   FIXME: Is this necessary any longer? */
#ifdef __MWERKS__
#if (__MWERKS__ < 0x0900) || macintosh || defined ( __MACH__ )
#   ifndef __WXMAC__
#       define __WXMAC__
#   endif
#endif
#endif

/* use wxWidgets' configure */
#include "wx/setup.h"

/* If using Metrowerks and not using configure-generated setup */
#if defined(__MWERKS__) && !defined(__WX_SETUP_H__)
#if (__MWERKS__ < 0x0900) || macintosh || defined ( __MACH__ )

#   define USE_MAC_MEMMGR

#   ifdef __MACH__
#       include <ansi_prefix.mach.h>
#       include <msl_c_version.h>
#       include <stdint.h>
#       undef WCHAR_MAX
#       include <machine/ansi.h>
#   endif

/* automatically includes MacHeaders */
#elif (__MWERKS__ >= 0x0900) && __INTEL__
    #define __WXMSW__
#endif
#endif

#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
#undef void
#undef const

/* use wxWidgets' configure */
/* #undef CHAR_IS_UNSIGNED */
#ifdef __CHAR_UNSIGNED__
  #ifndef CHAR_IS_UNSIGNED
    #define CHAR_IS_UNSIGNED
  #endif
#else
  #undef CHAR_IS_UNSIGNED
#endif

#define HAVE_STDDEF_H
#define HAVE_STDLIB_H
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS
#undef NEED_SHORT_EXTERNAL_NAMES
/* Define this if you get warnings about undefined structures. */
#undef INCOMPLETE_TYPES_BROKEN

#ifdef JPEG_INTERNALS

#undef RIGHT_SHIFT_IS_UNSIGNED

/* use wxWidgets' configure */
/* #define INLINE __inline__ */
#if defined(__VISAGECPP__) && (__IBMCPP__ >= 400 || __IBMC__ >= 400)
#define INLINE
#elif defined(__WATCOMC__)
#define INLINE
#else
#define INLINE inline
#endif

/* These are for configuring the JPEG memory manager. */
#undef DEFAULT_MAX_MEM
#undef NO_MKTEMP

#endif /* JPEG_INTERNALS */

#ifdef JPEG_CJPEG_DJPEG

#define BMP_SUPPORTED		/* BMP image file format */
#define GIF_SUPPORTED		/* GIF image file format */
#define PPM_SUPPORTED		/* PBMPLUS PPM/PGM image file format */
#undef RLE_SUPPORTED		/* Utah RLE image file format */
#define TARGA_SUPPORTED		/* Targa image file format */

#undef TWO_FILE_COMMANDLINE
#undef NEED_SIGNAL_CATCHER
#undef DONT_USE_B_MODE

/* Define this if you want percent-done progress reports from cjpeg/djpeg. */
#undef PROGRESS_REPORT

#endif /* JPEG_CJPEG_DJPEG */
#endif
    /* _MSC_VER */

