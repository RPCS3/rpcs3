/////////////////////////////////////////////////////////////////////////////
// Name:         dynload.h
// Purpose:      Dynamic loading framework
// Author:       Ron Lee, David Falkinder, Vadim Zeitlin and a cast of 1000's
//               (derived in part from dynlib.cpp (c) 1998 Guilhem Lavaux)
// Modified by:
// Created:      03/12/01
// RCS-ID:       $Id: dynload.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:    (c) 2001 Ron Lee <ron@debian.org>
// Licence:      wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DYNAMICLOADER_H__
#define _WX_DYNAMICLOADER_H__

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/defs.h"

#if wxUSE_DYNAMIC_LOADER

#include "wx/dynlib.h"
#include "wx/hashmap.h"
#include "wx/module.h"

class WXDLLIMPEXP_FWD_BASE wxPluginLibrary;


WX_DECLARE_STRING_HASH_MAP_WITH_DECL(wxPluginLibrary *, wxDLManifest,
                                     class WXDLLIMPEXP_BASE);
typedef wxDLManifest wxDLImports;

// ---------------------------------------------------------------------------
// wxPluginLibrary
// ---------------------------------------------------------------------------

// NOTE: Do not attempt to use a base class pointer to this class.
//       wxDL is not virtual and we deliberately hide some of it's
//       methods here.
//
//       Unless you know exacty why you need to, you probably shouldn't
//       instantiate this class directly anyway, use wxPluginManager
//       instead.

class WXDLLIMPEXP_BASE wxPluginLibrary : public wxDynamicLibrary
{
public:

    static wxDLImports* ms_classes;  // Static hash of all imported classes.

    wxPluginLibrary( const wxString &libname, int flags = wxDL_DEFAULT );
    ~wxPluginLibrary();

    wxPluginLibrary  *RefLib();
    bool              UnrefLib();

        // These two are called by the PluginSentinel on (PLUGGABLE) object
        // creation/destruction.  There is usually no reason for the user to
        // call them directly.  We have to separate this from the link count,
        // since the two are not interchangeable.

        // FIXME: for even better debugging PluginSentinel should register
        //        the name of the class created too, then we can state
        //        exactly which object was not destroyed which may be
        //        difficult to find otherwise.  Also this code should
        //        probably only be active in DEBUG mode, but let's just
        //        get it right first.

    void  RefObj() { ++m_objcount; }
    void  UnrefObj()
    {
        wxASSERT_MSG( m_objcount > 0, wxT("Too many objects deleted??") );
        --m_objcount;
    }

        // Override/hide some base class methods

    bool  IsLoaded() const { return m_linkcount > 0; }
    void  Unload() { UnrefLib(); }

private:

    wxClassInfo    *m_before;       // sm_first before loading this lib
    wxClassInfo    *m_after;        // ..and after.

    size_t          m_linkcount;    // Ref count of library link calls
    size_t          m_objcount;     // ..and (pluggable) object instantiations.
    wxModuleList    m_wxmodules;    // any wxModules that we initialised.

    void    UpdateClasses();        // Update ms_classes
    void    RestoreClasses();       // Removes this library from ms_classes
    void    RegisterModules();      // Init any wxModules in the lib.
    void    UnregisterModules();    // Cleanup any wxModules we installed.

    DECLARE_NO_COPY_CLASS(wxPluginLibrary)
};


class WXDLLIMPEXP_BASE wxPluginManager
{
public:

        // Static accessors.

    static wxPluginLibrary    *LoadLibrary( const wxString &libname,
                                            int flags = wxDL_DEFAULT );
    static bool                UnloadLibrary(const wxString &libname);

        // Instance methods.

    wxPluginManager() : m_entry(NULL) {}
    wxPluginManager(const wxString &libname, int flags = wxDL_DEFAULT)
    {
        Load(libname, flags);
    }
    ~wxPluginManager() { if ( IsLoaded() ) Unload(); }

    bool   Load(const wxString &libname, int flags = wxDL_DEFAULT);
    void   Unload();

    bool   IsLoaded() const { return m_entry && m_entry->IsLoaded(); }
    void  *GetSymbol(const wxString &symbol, bool *success = 0)
    {
        return m_entry->GetSymbol( symbol, success );
    }

    static void CreateManifest() { ms_manifest = new wxDLManifest(wxKEY_STRING); }
    static void ClearManifest() { delete ms_manifest; ms_manifest = NULL; }

private:
    // return the pointer to the entry for the library with given name in
    // ms_manifest or NULL if none
    static wxPluginLibrary *FindByName(const wxString& name)
    {
        const wxDLManifest::iterator i = ms_manifest->find(name);

        return i == ms_manifest->end() ? NULL : i->second;
    }

    static wxDLManifest* ms_manifest;  // Static hash of loaded libs.
    wxPluginLibrary*     m_entry;      // Cache our entry in the manifest.

    // We could allow this class to be copied if we really
    // wanted to, but not without modification.
    DECLARE_NO_COPY_CLASS(wxPluginManager)
};


#endif  // wxUSE_DYNAMIC_LOADER
#endif  // _WX_DYNAMICLOADER_H__

