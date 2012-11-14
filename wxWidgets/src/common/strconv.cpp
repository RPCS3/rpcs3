/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/strconv.cpp
// Purpose:     Unicode conversion classes
// Author:      Ove Kaaven, Robert Roebling, Vadim Zeitlin, Vaclav Slavik,
//              Ryan Norton, Fredrik Roubert (UTF7)
// Modified by:
// Created:     29/01/98
// RCS-ID:      $Id: strconv.cpp 64156 2010-04-27 08:52:30Z VZ $
// Copyright:   (c) 1999 Ove Kaaven, Robert Roebling, Vaclav Slavik
//              (c) 2000-2003 Vadim Zeitlin
//              (c) 2004 Ryan Norton, Fredrik Roubert
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #ifdef __WXMSW__
        #include "wx/msw/missing.h"
    #endif
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/utils.h"
    #include "wx/hashmap.h"
#endif

#include "wx/strconv.h"

#if wxUSE_WCHAR_T

#ifdef __WINDOWS__
    #include "wx/msw/private.h"
#endif

#ifndef __WXWINCE__
#include <errno.h>
#endif

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#if defined(__WIN32__) && !defined(__WXMICROWIN__)
    #define wxHAVE_WIN32_MB2WC
#endif

#ifdef __SALFORDC__
    #include <clib.h>
#endif

#ifdef HAVE_ICONV
    #include <iconv.h>
    #include "wx/thread.h"
#endif

#include "wx/encconv.h"
#include "wx/fontmap.h"

#ifdef __WXMAC__
#ifndef __DARWIN__
#include <ATSUnicode.h>
#include <TextCommon.h>
#include <TextEncodingConverter.h>
#endif

// includes Mac headers
#include "wx/mac/private.h"
#include "wx/thread.h"

#endif


#define TRACE_STRCONV _T("strconv")

// WC_UTF16 is defined only if sizeof(wchar_t) == 2, otherwise it's supposed to
// be 4 bytes
#if SIZEOF_WCHAR_T == 2
    #define WC_UTF16
#endif


// ============================================================================
// implementation
// ============================================================================

// helper function of cMB2WC(): check if n bytes at this location are all NUL
static bool NotAllNULs(const char *p, size_t n)
{
    while ( n && *p++ == '\0' )
        n--;

    return n != 0;
}

// ----------------------------------------------------------------------------
// UTF-16 en/decoding to/from UCS-4 with surrogates handling
// ----------------------------------------------------------------------------

static size_t encode_utf16(wxUint32 input, wxUint16 *output)
{
    if (input <= 0xffff)
    {
        if (output)
            *output = (wxUint16) input;

        return 1;
    }
    else if (input >= 0x110000)
    {
        return wxCONV_FAILED;
    }
    else
    {
        if (output)
        {
            *output++ = (wxUint16) ((input >> 10) + 0xd7c0);
            *output = (wxUint16) ((input & 0x3ff) + 0xdc00);
        }

        return 2;
    }
}

static size_t decode_utf16(const wxUint16* input, wxUint32& output)
{
    if ((*input < 0xd800) || (*input > 0xdfff))
    {
        output = *input;
        return 1;
    }
    else if ((input[1] < 0xdc00) || (input[1] > 0xdfff))
    {
        output = *input;
        return wxCONV_FAILED;
    }
    else
    {
        output = ((input[0] - 0xd7c0) << 10) + (input[1] - 0xdc00);
        return 2;
    }
}

#ifdef WC_UTF16
    typedef wchar_t wxDecodeSurrogate_t;
#else // !WC_UTF16
    typedef wxUint16 wxDecodeSurrogate_t;
#endif // WC_UTF16/!WC_UTF16

// returns the next UTF-32 character from the wchar_t buffer and advances the
// pointer to the character after this one
//
// if an invalid character is found, *pSrc is set to NULL, the caller must
// check for this
static wxUint32 wxDecodeSurrogate(const wxDecodeSurrogate_t **pSrc)
{
    wxUint32 out;
    const size_t
        n = decode_utf16(wx_reinterpret_cast(const wxUint16 *, *pSrc), out);
    if ( n == wxCONV_FAILED )
        *pSrc = NULL;
    else
        *pSrc += n;

    return out;
}

// ----------------------------------------------------------------------------
// wxMBConv
// ----------------------------------------------------------------------------

size_t
wxMBConv::ToWChar(wchar_t *dst, size_t dstLen,
                  const char *src, size_t srcLen) const
{
    // although new conversion classes are supposed to implement this function
    // directly, the existins ones only implement the old MB2WC() and so, to
    // avoid to have to rewrite all conversion classes at once, we provide a
    // default (but not efficient) implementation of this one in terms of the
    // old function by copying the input to ensure that it's NUL-terminated and
    // then using MB2WC() to convert it

    // the number of chars [which would be] written to dst [if it were not NULL]
    size_t dstWritten = 0;

    // the number of NULs terminating this string
    size_t nulLen = 0;  // not really needed, but just to avoid warnings

    // if we were not given the input size we just have to assume that the
    // string is properly terminated as we have no way of knowing how long it
    // is anyhow, but if we do have the size check whether there are enough
    // NULs at the end
    wxCharBuffer bufTmp;
    const char *srcEnd;
    if ( srcLen != wxNO_LEN )
    {
        // we need to know how to find the end of this string
        nulLen = GetMBNulLen();
        if ( nulLen == wxCONV_FAILED )
            return wxCONV_FAILED;

        // if there are enough NULs we can avoid the copy
        if ( srcLen < nulLen || NotAllNULs(src + srcLen - nulLen, nulLen) )
        {
            // make a copy in order to properly NUL-terminate the string
            bufTmp = wxCharBuffer(srcLen + nulLen - 1 /* 1 will be added */);
            char * const p = bufTmp.data();
            memcpy(p, src, srcLen);
            for ( char *s = p + srcLen; s < p + srcLen + nulLen; s++ )
                *s = '\0';

            src = bufTmp;
        }

        srcEnd = src + srcLen;
    }
    else // quit after the first loop iteration
    {
        srcEnd = NULL;
    }

    for ( ;; )
    {
        // try to convert the current chunk
        size_t lenChunk = MB2WC(NULL, src, 0);
        if ( lenChunk == wxCONV_FAILED )
            return wxCONV_FAILED;

        lenChunk++; // for the L'\0' at the end of this chunk

        dstWritten += lenChunk;

        if ( lenChunk == 1 )
        {
            // nothing left in the input string, conversion succeeded
            break;
        }

        if ( dst )
        {
            if ( dstWritten > dstLen )
                return wxCONV_FAILED;

            if ( MB2WC(dst, src, lenChunk) == wxCONV_FAILED )
                return wxCONV_FAILED;

            dst += lenChunk;
        }

        if ( !srcEnd )
        {
            // we convert just one chunk in this case as this is the entire
            // string anyhow
            break;
        }

        // advance the input pointer past the end of this chunk
        while ( NotAllNULs(src, nulLen) )
        {
            // notice that we must skip over multiple bytes here as we suppose
            // that if NUL takes 2 or 4 bytes, then all the other characters do
            // too and so if advanced by a single byte we might erroneously
            // detect sequences of NUL bytes in the middle of the input
            src += nulLen;
        }

        src += nulLen; // skipping over its terminator as well

        // note that ">=" (and not just "==") is needed here as the terminator
        // we skipped just above could be inside or just after the buffer
        // delimited by inEnd
        if ( src >= srcEnd )
            break;
    }

    return dstWritten;
}

size_t
wxMBConv::FromWChar(char *dst, size_t dstLen,
                    const wchar_t *src, size_t srcLen) const
{
    // the number of chars [which would be] written to dst [if it were not NULL]
    size_t dstWritten = 0;

    // make a copy of the input string unless it is already properly
    // NUL-terminated
    //
    // if we don't know its length we have no choice but to assume that it is,
    // indeed, properly terminated
    wxWCharBuffer bufTmp;
    if ( srcLen == wxNO_LEN )
    {
        srcLen = wxWcslen(src) + 1;
    }
    else if ( srcLen != 0 && src[srcLen - 1] != L'\0' )
    {
        // make a copy in order to properly NUL-terminate the string
        bufTmp = wxWCharBuffer(srcLen);
        memcpy(bufTmp.data(), src, srcLen * sizeof(wchar_t));
        src = bufTmp;
    }

    const size_t lenNul = GetMBNulLen();
    for ( const wchar_t * const srcEnd = src + srcLen;
          src < srcEnd;
          src += wxWcslen(src) + 1 /* skip L'\0' too */ )
    {
        // try to convert the current chunk
        size_t lenChunk = WC2MB(NULL, src, 0);

        if ( lenChunk == wxCONV_FAILED )
            return wxCONV_FAILED;

        lenChunk += lenNul;
        dstWritten += lenChunk;

        if ( dst )
        {
            if ( dstWritten > dstLen )
                return wxCONV_FAILED;

            if ( WC2MB(dst, src, lenChunk) == wxCONV_FAILED )
                return wxCONV_FAILED;

            dst += lenChunk;
        }
    }

    return dstWritten;
}

size_t wxMBConv::MB2WC(wchar_t *outBuff, const char *inBuff, size_t outLen) const
{
    size_t rc = ToWChar(outBuff, outLen, inBuff);
    if ( rc != wxCONV_FAILED )
    {
        // ToWChar() returns the buffer length, i.e. including the trailing
        // NUL, while this method doesn't take it into account
        rc--;
    }

    return rc;
}

size_t wxMBConv::WC2MB(char *outBuff, const wchar_t *inBuff, size_t outLen) const
{
    size_t rc = FromWChar(outBuff, outLen, inBuff);
    if ( rc != wxCONV_FAILED )
    {
        rc -= GetMBNulLen();
    }

    return rc;
}

wxMBConv::~wxMBConv()
{
    // nothing to do here (necessary for Darwin linking probably)
}

const wxWCharBuffer wxMBConv::cMB2WC(const char *psz) const
{
    if ( psz )
    {
        // calculate the length of the buffer needed first
        const size_t nLen = MB2WC(NULL, psz, 0);
        if ( nLen != wxCONV_FAILED )
        {
            // now do the actual conversion
            wxWCharBuffer buf(nLen /* +1 added implicitly */);

            // +1 for the trailing NULL
            if ( MB2WC(buf.data(), psz, nLen + 1) != wxCONV_FAILED )
                return buf;
        }
    }

    return wxWCharBuffer();
}

const wxCharBuffer wxMBConv::cWC2MB(const wchar_t *pwz) const
{
    if ( pwz )
    {
        const size_t nLen = WC2MB(NULL, pwz, 0);
        if ( nLen != wxCONV_FAILED )
        {
            // extra space for trailing NUL(s)
            static const size_t extraLen = GetMaxMBNulLen();

            wxCharBuffer buf(nLen + extraLen - 1);
            if ( WC2MB(buf.data(), pwz, nLen + extraLen) != wxCONV_FAILED )
                return buf;
        }
    }

    return wxCharBuffer();
}

const wxWCharBuffer
wxMBConv::cMB2WC(const char *inBuff, size_t inLen, size_t *outLen) const
{
    const size_t dstLen = ToWChar(NULL, 0, inBuff, inLen);
    if ( dstLen != wxCONV_FAILED )
    {
        wxWCharBuffer wbuf(dstLen - 1);
        if ( ToWChar(wbuf.data(), dstLen, inBuff, inLen) != wxCONV_FAILED )
        {
            if ( outLen )
            {
                *outLen = dstLen;
                if ( wbuf[dstLen - 1] == L'\0' )
                    (*outLen)--;
            }

            return wbuf;
        }
    }

    if ( outLen )
        *outLen = 0;

    return wxWCharBuffer();
}

const wxCharBuffer
wxMBConv::cWC2MB(const wchar_t *inBuff, size_t inLen, size_t *outLen) const
{
    size_t dstLen = FromWChar(NULL, 0, inBuff, inLen);
    if ( dstLen != wxCONV_FAILED )
    {
        // special case of empty input: can't allocate 0 size buffer below as
        // wxCharBuffer insists on NUL-terminating it
        wxCharBuffer buf(dstLen ? dstLen - 1 : 1);
        if ( FromWChar(buf.data(), dstLen, inBuff, inLen) != wxCONV_FAILED )
        {
            if ( outLen )
            {
                *outLen = dstLen;

                const size_t nulLen = GetMBNulLen();
                if ( dstLen >= nulLen &&
                        !NotAllNULs(buf.data() + dstLen - nulLen, nulLen) )
                {
                    // in this case the output is NUL-terminated and we're not
                    // supposed to count NUL
                    *outLen -= nulLen;
                }
            }

            return buf;
        }
    }

    if ( outLen )
        *outLen = 0;

    return wxCharBuffer();
}

// ----------------------------------------------------------------------------
// wxMBConvLibc
// ----------------------------------------------------------------------------

size_t wxMBConvLibc::MB2WC(wchar_t *buf, const char *psz, size_t n) const
{
    return wxMB2WC(buf, psz, n);
}

size_t wxMBConvLibc::WC2MB(char *buf, const wchar_t *psz, size_t n) const
{
    return wxWC2MB(buf, psz, n);
}

// ----------------------------------------------------------------------------
// wxConvBrokenFileNames
// ----------------------------------------------------------------------------

#ifdef __UNIX__

wxConvBrokenFileNames::wxConvBrokenFileNames(const wxChar *charset)
{
    if ( !charset || wxStricmp(charset, _T("UTF-8")) == 0
                  || wxStricmp(charset, _T("UTF8")) == 0  )
        m_conv = new wxMBConvUTF8(wxMBConvUTF8::MAP_INVALID_UTF8_TO_PUA);
    else
        m_conv = new wxCSConv(charset);
}

#endif // __UNIX__

// ----------------------------------------------------------------------------
// UTF-7
// ----------------------------------------------------------------------------

// Implementation (C) 2004 Fredrik Roubert

//
// BASE64 decoding table
//
static const unsigned char utf7unb64[] =
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
    0x3c, 0x3d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
    0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
    0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

size_t wxMBConvUTF7::MB2WC(wchar_t *buf, const char *psz, size_t n) const
{
    size_t len = 0;

    while ( *psz && (!buf || (len < n)) )
    {
        unsigned char cc = *psz++;
        if (cc != '+')
        {
            // plain ASCII char
            if (buf)
                *buf++ = cc;
            len++;
        }
        else if (*psz == '-')
        {
            // encoded plus sign
            if (buf)
                *buf++ = cc;
            len++;
            psz++;
        }
        else // start of BASE64 encoded string
        {
            bool lsb, ok;
            unsigned int d, l;
            for ( ok = lsb = false, d = 0, l = 0;
                  (cc = utf7unb64[(unsigned char)*psz]) != 0xff;
                  psz++ )
            {
                d <<= 6;
                d += cc;
                for (l += 6; l >= 8; lsb = !lsb)
                {
                    unsigned char c = (unsigned char)((d >> (l -= 8)) % 256);
                    if (lsb)
                    {
                        if (buf)
                            *buf++ |= c;
                        len ++;
                    }
                    else
                    {
                        if (buf)
                            *buf = (wchar_t)(c << 8);
                    }

                    ok = true;
                }
            }

            if ( !ok )
            {
                // in valid UTF7 we should have valid characters after '+'
                return wxCONV_FAILED;
            }

            if (*psz == '-')
                psz++;
        }
    }

    if ( buf && (len < n) )
        *buf = '\0';

    return len;
}

//
// BASE64 encoding table
//
static const unsigned char utf7enb64[] =
{
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'
};

//
// UTF-7 encoding table
//
// 0 - Set D (directly encoded characters)
// 1 - Set O (optional direct characters)
// 2 - whitespace characters (optional)
// 3 - special characters
//
static const unsigned char utf7encode[128] =
{
    3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 2, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 3, 0, 0, 0, 3,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 3, 3
};

size_t wxMBConvUTF7::WC2MB(char *buf, const wchar_t *psz, size_t n) const
{
    size_t len = 0;

    while (*psz && ((!buf) || (len < n)))
    {
        wchar_t cc = *psz++;
        if (cc < 0x80 && utf7encode[cc] < 1)
        {
            // plain ASCII char
            if (buf)
                *buf++ = (char)cc;

            len++;
        }
#ifndef WC_UTF16
        else if (((wxUint32)cc) > 0xffff)
        {
            // no surrogate pair generation (yet?)
            return wxCONV_FAILED;
        }
#endif
        else
        {
            if (buf)
                *buf++ = '+';

            len++;
            if (cc != '+')
            {
                // BASE64 encode string
                unsigned int lsb, d, l;
                for (d = 0, l = 0; /*nothing*/; psz++)
                {
                    for (lsb = 0; lsb < 2; lsb ++)
                    {
                        d <<= 8;
                        d += lsb ? cc & 0xff : (cc & 0xff00) >> 8;

                        for (l += 8; l >= 6; )
                        {
                            l -= 6;
                            if (buf)
                                *buf++ = utf7enb64[(d >> l) % 64];
                            len++;
                        }
                    }

                    cc = *psz;
                    if (!(cc) || (cc < 0x80 && utf7encode[cc] < 1))
                        break;
                }

                if (l != 0)
                {
                    if (buf)
                        *buf++ = utf7enb64[((d % 16) << (6 - l)) % 64];

                    len++;
                }
            }

            if (buf)
                *buf++ = '-';
            len++;
        }
    }

    if (buf && (len < n))
        *buf = 0;

    return len;
}

// ----------------------------------------------------------------------------
// UTF-8
// ----------------------------------------------------------------------------

static wxUint32 utf8_max[]=
    { 0x7f, 0x7ff, 0xffff, 0x1fffff, 0x3ffffff, 0x7fffffff, 0xffffffff };

// boundaries of the private use area we use to (temporarily) remap invalid
// characters invalid in a UTF-8 encoded string
const wxUint32 wxUnicodePUA = 0x100000;
const wxUint32 wxUnicodePUAEnd = wxUnicodePUA + 256;

size_t wxMBConvUTF8::MB2WC(wchar_t *buf, const char *psz, size_t n) const
{
    size_t len = 0;

    while (*psz && ((!buf) || (len < n)))
    {
        const char *opsz = psz;
        bool invalid = false;
        unsigned char cc = *psz++, fc = cc;
        unsigned cnt;
        for (cnt = 0; fc & 0x80; cnt++)
            fc <<= 1;

        if (!cnt)
        {
            // plain ASCII char
            if (buf)
                *buf++ = cc;
            len++;

            // escape the escape character for octal escapes
            if ((m_options & MAP_INVALID_UTF8_TO_OCTAL)
                    && cc == '\\' && (!buf || len < n))
            {
                if (buf)
                    *buf++ = cc;
                len++;
            }
        }
        else
        {
            cnt--;
            if (!cnt)
            {
                // invalid UTF-8 sequence
                invalid = true;
            }
            else
            {
                unsigned ocnt = cnt - 1;
                wxUint32 res = cc & (0x3f >> cnt);
                while (cnt--)
                {
                    cc = *psz;
                    if ((cc & 0xC0) != 0x80)
                    {
                        // invalid UTF-8 sequence
                        invalid = true;
                        break;
                    }

                    psz++;
                    res = (res << 6) | (cc & 0x3f);
                }

                if (invalid || res <= utf8_max[ocnt])
                {
                    // illegal UTF-8 encoding
                    invalid = true;
                }
                else if ((m_options & MAP_INVALID_UTF8_TO_PUA) &&
                        res >= wxUnicodePUA && res < wxUnicodePUAEnd)
                {
                    // if one of our PUA characters turns up externally
                    // it must also be treated as an illegal sequence
                    // (a bit like you have to escape an escape character)
                    invalid = true;
                }
                else
                {
#ifdef WC_UTF16
                    // cast is ok because wchar_t == wxUuint16 if WC_UTF16
                    size_t pa = encode_utf16(res, (wxUint16 *)buf);
                    if (pa == wxCONV_FAILED)
                    {
                        invalid = true;
                    }
                    else
                    {
                        if (buf)
                            buf += pa;
                        len += pa;
                    }
#else // !WC_UTF16
                    if (buf)
                        *buf++ = (wchar_t)res;
                    len++;
#endif // WC_UTF16/!WC_UTF16
                }
            }

            if (invalid)
            {
                if (m_options & MAP_INVALID_UTF8_TO_PUA)
                {
                    while (opsz < psz && (!buf || len < n))
                    {
#ifdef WC_UTF16
                        // cast is ok because wchar_t == wxUuint16 if WC_UTF16
                        size_t pa = encode_utf16((unsigned char)*opsz + wxUnicodePUA, (wxUint16 *)buf);
                        wxASSERT(pa != wxCONV_FAILED);
                        if (buf)
                            buf += pa;
                        opsz++;
                        len += pa;
#else
                        if (buf)
                            *buf++ = (wchar_t)(wxUnicodePUA + (unsigned char)*opsz);
                        opsz++;
                        len++;
#endif
                    }
                }
                else if (m_options & MAP_INVALID_UTF8_TO_OCTAL)
                {
                    while (opsz < psz && (!buf || len < n))
                    {
                        if ( buf && len + 3 < n )
                        {
                            unsigned char on = *opsz;
                            *buf++ = L'\\';
                            *buf++ = (wchar_t)( L'0' + on / 0100 );
                            *buf++ = (wchar_t)( L'0' + (on % 0100) / 010 );
                            *buf++ = (wchar_t)( L'0' + on % 010 );
                        }

                        opsz++;
                        len += 4;
                    }
                }
                else // MAP_INVALID_UTF8_NOT
                {
                    return wxCONV_FAILED;
                }
            }
        }
    }

    if (buf && (len < n))
        *buf = 0;

    return len;
}

static inline bool isoctal(wchar_t wch)
{
    return L'0' <= wch && wch <= L'7';
}

size_t wxMBConvUTF8::WC2MB(char *buf, const wchar_t *psz, size_t n) const
{
    size_t len = 0;

    while (*psz && ((!buf) || (len < n)))
    {
        wxUint32 cc;

#ifdef WC_UTF16
        // cast is ok for WC_UTF16
        size_t pa = decode_utf16((const wxUint16 *)psz, cc);
        psz += (pa == wxCONV_FAILED) ? 1 : pa;
#else
        cc = (*psz++) & 0x7fffffff;
#endif

        if ( (m_options & MAP_INVALID_UTF8_TO_PUA)
                && cc >= wxUnicodePUA && cc < wxUnicodePUAEnd )
        {
            if (buf)
                *buf++ = (char)(cc - wxUnicodePUA);
            len++;
        }
        else if ( (m_options & MAP_INVALID_UTF8_TO_OCTAL)
                    && cc == L'\\' && psz[0] == L'\\' )
        {
            if (buf)
                *buf++ = (char)cc;
            psz++;
            len++;
        }
        else if ( (m_options & MAP_INVALID_UTF8_TO_OCTAL) &&
                    cc == L'\\' &&
                        isoctal(psz[0]) && isoctal(psz[1]) && isoctal(psz[2]) )
        {
            if (buf)
            {
                *buf++ = (char) ((psz[0] - L'0') * 0100 +
                                 (psz[1] - L'0') * 010 +
                                 (psz[2] - L'0'));
            }

            psz += 3;
            len++;
        }
        else
        {
            unsigned cnt;
            for (cnt = 0; cc > utf8_max[cnt]; cnt++)
            {
            }

            if (!cnt)
            {
                // plain ASCII char
                if (buf)
                    *buf++ = (char) cc;
                len++;
            }
            else
            {
                len += cnt + 1;
                if (buf)
                {
                    *buf++ = (char) ((-128 >> cnt) | ((cc >> (cnt * 6)) & (0x3f >> cnt)));
                    while (cnt--)
                        *buf++ = (char) (0x80 | ((cc >> (cnt * 6)) & 0x3f));
                }
            }
        }
    }

    if (buf && (len < n))
        *buf = 0;

    return len;
}

// ============================================================================
// UTF-16
// ============================================================================

#ifdef WORDS_BIGENDIAN
    #define wxMBConvUTF16straight wxMBConvUTF16BE
    #define wxMBConvUTF16swap     wxMBConvUTF16LE
#else
    #define wxMBConvUTF16swap     wxMBConvUTF16BE
    #define wxMBConvUTF16straight wxMBConvUTF16LE
#endif

/* static */
size_t wxMBConvUTF16Base::GetLength(const char *src, size_t srcLen)
{
    if ( srcLen == wxNO_LEN )
    {
        // count the number of bytes in input, including the trailing NULs
        const wxUint16 *inBuff = wx_reinterpret_cast(const wxUint16 *, src);
        for ( srcLen = 1; *inBuff++; srcLen++ )
            ;

        srcLen *= BYTES_PER_CHAR;
    }
    else // we already have the length
    {
        // we can only convert an entire number of UTF-16 characters
        if ( srcLen % BYTES_PER_CHAR )
            return wxCONV_FAILED;
    }

    return srcLen;
}

// case when in-memory representation is UTF-16 too
#ifdef WC_UTF16

// ----------------------------------------------------------------------------
// conversions without endianness change
// ----------------------------------------------------------------------------

size_t
wxMBConvUTF16straight::ToWChar(wchar_t *dst, size_t dstLen,
                               const char *src, size_t srcLen) const
{
    // set up the scene for using memcpy() (which is presumably more efficient
    // than copying the bytes one by one)
    srcLen = GetLength(src, srcLen);
    if ( srcLen == wxNO_LEN )
        return wxCONV_FAILED;

    const size_t inLen = srcLen / BYTES_PER_CHAR;
    if ( dst )
    {
        if ( dstLen < inLen )
            return wxCONV_FAILED;

        memcpy(dst, src, srcLen);
    }

    return inLen;
}

size_t
wxMBConvUTF16straight::FromWChar(char *dst, size_t dstLen,
                                 const wchar_t *src, size_t srcLen) const
{
    if ( srcLen == wxNO_LEN )
        srcLen = wxWcslen(src) + 1;

    srcLen *= BYTES_PER_CHAR;

    if ( dst )
    {
        if ( dstLen < srcLen )
            return wxCONV_FAILED;

        memcpy(dst, src, srcLen);
    }

    return srcLen;
}

// ----------------------------------------------------------------------------
// endian-reversing conversions
// ----------------------------------------------------------------------------

size_t
wxMBConvUTF16swap::ToWChar(wchar_t *dst, size_t dstLen,
                           const char *src, size_t srcLen) const
{
    srcLen = GetLength(src, srcLen);
    if ( srcLen == wxNO_LEN )
        return wxCONV_FAILED;

    srcLen /= BYTES_PER_CHAR;

    if ( dst )
    {
        if ( dstLen < srcLen )
            return wxCONV_FAILED;

        const wxUint16 *inBuff = wx_reinterpret_cast(const wxUint16 *, src);
        for ( size_t n = 0; n < srcLen; n++, inBuff++ )
        {
            *dst++ = wxUINT16_SWAP_ALWAYS(*inBuff);
        }
    }

    return srcLen;
}

size_t
wxMBConvUTF16swap::FromWChar(char *dst, size_t dstLen,
                             const wchar_t *src, size_t srcLen) const
{
    if ( srcLen == wxNO_LEN )
        srcLen = wxWcslen(src) + 1;

    srcLen *= BYTES_PER_CHAR;

    if ( dst )
    {
        if ( dstLen < srcLen )
            return wxCONV_FAILED;

        wxUint16 *outBuff = wx_reinterpret_cast(wxUint16 *, dst);
        for ( size_t n = 0; n < srcLen; n += BYTES_PER_CHAR, src++ )
        {
            *outBuff++ = wxUINT16_SWAP_ALWAYS(*src);
        }
    }

    return srcLen;
}

#else // !WC_UTF16: wchar_t is UTF-32

// ----------------------------------------------------------------------------
// conversions without endianness change
// ----------------------------------------------------------------------------

size_t
wxMBConvUTF16straight::ToWChar(wchar_t *dst, size_t dstLen,
                               const char *src, size_t srcLen) const
{
    srcLen = GetLength(src, srcLen);
    if ( srcLen == wxNO_LEN )
        return wxCONV_FAILED;

    const size_t inLen = srcLen / BYTES_PER_CHAR;
    if ( !dst )
    {
        // optimization: return maximal space which could be needed for this
        // string even if the real size could be smaller if the buffer contains
        // any surrogates
        return inLen;
    }

    size_t outLen = 0;
    const wxUint16 *inBuff = wx_reinterpret_cast(const wxUint16 *, src);
    for ( const wxUint16 * const inEnd = inBuff + inLen; inBuff < inEnd; )
    {
        const wxUint32 ch = wxDecodeSurrogate(&inBuff);
        if ( !inBuff )
            return wxCONV_FAILED;

        if ( ++outLen > dstLen )
            return wxCONV_FAILED;

        *dst++ = ch;
    }


    return outLen;
}

size_t
wxMBConvUTF16straight::FromWChar(char *dst, size_t dstLen,
                                 const wchar_t *src, size_t srcLen) const
{
    if ( srcLen == wxNO_LEN )
        srcLen = wxWcslen(src) + 1;

    size_t outLen = 0;
    wxUint16 *outBuff = wx_reinterpret_cast(wxUint16 *, dst);
    for ( size_t n = 0; n < srcLen; n++ )
    {
        wxUint16 cc[2];
        const size_t numChars = encode_utf16(*src++, cc);
        if ( numChars == wxCONV_FAILED )
            return wxCONV_FAILED;

        outLen += numChars * BYTES_PER_CHAR;
        if ( outBuff )
        {
            if ( outLen > dstLen )
                return wxCONV_FAILED;

            *outBuff++ = cc[0];
            if ( numChars == 2 )
            {
                // second character of a surrogate
                *outBuff++ = cc[1];
            }
        }
    }

    return outLen;
}

// ----------------------------------------------------------------------------
// endian-reversing conversions
// ----------------------------------------------------------------------------

size_t
wxMBConvUTF16swap::ToWChar(wchar_t *dst, size_t dstLen,
                           const char *src, size_t srcLen) const
{
    srcLen = GetLength(src, srcLen);
    if ( srcLen == wxNO_LEN )
        return wxCONV_FAILED;

    const size_t inLen = srcLen / BYTES_PER_CHAR;
    if ( !dst )
    {
        // optimization: return maximal space which could be needed for this
        // string even if the real size could be smaller if the buffer contains
        // any surrogates
        return inLen;
    }

    size_t outLen = 0;
    const wxUint16 *inBuff = wx_reinterpret_cast(const wxUint16 *, src);
    for ( const wxUint16 * const inEnd = inBuff + inLen; inBuff < inEnd; )
    {
        wxUint32 ch;
        wxUint16 tmp[2];

        tmp[0] = wxUINT16_SWAP_ALWAYS(*inBuff);
        inBuff++;
        tmp[1] = wxUINT16_SWAP_ALWAYS(*inBuff);

        const size_t numChars = decode_utf16(tmp, ch);
        if ( numChars == wxCONV_FAILED )
            return wxCONV_FAILED;

        if ( numChars == 2 )
            inBuff++;

        if ( ++outLen > dstLen )
            return wxCONV_FAILED;

        *dst++ = ch;
    }


    return outLen;
}

size_t
wxMBConvUTF16swap::FromWChar(char *dst, size_t dstLen,
                             const wchar_t *src, size_t srcLen) const
{
    if ( srcLen == wxNO_LEN )
        srcLen = wxWcslen(src) + 1;

    size_t outLen = 0;
    wxUint16 *outBuff = wx_reinterpret_cast(wxUint16 *, dst);
    for ( const wchar_t *srcEnd = src + srcLen; src < srcEnd; src++ )
    {
        wxUint16 cc[2];
        const size_t numChars = encode_utf16(*src, cc);
        if ( numChars == wxCONV_FAILED )
            return wxCONV_FAILED;

        outLen += numChars * BYTES_PER_CHAR;
        if ( outBuff )
        {
            if ( outLen > dstLen )
                return wxCONV_FAILED;

            *outBuff++ = wxUINT16_SWAP_ALWAYS(cc[0]);
            if ( numChars == 2 )
            {
                // second character of a surrogate
                *outBuff++ = wxUINT16_SWAP_ALWAYS(cc[1]);
            }
        }
    }

    return outLen;
}

#endif // WC_UTF16/!WC_UTF16


// ============================================================================
// UTF-32
// ============================================================================

#ifdef WORDS_BIGENDIAN
    #define wxMBConvUTF32straight  wxMBConvUTF32BE
    #define wxMBConvUTF32swap      wxMBConvUTF32LE
#else
    #define wxMBConvUTF32swap      wxMBConvUTF32BE
    #define wxMBConvUTF32straight  wxMBConvUTF32LE
#endif


WXDLLIMPEXP_DATA_BASE(wxMBConvUTF32LE) wxConvUTF32LE;
WXDLLIMPEXP_DATA_BASE(wxMBConvUTF32BE) wxConvUTF32BE;

/* static */
size_t wxMBConvUTF32Base::GetLength(const char *src, size_t srcLen)
{
    if ( srcLen == wxNO_LEN )
    {
        // count the number of bytes in input, including the trailing NULs
        const wxUint32 *inBuff = wx_reinterpret_cast(const wxUint32 *, src);
        for ( srcLen = 1; *inBuff++; srcLen++ )
            ;

        srcLen *= BYTES_PER_CHAR;
    }
    else // we already have the length
    {
        // we can only convert an entire number of UTF-32 characters
        if ( srcLen % BYTES_PER_CHAR )
            return wxCONV_FAILED;
    }

    return srcLen;
}

// case when in-memory representation is UTF-16
#ifdef WC_UTF16

// ----------------------------------------------------------------------------
// conversions without endianness change
// ----------------------------------------------------------------------------

size_t
wxMBConvUTF32straight::ToWChar(wchar_t *dst, size_t dstLen,
                               const char *src, size_t srcLen) const
{
    srcLen = GetLength(src, srcLen);
    if ( srcLen == wxNO_LEN )
        return wxCONV_FAILED;

    const wxUint32 *inBuff = wx_reinterpret_cast(const wxUint32 *, src);
    const size_t inLen = srcLen / BYTES_PER_CHAR;
    size_t outLen = 0;
    for ( size_t n = 0; n < inLen; n++ )
    {
        wxUint16 cc[2];
        const size_t numChars = encode_utf16(*inBuff++, cc);
        if ( numChars == wxCONV_FAILED )
            return wxCONV_FAILED;

        outLen += numChars;
        if ( dst )
        {
            if ( outLen > dstLen )
                return wxCONV_FAILED;

            *dst++ = cc[0];
            if ( numChars == 2 )
            {
                // second character of a surrogate
                *dst++ = cc[1];
            }
        }
    }

    return outLen;
}

size_t
wxMBConvUTF32straight::FromWChar(char *dst, size_t dstLen,
                                 const wchar_t *src, size_t srcLen) const
{
    if ( srcLen == wxNO_LEN )
        srcLen = wxWcslen(src) + 1;

    if ( !dst )
    {
        // optimization: return maximal space which could be needed for this
        // string instead of the exact amount which could be less if there are
        // any surrogates in the input
        //
        // we consider that surrogates are rare enough to make it worthwhile to
        // avoid running the loop below at the cost of slightly extra memory
        // consumption
        return srcLen * BYTES_PER_CHAR;
    }

    wxUint32 *outBuff = wx_reinterpret_cast(wxUint32 *, dst);
    size_t outLen = 0;
    for ( const wchar_t * const srcEnd = src + srcLen; src < srcEnd; )
    {
        const wxUint32 ch = wxDecodeSurrogate(&src);
        if ( !src )
            return wxCONV_FAILED;

        outLen += BYTES_PER_CHAR;

        if ( outLen > dstLen )
            return wxCONV_FAILED;

        *outBuff++ = ch;
    }

    return outLen;
}

// ----------------------------------------------------------------------------
// endian-reversing conversions
// ----------------------------------------------------------------------------

size_t
wxMBConvUTF32swap::ToWChar(wchar_t *dst, size_t dstLen,
                           const char *src, size_t srcLen) const
{
    srcLen = GetLength(src, srcLen);
    if ( srcLen == wxNO_LEN )
        return wxCONV_FAILED;

    const wxUint32 *inBuff = wx_reinterpret_cast(const wxUint32 *, src);
    const size_t inLen = srcLen / BYTES_PER_CHAR;
    size_t outLen = 0;
    for ( size_t n = 0; n < inLen; n++, inBuff++ )
    {
        wxUint16 cc[2];
        const size_t numChars = encode_utf16(wxUINT32_SWAP_ALWAYS(*inBuff), cc);
        if ( numChars == wxCONV_FAILED )
            return wxCONV_FAILED;

        outLen += numChars;
        if ( dst )
        {
            if ( outLen > dstLen )
                return wxCONV_FAILED;

            *dst++ = cc[0];
            if ( numChars == 2 )
            {
                // second character of a surrogate
                *dst++ = cc[1];
            }
        }
    }

    return outLen;
}

size_t
wxMBConvUTF32swap::FromWChar(char *dst, size_t dstLen,
                             const wchar_t *src, size_t srcLen) const
{
    if ( srcLen == wxNO_LEN )
        srcLen = wxWcslen(src) + 1;

    if ( !dst )
    {
        // optimization: return maximal space which could be needed for this
        // string instead of the exact amount which could be less if there are
        // any surrogates in the input
        //
        // we consider that surrogates are rare enough to make it worthwhile to
        // avoid running the loop below at the cost of slightly extra memory
        // consumption
        return srcLen*BYTES_PER_CHAR;
    }

    wxUint32 *outBuff = wx_reinterpret_cast(wxUint32 *, dst);
    size_t outLen = 0;
    for ( const wchar_t * const srcEnd = src + srcLen; src < srcEnd; )
    {
        const wxUint32 ch = wxDecodeSurrogate(&src);
        if ( !src )
            return wxCONV_FAILED;

        outLen += BYTES_PER_CHAR;

        if ( outLen > dstLen )
            return wxCONV_FAILED;

        *outBuff++ = wxUINT32_SWAP_ALWAYS(ch);
    }

    return outLen;
}

#else // !WC_UTF16: wchar_t is UTF-32

// ----------------------------------------------------------------------------
// conversions without endianness change
// ----------------------------------------------------------------------------

size_t
wxMBConvUTF32straight::ToWChar(wchar_t *dst, size_t dstLen,
                               const char *src, size_t srcLen) const
{
    // use memcpy() as it should be much faster than hand-written loop
    srcLen = GetLength(src, srcLen);
    if ( srcLen == wxNO_LEN )
        return wxCONV_FAILED;

    const size_t inLen = srcLen/BYTES_PER_CHAR;
    if ( dst )
    {
        if ( dstLen < inLen )
            return wxCONV_FAILED;

        memcpy(dst, src, srcLen);
    }

    return inLen;
}

size_t
wxMBConvUTF32straight::FromWChar(char *dst, size_t dstLen,
                                 const wchar_t *src, size_t srcLen) const
{
    if ( srcLen == wxNO_LEN )
        srcLen = wxWcslen(src) + 1;

    srcLen *= BYTES_PER_CHAR;

    if ( dst )
    {
        if ( dstLen < srcLen )
            return wxCONV_FAILED;

        memcpy(dst, src, srcLen);
    }

    return srcLen;
}

// ----------------------------------------------------------------------------
// endian-reversing conversions
// ----------------------------------------------------------------------------

size_t
wxMBConvUTF32swap::ToWChar(wchar_t *dst, size_t dstLen,
                           const char *src, size_t srcLen) const
{
    srcLen = GetLength(src, srcLen);
    if ( srcLen == wxNO_LEN )
        return wxCONV_FAILED;

    srcLen /= BYTES_PER_CHAR;

    if ( dst )
    {
        if ( dstLen < srcLen )
            return wxCONV_FAILED;

        const wxUint32 *inBuff = wx_reinterpret_cast(const wxUint32 *, src);
        for ( size_t n = 0; n < srcLen; n++, inBuff++ )
        {
            *dst++ = wxUINT32_SWAP_ALWAYS(*inBuff);
        }
    }

    return srcLen;
}

size_t
wxMBConvUTF32swap::FromWChar(char *dst, size_t dstLen,
                             const wchar_t *src, size_t srcLen) const
{
    if ( srcLen == wxNO_LEN )
        srcLen = wxWcslen(src) + 1;

    srcLen *= BYTES_PER_CHAR;

    if ( dst )
    {
        if ( dstLen < srcLen )
            return wxCONV_FAILED;

        wxUint32 *outBuff = wx_reinterpret_cast(wxUint32 *, dst);
        for ( size_t n = 0; n < srcLen; n += BYTES_PER_CHAR, src++ )
        {
            *outBuff++ = wxUINT32_SWAP_ALWAYS(*src);
        }
    }

    return srcLen;
}

#endif // WC_UTF16/!WC_UTF16


// ============================================================================
// The classes doing conversion using the iconv_xxx() functions
// ============================================================================

#ifdef HAVE_ICONV

// VS: glibc 2.1.3 is broken in that iconv() conversion to/from UCS4 fails with
//     E2BIG if output buffer is _exactly_ as big as needed. Such case is
//     (unless there's yet another bug in glibc) the only case when iconv()
//     returns with (size_t)-1 (which means error) and says there are 0 bytes
//     left in the input buffer -- when _real_ error occurs,
//     bytes-left-in-input buffer is non-zero. Hence, this alternative test for
//     iconv() failure.
//     [This bug does not appear in glibc 2.2.]
#if defined(__GLIBC__) && __GLIBC__ == 2 && __GLIBC_MINOR__ <= 1
#define ICONV_FAILED(cres, bufLeft) ((cres == (size_t)-1) && \
                                     (errno != E2BIG || bufLeft != 0))
#else
#define ICONV_FAILED(cres, bufLeft)  (cres == (size_t)-1)
#endif

#define ICONV_CHAR_CAST(x)  ((ICONV_CONST char **)(x))

#define ICONV_T_INVALID ((iconv_t)-1)

#if SIZEOF_WCHAR_T == 4
    #define WC_BSWAP    wxUINT32_SWAP_ALWAYS
    #define WC_ENC      wxFONTENCODING_UTF32
#elif SIZEOF_WCHAR_T == 2
    #define WC_BSWAP    wxUINT16_SWAP_ALWAYS
    #define WC_ENC      wxFONTENCODING_UTF16
#else // sizeof(wchar_t) != 2 nor 4
    // does this ever happen?
    #error "Unknown sizeof(wchar_t): please report this to wx-dev@lists.wxwindows.org"
#endif

// ----------------------------------------------------------------------------
// wxMBConv_iconv: encapsulates an iconv character set
// ----------------------------------------------------------------------------

class wxMBConv_iconv : public wxMBConv
{
public:
    wxMBConv_iconv(const wxChar *name);
    virtual ~wxMBConv_iconv();

    virtual size_t MB2WC(wchar_t *buf, const char *psz, size_t n) const;
    virtual size_t WC2MB(char *buf, const wchar_t *psz, size_t n) const;

    // classify this encoding as explained in wxMBConv::GetMBNulLen() comment
    virtual size_t GetMBNulLen() const;

    virtual wxMBConv *Clone() const
    {
        wxMBConv_iconv *p = new wxMBConv_iconv(m_name);
        p->m_minMBCharWidth = m_minMBCharWidth;
        return p;
    }

    bool IsOk() const
        { return (m2w != ICONV_T_INVALID) && (w2m != ICONV_T_INVALID); }

protected:
    // the iconv handlers used to translate from multibyte
    // to wide char and in the other direction
    iconv_t m2w,
            w2m;

#if wxUSE_THREADS
    // guards access to m2w and w2m objects
    wxMutex m_iconvMutex;
#endif

private:
    // the name (for iconv_open()) of a wide char charset -- if none is
    // available on this machine, it will remain NULL
    static wxString ms_wcCharsetName;

    // true if the wide char encoding we use (i.e. ms_wcCharsetName) has
    // different endian-ness than the native one
    static bool ms_wcNeedsSwap;


    // name of the encoding handled by this conversion
    wxString m_name;

    // cached result of GetMBNulLen(); set to 0 meaning "unknown"
    // initially
    size_t m_minMBCharWidth;
};

// make the constructor available for unit testing
WXDLLIMPEXP_BASE wxMBConv* new_wxMBConv_iconv( const wxChar* name )
{
    wxMBConv_iconv* result = new wxMBConv_iconv( name );
    if ( !result->IsOk() )
    {
        delete result;
        return 0;
    }

    return result;
}

wxString wxMBConv_iconv::ms_wcCharsetName;
bool wxMBConv_iconv::ms_wcNeedsSwap = false;

wxMBConv_iconv::wxMBConv_iconv(const wxChar *name)
              : m_name(name)
{
    m_minMBCharWidth = 0;

    // iconv operates with chars, not wxChars, but luckily it uses only ASCII
    // names for the charsets
    const wxCharBuffer cname(wxString(name).ToAscii());

    // check for charset that represents wchar_t:
    if ( ms_wcCharsetName.empty() )
    {
        wxLogTrace(TRACE_STRCONV, _T("Looking for wide char codeset:"));

#if wxUSE_FONTMAP
        const wxChar **names = wxFontMapperBase::GetAllEncodingNames(WC_ENC);
#else // !wxUSE_FONTMAP
        static const wxChar *names_static[] =
        {
#if SIZEOF_WCHAR_T == 4
            _T("UCS-4"),
#elif SIZEOF_WCHAR_T == 2
            _T("UCS-2"),
#endif
            NULL
        };
        const wxChar **names = names_static;
#endif // wxUSE_FONTMAP/!wxUSE_FONTMAP

        for ( ; *names && ms_wcCharsetName.empty(); ++names )
        {
            const wxString nameCS(*names);

            // first try charset with explicit bytesex info (e.g. "UCS-4LE"):
            wxString nameXE(nameCS);

#ifdef WORDS_BIGENDIAN
                nameXE += _T("BE");
#else // little endian
                nameXE += _T("LE");
#endif

            wxLogTrace(TRACE_STRCONV, _T("  trying charset \"%s\""),
                       nameXE.c_str());

            m2w = iconv_open(nameXE.ToAscii(), cname);
            if ( m2w == ICONV_T_INVALID )
            {
                // try charset w/o bytesex info (e.g. "UCS4")
                wxLogTrace(TRACE_STRCONV, _T("  trying charset \"%s\""),
                           nameCS.c_str());
                m2w = iconv_open(nameCS.ToAscii(), cname);

                // and check for bytesex ourselves:
                if ( m2w != ICONV_T_INVALID )
                {
                    char    buf[2], *bufPtr;
                    wchar_t wbuf[2], *wbufPtr;
                    size_t  insz, outsz;
                    size_t  res;

                    buf[0] = 'A';
                    buf[1] = 0;
                    wbuf[0] = 0;
                    insz = 2;
                    outsz = SIZEOF_WCHAR_T * 2;
                    wbufPtr = wbuf;
                    bufPtr = buf;

                    res = iconv(
                        m2w, ICONV_CHAR_CAST(&bufPtr), &insz,
                        (char**)&wbufPtr, &outsz);

                    if (ICONV_FAILED(res, insz))
                    {
                        wxLogLastError(wxT("iconv"));
                        wxLogError(_("Conversion to charset '%s' doesn't work."),
                                   nameCS.c_str());
                    }
                    else // ok, can convert to this encoding, remember it
                    {
                        ms_wcCharsetName = nameCS;
                        ms_wcNeedsSwap = wbuf[0] != (wchar_t)buf[0];
                    }
                }
            }
            else // use charset not requiring byte swapping
            {
                ms_wcCharsetName = nameXE;
            }
        }

        wxLogTrace(TRACE_STRCONV,
                   wxT("iconv wchar_t charset is \"%s\"%s"),
                   ms_wcCharsetName.empty() ? _T("<none>")
                                            : ms_wcCharsetName.c_str(),
                   ms_wcNeedsSwap ? _T(" (needs swap)")
                                  : _T(""));
    }
    else // we already have ms_wcCharsetName
    {
        m2w = iconv_open(ms_wcCharsetName.ToAscii(), cname);
    }

    if ( ms_wcCharsetName.empty() )
    {
        w2m = ICONV_T_INVALID;
    }
    else
    {
        w2m = iconv_open(cname, ms_wcCharsetName.ToAscii());
        if ( w2m == ICONV_T_INVALID )
        {
            wxLogTrace(TRACE_STRCONV,
                       wxT("\"%s\" -> \"%s\" works but not the converse!?"),
                       ms_wcCharsetName.c_str(), cname.data());
        }
    }
}

wxMBConv_iconv::~wxMBConv_iconv()
{
    if ( m2w != ICONV_T_INVALID )
        iconv_close(m2w);
    if ( w2m != ICONV_T_INVALID )
        iconv_close(w2m);
}

size_t wxMBConv_iconv::MB2WC(wchar_t *buf, const char *psz, size_t n) const
{
    // find the string length: notice that must be done differently for
    // NUL-terminated strings and UTF-16/32 which are terminated with 2/4 NULs
    size_t inbuf;
    const size_t nulLen = GetMBNulLen();
    switch ( nulLen )
    {
        default:
            return wxCONV_FAILED;

        case 1:
            inbuf = strlen(psz); // arguably more optimized than our version
            break;

        case 2:
        case 4:
            // for UTF-16/32 not only we need to have 2/4 consecutive NULs but
            // they also have to start at character boundary and not span two
            // adjacent characters
            const char *p;
            for ( p = psz; NotAllNULs(p, nulLen); p += nulLen )
                ;
            inbuf = p - psz;
            break;
    }

#if wxUSE_THREADS
    // NB: iconv() is MT-safe, but each thread must use its own iconv_t handle.
    //     Unfortunately there are a couple of global wxCSConv objects such as
    //     wxConvLocal that are used all over wx code, so we have to make sure
    //     the handle is used by at most one thread at the time. Otherwise
    //     only a few wx classes would be safe to use from non-main threads
    //     as MB<->WC conversion would fail "randomly".
    wxMutexLocker lock(wxConstCast(this, wxMBConv_iconv)->m_iconvMutex);
#endif // wxUSE_THREADS

    size_t outbuf = n * SIZEOF_WCHAR_T;
    size_t res, cres;
    // VS: Use these instead of psz, buf because iconv() modifies its arguments:
    wchar_t *bufPtr = buf;
    const char *pszPtr = psz;

    if (buf)
    {
        // have destination buffer, convert there
        cres = iconv(m2w,
                     ICONV_CHAR_CAST(&pszPtr), &inbuf,
                     (char**)&bufPtr, &outbuf);
        res = n - (outbuf / SIZEOF_WCHAR_T);

        if (ms_wcNeedsSwap)
        {
            // convert to native endianness
            for ( unsigned i = 0; i < res; i++ )
                buf[n] = WC_BSWAP(buf[i]);
        }

        // NUL-terminate the string if there is any space left
        if (res < n)
            buf[res] = 0;
    }
    else
    {
        // no destination buffer... convert using temp buffer
        // to calculate destination buffer requirement
        wchar_t tbuf[8];
        res = 0;

        do
        {
            bufPtr = tbuf;
            outbuf = 8 * SIZEOF_WCHAR_T;

            cres = iconv(m2w,
                         ICONV_CHAR_CAST(&pszPtr), &inbuf,
                         (char**)&bufPtr, &outbuf );

            res += 8 - (outbuf / SIZEOF_WCHAR_T);
        }
        while ((cres == (size_t)-1) && (errno == E2BIG));
    }

    if (ICONV_FAILED(cres, inbuf))
    {
        //VS: it is ok if iconv fails, hence trace only
        wxLogTrace(TRACE_STRCONV, wxT("iconv failed: %s"), wxSysErrorMsg(wxSysErrorCode()));
        return wxCONV_FAILED;
    }

    return res;
}

size_t wxMBConv_iconv::WC2MB(char *buf, const wchar_t *psz, size_t n) const
{
#if wxUSE_THREADS
    // NB: explained in MB2WC
    wxMutexLocker lock(wxConstCast(this, wxMBConv_iconv)->m_iconvMutex);
#endif

    size_t inlen = wxWcslen(psz);
    size_t inbuf = inlen * SIZEOF_WCHAR_T;
    size_t outbuf = n;
    size_t res, cres;

    wchar_t *tmpbuf = 0;

    if (ms_wcNeedsSwap)
    {
        // need to copy to temp buffer to switch endianness
        // (doing WC_BSWAP twice on the original buffer won't help, as it
        //  could be in read-only memory, or be accessed in some other thread)
        tmpbuf = (wchar_t *)malloc(inbuf + SIZEOF_WCHAR_T);
        for ( size_t i = 0; i < inlen; i++ )
            tmpbuf[n] = WC_BSWAP(psz[i]);

        tmpbuf[inlen] = L'\0';
        psz = tmpbuf;
    }

    if (buf)
    {
        // have destination buffer, convert there
        cres = iconv( w2m, ICONV_CHAR_CAST(&psz), &inbuf, &buf, &outbuf );

        res = n - outbuf;

        // NB: iconv was given only wcslen(psz) characters on input, and so
        //     it couldn't convert the trailing zero. Let's do it ourselves
        //     if there's some room left for it in the output buffer.
        if (res < n)
            buf[0] = 0;
    }
    else
    {
        // no destination buffer: convert using temp buffer
        // to calculate destination buffer requirement
        char tbuf[16];
        res = 0;
        do
        {
            buf = tbuf;
            outbuf = 16;

            cres = iconv( w2m, ICONV_CHAR_CAST(&psz), &inbuf, &buf, &outbuf );

            res += 16 - outbuf;
        }
        while ((cres == (size_t)-1) && (errno == E2BIG));
    }

    if (ms_wcNeedsSwap)
    {
        free(tmpbuf);
    }

    if (ICONV_FAILED(cres, inbuf))
    {
        wxLogTrace(TRACE_STRCONV, wxT("iconv failed: %s"), wxSysErrorMsg(wxSysErrorCode()));
        return wxCONV_FAILED;
    }

    return res;
}

size_t wxMBConv_iconv::GetMBNulLen() const
{
    if ( m_minMBCharWidth == 0 )
    {
        wxMBConv_iconv * const self = wxConstCast(this, wxMBConv_iconv);

#if wxUSE_THREADS
        // NB: explained in MB2WC
        wxMutexLocker lock(self->m_iconvMutex);
#endif

        const wchar_t *wnul = L"";
        char buf[8]; // should be enough for NUL in any encoding
        size_t inLen = sizeof(wchar_t),
               outLen = WXSIZEOF(buf);
        char *inBuff = (char *)wnul;
        char *outBuff = buf;
        if ( iconv(w2m, ICONV_CHAR_CAST(&inBuff), &inLen, &outBuff, &outLen) == (size_t)-1 )
        {
            self->m_minMBCharWidth = (size_t)-1;
        }
        else // ok
        {
            self->m_minMBCharWidth = outBuff - buf;
        }
    }

    return m_minMBCharWidth;
}

#endif // HAVE_ICONV


// ============================================================================
// Win32 conversion classes
// ============================================================================

#ifdef wxHAVE_WIN32_MB2WC

// from utils.cpp
#if wxUSE_FONTMAP
extern WXDLLIMPEXP_BASE long wxCharsetToCodepage(const wxChar *charset);
extern WXDLLIMPEXP_BASE long wxEncodingToCodepage(wxFontEncoding encoding);
#endif

class wxMBConv_win32 : public wxMBConv
{
public:
    wxMBConv_win32()
    {
        m_CodePage = CP_ACP;
        m_minMBCharWidth = 0;
    }

    wxMBConv_win32(const wxMBConv_win32& conv)
        : wxMBConv()
    {
        m_CodePage = conv.m_CodePage;
        m_minMBCharWidth = conv.m_minMBCharWidth;
    }

#if wxUSE_FONTMAP
    wxMBConv_win32(const wxChar* name)
    {
        m_CodePage = wxCharsetToCodepage(name);
        m_minMBCharWidth = 0;
    }

    wxMBConv_win32(wxFontEncoding encoding)
    {
        m_CodePage = wxEncodingToCodepage(encoding);
        m_minMBCharWidth = 0;
    }
#endif // wxUSE_FONTMAP

    virtual size_t MB2WC(wchar_t *buf, const char *psz, size_t n) const
    {
        // note that we have to use MB_ERR_INVALID_CHARS flag as it without it
        // the behaviour is not compatible with the Unix version (using iconv)
        // and break the library itself, e.g. wxTextInputStream::NextChar()
        // wouldn't work if reading an incomplete MB char didn't result in an
        // error
        //
        // Moreover, MB_ERR_INVALID_CHARS is only supported on Win 2K SP4 or
        // Win XP or newer and it is not supported for UTF-[78] so we always
        // use our own conversions in this case. See
        //     http://blogs.msdn.com/michkap/archive/2005/04/19/409566.aspx
        //     http://msdn.microsoft.com/library/en-us/intl/unicode_17si.asp
        if ( m_CodePage == CP_UTF8 )
        {
            return wxConvUTF8.MB2WC(buf, psz, n);
        }

        if ( m_CodePage == CP_UTF7 )
        {
            return wxConvUTF7.MB2WC(buf, psz, n);
        }

        int flags = 0;
        if ( (m_CodePage < 50000 && m_CodePage != CP_SYMBOL) &&
                IsAtLeastWin2kSP4() )
        {
            flags = MB_ERR_INVALID_CHARS;
        }

        const size_t len = ::MultiByteToWideChar
                             (
                                m_CodePage,     // code page
                                flags,          // flags: fall on error
                                psz,            // input string
                                -1,             // its length (NUL-terminated)
                                buf,            // output string
                                buf ? n : 0     // size of output buffer
                             );
        if ( !len )
        {
            // function totally failed
            return wxCONV_FAILED;
        }

        // if we were really converting and didn't use MB_ERR_INVALID_CHARS,
        // check if we succeeded, by doing a double trip:
        if ( !flags && buf )
        {
            const size_t mbLen = strlen(psz);
            wxCharBuffer mbBuf(mbLen);
            if ( ::WideCharToMultiByte
                   (
                      m_CodePage,
                      0,
                      buf,
                      -1,
                      mbBuf.data(),
                      mbLen + 1,        // size in bytes, not length
                      NULL,
                      NULL
                   ) == 0 ||
                  strcmp(mbBuf, psz) != 0 )
            {
                // we didn't obtain the same thing we started from, hence
                // the conversion was lossy and we consider that it failed
                return wxCONV_FAILED;
            }
        }

        // note that it returns count of written chars for buf != NULL and size
        // of the needed buffer for buf == NULL so in either case the length of
        // the string (which never includes the terminating NUL) is one less
        return len - 1;
    }

    virtual size_t WC2MB(char *buf, const wchar_t *pwz, size_t n) const
    {
        /*
            we have a problem here: by default, WideCharToMultiByte() may
            replace characters unrepresentable in the target code page with bad
            quality approximations such as turning "1/2" symbol (U+00BD) into
            "1" for the code pages which don't have it and we, obviously, want
            to avoid this at any price

            the trouble is that this function does it _silently_, i.e. it won't
            even tell us whether it did or not... Win98/2000 and higher provide
            WC_NO_BEST_FIT_CHARS but it doesn't work for the older systems and
            we have to resort to a round trip, i.e. check that converting back
            results in the same string -- this is, of course, expensive but
            otherwise we simply can't be sure to not garble the data.
         */

        // determine if we can rely on WC_NO_BEST_FIT_CHARS: according to MSDN
        // it doesn't work with CJK encodings (which we test for rather roughly
        // here...) nor with UTF-7/8 nor, of course, with Windows versions not
        // supporting it
        BOOL usedDef wxDUMMY_INITIALIZE(false);
        BOOL *pUsedDef;
        int flags;
        if ( CanUseNoBestFit() && m_CodePage < 50000 )
        {
            // it's our lucky day
            flags = WC_NO_BEST_FIT_CHARS;
            pUsedDef = &usedDef;
        }
        else // old system or unsupported encoding
        {
            flags = 0;
            pUsedDef = NULL;
        }

        const size_t len = ::WideCharToMultiByte
                             (
                                m_CodePage,     // code page
                                flags,          // either none or no best fit
                                pwz,            // input string
                                -1,             // it is (wide) NUL-terminated
                                buf,            // output buffer
                                buf ? n : 0,    // and its size
                                NULL,           // default "replacement" char
                                pUsedDef        // [out] was it used?
                             );

        if ( !len )
        {
            // function totally failed
            return wxCONV_FAILED;
        }

        // if we were really converting, check if we succeeded
        if ( buf )
        {
            if ( flags )
            {
                // check if the conversion failed, i.e. if any replacements
                // were done
                if ( usedDef )
                    return wxCONV_FAILED;
            }
            else // we must resort to double tripping...
            {
                wxWCharBuffer wcBuf(n);
                if ( MB2WC(wcBuf.data(), buf, n) == wxCONV_FAILED ||
                        wcscmp(wcBuf, pwz) != 0 )
                {
                    // we didn't obtain the same thing we started from, hence
                    // the conversion was lossy and we consider that it failed
                    return wxCONV_FAILED;
                }
            }
        }

        // see the comment above for the reason of "len - 1"
        return len - 1;
    }

    virtual size_t GetMBNulLen() const
    {
        if ( m_minMBCharWidth == 0 )
        {
            int len = ::WideCharToMultiByte
                        (
                            m_CodePage,     // code page
                            0,              // no flags
                            L"",            // input string
                            1,              // translate just the NUL
                            NULL,           // output buffer
                            0,              // and its size
                            NULL,           // no replacement char
                            NULL            // [out] don't care if it was used
                        );

            wxMBConv_win32 * const self = wxConstCast(this, wxMBConv_win32);
            switch ( len )
            {
                default:
                    wxLogDebug(_T("Unexpected NUL length %d"), len);
                    self->m_minMBCharWidth = (size_t)-1;
                    break;

                case 0:
                    self->m_minMBCharWidth = (size_t)-1;
                    break;

                case 1:
                case 2:
                case 4:
                    self->m_minMBCharWidth = len;
                    break;
            }
        }

        return m_minMBCharWidth;
    }

    virtual wxMBConv *Clone() const { return new wxMBConv_win32(*this); }

    bool IsOk() const { return m_CodePage != -1; }

private:
    static bool CanUseNoBestFit()
    {
        static int s_isWin98Or2k = -1;

        if ( s_isWin98Or2k == -1 )
        {
            int verMaj, verMin;
            switch ( wxGetOsVersion(&verMaj, &verMin) )
            {
                case wxOS_WINDOWS_9X:
                    s_isWin98Or2k = verMaj >= 4 && verMin >= 10;
                    break;

                case wxOS_WINDOWS_NT:
                    s_isWin98Or2k = verMaj >= 5;
                    break;

                default:
                    // unknown: be conservative by default
                    s_isWin98Or2k = 0;
                    break;
            }

            wxASSERT_MSG( s_isWin98Or2k != -1, _T("should be set above") );
        }

        return s_isWin98Or2k == 1;
    }

    static bool IsAtLeastWin2kSP4()
    {
#ifdef __WXWINCE__
        return false;
#else
        static int s_isAtLeastWin2kSP4 = -1;

        if ( s_isAtLeastWin2kSP4 == -1 )
        {
            OSVERSIONINFOEX ver;

            memset(&ver, 0, sizeof(ver));
            ver.dwOSVersionInfoSize = sizeof(ver);
            GetVersionEx((OSVERSIONINFO*)&ver);

            s_isAtLeastWin2kSP4 =
              ((ver.dwMajorVersion > 5) || // Vista+
               (ver.dwMajorVersion == 5 && ver.dwMinorVersion > 0) || // XP/2003
               (ver.dwMajorVersion == 5 && ver.dwMinorVersion == 0 &&
               ver.wServicePackMajor >= 4)) // 2000 SP4+
              ? 1 : 0;
        }

        return s_isAtLeastWin2kSP4 == 1;
#endif
    }


    // the code page we're working with
    long m_CodePage;

    // cached result of GetMBNulLen(), set to 0 initially meaning
    // "unknown"
    size_t m_minMBCharWidth;
};

#endif // wxHAVE_WIN32_MB2WC

// ============================================================================
// Cocoa conversion classes
// ============================================================================

#if defined(__WXCOCOA__)

// RN: There is no UTF-32 support in either Core Foundation or Cocoa.
// Strangely enough, internally Core Foundation uses
// UTF-32 internally quite a bit - its just not public (yet).

#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFStringEncodingExt.h>

CFStringEncoding wxCFStringEncFromFontEnc(wxFontEncoding encoding)
{
    CFStringEncoding enc = kCFStringEncodingInvalidId ;

    switch (encoding)
    {
        case wxFONTENCODING_DEFAULT :
            enc = CFStringGetSystemEncoding();
            break ;

        case wxFONTENCODING_ISO8859_1 :
            enc = kCFStringEncodingISOLatin1 ;
            break ;
        case wxFONTENCODING_ISO8859_2 :
            enc = kCFStringEncodingISOLatin2;
            break ;
        case wxFONTENCODING_ISO8859_3 :
            enc = kCFStringEncodingISOLatin3 ;
            break ;
        case wxFONTENCODING_ISO8859_4 :
            enc = kCFStringEncodingISOLatin4;
            break ;
        case wxFONTENCODING_ISO8859_5 :
            enc = kCFStringEncodingISOLatinCyrillic;
            break ;
        case wxFONTENCODING_ISO8859_6 :
            enc = kCFStringEncodingISOLatinArabic;
            break ;
        case wxFONTENCODING_ISO8859_7 :
            enc = kCFStringEncodingISOLatinGreek;
            break ;
        case wxFONTENCODING_ISO8859_8 :
            enc = kCFStringEncodingISOLatinHebrew;
            break ;
        case wxFONTENCODING_ISO8859_9 :
            enc = kCFStringEncodingISOLatin5;
            break ;
        case wxFONTENCODING_ISO8859_10 :
            enc = kCFStringEncodingISOLatin6;
            break ;
        case wxFONTENCODING_ISO8859_11 :
            enc = kCFStringEncodingISOLatinThai;
            break ;
        case wxFONTENCODING_ISO8859_13 :
            enc = kCFStringEncodingISOLatin7;
            break ;
        case wxFONTENCODING_ISO8859_14 :
            enc = kCFStringEncodingISOLatin8;
            break ;
        case wxFONTENCODING_ISO8859_15 :
            enc = kCFStringEncodingISOLatin9;
            break ;

        case wxFONTENCODING_KOI8 :
            enc = kCFStringEncodingKOI8_R;
            break ;
        case wxFONTENCODING_ALTERNATIVE : // MS-DOS CP866
            enc = kCFStringEncodingDOSRussian;
            break ;

//      case wxFONTENCODING_BULGARIAN :
//          enc = ;
//          break ;

        case wxFONTENCODING_CP437 :
            enc = kCFStringEncodingDOSLatinUS ;
            break ;
        case wxFONTENCODING_CP850 :
            enc = kCFStringEncodingDOSLatin1;
            break ;
        case wxFONTENCODING_CP852 :
            enc = kCFStringEncodingDOSLatin2;
            break ;
        case wxFONTENCODING_CP855 :
            enc = kCFStringEncodingDOSCyrillic;
            break ;
        case wxFONTENCODING_CP866 :
            enc = kCFStringEncodingDOSRussian ;
            break ;
        case wxFONTENCODING_CP874 :
            enc = kCFStringEncodingDOSThai;
            break ;
        case wxFONTENCODING_CP932 :
            enc = kCFStringEncodingDOSJapanese;
            break ;
        case wxFONTENCODING_CP936 :
            enc = kCFStringEncodingDOSChineseSimplif ;
            break ;
        case wxFONTENCODING_CP949 :
            enc = kCFStringEncodingDOSKorean;
            break ;
        case wxFONTENCODING_CP950 :
            enc = kCFStringEncodingDOSChineseTrad;
            break ;
        case wxFONTENCODING_CP1250 :
            enc = kCFStringEncodingWindowsLatin2;
            break ;
        case wxFONTENCODING_CP1251 :
            enc = kCFStringEncodingWindowsCyrillic ;
            break ;
        case wxFONTENCODING_CP1252 :
            enc = kCFStringEncodingWindowsLatin1 ;
            break ;
        case wxFONTENCODING_CP1253 :
            enc = kCFStringEncodingWindowsGreek;
            break ;
        case wxFONTENCODING_CP1254 :
            enc = kCFStringEncodingWindowsLatin5;
            break ;
        case wxFONTENCODING_CP1255 :
            enc = kCFStringEncodingWindowsHebrew ;
            break ;
        case wxFONTENCODING_CP1256 :
            enc = kCFStringEncodingWindowsArabic ;
            break ;
        case wxFONTENCODING_CP1257 :
            enc = kCFStringEncodingWindowsBalticRim;
            break ;
//   This only really encodes to UTF7 (if that) evidently
//        case wxFONTENCODING_UTF7 :
//            enc = kCFStringEncodingNonLossyASCII ;
//            break ;
        case wxFONTENCODING_UTF8 :
            enc = kCFStringEncodingUTF8 ;
            break ;
        case wxFONTENCODING_EUC_JP :
            enc = kCFStringEncodingEUC_JP;
            break ;
        case wxFONTENCODING_UTF16 :
            enc = kCFStringEncodingUnicode ;
            break ;
        case wxFONTENCODING_MACROMAN :
            enc = kCFStringEncodingMacRoman ;
            break ;
        case wxFONTENCODING_MACJAPANESE :
            enc = kCFStringEncodingMacJapanese ;
            break ;
        case wxFONTENCODING_MACCHINESETRAD :
            enc = kCFStringEncodingMacChineseTrad ;
            break ;
        case wxFONTENCODING_MACKOREAN :
            enc = kCFStringEncodingMacKorean ;
            break ;
        case wxFONTENCODING_MACARABIC :
            enc = kCFStringEncodingMacArabic ;
            break ;
        case wxFONTENCODING_MACHEBREW :
            enc = kCFStringEncodingMacHebrew ;
            break ;
        case wxFONTENCODING_MACGREEK :
            enc = kCFStringEncodingMacGreek ;
            break ;
        case wxFONTENCODING_MACCYRILLIC :
            enc = kCFStringEncodingMacCyrillic ;
            break ;
        case wxFONTENCODING_MACDEVANAGARI :
            enc = kCFStringEncodingMacDevanagari ;
            break ;
        case wxFONTENCODING_MACGURMUKHI :
            enc = kCFStringEncodingMacGurmukhi ;
            break ;
        case wxFONTENCODING_MACGUJARATI :
            enc = kCFStringEncodingMacGujarati ;
            break ;
        case wxFONTENCODING_MACORIYA :
            enc = kCFStringEncodingMacOriya ;
            break ;
        case wxFONTENCODING_MACBENGALI :
            enc = kCFStringEncodingMacBengali ;
            break ;
        case wxFONTENCODING_MACTAMIL :
            enc = kCFStringEncodingMacTamil ;
            break ;
        case wxFONTENCODING_MACTELUGU :
            enc = kCFStringEncodingMacTelugu ;
            break ;
        case wxFONTENCODING_MACKANNADA :
            enc = kCFStringEncodingMacKannada ;
            break ;
        case wxFONTENCODING_MACMALAJALAM :
            enc = kCFStringEncodingMacMalayalam ;
            break ;
        case wxFONTENCODING_MACSINHALESE :
            enc = kCFStringEncodingMacSinhalese ;
            break ;
        case wxFONTENCODING_MACBURMESE :
            enc = kCFStringEncodingMacBurmese ;
            break ;
        case wxFONTENCODING_MACKHMER :
            enc = kCFStringEncodingMacKhmer ;
            break ;
        case wxFONTENCODING_MACTHAI :
            enc = kCFStringEncodingMacThai ;
            break ;
        case wxFONTENCODING_MACLAOTIAN :
            enc = kCFStringEncodingMacLaotian ;
            break ;
        case wxFONTENCODING_MACGEORGIAN :
            enc = kCFStringEncodingMacGeorgian ;
            break ;
        case wxFONTENCODING_MACARMENIAN :
            enc = kCFStringEncodingMacArmenian ;
            break ;
        case wxFONTENCODING_MACCHINESESIMP :
            enc = kCFStringEncodingMacChineseSimp ;
            break ;
        case wxFONTENCODING_MACTIBETAN :
            enc = kCFStringEncodingMacTibetan ;
            break ;
        case wxFONTENCODING_MACMONGOLIAN :
            enc = kCFStringEncodingMacMongolian ;
            break ;
        case wxFONTENCODING_MACETHIOPIC :
            enc = kCFStringEncodingMacEthiopic ;
            break ;
        case wxFONTENCODING_MACCENTRALEUR :
            enc = kCFStringEncodingMacCentralEurRoman ;
            break ;
        case wxFONTENCODING_MACVIATNAMESE :
            enc = kCFStringEncodingMacVietnamese ;
            break ;
        case wxFONTENCODING_MACARABICEXT :
            enc = kCFStringEncodingMacExtArabic ;
            break ;
        case wxFONTENCODING_MACSYMBOL :
            enc = kCFStringEncodingMacSymbol ;
            break ;
        case wxFONTENCODING_MACDINGBATS :
            enc = kCFStringEncodingMacDingbats ;
            break ;
        case wxFONTENCODING_MACTURKISH :
            enc = kCFStringEncodingMacTurkish ;
            break ;
        case wxFONTENCODING_MACCROATIAN :
            enc = kCFStringEncodingMacCroatian ;
            break ;
        case wxFONTENCODING_MACICELANDIC :
            enc = kCFStringEncodingMacIcelandic ;
            break ;
        case wxFONTENCODING_MACROMANIAN :
            enc = kCFStringEncodingMacRomanian ;
            break ;
        case wxFONTENCODING_MACCELTIC :
            enc = kCFStringEncodingMacCeltic ;
            break ;
        case wxFONTENCODING_MACGAELIC :
            enc = kCFStringEncodingMacGaelic ;
            break ;
//      case wxFONTENCODING_MACKEYBOARD :
//          enc = kCFStringEncodingMacKeyboardGlyphs ;
//          break ;

        default :
            // because gcc is picky
            break ;
    }

    return enc ;
}

class wxMBConv_cocoa : public wxMBConv
{
public:
    wxMBConv_cocoa()
    {
        Init(CFStringGetSystemEncoding()) ;
    }

    wxMBConv_cocoa(const wxMBConv_cocoa& conv)
    {
        m_encoding = conv.m_encoding;
    }

#if wxUSE_FONTMAP
    wxMBConv_cocoa(const wxChar* name)
    {
        Init( wxCFStringEncFromFontEnc(wxFontMapperBase::Get()->CharsetToEncoding(name, false) ) ) ;
    }
#endif

    wxMBConv_cocoa(wxFontEncoding encoding)
    {
        Init( wxCFStringEncFromFontEnc(encoding) );
    }

    virtual ~wxMBConv_cocoa()
    {
    }

    void Init( CFStringEncoding encoding)
    {
        m_encoding = encoding ;
    }

    size_t MB2WC(wchar_t * szOut, const char * szUnConv, size_t nOutSize) const
    {
        wxASSERT(szUnConv);

        CFStringRef theString = CFStringCreateWithBytes (
                                                NULL, //the allocator
                                                (const UInt8*)szUnConv,
                                                strlen(szUnConv),
                                                m_encoding,
                                                false //no BOM/external representation
                                                );

        wxASSERT(theString);

        size_t nOutLength = CFStringGetLength(theString);

        if (szOut == NULL)
        {
            CFRelease(theString);
            return nOutLength;
        }

        CFRange theRange = { 0, nOutSize };

#if SIZEOF_WCHAR_T == 4
        UniChar* szUniCharBuffer = new UniChar[nOutSize];
#endif

        CFStringGetCharacters(theString, theRange, szUniCharBuffer);

        CFRelease(theString);

        szUniCharBuffer[nOutLength] = '\0';

#if SIZEOF_WCHAR_T == 4
        wxMBConvUTF16 converter;
        converter.MB2WC( szOut, (const char*)szUniCharBuffer, nOutSize );
        delete [] szUniCharBuffer;
#endif

        return nOutLength;
    }

    size_t WC2MB(char *szOut, const wchar_t *szUnConv, size_t nOutSize) const
    {
        wxASSERT(szUnConv);

        size_t nRealOutSize;
        size_t nBufSize = wxWcslen(szUnConv);
        UniChar* szUniBuffer = (UniChar*) szUnConv;

#if SIZEOF_WCHAR_T == 4
        wxMBConvUTF16 converter ;
        nBufSize = converter.WC2MB( NULL, szUnConv, 0 );
        szUniBuffer = new UniChar[ (nBufSize / sizeof(UniChar)) + 1];
        converter.WC2MB( (char*) szUniBuffer, szUnConv, nBufSize + sizeof(UniChar));
        nBufSize /= sizeof(UniChar);
#endif

        CFStringRef theString = CFStringCreateWithCharactersNoCopy(
                                NULL, //allocator
                                szUniBuffer,
                                nBufSize,
                                kCFAllocatorNull //deallocator - we want to deallocate it ourselves
                            );

        wxASSERT(theString);

        //Note that CER puts a BOM when converting to unicode
        //so we  check and use getchars instead in that case
        if (m_encoding == kCFStringEncodingUnicode)
        {
            if (szOut != NULL)
                CFStringGetCharacters(theString, CFRangeMake(0, nOutSize - 1), (UniChar*) szOut);

            nRealOutSize = CFStringGetLength(theString) + 1;
        }
        else
        {
            CFStringGetBytes(
                theString,
                CFRangeMake(0, CFStringGetLength(theString)),
                m_encoding,
                0, //what to put in characters that can't be converted -
                    //0 tells CFString to return NULL if it meets such a character
                false, //not an external representation
                (UInt8*) szOut,
                nOutSize,
                (CFIndex*) &nRealOutSize
                        );
        }

        CFRelease(theString);

#if SIZEOF_WCHAR_T == 4
        delete[] szUniBuffer;
#endif

        return  nRealOutSize - 1;
    }

    virtual wxMBConv *Clone() const { return new wxMBConv_cocoa(*this); }

    bool IsOk() const
    {
        return m_encoding != kCFStringEncodingInvalidId &&
              CFStringIsEncodingAvailable(m_encoding);
    }

private:
    CFStringEncoding m_encoding ;
};

#endif // defined(__WXCOCOA__)

// ============================================================================
// Mac conversion classes
// ============================================================================

#if defined(__WXMAC__) && defined(TARGET_CARBON)

class wxMBConv_mac : public wxMBConv
{
public:
    wxMBConv_mac()
    {
        Init(CFStringGetSystemEncoding()) ;
    }

    wxMBConv_mac(const wxMBConv_mac& conv)
    {
        Init(conv.m_char_encoding);
    }

#if wxUSE_FONTMAP
    wxMBConv_mac(const wxChar* name)
    {
        wxFontEncoding enc = wxFontMapperBase::Get()->CharsetToEncoding(name, false);
        Init( (enc != wxFONTENCODING_SYSTEM) ? wxMacGetSystemEncFromFontEnc( enc ) : kTextEncodingUnknown);
    }
#endif

    wxMBConv_mac(wxFontEncoding encoding)
    {
        Init( wxMacGetSystemEncFromFontEnc(encoding) );
    }

    virtual ~wxMBConv_mac()
    {
        OSStatus status = noErr ;
        if (m_MB2WC_converter)
            status = TECDisposeConverter(m_MB2WC_converter);
        if (m_WC2MB_converter)
            status = TECDisposeConverter(m_WC2MB_converter);
    }

    void Init( TextEncodingBase encoding,TextEncodingVariant encodingVariant = kTextEncodingDefaultVariant ,
            TextEncodingFormat encodingFormat = kTextEncodingDefaultFormat)
    {
        m_MB2WC_converter = NULL ;
        m_WC2MB_converter = NULL ;
        if ( encoding != kTextEncodingUnknown )
        {
            m_char_encoding = CreateTextEncoding(encoding, encodingVariant, encodingFormat) ;
            m_unicode_encoding = CreateTextEncoding(kTextEncodingUnicodeDefault, 0, kUnicode16BitFormat) ;
        }
        else
        {
            m_char_encoding = kTextEncodingUnknown;
            m_unicode_encoding = kTextEncodingUnknown;
        }
    }

    virtual void CreateIfNeeded() const
    {
        if ( m_MB2WC_converter == NULL && m_WC2MB_converter == NULL && 
            m_char_encoding != kTextEncodingUnknown && m_unicode_encoding != kTextEncodingUnknown )
        {
            OSStatus status = noErr ;
            status = TECCreateConverter(&m_MB2WC_converter,
                                    m_char_encoding,
                                    m_unicode_encoding);
            wxASSERT_MSG( status == noErr , _("Unable to create TextEncodingConverter")) ;
            status = TECCreateConverter(&m_WC2MB_converter,
                                    m_unicode_encoding,
                                    m_char_encoding);
            wxASSERT_MSG( status == noErr , _("Unable to create TextEncodingConverter")) ;
        }
    }

    size_t MB2WC(wchar_t *buf, const char *psz, size_t n) const
    {
        CreateIfNeeded() ;
        OSStatus status = noErr ;
        ByteCount byteOutLen ;
        ByteCount byteInLen = strlen(psz) + 1;
        wchar_t *tbuf = NULL ;
        UniChar* ubuf = NULL ;
        size_t res = 0 ;

        if (buf == NULL)
        {
            // Apple specs say at least 32
            n = wxMax( 32, byteInLen ) ;
            tbuf = (wchar_t*) malloc( n * SIZEOF_WCHAR_T ) ;
        }

        ByteCount byteBufferLen = n * sizeof( UniChar ) ;

#if SIZEOF_WCHAR_T == 4
        ubuf = (UniChar*) malloc( byteBufferLen + 2 ) ;
#else
        ubuf = (UniChar*) (buf ? buf : tbuf) ;
#endif
        {
#if wxUSE_THREADS
            wxMutexLocker lock( m_MB2WC_guard );
#endif
            status = TECConvertText(
            m_MB2WC_converter, (ConstTextPtr) psz, byteInLen, &byteInLen,
            (TextPtr) ubuf, byteBufferLen, &byteOutLen);
        }

#if SIZEOF_WCHAR_T == 4
        // we have to terminate here, because n might be larger for the trailing zero, and if UniChar
        // is not properly terminated we get random characters at the end
        ubuf[byteOutLen / sizeof( UniChar ) ] = 0 ;
        wxMBConvUTF16 converter ;
        res = converter.MB2WC( (buf ? buf : tbuf), (const char*)ubuf, n ) ;
        free( ubuf ) ;
#else
        res = byteOutLen / sizeof( UniChar ) ;
#endif

        if ( buf == NULL )
             free(tbuf) ;

        if ( buf  && res < n)
            buf[res] = 0;

        return res ;
    }

    size_t WC2MB(char *buf, const wchar_t *psz, size_t n) const
    {
        CreateIfNeeded() ;
        OSStatus status = noErr ;
        ByteCount byteOutLen ;
        ByteCount byteInLen = wxWcslen(psz) * SIZEOF_WCHAR_T ;

        char *tbuf = NULL ;

        if (buf == NULL)
        {
            // Apple specs say at least 32
            n = wxMax( 32, ((byteInLen / SIZEOF_WCHAR_T) * 8) + SIZEOF_WCHAR_T );
            tbuf = (char*) malloc( n ) ;
        }

        ByteCount byteBufferLen = n ;
        UniChar* ubuf = NULL ;

#if SIZEOF_WCHAR_T == 4
        wxMBConvUTF16 converter ;
        size_t unicharlen = converter.WC2MB( NULL, psz, 0 ) ;
        byteInLen = unicharlen ;
        ubuf = (UniChar*) malloc( byteInLen + 2 ) ;
        converter.WC2MB( (char*) ubuf, psz, unicharlen + 2 ) ;
#else
        ubuf = (UniChar*) psz ;
#endif

        {
#if wxUSE_THREADS
            wxMutexLocker lock( m_WC2MB_guard );
#endif
            status = TECConvertText(
            m_WC2MB_converter, (ConstTextPtr) ubuf, byteInLen, &byteInLen,
            (TextPtr) (buf ? buf : tbuf), byteBufferLen, &byteOutLen);
        }
        
#if SIZEOF_WCHAR_T == 4
        free( ubuf ) ;
#endif

        if ( buf == NULL )
            free(tbuf) ;

        size_t res = byteOutLen ;
        if ( buf  && res < n)
        {
            buf[res] = 0;

            //we need to double-trip to verify it didn't insert any ? in place
            //of bogus characters
            wxWCharBuffer wcBuf(n);
            size_t pszlen = wxWcslen(psz);
            if ( MB2WC(wcBuf.data(), buf, n) == wxCONV_FAILED ||
                        wxWcslen(wcBuf) != pszlen ||
                        memcmp(wcBuf, psz, pszlen * sizeof(wchar_t)) != 0 )
            {
                // we didn't obtain the same thing we started from, hence
                // the conversion was lossy and we consider that it failed
                return wxCONV_FAILED;
            }
        }

        return res ;
    }

    virtual wxMBConv *Clone() const { return new wxMBConv_mac(*this); }

    bool IsOk() const
    {
        CreateIfNeeded() ;
        return m_MB2WC_converter != NULL && m_WC2MB_converter != NULL;
    }

protected :
    mutable TECObjectRef m_MB2WC_converter;
    mutable TECObjectRef m_WC2MB_converter;
#if wxUSE_THREADS
    mutable wxMutex m_MB2WC_guard;
    mutable wxMutex m_WC2MB_guard;
#endif

    TextEncodingBase m_char_encoding;
    TextEncodingBase m_unicode_encoding;
};

// MB is decomposed (D) normalized UTF8

class wxMBConv_macUTF8D : public wxMBConv_mac
{
public :
    wxMBConv_macUTF8D()
    {
        Init( kTextEncodingUnicodeDefault , kUnicodeNoSubset , kUnicodeUTF8Format ) ;
        m_uni = NULL;
        m_uniBack = NULL ;
    }

    virtual ~wxMBConv_macUTF8D()
    {
        if (m_uni!=NULL)
            DisposeUnicodeToTextInfo(&m_uni);
        if (m_uniBack!=NULL)
            DisposeUnicodeToTextInfo(&m_uniBack);
    }

    size_t WC2MB(char *buf, const wchar_t *psz, size_t n) const
    {
        CreateIfNeeded() ;
        OSStatus status = noErr ;
        ByteCount byteOutLen ;
        ByteCount byteInLen = wxWcslen(psz) * SIZEOF_WCHAR_T ;

        char *tbuf = NULL ;

        if (buf == NULL)
        {
            // Apple specs say at least 32
            n = wxMax( 32, ((byteInLen / SIZEOF_WCHAR_T) * 8) + SIZEOF_WCHAR_T );
            tbuf = (char*) malloc( n ) ;
        }

        ByteCount byteBufferLen = n ;
        UniChar* ubuf = NULL ;

#if SIZEOF_WCHAR_T == 4
        wxMBConvUTF16 converter ;
        size_t unicharlen = converter.WC2MB( NULL, psz, 0 ) ;
        byteInLen = unicharlen ;
        ubuf = (UniChar*) malloc( byteInLen + 2 ) ;
        converter.WC2MB( (char*) ubuf, psz, unicharlen + 2 ) ;
#else
        ubuf = (UniChar*) psz ;
#endif

        // ubuf is a non-decomposed UniChar buffer

        ByteCount dcubuflen = byteInLen * 2 + 2 ;
        ByteCount dcubufread , dcubufwritten ;
        UniChar *dcubuf = (UniChar*) malloc( dcubuflen ) ;

        {
#if wxUSE_THREADS
            wxMutexLocker lock( m_WC2MB_guard );
#endif
            ConvertFromUnicodeToText( m_uni , byteInLen , ubuf ,
                kUnicodeDefaultDirectionMask, 0, NULL, NULL, NULL, dcubuflen  , &dcubufread , &dcubufwritten , dcubuf ) ;

            // we now convert that decomposed buffer into UTF8

            status = TECConvertText(
            m_WC2MB_converter, (ConstTextPtr) dcubuf, dcubufwritten, &dcubufread,
            (TextPtr) (buf ? buf : tbuf), byteBufferLen, &byteOutLen);
        }
        
        free( dcubuf );

#if SIZEOF_WCHAR_T == 4
        free( ubuf ) ;
#endif

        if ( buf == NULL )
            free(tbuf) ;

        size_t res = byteOutLen ;
        if ( buf  && res < n)
        {
            buf[res] = 0;
            // don't test for round-trip fidelity yet, we cannot guarantee it yet
        }

        return res ;
    }

    size_t MB2WC(wchar_t *buf, const char *psz, size_t n) const
    {
        CreateIfNeeded() ;
        OSStatus status = noErr ;
        ByteCount byteOutLen ;
        ByteCount byteInLen = strlen(psz) + 1;
        wchar_t *tbuf = NULL ;
        UniChar* ubuf = NULL ;
        size_t res = 0 ;

        if (buf == NULL)
        {
            // Apple specs say at least 32
            n = wxMax( 32, byteInLen ) ;
            tbuf = (wchar_t*) malloc( n * SIZEOF_WCHAR_T ) ;
        }

        ByteCount byteBufferLen = n * sizeof( UniChar ) ;

#if SIZEOF_WCHAR_T == 4
        ubuf = (UniChar*) malloc( byteBufferLen + 2 ) ;
#else
        ubuf = (UniChar*) (buf ? buf : tbuf) ;
#endif

        ByteCount dcubuflen = byteBufferLen * 2 + 2 ;
        ByteCount dcubufread , dcubufwritten ;
        UniChar *dcubuf = (UniChar*) malloc( dcubuflen ) ;

        {
#if wxUSE_THREADS
            wxMutexLocker lock( m_MB2WC_guard );
#endif
            status = TECConvertText(
                                m_MB2WC_converter, (ConstTextPtr) psz, byteInLen, &byteInLen,
                                (TextPtr) dcubuf, dcubuflen, &byteOutLen);
            // we have to terminate here, because n might be larger for the trailing zero, and if UniChar
            // is not properly terminated we get random characters at the end
            dcubuf[byteOutLen / sizeof( UniChar ) ] = 0 ;

            // now from the decomposed UniChar to properly composed uniChar
            ConvertFromUnicodeToText( m_uniBack , byteOutLen , dcubuf ,
                                  kUnicodeDefaultDirectionMask, 0, NULL, NULL, NULL, dcubuflen  , &dcubufread , &dcubufwritten , ubuf ) ;
        }

        free( dcubuf );
        byteOutLen = dcubufwritten ;
        ubuf[byteOutLen / sizeof( UniChar ) ] = 0 ;


#if SIZEOF_WCHAR_T == 4
        wxMBConvUTF16 converter ;
        res = converter.MB2WC( (buf ? buf : tbuf), (const char*)ubuf, n ) ;
        free( ubuf ) ;
#else
        res = byteOutLen / sizeof( UniChar ) ;
#endif

        if ( buf == NULL )
            free(tbuf) ;

        if ( buf  && res < n)
            buf[res] = 0;

        return res ;
    }

    virtual void CreateIfNeeded() const
    {
        wxMBConv_mac::CreateIfNeeded() ;
        if ( m_uni == NULL )
        {
            m_map.unicodeEncoding = CreateTextEncoding(kTextEncodingUnicodeDefault,
                kUnicodeNoSubset, kTextEncodingDefaultFormat);
            m_map.otherEncoding = CreateTextEncoding(kTextEncodingUnicodeDefault,
                kUnicodeCanonicalDecompVariant, kTextEncodingDefaultFormat);
            m_map.mappingVersion = kUnicodeUseLatestMapping;

            OSStatus err = CreateUnicodeToTextInfo(&m_map, &m_uni);
            wxASSERT_MSG( err == noErr , _(" Couldn't create the UnicodeConverter")) ;

            m_map.unicodeEncoding = CreateTextEncoding(kTextEncodingUnicodeDefault,
                                                       kUnicodeNoSubset, kTextEncodingDefaultFormat);
            m_map.otherEncoding = CreateTextEncoding(kTextEncodingUnicodeDefault,
                                                     kUnicodeCanonicalCompVariant, kTextEncodingDefaultFormat);
            m_map.mappingVersion = kUnicodeUseLatestMapping;
            err = CreateUnicodeToTextInfo(&m_map, &m_uniBack);
            wxASSERT_MSG( err == noErr , _(" Couldn't create the UnicodeConverter")) ;
        }
    }
protected :
    mutable UnicodeToTextInfo   m_uni;
    mutable UnicodeToTextInfo   m_uniBack;
    mutable UnicodeMapping      m_map;
};
#endif // defined(__WXMAC__) && defined(TARGET_CARBON)

// ============================================================================
// wxEncodingConverter based conversion classes
// ============================================================================

#if wxUSE_FONTMAP

class wxMBConv_wxwin : public wxMBConv
{
private:
    void Init()
    {
        m_ok = m2w.Init(m_enc, wxFONTENCODING_UNICODE) &&
               w2m.Init(wxFONTENCODING_UNICODE, m_enc);
    }

public:
    // temporarily just use wxEncodingConverter stuff,
    // so that it works while a better implementation is built
    wxMBConv_wxwin(const wxChar* name)
    {
        if (name)
            m_enc = wxFontMapperBase::Get()->CharsetToEncoding(name, false);
        else
            m_enc = wxFONTENCODING_SYSTEM;

        Init();
    }

    wxMBConv_wxwin(wxFontEncoding enc)
    {
        m_enc = enc;

        Init();
    }

    size_t MB2WC(wchar_t *buf, const char *psz, size_t WXUNUSED(n)) const
    {
        size_t inbuf = strlen(psz);
        if (buf)
        {
            if (!m2w.Convert(psz, buf))
                return wxCONV_FAILED;
        }
        return inbuf;
    }

    size_t WC2MB(char *buf, const wchar_t *psz, size_t WXUNUSED(n)) const
    {
        const size_t inbuf = wxWcslen(psz);
        if (buf)
        {
            if (!w2m.Convert(psz, buf))
                return wxCONV_FAILED;
        }

        return inbuf;
    }

    virtual size_t GetMBNulLen() const
    {
        switch ( m_enc )
        {
            case wxFONTENCODING_UTF16BE:
            case wxFONTENCODING_UTF16LE:
                return 2;

            case wxFONTENCODING_UTF32BE:
            case wxFONTENCODING_UTF32LE:
                return 4;

            default:
                return 1;
        }
    }

    virtual wxMBConv *Clone() const { return new wxMBConv_wxwin(m_enc); }

    bool IsOk() const { return m_ok; }

public:
    wxFontEncoding m_enc;
    wxEncodingConverter m2w, w2m;

private:
    // were we initialized successfully?
    bool m_ok;

    DECLARE_NO_COPY_CLASS(wxMBConv_wxwin)
};

// make the constructors available for unit testing
WXDLLIMPEXP_BASE wxMBConv* new_wxMBConv_wxwin( const wxChar* name )
{
    wxMBConv_wxwin* result = new wxMBConv_wxwin( name );
    if ( !result->IsOk() )
    {
        delete result;
        return 0;
    }

    return result;
}

#endif // wxUSE_FONTMAP

// ============================================================================
// wxCSConv implementation
// ============================================================================

void wxCSConv::Init()
{
    m_name = NULL;
    m_convReal =  NULL;
    m_deferred = true;
}

wxCSConv::wxCSConv(const wxChar *charset)
{
    Init();

    if ( charset )
    {
        SetName(charset);
    }

#if wxUSE_FONTMAP
    m_encoding = wxFontMapperBase::GetEncodingFromName(charset);
    if ( m_encoding == wxFONTENCODING_MAX )
    {
        // set to unknown/invalid value
        m_encoding = wxFONTENCODING_SYSTEM;
    }
    else if ( m_encoding == wxFONTENCODING_DEFAULT )
    {
        // wxFONTENCODING_DEFAULT is same as US-ASCII in this context
        m_encoding = wxFONTENCODING_ISO8859_1;
    }
#else
    m_encoding = wxFONTENCODING_SYSTEM;
#endif
}

wxCSConv::wxCSConv(wxFontEncoding encoding)
{
    if ( encoding == wxFONTENCODING_MAX || encoding == wxFONTENCODING_DEFAULT )
    {
        wxFAIL_MSG( _T("invalid encoding value in wxCSConv ctor") );

        encoding = wxFONTENCODING_SYSTEM;
    }

    Init();

    m_encoding = encoding;
}

wxCSConv::~wxCSConv()
{
    Clear();
}

wxCSConv::wxCSConv(const wxCSConv& conv)
        : wxMBConv()
{
    Init();

    SetName(conv.m_name);
    m_encoding = conv.m_encoding;
}

wxCSConv& wxCSConv::operator=(const wxCSConv& conv)
{
    Clear();

    SetName(conv.m_name);
    m_encoding = conv.m_encoding;

    return *this;
}

void wxCSConv::Clear()
{
    free(m_name);
    delete m_convReal;

    m_name = NULL;
    m_convReal = NULL;
}

void wxCSConv::SetName(const wxChar *charset)
{
    if (charset)
    {
        m_name = wxStrdup(charset);
        m_deferred = true;
    }
}

#if wxUSE_FONTMAP

WX_DECLARE_HASH_MAP( wxFontEncoding, wxString, wxIntegerHash, wxIntegerEqual,
                     wxEncodingNameCache );

static wxEncodingNameCache gs_nameCache;
#endif

wxMBConv *wxCSConv::DoCreate() const
{
#if wxUSE_FONTMAP
    wxLogTrace(TRACE_STRCONV,
               wxT("creating conversion for %s"),
               (m_name ? m_name
                       : wxFontMapperBase::GetEncodingName(m_encoding).c_str()));
#endif // wxUSE_FONTMAP

    // check for the special case of ASCII or ISO8859-1 charset: as we have
    // special knowledge of it anyhow, we don't need to create a special
    // conversion object
    if ( m_encoding == wxFONTENCODING_ISO8859_1 ||
            m_encoding == wxFONTENCODING_DEFAULT )
    {
        // don't convert at all
        return NULL;
    }

    // we trust OS to do conversion better than we can so try external
    // conversion methods first
    //
    // the full order is:
    //      1. OS conversion (iconv() under Unix or Win32 API)
    //      2. hard coded conversions for UTF
    //      3. wxEncodingConverter as fall back

    // step (1)
#ifdef HAVE_ICONV
#if !wxUSE_FONTMAP
    if ( m_name )
#endif // !wxUSE_FONTMAP
    {
        wxString name(m_name);
#if wxUSE_FONTMAP
        wxFontEncoding encoding(m_encoding);
#endif

        if ( !name.empty() )
        {
            wxMBConv_iconv *conv = new wxMBConv_iconv(name);
            if ( conv->IsOk() )
                return conv;

            delete conv;

#if wxUSE_FONTMAP
            encoding =
                wxFontMapperBase::Get()->CharsetToEncoding(name, false);
#endif // wxUSE_FONTMAP
        }
#if wxUSE_FONTMAP
        {
            const wxEncodingNameCache::iterator it = gs_nameCache.find(encoding);
            if ( it != gs_nameCache.end() )
            {
                if ( it->second.empty() )
                    return NULL;

                wxMBConv_iconv *conv = new wxMBConv_iconv(it->second);
                if ( conv->IsOk() )
                    return conv;

                delete conv;
            }

            const wxChar** names = wxFontMapperBase::GetAllEncodingNames(encoding);
            // CS : in case this does not return valid names (eg for MacRoman) encoding
            // got a 'failure' entry in the cache all the same, although it just has to
            // be created using a different method, so only store failed iconv creation
            // attempts (or perhaps we shoulnd't do this at all ?)
            if ( names[0] != NULL )
            {
                for ( ; *names; ++names )
                {
                    wxMBConv_iconv *conv = new wxMBConv_iconv(*names);
                    if ( conv->IsOk() )
                    {
                        gs_nameCache[encoding] = *names;
                        return conv;
                    }

                    delete conv;
                }

                gs_nameCache[encoding] = _T(""); // cache the failure
            }
        }
#endif // wxUSE_FONTMAP
    }
#endif // HAVE_ICONV

#ifdef wxHAVE_WIN32_MB2WC
    {
#if wxUSE_FONTMAP
        wxMBConv_win32 *conv = m_name ? new wxMBConv_win32(m_name)
                                      : new wxMBConv_win32(m_encoding);
        if ( conv->IsOk() )
            return conv;

        delete conv;
#else
        return NULL;
#endif
    }
#endif // wxHAVE_WIN32_MB2WC

#if defined(__WXMAC__)
    {
        // leave UTF16 and UTF32 to the built-ins of wx
        if ( m_name || ( m_encoding < wxFONTENCODING_UTF16BE ||
            ( m_encoding >= wxFONTENCODING_MACMIN && m_encoding <= wxFONTENCODING_MACMAX ) ) )
        {
#if wxUSE_FONTMAP
            wxMBConv_mac *conv = m_name ? new wxMBConv_mac(m_name)
                                        : new wxMBConv_mac(m_encoding);
#else
            wxMBConv_mac *conv = new wxMBConv_mac(m_encoding);
#endif
            if ( conv->IsOk() )
                 return conv;

            delete conv;
        }
    }
#endif

#if defined(__WXCOCOA__)
    {
        if ( m_name || ( m_encoding <= wxFONTENCODING_UTF16 ) )
        {
#if wxUSE_FONTMAP
            wxMBConv_cocoa *conv = m_name ? new wxMBConv_cocoa(m_name)
                                          : new wxMBConv_cocoa(m_encoding);
#else
            wxMBConv_cocoa *conv = new wxMBConv_cocoa(m_encoding);
#endif

            if ( conv->IsOk() )
                 return conv;

            delete conv;
        }
    }
#endif
    // step (2)
    wxFontEncoding enc = m_encoding;
#if wxUSE_FONTMAP
    if ( enc == wxFONTENCODING_SYSTEM && m_name )
    {
        // use "false" to suppress interactive dialogs -- we can be called from
        // anywhere and popping up a dialog from here is the last thing we want to
        // do
        enc = wxFontMapperBase::Get()->CharsetToEncoding(m_name, false);
    }
#endif // wxUSE_FONTMAP

    switch ( enc )
    {
        case wxFONTENCODING_UTF7:
             return new wxMBConvUTF7;

        case wxFONTENCODING_UTF8:
             return new wxMBConvUTF8;

        case wxFONTENCODING_UTF16BE:
             return new wxMBConvUTF16BE;

        case wxFONTENCODING_UTF16LE:
             return new wxMBConvUTF16LE;

        case wxFONTENCODING_UTF32BE:
             return new wxMBConvUTF32BE;

        case wxFONTENCODING_UTF32LE:
             return new wxMBConvUTF32LE;

        default:
             // nothing to do but put here to suppress gcc warnings
             break;
    }

    // step (3)
#if wxUSE_FONTMAP
    {
        wxMBConv_wxwin *conv = m_name ? new wxMBConv_wxwin(m_name)
                                      : new wxMBConv_wxwin(m_encoding);
        if ( conv->IsOk() )
            return conv;

        delete conv;
    }
#endif // wxUSE_FONTMAP

    // NB: This is a hack to prevent deadlock. What could otherwise happen
    //     in Unicode build: wxConvLocal creation ends up being here
    //     because of some failure and logs the error. But wxLog will try to
    //     attach a timestamp, for which it will need wxConvLocal (to convert
    //     time to char* and then wchar_t*), but that fails, tries to log the
    //     error, but wxLog has an (already locked) critical section that
    //     guards the static buffer.
    static bool alreadyLoggingError = false;
    if (!alreadyLoggingError)
    {
        alreadyLoggingError = true;
        wxLogError(_("Cannot convert from the charset '%s'!"),
                   m_name ? m_name
                      :
#if wxUSE_FONTMAP
                         wxFontMapperBase::GetEncodingDescription(m_encoding).c_str()
#else // !wxUSE_FONTMAP
                         wxString::Format(_("encoding %i"), m_encoding).c_str()
#endif // wxUSE_FONTMAP/!wxUSE_FONTMAP
              );

        alreadyLoggingError = false;
    }

    return NULL;
}

void wxCSConv::CreateConvIfNeeded() const
{
    if ( m_deferred )
    {
        wxCSConv *self = (wxCSConv *)this; // const_cast

        // if we don't have neither the name nor the encoding, use the default
        // encoding for this system
        if ( !m_name && m_encoding == wxFONTENCODING_SYSTEM )
        {
#if wxUSE_INTL
            self->m_encoding = wxLocale::GetSystemEncoding();
#else
            // fallback to some reasonable default:
            self->m_encoding = wxFONTENCODING_ISO8859_1;
#endif // wxUSE_INTL
        }

        self->m_convReal = DoCreate();
        self->m_deferred = false;
    }
}

bool wxCSConv::IsOk() const
{
    CreateConvIfNeeded();

    // special case: no convReal created for wxFONTENCODING_ISO8859_1
    if ( m_encoding == wxFONTENCODING_ISO8859_1 )
        return true; // always ok as we do it ourselves

    // m_convReal->IsOk() is called at its own creation, so we know it must
    // be ok if m_convReal is non-NULL
    return m_convReal != NULL;
}

size_t wxCSConv::ToWChar(wchar_t *dst, size_t dstLen,
                         const char *src, size_t srcLen) const
{
    CreateConvIfNeeded();

    if (m_convReal)
        return m_convReal->ToWChar(dst, dstLen, src, srcLen);

    // latin-1 (direct)
    if ( srcLen == wxNO_LEN )
        srcLen = strlen(src) + 1; // take trailing NUL too

    if ( dst )
    {
        if ( dstLen < srcLen )
            return wxCONV_FAILED;

        for ( size_t n = 0; n < srcLen; n++ )
            dst[n] = (unsigned char)(src[n]);
    }

    return srcLen;
}

size_t wxCSConv::FromWChar(char *dst, size_t dstLen,
                           const wchar_t *src, size_t srcLen) const
{
    CreateConvIfNeeded();

    if (m_convReal)
        return m_convReal->FromWChar(dst, dstLen, src, srcLen);

    // latin-1 (direct)
    if ( srcLen == wxNO_LEN )
        srcLen = wxWcslen(src) + 1;

    if ( dst )
    {
        if ( dstLen < srcLen )
            return wxCONV_FAILED;

        for ( size_t n = 0; n < srcLen; n++ )
        {
            if ( src[n] > 0xFF )
                return wxCONV_FAILED;

            dst[n] = (char)src[n];
        }

    }
    else // still need to check the input validity
    {
        for ( size_t n = 0; n < srcLen; n++ )
        {
            if ( src[n] > 0xFF )
                return wxCONV_FAILED;
        }
    }

    return srcLen;
}

size_t wxCSConv::MB2WC(wchar_t *buf, const char *psz, size_t n) const
{
    // this function exists only for ABI-compatibility in 2.8 branch
    return wxMBConv::MB2WC(buf, psz, n);
}

size_t wxCSConv::WC2MB(char *buf, const wchar_t *psz, size_t n) const
{
    // this function exists only for ABI-compatibility in 2.8 branch
    return wxMBConv::WC2MB(buf, psz, n);
}

size_t wxCSConv::GetMBNulLen() const
{
    CreateConvIfNeeded();

    if ( m_convReal )
    {
        return m_convReal->GetMBNulLen();
    }

    return 1;
}

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

#ifdef __WINDOWS__
    static wxMBConv_win32 wxConvLibcObj;
#elif defined(__WXMAC__) && !defined(__MACH__)
    static wxMBConv_mac wxConvLibcObj ;
#else
    static wxMBConvLibc wxConvLibcObj;
#endif

static wxCSConv wxConvLocalObj(wxFONTENCODING_SYSTEM);
static wxCSConv wxConvISO8859_1Obj(wxFONTENCODING_ISO8859_1);
static wxMBConvUTF7 wxConvUTF7Obj;
static wxMBConvUTF8 wxConvUTF8Obj;
#if defined(__WXMAC__) && defined(TARGET_CARBON)
static wxMBConv_macUTF8D wxConvMacUTF8DObj;
#endif
WXDLLIMPEXP_DATA_BASE(wxMBConv&) wxConvLibc = wxConvLibcObj;
WXDLLIMPEXP_DATA_BASE(wxCSConv&) wxConvLocal = wxConvLocalObj;
WXDLLIMPEXP_DATA_BASE(wxCSConv&) wxConvISO8859_1 = wxConvISO8859_1Obj;
WXDLLIMPEXP_DATA_BASE(wxMBConvUTF7&) wxConvUTF7 = wxConvUTF7Obj;
WXDLLIMPEXP_DATA_BASE(wxMBConvUTF8&) wxConvUTF8 = wxConvUTF8Obj;
WXDLLIMPEXP_DATA_BASE(wxMBConv *) wxConvCurrent = &wxConvLibcObj;
WXDLLIMPEXP_DATA_BASE(wxMBConv *) wxConvUI = &wxConvLocal;
WXDLLIMPEXP_DATA_BASE(wxMBConv *) wxConvFileName = &
#ifdef __WXOSX__
#if defined(__WXMAC__) && defined(TARGET_CARBON)
                                    wxConvMacUTF8DObj;
#else
                                    wxConvUTF8Obj;
#endif
#else // !__WXOSX__
                                    wxConvLibcObj;
#endif // __WXOSX__/!__WXOSX__

#if wxUSE_UNICODE

wxWCharBuffer wxSafeConvertMB2WX(const char *s)
{
    if ( !s )
        return wxWCharBuffer();

    wxWCharBuffer wbuf(wxConvLibc.cMB2WX(s));
    if ( !wbuf )
        wbuf = wxConvUTF8.cMB2WX(s);
    if ( !wbuf )
        wbuf = wxConvISO8859_1.cMB2WX(s);

    return wbuf;
}

wxCharBuffer wxSafeConvertWX2MB(const wchar_t *ws)
{
    if ( !ws )
        return wxCharBuffer();

    wxCharBuffer buf(wxConvLibc.cWX2MB(ws));
    if ( !buf )
        buf = wxMBConvUTF8(wxMBConvUTF8::MAP_INVALID_UTF8_TO_OCTAL).cWX2MB(ws);

    return buf;
}

#endif // wxUSE_UNICODE

#else // !wxUSE_WCHAR_T

// stand-ins in absence of wchar_t
WXDLLIMPEXP_DATA_BASE(wxMBConv) wxConvLibc,
                                wxConvISO8859_1,
                                wxConvLocal,
                                wxConvUTF8;

WXDLLIMPEXP_DATA_BASE(wxMBConv *) wxConvCurrent = NULL;

#endif // wxUSE_WCHAR_T/!wxUSE_WCHAR_T
