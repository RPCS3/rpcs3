/////////////////////////////////////////////////////////////////////////////
// Name:        radiobut.h
// Purpose:     wxRadioButton class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: radiobut.h 41144 2006-09-10 23:08:13Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_RADIOBUT_H_
#define _WX_RADIOBUT_H_

class WXDLLEXPORT wxRadioButton: public wxControl
{
public:
    // ctors and creation functions
    wxRadioButton() { Init(); }

    wxRadioButton(wxWindow *parent,
                  wxWindowID id,
                  const wxString& label,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize,
                  long style = 0,
                  const wxValidator& validator = wxDefaultValidator,
                  const wxString& name = wxRadioButtonNameStr)
    {
        Init();

        Create(parent, id, label, pos, size, style, validator, name);
    }

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxString& label,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxRadioButtonNameStr);

    // implement the radio button interface
    virtual void SetValue(bool value);
    virtual bool GetValue() const;

    // implementation only from now on
    virtual bool MSWCommand(WXUINT param, WXWORD id);
    virtual void Command(wxCommandEvent& event);
    virtual bool HasTransparentBackground() { return true; }

    virtual WXDWORD MSWGetStyle(long style, WXDWORD *exstyle) const;

protected:
    virtual wxSize DoGetBestSize() const;

private:
    // common part of all ctors
    void Init();

    // we need to store the state internally as the result of GetValue()
    // sometimes gets out of sync in WM_COMMAND handler
    bool m_isChecked;

    DECLARE_DYNAMIC_CLASS_NO_COPY(wxRadioButton)
};

#endif
    // _WX_RADIOBUT_H_
