/////////////////////////////////////////////////////////////////////////////
// Name:        imagpng.h
// Purpose:     wxImage PNG handler
// Author:      Robert Roebling
// RCS-ID:      $Id: imagpng.h 37393 2006-02-08 21:47:09Z VZ $
// Copyright:   (c) Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_IMAGPNG_H_
#define _WX_IMAGPNG_H_

#include "wx/defs.h"

//-----------------------------------------------------------------------------
// wxPNGHandler
//-----------------------------------------------------------------------------

#if wxUSE_LIBPNG

#include "wx/image.h"

#define wxIMAGE_OPTION_PNG_FORMAT    wxT("PngFormat")
#define wxIMAGE_OPTION_PNG_BITDEPTH  wxT("PngBitDepth")

enum
{
    wxPNG_TYPE_COLOUR = 0,
    wxPNG_TYPE_GREY = 2,
    wxPNG_TYPE_GREY_RED = 3
};

class WXDLLEXPORT wxPNGHandler: public wxImageHandler
{
public:
    inline wxPNGHandler()
    {
        m_name = wxT("PNG file");
        m_extension = wxT("png");
        m_type = wxBITMAP_TYPE_PNG;
        m_mime = wxT("image/png");
    }

#if wxUSE_STREAMS
    virtual bool LoadFile( wxImage *image, wxInputStream& stream, bool verbose=true, int index=-1 );
    virtual bool SaveFile( wxImage *image, wxOutputStream& stream, bool verbose=true );
protected:
    virtual bool DoCanRead( wxInputStream& stream );
#endif

private:
    DECLARE_DYNAMIC_CLASS(wxPNGHandler)
};

#endif
  // wxUSE_LIBPNG

#endif
  // _WX_IMAGPNG_H_

