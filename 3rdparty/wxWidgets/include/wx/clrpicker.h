/////////////////////////////////////////////////////////////////////////////
// Name:        wx/clrpicker.h
// Purpose:     wxColourPickerCtrl base header
// Author:      Francesco Montorsi (based on Vadim Zeitlin's code)
// Modified by:
// Created:     14/4/2006
// Copyright:   (c) Vadim Zeitlin, Francesco Montorsi
// RCS-ID:      $Id: clrpicker.h 53135 2008-04-12 02:31:04Z VZ $
// Licence:     wxWindows Licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CLRPICKER_H_BASE_
#define _WX_CLRPICKER_H_BASE_

#include "wx/defs.h"


#if wxUSE_COLOURPICKERCTRL

#include "wx/pickerbase.h"


class WXDLLIMPEXP_FWD_CORE wxColourPickerEvent;

extern WXDLLEXPORT_DATA(const wxChar) wxColourPickerWidgetNameStr[];
extern WXDLLEXPORT_DATA(const wxChar) wxColourPickerCtrlNameStr[];


// ----------------------------------------------------------------------------
// wxColourPickerWidgetBase: a generic abstract interface which must be
//                           implemented by controls used by wxColourPickerCtrl
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxColourPickerWidgetBase
{
public:
    wxColourPickerWidgetBase() { m_colour = *wxBLACK; }
    virtual ~wxColourPickerWidgetBase() {}

    wxColour GetColour() const
        { return m_colour; }
    virtual void SetColour(const wxColour &col)
        { m_colour = col; UpdateColour(); }
    virtual void SetColour(const wxString &col)
        { m_colour.Set(col); UpdateColour(); }

protected:

    virtual void UpdateColour() = 0;

    // the current colour (may be invalid if none)
    wxColour m_colour;
};


// Styles which must be supported by all controls implementing wxColourPickerWidgetBase
// NB: these styles must be defined to carefully-chosen values to
//     avoid conflicts with wxButton's styles

// show the colour in HTML form (#AABBCC) as colour button label
// (instead of no label at all)
// NOTE: this style is supported just by wxColourButtonGeneric and
//       thus is not exposed in wxColourPickerCtrl
#define wxCLRP_SHOW_LABEL             0x0008

// map platform-dependent controls which implement the wxColourPickerWidgetBase
// under the name "wxColourPickerWidget".
// NOTE: wxColourPickerCtrl allocates a wxColourPickerWidget and relies on the
//       fact that all classes being mapped as wxColourPickerWidget have the
//       same prototype for their contructor (and also explains why we use
//       define instead of a typedef)
// since GTK > 2.4, there is GtkColorButton
#if defined(__WXGTK24__) && !defined(__WXUNIVERSAL__)
    #include "wx/gtk/clrpicker.h"
    #define wxColourPickerWidget      wxColourButton
#else
    #include "wx/generic/clrpickerg.h"
    #define wxColourPickerWidget      wxGenericColourButton
#endif


// ----------------------------------------------------------------------------
// wxColourPickerCtrl: platform-independent class which embeds a
// platform-dependent wxColourPickerWidget and, if wxCLRP_USE_TEXTCTRL style is
// used, a textctrl next to it.
// ----------------------------------------------------------------------------

#define wxCLRP_USE_TEXTCTRL       (wxPB_USE_TEXTCTRL)
#define wxCLRP_DEFAULT_STYLE      0

class WXDLLIMPEXP_CORE wxColourPickerCtrl : public wxPickerBase
{
public:
    wxColourPickerCtrl() : m_bIgnoreNextTextCtrlUpdate(false) {}
    virtual ~wxColourPickerCtrl() {}


    wxColourPickerCtrl(wxWindow *parent, wxWindowID id,
        const wxColour& col = *wxBLACK, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = wxCLRP_DEFAULT_STYLE,
        const wxValidator& validator = wxDefaultValidator,
        const wxString& name = wxColourPickerCtrlNameStr)
        : m_bIgnoreNextTextCtrlUpdate(false)
        { Create(parent, id, col, pos, size, style, validator, name); }

    bool Create(wxWindow *parent, wxWindowID id,
           const wxColour& col = *wxBLACK,
           const wxPoint& pos = wxDefaultPosition,
           const wxSize& size = wxDefaultSize,
           long style = wxCLRP_DEFAULT_STYLE,
           const wxValidator& validator = wxDefaultValidator,
           const wxString& name = wxColourPickerCtrlNameStr);


public:         // public API

    // get the colour chosen
    wxColour GetColour() const
        { return ((wxColourPickerWidget *)m_picker)->GetColour(); }

    // set currently displayed color
    void SetColour(const wxColour& col);

    // set colour using RGB(r,g,b) syntax or considering given text as a colour name;
    // returns true if the given text was successfully recognized.
    bool SetColour(const wxString& text);


public:        // internal functions

    // update the button colour to match the text control contents
    void UpdatePickerFromTextCtrl();

    // update the text control to match the button's colour
    void UpdateTextCtrlFromPicker();

    // event handler for our picker
    void OnColourChange(wxColourPickerEvent &);

protected:
    virtual long GetPickerStyle(long style) const
        { return (style & wxCLRP_SHOW_LABEL); }

    // true if the next UpdateTextCtrl() call is to ignore
    bool m_bIgnoreNextTextCtrlUpdate;

private:
    DECLARE_DYNAMIC_CLASS(wxColourPickerCtrl)
};


// ----------------------------------------------------------------------------
// wxColourPickerEvent: used by wxColourPickerCtrl only
// ----------------------------------------------------------------------------

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_CORE, wxEVT_COMMAND_COLOURPICKER_CHANGED, 1102)
END_DECLARE_EVENT_TYPES()

class WXDLLIMPEXP_CORE wxColourPickerEvent : public wxCommandEvent
{
public:
    wxColourPickerEvent() {}
    wxColourPickerEvent(wxObject *generator, int id, const wxColour &col)
        : wxCommandEvent(wxEVT_COMMAND_COLOURPICKER_CHANGED, id),
          m_colour(col)
    {
        SetEventObject(generator);
    }

    wxColour GetColour() const { return m_colour; }
    void SetColour(const wxColour &c) { m_colour = c; }


    // default copy ctor, assignment operator and dtor are ok
    virtual wxEvent *Clone() const { return new wxColourPickerEvent(*this); }

private:
    wxColour m_colour;

    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxColourPickerEvent)
};

// ----------------------------------------------------------------------------
// event types and macros
// ----------------------------------------------------------------------------

typedef void (wxEvtHandler::*wxColourPickerEventFunction)(wxColourPickerEvent&);

#define wxColourPickerEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxColourPickerEventFunction, &func)

#define EVT_COLOURPICKER_CHANGED(id, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_COLOURPICKER_CHANGED, id, wxColourPickerEventHandler(fn))


#endif // wxUSE_COLOURPICKERCTRL

#endif // _WX_CLRPICKER_H_BASE_

