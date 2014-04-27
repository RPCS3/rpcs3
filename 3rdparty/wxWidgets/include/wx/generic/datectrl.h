/////////////////////////////////////////////////////////////////////////////
// Name:        generic/datectrl.h
// Purpose:     generic wxDatePickerCtrl implementation
// Author:      Andreas Pflug
// Modified by:
// Created:     2005-01-19
// RCS-ID:      $Id: datectrl.h 42539 2006-10-27 18:02:21Z RR $
// Copyright:   (c) 2005 Andreas Pflug <pgadmin@pse-consulting.de>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_DATECTRL_H_
#define _WX_GENERIC_DATECTRL_H_

class WXDLLIMPEXP_ADV wxCalendarDateAttr;
class WXDLLIMPEXP_ADV wxCalendarCtrl;
class WXDLLIMPEXP_ADV wxCalendarEvent;
class WXDLLIMPEXP_ADV wxComboCtrl;
class WXDLLIMPEXP_ADV wxCalendarComboPopup;

class WXDLLIMPEXP_ADV wxDatePickerCtrlGeneric : public wxDatePickerCtrlBase
{
public:
    // creating the control
    wxDatePickerCtrlGeneric() { Init(); }
    virtual ~wxDatePickerCtrlGeneric();
    wxDatePickerCtrlGeneric(wxWindow *parent,
                            wxWindowID id,
                            const wxDateTime& date = wxDefaultDateTime,
                            const wxPoint& pos = wxDefaultPosition,
                            const wxSize& size = wxDefaultSize,
                            long style = wxDP_DEFAULT | wxDP_SHOWCENTURY,
                            const wxValidator& validator = wxDefaultValidator,
                            const wxString& name = wxDatePickerCtrlNameStr)
    {
        Init();

        (void)Create(parent, id, date, pos, size, style, validator, name);
    }

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxDateTime& date = wxDefaultDateTime,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDP_DEFAULT | wxDP_SHOWCENTURY,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxDatePickerCtrlNameStr);

    // wxDatePickerCtrl methods
    void SetValue(const wxDateTime& date);
    wxDateTime GetValue() const;

    bool GetRange(wxDateTime *dt1, wxDateTime *dt2) const;
    void SetRange(const wxDateTime &dt1, const wxDateTime &dt2);

    bool SetDateRange(const wxDateTime& lowerdate = wxDefaultDateTime,
                      const wxDateTime& upperdate = wxDefaultDateTime);

    // extra methods available only in this (generic) implementation
    bool SetFormat(const wxChar *fmt);
    wxCalendarCtrl *GetCalendar() const { return m_cal; }


    // implementation only from now on
    // -------------------------------

    // overridden base class methods
    virtual bool Destroy();

protected:
    virtual wxSize DoGetBestSize() const;

private:
    void Init();

    void OnText(wxCommandEvent &event);
    void OnSize(wxSizeEvent& event);
    void OnFocus(wxFocusEvent& event);

    wxCalendarCtrl *m_cal;
    wxComboCtrl* m_combo;
    wxCalendarComboPopup* m_popup;


    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxDatePickerCtrlGeneric)
};

#endif // _WX_GENERIC_DATECTRL_H_

