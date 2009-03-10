/*
 * Name:        wx/msw/chkconf.h
 * Purpose:     Compiler-specific configuration checking
 * Author:      Julian Smart
 * Modified by:
 * Created:     01/02/97
 * RCS-ID:      $Id: chkconf.h 44436 2007-02-10 02:06:54Z RD $
 * Copyright:   (c) Julian Smart
 * Licence:     wxWindows licence
 */

/* THIS IS A C FILE, DON'T USE C++ FEATURES (IN PARTICULAR COMMENTS) IN IT */

#ifndef _WX_MSW_CHKCONF_H_
#define _WX_MSW_CHKCONF_H_

/* ensure that MSW-specific settings are defined */
#ifndef wxUSE_DC_CACHEING
#   ifdef wxABORT_ON_CONFIG_ERROR
#       error "wxUSE_DC_CACHEING must be defined"
#   else
#       define wxUSE_DC_CACHEING 1
#   endif
#endif /* wxUSE_DC_CACHEING */


/*
 * disable the settings which don't work for some compilers
 */

/*
 * If using PostScript-in-MSW in Univ, must enable PostScript
 */
#if defined(__WXUNIVERSAL__) && wxUSE_POSTSCRIPT_ARCHITECTURE_IN_MSW && !wxUSE_POSTSCRIPT
#   undef wxUSE_POSTSCRIPT
#   define wxUSE_POSTSCRIPT 1
#endif

#ifndef wxUSE_NORLANDER_HEADERS
#   if ( wxCHECK_WATCOM_VERSION(1,0) || defined(__WINE__) ) || \
       ((defined(__MINGW32__) || defined(__CYGWIN__)) && ((__GNUC__>2) ||((__GNUC__==2) && (__GNUC_MINOR__>=95))))
#       define wxUSE_NORLANDER_HEADERS 1
#   else
#       define wxUSE_NORLANDER_HEADERS 0
#   endif
#endif

/*
 * We don't want to give an error if wxUSE_UNICODE_MSLU is enabled but
 * wxUSE_UNICODE is not as this would make it impossible to simply set the
 * former in wx/setup.h as then the library wouldn't compile in non-Unicode
 * configurations, so instead simply unset it silently when it doesn't make
 * sense.
 */
#if wxUSE_UNICODE_MSLU && !wxUSE_UNICODE
#   undef wxUSE_UNICODE_MSLU
#   define wxUSE_UNICODE_MSLU 0
#endif

/*
 * Don't use MSLU if compiling with Wine
 */

#if wxUSE_UNICODE_MSLU && defined(__WINE__)
#   undef wxUSE_UNICODE_MSLU
#   define wxUSE_UNICODE_MSLU 0
#endif

/*
 * All of the settings below require SEH support (__try/__catch) and can't work
 * without it.
 */
#if !defined(_MSC_VER) && \
    (!defined(__BORLANDC__) || __BORLANDC__ < 0x0550)
#    undef wxUSE_ON_FATAL_EXCEPTION
#    define wxUSE_ON_FATAL_EXCEPTION 0

#    undef wxUSE_CRASHREPORT
#    define wxUSE_CRASHREPORT 0

#    undef wxUSE_STACKWALKER
#    define wxUSE_STACKWALKER 0
#endif /* compiler doesn't support SEH */

/* wxUSE_DEBUG_NEW_ALWAYS doesn't work with CodeWarrior */
#if defined(__MWERKS__)
#    undef wxUSE_DEBUG_NEW_ALWAYS
#    define wxUSE_DEBUG_NEW_ALWAYS      0
#endif

#if defined(__GNUWIN32__)
    /* These don't work as expected for mingw32 and cygwin32 */
#   undef  wxUSE_MEMORY_TRACING
#   define wxUSE_MEMORY_TRACING            0

#   undef  wxUSE_GLOBAL_MEMORY_OPERATORS
#   define wxUSE_GLOBAL_MEMORY_OPERATORS   0

#   undef  wxUSE_DEBUG_NEW_ALWAYS
#   define wxUSE_DEBUG_NEW_ALWAYS          0

/* some Cygwin versions don't have wcslen */
#   if defined(__CYGWIN__) || defined(__CYGWIN32__)
#   if ! ((__GNUC__>2) ||((__GNUC__==2) && (__GNUC_MINOR__>=95)))
#       undef wxUSE_WCHAR_T
#       define wxUSE_WCHAR_T 0
#   endif
#endif

#endif /* __GNUWIN32__ */

/* wxUSE_MFC is not defined when using configure as it doesn't make sense for
   gcc or mingw32 anyhow */
#ifndef wxUSE_MFC
    #define wxUSE_MFC 0
#endif /* !defined(wxUSE_MFC) */

/* MFC duplicates these operators */
#if wxUSE_MFC
#   undef  wxUSE_GLOBAL_MEMORY_OPERATORS
#   define wxUSE_GLOBAL_MEMORY_OPERATORS   0

#   undef  wxUSE_DEBUG_NEW_ALWAYS
#   define wxUSE_DEBUG_NEW_ALWAYS          0
#endif /* wxUSE_MFC */

#if (defined(__GNUWIN32__) && !wxUSE_NORLANDER_HEADERS)
    /* GnuWin32 doesn't have appropriate headers for e.g. IUnknown. */
#   undef wxUSE_DRAG_AND_DROP
#   define wxUSE_DRAG_AND_DROP 0
#endif

#if !wxUSE_OWNER_DRAWN && !defined(__WXUNIVERSAL__)
#   undef wxUSE_CHECKLISTBOX
#   define wxUSE_CHECKLISTBOX 0
#endif

#if wxUSE_SPINCTRL
#   if !wxUSE_SPINBTN
#       ifdef wxABORT_ON_CONFIG_ERROR
#           error "wxSpinCtrl requires wxSpinButton on MSW"
#       else
#           undef wxUSE_SPINBTN
#           define wxUSE_SPINBTN 1
#       endif
#   endif
#endif

/*
   Win64-specific checks.
 */
#ifdef __WIN64__
#   if wxUSE_STACKWALKER
        /* this is not currently supported under Win64, volunteers needed to
           make it work */
#       undef wxUSE_STACKWALKER
#       define wxUSE_STACKWALKER 0

#       undef wxUSE_CRASHREPORT
#       define wxUSE_CRASHREPORT 0
#   endif
#endif /* __WIN64__ */


/*
   Compiler-specific checks.
 */
#if defined(__BORLANDC__) && (__BORLANDC__ < 0x500)
    /* BC++ 4.0 can't compile JPEG library */
#   undef wxUSE_LIBJPEG
#   define wxUSE_LIBJPEG 0
#endif

/* wxUSE_DEBUG_NEW_ALWAYS = 1 not compatible with BC++ in DLL mode */
#if defined(__BORLANDC__) && (defined(WXMAKINGDLL) || defined(WXUSINGDLL))
#   undef wxUSE_DEBUG_NEW_ALWAYS
#   define wxUSE_DEBUG_NEW_ALWAYS 0
#endif

/* DMC++ doesn't have definitions for date picker control, so use generic control
 */
#ifdef __DMC__
#   if wxUSE_DATEPICKCTRL
#       undef wxUSE_DATEPICKCTRL_GENERIC
#       undef wxUSE_DATEPICKCTRL
#   endif
#   define wxUSE_DATEPICKCTRL 0
#   define wxUSE_DATEPICKCTRL_GENERIC 1
#endif


/* check that MSW-specific options are defined too */
#ifndef wxUSE_ACTIVEX
#    ifdef wxABORT_ON_CONFIG_ERROR
#        error "wxUSE_ACTIVEX must be defined."
#    else
#        define wxUSE_ACTIVEX 0
#    endif
#endif /* !defined(wxUSE_ACTIVEX) */

#ifndef wxUSE_DIALUP_MANAGER
#    ifdef wxABORT_ON_CONFIG_ERROR
#        error "wxUSE_DIALUP_MANAGER must be defined."
#    else
#        define wxUSE_DIALUP_MANAGER 0
#    endif
#endif /* !defined(wxUSE_DIALUP_MANAGER) */

#ifndef wxUSE_MS_HTML_HELP
#    ifdef wxABORT_ON_CONFIG_ERROR
#        error "wxUSE_MS_HTML_HELP must be defined."
#    else
#        define wxUSE_MS_HTML_HELP 0
#    endif
#endif /* !defined(wxUSE_MS_HTML_HELP) */

#ifndef wxUSE_OLE
#    ifdef wxABORT_ON_CONFIG_ERROR
#        error "wxUSE_OLE must be defined."
#    else
#        define wxUSE_OLE 0
#    endif
#endif /* !defined(wxUSE_OLE) */

#ifndef wxUSE_OLE_AUTOMATION
#    ifdef wxABORT_ON_CONFIG_ERROR
#        error "wxUSE_OLE_AUTOMATION must be defined."
#    else
#        define wxUSE_OLE_AUTOMATION 0
#    endif
#endif /* !defined(wxUSE_OLE_AUTOMATION) */

#ifndef wxUSE_UNICODE_MSLU
#    ifdef wxABORT_ON_CONFIG_ERROR
#        error "wxUSE_UNICODE_MSLU must be defined."
#    else
#        define wxUSE_UNICODE_MSLU 0
#    endif
#endif  /* wxUSE_UNICODE_MSLU */

#ifndef wxUSE_UXTHEME
#    ifdef wxABORT_ON_CONFIG_ERROR
#        error "wxUSE_UXTHEME must be defined."
#    else
#        define wxUSE_UXTHEME 0
#    endif
#endif  /* wxUSE_UXTHEME */

#ifndef wxUSE_UXTHEME_AUTO
#    ifdef wxABORT_ON_CONFIG_ERROR
#        error "wxUSE_UXTHEME_AUTO must be defined."
#    else
#        define wxUSE_UXTHEME_AUTO 0
#    endif
#endif  /* wxUSE_UXTHEME_AUTO */


/*
   un/redefine the options which we can't compile (after checking that they're
   defined
 */
#ifdef __WINE__
    /* apparently it doesn't compile under Wine, remove it/when it does */
    #if wxUSE_ACTIVEX
        #undef wxUSE_ACTIVEX
        #define wxUSE_ACTIVEX 0
    #endif // wxUSE_ACTIVEX
#endif // __WINE__


/* check settings consistency for MSW-specific ones */
#if !wxUSE_VARIANT
#   if wxUSE_ACTIVEX
#       ifdef wxABORT_ON_CONFIG_ERROR
#           error "wxActiveXContainer requires wxVariant"
#       else
#           undef wxUSE_ACTIVEX
#           define wxUSE_ACTIVEX 0
#       endif
#   endif

#   if wxUSE_OLE_AUTOMATION
#       ifdef wxABORT_ON_CONFIG_ERROR
#           error "wxAutomationObject requires wxVariant"
#       else
#           undef wxUSE_OLE_AUTOMATION
#           define wxUSE_OLE_AUTOMATION 0
#       endif
#   endif
#endif /* !wxUSE_VARIANT */

#if !wxUSE_DYNAMIC_LOADER
#    if wxUSE_MS_HTML_HELP
#        ifdef wxABORT_ON_CONFIG_ERROR
#            error "wxUSE_MS_HTML_HELP requires wxUSE_DYNAMIC_LOADER."
#        else
#            undef wxUSE_MS_HTML_HELP
#            define wxUSE_MS_HTML_HELP 0
#        endif
#    endif
#    if wxUSE_DIALUP_MANAGER
#        ifdef wxABORT_ON_CONFIG_ERROR
#            error "wxUSE_DIALUP_MANAGER requires wxUSE_DYNAMIC_LOADER."
#        else
#            undef wxUSE_DIALUP_MANAGER
#            define wxUSE_DIALUP_MANAGER 0
#        endif
#    endif
#endif  /* !wxUSE_DYNAMIC_LOADER */

#if !wxUSE_DYNLIB_CLASS
#   if wxUSE_UXTHEME
#       ifdef wxABORT_ON_CONFIG_ERROR
#           error "wxUSE_UXTHEME requires wxUSE_DYNLIB_CLASS"
#       else
#           undef wxUSE_UXTHEME
#           define wxUSE_UXTHEME 0
#       endif
#   endif
#   if wxUSE_MEDIACTRL
#       ifdef wxABORT_ON_CONFIG_ERROR
#           error "wxUSE_MEDIACTRL requires wxUSE_DYNLIB_CLASS"
#       else
#           undef wxUSE_MEDIACTRL
#           define wxUSE_MEDIACTRL 0
#       endif
#   endif
#   if wxUSE_INKEDIT
#       ifdef wxABORT_ON_CONFIG_ERROR
#           error "wxUSE_INKEDIT requires wxUSE_DYNLIB_CLASS"
#       else
#           undef wxUSE_INKEDIT
#           define wxUSE_INKEDIT 0
#       endif
#   endif
#endif  /* !wxUSE_DYNLIB_CLASS */

#if !wxUSE_OLE
#   if wxUSE_ACTIVEX
#       ifdef wxABORT_ON_CONFIG_ERROR
#           error "wxActiveXContainer requires wxUSE_OLE"
#       else
#           undef wxUSE_ACTIVEX
#           define wxUSE_ACTIVEX 0
#       endif
#   endif

#   if wxUSE_DATAOBJ
#       ifdef wxABORT_ON_CONFIG_ERROR
#           error "wxUSE_DATAOBJ requires wxUSE_OLE"
#       else
#           undef wxUSE_DATAOBJ
#           define wxUSE_DATAOBJ 0
#       endif
#   endif

#   if wxUSE_OLE_AUTOMATION
#       ifdef wxABORT_ON_CONFIG_ERROR
#           error "wxAutomationObject requires wxUSE_OLE"
#       else
#           undef wxUSE_OLE_AUTOMATION
#           define wxUSE_OLE_AUTOMATION 0
#       endif
#   endif
#endif /* !wxUSE_OLE */

#if !wxUSE_ACTIVEX
#   if wxUSE_MEDIACTRL
#       ifdef wxABORT_ON_CONFIG_ERROR
#           error "wxMediaCtl requires wxActiveXContainer"
#       else
#           undef wxUSE_MEDIACTRL
#           define wxUSE_MEDIACTRL 0
#       endif
#   endif
#endif /* !wxUSE_ACTIVEX */

#if defined(_MSC_VER) && _MSC_VER <= 1200 && wxUSE_GRAPHICS_CONTEXT
#       ifdef wxABORT_ON_CONFIG_ERROR
#           error "wxGraphicsContext needs MSVC 7 or newer"
#       else
#           undef wxUSE_GRAPHICS_CONTEXT
#           define wxUSE_GRAPHICS_CONTEXT 0
#       endif
#endif

#endif /* _WX_MSW_CHKCONF_H_ */
