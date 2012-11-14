/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/bitmap.h
// Purpose:     wxBitmap class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: bitmap.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_BITMAP_H_
#define _WX_BITMAP_H_

#include "wx/msw/gdiimage.h"
#include "wx/palette.h"

class WXDLLIMPEXP_FWD_CORE wxBitmap;
class WXDLLIMPEXP_FWD_CORE wxBitmapHandler;
class WXDLLIMPEXP_FWD_CORE wxBitmapRefData;
class WXDLLIMPEXP_FWD_CORE wxControl;
class WXDLLIMPEXP_FWD_CORE wxCursor;
class WXDLLIMPEXP_FWD_CORE wxDC;
#if wxUSE_WXDIB
class WXDLLIMPEXP_FWD_CORE wxDIB;
#endif
class WXDLLIMPEXP_FWD_CORE wxIcon;
class WXDLLIMPEXP_FWD_CORE wxImage;
class WXDLLIMPEXP_FWD_CORE wxMask;
class WXDLLIMPEXP_FWD_CORE wxPalette;
class WXDLLIMPEXP_FWD_CORE wxPixelDataBase;

// ----------------------------------------------------------------------------
// wxBitmap: a mono or colour bitmap
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxBitmap : public wxGDIImage
{
public:
    // default ctor creates an invalid bitmap, you must Create() it later
    wxBitmap() { }

    // Initialize with raw data
    wxBitmap(const char bits[], int width, int height, int depth = 1);

    // Initialize with XPM data
    wxBitmap(const char* const* data);
#ifdef wxNEEDS_CHARPP
    wxBitmap(char** data)
    {
        *this = wxBitmap(wx_const_cast(const char* const*, data));
    }
#endif

    // Load a file or resource
    wxBitmap(const wxString& name, wxBitmapType type = wxBITMAP_TYPE_BMP_RESOURCE);

    // New constructor for generalised creation from data
    wxBitmap(const void* data, long type, int width, int height, int depth = 1);

    // Create a new, uninitialized bitmap of the given size and depth (if it
    // is omitted, will create a bitmap compatible with the display)
    //
    // NB: this ctor will create a DIB for 24 and 32bpp bitmaps, use ctor
    //     taking a DC argument if you want to force using DDB in this case
    wxBitmap(int width, int height, int depth = -1);

    // Create a bitmap compatible with the given DC
    wxBitmap(int width, int height, const wxDC& dc);

#if wxUSE_IMAGE
    // Convert from wxImage
    wxBitmap(const wxImage& image, int depth = -1)
        { (void)CreateFromImage(image, depth); }

    // Create a DDB compatible with the given DC from wxImage
    wxBitmap(const wxImage& image, const wxDC& dc)
        { (void)CreateFromImage(image, dc); }
#endif // wxUSE_IMAGE

    // we must have this, otherwise icons are silently copied into bitmaps using
    // the copy ctor but the resulting bitmap is invalid!
    wxBitmap(const wxIcon& icon) { CopyFromIcon(icon); }

    wxBitmap& operator=(const wxIcon& icon)
    {
        (void)CopyFromIcon(icon);

        return *this;
    }

    wxBitmap& operator=(const wxCursor& cursor)
    {
        (void)CopyFromCursor(cursor);

        return *this;
    }

    virtual ~wxBitmap();

#if wxUSE_IMAGE
    wxImage ConvertToImage() const;
#endif // wxUSE_IMAGE

    // get the given part of bitmap
    wxBitmap GetSubBitmap( const wxRect& rect ) const;

    // NB: This should not be called from user code. It is for wx internal
    // use only. 
    wxBitmap GetSubBitmapOfHDC( const wxRect& rect, WXHDC hdc ) const;

    // copies the contents and mask of the given (colour) icon to the bitmap
    bool CopyFromIcon(const wxIcon& icon);

    // copies the contents and mask of the given cursor to the bitmap
    bool CopyFromCursor(const wxCursor& cursor);

#if wxUSE_WXDIB
    // copies from a device independent bitmap
    bool CopyFromDIB(const wxDIB& dib);
#endif

    virtual bool Create(int width, int height, int depth = -1);
    virtual bool Create(int width, int height, const wxDC& dc);
    virtual bool Create(const void* data, long type, int width, int height, int depth = 1);
    virtual bool LoadFile(const wxString& name, long type = wxBITMAP_TYPE_BMP_RESOURCE);
    virtual bool SaveFile(const wxString& name, int type, const wxPalette *cmap = NULL);

    wxBitmapRefData *GetBitmapData() const
        { return (wxBitmapRefData *)m_refData; }

    // raw bitmap access support functions
    void *GetRawData(wxPixelDataBase& data, int bpp);
    void UngetRawData(wxPixelDataBase& data);

#if wxUSE_PALETTE
    wxPalette* GetPalette() const;
    void SetPalette(const wxPalette& palette);
#endif // wxUSE_PALETTE

    wxMask *GetMask() const;
    wxBitmap GetMaskBitmap() const;
    void SetMask(wxMask *mask);

    // these functions are internal and shouldn't be used, they risk to
    // disappear in the future
    bool HasAlpha() const;
    void UseAlpha();

#if WXWIN_COMPATIBILITY_2_4
    // these functions do nothing and are only there for backwards
    // compatibility
    wxDEPRECATED( int GetQuality() const );
    wxDEPRECATED( void SetQuality(int quality) );
#endif // WXWIN_COMPATIBILITY_2_4

    // implementation only from now on
    // -------------------------------

public:
    void SetHBITMAP(WXHBITMAP bmp) { SetHandle((WXHANDLE)bmp); }
    WXHBITMAP GetHBITMAP() const { return (WXHBITMAP)GetHandle(); }

#ifdef __WXDEBUG__
    void SetSelectedInto(wxDC *dc);
    wxDC *GetSelectedInto() const;
#endif // __WXDEBUG__

protected:
    virtual wxGDIImageRefData *CreateData() const;
    virtual wxObjectRefData *CloneRefData(const wxObjectRefData *data) const;

    // creates an uninitialized bitmap, called from Create()s above
    bool DoCreate(int w, int h, int depth, WXHDC hdc);

#if wxUSE_IMAGE
    // creates the bitmap from wxImage, supposed to be called from ctor
    bool CreateFromImage(const wxImage& image, int depth);

    // creates a DDB from wxImage, supposed to be called from ctor
    bool CreateFromImage(const wxImage& image, const wxDC& dc);

    // common part of the 2 methods above (hdc may be 0)
    bool CreateFromImage(const wxImage& image, int depth, WXHDC hdc);
#endif // wxUSE_IMAGE

private:
    // common part of CopyFromIcon/CopyFromCursor for Win32
    bool CopyFromIconOrCursor(const wxGDIImage& icon);


    DECLARE_DYNAMIC_CLASS(wxBitmap)
};

// ----------------------------------------------------------------------------
// wxMask: a mono bitmap used for drawing bitmaps transparently.
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxMask : public wxObject
{
public:
    wxMask();

    // Copy constructor
    wxMask(const wxMask &mask);

    // Construct a mask from a bitmap and a colour indicating the transparent
    // area
    wxMask(const wxBitmap& bitmap, const wxColour& colour);

    // Construct a mask from a bitmap and a palette index indicating the
    // transparent area
    wxMask(const wxBitmap& bitmap, int paletteIndex);

    // Construct a mask from a mono bitmap (copies the bitmap).
    wxMask(const wxBitmap& bitmap);

    // construct a mask from the givne bitmap handle
    wxMask(WXHBITMAP hbmp) { m_maskBitmap = hbmp; }

    virtual ~wxMask();

    bool Create(const wxBitmap& bitmap, const wxColour& colour);
    bool Create(const wxBitmap& bitmap, int paletteIndex);
    bool Create(const wxBitmap& bitmap);

    // Implementation
    WXHBITMAP GetMaskBitmap() const { return m_maskBitmap; }
    void SetMaskBitmap(WXHBITMAP bmp) { m_maskBitmap = bmp; }

protected:
    WXHBITMAP m_maskBitmap;

    DECLARE_DYNAMIC_CLASS(wxMask)
};

// ----------------------------------------------------------------------------
// wxBitmapHandler is a class which knows how to load/save bitmaps to/from file
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxBitmapHandler : public wxGDIImageHandler
{
public:
    wxBitmapHandler() { }
    wxBitmapHandler(const wxString& name, const wxString& ext, long type)
        : wxGDIImageHandler(name, ext, type)
    {
    }

    // keep wxBitmapHandler derived from wxGDIImageHandler compatible with the
    // old class which worked only with bitmaps
    virtual bool Create(wxBitmap *bitmap,
                        const void* data,
                        long flags,
                        int width, int height, int depth = 1);
    virtual bool LoadFile(wxBitmap *bitmap,
                          const wxString& name,
                          long flags,
                          int desiredWidth, int desiredHeight);
    virtual bool SaveFile(wxBitmap *bitmap,
                          const wxString& name,
                          int type,
                          const wxPalette *palette = NULL);

    virtual bool Create(wxGDIImage *image,
                        const void* data,
                        long flags,
                        int width, int height, int depth = 1);
    virtual bool Load(wxGDIImage *image,
                      const wxString& name,
                      long flags,
                      int desiredWidth, int desiredHeight);
    virtual bool Save(wxGDIImage *image,
                      const wxString& name,
                      int type);

private:
    DECLARE_DYNAMIC_CLASS(wxBitmapHandler)
};

#endif
  // _WX_BITMAP_H_
