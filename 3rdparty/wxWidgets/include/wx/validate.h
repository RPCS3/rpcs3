/////////////////////////////////////////////////////////////////////////////
// Name:        wx/validate.h
// Purpose:     wxValidator class
// Author:      Julian Smart
// Modified by:
// Created:     29/01/98
// RCS-ID:      $Id: validate.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) 1998 Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_VALIDATE_H_
#define _WX_VALIDATE_H_

#include "wx/defs.h"

#if wxUSE_VALIDATORS

#include "wx/event.h"

class WXDLLIMPEXP_FWD_CORE wxWindow;
class WXDLLIMPEXP_FWD_CORE wxWindowBase;

/*
 A validator has up to three purposes:

 1) To validate the data in the window that's associated
    with the validator.
 2) To transfer data to and from the window.
 3) To filter input, using its role as a wxEvtHandler
    to intercept e.g. OnChar.

 Note that wxValidator and derived classes use reference counting.
*/

class WXDLLEXPORT wxValidator : public wxEvtHandler
{
public:
    wxValidator();
    virtual ~wxValidator();

    // Make a clone of this validator (or return NULL) - currently necessary
    // if you're passing a reference to a validator.
    // Another possibility is to always pass a pointer to a new validator
    // (so the calling code can use a copy constructor of the relevant class).
    virtual wxObject *Clone() const
        { return (wxValidator *)NULL; }
    bool Copy(const wxValidator& val)
        { m_validatorWindow = val.m_validatorWindow; return true; }

    // Called when the value in the window must be validated.
    // This function can pop up an error message.
    virtual bool Validate(wxWindow *WXUNUSED(parent)) { return false; }

    // Called to transfer data to the window
    virtual bool TransferToWindow() { return false; }

    // Called to transfer data from the window
    virtual bool TransferFromWindow() { return false; }

    // accessors
    wxWindow *GetWindow() const { return (wxWindow *)m_validatorWindow; }
    void SetWindow(wxWindowBase *win) { m_validatorWindow = win; }

    // validators beep by default if invalid key is pressed, these functions
    // allow to change it
    static bool IsSilent() { return ms_isSilent; }
    static void SetBellOnError(bool doIt = true) { ms_isSilent = doIt; }

protected:
    wxWindowBase *m_validatorWindow;

private:
    static bool ms_isSilent;

    DECLARE_DYNAMIC_CLASS(wxValidator)
    DECLARE_NO_COPY_CLASS(wxValidator)
};

extern WXDLLEXPORT_DATA(const wxValidator) wxDefaultValidator;

#define wxVALIDATOR_PARAM(val) val

#else // !wxUSE_VALIDATORS
    // wxWidgets is compiled without support for wxValidator, but we still
    // want to be able to pass wxDefaultValidator to the functions which take
    // a wxValidator parameter to avoid using "#if wxUSE_VALIDATORS"
    // everywhere
    class WXDLLEXPORT wxValidator;
    #define wxDefaultValidator (*((wxValidator *)NULL))

    // this macro allows to avoid warnings about unused parameters when
    // wxUSE_VALIDATORS == 0
    #define wxVALIDATOR_PARAM(val)
#endif // wxUSE_VALIDATORS/!wxUSE_VALIDATORS

#endif // _WX_VALIDATE_H_

