///////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/datectrl.h
// Purpose:     wxDatePickerCtrl for Windows
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2005-01-09
// RCS-ID:      $Id: datectrl.h 42207 2006-10-21 16:29:33Z VZ $
// Copyright:   (c) 2005 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_DATECTRL_H_
#define _WX_MSW_DATECTRL_H_

// ----------------------------------------------------------------------------
// wxDatePickerCtrl
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxDatePickerCtrl : public wxDatePickerCtrlBase
{
public:
    // ctors
    wxDatePickerCtrl() { }

    wxDatePickerCtrl(wxWindow *parent,
                     wxWindowID id,
                     const wxDateTime& dt = wxDefaultDateTime,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxDP_DEFAULT | wxDP_SHOWCENTURY,
                     const wxValidator& validator = wxDefaultValidator,
                     const wxString& name = wxDatePickerCtrlNameStr)
    {
        Create(parent, id, dt, pos, size, style, validator, name);
    }

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxDateTime& dt = wxDefaultDateTime,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDP_DEFAULT | wxDP_SHOWCENTURY,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxDatePickerCtrlNameStr);

    // set/get the date
    virtual void SetValue(const wxDateTime& dt);
    virtual wxDateTime GetValue() const;

    // set/get the allowed valid range for the dates, if either/both of them
    // are invalid, there is no corresponding limit and if neither is set
    // GetRange() returns false
    virtual void SetRange(const wxDateTime& dt1, const wxDateTime& dt2);
    virtual bool GetRange(wxDateTime *dt1, wxDateTime *dt2) const;

    virtual WXDWORD MSWGetStyle(long style, WXDWORD *exstyle) const;

    virtual bool MSWOnNotify(int idCtrl, WXLPARAM lParam, WXLPARAM *result);

protected:
    virtual wxSize DoGetBestSize() const;

    // the date currently shown by the control, may be invalid
    wxDateTime m_date;


    DECLARE_DYNAMIC_CLASS_NO_COPY(wxDatePickerCtrl)
};

#endif // _WX_MSW_DATECTRL_H_
