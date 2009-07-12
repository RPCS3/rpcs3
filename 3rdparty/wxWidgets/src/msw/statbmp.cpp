/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/statbmp.cpp
// Purpose:     wxStaticBitmap
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: statbmp.cpp 56532 2008-10-27 18:15:19Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// declarations
// ===========================================================================

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_STATBMP

#include "wx/statbmp.h"

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/window.h"
    #include "wx/icon.h"
    #include "wx/dcclient.h"
#endif

#include "wx/msw/private.h"

#include "wx/sysopt.h"

#include <stdio.h>

// ---------------------------------------------------------------------------
// macors
// ---------------------------------------------------------------------------

#if wxUSE_EXTENDED_RTTI
WX_DEFINE_FLAGS( wxStaticBitmapStyle )

wxBEGIN_FLAGS( wxStaticBitmapStyle )
    // new style border flags, we put them first to
    // use them for streaming out
    wxFLAGS_MEMBER(wxBORDER_SIMPLE)
    wxFLAGS_MEMBER(wxBORDER_SUNKEN)
    wxFLAGS_MEMBER(wxBORDER_DOUBLE)
    wxFLAGS_MEMBER(wxBORDER_RAISED)
    wxFLAGS_MEMBER(wxBORDER_STATIC)
    wxFLAGS_MEMBER(wxBORDER_NONE)

    // old style border flags
    wxFLAGS_MEMBER(wxSIMPLE_BORDER)
    wxFLAGS_MEMBER(wxSUNKEN_BORDER)
    wxFLAGS_MEMBER(wxDOUBLE_BORDER)
    wxFLAGS_MEMBER(wxRAISED_BORDER)
    wxFLAGS_MEMBER(wxSTATIC_BORDER)
    wxFLAGS_MEMBER(wxBORDER)

    // standard window styles
    wxFLAGS_MEMBER(wxTAB_TRAVERSAL)
    wxFLAGS_MEMBER(wxCLIP_CHILDREN)
    wxFLAGS_MEMBER(wxTRANSPARENT_WINDOW)
    wxFLAGS_MEMBER(wxWANTS_CHARS)
    wxFLAGS_MEMBER(wxFULL_REPAINT_ON_RESIZE)
    wxFLAGS_MEMBER(wxALWAYS_SHOW_SB )
    wxFLAGS_MEMBER(wxVSCROLL)
    wxFLAGS_MEMBER(wxHSCROLL)

wxEND_FLAGS( wxStaticBitmapStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxStaticBitmap, wxControl,"wx/statbmp.h")

wxBEGIN_PROPERTIES_TABLE(wxStaticBitmap)
    wxPROPERTY_FLAGS( WindowStyle , wxStaticBitmapStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , EMPTY_MACROVALUE, 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxStaticBitmap)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_5( wxStaticBitmap, wxWindow* , Parent , wxWindowID , Id , wxBitmap, Bitmap, wxPoint , Position , wxSize , Size )

#else
IMPLEMENT_DYNAMIC_CLASS(wxStaticBitmap, wxControl)
#endif

/*
    TODO PROPERTIES :
        bitmap
*/

// ===========================================================================
// implementation
// ===========================================================================

// ---------------------------------------------------------------------------
// wxStaticBitmap
// ---------------------------------------------------------------------------

// we may have either bitmap or icon: if a bitmap with mask is passed, we
// will transform it to an icon ourselves because otherwise the mask will
// be ignored by Windows
// note that this function will create a new object every time
// it is called even if the image needs no conversion

static wxGDIImage* ConvertImage( const wxGDIImage& bitmap )
{
    bool isIcon = bitmap.IsKindOf( CLASSINFO(wxIcon) );

    if( !isIcon )
    {
        wxASSERT_MSG( wxDynamicCast(&bitmap, wxBitmap),
                      _T("not an icon and not a bitmap?") );

        const wxBitmap& bmp = (const wxBitmap&)bitmap;
        wxMask *mask = bmp.GetMask();
        if ( mask && mask->GetMaskBitmap() )
        {
            wxIcon* icon = new wxIcon;
            icon->CopyFromBitmap(bmp);

            return icon;
        }

        return new wxBitmap( bmp );
    }

    // copying a bitmap is a cheap operation
    return new wxIcon( (const wxIcon&)bitmap );
}

bool wxStaticBitmap::Create(wxWindow *parent,
                            wxWindowID id,
                            const wxGDIImage& bitmap,
                            const wxPoint& pos,
                            const wxSize& size,
                            long style,
                            const wxString& name)
{
    if ( !CreateControl(parent, id, pos, size, style, wxDefaultValidator, name) )
        return false;

    // we may have either bitmap or icon: if a bitmap with mask is passed, we
    // will transform it to an icon ourselves because otherwise the mask will
    // be ignored by Windows
    m_isIcon = bitmap.IsKindOf(CLASSINFO(wxIcon));

    wxGDIImage *image = ConvertImage( bitmap );
    m_isIcon = image->IsKindOf( CLASSINFO(wxIcon) );

    // create the native control
    if ( !MSWCreateControl(_T("STATIC"), wxEmptyString, pos, size) )
    {
        // control creation failed
        return false;
    }

    // no need to delete the new image
    SetImageNoCopy(image);

    // GetBestSize will work properly now, so set the best size if needed
    SetInitialSize(size);

    // painting manually is reported not to work under Windows CE (see #10093),
    // so don't do it there even if this probably means that alpha is not
    // supported there -- but at least bitmaps without alpha appear correctly
#ifndef __WXWINCE__
    // Windows versions before XP (and even XP if the application has no
    // manifest and so the old comctl32.dll is used) don't draw correctly the
    // images with alpha channel so we need to draw them ourselves and it's
    // easier to just always do it rather than check if we have an image with
    // alpha or not
    if ( wxTheApp->GetComCtl32Version() < 600 )
    {
        Connect(wxEVT_PAINT, wxPaintEventHandler(wxStaticBitmap::DoPaintManually));
    }
#endif // !__WXWINCE__

    return true;
}

wxBorder wxStaticBitmap::GetDefaultBorder() const
{
    return wxBORDER_NONE;
}

WXDWORD wxStaticBitmap::MSWGetStyle(long style, WXDWORD *exstyle) const
{
    WXDWORD msStyle = wxControl::MSWGetStyle(style, exstyle);

    // what kind of control are we?
    msStyle |= m_isIcon ? SS_ICON : SS_BITMAP;

    // we use SS_CENTERIMAGE to prevent the control from resizing the bitmap to
    // fit to its size -- this is unexpected and doesn't happen in other ports
    //
    // and SS_NOTIFY is necessary to receive mouse events
    msStyle |= SS_CENTERIMAGE | SS_NOTIFY;

    return msStyle;
}

bool wxStaticBitmap::ImageIsOk() const
{
    return m_image && m_image->Ok();
}

wxIcon wxStaticBitmap::GetIcon() const
{
    wxCHECK_MSG( m_image, wxIcon(), _T("no image in wxStaticBitmap") );

    // we can't ask for an icon if all we have is a bitmap
    wxCHECK_MSG( m_isIcon, wxIcon(), _T("no icon in this wxStaticBitmap") );

    return *(wxIcon *)m_image;
}

wxBitmap wxStaticBitmap::GetBitmap() const
{
    if ( m_isIcon )
    {
        // don't fail because we might have replaced the bitmap with icon
        // ourselves internally in ConvertImage() to keep the transparency but
        // the user code doesn't know about it so it still can use GetBitmap()
        // to retrieve the bitmap
        return wxBitmap(GetIcon());
    }
    else // we have a bitmap
    {
        wxCHECK_MSG( m_image, wxBitmap(), _T("no image in wxStaticBitmap") );

        return *(wxBitmap *)m_image;
    }
}

void wxStaticBitmap::Free()
{
    delete m_image;

    m_image = NULL;
}

wxSize wxStaticBitmap::DoGetBestSize() const
{
    if ( ImageIsOk() )
    {
        wxSize best(m_image->GetWidth(), m_image->GetHeight());
        CacheBestSize(best);
        return best;
    }

    // this is completely arbitrary
    return wxSize(16, 16);
}

void wxStaticBitmap::DoPaintManually(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);

    const wxSize size(GetSize());
    const wxBitmap bmp(GetBitmap());

    // Clear the background: notice that we're supposed to be transparent, so
    // use the parent background colour if we don't have our own instead of
    // falling back to the default
    const wxWindow *win = UseBgCol() ? this : GetParent();
    dc.SetBrush(win->GetBackgroundColour());
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());

    // Draw the image in the middle
    dc.DrawBitmap(bmp,
                  (size.GetWidth() - bmp.GetWidth()) / 2,
                  (size.GetHeight() - bmp.GetHeight()) / 2,
                  true /* use mask */);
}

void wxStaticBitmap::SetImage( const wxGDIImage* image )
{
    wxGDIImage* convertedImage = ConvertImage( *image );
    SetImageNoCopy( convertedImage );
    InvalidateBestSize();
}

void wxStaticBitmap::SetImageNoCopy( wxGDIImage* image)
{
    Free();

    m_isIcon = image->IsKindOf( CLASSINFO(wxIcon) );
    // the image has already been copied
    m_image = image;

    int x, y;
    int w, h;
    GetPosition(&x, &y);
    GetSize(&w, &h);

#ifdef __WIN32__
    HANDLE handle = (HANDLE)m_image->GetHandle();
    LONG style = ::GetWindowLong( (HWND)GetHWND(), GWL_STYLE ) ;
    ::SetWindowLong( (HWND)GetHWND(), GWL_STYLE, ( style & ~( SS_BITMAP|SS_ICON ) ) |
                     ( m_isIcon ? SS_ICON : SS_BITMAP ) );
    HGDIOBJ oldHandle = (HGDIOBJ)::SendMessage(GetHwnd(), STM_SETIMAGE,
                  m_isIcon ? IMAGE_ICON : IMAGE_BITMAP, (LPARAM)handle);
    // detect if this is still the handle we passed before or
    // if the static-control made a copy of the bitmap!
    if (m_currentHandle != 0 && oldHandle != (HGDIOBJ) m_currentHandle)
    {
        // the static control made a copy and we are responsible for deleting it
        DeleteObject((HGDIOBJ) oldHandle);
    }
    m_currentHandle = (WXHANDLE)handle;
#endif // Win32

    if ( ImageIsOk() )
    {
        int width = image->GetWidth(),
            height = image->GetHeight();
        if ( width && height )
        {
            w = width;
            h = height;

            ::MoveWindow(GetHwnd(), x, y, width, height, FALSE);
        }
    }

    RECT rect;
    rect.left   = x;
    rect.top    = y;
    rect.right  = x + w;
    rect.bottom = y + h;
    ::InvalidateRect(GetHwndOf(GetParent()), &rect, TRUE);
}

#endif // wxUSE_STATBMP
