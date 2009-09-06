/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/imagall.cpp
// Purpose:     wxImage access all handler
// Author:      Sylvain Bougnoux
// RCS-ID:      $Id: imagall.cpp 42644 2006-10-29 18:58:25Z VZ $
// Copyright:   (c) Sylvain Bougnoux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_IMAGE

#ifndef WX_PRECOMP
    #include "wx/image.h"
#endif

//-----------------------------------------------------------------------------
// This function allows dynamic access to all image handlers compile within
// the library. This function should be in a separate file as some compilers
// link against the whole object file as long as just one of is function is called!

void wxInitAllImageHandlers()
{
#if wxUSE_LIBPNG
  wxImage::AddHandler( new wxPNGHandler );
#endif
#if wxUSE_LIBJPEG
  wxImage::AddHandler( new wxJPEGHandler );
#endif
#if wxUSE_LIBTIFF
  wxImage::AddHandler( new wxTIFFHandler );
#endif
#if wxUSE_GIF
  wxImage::AddHandler( new wxGIFHandler );
#endif
#if wxUSE_PNM
  wxImage::AddHandler( new wxPNMHandler );
#endif
#if wxUSE_PCX
  wxImage::AddHandler( new wxPCXHandler );
#endif
#if wxUSE_IFF
  wxImage::AddHandler( new wxIFFHandler );
#endif
#if wxUSE_ICO_CUR
  wxImage::AddHandler( new wxICOHandler );
  wxImage::AddHandler( new wxCURHandler );
  wxImage::AddHandler( new wxANIHandler );
#endif
#if wxUSE_TGA
  wxImage::AddHandler( new wxTGAHandler );
#endif
#if wxUSE_XPM
  wxImage::AddHandler( new wxXPMHandler );
#endif
}

#endif // wxUSE_IMAGE
