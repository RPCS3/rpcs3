///////////////////////////////////////////////////////////////////////////////
// Name:        include/wx/msw/uxtheme.h
// Purpose:     wxUxThemeEngine class: support for XP themes
// Author:      John Platts, Vadim Zeitlin
// Modified by:
// Created:     2003
// RCS-ID:      $Id: uxtheme.h 42725 2006-10-30 15:37:42Z VZ $
// Copyright:   (c) 2003 John Platts, Vadim Zeitlin
// License:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UXTHEME_H_
#define _WX_UXTHEME_H_

#include "wx/defs.h"

#include "wx/msw/private.h"     // we use GetHwndOf()
#include "wx/msw/uxthemep.h"

typedef HTHEME  (__stdcall *PFNWXUOPENTHEMEDATA)(HWND, const wchar_t *);
typedef HRESULT (__stdcall *PFNWXUCLOSETHEMEDATA)(HTHEME);
typedef HRESULT (__stdcall *PFNWXUDRAWTHEMEBACKGROUND)(HTHEME, HDC, int, int, const RECT *, const RECT *);
typedef HRESULT (__stdcall *PFNWXUDRAWTHEMETEXT)(HTHEME, HDC, int, int, const wchar_t *, int, DWORD, DWORD, const RECT *);
typedef HRESULT (__stdcall *PFNWXUGETTHEMEBACKGROUNDCONTENTRECT)(HTHEME, HDC, int, int, const RECT *, RECT *);
typedef HRESULT (__stdcall *PFNWXUGETTHEMEBACKGROUNDEXTENT)(HTHEME, HDC, int, int, const RECT *, RECT *);
typedef HRESULT (__stdcall *PFNWXUGETTHEMEPARTSIZE)(HTHEME, HDC, int, int, const RECT *, /* enum */ THEMESIZE, SIZE *);
typedef HRESULT (__stdcall *PFNWXUGETTHEMETEXTEXTENT)(HTHEME, HDC, int, int, const wchar_t *, int, DWORD, const RECT *, RECT *);
typedef HRESULT (__stdcall *PFNWXUGETTHEMETEXTMETRICS)(HTHEME, HDC, int, int, TEXTMETRIC*);
typedef HRESULT (__stdcall *PFNWXUGETTHEMEBACKGROUNDREGION)(HTHEME, HDC, int, int, const RECT *, HRGN *);
typedef HRESULT (__stdcall *PFNWXUHITTESTTHEMEBACKGROUND)(HTHEME, HDC, int, int, DWORD, const RECT *, HRGN, POINT, unsigned short *);
typedef HRESULT (__stdcall *PFNWXUDRAWTHEMEEDGE)(HTHEME, HDC, int, int, const RECT *, unsigned int, unsigned int, RECT *);
typedef HRESULT (__stdcall *PFNWXUDRAWTHEMEICON)(HTHEME, HDC, int, int, const RECT *, HIMAGELIST, int);
typedef BOOL    (__stdcall *PFNWXUISTHEMEPARTDEFINED)(HTHEME, int, int);
typedef BOOL    (__stdcall *PFNWXUISTHEMEBACKGROUNDPARTIALLYTRANSPARENT)(HTHEME, int, int);
typedef HRESULT (__stdcall *PFNWXUGETTHEMECOLOR)(HTHEME, int, int, int, COLORREF*);
typedef HRESULT (__stdcall *PFNWXUGETTHEMEMETRIC)(HTHEME, HDC, int, int, int, int *);
typedef HRESULT (__stdcall *PFNWXUGETTHEMESTRING)(HTHEME, int, int, int, wchar_t *, int);
typedef HRESULT (__stdcall *PFNWXUGETTHEMEBOOL)(HTHEME, int, int, int, BOOL *);
typedef HRESULT (__stdcall *PFNWXUGETTHEMEINT)(HTHEME, int, int, int, int *);
typedef HRESULT (__stdcall *PFNWXUGETTHEMEENUMVALUE)(HTHEME, int, int, int, int *);
typedef HRESULT (__stdcall *PFNWXUGETTHEMEPOSITION)(HTHEME, int, int, int, POINT *);
typedef HRESULT (__stdcall *PFNWXUGETTHEMEFONT)(HTHEME, HDC, int, int, int, LOGFONT *);
typedef HRESULT (__stdcall *PFNWXUGETTHEMERECT)(HTHEME, int, int, int, RECT *);
typedef HRESULT (__stdcall *PFNWXUGETTHEMEMARGINS)(HTHEME, HDC, int, int, int, RECT *, MARGINS *);
typedef HRESULT (__stdcall *PFNWXUGETTHEMEINTLIST)(HTHEME, int, int, int, INTLIST*);
typedef HRESULT (__stdcall *PFNWXUGETTHEMEPROPERTYORIGIN)(HTHEME, int, int, int, /* enum */ PROPERTYORIGIN *);
typedef HRESULT (__stdcall *PFNWXUSETWINDOWTHEME)(HWND, const wchar_t*, const wchar_t *);
typedef HRESULT (__stdcall *PFNWXUGETTHEMEFILENAME)(HTHEME, int, int, int, wchar_t *, int);
typedef COLORREF(__stdcall *PFNWXUGETTHEMESYSCOLOR)(HTHEME, int);
typedef HBRUSH  (__stdcall *PFNWXUGETTHEMESYSCOLORBRUSH)(HTHEME, int);
typedef BOOL    (__stdcall *PFNWXUGETTHEMESYSBOOL)(HTHEME, int);
typedef int     (__stdcall *PFNWXUGETTHEMESYSSIZE)(HTHEME, int);
typedef HRESULT (__stdcall *PFNWXUGETTHEMESYSFONT)(HTHEME, int, LOGFONT *);
typedef HRESULT (__stdcall *PFNWXUGETTHEMESYSSTRING)(HTHEME, int, wchar_t *, int);
typedef HRESULT (__stdcall *PFNWXUGETTHEMESYSINT)(HTHEME, int, int *);
typedef BOOL    (__stdcall *PFNWXUISTHEMEACTIVE)();
typedef BOOL    (__stdcall *PFNWXUISAPPTHEMED)();
typedef HTHEME  (__stdcall *PFNWXUGETWINDOWTHEME)(HWND);
typedef HRESULT (__stdcall *PFNWXUENABLETHEMEDIALOGTEXTURE)(HWND, DWORD);
typedef BOOL    (__stdcall *PFNWXUISTHEMEDIALOGTEXTUREENABLED)(HWND);
typedef DWORD   (__stdcall *PFNWXUGETTHEMEAPPPROPERTIES)();
typedef void    (__stdcall *PFNWXUSETTHEMEAPPPROPERTIES)(DWORD);
typedef HRESULT (__stdcall *PFNWXUGETCURRENTTHEMENAME)(wchar_t *, int, wchar_t *, int, wchar_t *, int);
typedef HRESULT (__stdcall *PFNWXUGETTHEMEDOCUMENTATIONPROPERTY)(const wchar_t *, const wchar_t *, wchar_t *, int);
typedef HRESULT (__stdcall *PFNWXUDRAWTHEMEPARENTBACKGROUND)(HWND, HDC, RECT *);
typedef HRESULT (__stdcall *PFNWXUENABLETHEMING)(BOOL);

// ----------------------------------------------------------------------------
// wxUxThemeEngine: provides all theme functions from uxtheme.dll
// ----------------------------------------------------------------------------

// we always define this class, even if wxUSE_UXTHEME == 0, but we just make it
// empty in this case -- this allows to use it elsewhere without any #ifdefs
#if wxUSE_UXTHEME
    #include "wx/dynlib.h"

    #define wxUX_THEME_DECLARE(type, func) type func;
#else
    #define wxUX_THEME_DECLARE(type, func) type func(...) { return 0; }
#endif

class WXDLLEXPORT wxUxThemeEngine
{
public:
    // get the theme engine or NULL if themes are not available
    static wxUxThemeEngine *Get();

    // get the theme enging or NULL if themes are not available or not used for
    // this application
    static wxUxThemeEngine *GetIfActive();

    // all uxtheme.dll functions
    wxUX_THEME_DECLARE(PFNWXUOPENTHEMEDATA, OpenThemeData)
    wxUX_THEME_DECLARE(PFNWXUCLOSETHEMEDATA, CloseThemeData)
    wxUX_THEME_DECLARE(PFNWXUDRAWTHEMEBACKGROUND, DrawThemeBackground)
    wxUX_THEME_DECLARE(PFNWXUDRAWTHEMETEXT, DrawThemeText)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMEBACKGROUNDCONTENTRECT, GetThemeBackgroundContentRect)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMEBACKGROUNDEXTENT, GetThemeBackgroundExtent)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMEPARTSIZE, GetThemePartSize)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMETEXTEXTENT, GetThemeTextExtent)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMETEXTMETRICS, GetThemeTextMetrics)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMEBACKGROUNDREGION, GetThemeBackgroundRegion)
    wxUX_THEME_DECLARE(PFNWXUHITTESTTHEMEBACKGROUND, HitTestThemeBackground)
    wxUX_THEME_DECLARE(PFNWXUDRAWTHEMEEDGE, DrawThemeEdge)
    wxUX_THEME_DECLARE(PFNWXUDRAWTHEMEICON, DrawThemeIcon)
    wxUX_THEME_DECLARE(PFNWXUISTHEMEPARTDEFINED, IsThemePartDefined)
    wxUX_THEME_DECLARE(PFNWXUISTHEMEBACKGROUNDPARTIALLYTRANSPARENT, IsThemeBackgroundPartiallyTransparent)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMECOLOR, GetThemeColor)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMEMETRIC, GetThemeMetric)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMESTRING, GetThemeString)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMEBOOL, GetThemeBool)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMEINT, GetThemeInt)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMEENUMVALUE, GetThemeEnumValue)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMEPOSITION, GetThemePosition)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMEFONT, GetThemeFont)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMERECT, GetThemeRect)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMEMARGINS, GetThemeMargins)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMEINTLIST, GetThemeIntList)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMEPROPERTYORIGIN, GetThemePropertyOrigin)
    wxUX_THEME_DECLARE(PFNWXUSETWINDOWTHEME, SetWindowTheme)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMEFILENAME, GetThemeFilename)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMESYSCOLOR, GetThemeSysColor)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMESYSCOLORBRUSH, GetThemeSysColorBrush)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMESYSBOOL, GetThemeSysBool)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMESYSSIZE, GetThemeSysSize)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMESYSFONT, GetThemeSysFont)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMESYSSTRING, GetThemeSysString)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMESYSINT, GetThemeSysInt)
    wxUX_THEME_DECLARE(PFNWXUISTHEMEACTIVE, IsThemeActive)
    wxUX_THEME_DECLARE(PFNWXUISAPPTHEMED, IsAppThemed)
    wxUX_THEME_DECLARE(PFNWXUGETWINDOWTHEME, GetWindowTheme)
    wxUX_THEME_DECLARE(PFNWXUENABLETHEMEDIALOGTEXTURE, EnableThemeDialogTexture)
    wxUX_THEME_DECLARE(PFNWXUISTHEMEDIALOGTEXTUREENABLED, IsThemeDialogTextureEnabled)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMEAPPPROPERTIES, GetThemeAppProperties)
    wxUX_THEME_DECLARE(PFNWXUSETTHEMEAPPPROPERTIES, SetThemeAppProperties)
    wxUX_THEME_DECLARE(PFNWXUGETCURRENTTHEMENAME, GetCurrentThemeName)
    wxUX_THEME_DECLARE(PFNWXUGETTHEMEDOCUMENTATIONPROPERTY, GetThemeDocumentationProperty)
    wxUX_THEME_DECLARE(PFNWXUDRAWTHEMEPARENTBACKGROUND, DrawThemeParentBackground)
    wxUX_THEME_DECLARE(PFNWXUENABLETHEMING, EnableTheming)

private:
    // construcor is private as only Get() can create us and is also trivial as
    // everything really happens in Initialize()
    wxUxThemeEngine() { }

    // destructor is private as only Get() and wxUxThemeModule delete us, it is
    // not virtual as we're not supposed to be derived from
    ~wxUxThemeEngine() { }

#if wxUSE_UXTHEME
    // initialize the theme engine: load the DLL, resolve the functions
    //
    // return true if we can be used, false if themes are not available
    bool Initialize();


    // uxtheme.dll
    wxDynamicLibrary m_dllUxTheme;


    // the one and only theme engine, initially NULL
    static wxUxThemeEngine *ms_themeEngine;

    // this is a bool which initially has the value -1 meaning "unknown"
    static int ms_isThemeEngineAvailable;

    // it must be able to delete us
    friend class wxUxThemeModule;
#endif // wxUSE_UXTHEME

    DECLARE_NO_COPY_CLASS(wxUxThemeEngine)
};

#if wxUSE_UXTHEME

/* static */ inline wxUxThemeEngine *wxUxThemeEngine::GetIfActive()
{
    wxUxThemeEngine *engine = Get();
    return engine && engine->IsAppThemed() && engine->IsThemeActive()
                ? engine
                : NULL;
}

#else // !wxUSE_UXTHEME

/* static */ inline wxUxThemeEngine *wxUxThemeEngine::Get()
{
    return NULL;
}

/* static */ inline wxUxThemeEngine *wxUxThemeEngine::GetIfActive()
{
    return NULL;
}

#endif // wxUSE_UXTHEME/!wxUSE_UXTHEME

// ----------------------------------------------------------------------------
// wxUxThemeHandle: encapsulates ::Open/CloseThemeData()
// ----------------------------------------------------------------------------

class wxUxThemeHandle
{
public:
    wxUxThemeHandle(const wxWindow *win, const wchar_t *classes)
    {
        wxUxThemeEngine *engine = wxUxThemeEngine::Get();

        m_hTheme = engine ? (HTHEME)engine->OpenThemeData(GetHwndOf(win), classes)
                          : NULL;
    }

    operator HTHEME() const { return m_hTheme; }

    ~wxUxThemeHandle()
    {
        if ( m_hTheme )
        {
            wxUxThemeEngine::Get()->CloseThemeData(m_hTheme);
        }
    }

private:
    HTHEME m_hTheme;

    DECLARE_NO_COPY_CLASS(wxUxThemeHandle)
};

#endif // _WX_UXTHEME_H_

