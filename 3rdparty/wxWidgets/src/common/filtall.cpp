/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/filtall.cpp
// Purpose:     Link all filter streams
// Author:      Mike Wetherell
// RCS-ID:      $Id: filtall.cpp 42412 2006-10-25 20:41:12Z MW $
// Copyright:   (c) 2006 Mike Wetherell
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_STREAMS

#if wxUSE_ZLIB
#include "wx/zstream.h"
#endif

// Reference filter classes to ensure they are linked into a statically
// linked program that uses Find or GetFirst to look for an filter handler.
// It is in its own file so that the user can override this behaviour by
// providing their own implementation.

void wxUseFilterClasses()
{
#if wxUSE_ZLIB
    wxZlibClassFactory();
    wxGzipClassFactory();
#endif
}

#endif // wxUSE_STREAMS
