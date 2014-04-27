///////////////////////////////////////////////////////////////////////////////
// Name:        wx/convauto.h
// Purpose:     wxConvAuto class declaration
// Author:      Vadim Zeitlin
// Created:     2006-04-03
// RCS-ID:      $Id: convauto.h 45893 2007-05-08 20:05:16Z VZ $
// Copyright:   (c) 2006 Vadim Zeitlin
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CONVAUTO_H_
#define _WX_CONVAUTO_H_

#include "wx/strconv.h"

#if wxUSE_WCHAR_T

// ----------------------------------------------------------------------------
// wxConvAuto: uses BOM to automatically detect input encoding
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxConvAuto : public wxMBConv
{
public:
    // default ctor, the real conversion will be created on demand
    wxConvAuto() { m_conv = NULL; /* the rest will be initialized later */ }

    // copy ctor doesn't initialize anything neither as conversion can only be
    // deduced on first use
    wxConvAuto(const wxConvAuto& WXUNUSED(other)) : wxMBConv() { m_conv = NULL; }

    virtual ~wxConvAuto() { if ( m_conv && m_ownsConv ) delete m_conv; }

    // override the base class virtual function(s) to use our m_conv
    virtual size_t ToWChar(wchar_t *dst, size_t dstLen,
                           const char *src, size_t srcLen = wxNO_LEN) const;

    virtual size_t FromWChar(char *dst, size_t dstLen,
                             const wchar_t *src, size_t srcLen = wxNO_LEN) const;

    virtual size_t GetMBNulLen() const { return m_conv->GetMBNulLen(); }

    virtual wxMBConv *Clone() const { return new wxConvAuto(*this); }

private:
    // all currently recognized BOM values
    enum BOMType
    {
        BOM_None,
        BOM_UTF32BE,
        BOM_UTF32LE,
        BOM_UTF16BE,
        BOM_UTF16LE,
        BOM_UTF8
    };

    // return the BOM type of this buffer
    static BOMType DetectBOM(const char *src, size_t srcLen);

    // initialize m_conv with the conversion to use by default (UTF-8)
    void InitWithDefault()
    {
        m_conv = &wxConvUTF8;
        m_ownsConv = false;
    }

    // create the correct conversion object for the given BOM type
    void InitFromBOM(BOMType bomType);

    // create the correct conversion object for the BOM present in the
    // beginning of the buffer; adjust the buffer to skip the BOM if found
    void InitFromInput(const char **src, size_t *len);

    // adjust src and len to skip over the BOM (identified by m_bomType) at the
    // start of the buffer
    void SkipBOM(const char **src, size_t *len) const;


    // conversion object which we really use, NULL until the first call to
    // either ToWChar() or FromWChar()
    wxMBConv *m_conv;

    // our BOM type
    BOMType m_bomType;

    // true if we allocated m_conv ourselves, false if we just use an existing
    // global conversion
    bool m_ownsConv;

    // true if we already skipped BOM when converting (and not just calculating
    // the size)
    bool m_consumedBOM;


    DECLARE_NO_ASSIGN_CLASS(wxConvAuto)
};

#else // !wxUSE_WCHAR_T

// it doesn't matter how we define it in this case as it's unused anyhow, but
// do define it to allow the code using wxConvAuto() as default argument (this
// is done in many places) to compile
typedef wxMBConv wxConvAuto;

#endif // wxUSE_WCHAR_T/!wxUSE_WCHAR_T

#endif // _WX_CONVAUTO_H_

