/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/fontdlg.h
// Purpose:     wxFontDialog class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: fontdlg.h 38448 2006-03-30 14:04:17Z ABX $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_FONTDLG_H_
#define _WX_MSW_FONTDLG_H_

// ----------------------------------------------------------------------------
// wxFontDialog
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxFontDialog : public wxFontDialogBase
{
public:
    wxFontDialog() : wxFontDialogBase() { /* must be Create()d later */ }
    wxFontDialog(wxWindow *parent)
        : wxFontDialogBase(parent) { Create(parent); }
    wxFontDialog(wxWindow *parent, const wxFontData& data)
        : wxFontDialogBase(parent, data) { Create(parent, data); }

    virtual int ShowModal();

#if WXWIN_COMPATIBILITY_2_6
    // deprecated interface, don't use
    wxDEPRECATED( wxFontDialog(wxWindow *parent, const wxFontData *data) );
#endif // WXWIN_COMPATIBILITY_2_6

protected:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxFontDialog)
};

#if WXWIN_COMPATIBILITY_2_6
    // deprecated interface, don't use
inline wxFontDialog::wxFontDialog(wxWindow *parent, const wxFontData *data)
        : wxFontDialogBase(parent) { InitFontData(data); Create(parent); }
#endif // WXWIN_COMPATIBILITY_2_6

#endif
    // _WX_MSW_FONTDLG_H_
