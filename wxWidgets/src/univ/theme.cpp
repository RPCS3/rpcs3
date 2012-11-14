///////////////////////////////////////////////////////////////////////////////
// Name:        univ/theme.cpp
// Purpose:     implementation of wxTheme
// Author:      Vadim Zeitlin
// Modified by:
// Created:     06.08.00
// RCS-ID:      $Id: theme.cpp 42455 2006-10-26 15:33:10Z VS $
// Copyright:   (c) 2000 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// declarations
// ===========================================================================

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/log.h"
#endif // WX_PRECOMP

#include "wx/artprov.h"

#include "wx/univ/renderer.h"
#include "wx/univ/inphand.h"
#include "wx/univ/theme.h"

// ============================================================================
// implementation
// ============================================================================

wxThemeInfo *wxTheme::ms_allThemes = (wxThemeInfo *)NULL;
wxTheme *wxTheme::ms_theme = (wxTheme *)NULL;

// ----------------------------------------------------------------------------
// "dynamic" theme creation
// ----------------------------------------------------------------------------

wxThemeInfo::wxThemeInfo(Constructor c,
                         const wxChar *n,
                         const wxChar *d)
           : name(n), desc(d), ctor(c)
{
    // insert us (in the head of) the linked list
    next = wxTheme::ms_allThemes;
    wxTheme::ms_allThemes = this;
}

/* static */ wxTheme *wxTheme::Create(const wxString& name)
{
    // find the theme in the list by name
    wxThemeInfo *info = ms_allThemes;
    while ( info )
    {
        if ( name.CmpNoCase(info->name) == 0 )
        {
            return info->ctor();
        }

        info = info->next;
    }

    return (wxTheme *)NULL;
}

// ----------------------------------------------------------------------------
// the default theme (called by wxApp::OnInitGui)
// ----------------------------------------------------------------------------

/* static */ bool wxTheme::CreateDefault()
{
    if ( ms_theme )
    {
        // we already have a theme
        return true;
    }

    wxString nameDefTheme;

    // use the environment variable first
    const wxChar *p = wxGetenv(_T("WXTHEME"));
    if ( p )
    {
        nameDefTheme = p;
    }
#ifdef wxUNIV_DEFAULT_THEME
    else // use native theme by default
    {
        nameDefTheme = wxSTRINGIZE_T(wxUNIV_DEFAULT_THEME);
    }
#endif // wxUNIV_DEFAULT_THEME

    wxTheme *theme = Create(nameDefTheme);

    // fallback to the first one in the list
    if ( !theme && ms_allThemes )
    {
        theme = ms_allThemes->ctor();
    }

    // abort if still nothing
    if ( !theme )
    {
        wxLogError(_("Failed to initialize GUI: no built-in themes found."));

        return false;
    }

    // Set the theme as current.
    wxTheme::Set(theme);

    return true;
}

/* static */ wxTheme *wxTheme::Set(wxTheme *theme)
{
    wxTheme *themeOld = ms_theme;
    ms_theme = theme;

    if ( ms_theme )
    {
        // automatically start using the art provider of the new theme if it
        // has one
        wxArtProvider *art = ms_theme->GetArtProvider();
        if ( art )
            wxArtProvider::Push(art);
    }

    return themeOld;
}

// ----------------------------------------------------------------------------
// assorted trivial dtors
// ----------------------------------------------------------------------------

wxTheme::~wxTheme()
{
}


// ----------------------------------------------------------------------------
// wxDelegateTheme
// ----------------------------------------------------------------------------

wxDelegateTheme::wxDelegateTheme(const wxChar *theme)
{
    m_themeName = theme;
    m_theme = NULL;
}

wxDelegateTheme::~wxDelegateTheme()
{
    delete m_theme;
}

bool wxDelegateTheme::GetOrCreateTheme()
{
    if ( !m_theme )
        m_theme = wxTheme::Create(m_themeName);
    return m_theme != NULL;
}

wxRenderer *wxDelegateTheme::GetRenderer()
{
    if ( !GetOrCreateTheme() )
        return NULL;

    return m_theme->GetRenderer();
}

wxArtProvider *wxDelegateTheme::GetArtProvider()
{
    if ( !GetOrCreateTheme() )
        return NULL;

    return m_theme->GetArtProvider();
}

wxInputHandler *wxDelegateTheme::GetInputHandler(const wxString& control,
                                                 wxInputConsumer *consumer)
{
    if ( !GetOrCreateTheme() )
        return NULL;

    return m_theme->GetInputHandler(control, consumer);
}

wxColourScheme *wxDelegateTheme::GetColourScheme()
{
    if ( !GetOrCreateTheme() )
        return NULL;

    return m_theme->GetColourScheme();
}
