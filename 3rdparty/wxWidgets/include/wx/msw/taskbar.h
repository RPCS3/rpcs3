/////////////////////////////////////////////////////////////////////////
// File:        wx/msw/taskbar.h
// Purpose:     Defines wxTaskBarIcon class for manipulating icons on the
//              Windows task bar.
// Author:      Julian Smart
// Modified by: Vaclav Slavik
// Created:     24/3/98
// RCS-ID:      $Id: taskbar.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////

#ifndef _TASKBAR_H_
#define _TASKBAR_H_

#include "wx/icon.h"

// private helper class:
class WXDLLIMPEXP_FWD_ADV wxTaskBarIconWindow;

class WXDLLIMPEXP_ADV wxTaskBarIcon: public wxTaskBarIconBase
{
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxTaskBarIcon)
public:
    wxTaskBarIcon();
    virtual ~wxTaskBarIcon();

// Accessors
    inline bool IsOk() const { return true; }
    inline bool IsIconInstalled() const { return m_iconAdded; }

// Operations
    bool SetIcon(const wxIcon& icon, const wxString& tooltip = wxEmptyString);
    bool RemoveIcon(void);
    bool PopupMenu(wxMenu *menu); //, int x, int y);

#if WXWIN_COMPATIBILITY_2_4
    wxDEPRECATED( bool IsOK() const );

// Overridables
    virtual void OnMouseMove(wxEvent&);
    virtual void OnLButtonDown(wxEvent&);
    virtual void OnLButtonUp(wxEvent&);
    virtual void OnRButtonDown(wxEvent&);
    virtual void OnRButtonUp(wxEvent&);
    virtual void OnLButtonDClick(wxEvent&);
    virtual void OnRButtonDClick(wxEvent&);
#endif

// Implementation
protected:
    friend class wxTaskBarIconWindow;
    long WindowProc(unsigned int msg, unsigned int wParam, long lParam);
    void RegisterWindowMessages();

// Data members
protected:
    wxTaskBarIconWindow *m_win;
    bool                 m_iconAdded;
    wxIcon               m_icon;
    wxString             m_strTooltip;

#if WXWIN_COMPATIBILITY_2_4
    // non-virtual default event handlers to forward events to the virtuals
    void _OnMouseMove(wxTaskBarIconEvent&);
    void _OnLButtonDown(wxTaskBarIconEvent&);
    void _OnLButtonUp(wxTaskBarIconEvent&);
    void _OnRButtonDown(wxTaskBarIconEvent&);
    void _OnRButtonUp(wxTaskBarIconEvent&);
    void _OnLButtonDClick(wxTaskBarIconEvent&);
    void _OnRButtonDClick(wxTaskBarIconEvent&);

    DECLARE_EVENT_TABLE()
#endif
};

#if WXWIN_COMPATIBILITY_2_4
inline bool wxTaskBarIcon::IsOK() const { return IsOk(); }
#endif

#endif
    // _TASKBAR_H_
