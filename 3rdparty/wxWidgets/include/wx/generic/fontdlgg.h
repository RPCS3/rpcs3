/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/fontdlgg.h
// Purpose:     wxGenericFontDialog
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: fontdlgg.h 39286 2006-05-23 11:16:47Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_FONTDLGG_H
#define _WX_GENERIC_FONTDLGG_H

#include "wx/defs.h"
#include "wx/gdicmn.h"
#include "wx/font.h"
#include "wx/dialog.h"
#include "wx/cmndata.h"

#ifdef __WXWINCE__
#define USE_SPINCTRL_FOR_POINT_SIZE 1
class WXDLLEXPORT wxSpinEvent;
#else
#define USE_SPINCTRL_FOR_POINT_SIZE 0
#endif

/*
 * FONT DIALOG
 */

class WXDLLEXPORT wxChoice;
class WXDLLEXPORT wxText;
class WXDLLEXPORT wxCheckBox;
class WXDLLEXPORT wxFontPreviewer;

enum
{
    wxID_FONT_UNDERLINE = 3000,
    wxID_FONT_STYLE,
    wxID_FONT_WEIGHT,
    wxID_FONT_FAMILY,
    wxID_FONT_COLOUR,
    wxID_FONT_SIZE
};

class WXDLLEXPORT wxGenericFontDialog : public wxFontDialogBase
{
public:
    wxGenericFontDialog() { Init(); }
    wxGenericFontDialog(wxWindow *parent)
        : wxFontDialogBase(parent) { Init(); }
    wxGenericFontDialog(wxWindow *parent, const wxFontData& data)
        : wxFontDialogBase(parent, data) { Init(); }
    virtual ~wxGenericFontDialog();

    virtual int ShowModal();

#if WXWIN_COMPATIBILITY_2_6
    // deprecated, for backwards compatibility only
    wxDEPRECATED( wxGenericFontDialog(wxWindow *parent, const wxFontData *data) );
#endif // WXWIN_COMPATIBILITY_2_6

    // Internal functions
    void OnCloseWindow(wxCloseEvent& event);

    virtual void CreateWidgets();
    virtual void InitializeFont();

    void OnChangeFont(wxCommandEvent& event);

#if USE_SPINCTRL_FOR_POINT_SIZE
    void OnChangeSize(wxSpinEvent& event);
#endif

protected:

    virtual bool DoCreate(wxWindow *parent);

private:

    // common part of all ctors
    void Init();

    void DoChangeFont();

    wxFont m_dialogFont;

    wxChoice *m_familyChoice;
    wxChoice *m_styleChoice;
    wxChoice *m_weightChoice;
    wxChoice *m_colourChoice;
    wxCheckBox *m_underLineCheckBox;

#if !USE_SPINCTRL_FOR_POINT_SIZE
    wxChoice   *m_pointSizeChoice;
#endif

    wxFontPreviewer *m_previewer;
    bool       m_useEvents;

    //  static bool fontDialogCancelled;
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxGenericFontDialog)
};

WXDLLEXPORT const wxChar *wxFontFamilyIntToString(int family);
WXDLLEXPORT const wxChar *wxFontWeightIntToString(int weight);
WXDLLEXPORT const wxChar *wxFontStyleIntToString(int style);
WXDLLEXPORT int wxFontFamilyStringToInt(wxChar *family);
WXDLLEXPORT int wxFontWeightStringToInt(wxChar *weight);
WXDLLEXPORT int wxFontStyleStringToInt(wxChar *style);

#if WXWIN_COMPATIBILITY_2_6
    // deprecated, for backwards compatibility only
inline wxGenericFontDialog::wxGenericFontDialog(wxWindow *parent, const wxFontData *data)
                           :wxFontDialogBase(parent) { Init(); InitFontData(data); Create(parent); }
#endif // WXWIN_COMPATIBILITY_2_6

#endif // _WX_GENERIC_FONTDLGG_H
