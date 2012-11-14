/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/grid.h
// Purpose:     wxGrid and related classes
// Author:      Michael Bedward (based on code by Julian Smart, Robin Dunn)
// Modified by: Santiago Palacios
// Created:     1/08/1999
// RCS-ID:      $Id: grid.h 66942 2011-02-17 12:30:56Z JS $
// Copyright:   (c) Michael Bedward
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_GRID_H_
#define _WX_GENERIC_GRID_H_

#include "wx/defs.h"

#if wxUSE_GRID

#include "wx/scrolwin.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

extern WXDLLIMPEXP_DATA_ADV(const wxChar) wxGridNameStr[];

// Default parameters for wxGrid
//
#define WXGRID_DEFAULT_NUMBER_ROWS            10
#define WXGRID_DEFAULT_NUMBER_COLS            10
#if defined(__WXMSW__) || defined(__WXGTK20__)
#define WXGRID_DEFAULT_ROW_HEIGHT             25
#else
#define WXGRID_DEFAULT_ROW_HEIGHT             30
#endif  // __WXMSW__
#define WXGRID_DEFAULT_COL_WIDTH              80
#define WXGRID_DEFAULT_COL_LABEL_HEIGHT       32
#define WXGRID_DEFAULT_ROW_LABEL_WIDTH        82
#define WXGRID_LABEL_EDGE_ZONE                 2
#define WXGRID_MIN_ROW_HEIGHT                 15
#define WXGRID_MIN_COL_WIDTH                  15
#define WXGRID_DEFAULT_SCROLLBAR_WIDTH        16

// type names for grid table values
#define wxGRID_VALUE_STRING     wxT("string")
#define wxGRID_VALUE_BOOL       wxT("bool")
#define wxGRID_VALUE_NUMBER     wxT("long")
#define wxGRID_VALUE_FLOAT      wxT("double")
#define wxGRID_VALUE_CHOICE     wxT("choice")

#define wxGRID_VALUE_TEXT wxGRID_VALUE_STRING
#define wxGRID_VALUE_LONG wxGRID_VALUE_NUMBER

#if wxABI_VERSION >= 20808
    // magic constant which tells (to some functions) to automatically
    // calculate the appropriate size
    #define wxGRID_AUTOSIZE (-1)
#endif // wxABI_VERSION >= 20808

// ----------------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_ADV wxGrid;
class WXDLLIMPEXP_FWD_ADV wxGridCellAttr;
class WXDLLIMPEXP_FWD_ADV wxGridCellAttrProviderData;
class WXDLLIMPEXP_FWD_ADV wxGridColLabelWindow;
class WXDLLIMPEXP_FWD_ADV wxGridCornerLabelWindow;
class WXDLLIMPEXP_FWD_ADV wxGridRowLabelWindow;
class WXDLLIMPEXP_FWD_ADV wxGridWindow;
class WXDLLIMPEXP_FWD_ADV wxGridTypeRegistry;
class WXDLLIMPEXP_FWD_ADV wxGridSelection;

class WXDLLIMPEXP_FWD_CORE wxCheckBox;
class WXDLLIMPEXP_FWD_CORE wxComboBox;
class WXDLLIMPEXP_FWD_CORE wxTextCtrl;
#if wxUSE_SPINCTRL
class WXDLLIMPEXP_FWD_CORE wxSpinCtrl;
#endif

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

#define wxSafeIncRef(p) if ( p ) (p)->IncRef()
#define wxSafeDecRef(p) if ( p ) (p)->DecRef()

// ----------------------------------------------------------------------------
// wxGridCellWorker: common base class for wxGridCellRenderer and
// wxGridCellEditor
//
// NB: this is more an implementation convenience than a design issue, so this
//     class is not documented and is not public at all
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxGridCellWorker : public wxClientDataContainer
{
public:
    wxGridCellWorker() { m_nRef = 1; }

    // this class is ref counted: it is created with ref count of 1, so
    // calling DecRef() once will delete it. Calling IncRef() allows to lock
    // it until the matching DecRef() is called
    void IncRef() { m_nRef++; }
    void DecRef() { if ( --m_nRef == 0 ) delete this; }

    // interpret renderer parameters: arbitrary string whose interpretatin is
    // left to the derived classes
    virtual void SetParameters(const wxString& params);

protected:
    // virtual dtor for any base class - private because only DecRef() can
    // delete us
    virtual ~wxGridCellWorker();

private:
    size_t m_nRef;

    // suppress the stupid gcc warning about the class having private dtor and
    // no friends
    friend class wxGridCellWorkerDummyFriend;
};

// ----------------------------------------------------------------------------
// wxGridCellRenderer: this class is responsible for actually drawing the cell
// in the grid. You may pass it to the wxGridCellAttr (below) to change the
// format of one given cell or to wxGrid::SetDefaultRenderer() to change the
// view of all cells. This is an ABC, you will normally use one of the
// predefined derived classes or derive your own class from it.
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxGridCellRenderer : public wxGridCellWorker
{
public:
    // draw the given cell on the provided DC inside the given rectangle
    // using the style specified by the attribute and the default or selected
    // state corresponding to the isSelected value.
    //
    // this pure virtual function has a default implementation which will
    // prepare the DC using the given attribute: it will draw the rectangle
    // with the bg colour from attr and set the text colour and font
    virtual void Draw(wxGrid& grid,
                      wxGridCellAttr& attr,
                      wxDC& dc,
                      const wxRect& rect,
                      int row, int col,
                      bool isSelected) = 0;

    // get the preferred size of the cell for its contents
    virtual wxSize GetBestSize(wxGrid& grid,
                               wxGridCellAttr& attr,
                               wxDC& dc,
                               int row, int col) = 0;

    // create a new object which is the copy of this one
    virtual wxGridCellRenderer *Clone() const = 0;
};

// the default renderer for the cells containing string data
class WXDLLIMPEXP_ADV wxGridCellStringRenderer : public wxGridCellRenderer
{
public:
    // draw the string
    virtual void Draw(wxGrid& grid,
                      wxGridCellAttr& attr,
                      wxDC& dc,
                      const wxRect& rect,
                      int row, int col,
                      bool isSelected);

    // return the string extent
    virtual wxSize GetBestSize(wxGrid& grid,
                               wxGridCellAttr& attr,
                               wxDC& dc,
                               int row, int col);

    virtual wxGridCellRenderer *Clone() const
        { return new wxGridCellStringRenderer; }

protected:
    // set the text colours before drawing
    void SetTextColoursAndFont(const wxGrid& grid,
                               const wxGridCellAttr& attr,
                               wxDC& dc,
                               bool isSelected);

    // calc the string extent for given string/font
    wxSize DoGetBestSize(const wxGridCellAttr& attr,
                         wxDC& dc,
                         const wxString& text);
};

// the default renderer for the cells containing numeric (long) data
class WXDLLIMPEXP_ADV wxGridCellNumberRenderer : public wxGridCellStringRenderer
{
public:
    // draw the string right aligned
    virtual void Draw(wxGrid& grid,
                      wxGridCellAttr& attr,
                      wxDC& dc,
                      const wxRect& rect,
                      int row, int col,
                      bool isSelected);

    virtual wxSize GetBestSize(wxGrid& grid,
                               wxGridCellAttr& attr,
                               wxDC& dc,
                               int row, int col);

    virtual wxGridCellRenderer *Clone() const
        { return new wxGridCellNumberRenderer; }

protected:
    wxString GetString(const wxGrid& grid, int row, int col);
};

class WXDLLIMPEXP_ADV wxGridCellFloatRenderer : public wxGridCellStringRenderer
{
public:
    wxGridCellFloatRenderer(int width = -1, int precision = -1);

    // get/change formatting parameters
    int GetWidth() const { return m_width; }
    void SetWidth(int width) { m_width = width; m_format.clear(); }
    int GetPrecision() const { return m_precision; }
    void SetPrecision(int precision) { m_precision = precision; m_format.clear(); }

    // draw the string right aligned with given width/precision
    virtual void Draw(wxGrid& grid,
                      wxGridCellAttr& attr,
                      wxDC& dc,
                      const wxRect& rect,
                      int row, int col,
                      bool isSelected);

    virtual wxSize GetBestSize(wxGrid& grid,
                               wxGridCellAttr& attr,
                               wxDC& dc,
                               int row, int col);

    // parameters string format is "width[,precision]"
    virtual void SetParameters(const wxString& params);

    virtual wxGridCellRenderer *Clone() const;

protected:
    wxString GetString(const wxGrid& grid, int row, int col);

private:
    // formatting parameters
    int m_width,
        m_precision;

    wxString m_format;
};

// renderer for boolean fields
class WXDLLIMPEXP_ADV wxGridCellBoolRenderer : public wxGridCellRenderer
{
public:
    // draw a check mark or nothing
    virtual void Draw(wxGrid& grid,
                      wxGridCellAttr& attr,
                      wxDC& dc,
                      const wxRect& rect,
                      int row, int col,
                      bool isSelected);

    // return the checkmark size
    virtual wxSize GetBestSize(wxGrid& grid,
                               wxGridCellAttr& attr,
                               wxDC& dc,
                               int row, int col);

    virtual wxGridCellRenderer *Clone() const
        { return new wxGridCellBoolRenderer; }

private:
    static wxSize ms_sizeCheckMark;
};

// ----------------------------------------------------------------------------
// wxGridCellEditor:  This class is responsible for providing and manipulating
// the in-place edit controls for the grid.  Instances of wxGridCellEditor
// (actually, instances of derived classes since it is an ABC) can be
// associated with the cell attributes for individual cells, rows, columns, or
// even for the entire grid.
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxGridCellEditor : public wxGridCellWorker
{
public:
    wxGridCellEditor();

    bool IsCreated() { return m_control != NULL; }
    wxControl* GetControl() { return m_control; }
    void SetControl(wxControl* control) { m_control = control; }

    wxGridCellAttr* GetCellAttr() { return m_attr; }
    void SetCellAttr(wxGridCellAttr* attr) { m_attr = attr; }

    // Creates the actual edit control
    virtual void Create(wxWindow* parent,
                        wxWindowID id,
                        wxEvtHandler* evtHandler) = 0;

    // Size and position the edit control
    virtual void SetSize(const wxRect& rect);

    // Show or hide the edit control, use the specified attributes to set
    // colours/fonts for it
    virtual void Show(bool show, wxGridCellAttr *attr = (wxGridCellAttr *)NULL);

    // Draws the part of the cell not occupied by the control: the base class
    // version just fills it with background colour from the attribute
    virtual void PaintBackground(const wxRect& rectCell, wxGridCellAttr *attr);

    // Fetch the value from the table and prepare the edit control
    // to begin editing.  Set the focus to the edit control.
    virtual void BeginEdit(int row, int col, wxGrid* grid) = 0;

    // Complete the editing of the current cell. Returns true if the value has
    // changed.  If necessary, the control may be destroyed.
    virtual bool EndEdit(int row, int col, wxGrid* grid) = 0;

    // Reset the value in the control back to its starting value
    virtual void Reset() = 0;

    // return true to allow the given key to start editing: the base class
    // version only checks that the event has no modifiers. The derived
    // classes are supposed to do "if ( base::IsAcceptedKey() && ... )" in
    // their IsAcceptedKey() implementation, although, of course, it is not a
    // mandatory requirment.
    //
    // NB: if the key is F2 (special), editing will always start and this
    //     method will not be called at all (but StartingKey() will)
    virtual bool IsAcceptedKey(wxKeyEvent& event);

    // If the editor is enabled by pressing keys on the grid, this will be
    // called to let the editor do something about that first key if desired
    virtual void StartingKey(wxKeyEvent& event);

    // if the editor is enabled by clicking on the cell, this method will be
    // called
    virtual void StartingClick();

    // Some types of controls on some platforms may need some help
    // with the Return key.
    virtual void HandleReturn(wxKeyEvent& event);

    // Final cleanup
    virtual void Destroy();

    // create a new object which is the copy of this one
    virtual wxGridCellEditor *Clone() const = 0;

    // added GetValue so we can get the value which is in the control
    virtual wxString GetValue() const = 0;

protected:
    // the dtor is private because only DecRef() can delete us
    virtual ~wxGridCellEditor();

    // the control we show on screen
    wxControl*  m_control;

    // a temporary pointer to the attribute being edited
    wxGridCellAttr* m_attr;

    // if we change the colours/font of the control from the default ones, we
    // must restore the default later and we save them here between calls to
    // Show(true) and Show(false)
    wxColour m_colFgOld,
             m_colBgOld;
    wxFont m_fontOld;

    // suppress the stupid gcc warning about the class having private dtor and
    // no friends
    friend class wxGridCellEditorDummyFriend;

    DECLARE_NO_COPY_CLASS(wxGridCellEditor)
};

#if wxUSE_TEXTCTRL

// the editor for string/text data
class WXDLLIMPEXP_ADV wxGridCellTextEditor : public wxGridCellEditor
{
public:
    wxGridCellTextEditor();

    virtual void Create(wxWindow* parent,
                        wxWindowID id,
                        wxEvtHandler* evtHandler);
    virtual void SetSize(const wxRect& rect);

    virtual void PaintBackground(const wxRect& rectCell, wxGridCellAttr *attr);

    virtual bool IsAcceptedKey(wxKeyEvent& event);
    virtual void BeginEdit(int row, int col, wxGrid* grid);
    virtual bool EndEdit(int row, int col, wxGrid* grid);

    virtual void Reset();
    virtual void StartingKey(wxKeyEvent& event);
    virtual void HandleReturn(wxKeyEvent& event);

    // parameters string format is "max_width"
    virtual void SetParameters(const wxString& params);

    virtual wxGridCellEditor *Clone() const
        { return new wxGridCellTextEditor; }

    // added GetValue so we can get the value which is in the control
    virtual wxString GetValue() const;

protected:
    wxTextCtrl *Text() const { return (wxTextCtrl *)m_control; }

    // parts of our virtual functions reused by the derived classes
    void DoBeginEdit(const wxString& startValue);
    void DoReset(const wxString& startValue);

private:
    size_t   m_maxChars;        // max number of chars allowed
    wxString m_startValue;

    DECLARE_NO_COPY_CLASS(wxGridCellTextEditor)
};

// the editor for numeric (long) data
class WXDLLIMPEXP_ADV wxGridCellNumberEditor : public wxGridCellTextEditor
{
public:
    // allows to specify the range - if min == max == -1, no range checking is
    // done
    wxGridCellNumberEditor(int min = -1, int max = -1);

    virtual void Create(wxWindow* parent,
                        wxWindowID id,
                        wxEvtHandler* evtHandler);

    virtual bool IsAcceptedKey(wxKeyEvent& event);
    virtual void BeginEdit(int row, int col, wxGrid* grid);
    virtual bool EndEdit(int row, int col, wxGrid* grid);

    virtual void Reset();
    virtual void StartingKey(wxKeyEvent& event);

    // parameters string format is "min,max"
    virtual void SetParameters(const wxString& params);

    virtual wxGridCellEditor *Clone() const
        { return new wxGridCellNumberEditor(m_min, m_max); }

    // added GetValue so we can get the value which is in the control
    virtual wxString GetValue() const;

protected:
#if wxUSE_SPINCTRL
    wxSpinCtrl *Spin() const { return (wxSpinCtrl *)m_control; }
#endif

    // if HasRange(), we use wxSpinCtrl - otherwise wxTextCtrl
    bool HasRange() const
    {
#if wxUSE_SPINCTRL
        return m_min != m_max;
#else
        return false;
#endif
    }

    // string representation of m_valueOld
    wxString GetString() const
        { return wxString::Format(wxT("%ld"), m_valueOld); }

private:
    int m_min,
        m_max;

    long m_valueOld;

    DECLARE_NO_COPY_CLASS(wxGridCellNumberEditor)
};

// the editor for floating point numbers (double) data
class WXDLLIMPEXP_ADV wxGridCellFloatEditor : public wxGridCellTextEditor
{
public:
    wxGridCellFloatEditor(int width = -1, int precision = -1);

    virtual void Create(wxWindow* parent,
                        wxWindowID id,
                        wxEvtHandler* evtHandler);

    virtual bool IsAcceptedKey(wxKeyEvent& event);
    virtual void BeginEdit(int row, int col, wxGrid* grid);
    virtual bool EndEdit(int row, int col, wxGrid* grid);

    virtual void Reset();
    virtual void StartingKey(wxKeyEvent& event);

    virtual wxGridCellEditor *Clone() const
        { return new wxGridCellFloatEditor(m_width, m_precision); }

    // parameters string format is "width,precision"
    virtual void SetParameters(const wxString& params);

protected:
    // string representation of m_valueOld
    wxString GetString() const;

private:
    int m_width,
        m_precision;
    double m_valueOld;

    DECLARE_NO_COPY_CLASS(wxGridCellFloatEditor)
};

#endif // wxUSE_TEXTCTRL

#if wxUSE_CHECKBOX

// the editor for boolean data
class WXDLLIMPEXP_ADV wxGridCellBoolEditor : public wxGridCellEditor
{
public:
    wxGridCellBoolEditor() { }

    virtual void Create(wxWindow* parent,
                        wxWindowID id,
                        wxEvtHandler* evtHandler);

    virtual void SetSize(const wxRect& rect);
    virtual void Show(bool show, wxGridCellAttr *attr = NULL);

    virtual bool IsAcceptedKey(wxKeyEvent& event);
    virtual void BeginEdit(int row, int col, wxGrid* grid);
    virtual bool EndEdit(int row, int col, wxGrid* grid);

    virtual void Reset();
    virtual void StartingClick();
    virtual void StartingKey(wxKeyEvent& event);

    virtual wxGridCellEditor *Clone() const
        { return new wxGridCellBoolEditor; }

    // added GetValue so we can get the value which is in the control, see
    // also UseStringValues()
    virtual wxString GetValue() const;

    // set the string values returned by GetValue() for the true and false
    // states, respectively
    static void UseStringValues(const wxString& valueTrue = wxT("1"),
                                const wxString& valueFalse = wxEmptyString);

    // return true if the given string is equal to the string representation of
    // true value which we currently use
    static bool IsTrueValue(const wxString& value);

protected:
    wxCheckBox *CBox() const { return (wxCheckBox *)m_control; }

private:
    bool m_startValue;

    static wxString ms_stringValues[2];

    DECLARE_NO_COPY_CLASS(wxGridCellBoolEditor)
};

#endif // wxUSE_CHECKBOX

#if wxUSE_COMBOBOX

// the editor for string data allowing to choose from the list of strings
class WXDLLIMPEXP_ADV wxGridCellChoiceEditor : public wxGridCellEditor
{
public:
    // if !allowOthers, user can't type a string not in choices array
    wxGridCellChoiceEditor(size_t count = 0,
                           const wxString choices[] = NULL,
                           bool allowOthers = false);
    wxGridCellChoiceEditor(const wxArrayString& choices,
                           bool allowOthers = false);

    virtual void Create(wxWindow* parent,
                        wxWindowID id,
                        wxEvtHandler* evtHandler);

    virtual void PaintBackground(const wxRect& rectCell, wxGridCellAttr *attr);

    virtual void BeginEdit(int row, int col, wxGrid* grid);
    virtual bool EndEdit(int row, int col, wxGrid* grid);

    virtual void Reset();

    // parameters string format is "item1[,item2[...,itemN]]"
    virtual void SetParameters(const wxString& params);

    virtual wxGridCellEditor *Clone() const;

    // added GetValue so we can get the value which is in the control
    virtual wxString GetValue() const;

protected:
    wxComboBox *Combo() const { return (wxComboBox *)m_control; }

// DJC - (MAPTEK) you at least need access to m_choices if you
//                wish to override this class
protected:
    wxString        m_startValue;
    wxArrayString   m_choices;
    bool            m_allowOthers;

    DECLARE_NO_COPY_CLASS(wxGridCellChoiceEditor)
};

#endif // wxUSE_COMBOBOX

// ----------------------------------------------------------------------------
// wxGridCellAttr: this class can be used to alter the cells appearance in
// the grid by changing their colour/font/... from default. An object of this
// class may be returned by wxGridTable::GetAttr().
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxGridCellAttr : public wxClientDataContainer
{
public:
    enum wxAttrKind
    {
        Any,
        Default,
        Cell,
        Row,
        Col,
        Merged
    };

    // ctors
    wxGridCellAttr(wxGridCellAttr *attrDefault = NULL)
    {
        Init(attrDefault);

        // MB: args used to be 0,0 here but wxALIGN_LEFT is 0
        SetAlignment(-1, -1);
    }

    // VZ: considering the number of members wxGridCellAttr has now, this ctor
    //     seems to be pretty useless... may be we should just remove it?
    wxGridCellAttr(const wxColour& colText,
                   const wxColour& colBack,
                   const wxFont& font,
                   int hAlign,
                   int vAlign)
        : m_colText(colText), m_colBack(colBack), m_font(font)
    {
        Init();
        SetAlignment(hAlign, vAlign);
    }

    // creates a new copy of this object
    wxGridCellAttr *Clone() const;
    void MergeWith(wxGridCellAttr *mergefrom);

    // this class is ref counted: it is created with ref count of 1, so
    // calling DecRef() once will delete it. Calling IncRef() allows to lock
    // it until the matching DecRef() is called
    void IncRef() { m_nRef++; }
    void DecRef() { if ( --m_nRef == 0 ) delete this; }

    // setters
    void SetTextColour(const wxColour& colText) { m_colText = colText; }
    void SetBackgroundColour(const wxColour& colBack) { m_colBack = colBack; }
    void SetFont(const wxFont& font) { m_font = font; }
    void SetAlignment(int hAlign, int vAlign)
    {
        m_hAlign = hAlign;
        m_vAlign = vAlign;
    }
    void SetSize(int num_rows, int num_cols);
    void SetOverflow(bool allow = true)
        { m_overflow = allow ? Overflow : SingleCell; }
    void SetReadOnly(bool isReadOnly = true)
        { m_isReadOnly = isReadOnly ? ReadOnly : ReadWrite; }

    // takes ownership of the pointer
    void SetRenderer(wxGridCellRenderer *renderer)
        { wxSafeDecRef(m_renderer); m_renderer = renderer; }
    void SetEditor(wxGridCellEditor* editor)
        { wxSafeDecRef(m_editor); m_editor = editor; }

    void SetKind(wxAttrKind kind) { m_attrkind = kind; }

    // accessors
    bool HasTextColour() const { return m_colText.Ok(); }
    bool HasBackgroundColour() const { return m_colBack.Ok(); }
    bool HasFont() const { return m_font.Ok(); }
    bool HasAlignment() const { return (m_hAlign != -1 || m_vAlign != -1); }
    bool HasRenderer() const { return m_renderer != NULL; }
    bool HasEditor() const { return m_editor != NULL; }
    bool HasReadWriteMode() const { return m_isReadOnly != Unset; }
    bool HasOverflowMode() const { return m_overflow != UnsetOverflow; }
    bool HasSize() const { return m_sizeRows != 1 || m_sizeCols != 1; }

    const wxColour& GetTextColour() const;
    const wxColour& GetBackgroundColour() const;
    const wxFont& GetFont() const;
    void GetAlignment(int *hAlign, int *vAlign) const;
    void GetSize(int *num_rows, int *num_cols) const;
    bool GetOverflow() const
        { return m_overflow != SingleCell; }
    wxGridCellRenderer *GetRenderer(wxGrid* grid, int row, int col) const;
    wxGridCellEditor *GetEditor(wxGrid* grid, int row, int col) const;

    bool IsReadOnly() const { return m_isReadOnly == wxGridCellAttr::ReadOnly; }

    wxAttrKind GetKind() { return m_attrkind; }

    void SetDefAttr(wxGridCellAttr* defAttr) { m_defGridAttr = defAttr; }

protected:
    // the dtor is private because only DecRef() can delete us
    virtual ~wxGridCellAttr()
    {
        wxSafeDecRef(m_renderer);
        wxSafeDecRef(m_editor);
    }

private:
    enum wxAttrReadMode
    {
        Unset = -1,
        ReadWrite,
        ReadOnly
    };

    enum wxAttrOverflowMode
    {
        UnsetOverflow = -1,
        Overflow,
        SingleCell
    };

    // the common part of all ctors
    void Init(wxGridCellAttr *attrDefault = NULL);


    // the ref count - when it goes to 0, we die
    size_t   m_nRef;

    wxColour m_colText,
             m_colBack;
    wxFont   m_font;
    int      m_hAlign,
             m_vAlign;
    int      m_sizeRows,
             m_sizeCols;

    wxAttrOverflowMode  m_overflow;

    wxGridCellRenderer* m_renderer;
    wxGridCellEditor*   m_editor;
    wxGridCellAttr*     m_defGridAttr;

    wxAttrReadMode m_isReadOnly;

    wxAttrKind m_attrkind;

    // use Clone() instead
    DECLARE_NO_COPY_CLASS(wxGridCellAttr)

    // suppress the stupid gcc warning about the class having private dtor and
    // no friends
    friend class wxGridCellAttrDummyFriend;
};

// ----------------------------------------------------------------------------
// wxGridCellAttrProvider: class used by wxGridTableBase to retrieve/store the
// cell attributes.
// ----------------------------------------------------------------------------

// implementation note: we separate it from wxGridTableBase because we wish to
// avoid deriving a new table class if possible, and sometimes it will be
// enough to just derive another wxGridCellAttrProvider instead
//
// the default implementation is reasonably efficient for the generic case,
// but you might still wish to implement your own for some specific situations
// if you have performance problems with the stock one
class WXDLLIMPEXP_ADV wxGridCellAttrProvider : public wxClientDataContainer
{
public:
    wxGridCellAttrProvider();
    virtual ~wxGridCellAttrProvider();

    // DecRef() must be called on the returned pointer
    virtual wxGridCellAttr *GetAttr(int row, int col,
                                    wxGridCellAttr::wxAttrKind  kind ) const;

    // all these functions take ownership of the pointer, don't call DecRef()
    // on it
    virtual void SetAttr(wxGridCellAttr *attr, int row, int col);
    virtual void SetRowAttr(wxGridCellAttr *attr, int row);
    virtual void SetColAttr(wxGridCellAttr *attr, int col);

    // these functions must be called whenever some rows/cols are deleted
    // because the internal data must be updated then
    void UpdateAttrRows( size_t pos, int numRows );
    void UpdateAttrCols( size_t pos, int numCols );

private:
    void InitData();

    wxGridCellAttrProviderData *m_data;

    DECLARE_NO_COPY_CLASS(wxGridCellAttrProvider)
};

//////////////////////////////////////////////////////////////////////
//
//  Grid table classes
//
//////////////////////////////////////////////////////////////////////


class WXDLLIMPEXP_ADV wxGridTableBase : public wxObject, public wxClientDataContainer
{
public:
    wxGridTableBase();
    virtual ~wxGridTableBase();

    // You must override these functions in a derived table class
    //
    virtual int GetNumberRows() = 0;
    virtual int GetNumberCols() = 0;
    virtual bool IsEmptyCell( int row, int col ) = 0;
    virtual wxString GetValue( int row, int col ) = 0;
    virtual void SetValue( int row, int col, const wxString& value ) = 0;

    // Data type determination and value access
    virtual wxString GetTypeName( int row, int col );
    virtual bool CanGetValueAs( int row, int col, const wxString& typeName );
    virtual bool CanSetValueAs( int row, int col, const wxString& typeName );

    virtual long GetValueAsLong( int row, int col );
    virtual double GetValueAsDouble( int row, int col );
    virtual bool GetValueAsBool( int row, int col );

    virtual void SetValueAsLong( int row, int col, long value );
    virtual void SetValueAsDouble( int row, int col, double value );
    virtual void SetValueAsBool( int row, int col, bool value );

    // For user defined types
    virtual void* GetValueAsCustom( int row, int col, const wxString& typeName );
    virtual void  SetValueAsCustom( int row, int col, const wxString& typeName, void* value );


    // Overriding these is optional
    //
    virtual void SetView( wxGrid *grid ) { m_view = grid; }
    virtual wxGrid * GetView() const { return m_view; }

    virtual void Clear() {}
    virtual bool InsertRows( size_t pos = 0, size_t numRows = 1 );
    virtual bool AppendRows( size_t numRows = 1 );
    virtual bool DeleteRows( size_t pos = 0, size_t numRows = 1 );
    virtual bool InsertCols( size_t pos = 0, size_t numCols = 1 );
    virtual bool AppendCols( size_t numCols = 1 );
    virtual bool DeleteCols( size_t pos = 0, size_t numCols = 1 );

    virtual wxString GetRowLabelValue( int row );
    virtual wxString GetColLabelValue( int col );
    virtual void SetRowLabelValue( int WXUNUSED(row), const wxString& ) {}
    virtual void SetColLabelValue( int WXUNUSED(col), const wxString& ) {}

    // Attribute handling
    //

    // give us the attr provider to use - we take ownership of the pointer
    void SetAttrProvider(wxGridCellAttrProvider *attrProvider);

    // get the currently used attr provider (may be NULL)
    wxGridCellAttrProvider *GetAttrProvider() const { return m_attrProvider; }

    // Does this table allow attributes?  Default implementation creates
    // a wxGridCellAttrProvider if necessary.
    virtual bool CanHaveAttributes();

    // by default forwarded to wxGridCellAttrProvider if any. May be
    // overridden to handle attributes directly in the table.
    virtual wxGridCellAttr *GetAttr( int row, int col,
                                     wxGridCellAttr::wxAttrKind  kind );


    // these functions take ownership of the pointer
    virtual void SetAttr(wxGridCellAttr* attr, int row, int col);
    virtual void SetRowAttr(wxGridCellAttr *attr, int row);
    virtual void SetColAttr(wxGridCellAttr *attr, int col);

private:
    wxGrid * m_view;
    wxGridCellAttrProvider *m_attrProvider;

    DECLARE_ABSTRACT_CLASS(wxGridTableBase)
    DECLARE_NO_COPY_CLASS(wxGridTableBase)
};


// ----------------------------------------------------------------------------
// wxGridTableMessage
// ----------------------------------------------------------------------------

// IDs for messages sent from grid table to view
//
enum wxGridTableRequest
{
    wxGRIDTABLE_REQUEST_VIEW_GET_VALUES = 2000,
    wxGRIDTABLE_REQUEST_VIEW_SEND_VALUES,
    wxGRIDTABLE_NOTIFY_ROWS_INSERTED,
    wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
    wxGRIDTABLE_NOTIFY_ROWS_DELETED,
    wxGRIDTABLE_NOTIFY_COLS_INSERTED,
    wxGRIDTABLE_NOTIFY_COLS_APPENDED,
    wxGRIDTABLE_NOTIFY_COLS_DELETED
};

class WXDLLIMPEXP_ADV wxGridTableMessage
{
public:
    wxGridTableMessage();
    wxGridTableMessage( wxGridTableBase *table, int id,
                        int comInt1 = -1,
                        int comInt2 = -1 );

    void SetTableObject( wxGridTableBase *table ) { m_table = table; }
    wxGridTableBase * GetTableObject() const { return m_table; }
    void SetId( int id ) { m_id = id; }
    int  GetId() { return m_id; }
    void SetCommandInt( int comInt1 ) { m_comInt1 = comInt1; }
    int  GetCommandInt() { return m_comInt1; }
    void SetCommandInt2( int comInt2 ) { m_comInt2 = comInt2; }
    int  GetCommandInt2() { return m_comInt2; }

private:
    wxGridTableBase *m_table;
    int m_id;
    int m_comInt1;
    int m_comInt2;

    DECLARE_NO_COPY_CLASS(wxGridTableMessage)
};



// ------ wxGridStringArray
// A 2-dimensional array of strings for data values
//

WX_DECLARE_OBJARRAY_WITH_DECL(wxArrayString, wxGridStringArray,
                              class WXDLLIMPEXP_ADV);



// ------ wxGridStringTable
//
// Simplest type of data table for a grid for small tables of strings
// that are stored in memory
//

class WXDLLIMPEXP_ADV wxGridStringTable : public wxGridTableBase
{
public:
    wxGridStringTable();
    wxGridStringTable( int numRows, int numCols );
    virtual ~wxGridStringTable();

    // these are pure virtual in wxGridTableBase
    //
    int GetNumberRows();
    int GetNumberCols();
    wxString GetValue( int row, int col );
    void SetValue( int row, int col, const wxString& s );
    bool IsEmptyCell( int row, int col );

    // overridden functions from wxGridTableBase
    //
    void Clear();
    bool InsertRows( size_t pos = 0, size_t numRows = 1 );
    bool AppendRows( size_t numRows = 1 );
    bool DeleteRows( size_t pos = 0, size_t numRows = 1 );
    bool InsertCols( size_t pos = 0, size_t numCols = 1 );
    bool AppendCols( size_t numCols = 1 );
    bool DeleteCols( size_t pos = 0, size_t numCols = 1 );

    void SetRowLabelValue( int row, const wxString& );
    void SetColLabelValue( int col, const wxString& );
    wxString GetRowLabelValue( int row );
    wxString GetColLabelValue( int col );

private:
    wxGridStringArray m_data;

    // These only get used if you set your own labels, otherwise the
    // GetRow/ColLabelValue functions return wxGridTableBase defaults
    //
    wxArrayString     m_rowLabels;
    wxArrayString     m_colLabels;

    DECLARE_DYNAMIC_CLASS_NO_COPY( wxGridStringTable )
};



// ============================================================================
//  Grid view classes
// ============================================================================

// ----------------------------------------------------------------------------
// wxGridCellCoords: location of a cell in the grid
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxGridCellCoords
{
public:
    wxGridCellCoords() { m_row = m_col = -1; }
    wxGridCellCoords( int r, int c ) { m_row = r; m_col = c; }

    // default copy ctor is ok

    int GetRow() const { return m_row; }
    void SetRow( int n ) { m_row = n; }
    int GetCol() const { return m_col; }
    void SetCol( int n ) { m_col = n; }
    void Set( int row, int col ) { m_row = row; m_col = col; }

    wxGridCellCoords& operator=( const wxGridCellCoords& other )
    {
        if ( &other != this )
        {
            m_row=other.m_row;
            m_col=other.m_col;
        }
        return *this;
    }

    bool operator==( const wxGridCellCoords& other ) const
    {
        return (m_row == other.m_row  &&  m_col == other.m_col);
    }

    bool operator!=( const wxGridCellCoords& other ) const
    {
        return (m_row != other.m_row  ||  m_col != other.m_col);
    }

    bool operator!() const
    {
        return (m_row == -1 && m_col == -1 );
    }

private:
    int m_row;
    int m_col;
};


// For comparisons...
//
extern WXDLLIMPEXP_ADV wxGridCellCoords wxGridNoCellCoords;
extern WXDLLIMPEXP_ADV wxRect           wxGridNoCellRect;

// An array of cell coords...
//
WX_DECLARE_OBJARRAY_WITH_DECL(wxGridCellCoords, wxGridCellCoordsArray,
                              class WXDLLIMPEXP_ADV);

// ----------------------------------------------------------------------------
// wxGrid
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxGrid : public wxScrolledWindow
{
public:
    wxGrid() ;

        wxGrid( wxWindow *parent,
            wxWindowID id,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxWANTS_CHARS,
            const wxString& name = wxGridNameStr );

    bool Create( wxWindow *parent,
            wxWindowID id,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxWANTS_CHARS,
            const wxString& name = wxGridNameStr );

    virtual ~wxGrid();

    enum wxGridSelectionModes {wxGridSelectCells,
                               wxGridSelectRows,
                               wxGridSelectColumns};

    bool CreateGrid( int numRows, int numCols,
                     wxGrid::wxGridSelectionModes selmode =
                     wxGrid::wxGridSelectCells );

    void SetSelectionMode(wxGrid::wxGridSelectionModes selmode);
    wxGrid::wxGridSelectionModes GetSelectionMode() const;

    // ------ grid dimensions
    //
    int      GetNumberRows() { return  m_numRows; }
    int      GetNumberCols() { return  m_numCols; }


    // ------ display update functions
    //
    wxArrayInt CalcRowLabelsExposed( const wxRegion& reg );

    wxArrayInt CalcColLabelsExposed( const wxRegion& reg );
    wxGridCellCoordsArray CalcCellsExposed( const wxRegion& reg );


    // ------ event handlers
    //
    void ProcessRowLabelMouseEvent( wxMouseEvent& event );
    void ProcessColLabelMouseEvent( wxMouseEvent& event );
    void ProcessCornerLabelMouseEvent( wxMouseEvent& event );
    void ProcessGridCellMouseEvent( wxMouseEvent& event );
    bool ProcessTableMessage( wxGridTableMessage& );

    void DoEndDragResizeRow();
    void DoEndDragResizeCol();
    void DoEndDragMoveCol();

    wxGridTableBase * GetTable() const { return m_table; }
    bool SetTable( wxGridTableBase *table, bool takeOwnership = false,
                   wxGrid::wxGridSelectionModes selmode =
                   wxGrid::wxGridSelectCells );

    void ClearGrid();
    bool InsertRows( int pos = 0, int numRows = 1, bool updateLabels = true );
    bool AppendRows( int numRows = 1, bool updateLabels = true );
    bool DeleteRows( int pos = 0, int numRows = 1, bool updateLabels = true );
    bool InsertCols( int pos = 0, int numCols = 1, bool updateLabels = true );
    bool AppendCols( int numCols = 1, bool updateLabels = true );
    bool DeleteCols( int pos = 0, int numCols = 1, bool updateLabels = true );

    void DrawGridCellArea( wxDC& dc , const wxGridCellCoordsArray& cells );
    void DrawGridSpace( wxDC& dc );
    void DrawCellBorder( wxDC& dc, const wxGridCellCoords& );
    void DrawAllGridLines( wxDC& dc, const wxRegion & reg );
    void DrawCell( wxDC& dc, const wxGridCellCoords& );
    void DrawHighlight(wxDC& dc, const wxGridCellCoordsArray& cells);

    // this function is called when the current cell highlight must be redrawn
    // and may be overridden by the user
    virtual void DrawCellHighlight( wxDC& dc, const wxGridCellAttr *attr );

    virtual void DrawRowLabels( wxDC& dc, const wxArrayInt& rows );
    virtual void DrawRowLabel( wxDC& dc, int row );

    virtual void DrawColLabels( wxDC& dc, const wxArrayInt& cols );
    virtual void DrawColLabel( wxDC& dc, int col );


    // ------ Cell text drawing functions
    //
    void DrawTextRectangle( wxDC& dc, const wxString&, const wxRect&,
                            int horizontalAlignment = wxALIGN_LEFT,
                            int verticalAlignment = wxALIGN_TOP,
                            int textOrientation = wxHORIZONTAL );

    void DrawTextRectangle( wxDC& dc, const wxArrayString& lines, const wxRect&,
                            int horizontalAlignment = wxALIGN_LEFT,
                            int verticalAlignment = wxALIGN_TOP,
                            int textOrientation = wxHORIZONTAL );


    // Split a string containing newline chararcters into an array of
    // strings and return the number of lines
    //
    void StringToLines( const wxString& value, wxArrayString& lines );

    void GetTextBoxSize( const wxDC& dc,
                         const wxArrayString& lines,
                         long *width, long *height );


    // ------
    // Code that does a lot of grid modification can be enclosed
    // between BeginBatch() and EndBatch() calls to avoid screen
    // flicker
    //
    void     BeginBatch() { m_batchCount++; }
    void     EndBatch();

    int      GetBatchCount() { return m_batchCount; }

    virtual void Refresh(bool eraseb = true,
                         const wxRect* rect = (const wxRect *)  NULL);

    // Use this, rather than wxWindow::Refresh(), to force an
    // immediate repainting of the grid. Has no effect if you are
    // already inside a BeginBatch / EndBatch block.
    //
    // This function is necessary because wxGrid has a minimal OnPaint()
    // handler to reduce screen flicker.
    //
    void     ForceRefresh();


    // ------ edit control functions
    //
    bool IsEditable() const { return m_editable; }
    void EnableEditing( bool edit );

    void EnableCellEditControl( bool enable = true );
    void DisableCellEditControl() { EnableCellEditControl(false); }
    bool CanEnableCellControl() const;
    bool IsCellEditControlEnabled() const;
    bool IsCellEditControlShown() const;

    bool IsCurrentCellReadOnly() const;

    void ShowCellEditControl();
    void HideCellEditControl();
    void SaveEditControlValue();


    // ------ grid location functions
    //  Note that all of these functions work with the logical coordinates of
    //  grid cells and labels so you will need to convert from device
    //  coordinates for mouse events etc.
    //
    void XYToCell( int x, int y, wxGridCellCoords& );
    int  YToRow( int y );
    int  XToCol( int x, bool clipToMinMax = false );

    int  YToEdgeOfRow( int y );
    int  XToEdgeOfCol( int x );

    wxRect CellToRect( int row, int col );
    wxRect CellToRect( const wxGridCellCoords& coords )
        { return CellToRect( coords.GetRow(), coords.GetCol() ); }

    int  GetGridCursorRow() { return m_currentCellCoords.GetRow(); }
    int  GetGridCursorCol() { return m_currentCellCoords.GetCol(); }

    // check to see if a cell is either wholly visible (the default arg) or
    // at least partially visible in the grid window
    //
    bool IsVisible( int row, int col, bool wholeCellVisible = true );
    bool IsVisible( const wxGridCellCoords& coords, bool wholeCellVisible = true )
        { return IsVisible( coords.GetRow(), coords.GetCol(), wholeCellVisible ); }
    void MakeCellVisible( int row, int col );
    void MakeCellVisible( const wxGridCellCoords& coords )
        { MakeCellVisible( coords.GetRow(), coords.GetCol() ); }


    // ------ grid cursor movement functions
    //
    void SetGridCursor( int row, int col )
        { SetCurrentCell( wxGridCellCoords(row, col) ); }

    bool MoveCursorUp( bool expandSelection );
    bool MoveCursorDown( bool expandSelection );
    bool MoveCursorLeft( bool expandSelection );
    bool MoveCursorRight( bool expandSelection );
    bool MovePageDown();
    bool MovePageUp();
    bool MoveCursorUpBlock( bool expandSelection );
    bool MoveCursorDownBlock( bool expandSelection );
    bool MoveCursorLeftBlock( bool expandSelection );
    bool MoveCursorRightBlock( bool expandSelection );


    // ------ label and gridline formatting
    //
    int      GetDefaultRowLabelSize() { return WXGRID_DEFAULT_ROW_LABEL_WIDTH; }
    int      GetRowLabelSize() { return m_rowLabelWidth; }
    int      GetDefaultColLabelSize() { return WXGRID_DEFAULT_COL_LABEL_HEIGHT; }
    int      GetColLabelSize() { return m_colLabelHeight; }
    wxColour GetLabelBackgroundColour() { return m_labelBackgroundColour; }
    wxColour GetLabelTextColour() { return m_labelTextColour; }
    wxFont   GetLabelFont() { return m_labelFont; }
    void     GetRowLabelAlignment( int *horiz, int *vert );
    void     GetColLabelAlignment( int *horiz, int *vert );
    int      GetColLabelTextOrientation();
    wxString GetRowLabelValue( int row );
    wxString GetColLabelValue( int col );
    wxColour GetGridLineColour() { return m_gridLineColour; }

    // these methods may be overridden to customize individual grid lines
    // appearance
    virtual wxPen GetDefaultGridLinePen();
    virtual wxPen GetRowGridLinePen(int row);
    virtual wxPen GetColGridLinePen(int col);
    wxColour GetCellHighlightColour() { return m_cellHighlightColour; }
    int      GetCellHighlightPenWidth() { return m_cellHighlightPenWidth; }
    int      GetCellHighlightROPenWidth() { return m_cellHighlightROPenWidth; }

    void     SetRowLabelSize( int width );
    void     SetColLabelSize( int height );
    void     SetLabelBackgroundColour( const wxColour& );
    void     SetLabelTextColour( const wxColour& );
    void     SetLabelFont( const wxFont& );
    void     SetRowLabelAlignment( int horiz, int vert );
    void     SetColLabelAlignment( int horiz, int vert );
    void     SetColLabelTextOrientation( int textOrientation );
    void     SetRowLabelValue( int row, const wxString& );
    void     SetColLabelValue( int col, const wxString& );
    void     SetGridLineColour( const wxColour& );
    void     SetCellHighlightColour( const wxColour& );
    void     SetCellHighlightPenWidth(int width);
    void     SetCellHighlightROPenWidth(int width);

    void     EnableDragRowSize( bool enable = true );
    void     DisableDragRowSize() { EnableDragRowSize( false ); }
    bool     CanDragRowSize() { return m_canDragRowSize; }
    void     EnableDragColSize( bool enable = true );
    void     DisableDragColSize() { EnableDragColSize( false ); }
    bool     CanDragColSize() { return m_canDragColSize; }
    void     EnableDragColMove( bool enable = true );
    void     DisableDragColMove() { EnableDragColMove( false ); }
    bool     CanDragColMove() { return m_canDragColMove; }
    void     EnableDragGridSize(bool enable = true);
    void     DisableDragGridSize() { EnableDragGridSize(false); }
    bool     CanDragGridSize() { return m_canDragGridSize; }

    void     EnableDragCell( bool enable = true );
    void     DisableDragCell() { EnableDragCell( false ); }
    bool     CanDragCell() { return m_canDragCell; }

    // this sets the specified attribute for this cell or in this row/col
    void     SetAttr(int row, int col, wxGridCellAttr *attr);
    void     SetRowAttr(int row, wxGridCellAttr *attr);
    void     SetColAttr(int col, wxGridCellAttr *attr);

    // returns the attribute we may modify in place: a new one if this cell
    // doesn't have any yet or the existing one if it does
    //
    // DecRef() must be called on the returned pointer, as usual
    wxGridCellAttr *GetOrCreateCellAttr(int row, int col) const;


    // shortcuts for setting the column parameters

    // set the format for the data in the column: default is string
    void     SetColFormatBool(int col);
    void     SetColFormatNumber(int col);
    void     SetColFormatFloat(int col, int width = -1, int precision = -1);
    void     SetColFormatCustom(int col, const wxString& typeName);

    void     EnableGridLines( bool enable = true );
    bool     GridLinesEnabled() { return m_gridLinesEnabled; }

    // ------ row and col formatting
    //
    int      GetDefaultRowSize();
    int      GetRowSize( int row );
    int      GetDefaultColSize();
    int      GetColSize( int col );
    wxColour GetDefaultCellBackgroundColour();
    wxColour GetCellBackgroundColour( int row, int col );
    wxColour GetDefaultCellTextColour();
    wxColour GetCellTextColour( int row, int col );
    wxFont   GetDefaultCellFont();
    wxFont   GetCellFont( int row, int col );
    void     GetDefaultCellAlignment( int *horiz, int *vert );
    void     GetCellAlignment( int row, int col, int *horiz, int *vert );
    bool     GetDefaultCellOverflow();
    bool     GetCellOverflow( int row, int col );
    void     GetCellSize( int row, int col, int *num_rows, int *num_cols );

    void     SetDefaultRowSize( int height, bool resizeExistingRows = false );
    void     SetRowSize( int row, int height );
    void     SetDefaultColSize( int width, bool resizeExistingCols = false );

    void     SetColSize( int col, int width );

    //Column positions
    int GetColAt( int colPos ) const
    {
        if ( m_colAt.IsEmpty() )
            return colPos;
        else
            return m_colAt[colPos];
    }

    void SetColPos( int colID, int newPos );

    int GetColPos( int colID ) const
    {
        if ( m_colAt.IsEmpty() )
            return colID;
        else
        {
            for ( int i = 0; i < m_numCols; i++ )
            {
                if ( m_colAt[i] == colID )
                    return i;
            }
        }

        return -1;
    }

    // automatically size the column or row to fit to its contents, if
    // setAsMin is true, this optimal width will also be set as minimal width
    // for this column
    void     AutoSizeColumn( int col, bool setAsMin = true )
        { AutoSizeColOrRow(col, setAsMin, true); }
    void     AutoSizeRow( int row, bool setAsMin = true )
        { AutoSizeColOrRow(row, setAsMin, false); }

    // auto size all columns (very ineffective for big grids!)
    void     AutoSizeColumns( bool setAsMin = true )
        { (void)SetOrCalcColumnSizes(false, setAsMin); }

    void     AutoSizeRows( bool setAsMin = true )
        { (void)SetOrCalcRowSizes(false, setAsMin); }

    // auto size the grid, that is make the columns/rows of the "right" size
    // and also set the grid size to just fit its contents
    void     AutoSize();

    // autosize row height depending on label text
    void     AutoSizeRowLabelSize( int row );

    // autosize column width depending on label text
    void     AutoSizeColLabelSize( int col );

    // column won't be resized to be lesser width - this must be called during
    // the grid creation because it won't resize the column if it's already
    // narrower than the minimal width
    void     SetColMinimalWidth( int col, int width );
    void     SetRowMinimalHeight( int row, int width );

    /*  These members can be used to query and modify the minimal
     *  acceptable size of grid rows and columns. Call this function in
     *  your code which creates the grid if you want to display cells
     *  with a size smaller than the default acceptable minimum size.
     *  Like the members SetColMinimalWidth and SetRowMinimalWidth,
     *  the existing rows or columns will not be checked/resized.
     */
    void     SetColMinimalAcceptableWidth( int width );
    void     SetRowMinimalAcceptableHeight( int width );
    int      GetColMinimalAcceptableWidth() const;
    int      GetRowMinimalAcceptableHeight() const;

    void     SetDefaultCellBackgroundColour( const wxColour& );
    void     SetCellBackgroundColour( int row, int col, const wxColour& );
    void     SetDefaultCellTextColour( const wxColour& );

    void     SetCellTextColour( int row, int col, const wxColour& );
    void     SetDefaultCellFont( const wxFont& );
    void     SetCellFont( int row, int col, const wxFont& );
    void     SetDefaultCellAlignment( int horiz, int vert );
    void     SetCellAlignment( int row, int col, int horiz, int vert );
    void     SetDefaultCellOverflow( bool allow );
    void     SetCellOverflow( int row, int col, bool allow );
    void     SetCellSize( int row, int col, int num_rows, int num_cols );

    // takes ownership of the pointer
    void SetDefaultRenderer(wxGridCellRenderer *renderer);
    void SetCellRenderer(int row, int col, wxGridCellRenderer *renderer);
    wxGridCellRenderer *GetDefaultRenderer() const;
    wxGridCellRenderer* GetCellRenderer(int row, int col);

    // takes ownership of the pointer
    void SetDefaultEditor(wxGridCellEditor *editor);
    void SetCellEditor(int row, int col, wxGridCellEditor *editor);
    wxGridCellEditor *GetDefaultEditor() const;
    wxGridCellEditor* GetCellEditor(int row, int col);



    // ------ cell value accessors
    //
    wxString GetCellValue( int row, int col )
    {
        if ( m_table )
        {
            return m_table->GetValue( row, col );
        }
        else
        {
            return wxEmptyString;
        }
    }

    wxString GetCellValue( const wxGridCellCoords& coords )
        { return GetCellValue( coords.GetRow(), coords.GetCol() ); }

    void SetCellValue( int row, int col, const wxString& s );
    void SetCellValue( const wxGridCellCoords& coords, const wxString& s )
        { SetCellValue( coords.GetRow(), coords.GetCol(), s ); }

    // returns true if the cell can't be edited
    bool IsReadOnly(int row, int col) const;

    // make the cell editable/readonly
    void SetReadOnly(int row, int col, bool isReadOnly = true);

    // ------ select blocks of cells
    //
    void SelectRow( int row, bool addToSelected = false );
    void SelectCol( int col, bool addToSelected = false );

    void SelectBlock( int topRow, int leftCol, int bottomRow, int rightCol,
                      bool addToSelected = false );

    void SelectBlock( const wxGridCellCoords& topLeft,
                      const wxGridCellCoords& bottomRight,
                      bool addToSelected = false )
        { SelectBlock( topLeft.GetRow(), topLeft.GetCol(),
                       bottomRight.GetRow(), bottomRight.GetCol(),
                       addToSelected ); }

    void SelectAll();

    bool IsSelection();

    // ------ deselect blocks or cells
    //
    void DeselectRow( int row );
    void DeselectCol( int col );
    void DeselectCell( int row, int col );

    void ClearSelection();

    bool IsInSelection( int row, int col ) const;

    bool IsInSelection( const wxGridCellCoords& coords ) const
        { return IsInSelection( coords.GetRow(), coords.GetCol() ); }

    wxGridCellCoordsArray GetSelectedCells() const;
    wxGridCellCoordsArray GetSelectionBlockTopLeft() const;
    wxGridCellCoordsArray GetSelectionBlockBottomRight() const;
    wxArrayInt GetSelectedRows() const;
    wxArrayInt GetSelectedCols() const;

    // This function returns the rectangle that encloses the block of cells
    // limited by TopLeft and BottomRight cell in device coords and clipped
    //  to the client size of the grid window.
    //
    wxRect BlockToDeviceRect( const wxGridCellCoords & topLeft,
                              const wxGridCellCoords & bottomRight );

    // Access or update the selection fore/back colours
    wxColour GetSelectionBackground() const
        { return m_selectionBackground; }
    wxColour GetSelectionForeground() const
        { return m_selectionForeground; }

    void SetSelectionBackground(const wxColour& c) { m_selectionBackground = c; }
    void SetSelectionForeground(const wxColour& c) { m_selectionForeground = c; }


    // Methods for a registry for mapping data types to Renderers/Editors
    void RegisterDataType(const wxString& typeName,
                          wxGridCellRenderer* renderer,
                          wxGridCellEditor* editor);
    // DJC MAPTEK
    virtual wxGridCellEditor* GetDefaultEditorForCell(int row, int col) const;
    wxGridCellEditor* GetDefaultEditorForCell(const wxGridCellCoords& c) const
        { return GetDefaultEditorForCell(c.GetRow(), c.GetCol()); }
    virtual wxGridCellRenderer* GetDefaultRendererForCell(int row, int col) const;
    virtual wxGridCellEditor* GetDefaultEditorForType(const wxString& typeName) const;
    virtual wxGridCellRenderer* GetDefaultRendererForType(const wxString& typeName) const;

    // grid may occupy more space than needed for its rows/columns, this
    // function allows to set how big this extra space is
    void SetMargins(int extraWidth, int extraHeight)
    {
        m_extraWidth = extraWidth;
        m_extraHeight = extraHeight;

        CalcDimensions();
    }

    // Accessors for component windows
    wxWindow* GetGridWindow()            { return (wxWindow*)m_gridWin; }
    wxWindow* GetGridRowLabelWindow()    { return (wxWindow*)m_rowLabelWin; }
    wxWindow* GetGridColLabelWindow()    { return (wxWindow*)m_colLabelWin; }
    wxWindow* GetGridCornerLabelWindow() { return (wxWindow*)m_cornerLabelWin; }

    // Allow adjustment of scroll increment. The default is (15, 15).
    void SetScrollLineX(int x) { m_scrollLineX = x; }
    void SetScrollLineY(int y) { m_scrollLineY = y; }
    int GetScrollLineX() const { return m_scrollLineX; }
    int GetScrollLineY() const { return m_scrollLineY; }

    // Implementation
    int GetScrollX(int x) const
    {
        return (x + GetScrollLineX() - 1) / GetScrollLineX();
    }

    int GetScrollY(int y) const
    {
        return (y + GetScrollLineY() - 1) / GetScrollLineY();
    }


    // override some base class functions
    virtual bool Enable(bool enable = true);


    // ------ For compatibility with previous wxGrid only...
    //
    //  ************************************************
    //  **  Don't use these in new code because they  **
    //  **  are liable to disappear in a future       **
    //  **  revision                                  **
    //  ************************************************
    //

    wxGrid( wxWindow *parent,
            int x, int y, int w = wxDefaultCoord, int h = wxDefaultCoord,
            long style = wxWANTS_CHARS,
            const wxString& name = wxPanelNameStr )
        : wxScrolledWindow( parent, wxID_ANY, wxPoint(x,y), wxSize(w,h),
                            (style|wxWANTS_CHARS), name )
        {
            Create();
        }

    void SetCellValue( const wxString& val, int row, int col )
        { SetCellValue( row, col, val ); }

    void UpdateDimensions()
        { CalcDimensions(); }

    int GetRows() { return GetNumberRows(); }
    int GetCols() { return GetNumberCols(); }
    int GetCursorRow() { return GetGridCursorRow(); }
    int GetCursorColumn() { return GetGridCursorCol(); }

    int GetScrollPosX() { return 0; }
    int GetScrollPosY() { return 0; }

    void SetScrollX( int WXUNUSED(x) ) { }
    void SetScrollY( int WXUNUSED(y) ) { }

    void SetColumnWidth( int col, int width )
        { SetColSize( col, width ); }

    int GetColumnWidth( int col )
        { return GetColSize( col ); }

    void SetRowHeight( int row, int height )
        { SetRowSize( row, height ); }

    // GetRowHeight() is below

    int GetViewHeight() // returned num whole rows visible
        { return 0; }

    int GetViewWidth() // returned num whole cols visible
        { return 0; }

    void SetLabelSize( int orientation, int sz )
        {
            if ( orientation == wxHORIZONTAL )
                SetColLabelSize( sz );
            else
                SetRowLabelSize( sz );
        }

    int GetLabelSize( int orientation )
        {
            if ( orientation == wxHORIZONTAL )
                return GetColLabelSize();
            else
                return GetRowLabelSize();
        }

    void SetLabelAlignment( int orientation, int align )
        {
            if ( orientation == wxHORIZONTAL )
                SetColLabelAlignment( align, -1 );
            else
                SetRowLabelAlignment( align, -1 );
        }

    int GetLabelAlignment( int orientation, int WXUNUSED(align) )
        {
            int h, v;
            if ( orientation == wxHORIZONTAL )
            {
                GetColLabelAlignment( &h, &v );
                return h;
            }
            else
            {
                GetRowLabelAlignment( &h, &v );
                return h;
            }
        }

    void SetLabelValue( int orientation, const wxString& val, int pos )
        {
            if ( orientation == wxHORIZONTAL )
                SetColLabelValue( pos, val );
            else
                SetRowLabelValue( pos, val );
        }

    wxString GetLabelValue( int orientation, int pos)
        {
            if ( orientation == wxHORIZONTAL )
                return GetColLabelValue( pos );
            else
                return GetRowLabelValue( pos );
        }

    wxFont GetCellTextFont() const
        { return m_defaultCellAttr->GetFont(); }

    wxFont GetCellTextFont(int WXUNUSED(row), int WXUNUSED(col)) const
        { return m_defaultCellAttr->GetFont(); }

    void SetCellTextFont(const wxFont& fnt)
        { SetDefaultCellFont( fnt ); }

    void SetCellTextFont(const wxFont& fnt, int row, int col)
        { SetCellFont( row, col, fnt ); }

    void SetCellTextColour(const wxColour& val, int row, int col)
        { SetCellTextColour( row, col, val ); }

    void SetCellTextColour(const wxColour& col)
        { SetDefaultCellTextColour( col ); }

    void SetCellBackgroundColour(const wxColour& col)
        { SetDefaultCellBackgroundColour( col ); }

    void SetCellBackgroundColour(const wxColour& colour, int row, int col)
        { SetCellBackgroundColour( row, col, colour ); }

    bool GetEditable() { return IsEditable(); }
    void SetEditable( bool edit = true ) { EnableEditing( edit ); }
    bool GetEditInPlace() { return IsCellEditControlEnabled(); }

    void SetEditInPlace(bool WXUNUSED(edit) = true) { }

    void SetCellAlignment( int align, int row, int col)
    { SetCellAlignment(row, col, align, wxALIGN_CENTER); }
    void SetCellAlignment( int WXUNUSED(align) ) {}
    void SetCellBitmap(wxBitmap *WXUNUSED(bitmap), int WXUNUSED(row), int WXUNUSED(col))
    { }
    void SetDividerPen(const wxPen& WXUNUSED(pen)) { }
    wxPen& GetDividerPen() const;
    void OnActivate(bool WXUNUSED(active)) {}

    // ******** End of compatibility functions **********



    // ------ control IDs
    enum { wxGRID_CELLCTRL = 2000,
           wxGRID_TOPCTRL };

    // ------ control types
    enum { wxGRID_TEXTCTRL = 2100,
           wxGRID_CHECKBOX,
           wxGRID_CHOICE,
           wxGRID_COMBOBOX };

    // overridden wxWindow methods
    virtual void Fit();

#if wxABI_VERSION >= 20812
    // implementation only
    void CancelMouseCapture();
#endif

protected:
    virtual wxSize DoGetBestSize() const;

    bool m_created;

    wxGridWindow             *m_gridWin;
    wxGridRowLabelWindow     *m_rowLabelWin;
    wxGridColLabelWindow     *m_colLabelWin;
    wxGridCornerLabelWindow  *m_cornerLabelWin;

    wxGridTableBase          *m_table;
    bool                      m_ownTable;

    int m_numRows;
    int m_numCols;

    wxGridCellCoords m_currentCellCoords;

    wxGridCellCoords m_selectingTopLeft;
    wxGridCellCoords m_selectingBottomRight;
    wxGridCellCoords m_selectingKeyboard;
    wxGridSelection  *m_selection;
    wxColour    m_selectionBackground;
    wxColour    m_selectionForeground;

    // NB: *never* access m_row/col arrays directly because they are created
    //     on demand, *always* use accessor functions instead!

    // init the m_rowHeights/Bottoms arrays with default values
    void InitRowHeights();

    int        m_defaultRowHeight;
    int        m_minAcceptableRowHeight;
    wxArrayInt m_rowHeights;
    wxArrayInt m_rowBottoms;

    // init the m_colWidths/Rights arrays
    void InitColWidths();

    int        m_defaultColWidth;
    int        m_minAcceptableColWidth;
    wxArrayInt m_colWidths;
    wxArrayInt m_colRights;

    // get the col/row coords
    int GetColWidth(int col) const;
    int GetColLeft(int col) const;
    int GetColRight(int col) const;

    // this function must be public for compatibility...
public:
    int GetRowHeight(int row) const;
protected:

    int GetRowTop(int row) const;
    int GetRowBottom(int row) const;

    int m_rowLabelWidth;
    int m_colLabelHeight;

    // the size of the margin left to the right and bottom of the cell area
    int m_extraWidth,
        m_extraHeight;

    wxColour   m_labelBackgroundColour;
    wxColour   m_labelTextColour;
    wxFont     m_labelFont;

    int        m_rowLabelHorizAlign;
    int        m_rowLabelVertAlign;
    int        m_colLabelHorizAlign;
    int        m_colLabelVertAlign;
    int        m_colLabelTextOrientation;

    bool       m_defaultRowLabelValues;
    bool       m_defaultColLabelValues;

    wxColour   m_gridLineColour;
    bool       m_gridLinesEnabled;
    wxColour   m_cellHighlightColour;
    int        m_cellHighlightPenWidth;
    int        m_cellHighlightROPenWidth;


    // common part of AutoSizeColumn/Row() and GetBestSize()
    int SetOrCalcColumnSizes(bool calcOnly, bool setAsMin = true);
    int SetOrCalcRowSizes(bool calcOnly, bool setAsMin = true);

    // common part of AutoSizeColumn/Row()
    void AutoSizeColOrRow(int n, bool setAsMin, bool column /* or row? */);

    // if a column has a minimal width, it will be the value for it in this
    // hash table
    wxLongToLongHashMap m_colMinWidths,
                        m_rowMinHeights;

    // get the minimal width of the given column/row
    int GetColMinimalWidth(int col) const;
    int GetRowMinimalHeight(int col) const;

    // do we have some place to store attributes in?
    bool CanHaveAttributes();

    // cell attribute cache (currently we only cache 1, may be will do
    // more/better later)
    struct CachedAttr
    {
        int             row, col;
        wxGridCellAttr *attr;
    } m_attrCache;

    // invalidates the attribute cache
    void ClearAttrCache();

    // adds an attribute to cache
    void CacheAttr(int row, int col, wxGridCellAttr *attr) const;

    // looks for an attr in cache, returns true if found
    bool LookupAttr(int row, int col, wxGridCellAttr **attr) const;

    // looks for the attr in cache, if not found asks the table and caches the
    // result
    wxGridCellAttr *GetCellAttr(int row, int col) const;
    wxGridCellAttr *GetCellAttr(const wxGridCellCoords& coords )
        { return GetCellAttr( coords.GetRow(), coords.GetCol() ); }

    // the default cell attr object for cells that don't have their own
    wxGridCellAttr*     m_defaultCellAttr;


    bool m_inOnKeyDown;
    int  m_batchCount;


    wxGridTypeRegistry*    m_typeRegistry;

    enum CursorMode
    {
        WXGRID_CURSOR_SELECT_CELL,
        WXGRID_CURSOR_RESIZE_ROW,
        WXGRID_CURSOR_RESIZE_COL,
        WXGRID_CURSOR_SELECT_ROW,
        WXGRID_CURSOR_SELECT_COL,
        WXGRID_CURSOR_MOVE_COL
    };

    // this method not only sets m_cursorMode but also sets the correct cursor
    // for the given mode and, if captureMouse is not false releases the mouse
    // if it was captured and captures it if it must be captured
    //
    // for this to work, you should always use it and not set m_cursorMode
    // directly!
    void ChangeCursorMode(CursorMode mode,
                          wxWindow *win = (wxWindow *)NULL,
                          bool captureMouse = true);

    wxWindow *m_winCapture;     // the window which captured the mouse
    CursorMode m_cursorMode;

    //Column positions
    wxArrayInt m_colAt;
    int m_moveToCol;

    bool    m_canDragRowSize;
    bool    m_canDragColSize;
    bool    m_canDragColMove;
    bool    m_canDragGridSize;
    bool    m_canDragCell;
    int     m_dragLastPos;
    int     m_dragRowOrCol;
    bool    m_isDragging;
    wxPoint m_startDragPos;

    bool    m_waitForSlowClick;

    wxGridCellCoords m_selectionStart;

    wxCursor m_rowResizeCursor;
    wxCursor m_colResizeCursor;

    bool       m_editable;              // applies to whole grid
    bool       m_cellEditCtrlEnabled;   // is in-place edit currently shown?

    int m_scrollLineX; // X scroll increment
    int m_scrollLineY; // Y scroll increment

    void Create();
    void Init();
    void CalcDimensions();
    void CalcWindowSizes();
    bool Redimension( wxGridTableMessage& );


    int SendEvent( const wxEventType, int row, int col, wxMouseEvent& );
    int SendEvent( const wxEventType, int row, int col );
    int SendEvent( const wxEventType type)
    {
        return SendEvent(type,
                         m_currentCellCoords.GetRow(),
                         m_currentCellCoords.GetCol());
    }

    void OnPaint( wxPaintEvent& );
    void OnSize( wxSizeEvent& );
    void OnKeyDown( wxKeyEvent& );
    void OnKeyUp( wxKeyEvent& );
    void OnChar( wxKeyEvent& );
    void OnEraseBackground( wxEraseEvent& );


    void SetCurrentCell( const wxGridCellCoords& coords );
    void SetCurrentCell( int row, int col )
        { SetCurrentCell( wxGridCellCoords(row, col) ); }

    void HighlightBlock( int topRow, int leftCol, int bottomRow, int rightCol );

    void HighlightBlock( const wxGridCellCoords& topLeft,
                         const wxGridCellCoords& bottomRight )
        { HighlightBlock( topLeft.GetRow(), topLeft.GetCol(),
                       bottomRight.GetRow(), bottomRight.GetCol() ); }

    // ------ functions to get/send data (see also public functions)
    //
    bool GetModelValues();
    bool SetModelValues();

private:
    // Calculate the minimum acceptable size for labels area
    wxCoord CalcColOrRowLabelAreaMinSize(bool column /* or row? */);

    friend class WXDLLIMPEXP_FWD_ADV wxGridSelection;

    DECLARE_DYNAMIC_CLASS( wxGrid )
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxGrid)
};


// ----------------------------------------------------------------------------
// Grid event class and event types
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxGridEvent : public wxNotifyEvent
{
public:
    wxGridEvent()
        : wxNotifyEvent(), m_row(-1), m_col(-1), m_x(-1), m_y(-1),
        m_selecting(0), m_control(0), m_meta(0), m_shift(0), m_alt(0)
        {
        }

    wxGridEvent(int id, wxEventType type, wxObject* obj,
                int row=-1, int col=-1, int x=-1, int y=-1, bool sel = true,
                bool control = false, bool shift = false, bool alt = false, bool meta = false);

    virtual int GetRow() { return m_row; }
    virtual int GetCol() { return m_col; }
    wxPoint     GetPosition() { return wxPoint( m_x, m_y ); }
    bool        Selecting() { return m_selecting; }
    bool        ControlDown() { return m_control; }
    bool        MetaDown() { return m_meta; }
    bool        ShiftDown() { return m_shift; }
    bool        AltDown() { return m_alt; }
    bool        CmdDown()
    {
#if defined(__WXMAC__) || defined(__WXCOCOA__)
        return MetaDown();
#else
        return ControlDown();
#endif
    }

    virtual wxEvent *Clone() const { return new wxGridEvent(*this); }

protected:
    int         m_row;
    int         m_col;
    int         m_x;
    int         m_y;
    bool        m_selecting;
    bool        m_control;
    bool        m_meta;
    bool        m_shift;
    bool        m_alt;

    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxGridEvent)
};

class WXDLLIMPEXP_ADV wxGridSizeEvent : public wxNotifyEvent
{
public:
    wxGridSizeEvent()
        : wxNotifyEvent(), m_rowOrCol(-1), m_x(-1), m_y(-1),
        m_control(0), m_meta(0), m_shift(0), m_alt(0)
        {
        }

    wxGridSizeEvent(int id, wxEventType type, wxObject* obj,
                int rowOrCol=-1, int x=-1, int y=-1,
                bool control = false, bool shift = false, bool alt = false, bool meta = false);

    int         GetRowOrCol() { return m_rowOrCol; }
    wxPoint     GetPosition() { return wxPoint( m_x, m_y ); }
    bool        ControlDown() { return m_control; }
    bool        MetaDown() { return m_meta; }
    bool        ShiftDown() { return m_shift; }
    bool        AltDown() { return m_alt; }
    bool        CmdDown()
    {
#if defined(__WXMAC__) || defined(__WXCOCOA__)
        return MetaDown();
#else
        return ControlDown();
#endif
    }

    virtual wxEvent *Clone() const { return new wxGridSizeEvent(*this); }

protected:
    int         m_rowOrCol;
    int         m_x;
    int         m_y;
    bool        m_control;
    bool        m_meta;
    bool        m_shift;
    bool        m_alt;

    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxGridSizeEvent)
};


class WXDLLIMPEXP_ADV wxGridRangeSelectEvent : public wxNotifyEvent
{
public:
    wxGridRangeSelectEvent()
        : wxNotifyEvent()
        {
            m_topLeft     = wxGridNoCellCoords;
            m_bottomRight = wxGridNoCellCoords;
            m_selecting   = false;
            m_control     = false;
            m_meta        = false;
            m_shift       = false;
            m_alt         = false;
        }

    wxGridRangeSelectEvent(int id, wxEventType type, wxObject* obj,
                           const wxGridCellCoords& topLeft,
                           const wxGridCellCoords& bottomRight,
                           bool sel = true,
                           bool control = false, bool shift = false,
                           bool alt = false, bool meta = false);

    wxGridCellCoords GetTopLeftCoords() { return m_topLeft; }
    wxGridCellCoords GetBottomRightCoords() { return m_bottomRight; }
    int         GetTopRow()    { return m_topLeft.GetRow(); }
    int         GetBottomRow() { return m_bottomRight.GetRow(); }
    int         GetLeftCol()   { return m_topLeft.GetCol(); }
    int         GetRightCol()  { return m_bottomRight.GetCol(); }
    bool        Selecting() { return m_selecting; }
    bool        ControlDown()  { return m_control; }
    bool        MetaDown()     { return m_meta; }
    bool        ShiftDown()    { return m_shift; }
    bool        AltDown()      { return m_alt; }
    bool        CmdDown()
    {
#if defined(__WXMAC__) || defined(__WXCOCOA__)
        return MetaDown();
#else
        return ControlDown();
#endif
    }

    virtual wxEvent *Clone() const { return new wxGridRangeSelectEvent(*this); }

protected:
    wxGridCellCoords  m_topLeft;
    wxGridCellCoords  m_bottomRight;
    bool              m_selecting;
    bool              m_control;
    bool              m_meta;
    bool              m_shift;
    bool              m_alt;

    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxGridRangeSelectEvent)
};


class WXDLLIMPEXP_ADV wxGridEditorCreatedEvent : public wxCommandEvent {
public:
    wxGridEditorCreatedEvent()
        : wxCommandEvent()
        {
            m_row  = 0;
            m_col  = 0;
            m_ctrl = NULL;
        }

    wxGridEditorCreatedEvent(int id, wxEventType type, wxObject* obj,
                             int row, int col, wxControl* ctrl);

    int GetRow()                        { return m_row; }
    int GetCol()                        { return m_col; }
    wxControl* GetControl()             { return m_ctrl; }
    void SetRow(int row)                { m_row = row; }
    void SetCol(int col)                { m_col = col; }
    void SetControl(wxControl* ctrl)    { m_ctrl = ctrl; }

    virtual wxEvent *Clone() const { return new wxGridEditorCreatedEvent(*this); }

private:
    int m_row;
    int m_col;
    wxControl* m_ctrl;

    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxGridEditorCreatedEvent)
};


BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_CELL_LEFT_CLICK, 1580)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_CELL_RIGHT_CLICK, 1581)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_CELL_LEFT_DCLICK, 1582)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_CELL_RIGHT_DCLICK, 1583)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_LABEL_LEFT_CLICK, 1584)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_LABEL_RIGHT_CLICK, 1585)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_LABEL_LEFT_DCLICK, 1586)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_LABEL_RIGHT_DCLICK, 1587)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_ROW_SIZE, 1588)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_COL_SIZE, 1589)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_RANGE_SELECT, 1590)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_CELL_CHANGE, 1591)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_SELECT_CELL, 1592)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_EDITOR_SHOWN, 1593)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_EDITOR_HIDDEN, 1594)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_EDITOR_CREATED, 1595)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_CELL_BEGIN_DRAG, 1596)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_GRID_COL_MOVE, 1597)
END_DECLARE_EVENT_TYPES()


typedef void (wxEvtHandler::*wxGridEventFunction)(wxGridEvent&);
typedef void (wxEvtHandler::*wxGridSizeEventFunction)(wxGridSizeEvent&);
typedef void (wxEvtHandler::*wxGridRangeSelectEventFunction)(wxGridRangeSelectEvent&);
typedef void (wxEvtHandler::*wxGridEditorCreatedEventFunction)(wxGridEditorCreatedEvent&);

#define wxGridEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxGridEventFunction, &func)

#define wxGridSizeEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxGridSizeEventFunction, &func)

#define wxGridRangeSelectEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxGridRangeSelectEventFunction, &func)

#define wxGridEditorCreatedEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxGridEditorCreatedEventFunction, &func)

#define wx__DECLARE_GRIDEVT(evt, id, fn) \
    wx__DECLARE_EVT1(wxEVT_GRID_ ## evt, id, wxGridEventHandler(fn))

#define wx__DECLARE_GRIDSIZEEVT(evt, id, fn) \
    wx__DECLARE_EVT1(wxEVT_GRID_ ## evt, id, wxGridSizeEventHandler(fn))

#define wx__DECLARE_GRIDRANGESELEVT(evt, id, fn) \
    wx__DECLARE_EVT1(wxEVT_GRID_ ## evt, id, wxGridRangeSelectEventHandler(fn))

#define wx__DECLARE_GRIDEDITOREVT(evt, id, fn) \
    wx__DECLARE_EVT1(wxEVT_GRID_ ## evt, id, wxGridEditorCreatedEventHandler(fn))

#define EVT_GRID_CMD_CELL_LEFT_CLICK(id, fn)     wx__DECLARE_GRIDEVT(CELL_LEFT_CLICK, id, fn)
#define EVT_GRID_CMD_CELL_RIGHT_CLICK(id, fn)    wx__DECLARE_GRIDEVT(CELL_RIGHT_CLICK, id, fn)
#define EVT_GRID_CMD_CELL_LEFT_DCLICK(id, fn)    wx__DECLARE_GRIDEVT(CELL_LEFT_DCLICK, id, fn)
#define EVT_GRID_CMD_CELL_RIGHT_DCLICK(id, fn)   wx__DECLARE_GRIDEVT(CELL_RIGHT_DCLICK, id, fn)
#define EVT_GRID_CMD_LABEL_LEFT_CLICK(id, fn)    wx__DECLARE_GRIDEVT(LABEL_LEFT_CLICK, id, fn)
#define EVT_GRID_CMD_LABEL_RIGHT_CLICK(id, fn)   wx__DECLARE_GRIDEVT(LABEL_RIGHT_CLICK, id, fn)
#define EVT_GRID_CMD_LABEL_LEFT_DCLICK(id, fn)   wx__DECLARE_GRIDEVT(LABEL_LEFT_DCLICK, id, fn)
#define EVT_GRID_CMD_LABEL_RIGHT_DCLICK(id, fn)  wx__DECLARE_GRIDEVT(LABEL_RIGHT_DCLICK, id, fn)
#define EVT_GRID_CMD_ROW_SIZE(id, fn)            wx__DECLARE_GRIDSIZEEVT(ROW_SIZE, id, fn)
#define EVT_GRID_CMD_COL_SIZE(id, fn)            wx__DECLARE_GRIDSIZEEVT(COL_SIZE, id, fn)
#define EVT_GRID_CMD_COL_MOVE(id, fn)            wx__DECLARE_GRIDEVT(COL_MOVE, id, fn)
#define EVT_GRID_CMD_RANGE_SELECT(id, fn)        wx__DECLARE_GRIDRANGESELEVT(RANGE_SELECT, id, fn)
#define EVT_GRID_CMD_CELL_CHANGE(id, fn)         wx__DECLARE_GRIDEVT(CELL_CHANGE, id, fn)
#define EVT_GRID_CMD_SELECT_CELL(id, fn)         wx__DECLARE_GRIDEVT(SELECT_CELL, id, fn)
#define EVT_GRID_CMD_EDITOR_SHOWN(id, fn)        wx__DECLARE_GRIDEVT(EDITOR_SHOWN, id, fn)
#define EVT_GRID_CMD_EDITOR_HIDDEN(id, fn)       wx__DECLARE_GRIDEVT(EDITOR_HIDDEN, id, fn)
#define EVT_GRID_CMD_EDITOR_CREATED(id, fn)      wx__DECLARE_GRIDEDITOREVT(EDITOR_CREATED, id, fn)
#define EVT_GRID_CMD_CELL_BEGIN_DRAG(id, fn)     wx__DECLARE_GRIDEVT(CELL_BEGIN_DRAG, id, fn)

// same as above but for any id (exists mainly for backwards compatibility but
// then it's also true that you rarely have multiple grid in the same window)
#define EVT_GRID_CELL_LEFT_CLICK(fn)     EVT_GRID_CMD_CELL_LEFT_CLICK(wxID_ANY, fn)
#define EVT_GRID_CELL_RIGHT_CLICK(fn)    EVT_GRID_CMD_CELL_RIGHT_CLICK(wxID_ANY, fn)
#define EVT_GRID_CELL_LEFT_DCLICK(fn)    EVT_GRID_CMD_CELL_LEFT_DCLICK(wxID_ANY, fn)
#define EVT_GRID_CELL_RIGHT_DCLICK(fn)   EVT_GRID_CMD_CELL_RIGHT_DCLICK(wxID_ANY, fn)
#define EVT_GRID_LABEL_LEFT_CLICK(fn)    EVT_GRID_CMD_LABEL_LEFT_CLICK(wxID_ANY, fn)
#define EVT_GRID_LABEL_RIGHT_CLICK(fn)   EVT_GRID_CMD_LABEL_RIGHT_CLICK(wxID_ANY, fn)
#define EVT_GRID_LABEL_LEFT_DCLICK(fn)   EVT_GRID_CMD_LABEL_LEFT_DCLICK(wxID_ANY, fn)
#define EVT_GRID_LABEL_RIGHT_DCLICK(fn)  EVT_GRID_CMD_LABEL_RIGHT_DCLICK(wxID_ANY, fn)
#define EVT_GRID_ROW_SIZE(fn)            EVT_GRID_CMD_ROW_SIZE(wxID_ANY, fn)
#define EVT_GRID_COL_SIZE(fn)            EVT_GRID_CMD_COL_SIZE(wxID_ANY, fn)
#define EVT_GRID_COL_MOVE(fn)            EVT_GRID_CMD_COL_MOVE(wxID_ANY, fn)
#define EVT_GRID_RANGE_SELECT(fn)        EVT_GRID_CMD_RANGE_SELECT(wxID_ANY, fn)
#define EVT_GRID_CELL_CHANGE(fn)         EVT_GRID_CMD_CELL_CHANGE(wxID_ANY, fn)
#define EVT_GRID_SELECT_CELL(fn)         EVT_GRID_CMD_SELECT_CELL(wxID_ANY, fn)
#define EVT_GRID_EDITOR_SHOWN(fn)        EVT_GRID_CMD_EDITOR_SHOWN(wxID_ANY, fn)
#define EVT_GRID_EDITOR_HIDDEN(fn)       EVT_GRID_CMD_EDITOR_HIDDEN(wxID_ANY, fn)
#define EVT_GRID_EDITOR_CREATED(fn)      EVT_GRID_CMD_EDITOR_CREATED(wxID_ANY, fn)
#define EVT_GRID_CELL_BEGIN_DRAG(fn)     EVT_GRID_CMD_CELL_BEGIN_DRAG(wxID_ANY, fn)

#if 0  // TODO: implement these ?  others ?

extern const int wxEVT_GRID_CREATE_CELL;
extern const int wxEVT_GRID_CHANGE_LABELS;
extern const int wxEVT_GRID_CHANGE_SEL_LABEL;

#define EVT_GRID_CREATE_CELL(fn)      DECLARE_EVENT_TABLE_ENTRY( wxEVT_GRID_CREATE_CELL,      wxID_ANY, wxID_ANY, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxGridEventFunction, &fn ), NULL ),
#define EVT_GRID_CHANGE_LABELS(fn)    DECLARE_EVENT_TABLE_ENTRY( wxEVT_GRID_CHANGE_LABELS,    wxID_ANY, wxID_ANY, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxGridEventFunction, &fn ), NULL ),
#define EVT_GRID_CHANGE_SEL_LABEL(fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_GRID_CHANGE_SEL_LABEL, wxID_ANY, wxID_ANY, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxGridEventFunction, &fn ), NULL ),

#endif

#endif // wxUSE_GRID
#endif // _WX_GENERIC_GRID_H_
