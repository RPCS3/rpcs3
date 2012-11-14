/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/artprov.cpp
// Purpose:     wxArtProvider class
// Author:      Vaclav Slavik
// Modified by:
// Created:     18/03/2002
// RCS-ID:      $Id: artprov.cpp 57701 2008-12-31 23:40:06Z VS $
// Copyright:   (c) Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if defined(__BORLANDC__)
    #pragma hdrstop
#endif

#include "wx/artprov.h"

#ifndef WX_PRECOMP
    #include "wx/list.h"
    #include "wx/log.h"
    #include "wx/hashmap.h"
    #include "wx/image.h"
    #include "wx/module.h"
#endif

#ifdef __WXMSW__
    #include "wx/msw/wrapwin.h"
#endif

// ===========================================================================
// implementation
// ===========================================================================

#include "wx/listimpl.cpp"
WX_DECLARE_LIST(wxArtProvider, wxArtProvidersList);
WX_DEFINE_LIST(wxArtProvidersList)

// ----------------------------------------------------------------------------
// Cache class - stores already requested bitmaps
// ----------------------------------------------------------------------------

WX_DECLARE_EXPORTED_STRING_HASH_MAP(wxBitmap, wxArtProviderBitmapsHash);

class WXDLLEXPORT wxArtProviderCache
{
public:
    bool GetBitmap(const wxString& full_id, wxBitmap* bmp);
    void PutBitmap(const wxString& full_id, const wxBitmap& bmp)
        { m_bitmapsHash[full_id] = bmp; }

    void Clear();

    static wxString ConstructHashID(const wxArtID& id,
                                    const wxArtClient& client,
                                    const wxSize& size);

private:
    wxArtProviderBitmapsHash m_bitmapsHash;
};

bool wxArtProviderCache::GetBitmap(const wxString& full_id, wxBitmap* bmp)
{
    wxArtProviderBitmapsHash::iterator entry = m_bitmapsHash.find(full_id);
    if ( entry == m_bitmapsHash.end() )
    {
        return false;
    }
    else
    {
        *bmp = entry->second;
        return true;
    }
}

void wxArtProviderCache::Clear()
{
    m_bitmapsHash.clear();
}

/*static*/ wxString wxArtProviderCache::ConstructHashID(
                                const wxArtID& id, const wxArtClient& client,
                                const wxSize& size)
{
    wxString str;
    str.Printf(wxT("%s-%s-%i-%i"), id.c_str(), client.c_str(), size.x, size.y);
    return str;
}


// ============================================================================
// wxArtProvider class
// ============================================================================

IMPLEMENT_ABSTRACT_CLASS(wxArtProvider, wxObject)

wxArtProvidersList *wxArtProvider::sm_providers = NULL;
wxArtProviderCache *wxArtProvider::sm_cache = NULL;

// ----------------------------------------------------------------------------
// wxArtProvider ctors/dtor
// ----------------------------------------------------------------------------

wxArtProvider::~wxArtProvider()
{
    Remove(this);
}

// ----------------------------------------------------------------------------
// wxArtProvider operations on provider stack
// ----------------------------------------------------------------------------

/*static*/ void wxArtProvider::CommonAddingProvider()
{
    if ( !sm_providers )
    {
        sm_providers = new wxArtProvidersList;
        sm_cache = new wxArtProviderCache;
    }

    sm_cache->Clear();
}

/*static*/ void wxArtProvider::Push(wxArtProvider *provider)
{
    CommonAddingProvider();
    sm_providers->Insert(provider);
}

/*static*/ void wxArtProvider::Insert(wxArtProvider *provider)
{
    CommonAddingProvider();
    sm_providers->Append(provider);
}

/*static*/ void wxArtProvider::PushBack(wxArtProvider *provider)
{
    Insert(provider);
}

/*static*/ bool wxArtProvider::Pop()
{
    wxCHECK_MSG( sm_providers, false, _T("no wxArtProvider exists") );
    wxCHECK_MSG( !sm_providers->empty(), false, _T("wxArtProviders stack is empty") );

    delete sm_providers->GetFirst()->GetData();
    sm_cache->Clear();
    return true;
}

/*static*/ bool wxArtProvider::Remove(wxArtProvider *provider)
{
    wxCHECK_MSG( sm_providers, false, _T("no wxArtProvider exists") );

    if ( sm_providers->DeleteObject(provider) )
    {
        sm_cache->Clear();
        return true;
    }

    return false;
}

/*static*/ bool wxArtProvider::Delete(wxArtProvider *provider)
{
    // provider will remove itself from the stack in its dtor
    delete provider;

    return true;
}

/*static*/ void wxArtProvider::CleanUpProviders()
{
    if ( sm_providers )
    {
        while ( !sm_providers->empty() )
            delete *sm_providers->begin();

        delete sm_providers;
        sm_providers = NULL;

        delete sm_cache;
        sm_cache = NULL;
    }
}

// ----------------------------------------------------------------------------
// wxArtProvider: retrieving bitmaps/icons
// ----------------------------------------------------------------------------

/*static*/ wxBitmap wxArtProvider::GetBitmap(const wxArtID& id,
                                             const wxArtClient& client,
                                             const wxSize& size)
{
    // safety-check against writing client,id,size instead of id,client,size:
    wxASSERT_MSG( client.Last() == _T('C'), _T("invalid 'client' parameter") );

    wxCHECK_MSG( sm_providers, wxNullBitmap, _T("no wxArtProvider exists") );

    wxString hashId = wxArtProviderCache::ConstructHashID(id, client, size);

    wxBitmap bmp;
    if ( !sm_cache->GetBitmap(hashId, &bmp) )
    {
        for (wxArtProvidersList::compatibility_iterator node = sm_providers->GetFirst();
             node; node = node->GetNext())
        {
            bmp = node->GetData()->CreateBitmap(id, client, size);
            if ( bmp.Ok() )
            {
#if wxUSE_IMAGE && (!defined(__WXMSW__) || wxUSE_WXDIB)
                if ( size != wxDefaultSize &&
                     (bmp.GetWidth() != size.x || bmp.GetHeight() != size.y) )
                {
                    wxImage img = bmp.ConvertToImage();
                    img.Rescale(size.x, size.y);
                    bmp = wxBitmap(img);
                }
#endif
                break;
            }
        }

        sm_cache->PutBitmap(hashId, bmp);
    }

    return bmp;
}

/*static*/ wxIcon wxArtProvider::GetIcon(const wxArtID& id,
                                         const wxArtClient& client,
                                         const wxSize& size)
{
    wxCHECK_MSG( sm_providers, wxNullIcon, _T("no wxArtProvider exists") );

    wxBitmap bmp = GetBitmap(id, client, size);
    if ( !bmp.Ok() )
        return wxNullIcon;

    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    return icon;
}

#if defined(__WXGTK20__) && !defined(__WXUNIVERSAL__)
    #include "wx/gtk/private.h"
    extern GtkIconSize wxArtClientToIconSize(const wxArtClient& client);
#endif // defined(__WXGTK20__) && !defined(__WXUNIVERSAL__)

/*static*/ wxSize wxArtProvider::GetSizeHint(const wxArtClient& client,
                                         bool platform_dependent)
{
    if (!platform_dependent)
    {
        wxArtProvidersList::compatibility_iterator node = sm_providers->GetFirst();
        if (node)
            return node->GetData()->DoGetSizeHint(client);
    }

        // else return platform dependent size

#if defined(__WXGTK20__) && !defined(__WXUNIVERSAL__)
    // Gtk has specific sizes for each client, see artgtk.cpp
    GtkIconSize gtk_size = wxArtClientToIconSize(client);
    // no size hints for this client
    if (gtk_size == GTK_ICON_SIZE_INVALID)
        return wxDefaultSize;
    gint width, height;
    gtk_icon_size_lookup( gtk_size, &width, &height);
    return wxSize(width, height);
#else // !GTK+ 2
    // NB: These size hints may have to be adjusted per platform
    if (client == wxART_TOOLBAR)
        return wxSize(16, 15);
    else if (client == wxART_MENU)
        return wxSize(16, 15);
    else if (client == wxART_FRAME_ICON)
    {
#ifdef __WXMSW__
        return wxSize(::GetSystemMetrics(SM_CXSMICON),
                      ::GetSystemMetrics(SM_CYSMICON));
#else
        return wxSize(16, 16);
#endif // __WXMSW__/!__WXMSW__
    }
    else if (client == wxART_CMN_DIALOG || client == wxART_MESSAGE_BOX)
        return wxSize(32, 32);
    else if (client == wxART_HELP_BROWSER)
        return wxSize(16, 15);
    else if (client == wxART_BUTTON)
        return wxSize(16, 15);
    else // wxART_OTHER or perhaps a user's client, no specified size
        return wxDefaultSize;
#endif // GTK+ 2/else
}

// ----------------------------------------------------------------------------
// deprecated wxArtProvider methods
// ----------------------------------------------------------------------------

#if WXWIN_COMPATIBILITY_2_6

/* static */ void wxArtProvider::PushProvider(wxArtProvider *provider)
{
    Push(provider);
}

/* static */ void wxArtProvider::InsertProvider(wxArtProvider *provider)
{
    Insert(provider);
}

/* static */ bool wxArtProvider::PopProvider()
{
    return Pop();
}

/* static */ bool wxArtProvider::RemoveProvider(wxArtProvider *provider)
{
    // RemoveProvider() used to delete the provider being removed so this is
    // not a typo, we must call Delete() and not Remove() here
    return Delete(provider);
}

#endif // WXWIN_COMPATIBILITY_2_6

// ============================================================================
// wxArtProviderModule
// ============================================================================

class wxArtProviderModule: public wxModule
{
public:
    bool OnInit()
    {
        wxArtProvider::InitStdProvider();
        wxArtProvider::InitNativeProvider();
        return true;
    }
    void OnExit()
    {
        wxArtProvider::CleanUpProviders();
    }

    DECLARE_DYNAMIC_CLASS(wxArtProviderModule)
};

IMPLEMENT_DYNAMIC_CLASS(wxArtProviderModule, wxModule)
