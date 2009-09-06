/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/choice.h
// Purpose:     wxChoice class
// Author:      Julian Smart
// Modified by: Vadim Zeitlin to derive from wxChoiceBase
// Created:     01/02/97
// RCS-ID:      $Id: choice.h 51616 2008-02-09 15:22:15Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CHOICE_H_
#define _WX_CHOICE_H_

// ----------------------------------------------------------------------------
// Choice item
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxChoice : public wxChoiceBase
{
public:
    // ctors
    wxChoice() { Init(); }
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
        Init();
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
        Init();
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

    virtual void SetLabel(const wxString& label);

    virtual void Delete(unsigned int n);
    virtual void Clear();

    virtual unsigned int GetCount() const;
    virtual int GetSelection() const;
    virtual int GetCurrentSelection() const;
    virtual void SetSelection(int n);

    virtual int FindString(const wxString& s, bool bCase = false) const;
    virtual wxString GetString(unsigned int n) const;
    virtual void SetString(unsigned int n, const wxString& s);

    // MSW only
    virtual bool MSWCommand(WXUINT param, WXWORD id);
    WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam);
    virtual WXHBRUSH MSWControlColor(WXHDC hDC, WXHWND hWnd);
    virtual bool MSWShouldPreProcessMessage(WXMSG *pMsg);
    virtual WXDWORD MSWGetStyle(long style, WXDWORD *exstyle) const;

protected:
    // common part of all ctors
    void Init() { m_lastAcceptedSelection = wxID_NONE; }

    virtual int DoAppend(const wxString& item);
    virtual int DoInsert(const wxString& item, unsigned int pos);
    virtual void DoMoveWindow(int x, int y, int width, int height);
    virtual void DoSetItemClientData(unsigned int n, void* clientData);
    virtual void* DoGetItemClientData(unsigned int n) const;
    virtual void DoSetItemClientObject(unsigned int n, wxClientData* clientData);
    virtual wxClientData* DoGetItemClientObject(unsigned int n) const;

    // MSW implementation
    virtual wxSize DoGetBestSize() const;
    virtual void DoGetSize(int *w, int *h) const;
    virtual void DoSetSize(int x, int y,
                           int width, int height,
                           int sizeFlags = wxSIZE_AUTO);

    // update the height of the drop down list to fit the number of items we
    // have (without changing the visible height)
    void UpdateVisibleHeight();

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


    // last "completed" selection, i.e. not the transient one while the user is
    // browsing the popup list: this is only used when != wxID_NONE which is
    // the case while the drop down is opened
    int m_lastAcceptedSelection;


    DECLARE_DYNAMIC_CLASS_NO_COPY(wxChoice)
};

#endif // _WX_CHOICE_H_
