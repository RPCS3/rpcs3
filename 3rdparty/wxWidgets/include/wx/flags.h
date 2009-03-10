/////////////////////////////////////////////////////////////////////////////
// Name:        wx/flags.h
// Purpose:     a bitset suited for replacing the current style flags
// Author:      Stefan Csomor
// Modified by:
// Created:     27/07/03
// RCS-ID:      $Id: flags.h 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) 2003 Stefan Csomor
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_SETH__
#define _WX_SETH__

// wxBitset should be applied to an enum, then this can be used like
// bitwise operators but keeps the type safety and information, the
// enums must be in a sequence , their value determines the bit position
// that they represent
// The api is made as close as possible to <bitset>

template <class T> class wxBitset
{
    friend class wxEnumData ;
public:
    // creates a wxBitset<> object with all flags initialized to 0
    wxBitset() { m_data = 0; }

    // created a wxBitset<> object initialized according to the bits of the
    // integral value val
    wxBitset(unsigned long val) { m_data = val ; }

    // copies the content in the new wxBitset<> object from another one
    wxBitset(const wxBitset &src) { m_data = src.m_data; }

    // creates a wxBitset<> object that has the specific flag set
    wxBitset(const T el) { m_data |= 1 << el; }

    // returns the integral value that the bits of this object represent
    unsigned long to_ulong() const { return m_data ; }

    // assignment
    wxBitset &operator =(const wxBitset &rhs)
    {
        m_data = rhs.m_data;
        return *this;
    }

    // bitwise or operator, sets all bits that are in rhs and leaves
    // the rest unchanged
    wxBitset &operator |=(const wxBitset &rhs)
    {
        m_data |= rhs.m_data;
        return *this;
    }

    // bitwsie exclusive-or operator, toggles the value of all bits
    // that are set in bits and leaves all others unchanged
    wxBitset &operator ^=(const wxBitset &rhs) // difference
    {
        m_data ^= rhs.m_data;
        return *this;
    }

    // bitwise and operator, resets all bits that are not in rhs and leaves
    // all others unchanged
    wxBitset &operator &=(const wxBitset &rhs) // intersection
    {
        m_data &= rhs.m_data;
        return *this;
    }

    // bitwise or operator, returns a new bitset that has all bits set that set are in
    // bitset2 or in this bitset
    wxBitset operator |(const wxBitset &bitset2) const // union
    {
        wxBitset<T> s;
        s.m_data = m_data | bitset2.m_data;
        return s;
    }

    // bitwise exclusive-or operator, returns a new bitset that has all bits set that are set either in
    // bitset2 or in this bitset but not in both
    wxBitset operator ^(const wxBitset &bitset2) const // difference
    {
        wxBitset<T> s;
        s.m_data = m_data ^ bitset2.m_data;
        return s;
    }

    // bitwise and operator, returns a new bitset that has all bits set that are set both in
    // bitset2 and in this bitset
    wxBitset operator &(const wxBitset &bitset2) const // intersection
    {
        wxBitset<T> s;
        s.m_data = m_data & bitset2.m_data;
        return s;
    }

    // sets appropriate the bit to true
    wxBitset& set(const T el) //Add element
    {
        m_data |= 1 << el;
        return *this;
    }

    // clears the appropriate flag to false
    wxBitset& reset(const T el) //remove element
    {
        m_data &= ~(1 << el);
        return *this;
    }

    // clear all flags
    wxBitset& reset()
    {
        m_data = 0;
        return *this;
    }

    // true if this flag is set
    bool test(const T el) const
    {
        return (m_data & (1 << el)) ? true : false;
    }

    // true if no flag is set
    bool none() const
    {
        return m_data == 0;
    }

    // true if any flag is set
    bool any() const
    {
        return m_data != 0;
    }

    // true if both have the same flags
    bool operator ==(const wxBitset &rhs) const
    {
        return m_data == rhs.m_data;
    }

    // true if both differ in their flags set
    bool operator !=(const wxBitset &rhs) const
    {
        return !operator==(rhs);
    }

    bool operator[] (const T el) const { return test(el) ; }

private :
    unsigned long m_data;
};

#define WX_DEFINE_FLAGS( flags ) \
    class WXDLLEXPORT flags \
    {\
    public : \
        flags(long data=0) :m_data(data) {} \
        long m_data ;\
        bool operator ==(const flags &rhs) const { return m_data == rhs.m_data; }\
    } ;

#endif
