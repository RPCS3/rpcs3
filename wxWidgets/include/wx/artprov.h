/////////////////////////////////////////////////////////////////////////////
// Name:        wx/artprov.h
// Purpose:     wxArtProvider class
// Author:      Vaclav Slavik
// Modified by:
// Created:     18/03/2002
// RCS-ID:      $Id: artprov.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_ARTPROV_H_
#define _WX_ARTPROV_H_

#include "wx/string.h"
#include "wx/bitmap.h"
#include "wx/icon.h"

class WXDLLIMPEXP_FWD_CORE wxArtProvidersList;
class WXDLLIMPEXP_FWD_CORE wxArtProviderCache;
class wxArtProviderModule;

// ----------------------------------------------------------------------------
// Types
// ----------------------------------------------------------------------------

typedef wxString wxArtClient;
typedef wxString wxArtID;

#define wxART_MAKE_CLIENT_ID_FROM_STR(id)  (id + wxT("_C"))
#define wxART_MAKE_CLIENT_ID(id)           wxT(#id) wxT("_C")
#define wxART_MAKE_ART_ID_FROM_STR(id)     (id)
#define wxART_MAKE_ART_ID(id)              wxT(#id)

// ----------------------------------------------------------------------------
// Art clients
// ----------------------------------------------------------------------------

#define wxART_TOOLBAR              wxART_MAKE_CLIENT_ID(wxART_TOOLBAR)
#define wxART_MENU                 wxART_MAKE_CLIENT_ID(wxART_MENU)
#define wxART_FRAME_ICON           wxART_MAKE_CLIENT_ID(wxART_FRAME_ICON)

#define wxART_CMN_DIALOG           wxART_MAKE_CLIENT_ID(wxART_CMN_DIALOG)
#define wxART_HELP_BROWSER         wxART_MAKE_CLIENT_ID(wxART_HELP_BROWSER)
#define wxART_MESSAGE_BOX          wxART_MAKE_CLIENT_ID(wxART_MESSAGE_BOX)
#define wxART_BUTTON               wxART_MAKE_CLIENT_ID(wxART_BUTTON)

#define wxART_OTHER                wxART_MAKE_CLIENT_ID(wxART_OTHER)

// ----------------------------------------------------------------------------
// Art IDs
// ----------------------------------------------------------------------------

#define wxART_ADD_BOOKMARK         wxART_MAKE_ART_ID(wxART_ADD_BOOKMARK)
#define wxART_DEL_BOOKMARK         wxART_MAKE_ART_ID(wxART_DEL_BOOKMARK)
#define wxART_HELP_SIDE_PANEL      wxART_MAKE_ART_ID(wxART_HELP_SIDE_PANEL)
#define wxART_HELP_SETTINGS        wxART_MAKE_ART_ID(wxART_HELP_SETTINGS)
#define wxART_HELP_BOOK            wxART_MAKE_ART_ID(wxART_HELP_BOOK)
#define wxART_HELP_FOLDER          wxART_MAKE_ART_ID(wxART_HELP_FOLDER)
#define wxART_HELP_PAGE            wxART_MAKE_ART_ID(wxART_HELP_PAGE)
#define wxART_GO_BACK              wxART_MAKE_ART_ID(wxART_GO_BACK)
#define wxART_GO_FORWARD           wxART_MAKE_ART_ID(wxART_GO_FORWARD)
#define wxART_GO_UP                wxART_MAKE_ART_ID(wxART_GO_UP)
#define wxART_GO_DOWN              wxART_MAKE_ART_ID(wxART_GO_DOWN)
#define wxART_GO_TO_PARENT         wxART_MAKE_ART_ID(wxART_GO_TO_PARENT)
#define wxART_GO_HOME              wxART_MAKE_ART_ID(wxART_GO_HOME)
#define wxART_FILE_OPEN            wxART_MAKE_ART_ID(wxART_FILE_OPEN)
#define wxART_FILE_SAVE            wxART_MAKE_ART_ID(wxART_FILE_SAVE)
#define wxART_FILE_SAVE_AS         wxART_MAKE_ART_ID(wxART_FILE_SAVE_AS)
#define wxART_PRINT                wxART_MAKE_ART_ID(wxART_PRINT)
#define wxART_HELP                 wxART_MAKE_ART_ID(wxART_HELP)
#define wxART_TIP                  wxART_MAKE_ART_ID(wxART_TIP)
#define wxART_REPORT_VIEW          wxART_MAKE_ART_ID(wxART_REPORT_VIEW)
#define wxART_LIST_VIEW            wxART_MAKE_ART_ID(wxART_LIST_VIEW)
#define wxART_NEW_DIR              wxART_MAKE_ART_ID(wxART_NEW_DIR)
#define wxART_HARDDISK             wxART_MAKE_ART_ID(wxART_HARDDISK)
#define wxART_FLOPPY               wxART_MAKE_ART_ID(wxART_FLOPPY)
#define wxART_CDROM                wxART_MAKE_ART_ID(wxART_CDROM)
#define wxART_REMOVABLE            wxART_MAKE_ART_ID(wxART_REMOVABLE)
#define wxART_FOLDER               wxART_MAKE_ART_ID(wxART_FOLDER)
#define wxART_FOLDER_OPEN          wxART_MAKE_ART_ID(wxART_FOLDER_OPEN)
#define wxART_GO_DIR_UP            wxART_MAKE_ART_ID(wxART_GO_DIR_UP)
#define wxART_EXECUTABLE_FILE      wxART_MAKE_ART_ID(wxART_EXECUTABLE_FILE)
#define wxART_NORMAL_FILE          wxART_MAKE_ART_ID(wxART_NORMAL_FILE)
#define wxART_TICK_MARK            wxART_MAKE_ART_ID(wxART_TICK_MARK)
#define wxART_CROSS_MARK           wxART_MAKE_ART_ID(wxART_CROSS_MARK)
#define wxART_ERROR                wxART_MAKE_ART_ID(wxART_ERROR)
#define wxART_QUESTION             wxART_MAKE_ART_ID(wxART_QUESTION)
#define wxART_WARNING              wxART_MAKE_ART_ID(wxART_WARNING)
#define wxART_INFORMATION          wxART_MAKE_ART_ID(wxART_INFORMATION)
#define wxART_MISSING_IMAGE        wxART_MAKE_ART_ID(wxART_MISSING_IMAGE)

#define wxART_COPY                 wxART_MAKE_ART_ID(wxART_COPY)
#define wxART_CUT                  wxART_MAKE_ART_ID(wxART_CUT)
#define wxART_PASTE                wxART_MAKE_ART_ID(wxART_PASTE)
#define wxART_DELETE               wxART_MAKE_ART_ID(wxART_DELETE)
#define wxART_NEW                  wxART_MAKE_ART_ID(wxART_NEW)

#define wxART_UNDO                 wxART_MAKE_ART_ID(wxART_UNDO)
#define wxART_REDO                 wxART_MAKE_ART_ID(wxART_REDO)

#define wxART_QUIT                 wxART_MAKE_ART_ID(wxART_QUIT)

#define wxART_FIND                 wxART_MAKE_ART_ID(wxART_FIND)
#define wxART_FIND_AND_REPLACE     wxART_MAKE_ART_ID(wxART_FIND_AND_REPLACE)


// ----------------------------------------------------------------------------
// wxArtProvider class
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxArtProvider : public wxObject
{
public:
    // Dtor removes the provider from providers stack if it's still on it
    virtual ~wxArtProvider();


    // Add new provider to the top of providers stack (i.e. the provider will
    // be queried first of all).
    static void Push(wxArtProvider *provider);

    // Add new provider to the bottom of providers stack (i.e. the provider
    // will be queried as the last one).
#if wxABI_VERSION >= 20810
    static void PushBack(wxArtProvider *provider);
#endif
    // same as PushBack()
    static void Insert(wxArtProvider *provider);

    // Remove latest added provider and delete it.
    static bool Pop();

    // Remove provider from providers stack but don't delete it.
    static bool Remove(wxArtProvider *provider);

    // Delete the given provider and remove it from the providers stack.
    static bool Delete(wxArtProvider *provider);


    // Query the providers for bitmap with given ID and return it. Return
    // wxNullBitmap if no provider provides it.
    static wxBitmap GetBitmap(const wxArtID& id,
                              const wxArtClient& client = wxART_OTHER,
                              const wxSize& size = wxDefaultSize);

    // Query the providers for icon with given ID and return it. Return
    // wxNullIcon if no provider provides it.
    static wxIcon GetIcon(const wxArtID& id,
                          const wxArtClient& client = wxART_OTHER,
                          const wxSize& size = wxDefaultSize);

    // Get the size hint of an icon from a specific wxArtClient, queries
    // the topmost provider if platform_dependent = false
    static wxSize GetSizeHint(const wxArtClient& client, bool platform_dependent = false);

#if WXWIN_COMPATIBILITY_2_6
    // use the corresponding methods without redundant "Provider" suffix
    wxDEPRECATED( static void PushProvider(wxArtProvider *provider) );
    wxDEPRECATED( static void InsertProvider(wxArtProvider *provider) );
    wxDEPRECATED( static bool PopProvider() );

    // use Delete() if this is what you really need, or just delete the
    // provider pointer, do not use Remove() as it does not delete the pointer
    // unlike RemoveProvider() which does
    wxDEPRECATED( static bool RemoveProvider(wxArtProvider *provider) );
#endif // WXWIN_COMPATIBILITY_2_6

protected:
    friend class wxArtProviderModule;
    // Initializes default provider
    static void InitStdProvider();
    // Initializes platform's native provider, if available (e.g. GTK2)
    static void InitNativeProvider();
    // Destroy caches & all providers
    static void CleanUpProviders();

    // Get the default size of an icon for a specific client
    virtual wxSize DoGetSizeHint(const wxArtClient& client)
    {
        return GetSizeHint(client, true);
    }

    // Derived classes must override this method to create requested
    // art resource. This method is called only once per instance's
    // lifetime for each requested wxArtID.
    virtual wxBitmap CreateBitmap(const wxArtID& WXUNUSED(id),
                                  const wxArtClient& WXUNUSED(client),
                                  const wxSize& WXUNUSED(size)) = 0;

private:
    static void CommonAddingProvider();

private:
    // list of providers:
    static wxArtProvidersList *sm_providers;
    // art resources cache (so that CreateXXX is not called that often):
    static wxArtProviderCache *sm_cache;

    DECLARE_ABSTRACT_CLASS(wxArtProvider)
};


#endif // _WX_ARTPROV_H_
