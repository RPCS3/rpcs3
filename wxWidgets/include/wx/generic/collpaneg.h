/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/collpaneg.h
// Purpose:     wxGenericCollapsiblePane
// Author:      Francesco Montorsi
// Modified by:
// Created:     8/10/2006
// RCS-ID:      $Id: collpaneg.h 49804 2007-11-10 01:09:42Z VZ $
// Copyright:   (c) Francesco Montorsi
// Licence:     wxWindows Licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_COLLAPSABLE_PANE_H_GENERIC_
#define _WX_COLLAPSABLE_PANE_H_GENERIC_

// forward declared
class WXDLLIMPEXP_FWD_CORE wxButton;
class WXDLLIMPEXP_FWD_CORE wxStaticLine;

// class name
extern WXDLLIMPEXP_DATA_CORE(const wxChar) wxCollapsiblePaneNameStr[];

// ----------------------------------------------------------------------------
// wxGenericCollapsiblePane
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxGenericCollapsiblePane : public wxCollapsiblePaneBase
{
public:
    wxGenericCollapsiblePane() { Init(); }

    wxGenericCollapsiblePane(wxWindow *parent,
                        wxWindowID winid,
                        const wxString& label,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        long style = wxCP_DEFAULT_STYLE,
                        const wxValidator& val = wxDefaultValidator,
                        const wxString& name = wxCollapsiblePaneNameStr)
    {
        Init();

        Create(parent, winid, label, pos, size, style, val, name);
    }

    void Init()
    {
        m_pButton = NULL;
        m_pPane = NULL;
        m_pStaticLine = NULL;
        m_sz = NULL;
    }

    ~wxGenericCollapsiblePane();

    bool Create(wxWindow *parent,
                wxWindowID winid,
                const wxString& label,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxCP_DEFAULT_STYLE,
                const wxValidator& val = wxDefaultValidator,
                const wxString& name = wxCollapsiblePaneNameStr);

    // public wxCollapsiblePane API
    virtual void Collapse(bool collapse = true);
    virtual void SetLabel(const wxString &label);

    virtual bool IsCollapsed() const
        { return m_pPane==NULL || !m_pPane->IsShown(); }
    virtual wxWindow *GetPane() const
        { return m_pPane; }
    virtual wxString GetLabel() const
        { return m_strLabel; }

    virtual bool Layout();

    // implementation only, don't use
    void OnStateChange(const wxSize& sizeNew);

protected:
    // overridden methods
    virtual wxSize DoGetBestSize() const;

    wxString GetBtnLabel() const;
    int GetBorder() const;

    // child controls
    wxButton *m_pButton;
    wxStaticLine *m_pStaticLine;
    wxWindow *m_pPane;
    wxSizer *m_sz;

    // the button label without ">>" or "<<"
    wxString m_strLabel;

private:
    // event handlers
    void OnButton(wxCommandEvent &ev);
    void OnSize(wxSizeEvent &ev);

    DECLARE_DYNAMIC_CLASS(wxGenericCollapsiblePane)
    DECLARE_EVENT_TABLE()
};

#endif // _WX_COLLAPSABLE_PANE_H_GENERIC_
