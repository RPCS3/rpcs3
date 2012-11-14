///////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/mask.cpp
// Purpose:     generic wxMask implementation
// Author:      Vadim Zeitlin
// Created:     2006-09-28
// RCS-ID:      $Id: mask.cpp 41495 2006-09-28 23:02:39Z VZ $
// Copyright:   (c) 2006 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/bitmap.h"
    #include "wx/image.h"
#endif // WX_PRECOMP

#if wxUSE_GENERIC_MASK

// ============================================================================
// wxMask implementation
// ============================================================================

IMPLEMENT_DYNAMIC_CLASS(wxMask, wxObject)

void wxMask::FreeData()
{
    m_bitmap = wxNullBitmap;
}

bool wxMask::InitFromColour(const wxBitmap& bitmap, const wxColour& colour)
{
#if wxUSE_IMAGE
    const wxColour clr(bitmap.QuantizeColour(colour));

    wxImage imgSrc(bitmap.ConvertToImage());
    imgSrc.SetMask(false);
    wxImage image(imgSrc.ConvertToMono(clr.Red(), clr.Green(), clr.Blue()));
    if ( !image.Ok() )
        return false;

    m_bitmap = wxBitmap(image, 1);

    return m_bitmap.Ok();
#else // !wxUSE_IMAGE
    wxUnusedVar(bitmap);
    wxUnusedVar(colour);

    return false;
#endif // wxUSE_IMAGE/!wxUSE_IMAGE
}

bool wxMask::InitFromMonoBitmap(const wxBitmap& bitmap)
{
    wxCHECK_MSG( bitmap.Ok(), false, wxT("Invalid bitmap") );
    wxCHECK_MSG( bitmap.GetDepth() == 1, false, wxT("Cannot create mask from colour bitmap") );

    m_bitmap = bitmap;

    return true;
}

#endif // wxUSE_GENERIC_MASK
