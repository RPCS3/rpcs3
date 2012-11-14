/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/bmpbase.cpp
// Purpose:     wxBitmapBase
// Author:      VaclavSlavik
// Created:     2001/04/11
// RCS-ID:      $Id: bmpbase.cpp 42752 2006-10-30 19:26:48Z VZ $
// Copyright:   (c) 2001, Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/bitmap.h"

#ifndef WX_PRECOMP
    #include "wx/colour.h"
    #include "wx/icon.h"
    #include "wx/image.h"
#endif // WX_PRECOMP

// ----------------------------------------------------------------------------
// wxVariant support
// ----------------------------------------------------------------------------

#if wxUSE_VARIANT
IMPLEMENT_VARIANT_OBJECT_EXPORTED_SHALLOWCMP(wxBitmap,WXDLLEXPORT)
IMPLEMENT_VARIANT_OBJECT_EXPORTED_SHALLOWCMP(wxIcon,WXDLLEXPORT)
#endif

// ----------------------------------------------------------------------------
// wxBitmapBase
// ----------------------------------------------------------------------------

#if wxUSE_BITMAP_BASE

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/utils.h"
    #include "wx/palette.h"
    #include "wx/module.h"
#endif // WX_PRECOMP


IMPLEMENT_ABSTRACT_CLASS(wxBitmapBase, wxGDIObject)
IMPLEMENT_ABSTRACT_CLASS(wxBitmapHandlerBase,wxObject)

wxList wxBitmapBase::sm_handlers;

void wxBitmapBase::AddHandler(wxBitmapHandlerBase *handler)
{
    sm_handlers.Append(handler);
}

void wxBitmapBase::InsertHandler(wxBitmapHandlerBase *handler)
{
    sm_handlers.Insert(handler);
}

bool wxBitmapBase::RemoveHandler(const wxString& name)
{
    wxBitmapHandler *handler = FindHandler(name);
    if ( handler )
    {
        sm_handlers.DeleteObject(handler);
        return true;
    }
    else
        return false;
}

wxBitmapHandler *wxBitmapBase::FindHandler(const wxString& name)
{
    wxList::compatibility_iterator node = sm_handlers.GetFirst();
    while ( node )
    {
        wxBitmapHandler *handler = (wxBitmapHandler *)node->GetData();
        if ( handler->GetName() == name )
            return handler;
        node = node->GetNext();
    }
    return NULL;
}

wxBitmapHandler *wxBitmapBase::FindHandler(const wxString& extension, wxBitmapType bitmapType)
{
    wxList::compatibility_iterator node = sm_handlers.GetFirst();
    while ( node )
    {
        wxBitmapHandler *handler = (wxBitmapHandler *)node->GetData();
        if ( handler->GetExtension() == extension &&
                    (bitmapType == wxBITMAP_TYPE_ANY || handler->GetType() == bitmapType) )
            return handler;
        node = node->GetNext();
    }
    return NULL;
}

wxBitmapHandler *wxBitmapBase::FindHandler(wxBitmapType bitmapType)
{
    wxList::compatibility_iterator node = sm_handlers.GetFirst();
    while ( node )
    {
        wxBitmapHandler *handler = (wxBitmapHandler *)node->GetData();
        if (handler->GetType() == bitmapType)
            return handler;
        node = node->GetNext();
    }
    return NULL;
}

void wxBitmapBase::CleanUpHandlers()
{
    wxList::compatibility_iterator node = sm_handlers.GetFirst();
    while ( node )
    {
        wxBitmapHandler *handler = (wxBitmapHandler *)node->GetData();
        wxList::compatibility_iterator next = node->GetNext();
        delete handler;
        sm_handlers.Erase(node);
        node = next;
    }
}

bool wxBitmapHandlerBase::Create(wxBitmap*, const void*, long, int, int, int)
{
    return false;
}

bool wxBitmapHandlerBase::LoadFile(wxBitmap*, const wxString&, long, int, int)
{
    return false;
}

bool wxBitmapHandlerBase::SaveFile(const wxBitmap*, const wxString&, int, const wxPalette*)
{
    return false;
}

class wxBitmapBaseModule: public wxModule
{
DECLARE_DYNAMIC_CLASS(wxBitmapBaseModule)
public:
    wxBitmapBaseModule() {}
    bool OnInit() { wxBitmap::InitStandardHandlers(); return true; }
    void OnExit() { wxBitmap::CleanUpHandlers(); }
};

IMPLEMENT_DYNAMIC_CLASS(wxBitmapBaseModule, wxModule)

#endif // wxUSE_BITMAP_BASE

// ----------------------------------------------------------------------------
// wxBitmap common
// ----------------------------------------------------------------------------

#if !(defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXX11__))

wxBitmap::wxBitmap(const char* const* bits)
{
    wxCHECK2_MSG(bits != NULL, return, wxT("invalid bitmap data"));

#if wxUSE_IMAGE && wxUSE_XPM
    wxImage image(bits);
    wxCHECK2_MSG(image.Ok(), return, wxT("invalid bitmap data"));

    *this = wxBitmap(image);
#else
    wxFAIL_MSG(_T("creating bitmaps from XPMs not supported"));
#endif // wxUSE_IMAGE && wxUSE_XPM
}
#endif // !(defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXX11__))

// ----------------------------------------------------------------------------
// wxMaskBase
// ----------------------------------------------------------------------------

bool wxMaskBase::Create(const wxBitmap& bitmap, const wxColour& colour)
{
    FreeData();

    return InitFromColour(bitmap, colour);
}

#if wxUSE_PALETTE

bool wxMaskBase::Create(const wxBitmap& bitmap, int paletteIndex)
{
    wxPalette *pal = bitmap.GetPalette();

    wxCHECK_MSG( pal, false,
                 wxT("Cannot create mask from palette index of a bitmap without palette") );

    unsigned char r,g,b;
    pal->GetRGB(paletteIndex, &r, &g, &b);

    return Create(bitmap, wxColour(r, g, b));
}

#endif // wxUSE_PALETTE

bool wxMaskBase::Create(const wxBitmap& bitmap)
{
    FreeData();

    return InitFromMonoBitmap(bitmap);
}
