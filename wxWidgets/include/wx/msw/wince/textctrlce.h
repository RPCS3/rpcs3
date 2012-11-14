/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/wince/textctrlce.h
// Purpose:     wxTextCtrl implementation for smart phones driven by WinCE
// Author:      Wlodzimierz ABX Skiba
// Modified by:
// Created:     30.08.2004
// RCS-ID:      $Id: textctrlce.h 41739 2006-10-08 17:46:12Z VZ $
// Copyright:   (c) Wlodzimierz Skiba
// License:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_TEXTCTRLCE_H_
#define _WX_TEXTCTRLCE_H_

#include "wx/dynarray.h"

class WXDLLEXPORT wxTextCtrl;
WX_DEFINE_EXPORTED_ARRAY_PTR(wxTextCtrl *, wxArrayTextSpins);

class WXDLLEXPORT wxTextCtrl : public wxTextCtrlBase
{
public:
    // creation
    // --------

    wxTextCtrl() { Init(); }
    wxTextCtrl(wxWindow *parent, wxWindowID id,
               const wxString& value = wxEmptyString,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = 0,
               const wxValidator& validator = wxDefaultValidator,
               const wxString& name = wxTextCtrlNameStr)
    {
        Init();

        Create(parent, id, value, pos, size, style, validator, name);
    }
    virtual ~wxTextCtrl();

    bool Create(wxWindow *parent, wxWindowID id,
                const wxString& value = wxEmptyString,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxTextCtrlNameStr);

    // implement base class pure virtuals
    // ----------------------------------

    virtual wxString GetValue() const;
    virtual void SetValue(const wxString& value) { DoSetValue(value, SetValue_SendEvent); }

    virtual void ChangeValue(const wxString &value) { DoSetValue(value); }

    virtual wxString GetRange(long from, long to) const;

    virtual int GetLineLength(long lineNo) const;
    virtual wxString GetLineText(long lineNo) const;
    virtual int GetNumberOfLines() const;

    virtual bool IsModified() const;
    virtual bool IsEditable() const;

    virtual void GetSelection(long* from, long* to) const;

    // operations
    // ----------

    // editing
    virtual void Clear();
    virtual void Replace(long from, long to, const wxString& value);
    virtual void Remove(long from, long to);

    // load the controls contents from the file
    virtual bool LoadFile(const wxString& file);

    // clears the dirty flag
    virtual void MarkDirty();
    virtual void DiscardEdits();

    virtual void SetMaxLength(unsigned long len);

    // writing text inserts it at the current position, appending always
    // inserts it at the end
    virtual void WriteText(const wxString& text);
    virtual void AppendText(const wxString& text);

    // translate between the position (which is just an index in the text ctrl
    // considering all its contents as a single strings) and (x, y) coordinates
    // which represent column and line.
    virtual long XYToPosition(long x, long y) const;
    virtual bool PositionToXY(long pos, long *x, long *y) const;

    virtual void ShowPosition(long pos);
    virtual wxTextCtrlHitTestResult HitTest(const wxPoint& pt, long *pos) const;
    virtual wxTextCtrlHitTestResult HitTest(const wxPoint& pt,
                                            wxTextCoord *col,
                                            wxTextCoord *row) const
    {
        return wxTextCtrlBase::HitTest(pt, col, row);
    }

    // Clipboard operations
    virtual void Copy();
    virtual void Cut();
    virtual void Paste();

    virtual bool CanCopy() const;
    virtual bool CanCut() const;
    virtual bool CanPaste() const;

    // Undo/redo
    virtual void Undo();
    virtual void Redo();

    virtual bool CanUndo() const;
    virtual bool CanRedo() const;

    // Insertion point
    virtual void SetInsertionPoint(long pos);
    virtual void SetInsertionPointEnd();
    virtual long GetInsertionPoint() const;
    virtual wxTextPos GetLastPosition() const;

    virtual void SetSelection(long from, long to);
    virtual void SetEditable(bool editable);

    // Caret handling (Windows only)

    bool ShowNativeCaret(bool show = true);
    bool HideNativeCaret() { return ShowNativeCaret(false); }

    // Implementation from now on
    // --------------------------

    virtual void Command(wxCommandEvent& event);
    virtual bool MSWCommand(WXUINT param, WXWORD id);

    virtual void AdoptAttributesFromHWND();

    virtual bool AcceptsFocus() const;

    // callbacks
    void OnDropFiles(wxDropFilesEvent& event);
    void OnChar(wxKeyEvent& event); // Process 'enter' if required

    void OnCut(wxCommandEvent& event);
    void OnCopy(wxCommandEvent& event);
    void OnPaste(wxCommandEvent& event);
    void OnUndo(wxCommandEvent& event);
    void OnRedo(wxCommandEvent& event);
    void OnDelete(wxCommandEvent& event);
    void OnSelectAll(wxCommandEvent& event);

    void OnUpdateCut(wxUpdateUIEvent& event);
    void OnUpdateCopy(wxUpdateUIEvent& event);
    void OnUpdatePaste(wxUpdateUIEvent& event);
    void OnUpdateUndo(wxUpdateUIEvent& event);
    void OnUpdateRedo(wxUpdateUIEvent& event);
    void OnUpdateDelete(wxUpdateUIEvent& event);
    void OnUpdateSelectAll(wxUpdateUIEvent& event);

    // Show a context menu for Rich Edit controls (the standard
    // EDIT control has one already)
    void OnRightClick(wxMouseEvent& event);

    // be sure the caret remains invisible if the user
    // called HideNativeCaret() before
    void OnSetFocus(wxFocusEvent& event);

    // get the subclassed window proc of the buddy
    WXFARPROC GetBuddyWndProc() const { return m_wndProcBuddy; }

    // intercept WM_GETDLGCODE
    virtual WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam);

protected:
    // common part of all ctors
    void Init();

    // call this to increase the size limit (will do nothing if the current
    // limit is big enough)
    //
    // returns true if we increased the limit to allow entering more text,
    // false if we hit the limit set by SetMaxLength() and so didn't change it
    bool AdjustSpaceLimit();

    void DoSetValue(const wxString &value, int flags = 0);

    // replace the contents of the selection or of the entire control with the
    // given text
    void DoWriteText(const wxString& text, int flags = SetValue_SelectionOnly);

    // set the selection possibly without scrolling the caret into view
    void DoSetSelection(long from, long to, bool scrollCaret = true);

    // return true if there is a non empty selection in the control
    bool HasSelection() const;

    // get the length of the line containing the character at the given
    // position
    long GetLengthOfLineContainingPos(long pos) const;

    // send TEXT_UPDATED event, return true if it was handled, false otherwise
    bool SendUpdateEvent();

    // override some base class virtuals
    virtual void DoMoveWindow(int x, int y, int width, int height);
    virtual wxSize DoGetBestSize() const;

    virtual WXDWORD MSWGetStyle(long style, WXDWORD *exstyle) const;

    // if true, SendUpdateEvent() will eat the next event (see comments in the
    // code as to why this is needed)
    bool m_suppressNextUpdate;

    // all existing wxTextCtrl - this allows to find the one corresponding to
    // the given buddy window in GetSpinTextCtrl()
    static wxArrayTextSpins ms_allTextSpins;

protected:

    // the data for the "buddy" list
    WXHWND     m_hwndBuddy;
    WXFARPROC  m_wndProcBuddy;

private:
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxTextCtrl)

    bool m_isNativeCaretShown;
};

#endif // _WX_TEXTCTRLCE_H_
