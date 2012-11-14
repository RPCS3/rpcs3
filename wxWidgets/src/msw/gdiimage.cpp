///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/gdiimage.cpp
// Purpose:     wxGDIImage implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     20.11.99
// RCS-ID:      $Id: gdiimage.cpp 41689 2006-10-08 08:04:49Z PC $
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

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
    #include "wx/string.h"
    #include "wx/log.h"
    #include "wx/app.h"
    #include "wx/bitmap.h"
#endif // WX_PRECOMP

#include "wx/msw/private.h"

#include "wx/msw/gdiimage.h"

#if wxUSE_WXDIB
#include "wx/msw/dib.h"
#endif

#ifdef __WXWINCE__
#include <winreg.h>
#include <shellapi.h>
#endif

#include "wx/file.h"

#include "wx/listimpl.cpp"
WX_DEFINE_LIST(wxGDIImageHandlerList)

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

#ifndef __WXMICROWIN__

// all image handlers are declared/defined in this file because the outside
// world doesn't have to know about them (but only about wxBITMAP_TYPE_XXX ids)

class WXDLLEXPORT wxBMPFileHandler : public wxBitmapHandler
{
public:
    wxBMPFileHandler() : wxBitmapHandler(_T("Windows bitmap file"), _T("bmp"),
                                         wxBITMAP_TYPE_BMP)
    {
    }

    virtual bool LoadFile(wxBitmap *bitmap,
                          const wxString& name, long flags,
                          int desiredWidth, int desiredHeight);
    virtual bool SaveFile(wxBitmap *bitmap,
                          const wxString& name, int type,
                          const wxPalette *palette = NULL);

private:
    DECLARE_DYNAMIC_CLASS(wxBMPFileHandler)
};

class WXDLLEXPORT wxBMPResourceHandler: public wxBitmapHandler
{
public:
    wxBMPResourceHandler() : wxBitmapHandler(_T("Windows bitmap resource"),
                                             wxEmptyString,
                                             wxBITMAP_TYPE_BMP_RESOURCE)
    {
    }

    virtual bool LoadFile(wxBitmap *bitmap,
                          const wxString& name, long flags,
                          int desiredWidth, int desiredHeight);

private:
    DECLARE_DYNAMIC_CLASS(wxBMPResourceHandler)
};

class WXDLLEXPORT wxIconHandler : public wxGDIImageHandler
{
public:
    wxIconHandler(const wxString& name, const wxString& ext, long type)
        : wxGDIImageHandler(name, ext, type)
    {
    }

    // creating and saving icons is not supported
    virtual bool Create(wxGDIImage *WXUNUSED(image),
                        const void* WXUNUSED(data),
                        long WXUNUSED(flags),
                        int WXUNUSED(width),
                        int WXUNUSED(height),
                        int WXUNUSED(depth) = 1)
    {
        return false;
    }

    virtual bool Save(wxGDIImage *WXUNUSED(image),
                      const wxString& WXUNUSED(name),
                      int WXUNUSED(type))
    {
        return false;
    }

    virtual bool Load(wxGDIImage *image,
                      const wxString& name,
                      long flags,
                      int desiredWidth, int desiredHeight)
    {
        wxIcon *icon = wxDynamicCast(image, wxIcon);
        wxCHECK_MSG( icon, false, _T("wxIconHandler only works with icons") );

        return LoadIcon(icon, name, flags, desiredWidth, desiredHeight);
    }

protected:
    virtual bool LoadIcon(wxIcon *icon,
                          const wxString& name, long flags,
                          int desiredWidth = -1, int desiredHeight = -1) = 0;
};

class WXDLLEXPORT wxICOFileHandler : public wxIconHandler
{
public:
    wxICOFileHandler() : wxIconHandler(_T("ICO icon file"),
                                       _T("ico"),
                                       wxBITMAP_TYPE_ICO)
    {
    }

protected:
    virtual bool LoadIcon(wxIcon *icon,
                          const wxString& name, long flags,
                          int desiredWidth = -1, int desiredHeight = -1);

private:
    DECLARE_DYNAMIC_CLASS(wxICOFileHandler)
};

class WXDLLEXPORT wxICOResourceHandler: public wxIconHandler
{
public:
    wxICOResourceHandler() : wxIconHandler(_T("ICO resource"),
                                           _T("ico"),
                                           wxBITMAP_TYPE_ICO_RESOURCE)
    {
    }

protected:
    virtual bool LoadIcon(wxIcon *icon,
                          const wxString& name, long flags,
                          int desiredWidth = -1, int desiredHeight = -1);

private:
    DECLARE_DYNAMIC_CLASS(wxICOResourceHandler)
};

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxBMPFileHandler, wxBitmapHandler)
IMPLEMENT_DYNAMIC_CLASS(wxBMPResourceHandler, wxBitmapHandler)
IMPLEMENT_DYNAMIC_CLASS(wxICOFileHandler, wxObject)
IMPLEMENT_DYNAMIC_CLASS(wxICOResourceHandler, wxObject)

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

#endif
    // __MICROWIN__

// ============================================================================
// implementation
// ============================================================================

wxGDIImageHandlerList wxGDIImage::ms_handlers;

// ----------------------------------------------------------------------------
// wxGDIImage functions forwarded to wxGDIImageRefData
// ----------------------------------------------------------------------------

bool wxGDIImage::FreeResource(bool WXUNUSED(force))
{
    if ( !IsNull() )
    {
        GetGDIImageData()->Free();
        GetGDIImageData()->m_handle = 0;
    }

    return true;
}

WXHANDLE wxGDIImage::GetResourceHandle() const
{
    return GetHandle();
}

// ----------------------------------------------------------------------------
// wxGDIImage handler stuff
// ----------------------------------------------------------------------------

void wxGDIImage::AddHandler(wxGDIImageHandler *handler)
{
    ms_handlers.Append(handler);
}

void wxGDIImage::InsertHandler(wxGDIImageHandler *handler)
{
    ms_handlers.Insert(handler);
}

bool wxGDIImage::RemoveHandler(const wxString& name)
{
    wxGDIImageHandler *handler = FindHandler(name);
    if ( handler )
    {
        ms_handlers.DeleteObject(handler);
        return true;
    }
    else
        return false;
}

wxGDIImageHandler *wxGDIImage::FindHandler(const wxString& name)
{
    wxGDIImageHandlerList::compatibility_iterator node = ms_handlers.GetFirst();
    while ( node )
    {
        wxGDIImageHandler *handler = node->GetData();
        if ( handler->GetName() == name )
            return handler;
        node = node->GetNext();
    }

    return NULL;
}

wxGDIImageHandler *wxGDIImage::FindHandler(const wxString& extension,
                                           long type)
{
    wxGDIImageHandlerList::compatibility_iterator node = ms_handlers.GetFirst();
    while ( node )
    {
        wxGDIImageHandler *handler = node->GetData();
        if ( (handler->GetExtension() == extension) &&
             (type == -1 || handler->GetType() == type) )
        {
            return handler;
        }

        node = node->GetNext();
    }
    return NULL;
}

wxGDIImageHandler *wxGDIImage::FindHandler(long type)
{
    wxGDIImageHandlerList::compatibility_iterator node = ms_handlers.GetFirst();
    while ( node )
    {
        wxGDIImageHandler *handler = node->GetData();
        if ( handler->GetType() == type )
            return handler;

        node = node->GetNext();
    }

    return NULL;
}

void wxGDIImage::CleanUpHandlers()
{
    wxGDIImageHandlerList::compatibility_iterator node = ms_handlers.GetFirst();
    while ( node )
    {
        wxGDIImageHandler *handler = node->GetData();
        wxGDIImageHandlerList::compatibility_iterator next = node->GetNext();
        delete handler;
        ms_handlers.Erase( node );
        node = next;
    }
}

void wxGDIImage::InitStandardHandlers()
{
#ifndef __WXMICROWIN__
    AddHandler(new wxBMPResourceHandler);
    AddHandler(new wxBMPFileHandler);
    AddHandler(new wxICOResourceHandler);
    AddHandler(new wxICOFileHandler);
#endif
}

#ifndef __WXMICROWIN__

// ----------------------------------------------------------------------------
// wxBitmap handlers
// ----------------------------------------------------------------------------

bool wxBMPResourceHandler::LoadFile(wxBitmap *bitmap,
                                    const wxString& name, long WXUNUSED(flags),
                                    int WXUNUSED(desiredWidth),
                                    int WXUNUSED(desiredHeight))
{
    // TODO: load colourmap.
    bitmap->SetHBITMAP((WXHBITMAP)::LoadBitmap(wxGetInstance(), name));

    if ( !bitmap->Ok() )
    {
        // it's probably not found
        wxLogError(wxT("Can't load bitmap '%s' from resources! Check .rc file."),
                   name.c_str());

        return false;
    }

    BITMAP bm;
    if ( !::GetObject(GetHbitmapOf(*bitmap), sizeof(BITMAP), (LPSTR) &bm) )
    {
        wxLogLastError(wxT("GetObject(HBITMAP)"));
    }

    bitmap->SetWidth(bm.bmWidth);
    bitmap->SetHeight(bm.bmHeight);
    bitmap->SetDepth(bm.bmBitsPixel);

    // use 0xc0c0c0 as transparent colour by default
    bitmap->SetMask(new wxMask(*bitmap, *wxLIGHT_GREY));

    return true;
}

bool wxBMPFileHandler::LoadFile(wxBitmap *bitmap,
                                const wxString& name, long WXUNUSED(flags),
                                int WXUNUSED(desiredWidth),
                                int WXUNUSED(desiredHeight))
{
#if wxUSE_WXDIB
    wxCHECK_MSG( bitmap, false, _T("NULL bitmap in LoadFile") );

    wxDIB dib(name);

    return dib.IsOk() && bitmap->CopyFromDIB(dib);
#else
    return false;
#endif
}

bool wxBMPFileHandler::SaveFile(wxBitmap *bitmap,
                                const wxString& name,
                                int WXUNUSED(type),
                                const wxPalette * WXUNUSED(pal))
{
#if wxUSE_WXDIB
    wxCHECK_MSG( bitmap, false, _T("NULL bitmap in SaveFile") );

    wxDIB dib(*bitmap);

    return dib.Save(name);
#else
    return false;
#endif
}

// ----------------------------------------------------------------------------
// wxIcon handlers
// ----------------------------------------------------------------------------

bool wxICOFileHandler::LoadIcon(wxIcon *icon,
                                const wxString& name,
                                long WXUNUSED(flags),
                                int desiredWidth, int desiredHeight)
{
    icon->UnRef();

    // actual size
    wxSize size;

    HICON hicon = NULL;

    // Parse the filename: it may be of the form "filename;n" in order to
    // specify the nth icon in the file.
    //
    // For the moment, ignore the issue of possible semicolons in the
    // filename.
    int iconIndex = 0;
    wxString nameReal(name);
    wxString strIconIndex = name.AfterLast(wxT(';'));
    if (strIconIndex != name)
    {
        iconIndex = wxAtoi(strIconIndex);
        nameReal = name.BeforeLast(wxT(';'));
    }

#if 0
    // If we don't know what size icon we're looking for,
    // try to find out what's there.
    // Unfortunately this doesn't work, because ExtractIconEx
    // will scale the icon to the 'desired' size, even if that
    // size of icon isn't explicitly stored. So we would have
    // to parse the icon file outselves.
    if ( desiredWidth == -1 &&
         desiredHeight == -1)
    {
        // Try loading a large icon first
        if ( ::ExtractIconEx(nameReal, iconIndex, &hicon, NULL, 1) == 1)
        {
        }
        // Then try loading a small icon
        else if ( ::ExtractIconEx(nameReal, iconIndex, NULL, &hicon, 1) == 1)
        {
        }
    }
    else
#endif
        // were we asked for a large icon?
    if ( desiredWidth == ::GetSystemMetrics(SM_CXICON) &&
         desiredHeight == ::GetSystemMetrics(SM_CYICON) )
    {
        // get the specified large icon from file
        if ( !::ExtractIconEx(nameReal, iconIndex, &hicon, NULL, 1) )
        {
            // it is not an error, but it might still be useful to be informed
            // about it optionally
            wxLogTrace(_T("iconload"),
                       _T("No large icons found in the file '%s'."),
                       name.c_str());
        }
    }
    else if ( desiredWidth == ::GetSystemMetrics(SM_CXSMICON) &&
              desiredHeight == ::GetSystemMetrics(SM_CYSMICON) )
    {
        // get the specified small icon from file
        if ( !::ExtractIconEx(nameReal, iconIndex, NULL, &hicon, 1) )
        {
            wxLogTrace(_T("iconload"),
                       _T("No small icons found in the file '%s'."),
                       name.c_str());
        }
    }
    //else: not standard size, load below

#ifndef __WXWINCE__
    if ( !hicon )
    {
        // take any size icon from the file by index
        hicon = ::ExtractIcon(wxGetInstance(), nameReal, iconIndex);
    }
#endif

    if ( !hicon )
    {
        wxLogSysError(_T("Failed to load icon from the file '%s'"),
                      name.c_str());

        return false;
    }

    size = wxGetHiconSize(hicon);

    if ( (desiredWidth != -1 && desiredWidth != size.x) ||
         (desiredHeight != -1 && desiredHeight != size.y) )
    {
        wxLogTrace(_T("iconload"),
                   _T("Returning false from wxICOFileHandler::Load because of the size mismatch: actual (%d, %d), requested (%d, %d)"),
                   size.x, size.y,
                   desiredWidth, desiredHeight);

        ::DestroyIcon(hicon);

        return false;
    }

    icon->SetHICON((WXHICON)hicon);
    icon->SetSize(size.x, size.y);

    return icon->Ok();
}

bool wxICOResourceHandler::LoadIcon(wxIcon *icon,
                                    const wxString& name,
                                    long WXUNUSED(flags),
                                    int desiredWidth, int desiredHeight)
{
    HICON hicon;

    // do we need the icon of the specific size or would any icon do?
    bool hasSize = desiredWidth != -1 || desiredHeight != -1;

    wxASSERT_MSG( !hasSize || (desiredWidth != -1 && desiredHeight != -1),
                  _T("width and height should be either both -1 or not") );

    // try to load the icon from this program first to allow overriding the
    // standard icons (although why one would want to do it considering that
    // we already have wxApp::GetStdIcon() is unclear)

    // note that we can't just always call LoadImage() because it seems to do
    // some icon rescaling internally which results in very ugly 16x16 icons
    if ( hasSize )
    {
        hicon = (HICON)::LoadImage(wxGetInstance(), name, IMAGE_ICON,
                                    desiredWidth, desiredHeight,
                                    LR_DEFAULTCOLOR);
    }
    else
    {
        hicon = ::LoadIcon(wxGetInstance(), name);
    }

    // next check if it's not a standard icon
#ifndef __WXWINCE__
    if ( !hicon && !hasSize )
    {
        static const struct
        {
            const wxChar *name;
            LPTSTR id;
        } stdIcons[] =
        {
            { wxT("wxICON_QUESTION"),   IDI_QUESTION    },
            { wxT("wxICON_WARNING"),    IDI_EXCLAMATION },
            { wxT("wxICON_ERROR"),      IDI_HAND        },
            { wxT("wxICON_INFORMATION"),       IDI_ASTERISK    },
        };

        for ( size_t nIcon = 0; !hicon && nIcon < WXSIZEOF(stdIcons); nIcon++ )
        {
            if ( name == stdIcons[nIcon].name )
            {
                hicon = ::LoadIcon((HINSTANCE)NULL, stdIcons[nIcon].id);
            }
        }
    }
#endif

    wxSize size = wxGetHiconSize(hicon);
    icon->SetSize(size.x, size.y);

    icon->SetHICON((WXHICON)hicon);

    return icon->Ok();
}

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

wxSize wxGetHiconSize(HICON WXUNUSED_IN_WINCE(hicon))
{
    wxSize size;

#ifndef __WXWINCE__
    if ( hicon )
    {
        ICONINFO info;
        if ( !::GetIconInfo(hicon, &info) )
        {
            wxLogLastError(wxT("GetIconInfo"));
        }
        else
        {
            HBITMAP hbmp = info.hbmMask;
            if ( hbmp )
            {
                BITMAP bm;
                if ( ::GetObject(hbmp, sizeof(BITMAP), (LPSTR) &bm) )
                {
                    size = wxSize(bm.bmWidth, bm.bmHeight);
                }

                ::DeleteObject(info.hbmMask);
            }
            if ( info.hbmColor )
                ::DeleteObject(info.hbmColor);
        }
    }

    if ( !size.x )
#endif // !__WXWINCE__
    {
        // use default icon size on this hardware
        size.x = ::GetSystemMetrics(SM_CXICON);
        size.y = ::GetSystemMetrics(SM_CYICON);
    }

    return size;
}

#endif // __WXMICROWIN__
