/*
 *  Name:        wx/defs.h
 *  Purpose:     Declarations/definitions common to all wx source files
 *  Author:      Julian Smart and others
 *  Modified by: Ryan Norton (Converted to C)
 *  Created:     01/02/97
 *  RCS-ID:      $Id: defs.h 66923 2011-02-16 22:37:48Z JS $
 *  Copyright:   (c) Julian Smart
 *  Licence:     wxWindows licence
 */

/* THIS IS A C FILE, DON'T USE C++ FEATURES (IN PARTICULAR COMMENTS) IN IT */

#ifndef _WX_DEFS_H_
#define _WX_DEFS_H_

/*  ---------------------------------------------------------------------------- */
/*  compiler and OS identification */
/*  ---------------------------------------------------------------------------- */

#include "wx/platform.h"

#ifdef __cplusplus
/*  Make sure the environment is set correctly */
#   if defined(__WXMSW__) && defined(__X__)
#       error "Target can't be both X and Windows"
#   elif defined(__WXMSW__) && defined(__PALMOS__)
#       error "Target can't be both PalmOS and Windows"
#   elif !defined(__WXMOTIF__) && \
         !defined(__WXMSW__)   && \
         !defined(__WXPALMOS__)&& \
         !defined(__WXGTK__)   && \
         !defined(__WXPM__)    && \
         !defined(__WXMAC__)   && \
         !defined(__WXCOCOA__) && \
         !defined(__X__)       && \
         !defined(__WXMGL__)   && \
         !defined(__WXDFB__)   && \
         !defined(__WXX11__)   && \
          wxUSE_GUI
#       ifdef __UNIX__
#           error "No Target! You should use wx-config program for compilation flags!"
#       else /*  !Unix */
#           error "No Target! You should use supplied makefiles for compilation!"
#       endif /*  Unix/!Unix */
#   endif
#endif /*__cplusplus*/

#ifndef __WXWINDOWS__
    #define __WXWINDOWS__ 1
#endif

#ifndef wxUSE_BASE
    /*  by default consider that this is a monolithic build */
    #define wxUSE_BASE 1
#endif

#if !wxUSE_GUI && !defined(__WXBASE__)
    #define __WXBASE__
#endif

/*  include the feature test macros */
#include "wx/features.h"

/*  suppress some Visual C++ warnings */
#ifdef __VISUALC__
    /*  the only "real" warning here is 4244 but there are just too many of them */
    /*  in our code... one day someone should go and fix them but until then... */
#   pragma warning(disable:4097)    /*  typedef used as class */
#   pragma warning(disable:4201)    /*  nonstandard extension used: nameless struct/union */
#   pragma warning(disable:4244)    /*  conversion from double to float */
#   pragma warning(disable:4355)    /* 'this' used in base member initializer list */
#   pragma warning(disable:4511)    /*  copy ctor couldn't be generated */
#   pragma warning(disable:4512)    /*  operator=() couldn't be generated */
#   pragma warning(disable:4710)    /*  function not inlined */

    /* For VC++ 5.0 for release mode, the warning 'C4702: unreachable code */
    /* is buggy, and occurs for code that does actually get executed */
#   if !defined __WXDEBUG__ && __VISUALC__ <= 1100
#       pragma warning(disable:4702)    /* unreachable code */
#   endif
    /* The VC++ 5.0 warning 'C4003: not enough actual parameters for macro'
     * is incompatible with the wxWidgets headers since it is given when
     * parameters are empty but not missing. */
#   if __VISUALC__ <= 1100
#       pragma warning(disable:4003)    /* not enough actual parameters for macro */
#   endif

    /*
       VC++ 8 gives a warning when using standard functions such as sprintf,
       localtime, ... -- stop this madness, unless the user had already done it
     */
    #if __VISUALC__ >= 1400
        #ifndef _CRT_SECURE_NO_DEPRECATE
            #define _CRT_SECURE_NO_DEPRECATE 1
        #endif
        #ifndef _CRT_NON_CONFORMING_SWPRINTFS
            #define _CRT_NON_CONFORMING_SWPRINTFS 1
        #endif
    #endif /* VC++ 8 */
#endif /*  __VISUALC__ */

/*  suppress some Salford C++ warnings */
#ifdef __SALFORDC__
#   pragma suppress 353             /*  Possible nested comments */
#   pragma suppress 593             /*  Define not used */
#   pragma suppress 61              /*  enum has no name (doesn't suppress!) */
#   pragma suppress 106             /*  unnamed, unused parameter */
#   pragma suppress 571             /*  Virtual function hiding */
#endif /*  __SALFORDC__ */

/*  suppress some Borland C++ warnings */
#ifdef __BORLANDC__
#   pragma warn -inl                /*  Functions containing reserved words and certain constructs are not expanded inline */
#endif /*  __BORLANDC__ */

/*
   g++ gives a warning when a class has private dtor if it has no friends but
   this is a perfectly valid situation for a ref-counted class which destroys
   itself when its ref count drops to 0, so provide a macro to suppress this
   warning
 */
#ifdef __GNUG__
#   define wxSUPPRESS_GCC_PRIVATE_DTOR_WARNING(name) \
        friend class wxDummyFriendFor ## name;
#else /* !g++ */
#   define wxSUPPRESS_GCC_PRIVATE_DTOR_WARNING(name)
#endif

/*  ---------------------------------------------------------------------------- */
/*  wxWidgets version and compatibility defines */
/*  ---------------------------------------------------------------------------- */

#include "wx/version.h"

/*  ============================================================================ */
/*  non portable C++ features */
/*  ============================================================================ */

/*  ---------------------------------------------------------------------------- */
/*  compiler defects workarounds */
/*  ---------------------------------------------------------------------------- */

/*
   Digital Unix C++ compiler only defines this symbol for .cxx and .hxx files,
   so define it ourselves (newer versions do it for all files, though, and
   don't allow it to be redefined)
 */
#if defined(__DECCXX) && !defined(__VMS) && !defined(__cplusplus)
#define __cplusplus
#endif /* __DECCXX */

/*  Resolves linking problems under HP-UX when compiling with gcc/g++ */
#if defined(__HPUX__) && defined(__GNUG__)
#define va_list __gnuc_va_list
#endif /*  HP-UX */

/*  ---------------------------------------------------------------------------- */
/*  check for native bool type and TRUE/FALSE constants */
/*  ---------------------------------------------------------------------------- */

/*  Add more tests here for Windows compilers that already define bool */
/*  (under Unix, configure tests for this) */
#ifndef HAVE_BOOL
    #if defined( __MWERKS__ )
        #if (__MWERKS__ >= 0x1000) && __option(bool)
            #define HAVE_BOOL
        #endif
    #elif defined(__APPLE__) && defined(__APPLE_CC__)
        /*  Apple bundled gcc supports bool */
        #define HAVE_BOOL
    #elif defined(__VISUALC__) && (__VISUALC__ == 1020)
        /*  in VC++ 4.2 the bool keyword is reserved (hence can't be typedefed) */
        /*  but not implemented, so we must #define it */
        #define bool unsigned int
    #elif defined(__VISUALC__) && (__VISUALC__ == 1010)
        /*  For VisualC++ 4.1, we need to define */
        /*  bool as something between 4.0 & 5.0... */
        typedef unsigned int wxbool;
        #define bool wxbool
        #define HAVE_BOOL
    #elif defined(__VISUALC__) && (__VISUALC__ > 1020)
        /*  VC++ supports bool since 4.2 */
        #define HAVE_BOOL
    #elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x500)
        /*  Borland 5.0+ supports bool */
        #define HAVE_BOOL
    #elif wxCHECK_WATCOM_VERSION(1,0)
        /*  Watcom 11+ supports bool */
        #define HAVE_BOOL
    #elif defined(__DIGITALMARS__)
        /*  DigitalMars supports bool */
        #define HAVE_BOOL
    #elif defined(__GNUWIN32__) || defined(__MINGW32__) || defined(__CYGWIN__)
        /*  Cygwin supports bool */
        #define HAVE_BOOL
    #elif defined(__VISAGECPP__)
        #if __IBMCPP__ < 400
            typedef unsigned long bool;
            #define true ((bool)1)
            #define false ((bool)0)
        #endif
        #define HAVE_BOOL
    #endif /*  compilers */
#endif /*  HAVE_BOOL */

#if !defined(__MWERKS__) || !defined(true)
#if !defined(HAVE_BOOL) && !defined(bool) && !defined(VMS)
    /*  NB: of course, this doesn't replace the standard type, because, for */
    /*      example, overloading based on bool/int parameter doesn't work and */
    /*      so should be avoided in portable programs */
    typedef unsigned int bool;
#endif /*  bool */

/*  deal with TRUE/true stuff: we assume that if the compiler supports bool, it */
/*  supports true/false as well and that, OTOH, if it does _not_ support bool, */
/*  it doesn't support these keywords (this is less sure, in particular VC++ */
/*  4.x could be a problem here) */
#ifndef HAVE_BOOL
    #define true ((bool)1)
    #define false ((bool)0)
#endif
#endif

/*  for backwards compatibility, also define TRUE and FALSE */
/*  */
/*  note that these definitions should work both in C++ and C code, so don't */
/*  use true/false below */
#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

typedef short int WXTYPE;

/*  special care should be taken with this type under Windows where the real */
/*  window id is unsigned, so we must always do the cast before comparing them */
/*  (or else they would be always different!). Using wxGetWindowId() which does */
/*  the cast itself is recommended. Note that this type can't be unsigned */
/*  because wxID_ANY == -1 is a valid (and largely used) value for window id. */
typedef int wxWindowID;

/*  ---------------------------------------------------------------------------- */
/*  other feature tests */
/*  ---------------------------------------------------------------------------- */

/*  Every ride down a slippery slope begins with a single step.. */
/*  */
/*  Yes, using nested classes is indeed against our coding standards in */
/*  general, but there are places where you can use them to advantage */
/*  without totally breaking ports that cannot use them.  If you do, then */
/*  wrap it in this guard, but such cases should still be relatively rare. */
#define wxUSE_NESTED_CLASSES    1

/*  check for explicit keyword support */
#ifndef HAVE_EXPLICIT
    #if defined(__VISUALC__) && (__VISUALC__ >= 1100)
        /*  VC++ 6.0 and 5.0 have explicit (what about earlier versions?) */
        #define HAVE_EXPLICIT
    #elif ( defined(__MINGW32__) || defined(__CYGWIN32__) ) \
          && wxCHECK_GCC_VERSION(2, 95)
        /*  GCC 2.95 has explicit, what about earlier versions? */
        #define HAVE_EXPLICIT
    #elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x0520)
        /*  BC++ 4.52 doesn't support explicit, CBuilder 1 does */
        #define HAVE_EXPLICIT
    #elif defined(__MWERKS__) && (__MWERKS__ >= 0x2400)
        /*  Metrowerks CW6 or higher has explicit */
        #define HAVE_EXPLICIT
    #elif defined(__DIGITALMARS__)
        #define HAVE_EXPLICIT
    #endif
#endif /*  !HAVE_EXPLICIT */

#ifdef HAVE_EXPLICIT
    #define wxEXPLICIT explicit
#else /*  !HAVE_EXPLICIT */
    #define wxEXPLICIT
#endif /*  HAVE_EXPLICIT/!HAVE_EXPLICIT */

/* check for static/const_cast<>() (we don't use the other ones for now) */
#ifndef HAVE_CXX_CASTS
    #if defined(__VISUALC__) && (__VISUALC__ >= 1100)
        /*  VC++ 6.0 and 5.0 have C++ casts (what about earlier versions?) */
        #define HAVE_CXX_CASTS
    #elif defined(__MINGW32__) || defined(__CYGWIN32__)
        #if wxCHECK_GCC_VERSION(2, 95)
            /*  GCC 2.95 has C++ casts, what about earlier versions? */
            #define HAVE_CXX_CASTS
        #endif
    #endif
#endif /*  !HAVE_CXX_CASTS */

#ifdef HAVE_CXX_CASTS
    #ifndef HAVE_CONST_CAST
        #define HAVE_CONST_CAST
    #endif
    #ifndef HAVE_REINTERPRET_CAST
        #define HAVE_REINTERPRET_CAST
    #endif
    #ifndef HAVE_STATIC_CAST
        #define HAVE_STATIC_CAST
    #endif
    #ifndef HAVE_DYNAMIC_CAST
        #define HAVE_DYNAMIC_CAST
    #endif
#endif /*  HAVE_CXX_CASTS */

#ifdef HAVE_STATIC_CAST
    #define wx_static_cast(t, x) static_cast<t>(x)
#else
    #define wx_static_cast(t, x) ((t)(x))
#endif

#ifdef HAVE_CONST_CAST
    #define wx_const_cast(t, x) const_cast<t>(x)
#else
    #define wx_const_cast(t, x) ((t)(x))
#endif

#ifdef HAVE_REINTERPRET_CAST
    #define wx_reinterpret_cast(t, x) reinterpret_cast<t>(x)
#else
    #define wx_reinterpret_cast(t, x) ((t)(x))
#endif

/*
   This one is a wx invention: like static cast but used when we intentionally
   truncate from a larger to smaller type, static_cast<> can't be used for it
   as it results in warnings when using some compilers (SGI mipspro for example)
 */
#if defined(__INTELC__) && defined(__cplusplus)
    template <typename T, typename X>
    inline T wx_truncate_cast_impl(X x)
    {
        #pragma warning(push)
        /* implicit conversion of a 64-bit integral type to a smaller integral type */
        #pragma warning(disable: 1682)
        /* conversion from "X" to "T" may lose significant bits */
        #pragma warning(disable: 810)

        return x;

        #pragma warning(pop)
    }

    #define wx_truncate_cast(t, x) wx_truncate_cast_impl<t>(x)

#elif defined(__cplusplus) && defined(__VISUALC__) && __VISUALC__ >= 1310
    template <typename T, typename X>
    inline T wx_truncate_cast_impl(X x)
    {
        #pragma warning(push)
        /* conversion from 'X' to 'T', possible loss of data */
        #pragma warning(disable: 4267)

        return x;

        #pragma warning(pop)
    }

    #define wx_truncate_cast(t, x) wx_truncate_cast_impl<t>(x)
#else
    #define wx_truncate_cast(t, x) ((t)(x))
#endif

/* for consistency with wxStatic/DynamicCast defined in wx/object.h */
#define wxConstCast(obj, className) wx_const_cast(className *, obj)

#ifndef HAVE_STD_WSTRING
    #if defined(__VISUALC__) && (__VISUALC__ >= 1100)
        /*  VC++ 6.0 and 5.0 have std::wstring (what about earlier versions?) */
        #define HAVE_STD_WSTRING
    #elif ( defined(__MINGW32__) || defined(__CYGWIN32__) ) \
          && wxCHECK_GCC_VERSION(3, 3)
        /*  GCC 3.1 has std::wstring; 3.0 never was in MinGW, 2.95 hasn't it */
        #define HAVE_STD_WSTRING
    #endif
#endif

#ifndef HAVE_STD_STRING_COMPARE
    #if defined(__VISUALC__) && (__VISUALC__ >= 1100)
        /*  VC++ 6.0 and 5.0 have std::string::compare */
        /*  (what about earlier versions?) */
        #define HAVE_STD_STRING_COMPARE
    #elif ( defined(__MINGW32__) || defined(__CYGWIN32__) ) \
          && wxCHECK_GCC_VERSION(3, 1)
        /*  GCC 3.1 has std::string::compare; */
        /*  3.0 never was in MinGW, 2.95 hasn't it */
        #define HAVE_STD_STRING_COMPARE
    #endif
#endif

/* provide replacement for C99 va_copy() if the compiler doesn't have it */

/* could be already defined by configure or the user */
#ifndef wxVaCopy
    /* if va_copy is a macro or configure detected that we have it, use it */
    #if defined(va_copy) || defined(HAVE_VA_COPY)
        #define wxVaCopy va_copy
    #else /* no va_copy, try to provide a replacement */
        /*
           configure tries to determine whether va_list is an array or struct
           type, but it may not be used under Windows, so deal with a few
           special cases.
         */

        #ifdef __WATCOMC__
            /* Watcom uses array type for va_list except for PPC and Alpha */
            #if !defined(__PPC__) && !defined(__AXP__)
                #define VA_LIST_IS_ARRAY
            #endif
        #endif /* __WATCOMC__ */

        #if defined(__PPC__) && (defined(_CALL_SYSV) || defined (_WIN32))
            /*
                PPC using SysV ABI and NT/PPC are special in that they use an
                extra level of indirection.
             */
            #define VA_LIST_IS_POINTER
        #endif /* SysV or Win32 on __PPC__ */

        /*
            note that we use memmove(), not memcpy(), in case anybody tries
            to do wxVaCopy(ap, ap)
         */
        #if defined(VA_LIST_IS_POINTER)
            #define wxVaCopy(d, s)  memmove(*(d), *(s), sizeof(va_list))
        #elif defined(VA_LIST_IS_ARRAY)
            #define wxVaCopy(d, s) memmove((d), (s), sizeof(va_list))
        #else /* we can only hope that va_lists are simple lvalues */
            #define wxVaCopy(d, s) ((d) = (s))
        #endif
    #endif /* va_copy/!va_copy */
#endif /* wxVaCopy */


/*  ---------------------------------------------------------------------------- */
/*  portable calling conventions macros */
/*  ---------------------------------------------------------------------------- */

/*  stdcall is used for all functions called by Windows under Windows */
#if defined(__WINDOWS__)
    #if defined(__GNUWIN32__)
        #define wxSTDCALL __attribute__((stdcall))
    #else
        /*  both VC++ and Borland understand this */
        #define wxSTDCALL _stdcall
    #endif

#else /*  Win */
    /*  no such stupidness under Unix */
    #define wxSTDCALL
#endif /*  platform */

/*  LINKAGEMODE mode is empty for everyting except OS/2 */
#ifndef LINKAGEMODE
    #define LINKAGEMODE
#endif /*  LINKAGEMODE */

/*  wxCALLBACK should be used for the functions which are called back by */
/*  Windows (such as compare function for wxListCtrl) */
#if defined(__WIN32__) && !defined(__WXMICROWIN__)
    #define wxCALLBACK wxSTDCALL
#else
    /*  no stdcall under Unix nor Win16 */
    #define wxCALLBACK
#endif /*  platform */

/*  generic calling convention for the extern "C" functions */

#if defined(__VISUALC__)
  #define   wxC_CALLING_CONV    _cdecl
#elif defined(__VISAGECPP__)
  #define   wxC_CALLING_CONV    _Optlink
#else   /*  !Visual C++ */
  #define   wxC_CALLING_CONV
#endif  /*  compiler */

/*  callling convention for the qsort(3) callback */
#define wxCMPFUNC_CONV wxC_CALLING_CONV

/*  compatibility :-( */
#define CMPFUNC_CONV wxCMPFUNC_CONV

/*  DLL import/export declarations */
#include "wx/dlimpexp.h"

/*  ---------------------------------------------------------------------------- */
/*  Very common macros */
/*  ---------------------------------------------------------------------------- */

/*  Printf-like attribute definitions to obtain warnings with GNU C/C++ */
#ifndef ATTRIBUTE_PRINTF
#   if defined(__GNUC__) && !wxUSE_UNICODE
#       define ATTRIBUTE_PRINTF(m, n) __attribute__ ((__format__ (__printf__, m, n)))
#   else
#       define ATTRIBUTE_PRINTF(m, n)
#   endif

#   define ATTRIBUTE_PRINTF_1 ATTRIBUTE_PRINTF(1, 2)
#   define ATTRIBUTE_PRINTF_2 ATTRIBUTE_PRINTF(2, 3)
#   define ATTRIBUTE_PRINTF_3 ATTRIBUTE_PRINTF(3, 4)
#   define ATTRIBUTE_PRINTF_4 ATTRIBUTE_PRINTF(4, 5)
#   define ATTRIBUTE_PRINTF_5 ATTRIBUTE_PRINTF(5, 6)
#endif /* !defined(ATTRIBUTE_PRINTF) */

/*  Macro to issue warning when using deprecated functions with gcc3 or MSVC7: */
#if wxCHECK_GCC_VERSION(3, 1)
    #define wxDEPRECATED(x) x __attribute__ ((deprecated))
#elif defined(__VISUALC__) && (__VISUALC__ >= 1300)
    #define wxDEPRECATED(x) __declspec(deprecated) x
#else
    #define wxDEPRECATED(x) x
#endif

/*  everybody gets the assert and other debug macros */
#include "wx/debug.h"

/*  NULL declaration: it must be defined as 0 for C++ programs (in particular, */
/*  it must not be defined as "(void *)0" which is standard for C but completely */
/*  breaks C++ code) */
#ifndef __HANDHELDPC__
#include <stddef.h>
#endif

/*  delete pointer if it is not NULL and NULL it afterwards */
/*  (checking that it's !NULL before passing it to delete is just a */
/*   a question of style, because delete will do it itself anyhow, but it might */
/*   be considered as an error by some overzealous debugging implementations of */
/*   the library, so we do it ourselves) */
#define wxDELETE(p)      if ( (p) != NULL ) { delete p; p = NULL; }

/*  delete an array and NULL it (see comments above) */
#define wxDELETEA(p)     if ( (p) ) { delete [] (p); p = NULL; }

/*  size of statically declared array */
#define WXSIZEOF(array)   (sizeof(array)/sizeof(array[0]))

/*  symbolic constant used by all Find()-like functions returning positive */
/*  integer on success as failure indicator */
#define wxNOT_FOUND       (-1)

/*  ---------------------------------------------------------------------------- */
/*  macros to avoid compiler warnings */
/*  ---------------------------------------------------------------------------- */

/*  Macro to cut down on compiler warnings. */
#if 1 /*  there should be no more any compilers needing the "#else" version */
    #define WXUNUSED(identifier) /* identifier */
#else  /*  stupid, broken compiler */
    #define WXUNUSED(identifier) identifier
#endif

/*  some arguments are only used in debug mode, but unused in release one */
#ifdef __WXDEBUG__
    #define WXUNUSED_UNLESS_DEBUG(param)  param
#else
    #define WXUNUSED_UNLESS_DEBUG(param)  WXUNUSED(param)
#endif

/*  some arguments are not used in unicode mode */
#if wxUSE_UNICODE
    #define WXUNUSED_IN_UNICODE(param)  WXUNUSED(param)
#else
    #define WXUNUSED_IN_UNICODE(param)  param
#endif

/*  some arguments are not used in WinCE build */
#ifdef __WXWINCE__
    #define WXUNUSED_IN_WINCE(param)  WXUNUSED(param)
#else
    #define WXUNUSED_IN_WINCE(param)  param
#endif

/*  unused parameters in non stream builds */
#if wxUSE_STREAMS
    #define WXUNUSED_UNLESS_STREAMS(param)  param
#else
    #define WXUNUSED_UNLESS_STREAMS(param)  WXUNUSED(param)
#endif

/*  some compilers give warning about a possibly unused variable if it is */
/*  initialized in both branches of if/else and shut up if it is initialized */
/*  when declared, but other compilers then give warnings about unused variable */
/*  value -- this should satisfy both of them */
#if defined(__VISUALC__)
    #define wxDUMMY_INITIALIZE(val) = val
#else
    #define wxDUMMY_INITIALIZE(val)
#endif

/*  sometimes the value of a variable is *really* not used, to suppress  the */
/*  resulting warning you may pass it to this function */
#ifdef __cplusplus
#   ifdef __BORLANDC__
#       define wxUnusedVar(identifier) identifier
#   else
        template <class T>
            inline void wxUnusedVar(const T& WXUNUSED(t)) { }
#   endif
#endif

/*  ---------------------------------------------------------------------------- */
/*  compiler specific settings */
/*  ---------------------------------------------------------------------------- */

/*  to allow compiling with warning level 4 under Microsoft Visual C++ some */
/*  warnings just must be disabled */
#ifdef  __VISUALC__
  #pragma warning(disable: 4514) /*  unreferenced inline func has been removed */
/*
  you might be tempted to disable this one also: triggered by CHECK and FAIL
  macros in debug.h, but it's, overall, a rather useful one, so I leave it and
  will try to find some way to disable this warning just for CHECK/FAIL. Anyone?
*/
  #pragma warning(disable: 4127) /*  conditional expression is constant */
#endif  /*  VC++ */

#if defined(__MWERKS__)
    #undef try
    #undef except
    #undef finally
    #define except(x) catch(...)
#endif /*  Metrowerks */

#if wxONLY_WATCOM_EARLIER_THAN(1,4)
    typedef short mode_t;
#endif

/*  where should i put this? we need to make sure of this as it breaks */
/*  the <iostream> code. */
#if !wxUSE_IOSTREAMH && defined(__WXDEBUG__)
#  ifndef __MWERKS__
/*  #undef __WXDEBUG__ */
#    ifdef wxUSE_DEBUG_NEW_ALWAYS
#    undef wxUSE_DEBUG_NEW_ALWAYS
#    define wxUSE_DEBUG_NEW_ALWAYS 0
#    endif
#  endif
#endif

/*  ---------------------------------------------------------------------------- */
/*  standard wxWidgets types */
/*  ---------------------------------------------------------------------------- */

/*  the type for screen and DC coordinates */
typedef int wxCoord;

enum {  wxDefaultCoord = -1 };

/*  ---------------------------------------------------------------------------- */
/*  define fixed length types */
/*  ---------------------------------------------------------------------------- */

#if defined(__WXPALMOS__) || defined(__MINGW32__)
    #include <sys/types.h>
#endif

/*  chars are always one byte (by definition), shorts are always two (in */
/*  practice) */

/*  8bit */
#ifndef SIZEOF_CHAR
    #define SIZEOF_CHAR 1
#endif
typedef signed char wxInt8;
typedef unsigned char wxUint8;
typedef wxUint8 wxByte;


/*  16bit */
#ifdef SIZEOF_SHORT
    #if SIZEOF_SHORT != 2
        #error "wxWidgets assumes sizeof(short) == 2, please fix the code"
    #endif
#else
    #define SIZEOF_SHORT 2
#endif

typedef signed short wxInt16;
typedef unsigned short wxUint16;

typedef wxUint16 wxWord;

/*
  things are getting more interesting with ints, longs and pointers

  there are several different standard data models described by this table:

  +-----------+----------------------------+
  |type\model | LP64 ILP64 LLP64 ILP32 LP32|
  +-----------+----------------------------+
  |char       |  8     8     8     8     8 |
  |short      | 16    16    16    16    16 |
  |int        | 32    64    32    32    16 |
  |long       | 64    64    32    32    32 |
  |long long  |             64             |
  |void *     | 64    64    64    32    32 |
  +-----------+----------------------------+

  Win16 used LP32 (but we don't support it any longer), Win32 obviously used
  ILP32 and Win64 uses LLP64 (a.k.a. P64)

  Under Unix LP64 is the most widely used (the only I've ever seen, in fact)
 */

/*  32bit */
#ifdef __PALMOS__
    typedef int wxInt32;
    typedef unsigned int wxUint32;
    #define SIZEOF_INT 4
    #define SIZEOF_LONG 4
    #define SIZEOF_WCHAR_T 2
    #define SIZEOF_SIZE_T 4
    #define wxSIZE_T_IS_UINT
    #define SIZEOF_VOID_P 4
    #define SIZEOF_SIZE_T 4
#elif defined(__WINDOWS__)
    /*  Win64 uses LLP64 model and so ints and longs have the same size as in */
    /*  Win32 */
    #if defined(__WIN32__)
        typedef int wxInt32;
        typedef unsigned int wxUint32;

        /* Assume that if SIZEOF_INT is defined that all the other ones except
           SIZEOF_SIZE_T, are too.  See next #if below.  */
        #ifndef SIZEOF_INT
            #define SIZEOF_INT 4
            #define SIZEOF_LONG 4
            #define SIZEOF_WCHAR_T 2

            /*
               under Win64 sizeof(size_t) == 8 and so it is neither unsigned
               int nor unsigned long!
             */
            #ifdef __WIN64__
                #define SIZEOF_SIZE_T 8

                #undef wxSIZE_T_IS_UINT
            #else /* Win32 */
                #define SIZEOF_SIZE_T 4

                #define wxSIZE_T_IS_UINT
            #endif
            #undef wxSIZE_T_IS_ULONG

            #ifdef __WIN64__
                #define SIZEOF_VOID_P 8
            #else /*  Win32 */
                #define SIZEOF_VOID_P 4
            #endif /*  Win64/32 */
        #endif /*  !defined(SIZEOF_INT) */

        /*
          If Python.h was included first, it defines all of the SIZEOF's above
          except for SIZEOF_SIZE_T, so we need to do it here to avoid
          triggering the #error in the ssize_t typedefs below...
        */
        #ifndef SIZEOF_SIZE_T
            #ifdef __WIN64__
                #define SIZEOF_SIZE_T 8
            #else /* Win32 */
                #define SIZEOF_SIZE_T 4
            #endif
        #endif
    #else
        #error "Unsupported Windows version"
    #endif
#else /*  !Windows */
    /*  SIZEOF_XXX are normally defined by configure */
    #ifdef SIZEOF_INT
        #if SIZEOF_INT == 8
            /*  must be ILP64 data model, there is normally a special 32 bit */
            /*  type in it but we don't know what it is... */
            #error "No 32bit int type on this platform"
        #elif SIZEOF_INT == 4
            typedef int wxInt32;
            typedef unsigned int wxUint32;
        #elif SIZEOF_INT == 2
            /*  must be LP32 */
            #if SIZEOF_LONG != 4
                #error "No 32bit int type on this platform"
            #endif

            typedef long wxInt32;
            typedef unsigned long wxUint32;
        #else
            /*  wxWidgets is not ready for 128bit systems yet... */
            #error "Unknown sizeof(int) value, what are you compiling for?"
        #endif
    #else /*  !defined(SIZEOF_INT) */
        /*  assume default 32bit machine -- what else can we do? */
        wxCOMPILE_TIME_ASSERT( sizeof(int) == 4, IntMustBeExactly4Bytes);
        wxCOMPILE_TIME_ASSERT( sizeof(size_t) == 4, SizeTMustBeExactly4Bytes);
        wxCOMPILE_TIME_ASSERT( sizeof(void *) == 4, PtrMustBeExactly4Bytes);

        #define SIZEOF_INT 4
        #define SIZEOF_SIZE_T 4
        #define SIZEOF_VOID_P 4

        typedef int wxInt32;
        typedef unsigned int wxUint32;

        #if defined(__MACH__) && !defined(SIZEOF_WCHAR_T)
            #define SIZEOF_WCHAR_T 4
        #endif
        #if wxUSE_WCHAR_T && !defined(SIZEOF_WCHAR_T)
            /*  also assume that sizeof(wchar_t) == 2 (under Unix the most */
            /*  common case is 4 but there configure would have defined */
            /*  SIZEOF_WCHAR_T for us) */
            /*  the most common case */
            wxCOMPILE_TIME_ASSERT( sizeof(wchar_t) == 2,
                                    Wchar_tMustBeExactly2Bytes);

            #define SIZEOF_WCHAR_T 2
        #endif /*  wxUSE_WCHAR_T */
    #endif
#endif /*  Win/!Win */

typedef wxUint32 wxDword;

/*
   Define an integral type big enough to contain all of long, size_t and void *.
 */
#if SIZEOF_LONG >= SIZEOF_VOID_P && SIZEOF_LONG >= SIZEOF_SIZE_T
    /* normal case */
    typedef unsigned long wxUIntPtr;
    typedef long wxIntPtr;
#elif SIZEOF_SIZE_T >= SIZEOF_VOID_P
    /* Win64 case */
    typedef size_t wxUIntPtr;
    #define wxIntPtr ssize_t
#else
    /*
       This should never happen for the current architectures but if you're
       using one where it does, please contact wx-dev@lists.wxwidgets.org.
     */
    #error "Pointers can't be stored inside integer types."
#endif

#ifdef __cplusplus
/* And also define a couple of simple functions to cast pointer to/from it. */
inline wxUIntPtr wxPtrToUInt(const void *p)
{
    /*
       VC++ 7.1 gives warnings about casts such as below even when they're
       explicit with /Wp64 option, suppress them as we really know what we're
       doing here. Same thing with icc with -Wall.
     */
#ifdef __VISUALC__
    #if __VISUALC__ >= 1200
        #pragma warning(push)
    #endif
    /* pointer truncation from '' to '' */
    #pragma warning(disable: 4311)
#elif defined(__INTELC__)
    #pragma warning(push)
    /* conversion from pointer to same-sized integral type */
    #pragma warning(disable: 1684)
#endif

    return wx_reinterpret_cast(wxUIntPtr, p);

#if (defined(__VISUALC__) && __VISUALC__ >= 1200) || defined(__INTELC__)
    #pragma warning(pop)
#endif
}

inline void *wxUIntToPtr(wxUIntPtr p)
{
#ifdef __VISUALC__
    #if __VISUALC__ >= 1200
        #pragma warning(push)
    #endif
    /* conversion to type of greater size */
    #pragma warning(disable: 4312)
#elif defined(__INTELC__)
    #pragma warning(push)
    /* invalid type conversion: "wxUIntPtr={unsigned long}" to "void *" */
    #pragma warning(disable: 171)
#endif

    return wx_reinterpret_cast(void *, p);

#if (defined(__VISUALC__) && __VISUALC__ >= 1200) || defined(__INTELC__)
    #pragma warning(pop)
#endif
}
#endif /*__cplusplus*/


/*  64 bit */

/*  NB: we #define and not typedef wxLongLong_t because we use "#ifdef */
/*      wxLongLong_t" in wx/longlong.h */

/*      wxULongLong_t is set later (usually to unsigned wxLongLong_t) */

/*  to avoid compilation problems on 64bit machines with ambiguous method calls */
/*  we will need to define this */
#undef wxLongLongIsLong

/*
   First check for specific compilers which have known 64 bit integer types,
   this avoids clashes with SIZEOF_LONG[_LONG] being defined incorrectly for
   e.g. MSVC builds (Python.h defines it as 8 even for MSVC).

   Also notice that we check for "long long" before checking for 64 bit long as
   we still want to use "long long" and not "long" for wxLongLong_t on 64 bit
   architectures to be able to pass wxLongLong_t to the standard functions
   prototyped as taking "long long" such as strtoll().
 */
#if (defined(__VISUALC__) && defined(__WIN32__))
    #define wxLongLong_t __int64
    #define wxLongLongSuffix i64
    #define wxLongLongFmtSpec wxT("I64")
#elif defined(__BORLANDC__) && defined(__WIN32__) && (__BORLANDC__ >= 0x520)
    #define wxLongLong_t __int64
    #define wxLongLongSuffix i64
    #define wxLongLongFmtSpec wxT("L")
#elif (defined(__WATCOMC__) && (defined(__WIN32__) || defined(__DOS__) || defined(__OS2__)))
      #define wxLongLong_t __int64
      #define wxLongLongSuffix i64
      #define wxLongLongFmtSpec wxT("L")
#elif defined(__DIGITALMARS__)
      #define wxLongLong_t __int64
      #define wxLongLongSuffix LL
      #define wxLongLongFmtSpec wxT("ll")
#elif defined(__MINGW32__)
    #define wxLongLong_t long long
    #define wxLongLongSuffix ll
    #define wxLongLongFmtSpec wxT("I64")
#elif defined(__MWERKS__)
    #if __option(longlong)
        #define wxLongLong_t long long
        #define wxLongLongSuffix ll
        #define wxLongLongFmtSpec wxT("ll")
    #else
        #error "The 64 bit integer support in CodeWarrior has been disabled."
        #error "See the documentation on the 'longlong' pragma."
    #endif
#elif defined(__WXPALMOS__)
    #define wxLongLong_t int64_t
    #define wxLongLongSuffix ll
    #define wxLongLongFmtSpec wxT("ll")
#elif defined(__VISAGECPP__) && __IBMCPP__ >= 400
    #define wxLongLong_t long long
#elif (defined(SIZEOF_LONG_LONG) && SIZEOF_LONG_LONG >= 8)  || \
        defined(__GNUC__) || \
        defined(__CYGWIN__) || \
        defined(__WXMICROWIN__) || \
        (defined(__DJGPP__) && __DJGPP__ >= 2)
    #define wxLongLong_t long long
    #define wxLongLongSuffix ll
    #define wxLongLongFmtSpec wxT("ll")
#elif defined(SIZEOF_LONG) && (SIZEOF_LONG == 8)
    #define wxLongLong_t long
    #define wxLongLongSuffix l
    #define wxLongLongFmtSpec wxT("l")
    #define wxLongLongIsLong
#endif


#ifdef wxLongLong_t

    #ifdef __WXPALMOS__
        #define wxULongLong_t uint64_t
    #else
        #define wxULongLong_t unsigned wxLongLong_t
    #endif

    /*  these macros allow to define 64 bit constants in a portable way */
    #define wxLL(x) wxCONCAT(x, wxLongLongSuffix)
    #define wxULL(x) wxCONCAT(x, wxCONCAT(u, wxLongLongSuffix))

    typedef wxLongLong_t wxInt64;
    typedef wxULongLong_t wxUint64;

    #define wxHAS_INT64 1

#elif wxUSE_LONGLONG
    /*  these macros allow to define 64 bit constants in a portable way */
    #define wxLL(x) wxLongLong(x)
    #define wxULL(x) wxULongLong(x)

    #define wxInt64 wxLongLong
    #define wxUint64 wxULongLong

    #define wxHAS_INT64 1

#else /* !wxUSE_LONGLONG */

    #define wxHAS_INT64 0

#endif


/* Make sure ssize_t is defined (a signed type the same size as size_t) */
/* HAVE_SSIZE_T should be defined for compiliers that already have it */
#ifdef __MINGW32__
    #if defined(_SSIZE_T_) && !defined(HAVE_SSIZE_T)
        #define HAVE_SSIZE_T
    #endif
#endif
#if defined(__PALMOS__) && !defined(HAVE_SSIZE_T)
    #define HAVE_SSIZE_T
#endif
#if wxCHECK_WATCOM_VERSION(1,4)
    #define HAVE_SSIZE_T
#endif
#ifndef HAVE_SSIZE_T
    #if SIZEOF_SIZE_T == 4
        typedef wxInt32 ssize_t;
    #elif SIZEOF_SIZE_T == 8
        typedef wxInt64 ssize_t;
    #else
        #error "error defining ssize_t, size_t is not 4 or 8 bytes"
    #endif

    /* prevent ssize_t redefinitions in other libraries */
    #define HAVE_SSIZE_T
#endif


/*  base floating point types */
/*  wxFloat32: 32 bit IEEE float ( 1 sign, 8 exponent bits, 23 fraction bits */
/*  wxFloat64: 64 bit IEEE float ( 1 sign, 11 exponent bits, 52 fraction bits */
/*  wxDouble: native fastest representation that has at least wxFloat64 */
/*            precision, so use the IEEE types for storage, and this for */
/*            calculations */

typedef float wxFloat32;
#if (defined( __WXMAC__ ) || defined(__WXCOCOA__))  && defined (__MWERKS__)
    typedef short double wxFloat64;
#else
    typedef double wxFloat64;
#endif

typedef double wxDouble;

/*
    Some (non standard) compilers typedef wchar_t as an existing type instead
    of treating it as a real fundamental type, set wxWCHAR_T_IS_REAL_TYPE to 0
    for them and to 1 for all the others.
 */
#if wxUSE_WCHAR_T
    /*
        VC++ typedefs wchar_t as unsigned short by default, that is unless
        /Za or /Zc:wchar_t option is used in which case _WCHAR_T_DEFINED is
        defined.
     */
#   if defined(__VISUALC__) && !defined(_NATIVE_WCHAR_T_DEFINED)
#       define wxWCHAR_T_IS_REAL_TYPE 0
#   else /* compiler having standard-conforming wchar_t */
#       define wxWCHAR_T_IS_REAL_TYPE 1
#   endif
#endif /* wxUSE_WCHAR_T */

/*  ---------------------------------------------------------------------------- */
/*  byte ordering related definition and macros */
/*  ---------------------------------------------------------------------------- */

/*  byte sex */

#define  wxBIG_ENDIAN     4321
#define  wxLITTLE_ENDIAN  1234
#define  wxPDP_ENDIAN     3412

#ifdef WORDS_BIGENDIAN
#define  wxBYTE_ORDER  wxBIG_ENDIAN
#else
#define  wxBYTE_ORDER  wxLITTLE_ENDIAN
#endif

/*  byte swapping */

#if defined (__MWERKS__) && ( (__MWERKS__ < 0x0900) || macintosh )
/*  assembler versions for these */
#ifdef __POWERPC__
    inline wxUint16 wxUINT16_SWAP_ALWAYS( wxUint16 i )
        {return (__lhbrx( &i , 0 ) );}
    inline wxInt16 wxINT16_SWAP_ALWAYS( wxInt16 i )
        {return (__lhbrx( &i , 0 ) );}
    inline wxUint32 wxUINT32_SWAP_ALWAYS( wxUint32 i )
        {return (__lwbrx( &i , 0 ) );}
    inline wxInt32 wxINT32_SWAP_ALWAYS( wxInt32 i )
        {return (__lwbrx( &i , 0 ) );}
#else
    #pragma parameter __D0 wxUINT16_SWAP_ALWAYS(__D0)
    pascal wxUint16 wxUINT16_SWAP_ALWAYS(wxUint16 value)
        = { 0xE158 };

    #pragma parameter __D0 wxINT16_SWAP_ALWAYS(__D0)
    pascal wxInt16 wxINT16_SWAP_ALWAYS(wxInt16 value)
        = { 0xE158 };

    #pragma parameter __D0 wxUINT32_SWAP_ALWAYS (__D0)
    pascal wxUint32 wxUINT32_SWAP_ALWAYS(wxUint32 value)
        = { 0xE158, 0x4840, 0xE158 };

    #pragma parameter __D0 wxINT32_SWAP_ALWAYS (__D0)
    pascal wxInt32 wxINT32_SWAP_ALWAYS(wxInt32 value)
        = { 0xE158, 0x4840, 0xE158 };

#endif
#else /*  !MWERKS */
#define wxUINT16_SWAP_ALWAYS(val) \
   ((wxUint16) ( \
    (((wxUint16) (val) & (wxUint16) 0x00ffU) << 8) | \
    (((wxUint16) (val) & (wxUint16) 0xff00U) >> 8)))

#define wxINT16_SWAP_ALWAYS(val) \
   ((wxInt16) ( \
    (((wxUint16) (val) & (wxUint16) 0x00ffU) << 8) | \
    (((wxUint16) (val) & (wxUint16) 0xff00U) >> 8)))

#define wxUINT32_SWAP_ALWAYS(val) \
   ((wxUint32) ( \
    (((wxUint32) (val) & (wxUint32) 0x000000ffU) << 24) | \
    (((wxUint32) (val) & (wxUint32) 0x0000ff00U) <<  8) | \
    (((wxUint32) (val) & (wxUint32) 0x00ff0000U) >>  8) | \
    (((wxUint32) (val) & (wxUint32) 0xff000000U) >> 24)))

#define wxINT32_SWAP_ALWAYS(val) \
   ((wxInt32) ( \
    (((wxUint32) (val) & (wxUint32) 0x000000ffU) << 24) | \
    (((wxUint32) (val) & (wxUint32) 0x0000ff00U) <<  8) | \
    (((wxUint32) (val) & (wxUint32) 0x00ff0000U) >>  8) | \
    (((wxUint32) (val) & (wxUint32) 0xff000000U) >> 24)))
#endif
/*  machine specific byte swapping */

#ifdef wxLongLong_t
    #define wxUINT64_SWAP_ALWAYS(val) \
       ((wxUint64) ( \
        (((wxUint64) (val) & (wxUint64) wxULL(0x00000000000000ff)) << 56) | \
        (((wxUint64) (val) & (wxUint64) wxULL(0x000000000000ff00)) << 40) | \
        (((wxUint64) (val) & (wxUint64) wxULL(0x0000000000ff0000)) << 24) | \
        (((wxUint64) (val) & (wxUint64) wxULL(0x00000000ff000000)) <<  8) | \
        (((wxUint64) (val) & (wxUint64) wxULL(0x000000ff00000000)) >>  8) | \
        (((wxUint64) (val) & (wxUint64) wxULL(0x0000ff0000000000)) >> 24) | \
        (((wxUint64) (val) & (wxUint64) wxULL(0x00ff000000000000)) >> 40) | \
        (((wxUint64) (val) & (wxUint64) wxULL(0xff00000000000000)) >> 56)))

    #define wxINT64_SWAP_ALWAYS(val) \
       ((wxInt64) ( \
        (((wxUint64) (val) & (wxUint64) wxULL(0x00000000000000ff)) << 56) | \
        (((wxUint64) (val) & (wxUint64) wxULL(0x000000000000ff00)) << 40) | \
        (((wxUint64) (val) & (wxUint64) wxULL(0x0000000000ff0000)) << 24) | \
        (((wxUint64) (val) & (wxUint64) wxULL(0x00000000ff000000)) <<  8) | \
        (((wxUint64) (val) & (wxUint64) wxULL(0x000000ff00000000)) >>  8) | \
        (((wxUint64) (val) & (wxUint64) wxULL(0x0000ff0000000000)) >> 24) | \
        (((wxUint64) (val) & (wxUint64) wxULL(0x00ff000000000000)) >> 40) | \
        (((wxUint64) (val) & (wxUint64) wxULL(0xff00000000000000)) >> 56)))
#elif wxUSE_LONGLONG /*  !wxLongLong_t */
    #define wxUINT64_SWAP_ALWAYS(val) \
       ((wxUint64) ( \
        ((wxULongLong(val) & wxULongLong(0L, 0x000000ffU)) << 56) | \
        ((wxULongLong(val) & wxULongLong(0L, 0x0000ff00U)) << 40) | \
        ((wxULongLong(val) & wxULongLong(0L, 0x00ff0000U)) << 24) | \
        ((wxULongLong(val) & wxULongLong(0L, 0xff000000U)) <<  8) | \
        ((wxULongLong(val) & wxULongLong(0x000000ffL, 0U)) >>  8) | \
        ((wxULongLong(val) & wxULongLong(0x0000ff00L, 0U)) >> 24) | \
        ((wxULongLong(val) & wxULongLong(0x00ff0000L, 0U)) >> 40) | \
        ((wxULongLong(val) & wxULongLong(0xff000000L, 0U)) >> 56)))

    #define wxINT64_SWAP_ALWAYS(val) \
       ((wxInt64) ( \
        ((wxLongLong(val) & wxLongLong(0L, 0x000000ffU)) << 56) | \
        ((wxLongLong(val) & wxLongLong(0L, 0x0000ff00U)) << 40) | \
        ((wxLongLong(val) & wxLongLong(0L, 0x00ff0000U)) << 24) | \
        ((wxLongLong(val) & wxLongLong(0L, 0xff000000U)) <<  8) | \
        ((wxLongLong(val) & wxLongLong(0x000000ffL, 0U)) >>  8) | \
        ((wxLongLong(val) & wxLongLong(0x0000ff00L, 0U)) >> 24) | \
        ((wxLongLong(val) & wxLongLong(0x00ff0000L, 0U)) >> 40) | \
        ((wxLongLong(val) & wxLongLong(0xff000000L, 0U)) >> 56)))
#endif /*  wxLongLong_t/!wxLongLong_t */

#ifdef WORDS_BIGENDIAN
    #define wxUINT16_SWAP_ON_BE(val)  wxUINT16_SWAP_ALWAYS(val)
    #define wxINT16_SWAP_ON_BE(val)   wxINT16_SWAP_ALWAYS(val)
    #define wxUINT16_SWAP_ON_LE(val)  (val)
    #define wxINT16_SWAP_ON_LE(val)   (val)
    #define wxUINT32_SWAP_ON_BE(val)  wxUINT32_SWAP_ALWAYS(val)
    #define wxINT32_SWAP_ON_BE(val)   wxINT32_SWAP_ALWAYS(val)
    #define wxUINT32_SWAP_ON_LE(val)  (val)
    #define wxINT32_SWAP_ON_LE(val)   (val)
    #if wxHAS_INT64
        #define wxUINT64_SWAP_ON_BE(val)  wxUINT64_SWAP_ALWAYS(val)
        #define wxUINT64_SWAP_ON_LE(val)  (val)
    #endif
#else
    #define wxUINT16_SWAP_ON_LE(val)  wxUINT16_SWAP_ALWAYS(val)
    #define wxINT16_SWAP_ON_LE(val)   wxINT16_SWAP_ALWAYS(val)
    #define wxUINT16_SWAP_ON_BE(val)  (val)
    #define wxINT16_SWAP_ON_BE(val)   (val)
    #define wxUINT32_SWAP_ON_LE(val)  wxUINT32_SWAP_ALWAYS(val)
    #define wxINT32_SWAP_ON_LE(val)   wxINT32_SWAP_ALWAYS(val)
    #define wxUINT32_SWAP_ON_BE(val)  (val)
    #define wxINT32_SWAP_ON_BE(val)   (val)
    #if wxHAS_INT64
        #define wxUINT64_SWAP_ON_LE(val)  wxUINT64_SWAP_ALWAYS(val)
        #define wxUINT64_SWAP_ON_BE(val)  (val)
    #endif
#endif

/*  ---------------------------------------------------------------------------- */
/*  Geometric flags */
/*  ---------------------------------------------------------------------------- */

enum wxGeometryCentre
{
    wxCENTRE                  = 0x0001,
    wxCENTER                  = wxCENTRE
};

/*  centering into frame rather than screen (obsolete) */
#define wxCENTER_FRAME          0x0000
/*  centre on screen rather than parent */
#define wxCENTRE_ON_SCREEN      0x0002
#define wxCENTER_ON_SCREEN      wxCENTRE_ON_SCREEN

enum wxOrientation
{
    /* don't change the values of these elements, they are used elsewhere */
    wxHORIZONTAL              = 0x0004,
    wxVERTICAL                = 0x0008,

    wxBOTH                    = wxVERTICAL | wxHORIZONTAL
};

enum wxDirection
{
    wxLEFT                    = 0x0010,
    wxRIGHT                   = 0x0020,
    wxUP                      = 0x0040,
    wxDOWN                    = 0x0080,

    wxTOP                     = wxUP,
    wxBOTTOM                  = wxDOWN,

    wxNORTH                   = wxUP,
    wxSOUTH                   = wxDOWN,
    wxWEST                    = wxLEFT,
    wxEAST                    = wxRIGHT,

    wxALL                     = (wxUP | wxDOWN | wxRIGHT | wxLEFT)
};

enum wxAlignment
{
    wxALIGN_NOT               = 0x0000,
    wxALIGN_CENTER_HORIZONTAL = 0x0100,
    wxALIGN_CENTRE_HORIZONTAL = wxALIGN_CENTER_HORIZONTAL,
    wxALIGN_LEFT              = wxALIGN_NOT,
    wxALIGN_TOP               = wxALIGN_NOT,
    wxALIGN_RIGHT             = 0x0200,
    wxALIGN_BOTTOM            = 0x0400,
    wxALIGN_CENTER_VERTICAL   = 0x0800,
    wxALIGN_CENTRE_VERTICAL   = wxALIGN_CENTER_VERTICAL,

    wxALIGN_CENTER            = (wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL),
    wxALIGN_CENTRE            = wxALIGN_CENTER,

    /*  a mask to extract alignment from the combination of flags */
    wxALIGN_MASK              = 0x0f00
};

enum wxStretch
{
    wxSTRETCH_NOT             = 0x0000,
    wxSHRINK                  = 0x1000,
    wxGROW                    = 0x2000,
    wxEXPAND                  = wxGROW,
    wxSHAPED                  = 0x4000,
    wxFIXED_MINSIZE           = 0x8000,
#if wxABI_VERSION >= 20808
    wxRESERVE_SPACE_EVEN_IF_HIDDEN = 0x0002,
#endif
    wxTILE                    = 0xc000,

    /* for compatibility only, default now, don't use explicitly any more */
#if WXWIN_COMPATIBILITY_2_4
    wxADJUST_MINSIZE          = 0x00100000
#else
    wxADJUST_MINSIZE          = 0
#endif
};

/*  border flags: the values are chosen for backwards compatibility */
enum wxBorder
{
    /*  this is different from wxBORDER_NONE as by default the controls do have */
    /*  border */
    wxBORDER_DEFAULT = 0,

    wxBORDER_NONE   = 0x00200000,
    wxBORDER_STATIC = 0x01000000,
    wxBORDER_SIMPLE = 0x02000000,
    wxBORDER_RAISED = 0x04000000,
    wxBORDER_SUNKEN = 0x08000000,
    wxBORDER_DOUBLE = 0x10000000, /* deprecated */
    wxBORDER_THEME =  0x10000000,

    /*  a mask to extract border style from the combination of flags */
    wxBORDER_MASK   = 0x1f200000
};

/* This makes it easier to specify a 'normal' border for a control */
#if defined(__SMARTPHONE__) || defined(__POCKETPC__)
#define wxDEFAULT_CONTROL_BORDER    wxBORDER_SIMPLE
#else
#define wxDEFAULT_CONTROL_BORDER    wxBORDER_SUNKEN
#endif

/*  ---------------------------------------------------------------------------- */
/*  Window style flags */
/*  ---------------------------------------------------------------------------- */

/*
 * Values are chosen so they can be |'ed in a bit list.
 * Some styles are used across more than one group,
 * so the values mustn't clash with others in the group.
 * Otherwise, numbers can be reused across groups.
 *
 * From version 1.66:
 * Window (cross-group) styles now take up the first half
 * of the flag, and control-specific styles the
 * second half.
 *
 */

/*
 * Window (Frame/dialog/subwindow/panel item) style flags
 */
#define wxVSCROLL               0x80000000
#define wxHSCROLL               0x40000000
#define wxCAPTION               0x20000000

/*  New styles (border styles are now in their own enum) */
#define wxDOUBLE_BORDER         wxBORDER_DOUBLE
#define wxSUNKEN_BORDER         wxBORDER_SUNKEN
#define wxRAISED_BORDER         wxBORDER_RAISED
#define wxBORDER                wxBORDER_SIMPLE
#define wxSIMPLE_BORDER         wxBORDER_SIMPLE
#define wxSTATIC_BORDER         wxBORDER_STATIC
#define wxNO_BORDER             wxBORDER_NONE

/*  wxALWAYS_SHOW_SB: instead of hiding the scrollbar when it is not needed, */
/*  disable it - but still show (see also wxLB_ALWAYS_SB style) */
/*  */
/*  NB: as this style is only supported by wxUniversal and wxMSW so far */
#define wxALWAYS_SHOW_SB        0x00800000

/*  Clip children when painting, which reduces flicker in e.g. frames and */
/*  splitter windows, but can't be used in a panel where a static box must be */
/*  'transparent' (panel paints the background for it) */
#define wxCLIP_CHILDREN         0x00400000

/*  Note we're reusing the wxCAPTION style because we won't need captions */
/*  for subwindows/controls */
#define wxCLIP_SIBLINGS         0x20000000

#define wxTRANSPARENT_WINDOW    0x00100000

/*  Add this style to a panel to get tab traversal working outside of dialogs */
/*  (on by default for wxPanel, wxDialog, wxScrolledWindow) */
#define wxTAB_TRAVERSAL         0x00080000

/*  Add this style if the control wants to get all keyboard messages (under */
/*  Windows, it won't normally get the dialog navigation key events) */
#define wxWANTS_CHARS           0x00040000

/*  Make window retained (Motif only, see src/generic/scrolwing.cpp)
 *  This is non-zero only under wxMotif, to avoid a clash with wxPOPUP_WINDOW
 *  on other platforms
 */

#ifdef __WXMOTIF__
#define wxRETAINED              0x00020000
#else
#define wxRETAINED              0x00000000
#endif
#define wxBACKINGSTORE          wxRETAINED

/*  set this flag to create a special popup window: it will be always shown on */
/*  top of other windows, will capture the mouse and will be dismissed when the */
/*  mouse is clicked outside of it or if it loses focus in any other way */
#define wxPOPUP_WINDOW          0x00020000

/*  force a full repaint when the window is resized (instead of repainting just */
/*  the invalidated area) */
#define wxFULL_REPAINT_ON_RESIZE 0x00010000

/*  obsolete: now this is the default behaviour */
/*  */
/*  don't invalidate the whole window (resulting in a PAINT event) when the */
/*  window is resized (currently, makes sense for wxMSW only) */
#define wxNO_FULL_REPAINT_ON_RESIZE 0

/* A mask which can be used to filter (out) all wxWindow-specific styles.
 */
#define wxWINDOW_STYLE_MASK     \
    (wxVSCROLL|wxHSCROLL|wxBORDER_MASK|wxALWAYS_SHOW_SB|wxCLIP_CHILDREN| \
     wxCLIP_SIBLINGS|wxTRANSPARENT_WINDOW|wxTAB_TRAVERSAL|wxWANTS_CHARS| \
     wxRETAINED|wxPOPUP_WINDOW|wxFULL_REPAINT_ON_RESIZE)

/*
 * Extra window style flags (use wxWS_EX prefix to make it clear that they
 * should be passed to wxWindow::SetExtraStyle(), not SetWindowStyle())
 */

/*  by default, TransferDataTo/FromWindow() only work on direct children of the */
/*  window (compatible behaviour), set this flag to make them recursively */
/*  descend into all subwindows */
#define wxWS_EX_VALIDATE_RECURSIVELY    0x00000001

/*  wxCommandEvents and the objects of the derived classes are forwarded to the */
/*  parent window and so on recursively by default. Using this flag for the */
/*  given window allows to block this propagation at this window, i.e. prevent */
/*  the events from being propagated further upwards. The dialogs have this */
/*  flag on by default. */
#define wxWS_EX_BLOCK_EVENTS            0x00000002

/*  don't use this window as an implicit parent for the other windows: this must */
/*  be used with transient windows as otherwise there is the risk of creating a */
/*  dialog/frame with this window as a parent which would lead to a crash if the */
/*  parent is destroyed before the child */
#define wxWS_EX_TRANSIENT               0x00000004

/*  don't paint the window background, we'll assume it will */
/*  be done by a theming engine. This is not yet used but could */
/*  possibly be made to work in the future, at least on Windows */
#define wxWS_EX_THEMED_BACKGROUND       0x00000008

/*  this window should always process idle events */
#define wxWS_EX_PROCESS_IDLE            0x00000010

/*  this window should always process UI update events */
#define wxWS_EX_PROCESS_UI_UPDATES      0x00000020

/*  Draw the window in a metal theme on Mac */
#define wxFRAME_EX_METAL                0x00000040
#define wxDIALOG_EX_METAL               0x00000040

/*  Use this style to add a context-sensitive help to the window (currently for */
/*  Win32 only and it doesn't work if wxMINIMIZE_BOX or wxMAXIMIZE_BOX are used) */
#define wxWS_EX_CONTEXTHELP             0x00000080

/* synonyms for wxWS_EX_CONTEXTHELP for compatibility */
#define wxFRAME_EX_CONTEXTHELP          wxWS_EX_CONTEXTHELP
#define wxDIALOG_EX_CONTEXTHELP         wxWS_EX_CONTEXTHELP

/*  Create a window which is attachable to another top level window */
#define wxFRAME_DRAWER          0x0020

/*
 * MDI parent frame style flags
 * Can overlap with some of the above.
 */

#define wxFRAME_NO_WINDOW_MENU  0x0100

/*
 * wxMenuBar style flags
 */
/*  use native docking */
#define wxMB_DOCKABLE       0x0001

/*
 * wxMenu style flags
 */
#define wxMENU_TEAROFF      0x0001

/*
 * Apply to all panel items
 */
#define wxCOLOURED          0x0800
#define wxFIXED_LENGTH      0x0400

/*
 * Styles for wxListBox
 */
#define wxLB_SORT           0x0010
#define wxLB_SINGLE         0x0020
#define wxLB_MULTIPLE       0x0040
#define wxLB_EXTENDED       0x0080
/*  wxLB_OWNERDRAW is Windows-only */
#define wxLB_OWNERDRAW      0x0100
#define wxLB_NEEDED_SB      0x0200
#define wxLB_ALWAYS_SB      0x0400
#define wxLB_HSCROLL        wxHSCROLL
/*  always show an entire number of rows */
#define wxLB_INT_HEIGHT     0x0800

#if WXWIN_COMPATIBILITY_2_6
    /*  deprecated synonyms */
    #define wxPROCESS_ENTER   0x0400  /*  wxTE_PROCESS_ENTER */
    #define wxPASSWORD        0x0800  /*  wxTE_PASSWORD */
#endif

/*
 * wxComboBox style flags
 */
#define wxCB_SIMPLE         0x0004
#define wxCB_SORT           0x0008
#define wxCB_READONLY       0x0010
#define wxCB_DROPDOWN       0x0020

/*
 * wxRadioBox style flags
 */
/*  should we number the items from left to right or from top to bottom in a 2d */
/*  radiobox? */
#define wxRA_LEFTTORIGHT    0x0001
#define wxRA_TOPTOBOTTOM    0x0002

/*  New, more intuitive names to specify majorDim argument */
#define wxRA_SPECIFY_COLS   wxHORIZONTAL
#define wxRA_SPECIFY_ROWS   wxVERTICAL

/*  Old names for compatibility */
#define wxRA_HORIZONTAL     wxHORIZONTAL
#define wxRA_VERTICAL       wxVERTICAL
#define wxRA_USE_CHECKBOX   0x0010 /* alternative native subcontrols (wxPalmOS) */

/*
 * wxRadioButton style flag
 */
#define wxRB_GROUP          0x0004
#define wxRB_SINGLE         0x0008
#define wxRB_USE_CHECKBOX   0x0010 /* alternative native control (wxPalmOS) */

/*
 * wxScrollBar flags
 */
#define wxSB_HORIZONTAL      wxHORIZONTAL
#define wxSB_VERTICAL        wxVERTICAL

/*
 * wxSpinButton flags.
 * Note that a wxSpinCtrl is sometimes defined as
 * a wxTextCtrl, and so the flags must be different
 * from wxTextCtrl's.
 */
#define wxSP_HORIZONTAL       wxHORIZONTAL /*  4 */
#define wxSP_VERTICAL         wxVERTICAL   /*  8 */
#define wxSP_ARROW_KEYS       0x1000
#define wxSP_WRAP             0x2000

/*
 * wxTabCtrl flags
 */
#define wxTC_RIGHTJUSTIFY     0x0010
#define wxTC_FIXEDWIDTH       0x0020
#define wxTC_TOP              0x0000    /*  default */
#define wxTC_LEFT             0x0020
#define wxTC_RIGHT            0x0040
#define wxTC_BOTTOM           0x0080
#define wxTC_MULTILINE        0x0200    /* == wxNB_MULTILINE */
#define wxTC_OWNERDRAW        0x0400

/*
 * wxStatusBar95 flags
 */
#define wxST_SIZEGRIP         0x0010

/*
 * wxStaticText flags
 */
#define wxST_NO_AUTORESIZE    0x0001
#define wxST_DOTS_MIDDLE      0x0002
#define wxST_DOTS_END         0x0004

/*
 * wxStaticBitmap flags
 */
#define wxBI_EXPAND           wxEXPAND

/*
 * wxStaticLine flags
 */
#define wxLI_HORIZONTAL         wxHORIZONTAL
#define wxLI_VERTICAL           wxVERTICAL


/*
 * extended dialog specifiers. these values are stored in a different
 * flag and thus do not overlap with other style flags. note that these
 * values do not correspond to the return values of the dialogs (for
 * those values, look at the wxID_XXX defines).
 */

/*  wxCENTRE already defined as  0x00000001 */
#define wxYES                   0x00000002
#define wxOK                    0x00000004
#define wxNO                    0x00000008
#define wxYES_NO                (wxYES | wxNO)
#define wxCANCEL                0x00000010

#define wxYES_DEFAULT           0x00000000  /*  has no effect (default) */
#define wxNO_DEFAULT            0x00000080

#define wxICON_EXCLAMATION      0x00000100
#define wxICON_HAND             0x00000200
#define wxICON_WARNING          wxICON_EXCLAMATION
#define wxICON_ERROR            wxICON_HAND
#define wxICON_QUESTION         0x00000400
#define wxICON_INFORMATION      0x00000800
#define wxICON_STOP             wxICON_HAND
#define wxICON_ASTERISK         wxICON_INFORMATION
#define wxICON_MASK             (0x00000100|0x00000200|0x00000400|0x00000800)

#define  wxFORWARD              0x00001000
#define  wxBACKWARD             0x00002000
#define  wxRESET                0x00004000
#define  wxHELP                 0x00008000
#define  wxMORE                 0x00010000
#define  wxSETUP                0x00020000

/*
 * Background styles. See wxWindow::SetBackgroundStyle
 */

enum wxBackgroundStyle
{
  wxBG_STYLE_SYSTEM,
  wxBG_STYLE_COLOUR,
  wxBG_STYLE_CUSTOM
};

/*  ---------------------------------------------------------------------------- */
/*  standard IDs */
/*  ---------------------------------------------------------------------------- */

/*  Standard menu IDs */
enum
{
    /* no id matches this one when compared to it */
    wxID_NONE = -3,

    /*  id for a separator line in the menu (invalid for normal item) */
    wxID_SEPARATOR = -2,

    /* any id: means that we don't care about the id, whether when installing
     * an event handler or when creating a new window */
    wxID_ANY = -1,


    /* all predefined ids are between wxID_LOWEST and wxID_HIGHEST */
    wxID_LOWEST = 4999,

    wxID_OPEN,
    wxID_CLOSE,
    wxID_NEW,
    wxID_SAVE,
    wxID_SAVEAS,
    wxID_REVERT,
    wxID_EXIT,
    wxID_UNDO,
    wxID_REDO,
    wxID_HELP,
    wxID_PRINT,
    wxID_PRINT_SETUP,
    wxID_PAGE_SETUP,
    wxID_PREVIEW,
    wxID_ABOUT,
    wxID_HELP_CONTENTS,
    wxID_HELP_INDEX,
    wxID_HELP_SEARCH,
    wxID_HELP_COMMANDS,
    wxID_HELP_PROCEDURES,
    wxID_HELP_CONTEXT,
    wxID_CLOSE_ALL,
    wxID_PREFERENCES,

    wxID_EDIT = 5030,
    wxID_CUT,
    wxID_COPY,
    wxID_PASTE,
    wxID_CLEAR,
    wxID_FIND,
    wxID_DUPLICATE,
    wxID_SELECTALL,
    wxID_DELETE,
    wxID_REPLACE,
    wxID_REPLACE_ALL,
    wxID_PROPERTIES,

    wxID_VIEW_DETAILS,
    wxID_VIEW_LARGEICONS,
    wxID_VIEW_SMALLICONS,
    wxID_VIEW_LIST,
    wxID_VIEW_SORTDATE,
    wxID_VIEW_SORTNAME,
    wxID_VIEW_SORTSIZE,
    wxID_VIEW_SORTTYPE,

    wxID_FILE = 5050,
    wxID_FILE1,
    wxID_FILE2,
    wxID_FILE3,
    wxID_FILE4,
    wxID_FILE5,
    wxID_FILE6,
    wxID_FILE7,
    wxID_FILE8,
    wxID_FILE9,

    /*  Standard button and menu IDs */
    wxID_OK = 5100,
    wxID_CANCEL,
    wxID_APPLY,
    wxID_YES,
    wxID_NO,
    wxID_STATIC,
    wxID_FORWARD,
    wxID_BACKWARD,
    wxID_DEFAULT,
    wxID_MORE,
    wxID_SETUP,
    wxID_RESET,
    wxID_CONTEXT_HELP,
    wxID_YESTOALL,
    wxID_NOTOALL,
    wxID_ABORT,
    wxID_RETRY,
    wxID_IGNORE,
    wxID_ADD,
    wxID_REMOVE,

    wxID_UP,
    wxID_DOWN,
    wxID_HOME,
    wxID_REFRESH,
    wxID_STOP,
    wxID_INDEX,

    wxID_BOLD,
    wxID_ITALIC,
    wxID_JUSTIFY_CENTER,
    wxID_JUSTIFY_FILL,
    wxID_JUSTIFY_RIGHT,
    wxID_JUSTIFY_LEFT,
    wxID_UNDERLINE,
    wxID_INDENT,
    wxID_UNINDENT,
    wxID_ZOOM_100,
    wxID_ZOOM_FIT,
    wxID_ZOOM_IN,
    wxID_ZOOM_OUT,
    wxID_UNDELETE,
    wxID_REVERT_TO_SAVED,

    /*  System menu IDs (used by wxUniv): */
    wxID_SYSTEM_MENU = 5200,
    wxID_CLOSE_FRAME,
    wxID_MOVE_FRAME,
    wxID_RESIZE_FRAME,
    wxID_MAXIMIZE_FRAME,
    wxID_ICONIZE_FRAME,
    wxID_RESTORE_FRAME,

    /*  IDs used by generic file dialog (13 consecutive starting from this value) */
    wxID_FILEDLGG = 5900,

    wxID_HIGHEST = 5999
};

/*  ---------------------------------------------------------------------------- */
/*  other constants */
/*  ---------------------------------------------------------------------------- */

/*  menu and toolbar item kinds */
enum wxItemKind
{
    wxITEM_SEPARATOR = -1,
    wxITEM_NORMAL,
    wxITEM_CHECK,
    wxITEM_RADIO,
    wxITEM_MAX
};

/*  hit test results */
enum wxHitTest
{
    wxHT_NOWHERE,

    /*  scrollbar */
    wxHT_SCROLLBAR_FIRST = wxHT_NOWHERE,
    wxHT_SCROLLBAR_ARROW_LINE_1,    /*  left or upper arrow to scroll by line */
    wxHT_SCROLLBAR_ARROW_LINE_2,    /*  right or down */
    wxHT_SCROLLBAR_ARROW_PAGE_1,    /*  left or upper arrow to scroll by page */
    wxHT_SCROLLBAR_ARROW_PAGE_2,    /*  right or down */
    wxHT_SCROLLBAR_THUMB,           /*  on the thumb */
    wxHT_SCROLLBAR_BAR_1,           /*  bar to the left/above the thumb */
    wxHT_SCROLLBAR_BAR_2,           /*  bar to the right/below the thumb */
    wxHT_SCROLLBAR_LAST,

    /*  window */
    wxHT_WINDOW_OUTSIDE,            /*  not in this window at all */
    wxHT_WINDOW_INSIDE,             /*  in the client area */
    wxHT_WINDOW_VERT_SCROLLBAR,     /*  on the vertical scrollbar */
    wxHT_WINDOW_HORZ_SCROLLBAR,     /*  on the horizontal scrollbar */
    wxHT_WINDOW_CORNER,             /*  on the corner between 2 scrollbars */

    wxHT_MAX
};

/*  ---------------------------------------------------------------------------- */
/*  Possible SetSize flags */
/*  ---------------------------------------------------------------------------- */

/*  Use internally-calculated width if -1 */
#define wxSIZE_AUTO_WIDTH       0x0001
/*  Use internally-calculated height if -1 */
#define wxSIZE_AUTO_HEIGHT      0x0002
/*  Use internally-calculated width and height if each is -1 */
#define wxSIZE_AUTO             (wxSIZE_AUTO_WIDTH|wxSIZE_AUTO_HEIGHT)
/*  Ignore missing (-1) dimensions (use existing). */
/*  For readability only: test for wxSIZE_AUTO_WIDTH/HEIGHT in code. */
#define wxSIZE_USE_EXISTING     0x0000
/*  Allow -1 as a valid position */
#define wxSIZE_ALLOW_MINUS_ONE  0x0004
/*  Don't do parent client adjustments (for implementation only) */
#define wxSIZE_NO_ADJUSTMENTS   0x0008
/*  Change the window position even if it seems to be already correct */
#define wxSIZE_FORCE            0x0010

/*  ---------------------------------------------------------------------------- */
/*  GDI descriptions */
/*  ---------------------------------------------------------------------------- */

enum
{
    /*  Text font families */
    wxDEFAULT    = 70,
    wxDECORATIVE,
    wxROMAN,
    wxSCRIPT,
    wxSWISS,
    wxMODERN,
    wxTELETYPE,  /* @@@@ */

    /*  Proportional or Fixed width fonts (not yet used) */
    wxVARIABLE   = 80,
    wxFIXED,

    wxNORMAL     = 90,
    wxLIGHT,
    wxBOLD,
    /*  Also wxNORMAL for normal (non-italic text) */
    wxITALIC,
    wxSLANT,

    /*  Pen styles */
    wxSOLID      =   100,
    wxDOT,
    wxLONG_DASH,
    wxSHORT_DASH,
    wxDOT_DASH,
    wxUSER_DASH,

    wxTRANSPARENT,

    /*  Brush & Pen Stippling. Note that a stippled pen cannot be dashed!! */
    /*  Note also that stippling a Pen IS meaningfull, because a Line is */
    wxSTIPPLE_MASK_OPAQUE, /* mask is used for blitting monochrome using text fore and back ground colors */
    wxSTIPPLE_MASK,        /* mask is used for masking areas in the stipple bitmap (TO DO) */
    /*  drawn with a Pen, and without any Brush -- and it can be stippled. */
    wxSTIPPLE =          110,

    wxBDIAGONAL_HATCH,     /* In wxWidgets < 2.6 use WX_HATCH macro  */
    wxCROSSDIAG_HATCH,     /* to verify these wx*_HATCH are in style */
    wxFDIAGONAL_HATCH,     /* of wxBrush. In wxWidgets >= 2.6 use    */
    wxCROSS_HATCH,         /* wxBrush::IsHatch() instead.            */
    wxHORIZONTAL_HATCH,
    wxVERTICAL_HATCH,
    wxFIRST_HATCH = wxBDIAGONAL_HATCH,
    wxLAST_HATCH = wxVERTICAL_HATCH,

    wxJOIN_BEVEL =     120,
    wxJOIN_MITER,
    wxJOIN_ROUND,

    wxCAP_ROUND =      130,
    wxCAP_PROJECTING,
    wxCAP_BUTT
};

#if WXWIN_COMPATIBILITY_2_4
    #define IS_HATCH(s)    ((s)>=wxFIRST_HATCH && (s)<=wxLAST_HATCH)
#else
    /* use wxBrush::IsHatch() instead thought wxMotif still uses it in src/motif/dcclient.cpp */
#endif

/*  Logical ops */
typedef enum
{
    wxCLEAR,        wxROP_BLACK = wxCLEAR,             wxBLIT_BLACKNESS = wxCLEAR,        /*  0 */
    wxXOR,          wxROP_XORPEN = wxXOR,              wxBLIT_SRCINVERT = wxXOR,          /*  src XOR dst */
    wxINVERT,       wxROP_NOT = wxINVERT,              wxBLIT_DSTINVERT = wxINVERT,       /*  NOT dst */
    wxOR_REVERSE,   wxROP_MERGEPENNOT = wxOR_REVERSE,  wxBLIT_00DD0228 = wxOR_REVERSE,    /*  src OR (NOT dst) */
    wxAND_REVERSE,  wxROP_MASKPENNOT = wxAND_REVERSE,  wxBLIT_SRCERASE = wxAND_REVERSE,   /*  src AND (NOT dst) */
    wxCOPY,         wxROP_COPYPEN = wxCOPY,            wxBLIT_SRCCOPY = wxCOPY,           /*  src */
    wxAND,          wxROP_MASKPEN = wxAND,             wxBLIT_SRCAND = wxAND,             /*  src AND dst */
    wxAND_INVERT,   wxROP_MASKNOTPEN = wxAND_INVERT,   wxBLIT_00220326 = wxAND_INVERT,    /*  (NOT src) AND dst */
    wxNO_OP,        wxROP_NOP = wxNO_OP,               wxBLIT_00AA0029 = wxNO_OP,         /*  dst */
    wxNOR,          wxROP_NOTMERGEPEN = wxNOR,         wxBLIT_NOTSRCERASE = wxNOR,        /*  (NOT src) AND (NOT dst) */
    wxEQUIV,        wxROP_NOTXORPEN = wxEQUIV,         wxBLIT_00990066 = wxEQUIV,         /*  (NOT src) XOR dst */
    wxSRC_INVERT,   wxROP_NOTCOPYPEN = wxSRC_INVERT,   wxBLIT_NOTSCRCOPY = wxSRC_INVERT,  /*  (NOT src) */
    wxOR_INVERT,    wxROP_MERGENOTPEN = wxOR_INVERT,   wxBLIT_MERGEPAINT = wxOR_INVERT,   /*  (NOT src) OR dst */
    wxNAND,         wxROP_NOTMASKPEN = wxNAND,         wxBLIT_007700E6 = wxNAND,          /*  (NOT src) OR (NOT dst) */
    wxOR,           wxROP_MERGEPEN = wxOR,             wxBLIT_SRCPAINT = wxOR,            /*  src OR dst */
    wxSET,          wxROP_WHITE = wxSET,               wxBLIT_WHITENESS = wxSET           /*  1 */
} form_ops_t;

/*  Flood styles */
enum
{
    wxFLOOD_SURFACE = 1,
    wxFLOOD_BORDER
};

/*  Polygon filling mode */
enum
{
    wxODDEVEN_RULE = 1,
    wxWINDING_RULE
};

/*  ToolPanel in wxFrame (VZ: unused?) */
enum
{
    wxTOOL_TOP = 1,
    wxTOOL_BOTTOM,
    wxTOOL_LEFT,
    wxTOOL_RIGHT
};

/*  the values of the format constants should be the same as corresponding */
/*  CF_XXX constants in Windows API */
enum wxDataFormatId
{
    wxDF_INVALID =          0,
    wxDF_TEXT =             1,  /* CF_TEXT */
    wxDF_BITMAP =           2,  /* CF_BITMAP */
    wxDF_METAFILE =         3,  /* CF_METAFILEPICT */
    wxDF_SYLK =             4,
    wxDF_DIF =              5,
    wxDF_TIFF =             6,
    wxDF_OEMTEXT =          7,  /* CF_OEMTEXT */
    wxDF_DIB =              8,  /* CF_DIB */
    wxDF_PALETTE =          9,
    wxDF_PENDATA =          10,
    wxDF_RIFF =             11,
    wxDF_WAVE =             12,
    wxDF_UNICODETEXT =      13,
    wxDF_ENHMETAFILE =      14,
    wxDF_FILENAME =         15, /* CF_HDROP */
    wxDF_LOCALE =           16,
    wxDF_PRIVATE =          20,
    wxDF_HTML =             30, /* Note: does not correspond to CF_ constant */
    wxDF_MAX
};

/*  Virtual keycodes */
enum wxKeyCode
{
    WXK_BACK    =    8,
    WXK_TAB     =    9,
    WXK_RETURN  =    13,
    WXK_ESCAPE  =    27,
    WXK_SPACE   =    32,
    WXK_DELETE  =    127,

    /* These are, by design, not compatible with unicode characters.
       If you want to get a unicode character from a key event, use
       wxKeyEvent::GetUnicodeKey instead.                           */
    WXK_START   = 300,
    WXK_LBUTTON,
    WXK_RBUTTON,
    WXK_CANCEL,
    WXK_MBUTTON,
    WXK_CLEAR,
    WXK_SHIFT,
    WXK_ALT,
    WXK_CONTROL,
    WXK_MENU,
    WXK_PAUSE,
    WXK_CAPITAL,
    WXK_END,
    WXK_HOME,
    WXK_LEFT,
    WXK_UP,
    WXK_RIGHT,
    WXK_DOWN,
    WXK_SELECT,
    WXK_PRINT,
    WXK_EXECUTE,
    WXK_SNAPSHOT,
    WXK_INSERT,
    WXK_HELP,
    WXK_NUMPAD0,
    WXK_NUMPAD1,
    WXK_NUMPAD2,
    WXK_NUMPAD3,
    WXK_NUMPAD4,
    WXK_NUMPAD5,
    WXK_NUMPAD6,
    WXK_NUMPAD7,
    WXK_NUMPAD8,
    WXK_NUMPAD9,
    WXK_MULTIPLY,
    WXK_ADD,
    WXK_SEPARATOR,
    WXK_SUBTRACT,
    WXK_DECIMAL,
    WXK_DIVIDE,
    WXK_F1,
    WXK_F2,
    WXK_F3,
    WXK_F4,
    WXK_F5,
    WXK_F6,
    WXK_F7,
    WXK_F8,
    WXK_F9,
    WXK_F10,
    WXK_F11,
    WXK_F12,
    WXK_F13,
    WXK_F14,
    WXK_F15,
    WXK_F16,
    WXK_F17,
    WXK_F18,
    WXK_F19,
    WXK_F20,
    WXK_F21,
    WXK_F22,
    WXK_F23,
    WXK_F24,
    WXK_NUMLOCK,
    WXK_SCROLL,
    WXK_PAGEUP,
    WXK_PAGEDOWN,
#if WXWIN_COMPATIBILITY_2_6
    WXK_PRIOR = WXK_PAGEUP,
    WXK_NEXT  = WXK_PAGEDOWN,
#endif

    WXK_NUMPAD_SPACE,
    WXK_NUMPAD_TAB,
    WXK_NUMPAD_ENTER,
    WXK_NUMPAD_F1,
    WXK_NUMPAD_F2,
    WXK_NUMPAD_F3,
    WXK_NUMPAD_F4,
    WXK_NUMPAD_HOME,
    WXK_NUMPAD_LEFT,
    WXK_NUMPAD_UP,
    WXK_NUMPAD_RIGHT,
    WXK_NUMPAD_DOWN,
    WXK_NUMPAD_PAGEUP,
    WXK_NUMPAD_PAGEDOWN,
#if WXWIN_COMPATIBILITY_2_6
    WXK_NUMPAD_PRIOR = WXK_NUMPAD_PAGEUP,
    WXK_NUMPAD_NEXT  = WXK_NUMPAD_PAGEDOWN,
#endif
    WXK_NUMPAD_END,
    WXK_NUMPAD_BEGIN,
    WXK_NUMPAD_INSERT,
    WXK_NUMPAD_DELETE,
    WXK_NUMPAD_EQUAL,
    WXK_NUMPAD_MULTIPLY,
    WXK_NUMPAD_ADD,
    WXK_NUMPAD_SEPARATOR,
    WXK_NUMPAD_SUBTRACT,
    WXK_NUMPAD_DECIMAL,
    WXK_NUMPAD_DIVIDE,

    WXK_WINDOWS_LEFT,
    WXK_WINDOWS_RIGHT,
    WXK_WINDOWS_MENU ,
    WXK_COMMAND,

    /* Hardware-specific buttons */
    WXK_SPECIAL1 = 193,
    WXK_SPECIAL2,
    WXK_SPECIAL3,
    WXK_SPECIAL4,
    WXK_SPECIAL5,
    WXK_SPECIAL6,
    WXK_SPECIAL7,
    WXK_SPECIAL8,
    WXK_SPECIAL9,
    WXK_SPECIAL10,
    WXK_SPECIAL11,
    WXK_SPECIAL12,
    WXK_SPECIAL13,
    WXK_SPECIAL14,
    WXK_SPECIAL15,
    WXK_SPECIAL16,
    WXK_SPECIAL17,
    WXK_SPECIAL18,
    WXK_SPECIAL19,
    WXK_SPECIAL20
};

/* This enum contains bit mask constants used in wxKeyEvent */
enum wxKeyModifier
{
    wxMOD_NONE      = 0x0000,
    wxMOD_ALT       = 0x0001,
    wxMOD_CONTROL   = 0x0002,
    wxMOD_ALTGR     = wxMOD_ALT | wxMOD_CONTROL,
    wxMOD_SHIFT     = 0x0004,
    wxMOD_META      = 0x0008,
    wxMOD_WIN       = wxMOD_META,
#if defined(__WXMAC__) || defined(__WXCOCOA__)
    wxMOD_CMD       = wxMOD_META,
#else
    wxMOD_CMD       = wxMOD_CONTROL,
#endif
    wxMOD_ALL       = 0xffff
};

/*  Mapping modes (same values as used by Windows, don't change) */
enum
{
    wxMM_TEXT = 1,
    wxMM_LOMETRIC,
    wxMM_HIMETRIC,
    wxMM_LOENGLISH,
    wxMM_HIENGLISH,
    wxMM_TWIPS,
    wxMM_ISOTROPIC,
    wxMM_ANISOTROPIC,
    wxMM_POINTS,
    wxMM_METRIC
};

/* Shortcut for easier dialog-unit-to-pixel conversion */
#define wxDLG_UNIT(parent, pt) parent->ConvertDialogToPixels(pt)

/* Paper types */
typedef enum
{
    wxPAPER_NONE,               /*  Use specific dimensions */
    wxPAPER_LETTER,             /*  Letter, 8 1/2 by 11 inches */
    wxPAPER_LEGAL,              /*  Legal, 8 1/2 by 14 inches */
    wxPAPER_A4,                 /*  A4 Sheet, 210 by 297 millimeters */
    wxPAPER_CSHEET,             /*  C Sheet, 17 by 22 inches */
    wxPAPER_DSHEET,             /*  D Sheet, 22 by 34 inches */
    wxPAPER_ESHEET,             /*  E Sheet, 34 by 44 inches */
    wxPAPER_LETTERSMALL,        /*  Letter Small, 8 1/2 by 11 inches */
    wxPAPER_TABLOID,            /*  Tabloid, 11 by 17 inches */
    wxPAPER_LEDGER,             /*  Ledger, 17 by 11 inches */
    wxPAPER_STATEMENT,          /*  Statement, 5 1/2 by 8 1/2 inches */
    wxPAPER_EXECUTIVE,          /*  Executive, 7 1/4 by 10 1/2 inches */
    wxPAPER_A3,                 /*  A3 sheet, 297 by 420 millimeters */
    wxPAPER_A4SMALL,            /*  A4 small sheet, 210 by 297 millimeters */
    wxPAPER_A5,                 /*  A5 sheet, 148 by 210 millimeters */
    wxPAPER_B4,                 /*  B4 sheet, 250 by 354 millimeters */
    wxPAPER_B5,                 /*  B5 sheet, 182-by-257-millimeter paper */
    wxPAPER_FOLIO,              /*  Folio, 8-1/2-by-13-inch paper */
    wxPAPER_QUARTO,             /*  Quarto, 215-by-275-millimeter paper */
    wxPAPER_10X14,              /*  10-by-14-inch sheet */
    wxPAPER_11X17,              /*  11-by-17-inch sheet */
    wxPAPER_NOTE,               /*  Note, 8 1/2 by 11 inches */
    wxPAPER_ENV_9,              /*  #9 Envelope, 3 7/8 by 8 7/8 inches */
    wxPAPER_ENV_10,             /*  #10 Envelope, 4 1/8 by 9 1/2 inches */
    wxPAPER_ENV_11,             /*  #11 Envelope, 4 1/2 by 10 3/8 inches */
    wxPAPER_ENV_12,             /*  #12 Envelope, 4 3/4 by 11 inches */
    wxPAPER_ENV_14,             /*  #14 Envelope, 5 by 11 1/2 inches */
    wxPAPER_ENV_DL,             /*  DL Envelope, 110 by 220 millimeters */
    wxPAPER_ENV_C5,             /*  C5 Envelope, 162 by 229 millimeters */
    wxPAPER_ENV_C3,             /*  C3 Envelope, 324 by 458 millimeters */
    wxPAPER_ENV_C4,             /*  C4 Envelope, 229 by 324 millimeters */
    wxPAPER_ENV_C6,             /*  C6 Envelope, 114 by 162 millimeters */
    wxPAPER_ENV_C65,            /*  C65 Envelope, 114 by 229 millimeters */
    wxPAPER_ENV_B4,             /*  B4 Envelope, 250 by 353 millimeters */
    wxPAPER_ENV_B5,             /*  B5 Envelope, 176 by 250 millimeters */
    wxPAPER_ENV_B6,             /*  B6 Envelope, 176 by 125 millimeters */
    wxPAPER_ENV_ITALY,          /*  Italy Envelope, 110 by 230 millimeters */
    wxPAPER_ENV_MONARCH,        /*  Monarch Envelope, 3 7/8 by 7 1/2 inches */
    wxPAPER_ENV_PERSONAL,       /*  6 3/4 Envelope, 3 5/8 by 6 1/2 inches */
    wxPAPER_FANFOLD_US,         /*  US Std Fanfold, 14 7/8 by 11 inches */
    wxPAPER_FANFOLD_STD_GERMAN, /*  German Std Fanfold, 8 1/2 by 12 inches */
    wxPAPER_FANFOLD_LGL_GERMAN, /*  German Legal Fanfold, 8 1/2 by 13 inches */

    wxPAPER_ISO_B4,             /*  B4 (ISO) 250 x 353 mm */
    wxPAPER_JAPANESE_POSTCARD,  /*  Japanese Postcard 100 x 148 mm */
    wxPAPER_9X11,               /*  9 x 11 in */
    wxPAPER_10X11,              /*  10 x 11 in */
    wxPAPER_15X11,              /*  15 x 11 in */
    wxPAPER_ENV_INVITE,         /*  Envelope Invite 220 x 220 mm */
    wxPAPER_LETTER_EXTRA,       /*  Letter Extra 9 \275 x 12 in */
    wxPAPER_LEGAL_EXTRA,        /*  Legal Extra 9 \275 x 15 in */
    wxPAPER_TABLOID_EXTRA,      /*  Tabloid Extra 11.69 x 18 in */
    wxPAPER_A4_EXTRA,           /*  A4 Extra 9.27 x 12.69 in */
    wxPAPER_LETTER_TRANSVERSE,  /*  Letter Transverse 8 \275 x 11 in */
    wxPAPER_A4_TRANSVERSE,      /*  A4 Transverse 210 x 297 mm */
    wxPAPER_LETTER_EXTRA_TRANSVERSE, /*  Letter Extra Transverse 9\275 x 12 in */
    wxPAPER_A_PLUS,             /*  SuperA/SuperA/A4 227 x 356 mm */
    wxPAPER_B_PLUS,             /*  SuperB/SuperB/A3 305 x 487 mm */
    wxPAPER_LETTER_PLUS,        /*  Letter Plus 8.5 x 12.69 in */
    wxPAPER_A4_PLUS,            /*  A4 Plus 210 x 330 mm */
    wxPAPER_A5_TRANSVERSE,      /*  A5 Transverse 148 x 210 mm */
    wxPAPER_B5_TRANSVERSE,      /*  B5 (JIS) Transverse 182 x 257 mm */
    wxPAPER_A3_EXTRA,           /*  A3 Extra 322 x 445 mm */
    wxPAPER_A5_EXTRA,           /*  A5 Extra 174 x 235 mm */
    wxPAPER_B5_EXTRA,           /*  B5 (ISO) Extra 201 x 276 mm */
    wxPAPER_A2,                 /*  A2 420 x 594 mm */
    wxPAPER_A3_TRANSVERSE,      /*  A3 Transverse 297 x 420 mm */
    wxPAPER_A3_EXTRA_TRANSVERSE, /*  A3 Extra Transverse 322 x 445 mm */

    wxPAPER_DBL_JAPANESE_POSTCARD,/* Japanese Double Postcard 200 x 148 mm */
    wxPAPER_A6,                 /* A6 105 x 148 mm */
    wxPAPER_JENV_KAKU2,         /* Japanese Envelope Kaku #2 */
    wxPAPER_JENV_KAKU3,         /* Japanese Envelope Kaku #3 */
    wxPAPER_JENV_CHOU3,         /* Japanese Envelope Chou #3 */
    wxPAPER_JENV_CHOU4,         /* Japanese Envelope Chou #4 */
    wxPAPER_LETTER_ROTATED,     /* Letter Rotated 11 x 8 1/2 in */
    wxPAPER_A3_ROTATED,         /* A3 Rotated 420 x 297 mm */
    wxPAPER_A4_ROTATED,         /* A4 Rotated 297 x 210 mm */
    wxPAPER_A5_ROTATED,         /* A5 Rotated 210 x 148 mm */
    wxPAPER_B4_JIS_ROTATED,     /* B4 (JIS) Rotated 364 x 257 mm */
    wxPAPER_B5_JIS_ROTATED,     /* B5 (JIS) Rotated 257 x 182 mm */
    wxPAPER_JAPANESE_POSTCARD_ROTATED,/* Japanese Postcard Rotated 148 x 100 mm */
    wxPAPER_DBL_JAPANESE_POSTCARD_ROTATED,/* Double Japanese Postcard Rotated 148 x 200 mm */
    wxPAPER_A6_ROTATED,         /* A6 Rotated 148 x 105 mm */
    wxPAPER_JENV_KAKU2_ROTATED, /* Japanese Envelope Kaku #2 Rotated */
    wxPAPER_JENV_KAKU3_ROTATED, /* Japanese Envelope Kaku #3 Rotated */
    wxPAPER_JENV_CHOU3_ROTATED, /* Japanese Envelope Chou #3 Rotated */
    wxPAPER_JENV_CHOU4_ROTATED, /* Japanese Envelope Chou #4 Rotated */
    wxPAPER_B6_JIS,             /* B6 (JIS) 128 x 182 mm */
    wxPAPER_B6_JIS_ROTATED,     /* B6 (JIS) Rotated 182 x 128 mm */
    wxPAPER_12X11,              /* 12 x 11 in */
    wxPAPER_JENV_YOU4,          /* Japanese Envelope You #4 */
    wxPAPER_JENV_YOU4_ROTATED,  /* Japanese Envelope You #4 Rotated */
    wxPAPER_P16K,               /* PRC 16K 146 x 215 mm */
    wxPAPER_P32K,               /* PRC 32K 97 x 151 mm */
    wxPAPER_P32KBIG,            /* PRC 32K(Big) 97 x 151 mm */
    wxPAPER_PENV_1,             /* PRC Envelope #1 102 x 165 mm */
    wxPAPER_PENV_2,             /* PRC Envelope #2 102 x 176 mm */
    wxPAPER_PENV_3,             /* PRC Envelope #3 125 x 176 mm */
    wxPAPER_PENV_4,             /* PRC Envelope #4 110 x 208 mm */
    wxPAPER_PENV_5,             /* PRC Envelope #5 110 x 220 mm */
    wxPAPER_PENV_6,             /* PRC Envelope #6 120 x 230 mm */
    wxPAPER_PENV_7,             /* PRC Envelope #7 160 x 230 mm */
    wxPAPER_PENV_8,             /* PRC Envelope #8 120 x 309 mm */
    wxPAPER_PENV_9,             /* PRC Envelope #9 229 x 324 mm */
    wxPAPER_PENV_10,            /* PRC Envelope #10 324 x 458 mm */
    wxPAPER_P16K_ROTATED,       /* PRC 16K Rotated */
    wxPAPER_P32K_ROTATED,       /* PRC 32K Rotated */
    wxPAPER_P32KBIG_ROTATED,    /* PRC 32K(Big) Rotated */
    wxPAPER_PENV_1_ROTATED,     /* PRC Envelope #1 Rotated 165 x 102 mm */
    wxPAPER_PENV_2_ROTATED,     /* PRC Envelope #2 Rotated 176 x 102 mm */
    wxPAPER_PENV_3_ROTATED,     /* PRC Envelope #3 Rotated 176 x 125 mm */
    wxPAPER_PENV_4_ROTATED,     /* PRC Envelope #4 Rotated 208 x 110 mm */
    wxPAPER_PENV_5_ROTATED,     /* PRC Envelope #5 Rotated 220 x 110 mm */
    wxPAPER_PENV_6_ROTATED,     /* PRC Envelope #6 Rotated 230 x 120 mm */
    wxPAPER_PENV_7_ROTATED,     /* PRC Envelope #7 Rotated 230 x 160 mm */
    wxPAPER_PENV_8_ROTATED,     /* PRC Envelope #8 Rotated 309 x 120 mm */
    wxPAPER_PENV_9_ROTATED,     /* PRC Envelope #9 Rotated 324 x 229 mm */
    wxPAPER_PENV_10_ROTATED    /* PRC Envelope #10 Rotated 458 x 324 m */
} wxPaperSize;

/* Printing orientation */
#ifndef wxPORTRAIT
#define wxPORTRAIT      1
#define wxLANDSCAPE     2
#endif

/* Duplex printing modes
 */

enum wxDuplexMode
{
    wxDUPLEX_SIMPLEX, /*  Non-duplex */
    wxDUPLEX_HORIZONTAL,
    wxDUPLEX_VERTICAL
};

/* Print quality.
 */

#define wxPRINT_QUALITY_HIGH    -1
#define wxPRINT_QUALITY_MEDIUM  -2
#define wxPRINT_QUALITY_LOW     -3
#define wxPRINT_QUALITY_DRAFT   -4

typedef int wxPrintQuality;

/* Print mode (currently PostScript only)
 */

enum wxPrintMode
{
    wxPRINT_MODE_NONE =    0,
    wxPRINT_MODE_PREVIEW = 1,   /*  Preview in external application */
    wxPRINT_MODE_FILE =    2,   /*  Print to file */
    wxPRINT_MODE_PRINTER = 3,   /*  Send to printer */
    wxPRINT_MODE_STREAM =  4    /*  Send postscript data into a stream */
};

/*  ---------------------------------------------------------------------------- */
/*  UpdateWindowUI flags */
/*  ---------------------------------------------------------------------------- */

enum wxUpdateUI
{
    wxUPDATE_UI_NONE          = 0x0000,
    wxUPDATE_UI_RECURSE       = 0x0001,
    wxUPDATE_UI_FROMIDLE      = 0x0002 /*  Invoked from On(Internal)Idle */
};

/*  ---------------------------------------------------------------------------- */
/*  Notification Event flags - used for dock icon bouncing, etc. */
/*  ---------------------------------------------------------------------------- */

enum wxNotificationOptions
{
    wxNOTIFY_NONE           = 0x0000,
    wxNOTIFY_ONCE           = 0x0001,
    wxNOTIFY_REPEAT         = 0x0002
};

/*  ---------------------------------------------------------------------------- */
/*  miscellaneous */
/*  ---------------------------------------------------------------------------- */

/*  define this macro if font handling is done using the X font names */
#if (defined(__WXGTK__) && !defined(__WXGTK20__)) || defined(__X__)
    #define _WX_X_FONTLIKE
#endif

/*  macro to specify "All Files" on different platforms */
#if defined(__WXMSW__) || defined(__WXPM__)
#   define wxALL_FILES_PATTERN   wxT("*.*")
#   define wxALL_FILES           gettext_noop("All files (*.*)|*.*")
#else
#   define wxALL_FILES_PATTERN   wxT("*")
#   define wxALL_FILES           gettext_noop("All files (*)|*")
#endif

#if defined(__CYGWIN__) && defined(__WXMSW__)
#   if wxUSE_STL || defined(wxUSE_STD_STRING)
         /*
            NASTY HACK because the gethostname in sys/unistd.h which the gnu
            stl includes and wx builds with by default clash with each other
            (windows version 2nd param is int, sys/unistd.h version is unsigned
            int).
          */
#        define gethostname gethostnameHACK
#        include <unistd.h>
#        undef gethostname
#   endif
#endif

/*  --------------------------------------------------------------------------- */
/*  macros that enable wxWidgets apps to be compiled in absence of the */
/*  sytem headers, although some platform specific types are used in the */
/*  platform specific (implementation) parts of the headers */
/*  --------------------------------------------------------------------------- */

#ifdef __WXMAC__

#define WX_OPAQUE_TYPE( name ) struct wxOpaque##name

typedef unsigned char WXCOLORREF[6];
typedef void*       WXCGIMAGEREF;
typedef void*       WXHBITMAP;
typedef void*       WXHCURSOR;
typedef void*       WXHRGN;
typedef void*       WXRECTPTR;
typedef void*       WXPOINTPTR;
typedef void*       WXHWND;
typedef void*       WXEVENTREF;
typedef void*       WXEVENTHANDLERREF;
typedef void*       WXEVENTHANDLERCALLREF;
typedef void*       WXAPPLEEVENTREF;
typedef void*       WXHDC;
typedef void*       WXHMENU;
typedef unsigned int    WXUINT;
typedef unsigned long   WXDWORD;
typedef unsigned short  WXWORD;

typedef WX_OPAQUE_TYPE(CIconHandle ) * WXHICON ;
typedef WX_OPAQUE_TYPE(PicHandle ) * WXHMETAFILE ;


/* typedef void*       WXWidget; */
/* typedef void*       WXWindow; */
typedef WX_OPAQUE_TYPE(ControlRef ) * WXWidget ;
typedef WX_OPAQUE_TYPE(WindowRef) * WXWindow ;
typedef void*       WXDisplay;

/* typedef WindowPtr       WXHWND; */
/* typedef Handle          WXHANDLE; */
/* typedef CIconHandle     WXHICON; */
/* typedef unsigned long   WXHFONT; */
/* typedef MenuHandle      WXHMENU; */
/* typedef unsigned long   WXHPEN; */
/* typedef unsigned long   WXHBRUSH; */
/* typedef unsigned long   WXHPALETTE; */
/* typedef CursHandle      WXHCURSOR; */
/* typedef RgnHandle       WXHRGN; */
/* typedef unsigned long   WXHACCEL; */
/* typedef unsigned long   WXHINSTANCE; */
/* typedef unsigned long   WXHIMAGELIST; */
/* typedef unsigned long   WXHGLOBAL; */
/* typedef GrafPtr         WXHDC; */
/* typedef unsigned int    WXWPARAM; */
/* typedef long            WXLPARAM; */
/* typedef void *          WXRGNDATA; */
/* typedef void *          WXMSG; */
/* typedef unsigned long   WXHCONV; */
/* typedef unsigned long   WXHKEY; */
/* typedef void *          WXDRAWITEMSTRUCT; */
/* typedef void *          WXMEASUREITEMSTRUCT; */
/* typedef void *          WXLPCREATESTRUCT; */
/* typedef int (*WXFARPROC)(); */

/* typedef WindowPtr       WXWindow; */
/* typedef ControlHandle   WXWidget; */

#endif

#if defined( __WXCOCOA__ ) || ( defined(__WXMAC__) && defined(__DARWIN__) )

/* Definitions of 32-bit/64-bit types
 * These are typedef'd exactly the same way in newer OS X headers so
 * redefinition when real headers are included should not be a problem.  If
 * it is, the types are being defined wrongly here.
 * The purpose of these types is so they can be used from public wx headers.
 * and also because the older (pre-Leopard) headers don't define them.
 */

/* NOTE: We don't pollute namespace with CGFLOAT_MIN/MAX/IS_DOUBLE macros
 * since they are unlikely to be needed in a public header.
 */
#if defined(__LP64__) && __LP64__
    typedef double CGFloat;
#else
    typedef float CGFloat;
#endif

#if (defined(__LP64__) && __LP64__) || (defined(NS_BUILD_32_LIKE_64) && NS_BUILD_32_LIKE_64)
typedef long NSInteger;
typedef unsigned long NSUInteger;
#else
typedef int NSInteger;
typedef unsigned int NSUInteger;
#endif

/* Objective-C type declarations.
 * These are to be used in public headers in lieu of NSSomething* because
 * Objective-C class names are not available in C/C++ code.
 */

/*  NOTE: This ought to work with other compilers too, but I'm being cautious */
#if (defined(__GNUC__) && defined(__APPLE__)) || defined(__MWERKS__)
/* It's desirable to have type safety for Objective-C(++) code as it does
at least catch typos of method names among other things.  However, it
is not possible to declare an Objective-C class from plain old C or C++
code.  Furthermore, because of C++ name mangling, the type name must
be the same for both C++ and Objective-C++ code.  Therefore, we define
what should be a pointer to an Objective-C class as a pointer to a plain
old C struct with the same name.  Unfortunately, because the compiler
does not see a struct as an Objective-C class we cannot declare it
as a struct in Objective-C(++) mode.
*/
#if defined(__OBJC__)
#define DECLARE_WXCOCOA_OBJC_CLASS(klass) \
@class klass; \
typedef klass *WX_##klass
#else /*  not defined(__OBJC__) */
#define DECLARE_WXCOCOA_OBJC_CLASS(klass) \
typedef struct klass *WX_##klass
#endif /*  defined(__OBJC__) */

#else /*  not Apple's GNU or CodeWarrior */
#warning "Objective-C types will not be checked by the compiler."
/*  NOTE: typedef struct objc_object *id; */
/*  IOW, we're declaring these using the id type without using that name, */
/*  since "id" is used extensively not only within wxWidgets itself, but */
/*  also in wxWidgets application code.  The following works fine when */
/*  compiling C(++) code, and works without typesafety for Obj-C(++) code */
#define DECLARE_WXCOCOA_OBJC_CLASS(klass) \
typedef struct objc_object *WX_##klass

#endif /*  (defined(__GNUC__) && defined(__APPLE__)) || defined(__MWERKS__) */

DECLARE_WXCOCOA_OBJC_CLASS(NSApplication);
DECLARE_WXCOCOA_OBJC_CLASS(NSBitmapImageRep);
DECLARE_WXCOCOA_OBJC_CLASS(NSBox);
DECLARE_WXCOCOA_OBJC_CLASS(NSButton);
DECLARE_WXCOCOA_OBJC_CLASS(NSColor);
DECLARE_WXCOCOA_OBJC_CLASS(NSColorPanel);
DECLARE_WXCOCOA_OBJC_CLASS(NSControl);
DECLARE_WXCOCOA_OBJC_CLASS(NSCursor);
DECLARE_WXCOCOA_OBJC_CLASS(NSEvent);
DECLARE_WXCOCOA_OBJC_CLASS(NSFontPanel);
DECLARE_WXCOCOA_OBJC_CLASS(NSImage);
DECLARE_WXCOCOA_OBJC_CLASS(NSLayoutManager);
DECLARE_WXCOCOA_OBJC_CLASS(NSMenu);
DECLARE_WXCOCOA_OBJC_CLASS(NSMenuExtra);
DECLARE_WXCOCOA_OBJC_CLASS(NSMenuItem);
DECLARE_WXCOCOA_OBJC_CLASS(NSMutableArray);
DECLARE_WXCOCOA_OBJC_CLASS(NSNotification);
DECLARE_WXCOCOA_OBJC_CLASS(NSObject);
DECLARE_WXCOCOA_OBJC_CLASS(NSPanel);
DECLARE_WXCOCOA_OBJC_CLASS(NSScrollView);
DECLARE_WXCOCOA_OBJC_CLASS(NSSound);
DECLARE_WXCOCOA_OBJC_CLASS(NSStatusItem);
DECLARE_WXCOCOA_OBJC_CLASS(NSTableColumn);
DECLARE_WXCOCOA_OBJC_CLASS(NSTableView);
DECLARE_WXCOCOA_OBJC_CLASS(NSTextContainer);
DECLARE_WXCOCOA_OBJC_CLASS(NSTextField);
DECLARE_WXCOCOA_OBJC_CLASS(NSTextStorage);
DECLARE_WXCOCOA_OBJC_CLASS(NSThread);
DECLARE_WXCOCOA_OBJC_CLASS(NSWindow);
DECLARE_WXCOCOA_OBJC_CLASS(NSView);
#ifdef __WXMAC__
// things added for __WXMAC__
DECLARE_WXCOCOA_OBJC_CLASS(NSString);
#else
// things only for __WXCOCOA__
typedef WX_NSView WXWidget; /*  wxWidgets BASE definition */
#endif
#endif /*  __WXCOCOA__  || ( __WXMAC__ &__DARWIN__)*/

#if defined(__WXPALMOS__)

typedef void *          WXHWND;
typedef void *          WXHANDLE;
typedef void *          WXHICON;
typedef void *          WXHFONT;
typedef void *          WXHMENU;
typedef void *          WXHPEN;
typedef void *          WXHBRUSH;
typedef void *          WXHPALETTE;
typedef void *          WXHCURSOR;
typedef void *          WXHRGN;
typedef void *          WXHACCEL;
typedef void *          WXHINSTANCE;
typedef void *          WXHBITMAP;
typedef void *          WXHIMAGELIST;
typedef void *          WXHGLOBAL;
typedef void *          WXHDC;
typedef unsigned int    WXUINT;
typedef unsigned long   WXDWORD;
typedef unsigned short  WXWORD;

typedef unsigned long   WXCOLORREF;
typedef struct tagMSG   WXMSG;

typedef WXHWND          WXWINHANDLE; /* WinHandle of PalmOS */
typedef WXWINHANDLE     WXWidget;

typedef void *          WXFORMPTR;
typedef void *          WXEVENTPTR;
typedef void *          WXRECTANGLEPTR;

#endif /* __WXPALMOS__ */


/* ABX: check __WIN32__ instead of __WXMSW__ for the same MSWBase in any Win32 port */
#if defined(__WIN32__)

/*  the keywords needed for WinMain() declaration */
#ifndef WXFAR
#    define WXFAR
#endif

/*  Stand-ins for Windows types to avoid #including all of windows.h */
typedef void *          WXHWND;
typedef void *          WXHANDLE;
typedef void *          WXHICON;
typedef void *          WXHFONT;
typedef void *          WXHMENU;
typedef void *          WXHPEN;
typedef void *          WXHBRUSH;
typedef void *          WXHPALETTE;
typedef void *          WXHCURSOR;
typedef void *          WXHRGN;
typedef void *          WXRECTPTR;
typedef void *          WXHACCEL;
typedef void WXFAR  *   WXHINSTANCE;
typedef void *          WXHBITMAP;
typedef void *          WXHIMAGELIST;
typedef void *          WXHGLOBAL;
typedef void *          WXHDC;
typedef unsigned int    WXUINT;
typedef unsigned long   WXDWORD;
typedef unsigned short  WXWORD;

typedef unsigned long   WXCOLORREF;
typedef void *          WXRGNDATA;
typedef struct tagMSG   WXMSG;
typedef void *          WXHCONV;
typedef void *          WXHKEY;
typedef void *          WXHTREEITEM;

typedef void *          WXDRAWITEMSTRUCT;
typedef void *          WXMEASUREITEMSTRUCT;
typedef void *          WXLPCREATESTRUCT;

typedef WXHWND          WXWidget;

#ifdef __WIN64__
typedef unsigned __int64    WXWPARAM;
typedef __int64            WXLPARAM;
typedef __int64            WXLRESULT;
#else
typedef unsigned int    WXWPARAM;
typedef long            WXLPARAM;
typedef long            WXLRESULT;
#endif

#if defined(__GNUWIN32__) || defined(__WXMICROWIN__)
typedef int             (*WXFARPROC)();
#else
typedef int             (__stdcall *WXFARPROC)();
#endif
#endif /*  __WIN32__ */


#if defined(__OS2__)
typedef unsigned long   DWORD;
typedef unsigned short  WORD;
#endif

#if defined(__WXPM__) || defined(__EMX__)
#ifdef __WXPM__
/*  Stand-ins for OS/2 types, to avoid #including all of os2.h */
typedef unsigned long   WXHWND;
typedef unsigned long   WXHANDLE;
typedef unsigned long   WXHICON;
typedef unsigned long   WXHFONT;
typedef unsigned long   WXHMENU;
typedef unsigned long   WXHPEN;
typedef unsigned long   WXHBRUSH;
typedef unsigned long   WXHPALETTE;
typedef unsigned long   WXHCURSOR;
typedef unsigned long   WXHRGN;
typedef unsigned long   WXHACCEL;
typedef unsigned long   WXHBITMAP;
typedef unsigned long   WXHDC;
typedef unsigned int    WXUINT;
typedef unsigned long   WXDWORD;
typedef unsigned short  WXWORD;

typedef unsigned long   WXCOLORREF;
typedef void *          WXMSG;
typedef unsigned long   WXHTREEITEM;

typedef void *          WXDRAWITEMSTRUCT;
typedef void *          WXMEASUREITEMSTRUCT;
typedef void *          WXLPCREATESTRUCT;

typedef WXHWND          WXWidget;
#endif
#ifdef __EMX__
/* Need a well-known type for WXFARPROC
   below. MPARAM is typedef'ed too late. */
#define WXWPARAM        void *
#define WXLPARAM        void *
#else
#define WXWPARAM        MPARAM
#define WXLPARAM        MPARAM
#endif
#define RECT            RECTL
#define LOGFONT         FATTRS
#define LOWORD          SHORT1FROMMP
#define HIWORD          SHORT2FROMMP

typedef unsigned long   WXMPARAM;
typedef unsigned long   WXMSGID;
typedef void*           WXRESULT;
/* typedef int             (*WXFARPROC)(); */
/*  some windows handles not defined by PM */
typedef unsigned long   HANDLE;
typedef unsigned long   HICON;
typedef unsigned long   HFONT;
typedef unsigned long   HMENU;
typedef unsigned long   HPEN;
typedef unsigned long   HBRUSH;
typedef unsigned long   HPALETTE;
typedef unsigned long   HCURSOR;
typedef unsigned long   HINSTANCE;
typedef unsigned long   HIMAGELIST;
typedef unsigned long   HGLOBAL;
#endif /*  WXPM || EMX */

#if defined (__WXPM__)
/*  WIN32 graphics types for OS/2 GPI */

/*  RGB under OS2 is more like a PALETTEENTRY struct under Windows so we need a real RGB def */
#define OS2RGB(r,g,b) ((DWORD)((unsigned char)(b) | ((unsigned char)(g) << 8)) | ((unsigned char)(r) << 16))

typedef unsigned long COLORREF;
#define GetRValue(rgb) ((unsigned char)((rgb) >> 16))
#define GetGValue(rgb) ((unsigned char)(((unsigned short)(rgb)) >> 8))
#define GetBValue(rgb) ((unsigned char)(rgb))
#define PALETTEINDEX(i) ((COLORREF)(0x01000000 | (DWORD)(WORD)(i)))
#define PALETTERGB(r,g,b) (0x02000000 | OS2RGB(r,g,b))
/*  OS2's RGB/RGB2 is backwards from this */
typedef struct tagPALETTEENTRY
{
    char bRed;
    char bGreen;
    char bBlue;
    char bFlags;
} PALETTEENTRY;
typedef struct tagLOGPALETTE
{
    WORD palVersion;
    WORD palNumentries;
    WORD PALETTEENTRY[1];
} LOGPALETTE;

#if (defined(__VISAGECPP__) && (__IBMCPP__ < 400)) || defined (__WATCOMC__)
    /*  VA 3.0 for some reason needs base data types when typedefing a proc proto??? */
typedef void* (_System *WXFARPROC)(unsigned long, unsigned long, void*, void*);
#else
#if defined(__EMX__) && !defined(_System)
#define _System
#endif
typedef WXRESULT (_System *WXFARPROC)(WXHWND, WXMSGID, WXWPARAM, WXLPARAM);
#endif

#endif /* __WXPM__ */


#if defined(__WXMOTIF__) || defined(__WXX11__)
/* Stand-ins for X/Xt/Motif types */
typedef void*           WXWindow;
typedef void*           WXWidget;
typedef void*           WXAppContext;
typedef void*           WXColormap;
typedef void*           WXColor;
typedef void            WXDisplay;
typedef void            WXEvent;
typedef void*           WXCursor;
typedef void*           WXPixmap;
typedef void*           WXFontStructPtr;
typedef void*           WXGC;
typedef void*           WXRegion;
typedef void*           WXFont;
typedef void*           WXImage;
typedef void*           WXFontList;
typedef void*           WXFontSet;
typedef void*           WXRendition;
typedef void*           WXRenderTable;
typedef void*           WXFontType; /* either a XmFontList or XmRenderTable */
typedef void*           WXString;

typedef unsigned long   Atom;  /* this might fail on a few architectures */
typedef long            WXPixel; /* safety catch in src/motif/colour.cpp */

#endif /*  Motif */

#ifdef __WXGTK__

/* Stand-ins for GLIB types */
typedef char           gchar;
typedef signed char    gint8;
typedef int            gint;
typedef unsigned       guint;
typedef unsigned long  gulong;
typedef void*          gpointer;
typedef struct _GSList GSList;

/* Stand-ins for GDK types */
typedef struct _GdkColor        GdkColor;
typedef struct _GdkColormap     GdkColormap;
typedef struct _GdkFont         GdkFont;
typedef struct _GdkGC           GdkGC;
typedef struct _GdkVisual       GdkVisual;

#ifdef __WXGTK20__
typedef struct _GdkAtom        *GdkAtom;
typedef struct _GdkDrawable     GdkWindow;
typedef struct _GdkDrawable     GdkBitmap;
typedef struct _GdkDrawable     GdkPixmap;
#else /*  GTK+ 1.2 */
typedef gulong                  GdkAtom;
typedef struct _GdkWindow       GdkWindow;
typedef struct _GdkWindow       GdkBitmap;
typedef struct _GdkWindow       GdkPixmap;
#endif /*  GTK+ 1.2/2.0 */

typedef struct _GdkCursor       GdkCursor;
typedef struct _GdkRegion       GdkRegion;
typedef struct _GdkDragContext  GdkDragContext;

#ifdef HAVE_XIM
typedef struct _GdkIC           GdkIC;
typedef struct _GdkICAttr       GdkICAttr;
#endif

/* Stand-ins for GTK types */
typedef struct _GtkWidget         GtkWidget;
typedef struct _GtkRcStyle        GtkRcStyle;
typedef struct _GtkAdjustment     GtkAdjustment;
typedef struct _GtkList           GtkList;
typedef struct _GtkToolbar        GtkToolbar;
typedef struct _GtkTooltips       GtkTooltips;
typedef struct _GtkNotebook       GtkNotebook;
typedef struct _GtkNotebookPage   GtkNotebookPage;
typedef struct _GtkAccelGroup     GtkAccelGroup;
typedef struct _GtkItemFactory    GtkItemFactory;
typedef struct _GtkSelectionData  GtkSelectionData;
typedef struct _GtkTextBuffer     GtkTextBuffer;
typedef struct _GtkRange          GtkRange;

typedef GtkWidget *WXWidget;

#ifndef __WXGTK20__
#define GTK_OBJECT_GET_CLASS(object) (GTK_OBJECT(object)->klass)
#define GTK_CLASS_TYPE(klass) ((klass)->type)
#endif

#endif /*  __WXGTK__ */

#if defined(__WXGTK20__) || (defined(__WXX11__) && wxUSE_UNICODE)
#define wxUSE_PANGO 1
#else
#define wxUSE_PANGO 0
#endif

#if wxUSE_PANGO
/* Stand-ins for Pango types */
typedef struct _PangoContext         PangoContext;
typedef struct _PangoLayout          PangoLayout;
typedef struct _PangoFontDescription PangoFontDescription;
#endif

#ifdef __WXMGL__
typedef struct window_t *WXWidget;
#endif /*  MGL */

#ifdef __WXDFB__
/* DirectFB doesn't have the concept of non-TLW window, so use
   something arbitrary */
typedef const void* WXWidget;
#endif /*  DFB */

/*  This is required because of clashing macros in windows.h, which may be */
/*  included before or after wxWidgets classes, and therefore must be */
/*  disabled here before any significant wxWidgets headers are included. */
#ifdef __cplusplus
#ifdef __WXMSW__
#include "wx/msw/winundef.h"
#endif /*  __WXMSW__ */
#endif /* __cplusplus */

/*  --------------------------------------------------------------------------- */
/*  macro to define a class without copy ctor nor assignment operator */
/*  --------------------------------------------------------------------------- */

#define DECLARE_NO_COPY_CLASS(classname)        \
    private:                                    \
        classname(const classname&);            \
        classname& operator=(const classname&);

#define DECLARE_NO_ASSIGN_CLASS(classname)      \
    private:                                    \
        classname& operator=(const classname&);

/*  --------------------------------------------------------------------------- */
/*  If a manifest is being automatically generated, add common controls 6 to it */
/*  --------------------------------------------------------------------------- */

#if (!defined wxUSE_NO_MANIFEST || wxUSE_NO_MANIFEST == 0 ) && \
    ( defined _MSC_FULL_VER && _MSC_FULL_VER >= 140040130 )

#define WX_CC_MANIFEST(cpu)                     \
    "/manifestdependency:\"type='win32'         \
     name='Microsoft.Windows.Common-Controls'   \
     version='6.0.0.0'                          \
     processorArchitecture='"cpu"'              \
     publicKeyToken='6595b64144ccf1df'          \
     language='*'\""

#if defined _M_IX86
    #pragma comment(linker, WX_CC_MANIFEST("x86"))
#elif defined _M_X64
    #pragma comment(linker, WX_CC_MANIFEST("amd64"))
#elif defined _M_IA64
    #pragma comment(linker, WX_CC_MANIFEST("ia64"))
#else
    #pragma comment(linker, WX_CC_MANIFEST("*"))
#endif

#endif /* !wxUSE_NO_MANIFEST && _MSC_FULL_VER >= 140040130 */

#endif
    /*  _WX_DEFS_H_ */
