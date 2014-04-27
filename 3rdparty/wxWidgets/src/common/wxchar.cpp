/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/wxchar.cpp
// Purpose:     wxChar implementation
// Author:      Ove Kaven
// Modified by: Ron Lee, Francesco Montorsi
// Created:     09/04/99
// RCS-ID:      $Id: wxchar.cpp 58994 2009-02-18 15:49:09Z FM $
// Copyright:   (c) wxWidgets copyright
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// headers, declarations, constants
// ===========================================================================

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/wxchar.h"

#define _ISOC9X_SOURCE 1 // to get vsscanf()
#define _BSD_SOURCE    1 // to still get strdup()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __WXWINCE__
    #include <time.h>
    #include <locale.h>
#else
    #include "wx/msw/wince/time.h"
#endif

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/hash.h"
    #include "wx/utils.h"     // for wxMin and wxMax
    #include "wx/log.h"
#endif

#if defined(__WIN32__) && defined(wxNEED_WX_CTYPE_H)
  #include <windef.h>
    #include <winbase.h>
    #include <winnls.h>
    #include <winnt.h>
#endif

#if defined(__MWERKS__) && __MSL__ >= 0x6000
namespace std {}
using namespace std ;
#endif

#if wxUSE_WCHAR_T
size_t WXDLLEXPORT wxMB2WC(wchar_t *buf, const char *psz, size_t n)
{
  // assume that we have mbsrtowcs() too if we have wcsrtombs()
#ifdef HAVE_WCSRTOMBS
  mbstate_t mbstate;
  memset(&mbstate, 0, sizeof(mbstate_t));
#endif

  if (buf) {
    if (!n || !*psz) {
      if (n) *buf = wxT('\0');
      return 0;
    }
#ifdef HAVE_WCSRTOMBS
    return mbsrtowcs(buf, &psz, n, &mbstate);
#else
    return wxMbstowcs(buf, psz, n);
#endif
  }

  // note that we rely on common (and required by Unix98 but unfortunately not
  // C99) extension which allows to call mbs(r)towcs() with NULL output pointer
  // to just get the size of the needed buffer -- this is needed as otherwise
  // we have no idea about how much space we need and if the CRT doesn't
  // support it (the only currently known example being Metrowerks, see
  // wx/wxchar.h) we don't use its mbstowcs() at all
#ifdef HAVE_WCSRTOMBS
  return mbsrtowcs((wchar_t *) NULL, &psz, 0, &mbstate);
#else
  return wxMbstowcs((wchar_t *) NULL, psz, 0);
#endif
}

size_t WXDLLEXPORT wxWC2MB(char *buf, const wchar_t *pwz, size_t n)
{
#ifdef HAVE_WCSRTOMBS
  mbstate_t mbstate;
  memset(&mbstate, 0, sizeof(mbstate_t));
#endif

  if (buf) {
    if (!n || !*pwz) {
      // glibc2.1 chokes on null input
      if (n) *buf = '\0';
      return 0;
    }
#ifdef HAVE_WCSRTOMBS
    return wcsrtombs(buf, &pwz, n, &mbstate);
#else
    return wxWcstombs(buf, pwz, n);
#endif
  }

#ifdef HAVE_WCSRTOMBS
  return wcsrtombs((char *) NULL, &pwz, 0, &mbstate);
#else
  return wxWcstombs((char *) NULL, pwz, 0);
#endif
}
#endif // wxUSE_WCHAR_T

bool WXDLLEXPORT wxOKlibc()
{
#if wxUSE_WCHAR_T && defined(__UNIX__) && defined(__GLIBC__) && !defined(__WINE__)
  // glibc 2.0 uses UTF-8 even when it shouldn't
  wchar_t res = 0;
  if ((MB_CUR_MAX == 2) &&
      (wxMB2WC(&res, "\xdd\xa5", 1) == 1) &&
      (res==0x765)) {
    // this is UTF-8 allright, check whether that's what we want
    char *cur_locale = setlocale(LC_CTYPE, NULL);
    if ((strlen(cur_locale) < 4) ||
            (strcasecmp(cur_locale + strlen(cur_locale) - 4, "utf8")) ||
            (strcasecmp(cur_locale + strlen(cur_locale) - 5, "utf-8"))) {
      // nope, don't use libc conversion
      return false;
    }
  }
#endif
  return true;
}

// ============================================================================
// printf() functions business
// ============================================================================

// special test mode: define all functions below even if we don't really need
// them to be able to test them
#ifdef wxTEST_PRINTF
    #undef wxFprintf
    #undef wxPrintf
    #undef wxSprintf
    #undef wxVfprintf
    #undef wxVsprintf
    #undef wxVprintf
    #undef wxVsnprintf_
    #undef wxSnprintf_

    #define wxNEED_WPRINTF

    int wxVfprintf( FILE *stream, const wxChar *format, va_list argptr );
#endif

// ----------------------------------------------------------------------------
// implement [v]snprintf() if the system doesn't provide a safe one
// or if the system's one does not support positional parameters
// (very useful for i18n purposes)
// ----------------------------------------------------------------------------

#if !defined(wxVsnprintf_)

#if !wxUSE_WXVSNPRINTF
    #error wxUSE_WXVSNPRINTF must be 1 if our wxVsnprintf_ is used
#endif

// wxUSE_STRUTILS says our wxVsnprintf_ implementation to use or not to
// use wxStrlen and wxStrncpy functions over one-char processing loops.
//
// Some benchmarking revealed that wxUSE_STRUTILS == 1 has the following
// effects:
// -> on Windows:
//     when in ANSI mode, this setting does not change almost anything
//     when in Unicode mode, it gives ~ 50% of slowdown !
// -> on Linux:
//     both in ANSI and Unicode mode it gives ~ 60% of speedup !
//
#if defined(WIN32) && wxUSE_UNICODE
#define wxUSE_STRUTILS      0
#else
#define wxUSE_STRUTILS      1
#endif

// some limits of our implementation
#define wxMAX_SVNPRINTF_ARGUMENTS         64
#define wxMAX_SVNPRINTF_FLAGBUFFER_LEN    32
#define wxMAX_SVNPRINTF_SCRATCHBUFFER_LEN   512

// prefer snprintf over sprintf
#if defined(__VISUALC__) || \
        (defined(__BORLANDC__) && __BORLANDC__ >= 0x540)
    #define system_sprintf(buff, max, flags, data)      \
        ::_snprintf(buff, max, flags, data)
#elif defined(HAVE_SNPRINTF)
    #define system_sprintf(buff, max, flags, data)      \
        ::snprintf(buff, max, flags, data)
#else       // NB: at least sprintf() should always be available
    // since 'max' is not used in this case, wxVsnprintf() should always
    // ensure that 'buff' is big enough for all common needs
    // (see wxMAX_SVNPRINTF_FLAGBUFFER_LEN and wxMAX_SVNPRINTF_SCRATCHBUFFER_LEN)
    #define system_sprintf(buff, max, flags, data)      \
        ::sprintf(buff, flags, data)

    #define SYSTEM_SPRINTF_IS_UNSAFE
#endif

// the conversion specifiers accepted by wxVsnprintf_
enum wxPrintfArgType {
    wxPAT_INVALID = -1,

    wxPAT_INT,          // %d, %i, %o, %u, %x, %X
    wxPAT_LONGINT,      // %ld, etc
#ifdef wxLongLong_t
    wxPAT_LONGLONGINT,  // %Ld, etc
#endif
    wxPAT_SIZET,        // %Zd, etc

    wxPAT_DOUBLE,       // %e, %E, %f, %g, %G
    wxPAT_LONGDOUBLE,   // %le, etc

    wxPAT_POINTER,      // %p

    wxPAT_CHAR,         // %hc  (in ANSI mode: %c, too)
    wxPAT_WCHAR,        // %lc  (in Unicode mode: %c, too)

    wxPAT_PCHAR,        // %s   (related to a char *)
    wxPAT_PWCHAR,       // %s   (related to a wchar_t *)

    wxPAT_NINT,         // %n
    wxPAT_NSHORTINT,    // %hn
    wxPAT_NLONGINT      // %ln
};

// an argument passed to wxVsnprintf_
typedef union {
    int pad_int;                        //  %d, %i, %o, %u, %x, %X
    long int pad_longint;               // %ld, etc
#ifdef wxLongLong_t
    wxLongLong_t pad_longlongint;      // %Ld, etc
#endif
    size_t pad_sizet;                   // %Zd, etc

    double pad_double;                  // %e, %E, %f, %g, %G
    long double pad_longdouble;         // %le, etc

    void *pad_pointer;                  // %p

    char pad_char;                      // %hc  (in ANSI mode: %c, too)
    wchar_t pad_wchar;                  // %lc  (in Unicode mode: %c, too)

    char *pad_pchar;                    // %s   (related to a char *)
    wchar_t *pad_pwchar;                // %s   (related to a wchar_t *)

    int *pad_nint;                      // %n
    short int *pad_nshortint;           // %hn
    long int *pad_nlongint;             // %ln
} wxPrintfArg;


// Contains parsed data relative to a conversion specifier given to
// wxVsnprintf_ and parsed from the format string
// NOTE: in C++ there is almost no difference between struct & classes thus
//       there is no performance gain by using a struct here...
class wxPrintfConvSpec
{
public:

    // the position of the argument relative to this conversion specifier
    size_t m_pos;

    // the type of this conversion specifier
    wxPrintfArgType m_type;

    // the minimum and maximum width
    // when one of this var is set to -1 it means: use the following argument
    // in the stack as minimum/maximum width for this conversion specifier
    int m_nMinWidth, m_nMaxWidth;

    // does the argument need to the be aligned to left ?
    bool m_bAlignLeft;

    // pointer to the '%' of this conversion specifier in the format string
    // NOTE: this points somewhere in the string given to the Parse() function -
    //       it's task of the caller ensure that memory is still valid !
    const wxChar *m_pArgPos;

    // pointer to the last character of this conversion specifier in the
    // format string
    // NOTE: this points somewhere in the string given to the Parse() function -
    //       it's task of the caller ensure that memory is still valid !
    const wxChar *m_pArgEnd;

    // a little buffer where formatting flags like #+\.hlqLZ are stored by Parse()
    // for use in Process()
    // NB: even if this buffer is used only for numeric conversion specifiers and
    //     thus could be safely declared as a char[] buffer, we want it to be wxChar
    //     so that in Unicode builds we can avoid to convert its contents to Unicode
    //     chars when copying it in user's buffer.
    char m_szFlags[wxMAX_SVNPRINTF_FLAGBUFFER_LEN];


public:

    // we don't declare this as a constructor otherwise it would be called
    // automatically and we don't want this: to be optimized, wxVsnprintf_
    // calls this function only on really-used instances of this class.
    void Init();

    // Parses the first conversion specifier in the given string, which must
    // begin with a '%'. Returns false if the first '%' does not introduce a
    // (valid) conversion specifier and thus should be ignored.
    bool Parse(const wxChar *format);

    // Process this conversion specifier and puts the result in the given
    // buffer. Returns the number of characters written in 'buf' or -1 if
    // there's not enough space.
    int Process(wxChar *buf, size_t lenMax, wxPrintfArg *p, size_t written);

    // Loads the argument of this conversion specifier from given va_list.
    bool LoadArg(wxPrintfArg *p, va_list &argptr);

private:
    // An helper function of LoadArg() which is used to handle the '*' flag
    void ReplaceAsteriskWith(int w);
};

void wxPrintfConvSpec::Init()
{
    m_nMinWidth = 0;
    m_nMaxWidth = 0xFFFF;
    m_pos = 0;
    m_bAlignLeft = false;
    m_pArgPos = m_pArgEnd = NULL;
    m_type = wxPAT_INVALID;

    // this character will never be removed from m_szFlags array and
    // is important when calling sprintf() in wxPrintfConvSpec::Process() !
    m_szFlags[0] = '%';
}

bool wxPrintfConvSpec::Parse(const wxChar *format)
{
    bool done = false;

    // temporary parse data
    size_t flagofs = 1;
    bool in_prec,       // true if we found the dot in some previous iteration
         prec_dot;      // true if the dot has been already added to m_szFlags
    int ilen = 0;

    m_bAlignLeft = in_prec = prec_dot = false;
    m_pArgPos = m_pArgEnd = format;
    do
    {
#define CHECK_PREC \
        if (in_prec && !prec_dot) \
        { \
            m_szFlags[flagofs++] = '.'; \
            prec_dot = true; \
        }

        // what follows '%'?
        const wxChar ch = *(++m_pArgEnd);
        switch ( ch )
        {
            case wxT('\0'):
                return false;       // not really an argument

            case wxT('%'):
                return false;       // not really an argument

            case wxT('#'):
            case wxT('0'):
            case wxT(' '):
            case wxT('+'):
            case wxT('\''):
                CHECK_PREC
                m_szFlags[flagofs++] = char(ch);
                break;

            case wxT('-'):
                CHECK_PREC
                m_bAlignLeft = true;
                m_szFlags[flagofs++] = char(ch);
                break;

            case wxT('.'):
                CHECK_PREC
                in_prec = true;
                prec_dot = false;
                m_nMaxWidth = 0;
                // dot will be auto-added to m_szFlags if non-negative
                // number follows
                break;

            case wxT('h'):
                ilen = -1;
                CHECK_PREC
                m_szFlags[flagofs++] = char(ch);
                break;

            case wxT('l'):
                // NB: it's safe to use flagofs-1 as flagofs always start from 1
                if (m_szFlags[flagofs-1] == 'l')       // 'll' modifier is the same as 'L' or 'q'
                    ilen = 2;
                else
                ilen = 1;
                CHECK_PREC
                m_szFlags[flagofs++] = char(ch);
                break;

            case wxT('q'):
            case wxT('L'):
                ilen = 2;
                CHECK_PREC
                m_szFlags[flagofs++] = char(ch);
                break;
#ifdef __WXMSW__
            // under Windows we support the special '%I64' notation as longlong
            // integer conversion specifier for MSVC compatibility
            // (it behaves exactly as '%lli' or '%Li' or '%qi')
            case wxT('I'):
                if (*(m_pArgEnd+1) != wxT('6') ||
                    *(m_pArgEnd+2) != wxT('4'))
                    return false;       // bad format

                m_pArgEnd++;
                m_pArgEnd++;

                ilen = 2;
                CHECK_PREC
                m_szFlags[flagofs++] = char(ch);
                m_szFlags[flagofs++] = '6';
                m_szFlags[flagofs++] = '4';
                break;
#endif      // __WXMSW__

            case wxT('Z'):
                ilen = 3;
                CHECK_PREC
                m_szFlags[flagofs++] = char(ch);
                break;

            case wxT('*'):
                if (in_prec)
                {
                    CHECK_PREC

                    // tell Process() to use the next argument
                    // in the stack as maxwidth...
                    m_nMaxWidth = -1;
                }
                else
                {
                    // tell Process() to use the next argument
                    // in the stack as minwidth...
                    m_nMinWidth = -1;
                }

                // save the * in our formatting buffer...
                // will be replaced later by Process()
                m_szFlags[flagofs++] = char(ch);
                break;

            case wxT('1'): case wxT('2'): case wxT('3'):
            case wxT('4'): case wxT('5'): case wxT('6'):
            case wxT('7'): case wxT('8'): case wxT('9'):
                {
                    int len = 0;
                    CHECK_PREC
                    while ( (*m_pArgEnd >= wxT('0')) &&
                            (*m_pArgEnd <= wxT('9')) )
                    {
                        m_szFlags[flagofs++] = char(*m_pArgEnd);
                        len = len*10 + (*m_pArgEnd - wxT('0'));
                        m_pArgEnd++;
                    }

                    if (in_prec)
                        m_nMaxWidth = len;
                    else
                        m_nMinWidth = len;

                    m_pArgEnd--; // the main loop pre-increments n again
                }
                break;

            case wxT('$'):      // a positional parameter (e.g. %2$s) ?
                {
                    if (m_nMinWidth <= 0)
                        break;      // ignore this formatting flag as no
                                    // numbers are preceding it

                    // remove from m_szFlags all digits previously added
                    do {
                        flagofs--;
                    } while (m_szFlags[flagofs] >= '1' &&
                             m_szFlags[flagofs] <= '9');

                    // re-adjust the offset making it point to the
                    // next free char of m_szFlags
                    flagofs++;

                    m_pos = m_nMinWidth;
                    m_nMinWidth = 0;
                }
                break;

            case wxT('d'):
            case wxT('i'):
            case wxT('o'):
            case wxT('u'):
            case wxT('x'):
            case wxT('X'):
                CHECK_PREC
                m_szFlags[flagofs++] = char(ch);
                m_szFlags[flagofs] = '\0';
                if (ilen == 0)
                    m_type = wxPAT_INT;
                else if (ilen == -1)
                    // NB: 'short int' value passed through '...'
                    //      is promoted to 'int', so we have to get
                    //      an int from stack even if we need a short
                    m_type = wxPAT_INT;
                else if (ilen == 1)
                    m_type = wxPAT_LONGINT;
                else if (ilen == 2)
#ifdef wxLongLong_t
                    m_type = wxPAT_LONGLONGINT;
#else // !wxLongLong_t
                    m_type = wxPAT_LONGINT;
#endif // wxLongLong_t/!wxLongLong_t
                else if (ilen == 3)
                    m_type = wxPAT_SIZET;
                done = true;
                break;

            case wxT('e'):
            case wxT('E'):
            case wxT('f'):
            case wxT('g'):
            case wxT('G'):
                CHECK_PREC
                m_szFlags[flagofs++] = char(ch);
                m_szFlags[flagofs] = '\0';
                if (ilen == 2)
                    m_type = wxPAT_LONGDOUBLE;
                else
                    m_type = wxPAT_DOUBLE;
                done = true;
                break;

            case wxT('p'):
                m_type = wxPAT_POINTER;
                m_szFlags[flagofs++] = char(ch);
                m_szFlags[flagofs] = '\0';
                done = true;
                break;

            case wxT('c'):
                m_szFlags[flagofs++] = char(ch);
                m_szFlags[flagofs] = '\0';
                if (ilen == -1)
                {
                    // in Unicode mode %hc == ANSI character
                    // and in ANSI mode, %hc == %c == ANSI...
                    m_type = wxPAT_CHAR;
                }
                else if (ilen == 1)
                {
                    // in ANSI mode %lc == Unicode character
                    // and in Unicode mode, %lc == %c == Unicode...
                    m_type = wxPAT_WCHAR;
                }
                else
                {
#if wxUSE_UNICODE
                    // in Unicode mode, %c == Unicode character
                    m_type = wxPAT_WCHAR;
#else
                    // in ANSI mode, %c == ANSI character
                    m_type = wxPAT_CHAR;
#endif
                }
                done = true;
                break;

            case wxT('s'):
                m_szFlags[flagofs++] = char(ch);
                m_szFlags[flagofs] = '\0';
                if (ilen == -1)
                {
                    // Unicode mode wx extension: we'll let %hs mean non-Unicode
                    // strings (when in ANSI mode, %s == %hs == ANSI string)
                    m_type = wxPAT_PCHAR;
                }
                else if (ilen == 1)
                {
                    // in Unicode mode, %ls == %s == Unicode string
                    // in ANSI mode, %ls == Unicode string
                    m_type = wxPAT_PWCHAR;
                }
                else
                {
#if wxUSE_UNICODE
                    m_type = wxPAT_PWCHAR;
#else
                    m_type = wxPAT_PCHAR;
#endif
                }
                done = true;
                break;

            case wxT('n'):
                m_szFlags[flagofs++] = char(ch);
                m_szFlags[flagofs] = '\0';
                if (ilen == 0)
                    m_type = wxPAT_NINT;
                else if (ilen == -1)
                    m_type = wxPAT_NSHORTINT;
                else if (ilen >= 1)
                    m_type = wxPAT_NLONGINT;
                done = true;
                break;

            default:
                // bad format, don't consider this an argument;
                // leave it unchanged
                return false;
        }

        if (flagofs == wxMAX_SVNPRINTF_FLAGBUFFER_LEN)
        {
            wxLogDebug(wxT("Too many flags specified for a single conversion specifier!"));
            return false;
        }
    }
    while (!done);

    return true;        // parsing was successful
}


void wxPrintfConvSpec::ReplaceAsteriskWith(int width)
{
    char temp[wxMAX_SVNPRINTF_FLAGBUFFER_LEN];

    // find the first * in our flag buffer
    char *pwidth = strchr(m_szFlags, '*');
    wxCHECK_RET(pwidth, _T("field width must be specified"));

    // save what follows the * (the +1 is to skip the asterisk itself!)
    strcpy(temp, pwidth+1);
    if (width < 0)
    {
        pwidth[0] = wxT('-');
        pwidth++;
    }

    // replace * with the actual integer given as width
#ifndef SYSTEM_SPRINTF_IS_UNSAFE
    int maxlen = (m_szFlags + wxMAX_SVNPRINTF_FLAGBUFFER_LEN - pwidth) /
                        sizeof(*m_szFlags);
#endif
    int offset = system_sprintf(pwidth, maxlen, "%d", abs(width));

    // restore after the expanded * what was following it
    strcpy(pwidth+offset, temp);
}

bool wxPrintfConvSpec::LoadArg(wxPrintfArg *p, va_list &argptr)
{
    // was the '*' width/precision specifier used ?
    if (m_nMaxWidth == -1)
    {
        // take the maxwidth specifier from the stack
        m_nMaxWidth = va_arg(argptr, int);
        if (m_nMaxWidth < 0)
            m_nMaxWidth = 0;
        else
            ReplaceAsteriskWith(m_nMaxWidth);
    }

    if (m_nMinWidth == -1)
    {
        // take the minwidth specifier from the stack
        m_nMinWidth = va_arg(argptr, int);

        ReplaceAsteriskWith(m_nMinWidth);
        if (m_nMinWidth < 0)
        {
            m_bAlignLeft = !m_bAlignLeft;
            m_nMinWidth = -m_nMinWidth;
        }
    }

    switch (m_type) {
        case wxPAT_INT:
            p->pad_int = va_arg(argptr, int);
            break;
        case wxPAT_LONGINT:
            p->pad_longint = va_arg(argptr, long int);
            break;
#ifdef wxLongLong_t
        case wxPAT_LONGLONGINT:
            p->pad_longlongint = va_arg(argptr, wxLongLong_t);
            break;
#endif // wxLongLong_t
        case wxPAT_SIZET:
            p->pad_sizet = va_arg(argptr, size_t);
            break;
        case wxPAT_DOUBLE:
            p->pad_double = va_arg(argptr, double);
            break;
        case wxPAT_LONGDOUBLE:
            p->pad_longdouble = va_arg(argptr, long double);
            break;
        case wxPAT_POINTER:
            p->pad_pointer = va_arg(argptr, void *);
            break;

        case wxPAT_CHAR:
            p->pad_char = (char)va_arg(argptr, int);  // char is promoted to int when passed through '...'
            break;
        case wxPAT_WCHAR:
            p->pad_wchar = (wchar_t)va_arg(argptr, int);  // char is promoted to int when passed through '...'
            break;

        case wxPAT_PCHAR:
            p->pad_pchar = va_arg(argptr, char *);
            break;
        case wxPAT_PWCHAR:
            p->pad_pwchar = va_arg(argptr, wchar_t *);
            break;

        case wxPAT_NINT:
            p->pad_nint = va_arg(argptr, int *);
            break;
        case wxPAT_NSHORTINT:
            p->pad_nshortint = va_arg(argptr, short int *);
            break;
        case wxPAT_NLONGINT:
            p->pad_nlongint = va_arg(argptr, long int *);
            break;

        case wxPAT_INVALID:
        default:
            return false;
    }

    return true;    // loading was successful
}

int wxPrintfConvSpec::Process(wxChar *buf, size_t lenMax, wxPrintfArg *p, size_t written)
{
    // buffer to avoid dynamic memory allocation each time for small strings;
    // note that this buffer is used only to hold results of number formatting,
    // %s directly writes user's string in buf, without using szScratch
    char szScratch[wxMAX_SVNPRINTF_SCRATCHBUFFER_LEN];
    size_t lenScratch = 0, lenCur = 0;

#define APPEND_CH(ch) \
                { \
                    if ( lenCur == lenMax ) \
                        return -1; \
                    \
                    buf[lenCur++] = ch; \
                }

#define APPEND_STR(s) \
                { \
                    for ( const wxChar *p = s; *p; p++ ) \
                    { \
                        APPEND_CH(*p); \
                    } \
                }

    switch ( m_type )
    {
        case wxPAT_INT:
            lenScratch = system_sprintf(szScratch, wxMAX_SVNPRINTF_SCRATCHBUFFER_LEN, m_szFlags, p->pad_int);
            break;

        case wxPAT_LONGINT:
            lenScratch = system_sprintf(szScratch, wxMAX_SVNPRINTF_SCRATCHBUFFER_LEN, m_szFlags, p->pad_longint);
            break;

#ifdef wxLongLong_t
        case wxPAT_LONGLONGINT:
            lenScratch = system_sprintf(szScratch, wxMAX_SVNPRINTF_SCRATCHBUFFER_LEN, m_szFlags, p->pad_longlongint);
            break;
#endif // SIZEOF_LONG_LONG

        case wxPAT_SIZET:
            lenScratch = system_sprintf(szScratch, wxMAX_SVNPRINTF_SCRATCHBUFFER_LEN, m_szFlags, p->pad_sizet);
            break;

        case wxPAT_LONGDOUBLE:
            lenScratch = system_sprintf(szScratch, wxMAX_SVNPRINTF_SCRATCHBUFFER_LEN, m_szFlags, p->pad_longdouble);
            break;

        case wxPAT_DOUBLE:
            lenScratch = system_sprintf(szScratch, wxMAX_SVNPRINTF_SCRATCHBUFFER_LEN, m_szFlags, p->pad_double);
            break;

        case wxPAT_POINTER:
            lenScratch = system_sprintf(szScratch, wxMAX_SVNPRINTF_SCRATCHBUFFER_LEN, m_szFlags, p->pad_pointer);
            break;

        case wxPAT_CHAR:
        case wxPAT_WCHAR:
            {
                wxChar val =
#if wxUSE_UNICODE
                    p->pad_wchar;

                if (m_type == wxPAT_CHAR)
                {
                    // user passed a character explicitely indicated as ANSI...
                    const char buf[2] = { p->pad_char, 0 };
                    val = wxString(buf, wxConvLibc)[0u];

                    //wprintf(L"converting ANSI=>Unicode");   // for debug
                }
#else
                    p->pad_char;

#if wxUSE_WCHAR_T
                if (m_type == wxPAT_WCHAR)
                {
                    // user passed a character explicitely indicated as Unicode...
                    const wchar_t buf[2] = { p->pad_wchar, 0 };
                    val = wxString(buf, wxConvLibc)[0u];

                    //printf("converting Unicode=>ANSI");   // for debug
                }
#endif
#endif

                size_t i;

                if (!m_bAlignLeft)
                    for (i = 1; i < (size_t)m_nMinWidth; i++)
                        APPEND_CH(_T(' '));

                APPEND_CH(val);

                if (m_bAlignLeft)
                    for (i = 1; i < (size_t)m_nMinWidth; i++)
                        APPEND_CH(_T(' '));
            }
            break;

        case wxPAT_PCHAR:
        case wxPAT_PWCHAR:
            {
                wxString s;
                const wxChar *val =
#if wxUSE_UNICODE
                    p->pad_pwchar;

                if (m_type == wxPAT_PCHAR)
                {
                    // user passed a string explicitely indicated as ANSI...
                    val = s = wxString(p->pad_pchar, wxConvLibc);

                    //wprintf(L"converting ANSI=>Unicode");   // for debug
                }
#else
                    p->pad_pchar;

#if wxUSE_WCHAR_T
                if (m_type == wxPAT_PWCHAR)
                {
                    // user passed a string explicitely indicated as Unicode...
                    val = s = wxString(p->pad_pwchar, wxConvLibc);

                    //printf("converting Unicode=>ANSI");   // for debug
                }
#endif
#endif
                int len;

                if (val)
                {
#if wxUSE_STRUTILS
                    // at this point we are sure that m_nMaxWidth is positive or null
                    // (see top of wxPrintfConvSpec::LoadArg)
                    len = wxMin((unsigned int)m_nMaxWidth, wxStrlen(val));
#else
                    for ( len = 0; val[len] && (len < m_nMaxWidth); len++ )
                        ;
#endif
                }
                else if (m_nMaxWidth >= 6)
                {
                    val = wxT("(null)");
                    len = 6;
                }
                else
                {
                    val = wxEmptyString;
                    len = 0;
                }

                int i;

                if (!m_bAlignLeft)
                {
                    for (i = len; i < m_nMinWidth; i++)
                        APPEND_CH(_T(' '));
                }

#if wxUSE_STRUTILS
                len = wxMin((unsigned int)len, lenMax-lenCur);
                wxStrncpy(buf+lenCur, val, len);
                lenCur += len;
#else
                for (i = 0; i < len; i++)
                    APPEND_CH(val[i]);
#endif

                if (m_bAlignLeft)
                {
                    for (i = len; i < m_nMinWidth; i++)
                        APPEND_CH(_T(' '));
                }
            }
            break;

        case wxPAT_NINT:
            *p->pad_nint = written;
            break;

        case wxPAT_NSHORTINT:
            *p->pad_nshortint = (short int)written;
            break;

        case wxPAT_NLONGINT:
            *p->pad_nlongint = written;
            break;

        case wxPAT_INVALID:
        default:
            return -1;
    }

    // if we used system's sprintf() then we now need to append the s_szScratch
    // buffer to the given one...
    switch (m_type)
    {
        case wxPAT_INT:
        case wxPAT_LONGINT:
#ifdef wxLongLong_t
        case wxPAT_LONGLONGINT:
#endif
        case wxPAT_SIZET:
        case wxPAT_LONGDOUBLE:
        case wxPAT_DOUBLE:
        case wxPAT_POINTER:
            wxASSERT(lenScratch < wxMAX_SVNPRINTF_SCRATCHBUFFER_LEN);
#if !wxUSE_UNICODE
            {
                if (lenMax < lenScratch)
                {
                    // fill output buffer and then return -1
                    wxStrncpy(buf, szScratch, lenMax);
                    return -1;
                }
                wxStrncpy(buf, szScratch, lenScratch);
                lenCur += lenScratch;
            }
#else
            {
                // Copy the char scratch to the wide output. This requires
                // conversion, but we can optimise by making use of the fact
                // that we are formatting numbers, this should mean only 7-bit
                // ascii characters are involved.
                wxChar *bufptr = buf;
                const wxChar *bufend = buf + lenMax;
                const char *scratchptr = szScratch;

                // Simply copy each char to a wxChar, stopping on the first
                // null or non-ascii byte. Checking '(signed char)*scratchptr
                // > 0' is an extra optimisation over '*scratchptr != 0 &&
                // isascii(*scratchptr)', though it assumes signed char is
                // 8-bit 2 complement.
                while ((signed char)*scratchptr > 0 && bufptr != bufend)
                    *bufptr++ = *scratchptr++;

                if (bufptr == bufend)
                    return -1;

                lenCur += bufptr - buf;

                // check if the loop stopped on a non-ascii char, if yes then
                // fall back to wxMB2WX
                if (*scratchptr)
                {
                    size_t len = wxMB2WX(bufptr, scratchptr, bufend - bufptr);

                    if (len && len != (size_t)(-1))
                        if (bufptr[len - 1])
                            return -1;
                        else
                            lenCur += len;
                }
            }
#endif
            break;

        default:
            break;      // all other cases were completed previously
    }

    return lenCur;
}

// Copy chars from source to dest converting '%%' to '%'. Takes at most maxIn
// chars from source and write at most outMax chars to dest, returns the
// number of chars actually written. Does not treat null specially.
//
static int wxCopyStrWithPercents(
        size_t maxOut,
        wxChar *dest,
        size_t maxIn,
        const wxChar *source)
{
    size_t written = 0;

    if (maxIn == 0)
        return 0;

    size_t i;
    for ( i = 0; i < maxIn-1 && written < maxOut; source++, i++)
    {
        dest[written++] = *source;
        if (*(source+1) == wxT('%'))
        {
            // skip this additional '%' character
            source++;
            i++;
        }
    }

    if (i < maxIn && written < maxOut)
        // copy last character inconditionally
        dest[written++] = *source;

    return written;
}

int WXDLLEXPORT wxVsnprintf_(wxChar *buf, size_t lenMax,
                             const wxChar *format, va_list argptr)
{
    // useful for debugging, to understand if we are really using this function
    // rather than the system implementation
#if 0
    wprintf(L"Using wxVsnprintf_\n");
#endif

    // required memory:
    wxPrintfConvSpec arg[wxMAX_SVNPRINTF_ARGUMENTS];
    wxPrintfArg argdata[wxMAX_SVNPRINTF_ARGUMENTS];
    wxPrintfConvSpec *pspec[wxMAX_SVNPRINTF_ARGUMENTS] = { NULL };

    size_t i;

    // number of characters in the buffer so far, must be less than lenMax
    size_t lenCur = 0;

    size_t nargs = 0;
    const wxChar *toparse = format;

    // parse the format string
    bool posarg_present = false, nonposarg_present = false;
    for (; *toparse != wxT('\0'); toparse++)
    {
        if (*toparse == wxT('%') )
        {
            arg[nargs].Init();

            // let's see if this is a (valid) conversion specifier...
            if (arg[nargs].Parse(toparse))
            {
                // ...yes it is
                wxPrintfConvSpec *current = &arg[nargs];

                // make toparse point to the end of this specifier
                toparse = current->m_pArgEnd;

                if (current->m_pos > 0)
                {
                    // the positionals start from number 1... adjust the index
                    current->m_pos--;
                    posarg_present = true;
                }
                else
                {
                    // not a positional argument...
                    current->m_pos = nargs;
                    nonposarg_present = true;
                }

                // this conversion specifier is tied to the pos-th argument...
                pspec[current->m_pos] = current;
                nargs++;

                if (nargs == wxMAX_SVNPRINTF_ARGUMENTS)
                {
                    wxLogDebug(wxT("A single call to wxVsnprintf() has more than %d arguments; ")
                               wxT("ignoring all remaining arguments."), wxMAX_SVNPRINTF_ARGUMENTS);
                    break;  // cannot handle any additional conv spec
                }
            }
            else
            {
                // it's safe to look in the next character of toparse as at worst
                // we'll hit its \0
                if (*(toparse+1) == wxT('%'))
                    toparse++;      // the Parse() returned false because we've found a %%
            }
        }
    }

    if (posarg_present && nonposarg_present)
    {
        buf[0] = 0;
        return -1;      // format strings with both positional and
    }                   // non-positional conversion specifier are unsupported !!

    // on platforms where va_list is an array type, it is necessary to make a
    // copy to be able to pass it to LoadArg as a reference.
    bool ok = true;
    va_list ap;
    wxVaCopy(ap, argptr);

    // now load arguments from stack
    for (i=0; i < nargs && ok; i++)
    {
        // !pspec[i] means that the user forgot a positional parameter (e.g. %$1s %$3s);
        // LoadArg == false means that wxPrintfConvSpec::Parse failed to set the
        // conversion specifier 'type' to a valid value...
        ok = pspec[i] && pspec[i]->LoadArg(&argdata[i], ap);
    }

    va_end(ap);

    // something failed while loading arguments from the variable list...
    // (e.g. the user repeated twice the same positional argument)
    if (!ok)
    {
        buf[0] = 0;
        return -1;
    }

    // finally, process each conversion specifier with its own argument
    toparse = format;
    for (i=0; i < nargs; i++)
    {
        // copy in the output buffer the portion of the format string between
        // last specifier and the current one
        size_t tocopy = ( arg[i].m_pArgPos - toparse );

        lenCur += wxCopyStrWithPercents(lenMax - lenCur, buf + lenCur,
                                        tocopy, toparse);
        if (lenCur == lenMax)
        {
            buf[lenMax - 1] = 0;
            return lenMax+1;      // not enough space in the output buffer !
        }

        // process this specifier directly in the output buffer
        int n = arg[i].Process(buf+lenCur, lenMax - lenCur, &argdata[arg[i].m_pos], lenCur);
        if (n == -1)
        {
            buf[lenMax-1] = wxT('\0');  // be sure to always NUL-terminate the string
            return lenMax+1;      // not enough space in the output buffer !
        }
        lenCur += n;

        // the +1 is because wxPrintfConvSpec::m_pArgEnd points to the last character
        // of the format specifier, but we are not interested to it...
        toparse = arg[i].m_pArgEnd + 1;
    }

    // copy portion of the format string after last specifier
    // NOTE: toparse is pointing to the character just after the last processed
    //       conversion specifier
    // NOTE2: the +1 is because we want to copy also the '\0'
    size_t tocopy = wxStrlen(format) + 1  - ( toparse - format ) ;

    lenCur += wxCopyStrWithPercents(lenMax - lenCur, buf + lenCur,
                                    tocopy, toparse) - 1;
    if (buf[lenCur])
    {
        buf[lenCur] = 0;
        return lenMax+1;     // not enough space in the output buffer !
    }

    // Don't do:
    //      wxASSERT(lenCur == wxStrlen(buf));
    // in fact if we embedded NULLs in the output buffer (using %c with a '\0')
    // such check would fail

    return lenCur;
}

#undef APPEND_CH
#undef APPEND_STR
#undef CHECK_PREC

#else    // wxVsnprintf_ is defined

#if wxUSE_WXVSNPRINTF
    #error wxUSE_WXVSNPRINTF must be 0 if our wxVsnprintf_ is not used
#endif

#endif // !wxVsnprintf_

#if !defined(wxSnprintf_)
int WXDLLEXPORT wxSnprintf_(wxChar *buf, size_t len, const wxChar *format, ...)
{
    va_list argptr;
    va_start(argptr, format);

    int iLen = wxVsnprintf_(buf, len, format, argptr);

    va_end(argptr);

    return iLen;
}
#endif // wxSnprintf_

#if defined(__DMC__)
    /* Digital Mars adds count to _stprintf (C99) so convert */
    #if wxUSE_UNICODE
        int wxSprintf (wchar_t * __RESTRICT s, const wchar_t * __RESTRICT format, ... )
        {
            va_list arglist;

            va_start( arglist, format );
            int iLen = swprintf ( s, -1, format, arglist );
            va_end( arglist );
            return iLen ;
        }

    #endif // wxUSE_UNICODE

#endif //__DMC__

#if defined(__MINGW32__) && ( defined(_STLPORT_VERSION) && _STLPORT_VERSION >= 0x510 )
    /* MinGW with STLPort 5.1 has clashing defines for _stprintf so use swprintf */
    /* STLPort 5.1 defines standard (C99) vswprintf() and swprintf() that takes count. */
    int wxSprintf (wchar_t * s, const wchar_t * format, ... )
    {
        va_list arglist;

        va_start( arglist, format );
        int iLen = swprintf ( s, -1, format, arglist );
        va_end( arglist );
        return iLen ;
    }
#endif // MINGW32 _STLPORT_VERSION >= 0x510

// ----------------------------------------------------------------------------
// implement the standard IO functions for wide char if libc doesn't have them
// ----------------------------------------------------------------------------

#ifdef wxNEED_FPUTS
int wxFputs(const wchar_t *ws, FILE *stream)
{
    wxCharBuffer buf(wxConvLibc.cWC2MB(ws));
    if ( !buf )
        return -1;

    // counting the number of wide characters written isn't worth the trouble,
    // simply distinguish between ok and error
    return fputs(buf, stream) == -1 ? -1 : 0;
}
#endif // wxNEED_FPUTS

#ifdef wxNEED_PUTS
int wxPuts(const wxChar *ws)
{
    int rc = wxFputs(ws, stdout);
    if ( rc != -1 )
    {
        if ( wxFputs(L"\n", stdout) == -1 )
            return -1;

        rc++;
    }

    return rc;
}
#endif // wxNEED_PUTS

#ifdef wxNEED_PUTC
int /* not wint_t */ wxPutc(wchar_t wc, FILE *stream)
{
    wchar_t ws[2] = { wc, L'\0' };

    return wxFputs(ws, stream);
}
#endif // wxNEED_PUTC

// NB: we only implement va_list functions here, the ones taking ... are
//     defined below for wxNEED_PRINTF_CONVERSION case anyhow and we reuse
//     the definitions there to avoid duplicating them here
#ifdef wxNEED_WPRINTF

// TODO: implement the scanf() functions
int vwscanf(const wxChar *format, va_list argptr)
{
    wxFAIL_MSG( _T("TODO") );

    return -1;
}

int vswscanf(const wxChar *ws, const wxChar *format, va_list argptr)
{
    // The best we can do without proper Unicode support in glibc is to
    // convert the strings into MB representation and run ANSI version
    // of the function. This doesn't work with %c and %s because of difference
    // in size of char and wchar_t, though.

    wxCHECK_MSG( wxStrstr(format, _T("%s")) == NULL, -1,
                 _T("incomplete vswscanf implementation doesn't allow %s") );
    wxCHECK_MSG( wxStrstr(format, _T("%c")) == NULL, -1,
                 _T("incomplete vswscanf implementation doesn't allow %c") );

    va_list argcopy;
    wxVaCopy(argcopy, argptr);
    return vsscanf(wxConvLibc.cWX2MB(ws), wxConvLibc.cWX2MB(format), argcopy);
}

int vfwscanf(FILE *stream, const wxChar *format, va_list argptr)
{
    wxFAIL_MSG( _T("TODO") );

    return -1;
}

#define vswprintf wxVsnprintf_

int vfwprintf(FILE *stream, const wxChar *format, va_list argptr)
{
    wxString s;
    int rc = s.PrintfV(format, argptr);

    if ( rc != -1 )
    {
        // we can't do much better without Unicode support in libc...
        if ( fprintf(stream, "%s", (const char*)s.mb_str() ) == -1 )
            return -1;
    }

    return rc;
}

int vwprintf(const wxChar *format, va_list argptr)
{
    return wxVfprintf(stdout, format, argptr);
}

#endif // wxNEED_WPRINTF

#ifdef wxNEED_PRINTF_CONVERSION

// ----------------------------------------------------------------------------
// wxFormatConverter: class doing the "%s" -> "%ls" conversion
// ----------------------------------------------------------------------------

/*
   Here are the gory details. We want to follow the Windows/MS conventions,
   that is to have

   In ANSI mode:

   format specifier         results in
   -----------------------------------
   %c, %hc, %hC             char
   %lc, %C, %lC             wchar_t

   In Unicode mode:

   format specifier         results in
   -----------------------------------
   %hc, %C, %hC             char
   %c, %lc, %lC             wchar_t


   while on POSIX systems we have %C identical to %lc and %c always means char
   (in any mode) while %lc always means wchar_t,

   So to use native functions in order to get our semantics we must do the
   following translations in Unicode mode (nothing to do in ANSI mode):

   wxWidgets specifier      POSIX specifier
   ----------------------------------------

   %hc, %C, %hC             %c
   %c                       %lc


   And, of course, the same should be done for %s as well.
*/

class wxFormatConverter
{
public:
    wxFormatConverter(const wxChar *format);

    // notice that we only translated the string if m_fmtOrig == NULL (as set
    // by CopyAllBefore()), otherwise we should simply use the original format
    operator const wxChar *() const
        { return m_fmtOrig ? m_fmtOrig : m_fmt.c_str(); }

private:
    // copy another character to the translated format: this function does the
    // copy if we are translating but doesn't do anything at all if we don't,
    // so we don't create the translated format string at all unless we really
    // need to (i.e. InsertFmtChar() is called)
    wxChar CopyFmtChar(wxChar ch)
    {
        if ( !m_fmtOrig )
        {
            // we're translating, do copy
            m_fmt += ch;
        }
        else
        {
            // simply increase the count which should be copied by
            // CopyAllBefore() later if needed
            m_nCopied++;
        }

        return ch;
    }

    // insert an extra character
    void InsertFmtChar(wxChar ch)
    {
        if ( m_fmtOrig )
        {
            // so far we haven't translated anything yet
            CopyAllBefore();
        }

        m_fmt += ch;
    }

    void CopyAllBefore()
    {
        wxASSERT_MSG( m_fmtOrig && m_fmt.empty(), _T("logic error") );

        m_fmt = wxString(m_fmtOrig, m_nCopied);

        // we won't need it any longer
        m_fmtOrig = NULL;
    }

    static bool IsFlagChar(wxChar ch)
    {
        return ch == _T('-') || ch == _T('+') ||
               ch == _T('0') || ch == _T(' ') || ch == _T('#');
    }

    void SkipDigits(const wxChar **ptpc)
    {
        while ( **ptpc >= _T('0') && **ptpc <= _T('9') )
            CopyFmtChar(*(*ptpc)++);
    }

    // the translated format
    wxString m_fmt;

    // the original format
    const wxChar *m_fmtOrig;

    // the number of characters already copied
    size_t m_nCopied;
};

wxFormatConverter::wxFormatConverter(const wxChar *format)
{
    m_fmtOrig = format;
    m_nCopied = 0;

    while ( *format )
    {
        if ( CopyFmtChar(*format++) == _T('%') )
        {
            // skip any flags
            while ( IsFlagChar(*format) )
                CopyFmtChar(*format++);

            // and possible width
            if ( *format == _T('*') )
                CopyFmtChar(*format++);
            else
                SkipDigits(&format);

            // precision?
            if ( *format == _T('.') )
            {
                CopyFmtChar(*format++);
                if ( *format == _T('*') )
                    CopyFmtChar(*format++);
                else
                    SkipDigits(&format);
            }

            // next we can have a size modifier
            enum
            {
                Default,
                Short,
                Long
            } size;

            switch ( *format )
            {
                case _T('h'):
                    size = Short;
                    format++;
                    break;

                case _T('l'):
                    // "ll" has a different meaning!
                    if ( format[1] != _T('l') )
                    {
                        size = Long;
                        format++;
                        break;
                    }
                    //else: fall through

                default:
                    size = Default;
            }

            // and finally we should have the type
            switch ( *format )
            {
                case _T('C'):
                case _T('S'):
                    // %C and %hC -> %c and %lC -> %lc
                    if ( size == Long )
                        CopyFmtChar(_T('l'));

                    InsertFmtChar(*format++ == _T('C') ? _T('c') : _T('s'));
                    break;

                case _T('c'):
                case _T('s'):
                    // %c -> %lc but %hc stays %hc and %lc is still %lc
                    if ( size == Default)
                        InsertFmtChar(_T('l'));
                    // fall through

                default:
                    // nothing special to do
                    if ( size != Default )
                        CopyFmtChar(*(format - 1));
                    CopyFmtChar(*format++);
            }
        }
    }
}

#else // !wxNEED_PRINTF_CONVERSION
    // no conversion necessary
    #define wxFormatConverter(x) (x)
#endif // wxNEED_PRINTF_CONVERSION/!wxNEED_PRINTF_CONVERSION

#ifdef __WXDEBUG__
// For testing the format converter
wxString wxConvertFormat(const wxChar *format)
{
    return wxString(wxFormatConverter(format));
}
#endif

// ----------------------------------------------------------------------------
// wxPrintf(), wxScanf() and relatives
// ----------------------------------------------------------------------------

#if defined(wxNEED_PRINTF_CONVERSION) || defined(wxNEED_WPRINTF)

int wxScanf( const wxChar *format, ... )
{
    va_list argptr;
    va_start(argptr, format);

    int ret = vwscanf(wxFormatConverter(format), argptr );

    va_end(argptr);

    return ret;
}

int wxSscanf( const wxChar *str, const wxChar *format, ... )
{
    va_list argptr;
    va_start(argptr, format);

    int ret = vswscanf( str, wxFormatConverter(format), argptr );

    va_end(argptr);

    return ret;
}

int wxFscanf( FILE *stream, const wxChar *format, ... )
{
    va_list argptr;
    va_start(argptr, format);
    int ret = vfwscanf(stream, wxFormatConverter(format), argptr);

    va_end(argptr);

    return ret;
}

int wxPrintf( const wxChar *format, ... )
{
    va_list argptr;
    va_start(argptr, format);

    int ret = vwprintf( wxFormatConverter(format), argptr );

    va_end(argptr);

    return ret;
}

#ifndef wxSnprintf
int wxSnprintf( wxChar *str, size_t size, const wxChar *format, ... )
{
    va_list argptr;
    va_start(argptr, format);

    int ret = vswprintf( str, size, wxFormatConverter(format), argptr );

    // VsnprintfTestCase reveals that glibc's implementation of vswprintf
    // doesn't nul terminate on truncation.
    str[size - 1] = 0;

    va_end(argptr);

    return ret;
}
#endif // wxSnprintf

int wxSprintf( wxChar *str, const wxChar *format, ... )
{
    va_list argptr;
    va_start(argptr, format);

    // note that wxString::FormatV() uses wxVsnprintf(), not wxSprintf(), so
    // it's safe to implement this one in terms of it
    wxString s(wxString::FormatV(format, argptr));
    wxStrcpy(str, s);

    va_end(argptr);

    return s.length();
}

int wxFprintf( FILE *stream, const wxChar *format, ... )
{
    va_list argptr;
    va_start( argptr, format );

    int ret = vfwprintf( stream, wxFormatConverter(format), argptr );

    va_end(argptr);

    return ret;
}

int wxVsscanf( const wxChar *str, const wxChar *format, va_list argptr )
{
    return vswscanf( str, wxFormatConverter(format), argptr );
}

int wxVfprintf( FILE *stream, const wxChar *format, va_list argptr )
{
    return vfwprintf( stream, wxFormatConverter(format), argptr );
}

int wxVprintf( const wxChar *format, va_list argptr )
{
    return vwprintf( wxFormatConverter(format), argptr );
}

#ifndef wxVsnprintf
int wxVsnprintf( wxChar *str, size_t size, const wxChar *format, va_list argptr )
{
    return vswprintf( str, size, wxFormatConverter(format), argptr );
}
#endif // wxVsnprintf

int wxVsprintf( wxChar *str, const wxChar *format, va_list argptr )
{
    // same as for wxSprintf()
    return vswprintf(str, INT_MAX / 4, wxFormatConverter(format), argptr);
}

#endif // wxNEED_PRINTF_CONVERSION

#if wxUSE_WCHAR_T

// ----------------------------------------------------------------------------
// ctype.h stuff (currently unused)
// ----------------------------------------------------------------------------

#if defined(__WIN32__) && defined(wxNEED_WX_CTYPE_H)
inline WORD wxMSW_ctype(wxChar ch)
{
  WORD ret;
  GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, &ch, 1, &ret);
  return ret;
}

WXDLLEXPORT int wxIsalnum(wxChar ch) { return IsCharAlphaNumeric(ch); }
WXDLLEXPORT int wxIsalpha(wxChar ch) { return IsCharAlpha(ch); }
WXDLLEXPORT int wxIscntrl(wxChar ch) { return wxMSW_ctype(ch) & C1_CNTRL; }
WXDLLEXPORT int wxIsdigit(wxChar ch) { return wxMSW_ctype(ch) & C1_DIGIT; }
WXDLLEXPORT int wxIsgraph(wxChar ch) { return wxMSW_ctype(ch) & (C1_DIGIT|C1_PUNCT|C1_ALPHA); }
WXDLLEXPORT int wxIslower(wxChar ch) { return IsCharLower(ch); }
WXDLLEXPORT int wxIsprint(wxChar ch) { return wxMSW_ctype(ch) & (C1_DIGIT|C1_SPACE|C1_PUNCT|C1_ALPHA); }
WXDLLEXPORT int wxIspunct(wxChar ch) { return wxMSW_ctype(ch) & C1_PUNCT; }
WXDLLEXPORT int wxIsspace(wxChar ch) { return wxMSW_ctype(ch) & C1_SPACE; }
WXDLLEXPORT int wxIsupper(wxChar ch) { return IsCharUpper(ch); }
WXDLLEXPORT int wxIsxdigit(wxChar ch) { return wxMSW_ctype(ch) & C1_XDIGIT; }
WXDLLEXPORT int wxTolower(wxChar ch) { return (wxChar)CharLower((LPTSTR)(ch)); }
WXDLLEXPORT int wxToupper(wxChar ch) { return (wxChar)CharUpper((LPTSTR)(ch)); }
#endif

#ifdef wxNEED_WX_MBSTOWCS

WXDLLEXPORT size_t wxMbstowcs (wchar_t * out, const char * in, size_t outlen)
{
    if (!out)
    {
        size_t outsize = 0;
        while(*in++)
            outsize++;
        return outsize;
    }

    const char* origin = in;

    while (outlen-- && *in)
    {
        *out++ = (wchar_t) *in++;
    }

    *out = '\0';

    return in - origin;
}

WXDLLEXPORT size_t wxWcstombs (char * out, const wchar_t * in, size_t outlen)
{
    if (!out)
    {
        size_t outsize = 0;
        while(*in++)
            outsize++;
        return outsize;
    }

    const wchar_t* origin = in;

    while (outlen-- && *in)
    {
        *out++ = (char) *in++;
    }

    *out = '\0';

    return in - origin;
}

#endif // wxNEED_WX_MBSTOWCS

#if defined(wxNEED_WX_CTYPE_H)

#include <CoreFoundation/CoreFoundation.h>

#define cfalnumset CFCharacterSetGetPredefined(kCFCharacterSetAlphaNumeric)
#define cfalphaset CFCharacterSetGetPredefined(kCFCharacterSetLetter)
#define cfcntrlset CFCharacterSetGetPredefined(kCFCharacterSetControl)
#define cfdigitset CFCharacterSetGetPredefined(kCFCharacterSetDecimalDigit)
//CFCharacterSetRef cfgraphset = kCFCharacterSetControl && !' '
#define cflowerset CFCharacterSetGetPredefined(kCFCharacterSetLowercaseLetter)
//CFCharacterSetRef cfprintset = !kCFCharacterSetControl
#define cfpunctset CFCharacterSetGetPredefined(kCFCharacterSetPunctuation)
#define cfspaceset CFCharacterSetGetPredefined(kCFCharacterSetWhitespaceAndNewline)
#define cfupperset CFCharacterSetGetPredefined(kCFCharacterSetUppercaseLetter)

WXDLLEXPORT int wxIsalnum(wxChar ch) { return CFCharacterSetIsCharacterMember(cfalnumset, ch); }
WXDLLEXPORT int wxIsalpha(wxChar ch) { return CFCharacterSetIsCharacterMember(cfalphaset, ch); }
WXDLLEXPORT int wxIscntrl(wxChar ch) { return CFCharacterSetIsCharacterMember(cfcntrlset, ch); }
WXDLLEXPORT int wxIsdigit(wxChar ch) { return CFCharacterSetIsCharacterMember(cfdigitset, ch); }
WXDLLEXPORT int wxIsgraph(wxChar ch) { return !CFCharacterSetIsCharacterMember(cfcntrlset, ch) && ch != ' '; }
WXDLLEXPORT int wxIslower(wxChar ch) { return CFCharacterSetIsCharacterMember(cflowerset, ch); }
WXDLLEXPORT int wxIsprint(wxChar ch) { return !CFCharacterSetIsCharacterMember(cfcntrlset, ch); }
WXDLLEXPORT int wxIspunct(wxChar ch) { return CFCharacterSetIsCharacterMember(cfpunctset, ch); }
WXDLLEXPORT int wxIsspace(wxChar ch) { return CFCharacterSetIsCharacterMember(cfspaceset, ch); }
WXDLLEXPORT int wxIsupper(wxChar ch) { return CFCharacterSetIsCharacterMember(cfupperset, ch); }
WXDLLEXPORT int wxIsxdigit(wxChar ch) { return wxIsdigit(ch) || (ch>='a' && ch<='f') || (ch>='A' && ch<='F'); }
WXDLLEXPORT int wxTolower(wxChar ch) { return (wxChar)tolower((char)(ch)); }
WXDLLEXPORT int wxToupper(wxChar ch) { return (wxChar)toupper((char)(ch)); }

#endif  // wxNEED_WX_CTYPE_H

#ifndef wxStrdupA

WXDLLEXPORT char *wxStrdupA(const char *s)
{
    return strcpy((char *)malloc(strlen(s) + 1), s);
}

#endif // wxStrdupA

#ifndef wxStrdupW

WXDLLEXPORT wchar_t * wxStrdupW(const wchar_t *pwz)
{
  size_t size = (wxWcslen(pwz) + 1) * sizeof(wchar_t);
  wchar_t *ret = (wchar_t *) malloc(size);
  memcpy(ret, pwz, size);
  return ret;
}

#endif // wxStrdupW

#ifndef wxStricmp
int WXDLLEXPORT wxStricmp(const wxChar *psz1, const wxChar *psz2)
{
  register wxChar c1, c2;
  do {
    c1 = wxTolower(*psz1++);
    c2 = wxTolower(*psz2++);
  } while ( c1 && (c1 == c2) );
  return c1 - c2;
}
#endif

#ifndef wxStricmp
int WXDLLEXPORT wxStrnicmp(const wxChar *s1, const wxChar *s2, size_t n)
{
  // initialize the variables just to suppress stupid gcc warning
  register wxChar c1 = 0, c2 = 0;
  while (n && ((c1 = wxTolower(*s1)) == (c2 = wxTolower(*s2)) ) && c1) n--, s1++, s2++;
  if (n) {
    if (c1 < c2) return -1;
    if (c1 > c2) return 1;
  }
  return 0;
}
#endif

#ifndef wxSetlocale
WXDLLEXPORT wxWCharBuffer wxSetlocale(int category, const wxChar *locale)
{
    char *localeOld = setlocale(category, wxConvLibc.cWX2MB(locale));

    return wxWCharBuffer(wxConvLibc.cMB2WC(localeOld));
}
#endif

#if wxUSE_WCHAR_T && !defined(HAVE_WCSLEN)
WXDLLEXPORT size_t wxWcslen(const wchar_t *s)
{
    size_t n = 0;
    while ( *s++ )
        n++;

    return n;
}
#endif

// ----------------------------------------------------------------------------
// string.h functions
// ----------------------------------------------------------------------------

#ifdef wxNEED_WX_STRING_H

// RN:  These need to be c externed for the regex lib
#ifdef __cplusplus
extern "C" {
#endif

WXDLLEXPORT wxChar * wxStrcat(wxChar *dest, const wxChar *src)
{
  wxChar *ret = dest;
  while (*dest) dest++;
  while ((*dest++ = *src++));
  return ret;
}

WXDLLEXPORT const wxChar * wxStrchr(const wxChar *s, wxChar c)
{
    // be careful here as the terminating NUL makes part of the string
    while ( *s != c )
    {
        if ( !*s++ )
            return NULL;
    }

    return s;
}

WXDLLEXPORT int wxStrcmp(const wxChar *s1, const wxChar *s2)
{
  while ((*s1 == *s2) && *s1) s1++, s2++;
  if ((wxUChar)*s1 < (wxUChar)*s2) return -1;
  if ((wxUChar)*s1 > (wxUChar)*s2) return 1;
  return 0;
}

WXDLLEXPORT wxChar * wxStrcpy(wxChar *dest, const wxChar *src)
{
  wxChar *ret = dest;
  while ((*dest++ = *src++));
  return ret;
}

WXDLLEXPORT size_t wxStrlen_(const wxChar *s)
{
    size_t n = 0;
    while ( *s++ )
        n++;

    return n;
}


WXDLLEXPORT wxChar * wxStrncat(wxChar *dest, const wxChar *src, size_t n)
{
  wxChar *ret = dest;
  while (*dest) dest++;
  while (n && (*dest++ = *src++)) n--;
  return ret;
}

WXDLLEXPORT int wxStrncmp(const wxChar *s1, const wxChar *s2, size_t n)
{
  while (n && (*s1 == *s2) && *s1) n--, s1++, s2++;
  if (n) {
    if ((wxUChar)*s1 < (wxUChar)*s2) return -1;
    if ((wxUChar)*s1 > (wxUChar)*s2) return 1;
  }
  return 0;
}

WXDLLEXPORT wxChar * wxStrncpy(wxChar *dest, const wxChar *src, size_t n)
{
  wxChar *ret = dest;
  while (n && (*dest++ = *src++)) n--;
  while (n) *dest++=0, n--; // the docs specify padding with zeroes
  return ret;
}

WXDLLEXPORT const wxChar * wxStrpbrk(const wxChar *s, const wxChar *accept)
{
  while (*s && !wxStrchr(accept, *s))
      s++;

  return *s ? s : NULL;
}

WXDLLEXPORT const wxChar * wxStrrchr(const wxChar *s, wxChar c)
{
    const wxChar *ret = NULL;
    do
    {
        if ( *s == c )
            ret = s;
        s++;
    }
    while ( *s );

    return ret;
}

WXDLLEXPORT size_t wxStrspn(const wxChar *s, const wxChar *accept)
{
  size_t len = 0;
  while (wxStrchr(accept, *s++)) len++;
  return len;
}

WXDLLEXPORT const wxChar *wxStrstr(const wxChar *haystack, const wxChar *needle)
{
    wxASSERT_MSG( needle != NULL, _T("NULL argument in wxStrstr") );

    // VZ: this is not exactly the most efficient string search algorithm...

    const size_t len = wxStrlen(needle);

    while ( const wxChar *fnd = wxStrchr(haystack, *needle) )
    {
        if ( !wxStrncmp(fnd, needle, len) )
            return fnd;

        haystack = fnd + 1;
    }

    return NULL;
}

#ifdef __cplusplus
}
#endif

WXDLLEXPORT double wxStrtod(const wxChar *nptr, wxChar **endptr)
{
  const wxChar decSep(
#if wxUSE_INTL
      wxLocale::GetInfo(wxLOCALE_DECIMAL_POINT, wxLOCALE_CAT_NUMBER)[0]
#else
      _T('.')
#endif
      );
  const wxChar *start = nptr;

  while (wxIsspace(*nptr)) nptr++;
  if (*nptr == wxT('+') || *nptr == wxT('-')) nptr++;
  while (wxIsdigit(*nptr)) nptr++;
  if (*nptr == decSep) {
    nptr++;
    while (wxIsdigit(*nptr)) nptr++;
  }
  if (*nptr == wxT('E') || *nptr == wxT('e')) {
    nptr++;
    if (*nptr == wxT('+') || *nptr == wxT('-')) nptr++;
    while (wxIsdigit(*nptr)) nptr++;
  }

  wxString data(start, nptr-start);
  const wxWX2MBbuf dat = data.mb_str(wxConvLibc);
  char *rdat = wxMBSTRINGCAST dat;
  double ret = strtod(dat, &rdat);

  if (endptr) *endptr = (wxChar *)(start + (rdat - (const char *)dat));

  return ret;
}

WXDLLEXPORT long int wxStrtol(const wxChar *nptr, wxChar **endptr, int base)
{
  const wxChar *start = nptr;

  while (wxIsspace(*nptr)) nptr++;
  if (*nptr == wxT('+') || *nptr == wxT('-')) nptr++;
  if (((base == 0) || (base == 16)) &&
      (nptr[0] == wxT('0') && nptr[1] == wxT('x'))) {
    nptr += 2;
    base = 16;
  }
  else if ((base == 0) && (nptr[0] == wxT('0'))) base = 8;
  else if (base == 0) base = 10;

  while ((wxIsdigit(*nptr) && (*nptr - wxT('0') < base)) ||
         (wxIsalpha(*nptr) && (wxToupper(*nptr) - wxT('A') + 10 < base))) nptr++;

  wxString data(start, nptr-start);
  wxWX2MBbuf dat = data.mb_str(wxConvLibc);
  char *rdat = wxMBSTRINGCAST dat;
  long int ret = strtol(dat, &rdat, base);

  if (endptr) *endptr = (wxChar *)(start + (rdat - (const char *)dat));

  return ret;
}

WXDLLEXPORT unsigned long int wxStrtoul(const wxChar *nptr, wxChar **endptr, int base)
{
    return (unsigned long int) wxStrtol(nptr, endptr, base);
}

#endif // wxNEED_WX_STRING_H

#ifdef wxNEED_WX_STDIO_H
WXDLLEXPORT FILE * wxFopen(const wxChar *path, const wxChar *mode)
{
    char mode_buffer[10];
    for (size_t i = 0; i < wxStrlen(mode)+1; i++)
       mode_buffer[i] = (char) mode[i];

    return fopen( wxConvFile.cWX2MB(path), mode_buffer );
}

WXDLLEXPORT FILE * wxFreopen(const wxChar *path, const wxChar *mode, FILE *stream)
{
    char mode_buffer[10];
    for (size_t i = 0; i < wxStrlen(mode)+1; i++)
       mode_buffer[i] = (char) mode[i];

    return freopen( wxConvFile.cWX2MB(path), mode_buffer, stream );
}

WXDLLEXPORT int wxRemove(const wxChar *path)
{
    return remove( wxConvFile.cWX2MB(path) );
}

WXDLLEXPORT int wxRename(const wxChar *oldpath, const wxChar *newpath)
{
    return rename( wxConvFile.cWX2MB(oldpath), wxConvFile.cWX2MB(newpath) );
}
#endif

#ifndef wxAtof
double   WXDLLEXPORT wxAtof(const wxChar *psz)
{
#ifdef __WXWINCE__
    double d;
    wxString str(psz);
    if (str.ToDouble(& d))
        return d;

    return 0.0;
#else
    return atof(wxConvLibc.cWX2MB(psz));
#endif
}
#endif

#ifdef wxNEED_WX_STDLIB_H
int      WXDLLEXPORT wxAtoi(const wxChar *psz)
{
  return atoi(wxConvLibc.cWX2MB(psz));
}

long     WXDLLEXPORT wxAtol(const wxChar *psz)
{
  return atol(wxConvLibc.cWX2MB(psz));
}

wxChar * WXDLLEXPORT wxGetenv(const wxChar *name)
{
#if wxUSE_UNICODE
    // NB: buffer returned by getenv() is allowed to be overwritten next
    //     time getenv() is called, so it is OK to use static string
    //     buffer to hold the data.
    static wxWCharBuffer value((wxChar*)NULL);
    value = wxConvLibc.cMB2WX(getenv(wxConvLibc.cWX2MB(name)));
    return value.data();
#else
    return getenv(name);
#endif
}
#endif // wxNEED_WX_STDLIB_H

#ifdef wxNEED_WXSYSTEM
int WXDLLEXPORT wxSystem(const wxChar *psz)
{
    return system(wxConvLibc.cWX2MB(psz));
}
#endif // wxNEED_WXSYSTEM

#ifdef wxNEED_WX_TIME_H
WXDLLEXPORT size_t
wxStrftime(wxChar *s, size_t maxsize, const wxChar *fmt, const struct tm *tm)
{
    if ( !maxsize )
        return 0;

    wxCharBuffer buf(maxsize);

    wxCharBuffer bufFmt(wxConvLibc.cWX2MB(fmt));
    if ( !bufFmt )
        return 0;

    size_t ret = strftime(buf.data(), maxsize, bufFmt, tm);
    if  ( !ret )
        return 0;

    wxWCharBuffer wbuf = wxConvLibc.cMB2WX(buf);
    if ( !wbuf )
        return 0;

    wxStrncpy(s, wbuf, maxsize);
    return wxStrlen(s);
}
#endif // wxNEED_WX_TIME_H

#ifndef wxCtime
WXDLLEXPORT wxChar *wxCtime(const time_t *timep)
{
    // normally the string is 26 chars but give one more in case some broken
    // DOS compiler decides to use "\r\n" instead of "\n" at the end
    static wxChar buf[27];

    // ctime() is guaranteed to return a string containing only ASCII
    // characters, as its format is always the same for any locale
    wxStrncpy(buf, wxString::FromAscii(ctime(timep)), WXSIZEOF(buf));
    buf[WXSIZEOF(buf) - 1] = _T('\0');

    return buf;
}
#endif // wxCtime

#endif // wxUSE_WCHAR_T

// ----------------------------------------------------------------------------
// functions which we may need even if !wxUSE_WCHAR_T
// ----------------------------------------------------------------------------

#ifndef wxStrtok

WXDLLEXPORT wxChar * wxStrtok(wxChar *psz, const wxChar *delim, wxChar **save_ptr)
{
    if (!psz)
    {
        psz = *save_ptr;
        if ( !psz )
            return NULL;
    }

    psz += wxStrspn(psz, delim);
    if (!*psz)
    {
        *save_ptr = (wxChar *)NULL;
        return (wxChar *)NULL;
    }

    wxChar *ret = psz;
    psz = wxStrpbrk(psz, delim);
    if (!psz)
    {
        *save_ptr = (wxChar*)NULL;
    }
    else
    {
        *psz = wxT('\0');
        *save_ptr = psz + 1;
    }

    return ret;
}

#endif // wxStrtok

// ----------------------------------------------------------------------------
// missing C RTL functions
// ----------------------------------------------------------------------------

#ifdef wxNEED_STRDUP

char *strdup(const char *s)
{
    char *dest = (char*) malloc( strlen( s ) + 1 ) ;
    if ( dest )
        strcpy( dest , s ) ;
    return dest ;
}
#endif // wxNEED_STRDUP

#if defined(__WXWINCE__) && (_WIN32_WCE <= 211)

void *calloc( size_t num, size_t size )
{
    void** ptr = (void **)malloc(num * size);
    memset( ptr, 0, num * size);
    return ptr;
}

#endif // __WXWINCE__ <= 211

#ifdef __WXWINCE__

int wxRemove(const wxChar *path)
{
    return ::DeleteFile(path) == 0;
}

#endif
