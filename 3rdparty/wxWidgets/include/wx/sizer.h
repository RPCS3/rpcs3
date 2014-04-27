/////////////////////////////////////////////////////////////////////////////
// Name:        wx/sizer.h
// Purpose:     provide wxSizer class for layout
// Author:      Robert Roebling and Robin Dunn
// Modified by: Ron Lee, Vadim Zeitlin (wxSizerFlags)
// Created:
// RCS-ID:      $Id: sizer.h 52331 2008-03-05 15:02:22Z VS $
// Copyright:   (c) Robin Dunn, Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef __WXSIZER_H__
#define __WXSIZER_H__

#include "wx/defs.h"

#include "wx/window.h"

//---------------------------------------------------------------------------
// classes
//---------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_CORE wxButton;
class WXDLLIMPEXP_FWD_CORE wxBoxSizer;
class WXDLLIMPEXP_FWD_CORE wxSizerItem;
class WXDLLIMPEXP_FWD_CORE wxSizer;
class WXDLLIMPEXP_FWD_CORE wxFlexGridSizer;
class WXDLLIMPEXP_FWD_CORE wxGridBagSizer;

#ifndef wxUSE_BORDER_BY_DEFAULT
    #ifdef __SMARTPHONE__
        // no borders by default on limited size screen
        #define wxUSE_BORDER_BY_DEFAULT 0
    #else
        #define wxUSE_BORDER_BY_DEFAULT 1
    #endif
#endif

// ----------------------------------------------------------------------------
// wxSizerFlags: flags used for an item in the sizer
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxSizerFlags
{
public:
    // construct the flags object initialized with the given proportion (0 by
    // default)
    wxSizerFlags(int proportion = 0) : m_proportion(proportion)
    {
        m_flags = 0;
        m_borderInPixels = 0;
    }

    // setters for all sizer flags, they all return the object itself so that
    // calls to them can be chained

    wxSizerFlags& Proportion(int proportion)
    {
        m_proportion = proportion;
        return *this;
    }

    wxSizerFlags& Align(int alignment) // combination of wxAlignment values
    {
        m_flags &= ~wxALIGN_MASK;
        m_flags |= alignment;

        return *this;
    }

    wxSizerFlags& Expand()
    {
        m_flags |= wxEXPAND;
        return *this;
    }

    // some shortcuts for Align()
    wxSizerFlags& Centre() { return Align(wxCENTRE); }
    wxSizerFlags& Center() { return Centre(); }
    wxSizerFlags& Left() { return Align(wxALIGN_LEFT); }
    wxSizerFlags& Right() { return Align(wxALIGN_RIGHT); }

#if wxABI_VERSION >= 20802
    wxSizerFlags& Top() { return Align(wxALIGN_TOP); }
    wxSizerFlags& Bottom() { return Align(wxALIGN_BOTTOM); }
#endif // wxABI 2.8.2+

    // default border size used by Border() below
    static int GetDefaultBorder()
    {
#if wxUSE_BORDER_BY_DEFAULT
        // FIXME: default border size shouldn't be hardcoded and at the very
        //        least they should depend on the current font size
        return 5;
#else
        return 0;
#endif
    }


    wxSizerFlags& Border(int direction, int borderInPixels)
    {
        m_flags &= ~wxALL;
        m_flags |= direction;

        m_borderInPixels = borderInPixels;

        return *this;
    }

    wxSizerFlags& Border(int direction = wxALL)
    {
#if wxUSE_BORDER_BY_DEFAULT
        return Border(direction, GetDefaultBorder());
#else
        // no borders by default on limited size screen
        wxUnusedVar(direction);

        return *this;
#endif
    }

    wxSizerFlags& DoubleBorder(int direction = wxALL)
    {
#if wxUSE_BORDER_BY_DEFAULT
        return Border(direction, 2*GetDefaultBorder());
#else
        wxUnusedVar(direction);

        return *this;
#endif
    }

    wxSizerFlags& TripleBorder(int direction = wxALL)
    {
#if wxUSE_BORDER_BY_DEFAULT
        return Border(direction, 3*GetDefaultBorder());
#else
        wxUnusedVar(direction);

        return *this;
#endif
    }

    wxSizerFlags& HorzBorder()
    {
#if wxUSE_BORDER_BY_DEFAULT
        return Border(wxLEFT | wxRIGHT, GetDefaultBorder());
#else
        return *this;
#endif
    }

    wxSizerFlags& DoubleHorzBorder()
    {
#if wxUSE_BORDER_BY_DEFAULT
        return Border(wxLEFT | wxRIGHT, 2*GetDefaultBorder());
#else
        return *this;
#endif
    }

#if wxABI_VERSION >= 20802
    // setters for the others flags
    wxSizerFlags& Shaped()
    {
        m_flags |= wxSHAPED;

        return *this;
    }

    wxSizerFlags& FixedMinSize()
    {
        m_flags |= wxFIXED_MINSIZE;

        return *this;
    }
#endif // wx 2.8.2+

#if wxABI_VERSION >= 20808
    // makes the item ignore window's visibility status
    wxSizerFlags& ReserveSpaceEvenIfHidden();
#endif

    // accessors for wxSizer only
    int GetProportion() const { return m_proportion; }
    int GetFlags() const { return m_flags; }
    int GetBorderInPixels() const { return m_borderInPixels; }

private:
    int m_proportion;
    int m_flags;
    int m_borderInPixels;
};


// ----------------------------------------------------------------------------
// wxSizerSpacer: used by wxSizerItem to represent a spacer
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxSizerSpacer
{
public:
    wxSizerSpacer(const wxSize& size) : m_size(size), m_isShown(true) { }

    void SetSize(const wxSize& size) { m_size = size; }
    const wxSize& GetSize() const { return m_size; }

    void Show(bool show) { m_isShown = show; }
    bool IsShown() const { return m_isShown; }

private:
    // the size, in pixel
    wxSize m_size;

    // is the spacer currently shown?
    bool m_isShown;
};

// ----------------------------------------------------------------------------
// wxSizerItem
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxSizerItem : public wxObject
{
public:
    // window
    wxSizerItem( wxWindow *window,
                 int proportion,
                 int flag,
                 int border,
                 wxObject* userData );

    // window with flags
    wxSizerItem(wxWindow *window, const wxSizerFlags& flags)
    {
        Init(flags);

        SetWindow(window);
    }

    // subsizer
    wxSizerItem( wxSizer *sizer,
                 int proportion,
                 int flag,
                 int border,
                 wxObject* userData );

    // sizer with flags
    wxSizerItem(wxSizer *sizer, const wxSizerFlags& flags)
    {
        Init(flags);

        SetSizer(sizer);
    }

    // spacer
    wxSizerItem( int width,
                 int height,
                 int proportion,
                 int flag,
                 int border,
                 wxObject* userData);

    // spacer with flags
    wxSizerItem(int width, int height, const wxSizerFlags& flags)
    {
        Init(flags);

        SetSpacer(width, height);
    }

    wxSizerItem();
    virtual ~wxSizerItem();

    virtual void DeleteWindows();

    // Enable deleting the SizerItem without destroying the contained sizer.
    void DetachSizer() { m_sizer = NULL; }

    virtual wxSize GetSize() const;
    virtual wxSize CalcMin();
    virtual void SetDimension( const wxPoint& pos, const wxSize& size );

    wxSize GetMinSize() const
        { return m_minSize; }
    wxSize GetMinSizeWithBorder() const;

    void SetMinSize(const wxSize& size)
    {
        if ( IsWindow() )
            m_window->SetMinSize(size);
        m_minSize = size;
    }
    void SetMinSize( int x, int y )
        { SetMinSize(wxSize(x, y)); }
    void SetInitSize( int x, int y )
        { SetMinSize(wxSize(x, y)); }

    // if either of dimensions is zero, ratio is assumed to be 1
    // to avoid "divide by zero" errors
    void SetRatio(int width, int height)
        { m_ratio = (width && height) ? ((float) width / (float) height) : 1; }
    void SetRatio(const wxSize& size)
        { SetRatio(size.x, size.y); }
    void SetRatio(float ratio)
        { m_ratio = ratio; }
    float GetRatio() const
        { return m_ratio; }

    virtual wxRect GetRect() { return m_rect; }

    bool IsWindow() const { return m_kind == Item_Window; }
    bool IsSizer() const { return m_kind == Item_Sizer; }
    bool IsSpacer() const { return m_kind == Item_Spacer; }

#if WXWIN_COMPATIBILITY_2_6
    // Deprecated in 2.6, use {G,S}etProportion instead.
    wxDEPRECATED( void SetOption( int option ) );
    wxDEPRECATED( int GetOption() const );
#endif // WXWIN_COMPATIBILITY_2_6

    void SetProportion( int proportion )
        { m_proportion = proportion; }
    int GetProportion() const
        { return m_proportion; }
    void SetFlag( int flag )
        { m_flag = flag; }
    int GetFlag() const
        { return m_flag; }
    void SetBorder( int border )
        { m_border = border; }
    int GetBorder() const
        { return m_border; }

    wxWindow *GetWindow() const
        { return m_kind == Item_Window ? m_window : NULL; }
    wxSizer *GetSizer() const
        { return m_kind == Item_Sizer ? m_sizer : NULL; }
    wxSize GetSpacer() const;

    // this function behaves obviously for the windows and spacers but for the
    // sizers it returns true if any sizer element is shown and only returns
    // false if all of them are hidden
    bool IsShown() const;
    void Show(bool show);

    void SetUserData(wxObject* userData)
        { delete m_userData; m_userData = userData; }
    wxObject* GetUserData() const
        { return m_userData; }
    wxPoint GetPosition() const
        { return m_pos; }


    // these functions do not free old sizer/spacer
    void SetWindow(wxWindow *window);
    void SetSizer(wxSizer *sizer);
    void SetSpacer(const wxSize& size);
    void SetSpacer(int width, int height) { SetSpacer(wxSize(width, height)); }

protected:
    // common part of several ctors
    void Init() { m_userData = NULL; }

    // common part of ctors taking wxSizerFlags
    void Init(const wxSizerFlags& flags);


    // discriminated union: depending on m_kind one of the fields is valid
    enum
    {
        Item_None,
        Item_Window,
        Item_Sizer,
        Item_Spacer,
        Item_Max
    } m_kind;
    union
    {
        wxWindow      *m_window;
        wxSizer       *m_sizer;
        wxSizerSpacer *m_spacer;
    };

    wxPoint      m_pos;
    wxSize       m_minSize;
    int          m_proportion;
    int          m_border;
    int          m_flag;

    // on screen rectangle of this item (not including borders)
    wxRect       m_rect;

    // Aspect ratio can always be calculated from m_size,
    // but this would cause precision loss when the window
    // is shrunk.  It is safer to preserve the initial value.
    float        m_ratio;

    wxObject    *m_userData;

private:
    // 2.8-only implementation detail for wxRESERVE_SPACE_EVEN_IF_HIDDEN
    bool ShouldAccountFor() const;

    DECLARE_CLASS(wxSizerItem)
    DECLARE_NO_COPY_CLASS(wxSizerItem)

    friend class wxBoxSizer;
    friend class wxFlexGridSizer;
    friend class wxGridBagSizer;
};

WX_DECLARE_EXPORTED_LIST( wxSizerItem, wxSizerItemList );


//---------------------------------------------------------------------------
// wxSizer
//---------------------------------------------------------------------------

class WXDLLEXPORT wxSizer: public wxObject, public wxClientDataContainer
{
public:
    wxSizer() { m_containingWindow = NULL; }
    virtual ~wxSizer();

    // methods for adding elements to the sizer: there are Add/Insert/Prepend
    // overloads for each of window/sizer/spacer/wxSizerItem
    wxSizerItem* Add(wxWindow *window,
                     int proportion = 0,
                     int flag = 0,
                     int border = 0,
                     wxObject* userData = NULL);
    wxSizerItem* Add(wxSizer *sizer,
                     int proportion = 0,
                     int flag = 0,
                     int border = 0,
                     wxObject* userData = NULL);
    wxSizerItem* Add(int width,
                     int height,
                     int proportion = 0,
                     int flag = 0,
                     int border = 0,
                     wxObject* userData = NULL);
    wxSizerItem* Add( wxWindow *window, const wxSizerFlags& flags);
    wxSizerItem* Add( wxSizer *sizer, const wxSizerFlags& flags);
    wxSizerItem* Add( wxSizerItem *item);

    wxSizerItem* AddSpacer(int size);
    wxSizerItem* AddStretchSpacer(int prop = 1);

    wxSizerItem* Insert(size_t index,
                        wxWindow *window,
                        int proportion = 0,
                        int flag = 0,
                        int border = 0,
                        wxObject* userData = NULL);
    wxSizerItem* Insert(size_t index,
                        wxSizer *sizer,
                        int proportion = 0,
                        int flag = 0,
                        int border = 0,
                        wxObject* userData = NULL);
    wxSizerItem* Insert(size_t index,
                        int width,
                        int height,
                        int proportion = 0,
                        int flag = 0,
                        int border = 0,
                        wxObject* userData = NULL);
    wxSizerItem* Insert(size_t index,
                        wxWindow *window,
                        const wxSizerFlags& flags);
    wxSizerItem* Insert(size_t index,
                        wxSizer *sizer,
                        const wxSizerFlags& flags);
    virtual wxSizerItem* Insert( size_t index, wxSizerItem *item);

    wxSizerItem* InsertSpacer(size_t index, int size);
    wxSizerItem* InsertStretchSpacer(size_t index, int prop = 1);

    wxSizerItem* Prepend(wxWindow *window,
                         int proportion = 0,
                         int flag = 0,
                         int border = 0,
                         wxObject* userData = NULL);
    wxSizerItem* Prepend(wxSizer *sizer,
                         int proportion = 0,
                         int flag = 0,
                         int border = 0,
                         wxObject* userData = NULL);
    wxSizerItem* Prepend(int width,
                         int height,
                         int proportion = 0,
                         int flag = 0,
                         int border = 0,
                         wxObject* userData = NULL);
    wxSizerItem* Prepend(wxWindow *window, const wxSizerFlags& flags);
    wxSizerItem* Prepend(wxSizer *sizer, const wxSizerFlags& flags);
    wxSizerItem* Prepend(wxSizerItem *item);

    wxSizerItem* PrependSpacer(int size);
    wxSizerItem* PrependStretchSpacer(int prop = 1);

    // set (or possibly unset if window is NULL) or get the window this sizer
    // is used in
    void SetContainingWindow(wxWindow *window);
    wxWindow *GetContainingWindow() const { return m_containingWindow; }

#if WXWIN_COMPATIBILITY_2_6
    // Deprecated in 2.6 since historically it does not delete the window,
    // use Detach instead.
    wxDEPRECATED( virtual bool Remove( wxWindow *window ) );
#endif // WXWIN_COMPATIBILITY_2_6

    virtual bool Remove( wxSizer *sizer );
    virtual bool Remove( int index );

    virtual bool Detach( wxWindow *window );
    virtual bool Detach( wxSizer *sizer );
    virtual bool Detach( int index );

    virtual bool Replace( wxWindow *oldwin, wxWindow *newwin, bool recursive = false );
    virtual bool Replace( wxSizer *oldsz, wxSizer *newsz, bool recursive = false );
    virtual bool Replace( size_t index, wxSizerItem *newitem );

    virtual void Clear( bool delete_windows = false );
    virtual void DeleteWindows();

    void SetMinSize( int width, int height )
        { DoSetMinSize( width, height ); }
    void SetMinSize( const wxSize& size )
        { DoSetMinSize( size.x, size.y ); }

    // Searches recursively
    bool SetItemMinSize( wxWindow *window, int width, int height )
        { return DoSetItemMinSize( window, width, height ); }
    bool SetItemMinSize( wxWindow *window, const wxSize& size )
        { return DoSetItemMinSize( window, size.x, size.y ); }

    // Searches recursively
    bool SetItemMinSize( wxSizer *sizer, int width, int height )
        { return DoSetItemMinSize( sizer, width, height ); }
    bool SetItemMinSize( wxSizer *sizer, const wxSize& size )
        { return DoSetItemMinSize( sizer, size.x, size.y ); }

    bool SetItemMinSize( size_t index, int width, int height )
        { return DoSetItemMinSize( index, width, height ); }
    bool SetItemMinSize( size_t index, const wxSize& size )
        { return DoSetItemMinSize( index, size.x, size.y ); }

    wxSize GetSize() const
        { return m_size; }
    wxPoint GetPosition() const
        { return m_position; }

    // Calculate the minimal size or return m_minSize if bigger.
    wxSize GetMinSize();

    virtual void RecalcSizes() = 0;
    virtual wxSize CalcMin() = 0;

    virtual void Layout();

#if wxABI_VERSION >= 20808
    wxSize ComputeFittingClientSize(wxWindow *window);
    wxSize ComputeFittingWindowSize(wxWindow *window);
#endif

    wxSize Fit( wxWindow *window );
    void FitInside( wxWindow *window );
    void SetSizeHints( wxWindow *window );
    void SetVirtualSizeHints( wxWindow *window );

    wxSizerItemList& GetChildren()
        { return m_children; }

    void SetDimension( int x, int y, int width, int height );

    wxSizerItem* GetItem( wxWindow *window, bool recursive = false );
    wxSizerItem* GetItem( wxSizer *sizer, bool recursive = false );
    wxSizerItem* GetItem( size_t index );

    // Manage whether individual scene items are considered
    // in the layout calculations or not.
    bool Show( wxWindow *window, bool show = true, bool recursive = false );
    bool Show( wxSizer *sizer, bool show = true, bool recursive = false );
    bool Show( size_t index, bool show = true );

    bool Hide( wxSizer *sizer, bool recursive = false )
        { return Show( sizer, false, recursive ); }
    bool Hide( wxWindow *window, bool recursive = false )
        { return Show( window, false, recursive ); }
    bool Hide( size_t index )
        { return Show( index, false ); }

    bool IsShown( wxWindow *window ) const;
    bool IsShown( wxSizer *sizer ) const;
    bool IsShown( size_t index ) const;

    // Recursively call wxWindow::Show () on all sizer items.
    virtual void ShowItems (bool show);

    void Show(bool show) { ShowItems(show); }

protected:
    wxSize              m_size;
    wxSize              m_minSize;
    wxPoint             m_position;
    wxSizerItemList     m_children;

    // the window this sizer is used in, can be NULL
    wxWindow *m_containingWindow;

    wxSize GetMaxWindowSize( wxWindow *window ) const;
    wxSize GetMinWindowSize( wxWindow *window );
    wxSize GetMaxClientSize( wxWindow *window ) const;
    wxSize GetMinClientSize( wxWindow *window );
    wxSize VirtualFitSize( wxWindow *window );

    virtual void DoSetMinSize( int width, int height );
    virtual bool DoSetItemMinSize( wxWindow *window, int width, int height );
    virtual bool DoSetItemMinSize( wxSizer *sizer, int width, int height );
    virtual bool DoSetItemMinSize( size_t index, int width, int height );

private:
    DECLARE_CLASS(wxSizer)
};

//---------------------------------------------------------------------------
// wxGridSizer
//---------------------------------------------------------------------------

class WXDLLEXPORT wxGridSizer: public wxSizer
{
public:
    wxGridSizer( int rows, int cols, int vgap, int hgap );
    wxGridSizer( int cols, int vgap = 0, int hgap = 0 );

    virtual void RecalcSizes();
    virtual wxSize CalcMin();

    void SetCols( int cols )    { m_cols = cols; }
    void SetRows( int rows )    { m_rows = rows; }
    void SetVGap( int gap )     { m_vgap = gap; }
    void SetHGap( int gap )     { m_hgap = gap; }
    int GetCols() const         { return m_cols; }
    int GetRows() const         { return m_rows; }
    int GetVGap() const         { return m_vgap; }
    int GetHGap() const         { return m_hgap; }

protected:
    int    m_rows;
    int    m_cols;
    int    m_vgap;
    int    m_hgap;

    // return the number of total items and the number of columns and rows
    int CalcRowsCols(int& rows, int& cols) const;

    void SetItemBounds( wxSizerItem *item, int x, int y, int w, int h );

private:
    DECLARE_CLASS(wxGridSizer)
};

//---------------------------------------------------------------------------
// wxFlexGridSizer
//---------------------------------------------------------------------------

// the bevaiour for resizing wxFlexGridSizer cells in the "non-flexible"
// direction
enum wxFlexSizerGrowMode
{
    // don't resize the cells in non-flexible direction at all
    wxFLEX_GROWMODE_NONE,

    // uniformly resize only the specified ones (default)
    wxFLEX_GROWMODE_SPECIFIED,

    // uniformly resize all cells
    wxFLEX_GROWMODE_ALL
};

class WXDLLEXPORT wxFlexGridSizer: public wxGridSizer
{
public:
    // ctors/dtor
    wxFlexGridSizer( int rows, int cols, int vgap, int hgap );
    wxFlexGridSizer( int cols, int vgap = 0, int hgap = 0 );
    virtual ~wxFlexGridSizer();


    // set the rows/columns which will grow (the others will remain of the
    // constant initial size)
    void AddGrowableRow( size_t idx, int proportion = 0 );
    void RemoveGrowableRow( size_t idx );
    void AddGrowableCol( size_t idx, int proportion = 0 );
    void RemoveGrowableCol( size_t idx );


    // the sizer cells may grow in both directions, not grow at all or only
    // grow in one direction but not the other

    // the direction may be wxVERTICAL, wxHORIZONTAL or wxBOTH (default)
    void SetFlexibleDirection(int direction) { m_flexDirection = direction; }
    int GetFlexibleDirection() const { return m_flexDirection; }

    // note that the grow mode only applies to the direction which is not
    // flexible
    void SetNonFlexibleGrowMode(wxFlexSizerGrowMode mode) { m_growMode = mode; }
    wxFlexSizerGrowMode GetNonFlexibleGrowMode() const { return m_growMode; }

    // Read-only access to the row heights and col widths arrays
    const wxArrayInt& GetRowHeights() const { return m_rowHeights; }
    const wxArrayInt& GetColWidths() const  { return m_colWidths; }

    // implementation
    virtual void RecalcSizes();
    virtual wxSize CalcMin();

protected:
    void AdjustForFlexDirection();
    void AdjustForGrowables(const wxSize& sz, const wxSize& minsz,
                            int nrows, int ncols);

    // the heights/widths of all rows/columns
    wxArrayInt  m_rowHeights,
                m_colWidths;

    // indices of the growable columns and rows
    wxArrayInt  m_growableRows,
                m_growableCols;

    // proportion values of the corresponding growable rows and columns
    wxArrayInt  m_growableRowsProportions,
                m_growableColsProportions;

    // parameters describing whether the growable cells should be resized in
    // both directions or only one
    int m_flexDirection;
    wxFlexSizerGrowMode m_growMode;

    // saves CalcMin result to optimize RecalcSizes
    wxSize m_calculatedMinSize;

private:
    DECLARE_CLASS(wxFlexGridSizer)
    DECLARE_NO_COPY_CLASS(wxFlexGridSizer)
};

//---------------------------------------------------------------------------
// wxBoxSizer
//---------------------------------------------------------------------------

class WXDLLEXPORT wxBoxSizer: public wxSizer
{
public:
    wxBoxSizer( int orient );

    void RecalcSizes();
    wxSize CalcMin();

    int GetOrientation() const
        { return m_orient; }

    void SetOrientation(int orient)
        { m_orient = orient; }

protected:
    int m_orient;
    int m_stretchable;
    int m_minWidth;
    int m_minHeight;
    int m_fixedWidth;
    int m_fixedHeight;

private:
    DECLARE_CLASS(wxBoxSizer)
};

//---------------------------------------------------------------------------
// wxStaticBoxSizer
//---------------------------------------------------------------------------

#if wxUSE_STATBOX

class WXDLLIMPEXP_FWD_CORE wxStaticBox;

class WXDLLEXPORT wxStaticBoxSizer: public wxBoxSizer
{
public:
    wxStaticBoxSizer(wxStaticBox *box, int orient);
    wxStaticBoxSizer(int orient, wxWindow *win, const wxString& label = wxEmptyString);
    virtual ~wxStaticBoxSizer();

    void RecalcSizes();
    wxSize CalcMin();

    wxStaticBox *GetStaticBox() const
        { return m_staticBox; }

    // override to hide/show the static box as well
    virtual void ShowItems (bool show);

    virtual bool Detach( wxWindow *window );
    virtual bool Detach( wxSizer *sizer ) { return wxBoxSizer::Detach(sizer); }
    virtual bool Detach( int index ) { return wxBoxSizer::Detach(index); }

protected:
    wxStaticBox   *m_staticBox;

private:
    DECLARE_CLASS(wxStaticBoxSizer)
    DECLARE_NO_COPY_CLASS(wxStaticBoxSizer)
};

#endif // wxUSE_STATBOX

#if wxUSE_BUTTON

class WXDLLEXPORT wxStdDialogButtonSizer: public wxBoxSizer
{
public:
    // Constructor just creates a new wxBoxSizer, not much else.
    // Box sizer orientation is automatically determined here:
    // vertical for PDAs, horizontal for everything else?
    wxStdDialogButtonSizer();

    // Checks button ID against system IDs and sets one of the pointers below
    // to this button. Does not do any sizer-related things here.
    void AddButton(wxButton *button);

    // Use these if no standard ID can/should be used
    void SetAffirmativeButton( wxButton *button );
    void SetNegativeButton( wxButton *button );
    void SetCancelButton( wxButton *button );

    // All platform-specific code here, checks which buttons exist and add
    // them to the sizer accordingly.
    // Note - one potential hack on Mac we could use here,
    // if m_buttonAffirmative is wxID_SAVE then ensure wxID_SAVE
    // is set to _("Save") and m_buttonNegative is set to _("Don't Save")
    // I wouldn't add any other hacks like that into here,
    // but this one I can see being useful.
    void Realize();

    wxButton *GetAffirmativeButton() const { return m_buttonAffirmative; }
    wxButton *GetApplyButton() const { return m_buttonApply; }
    wxButton *GetNegativeButton() const { return m_buttonNegative; }
    wxButton *GetCancelButton() const { return m_buttonCancel; }
    wxButton *GetHelpButton() const { return m_buttonHelp; }

protected:
    wxButton *m_buttonAffirmative;  // wxID_OK, wxID_YES, wxID_SAVE go here
    wxButton *m_buttonApply;
    wxButton *m_buttonNegative;     // wxID_NO
    wxButton *m_buttonCancel;
    wxButton *m_buttonHelp;

private:
    DECLARE_CLASS(wxStdDialogButtonSizer)
    DECLARE_NO_COPY_CLASS(wxStdDialogButtonSizer)
};

#endif // wxUSE_BUTTON

#if WXWIN_COMPATIBILITY_2_4
// NB: wxBookCtrlSizer and wxNotebookSizer are deprecated, they
//     don't do anything. wxBookCtrlBase::DoGetBestSize does the job now.

// ----------------------------------------------------------------------------
// wxBookCtrlSizer
// ----------------------------------------------------------------------------

#if wxUSE_BOOKCTRL

// this sizer works with wxNotebook/wxListbook/... and sizes the control to
// fit its pages
class WXDLLIMPEXP_FWD_CORE wxBookCtrlBase;

class WXDLLEXPORT wxBookCtrlSizer : public wxSizer
{
public:
#if WXWIN_COMPATIBILITY_2_6
    wxDEPRECATED( wxBookCtrlSizer(wxBookCtrlBase *bookctrl) );
#endif // WXWIN_COMPATIBILITY_2_6

    wxBookCtrlBase *GetControl() const { return m_bookctrl; }

    virtual void RecalcSizes();
    virtual wxSize CalcMin();

protected:
    // this protected ctor lets us mark the real one above as deprecated
    // and still have warning-free build of the library itself:
    wxBookCtrlSizer() {}

    wxBookCtrlBase *m_bookctrl;

private:
    DECLARE_CLASS(wxBookCtrlSizer)
    DECLARE_NO_COPY_CLASS(wxBookCtrlSizer)
};


#if wxUSE_NOTEBOOK

// before wxBookCtrlBase we only had wxNotebookSizer, keep it for backwards
// compatibility
class WXDLLIMPEXP_FWD_CORE wxNotebook;

class WXDLLEXPORT wxNotebookSizer : public wxBookCtrlSizer
{
public:
#if WXWIN_COMPATIBILITY_2_6
    wxDEPRECATED( wxNotebookSizer(wxNotebook *nb) );
#endif // WXWIN_COMPATIBILITY_2_6

    wxNotebook *GetNotebook() const { return (wxNotebook *)m_bookctrl; }

private:
    DECLARE_CLASS(wxNotebookSizer)
    DECLARE_NO_COPY_CLASS(wxNotebookSizer)
};

#endif // wxUSE_NOTEBOOK

#endif // wxUSE_BOOKCTRL

#endif // WXWIN_COMPATIBILITY_2_4

// ----------------------------------------------------------------------------
// inline functions implementation
// ----------------------------------------------------------------------------

inline wxSizerItem*
wxSizer::Add( wxSizerItem *item )
{
    return Insert( m_children.GetCount(), item );
}

inline wxSizerItem*
wxSizer::Add( wxWindow *window, int proportion, int flag, int border, wxObject* userData )
{
    return Add( new wxSizerItem( window, proportion, flag, border, userData ) );
}

inline wxSizerItem*
wxSizer::Add( wxSizer *sizer, int proportion, int flag, int border, wxObject* userData )
{
    return Add( new wxSizerItem( sizer, proportion, flag, border, userData ) );
}

inline wxSizerItem*
wxSizer::Add( int width, int height, int proportion, int flag, int border, wxObject* userData )
{
    return Add( new wxSizerItem( width, height, proportion, flag, border, userData ) );
}

inline wxSizerItem*
wxSizer::Add( wxWindow *window, const wxSizerFlags& flags )
{
    return Add( new wxSizerItem(window, flags) );
}

inline wxSizerItem*
wxSizer::Add( wxSizer *sizer, const wxSizerFlags& flags )
{
    return Add( new wxSizerItem(sizer, flags) );
}

inline wxSizerItem*
wxSizer::AddSpacer(int size)
{
    return Add(size, size);
}

inline wxSizerItem*
wxSizer::AddStretchSpacer(int prop)
{
    return Add(0, 0, prop);
}

inline wxSizerItem*
wxSizer::Prepend( wxSizerItem *item )
{
    return Insert( 0, item );
}

inline wxSizerItem*
wxSizer::Prepend( wxWindow *window, int proportion, int flag, int border, wxObject* userData )
{
    return Prepend( new wxSizerItem( window, proportion, flag, border, userData ) );
}

inline wxSizerItem*
wxSizer::Prepend( wxSizer *sizer, int proportion, int flag, int border, wxObject* userData )
{
    return Prepend( new wxSizerItem( sizer, proportion, flag, border, userData ) );
}

inline wxSizerItem*
wxSizer::Prepend( int width, int height, int proportion, int flag, int border, wxObject* userData )
{
    return Prepend( new wxSizerItem( width, height, proportion, flag, border, userData ) );
}

inline wxSizerItem*
wxSizer::PrependSpacer(int size)
{
    return Prepend(size, size);
}

inline wxSizerItem*
wxSizer::PrependStretchSpacer(int prop)
{
    return Prepend(0, 0, prop);
}

inline wxSizerItem*
wxSizer::Prepend( wxWindow *window, const wxSizerFlags& flags )
{
    return Prepend( new wxSizerItem(window, flags) );
}

inline wxSizerItem*
wxSizer::Prepend( wxSizer *sizer, const wxSizerFlags& flags )
{
    return Prepend( new wxSizerItem(sizer, flags) );
}

inline wxSizerItem*
wxSizer::Insert( size_t index,
                 wxWindow *window,
                 int proportion,
                 int flag,
                 int border,
                 wxObject* userData )
{
    return Insert( index, new wxSizerItem( window, proportion, flag, border, userData ) );
}

inline wxSizerItem*
wxSizer::Insert( size_t index,
                 wxSizer *sizer,
                 int proportion,
                 int flag,
                 int border,
                 wxObject* userData )
{
    return Insert( index, new wxSizerItem( sizer, proportion, flag, border, userData ) );
}

inline wxSizerItem*
wxSizer::Insert( size_t index,
                 int width,
                 int height,
                 int proportion,
                 int flag,
                 int border,
                 wxObject* userData )
{
    return Insert( index, new wxSizerItem( width, height, proportion, flag, border, userData ) );
}

inline wxSizerItem*
wxSizer::Insert( size_t index, wxWindow *window, const wxSizerFlags& flags )
{
    return Insert( index, new wxSizerItem(window, flags) );
}

inline wxSizerItem*
wxSizer::Insert( size_t index, wxSizer *sizer, const wxSizerFlags& flags )
{
    return Insert( index, new wxSizerItem(sizer, flags) );
}

inline wxSizerItem*
wxSizer::InsertSpacer(size_t index, int size)
{
    return Insert(index, size, size);
}

inline wxSizerItem*
wxSizer::InsertStretchSpacer(size_t index, int prop)
{
    return Insert(index, 0, 0, prop);
}


#endif // __WXSIZER_H__
