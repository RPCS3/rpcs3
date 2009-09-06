/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/wince/tbarwce.h
// Purpose:     Windows CE wxToolBar class
// Author:      Julian Smart
// Modified by:
// Created:     2003-07-12
// RCS-ID:      $Id: tbarwce.h 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_BARWCE_H_
#define _WX_BARWCE_H_

#if wxUSE_TOOLBAR

#include "wx/dynarray.h"

// Smartphones don't have toolbars, so use a dummy class
#ifdef __SMARTPHONE__

class WXDLLEXPORT wxToolBar : public wxToolBarBase
{
public:
    // ctors and dtor
    wxToolBar() { }

    wxToolBar(wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxNO_BORDER | wxTB_HORIZONTAL,
                const wxString& name = wxToolBarNameStr)
    {
        Create(parent, id, pos, size, style, name);
    }

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxNO_BORDER | wxTB_HORIZONTAL,
                const wxString& name = wxToolBarNameStr);

    // override/implement base class virtuals
    virtual wxToolBarToolBase *FindToolForPosition(wxCoord x, wxCoord y) const;
    virtual bool Realize() { return true; }

protected:
    // implement base class pure virtuals
    virtual bool DoInsertTool(size_t pos, wxToolBarToolBase *tool);
    virtual bool DoDeleteTool(size_t pos, wxToolBarToolBase *tool);

    virtual void DoEnableTool(wxToolBarToolBase *tool, bool enable);
    virtual void DoToggleTool(wxToolBarToolBase *tool, bool toggle);
    virtual void DoSetToggle(wxToolBarToolBase *tool, bool toggle);

    virtual wxToolBarToolBase *CreateTool(int id,
                                          const wxString& label,
                                          const wxBitmap& bmpNormal,
                                          const wxBitmap& bmpDisabled,
                                          wxItemKind kind,
                                          wxObject *clientData,
                                          const wxString& shortHelp,
                                          const wxString& longHelp);
    virtual wxToolBarToolBase *CreateTool(wxControl *control);

private:
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxToolBar)
    DECLARE_NO_COPY_CLASS(wxToolBar)
};

#else

// For __POCKETPC__

#include "wx/msw/tbar95.h"

class WXDLLEXPORT wxToolMenuBar : public wxToolBar
{
public:
    // ctors and dtor
    wxToolMenuBar() { Init(); }

    wxToolMenuBar(wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxNO_BORDER | wxTB_HORIZONTAL,
                const wxString& name = wxToolBarNameStr,
                wxMenuBar* menuBar = NULL)
    {
        Init();

        Create(parent, id, pos, size, style, name, menuBar);
    }

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxNO_BORDER | wxTB_HORIZONTAL,
                const wxString& name = wxToolBarNameStr,
                wxMenuBar* menuBar = NULL);

    virtual ~wxToolMenuBar();

    // override/implement base class virtuals
    virtual bool Realize();

    // implementation only from now on
    // -------------------------------

    // Override in order to bypass wxToolBar's overridden function
    virtual WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam);

    virtual bool MSWCommand(WXUINT param, WXWORD id);

    // Return HMENU for the menu associated with the commandbar
    WXHMENU GetHMenu();

    // Set the wxMenuBar associated with this commandbar
    void SetMenuBar(wxMenuBar* menuBar) { m_menuBar = menuBar; }

    // Returns the wxMenuBar associated with this commandbar
    wxMenuBar* GetMenuBar() const { return m_menuBar; }

protected:
    // common part of all ctors
    void Init();

    // create the native toolbar control
    bool MSWCreateToolbar(const wxPoint& pos, const wxSize& size, wxMenuBar* menuBar);

    // recreate the control completely
    void Recreate();

    // implement base class pure virtuals
    virtual bool DoInsertTool(size_t pos, wxToolBarToolBase *tool);
    virtual bool DoDeleteTool(size_t pos, wxToolBarToolBase *tool);

    virtual wxToolBarToolBase *CreateTool(int id,
                                          const wxString& label,
                                          const wxBitmap& bmpNormal,
                                          const wxBitmap& bmpDisabled,
                                          wxItemKind kind,
                                          wxObject *clientData,
                                          const wxString& shortHelp,
                                          const wxString& longHelp);
    virtual wxToolBarToolBase *CreateTool(wxControl *control);

    // The menubar associated with this toolbar
    wxMenuBar*  m_menuBar;

private:
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxToolMenuBar)
    DECLARE_NO_COPY_CLASS(wxToolMenuBar)
};

#endif
  // __SMARTPHONE__

#endif // wxUSE_TOOLBAR

#endif
    // _WX_BARWCE_H_
