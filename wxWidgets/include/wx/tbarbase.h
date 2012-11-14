/////////////////////////////////////////////////////////////////////////////
// Name:        wx/tbarbase.h
// Purpose:     Base class for toolbar classes
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: tbarbase.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_TBARBASE_H_
#define _WX_TBARBASE_H_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/defs.h"

#if wxUSE_TOOLBAR

#include "wx/bitmap.h"
#include "wx/list.h"
#include "wx/control.h"

class WXDLLIMPEXP_FWD_CORE wxToolBarBase;
class WXDLLIMPEXP_FWD_CORE wxToolBarToolBase;
class WXDLLIMPEXP_FWD_CORE wxImage;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

extern WXDLLEXPORT_DATA(const wxChar) wxToolBarNameStr[];
extern WXDLLEXPORT_DATA(const wxSize) wxDefaultSize;
extern WXDLLEXPORT_DATA(const wxPoint) wxDefaultPosition;

enum wxToolBarToolStyle
{
    wxTOOL_STYLE_BUTTON    = 1,
    wxTOOL_STYLE_SEPARATOR = 2,
    wxTOOL_STYLE_CONTROL
};

// ----------------------------------------------------------------------------
// wxToolBarTool is a toolbar element.
//
// It has a unique id (except for the separators which always have id wxID_ANY), the
// style (telling whether it is a normal button, separator or a control), the
// state (toggled or not, enabled or not) and short and long help strings. The
// default implementations use the short help string for the tooltip text which
// is popped up when the mouse pointer enters the tool and the long help string
// for the applications status bar.
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxToolBarToolBase : public wxObject
{
public:
    // ctors & dtor
    // ------------

    wxToolBarToolBase(wxToolBarBase *tbar = (wxToolBarBase *)NULL,
                      int toolid = wxID_SEPARATOR,
                      const wxString& label = wxEmptyString,
                      const wxBitmap& bmpNormal = wxNullBitmap,
                      const wxBitmap& bmpDisabled = wxNullBitmap,
                      wxItemKind kind = wxITEM_NORMAL,
                      wxObject *clientData = (wxObject *) NULL,
                      const wxString& shortHelpString = wxEmptyString,
                      const wxString& longHelpString = wxEmptyString)
        : m_label(label),
          m_shortHelpString(shortHelpString),
          m_longHelpString(longHelpString)
    {
        m_tbar = tbar;
        m_id = toolid;
        if (m_id == wxID_ANY)
            m_id = wxNewId();
        m_clientData = clientData;

        m_bmpNormal = bmpNormal;
        m_bmpDisabled = bmpDisabled;

        m_kind = kind;

        m_enabled = true;
        m_toggled = false;

        m_toolStyle = toolid == wxID_SEPARATOR ? wxTOOL_STYLE_SEPARATOR
                                           : wxTOOL_STYLE_BUTTON;
    }

    wxToolBarToolBase(wxToolBarBase *tbar, wxControl *control)
    {
        m_tbar = tbar;
        m_control = control;
        m_id = control->GetId();

        m_kind = wxITEM_MAX;    // invalid value

        m_enabled = true;
        m_toggled = false;

        m_toolStyle = wxTOOL_STYLE_CONTROL;
    }

    virtual ~wxToolBarToolBase(){}

    // accessors
    // ---------

    // general
    int GetId() const { return m_id; }

    wxControl *GetControl() const
    {
        wxASSERT_MSG( IsControl(), wxT("this toolbar tool is not a control") );

        return m_control;
    }

    wxToolBarBase *GetToolBar() const { return m_tbar; }

    // style
    bool IsButton() const { return m_toolStyle == wxTOOL_STYLE_BUTTON; }
    bool IsControl() const { return m_toolStyle == wxTOOL_STYLE_CONTROL; }
    bool IsSeparator() const { return m_toolStyle == wxTOOL_STYLE_SEPARATOR; }
    int GetStyle() const { return m_toolStyle; }
    wxItemKind GetKind() const
    {
        wxASSERT_MSG( IsButton(), wxT("only makes sense for buttons") );

        return m_kind;
    }

    // state
    bool IsEnabled() const { return m_enabled; }
    bool IsToggled() const { return m_toggled; }
    bool CanBeToggled() const
        { return m_kind == wxITEM_CHECK || m_kind == wxITEM_RADIO; }

    // attributes
    const wxBitmap& GetNormalBitmap() const { return m_bmpNormal; }
    const wxBitmap& GetDisabledBitmap() const { return m_bmpDisabled; }

    const wxBitmap& GetBitmap() const
        { return IsEnabled() ? GetNormalBitmap() : GetDisabledBitmap(); }

    const wxString& GetLabel() const { return m_label; }

    const wxString& GetShortHelp() const { return m_shortHelpString; }
    const wxString& GetLongHelp() const { return m_longHelpString; }

    wxObject *GetClientData() const
    {
        if ( m_toolStyle == wxTOOL_STYLE_CONTROL )
        {
            return (wxObject*)m_control->GetClientData();
        }
        else
        {
            return m_clientData;
        }
    }

    // modifiers: return true if the state really changed
    bool Enable(bool enable);
    bool Toggle(bool toggle);
    bool SetToggle(bool toggle);
    bool SetShortHelp(const wxString& help);
    bool SetLongHelp(const wxString& help);

    void Toggle() { Toggle(!IsToggled()); }

    void SetNormalBitmap(const wxBitmap& bmp) { m_bmpNormal = bmp; }
    void SetDisabledBitmap(const wxBitmap& bmp) { m_bmpDisabled = bmp; }

    virtual void SetLabel(const wxString& label) { m_label = label; }

    void SetClientData(wxObject *clientData)
    {
        if ( m_toolStyle == wxTOOL_STYLE_CONTROL )
        {
            m_control->SetClientData(clientData);
        }
        else
        {
            m_clientData = clientData;
        }
    }

    // add tool to/remove it from a toolbar
    virtual void Detach() { m_tbar = (wxToolBarBase *)NULL; }
    virtual void Attach(wxToolBarBase *tbar) { m_tbar = tbar; }

protected:
    wxToolBarBase *m_tbar;  // the toolbar to which we belong (may be NULL)

    // tool parameters
    int m_toolStyle;    // see enum wxToolBarToolStyle
    int m_id;           // the tool id, wxID_SEPARATOR for separator
    wxItemKind m_kind;  // for normal buttons may be wxITEM_NORMAL/CHECK/RADIO

    // as controls have their own client data, no need to waste memory
    union
    {
        wxObject         *m_clientData;
        wxControl        *m_control;
    };

    // tool state
    bool m_toggled;
    bool m_enabled;

    // normal and disabled bitmaps for the tool, both can be invalid
    wxBitmap m_bmpNormal;
    wxBitmap m_bmpDisabled;

    // the button label
    wxString m_label;

    // short and long help strings
    wxString m_shortHelpString;
    wxString m_longHelpString;

    DECLARE_DYNAMIC_CLASS_NO_COPY(wxToolBarToolBase)
};

// a list of toolbar tools
WX_DECLARE_EXPORTED_LIST(wxToolBarToolBase, wxToolBarToolsList);

// ----------------------------------------------------------------------------
// the base class for all toolbars
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxToolBarBase : public wxControl
{
public:
    wxToolBarBase();
    virtual ~wxToolBarBase();

    // toolbar construction
    // --------------------

    // the full AddTool() function
    //
    // If bmpDisabled is wxNullBitmap, a shadowed version of the normal bitmap
    // is created and used as the disabled image.
    wxToolBarToolBase *AddTool(int toolid,
                               const wxString& label,
                               const wxBitmap& bitmap,
                               const wxBitmap& bmpDisabled,
                               wxItemKind kind = wxITEM_NORMAL,
                               const wxString& shortHelp = wxEmptyString,
                               const wxString& longHelp = wxEmptyString,
                               wxObject *data = NULL)
    {
        return DoAddTool(toolid, label, bitmap, bmpDisabled, kind,
                         shortHelp, longHelp, data);
    }

    // the most common AddTool() version
    wxToolBarToolBase *AddTool(int toolid,
                               const wxString& label,
                               const wxBitmap& bitmap,
                               const wxString& shortHelp = wxEmptyString,
                               wxItemKind kind = wxITEM_NORMAL)
    {
        return AddTool(toolid, label, bitmap, wxNullBitmap, kind, shortHelp);
    }

    // add a check tool, i.e. a tool which can be toggled
    wxToolBarToolBase *AddCheckTool(int toolid,
                                    const wxString& label,
                                    const wxBitmap& bitmap,
                                    const wxBitmap& bmpDisabled = wxNullBitmap,
                                    const wxString& shortHelp = wxEmptyString,
                                    const wxString& longHelp = wxEmptyString,
                                    wxObject *data = NULL)
    {
        return AddTool(toolid, label, bitmap, bmpDisabled, wxITEM_CHECK,
                       shortHelp, longHelp, data);
    }

    // add a radio tool, i.e. a tool which can be toggled and releases any
    // other toggled radio tools in the same group when it happens
    wxToolBarToolBase *AddRadioTool(int toolid,
                                    const wxString& label,
                                    const wxBitmap& bitmap,
                                    const wxBitmap& bmpDisabled = wxNullBitmap,
                                    const wxString& shortHelp = wxEmptyString,
                                    const wxString& longHelp = wxEmptyString,
                                    wxObject *data = NULL)
    {
        return AddTool(toolid, label, bitmap, bmpDisabled, wxITEM_RADIO,
                       shortHelp, longHelp, data);
    }


    // insert the new tool at the given position, if pos == GetToolsCount(), it
    // is equivalent to AddTool()
    virtual wxToolBarToolBase *InsertTool
                               (
                                    size_t pos,
                                    int toolid,
                                    const wxString& label,
                                    const wxBitmap& bitmap,
                                    const wxBitmap& bmpDisabled = wxNullBitmap,
                                    wxItemKind kind = wxITEM_NORMAL,
                                    const wxString& shortHelp = wxEmptyString,
                                    const wxString& longHelp = wxEmptyString,
                                    wxObject *clientData = NULL
                               );

    virtual wxToolBarToolBase *AddTool (wxToolBarToolBase *tool);
    virtual wxToolBarToolBase *InsertTool (size_t pos, wxToolBarToolBase *tool);

    // add an arbitrary control to the toolbar (notice that
    // the control will be deleted by the toolbar and that it will also adjust
    // its position/size)
    //
    // NB: the control should have toolbar as its parent
    virtual wxToolBarToolBase *AddControl(wxControl *control);
    virtual wxToolBarToolBase *InsertControl(size_t pos, wxControl *control);

    // get the control with the given id or return NULL
    virtual wxControl *FindControl( int toolid );

    // add a separator to the toolbar
    virtual wxToolBarToolBase *AddSeparator();
    virtual wxToolBarToolBase *InsertSeparator(size_t pos);

    // remove the tool from the toolbar: the caller is responsible for actually
    // deleting the pointer
    virtual wxToolBarToolBase *RemoveTool(int toolid);

    // delete tool either by index or by position
    virtual bool DeleteToolByPos(size_t pos);
    virtual bool DeleteTool(int toolid);

    // delete all tools
    virtual void ClearTools();

    // must be called after all buttons have been created to finish toolbar
    // initialisation
    virtual bool Realize();

    // tools state
    // -----------

    virtual void EnableTool(int toolid, bool enable);
    virtual void ToggleTool(int toolid, bool toggle);

    // Set this to be togglable (or not)
    virtual void SetToggle(int toolid, bool toggle);

    // set/get tools client data (not for controls)
    virtual wxObject *GetToolClientData(int toolid) const;
    virtual void SetToolClientData(int toolid, wxObject *clientData);

    // returns tool pos, or wxNOT_FOUND if tool isn't found
    virtual int GetToolPos(int id) const;

    // return true if the tool is toggled
    virtual bool GetToolState(int toolid) const;

    virtual bool GetToolEnabled(int toolid) const;

    virtual void SetToolShortHelp(int toolid, const wxString& helpString);
    virtual wxString GetToolShortHelp(int toolid) const;
    virtual void SetToolLongHelp(int toolid, const wxString& helpString);
    virtual wxString GetToolLongHelp(int toolid) const;

    // margins/packing/separation
    // --------------------------

    virtual void SetMargins(int x, int y);
    void SetMargins(const wxSize& size)
        { SetMargins((int) size.x, (int) size.y); }
    virtual void SetToolPacking(int packing)
        { m_toolPacking = packing; }
    virtual void SetToolSeparation(int separation)
        { m_toolSeparation = separation; }

    virtual wxSize GetToolMargins() const { return wxSize(m_xMargin, m_yMargin); }
    virtual int GetToolPacking() const { return m_toolPacking; }
    virtual int GetToolSeparation() const { return m_toolSeparation; }

    // toolbar geometry
    // ----------------

    // set the number of toolbar rows
    virtual void SetRows(int nRows);

    // the toolbar can wrap - limit the number of columns or rows it may take
    void SetMaxRowsCols(int rows, int cols)
        { m_maxRows = rows; m_maxCols = cols; }
    int GetMaxRows() const { return m_maxRows; }
    int GetMaxCols() const { return m_maxCols; }

    // get/set the size of the bitmaps used by the toolbar: should be called
    // before adding any tools to the toolbar
    virtual void SetToolBitmapSize(const wxSize& size)
        { m_defaultWidth = size.x; m_defaultHeight = size.y; }
    virtual wxSize GetToolBitmapSize() const
        { return wxSize(m_defaultWidth, m_defaultHeight); }

    // the button size in some implementations is bigger than the bitmap size:
    // get the total button size (by default the same as bitmap size)
    virtual wxSize GetToolSize() const
        { return GetToolBitmapSize(); }

    // returns a (non separator) tool containing the point (x, y) or NULL if
    // there is no tool at this point (corrdinates are client)
    virtual wxToolBarToolBase *FindToolForPosition(wxCoord x,
                                                   wxCoord y) const = 0;

    // find the tool by id
    wxToolBarToolBase *FindById(int toolid) const;

    // return true if this is a vertical toolbar, otherwise false
    bool IsVertical() const { return HasFlag(wxTB_LEFT | wxTB_RIGHT); }


    // the old versions of the various methods kept for compatibility
    // don't use in the new code!
    // --------------------------------------------------------------

    wxToolBarToolBase *AddTool(int toolid,
                               const wxBitmap& bitmap,
                               const wxBitmap& bmpDisabled,
                               bool toggle = false,
                               wxObject *clientData = NULL,
                               const wxString& shortHelpString = wxEmptyString,
                               const wxString& longHelpString = wxEmptyString)
    {
        return AddTool(toolid, wxEmptyString,
                       bitmap, bmpDisabled,
                       toggle ? wxITEM_CHECK : wxITEM_NORMAL,
                       shortHelpString, longHelpString, clientData);
    }

    wxToolBarToolBase *AddTool(int toolid,
                               const wxBitmap& bitmap,
                               const wxString& shortHelpString = wxEmptyString,
                               const wxString& longHelpString = wxEmptyString)
    {
        return AddTool(toolid, wxEmptyString,
                       bitmap, wxNullBitmap, wxITEM_NORMAL,
                       shortHelpString, longHelpString, NULL);
    }

    wxToolBarToolBase *AddTool(int toolid,
                               const wxBitmap& bitmap,
                               const wxBitmap& bmpDisabled,
                               bool toggle,
                               wxCoord xPos,
                               wxCoord yPos = wxDefaultCoord,
                               wxObject *clientData = NULL,
                               const wxString& shortHelp = wxEmptyString,
                               const wxString& longHelp = wxEmptyString)
    {
        return DoAddTool(toolid, wxEmptyString, bitmap, bmpDisabled,
                         toggle ? wxITEM_CHECK : wxITEM_NORMAL,
                         shortHelp, longHelp, clientData, xPos, yPos);
    }

    wxToolBarToolBase *InsertTool(size_t pos,
                                  int toolid,
                                  const wxBitmap& bitmap,
                                  const wxBitmap& bmpDisabled = wxNullBitmap,
                                  bool toggle = false,
                                  wxObject *clientData = NULL,
                                  const wxString& shortHelp = wxEmptyString,
                                  const wxString& longHelp = wxEmptyString)
    {
        return InsertTool(pos, toolid, wxEmptyString, bitmap, bmpDisabled,
                          toggle ? wxITEM_CHECK : wxITEM_NORMAL,
                          shortHelp, longHelp, clientData);
    }

    // event handlers
    // --------------

    // NB: these functions are deprecated, use EVT_TOOL_XXX() instead!

    // Only allow toggle if returns true. Call when left button up.
    virtual bool OnLeftClick(int toolid, bool toggleDown);

    // Call when right button down.
    virtual void OnRightClick(int toolid, long x, long y);

    // Called when the mouse cursor enters a tool bitmap.
    // Argument is wxID_ANY if mouse is exiting the toolbar.
    virtual void OnMouseEnter(int toolid);

    // more deprecated functions
    // -------------------------

    // use GetToolMargins() instead
    wxSize GetMargins() const { return GetToolMargins(); }

    // implementation only from now on
    // -------------------------------

    size_t GetToolsCount() const { return m_tools.GetCount(); }

    // Do the toolbar button updates (check for EVT_UPDATE_UI handlers)
    virtual void UpdateWindowUI(long flags = wxUPDATE_UI_NONE) ;

    // don't want toolbars to accept the focus
    virtual bool AcceptsFocus() const { return false; }

protected:
    // to implement in derived classes
    // -------------------------------

    // create a new toolbar tool and add it to the toolbar, this is typically
    // implemented by just calling InsertTool()
    virtual wxToolBarToolBase *DoAddTool
                               (
                                   int toolid,
                                   const wxString& label,
                                   const wxBitmap& bitmap,
                                   const wxBitmap& bmpDisabled,
                                   wxItemKind kind,
                                   const wxString& shortHelp = wxEmptyString,
                                   const wxString& longHelp = wxEmptyString,
                                   wxObject *clientData = NULL,
                                   wxCoord xPos = wxDefaultCoord,
                                   wxCoord yPos = wxDefaultCoord
                               );

    // the tool is not yet inserted into m_tools list when this function is
    // called and will only be added to it if this function succeeds
    virtual bool DoInsertTool(size_t pos, wxToolBarToolBase *tool) = 0;

    // the tool is still in m_tools list when this function is called, it will
    // only be deleted from it if it succeeds
    virtual bool DoDeleteTool(size_t pos, wxToolBarToolBase *tool) = 0;

    // called when the tools enabled flag changes
    virtual void DoEnableTool(wxToolBarToolBase *tool, bool enable) = 0;

    // called when the tool is toggled
    virtual void DoToggleTool(wxToolBarToolBase *tool, bool toggle) = 0;

    // called when the tools "can be toggled" flag changes
    virtual void DoSetToggle(wxToolBarToolBase *tool, bool toggle) = 0;

    // the functions to create toolbar tools
    virtual wxToolBarToolBase *CreateTool(int toolid,
                                          const wxString& label,
                                          const wxBitmap& bmpNormal,
                                          const wxBitmap& bmpDisabled,
                                          wxItemKind kind,
                                          wxObject *clientData,
                                          const wxString& shortHelp,
                                          const wxString& longHelp) = 0;

    virtual wxToolBarToolBase *CreateTool(wxControl *control) = 0;

    // helper functions
    // ----------------

    // call this from derived class ctor/Create() to ensure that we have either
    // wxTB_HORIZONTAL or wxTB_VERTICAL style, there is a lot of existing code
    // which randomly checks either one or the other of them and gets confused
    // if neither is set (and making one of them 0 is not an option neither as
    // then the existing tests would break down)
    void FixupStyle();

    // un-toggle all buttons in the same radio group
    void UnToggleRadioGroup(wxToolBarToolBase *tool);

    // the list of all our tools
    wxToolBarToolsList m_tools;

    // the offset of the first tool
    int m_xMargin;
    int m_yMargin;

    // the maximum number of toolbar rows/columns
    int m_maxRows;
    int m_maxCols;

    // the tool packing and separation
    int m_toolPacking,
        m_toolSeparation;

    // the size of the toolbar bitmaps
    wxCoord m_defaultWidth, m_defaultHeight;

private:
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxToolBarBase)
};

// Helper function for creating the image for disabled buttons
bool wxCreateGreyedImage(const wxImage& in, wxImage& out) ;

#endif // wxUSE_TOOLBAR

#endif
    // _WX_TBARBASE_H_

