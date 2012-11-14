/////////////////////////////////////////////////////////////////////////////
// Name:        src/univ/textctrl.cpp
// Purpose:     wxTextCtrl
// Author:      Vadim Zeitlin
// Modified by:
// Created:     15.09.00
// RCS-ID:      $Id: textctrl.cpp 48721 2007-09-16 11:04:26Z VZ $
// Copyright:   (c) 2000 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/*
   TODO

+  1. update vert scrollbar when any line length changes for WrapLines()
+  2. cursor movement ("Hello,^" -> "^verse!" on Arrow Down)?
     -> maybe save the x position and use it instead of current in handling
        DOWN/UP actions (this would make up/down always return the cursor to
        the same location)?
   3. split file into chunks
+? 4. rewrite Replace() refresh logic to deal with wrapping lines
+? 5. cache info found by GetPartOfWrappedLine() - performance must be horrible
      with lots of text

   6. backspace refreshes too much (until end of line)
 */

/*
   Optimisation hints from PureQuantify:

  +1. wxStringTokenize is the slowest part of Replace
   2. GetDC/ReleaseDC are very slow, avoid calling them several times
  +3. GetCharHeight() should be cached too
   4. wxClientDC construction/destruction in HitTestLine is horribly expensive

   For line wrapping controls HitTest2 takes 50% of program time. The results
   of GetRowsPerLine and GetPartOfWrappedLine *MUST* be cached.

   Search for "OPT!" for things which must be optimized.
 */

/*
   Some terminology:

   Everywhere in this file LINE refers to a logical line of text, and ROW to a
   physical line of text on the display. They are the same unless WrapLines()
   is true in which case a single LINE may correspond to multiple ROWs.

   A text position is an unsigned int (which for reasons of compatibility is
   still a long as wxTextPos) from 0 to GetLastPosition() inclusive. The positions
   correspond to the gaps between the letters so the position 0 is just
   before the first character and the last position is the one beyond the last
   character. For an empty text control GetLastPosition() returns 0.

   Lines and columns returned/accepted by XYToPosition() and PositionToXY()
   start from 0. The y coordinate is a LINE, not a ROW. Columns correspond to
   the characters, the first column of a line is the first character in it,
   the last one is length(line text). For compatibility, again, lines and
   columns are also longs.

   When translating lines/column coordinates to/from positions, the line and
   column give the character after the given position. Thus, GetLastPosition()
   doesn't have any corresponding column.

   An example of positions and lines/columns for a control without wrapping
   containing the text "Hello, Universe!\nGoodbye"

                               1 1 1 1 1 1 1
   pos:    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6
            H e l l o ,   U n i v e r s e !         line 0
   col:     0 1 2 3 4 5 6 7 8 9 1 1 1 1 1 1
                                0 1 2 3 4 5

   pos:    1 1 1 2 2 2 2 2
           7 8 9 0 1 2 3 4
            G o o d b y e                           line 1
   col:     0 1 2 3 4 5 6


   The same example for a control with line wrap assuming "Universe" is too
   long to fit on the same line with "Hello,":

   pos:    0 1 2 3 4 5
            H e l l o ,                             line 0 (row 0)
   col:     0 1 2 3 4 5

                   1 1 1 1 1 1 1
   pos:    6 7 8 9 0 1 2 3 4 5 6
              U n i v e r s e !                     line 0 (row 1)
   col:     6 7 8 9 1 1 1 1 1 1
                    0 1 2 3 4 5

   (line 1 == row 2 same as above)

   Note that there is still the same number of columns and positions and that
   there is no (logical) position at the end of the first ROW. This position
   is identified with the preceding one (which is not how Windows does it: it
   identifies it with the next one, i.e. the first position of the next line,
   but much more logical IMHO).
 */

/*
   Search for "OPT" for possible optimizations

   A possible global optimization would be to always store the coords in the
   text in triplets (pos, col, line) and update them simultaneously instead of
   recalculating col and line from pos each time it is needed. Currently we
   only do it for the current position but we might also do it for the
   selection start and end.
 */

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_TEXTCTRL

#include "wx/textctrl.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/dcclient.h"
    #include "wx/validate.h"
    #include "wx/dataobj.h"
#endif

#include <ctype.h>

#include "wx/clipbrd.h"

#include "wx/textfile.h"

#include "wx/caret.h"

#include "wx/univ/inphand.h"
#include "wx/univ/renderer.h"
#include "wx/univ/colschem.h"
#include "wx/univ/theme.h"

#include "wx/cmdproc.h"

// turn extra wxTextCtrl-specific debugging on/off
#define WXDEBUG_TEXT

// turn wxTextCtrl::Replace() debugging on (slows down code a *lot*!)
#define WXDEBUG_TEXT_REPLACE

#ifndef __WXDEBUG__
    #undef WXDEBUG_TEXT
    #undef WXDEBUG_TEXT_REPLACE
#endif

// wxStringTokenize only needed for debug checks
#ifdef WXDEBUG_TEXT_REPLACE
    #include "wx/tokenzr.h"
#endif // WXDEBUG_TEXT_REPLACE

// ----------------------------------------------------------------------------
// wxStdTextCtrlInputHandler: this control handles only the mouse/kbd actions
// common to Win32 and GTK, platform-specific things are implemented elsewhere
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxStdTextCtrlInputHandler : public wxStdInputHandler
{
public:
    wxStdTextCtrlInputHandler(wxInputHandler *inphand);

    virtual bool HandleKey(wxInputConsumer *consumer,
                           const wxKeyEvent& event,
                           bool pressed);
    virtual bool HandleMouse(wxInputConsumer *consumer,
                             const wxMouseEvent& event);
    virtual bool HandleMouseMove(wxInputConsumer *consumer,
                                 const wxMouseEvent& event);
    virtual bool HandleFocus(wxInputConsumer *consumer, const wxFocusEvent& event);

protected:
    // get the position of the mouse click
    static wxTextPos HitTest(const wxTextCtrl *text, const wxPoint& pos);

    // capture data
    wxTextCtrl *m_winCapture;
};

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// exchange two positions so that from is always less than or equal to to
static inline void OrderPositions(wxTextPos& from, wxTextPos& to)
{
    if ( from > to )
    {
        wxTextPos tmp = from;
        from = to;
        to = tmp;
    }
}

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// names of text ctrl commands
#define wxTEXT_COMMAND_INSERT _T("insert")
#define wxTEXT_COMMAND_REMOVE _T("remove")

// the value which is never used for text position, even not -1 which is
// sometimes used for some special meaning
static const wxTextPos INVALID_POS_VALUE = wxInvalidTextCoord;

// overlap between pages (when using PageUp/Dn) in lines
static const size_t PAGE_OVERLAP_IN_LINES = 1;

// ----------------------------------------------------------------------------
// private data of wxTextCtrl
// ----------------------------------------------------------------------------

// the data only used by single line text controls
struct wxTextSingleLineData
{
    // the position of the first visible pixel and the first visible column
    wxCoord m_ofsHorz;
    wxTextCoord m_colStart;

    // and the last ones (m_posLastVisible is the width but m_colLastVisible
    // is an absolute value)
    wxCoord m_posLastVisible;
    wxTextCoord m_colLastVisible;

    // def ctor
    wxTextSingleLineData()
    {
        m_colStart = 0;
        m_ofsHorz = 0;

        m_colLastVisible = -1;
        m_posLastVisible = -1;
    }

};

// the data only used by multi line text controls
struct wxTextMultiLineData
{
    // the lines of text
    wxArrayString m_lines;

    // the current ranges of the scrollbars
    int m_scrollRangeX,
        m_scrollRangeY;

    // should we adjust the horz/vert scrollbar?
    bool m_updateScrollbarX,
         m_updateScrollbarY;

    // the max line length in pixels
    wxCoord m_widthMax;

    // the index of the line which has the length of m_widthMax
    wxTextCoord m_lineLongest;

    // the rect in which text appears: it is even less than m_rectText because
    // only the last _complete_ line is shown, hence there is an unoccupied
    // horizontal band at the bottom of it
    wxRect m_rectTextReal;

    // the x-coordinate of the caret before we started moving it vertically:
    // this is used to ensure that moving the caret up and then down will
    // return it to the same position as if we always round it in one direction
    // we would shift it in that direction
    //
    // when m_xCaret == -1, we don't have any remembered position
    wxCoord m_xCaret;

    // the def ctor
    wxTextMultiLineData()
    {
        m_scrollRangeX =
        m_scrollRangeY = 0;

        m_updateScrollbarX =
        m_updateScrollbarY = false;

        m_widthMax = -1;
        m_lineLongest = 0;

        m_xCaret = -1;
    }
};

// the data only used by multi line text controls in line wrap mode
class wxWrappedLineData
{
    // these functions set all our values, so give them access to them
friend void wxTextCtrl::LayoutLine(wxTextCoord line,
                                    wxWrappedLineData& lineData) const;
friend void wxTextCtrl::LayoutLines(wxTextCoord) const;

public:
    // def ctor
    wxWrappedLineData()
    {
        m_rowFirst = -1;
    }

    // get the start of any row (remember that accessing m_rowsStart doesn't work
    // for the first one)
    wxTextCoord GetRowStart(wxTextCoord row) const
    {
        wxASSERT_MSG( IsValid(), _T("this line hadn't been laid out") );

        return row ? m_rowsStart[row - 1] : 0;
    }

    // get the length of the row (using the total line length which we don't
    // have here but need to calculate the length of the last row, so it must
    // be given to us)
    wxTextCoord GetRowLength(wxTextCoord row, wxTextCoord lenLine) const
    {
        wxASSERT_MSG( IsValid(), _T("this line hadn't been laid out") );

        // note that m_rowsStart[row] is the same as GetRowStart(row + 1) (but
        // slightly more efficient) and lenLine is the same as the start of the
        // first row of the next line
        return ((size_t)row == m_rowsStart.GetCount() ? lenLine : m_rowsStart[row])
                    - GetRowStart(row);
    }

    // return the width of the row in pixels
    wxCoord GetRowWidth(wxTextCoord row) const
    {
        wxASSERT_MSG( IsValid(), _T("this line hadn't been laid out") );

        return m_rowsWidth[row];
    }

    // return the number of rows
    size_t GetRowCount() const
    {
        wxASSERT_MSG( IsValid(), _T("this line hadn't been laid out") );

        return m_rowsStart.GetCount() + 1;
    }

    // return the number of additional (i.e. after the first one) rows
    size_t GetExtraRowCount() const
    {
        wxASSERT_MSG( IsValid(), _T("this line hadn't been laid out") );

        return m_rowsStart.GetCount();
    }

    // return the first row of this line
    wxTextCoord GetFirstRow() const
    {
        wxASSERT_MSG( IsValid(), _T("this line hadn't been laid out") );

        return m_rowFirst;
    }

    // return the first row of the next line
    wxTextCoord GetNextRow() const
    {
        wxASSERT_MSG( IsValid(), _T("this line hadn't been laid out") );

        return m_rowFirst + m_rowsStart.GetCount() + 1;
    }

    // this just provides direct access to m_rowsStart aerray for efficiency
    wxTextCoord GetExtraRowStart(wxTextCoord row) const
    {
        wxASSERT_MSG( IsValid(), _T("this line hadn't been laid out") );

        return m_rowsStart[row];
    }

    // this code is unused any longer
#if 0
    // return true if the column is in the start of the last row (hence the row
    // it is in is not wrapped)
    bool IsLastRow(wxTextCoord colRowStart) const
    {
        return colRowStart == GetRowStart(m_rowsStart.GetCount());
    }

    // return true if the column is the last column of the row starting in
    // colRowStart
    bool IsLastColInRow(wxTextCoord colRowStart,
                        wxTextCoord colRowEnd,
                        wxTextCoord lenLine) const
    {
        // find the row which starts with colRowStart
        size_t nRows = GetRowCount();
        for ( size_t n = 0; n < nRows; n++ )
        {
            if ( GetRowStart(n) == colRowStart )
            {
                wxTextCoord colNextRowStart = n == nRows - 1
                                                ? lenLine
                                                : GetRowStart(n + 1);

                wxASSERT_MSG( colRowEnd < colNextRowStart,
                              _T("this column is not in this row at all!") );

                return colRowEnd == colNextRowStart - 1;
            }
        }

        // caller got it wrong
        wxFAIL_MSG( _T("this column is not in the start of the row!") );

        return false;
    }
#endif // 0

    // is this row the last one in its line?
    bool IsLastRow(wxTextCoord row) const
    {
        return (size_t)row == GetExtraRowCount();
    }

    // the line is valid if it had been laid out correctly: note that just
    // shiwting the line (because one of previous lines changed) doesn't make
    // it invalid
    bool IsValid() const { return !m_rowsWidth.IsEmpty(); }

    // invalidating line will relayout it
    void Invalidate() { m_rowsWidth.Empty(); }

private:
    // for each line we remember the starting columns of all its rows after the
    // first one (which always starts at 0), i.e. if a line is wrapped twice
    // (== takes 3 rows) its m_rowsStart[0] may be 10 and m_rowsStart[1] == 15
    wxArrayLong m_rowsStart;

    // and the width of each row in pixels (this array starts from 0, as usual)
    wxArrayInt m_rowsWidth;

    // and also its starting row (0 for the first line, first lines'
    // m_rowsStart.GetCount() + 1 for the second &c): it is set to -1 initially
    // and this means that the struct hadn't yet been initialized
    wxTextCoord m_rowFirst;

    // the last modification "time"-stamp used by LayoutLines()
    size_t m_timestamp;
};

WX_DECLARE_OBJARRAY(wxWrappedLineData, wxArrayWrappedLinesData);
#include "wx/arrimpl.cpp"
WX_DEFINE_OBJARRAY(wxArrayWrappedLinesData);

struct wxTextWrappedData : public wxTextMultiLineData
{
    // the width of the column to the right of the text rect used for the
    // indicator mark display for the wrapped lines
    wxCoord m_widthMark;

    // the data for each line
    wxArrayWrappedLinesData m_linesData;

    // flag telling us to recalculate all starting rows starting from this line
    // (if it is -1, we don't have to recalculate anything) - it is set when
    // the number of the rows in the middle of the control changes
    wxTextCoord m_rowFirstInvalid;

    // the current timestamp used by LayoutLines()
    size_t m_timestamp;

    // invalidate starting rows of all lines (NOT rows!) after this one
    void InvalidateLinesBelow(wxTextCoord line)
    {
        if ( m_rowFirstInvalid == -1 || m_rowFirstInvalid > line )
        {
            m_rowFirstInvalid = line;
        }
    }

    // check if this line is valid: i.e. before the first invalid one
    bool IsValidLine(wxTextCoord line) const
    {
        return ((m_rowFirstInvalid == -1) || (line < m_rowFirstInvalid)) &&
                    m_linesData[line].IsValid();
    }

    // def ctor
    wxTextWrappedData()
    {
        m_widthMark = 0;
        m_rowFirstInvalid = -1;
        m_timestamp = 0;
    }
};

// ----------------------------------------------------------------------------
// private classes for undo/redo management
// ----------------------------------------------------------------------------

/*
   We use custom versions of wxWidgets command processor to implement undo/redo
   as we want to avoid storing the backpointer to wxTextCtrl in wxCommand
   itself: this is a waste of memory as all commands in the given command
   processor always have the same associated wxTextCtrl and so it makes sense
   to store the backpointer there.

   As for the rest of the implementation, it's fairly standard: we have 2
   command classes corresponding to adding and removing text.
 */

// a command corresponding to a wxTextCtrl action
class wxTextCtrlCommand : public wxCommand
{
public:
    wxTextCtrlCommand(const wxString& name) : wxCommand(true, name) { }

    // we don't use these methods as they don't make sense for us as we need a
    // wxTextCtrl to be applied
    virtual bool Do() { wxFAIL_MSG(_T("shouldn't be called")); return false; }
    virtual bool Undo() { wxFAIL_MSG(_T("shouldn't be called")); return false; }

    // instead, our command processor uses these methods
    virtual bool Do(wxTextCtrl *text) = 0;
    virtual bool Undo(wxTextCtrl *text) = 0;
};

// insert text command
class wxTextCtrlInsertCommand : public wxTextCtrlCommand
{
public:
    wxTextCtrlInsertCommand(const wxString& textToInsert)
        : wxTextCtrlCommand(wxTEXT_COMMAND_INSERT), m_text(textToInsert)
    {
        m_from = -1;
    }

    // combine the 2 commands together
    void Append(wxTextCtrlInsertCommand *other);

    virtual bool CanUndo() const;
    virtual bool Do(wxTextCtrl *text);
    virtual bool Do() { return wxTextCtrlCommand::Do(); }
    virtual bool Undo() { return wxTextCtrlCommand::Undo(); }
    virtual bool Undo(wxTextCtrl *text);

private:
    // the text we insert
    wxString m_text;

    // the position where we inserted the text
    wxTextPos m_from;
};

// remove text command
class wxTextCtrlRemoveCommand : public wxTextCtrlCommand
{
public:
    wxTextCtrlRemoveCommand(wxTextPos from, wxTextPos to)
        : wxTextCtrlCommand(wxTEXT_COMMAND_REMOVE)
    {
        m_from = from;
        m_to = to;
    }

    virtual bool CanUndo() const;
    virtual bool Do(wxTextCtrl *text);
    virtual bool Do() { return wxTextCtrlCommand::Do(); }
    virtual bool Undo() { return wxTextCtrlCommand::Undo(); }
    virtual bool Undo(wxTextCtrl *text);

private:
    // the range of text to delete
    wxTextPos m_from,
         m_to;

    // the text which was deleted when this command was Do()ne
    wxString m_textDeleted;
};

// a command processor for a wxTextCtrl
class wxTextCtrlCommandProcessor : public wxCommandProcessor
{
public:
    wxTextCtrlCommandProcessor(wxTextCtrl *text)
    {
        m_compressInserts = false;

        m_text = text;
    }

    // override Store() to compress multiple wxTextCtrlInsertCommand into one
    virtual void Store(wxCommand *command);

    // stop compressing insert commands when this is called
    void StopCompressing() { m_compressInserts = false; }

    // accessors
    wxTextCtrl *GetTextCtrl() const { return m_text; }
    bool IsCompressing() const { return m_compressInserts; }

protected:
    virtual bool DoCommand(wxCommand& cmd)
        { return ((wxTextCtrlCommand &)cmd).Do(m_text); }
    virtual bool UndoCommand(wxCommand& cmd)
        { return ((wxTextCtrlCommand &)cmd).Undo(m_text); }

    // check if this command is a wxTextCtrlInsertCommand and return it casted
    // to the right type if it is or NULL otherwise
    wxTextCtrlInsertCommand *IsInsertCommand(wxCommand *cmd);

private:
    // the control we're associated with
    wxTextCtrl *m_text;

    // if the flag is true we're compressing subsequent insert commands into
    // one so that the entire typing could be undone in one call to Undo()
    bool m_compressInserts;
};

// ============================================================================
// implementation
// ============================================================================

BEGIN_EVENT_TABLE(wxTextCtrl, wxTextCtrlBase)
    EVT_CHAR(wxTextCtrl::OnChar)

    EVT_SIZE(wxTextCtrl::OnSize)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(wxTextCtrl, wxTextCtrlBase)

// ----------------------------------------------------------------------------
// creation
// ----------------------------------------------------------------------------

void wxTextCtrl::Init()
{
    m_selAnchor =
    m_selStart =
    m_selEnd = -1;

    m_isModified = false;
    m_isEditable = true;
    m_wrapLines = false;

    m_posLast =
    m_curPos =
    m_curCol =
    m_curRow = 0;

    m_heightLine =
    m_widthAvg = -1;

    // init the undo manager
    m_cmdProcessor = new wxTextCtrlCommandProcessor(this);

    // no data yet
    m_data.data = NULL;
}

bool wxTextCtrl::Create(wxWindow *parent,
                        wxWindowID id,
                        const wxString& value,
                        const wxPoint& pos,
                        const wxSize& size,
                        long style,
                        const wxValidator& validator,
                        const wxString &name)
{
    if ( style & wxTE_MULTILINE )
    {
        // for compatibility with wxMSW we create the controls with vertical
        // scrollbar always shown unless they have wxTE_RICH style (because
        // Windows text controls always has vert scrollbar but richedit one
        // doesn't)
        if ( !(style & wxTE_RICH) )
        {
            style |= wxALWAYS_SHOW_SB;
        }

        // wrapping style: wxTE_DONTWRAP == wxHSCROLL so if it's _not_ given,
        // we won't have horizontal scrollbar automatically, no need to do
        // anything

        // TODO: support wxTE_NO_VSCROLL (?)

        // create data object for normal multiline or for controls with line
        // wrap as needed
        if ( style & wxHSCROLL )
        {
            m_data.mdata = new wxTextMultiLineData;
        }
        else // we must wrap lines if we don't have horizontal scrollbar
        {
            // NB: we can't rely on HasFlag(wxHSCROLL) as the flags can change
            //     later and even wxWindow::Create() itself temporarily resets
            //     wxHSCROLL in wxUniv, so remember that we have a wrapped data
            //     and not just a multi line data in a separate variable
            m_wrapLines = true;
            m_data.wdata = new wxTextWrappedData;
        }
    }
    else
    {
        // this doesn't make sense for single line controls
        style &= ~wxHSCROLL;

        // create data object for single line controls
        m_data.sdata = new wxTextSingleLineData;
    }

#if wxUSE_TWO_WINDOWS
    if ((style & wxBORDER_MASK) == 0)
        style |= wxBORDER_SUNKEN;
#endif

    if ( !wxControl::Create(parent, id, pos, size, style,
                            validator, name) )
    {
        return false;
    }

    SetCursor(wxCURSOR_IBEAM);

    if ( style & wxTE_MULTILINE )
    {
        // we should always have at least one line in a multiline control
        MData().m_lines.Add(wxEmptyString);

        if ( !(style & wxHSCROLL) )
        {
            WData().m_linesData.Add(new wxWrappedLineData);
            WData().InvalidateLinesBelow(0);
        }

        // we might support it but it's quite useless and other ports don't
        // support it anyhow
        wxASSERT_MSG( !(style & wxTE_PASSWORD),
                      _T("wxTE_PASSWORD can't be used with multiline ctrls") );
    }

    RecalcFontMetrics();
    SetValue(value);
    SetInitialSize(size);

    m_isEditable = !(style & wxTE_READONLY);

    CreateCaret();
    InitInsertionPoint();

    // we can't show caret right now as we're not shown yet and so it would
    // result in garbage on the screen - we'll do it after first OnPaint()
    m_hasCaret = false;

    CreateInputHandler(wxINP_HANDLER_TEXTCTRL);

    wxSizeEvent sizeEvent(GetSize(), GetId());
    GetEventHandler()->ProcessEvent(sizeEvent);

    return true;
}

wxTextCtrl::~wxTextCtrl()
{
    delete m_cmdProcessor;

    if ( m_data.data )
    {
        if ( IsSingleLine() )
            delete m_data.sdata;
        else if ( WrapLines() )
            delete m_data.wdata;
        else
            delete m_data.mdata;
    }
}

// ----------------------------------------------------------------------------
// set/get the value
// ----------------------------------------------------------------------------

void wxTextCtrl::DoSetValue(const wxString& value, int flags)
{
    if ( IsSingleLine() && (value == GetValue()) )
    {
        // nothing changed
        return;
    }

    Replace(0, GetLastPosition(), value);

    if ( IsSingleLine() )
    {
        SetInsertionPoint(0);
    }

    if ( flags & SetValue_SendEvent )
        SendTextUpdatedEvent();
}

const wxArrayString& wxTextCtrl::GetLines() const
{
    return MData().m_lines;
}

size_t wxTextCtrl::GetLineCount() const
{
    return MData().m_lines.GetCount();
}

wxString wxTextCtrl::GetValue() const
{
    // for multiline controls we don't always store the total value but only
    // recompute it when asked - and to invalidate it we just empty it in
    // Replace()
    if ( !IsSingleLine() && m_value.empty() )
    {
        // recalculate: note that we always do it for empty multilien control,
        // but then it's so quick that it's not important

        // the first line is special as there is no \n before it, so it's
        // outside the loop
        const wxArrayString& lines = GetLines();
        wxTextCtrl *self = wxConstCast(this, wxTextCtrl);
        self->m_value << lines[0u];
        size_t count = lines.GetCount();
        for ( size_t n = 1; n < count; n++ )
        {
            self->m_value << _T('\n') << lines[n];
        }
    }

    return m_value;
}

void wxTextCtrl::Clear()
{
    SetValue(wxEmptyString);
}

bool wxTextCtrl::ReplaceLine(wxTextCoord line,
                             const wxString& text)
{
    if ( WrapLines() )
    {
        // first, we have to relayout the line entirely
        //
        // OPT: we might try not to recalc the unchanged part of line

        wxWrappedLineData& lineData = WData().m_linesData[line];

        // if we had some number of rows before, use this number, otherwise
        // just make sure that the test below (rowsNew != rowsOld) will be true
        int rowsOld;
        if ( lineData.IsValid() )
        {
            rowsOld = lineData.GetExtraRowCount();
        }
        else // line wasn't laid out yet
        {
            // assume it changed entirely as we can't do anything better
            rowsOld = -1;
        }

        // now change the line
        MData().m_lines[line] = text;

        // OPT: we choose to lay it our immediately instead of delaying it
        //      until it is needed because it allows us to avoid invalidating
        //      lines further down if the number of rows didn't chnage, but
        //      maybe we can imporve this even further?
        LayoutLine(line, lineData);

        int rowsNew = lineData.GetExtraRowCount();

        if ( rowsNew != rowsOld )
        {
            // we have to update the line wrap marks as this is normally done
            // by LayoutLines() which we bypassed by calling LayoutLine()
            // directly
            wxTextCoord rowFirst = lineData.GetFirstRow(),
                        rowCount = wxMax(rowsOld, rowsNew);
            RefreshLineWrapMarks(rowFirst, rowFirst + rowCount);

            // next, if this is not the last line, as the number of rows in it
            // changed, we need to shift all the lines below it
            if ( (size_t)line < WData().m_linesData.GetCount() )
            {
                // number of rows changed shifting all lines below
                WData().InvalidateLinesBelow(line + 1);
            }

            // the number of rows changed
            return true;
        }
    }
    else // no line wrap
    {
        MData().m_lines[line] = text;
    }

    // the number of rows didn't change
    return false;
}

void wxTextCtrl::RemoveLine(wxTextCoord line)
{
    MData().m_lines.RemoveAt(line);
    if ( WrapLines() )
    {
        // we need to recalculate all the starting rows from this line, but we
        // can avoid doing it if this line was never calculated: this means
        // that we will recalculate all lines below it anyhow later if needed
        if ( WData().IsValidLine(line) )
        {
            WData().InvalidateLinesBelow(line);
        }

        WData().m_linesData.RemoveAt(line);
    }
}

void wxTextCtrl::InsertLine(wxTextCoord line, const wxString& text)
{
    MData().m_lines.Insert(text, line);
    if ( WrapLines() )
    {
        WData().m_linesData.Insert(new wxWrappedLineData, line);

        // invalidate everything below it
        WData().InvalidateLinesBelow(line);
    }
}

void wxTextCtrl::Replace(wxTextPos from, wxTextPos to, const wxString& text)
{
    wxTextCoord colStart, colEnd,
                lineStart, lineEnd;

    if ( (from > to) ||
         !PositionToXY(from, &colStart, &lineStart) ||
         !PositionToXY(to, &colEnd, &lineEnd) )
    {
        wxFAIL_MSG(_T("invalid range in wxTextCtrl::Replace"));

        return;
    }

#ifdef WXDEBUG_TEXT_REPLACE
    // a straighforward (but very inefficient) way of calculating what the new
    // value should be
    wxString textTotal = GetValue();
    wxString textTotalNew(textTotal, (size_t)from);
    textTotalNew += text;
    if ( (size_t)to < textTotal.length() )
        textTotalNew += textTotal.c_str() + (size_t)to;
#endif // WXDEBUG_TEXT_REPLACE

    // remember the old selection and reset it immediately: we must do it
    // before calling Refresh(anything) as, at least under GTK, this leads to
    // an _immediate_ repaint (under MSW it is delayed) and hence parts of
    // text would be redrawn as selected if we didn't reset the selection
    int selStartOld = m_selStart,
        selEndOld = m_selEnd;

    m_selStart =
    m_selEnd = -1;

    if ( IsSingleLine() )
    {
        // replace the part of the text with the new value
        wxString valueNew(m_value, (size_t)from);

        // remember it for later use
        wxCoord startNewText = GetTextWidth(valueNew);

        valueNew += text;
        if ( (size_t)to < m_value.length() )
        {
            valueNew += m_value.c_str() + (size_t)to;
        }

        // we usually refresh till the end of line except of the most common case
        // when some text is appended to the end of the string in which case we
        // refresh just it
        wxCoord widthNewText;

        if ( (size_t)from < m_value.length() )
        {
            // refresh till the end of line
            widthNewText = 0;
        }
        else // text appended, not replaced
        {
            // refresh only the new text
            widthNewText = GetTextWidth(text);
        }

        m_value = valueNew;

        // force SData().m_colLastVisible update
        SData().m_colLastVisible = -1;

        // repaint
        RefreshPixelRange(0, startNewText, widthNewText);
    }
    else // multiline
    {
        //OPT: special case for replacements inside single line?

        /*
           Join all the lines in the replacement range into one string, then
           replace a part of it with the new text and break it into lines again.
        */

        // (0) we want to know if this replacement changes the number of rows
        //     as if it does we need to refresh everything below the changed
        //     text (it will be shifted...) and we can avoid it if there is no
        //     row relayout
        bool rowsNumberChanged = false;

        // (1) join lines
        const wxArrayString& linesOld = GetLines();
        wxString textOrig;
        wxTextCoord line;
        for ( line = lineStart; line <= lineEnd; line++ )
        {
            if ( line > lineStart )
            {
                // from the previous line
                textOrig += _T('\n');
            }

            textOrig += linesOld[line];
        }

        // we need to append the '\n' for the last line unless there is no
        // following line
        size_t countOld = linesOld.GetCount();

        // (2) replace text in the combined string

        // (2a) leave the part before replaced area unchanged
        wxString textNew(textOrig, colStart);

        // these values will be used to refresh the changed area below
        wxCoord widthNewText,
                startNewText = GetTextWidth(textNew);
        if ( (size_t)colStart == linesOld[lineStart].length() )
        {
            // text appended, refresh just enough to show the new text
            widthNewText = GetTextWidth(text.BeforeFirst(_T('\n')));
        }
        else // text inserted, refresh till the end of line
        {
            widthNewText = 0;
        }

        // (2b) insert new text
        textNew += text;

        // (2c) and append the end of the old text

        // adjust for index shift: to is relative to colStart, not 0
        size_t toRel = (size_t)((to - from) + colStart);
        if ( toRel < textOrig.length() )
        {
            textNew += textOrig.c_str() + toRel;
        }

        // (3) break it into lines

        wxArrayString lines;
        const wxChar *curLineStart = textNew.c_str();
        for ( const wxChar *p = textNew.c_str(); ; p++ )
        {
            // end of line/text?
            if ( !*p || *p == _T('\n') )
            {
                lines.Add(wxString(curLineStart, p));
                if ( !*p )
                    break;

                curLineStart = p + 1;
            }
        }

#ifdef WXDEBUG_TEXT_REPLACE
        // (3a) all empty tokens should be counted as replacing with "foo" and
        //      with "foo\n" should have different effects
        wxArrayString lines2 = wxStringTokenize(textNew, _T("\n"),
                                                wxTOKEN_RET_EMPTY_ALL);

        if ( lines2.IsEmpty() )
        {
            lines2.Add(wxEmptyString);
        }

        wxASSERT_MSG( lines.GetCount() == lines2.GetCount(),
                      _T("Replace() broken") );
        for ( size_t n = 0; n < lines.GetCount(); n++ )
        {
            wxASSERT_MSG( lines[n] == lines2[n], _T("Replace() broken") );
        }
#endif // WXDEBUG_TEXT_REPLACE

        // (3b) special case: if we replace everything till the end we need to
        //      keep an empty line or the lines would disappear completely
        //      (this also takes care of never leaving m_lines empty)
        if ( ((size_t)lineEnd == countOld - 1) && lines.IsEmpty() )
        {
            lines.Add(wxEmptyString);
        }

        size_t nReplaceCount = lines.GetCount(),
               nReplaceLine = 0;

        // (4) merge into the array

        // (4a) replace
        for ( line = lineStart; line <= lineEnd; line++, nReplaceLine++ )
        {
            if ( nReplaceLine < nReplaceCount )
            {
                // we have the replacement line for this one
                if ( ReplaceLine(line, lines[nReplaceLine]) )
                {
                    rowsNumberChanged = true;
                }

                UpdateMaxWidth(line);
            }
            else // no more replacement lines
            {
                // (4b) delete all extra lines (note that we need to delete
                //      them backwards because indices shift while we do it)
                bool deletedLongestLine = false;
                for ( wxTextCoord lineDel = lineEnd; lineDel >= line; lineDel-- )
                {
                    if ( lineDel == MData().m_lineLongest )
                    {
                        // we will need to recalc the max line width
                        deletedLongestLine = true;
                    }

                    RemoveLine(lineDel);
                }

                if ( deletedLongestLine )
                {
                    RecalcMaxWidth();
                }

                // even the line number changed
                rowsNumberChanged = true;

                // update line to exit the loop
                line = lineEnd + 1;
            }
        }

        // (4c) insert the new lines
        if ( nReplaceLine < nReplaceCount )
        {
            // even the line number changed
            rowsNumberChanged = true;

            do
            {
                InsertLine(++lineEnd, lines[nReplaceLine++]);

                UpdateMaxWidth(lineEnd);
            }
            while ( nReplaceLine < nReplaceCount );
        }

        // (5) now refresh the changed area

        // update the (cached) last position first as refresh functions use it
        m_posLast += text.length() - to + from;

        // we may optimize refresh if the number of rows didn't change - but if
        // it did we have to refresh everything below the part we chanegd as
        // well as it might have moved
        if ( !rowsNumberChanged )
        {
            // refresh the line we changed
            if ( !WrapLines() )
            {
                RefreshPixelRange(lineStart++, startNewText, widthNewText);
            }
            else
            {
                //OPT: we shouldn't refresh the unchanged part of the line in
                //     this case, but instead just refresh the tail of it - the
                //     trouble is that we don't know here where does this tail
                //     start
            }

            // number of rows didn't change, refresh the updated rows and the
            // last one
            if ( lineStart <= lineEnd )
                RefreshLineRange(lineStart, lineEnd);
        }
        else // rows number did change
        {
            if ( !WrapLines() )
            {
                // refresh only part of the first line
                RefreshPixelRange(lineStart++, startNewText, widthNewText);
            }
            //else: we have to refresh everything as some part of the text
            //      could be in the previous row before but moved to the next
            //      one now (due to word wrap)

            wxTextCoord lineEnd = GetLines().GetCount() - 1;
            if ( lineStart <= lineEnd )
                RefreshLineRange(lineStart, lineEnd);

            // refresh text rect left below
            RefreshLineRange(lineEnd + 1, 0);

            // the vert scrollbar might [dis]appear
            MData().m_updateScrollbarY = true;
        }

        // must recalculate it - will do later
        m_value.clear();
    }

#ifdef WXDEBUG_TEXT_REPLACE
    // optimized code above should give the same result as straightforward
    // computation in the beginning
    wxASSERT_MSG( GetValue() == textTotalNew, _T("error in Replace()") );
#endif // WXDEBUG_TEXT_REPLACE

    // update the current position: note that we always put the cursor at the
    // end of the replacement text
    DoSetInsertionPoint(from + text.length());

    // and the selection: this is complicated by the fact that selection coords
    // must be first updated to reflect change in text coords, i.e. if we had
    // selection from 17 to 19 and we just removed this range, we don't have to
    // refresh anything, so we can't just use ClearSelection() here
    if ( selStartOld != -1 )
    {
        // refresh the parst of the selection outside the changed text (which
        // we already refreshed)
        if ( selStartOld < from )
            RefreshTextRange(selStartOld, from);
        if ( to < selEndOld )
            RefreshTextRange(to, selEndOld);

    }

    // now call it to do the rest (not related to refreshing)
    ClearSelection();
}

void wxTextCtrl::Remove(wxTextPos from, wxTextPos to)
{
    // Replace() only works with correctly ordered arguments, so exchange them
    // if necessary
    OrderPositions(from, to);

    Replace(from, to, wxEmptyString);
}

void wxTextCtrl::WriteText(const wxString& text)
{
    // replace the selection with the new text
    RemoveSelection();

    Replace(m_curPos, m_curPos, text);
}

void wxTextCtrl::AppendText(const wxString& text)
{
    SetInsertionPointEnd();
    WriteText(text);
}

// ----------------------------------------------------------------------------
// current position
// ----------------------------------------------------------------------------

void wxTextCtrl::SetInsertionPoint(wxTextPos pos)
{
    wxCHECK_RET( pos >= 0 && pos <= GetLastPosition(),
                 _T("insertion point position out of range") );

    // don't do anything if it didn't change
    if ( pos != m_curPos )
    {
        DoSetInsertionPoint(pos);
    }

    if ( !IsSingleLine() )
    {
        // moving cursor should reset the stored abscissa (even if the cursor
        // position didn't actually change!)
        MData().m_xCaret = -1;
    }

    ClearSelection();
}

void wxTextCtrl::InitInsertionPoint()
{
    // so far always put it in the beginning
    DoSetInsertionPoint(0);

    // this will also set the selection anchor correctly
    ClearSelection();
}

void wxTextCtrl::MoveInsertionPoint(wxTextPos pos)
{
    wxASSERT_MSG( pos >= 0 && pos <= GetLastPosition(),
                 _T("DoSetInsertionPoint() can only be called with valid pos") );

    m_curPos = pos;
    PositionToXY(m_curPos, &m_curCol, &m_curRow);
}

void wxTextCtrl::DoSetInsertionPoint(wxTextPos pos)
{
    MoveInsertionPoint(pos);

    ShowPosition(pos);
}

void wxTextCtrl::SetInsertionPointEnd()
{
    SetInsertionPoint(GetLastPosition());
}

wxTextPos wxTextCtrl::GetInsertionPoint() const
{
    return m_curPos;
}

wxTextPos wxTextCtrl::GetLastPosition() const
{
    wxTextPos pos;
    if ( IsSingleLine() )
    {
        pos = m_value.length();
    }
    else // multiline
    {
#ifdef WXDEBUG_TEXT
        pos = 0;
        size_t nLineCount = GetLineCount();
        for ( size_t nLine = 0; nLine < nLineCount; nLine++ )
        {
            // +1 is because the positions at the end of this line and of the
            // start of the next one are different
            pos += GetLines()[nLine].length() + 1;
        }

        if ( pos > 0 )
        {
            // the last position is at the end of the last line, not in the
            // beginning of the next line after it
            pos--;
        }

        // more probable reason of this would be to forget to update m_posLast
        wxASSERT_MSG( pos == m_posLast, _T("bug in GetLastPosition()") );
#endif // WXDEBUG_TEXT

        pos = m_posLast;
    }

    return pos;
}

// ----------------------------------------------------------------------------
// selection
// ----------------------------------------------------------------------------

void wxTextCtrl::GetSelection(wxTextPos* from, wxTextPos* to) const
{
    if ( from )
        *from = m_selStart;
    if ( to )
        *to = m_selEnd;
}

wxString wxTextCtrl::GetSelectionText() const
{
    wxString sel;

    if ( HasSelection() )
    {
        if ( IsSingleLine() )
        {
            sel = m_value.Mid(m_selStart, m_selEnd - m_selStart);
        }
        else // multiline
        {
            wxTextCoord colStart, lineStart,
                        colEnd, lineEnd;
            PositionToXY(m_selStart, &colStart, &lineStart);
            PositionToXY(m_selEnd, &colEnd, &lineEnd);

            // as always, we need to check for the special case when the start
            // and end line are the same
            if ( lineEnd == lineStart )
            {
                sel = GetLines()[lineStart].Mid(colStart, colEnd - colStart);
            }
            else // sel on multiple lines
            {
                // take the end of the first line
                sel = GetLines()[lineStart].c_str() + colStart;
                sel += _T('\n');

                // all intermediate ones
                for ( wxTextCoord line = lineStart + 1; line < lineEnd; line++ )
                {
                    sel << GetLines()[line] << _T('\n');
                }

                // and the start of the last one
                sel += GetLines()[lineEnd].Left(colEnd);
            }
        }
    }

    return sel;
}

void wxTextCtrl::SetSelection(wxTextPos from, wxTextPos to)
{
    // selecting till -1 is the same as selecting to the end
    if ( to == -1 )
    {
        // and selecting (-1, -1) range is the same as selecting everything, by
        // convention
        if ( from == -1 )
            from = 0;
        to = GetLastPosition();
    }

    if ( from == -1 || to == from )
    {
        ClearSelection();
    }
    else // valid sel range
    {
        // remember the 'to' position as the current position, used to move the
        // caret there later
        wxTextPos toOrig = to;

        OrderPositions(from, to);

        wxCHECK_RET( to <= GetLastPosition(),
                     _T("invalid range in wxTextCtrl::SetSelection") );

        if ( from != m_selStart || to != m_selEnd )
        {
            // we need to use temp vars as RefreshTextRange() may call DoDraw()
            // directly and so m_selStart/End must be reset by then
            wxTextPos selStartOld = m_selStart,
                      selEndOld = m_selEnd;

            m_selStart = from;
            m_selEnd = to;

            wxLogTrace(_T("text"), _T("Selection range is %ld-%ld"),
                       m_selStart, m_selEnd);

            // refresh only the part of text which became (un)selected if
            // possible
            if ( selStartOld == m_selStart )
            {
                RefreshTextRange(selEndOld, m_selEnd);
            }
            else if ( selEndOld == m_selEnd )
            {
                RefreshTextRange(m_selStart, selStartOld);
            }
            else
            {
                // OPT: could check for other cases too but it is probably not
                //      worth it as the two above are the most common ones
                if ( selStartOld != -1 )
                    RefreshTextRange(selStartOld, selEndOld);
                if ( m_selStart != -1 )
                    RefreshTextRange(m_selStart, m_selEnd);
            }

            // we need to fully repaint the invalidated areas of the window
            // before scrolling it (from DoSetInsertionPoint which is typically
            // called after SetSelection()), otherwise they may stay unpainted
            m_targetWindow->Update();
        }
        //else: nothing to do

        // the insertion point is put at the location where the caret was moved
        DoSetInsertionPoint(toOrig);
    }
}

void wxTextCtrl::ClearSelection()
{
    if ( HasSelection() )
    {
        // we need to use temp vars as RefreshTextRange() may call DoDraw()
        // directly (see above as well)
        wxTextPos selStart = m_selStart,
                  selEnd = m_selEnd;

        // no selection any more
        m_selStart =
        m_selEnd = -1;

        // refresh the old selection
        RefreshTextRange(selStart, selEnd);
    }

    // the anchor should be moved even if there was no selection previously
    m_selAnchor = m_curPos;
}

void wxTextCtrl::RemoveSelection()
{
    if ( !HasSelection() )
        return;

    Remove(m_selStart, m_selEnd);
}

bool wxTextCtrl::GetSelectedPartOfLine(wxTextCoord line,
                                       wxTextPos *start, wxTextPos *end) const
{
    if ( start )
        *start = -1;
    if ( end )
        *end = -1;

    if ( !HasSelection() )
    {
        // no selection at all, hence no selection in this line
        return false;
    }

    wxTextCoord lineStart, colStart;
    PositionToXY(m_selStart, &colStart, &lineStart);
    if ( lineStart > line )
    {
        // this line is entirely above the selection
        return false;
    }

    wxTextCoord lineEnd, colEnd;
    PositionToXY(m_selEnd, &colEnd, &lineEnd);
    if ( lineEnd < line )
    {
        // this line is entirely below the selection
        return false;
    }

    if ( line == lineStart )
    {
        if ( start )
            *start = colStart;
        if ( end )
            *end = lineEnd == lineStart ? colEnd : GetLineLength(line);
    }
    else if ( line == lineEnd )
    {
        if ( start )
            *start = lineEnd == lineStart ? colStart : 0;
        if ( end )
            *end = colEnd;
    }
    else // the line is entirely inside the selection
    {
        if ( start )
            *start = 0;
        if ( end )
            *end = GetLineLength(line);
    }

    return true;
}

// ----------------------------------------------------------------------------
// flags
// ----------------------------------------------------------------------------

bool wxTextCtrl::IsModified() const
{
    return m_isModified;
}

bool wxTextCtrl::IsEditable() const
{
    // disabled control can never be edited
    return m_isEditable && IsEnabled();
}

void wxTextCtrl::MarkDirty()
{
    m_isModified = true;
}

void wxTextCtrl::DiscardEdits()
{
    m_isModified = false;
}

void wxTextCtrl::SetEditable(bool editable)
{
    if ( editable != m_isEditable )
    {
        m_isEditable = editable;

        // the caret (dis)appears
        CreateCaret();

        // the appearance of the control might have changed
        Refresh();
    }
}

// ----------------------------------------------------------------------------
// col/lines <-> position correspondence
// ----------------------------------------------------------------------------

/*
    A few remarks about this stuff:

    o   The numbering of the text control columns/rows starts from 0.
    o   Start of first line is position 0, its last position is line.length()
    o   Start of the next line is the last position of the previous line + 1
 */

int wxTextCtrl::GetLineLength(wxTextCoord line) const
{
    if ( IsSingleLine() )
    {
        wxASSERT_MSG( line == 0, _T("invalid GetLineLength() parameter") );

        return m_value.length();
    }
    else // multiline
    {
        wxCHECK_MSG( (size_t)line < GetLineCount(), -1,
                     _T("line index out of range") );

        return GetLines()[line].length();
    }
}

wxString wxTextCtrl::GetLineText(wxTextCoord line) const
{
    if ( IsSingleLine() )
    {
        wxASSERT_MSG( line == 0, _T("invalid GetLineLength() parameter") );

        return m_value;
    }
    else // multiline
    {
        //this is called during DoGetBestSize
        if (line == 0 && GetLineCount() == 0) return wxEmptyString ;

        wxCHECK_MSG( (size_t)line < GetLineCount(), wxEmptyString,
                     _T("line index out of range") );

        return GetLines()[line];
    }
}

int wxTextCtrl::GetNumberOfLines() const
{
    // there is always 1 line, even if the text is empty
    return IsSingleLine() ? 1 : GetLineCount();
}

wxTextPos wxTextCtrl::XYToPosition(wxTextCoord x, wxTextCoord y) const
{
    // note that this method should accept any values of x and y and return -1
    // if they are out of range
    if ( IsSingleLine() )
    {
        return ( x > GetLastPosition() || y > 0 ) ? wxOutOfRangeTextCoord : x;
    }
    else // multiline
    {
        if ( (size_t)y >= GetLineCount() )
        {
            // this position is below the text
            return GetLastPosition();
        }

        wxTextPos pos = 0;
        for ( size_t nLine = 0; nLine < (size_t)y; nLine++ )
        {
            // +1 is because the positions at the end of this line and of the
            // start of the next one are different
            pos += GetLines()[nLine].length() + 1;
        }

        // take into account also the position in line
        if ( (size_t)x > GetLines()[y].length() )
        {
            // don't return position in the next line
            x = GetLines()[y].length();
        }

        return pos + x;
    }
}

bool wxTextCtrl::PositionToXY(wxTextPos pos,
                              wxTextCoord *x, wxTextCoord *y) const
{
    if ( IsSingleLine() )
    {
        if ( (size_t)pos > m_value.length() )
            return false;

        if ( x )
            *x = pos;
        if ( y )
            *y = 0;

        return true;
    }
    else // multiline
    {
        wxTextPos posCur = 0;
        size_t nLineCount = GetLineCount();
        for ( size_t nLine = 0; nLine < nLineCount; nLine++ )
        {
            // +1 is because the start the start of the next line is one
            // position after the end of this one
            wxTextPos posNew = posCur + GetLines()[nLine].length() + 1;
            if ( posNew > pos )
            {
                // we've found the line, now just calc the column
                if ( x )
                    *x = pos - posCur;

                if ( y )
                    *y = nLine;

#ifdef WXDEBUG_TEXT
                wxASSERT_MSG( XYToPosition(pos - posCur, nLine) == pos,
                              _T("XYToPosition() or PositionToXY() broken") );
#endif // WXDEBUG_TEXT

                return true;
            }
            else // go further down
            {
                posCur = posNew;
            }
        }

        // beyond the last line
        return false;
    }
}

wxTextCoord wxTextCtrl::GetRowsPerLine(wxTextCoord line) const
{
    // a normal line has one row
    wxTextCoord numRows = 1;

    if ( WrapLines() )
    {
        // add the number of additional rows
        numRows += WData().m_linesData[line].GetExtraRowCount();
    }

    return numRows;
}

wxTextCoord wxTextCtrl::GetRowCount() const
{
    wxTextCoord count = GetLineCount();
    if (count == 0)
        return 0;
    if ( WrapLines() )
    {
        count = GetFirstRowOfLine(count - 1) +
                    WData().m_linesData[count - 1].GetRowCount();
    }

    return count;
}

wxTextCoord wxTextCtrl::GetRowAfterLine(wxTextCoord line) const
{
    if ( !WrapLines() )
        return line + 1;

    if ( !WData().IsValidLine(line) )
    {
        LayoutLines(line);
    }

    return WData().m_linesData[line].GetNextRow();
}

wxTextCoord wxTextCtrl::GetFirstRowOfLine(wxTextCoord line) const
{
    if ( !WrapLines() )
        return line;

    if ( !WData().IsValidLine(line) )
    {
        LayoutLines(line);
    }

    return WData().m_linesData[line].GetFirstRow();
}

bool wxTextCtrl::PositionToLogicalXY(wxTextPos pos,
                                     wxCoord *xOut,
                                     wxCoord *yOut) const
{
    wxTextCoord col, line;

    // optimization for special (but common) case when we already have the col
    // and line
    if ( pos == m_curPos )
    {
        col = m_curCol;
        line = m_curRow;
    }
    else // must really calculate col/line from pos
    {
        if ( !PositionToXY(pos, &col, &line) )
            return false;
    }

    int hLine = GetLineHeight();
    wxCoord x, y;
    wxString textLine = GetLineText(line);
    if ( IsSingleLine() || !WrapLines() )
    {
        x = GetTextWidth(textLine.Left(col));
        y = line*hLine;
    }
    else // difficult case: multline control with line wrap
    {
        y = GetFirstRowOfLine(line);

        wxTextCoord colRowStart;
        y += GetRowInLine(line, col, &colRowStart);

        y *= hLine;

        // x is the width of the text before this position in this row
        x = GetTextWidth(textLine.Mid(colRowStart, col - colRowStart));
    }

    if ( xOut )
        *xOut = x;
    if ( yOut )
        *yOut = y;

    return true;
}

bool wxTextCtrl::PositionToDeviceXY(wxTextPos pos,
                                    wxCoord *xOut,
                                    wxCoord *yOut) const
{
    wxCoord x, y;
    if ( !PositionToLogicalXY(pos, &x, &y) )
        return false;

    // finally translate the logical text rect coords into physical client
    // coords
    CalcScrolledPosition(m_rectText.x + x, m_rectText.y + y, xOut, yOut);

    return true;
}

wxPoint wxTextCtrl::GetCaretPosition() const
{
    wxCoord xCaret, yCaret;
    if ( !PositionToDeviceXY(m_curPos, &xCaret, &yCaret) )
    {
        wxFAIL_MSG( _T("Caret can't be beyond the text!") );
    }

    return wxPoint(xCaret, yCaret);
}

// pos may be -1 to show the current position
void wxTextCtrl::ShowPosition(wxTextPos pos)
{
    bool showCaret = GetCaret() && GetCaret()->IsVisible();
    if (showCaret)
        HideCaret();

    if ( IsSingleLine() )
    {
        ShowHorzPosition(GetTextWidth(m_value.Left(pos)));
    }
    else if ( MData().m_scrollRangeX || MData().m_scrollRangeY ) // multiline with scrollbars
    {
        int xStart, yStart;
        GetViewStart(&xStart, &yStart);

        if ( pos == -1 )
            pos = m_curPos;

        wxCoord x, y;
        PositionToLogicalXY(pos, &x, &y);

        wxRect rectText = GetRealTextArea();

        // scroll the position vertically into view: if it is currently above
        // it, make it the first one, otherwise the last one
        if ( MData().m_scrollRangeY )
        {
            y /= GetLineHeight();

            if ( y < yStart )
            {
                Scroll(0, y);
            }
            else // we are currently in or below the view area
            {
                // find the last row currently shown
                wxTextCoord yEnd;

                if ( WrapLines() )
                {
                    // to find the last row we need to use the generic HitTest
                    wxTextCoord col;

                    // OPT this is a bit silly: we undo this in HitTest(), so
                    //     it would be better to factor out the common
                    //     functionality into a separate function (OTOH it
                    //     won't probably save us that much)
                    wxPoint pt(0, rectText.height - 1);
                    pt += GetClientAreaOrigin();
                    pt += m_rectText.GetPosition();
                    HitTest(pt, &col, &yEnd);

                    // find the row inside the line
                    yEnd = GetFirstRowOfLine(yEnd) + GetRowInLine(yEnd, col);
                }
                else
                {
                    // finding the last line is easy if each line has exactly
                    // one row
                    yEnd = yStart + rectText.height / GetLineHeight() - 1;
                }

                if ( yEnd < y )
                {
                    // scroll down: the current item should appear at the
                    // bottom of the view
                    Scroll(0, y - (yEnd - yStart));
                }
            }
        }

        // scroll the position horizontally into view
        //
        // we follow what I believe to be Windows behaviour here, that is if
        // the position is already entirely in the view we do nothing, but if
        // we do have to scroll the window to bring it into view, we scroll it
        // not just enough to show the position but slightly more so that this
        // position is at 1/3 of the window width from the closest border to it
        // (I'm not sure that Windows does exactly this but it looks like this)
        if ( MData().m_scrollRangeX )
        {
            // unlike for the rows, xStart doesn't correspond to the starting
            // column as they all have different widths, so we need to
            // translate everything to pixels

            // we want the text between x and x2 be entirely inside the view
            // (i.e. the current character)

            // make xStart the first visible pixel (and not position)
            int wChar = GetAverageWidth();
            xStart *= wChar;

            if ( x < xStart )
            {
                // we want the position of this column be 1/3 to the right of
                // the left edge
                x -= rectText.width / 3;
                if ( x < 0 )
                    x = 0;
                Scroll(x / wChar, y);
            }
            else // maybe we're beyond the right border of the view?
            {
                wxTextCoord col, row;
                if ( PositionToXY(pos, &col, &row) )
                {
                    wxString lineText = GetLineText(row);
                    wxCoord x2 = x + GetTextWidth(lineText[(size_t)col]);
                    if ( x2 > xStart + rectText.width )
                    {
                        // we want the position of this column be 1/3 to the
                        // left of the right edge, i.e. 2/3 right of the left
                        // one
                        x2 -= (2*rectText.width)/3;
                        if ( x2 < 0 )
                            x2 = 0;
                        Scroll(x2 / wChar, row);
                    }
                }
            }
        }
    }
    //else: multiline but no scrollbars, hence nothing to do

    if (showCaret)
        ShowCaret();
}

// ----------------------------------------------------------------------------
// word stuff
// ----------------------------------------------------------------------------

/*
    TODO: we could have (easy to do) vi-like options for word movement, i.e.
          distinguish between inlusive/exclusive words and between words and
          WORDS (in vim sense) and also, finally, make the set of characters
          which make up a word configurable - currently we use the exclusive
          WORDS only (coincidentally, this is what Windows edit control does)

          For future references, here is what vim help says:

          A word consists of a sequence of letters, digits and underscores, or
          a sequence of other non-blank characters, separated with white space
          (spaces, tabs, <EOL>).  This can be changed with the 'iskeyword'
          option.

          A WORD consists of a sequence of non-blank characters, separated with
          white space.  An empty line is also considered to be a word and a
          WORD.
 */

static inline bool IsWordChar(wxChar ch)
{
    return !wxIsspace(ch);
}

wxTextPos wxTextCtrl::GetWordStart() const
{
    if ( m_curPos == -1 || m_curPos == 0 )
        return 0;

    if ( m_curCol == 0 )
    {
        // go to the end of the previous line
        return m_curPos - 1;
    }

    // it shouldn't be possible to learn where the word starts in the password
    // text entry zone
    if ( IsPassword() )
        return 0;

    // start at the previous position
    const wxChar *p0 = GetLineText(m_curRow).c_str();
    const wxChar *p = p0 + m_curCol - 1;

    // find the end of the previous word
    while ( (p > p0) && !IsWordChar(*p) )
        p--;

    // now find the beginning of this word
    while ( (p > p0) && IsWordChar(*p) )
        p--;

    // we might have gone too far
    if ( !IsWordChar(*p) )
        p++;

    return (m_curPos - m_curCol) + p - p0;
}

wxTextPos wxTextCtrl::GetWordEnd() const
{
    if ( m_curPos == -1 )
        return 0;

    wxString line = GetLineText(m_curRow);
    if ( (size_t)m_curCol == line.length() )
    {
        // if we're on the last position in the line, go to the next one - if
        // it exists
        wxTextPos pos = m_curPos;
        if ( pos < GetLastPosition() )
            pos++;

        return pos;
    }

    // it shouldn't be possible to learn where the word ends in the password
    // text entry zone
    if ( IsPassword() )
        return GetLastPosition();

    // start at the current position
    const wxChar *p0 = line.c_str();
    const wxChar *p = p0 + m_curCol;

    // find the start of the next word
    while ( *p && !IsWordChar(*p) )
        p++;

    // now find the end of it
    while ( *p && IsWordChar(*p) )
        p++;

    // and find the start of the next word
    while ( *p && !IsWordChar(*p) )
        p++;

    return (m_curPos - m_curCol) + p - p0;
}

// ----------------------------------------------------------------------------
// clipboard stuff
// ----------------------------------------------------------------------------

void wxTextCtrl::Copy()
{
#if wxUSE_CLIPBOARD
    if ( HasSelection() )
    {
        wxClipboardLocker clipLock;

        // wxTextFile::Translate() is needed to transform all '\n' into "\r\n"
        wxString text = wxTextFile::Translate(GetTextToShow(GetSelectionText()));
        wxTextDataObject *data = new wxTextDataObject(text);
        wxTheClipboard->SetData(data);
    }
#endif // wxUSE_CLIPBOARD
}

void wxTextCtrl::Cut()
{
    (void)DoCut();
}

bool wxTextCtrl::DoCut()
{
    if ( !HasSelection() )
        return false;

    Copy();

    RemoveSelection();

    return true;
}

void wxTextCtrl::Paste()
{
    (void)DoPaste();
}

bool wxTextCtrl::DoPaste()
{
#if wxUSE_CLIPBOARD
    wxClipboardLocker clipLock;

    wxTextDataObject data;
    if ( wxTheClipboard->IsSupported(data.GetFormat())
            && wxTheClipboard->GetData(data) )
    {
        // reverse transformation: '\r\n\" -> '\n'
        wxString text = wxTextFile::Translate(data.GetText(),
                                              wxTextFileType_Unix);
        if ( !text.empty() )
        {
            WriteText(text);

            return true;
        }
    }
#endif // wxUSE_CLIPBOARD

    return false;
}

// ----------------------------------------------------------------------------
// Undo and redo
// ----------------------------------------------------------------------------

wxTextCtrlInsertCommand *
wxTextCtrlCommandProcessor::IsInsertCommand(wxCommand *command)
{
    return (wxTextCtrlInsertCommand *)
            (command && (command->GetName() == wxTEXT_COMMAND_INSERT)
                ? command : NULL);
}

void wxTextCtrlCommandProcessor::Store(wxCommand *command)
{
    wxTextCtrlInsertCommand *cmdIns = IsInsertCommand(command);
    if ( cmdIns )
    {
        if ( IsCompressing() )
        {
            wxTextCtrlInsertCommand *
                cmdInsLast = IsInsertCommand(GetCurrentCommand());

            // it is possible that we don't have any last command at all if,
            // for example, it was undone since the last Store(), so deal with
            // this case too
            if ( cmdInsLast )
            {
                cmdInsLast->Append(cmdIns);

                delete cmdIns;

                // don't need to call the base class version
                return;
            }
        }

        // append the following insert commands to this one
        m_compressInserts = true;

        // let the base class version will do the job normally
    }
    else // not an insert command
    {
        // stop compressing insert commands - this won't work with the last
        // command not being an insert one anyhow
        StopCompressing();

        // let the base class version will do the job normally
    }

    wxCommandProcessor::Store(command);
}

void wxTextCtrlInsertCommand::Append(wxTextCtrlInsertCommand *other)
{
    m_text += other->m_text;
}

bool wxTextCtrlInsertCommand::CanUndo() const
{
    return m_from != -1;
}

bool wxTextCtrlInsertCommand::Do(wxTextCtrl *text)
{
    // the text is going to be inserted at the current position, remember where
    // exactly it is
    m_from = text->GetInsertionPoint();

    // and now do insert it
    text->WriteText(m_text);

    return true;
}

bool wxTextCtrlInsertCommand::Undo(wxTextCtrl *text)
{
    wxCHECK_MSG( CanUndo(), false, _T("impossible to undo insert cmd") );

    // remove the text from where we inserted it
    text->Remove(m_from, m_from + m_text.length());

    return true;
}

bool wxTextCtrlRemoveCommand::CanUndo() const
{
    // if we were executed, we should have the text we removed
    return !m_textDeleted.empty();
}

bool wxTextCtrlRemoveCommand::Do(wxTextCtrl *text)
{
    text->SetSelection(m_from, m_to);
    m_textDeleted = text->GetSelectionText();
    text->RemoveSelection();

    return true;
}

bool wxTextCtrlRemoveCommand::Undo(wxTextCtrl *text)
{
    // it is possible that the text was deleted and that we can't restore text
    // at the same position we removed it any more
    wxTextPos posLast = text->GetLastPosition();
    text->SetInsertionPoint(m_from > posLast ? posLast : m_from);
    text->WriteText(m_textDeleted);

    return true;
}

void wxTextCtrl::Undo()
{
    // the caller must check it
    wxASSERT_MSG( CanUndo(), _T("can't call Undo() if !CanUndo()") );

    m_cmdProcessor->Undo();
}

void wxTextCtrl::Redo()
{
    // the caller must check it
    wxASSERT_MSG( CanRedo(), _T("can't call Undo() if !CanUndo()") );

    m_cmdProcessor->Redo();
}

bool wxTextCtrl::CanUndo() const
{
    return IsEditable() && m_cmdProcessor->CanUndo();
}

bool wxTextCtrl::CanRedo() const
{
    return IsEditable() && m_cmdProcessor->CanRedo();
}

// ----------------------------------------------------------------------------
// geometry
// ----------------------------------------------------------------------------

wxSize wxTextCtrl::DoGetBestClientSize() const
{
    // when we're called for the very first time from Create() we must
    // calculate the font metrics here because we can't do it before calling
    // Create() (there is no window yet and wxGTK crashes) but we need them
    // here
    if ( m_heightLine == -1 )
    {
        wxConstCast(this, wxTextCtrl)->RecalcFontMetrics();
    }

    wxCoord w, h;
    GetTextExtent(GetTextToShow(GetLineText(0)), &w, &h);

    int wChar = GetAverageWidth(),
        hChar = GetLineHeight();

    int widthMin = wxMax(10*wChar, 100);
    if ( w < widthMin )
        w = widthMin;
    if ( h < hChar )
        h = hChar;

    if ( !IsSingleLine() )
    {
        // let the control have a reasonable number of lines
        int lines = GetNumberOfLines();
        if ( lines < 5 )
            lines = 5;
        else if ( lines > 10 )
            lines = 10;
        h *= lines;
    }

    wxRect rectText;
    rectText.width = w;
    rectText.height = h;
    wxRect rectTotal = GetRenderer()->GetTextTotalArea(this, rectText);
    return wxSize(rectTotal.width, rectTotal.height);
}

void wxTextCtrl::UpdateTextRect()
{
    wxRect rectTotal(GetClientSize());
    wxCoord *extraSpace = WrapLines() ? &WData().m_widthMark : NULL;
    m_rectText = GetRenderer()->GetTextClientArea(this, rectTotal, extraSpace);

    // code elsewhere is confused by negative rect size
    if ( m_rectText.width <= 0 )
        m_rectText.width = 1;
    if ( m_rectText.height <= 0 )
        m_rectText.height = 1;

    if ( !IsSingleLine() )
    {
        // invalidate it so that GetRealTextArea() will recalc it
        MData().m_rectTextReal.width = 0;

        // only scroll this rect when the window is scrolled: note that we have
        // to scroll not only the text but the line wrap marks too if we show
        // them
        wxRect rectText = GetRealTextArea();
        if ( extraSpace && *extraSpace )
        {
            rectText.width += *extraSpace;
        }
        SetTargetRect(rectText);

        // relayout all lines
        if ( WrapLines() )
        {
            WData().m_rowFirstInvalid = 0;

            // increase timestamp: this means that the lines which had been
            // laid out before will be relayd out the next time LayoutLines()
            // is called because their timestamp will be smaller than the
            // current one
            WData().m_timestamp++;
        }
    }

    UpdateLastVisible();
}

void wxTextCtrl::UpdateLastVisible()
{
    // this method is only used for horizontal "scrollbarless" scrolling which
    // is used only with single line controls
    if ( !IsSingleLine() )
        return;

    // use (efficient) HitTestLine to find the last visible character
    wxString text = m_value.Mid((size_t)SData().m_colStart /* to the end */);
    wxTextCoord col;
    switch ( HitTestLine(text, m_rectText.width, &col) )
    {
        case wxTE_HT_BEYOND:
            // everything is visible
            SData().m_ofsHorz = 0;

            SData().m_colStart = 0;
            SData().m_colLastVisible = text.length();

            // calculate it below
            SData().m_posLastVisible = -1;
            break;

           /*
        case wxTE_HT_BEFORE:
        case wxTE_HT_BELOW:
            */
        default:
            wxFAIL_MSG(_T("unexpected HitTestLine() return value"));
            // fall through

        case wxTE_HT_ON_TEXT:
            if ( col > 0 )
            {
                // the last entirely seen character is the previous one because
                // this one is only partly visible - unless the width of the
                // string is exactly the max width
                SData().m_posLastVisible = GetTextWidth(text.Truncate(col + 1));
                if ( SData().m_posLastVisible > m_rectText.width )
                {
                    // this character is not entirely visible, take the
                    // previous one
                    col--;

                    // recalc it
                    SData().m_posLastVisible = -1;
                }
                //else: we can just see it

                SData().m_colLastVisible = col;
            }
            break;
    }

    // calculate the width of the text really shown
    if ( SData().m_posLastVisible == -1 )
    {
        SData().m_posLastVisible = GetTextWidth(text.Truncate(SData().m_colLastVisible + 1));
    }

    // current value is relative the start of the string text which starts at
    // SData().m_colStart, we need an absolute offset into string
    SData().m_colLastVisible += SData().m_colStart;

    wxLogTrace(_T("text"), _T("Last visible column/position is %d/%ld"),
               (int) SData().m_colLastVisible, (long) SData().m_posLastVisible);
}

void wxTextCtrl::OnSize(wxSizeEvent& event)
{
    UpdateTextRect();

    if ( !IsSingleLine() )
    {
#if 0
        // update them immediately because if we are called for the first time,
        // we need to create them in order for the base class version to
        // position the scrollbars correctly - if we don't do it now, it won't
        // happen at all if we don't get more size events
        UpdateScrollbars();
#endif // 0

        MData().m_updateScrollbarX =
        MData().m_updateScrollbarY = true;
    }

    event.Skip();
}

wxCoord wxTextCtrl::GetTotalWidth() const
{
    wxCoord w;
    CalcUnscrolledPosition(m_rectText.width, 0, &w, NULL);
    return w;
}

wxCoord wxTextCtrl::GetTextWidth(const wxString& text) const
{
    wxCoord w;
    GetTextExtent(GetTextToShow(text), &w, NULL);
    return w;
}

wxRect wxTextCtrl::GetRealTextArea() const
{
    // for single line text control it's just the same as text rect
    if ( IsSingleLine() )
        return m_rectText;

    // the real text area always holds an entire number of lines, so the only
    // difference with the text area is a narrow strip along the bottom border
    wxRect rectText = MData().m_rectTextReal;
    if ( !rectText.width )
    {
        // recalculate it
        rectText = m_rectText;

        // when we're called for the very first time, the line height might not
        // had been calculated yet, so do get it now
        wxTextCtrl *self = wxConstCast(this, wxTextCtrl);
        self->RecalcFontMetrics();

        int hLine = GetLineHeight();
        rectText.height = (m_rectText.height / hLine) * hLine;

        // cache the result
        self->MData().m_rectTextReal = rectText;
    }

    return rectText;
}

wxTextCoord wxTextCtrl::GetRowInLine(wxTextCoord line,
                                     wxTextCoord col,
                                     wxTextCoord *colRowStart) const
{
    wxASSERT_MSG( WrapLines(), _T("shouldn't be called") );

    const wxWrappedLineData& lineData = WData().m_linesData[line];

    if ( !WData().IsValidLine(line) )
        LayoutLines(line);

    // row is here counted a bit specially: 0 is the 2nd row of the line (1st
    // extra row)
    size_t row = 0,
           rowMax = lineData.GetExtraRowCount();
    if ( rowMax )
    {
        row = 0;
        while ( (row < rowMax) && (col >= lineData.GetExtraRowStart(row)) )
            row++;

        // it's ok here that row is 1 greater than needed: like this, it is
        // counted as a normal (and not extra) row
    }
    //else: only one row anyhow

    if ( colRowStart )
    {
        // +1 because we need a real row number, not the extra row one
        *colRowStart = lineData.GetRowStart(row);

        // this can't happen, of course
        wxASSERT_MSG( *colRowStart <= col, _T("GetRowInLine() is broken") );
    }

    return row;
}

void wxTextCtrl::LayoutLine(wxTextCoord line, wxWrappedLineData& lineData) const
{
    // FIXME: this uses old GetPartOfWrappedLine() which is not used anywhere
    //        else now and has rather awkward interface for our needs here

    lineData.m_rowsStart.Empty();
    lineData.m_rowsWidth.Empty();

    const wxString& text = GetLineText(line);
    wxCoord widthRow;
    size_t colRowStart = 0;
    do
    {
        size_t lenRow = GetPartOfWrappedLine
                        (
                            text.c_str() + colRowStart,
                            &widthRow
                        );

        // remember the start of this row (not for the first one as
        // it's always 0) and its width
        if ( colRowStart )
            lineData.m_rowsStart.Add(colRowStart);
        lineData.m_rowsWidth.Add(widthRow);

        colRowStart += lenRow;
    }
    while ( colRowStart < text.length() );

    // put the current timestamp on it
    lineData.m_timestamp = WData().m_timestamp;
}

void wxTextCtrl::LayoutLines(wxTextCoord lineLast) const
{
    wxASSERT_MSG( WrapLines(), _T("should only be used for line wrapping") );

    // if we were called, some line was dirty and if it was dirty we must have
    // had m_rowFirstInvalid set to something too
    wxTextCoord lineFirst = WData().m_rowFirstInvalid;
    wxASSERT_MSG( lineFirst != -1, _T("nothing to layout?") );

    wxTextCoord rowFirst, rowCur;
    if ( lineFirst )
    {
        // start after the last known valid line
        const wxWrappedLineData& lineData = WData().m_linesData[lineFirst - 1];
        rowFirst = lineData.GetFirstRow() + lineData.GetRowCount();
    }
    else // no valid lines, start at row 0
    {
        rowFirst = 0;
    }

    rowCur = rowFirst;
    for ( wxTextCoord line = lineFirst; line <= lineLast; line++ )
    {
        // set the starting row for this line
        wxWrappedLineData& lineData = WData().m_linesData[line];
        lineData.m_rowFirst = rowCur;

        // had the line been already broken into rows?
        //
        // if so, compare its timestamp with the current one: if nothing has
        // been changed, don't relayout it
        if ( !lineData.IsValid() ||
                (lineData.m_timestamp < WData().m_timestamp) )
        {
            // now do break it in rows
            LayoutLine(line, lineData);
        }

        rowCur += lineData.GetRowCount();
    }

    // we are now valid at least up to this line, but if it is the last one we
    // just don't have any more invalid rows at all
    if ( (size_t)lineLast == WData().m_linesData.GetCount() -1 )
    {
        lineLast = -1;
    }

    wxTextCtrl *self = wxConstCast(this, wxTextCtrl);
    self->WData().m_rowFirstInvalid = lineLast;

    // also refresh the line end indicators (FIXME shouldn't do it always!)
    self->RefreshLineWrapMarks(rowFirst, rowCur);
}

size_t wxTextCtrl::GetPartOfWrappedLine(const wxChar* text,
                                        wxCoord *widthReal) const
{
    // this function is slow, it shouldn't be called unless really needed
    wxASSERT_MSG( WrapLines(), _T("shouldn't be called") );

    wxString s(text);
    wxTextCoord col;
    wxCoord wReal = wxDefaultCoord;
    switch ( HitTestLine(s, m_rectText.width, &col) )
    {
            /*
        case wxTE_HT_BEFORE:
        case wxTE_HT_BELOW:
            */
        default:
            wxFAIL_MSG(_T("unexpected HitTestLine() return value"));
            // fall through

        case wxTE_HT_ON_TEXT:
            if ( col > 0 )
            {
                // the last entirely seen character is the previous one because
                // this one is only partly visible - unless the width of the
                // string is exactly the max width
                wReal = GetTextWidth(s.Truncate(col + 1));
                if ( wReal > m_rectText.width )
                {
                    // this character is not entirely visible, take the
                    // previous one
                    col--;

                    // recalc the width
                    wReal = wxDefaultCoord;
                }
                //else: we can just see it

                // wrap at any character or only at words boundaries?
                if ( !(GetWindowStyle() & wxTE_CHARWRAP) )
                {
                    // find the (last) not word char before this word
                    wxTextCoord colWordStart;
                    for ( colWordStart = col;
                          colWordStart && IsWordChar(s[(size_t)colWordStart]);
                          colWordStart-- )
                        ;

                    if ( colWordStart > 0 )
                    {
                        if ( colWordStart != col )
                        {
                            // will have to recalc the real width
                            wReal = wxDefaultCoord;

                            col = colWordStart;
                        }
                    }
                    //else: only a single word, have to wrap it here
                }
            }
            break;

        case wxTE_HT_BEYOND:
            break;
    }

    // we return the number of characters, not the index of the last one
    if ( (size_t)col < s.length() )
    {
        // but don't return more than this (empty) string has
        col++;
    }

    if ( widthReal )
    {
        if ( wReal == wxDefaultCoord )
        {
            // calc it if not done yet
            wReal = GetTextWidth(s.Truncate(col));
        }

        *widthReal = wReal;
    }

    // VZ: old, horribly inefficient code which can still be used for checking
    //     the result (in line, not word, wrap mode only) - to be removed later
#if 0
    wxTextCtrl *self = wxConstCast(this, wxTextCtrl);
    wxClientDC dc(self);
    dc.SetFont(GetFont());
    self->DoPrepareDC(dc);

    wxCoord widthMax = m_rectText.width;

    // the text which we can keep in this ROW
    wxString str;
    wxCoord w, wOld;
    for ( wOld = w = 0; *text && (w <= widthMax); )
    {
        wOld = w;
        str += *text++;
        dc.GetTextExtent(str, &w, NULL);
    }

    if ( w > widthMax )
    {
        // if we wrapped, the last letter was one too much
        if ( str.length() > 1 )
        {
            // remove it
            str.erase(str.length() - 1, 1);
        }
        else // but always keep at least one letter in each row
        {
            // the real width then is the last value of w and not teh one
            // before last
            wOld = w;
        }
    }
    else // we didn't wrap
    {
        wOld = w;
    }

    wxASSERT( col == str.length() );

    if ( widthReal )
    {
        wxASSERT( *widthReal == wOld );

        *widthReal = wOld;
    }

    //return str.length();
#endif

    return col;
}

// OPT: this function is called a lot - would be nice to optimize it but I
//      don't really know how yet
wxTextCtrlHitTestResult wxTextCtrl::HitTestLine(const wxString& line,
                                                wxCoord x,
                                                wxTextCoord *colOut) const
{
    wxTextCtrlHitTestResult res = wxTE_HT_ON_TEXT;

    int col;
    wxTextCtrl *self = wxConstCast(this, wxTextCtrl);
    wxClientDC dc(self);
    dc.SetFont(GetFont());
    self->DoPrepareDC(dc);

    wxCoord width;
    dc.GetTextExtent(line, &width, NULL);
    if ( x >= width )
    {
        // clicking beyond the end of line is equivalent to clicking at
        // the end of it, so return the last line column
        col = line.length();
        if ( col )
        {
            // unless the line is empty and so doesn't have any column at all -
            // in this case return 0, what else can we do?
            col--;
        }

        res = wxTE_HT_BEYOND;
    }
    else if ( x < 0 )
    {
        col = 0;

        res = wxTE_HT_BEFORE;
    }
    else // we're inside the line
    {
        // now calculate the column: first, approximate it with fixed-width
        // value and then calculate the correct value iteratively: note that
        // we use the first character of the line instead of (average)
        // GetCharWidth(): it is common to have lines of dashes, for example,
        // and this should give us much better approximation in such case
        //
        // OPT: maybe using (cache) m_widthAvg would be still faster? profile!
        dc.GetTextExtent(line[0], &width, NULL);

        col = x / width;
        if ( col < 0 )
        {
            col = 0;
        }
        else if ( (size_t)col > line.length() )
        {
            col = line.length();
        }

        // matchDir is the direction in which we should move to reach the
        // character containing the given position
        enum
        {
            Match_Left  = -1,
            Match_None  = 0,
            Match_Right = 1
        } matchDir = Match_None;
        for ( ;; )
        {
            // check that we didn't go beyond the line boundary
            if ( col < 0 )
            {
                col = 0;
                break;
            }
            if ( (size_t)col > line.length() )
            {
                col = line.length();
                break;
            }

            wxString strBefore(line, (size_t)col);
            dc.GetTextExtent(strBefore, &width, NULL);
            if ( width > x )
            {
                if ( matchDir == Match_Right )
                {
                    // we were going to the right and, finally, moved beyond
                    // the original position - stop on the previous one
                    col--;

                    break;
                }

                if ( matchDir == Match_None )
                {
                    // we just started iterating, now we know that we should
                    // move to the left
                    matchDir = Match_Left;
                }
                //else: we are still to the right of the target, continue
            }
            else // width < x
            {
                // invert the logic above
                if ( matchDir == Match_Left )
                {
                    // with the exception that we don't need to backtrack here
                    break;
                }

                if ( matchDir == Match_None )
                {
                    // go to the right
                    matchDir = Match_Right;
                }
            }

            // this is not supposed to happen
            wxASSERT_MSG( matchDir, _T("logic error in wxTextCtrl::HitTest") );

            if ( matchDir == Match_Right )
                col++;
            else
                col--;
        }
    }

    // check that we calculated it correctly
#ifdef WXDEBUG_TEXT
    if ( res == wxTE_HT_ON_TEXT )
    {
        wxCoord width1;
        wxString text = line.Left(col);
        dc.GetTextExtent(text, &width1, NULL);
        if ( (size_t)col < line.length() )
        {
            wxCoord width2;

            text += line[col];
            dc.GetTextExtent(text, &width2, NULL);

            wxASSERT_MSG( (width1 <= x) && (x < width2),
                          _T("incorrect HitTestLine() result") );
        }
        else // we return last char
        {
            wxASSERT_MSG( x >= width1, _T("incorrect HitTestLine() result") );
        }
    }
#endif // WXDEBUG_TEXT

    if ( colOut )
        *colOut = col;

    return res;
}

wxTextCtrlHitTestResult wxTextCtrl::HitTest(const wxPoint& pt, long *pos) const
{
    wxTextCoord x, y;
    wxTextCtrlHitTestResult rc = HitTest(pt, &x, &y);
    if ( rc != wxTE_HT_UNKNOWN && pos )
    {
        *pos = XYToPosition(x, y);
    }

    return rc;
}

wxTextCtrlHitTestResult wxTextCtrl::HitTest(const wxPoint& pos,
                                            wxTextCoord *colOut,
                                            wxTextCoord *rowOut) const
{
    return HitTest2(pos.y, pos.x, 0, rowOut, colOut, NULL, NULL);
}

wxTextCtrlHitTestResult wxTextCtrl::HitTestLogical(const wxPoint& pos,
                                                   wxTextCoord *colOut,
                                                   wxTextCoord *rowOut) const
{
    return HitTest2(pos.y, pos.x, 0, rowOut, colOut, NULL, NULL, false);
}

wxTextCtrlHitTestResult wxTextCtrl::HitTest2(wxCoord y0,
                                             wxCoord x10,
                                             wxCoord x20,
                                             wxTextCoord *rowOut,
                                             wxTextCoord *colStart,
                                             wxTextCoord *colEnd,
                                             wxTextCoord *colRowStartOut,
                                             bool deviceCoords) const
{
    // is the point in the text area or to the right or below it?
    wxTextCtrlHitTestResult res = wxTE_HT_ON_TEXT;

    // translate the window coords x0 and y0 into the client coords in the text
    // area by adjusting for both the client and text area offsets (unless this
    // was already done)
    int x1, y;
    if ( deviceCoords )
    {
        wxPoint pt = GetClientAreaOrigin() + m_rectText.GetPosition();
        CalcUnscrolledPosition(x10 - pt.x, y0 - pt.y, &x1, &y);
    }
    else
    {
        y = y0;
        x1 = x10;
    }

    // calculate the row (it is really a LINE, not a ROW)
    wxTextCoord row;

    // these vars are used only for WrapLines() case
    wxTextCoord colRowStart = 0;
    size_t rowLen = 0;

    if ( colRowStartOut )
        *colRowStartOut = 0;

    int hLine = GetLineHeight();
    if ( y < 0 )
    {
        // and clicking before it is the same as clicking on the first one
        row = 0;

        res = wxTE_HT_BEFORE;
    }
    else // y >= 0
    {
        wxTextCoord rowLast = GetNumberOfLines() - 1;
        row = y / hLine;
        if ( IsSingleLine() || !WrapLines() )
        {
            // in this case row calculation is simple as all lines have the
            // same height and so row is the same as line
            if ( row > rowLast )
            {
                // clicking below the text is the same as clicking on the last
                // line
                row = rowLast;

                res = wxTE_HT_BELOW;
            }
        }
        else // multline control with line wrap
        {
            // use binary search to find the line containing this row
            const wxArrayWrappedLinesData& linesData = WData().m_linesData;
            size_t lo = 0,
                   hi = linesData.GetCount(),
                   cur;
            while ( lo < hi )
            {
                cur = (lo + hi)/2;
                const wxWrappedLineData& lineData = linesData[cur];
                if ( !WData().IsValidLine(cur) )
                    LayoutLines(cur);
                wxTextCoord rowFirst = lineData.GetFirstRow();

                if ( row < rowFirst )
                {
                    hi = cur;
                }
                else
                {
                    // our row is after the first row of the cur line:
                    // obviously, if cur is the last line, it contains this
                    // row, otherwise we have to test that it is before the
                    // first row of the next line
                    bool found = cur == linesData.GetCount() - 1;
                    if ( found )
                    {
                        // if the row is beyond the end of text, adjust it to
                        // be the last one and set res accordingly
                        if ( (size_t)(row - rowFirst) >= lineData.GetRowCount() )
                        {
                            res = wxTE_HT_BELOW;

                            row = lineData.GetRowCount() + rowFirst - 1;
                        }
                    }
                    else // not the last row
                    {
                        const wxWrappedLineData&
                                lineNextData = linesData[cur + 1];
                        if ( !WData().IsValidLine(cur + 1) )
                            LayoutLines(cur + 1);
                        found = row < lineNextData.GetFirstRow();
                    }

                    if ( found )
                    {
                        colRowStart = lineData.GetRowStart(row - rowFirst);
                        rowLen = lineData.GetRowLength(row - rowFirst,
                                                       GetLines()[cur].length());
                        row = cur;

                        break;
                    }
                    else
                    {
                        lo = cur;
                    }
                }
            }
        }
    }

    if ( res == wxTE_HT_ON_TEXT )
    {
        // now find the position in the line
        wxString lineText = GetLineText(row),
                 rowText;

        if ( colRowStart || rowLen )
        {
            // look in this row only, not in whole line
            rowText = lineText.Mid(colRowStart, rowLen);
        }
        else
        {
            // just take the whole string
            rowText = lineText;
        }

        if ( colStart )
        {
            res = HitTestLine(GetTextToShow(rowText), x1, colStart);

            if ( colRowStart )
            {
                if ( colRowStartOut )
                {
                    // give them the column offset in this ROW in pixels
                    *colRowStartOut = colRowStart;
                }

                // take into account that the ROW doesn't start in the
                // beginning of the LINE
                *colStart += colRowStart;
            }

            if ( colEnd )
            {
                // the hit test result we return is for x1, so throw out
                // the result for x2 here
                int x2 = x1 + x20 - x10;
                (void)HitTestLine(GetTextToShow(rowText), x2, colEnd);

                *colEnd += colRowStart;
            }
        }
    }
    else // before/after vertical text span
    {
        if ( colStart )
        {
            // fill the column with the first/last position in the
            // corresponding line
            if ( res == wxTE_HT_BEFORE )
                *colStart = 0;
            else // res == wxTE_HT_BELOW
                *colStart = GetLineText(GetNumberOfLines() - 1).length();
        }
    }

    if ( rowOut )
    {
        // give them the row in text coords (as is)
        *rowOut = row;
    }

    return res;
}

bool wxTextCtrl::GetLineAndRow(wxTextCoord row,
                               wxTextCoord *lineOut,
                               wxTextCoord *rowInLineOut) const
{
    wxTextCoord line,
                rowInLine = 0;

    if ( row < 0 )
        return false;

    int nLines = GetNumberOfLines();
    if ( WrapLines() )
    {
        const wxArrayWrappedLinesData& linesData = WData().m_linesData;
        for ( line = 0; line < nLines; line++ )
        {
            if ( !WData().IsValidLine(line) )
                LayoutLines(line);

            if ( row < linesData[line].GetNextRow() )
            {
                // we found the right line
                rowInLine = row - linesData[line].GetFirstRow();

                break;
            }
        }

        if ( line == nLines )
        {
            // the row is out of range
            return false;
        }
    }
    else // no line wrapping, everything is easy
    {
        if ( row >= nLines )
            return false;

        line = row;
    }

    if ( lineOut )
        *lineOut = line;
    if ( rowInLineOut )
        *rowInLineOut = rowInLine;

    return true;
}

// ----------------------------------------------------------------------------
// scrolling
// ----------------------------------------------------------------------------

/*
   wxTextCtrl has not one but two scrolling mechanisms: one is semi-automatic
   scrolling in both horizontal and vertical direction implemented using
   wxScrollHelper and the second one is manual scrolling implemented using
   SData().m_ofsHorz and used by the single line controls without scroll bar.

   The first version (the standard one) always scrolls by fixed amount which is
   fine for vertical scrolling as all lines have the same height but is rather
   ugly for horizontal scrolling if proportional font is used. This is why we
   manually update and use SData().m_ofsHorz which contains the length of the string
   which is hidden beyond the left border. An important property of text
   controls using this kind of scrolling is that an entire number of characters
   is always shown and that parts of characters never appear on display -
   neither in the leftmost nor rightmost positions.

   Once again, for multi line controls SData().m_ofsHorz is always 0 and scrolling is
   done as usual for wxScrollWindow.
 */

void wxTextCtrl::ShowHorzPosition(wxCoord pos)
{
    wxASSERT_MSG( IsSingleLine(), _T("doesn't work for multiline") );

    // pos is the logical position to show

    // SData().m_ofsHorz is the first logical position shown
    if ( pos < SData().m_ofsHorz )
    {
        // scroll backwards
        wxTextCoord col;
        HitTestLine(m_value, pos, &col);
        ScrollText(col);
    }
    else
    {
        wxCoord width = m_rectText.width;
        if ( !width )
        {
            // if we are called from the ctor, m_rectText is not initialized
            // yet, so do it now
            UpdateTextRect();
            width = m_rectText.width;
        }

        // SData().m_ofsHorz + width is the last logical position shown
        if ( pos > SData().m_ofsHorz + width)
        {
            // scroll forward
            wxTextCoord col;
            HitTestLine(m_value, pos - width, &col);
            ScrollText(col + 1);
        }
    }
}

// scroll the window horizontally so that the first visible character becomes
// the one at this position
void wxTextCtrl::ScrollText(wxTextCoord col)
{
    wxASSERT_MSG( IsSingleLine(),
                  _T("ScrollText() is for single line controls only") );

    // never scroll beyond the left border
    if ( col < 0 )
        col = 0;

    // OPT: could only get the extent of the part of the string between col
    //      and SData().m_colStart
    wxCoord ofsHorz = GetTextWidth(GetLineText(0).Left(col));

    if ( ofsHorz != SData().m_ofsHorz )
    {
        // remember the last currently used pixel
        int posLastVisible = SData().m_posLastVisible;
        if ( posLastVisible == -1 )
        {
            // this may happen when we're called very early, during the
            // controls construction
            UpdateLastVisible();

            posLastVisible = SData().m_posLastVisible;
        }

        // NB1: to scroll to the right, offset must be negative, hence the
        //      order of operands
        int dx = SData().m_ofsHorz - ofsHorz;

        // NB2: we call Refresh() below which results in a call to
        //      DoDraw(), so we must update SData().m_ofsHorz before calling it
        SData().m_ofsHorz = ofsHorz;
        SData().m_colStart = col;

        // after changing m_colStart, recalc the last visible position: we need
        // to recalc the last visible position beore scrolling in order to make
        // it appear exactly at the right edge of the text area after scrolling
        UpdateLastVisible();

#if 0 // do we?
        if ( dx < 0 )
        {
            // we want to force the update of it after scrolling
            SData().m_colLastVisible = -1;
        }
#endif

        // scroll only the rectangle inside which there is the text
        wxRect rect = m_rectText;
        rect.width = posLastVisible;

        rect = ScrollNoRefresh(dx, 0, &rect);

        /*
           we need to manually refresh the part which ScrollWindow() doesn't
           refresh (with new API this means the part outside the rect returned
           by ScrollNoRefresh): indeed, if we had this:

                                   ********o

           where '*' is text and 'o' is blank area at the end (too small to
           hold the next char) then after scrolling by 2 positions to the left
           we're going to have

                                   ******RRo

           where 'R' is the area refreshed by ScrollWindow() - but we still
           need to refresh the 'o' at the end as it may be now big enough to
           hold the new character shifted into view.

           when we are scrolling to the right, we need to update this rect as
           well because it might have contained something before but doesn't
           contain anything any more
         */

        // we can combine both rectangles into one when scrolling to the left,
        // but we need two separate Refreshes() otherwise
        if ( dx > 0 )
        {
            // refresh the uncovered part on the left
            Refresh(true, &rect);

            // and now the area on the right
            rect.x = m_rectText.x + posLastVisible;
            rect.width = m_rectText.width - posLastVisible;
        }
        else // scrolling to the left
        {
            // just extend the rect covering the uncovered area to the edge of
            // the text rect
            rect.width += m_rectText.width - posLastVisible;
        }

        Refresh(true, &rect);

        // I don't know exactly why is this needed here but without it we may
        // scroll the window again (from the same method) before the previously
        // invalidated area is repainted when typing *very* quickly - and this
        // may lead to the display corruption
        Update();
    }
}

void wxTextCtrl::CalcUnscrolledPosition(int x, int y, int *xx, int *yy) const
{
    if ( IsSingleLine() )
    {
        // we don't use wxScrollHelper
        if ( xx )
            *xx = x + SData().m_ofsHorz;
        if ( yy )
            *yy = y;
    }
    else
    {
        // let the base class do it
        wxScrollHelper::CalcUnscrolledPosition(x, y, xx, yy);
    }
}

void wxTextCtrl::CalcScrolledPosition(int x, int y, int *xx, int *yy) const
{
    if ( IsSingleLine() )
    {
        // we don't use wxScrollHelper
        if ( xx )
            *xx = x - SData().m_ofsHorz;
        if ( yy )
            *yy = y;
    }
    else
    {
        // let the base class do it
        wxScrollHelper::CalcScrolledPosition(x, y, xx, yy);
    }
}

void wxTextCtrl::DoPrepareDC(wxDC& dc)
{
    // for single line controls we only have to deal with SData().m_ofsHorz and it's
    // useless to call base class version as they don't use normal scrolling
    if ( IsSingleLine() && SData().m_ofsHorz )
    {
        // adjust the DC origin if the text is shifted
        wxPoint pt = dc.GetDeviceOrigin();
        dc.SetDeviceOrigin(pt.x - SData().m_ofsHorz, pt.y);
    }
    else
    {
        wxScrollHelper::DoPrepareDC(dc);
    }
}

void wxTextCtrl::UpdateMaxWidth(wxTextCoord line)
{
    // OPT!

    // check if the max width changes after this line was modified
    wxCoord widthMaxOld = MData().m_widthMax,
            width;
    GetTextExtent(GetLineText(line), &width, NULL);

    if ( line == MData().m_lineLongest )
    {
        // this line was the longest one, is it still?
        if ( width > MData().m_widthMax )
        {
            MData().m_widthMax = width;
        }
        else if ( width < MData().m_widthMax )
        {
            // we need to find the new longest line
            RecalcMaxWidth();
        }
        //else: its length didn't change, nothing to do
    }
    else // it wasn't the longest line, but maybe it became it?
    {
        // GetMaxWidth() and not MData().m_widthMax as it might be not calculated yet
        if ( width > GetMaxWidth() )
        {
            MData().m_widthMax = width;
            MData().m_lineLongest = line;
        }
    }

    MData().m_updateScrollbarX = MData().m_widthMax != widthMaxOld;
}

void wxTextCtrl::RecalcFontMetrics()
{
    m_heightLine = GetCharHeight();
    m_widthAvg = GetCharWidth();
}

void wxTextCtrl::RecalcMaxWidth()
{
    wxASSERT_MSG( !IsSingleLine(), _T("only used for multiline") );

    MData().m_widthMax = -1;
    (void)GetMaxWidth();
}

wxCoord wxTextCtrl::GetMaxWidth() const
{
    if ( MData().m_widthMax == -1 )
    {
        // recalculate it

        // OPT: should we remember the widths of all the lines?

        wxTextCtrl *self = wxConstCast(this, wxTextCtrl);
        wxClientDC dc(self);
        dc.SetFont(GetFont());

        self->MData().m_widthMax = 0;

        size_t count = GetLineCount();
        for ( size_t n = 0; n < count; n++ )
        {
            wxCoord width;
            dc.GetTextExtent(GetLines()[n], &width, NULL);
            if ( width > MData().m_widthMax )
            {
                // remember the width and the line which has it
                self->MData().m_widthMax = width;
                self->MData().m_lineLongest = n;
            }
        }
    }

    wxASSERT_MSG( MData().m_widthMax != -1, _T("should have at least 1 line") );

    return MData().m_widthMax;
}

void wxTextCtrl::UpdateScrollbars()
{
    wxASSERT_MSG( !IsSingleLine(), _T("only used for multiline") );

    wxSize size = GetRealTextArea().GetSize();

    // is our height enough to show all items?
    wxTextCoord nRows = GetRowCount();
    wxCoord lineHeight = GetLineHeight();
    bool showScrollbarY = nRows*lineHeight > size.y;

    // is our width enough to show the longest line?
    wxCoord charWidth, maxWidth;
    bool showScrollbarX;
    if ( !WrapLines() )
    {
        charWidth = GetAverageWidth();
        maxWidth = GetMaxWidth();
        showScrollbarX = maxWidth > size.x;
    }
    else // never show the horz scrollbar
    {
        // just to suppress compiler warnings about using uninit vars below
        charWidth = maxWidth = 0;

        showScrollbarX = false;
    }

    // calc the scrollbars ranges
    int scrollRangeX = showScrollbarX
                        ? (maxWidth + 2*charWidth - 1) / charWidth
                        : 0;
    int scrollRangeY = showScrollbarY ? nRows : 0;

    int scrollRangeXOld = MData().m_scrollRangeX,
        scrollRangeYOld = MData().m_scrollRangeY;
    if ( (scrollRangeY != scrollRangeYOld) || (scrollRangeX != scrollRangeXOld) )
    {
        int x, y;
        GetViewStart(&x, &y);

#if 0
        // we want to leave the scrollbars at the same position which means
        // that x and y have to be adjusted as the number of positions may have
        // changed
        //
        // the number of positions is calculated from knowing that last
        // position = range - thumbSize and thumbSize == pageSize which is
        // equal to the window width / pixelsPerLine
        if ( scrollRangeXOld )
        {
            x *= scrollRangeX - m_rectText.width / charWidth;
            x /= scrollRangeXOld - m_rectText.width / charWidth;
        }

        if ( scrollRangeYOld )
            y *= scrollRangeY / scrollRangeYOld;
#endif // 0

        SetScrollbars(charWidth, lineHeight,
                      scrollRangeX, scrollRangeY,
                      x, y,
                      true /* no refresh */);

        if ( scrollRangeXOld )
        {
            const int w = m_rectText.width / charWidth;
            if ( w != scrollRangeXOld )
            {
                x *= scrollRangeX - w;
                x /= scrollRangeXOld - w;
            }
            Scroll(x, y);
        }

        MData().m_scrollRangeX = scrollRangeX;
        MData().m_scrollRangeY = scrollRangeY;

        // bring the current position in view
        ShowPosition(-1);
    }

    MData().m_updateScrollbarX =
    MData().m_updateScrollbarY = false;
}

void wxTextCtrl::OnInternalIdle()
{
    // notice that single line text control never has scrollbars
    if ( !IsSingleLine() &&
            (MData().m_updateScrollbarX || MData().m_updateScrollbarY) )
    {
        UpdateScrollbars();
    }
    wxControl::OnInternalIdle();
}

bool wxTextCtrl::SendAutoScrollEvents(wxScrollWinEvent& event) const
{
    bool forward = event.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN;
    if ( event.GetOrientation() == wxHORIZONTAL )
    {
        return forward ? m_curCol <= GetLineLength(m_curRow) : m_curCol > 0;
    }
    else // wxVERTICAL
    {
        return forward ? m_curRow < GetNumberOfLines() : m_curRow > 0;
    }
}

// ----------------------------------------------------------------------------
// refresh
// ----------------------------------------------------------------------------

void wxTextCtrl::RefreshSelection()
{
    if ( HasSelection() )
    {
        RefreshTextRange(m_selStart, m_selEnd);
    }
}

void wxTextCtrl::RefreshLineRange(wxTextCoord lineFirst, wxTextCoord lineLast)
{
    wxASSERT_MSG( lineFirst <= lineLast || !lineLast,
                  _T("no lines to refresh") );

    wxRect rect;
    // rect.x is already 0
    rect.width = m_rectText.width;
    wxCoord h = GetLineHeight();

    wxTextCoord rowFirst;
    if ( lineFirst < GetNumberOfLines() )
    {
        rowFirst = GetFirstRowOfLine(lineFirst);
    }
    else // lineFirst == GetNumberOfLines()
    {
        // lineFirst may be beyond the last line only if we refresh till
        // the end, otherwise it's illegal
        wxASSERT_MSG( lineFirst == GetNumberOfLines() && !lineLast,
                      _T("invalid line range") );

        rowFirst = GetRowAfterLine(lineFirst - 1);
    }

    rect.y = rowFirst*h;

    if ( lineLast )
    {
        // refresh till this line (inclusive)
        wxTextCoord rowLast = GetRowAfterLine(lineLast);

        rect.height = (rowLast - rowFirst + 1)*h;
    }
    else // lineLast == 0 means to refresh till the end
    {
        // FIXME: calc it exactly
        rect.height = 32000;
    }

    RefreshTextRect(rect);
}

void wxTextCtrl::RefreshTextRange(wxTextPos start, wxTextPos end)
{
    wxCHECK_RET( start != -1 && end != -1,
                 _T("invalid RefreshTextRange() arguments") );

    // accept arguments in any order as it is more conenient for the caller
    OrderPositions(start, end);

    // this is acceptable but we don't do anything in this case
    if ( start == end )
        return;

    wxTextPos colStart, lineStart;
    if ( !PositionToXY(start, &colStart, &lineStart) )
    {
        // the range is entirely beyond the end of the text, nothing to do
        return;
    }

    wxTextCoord colEnd, lineEnd;
    if ( !PositionToXY(end, &colEnd, &lineEnd) )
    {
        // the range spans beyond the end of text, refresh to the end
        colEnd = -1;
        lineEnd = GetNumberOfLines() - 1;
    }

    // refresh all lines one by one
    for ( wxTextCoord line = lineStart; line <= lineEnd; line++ )
    {
        // refresh the first line from the start of the range to the end, the
        // intermediate ones entirely and the last one from the beginning to
        // the end of the range
        wxTextPos posStart = line == lineStart ? colStart : 0;
        size_t posCount;
        if ( (line != lineEnd) || (colEnd == -1) )
        {
            // intermediate line or the last one but we need to refresh it
            // until the end anyhow - do it
            posCount = wxString::npos;
        }
        else // last line
        {
            // refresh just the positions in between the start and the end one
            posCount = colEnd - posStart;
        }

        if ( posCount )
            RefreshColRange(line, posStart, posCount);
    }
}

void wxTextCtrl::RefreshColRange(wxTextCoord line,
                                 wxTextPos start,
                                 size_t count)
{
    wxString text = GetLineText(line);

    wxASSERT_MSG( (size_t)start <= text.length() && count,
                  _T("invalid RefreshColRange() parameter") );

    RefreshPixelRange(line,
                      GetTextWidth(text.Left((size_t)start)),
                      GetTextWidth(text.Mid((size_t)start, (size_t)count)));
}

// this method accepts "logical" coords in the sense that they are coordinates
// in a logical line but it can span several rows if we wrap lines and
// RefreshPixelRange() will then refresh several rows
void wxTextCtrl::RefreshPixelRange(wxTextCoord line,
                                   wxCoord start,
                                   wxCoord width)
{
    // we will use line text only in line wrap case
    wxString text;
    if ( WrapLines() )
    {
        text = GetLineText(line);
    }

    // special case: width == 0 means to refresh till the end of line
    if ( width == 0 )
    {
        // refresh till the end of visible line
        width = GetTotalWidth();

        if ( WrapLines() )
        {
            // refresh till the end of text
            wxCoord widthAll = GetTextWidth(text);

            // extend width to the end of ROW
            width = widthAll - widthAll % width + width;
        }

        // no need to refresh beyond the end of line
        width -= start;
    }
    //else: just refresh the specified part

    wxCoord h = GetLineHeight();
    wxRect rect;
    rect.x = start;
    rect.y = GetFirstRowOfLine(line)*h;
    rect.height = h;

    if ( WrapLines() )
    {
        // (1) skip all rows which we don't touch at all
        const wxWrappedLineData& lineData = WData().m_linesData[line];
        if ( !WData().IsValidLine(line) )
            LayoutLines(line);

        wxCoord wLine = 0; // suppress compiler warning about uninit var
        size_t rowLast = lineData.GetRowCount(),
               row = 0;
        while ( (row < rowLast) &&
                (rect.x > (wLine = lineData.GetRowWidth(row++))) )
        {
            rect.x -= wLine;
            rect.y += h;
        }

        // (2) now refresh all lines except the last one: note that the first
        //     line is refreshed from the given start to the end, all the next
        //     ones - entirely
        while ( (row < rowLast) && (width > wLine - rect.x) )
        {
            rect.width = GetTotalWidth() - rect.x;
            RefreshTextRect(rect);

            width -= wLine - rect.x;
            rect.x = 0;
            rect.y += h;

            wLine = lineData.GetRowWidth(row++);
        }

        // (3) the code below will refresh the last line
    }

    rect.width = width;

    RefreshTextRect(rect);
}

void wxTextCtrl::RefreshTextRect(const wxRect& rectClient, bool textOnly)
{
    wxRect rect;
    CalcScrolledPosition(rectClient.x, rectClient.y, &rect.x, &rect.y);
    rect.width = rectClient.width;
    rect.height = rectClient.height;

    // account for the text area offset
    rect.Offset(m_rectText.GetPosition());

    // don't refresh beyond the text area unless we're refreshing the line wrap
    // marks in which case textOnly is false
    if ( textOnly )
    {
        if ( rect.GetRight() > m_rectText.GetRight() )
        {
            rect.SetRight(m_rectText.GetRight());

            if ( rect.width <= 0 )
            {
                // nothing to refresh
                return;
            }
        }
    }

    // check the bottom boundary always, even for the line wrap marks
    if ( rect.GetBottom() > m_rectText.GetBottom() )
    {
        rect.SetBottom(m_rectText.GetBottom());

        if ( rect.height <= 0 )
        {
            // nothing to refresh
            return;
        }
    }

    // never refresh before the visible rect
    if ( rect.x < m_rectText.x )
        rect.x = m_rectText.x;

    if ( rect.y < m_rectText.y )
        rect.y = m_rectText.y;

    wxLogTrace(_T("text"), _T("Refreshing (%d, %d)-(%d, %d)"),
               rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);

    Refresh(true, &rect);
}

void wxTextCtrl::RefreshLineWrapMarks(wxTextCoord rowFirst,
                                      wxTextCoord rowLast)
{
    if ( WData().m_widthMark )
    {
        wxRect rectMarks;
        rectMarks.x = m_rectText.width;
        rectMarks.width = WData().m_widthMark;
        rectMarks.y = rowFirst*GetLineHeight();
        rectMarks.height = (rowLast - rowFirst)*GetLineHeight();

        RefreshTextRect(rectMarks, false /* don't limit to text area */);
    }
}

// ----------------------------------------------------------------------------
// border drawing
// ----------------------------------------------------------------------------

void wxTextCtrl::DoDrawBorder(wxDC& dc, const wxRect& rect)
{
    m_renderer->DrawTextBorder(dc, GetBorder(), rect, GetStateFlags());
}

// ----------------------------------------------------------------------------
// client area drawing
// ----------------------------------------------------------------------------

/*
   Several remarks about wxTextCtrl redraw logic:

   1. only the regions which must be updated are redrawn, this means that we
      never Refresh() the entire window but use RefreshPixelRange() and
      ScrollWindow() which only refresh small parts of it and iterate over the
      update region in our DoDraw()

   2. the text displayed on the screen is obtained using GetTextToShow(): it
      should be used for all drawing/measuring
 */

wxString wxTextCtrl::GetTextToShow(const wxString& text) const
{
    wxString textShown;
    if ( IsPassword() )
        textShown = wxString(_T('*'), text.length());
    else
        textShown = text;

    return textShown;
}

void wxTextCtrl::DoDrawTextInRect(wxDC& dc, const wxRect& rectUpdate)
{
    // debugging trick to see the update rect visually
#ifdef WXDEBUG_TEXT
    static int s_countUpdates = -1;
    if ( s_countUpdates != -1 )
    {
        wxWindowDC dc(this);
        dc.SetBrush(*(++s_countUpdates % 2 ? wxRED_BRUSH : wxGREEN_BRUSH));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(rectUpdate);
    }
#endif // WXDEBUG_TEXT

    // calculate the range lineStart..lineEnd of lines to redraw
    wxTextCoord lineStart, lineEnd;
    if ( IsSingleLine() )
    {
        lineStart =
        lineEnd = 0;
    }
    else // multiline
    {
        wxPoint pt = rectUpdate.GetPosition();
        (void)HitTest(pt, NULL, &lineStart);

        pt.y += rectUpdate.height;
        (void)HitTest(pt, NULL, &lineEnd);
    }

    // prepare for drawing
    wxCoord hLine = GetLineHeight();

    // these vars will be used for hit testing of the current row
    wxCoord y = rectUpdate.y;
    const wxCoord x1 = rectUpdate.x;
    const wxCoord x2 = rectUpdate.x + rectUpdate.width;

    wxRect rectText;
    rectText.height = hLine;
    wxCoord yClient = y - GetClientAreaOrigin().y;

    // we want to always start at the top of the line, otherwise if we redraw a
    // rect whose top is in the middle of a line, we'd draw this line shifted
    yClient -= (yClient - m_rectText.y) % hLine;

    if ( IsSingleLine() )
    {
        rectText.y = yClient;
    }
    else // multiline, adjust for scrolling
    {
        CalcUnscrolledPosition(0, yClient, NULL, &rectText.y);
    }

    wxRenderer *renderer = GetRenderer();

    // do draw the invalidated parts of each line: note that we iterate here
    // over ROWs, not over LINEs
    for ( wxTextCoord line = lineStart;
          y < rectUpdate.y + rectUpdate.height;
          y += hLine,
          rectText.y += hLine )
    {
        // calculate the update rect in text positions for this line
        wxTextCoord colStart, colEnd, colRowStart;
        wxTextCtrlHitTestResult ht = HitTest2(y, x1, x2,
                                              &line, &colStart, &colEnd,
                                              &colRowStart);

        if ( (ht == wxTE_HT_BEYOND) || (ht == wxTE_HT_BELOW) )
        {
            wxASSERT_MSG( line <= lineEnd, _T("how did we get that far?") );

            if ( line == lineEnd )
            {
                // we redrew everything
                break;
            }

            // the update rect is beyond the end of line, no need to redraw
            // anything on this line - but continue with the remaining ones
            continue;
        }

        // for single line controls we may additionally cut off everything
        // which is to the right of the last visible position
        if ( IsSingleLine() )
        {
            // don't show the columns which are scrolled out to the left
            if ( colStart < SData().m_colStart )
                colStart = SData().m_colStart;

            // colEnd may be less than colStart if colStart was changed by the
            // assignment above
            if ( colEnd < colStart )
                colEnd = colStart;

            // don't draw the chars beyond the rightmost one
            if ( SData().m_colLastVisible == -1 )
            {
                // recalculate this rightmost column
                UpdateLastVisible();
            }

            if ( colStart > SData().m_colLastVisible )
            {
                // don't bother redrawing something that is beyond the last
                // visible position
                continue;
            }

            if ( colEnd > SData().m_colLastVisible )
            {
                colEnd = SData().m_colLastVisible;
            }
        }

        // extract the part of line we need to redraw
        wxString textLine = GetTextToShow(GetLineText(line));
        wxString text = textLine.Mid(colStart, colEnd - colStart + 1);

        // now deal with the selection: only do something if at least part of
        // the line is selected
        wxTextPos selStart, selEnd;
        if ( GetSelectedPartOfLine(line, &selStart, &selEnd) )
        {
            // and if this part is (at least partly) in the current row
            if ( (selStart <= colEnd) &&
                    (selEnd >= wxMax(colStart, colRowStart)) )
            {
                // these values are relative to the start of the line while the
                // string passed to DrawTextLine() is only part of it, so
                // adjust the selection range accordingly
                selStart -= colStart;
                selEnd -= colStart;

                if ( selStart < 0 )
                    selStart = 0;

                if ( (size_t)selEnd >= text.length() )
                    selEnd = text.length();
            }
            else
            {
                // reset selStart and selEnd to avoid passing them to
                // DrawTextLine() below
                selStart =
                selEnd = -1;
            }
        }

        // calculate the text coords on screen
        wxASSERT_MSG( colStart >= colRowStart, _T("invalid string part") );
        wxCoord ofsStart = GetTextWidth(
                                    textLine.Mid(colRowStart,
                                                 colStart - colRowStart));
        rectText.x = m_rectText.x + ofsStart;
        rectText.width = GetTextWidth(text);

        // do draw the text
        renderer->DrawTextLine(dc, text, rectText, selStart, selEnd,
                               GetStateFlags());
        wxLogTrace(_T("text"), _T("Line %ld: positions %ld-%ld redrawn."),
                   line, colStart, colEnd);
    }
}

void wxTextCtrl::DoDrawLineWrapMarks(wxDC& dc, const wxRect& rectUpdate)
{
    wxASSERT_MSG( WrapLines() && WData().m_widthMark,
                  _T("shouldn't be called at all") );

    wxRenderer *renderer = GetRenderer();

    wxRect rectMark;
    rectMark.x = rectUpdate.x;
    rectMark.width = rectUpdate.width;
    wxCoord yTop = GetClientAreaOrigin().y;
    CalcUnscrolledPosition(0, rectUpdate.y - yTop, NULL, &rectMark.y);
    wxCoord hLine = GetLineHeight();
    rectMark.height = hLine;

    wxTextCoord line, rowInLine;

    wxCoord yBottom;
    CalcUnscrolledPosition(0, rectUpdate.GetBottom() - yTop, NULL, &yBottom);
    for ( ; rectMark.y < yBottom; rectMark.y += hLine )
    {
        if ( !GetLineAndRow(rectMark.y / hLine, &line, &rowInLine) )
        {
            // we went beyond the end of text
            break;
        }

        // is this row continued on the next one?
        if ( !WData().m_linesData[line].IsLastRow(rowInLine) )
        {
            renderer->DrawLineWrapMark(dc, rectMark);
        }
    }
}

void wxTextCtrl::DoDraw(wxControlRenderer *renderer)
{
    // hide the caret while we're redrawing the window and show it after we are
    // done with it
    wxCaretSuspend cs(this);

    // prepare the DC
    wxDC& dc = renderer->GetDC();
    dc.SetFont(GetFont());
    dc.SetTextForeground(GetForegroundColour());

    // get the intersection of the update region with the text area: note that
    // the update region is in window coords and text area is in the client
    // ones, so it must be shifted before computing intersection
    wxRegion rgnUpdate = GetUpdateRegion();

    wxRect rectTextArea = GetRealTextArea();
    wxPoint pt = GetClientAreaOrigin();
    wxRect rectTextAreaAdjusted = rectTextArea;
    rectTextAreaAdjusted.x += pt.x;
    rectTextAreaAdjusted.y += pt.y;
    rgnUpdate.Intersect(rectTextAreaAdjusted);

    // even though the drawing is already clipped to the update region, we must
    // explicitly clip it to the rect we will use as otherwise parts of letters
    // might be drawn outside of it (if even a small part of a charater is
    // inside, HitTest() will return its column and DrawText() can't draw only
    // the part of the character, of course)
#ifdef __WXMSW__
    // FIXME: is this really a bug in wxMSW?
    rectTextArea.width--;
#endif // __WXMSW__
    dc.SetClippingRegion(rectTextArea);

    // adjust for scrolling
    DoPrepareDC(dc);

    // and now refresh the invalidated parts of the window
    wxRegionIterator iter(rgnUpdate);
    for ( ; iter.HaveRects(); iter++ )
    {
        wxRect r = iter.GetRect();

        // this is a workaround for wxGTK::wxRegion bug
#ifdef __WXGTK__
        if ( !r.width || !r.height )
        {
            // ignore invalid rect
            continue;
        }
#endif // __WXGTK__

        DoDrawTextInRect(dc, r);
    }

    // now redraw the line wrap marks (if we draw them)
    if ( WrapLines() && WData().m_widthMark )
    {
        // this is the rect inside which line wrap marks are drawn
        wxRect rectMarks;
        rectMarks.x = rectTextAreaAdjusted.GetRight() + 1;
        rectMarks.y = rectTextAreaAdjusted.y;
        rectMarks.width = WData().m_widthMark;
        rectMarks.height = rectTextAreaAdjusted.height;

        rgnUpdate = GetUpdateRegion();
        rgnUpdate.Intersect(rectMarks);

        wxRect rectUpdate = rgnUpdate.GetBox();
        if ( rectUpdate.width && rectUpdate.height )
        {
            // the marks are outside previously set clipping region
            dc.DestroyClippingRegion();

            DoDrawLineWrapMarks(dc, rectUpdate);
        }
    }

    // show caret first time only: we must show it after drawing the text or
    // the display can be corrupted when it's hidden
    if ( !m_hasCaret && GetCaret() && (FindFocus() == this) )
    {
        ShowCaret();

        m_hasCaret = true;
    }
}

// ----------------------------------------------------------------------------
// caret
// ----------------------------------------------------------------------------

bool wxTextCtrl::SetFont(const wxFont& font)
{
    if ( !wxControl::SetFont(font) )
        return false;

    // and refresh everything, of course
    InitInsertionPoint();
    ClearSelection();

    // update geometry parameters
    UpdateTextRect();
    RecalcFontMetrics();
    if ( !IsSingleLine() )
    {
        UpdateScrollbars();
        RecalcMaxWidth();
    }

    // recreate it, in fact
    CreateCaret();

    Refresh();

    return true;
}

bool wxTextCtrl::Enable(bool enable)
{
    if ( !wxTextCtrlBase::Enable(enable) )
        return false;

    if (FindFocus() == this && GetCaret() &&
        ((enable && !GetCaret()->IsVisible()) ||
         (!enable && GetCaret()->IsVisible())))
        ShowCaret(enable);

    return true;
}

void wxTextCtrl::CreateCaret()
{
    wxCaret *caret;

    if ( IsEditable() )
    {
        // FIXME use renderer
        caret = new wxCaret(this, 1, GetLineHeight());
    }
    else
    {
        // read only controls don't have the caret
        caret = (wxCaret *)NULL;
    }

    // SetCaret() will delete the old caret if any
    SetCaret(caret);
}

void wxTextCtrl::ShowCaret(bool show)
{
    wxCaret *caret = GetCaret();
    if ( caret )
    {
        // (re)position caret correctly
        caret->Move(GetCaretPosition());

        // and show it there
        if ((show && !caret->IsVisible()) ||
            (!show && caret->IsVisible()))
            caret->Show(show);
    }
}

// ----------------------------------------------------------------------------
// vertical scrolling (multiline only)
// ----------------------------------------------------------------------------

size_t wxTextCtrl::GetLinesPerPage() const
{
    if ( IsSingleLine() )
        return 1;

    return GetRealTextArea().height / GetLineHeight();
}

wxTextPos wxTextCtrl::GetPositionAbove()
{
    wxCHECK_MSG( !IsSingleLine(), INVALID_POS_VALUE,
                 _T("can't move cursor vertically in a single line control") );

    // move the cursor up by one ROW not by one LINE: this means that
    // we should really use HitTest() and not just go to the same
    // position in the previous line
    wxPoint pt = GetCaretPosition() - m_rectText.GetPosition();
    if ( MData().m_xCaret == -1 )
    {
        // remember the initial cursor abscissa
        MData().m_xCaret = pt.x;
    }
    else
    {
        // use the remembered abscissa
        pt.x = MData().m_xCaret;
    }

    CalcUnscrolledPosition(pt.x, pt.y, &pt.x, &pt.y);
    pt.y -= GetLineHeight();

    wxTextCoord col, row;
    if ( HitTestLogical(pt, &col, &row) == wxTE_HT_BEFORE )
    {
        // can't move further
        return INVALID_POS_VALUE;
    }

    return XYToPosition(col, row);
}

wxTextPos wxTextCtrl::GetPositionBelow()
{
    wxCHECK_MSG( !IsSingleLine(), INVALID_POS_VALUE,
                 _T("can't move cursor vertically in a single line control") );

    // see comments for wxACTION_TEXT_UP
    wxPoint pt = GetCaretPosition() - m_rectText.GetPosition();
    if ( MData().m_xCaret == -1 )
    {
        // remember the initial cursor abscissa
        MData().m_xCaret = pt.x;
    }
    else
    {
        // use the remembered abscissa
        pt.x = MData().m_xCaret;
    }

    CalcUnscrolledPosition(pt.x, pt.y, &pt.x, &pt.y);
    pt.y += GetLineHeight();

    wxTextCoord col, row;
    if ( HitTestLogical(pt, &col, &row) == wxTE_HT_BELOW )
    {
        // can't go further down
        return INVALID_POS_VALUE;
    }

    // note that wxTE_HT_BEYOND is ok: it happens when we go down
    // from a longer line to a shorter one, for example (OTOH
    // wxTE_HT_BEFORE can never happen)
    return XYToPosition(col, row);
}

// ----------------------------------------------------------------------------
// input
// ----------------------------------------------------------------------------

bool wxTextCtrl::PerformAction(const wxControlAction& actionOrig,
                               long numArg,
                               const wxString& strArg)
{
    // has the text changed as result of this action?
    bool textChanged = false;

    // the remembered cursor abscissa for multiline text controls is usually
    // reset after each user action but for ones which do use it (UP and DOWN
    // for example) we shouldn't do it - as indicated by this flag
    bool rememberAbscissa = false;

    // the command this action corresponds to or NULL if this action doesn't
    // change text at all or can't be undone
    wxTextCtrlCommand *command = (wxTextCtrlCommand *)NULL;

    wxString action;
    bool del = false,
         sel = false;
    if ( actionOrig.StartsWith(wxACTION_TEXT_PREFIX_DEL, &action) )
    {
        if ( IsEditable() )
            del = true;
    }
    else if ( actionOrig.StartsWith(wxACTION_TEXT_PREFIX_SEL, &action) )
    {
        sel = true;
    }
    else // not selection nor delete action
    {
        action = actionOrig;
    }

    // set newPos to -2 as it can't become equal to it in the assignments below
    // (but it can become -1)
    wxTextPos newPos = INVALID_POS_VALUE;

    if ( action == wxACTION_TEXT_HOME )
    {
        newPos = m_curPos - m_curCol;
    }
    else if ( action == wxACTION_TEXT_END )
    {
        newPos = m_curPos + GetLineLength(m_curRow) - m_curCol;
    }
    else if ( (action == wxACTION_TEXT_GOTO) ||
              (action == wxACTION_TEXT_FIRST) ||
              (action == wxACTION_TEXT_LAST) )
    {
        if ( action == wxACTION_TEXT_FIRST )
            numArg = 0;
        else if ( action == wxACTION_TEXT_LAST )
            numArg = GetLastPosition();
        //else: numArg already contains the position

        newPos = numArg;
    }
    else if ( action == wxACTION_TEXT_UP )
    {
        if ( !IsSingleLine() )
        {
            newPos = GetPositionAbove();

            if ( newPos != INVALID_POS_VALUE )
            {
                // remember where the cursor original had been
                rememberAbscissa = true;
            }
        }
    }
    else if ( action == wxACTION_TEXT_DOWN )
    {
        if ( !IsSingleLine() )
        {
            newPos = GetPositionBelow();

            if ( newPos != INVALID_POS_VALUE )
            {
                // remember where the cursor original had been
                rememberAbscissa = true;
            }
        }
    }
    else if ( action == wxACTION_TEXT_LEFT )
    {
        newPos = m_curPos - 1;
    }
    else if ( action == wxACTION_TEXT_WORD_LEFT )
    {
        newPos = GetWordStart();
    }
    else if ( action == wxACTION_TEXT_RIGHT )
    {
        newPos = m_curPos + 1;
    }
    else if ( action == wxACTION_TEXT_WORD_RIGHT )
    {
        newPos = GetWordEnd();
    }
    else if ( action == wxACTION_TEXT_INSERT )
    {
        if ( IsEditable() && !strArg.empty() )
        {
            // inserting text can be undone
            command = new wxTextCtrlInsertCommand(strArg);

            textChanged = true;
        }
    }
    else if ( (action == wxACTION_TEXT_PAGE_UP) ||
              (action == wxACTION_TEXT_PAGE_DOWN) )
    {
        if ( !IsSingleLine() )
        {
            size_t count = GetLinesPerPage();
            if ( count > PAGE_OVERLAP_IN_LINES )
            {
                // pages should overlap slightly to allow the reader to keep
                // orientation in the text
                count -= PAGE_OVERLAP_IN_LINES;
            }

            // remember where the cursor original had been
            rememberAbscissa = true;

            bool goUp = action == wxACTION_TEXT_PAGE_UP;
            for ( size_t line = 0; line < count; line++ )
            {
                wxTextPos pos = goUp ? GetPositionAbove() : GetPositionBelow();
                if ( pos == INVALID_POS_VALUE )
                {
                    // can't move further
                    break;
                }

                MoveInsertionPoint(pos);
                newPos = pos;
            }

            // we implement the Unix scrolling model here: cursor will always
            // be on the first line after Page Down and on the last one after
            // Page Up
            //
            // Windows programs usually keep the cursor line offset constant
            // but do we really need it?
            wxCoord y;
            if ( goUp )
            {
                // find the line such that when it is the first one, the
                // current position is in the last line
                wxTextPos pos = 0;
                for ( size_t line = 0; line < count; line++ )
                {
                    pos = GetPositionAbove();
                    if ( pos == INVALID_POS_VALUE )
                        break;

                    MoveInsertionPoint(pos);
                }

                MoveInsertionPoint(newPos);

                PositionToLogicalXY(pos, NULL, &y);
            }
            else // scrolled down
            {
                PositionToLogicalXY(newPos, NULL, &y);
            }

            // scroll vertically only
            Scroll(wxDefaultCoord, y);
        }
    }
    else if ( action == wxACTION_TEXT_SEL_WORD )
    {
        SetSelection(GetWordStart(), GetWordEnd());
    }
    else if ( action == wxACTION_TEXT_ANCHOR_SEL )
    {
        newPos = numArg;
    }
    else if ( action == wxACTION_TEXT_EXTEND_SEL )
    {
        SetSelection(m_selAnchor, numArg);
    }
    else if ( action == wxACTION_TEXT_COPY )
    {
        Copy();
    }
    else if ( action == wxACTION_TEXT_CUT )
    {
        if ( IsEditable() )
            Cut();
    }
    else if ( action == wxACTION_TEXT_PASTE )
    {
        if ( IsEditable() )
            Paste();
    }
    else if ( action == wxACTION_TEXT_UNDO )
    {
        if ( CanUndo() )
            Undo();
    }
    else if ( action == wxACTION_TEXT_REDO )
    {
        if ( CanRedo() )
            Redo();
    }
    else
    {
        return wxControl::PerformAction(action, numArg, strArg);
    }

    if ( newPos != INVALID_POS_VALUE )
    {
        // bring the new position into the range
        if ( newPos < 0 )
            newPos = 0;

        wxTextPos posLast = GetLastPosition();
        if ( newPos > posLast )
            newPos = posLast;

        if ( del )
        {
            // if we have the selection, remove just it
            wxTextPos from, to;
            if ( HasSelection() )
            {
                from = m_selStart;
                to = m_selEnd;
            }
            else
            {
                // otherwise delete everything between current position and
                // the new one
                if ( m_curPos != newPos )
                {
                    from = m_curPos;
                    to = newPos;
                }
                else // nothing to delete
                {
                    // prevent test below from working
                    from = INVALID_POS_VALUE;

                    // and this is just to silent the compiler warning
                    to = 0;
                }
            }

            if ( from != INVALID_POS_VALUE )
            {
                command = new wxTextCtrlRemoveCommand(from, to);
            }
        }
        else // cursor movement command
        {
            // just go there
            DoSetInsertionPoint(newPos);

            if ( sel )
            {
                SetSelection(m_selAnchor, m_curPos);
            }
            else // simple movement
            {
                // clear the existing selection
                ClearSelection();
            }
        }

        if ( !rememberAbscissa && !IsSingleLine() )
        {
            MData().m_xCaret = -1;
        }
    }

    if ( command )
    {
        // execute and remember it to be able to undo it later
        m_cmdProcessor->Submit(command);

        // undoable commands always change text
        textChanged = true;
    }
    else // no undoable command
    {
        // m_cmdProcessor->StopCompressing()
    }

    if ( textChanged )
    {
        wxASSERT_MSG( IsEditable(), _T("non editable control changed?") );

        wxCommandEvent event(wxEVT_COMMAND_TEXT_UPDATED, GetId());
        InitCommandEvent(event);
        GetEventHandler()->ProcessEvent(event);

        // as the text changed...
        m_isModified = true;
    }

    return true;
}

void wxTextCtrl::OnChar(wxKeyEvent& event)
{
    // only process the key events from "simple keys" here
    if ( !event.HasModifiers() )
    {
        int keycode = event.GetKeyCode();
#if wxUSE_UNICODE
        wxChar unicode = event.GetUnicodeKey();
#endif
        if ( keycode == WXK_RETURN )
        {
            if ( IsSingleLine() || (GetWindowStyle() & wxTE_PROCESS_ENTER) )
            {
                wxCommandEvent event(wxEVT_COMMAND_TEXT_ENTER, GetId());
                InitCommandEvent(event);
                event.SetString(GetValue());
                GetEventHandler()->ProcessEvent(event);
            }
            else // interpret <Enter> normally: insert new line
            {
                PerformAction(wxACTION_TEXT_INSERT, -1, _T('\n'));
            }
        }
        else if ( keycode < 255 && isprint(keycode) )
        {
            PerformAction(wxACTION_TEXT_INSERT, -1, (wxChar)keycode);

            // skip event.Skip() below
            return;
        }
#if wxUSE_UNICODE
        else if (unicode > 0)
        {
            PerformAction(wxACTION_TEXT_INSERT, -1, unicode);

            return;
        }
#endif
    }
#ifdef __WXDEBUG__
    // Ctrl-R refreshes the control in debug mode
    else if ( event.ControlDown() && event.GetKeyCode() == 'r' )
        Refresh();
#endif // __WXDEBUG__

    event.Skip();
}

/* static */
wxInputHandler *wxTextCtrl::GetStdInputHandler(wxInputHandler *handlerDef)
{
    static wxStdTextCtrlInputHandler s_handler(handlerDef);

    return &s_handler;
}

// ----------------------------------------------------------------------------
// wxStdTextCtrlInputHandler
// ----------------------------------------------------------------------------

wxStdTextCtrlInputHandler::wxStdTextCtrlInputHandler(wxInputHandler *inphand)
                         : wxStdInputHandler(inphand)
{
    m_winCapture = (wxTextCtrl *)NULL;
}

/* static */
wxTextPos wxStdTextCtrlInputHandler::HitTest(const wxTextCtrl *text,
                                             const wxPoint& pt)
{
    wxTextCoord col, row;
    wxTextCtrlHitTestResult ht = text->HitTest(pt, &col, &row);

    wxTextPos pos = text->XYToPosition(col, row);

    // if the point is after the last column we must adjust the position to be
    // the last position in the line (unless it is already the last)
    if ( (ht == wxTE_HT_BEYOND) && (pos < text->GetLastPosition()) )
    {
        pos++;
    }

    return pos;
}

bool wxStdTextCtrlInputHandler::HandleKey(wxInputConsumer *consumer,
                                          const wxKeyEvent& event,
                                          bool pressed)
{
    // we're only interested in key presses
    if ( !pressed )
        return false;

    int keycode = event.GetKeyCode();

    wxControlAction action;
    wxString str;
    bool ctrlDown = event.ControlDown(),
         shiftDown = event.ShiftDown();
    if ( shiftDown )
    {
        action = wxACTION_TEXT_PREFIX_SEL;
    }

    // the only key combination with Alt we recognize is Alt-Bksp for undo, so
    // treat it first separately
    if ( event.AltDown() )
    {
        if ( keycode == WXK_BACK && !ctrlDown && !shiftDown )
            action = wxACTION_TEXT_UNDO;
    }
    else switch ( keycode )
    {
        // cursor movement
        case WXK_HOME:
            action << (ctrlDown ? wxACTION_TEXT_FIRST
                                : wxACTION_TEXT_HOME);
            break;

        case WXK_END:
            action << (ctrlDown ? wxACTION_TEXT_LAST
                                : wxACTION_TEXT_END);
            break;

        case WXK_UP:
            if ( !ctrlDown )
                action << wxACTION_TEXT_UP;
            break;

        case WXK_DOWN:
            if ( !ctrlDown )
                action << wxACTION_TEXT_DOWN;
            break;

        case WXK_LEFT:
            action << (ctrlDown ? wxACTION_TEXT_WORD_LEFT
                                : wxACTION_TEXT_LEFT);
            break;

        case WXK_RIGHT:
            action << (ctrlDown ? wxACTION_TEXT_WORD_RIGHT
                                : wxACTION_TEXT_RIGHT);
            break;

        case WXK_PAGEDOWN:
            // we don't map Ctrl-PgUp/Dn to anything special - what should it
            // to? for now, it's the same as without control
            action << wxACTION_TEXT_PAGE_DOWN;
            break;

        case WXK_PAGEUP:
            action << wxACTION_TEXT_PAGE_UP;
            break;

        // delete
        case WXK_DELETE:
            if ( !ctrlDown )
                action << wxACTION_TEXT_PREFIX_DEL << wxACTION_TEXT_RIGHT;
            break;

        case WXK_BACK:
            if ( !ctrlDown )
                action << wxACTION_TEXT_PREFIX_DEL << wxACTION_TEXT_LEFT;
            break;

        // something else
        default:
            // reset the action as it could be already set to one of the
            // prefixes
            action = wxACTION_NONE;

            if ( ctrlDown )
            {
                switch ( keycode )
                {
                    case 'A':
                        action = wxACTION_TEXT_REDO;
                        break;

                    case 'C':
                        action = wxACTION_TEXT_COPY;
                        break;

                    case 'V':
                        action = wxACTION_TEXT_PASTE;
                        break;

                    case 'X':
                        action = wxACTION_TEXT_CUT;
                        break;

                    case 'Z':
                        action = wxACTION_TEXT_UNDO;
                        break;
                }
            }
    }

    if ( (action != wxACTION_NONE) && (action != wxACTION_TEXT_PREFIX_SEL) )
    {
        consumer->PerformAction(action, -1, str);

        return true;
    }

    return wxStdInputHandler::HandleKey(consumer, event, pressed);
}

bool wxStdTextCtrlInputHandler::HandleMouse(wxInputConsumer *consumer,
                                            const wxMouseEvent& event)
{
    if ( event.LeftDown() )
    {
        wxASSERT_MSG( !m_winCapture, _T("left button going down twice?") );

        wxTextCtrl *text = wxStaticCast(consumer->GetInputWindow(), wxTextCtrl);

        m_winCapture = text;
        m_winCapture->CaptureMouse();

        text->HideCaret();

        wxTextPos pos = HitTest(text, event.GetPosition());
        if ( pos != -1 )
        {
            text->PerformAction(wxACTION_TEXT_ANCHOR_SEL, pos);
        }
    }
    else if ( event.LeftDClick() )
    {
        // select the word the cursor is on
        consumer->PerformAction(wxACTION_TEXT_SEL_WORD);
    }
    else if ( event.LeftUp() )
    {
        if ( m_winCapture )
        {
            m_winCapture->ShowCaret();

            m_winCapture->ReleaseMouse();
            m_winCapture = (wxTextCtrl *)NULL;
        }
    }

    return wxStdInputHandler::HandleMouse(consumer, event);
}

bool wxStdTextCtrlInputHandler::HandleMouseMove(wxInputConsumer *consumer,
                                                const wxMouseEvent& event)
{
    if ( m_winCapture )
    {
        // track it
        wxTextCtrl *text = wxStaticCast(m_winCapture, wxTextCtrl);
        wxTextPos pos = HitTest(text, event.GetPosition());
        if ( pos != -1 )
        {
            text->PerformAction(wxACTION_TEXT_EXTEND_SEL, pos);
        }
    }

    return wxStdInputHandler::HandleMouseMove(consumer, event);
}

bool
wxStdTextCtrlInputHandler::HandleFocus(wxInputConsumer *consumer,
                                       const wxFocusEvent& event)
{
    wxTextCtrl *text = wxStaticCast(consumer->GetInputWindow(), wxTextCtrl);

    // the selection appearance changes depending on whether we have the focus
    text->RefreshSelection();

    if (event.GetEventType() == wxEVT_SET_FOCUS)
    {
        if (text->GetCaret() && !text->GetCaret()->IsVisible())
            text->ShowCaret();
    }
    else
    {
        if (text->GetCaret() && text->GetCaret()->IsVisible())
            text->HideCaret();
    }

    // never refresh entirely
    return false;
}

#endif // wxUSE_TEXTCTRL
