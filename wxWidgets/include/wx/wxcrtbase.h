/*
 * Name:        wx/wxcrtbase.h
 * Purpose:     Type-safe ANSI and Unicode builds compatible wrappers for
 *              CRT functions
 * Author:      Joel Farley, Ove Kaaven
 * Modified by: Vadim Zeitlin, Robert Roebling, Ron Lee
 * Created:     1998/06/12
 * Copyright:   (c) 1998-2006 wxWidgets dev team
 * Licence:     wxWindows licence
 */

/* THIS IS A C FILE, DON'T USE C++ FEATURES (IN PARTICULAR COMMENTS) IN IT */

#ifndef _WX_WXCRTBASE_H_
#define _WX_WXCRTBASE_H_

/* -------------------------------------------------------------------------
                        headers and missing declarations
   ------------------------------------------------------------------------- */

#include "wx/chartype.h"

/*
    Standard headers we need here.

    NB: don't include any wxWidgets headers here because almost all of them
        include this one!

    NB2: User code should include wx/crt.h instead of including this
         header directly.

 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <time.h>

#if defined(__WINDOWS__) && !defined(__WXWINCE__)
    #include <io.h>
#endif

#if defined(HAVE_STRTOK_R) && defined(__DARWIN__) && defined(_MSL_USING_MW_C_HEADERS) && _MSL_USING_MW_C_HEADERS
    char *strtok_r(char *, const char *, char **);
#endif

/*
    Using -std=c++{98,0x} option with mingw32 disables most of standard
    library extensions, so we can't rely on the presence of common non-ANSI
    functions, define a special symbol to test for this. Notice that this
    doesn't need to be done for g++ under Linux where _GNU_SOURCE (which is
    defined by default) still makes all common extensions available even in
    ANSI mode.
 */
#if defined(__MINGW32__) && defined(__STRICT_ANSI__)
    #define __WX_STRICT_ANSI_GCC__
#endif

/*
   a few compilers don't have the (non standard but common) isascii function,
   define it ourselves for them
 */
#ifndef isascii
    #if defined(__WX_STRICT_ANSI_GCC__)
        #define wxNEED_ISASCII
    #elif defined(_WIN32_WCE)
        #if _WIN32_WCE <= 211
            #define wxNEED_ISASCII
        #endif
    #endif
#endif /* isascii */

#ifdef wxNEED_ISASCII
    inline int isascii(int c) { return (unsigned)c < 0x80; }
#endif

#ifdef _WIN32_WCE
    #if _WIN32_WCE <= 211
        #define isspace(c) ((c) == wxT(' ') || (c) == wxT('\t'))
    #endif
#endif /* _WIN32_WCE */

/* string.h functions */
#ifndef strdup
    #if defined(__WXWINCE__)
        #if _WIN32_WCE <= 211
            #define wxNEED_STRDUP
        #endif
    #endif
#endif /* strdup */

#ifdef wxNEED_STRDUP
    WXDLLIMPEXP_BASE char *strdup(const char* s);
#endif

/* missing functions in some WinCE versions */
#ifdef _WIN32_WCE
#if (_WIN32_WCE < 300)
WXDLLIMPEXP_BASE void *calloc( size_t num, size_t size );
#endif
#endif /* _WIN32_WCE */


/* -------------------------------------------------------------------------
                            UTF-8 locale handling
   ------------------------------------------------------------------------- */

#ifdef __cplusplus
    #if wxUSE_UNICODE_UTF8
        /* flag indicating whether the current locale uses UTF-8 or not; must be
           updated every time the locale is changed! */
        #if wxUSE_UTF8_LOCALE_ONLY
        #define wxLocaleIsUtf8 true
        #else
        extern WXDLLIMPEXP_BASE bool wxLocaleIsUtf8;
        #endif
        /* function used to update the flag: */
        extern WXDLLIMPEXP_BASE void wxUpdateLocaleIsUtf8();
    #else /* !wxUSE_UNICODE_UTF8 */
        inline void wxUpdateLocaleIsUtf8() {}
    #endif /* wxUSE_UNICODE_UTF8/!wxUSE_UNICODE_UTF8 */
#endif /* __cplusplus */


/* -------------------------------------------------------------------------
                                 string.h
   ------------------------------------------------------------------------- */

#define wxCRT_StrcatA    strcat
#define wxCRT_StrchrA    strchr
#define wxCRT_StrcmpA    strcmp
#define wxCRT_StrcpyA    strcpy
#define wxCRT_StrcspnA   strcspn
#define wxCRT_StrlenA    strlen
#define wxCRT_StrncatA   strncat
#define wxCRT_StrncmpA   strncmp
#define wxCRT_StrncpyA   strncpy
#define wxCRT_StrpbrkA   strpbrk
#define wxCRT_StrrchrA   strrchr
#define wxCRT_StrspnA    strspn
#define wxCRT_StrstrA    strstr

#define wxCRT_StrcatW    wcscat
#define wxCRT_StrchrW    wcschr
#define wxCRT_StrcmpW    wcscmp
#define wxCRT_StrcpyW    wcscpy
#define wxCRT_StrcspnW   wcscspn
#define wxCRT_StrncatW   wcsncat
#define wxCRT_StrncmpW   wcsncmp
#define wxCRT_StrncpyW   wcsncpy
#define wxCRT_StrpbrkW   wcspbrk
#define wxCRT_StrrchrW   wcsrchr
#define wxCRT_StrspnW    wcsspn
#define wxCRT_StrstrW    wcsstr

/* these functions are not defined under CE, at least in VC8 CRT */
#if !defined(__WXWINCE__)
    #define wxCRT_StrcollA   strcoll
    #define wxCRT_StrxfrmA   strxfrm

    #define wxCRT_StrcollW   wcscoll
    #define wxCRT_StrxfrmW   wcsxfrm
#endif /* __WXWINCE__ */

/* Almost all compilers have strdup(), but VC++ and MinGW call it _strdup().
   And it's not available in MinGW strict ANSI mode nor under Windows CE. */
#if (defined(__VISUALC__) && __VISUALC__ >= 1400)
    #define wxCRT_StrdupA _strdup
#elif defined(__MINGW32__)
    #ifndef __WX_STRICT_ANSI_GCC__
        #define wxCRT_StrdupA _strdup
    #endif
#elif !defined(__WXWINCE__)
    #define wxCRT_StrdupA strdup
#endif

/* most Windows compilers provide _wcsdup() */
#if defined(__WINDOWS__) && \
        !(defined(__CYGWIN__) || defined(__WX_STRICT_ANSI_GCC__))
    #define wxCRT_StrdupW _wcsdup
#elif defined(HAVE_WCSDUP)
    #define wxCRT_StrdupW wcsdup
#endif

#ifdef wxHAVE_TCHAR_SUPPORT
    /* we surely have wchar_t if we have TCHAR have wcslen() */
    #ifndef HAVE_WCSLEN
        #define HAVE_WCSLEN
    #endif
#endif /* wxHAVE_TCHAR_SUPPORT */

#ifdef HAVE_WCSLEN
    #define wxCRT_StrlenW wcslen
#endif

#define wxCRT_StrtodA    strtod
#define wxCRT_StrtolA    strtol
#define wxCRT_StrtoulA   strtoul
#define wxCRT_StrtodW    wcstod
#define wxCRT_StrtolW    wcstol
#define wxCRT_StrtoulW   wcstoul

#ifdef __VISUALC__
    #if __VISUALC__ >= 1300 && !defined(__WXWINCE__)
        #define wxCRT_StrtollA   _strtoi64
        #define wxCRT_StrtoullA  _strtoui64
        #define wxCRT_StrtollW   _wcstoi64
        #define wxCRT_StrtoullW  _wcstoui64
    #endif /* VC++ 7+ */
#else
    #ifdef HAVE_STRTOULL
        #define wxCRT_StrtollA   strtoll
        #define wxCRT_StrtoullA  strtoull
    #endif /* HAVE_STRTOULL */
    #ifdef HAVE_WCSTOULL
        /* assume that we have wcstoull(), which is also C99, too */
        #define wxCRT_StrtollW   wcstoll
        #define wxCRT_StrtoullW  wcstoull
    #endif /* HAVE_WCSTOULL */
#endif

/*
    Only VC8 and later provide strnlen() and wcsnlen() functions under Windows
    and it's also only available starting from Windows CE 6.0 only in CE build.
 */
#if wxCHECK_VISUALC_VERSION(8) && (!defined(_WIN32_WCE) || (_WIN32_WCE >= 0x600))
    #ifndef HAVE_STRNLEN
        #define HAVE_STRNLEN
    #endif
    #ifndef HAVE_WCSNLEN
        #define HAVE_WCSNLEN
    #endif
#endif

#ifdef HAVE_STRNLEN
    #define wxCRT_StrnlenA  strnlen
#endif

#ifdef HAVE_WCSNLEN
    #define wxCRT_StrnlenW  wcsnlen
#endif

/* define wxCRT_StricmpA/W and wxCRT_StrnicmpA/W for various compilers */

#if defined(__BORLANDC__) || defined(__WATCOMC__) || \
        defined(__VISAGECPP__) || \
        defined(__EMX__) || defined(__DJGPP__)
    #define wxCRT_StricmpA stricmp
    #define wxCRT_StrnicmpA strnicmp
#elif defined(__SYMANTEC__) || (defined(__VISUALC__) && !defined(__WXWINCE__))
    #define wxCRT_StricmpA _stricmp
    #define wxCRT_StrnicmpA _strnicmp
#elif defined(__UNIX__) || (defined(__GNUWIN32__) && !defined(__WX_STRICT_ANSI_GCC__))
    #define wxCRT_StricmpA strcasecmp
    #define wxCRT_StrnicmpA strncasecmp
/* #else -- use wxWidgets implementation */
#endif

#ifdef __VISUALC__
    #define wxCRT_StricmpW _wcsicmp
    #define wxCRT_StrnicmpW _wcsnicmp
#elif defined(__UNIX__)
    #ifdef HAVE_WCSCASECMP
        #define wxCRT_StricmpW wcscasecmp
    #endif
    #ifdef HAVE_WCSNCASECMP
        #define wxCRT_StrnicmpW wcsncasecmp
    #endif
/* #else -- use wxWidgets implementation */
#endif

#ifdef HAVE_STRTOK_R
    #define  wxCRT_StrtokA(str, sep, last)    strtok_r(str, sep, last)
#endif
/* FIXME-UTF8: detect and use wcstok() if available for wxCRT_StrtokW */

/* these are extern "C" because they are used by regex lib: */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef wxCRT_StrlenW
WXDLLIMPEXP_BASE size_t wxCRT_StrlenW(const wchar_t *s);
#endif

#ifndef wxCRT_StrncmpW
WXDLLIMPEXP_BASE int wxCRT_StrncmpW(const wchar_t *s1, const wchar_t *s2, size_t n);
#endif

#ifdef __cplusplus
}
#endif

/* FIXME-UTF8: remove this once we are Unicode only */
#if wxUSE_UNICODE
    #define wxCRT_StrlenNative  wxCRT_StrlenW
    #define wxCRT_StrncmpNative wxCRT_StrncmpW
    #define wxCRT_ToupperNative wxCRT_ToupperW
    #define wxCRT_TolowerNative wxCRT_TolowerW
#else
    #define wxCRT_StrlenNative  wxCRT_StrlenA
    #define wxCRT_StrncmpNative wxCRT_StrncmpA
    #define wxCRT_ToupperNative toupper
    #define wxCRT_TolowerNative tolower
#endif

#ifndef wxCRT_StrcatW
WXDLLIMPEXP_BASE wchar_t *wxCRT_StrcatW(wchar_t *dest, const wchar_t *src);
#endif

#ifndef wxCRT_StrchrW
WXDLLIMPEXP_BASE const wchar_t *wxCRT_StrchrW(const wchar_t *s, wchar_t c);
#endif

#ifndef wxCRT_StrcmpW
WXDLLIMPEXP_BASE int wxCRT_StrcmpW(const wchar_t *s1, const wchar_t *s2);
#endif

#ifndef wxCRT_StrcollW
WXDLLIMPEXP_BASE int wxCRT_StrcollW(const wchar_t *s1, const wchar_t *s2);
#endif

#ifndef wxCRT_StrcpyW
WXDLLIMPEXP_BASE wchar_t *wxCRT_StrcpyW(wchar_t *dest, const wchar_t *src);
#endif

#ifndef wxCRT_StrcspnW
WXDLLIMPEXP_BASE size_t wxCRT_StrcspnW(const wchar_t *s, const wchar_t *reject);
#endif

#ifndef wxCRT_StrncatW
WXDLLIMPEXP_BASE wchar_t *wxCRT_StrncatW(wchar_t *dest, const wchar_t *src, size_t n);
#endif

#ifndef wxCRT_StrncpyW
WXDLLIMPEXP_BASE wchar_t *wxCRT_StrncpyW(wchar_t *dest, const wchar_t *src, size_t n);
#endif

#ifndef wxCRT_StrpbrkW
WXDLLIMPEXP_BASE const wchar_t *wxCRT_StrpbrkW(const wchar_t *s, const wchar_t *accept);
#endif

#ifndef wxCRT_StrrchrW
WXDLLIMPEXP_BASE const wchar_t *wxCRT_StrrchrW(const wchar_t *s, wchar_t c);
#endif

#ifndef wxCRT_StrspnW
WXDLLIMPEXP_BASE size_t wxCRT_StrspnW(const wchar_t *s, const wchar_t *accept);
#endif

#ifndef wxCRT_StrstrW
WXDLLIMPEXP_BASE const wchar_t *wxCRT_StrstrW(const wchar_t *haystack, const wchar_t *needle);
#endif

#ifndef wxCRT_StrtodW
WXDLLIMPEXP_BASE double wxCRT_StrtodW(const wchar_t *nptr, wchar_t **endptr);
#endif

#ifndef wxCRT_StrtolW
WXDLLIMPEXP_BASE long int wxCRT_StrtolW(const wchar_t *nptr, wchar_t **endptr, int base);
#endif

#ifndef wxCRT_StrtoulW
WXDLLIMPEXP_BASE unsigned long int wxCRT_StrtoulW(const wchar_t *nptr, wchar_t **endptr, int base);
#endif

#ifndef wxCRT_StrxfrmW
WXDLLIMPEXP_BASE size_t wxCRT_StrxfrmW(wchar_t *dest, const wchar_t *src, size_t n);
#endif

#ifndef wxCRT_StrdupA
WXDLLIMPEXP_BASE char *wxCRT_StrdupA(const char *psz);
#endif

#ifndef wxCRT_StrdupW
WXDLLIMPEXP_BASE wchar_t *wxCRT_StrdupW(const wchar_t *pwz);
#endif

#ifndef wxCRT_StricmpA
WXDLLIMPEXP_BASE int wxCRT_StricmpA(const char *psz1, const char *psz2);
#endif

#ifndef wxCRT_StricmpW
WXDLLIMPEXP_BASE int wxCRT_StricmpW(const wchar_t *psz1, const wchar_t *psz2);
#endif

#ifndef wxCRT_StrnicmpA
WXDLLIMPEXP_BASE int wxCRT_StrnicmpA(const char *psz1, const char *psz2, size_t len);
#endif

#ifndef wxCRT_StrnicmpW
WXDLLIMPEXP_BASE int wxCRT_StrnicmpW(const wchar_t *psz1, const wchar_t *psz2, size_t len);
#endif

#ifndef wxCRT_StrtokA
WXDLLIMPEXP_BASE char *wxCRT_StrtokA(char *psz, const char *delim, char **save_ptr);
#endif

#ifndef wxCRT_StrtokW
WXDLLIMPEXP_BASE wchar_t *wxCRT_StrtokW(wchar_t *psz, const wchar_t *delim, wchar_t **save_ptr);
#endif

/* supply strtoll and strtoull, if needed */
#ifdef wxLongLong_t
    #ifndef wxCRT_StrtollA
        WXDLLIMPEXP_BASE wxLongLong_t wxCRT_StrtollA(const char* nptr,
                                                     char** endptr,
                                                     int base);
        WXDLLIMPEXP_BASE wxULongLong_t wxCRT_StrtoullA(const char* nptr,
                                                       char** endptr,
                                                       int base);
    #endif
    #ifndef wxCRT_StrtollW
        WXDLLIMPEXP_BASE wxLongLong_t wxCRT_StrtollW(const wchar_t* nptr,
                                                     wchar_t** endptr,
                                                     int base);
        WXDLLIMPEXP_BASE wxULongLong_t wxCRT_StrtoullW(const wchar_t* nptr,
                                                       wchar_t** endptr,
                                                       int base);
    #endif
#endif /* wxLongLong_t */


/* -------------------------------------------------------------------------
                                  stdio.h
   ------------------------------------------------------------------------- */

#if defined(__UNIX__) || defined(__WXMAC__)
    #define wxMBFILES 1
#else
    #define wxMBFILES 0
#endif


/* these functions are only needed in the form used for filenames (i.e. char*
   on Unix, wchar_t* on Windows), so we don't need to use A/W suffix: */
#if wxMBFILES || !wxUSE_UNICODE /* ANSI filenames */

    #define wxCRT_Fopen   fopen
    #define wxCRT_Freopen freopen
    #define wxCRT_Remove  remove
    #define wxCRT_Rename  rename

#else /* Unicode filenames */
    /* special case: these functions are missing under Win9x with Unicows so we
       have to implement them ourselves */
    #if wxUSE_UNICODE_MSLU || defined(__WX_STRICT_ANSI_GCC__)
            WXDLLIMPEXP_BASE FILE* wxMSLU__wfopen(const wchar_t *name, const wchar_t *mode);
            WXDLLIMPEXP_BASE FILE* wxMSLU__wfreopen(const wchar_t *name, const wchar_t *mode, FILE *stream);
            WXDLLIMPEXP_BASE int wxMSLU__wrename(const wchar_t *oldname, const wchar_t *newname);
            WXDLLIMPEXP_BASE int wxMSLU__wremove(const wchar_t *name);
            #define wxCRT_Fopen     wxMSLU__wfopen
            #define wxCRT_Freopen   wxMSLU__wfreopen
            #define wxCRT_Remove    wxMSLU__wremove
            #define wxCRT_Rename    wxMSLU__wrename
    #else
        /* WinCE CRT doesn't provide these functions so use our own */
        #ifdef __WXWINCE__
            WXDLLIMPEXP_BASE int wxCRT_Rename(const wchar_t *src,
                                              const wchar_t *dst);
            WXDLLIMPEXP_BASE int wxCRT_Remove(const wchar_t *path);
        #else
            #define wxCRT_Rename   _wrename
            #define wxCRT_Remove _wremove
        #endif
        #define wxCRT_Fopen    _wfopen
        #define wxCRT_Freopen  _wfreopen
    #endif

#endif /* wxMBFILES/!wxMBFILES */

#define wxCRT_PutsA       puts
#define wxCRT_FputsA      fputs
#define wxCRT_FgetsA      fgets
#define wxCRT_FputcA      fputc
#define wxCRT_FgetcA      fgetc
#define wxCRT_UngetcA     ungetc

#ifdef wxHAVE_TCHAR_SUPPORT
    #define wxCRT_PutsW   _putws
    #define wxCRT_FputsW  fputws
    #define wxCRT_FputcW  fputwc
#endif
#ifdef HAVE_FPUTWS
    #define wxCRT_FputsW  fputws
#endif
#ifdef HAVE_PUTWS
    #define wxCRT_PutsW   putws
#endif
#ifdef HAVE_FPUTWC
    #define wxCRT_FputcW  fputwc
#endif
#define wxCRT_FgetsW  fgetws

#ifndef wxCRT_PutsW
WXDLLIMPEXP_BASE int wxCRT_PutsW(const wchar_t *ws);
#endif

#ifndef wxCRT_FputsW
WXDLLIMPEXP_BASE int wxCRT_FputsW(const wchar_t *ch, FILE *stream);
#endif

#ifndef wxCRT_FputcW
WXDLLIMPEXP_BASE int wxCRT_FputcW(wchar_t wc, FILE *stream);
#endif

/*
   NB: tmpnam() is unsafe and thus is not wrapped!
       Use other wxWidgets facilities instead:
        wxFileName::CreateTempFileName, wxTempFile, or wxTempFileOutputStream
*/
#define wxTmpnam(x)         wxTmpnam_is_insecure_use_wxTempFile_instead

/* FIXME-CE: provide our own perror() using ::GetLastError() */
#ifndef __WXWINCE__

#define wxCRT_PerrorA   perror
#ifdef wxHAVE_TCHAR_SUPPORT
    #define wxCRT_PerrorW _wperror
#endif

#endif /* !__WXWINCE__ */

/* -------------------------------------------------------------------------
                                  stdlib.h
   ------------------------------------------------------------------------- */

/* there are no env vars at all under CE, so no _tgetenv neither */
#ifdef __WXWINCE__
    /* can't define as inline function as this is a C file... */
    #define wxCRT_GetenvA(name)     (name, NULL)
    #define wxCRT_GetenvW(name)     (name, NULL)
#else
    #define wxCRT_GetenvA           getenv
    #ifdef _tgetenv
        #define wxCRT_GetenvW       _wgetenv
    #endif
#endif

#ifndef wxCRT_GetenvW
WXDLLIMPEXP_BASE wchar_t * wxCRT_GetenvW(const wchar_t *name);
#endif


#define wxCRT_SystemA               system
/* mingw32 doesn't provide _tsystem() or _wsystem(): */
#if defined(_tsystem)
    #define  wxCRT_SystemW          _wsystem
#endif

#define wxCRT_AtofA                 atof
#define wxCRT_AtoiA                 atoi
#define wxCRT_AtolA                 atol

#if defined(wxHAVE_TCHAR_SUPPORT) && !defined(__WX_STRICT_ANSI_GCC__)
    #define  wxCRT_AtoiW           _wtoi
    #define  wxCRT_AtolW           _wtol
    /* _wtof doesn't exist */
#else
#ifndef __VMS
    #define wxCRT_AtofW(s)         wcstod(s, NULL)
#endif
    #define wxCRT_AtolW(s)         wcstol(s, NULL, 10)
    /* wcstoi doesn't exist */
#endif

/* -------------------------------------------------------------------------
                                time.h
   ------------------------------------------------------------------------- */

#define wxCRT_StrftimeA  strftime
#ifdef __SGI__
    /*
        IRIX provides not one but two versions of wcsftime(): XPG4 one which
        uses "const char*" for the third parameter and so can't be used and the
        correct, XPG5, one. Unfortunately we can't just define _XOPEN_SOURCE
        high enough to get XPG5 version as this undefines other symbols which
        make other functions we use unavailable (see <standards.h> for gory
        details). So just declare the XPG5 version ourselves, we're extremely
        unlikely to ever be compiled on a system without it. But if we ever do,
        a configure test would need to be added for it (and _MIPS_SYMBOL_PRESENT
        should be used to check for its presence during run-time, i.e. it would
        probably be simpler to just always use our own wxCRT_StrftimeW() below
        if it does ever become a problem).
     */
#ifdef __cplusplus
    extern "C"
#endif
    size_t
    _xpg5_wcsftime(wchar_t *, size_t, const wchar_t *, const struct tm * );
    #define wxCRT_StrftimeW _xpg5_wcsftime
#else
    /*
        Assume it's always available under non-Unix systems as this does seem
        to be the case for now. And under Unix we trust configure to detect it
        (except for SGI special case above).
     */
    #if defined(HAVE_WCSFTIME) || !defined(__UNIX__)
        #define wxCRT_StrftimeW  wcsftime
    #endif
#endif

#ifndef wxCRT_StrftimeW
WXDLLIMPEXP_BASE size_t wxCRT_StrftimeW(wchar_t *s, size_t max,
                                        const wchar_t *fmt,
                                        const struct tm *tm);
#endif



/* -------------------------------------------------------------------------
                                ctype.h
   ------------------------------------------------------------------------- */

#ifdef __WATCOMC__
  #define WXWCHAR_T_CAST(c) (wint_t)(c)
#else
  #define WXWCHAR_T_CAST(c) c
#endif

#define wxCRT_IsalnumW(c)   iswalnum(WXWCHAR_T_CAST(c))
#define wxCRT_IsalphaW(c)   iswalpha(WXWCHAR_T_CAST(c))
#define wxCRT_IscntrlW(c)   iswcntrl(WXWCHAR_T_CAST(c))
#define wxCRT_IsdigitW(c)   iswdigit(WXWCHAR_T_CAST(c))
#define wxCRT_IsgraphW(c)   iswgraph(WXWCHAR_T_CAST(c))
#define wxCRT_IslowerW(c)   iswlower(WXWCHAR_T_CAST(c))
#define wxCRT_IsprintW(c)   iswprint(WXWCHAR_T_CAST(c))
#define wxCRT_IspunctW(c)   iswpunct(WXWCHAR_T_CAST(c))
#define wxCRT_IsspaceW(c)   iswspace(WXWCHAR_T_CAST(c))
#define wxCRT_IsupperW(c)   iswupper(WXWCHAR_T_CAST(c))
#define wxCRT_IsxdigitW(c)  iswxdigit(WXWCHAR_T_CAST(c))

#ifdef __GLIBC__
    #if defined(__GLIBC__) && (__GLIBC__ == 2) && (__GLIBC_MINOR__ == 0)
        /* /usr/include/wctype.h incorrectly declares translations */
        /* tables which provokes tons of compile-time warnings -- try */
        /* to correct this */
        #define wxCRT_TolowerW(wc) towctrans((wc), (wctrans_t)__ctype_tolower)
        #define wxCRT_ToupperW(wc) towctrans((wc), (wctrans_t)__ctype_toupper)
    #else /* !glibc 2.0 */
        #define wxCRT_TolowerW   towlower
        #define wxCRT_ToupperW   towupper
    #endif
#else /* !__GLIBC__ */
    /* There is a bug in VC6 C RTL: toxxx() functions dosn't do anything
       with signed chars < 0, so "fix" it here. */
    #define wxCRT_TolowerW(c)   towlower((wxUChar)(wxChar)(c))
    #define wxCRT_ToupperW(c)   towupper((wxUChar)(wxChar)(c))
#endif /* __GLIBC__/!__GLIBC__ */





/* -------------------------------------------------------------------------
       wx wrappers for CRT functions in both char* and wchar_t* versions
   ------------------------------------------------------------------------- */

#ifdef __cplusplus

/* NB: this belongs to wxcrt.h and not this header, but it makes life easier
 *     for buffer.h and stringimpl.h (both of which must be included before
 *     string.h, which is required by wxcrt.h) to have them here: */

/* safe version of strlen() (returns 0 if passed NULL pointer) */
inline size_t wxStrlen(const char *s) { return s ? wxCRT_StrlenA(s) : 0; }
inline size_t wxStrlen(const wchar_t *s) { return s ? wxCRT_StrlenW(s) : 0; }
#ifndef wxWCHAR_T_IS_WXCHAR16
       WXDLLIMPEXP_BASE size_t wxStrlen(const wxChar16 *s );
#endif
#ifndef wxWCHAR_T_IS_WXCHAR32
       WXDLLIMPEXP_BASE size_t wxStrlen(const wxChar32 *s );
#endif
#define wxWcslen wxCRT_StrlenW

#define wxStrdupA wxCRT_StrdupA
#define wxStrdupW wxCRT_StrdupW
inline char* wxStrdup(const char *s) { return wxCRT_StrdupA(s); }
inline wchar_t* wxStrdup(const wchar_t *s) { return wxCRT_StrdupW(s); }
#ifndef wxWCHAR_T_IS_WXCHAR16
       WXDLLIMPEXP_BASE wxChar16* wxStrdup(const wxChar16* s);
#endif
#ifndef wxWCHAR_T_IS_WXCHAR32
       WXDLLIMPEXP_BASE wxChar32* wxStrdup(const wxChar32* s);
#endif

#endif /* __cplusplus */

#endif /* _WX_WXCRTBASE_H_ */
