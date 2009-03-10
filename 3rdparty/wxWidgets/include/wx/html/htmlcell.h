/////////////////////////////////////////////////////////////////////////////
// Name:        htmlcell.h
// Purpose:     wxHtmlCell class is used by wxHtmlWindow/wxHtmlWinParser
//              as a basic visual element of HTML page
// Author:      Vaclav Slavik
// RCS-ID:      $Id: htmlcell.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) 1999-2003 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HTMLCELL_H_
#define _WX_HTMLCELL_H_

#include "wx/defs.h"

#if wxUSE_HTML

#include "wx/html/htmltag.h"
#include "wx/html/htmldefs.h"
#include "wx/window.h"


class WXDLLIMPEXP_FWD_HTML wxHtmlWindowInterface;
class WXDLLIMPEXP_FWD_HTML wxHtmlLinkInfo;
class WXDLLIMPEXP_FWD_HTML wxHtmlCell;
class WXDLLIMPEXP_FWD_HTML wxHtmlContainerCell;


// wxHtmlSelection is data holder with information about text selection.
// Selection is defined by two positions (beginning and end of the selection)
// and two leaf(!) cells at these positions.
class WXDLLIMPEXP_HTML wxHtmlSelection
{
public:
    wxHtmlSelection()
        : m_fromPos(wxDefaultPosition), m_toPos(wxDefaultPosition),
          m_fromPrivPos(wxDefaultPosition), m_toPrivPos(wxDefaultPosition),
          m_fromCell(NULL), m_toCell(NULL) {}

    void Set(const wxPoint& fromPos, const wxHtmlCell *fromCell,
             const wxPoint& toPos, const wxHtmlCell *toCell);
    void Set(const wxHtmlCell *fromCell, const wxHtmlCell *toCell);

    const wxHtmlCell *GetFromCell() const { return m_fromCell; }
    const wxHtmlCell *GetToCell() const { return m_toCell; }

    // these values are in absolute coordinates:
    const wxPoint& GetFromPos() const { return m_fromPos; }
    const wxPoint& GetToPos() const { return m_toPos; }

    // these are From/ToCell's private data
    const wxPoint& GetFromPrivPos() const { return m_fromPrivPos; }
    const wxPoint& GetToPrivPos() const { return m_toPrivPos; }
    void SetFromPrivPos(const wxPoint& pos) { m_fromPrivPos = pos; }
    void SetToPrivPos(const wxPoint& pos) { m_toPrivPos = pos; }
    void ClearPrivPos() { m_toPrivPos = m_fromPrivPos = wxDefaultPosition; }

    bool IsEmpty() const
        { return m_fromPos == wxDefaultPosition &&
                 m_toPos == wxDefaultPosition; }

private:
    wxPoint m_fromPos, m_toPos;
    wxPoint m_fromPrivPos, m_toPrivPos;
    const wxHtmlCell *m_fromCell, *m_toCell;
};



enum wxHtmlSelectionState
{
    wxHTML_SEL_OUT,     // currently rendered cell is outside the selection
    wxHTML_SEL_IN,      // ... is inside selection
    wxHTML_SEL_CHANGING // ... is the cell on which selection state changes
};

// Selection state is passed to wxHtmlCell::Draw so that it can render itself
// differently e.g. when inside text selection or outside it.
class WXDLLIMPEXP_HTML wxHtmlRenderingState
{
public:
    wxHtmlRenderingState() : m_selState(wxHTML_SEL_OUT) {}

    void SetSelectionState(wxHtmlSelectionState s) { m_selState = s; }
    wxHtmlSelectionState GetSelectionState() const { return m_selState; }

    void SetFgColour(const wxColour& c) { m_fgColour = c; }
    const wxColour& GetFgColour() const { return m_fgColour; }
    void SetBgColour(const wxColour& c) { m_bgColour = c; }
    const wxColour& GetBgColour() const { return m_bgColour; }

private:
    wxHtmlSelectionState  m_selState;
    wxColour              m_fgColour, m_bgColour;
};


// HTML rendering customization. This class is used when rendering wxHtmlCells
// as a callback:
class WXDLLIMPEXP_HTML wxHtmlRenderingStyle
{
public:
    virtual ~wxHtmlRenderingStyle() {}
    virtual wxColour GetSelectedTextColour(const wxColour& clr) = 0;
    virtual wxColour GetSelectedTextBgColour(const wxColour& clr) = 0;
};

// Standard style:
class WXDLLIMPEXP_HTML wxDefaultHtmlRenderingStyle : public wxHtmlRenderingStyle
{
public:
    virtual wxColour GetSelectedTextColour(const wxColour& clr);
    virtual wxColour GetSelectedTextBgColour(const wxColour& clr);
};


// Information given to cells when drawing them. Contains rendering state,
// selection information and rendering style object that can be used to
// customize the output.
class WXDLLIMPEXP_HTML wxHtmlRenderingInfo
{
public:
    wxHtmlRenderingInfo() : m_selection(NULL), m_style(NULL) {}

    void SetSelection(wxHtmlSelection *s) { m_selection = s; }
    wxHtmlSelection *GetSelection() const { return m_selection; }

    void SetStyle(wxHtmlRenderingStyle *style) { m_style = style; }
    wxHtmlRenderingStyle& GetStyle() { return *m_style; }

    wxHtmlRenderingState& GetState() { return m_state; }

protected:
    wxHtmlSelection      *m_selection;
    wxHtmlRenderingStyle *m_style;
    wxHtmlRenderingState m_state;
};


// Flags for wxHtmlCell::FindCellByPos
enum
{
    wxHTML_FIND_EXACT             = 1,
    wxHTML_FIND_NEAREST_BEFORE    = 2,
    wxHTML_FIND_NEAREST_AFTER     = 4
};


// Superscript/subscript/normal script mode of a cell
enum wxHtmlScriptMode
{
    wxHTML_SCRIPT_NORMAL,
    wxHTML_SCRIPT_SUB,
    wxHTML_SCRIPT_SUP
};


// ---------------------------------------------------------------------------
// wxHtmlCell
//                  Internal data structure. It represents fragments of parsed
//                  HTML page - a word, picture, table, horizontal line and so
//                  on.  It is used by wxHtmlWindow to represent HTML page in
//                  memory.
// ---------------------------------------------------------------------------


class WXDLLIMPEXP_HTML wxHtmlCell : public wxObject
{
public:
    wxHtmlCell();
    virtual ~wxHtmlCell();

    void SetParent(wxHtmlContainerCell *p) {m_Parent = p;}
    wxHtmlContainerCell *GetParent() const {return m_Parent;}

    int GetPosX() const {return m_PosX;}
    int GetPosY() const {return m_PosY;}
    int GetWidth() const {return m_Width;}

    // Returns the maximum possible length of the cell.
    // Call Layout at least once before using GetMaxTotalWidth()
    virtual int GetMaxTotalWidth() const { return m_Width; }

    int GetHeight() const {return m_Height;}
    int GetDescent() const {return m_Descent;}

    void SetScriptMode(wxHtmlScriptMode mode, long previousBase);
    wxHtmlScriptMode GetScriptMode() const { return m_ScriptMode; }
    long GetScriptBaseline() { return m_ScriptBaseline; }

    // Formatting cells are not visible on the screen, they only alter
    // renderer's state.
    bool IsFormattingCell() const { return m_Width == 0 && m_Height == 0; }

    const wxString& GetId() const { return m_id; }
    void SetId(const wxString& id) { m_id = id; }

    // returns the link associated with this cell. The position is position
    // within the cell so it varies from 0 to m_Width, from 0 to m_Height
    virtual wxHtmlLinkInfo* GetLink(int WXUNUSED(x) = 0,
                                    int WXUNUSED(y) = 0) const
        { return m_Link; }

    // Returns cursor to be used when mouse is over the cell:
    virtual wxCursor GetMouseCursor(wxHtmlWindowInterface *window) const;

#if WXWIN_COMPATIBILITY_2_6
    // this was replaced by GetMouseCursor, don't use in new code!
    virtual wxCursor GetCursor() const;
#endif

    // return next cell among parent's cells
    wxHtmlCell *GetNext() const {return m_Next;}
    // returns first child cell (if there are any, i.e. if this is container):
    virtual wxHtmlCell* GetFirstChild() const { return NULL; }

    // members writing methods
    virtual void SetPos(int x, int y) {m_PosX = x, m_PosY = y;}
    void SetLink(const wxHtmlLinkInfo& link);
    void SetNext(wxHtmlCell *cell) {m_Next = cell;}

    // 1. adjust cell's width according to the fact that maximal possible width
    //    is w.  (this has sense when working with horizontal lines, tables
    //    etc.)
    // 2. prepare layout (=fill-in m_PosX, m_PosY (and sometime m_Height)
    //    members) = place items to fit window, according to the width w
    virtual void Layout(int w);

    // renders the cell
    virtual void Draw(wxDC& WXUNUSED(dc),
                      int WXUNUSED(x), int WXUNUSED(y),
                      int WXUNUSED(view_y1), int WXUNUSED(view_y2),
                      wxHtmlRenderingInfo& WXUNUSED(info)) {}

    // proceed drawing actions in case the cell is not visible (scrolled out of
    // screen).  This is needed to change fonts, colors and so on.
    virtual void DrawInvisible(wxDC& WXUNUSED(dc),
                               int WXUNUSED(x), int WXUNUSED(y),
                               wxHtmlRenderingInfo& WXUNUSED(info)) {}

    // This method returns pointer to the FIRST cell for that
    // the condition
    // is true. It first checks if the condition is true for this
    // cell and then calls m_Next->Find(). (Note: it checks
    // all subcells if the cell is container)
    // Condition is unique condition identifier (see htmldefs.h)
    // (user-defined condition IDs should start from 10000)
    // and param is optional parameter
    // Example : m_Cell->Find(wxHTML_COND_ISANCHOR, "news");
    //   returns pointer to anchor news
    virtual const wxHtmlCell* Find(int condition, const void* param) const;


    // This function is called when mouse button is clicked over the cell.
    // Returns true if a link is clicked, false otherwise.
    //
    // 'window' is pointer to wxHtmlWindowInterface of the window which
    // generated the event.
    // HINT: if this handling is not enough for you you should use
    //       wxHtmlWidgetCell
    virtual bool ProcessMouseClick(wxHtmlWindowInterface *window,
                                   const wxPoint& pos,
                                   const wxMouseEvent& event);

#if WXWIN_COMPATIBILITY_2_6
    // this was replaced by ProcessMouseClick, don't use in new code!
    virtual void OnMouseClick(wxWindow *window,
                              int x, int y, const wxMouseEvent& event);
#endif

    // This method used to adjust pagebreak position. The parameter is variable
    // that contains y-coordinate of page break (= horizontal line that should
    // not be crossed by words, images etc.). If this cell cannot be divided
    // into two pieces (each one on another page) then it moves the pagebreak
    // few pixels up.
    //
    // Returned value : true if pagebreak was modified, false otherwise
    // Usage : while (container->AdjustPagebreak(&p)) {}
    virtual bool AdjustPagebreak(int *pagebreak,
                                 wxArrayInt& known_pagebreaks) const;

    // Sets cell's behaviour on pagebreaks (see AdjustPagebreak). Default
    // is true - the cell can be split on two pages
    void SetCanLiveOnPagebreak(bool can) { m_CanLiveOnPagebreak = can; }

    // Can the line be broken before this cell?
    virtual bool IsLinebreakAllowed() const
        { return !IsFormattingCell(); }

    // Returns true for simple == terminal cells, i.e. not composite ones.
    // This if for internal usage only and may disappear in future versions!
    virtual bool IsTerminalCell() const { return true; }

    // Find a cell inside this cell positioned at the given coordinates
    // (relative to this's positions). Returns NULL if no such cell exists.
    // The flag can be used to specify whether to look for terminal or
    // nonterminal cells or both. In either case, returned cell is deepest
    // cell in cells tree that contains [x,y].
    virtual wxHtmlCell *FindCellByPos(wxCoord x, wxCoord y,
                                  unsigned flags = wxHTML_FIND_EXACT) const;

    // Returns absolute position of the cell on HTML canvas.
    // If rootCell is provided, then it's considered to be the root of the
    // hierarchy and the returned value is relative to it.
    wxPoint GetAbsPos(wxHtmlCell *rootCell = NULL) const;

    // Returns root cell of the hierarchy (i.e. grand-grand-...-parent that
    // doesn't have a parent itself)
    wxHtmlCell *GetRootCell() const;

    // Returns first (last) terminal cell inside this cell. It may return NULL,
    // but it is rare -- only if there are no terminals in the tree.
    virtual wxHtmlCell *GetFirstTerminal() const
        { return wxConstCast(this, wxHtmlCell); }
    virtual wxHtmlCell *GetLastTerminal() const
        { return wxConstCast(this, wxHtmlCell); }

    // Returns cell's depth, i.e. how far under the root cell it is
    // (if it is the root, depth is 0)
    unsigned GetDepth() const;

    // Returns true if the cell appears before 'cell' in natural order of
    // cells (= as they are read). If cell A is (grand)parent of cell B,
    // then both A.IsBefore(B) and B.IsBefore(A) always return true.
    bool IsBefore(wxHtmlCell *cell) const;

    // Converts the cell into text representation. If sel != NULL then
    // only part of the cell inside the selection is converted.
    virtual wxString ConvertToText(wxHtmlSelection *WXUNUSED(sel)) const
        { return wxEmptyString; }

protected:
    // pointer to the next cell
    wxHtmlCell *m_Next;
    // pointer to parent cell
    wxHtmlContainerCell *m_Parent;

    // dimensions of fragment (m_Descent is used to position text & images)
    long m_Width, m_Height, m_Descent;
    // position where the fragment is drawn:
    long m_PosX, m_PosY;

    // superscript/subscript/normal:
    wxHtmlScriptMode m_ScriptMode;
    long m_ScriptBaseline;

    // destination address if this fragment is hypertext link, NULL otherwise
    wxHtmlLinkInfo *m_Link;
    // true if this cell can be placed on pagebreak, false otherwise
    bool m_CanLiveOnPagebreak;
    // unique identifier of the cell, generated from "id" property of tags
    wxString m_id;

    DECLARE_ABSTRACT_CLASS(wxHtmlCell)
    DECLARE_NO_COPY_CLASS(wxHtmlCell)
};




// ----------------------------------------------------------------------------
// Inherited cells:
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// wxHtmlWordCell
//                  Single word in input stream.
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_HTML wxHtmlWordCell : public wxHtmlCell
{
public:
    wxHtmlWordCell(const wxString& word, const wxDC& dc);
    void Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
              wxHtmlRenderingInfo& info);
    virtual wxCursor GetMouseCursor(wxHtmlWindowInterface *window) const;
    wxString ConvertToText(wxHtmlSelection *sel) const;
    bool IsLinebreakAllowed() const { return m_allowLinebreak; }

    void SetPreviousWord(wxHtmlWordCell *cell);

protected:
    void SetSelectionPrivPos(const wxDC& dc, wxHtmlSelection *s) const;
    void Split(const wxDC& dc,
               const wxPoint& selFrom, const wxPoint& selTo,
               unsigned& pos1, unsigned& pos2) const;

    wxString m_Word;
    bool     m_allowLinebreak;

    DECLARE_ABSTRACT_CLASS(wxHtmlWordCell)
    DECLARE_NO_COPY_CLASS(wxHtmlWordCell)
};





// Container contains other cells, thus forming tree structure of rendering
// elements. Basic code of layout algorithm is contained in this class.
class WXDLLIMPEXP_HTML wxHtmlContainerCell : public wxHtmlCell
{
public:
    wxHtmlContainerCell(wxHtmlContainerCell *parent);
    virtual ~wxHtmlContainerCell();

    virtual void Layout(int w);
    virtual void Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
                      wxHtmlRenderingInfo& info);
    virtual void DrawInvisible(wxDC& dc, int x, int y,
                               wxHtmlRenderingInfo& info);
/*    virtual bool AdjustPagebreak(int *pagebreak, int *known_pagebreaks = NULL, int number_of_pages = 0) const;*/
    virtual bool AdjustPagebreak(int *pagebreak, wxArrayInt& known_pagebreaks) const;

    // insert cell at the end of m_Cells list
    void InsertCell(wxHtmlCell *cell);

    // sets horizontal/vertical alignment
    void SetAlignHor(int al) {m_AlignHor = al; m_LastLayout = -1;}
    int GetAlignHor() const {return m_AlignHor;}
    void SetAlignVer(int al) {m_AlignVer = al; m_LastLayout = -1;}
    int GetAlignVer() const {return m_AlignVer;}

    // sets left-border indentation. units is one of wxHTML_UNITS_* constants
    // what is combination of wxHTML_INDENT_*
    void SetIndent(int i, int what, int units = wxHTML_UNITS_PIXELS);
    // returns the indentation. ind is one of wxHTML_INDENT_* constants
    int GetIndent(int ind) const;
    // returns type of value returned by GetIndent(ind)
    int GetIndentUnits(int ind) const;

    // sets alignment info based on given tag's params
    void SetAlign(const wxHtmlTag& tag);
    // sets floating width adjustment
    // (examples : 32 percent of parent container,
    // -15 pixels percent (this means 100 % - 15 pixels)
    void SetWidthFloat(int w, int units) {m_WidthFloat = w; m_WidthFloatUnits = units; m_LastLayout = -1;}
    void SetWidthFloat(const wxHtmlTag& tag, double pixel_scale = 1.0);
    // sets minimal height of this container.
    void SetMinHeight(int h, int align = wxHTML_ALIGN_TOP) {m_MinHeight = h; m_MinHeightAlign = align; m_LastLayout = -1;}

    void SetBackgroundColour(const wxColour& clr) {m_UseBkColour = true; m_BkColour = clr;}
    // returns background colour (of wxNullColour if none set), so that widgets can
    // adapt to it:
    wxColour GetBackgroundColour();
    void SetBorder(const wxColour& clr1, const wxColour& clr2) {m_UseBorder = true; m_BorderColour1 = clr1, m_BorderColour2 = clr2;}
    virtual wxHtmlLinkInfo* GetLink(int x = 0, int y = 0) const;
    virtual const wxHtmlCell* Find(int condition, const void* param) const;

#if WXWIN_COMPATIBILITY_2_6
    // this was replaced by ProcessMouseClick, don't use in new code!
    virtual void OnMouseClick(wxWindow *window,
                              int x, int y, const wxMouseEvent& event);
#endif
    virtual bool ProcessMouseClick(wxHtmlWindowInterface *window,
                                   const wxPoint& pos,
                                   const wxMouseEvent& event);

    virtual wxHtmlCell* GetFirstChild() const { return m_Cells; }
#if WXWIN_COMPATIBILITY_2_4
    wxDEPRECATED( wxHtmlCell* GetFirstCell() const );
#endif
    // returns last child cell:
    wxHtmlCell* GetLastChild() const { return m_LastCell; }

    // see comment in wxHtmlCell about this method
    virtual bool IsTerminalCell() const { return false; }

    virtual wxHtmlCell *FindCellByPos(wxCoord x, wxCoord y,
                                  unsigned flags = wxHTML_FIND_EXACT) const;

    virtual wxHtmlCell *GetFirstTerminal() const;
    virtual wxHtmlCell *GetLastTerminal() const;


    // Removes indentation on top or bottom of the container (i.e. above or
    // below first/last terminal cell). For internal use only.
    virtual void RemoveExtraSpacing(bool top, bool bottom);

    // Returns the maximum possible length of the container.
    // Call Layout at least once before using GetMaxTotalWidth()
    virtual int GetMaxTotalWidth() const { return m_MaxTotalWidth; }

protected:
    void UpdateRenderingStatePre(wxHtmlRenderingInfo& info,
                                 wxHtmlCell *cell) const;
    void UpdateRenderingStatePost(wxHtmlRenderingInfo& info,
                                  wxHtmlCell *cell) const;

protected:
    int m_IndentLeft, m_IndentRight, m_IndentTop, m_IndentBottom;
            // indentation of subcells. There is always m_Indent pixels
            // big space between given border of the container and the subcells
            // it m_Indent < 0 it is in PERCENTS, otherwise it is in pixels
    int m_MinHeight, m_MinHeightAlign;
        // minimal height.
    wxHtmlCell *m_Cells, *m_LastCell;
            // internal cells, m_Cells points to the first of them, m_LastCell to the last one.
            // (LastCell is needed only to speed-up InsertCell)
    int m_AlignHor, m_AlignVer;
            // alignment horizontal and vertical (left, center, right)
    int m_WidthFloat, m_WidthFloatUnits;
            // width float is used in adjustWidth
    bool m_UseBkColour;
    wxColour m_BkColour;
            // background color of this container
    bool m_UseBorder;
    wxColour m_BorderColour1, m_BorderColour2;
            // borders color of this container
    int m_LastLayout;
            // if != -1 then call to Layout may be no-op
            // if previous call to Layout has same argument
    int m_MaxTotalWidth;
            // Maximum possible length if ignoring line wrap


    DECLARE_ABSTRACT_CLASS(wxHtmlContainerCell)
    DECLARE_NO_COPY_CLASS(wxHtmlContainerCell)
};

#if WXWIN_COMPATIBILITY_2_4
inline wxHtmlCell* wxHtmlContainerCell::GetFirstCell() const
    { return GetFirstChild(); }
#endif




// ---------------------------------------------------------------------------
// wxHtmlColourCell
//                  Color changer.
// ---------------------------------------------------------------------------

class WXDLLIMPEXP_HTML wxHtmlColourCell : public wxHtmlCell
{
public:
    wxHtmlColourCell(const wxColour& clr, int flags = wxHTML_CLR_FOREGROUND) : wxHtmlCell() {m_Colour = clr; m_Flags = flags;}
    virtual void Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
                      wxHtmlRenderingInfo& info);
    virtual void DrawInvisible(wxDC& dc, int x, int y,
                               wxHtmlRenderingInfo& info);

protected:
    wxColour m_Colour;
    unsigned m_Flags;

    DECLARE_ABSTRACT_CLASS(wxHtmlColourCell)
    DECLARE_NO_COPY_CLASS(wxHtmlColourCell)
};




//--------------------------------------------------------------------------------
// wxHtmlFontCell
//                  Sets actual font used for text rendering
//--------------------------------------------------------------------------------

class WXDLLIMPEXP_HTML wxHtmlFontCell : public wxHtmlCell
{
public:
    wxHtmlFontCell(wxFont *font) : wxHtmlCell() { m_Font = (*font); }
    virtual void Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
                      wxHtmlRenderingInfo& info);
    virtual void DrawInvisible(wxDC& dc, int x, int y,
                               wxHtmlRenderingInfo& info);

protected:
    wxFont m_Font;

    DECLARE_ABSTRACT_CLASS(wxHtmlFontCell)
    DECLARE_NO_COPY_CLASS(wxHtmlFontCell)
};






//--------------------------------------------------------------------------------
// wxHtmlwidgetCell
//                  This cell is connected with wxWindow object
//                  You can use it to insert windows into HTML page
//                  (buttons, input boxes etc.)
//--------------------------------------------------------------------------------

class WXDLLIMPEXP_HTML wxHtmlWidgetCell : public wxHtmlCell
{
public:
    // !!! wnd must have correct parent!
    // if w != 0 then the m_Wnd has 'floating' width - it adjust
    // it's width according to parent container's width
    // (w is percent of parent's width)
    wxHtmlWidgetCell(wxWindow *wnd, int w = 0);
    virtual ~wxHtmlWidgetCell() { m_Wnd->Destroy(); }
    virtual void Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
                      wxHtmlRenderingInfo& info);
    virtual void DrawInvisible(wxDC& dc, int x, int y,
                               wxHtmlRenderingInfo& info);
    virtual void Layout(int w);

protected:
    wxWindow* m_Wnd;
    int m_WidthFloat;
            // width float is used in adjustWidth (it is in percents)

    DECLARE_ABSTRACT_CLASS(wxHtmlWidgetCell)
    DECLARE_NO_COPY_CLASS(wxHtmlWidgetCell)
};



//--------------------------------------------------------------------------------
// wxHtmlLinkInfo
//                  Internal data structure. It represents hypertext link
//--------------------------------------------------------------------------------

class WXDLLIMPEXP_HTML wxHtmlLinkInfo : public wxObject
{
public:
    wxHtmlLinkInfo() : wxObject()
          { m_Href = m_Target = wxEmptyString; m_Event = NULL, m_Cell = NULL; }
    wxHtmlLinkInfo(const wxString& href, const wxString& target = wxEmptyString) : wxObject()
          { m_Href = href; m_Target = target; m_Event = NULL, m_Cell = NULL; }
    wxHtmlLinkInfo(const wxHtmlLinkInfo& l) : wxObject()
          { m_Href = l.m_Href, m_Target = l.m_Target, m_Event = l.m_Event;
            m_Cell = l.m_Cell; }
    wxHtmlLinkInfo& operator=(const wxHtmlLinkInfo& l)
          { m_Href = l.m_Href, m_Target = l.m_Target, m_Event = l.m_Event;
            m_Cell = l.m_Cell; return *this; }

    void SetEvent(const wxMouseEvent *e) { m_Event = e; }
    void SetHtmlCell(const wxHtmlCell *e) { m_Cell = e; }

    wxString GetHref() const { return m_Href; }
    wxString GetTarget() const { return m_Target; }
    const wxMouseEvent* GetEvent() const { return m_Event; }
    const wxHtmlCell* GetHtmlCell() const { return m_Cell; }

private:
    wxString m_Href, m_Target;
    const wxMouseEvent *m_Event;
    const wxHtmlCell *m_Cell;
};



// ----------------------------------------------------------------------------
// wxHtmlTerminalCellsInterator
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_HTML wxHtmlTerminalCellsInterator
{
public:
    wxHtmlTerminalCellsInterator(const wxHtmlCell *from, const wxHtmlCell *to)
        : m_to(to), m_pos(from) {}

    operator bool() const { return m_pos != NULL; }
    const wxHtmlCell* operator++();
    const wxHtmlCell* operator->() const { return m_pos; }
    const wxHtmlCell* operator*() const { return m_pos; }

private:
    const wxHtmlCell *m_to, *m_pos;
};



#endif // wxUSE_HTML

#endif // _WX_HTMLCELL_H_

