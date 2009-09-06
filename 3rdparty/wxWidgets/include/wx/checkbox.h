///////////////////////////////////////////////////////////////////////////////
// Name:        wx/checkbox.h
// Purpose:     wxCheckBox class interface
// Author:      Vadim Zeitlin
// Modified by:
// Created:     07.09.00
// RCS-ID:      $Id: checkbox.h 39901 2006-06-30 10:51:44Z VS $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CHECKBOX_H_BASE_
#define _WX_CHECKBOX_H_BASE_

#include "wx/defs.h"

#if wxUSE_CHECKBOX

#include "wx/control.h"


/*
 * wxCheckBox style flags
 * (Using wxCHK_* because wxCB_* is used by wxComboBox).
 * Determine whether to use a 3-state or 2-state
 * checkbox. 3-state enables to differentiate
 * between 'unchecked', 'checked' and 'undetermined'.
 */
#define wxCHK_2STATE           0x0000
#define wxCHK_3STATE           0x1000

/*
 * If this style is set the user can set the checkbox to the
 * undetermined state. If not set the undetermined set can only
 * be set programmatically.
 * This style can only be used with 3 state checkboxes.
 */
#define wxCHK_ALLOW_3RD_STATE_FOR_USER 0x2000

/*
 * The possible states of a 3-state checkbox (Compatible
 * with the 2-state checkbox).
 */
enum wxCheckBoxState
{
    wxCHK_UNCHECKED,
    wxCHK_CHECKED,
    wxCHK_UNDETERMINED /* 3-state checkbox only */
};


extern WXDLLEXPORT_DATA(const wxChar) wxCheckBoxNameStr[];

// ----------------------------------------------------------------------------
// wxCheckBox: a control which shows a label and a box which may be checked
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxCheckBoxBase : public wxControl
{
public:
    wxCheckBoxBase() { }

    // set/get the checked status of the listbox
    virtual void SetValue(bool value) = 0;
    virtual bool GetValue() const = 0;

    bool IsChecked() const
    {
        wxASSERT_MSG( !Is3State(), wxT("Calling IsChecked() doesn't make sense for")
            wxT(" a three state checkbox, Use Get3StateValue() instead") );

        return GetValue();
    }

    wxCheckBoxState Get3StateValue() const
    {
        wxCheckBoxState state = DoGet3StateValue();

        if ( state == wxCHK_UNDETERMINED && !Is3State() )
        {
            // Undetermined state with a 2-state checkbox??
            wxFAIL_MSG( wxT("DoGet3StateValue() says the 2-state checkbox is ")
                wxT("in an undetermined/third state") );

            state = wxCHK_UNCHECKED;
        }

        return state;
    }

    void Set3StateValue(wxCheckBoxState state)
    {
        if ( state == wxCHK_UNDETERMINED && !Is3State() )
        {
            wxFAIL_MSG(wxT("Setting a 2-state checkbox to undetermined state"));
            state = wxCHK_UNCHECKED;
        }

        DoSet3StateValue(state);
    }

    bool Is3State() const { return HasFlag(wxCHK_3STATE); }

    bool Is3rdStateAllowedForUser() const
    {
        return HasFlag(wxCHK_ALLOW_3RD_STATE_FOR_USER);
    }

    virtual bool HasTransparentBackground() { return true; }

    // wxCheckBox-specific processing after processing the update event
    virtual void DoUpdateWindowUI(wxUpdateUIEvent& event)
    {
        wxControl::DoUpdateWindowUI(event);

        if ( event.GetSetChecked() )
            SetValue(event.GetChecked());
    }

protected:
    virtual void DoSet3StateValue(wxCheckBoxState WXUNUSED(state)) { wxFAIL; }

    virtual wxCheckBoxState DoGet3StateValue() const
    {
        wxFAIL;
        return wxCHK_UNCHECKED;
    }

private:
    DECLARE_NO_COPY_CLASS(wxCheckBoxBase)
};

#if defined(__WXUNIVERSAL__)
    #include "wx/univ/checkbox.h"
#elif defined(__WXMSW__)
    #include "wx/msw/checkbox.h"
#elif defined(__WXMOTIF__)
    #include "wx/motif/checkbox.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/checkbox.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/checkbox.h"
#elif defined(__WXMAC__)
    #include "wx/mac/checkbox.h"
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/checkbox.h"
#elif defined(__WXPM__)
    #include "wx/os2/checkbox.h"
#elif defined(__WXPALMOS__)
    #include "wx/palmos/checkbox.h"
#endif

#endif // wxUSE_CHECKBOX

#endif
    // _WX_CHECKBOX_H_BASE_
