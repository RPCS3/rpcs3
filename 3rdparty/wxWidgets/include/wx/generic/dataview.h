/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/dataview.h
// Purpose:     wxDataViewCtrl generic implementation header
// Author:      Robert Roebling
// Id:          $Id: dataview.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) 1998 Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef __GENERICDATAVIEWCTRLH__
#define __GENERICDATAVIEWCTRLH__

#include "wx/defs.h"
#include "wx/object.h"
#include "wx/list.h"
#include "wx/control.h"
#include "wx/scrolwin.h"
#include "wx/icon.h"

// ---------------------------------------------------------
// classes
// ---------------------------------------------------------

class WXDLLIMPEXP_FWD_ADV wxDataViewCtrl;
class WXDLLIMPEXP_FWD_ADV wxDataViewMainWindow;
class WXDLLIMPEXP_FWD_ADV wxDataViewHeaderWindow;

// ---------------------------------------------------------
// wxDataViewRenderer
// ---------------------------------------------------------

class WXDLLIMPEXP_ADV wxDataViewRenderer: public wxDataViewRendererBase
{
public:
    wxDataViewRenderer( const wxString &varianttype, wxDataViewCellMode mode = wxDATAVIEW_CELL_INERT );
    virtual ~wxDataViewRenderer();

    virtual bool Render( wxRect cell, wxDC *dc, int state ) = 0;
    virtual wxSize GetSize() = 0;

    virtual bool Activate( wxRect WXUNUSED(cell),
                           wxDataViewListModel *WXUNUSED(model),
                           unsigned int WXUNUSED(col),
                           unsigned int WXUNUSED(row) )
                           { return false; }

    virtual bool LeftClick( wxPoint WXUNUSED(cursor),
                            wxRect WXUNUSED(cell),
                            wxDataViewListModel *WXUNUSED(model),
                            unsigned int WXUNUSED(col),
                            unsigned int WXUNUSED(row) )
                            { return false; }
    virtual bool RightClick( wxPoint WXUNUSED(cursor),
                             wxRect WXUNUSED(cell),
                             wxDataViewListModel *WXUNUSED(model),
                             unsigned int WXUNUSED(col),
                             unsigned int WXUNUSED(row) )
                             { return false; }
    virtual bool StartDrag( wxPoint WXUNUSED(cursor),
                            wxRect WXUNUSED(cell),
                            wxDataViewListModel *WXUNUSED(model),
                            unsigned int WXUNUSED(col),
                            unsigned int WXUNUSED(row) )
                            { return false; }

    // Create DC on request
    virtual wxDC *GetDC();

private:
    wxDC        *m_dc;

protected:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxDataViewRenderer)
};

// ---------------------------------------------------------
// wxDataViewCustomRenderer
// ---------------------------------------------------------

class WXDLLIMPEXP_ADV wxDataViewCustomRenderer: public wxDataViewRenderer
{
public:
    wxDataViewCustomRenderer( const wxString &varianttype = wxT("string"),
                              wxDataViewCellMode mode = wxDATAVIEW_CELL_INERT );

protected:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxDataViewCustomRenderer)
};

// ---------------------------------------------------------
// wxDataViewTextRenderer
// ---------------------------------------------------------

class WXDLLIMPEXP_ADV wxDataViewTextRenderer: public wxDataViewCustomRenderer
{
public:
    wxDataViewTextRenderer( const wxString &varianttype = wxT("string"),
                            wxDataViewCellMode mode = wxDATAVIEW_CELL_INERT );

    bool SetValue( const wxVariant &value );
    bool GetValue( wxVariant &value );

    bool Render( wxRect cell, wxDC *dc, int state );
    wxSize GetSize();

private:
    wxString m_text;

protected:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxDataViewTextRenderer)
};

// ---------------------------------------------------------
// wxDataViewBitmapRenderer
// ---------------------------------------------------------

class WXDLLIMPEXP_ADV wxDataViewBitmapRenderer: public wxDataViewCustomRenderer
{
public:
    wxDataViewBitmapRenderer( const wxString &varianttype = wxT("wxBitmap"),
                              wxDataViewCellMode mode = wxDATAVIEW_CELL_INERT );

    bool SetValue( const wxVariant &value );
    bool GetValue( wxVariant &value );

    bool Render( wxRect cell, wxDC *dc, int state );
    wxSize GetSize();

private:
    wxIcon m_icon;
    wxBitmap m_bitmap;

protected:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxDataViewBitmapRenderer)
};

// ---------------------------------------------------------
// wxDataViewToggleRenderer
// ---------------------------------------------------------

class WXDLLIMPEXP_ADV wxDataViewToggleRenderer: public wxDataViewCustomRenderer
{
public:
    wxDataViewToggleRenderer( const wxString &varianttype = wxT("bool"),
                              wxDataViewCellMode mode = wxDATAVIEW_CELL_INERT );

    bool SetValue( const wxVariant &value );
    bool GetValue( wxVariant &value );

    bool Render( wxRect cell, wxDC *dc, int state );
    bool Activate( wxRect cell, wxDataViewListModel *model, unsigned int col, unsigned int row );
    wxSize GetSize();

private:
    bool    m_toggle;

protected:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxDataViewToggleRenderer)
};

// ---------------------------------------------------------
// wxDataViewProgressRenderer
// ---------------------------------------------------------

class WXDLLIMPEXP_ADV wxDataViewProgressRenderer: public wxDataViewCustomRenderer
{
public:
    wxDataViewProgressRenderer( const wxString &label = wxEmptyString,
                                const wxString &varianttype = wxT("long"),
                                wxDataViewCellMode mode = wxDATAVIEW_CELL_INERT );
    virtual ~wxDataViewProgressRenderer();

    bool SetValue( const wxVariant &value );

    virtual bool Render( wxRect cell, wxDC *dc, int state );
    virtual wxSize GetSize();

private:
    wxString    m_label;
    int         m_value;

protected:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxDataViewProgressRenderer)
};

// ---------------------------------------------------------
// wxDataViewDateRenderer
// ---------------------------------------------------------

class WXDLLIMPEXP_ADV wxDataViewDateRenderer: public wxDataViewCustomRenderer
{
public:
    wxDataViewDateRenderer( const wxString &varianttype = wxT("datetime"),
                            wxDataViewCellMode mode = wxDATAVIEW_CELL_ACTIVATABLE );

    bool SetValue( const wxVariant &value );

    virtual bool Render( wxRect cell, wxDC *dc, int state );
    virtual wxSize GetSize();
    virtual bool Activate( wxRect cell,
                           wxDataViewListModel *model, unsigned int col, unsigned int row );

private:
    wxDateTime    m_date;

protected:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxDataViewDateRenderer)
};

// ---------------------------------------------------------
// wxDataViewColumn
// ---------------------------------------------------------

class WXDLLIMPEXP_ADV wxDataViewColumn: public wxDataViewColumnBase
{
public:
    wxDataViewColumn( const wxString &title, wxDataViewRenderer *renderer, unsigned int model_column, 
        int width = 80, int flags = wxDATAVIEW_COL_RESIZABLE );
    wxDataViewColumn( const wxBitmap &bitmap, wxDataViewRenderer *renderer, unsigned int model_column,
        int width = 80, int flags = wxDATAVIEW_COL_RESIZABLE );
    virtual ~wxDataViewColumn();

    virtual void SetTitle( const wxString &title );
    virtual void SetBitmap( const wxBitmap &bitmap );
    
    virtual void SetAlignment( wxAlignment align );
    
    virtual void SetSortable( bool sortable );
    virtual bool GetSortable();
    virtual void SetSortOrder( bool ascending );
    virtual bool IsSortOrderAscending();

    virtual int GetWidth();

private:
    int                      m_width;
    int                      m_fixedWidth;

protected:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxDataViewColumn)
};

// ---------------------------------------------------------
// wxDataViewCtrl
// ---------------------------------------------------------

class WXDLLIMPEXP_ADV wxDataViewCtrl: public wxDataViewCtrlBase,
                                       public wxScrollHelperNative
{
public:
    wxDataViewCtrl() : wxScrollHelperNative(this)
    {
        Init();
    }

    wxDataViewCtrl( wxWindow *parent, wxWindowID id,
           const wxPoint& pos = wxDefaultPosition,
           const wxSize& size = wxDefaultSize, long style = 0,
           const wxValidator& validator = wxDefaultValidator )
             : wxScrollHelperNative(this)
    {
        Create(parent, id, pos, size, style, validator );
    }

    virtual ~wxDataViewCtrl();

    void Init();

    bool Create(wxWindow *parent, wxWindowID id,
           const wxPoint& pos = wxDefaultPosition,
           const wxSize& size = wxDefaultSize, long style = 0,
           const wxValidator& validator = wxDefaultValidator );

    virtual bool AssociateModel( wxDataViewListModel *model );
    virtual bool AppendColumn( wxDataViewColumn *col );

    virtual void SetSelection( int row ); // -1 for unselect
    virtual void SetSelectionRange( unsigned int from, unsigned int to );
    virtual void SetSelections( const wxArrayInt& aSelections);
    virtual void Unselect( unsigned int row );
    
    virtual bool IsSelected( unsigned int row ) const;
    virtual int GetSelection() const;
    virtual int GetSelections(wxArrayInt& aSelections) const;

private:
    friend class wxDataViewMainWindow;
    friend class wxDataViewHeaderWindow;
    wxDataViewListModelNotifier *m_notifier;
    wxDataViewMainWindow        *m_clientArea;
    wxDataViewHeaderWindow      *m_headerArea;

private:
    void OnSize( wxSizeEvent &event );

    // we need to return a special WM_GETDLGCODE value to process just the
    // arrows but let the other navigation characters through
#ifdef __WXMSW__
    virtual WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam);
#endif // __WXMSW__

    WX_FORWARD_TO_SCROLL_HELPER()

private:
    DECLARE_DYNAMIC_CLASS(wxDataViewCtrl)
    DECLARE_NO_COPY_CLASS(wxDataViewCtrl)
    DECLARE_EVENT_TABLE()
};


#endif // __GENERICDATAVIEWCTRLH__
