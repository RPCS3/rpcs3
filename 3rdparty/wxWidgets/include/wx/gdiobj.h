/////////////////////////////////////////////////////////////////////////////
// Name:        wx/gdiobj.h
// Purpose:     wxGDIObject base header
// Author:      Julian Smart
// Modified by:
// Created:
// Copyright:   (c) Julian Smart
// RCS-ID:      $Id: gdiobj.h 42211 2006-10-21 17:19:11Z SN $
// Licence:     wxWindows Licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GDIOBJ_H_BASE_
#define _WX_GDIOBJ_H_BASE_

#include "wx/object.h"

// ----------------------------------------------------------------------------
// wxGDIRefData is the base class for wxXXXData structures which contain the
// real data for the GDI object and are shared among all wxWin objects sharing
// the same native GDI object
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxGDIRefData: public wxObjectRefData { };

// ----------------------------------------------------------------------------
// wxGDIObject
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxGDIObject: public wxObject
{
public:
    bool IsNull() const { return m_refData == NULL; }

#if defined(__WXMSW__) || defined(__WXPM__) || defined(__WXPALMOS__)
    // Creates the resource
    virtual bool RealizeResource() { return false; }

    // Frees the resource
    virtual bool FreeResource(bool WXUNUSED(force) = false) { return false; }

    virtual bool IsFree() const { return false; }

    // Returns handle.
    virtual WXHANDLE GetResourceHandle() const { return 0; }
#endif // defined(__WXMSW__) || defined(__WXPM__)

    DECLARE_DYNAMIC_CLASS(wxGDIObject)
};

#endif
    // _WX_GDIOBJ_H_BASE_
