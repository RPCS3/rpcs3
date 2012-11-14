///////////////////////////////////////////////////////////////////////////////
// Name:        wx/textctrl.h
// Purpose:     wxTextCtrlBase class - the interface of wxTextCtrl
// Author:      Vadim Zeitlin
// Modified by:
// Created:     13.07.99
// RCS-ID:      $Id: textctrl.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) Vadim Zeitlin
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_TEXTCTRL_H_BASE_
#define _WX_TEXTCTRL_H_BASE_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/defs.h"

#if wxUSE_TEXTCTRL

#include "wx/control.h"         // the base class
#include "wx/dynarray.h"        // wxArrayInt
#include "wx/gdicmn.h"          // wxPoint

// Open Watcom 1.3 does allow only ios::rdbuf() while
// we want something with streambuf parameter
// Also, can't use streambuf if making or using a DLL :-(

#if defined(__WATCOMC__) || \
    defined(__MWERKS__) || \
    (defined(__WINDOWS__) && (defined(WXUSINGDLL) || defined(WXMAKINGDLL)))
    #define wxHAS_TEXT_WINDOW_STREAM 0
#elif wxUSE_STD_IOSTREAM
    #include "wx/ioswrap.h"
    #define wxHAS_TEXT_WINDOW_STREAM 1
#else
    #define wxHAS_TEXT_WINDOW_STREAM 0
#endif

#if WXWIN_COMPATIBILITY_2_4 && !wxHAS_TEXT_WINDOW_STREAM
    // define old flag if one could use it somewhere
    #define NO_TEXT_WINDOW_STREAM
#endif

class WXDLLIMPEXP_FWD_CORE wxTextCtrl;
class WXDLLIMPEXP_FWD_CORE wxTextCtrlBase;

// ----------------------------------------------------------------------------
// wxTextCtrl types
// ----------------------------------------------------------------------------

// wxTextPos is the position in the text
typedef long wxTextPos;

// wxTextCoord is the line or row number (which should have been unsigned but
// is long for backwards compatibility)
typedef long wxTextCoord;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

extern WXDLLEXPORT_DATA(const wxChar) wxTextCtrlNameStr[];

// this is intentionally not enum to avoid warning fixes with
// typecasting from enum type to wxTextCoord
const wxTextCoord wxOutOfRangeTextCoord = -1;
const wxTextCoord wxInvalidTextCoord    = -2;

// ----------------------------------------------------------------------------
// wxTextCtrl style flags
// ----------------------------------------------------------------------------

#define wxTE_NO_VSCROLL     0x0002
#define wxTE_AUTO_SCROLL    0x0008

#define wxTE_READONLY       0x0010
#define wxTE_MULTILINE      0x0020
#define wxTE_PROCESS_TAB    0x0040

// alignment flags
#define wxTE_LEFT           0x0000                    // 0x0000
#define wxTE_CENTER         wxALIGN_CENTER_HORIZONTAL // 0x0100
#define wxTE_RIGHT          wxALIGN_RIGHT             // 0x0200
#define wxTE_CENTRE         wxTE_CENTER

// this style means to use RICHEDIT control and does something only under wxMSW
// and Win32 and is silently ignored under all other platforms
#define wxTE_RICH           0x0080

#define wxTE_PROCESS_ENTER  0x0400
#define wxTE_PASSWORD       0x0800

// automatically detect the URLs and generate the events when mouse is
// moved/clicked over an URL
//
// this is for Win32 richedit and wxGTK2 multiline controls only so far
#define wxTE_AUTO_URL       0x1000

// by default, the Windows text control doesn't show the selection when it
// doesn't have focus - use this style to force it to always show it
#define wxTE_NOHIDESEL      0x2000

// use wxHSCROLL to not wrap text at all, wxTE_CHARWRAP to wrap it at any
// position and wxTE_WORDWRAP to wrap at words boundary
//
// if no wrapping style is given at all, the control wraps at word boundary
#define wxTE_DONTWRAP       wxHSCROLL
#define wxTE_CHARWRAP       0x4000  // wrap at any position
#define wxTE_WORDWRAP       0x0001  // wrap only at words boundaries
#define wxTE_BESTWRAP       0x0000  // this is the default

#if WXWIN_COMPATIBILITY_2_6
    // obsolete synonym
    #define wxTE_LINEWRAP       wxTE_CHARWRAP
#endif // WXWIN_COMPATIBILITY_2_6

// force using RichEdit version 2.0 or 3.0 instead of 1.0 (default) for
// wxTE_RICH controls - can be used together with or instead of wxTE_RICH
#define wxTE_RICH2          0x8000

// reuse wxTE_RICH2's value for CAPEDIT control on Windows CE
#if defined(__SMARTPHONE__) || defined(__POCKETPC__)
#define wxTE_CAPITALIZE     wxTE_RICH2
#else
#define wxTE_CAPITALIZE     0
#endif

// ----------------------------------------------------------------------------
// wxTextCtrl file types
// ----------------------------------------------------------------------------

#define wxTEXT_TYPE_ANY     0

// ----------------------------------------------------------------------------
// wxTextCtrl::HitTest return values
// ----------------------------------------------------------------------------

// the point asked is ...
enum wxTextCtrlHitTestResult
{
    wxTE_HT_UNKNOWN = -2,   // this means HitTest() is simply not implemented
    wxTE_HT_BEFORE,         // either to the left or upper
    wxTE_HT_ON_TEXT,        // directly on
    wxTE_HT_BELOW,          // below [the last line]
    wxTE_HT_BEYOND          // after [the end of line]
};
// ... the character returned

// ----------------------------------------------------------------------------
// Types for wxTextAttr
// ----------------------------------------------------------------------------

// Alignment

enum wxTextAttrAlignment
{
    wxTEXT_ALIGNMENT_DEFAULT,
    wxTEXT_ALIGNMENT_LEFT,
    wxTEXT_ALIGNMENT_CENTRE,
    wxTEXT_ALIGNMENT_CENTER = wxTEXT_ALIGNMENT_CENTRE,
    wxTEXT_ALIGNMENT_RIGHT,
    wxTEXT_ALIGNMENT_JUSTIFIED
};

// Flags to indicate which attributes are being applied

#define wxTEXT_ATTR_TEXT_COLOUR             0x0001
#define wxTEXT_ATTR_BACKGROUND_COLOUR       0x0002
#define wxTEXT_ATTR_FONT_FACE               0x0004
#define wxTEXT_ATTR_FONT_SIZE               0x0008
#define wxTEXT_ATTR_FONT_WEIGHT             0x0010
#define wxTEXT_ATTR_FONT_ITALIC             0x0020
#define wxTEXT_ATTR_FONT_UNDERLINE          0x0040
#define wxTEXT_ATTR_FONT \
  ( wxTEXT_ATTR_FONT_FACE | wxTEXT_ATTR_FONT_SIZE | wxTEXT_ATTR_FONT_WEIGHT | \
    wxTEXT_ATTR_FONT_ITALIC | wxTEXT_ATTR_FONT_UNDERLINE )
#define wxTEXT_ATTR_ALIGNMENT               0x0080
#define wxTEXT_ATTR_LEFT_INDENT             0x0100
#define wxTEXT_ATTR_RIGHT_INDENT            0x0200
#define wxTEXT_ATTR_TABS                    0x0400

// ----------------------------------------------------------------------------
// wxTextAttr: a structure containing the visual attributes of a text
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxTextAttr
{
public:
    // ctors
    wxTextAttr() { Init(); }
    wxTextAttr(const wxColour& colText,
               const wxColour& colBack = wxNullColour,
               const wxFont& font = wxNullFont,
               wxTextAttrAlignment alignment = wxTEXT_ALIGNMENT_DEFAULT);

    // operations
    void Init();

    // merges the attributes of the base and the overlay objects and returns
    // the result; the parameter attributes take precedence
    //
    // WARNING: the order of arguments is the opposite of Combine()
    static wxTextAttr Merge(const wxTextAttr& base, const wxTextAttr& overlay)
    {
        return Combine(overlay, base, NULL);
    }

    // merges the attributes of this object and overlay
    void Merge(const wxTextAttr& overlay)
    {
        *this = Merge(*this, overlay);
    }


    // operators
    void operator= (const wxTextAttr& attr);

    // setters
    void SetTextColour(const wxColour& colText) { m_colText = colText; m_flags |= wxTEXT_ATTR_TEXT_COLOUR; }
    void SetBackgroundColour(const wxColour& colBack) { m_colBack = colBack; m_flags |= wxTEXT_ATTR_BACKGROUND_COLOUR; }
    void SetFont(const wxFont& font, long flags = wxTEXT_ATTR_FONT) { m_font = font; m_flags |= flags; }
    void SetAlignment(wxTextAttrAlignment alignment) { m_textAlignment = alignment; m_flags |= wxTEXT_ATTR_ALIGNMENT; }
    void SetTabs(const wxArrayInt& tabs) { m_tabs = tabs; m_flags |= wxTEXT_ATTR_TABS; }
    void SetLeftIndent(int indent, int subIndent = 0) { m_leftIndent = indent; m_leftSubIndent = subIndent; m_flags |= wxTEXT_ATTR_LEFT_INDENT; }
    void SetRightIndent(int indent) { m_rightIndent = indent; m_flags |= wxTEXT_ATTR_RIGHT_INDENT; }
    void SetFlags(long flags) { m_flags = flags; }

    // accessors
    bool HasTextColour() const { return m_colText.Ok() && HasFlag(wxTEXT_ATTR_TEXT_COLOUR) ; }
    bool HasBackgroundColour() const { return m_colBack.Ok() && HasFlag(wxTEXT_ATTR_BACKGROUND_COLOUR) ; }
    bool HasFont() const { return m_font.Ok() && HasFlag(wxTEXT_ATTR_FONT) ; }
    bool HasAlignment() const { return (m_textAlignment != wxTEXT_ALIGNMENT_DEFAULT) && ((m_flags & wxTEXT_ATTR_ALIGNMENT) != 0) ; }
    bool HasTabs() const { return (m_flags & wxTEXT_ATTR_TABS) != 0 ; }
    bool HasLeftIndent() const { return (m_flags & wxTEXT_ATTR_LEFT_INDENT) != 0 ; }
    bool HasRightIndent() const { return (m_flags & wxTEXT_ATTR_RIGHT_INDENT) != 0 ; }
    bool HasFlag(long flag) const { return (m_flags & flag) != 0; }

    const wxColour& GetTextColour() const { return m_colText; }
    const wxColour& GetBackgroundColour() const { return m_colBack; }
    const wxFont& GetFont() const { return m_font; }
    wxTextAttrAlignment GetAlignment() const { return m_textAlignment; }
    const wxArrayInt& GetTabs() const { return m_tabs; }
    long GetLeftIndent() const { return m_leftIndent; }
    long GetLeftSubIndent() const { return m_leftSubIndent; }
    long GetRightIndent() const { return m_rightIndent; }
    long GetFlags() const { return m_flags; }

    // returns false if we have any attributes set, true otherwise
    bool IsDefault() const
    {
        return !HasTextColour() && !HasBackgroundColour() && !HasFont() && !HasAlignment() &&
               !HasTabs() && !HasLeftIndent() && !HasRightIndent() ;
    }

    // return the attribute having the valid font and colours: it uses the
    // attributes set in attr and falls back first to attrDefault and then to
    // the text control font/colours for those attributes which are not set
    static wxTextAttr Combine(const wxTextAttr& attr,
                              const wxTextAttr& attrDef,
                              const wxTextCtrlBase *text);

private:
    long                m_flags;
    wxColour            m_colText,
                        m_colBack;
    wxFont              m_font;
    wxTextAttrAlignment m_textAlignment;
    wxArrayInt          m_tabs; // array of int: tab stops in 1/10 mm
    int                 m_leftIndent; // left indent in 1/10 mm
    int                 m_leftSubIndent; // left indent for all but the first
                                         // line in a paragraph relative to the
                                         // first line, in 1/10 mm
    int                 m_rightIndent; // right indent in 1/10 mm
};

// ----------------------------------------------------------------------------
// wxTextCtrl: a single or multiple line text zone where user can enter and
// edit text
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxTextCtrlBase : public wxControl
#if wxHAS_TEXT_WINDOW_STREAM
                                 , public wxSTD streambuf
#endif

{
public:
    // creation
    // --------

    wxTextCtrlBase(){}
    virtual ~wxTextCtrlBase(){}

    // accessors
    // ---------

    virtual wxString GetValue() const = 0;
    virtual bool IsEmpty() const { return GetValue().empty(); }

    virtual void SetValue(const wxString& value)
        { DoSetValue(value, SetValue_SendEvent); }
    virtual void ChangeValue(const wxString& value)
        { DoSetValue(value); }

    virtual wxString GetRange(long from, long to) const;

    virtual int GetLineLength(long lineNo) const = 0;
    virtual wxString GetLineText(long lineNo) const = 0;
    virtual int GetNumberOfLines() const = 0;

    virtual bool IsModified() const = 0;
    virtual bool IsEditable() const = 0;

    // more readable flag testing methods
    bool IsSingleLine() const { return !HasFlag(wxTE_MULTILINE); }
    bool IsMultiLine() const { return !IsSingleLine(); }

    // If the return values from and to are the same, there is no selection.
    virtual void GetSelection(long* from, long* to) const = 0;

    virtual wxString GetStringSelection() const;

    // operations
    // ----------

    // editing
    virtual void Clear() = 0;
    virtual void Replace(long from, long to, const wxString& value) = 0;
    virtual void Remove(long from, long to) = 0;

    // load/save the control's contents from/to a file
    bool LoadFile(const wxString& file, int fileType = wxTEXT_TYPE_ANY) { return DoLoadFile(file, fileType); }
    bool SaveFile(const wxString& file = wxEmptyString, int fileType = wxTEXT_TYPE_ANY);

    // implementation for loading/saving
    virtual bool DoLoadFile(const wxString& file, int fileType);
    virtual bool DoSaveFile(const wxString& file, int fileType);

    // sets/clears the dirty flag
    virtual void MarkDirty() = 0;
    virtual void DiscardEdits() = 0;
    void SetModified(bool modified)
    {
        if ( modified )
            MarkDirty();
        else
            DiscardEdits();
    }

    // set the max number of characters which may be entered in a single line
    // text control
    virtual void SetMaxLength(unsigned long WXUNUSED(len)) { }

    // writing text inserts it at the current position, appending always
    // inserts it at the end
    virtual void WriteText(const wxString& text) = 0;
    virtual void AppendText(const wxString& text) = 0;

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
    virtual long XYToPosition(long x, long y) const = 0;
    virtual bool PositionToXY(long pos, long *x, long *y) const = 0;

    virtual void ShowPosition(long pos) = 0;

    // find the character at position given in pixels
    //
    // NB: pt is in device coords (not adjusted for the client area origin nor
    //     scrolling)
    virtual wxTextCtrlHitTestResult HitTest(const wxPoint& pt, long *pos) const;
    virtual wxTextCtrlHitTestResult HitTest(const wxPoint& pt,
                                            wxTextCoord *col,
                                            wxTextCoord *row) const;

    // Clipboard operations
    virtual void Copy() = 0;
    virtual void Cut() = 0;
    virtual void Paste() = 0;

    virtual bool CanCopy() const;
    virtual bool CanCut() const;
    virtual bool CanPaste() const;

    // Undo/redo
    virtual void Undo() = 0;
    virtual void Redo() = 0;

    virtual bool CanUndo() const = 0;
    virtual bool CanRedo() const = 0;

    // Insertion point
    virtual void SetInsertionPoint(long pos) = 0;
    virtual void SetInsertionPointEnd() = 0;
    virtual long GetInsertionPoint() const = 0;
    virtual wxTextPos GetLastPosition() const = 0;

    virtual void SetSelection(long from, long to) = 0;
    virtual void SelectAll();
    virtual void SetEditable(bool editable) = 0;

    // stream-like insertion operators: these are always available, whether we
    // were, or not, compiled with streambuf support
    wxTextCtrl& operator<<(const wxString& s);
    wxTextCtrl& operator<<(int i);
    wxTextCtrl& operator<<(long i);
    wxTextCtrl& operator<<(float f);
    wxTextCtrl& operator<<(double d);
    wxTextCtrl& operator<<(const wxChar c);

    // generate the wxEVT_COMMAND_TEXT_UPDATED event, like SetValue() does
    void SendTextUpdatedEvent();

    // do the window-specific processing after processing the update event
    virtual void DoUpdateWindowUI(wxUpdateUIEvent& event);

    virtual bool ShouldInheritColours() const { return false; }

protected:
    // override streambuf method
#if wxHAS_TEXT_WINDOW_STREAM
    int overflow(int i);
#endif // wxHAS_TEXT_WINDOW_STREAM

    // flags for DoSetValue(): common part of SetValue() and ChangeValue() and
    // also used to implement WriteText() in wxMSW
    enum
    {
        SetValue_SendEvent = 1,
        SetValue_SelectionOnly = 2
    };

    virtual void DoSetValue(const wxString& value, int flags = 0) = 0;


    // the name of the last file loaded with LoadFile() which will be used by
    // SaveFile() by default
    wxString m_filename;

    // the text style which will be used for any new text added to the control
    wxTextAttr m_defaultStyle;

    DECLARE_NO_COPY_CLASS(wxTextCtrlBase)
    DECLARE_ABSTRACT_CLASS(wxTextCtrlBase)
};

// ----------------------------------------------------------------------------
// include the platform-dependent class definition
// ----------------------------------------------------------------------------

#if defined(__WXX11__)
    #include "wx/x11/textctrl.h"
#elif defined(__WXUNIVERSAL__)
    #include "wx/univ/textctrl.h"
#elif defined(__SMARTPHONE__) && defined(__WXWINCE__)
    #include "wx/msw/wince/textctrlce.h"
#elif defined(__WXMSW__)
    #include "wx/msw/textctrl.h"
#elif defined(__WXMOTIF__)
    #include "wx/motif/textctrl.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/textctrl.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/textctrl.h"
#elif defined(__WXMAC__)
    #include "wx/mac/textctrl.h"
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/textctrl.h"
#elif defined(__WXPM__)
    #include "wx/os2/textctrl.h"
#endif

// ----------------------------------------------------------------------------
// wxTextCtrl events
// ----------------------------------------------------------------------------

#if !WXWIN_COMPATIBILITY_EVENT_TYPES

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TEXT_UPDATED, 7)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TEXT_ENTER, 8)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TEXT_URL, 13)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TEXT_MAXLEN, 14)
END_DECLARE_EVENT_TYPES()

#endif // !WXWIN_COMPATIBILITY_EVENT_TYPES

class WXDLLEXPORT wxTextUrlEvent : public wxCommandEvent
{
public:
    wxTextUrlEvent(int winid, const wxMouseEvent& evtMouse,
                   long start, long end)
        : wxCommandEvent(wxEVT_COMMAND_TEXT_URL, winid)
        , m_evtMouse(evtMouse), m_start(start), m_end(end)
        { }

    // get the mouse event which happend over the URL
    const wxMouseEvent& GetMouseEvent() const { return m_evtMouse; }

    // get the start of the URL
    long GetURLStart() const { return m_start; }

    // get the end of the URL
    long GetURLEnd() const { return m_end; }

protected:
    // the corresponding mouse event
    wxMouseEvent m_evtMouse;

    // the start and end indices of the URL in the text control
    long m_start,
         m_end;

private:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxTextUrlEvent)

public:
    // for wxWin RTTI only, don't use
    wxTextUrlEvent() : m_evtMouse(), m_start(0), m_end(0) { }
};

typedef void (wxEvtHandler::*wxTextUrlEventFunction)(wxTextUrlEvent&);

#define wxTextEventHandler(func) wxCommandEventHandler(func)
#define wxTextUrlEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxTextUrlEventFunction, &func)

#define wx__DECLARE_TEXTEVT(evt, id, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_TEXT_ ## evt, id, wxTextEventHandler(fn))

#define wx__DECLARE_TEXTURLEVT(evt, id, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_TEXT_ ## evt, id, wxTextUrlEventHandler(fn))

#define EVT_TEXT(id, fn) wx__DECLARE_TEXTEVT(UPDATED, id, fn)
#define EVT_TEXT_ENTER(id, fn) wx__DECLARE_TEXTEVT(ENTER, id, fn)
#define EVT_TEXT_URL(id, fn) wx__DECLARE_TEXTURLEVT(URL, id, fn)
#define EVT_TEXT_MAXLEN(id, fn) wx__DECLARE_TEXTEVT(MAXLEN, id, fn)

#if wxHAS_TEXT_WINDOW_STREAM

// ----------------------------------------------------------------------------
// wxStreamToTextRedirector: this class redirects all data sent to the given
// C++ stream to the wxTextCtrl given to its ctor during its lifetime.
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxStreamToTextRedirector
{
private:
    void Init(wxTextCtrl *text)
    {
        m_sbufOld = m_ostr.rdbuf();
        m_ostr.rdbuf(text);
    }

public:
    wxStreamToTextRedirector(wxTextCtrl *text)
        : m_ostr(wxSTD cout)
    {
        Init(text);
    }

    wxStreamToTextRedirector(wxTextCtrl *text, wxSTD ostream *ostr)
        : m_ostr(*ostr)
    {
        Init(text);
    }

    ~wxStreamToTextRedirector()
    {
        m_ostr.rdbuf(m_sbufOld);
    }

private:
    // the stream we're redirecting
    wxSTD ostream&   m_ostr;

    // the old streambuf (before we changed it)
    wxSTD streambuf *m_sbufOld;
};

#endif // wxHAS_TEXT_WINDOW_STREAM

#endif // wxUSE_TEXTCTRL

#endif
    // _WX_TEXTCTRL_H_BASE_
