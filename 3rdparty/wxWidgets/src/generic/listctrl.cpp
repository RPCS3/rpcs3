/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/listctrl.cpp
// Purpose:     generic implementation of wxListCtrl
// Author:      Robert Roebling
//              Vadim Zeitlin (virtual list control support)
// Id:          $Id: listctrl.cpp 57542 2008-12-25 13:03:24Z VZ $
// Copyright:   (c) 1998 Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// TODO
//
//   1. we need to implement searching/sorting for virtual controls somehow
// 2. when changing selection the lines are refreshed twice


// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_LISTCTRL

#include "wx/listctrl.h"

#if (!defined(__WXMSW__) || defined(__WXUNIVERSAL__)) && (!defined(__WXMAC__)|| defined(__WXUNIVERSAL__))
    // if we have a native version, its implementation file does all this
    IMPLEMENT_DYNAMIC_CLASS(wxListItem, wxObject)
    IMPLEMENT_DYNAMIC_CLASS(wxListView, wxListCtrl)
    IMPLEMENT_DYNAMIC_CLASS(wxListEvent, wxNotifyEvent)

    IMPLEMENT_DYNAMIC_CLASS(wxListCtrl, wxGenericListCtrl)
#endif

#ifndef WX_PRECOMP
    #include "wx/scrolwin.h"
    #include "wx/timer.h"
    #include "wx/settings.h"
    #include "wx/dynarray.h"
    #include "wx/dcclient.h"
    #include "wx/dcscreen.h"
    #include "wx/math.h"
#endif

#include "wx/imaglist.h"
#include "wx/selstore.h"
#include "wx/renderer.h"

#ifdef __WXMAC__
    #include "wx/mac/private.h"
#endif


// NOTE: If using the wxListBox visual attributes works everywhere then this can
// be removed, as well as the #else case below.
#define _USE_VISATTR 0


// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// // the height of the header window (FIXME: should depend on its font!)
// static const int HEADER_HEIGHT = 23;

static const int SCROLL_UNIT_X = 15;

// the spacing between the lines (in report mode)
static const int LINE_SPACING = 0;

// extra margins around the text label
#ifdef __WXGTK__
static const int EXTRA_WIDTH = 6;
#else
static const int EXTRA_WIDTH = 4;
#endif
static const int EXTRA_HEIGHT = 4;

// margin between the window and the items
static const int EXTRA_BORDER_X = 2;
static const int EXTRA_BORDER_Y = 2;

// offset for the header window
static const int HEADER_OFFSET_X = 0;
static const int HEADER_OFFSET_Y = 0;

// margin between rows of icons in [small] icon view
static const int MARGIN_BETWEEN_ROWS = 6;

// when autosizing the columns, add some slack
static const int AUTOSIZE_COL_MARGIN = 10;

// default width for the header columns
static const int WIDTH_COL_DEFAULT = 80;

// the space between the image and the text in the report mode
static const int IMAGE_MARGIN_IN_REPORT_MODE = 5;

// the space between the image and the text in the report mode in header
static const int HEADER_IMAGE_MARGIN_IN_REPORT_MODE = 2;

// ============================================================================
// private classes
// ============================================================================

//-----------------------------------------------------------------------------
//  wxColWidthInfo (internal)
//-----------------------------------------------------------------------------

struct wxColWidthInfo
{
    int     nMaxWidth;
    bool    bNeedsUpdate;   //  only set to true when an item whose
                            //  width == nMaxWidth is removed

    wxColWidthInfo(int w = 0, bool needsUpdate = false)
    {
        nMaxWidth = w;
        bNeedsUpdate = needsUpdate;
    }
};

WX_DEFINE_ARRAY_PTR(wxColWidthInfo *, ColWidthArray);

//-----------------------------------------------------------------------------
//  wxListItemData (internal)
//-----------------------------------------------------------------------------

class wxListItemData
{
public:
    wxListItemData(wxListMainWindow *owner);
    ~wxListItemData();

    void SetItem( const wxListItem &info );
    void SetImage( int image ) { m_image = image; }
    void SetData( wxUIntPtr data ) { m_data = data; }
    void SetPosition( int x, int y );
    void SetSize( int width, int height );

    bool HasText() const { return !m_text.empty(); }
    const wxString& GetText() const { return m_text; }
    void SetText(const wxString& text) { m_text = text; }

    // we can't use empty string for measuring the string width/height, so
    // always return something
    wxString GetTextForMeasuring() const
    {
        wxString s = GetText();
        if ( s.empty() )
            s = _T('H');

        return s;
    }

    bool IsHit( int x, int y ) const;

    int GetX() const;
    int GetY() const;
    int GetWidth() const;
    int GetHeight() const;

    int GetImage() const { return m_image; }
    bool HasImage() const { return GetImage() != -1; }

    void GetItem( wxListItem &info ) const;

    void SetAttr(wxListItemAttr *attr) { m_attr = attr; }
    wxListItemAttr *GetAttr() const { return m_attr; }

public:
    // the item image or -1
    int m_image;

    // user data associated with the item
    wxUIntPtr m_data;

    // the item coordinates are not used in report mode; instead this pointer is
    // NULL and the owner window is used to retrieve the item position and size
    wxRect *m_rect;

    // the list ctrl we are in
    wxListMainWindow *m_owner;

    // custom attributes or NULL
    wxListItemAttr *m_attr;

protected:
    // common part of all ctors
    void Init();

    wxString m_text;
};

//-----------------------------------------------------------------------------
//  wxListHeaderData (internal)
//-----------------------------------------------------------------------------

class wxListHeaderData : public wxObject
{
public:
    wxListHeaderData();
    wxListHeaderData( const wxListItem &info );
    void SetItem( const wxListItem &item );
    void SetPosition( int x, int y );
    void SetWidth( int w );
    void SetState( int state );
    void SetFormat( int format );
    void SetHeight( int h );
    bool HasImage() const;

    bool HasText() const { return !m_text.empty(); }
    const wxString& GetText() const { return m_text; }
    void SetText(const wxString& text) { m_text = text; }

    void GetItem( wxListItem &item );

    bool IsHit( int x, int y ) const;
    int GetImage() const;
    int GetWidth() const;
    int GetFormat() const;
    int GetState() const;

protected:
    long      m_mask;
    int       m_image;
    wxString  m_text;
    int       m_format;
    int       m_width;
    int       m_xpos,
              m_ypos;
    int       m_height;
    int       m_state;

private:
    void Init();
};

//-----------------------------------------------------------------------------
//  wxListLineData (internal)
//-----------------------------------------------------------------------------

WX_DECLARE_EXPORTED_LIST(wxListItemData, wxListItemDataList);
#include "wx/listimpl.cpp"
WX_DEFINE_LIST(wxListItemDataList)

class wxListLineData
{
public:
    // the list of subitems: only may have more than one item in report mode
    wxListItemDataList m_items;

    // this is not used in report view
    struct GeometryInfo
    {
        // total item rect
        wxRect m_rectAll;

        // label only
        wxRect m_rectLabel;

        // icon only
        wxRect m_rectIcon;

        // the part to be highlighted
        wxRect m_rectHighlight;

        // extend all our rects to be centered inside the one of given width
        void ExtendWidth(wxCoord w)
        {
            wxASSERT_MSG( m_rectAll.width <= w,
                            _T("width can only be increased") );

            m_rectAll.width = w;
            m_rectLabel.x = m_rectAll.x + (w - m_rectLabel.width) / 2;
            m_rectIcon.x = m_rectAll.x + (w - m_rectIcon.width) / 2;
            m_rectHighlight.x = m_rectAll.x + (w - m_rectHighlight.width) / 2;
        }
    }
    *m_gi;

    // is this item selected? [NB: not used in virtual mode]
    bool m_highlighted;

    // back pointer to the list ctrl
    wxListMainWindow *m_owner;

public:
    wxListLineData(wxListMainWindow *owner);

    ~wxListLineData()
    {
        WX_CLEAR_LIST(wxListItemDataList, m_items);
        delete m_gi;
    }

    // are we in report mode?
    inline bool InReportView() const;

    // are we in virtual report mode?
    inline bool IsVirtual() const;

    // these 2 methods shouldn't be called for report view controls, in that
    // case we determine our position/size ourselves

    // calculate the size of the line
    void CalculateSize( wxDC *dc, int spacing );

    // remember the position this line appears at
    void SetPosition( int x, int y, int spacing );

    // wxListCtrl API

    void SetImage( int image ) { SetImage(0, image); }
    int GetImage() const { return GetImage(0); }
    void SetImage( int index, int image );
    int GetImage( int index ) const;

    bool HasImage() const { return GetImage() != -1; }
    bool HasText() const { return !GetText(0).empty(); }

    void SetItem( int index, const wxListItem &info );
    void GetItem( int index, wxListItem &info );

    wxString GetText(int index) const;
    void SetText( int index, const wxString& s );

    wxListItemAttr *GetAttr() const;
    void SetAttr(wxListItemAttr *attr);

    // return true if the highlighting really changed
    bool Highlight( bool on );

    void ReverseHighlight();

    bool IsHighlighted() const
    {
        wxASSERT_MSG( !IsVirtual(), _T("unexpected call to IsHighlighted") );

        return m_highlighted;
    }

    // draw the line on the given DC in icon/list mode
    void Draw( wxDC *dc );

    // the same in report mode
    void DrawInReportMode( wxDC *dc,
                           const wxRect& rect,
                           const wxRect& rectHL,
                           bool highlighted );

private:
    // set the line to contain num items (only can be > 1 in report mode)
    void InitItems( int num );

    // get the mode (i.e. style)  of the list control
    inline int GetMode() const;

    // prepare the DC for drawing with these item's attributes, return true if
    // we need to draw the items background to highlight it, false otherwise
    bool SetAttributes(wxDC *dc,
                       const wxListItemAttr *attr,
                       bool highlight);

    // draw the text on the DC with the correct justification; also add an
    // ellipsis if the text is too large to fit in the current width
    void DrawTextFormatted(wxDC *dc,
                           const wxString &text,
                           int col,
                           int x,
                           int yMid,    // this is middle, not top, of the text
                           int width);
};

WX_DECLARE_EXPORTED_OBJARRAY(wxListLineData, wxListLineDataArray);
#include "wx/arrimpl.cpp"
WX_DEFINE_OBJARRAY(wxListLineDataArray)

//-----------------------------------------------------------------------------
//  wxListHeaderWindow (internal)
//-----------------------------------------------------------------------------

class wxListHeaderWindow : public wxWindow
{
protected:
    wxListMainWindow  *m_owner;
    const wxCursor    *m_currentCursor;
    wxCursor          *m_resizeCursor;
    bool               m_isDragging;

    // column being resized or -1
    int m_column;

    // divider line position in logical (unscrolled) coords
    int m_currentX;

    // minimal position beyond which the divider line
    // can't be dragged in logical coords
    int m_minX;

public:
    wxListHeaderWindow();

    wxListHeaderWindow( wxWindow *win,
                        wxWindowID id,
                        wxListMainWindow *owner,
                        const wxPoint &pos = wxDefaultPosition,
                        const wxSize &size = wxDefaultSize,
                        long style = 0,
                        const wxString &name = wxT("wxlistctrlcolumntitles") );

    virtual ~wxListHeaderWindow();

    void DrawCurrent();
    void AdjustDC( wxDC& dc );

    void OnPaint( wxPaintEvent &event );
    void OnMouse( wxMouseEvent &event );
    void OnSetFocus( wxFocusEvent &event );

    // needs refresh
    bool m_dirty;

private:
    // common part of all ctors
    void Init();

    // generate and process the list event of the given type, return true if
    // it wasn't vetoed, i.e. if we should proceed
    bool SendListEvent(wxEventType type, const wxPoint& pos);

    DECLARE_DYNAMIC_CLASS(wxListHeaderWindow)
    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
// wxListRenameTimer (internal)
//-----------------------------------------------------------------------------

class wxListRenameTimer: public wxTimer
{
private:
    wxListMainWindow *m_owner;

public:
    wxListRenameTimer( wxListMainWindow *owner );
    void Notify();
};

//-----------------------------------------------------------------------------
// wxListTextCtrlWrapper: wraps a wxTextCtrl to make it work for inline editing
//-----------------------------------------------------------------------------

class wxListTextCtrlWrapper : public wxEvtHandler
{
public:
    // NB: text must be a valid object but not Create()d yet
    wxListTextCtrlWrapper(wxListMainWindow *owner,
                          wxTextCtrl *text,
                          size_t itemEdit);

    wxTextCtrl *GetText() const { return m_text; }

    void AcceptChangesAndFinish();

protected:
    void OnChar( wxKeyEvent &event );
    void OnKeyUp( wxKeyEvent &event );
    void OnKillFocus( wxFocusEvent &event );

    bool AcceptChanges();
    void Finish();

private:
    wxListMainWindow   *m_owner;
    wxTextCtrl         *m_text;
    wxString            m_startValue;
    size_t              m_itemEdited;
    bool                m_finished;
    bool                m_aboutToFinish;

    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
//  wxListMainWindow (internal)
//-----------------------------------------------------------------------------

WX_DECLARE_EXPORTED_LIST(wxListHeaderData, wxListHeaderDataList);
#include "wx/listimpl.cpp"
WX_DEFINE_LIST(wxListHeaderDataList)

class wxListMainWindow : public wxScrolledWindow
{
public:
    wxListMainWindow();
    wxListMainWindow( wxWindow *parent,
                      wxWindowID id,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize,
                      long style = 0,
                      const wxString &name = _T("listctrlmainwindow") );

    virtual ~wxListMainWindow();

    bool HasFlag(int flag) const { return m_parent->HasFlag(flag); }

    // return true if this is a virtual list control
    bool IsVirtual() const { return HasFlag(wxLC_VIRTUAL); }

    // return true if the control is in report mode
    bool InReportView() const { return HasFlag(wxLC_REPORT); }

    // return true if we are in single selection mode, false if multi sel
    bool IsSingleSel() const { return HasFlag(wxLC_SINGLE_SEL); }

    // do we have a header window?
    bool HasHeader() const
        { return InReportView() && !HasFlag(wxLC_NO_HEADER); }

    void HighlightAll( bool on );

    // all these functions only do something if the line is currently visible

    // change the line "selected" state, return true if it really changed
    bool HighlightLine( size_t line, bool highlight = true);

    // as HighlightLine() but do it for the range of lines: this is incredibly
    // more efficient for virtual list controls!
    //
    // NB: unlike HighlightLine() this one does refresh the lines on screen
    void HighlightLines( size_t lineFrom, size_t lineTo, bool on = true );

    // toggle the line state and refresh it
    void ReverseHighlight( size_t line )
        { HighlightLine(line, !IsHighlighted(line)); RefreshLine(line); }

    // return true if the line is highlighted
    bool IsHighlighted(size_t line) const;

    // refresh one or several lines at once
    void RefreshLine( size_t line );
    void RefreshLines( size_t lineFrom, size_t lineTo );

    // refresh all selected items
    void RefreshSelected();

    // refresh all lines below the given one: the difference with
    // RefreshLines() is that the index here might not be a valid one (happens
    // when the last line is deleted)
    void RefreshAfter( size_t lineFrom );

    // the methods which are forwarded to wxListLineData itself in list/icon
    // modes but are here because the lines don't store their positions in the
    // report mode

    // get the bound rect for the entire line
    wxRect GetLineRect(size_t line) const;

    // get the bound rect of the label
    wxRect GetLineLabelRect(size_t line) const;

    // get the bound rect of the items icon (only may be called if we do have
    // an icon!)
    wxRect GetLineIconRect(size_t line) const;

    // get the rect to be highlighted when the item has focus
    wxRect GetLineHighlightRect(size_t line) const;

    // get the size of the total line rect
    wxSize GetLineSize(size_t line) const
        { return GetLineRect(line).GetSize(); }

    // return the hit code for the corresponding position (in this line)
    long HitTestLine(size_t line, int x, int y) const;

    // bring the selected item into view, scrolling to it if necessary
    void MoveToItem(size_t item);

    bool ScrollList( int WXUNUSED(dx), int dy );

    // bring the current item into view
    void MoveToFocus() { MoveToItem(m_current); }

    // start editing the label of the given item
    wxTextCtrl *EditLabel(long item,
                          wxClassInfo* textControlClass = CLASSINFO(wxTextCtrl));
    wxTextCtrl *GetEditControl() const
    {
        return m_textctrlWrapper ? m_textctrlWrapper->GetText() : NULL;
    }

    void FinishEditing(wxTextCtrl *text)
    {
        delete text;
        m_textctrlWrapper = NULL;
        SetFocusIgnoringChildren();
    }

    // suspend/resume redrawing the control
    void Freeze();
    void Thaw();

    void OnRenameTimer();
    bool OnRenameAccept(size_t itemEdit, const wxString& value);
    void OnRenameCancelled(size_t itemEdit);

    void OnMouse( wxMouseEvent &event );

    // called to switch the selection from the current item to newCurrent,
    void OnArrowChar( size_t newCurrent, const wxKeyEvent& event );

    void OnChar( wxKeyEvent &event );
    void OnKeyDown( wxKeyEvent &event );
    void OnKeyUp( wxKeyEvent &event );
    void OnSetFocus( wxFocusEvent &event );
    void OnKillFocus( wxFocusEvent &event );
    void OnScroll( wxScrollWinEvent& event );

    void OnPaint( wxPaintEvent &event );

    void OnChildFocus(wxChildFocusEvent& event);
    
    void DrawImage( int index, wxDC *dc, int x, int y );
    void GetImageSize( int index, int &width, int &height ) const;
    int GetTextLength( const wxString &s ) const;

    void SetImageList( wxImageList *imageList, int which );
    void SetItemSpacing( int spacing, bool isSmall = false );
    int GetItemSpacing( bool isSmall = false );

    void SetColumn( int col, wxListItem &item );
    void SetColumnWidth( int col, int width );
    void GetColumn( int col, wxListItem &item ) const;
    int GetColumnWidth( int col ) const;
    int GetColumnCount() const { return m_columns.GetCount(); }

    // returns the sum of the heights of all columns
    int GetHeaderWidth() const;

    int GetCountPerPage() const;

    void SetItem( wxListItem &item );
    void GetItem( wxListItem &item ) const;
    void SetItemState( long item, long state, long stateMask );
    void SetItemStateAll( long state, long stateMask );
    int GetItemState( long item, long stateMask ) const;
    void GetItemRect( long index, wxRect &rect ) const;
    wxRect GetViewRect() const;
    bool GetItemPosition( long item, wxPoint& pos ) const;
    int GetSelectedItemCount() const;

    wxString GetItemText(long item) const
    {
        wxListItem info;
        info.m_mask = wxLIST_MASK_TEXT;
        info.m_itemId = item;
        GetItem( info );
        return info.m_text;
    }

    void SetItemText(long item, const wxString& value)
    {
        wxListItem info;
        info.m_mask = wxLIST_MASK_TEXT;
        info.m_itemId = item;
        info.m_text = value;
        SetItem( info );
    }

    // set the scrollbars and update the positions of the items
    void RecalculatePositions(bool noRefresh = false);

    // refresh the window and the header
    void RefreshAll();

    long GetNextItem( long item, int geometry, int state ) const;
    void DeleteItem( long index );
    void DeleteAllItems();
    void DeleteColumn( int col );
    void DeleteEverything();
    void EnsureVisible( long index );
    long FindItem( long start, const wxString& str, bool partial = false );
    long FindItem( long start, wxUIntPtr data);
    long FindItem( const wxPoint& pt );
    long HitTest( int x, int y, int &flags ) const;
    void InsertItem( wxListItem &item );
    void InsertColumn( long col, wxListItem &item );
    int GetItemWidthWithImage(wxListItem * item);
    void SortItems( wxListCtrlCompare fn, long data );

    size_t GetItemCount() const;
    bool IsEmpty() const { return GetItemCount() == 0; }
    void SetItemCount(long count);

    // change the current (== focused) item, send a notification event
    void ChangeCurrent(size_t current);
    void ResetCurrent() { ChangeCurrent((size_t)-1); }
    bool HasCurrent() const { return m_current != (size_t)-1; }

    // send out a wxListEvent
    void SendNotify( size_t line,
                     wxEventType command,
                     const wxPoint& point = wxDefaultPosition );

    // override base class virtual to reset m_lineHeight when the font changes
    virtual bool SetFont(const wxFont& font)
    {
        if ( !wxScrolledWindow::SetFont(font) )
            return false;

        m_lineHeight = 0;

        return true;
    }

    // these are for wxListLineData usage only

    // get the backpointer to the list ctrl
    wxGenericListCtrl *GetListCtrl() const
    {
        return wxStaticCast(GetParent(), wxGenericListCtrl);
    }

    // get the height of all lines (assuming they all do have the same height)
    wxCoord GetLineHeight() const;

    // get the y position of the given line (only for report view)
    wxCoord GetLineY(size_t line) const;

    // get the brush to use for the item highlighting
    wxBrush *GetHighlightBrush() const
    {
        return m_hasFocus ? m_highlightBrush : m_highlightUnfocusedBrush;
    }

    bool HasFocus() const
    {
        return m_hasFocus;
    }

//protected:
    // the array of all line objects for a non virtual list control (for the
    // virtual list control we only ever use m_lines[0])
    wxListLineDataArray  m_lines;

    // the list of column objects
    wxListHeaderDataList m_columns;

    // currently focused item or -1
    size_t               m_current;

    // the number of lines per page
    int                  m_linesPerPage;

    // this flag is set when something which should result in the window
    // redrawing happens (i.e. an item was added or deleted, or its appearance
    // changed) and OnPaint() doesn't redraw the window while it is set which
    // allows to minimize the number of repaintings when a lot of items are
    // being added. The real repainting occurs only after the next OnIdle()
    // call
    bool                 m_dirty;

    wxColour            *m_highlightColour;
    wxImageList         *m_small_image_list;
    wxImageList         *m_normal_image_list;
    int                  m_small_spacing;
    int                  m_normal_spacing;
    bool                 m_hasFocus;

    bool                 m_lastOnSame;
    wxTimer             *m_renameTimer;
    bool                 m_isCreated;
    int                  m_dragCount;
    wxPoint              m_dragStart;
    ColWidthArray        m_aColWidths;

    // for double click logic
    size_t m_lineLastClicked,
           m_lineBeforeLastClicked,
           m_lineSelectSingleOnUp;

protected:
    wxWindow *GetMainWindowOfCompositeControl() { return GetParent(); }

    // the total count of items in a virtual list control
    size_t m_countVirt;

    // the object maintaining the items selection state, only used in virtual
    // controls
    wxSelectionStore m_selStore;

    // common part of all ctors
    void Init();

    // get the line data for the given index
    wxListLineData *GetLine(size_t n) const
    {
        wxASSERT_MSG( n != (size_t)-1, _T("invalid line index") );

        if ( IsVirtual() )
        {
            wxConstCast(this, wxListMainWindow)->CacheLineData(n);
            n = 0;
        }

        return &m_lines[n];
    }

    // get a dummy line which can be used for geometry calculations and such:
    // you must use GetLine() if you want to really draw the line
    wxListLineData *GetDummyLine() const;

    // cache the line data of the n-th line in m_lines[0]
    void CacheLineData(size_t line);

    // get the range of visible lines
    void GetVisibleLinesRange(size_t *from, size_t *to);

    // force us to recalculate the range of visible lines
    void ResetVisibleLinesRange() { m_lineFrom = (size_t)-1; }

    // get the colour to be used for drawing the rules
    wxColour GetRuleColour() const
    {
        return wxSystemSettings::GetColour(wxSYS_COLOUR_3DLIGHT);
    }

private:
    // initialize the current item if needed
    void UpdateCurrent();

    // delete all items but don't refresh: called from dtor
    void DoDeleteAllItems();

    // the height of one line using the current font
    wxCoord m_lineHeight;

    // the total header width or 0 if not calculated yet
    wxCoord m_headerWidth;

    // the first and last lines being shown on screen right now (inclusive),
    // both may be -1 if they must be calculated so never access them directly:
    // use GetVisibleLinesRange() above instead
    size_t m_lineFrom,
           m_lineTo;

    // the brushes to use for item highlighting when we do/don't have focus
    wxBrush *m_highlightBrush,
            *m_highlightUnfocusedBrush;

    // if this is > 0, the control is frozen and doesn't redraw itself
    size_t m_freezeCount;

    // wrapper around the text control currently used for in place editing or
    // NULL if no item is being edited
    wxListTextCtrlWrapper *m_textctrlWrapper;


    DECLARE_DYNAMIC_CLASS(wxListMainWindow)
    DECLARE_EVENT_TABLE()

    friend class wxGenericListCtrl;
};


wxListItemData::~wxListItemData()
{
    // in the virtual list control the attributes are managed by the main
    // program, so don't delete them
    if ( !m_owner->IsVirtual() )
        delete m_attr;

    delete m_rect;
}

void wxListItemData::Init()
{
    m_image = -1;
    m_data = 0;

    m_attr = NULL;
}

wxListItemData::wxListItemData(wxListMainWindow *owner)
{
    Init();

    m_owner = owner;

    if ( owner->InReportView() )
        m_rect = NULL;
    else
        m_rect = new wxRect;
}

void wxListItemData::SetItem( const wxListItem &info )
{
    if ( info.m_mask & wxLIST_MASK_TEXT )
        SetText(info.m_text);
    if ( info.m_mask & wxLIST_MASK_IMAGE )
        m_image = info.m_image;
    if ( info.m_mask & wxLIST_MASK_DATA )
        m_data = info.m_data;

    if ( info.HasAttributes() )
    {
        if ( m_attr )
            m_attr->AssignFrom(*info.GetAttributes());
        else
            m_attr = new wxListItemAttr(*info.GetAttributes());
    }

    if ( m_rect )
    {
        m_rect->x =
        m_rect->y =
        m_rect->height = 0;
        m_rect->width = info.m_width;
    }
}

void wxListItemData::SetPosition( int x, int y )
{
    wxCHECK_RET( m_rect, _T("unexpected SetPosition() call") );

    m_rect->x = x;
    m_rect->y = y;
}

void wxListItemData::SetSize( int width, int height )
{
    wxCHECK_RET( m_rect, _T("unexpected SetSize() call") );

    if ( width != -1 )
        m_rect->width = width;
    if ( height != -1 )
        m_rect->height = height;
}

bool wxListItemData::IsHit( int x, int y ) const
{
    wxCHECK_MSG( m_rect, false, _T("can't be called in this mode") );

    return wxRect(GetX(), GetY(), GetWidth(), GetHeight()).Contains(x, y);
}

int wxListItemData::GetX() const
{
    wxCHECK_MSG( m_rect, 0, _T("can't be called in this mode") );

    return m_rect->x;
}

int wxListItemData::GetY() const
{
    wxCHECK_MSG( m_rect, 0, _T("can't be called in this mode") );

    return m_rect->y;
}

int wxListItemData::GetWidth() const
{
    wxCHECK_MSG( m_rect, 0, _T("can't be called in this mode") );

    return m_rect->width;
}

int wxListItemData::GetHeight() const
{
    wxCHECK_MSG( m_rect, 0, _T("can't be called in this mode") );

    return m_rect->height;
}

void wxListItemData::GetItem( wxListItem &info ) const
{
    long mask = info.m_mask;
    if ( !mask )
        // by default, get everything for backwards compatibility
        mask = -1;

    if ( mask & wxLIST_MASK_TEXT )
        info.m_text = m_text;
    if ( mask & wxLIST_MASK_IMAGE )
        info.m_image = m_image;
    if ( mask & wxLIST_MASK_DATA )
        info.m_data = m_data;

    if ( m_attr )
    {
        if ( m_attr->HasTextColour() )
            info.SetTextColour(m_attr->GetTextColour());
        if ( m_attr->HasBackgroundColour() )
            info.SetBackgroundColour(m_attr->GetBackgroundColour());
        if ( m_attr->HasFont() )
            info.SetFont(m_attr->GetFont());
    }
}

//-----------------------------------------------------------------------------
//  wxListHeaderData
//-----------------------------------------------------------------------------

void wxListHeaderData::Init()
{
    m_mask = 0;
    m_image = -1;
    m_format = 0;
    m_width = 0;
    m_xpos = 0;
    m_ypos = 0;
    m_height = 0;
    m_state = 0;
}

wxListHeaderData::wxListHeaderData()
{
    Init();
}

wxListHeaderData::wxListHeaderData( const wxListItem &item )
{
    Init();

    SetItem( item );
}

void wxListHeaderData::SetItem( const wxListItem &item )
{
    m_mask = item.m_mask;

    if ( m_mask & wxLIST_MASK_TEXT )
        m_text = item.m_text;

    if ( m_mask & wxLIST_MASK_IMAGE )
        m_image = item.m_image;

    if ( m_mask & wxLIST_MASK_FORMAT )
        m_format = item.m_format;

    if ( m_mask & wxLIST_MASK_WIDTH )
        SetWidth(item.m_width);

    if ( m_mask & wxLIST_MASK_STATE )
        SetState(item.m_state);
}

void wxListHeaderData::SetPosition( int x, int y )
{
    m_xpos = x;
    m_ypos = y;
}

void wxListHeaderData::SetHeight( int h )
{
    m_height = h;
}

void wxListHeaderData::SetWidth( int w )
{
    m_width = w < 0 ? WIDTH_COL_DEFAULT : w;
}

void wxListHeaderData::SetState( int flag )
{
    m_state = flag;
}

void wxListHeaderData::SetFormat( int format )
{
    m_format = format;
}

bool wxListHeaderData::HasImage() const
{
    return m_image != -1;
}

bool wxListHeaderData::IsHit( int x, int y ) const
{
    return ((x >= m_xpos) && (x <= m_xpos+m_width) && (y >= m_ypos) && (y <= m_ypos+m_height));
}

void wxListHeaderData::GetItem( wxListItem& item )
{
    item.m_mask = m_mask;
    item.m_text = m_text;
    item.m_image = m_image;
    item.m_format = m_format;
    item.m_width = m_width;
    item.m_state = m_state;
}

int wxListHeaderData::GetImage() const
{
    return m_image;
}

int wxListHeaderData::GetWidth() const
{
    return m_width;
}

int wxListHeaderData::GetFormat() const
{
    return m_format;
}

int wxListHeaderData::GetState() const
{
    return m_state;
}

//-----------------------------------------------------------------------------
//  wxListLineData
//-----------------------------------------------------------------------------

inline int wxListLineData::GetMode() const
{
    return m_owner->GetListCtrl()->GetWindowStyleFlag() & wxLC_MASK_TYPE;
}

inline bool wxListLineData::InReportView() const
{
    return m_owner->HasFlag(wxLC_REPORT);
}

inline bool wxListLineData::IsVirtual() const
{
    return m_owner->IsVirtual();
}

wxListLineData::wxListLineData( wxListMainWindow *owner )
{
    m_owner = owner;

    if ( InReportView() )
        m_gi = NULL;
    else // !report
        m_gi = new GeometryInfo;

    m_highlighted = false;

    InitItems( GetMode() == wxLC_REPORT ? m_owner->GetColumnCount() : 1 );
}

void wxListLineData::CalculateSize( wxDC *dc, int spacing )
{
    wxListItemDataList::compatibility_iterator node = m_items.GetFirst();
    wxCHECK_RET( node, _T("no subitems at all??") );

    wxListItemData *item = node->GetData();

    wxString s;
    wxCoord lw, lh;

    switch ( GetMode() )
    {
        case wxLC_ICON:
        case wxLC_SMALL_ICON:
            m_gi->m_rectAll.width = spacing;

            s = item->GetText();

            if ( s.empty() )
            {
                lh =
                m_gi->m_rectLabel.width =
                m_gi->m_rectLabel.height = 0;
            }
            else // has label
            {
                dc->GetTextExtent( s, &lw, &lh );
                lw += EXTRA_WIDTH;
                lh += EXTRA_HEIGHT;

                m_gi->m_rectAll.height = spacing + lh;
                if (lw > spacing)
                    m_gi->m_rectAll.width = lw;

                m_gi->m_rectLabel.width = lw;
                m_gi->m_rectLabel.height = lh;
            }

            if (item->HasImage())
            {
                int w, h;
                m_owner->GetImageSize( item->GetImage(), w, h );
                m_gi->m_rectIcon.width = w + 8;
                m_gi->m_rectIcon.height = h + 8;

                if ( m_gi->m_rectIcon.width > m_gi->m_rectAll.width )
                    m_gi->m_rectAll.width = m_gi->m_rectIcon.width;
                if ( m_gi->m_rectIcon.height + lh > m_gi->m_rectAll.height - 4 )
                    m_gi->m_rectAll.height = m_gi->m_rectIcon.height + lh + 4;
            }

            if ( item->HasText() )
            {
                m_gi->m_rectHighlight.width = m_gi->m_rectLabel.width;
                m_gi->m_rectHighlight.height = m_gi->m_rectLabel.height;
            }
            else // no text, highlight the icon
            {
                m_gi->m_rectHighlight.width = m_gi->m_rectIcon.width;
                m_gi->m_rectHighlight.height = m_gi->m_rectIcon.height;
            }
            break;

        case wxLC_LIST:
            s = item->GetTextForMeasuring();

            dc->GetTextExtent( s, &lw, &lh );
            lw += EXTRA_WIDTH;
            lh += EXTRA_HEIGHT;

            m_gi->m_rectLabel.width = lw;
            m_gi->m_rectLabel.height = lh;

            m_gi->m_rectAll.width = lw;
            m_gi->m_rectAll.height = lh;

            if (item->HasImage())
            {
                int w, h;
                m_owner->GetImageSize( item->GetImage(), w, h );
                m_gi->m_rectIcon.width = w;
                m_gi->m_rectIcon.height = h;

                m_gi->m_rectAll.width += 4 + w;
                if (h > m_gi->m_rectAll.height)
                    m_gi->m_rectAll.height = h;
            }

            m_gi->m_rectHighlight.width = m_gi->m_rectAll.width;
            m_gi->m_rectHighlight.height = m_gi->m_rectAll.height;
            break;

        case wxLC_REPORT:
            wxFAIL_MSG( _T("unexpected call to SetSize") );
            break;

        default:
            wxFAIL_MSG( _T("unknown mode") );
            break;
    }
}

void wxListLineData::SetPosition( int x, int y, int spacing )
{
    wxListItemDataList::compatibility_iterator node = m_items.GetFirst();
    wxCHECK_RET( node, _T("no subitems at all??") );

    wxListItemData *item = node->GetData();

    switch ( GetMode() )
    {
        case wxLC_ICON:
        case wxLC_SMALL_ICON:
            m_gi->m_rectAll.x = x;
            m_gi->m_rectAll.y = y;

            if ( item->HasImage() )
            {
                m_gi->m_rectIcon.x = m_gi->m_rectAll.x + 4 +
                    (m_gi->m_rectAll.width - m_gi->m_rectIcon.width) / 2;
                m_gi->m_rectIcon.y = m_gi->m_rectAll.y + 4;
            }

            if ( item->HasText() )
            {
                if (m_gi->m_rectAll.width > spacing)
                    m_gi->m_rectLabel.x = m_gi->m_rectAll.x + (EXTRA_WIDTH/2);
                else
                    m_gi->m_rectLabel.x = m_gi->m_rectAll.x + (EXTRA_WIDTH/2) + (spacing / 2) - (m_gi->m_rectLabel.width / 2);
                m_gi->m_rectLabel.y = m_gi->m_rectAll.y + m_gi->m_rectAll.height + 2 - m_gi->m_rectLabel.height;
                m_gi->m_rectHighlight.x = m_gi->m_rectLabel.x - 2;
                m_gi->m_rectHighlight.y = m_gi->m_rectLabel.y - 2;
            }
            else // no text, highlight the icon
            {
                m_gi->m_rectHighlight.x = m_gi->m_rectIcon.x - 4;
                m_gi->m_rectHighlight.y = m_gi->m_rectIcon.y - 4;
            }
            break;

        case wxLC_LIST:
            m_gi->m_rectAll.x = x;
            m_gi->m_rectAll.y = y;

            m_gi->m_rectHighlight.x = m_gi->m_rectAll.x;
            m_gi->m_rectHighlight.y = m_gi->m_rectAll.y;
            m_gi->m_rectLabel.y = m_gi->m_rectAll.y + 2;

            if (item->HasImage())
            {
                m_gi->m_rectIcon.x = m_gi->m_rectAll.x + 2;
                m_gi->m_rectIcon.y = m_gi->m_rectAll.y + 2;
                m_gi->m_rectLabel.x = m_gi->m_rectAll.x + 4 + (EXTRA_WIDTH/2) + m_gi->m_rectIcon.width;
            }
            else
            {
                m_gi->m_rectLabel.x = m_gi->m_rectAll.x + (EXTRA_WIDTH/2);
            }
            break;

        case wxLC_REPORT:
            wxFAIL_MSG( _T("unexpected call to SetPosition") );
            break;

        default:
            wxFAIL_MSG( _T("unknown mode") );
            break;
    }
}

void wxListLineData::InitItems( int num )
{
    for (int i = 0; i < num; i++)
        m_items.Append( new wxListItemData(m_owner) );
}

void wxListLineData::SetItem( int index, const wxListItem &info )
{
    wxListItemDataList::compatibility_iterator node = m_items.Item( index );
    wxCHECK_RET( node, _T("invalid column index in SetItem") );

    wxListItemData *item = node->GetData();
    item->SetItem( info );
}

void wxListLineData::GetItem( int index, wxListItem &info )
{
    wxListItemDataList::compatibility_iterator node = m_items.Item( index );
    if (node)
    {
        wxListItemData *item = node->GetData();
        item->GetItem( info );
    }
}

wxString wxListLineData::GetText(int index) const
{
    wxString s;

    wxListItemDataList::compatibility_iterator node = m_items.Item( index );
    if (node)
    {
        wxListItemData *item = node->GetData();
        s = item->GetText();
    }

    return s;
}

void wxListLineData::SetText( int index, const wxString& s )
{
    wxListItemDataList::compatibility_iterator node = m_items.Item( index );
    if (node)
    {
        wxListItemData *item = node->GetData();
        item->SetText( s );
    }
}

void wxListLineData::SetImage( int index, int image )
{
    wxListItemDataList::compatibility_iterator node = m_items.Item( index );
    wxCHECK_RET( node, _T("invalid column index in SetImage()") );

    wxListItemData *item = node->GetData();
    item->SetImage(image);
}

int wxListLineData::GetImage( int index ) const
{
    wxListItemDataList::compatibility_iterator node = m_items.Item( index );
    wxCHECK_MSG( node, -1, _T("invalid column index in GetImage()") );

    wxListItemData *item = node->GetData();
    return item->GetImage();
}

wxListItemAttr *wxListLineData::GetAttr() const
{
    wxListItemDataList::compatibility_iterator node = m_items.GetFirst();
    wxCHECK_MSG( node, NULL, _T("invalid column index in GetAttr()") );

    wxListItemData *item = node->GetData();
    return item->GetAttr();
}

void wxListLineData::SetAttr(wxListItemAttr *attr)
{
    wxListItemDataList::compatibility_iterator node = m_items.GetFirst();
    wxCHECK_RET( node, _T("invalid column index in SetAttr()") );

    wxListItemData *item = node->GetData();
    item->SetAttr(attr);
}

bool wxListLineData::SetAttributes(wxDC *dc,
                                   const wxListItemAttr *attr,
                                   bool highlighted)
{
    wxWindow *listctrl = m_owner->GetParent();

    // fg colour

    // don't use foreground colour for drawing highlighted items - this might
    // make them completely invisible (and there is no way to do bit
    // arithmetics on wxColour, unfortunately)
    wxColour colText;
    if ( highlighted )
#ifdef __WXMAC__
    {
        if (m_owner->HasFocus()
#ifdef __WXMAC__
                && IsControlActive( (ControlRef)m_owner->GetHandle() )
#endif
        )
            colText = *wxWHITE;
        else
            colText = *wxBLACK;
    }
#else
        colText = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
#endif
    else if ( attr && attr->HasTextColour() )
        colText = attr->GetTextColour();
    else
        colText = listctrl->GetForegroundColour();

    dc->SetTextForeground(colText);

    // font
    wxFont font;
    if ( attr && attr->HasFont() )
        font = attr->GetFont();
    else
        font = listctrl->GetFont();

    dc->SetFont(font);

    // bg colour
    bool hasBgCol = attr && attr->HasBackgroundColour();
    if ( highlighted || hasBgCol )
    {
        if ( highlighted )
            dc->SetBrush( *m_owner->GetHighlightBrush() );
        else
            dc->SetBrush(wxBrush(attr->GetBackgroundColour(), wxSOLID));

        dc->SetPen( *wxTRANSPARENT_PEN );

        return true;
    }

    return false;
}

void wxListLineData::Draw( wxDC *dc )
{
    wxListItemDataList::compatibility_iterator node = m_items.GetFirst();
    wxCHECK_RET( node, _T("no subitems at all??") );

    bool highlighted = IsHighlighted();

    wxListItemAttr *attr = GetAttr();

    if ( SetAttributes(dc, attr, highlighted) )
#if ( !defined(__WXGTK20__) && !defined(__WXMAC__) )
    {
        dc->DrawRectangle( m_gi->m_rectHighlight );
    }
#else
    {
        if (highlighted)
        {
            int flags = wxCONTROL_SELECTED;
            if (m_owner->HasFocus()
#ifdef __WXMAC__
                && IsControlActive( (ControlRef)m_owner->GetHandle() )
#endif
            )
                flags |= wxCONTROL_FOCUSED;
            wxRendererNative::Get().DrawItemSelectionRect( m_owner, *dc, m_gi->m_rectHighlight, flags );

        }
        else
        {
            dc->DrawRectangle( m_gi->m_rectHighlight );
        }
    }
#endif

    // just for debugging to better see where the items are
#if 0
    dc->SetPen(*wxRED_PEN);
    dc->SetBrush(*wxTRANSPARENT_BRUSH);
    dc->DrawRectangle( m_gi->m_rectAll );
    dc->SetPen(*wxGREEN_PEN);
    dc->DrawRectangle( m_gi->m_rectIcon );
#endif

    wxListItemData *item = node->GetData();
    if (item->HasImage())
    {
        // centre the image inside our rectangle, this looks nicer when items
        // ae aligned in a row
        const wxRect& rectIcon = m_gi->m_rectIcon;

        m_owner->DrawImage(item->GetImage(), dc, rectIcon.x, rectIcon.y);
    }

    if (item->HasText())
    {
        const wxRect& rectLabel = m_gi->m_rectLabel;

        wxDCClipper clipper(*dc, rectLabel);
        dc->DrawText(item->GetText(), rectLabel.x, rectLabel.y);
    }
}

void wxListLineData::DrawInReportMode( wxDC *dc,
                                       const wxRect& rect,
                                       const wxRect& rectHL,
                                       bool highlighted )
{
    // TODO: later we should support setting different attributes for
    //       different columns - to do it, just add "col" argument to
    //       GetAttr() and move these lines into the loop below
    wxListItemAttr *attr = GetAttr();
    if ( SetAttributes(dc, attr, highlighted) )
#if ( !defined(__WXGTK20__) && !defined(__WXMAC__) )
    {
        dc->DrawRectangle( rectHL );
    }
#else
    {
        if (highlighted)
        {
            int flags = wxCONTROL_SELECTED;
            if (m_owner->HasFocus()
#ifdef __WXMAC__
                && IsControlActive( (ControlRef)m_owner->GetHandle() )
#endif
            )
                flags |= wxCONTROL_FOCUSED;
            wxRendererNative::Get().DrawItemSelectionRect( m_owner, *dc, rectHL, flags );
        }
        else
        {
            dc->DrawRectangle( rectHL );
        }
    }
#endif

    wxCoord x = rect.x + HEADER_OFFSET_X,
            yMid = rect.y + rect.height/2;
#ifdef __WXGTK__
    // This probably needs to be done
    // on all platforms as the icons
    // otherwise nearly touch the border
    x += 2;
#endif

    size_t col = 0;
    for ( wxListItemDataList::compatibility_iterator node = m_items.GetFirst();
          node;
          node = node->GetNext(), col++ )
    {
        wxListItemData *item = node->GetData();

        int width = m_owner->GetColumnWidth(col);
        int xOld = x;
        x += width;

        if ( item->HasImage() )
        {
            int ix, iy;
            m_owner->GetImageSize( item->GetImage(), ix, iy );
            m_owner->DrawImage( item->GetImage(), dc, xOld, yMid - iy/2 );

            ix += IMAGE_MARGIN_IN_REPORT_MODE;

            xOld += ix;
            width -= ix;
        }

        if ( item->HasText() )
            DrawTextFormatted(dc, item->GetText(), col, xOld, yMid, width - 8);
    }
}

void wxListLineData::DrawTextFormatted(wxDC *dc,
                                       const wxString& textOrig,
                                       int col,
                                       int x,
                                       int yMid,
                                       int width)
{
    // we don't support displaying multiple lines currently (and neither does
    // wxMSW FWIW) so just merge all the lines
    wxString text(textOrig);
    text.Replace(_T("\n"), _T(" "));

    wxCoord w, h;
    dc->GetTextExtent(text, &w, &h);

    const wxCoord y = yMid - (h + 1)/2;

    wxDCClipper clipper(*dc, x, y, width, h);

    // determine if the string can fit inside the current width
    if (w <= width)
    {
        // it can, draw it using the items alignment
        wxListItem item;
        m_owner->GetColumn(col, item);
        switch ( item.GetAlign() )
        {
            case wxLIST_FORMAT_LEFT:
                // nothing to do
                break;

            case wxLIST_FORMAT_RIGHT:
                x += width - w;
                break;

            case wxLIST_FORMAT_CENTER:
                x += (width - w) / 2;
                break;

            default:
                wxFAIL_MSG( _T("unknown list item format") );
                break;
        }

        dc->DrawText(text, x, y);
    }
    else // otherwise, truncate and add an ellipsis if possible
    {
        // determine the base width
        wxString ellipsis(wxT("..."));
        wxCoord base_w;
        dc->GetTextExtent(ellipsis, &base_w, &h);

        // continue until we have enough space or only one character left
        wxCoord w_c, h_c;
        size_t len = text.length();
        wxString drawntext = text.Left(len);
        while (len > 1)
        {
            dc->GetTextExtent(drawntext.Last(), &w_c, &h_c);
            drawntext.RemoveLast();
            len--;
            w -= w_c;
            if (w + base_w <= width)
                break;
        }

        // if still not enough space, remove ellipsis characters
        while (ellipsis.length() > 0 && w + base_w > width)
        {
            ellipsis = ellipsis.Left(ellipsis.length() - 1);
            dc->GetTextExtent(ellipsis, &base_w, &h);
        }

        // now draw the text
        dc->DrawText(drawntext, x, y);
        dc->DrawText(ellipsis, x + w, y);
    }
}

bool wxListLineData::Highlight( bool on )
{
    wxCHECK_MSG( !IsVirtual(), false, _T("unexpected call to Highlight") );

    if ( on == m_highlighted )
        return false;

    m_highlighted = on;

    return true;
}

void wxListLineData::ReverseHighlight( void )
{
    Highlight(!IsHighlighted());
}

//-----------------------------------------------------------------------------
//  wxListHeaderWindow
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxListHeaderWindow,wxWindow)

BEGIN_EVENT_TABLE(wxListHeaderWindow,wxWindow)
    EVT_PAINT         (wxListHeaderWindow::OnPaint)
    EVT_MOUSE_EVENTS  (wxListHeaderWindow::OnMouse)
    EVT_SET_FOCUS     (wxListHeaderWindow::OnSetFocus)
END_EVENT_TABLE()

void wxListHeaderWindow::Init()
{
    m_currentCursor = (wxCursor *) NULL;
    m_isDragging = false;
    m_dirty = false;
}

wxListHeaderWindow::wxListHeaderWindow()
{
    Init();

    m_owner = (wxListMainWindow *) NULL;
    m_resizeCursor = (wxCursor *) NULL;
}

wxListHeaderWindow::wxListHeaderWindow( wxWindow *win,
                                        wxWindowID id,
                                        wxListMainWindow *owner,
                                        const wxPoint& pos,
                                        const wxSize& size,
                                        long style,
                                        const wxString &name )
                  : wxWindow( win, id, pos, size, style, name )
{
    Init();

    m_owner = owner;
    m_resizeCursor = new wxCursor( wxCURSOR_SIZEWE );

#if _USE_VISATTR
    wxVisualAttributes attr = wxPanel::GetClassDefaultAttributes();
    SetOwnForegroundColour( attr.colFg );
    SetOwnBackgroundColour( attr.colBg );
    if (!m_hasFont)
        SetOwnFont( attr.font );
#else
    SetOwnForegroundColour( wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    SetOwnBackgroundColour( wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    if (!m_hasFont)
        SetOwnFont( wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT ));
#endif
}

wxListHeaderWindow::~wxListHeaderWindow()
{
    delete m_resizeCursor;
}

#ifdef __WXUNIVERSAL__
#include "wx/univ/renderer.h"
#include "wx/univ/theme.h"
#endif

// shift the DC origin to match the position of the main window horz
// scrollbar: this allows us to always use logical coords
void wxListHeaderWindow::AdjustDC(wxDC& dc)
{
    int xpix;
    m_owner->GetScrollPixelsPerUnit( &xpix, NULL );

    int view_start;
    m_owner->GetViewStart( &view_start, NULL );


    int org_x = 0;
    int org_y = 0;
    dc.GetDeviceOrigin( &org_x, &org_y );

    // account for the horz scrollbar offset
#ifdef __WXGTK__
    if (GetLayoutDirection() == wxLayout_RightToLeft)
    {
        // Maybe we just have to check for m_signX
        // in the DC, but I leave the #ifdef __WXGTK__
        // for now
        dc.SetDeviceOrigin( org_x + (view_start * xpix), org_y );
    }
    else
#endif
        dc.SetDeviceOrigin( org_x - (view_start * xpix), org_y );
}

void wxListHeaderWindow::OnPaint( wxPaintEvent &WXUNUSED(event) )
{
    wxPaintDC dc( this );

    PrepareDC( dc );
    AdjustDC( dc );

    dc.SetFont( GetFont() );

    // width and height of the entire header window
    int w, h;
    GetClientSize( &w, &h );
    m_owner->CalcUnscrolledPosition(w, 0, &w, NULL);

    dc.SetBackgroundMode(wxTRANSPARENT);
    dc.SetTextForeground(GetForegroundColour());

    int x = HEADER_OFFSET_X;
    int numColumns = m_owner->GetColumnCount();
    wxListItem item;
    for ( int i = 0; i < numColumns && x < w; i++ )
    {
        m_owner->GetColumn( i, item );
        int wCol = item.m_width;

        int cw = wCol;
        int ch = h;

        int flags = 0;
        if (!m_parent->IsEnabled())
            flags |= wxCONTROL_DISABLED;

// NB: The code below is not really Mac-specific, but since we are close
// to 2.8 release and I don't have time to test on other platforms, I
// defined this only for wxMac. If this behavior is desired on
// other platforms, please go ahead and revise or remove the #ifdef.
#ifdef __WXMAC__
        if ( !m_owner->IsVirtual() && (item.m_mask & wxLIST_MASK_STATE) &&
                (item.m_state & wxLIST_STATE_SELECTED) )
            flags |= wxCONTROL_SELECTED;
#endif

        wxRendererNative::Get().DrawHeaderButton
                                (
                                    this,
                                    dc,
                                    wxRect(x, HEADER_OFFSET_Y, cw, ch),
                                    flags
                                );

        // see if we have enough space for the column label

        // for this we need the width of the text
        wxCoord wLabel;
        wxCoord hLabel;
        dc.GetTextExtent(item.GetText(), &wLabel, &hLabel);
        wLabel += 2 * EXTRA_WIDTH;

        // and the width of the icon, if any
        int ix = 0, iy = 0;    // init them just to suppress the compiler warnings
        const int image = item.m_image;
        wxImageList *imageList;
        if ( image != -1 )
        {
            imageList = m_owner->m_small_image_list;
            if ( imageList )
            {
                imageList->GetSize(image, ix, iy);
                wLabel += ix + HEADER_IMAGE_MARGIN_IN_REPORT_MODE;
            }
        }
        else
        {
            imageList = NULL;
        }

        // ignore alignment if there is not enough space anyhow
        int xAligned;
        switch ( wLabel < cw ? item.GetAlign() : wxLIST_FORMAT_LEFT )
        {
            default:
                wxFAIL_MSG( _T("unknown list item format") );
                // fall through

            case wxLIST_FORMAT_LEFT:
                xAligned = x;
                break;

            case wxLIST_FORMAT_RIGHT:
                xAligned = x + cw - wLabel;
                break;

            case wxLIST_FORMAT_CENTER:
                xAligned = x + (cw - wLabel) / 2;
                break;
        }

        // draw the text and image clipping them so that they
        // don't overwrite the column boundary
        wxDCClipper clipper(dc, x, HEADER_OFFSET_Y, cw, h  );

        // if we have an image, draw it on the right of the label
        if ( imageList )
        {
            imageList->Draw
                       (
                        image,
                        dc,
                        xAligned + wLabel - ix - HEADER_IMAGE_MARGIN_IN_REPORT_MODE,
                        HEADER_OFFSET_Y + (h - 4 - iy)/2,
                        wxIMAGELIST_DRAW_TRANSPARENT
                       );
        }

        dc.DrawText( item.GetText(),
                     xAligned + EXTRA_WIDTH, h / 2 - hLabel / 2 ); //HEADER_OFFSET_Y + EXTRA_HEIGHT );

        x += wCol;
    }

    // Fill in what's missing to the right of the columns, otherwise we will
    // leave an unpainted area when columns are removed (and it looks better)
    if ( x < w )
    {
        wxRendererNative::Get().DrawHeaderButton
                                (
                                    this,
                                    dc,
                                    wxRect(x, HEADER_OFFSET_Y, w - x, h),
                                    0
                                );
    }
}

void wxListHeaderWindow::DrawCurrent()
{
#if 1
    m_owner->SetColumnWidth( m_column, m_currentX - m_minX );
#else
    int x1 = m_currentX;
    int y1 = 0;
    m_owner->ClientToScreen( &x1, &y1 );

    int x2 = m_currentX;
    int y2 = 0;
    m_owner->GetClientSize( NULL, &y2 );
    m_owner->ClientToScreen( &x2, &y2 );

    wxScreenDC dc;
    dc.SetLogicalFunction( wxINVERT );
    dc.SetPen( wxPen( *wxBLACK, 2, wxSOLID ) );
    dc.SetBrush( *wxTRANSPARENT_BRUSH );

    AdjustDC(dc);

    dc.DrawLine( x1, y1, x2, y2 );

    dc.SetLogicalFunction( wxCOPY );

    dc.SetPen( wxNullPen );
    dc.SetBrush( wxNullBrush );
#endif
}

void wxListHeaderWindow::OnMouse( wxMouseEvent &event )
{
    // we want to work with logical coords
    int x;
    m_owner->CalcUnscrolledPosition(event.GetX(), 0, &x, NULL);
    int y = event.GetY();

    if (m_isDragging)
    {
        SendListEvent(wxEVT_COMMAND_LIST_COL_DRAGGING, event.GetPosition());

        // we don't draw the line beyond our window, but we allow dragging it
        // there
        int w = 0;
        GetClientSize( &w, NULL );
        m_owner->CalcUnscrolledPosition(w, 0, &w, NULL);
        w -= 6;

        // erase the line if it was drawn
        if ( m_currentX < w )
            DrawCurrent();

        if (event.ButtonUp())
        {
            ReleaseMouse();
            m_isDragging = false;
            m_dirty = true;
            m_owner->SetColumnWidth( m_column, m_currentX - m_minX );
            SendListEvent(wxEVT_COMMAND_LIST_COL_END_DRAG, event.GetPosition());
        }
        else
        {
            if (x > m_minX + 7)
                m_currentX = x;
            else
                m_currentX = m_minX + 7;

            // draw in the new location
            if ( m_currentX < w )
                DrawCurrent();
        }
    }
    else // not dragging
    {
        m_minX = 0;
        bool hit_border = false;

        // end of the current column
        int xpos = 0;

        // find the column where this event occurred
        int col,
            countCol = m_owner->GetColumnCount();
        for (col = 0; col < countCol; col++)
        {
            xpos += m_owner->GetColumnWidth( col );
            m_column = col;

            if ( (abs(x-xpos) < 3) && (y < 22) )
            {
                // near the column border
                hit_border = true;
                break;
            }

            if ( x < xpos )
            {
                // inside the column
                break;
            }

            m_minX = xpos;
        }

        if ( col == countCol )
            m_column = -1;

        if (event.LeftDown() || event.RightUp())
        {
            if (hit_border && event.LeftDown())
            {
                if ( SendListEvent(wxEVT_COMMAND_LIST_COL_BEGIN_DRAG,
                                   event.GetPosition()) )
                {
                    m_isDragging = true;
                    m_currentX = x;
                    CaptureMouse();
                    DrawCurrent();
                }
                //else: column resizing was vetoed by the user code
            }
            else // click on a column
            {
                // record the selected state of the columns
                if (event.LeftDown())
                {
                    for (int i=0; i < m_owner->GetColumnCount(); i++)
                    {
                        wxListItem colItem;
                        m_owner->GetColumn(i, colItem);
                        long state = colItem.GetState();
                        if (i == m_column)
                            colItem.SetState(state | wxLIST_STATE_SELECTED);
                        else
                            colItem.SetState(state & ~wxLIST_STATE_SELECTED);
                        m_owner->SetColumn(i, colItem);
                    }
                }

                SendListEvent( event.LeftDown()
                                    ? wxEVT_COMMAND_LIST_COL_CLICK
                                    : wxEVT_COMMAND_LIST_COL_RIGHT_CLICK,
                                event.GetPosition());
            }
        }
        else if (event.Moving())
        {
            bool setCursor;
            if (hit_border)
            {
                setCursor = m_currentCursor == wxSTANDARD_CURSOR;
                m_currentCursor = m_resizeCursor;
            }
            else
            {
                setCursor = m_currentCursor != wxSTANDARD_CURSOR;
                m_currentCursor = wxSTANDARD_CURSOR;
            }

            if ( setCursor )
                SetCursor(*m_currentCursor);
        }
    }
}

void wxListHeaderWindow::OnSetFocus( wxFocusEvent &WXUNUSED(event) )
{
    m_owner->SetFocus();
    m_owner->Update();
}

bool wxListHeaderWindow::SendListEvent(wxEventType type, const wxPoint& pos)
{
    wxWindow *parent = GetParent();
    wxListEvent le( type, parent->GetId() );
    le.SetEventObject( parent );
    le.m_pointDrag = pos;

    // the position should be relative to the parent window, not
    // this one for compatibility with MSW and common sense: the
    // user code doesn't know anything at all about this header
    // window, so why should it get positions relative to it?
    le.m_pointDrag.y -= GetSize().y;

    le.m_col = m_column;
    return !parent->GetEventHandler()->ProcessEvent( le ) || le.IsAllowed();
}

//-----------------------------------------------------------------------------
// wxListRenameTimer (internal)
//-----------------------------------------------------------------------------

wxListRenameTimer::wxListRenameTimer( wxListMainWindow *owner )
{
    m_owner = owner;
}

void wxListRenameTimer::Notify()
{
    m_owner->OnRenameTimer();
}

//-----------------------------------------------------------------------------
// wxListTextCtrlWrapper (internal)
//-----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxListTextCtrlWrapper, wxEvtHandler)
    EVT_CHAR           (wxListTextCtrlWrapper::OnChar)
    EVT_KEY_UP         (wxListTextCtrlWrapper::OnKeyUp)
    EVT_KILL_FOCUS     (wxListTextCtrlWrapper::OnKillFocus)
END_EVENT_TABLE()

wxListTextCtrlWrapper::wxListTextCtrlWrapper(wxListMainWindow *owner,
                                             wxTextCtrl *text,
                                             size_t itemEdit)
              : m_startValue(owner->GetItemText(itemEdit)),
                m_itemEdited(itemEdit)
{
    m_owner = owner;
    m_text = text;
    m_finished = false;
    m_aboutToFinish = false;

    wxRect rectLabel = owner->GetLineLabelRect(itemEdit);

    m_owner->CalcScrolledPosition(rectLabel.x, rectLabel.y,
                                  &rectLabel.x, &rectLabel.y);

    m_text->Create(owner, wxID_ANY, m_startValue,
                   wxPoint(rectLabel.x-4,rectLabel.y-4),
                   wxSize(rectLabel.width+11,rectLabel.height+8));
    m_text->SetFocus();

    m_text->PushEventHandler(this);
}

void wxListTextCtrlWrapper::Finish()
{
    if ( !m_finished )
    {
        m_finished = true;

        m_text->RemoveEventHandler(this);
        m_owner->FinishEditing(m_text);

        wxPendingDelete.Append( this );
    }
}

bool wxListTextCtrlWrapper::AcceptChanges()
{
    const wxString value = m_text->GetValue();

    // notice that we should always call OnRenameAccept() to generate the "end
    // label editing" event, even if the user hasn't really changed anything
    if ( !m_owner->OnRenameAccept(m_itemEdited, value) )
    {
        // vetoed by the user
        return false;
    }

    // accepted, do rename the item (unless nothing changed)
    if ( value != m_startValue )
        m_owner->SetItemText(m_itemEdited, value);

    return true;
}

void wxListTextCtrlWrapper::AcceptChangesAndFinish()
{
    m_aboutToFinish = true;

    // Notify the owner about the changes
    AcceptChanges();

    // Even if vetoed, close the control (consistent with MSW)
    Finish();
}

void wxListTextCtrlWrapper::OnChar( wxKeyEvent &event )
{
    switch ( event.m_keyCode )
    {
        case WXK_RETURN:
            AcceptChangesAndFinish();
            break;

        case WXK_ESCAPE:
            m_owner->OnRenameCancelled( m_itemEdited );
            Finish();
            break;

        default:
            event.Skip();
    }
}

void wxListTextCtrlWrapper::OnKeyUp( wxKeyEvent &event )
{
    if (m_finished)
    {
        event.Skip();
        return;
    }

    // auto-grow the textctrl:
    wxSize parentSize = m_owner->GetSize();
    wxPoint myPos = m_text->GetPosition();
    wxSize mySize = m_text->GetSize();
    int sx, sy;
    m_text->GetTextExtent(m_text->GetValue() + _T("MM"), &sx, &sy);
    if (myPos.x + sx > parentSize.x)
        sx = parentSize.x - myPos.x;
    if (mySize.x > sx)
        sx = mySize.x;
    m_text->SetSize(sx, wxDefaultCoord);

    event.Skip();
}

void wxListTextCtrlWrapper::OnKillFocus( wxFocusEvent &event )
{
    if ( !m_finished && !m_aboutToFinish )
    {
        if ( !AcceptChanges() )
            m_owner->OnRenameCancelled( m_itemEdited );

        Finish();
    }

    // We must let the native text control handle focus
    event.Skip();
}

//-----------------------------------------------------------------------------
//  wxListMainWindow
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxListMainWindow,wxScrolledWindow)

BEGIN_EVENT_TABLE(wxListMainWindow,wxScrolledWindow)
  EVT_PAINT          (wxListMainWindow::OnPaint)
  EVT_MOUSE_EVENTS   (wxListMainWindow::OnMouse)
  EVT_CHAR           (wxListMainWindow::OnChar)
  EVT_KEY_DOWN       (wxListMainWindow::OnKeyDown)
  EVT_KEY_UP         (wxListMainWindow::OnKeyUp)
  EVT_SET_FOCUS      (wxListMainWindow::OnSetFocus)
  EVT_KILL_FOCUS     (wxListMainWindow::OnKillFocus)
  EVT_SCROLLWIN      (wxListMainWindow::OnScroll)
  EVT_CHILD_FOCUS    (wxListMainWindow::OnChildFocus)
END_EVENT_TABLE()

void wxListMainWindow::Init()
{
    m_dirty = true;
    m_countVirt = 0;
    m_lineFrom =
    m_lineTo = (size_t)-1;
    m_linesPerPage = 0;

    m_headerWidth =
    m_lineHeight = 0;

    m_small_image_list = (wxImageList *) NULL;
    m_normal_image_list = (wxImageList *) NULL;

    m_small_spacing = 30;
    m_normal_spacing = 40;

    m_hasFocus = false;
    m_dragCount = 0;
    m_isCreated = false;

    m_lastOnSame = false;
    m_renameTimer = new wxListRenameTimer( this );
    m_textctrlWrapper = NULL;

    m_current =
    m_lineLastClicked =
    m_lineSelectSingleOnUp =
    m_lineBeforeLastClicked = (size_t)-1;

    m_freezeCount = 0;
}

wxListMainWindow::wxListMainWindow()
{
    Init();

    m_highlightBrush =
    m_highlightUnfocusedBrush = (wxBrush *) NULL;
}

wxListMainWindow::wxListMainWindow( wxWindow *parent,
                                    wxWindowID id,
                                    const wxPoint& pos,
                                    const wxSize& size,
                                    long style,
                                    const wxString &name )
                : wxScrolledWindow( parent, id, pos, size,
                                    style | wxHSCROLL | wxVSCROLL, name )
{
    Init();

    m_highlightBrush = new wxBrush
                         (
                            wxSystemSettings::GetColour
                            (
                                wxSYS_COLOUR_HIGHLIGHT
                            ),
                            wxSOLID
                         );

    m_highlightUnfocusedBrush = new wxBrush
                              (
                                 wxSystemSettings::GetColour
                                 (
                                     wxSYS_COLOUR_BTNSHADOW
                                 ),
                                 wxSOLID
                              );

    SetScrollbars( 0, 0, 0, 0, 0, 0 );

    wxVisualAttributes attr = wxGenericListCtrl::GetClassDefaultAttributes();
    SetOwnForegroundColour( attr.colFg );
    SetOwnBackgroundColour( attr.colBg );
    if (!m_hasFont)
        SetOwnFont( attr.font );
}

wxListMainWindow::~wxListMainWindow()
{
    DoDeleteAllItems();
    WX_CLEAR_LIST(wxListHeaderDataList, m_columns);
    WX_CLEAR_ARRAY(m_aColWidths);

    delete m_highlightBrush;
    delete m_highlightUnfocusedBrush;
    delete m_renameTimer;
}

void wxListMainWindow::CacheLineData(size_t line)
{
    wxGenericListCtrl *listctrl = GetListCtrl();

    wxListLineData *ld = GetDummyLine();

    size_t countCol = GetColumnCount();
    for ( size_t col = 0; col < countCol; col++ )
    {
        ld->SetText(col, listctrl->OnGetItemText(line, col));
        ld->SetImage(col, listctrl->OnGetItemColumnImage(line, col));
    }

    ld->SetAttr(listctrl->OnGetItemAttr(line));
}

wxListLineData *wxListMainWindow::GetDummyLine() const
{
    wxASSERT_MSG( !IsEmpty(), _T("invalid line index") );
    wxASSERT_MSG( IsVirtual(), _T("GetDummyLine() shouldn't be called") );

    wxListMainWindow *self = wxConstCast(this, wxListMainWindow);

    // we need to recreate the dummy line if the number of columns in the
    // control changed as it would have the incorrect number of fields
    // otherwise
    if ( !m_lines.IsEmpty() &&
            m_lines[0].m_items.GetCount() != (size_t)GetColumnCount() )
    {
        self->m_lines.Clear();
    }

    if ( m_lines.IsEmpty() )
    {
        wxListLineData *line = new wxListLineData(self);
        self->m_lines.Add(line);

        // don't waste extra memory -- there never going to be anything
        // else/more in this array
        self->m_lines.Shrink();
    }

    return &m_lines[0];
}

// ----------------------------------------------------------------------------
// line geometry (report mode only)
// ----------------------------------------------------------------------------

wxCoord wxListMainWindow::GetLineHeight() const
{
    // we cache the line height as calling GetTextExtent() is slow
    if ( !m_lineHeight )
    {
        wxListMainWindow *self = wxConstCast(this, wxListMainWindow);

        wxClientDC dc( self );
        dc.SetFont( GetFont() );

        wxCoord y;
        dc.GetTextExtent(_T("H"), NULL, &y);

        if ( m_small_image_list && m_small_image_list->GetImageCount() )
        {
            int iw = 0, ih = 0;
            m_small_image_list->GetSize(0, iw, ih);
            y = wxMax(y, ih);
        }

        y += EXTRA_HEIGHT;
        self->m_lineHeight = y + LINE_SPACING;
    }

    return m_lineHeight;
}

wxCoord wxListMainWindow::GetLineY(size_t line) const
{
    wxASSERT_MSG( InReportView(), _T("only works in report mode") );

    return LINE_SPACING + line * GetLineHeight();
}

wxRect wxListMainWindow::GetLineRect(size_t line) const
{
    if ( !InReportView() )
        return GetLine(line)->m_gi->m_rectAll;

    wxRect rect;
    rect.x = HEADER_OFFSET_X;
    rect.y = GetLineY(line);
    rect.width = GetHeaderWidth();
    rect.height = GetLineHeight();

    return rect;
}

wxRect wxListMainWindow::GetLineLabelRect(size_t line) const
{
    if ( !InReportView() )
        return GetLine(line)->m_gi->m_rectLabel;

    int image_x = 0;
    wxListLineData *data = GetLine(line);
    wxListItemDataList::compatibility_iterator node = data->m_items.GetFirst();
    if (node)
    {
        wxListItemData *item = node->GetData();
        if ( item->HasImage() )
        {
            int ix, iy;
            GetImageSize( item->GetImage(), ix, iy );
            image_x = 3 + ix + IMAGE_MARGIN_IN_REPORT_MODE;
        }
    }

    wxRect rect;
    rect.x = image_x + HEADER_OFFSET_X;
    rect.y = GetLineY(line);
    rect.width = GetColumnWidth(0) - image_x;
    rect.height = GetLineHeight();

    return rect;
}

wxRect wxListMainWindow::GetLineIconRect(size_t line) const
{
    if ( !InReportView() )
        return GetLine(line)->m_gi->m_rectIcon;

    wxListLineData *ld = GetLine(line);
    wxASSERT_MSG( ld->HasImage(), _T("should have an image") );

    wxRect rect;
    rect.x = HEADER_OFFSET_X;
    rect.y = GetLineY(line);
    GetImageSize(ld->GetImage(), rect.width, rect.height);

    return rect;
}

wxRect wxListMainWindow::GetLineHighlightRect(size_t line) const
{
    return InReportView() ? GetLineRect(line)
                          : GetLine(line)->m_gi->m_rectHighlight;
}

long wxListMainWindow::HitTestLine(size_t line, int x, int y) const
{
    wxASSERT_MSG( line < GetItemCount(), _T("invalid line in HitTestLine") );

    wxListLineData *ld = GetLine(line);

    if ( ld->HasImage() && GetLineIconRect(line).Contains(x, y) )
        return wxLIST_HITTEST_ONITEMICON;

    // VS: Testing for "ld->HasText() || InReportView()" instead of
    //     "ld->HasText()" is needed to make empty lines in report view
    //     possible
    if ( ld->HasText() || InReportView() )
    {
        wxRect rect = InReportView() ? GetLineRect(line)
                                     : GetLineLabelRect(line);

        if ( rect.Contains(x, y) )
            return wxLIST_HITTEST_ONITEMLABEL;
    }

    return 0;
}

// ----------------------------------------------------------------------------
// highlight (selection) handling
// ----------------------------------------------------------------------------

bool wxListMainWindow::IsHighlighted(size_t line) const
{
    if ( IsVirtual() )
    {
        return m_selStore.IsSelected(line);
    }
    else // !virtual
    {
        wxListLineData *ld = GetLine(line);
        wxCHECK_MSG( ld, false, _T("invalid index in IsHighlighted") );

        return ld->IsHighlighted();
    }
}

void wxListMainWindow::HighlightLines( size_t lineFrom,
                                       size_t lineTo,
                                       bool highlight )
{
    if ( IsVirtual() )
    {
        wxArrayInt linesChanged;
        if ( !m_selStore.SelectRange(lineFrom, lineTo, highlight,
                                     &linesChanged) )
        {
            // meny items changed state, refresh everything
            RefreshLines(lineFrom, lineTo);
        }
        else // only a few items changed state, refresh only them
        {
            size_t count = linesChanged.GetCount();
            for ( size_t n = 0; n < count; n++ )
            {
                RefreshLine(linesChanged[n]);
            }
        }
    }
    else // iterate over all items in non report view
    {
        for ( size_t line = lineFrom; line <= lineTo; line++ )
        {
            if ( HighlightLine(line, highlight) )
                RefreshLine(line);
        }
    }
}

bool wxListMainWindow::HighlightLine( size_t line, bool highlight )
{
    bool changed;

    if ( IsVirtual() )
    {
        changed = m_selStore.SelectItem(line, highlight);
    }
    else // !virtual
    {
        wxListLineData *ld = GetLine(line);
        wxCHECK_MSG( ld, false, _T("invalid index in HighlightLine") );

        changed = ld->Highlight(highlight);
    }

    if ( changed )
    {
        SendNotify( line, highlight ? wxEVT_COMMAND_LIST_ITEM_SELECTED
                                    : wxEVT_COMMAND_LIST_ITEM_DESELECTED );
    }

    return changed;
}

void wxListMainWindow::RefreshLine( size_t line )
{
    if ( InReportView() )
    {
        size_t visibleFrom, visibleTo;
        GetVisibleLinesRange(&visibleFrom, &visibleTo);

        if ( line < visibleFrom || line > visibleTo )
            return;
    }

    wxRect rect = GetLineRect(line);

    CalcScrolledPosition( rect.x, rect.y, &rect.x, &rect.y );
    RefreshRect( rect );
}

void wxListMainWindow::RefreshLines( size_t lineFrom, size_t lineTo )
{
    // we suppose that they are ordered by caller
    wxASSERT_MSG( lineFrom <= lineTo, _T("indices in disorder") );

    wxASSERT_MSG( lineTo < GetItemCount(), _T("invalid line range") );

    if ( InReportView() )
    {
        size_t visibleFrom, visibleTo;
        GetVisibleLinesRange(&visibleFrom, &visibleTo);

        if ( lineFrom < visibleFrom )
            lineFrom = visibleFrom;
        if ( lineTo > visibleTo )
            lineTo = visibleTo;

        wxRect rect;
        rect.x = 0;
        rect.y = GetLineY(lineFrom);
        rect.width = GetClientSize().x;
        rect.height = GetLineY(lineTo) - rect.y + GetLineHeight();

        CalcScrolledPosition( rect.x, rect.y, &rect.x, &rect.y );
        RefreshRect( rect );
    }
    else // !report
    {
        // TODO: this should be optimized...
        for ( size_t line = lineFrom; line <= lineTo; line++ )
        {
            RefreshLine(line);
        }
    }
}

void wxListMainWindow::RefreshAfter( size_t lineFrom )
{
    if ( InReportView() )
    {
        size_t visibleFrom, visibleTo;
        GetVisibleLinesRange(&visibleFrom, &visibleTo);

        if ( lineFrom < visibleFrom )
            lineFrom = visibleFrom;
        else if ( lineFrom > visibleTo )
            return;

        wxRect rect;
        rect.x = 0;
        rect.y = GetLineY(lineFrom);
        CalcScrolledPosition( rect.x, rect.y, &rect.x, &rect.y );

        wxSize size = GetClientSize();
        rect.width = size.x;

        // refresh till the bottom of the window
        rect.height = size.y - rect.y;

        RefreshRect( rect );
    }
    else // !report
    {
        // TODO: how to do it more efficiently?
        m_dirty = true;
    }
}

void wxListMainWindow::RefreshSelected()
{
    if ( IsEmpty() )
        return;

    size_t from, to;
    if ( InReportView() )
    {
        GetVisibleLinesRange(&from, &to);
    }
    else // !virtual
    {
        from = 0;
        to = GetItemCount() - 1;
    }

    if ( HasCurrent() && m_current >= from && m_current <= to )
        RefreshLine(m_current);

    for ( size_t line = from; line <= to; line++ )
    {
        // NB: the test works as expected even if m_current == -1
        if ( line != m_current && IsHighlighted(line) )
            RefreshLine(line);
    }
}

void wxListMainWindow::Freeze()
{
    m_freezeCount++;
}

void wxListMainWindow::Thaw()
{
    wxCHECK_RET( m_freezeCount > 0, _T("thawing unfrozen list control?") );

    if ( --m_freezeCount == 0 )
        Refresh();
}

void wxListMainWindow::OnPaint( wxPaintEvent &WXUNUSED(event) )
{
    // Note: a wxPaintDC must be constructed even if no drawing is
    // done (a Windows requirement).
    wxPaintDC dc( this );

    if ( IsEmpty() || m_freezeCount )
        // nothing to draw or not the moment to draw it
        return;

    if ( m_dirty )
        // delay the repainting until we calculate all the items positions
        return;

    PrepareDC( dc );

    int dev_x, dev_y;
    CalcScrolledPosition( 0, 0, &dev_x, &dev_y );

    dc.SetFont( GetFont() );

    if ( InReportView() )
    {
        int lineHeight = GetLineHeight();

        size_t visibleFrom, visibleTo;
        GetVisibleLinesRange(&visibleFrom, &visibleTo);

        wxRect rectLine;
        int xOrig = dc.LogicalToDeviceX( 0 );
        int yOrig = dc.LogicalToDeviceY( 0 );

        // tell the caller cache to cache the data
        if ( IsVirtual() )
        {
            wxListEvent evCache(wxEVT_COMMAND_LIST_CACHE_HINT,
                                GetParent()->GetId());
            evCache.SetEventObject( GetParent() );
            evCache.m_oldItemIndex = visibleFrom;
            evCache.m_itemIndex = visibleTo;
            GetParent()->GetEventHandler()->ProcessEvent( evCache );
        }

        for ( size_t line = visibleFrom; line <= visibleTo; line++ )
        {
            rectLine = GetLineRect(line);


            if ( !IsExposed(rectLine.x + xOrig, rectLine.y + yOrig,
                            rectLine.width, rectLine.height) )
            {
                // don't redraw unaffected lines to avoid flicker
                continue;
            }

            GetLine(line)->DrawInReportMode( &dc,
                                             rectLine,
                                             GetLineHighlightRect(line),
                                             IsHighlighted(line) );
        }

        if ( HasFlag(wxLC_HRULES) )
        {
            wxPen pen(GetRuleColour(), 1, wxSOLID);
            wxSize clientSize = GetClientSize();

            size_t i = visibleFrom;
            if (i == 0) i = 1; // Don't draw the first one
            for ( ; i <= visibleTo; i++ )
            {
                dc.SetPen(pen);
                dc.SetBrush( *wxTRANSPARENT_BRUSH );
                dc.DrawLine(0 - dev_x, i * lineHeight,
                            clientSize.x - dev_x, i * lineHeight);
            }

            // Draw last horizontal rule
            if ( visibleTo == GetItemCount() - 1 )
            {
                dc.SetPen( pen );
                dc.SetBrush( *wxTRANSPARENT_BRUSH );
                dc.DrawLine(0 - dev_x, (m_lineTo + 1) * lineHeight,
                            clientSize.x - dev_x , (m_lineTo + 1) * lineHeight );
            }
        }

        // Draw vertical rules if required
        if ( HasFlag(wxLC_VRULES) && !IsEmpty() )
        {
            wxPen pen(GetRuleColour(), 1, wxSOLID);
            wxRect firstItemRect, lastItemRect;

            GetItemRect(visibleFrom, firstItemRect);
            GetItemRect(visibleTo, lastItemRect);
            int x = firstItemRect.GetX();
            dc.SetPen(pen);
            dc.SetBrush(* wxTRANSPARENT_BRUSH);

            for (int col = 0; col < GetColumnCount(); col++)
            {
                int colWidth = GetColumnWidth(col);
                x += colWidth;
                int x_pos = x - dev_x;
                if (col < GetColumnCount()-1) x_pos -= 2;
                dc.DrawLine(x_pos, firstItemRect.GetY() - 1 - dev_y,
                            x_pos, lastItemRect.GetBottom() + 1 - dev_y);
            }
        }
    }
    else // !report
    {
        size_t count = GetItemCount();
        for ( size_t i = 0; i < count; i++ )
        {
            GetLine(i)->Draw( &dc );
        }
    }

#ifndef __WXMAC__
    // Don't draw rect outline under Mac at all.
    if ( HasCurrent() )
    {
        if ( m_hasFocus )
        {
            wxRect rect( GetLineHighlightRect( m_current ) );
#ifndef __WXGTK20__
            dc.SetPen( *wxBLACK_PEN );
            dc.SetBrush( *wxTRANSPARENT_BRUSH );
            dc.DrawRectangle( rect );
#else
            wxRendererNative::Get().DrawItemSelectionRect( this, dc, rect, wxCONTROL_CURRENT|wxCONTROL_FOCUSED );

#endif
        }
    }
#endif
}

void wxListMainWindow::HighlightAll( bool on )
{
    if ( IsSingleSel() )
    {
        wxASSERT_MSG( !on, _T("can't do this in a single selection control") );

        // we just have one item to turn off
        if ( HasCurrent() && IsHighlighted(m_current) )
        {
            HighlightLine(m_current, false);
            RefreshLine(m_current);
        }
    }
    else // multi selection
    {
        if ( !IsEmpty() )
            HighlightLines(0, GetItemCount() - 1, on);
    }
}

void wxListMainWindow::OnChildFocus(wxChildFocusEvent& WXUNUSED(event))
{
    // Do nothing here.  This prevents the default handler in wxScrolledWindow
    // from needlessly scrolling the window when the edit control is
    // dismissed.  See ticket #9563.
}

void wxListMainWindow::SendNotify( size_t line,
                                   wxEventType command,
                                   const wxPoint& point )
{
    wxListEvent le( command, GetParent()->GetId() );
    le.SetEventObject( GetParent() );

    le.m_itemIndex = line;

    // set only for events which have position
    if ( point != wxDefaultPosition )
        le.m_pointDrag = point;

    // don't try to get the line info for virtual list controls: the main
    // program has it anyhow and if we did it would result in accessing all
    // the lines, even those which are not visible now and this is precisely
    // what we're trying to avoid
    if ( !IsVirtual() )
    {
        if ( line != (size_t)-1 )
        {
            GetLine(line)->GetItem( 0, le.m_item );
        }
        //else: this happens for wxEVT_COMMAND_LIST_ITEM_FOCUSED event
    }
    //else: there may be no more such item

    GetParent()->GetEventHandler()->ProcessEvent( le );
}

void wxListMainWindow::ChangeCurrent(size_t current)
{
    m_current = current;

    // as the current item changed, we shouldn't start editing it when the
    // "slow click" timer expires as the click happened on another item
    if ( m_renameTimer->IsRunning() )
        m_renameTimer->Stop();

    SendNotify(current, wxEVT_COMMAND_LIST_ITEM_FOCUSED);
}

wxTextCtrl *wxListMainWindow::EditLabel(long item, wxClassInfo* textControlClass)
{
    wxCHECK_MSG( (item >= 0) && ((size_t)item < GetItemCount()), NULL,
                 wxT("wrong index in wxGenericListCtrl::EditLabel()") );

    wxASSERT_MSG( textControlClass->IsKindOf(CLASSINFO(wxTextCtrl)),
                 wxT("EditLabel() needs a text control") );

    size_t itemEdit = (size_t)item;

    wxListEvent le( wxEVT_COMMAND_LIST_BEGIN_LABEL_EDIT, GetParent()->GetId() );
    le.SetEventObject( GetParent() );
    le.m_itemIndex = item;
    wxListLineData *data = GetLine(itemEdit);
    wxCHECK_MSG( data, NULL, _T("invalid index in EditLabel()") );
    data->GetItem( 0, le.m_item );

    if ( GetParent()->GetEventHandler()->ProcessEvent( le ) && !le.IsAllowed() )
    {
        // vetoed by user code
        return NULL;
    }

    // We have to call this here because the label in question might just have
    // been added and no screen update taken place.
    if ( m_dirty )
    {
        wxSafeYield();

        // Pending events dispatched by wxSafeYield might have changed the item
        // count
        if ( (size_t)item >= GetItemCount() )
            return NULL;
    }

    wxTextCtrl * const text = (wxTextCtrl *)textControlClass->CreateObject();
    m_textctrlWrapper = new wxListTextCtrlWrapper(this, text, item);
    return m_textctrlWrapper->GetText();
}

void wxListMainWindow::OnRenameTimer()
{
    wxCHECK_RET( HasCurrent(), wxT("unexpected rename timer") );

    EditLabel( m_current );
}

bool wxListMainWindow::OnRenameAccept(size_t itemEdit, const wxString& value)
{
    wxListEvent le( wxEVT_COMMAND_LIST_END_LABEL_EDIT, GetParent()->GetId() );
    le.SetEventObject( GetParent() );
    le.m_itemIndex = itemEdit;

    wxListLineData *data = GetLine(itemEdit);

    wxCHECK_MSG( data, false, _T("invalid index in OnRenameAccept()") );

    data->GetItem( 0, le.m_item );
    le.m_item.m_text = value;
    return !GetParent()->GetEventHandler()->ProcessEvent( le ) ||
                le.IsAllowed();
}

void wxListMainWindow::OnRenameCancelled(size_t itemEdit)
{
    // let owner know that the edit was cancelled
    wxListEvent le( wxEVT_COMMAND_LIST_END_LABEL_EDIT, GetParent()->GetId() );

    le.SetEditCanceled(true);

    le.SetEventObject( GetParent() );
    le.m_itemIndex = itemEdit;

    wxListLineData *data = GetLine(itemEdit);
    wxCHECK_RET( data, _T("invalid index in OnRenameCancelled()") );

    data->GetItem( 0, le.m_item );
    GetEventHandler()->ProcessEvent( le );
}

void wxListMainWindow::OnMouse( wxMouseEvent &event )
{

#ifdef __WXMAC__
    // On wxMac we can't depend on the EVT_KILL_FOCUS event to properly
    // shutdown the edit control when the mouse is clicked elsewhere on the
    // listctrl because the order of events is different (or something like
    // that), so explicitly end the edit if it is active.
    if ( event.LeftDown() && m_textctrlWrapper )
        m_textctrlWrapper->AcceptChangesAndFinish();
#endif // __WXMAC__

    if ( event.LeftDown() )
        SetFocusIgnoringChildren();

    event.SetEventObject( GetParent() );
    if ( GetParent()->GetEventHandler()->ProcessEvent( event) )
        return;

    if (event.GetEventType() == wxEVT_MOUSEWHEEL)
    {
        // let the base handle mouse wheel events.
        event.Skip();
        return;
    }

    if ( !HasCurrent() || IsEmpty() )
    {
        if (event.RightDown())
        {
            SendNotify( (size_t)-1, wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, event.GetPosition() );
            // Allow generation of context menu event
            event.Skip();
        }
        return;
    }

    if (m_dirty)
        return;

    if ( !(event.Dragging() || event.ButtonDown() || event.LeftUp() ||
        event.ButtonDClick()) )
        return;

    int x = event.GetX();
    int y = event.GetY();
    CalcUnscrolledPosition( x, y, &x, &y );

    // where did we hit it (if we did)?
    long hitResult = 0;

    size_t count = GetItemCount(),
           current;

    if ( InReportView() )
    {
        current = y / GetLineHeight();
        if ( current < count )
            hitResult = HitTestLine(current, x, y);
    }
    else // !report
    {
        // TODO: optimize it too! this is less simple than for report view but
        //       enumerating all items is still not a way to do it!!
        for ( current = 0; current < count; current++ )
        {
            hitResult = HitTestLine(current, x, y);
            if ( hitResult )
                break;
        }
    }

    if (event.Dragging())
    {
        if (m_dragCount == 0)
        {
            // we have to report the raw, physical coords as we want to be
            // able to call HitTest(event.m_pointDrag) from the user code to
            // get the item being dragged
            m_dragStart = event.GetPosition();
        }

        m_dragCount++;

        if (m_dragCount != 3)
            return;

        int command = event.RightIsDown() ? wxEVT_COMMAND_LIST_BEGIN_RDRAG
                                          : wxEVT_COMMAND_LIST_BEGIN_DRAG;

        wxListEvent le( command, GetParent()->GetId() );
        le.SetEventObject( GetParent() );
        le.m_itemIndex = m_lineLastClicked;
        le.m_pointDrag = m_dragStart;
        GetParent()->GetEventHandler()->ProcessEvent( le );

        return;
    }
    else
    {
        m_dragCount = 0;
    }

    if ( !hitResult )
    {
        // outside of any item
        if (event.RightDown())
        {
            SendNotify( (size_t) -1, wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, event.GetPosition() );

            wxContextMenuEvent evtCtx(
                wxEVT_CONTEXT_MENU,
                GetParent()->GetId(),
                ClientToScreen(event.GetPosition()));
            evtCtx.SetEventObject(GetParent());
            GetParent()->GetEventHandler()->ProcessEvent(evtCtx);
        }
        else
        {
            // reset the selection and bail out
            HighlightAll(false);
        }

        return;
    }

    bool forceClick = false;
    if (event.ButtonDClick())
    {
        if ( m_renameTimer->IsRunning() )
            m_renameTimer->Stop();

        m_lastOnSame = false;

        if ( current == m_lineLastClicked )
        {
            SendNotify( current, wxEVT_COMMAND_LIST_ITEM_ACTIVATED );

            return;
        }
        else
        {
            // The first click was on another item, so don't interpret this as
            // a double click, but as a simple click instead
            forceClick = true;
        }
    }

    if (event.LeftUp())
    {
        if (m_lineSelectSingleOnUp != (size_t)-1)
        {
            // select single line
            HighlightAll( false );
            ReverseHighlight(m_lineSelectSingleOnUp);
        }

        if (m_lastOnSame)
        {
            if ((current == m_current) &&
                (hitResult == wxLIST_HITTEST_ONITEMLABEL) &&
                HasFlag(wxLC_EDIT_LABELS) )
            {
                if (InReportView())
                {
                    wxRect label = GetLineLabelRect( current );
                    if (label.Contains( x, y ))
                        m_renameTimer->Start( 250, true );

                }
                else
                    m_renameTimer->Start( 250, true );
            }
        }

        m_lastOnSame = false;
        m_lineSelectSingleOnUp = (size_t)-1;
    }
    else
    {
        // This is necessary, because after a DnD operation in
        // from and to ourself, the up event is swallowed by the
        // DnD code. So on next non-up event (which means here and
        // now) m_lineSelectSingleOnUp should be reset.
        m_lineSelectSingleOnUp = (size_t)-1;
    }
    if (event.RightDown())
    {
        m_lineBeforeLastClicked = m_lineLastClicked;
        m_lineLastClicked = current;

        // If the item is already selected, do not update the selection.
        // Multi-selections should not be cleared if a selected item is clicked.
        if (!IsHighlighted(current))
        {
            HighlightAll(false);
            ChangeCurrent(current);
            ReverseHighlight(m_current);
        }

        SendNotify( current, wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, event.GetPosition() );

        wxContextMenuEvent evtCtx(
                wxEVT_CONTEXT_MENU,
                GetParent()->GetId(),
                ClientToScreen(event.GetPosition()));
        evtCtx.SetEventObject(GetParent());
        GetParent()->GetEventHandler()->ProcessEvent(evtCtx);
    }
    else if (event.MiddleDown())
    {
        SendNotify( current, wxEVT_COMMAND_LIST_ITEM_MIDDLE_CLICK );
    }
    else if ( event.LeftDown() || forceClick )
    {
        m_lineBeforeLastClicked = m_lineLastClicked;
        m_lineLastClicked = current;

        size_t oldCurrent = m_current;
        bool oldWasSelected = IsHighlighted(m_current);

        bool cmdModifierDown = event.CmdDown();
        if ( IsSingleSel() || !(cmdModifierDown || event.ShiftDown()) )
        {
            if ( IsSingleSel() || !IsHighlighted(current) )
            {
                HighlightAll( false );

                ChangeCurrent(current);

                ReverseHighlight(m_current);
            }
            else // multi sel & current is highlighted & no mod keys
            {
                m_lineSelectSingleOnUp = current;
                ChangeCurrent(current); // change focus
            }
        }
        else // multi sel & either ctrl or shift is down
        {
            if (cmdModifierDown)
            {
                ChangeCurrent(current);

                ReverseHighlight(m_current);
            }
            else if (event.ShiftDown())
            {
                ChangeCurrent(current);

                size_t lineFrom = oldCurrent,
                       lineTo = current;

                if ( lineTo < lineFrom )
                {
                    lineTo = lineFrom;
                    lineFrom = m_current;
                }

                HighlightLines(lineFrom, lineTo);
            }
            else // !ctrl, !shift
            {
                // test in the enclosing if should make it impossible
                wxFAIL_MSG( _T("how did we get here?") );
            }
        }

        if (m_current != oldCurrent)
            RefreshLine( oldCurrent );

        // forceClick is only set if the previous click was on another item
        m_lastOnSame = !forceClick && (m_current == oldCurrent) && oldWasSelected;
    }
}

void wxListMainWindow::MoveToItem(size_t item)
{
    if ( item == (size_t)-1 )
        return;

    wxRect rect = GetLineRect(item);

    int client_w, client_h;
    GetClientSize( &client_w, &client_h );

    const int hLine = GetLineHeight();

    int view_x = SCROLL_UNIT_X * GetScrollPos( wxHORIZONTAL );
    int view_y = hLine * GetScrollPos( wxVERTICAL );

    if ( InReportView() )
    {
        // the next we need the range of lines shown it might be different,
        // so recalculate it
        ResetVisibleLinesRange();

        if (rect.y < view_y)
            Scroll( -1, rect.y / hLine );
        if (rect.y + rect.height + 5 > view_y + client_h)
            Scroll( -1, (rect.y + rect.height - client_h + hLine) / hLine );

#ifdef __WXMAC__
        // At least on Mac the visible lines value will get reset inside of
        // Scroll *before* it actually scrolls the window because of the
        // Update() that happens there, so it will still have the wrong value.
        // So let's reset it again and wait for it to be recalculated in the
        // next paint event.  I would expect this problem to show up in wxGTK
        // too but couldn't duplicate it there.  Perhaps the order of events
        // is different...  --Robin
        ResetVisibleLinesRange();
#endif
    }
    else // !report
    {
        int sx = -1,
            sy = -1;

        if (rect.x-view_x < 5)
            sx = (rect.x - 5) / SCROLL_UNIT_X;
        if (rect.x + rect.width - 5 > view_x + client_w)
            sx = (rect.x + rect.width - client_w + SCROLL_UNIT_X) / SCROLL_UNIT_X;

        if (rect.y-view_y < 5)
            sy = (rect.y - 5) / hLine;
        if (rect.y + rect.height - 5 > view_y + client_h)
            sy = (rect.y + rect.height - client_h + hLine) / hLine;

        Scroll(sx, sy);
    }
}

bool wxListMainWindow::ScrollList(int WXUNUSED(dx), int dy)
{
    if ( !InReportView() )
    {
        // TODO: this should work in all views but is not implemented now
        return false;
    }

    size_t top, bottom;
    GetVisibleLinesRange(&top, &bottom);

    if ( bottom == (size_t)-1 )
        return 0;

    ResetVisibleLinesRange();

    int hLine = GetLineHeight();

    Scroll(-1, top + dy / hLine);

#ifdef __WXMAC__
    // see comment in MoveToItem() for why we do this
    ResetVisibleLinesRange();
#endif

    return true;
}

// ----------------------------------------------------------------------------
// keyboard handling
// ----------------------------------------------------------------------------

void wxListMainWindow::OnArrowChar(size_t newCurrent, const wxKeyEvent& event)
{
    wxCHECK_RET( newCurrent < (size_t)GetItemCount(),
                 _T("invalid item index in OnArrowChar()") );

    size_t oldCurrent = m_current;

    // in single selection we just ignore Shift as we can't select several
    // items anyhow
    if ( event.ShiftDown() && !IsSingleSel() )
    {
        ChangeCurrent(newCurrent);

        // refresh the old focus to remove it
        RefreshLine( oldCurrent );

        // select all the items between the old and the new one
        if ( oldCurrent > newCurrent )
        {
            newCurrent = oldCurrent;
            oldCurrent = m_current;
        }

        HighlightLines(oldCurrent, newCurrent);
    }
    else // !shift
    {
        // all previously selected items are unselected unless ctrl is held
        // in a multiselection control
        if ( !event.ControlDown() || IsSingleSel() )
            HighlightAll(false);

        ChangeCurrent(newCurrent);

        // refresh the old focus to remove it
        RefreshLine( oldCurrent );

        // in single selection mode we must always have a selected item
        if ( !event.ControlDown() || IsSingleSel() )
            HighlightLine( m_current, true );
    }

    RefreshLine( m_current );

    MoveToFocus();
}

void wxListMainWindow::OnKeyDown( wxKeyEvent &event )
{
    wxWindow *parent = GetParent();

    // propagate the key event upwards
    wxKeyEvent ke(event);
    if (parent->GetEventHandler()->ProcessEvent( ke ))
        return;

    event.Skip();
}

void wxListMainWindow::OnKeyUp( wxKeyEvent &event )
{
    wxWindow *parent = GetParent();

    // propagate the key event upwards
    wxKeyEvent ke(event);
    ke.SetEventObject( parent );
    if (parent->GetEventHandler()->ProcessEvent( ke ))
        return;

    event.Skip();
}

void wxListMainWindow::OnChar( wxKeyEvent &event )
{
    wxWindow *parent = GetParent();

    // send a list_key event up
    if ( HasCurrent() )
    {
        wxListEvent le( wxEVT_COMMAND_LIST_KEY_DOWN, GetParent()->GetId() );
        le.m_itemIndex = m_current;
        GetLine(m_current)->GetItem( 0, le.m_item );
        le.m_code = event.GetKeyCode();
        le.SetEventObject( parent );
        parent->GetEventHandler()->ProcessEvent( le );
    }

    // propagate the char event upwards
    wxKeyEvent ke(event);
    if (parent->GetEventHandler()->ProcessEvent( ke ))
        return;

    if (event.GetKeyCode() == WXK_TAB)
    {
        wxNavigationKeyEvent nevent;
        nevent.SetWindowChange( event.ControlDown() );
        nevent.SetDirection( !event.ShiftDown() );
        nevent.SetEventObject( GetParent()->GetParent() );
        nevent.SetCurrentFocus( m_parent );
        if (GetParent()->GetParent()->GetEventHandler()->ProcessEvent( nevent ))
            return;
    }

    // no item -> nothing to do
    if (!HasCurrent())
    {
        event.Skip();
        return;
    }

    // don't use m_linesPerPage directly as it might not be computed yet
    const int pageSize = GetCountPerPage();
    wxCHECK_RET( pageSize, _T("should have non zero page size") );

    if (GetLayoutDirection() == wxLayout_RightToLeft)
    {
        if (event.GetKeyCode() == WXK_RIGHT)
            event.m_keyCode = WXK_LEFT;
        else if (event.GetKeyCode() == WXK_LEFT)
            event.m_keyCode = WXK_RIGHT;
    }

    switch ( event.GetKeyCode() )
    {
        case WXK_UP:
            if ( m_current > 0 )
                OnArrowChar( m_current - 1, event );
            break;

        case WXK_DOWN:
            if ( m_current < (size_t)GetItemCount() - 1 )
                OnArrowChar( m_current + 1, event );
            break;

        case WXK_END:
            if (!IsEmpty())
                OnArrowChar( GetItemCount() - 1, event );
            break;

        case WXK_HOME:
            if (!IsEmpty())
                OnArrowChar( 0, event );
            break;

        case WXK_PAGEUP:
            {
                int steps = InReportView() ? pageSize - 1
                                           : m_current % pageSize;

                int index = m_current - steps;
                if (index < 0)
                    index = 0;

                OnArrowChar( index, event );
            }
            break;

        case WXK_PAGEDOWN:
            {
                int steps = InReportView()
                                ? pageSize - 1
                                : pageSize - (m_current % pageSize) - 1;

                size_t index = m_current + steps;
                size_t count = GetItemCount();
                if ( index >= count )
                    index = count - 1;

                OnArrowChar( index, event );
            }
            break;

        case WXK_LEFT:
            if ( !InReportView() )
            {
                int index = m_current - pageSize;
                if (index < 0)
                    index = 0;

                OnArrowChar( index, event );
            }
            break;

        case WXK_RIGHT:
            if ( !InReportView() )
            {
                size_t index = m_current + pageSize;

                size_t count = GetItemCount();
                if ( index >= count )
                    index = count - 1;

                OnArrowChar( index, event );
            }
            break;

        case WXK_SPACE:
            if ( IsSingleSel() )
            {
                if ( event.ControlDown() )
                {
                    ReverseHighlight(m_current);
                }
                else // normal space press
                {
                    SendNotify( m_current, wxEVT_COMMAND_LIST_ITEM_ACTIVATED );
                }
            }
            else // multiple selection
            {
                ReverseHighlight(m_current);
            }
            break;

        case WXK_RETURN:
        case WXK_EXECUTE:
            SendNotify( m_current, wxEVT_COMMAND_LIST_ITEM_ACTIVATED );
            break;

        default:
            event.Skip();
    }
}

// ----------------------------------------------------------------------------
// focus handling
// ----------------------------------------------------------------------------

void wxListMainWindow::OnSetFocus( wxFocusEvent &WXUNUSED(event) )
{
    if ( GetParent() )
    {
        wxFocusEvent event( wxEVT_SET_FOCUS, GetParent()->GetId() );
        event.SetEventObject( GetParent() );
        if ( GetParent()->GetEventHandler()->ProcessEvent( event) )
            return;
    }

    // wxGTK sends us EVT_SET_FOCUS events even if we had never got
    // EVT_KILL_FOCUS before which means that we finish by redrawing the items
    // which are already drawn correctly resulting in horrible flicker - avoid
    // it
    if ( !m_hasFocus )
    {
        m_hasFocus = true;

        RefreshSelected();
    }
}

void wxListMainWindow::OnKillFocus( wxFocusEvent &WXUNUSED(event) )
{
    if ( GetParent() )
    {
        wxFocusEvent event( wxEVT_KILL_FOCUS, GetParent()->GetId() );
        event.SetEventObject( GetParent() );
        if ( GetParent()->GetEventHandler()->ProcessEvent( event) )
            return;
    }

    m_hasFocus = false;
    RefreshSelected();
}

void wxListMainWindow::DrawImage( int index, wxDC *dc, int x, int y )
{
    if ( HasFlag(wxLC_ICON) && (m_normal_image_list))
    {
        m_normal_image_list->Draw( index, *dc, x, y, wxIMAGELIST_DRAW_TRANSPARENT );
    }
    else if ( HasFlag(wxLC_SMALL_ICON) && (m_small_image_list))
    {
        m_small_image_list->Draw( index, *dc, x, y, wxIMAGELIST_DRAW_TRANSPARENT );
    }
    else if ( HasFlag(wxLC_LIST) && (m_small_image_list))
    {
        m_small_image_list->Draw( index, *dc, x, y, wxIMAGELIST_DRAW_TRANSPARENT );
    }
    else if ( InReportView() && (m_small_image_list))
    {
        m_small_image_list->Draw( index, *dc, x, y, wxIMAGELIST_DRAW_TRANSPARENT );
    }
}

void wxListMainWindow::GetImageSize( int index, int &width, int &height ) const
{
    if ( HasFlag(wxLC_ICON) && m_normal_image_list )
    {
        m_normal_image_list->GetSize( index, width, height );
    }
    else if ( HasFlag(wxLC_SMALL_ICON) && m_small_image_list )
    {
        m_small_image_list->GetSize( index, width, height );
    }
    else if ( HasFlag(wxLC_LIST) && m_small_image_list )
    {
        m_small_image_list->GetSize( index, width, height );
    }
    else if ( InReportView() && m_small_image_list )
    {
        m_small_image_list->GetSize( index, width, height );
    }
    else
    {
        width =
        height = 0;
    }
}

int wxListMainWindow::GetTextLength( const wxString &s ) const
{
    wxClientDC dc( wxConstCast(this, wxListMainWindow) );
    dc.SetFont( GetFont() );

    wxCoord lw;
    dc.GetTextExtent( s, &lw, NULL );

    return lw + AUTOSIZE_COL_MARGIN;
}

void wxListMainWindow::SetImageList( wxImageList *imageList, int which )
{
    m_dirty = true;

    // calc the spacing from the icon size
    int width = 0, height = 0;

    if ((imageList) && (imageList->GetImageCount()) )
        imageList->GetSize(0, width, height);

    if (which == wxIMAGE_LIST_NORMAL)
    {
        m_normal_image_list = imageList;
        m_normal_spacing = width + 8;
    }

    if (which == wxIMAGE_LIST_SMALL)
    {
        m_small_image_list = imageList;
        m_small_spacing = width + 14;
        m_lineHeight = 0;  // ensure that the line height will be recalc'd
    }
}

void wxListMainWindow::SetItemSpacing( int spacing, bool isSmall )
{
    m_dirty = true;
    if (isSmall)
        m_small_spacing = spacing;
    else
        m_normal_spacing = spacing;
}

int wxListMainWindow::GetItemSpacing( bool isSmall )
{
    return isSmall ? m_small_spacing : m_normal_spacing;
}

// ----------------------------------------------------------------------------
// columns
// ----------------------------------------------------------------------------

void wxListMainWindow::SetColumn( int col, wxListItem &item )
{
    wxListHeaderDataList::compatibility_iterator node = m_columns.Item( col );

    wxCHECK_RET( node, _T("invalid column index in SetColumn") );

    if ( item.m_width == wxLIST_AUTOSIZE_USEHEADER )
        item.m_width = GetTextLength( item.m_text );

    wxListHeaderData *column = node->GetData();
    column->SetItem( item );

    wxListHeaderWindow *headerWin = GetListCtrl()->m_headerWin;
    if ( headerWin )
        headerWin->m_dirty = true;

    m_dirty = true;

    // invalidate it as it has to be recalculated
    m_headerWidth = 0;
}

void wxListMainWindow::SetColumnWidth( int col, int width )
{
    wxCHECK_RET( col >= 0 && col < GetColumnCount(),
                 _T("invalid column index") );

    wxCHECK_RET( InReportView(),
                 _T("SetColumnWidth() can only be called in report mode.") );

    m_dirty = true;
    wxListHeaderWindow *headerWin = GetListCtrl()->m_headerWin;
    if ( headerWin )
        headerWin->m_dirty = true;

    wxListHeaderDataList::compatibility_iterator node = m_columns.Item( col );
    wxCHECK_RET( node, _T("no column?") );

    wxListHeaderData *column = node->GetData();

    size_t count = GetItemCount();

    if (width == wxLIST_AUTOSIZE_USEHEADER)
    {
        width = GetTextLength(column->GetText());
        width += 2*EXTRA_WIDTH;

        // check for column header's image availability
        const int image = column->GetImage();
        if ( image != -1 )
        {
            if ( m_small_image_list )
            {
                int ix = 0, iy = 0;
                m_small_image_list->GetSize(image, ix, iy);
                width += ix + HEADER_IMAGE_MARGIN_IN_REPORT_MODE;
            }
        }
    }
    else if ( width == wxLIST_AUTOSIZE )
    {
        if ( IsVirtual() )
        {
            // TODO: determine the max width somehow...
            width = WIDTH_COL_DEFAULT;
        }
        else // !virtual
        {
            wxClientDC dc(this);
            dc.SetFont( GetFont() );

            int max = AUTOSIZE_COL_MARGIN;

            //  if the cached column width isn't valid then recalculate it
            if (m_aColWidths.Item(col)->bNeedsUpdate)
            {
                for (size_t i = 0; i < count; i++)
                {
                    wxListLineData *line = GetLine( i );
                    wxListItemDataList::compatibility_iterator n = line->m_items.Item( col );

                    wxCHECK_RET( n, _T("no subitem?") );

                    wxListItemData *itemData = n->GetData();
                    wxListItem      item;

                    itemData->GetItem(item);
                    int itemWidth = GetItemWidthWithImage(&item);
                    if (itemWidth > max)
                        max = itemWidth;
                }

                m_aColWidths.Item(col)->bNeedsUpdate = false;
                m_aColWidths.Item(col)->nMaxWidth = max;
            }

            max = m_aColWidths.Item(col)->nMaxWidth;
            width = max + AUTOSIZE_COL_MARGIN;
        }
    }

    column->SetWidth( width );

    // invalidate it as it has to be recalculated
    m_headerWidth = 0;
}

int wxListMainWindow::GetHeaderWidth() const
{
    if ( !m_headerWidth )
    {
        wxListMainWindow *self = wxConstCast(this, wxListMainWindow);

        size_t count = GetColumnCount();
        for ( size_t col = 0; col < count; col++ )
        {
            self->m_headerWidth += GetColumnWidth(col);
        }
    }

    return m_headerWidth;
}

void wxListMainWindow::GetColumn( int col, wxListItem &item ) const
{
    wxListHeaderDataList::compatibility_iterator node = m_columns.Item( col );
    wxCHECK_RET( node, _T("invalid column index in GetColumn") );

    wxListHeaderData *column = node->GetData();
    column->GetItem( item );
}

int wxListMainWindow::GetColumnWidth( int col ) const
{
    wxListHeaderDataList::compatibility_iterator node = m_columns.Item( col );
    wxCHECK_MSG( node, 0, _T("invalid column index") );

    wxListHeaderData *column = node->GetData();
    return column->GetWidth();
}

// ----------------------------------------------------------------------------
// item state
// ----------------------------------------------------------------------------

void wxListMainWindow::SetItem( wxListItem &item )
{
    long id = item.m_itemId;
    wxCHECK_RET( id >= 0 && (size_t)id < GetItemCount(),
                 _T("invalid item index in SetItem") );

    if ( !IsVirtual() )
    {
        wxListLineData *line = GetLine((size_t)id);
        line->SetItem( item.m_col, item );

        // Set item state if user wants
        if ( item.m_mask & wxLIST_MASK_STATE )
            SetItemState( item.m_itemId, item.m_state, item.m_state );

        if (InReportView())
        {
            //  update the Max Width Cache if needed
            int width = GetItemWidthWithImage(&item);

            if (width > m_aColWidths.Item(item.m_col)->nMaxWidth)
                m_aColWidths.Item(item.m_col)->nMaxWidth = width;
        }
    }

    // update the item on screen
    wxRect rectItem;
    GetItemRect(id, rectItem);
    RefreshRect(rectItem);
}

void wxListMainWindow::SetItemStateAll(long state, long stateMask)
{
    if ( IsEmpty() )
        return;

    // first deal with selection
    if ( stateMask & wxLIST_STATE_SELECTED )
    {
        // set/clear select state
        if ( IsVirtual() )
        {
            // optimized version for virtual listctrl.
            m_selStore.SelectRange(0, GetItemCount() - 1, state == wxLIST_STATE_SELECTED);
            Refresh();
        }
        else if ( state & wxLIST_STATE_SELECTED )
        {
            const long count = GetItemCount();
            for( long i = 0; i <  count; i++ )
            {
                SetItemState( i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
            }

        }
        else
        {
            // clear for non virtual (somewhat optimized by using GetNextItem())
            long i = -1;
            while ( (i = GetNextItem(i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != -1 )
            {
                SetItemState( i, 0, wxLIST_STATE_SELECTED );
            }
        }
    }

    if ( HasCurrent() && (state == 0) && (stateMask & wxLIST_STATE_FOCUSED) )
    {
        // unfocus all: only one item can be focussed, so clearing focus for
        // all items is simply clearing focus of the focussed item.
        SetItemState(m_current, state, stateMask);
    }
    //(setting focus to all items makes no sense, so it is not handled here.)
}

void wxListMainWindow::SetItemState( long litem, long state, long stateMask )
{
    if ( litem == -1 )
    {
        SetItemStateAll(state, stateMask);
        return;
    }

    wxCHECK_RET( litem >= 0 && (size_t)litem < GetItemCount(),
                 _T("invalid list ctrl item index in SetItem") );

    size_t oldCurrent = m_current;
    size_t item = (size_t)litem;    // safe because of the check above

    // do we need to change the focus?
    if ( stateMask & wxLIST_STATE_FOCUSED )
    {
        if ( state & wxLIST_STATE_FOCUSED )
        {
            // don't do anything if this item is already focused
            if ( item != m_current )
            {
                ChangeCurrent(item);

                if ( oldCurrent != (size_t)-1 )
                {
                    if ( IsSingleSel() )
                    {
                        HighlightLine(oldCurrent, false);
                    }

                    RefreshLine(oldCurrent);
                }

                RefreshLine( m_current );
            }
        }
        else // unfocus
        {
            // don't do anything if this item is not focused
            if ( item == m_current )
            {
                ResetCurrent();

                if ( IsSingleSel() )
                {
                    // we must unselect the old current item as well or we
                    // might end up with more than one selected item in a
                    // single selection control
                    HighlightLine(oldCurrent, false);
                }

                RefreshLine( oldCurrent );
            }
        }
    }

    // do we need to change the selection state?
    if ( stateMask & wxLIST_STATE_SELECTED )
    {
        bool on = (state & wxLIST_STATE_SELECTED) != 0;

        if ( IsSingleSel() )
        {
            if ( on )
            {
                // selecting the item also makes it the focused one in the
                // single sel mode
                if ( m_current != item )
                {
                    ChangeCurrent(item);

                    if ( oldCurrent != (size_t)-1 )
                    {
                        HighlightLine( oldCurrent, false );
                        RefreshLine( oldCurrent );
                    }
                }
            }
            else // off
            {
                // only the current item may be selected anyhow
                if ( item != m_current )
                    return;
            }
        }

        if ( HighlightLine(item, on) )
        {
            RefreshLine(item);
        }
    }
}

int wxListMainWindow::GetItemState( long item, long stateMask ) const
{
    wxCHECK_MSG( item >= 0 && (size_t)item < GetItemCount(), 0,
                 _T("invalid list ctrl item index in GetItemState()") );

    int ret = wxLIST_STATE_DONTCARE;

    if ( stateMask & wxLIST_STATE_FOCUSED )
    {
        if ( (size_t)item == m_current )
            ret |= wxLIST_STATE_FOCUSED;
    }

    if ( stateMask & wxLIST_STATE_SELECTED )
    {
        if ( IsHighlighted(item) )
            ret |= wxLIST_STATE_SELECTED;
    }

    return ret;
}

void wxListMainWindow::GetItem( wxListItem &item ) const
{
    wxCHECK_RET( item.m_itemId >= 0 && (size_t)item.m_itemId < GetItemCount(),
                 _T("invalid item index in GetItem") );

    wxListLineData *line = GetLine((size_t)item.m_itemId);
    line->GetItem( item.m_col, item );

    // Get item state if user wants it
    if ( item.m_mask & wxLIST_MASK_STATE )
        item.m_state = GetItemState( item.m_itemId, wxLIST_STATE_SELECTED |
                                                 wxLIST_STATE_FOCUSED );
}

// ----------------------------------------------------------------------------
// item count
// ----------------------------------------------------------------------------

size_t wxListMainWindow::GetItemCount() const
{
    return IsVirtual() ? m_countVirt : m_lines.GetCount();
}

void wxListMainWindow::SetItemCount(long count)
{
    m_selStore.SetItemCount(count);
    m_countVirt = count;

    ResetVisibleLinesRange();

    // scrollbars must be reset
    m_dirty = true;
}

int wxListMainWindow::GetSelectedItemCount() const
{
    // deal with the quick case first
    if ( IsSingleSel() )
        return HasCurrent() ? IsHighlighted(m_current) : false;

    // virtual controls remmebers all its selections itself
    if ( IsVirtual() )
        return m_selStore.GetSelectedCount();

    // TODO: we probably should maintain the number of items selected even for
    //       non virtual controls as enumerating all lines is really slow...
    size_t countSel = 0;
    size_t count = GetItemCount();
    for ( size_t line = 0; line < count; line++ )
    {
        if ( GetLine(line)->IsHighlighted() )
            countSel++;
    }

    return countSel;
}

// ----------------------------------------------------------------------------
// item position/size
// ----------------------------------------------------------------------------

wxRect wxListMainWindow::GetViewRect() const
{
    wxASSERT_MSG( !HasFlag(wxLC_REPORT | wxLC_LIST),
                    _T("wxListCtrl::GetViewRect() only works in icon mode") );

    // we need to find the longest/tallest label
    wxCoord xMax = 0, yMax = 0;
    const int count = GetItemCount();
    if ( count )
    {
        for ( int i = 0; i < count; i++ )
        {
            // we need logical, not physical, coordinates here, so use
            // GetLineRect() instead of GetItemRect()
            wxRect r = GetLineRect(i);

            wxCoord x = r.GetRight(),
                    y = r.GetBottom();

            if ( x > xMax )
                xMax = x;
            if ( y > yMax )
                yMax = y;
        }
    }

    // some fudge needed to make it look prettier
    xMax += 2 * EXTRA_BORDER_X;
    yMax += 2 * EXTRA_BORDER_Y;

    // account for the scrollbars if necessary
    const wxSize sizeAll = GetClientSize();
    if ( xMax > sizeAll.x )
        yMax += wxSystemSettings::GetMetric(wxSYS_HSCROLL_Y);
    if ( yMax > sizeAll.y )
        xMax += wxSystemSettings::GetMetric(wxSYS_VSCROLL_X);

    return wxRect(0, 0, xMax, yMax);
}

void wxListMainWindow::GetItemRect( long index, wxRect &rect ) const
{
    wxCHECK_RET( index >= 0 && (size_t)index < GetItemCount(),
                 _T("invalid index in GetItemRect") );

    // ensure that we're laid out, otherwise we could return nonsense
    if ( m_dirty )
    {
        wxConstCast(this, wxListMainWindow)->
            RecalculatePositions(true /* no refresh */);
    }

    rect = GetLineRect((size_t)index);

    CalcScrolledPosition(rect.x, rect.y, &rect.x, &rect.y);
}

bool wxListMainWindow::GetItemPosition(long item, wxPoint& pos) const
{
    wxRect rect;
    GetItemRect(item, rect);

    pos.x = rect.x;
    pos.y = rect.y;

    return true;
}

// ----------------------------------------------------------------------------
// geometry calculation
// ----------------------------------------------------------------------------

void wxListMainWindow::RecalculatePositions(bool noRefresh)
{
    const int lineHeight = GetLineHeight();

    wxClientDC dc( this );
    dc.SetFont( GetFont() );

    const size_t count = GetItemCount();

    int iconSpacing;
    if ( HasFlag(wxLC_ICON) && m_normal_image_list )
        iconSpacing = m_normal_spacing;
    else if ( HasFlag(wxLC_SMALL_ICON) && m_small_image_list )
        iconSpacing = m_small_spacing;
    else
        iconSpacing = 0;

    // Note that we do not call GetClientSize() here but
    // GetSize() and subtract the border size for sunken
    // borders manually. This is technically incorrect,
    // but we need to know the client area's size WITHOUT
    // scrollbars here. Since we don't know if there are
    // any scrollbars, we use GetSize() instead. Another
    // solution would be to call SetScrollbars() here to
    // remove the scrollbars and call GetClientSize() then,
    // but this might result in flicker and - worse - will
    // reset the scrollbars to 0 which is not good at all
    // if you resize a dialog/window, but don't want to
    // reset the window scrolling. RR.
    // Furthermore, we actually do NOT subtract the border
    // width as 2 pixels is just the extra space which we
    // need around the actual content in the window. Other-
    // wise the text would e.g. touch the upper border. RR.
    int clientWidth,
        clientHeight;
    GetSize( &clientWidth, &clientHeight );

    if ( InReportView() )
    {
        // all lines have the same height and we scroll one line per step
        int entireHeight = count * lineHeight + LINE_SPACING;

        m_linesPerPage = clientHeight / lineHeight;

        ResetVisibleLinesRange();

        SetScrollbars( SCROLL_UNIT_X, lineHeight,
                       GetHeaderWidth() / SCROLL_UNIT_X,
                       (entireHeight + lineHeight - 1) / lineHeight,
                       GetScrollPos(wxHORIZONTAL),
                       GetScrollPos(wxVERTICAL),
                       true );
    }
    else // !report
    {
        // we have 3 different layout strategies: either layout all items
        // horizontally/vertically (wxLC_ALIGN_XXX styles explicitly given) or
        // to arrange them in top to bottom, left to right (don't ask me why
        // not the other way round...) order
        if ( HasFlag(wxLC_ALIGN_LEFT | wxLC_ALIGN_TOP) )
        {
            int x = EXTRA_BORDER_X;
            int y = EXTRA_BORDER_Y;

            wxCoord widthMax = 0;

            size_t i;
            for ( i = 0; i < count; i++ )
            {
                wxListLineData *line = GetLine(i);
                line->CalculateSize( &dc, iconSpacing );
                line->SetPosition( x, y, iconSpacing );

                wxSize sizeLine = GetLineSize(i);

                if ( HasFlag(wxLC_ALIGN_TOP) )
                {
                    if ( sizeLine.x > widthMax )
                        widthMax = sizeLine.x;

                    y += sizeLine.y;
                }
                else // wxLC_ALIGN_LEFT
                {
                    x += sizeLine.x + MARGIN_BETWEEN_ROWS;
                }
            }

            if ( HasFlag(wxLC_ALIGN_TOP) )
            {
                // traverse the items again and tweak their sizes so that they are
                // all the same in a row
                for ( i = 0; i < count; i++ )
                {
                    wxListLineData *line = GetLine(i);
                    line->m_gi->ExtendWidth(widthMax);
                }
            }

            SetScrollbars
            (
                SCROLL_UNIT_X,
                lineHeight,
                (x + SCROLL_UNIT_X) / SCROLL_UNIT_X,
                (y + lineHeight) / lineHeight,
                GetScrollPos( wxHORIZONTAL ),
                GetScrollPos( wxVERTICAL ),
                true
            );
        }
        else // "flowed" arrangement, the most complicated case
        {
            // at first we try without any scrollbars, if the items don't fit into
            // the window, we recalculate after subtracting the space taken by the
            // scrollbar

            int entireWidth = 0;

            for (int tries = 0; tries < 2; tries++)
            {
                entireWidth = 2 * EXTRA_BORDER_X;

                if (tries == 1)
                {
                    // Now we have decided that the items do not fit into the
                    // client area, so we need a scrollbar
                    entireWidth += SCROLL_UNIT_X;
                }

                int x = EXTRA_BORDER_X;
                int y = EXTRA_BORDER_Y;
                int maxWidthInThisRow = 0;

                m_linesPerPage = 0;
                int currentlyVisibleLines = 0;

                for (size_t i = 0; i < count; i++)
                {
                    currentlyVisibleLines++;
                    wxListLineData *line = GetLine( i );
                    line->CalculateSize( &dc, iconSpacing );
                    line->SetPosition( x, y, iconSpacing );

                    wxSize sizeLine = GetLineSize( i );

                    if ( maxWidthInThisRow < sizeLine.x )
                        maxWidthInThisRow = sizeLine.x;

                    y += sizeLine.y;
                    if (currentlyVisibleLines > m_linesPerPage)
                        m_linesPerPage = currentlyVisibleLines;

                    if ( y + sizeLine.y >= clientHeight )
                    {
                        currentlyVisibleLines = 0;
                        y = EXTRA_BORDER_Y;
                        maxWidthInThisRow += MARGIN_BETWEEN_ROWS;
                        x += maxWidthInThisRow;
                        entireWidth += maxWidthInThisRow;
                        maxWidthInThisRow = 0;
                    }

                    // We have reached the last item.
                    if ( i == count - 1 )
                        entireWidth += maxWidthInThisRow;

                    if ( (tries == 0) &&
                            (entireWidth + SCROLL_UNIT_X > clientWidth) )
                    {
                        clientHeight -= wxSystemSettings::
                                            GetMetric(wxSYS_HSCROLL_Y);
                        m_linesPerPage = 0;
                        break;
                    }

                    if ( i == count - 1 )
                        tries = 1;  // Everything fits, no second try required.
                }
            }

            SetScrollbars
            (
                SCROLL_UNIT_X,
                lineHeight,
                (entireWidth + SCROLL_UNIT_X) / SCROLL_UNIT_X,
                0,
                GetScrollPos( wxHORIZONTAL ),
                0,
                true
            );
        }
    }

    if ( !noRefresh )
    {
        // FIXME: why should we call it from here?
        UpdateCurrent();

        RefreshAll();
    }
}

void wxListMainWindow::RefreshAll()
{
    m_dirty = false;
    Refresh();

    wxListHeaderWindow *headerWin = GetListCtrl()->m_headerWin;
    if ( headerWin && headerWin->m_dirty )
    {
        headerWin->m_dirty = false;
        headerWin->Refresh();
    }
}

void wxListMainWindow::UpdateCurrent()
{
    if ( !HasCurrent() && !IsEmpty() )
        ChangeCurrent(0);
}

long wxListMainWindow::GetNextItem( long item,
                                    int WXUNUSED(geometry),
                                    int state ) const
{
    long ret = item,
         max = GetItemCount();
    wxCHECK_MSG( (ret == -1) || (ret < max), -1,
                 _T("invalid listctrl index in GetNextItem()") );

    // notice that we start with the next item (or the first one if item == -1)
    // and this is intentional to allow writing a simple loop to iterate over
    // all selected items
    ret++;
    if ( ret == max )
        // this is not an error because the index was OK initially,
        // just no such item
        return -1;

    if ( !state )
        // any will do
        return (size_t)ret;

    size_t count = GetItemCount();
    for ( size_t line = (size_t)ret; line < count; line++ )
    {
        if ( (state & wxLIST_STATE_FOCUSED) && (line == m_current) )
            return line;

        if ( (state & wxLIST_STATE_SELECTED) && IsHighlighted(line) )
            return line;
    }

    return -1;
}

// ----------------------------------------------------------------------------
// deleting stuff
// ----------------------------------------------------------------------------

void wxListMainWindow::DeleteItem( long lindex )
{
    size_t count = GetItemCount();

    wxCHECK_RET( (lindex >= 0) && ((size_t)lindex < count),
                 _T("invalid item index in DeleteItem") );

    size_t index = (size_t)lindex;

    // we don't need to adjust the index for the previous items
    if ( HasCurrent() && m_current >= index )
    {
        // if the current item is being deleted, we want the next one to
        // become selected - unless there is no next one - so don't adjust
        // m_current in this case
        if ( m_current != index || m_current == count - 1 )
            m_current--;
    }

    if ( InReportView() )
    {
        //  mark the Column Max Width cache as dirty if the items in the line
        //  we're deleting contain the Max Column Width
        wxListLineData * const line = GetLine(index);
        wxListItemDataList::compatibility_iterator n;
        wxListItemData *itemData;
        wxListItem      item;
        int             itemWidth;

        for (size_t i = 0; i < m_columns.GetCount(); i++)
        {
            n = line->m_items.Item( i );
            itemData = n->GetData();
            itemData->GetItem(item);

            itemWidth = GetItemWidthWithImage(&item);

            if (itemWidth >= m_aColWidths.Item(i)->nMaxWidth)
                m_aColWidths.Item(i)->bNeedsUpdate = true;
        }

        ResetVisibleLinesRange();
    }

    SendNotify( index, wxEVT_COMMAND_LIST_DELETE_ITEM, wxDefaultPosition );

    if ( IsVirtual() )
    {
        m_countVirt--;
        m_selStore.OnItemDelete(index);
    }
    else
    {
        m_lines.RemoveAt( index );
    }

    // we need to refresh the (vert) scrollbar as the number of items changed
    m_dirty = true;

    RefreshAfter(index);
}

void wxListMainWindow::DeleteColumn( int col )
{
    wxListHeaderDataList::compatibility_iterator node = m_columns.Item( col );

    wxCHECK_RET( node, wxT("invalid column index in DeleteColumn()") );

    m_dirty = true;
    delete node->GetData();
    m_columns.Erase( node );

    if ( !IsVirtual() )
    {
        // update all the items
        for ( size_t i = 0; i < m_lines.GetCount(); i++ )
        {
            wxListLineData * const line = GetLine(i);
            wxListItemDataList::compatibility_iterator n = line->m_items.Item( col );
            delete n->GetData();
            line->m_items.Erase(n);
        }
    }

    if ( InReportView() )   //  we only cache max widths when in Report View
    {
        delete m_aColWidths.Item(col);
        m_aColWidths.RemoveAt(col);
    }

    // invalidate it as it has to be recalculated
    m_headerWidth = 0;
}

void wxListMainWindow::DoDeleteAllItems()
{
    if ( IsEmpty() )
        // nothing to do - in particular, don't send the event
        return;

    ResetCurrent();

    // to make the deletion of all items faster, we don't send the
    // notifications for each item deletion in this case but only one event
    // for all of them: this is compatible with wxMSW and documented in
    // DeleteAllItems() description

    wxListEvent event( wxEVT_COMMAND_LIST_DELETE_ALL_ITEMS, GetParent()->GetId() );
    event.SetEventObject( GetParent() );
    GetParent()->GetEventHandler()->ProcessEvent( event );

    if ( IsVirtual() )
    {
        m_countVirt = 0;
        m_selStore.Clear();
    }

    if ( InReportView() )
    {
        ResetVisibleLinesRange();
        for (size_t i = 0; i < m_aColWidths.GetCount(); i++)
        {
            m_aColWidths.Item(i)->bNeedsUpdate = true;
        }
    }

    m_lines.Clear();
}

void wxListMainWindow::DeleteAllItems()
{
    DoDeleteAllItems();

    RecalculatePositions();
}

void wxListMainWindow::DeleteEverything()
{
    WX_CLEAR_LIST(wxListHeaderDataList, m_columns);
    WX_CLEAR_ARRAY(m_aColWidths);

    DeleteAllItems();
}

// ----------------------------------------------------------------------------
// scanning for an item
// ----------------------------------------------------------------------------

void wxListMainWindow::EnsureVisible( long index )
{
    wxCHECK_RET( index >= 0 && (size_t)index < GetItemCount(),
                 _T("invalid index in EnsureVisible") );

    // We have to call this here because the label in question might just have
    // been added and its position is not known yet
    if ( m_dirty )
        RecalculatePositions(true /* no refresh */);

    MoveToItem((size_t)index);
}

long wxListMainWindow::FindItem(long start, const wxString& str, bool partial )
{
    if (str.empty())
        return wxNOT_FOUND;

    long pos = start;
    wxString str_upper = str.Upper();
    if (pos < 0)
        pos = 0;

    size_t count = GetItemCount();
    for ( size_t i = (size_t)pos; i < count; i++ )
    {
        wxListLineData *line = GetLine(i);
        wxString line_upper = line->GetText(0).Upper();
        if (!partial)
        {
            if (line_upper == str_upper )
                return i;
        }
        else
        {
            if (line_upper.find(str_upper) == 0)
                return i;
        }
    }

    return wxNOT_FOUND;
}

long wxListMainWindow::FindItem(long start, wxUIntPtr data)
{
    long pos = start;
    if (pos < 0)
        pos = 0;

    size_t count = GetItemCount();
    for (size_t i = (size_t)pos; i < count; i++)
    {
        wxListLineData *line = GetLine(i);
        wxListItem item;
        line->GetItem( 0, item );
        if (item.m_data == data)
            return i;
    }

    return wxNOT_FOUND;
}

long wxListMainWindow::FindItem( const wxPoint& pt )
{
    size_t topItem;
    GetVisibleLinesRange( &topItem, NULL );

    wxPoint p;
    GetItemPosition( GetItemCount() - 1, p );
    if ( p.y == 0 )
        return topItem;

    long id = (long)floor( pt.y * double(GetItemCount() - topItem - 1) / p.y + topItem );
    if ( id >= 0 && id < (long)GetItemCount() )
        return id;

    return wxNOT_FOUND;
}

long wxListMainWindow::HitTest( int x, int y, int &flags ) const
{
    CalcUnscrolledPosition( x, y, &x, &y );

    size_t count = GetItemCount();

    if ( InReportView() )
    {
        size_t current = y / GetLineHeight();
        if ( current < count )
        {
            flags = HitTestLine(current, x, y);
            if ( flags )
                return current;
        }
    }
    else // !report
    {
        // TODO: optimize it too! this is less simple than for report view but
        //       enumerating all items is still not a way to do it!!
        for ( size_t current = 0; current < count; current++ )
        {
            flags = HitTestLine(current, x, y);
            if ( flags )
                return current;
        }
    }

    return wxNOT_FOUND;
}

// ----------------------------------------------------------------------------
// adding stuff
// ----------------------------------------------------------------------------

void wxListMainWindow::InsertItem( wxListItem &item )
{
    wxASSERT_MSG( !IsVirtual(), _T("can't be used with virtual control") );

    int count = GetItemCount();
    wxCHECK_RET( item.m_itemId >= 0, _T("invalid item index") );

    if (item.m_itemId > count)
        item.m_itemId = count;

    size_t id = item.m_itemId;

    m_dirty = true;

    if ( InReportView() )
    {
        ResetVisibleLinesRange();

        // calculate the width of the item and adjust the max column width
        wxColWidthInfo *pWidthInfo = m_aColWidths.Item(item.GetColumn());
        int width = GetItemWidthWithImage(&item);
        item.SetWidth(width);
        if (width > pWidthInfo->nMaxWidth)
            pWidthInfo->nMaxWidth = width;
    }

    wxListLineData *line = new wxListLineData(this);

    line->SetItem( item.m_col, item );

    m_lines.Insert( line, id );

    m_dirty = true;

    // If an item is selected at or below the point of insertion, we need to
    // increment the member variables because the current row's index has gone
    // up by one
    if ( HasCurrent() && m_current >= id )
        m_current++;

    SendNotify(id, wxEVT_COMMAND_LIST_INSERT_ITEM);

    RefreshLines(id, GetItemCount() - 1);
}

void wxListMainWindow::InsertColumn( long col, wxListItem &item )
{
    m_dirty = true;
    if ( InReportView() )
    {
        if (item.m_width == wxLIST_AUTOSIZE_USEHEADER)
            item.m_width = GetTextLength( item.m_text );

        wxListHeaderData *column = new wxListHeaderData( item );
        wxColWidthInfo *colWidthInfo = new wxColWidthInfo();

        bool insert = (col >= 0) && ((size_t)col < m_columns.GetCount());
        if ( insert )
        {
            wxListHeaderDataList::compatibility_iterator
                node = m_columns.Item( col );
            m_columns.Insert( node, column );
            m_aColWidths.Insert( colWidthInfo, col );
        }
        else
        {
            m_columns.Append( column );
            m_aColWidths.Add( colWidthInfo );
        }

        if ( !IsVirtual() )
        {
            // update all the items
            for ( size_t i = 0; i < m_lines.GetCount(); i++ )
            {
                wxListLineData * const line = GetLine(i);
                wxListItemData * const data = new wxListItemData(this);
                if ( insert )
                    line->m_items.Insert(col, data);
                else
                    line->m_items.Append(data);
            }
        }

        // invalidate it as it has to be recalculated
        m_headerWidth = 0;
    }
}

int wxListMainWindow::GetItemWidthWithImage(wxListItem * item)
{
    int width = 0;
    wxClientDC dc(this);

    dc.SetFont( GetFont() );

    if (item->GetImage() != -1)
    {
        int ix, iy;
        GetImageSize( item->GetImage(), ix, iy );
        width += ix + 5;
    }

    if (!item->GetText().empty())
    {
        wxCoord w;
        dc.GetTextExtent( item->GetText(), &w, NULL );
        width += w;
    }

    return width;
}

// ----------------------------------------------------------------------------
// sorting
// ----------------------------------------------------------------------------

wxListCtrlCompare list_ctrl_compare_func_2;
long              list_ctrl_compare_data;

int LINKAGEMODE list_ctrl_compare_func_1( wxListLineData **arg1, wxListLineData **arg2 )
{
    wxListLineData *line1 = *arg1;
    wxListLineData *line2 = *arg2;
    wxListItem item;
    line1->GetItem( 0, item );
    wxUIntPtr data1 = item.m_data;
    line2->GetItem( 0, item );
    wxUIntPtr data2 = item.m_data;
    return list_ctrl_compare_func_2( data1, data2, list_ctrl_compare_data );
}

void wxListMainWindow::SortItems( wxListCtrlCompare fn, long data )
{
    // selections won't make sense any more after sorting the items so reset
    // them
    HighlightAll(false);
    ResetCurrent();

    list_ctrl_compare_func_2 = fn;
    list_ctrl_compare_data = data;
    m_lines.Sort( list_ctrl_compare_func_1 );
    m_dirty = true;
}

// ----------------------------------------------------------------------------
// scrolling
// ----------------------------------------------------------------------------

void wxListMainWindow::OnScroll(wxScrollWinEvent& event)
{
    // FIXME
#if ( defined(__WXGTK__) || defined(__WXMAC__) ) && !defined(__WXUNIVERSAL__)
    wxScrolledWindow::OnScroll(event);
#else
    HandleOnScroll( event );
#endif

    // update our idea of which lines are shown when we redraw the window the
    // next time
    ResetVisibleLinesRange();

    if ( event.GetOrientation() == wxHORIZONTAL && HasHeader() )
    {
        wxGenericListCtrl* lc = GetListCtrl();
        wxCHECK_RET( lc, _T("no listctrl window?") );

        lc->m_headerWin->Refresh();
        lc->m_headerWin->Update();
    }
}

int wxListMainWindow::GetCountPerPage() const
{
    if ( !m_linesPerPage )
    {
        wxConstCast(this, wxListMainWindow)->
            m_linesPerPage = GetClientSize().y / GetLineHeight();
    }

    return m_linesPerPage;
}

void wxListMainWindow::GetVisibleLinesRange(size_t *from, size_t *to)
{
    wxASSERT_MSG( InReportView(), _T("this is for report mode only") );

    if ( m_lineFrom == (size_t)-1 )
    {
        size_t count = GetItemCount();
        if ( count )
        {
            m_lineFrom = GetScrollPos(wxVERTICAL);

            // this may happen if SetScrollbars() hadn't been called yet
            if ( m_lineFrom >= count )
                m_lineFrom = count - 1;

            // we redraw one extra line but this is needed to make the redrawing
            // logic work when there is a fractional number of lines on screen
            m_lineTo = m_lineFrom + m_linesPerPage;
            if ( m_lineTo >= count )
                m_lineTo = count - 1;
        }
        else // empty control
        {
            m_lineFrom = 0;
            m_lineTo = (size_t)-1;
        }
    }

    wxASSERT_MSG( IsEmpty() ||
                  (m_lineFrom <= m_lineTo && m_lineTo < GetItemCount()),
                  _T("GetVisibleLinesRange() returns incorrect result") );

    if ( from )
        *from = m_lineFrom;
    if ( to )
        *to = m_lineTo;
}

// -------------------------------------------------------------------------------------
// wxGenericListCtrl
// -------------------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxGenericListCtrl, wxControl)

BEGIN_EVENT_TABLE(wxGenericListCtrl,wxControl)
  EVT_SIZE(wxGenericListCtrl::OnSize)
END_EVENT_TABLE()

wxGenericListCtrl::wxGenericListCtrl()
{
    m_imageListNormal = (wxImageList *) NULL;
    m_imageListSmall = (wxImageList *) NULL;
    m_imageListState = (wxImageList *) NULL;

    m_ownsImageListNormal =
    m_ownsImageListSmall =
    m_ownsImageListState = false;

    m_mainWin = (wxListMainWindow*) NULL;
    m_headerWin = (wxListHeaderWindow*) NULL;
    m_headerHeight = 0;
}

wxGenericListCtrl::~wxGenericListCtrl()
{
    if (m_ownsImageListNormal)
        delete m_imageListNormal;
    if (m_ownsImageListSmall)
        delete m_imageListSmall;
    if (m_ownsImageListState)
        delete m_imageListState;
}

void wxGenericListCtrl::CalculateAndSetHeaderHeight()
{
    if ( m_headerWin )
    {
#ifdef __WXMAC__
        SInt32 h;
        GetThemeMetric( kThemeMetricListHeaderHeight, &h );
#else
        // we use 'g' to get the descent, too
        int w, h, d;
        m_headerWin->GetTextExtent(wxT("Hg"), &w, &h, &d);
        h += d + 2 * HEADER_OFFSET_Y + EXTRA_HEIGHT;
#endif

        // only update if changed
        if ( h != m_headerHeight )
        {
            m_headerHeight = h;

            if ( HasHeader() )
                ResizeReportView(true);
            else    //why is this needed if it doesn't have a header?
                m_headerWin->SetSize(m_headerWin->GetSize().x, m_headerHeight);
        }
    }
}

void wxGenericListCtrl::CreateHeaderWindow()
{
    m_headerWin = new wxListHeaderWindow
                      (
                        this, wxID_ANY, m_mainWin,
                        wxPoint(0,0),
                        wxSize(GetClientSize().x, m_headerHeight),
                        wxTAB_TRAVERSAL
                      );
    CalculateAndSetHeaderHeight();
}

bool wxGenericListCtrl::Create(wxWindow *parent,
                        wxWindowID id,
                        const wxPoint &pos,
                        const wxSize &size,
                        long style,
                        const wxValidator &validator,
                        const wxString &name)
{
    m_imageListNormal =
    m_imageListSmall =
    m_imageListState = (wxImageList *) NULL;
    m_ownsImageListNormal =
    m_ownsImageListSmall =
    m_ownsImageListState = false;

    m_mainWin = (wxListMainWindow*) NULL;
    m_headerWin = (wxListHeaderWindow*) NULL;

    m_headerHeight = 0;

    if ( !(style & wxLC_MASK_TYPE) )
    {
        style = style | wxLC_LIST;
    }

    // add more styles here that should only appear
    // in the main window
    unsigned long only_main_window_style = wxALWAYS_SHOW_SB;

    if ( !wxControl::Create( parent, id, pos, size, style & ~only_main_window_style, validator, name ) )
        return false;

    // don't create the inner window with the border
    style &= ~wxBORDER_MASK;

    m_mainWin = new wxListMainWindow( this, wxID_ANY, wxPoint(0, 0), size, style );

#ifdef  __WXMAC_CARBON__
    // Human Interface Guidelines ask us for a special font in this case
    if ( GetWindowVariant() == wxWINDOW_VARIANT_NORMAL )
    {
        wxFont font;
        font.MacCreateThemeFont( kThemeViewsFont );
        SetFont( font );
    }
#endif

    if ( InReportView() )
    {
        CreateHeaderWindow();

#ifdef  __WXMAC_CARBON__
        if (m_headerWin)
        {
            wxFont font;
            font.MacCreateThemeFont( kThemeSmallSystemFont );
            m_headerWin->SetFont( font );
            CalculateAndSetHeaderHeight();
        }
#endif

        if ( HasFlag(wxLC_NO_HEADER) )
            // VZ: why do we create it at all then?
            m_headerWin->Show( false );
    }

    SetInitialSize(size);

    return true;
}

void wxGenericListCtrl::SetSingleStyle( long style, bool add )
{
    wxASSERT_MSG( !(style & wxLC_VIRTUAL),
                  _T("wxLC_VIRTUAL can't be [un]set") );

    long flag = GetWindowStyle();

    if (add)
    {
        if (style & wxLC_MASK_TYPE)
            flag &= ~(wxLC_MASK_TYPE | wxLC_VIRTUAL);
        if (style & wxLC_MASK_ALIGN)
            flag &= ~wxLC_MASK_ALIGN;
        if (style & wxLC_MASK_SORT)
            flag &= ~wxLC_MASK_SORT;
    }

    if (add)
        flag |= style;
    else
        flag &= ~style;

    // some styles can be set without recreating everything (as happens in
    // SetWindowStyleFlag() which calls wxListMainWindow::DeleteEverything())
    if ( !(style & ~(wxLC_HRULES | wxLC_VRULES)) )
    {
        Refresh();
        wxWindow::SetWindowStyleFlag(flag);
    }
    else
    {
        SetWindowStyleFlag( flag );
    }
}

void wxGenericListCtrl::SetWindowStyleFlag( long flag )
{
    if (m_mainWin)
    {
        m_mainWin->DeleteEverything();

        // has the header visibility changed?
        bool hasHeader = HasHeader();
        bool willHaveHeader = (flag & wxLC_REPORT) && !(flag & wxLC_NO_HEADER);

        if ( hasHeader != willHaveHeader )
        {
            // toggle it
            if ( hasHeader )
            {
                if ( m_headerWin )
                {
                    // don't delete, just hide, as we can reuse it later
                    m_headerWin->Show(false);
                }
                //else: nothing to do
            }
            else // must show header
            {
                if (!m_headerWin)
                {
                    CreateHeaderWindow();
                }
                else // already have it, just show
                {
                    m_headerWin->Show( true );
                }
            }

            ResizeReportView(willHaveHeader);
        }
    }

    wxWindow::SetWindowStyleFlag( flag );
}

bool wxGenericListCtrl::GetColumn(int col, wxListItem &item) const
{
    m_mainWin->GetColumn( col, item );
    return true;
}

bool wxGenericListCtrl::SetColumn( int col, wxListItem& item )
{
    m_mainWin->SetColumn( col, item );
    return true;
}

int wxGenericListCtrl::GetColumnWidth( int col ) const
{
    return m_mainWin->GetColumnWidth( col );
}

bool wxGenericListCtrl::SetColumnWidth( int col, int width )
{
    m_mainWin->SetColumnWidth( col, width );
    return true;
}

int wxGenericListCtrl::GetCountPerPage() const
{
  return m_mainWin->GetCountPerPage();  // different from Windows ?
}

bool wxGenericListCtrl::GetItem( wxListItem &info ) const
{
    m_mainWin->GetItem( info );
    return true;
}

bool wxGenericListCtrl::SetItem( wxListItem &info )
{
    m_mainWin->SetItem( info );
    return true;
}

long wxGenericListCtrl::SetItem( long index, int col, const wxString& label, int imageId )
{
    wxListItem info;
    info.m_text = label;
    info.m_mask = wxLIST_MASK_TEXT;
    info.m_itemId = index;
    info.m_col = col;
    if ( imageId > -1 )
    {
        info.m_image = imageId;
        info.m_mask |= wxLIST_MASK_IMAGE;
    }

    m_mainWin->SetItem(info);
    return true;
}

int wxGenericListCtrl::GetItemState( long item, long stateMask ) const
{
    return m_mainWin->GetItemState( item, stateMask );
}

bool wxGenericListCtrl::SetItemState( long item, long state, long stateMask )
{
    m_mainWin->SetItemState( item, state, stateMask );
    return true;
}

bool
wxGenericListCtrl::SetItemImage( long item, int image, int WXUNUSED(selImage) )
{
    return SetItemColumnImage(item, 0, image);
}

bool
wxGenericListCtrl::SetItemColumnImage( long item, long column, int image )
{
    wxListItem info;
    info.m_image = image;
    info.m_mask = wxLIST_MASK_IMAGE;
    info.m_itemId = item;
    info.m_col = column;
    m_mainWin->SetItem( info );
    return true;
}

wxString wxGenericListCtrl::GetItemText( long item ) const
{
    return m_mainWin->GetItemText(item);
}

void wxGenericListCtrl::SetItemText( long item, const wxString& str )
{
    m_mainWin->SetItemText(item, str);
}

wxUIntPtr wxGenericListCtrl::GetItemData( long item ) const
{
    wxListItem info;
    info.m_mask = wxLIST_MASK_DATA;
    info.m_itemId = item;
    m_mainWin->GetItem( info );
    return info.m_data;
}

bool wxGenericListCtrl::SetItemPtrData( long item, wxUIntPtr data )
{
    wxListItem info;
    info.m_mask = wxLIST_MASK_DATA;
    info.m_itemId = item;
    info.m_data = data;
    m_mainWin->SetItem( info );
    return true;
}

bool wxGenericListCtrl::SetItemData(long item, long data)
{
    return SetItemPtrData(item, data);
}

wxRect wxGenericListCtrl::GetViewRect() const
{
    return m_mainWin->GetViewRect();
}

bool wxGenericListCtrl::GetItemRect( long item, wxRect &rect, int WXUNUSED(code) ) const
{
    m_mainWin->GetItemRect( item, rect );
    if ( m_mainWin->HasHeader() )
        rect.y += m_headerHeight + 1;
    return true;
}

bool wxGenericListCtrl::GetItemPosition( long item, wxPoint& pos ) const
{
    m_mainWin->GetItemPosition( item, pos );
    return true;
}

bool wxGenericListCtrl::SetItemPosition( long WXUNUSED(item), const wxPoint& WXUNUSED(pos) )
{
    return 0;
}

int wxGenericListCtrl::GetItemCount() const
{
    return m_mainWin->GetItemCount();
}

int wxGenericListCtrl::GetColumnCount() const
{
    return m_mainWin->GetColumnCount();
}

void wxGenericListCtrl::SetItemSpacing( int spacing, bool isSmall )
{
    m_mainWin->SetItemSpacing( spacing, isSmall );
}

wxSize wxGenericListCtrl::GetItemSpacing() const
{
    const int spacing = m_mainWin->GetItemSpacing(HasFlag(wxLC_SMALL_ICON));

    return wxSize(spacing, spacing);
}

#if WXWIN_COMPATIBILITY_2_6
int wxGenericListCtrl::GetItemSpacing( bool isSmall ) const
{
    return m_mainWin->GetItemSpacing( isSmall );
}
#endif // WXWIN_COMPATIBILITY_2_6

void wxGenericListCtrl::SetItemTextColour( long item, const wxColour &col )
{
    wxListItem info;
    info.m_itemId = item;
    info.SetTextColour( col );
    m_mainWin->SetItem( info );
}

wxColour wxGenericListCtrl::GetItemTextColour( long item ) const
{
    wxListItem info;
    info.m_itemId = item;
    m_mainWin->GetItem( info );
    return info.GetTextColour();
}

void wxGenericListCtrl::SetItemBackgroundColour( long item, const wxColour &col )
{
    wxListItem info;
    info.m_itemId = item;
    info.SetBackgroundColour( col );
    m_mainWin->SetItem( info );
}

wxColour wxGenericListCtrl::GetItemBackgroundColour( long item ) const
{
    wxListItem info;
    info.m_itemId = item;
    m_mainWin->GetItem( info );
    return info.GetBackgroundColour();
}

int wxGenericListCtrl::GetScrollPos( int orient ) const
{
    return m_mainWin->GetScrollPos( orient );
}

void wxGenericListCtrl::SetScrollPos( int orient, int pos, bool refresh )
{
    m_mainWin->SetScrollPos( orient, pos, refresh );
}

void wxGenericListCtrl::SetItemFont( long item, const wxFont &f )
{
    wxListItem info;
    info.m_itemId = item;
    info.SetFont( f );
    m_mainWin->SetItem( info );
}

wxFont wxGenericListCtrl::GetItemFont( long item ) const
{
    wxListItem info;
    info.m_itemId = item;
    m_mainWin->GetItem( info );
    return info.GetFont();
}

int wxGenericListCtrl::GetSelectedItemCount() const
{
    return m_mainWin->GetSelectedItemCount();
}

wxColour wxGenericListCtrl::GetTextColour() const
{
    return GetForegroundColour();
}

void wxGenericListCtrl::SetTextColour(const wxColour& col)
{
    SetForegroundColour(col);
}

long wxGenericListCtrl::GetTopItem() const
{
    size_t top;
    m_mainWin->GetVisibleLinesRange(&top, NULL);
    return (long)top;
}

long wxGenericListCtrl::GetNextItem( long item, int geom, int state ) const
{
    return m_mainWin->GetNextItem( item, geom, state );
}

wxImageList *wxGenericListCtrl::GetImageList(int which) const
{
    if (which == wxIMAGE_LIST_NORMAL)
        return m_imageListNormal;
    else if (which == wxIMAGE_LIST_SMALL)
        return m_imageListSmall;
    else if (which == wxIMAGE_LIST_STATE)
        return m_imageListState;

    return (wxImageList *) NULL;
}

void wxGenericListCtrl::SetImageList( wxImageList *imageList, int which )
{
    if ( which == wxIMAGE_LIST_NORMAL )
    {
        if (m_ownsImageListNormal)
            delete m_imageListNormal;
        m_imageListNormal = imageList;
        m_ownsImageListNormal = false;
    }
    else if ( which == wxIMAGE_LIST_SMALL )
    {
        if (m_ownsImageListSmall)
            delete m_imageListSmall;
        m_imageListSmall = imageList;
        m_ownsImageListSmall = false;
    }
    else if ( which == wxIMAGE_LIST_STATE )
    {
        if (m_ownsImageListState)
            delete m_imageListState;
        m_imageListState = imageList;
        m_ownsImageListState = false;
    }

    m_mainWin->SetImageList( imageList, which );
}

void wxGenericListCtrl::AssignImageList(wxImageList *imageList, int which)
{
    SetImageList(imageList, which);
    if ( which == wxIMAGE_LIST_NORMAL )
        m_ownsImageListNormal = true;
    else if ( which == wxIMAGE_LIST_SMALL )
        m_ownsImageListSmall = true;
    else if ( which == wxIMAGE_LIST_STATE )
        m_ownsImageListState = true;
}

bool wxGenericListCtrl::Arrange( int WXUNUSED(flag) )
{
    return 0;
}

bool wxGenericListCtrl::DeleteItem( long item )
{
    m_mainWin->DeleteItem( item );
    return true;
}

bool wxGenericListCtrl::DeleteAllItems()
{
    m_mainWin->DeleteAllItems();
    return true;
}

bool wxGenericListCtrl::DeleteAllColumns()
{
    size_t count = m_mainWin->m_columns.GetCount();
    for ( size_t n = 0; n < count; n++ )
        DeleteColumn( 0 );
    return true;
}

void wxGenericListCtrl::ClearAll()
{
    m_mainWin->DeleteEverything();
}

bool wxGenericListCtrl::DeleteColumn( int col )
{
    m_mainWin->DeleteColumn( col );

    // if we don't have the header any longer, we need to relayout the window
    if ( !GetColumnCount() )
        ResizeReportView(false /* no header */);
    return true;
}

wxTextCtrl *wxGenericListCtrl::EditLabel(long item,
                                         wxClassInfo* textControlClass)
{
    return m_mainWin->EditLabel( item, textControlClass );
}

wxTextCtrl *wxGenericListCtrl::GetEditControl() const
{
    return m_mainWin->GetEditControl();
}

bool wxGenericListCtrl::EnsureVisible( long item )
{
    m_mainWin->EnsureVisible( item );
    return true;
}

long wxGenericListCtrl::FindItem( long start, const wxString& str, bool partial )
{
    return m_mainWin->FindItem( start, str, partial );
}

long wxGenericListCtrl::FindItem( long start, wxUIntPtr data )
{
    return m_mainWin->FindItem( start, data );
}

long wxGenericListCtrl::FindItem( long WXUNUSED(start), const wxPoint& pt,
                           int WXUNUSED(direction))
{
    return m_mainWin->FindItem( pt );
}

// TODO: sub item hit testing
long wxGenericListCtrl::HitTest(const wxPoint& point, int& flags, long *) const
{
    return m_mainWin->HitTest( (int)point.x, (int)point.y, flags );
}

long wxGenericListCtrl::InsertItem( wxListItem& info )
{
    m_mainWin->InsertItem( info );
    return info.m_itemId;
}

long wxGenericListCtrl::InsertItem( long index, const wxString &label )
{
    wxListItem info;
    info.m_text = label;
    info.m_mask = wxLIST_MASK_TEXT;
    info.m_itemId = index;
    return InsertItem( info );
}

long wxGenericListCtrl::InsertItem( long index, int imageIndex )
{
    wxListItem info;
    info.m_mask = wxLIST_MASK_IMAGE;
    info.m_image = imageIndex;
    info.m_itemId = index;
    return InsertItem( info );
}

long wxGenericListCtrl::InsertItem( long index, const wxString &label, int imageIndex )
{
    wxListItem info;
    info.m_text = label;
    info.m_image = imageIndex;
    info.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE;
    info.m_itemId = index;
    return InsertItem( info );
}

long wxGenericListCtrl::InsertColumn( long col, wxListItem &item )
{
    wxCHECK_MSG( m_headerWin, -1, _T("can't add column in non report mode") );

    m_mainWin->InsertColumn( col, item );

    // if we hadn't had a header before but have one now
    // then we need to relayout the window
    if ( GetColumnCount() == 1 && m_mainWin->HasHeader() )
        ResizeReportView(true /* have header */);

    m_headerWin->Refresh();

    return 0;
}

long wxGenericListCtrl::InsertColumn( long col, const wxString &heading,
                               int format, int width )
{
    wxListItem item;
    item.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_FORMAT;
    item.m_text = heading;
    if (width >= -2)
    {
        item.m_mask |= wxLIST_MASK_WIDTH;
        item.m_width = width;
    }

    item.m_format = format;

    return InsertColumn( col, item );
}

bool wxGenericListCtrl::ScrollList( int dx, int dy )
{
    return m_mainWin->ScrollList(dx, dy);
}

// Sort items.
// fn is a function which takes 3 long arguments: item1, item2, data.
// item1 is the long data associated with a first item (NOT the index).
// item2 is the long data associated with a second item (NOT the index).
// data is the same value as passed to SortItems.
// The return value is a negative number if the first item should precede the second
// item, a positive number of the second item should precede the first,
// or zero if the two items are equivalent.
// data is arbitrary data to be passed to the sort function.

bool wxGenericListCtrl::SortItems( wxListCtrlCompare fn, long data )
{
    m_mainWin->SortItems( fn, data );
    return true;
}

// ----------------------------------------------------------------------------
// event handlers
// ----------------------------------------------------------------------------

void wxGenericListCtrl::OnSize(wxSizeEvent& WXUNUSED(event))
{
    if ( !m_mainWin )
        return;

    ResizeReportView(m_mainWin->HasHeader());
    m_mainWin->RecalculatePositions();
}

void wxGenericListCtrl::ResizeReportView(bool showHeader)
{
    int cw, ch;
    GetClientSize( &cw, &ch );

    if ( showHeader )
    {
        m_headerWin->SetSize( 0, 0, cw, m_headerHeight );
        if(ch > m_headerHeight)
            m_mainWin->SetSize( 0, m_headerHeight + 1,
                                   cw, ch - m_headerHeight - 1 );
        else
            m_mainWin->SetSize( 0, m_headerHeight + 1,
                                   cw, 0);
    }
    else // no header window
    {
        m_mainWin->SetSize( 0, 0, cw, ch );
    }
}

void wxGenericListCtrl::OnInternalIdle()
{
    wxWindow::OnInternalIdle();

    // do it only if needed
    if ( !m_mainWin->m_dirty )
        return;

    m_mainWin->RecalculatePositions();
}

// ----------------------------------------------------------------------------
// font/colours
// ----------------------------------------------------------------------------

bool wxGenericListCtrl::SetBackgroundColour( const wxColour &colour )
{
    if (m_mainWin)
    {
        m_mainWin->SetBackgroundColour( colour );
        m_mainWin->m_dirty = true;
    }

    return true;
}

bool wxGenericListCtrl::SetForegroundColour( const wxColour &colour )
{
    if ( !wxWindow::SetForegroundColour( colour ) )
        return false;

    if (m_mainWin)
    {
        m_mainWin->SetForegroundColour( colour );
        m_mainWin->m_dirty = true;
    }

    if (m_headerWin)
        m_headerWin->SetForegroundColour( colour );

    return true;
}

bool wxGenericListCtrl::SetFont( const wxFont &font )
{
    if ( !wxWindow::SetFont( font ) )
        return false;

    if (m_mainWin)
    {
        m_mainWin->SetFont( font );
        m_mainWin->m_dirty = true;
    }

    if (m_headerWin)
    {
        m_headerWin->SetFont( font );
        CalculateAndSetHeaderHeight();
    }

    Refresh();

    return true;
}

// static
wxVisualAttributes
wxGenericListCtrl::GetClassDefaultAttributes(wxWindowVariant variant)
{
#if _USE_VISATTR
    // Use the same color scheme as wxListBox
    return wxListBox::GetClassDefaultAttributes(variant);
#else
    wxUnusedVar(variant);
    wxVisualAttributes attr;
    attr.colFg = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);
    attr.colBg = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX);
    attr.font  = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    return attr;
#endif
}

// ----------------------------------------------------------------------------
// methods forwarded to m_mainWin
// ----------------------------------------------------------------------------

#if wxUSE_DRAG_AND_DROP

void wxGenericListCtrl::SetDropTarget( wxDropTarget *dropTarget )
{
    m_mainWin->SetDropTarget( dropTarget );
}

wxDropTarget *wxGenericListCtrl::GetDropTarget() const
{
    return m_mainWin->GetDropTarget();
}

#endif

bool wxGenericListCtrl::SetCursor( const wxCursor &cursor )
{
    return m_mainWin ? m_mainWin->wxWindow::SetCursor(cursor) : false;
}

wxColour wxGenericListCtrl::GetBackgroundColour() const
{
    return m_mainWin ? m_mainWin->GetBackgroundColour() : wxColour();
}

wxColour wxGenericListCtrl::GetForegroundColour() const
{
    return m_mainWin ? m_mainWin->GetForegroundColour() : wxColour();
}

bool wxGenericListCtrl::DoPopupMenu( wxMenu *menu, int x, int y )
{
#if wxUSE_MENUS
    return m_mainWin->PopupMenu( menu, x, y );
#else
    return false;
#endif
}

void wxGenericListCtrl::DoClientToScreen( int *x, int *y ) const
{
    m_mainWin->DoClientToScreen(x, y);
}

void wxGenericListCtrl::DoScreenToClient( int *x, int *y ) const
{
    m_mainWin->DoScreenToClient(x, y);
}

void wxGenericListCtrl::SetFocus()
{
    // The test in window.cpp fails as we are a composite
    // window, so it checks against "this", but not m_mainWin.
    if ( DoFindFocus() != this )
        m_mainWin->SetFocus();
}

wxSize wxGenericListCtrl::DoGetBestSize() const
{
    // Something is better than nothing...
    // 100x80 is what the MSW version will get from the default
    // wxControl::DoGetBestSize
    return wxSize(100, 80);
}

// ----------------------------------------------------------------------------
// virtual list control support
// ----------------------------------------------------------------------------

wxString wxGenericListCtrl::OnGetItemText(long WXUNUSED(item), long WXUNUSED(col)) const
{
    // this is a pure virtual function, in fact - which is not really pure
    // because the controls which are not virtual don't need to implement it
    wxFAIL_MSG( _T("wxGenericListCtrl::OnGetItemText not supposed to be called") );

    return wxEmptyString;
}

int wxGenericListCtrl::OnGetItemImage(long WXUNUSED(item)) const
{
    wxCHECK_MSG(!GetImageList(wxIMAGE_LIST_SMALL),
                -1,
                wxT("List control has an image list, OnGetItemImage or OnGetItemColumnImage should be overridden."));
    return -1;
}

int wxGenericListCtrl::OnGetItemColumnImage(long item, long column) const
{
    if (!column)
        return OnGetItemImage(item);

   return -1;
}

wxListItemAttr *
wxGenericListCtrl::OnGetItemAttr(long WXUNUSED_UNLESS_DEBUG(item)) const
{
    wxASSERT_MSG( item >= 0 && item < GetItemCount(),
                  _T("invalid item index in OnGetItemAttr()") );

    // no attributes by default
    return NULL;
}

void wxGenericListCtrl::SetItemCount(long count)
{
    wxASSERT_MSG( IsVirtual(), _T("this is for virtual controls only") );

    m_mainWin->SetItemCount(count);
}

void wxGenericListCtrl::RefreshItem(long item)
{
    m_mainWin->RefreshLine(item);
}

void wxGenericListCtrl::RefreshItems(long itemFrom, long itemTo)
{
    m_mainWin->RefreshLines(itemFrom, itemTo);
}

// Generic wxListCtrl is more or less a container for two other
// windows which drawings are done upon. These are namely
// 'm_headerWin' and 'm_mainWin'.
// Here we override 'virtual wxWindow::Refresh()' to mimic the
// behaviour wxListCtrl has under wxMSW.
//
void wxGenericListCtrl::Refresh(bool eraseBackground, const wxRect *rect)
{
    if (!rect)
    {
        // The easy case, no rectangle specified.
        if (m_headerWin)
            m_headerWin->Refresh(eraseBackground);

        if (m_mainWin)
            m_mainWin->Refresh(eraseBackground);
    }
    else
    {
        // Refresh the header window
        if (m_headerWin)
        {
            wxRect rectHeader = m_headerWin->GetRect();
            rectHeader.Intersect(*rect);
            if (rectHeader.GetWidth() && rectHeader.GetHeight())
            {
                int x, y;
                m_headerWin->GetPosition(&x, &y);
                rectHeader.Offset(-x, -y);
                m_headerWin->Refresh(eraseBackground, &rectHeader);
            }
        }

        // Refresh the main window
        if (m_mainWin)
        {
            wxRect rectMain = m_mainWin->GetRect();
            rectMain.Intersect(*rect);
            if (rectMain.GetWidth() && rectMain.GetHeight())
            {
                int x, y;
                m_mainWin->GetPosition(&x, &y);
                rectMain.Offset(-x, -y);
                m_mainWin->Refresh(eraseBackground, &rectMain);
            }
        }
    }
}

void wxGenericListCtrl::Freeze()
{
    m_mainWin->Freeze();
}

void wxGenericListCtrl::Thaw()
{
    m_mainWin->Thaw();
}

#endif // wxUSE_LISTCTRL
