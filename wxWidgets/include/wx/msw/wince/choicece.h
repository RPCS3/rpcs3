///////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/wince/choicece.h
// Purpose:     wxChoice implementation for smart phones driven by WinCE
// Author:      Wlodzimierz ABX Skiba
// Modified by:
// Created:     29.07.2004
// RCS-ID:      $Id: choicece.h 38319 2006-03-23 22:05:23Z VZ $
// Copyright:   (c) Wlodzimierz Skiba
// License:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CHOICECE_H_BASE_
#define _WX_CHOICECE_H_BASE_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/defs.h"

#if wxUSE_CHOICE

#include "wx/dynarray.h"

class WXDLLEXPORT wxChoice;
WX_DEFINE_EXPORTED_ARRAY_PTR(wxChoice *, wxArrayChoiceSpins);

// ----------------------------------------------------------------------------
// Choice item
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxChoice : public wxChoiceBase
{
public:
    // ctors
    wxChoice() { }
    virtual ~wxChoice();

    wxChoice(wxWindow *parent,
             wxWindowID id,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             int n = 0, const wxString choices[] = NULL,
             long style = 0,
             const wxValidator& validator = wxDefaultValidator,
             const wxString& name = wxChoiceNameStr)
    {
        Create(parent, id, pos, size, n, choices, style, validator, name);
    }
    wxChoice(wxWindow *parent,
             wxWindowID id,
             const wxPoint& pos,
             const wxSize& size,
             const wxArrayString& choices,
             long style = 0,
             const wxValidator& validator = wxDefaultValidator,
             const wxString& name = wxChoiceNameStr)
    {
        Create(parent, id, pos, size, choices, style, validator, name);
    }

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                int n = 0, const wxString choices[] = NULL,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxChoiceNameStr);

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos,
                const wxSize& size,
                const wxArrayString& choices,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxChoiceNameStr);

    // implement base class pure virtuals
    virtual int DoAppend(const wxString& item);
    virtual int DoInsert(const wxString& item, unsigned int pos);
    virtual void Delete(unsigned int n);
    virtual void Clear() ;

    virtual unsigned int GetCount() const;
    virtual int GetSelection() const;
    virtual void SetSelection(int n);

    virtual int FindString(const wxString& s, bool bCase = false) const;
    virtual wxString GetString(unsigned int n) const;
    virtual void SetString(unsigned int n, const wxString& s);

    // get the subclassed window proc of the buddy list of choices
    WXFARPROC GetBuddyWndProc() const { return m_wndProcBuddy; }

    // return the choice object whose buddy is the given window or NULL
    static wxChoice *GetChoiceForListBox(WXHWND hwndBuddy);

    virtual bool MSWCommand(WXUINT param, WXWORD id);

protected:
    virtual void DoSetItemClientData(unsigned int n, void* clientData);
    virtual void* DoGetItemClientData(unsigned int n) const;
    virtual void DoSetItemClientObject(unsigned int n, wxClientData* clientData);
    virtual wxClientData* DoGetItemClientObject(unsigned int n) const;

    // MSW implementation
    virtual void DoGetPosition(int *x, int *y) const;
    virtual void DoMoveWindow(int x, int y, int width, int height);
    virtual wxSize DoGetBestSize() const;
    virtual void DoGetSize(int *width, int *height) const;

    virtual WXDWORD MSWGetStyle(long style, WXDWORD *exstyle) const;

    // create and initialize the control
    bool CreateAndInit(wxWindow *parent, wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       int n, const wxString choices[],
                       long style,
                       const wxValidator& validator,
                       const wxString& name);

    // free all memory we have (used by Clear() and dtor)
    void Free();

    // the data for the "buddy" list
    WXHWND     m_hwndBuddy;
    WXFARPROC  m_wndProcBuddy;

    // all existing wxChoice - this allows to find the one corresponding to
    // the given buddy window in GetSpinChoiceCtrl()
    static wxArrayChoiceSpins ms_allChoiceSpins;

private:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxChoice)
};

#endif // wxUSE_CHOICE

#endif // _WX_CHOICECE_H_BASE_
