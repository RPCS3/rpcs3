/////////////////////////////////////////////////////////////////////////////
// Name:         src/common/dynload.cpp
// Purpose:      Dynamic loading framework
// Author:       Ron Lee, David Falkinder, Vadim Zeitlin and a cast of 1000's
//               (derived in part from dynlib.cpp (c) 1998 Guilhem Lavaux)
// Modified by:
// Created:      03/12/01
// RCS-ID:       $Id: dynload.cpp 40943 2006-08-31 19:31:43Z ABX $
// Copyright:    (c) 2001 Ron Lee <ron@debian.org>
// Licence:      wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_DYNAMIC_LOADER

#ifdef __WINDOWS__
    #include "wx/msw/private.h"
#endif

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/intl.h"
    #include "wx/hash.h"
    #include "wx/utils.h"
    #include "wx/module.h"
#endif

#include "wx/strconv.h"

#include "wx/dynload.h"


// ---------------------------------------------------------------------------
// wxPluginLibrary
// ---------------------------------------------------------------------------


wxDLImports*  wxPluginLibrary::ms_classes = NULL;

class wxPluginLibraryModule : public wxModule
{
public:
    wxPluginLibraryModule() { }

    // TODO: create ms_classes on demand, why always preallocate it?
    virtual bool OnInit()
    {
        wxPluginLibrary::ms_classes = new wxDLImports;
        wxPluginManager::CreateManifest();
        return true;
    }

    virtual void OnExit()
    {
        delete wxPluginLibrary::ms_classes;
        wxPluginLibrary::ms_classes = NULL;
        wxPluginManager::ClearManifest();
    }

private:
    DECLARE_DYNAMIC_CLASS(wxPluginLibraryModule )
};

IMPLEMENT_DYNAMIC_CLASS(wxPluginLibraryModule, wxModule)


wxPluginLibrary::wxPluginLibrary(const wxString &libname, int flags)
        : m_linkcount(1)
        , m_objcount(0)
{
    m_before = wxClassInfo::sm_first;
    Load( libname, flags );
    m_after = wxClassInfo::sm_first;

    if( m_handle != 0 )
    {
        UpdateClasses();
        RegisterModules();
    }
    else
    {
        // Flag us for deletion
        --m_linkcount;
    }
}

wxPluginLibrary::~wxPluginLibrary()
{
    if( m_handle != 0 )
    {
        UnregisterModules();
        RestoreClasses();
    }
}

wxPluginLibrary *wxPluginLibrary::RefLib()
{
    wxCHECK_MSG( m_linkcount > 0, NULL,
                 _T("Library had been already deleted!") );

    ++m_linkcount;
    return this;
}

bool wxPluginLibrary::UnrefLib()
{
    wxASSERT_MSG( m_objcount == 0,
                  _T("Library unloaded before all objects were destroyed") );

    if ( m_linkcount == 0 || --m_linkcount == 0 )
    {
        delete this;
        return true;
    }

    return false;
}

// ------------------------
// Private methods
// ------------------------

void wxPluginLibrary::UpdateClasses()
{
    for (wxClassInfo *info = m_after; info != m_before; info = info->m_next)
    {
        if( info->GetClassName() )
        {
            // Hash all the class names into a local table too so
            // we can quickly find the entry they correspond to.
            (*ms_classes)[info->GetClassName()] = this;
        }
    }
}

void wxPluginLibrary::RestoreClasses()
{
    // Check if there is a need to restore classes.
    if (!ms_classes)
        return;

    for(wxClassInfo *info = m_after; info != m_before; info = info->m_next)
    {
        ms_classes->erase(ms_classes->find(info->GetClassName()));
    }
}

void wxPluginLibrary::RegisterModules()
{
    // Plugin libraries might have wxModules, Register and initialise them if
    // they do.
    //
    // Note that these classes are NOT included in the reference counting since
    // it's implicit that they will be unloaded if and when the last handle to
    // the library is.  We do have to keep a copy of the module's pointer
    // though, as there is currently no way to Unregister it without it.

    wxASSERT_MSG( m_linkcount == 1,
                  _T("RegisterModules should only be called for the first load") );

    for ( wxClassInfo *info = m_after; info != m_before; info = info->m_next)
    {
        if( info->IsKindOf(CLASSINFO(wxModule)) )
        {
            wxModule *m = wxDynamicCast(info->CreateObject(), wxModule);

            wxASSERT_MSG( m, _T("wxDynamicCast of wxModule failed") );

            m_wxmodules.push_back(m);
            wxModule::RegisterModule(m);
        }
    }

    // FIXME: Likewise this is (well was) very similar to InitializeModules()

    for ( wxModuleList::iterator it = m_wxmodules.begin();
          it != m_wxmodules.end();
          ++it)
    {
        if( !(*it)->Init() )
        {
            wxLogDebug(_T("wxModule::Init() failed for wxPluginLibrary"));

            // XXX: Watch this, a different hash implementation might break it,
            //      a good hash implementation would let us fix it though.

            // The name of the game is to remove any uninitialised modules and
            // let the dtor Exit the rest on shutdown, (which we'll initiate
            // shortly).

            wxModuleList::iterator oldNode = m_wxmodules.end();
            do {
                ++it;
                if( oldNode != m_wxmodules.end() )
                    m_wxmodules.erase(oldNode);
                wxModule::UnregisterModule( *it );
                oldNode = it;
            } while( it != m_wxmodules.end() );

            --m_linkcount;     // Flag us for deletion
            break;
        }
    }
}

void wxPluginLibrary::UnregisterModules()
{
    wxModuleList::iterator it;

    for ( it = m_wxmodules.begin(); it != m_wxmodules.end(); ++it )
        (*it)->Exit();

    for ( it = m_wxmodules.begin(); it != m_wxmodules.end(); ++it )
        wxModule::UnregisterModule( *it );

    // NB: content of the list was deleted by UnregisterModule calls above:
    m_wxmodules.clear();
}


// ---------------------------------------------------------------------------
// wxPluginManager
// ---------------------------------------------------------------------------

wxDLManifest*   wxPluginManager::ms_manifest = NULL;

// ------------------------
// Static accessors
// ------------------------

wxPluginLibrary *
wxPluginManager::LoadLibrary(const wxString &libname, int flags)
{
    wxString realname(libname);

    if( !(flags & wxDL_VERBATIM) )
        realname += wxDynamicLibrary::GetDllExt();

    wxPluginLibrary *entry;

    if ( flags & wxDL_NOSHARE )
    {
        entry = NULL;
    }
    else
    {
        entry = FindByName(realname);
    }

    if ( entry )
    {
        wxLogTrace(_T("dll"),
                   _T("LoadLibrary(%s): already loaded."), realname.c_str());

        entry->RefLib();
    }
    else
    {
        entry = new wxPluginLibrary( libname, flags );

        if ( entry->IsLoaded() )
        {
            (*ms_manifest)[realname] = entry;

            wxLogTrace(_T("dll"),
                       _T("LoadLibrary(%s): loaded ok."), realname.c_str());

        }
        else
        {
            wxLogTrace(_T("dll"),
                       _T("LoadLibrary(%s): failed to load."), realname.c_str());

            // we have created entry just above
            if ( !entry->UnrefLib() )
            {
                // ... so UnrefLib() is supposed to delete it
                wxFAIL_MSG( _T("Currently linked library is not loaded?") );
            }

            entry = NULL;
        }
    }

    return entry;
}

bool wxPluginManager::UnloadLibrary(const wxString& libname)
{
    wxString realname = libname;

    wxPluginLibrary *entry = FindByName(realname);

    if ( !entry )
    {
        realname += wxDynamicLibrary::GetDllExt();

        entry = FindByName(realname);
    }

    if ( !entry )
    {
        wxLogDebug(_T("Attempt to unload library '%s' which is not loaded."),
                   libname.c_str());

        return false;
    }

    wxLogTrace(_T("dll"), _T("UnloadLibrary(%s)"), realname.c_str());

    if ( !entry->UnrefLib() )
    {
        // not really unloaded yet
        return false;
    }

    ms_manifest->erase(ms_manifest->find(realname));

    return true;
}

// ------------------------
// Class implementation
// ------------------------

bool wxPluginManager::Load(const wxString &libname, int flags)
{
    m_entry = wxPluginManager::LoadLibrary(libname, flags);

    return IsLoaded();
}

void wxPluginManager::Unload()
{
    wxCHECK_RET( m_entry, _T("unloading an invalid wxPluginManager?") );

    for ( wxDLManifest::iterator i = ms_manifest->begin();
          i != ms_manifest->end();
          ++i )
    {
        if ( i->second == m_entry )
        {
            ms_manifest->erase(i);
            break;
        }
    }

    m_entry->UnrefLib();

    m_entry = NULL;
}

#endif  // wxUSE_DYNAMIC_LOADER
