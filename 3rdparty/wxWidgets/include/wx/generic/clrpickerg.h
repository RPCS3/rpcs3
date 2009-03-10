/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/clrpickerg.h
// Purpose:     wxGenericColourButton header
// Author:      Francesco Montorsi (based on Vadim Zeitlin's code)
// Modified by:
// Created:     14/4/2006
// Copyright:   (c) Vadim Zeitlin, Francesco Montorsi
// RCS-ID:      $Id: clrpickerg.h 40322 2006-07-25 11:41:53Z ABX $
// Licence:     wxWindows Licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CLRPICKER_H_
#define _WX_CLRPICKER_H_

#include "wx/button.h"
#include "wx/cmndata.h"

//-----------------------------------------------------------------------------
// wxGenericColourButton: a button which brings up a wxColourDialog
//-----------------------------------------------------------------------------

// show the colour in HTML form (#AABBCC) as colour button label
#define wxCLRBTN_SHOW_LABEL     100

// the default style
#define wxCLRBTN_DEFAULT_STYLE  (wxCLRBTN_SHOW_LABEL)


class WXDLLIMPEXP_CORE wxGenericColourButton : public wxButton,
                                               public wxColourPickerWidgetBase
{
public:
    wxGenericColourButton() {}
    wxGenericColourButton(wxWindow *parent,
                          wxWindowID id,
                          const wxColour& col = *wxBLACK,
                          const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxDefaultSize,
                          long style = wxCLRBTN_DEFAULT_STYLE,
                          const wxValidator& validator = wxDefaultValidator,
                          const wxString& name = wxColourPickerWidgetNameStr)
    {
        Create(parent, id, col, pos, size, style, validator, name);
    }

    virtual ~wxGenericColourButton() {}


public:     // API extensions specific for wxGenericColourButton

    // user can override this to init colour data in a different way
    virtual void InitColourData();

    // returns the colour data shown in wxColourDialog
    wxColourData *GetColourData() { return &ms_data; }


public:

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxColour& col = *wxBLACK,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxCLRBTN_DEFAULT_STYLE,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxColourPickerWidgetNameStr);

    void OnButtonClick(wxCommandEvent &);


protected:

    wxSize DoGetBestSize() const;

    void UpdateColour();

    // the colour data shown in wxColourPickerCtrlGeneric
    // controls. This member is static so that all colour pickers
    // in the program share the same set of custom colours.
    static wxColourData ms_data;

private:
   DECLARE_DYNAMIC_CLASS(wxGenericColourButton)
};


#endif // _WX_CLRPICKER_H_
