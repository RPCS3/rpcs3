/////////////////////////////////////////////////////////////////////////////
// Name:        wx/richtext/richtextctrl.h
// Purpose:     A rich edit control
// Author:      Julian Smart
// Modified by:
// Created:     2005-09-30
// RCS-ID:      $Id: richtextctrl.h 62194 2009-09-29 06:45:04Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_RICHTEXTCTRL_H_
#define _WX_RICHTEXTCTRL_H_

#include "wx/richtext/richtextbuffer.h"

#if wxUSE_RICHTEXT

#include "wx/scrolwin.h"
#include "wx/caret.h"

#include "wx/textctrl.h"

#if !defined(__WXGTK__) && !defined(__WXMAC__)
#define wxRICHTEXT_BUFFERED_PAINTING 1
#else
#define wxRICHTEXT_BUFFERED_PAINTING 0
#endif

class WXDLLIMPEXP_FWD_RICHTEXT wxRichTextStyleDefinition;

/*!
 * Styles and flags
 */

/* Styles
 */

#define wxRE_READONLY          0x0010
#define wxRE_MULTILINE         0x0020
#define wxRE_CENTRE_CARET      0x8000
#define wxRE_CENTER_CARET      wxRE_CENTRE_CARET

/* Flags
 */

#define wxRICHTEXT_SHIFT_DOWN  0x01
#define wxRICHTEXT_CTRL_DOWN   0x02
#define wxRICHTEXT_ALT_DOWN    0x04

/* Defaults
 */

#define wxRICHTEXT_DEFAULT_OVERALL_SIZE wxSize(-1, -1)
#define wxRICHTEXT_DEFAULT_IMAGE_SIZE wxSize(80, 80)
#define wxRICHTEXT_DEFAULT_SPACING 3
#define wxRICHTEXT_DEFAULT_MARGIN 3
#define wxRICHTEXT_DEFAULT_UNFOCUSSED_BACKGROUND wxColour(175, 175, 175)
#define wxRICHTEXT_DEFAULT_FOCUSSED_BACKGROUND wxColour(140, 140, 140)
#define wxRICHTEXT_DEFAULT_UNSELECTED_BACKGROUND wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE)
#define wxRICHTEXT_DEFAULT_TYPE_COLOUR wxColour(0, 0, 200)
#define wxRICHTEXT_DEFAULT_FOCUS_RECT_COLOUR wxColour(100, 80, 80)
#define wxRICHTEXT_DEFAULT_CARET_WIDTH 2
// Minimum buffer size before delayed layout kicks in
#define wxRICHTEXT_DEFAULT_DELAYED_LAYOUT_THRESHOLD 20000
// Milliseconds before layout occurs after resize
#define wxRICHTEXT_DEFAULT_LAYOUT_INTERVAL 50

/*!
 * Forward declarations
 */

/*!
 * wxRichTextItem class declaration
 */

// Drawing styles/states
#define wxRICHTEXT_SELECTED    0x01
#define wxRICHTEXT_TAGGED      0x02
// The control is focussed
#define wxRICHTEXT_FOCUSSED    0x04
// The item itself has the focus
#define wxRICHTEXT_IS_FOCUS    0x08

/*!
 * wxRichTextCtrl class declaration
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextCtrl : public wxTextCtrlBase,
                                            public wxScrollHelper
{
    DECLARE_CLASS( wxRichTextCtrl )
    DECLARE_EVENT_TABLE()

public:
// Constructors

    wxRichTextCtrl( );
    wxRichTextCtrl( wxWindow* parent, wxWindowID id = -1, const wxString& value = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
        long style = wxRE_MULTILINE, const wxValidator& validator = wxDefaultValidator, const wxString& name = wxTextCtrlNameStr);

    virtual ~wxRichTextCtrl( );

// Operations

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& value = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
        long style = wxRE_MULTILINE, const wxValidator& validator = wxDefaultValidator, const wxString& name = wxTextCtrlNameStr );

    /// Member initialisation
    void Init();

///// wxTextCtrl compatibility

// Accessors

    virtual wxString GetValue() const;

    virtual wxString GetRange(long from, long to) const;

    virtual int GetLineLength(long lineNo) const ;
    virtual wxString GetLineText(long lineNo) const ;
    virtual int GetNumberOfLines() const ;

    virtual bool IsModified() const ;
    virtual bool IsEditable() const ;

    // more readable flag testing methods
    bool IsSingleLine() const { return !HasFlag(wxRE_MULTILINE); }
    bool IsMultiLine() const { return !IsSingleLine(); }

    // If the return values from and to are the same, there is no selection.
    virtual void GetSelection(long* from, long* to) const;

    virtual wxString GetStringSelection() const;

    /// Get filename
    wxString GetFilename() const { return m_filename; }

    /// Set filename
    void SetFilename(const wxString& filename) { m_filename = filename; }

    /// Set the threshold in character positions for doing layout optimization during sizing
    void SetDelayedLayoutThreshold(long threshold) { m_delayedLayoutThreshold = threshold; }

    /// Get the threshold in character positions for doing layout optimization during sizing
    long GetDelayedLayoutThreshold() const { return m_delayedLayoutThreshold; }

#if wxABI_VERSION >= 20808
    /// Set text cursor
    void SetTextCursor(const wxCursor& cursor ) { m_textCursor = cursor; }

    /// Get text cursor
    wxCursor GetTextCursor() const { return m_textCursor; }

    /// Set URL cursor
    void SetURLCursor(const wxCursor& cursor ) { m_urlCursor = cursor; }

    /// Get URL cursor
    wxCursor GetURLCursor() const { return m_urlCursor; }
#endif

#if wxABI_VERSION >= 20811
    /// Get/set context menu
    wxMenu* GetContextMenu() const { return m_contextMenu; }
    void SetContextMenu(wxMenu* menu);
#endif

// Operations

    // editing
    virtual void Clear();
    virtual void Replace(long from, long to, const wxString& value);
    virtual void Remove(long from, long to);

    // load/save the controls contents from/to the file
    virtual bool DoLoadFile(const wxString& file, int fileType);
    virtual bool DoSaveFile(const wxString& file = wxEmptyString, int fileType = wxRICHTEXT_TYPE_ANY);

    /// Set the handler flags, controlling loading and saving
    void SetHandlerFlags(int flags) { GetBuffer().SetHandlerFlags(flags); }

    /// Get the handler flags, controlling loading and saving
    int GetHandlerFlags() const { return GetBuffer().GetHandlerFlags(); }

    // sets/clears the dirty flag
    virtual void MarkDirty();
    virtual void DiscardEdits();

    // set the max number of characters which may be entered in a single line
    // text control
    virtual void SetMaxLength(unsigned long WXUNUSED(len)) { }

    // writing text inserts it at the current position, appending always
    // inserts it at the end
    virtual void WriteText(const wxString& text);
    virtual void AppendText(const wxString& text);

    // text control under some platforms supports the text styles: these
    // methods allow to apply the given text style to the given selection or to
    // set/get the style which will be used for all appended text
    virtual bool SetStyle(long start, long end, const wxTextAttr& style);
    virtual bool SetStyle(long start, long end, const wxTextAttrEx& style);
    virtual bool SetStyle(const wxRichTextRange& range, const wxRichTextAttr& style);
    virtual bool GetStyle(long position, wxTextAttr& style);
    virtual bool GetStyle(long position, wxTextAttrEx& style);
    virtual bool GetStyle(long position, wxRichTextAttr& style);

    // get the common set of styles for the range
    virtual bool GetStyleForRange(const wxRichTextRange& range, wxRichTextAttr& style);
    virtual bool GetStyleForRange(const wxRichTextRange& range, wxTextAttrEx& style);

    // extended style setting operation with flags including:
    // wxRICHTEXT_SETSTYLE_WITH_UNDO, wxRICHTEXT_SETSTYLE_OPTIMIZE, wxRICHTEXT_SETSTYLE_PARAGRAPHS_ONLY, wxRICHTEXT_SETSTYLE_CHARACTERS_ONLY
    // see richtextbuffer.h for more details.
    virtual bool SetStyleEx(long start, long end, const wxTextAttrEx& style, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO);
    virtual bool SetStyleEx(const wxRichTextRange& range, const wxTextAttrEx& style, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO);
    virtual bool SetStyleEx(const wxRichTextRange& range, const wxRichTextAttr& style, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO);

    /// Get the content (uncombined) attributes for this position.
    virtual bool GetUncombinedStyle(long position, wxTextAttr& style);
    virtual bool GetUncombinedStyle(long position, wxTextAttrEx& style);
    virtual bool GetUncombinedStyle(long position, wxRichTextAttr& style);

    virtual bool SetDefaultStyle(const wxTextAttrEx& style);
    virtual bool SetDefaultStyle(const wxTextAttr& style);

    // TODO: change to GetDefaultStyle if we merge wxTextAttr and wxTextAttrEx
    virtual const wxTextAttrEx& GetDefaultStyleEx() const;
    virtual const wxTextAttr& GetDefaultStyle() const;

    /// Set list style
    virtual bool SetListStyle(const wxRichTextRange& range, wxRichTextListStyleDefinition* def, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO, int startFrom = 1, int specifiedLevel = -1);
    virtual bool SetListStyle(const wxRichTextRange& range, const wxString& defName, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO, int startFrom = 1, int specifiedLevel = -1);

    /// Clear list for given range
    virtual bool ClearListStyle(const wxRichTextRange& range, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO);

    /// Number/renumber any list elements in the given range
    /// def/defName can be NULL/empty to indicate that the existing list style should be used.
    virtual bool NumberList(const wxRichTextRange& range, wxRichTextListStyleDefinition* def = NULL, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO, int startFrom = 1, int specifiedLevel = -1);
    virtual bool NumberList(const wxRichTextRange& range, const wxString& defName, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO, int startFrom = 1, int specifiedLevel = -1);

    /// Promote the list items within the given range. promoteBy can be a positive or negative number, e.g. 1 or -1
    /// def/defName can be NULL/empty to indicate that the existing list style should be used.
    virtual bool PromoteList(int promoteBy, const wxRichTextRange& range, wxRichTextListStyleDefinition* def = NULL, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO, int specifiedLevel = -1);
    virtual bool PromoteList(int promoteBy, const wxRichTextRange& range, const wxString& defName, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO, int specifiedLevel = -1);

    /// Deletes the content in the given range
    virtual bool Delete(const wxRichTextRange& range);

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
    virtual void DeleteSelection();

    virtual bool CanCopy() const;
    virtual bool CanCut() const;
    virtual bool CanPaste() const;
    virtual bool CanDeleteSelection() const;

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

    /// Call Freeze to prevent refresh
    virtual void Freeze();

    /// Call Thaw to refresh
    virtual void Thaw();

    /// Call Thaw to refresh
    virtual bool IsFrozen() const { return m_freezeCount > 0; }

    virtual bool HasSelection() const;

///// Functionality specific to wxRichTextCtrl

    /// Write an image at the current insertion point. Supply optional type to use
    /// for internal and file storage of the raw data.
    virtual bool WriteImage(const wxImage& image, int bitmapType = wxBITMAP_TYPE_PNG);

    /// Write a bitmap at the current insertion point. Supply optional type to use
    /// for internal and file storage of the raw data.
    virtual bool WriteImage(const wxBitmap& bitmap, int bitmapType = wxBITMAP_TYPE_PNG);

    /// Load an image from file and write at the current insertion point.
    virtual bool WriteImage(const wxString& filename, int bitmapType);

    /// Write an image block at the current insertion point.
    virtual bool WriteImage(const wxRichTextImageBlock& imageBlock);

    /// Insert a newline (actually paragraph) at the current insertion point.
    virtual bool Newline();

    /// Insert a line break at the current insertion point.
    virtual bool LineBreak();

    /// Set basic (overall) style
    virtual void SetBasicStyle(const wxTextAttrEx& style) { GetBuffer().SetBasicStyle(style); }
    virtual void SetBasicStyle(const wxRichTextAttr& style) { GetBuffer().SetBasicStyle(style); }

    /// Get basic (overall) style
    virtual const wxTextAttrEx& GetBasicStyle() const { return GetBuffer().GetBasicStyle(); }

    /// Begin using a style
    virtual bool BeginStyle(const wxTextAttrEx& style) { return GetBuffer().BeginStyle(style); }

    /// End the style
    virtual bool EndStyle() { return GetBuffer().EndStyle(); }

    /// End all styles
    virtual bool EndAllStyles() { return GetBuffer().EndAllStyles(); }

    /// Begin using bold
    bool BeginBold() { return GetBuffer().BeginBold(); }

    /// End using bold
    bool EndBold()  { return GetBuffer().EndBold(); }

    /// Begin using italic
    bool BeginItalic() { return GetBuffer().BeginItalic(); }

    /// End using italic
    bool EndItalic() { return GetBuffer().EndItalic(); }

    /// Begin using underline
    bool BeginUnderline() { return GetBuffer().BeginUnderline(); }

    /// End using underline
    bool EndUnderline() { return GetBuffer().EndUnderline(); }

    /// Begin using point size
    bool BeginFontSize(int pointSize) { return GetBuffer().BeginFontSize(pointSize); }

    /// End using point size
    bool EndFontSize() { return GetBuffer().EndFontSize(); }

    /// Begin using this font
    bool BeginFont(const wxFont& font) { return GetBuffer().BeginFont(font); }

    /// End using a font
    bool EndFont() { return GetBuffer().EndFont(); }

    /// Begin using this colour
    bool BeginTextColour(const wxColour& colour) { return GetBuffer().BeginTextColour(colour); }

    /// End using a colour
    bool EndTextColour() { return GetBuffer().EndTextColour(); }

    /// Begin using alignment
    bool BeginAlignment(wxTextAttrAlignment alignment) { return GetBuffer().BeginAlignment(alignment); }

    /// End alignment
    bool EndAlignment() { return GetBuffer().EndAlignment(); }

    /// Begin left indent
    bool BeginLeftIndent(int leftIndent, int leftSubIndent = 0) { return GetBuffer().BeginLeftIndent(leftIndent, leftSubIndent); }

    /// End left indent
    bool EndLeftIndent() { return GetBuffer().EndLeftIndent(); }

    /// Begin right indent
    bool BeginRightIndent(int rightIndent) { return GetBuffer().BeginRightIndent(rightIndent); }

    /// End right indent
    bool EndRightIndent() { return GetBuffer().EndRightIndent(); }

    /// Begin paragraph spacing
    bool BeginParagraphSpacing(int before, int after) { return GetBuffer().BeginParagraphSpacing(before, after); }

    /// End paragraph spacing
    bool EndParagraphSpacing() { return GetBuffer().EndParagraphSpacing(); }

    /// Begin line spacing
    bool BeginLineSpacing(int lineSpacing) { return GetBuffer().BeginLineSpacing(lineSpacing); }

    /// End line spacing
    bool EndLineSpacing() { return GetBuffer().EndLineSpacing(); }

    /// Begin numbered bullet
    bool BeginNumberedBullet(int bulletNumber, int leftIndent, int leftSubIndent, int bulletStyle = wxTEXT_ATTR_BULLET_STYLE_ARABIC|wxTEXT_ATTR_BULLET_STYLE_PERIOD)
    { return GetBuffer().BeginNumberedBullet(bulletNumber, leftIndent, leftSubIndent, bulletStyle); }

    /// End numbered bullet
    bool EndNumberedBullet() { return GetBuffer().EndNumberedBullet(); }

    /// Begin symbol bullet
    bool BeginSymbolBullet(const wxString& symbol, int leftIndent, int leftSubIndent, int bulletStyle = wxTEXT_ATTR_BULLET_STYLE_SYMBOL)
    { return GetBuffer().BeginSymbolBullet(symbol, leftIndent, leftSubIndent, bulletStyle); }

    /// End symbol bullet
    bool EndSymbolBullet() { return GetBuffer().EndSymbolBullet(); }

    /// Begin standard bullet
    bool BeginStandardBullet(const wxString& bulletName, int leftIndent, int leftSubIndent, int bulletStyle = wxTEXT_ATTR_BULLET_STYLE_STANDARD)
    { return GetBuffer().BeginStandardBullet(bulletName, leftIndent, leftSubIndent, bulletStyle); }

    /// End standard bullet
    bool EndStandardBullet() { return GetBuffer().EndStandardBullet(); }

    /// Begin named character style
    bool BeginCharacterStyle(const wxString& characterStyle) { return GetBuffer().BeginCharacterStyle(characterStyle); }

    /// End named character style
    bool EndCharacterStyle() { return GetBuffer().EndCharacterStyle(); }

    /// Begin named paragraph style
    bool BeginParagraphStyle(const wxString& paragraphStyle) { return GetBuffer().BeginParagraphStyle(paragraphStyle); }

    /// End named character style
    bool EndParagraphStyle() { return GetBuffer().EndParagraphStyle(); }

    /// Begin named list style
    bool BeginListStyle(const wxString& listStyle, int level = 1, int number = 1) { return GetBuffer().BeginListStyle(listStyle, level, number); }

    /// End named character style
    bool EndListStyle() { return GetBuffer().EndListStyle(); }

    /// Begin URL
    bool BeginURL(const wxString& url, const wxString& characterStyle = wxEmptyString) { return GetBuffer().BeginURL(url, characterStyle); }

    /// End URL
    bool EndURL() { return GetBuffer().EndURL(); }

    /// Sets the default style to the style under the cursor
    bool SetDefaultStyleToCursorStyle();

    /// Clear the selection
    virtual void SelectNone();

    /// Select the word at the given character position
    virtual bool SelectWord(long position);

    /// Get/set the selection range in character positions. -1, -1 means no selection.
    /// The range is in API convention, i.e. a single character selection is denoted
    /// by (n, n+1)
    wxRichTextRange GetSelectionRange() const;
    void SetSelectionRange(const wxRichTextRange& range);

    /// Get/set the selection range in character positions. -1, -1 means no selection.
    /// The range is in internal format, i.e. a single character selection is denoted
    /// by (n, n)
    const wxRichTextRange& GetInternalSelectionRange() const { return m_selectionRange; }
    void SetInternalSelectionRange(const wxRichTextRange& range) { m_selectionRange = range; }

    /// Add a new paragraph of text to the end of the buffer
    virtual wxRichTextRange AddParagraph(const wxString& text);

    /// Add an image
    virtual wxRichTextRange AddImage(const wxImage& image);

    /// Layout the buffer: which we must do before certain operations, such as
    /// setting the caret position.
    virtual bool LayoutContent(bool onlyVisibleRect = false);

    /// Move the caret to the given character position
    virtual bool MoveCaret(long pos, bool showAtLineStart = false);

    /// Move right
    virtual bool MoveRight(int noPositions = 1, int flags = 0);

    /// Move left
    virtual bool MoveLeft(int noPositions = 1, int flags = 0);

    /// Move up
    virtual bool MoveUp(int noLines = 1, int flags = 0);

    /// Move up
    virtual bool MoveDown(int noLines = 1, int flags = 0);

    /// Move to the end of the line
    virtual bool MoveToLineEnd(int flags = 0);

    /// Move to the start of the line
    virtual bool MoveToLineStart(int flags = 0);

    /// Move to the end of the paragraph
    virtual bool MoveToParagraphEnd(int flags = 0);

    /// Move to the start of the paragraph
    virtual bool MoveToParagraphStart(int flags = 0);

    /// Move to the start of the buffer
    virtual bool MoveHome(int flags = 0);

    /// Move to the end of the buffer
    virtual bool MoveEnd(int flags = 0);

    /// Move n pages up
    virtual bool PageUp(int noPages = 1, int flags = 0);

    /// Move n pages down
    virtual bool PageDown(int noPages = 1, int flags = 0);

    /// Move n words left
    virtual bool WordLeft(int noPages = 1, int flags = 0);

    /// Move n words right
    virtual bool WordRight(int noPages = 1, int flags = 0);

    /// Returns the buffer associated with the control.
    wxRichTextBuffer& GetBuffer() { return m_buffer; }
    const wxRichTextBuffer& GetBuffer() const { return m_buffer; }

    /// Start batching undo history for commands.
    virtual bool BeginBatchUndo(const wxString& cmdName) { return m_buffer.BeginBatchUndo(cmdName); }

    /// End batching undo history for commands.
    virtual bool EndBatchUndo() { return m_buffer.EndBatchUndo(); }

    /// Are we batching undo history for commands?
    virtual bool BatchingUndo() const { return m_buffer.BatchingUndo(); }

    /// Start suppressing undo history for commands.
    virtual bool BeginSuppressUndo() { return m_buffer.BeginSuppressUndo(); }

    /// End suppressing undo history for commands.
    virtual bool EndSuppressUndo() { return m_buffer.EndSuppressUndo(); }

    /// Are we suppressing undo history for commands?
    virtual bool SuppressingUndo() const { return m_buffer.SuppressingUndo(); }

    /// Test if this whole range has character attributes of the specified kind. If any
    /// of the attributes are different within the range, the test fails. You
    /// can use this to implement, for example, bold button updating. style must have
    /// flags indicating which attributes are of interest.
    virtual bool HasCharacterAttributes(const wxRichTextRange& range, const wxTextAttrEx& style) const
    {
        return GetBuffer().HasCharacterAttributes(range.ToInternal(), style);
    }
    virtual bool HasCharacterAttributes(const wxRichTextRange& range, const wxRichTextAttr& style) const
    {
        return GetBuffer().HasCharacterAttributes(range.ToInternal(), style);
    }

    /// Test if this whole range has paragraph attributes of the specified kind. If any
    /// of the attributes are different within the range, the test fails. You
    /// can use this to implement, for example, centering button updating. style must have
    /// flags indicating which attributes are of interest.
    virtual bool HasParagraphAttributes(const wxRichTextRange& range, const wxTextAttrEx& style) const
    {
        return GetBuffer().HasParagraphAttributes(range.ToInternal(), style);
    }
    virtual bool HasParagraphAttributes(const wxRichTextRange& range, const wxRichTextAttr& style) const
    {
        return GetBuffer().HasParagraphAttributes(range.ToInternal(), style);
    }

    /// Is all of the selection bold?
    virtual bool IsSelectionBold();

    /// Is all of the selection italics?
    virtual bool IsSelectionItalics();

    /// Is all of the selection underlined?
    virtual bool IsSelectionUnderlined();

    /// Is all of the selection aligned according to the specified flag?
    virtual bool IsSelectionAligned(wxTextAttrAlignment alignment);

    /// Apply bold to the selection
    virtual bool ApplyBoldToSelection();

    /// Apply italic to the selection
    virtual bool ApplyItalicToSelection();

    /// Apply underline to the selection
    virtual bool ApplyUnderlineToSelection();

    /// Apply alignment to the selection
    virtual bool ApplyAlignmentToSelection(wxTextAttrAlignment alignment);

    /// Apply a named style to the selection
    virtual bool ApplyStyle(wxRichTextStyleDefinition* def);

    /// Set style sheet, if any
    void SetStyleSheet(wxRichTextStyleSheet* styleSheet) { GetBuffer().SetStyleSheet(styleSheet); }
    wxRichTextStyleSheet* GetStyleSheet() const { return GetBuffer().GetStyleSheet(); }

    /// Push style sheet to top of stack
    bool PushStyleSheet(wxRichTextStyleSheet* styleSheet) { return GetBuffer().PushStyleSheet(styleSheet); }

    /// Pop style sheet from top of stack
    wxRichTextStyleSheet* PopStyleSheet() { return GetBuffer().PopStyleSheet(); }

    /// Apply the style sheet to the buffer, for example if the styles have changed.
    bool ApplyStyleSheet(wxRichTextStyleSheet* styleSheet = NULL);

// Command handlers

    void Command(wxCommandEvent& event);
    void OnDropFiles(wxDropFilesEvent& event);
    void OnCaptureLost(wxMouseCaptureLostEvent& event);

    void OnCut(wxCommandEvent& event);
    void OnCopy(wxCommandEvent& event);
    void OnPaste(wxCommandEvent& event);
    void OnUndo(wxCommandEvent& event);
    void OnRedo(wxCommandEvent& event);
    void OnSelectAll(wxCommandEvent& event);
    void OnClear(wxCommandEvent& event);

    void OnUpdateCut(wxUpdateUIEvent& event);
    void OnUpdateCopy(wxUpdateUIEvent& event);
    void OnUpdatePaste(wxUpdateUIEvent& event);
    void OnUpdateUndo(wxUpdateUIEvent& event);
    void OnUpdateRedo(wxUpdateUIEvent& event);
    void OnUpdateSelectAll(wxUpdateUIEvent& event);
    void OnUpdateClear(wxUpdateUIEvent& event);

    // Show a context menu for Rich Edit controls (the standard
    // EDIT control has one already)
    void OnContextMenu(wxContextMenuEvent& event);

// Event handlers

    /// Painting
    void OnPaint(wxPaintEvent& event);
    void OnEraseBackground(wxEraseEvent& event);

    /// Left-click
    void OnLeftClick(wxMouseEvent& event);

    /// Left-up
    void OnLeftUp(wxMouseEvent& event);

    /// Motion
    void OnMoveMouse(wxMouseEvent& event);

    /// Left-double-click
    void OnLeftDClick(wxMouseEvent& event);

    /// Middle-click
    void OnMiddleClick(wxMouseEvent& event);

    /// Right-click
    void OnRightClick(wxMouseEvent& event);

    /// Key press
    void OnChar(wxKeyEvent& event);

    /// Sizing
    void OnSize(wxSizeEvent& event);

    /// Setting/losing focus
    void OnSetFocus(wxFocusEvent& event);
    void OnKillFocus(wxFocusEvent& event);

    /// Idle-time processing
    void OnIdle(wxIdleEvent& event);

    /// Scrolling
    void OnScroll(wxScrollWinEvent& event);

    /// Set font, and also default attributes
    virtual bool SetFont(const wxFont& font);

    /// Set up scrollbars, e.g. after a resize
    virtual void SetupScrollbars(bool atTop = false);

    /// Keyboard navigation
    virtual bool KeyboardNavigate(int keyCode, int flags);

    /// Paint the background
    virtual void PaintBackground(wxDC& dc);

#if wxRICHTEXT_BUFFERED_PAINTING
    /// Recreate buffer bitmap if necessary
    virtual bool RecreateBuffer(const wxSize& size = wxDefaultSize);
#endif

    /// Set the selection
    virtual void DoSetSelection(long from, long to, bool scrollCaret = true);

    /// Write text
    virtual void DoWriteText(const wxString& value, int flags = 0);

    /// Should we inherit colours?
    virtual bool ShouldInheritColours() const { return false; }

    /// Position the caret
    virtual void PositionCaret();

    /// Extend the selection, returning true if the selection was
    /// changed. Selections are in caret positions.
    virtual bool ExtendSelection(long oldPosition, long newPosition, int flags);

    /// Scroll into view. This takes a _caret_ position.
    virtual bool ScrollIntoView(long position, int keyCode);

    /// The caret position is the character position just before the caret.
    /// A value of -1 means the caret is at the start of the buffer.
    void SetCaretPosition(long position, bool showAtLineStart = false) ;
    long GetCaretPosition() const { return m_caretPosition; }

    /// The adjusted caret position is the character position adjusted to take
    /// into account whether we're at the start of a paragraph, in which case
    /// style information should be taken from the next position, not current one.
    long GetAdjustedCaretPosition(long caretPos) const;

    /// Move caret one visual step forward: this may mean setting a flag
    /// and keeping the same position if we're going from the end of one line
    /// to the start of the next, which may be the exact same caret position.
    void MoveCaretForward(long oldPosition) ;

    /// Move caret one visual step forward: this may mean setting a flag
    /// and keeping the same position if we're going from the end of one line
    /// to the start of the next, which may be the exact same caret position.
    void MoveCaretBack(long oldPosition) ;

    /// Get the caret height and position for the given character position
    bool GetCaretPositionForIndex(long position, wxRect& rect);

    /// Gets the line for the visible caret position. If the caret is
    /// shown at the very end of the line, it means the next character is actually
    /// on the following line. So let's get the line we're expecting to find
    /// if this is the case.
    wxRichTextLine* GetVisibleLineForCaretPosition(long caretPosition) const;

    /// Gets the command processor
    wxCommandProcessor* GetCommandProcessor() const { return GetBuffer().GetCommandProcessor(); }

    /// Delete content if there is a selection, e.g. when pressing a key.
    /// Returns the new caret position in newPos, or leaves it if there
    /// was no action.
    bool DeleteSelectedContent(long* newPos= NULL);

    /// Transform logical to physical
    wxPoint GetPhysicalPoint(const wxPoint& ptLogical) const;

    /// Transform physical to logical
    wxPoint GetLogicalPoint(const wxPoint& ptPhysical) const;

    /// Finds the caret position for the next word. Direction
    /// is 1 (forward) or -1 (backwards).
    virtual long FindNextWordPosition(int direction = 1) const;

    /// Is the given position visible on the screen?
    bool IsPositionVisible(long pos) const;

    /// Returns the first visible position in the current view
    long GetFirstVisiblePosition() const;

    /// Returns the caret position since the default formatting was changed. As
    /// soon as this position changes, we no longer reflect the default style
    /// in the UI. A value of -2 means that we should only reflect the style of the
    /// content under the caret.
    long GetCaretPositionForDefaultStyle() const { return m_caretPositionForDefaultStyle; }

    /// Set the caret position for the default style that the user is selecting.
    void SetCaretPositionForDefaultStyle(long pos) { m_caretPositionForDefaultStyle = pos; }

    /// Should the UI reflect the default style chosen by the user, rather than the style under
    /// the caret?
    bool IsDefaultStyleShowing() const { return m_caretPositionForDefaultStyle != -2; }

    /// Convenience function that tells the control to start reflecting the default
    /// style, since the user is changing it.
    void SetAndShowDefaultStyle(const wxRichTextAttr& attr)
    {
        SetDefaultStyle(attr);
        SetCaretPositionForDefaultStyle(GetCaretPosition());
    }

    /// Get the first visible point in the window
    wxPoint GetFirstVisiblePoint() const;

// Implementation

     /// Font names take a long time to retrieve, so cache them (on demand)
     static const wxArrayString& GetAvailableFontNames();
     static void ClearAvailableFontNames();

     WX_FORWARD_TO_SCROLL_HELPER()

// Overrides
protected:

    virtual wxSize DoGetBestSize() const ;

    virtual void DoSetValue(const wxString& value, int flags = 0);


// Data members
private:

    /// Allows nested Freeze/Thaw
    int                     m_freezeCount;

#if wxRICHTEXT_BUFFERED_PAINTING
    /// Buffer bitmap
    wxBitmap                m_bufferBitmap;
#endif

    /// Text buffer
    wxRichTextBuffer        m_buffer;

    wxMenu*                 m_contextMenu;

    /// Caret position (1 less than the character position, so -1 is the
    /// first caret position).
    long                    m_caretPosition;

    /// Caret position when the default formatting has been changed. As
    /// soon as this position changes, we no longer reflect the default style
    /// in the UI.
    long                    m_caretPositionForDefaultStyle;

    /// Selection range in character positions. -2, -2 means no selection.
    wxRichTextRange         m_selectionRange;

    /// Anchor so we know how to extend the selection
    /// It's a caret position since it's between two characters.
    long                    m_selectionAnchor;

    /// Are we editable?
    bool                    m_editable;

    /// Are we showing the caret position at the start of a line
    /// instead of at the end of the previous one?
    bool                    m_caretAtLineStart;

    /// Are we dragging a selection?
    bool                    m_dragging;

    /// Start position for drag
    wxPoint                 m_dragStart;

    /// Do we need full layout in idle?
    bool                    m_fullLayoutRequired;
    wxLongLong              m_fullLayoutTime;
    long                    m_fullLayoutSavedPosition;

    /// Threshold for doing delayed layout
    long                    m_delayedLayoutThreshold;

    /// Cursors
    wxCursor                m_textCursor;
    wxCursor                m_urlCursor;

    static wxArrayString    sm_availableFontNames;
};

/*!
 * wxRichTextEvent - the event class for wxRichTextCtrl notifications
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextEvent : public wxNotifyEvent
{
public:
    wxRichTextEvent(wxEventType commandType = wxEVT_NULL, int winid = 0)
        : wxNotifyEvent(commandType, winid),
        m_flags(0), m_position(-1), m_oldStyleSheet(NULL), m_newStyleSheet(NULL),
        m_char((wxChar) 0)
        { }

    wxRichTextEvent(const wxRichTextEvent& event)
        : wxNotifyEvent(event),
        m_flags(event.m_flags), m_position(-1),
        m_oldStyleSheet(event.m_oldStyleSheet), m_newStyleSheet(event.m_newStyleSheet),
        m_char((wxChar) 0)
        { }

    long GetPosition() const { return m_position; }
    void SetPosition(long pos) { m_position = pos; }

    int GetFlags() const { return m_flags; }
    void SetFlags(int flags) { m_flags = flags; }

    wxRichTextStyleSheet* GetOldStyleSheet() const { return m_oldStyleSheet; }
    void SetOldStyleSheet(wxRichTextStyleSheet* sheet) { m_oldStyleSheet = sheet; }

    wxRichTextStyleSheet* GetNewStyleSheet() const { return m_newStyleSheet; }
    void SetNewStyleSheet(wxRichTextStyleSheet* sheet) { m_newStyleSheet = sheet; }

    const wxRichTextRange& GetRange() const { return m_range; }
    void SetRange(const wxRichTextRange& range) { m_range = range; }

    wxChar GetCharacter() const { return m_char; }
    void SetCharacter(wxChar ch) { m_char = ch; }

    virtual wxEvent *Clone() const { return new wxRichTextEvent(*this); }

protected:
    int                     m_flags;
    long                    m_position;
    wxRichTextStyleSheet*   m_oldStyleSheet;
    wxRichTextStyleSheet*   m_newStyleSheet;
    wxRichTextRange         m_range;
    wxChar                  m_char;

private:
    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxRichTextEvent)
};

/*!
 * wxRichTextCtrl event macros
 */

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_RICHTEXT, wxEVT_COMMAND_RICHTEXT_LEFT_CLICK, 2602)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_RICHTEXT, wxEVT_COMMAND_RICHTEXT_RIGHT_CLICK, 2603)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_RICHTEXT, wxEVT_COMMAND_RICHTEXT_MIDDLE_CLICK, 2604)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_RICHTEXT, wxEVT_COMMAND_RICHTEXT_LEFT_DCLICK, 2605)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_RICHTEXT, wxEVT_COMMAND_RICHTEXT_RETURN, 2606)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_RICHTEXT, wxEVT_COMMAND_RICHTEXT_CHARACTER, 2607)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_RICHTEXT, wxEVT_COMMAND_RICHTEXT_DELETE, 2608)

    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_RICHTEXT, wxEVT_COMMAND_RICHTEXT_STYLESHEET_CHANGING, 2609)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_RICHTEXT, wxEVT_COMMAND_RICHTEXT_STYLESHEET_CHANGED, 2610)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_RICHTEXT, wxEVT_COMMAND_RICHTEXT_STYLESHEET_REPLACING, 2611)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_RICHTEXT, wxEVT_COMMAND_RICHTEXT_STYLESHEET_REPLACED, 2612)

    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_RICHTEXT, wxEVT_COMMAND_RICHTEXT_CONTENT_INSERTED, 2613)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_RICHTEXT, wxEVT_COMMAND_RICHTEXT_CONTENT_DELETED, 2614)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_RICHTEXT, wxEVT_COMMAND_RICHTEXT_STYLE_CHANGED, 2615)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_RICHTEXT, wxEVT_COMMAND_RICHTEXT_SELECTION_CHANGED, 2616)

#if wxABI_VERSION >= 20808
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_RICHTEXT, wxEVT_COMMAND_RICHTEXT_BUFFER_RESET, 2617)
#endif
END_DECLARE_EVENT_TYPES()

typedef void (wxEvtHandler::*wxRichTextEventFunction)(wxRichTextEvent&);

#define wxRichTextEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxRichTextEventFunction, &func)

#define EVT_RICHTEXT_LEFT_CLICK(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_RICHTEXT_LEFT_CLICK, id, -1, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxRichTextEventFunction, & fn ), NULL ),
#define EVT_RICHTEXT_RIGHT_CLICK(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_RICHTEXT_RIGHT_CLICK, id, -1, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxRichTextEventFunction, & fn ), NULL ),
#define EVT_RICHTEXT_MIDDLE_CLICK(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_RICHTEXT_MIDDLE_CLICK, id, -1, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxRichTextEventFunction, & fn ), NULL ),
#define EVT_RICHTEXT_LEFT_DCLICK(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_RICHTEXT_LEFT_DCLICK, id, -1, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxRichTextEventFunction, & fn ), NULL ),
#define EVT_RICHTEXT_RETURN(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_RICHTEXT_RETURN, id, -1, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxRichTextEventFunction, & fn ), NULL ),
#define EVT_RICHTEXT_CHARACTER(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_RICHTEXT_CHARACTER, id, -1, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxRichTextEventFunction, & fn ), NULL ),
#define EVT_RICHTEXT_DELETE(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_RICHTEXT_DELETE, id, -1, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxRichTextEventFunction, & fn ), NULL ),

#define EVT_RICHTEXT_STYLESHEET_CHANGING(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_RICHTEXT_STYLESHEET_CHANGING, id, -1, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxRichTextEventFunction, & fn ), NULL ),
#define EVT_RICHTEXT_STYLESHEET_CHANGED(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_RICHTEXT_STYLESHEET_CHANGED, id, -1, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxRichTextEventFunction, & fn ), NULL ),
#define EVT_RICHTEXT_STYLESHEET_REPLACING(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_RICHTEXT_STYLESHEET_REPLACING, id, -1, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxRichTextEventFunction, & fn ), NULL ),
#define EVT_RICHTEXT_STYLESHEET_REPLACED(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_RICHTEXT_STYLESHEET_REPLACED, id, -1, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxRichTextEventFunction, & fn ), NULL ),

#define EVT_RICHTEXT_CONTENT_INSERTED(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_RICHTEXT_CONTENT_INSERTED, id, -1, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxRichTextEventFunction, & fn ), NULL ),
#define EVT_RICHTEXT_CONTENT_DELETED(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_RICHTEXT_CONTENT_DELETED, id, -1, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxRichTextEventFunction, & fn ), NULL ),
#define EVT_RICHTEXT_STYLE_CHANGED(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_RICHTEXT_STYLE_CHANGED, id, -1, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxRichTextEventFunction, & fn ), NULL ),
#define EVT_RICHTEXT_SELECTION_CHANGED(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_RICHTEXT_SELECTION_CHANGED, id, -1, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxRichTextEventFunction, & fn ), NULL ),
#define EVT_RICHTEXT_BUFFER_RESET(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_RICHTEXT_BUFFER_RESET, id, -1, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxRichTextEventFunction, & fn ), NULL ),

#endif
    // wxUSE_RICHTEXT

#endif
    // _WX_RICHTEXTCTRL_H_
