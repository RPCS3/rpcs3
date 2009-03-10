/*
 * Name:        wx/dlimpexp.h
 * Purpose:     Macros for declaring DLL-imported/exported functions
 * Author:      Vadim Zeitlin
 * Modified by:
 * Created:     16.10.2003 (extracted from wx/defs.h)
 * RCS-ID:      $Id: dlimpexp.h 49563 2007-10-31 20:46:21Z VZ $
 * Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
 * Licence:     wxWindows licence
 */

/*
    This is a C file, not C++ one, do not use C++ comments here!
 */

#ifndef _WX_DLIMPEXP_H_
#define _WX_DLIMPEXP_H_

#if defined(__WXMSW__)
    /*
       __declspec works in BC++ 5 and later, Watcom C++ 11.0 and later as well
       as VC++ and gcc
     */
#    if defined(__VISUALC__) || defined(__BORLANDC__) || defined(__GNUC__) || defined(__WATCOMC__)
#        define WXEXPORT __declspec(dllexport)
#        define WXIMPORT __declspec(dllimport)
#    else /* compiler doesn't support __declspec() */
#        define WXEXPORT
#        define WXIMPORT
#    endif
#elif defined(__WXPM__)
#    if defined (__WATCOMC__)
#        define WXEXPORT __declspec(dllexport)
        /*
           __declspec(dllimport) prepends __imp to imported symbols. We do NOT
           want that!
         */
#        define WXIMPORT
#    elif defined(__EMX__)
#        define WXEXPORT
#        define WXIMPORT
#    elif (!(defined(__VISAGECPP__) && (__IBMCPP__ < 400 || __IBMC__ < 400 )))
#        define WXEXPORT _Export
#        define WXIMPORT _Export
#    endif
#elif defined(__WXMAC__) || defined(__WXCOCOA__)
#    ifdef __MWERKS__
#        define WXEXPORT __declspec(export)
#        define WXIMPORT __declspec(import)
#    endif
#elif defined(__CYGWIN__)
#    define WXEXPORT __declspec(dllexport)
#    define WXIMPORT __declspec(dllimport)
#endif

/* for other platforms/compilers we don't anything */
#ifndef WXEXPORT
#    define WXEXPORT
#    define WXIMPORT
#endif

/*
   We support building wxWidgets as a set of several libraries but we don't
   support arbitrary combinations of libs/DLLs: either we build all of them as
   DLLs (in which case WXMAKINGDLL is defined) or none (it isn't).

   However we have a problem because we need separate WXDLLEXPORT versions for
   different libraries as, for example, wxString class should be dllexported
   when compiled in wxBase and dllimported otherwise, so we do define separate
   WXMAKING/USINGDLL_XYZ constants for each component XYZ.
 */
#ifdef WXMAKINGDLL
#    if wxUSE_BASE
#        define WXMAKINGDLL_BASE
#    endif

#    define WXMAKINGDLL_NET
#    define WXMAKINGDLL_CORE
#    define WXMAKINGDLL_ADV
#    define WXMAKINGDLL_QA
#    define WXMAKINGDLL_ODBC
#    define WXMAKINGDLL_DBGRID
#    define WXMAKINGDLL_HTML
#    define WXMAKINGDLL_GL
#    define WXMAKINGDLL_XML
#    define WXMAKINGDLL_XRC
#    define WXMAKINGDLL_AUI
#    define WXMAKINGDLL_RICHTEXT
#    define WXMAKINGDLL_MEDIA
#endif /* WXMAKINGDLL */

/*
   WXDLLEXPORT maps to export declaration when building the DLL, to import
   declaration if using it or to nothing at all if we don't use wxWin as DLL
 */
#ifdef WXMAKINGDLL_BASE
#    define WXDLLIMPEXP_BASE WXEXPORT
#    define WXDLLIMPEXP_DATA_BASE(type) WXEXPORT type
#elif defined(WXUSINGDLL)
#    define WXDLLIMPEXP_BASE WXIMPORT
#    define WXDLLIMPEXP_DATA_BASE(type) WXIMPORT type
#else /* not making nor using DLL */
#    define WXDLLIMPEXP_BASE
#    define WXDLLIMPEXP_DATA_BASE(type) type
#endif

#ifdef WXMAKINGDLL_NET
#    define WXDLLIMPEXP_NET WXEXPORT
#    define WXDLLIMPEXP_DATA_NET(type) WXEXPORT type
#elif defined(WXUSINGDLL)
#    define WXDLLIMPEXP_NET WXIMPORT
#    define WXDLLIMPEXP_DATA_NET(type) WXIMPORT type
#else /* not making nor using DLL */
#    define WXDLLIMPEXP_NET
#    define WXDLLIMPEXP_DATA_NET(type) type
#endif

#ifdef WXMAKINGDLL_CORE
#    define WXDLLIMPEXP_CORE WXEXPORT
#    define WXDLLIMPEXP_DATA_CORE(type) WXEXPORT type
#elif defined(WXUSINGDLL)
#    define WXDLLIMPEXP_CORE WXIMPORT
#    define WXDLLIMPEXP_DATA_CORE(type) WXIMPORT type
#else /* not making nor using DLL */
#    define WXDLLIMPEXP_CORE
#    define WXDLLIMPEXP_DATA_CORE(type) type
#endif

#ifdef WXMAKINGDLL_ADV
#    define WXDLLIMPEXP_ADV WXEXPORT
#    define WXDLLIMPEXP_DATA_ADV(type) WXEXPORT type
#elif defined(WXUSINGDLL)
#    define WXDLLIMPEXP_ADV WXIMPORT
#    define WXDLLIMPEXP_DATA_ADV(type) WXIMPORT type
#else /* not making nor using DLL */
#    define WXDLLIMPEXP_ADV
#    define WXDLLIMPEXP_DATA_ADV(type) type
#endif

#ifdef WXMAKINGDLL_ODBC
#    define WXDLLIMPEXP_ODBC WXEXPORT
#    define WXDLLIMPEXP_DATA_ODBC(type) WXEXPORT type
#elif defined(WXUSINGDLL)
#    define WXDLLIMPEXP_ODBC WXIMPORT
#    define WXDLLIMPEXP_DATA_ODBC(type) WXIMPORT type
#else /* not making nor using DLL */
#    define WXDLLIMPEXP_ODBC
#    define WXDLLIMPEXP_DATA_ODBC(type) type
#endif

#ifdef WXMAKINGDLL_QA
#    define WXDLLIMPEXP_QA WXEXPORT
#    define WXDLLIMPEXP_DATA_QA(type) WXEXPORT type
#elif defined(WXUSINGDLL)
#    define WXDLLIMPEXP_QA WXIMPORT
#    define WXDLLIMPEXP_DATA_QA(type) WXIMPORT type
#else /* not making nor using DLL */
#    define WXDLLIMPEXP_QA
#    define WXDLLIMPEXP_DATA_QA(type) type
#endif

#ifdef WXMAKINGDLL_DBGRID
#    define WXDLLIMPEXP_DBGRID WXEXPORT
#    define WXDLLIMPEXP_DATA_DBGRID(type) WXEXPORT type
#elif defined(WXUSINGDLL)
#    define WXDLLIMPEXP_DBGRID WXIMPORT
#    define WXDLLIMPEXP_DATA_DBGRID(type) WXIMPORT type
#else /* not making nor using DLL */
#    define WXDLLIMPEXP_DBGRID
#    define WXDLLIMPEXP_DATA_DBGRID(type) type
#endif

#ifdef WXMAKINGDLL_HTML
#    define WXDLLIMPEXP_HTML WXEXPORT
#    define WXDLLIMPEXP_DATA_HTML(type) WXEXPORT type
#elif defined(WXUSINGDLL)
#    define WXDLLIMPEXP_HTML WXIMPORT
#    define WXDLLIMPEXP_DATA_HTML(type) WXIMPORT type
#else /* not making nor using DLL */
#    define WXDLLIMPEXP_HTML
#    define WXDLLIMPEXP_DATA_HTML(type) type
#endif

#ifdef WXMAKINGDLL_GL
#    define WXDLLIMPEXP_GL WXEXPORT
#elif defined(WXUSINGDLL)
#    define WXDLLIMPEXP_GL WXIMPORT
#else /* not making nor using DLL */
#    define WXDLLIMPEXP_GL
#endif

#ifdef WXMAKINGDLL_XML
#    define WXDLLIMPEXP_XML WXEXPORT
#elif defined(WXUSINGDLL)
#    define WXDLLIMPEXP_XML WXIMPORT
#else /* not making nor using DLL */
#    define WXDLLIMPEXP_XML
#endif

#ifdef WXMAKINGDLL_XRC
#    define WXDLLIMPEXP_XRC WXEXPORT
#elif defined(WXUSINGDLL)
#    define WXDLLIMPEXP_XRC WXIMPORT
#else /* not making nor using DLL */
#    define WXDLLIMPEXP_XRC
#endif

#ifdef WXMAKINGDLL_AUI
#    define WXDLLIMPEXP_AUI WXEXPORT
#elif defined(WXUSINGDLL)
#    define WXDLLIMPEXP_AUI WXIMPORT
#else /* not making nor using DLL */
#    define WXDLLIMPEXP_AUI
#endif

#ifdef WXMAKINGDLL_RICHTEXT
#    define WXDLLIMPEXP_RICHTEXT WXEXPORT
#elif defined(WXUSINGDLL)
#    define WXDLLIMPEXP_RICHTEXT WXIMPORT
#else /* not making nor using DLL */
#    define WXDLLIMPEXP_RICHTEXT
#endif

#ifdef WXMAKINGDLL_MEDIA
#    define WXDLLIMPEXP_MEDIA WXEXPORT
#elif defined(WXUSINGDLL)
#    define WXDLLIMPEXP_MEDIA WXIMPORT
#else /* not making nor using DLL */
#    define WXDLLIMPEXP_MEDIA
#endif

/* for backwards compatibility, define suffix-less versions too */
#define WXDLLEXPORT WXDLLIMPEXP_CORE
#define WXDLLEXPORT_DATA WXDLLIMPEXP_DATA_CORE

/* wx-2.9 introduces new macros for forward declarations, include them
 * here for forward compatibility:
 
   GCC warns about using __attribute__ (and also __declspec in mingw32 case) on
   forward declarations while MSVC complains about forward declarations without
   __declspec for the classes later declared with it, so we need a separate set
   of macros for forward declarations to hide this difference:
 */
#if defined(__WINDOWS__) && defined(__GNUC__)
    #define WXDLLIMPEXP_FWD_BASE
    #define WXDLLIMPEXP_FWD_NET
    #define WXDLLIMPEXP_FWD_CORE
    #define WXDLLIMPEXP_FWD_ADV
    #define WXDLLIMPEXP_FWD_QA
    #define WXDLLIMPEXP_FWD_HTML
    #define WXDLLIMPEXP_FWD_GL
    #define WXDLLIMPEXP_FWD_XML
    #define WXDLLIMPEXP_FWD_XRC
    #define WXDLLIMPEXP_FWD_AUI
    #define WXDLLIMPEXP_FWD_RICHTEXT
    #define WXDLLIMPEXP_FWD_MEDIA
    #define WXDLLIMPEXP_FWD_STC
#else
    #define WXDLLIMPEXP_FWD_BASE      WXDLLIMPEXP_BASE
    #define WXDLLIMPEXP_FWD_NET       WXDLLIMPEXP_NET
    #define WXDLLIMPEXP_FWD_CORE      WXDLLIMPEXP_CORE
    #define WXDLLIMPEXP_FWD_ADV       WXDLLIMPEXP_ADV
    #define WXDLLIMPEXP_FWD_QA        WXDLLIMPEXP_QA
    #define WXDLLIMPEXP_FWD_HTML      WXDLLIMPEXP_HTML
    #define WXDLLIMPEXP_FWD_GL        WXDLLIMPEXP_GL
    #define WXDLLIMPEXP_FWD_XML       WXDLLIMPEXP_XML
    #define WXDLLIMPEXP_FWD_XRC       WXDLLIMPEXP_XRC
    #define WXDLLIMPEXP_FWD_AUI       WXDLLIMPEXP_AUI
    #define WXDLLIMPEXP_FWD_RICHTEXT  WXDLLIMPEXP_RICHTEXT
    #define WXDLLIMPEXP_FWD_MEDIA     WXDLLIMPEXP_MEDIA
    #define WXDLLIMPEXP_FWD_STC       WXDLLIMPEXP_STC
#endif

#endif /* _WX_DLIMPEXP_H_ */

