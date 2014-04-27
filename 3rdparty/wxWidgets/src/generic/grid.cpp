///////////////////////////////////////////////////////////////////////////
// Name:        src/generic/grid.cpp
// Purpose:     wxGrid and related classes
// Author:      Michael Bedward (based on code by Julian Smart, Robin Dunn)
// Modified by: Robin Dunn, Vadim Zeitlin, Santiago Palacios
// Created:     1/08/1999
// RCS-ID:      $Id: grid.cpp 58753 2009-02-08 10:23:19Z VZ $
// Copyright:   (c) Michael Bedward (mbedward@ozemail.com.au)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_GRID

#include "wx/grid.h"

#ifndef WX_PRECOMP
    #include "wx/utils.h"
    #include "wx/dcclient.h"
    #include "wx/settings.h"
    #include "wx/log.h"
    #include "wx/textctrl.h"
    #include "wx/checkbox.h"
    #include "wx/combobox.h"
    #include "wx/valtext.h"
    #include "wx/intl.h"
    #include "wx/math.h"
    #include "wx/listbox.h"
#endif

#include "wx/textfile.h"
#include "wx/spinctrl.h"
#include "wx/tokenzr.h"
#include "wx/renderer.h"

#include "wx/generic/gridsel.h"

const wxChar wxGridNameStr[] = wxT("grid");

#if defined(__WXMOTIF__)
    #define WXUNUSED_MOTIF(identifier)  WXUNUSED(identifier)
#else
    #define WXUNUSED_MOTIF(identifier)  identifier
#endif

#if defined(__WXGTK__)
    #define WXUNUSED_GTK(identifier)    WXUNUSED(identifier)
#else
    #define WXUNUSED_GTK(identifier)    identifier
#endif

// Required for wxIs... functions
#include <ctype.h>

// ----------------------------------------------------------------------------
// array classes
// ----------------------------------------------------------------------------

WX_DEFINE_ARRAY_WITH_DECL_PTR(wxGridCellAttr *, wxArrayAttrs,
                                 class WXDLLIMPEXP_ADV);

struct wxGridCellWithAttr
{
    wxGridCellWithAttr(int row, int col, wxGridCellAttr *attr_)
        : coords(row, col), attr(attr_)
    {
        wxASSERT( attr );
    }

    wxGridCellWithAttr(const wxGridCellWithAttr& other)
        : coords(other.coords),
          attr(other.attr)
    {
        attr->IncRef();
    }

    wxGridCellWithAttr& operator=(const wxGridCellWithAttr& other)
    {
        coords = other.coords;
        if (attr != other.attr)
        {
            attr->DecRef();
            attr = other.attr;
            attr->IncRef();
        }
        return *this;
    }

    void ChangeAttr(wxGridCellAttr * new_attr)
    {
        if (attr != new_attr)
        {
            // "Delete" (i.e. DecRef) the old attribute.
            attr->DecRef();
            attr = new_attr;
            // Take ownership of the new attribute, i.e. no IncRef.
        }
    }

    ~wxGridCellWithAttr()
    {
        attr->DecRef();
    }

    wxGridCellCoords coords;
    wxGridCellAttr  *attr;
};

WX_DECLARE_OBJARRAY_WITH_DECL(wxGridCellWithAttr, wxGridCellWithAttrArray,
                              class WXDLLIMPEXP_ADV);

#include "wx/arrimpl.cpp"

WX_DEFINE_OBJARRAY(wxGridCellCoordsArray)
WX_DEFINE_OBJARRAY(wxGridCellWithAttrArray)

// ----------------------------------------------------------------------------
// events
// ----------------------------------------------------------------------------

DEFINE_EVENT_TYPE(wxEVT_GRID_CELL_LEFT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_GRID_CELL_RIGHT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_GRID_CELL_LEFT_DCLICK)
DEFINE_EVENT_TYPE(wxEVT_GRID_CELL_RIGHT_DCLICK)
DEFINE_EVENT_TYPE(wxEVT_GRID_CELL_BEGIN_DRAG)
DEFINE_EVENT_TYPE(wxEVT_GRID_LABEL_LEFT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_GRID_LABEL_RIGHT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_GRID_LABEL_LEFT_DCLICK)
DEFINE_EVENT_TYPE(wxEVT_GRID_LABEL_RIGHT_DCLICK)
DEFINE_EVENT_TYPE(wxEVT_GRID_ROW_SIZE)
DEFINE_EVENT_TYPE(wxEVT_GRID_COL_SIZE)
DEFINE_EVENT_TYPE(wxEVT_GRID_COL_MOVE)
DEFINE_EVENT_TYPE(wxEVT_GRID_RANGE_SELECT)
DEFINE_EVENT_TYPE(wxEVT_GRID_CELL_CHANGE)
DEFINE_EVENT_TYPE(wxEVT_GRID_SELECT_CELL)
DEFINE_EVENT_TYPE(wxEVT_GRID_EDITOR_SHOWN)
DEFINE_EVENT_TYPE(wxEVT_GRID_EDITOR_HIDDEN)
DEFINE_EVENT_TYPE(wxEVT_GRID_EDITOR_CREATED)

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxGridRowLabelWindow : public wxWindow
{
public:
    wxGridRowLabelWindow() { m_owner = (wxGrid *)NULL; }
    wxGridRowLabelWindow( wxGrid *parent, wxWindowID id,
                          const wxPoint &pos, const wxSize &size );

    virtual bool AcceptsFocus() const { return false; }

private:
    wxGrid   *m_owner;

    void OnPaint( wxPaintEvent& event );
    void OnMouseEvent( wxMouseEvent& event );
    void OnMouseWheel( wxMouseEvent& event );
    void OnKeyDown( wxKeyEvent& event );
    void OnKeyUp( wxKeyEvent& );
    void OnChar( wxKeyEvent& );

    DECLARE_DYNAMIC_CLASS(wxGridRowLabelWindow)
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxGridRowLabelWindow)
};


class WXDLLIMPEXP_ADV wxGridColLabelWindow : public wxWindow
{
public:
    wxGridColLabelWindow() { m_owner = (wxGrid *)NULL; }
    wxGridColLabelWindow( wxGrid *parent, wxWindowID id,
                          const wxPoint &pos, const wxSize &size );

    virtual bool AcceptsFocus() const { return false; }

private:
    wxGrid   *m_owner;

    void OnPaint( wxPaintEvent& event );
    void OnMouseEvent( wxMouseEvent& event );
    void OnMouseWheel( wxMouseEvent& event );
    void OnKeyDown( wxKeyEvent& event );
    void OnKeyUp( wxKeyEvent& );
    void OnChar( wxKeyEvent& );

    DECLARE_DYNAMIC_CLASS(wxGridColLabelWindow)
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxGridColLabelWindow)
};


class WXDLLIMPEXP_ADV wxGridCornerLabelWindow : public wxWindow
{
public:
    wxGridCornerLabelWindow() { m_owner = (wxGrid *)NULL; }
    wxGridCornerLabelWindow( wxGrid *parent, wxWindowID id,
                             const wxPoint &pos, const wxSize &size );

    virtual bool AcceptsFocus() const { return false; }

private:
    wxGrid *m_owner;

    void OnMouseEvent( wxMouseEvent& event );
    void OnMouseWheel( wxMouseEvent& event );
    void OnKeyDown( wxKeyEvent& event );
    void OnKeyUp( wxKeyEvent& );
    void OnChar( wxKeyEvent& );
    void OnPaint( wxPaintEvent& event );

    DECLARE_DYNAMIC_CLASS(wxGridCornerLabelWindow)
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxGridCornerLabelWindow)
};

class WXDLLIMPEXP_ADV wxGridWindow : public wxWindow
{
public:
    wxGridWindow()
    {
        m_owner = NULL;
        m_rowLabelWin = NULL;
        m_colLabelWin = NULL;
    }

    wxGridWindow( wxGrid *parent,
                  wxGridRowLabelWindow *rowLblWin,
                  wxGridColLabelWindow *colLblWin,
                  wxWindowID id, const wxPoint &pos, const wxSize &size );
    virtual ~wxGridWindow() {}

    void ScrollWindow( int dx, int dy, const wxRect *rect );

    wxGrid* GetOwner() { return m_owner; }

private:
    wxGrid                   *m_owner;
    wxGridRowLabelWindow     *m_rowLabelWin;
    wxGridColLabelWindow     *m_colLabelWin;

    void OnPaint( wxPaintEvent &event );
    void OnMouseWheel( wxMouseEvent& event );
    void OnMouseEvent( wxMouseEvent& event );
    void OnKeyDown( wxKeyEvent& );
    void OnKeyUp( wxKeyEvent& );
    void OnChar( wxKeyEvent& );
    void OnEraseBackground( wxEraseEvent& );
    void OnFocus( wxFocusEvent& );

    DECLARE_DYNAMIC_CLASS(wxGridWindow)
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxGridWindow)
};


class wxGridCellEditorEvtHandler : public wxEvtHandler
{
public:
    wxGridCellEditorEvtHandler(wxGrid* grid, wxGridCellEditor* editor)
        : m_grid(grid),
          m_editor(editor),
          m_inSetFocus(false)
    {
    }

    void OnKillFocus(wxFocusEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnChar(wxKeyEvent& event);

    void SetInSetFocus(bool inSetFocus) { m_inSetFocus = inSetFocus; }

private:
    wxGrid             *m_grid;
    wxGridCellEditor   *m_editor;

    // Work around the fact that a focus kill event can be sent to
    // a combobox within a set focus event.
    bool                m_inSetFocus;

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxGridCellEditorEvtHandler)
    DECLARE_NO_COPY_CLASS(wxGridCellEditorEvtHandler)
};


IMPLEMENT_ABSTRACT_CLASS(wxGridCellEditorEvtHandler, wxEvtHandler)

BEGIN_EVENT_TABLE( wxGridCellEditorEvtHandler, wxEvtHandler )
    EVT_KILL_FOCUS( wxGridCellEditorEvtHandler::OnKillFocus )
    EVT_KEY_DOWN( wxGridCellEditorEvtHandler::OnKeyDown )
    EVT_CHAR( wxGridCellEditorEvtHandler::OnChar )
END_EVENT_TABLE()


// ----------------------------------------------------------------------------
// the internal data representation used by wxGridCellAttrProvider
// ----------------------------------------------------------------------------

// this class stores attributes set for cells
class WXDLLIMPEXP_ADV wxGridCellAttrData
{
public:
    void SetAttr(wxGridCellAttr *attr, int row, int col);
    wxGridCellAttr *GetAttr(int row, int col) const;
    void UpdateAttrRows( size_t pos, int numRows );
    void UpdateAttrCols( size_t pos, int numCols );

private:
    // searches for the attr for given cell, returns wxNOT_FOUND if not found
    int FindIndex(int row, int col) const;

    wxGridCellWithAttrArray m_attrs;
};

// this class stores attributes set for rows or columns
class WXDLLIMPEXP_ADV wxGridRowOrColAttrData
{
public:
    // empty ctor to suppress warnings
    wxGridRowOrColAttrData() {}
    ~wxGridRowOrColAttrData();

    void SetAttr(wxGridCellAttr *attr, int rowOrCol);
    wxGridCellAttr *GetAttr(int rowOrCol) const;
    void UpdateAttrRowsOrCols( size_t pos, int numRowsOrCols );

private:
    wxArrayInt m_rowsOrCols;
    wxArrayAttrs m_attrs;
};

// NB: this is just a wrapper around 3 objects: one which stores cell
//     attributes, and 2 others for row/col ones
class WXDLLIMPEXP_ADV wxGridCellAttrProviderData
{
public:
    wxGridCellAttrData m_cellAttrs;
    wxGridRowOrColAttrData m_rowAttrs,
                           m_colAttrs;
};


// ----------------------------------------------------------------------------
// data structures used for the data type registry
// ----------------------------------------------------------------------------

struct wxGridDataTypeInfo
{
    wxGridDataTypeInfo(const wxString& typeName,
                       wxGridCellRenderer* renderer,
                       wxGridCellEditor* editor)
        : m_typeName(typeName), m_renderer(renderer), m_editor(editor)
        {}

    ~wxGridDataTypeInfo()
    {
        wxSafeDecRef(m_renderer);
        wxSafeDecRef(m_editor);
    }

    wxString            m_typeName;
    wxGridCellRenderer* m_renderer;
    wxGridCellEditor*   m_editor;

    DECLARE_NO_COPY_CLASS(wxGridDataTypeInfo)
};


WX_DEFINE_ARRAY_WITH_DECL_PTR(wxGridDataTypeInfo*, wxGridDataTypeInfoArray,
                                 class WXDLLIMPEXP_ADV);


class WXDLLIMPEXP_ADV wxGridTypeRegistry
{
public:
  wxGridTypeRegistry() {}
    ~wxGridTypeRegistry();

    void RegisterDataType(const wxString& typeName,
                     wxGridCellRenderer* renderer,
                     wxGridCellEditor* editor);

    // find one of already registered data types
    int FindRegisteredDataType(const wxString& typeName);

    // try to FindRegisteredDataType(), if this fails and typeName is one of
    // standard typenames, register it and return its index
    int FindDataType(const wxString& typeName);

    // try to FindDataType(), if it fails see if it is not one of already
    // registered data types with some params in which case clone the
    // registered data type and set params for it
    int FindOrCloneDataType(const wxString& typeName);

    wxGridCellRenderer* GetRenderer(int index);
    wxGridCellEditor*   GetEditor(int index);

private:
    wxGridDataTypeInfoArray m_typeinfo;
};


// ----------------------------------------------------------------------------
// conditional compilation
// ----------------------------------------------------------------------------

#ifndef WXGRID_DRAW_LINES
#define WXGRID_DRAW_LINES 1
#endif

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

//#define DEBUG_ATTR_CACHE
#ifdef DEBUG_ATTR_CACHE
    static size_t gs_nAttrCacheHits = 0;
    static size_t gs_nAttrCacheMisses = 0;
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

wxGridCellCoords wxGridNoCellCoords( -1, -1 );
wxRect wxGridNoCellRect( -1, -1, -1, -1 );

// scroll line size
// TODO: this doesn't work at all, grid cells have different sizes and approx
//       calculations don't work as because of the size mismatch scrollbars
//       sometimes fail to be shown when they should be or vice versa
//
//       The scroll bars may be a little flakey once in a while, but that is
//       surely much less horrible than having scroll lines of only 1!!!
//       -- Robin
//
//       Well, it's still seriously broken so it might be better but needs
//       fixing anyhow
//       -- Vadim
static const size_t GRID_SCROLL_LINE_X = 15;  // 1;
static const size_t GRID_SCROLL_LINE_Y = GRID_SCROLL_LINE_X;

// the size of hash tables used a bit everywhere (the max number of elements
// in these hash tables is the number of rows/columns)
static const int GRID_HASH_SIZE = 100;

#if 0
// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

static inline int GetScrollX(int x)
{
    return (x + GRID_SCROLL_LINE_X - 1) / GRID_SCROLL_LINE_X;
}

static inline int GetScrollY(int y)
{
    return (y + GRID_SCROLL_LINE_Y - 1) / GRID_SCROLL_LINE_Y;
}
#endif

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxGridCellEditor
// ----------------------------------------------------------------------------

wxGridCellEditor::wxGridCellEditor()
{
    m_control = NULL;
    m_attr = NULL;
}

wxGridCellEditor::~wxGridCellEditor()
{
    Destroy();
}

void wxGridCellEditor::Create(wxWindow* WXUNUSED(parent),
                              wxWindowID WXUNUSED(id),
                              wxEvtHandler* evtHandler)
{
    if ( evtHandler )
        m_control->PushEventHandler(evtHandler);
}

void wxGridCellEditor::PaintBackground(const wxRect& rectCell,
                                       wxGridCellAttr *attr)
{
    // erase the background because we might not fill the cell
    wxClientDC dc(m_control->GetParent());
    wxGridWindow* gridWindow = wxDynamicCast(m_control->GetParent(), wxGridWindow);
    if (gridWindow)
        gridWindow->GetOwner()->PrepareDC(dc);

    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(attr->GetBackgroundColour(), wxSOLID));
    dc.DrawRectangle(rectCell);

    // redraw the control we just painted over
    m_control->Refresh();
}

void wxGridCellEditor::Destroy()
{
    if (m_control)
    {
        m_control->PopEventHandler( true /* delete it*/ );

        m_control->Destroy();
        m_control = NULL;
    }
}

void wxGridCellEditor::Show(bool show, wxGridCellAttr *attr)
{
    wxASSERT_MSG(m_control, wxT("The wxGridCellEditor must be created first!"));

    m_control->Show(show);

    if ( show )
    {
        // set the colours/fonts if we have any
        if ( attr )
        {
            m_colFgOld = m_control->GetForegroundColour();
            m_control->SetForegroundColour(attr->GetTextColour());

            m_colBgOld = m_control->GetBackgroundColour();
            m_control->SetBackgroundColour(attr->GetBackgroundColour());

// Workaround for GTK+1 font setting problem on some platforms
#if !defined(__WXGTK__) || defined(__WXGTK20__)
            m_fontOld = m_control->GetFont();
            m_control->SetFont(attr->GetFont());
#endif

            // can't do anything more in the base class version, the other
            // attributes may only be used by the derived classes
        }
    }
    else
    {
        // restore the standard colours fonts
        if ( m_colFgOld.Ok() )
        {
            m_control->SetForegroundColour(m_colFgOld);
            m_colFgOld = wxNullColour;
        }

        if ( m_colBgOld.Ok() )
        {
            m_control->SetBackgroundColour(m_colBgOld);
            m_colBgOld = wxNullColour;
        }

// Workaround for GTK+1 font setting problem on some platforms
#if !defined(__WXGTK__) || defined(__WXGTK20__)
        if ( m_fontOld.Ok() )
        {
            m_control->SetFont(m_fontOld);
            m_fontOld = wxNullFont;
        }
#endif
    }
}

void wxGridCellEditor::SetSize(const wxRect& rect)
{
    wxASSERT_MSG(m_control, wxT("The wxGridCellEditor must be created first!"));

    m_control->SetSize(rect, wxSIZE_ALLOW_MINUS_ONE);
}

void wxGridCellEditor::HandleReturn(wxKeyEvent& event)
{
    event.Skip();
}

bool wxGridCellEditor::IsAcceptedKey(wxKeyEvent& event)
{
    bool ctrl = event.ControlDown();
    bool alt  = event.AltDown();

#ifdef __WXMAC__
    // On the Mac the Alt key is more like shift and is used for entry of
    // valid characters, so check for Ctrl and Meta instead.
    alt = event.MetaDown();
#endif

    // Assume it's not a valid char if ctrl or alt is down, but if both are
    // down then it may be because of an AltGr key combination, so let them
    // through in that case.
    if ((ctrl || alt) && !(ctrl && alt))
        return false;

    int key = 0;
    bool keyOk = true;

#ifdef __WXGTK20__
    // If it's a F-Key or other special key then it shouldn't start the
    // editor.
    if (event.GetKeyCode() >= WXK_START)
        return false;
#endif
#if wxUSE_UNICODE
    // if the unicode key code is not really a unicode character (it may
    // be a function key or etc., the platforms appear to always give us a
    // small value in this case) then fallback to the ASCII key code but
    // don't do anything for function keys or etc.
    key = event.GetUnicodeKey();
    if (key <= 127)
    {
        key = event.GetKeyCode();
        keyOk = (key <= 127);
    }
#else
    key = event.GetKeyCode();
    keyOk = (key <= 255);
#endif

    return keyOk;
}

void wxGridCellEditor::StartingKey(wxKeyEvent& event)
{
    event.Skip();
}

void wxGridCellEditor::StartingClick()
{
}

#if wxUSE_TEXTCTRL

// ----------------------------------------------------------------------------
// wxGridCellTextEditor
// ----------------------------------------------------------------------------

wxGridCellTextEditor::wxGridCellTextEditor()
{
    m_maxChars = 0;
}

void wxGridCellTextEditor::Create(wxWindow* parent,
                                  wxWindowID id,
                                  wxEvtHandler* evtHandler)
{
    m_control = new wxTextCtrl(parent, id, wxEmptyString,
                               wxDefaultPosition, wxDefaultSize
#if defined(__WXMSW__)
                               ,
                               wxTE_PROCESS_ENTER |
                               wxTE_PROCESS_TAB |
                               wxTE_AUTO_SCROLL |
                               wxNO_BORDER
#endif
                              );

    // set max length allowed in the textctrl, if the parameter was set
    if (m_maxChars != 0)
    {
        ((wxTextCtrl*)m_control)->SetMaxLength(m_maxChars);
    }

    wxGridCellEditor::Create(parent, id, evtHandler);
}

void wxGridCellTextEditor::PaintBackground(const wxRect& WXUNUSED(rectCell),
                                           wxGridCellAttr * WXUNUSED(attr))
{
    // as we fill the entire client area,
    // don't do anything here to minimize flicker
}

void wxGridCellTextEditor::SetSize(const wxRect& rectOrig)
{
    wxRect rect(rectOrig);

    // Make the edit control large enough to allow for internal margins
    //
    // TODO: remove this if the text ctrl sizing is improved esp. for unix
    //
#if defined(__WXGTK__)
    if (rect.x != 0)
    {
        rect.x += 1;
        rect.y += 1;
        rect.width -= 1;
        rect.height -= 1;
    }
#elif defined(__WXMSW__)
    if ( rect.x == 0 )
        rect.x += 2;
    else
        rect.x += 3;

    if ( rect.y == 0 )
        rect.y += 2;
    else
        rect.y += 3;

    rect.width -= 2;
    rect.height -= 2;
#else
    int extra_x = ( rect.x > 2 ) ? 2 : 1;
    int extra_y = ( rect.y > 2 ) ? 2 : 1;

    #if defined(__WXMOTIF__)
        extra_x *= 2;
        extra_y *= 2;
    #endif

    rect.SetLeft( wxMax(0, rect.x - extra_x) );
    rect.SetTop( wxMax(0, rect.y - extra_y) );
    rect.SetRight( rect.GetRight() + 2 * extra_x );
    rect.SetBottom( rect.GetBottom() + 2 * extra_y );
#endif

    wxGridCellEditor::SetSize(rect);
}

void wxGridCellTextEditor::BeginEdit(int row, int col, wxGrid* grid)
{
    wxASSERT_MSG(m_control, wxT("The wxGridCellEditor must be created first!"));

    m_startValue = grid->GetTable()->GetValue(row, col);

    DoBeginEdit(m_startValue);
}

void wxGridCellTextEditor::DoBeginEdit(const wxString& startValue)
{
    Text()->SetValue(startValue);
    Text()->SetInsertionPointEnd();
    Text()->SetSelection(-1, -1);
    Text()->SetFocus();
}

bool wxGridCellTextEditor::EndEdit(int row, int col, wxGrid* grid)
{
    wxASSERT_MSG(m_control, wxT("The wxGridCellEditor must be created first!"));

    bool changed = false;
    wxString value = Text()->GetValue();
    if (value != m_startValue)
        changed = true;

    if (changed)
        grid->GetTable()->SetValue(row, col, value);

    m_startValue = wxEmptyString;

    // No point in setting the text of the hidden control
    //Text()->SetValue(m_startValue);

    return changed;
}

void wxGridCellTextEditor::Reset()
{
    wxASSERT_MSG(m_control, wxT("The wxGridCellEditor must be created first!"));

    DoReset(m_startValue);
}

void wxGridCellTextEditor::DoReset(const wxString& startValue)
{
    Text()->SetValue(startValue);
    Text()->SetInsertionPointEnd();
}

bool wxGridCellTextEditor::IsAcceptedKey(wxKeyEvent& event)
{
    return wxGridCellEditor::IsAcceptedKey(event);
}

void wxGridCellTextEditor::StartingKey(wxKeyEvent& event)
{
    // Since this is now happening in the EVT_CHAR event EmulateKeyPress is no
    // longer an appropriate way to get the character into the text control.
    // Do it ourselves instead.  We know that if we get this far that we have
    // a valid character, so not a whole lot of testing needs to be done.

    wxTextCtrl* tc = Text();
    wxChar ch;
    long pos;

#if wxUSE_UNICODE
    ch = event.GetUnicodeKey();
    if (ch <= 127)
        ch = (wxChar)event.GetKeyCode();
#else
    ch = (wxChar)event.GetKeyCode();
#endif

    switch (ch)
    {
        case WXK_DELETE:
            // delete the character at the cursor
            pos = tc->GetInsertionPoint();
            if (pos < tc->GetLastPosition())
                tc->Remove(pos, pos + 1);
            break;

        case WXK_BACK:
            // delete the character before the cursor
            pos = tc->GetInsertionPoint();
            if (pos > 0)
                tc->Remove(pos - 1, pos);
            break;

        default:
            tc->WriteText(ch);
            break;
    }
}

void wxGridCellTextEditor::HandleReturn( wxKeyEvent&
                                         WXUNUSED_GTK(WXUNUSED_MOTIF(event)) )
{
#if defined(__WXMOTIF__) || defined(__WXGTK__)
    // wxMotif needs a little extra help...
    size_t pos = (size_t)( Text()->GetInsertionPoint() );
    wxString s( Text()->GetValue() );
    s = s.Left(pos) + wxT("\n") + s.Mid(pos);
    Text()->SetValue(s);
    Text()->SetInsertionPoint( pos );
#else
    // the other ports can handle a Return key press
    //
    event.Skip();
#endif
}

void wxGridCellTextEditor::SetParameters(const wxString& params)
{
    if ( !params )
    {
        // reset to default
        m_maxChars = 0;
    }
    else
    {
        long tmp;
        if ( params.ToLong(&tmp) )
        {
            m_maxChars = (size_t)tmp;
        }
        else
        {
            wxLogDebug( _T("Invalid wxGridCellTextEditor parameter string '%s' ignored"), params.c_str() );
        }
    }
}

// return the value in the text control
wxString wxGridCellTextEditor::GetValue() const
{
    return Text()->GetValue();
}

// ----------------------------------------------------------------------------
// wxGridCellNumberEditor
// ----------------------------------------------------------------------------

wxGridCellNumberEditor::wxGridCellNumberEditor(int min, int max)
{
    m_min = min;
    m_max = max;
}

void wxGridCellNumberEditor::Create(wxWindow* parent,
                                    wxWindowID id,
                                    wxEvtHandler* evtHandler)
{
#if wxUSE_SPINCTRL
    if ( HasRange() )
    {
        // create a spin ctrl
        m_control = new wxSpinCtrl(parent, wxID_ANY, wxEmptyString,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxSP_ARROW_KEYS,
                                   m_min, m_max);

        wxGridCellEditor::Create(parent, id, evtHandler);
    }
    else
#endif
    {
        // just a text control
        wxGridCellTextEditor::Create(parent, id, evtHandler);

#if wxUSE_VALIDATORS
        Text()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
#endif
    }
}

void wxGridCellNumberEditor::BeginEdit(int row, int col, wxGrid* grid)
{
    // first get the value
    wxGridTableBase *table = grid->GetTable();
    if ( table->CanGetValueAs(row, col, wxGRID_VALUE_NUMBER) )
    {
        m_valueOld = table->GetValueAsLong(row, col);
    }
    else
    {
        m_valueOld = 0;
        wxString sValue = table->GetValue(row, col);
        if (! sValue.ToLong(&m_valueOld) && ! sValue.empty())
        {
            wxFAIL_MSG( _T("this cell doesn't have numeric value") );
            return;
        }
    }

#if wxUSE_SPINCTRL
    if ( HasRange() )
    {
        Spin()->SetValue((int)m_valueOld);
        Spin()->SetFocus();
    }
    else
#endif
    {
        DoBeginEdit(GetString());
    }
}

bool wxGridCellNumberEditor::EndEdit(int row, int col,
                                     wxGrid* grid)
{
    long value = 0;
    wxString text;

#if wxUSE_SPINCTRL
    if ( HasRange() )
    {
        value = Spin()->GetValue();
        if ( value == m_valueOld )
            return false;

        text.Printf(wxT("%ld"), value);
    }
    else // using unconstrained input
#endif // wxUSE_SPINCTRL
    {
        const wxString textOld(grid->GetCellValue(row, col));
        text = Text()->GetValue();
        if ( text.empty() )
        {
            if ( textOld.empty() )
                return false;
        }
        else // non-empty text now (maybe 0)
        {
            if ( !text.ToLong(&value) )
                return false;

            // if value == m_valueOld == 0 but old text was "" and new one is
            // "0" something still did change
            if ( value == m_valueOld && (value || !textOld.empty()) )
                return false;
        }
    }

    wxGridTableBase * const table = grid->GetTable();
    if ( table->CanSetValueAs(row, col, wxGRID_VALUE_NUMBER) )
        table->SetValueAsLong(row, col, value);
    else
        table->SetValue(row, col, text);

    return true;
}

void wxGridCellNumberEditor::Reset()
{
#if wxUSE_SPINCTRL
    if ( HasRange() )
    {
        Spin()->SetValue((int)m_valueOld);
    }
    else
#endif
    {
        DoReset(GetString());
    }
}

bool wxGridCellNumberEditor::IsAcceptedKey(wxKeyEvent& event)
{
    if ( wxGridCellEditor::IsAcceptedKey(event) )
    {
        int keycode = event.GetKeyCode();
        if ( (keycode < 128) &&
             (wxIsdigit(keycode) || keycode == '+' || keycode == '-'))
        {
            return true;
        }
    }

    return false;
}

void wxGridCellNumberEditor::StartingKey(wxKeyEvent& event)
{
    int keycode = event.GetKeyCode();
    if ( !HasRange() )
    {
        if ( wxIsdigit(keycode) || keycode == '+' || keycode == '-')
        {
            wxGridCellTextEditor::StartingKey(event);

            // skip Skip() below
            return;
        }
    }
#if wxUSE_SPINCTRL
    else
    {
        if ( wxIsdigit(keycode) )
        {
            wxSpinCtrl* spin = (wxSpinCtrl*)m_control;
            spin->SetValue(keycode - '0');
            spin->SetSelection(1,1);
            return;
        }
    }
#endif

    event.Skip();
}

void wxGridCellNumberEditor::SetParameters(const wxString& params)
{
    if ( !params )
    {
        // reset to default
        m_min =
        m_max = -1;
    }
    else
    {
        long tmp;
        if ( params.BeforeFirst(_T(',')).ToLong(&tmp) )
        {
            m_min = (int)tmp;

            if ( params.AfterFirst(_T(',')).ToLong(&tmp) )
            {
                m_max = (int)tmp;

                // skip the error message below
                return;
            }
        }

        wxLogDebug(_T("Invalid wxGridCellNumberEditor parameter string '%s' ignored"), params.c_str());
    }
}

// return the value in the spin control if it is there (the text control otherwise)
wxString wxGridCellNumberEditor::GetValue() const
{
    wxString s;

#if wxUSE_SPINCTRL
    if ( HasRange() )
    {
        long value = Spin()->GetValue();
        s.Printf(wxT("%ld"), value);
    }
    else
#endif
    {
        s = Text()->GetValue();
    }

    return s;
}

// ----------------------------------------------------------------------------
// wxGridCellFloatEditor
// ----------------------------------------------------------------------------

wxGridCellFloatEditor::wxGridCellFloatEditor(int width, int precision)
{
    m_width = width;
    m_precision = precision;
}

void wxGridCellFloatEditor::Create(wxWindow* parent,
                                   wxWindowID id,
                                   wxEvtHandler* evtHandler)
{
    wxGridCellTextEditor::Create(parent, id, evtHandler);

#if wxUSE_VALIDATORS
    Text()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
#endif
}

void wxGridCellFloatEditor::BeginEdit(int row, int col, wxGrid* grid)
{
    // first get the value
    wxGridTableBase * const table = grid->GetTable();
    if ( table->CanGetValueAs(row, col, wxGRID_VALUE_FLOAT) )
    {
        m_valueOld = table->GetValueAsDouble(row, col);
    }
    else
    {
        m_valueOld = 0.0;

        const wxString value = table->GetValue(row, col);
        if ( !value.empty() )
        {
            if ( !value.ToDouble(&m_valueOld) )
            {
                wxFAIL_MSG( _T("this cell doesn't have float value") );
                return;
            }
        }
    }

    DoBeginEdit(GetString());
}

bool wxGridCellFloatEditor::EndEdit(int row, int col, wxGrid* grid)
{
    const wxString text(Text()->GetValue()),
                   textOld(grid->GetCellValue(row, col));

    double value;
    if ( !text.empty() )
    {
        if ( !text.ToDouble(&value) )
            return false;
    }
    else // new value is empty string
    {
        if ( textOld.empty() )
            return false;           // nothing changed

        value = 0.;
    }

    // the test for empty strings ensures that we don't skip the value setting
    // when "" is replaced by "0" or vice versa as "" numeric value is also 0.
    if ( wxIsSameDouble(value, m_valueOld) && !text.empty() && !textOld.empty() )
        return false;           // nothing changed

    wxGridTableBase * const table = grid->GetTable();

    if ( table->CanSetValueAs(row, col, wxGRID_VALUE_FLOAT) )
        table->SetValueAsDouble(row, col, value);
    else
        table->SetValue(row, col, text);

    return true;
}

void wxGridCellFloatEditor::Reset()
{
    DoReset(GetString());
}

void wxGridCellFloatEditor::StartingKey(wxKeyEvent& event)
{
    int keycode = event.GetKeyCode();
    char tmpbuf[2];
    tmpbuf[0] = (char) keycode;
    tmpbuf[1] = '\0';
    wxString strbuf(tmpbuf, *wxConvCurrent);

#if wxUSE_INTL
    bool is_decimal_point = ( strbuf ==
       wxLocale::GetInfo(wxLOCALE_DECIMAL_POINT, wxLOCALE_CAT_NUMBER) );
#else
    bool is_decimal_point = ( strbuf == _T(".") );
#endif

    if ( wxIsdigit(keycode) || keycode == '+' || keycode == '-'
         || is_decimal_point )
    {
        wxGridCellTextEditor::StartingKey(event);

        // skip Skip() below
        return;
    }

    event.Skip();
}

void wxGridCellFloatEditor::SetParameters(const wxString& params)
{
    if ( !params )
    {
        // reset to default
        m_width =
        m_precision = -1;
    }
    else
    {
        long tmp;
        if ( params.BeforeFirst(_T(',')).ToLong(&tmp) )
        {
            m_width = (int)tmp;

            if ( params.AfterFirst(_T(',')).ToLong(&tmp) )
            {
                m_precision = (int)tmp;

                // skip the error message below
                return;
            }
        }

        wxLogDebug(_T("Invalid wxGridCellFloatEditor parameter string '%s' ignored"), params.c_str());
    }
}

wxString wxGridCellFloatEditor::GetString() const
{
    wxString fmt;
    if ( m_precision == -1 && m_width != -1)
    {
        // default precision
        fmt.Printf(_T("%%%d.f"), m_width);
    }
    else if ( m_precision != -1 && m_width == -1)
    {
        // default width
        fmt.Printf(_T("%%.%df"), m_precision);
    }
    else if ( m_precision != -1 && m_width != -1 )
    {
        fmt.Printf(_T("%%%d.%df"), m_width, m_precision);
    }
    else
    {
        // default width/precision
        fmt = _T("%f");
    }

    return wxString::Format(fmt, m_valueOld);
}

bool wxGridCellFloatEditor::IsAcceptedKey(wxKeyEvent& event)
{
    if ( wxGridCellEditor::IsAcceptedKey(event) )
    {
        const int keycode = event.GetKeyCode();
        if ( isascii(keycode) )
        {
            char tmpbuf[2];
            tmpbuf[0] = (char) keycode;
            tmpbuf[1] = '\0';
            wxString strbuf(tmpbuf, *wxConvCurrent);

#if wxUSE_INTL
            const wxString decimalPoint =
                wxLocale::GetInfo(wxLOCALE_DECIMAL_POINT, wxLOCALE_CAT_NUMBER);
#else
            const wxString decimalPoint(_T('.'));
#endif

            // accept digits, 'e' as in '1e+6', also '-', '+', and '.'
            if ( wxIsdigit(keycode) ||
                    tolower(keycode) == 'e' ||
                        keycode == decimalPoint ||
                            keycode == '+' ||
                                keycode == '-' )
            {
                return true;
            }
        }
    }

    return false;
}

#endif // wxUSE_TEXTCTRL

#if wxUSE_CHECKBOX

// ----------------------------------------------------------------------------
// wxGridCellBoolEditor
// ----------------------------------------------------------------------------

// the default values for GetValue()
wxString wxGridCellBoolEditor::ms_stringValues[2] = { _T(""), _T("1") };

void wxGridCellBoolEditor::Create(wxWindow* parent,
                                  wxWindowID id,
                                  wxEvtHandler* evtHandler)
{
    m_control = new wxCheckBox(parent, id, wxEmptyString,
                               wxDefaultPosition, wxDefaultSize,
                               wxNO_BORDER);

    wxGridCellEditor::Create(parent, id, evtHandler);
}

void wxGridCellBoolEditor::SetSize(const wxRect& r)
{
    bool resize = false;
    wxSize size = m_control->GetSize();
    wxCoord minSize = wxMin(r.width, r.height);

    // check if the checkbox is not too big/small for this cell
    wxSize sizeBest = m_control->GetBestSize();
    if ( !(size == sizeBest) )
    {
        // reset to default size if it had been made smaller
        size = sizeBest;

        resize = true;
    }

    if ( size.x >= minSize || size.y >= minSize )
    {
        // leave 1 pixel margin
        size.x = size.y = minSize - 2;

        resize = true;
    }

    if ( resize )
    {
        m_control->SetSize(size);
    }

    // position it in the centre of the rectangle (TODO: support alignment?)

#if defined(__WXGTK__) || defined (__WXMOTIF__)
    // the checkbox without label still has some space to the right in wxGTK,
    // so shift it to the right
    size.x -= 8;
#elif defined(__WXMSW__)
    // here too, but in other way
    size.x += 1;
    size.y -= 2;
#endif

    int hAlign = wxALIGN_CENTRE;
    int vAlign = wxALIGN_CENTRE;
    if (GetCellAttr())
        GetCellAttr()->GetAlignment(& hAlign, & vAlign);

    int x = 0, y = 0;
    if (hAlign == wxALIGN_LEFT)
    {
        x = r.x + 2;

#ifdef __WXMSW__
        x += 2;
#endif

        y = r.y + r.height / 2 - size.y / 2;
    }
    else if (hAlign == wxALIGN_RIGHT)
    {
        x = r.x + r.width - size.x - 2;
        y = r.y + r.height / 2 - size.y / 2;
    }
    else if (hAlign == wxALIGN_CENTRE)
    {
        x = r.x + r.width / 2 - size.x / 2;
        y = r.y + r.height / 2 - size.y / 2;
    }

    m_control->Move(x, y);
}

void wxGridCellBoolEditor::Show(bool show, wxGridCellAttr *attr)
{
    m_control->Show(show);

    if ( show )
    {
        wxColour colBg = attr ? attr->GetBackgroundColour() : *wxLIGHT_GREY;
        CBox()->SetBackgroundColour(colBg);
    }
}

void wxGridCellBoolEditor::BeginEdit(int row, int col, wxGrid* grid)
{
    wxASSERT_MSG(m_control,
                 wxT("The wxGridCellEditor must be created first!"));

    if (grid->GetTable()->CanGetValueAs(row, col, wxGRID_VALUE_BOOL))
    {
        m_startValue = grid->GetTable()->GetValueAsBool(row, col);
    }
    else
    {
        wxString cellval( grid->GetTable()->GetValue(row, col) );

        if ( cellval == ms_stringValues[false] )
            m_startValue = false;
        else if ( cellval == ms_stringValues[true] )
            m_startValue = true;
        else
        {
            // do not try to be smart here and convert it to true or false
            // because we'll still overwrite it with something different and
            // this risks to be very surprising for the user code, let them
            // know about it
            wxFAIL_MSG( _T("invalid value for a cell with bool editor!") );
        }
    }

    CBox()->SetValue(m_startValue);
    CBox()->SetFocus();
}

bool wxGridCellBoolEditor::EndEdit(int row, int col,
                                   wxGrid* grid)
{
    wxASSERT_MSG(m_control,
                 wxT("The wxGridCellEditor must be created first!"));

    bool changed = false;
    bool value = CBox()->GetValue();
    if ( value != m_startValue )
        changed = true;

    if ( changed )
    {
        wxGridTableBase * const table = grid->GetTable();
        if ( table->CanGetValueAs(row, col, wxGRID_VALUE_BOOL) )
            table->SetValueAsBool(row, col, value);
        else
            table->SetValue(row, col, GetValue());
    }

    return changed;
}

void wxGridCellBoolEditor::Reset()
{
    wxASSERT_MSG(m_control,
                 wxT("The wxGridCellEditor must be created first!"));

    CBox()->SetValue(m_startValue);
}

void wxGridCellBoolEditor::StartingClick()
{
    CBox()->SetValue(!CBox()->GetValue());
}

bool wxGridCellBoolEditor::IsAcceptedKey(wxKeyEvent& event)
{
    if ( wxGridCellEditor::IsAcceptedKey(event) )
    {
        int keycode = event.GetKeyCode();
        switch ( keycode )
        {
            case WXK_SPACE:
            case '+':
            case '-':
                return true;
        }
    }

    return false;
}

void wxGridCellBoolEditor::StartingKey(wxKeyEvent& event)
{
    int keycode = event.GetKeyCode();
    switch ( keycode )
    {
        case WXK_SPACE:
            CBox()->SetValue(!CBox()->GetValue());
            break;

        case '+':
            CBox()->SetValue(true);
            break;

        case '-':
            CBox()->SetValue(false);
            break;
    }
}

wxString wxGridCellBoolEditor::GetValue() const
{
  return ms_stringValues[CBox()->GetValue()];
}

/* static */ void
wxGridCellBoolEditor::UseStringValues(const wxString& valueTrue,
                                      const wxString& valueFalse)
{
    ms_stringValues[false] = valueFalse;
    ms_stringValues[true] = valueTrue;
}

/* static */ bool
wxGridCellBoolEditor::IsTrueValue(const wxString& value)
{
    return value == ms_stringValues[true];
}

#endif // wxUSE_CHECKBOX

#if wxUSE_COMBOBOX

// ----------------------------------------------------------------------------
// wxGridCellChoiceEditor
// ----------------------------------------------------------------------------

wxGridCellChoiceEditor::wxGridCellChoiceEditor(const wxArrayString& choices,
                                               bool allowOthers)
    : m_choices(choices),
      m_allowOthers(allowOthers) { }

wxGridCellChoiceEditor::wxGridCellChoiceEditor(size_t count,
                                               const wxString choices[],
                                               bool allowOthers)
                      : m_allowOthers(allowOthers)
{
    if ( count )
    {
        m_choices.Alloc(count);
        for ( size_t n = 0; n < count; n++ )
        {
            m_choices.Add(choices[n]);
        }
    }
}

wxGridCellEditor *wxGridCellChoiceEditor::Clone() const
{
    wxGridCellChoiceEditor *editor = new wxGridCellChoiceEditor;
    editor->m_allowOthers = m_allowOthers;
    editor->m_choices = m_choices;

    return editor;
}

void wxGridCellChoiceEditor::Create(wxWindow* parent,
                                    wxWindowID id,
                                    wxEvtHandler* evtHandler)
{
    int style = wxTE_PROCESS_ENTER |
                wxTE_PROCESS_TAB |
                wxBORDER_NONE;

    if ( !m_allowOthers )
        style |= wxCB_READONLY;

    m_control = new wxComboBox(parent, id, wxEmptyString,
                               wxDefaultPosition, wxDefaultSize,
                               m_choices,
                               style);

    wxGridCellEditor::Create(parent, id, evtHandler);
}

void wxGridCellChoiceEditor::PaintBackground(const wxRect& rectCell,
                                             wxGridCellAttr * attr)
{
    // as we fill the entire client area, don't do anything here to minimize
    // flicker

    // TODO: It doesn't actually fill the client area since the height of a
    // combo always defaults to the standard.  Until someone has time to
    // figure out the right rectangle to paint, just do it the normal way.
    wxGridCellEditor::PaintBackground(rectCell, attr);
}

void wxGridCellChoiceEditor::BeginEdit(int row, int col, wxGrid* grid)
{
    wxASSERT_MSG(m_control,
                 wxT("The wxGridCellEditor must be created first!"));

    wxGridCellEditorEvtHandler* evtHandler = NULL;
    if (m_control)
        evtHandler = wxDynamicCast(m_control->GetEventHandler(), wxGridCellEditorEvtHandler);

    // Don't immediately end if we get a kill focus event within BeginEdit
    if (evtHandler)
        evtHandler->SetInSetFocus(true);

    m_startValue = grid->GetTable()->GetValue(row, col);

    if (m_allowOthers)
    {
        Combo()->SetValue(m_startValue);
    }
    else
    {
        // find the right position, or default to the first if not found
        int pos = Combo()->FindString(m_startValue);
        if (pos == wxNOT_FOUND)
            pos = 0;
        Combo()->SetSelection(pos);
    }

    Combo()->SetInsertionPointEnd();
    Combo()->SetFocus();

    if (evtHandler)
    {
        // When dropping down the menu, a kill focus event
        // happens after this point, so we can't reset the flag yet.
#if !defined(__WXGTK20__)
        evtHandler->SetInSetFocus(false);
#endif
    }
}

bool wxGridCellChoiceEditor::EndEdit(int row, int col,
                                     wxGrid* grid)
{
    wxString value = Combo()->GetValue();
    if ( value == m_startValue )
        return false;

    grid->GetTable()->SetValue(row, col, value);

    return true;
}

void wxGridCellChoiceEditor::Reset()
{
    Combo()->SetValue(m_startValue);
    Combo()->SetInsertionPointEnd();
}

void wxGridCellChoiceEditor::SetParameters(const wxString& params)
{
    if ( !params )
    {
        // what can we do?
        return;
    }

    m_choices.Empty();

    wxStringTokenizer tk(params, _T(','));
    while ( tk.HasMoreTokens() )
    {
        m_choices.Add(tk.GetNextToken());
    }
}

// return the value in the text control
wxString wxGridCellChoiceEditor::GetValue() const
{
  return Combo()->GetValue();
}

#endif // wxUSE_COMBOBOX

// ----------------------------------------------------------------------------
// wxGridCellEditorEvtHandler
// ----------------------------------------------------------------------------

void wxGridCellEditorEvtHandler::OnKillFocus(wxFocusEvent& event)
{
    // Don't disable the cell if we're just starting to edit it
    if (m_inSetFocus)
        return;

    // accept changes
    m_grid->DisableCellEditControl();

    event.Skip();
}

void wxGridCellEditorEvtHandler::OnKeyDown(wxKeyEvent& event)
{
    switch ( event.GetKeyCode() )
    {
        case WXK_ESCAPE:
            m_editor->Reset();
            m_grid->DisableCellEditControl();
            break;

        case WXK_TAB:
            m_grid->GetEventHandler()->ProcessEvent( event );
            break;

        case WXK_RETURN:
        case WXK_NUMPAD_ENTER:
            if (!m_grid->GetEventHandler()->ProcessEvent(event))
                m_editor->HandleReturn(event);
            break;

        default:
            event.Skip();
            break;
    }
}

void wxGridCellEditorEvtHandler::OnChar(wxKeyEvent& event)
{
    int row = m_grid->GetGridCursorRow();
    int col = m_grid->GetGridCursorCol();
    wxRect rect = m_grid->CellToRect( row, col );
    int cw, ch;
    m_grid->GetGridWindow()->GetClientSize( &cw, &ch );

    // if cell width is smaller than grid client area, cell is wholly visible
    bool wholeCellVisible = (rect.GetWidth() < cw);

    switch ( event.GetKeyCode() )
    {
        case WXK_ESCAPE:
        case WXK_TAB:
        case WXK_RETURN:
        case WXK_NUMPAD_ENTER:
            break;

        case WXK_HOME:
        {
            if ( wholeCellVisible )
            {
                // no special processing needed...
                event.Skip();
                break;
            }

            // do special processing for partly visible cell...

            // get the widths of all cells previous to this one
            int colXPos = 0;
            for ( int i = 0; i < col; i++ )
            {
                colXPos += m_grid->GetColSize(i);
            }

            int xUnit = 1, yUnit = 1;
            m_grid->GetScrollPixelsPerUnit(&xUnit, &yUnit);
            if (col != 0)
            {
                m_grid->Scroll(colXPos / xUnit - 1, m_grid->GetScrollPos(wxVERTICAL));
            }
            else
            {
                m_grid->Scroll(colXPos / xUnit, m_grid->GetScrollPos(wxVERTICAL));
            }
            event.Skip();
            break;
        }

        case WXK_END:
        {
            if ( wholeCellVisible )
            {
                // no special processing needed...
                event.Skip();
                break;
            }

            // do special processing for partly visible cell...

            int textWidth = 0;
            wxString value = m_grid->GetCellValue(row, col);
            if ( wxEmptyString != value )
            {
                // get width of cell CONTENTS (text)
                int y;
                wxFont font = m_grid->GetCellFont(row, col);
                m_grid->GetTextExtent(value, &textWidth, &y, NULL, NULL, &font);

                // try to RIGHT align the text by scrolling
                int client_right = m_grid->GetGridWindow()->GetClientSize().GetWidth();

                // (m_grid->GetScrollLineX()*2) is a factor for not scrolling to far,
                // otherwise the last part of the cell content might be hidden below the scroll bar
                // FIXME: maybe there is a more suitable correction?
                textWidth -= (client_right - (m_grid->GetScrollLineX() * 2));
                if ( textWidth < 0 )
                {
                    textWidth = 0;
                }
            }

            // get the widths of all cells previous to this one
            int colXPos = 0;
            for ( int i = 0; i < col; i++ )
            {
                colXPos += m_grid->GetColSize(i);
            }

            // and add the (modified) text width of the cell contents
            // as we'd like to see the last part of the cell contents
            colXPos += textWidth;

            int xUnit = 1, yUnit = 1;
            m_grid->GetScrollPixelsPerUnit(&xUnit, &yUnit);
            m_grid->Scroll(colXPos / xUnit - 1, m_grid->GetScrollPos(wxVERTICAL));
            event.Skip();
            break;
        }

        default:
            event.Skip();
            break;
    }
}

// ----------------------------------------------------------------------------
// wxGridCellWorker is an (almost) empty common base class for
// wxGridCellRenderer and wxGridCellEditor managing ref counting
// ----------------------------------------------------------------------------

void wxGridCellWorker::SetParameters(const wxString& WXUNUSED(params))
{
    // nothing to do
}

wxGridCellWorker::~wxGridCellWorker()
{
}

// ============================================================================
// renderer classes
// ============================================================================

// ----------------------------------------------------------------------------
// wxGridCellRenderer
// ----------------------------------------------------------------------------

void wxGridCellRenderer::Draw(wxGrid& grid,
                              wxGridCellAttr& attr,
                              wxDC& dc,
                              const wxRect& rect,
                              int WXUNUSED(row), int WXUNUSED(col),
                              bool isSelected)
{
    dc.SetBackgroundMode( wxSOLID );

    // grey out fields if the grid is disabled
    if ( grid.IsEnabled() )
    {
        if ( isSelected )
        {
            wxColour clr;
            if ( wxWindow::FindFocus() == grid.GetGridWindow() )
                clr = grid.GetSelectionBackground();
            else
                clr = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
            dc.SetBrush( wxBrush(clr, wxSOLID) );
        }
        else
        {
            dc.SetBrush( wxBrush(attr.GetBackgroundColour(), wxSOLID) );
        }
    }
    else
    {
        dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE), wxSOLID));
    }

    dc.SetPen( *wxTRANSPARENT_PEN );
    dc.DrawRectangle(rect);
}

// ----------------------------------------------------------------------------
// wxGridCellStringRenderer
// ----------------------------------------------------------------------------

void wxGridCellStringRenderer::SetTextColoursAndFont(const wxGrid& grid,
                                                     const wxGridCellAttr& attr,
                                                     wxDC& dc,
                                                     bool isSelected)
{
    dc.SetBackgroundMode( wxTRANSPARENT );

    // TODO some special colours for attr.IsReadOnly() case?

    // different coloured text when the grid is disabled
    if ( grid.IsEnabled() )
    {
        if ( isSelected )
        {
            wxColour clr;
            if ( wxWindow::FindFocus() ==
                 wx_const_cast(wxGrid&, grid).GetGridWindow() )
                clr = grid.GetSelectionBackground();
            else
                clr = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
            dc.SetTextBackground( clr );
            dc.SetTextForeground( grid.GetSelectionForeground() );
        }
        else
        {
            dc.SetTextBackground( attr.GetBackgroundColour() );
            dc.SetTextForeground( attr.GetTextColour() );
        }
    }
    else
    {
        dc.SetTextBackground(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
        dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    }

    dc.SetFont( attr.GetFont() );
}

wxSize wxGridCellStringRenderer::DoGetBestSize(const wxGridCellAttr& attr,
                                               wxDC& dc,
                                               const wxString& text)
{
    wxCoord x = 0, y = 0, max_x = 0;
    dc.SetFont(attr.GetFont());
    wxStringTokenizer tk(text, _T('\n'));
    while ( tk.HasMoreTokens() )
    {
        dc.GetTextExtent(tk.GetNextToken(), &x, &y);
        max_x = wxMax(max_x, x);
    }

    y *= 1 + text.Freq(wxT('\n')); // multiply by the number of lines.

    return wxSize(max_x, y);
}

wxSize wxGridCellStringRenderer::GetBestSize(wxGrid& grid,
                                             wxGridCellAttr& attr,
                                             wxDC& dc,
                                             int row, int col)
{
    return DoGetBestSize(attr, dc, grid.GetCellValue(row, col));
}

void wxGridCellStringRenderer::Draw(wxGrid& grid,
                                    wxGridCellAttr& attr,
                                    wxDC& dc,
                                    const wxRect& rectCell,
                                    int row, int col,
                                    bool isSelected)
{
    wxRect rect = rectCell;
    rect.Inflate(-1);

    // erase only this cells background, overflow cells should have been erased
    wxGridCellRenderer::Draw(grid, attr, dc, rectCell, row, col, isSelected);

    int hAlign, vAlign;
    attr.GetAlignment(&hAlign, &vAlign);

    int overflowCols = 0;

    if (attr.GetOverflow())
    {
        int cols = grid.GetNumberCols();
        int best_width = GetBestSize(grid,attr,dc,row,col).GetWidth();
        int cell_rows, cell_cols;
        attr.GetSize( &cell_rows, &cell_cols ); // shouldn't get here if <= 0
        if ((best_width > rectCell.width) && (col < cols) && grid.GetTable())
        {
            int i, c_cols, c_rows;
            for (i = col+cell_cols; i < cols; i++)
            {
                bool is_empty = true;
                for (int j=row; j < row + cell_rows; j++)
                {
                    // check w/ anchor cell for multicell block
                    grid.GetCellSize(j, i, &c_rows, &c_cols);
                    if (c_rows > 0)
                        c_rows = 0;
                    if (!grid.GetTable()->IsEmptyCell(j + c_rows, i))
                    {
                        is_empty = false;
                        break;
                    }
                }

                if (is_empty)
                {
                    rect.width += grid.GetColSize(i);
                }
                else
                {
                    i--;
                    break;
                }

                if (rect.width >= best_width)
                    break;
            }

            overflowCols = i - col - cell_cols + 1;
            if (overflowCols >= cols)
                overflowCols = cols - 1;
        }

        if (overflowCols > 0) // redraw overflow cells w/ proper hilight
        {
            hAlign = wxALIGN_LEFT; // if oveflowed then it's left aligned
            wxRect clip = rect;
            clip.x += rectCell.width;
            // draw each overflow cell individually
            int col_end = col + cell_cols + overflowCols;
            if (col_end >= grid.GetNumberCols())
                col_end = grid.GetNumberCols() - 1;
            for (int i = col + cell_cols; i <= col_end; i++)
            {
                clip.width = grid.GetColSize(i) - 1;
                dc.DestroyClippingRegion();
                dc.SetClippingRegion(clip);

                SetTextColoursAndFont(grid, attr, dc,
                        grid.IsInSelection(row,i));

                grid.DrawTextRectangle(dc, grid.GetCellValue(row, col),
                        rect, hAlign, vAlign);
                clip.x += grid.GetColSize(i) - 1;
            }

            rect = rectCell;
            rect.Inflate(-1);
            rect.width++;
            dc.DestroyClippingRegion();
        }
    }

    // now we only have to draw the text
    SetTextColoursAndFont(grid, attr, dc, isSelected);

    grid.DrawTextRectangle(dc, grid.GetCellValue(row, col),
                           rect, hAlign, vAlign);
}

// ----------------------------------------------------------------------------
// wxGridCellNumberRenderer
// ----------------------------------------------------------------------------

wxString wxGridCellNumberRenderer::GetString(const wxGrid& grid, int row, int col)
{
    wxGridTableBase *table = grid.GetTable();
    wxString text;
    if ( table->CanGetValueAs(row, col, wxGRID_VALUE_NUMBER) )
    {
        text.Printf(_T("%ld"), table->GetValueAsLong(row, col));
    }
    else
    {
        text = table->GetValue(row, col);
    }

    return text;
}

void wxGridCellNumberRenderer::Draw(wxGrid& grid,
                                    wxGridCellAttr& attr,
                                    wxDC& dc,
                                    const wxRect& rectCell,
                                    int row, int col,
                                    bool isSelected)
{
    wxGridCellRenderer::Draw(grid, attr, dc, rectCell, row, col, isSelected);

    SetTextColoursAndFont(grid, attr, dc, isSelected);

    // draw the text right aligned by default
    int hAlign, vAlign;
    attr.GetAlignment(&hAlign, &vAlign);
    hAlign = wxALIGN_RIGHT;

    wxRect rect = rectCell;
    rect.Inflate(-1);

    grid.DrawTextRectangle(dc, GetString(grid, row, col), rect, hAlign, vAlign);
}

wxSize wxGridCellNumberRenderer::GetBestSize(wxGrid& grid,
                                             wxGridCellAttr& attr,
                                             wxDC& dc,
                                             int row, int col)
{
    return DoGetBestSize(attr, dc, GetString(grid, row, col));
}

// ----------------------------------------------------------------------------
// wxGridCellFloatRenderer
// ----------------------------------------------------------------------------

wxGridCellFloatRenderer::wxGridCellFloatRenderer(int width, int precision)
{
    SetWidth(width);
    SetPrecision(precision);
}

wxGridCellRenderer *wxGridCellFloatRenderer::Clone() const
{
    wxGridCellFloatRenderer *renderer = new wxGridCellFloatRenderer;
    renderer->m_width = m_width;
    renderer->m_precision = m_precision;
    renderer->m_format = m_format;

    return renderer;
}

wxString wxGridCellFloatRenderer::GetString(const wxGrid& grid, int row, int col)
{
    wxGridTableBase *table = grid.GetTable();

    bool hasDouble;
    double val;
    wxString text;
    if ( table->CanGetValueAs(row, col, wxGRID_VALUE_FLOAT) )
    {
        val = table->GetValueAsDouble(row, col);
        hasDouble = true;
    }
    else
    {
        text = table->GetValue(row, col);
        hasDouble = text.ToDouble(&val);
    }

    if ( hasDouble )
    {
        if ( !m_format )
        {
            if ( m_width == -1 )
            {
                if ( m_precision == -1 )
                {
                    // default width/precision
                    m_format = _T("%f");
                }
                else
                {
                    m_format.Printf(_T("%%.%df"), m_precision);
                }
            }
            else if ( m_precision == -1 )
            {
                // default precision
                m_format.Printf(_T("%%%d.f"), m_width);
            }
            else
            {
                m_format.Printf(_T("%%%d.%df"), m_width, m_precision);
            }
        }

        text.Printf(m_format, val);

    }
    //else: text already contains the string

    return text;
}

void wxGridCellFloatRenderer::Draw(wxGrid& grid,
                                   wxGridCellAttr& attr,
                                   wxDC& dc,
                                   const wxRect& rectCell,
                                   int row, int col,
                                   bool isSelected)
{
    wxGridCellRenderer::Draw(grid, attr, dc, rectCell, row, col, isSelected);

    SetTextColoursAndFont(grid, attr, dc, isSelected);

    // draw the text right aligned by default
    int hAlign, vAlign;
    attr.GetAlignment(&hAlign, &vAlign);
    hAlign = wxALIGN_RIGHT;

    wxRect rect = rectCell;
    rect.Inflate(-1);

    grid.DrawTextRectangle(dc, GetString(grid, row, col), rect, hAlign, vAlign);
}

wxSize wxGridCellFloatRenderer::GetBestSize(wxGrid& grid,
                                            wxGridCellAttr& attr,
                                            wxDC& dc,
                                            int row, int col)
{
    return DoGetBestSize(attr, dc, GetString(grid, row, col));
}

void wxGridCellFloatRenderer::SetParameters(const wxString& params)
{
    if ( !params )
    {
        // reset to defaults
        SetWidth(-1);
        SetPrecision(-1);
    }
    else
    {
        wxString tmp = params.BeforeFirst(_T(','));
        if ( !tmp.empty() )
        {
            long width;
            if ( tmp.ToLong(&width) )
            {
                SetWidth((int)width);
            }
            else
            {
                wxLogDebug(_T("Invalid wxGridCellFloatRenderer width parameter string '%s ignored"), params.c_str());
            }
        }

        tmp = params.AfterFirst(_T(','));
        if ( !tmp.empty() )
        {
            long precision;
            if ( tmp.ToLong(&precision) )
            {
                SetPrecision((int)precision);
            }
            else
            {
                wxLogDebug(_T("Invalid wxGridCellFloatRenderer precision parameter string '%s ignored"), params.c_str());
            }
        }
    }
}

// ----------------------------------------------------------------------------
// wxGridCellBoolRenderer
// ----------------------------------------------------------------------------

wxSize wxGridCellBoolRenderer::ms_sizeCheckMark;

// FIXME these checkbox size calculations are really ugly...

// between checkmark and box
static const wxCoord wxGRID_CHECKMARK_MARGIN = 2;

wxSize wxGridCellBoolRenderer::GetBestSize(wxGrid& grid,
                                           wxGridCellAttr& WXUNUSED(attr),
                                           wxDC& WXUNUSED(dc),
                                           int WXUNUSED(row),
                                           int WXUNUSED(col))
{
    // compute it only once (no locks for MT safeness in GUI thread...)
    if ( !ms_sizeCheckMark.x )
    {
        // get checkbox size
        wxCheckBox *checkbox = new wxCheckBox(&grid, wxID_ANY, wxEmptyString);
        wxSize size = checkbox->GetBestSize();
        wxCoord checkSize = size.y + 2 * wxGRID_CHECKMARK_MARGIN;

        // FIXME wxGTK::wxCheckBox::GetBestSize() gives "wrong" result
#if defined(__WXGTK__) || defined(__WXMOTIF__)
        checkSize -= size.y / 2;
#endif

        delete checkbox;

        ms_sizeCheckMark.x = ms_sizeCheckMark.y = checkSize;
    }

    return ms_sizeCheckMark;
}

void wxGridCellBoolRenderer::Draw(wxGrid& grid,
                                  wxGridCellAttr& attr,
                                  wxDC& dc,
                                  const wxRect& rect,
                                  int row, int col,
                                  bool isSelected)
{
    wxGridCellRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);

    // draw a check mark in the centre (ignoring alignment - TODO)
    wxSize size = GetBestSize(grid, attr, dc, row, col);

    // don't draw outside the cell
    wxCoord minSize = wxMin(rect.width, rect.height);
    if ( size.x >= minSize || size.y >= minSize )
    {
        // and even leave (at least) 1 pixel margin
        size.x = size.y = minSize - 2;
    }

    // draw a border around checkmark
    int vAlign, hAlign;
    attr.GetAlignment(&hAlign, &vAlign);

    wxRect rectBorder;
    if (hAlign == wxALIGN_CENTRE)
    {
        rectBorder.x = rect.x + rect.width / 2 - size.x / 2;
        rectBorder.y = rect.y + rect.height / 2 - size.y / 2;
        rectBorder.width = size.x;
        rectBorder.height = size.y;
    }
    else if (hAlign == wxALIGN_LEFT)
    {
        rectBorder.x = rect.x + 2;
        rectBorder.y = rect.y + rect.height / 2 - size.y / 2;
        rectBorder.width = size.x;
        rectBorder.height = size.y;
    }
    else if (hAlign == wxALIGN_RIGHT)
    {
        rectBorder.x = rect.x + rect.width - size.x - 2;
        rectBorder.y = rect.y + rect.height / 2 - size.y / 2;
        rectBorder.width = size.x;
        rectBorder.height = size.y;
    }

    bool value;
    if ( grid.GetTable()->CanGetValueAs(row, col, wxGRID_VALUE_BOOL) )
    {
        value = grid.GetTable()->GetValueAsBool(row, col);
    }
    else
    {
        wxString cellval( grid.GetTable()->GetValue(row, col) );
        value = wxGridCellBoolEditor::IsTrueValue(cellval);
    }

    if ( value )
    {
        wxRect rectMark = rectBorder;

#ifdef __WXMSW__
        // MSW DrawCheckMark() is weird (and should probably be changed...)
        rectMark.Inflate(-wxGRID_CHECKMARK_MARGIN / 2);
        rectMark.x++;
        rectMark.y++;
#else
        rectMark.Inflate(-wxGRID_CHECKMARK_MARGIN);
#endif

        dc.SetTextForeground(attr.GetTextColour());
        dc.DrawCheckMark(rectMark);
    }

    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(wxPen(attr.GetTextColour(), 1, wxSOLID));
    dc.DrawRectangle(rectBorder);
}

// ----------------------------------------------------------------------------
// wxGridCellAttr
// ----------------------------------------------------------------------------

void wxGridCellAttr::Init(wxGridCellAttr *attrDefault)
{
    m_nRef = 1;

    m_isReadOnly = Unset;

    m_renderer = NULL;
    m_editor = NULL;

    m_attrkind = wxGridCellAttr::Cell;

    m_sizeRows = m_sizeCols = 1;
    m_overflow = UnsetOverflow;

    SetDefAttr(attrDefault);
}

wxGridCellAttr *wxGridCellAttr::Clone() const
{
    wxGridCellAttr *attr = new wxGridCellAttr(m_defGridAttr);

    if ( HasTextColour() )
        attr->SetTextColour(GetTextColour());
    if ( HasBackgroundColour() )
        attr->SetBackgroundColour(GetBackgroundColour());
    if ( HasFont() )
        attr->SetFont(GetFont());
    if ( HasAlignment() )
        attr->SetAlignment(m_hAlign, m_vAlign);

    attr->SetSize( m_sizeRows, m_sizeCols );

    if ( m_renderer )
    {
        attr->SetRenderer(m_renderer);
        m_renderer->IncRef();
    }
    if ( m_editor )
    {
        attr->SetEditor(m_editor);
        m_editor->IncRef();
    }

    if ( IsReadOnly() )
        attr->SetReadOnly();

    attr->SetOverflow( m_overflow == Overflow );
    attr->SetKind( m_attrkind );

    return attr;
}

void wxGridCellAttr::MergeWith(wxGridCellAttr *mergefrom)
{
    if ( !HasTextColour() && mergefrom->HasTextColour() )
        SetTextColour(mergefrom->GetTextColour());
    if ( !HasBackgroundColour() && mergefrom->HasBackgroundColour() )
        SetBackgroundColour(mergefrom->GetBackgroundColour());
    if ( !HasFont() && mergefrom->HasFont() )
        SetFont(mergefrom->GetFont());
    if ( !HasAlignment() && mergefrom->HasAlignment() )
    {
        int hAlign, vAlign;
        mergefrom->GetAlignment( &hAlign, &vAlign);
        SetAlignment(hAlign, vAlign);
    }
    if ( !HasSize() && mergefrom->HasSize() )
        mergefrom->GetSize( &m_sizeRows, &m_sizeCols );

    // Directly access member functions as GetRender/Editor don't just return
    // m_renderer/m_editor
    //
    // Maybe add support for merge of Render and Editor?
    if (!HasRenderer() && mergefrom->HasRenderer() )
    {
        m_renderer = mergefrom->m_renderer;
        m_renderer->IncRef();
    }
    if ( !HasEditor() && mergefrom->HasEditor() )
    {
        m_editor =  mergefrom->m_editor;
        m_editor->IncRef();
    }
    if ( !HasReadWriteMode() && mergefrom->HasReadWriteMode() )
        SetReadOnly(mergefrom->IsReadOnly());

    if (!HasOverflowMode() && mergefrom->HasOverflowMode() )
        SetOverflow(mergefrom->GetOverflow());

    SetDefAttr(mergefrom->m_defGridAttr);
}

void wxGridCellAttr::SetSize(int num_rows, int num_cols)
{
    // The size of a cell is normally 1,1

    // If this cell is larger (2,2) then this is the top left cell
    // the other cells that will be covered (lower right cells) must be
    // set to negative or zero values such that
    // row + num_rows of the covered cell points to the larger cell (this cell)
    // same goes for the col + num_cols.

    // Size of 0,0 is NOT valid, neither is <=0 and any positive value

    wxASSERT_MSG( (!((num_rows > 0) && (num_cols <= 0)) ||
                  !((num_rows <= 0) && (num_cols > 0)) ||
                  !((num_rows == 0) && (num_cols == 0))),
                  wxT("wxGridCellAttr::SetSize only takes two postive values or negative/zero values"));

    m_sizeRows = num_rows;
    m_sizeCols = num_cols;
}

const wxColour& wxGridCellAttr::GetTextColour() const
{
    if (HasTextColour())
    {
        return m_colText;
    }
    else if (m_defGridAttr && m_defGridAttr != this)
    {
        return m_defGridAttr->GetTextColour();
    }
    else
    {
        wxFAIL_MSG(wxT("Missing default cell attribute"));
        return wxNullColour;
    }
}

const wxColour& wxGridCellAttr::GetBackgroundColour() const
{
    if (HasBackgroundColour())
    {
        return m_colBack;
    }
    else if (m_defGridAttr && m_defGridAttr != this)
    {
        return m_defGridAttr->GetBackgroundColour();
    }
    else
    {
        wxFAIL_MSG(wxT("Missing default cell attribute"));
        return wxNullColour;
    }
}

const wxFont& wxGridCellAttr::GetFont() const
{
    if (HasFont())
    {
        return m_font;
    }
    else if (m_defGridAttr && m_defGridAttr != this)
    {
        return m_defGridAttr->GetFont();
    }
    else
    {
        wxFAIL_MSG(wxT("Missing default cell attribute"));
        return wxNullFont;
    }
}

void wxGridCellAttr::GetAlignment(int *hAlign, int *vAlign) const
{
    if (HasAlignment())
    {
        if ( hAlign )
            *hAlign = m_hAlign;
        if ( vAlign )
            *vAlign = m_vAlign;
    }
    else if (m_defGridAttr && m_defGridAttr != this)
    {
        m_defGridAttr->GetAlignment(hAlign, vAlign);
    }
    else
    {
        wxFAIL_MSG(wxT("Missing default cell attribute"));
    }
}

void wxGridCellAttr::GetSize( int *num_rows, int *num_cols ) const
{
    if ( num_rows )
        *num_rows = m_sizeRows;
    if ( num_cols )
        *num_cols = m_sizeCols;
}

// GetRenderer and GetEditor use a slightly different decision path about
// which attribute to use.  If a non-default attr object has one then it is
// used, otherwise the default editor or renderer is fetched from the grid and
// used.  It should be the default for the data type of the cell.  If it is
// NULL (because the table has a type that the grid does not have in its
// registry), then the grid's default editor or renderer is used.

wxGridCellRenderer* wxGridCellAttr::GetRenderer(wxGrid* grid, int row, int col) const
{
    wxGridCellRenderer *renderer = NULL;

    if ( m_renderer && this != m_defGridAttr )
    {
        // use the cells renderer if it has one
        renderer = m_renderer;
        renderer->IncRef();
    }
    else // no non-default cell renderer
    {
        // get default renderer for the data type
        if ( grid )
        {
            // GetDefaultRendererForCell() will do IncRef() for us
            renderer = grid->GetDefaultRendererForCell(row, col);
        }

        if ( renderer == NULL )
        {
            if ( (m_defGridAttr != NULL) && (m_defGridAttr != this) )
            {
                // if we still don't have one then use the grid default
                // (no need for IncRef() here neither)
                renderer = m_defGridAttr->GetRenderer(NULL, 0, 0);
            }
            else // default grid attr
            {
                // use m_renderer which we had decided not to use initially
                renderer = m_renderer;
                if ( renderer )
                    renderer->IncRef();
            }
        }
    }

    // we're supposed to always find something
    wxASSERT_MSG(renderer, wxT("Missing default cell renderer"));

    return renderer;
}

// same as above, except for s/renderer/editor/g
wxGridCellEditor* wxGridCellAttr::GetEditor(wxGrid* grid, int row, int col) const
{
    wxGridCellEditor *editor = NULL;

    if ( m_editor && this != m_defGridAttr )
    {
        // use the cells editor if it has one
        editor = m_editor;
        editor->IncRef();
    }
    else // no non default cell editor
    {
        // get default editor for the data type
        if ( grid )
        {
            // GetDefaultEditorForCell() will do IncRef() for us
            editor = grid->GetDefaultEditorForCell(row, col);
        }

        if ( editor == NULL )
        {
            if ( (m_defGridAttr != NULL) && (m_defGridAttr != this) )
            {
                // if we still don't have one then use the grid default
                // (no need for IncRef() here neither)
                editor = m_defGridAttr->GetEditor(NULL, 0, 0);
            }
            else // default grid attr
            {
                // use m_editor which we had decided not to use initially
                editor = m_editor;
                if ( editor )
                    editor->IncRef();
            }
        }
    }

    // we're supposed to always find something
    wxASSERT_MSG(editor, wxT("Missing default cell editor"));

    return editor;
}

// ----------------------------------------------------------------------------
// wxGridCellAttrData
// ----------------------------------------------------------------------------

void wxGridCellAttrData::SetAttr(wxGridCellAttr *attr, int row, int col)
{
    // Note: contrary to wxGridRowOrColAttrData::SetAttr, we must not
    //       touch attribute's reference counting explicitly, since this
    //       is managed by class wxGridCellWithAttr
    int n = FindIndex(row, col);
    if ( n == wxNOT_FOUND )
    {
        if ( attr )
        {
            // add the attribute
            m_attrs.Add(new wxGridCellWithAttr(row, col, attr));
        }
        //else: nothing to do
    }
    else // we already have an attribute for this cell
    {
        if ( attr )
        {
            // change the attribute
            m_attrs[(size_t)n].ChangeAttr(attr);
        }
        else
        {
            // remove this attribute
            m_attrs.RemoveAt((size_t)n);
        }
    }
}

wxGridCellAttr *wxGridCellAttrData::GetAttr(int row, int col) const
{
    wxGridCellAttr *attr = (wxGridCellAttr *)NULL;

    int n = FindIndex(row, col);
    if ( n != wxNOT_FOUND )
    {
        attr = m_attrs[(size_t)n].attr;
        attr->IncRef();
    }

    return attr;
}

void wxGridCellAttrData::UpdateAttrRows( size_t pos, int numRows )
{
    size_t count = m_attrs.GetCount();
    for ( size_t n = 0; n < count; n++ )
    {
        wxGridCellCoords& coords = m_attrs[n].coords;
        wxCoord row = coords.GetRow();
        if ((size_t)row >= pos)
        {
            if (numRows > 0)
            {
                // If rows inserted, include row counter where necessary
                coords.SetRow(row + numRows);
            }
            else if (numRows < 0)
            {
                // If rows deleted ...
                if ((size_t)row >= pos - numRows)
                {
                    // ...either decrement row counter (if row still exists)...
                    coords.SetRow(row + numRows);
                }
                else
                {
                    // ...or remove the attribute
                    m_attrs.RemoveAt(n);
                    n--;
                    count--;
                }
            }
        }
    }
}

void wxGridCellAttrData::UpdateAttrCols( size_t pos, int numCols )
{
    size_t count = m_attrs.GetCount();
    for ( size_t n = 0; n < count; n++ )
    {
        wxGridCellCoords& coords = m_attrs[n].coords;
        wxCoord col = coords.GetCol();
        if ( (size_t)col >= pos )
        {
            if ( numCols > 0 )
            {
                // If rows inserted, include row counter where necessary
                coords.SetCol(col + numCols);
            }
            else if (numCols < 0)
            {
                // If rows deleted ...
                if ((size_t)col >= pos - numCols)
                {
                    // ...either decrement row counter (if row still exists)...
                    coords.SetCol(col + numCols);
                }
                else
                {
                    // ...or remove the attribute
                    m_attrs.RemoveAt(n);
                    n--;
                    count--;
                }
            }
        }
    }
}

int wxGridCellAttrData::FindIndex(int row, int col) const
{
    size_t count = m_attrs.GetCount();
    for ( size_t n = 0; n < count; n++ )
    {
        const wxGridCellCoords& coords = m_attrs[n].coords;
        if ( (coords.GetRow() == row) && (coords.GetCol() == col) )
        {
            return n;
        }
    }

    return wxNOT_FOUND;
}

// ----------------------------------------------------------------------------
// wxGridRowOrColAttrData
// ----------------------------------------------------------------------------

wxGridRowOrColAttrData::~wxGridRowOrColAttrData()
{
    size_t count = m_attrs.Count();
    for ( size_t n = 0; n < count; n++ )
    {
        m_attrs[n]->DecRef();
    }
}

wxGridCellAttr *wxGridRowOrColAttrData::GetAttr(int rowOrCol) const
{
    wxGridCellAttr *attr = (wxGridCellAttr *)NULL;

    int n = m_rowsOrCols.Index(rowOrCol);
    if ( n != wxNOT_FOUND )
    {
        attr = m_attrs[(size_t)n];
        attr->IncRef();
    }

    return attr;
}

void wxGridRowOrColAttrData::SetAttr(wxGridCellAttr *attr, int rowOrCol)
{
    int i = m_rowsOrCols.Index(rowOrCol);
    if ( i == wxNOT_FOUND )
    {
        if ( attr )
        {
            // add the attribute - no need to do anything to reference count
            //                     since we take ownership of the attribute.
            m_rowsOrCols.Add(rowOrCol);
            m_attrs.Add(attr);
        }
        // nothing to remove
    }
    else
    {
        size_t n = (size_t)i;
        if ( m_attrs[n] == attr )
            // nothing to do
            return;
        if ( attr )
        {
            // change the attribute, handling reference count manually,
            //                       taking ownership of the new attribute.
            m_attrs[n]->DecRef();
            m_attrs[n] = attr;
        }
        else
        {
            // remove this attribute, handling reference count manually
            m_attrs[n]->DecRef();
            m_rowsOrCols.RemoveAt(n);
            m_attrs.RemoveAt(n);
        }
    }
}

void wxGridRowOrColAttrData::UpdateAttrRowsOrCols( size_t pos, int numRowsOrCols )
{
    size_t count = m_attrs.GetCount();
    for ( size_t n = 0; n < count; n++ )
    {
        int & rowOrCol = m_rowsOrCols[n];
        if ( (size_t)rowOrCol >= pos )
        {
            if ( numRowsOrCols > 0 )
            {
                // If rows inserted, include row counter where necessary
                rowOrCol += numRowsOrCols;
            }
            else if ( numRowsOrCols < 0)
            {
                // If rows deleted, either decrement row counter (if row still exists)
                if ((size_t)rowOrCol >= pos - numRowsOrCols)
                    rowOrCol += numRowsOrCols;
                else
                {
                    m_rowsOrCols.RemoveAt(n);
                    m_attrs[n]->DecRef();
                    m_attrs.RemoveAt(n);
                    n--;
                    count--;
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// wxGridCellAttrProvider
// ----------------------------------------------------------------------------

wxGridCellAttrProvider::wxGridCellAttrProvider()
{
    m_data = (wxGridCellAttrProviderData *)NULL;
}

wxGridCellAttrProvider::~wxGridCellAttrProvider()
{
    delete m_data;
}

void wxGridCellAttrProvider::InitData()
{
    m_data = new wxGridCellAttrProviderData;
}

wxGridCellAttr *wxGridCellAttrProvider::GetAttr(int row, int col,
                                                wxGridCellAttr::wxAttrKind  kind ) const
{
    wxGridCellAttr *attr = (wxGridCellAttr *)NULL;
    if ( m_data )
    {
        switch (kind)
        {
            case (wxGridCellAttr::Any):
                // Get cached merge attributes.
                // Currently not used as no cache implemented as not mutable
                // attr = m_data->m_mergeAttr.GetAttr(row, col);
                if (!attr)
                {
                    // Basically implement old version.
                    // Also check merge cache, so we don't have to re-merge every time..
                    wxGridCellAttr *attrcell = m_data->m_cellAttrs.GetAttr(row, col);
                    wxGridCellAttr *attrrow = m_data->m_rowAttrs.GetAttr(row);
                    wxGridCellAttr *attrcol = m_data->m_colAttrs.GetAttr(col);

                    if ((attrcell != attrrow) && (attrrow != attrcol) && (attrcell != attrcol))
                    {
                        // Two or more are non NULL
                        attr = new wxGridCellAttr;
                        attr->SetKind(wxGridCellAttr::Merged);

                        // Order is important..
                        if (attrcell)
                        {
                            attr->MergeWith(attrcell);
                            attrcell->DecRef();
                        }
                        if (attrcol)
                        {
                            attr->MergeWith(attrcol);
                            attrcol->DecRef();
                        }
                        if (attrrow)
                        {
                            attr->MergeWith(attrrow);
                            attrrow->DecRef();
                        }

                        // store merge attr if cache implemented
                        //attr->IncRef();
                        //m_data->m_mergeAttr.SetAttr(attr, row, col);
                    }
                    else
                    {
                        // one or none is non null return it or null.
                        if (attrrow)
                            attr = attrrow;
                        if (attrcol)
                        {
                            if (attr)
                                attr->DecRef();
                            attr = attrcol;
                        }
                        if (attrcell)
                        {
                            if (attr)
                                attr->DecRef();
                            attr = attrcell;
                        }
                    }
                }
                break;

            case (wxGridCellAttr::Cell):
                attr = m_data->m_cellAttrs.GetAttr(row, col);
                break;

            case (wxGridCellAttr::Col):
                attr = m_data->m_colAttrs.GetAttr(col);
                break;

            case (wxGridCellAttr::Row):
                attr = m_data->m_rowAttrs.GetAttr(row);
                break;

            default:
                // unused as yet...
                // (wxGridCellAttr::Default):
                // (wxGridCellAttr::Merged):
                break;
        }
    }

    return attr;
}

void wxGridCellAttrProvider::SetAttr(wxGridCellAttr *attr,
                                     int row, int col)
{
    if ( !m_data )
        InitData();

    m_data->m_cellAttrs.SetAttr(attr, row, col);
}

void wxGridCellAttrProvider::SetRowAttr(wxGridCellAttr *attr, int row)
{
    if ( !m_data )
        InitData();

    m_data->m_rowAttrs.SetAttr(attr, row);
}

void wxGridCellAttrProvider::SetColAttr(wxGridCellAttr *attr, int col)
{
    if ( !m_data )
        InitData();

    m_data->m_colAttrs.SetAttr(attr, col);
}

void wxGridCellAttrProvider::UpdateAttrRows( size_t pos, int numRows )
{
    if ( m_data )
    {
        m_data->m_cellAttrs.UpdateAttrRows( pos, numRows );

        m_data->m_rowAttrs.UpdateAttrRowsOrCols( pos, numRows );
    }
}

void wxGridCellAttrProvider::UpdateAttrCols( size_t pos, int numCols )
{
    if ( m_data )
    {
        m_data->m_cellAttrs.UpdateAttrCols( pos, numCols );

        m_data->m_colAttrs.UpdateAttrRowsOrCols( pos, numCols );
    }
}

// ----------------------------------------------------------------------------
// wxGridTypeRegistry
// ----------------------------------------------------------------------------

wxGridTypeRegistry::~wxGridTypeRegistry()
{
    size_t count = m_typeinfo.Count();
    for ( size_t i = 0; i < count; i++ )
        delete m_typeinfo[i];
}

void wxGridTypeRegistry::RegisterDataType(const wxString& typeName,
                                          wxGridCellRenderer* renderer,
                                          wxGridCellEditor* editor)
{
    wxGridDataTypeInfo* info = new wxGridDataTypeInfo(typeName, renderer, editor);

    // is it already registered?
    int loc = FindRegisteredDataType(typeName);
    if ( loc != wxNOT_FOUND )
    {
        delete m_typeinfo[loc];
        m_typeinfo[loc] = info;
    }
    else
    {
        m_typeinfo.Add(info);
    }
}

int wxGridTypeRegistry::FindRegisteredDataType(const wxString& typeName)
{
    size_t count = m_typeinfo.GetCount();
    for ( size_t i = 0; i < count; i++ )
    {
        if ( typeName == m_typeinfo[i]->m_typeName )
        {
            return i;
        }
    }

    return wxNOT_FOUND;
}

int wxGridTypeRegistry::FindDataType(const wxString& typeName)
{
    int index = FindRegisteredDataType(typeName);
    if ( index == wxNOT_FOUND )
    {
        // check whether this is one of the standard ones, in which case
        // register it "on the fly"
#if wxUSE_TEXTCTRL
        if ( typeName == wxGRID_VALUE_STRING )
        {
            RegisterDataType(wxGRID_VALUE_STRING,
                             new wxGridCellStringRenderer,
                             new wxGridCellTextEditor);
        }
        else
#endif // wxUSE_TEXTCTRL
#if wxUSE_CHECKBOX
        if ( typeName == wxGRID_VALUE_BOOL )
        {
            RegisterDataType(wxGRID_VALUE_BOOL,
                             new wxGridCellBoolRenderer,
                             new wxGridCellBoolEditor);
        }
        else
#endif // wxUSE_CHECKBOX
#if wxUSE_TEXTCTRL
        if ( typeName == wxGRID_VALUE_NUMBER )
        {
            RegisterDataType(wxGRID_VALUE_NUMBER,
                             new wxGridCellNumberRenderer,
                             new wxGridCellNumberEditor);
        }
        else if ( typeName == wxGRID_VALUE_FLOAT )
        {
            RegisterDataType(wxGRID_VALUE_FLOAT,
                             new wxGridCellFloatRenderer,
                             new wxGridCellFloatEditor);
        }
        else
#endif // wxUSE_TEXTCTRL
#if wxUSE_COMBOBOX
        if ( typeName == wxGRID_VALUE_CHOICE )
        {
            RegisterDataType(wxGRID_VALUE_CHOICE,
                             new wxGridCellStringRenderer,
                             new wxGridCellChoiceEditor);
        }
        else
#endif // wxUSE_COMBOBOX
        {
            return wxNOT_FOUND;
        }

        // we get here only if just added the entry for this type, so return
        // the last index
        index = m_typeinfo.GetCount() - 1;
    }

    return index;
}

int wxGridTypeRegistry::FindOrCloneDataType(const wxString& typeName)
{
    int index = FindDataType(typeName);
    if ( index == wxNOT_FOUND )
    {
        // the first part of the typename is the "real" type, anything after ':'
        // are the parameters for the renderer
        index = FindDataType(typeName.BeforeFirst(_T(':')));
        if ( index == wxNOT_FOUND )
        {
            return wxNOT_FOUND;
        }

        wxGridCellRenderer *renderer = GetRenderer(index);
        wxGridCellRenderer *rendererOld = renderer;
        renderer = renderer->Clone();
        rendererOld->DecRef();

        wxGridCellEditor *editor = GetEditor(index);
        wxGridCellEditor *editorOld = editor;
        editor = editor->Clone();
        editorOld->DecRef();

        // do it even if there are no parameters to reset them to defaults
        wxString params = typeName.AfterFirst(_T(':'));
        renderer->SetParameters(params);
        editor->SetParameters(params);

        // register the new typename
        RegisterDataType(typeName, renderer, editor);

        // we just registered it, it's the last one
        index = m_typeinfo.GetCount() - 1;
    }

    return index;
}

wxGridCellRenderer* wxGridTypeRegistry::GetRenderer(int index)
{
    wxGridCellRenderer* renderer = m_typeinfo[index]->m_renderer;
    if (renderer)
        renderer->IncRef();

    return renderer;
}

wxGridCellEditor* wxGridTypeRegistry::GetEditor(int index)
{
    wxGridCellEditor* editor = m_typeinfo[index]->m_editor;
    if (editor)
        editor->IncRef();

    return editor;
}

// ----------------------------------------------------------------------------
// wxGridTableBase
// ----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS( wxGridTableBase, wxObject )

wxGridTableBase::wxGridTableBase()
{
    m_view = (wxGrid *) NULL;
    m_attrProvider = (wxGridCellAttrProvider *) NULL;
}

wxGridTableBase::~wxGridTableBase()
{
    delete m_attrProvider;
}

void wxGridTableBase::SetAttrProvider(wxGridCellAttrProvider *attrProvider)
{
    delete m_attrProvider;
    m_attrProvider = attrProvider;
}

bool wxGridTableBase::CanHaveAttributes()
{
    if ( ! GetAttrProvider() )
    {
        // use the default attr provider by default
        SetAttrProvider(new wxGridCellAttrProvider);
    }

    return true;
}

wxGridCellAttr *wxGridTableBase::GetAttr(int row, int col, wxGridCellAttr::wxAttrKind  kind)
{
    if ( m_attrProvider )
        return m_attrProvider->GetAttr(row, col, kind);
    else
        return (wxGridCellAttr *)NULL;
}

void wxGridTableBase::SetAttr(wxGridCellAttr* attr, int row, int col)
{
    if ( m_attrProvider )
    {
        if ( attr )
            attr->SetKind(wxGridCellAttr::Cell);
        m_attrProvider->SetAttr(attr, row, col);
    }
    else
    {
        // as we take ownership of the pointer and don't store it, we must
        // free it now
        wxSafeDecRef(attr);
    }
}

void wxGridTableBase::SetRowAttr(wxGridCellAttr *attr, int row)
{
    if ( m_attrProvider )
    {
        attr->SetKind(wxGridCellAttr::Row);
        m_attrProvider->SetRowAttr(attr, row);
    }
    else
    {
        // as we take ownership of the pointer and don't store it, we must
        // free it now
        wxSafeDecRef(attr);
    }
}

void wxGridTableBase::SetColAttr(wxGridCellAttr *attr, int col)
{
    if ( m_attrProvider )
    {
        attr->SetKind(wxGridCellAttr::Col);
        m_attrProvider->SetColAttr(attr, col);
    }
    else
    {
        // as we take ownership of the pointer and don't store it, we must
        // free it now
        wxSafeDecRef(attr);
    }
}

bool wxGridTableBase::InsertRows( size_t WXUNUSED(pos),
                                  size_t WXUNUSED(numRows) )
{
    wxFAIL_MSG( wxT("Called grid table class function InsertRows\nbut your derived table class does not override this function") );

    return false;
}

bool wxGridTableBase::AppendRows( size_t WXUNUSED(numRows) )
{
    wxFAIL_MSG( wxT("Called grid table class function AppendRows\nbut your derived table class does not override this function"));

    return false;
}

bool wxGridTableBase::DeleteRows( size_t WXUNUSED(pos),
                                  size_t WXUNUSED(numRows) )
{
    wxFAIL_MSG( wxT("Called grid table class function DeleteRows\nbut your derived table class does not override this function"));

    return false;
}

bool wxGridTableBase::InsertCols( size_t WXUNUSED(pos),
                                  size_t WXUNUSED(numCols) )
{
    wxFAIL_MSG( wxT("Called grid table class function InsertCols\nbut your derived table class does not override this function"));

    return false;
}

bool wxGridTableBase::AppendCols( size_t WXUNUSED(numCols) )
{
    wxFAIL_MSG(wxT("Called grid table class function AppendCols\nbut your derived table class does not override this function"));

    return false;
}

bool wxGridTableBase::DeleteCols( size_t WXUNUSED(pos),
                                  size_t WXUNUSED(numCols) )
{
    wxFAIL_MSG( wxT("Called grid table class function DeleteCols\nbut your derived table class does not override this function"));

    return false;
}

wxString wxGridTableBase::GetRowLabelValue( int row )
{
    wxString s;

    // RD: Starting the rows at zero confuses users,
    // no matter how much it makes sense to us geeks.
    s << row + 1;

    return s;
}

wxString wxGridTableBase::GetColLabelValue( int col )
{
    // default col labels are:
    //   cols 0 to 25   : A-Z
    //   cols 26 to 675 : AA-ZZ
    //   etc.

    wxString s;
    unsigned int i, n;
    for ( n = 1; ; n++ )
    {
        s += (wxChar) (_T('A') + (wxChar)(col % 26));
        col = col / 26 - 1;
        if ( col < 0 )
            break;
    }

    // reverse the string...
    wxString s2;
    for ( i = 0; i < n; i++ )
    {
        s2 += s[n - i - 1];
    }

    return s2;
}

wxString wxGridTableBase::GetTypeName( int WXUNUSED(row), int WXUNUSED(col) )
{
    return wxGRID_VALUE_STRING;
}

bool wxGridTableBase::CanGetValueAs( int WXUNUSED(row), int WXUNUSED(col),
                                     const wxString& typeName )
{
    return typeName == wxGRID_VALUE_STRING;
}

bool wxGridTableBase::CanSetValueAs( int row, int col, const wxString& typeName )
{
    return CanGetValueAs(row, col, typeName);
}

long wxGridTableBase::GetValueAsLong( int WXUNUSED(row), int WXUNUSED(col) )
{
    return 0;
}

double wxGridTableBase::GetValueAsDouble( int WXUNUSED(row), int WXUNUSED(col) )
{
    return 0.0;
}

bool wxGridTableBase::GetValueAsBool( int WXUNUSED(row), int WXUNUSED(col) )
{
    return false;
}

void wxGridTableBase::SetValueAsLong( int WXUNUSED(row), int WXUNUSED(col),
                                      long WXUNUSED(value) )
{
}

void wxGridTableBase::SetValueAsDouble( int WXUNUSED(row), int WXUNUSED(col),
                                        double WXUNUSED(value) )
{
}

void wxGridTableBase::SetValueAsBool( int WXUNUSED(row), int WXUNUSED(col),
                                      bool WXUNUSED(value) )
{
}

void* wxGridTableBase::GetValueAsCustom( int WXUNUSED(row), int WXUNUSED(col),
                                         const wxString& WXUNUSED(typeName) )
{
    return NULL;
}

void  wxGridTableBase::SetValueAsCustom( int WXUNUSED(row), int WXUNUSED(col),
                                         const wxString& WXUNUSED(typeName),
                                         void* WXUNUSED(value) )
{
}

//////////////////////////////////////////////////////////////////////
//
// Message class for the grid table to send requests and notifications
// to the grid view
//

wxGridTableMessage::wxGridTableMessage()
{
    m_table = (wxGridTableBase *) NULL;
    m_id = -1;
    m_comInt1 = -1;
    m_comInt2 = -1;
}

wxGridTableMessage::wxGridTableMessage( wxGridTableBase *table, int id,
                                        int commandInt1, int commandInt2 )
{
    m_table = table;
    m_id = id;
    m_comInt1 = commandInt1;
    m_comInt2 = commandInt2;
}

//////////////////////////////////////////////////////////////////////
//
// A basic grid table for string data. An object of this class will
// created by wxGrid if you don't specify an alternative table class.
//

WX_DEFINE_OBJARRAY(wxGridStringArray)

IMPLEMENT_DYNAMIC_CLASS( wxGridStringTable, wxGridTableBase )

wxGridStringTable::wxGridStringTable()
        : wxGridTableBase()
{
}

wxGridStringTable::wxGridStringTable( int numRows, int numCols )
        : wxGridTableBase()
{
    m_data.Alloc( numRows );

    wxArrayString sa;
    sa.Alloc( numCols );
    sa.Add( wxEmptyString, numCols );

    m_data.Add( sa, numRows );
}

wxGridStringTable::~wxGridStringTable()
{
}

int wxGridStringTable::GetNumberRows()
{
    return m_data.GetCount();
}

int wxGridStringTable::GetNumberCols()
{
    if ( m_data.GetCount() > 0 )
        return m_data[0].GetCount();
    else
        return 0;
}

wxString wxGridStringTable::GetValue( int row, int col )
{
    wxCHECK_MSG( (row < GetNumberRows()) && (col < GetNumberCols()),
                 wxEmptyString,
                 _T("invalid row or column index in wxGridStringTable") );

    return m_data[row][col];
}

void wxGridStringTable::SetValue( int row, int col, const wxString& value )
{
    wxCHECK_RET( (row < GetNumberRows()) && (col < GetNumberCols()),
                 _T("invalid row or column index in wxGridStringTable") );

    m_data[row][col] = value;
}

bool wxGridStringTable::IsEmptyCell( int row, int col )
{
    wxCHECK_MSG( (row < GetNumberRows()) && (col < GetNumberCols()),
                 true,
                  _T("invalid row or column index in wxGridStringTable") );

    return (m_data[row][col] == wxEmptyString);
}

void wxGridStringTable::Clear()
{
    int row, col;
    int numRows, numCols;

    numRows = m_data.GetCount();
    if ( numRows > 0 )
    {
        numCols = m_data[0].GetCount();

        for ( row = 0; row < numRows; row++ )
        {
            for ( col = 0; col < numCols; col++ )
            {
                m_data[row][col] = wxEmptyString;
            }
        }
    }
}

bool wxGridStringTable::InsertRows( size_t pos, size_t numRows )
{
    size_t curNumRows = m_data.GetCount();
    size_t curNumCols = ( curNumRows > 0 ? m_data[0].GetCount() :
                          ( GetView() ? GetView()->GetNumberCols() : 0 ) );

    if ( pos >= curNumRows )
    {
        return AppendRows( numRows );
    }

    wxArrayString sa;
    sa.Alloc( curNumCols );
    sa.Add( wxEmptyString, curNumCols );
    m_data.Insert( sa, pos, numRows );

    if ( GetView() )
    {
        wxGridTableMessage msg( this,
                                wxGRIDTABLE_NOTIFY_ROWS_INSERTED,
                                pos,
                                numRows );

        GetView()->ProcessTableMessage( msg );
    }

    return true;
}

bool wxGridStringTable::AppendRows( size_t numRows )
{
    size_t curNumRows = m_data.GetCount();
    size_t curNumCols = ( curNumRows > 0
                         ? m_data[0].GetCount()
                         : ( GetView() ? GetView()->GetNumberCols() : 0 ) );

    wxArrayString sa;
    if ( curNumCols > 0 )
    {
        sa.Alloc( curNumCols );
        sa.Add( wxEmptyString, curNumCols );
    }

    m_data.Add( sa, numRows );

    if ( GetView() )
    {
        wxGridTableMessage msg( this,
                                wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
                                numRows );

        GetView()->ProcessTableMessage( msg );
    }

    return true;
}

bool wxGridStringTable::DeleteRows( size_t pos, size_t numRows )
{
    size_t curNumRows = m_data.GetCount();

    if ( pos >= curNumRows )
    {
        wxFAIL_MSG( wxString::Format
                    (
                        wxT("Called wxGridStringTable::DeleteRows(pos=%lu, N=%lu)\nPos value is invalid for present table with %lu rows"),
                        (unsigned long)pos,
                        (unsigned long)numRows,
                        (unsigned long)curNumRows
                    ) );

        return false;
    }

    if ( numRows > curNumRows - pos )
    {
        numRows = curNumRows - pos;
    }

    if ( numRows >= curNumRows )
    {
        m_data.Clear();
    }
    else
    {
        m_data.RemoveAt( pos, numRows );
    }

    if ( GetView() )
    {
        wxGridTableMessage msg( this,
                                wxGRIDTABLE_NOTIFY_ROWS_DELETED,
                                pos,
                                numRows );

        GetView()->ProcessTableMessage( msg );
    }

    return true;
}

bool wxGridStringTable::InsertCols( size_t pos, size_t numCols )
{
    size_t row, col;

    size_t curNumRows = m_data.GetCount();
    size_t curNumCols = ( curNumRows > 0
                         ? m_data[0].GetCount()
                         : ( GetView() ? GetView()->GetNumberCols() : 0 ) );

    if ( pos >= curNumCols )
    {
        return AppendCols( numCols );
    }

    if ( !m_colLabels.IsEmpty() )
    {
        m_colLabels.Insert( wxEmptyString, pos, numCols );

        size_t i;
        for ( i = pos; i < pos + numCols; i++ )
            m_colLabels[i] = wxGridTableBase::GetColLabelValue( i );
    }

    for ( row = 0; row < curNumRows; row++ )
    {
        for ( col = pos; col < pos + numCols; col++ )
        {
            m_data[row].Insert( wxEmptyString, col );
        }
    }

    if ( GetView() )
    {
        wxGridTableMessage msg( this,
                                wxGRIDTABLE_NOTIFY_COLS_INSERTED,
                                pos,
                                numCols );

        GetView()->ProcessTableMessage( msg );
    }

    return true;
}

bool wxGridStringTable::AppendCols( size_t numCols )
{
    size_t row;

    size_t curNumRows = m_data.GetCount();

#if 0
    if ( !curNumRows )
    {
        // TODO: something better than this ?
        //
        wxFAIL_MSG( wxT("Unable to append cols to a grid table with no rows.\nCall AppendRows() first") );
        return false;
    }
#endif

    for ( row = 0; row < curNumRows; row++ )
    {
        m_data[row].Add( wxEmptyString, numCols );
    }

    if ( GetView() )
    {
        wxGridTableMessage msg( this,
                                wxGRIDTABLE_NOTIFY_COLS_APPENDED,
                                numCols );

        GetView()->ProcessTableMessage( msg );
    }

    return true;
}

bool wxGridStringTable::DeleteCols( size_t pos, size_t numCols )
{
    size_t row;

    size_t curNumRows = m_data.GetCount();
    size_t curNumCols = ( curNumRows > 0 ? m_data[0].GetCount() :
                          ( GetView() ? GetView()->GetNumberCols() : 0 ) );

    if ( pos >= curNumCols )
    {
        wxFAIL_MSG( wxString::Format
                    (
                        wxT("Called wxGridStringTable::DeleteCols(pos=%lu, N=%lu)\nPos value is invalid for present table with %lu cols"),
                        (unsigned long)pos,
                        (unsigned long)numCols,
                        (unsigned long)curNumCols
                    ) );
        return false;
    }

    int colID;
    if ( GetView() )
        colID = GetView()->GetColAt( pos );
    else
        colID = pos;

    if ( numCols > curNumCols - colID )
    {
        numCols = curNumCols - colID;
    }

    if ( !m_colLabels.IsEmpty() )
    {
        // m_colLabels stores just as many elements as it needs, e.g. if only
        // the label of the first column had been set it would have only one
        // element and not numCols, so account for it
        int nToRm = m_colLabels.size() - colID;
        if ( nToRm > 0 )
            m_colLabels.RemoveAt( colID, nToRm );
    }

    for ( row = 0; row < curNumRows; row++ )
    {
        if ( numCols >= curNumCols )
        {
            m_data[row].Clear();
        }
        else
        {
            m_data[row].RemoveAt( colID, numCols );
        }
    }

    if ( GetView() )
    {
        wxGridTableMessage msg( this,
                                wxGRIDTABLE_NOTIFY_COLS_DELETED,
                                pos,
                                numCols );

        GetView()->ProcessTableMessage( msg );
    }

    return true;
}

wxString wxGridStringTable::GetRowLabelValue( int row )
{
    if ( row > (int)(m_rowLabels.GetCount()) - 1 )
    {
        // using default label
        //
        return wxGridTableBase::GetRowLabelValue( row );
    }
    else
    {
        return m_rowLabels[row];
    }
}

wxString wxGridStringTable::GetColLabelValue( int col )
{
    if ( col > (int)(m_colLabels.GetCount()) - 1 )
    {
        // using default label
        //
        return wxGridTableBase::GetColLabelValue( col );
    }
    else
    {
        return m_colLabels[col];
    }
}

void wxGridStringTable::SetRowLabelValue( int row, const wxString& value )
{
    if ( row > (int)(m_rowLabels.GetCount()) - 1 )
    {
        int n = m_rowLabels.GetCount();
        int i;

        for ( i = n; i <= row; i++ )
        {
            m_rowLabels.Add( wxGridTableBase::GetRowLabelValue(i) );
        }
    }

    m_rowLabels[row] = value;
}

void wxGridStringTable::SetColLabelValue( int col, const wxString& value )
{
    if ( col > (int)(m_colLabels.GetCount()) - 1 )
    {
        int n = m_colLabels.GetCount();
        int i;

        for ( i = n; i <= col; i++ )
        {
            m_colLabels.Add( wxGridTableBase::GetColLabelValue(i) );
        }
    }

    m_colLabels[col] = value;
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS( wxGridRowLabelWindow, wxWindow )

BEGIN_EVENT_TABLE( wxGridRowLabelWindow, wxWindow )
    EVT_PAINT( wxGridRowLabelWindow::OnPaint )
    EVT_MOUSEWHEEL( wxGridRowLabelWindow::OnMouseWheel )
    EVT_MOUSE_EVENTS( wxGridRowLabelWindow::OnMouseEvent )
    EVT_KEY_DOWN( wxGridRowLabelWindow::OnKeyDown )
    EVT_KEY_UP( wxGridRowLabelWindow::OnKeyUp )
    EVT_CHAR( wxGridRowLabelWindow::OnChar )
END_EVENT_TABLE()

wxGridRowLabelWindow::wxGridRowLabelWindow( wxGrid *parent,
                                            wxWindowID id,
                                            const wxPoint &pos, const wxSize &size )
  : wxWindow( parent, id, pos, size, wxWANTS_CHARS | wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE )
{
    m_owner = parent;
}

void wxGridRowLabelWindow::OnPaint( wxPaintEvent& WXUNUSED(event) )
{
    wxPaintDC dc(this);

    // NO - don't do this because it will set both the x and y origin
    // coords to match the parent scrolled window and we just want to
    // set the y coord  - MB
    //
    // m_owner->PrepareDC( dc );

    int x, y;
    m_owner->CalcUnscrolledPosition( 0, 0, &x, &y );
    wxPoint pt = dc.GetDeviceOrigin();
    dc.SetDeviceOrigin( pt.x, pt.y-y );

    wxArrayInt rows = m_owner->CalcRowLabelsExposed( GetUpdateRegion() );
    m_owner->DrawRowLabels( dc, rows );
}

void wxGridRowLabelWindow::OnMouseEvent( wxMouseEvent& event )
{
    m_owner->ProcessRowLabelMouseEvent( event );
}

void wxGridRowLabelWindow::OnMouseWheel( wxMouseEvent& event )
{
    m_owner->GetEventHandler()->ProcessEvent( event );
}

// This seems to be required for wxMotif otherwise the mouse
// cursor must be in the cell edit control to get key events
//
void wxGridRowLabelWindow::OnKeyDown( wxKeyEvent& event )
{
    if ( !m_owner->GetEventHandler()->ProcessEvent( event ) )
        event.Skip();
}

void wxGridRowLabelWindow::OnKeyUp( wxKeyEvent& event )
{
    if ( !m_owner->GetEventHandler()->ProcessEvent( event ) )
        event.Skip();
}

void wxGridRowLabelWindow::OnChar( wxKeyEvent& event )
{
    if ( !m_owner->GetEventHandler()->ProcessEvent( event ) )
        event.Skip();
}

//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS( wxGridColLabelWindow, wxWindow )

BEGIN_EVENT_TABLE( wxGridColLabelWindow, wxWindow )
    EVT_PAINT( wxGridColLabelWindow::OnPaint )
    EVT_MOUSEWHEEL( wxGridColLabelWindow::OnMouseWheel )
    EVT_MOUSE_EVENTS( wxGridColLabelWindow::OnMouseEvent )
    EVT_KEY_DOWN( wxGridColLabelWindow::OnKeyDown )
    EVT_KEY_UP( wxGridColLabelWindow::OnKeyUp )
    EVT_CHAR( wxGridColLabelWindow::OnChar )
END_EVENT_TABLE()

wxGridColLabelWindow::wxGridColLabelWindow( wxGrid *parent,
                                            wxWindowID id,
                                            const wxPoint &pos, const wxSize &size )
  : wxWindow( parent, id, pos, size, wxWANTS_CHARS | wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE )
{
    m_owner = parent;
}

void wxGridColLabelWindow::OnPaint( wxPaintEvent& WXUNUSED(event) )
{
    wxPaintDC dc(this);

    // NO - don't do this because it will set both the x and y origin
    // coords to match the parent scrolled window and we just want to
    // set the x coord  - MB
    //
    // m_owner->PrepareDC( dc );

    int x, y;
    m_owner->CalcUnscrolledPosition( 0, 0, &x, &y );
    wxPoint pt = dc.GetDeviceOrigin();
    if (GetLayoutDirection() == wxLayout_RightToLeft)
        dc.SetDeviceOrigin( pt.x+x, pt.y );
    else
        dc.SetDeviceOrigin( pt.x-x, pt.y );

    wxArrayInt cols = m_owner->CalcColLabelsExposed( GetUpdateRegion() );
    m_owner->DrawColLabels( dc, cols );
}

void wxGridColLabelWindow::OnMouseEvent( wxMouseEvent& event )
{
    m_owner->ProcessColLabelMouseEvent( event );
}

void wxGridColLabelWindow::OnMouseWheel( wxMouseEvent& event )
{
    m_owner->GetEventHandler()->ProcessEvent( event );
}

// This seems to be required for wxMotif otherwise the mouse
// cursor must be in the cell edit control to get key events
//
void wxGridColLabelWindow::OnKeyDown( wxKeyEvent& event )
{
    if ( !m_owner->GetEventHandler()->ProcessEvent( event ) )
        event.Skip();
}

void wxGridColLabelWindow::OnKeyUp( wxKeyEvent& event )
{
    if ( !m_owner->GetEventHandler()->ProcessEvent( event ) )
        event.Skip();
}

void wxGridColLabelWindow::OnChar( wxKeyEvent& event )
{
    if ( !m_owner->GetEventHandler()->ProcessEvent( event ) )
        event.Skip();
}

//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS( wxGridCornerLabelWindow, wxWindow )

BEGIN_EVENT_TABLE( wxGridCornerLabelWindow, wxWindow )
    EVT_MOUSEWHEEL( wxGridCornerLabelWindow::OnMouseWheel )
    EVT_MOUSE_EVENTS( wxGridCornerLabelWindow::OnMouseEvent )
    EVT_PAINT( wxGridCornerLabelWindow::OnPaint )
    EVT_KEY_DOWN( wxGridCornerLabelWindow::OnKeyDown )
    EVT_KEY_UP( wxGridCornerLabelWindow::OnKeyUp )
    EVT_CHAR( wxGridCornerLabelWindow::OnChar )
END_EVENT_TABLE()

wxGridCornerLabelWindow::wxGridCornerLabelWindow( wxGrid *parent,
                                                  wxWindowID id,
                                                  const wxPoint &pos, const wxSize &size )
  : wxWindow( parent, id, pos, size, wxWANTS_CHARS | wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE )
{
    m_owner = parent;
}

void wxGridCornerLabelWindow::OnPaint( wxPaintEvent& WXUNUSED(event) )
{
    wxPaintDC dc(this);

    int client_height = 0;
    int client_width = 0;
    GetClientSize( &client_width, &client_height );

    // VZ: any reason for this ifdef? (FIXME)
#if 0
def __WXGTK__
    wxRect rect;
    rect.SetX( 1 );
    rect.SetY( 1 );
    rect.SetWidth( client_width - 2 );
    rect.SetHeight( client_height - 2 );

    wxRendererNative::Get().DrawHeaderButton( this, dc, rect, 0 );
#else // !__WXGTK__
    dc.SetPen( wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW), 1, wxSOLID) );
    dc.DrawLine( client_width - 1, client_height - 1, client_width - 1, 0 );
    dc.DrawLine( client_width - 1, client_height - 1, 0, client_height - 1 );
    dc.DrawLine( 0, 0, client_width, 0 );
    dc.DrawLine( 0, 0, 0, client_height );

    dc.SetPen( *wxWHITE_PEN );
    dc.DrawLine( 1, 1, client_width - 1, 1 );
    dc.DrawLine( 1, 1, 1, client_height - 1 );
#endif
}

void wxGridCornerLabelWindow::OnMouseEvent( wxMouseEvent& event )
{
    m_owner->ProcessCornerLabelMouseEvent( event );
}

void wxGridCornerLabelWindow::OnMouseWheel( wxMouseEvent& event )
{
    m_owner->GetEventHandler()->ProcessEvent(event);
}

// This seems to be required for wxMotif otherwise the mouse
// cursor must be in the cell edit control to get key events
//
void wxGridCornerLabelWindow::OnKeyDown( wxKeyEvent& event )
{
    if ( !m_owner->GetEventHandler()->ProcessEvent( event ) )
        event.Skip();
}

void wxGridCornerLabelWindow::OnKeyUp( wxKeyEvent& event )
{
    if ( !m_owner->GetEventHandler()->ProcessEvent( event ) )
        event.Skip();
}

void wxGridCornerLabelWindow::OnChar( wxKeyEvent& event )
{
    if ( !m_owner->GetEventHandler()->ProcessEvent( event ) )
        event.Skip();
}

//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS( wxGridWindow, wxWindow )

BEGIN_EVENT_TABLE( wxGridWindow, wxWindow )
    EVT_PAINT( wxGridWindow::OnPaint )
    EVT_MOUSEWHEEL( wxGridWindow::OnMouseWheel )
    EVT_MOUSE_EVENTS( wxGridWindow::OnMouseEvent )
    EVT_KEY_DOWN( wxGridWindow::OnKeyDown )
    EVT_KEY_UP( wxGridWindow::OnKeyUp )
    EVT_CHAR( wxGridWindow::OnChar )
    EVT_SET_FOCUS( wxGridWindow::OnFocus )
    EVT_KILL_FOCUS( wxGridWindow::OnFocus )
    EVT_ERASE_BACKGROUND( wxGridWindow::OnEraseBackground )
END_EVENT_TABLE()

wxGridWindow::wxGridWindow( wxGrid *parent,
                            wxGridRowLabelWindow *rowLblWin,
                            wxGridColLabelWindow *colLblWin,
                            wxWindowID id,
                            const wxPoint &pos,
                            const wxSize &size )
            : wxWindow(
                parent, id, pos, size,
                wxWANTS_CHARS | wxBORDER_NONE | wxCLIP_CHILDREN | wxFULL_REPAINT_ON_RESIZE,
                wxT("grid window") )
{
    m_owner = parent;
    m_rowLabelWin = rowLblWin;
    m_colLabelWin = colLblWin;
}

void wxGridWindow::OnPaint( wxPaintEvent &WXUNUSED(event) )
{
    wxPaintDC dc( this );
    m_owner->PrepareDC( dc );
    wxRegion reg = GetUpdateRegion();
    wxGridCellCoordsArray dirtyCells = m_owner->CalcCellsExposed( reg );
    m_owner->DrawGridCellArea( dc, dirtyCells );

#if WXGRID_DRAW_LINES
    m_owner->DrawAllGridLines( dc, reg );
#endif

    m_owner->DrawGridSpace( dc );
    m_owner->DrawHighlight( dc, dirtyCells );
}

void wxGridWindow::ScrollWindow( int dx, int dy, const wxRect *rect )
{
    wxWindow::ScrollWindow( dx, dy, rect );
    m_rowLabelWin->ScrollWindow( 0, dy, rect );
    m_colLabelWin->ScrollWindow( dx, 0, rect );
}

void wxGridWindow::OnMouseEvent( wxMouseEvent& event )
{
    if (event.ButtonDown(wxMOUSE_BTN_LEFT) && FindFocus() != this)
        SetFocus();

    m_owner->ProcessGridCellMouseEvent( event );
}

void wxGridWindow::OnMouseWheel( wxMouseEvent& event )
{
    m_owner->GetEventHandler()->ProcessEvent( event );
}

// This seems to be required for wxMotif/wxGTK otherwise the mouse
// cursor must be in the cell edit control to get key events
//
void wxGridWindow::OnKeyDown( wxKeyEvent& event )
{
    if ( !m_owner->GetEventHandler()->ProcessEvent( event ) )
        event.Skip();
}

void wxGridWindow::OnKeyUp( wxKeyEvent& event )
{
    if ( !m_owner->GetEventHandler()->ProcessEvent( event ) )
        event.Skip();
}

void wxGridWindow::OnChar( wxKeyEvent& event )
{
    if ( !m_owner->GetEventHandler()->ProcessEvent( event ) )
        event.Skip();
}

void wxGridWindow::OnEraseBackground( wxEraseEvent& WXUNUSED(event) )
{
}

void wxGridWindow::OnFocus(wxFocusEvent& event)
{
    // current cell cursor {dis,re}appears on focus change:
    wxRect cursor = m_owner->CellToRect(m_owner->GetGridCursorRow(),
                                        m_owner->GetGridCursorCol());
    Refresh(true, &cursor);

    // and if we have any selection, it has to be repainted, because it
    // uses different colour when the grid is not focused:
    if ( m_owner->IsSelection() )
    {
        Refresh();
    }
    else
    {
        // NB: Note that this code is in "else" branch only because the other
        //     branch refreshes everything and so there's no point in calling
        //     Refresh() again, *not* because it should only be done if
        //     !IsSelection(). If the above code is ever optimized to refresh
        //     only selected area, this needs to be moved out of the "else"
        //     branch so that it's always executed.

        // current cell cursor {dis,re}appears on focus change:
        const wxGridCellCoords cursorCoords(m_owner->GetGridCursorRow(),
                                            m_owner->GetGridCursorCol());
        const wxRect cursor =
            m_owner->BlockToDeviceRect(cursorCoords, cursorCoords);
        Refresh(true, &cursor);
    }

    if ( !m_owner->GetEventHandler()->ProcessEvent( event ) )
        event.Skip();
}

//////////////////////////////////////////////////////////////////////

// Internal Helper function for computing row or column from some
// (unscrolled) coordinate value, using either
// m_defaultRowHeight/m_defaultColWidth or binary search on array
// of m_rowBottoms/m_ColRights to speed up the search!

// Internal helper macros for simpler use of that function

static int CoordToRowOrCol(int coord, int defaultDist, int minDist,
                           const wxArrayInt& BorderArray, int nMax,
                           bool clipToMinMax);

#define internalXToCol(x) XToCol(x, true)
#define internalYToRow(y) CoordToRowOrCol(y, m_defaultRowHeight, \
                                          m_minAcceptableRowHeight, \
                                          m_rowBottoms, m_numRows, true)

/////////////////////////////////////////////////////////////////////

#if wxUSE_EXTENDED_RTTI
WX_DEFINE_FLAGS( wxGridStyle )

wxBEGIN_FLAGS( wxGridStyle )
    // new style border flags, we put them first to
    // use them for streaming out
    wxFLAGS_MEMBER(wxBORDER_SIMPLE)
    wxFLAGS_MEMBER(wxBORDER_SUNKEN)
    wxFLAGS_MEMBER(wxBORDER_DOUBLE)
    wxFLAGS_MEMBER(wxBORDER_RAISED)
    wxFLAGS_MEMBER(wxBORDER_STATIC)
    wxFLAGS_MEMBER(wxBORDER_NONE)

    // old style border flags
    wxFLAGS_MEMBER(wxSIMPLE_BORDER)
    wxFLAGS_MEMBER(wxSUNKEN_BORDER)
    wxFLAGS_MEMBER(wxDOUBLE_BORDER)
    wxFLAGS_MEMBER(wxRAISED_BORDER)
    wxFLAGS_MEMBER(wxSTATIC_BORDER)
    wxFLAGS_MEMBER(wxBORDER)

    // standard window styles
    wxFLAGS_MEMBER(wxTAB_TRAVERSAL)
    wxFLAGS_MEMBER(wxCLIP_CHILDREN)
    wxFLAGS_MEMBER(wxTRANSPARENT_WINDOW)
    wxFLAGS_MEMBER(wxWANTS_CHARS)
    wxFLAGS_MEMBER(wxFULL_REPAINT_ON_RESIZE)
    wxFLAGS_MEMBER(wxALWAYS_SHOW_SB)
    wxFLAGS_MEMBER(wxVSCROLL)
    wxFLAGS_MEMBER(wxHSCROLL)

wxEND_FLAGS( wxGridStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxGrid, wxScrolledWindow,"wx/grid.h")

wxBEGIN_PROPERTIES_TABLE(wxGrid)
    wxHIDE_PROPERTY( Children )
    wxPROPERTY_FLAGS( WindowStyle , wxGridStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , EMPTY_MACROVALUE, 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxGrid)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_5( wxGrid , wxWindow* , Parent , wxWindowID , Id , wxPoint , Position , wxSize , Size , long , WindowStyle )

/*
 TODO : Expose more information of a list's layout, etc. via appropriate objects (e.g., NotebookPageInfo)
*/
#else
IMPLEMENT_DYNAMIC_CLASS( wxGrid, wxScrolledWindow )
#endif

BEGIN_EVENT_TABLE( wxGrid, wxScrolledWindow )
    EVT_PAINT( wxGrid::OnPaint )
    EVT_SIZE( wxGrid::OnSize )
    EVT_KEY_DOWN( wxGrid::OnKeyDown )
    EVT_KEY_UP( wxGrid::OnKeyUp )
    EVT_CHAR ( wxGrid::OnChar )
    EVT_ERASE_BACKGROUND( wxGrid::OnEraseBackground )
END_EVENT_TABLE()

wxGrid::wxGrid()
{
    // in order to make sure that a size event is not
    // trigerred in a unfinished state
    m_cornerLabelWin = NULL;
    m_rowLabelWin = NULL;
    m_colLabelWin = NULL;
    m_gridWin = NULL;
}

wxGrid::wxGrid( wxWindow *parent,
                 wxWindowID id,
                 const wxPoint& pos,
                 const wxSize& size,
                 long style,
                 const wxString& name )
  : wxScrolledWindow( parent, id, pos, size, (style | wxWANTS_CHARS), name ),
    m_colMinWidths(GRID_HASH_SIZE),
    m_rowMinHeights(GRID_HASH_SIZE)
{
    Create();
    SetInitialSize(size);
}

bool wxGrid::Create(wxWindow *parent, wxWindowID id,
                          const wxPoint& pos, const wxSize& size,
                          long style, const wxString& name)
{
    if (!wxScrolledWindow::Create(parent, id, pos, size,
                                  style | wxWANTS_CHARS, name))
        return false;

    m_colMinWidths = wxLongToLongHashMap(GRID_HASH_SIZE);
    m_rowMinHeights = wxLongToLongHashMap(GRID_HASH_SIZE);

    Create();
    SetInitialSize(size);
    CalcDimensions();

    return true;
}

wxGrid::~wxGrid()
{
    if ( m_winCapture && m_winCapture->HasCapture() )
        m_winCapture->ReleaseMouse();

    // Ensure that the editor control is destroyed before the grid is,
    // otherwise we crash later when the editor tries to do something with the
    // half destroyed grid
    HideCellEditControl();

    // Must do this or ~wxScrollHelper will pop the wrong event handler
    SetTargetWindow(this);
    ClearAttrCache();
    wxSafeDecRef(m_defaultCellAttr);

#ifdef DEBUG_ATTR_CACHE
    size_t total = gs_nAttrCacheHits + gs_nAttrCacheMisses;
    wxPrintf(_T("wxGrid attribute cache statistics: "
                "total: %u, hits: %u (%u%%)\n"),
             total, gs_nAttrCacheHits,
             total ? (gs_nAttrCacheHits*100) / total : 0);
#endif

    // if we own the table, just delete it, otherwise at least don't leave it
    // with dangling view pointer
    if ( m_ownTable )
        delete m_table;
    else if ( m_table && m_table->GetView() == this )
        m_table->SetView(NULL);

    delete m_typeRegistry;
    delete m_selection;
}

//
// ----- internal init and update functions
//

// NOTE: If using the default visual attributes works everywhere then this can
// be removed as well as the #else cases below.
#define _USE_VISATTR 0

void wxGrid::Create()
{
    // set to true by CreateGrid
    m_created = false;

    // create the type registry
    m_typeRegistry = new wxGridTypeRegistry;
    m_selection = NULL;

    m_table = (wxGridTableBase *) NULL;
    m_ownTable = false;

    m_cellEditCtrlEnabled = false;

    m_defaultCellAttr = new wxGridCellAttr();

    // Set default cell attributes
    m_defaultCellAttr->SetDefAttr(m_defaultCellAttr);
    m_defaultCellAttr->SetKind(wxGridCellAttr::Default);
    m_defaultCellAttr->SetFont(GetFont());
    m_defaultCellAttr->SetAlignment(wxALIGN_LEFT, wxALIGN_TOP);
    m_defaultCellAttr->SetRenderer(new wxGridCellStringRenderer);
    m_defaultCellAttr->SetEditor(new wxGridCellTextEditor);

#if _USE_VISATTR
    wxVisualAttributes gva = wxListBox::GetClassDefaultAttributes();
    wxVisualAttributes lva = wxPanel::GetClassDefaultAttributes();

    m_defaultCellAttr->SetTextColour(gva.colFg);
    m_defaultCellAttr->SetBackgroundColour(gva.colBg);

#else
    m_defaultCellAttr->SetTextColour(
        wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    m_defaultCellAttr->SetBackgroundColour(
        wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
#endif

    m_numRows = 0;
    m_numCols = 0;
    m_currentCellCoords = wxGridNoCellCoords;

    m_rowLabelWidth = WXGRID_DEFAULT_ROW_LABEL_WIDTH;
    m_colLabelHeight = WXGRID_DEFAULT_COL_LABEL_HEIGHT;

    // subwindow components that make up the wxGrid
    m_rowLabelWin = new wxGridRowLabelWindow( this,
                                              wxID_ANY,
                                              wxDefaultPosition,
                                              wxDefaultSize );

    m_colLabelWin = new wxGridColLabelWindow( this,
                                              wxID_ANY,
                                              wxDefaultPosition,
                                              wxDefaultSize );

    m_cornerLabelWin = new wxGridCornerLabelWindow( this,
                                                    wxID_ANY,
                                                    wxDefaultPosition,
                                                    wxDefaultSize );

    m_gridWin = new wxGridWindow( this,
                                  m_rowLabelWin,
                                  m_colLabelWin,
                                  wxID_ANY,
                                  wxDefaultPosition,
                                  wxDefaultSize );

    SetTargetWindow( m_gridWin );

#if _USE_VISATTR
    wxColour gfg = gva.colFg;
    wxColour gbg = gva.colBg;
    wxColour lfg = lva.colFg;
    wxColour lbg = lva.colBg;
#else
    wxColour gfg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );
    wxColour gbg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
    wxColour lfg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );
    wxColour lbg = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE );
#endif

    m_cornerLabelWin->SetOwnForegroundColour(lfg);
    m_cornerLabelWin->SetOwnBackgroundColour(lbg);
    m_rowLabelWin->SetOwnForegroundColour(lfg);
    m_rowLabelWin->SetOwnBackgroundColour(lbg);
    m_colLabelWin->SetOwnForegroundColour(lfg);
    m_colLabelWin->SetOwnBackgroundColour(lbg);

    m_gridWin->SetOwnForegroundColour(gfg);
    m_gridWin->SetOwnBackgroundColour(gbg);

    Init();
}

bool wxGrid::CreateGrid( int numRows, int numCols,
                         wxGrid::wxGridSelectionModes selmode )
{
    wxCHECK_MSG( !m_created,
                 false,
                 wxT("wxGrid::CreateGrid or wxGrid::SetTable called more than once") );

    m_numRows = numRows;
    m_numCols = numCols;

    m_table = new wxGridStringTable( m_numRows, m_numCols );
    m_table->SetView( this );
    m_ownTable = true;
    m_selection = new wxGridSelection( this, selmode );

    CalcDimensions();

    m_created = true;

    return m_created;
}

void wxGrid::SetSelectionMode(wxGrid::wxGridSelectionModes selmode)
{
    wxCHECK_RET( m_created,
                 wxT("Called wxGrid::SetSelectionMode() before calling CreateGrid()") );

    m_selection->SetSelectionMode( selmode );
}

wxGrid::wxGridSelectionModes wxGrid::GetSelectionMode() const
{
    wxCHECK_MSG( m_created, wxGrid::wxGridSelectCells,
                 wxT("Called wxGrid::GetSelectionMode() before calling CreateGrid()") );

    return m_selection->GetSelectionMode();
}

bool wxGrid::SetTable( wxGridTableBase *table, bool takeOwnership,
                       wxGrid::wxGridSelectionModes selmode )
{
    bool checkSelection = false;
    if ( m_created )
    {
        // stop all processing
        m_created = false;

        if (m_table)
        {
            m_table->SetView(0);
            if( m_ownTable )
                delete m_table;
            m_table = NULL;
        }

        delete m_selection;
        m_selection = NULL;

        m_ownTable = false;
        m_numRows = 0;
        m_numCols = 0;
        checkSelection = true;

        // kill row and column size arrays
        m_colWidths.Empty();
        m_colRights.Empty();
        m_rowHeights.Empty();
        m_rowBottoms.Empty();
    }

    if (table)
    {
        m_numRows = table->GetNumberRows();
        m_numCols = table->GetNumberCols();

        m_table = table;
        m_table->SetView( this );
        m_ownTable = takeOwnership;
        m_selection = new wxGridSelection( this, selmode );
        if (checkSelection)
        {
            // If the newly set table is smaller than the
            // original one current cell and selection regions
            // might be invalid,
            m_selectingKeyboard = wxGridNoCellCoords;
            m_currentCellCoords =
              wxGridCellCoords(wxMin(m_numRows, m_currentCellCoords.GetRow()),
                               wxMin(m_numCols, m_currentCellCoords.GetCol()));
            if (m_selectingTopLeft.GetRow() >= m_numRows ||
                m_selectingTopLeft.GetCol() >= m_numCols)
            {
                m_selectingTopLeft = wxGridNoCellCoords;
                m_selectingBottomRight = wxGridNoCellCoords;
            }
            else
                m_selectingBottomRight =
                  wxGridCellCoords(wxMin(m_numRows,
                                         m_selectingBottomRight.GetRow()),
                                   wxMin(m_numCols,
                                         m_selectingBottomRight.GetCol()));
        }
        CalcDimensions();

        m_created = true;
    }

    return m_created;
}

void wxGrid::Init()
{
    m_rowLabelWidth  = WXGRID_DEFAULT_ROW_LABEL_WIDTH;
    m_colLabelHeight = WXGRID_DEFAULT_COL_LABEL_HEIGHT;

    if ( m_rowLabelWin )
    {
        m_labelBackgroundColour = m_rowLabelWin->GetBackgroundColour();
    }
    else
    {
        m_labelBackgroundColour = *wxWHITE;
    }

    m_labelTextColour = *wxBLACK;

    // init attr cache
    m_attrCache.row = -1;
    m_attrCache.col = -1;
    m_attrCache.attr = NULL;

    // TODO: something better than this ?
    //
    m_labelFont = this->GetFont();
    m_labelFont.SetWeight( wxBOLD );

    m_rowLabelHorizAlign = wxALIGN_CENTRE;
    m_rowLabelVertAlign  = wxALIGN_CENTRE;

    m_colLabelHorizAlign = wxALIGN_CENTRE;
    m_colLabelVertAlign  = wxALIGN_CENTRE;
    m_colLabelTextOrientation = wxHORIZONTAL;

    m_defaultColWidth  = WXGRID_DEFAULT_COL_WIDTH;
    m_defaultRowHeight = m_gridWin->GetCharHeight();

    m_minAcceptableColWidth  = WXGRID_MIN_COL_WIDTH;
    m_minAcceptableRowHeight = WXGRID_MIN_ROW_HEIGHT;

#if defined(__WXMOTIF__) || defined(__WXGTK__)  // see also text ctrl sizing in ShowCellEditControl()
    m_defaultRowHeight += 8;
#else
    m_defaultRowHeight += 4;
#endif

    m_gridLineColour = wxColour( 192,192,192 );
    m_gridLinesEnabled = true;
    m_cellHighlightColour = *wxBLACK;
    m_cellHighlightPenWidth = 2;
    m_cellHighlightROPenWidth = 1;

    m_canDragColMove = false;

    m_cursorMode  = WXGRID_CURSOR_SELECT_CELL;
    m_winCapture = (wxWindow *)NULL;
    m_canDragRowSize = true;
    m_canDragColSize = true;
    m_canDragGridSize = true;
    m_canDragCell = false;
    m_dragLastPos  = -1;
    m_dragRowOrCol = -1;
    m_isDragging = false;
    m_startDragPos = wxDefaultPosition;

    m_waitForSlowClick = false;

    m_rowResizeCursor = wxCursor( wxCURSOR_SIZENS );
    m_colResizeCursor = wxCursor( wxCURSOR_SIZEWE );

    m_currentCellCoords = wxGridNoCellCoords;

    ClearSelection();

    m_selectionBackground = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
    m_selectionForeground = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);

    m_editable = true;  // default for whole grid

    m_inOnKeyDown = false;
    m_batchCount = 0;

    m_extraWidth =
    m_extraHeight = 0;

    m_scrollLineX = GRID_SCROLL_LINE_X;
    m_scrollLineY = GRID_SCROLL_LINE_Y;
}

// ----------------------------------------------------------------------------
// the idea is to call these functions only when necessary because they create
// quite big arrays which eat memory mostly unnecessary - in particular, if
// default widths/heights are used for all rows/columns, we may not use these
// arrays at all
//
// with some extra code, it should be possible to only store the widths/heights
// different from default ones (resulting in space savings for huge grids) but
// this is not done currently
// ----------------------------------------------------------------------------

void wxGrid::InitRowHeights()
{
    m_rowHeights.Empty();
    m_rowBottoms.Empty();

    m_rowHeights.Alloc( m_numRows );
    m_rowBottoms.Alloc( m_numRows );

    m_rowHeights.Add( m_defaultRowHeight, m_numRows );

    int rowBottom = 0;
    for ( int i = 0; i < m_numRows; i++ )
    {
        rowBottom += m_defaultRowHeight;
        m_rowBottoms.Add( rowBottom );
    }
}

void wxGrid::InitColWidths()
{
    m_colWidths.Empty();
    m_colRights.Empty();

    m_colWidths.Alloc( m_numCols );
    m_colRights.Alloc( m_numCols );

    m_colWidths.Add( m_defaultColWidth, m_numCols );

    int colRight = 0;
    for ( int i = 0; i < m_numCols; i++ )
    {
        colRight = ( GetColPos( i ) + 1 ) * m_defaultColWidth;
        m_colRights.Add( colRight );
    }
}

int wxGrid::GetColWidth(int col) const
{
    return m_colWidths.IsEmpty() ? m_defaultColWidth : m_colWidths[col];
}

int wxGrid::GetColLeft(int col) const
{
    return m_colRights.IsEmpty() ? GetColPos( col ) * m_defaultColWidth
                                 : m_colRights[col] - m_colWidths[col];
}

int wxGrid::GetColRight(int col) const
{
    return m_colRights.IsEmpty() ? (GetColPos( col ) + 1) * m_defaultColWidth
                                 : m_colRights[col];
}

int wxGrid::GetRowHeight(int row) const
{
    return m_rowHeights.IsEmpty() ? m_defaultRowHeight : m_rowHeights[row];
}

int wxGrid::GetRowTop(int row) const
{
    return m_rowBottoms.IsEmpty() ? row * m_defaultRowHeight
                                  : m_rowBottoms[row] - m_rowHeights[row];
}

int wxGrid::GetRowBottom(int row) const
{
    return m_rowBottoms.IsEmpty() ? (row + 1) * m_defaultRowHeight
                                  : m_rowBottoms[row];
}

void wxGrid::CalcDimensions()
{
    // compute the size of the scrollable area
    int w = m_numCols > 0 ? GetColRight(GetColAt(m_numCols - 1)) : 0;
    int h = m_numRows > 0 ? GetRowBottom(m_numRows - 1) : 0;

    w += m_extraWidth;
    h += m_extraHeight;

    // take into account editor if shown
    if ( IsCellEditControlShown() )
    {
        int w2, h2;
        int r = m_currentCellCoords.GetRow();
        int c = m_currentCellCoords.GetCol();
        int x = GetColLeft(c);
        int y = GetRowTop(r);

        // how big is the editor
        wxGridCellAttr* attr = GetCellAttr(r, c);
        wxGridCellEditor* editor = attr->GetEditor(this, r, c);
        editor->GetControl()->GetSize(&w2, &h2);
        w2 += x;
        h2 += y;
        if ( w2 > w )
            w = w2;
        if ( h2 > h )
            h = h2;
        editor->DecRef();
        attr->DecRef();
    }

    // preserve (more or less) the previous position
    int x, y;
    GetViewStart( &x, &y );

    // ensure the position is valid for the new scroll ranges
    if ( x >= w )
        x = wxMax( w - 1, 0 );
    if ( y >= h )
        y = wxMax( h - 1, 0 );

    // do set scrollbar parameters
    SetScrollbars( m_scrollLineX, m_scrollLineY,
                   GetScrollX(w), GetScrollY(h),
                   x, y,
                   GetBatchCount() != 0);

    // if our OnSize() hadn't been called (it would if we have scrollbars), we
    // still must reposition the children
    CalcWindowSizes();
}

void wxGrid::CalcWindowSizes()
{
    // escape if the window is has not been fully created yet

    if ( m_cornerLabelWin == NULL )
        return;

    int cw, ch;
    GetClientSize( &cw, &ch );

    // this block of code tries to work around the following problem: the grid
    // could have been just resized to have enough space to show the full grid
    // window contents without the scrollbars, but its client size could be
    // not big enough because the grid has the scrollbars right now and so the
    // scrollbars would remain even though we don't need them any more
    //
    // to prevent this from happening, check if we have enough space for
    // everything without the scrollbars and explicitly disable them then
    wxSize size = GetSize() - GetWindowBorderSize();
    if ( size != wxSize(cw, ch) )
    {
        // check if we have enough space for grid window after accounting for
        // the fixed size elements
        size.x -= m_rowLabelWidth;
        size.y -= m_colLabelHeight;

        const wxSize vsize = m_gridWin->GetVirtualSize();

        if ( size.x >= vsize.x && size.y >= vsize.y )
        {
            // yes, we do, so remove the scrollbars and use the new client size
            // (which should be the same as full window size - borders now)
            SetScrollbars(0, 0, 0, 0);
            GetClientSize(&cw, &ch);
        }
    }

    // the grid may be too small to have enough space for the labels yet, don't
    // size the windows to negative sizes in this case
    int gw = cw - m_rowLabelWidth;
    int gh = ch - m_colLabelHeight;
    if (gw < 0)
        gw = 0;
    if (gh < 0)
        gh = 0;

    if ( m_cornerLabelWin && m_cornerLabelWin->IsShown() )
        m_cornerLabelWin->SetSize( 0, 0, m_rowLabelWidth, m_colLabelHeight );

    if ( m_colLabelWin && m_colLabelWin->IsShown() )
        m_colLabelWin->SetSize( m_rowLabelWidth, 0, gw, m_colLabelHeight );

    if ( m_rowLabelWin && m_rowLabelWin->IsShown() )
        m_rowLabelWin->SetSize( 0, m_colLabelHeight, m_rowLabelWidth, gh );

    if ( m_gridWin && m_gridWin->IsShown() )
        m_gridWin->SetSize( m_rowLabelWidth, m_colLabelHeight, gw, gh );
}

// this is called when the grid table sends a message
// to indicate that it has been redimensioned
//
bool wxGrid::Redimension( wxGridTableMessage& msg )
{
    int i;
    bool result = false;

    // Clear the attribute cache as the attribute might refer to a different
    // cell than stored in the cache after adding/removing rows/columns.
    ClearAttrCache();

    // By the same reasoning, the editor should be dismissed if columns are
    // added or removed. And for consistency, it should IMHO always be
    // removed, not only if the cell "underneath" it actually changes.
    // For now, I intentionally do not save the editor's content as the
    // cell it might want to save that stuff to might no longer exist.
    HideCellEditControl();

#if 0
    // if we were using the default widths/heights so far, we must change them
    // now
    if ( m_colWidths.IsEmpty() )
    {
        InitColWidths();
    }

    if ( m_rowHeights.IsEmpty() )
    {
        InitRowHeights();
    }
#endif

    switch ( msg.GetId() )
    {
        case wxGRIDTABLE_NOTIFY_ROWS_INSERTED:
        {
            size_t pos = msg.GetCommandInt();
            int numRows = msg.GetCommandInt2();

            m_numRows += numRows;

            if ( !m_rowHeights.IsEmpty() )
            {
                m_rowHeights.Insert( m_defaultRowHeight, pos, numRows );
                m_rowBottoms.Insert( 0, pos, numRows );

                int bottom = 0;
                if ( pos > 0 )
                    bottom = m_rowBottoms[pos - 1];

                for ( i = pos; i < m_numRows; i++ )
                {
                    bottom += m_rowHeights[i];
                    m_rowBottoms[i] = bottom;
                }
            }

            if ( m_currentCellCoords == wxGridNoCellCoords )
            {
                // if we have just inserted cols into an empty grid the current
                // cell will be undefined...
                //
                SetCurrentCell( 0, 0 );
            }

            if ( m_selection )
                m_selection->UpdateRows( pos, numRows );
            wxGridCellAttrProvider * attrProvider = m_table->GetAttrProvider();
            if (attrProvider)
                attrProvider->UpdateAttrRows( pos, numRows );

            if ( !GetBatchCount() )
            {
                CalcDimensions();
                m_rowLabelWin->Refresh();
            }
        }
        result = true;
        break;

        case wxGRIDTABLE_NOTIFY_ROWS_APPENDED:
        {
            int numRows = msg.GetCommandInt();
            int oldNumRows = m_numRows;
            m_numRows += numRows;

            if ( !m_rowHeights.IsEmpty() )
            {
                m_rowHeights.Add( m_defaultRowHeight, numRows );
                m_rowBottoms.Add( 0, numRows );

                int bottom = 0;
                if ( oldNumRows > 0 )
                    bottom = m_rowBottoms[oldNumRows - 1];

                for ( i = oldNumRows; i < m_numRows; i++ )
                {
                    bottom += m_rowHeights[i];
                    m_rowBottoms[i] = bottom;
                }
            }

            if ( m_currentCellCoords == wxGridNoCellCoords )
            {
                // if we have just inserted cols into an empty grid the current
                // cell will be undefined...
                //
                SetCurrentCell( 0, 0 );
            }

            if ( !GetBatchCount() )
            {
                CalcDimensions();
                m_rowLabelWin->Refresh();
            }
        }
        result = true;
        break;

        case wxGRIDTABLE_NOTIFY_ROWS_DELETED:
        {
            size_t pos = msg.GetCommandInt();
            int numRows = msg.GetCommandInt2();
            m_numRows -= numRows;

            if ( !m_rowHeights.IsEmpty() )
            {
                m_rowHeights.RemoveAt( pos, numRows );
                m_rowBottoms.RemoveAt( pos, numRows );

                int h = 0;
                for ( i = 0; i < m_numRows; i++ )
                {
                    h += m_rowHeights[i];
                    m_rowBottoms[i] = h;
                }
            }

            if ( !m_numRows )
            {
                m_currentCellCoords = wxGridNoCellCoords;
            }
            else
            {
                if ( m_currentCellCoords.GetRow() >= m_numRows )
                    m_currentCellCoords.Set( 0, 0 );
            }

            if ( m_selection )
                m_selection->UpdateRows( pos, -((int)numRows) );
            wxGridCellAttrProvider * attrProvider = m_table->GetAttrProvider();
            if (attrProvider)
            {
                attrProvider->UpdateAttrRows( pos, -((int)numRows) );

// ifdef'd out following patch from Paul Gammans
#if 0
                // No need to touch column attributes, unless we
                // removed _all_ rows, in this case, we remove
                // all column attributes.
                // I hate to do this here, but the
                // needed data is not available inside UpdateAttrRows.
                if ( !GetNumberRows() )
                    attrProvider->UpdateAttrCols( 0, -GetNumberCols() );
#endif
            }

            if ( !GetBatchCount() )
            {
                CalcDimensions();
                m_rowLabelWin->Refresh();
            }
        }
        result = true;
        break;

        case wxGRIDTABLE_NOTIFY_COLS_INSERTED:
        {
            size_t pos = msg.GetCommandInt();
            int numCols = msg.GetCommandInt2();
            m_numCols += numCols;

            if ( !m_colAt.IsEmpty() )
            {
                //Shift the column IDs
                int i;
                for ( i = 0; i < m_numCols - numCols; i++ )
                {
                    if ( m_colAt[i] >= (int)pos )
                        m_colAt[i] += numCols;
                }

                m_colAt.Insert( pos, pos, numCols );

                //Set the new columns' positions
                for ( i = pos + 1; i < (int)pos + numCols; i++ )
                {
                    m_colAt[i] = i;
                }
            }

            if ( !m_colWidths.IsEmpty() )
            {
                m_colWidths.Insert( m_defaultColWidth, pos, numCols );
                m_colRights.Insert( 0, pos, numCols );

                int right = 0;
                if ( pos > 0 )
                    right = m_colRights[GetColAt( pos - 1 )];

                int colPos;
                for ( colPos = pos; colPos < m_numCols; colPos++ )
                {
                    i = GetColAt( colPos );

                    right += m_colWidths[i];
                    m_colRights[i] = right;
                }
            }

            if ( m_currentCellCoords == wxGridNoCellCoords )
            {
                // if we have just inserted cols into an empty grid the current
                // cell will be undefined...
                //
                SetCurrentCell( 0, 0 );
            }

            if ( m_selection )
                m_selection->UpdateCols( pos, numCols );
            wxGridCellAttrProvider * attrProvider = m_table->GetAttrProvider();
            if (attrProvider)
                attrProvider->UpdateAttrCols( pos, numCols );
            if ( !GetBatchCount() )
            {
                CalcDimensions();
                m_colLabelWin->Refresh();
            }
        }
        result = true;
        break;

        case wxGRIDTABLE_NOTIFY_COLS_APPENDED:
        {
            int numCols = msg.GetCommandInt();
            int oldNumCols = m_numCols;
            m_numCols += numCols;

            if ( !m_colAt.IsEmpty() )
            {
                m_colAt.Add( 0, numCols );

                //Set the new columns' positions
                int i;
                for ( i = oldNumCols; i < m_numCols; i++ )
                {
                    m_colAt[i] = i;
                }
            }

            if ( !m_colWidths.IsEmpty() )
            {
                m_colWidths.Add( m_defaultColWidth, numCols );
                m_colRights.Add( 0, numCols );

                int right = 0;
                if ( oldNumCols > 0 )
                    right = m_colRights[GetColAt( oldNumCols - 1 )];

                int colPos;
                for ( colPos = oldNumCols; colPos < m_numCols; colPos++ )
                {
                    i = GetColAt( colPos );

                    right += m_colWidths[i];
                    m_colRights[i] = right;
                }
            }

            if ( m_currentCellCoords == wxGridNoCellCoords )
            {
                // if we have just inserted cols into an empty grid the current
                // cell will be undefined...
                //
                SetCurrentCell( 0, 0 );
            }
            if ( !GetBatchCount() )
            {
                CalcDimensions();
                m_colLabelWin->Refresh();
            }
        }
        result = true;
        break;

        case wxGRIDTABLE_NOTIFY_COLS_DELETED:
        {
            size_t pos = msg.GetCommandInt();
            int numCols = msg.GetCommandInt2();
            m_numCols -= numCols;

            if ( !m_colAt.IsEmpty() )
            {
                int colID = GetColAt( pos );

                m_colAt.RemoveAt( pos, numCols );

                //Shift the column IDs
                int colPos;
                for ( colPos = 0; colPos < m_numCols; colPos++ )
                {
                    if ( m_colAt[colPos] > colID )
                        m_colAt[colPos] -= numCols;
                }
            }

            if ( !m_colWidths.IsEmpty() )
            {
                m_colWidths.RemoveAt( pos, numCols );
                m_colRights.RemoveAt( pos, numCols );

                int w = 0;
                int colPos;
                for ( colPos = 0; colPos < m_numCols; colPos++ )
                {
                    i = GetColAt( colPos );

                    w += m_colWidths[i];
                    m_colRights[i] = w;
                }
            }

            if ( !m_numCols )
            {
                m_currentCellCoords = wxGridNoCellCoords;
            }
            else
            {
                if ( m_currentCellCoords.GetCol() >= m_numCols )
                  m_currentCellCoords.Set( 0, 0 );
            }

            if ( m_selection )
                m_selection->UpdateCols( pos, -((int)numCols) );
            wxGridCellAttrProvider * attrProvider = m_table->GetAttrProvider();
            if (attrProvider)
            {
                attrProvider->UpdateAttrCols( pos, -((int)numCols) );

// ifdef'd out following patch from Paul Gammans
#if 0
                // No need to touch row attributes, unless we
                // removed _all_ columns, in this case, we remove
                // all row attributes.
                // I hate to do this here, but the
                // needed data is not available inside UpdateAttrCols.
                if ( !GetNumberCols() )
                    attrProvider->UpdateAttrRows( 0, -GetNumberRows() );
#endif
            }

            if ( !GetBatchCount() )
            {
                CalcDimensions();
                m_colLabelWin->Refresh();
            }
        }
        result = true;
        break;
    }

    if (result && !GetBatchCount() )
        m_gridWin->Refresh();

    return result;
}

wxArrayInt wxGrid::CalcRowLabelsExposed( const wxRegion& reg )
{
    wxRegionIterator iter( reg );
    wxRect r;

    wxArrayInt  rowlabels;

    int top, bottom;
    while ( iter )
    {
        r = iter.GetRect();

        // TODO: remove this when we can...
        // There is a bug in wxMotif that gives garbage update
        // rectangles if you jump-scroll a long way by clicking the
        // scrollbar with middle button.  This is a work-around
        //
#if defined(__WXMOTIF__)
        int cw, ch;
        m_gridWin->GetClientSize( &cw, &ch );
        if ( r.GetTop() > ch )
            r.SetTop( 0 );
        r.SetBottom( wxMin( r.GetBottom(), ch ) );
#endif

        // logical bounds of update region
        //
        int dummy;
        CalcUnscrolledPosition( 0, r.GetTop(), &dummy, &top );
        CalcUnscrolledPosition( 0, r.GetBottom(), &dummy, &bottom );

        // find the row labels within these bounds
        //
        int row;
        for ( row = internalYToRow(top); row < m_numRows; row++ )
        {
            if ( GetRowBottom(row) < top )
                continue;

            if ( GetRowTop(row) > bottom )
                break;

            rowlabels.Add( row );
        }

        ++iter;
    }

    return rowlabels;
}

wxArrayInt wxGrid::CalcColLabelsExposed( const wxRegion& reg )
{
    wxRegionIterator iter( reg );
    wxRect r;

    wxArrayInt colLabels;

    int left, right;
    while ( iter )
    {
        r = iter.GetRect();

        // TODO: remove this when we can...
        // There is a bug in wxMotif that gives garbage update
        // rectangles if you jump-scroll a long way by clicking the
        // scrollbar with middle button.  This is a work-around
        //
#if defined(__WXMOTIF__)
        int cw, ch;
        m_gridWin->GetClientSize( &cw, &ch );
        if ( r.GetLeft() > cw )
            r.SetLeft( 0 );
        r.SetRight( wxMin( r.GetRight(), cw ) );
#endif

        // logical bounds of update region
        //
        int dummy;
        CalcUnscrolledPosition( r.GetLeft(), 0, &left, &dummy );
        CalcUnscrolledPosition( r.GetRight(), 0, &right, &dummy );

        // find the cells within these bounds
        //
        int col;
        int colPos;
        for ( colPos = GetColPos( internalXToCol(left) ); colPos < m_numCols; colPos++ )
        {
            col = GetColAt( colPos );

            if ( GetColRight(col) < left )
                continue;

            if ( GetColLeft(col) > right )
                break;

            colLabels.Add( col );
        }

        ++iter;
    }

    return colLabels;
}

wxGridCellCoordsArray wxGrid::CalcCellsExposed( const wxRegion& reg )
{
    wxRegionIterator iter( reg );
    wxRect r;

    wxGridCellCoordsArray  cellsExposed;

    int left, top, right, bottom;
    while ( iter )
    {
        r = iter.GetRect();

        // TODO: remove this when we can...
        // There is a bug in wxMotif that gives garbage update
        // rectangles if you jump-scroll a long way by clicking the
        // scrollbar with middle button.  This is a work-around
        //
#if defined(__WXMOTIF__)
        int cw, ch;
        m_gridWin->GetClientSize( &cw, &ch );
        if ( r.GetTop() > ch ) r.SetTop( 0 );
        if ( r.GetLeft() > cw ) r.SetLeft( 0 );
        r.SetRight( wxMin( r.GetRight(), cw ) );
        r.SetBottom( wxMin( r.GetBottom(), ch ) );
#endif

        // logical bounds of update region
        //
        CalcUnscrolledPosition( r.GetLeft(), r.GetTop(), &left, &top );
        CalcUnscrolledPosition( r.GetRight(), r.GetBottom(), &right, &bottom );

        // find the cells within these bounds
        //
        int row, col;
        for ( row = internalYToRow(top); row < m_numRows; row++ )
        {
            if ( GetRowBottom(row) <= top )
                continue;

            if ( GetRowTop(row) > bottom )
                break;

            int colPos;
            for ( colPos = GetColPos( internalXToCol(left) ); colPos < m_numCols; colPos++ )
            {
                col = GetColAt( colPos );

                if ( GetColRight(col) <= left )
                    continue;

                if ( GetColLeft(col) > right )
                    break;

                cellsExposed.Add( wxGridCellCoords( row, col ) );
            }
        }

        ++iter;
    }

    return cellsExposed;
}


void wxGrid::ProcessRowLabelMouseEvent( wxMouseEvent& event )
{
    int x, y, row;
    wxPoint pos( event.GetPosition() );
    CalcUnscrolledPosition( pos.x, pos.y, &x, &y );

    if ( event.Dragging() )
    {
        if (!m_isDragging)
        {
            m_isDragging = true;
            m_rowLabelWin->CaptureMouse();
        }

        if ( event.LeftIsDown() )
        {
            switch ( m_cursorMode )
            {
                case WXGRID_CURSOR_RESIZE_ROW:
                {
                    int cw, ch, left, dummy;
                    m_gridWin->GetClientSize( &cw, &ch );
                    CalcUnscrolledPosition( 0, 0, &left, &dummy );

                    wxClientDC dc( m_gridWin );
                    PrepareDC( dc );
                    y = wxMax( y,
                               GetRowTop(m_dragRowOrCol) +
                               GetRowMinimalHeight(m_dragRowOrCol) );
                    dc.SetLogicalFunction(wxINVERT);
                    if ( m_dragLastPos >= 0 )
                    {
                        dc.DrawLine( left, m_dragLastPos, left+cw, m_dragLastPos );
                    }
                    dc.DrawLine( left, y, left+cw, y );
                    m_dragLastPos = y;
                }
                break;

                case WXGRID_CURSOR_SELECT_ROW:
                {
                    if ( (row = YToRow( y )) >= 0 )
                    {
                        if ( m_selection )
                        {
                            m_selection->SelectRow( row,
                                                    event.ControlDown(),
                                                    event.ShiftDown(),
                                                    event.AltDown(),
                                                    event.MetaDown() );
                        }
                    }
                }
                break;

                // default label to suppress warnings about "enumeration value
                // 'xxx' not handled in switch
                default:
                    break;
            }
        }
        return;
    }

    if ( m_isDragging && (event.Entering() || event.Leaving()) )
        return;

    if (m_isDragging)
    {
        if (m_rowLabelWin->HasCapture())
            m_rowLabelWin->ReleaseMouse();
        m_isDragging = false;
    }

    // ------------ Entering or leaving the window
    //
    if ( event.Entering() || event.Leaving() )
    {
        ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL, m_rowLabelWin);
    }

    // ------------ Left button pressed
    //
    else if ( event.LeftDown() )
    {
        // don't send a label click event for a hit on the
        // edge of the row label - this is probably the user
        // wanting to resize the row
        //
        if ( YToEdgeOfRow(y) < 0 )
        {
            row = YToRow(y);
            if ( row >= 0 &&
                 !SendEvent( wxEVT_GRID_LABEL_LEFT_CLICK, row, -1, event ) )
            {
                if ( !event.ShiftDown() && !event.CmdDown() )
                    ClearSelection();
                if ( m_selection )
                {
                    if ( event.ShiftDown() )
                    {
                        m_selection->SelectBlock( m_currentCellCoords.GetRow(),
                                                  0,
                                                  row,
                                                  GetNumberCols() - 1,
                                                  event.ControlDown(),
                                                  event.ShiftDown(),
                                                  event.AltDown(),
                                                  event.MetaDown() );
                    }
                    else
                    {
                        m_selection->SelectRow( row,
                                                event.ControlDown(),
                                                event.ShiftDown(),
                                                event.AltDown(),
                                                event.MetaDown() );
                    }
                }

                ChangeCursorMode(WXGRID_CURSOR_SELECT_ROW, m_rowLabelWin);
            }
        }
        else
        {
            // starting to drag-resize a row
            if ( CanDragRowSize() )
                ChangeCursorMode(WXGRID_CURSOR_RESIZE_ROW, m_rowLabelWin);
        }
    }

    // ------------ Left double click
    //
    else if (event.LeftDClick() )
    {
        row = YToEdgeOfRow(y);
        if ( row < 0 )
        {
            row = YToRow(y);
            if ( row >=0 &&
                 !SendEvent( wxEVT_GRID_LABEL_LEFT_DCLICK, row, -1, event ) )
            {
                // no default action at the moment
            }
        }
        else
        {
            // adjust row height depending on label text
            AutoSizeRowLabelSize( row );

            ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL, m_colLabelWin);
            m_dragLastPos = -1;
        }
    }

    // ------------ Left button released
    //
    else if ( event.LeftUp() )
    {
        if ( m_cursorMode == WXGRID_CURSOR_RESIZE_ROW )
        {
            DoEndDragResizeRow();

            // Note: we are ending the event *after* doing
            // default processing in this case
            //
            SendEvent( wxEVT_GRID_ROW_SIZE, m_dragRowOrCol, -1, event );
        }

        ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL, m_rowLabelWin);
        m_dragLastPos = -1;
    }

    // ------------ Right button down
    //
    else if ( event.RightDown() )
    {
        row = YToRow(y);
        if ( row >=0 &&
             !SendEvent( wxEVT_GRID_LABEL_RIGHT_CLICK, row, -1, event ) )
        {
            // no default action at the moment
        }
    }

    // ------------ Right double click
    //
    else if ( event.RightDClick() )
    {
        row = YToRow(y);
        if ( row >= 0 &&
             !SendEvent( wxEVT_GRID_LABEL_RIGHT_DCLICK, row, -1, event ) )
        {
            // no default action at the moment
        }
    }

    // ------------ No buttons down and mouse moving
    //
    else if ( event.Moving() )
    {
        m_dragRowOrCol = YToEdgeOfRow( y );
        if ( m_dragRowOrCol >= 0 )
        {
            if ( m_cursorMode == WXGRID_CURSOR_SELECT_CELL )
            {
                // don't capture the mouse yet
                if ( CanDragRowSize() )
                    ChangeCursorMode(WXGRID_CURSOR_RESIZE_ROW, m_rowLabelWin, false);
            }
        }
        else if ( m_cursorMode != WXGRID_CURSOR_SELECT_CELL )
        {
            ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL, m_rowLabelWin, false);
        }
    }
}

void wxGrid::ProcessColLabelMouseEvent( wxMouseEvent& event )
{
    int x, y, col;
    wxPoint pos( event.GetPosition() );
    CalcUnscrolledPosition( pos.x, pos.y, &x, &y );

    if ( event.Dragging() )
    {
        if (!m_isDragging)
        {
            m_isDragging = true;
            m_colLabelWin->CaptureMouse();

            if ( m_cursorMode == WXGRID_CURSOR_MOVE_COL )
                m_dragRowOrCol = XToCol( x );
        }

        if ( event.LeftIsDown() )
        {
            switch ( m_cursorMode )
            {
                case WXGRID_CURSOR_RESIZE_COL:
                {
                    int cw, ch, dummy, top;
                    m_gridWin->GetClientSize( &cw, &ch );
                    CalcUnscrolledPosition( 0, 0, &dummy, &top );

                    wxClientDC dc( m_gridWin );
                    PrepareDC( dc );

                    x = wxMax( x, GetColLeft(m_dragRowOrCol) +
                                  GetColMinimalWidth(m_dragRowOrCol));
                    dc.SetLogicalFunction(wxINVERT);
                    if ( m_dragLastPos >= 0 )
                    {
                        dc.DrawLine( m_dragLastPos, top, m_dragLastPos, top + ch );
                    }
                    dc.DrawLine( x, top, x, top + ch );
                    m_dragLastPos = x;
                }
                break;

                case WXGRID_CURSOR_SELECT_COL:
                {
                    if ( (col = XToCol( x )) >= 0 )
                    {
                        if ( m_selection )
                        {
                            m_selection->SelectCol( col,
                                                    event.ControlDown(),
                                                    event.ShiftDown(),
                                                    event.AltDown(),
                                                    event.MetaDown() );
                        }
                    }
                }
                break;

                case WXGRID_CURSOR_MOVE_COL:
                {
                    if ( x < 0 )
                        m_moveToCol = GetColAt( 0 );
                    else
                        m_moveToCol = XToCol( x );

                    int markerX;

                    if ( m_moveToCol < 0 )
                        markerX = GetColRight( GetColAt( m_numCols - 1 ) );
                    else
                        markerX = GetColLeft( m_moveToCol );

                    if ( markerX != m_dragLastPos )
                    {
                        wxClientDC dc( m_colLabelWin );
                        DoPrepareDC(dc);

                        int cw, ch;
                        m_colLabelWin->GetClientSize( &cw, &ch );

                        markerX++;

                        //Clean up the last indicator
                        if ( m_dragLastPos >= 0 )
                        {
                            wxPen pen( m_colLabelWin->GetBackgroundColour(), 2 );
                            dc.SetPen(pen);
                            dc.DrawLine( m_dragLastPos + 1, 0, m_dragLastPos + 1, ch );
                            dc.SetPen(wxNullPen);

                            if ( XToCol( m_dragLastPos ) != -1 )
                                DrawColLabel( dc, XToCol( m_dragLastPos ) );
                        }

                        //Moving to the same place? Don't draw a marker
                        if ( (m_moveToCol == m_dragRowOrCol)
                          || (GetColPos( m_moveToCol ) == GetColPos( m_dragRowOrCol ) + 1)
                          || (m_moveToCol < 0 && m_dragRowOrCol == GetColAt( m_numCols - 1 )))
                        {
                            m_dragLastPos = -1;
                            return;
                        }

                        //Draw the marker
                        wxPen pen( *wxBLUE, 2 );
                        dc.SetPen(pen);

                        dc.DrawLine( markerX, 0, markerX, ch );

                        dc.SetPen(wxNullPen);

                        m_dragLastPos = markerX - 1;
                    }
                }
                break;

                // default label to suppress warnings about "enumeration value
                // 'xxx' not handled in switch
                default:
                    break;
            }
        }
        return;
    }

    if ( m_isDragging && (event.Entering() || event.Leaving()) )
        return;

    if (m_isDragging)
    {
        if (m_colLabelWin->HasCapture())
            m_colLabelWin->ReleaseMouse();
        m_isDragging = false;
    }

    // ------------ Entering or leaving the window
    //
    if ( event.Entering() || event.Leaving() )
    {
        ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL, m_colLabelWin);
    }

    // ------------ Left button pressed
    //
    else if ( event.LeftDown() )
    {
        // don't send a label click event for a hit on the
        // edge of the col label - this is probably the user
        // wanting to resize the col
        //
        if ( XToEdgeOfCol(x) < 0 )
        {
            col = XToCol(x);
            if ( col >= 0 &&
                 !SendEvent( wxEVT_GRID_LABEL_LEFT_CLICK, -1, col, event ) )
            {
                if ( m_canDragColMove )
                {
                    //Show button as pressed
                    wxClientDC dc( m_colLabelWin );
                    int colLeft = GetColLeft( col );
                    int colRight = GetColRight( col ) - 1;
                    dc.SetPen( wxPen( m_colLabelWin->GetBackgroundColour(), 1 ) );
                    dc.DrawLine( colLeft, 1, colLeft, m_colLabelHeight-1 );
                    dc.DrawLine( colLeft, 1, colRight, 1 );

                    ChangeCursorMode(WXGRID_CURSOR_MOVE_COL, m_colLabelWin);
                }
                else
                {
                    if ( !event.ShiftDown() && !event.CmdDown() )
                        ClearSelection();
                    if ( m_selection )
                    {
                        if ( event.ShiftDown() )
                        {
                            m_selection->SelectBlock( 0,
                                                      m_currentCellCoords.GetCol(),
                                                      GetNumberRows() - 1, col,
                                                      event.ControlDown(),
                                                      event.ShiftDown(),
                                                      event.AltDown(),
                                                      event.MetaDown() );
                        }
                        else
                        {
                            m_selection->SelectCol( col,
                                                    event.ControlDown(),
                                                    event.ShiftDown(),
                                                    event.AltDown(),
                                                    event.MetaDown() );
                        }
                    }

                    ChangeCursorMode(WXGRID_CURSOR_SELECT_COL, m_colLabelWin);
                }
            }
        }
        else
        {
            // starting to drag-resize a col
            //
            if ( CanDragColSize() )
                ChangeCursorMode(WXGRID_CURSOR_RESIZE_COL, m_colLabelWin);
        }
    }

    // ------------ Left double click
    //
    if ( event.LeftDClick() )
    {
        col = XToEdgeOfCol(x);
        if ( col < 0 )
        {
            col = XToCol(x);
            if ( col >= 0 &&
                 ! SendEvent( wxEVT_GRID_LABEL_LEFT_DCLICK, -1, col, event ) )
            {
                // no default action at the moment
            }
        }
        else
        {
            // adjust column width depending on label text
            AutoSizeColLabelSize( col );

            ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL, m_colLabelWin);
            m_dragLastPos = -1;
        }
    }

    // ------------ Left button released
    //
    else if ( event.LeftUp() )
    {
        switch ( m_cursorMode )
        {
            case WXGRID_CURSOR_RESIZE_COL:
                DoEndDragResizeCol();

                // Note: we are ending the event *after* doing
                // default processing in this case
                //
                SendEvent( wxEVT_GRID_COL_SIZE, -1, m_dragRowOrCol, event );
                break;

            case WXGRID_CURSOR_MOVE_COL:
                DoEndDragMoveCol();

                SendEvent( wxEVT_GRID_COL_MOVE, -1, m_dragRowOrCol, event );
                break;

            case WXGRID_CURSOR_SELECT_COL:
            case WXGRID_CURSOR_SELECT_CELL:
            case WXGRID_CURSOR_RESIZE_ROW:
            case WXGRID_CURSOR_SELECT_ROW:
                // nothing to do (?)
                break;
        }

        ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL, m_colLabelWin);
        m_dragLastPos = -1;
    }

    // ------------ Right button down
    //
    else if ( event.RightDown() )
    {
        col = XToCol(x);
        if ( col >= 0 &&
             !SendEvent( wxEVT_GRID_LABEL_RIGHT_CLICK, -1, col, event ) )
        {
            // no default action at the moment
        }
    }

    // ------------ Right double click
    //
    else if ( event.RightDClick() )
    {
        col = XToCol(x);
        if ( col >= 0 &&
             !SendEvent( wxEVT_GRID_LABEL_RIGHT_DCLICK, -1, col, event ) )
        {
            // no default action at the moment
        }
    }

    // ------------ No buttons down and mouse moving
    //
    else if ( event.Moving() )
    {
        m_dragRowOrCol = XToEdgeOfCol( x );
        if ( m_dragRowOrCol >= 0 )
        {
            if ( m_cursorMode == WXGRID_CURSOR_SELECT_CELL )
            {
                // don't capture the cursor yet
                if ( CanDragColSize() )
                    ChangeCursorMode(WXGRID_CURSOR_RESIZE_COL, m_colLabelWin, false);
            }
        }
        else if ( m_cursorMode != WXGRID_CURSOR_SELECT_CELL )
        {
            ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL, m_colLabelWin, false);
        }
    }
}

void wxGrid::ProcessCornerLabelMouseEvent( wxMouseEvent& event )
{
    if ( event.LeftDown() )
    {
        // indicate corner label by having both row and
        // col args == -1
        //
        if ( !SendEvent( wxEVT_GRID_LABEL_LEFT_CLICK, -1, -1, event ) )
        {
            SelectAll();
        }
    }
    else if ( event.LeftDClick() )
    {
        SendEvent( wxEVT_GRID_LABEL_LEFT_DCLICK, -1, -1, event );
    }
    else if ( event.RightDown() )
    {
        if ( !SendEvent( wxEVT_GRID_LABEL_RIGHT_CLICK, -1, -1, event ) )
        {
            // no default action at the moment
        }
    }
    else if ( event.RightDClick() )
    {
        if ( !SendEvent( wxEVT_GRID_LABEL_RIGHT_DCLICK, -1, -1, event ) )
        {
            // no default action at the moment
        }
    }
}

void wxGrid::ChangeCursorMode(CursorMode mode,
                              wxWindow *win,
                              bool captureMouse)
{
#ifdef __WXDEBUG__
    static const wxChar *cursorModes[] =
    {
        _T("SELECT_CELL"),
        _T("RESIZE_ROW"),
        _T("RESIZE_COL"),
        _T("SELECT_ROW"),
        _T("SELECT_COL"),
        _T("MOVE_COL"),
    };

    wxLogTrace(_T("grid"),
               _T("wxGrid cursor mode (mouse capture for %s): %s -> %s"),
               win == m_colLabelWin ? _T("colLabelWin")
                                    : win ? _T("rowLabelWin")
                                          : _T("gridWin"),
               cursorModes[m_cursorMode], cursorModes[mode]);
#endif

    if ( mode == m_cursorMode &&
         win == m_winCapture &&
         captureMouse == (m_winCapture != NULL))
        return;

    if ( !win )
    {
        // by default use the grid itself
        win = m_gridWin;
    }

    if ( m_winCapture )
    {
        if (m_winCapture->HasCapture())
            m_winCapture->ReleaseMouse();
        m_winCapture = (wxWindow *)NULL;
    }

    m_cursorMode = mode;

    switch ( m_cursorMode )
    {
        case WXGRID_CURSOR_RESIZE_ROW:
            win->SetCursor( m_rowResizeCursor );
            break;

        case WXGRID_CURSOR_RESIZE_COL:
            win->SetCursor( m_colResizeCursor );
            break;

        case WXGRID_CURSOR_MOVE_COL:
            win->SetCursor( wxCursor(wxCURSOR_HAND) );
            break;

        default:
            win->SetCursor( *wxSTANDARD_CURSOR );
            break;
    }

    // we need to capture mouse when resizing
    bool resize = m_cursorMode == WXGRID_CURSOR_RESIZE_ROW ||
                  m_cursorMode == WXGRID_CURSOR_RESIZE_COL;

    if ( captureMouse && resize )
    {
        win->CaptureMouse();
        m_winCapture = win;
    }
}

void wxGrid::ProcessGridCellMouseEvent( wxMouseEvent& event )
{
    int x, y;
    wxPoint pos( event.GetPosition() );
    CalcUnscrolledPosition( pos.x, pos.y, &x, &y );

    wxGridCellCoords coords;
    XYToCell( x, y, coords );

    int cell_rows, cell_cols;
    bool isFirstDrag = !m_isDragging;
    GetCellSize( coords.GetRow(), coords.GetCol(), &cell_rows, &cell_cols );
    if ((cell_rows < 0) || (cell_cols < 0))
    {
        coords.SetRow(coords.GetRow() + cell_rows);
        coords.SetCol(coords.GetCol() + cell_cols);
    }

    if ( event.Dragging() )
    {
        //wxLogDebug("pos(%d, %d) coords(%d, %d)", pos.x, pos.y, coords.GetRow(), coords.GetCol());

        // Don't start doing anything until the mouse has been dragged at
        // least 3 pixels in any direction...
        if (! m_isDragging)
        {
            if (m_startDragPos == wxDefaultPosition)
            {
                m_startDragPos = pos;
                return;
            }
            if (abs(m_startDragPos.x - pos.x) < 4 && abs(m_startDragPos.y - pos.y) < 4)
                return;
        }

        m_isDragging = true;
        if ( m_cursorMode == WXGRID_CURSOR_SELECT_CELL )
        {
            // Hide the edit control, so it
            // won't interfere with drag-shrinking.
            if ( IsCellEditControlShown() )
            {
                HideCellEditControl();
                SaveEditControlValue();
            }

            if ( coords != wxGridNoCellCoords )
            {
                if ( event.CmdDown() )
                {
                    if ( m_selectingKeyboard == wxGridNoCellCoords)
                        m_selectingKeyboard = coords;
                    HighlightBlock( m_selectingKeyboard, coords );
                }
                else if ( CanDragCell() )
                {
                    if ( isFirstDrag )
                    {
                        if ( m_selectingKeyboard == wxGridNoCellCoords)
                            m_selectingKeyboard = coords;

                        SendEvent( wxEVT_GRID_CELL_BEGIN_DRAG,
                                   coords.GetRow(),
                                   coords.GetCol(),
                                   event );
                        return;
                    }
                }
                else
                {
                    if ( !IsSelection() )
                    {
                        HighlightBlock( coords, coords );
                    }
                    else
                    {
                        HighlightBlock( m_currentCellCoords, coords );
                    }
                }

                if (! IsVisible(coords))
                {
                    MakeCellVisible(coords);
                    // TODO: need to introduce a delay or something here.  The
                    // scrolling is way to fast, at least on MSW - also on GTK.
                }
            }
            // Have we captured the mouse yet?
            if (! m_winCapture)
            {
                m_winCapture = m_gridWin;
                m_winCapture->CaptureMouse();
            }


        }
        else if ( event.LeftIsDown() &&
                  m_cursorMode == WXGRID_CURSOR_RESIZE_ROW )
        {
            int cw, ch, left, dummy;
            m_gridWin->GetClientSize( &cw, &ch );
            CalcUnscrolledPosition( 0, 0, &left, &dummy );

            wxClientDC dc( m_gridWin );
            PrepareDC( dc );
            y = wxMax( y, GetRowTop(m_dragRowOrCol) +
                          GetRowMinimalHeight(m_dragRowOrCol) );
            dc.SetLogicalFunction(wxINVERT);
            if ( m_dragLastPos >= 0 )
            {
                dc.DrawLine( left, m_dragLastPos, left+cw, m_dragLastPos );
            }
            dc.DrawLine( left, y, left+cw, y );
            m_dragLastPos = y;
        }
        else if ( event.LeftIsDown() &&
                  m_cursorMode == WXGRID_CURSOR_RESIZE_COL )
        {
            int cw, ch, dummy, top;
            m_gridWin->GetClientSize( &cw, &ch );
            CalcUnscrolledPosition( 0, 0, &dummy, &top );

            wxClientDC dc( m_gridWin );
            PrepareDC( dc );
            x = wxMax( x, GetColLeft(m_dragRowOrCol) +
                          GetColMinimalWidth(m_dragRowOrCol) );
            dc.SetLogicalFunction(wxINVERT);
            if ( m_dragLastPos >= 0 )
            {
                dc.DrawLine( m_dragLastPos, top, m_dragLastPos, top + ch );
            }
            dc.DrawLine( x, top, x, top + ch );
            m_dragLastPos = x;
        }

        return;
    }

    m_isDragging = false;
    m_startDragPos = wxDefaultPosition;

    // VZ: if we do this, the mode is reset to WXGRID_CURSOR_SELECT_CELL
    //     immediately after it becomes WXGRID_CURSOR_RESIZE_ROW/COL under
    //     wxGTK
#if 0
    if ( event.Entering() || event.Leaving() )
    {
        ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL);
        m_gridWin->SetCursor( *wxSTANDARD_CURSOR );
    }
    else
#endif // 0

    // ------------ Left button pressed
    //
    if ( event.LeftDown() && coords != wxGridNoCellCoords )
    {
        if ( !SendEvent( wxEVT_GRID_CELL_LEFT_CLICK,
                       coords.GetRow(),
                       coords.GetCol(),
                       event ) )
        {
            if ( !event.CmdDown() )
                ClearSelection();
            if ( event.ShiftDown() )
            {
                if ( m_selection )
                {
                    m_selection->SelectBlock( m_currentCellCoords.GetRow(),
                                              m_currentCellCoords.GetCol(),
                                              coords.GetRow(),
                                              coords.GetCol(),
                                              event.ControlDown(),
                                              event.ShiftDown(),
                                              event.AltDown(),
                                              event.MetaDown() );
                }
            }
            else if ( XToEdgeOfCol(x) < 0 &&
                      YToEdgeOfRow(y) < 0 )
            {
                DisableCellEditControl();
                MakeCellVisible( coords );

                if ( event.CmdDown() )
                {
                    if ( m_selection )
                    {
                        m_selection->ToggleCellSelection( coords.GetRow(),
                                                          coords.GetCol(),
                                                          event.ControlDown(),
                                                          event.ShiftDown(),
                                                          event.AltDown(),
                                                          event.MetaDown() );
                    }
                    m_selectingTopLeft = wxGridNoCellCoords;
                    m_selectingBottomRight = wxGridNoCellCoords;
                    m_selectingKeyboard = coords;
                }
                else
                {
                    m_waitForSlowClick = m_currentCellCoords == coords && coords != wxGridNoCellCoords;
                    SetCurrentCell( coords );
                    if ( m_selection )
                    {
                        if ( m_selection->GetSelectionMode() !=
                                wxGrid::wxGridSelectCells )
                        {
                            HighlightBlock( coords, coords );
                        }
                    }
                }
            }
        }
    }

    // ------------ Left double click
    //
    else if ( event.LeftDClick() && coords != wxGridNoCellCoords )
    {
        DisableCellEditControl();

        if ( XToEdgeOfCol(x) < 0 && YToEdgeOfRow(y) < 0 )
        {
            if ( !SendEvent( wxEVT_GRID_CELL_LEFT_DCLICK,
                             coords.GetRow(),
                             coords.GetCol(),
                             event ) )
            {
                // we want double click to select a cell and start editing
                // (i.e. to behave in same way as sequence of two slow clicks):
                m_waitForSlowClick = true;
            }
        }
    }

    // ------------ Left button released
    //
    else if ( event.LeftUp() )
    {
        if ( m_cursorMode == WXGRID_CURSOR_SELECT_CELL )
        {
            if (m_winCapture)
            {
                if (m_winCapture->HasCapture())
                    m_winCapture->ReleaseMouse();
                m_winCapture = NULL;
            }

            if ( coords == m_currentCellCoords && m_waitForSlowClick && CanEnableCellControl() )
            {
                ClearSelection();
                EnableCellEditControl();

                wxGridCellAttr *attr = GetCellAttr(coords);
                wxGridCellEditor *editor = attr->GetEditor(this, coords.GetRow(), coords.GetCol());
                editor->StartingClick();
                editor->DecRef();
                attr->DecRef();

                m_waitForSlowClick = false;
            }
            else if ( m_selectingTopLeft != wxGridNoCellCoords &&
                 m_selectingBottomRight != wxGridNoCellCoords )
            {
                if ( m_selection )
                {
                    m_selection->SelectBlock( m_selectingTopLeft.GetRow(),
                                              m_selectingTopLeft.GetCol(),
                                              m_selectingBottomRight.GetRow(),
                                              m_selectingBottomRight.GetCol(),
                                              event.ControlDown(),
                                              event.ShiftDown(),
                                              event.AltDown(),
                                              event.MetaDown() );
                }

                m_selectingTopLeft = wxGridNoCellCoords;
                m_selectingBottomRight = wxGridNoCellCoords;

                // Show the edit control, if it has been hidden for
                // drag-shrinking.
                ShowCellEditControl();
            }
        }
        else if ( m_cursorMode == WXGRID_CURSOR_RESIZE_ROW )
        {
            ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL);
            DoEndDragResizeRow();

            // Note: we are ending the event *after* doing
            // default processing in this case
            //
            SendEvent( wxEVT_GRID_ROW_SIZE, m_dragRowOrCol, -1, event );
        }
        else if ( m_cursorMode == WXGRID_CURSOR_RESIZE_COL )
        {
            ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL);
            DoEndDragResizeCol();

            // Note: we are ending the event *after* doing
            // default processing in this case
            //
            SendEvent( wxEVT_GRID_COL_SIZE, -1, m_dragRowOrCol, event );
        }

        m_dragLastPos = -1;
    }

    // ------------ Right button down
    //
    else if ( event.RightDown() && coords != wxGridNoCellCoords )
    {
        DisableCellEditControl();
        if ( !SendEvent( wxEVT_GRID_CELL_RIGHT_CLICK,
                         coords.GetRow(),
                         coords.GetCol(),
                         event ) )
        {
            // no default action at the moment
        }
    }

    // ------------ Right double click
    //
    else if ( event.RightDClick() && coords != wxGridNoCellCoords )
    {
        DisableCellEditControl();
        if ( !SendEvent( wxEVT_GRID_CELL_RIGHT_DCLICK,
                         coords.GetRow(),
                         coords.GetCol(),
                         event ) )
        {
            // no default action at the moment
        }
    }

    // ------------ Moving and no button action
    //
    else if ( event.Moving() && !event.IsButton() )
    {
        if ( coords.GetRow() < 0 || coords.GetCol() < 0 )
        {
            // out of grid cell area
            ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL);
            return;
        }

        int dragRow = YToEdgeOfRow( y );
        int dragCol = XToEdgeOfCol( x );

        // Dragging on the corner of a cell to resize in both
        // directions is not implemented yet...
        //
        if ( dragRow >= 0 && dragCol >= 0 )
        {
            ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL);
            return;
        }

        if ( dragRow >= 0 )
        {
            m_dragRowOrCol = dragRow;

            if ( m_cursorMode == WXGRID_CURSOR_SELECT_CELL )
            {
                if ( CanDragRowSize() && CanDragGridSize() )
                    ChangeCursorMode(WXGRID_CURSOR_RESIZE_ROW, NULL, false);
            }
        }
        else if ( dragCol >= 0 )
        {
            m_dragRowOrCol = dragCol;

            if ( m_cursorMode == WXGRID_CURSOR_SELECT_CELL )
            {
                if ( CanDragColSize() && CanDragGridSize() )
                    ChangeCursorMode(WXGRID_CURSOR_RESIZE_COL, NULL, false);
            }
        }
        else // Neither on a row or col edge
        {
            if ( m_cursorMode != WXGRID_CURSOR_SELECT_CELL )
            {
                ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL);
            }
        }
    }
}

void wxGrid::DoEndDragResizeRow()
{
    if ( m_dragLastPos >= 0 )
    {
        // erase the last line and resize the row
        //
        int cw, ch, left, dummy;
        m_gridWin->GetClientSize( &cw, &ch );
        CalcUnscrolledPosition( 0, 0, &left, &dummy );

        wxClientDC dc( m_gridWin );
        PrepareDC( dc );
        dc.SetLogicalFunction( wxINVERT );
        dc.DrawLine( left, m_dragLastPos, left + cw, m_dragLastPos );
        HideCellEditControl();
        SaveEditControlValue();

        int rowTop = GetRowTop(m_dragRowOrCol);
        SetRowSize( m_dragRowOrCol,
                    wxMax( m_dragLastPos - rowTop, m_minAcceptableRowHeight ) );

        if ( !GetBatchCount() )
        {
            // Only needed to get the correct rect.y:
            wxRect rect ( CellToRect( m_dragRowOrCol, 0 ) );
            rect.x = 0;
            CalcScrolledPosition(0, rect.y, &dummy, &rect.y);
            rect.width = m_rowLabelWidth;
            rect.height = ch - rect.y;
            m_rowLabelWin->Refresh( true, &rect );
            rect.width = cw;

            // if there is a multicell block, paint all of it
            if (m_table)
            {
                int i, cell_rows, cell_cols, subtract_rows = 0;
                int leftCol = XToCol(left);
                int rightCol = internalXToCol(left + cw);
                if (leftCol >= 0)
                {
                    for (i=leftCol; i<rightCol; i++)
                    {
                        GetCellSize(m_dragRowOrCol, i, &cell_rows, &cell_cols);
                        if (cell_rows < subtract_rows)
                            subtract_rows = cell_rows;
                    }
                    rect.y = GetRowTop(m_dragRowOrCol + subtract_rows);
                    CalcScrolledPosition(0, rect.y, &dummy, &rect.y);
                    rect.height = ch - rect.y;
                }
            }
            m_gridWin->Refresh( false, &rect );
        }

        ShowCellEditControl();
    }
}


void wxGrid::DoEndDragResizeCol()
{
    if ( m_dragLastPos >= 0 )
    {
        // erase the last line and resize the col
        //
        int cw, ch, dummy, top;
        m_gridWin->GetClientSize( &cw, &ch );
        CalcUnscrolledPosition( 0, 0, &dummy, &top );

        wxClientDC dc( m_gridWin );
        PrepareDC( dc );
        dc.SetLogicalFunction( wxINVERT );
        dc.DrawLine( m_dragLastPos, top, m_dragLastPos, top + ch );
        HideCellEditControl();
        SaveEditControlValue();

        int colLeft = GetColLeft(m_dragRowOrCol);
        SetColSize( m_dragRowOrCol,
                    wxMax( m_dragLastPos - colLeft,
                           GetColMinimalWidth(m_dragRowOrCol) ) );

        if ( !GetBatchCount() )
        {
            // Only needed to get the correct rect.x:
            wxRect rect ( CellToRect( 0, m_dragRowOrCol ) );
            rect.y = 0;
            CalcScrolledPosition(rect.x, 0, &rect.x, &dummy);
            rect.width = cw - rect.x;
            rect.height = m_colLabelHeight;
            m_colLabelWin->Refresh( true, &rect );
            rect.height = ch;

            // if there is a multicell block, paint all of it
            if (m_table)
            {
                int i, cell_rows, cell_cols, subtract_cols = 0;
                int topRow = YToRow(top);
                int bottomRow = internalYToRow(top + cw);
                if (topRow >= 0)
                {
                    for (i=topRow; i<bottomRow; i++)
                    {
                        GetCellSize(i, m_dragRowOrCol, &cell_rows, &cell_cols);
                        if (cell_cols < subtract_cols)
                            subtract_cols = cell_cols;
                    }

                    rect.x = GetColLeft(m_dragRowOrCol + subtract_cols);
                    CalcScrolledPosition(rect.x, 0, &rect.x, &dummy);
                    rect.width = cw - rect.x;
                }
            }

            m_gridWin->Refresh( false, &rect );
        }

        ShowCellEditControl();
    }
}

void wxGrid::DoEndDragMoveCol()
{
    //The user clicked on the column but didn't actually drag
    if ( m_dragLastPos < 0 )
    {
        m_colLabelWin->Refresh();   //Do this to "unpress" the column
        return;
    }

    int newPos;
    if ( m_moveToCol == -1 )
        newPos = m_numCols - 1;
    else
    {
        newPos = GetColPos( m_moveToCol );
        if ( newPos > GetColPos( m_dragRowOrCol ) )
            newPos--;
    }

    SetColPos( m_dragRowOrCol, newPos );
}

void wxGrid::SetColPos( int colID, int newPos )
{
    if ( m_colAt.IsEmpty() )
    {
        m_colAt.Alloc( m_numCols );

        int i;
        for ( i = 0; i < m_numCols; i++ )
        {
            m_colAt.Add( i );
        }
    }

    int oldPos = GetColPos( colID );

    //Reshuffle the m_colAt array
    if ( newPos > oldPos )
    {
        int i;
        for ( i = oldPos; i < newPos; i++ )
        {
            m_colAt[i] = m_colAt[i+1];
        }
    }
    else
    {
        int i;
        for ( i = oldPos; i > newPos; i-- )
        {
            m_colAt[i] = m_colAt[i-1];
        }
    }

    m_colAt[newPos] = colID;

    //Recalculate the column rights
    if ( !m_colWidths.IsEmpty() )
    {
        int colRight = 0;
        int colPos;
        for ( colPos = 0; colPos < m_numCols; colPos++ )
        {
            int colID = GetColAt( colPos );

            colRight += m_colWidths[colID];
            m_colRights[colID] = colRight;
        }
    }

    m_colLabelWin->Refresh();
    m_gridWin->Refresh();
}



void wxGrid::EnableDragColMove( bool enable )
{
    if ( m_canDragColMove == enable )
        return;

    m_canDragColMove = enable;

    if ( !m_canDragColMove )
    {
        m_colAt.Clear();

        //Recalculate the column rights
        if ( !m_colWidths.IsEmpty() )
        {
            int colRight = 0;
            int colPos;
            for ( colPos = 0; colPos < m_numCols; colPos++ )
            {
                colRight += m_colWidths[colPos];
                m_colRights[colPos] = colRight;
            }
        }

        m_colLabelWin->Refresh();
        m_gridWin->Refresh();
    }
}


//
// ------ interaction with data model
//
bool wxGrid::ProcessTableMessage( wxGridTableMessage& msg )
{
    switch ( msg.GetId() )
    {
        case wxGRIDTABLE_REQUEST_VIEW_GET_VALUES:
            return GetModelValues();

        case wxGRIDTABLE_REQUEST_VIEW_SEND_VALUES:
            return SetModelValues();

        case wxGRIDTABLE_NOTIFY_ROWS_INSERTED:
        case wxGRIDTABLE_NOTIFY_ROWS_APPENDED:
        case wxGRIDTABLE_NOTIFY_ROWS_DELETED:
        case wxGRIDTABLE_NOTIFY_COLS_INSERTED:
        case wxGRIDTABLE_NOTIFY_COLS_APPENDED:
        case wxGRIDTABLE_NOTIFY_COLS_DELETED:
            return Redimension( msg );

        default:
            return false;
    }
}

// The behaviour of this function depends on the grid table class
// Clear() function. For the default wxGridStringTable class the
// behavious is to replace all cell contents with wxEmptyString but
// not to change the number of rows or cols.
//
void wxGrid::ClearGrid()
{
    if ( m_table )
    {
        if (IsCellEditControlEnabled())
            DisableCellEditControl();

        m_table->Clear();
        if (!GetBatchCount())
            m_gridWin->Refresh();
    }
}

bool wxGrid::InsertRows( int pos, int numRows, bool WXUNUSED(updateLabels) )
{
    // TODO: something with updateLabels flag

    if ( !m_created )
    {
        wxFAIL_MSG( wxT("Called wxGrid::InsertRows() before calling CreateGrid()") );
        return false;
    }

    if ( m_table )
    {
        if (IsCellEditControlEnabled())
            DisableCellEditControl();

        bool done = m_table->InsertRows( pos, numRows );
        return done;

        // the table will have sent the results of the insert row
        // operation to this view object as a grid table message
    }

    return false;
}

bool wxGrid::AppendRows( int numRows, bool WXUNUSED(updateLabels) )
{
    // TODO: something with updateLabels flag

    if ( !m_created )
    {
        wxFAIL_MSG( wxT("Called wxGrid::AppendRows() before calling CreateGrid()") );
        return false;
    }

    if ( m_table )
    {
        bool done = m_table && m_table->AppendRows( numRows );
        return done;

        // the table will have sent the results of the append row
        // operation to this view object as a grid table message
    }

    return false;
}

bool wxGrid::DeleteRows( int pos, int numRows, bool WXUNUSED(updateLabels) )
{
    // TODO: something with updateLabels flag

    if ( !m_created )
    {
        wxFAIL_MSG( wxT("Called wxGrid::DeleteRows() before calling CreateGrid()") );
        return false;
    }

    if ( m_table )
    {
        if (IsCellEditControlEnabled())
            DisableCellEditControl();

        bool done = m_table->DeleteRows( pos, numRows );
        return done;
        // the table will have sent the results of the delete row
        // operation to this view object as a grid table message
    }

    return false;
}

bool wxGrid::InsertCols( int pos, int numCols, bool WXUNUSED(updateLabels) )
{
    // TODO: something with updateLabels flag

    if ( !m_created )
    {
        wxFAIL_MSG( wxT("Called wxGrid::InsertCols() before calling CreateGrid()") );
        return false;
    }

    if ( m_table )
    {
        if (IsCellEditControlEnabled())
            DisableCellEditControl();

        bool done = m_table->InsertCols( pos, numCols );
        return done;
        // the table will have sent the results of the insert col
        // operation to this view object as a grid table message
    }

    return false;
}

bool wxGrid::AppendCols( int numCols, bool WXUNUSED(updateLabels) )
{
    // TODO: something with updateLabels flag

    if ( !m_created )
    {
        wxFAIL_MSG( wxT("Called wxGrid::AppendCols() before calling CreateGrid()") );
        return false;
    }

    if ( m_table )
    {
        bool done = m_table->AppendCols( numCols );
        return done;
        // the table will have sent the results of the append col
        // operation to this view object as a grid table message
    }

    return false;
}

bool wxGrid::DeleteCols( int pos, int numCols, bool WXUNUSED(updateLabels) )
{
    // TODO: something with updateLabels flag

    if ( !m_created )
    {
        wxFAIL_MSG( wxT("Called wxGrid::DeleteCols() before calling CreateGrid()") );
        return false;
    }

    if ( m_table )
    {
        if (IsCellEditControlEnabled())
            DisableCellEditControl();

        bool done = m_table->DeleteCols( pos, numCols );
        return done;
        // the table will have sent the results of the delete col
        // operation to this view object as a grid table message
    }

    return false;
}

//
// ----- event handlers
//

// Generate a grid event based on a mouse event and
// return the result of ProcessEvent()
//
int wxGrid::SendEvent( const wxEventType type,
                        int row, int col,
                        wxMouseEvent& mouseEv )
{
   bool claimed, vetoed;

   if ( type == wxEVT_GRID_ROW_SIZE || type == wxEVT_GRID_COL_SIZE )
   {
       int rowOrCol = (row == -1 ? col : row);

       wxGridSizeEvent gridEvt( GetId(),
               type,
               this,
               rowOrCol,
               mouseEv.GetX() + GetRowLabelSize(),
               mouseEv.GetY() + GetColLabelSize(),
               mouseEv.ControlDown(),
               mouseEv.ShiftDown(),
               mouseEv.AltDown(),
               mouseEv.MetaDown() );

       claimed = GetEventHandler()->ProcessEvent(gridEvt);
       vetoed = !gridEvt.IsAllowed();
   }
   else if ( type == wxEVT_GRID_RANGE_SELECT )
   {
       // Right now, it should _never_ end up here!
       wxGridRangeSelectEvent gridEvt( GetId(),
               type,
               this,
               m_selectingTopLeft,
               m_selectingBottomRight,
               true,
               mouseEv.ControlDown(),
               mouseEv.ShiftDown(),
               mouseEv.AltDown(),
               mouseEv.MetaDown() );

       claimed = GetEventHandler()->ProcessEvent(gridEvt);
       vetoed = !gridEvt.IsAllowed();
   }
   else if ( type == wxEVT_GRID_LABEL_LEFT_CLICK ||
             type == wxEVT_GRID_LABEL_LEFT_DCLICK ||
             type == wxEVT_GRID_LABEL_RIGHT_CLICK ||
             type == wxEVT_GRID_LABEL_RIGHT_DCLICK )
   {
       wxPoint pos = mouseEv.GetPosition();

       if ( mouseEv.GetEventObject() == GetGridRowLabelWindow() )
           pos.y += GetColLabelSize();
       if ( mouseEv.GetEventObject() == GetGridColLabelWindow() )
           pos.x += GetRowLabelSize();

       wxGridEvent gridEvt( GetId(),
               type,
               this,
               row, col,
               pos.x,
               pos.y,
               false,
               mouseEv.ControlDown(),
               mouseEv.ShiftDown(),
               mouseEv.AltDown(),
               mouseEv.MetaDown() );
       claimed = GetEventHandler()->ProcessEvent(gridEvt);
       vetoed = !gridEvt.IsAllowed();
   }
   else
   {
       wxGridEvent gridEvt( GetId(),
               type,
               this,
               row, col,
               mouseEv.GetX() + GetRowLabelSize(),
               mouseEv.GetY() + GetColLabelSize(),
               false,
               mouseEv.ControlDown(),
               mouseEv.ShiftDown(),
               mouseEv.AltDown(),
               mouseEv.MetaDown() );
       claimed = GetEventHandler()->ProcessEvent(gridEvt);
       vetoed = !gridEvt.IsAllowed();
   }

   // A Veto'd event may not be `claimed' so test this first
   if (vetoed)
       return -1;

   return claimed ? 1 : 0;
}

// Generate a grid event of specified type and return the result
// of ProcessEvent().
//
int wxGrid::SendEvent( const wxEventType type,
                        int row, int col )
{
   bool claimed, vetoed;

    if ( type == wxEVT_GRID_ROW_SIZE || type == wxEVT_GRID_COL_SIZE )
    {
        int rowOrCol = (row == -1 ? col : row);

        wxGridSizeEvent gridEvt( GetId(), type, this, rowOrCol );

        claimed = GetEventHandler()->ProcessEvent(gridEvt);
        vetoed  = !gridEvt.IsAllowed();
    }
    else
    {
        wxGridEvent gridEvt( GetId(), type, this, row, col );

        claimed = GetEventHandler()->ProcessEvent(gridEvt);
        vetoed  = !gridEvt.IsAllowed();
     }

    // A Veto'd event may not be `claimed' so test this first
    if (vetoed)
        return -1;

    return claimed ? 1 : 0;
}

void wxGrid::OnPaint( wxPaintEvent& WXUNUSED(event) )
{
    // needed to prevent zillions of paint events on MSW
    wxPaintDC dc(this);
}

void wxGrid::Refresh(bool eraseb, const wxRect* rect)
{
    // Don't do anything if between Begin/EndBatch...
    // EndBatch() will do all this on the last nested one anyway.
    if (! GetBatchCount())
    {
        // Refresh to get correct scrolled position:
        wxScrolledWindow::Refresh(eraseb, rect);

        if (rect)
        {
            int rect_x, rect_y, rectWidth, rectHeight;
            int width_label, width_cell, height_label, height_cell;
            int x, y;

            // Copy rectangle can get scroll offsets..
            rect_x = rect->GetX();
            rect_y = rect->GetY();
            rectWidth = rect->GetWidth();
            rectHeight = rect->GetHeight();

            width_label = m_rowLabelWidth - rect_x;
            if (width_label > rectWidth)
                width_label = rectWidth;

            height_label = m_colLabelHeight - rect_y;
            if (height_label > rectHeight)
                height_label = rectHeight;

            if (rect_x > m_rowLabelWidth)
            {
                x = rect_x - m_rowLabelWidth;
                width_cell = rectWidth;
            }
            else
            {
                x = 0;
                width_cell = rectWidth - (m_rowLabelWidth - rect_x);
            }

            if (rect_y > m_colLabelHeight)
            {
                y = rect_y - m_colLabelHeight;
                height_cell = rectHeight;
            }
            else
            {
                y = 0;
                height_cell = rectHeight - (m_colLabelHeight - rect_y);
            }

            // Paint corner label part intersecting rect.
            if ( width_label > 0 && height_label > 0 )
            {
                wxRect anotherrect(rect_x, rect_y, width_label, height_label);
                m_cornerLabelWin->Refresh(eraseb, &anotherrect);
            }

            // Paint col labels part intersecting rect.
            if ( width_cell > 0 && height_label > 0 )
            {
                wxRect anotherrect(x, rect_y, width_cell, height_label);
                m_colLabelWin->Refresh(eraseb, &anotherrect);
            }

            // Paint row labels part intersecting rect.
            if ( width_label > 0 && height_cell > 0 )
            {
                wxRect anotherrect(rect_x, y, width_label, height_cell);
                m_rowLabelWin->Refresh(eraseb, &anotherrect);
            }

            // Paint cell area part intersecting rect.
            if ( width_cell > 0 && height_cell > 0 )
            {
                wxRect anotherrect(x, y, width_cell, height_cell);
                m_gridWin->Refresh(eraseb, &anotherrect);
            }
        }
        else
        {
            m_cornerLabelWin->Refresh(eraseb, NULL);
            m_colLabelWin->Refresh(eraseb, NULL);
            m_rowLabelWin->Refresh(eraseb, NULL);
            m_gridWin->Refresh(eraseb, NULL);
        }
    }
}

void wxGrid::OnSize( wxSizeEvent& WXUNUSED(event) )
{
    if (m_targetWindow != this) // check whether initialisation has been done
    {
        // update our children window positions and scrollbars
        CalcDimensions();
    }
}

void wxGrid::OnKeyDown( wxKeyEvent& event )
{
    if ( m_inOnKeyDown )
    {
        // shouldn't be here - we are going round in circles...
        //
        wxFAIL_MSG( wxT("wxGrid::OnKeyDown called while already active") );
    }

    m_inOnKeyDown = true;

    // propagate the event up and see if it gets processed
    wxWindow *parent = GetParent();
    wxKeyEvent keyEvt( event );
    keyEvt.SetEventObject( parent );

    if ( !parent->GetEventHandler()->ProcessEvent( keyEvt ) )
    {
        if (GetLayoutDirection() == wxLayout_RightToLeft)
        {
            if (event.GetKeyCode() == WXK_RIGHT)
                event.m_keyCode = WXK_LEFT;
            else if (event.GetKeyCode() == WXK_LEFT)
                event.m_keyCode = WXK_RIGHT;
        }

        // try local handlers
        switch ( event.GetKeyCode() )
        {
            case WXK_UP:
                if ( event.ControlDown() )
                    MoveCursorUpBlock( event.ShiftDown() );
                else
                    MoveCursorUp( event.ShiftDown() );
                break;

            case WXK_DOWN:
                if ( event.ControlDown() )
                    MoveCursorDownBlock( event.ShiftDown() );
                else
                    MoveCursorDown( event.ShiftDown() );
                break;

            case WXK_LEFT:
                if ( event.ControlDown() )
                    MoveCursorLeftBlock( event.ShiftDown() );
                else
                    MoveCursorLeft( event.ShiftDown() );
                break;

            case WXK_RIGHT:
                if ( event.ControlDown() )
                    MoveCursorRightBlock( event.ShiftDown() );
                else
                    MoveCursorRight( event.ShiftDown() );
                break;

            case WXK_RETURN:
            case WXK_NUMPAD_ENTER:
                if ( event.ControlDown() )
                {
                    event.Skip();  // to let the edit control have the return
                }
                else
                {
                    if ( GetGridCursorRow() < GetNumberRows()-1 )
                    {
                        MoveCursorDown( event.ShiftDown() );
                    }
                    else
                    {
                        // at the bottom of a column
                        DisableCellEditControl();
                    }
                }
                break;

            case WXK_ESCAPE:
                ClearSelection();
                break;

            case WXK_TAB:
                if (event.ShiftDown())
                {
                    if ( GetGridCursorCol() > 0 )
                    {
                        MoveCursorLeft( false );
                    }
                    else
                    {
                        // at left of grid
                        DisableCellEditControl();
                    }
                }
                else
                {
                    if ( GetGridCursorCol() < GetNumberCols() - 1 )
                    {
                        MoveCursorRight( false );
                    }
                    else
                    {
                        // at right of grid
                        DisableCellEditControl();
                    }
                }
                break;

            case WXK_HOME:
                if ( event.ControlDown() )
                {
                    MakeCellVisible( 0, 0 );
                    SetCurrentCell( 0, 0 );
                }
                else
                {
                    event.Skip();
                }
                break;

            case WXK_END:
                if ( event.ControlDown() )
                {
                    MakeCellVisible( m_numRows - 1, m_numCols - 1 );
                    SetCurrentCell( m_numRows - 1, m_numCols - 1 );
                }
                else
                {
                    event.Skip();
                }
                break;

            case WXK_PAGEUP:
                MovePageUp();
                break;

            case WXK_PAGEDOWN:
                MovePageDown();
                break;

            case WXK_SPACE:
                if ( event.ControlDown() )
                {
                    if ( m_selection )
                    {
                        m_selection->ToggleCellSelection(
                            m_currentCellCoords.GetRow(),
                            m_currentCellCoords.GetCol(),
                            event.ControlDown(),
                            event.ShiftDown(),
                            event.AltDown(),
                            event.MetaDown() );
                    }
                    break;
                }

                if ( !IsEditable() )
                    MoveCursorRight( false );
                else
                    event.Skip();
                break;

            default:
                event.Skip();
                break;
        }
    }

    m_inOnKeyDown = false;
}

void wxGrid::OnKeyUp( wxKeyEvent& event )
{
    // try local handlers
    //
    if ( event.GetKeyCode() == WXK_SHIFT )
    {
        if ( m_selectingTopLeft != wxGridNoCellCoords &&
             m_selectingBottomRight != wxGridNoCellCoords )
        {
            if ( m_selection )
            {
                m_selection->SelectBlock(
                    m_selectingTopLeft.GetRow(),
                    m_selectingTopLeft.GetCol(),
                    m_selectingBottomRight.GetRow(),
                    m_selectingBottomRight.GetCol(),
                    event.ControlDown(),
                    true,
                    event.AltDown(),
                    event.MetaDown() );
            }
        }

        m_selectingTopLeft = wxGridNoCellCoords;
        m_selectingBottomRight = wxGridNoCellCoords;
        m_selectingKeyboard = wxGridNoCellCoords;
    }
}

void wxGrid::OnChar( wxKeyEvent& event )
{
    // is it possible to edit the current cell at all?
    if ( !IsCellEditControlEnabled() && CanEnableCellControl() )
    {
        // yes, now check whether the cells editor accepts the key
        int row = m_currentCellCoords.GetRow();
        int col = m_currentCellCoords.GetCol();
        wxGridCellAttr *attr = GetCellAttr(row, col);
        wxGridCellEditor *editor = attr->GetEditor(this, row, col);

        // <F2> is special and will always start editing, for
        // other keys - ask the editor itself
        if ( (event.GetKeyCode() == WXK_F2 && !event.HasModifiers())
             || editor->IsAcceptedKey(event) )
        {
            // ensure cell is visble
            MakeCellVisible(row, col);
            EnableCellEditControl();

            // a problem can arise if the cell is not completely
            // visible (even after calling MakeCellVisible the
            // control is not created and calling StartingKey will
            // crash the app
            if ( event.GetKeyCode() != WXK_F2 && editor->IsCreated() && m_cellEditCtrlEnabled )
                editor->StartingKey(event);
        }
        else
        {
            event.Skip();
        }

        editor->DecRef();
        attr->DecRef();
    }
    else
    {
        event.Skip();
    }
}

void wxGrid::OnEraseBackground(wxEraseEvent&)
{
}

void wxGrid::SetCurrentCell( const wxGridCellCoords& coords )
{
    if ( SendEvent( wxEVT_GRID_SELECT_CELL, coords.GetRow(), coords.GetCol() ) )
    {
        // the event has been intercepted - do nothing
        return;
    }

#if !(defined(__WXMAC__) && wxMAC_USE_CORE_GRAPHICS)
    wxClientDC dc( m_gridWin );
    PrepareDC( dc );
#endif

    if ( m_currentCellCoords != wxGridNoCellCoords )
    {
        DisableCellEditControl();

        if ( IsVisible( m_currentCellCoords, false ) )
        {
            wxRect r;
            r = BlockToDeviceRect( m_currentCellCoords, m_currentCellCoords );
            if ( !m_gridLinesEnabled )
            {
                r.x--;
                r.y--;
                r.width++;
                r.height++;
            }

            wxGridCellCoordsArray cells = CalcCellsExposed( r );

            // Otherwise refresh redraws the highlight!
            m_currentCellCoords = coords;

#if defined(__WXMAC__) && wxMAC_USE_CORE_GRAPHICS
            m_gridWin->Refresh(true /*, & r */);
#else
            DrawGridCellArea( dc, cells );
            DrawAllGridLines( dc, r );
#endif
        }
    }

    m_currentCellCoords = coords;

    wxGridCellAttr *attr = GetCellAttr( coords );
#if !(defined(__WXMAC__) && wxMAC_USE_CORE_GRAPHICS)
    DrawCellHighlight( dc, attr );
#endif
    attr->DecRef();
}

void wxGrid::HighlightBlock( int topRow, int leftCol, int bottomRow, int rightCol )
{
    int temp;
    wxGridCellCoords updateTopLeft, updateBottomRight;

    if ( m_selection )
    {
        if ( m_selection->GetSelectionMode() == wxGrid::wxGridSelectRows )
        {
            leftCol = 0;
            rightCol = GetNumberCols() - 1;
        }
        else if ( m_selection->GetSelectionMode() == wxGrid::wxGridSelectColumns )
        {
            topRow = 0;
            bottomRow = GetNumberRows() - 1;
        }
    }

    if ( topRow > bottomRow )
    {
        temp = topRow;
        topRow = bottomRow;
        bottomRow = temp;
    }

    if ( leftCol > rightCol )
    {
        temp = leftCol;
        leftCol = rightCol;
        rightCol = temp;
    }

    updateTopLeft = wxGridCellCoords( topRow, leftCol );
    updateBottomRight = wxGridCellCoords( bottomRow, rightCol );

    // First the case that we selected a completely new area
    if ( m_selectingTopLeft == wxGridNoCellCoords ||
         m_selectingBottomRight == wxGridNoCellCoords )
    {
        wxRect rect;
        rect = BlockToDeviceRect( wxGridCellCoords ( topRow, leftCol ),
                                  wxGridCellCoords ( bottomRow, rightCol ) );
        m_gridWin->Refresh( false, &rect );
    }

    // Now handle changing an existing selection area.
    else if ( m_selectingTopLeft != updateTopLeft ||
              m_selectingBottomRight != updateBottomRight )
    {
        // Compute two optimal update rectangles:
        // Either one rectangle is a real subset of the
        // other, or they are (almost) disjoint!
        wxRect  rect[4];
        bool    need_refresh[4];
        need_refresh[0] =
        need_refresh[1] =
        need_refresh[2] =
        need_refresh[3] = false;
        int     i;

        // Store intermediate values
        wxCoord oldLeft = m_selectingTopLeft.GetCol();
        wxCoord oldTop = m_selectingTopLeft.GetRow();
        wxCoord oldRight = m_selectingBottomRight.GetCol();
        wxCoord oldBottom = m_selectingBottomRight.GetRow();

        // Determine the outer/inner coordinates.
        if (oldLeft > leftCol)
        {
            temp = oldLeft;
            oldLeft = leftCol;
            leftCol = temp;
        }
        if (oldTop > topRow )
        {
            temp = oldTop;
            oldTop = topRow;
            topRow = temp;
        }
        if (oldRight < rightCol )
        {
            temp = oldRight;
            oldRight = rightCol;
            rightCol = temp;
        }
        if (oldBottom < bottomRow)
        {
            temp = oldBottom;
            oldBottom = bottomRow;
            bottomRow = temp;
        }

        // Now, either the stuff marked old is the outer
        // rectangle or we don't have a situation where one
        // is contained in the other.

        if ( oldLeft < leftCol )
        {
            // Refresh the newly selected or deselected
            // area to the left of the old or new selection.
            need_refresh[0] = true;
            rect[0] = BlockToDeviceRect(
                wxGridCellCoords( oldTop,  oldLeft ),
                wxGridCellCoords( oldBottom, leftCol - 1 ) );
        }

        if ( oldTop < topRow )
        {
            // Refresh the newly selected or deselected
            // area above the old or new selection.
            need_refresh[1] = true;
            rect[1] = BlockToDeviceRect(
                wxGridCellCoords( oldTop, leftCol ),
                wxGridCellCoords( topRow - 1, rightCol ) );
        }

        if ( oldRight > rightCol )
        {
            // Refresh the newly selected or deselected
            // area to the right of the old or new selection.
            need_refresh[2] = true;
            rect[2] = BlockToDeviceRect(
                wxGridCellCoords( oldTop, rightCol + 1 ),
                wxGridCellCoords( oldBottom, oldRight ) );
        }

        if ( oldBottom > bottomRow )
        {
            // Refresh the newly selected or deselected
            // area below the old or new selection.
            need_refresh[3] = true;
            rect[3] = BlockToDeviceRect(
                wxGridCellCoords( bottomRow + 1, leftCol ),
                wxGridCellCoords( oldBottom, rightCol ) );
        }

        // various Refresh() calls
        for (i = 0; i < 4; i++ )
            if ( need_refresh[i] && rect[i] != wxGridNoCellRect )
                m_gridWin->Refresh( false, &(rect[i]) );
    }

    // change selection
    m_selectingTopLeft = updateTopLeft;
    m_selectingBottomRight = updateBottomRight;
}

//
// ------ functions to get/send data (see also public functions)
//

bool wxGrid::GetModelValues()
{
    // Hide the editor, so it won't hide a changed value.
    HideCellEditControl();

    if ( m_table )
    {
        // all we need to do is repaint the grid
        //
        m_gridWin->Refresh();
        return true;
    }

    return false;
}

bool wxGrid::SetModelValues()
{
    int row, col;

    // Disable the editor, so it won't hide a changed value.
    // Do we also want to save the current value of the editor first?
    // I think so ...
    DisableCellEditControl();

    if ( m_table )
    {
        for ( row = 0; row < m_numRows; row++ )
        {
            for ( col = 0; col < m_numCols; col++ )
            {
                m_table->SetValue( row, col, GetCellValue(row, col) );
            }
        }

        return true;
    }

    return false;
}

// Note - this function only draws cells that are in the list of
// exposed cells (usually set from the update region by
// CalcExposedCells)
//
void wxGrid::DrawGridCellArea( wxDC& dc, const wxGridCellCoordsArray& cells )
{
    if ( !m_numRows || !m_numCols )
        return;

    int i, numCells = cells.GetCount();
    int row, col, cell_rows, cell_cols;
    wxGridCellCoordsArray redrawCells;

    for ( i = numCells - 1; i >= 0; i-- )
    {
        row = cells[i].GetRow();
        col = cells[i].GetCol();
        GetCellSize( row, col, &cell_rows, &cell_cols );

        // If this cell is part of a multicell block, find owner for repaint
        if ( cell_rows <= 0 || cell_cols <= 0 )
        {
            wxGridCellCoords cell( row + cell_rows, col + cell_cols );
            bool marked = false;
            for ( int j = 0; j < numCells; j++ )
            {
                if ( cell == cells[j] )
                {
                    marked = true;
                    break;
                }
            }

            if (!marked)
            {
                int count = redrawCells.GetCount();
                for (int j = 0; j < count; j++)
                {
                    if ( cell == redrawCells[j] )
                    {
                        marked = true;
                        break;
                    }
                }

                if (!marked)
                    redrawCells.Add( cell );
            }

            // don't bother drawing this cell
            continue;
        }

        // If this cell is empty, find cell to left that might want to overflow
        if (m_table && m_table->IsEmptyCell(row, col))
        {
            for ( int l = 0; l < cell_rows; l++ )
            {
                // find a cell in this row to leave already marked for repaint
                int left = col;
                for (int k = 0; k < int(redrawCells.GetCount()); k++)
                    if ((redrawCells[k].GetCol() < left) &&
                        (redrawCells[k].GetRow() == row))
                    {
                        left = redrawCells[k].GetCol();
                    }

                if (left == col)
                    left = 0; // oh well

                for (int j = col - 1; j >= left; j--)
                {
                    if (!m_table->IsEmptyCell(row + l, j))
                    {
                        if (GetCellOverflow(row + l, j))
                        {
                            wxGridCellCoords cell(row + l, j);
                            bool marked = false;

                            for (int k = 0; k < numCells; k++)
                            {
                                if ( cell == cells[k] )
                                {
                                    marked = true;
                                    break;
                                }
                            }

                            if (!marked)
                            {
                                int count = redrawCells.GetCount();
                                for (int k = 0; k < count; k++)
                                {
                                    if ( cell == redrawCells[k] )
                                    {
                                        marked = true;
                                        break;
                                    }
                                }
                                if (!marked)
                                    redrawCells.Add( cell );
                            }
                        }
                        break;
                    }
                }
            }
        }

        DrawCell( dc, cells[i] );
    }

    numCells = redrawCells.GetCount();

    for ( i = numCells - 1; i >= 0; i-- )
    {
        DrawCell( dc, redrawCells[i] );
    }
}

void wxGrid::DrawGridSpace( wxDC& dc )
{
  int cw, ch;
  m_gridWin->GetClientSize( &cw, &ch );

  int right, bottom;
  CalcUnscrolledPosition( cw, ch, &right, &bottom );

  int rightCol = m_numCols > 0 ? GetColRight(GetColAt( m_numCols - 1 )) : 0;
  int bottomRow = m_numRows > 0 ? GetRowBottom(m_numRows - 1) : 0;

  if ( right > rightCol || bottom > bottomRow )
  {
      int left, top;
      CalcUnscrolledPosition( 0, 0, &left, &top );

      dc.SetBrush( wxBrush(GetDefaultCellBackgroundColour(), wxSOLID) );
      dc.SetPen( *wxTRANSPARENT_PEN );

      if ( right > rightCol )
      {
          dc.DrawRectangle( rightCol, top, right - rightCol, ch );
      }

      if ( bottom > bottomRow )
      {
          dc.DrawRectangle( left, bottomRow, cw, bottom - bottomRow );
      }
  }
}

void wxGrid::DrawCell( wxDC& dc, const wxGridCellCoords& coords )
{
    int row = coords.GetRow();
    int col = coords.GetCol();

    if ( GetColWidth(col) <= 0 || GetRowHeight(row) <= 0 )
        return;

    // we draw the cell border ourselves
#if !WXGRID_DRAW_LINES
    if ( m_gridLinesEnabled )
        DrawCellBorder( dc, coords );
#endif

    wxGridCellAttr* attr = GetCellAttr(row, col);

    bool isCurrent = coords == m_currentCellCoords;

    wxRect rect = CellToRect( row, col );

    // if the editor is shown, we should use it and not the renderer
    // Note: However, only if it is really _shown_, i.e. not hidden!
    if ( isCurrent && IsCellEditControlShown() )
    {
        // NB: this "#if..." is temporary and fixes a problem where the
        // edit control is erased by this code after being rendered.
        // On wxMac (QD build only), the cell editor is a wxTextCntl and is rendered
        // implicitly, causing this out-of order render.
#if !defined(__WXMAC__)
        wxGridCellEditor *editor = attr->GetEditor(this, row, col);
        editor->PaintBackground(rect, attr);
        editor->DecRef();
#endif
    }
    else
    {
        // but all the rest is drawn by the cell renderer and hence may be customized
        wxGridCellRenderer *renderer = attr->GetRenderer(this, row, col);
        renderer->Draw(*this, *attr, dc, rect, row, col, IsInSelection(coords));
        renderer->DecRef();
    }

    attr->DecRef();
}

void wxGrid::DrawCellHighlight( wxDC& dc, const wxGridCellAttr *attr )
{
    // don't show highlight when the grid doesn't have focus
    if ( wxWindow::FindFocus() != m_gridWin )
        return;

    int row = m_currentCellCoords.GetRow();
    int col = m_currentCellCoords.GetCol();

    if ( GetColWidth(col) <= 0 || GetRowHeight(row) <= 0 )
        return;

    wxRect rect = CellToRect(row, col);

    // hmmm... what could we do here to show that the cell is disabled?
    // for now, I just draw a thinner border than for the other ones, but
    // it doesn't look really good

    int penWidth = attr->IsReadOnly() ? m_cellHighlightROPenWidth : m_cellHighlightPenWidth;

    if (penWidth > 0)
    {
        // The center of the drawn line is where the position/width/height of
        // the rectangle is actually at (on wxMSW at least), so the
        // size of the rectangle is reduced to compensate for the thickness of
        // the line. If this is too strange on non-wxMSW platforms then
        // please #ifdef this appropriately.
        rect.x += penWidth / 2;
        rect.y += penWidth / 2;
        rect.width -= penWidth - 1;
        rect.height -= penWidth - 1;

        // Now draw the rectangle
        // use the cellHighlightColour if the cell is inside a selection, this
        // will ensure the cell is always visible.
        dc.SetPen(wxPen(IsInSelection(row,col) ? m_selectionForeground : m_cellHighlightColour, penWidth, wxSOLID));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(rect);
    }

#if 0
        // VZ: my experiments with 3D borders...

        // how to properly set colours for arbitrary bg?
        wxCoord x1 = rect.x,
                y1 = rect.y,
                x2 = rect.x + rect.width - 1,
                y2 = rect.y + rect.height - 1;

        dc.SetPen(*wxWHITE_PEN);
        dc.DrawLine(x1, y1, x2, y1);
        dc.DrawLine(x1, y1, x1, y2);

        dc.DrawLine(x1 + 1, y2 - 1, x2 - 1, y2 - 1);
        dc.DrawLine(x2 - 1, y1 + 1, x2 - 1, y2);

        dc.SetPen(*wxBLACK_PEN);
        dc.DrawLine(x1, y2, x2, y2);
        dc.DrawLine(x2, y1, x2, y2 + 1);
#endif
}

wxPen wxGrid::GetDefaultGridLinePen()
{
    return wxPen(GetGridLineColour(), 1, wxSOLID);
}

wxPen wxGrid::GetRowGridLinePen(int WXUNUSED(row))
{
    return GetDefaultGridLinePen();
}

wxPen wxGrid::GetColGridLinePen(int WXUNUSED(col))
{
    return GetDefaultGridLinePen();
}

void wxGrid::DrawCellBorder( wxDC& dc, const wxGridCellCoords& coords )
{
    int row = coords.GetRow();
    int col = coords.GetCol();
    if ( GetColWidth(col) <= 0 || GetRowHeight(row) <= 0 )
        return;


    wxRect rect = CellToRect( row, col );

    // right hand border
    dc.SetPen( GetColGridLinePen(col) );
    dc.DrawLine( rect.x + rect.width, rect.y,
                 rect.x + rect.width, rect.y + rect.height + 1 );

    // bottom border
    dc.SetPen( GetRowGridLinePen(row) );
    dc.DrawLine( rect.x, rect.y + rect.height,
                 rect.x + rect.width, rect.y + rect.height);
}

void wxGrid::DrawHighlight(wxDC& dc, const wxGridCellCoordsArray& cells)
{
    // This if block was previously in wxGrid::OnPaint but that doesn't
    // seem to get called under wxGTK - MB
    //
    if ( m_currentCellCoords == wxGridNoCellCoords &&
         m_numRows && m_numCols )
    {
        m_currentCellCoords.Set(0, 0);
    }

    if ( IsCellEditControlShown() )
    {
        // don't show highlight when the edit control is shown
        return;
    }

    // if the active cell was repainted, repaint its highlight too because it
    // might have been damaged by the grid lines
    size_t count = cells.GetCount();
    for ( size_t n = 0; n < count; n++ )
    {
        wxGridCellCoords cell = cells[n];

        // If we are using attributes, then we may have just exposed another
        // cell in a partially-visible merged cluster of cells. If the "anchor"
        // (upper left) cell of this merged cluster is the cell indicated by
        // m_currentCellCoords, then we need to refresh the cell highlight even
        // though the "anchor" itself is not part of our update segment.
        if ( CanHaveAttributes() )
        {
            int rows = 0,
                cols = 0;
            GetCellSize(cell.GetRow(), cell.GetCol(), &rows, &cols);

            if ( rows < 0 )
                cell.SetRow(cell.GetRow() + rows);

            if ( cols < 0 )
                cell.SetCol(cell.GetCol() + cols);
        }

        if ( cell == m_currentCellCoords )
        {
            wxGridCellAttr* attr = GetCellAttr(m_currentCellCoords);
            DrawCellHighlight(dc, attr);
            attr->DecRef();

            break;
        }
    }
}

// TODO: remove this ???
// This is used to redraw all grid lines e.g. when the grid line colour
// has been changed
//
void wxGrid::DrawAllGridLines( wxDC& dc, const wxRegion & WXUNUSED(reg) )
{
#if !WXGRID_DRAW_LINES
    return;
#endif

    if ( !m_gridLinesEnabled || !m_numRows || !m_numCols )
         return;

    int top, bottom, left, right;

#if 0  //#ifndef __WXGTK__
    if (reg.IsEmpty())
    {
      int cw, ch;
      m_gridWin->GetClientSize(&cw, &ch);

      // virtual coords of visible area
      //
      CalcUnscrolledPosition( 0, 0, &left, &top );
      CalcUnscrolledPosition( cw, ch, &right, &bottom );
    }
    else
    {
      wxCoord x, y, w, h;
      reg.GetBox(x, y, w, h);
      CalcUnscrolledPosition( x, y, &left, &top );
      CalcUnscrolledPosition( x + w, y + h, &right, &bottom );
    }
#else
      int cw, ch;
      m_gridWin->GetClientSize(&cw, &ch);
      CalcUnscrolledPosition( 0, 0, &left, &top );
      CalcUnscrolledPosition( cw, ch, &right, &bottom );
#endif

    // avoid drawing grid lines past the last row and col
    //
    right = wxMin( right, GetColRight(GetColAt( m_numCols - 1 )) );
    bottom = wxMin( bottom, GetRowBottom(m_numRows - 1) );

    // no gridlines inside multicells, clip them out
    int leftCol = GetColPos( internalXToCol(left) );
    int topRow = internalYToRow(top);
    int rightCol = GetColPos( internalXToCol(right) );
    int bottomRow = internalYToRow(bottom);

#if !defined(__WXMAC__) || wxMAC_USE_CORE_GRAPHICS
    wxRegion clippedcells(0, 0, cw, ch);

    int i, j, cell_rows, cell_cols;
    wxRect rect;

    for (j=topRow; j<=bottomRow; j++)
    {
        int colPos;
        for (colPos=leftCol; colPos<=rightCol; colPos++)
        {
            i = GetColAt( colPos );

            GetCellSize( j, i, &cell_rows, &cell_cols );
            if ((cell_rows > 1) || (cell_cols > 1))
            {
                rect = CellToRect(j,i);
                CalcScrolledPosition( rect.x, rect.y, &rect.x, &rect.y );
                clippedcells.Subtract(rect);
            }
            else if ((cell_rows < 0) || (cell_cols < 0))
            {
                rect = CellToRect(j + cell_rows, i + cell_cols);
                CalcScrolledPosition( rect.x, rect.y, &rect.x, &rect.y );
                clippedcells.Subtract(rect);
            }
        }
    }
#else
    wxRegion clippedcells( left, top, right - left, bottom - top );

    int i, j, cell_rows, cell_cols;
    wxRect rect;

    for (j=topRow; j<=bottomRow; j++)
    {
        for (i=leftCol; i<=rightCol; i++)
        {
            GetCellSize( j, i, &cell_rows, &cell_cols );
            if ((cell_rows > 1) || (cell_cols > 1))
            {
                rect = CellToRect(j, i);
                clippedcells.Subtract(rect);
            }
            else if ((cell_rows < 0) || (cell_cols < 0))
            {
                rect = CellToRect(j + cell_rows, i + cell_cols);
                clippedcells.Subtract(rect);
            }
        }
    }
#endif

    dc.SetClippingRegion( clippedcells );


    // horizontal grid lines
    //
    // already declared above - int i;
    for ( i = internalYToRow(top); i < m_numRows; i++ )
    {
        int bot = GetRowBottom(i) - 1;

        if ( bot > bottom )
        {
            break;
        }

        if ( bot >= top )
        {
            dc.SetPen( GetRowGridLinePen(i) );
            dc.DrawLine( left, bot, right, bot );
        }
    }

    // vertical grid lines
    //
    int colPos;
    for ( colPos = leftCol; colPos < m_numCols; colPos++ )
    {
        i = GetColAt( colPos );

        int colRight = GetColRight(i);
#ifdef __WXGTK__
        if (GetLayoutDirection() != wxLayout_RightToLeft)
#endif
            colRight--;

        if ( colRight > right )
        {
            break;
        }

        if ( colRight >= left )
        {
            dc.SetPen( GetColGridLinePen(i) );
            dc.DrawLine( colRight, top, colRight, bottom );
        }
    }

    dc.DestroyClippingRegion();
}

void wxGrid::DrawRowLabels( wxDC& dc, const wxArrayInt& rows)
{
    if ( !m_numRows )
        return;

    size_t i;
    size_t numLabels = rows.GetCount();

    for ( i = 0; i < numLabels; i++ )
    {
        DrawRowLabel( dc, rows[i] );
    }
}

void wxGrid::DrawRowLabel( wxDC& dc, int row )
{
    if ( GetRowHeight(row) <= 0 || m_rowLabelWidth <= 0 )
        return;

    wxRect rect;

#if 0
def __WXGTK20__
    rect.SetX( 1 );
    rect.SetY( GetRowTop(row) + 1 );
    rect.SetWidth( m_rowLabelWidth - 2 );
    rect.SetHeight( GetRowHeight(row) - 2 );

    CalcScrolledPosition( 0, rect.y, NULL, &rect.y );

    wxWindowDC *win_dc = (wxWindowDC*) &dc;

    wxRendererNative::Get().DrawHeaderButton( win_dc->m_owner, dc, rect, 0 );
#else
    int rowTop = GetRowTop(row),
        rowBottom = GetRowBottom(row) - 1;

    dc.SetPen( wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW), 1, wxSOLID) );
    dc.DrawLine( m_rowLabelWidth - 1, rowTop, m_rowLabelWidth - 1, rowBottom );
    dc.DrawLine( 0, rowTop, 0, rowBottom );
    dc.DrawLine( 0, rowBottom, m_rowLabelWidth, rowBottom );

    dc.SetPen( *wxWHITE_PEN );
    dc.DrawLine( 1, rowTop, 1, rowBottom );
    dc.DrawLine( 1, rowTop, m_rowLabelWidth - 1, rowTop );
#endif

    dc.SetBackgroundMode( wxTRANSPARENT );
    dc.SetTextForeground( GetLabelTextColour() );
    dc.SetFont( GetLabelFont() );

    int hAlign, vAlign;
    GetRowLabelAlignment( &hAlign, &vAlign );

    rect.SetX( 2 );
    rect.SetY( GetRowTop(row) + 2 );
    rect.SetWidth( m_rowLabelWidth - 4 );
    rect.SetHeight( GetRowHeight(row) - 4 );
    DrawTextRectangle( dc, GetRowLabelValue( row ), rect, hAlign, vAlign );
}

void wxGrid::DrawColLabels( wxDC& dc,const wxArrayInt& cols )
{
    if ( !m_numCols )
        return;

    size_t i;
    size_t numLabels = cols.GetCount();

    for ( i = 0; i < numLabels; i++ )
    {
        DrawColLabel( dc, cols[i] );
    }
}

void wxGrid::DrawColLabel( wxDC& dc, int col )
{
    if ( GetColWidth(col) <= 0 || m_colLabelHeight <= 0 )
        return;

    int colLeft = GetColLeft(col);

    wxRect rect;

#if 0
def __WXGTK20__
    rect.SetX( colLeft + 1 );
    rect.SetY( 1 );
    rect.SetWidth( GetColWidth(col) - 2 );
    rect.SetHeight( m_colLabelHeight - 2 );

    wxWindowDC *win_dc = (wxWindowDC*) &dc;

    wxRendererNative::Get().DrawHeaderButton( win_dc->m_owner, dc, rect, 0 );
#else
    int colRight = GetColRight(col) - 1;

    dc.SetPen( wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW), 1, wxSOLID) );
    dc.DrawLine( colRight, 0, colRight, m_colLabelHeight - 1 );
    dc.DrawLine( colLeft, 0, colRight, 0 );
    dc.DrawLine( colLeft, m_colLabelHeight - 1,
                 colRight + 1, m_colLabelHeight - 1 );

    dc.SetPen( *wxWHITE_PEN );
    dc.DrawLine( colLeft, 1, colLeft, m_colLabelHeight - 1 );
    dc.DrawLine( colLeft, 1, colRight, 1 );
#endif

    dc.SetBackgroundMode( wxTRANSPARENT );
    dc.SetTextForeground( GetLabelTextColour() );
    dc.SetFont( GetLabelFont() );

    int hAlign, vAlign, orient;
    GetColLabelAlignment( &hAlign, &vAlign );
    orient = GetColLabelTextOrientation();

    rect.SetX( colLeft + 2 );
    rect.SetY( 2 );
    rect.SetWidth( GetColWidth(col) - 4 );
    rect.SetHeight( m_colLabelHeight - 4 );
    DrawTextRectangle( dc, GetColLabelValue( col ), rect, hAlign, vAlign, orient );
}

void wxGrid::DrawTextRectangle( wxDC& dc,
                                const wxString& value,
                                const wxRect& rect,
                                int horizAlign,
                                int vertAlign,
                                int textOrientation )
{
    wxArrayString lines;

    StringToLines( value, lines );

    // Forward to new API.
    DrawTextRectangle( dc,
        lines,
        rect,
        horizAlign,
        vertAlign,
        textOrientation );
}

// VZ: this should be replaced with wxDC::DrawLabel() to which we just have to
//     add textOrientation support
void wxGrid::DrawTextRectangle(wxDC& dc,
                               const wxArrayString& lines,
                               const wxRect& rect,
                               int horizAlign,
                               int vertAlign,
                               int textOrientation)
{
    if ( lines.empty() )
        return;

    wxDCClipper clip(dc, rect);

    long textWidth,
         textHeight;

    if ( textOrientation == wxHORIZONTAL )
        GetTextBoxSize( dc, lines, &textWidth, &textHeight );
    else
        GetTextBoxSize( dc, lines, &textHeight, &textWidth );

    int x = 0,
        y = 0;
    switch ( vertAlign )
    {
        case wxALIGN_BOTTOM:
            if ( textOrientation == wxHORIZONTAL )
                y = rect.y + (rect.height - textHeight - 1);
            else
                x = rect.x + rect.width - textWidth;
            break;

        case wxALIGN_CENTRE:
            if ( textOrientation == wxHORIZONTAL )
                y = rect.y + ((rect.height - textHeight) / 2);
            else
                x = rect.x + ((rect.width - textWidth) / 2);
            break;

        case wxALIGN_TOP:
        default:
            if ( textOrientation == wxHORIZONTAL )
                y = rect.y + 1;
            else
                x = rect.x + 1;
            break;
    }

    // Align each line of a multi-line label
    size_t nLines = lines.GetCount();
    for ( size_t l = 0; l < nLines; l++ )
    {
        const wxString& line = lines[l];

        if ( line.empty() )
        {
            *(textOrientation == wxHORIZONTAL ? &y : &x) += dc.GetCharHeight();
            continue;
        }

        long lineWidth = 0,
             lineHeight = 0;
        dc.GetTextExtent(line, &lineWidth, &lineHeight);

        switch ( horizAlign )
        {
            case wxALIGN_RIGHT:
                if ( textOrientation == wxHORIZONTAL )
                    x = rect.x + (rect.width - lineWidth - 1);
                else
                    y = rect.y + lineWidth + 1;
                break;

            case wxALIGN_CENTRE:
                if ( textOrientation == wxHORIZONTAL )
                    x = rect.x + ((rect.width - lineWidth) / 2);
                else
                    y = rect.y + rect.height - ((rect.height - lineWidth) / 2);
                break;

            case wxALIGN_LEFT:
            default:
                if ( textOrientation == wxHORIZONTAL )
                    x = rect.x + 1;
                else
                    y = rect.y + rect.height - 1;
                break;
        }

        if ( textOrientation == wxHORIZONTAL )
        {
            dc.DrawText( line, x, y );
            y += lineHeight;
        }
        else
        {
            dc.DrawRotatedText( line, x, y, 90.0 );
            x += lineHeight;
        }
    }
}

// Split multi-line text up into an array of strings.
// Any existing contents of the string array are preserved.
//
void wxGrid::StringToLines( const wxString& value, wxArrayString& lines )
{
    int startPos = 0;
    int pos;
    wxString eol = wxTextFile::GetEOL( wxTextFileType_Unix );
    wxString tVal = wxTextFile::Translate( value, wxTextFileType_Unix );

    while ( startPos < (int)tVal.length() )
    {
        pos = tVal.Mid(startPos).Find( eol );
        if ( pos < 0 )
        {
            break;
        }
        else if ( pos == 0 )
        {
            lines.Add( wxEmptyString );
        }
        else
        {
            lines.Add( tVal.Mid(startPos, pos) );
        }

        startPos += pos + 1;
    }

    if ( startPos < (int)tVal.length() )
    {
        lines.Add( tVal.Mid( startPos ) );
    }
}

void wxGrid::GetTextBoxSize( const wxDC& dc,
                             const wxArrayString& lines,
                             long *width, long *height )
{
    long w = 0;
    long h = 0;
    long lineW = 0, lineH = 0;

    size_t i;
    for ( i = 0; i < lines.GetCount(); i++ )
    {
        dc.GetTextExtent( lines[i], &lineW, &lineH );
        w = wxMax( w, lineW );
        h += lineH;
    }

    *width = w;
    *height = h;
}

//
// ------ Batch processing.
//
void wxGrid::EndBatch()
{
    if ( m_batchCount > 0 )
    {
        m_batchCount--;
        if ( !m_batchCount )
        {
            CalcDimensions();
            m_rowLabelWin->Refresh();
            m_colLabelWin->Refresh();
            m_cornerLabelWin->Refresh();
            m_gridWin->Refresh();
        }
    }
}

// Use this, rather than wxWindow::Refresh(), to force an immediate
// repainting of the grid. Has no effect if you are already inside a
// BeginBatch / EndBatch block.
//
void wxGrid::ForceRefresh()
{
    BeginBatch();
    EndBatch();
}

bool wxGrid::Enable(bool enable)
{
    if ( !wxScrolledWindow::Enable(enable) )
        return false;

    // redraw in the new state
    m_gridWin->Refresh();

    return true;
}

//
// ------ Edit control functions
//

void wxGrid::EnableEditing( bool edit )
{
    // TODO: improve this ?
    //
    if ( edit != m_editable )
    {
        if (!edit)
            EnableCellEditControl(edit);
        m_editable = edit;
    }
}

void wxGrid::EnableCellEditControl( bool enable )
{
    if (! m_editable)
        return;

    if ( enable != m_cellEditCtrlEnabled )
    {
        if ( enable )
        {
            if (SendEvent( wxEVT_GRID_EDITOR_SHOWN) <0)
                return;

            // this should be checked by the caller!
            wxASSERT_MSG( CanEnableCellControl(), _T("can't enable editing for this cell!") );

            // do it before ShowCellEditControl()
            m_cellEditCtrlEnabled = enable;

            ShowCellEditControl();
        }
        else
        {
            //FIXME:add veto support
            SendEvent( wxEVT_GRID_EDITOR_HIDDEN );

            HideCellEditControl();
            SaveEditControlValue();

            // do it after HideCellEditControl()
            m_cellEditCtrlEnabled = enable;
        }
    }
}

bool wxGrid::IsCurrentCellReadOnly() const
{
    // const_cast
    wxGridCellAttr* attr = ((wxGrid *)this)->GetCellAttr(m_currentCellCoords);
    bool readonly = attr->IsReadOnly();
    attr->DecRef();

    return readonly;
}

bool wxGrid::CanEnableCellControl() const
{
    return m_editable && (m_currentCellCoords != wxGridNoCellCoords) &&
        !IsCurrentCellReadOnly();
}

bool wxGrid::IsCellEditControlEnabled() const
{
    // the cell edit control might be disable for all cells or just for the
    // current one if it's read only
    return m_cellEditCtrlEnabled ? !IsCurrentCellReadOnly() : false;
}

bool wxGrid::IsCellEditControlShown() const
{
    bool isShown = false;

    if ( m_cellEditCtrlEnabled )
    {
        int row = m_currentCellCoords.GetRow();
        int col = m_currentCellCoords.GetCol();
        wxGridCellAttr* attr = GetCellAttr(row, col);
        wxGridCellEditor* editor = attr->GetEditor((wxGrid*) this, row, col);
        attr->DecRef();

        if ( editor )
        {
            if ( editor->IsCreated() )
            {
                isShown = editor->GetControl()->IsShown();
            }

            editor->DecRef();
        }
    }

    return isShown;
}

void wxGrid::ShowCellEditControl()
{
    if ( IsCellEditControlEnabled() )
    {
        if ( !IsVisible( m_currentCellCoords, false ) )
        {
            m_cellEditCtrlEnabled = false;
            return;
        }
        else
        {
            wxRect rect = CellToRect( m_currentCellCoords );
            int row = m_currentCellCoords.GetRow();
            int col = m_currentCellCoords.GetCol();

            // if this is part of a multicell, find owner (topleft)
            int cell_rows, cell_cols;
            GetCellSize( row, col, &cell_rows, &cell_cols );
            if ( cell_rows <= 0 || cell_cols <= 0 )
            {
                row += cell_rows;
                col += cell_cols;
                m_currentCellCoords.SetRow( row );
                m_currentCellCoords.SetCol( col );
            }

            // erase the highlight and the cell contents because the editor
            // might not cover the entire cell
            wxClientDC dc( m_gridWin );
            PrepareDC( dc );
            wxGridCellAttr* attr = GetCellAttr(row, col);
            dc.SetBrush(wxBrush(attr->GetBackgroundColour(), wxSOLID));
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRectangle(rect);

            // convert to scrolled coords
            CalcScrolledPosition( rect.x, rect.y, &rect.x, &rect.y );

            int nXMove = 0;
            if (rect.x < 0)
                nXMove = rect.x;

            // cell is shifted by one pixel
            // However, don't allow x or y to become negative
            // since the SetSize() method interprets that as
            // "don't change."
            if (rect.x > 0)
                rect.x--;
            if (rect.y > 0)
                rect.y--;

            wxGridCellEditor* editor = attr->GetEditor(this, row, col);
            if ( !editor->IsCreated() )
            {
                editor->Create(m_gridWin, wxID_ANY,
                               new wxGridCellEditorEvtHandler(this, editor));

                wxGridEditorCreatedEvent evt(GetId(),
                                             wxEVT_GRID_EDITOR_CREATED,
                                             this,
                                             row,
                                             col,
                                             editor->GetControl());
                GetEventHandler()->ProcessEvent(evt);
            }

            // resize editor to overflow into righthand cells if allowed
            int maxWidth = rect.width;
            wxString value = GetCellValue(row, col);
            if ( (value != wxEmptyString) && (attr->GetOverflow()) )
            {
                int y;
                GetTextExtent(value, &maxWidth, &y, NULL, NULL, &attr->GetFont());
                if (maxWidth < rect.width)
                    maxWidth = rect.width;
            }

            int client_right = m_gridWin->GetClientSize().GetWidth();
            if (rect.x + maxWidth > client_right)
                maxWidth = client_right - rect.x;

            if ((maxWidth > rect.width) && (col < m_numCols) && m_table)
            {
                GetCellSize( row, col, &cell_rows, &cell_cols );
                // may have changed earlier
                for (int i = col + cell_cols; i < m_numCols; i++)
                {
                    int c_rows, c_cols;
                    GetCellSize( row, i, &c_rows, &c_cols );

                    // looks weird going over a multicell
                    if (m_table->IsEmptyCell( row, i ) &&
                            (rect.width < maxWidth) && (c_rows == 1))
                    {
                        rect.width += GetColWidth( i );
                    }
                    else
                        break;
                }

                if (rect.GetRight() > client_right)
                    rect.SetRight( client_right - 1 );
            }

            editor->SetCellAttr( attr );
            editor->SetSize( rect );
            if (nXMove != 0)
                editor->GetControl()->Move(
                    editor->GetControl()->GetPosition().x + nXMove,
                    editor->GetControl()->GetPosition().y );
            editor->Show( true, attr );

            // recalc dimensions in case we need to
            // expand the scrolled window to account for editor
            CalcDimensions();

            editor->BeginEdit(row, col, this);
            editor->SetCellAttr(NULL);

            editor->DecRef();
            attr->DecRef();
        }
    }
}

void wxGrid::HideCellEditControl()
{
    if ( IsCellEditControlEnabled() )
    {
        int row = m_currentCellCoords.GetRow();
        int col = m_currentCellCoords.GetCol();

        wxGridCellAttr *attr = GetCellAttr(row, col);
        wxGridCellEditor *editor = attr->GetEditor(this, row, col);
        const bool
            editorHadFocus = wxWindow::FindFocus() == editor->GetControl();
        editor->Show( false );
        editor->DecRef();
        attr->DecRef();

        // return the focus to the grid itself if the editor had it
        //
        // note that we must not do this unconditionally to avoid stealing
        // focus from the window which just received it if we are hiding the
        // editor precisely because we lost focus
        if ( editorHadFocus )
            m_gridWin->SetFocus();

        // refresh whole row to the right
        wxRect rect( CellToRect(row, col) );
        CalcScrolledPosition(rect.x, rect.y, &rect.x, &rect.y );
        rect.width = m_gridWin->GetClientSize().GetWidth() - rect.x;

#ifdef __WXMAC__
        // ensure that the pixels under the focus ring get refreshed as well
        rect.Inflate(10, 10);
#endif

        m_gridWin->Refresh( false, &rect );
    }
}

void wxGrid::SaveEditControlValue()
{
    if ( IsCellEditControlEnabled() )
    {
        int row = m_currentCellCoords.GetRow();
        int col = m_currentCellCoords.GetCol();

        wxString oldval = GetCellValue(row, col);

        wxGridCellAttr* attr = GetCellAttr(row, col);
        wxGridCellEditor* editor = attr->GetEditor(this, row, col);
        bool changed = editor->EndEdit(row, col, this);

        editor->DecRef();
        attr->DecRef();

        if (changed)
        {
            if ( SendEvent( wxEVT_GRID_CELL_CHANGE,
                       m_currentCellCoords.GetRow(),
                       m_currentCellCoords.GetCol() ) < 0 )
            {
                // Event has been vetoed, set the data back.
                SetCellValue(row, col, oldval);
            }
        }
    }
}

//
// ------ Grid location functions
//  Note that all of these functions work with the logical coordinates of
//  grid cells and labels so you will need to convert from device
//  coordinates for mouse events etc.
//

void wxGrid::XYToCell( int x, int y, wxGridCellCoords& coords )
{
    int row = YToRow(y);
    int col = XToCol(x);

    if ( row == -1 || col == -1 )
    {
        coords = wxGridNoCellCoords;
    }
    else
    {
        coords.Set( row, col );
    }
}

// Internal Helper function for computing row or column from some
// (unscrolled) coordinate value, using either
// m_defaultRowHeight/m_defaultColWidth or binary search on array
// of m_rowBottoms/m_ColRights to speed up the search!

static int CoordToRowOrCol(int coord, int defaultDist, int minDist,
                           const wxArrayInt& BorderArray, int nMax,
                           bool clipToMinMax)
{
    if (coord < 0)
        return clipToMinMax && (nMax > 0) ? 0 : -1;

    if (!defaultDist)
        defaultDist = 1;

    size_t i_max = coord / defaultDist,
           i_min = 0;

    if (BorderArray.IsEmpty())
    {
        if ((int) i_max < nMax)
            return i_max;
        return clipToMinMax ? nMax - 1 : -1;
    }

    if ( i_max >= BorderArray.GetCount())
    {
        i_max = BorderArray.GetCount() - 1;
    }
    else
    {
        if ( coord >= BorderArray[i_max])
        {
            i_min = i_max;
            if (minDist)
                i_max = coord / minDist;
            else
                i_max =  BorderArray.GetCount() - 1;
        }

        if ( i_max >= BorderArray.GetCount())
            i_max = BorderArray.GetCount() - 1;
    }

    if ( coord >= BorderArray[i_max])
        return clipToMinMax ? (int)i_max : -1;
    if ( coord < BorderArray[0] )
        return 0;

    while ( i_max - i_min > 0 )
    {
        wxCHECK_MSG(BorderArray[i_min] <= coord && coord < BorderArray[i_max],
                    0, _T("wxGrid: internal error in CoordToRowOrCol"));
        if (coord >=  BorderArray[ i_max - 1])
            return i_max;
        else
            i_max--;
        int median = i_min + (i_max - i_min + 1) / 2;
        if (coord < BorderArray[median])
            i_max = median;
        else
            i_min = median;
    }

    return i_max;
}

int wxGrid::YToRow( int y )
{
    return CoordToRowOrCol(y, m_defaultRowHeight,
                           m_minAcceptableRowHeight, m_rowBottoms, m_numRows, false);
}

int wxGrid::XToCol( int x, bool clipToMinMax )
{
    if (x < 0)
        return clipToMinMax && (m_numCols > 0) ? GetColAt( 0 ) : -1;

    if (!m_defaultColWidth)
        m_defaultColWidth = 1;

    int maxPos = x / m_defaultColWidth;
    int minPos = 0;

    if (m_colRights.IsEmpty())
    {
        if(maxPos < m_numCols)
            return GetColAt( maxPos );
        return clipToMinMax ? GetColAt( m_numCols - 1 ) : -1;
    }

    if ( maxPos >= m_numCols)
        maxPos = m_numCols - 1;
    else
    {
        if ( x >= m_colRights[GetColAt( maxPos )])
        {
            minPos = maxPos;
            if (m_minAcceptableColWidth)
                maxPos = x / m_minAcceptableColWidth;
            else
                maxPos =  m_numCols - 1;
        }
        if ( maxPos >= m_numCols)
            maxPos = m_numCols - 1;
    }

    //X is beyond the last column
    if ( x >= m_colRights[GetColAt( maxPos )])
        return clipToMinMax ? GetColAt( maxPos ) : -1;

    //X is before the first column
    if ( x < m_colRights[GetColAt( 0 )] )
        return GetColAt( 0 );

    //Perform a binary search
    while ( maxPos - minPos > 0 )
    {
        wxCHECK_MSG(m_colRights[GetColAt( minPos )] <= x && x < m_colRights[GetColAt( maxPos )],
                    0, _T("wxGrid: internal error in XToCol"));

        if (x >=  m_colRights[GetColAt( maxPos - 1 )])
            return GetColAt( maxPos );
        else
            maxPos--;
        int median = minPos + (maxPos - minPos + 1) / 2;
        if (x < m_colRights[GetColAt( median )])
            maxPos = median;
        else
            minPos = median;
    }
    return GetColAt( maxPos );
}

// return the row number that that the y coord is near
//  the edge of, or -1 if not near an edge.
// coords can only possibly be near an edge if
//    (a) the row/column is large enough to still allow for an "inner" area
//        that is _not_ nead the edge (i.e., if the height/width is smaller
//        than WXGRID_LABEL_EDGE_ZONE, coords are _never_ considered to be
//        near the edge).
//   and
//    (b) resizing rows/columns (the thing for which edge detection is
//        relevant at all) is enabled.
//
int wxGrid::YToEdgeOfRow( int y )
{
    int i;
    i = internalYToRow(y);

    if ( GetRowHeight(i) > WXGRID_LABEL_EDGE_ZONE && CanDragRowSize() )
    {
        // We know that we are in row i, test whether we are
        // close enough to lower or upper border, respectively.
        if ( abs(GetRowBottom(i) - y) < WXGRID_LABEL_EDGE_ZONE )
            return i;
        else if ( i > 0 && y - GetRowTop(i) < WXGRID_LABEL_EDGE_ZONE )
            return i - 1;
    }

    return -1;
}

// return the col number that that the x coord is near the edge of, or
// -1 if not near an edge
// See comment at YToEdgeOfRow for conditions on edge detection.
//
int wxGrid::XToEdgeOfCol( int x )
{
    int i;
    i = internalXToCol(x);

    if ( GetColWidth(i) > WXGRID_LABEL_EDGE_ZONE && CanDragColSize() )
    {
        // We know that we are in column i; test whether we are
        // close enough to right or left border, respectively.
        if ( abs(GetColRight(i) - x) < WXGRID_LABEL_EDGE_ZONE )
            return i;
        else if ( i > 0 && x - GetColLeft(i) < WXGRID_LABEL_EDGE_ZONE )
            return i - 1;
    }

    return -1;
}

wxRect wxGrid::CellToRect( int row, int col )
{
    wxRect rect( -1, -1, -1, -1 );

    if ( row >= 0 && row < m_numRows &&
         col >= 0 && col < m_numCols )
    {
        int i, cell_rows, cell_cols;
        rect.width = rect.height = 0;
        GetCellSize( row, col, &cell_rows, &cell_cols );
        // if negative then find multicell owner
        if (cell_rows < 0)
            row += cell_rows;
        if (cell_cols < 0)
             col += cell_cols;
        GetCellSize( row, col, &cell_rows, &cell_cols );

        rect.x = GetColLeft(col);
        rect.y = GetRowTop(row);
        for (i=col; i < col + cell_cols; i++)
            rect.width += GetColWidth(i);
        for (i=row; i < row + cell_rows; i++)
            rect.height += GetRowHeight(i);
    }

    // if grid lines are enabled, then the area of the cell is a bit smaller
    if (m_gridLinesEnabled)
    {
        rect.width -= 1;
        rect.height -= 1;
    }

    return rect;
}

bool wxGrid::IsVisible( int row, int col, bool wholeCellVisible )
{
    // get the cell rectangle in logical coords
    //
    wxRect r( CellToRect( row, col ) );

    // convert to device coords
    //
    int left, top, right, bottom;
    CalcScrolledPosition( r.GetLeft(), r.GetTop(), &left, &top );
    CalcScrolledPosition( r.GetRight(), r.GetBottom(), &right, &bottom );

    // check against the client area of the grid window
    int cw, ch;
    m_gridWin->GetClientSize( &cw, &ch );

    if ( wholeCellVisible )
    {
        // is the cell wholly visible ?
        return ( left >= 0 && right <= cw &&
                 top >= 0 && bottom <= ch );
    }
    else
    {
        // is the cell partly visible ?
        //
        return ( ((left >= 0 && left < cw) || (right > 0 && right <= cw)) &&
                 ((top >= 0 && top < ch) || (bottom > 0 && bottom <= ch)) );
    }
}

// make the specified cell location visible by doing a minimal amount
// of scrolling
//
void wxGrid::MakeCellVisible( int row, int col )
{
    int i;
    int xpos = -1, ypos = -1;

    if ( row >= 0 && row < m_numRows &&
         col >= 0 && col < m_numCols )
    {
        // get the cell rectangle in logical coords
        wxRect r( CellToRect( row, col ) );

        // convert to device coords
        int left, top, right, bottom;
        CalcScrolledPosition( r.GetLeft(), r.GetTop(), &left, &top );
        CalcScrolledPosition( r.GetRight(), r.GetBottom(), &right, &bottom );

        int cw, ch;
        m_gridWin->GetClientSize( &cw, &ch );

        if ( top < 0 )
        {
            ypos = r.GetTop();
        }
        else if ( bottom > ch )
        {
            int h = r.GetHeight();
            ypos = r.GetTop();
            for ( i = row - 1; i >= 0; i-- )
            {
                int rowHeight = GetRowHeight(i);
                if ( h + rowHeight > ch )
                    break;

                h += rowHeight;
                ypos -= rowHeight;
            }

            // we divide it later by GRID_SCROLL_LINE, make sure that we don't
            // have rounding errors (this is important, because if we do,
            // we might not scroll at all and some cells won't be redrawn)
            //
            // Sometimes GRID_SCROLL_LINE / 2 is not enough,
            // so just add a full scroll unit...
            ypos += m_scrollLineY;
        }

        // special handling for wide cells - show always left part of the cell!
        // Otherwise, e.g. when stepping from row to row, it would jump between
        // left and right part of the cell on every step!
//      if ( left < 0 )
        if ( left < 0 || (right - left) >= cw )
        {
            xpos = r.GetLeft();
        }
        else if ( right > cw )
        {
            // position the view so that the cell is on the right
            int x0, y0;
            CalcUnscrolledPosition(0, 0, &x0, &y0);
            xpos = x0 + (right - cw);

            // see comment for ypos above
            xpos += m_scrollLineX;
        }

        if ( xpos != -1 || ypos != -1 )
        {
            if ( xpos != -1 )
                xpos /= m_scrollLineX;
            if ( ypos != -1 )
                ypos /= m_scrollLineY;
            Scroll( xpos, ypos );
            AdjustScrollbars();
        }
    }
}

//
// ------ Grid cursor movement functions
//

bool wxGrid::MoveCursorUp( bool expandSelection )
{
    if ( m_currentCellCoords != wxGridNoCellCoords &&
         m_currentCellCoords.GetRow() >= 0 )
    {
        if ( expandSelection )
        {
            if ( m_selectingKeyboard == wxGridNoCellCoords )
                m_selectingKeyboard = m_currentCellCoords;
            if ( m_selectingKeyboard.GetRow() > 0 )
            {
                m_selectingKeyboard.SetRow( m_selectingKeyboard.GetRow() - 1 );
                MakeCellVisible( m_selectingKeyboard.GetRow(),
                                 m_selectingKeyboard.GetCol() );
                HighlightBlock( m_currentCellCoords, m_selectingKeyboard );
            }
        }
        else if ( m_currentCellCoords.GetRow() > 0 )
        {
            int row = m_currentCellCoords.GetRow() - 1;
            int col = m_currentCellCoords.GetCol();
            ClearSelection();
            MakeCellVisible( row, col );
            SetCurrentCell( row, col );
        }
        else
            return false;

        return true;
    }

    return false;
}

bool wxGrid::MoveCursorDown( bool expandSelection )
{
    if ( m_currentCellCoords != wxGridNoCellCoords &&
         m_currentCellCoords.GetRow() < m_numRows )
    {
        if ( expandSelection )
        {
            if ( m_selectingKeyboard == wxGridNoCellCoords )
                m_selectingKeyboard = m_currentCellCoords;
            if ( m_selectingKeyboard.GetRow() < m_numRows - 1 )
            {
                m_selectingKeyboard.SetRow( m_selectingKeyboard.GetRow() + 1 );
                MakeCellVisible( m_selectingKeyboard.GetRow(),
                        m_selectingKeyboard.GetCol() );
                HighlightBlock( m_currentCellCoords, m_selectingKeyboard );
            }
        }
        else if ( m_currentCellCoords.GetRow() < m_numRows - 1 )
        {
            int row = m_currentCellCoords.GetRow() + 1;
            int col = m_currentCellCoords.GetCol();
            ClearSelection();
            MakeCellVisible( row, col );
            SetCurrentCell( row, col );
        }
        else
            return false;

        return true;
    }

    return false;
}

bool wxGrid::MoveCursorLeft( bool expandSelection )
{
    if ( m_currentCellCoords != wxGridNoCellCoords &&
         m_currentCellCoords.GetCol() >= 0 )
    {
        if ( expandSelection )
        {
            if ( m_selectingKeyboard == wxGridNoCellCoords )
                m_selectingKeyboard = m_currentCellCoords;
            if ( m_selectingKeyboard.GetCol() > 0 )
            {
                m_selectingKeyboard.SetCol( m_selectingKeyboard.GetCol() - 1 );
                MakeCellVisible( m_selectingKeyboard.GetRow(),
                        m_selectingKeyboard.GetCol() );
                HighlightBlock( m_currentCellCoords, m_selectingKeyboard );
            }
        }
        else if ( GetColPos( m_currentCellCoords.GetCol() ) > 0 )
        {
            int row = m_currentCellCoords.GetRow();
            int col = GetColAt( GetColPos( m_currentCellCoords.GetCol() ) - 1 );
            ClearSelection();

            MakeCellVisible( row, col );
            SetCurrentCell( row, col );
        }
        else
            return false;

        return true;
    }

    return false;
}

bool wxGrid::MoveCursorRight( bool expandSelection )
{
    if ( m_currentCellCoords != wxGridNoCellCoords &&
         m_currentCellCoords.GetCol() < m_numCols )
    {
        if ( expandSelection )
        {
            if ( m_selectingKeyboard == wxGridNoCellCoords )
                m_selectingKeyboard = m_currentCellCoords;
            if ( m_selectingKeyboard.GetCol() < m_numCols - 1 )
            {
                m_selectingKeyboard.SetCol( m_selectingKeyboard.GetCol() + 1 );
                MakeCellVisible( m_selectingKeyboard.GetRow(),
                        m_selectingKeyboard.GetCol() );
                HighlightBlock( m_currentCellCoords, m_selectingKeyboard );
            }
        }
        else if ( GetColPos( m_currentCellCoords.GetCol() ) < m_numCols - 1 )
        {
            int row = m_currentCellCoords.GetRow();
            int col = GetColAt( GetColPos( m_currentCellCoords.GetCol() ) + 1 );
            ClearSelection();

            MakeCellVisible( row, col );
            SetCurrentCell( row, col );
        }
        else
            return false;

        return true;
    }

    return false;
}

bool wxGrid::MovePageUp()
{
    if ( m_currentCellCoords == wxGridNoCellCoords )
        return false;

    int row = m_currentCellCoords.GetRow();
    if ( row > 0 )
    {
        int cw, ch;
        m_gridWin->GetClientSize( &cw, &ch );

        int y = GetRowTop(row);
        int newRow = internalYToRow( y - ch + 1 );

        if ( newRow == row )
        {
            // row > 0, so newRow can never be less than 0 here.
            newRow = row - 1;
        }

        MakeCellVisible( newRow, m_currentCellCoords.GetCol() );
        SetCurrentCell( newRow, m_currentCellCoords.GetCol() );

        return true;
    }

    return false;
}

bool wxGrid::MovePageDown()
{
    if ( m_currentCellCoords == wxGridNoCellCoords )
        return false;

    int row = m_currentCellCoords.GetRow();
    if ( (row + 1) < m_numRows )
    {
        int cw, ch;
        m_gridWin->GetClientSize( &cw, &ch );

        int y = GetRowTop(row);
        int newRow = internalYToRow( y + ch );
        if ( newRow == row )
        {
            // row < m_numRows, so newRow can't overflow here.
            newRow = row + 1;
        }

        MakeCellVisible( newRow, m_currentCellCoords.GetCol() );
        SetCurrentCell( newRow, m_currentCellCoords.GetCol() );

        return true;
    }

    return false;
}

bool wxGrid::MoveCursorUpBlock( bool expandSelection )
{
    if ( m_table &&
         m_currentCellCoords != wxGridNoCellCoords &&
         m_currentCellCoords.GetRow() > 0 )
    {
        int row = m_currentCellCoords.GetRow();
        int col = m_currentCellCoords.GetCol();

        if ( m_table->IsEmptyCell(row, col) )
        {
            // starting in an empty cell: find the next block of
            // non-empty cells
            //
            while ( row > 0 )
            {
                row--;
                if ( !(m_table->IsEmptyCell(row, col)) )
                    break;
            }
        }
        else if ( m_table->IsEmptyCell(row - 1, col) )
        {
            // starting at the top of a block: find the next block
            //
            row--;
            while ( row > 0 )
            {
                row--;
                if ( !(m_table->IsEmptyCell(row, col)) )
                    break;
            }
        }
        else
        {
            // starting within a block: find the top of the block
            //
            while ( row > 0 )
            {
                row--;
                if ( m_table->IsEmptyCell(row, col) )
                {
                    row++;
                    break;
                }
            }
        }

        MakeCellVisible( row, col );
        if ( expandSelection )
        {
            m_selectingKeyboard = wxGridCellCoords( row, col );
            HighlightBlock( m_currentCellCoords, m_selectingKeyboard );
        }
        else
        {
            ClearSelection();
            SetCurrentCell( row, col );
        }

        return true;
    }

    return false;
}

bool wxGrid::MoveCursorDownBlock( bool expandSelection )
{
    if ( m_table &&
         m_currentCellCoords != wxGridNoCellCoords &&
         m_currentCellCoords.GetRow() < m_numRows - 1 )
    {
        int row = m_currentCellCoords.GetRow();
        int col = m_currentCellCoords.GetCol();

        if ( m_table->IsEmptyCell(row, col) )
        {
            // starting in an empty cell: find the next block of
            // non-empty cells
            //
            while ( row < m_numRows - 1 )
            {
                row++;
                if ( !(m_table->IsEmptyCell(row, col)) )
                    break;
            }
        }
        else if ( m_table->IsEmptyCell(row + 1, col) )
        {
            // starting at the bottom of a block: find the next block
            //
            row++;
            while ( row < m_numRows - 1 )
            {
                row++;
                if ( !(m_table->IsEmptyCell(row, col)) )
                    break;
            }
        }
        else
        {
            // starting within a block: find the bottom of the block
            //
            while ( row < m_numRows - 1 )
            {
                row++;
                if ( m_table->IsEmptyCell(row, col) )
                {
                    row--;
                    break;
                }
            }
        }

        MakeCellVisible( row, col );
        if ( expandSelection )
        {
            m_selectingKeyboard = wxGridCellCoords( row, col );
            HighlightBlock( m_currentCellCoords, m_selectingKeyboard );
        }
        else
        {
            ClearSelection();
            SetCurrentCell( row, col );
        }

        return true;
    }

    return false;
}

bool wxGrid::MoveCursorLeftBlock( bool expandSelection )
{
    if ( m_table &&
         m_currentCellCoords != wxGridNoCellCoords &&
         m_currentCellCoords.GetCol() > 0 )
    {
        int row = m_currentCellCoords.GetRow();
        int col = m_currentCellCoords.GetCol();

        if ( m_table->IsEmptyCell(row, col) )
        {
            // starting in an empty cell: find the next block of
            // non-empty cells
            //
            while ( col > 0 )
            {
                col--;
                if ( !(m_table->IsEmptyCell(row, col)) )
                    break;
            }
        }
        else if ( m_table->IsEmptyCell(row, col - 1) )
        {
            // starting at the left of a block: find the next block
            //
            col--;
            while ( col > 0 )
            {
                col--;
                if ( !(m_table->IsEmptyCell(row, col)) )
                    break;
            }
        }
        else
        {
            // starting within a block: find the left of the block
            //
            while ( col > 0 )
            {
                col--;
                if ( m_table->IsEmptyCell(row, col) )
                {
                    col++;
                    break;
                }
            }
        }

        MakeCellVisible( row, col );
        if ( expandSelection )
        {
            m_selectingKeyboard = wxGridCellCoords( row, col );
            HighlightBlock( m_currentCellCoords, m_selectingKeyboard );
        }
        else
        {
            ClearSelection();
            SetCurrentCell( row, col );
        }

        return true;
    }

    return false;
}

bool wxGrid::MoveCursorRightBlock( bool expandSelection )
{
    if ( m_table &&
         m_currentCellCoords != wxGridNoCellCoords &&
         m_currentCellCoords.GetCol() < m_numCols - 1 )
    {
        int row = m_currentCellCoords.GetRow();
        int col = m_currentCellCoords.GetCol();

        if ( m_table->IsEmptyCell(row, col) )
        {
            // starting in an empty cell: find the next block of
            // non-empty cells
            //
            while ( col < m_numCols - 1 )
            {
                col++;
                if ( !(m_table->IsEmptyCell(row, col)) )
                    break;
            }
        }
        else if ( m_table->IsEmptyCell(row, col + 1) )
        {
            // starting at the right of a block: find the next block
            //
            col++;
            while ( col < m_numCols - 1 )
            {
                col++;
                if ( !(m_table->IsEmptyCell(row, col)) )
                    break;
            }
        }
        else
        {
            // starting within a block: find the right of the block
            //
            while ( col < m_numCols - 1 )
            {
                col++;
                if ( m_table->IsEmptyCell(row, col) )
                {
                    col--;
                    break;
                }
            }
        }

        MakeCellVisible( row, col );
        if ( expandSelection )
        {
            m_selectingKeyboard = wxGridCellCoords( row, col );
            HighlightBlock( m_currentCellCoords, m_selectingKeyboard );
        }
        else
        {
            ClearSelection();
            SetCurrentCell( row, col );
        }

        return true;
    }

    return false;
}

//
// ------ Label values and formatting
//

void wxGrid::GetRowLabelAlignment( int *horiz, int *vert )
{
    if ( horiz )
        *horiz = m_rowLabelHorizAlign;
    if ( vert )
        *vert  = m_rowLabelVertAlign;
}

void wxGrid::GetColLabelAlignment( int *horiz, int *vert )
{
    if ( horiz )
        *horiz = m_colLabelHorizAlign;
    if ( vert )
        *vert  = m_colLabelVertAlign;
}

int wxGrid::GetColLabelTextOrientation()
{
    return m_colLabelTextOrientation;
}

wxString wxGrid::GetRowLabelValue( int row )
{
    if ( m_table )
    {
        return m_table->GetRowLabelValue( row );
    }
    else
    {
        wxString s;
        s << row;
        return s;
    }
}

wxString wxGrid::GetColLabelValue( int col )
{
    if ( m_table )
    {
        return m_table->GetColLabelValue( col );
    }
    else
    {
        wxString s;
        s << col;
        return s;
    }
}

void wxGrid::SetRowLabelSize( int width )
{
    wxASSERT( width >= 0 || width == wxGRID_AUTOSIZE );

    if ( width == wxGRID_AUTOSIZE )
    {
        width = CalcColOrRowLabelAreaMinSize(false/*row*/);
    }

    if ( width != m_rowLabelWidth )
    {
        if ( width == 0 )
        {
            m_rowLabelWin->Show( false );
            m_cornerLabelWin->Show( false );
        }
        else if ( m_rowLabelWidth == 0 )
        {
            m_rowLabelWin->Show( true );
            if ( m_colLabelHeight > 0 )
                m_cornerLabelWin->Show( true );
        }

        m_rowLabelWidth = width;
        CalcWindowSizes();
        wxScrolledWindow::Refresh( true );
    }
}

void wxGrid::SetColLabelSize( int height )
{
    wxASSERT( height >=0 || height == wxGRID_AUTOSIZE );

    if ( height == wxGRID_AUTOSIZE )
    {
        height = CalcColOrRowLabelAreaMinSize(true/*column*/);
    }

    if ( height != m_colLabelHeight )
    {
        if ( height == 0 )
        {
            m_colLabelWin->Show( false );
            m_cornerLabelWin->Show( false );
        }
        else if ( m_colLabelHeight == 0 )
        {
            m_colLabelWin->Show( true );
            if ( m_rowLabelWidth > 0 )
                m_cornerLabelWin->Show( true );
        }

        m_colLabelHeight = height;
        CalcWindowSizes();
        wxScrolledWindow::Refresh( true );
    }
}

void wxGrid::SetLabelBackgroundColour( const wxColour& colour )
{
    if ( m_labelBackgroundColour != colour )
    {
        m_labelBackgroundColour = colour;
        m_rowLabelWin->SetBackgroundColour( colour );
        m_colLabelWin->SetBackgroundColour( colour );
        m_cornerLabelWin->SetBackgroundColour( colour );

        if ( !GetBatchCount() )
        {
            m_rowLabelWin->Refresh();
            m_colLabelWin->Refresh();
            m_cornerLabelWin->Refresh();
        }
    }
}

void wxGrid::SetLabelTextColour( const wxColour& colour )
{
    if ( m_labelTextColour != colour )
    {
        m_labelTextColour = colour;
        if ( !GetBatchCount() )
        {
            m_rowLabelWin->Refresh();
            m_colLabelWin->Refresh();
        }
    }
}

void wxGrid::SetLabelFont( const wxFont& font )
{
    m_labelFont = font;
    if ( !GetBatchCount() )
    {
        m_rowLabelWin->Refresh();
        m_colLabelWin->Refresh();
    }
}

void wxGrid::SetRowLabelAlignment( int horiz, int vert )
{
    // allow old (incorrect) defs to be used
    switch ( horiz )
    {
        case wxLEFT:   horiz = wxALIGN_LEFT; break;
        case wxRIGHT:  horiz = wxALIGN_RIGHT; break;
        case wxCENTRE: horiz = wxALIGN_CENTRE; break;
    }

    switch ( vert )
    {
        case wxTOP:    vert = wxALIGN_TOP;    break;
        case wxBOTTOM: vert = wxALIGN_BOTTOM; break;
        case wxCENTRE: vert = wxALIGN_CENTRE; break;
    }

    if ( horiz == wxALIGN_LEFT || horiz == wxALIGN_CENTRE || horiz == wxALIGN_RIGHT )
    {
        m_rowLabelHorizAlign = horiz;
    }

    if ( vert == wxALIGN_TOP || vert == wxALIGN_CENTRE || vert == wxALIGN_BOTTOM )
    {
        m_rowLabelVertAlign = vert;
    }

    if ( !GetBatchCount() )
    {
        m_rowLabelWin->Refresh();
    }
}

void wxGrid::SetColLabelAlignment( int horiz, int vert )
{
    // allow old (incorrect) defs to be used
    switch ( horiz )
    {
        case wxLEFT:   horiz = wxALIGN_LEFT; break;
        case wxRIGHT:  horiz = wxALIGN_RIGHT; break;
        case wxCENTRE: horiz = wxALIGN_CENTRE; break;
    }

    switch ( vert )
    {
        case wxTOP:    vert = wxALIGN_TOP;    break;
        case wxBOTTOM: vert = wxALIGN_BOTTOM; break;
        case wxCENTRE: vert = wxALIGN_CENTRE; break;
    }

    if ( horiz == wxALIGN_LEFT || horiz == wxALIGN_CENTRE || horiz == wxALIGN_RIGHT )
    {
        m_colLabelHorizAlign = horiz;
    }

    if ( vert == wxALIGN_TOP || vert == wxALIGN_CENTRE || vert == wxALIGN_BOTTOM )
    {
        m_colLabelVertAlign = vert;
    }

    if ( !GetBatchCount() )
    {
        m_colLabelWin->Refresh();
    }
}

// Note: under MSW, the default column label font must be changed because it
//       does not support vertical printing
//
// Example: wxFont font(9, wxSWISS, wxNORMAL, wxBOLD);
//                      pGrid->SetLabelFont(font);
//                      pGrid->SetColLabelTextOrientation(wxVERTICAL);
//
void wxGrid::SetColLabelTextOrientation( int textOrientation )
{
    if ( textOrientation == wxHORIZONTAL || textOrientation == wxVERTICAL )
        m_colLabelTextOrientation = textOrientation;

    if ( !GetBatchCount() )
        m_colLabelWin->Refresh();
}

void wxGrid::SetRowLabelValue( int row, const wxString& s )
{
    if ( m_table )
    {
        m_table->SetRowLabelValue( row, s );
        if ( !GetBatchCount() )
        {
            wxRect rect = CellToRect( row, 0 );
            if ( rect.height > 0 )
            {
                CalcScrolledPosition(0, rect.y, &rect.x, &rect.y);
                rect.x = 0;
                rect.width = m_rowLabelWidth;
                m_rowLabelWin->Refresh( true, &rect );
            }
        }
    }
}

void wxGrid::SetColLabelValue( int col, const wxString& s )
{
    if ( m_table )
    {
        m_table->SetColLabelValue( col, s );
        if ( !GetBatchCount() )
        {
            wxRect rect = CellToRect( 0, col );
            if ( rect.width > 0 )
            {
                CalcScrolledPosition(rect.x, 0, &rect.x, &rect.y);
                rect.y = 0;
                rect.height = m_colLabelHeight;
                m_colLabelWin->Refresh( true, &rect );
            }
        }
    }
}

void wxGrid::SetGridLineColour( const wxColour& colour )
{
    if ( m_gridLineColour != colour )
    {
        m_gridLineColour = colour;

        wxClientDC dc( m_gridWin );
        PrepareDC( dc );
        DrawAllGridLines( dc, wxRegion() );
    }
}

void wxGrid::SetCellHighlightColour( const wxColour& colour )
{
    if ( m_cellHighlightColour != colour )
    {
        m_cellHighlightColour = colour;

        wxClientDC dc( m_gridWin );
        PrepareDC( dc );
        wxGridCellAttr* attr = GetCellAttr(m_currentCellCoords);
        DrawCellHighlight(dc, attr);
        attr->DecRef();
    }
}

void wxGrid::SetCellHighlightPenWidth(int width)
{
    if (m_cellHighlightPenWidth != width)
    {
        m_cellHighlightPenWidth = width;

        // Just redrawing the cell highlight is not enough since that won't
        // make any visible change if the the thickness is getting smaller.
        int row = m_currentCellCoords.GetRow();
        int col = m_currentCellCoords.GetCol();
        if ( row == -1 || col == -1 || GetColWidth(col) <= 0 || GetRowHeight(row) <= 0 )
            return;

        wxRect rect = CellToRect(row, col);
        m_gridWin->Refresh(true, &rect);
    }
}

void wxGrid::SetCellHighlightROPenWidth(int width)
{
    if (m_cellHighlightROPenWidth != width)
    {
        m_cellHighlightROPenWidth = width;

        // Just redrawing the cell highlight is not enough since that won't
        // make any visible change if the the thickness is getting smaller.
        int row = m_currentCellCoords.GetRow();
        int col = m_currentCellCoords.GetCol();
        if ( row == -1 || col == -1 ||
                GetColWidth(col) <= 0 || GetRowHeight(row) <= 0 )
            return;

        wxRect rect = CellToRect(row, col);
        m_gridWin->Refresh(true, &rect);
    }
}

void wxGrid::EnableGridLines( bool enable )
{
    if ( enable != m_gridLinesEnabled )
    {
        m_gridLinesEnabled = enable;

        if ( !GetBatchCount() )
        {
            if ( enable )
            {
                wxClientDC dc( m_gridWin );
                PrepareDC( dc );
                DrawAllGridLines( dc, wxRegion() );
            }
            else
            {
                m_gridWin->Refresh();
            }
        }
    }
}

int wxGrid::GetDefaultRowSize()
{
    return m_defaultRowHeight;
}

int wxGrid::GetRowSize( int row )
{
    wxCHECK_MSG( row >= 0 && row < m_numRows, 0, _T("invalid row index") );

    return GetRowHeight(row);
}

int wxGrid::GetDefaultColSize()
{
    return m_defaultColWidth;
}

int wxGrid::GetColSize( int col )
{
    wxCHECK_MSG( col >= 0 && col < m_numCols, 0, _T("invalid column index") );

    return GetColWidth(col);
}

// ============================================================================
// access to the grid attributes: each of them has a default value in the grid
// itself and may be overidden on a per-cell basis
// ============================================================================

// ----------------------------------------------------------------------------
// setting default attributes
// ----------------------------------------------------------------------------

void wxGrid::SetDefaultCellBackgroundColour( const wxColour& col )
{
    m_defaultCellAttr->SetBackgroundColour(col);
#ifdef __WXGTK__
    m_gridWin->SetBackgroundColour(col);
#endif
}

void wxGrid::SetDefaultCellTextColour( const wxColour& col )
{
    m_defaultCellAttr->SetTextColour(col);
}

void wxGrid::SetDefaultCellAlignment( int horiz, int vert )
{
    m_defaultCellAttr->SetAlignment(horiz, vert);
}

void wxGrid::SetDefaultCellOverflow( bool allow )
{
    m_defaultCellAttr->SetOverflow(allow);
}

void wxGrid::SetDefaultCellFont( const wxFont& font )
{
    m_defaultCellAttr->SetFont(font);
}

// For editors and renderers the type registry takes precedence over the
// default attr, so we need to register the new editor/renderer for the string
// data type in order to make setting a default editor/renderer appear to
// work correctly.

void wxGrid::SetDefaultRenderer(wxGridCellRenderer *renderer)
{
    RegisterDataType(wxGRID_VALUE_STRING,
                     renderer,
                     GetDefaultEditorForType(wxGRID_VALUE_STRING));
}

void wxGrid::SetDefaultEditor(wxGridCellEditor *editor)
{
    RegisterDataType(wxGRID_VALUE_STRING,
                     GetDefaultRendererForType(wxGRID_VALUE_STRING),
                     editor);
}

// ----------------------------------------------------------------------------
// access to the default attrbiutes
// ----------------------------------------------------------------------------

wxColour wxGrid::GetDefaultCellBackgroundColour()
{
    return m_defaultCellAttr->GetBackgroundColour();
}

wxColour wxGrid::GetDefaultCellTextColour()
{
    return m_defaultCellAttr->GetTextColour();
}

wxFont wxGrid::GetDefaultCellFont()
{
    return m_defaultCellAttr->GetFont();
}

void wxGrid::GetDefaultCellAlignment( int *horiz, int *vert )
{
    m_defaultCellAttr->GetAlignment(horiz, vert);
}

bool wxGrid::GetDefaultCellOverflow()
{
    return m_defaultCellAttr->GetOverflow();
}

wxGridCellRenderer *wxGrid::GetDefaultRenderer() const
{
    return m_defaultCellAttr->GetRenderer(NULL, 0, 0);
}

wxGridCellEditor *wxGrid::GetDefaultEditor() const
{
    return m_defaultCellAttr->GetEditor(NULL, 0, 0);
}

// ----------------------------------------------------------------------------
// access to cell attributes
// ----------------------------------------------------------------------------

wxColour wxGrid::GetCellBackgroundColour(int row, int col)
{
    wxGridCellAttr *attr = GetCellAttr(row, col);
    wxColour colour = attr->GetBackgroundColour();
    attr->DecRef();

    return colour;
}

wxColour wxGrid::GetCellTextColour( int row, int col )
{
    wxGridCellAttr *attr = GetCellAttr(row, col);
    wxColour colour = attr->GetTextColour();
    attr->DecRef();

    return colour;
}

wxFont wxGrid::GetCellFont( int row, int col )
{
    wxGridCellAttr *attr = GetCellAttr(row, col);
    wxFont font = attr->GetFont();
    attr->DecRef();

    return font;
}

void wxGrid::GetCellAlignment( int row, int col, int *horiz, int *vert )
{
    wxGridCellAttr *attr = GetCellAttr(row, col);
    attr->GetAlignment(horiz, vert);
    attr->DecRef();
}

bool wxGrid::GetCellOverflow( int row, int col )
{
    wxGridCellAttr *attr = GetCellAttr(row, col);
    bool allow = attr->GetOverflow();
    attr->DecRef();

    return allow;
}

void wxGrid::GetCellSize( int row, int col, int *num_rows, int *num_cols )
{
    wxGridCellAttr *attr = GetCellAttr(row, col);
    attr->GetSize( num_rows, num_cols );
    attr->DecRef();
}

wxGridCellRenderer* wxGrid::GetCellRenderer(int row, int col)
{
    wxGridCellAttr* attr = GetCellAttr(row, col);
    wxGridCellRenderer* renderer = attr->GetRenderer(this, row, col);
    attr->DecRef();

    return renderer;
}

wxGridCellEditor* wxGrid::GetCellEditor(int row, int col)
{
    wxGridCellAttr* attr = GetCellAttr(row, col);
    wxGridCellEditor* editor = attr->GetEditor(this, row, col);
    attr->DecRef();

    return editor;
}

bool wxGrid::IsReadOnly(int row, int col) const
{
    wxGridCellAttr* attr = GetCellAttr(row, col);
    bool isReadOnly = attr->IsReadOnly();
    attr->DecRef();

    return isReadOnly;
}

// ----------------------------------------------------------------------------
// attribute support: cache, automatic provider creation, ...
// ----------------------------------------------------------------------------

bool wxGrid::CanHaveAttributes()
{
    if ( !m_table )
    {
        return false;
    }

    return m_table->CanHaveAttributes();
}

void wxGrid::ClearAttrCache()
{
    if ( m_attrCache.row != -1 )
    {
        wxGridCellAttr *oldAttr = m_attrCache.attr;
        m_attrCache.attr = NULL;
        m_attrCache.row = -1;
        // wxSafeDecRec(...) might cause event processing that accesses
        // the cached attribute, if one exists (e.g. by deleting the
        // editor stored within the attribute). Therefore it is important
        // to invalidate the cache  before calling wxSafeDecRef!
        wxSafeDecRef(oldAttr);
    }
}

void wxGrid::CacheAttr(int row, int col, wxGridCellAttr *attr) const
{
    if ( attr != NULL )
    {
        wxGrid *self = (wxGrid *)this;  // const_cast

        self->ClearAttrCache();
        self->m_attrCache.row = row;
        self->m_attrCache.col = col;
        self->m_attrCache.attr = attr;
        wxSafeIncRef(attr);
    }
}

bool wxGrid::LookupAttr(int row, int col, wxGridCellAttr **attr) const
{
    if ( row == m_attrCache.row && col == m_attrCache.col )
    {
        *attr = m_attrCache.attr;
        wxSafeIncRef(m_attrCache.attr);

#ifdef DEBUG_ATTR_CACHE
        gs_nAttrCacheHits++;
#endif

        return true;
    }
    else
    {
#ifdef DEBUG_ATTR_CACHE
        gs_nAttrCacheMisses++;
#endif

        return false;
    }
}

wxGridCellAttr *wxGrid::GetCellAttr(int row, int col) const
{
    wxGridCellAttr *attr = NULL;
    // Additional test to avoid looking at the cache e.g. for
    // wxNoCellCoords, as this will confuse memory management.
    if ( row >= 0 )
    {
        if ( !LookupAttr(row, col, &attr) )
        {
            attr = m_table ? m_table->GetAttr(row, col, wxGridCellAttr::Any)
                           : (wxGridCellAttr *)NULL;
            CacheAttr(row, col, attr);
        }
    }

    if (attr)
    {
        attr->SetDefAttr(m_defaultCellAttr);
    }
    else
    {
        attr = m_defaultCellAttr;
        attr->IncRef();
    }

    return attr;
}

wxGridCellAttr *wxGrid::GetOrCreateCellAttr(int row, int col) const
{
    wxGridCellAttr *attr = (wxGridCellAttr *)NULL;
    bool canHave = ((wxGrid*)this)->CanHaveAttributes();

    wxCHECK_MSG( canHave, attr, _T("Cell attributes not allowed"));
    wxCHECK_MSG( m_table, attr, _T("must have a table") );

    attr = m_table->GetAttr(row, col, wxGridCellAttr::Cell);
    if ( !attr )
    {
        attr = new wxGridCellAttr(m_defaultCellAttr);

        // artificially inc the ref count to match DecRef() in caller
        attr->IncRef();
        m_table->SetAttr(attr, row, col);
    }

    return attr;
}

// ----------------------------------------------------------------------------
// setting column attributes (wrappers around SetColAttr)
// ----------------------------------------------------------------------------

void wxGrid::SetColFormatBool(int col)
{
    SetColFormatCustom(col, wxGRID_VALUE_BOOL);
}

void wxGrid::SetColFormatNumber(int col)
{
    SetColFormatCustom(col, wxGRID_VALUE_NUMBER);
}

void wxGrid::SetColFormatFloat(int col, int width, int precision)
{
    wxString typeName = wxGRID_VALUE_FLOAT;
    if ( (width != -1) || (precision != -1) )
    {
        typeName << _T(':') << width << _T(',') << precision;
    }

    SetColFormatCustom(col, typeName);
}

void wxGrid::SetColFormatCustom(int col, const wxString& typeName)
{
    wxGridCellAttr *attr = m_table->GetAttr(-1, col, wxGridCellAttr::Col );
    if (!attr)
        attr = new wxGridCellAttr;
    wxGridCellRenderer *renderer = GetDefaultRendererForType(typeName);
    attr->SetRenderer(renderer);

    SetColAttr(col, attr);

}

// ----------------------------------------------------------------------------
// setting cell attributes: this is forwarded to the table
// ----------------------------------------------------------------------------

void wxGrid::SetAttr(int row, int col, wxGridCellAttr *attr)
{
    if ( CanHaveAttributes() )
    {
        m_table->SetAttr(attr, row, col);
        ClearAttrCache();
    }
    else
    {
        wxSafeDecRef(attr);
    }
}

void wxGrid::SetRowAttr(int row, wxGridCellAttr *attr)
{
    if ( CanHaveAttributes() )
    {
        m_table->SetRowAttr(attr, row);
        ClearAttrCache();
    }
    else
    {
        wxSafeDecRef(attr);
    }
}

void wxGrid::SetColAttr(int col, wxGridCellAttr *attr)
{
    if ( CanHaveAttributes() )
    {
        m_table->SetColAttr(attr, col);
        ClearAttrCache();
    }
    else
    {
        wxSafeDecRef(attr);
    }
}

void wxGrid::SetCellBackgroundColour( int row, int col, const wxColour& colour )
{
    if ( CanHaveAttributes() )
    {
        wxGridCellAttr *attr = GetOrCreateCellAttr(row, col);
        attr->SetBackgroundColour(colour);
        attr->DecRef();
    }
}

void wxGrid::SetCellTextColour( int row, int col, const wxColour& colour )
{
    if ( CanHaveAttributes() )
    {
        wxGridCellAttr *attr = GetOrCreateCellAttr(row, col);
        attr->SetTextColour(colour);
        attr->DecRef();
    }
}

void wxGrid::SetCellFont( int row, int col, const wxFont& font )
{
    if ( CanHaveAttributes() )
    {
        wxGridCellAttr *attr = GetOrCreateCellAttr(row, col);
        attr->SetFont(font);
        attr->DecRef();
    }
}

void wxGrid::SetCellAlignment( int row, int col, int horiz, int vert )
{
    if ( CanHaveAttributes() )
    {
        wxGridCellAttr *attr = GetOrCreateCellAttr(row, col);
        attr->SetAlignment(horiz, vert);
        attr->DecRef();
    }
}

void wxGrid::SetCellOverflow( int row, int col, bool allow )
{
    if ( CanHaveAttributes() )
    {
        wxGridCellAttr *attr = GetOrCreateCellAttr(row, col);
        attr->SetOverflow(allow);
        attr->DecRef();
    }
}

void wxGrid::SetCellSize( int row, int col, int num_rows, int num_cols )
{
    if ( CanHaveAttributes() )
    {
        int cell_rows, cell_cols;

        wxGridCellAttr *attr = GetOrCreateCellAttr(row, col);
        attr->GetSize(&cell_rows, &cell_cols);
        attr->SetSize(num_rows, num_cols);
        attr->DecRef();

        // Cannot set the size of a cell to 0 or negative values
        // While it is perfectly legal to do that, this function cannot
        // handle all the possibilies, do it by hand by getting the CellAttr.
        // You can only set the size of a cell to 1,1 or greater with this fn
        wxASSERT_MSG( !((cell_rows < 1) || (cell_cols < 1)),
                      wxT("wxGrid::SetCellSize setting cell size that is already part of another cell"));
        wxASSERT_MSG( !((num_rows < 1) || (num_cols < 1)),
                      wxT("wxGrid::SetCellSize setting cell size to < 1"));

        // if this was already a multicell then "turn off" the other cells first
        if ((cell_rows > 1) || (cell_cols > 1))
        {
            int i, j;
            for (j=row; j < row + cell_rows; j++)
            {
                for (i=col; i < col + cell_cols; i++)
                {
                    if ((i != col) || (j != row))
                    {
                        wxGridCellAttr *attr_stub = GetOrCreateCellAttr(j, i);
                        attr_stub->SetSize( 1, 1 );
                        attr_stub->DecRef();
                    }
                }
            }
        }

        // mark the cells that will be covered by this cell to
        // negative or zero values to point back at this cell
        if (((num_rows > 1) || (num_cols > 1)) && (num_rows >= 1) && (num_cols >= 1))
        {
            int i, j;
            for (j=row; j < row + num_rows; j++)
            {
                for (i=col; i < col + num_cols; i++)
                {
                    if ((i != col) || (j != row))
                    {
                        wxGridCellAttr *attr_stub = GetOrCreateCellAttr(j, i);
                        attr_stub->SetSize( row - j, col - i );
                        attr_stub->DecRef();
                    }
                }
            }
        }
    }
}

void wxGrid::SetCellRenderer(int row, int col, wxGridCellRenderer *renderer)
{
    if ( CanHaveAttributes() )
    {
        wxGridCellAttr *attr = GetOrCreateCellAttr(row, col);
        attr->SetRenderer(renderer);
        attr->DecRef();
    }
}

void wxGrid::SetCellEditor(int row, int col, wxGridCellEditor* editor)
{
    if ( CanHaveAttributes() )
    {
        wxGridCellAttr *attr = GetOrCreateCellAttr(row, col);
        attr->SetEditor(editor);
        attr->DecRef();
    }
}

void wxGrid::SetReadOnly(int row, int col, bool isReadOnly)
{
    if ( CanHaveAttributes() )
    {
        wxGridCellAttr *attr = GetOrCreateCellAttr(row, col);
        attr->SetReadOnly(isReadOnly);
        attr->DecRef();
    }
}

// ----------------------------------------------------------------------------
// Data type registration
// ----------------------------------------------------------------------------

void wxGrid::RegisterDataType(const wxString& typeName,
                              wxGridCellRenderer* renderer,
                              wxGridCellEditor* editor)
{
    m_typeRegistry->RegisterDataType(typeName, renderer, editor);
}


wxGridCellEditor * wxGrid::GetDefaultEditorForCell(int row, int col) const
{
    wxString typeName = m_table->GetTypeName(row, col);
    return GetDefaultEditorForType(typeName);
}

wxGridCellRenderer * wxGrid::GetDefaultRendererForCell(int row, int col) const
{
    wxString typeName = m_table->GetTypeName(row, col);
    return GetDefaultRendererForType(typeName);
}

wxGridCellEditor * wxGrid::GetDefaultEditorForType(const wxString& typeName) const
{
    int index = m_typeRegistry->FindOrCloneDataType(typeName);
    if ( index == wxNOT_FOUND )
    {
        wxString errStr;

        errStr.Printf(wxT("Unknown data type name [%s]"), typeName.c_str());
        wxFAIL_MSG(errStr.c_str());

        return NULL;
    }

    return m_typeRegistry->GetEditor(index);
}

wxGridCellRenderer * wxGrid::GetDefaultRendererForType(const wxString& typeName) const
{
    int index = m_typeRegistry->FindOrCloneDataType(typeName);
    if ( index == wxNOT_FOUND )
    {
        wxString errStr;

        errStr.Printf(wxT("Unknown data type name [%s]"), typeName.c_str());
        wxFAIL_MSG(errStr.c_str());

        return NULL;
    }

    return m_typeRegistry->GetRenderer(index);
}

// ----------------------------------------------------------------------------
// row/col size
// ----------------------------------------------------------------------------

void wxGrid::EnableDragRowSize( bool enable )
{
    m_canDragRowSize = enable;
}

void wxGrid::EnableDragColSize( bool enable )
{
    m_canDragColSize = enable;
}

void wxGrid::EnableDragGridSize( bool enable )
{
    m_canDragGridSize = enable;
}

void wxGrid::EnableDragCell( bool enable )
{
    m_canDragCell = enable;
}

void wxGrid::SetDefaultRowSize( int height, bool resizeExistingRows )
{
    m_defaultRowHeight = wxMax( height, m_minAcceptableRowHeight );

    if ( resizeExistingRows )
    {
        // since we are resizing all rows to the default row size,
        // we can simply clear the row heights and row bottoms
        // arrays (which also allows us to take advantage of
        // some speed optimisations)
        m_rowHeights.Empty();
        m_rowBottoms.Empty();
        if ( !GetBatchCount() )
            CalcDimensions();
    }
}

void wxGrid::SetRowSize( int row, int height )
{
    wxCHECK_RET( row >= 0 && row < m_numRows, _T("invalid row index") );

    // if < 0 then calculate new height from label
    if ( height < 0 )
    {
        long w, h;
        wxArrayString lines;
        wxClientDC dc(m_rowLabelWin);
        dc.SetFont(GetLabelFont());
        StringToLines(GetRowLabelValue( row ), lines);
        GetTextBoxSize( dc, lines, &w, &h );
        //check that it is not less than the minimal height
        height = wxMax(h, GetRowMinimalAcceptableHeight());
    }

    // See comment in SetColSize
    if ( height < GetRowMinimalAcceptableHeight())
        return;

    if ( m_rowHeights.IsEmpty() )
    {
        // need to really create the array
        InitRowHeights();
    }

    int h = wxMax( 0, height );
    int diff = h - m_rowHeights[row];

    m_rowHeights[row] = h;
    for ( int i = row; i < m_numRows; i++ )
    {
        m_rowBottoms[i] += diff;
    }

    if ( !GetBatchCount() )
        CalcDimensions();
}

void wxGrid::SetDefaultColSize( int width, bool resizeExistingCols )
{
    m_defaultColWidth = wxMax( width, m_minAcceptableColWidth );

    if ( resizeExistingCols )
    {
        // since we are resizing all columns to the default column size,
        // we can simply clear the col widths and col rights
        // arrays (which also allows us to take advantage of
        // some speed optimisations)
        m_colWidths.Empty();
        m_colRights.Empty();
        if ( !GetBatchCount() )
            CalcDimensions();
    }
}

void wxGrid::SetColSize( int col, int width )
{
    wxCHECK_RET( col >= 0 && col < m_numCols, _T("invalid column index") );

    // if < 0 then calculate new width from label
    if ( width < 0 )
    {
        long w, h;
        wxArrayString lines;
        wxClientDC dc(m_colLabelWin);
        dc.SetFont(GetLabelFont());
        StringToLines(GetColLabelValue(col), lines);
        if ( GetColLabelTextOrientation() == wxHORIZONTAL )
            GetTextBoxSize( dc, lines, &w, &h );
        else
            GetTextBoxSize( dc, lines, &h, &w );
        width = w + 6;
        //check that it is not less than the minimal width
        width = wxMax(width, GetColMinimalAcceptableWidth());
    }

    // should we check that it's bigger than GetColMinimalWidth(col) here?
    //                                                                 (VZ)
    // No, because it is reasonable to assume the library user know's
    // what he is doing. However we should test against the weaker
    // constraint of minimalAcceptableWidth, as this breaks rendering
    //
    // This test then fixes sf.net bug #645734

    if ( width < GetColMinimalAcceptableWidth() )
        return;

    if ( m_colWidths.IsEmpty() )
    {
        // need to really create the array
        InitColWidths();
    }

    int w = wxMax( 0, width );
    int diff = w - m_colWidths[col];
    m_colWidths[col] = w;

    for ( int colPos = GetColPos(col); colPos < m_numCols; colPos++ )
    {
        m_colRights[GetColAt(colPos)] += diff;
    }

    if ( !GetBatchCount() )
        CalcDimensions();
}

void wxGrid::SetColMinimalWidth( int col, int width )
{
    if (width > GetColMinimalAcceptableWidth())
    {
        wxLongToLongHashMap::key_type key = (wxLongToLongHashMap::key_type)col;
        m_colMinWidths[key] = width;
    }
}

void wxGrid::SetRowMinimalHeight( int row, int width )
{
    if (width > GetRowMinimalAcceptableHeight())
    {
        wxLongToLongHashMap::key_type key = (wxLongToLongHashMap::key_type)row;
        m_rowMinHeights[key] = width;
    }
}

int wxGrid::GetColMinimalWidth(int col) const
{
    wxLongToLongHashMap::key_type key = (wxLongToLongHashMap::key_type)col;
    wxLongToLongHashMap::const_iterator it = m_colMinWidths.find(key);

    return it != m_colMinWidths.end() ? (int)it->second : m_minAcceptableColWidth;
}

int wxGrid::GetRowMinimalHeight(int row) const
{
    wxLongToLongHashMap::key_type key = (wxLongToLongHashMap::key_type)row;
    wxLongToLongHashMap::const_iterator it = m_rowMinHeights.find(key);

    return it != m_rowMinHeights.end() ? (int)it->second : m_minAcceptableRowHeight;
}

void wxGrid::SetColMinimalAcceptableWidth( int width )
{
    // We do allow a width of 0 since this gives us
    // an easy way to temporarily hiding columns.
    if ( width >= 0 )
        m_minAcceptableColWidth = width;
}

void wxGrid::SetRowMinimalAcceptableHeight( int height )
{
    // We do allow a height of 0 since this gives us
    // an easy way to temporarily hiding rows.
    if ( height >= 0 )
        m_minAcceptableRowHeight = height;
}

int  wxGrid::GetColMinimalAcceptableWidth() const
{
    return m_minAcceptableColWidth;
}

int  wxGrid::GetRowMinimalAcceptableHeight() const
{
    return m_minAcceptableRowHeight;
}

// ----------------------------------------------------------------------------
// auto sizing
// ----------------------------------------------------------------------------

void wxGrid::AutoSizeColOrRow( int colOrRow, bool setAsMin, bool column )
{
    wxClientDC dc(m_gridWin);

    // cancel editing of cell
    HideCellEditControl();
    SaveEditControlValue();

    // init both of them to avoid compiler warnings, even if we only need one
    int row = -1,
        col = -1;
    if ( column )
        col = colOrRow;
    else
        row = colOrRow;

    wxCoord extent, extentMax = 0;
    int max = column ? m_numRows : m_numCols;
    for ( int rowOrCol = 0; rowOrCol < max; rowOrCol++ )
    {
        if ( column )
            row = rowOrCol;
        else
            col = rowOrCol;

        wxGridCellAttr *attr = GetCellAttr(row, col);
        wxGridCellRenderer *renderer = attr->GetRenderer(this, row, col);
        if ( renderer )
        {
            wxSize size = renderer->GetBestSize(*this, *attr, dc, row, col);
            extent = column ? size.x : size.y;
            if ( extent > extentMax )
                extentMax = extent;

            renderer->DecRef();
        }

        attr->DecRef();
    }

    // now also compare with the column label extent
    wxCoord w, h;
    dc.SetFont( GetLabelFont() );

    if ( column )
    {
        dc.GetMultiLineTextExtent( GetColLabelValue(col), &w, &h );
        if ( GetColLabelTextOrientation() == wxVERTICAL )
            w = h;
    }
    else
        dc.GetMultiLineTextExtent( GetRowLabelValue(row), &w, &h );

    extent = column ? w : h;
    if ( extent > extentMax )
        extentMax = extent;

    if ( !extentMax )
    {
        // empty column - give default extent (notice that if extentMax is less
        // than default extent but != 0, it's OK)
        extentMax = column ? m_defaultColWidth : m_defaultRowHeight;
    }
    else
    {
        if ( column )
            // leave some space around text
            extentMax += 10;
        else
            extentMax += 6;
    }

    if ( column )
    {
        // Ensure automatic width is not less than minimal width. See the
        // comment in SetColSize() for explanation of why this isn't done
        // in SetColSize().
        if ( !setAsMin )
            extentMax = wxMax(extentMax, GetColMinimalWidth(col));

        SetColSize( col, extentMax );
        if ( !GetBatchCount() )
        {
            int cw, ch, dummy;
            m_gridWin->GetClientSize( &cw, &ch );
            wxRect rect ( CellToRect( 0, col ) );
            rect.y = 0;
            CalcScrolledPosition(rect.x, 0, &rect.x, &dummy);
            rect.width = cw - rect.x;
            rect.height = m_colLabelHeight;
            m_colLabelWin->Refresh( true, &rect );
        }
    }
    else
    {
        // Ensure automatic width is not less than minimal height. See the
        // comment in SetColSize() for explanation of why this isn't done
        // in SetRowSize().
        if ( !setAsMin )
            extentMax = wxMax(extentMax, GetRowMinimalHeight(row));

        SetRowSize(row, extentMax);
        if ( !GetBatchCount() )
        {
            int cw, ch, dummy;
            m_gridWin->GetClientSize( &cw, &ch );
            wxRect rect( CellToRect( row, 0 ) );
            rect.x = 0;
            CalcScrolledPosition(0, rect.y, &dummy, &rect.y);
            rect.width = m_rowLabelWidth;
            rect.height = ch - rect.y;
            m_rowLabelWin->Refresh( true, &rect );
        }
    }

    if ( setAsMin )
    {
        if ( column )
            SetColMinimalWidth(col, extentMax);
        else
            SetRowMinimalHeight(row, extentMax);
    }
}

wxCoord wxGrid::CalcColOrRowLabelAreaMinSize(bool column)
{
    // calculate size for the rows or columns?
    const bool calcRows = !column;

    wxClientDC dc(calcRows ? GetGridRowLabelWindow()
                           : GetGridColLabelWindow());
    dc.SetFont(GetLabelFont());

    // which dimension should we take into account for calculations?
    //
    // for columns, the text can be only horizontal so it's easy but for rows
    // we also have to take into account the text orientation
    const bool
        useWidth = calcRows || (GetColLabelTextOrientation() == wxVERTICAL);

    wxArrayString lines;
    wxCoord extentMax = 0;

    const int numRowsOrCols = calcRows ? m_numRows : m_numCols;
    for ( int rowOrCol = 0; rowOrCol < numRowsOrCols; rowOrCol++ )
    {
        lines.Clear();
        // NB: extra parentheses needed to avoid bcc 5.82 compilation errors
        StringToLines((calcRows ? GetRowLabelValue(rowOrCol)
                                : GetColLabelValue(rowOrCol)),
                      lines);

        long w, h;
        GetTextBoxSize(dc, lines, &w, &h);

        const wxCoord extent = useWidth ? w : h;
        if ( extent > extentMax )
            extentMax = extent;
    }

    if ( !extentMax )
    {
        // empty column - give default extent (notice that if extentMax is less
        // than default extent but != 0, it's OK)
        extentMax = calcRows ? GetDefaultRowLabelSize()
                             : GetDefaultColLabelSize();
    }

    // leave some space around text (taken from AutoSizeColOrRow)
    if ( calcRows )
        extentMax += 10;
    else
        extentMax += 6;

    return extentMax;
}

int wxGrid::SetOrCalcColumnSizes(bool calcOnly, bool setAsMin)
{
    int width = m_rowLabelWidth;

    if ( !calcOnly )
        BeginBatch();

    for ( int col = 0; col < m_numCols; col++ )
    {
        if ( !calcOnly )
            AutoSizeColumn(col, setAsMin);

        width += GetColWidth(col);
    }

    if ( !calcOnly )
        EndBatch();

    return width;
}

int wxGrid::SetOrCalcRowSizes(bool calcOnly, bool setAsMin)
{
    int height = m_colLabelHeight;

    if ( !calcOnly )
        BeginBatch();

    for ( int row = 0; row < m_numRows; row++ )
    {
        if ( !calcOnly )
            AutoSizeRow(row, setAsMin);

        height += GetRowHeight(row);
    }

    if ( !calcOnly )
        EndBatch();

    return height;
}

void wxGrid::AutoSize()
{
    BeginBatch();

    // we need to round up the size of the scrollable area to a multiple of
    // scroll step to ensure that we don't get the scrollbars when we're sized
    // exactly to fit our contents
    wxSize size(SetOrCalcColumnSizes(false) - m_rowLabelWidth + m_extraWidth,
                SetOrCalcRowSizes(false) - m_colLabelHeight + m_extraHeight);
    wxSize sizeFit(GetScrollX(size.x) * GetScrollLineX(),
                   GetScrollY(size.y) * GetScrollLineY());

    // distribute the extra space between the columns/rows to avoid having
    // extra white space
    wxCoord diff = sizeFit.x - size.x;
    if ( diff && m_numCols )
    {
        // try to resize the columns uniformly
        wxCoord diffPerCol = diff / m_numCols;
        if ( diffPerCol )
        {
            for ( int col = 0; col < m_numCols; col++ )
            {
                SetColSize(col, GetColWidth(col) + diffPerCol);
            }
        }

        // add remaining amount to the last columns
        diff -= diffPerCol * m_numCols;
        if ( diff )
        {
            for ( int col = m_numCols - 1; col >= m_numCols - diff; col-- )
            {
                SetColSize(col, GetColWidth(col) + 1);
            }
        }
    }

    // same for rows
    diff = sizeFit.y - size.y;
    if ( diff && m_numRows )
    {
        // try to resize the columns uniformly
        wxCoord diffPerRow = diff / m_numRows;
        if ( diffPerRow )
        {
            for ( int row = 0; row < m_numRows; row++ )
            {
                SetRowSize(row, GetRowHeight(row) + diffPerRow);
            }
        }

        // add remaining amount to the last rows
        diff -= diffPerRow * m_numRows;
        if ( diff )
        {
            for ( int row = m_numRows - 1; row >= m_numRows - diff; row-- )
            {
                SetRowSize(row, GetRowHeight(row) + 1);
            }
        }
    }

    // we know that we're not going to have scrollbars so disable them now to
    // avoid trouble in SetClientSize() which can otherwise set the correct
    // client size but also leave space for (not needed any more) scrollbars
    SetScrollbars(0, 0, 0, 0, 0, 0, true);
    SetClientSize(sizeFit.x + m_rowLabelWidth, sizeFit.y + m_colLabelHeight);

    EndBatch();
}

void wxGrid::AutoSizeRowLabelSize( int row )
{
    // Hide the edit control, so it
    // won't interfere with drag-shrinking.
    if ( IsCellEditControlShown() )
    {
        HideCellEditControl();
        SaveEditControlValue();
    }

    // autosize row height depending on label text
    SetRowSize(row, -1);
    ForceRefresh();
}

void wxGrid::AutoSizeColLabelSize( int col )
{
    // Hide the edit control, so it
    // won't interfere with drag-shrinking.
    if ( IsCellEditControlShown() )
    {
        HideCellEditControl();
        SaveEditControlValue();
    }

    // autosize column width depending on label text
    SetColSize(col, -1);
    ForceRefresh();
}

wxSize wxGrid::DoGetBestSize() const
{
    wxGrid *self = (wxGrid *)this;  // const_cast

    // we do the same as in AutoSize() here with the exception that we don't
    // change the column/row sizes, only calculate them
    wxSize size(self->SetOrCalcColumnSizes(true) - m_rowLabelWidth + m_extraWidth,
                self->SetOrCalcRowSizes(true) - m_colLabelHeight + m_extraHeight);
    wxSize sizeFit(GetScrollX(size.x) * GetScrollLineX(),
                   GetScrollY(size.y) * GetScrollLineY());

    // NOTE: This size should be cached, but first we need to add calls to
    // InvalidateBestSize everywhere that could change the results of this
    // calculation.
    // CacheBestSize(size);

    return wxSize(sizeFit.x + m_rowLabelWidth, sizeFit.y + m_colLabelHeight)
            + GetWindowBorderSize();
}

void wxGrid::Fit()
{
    AutoSize();
}

wxPen& wxGrid::GetDividerPen() const
{
    return wxNullPen;
}

// ----------------------------------------------------------------------------
// cell value accessor functions
// ----------------------------------------------------------------------------

void wxGrid::SetCellValue( int row, int col, const wxString& s )
{
    if ( m_table )
    {
        m_table->SetValue( row, col, s );
        if ( !GetBatchCount() )
        {
            int dummy;
            wxRect rect( CellToRect( row, col ) );
            rect.x = 0;
            rect.width = m_gridWin->GetClientSize().GetWidth();
            CalcScrolledPosition(0, rect.y, &dummy, &rect.y);
            m_gridWin->Refresh( false, &rect );
        }

        if ( m_currentCellCoords.GetRow() == row &&
             m_currentCellCoords.GetCol() == col &&
             IsCellEditControlShown())
             // Note: If we are using IsCellEditControlEnabled,
             // this interacts badly with calling SetCellValue from
             // an EVT_GRID_CELL_CHANGE handler.
        {
            HideCellEditControl();
            ShowCellEditControl(); // will reread data from table
        }
    }
}

// ----------------------------------------------------------------------------
// block, row and column selection
// ----------------------------------------------------------------------------

void wxGrid::SelectRow( int row, bool addToSelected )
{
    if ( IsSelection() && !addToSelected )
        ClearSelection();

    if ( m_selection )
        m_selection->SelectRow( row, false, addToSelected );
}

void wxGrid::SelectCol( int col, bool addToSelected )
{
    if ( IsSelection() && !addToSelected )
        ClearSelection();

    if ( m_selection )
        m_selection->SelectCol( col, false, addToSelected );
}

void wxGrid::SelectBlock( int topRow, int leftCol, int bottomRow, int rightCol,
                          bool addToSelected )
{
    if ( IsSelection() && !addToSelected )
        ClearSelection();

    if ( m_selection )
        m_selection->SelectBlock( topRow, leftCol, bottomRow, rightCol,
                                  false, addToSelected );
}

void wxGrid::SelectAll()
{
    if ( m_numRows > 0 && m_numCols > 0 )
    {
        if ( m_selection )
            m_selection->SelectBlock( 0, 0, m_numRows - 1, m_numCols - 1 );
    }
}

// ----------------------------------------------------------------------------
// cell, row and col deselection
// ----------------------------------------------------------------------------

void wxGrid::DeselectRow( int row )
{
    if ( !m_selection )
        return;

    if ( m_selection->GetSelectionMode() == wxGrid::wxGridSelectRows )
    {
        if ( m_selection->IsInSelection(row, 0 ) )
            m_selection->ToggleCellSelection(row, 0);
    }
    else
    {
        int nCols = GetNumberCols();
        for ( int i = 0; i < nCols; i++ )
        {
            if ( m_selection->IsInSelection(row, i ) )
                m_selection->ToggleCellSelection(row, i);
        }
    }
}

void wxGrid::DeselectCol( int col )
{
    if ( !m_selection )
        return;

    if ( m_selection->GetSelectionMode() == wxGrid::wxGridSelectColumns )
    {
        if ( m_selection->IsInSelection(0, col ) )
            m_selection->ToggleCellSelection(0, col);
    }
    else
    {
        int nRows = GetNumberRows();
        for ( int i = 0; i < nRows; i++ )
        {
            if ( m_selection->IsInSelection(i, col ) )
                m_selection->ToggleCellSelection(i, col);
        }
    }
}

void wxGrid::DeselectCell( int row, int col )
{
    if ( m_selection && m_selection->IsInSelection(row, col) )
        m_selection->ToggleCellSelection(row, col);
}

bool wxGrid::IsSelection()
{
    return ( m_selection && (m_selection->IsSelection() ||
             ( m_selectingTopLeft != wxGridNoCellCoords &&
               m_selectingBottomRight != wxGridNoCellCoords) ) );
}

bool wxGrid::IsInSelection( int row, int col ) const
{
    return ( m_selection && (m_selection->IsInSelection( row, col ) ||
             ( row >= m_selectingTopLeft.GetRow() &&
               col >= m_selectingTopLeft.GetCol() &&
               row <= m_selectingBottomRight.GetRow() &&
               col <= m_selectingBottomRight.GetCol() )) );
}

wxGridCellCoordsArray wxGrid::GetSelectedCells() const
{
    if (!m_selection)
    {
        wxGridCellCoordsArray a;
        return a;
    }

    return m_selection->m_cellSelection;
}

wxGridCellCoordsArray wxGrid::GetSelectionBlockTopLeft() const
{
    if (!m_selection)
    {
        wxGridCellCoordsArray a;
        return a;
    }

    return m_selection->m_blockSelectionTopLeft;
}

wxGridCellCoordsArray wxGrid::GetSelectionBlockBottomRight() const
{
    if (!m_selection)
    {
        wxGridCellCoordsArray a;
        return a;
    }

    return m_selection->m_blockSelectionBottomRight;
}

wxArrayInt wxGrid::GetSelectedRows() const
{
    if (!m_selection)
    {
        wxArrayInt a;
        return a;
    }

    return m_selection->m_rowSelection;
}

wxArrayInt wxGrid::GetSelectedCols() const
{
    if (!m_selection)
    {
        wxArrayInt a;
        return a;
    }

    return m_selection->m_colSelection;
}

void wxGrid::ClearSelection()
{
    wxRect r1 = BlockToDeviceRect( m_selectingTopLeft, m_selectingBottomRight);
    wxRect r2 = BlockToDeviceRect( m_currentCellCoords, m_selectingKeyboard );
    m_selectingTopLeft =
    m_selectingBottomRight =
    m_selectingKeyboard = wxGridNoCellCoords;
    Refresh( false, &r1 );
    Refresh( false, &r2 );
    if ( m_selection )
        m_selection->ClearSelection();
}

// This function returns the rectangle that encloses the given block
// in device coords clipped to the client size of the grid window.
//
wxRect wxGrid::BlockToDeviceRect( const wxGridCellCoords& topLeft,
                                  const wxGridCellCoords& bottomRight )
{
    wxRect resultRect;
    wxRect tempCellRect = CellToRect(topLeft);
    if ( tempCellRect != wxGridNoCellRect )
    {
        resultRect = tempCellRect;
    }
    else
    {
        resultRect = wxRect(0, 0, 0, 0);
    }

    tempCellRect = CellToRect(bottomRight);
    if ( tempCellRect != wxGridNoCellRect )
    {
        resultRect += tempCellRect;
    }
    else
    {
        // If both inputs were "wxGridNoCellRect," then there's nothing to do.
        return wxGridNoCellRect;
    }

    // Ensure that left/right and top/bottom pairs are in order.
    int left = resultRect.GetLeft();
    int top = resultRect.GetTop();
    int right = resultRect.GetRight();
    int bottom = resultRect.GetBottom();

    int leftCol = topLeft.GetCol();
    int topRow = topLeft.GetRow();
    int rightCol = bottomRight.GetCol();
    int bottomRow = bottomRight.GetRow();

    if (left > right)
    {
        int tmp = left;
        left = right;
        right = tmp;

        tmp = leftCol;
        leftCol = rightCol;
        rightCol = tmp;
    }

    if (top > bottom)
    {
        int tmp = top;
        top = bottom;
        bottom = tmp;

        tmp = topRow;
        topRow = bottomRow;
        bottomRow = tmp;
    }

    // The following loop is ONLY necessary to detect and handle merged cells.
    int cw, ch;
    m_gridWin->GetClientSize( &cw, &ch );

    // Get the origin coordinates: notice that they will be negative if the
    // grid is scrolled downwards/to the right.
    int gridOriginX = 0;
    int gridOriginY = 0;
    CalcScrolledPosition(gridOriginX, gridOriginY, &gridOriginX, &gridOriginY);

    int onScreenLeftmostCol = internalXToCol(-gridOriginX);
    int onScreenUppermostRow = internalYToRow(-gridOriginY);

    int onScreenRightmostCol = internalXToCol(-gridOriginX + cw);
    int onScreenBottommostRow = internalYToRow(-gridOriginY + ch);

    // Bound our loop so that we only examine the portion of the selected block
    // that is shown on screen. Therefore, we compare the Top-Left block values
    // to the Top-Left screen values, and the Bottom-Right block values to the
    // Bottom-Right screen values, choosing appropriately.
    const int visibleTopRow = wxMax(topRow, onScreenUppermostRow);
    const int visibleBottomRow = wxMin(bottomRow, onScreenBottommostRow);
    const int visibleLeftCol = wxMax(leftCol, onScreenLeftmostCol);
    const int visibleRightCol = wxMin(rightCol, onScreenRightmostCol);

    for ( int j = visibleTopRow; j <= visibleBottomRow; j++ )
    {
        for ( int i = visibleLeftCol; i <= visibleRightCol; i++ )
        {
            if ( (j == visibleTopRow) || (j == visibleBottomRow) ||
                    (i == visibleLeftCol) || (i == visibleRightCol) )
            {
                tempCellRect = CellToRect( j, i );

                if (tempCellRect.x < left)
                    left = tempCellRect.x;
                if (tempCellRect.y < top)
                    top = tempCellRect.y;
                if (tempCellRect.x + tempCellRect.width > right)
                    right = tempCellRect.x + tempCellRect.width;
                if (tempCellRect.y + tempCellRect.height > bottom)
                    bottom = tempCellRect.y + tempCellRect.height;
            }
            else
            {
                i = visibleRightCol; // jump over inner cells.
            }
        }
    }

    // Convert to scrolled coords
    CalcScrolledPosition( left, top, &left, &top );
    CalcScrolledPosition( right, bottom, &right, &bottom );

    if (right < 0 || bottom < 0 || left > cw || top > ch)
        return wxRect(0,0,0,0);

    resultRect.SetLeft( wxMax(0, left) );
    resultRect.SetTop( wxMax(0, top) );
    resultRect.SetRight( wxMin(cw, right) );
    resultRect.SetBottom( wxMin(ch, bottom) );

    return resultRect;
}

// ----------------------------------------------------------------------------
// grid event classes
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS( wxGridEvent, wxNotifyEvent )

wxGridEvent::wxGridEvent( int id, wxEventType type, wxObject* obj,
                          int row, int col, int x, int y, bool sel,
                          bool control, bool shift, bool alt, bool meta )
        : wxNotifyEvent( type, id )
{
    m_row = row;
    m_col = col;
    m_x = x;
    m_y = y;
    m_selecting = sel;
    m_control = control;
    m_shift = shift;
    m_alt = alt;
    m_meta = meta;

    SetEventObject(obj);
}


IMPLEMENT_DYNAMIC_CLASS( wxGridSizeEvent, wxNotifyEvent )

wxGridSizeEvent::wxGridSizeEvent( int id, wxEventType type, wxObject* obj,
                                  int rowOrCol, int x, int y,
                                  bool control, bool shift, bool alt, bool meta )
        : wxNotifyEvent( type, id )
{
    m_rowOrCol = rowOrCol;
    m_x = x;
    m_y = y;
    m_control = control;
    m_shift = shift;
    m_alt = alt;
    m_meta = meta;

    SetEventObject(obj);
}


IMPLEMENT_DYNAMIC_CLASS( wxGridRangeSelectEvent, wxNotifyEvent )

wxGridRangeSelectEvent::wxGridRangeSelectEvent(int id, wxEventType type, wxObject* obj,
                                               const wxGridCellCoords& topLeft,
                                               const wxGridCellCoords& bottomRight,
                                               bool sel, bool control,
                                               bool shift, bool alt, bool meta )
        : wxNotifyEvent( type, id )
{
    m_topLeft = topLeft;
    m_bottomRight = bottomRight;
    m_selecting = sel;
    m_control = control;
    m_shift = shift;
    m_alt = alt;
    m_meta = meta;

    SetEventObject(obj);
}


IMPLEMENT_DYNAMIC_CLASS(wxGridEditorCreatedEvent, wxCommandEvent)

wxGridEditorCreatedEvent::wxGridEditorCreatedEvent(int id, wxEventType type,
                                                   wxObject* obj, int row,
                                                   int col, wxControl* ctrl)
    : wxCommandEvent(type, id)
{
    SetEventObject(obj);
    m_row = row;
    m_col = col;
    m_ctrl = ctrl;
}

#endif // wxUSE_GRID
