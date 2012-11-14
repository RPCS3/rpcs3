/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/imaggif.cpp
// Purpose:     wxGIFHandler
// Author:      Vaclav Slavik & Guillermo Rodriguez Garcia
// RCS-ID:      $Id: imaggif.cpp 41819 2006-10-09 17:51:07Z VZ $
// Copyright:   (c) 1999 Vaclav Slavik & Guillermo Rodriguez Garcia
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_IMAGE && wxUSE_GIF

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/log.h"
#endif

#include "wx/imaggif.h"
#include "wx/gifdecod.h"
#include "wx/wfstream.h"

IMPLEMENT_DYNAMIC_CLASS(wxGIFHandler,wxImageHandler)

//-----------------------------------------------------------------------------
// wxGIFHandler
//-----------------------------------------------------------------------------

#if wxUSE_STREAMS

bool wxGIFHandler::LoadFile(wxImage *image, wxInputStream& stream,
                            bool verbose, int index)
{
    wxGIFDecoder *decod;
    wxGIFErrorCode error;
    bool ok = true;

//    image->Destroy();
    decod = new wxGIFDecoder();
    error = decod->LoadGIF(stream);

    if ((error != wxGIF_OK) && (error != wxGIF_TRUNCATED))
    {
        if (verbose)
        {
            switch (error)
            {
                case wxGIF_INVFORMAT:
                    wxLogError(_("GIF: error in GIF image format."));
                    break;
                case wxGIF_MEMERR:
                    wxLogError(_("GIF: not enough memory."));
                    break;
                default:
                    wxLogError(_("GIF: unknown error!!!"));
                    break;
            }
        }
        delete decod;
        return false;
    }

    if ((error == wxGIF_TRUNCATED) && verbose)
    {
        wxLogError(_("GIF: data stream seems to be truncated."));
        /* go on; image data is OK */
    }

    if (ok)
    {
        ok = decod->ConvertToImage(index != -1 ? (size_t)index : 0, image);
    }
    else
    {
        wxLogError(_("GIF: Invalid gif index."));
    }

    delete decod;

    return ok;
}

bool wxGIFHandler::SaveFile( wxImage * WXUNUSED(image),
                             wxOutputStream& WXUNUSED(stream), bool verbose )
{
    if (verbose)
        wxLogDebug(wxT("GIF: the handler is read-only!!"));

    return false;
}

bool wxGIFHandler::DoCanRead( wxInputStream& stream )
{
    wxGIFDecoder decod;
    return decod.CanRead(stream);
}

#endif  // wxUSE_STREAMS

#endif  // wxUSE_GIF
