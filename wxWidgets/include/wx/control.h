/////////////////////////////////////////////////////////////////////////////
// Name:        wx/control.h
// Purpose:     wxControl common interface
// Author:      Vadim Zeitlin
// Modified by:
// Created:     26.07.99
// RCS-ID:      $Id: control.h 42816 2006-10-31 08:50:17Z RD $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CONTROL_H_BASE_
#define _WX_CONTROL_H_BASE_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/defs.h"

#if wxUSE_CONTROLS

#include "wx/window.h"      // base class

extern WXDLLEXPORT_DATA(const wxChar) wxControlNameStr[];

// ----------------------------------------------------------------------------
// wxControl is the base class for all controls
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxControlBase : public wxWindow
{
public:
    wxControlBase() { }

    virtual ~wxControlBase();

    // Create() function adds the validator parameter
    bool Create(wxWindow *parent, wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxControlNameStr);

    // get the control alignment (left/right/centre, top/bottom/centre)
    int GetAlignment() const { return m_windowStyle & wxALIGN_MASK; }

    // get the string without mnemonic characters ('&')
    static wxString GetLabelText(const wxString& label);

    // get just the text of the label, without mnemonic characters ('&')
    wxString GetLabelText() const { return GetLabelText(GetLabel()); }

    // controls by default inherit the colours of their parents, if a
    // particular control class doesn't want to do it, it can override
    // ShouldInheritColours() to return false
    virtual bool ShouldInheritColours() const { return true; }


    // WARNING: this doesn't work for all controls nor all platforms!
    //
    // simulates the event of given type (i.e. wxButton::Command() is just as
    // if the button was clicked)
    virtual void Command(wxCommandEvent &event);

    virtual void SetLabel( const wxString &label );
    virtual bool SetFont(const wxFont& font);

    // wxControl-specific processing after processing the update event
    virtual void DoUpdateWindowUI(wxUpdateUIEvent& event);

protected:
    // creates the control (calls wxWindowBase::CreateBase inside) and adds it
    // to the list of parents children
    bool CreateControl(wxWindowBase *parent,
                       wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style,
                       const wxValidator& validator,
                       const wxString& name);

    // initialize the common fields of wxCommandEvent
    void InitCommandEvent(wxCommandEvent& event) const;

    DECLARE_NO_COPY_CLASS(wxControlBase)
};

// ----------------------------------------------------------------------------
// include platform-dependent wxControl declarations
// ----------------------------------------------------------------------------

#if defined(__WXUNIVERSAL__)
    #include "wx/univ/control.h"
#elif defined(__WXPALMOS__)
    #include "wx/palmos/control.h"
#elif defined(__WXMSW__)
    #include "wx/msw/control.h"
#elif defined(__WXMOTIF__)
    #include "wx/motif/control.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/control.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/control.h"
#elif defined(__WXMAC__)
    #include "wx/mac/control.h"
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/control.h"
#elif defined(__WXPM__)
    #include "wx/os2/control.h"
#endif

#endif // wxUSE_CONTROLS

#endif
    // _WX_CONTROL_H_BASE_
