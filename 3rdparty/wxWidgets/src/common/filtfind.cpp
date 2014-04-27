/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/filtfind.cpp
// Purpose:     Streams for filter formats
// Author:      Mike Wetherell
// RCS-ID:      $Id: filtfind.cpp 42412 2006-10-25 20:41:12Z MW $
// Copyright:   (c) Mike Wetherell
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_STREAMS

#include "wx/stream.h"

// These functions are in a separate file so that statically linked apps
// that do not call them to search for filter handlers will only link in
// the filter classes they use.

const wxFilterClassFactory *
wxFilterClassFactory::Find(const wxChar *protocol, wxStreamProtocolType type)
{
    for (const wxFilterClassFactory *f = GetFirst(); f; f = f->GetNext())
        if (f->CanHandle(protocol, type))
            return f;

    return NULL;
}

// static
const wxFilterClassFactory *wxFilterClassFactory::GetFirst()
{
    if (!sm_first)
        wxUseFilterClasses();
    return sm_first;
}

#endif // wxUSE_STREAMS
