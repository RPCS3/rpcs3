/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/module.cpp
// Purpose:     Modules initialization/destruction
// Author:      Wolfram Gloger/adapted by Guilhem Lavaux
// Modified by:
// Created:     04/11/98
// RCS-ID:      $Id: module.cpp 39677 2006-06-11 22:19:12Z VZ $
// Copyright:   (c) Wolfram Gloger and Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/module.h"

#ifndef WX_PRECOMP
    #include "wx/hash.h"
    #include "wx/intl.h"
    #include "wx/log.h"
#endif

#include "wx/listimpl.cpp"

#define TRACE_MODULE _T("module")

WX_DEFINE_LIST(wxModuleList)

IMPLEMENT_CLASS(wxModule, wxObject)

wxModuleList wxModule::m_modules;

void wxModule::RegisterModule(wxModule* module)
{
    module->m_state = State_Registered;
    m_modules.Append(module);
}

void wxModule::UnregisterModule(wxModule* module)
{
    m_modules.DeleteObject(module);
    delete module;
}

// Collect up all module-derived classes, create an instance of each,
// and register them.
void wxModule::RegisterModules()
{
    wxHashTable::compatibility_iterator node;
    wxClassInfo* classInfo;

    wxClassInfo::sm_classTable->BeginFind();
    node = wxClassInfo::sm_classTable->Next();
    while (node)
    {
        classInfo = (wxClassInfo *)node->GetData();
        if ( classInfo->IsKindOf(CLASSINFO(wxModule)) &&
            (classInfo != (& (wxModule::ms_classInfo))) )
        {
            wxLogTrace(TRACE_MODULE, wxT("Registering module %s"),
                       classInfo->GetClassName());
            wxModule* module = (wxModule *)classInfo->CreateObject();
            RegisterModule(module);
        }
        node = wxClassInfo::sm_classTable->Next();
    }
}

bool wxModule::DoInitializeModule(wxModule *module,
                                  wxModuleList &initializedModules)
{
    if ( module->m_state == State_Initializing )
    {
        wxLogError(_("Circular dependency involving module \"%s\" detected."),
                   module->GetClassInfo()->GetClassName());
        return false;
    }

    module->m_state = State_Initializing;

    const wxArrayClassInfo& dependencies = module->m_dependencies;

    // satisfy module dependencies by loading them before the current module
    for ( unsigned int i = 0; i < dependencies.size(); ++i )
    {
        wxClassInfo * cinfo = dependencies[i];

        // Check if the module is already initialized
        wxModuleList::compatibility_iterator node;
        for ( node = initializedModules.GetFirst(); node; node = node->GetNext() )
        {
            if ( node->GetData()->GetClassInfo() == cinfo )
                break;
        }

        if ( node )
        {
            // this dependency is already initialized, nothing to do
            continue;
        }

        // find the module in the registered modules list
        for ( node = m_modules.GetFirst(); node; node = node->GetNext() )
        {
            wxModule *moduleDep = node->GetData();
            if ( moduleDep->GetClassInfo() == cinfo )
            {
                if ( !DoInitializeModule(moduleDep, initializedModules ) )
                {
                    // failed to initialize a dependency, so fail this one too
                    return false;
                }

                break;
            }
        }

        if ( !node )
        {
            wxLogError(_("Dependency \"%s\" of module \"%s\" doesn't exist."),
                       cinfo->GetClassName(),
                       module->GetClassInfo()->GetClassName());
            return false;
        }
    }

    if ( !module->Init() )
    {
        wxLogError(_("Module \"%s\" initialization failed"),
                   module->GetClassInfo()->GetClassName());
        return false;
    }

    wxLogTrace(TRACE_MODULE, wxT("Module \"%s\" initialized"),
               module->GetClassInfo()->GetClassName());

    module->m_state = State_Initialized;
    initializedModules.Append(module);

    return true;
}

// Initialize user-defined modules
bool wxModule::InitializeModules()
{
    wxModuleList initializedModules;

    for ( wxModuleList::compatibility_iterator node = m_modules.GetFirst();
          node;
          node = node->GetNext() )
    {
        wxModule *module = node->GetData();

        // the module could have been already initialized as dependency of
        // another one
        if ( module->m_state == State_Registered )
        {
            if ( !DoInitializeModule( module, initializedModules ) )
            {
                // failed to initialize all modules, so clean up the already
                // initialized ones
                DoCleanUpModules(initializedModules);

                return false;
            }
        }
    }

    // remember the real initialisation order
    m_modules = initializedModules;

    return true;
}

// Clean up all currently initialized modules
void wxModule::DoCleanUpModules(const wxModuleList& modules)
{
    // cleanup user-defined modules in the reverse order compared to their
    // initialization -- this ensures that dependencies are respected
    for ( wxModuleList::compatibility_iterator node = modules.GetLast();
          node;
          node = node->GetPrevious() )
    {
        wxLogTrace(TRACE_MODULE, wxT("Cleanup module %s"),
                   node->GetData()->GetClassInfo()->GetClassName());

        wxModule * module = node->GetData();

        wxASSERT_MSG( module->m_state == State_Initialized,
                        _T("not initialized module being cleaned up") );

        module->Exit();
        module->m_state = State_Registered;
    }

    // clear all modules, even the non-initialized ones
    WX_CLEAR_LIST(wxModuleList, m_modules);
}
