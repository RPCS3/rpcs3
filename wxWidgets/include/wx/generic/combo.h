/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/combo.h
// Purpose:     Generic wxComboCtrl
// Author:      Jaakko Salli
// Modified by:
// Created:     Apr-30-2006
// RCS-ID:      $Id: combo.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) Jaakko Salli
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_COMBOCTRL_H_
#define _WX_GENERIC_COMBOCTRL_H_

#if wxUSE_COMBOCTRL

// Only define generic if native doesn't have all the features
#if !defined(wxCOMBOCONTROL_FULLY_FEATURED)

// ----------------------------------------------------------------------------
// Generic wxComboCtrl
// ----------------------------------------------------------------------------

#if defined(__WXUNIVERSAL__)

// all actions of single line text controls are supported

// popup/dismiss the choice window
#define wxACTION_COMBOBOX_POPUP     wxT("popup")
#define wxACTION_COMBOBOX_DISMISS   wxT("dismiss")

#endif

extern WXDLLIMPEXP_DATA_CORE(const wxChar) wxComboBoxNameStr[];

class WXDLLEXPORT wxGenericComboCtrl : public wxComboCtrlBase
{
public:
    // ctors and such
    wxGenericComboCtrl() : wxComboCtrlBase() { Init(); }

    wxGenericComboCtrl(wxWindow *parent,
                       wxWindowID id = wxID_ANY,
                       const wxString& value = wxEmptyString,
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& size = wxDefaultSize,
                       long style = 0,
                       const wxValidator& validator = wxDefaultValidator,
                       const wxString& name = wxComboBoxNameStr)
        : wxComboCtrlBase()
    {
        Init();

        (void)Create(parent, id, value, pos, size, style, validator, name);
    }

    bool Create(wxWindow *parent,
                wxWindowID id = wxID_ANY,
                const wxString& value = wxEmptyString,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxComboBoxNameStr);

    virtual ~wxGenericComboCtrl();

    void SetCustomPaintWidth( int width );

    virtual bool IsKeyPopupToggle(const wxKeyEvent& event) const;

    static int GetFeatures() { return wxComboCtrlFeatures::All; }

#if defined(__WXUNIVERSAL__)
    // we have our own input handler and our own actions
    virtual bool PerformAction(const wxControlAction& action,
                               long numArg = 0l,
                               const wxString& strArg = wxEmptyString);
#endif

protected:

    // Mandatory virtuals
    virtual void OnResize();

    // Event handlers
    void OnPaintEvent( wxPaintEvent& event );
    void OnMouseEvent( wxMouseEvent& event );

private:
    void Init();

    DECLARE_EVENT_TABLE()

    DECLARE_DYNAMIC_CLASS(wxGenericComboCtrl)
};


#ifndef _WX_COMBOCONTROL_H_

// If native wxComboCtrl was not defined, then prepare a simple
// front-end so that wxRTTI works as expected.

class WXDLLEXPORT wxComboCtrl : public wxGenericComboCtrl
{
public:
    wxComboCtrl() : wxGenericComboCtrl() {}

    wxComboCtrl(wxWindow *parent,
                wxWindowID id = wxID_ANY,
                const wxString& value = wxEmptyString,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxComboBoxNameStr)
        : wxGenericComboCtrl()
    {
        (void)Create(parent, id, value, pos, size, style, validator, name);
    }

    virtual ~wxComboCtrl() {}

protected:

private:
    DECLARE_DYNAMIC_CLASS(wxComboCtrl)
};

#endif // _WX_COMBOCONTROL_H_

#else

#define wxGenericComboCtrl   wxComboCtrl

#endif // !defined(wxCOMBOCONTROL_FULLY_FEATURED)

#endif // wxUSE_COMBOCTRL
#endif
    // _WX_GENERIC_COMBOCTRL_H_
