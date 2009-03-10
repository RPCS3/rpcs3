///////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/mask.h
// Purpose:     generic implementation of wxMask
// Author:      Vadim Zeitlin
// Created:     2006-09-28
// RCS-ID:      $Id: mask.h 41495 2006-09-28 23:02:39Z VZ $
// Copyright:   (c) 2006 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_MASKG_H_
#define _WX_GENERIC_MASKG_H_

// ----------------------------------------------------------------------------
// generic wxMask implementation
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxMask : public wxMaskBase
{
public:
    wxMask() { }
    wxMask(const wxBitmap& bitmap, const wxColour& colour)
    {
        InitFromColour(bitmap, colour);
    }

#if wxUSE_PALETTE
    wxMask(const wxBitmap& bitmap, int paletteIndex)
    {
        Create(bitmap, paletteIndex);
    }
#endif // wxUSE_PALETTE

    wxMask(const wxBitmap& bitmap)
    {
        InitFromMonoBitmap(bitmap);
    }

    // implementation-only from now on
    wxBitmap GetBitmap() const { return m_bitmap; }

private:
    // implement wxMaskBase pure virtuals
    virtual void FreeData();
    virtual bool InitFromColour(const wxBitmap& bitmap, const wxColour& colour);
    virtual bool InitFromMonoBitmap(const wxBitmap& bitmap);

    wxBitmap m_bitmap;

    DECLARE_DYNAMIC_CLASS(wxMask)
};

#endif // _WX_GENERIC_MASKG_H_

