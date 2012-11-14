/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/fontpickerg.h
// Purpose:     wxGenericFontButton header
// Author:      Francesco Montorsi
// Modified by:
// Created:     14/4/2006
// Copyright:   (c) Francesco Montorsi
// RCS-ID:      $Id: fontpickerg.h 42999 2006-11-03 21:54:13Z VZ $
// Licence:     wxWindows Licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_FONTPICKER_H_
#define _WX_FONTPICKER_H_

#include "wx/button.h"
#include "wx/cmndata.h"

//-----------------------------------------------------------------------------
// wxGenericFontButton: a button which brings up a wxColourDialog
//-----------------------------------------------------------------------------

#define wxFONTBTN_DEFAULT_STYLE \
    (wxFNTP_FONTDESC_AS_LABEL | wxFNTP_USEFONT_FOR_LABEL)

class WXDLLIMPEXP_CORE wxGenericFontButton : public wxButton,
                                             public wxFontPickerWidgetBase
{
public:
    wxGenericFontButton() {}
    wxGenericFontButton(wxWindow *parent,
                        wxWindowID id,
                        const wxFont &initial = wxNullFont,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        long style = wxFONTBTN_DEFAULT_STYLE,
                        const wxValidator& validator = wxDefaultValidator,
                        const wxString& name = wxFontPickerWidgetNameStr)
    {
        Create(parent, id, initial, pos, size, style, validator, name);
    }

    virtual ~wxGenericFontButton() {}


public:     // API extensions specific for wxGenericFontButton

    // user can override this to init font data in a different way
    virtual void InitFontData();

    // returns the font data shown in wxColourDialog
    wxFontData *GetFontData() { return &ms_data; }


public:

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxFont &initial = *wxNORMAL_FONT,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxFONTBTN_DEFAULT_STYLE,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxFontPickerWidgetNameStr);

    void OnButtonClick(wxCommandEvent &);


protected:

    void UpdateFont();

    // the colour data shown in wxColourPickerCtrlGeneric
    // controls. This member is static so that all colour pickers
    // in the program share the same set of custom colours.
    static wxFontData ms_data;

private:
   DECLARE_DYNAMIC_CLASS(wxGenericFontButton)
};


#endif // _WX_FONTPICKER_H_
