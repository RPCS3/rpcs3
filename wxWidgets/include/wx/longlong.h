/////////////////////////////////////////////////////////////////////////////
// Name:        wx/longlong.h
// Purpose:     declaration of wxLongLong class - best implementation of a 64
//              bit integer for the current platform.
// Author:      Jeffrey C. Ollie <jeff@ollie.clive.ia.us>, Vadim Zeitlin
// Modified by:
// Created:     10.02.99
// RCS-ID:      $Id: longlong.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_LONGLONG_H
#define _WX_LONGLONG_H

#include "wx/defs.h"

#if wxUSE_LONGLONG

#include "wx/string.h"

#include <limits.h>     // for LONG_MAX

// define this to compile wxLongLongWx in "test" mode: the results of all
// calculations will be compared with the real results taken from
// wxLongLongNative -- this is extremely useful to find the bugs in
// wxLongLongWx class!

// #define wxLONGLONG_TEST_MODE

#ifdef wxLONGLONG_TEST_MODE
    #define wxUSE_LONGLONG_WX 1
    #define wxUSE_LONGLONG_NATIVE 1
#endif // wxLONGLONG_TEST_MODE

// ----------------------------------------------------------------------------
// decide upon which class we will use
// ----------------------------------------------------------------------------

#ifndef wxLongLong_t
    // both warning and pragma warning are not portable, but at least an
    // unknown pragma should never be an error -- except that, actually, some
    // broken compilers don't like it, so we have to disable it in this case
    // <sigh>
    #ifdef __GNUC__
        #warning "Your compiler does not appear to support 64 bit "\
                 "integers, using emulation class instead.\n" \
                 "Please report your compiler version to " \
                 "wx-dev@lists.wxwidgets.org!"
    #elif !(defined(__WATCOMC__) || defined(__VISAGECPP__))
        #pragma warning "Your compiler does not appear to support 64 bit "\
                        "integers, using emulation class instead.\n" \
                        "Please report your compiler version to " \
                        "wx-dev@lists.wxwidgets.org!"
    #endif

    #define wxUSE_LONGLONG_WX 1
#endif // compiler

// the user may predefine wxUSE_LONGLONG_NATIVE and/or wxUSE_LONGLONG_NATIVE
// to disable automatic testing (useful for the test program which defines
// both classes) but by default we only use one class
#if (defined(wxUSE_LONGLONG_WX) && wxUSE_LONGLONG_WX) || !defined(wxLongLong_t)
    // don't use both classes unless wxUSE_LONGLONG_NATIVE was explicitly set:
    // this is useful in test programs and only there
    #ifndef wxUSE_LONGLONG_NATIVE
        #define wxUSE_LONGLONG_NATIVE 0
    #endif

    class WXDLLIMPEXP_BASE wxLongLongWx;
    class WXDLLIMPEXP_BASE wxULongLongWx;
#if defined(__VISUALC__) && !defined(__WIN32__)
    #define wxLongLong wxLongLongWx
    #define wxULongLong wxULongLongWx
#else
    typedef wxLongLongWx wxLongLong;
    typedef wxULongLongWx wxULongLong;
#endif

#else
    // if nothing is defined, use native implementation by default, of course
    #ifndef wxUSE_LONGLONG_NATIVE
        #define wxUSE_LONGLONG_NATIVE 1
    #endif
#endif

#ifndef wxUSE_LONGLONG_WX
    #define wxUSE_LONGLONG_WX 0
    class WXDLLIMPEXP_FWD_BASE wxLongLongNative;
    class WXDLLIMPEXP_FWD_BASE wxULongLongNative;
    typedef wxLongLongNative wxLongLong;
    typedef wxULongLongNative wxULongLong;
#endif

// NB: if both wxUSE_LONGLONG_WX and NATIVE are defined, the user code should
//     typedef wxLongLong as it wants, we don't do it

// ----------------------------------------------------------------------------
// choose the appropriate class
// ----------------------------------------------------------------------------

// we use iostream for wxLongLong output
#include "wx/iosfwrap.h"

#if wxUSE_LONGLONG_NATIVE

class WXDLLIMPEXP_BASE wxLongLongNative
{
public:
    // ctors
        // default ctor initializes to 0
    wxLongLongNative() : m_ll(0) { }
        // from long long
    wxLongLongNative(wxLongLong_t ll) : m_ll(ll) { }
        // from 2 longs
    wxLongLongNative(wxInt32 hi, wxUint32 lo)
    {
        // cast to wxLongLong_t first to avoid precision loss!
        m_ll = ((wxLongLong_t) hi) << 32;
        m_ll |= (wxLongLong_t) lo;
    }
#if wxUSE_LONGLONG_WX
    wxLongLongNative(wxLongLongWx ll);
#endif

    // default copy ctor is ok

    // no dtor

    // assignment operators
        // from native 64 bit integer
#ifndef wxLongLongIsLong
    wxLongLongNative& operator=(wxLongLong_t ll)
        { m_ll = ll; return *this; }
    wxLongLongNative& operator=(wxULongLong_t ll)
        { m_ll = ll; return *this; }
#endif // !wxLongLongNative
    wxLongLongNative& operator=(const wxULongLongNative &ll);
    wxLongLongNative& operator=(int l)
        { m_ll = l; return *this; }
    wxLongLongNative& operator=(long l)
        { m_ll = l; return *this; }
    wxLongLongNative& operator=(unsigned int l)
        { m_ll = l; return *this; }
    wxLongLongNative& operator=(unsigned long l)
        { m_ll = l; return *this; }
#if wxUSE_LONGLONG_WX
    wxLongLongNative& operator=(wxLongLongWx ll);
    wxLongLongNative& operator=(const class wxULongLongWx &ll);
#endif


        // from double: this one has an explicit name because otherwise we
        // would have ambiguity with "ll = int" and also because we don't want
        // to have implicit conversions between doubles and wxLongLongs
    wxLongLongNative& Assign(double d)
        { m_ll = (wxLongLong_t)d; return *this; }

    // assignment operators from wxLongLongNative is ok

    // accessors
        // get high part
    wxInt32 GetHi() const
        { return wx_truncate_cast(wxInt32, m_ll >> 32); }
        // get low part
    wxUint32 GetLo() const
        { return wx_truncate_cast(wxUint32, m_ll); }

        // get absolute value
    wxLongLongNative Abs() const { return wxLongLongNative(*this).Abs(); }
    wxLongLongNative& Abs() { if ( m_ll < 0 ) m_ll = -m_ll; return *this; }

        // convert to native long long
    wxLongLong_t GetValue() const { return m_ll; }

        // convert to long with range checking in debug mode (only!)
    long ToLong() const
    {
        wxASSERT_MSG( (m_ll >= LONG_MIN) && (m_ll <= LONG_MAX),
                      wxT("wxLongLong to long conversion loss of precision") );

        return wx_truncate_cast(long, m_ll);
    }

        // convert to double
    double ToDouble() const { return wx_truncate_cast(double, m_ll); }

    // don't provide implicit conversion to wxLongLong_t or we will have an
    // ambiguity for all arithmetic operations
    //operator wxLongLong_t() const { return m_ll; }

    // operations
        // addition
    wxLongLongNative operator+(const wxLongLongNative& ll) const
        { return wxLongLongNative(m_ll + ll.m_ll); }
    wxLongLongNative& operator+=(const wxLongLongNative& ll)
        { m_ll += ll.m_ll; return *this; }

    wxLongLongNative operator+(const wxLongLong_t ll) const
        { return wxLongLongNative(m_ll + ll); }
    wxLongLongNative& operator+=(const wxLongLong_t ll)
        { m_ll += ll; return *this; }

        // pre increment
    wxLongLongNative& operator++()
        { m_ll++; return *this; }

        // post increment
    wxLongLongNative operator++(int)
        { wxLongLongNative value(*this); m_ll++; return value; }

        // negation operator
    wxLongLongNative operator-() const
        { return wxLongLongNative(-m_ll); }
    wxLongLongNative& Negate() { m_ll = -m_ll; return *this; }

        // subtraction
    wxLongLongNative operator-(const wxLongLongNative& ll) const
        { return wxLongLongNative(m_ll - ll.m_ll); }
    wxLongLongNative& operator-=(const wxLongLongNative& ll)
        { m_ll -= ll.m_ll; return *this; }

    wxLongLongNative operator-(const wxLongLong_t ll) const
        { return wxLongLongNative(m_ll - ll); }
    wxLongLongNative& operator-=(const wxLongLong_t ll)
        { m_ll -= ll; return *this; }

        // pre decrement
    wxLongLongNative& operator--()
        { m_ll--; return *this; }

        // post decrement
    wxLongLongNative operator--(int)
        { wxLongLongNative value(*this); m_ll--; return value; }

    // shifts
        // left shift
    wxLongLongNative operator<<(int shift) const
        { return wxLongLongNative(m_ll << shift); }
    wxLongLongNative& operator<<=(int shift)
        { m_ll <<= shift; return *this; }

        // right shift
    wxLongLongNative operator>>(int shift) const
        { return wxLongLongNative(m_ll >> shift); }
    wxLongLongNative& operator>>=(int shift)
        { m_ll >>= shift; return *this; }

    // bitwise operators
    wxLongLongNative operator&(const wxLongLongNative& ll) const
        { return wxLongLongNative(m_ll & ll.m_ll); }
    wxLongLongNative& operator&=(const wxLongLongNative& ll)
        { m_ll &= ll.m_ll; return *this; }

    wxLongLongNative operator|(const wxLongLongNative& ll) const
        { return wxLongLongNative(m_ll | ll.m_ll); }
    wxLongLongNative& operator|=(const wxLongLongNative& ll)
        { m_ll |= ll.m_ll; return *this; }

    wxLongLongNative operator^(const wxLongLongNative& ll) const
        { return wxLongLongNative(m_ll ^ ll.m_ll); }
    wxLongLongNative& operator^=(const wxLongLongNative& ll)
        { m_ll ^= ll.m_ll; return *this; }

    // multiplication/division
    wxLongLongNative operator*(const wxLongLongNative& ll) const
        { return wxLongLongNative(m_ll * ll.m_ll); }
    wxLongLongNative operator*(long l) const
        { return wxLongLongNative(m_ll * l); }
    wxLongLongNative& operator*=(const wxLongLongNative& ll)
        { m_ll *= ll.m_ll; return *this; }
    wxLongLongNative& operator*=(long l)
        { m_ll *= l; return *this; }

    wxLongLongNative operator/(const wxLongLongNative& ll) const
        { return wxLongLongNative(m_ll / ll.m_ll); }
    wxLongLongNative operator/(long l) const
        { return wxLongLongNative(m_ll / l); }
    wxLongLongNative& operator/=(const wxLongLongNative& ll)
        { m_ll /= ll.m_ll; return *this; }
    wxLongLongNative& operator/=(long l)
        { m_ll /= l; return *this; }

    wxLongLongNative operator%(const wxLongLongNative& ll) const
        { return wxLongLongNative(m_ll % ll.m_ll); }
    wxLongLongNative operator%(long l) const
        { return wxLongLongNative(m_ll % l); }

    // comparison
    bool operator==(const wxLongLongNative& ll) const
        { return m_ll == ll.m_ll; }
    bool operator==(long l) const
        { return m_ll == l; }
    bool operator!=(const wxLongLongNative& ll) const
        { return m_ll != ll.m_ll; }
    bool operator!=(long l) const
        { return m_ll != l; }
    bool operator<(const wxLongLongNative& ll) const
        { return m_ll < ll.m_ll; }
    bool operator<(long l) const
        { return m_ll < l; }
    bool operator>(const wxLongLongNative& ll) const
        { return m_ll > ll.m_ll; }
    bool operator>(long l) const
        { return m_ll > l; }
    bool operator<=(const wxLongLongNative& ll) const
        { return m_ll <= ll.m_ll; }
    bool operator<=(long l) const
        { return m_ll <= l; }
    bool operator>=(const wxLongLongNative& ll) const
        { return m_ll >= ll.m_ll; }
    bool operator>=(long l) const
        { return m_ll >= l; }

    // miscellaneous

        // return the string representation of this number
    wxString ToString() const;

        // conversion to byte array: returns a pointer to static buffer!
    void *asArray() const;

#if wxUSE_STD_IOSTREAM
        // input/output
    friend WXDLLIMPEXP_BASE
    wxSTD ostream& operator<<(wxSTD ostream&, const wxLongLongNative&);
#endif

    friend WXDLLIMPEXP_BASE
    wxString& operator<<(wxString&, const wxLongLongNative&);

#if wxUSE_STREAMS
    friend WXDLLIMPEXP_BASE
    class wxTextOutputStream& operator<<(class wxTextOutputStream&, const wxLongLongNative&);
    friend WXDLLIMPEXP_BASE
    class wxTextInputStream& operator>>(class wxTextInputStream&, wxLongLongNative&);
#endif

private:
    wxLongLong_t  m_ll;
};


class WXDLLIMPEXP_BASE wxULongLongNative
{
public:
    // ctors
        // default ctor initializes to 0
    wxULongLongNative() : m_ll(0) { }
        // from long long
    wxULongLongNative(wxULongLong_t ll) : m_ll(ll) { }
        // from 2 longs
    wxULongLongNative(wxUint32 hi, wxUint32 lo) : m_ll(0)
    {
        // cast to wxLongLong_t first to avoid precision loss!
        m_ll = ((wxULongLong_t) hi) << 32;
        m_ll |= (wxULongLong_t) lo;
    }

#if wxUSE_LONGLONG_WX
    wxULongLongNative(const class wxULongLongWx &ll);
#endif

    // default copy ctor is ok

    // no dtor

    // assignment operators
        // from native 64 bit integer
#ifndef wxLongLongIsLong
    wxULongLongNative& operator=(wxULongLong_t ll)
        { m_ll = ll; return *this; }
    wxULongLongNative& operator=(wxLongLong_t ll)
        { m_ll = ll; return *this; }
#endif // !wxLongLongNative
    wxULongLongNative& operator=(int l)
        { m_ll = l; return *this; }
    wxULongLongNative& operator=(long l)
        { m_ll = l; return *this; }
    wxULongLongNative& operator=(unsigned int l)
        { m_ll = l; return *this; }
    wxULongLongNative& operator=(unsigned long l)
        { m_ll = l; return *this; }
    wxULongLongNative& operator=(const wxLongLongNative &ll)
        { m_ll = ll.GetValue(); return *this; }
#if wxUSE_LONGLONG_WX
    wxULongLongNative& operator=(wxLongLongWx ll);
    wxULongLongNative& operator=(const class wxULongLongWx &ll);
#endif

    // assignment operators from wxULongLongNative is ok

    // accessors
        // get high part
    wxUint32 GetHi() const
        { return wx_truncate_cast(wxUint32, m_ll >> 32); }
        // get low part
    wxUint32 GetLo() const
        { return wx_truncate_cast(wxUint32, m_ll); }

        // convert to native ulong long
    wxULongLong_t GetValue() const { return m_ll; }

        // convert to ulong with range checking in debug mode (only!)
    unsigned long ToULong() const
    {
        wxASSERT_MSG( m_ll <= LONG_MAX,
                      wxT("wxULongLong to long conversion loss of precision") );

        return wx_truncate_cast(unsigned long, m_ll);
    }

        // convert to double
#ifdef _MSC_VER
    double ToDouble() const { return wx_truncate_cast(double, (__int64) m_ll); }
#else
    double ToDouble() const { return wx_truncate_cast(double, m_ll); }
#endif

    // operations
        // addition
    wxULongLongNative operator+(const wxULongLongNative& ll) const
        { return wxULongLongNative(m_ll + ll.m_ll); }
    wxULongLongNative& operator+=(const wxULongLongNative& ll)
        { m_ll += ll.m_ll; return *this; }

    wxULongLongNative operator+(const wxULongLong_t ll) const
        { return wxULongLongNative(m_ll + ll); }
    wxULongLongNative& operator+=(const wxULongLong_t ll)
        { m_ll += ll; return *this; }

        // pre increment
    wxULongLongNative& operator++()
        { m_ll++; return *this; }

        // post increment
    wxULongLongNative operator++(int)
        { wxULongLongNative value(*this); m_ll++; return value; }

        // subtraction
    wxULongLongNative operator-(const wxULongLongNative& ll) const
        { return wxULongLongNative(m_ll - ll.m_ll); }
    wxULongLongNative& operator-=(const wxULongLongNative& ll)
        { m_ll -= ll.m_ll; return *this; }

    wxULongLongNative operator-(const wxULongLong_t ll) const
        { return wxULongLongNative(m_ll - ll); }
    wxULongLongNative& operator-=(const wxULongLong_t ll)
        { m_ll -= ll; return *this; }

        // pre decrement
    wxULongLongNative& operator--()
        { m_ll--; return *this; }

        // post decrement
    wxULongLongNative operator--(int)
        { wxULongLongNative value(*this); m_ll--; return value; }

    // shifts
        // left shift
    wxULongLongNative operator<<(int shift) const
        { return wxULongLongNative(m_ll << shift); }
    wxULongLongNative& operator<<=(int shift)
        { m_ll <<= shift; return *this; }

        // right shift
    wxULongLongNative operator>>(int shift) const
        { return wxULongLongNative(m_ll >> shift); }
    wxULongLongNative& operator>>=(int shift)
        { m_ll >>= shift; return *this; }

    // bitwise operators
    wxULongLongNative operator&(const wxULongLongNative& ll) const
        { return wxULongLongNative(m_ll & ll.m_ll); }
    wxULongLongNative& operator&=(const wxULongLongNative& ll)
        { m_ll &= ll.m_ll; return *this; }

    wxULongLongNative operator|(const wxULongLongNative& ll) const
        { return wxULongLongNative(m_ll | ll.m_ll); }
    wxULongLongNative& operator|=(const wxULongLongNative& ll)
        { m_ll |= ll.m_ll; return *this; }

    wxULongLongNative operator^(const wxULongLongNative& ll) const
        { return wxULongLongNative(m_ll ^ ll.m_ll); }
    wxULongLongNative& operator^=(const wxULongLongNative& ll)
        { m_ll ^= ll.m_ll; return *this; }

    // multiplication/division
    wxULongLongNative operator*(const wxULongLongNative& ll) const
        { return wxULongLongNative(m_ll * ll.m_ll); }
    wxULongLongNative operator*(unsigned long l) const
        { return wxULongLongNative(m_ll * l); }
    wxULongLongNative& operator*=(const wxULongLongNative& ll)
        { m_ll *= ll.m_ll; return *this; }
    wxULongLongNative& operator*=(unsigned long l)
        { m_ll *= l; return *this; }

    wxULongLongNative operator/(const wxULongLongNative& ll) const
        { return wxULongLongNative(m_ll / ll.m_ll); }
    wxULongLongNative operator/(unsigned long l) const
        { return wxULongLongNative(m_ll / l); }
    wxULongLongNative& operator/=(const wxULongLongNative& ll)
        { m_ll /= ll.m_ll; return *this; }
    wxULongLongNative& operator/=(unsigned long l)
        { m_ll /= l; return *this; }

    wxULongLongNative operator%(const wxULongLongNative& ll) const
        { return wxULongLongNative(m_ll % ll.m_ll); }
    wxULongLongNative operator%(unsigned long l) const
        { return wxULongLongNative(m_ll % l); }

    // comparison
    bool operator==(const wxULongLongNative& ll) const
        { return m_ll == ll.m_ll; }
    bool operator==(unsigned long l) const
        { return m_ll == l; }
    bool operator!=(const wxULongLongNative& ll) const
        { return m_ll != ll.m_ll; }
    bool operator!=(unsigned long l) const
        { return m_ll != l; }
    bool operator<(const wxULongLongNative& ll) const
        { return m_ll < ll.m_ll; }
    bool operator<(unsigned long l) const
        { return m_ll < l; }
    bool operator>(const wxULongLongNative& ll) const
        { return m_ll > ll.m_ll; }
    bool operator>(unsigned long l) const
        { return m_ll > l; }
    bool operator<=(const wxULongLongNative& ll) const
        { return m_ll <= ll.m_ll; }
    bool operator<=(unsigned long l) const
        { return m_ll <= l; }
    bool operator>=(const wxULongLongNative& ll) const
        { return m_ll >= ll.m_ll; }
    bool operator>=(unsigned long l) const
        { return m_ll >= l; }

    // miscellaneous

        // return the string representation of this number
    wxString ToString() const;

        // conversion to byte array: returns a pointer to static buffer!
    void *asArray() const;

#if wxUSE_STD_IOSTREAM
        // input/output
    friend WXDLLIMPEXP_BASE
    wxSTD ostream& operator<<(wxSTD ostream&, const wxULongLongNative&);
#endif

    friend WXDLLIMPEXP_BASE
    wxString& operator<<(wxString&, const wxULongLongNative&);

#if wxUSE_STREAMS
    friend WXDLLIMPEXP_BASE
    class wxTextOutputStream& operator<<(class wxTextOutputStream&, const wxULongLongNative&);
    friend WXDLLIMPEXP_BASE
    class wxTextInputStream& operator>>(class wxTextInputStream&, wxULongLongNative&);
#endif

private:
    wxULongLong_t  m_ll;
};

inline
wxLongLongNative& wxLongLongNative::operator=(const wxULongLongNative &ll)
{
    m_ll = ll.GetValue();
    return *this;
}

#endif // wxUSE_LONGLONG_NATIVE

#if wxUSE_LONGLONG_WX

class WXDLLIMPEXP_BASE wxLongLongWx
{
public:
    // ctors
        // default ctor initializes to 0
    wxLongLongWx()
    {
        m_lo = m_hi = 0;

#ifdef wxLONGLONG_TEST_MODE
        m_ll = 0;

        Check();
#endif // wxLONGLONG_TEST_MODE
    }
        // from long
    wxLongLongWx(long l) { *this = l; }
        // from 2 longs
    wxLongLongWx(long hi, unsigned long lo)
    {
        m_hi = hi;
        m_lo = lo;

#ifdef wxLONGLONG_TEST_MODE
        m_ll = hi;
        m_ll <<= 32;
        m_ll |= lo;

        Check();
#endif // wxLONGLONG_TEST_MODE
    }

    // default copy ctor is ok in both cases

    // no dtor

    // assignment operators
        // from long
    wxLongLongWx& operator=(long l)
    {
        m_lo = l;
        m_hi = (l < 0 ? -1l : 0l);

#ifdef wxLONGLONG_TEST_MODE
        m_ll = l;

        Check();
#endif // wxLONGLONG_TEST_MODE

        return *this;
    }
        // from int
    wxLongLongWx& operator=(int l)
    {
        return operator=((long)l);
    }

    wxLongLongWx& operator=(unsigned long l)
    {
        m_lo = l;
        m_hi = 0;

#ifdef wxLONGLONG_TEST_MODE
        m_ll = l;

        Check();
#endif // wxLONGLONG_TEST_MODE

        return *this;
    }

    wxLongLongWx& operator=(unsigned int l)
    {
        return operator=((unsigned long)l);
    }

    wxLongLongWx& operator=(const class wxULongLongWx &ll);

    // from double
    wxLongLongWx& Assign(double d);
        // can't have assignment operator from 2 longs

    // accessors
        // get high part
    long GetHi() const { return m_hi; }
        // get low part
    unsigned long GetLo() const { return m_lo; }

        // get absolute value
    wxLongLongWx Abs() const { return wxLongLongWx(*this).Abs(); }
    wxLongLongWx& Abs()
    {
        if ( m_hi < 0 )
            m_hi = -m_hi;

#ifdef wxLONGLONG_TEST_MODE
        if ( m_ll < 0 )
            m_ll = -m_ll;

        Check();
#endif // wxLONGLONG_TEST_MODE

        return *this;
    }

        // convert to long with range checking in debug mode (only!)
    long ToLong() const
    {
        wxASSERT_MSG( (m_hi == 0l) || (m_hi == -1l),
                      wxT("wxLongLong to long conversion loss of precision") );

        return (long)m_lo;
    }

        // convert to double
    double ToDouble() const;

    // operations
        // addition
    wxLongLongWx operator+(const wxLongLongWx& ll) const;
    wxLongLongWx& operator+=(const wxLongLongWx& ll);
    wxLongLongWx operator+(long l) const;
    wxLongLongWx& operator+=(long l);

        // pre increment operator
    wxLongLongWx& operator++();

        // post increment operator
    wxLongLongWx& operator++(int) { return ++(*this); }

        // negation operator
    wxLongLongWx operator-() const;
    wxLongLongWx& Negate();

        // subraction
    wxLongLongWx operator-(const wxLongLongWx& ll) const;
    wxLongLongWx& operator-=(const wxLongLongWx& ll);

        // pre decrement operator
    wxLongLongWx& operator--();

        // post decrement operator
    wxLongLongWx& operator--(int) { return --(*this); }

    // shifts
        // left shift
    wxLongLongWx operator<<(int shift) const;
    wxLongLongWx& operator<<=(int shift);

        // right shift
    wxLongLongWx operator>>(int shift) const;
    wxLongLongWx& operator>>=(int shift);

    // bitwise operators
    wxLongLongWx operator&(const wxLongLongWx& ll) const;
    wxLongLongWx& operator&=(const wxLongLongWx& ll);
    wxLongLongWx operator|(const wxLongLongWx& ll) const;
    wxLongLongWx& operator|=(const wxLongLongWx& ll);
    wxLongLongWx operator^(const wxLongLongWx& ll) const;
    wxLongLongWx& operator^=(const wxLongLongWx& ll);
    wxLongLongWx operator~() const;

    // comparison
    bool operator==(const wxLongLongWx& ll) const
        { return m_lo == ll.m_lo && m_hi == ll.m_hi; }
#if wxUSE_LONGLONG_NATIVE
    bool operator==(const wxLongLongNative& ll) const
        { return m_lo == ll.GetLo() && m_hi == ll.GetHi(); }
#endif
    bool operator!=(const wxLongLongWx& ll) const
        { return !(*this == ll); }
    bool operator<(const wxLongLongWx& ll) const;
    bool operator>(const wxLongLongWx& ll) const;
    bool operator<=(const wxLongLongWx& ll) const
        { return *this < ll || *this == ll; }
    bool operator>=(const wxLongLongWx& ll) const
        { return *this > ll || *this == ll; }

    bool operator<(long l) const { return *this < wxLongLongWx(l); }
    bool operator>(long l) const { return *this > wxLongLongWx(l); }
    bool operator==(long l) const
    {
        return l >= 0 ? (m_hi == 0 && m_lo == (unsigned long)l)
                      : (m_hi == -1 && m_lo == (unsigned long)l);
    }

    bool operator<=(long l) const { return *this < l || *this == l; }
    bool operator>=(long l) const { return *this > l || *this == l; }

    // multiplication
    wxLongLongWx operator*(const wxLongLongWx& ll) const;
    wxLongLongWx& operator*=(const wxLongLongWx& ll);

    // division
    wxLongLongWx operator/(const wxLongLongWx& ll) const;
    wxLongLongWx& operator/=(const wxLongLongWx& ll);

    wxLongLongWx operator%(const wxLongLongWx& ll) const;

    void Divide(const wxLongLongWx& divisor,
                wxLongLongWx& quotient,
                wxLongLongWx& remainder) const;

    // input/output

    // return the string representation of this number
    wxString ToString() const;

    void *asArray() const;

#if wxUSE_STD_IOSTREAM
    friend WXDLLIMPEXP_BASE
    wxSTD ostream& operator<<(wxSTD ostream&, const wxLongLongWx&);
#endif // wxUSE_STD_IOSTREAM

    friend WXDLLIMPEXP_BASE
    wxString& operator<<(wxString&, const wxLongLongWx&);

#if wxUSE_STREAMS
    friend WXDLLIMPEXP_BASE
    class wxTextOutputStream& operator<<(class wxTextOutputStream&, const wxLongLongWx&);
    friend WXDLLIMPEXP_BASE
    class wxTextInputStream& operator>>(class wxTextInputStream&, wxLongLongWx&);
#endif

private:
    // long is at least 32 bits, so represent our 64bit number as 2 longs

    long m_hi;                // signed bit is in the high part
    unsigned long m_lo;

#ifdef wxLONGLONG_TEST_MODE
    void Check()
    {
        wxASSERT( (m_ll >> 32) == m_hi && (unsigned long)m_ll == m_lo );
    }

    wxLongLong_t m_ll;
#endif // wxLONGLONG_TEST_MODE
};


class WXDLLIMPEXP_BASE wxULongLongWx
{
public:
    // ctors
        // default ctor initializes to 0
    wxULongLongWx()
    {
        m_lo = m_hi = 0;

#ifdef wxLONGLONG_TEST_MODE
        m_ll = 0;

        Check();
#endif // wxLONGLONG_TEST_MODE
    }
        // from ulong
    wxULongLongWx(unsigned long l) { *this = l; }
        // from 2 ulongs
    wxULongLongWx(unsigned long hi, unsigned long lo)
    {
        m_hi = hi;
        m_lo = lo;

#ifdef wxLONGLONG_TEST_MODE
        m_ll = hi;
        m_ll <<= 32;
        m_ll |= lo;

        Check();
#endif // wxLONGLONG_TEST_MODE
    }

    // from signed to unsigned
    wxULongLongWx(wxLongLongWx ll)
    {
        wxASSERT(ll.GetHi() >= 0);
        m_hi = (unsigned long)ll.GetHi();
        m_lo = ll.GetLo();
    }

    // default copy ctor is ok in both cases

    // no dtor

    // assignment operators
        // from long
    wxULongLongWx& operator=(unsigned long l)
    {
        m_lo = l;
        m_hi = 0;

#ifdef wxLONGLONG_TEST_MODE
        m_ll = l;

        Check();
#endif // wxLONGLONG_TEST_MODE

        return *this;
    }
    wxULongLongWx& operator=(long l)
    {
        m_lo = l;
        m_hi = (unsigned long) ((l<0) ? -1l : 0);

#ifdef wxLONGLONG_TEST_MODE
        m_ll = (wxULongLong_t) (wxLongLong_t) l;

        Check();
#endif // wxLONGLONG_TEST_MODE

        return *this;
    }
    wxULongLongWx& operator=(const class wxLongLongWx &ll) {
        // Should we use an assert like it was before in the constructor?
        // wxASSERT(ll.GetHi() >= 0);
        m_hi = (unsigned long)ll.GetHi();
        m_lo = ll.GetLo();
        return *this;
    }

    // can't have assignment operator from 2 longs

    // accessors
        // get high part
    unsigned long GetHi() const { return m_hi; }
        // get low part
    unsigned long GetLo() const { return m_lo; }

        // convert to long with range checking in debug mode (only!)
    unsigned long ToULong() const
    {
        wxASSERT_MSG( m_hi == 0ul,
                      wxT("wxULongLong to long conversion loss of precision") );

        return (unsigned long)m_lo;
    }

        // convert to double
    double ToDouble() const;

    // operations
        // addition
    wxULongLongWx operator+(const wxULongLongWx& ll) const;
    wxULongLongWx& operator+=(const wxULongLongWx& ll);
    wxULongLongWx operator+(unsigned long l) const;
    wxULongLongWx& operator+=(unsigned long l);

        // pre increment operator
    wxULongLongWx& operator++();

        // post increment operator
    wxULongLongWx& operator++(int) { return ++(*this); }

        // subtraction
    wxLongLongWx operator-(const wxULongLongWx& ll) const;
    wxULongLongWx& operator-=(const wxULongLongWx& ll);

        // pre decrement operator
    wxULongLongWx& operator--();

        // post decrement operator
    wxULongLongWx& operator--(int) { return --(*this); }

    // shifts
        // left shift
    wxULongLongWx operator<<(int shift) const;
    wxULongLongWx& operator<<=(int shift);

        // right shift
    wxULongLongWx operator>>(int shift) const;
    wxULongLongWx& operator>>=(int shift);

    // bitwise operators
    wxULongLongWx operator&(const wxULongLongWx& ll) const;
    wxULongLongWx& operator&=(const wxULongLongWx& ll);
    wxULongLongWx operator|(const wxULongLongWx& ll) const;
    wxULongLongWx& operator|=(const wxULongLongWx& ll);
    wxULongLongWx operator^(const wxULongLongWx& ll) const;
    wxULongLongWx& operator^=(const wxULongLongWx& ll);
    wxULongLongWx operator~() const;

    // comparison
    bool operator==(const wxULongLongWx& ll) const
        { return m_lo == ll.m_lo && m_hi == ll.m_hi; }
    bool operator!=(const wxULongLongWx& ll) const
        { return !(*this == ll); }
    bool operator<(const wxULongLongWx& ll) const;
    bool operator>(const wxULongLongWx& ll) const;
    bool operator<=(const wxULongLongWx& ll) const
        { return *this < ll || *this == ll; }
    bool operator>=(const wxULongLongWx& ll) const
        { return *this > ll || *this == ll; }

    bool operator<(unsigned long l) const { return *this < wxULongLongWx(l); }
    bool operator>(unsigned long l) const { return *this > wxULongLongWx(l); }
    bool operator==(unsigned long l) const
    {
        return (m_hi == 0 && m_lo == (unsigned long)l);
    }

    bool operator<=(unsigned long l) const { return *this < l || *this == l; }
    bool operator>=(unsigned long l) const { return *this > l || *this == l; }

    // multiplication
    wxULongLongWx operator*(const wxULongLongWx& ll) const;
    wxULongLongWx& operator*=(const wxULongLongWx& ll);

    // division
    wxULongLongWx operator/(const wxULongLongWx& ll) const;
    wxULongLongWx& operator/=(const wxULongLongWx& ll);

    wxULongLongWx operator%(const wxULongLongWx& ll) const;

    void Divide(const wxULongLongWx& divisor,
                wxULongLongWx& quotient,
                wxULongLongWx& remainder) const;

    // input/output

    // return the string representation of this number
    wxString ToString() const;

    void *asArray() const;

#if wxUSE_STD_IOSTREAM
    friend WXDLLIMPEXP_BASE
    wxSTD ostream& operator<<(wxSTD ostream&, const wxULongLongWx&);
#endif // wxUSE_STD_IOSTREAM

    friend WXDLLIMPEXP_BASE
    wxString& operator<<(wxString&, const wxULongLongWx&);

#if wxUSE_STREAMS
    friend WXDLLIMPEXP_BASE
    class wxTextOutputStream& operator<<(class wxTextOutputStream&, const wxULongLongWx&);
    friend WXDLLIMPEXP_BASE
    class wxTextInputStream& operator>>(class wxTextInputStream&, wxULongLongWx&);
#endif

private:
    // long is at least 32 bits, so represent our 64bit number as 2 longs

    unsigned long m_hi;
    unsigned long m_lo;

#ifdef wxLONGLONG_TEST_MODE
    void Check()
    {
        wxASSERT( (m_ll >> 32) == m_hi && (unsigned long)m_ll == m_lo );
    }

    wxULongLong_t m_ll;
#endif // wxLONGLONG_TEST_MODE
};

#endif // wxUSE_LONGLONG_WX

// ----------------------------------------------------------------------------
// binary operators
// ----------------------------------------------------------------------------

inline bool operator<(long l, const wxLongLong& ll) { return ll > l; }
inline bool operator>(long l, const wxLongLong& ll) { return ll < l; }
inline bool operator<=(long l, const wxLongLong& ll) { return ll >= l; }
inline bool operator>=(long l, const wxLongLong& ll) { return ll <= l; }
inline bool operator==(long l, const wxLongLong& ll) { return ll == l; }
inline bool operator!=(long l, const wxLongLong& ll) { return ll != l; }

inline wxLongLong operator+(long l, const wxLongLong& ll) { return ll + l; }
inline wxLongLong operator-(long l, const wxLongLong& ll)
{
    return wxLongLong(l) - ll;
}

inline bool operator<(unsigned long l, const wxULongLong& ull) { return ull > l; }
inline bool operator>(unsigned long l, const wxULongLong& ull) { return ull < l; }
inline bool operator<=(unsigned long l, const wxULongLong& ull) { return ull >= l; }
inline bool operator>=(unsigned long l, const wxULongLong& ull) { return ull <= l; }
inline bool operator==(unsigned long l, const wxULongLong& ull) { return ull == l; }
inline bool operator!=(unsigned long l, const wxULongLong& ull) { return ull != l; }

inline wxULongLong operator+(unsigned long l, const wxULongLong& ull) { return ull + l; }

inline wxLongLong operator-(unsigned long l, const wxULongLong& ull)
{
    wxULongLong ret = wxULongLong(l) - ull;
    return wxLongLong((long)ret.GetHi(),ret.GetLo());
}

#if wxUSE_LONGLONG_NATIVE && wxUSE_STREAMS

WXDLLIMPEXP_BASE class wxTextOutputStream &operator<<(class wxTextOutputStream &stream, wxULongLong_t value);
WXDLLIMPEXP_BASE class wxTextOutputStream &operator<<(class wxTextOutputStream &stream, wxLongLong_t value);

WXDLLIMPEXP_BASE class wxTextInputStream &operator>>(class wxTextInputStream &stream, wxULongLong_t &value);
WXDLLIMPEXP_BASE class wxTextInputStream &operator>>(class wxTextInputStream &stream, wxLongLong_t &value);

#endif

#endif // wxUSE_LONGLONG

#endif // _WX_LONGLONG_H
