/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/font.h
// Purpose:     wxFont class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: font.h 39411 2006-05-29 00:03:36Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_FONT_H_
#define _WX_FONT_H_

#include "wx/gdicmn.h"

// ----------------------------------------------------------------------------
// wxFont
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxFont : public wxFontBase
{
public:
    // ctors and such
    wxFont() { }

    wxFont(int size,
           int family,
           int style,
           int weight,
           bool underlined = false,
           const wxString& face = wxEmptyString,
           wxFontEncoding encoding = wxFONTENCODING_DEFAULT)
    {
        (void)Create(size, family, style, weight, underlined, face, encoding);
    }

    wxFont(const wxSize& pixelSize,
           int family,
           int style,
           int weight,
           bool underlined = false,
           const wxString& face = wxEmptyString,
           wxFontEncoding encoding = wxFONTENCODING_DEFAULT)
    {
        (void)Create(pixelSize, family, style, weight,
                     underlined, face, encoding);
    }

    wxFont(const wxNativeFontInfo& info, WXHFONT hFont = 0)
    {
        Create(info, hFont);
    }

    wxFont(const wxString& fontDesc);

    bool Create(int size,
                int family,
                int style,
                int weight,
                bool underlined = false,
                const wxString& face = wxEmptyString,
                wxFontEncoding encoding = wxFONTENCODING_DEFAULT)
    {
        return DoCreate(size, wxDefaultSize, false, family, style,
                        weight, underlined, face, encoding);
    }

    bool Create(const wxSize& pixelSize,
                int family,
                int style,
                int weight,
                bool underlined = false,
                const wxString& face = wxEmptyString,
                wxFontEncoding encoding = wxFONTENCODING_DEFAULT)
    {
        return DoCreate(-1, pixelSize, true, family, style,
                        weight, underlined, face, encoding);
    }

    bool Create(const wxNativeFontInfo& info, WXHFONT hFont = 0);

    virtual ~wxFont();

    // wxFontBase overridden functions
    virtual wxString GetNativeFontInfoDesc() const;
    virtual wxString GetNativeFontInfoUserDesc() const;

    // implement base class pure virtuals
    virtual int GetPointSize() const;
    virtual wxSize GetPixelSize() const;
    virtual bool IsUsingSizeInPixels() const;
    virtual int GetFamily() const;
    virtual int GetStyle() const;
    virtual int GetWeight() const;
    virtual bool GetUnderlined() const;
    virtual wxString GetFaceName() const;
    virtual wxFontEncoding GetEncoding() const;
    virtual const wxNativeFontInfo *GetNativeFontInfo() const;

    virtual void SetPointSize(int pointSize);
    virtual void SetPixelSize(const wxSize& pixelSize);
    virtual void SetFamily(int family);
    virtual void SetStyle(int style);
    virtual void SetWeight(int weight);
    virtual bool SetFaceName(const wxString& faceName);
    virtual void SetUnderlined(bool underlined);
    virtual void SetEncoding(wxFontEncoding encoding);

    virtual bool IsFixedWidth() const;

    // implementation only from now on
    // -------------------------------

    virtual bool IsFree() const;
    virtual bool RealizeResource();
    virtual WXHANDLE GetResourceHandle() const;
    virtual bool FreeResource(bool force = false);

    // for consistency with other wxMSW classes
    WXHFONT GetHFONT() const;

    /*
       virtual bool UseResource();
       virtual bool ReleaseResource();
     */

protected:
    // real font creation function, used in all cases
    bool DoCreate(int size,
                  const wxSize& pixelSize,
                  bool sizeUsingPixels,
                  int family,
                  int style,
                  int weight,
                  bool underlined = false,
                  const wxString& face = wxEmptyString,
                  wxFontEncoding encoding = wxFONTENCODING_DEFAULT);

    virtual void DoSetNativeFontInfo(const wxNativeFontInfo& info);

    void Unshare();

private:
    DECLARE_DYNAMIC_CLASS(wxFont)
};

#endif // _WX_FONT_H_
