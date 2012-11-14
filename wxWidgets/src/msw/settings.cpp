/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/settings.cpp
// Purpose:     wxSystemSettingsNative implementation for MSW
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: settings.cpp 67017 2011-02-25 09:37:28Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/settings.h"

#ifndef WX_PRECOMP
    #include "wx/utils.h"
    #include "wx/gdicmn.h"
    #include "wx/module.h"
#endif

#include "wx/msw/private.h"
#include "wx/msw/missing.h" // for SM_CXCURSOR, SM_CYCURSOR, SM_TABLETPC

#ifndef SPI_GETFLATMENU
#define SPI_GETFLATMENU                     0x1022
#endif

#include "wx/fontutil.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// the module which is used to clean up wxSystemSettingsNative data (this is a
// singleton class so it can't be done in the dtor)
class wxSystemSettingsModule : public wxModule
{
public:
    virtual bool OnInit();
    virtual void OnExit();

private:
    DECLARE_DYNAMIC_CLASS(wxSystemSettingsModule)
};

// ----------------------------------------------------------------------------
// global data
// ----------------------------------------------------------------------------

// the font returned by GetFont(wxSYS_DEFAULT_GUI_FONT): it is created when
// GetFont() is called for the first time and deleted by wxSystemSettingsModule
static wxFont *gs_fontDefault = NULL;

// ============================================================================
// implementation
// ============================================================================

// TODO: see ::SystemParametersInfo for all sorts of Windows settings.
// Different args are required depending on the id. How does this differ
// from GetSystemMetric, and should it? Perhaps call it GetSystemParameter
// and pass an optional void* arg to get further info.
// Should also have SetSystemParameter.
// Also implement WM_WININICHANGE (NT) / WM_SETTINGCHANGE (Win95)

// ----------------------------------------------------------------------------
// wxSystemSettingsModule
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxSystemSettingsModule, wxModule)

bool wxSystemSettingsModule::OnInit()
{
    return true;
}

void wxSystemSettingsModule::OnExit()
{
    delete gs_fontDefault;
    gs_fontDefault = NULL;
}

// ----------------------------------------------------------------------------
// wxSystemSettingsNative
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// colours
// ----------------------------------------------------------------------------

wxColour wxSystemSettingsNative::GetColour(wxSystemColour index)
{
    // we use 0 as the default value just to avoid compiler warnings, as there
    // is no invalid colour value we use hasCol as the real indicator of
    // whether colSys was initialized or not
    COLORREF colSys = 0;
    bool hasCol = false;

    // the default colours for the entries after BTNHIGHLIGHT
    static const COLORREF s_defaultSysColors[] =
    {
        0x000000,   // 3DDKSHADOW
        0xdfdfdf,   // 3DLIGHT
        0x000000,   // INFOTEXT
        0xe1ffff,   // INFOBK

        0,          // filler - no std colour with this index

        // TODO: please fill in the standard values of those, I don't have them
        0,          // HOTLIGHT
        0,          // GRADIENTACTIVECAPTION
        0,          // GRADIENTINACTIVECAPTION
        0,          // MENU
        0,          // MENUBAR (unused)
    };

    if ( index == wxSYS_COLOUR_LISTBOXTEXT)
    {
        // there is no standard colour with this index, map to another one
        index = wxSYS_COLOUR_WINDOWTEXT;
    }
    else if ( index == wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT)
    {
        // there is no standard colour with this index, map to another one
        index = wxSYS_COLOUR_HIGHLIGHTTEXT;
    }
    else if ( index == wxSYS_COLOUR_LISTBOX )
    {
        // there is no standard colour with this index, map to another one
        index = wxSYS_COLOUR_WINDOW;
    }
    else if ( index > wxSYS_COLOUR_BTNHIGHLIGHT )
    {
        // the indices before BTNHIGHLIGHT are understood by GetSysColor() in
        // all Windows version, for the other ones we have to check
        bool useDefault;

        int verMaj, verMin;
        wxGetOsVersion(&verMaj, &verMin);
        if ( verMaj < 4 )
        {
            // NT 3.5
            useDefault = true;
        }
        else if ( verMaj == 4 )
        {
            // Win95/NT 4.0
            useDefault = index > wxSYS_COLOUR_INFOBK;
        }
        else if ( verMaj == 5 && verMin == 0 )
        {
            // Win98/Win2K
            useDefault = index > wxSYS_COLOUR_GRADIENTINACTIVECAPTION;
        }
        else // >= 5.1
        {
            // 5.1 is Windows XP
            useDefault = false;
            // Determine if we are using flat menus, only then allow wxSYS_COLOUR_MENUBAR
            if ( index == wxSYS_COLOUR_MENUBAR )
            {
                BOOL isFlat ;
                if ( SystemParametersInfo( SPI_GETFLATMENU , 0 ,&isFlat, 0 ) )
                {
                    if ( !isFlat )
                        index = wxSYS_COLOUR_MENU ;
                }
            }
        }

        if ( useDefault )
        {
            // special handling for MENUBAR colour: we use this in wxToolBar
            // and wxStatusBar to have correct bg colour under Windows XP
            // (which uses COLOR_MENUBAR for them) but they should still look
            // correctly under previous Windows versions as well
            if ( index == wxSYS_COLOUR_MENUBAR )
            {
                index = wxSYS_COLOUR_3DFACE;
            }
            else // replace with default colour
            {
                unsigned int n = index - wxSYS_COLOUR_BTNHIGHLIGHT;

                wxASSERT_MSG( n < WXSIZEOF(s_defaultSysColors),
                              _T("forgot tp update the default colours array") );

                colSys = s_defaultSysColors[n];
                hasCol = true;
            }
        }
    }

    if ( !hasCol )
    {
#ifdef __WXWINCE__
        colSys = ::GetSysColor(index|SYS_COLOR_INDEX_FLAG);
#else
        colSys = ::GetSysColor(index);
#endif
    }

    return wxRGBToColour(colSys);
}

// ----------------------------------------------------------------------------
// fonts
// ----------------------------------------------------------------------------

wxFont wxCreateFontFromStockObject(int index)
{
    wxFont font;

    HFONT hFont = (HFONT) ::GetStockObject(index);
    if ( hFont )
    {
        LOGFONT lf;
        if ( ::GetObject(hFont, sizeof(LOGFONT), &lf) != 0 )
        {
            wxNativeFontInfo info;
            info.lf = lf;
#ifndef __WXWINCE__
            // We want Windows 2000 or later to have new fonts even MS Shell Dlg
            // is returned as default GUI font for compatibility
            int verMaj;
            if(index == DEFAULT_GUI_FONT && wxGetOsVersion(&verMaj) == wxOS_WINDOWS_NT && verMaj >= 5)
                wxStrcpy(info.lf.lfFaceName, wxT("MS Shell Dlg 2"));
#endif
            // Under MicroWindows we pass the HFONT as well
            // because it's hard to convert HFONT -> LOGFONT -> HFONT
            // It's OK to delete stock objects, the delete will be ignored.
#ifdef __WXMICROWIN__
            font.Create(info, (WXHFONT) hFont);
#else
            font.Create(info);
#endif
        }
        else
        {
            wxFAIL_MSG( _T("failed to get LOGFONT") );
        }
    }
    else // GetStockObject() failed
    {
        wxFAIL_MSG( _T("stock font not found") );
    }

    return font;
}

wxFont wxSystemSettingsNative::GetFont(wxSystemFont index)
{
#ifdef __WXWINCE__
    // under CE only a single SYSTEM_FONT exists
    index;

    if ( !gs_fontDefault )
    {
        gs_fontDefault = new wxFont(wxCreateFontFromStockObject(SYSTEM_FONT));
    }

    return *gs_fontDefault;
#else // !__WXWINCE__
    // wxWindow ctor calls GetFont(wxSYS_DEFAULT_GUI_FONT) so we're
    // called fairly often -- this is why we cache this particular font
    const bool isDefaultRequested = index == wxSYS_DEFAULT_GUI_FONT;
    if ( isDefaultRequested )
    {
        if ( gs_fontDefault )
            return *gs_fontDefault;
    }

    wxFont font = wxCreateFontFromStockObject(index);

    if ( isDefaultRequested )
    {
        // if we got here it means we hadn't cached it yet - do now
        gs_fontDefault = new wxFont(font);
    }

    return font;
#endif // __WXWINCE__/!__WXWINCE__
}

// ----------------------------------------------------------------------------
// system metrics/features
// ----------------------------------------------------------------------------

// TODO: some of the "metrics" clearly should be features now that we have
//       HasFeature()!

// the conversion table from wxSystemMetric enum to GetSystemMetrics() param
//
// if the constant is not defined, put -1 in the table to indicate that it is
// unknown
static const int gs_metricsMap[] =
{
    -1,  // wxSystemMetric enums start at 1, so give a dummy value for pos 0.
#if defined(__WIN32__) && !defined(__WXWINCE__)
    SM_CMOUSEBUTTONS,
#else
    -1,
#endif

    SM_CXBORDER,
    SM_CYBORDER,
#ifdef SM_CXCURSOR
    SM_CXCURSOR,
    SM_CYCURSOR,
#else
    -1, -1,
#endif
    SM_CXDOUBLECLK,
    SM_CYDOUBLECLK,
#if defined(__WIN32__) && defined(SM_CXDRAG)
    SM_CXDRAG,
    SM_CYDRAG,
    SM_CXEDGE,
    SM_CYEDGE,
#else
    -1, -1, -1, -1,
#endif
    SM_CXHSCROLL,
    SM_CYHSCROLL,
#ifdef SM_CXHTHUMB
    SM_CXHTHUMB,
#else
    -1,
#endif
    SM_CXICON,
    SM_CYICON,
    SM_CXICONSPACING,
    SM_CYICONSPACING,
#ifdef SM_CXHTHUMB
    SM_CXMIN,
    SM_CYMIN,
#else
    -1, -1,
#endif
    SM_CXSCREEN,
    SM_CYSCREEN,

#if defined(__WIN32__) && defined(SM_CXSIZEFRAME)
    SM_CXSIZEFRAME,
    SM_CYSIZEFRAME,
    SM_CXSMICON,
    SM_CYSMICON,
#else
    -1, -1, -1, -1,
#endif
    SM_CYHSCROLL,
    SM_CXHSCROLL,
    SM_CXVSCROLL,
    SM_CYVSCROLL,
#ifdef SM_CYVTHUMB
    SM_CYVTHUMB,
#else
    -1,
#endif
    SM_CYCAPTION,
    SM_CYMENU,
#if defined(__WIN32__) && defined(SM_NETWORK)
    SM_NETWORK,
#else
    -1,
#endif
#ifdef SM_PENWINDOWS
    SM_PENWINDOWS,
#else
    -1,
#endif
#if defined(__WIN32__) && defined(SM_SHOWSOUNDS)
    SM_SHOWSOUNDS,
#else
    -1,
#endif
#ifdef SM_SWAPBUTTON
    SM_SWAPBUTTON,
#else
    -1
#endif
};

// Get a system metric, e.g. scrollbar size
int wxSystemSettingsNative::GetMetric(wxSystemMetric index, wxWindow* WXUNUSED(win))
{
#ifdef __WXMICROWIN__
    // TODO: probably use wxUniv themes functionality
    return 0;
#else // !__WXMICROWIN__
    wxCHECK_MSG( index > 0 && (size_t)index < WXSIZEOF(gs_metricsMap), 0,
                 _T("invalid metric") );

    int indexMSW = gs_metricsMap[index];
    if ( indexMSW == -1 )
    {
        // not supported under current system
        return -1;
    }

    int rc = ::GetSystemMetrics(indexMSW);
    if ( index == wxSYS_NETWORK_PRESENT )
    {
        // only the last bit is significant according to the MSDN
        rc &= 1;
    }

    return rc;
#endif // __WXMICROWIN__/!__WXMICROWIN__
}

bool wxSystemSettingsNative::HasFeature(wxSystemFeature index)
{
    switch ( index )
    {
        case wxSYS_CAN_ICONIZE_FRAME:
        case wxSYS_CAN_DRAW_FRAME_DECORATIONS:
            return true;

        case wxSYS_TABLET_PRESENT:
            return ::GetSystemMetrics(SM_TABLETPC) != 0;

        default:
            wxFAIL_MSG( _T("unknown system feature") );

            return false;
    }
}

// ----------------------------------------------------------------------------
// function from wx/msw/wrapcctl.h: there is really no other place for it...
// ----------------------------------------------------------------------------

#if wxUSE_LISTCTRL || wxUSE_TREECTRL

extern wxFont wxGetCCDefaultFont()
{
#ifndef __WXWINCE__
    // under the systems enumerated below (anything released after Win98), the
    // default font used for the common controls seems to be the desktop font
    // which is also used for the icon titles and not the stock default GUI
    // font
    bool useIconFont;
    int verMaj, verMin;
    switch ( wxGetOsVersion(&verMaj, &verMin) )
    {
        case wxOS_WINDOWS_9X:
            // 4.10 is Win98
            useIconFont = verMaj == 4 && verMin >= 10;
            break;

        case wxOS_WINDOWS_NT:
            // 5.0 is Win2k
            useIconFont = verMaj >= 5;
            break;

        default:
            useIconFont = false;
    }

    if ( useIconFont )
    {
        LOGFONT lf;
        if ( ::SystemParametersInfo
               (
                    SPI_GETICONTITLELOGFONT,
                    sizeof(lf),
                    &lf,
                    0
               ) )
        {
            return wxFont(wxCreateFontFromLogFont(&lf));
        }
        else
        {
            wxLogLastError(_T("SystemParametersInfo(SPI_GETICONTITLELOGFONT"));
        }
    }
#endif // __WXWINCE__

    // fall back to the default font for the normal controls
    return wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
}

#endif // wxUSE_LISTCTRL || wxUSE_TREECTRL
