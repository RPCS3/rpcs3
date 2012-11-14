/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/checkbox.h
// Purpose:     wxCheckBox class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: checkbox.h 37393 2006-02-08 21:47:09Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CHECKBOX_H_
#define _WX_CHECKBOX_H_

// Checkbox item (single checkbox)
class WXDLLEXPORT wxCheckBox : public wxCheckBoxBase
{
public:
    wxCheckBox() { }
    wxCheckBox(wxWindow *parent,
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
    virtual bool GetValue() const;

    // override some base class virtuals
    virtual bool MSWCommand(WXUINT param, WXWORD id);
    virtual void Command(wxCommandEvent& event);
    virtual bool SetForegroundColour(const wxColour& colour);
    virtual bool MSWOnDraw(WXDRAWITEMSTRUCT *item);

protected:
    virtual wxSize DoGetBestSize() const;

    virtual void DoSet3StateValue(wxCheckBoxState value);
    virtual wxCheckBoxState DoGet3StateValue() const;

    // make the checkbox owner drawn or reset it to normal style
    void MakeOwnerDrawn(bool ownerDrawn);

    // return true if this checkbox is owner drawn
    bool IsOwnerDrawn() const;

private:
    // common part of all ctors
    void Init();

    // event handlers used by owner-drawn checkbox
    void OnMouseEnterOrLeave(wxMouseEvent& event);
    void OnMouseLeft(wxMouseEvent& event);
    void OnFocus(wxFocusEvent& event);


    // current state of the checkbox
    wxCheckBoxState m_state;

    // true if the checkbox is currently pressed
    bool m_isPressed;

    // true if mouse is currently over the control
    bool m_isHot;


    DECLARE_DYNAMIC_CLASS_NO_COPY(wxCheckBox)
};

#endif
    // _WX_CHECKBOX_H_
