/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/imaglist.cpp
// Purpose:     wxImageList implementation for Win32
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: imaglist.cpp 43786 2006-12-04 02:09:59Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/msw/wrapcctl.h" // include <commctrl.h> "properly"
    #include "wx/window.h"
    #include "wx/icon.h"
    #include "wx/dc.h"
    #include "wx/string.h"
    #include "wx/dcmemory.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/image.h"
    #include <stdio.h>
#endif

#include "wx/imaglist.h"
#include "wx/msw/private.h"

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxImageList, wxObject)

#define GetHImageList()     ((HIMAGELIST)m_hImageList)

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// returns the mask if it's valid, otherwise the bitmap mask and, if it's not
// valid neither, a "solid" mask (no transparent zones at all)
static HBITMAP GetMaskForImage(const wxBitmap& bitmap, const wxBitmap& mask);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxImageList creation/destruction
// ----------------------------------------------------------------------------

wxImageList::wxImageList()
{
    m_hImageList = 0;
}

// Creates an image list
bool wxImageList::Create(int width, int height, bool mask, int initial)
{
    UINT flags = 0;

    // set appropriate color depth
#ifdef __WXWINCE__
    flags |= ILC_COLOR;
#else
    int dd = wxDisplayDepth();

    if (dd <= 4)       flags |= ILC_COLOR;   // 16 color
    else if (dd <= 8)  flags |= ILC_COLOR8;  // 256 color
    else if (dd <= 16) flags |= ILC_COLOR16; // 64k hi-color
    else if (dd <= 24) flags |= ILC_COLOR24; // 16m truecolor
    else if (dd <= 32) flags |= ILC_COLOR32; // 16m truecolor
#endif

    if ( mask )
        flags |= ILC_MASK;

    // Grow by 1, I guess this is reasonable behaviour most of the time
    m_hImageList = (WXHIMAGELIST) ImageList_Create(width, height, flags,
                                                   initial, 1);
    if ( !m_hImageList )
    {
        wxLogLastError(wxT("ImageList_Create()"));
    }

    return m_hImageList != 0;
}

wxImageList::~wxImageList()
{
    if ( m_hImageList )
    {
        ImageList_Destroy(GetHImageList());
        m_hImageList = 0;
    }
}

// ----------------------------------------------------------------------------
// wxImageList attributes
// ----------------------------------------------------------------------------

// Returns the number of images in the image list.
int wxImageList::GetImageCount() const
{
    wxASSERT_MSG( m_hImageList, _T("invalid image list") );

    return ImageList_GetImageCount(GetHImageList());
}

// Returns the size (same for all images) of the images in the list
bool wxImageList::GetSize(int WXUNUSED(index), int &width, int &height) const
{
    wxASSERT_MSG( m_hImageList, _T("invalid image list") );

    return ImageList_GetIconSize(GetHImageList(), &width, &height) != 0;
}

// ----------------------------------------------------------------------------
// wxImageList operations
// ----------------------------------------------------------------------------

// Adds a bitmap, and optionally a mask bitmap.
// Note that wxImageList creates new bitmaps, so you may delete
// 'bitmap' and 'mask'.
int wxImageList::Add(const wxBitmap& bitmap, const wxBitmap& mask)
{
    HBITMAP hbmpMask = GetMaskForImage(bitmap, mask);

    int index = ImageList_Add(GetHImageList(), GetHbitmapOf(bitmap), hbmpMask);
    if ( index == -1 )
    {
        wxLogError(_("Couldn't add an image to the image list."));
    }

    ::DeleteObject(hbmpMask);

    return index;
}

// Adds a bitmap, using the specified colour to create the mask bitmap
// Note that wxImageList creates new bitmaps, so you may delete
// 'bitmap'.
int wxImageList::Add(const wxBitmap& bitmap, const wxColour& maskColour)
{
    int index = ImageList_AddMasked(GetHImageList(),
                                    GetHbitmapOf(bitmap),
                                    wxColourToRGB(maskColour));
    if ( index == -1 )
    {
        wxLogError(_("Couldn't add an image to the image list."));
    }

    return index;
}

// Adds a bitmap and mask from an icon.
int wxImageList::Add(const wxIcon& icon)
{
    int index = ImageList_AddIcon(GetHImageList(), GetHiconOf(icon));
    if ( index == -1 )
    {
        wxLogError(_("Couldn't add an image to the image list."));
    }

    return index;
}

// Replaces a bitmap, optionally passing a mask bitmap.
// Note that wxImageList creates new bitmaps, so you may delete
// 'bitmap' and 'mask'.
bool wxImageList::Replace(int index,
                          const wxBitmap& bitmap, const wxBitmap& mask)
{
    HBITMAP hbmpMask = GetMaskForImage(bitmap, mask);

    bool ok = ImageList_Replace(GetHImageList(), index,
                                GetHbitmapOf(bitmap), hbmpMask) != 0;
    if ( !ok )
    {
        wxLogLastError(wxT("ImageList_Replace()"));
    }

    ::DeleteObject(hbmpMask);

    return ok;
}

// Replaces a bitmap and mask from an icon.
bool wxImageList::Replace(int i, const wxIcon& icon)
{
    bool ok = ImageList_ReplaceIcon(GetHImageList(), i, GetHiconOf(icon)) != -1;
    if ( !ok )
    {
        wxLogLastError(wxT("ImageList_ReplaceIcon()"));
    }

    return ok;
}

// Removes the image at the given index.
bool wxImageList::Remove(int index)
{
    bool ok = ImageList_Remove(GetHImageList(), index) != 0;
    if ( !ok )
    {
        wxLogLastError(wxT("ImageList_Remove()"));
    }

    return ok;
}

// Remove all images
bool wxImageList::RemoveAll()
{
    // don't use ImageList_RemoveAll() because mingw32 headers don't have it
    return Remove(-1);
}

// Draws the given image on a dc at the specified position.
// If 'solidBackground' is true, Draw sets the image list background
// colour to the background colour of the wxDC, to speed up
// drawing by eliminating masked drawing where possible.
bool wxImageList::Draw(int index,
                       wxDC& dc,
                       int x, int y,
                       int flags,
                       bool solidBackground)
{
    HDC hDC = GetHdcOf(dc);
    wxCHECK_MSG( hDC, false, _T("invalid wxDC in wxImageList::Draw") );

    COLORREF clr = CLR_NONE;    // transparent by default
    if ( solidBackground )
    {
        const wxBrush& brush = dc.GetBackground();
        if ( brush.Ok() )
        {
            clr = wxColourToRGB(brush.GetColour());
        }
    }

    ImageList_SetBkColor(GetHImageList(), clr);

    UINT style = 0;
    if ( flags & wxIMAGELIST_DRAW_NORMAL )
        style |= ILD_NORMAL;
    if ( flags & wxIMAGELIST_DRAW_TRANSPARENT )
        style |= ILD_TRANSPARENT;
    if ( flags & wxIMAGELIST_DRAW_SELECTED )
        style |= ILD_SELECTED;
    if ( flags & wxIMAGELIST_DRAW_FOCUSED )
        style |= ILD_FOCUS;

    bool ok = ImageList_Draw(GetHImageList(), index, hDC, x, y, style) != 0;
    if ( !ok )
    {
        wxLogLastError(wxT("ImageList_Draw()"));
    }

    return ok;
}

// Get the bitmap
wxBitmap wxImageList::GetBitmap(int index) const
{
#if wxUSE_WXDIB
    int bmp_width = 0, bmp_height = 0;
    GetSize(index, bmp_width, bmp_height);

    wxBitmap bitmap(bmp_width, bmp_height);
    wxMemoryDC dc;
    dc.SelectObject(bitmap);

    // draw it the first time to find a suitable mask colour
    ((wxImageList*)this)->Draw(index, dc, 0, 0, wxIMAGELIST_DRAW_TRANSPARENT);
    dc.SelectObject(wxNullBitmap);

    // find the suitable mask colour
    wxImage image = bitmap.ConvertToImage();
    unsigned char r = 0, g = 0, b = 0;
    image.FindFirstUnusedColour(&r, &g, &b);

    // redraw whole image and bitmap in the mask colour
    image.Create(bmp_width, bmp_height);
    image.Replace(0, 0, 0, r, g, b);
    bitmap = wxBitmap(image);

    // redraw icon over the mask colour to actually draw it
    dc.SelectObject(bitmap);
    ((wxImageList*)this)->Draw(index, dc, 0, 0, wxIMAGELIST_DRAW_TRANSPARENT);
    dc.SelectObject(wxNullBitmap);

    // get the image, set the mask colour and convert back to get transparent bitmap
    image = bitmap.ConvertToImage();
    image.SetMaskColour(r, g, b);
    bitmap = wxBitmap(image);
#else
    wxBitmap bitmap;
#endif
    return bitmap;
}

// Get the icon
wxIcon wxImageList::GetIcon(int index) const
{
    HICON hIcon = ImageList_ExtractIcon(0, GetHImageList(), index);
    if (hIcon)
    {
        wxIcon icon;
        icon.SetHICON((WXHICON)hIcon);

        int iconW, iconH;
        GetSize(index, iconW, iconH);
        icon.SetSize(iconW, iconH);

        return icon;
    }
    else
        return wxNullIcon;
}

// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------

static HBITMAP GetMaskForImage(const wxBitmap& bitmap, const wxBitmap& mask)
{
#if wxUSE_IMAGE
    wxBitmap bitmapWithMask;
#endif // wxUSE_IMAGE

    HBITMAP hbmpMask;
    wxMask *pMask;
    bool deleteMask = false;

    if ( mask.Ok() )
    {
        hbmpMask = GetHbitmapOf(mask);
        pMask = NULL;
    }
    else
    {
        pMask = bitmap.GetMask();

#if wxUSE_IMAGE
        // check if we don't have alpha in this bitmap -- we can create a mask
        // from it (and we need to do it for the older systems which don't
        // support 32bpp bitmaps natively)
        if ( !pMask )
        {
            wxImage img(bitmap.ConvertToImage());
            if ( img.HasAlpha() )
            {
                img.ConvertAlphaToMask();
                bitmapWithMask = wxBitmap(img);
                pMask = bitmapWithMask.GetMask();
            }
        }
#endif // wxUSE_IMAGE

        if ( !pMask )
        {
            // use the light grey count as transparent: the trouble here is
            // that the light grey might have been changed by Windows behind
            // our back, so use the standard colour map to get its real value
            wxCOLORMAP *cmap = wxGetStdColourMap();
            wxColour col;
            wxRGBToColour(col, cmap[wxSTD_COL_BTNFACE].from);

            pMask = new wxMask(bitmap, col);

            deleteMask = true;
        }

        hbmpMask = (HBITMAP)pMask->GetMaskBitmap();
    }

    // windows mask convention is opposite to the wxWidgets one
    HBITMAP hbmpMaskInv = wxInvertMask(hbmpMask);

    if ( deleteMask )
    {
        delete pMask;
    }

    return hbmpMaskInv;
}
