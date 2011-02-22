/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/* This file contains portable string manipulation functions for SDL */

#include "SDL_stdinc.h"


#define SDL_isupperhex(X)   (((X) >= 'A') && ((X) <= 'F'))
#define SDL_islowerhex(X)   (((X) >= 'a') && ((X) <= 'f'))

#define UTF8_IsLeadByte(c) ((c) >= 0xC0 && (c) <= 0xF4)
#define UTF8_IsTrailingByte(c) ((c) >= 0x80 && (c) <= 0xBF)

int UTF8_TrailingBytes(unsigned char c)
{
    if (c >= 0xC0 && c<= 0xDF)
        return 1;
    else if (c >= 0xE0 && c <= 0xEF)
        return 2;
    else if (c >= 0xF0 && c <= 0xF4)
        return 3;
    else
        return 0;
}

#if !defined(HAVE_SSCANF) || !defined(HAVE_STRTOL)
static size_t
SDL_ScanLong(const char *text, int radix, long *valuep)
{
    const char *textstart = text;
    long value = 0;
    SDL_bool negative = SDL_FALSE;

    if (*text == '-') {
        negative = SDL_TRUE;
        ++text;
    }
    if (radix == 16 && SDL_strncmp(text, "0x", 2) == 0) {
        text += 2;
    }
    for (;;) {
        int v;
        if (SDL_isdigit((unsigned char) *text)) {
            v = *text - '0';
        } else if (radix == 16 && SDL_isupperhex(*text)) {
            v = 10 + (*text - 'A');
        } else if (radix == 16 && SDL_islowerhex(*text)) {
            v = 10 + (*text - 'a');
        } else {
            break;
        }
        value *= radix;
        value += v;
        ++text;
    }
    if (valuep) {
        if (negative && value) {
            *valuep = -value;
        } else {
            *valuep = value;
        }
    }
    return (text - textstart);
}
#endif

#if !defined(HAVE_SSCANF) || !defined(HAVE_STRTOUL) || !defined(HAVE_STRTOD)
static size_t
SDL_ScanUnsignedLong(const char *text, int radix, unsigned long *valuep)
{
    const char *textstart = text;
    unsigned long value = 0;

    if (radix == 16 && SDL_strncmp(text, "0x", 2) == 0) {
        text += 2;
    }
    for (;;) {
        int v;
        if (SDL_isdigit((unsigned char) *text)) {
            v = *text - '0';
        } else if (radix == 16 && SDL_isupperhex(*text)) {
            v = 10 + (*text - 'A');
        } else if (radix == 16 && SDL_islowerhex(*text)) {
            v = 10 + (*text - 'a');
        } else {
            break;
        }
        value *= radix;
        value += v;
        ++text;
    }
    if (valuep) {
        *valuep = value;
    }
    return (text - textstart);
}
#endif

#ifndef HAVE_SSCANF
static size_t
SDL_ScanUintPtrT(const char *text, int radix, uintptr_t * valuep)
{
    const char *textstart = text;
    uintptr_t value = 0;

    if (radix == 16 && SDL_strncmp(text, "0x", 2) == 0) {
        text += 2;
    }
    for (;;) {
        int v;
        if (SDL_isdigit((unsigned char) *text)) {
            v = *text - '0';
        } else if (radix == 16 && SDL_isupperhex(*text)) {
            v = 10 + (*text - 'A');
        } else if (radix == 16 && SDL_islowerhex(*text)) {
            v = 10 + (*text - 'a');
        } else {
            break;
        }
        value *= radix;
        value += v;
        ++text;
    }
    if (valuep) {
        *valuep = value;
    }
    return (text - textstart);
}
#endif

#ifdef SDL_HAS_64BIT_TYPE
#if !defined(HAVE_SSCANF) || !defined(HAVE_STRTOLL)
static size_t
SDL_ScanLongLong(const char *text, int radix, Sint64 * valuep)
{
    const char *textstart = text;
    Sint64 value = 0;
    SDL_bool negative = SDL_FALSE;

    if (*text == '-') {
        negative = SDL_TRUE;
        ++text;
    }
    if (radix == 16 && SDL_strncmp(text, "0x", 2) == 0) {
        text += 2;
    }
    for (;;) {
        int v;
        if (SDL_isdigit((unsigned char) *text)) {
            v = *text - '0';
        } else if (radix == 16 && SDL_isupperhex(*text)) {
            v = 10 + (*text - 'A');
        } else if (radix == 16 && SDL_islowerhex(*text)) {
            v = 10 + (*text - 'a');
        } else {
            break;
        }
        value *= radix;
        value += v;
        ++text;
    }
    if (valuep) {
        if (negative && value) {
            *valuep = -value;
        } else {
            *valuep = value;
        }
    }
    return (text - textstart);
}
#endif

#if !defined(HAVE_SSCANF) || !defined(HAVE_STRTOULL)
static size_t
SDL_ScanUnsignedLongLong(const char *text, int radix, Uint64 * valuep)
{
    const char *textstart = text;
    Uint64 value = 0;

    if (radix == 16 && SDL_strncmp(text, "0x", 2) == 0) {
        text += 2;
    }
    for (;;) {
        int v;
        if (SDL_isdigit((unsigned char) *text)) {
            v = *text - '0';
        } else if (radix == 16 && SDL_isupperhex(*text)) {
            v = 10 + (*text - 'A');
        } else if (radix == 16 && SDL_islowerhex(*text)) {
            v = 10 + (*text - 'a');
        } else {
            break;
        }
        value *= radix;
        value += v;
        ++text;
    }
    if (valuep) {
        *valuep = value;
    }
    return (text - textstart);
}
#endif
#endif /* SDL_HAS_64BIT_TYPE */

#if !defined(HAVE_SSCANF) || !defined(HAVE_STRTOD)
static size_t
SDL_ScanFloat(const char *text, double *valuep)
{
    const char *textstart = text;
    unsigned long lvalue = 0;
    double value = 0.0;
    SDL_bool negative = SDL_FALSE;

    if (*text == '-') {
        negative = SDL_TRUE;
        ++text;
    }
    text += SDL_ScanUnsignedLong(text, 10, &lvalue);
    value += lvalue;
    if (*text == '.') {
        int mult = 10;
        ++text;
        while (SDL_isdigit((unsigned char) *text)) {
            lvalue = *text - '0';
            value += (double) lvalue / mult;
            mult *= 10;
            ++text;
        }
    }
    if (valuep) {
        if (negative && value) {
            *valuep = -value;
        } else {
            *valuep = value;
        }
    }
    return (text - textstart);
}
#endif

#ifndef SDL_memset
void *
SDL_memset(void *dst, int c, size_t len)
{
    size_t left = (len % 4);
    Uint32 *dstp4;
    Uint8 *dstp1;
    Uint32 value4 = (c | (c << 8) | (c << 16) | (c << 24));
    Uint8 value1 = (Uint8) c;

    dstp4 = (Uint32 *) dst;
    len /= 4;
    while (len--) {
        *dstp4++ = value4;
    }

    dstp1 = (Uint8 *) dstp4;
    switch (left) {
    case 3:
        *dstp1++ = value1;
    case 2:
        *dstp1++ = value1;
    case 1:
        *dstp1++ = value1;
    }

    return dst;
}
#endif

#ifndef SDL_memcpy
void *
SDL_memcpy(void *dst, const void *src, size_t len)
{
    size_t left = (len % 4);
    Uint32 *srcp4, *dstp4;
    Uint8 *srcp1, *dstp1;

    srcp4 = (Uint32 *) src;
    dstp4 = (Uint32 *) dst;
    len /= 4;
    while (len--) {
        *dstp4++ = *srcp4++;
    }

    srcp1 = (Uint8 *) srcp4;
    dstp1 = (Uint8 *) dstp4;
    switch (left) {
    case 3:
        *dstp1++ = *srcp1++;
    case 2:
        *dstp1++ = *srcp1++;
    case 1:
        *dstp1++ = *srcp1++;
    }

    return dst;
}
#endif

#ifndef SDL_memmove
void *
SDL_memmove(void *dst, const void *src, size_t len)
{
    char *srcp = (char *) src;
    char *dstp = (char *) dst;

    if (src < dst) {
        srcp += len - 1;
        dstp += len - 1;
        while (len--) {
            *dstp-- = *srcp--;
        }
    } else {
        while (len--) {
            *dstp++ = *srcp++;
        }
    }
    return dst;
}
#endif

#ifndef SDL_memcmp
int
SDL_memcmp(const void *s1, const void *s2, size_t len)
{
    char *s1p = (char *) s1;
    char *s2p = (char *) s2;
    while (len--) {
        if (*s1p != *s2p) {
            return (*s1p - *s2p);
        }
        ++s1p;
        ++s2p;
    }
    return 0;
}
#endif

#ifndef HAVE_STRLEN
size_t
SDL_strlen(const char *string)
{
    size_t len = 0;
    while (*string++) {
        ++len;
    }
    return len;
}
#endif

#ifndef HAVE_WCSLEN
size_t
SDL_wcslen(const wchar_t * string)
{
    size_t len = 0;
    while (*string++) {
        ++len;
    }
    return len;
}
#endif

#ifndef HAVE_WCSLCPY
size_t
SDL_wcslcpy(wchar_t *dst, const wchar_t *src, size_t maxlen)
{
    size_t srclen = SDL_wcslen(src);
    if (maxlen > 0) {
        size_t len = SDL_min(srclen, maxlen - 1);
        SDL_memcpy(dst, src, len * sizeof(wchar_t));
        dst[len] = '\0';
    }
    return srclen;
}
#endif

#ifndef HAVE_WCSLCAT
size_t
SDL_wcslcat(wchar_t *dst, const wchar_t *src, size_t maxlen)
{
    size_t dstlen = SDL_wcslen(dst);
    size_t srclen = SDL_wcslen(src);
    if (dstlen < maxlen) {
        SDL_wcslcpy(dst + dstlen, src, maxlen - dstlen);
    }
    return dstlen + srclen;
}
#endif

#ifndef HAVE_STRLCPY
size_t
SDL_strlcpy(char *dst, const char *src, size_t maxlen)
{
    size_t srclen = SDL_strlen(src);
    if (maxlen > 0) {
        size_t len = SDL_min(srclen, maxlen - 1);
        SDL_memcpy(dst, src, len);
        dst[len] = '\0';
    }
    return srclen;
}
#endif

size_t SDL_utf8strlcpy(char *dst, const char *src, size_t dst_bytes)
{
    size_t src_bytes = SDL_strlen(src);
    size_t bytes = SDL_min(src_bytes, dst_bytes - 1);
    size_t i = 0;
    char trailing_bytes = 0;
    if (bytes)
    {
        unsigned char c = (unsigned char)src[bytes - 1];
        if (UTF8_IsLeadByte(c))
            --bytes;
        else if (UTF8_IsTrailingByte(c))
        {
            for (i = bytes - 1; i != 0; --i)
            {
                c = (unsigned char)src[i];
                trailing_bytes = UTF8_TrailingBytes(c);
                if (trailing_bytes)
                {
                    if (bytes - i != trailing_bytes + 1)
                        bytes = i;

                    break;
                }
            }
        }
        SDL_memcpy(dst, src, bytes);
    }
    dst[bytes] = '\0';
    return bytes;
}

#ifndef HAVE_STRLCAT
size_t
SDL_strlcat(char *dst, const char *src, size_t maxlen)
{
    size_t dstlen = SDL_strlen(dst);
    size_t srclen = SDL_strlen(src);
    if (dstlen < maxlen) {
        SDL_strlcpy(dst + dstlen, src, maxlen - dstlen);
    }
    return dstlen + srclen;
}
#endif

#ifndef HAVE_STRDUP
char *
SDL_strdup(const char *string)
{
    size_t len = SDL_strlen(string) + 1;
    char *newstr = SDL_malloc(len);
    if (newstr) {
        SDL_strlcpy(newstr, string, len);
    }
    return newstr;
}
#endif

#ifndef HAVE__STRREV
char *
SDL_strrev(char *string)
{
    size_t len = SDL_strlen(string);
    char *a = &string[0];
    char *b = &string[len - 1];
    len /= 2;
    while (len--) {
        char c = *a;
        *a++ = *b;
        *b-- = c;
    }
    return string;
}
#endif

#ifndef HAVE__STRUPR
char *
SDL_strupr(char *string)
{
    char *bufp = string;
    while (*bufp) {
        *bufp = SDL_toupper((unsigned char) *bufp);
        ++bufp;
    }
    return string;
}
#endif

#ifndef HAVE__STRLWR
char *
SDL_strlwr(char *string)
{
    char *bufp = string;
    while (*bufp) {
        *bufp = SDL_tolower((unsigned char) *bufp);
        ++bufp;
    }
    return string;
}
#endif

#ifndef HAVE_STRCHR
char *
SDL_strchr(const char *string, int c)
{
    while (*string) {
        if (*string == c) {
            return (char *) string;
        }
        ++string;
    }
    return NULL;
}
#endif

#ifndef HAVE_STRRCHR
char *
SDL_strrchr(const char *string, int c)
{
    const char *bufp = string + SDL_strlen(string) - 1;
    while (bufp >= string) {
        if (*bufp == c) {
            return (char *) bufp;
        }
        --bufp;
    }
    return NULL;
}
#endif

#ifndef HAVE_STRSTR
char *
SDL_strstr(const char *haystack, const char *needle)
{
    size_t length = SDL_strlen(needle);
    while (*haystack) {
        if (SDL_strncmp(haystack, needle, length) == 0) {
            return (char *) haystack;
        }
        ++haystack;
    }
    return NULL;
}
#endif

#if !defined(HAVE__LTOA)  || !defined(HAVE__I64TOA) || \
    !defined(HAVE__ULTOA) || !defined(HAVE__UI64TOA)
static const char ntoa_table[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z'
};
#endif /* ntoa() conversion table */

#ifndef HAVE__LTOA
char *
SDL_ltoa(long value, char *string, int radix)
{
    char *bufp = string;

    if (value < 0) {
        *bufp++ = '-';
        value = -value;
    }
    if (value) {
        while (value > 0) {
            *bufp++ = ntoa_table[value % radix];
            value /= radix;
        }
    } else {
        *bufp++ = '0';
    }
    *bufp = '\0';

    /* The numbers went into the string backwards. :) */
    if (*string == '-') {
        SDL_strrev(string + 1);
    } else {
        SDL_strrev(string);
    }

    return string;
}
#endif

#ifndef HAVE__ULTOA
char *
SDL_ultoa(unsigned long value, char *string, int radix)
{
    char *bufp = string;

    if (value) {
        while (value > 0) {
            *bufp++ = ntoa_table[value % radix];
            value /= radix;
        }
    } else {
        *bufp++ = '0';
    }
    *bufp = '\0';

    /* The numbers went into the string backwards. :) */
    SDL_strrev(string);

    return string;
}
#endif

#ifndef HAVE_STRTOL
long
SDL_strtol(const char *string, char **endp, int base)
{
    size_t len;
    long value;

    if (!base) {
        if ((SDL_strlen(string) > 2) && (SDL_strncmp(string, "0x", 2) == 0)) {
            base = 16;
        } else {
            base = 10;
        }
    }

    len = SDL_ScanLong(string, base, &value);
    if (endp) {
        *endp = (char *) string + len;
    }
    return value;
}
#endif

#ifndef HAVE_STRTOUL
unsigned long
SDL_strtoul(const char *string, char **endp, int base)
{
    size_t len;
    unsigned long value;

    if (!base) {
        if ((SDL_strlen(string) > 2) && (SDL_strncmp(string, "0x", 2) == 0)) {
            base = 16;
        } else {
            base = 10;
        }
    }

    len = SDL_ScanUnsignedLong(string, base, &value);
    if (endp) {
        *endp = (char *) string + len;
    }
    return value;
}
#endif

#ifdef SDL_HAS_64BIT_TYPE

#ifndef HAVE__I64TOA
char *
SDL_lltoa(Sint64 value, char *string, int radix)
{
    char *bufp = string;

    if (value < 0) {
        *bufp++ = '-';
        value = -value;
    }
    if (value) {
        while (value > 0) {
            *bufp++ = ntoa_table[value % radix];
            value /= radix;
        }
    } else {
        *bufp++ = '0';
    }
    *bufp = '\0';

    /* The numbers went into the string backwards. :) */
    if (*string == '-') {
        SDL_strrev(string + 1);
    } else {
        SDL_strrev(string);
    }

    return string;
}
#endif

#ifndef HAVE__UI64TOA
char *
SDL_ulltoa(Uint64 value, char *string, int radix)
{
    char *bufp = string;

    if (value) {
        while (value > 0) {
            *bufp++ = ntoa_table[value % radix];
            value /= radix;
        }
    } else {
        *bufp++ = '0';
    }
    *bufp = '\0';

    /* The numbers went into the string backwards. :) */
    SDL_strrev(string);

    return string;
}
#endif

#ifndef HAVE_STRTOLL
Sint64
SDL_strtoll(const char *string, char **endp, int base)
{
    size_t len;
    Sint64 value;

    if (!base) {
        if ((SDL_strlen(string) > 2) && (SDL_strncmp(string, "0x", 2) == 0)) {
            base = 16;
        } else {
            base = 10;
        }
    }

    len = SDL_ScanLongLong(string, base, &value);
    if (endp) {
        *endp = (char *) string + len;
    }
    return value;
}
#endif

#ifndef HAVE_STRTOULL
Uint64
SDL_strtoull(const char *string, char **endp, int base)
{
    size_t len;
    Uint64 value;

    if (!base) {
        if ((SDL_strlen(string) > 2) && (SDL_strncmp(string, "0x", 2) == 0)) {
            base = 16;
        } else {
            base = 10;
        }
    }

    len = SDL_ScanUnsignedLongLong(string, base, &value);
    if (endp) {
        *endp = (char *) string + len;
    }
    return value;
}
#endif

#endif /* SDL_HAS_64BIT_TYPE */

#ifndef HAVE_STRTOD
double
SDL_strtod(const char *string, char **endp)
{
    size_t len;
    double value;

    len = SDL_ScanFloat(string, &value);
    if (endp) {
        *endp = (char *) string + len;
    }
    return value;
}
#endif

#ifndef HAVE_STRCMP
int
SDL_strcmp(const char *str1, const char *str2)
{
    while (*str1 && *str2) {
        if (*str1 != *str2)
            break;
        ++str1;
        ++str2;
    }
    return (int) ((unsigned char) *str1 - (unsigned char) *str2);
}
#endif

#ifndef HAVE_STRNCMP
int
SDL_strncmp(const char *str1, const char *str2, size_t maxlen)
{
    while (*str1 && *str2 && maxlen) {
        if (*str1 != *str2)
            break;
        ++str1;
        ++str2;
        --maxlen;
    }
    if (!maxlen) {
        return 0;
    }
    return (int) ((unsigned char) *str1 - (unsigned char) *str2);
}
#endif

#if !defined(HAVE_STRCASECMP) && !defined(HAVE__STRICMP)
int
SDL_strcasecmp(const char *str1, const char *str2)
{
    char a = 0;
    char b = 0;
    while (*str1 && *str2) {
        a = SDL_tolower((unsigned char) *str1);
        b = SDL_tolower((unsigned char) *str2);
        if (a != b)
            break;
        ++str1;
        ++str2;
    }
    a = SDL_tolower(*str1);
    b = SDL_tolower(*str2);
    return (int) ((unsigned char) a - (unsigned char) b);
}
#endif

#if !defined(HAVE_STRNCASECMP) && !defined(HAVE__STRNICMP)
int
SDL_strncasecmp(const char *str1, const char *str2, size_t maxlen)
{
    char a = 0;
    char b = 0;
    while (*str1 && *str2 && maxlen) {
        a = SDL_tolower((unsigned char) *str1);
        b = SDL_tolower((unsigned char) *str2);
        if (a != b)
            break;
        ++str1;
        ++str2;
        --maxlen;
    }
    a = SDL_tolower(*str1);
    b = SDL_tolower(*str2);
    return (int) ((unsigned char) a - (unsigned char) b);
}
#endif

#ifndef HAVE_SSCANF
int
SDL_sscanf(const char *text, const char *fmt, ...)
{
    va_list ap;
    int retval = 0;

    va_start(ap, fmt);
    while (*fmt) {
        if (*fmt == ' ') {
            while (SDL_isspace((unsigned char) *text)) {
                ++text;
            }
            ++fmt;
            continue;
        }
        if (*fmt == '%') {
            SDL_bool done = SDL_FALSE;
            long count = 0;
            int radix = 10;
            enum
            {
                DO_SHORT,
                DO_INT,
                DO_LONG,
                DO_LONGLONG
            } inttype = DO_INT;
            SDL_bool suppress = SDL_FALSE;

            ++fmt;
            if (*fmt == '%') {
                if (*text == '%') {
                    ++text;
                    ++fmt;
                    continue;
                }
                break;
            }
            if (*fmt == '*') {
                suppress = SDL_TRUE;
                ++fmt;
            }
            fmt += SDL_ScanLong(fmt, 10, &count);

            if (*fmt == 'c') {
                if (!count) {
                    count = 1;
                }
                if (suppress) {
                    while (count--) {
                        ++text;
                    }
                } else {
                    char *valuep = va_arg(ap, char *);
                    while (count--) {
                        *valuep++ = *text++;
                    }
                    ++retval;
                }
                continue;
            }

            while (SDL_isspace((unsigned char) *text)) {
                ++text;
            }

            /* FIXME: implement more of the format specifiers */
            while (!done) {
                switch (*fmt) {
                case '*':
                    suppress = SDL_TRUE;
                    break;
                case 'h':
                    if (inttype > DO_SHORT) {
                        ++inttype;
                    }
                    break;
                case 'l':
                    if (inttype < DO_LONGLONG) {
                        ++inttype;
                    }
                    break;
                case 'I':
                    if (SDL_strncmp(fmt, "I64", 3) == 0) {
                        fmt += 2;
                        inttype = DO_LONGLONG;
                    }
                    break;
                case 'i':
                    {
                        int index = 0;
                        if (text[index] == '-') {
                            ++index;
                        }
                        if (text[index] == '0') {
                            if (SDL_tolower((unsigned char) text[index + 1])
                                == 'x') {
                                radix = 16;
                            } else {
                                radix = 8;
                            }
                        }
                    }
                    /* Fall through to %d handling */
                case 'd':
#ifdef SDL_HAS_64BIT_TYPE
                    if (inttype == DO_LONGLONG) {
                        Sint64 value;
                        text += SDL_ScanLongLong(text, radix, &value);
                        if (!suppress) {
                            Sint64 *valuep = va_arg(ap, Sint64 *);
                            *valuep = value;
                            ++retval;
                        }
                    } else
#endif /* SDL_HAS_64BIT_TYPE */
                    {
                        long value;
                        text += SDL_ScanLong(text, radix, &value);
                        if (!suppress) {
                            switch (inttype) {
                            case DO_SHORT:
                                {
                                    short *valuep = va_arg(ap, short *);
                                    *valuep = (short) value;
                                }
                                break;
                            case DO_INT:
                                {
                                    int *valuep = va_arg(ap, int *);
                                    *valuep = (int) value;
                                }
                                break;
                            case DO_LONG:
                                {
                                    long *valuep = va_arg(ap, long *);
                                    *valuep = value;
                                }
                                break;
                            case DO_LONGLONG:
                                /* Handled above */
                                break;
                            }
                            ++retval;
                        }
                    }
                    done = SDL_TRUE;
                    break;
                case 'o':
                    if (radix == 10) {
                        radix = 8;
                    }
                    /* Fall through to unsigned handling */
                case 'x':
                case 'X':
                    if (radix == 10) {
                        radix = 16;
                    }
                    /* Fall through to unsigned handling */
                case 'u':
#ifdef SDL_HAS_64BIT_TYPE
                    if (inttype == DO_LONGLONG) {
                        Uint64 value;
                        text += SDL_ScanUnsignedLongLong(text, radix, &value);
                        if (!suppress) {
                            Uint64 *valuep = va_arg(ap, Uint64 *);
                            *valuep = value;
                            ++retval;
                        }
                    } else
#endif /* SDL_HAS_64BIT_TYPE */
                    {
                        unsigned long value;
                        text += SDL_ScanUnsignedLong(text, radix, &value);
                        if (!suppress) {
                            switch (inttype) {
                            case DO_SHORT:
                                {
                                    short *valuep = va_arg(ap, short *);
                                    *valuep = (short) value;
                                }
                                break;
                            case DO_INT:
                                {
                                    int *valuep = va_arg(ap, int *);
                                    *valuep = (int) value;
                                }
                                break;
                            case DO_LONG:
                                {
                                    long *valuep = va_arg(ap, long *);
                                    *valuep = value;
                                }
                                break;
                            case DO_LONGLONG:
                                /* Handled above */
                                break;
                            }
                            ++retval;
                        }
                    }
                    done = SDL_TRUE;
                    break;
                case 'p':
                    {
                        uintptr_t value;
                        text += SDL_ScanUintPtrT(text, 16, &value);
                        if (!suppress) {
                            void **valuep = va_arg(ap, void **);
                            *valuep = (void *) value;
                            ++retval;
                        }
                    }
                    done = SDL_TRUE;
                    break;
                case 'f':
                    {
                        double value;
                        text += SDL_ScanFloat(text, &value);
                        if (!suppress) {
                            float *valuep = va_arg(ap, float *);
                            *valuep = (float) value;
                            ++retval;
                        }
                    }
                    done = SDL_TRUE;
                    break;
                case 's':
                    if (suppress) {
                        while (!SDL_isspace((unsigned char) *text)) {
                            ++text;
                            if (count) {
                                if (--count == 0) {
                                    break;
                                }
                            }
                        }
                    } else {
                        char *valuep = va_arg(ap, char *);
                        while (!SDL_isspace((unsigned char) *text)) {
                            *valuep++ = *text++;
                            if (count) {
                                if (--count == 0) {
                                    break;
                                }
                            }
                        }
                        *valuep = '\0';
                        ++retval;
                    }
                    done = SDL_TRUE;
                    break;
                default:
                    done = SDL_TRUE;
                    break;
                }
                ++fmt;
            }
            continue;
        }
        if (*text == *fmt) {
            ++text;
            ++fmt;
            continue;
        }
        /* Text didn't match format specifier */
        break;
    }
    va_end(ap);

    return retval;
}
#endif

#ifndef HAVE_SNPRINTF
int
SDL_snprintf(char *text, size_t maxlen, const char *fmt, ...)
{
    va_list ap;
    int retval;

    va_start(ap, fmt);
    retval = SDL_vsnprintf(text, maxlen, fmt, ap);
    va_end(ap);

    return retval;
}
#endif

#ifndef HAVE_VSNPRINTF
static size_t
SDL_PrintLong(char *text, long value, int radix, size_t maxlen)
{
    char num[130];
    size_t size;

    SDL_ltoa(value, num, radix);
    size = SDL_strlen(num);
    if (size >= maxlen) {
        size = maxlen - 1;
    }
    SDL_strlcpy(text, num, size + 1);

    return size;
}

static size_t
SDL_PrintUnsignedLong(char *text, unsigned long value, int radix,
                      size_t maxlen)
{
    char num[130];
    size_t size;

    SDL_ultoa(value, num, radix);
    size = SDL_strlen(num);
    if (size >= maxlen) {
        size = maxlen - 1;
    }
    SDL_strlcpy(text, num, size + 1);

    return size;
}

#ifdef SDL_HAS_64BIT_TYPE
static size_t
SDL_PrintLongLong(char *text, Sint64 value, int radix, size_t maxlen)
{
    char num[130];
    size_t size;

    SDL_lltoa(value, num, radix);
    size = SDL_strlen(num);
    if (size >= maxlen) {
        size = maxlen - 1;
    }
    SDL_strlcpy(text, num, size + 1);

    return size;
}

static size_t
SDL_PrintUnsignedLongLong(char *text, Uint64 value, int radix, size_t maxlen)
{
    char num[130];
    size_t size;

    SDL_ulltoa(value, num, radix);
    size = SDL_strlen(num);
    if (size >= maxlen) {
        size = maxlen - 1;
    }
    SDL_strlcpy(text, num, size + 1);

    return size;
}
#endif /* SDL_HAS_64BIT_TYPE */
static size_t
SDL_PrintFloat(char *text, double arg, size_t maxlen)
{
    char *textstart = text;
    if (arg) {
        /* This isn't especially accurate, but hey, it's easy. :) */
        const double precision = 0.00000001;
        size_t len;
        unsigned long value;

        if (arg < 0) {
            *text++ = '-';
            --maxlen;
            arg = -arg;
        }
        value = (unsigned long) arg;
        len = SDL_PrintUnsignedLong(text, value, 10, maxlen);
        text += len;
        maxlen -= len;
        arg -= value;
        if (arg > precision && maxlen) {
            int mult = 10;
            *text++ = '.';
            while ((arg > precision) && maxlen) {
                value = (unsigned long) (arg * mult);
                len = SDL_PrintUnsignedLong(text, value, 10, maxlen);
                text += len;
                maxlen -= len;
                arg -= (double) value / mult;
                mult *= 10;
            }
        }
    } else {
        *text++ = '0';
    }
    return (text - textstart);
}

static size_t
SDL_PrintString(char *text, const char *string, size_t maxlen)
{
    char *textstart = text;
    while (*string && maxlen--) {
        *text++ = *string++;
    }
    return (text - textstart);
}

int
SDL_vsnprintf(char *text, size_t maxlen, const char *fmt, va_list ap)
{
    char *textstart = text;
    if (maxlen <= 0) {
        return 0;
    }
    --maxlen;                   /* For the trailing '\0' */
    while (*fmt && maxlen) {
        if (*fmt == '%') {
            SDL_bool done = SDL_FALSE;
            size_t len = 0;
            SDL_bool do_lowercase = SDL_FALSE;
            int radix = 10;
            enum
            {
                DO_INT,
                DO_LONG,
                DO_LONGLONG
            } inttype = DO_INT;

            ++fmt;
            /* FIXME: implement more of the format specifiers */
            while (*fmt == '.' || (*fmt >= '0' && *fmt <= '9')) {
                ++fmt;
            }
            while (!done) {
                switch (*fmt) {
                case '%':
                    *text = '%';
                    len = 1;
                    done = SDL_TRUE;
                    break;
                case 'c':
                    /* char is promoted to int when passed through (...) */
                    *text = (char) va_arg(ap, int);
                    len = 1;
                    done = SDL_TRUE;
                    break;
                case 'h':
                    /* short is promoted to int when passed through (...) */
                    break;
                case 'l':
                    if (inttype < DO_LONGLONG) {
                        ++inttype;
                    }
                    break;
                case 'I':
                    if (SDL_strncmp(fmt, "I64", 3) == 0) {
                        fmt += 2;
                        inttype = DO_LONGLONG;
                    }
                    break;
                case 'i':
                case 'd':
                    switch (inttype) {
                    case DO_INT:
                        len =
                            SDL_PrintLong(text,
                                          (long) va_arg(ap, int),
                                          radix, maxlen);
                        break;
                    case DO_LONG:
                        len =
                            SDL_PrintLong(text, va_arg(ap, long),
                                          radix, maxlen);
                        break;
                    case DO_LONGLONG:
#ifdef SDL_HAS_64BIT_TYPE
                        len =
                            SDL_PrintLongLong(text,
                                              va_arg(ap, Sint64),
                                              radix, maxlen);
#else
                        len =
                            SDL_PrintLong(text, va_arg(ap, long),
                                          radix, maxlen);
#endif
                        break;
                    }
                    done = SDL_TRUE;
                    break;
                case 'p':
                case 'x':
                    do_lowercase = SDL_TRUE;
                    /* Fall through to 'X' handling */
                case 'X':
                    if (radix == 10) {
                        radix = 16;
                    }
                    if (*fmt == 'p') {
                        inttype = DO_LONG;
                    }
                    /* Fall through to unsigned handling */
                case 'o':
                    if (radix == 10) {
                        radix = 8;
                    }
                    /* Fall through to unsigned handling */
                case 'u':
                    switch (inttype) {
                    case DO_INT:
                        len = SDL_PrintUnsignedLong(text, (unsigned long)
                                                    va_arg(ap,
                                                           unsigned
                                                           int),
                                                    radix, maxlen);
                        break;
                    case DO_LONG:
                        len =
                            SDL_PrintUnsignedLong(text,
                                                  va_arg(ap,
                                                         unsigned
                                                         long),
                                                  radix, maxlen);
                        break;
                    case DO_LONGLONG:
#ifdef SDL_HAS_64BIT_TYPE
                        len =
                            SDL_PrintUnsignedLongLong(text,
                                                      va_arg(ap,
                                                             Uint64),
                                                      radix, maxlen);
#else
                        len =
                            SDL_PrintUnsignedLong(text,
                                                  va_arg(ap,
                                                         unsigned
                                                         long),
                                                  radix, maxlen);
#endif
                        break;
                    }
                    if (do_lowercase) {
                        SDL_strlwr(text);
                    }
                    done = SDL_TRUE;
                    break;
                case 'f':
                    len = SDL_PrintFloat(text, va_arg(ap, double), maxlen);
                    done = SDL_TRUE;
                    break;
                case 's':
                    len = SDL_PrintString(text, va_arg(ap, char *), maxlen);
                    done = SDL_TRUE;
                    break;
                default:
                    done = SDL_TRUE;
                    break;
                }
                ++fmt;
            }
            text += len;
            maxlen -= len;
        } else {
            *text++ = *fmt++;
            --maxlen;
        }
    }
    *text = '\0';

    return (int)(text - textstart);
}
#endif
/* vi: set ts=4 sw=4 expandtab: */
