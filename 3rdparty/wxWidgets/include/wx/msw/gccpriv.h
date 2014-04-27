/*
 Name:        wx/msw/gccpriv.h
 Purpose:     MinGW/Cygwin definitions
 Author:      Vadim Zeitlin
 Modified by:
 Created:
 RCS-ID:      $Id: gccpriv.h 36155 2005-11-10 16:16:05Z ABX $
 Copyright:   (c) Vadim Zeitlin
 Licence:     wxWindows Licence
*/

/* THIS IS A C FILE, DON'T USE C++ FEATURES (IN PARTICULAR COMMENTS) IN IT */

#ifndef _WX_MSW_GCCPRIV_H_
#define _WX_MSW_GCCPRIV_H_

#if defined(__MINGW32__) && !defined(__GNUWIN32__)
    #define __GNUWIN32__
#endif

#if defined(__MINGW32__) && ( ( __GNUC__ > 2 ) || ( ( __GNUC__ == 2 ) && ( __GNUC_MINOR__ >= 95 ) ) )
    #include <_mingw.h>
#endif

#if defined( __MINGW32__ ) && !defined(__WINE__) && !defined( HAVE_W32API_H )
    #if __MINGW32_MAJOR_VERSION >= 1
        #define HAVE_W32API_H
    #endif
#elif defined( __CYGWIN__ ) && !defined( HAVE_W32API_H )
    #if ( __GNUC__ > 2 )
        #define HAVE_W32API_H
    #endif
#endif

#if wxCHECK_WATCOM_VERSION(1,0)
    #define HAVE_W32API_H
#endif

/* check for MinGW/Cygwin w32api version ( releases >= 0.5, only ) */
#if defined( HAVE_W32API_H )
#include <w32api.h>
#endif

/* Watcom can't handle defined(xxx) here: */
#if defined(__W32API_MAJOR_VERSION) && defined(__W32API_MINOR_VERSION)
    #define wxCHECK_W32API_VERSION( major, minor ) \
 ( ( ( __W32API_MAJOR_VERSION > (major) ) \
      || ( __W32API_MAJOR_VERSION == (major) && __W32API_MINOR_VERSION >= (minor) ) ) )
#else
    #define wxCHECK_W32API_VERSION( major, minor ) (0)
#endif

/* Cygwin / Mingw32 with gcc >= 2.95 use new windows headers which
   are more ms-like (header author is Anders Norlander, hence the name) */
#if (defined(__MINGW32__) || defined(__CYGWIN__) || defined(__WINE__)) && ((__GNUC__>2) || ((__GNUC__==2) && (__GNUC_MINOR__>=95)))
    #ifndef wxUSE_NORLANDER_HEADERS
        #define wxUSE_NORLANDER_HEADERS 1
    #endif
#else
    #ifndef wxUSE_NORLANDER_HEADERS
        #define wxUSE_NORLANDER_HEADERS 0
    #endif
#endif

/* "old" GNUWIN32 is the one without Norlander's headers: it lacks the
   standard Win32 headers and we define the used stuff ourselves for it
   in wx/msw/gnuwin32/extra.h */
#if defined(__GNUC__) && !wxUSE_NORLANDER_HEADERS
    #define __GNUWIN32_OLD__
#endif

/* Cygwin 1.0 */
#if defined(__CYGWIN__) && ((__GNUC__==2) && (__GNUC_MINOR__==9))
    #define __CYGWIN10__
#endif

/* Check for Mingw runtime version: */
#if defined(__MINGW32_MAJOR_VERSION) && defined(__MINGW32_MINOR_VERSION)
    #define wxCHECK_MINGW32_VERSION( major, minor ) \
 ( ( ( __MINGW32_MAJOR_VERSION > (major) ) \
      || ( __MINGW32_MAJOR_VERSION == (major) && __MINGW32_MINOR_VERSION >= (minor) ) ) )
#else
    #define wxCHECK_MINGW32_VERSION( major, minor ) (0)
#endif

/* Mingw runtime 1.0-20010604 has some missing _tXXXX functions,
   so let's define them ourselves: */
#if defined(__GNUWIN32__) && wxCHECK_W32API_VERSION( 1, 0 ) \
    && !wxCHECK_W32API_VERSION( 1, 1 )
    #ifndef _tsetlocale
      #if wxUSE_UNICODE
      #define _tsetlocale _wsetlocale
      #else
      #define _tsetlocale setlocale
      #endif
    #endif
    #ifndef _tgetenv
      #if wxUSE_UNICODE
      #define _tgetenv _wgetenv
      #else
      #define _tgetenv getenv
      #endif
    #endif
    #ifndef _tfopen
      #if wxUSE_UNICODE
      #define _tfopen _wfopen
      #else
      #define _tfopen fopen
      #endif
    #endif
#endif

/* current (= before mingw-runtime 3.3) mingw32 headers forget to
   define _puttchar, this will probably be fixed in the next versions but
   for now do it ourselves
 */
#if defined( __MINGW32__ ) && \
        !wxCHECK_MINGW32_VERSION(3,3) && !defined( _puttchar )
    #ifdef wxUSE_UNICODE
        #define  _puttchar   putwchar
    #else
        #define  _puttchar   puttchar
    #endif
#endif

#endif
  /* _WX_MSW_GCCPRIV_H_ */
