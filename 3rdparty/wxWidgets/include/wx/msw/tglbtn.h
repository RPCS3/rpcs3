/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/tglbtn.h
// Purpose:     Declaration of the wxToggleButton class, which implements a
//              toggle button under wxMSW.
// Author:      John Norris, minor changes by Axel Schlueter
// Modified by:
// Created:     08.02.01
// RCS-ID:      $Id: tglbtn.h 37393 2006-02-08 21:47:09Z VZ $
// Copyright:   (c) 2000 Johnny C. Norris II
// License:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_TOGGLEBUTTON_H_
#define _WX_TOGGLEBUTTON_H_

extern WXDLLEXPORT_DATA(const wxChar) wxCheckBoxNameStr[];

// Checkbox item (single checkbox)
class WXDLLEXPORT wxToggleButton : public wxControl
{
public:
    wxToggleButton() {}
    wxToggleButton(wxWindow *parent,
                   wxWindowID id,
                   const wxString& label,
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize,
                   long style = 0,
                   const wxValidator& validator = wxDefaultValidator,
                   const wxString& name = wxCheckBoxNameStr)
    {
        Create(parent, id, label, pos, size, style, validator, name);
    }

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxString& label,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxCheckBoxNameStr);

    virtual void SetValue(bool value);
    virtual bool GetValue() const ;

    virtual bool MSWCommand(WXUINT param, WXWORD id);
    virtual void Command(wxCommandEvent& event);
    virtual WXDWORD MSWGetStyle(long flags, WXDWORD *exstyle = NULL) const;

protected:
    virtual wxSize DoGetBestSize() const;
    virtual wxBorder GetDefaultBorder() const;

private:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxToggleButton)
};

#endif // _WX_TOGGLEBUTTON_H_

