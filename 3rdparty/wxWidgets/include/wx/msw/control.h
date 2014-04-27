/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/control.h
// Purpose:     wxControl class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: control.h 45498 2007-04-16 13:03:05Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CONTROL_H_
#define _WX_CONTROL_H_

#include "wx/dynarray.h"

// General item class
class WXDLLEXPORT wxControl : public wxControlBase
{
public:
    wxControl() { }

    wxControl(wxWindow *parent, wxWindowID id,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize, long style = 0,
              const wxValidator& validator = wxDefaultValidator,
              const wxString& name = wxControlNameStr)
    {
        Create(parent, id, pos, size, style, validator, name);
    }

    bool Create(wxWindow *parent, wxWindowID id,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize, long style = 0,
            const wxValidator& validator = wxDefaultValidator,
            const wxString& name = wxControlNameStr);

    virtual ~wxControl();

    // Simulates an event
    virtual void Command(wxCommandEvent& event) { ProcessCommand(event); }


    // implementation from now on
    // --------------------------

    virtual wxVisualAttributes GetDefaultAttributes() const
    {
        return GetClassDefaultAttributes(GetWindowVariant());
    }

    static wxVisualAttributes
    GetClassDefaultAttributes(wxWindowVariant variant = wxWINDOW_VARIANT_NORMAL);

    // Calls the callback and appropriate event handlers
    bool ProcessCommand(wxCommandEvent& event);

    // MSW-specific
    virtual bool MSWOnNotify(int idCtrl, WXLPARAM lParam, WXLPARAM *result);

    // For ownerdraw items
    virtual bool MSWOnDraw(WXDRAWITEMSTRUCT *WXUNUSED(item)) { return false; }
    virtual bool MSWOnMeasure(WXMEASUREITEMSTRUCT *WXUNUSED(item)) { return false; }

    const wxArrayLong& GetSubcontrols() const { return m_subControls; }

    // default handling of WM_CTLCOLORxxx: this is public so that wxWindow
    // could call it
    virtual WXHBRUSH MSWControlColor(WXHDC pDC, WXHWND hWnd);

    // default style for the control include WS_TABSTOP if it AcceptsFocus()
    virtual WXDWORD MSWGetStyle(long style, WXDWORD *exstyle) const;

protected:
    // choose the default border for this window
    virtual wxBorder GetDefaultBorder() const;

    // return default best size (doesn't really make any sense, override this)
    virtual wxSize DoGetBestSize() const;

    // This is a helper for all wxControls made with UPDOWN native control.
    // In wxMSW it was only wxSpinCtrl derived from wxSpinButton but in
    // WinCE of Smartphones this happens also for native wxTextCtrl,
    // wxChoice and others.
    virtual wxSize GetBestSpinnerSize(const bool is_vertical) const;

    // create the control of the given Windows class: this is typically called
    // from Create() method of the derived class passing its label, pos and
    // size parameter (style parameter is not needed because m_windowStyle is
    // supposed to had been already set and so is used instead when this
    // function is called)
    bool MSWCreateControl(const wxChar *classname,
                          const wxString& label,
                          const wxPoint& pos,
                          const wxSize& size);

    // NB: the method below is deprecated now, with MSWGetStyle() the method
    //     above should be used instead! Once all the controls are updated to
    //     implement MSWGetStyle() this version will disappear.
    //
    // create the control of the given class with the given style (combination
    // of WS_XXX flags, i.e. Windows style, not wxWidgets one), returns
    // false if creation failed
    //
    // All parameters except classname and style are optional, if the
    // size/position are not given, they should be set later with SetSize()
    // and, label (the title of the window), of course, is left empty. The
    // extended style is determined from the style and the app 3D settings
    // automatically if it's not specified explicitly.
    bool MSWCreateControl(const wxChar *classname,
                          WXDWORD style,
                          const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxDefaultSize,
                          const wxString& label = wxEmptyString,
                          WXDWORD exstyle = (WXDWORD)-1);

    // call this from the derived class MSWControlColor() if you want to show
    // the control greyed out (and opaque)
    WXHBRUSH MSWControlColorDisabled(WXHDC pDC);

    // common part of the 3 functions above: pass wxNullColour to use the
    // appropriate background colour (meaning ours or our parents) or a fixed
    // one
    virtual WXHBRUSH DoMSWControlColor(WXHDC pDC, wxColour colBg, WXHWND hWnd);

    // this is a helper for the derived class GetClassDefaultAttributes()
    // implementation: it returns the right colours for the classes which
    // contain something else (e.g. wxListBox, wxTextCtrl, ...) instead of
    // being simple controls (such as wxButton, wxCheckBox, ...)
    static wxVisualAttributes
        GetCompositeControlsDefaultAttributes(wxWindowVariant variant);

    // for controls like radiobuttons which are really composite this array
    // holds the ids (not HWNDs!) of the sub controls
    wxArrayLong m_subControls;

private:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxControl)
};

#endif // _WX_CONTROL_H_
