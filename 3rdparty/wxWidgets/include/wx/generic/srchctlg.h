/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/srchctlg.h
// Purpose:     generic wxSearchCtrl class
// Author:      Vince Harron
// Created:     2006-02-19
// RCS-ID:      $Id: srchctlg.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   Vince Harron
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_SEARCHCTRL_H_
#define _WX_GENERIC_SEARCHCTRL_H_

#if wxUSE_SEARCHCTRL

#include "wx/bitmap.h"

class WXDLLIMPEXP_FWD_CORE wxSearchButton;
class WXDLLIMPEXP_FWD_CORE wxSearchTextCtrl;

// ----------------------------------------------------------------------------
// wxSearchCtrl is a combination of wxTextCtrl and wxSearchButton
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxSearchCtrl : public wxSearchCtrlBase
{
public:
    // creation
    // --------

    wxSearchCtrl();
    wxSearchCtrl(wxWindow *parent, wxWindowID id,
               const wxString& value = wxEmptyString,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = 0,
               const wxValidator& validator = wxDefaultValidator,
               const wxString& name = wxSearchCtrlNameStr);

    virtual ~wxSearchCtrl();

    bool Create(wxWindow *parent, wxWindowID id,
                const wxString& value = wxEmptyString,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxSearchCtrlNameStr);

#if wxUSE_MENUS
    // get/set search button menu
    // --------------------------
    virtual void SetMenu( wxMenu* menu );
    virtual wxMenu* GetMenu();
#endif // wxUSE_MENUS

    // get/set search options
    // ----------------------
    virtual void ShowSearchButton( bool show );
    virtual bool IsSearchButtonVisible() const;

    virtual void ShowCancelButton( bool show );
    virtual bool IsCancelButtonVisible() const;

#if wxABI_VERSION >= 20802
    // TODO: In 2.9 these should probably be virtual, and declared in the base class...
    void SetDescriptiveText(const wxString& text);
    wxString GetDescriptiveText() const;
#endif

    // accessors
    // ---------

    virtual wxString GetValue() const;
    virtual void SetValue(const wxString& value);

    virtual wxString GetRange(long from, long to) const;

    virtual int GetLineLength(long lineNo) const;
    virtual wxString GetLineText(long lineNo) const;
    virtual int GetNumberOfLines() const;

    virtual bool IsModified() const;
    virtual bool IsEditable() const;

    // more readable flag testing methods
    virtual bool IsSingleLine() const;
    virtual bool IsMultiLine() const;

    // If the return values from and to are the same, there is no selection.
    virtual void GetSelection(long* from, long* to) const;

    virtual wxString GetStringSelection() const;

    // operations
    // ----------

    // editing
    virtual void Clear();
    virtual void Replace(long from, long to, const wxString& value);
    virtual void Remove(long from, long to);

    // load/save the controls contents from/to the file
    virtual bool LoadFile(const wxString& file);
    virtual bool SaveFile(const wxString& file = wxEmptyString);

    // sets/clears the dirty flag
    virtual void MarkDirty();
    virtual void DiscardEdits();

    // set the max number of characters which may be entered in a single line
    // text control
    virtual void SetMaxLength(unsigned long WXUNUSED(len));

    // writing text inserts it at the current position, appending always
    // inserts it at the end
    virtual void WriteText(const wxString& text);
    virtual void AppendText(const wxString& text);

    // insert the character which would have resulted from this key event,
    // return true if anything has been inserted
    virtual bool EmulateKeyPress(const wxKeyEvent& event);

    // text control under some platforms supports the text styles: these
    // methods allow to apply the given text style to the given selection or to
    // set/get the style which will be used for all appended text
    virtual bool SetStyle(long start, long end, const wxTextAttr& style);
    virtual bool GetStyle(long position, wxTextAttr& style);
    virtual bool SetDefaultStyle(const wxTextAttr& style);
    virtual const wxTextAttr& GetDefaultStyle() const;

    // translate between the position (which is just an index in the text ctrl
    // considering all its contents as a single strings) and (x, y) coordinates
    // which represent column and line.
    virtual long XYToPosition(long x, long y) const;
    virtual bool PositionToXY(long pos, long *x, long *y) const;

    virtual void ShowPosition(long pos);

    // find the character at position given in pixels
    //
    // NB: pt is in device coords (not adjusted for the client area origin nor
    //     scrolling)
    virtual wxTextCtrlHitTestResult HitTest(const wxPoint& pt, long *pos) const;
    virtual wxTextCtrlHitTestResult HitTest(const wxPoint& pt,
                                            wxTextCoord *col,
                                            wxTextCoord *row) const;

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
    virtual void SelectAll();
    virtual void SetEditable(bool editable);

#if 0

    // override streambuf method
#if wxHAS_TEXT_WINDOW_STREAM
    int overflow(int i);
#endif // wxHAS_TEXT_WINDOW_STREAM

    // stream-like insertion operators: these are always available, whether we
    // were, or not, compiled with streambuf support
    wxTextCtrl& operator<<(const wxString& s);
    wxTextCtrl& operator<<(int i);
    wxTextCtrl& operator<<(long i);
    wxTextCtrl& operator<<(float f);
    wxTextCtrl& operator<<(double d);
    wxTextCtrl& operator<<(const wxChar c);
#endif

    // do the window-specific processing after processing the update event
    virtual void DoUpdateWindowUI(wxUpdateUIEvent& event);

    virtual bool ShouldInheritColours() const;

    // wxWindow overrides
    virtual bool SetFont(const wxFont& font);

    // search control generic only
    void SetSearchBitmap( const wxBitmap& bitmap );
    void SetCancelBitmap( const wxBitmap& bitmap );
#if wxUSE_MENUS
    void SetSearchMenuBitmap( const wxBitmap& bitmap );
#endif // wxUSE_MENUS

protected:
    virtual void DoSetValue(const wxString& value, int flags = 0);

    // override the base class virtuals involved into geometry calculations
    virtual wxSize DoGetBestSize() const;
    virtual void DoMoveWindow(int x, int y, int width, int height);
    virtual void LayoutControls(int x, int y, int width, int height);

    virtual void RecalcBitmaps();

    void Init();

    virtual wxBitmap RenderSearchBitmap( int x, int y, bool renderDrop );
    virtual wxBitmap RenderCancelBitmap( int x, int y );

    virtual void OnSearchButton( wxCommandEvent& event );

    void OnSetFocus( wxFocusEvent& event );
    void OnSize( wxSizeEvent& event );

    bool HasMenu() const
    {
#if wxUSE_MENUS
        return m_menu != NULL;
#else // !wxUSE_MENUS
        return false;
#endif // wxUSE_MENUS/!wxUSE_MENUS
    }

private:
    friend class wxSearchButton;

#if wxUSE_MENUS
    void PopupSearchMenu();
#endif // wxUSE_MENUS

    // the subcontrols
    wxSearchTextCtrl *m_text;
    wxSearchButton *m_searchButton;
    wxSearchButton *m_cancelButton;
#if wxUSE_MENUS
    wxMenu *m_menu;
#endif // wxUSE_MENUS

    bool m_searchButtonVisible;
    bool m_cancelButtonVisible;

    bool m_searchBitmapUser;
    bool m_cancelBitmapUser;
#if wxUSE_MENUS
    bool m_searchMenuBitmapUser;
#endif // wxUSE_MENUS

    wxBitmap m_searchBitmap;
    wxBitmap m_cancelBitmap;
#if wxUSE_MENUS
    wxBitmap m_searchMenuBitmap;
#endif // wxUSE_MENUS

private:
    DECLARE_DYNAMIC_CLASS(wxSearchCtrl)

    DECLARE_EVENT_TABLE()
};

#endif // wxUSE_SEARCHCTRL

#endif // _WX_GENERIC_SEARCHCTRL_H_

