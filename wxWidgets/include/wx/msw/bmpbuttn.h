/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/bmpbuttn.h
// Purpose:     wxBitmapButton class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: bmpbuttn.h 36078 2005-11-03 19:38:20Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_BMPBUTTN_H_
#define _WX_BMPBUTTN_H_

#include "wx/button.h"
#include "wx/bitmap.h"
#include "wx/brush.h"

class WXDLLEXPORT wxBitmapButton : public wxBitmapButtonBase
{
public:
    wxBitmapButton() { }

    wxBitmapButton(wxWindow *parent,
                   wxWindowID id,
                   const wxBitmap& bitmap,
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize,
                   long style = wxBU_AUTODRAW,
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
                long style = wxBU_AUTODRAW,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxButtonNameStr);

    // Implementation
    virtual bool SetBackgroundColour(const wxColour& colour);
    virtual void SetDefault();
    virtual bool MSWOnDraw(WXDRAWITEMSTRUCT *item);
    virtual void DrawFace( WXHDC dc, int left, int top, int right, int bottom, bool sel );
    virtual void DrawButtonFocus( WXHDC dc, int left, int top, int right, int bottom, bool sel );
    virtual void DrawButtonDisable( WXHDC dc, int left, int top, int right, int bottom, bool with_marg );

protected:
    // reimplement some base class virtuals
    virtual wxSize DoGetBestSize() const;
    virtual void OnSetBitmap();

    // invalidate m_brushDisabled when system colours change
    void OnSysColourChanged(wxSysColourChangedEvent& event);

    // change the currently bitmap if we have a hover one
    void OnMouseEnterOrLeave(wxMouseEvent& event);


    // the brush we use to draw disabled buttons
    wxBrush m_brushDisabled;


    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxBitmapButton)
};

#endif // _WX_BMPBUTTN_H_
