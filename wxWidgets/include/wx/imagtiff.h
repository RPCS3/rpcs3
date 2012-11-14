/////////////////////////////////////////////////////////////////////////////
// Name:        imagtiff.h
// Purpose:     wxImage TIFF handler
// Author:      Robert Roebling
// RCS-ID:      $Id: imagtiff.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_IMAGTIFF_H_
#define _WX_IMAGTIFF_H_

#include "wx/defs.h"

//-----------------------------------------------------------------------------
// wxTIFFHandler
//-----------------------------------------------------------------------------

#if wxUSE_LIBTIFF

#include "wx/image.h"

// defines for wxImage::SetOption
#define wxIMAGE_OPTION_BITSPERSAMPLE               wxString(wxT("BitsPerSample"))
#define wxIMAGE_OPTION_SAMPLESPERPIXEL             wxString(wxT("SamplesPerPixel"))
#define wxIMAGE_OPTION_COMPRESSION                 wxString(wxT("Compression"))
#define wxIMAGE_OPTION_IMAGEDESCRIPTOR             wxString(wxT("ImageDescriptor"))

class WXDLLEXPORT wxTIFFHandler: public wxImageHandler
{
public:
    wxTIFFHandler();

#if wxUSE_STREAMS
    virtual bool LoadFile( wxImage *image, wxInputStream& stream, bool verbose=true, int index=-1 );
    virtual bool SaveFile( wxImage *image, wxOutputStream& stream, bool verbose=true );
    virtual int GetImageCount( wxInputStream& stream );
protected:
    virtual bool DoCanRead( wxInputStream& stream );
#endif

private:
    DECLARE_DYNAMIC_CLASS(wxTIFFHandler)
};

#endif // wxUSE_LIBTIFF

#endif // _WX_IMAGTIFF_H_

