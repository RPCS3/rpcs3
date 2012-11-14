/////////////////////////////////////////////////////////////////////////////
// Name:        wx/fontpicker.h
// Purpose:     wxFontPickerCtrl base header
// Author:      Francesco Montorsi
// Modified by:
// Created:     14/4/2006
// Copyright:   (c) Francesco Montorsi
// RCS-ID:      $Id: fontpicker.h 53135 2008-04-12 02:31:04Z VZ $
// Licence:     wxWindows Licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_FONTPICKER_H_BASE_
#define _WX_FONTPICKER_H_BASE_

#include "wx/defs.h"


#if wxUSE_FONTPICKERCTRL

#include "wx/pickerbase.h"


class WXDLLIMPEXP_FWD_CORE wxFontPickerEvent;

extern WXDLLEXPORT_DATA(const wxChar) wxFontPickerWidgetNameStr[];
extern WXDLLEXPORT_DATA(const wxChar) wxFontPickerCtrlNameStr[];


// ----------------------------------------------------------------------------
// wxFontPickerWidgetBase: a generic abstract interface which must be
//                         implemented by controls used by wxFontPickerCtrl
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxFontPickerWidgetBase
{
public:
    wxFontPickerWidgetBase() { m_selectedFont = *wxNORMAL_FONT; }
    virtual ~wxFontPickerWidgetBase() {}

    wxFont GetSelectedFont() const
        { return m_selectedFont; }
    virtual void SetSelectedFont(const wxFont &f)
        { m_selectedFont = f; UpdateFont(); }

protected:

    virtual void UpdateFont() = 0;

    // the current font (may be invalid if none)
    // NOTE: don't call this m_font as wxWindow::m_font already exists
    wxFont m_selectedFont;
};

// Styles which must be supported by all controls implementing wxFontPickerWidgetBase
// NB: these styles must be defined to carefully-chosen values to
//     avoid conflicts with wxButton's styles


// keeps the label of the button updated with the fontface name + font size
// E.g. choosing "Times New Roman bold, italic with size 10" from the fontdialog,
//      updates the wxFontButtonGeneric's label (overwriting any previous label)
//      with the "Times New Roman, 10" text (only fontface + fontsize is displayed
//      to avoid extralong labels).
#define wxFNTP_FONTDESC_AS_LABEL      0x0008

// uses the currently selected font to draw the label of the button
#define wxFNTP_USEFONT_FOR_LABEL      0x0010

// since GTK > 2.4, there is GtkFontButton
#if defined(__WXGTK24__) && !defined(__WXUNIVERSAL__)
    #include "wx/gtk/fontpicker.h"
    #define wxFontPickerWidget      wxFontButton
#else
    #include "wx/generic/fontpickerg.h"
    #define wxFontPickerWidget      wxGenericFontButton
#endif


// ----------------------------------------------------------------------------
// wxFontPickerCtrl specific flags
// ----------------------------------------------------------------------------

#define wxFNTP_USE_TEXTCTRL       (wxPB_USE_TEXTCTRL)
#define wxFNTP_DEFAULT_STYLE      (wxFNTP_FONTDESC_AS_LABEL|wxFNTP_USEFONT_FOR_LABEL)

// not a style but rather the default value of the maximum pointsize allowed
#define wxFNTP_MAXPOINT_SIZE      100


// ----------------------------------------------------------------------------
// wxFontPickerCtrl: platform-independent class which embeds the
// platform-dependent wxFontPickerWidget andm if wxFNTP_USE_TEXTCTRL style is
// used, a textctrl next to it.
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxFontPickerCtrl : public wxPickerBase
{
public:
    wxFontPickerCtrl()
        : m_bIgnoreNextTextCtrlUpdate(false),
        m_nMaxPointSize(wxFNTP_MAXPOINT_SIZE)
    {
    }

    virtual ~wxFontPickerCtrl() {}


    wxFontPickerCtrl(wxWindow *parent,
                     wxWindowID id,
                     const wxFont& initial = wxNullFont,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxFNTP_DEFAULT_STYLE,
                     const wxValidator& validator = wxDefaultValidator,
                     const wxString& name = wxFontPickerCtrlNameStr)
        : m_bIgnoreNextTextCtrlUpdate(false),
          m_nMaxPointSize(wxFNTP_MAXPOINT_SIZE)
    {
        Create(parent, id, initial, pos, size, style, validator, name);
    }

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxFont& initial = wxNullFont,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxFNTP_DEFAULT_STYLE,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxFontPickerCtrlNameStr);


public:         // public API

    // get the font chosen
    wxFont GetSelectedFont() const
        { return ((wxFontPickerWidget *)m_picker)->GetSelectedFont(); }

    // sets currently displayed font
    void SetSelectedFont(const wxFont& f);

    // set/get the max pointsize
    void SetMaxPointSize(unsigned int max)
        { m_nMaxPointSize=max; }
    unsigned int GetMaxPointSize() const
        { return m_nMaxPointSize; }

public:        // internal functions

    void UpdatePickerFromTextCtrl();
    void UpdateTextCtrlFromPicker();

    // event handler for our picker
    void OnFontChange(wxFontPickerEvent &);

    // used to convert wxString <-> wxFont
    virtual wxString Font2String(const wxFont &font);
    virtual wxFont String2Font(const wxString &font);

protected:

    // extracts the style for our picker from wxFontPickerCtrl's style
    long GetPickerStyle(long style) const
        { return (style & (wxFNTP_FONTDESC_AS_LABEL|wxFNTP_USEFONT_FOR_LABEL)); }

    // true if the next UpdateTextCtrl() call is to ignore
    bool m_bIgnoreNextTextCtrlUpdate;

    // the maximum pointsize allowed to the user
    unsigned int m_nMaxPointSize;

private:
    DECLARE_DYNAMIC_CLASS(wxFontPickerCtrl)
};


// ----------------------------------------------------------------------------
// wxFontPickerEvent: used by wxFontPickerCtrl only
// ----------------------------------------------------------------------------

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_CORE, wxEVT_COMMAND_FONTPICKER_CHANGED, 1102)
END_DECLARE_EVENT_TYPES()

class WXDLLIMPEXP_CORE wxFontPickerEvent : public wxCommandEvent
{
public:
    wxFontPickerEvent() {}
    wxFontPickerEvent(wxObject *generator, int id, const wxFont &f)
        : wxCommandEvent(wxEVT_COMMAND_FONTPICKER_CHANGED, id),
          m_font(f)
    {
        SetEventObject(generator);
    }

    wxFont GetFont() const { return m_font; }
    void SetFont(const wxFont &c) { m_font = c; }

    // default copy ctor, assignment operator and dtor are ok
    virtual wxEvent *Clone() const { return new wxFontPickerEvent(*this); }

private:
    wxFont m_font;

    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxFontPickerEvent)
};

// ----------------------------------------------------------------------------
// event types and macros
// ----------------------------------------------------------------------------

typedef void (wxEvtHandler::*wxFontPickerEventFunction)(wxFontPickerEvent&);

#define wxFontPickerEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxFontPickerEventFunction, &func)

#define EVT_FONTPICKER_CHANGED(id, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_FONTPICKER_CHANGED, id, wxFontPickerEventHandler(fn))


#endif // wxUSE_FONTPICKERCTRL

#endif
    // _WX_FONTPICKER_H_BASE_
