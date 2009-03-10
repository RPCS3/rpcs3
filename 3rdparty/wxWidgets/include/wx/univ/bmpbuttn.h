/////////////////////////////////////////////////////////////////////////////
// Name:        wx/univ/bmpbuttn.h
// Purpose:     wxBitmapButton class for wxUniversal
// Author:      Vadim Zeitlin
// Modified by:
// Created:     25.08.00
// RCS-ID:      $Id: bmpbuttn.h 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIV_BMPBUTTN_H_
#define _WX_UNIV_BMPBUTTN_H_

class WXDLLEXPORT wxBitmapButton : public wxBitmapButtonBase
{
public:
    wxBitmapButton() { }

    wxBitmapButton(wxWindow *parent,
                   wxWindowID id,
                   const wxBitmap& bitmap,
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize,
                   long style = 0,
                   const wxValidator& validator = wxDefaultValidator,
                   const wxString& name = wxButtonNameStr)
    {
        Create(parent, id, bitmap, pos, size, style, validator, name);
    }

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxBitmap& bitmap,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxButtonNameStr);

    virtual void SetMargins(int x, int y)
    {
        SetImageMargins(x, y);

        wxBitmapButtonBase::SetMargins(x, y);
    }

    virtual bool Enable(bool enable = true);

    virtual bool SetCurrent(bool doit = true);

    virtual void Press();
    virtual void Release();

protected:
    void OnSetFocus(wxFocusEvent& event);
    void OnKillFocus(wxFocusEvent& event);

    // called when one of the bitmap is changed by user
    virtual void OnSetBitmap();

    // set bitmap to the given one if it's ok or to m_bmpNormal and return
    // true if the bitmap really changed
    bool ChangeBitmap(const wxBitmap& bmp);

private:
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxBitmapButton)
};

#endif // _WX_UNIV_BMPBUTTN_H_

