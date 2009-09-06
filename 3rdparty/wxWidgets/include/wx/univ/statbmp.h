/////////////////////////////////////////////////////////////////////////////
// Name:        wx/univ/statbmp.h
// Purpose:     wxStaticBitmap class for wxUniversal
// Author:      Vadim Zeitlin
// Modified by:
// Created:     25.08.00
// RCS-ID:      $Id: statbmp.h 37393 2006-02-08 21:47:09Z VZ $
// Copyright:   (c) 2000 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIV_STATBMP_H_
#define _WX_UNIV_STATBMP_H_

#include "wx/bitmap.h"

// ----------------------------------------------------------------------------
// wxStaticBitmap
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxStaticBitmap : public wxStaticBitmapBase
{
public:
    wxStaticBitmap()
    {
    }

    wxStaticBitmap(wxWindow *parent,
                   const wxBitmap& label,
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize,
                   long style = 0)
    {
        Create(parent, wxID_ANY, label, pos, size, style);
    }

    wxStaticBitmap(wxWindow *parent,
                   wxWindowID id,
                   const wxBitmap& label,
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize,
                   long style = 0,
                   const wxString& name = wxStaticBitmapNameStr)
    {
        Create(parent, id, label, pos, size, style, name);
    }

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxBitmap& label,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString& name = wxStaticBitmapNameStr);

    virtual void SetBitmap(const wxBitmap& bitmap);
    virtual void SetIcon(const wxIcon& icon);
    virtual wxBitmap GetBitmap() const { return m_bitmap; }

    wxIcon GetIcon() const;

    virtual bool HasTransparentBackground() { return true; }

protected:
    virtual void DoDraw(wxControlRenderer *renderer);

private:
    // the bitmap which we show
    wxBitmap m_bitmap;

    DECLARE_DYNAMIC_CLASS(wxStaticBitmap)
};

#endif // _WX_UNIV_STATBMP_H_
