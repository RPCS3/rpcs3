/////////////////////////////////////////////////////////////////////////////
// Name:        automtn.h
// Purpose:     OLE automation utilities
// Author:      Julian Smart
// Modified by:
// Created:     11/6/98
// RCS-ID:      $Id: automtn.h 45498 2007-04-16 13:03:05Z VZ $
// Copyright:   (c) 1998, Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_AUTOMTN_H_
#define _WX_AUTOMTN_H_

#include "wx/defs.h"

#if wxUSE_OLE_AUTOMATION

#include "wx/object.h"
#include "wx/variant.h"

typedef void            WXIDISPATCH;
typedef unsigned short* WXBSTR;

#ifdef GetObject
#undef GetObject
#endif

/*
 * wxAutomationObject
 * Wraps up an IDispatch pointer and invocation; does variant conversion.
 */

class WXDLLEXPORT wxAutomationObject: public wxObject
{
public:
    wxAutomationObject(WXIDISPATCH* dispatchPtr = NULL);
    virtual ~wxAutomationObject();

    // Set/get dispatch pointer
    inline void SetDispatchPtr(WXIDISPATCH* dispatchPtr) { m_dispatchPtr = dispatchPtr; }
    inline WXIDISPATCH* GetDispatchPtr() const { return m_dispatchPtr; }

    // Get a dispatch pointer from the current object associated
    // with a class id, such as "Excel.Application"
    bool GetInstance(const wxString& classId) const;

    // Get a dispatch pointer from a new instance of the the class
    bool CreateInstance(const wxString& classId) const;

    // Low-level invocation function. Pass either an array of variants,
    // or an array of pointers to variants.
    bool Invoke(const wxString& member, int action,
        wxVariant& retValue, int noArgs, wxVariant args[], const wxVariant* ptrArgs[] = 0) const;

    // Invoke a member function
    wxVariant CallMethod(const wxString& method, int noArgs, wxVariant args[]);
    wxVariant CallMethodArray(const wxString& method, int noArgs, const wxVariant **args);

    // Convenience function
    wxVariant CallMethod(const wxString& method,
        const wxVariant& arg1 = wxNullVariant, const wxVariant& arg2 = wxNullVariant,
        const wxVariant& arg3 = wxNullVariant, const wxVariant& arg4 = wxNullVariant,
        const wxVariant& arg5 = wxNullVariant, const wxVariant& arg6 = wxNullVariant);

    // Get/Put property
    wxVariant GetProperty(const wxString& property, int noArgs = 0, wxVariant args[] = (wxVariant*) NULL) const;
    wxVariant GetPropertyArray(const wxString& property, int noArgs, const wxVariant **args) const;
    wxVariant GetProperty(const wxString& property,
        const wxVariant& arg1, const wxVariant& arg2 = wxNullVariant,
        const wxVariant& arg3 = wxNullVariant, const wxVariant& arg4 = wxNullVariant,
        const wxVariant& arg5 = wxNullVariant, const wxVariant& arg6 = wxNullVariant);

    bool PutPropertyArray(const wxString& property, int noArgs, const wxVariant **args);
    bool PutProperty(const wxString& property, int noArgs, wxVariant args[]) ;
    bool PutProperty(const wxString& property,
        const wxVariant& arg1, const wxVariant& arg2 = wxNullVariant,
        const wxVariant& arg3 = wxNullVariant, const wxVariant& arg4 = wxNullVariant,
        const wxVariant& arg5 = wxNullVariant, const wxVariant& arg6 = wxNullVariant);

    // Uses DISPATCH_PROPERTYGET
    // and returns a dispatch pointer. The calling code should call Release
    // on the pointer, though this could be implicit by constructing an wxAutomationObject
    // with it and letting the destructor call Release.
    WXIDISPATCH* GetDispatchProperty(const wxString& property, int noArgs, wxVariant args[]) const;
    WXIDISPATCH* GetDispatchProperty(const wxString& property, int noArgs, const wxVariant **args) const;

    // A way of initialising another wxAutomationObject with a dispatch object,
    // without having to deal with nasty IDispatch pointers.
    bool GetObject(wxAutomationObject& obj, const wxString& property, int noArgs = 0, wxVariant args[] = (wxVariant*) NULL) const;
    bool GetObject(wxAutomationObject& obj, const wxString& property, int noArgs, const wxVariant **args) const;

public:
    WXIDISPATCH*  m_dispatchPtr;

    DECLARE_NO_COPY_CLASS(wxAutomationObject)
};

#endif // wxUSE_OLE_AUTOMATION

#endif // _WX_AUTOMTN_H_
