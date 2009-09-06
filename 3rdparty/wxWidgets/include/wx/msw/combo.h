/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/combo.h
// Purpose:     wxComboCtrl class
// Author:      Jaakko Salli
// Modified by:
// Created:     Apr-30-2006
// RCS-ID:      $Id: combo.h 43881 2006-12-09 19:48:21Z PC $
// Copyright:   (c) Jaakko Salli
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_COMBOCONTROL_H_
#define _WX_COMBOCONTROL_H_

// NB: Definition of _WX_COMBOCONTROL_H_ is used in wx/generic/combo.h to
//     determine whether there is native wxComboCtrl, so make sure you
//     use it in all native wxComboCtrls.

#if wxUSE_COMBOCTRL

#if !defined(__WXWINCE__) && wxUSE_TIMER
    #include "wx/timer.h"
    #define wxUSE_COMBOCTRL_POPUP_ANIMATION     1
#else
    #define wxUSE_COMBOCTRL_POPUP_ANIMATION     0
#endif


// ----------------------------------------------------------------------------
// Native wxComboCtrl
// ----------------------------------------------------------------------------

// Define this only if native implementation includes all features
#define wxCOMBOCONTROL_FULLY_FEATURED

extern WXDLLIMPEXP_DATA_CORE(const wxChar) wxComboBoxNameStr[];

class WXDLLEXPORT wxComboCtrl : public wxComboCtrlBase
{
public:
    // ctors and such
    wxComboCtrl() : wxComboCtrlBase() { Init(); }

    wxComboCtrl(wxWindow *parent,
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

    virtual ~wxComboCtrl();

    virtual void PrepareBackground( wxDC& dc, const wxRect& rect, int flags ) const;
    virtual bool IsKeyPopupToggle(const wxKeyEvent& event) const;

    static int GetFeatures() { return wxComboCtrlFeatures::All; }

#if wxUSE_COMBOCTRL_POPUP_ANIMATION
    void OnTimerEvent( wxTimerEvent& event );
protected:
    virtual bool AnimateShow( const wxRect& rect, int flags );
#endif

protected:

    // customization
    virtual void OnResize();
    virtual wxCoord GetNativeTextIndent() const;
    virtual void OnThemeChange();

    // event handlers
    void OnPaintEvent( wxPaintEvent& event );
    void OnMouseEvent( wxMouseEvent& event );

private:
    void Init();

#if wxUSE_COMBOCTRL_POPUP_ANIMATION
    // Popup animation related
    wxLongLong  m_animStart;
    wxTimer     m_animTimer;
    wxRect      m_animRect;
    int         m_animFlags;
#endif

    DECLARE_EVENT_TABLE()

    DECLARE_DYNAMIC_CLASS(wxComboCtrl)
};


#endif // wxUSE_COMBOCTRL
#endif
    // _WX_COMBOCONTROL_H_
