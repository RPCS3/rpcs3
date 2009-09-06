///////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/wrapshl.h
// Purpose:     wrapper class for stuff from shell32.dll
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-10-19
// RCS-ID:      $Id: wrapshl.h 40367 2006-07-29 19:19:50Z VZ $
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_WRAPSHL_H_
#define _WX_MSW_WRAPSHL_H_

#ifdef __WXWINCE__
    #include <winreg.h>
    #include <objbase.h>
    #include <shlguid.h>
#endif

#include <shlobj.h>

#include "wx/msw/winundef.h"

// ----------------------------------------------------------------------------
// wxItemIdList implements RAII on top of ITEMIDLIST
// ----------------------------------------------------------------------------

class wxItemIdList
{
public:
    // ctor takes ownership of the item and will free it
    wxItemIdList(LPITEMIDLIST pidl)
    {
        m_pidl = pidl;
    }

    static void Free(LPITEMIDLIST pidl)
    {
        if ( pidl )
        {
            LPMALLOC pMalloc;
            SHGetMalloc(&pMalloc);
            if ( pMalloc )
            {
                pMalloc->Free(pidl);
                pMalloc->Release();
            }
            else
            {
                wxLogLastError(wxT("SHGetMalloc"));
            }
        }
    }

    ~wxItemIdList()
    {
        Free(m_pidl);
    }

    // implicit conversion to LPITEMIDLIST
    operator LPITEMIDLIST() const { return m_pidl; }

    // get the corresponding path, returns empty string on error
    wxString GetPath() const
    {
        wxString path;
        if ( !SHGetPathFromIDList(m_pidl, wxStringBuffer(path, MAX_PATH)) )
        {
            wxLogLastError(_T("SHGetPathFromIDList"));
        }

        return path;
    }

private:
    LPITEMIDLIST m_pidl;

    DECLARE_NO_COPY_CLASS(wxItemIdList)
};

// enable autocompleting filenames in the text control with given HWND
//
// this only works on systems with shlwapi.dll 5.0 or later
//
// implemented in src/msw/utilsgui.cpp
extern bool wxEnableFileNameAutoComplete(HWND hwnd);

#endif // _WX_MSW_WRAPSHL_H_

