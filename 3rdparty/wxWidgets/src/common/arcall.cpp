/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/arcall.cpp
// Purpose:     wxArchive link all archive streams
// Author:      Mike Wetherell
// RCS-ID:      $Id: arcall.cpp 42508 2006-10-27 09:53:38Z MW $
// Copyright:   (c) 2006 Mike Wetherell
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_ARCHIVE_STREAMS

#if wxUSE_ZIPSTREAM
#include "wx/zipstrm.h"
#endif
#if wxUSE_TARSTREAM
#include "wx/tarstrm.h"
#endif

// Reference archive classes to ensure they are linked into a statically
// linked program that uses Find or GetFirst to look for an archive handler.
// It is in its own file so that the user can override this behaviour by
// providing their own implementation.

void wxUseArchiveClasses()
{
#if wxUSE_ZIPSTREAM
    wxZipClassFactory();
#endif
#if wxUSE_TARSTREAM
    wxTarClassFactory();
#endif
}

#endif // wxUSE_ARCHIVE_STREAMS
