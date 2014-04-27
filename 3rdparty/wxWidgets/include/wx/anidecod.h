/////////////////////////////////////////////////////////////////////////////
// Name:        wx/anidecod.h
// Purpose:     wxANIDecoder, ANI reader for wxImage and wxAnimation
// Author:      Francesco Montorsi
// CVS-ID:      $Id: anidecod.h 45563 2007-04-21 18:17:50Z VZ $
// Copyright:   (c) 2006 Francesco Montorsi
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_ANIDECOD_H
#define _WX_ANIDECOD_H

#include "wx/defs.h"

#if wxUSE_STREAMS && wxUSE_ICO_CUR

#include "wx/stream.h"
#include "wx/image.h"
#include "wx/animdecod.h"
#include "wx/dynarray.h"


class /*WXDLLEXPORT*/ wxANIFrameInfo;

WX_DECLARE_EXPORTED_OBJARRAY(wxANIFrameInfo, wxANIFrameInfoArray);
WX_DECLARE_EXPORTED_OBJARRAY(wxImage, wxImageArray);

// --------------------------------------------------------------------------
// wxANIDecoder class
// --------------------------------------------------------------------------

class WXDLLEXPORT wxANIDecoder : public wxAnimationDecoder
{
public:
    // constructor, destructor, etc.
    wxANIDecoder();
    ~wxANIDecoder();


    virtual wxSize GetFrameSize(unsigned int frame) const;
    virtual wxPoint GetFramePosition(unsigned int frame) const;
    virtual wxAnimationDisposal GetDisposalMethod(unsigned int frame) const;
    virtual long GetDelay(unsigned int frame) const;
    virtual wxColour GetTransparentColour(unsigned int frame) const;

    // implementation of wxAnimationDecoder's pure virtuals
    virtual bool CanRead( wxInputStream& stream ) const;
    virtual bool Load( wxInputStream& stream );

    bool ConvertToImage(unsigned int frame, wxImage *image) const;

    wxAnimationDecoder *Clone() const
        { return new wxANIDecoder; }
    wxAnimationType GetType() const
        { return wxANIMATION_TYPE_ANI; }

private:
    // frames stored as wxImage(s): ANI files are meant to be used mostly for animated
    // cursors and thus they do not use any optimization to encode differences between
    // two frames: they are just a list of images to display sequentially.
    wxImageArray m_images;

    // the info about each image stored in m_images.
    // NB: m_info.GetCount() may differ from m_images.GetCount()!
    wxANIFrameInfoArray m_info;

    // this is the wxCURHandler used to load the ICON chunk of the ANI files
    static wxCURHandler sm_handler;


    DECLARE_NO_COPY_CLASS(wxANIDecoder)
};


#endif  // wxUSE_STREAM && wxUSE_ICO_CUR

#endif  // _WX_ANIDECOD_H
