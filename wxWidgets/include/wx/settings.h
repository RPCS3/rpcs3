/////////////////////////////////////////////////////////////////////////////
// Name:        wx/settings.h
// Purpose:     wxSystemSettings class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: settings.h 67017 2011-02-25 09:37:28Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_SETTINGS_H_BASE_
#define _WX_SETTINGS_H_BASE_

#include "wx/colour.h"
#include "wx/font.h"

class WXDLLIMPEXP_FWD_CORE wxWindow;

// possible values for wxSystemSettings::GetFont() parameter
//
// NB: wxMSW assumes that they have the same values as the parameters of
//     Windows GetStockObject() API, don't change the values!
enum wxSystemFont
{
    wxSYS_OEM_FIXED_FONT = 10,
    wxSYS_ANSI_FIXED_FONT,
    wxSYS_ANSI_VAR_FONT,
    wxSYS_SYSTEM_FONT,
    wxSYS_DEVICE_DEFAULT_FONT,
    wxSYS_DEFAULT_PALETTE,
    wxSYS_SYSTEM_FIXED_FONT,
    wxSYS_DEFAULT_GUI_FONT,

    // this was just a temporary aberration, do not use it any more
    wxSYS_ICONTITLE_FONT = wxSYS_DEFAULT_GUI_FONT
};

// possible values for wxSystemSettings::GetColour() parameter
//
// NB: wxMSW assumes that they have the same values as the parameters of
//     Windows GetSysColor() API, don't change the values!
enum wxSystemColour
{
    wxSYS_COLOUR_SCROLLBAR,
    wxSYS_COLOUR_BACKGROUND,
    wxSYS_COLOUR_DESKTOP = wxSYS_COLOUR_BACKGROUND,
    wxSYS_COLOUR_ACTIVECAPTION,
    wxSYS_COLOUR_INACTIVECAPTION,
    wxSYS_COLOUR_MENU,
    wxSYS_COLOUR_WINDOW,
    wxSYS_COLOUR_WINDOWFRAME,
    wxSYS_COLOUR_MENUTEXT,
    wxSYS_COLOUR_WINDOWTEXT,
    wxSYS_COLOUR_CAPTIONTEXT,
    wxSYS_COLOUR_ACTIVEBORDER,
    wxSYS_COLOUR_INACTIVEBORDER,
    wxSYS_COLOUR_APPWORKSPACE,
    wxSYS_COLOUR_HIGHLIGHT,
    wxSYS_COLOUR_HIGHLIGHTTEXT,
    wxSYS_COLOUR_BTNFACE,
    wxSYS_COLOUR_3DFACE = wxSYS_COLOUR_BTNFACE,
    wxSYS_COLOUR_BTNSHADOW,
    wxSYS_COLOUR_3DSHADOW = wxSYS_COLOUR_BTNSHADOW,
    wxSYS_COLOUR_GRAYTEXT,
    wxSYS_COLOUR_BTNTEXT,
    wxSYS_COLOUR_INACTIVECAPTIONTEXT,
    wxSYS_COLOUR_BTNHIGHLIGHT,
    wxSYS_COLOUR_BTNHILIGHT = wxSYS_COLOUR_BTNHIGHLIGHT,
    wxSYS_COLOUR_3DHIGHLIGHT = wxSYS_COLOUR_BTNHIGHLIGHT,
    wxSYS_COLOUR_3DHILIGHT = wxSYS_COLOUR_BTNHIGHLIGHT,
    wxSYS_COLOUR_3DDKSHADOW,
    wxSYS_COLOUR_3DLIGHT,
    wxSYS_COLOUR_INFOTEXT,
    wxSYS_COLOUR_INFOBK,
    wxSYS_COLOUR_LISTBOX,
    wxSYS_COLOUR_HOTLIGHT,
    wxSYS_COLOUR_GRADIENTACTIVECAPTION,
    wxSYS_COLOUR_GRADIENTINACTIVECAPTION,
    wxSYS_COLOUR_MENUHILIGHT,
    wxSYS_COLOUR_MENUBAR,
    wxSYS_COLOUR_LISTBOXTEXT,
    wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT,

    wxSYS_COLOUR_MAX
};

// possible values for wxSystemSettings::GetMetric() index parameter
//
// NB: update the conversion table in msw/settings.cpp if you change the values
//     of the elements of this enum
enum wxSystemMetric
{
    wxSYS_MOUSE_BUTTONS = 1,
    wxSYS_BORDER_X,
    wxSYS_BORDER_Y,
    wxSYS_CURSOR_X,
    wxSYS_CURSOR_Y,
    wxSYS_DCLICK_X,
    wxSYS_DCLICK_Y,
    wxSYS_DRAG_X,
    wxSYS_DRAG_Y,
    wxSYS_EDGE_X,
    wxSYS_EDGE_Y,
    wxSYS_HSCROLL_ARROW_X,
    wxSYS_HSCROLL_ARROW_Y,
    wxSYS_HTHUMB_X,
    wxSYS_ICON_X,
    wxSYS_ICON_Y,
    wxSYS_ICONSPACING_X,
    wxSYS_ICONSPACING_Y,
    wxSYS_WINDOWMIN_X,
    wxSYS_WINDOWMIN_Y,
    wxSYS_SCREEN_X,
    wxSYS_SCREEN_Y,
    wxSYS_FRAMESIZE_X,
    wxSYS_FRAMESIZE_Y,
    wxSYS_SMALLICON_X,
    wxSYS_SMALLICON_Y,
    wxSYS_HSCROLL_Y,
    wxSYS_VSCROLL_X,
    wxSYS_VSCROLL_ARROW_X,
    wxSYS_VSCROLL_ARROW_Y,
    wxSYS_VTHUMB_Y,
    wxSYS_CAPTION_Y,
    wxSYS_MENU_Y,
    wxSYS_NETWORK_PRESENT,
    wxSYS_PENWINDOWS_PRESENT,
    wxSYS_SHOW_SOUNDS,
    wxSYS_SWAP_BUTTONS
};

// possible values for wxSystemSettings::HasFeature() parameter
enum wxSystemFeature
{
    wxSYS_CAN_DRAW_FRAME_DECORATIONS = 1,
    wxSYS_CAN_ICONIZE_FRAME,
    wxSYS_TABLET_PRESENT
};

// values for different screen designs
enum wxSystemScreenType
{
    wxSYS_SCREEN_NONE = 0,  //   not yet defined

    wxSYS_SCREEN_TINY,      //   <
    wxSYS_SCREEN_PDA,       //   >= 320x240
    wxSYS_SCREEN_SMALL,     //   >= 640x480
    wxSYS_SCREEN_DESKTOP    //   >= 800x600
};

// ----------------------------------------------------------------------------
// wxSystemSettingsNative: defines the API for wxSystemSettings class
// ----------------------------------------------------------------------------

// this is a namespace rather than a class: it has only non virtual static
// functions
//
// also note that the methods are implemented in the platform-specific source
// files (i.e. this is not a real base class as we can't override its virtual
// functions because it doesn't have any)

class WXDLLEXPORT wxSystemSettingsNative
{
public:
    // get a standard system colour
    static wxColour GetColour(wxSystemColour index);

    // get a standard system font
    static wxFont GetFont(wxSystemFont index);

    // get a system-dependent metric
    static int GetMetric(wxSystemMetric index, wxWindow * win = NULL);

    // return true if the port has certain feature
    static bool HasFeature(wxSystemFeature index);
};

// ----------------------------------------------------------------------------
// include the declaration of the real platform-dependent class
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxSystemSettings : public wxSystemSettingsNative
{
public:
#ifdef __WXUNIVERSAL__
    // in wxUniversal we want to use the theme standard colours instead of the
    // system ones, otherwise wxSystemSettings is just the same as
    // wxSystemSettingsNative
    static wxColour GetColour(wxSystemColour index);
#endif // __WXUNIVERSAL__

    // Get system screen design (desktop, pda, ..) used for
    // laying out various dialogs.
    static wxSystemScreenType GetScreenType();

    // Override default.
    static void SetScreenType( wxSystemScreenType screen );

    // Value
    static wxSystemScreenType ms_screen;

#if WXWIN_COMPATIBILITY_2_4
    // the backwards compatible versions of wxSystemSettingsNative functions,
    // don't use these methods in the new code!
    wxDEPRECATED(static wxColour GetSystemColour(int index));
    wxDEPRECATED(static wxFont GetSystemFont(int index));
    wxDEPRECATED(static int GetSystemMetric(int index));
#endif
};

#endif
    // _WX_SETTINGS_H_BASE_

