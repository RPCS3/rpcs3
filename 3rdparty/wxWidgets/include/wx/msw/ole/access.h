///////////////////////////////////////////////////////////////////////////////
// Name:        ole/access.h
// Purpose:     declaration of the wxAccessible class
// Author:      Julian Smart
// Modified by:
// Created:     2003-02-12
// RCS-ID:      $Id: access.h 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) 2003 Julian Smart
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef   _WX_ACCESS_H_
#define   _WX_ACCESS_H_

#if wxUSE_ACCESSIBILITY

// ----------------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------------

class wxIAccessible;
class WXDLLEXPORT wxWindow;

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// wxAccessible implements accessibility behaviour.
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxAccessible : public wxAccessibleBase
{
public:
    wxAccessible(wxWindow *win = NULL);
    virtual ~wxAccessible();

// Overridables

// Accessors

    // Returns the wxIAccessible pointer
    wxIAccessible* GetIAccessible() { return m_pIAccessible; }

    // Returns the IAccessible standard interface pointer
    void* GetIAccessibleStd() ;

// Operations

    // Sends an event when something changes in an accessible object.
    static void NotifyEvent(int eventType, wxWindow* window, wxAccObject objectType,
                            int objectId);

protected:
    void Init();

private:
    wxIAccessible * m_pIAccessible;  // the pointer to COM interface
    void*           m_pIAccessibleStd;  // the pointer to the standard COM interface,
                                        // for default processing

    DECLARE_NO_COPY_CLASS(wxAccessible)
};

#endif  //wxUSE_ACCESSIBILITY

#endif  //_WX_ACCESS_H_

