/////////////////////////////////////////////////////////////////////////////
// Name:        wx/imagtga.h
// Purpose:     wxImage TGA handler
// Author:      Seth Jackson
// RCS-ID:      $Id: imagtga.h 43843 2006-12-07 05:44:44Z PC $
// Copyright:   (c) 2005 Seth Jackson
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_IMAGTGA_H_
#define _WX_IMAGTGA_H_

#include "wx/image.h"

//-----------------------------------------------------------------------------
// wxTGAHandler
//-----------------------------------------------------------------------------

#if wxUSE_TGA

class WXDLLEXPORT wxTGAHandler : public wxImageHandler
{
public:
    wxTGAHandler()
    {
        m_name = wxT("TGA file");
        m_extension = wxT("tga");
        m_type = wxBITMAP_TYPE_TGA;
        m_mime = wxT("image/tga");
    }

#if wxUSE_STREAMS
    virtual bool LoadFile(wxImage* image, wxInputStream& stream,
                            bool verbose = true, int index = -1);
    virtual bool SaveFile(wxImage* image, wxOutputStream& stream,
                             bool verbose = true);
protected:
    virtual bool DoCanRead(wxInputStream& stream);
#endif // wxUSE_STREAMS

    DECLARE_DYNAMIC_CLASS(wxTGAHandler)
};

#endif // wxUSE_TGA

#endif // _WX_IMAGTGA_H_
