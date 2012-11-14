/////////////////////////////////////////////////////////////////////////////
// Name:        wx/imagbmp.h
// Purpose:     wxImage BMP, ICO, CUR and ANI handlers
// Author:      Robert Roebling, Chris Elliott
// RCS-ID:      $Id: imagbmp.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) Robert Roebling, Chris Elliott
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_IMAGBMP_H_
#define _WX_IMAGBMP_H_

#include "wx/image.h"

// defines for saving the BMP file in different formats, Bits Per Pixel
// USE: wximage.SetOption( wxIMAGE_OPTION_BMP_FORMAT, wxBMP_xBPP );
#define wxIMAGE_OPTION_BMP_FORMAT wxString(wxT("wxBMP_FORMAT"))

// These two options are filled in upon reading CUR file and can (should) be
// specified when saving a CUR file - they define the hotspot of the cursor:
#define wxIMAGE_OPTION_CUR_HOTSPOT_X  wxT("HotSpotX")
#define wxIMAGE_OPTION_CUR_HOTSPOT_Y  wxT("HotSpotY")

#if WXWIN_COMPATIBILITY_2_4
    // Do not use these macros, they are deprecated
    #define wxBMP_FORMAT    wxIMAGE_OPTION_BMP_FORMAT
    #define wxCUR_HOTSPOT_X wxIMAGE_OPTION_CUR_HOTSPOT_X
    #define wxCUR_HOTSPOT_Y wxIMAGE_OPTION_CUR_HOTSPOT_Y
#endif


enum
{
    wxBMP_24BPP        = 24, // default, do not need to set
    //wxBMP_16BPP      = 16, // wxQuantize can only do 236 colors?
    wxBMP_8BPP         =  8, // 8bpp, quantized colors
    wxBMP_8BPP_GREY    =  9, // 8bpp, rgb averaged to greys
    wxBMP_8BPP_GRAY    =  wxBMP_8BPP_GREY,
    wxBMP_8BPP_RED     = 10, // 8bpp, red used as greyscale
    wxBMP_8BPP_PALETTE = 11, // 8bpp, use the wxImage's palette
    wxBMP_4BPP         =  4, // 4bpp, quantized colors
    wxBMP_1BPP         =  1, // 1bpp, quantized "colors"
    wxBMP_1BPP_BW      =  2  // 1bpp, black & white from red
};

// ----------------------------------------------------------------------------
// wxBMPHandler
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxBMPHandler : public wxImageHandler
{
public:
    wxBMPHandler()
    {
        m_name = wxT("Windows bitmap file");
        m_extension = wxT("bmp");
        m_type = wxBITMAP_TYPE_BMP;
        m_mime = wxT("image/x-bmp");
    }

#if wxUSE_STREAMS
    virtual bool SaveFile( wxImage *image, wxOutputStream& stream, bool verbose=true );
    virtual bool LoadFile( wxImage *image, wxInputStream& stream, bool verbose=true, int index=-1 );

protected:
    virtual bool DoCanRead( wxInputStream& stream );
    bool SaveDib(wxImage *image, wxOutputStream& stream, bool verbose,
                 bool IsBmp, bool IsMask);
    bool DoLoadDib(wxImage *image, int width, int height, int bpp, int ncolors,
                   int comp, wxFileOffset bmpOffset, wxInputStream& stream,
                   bool verbose, bool IsBmp, bool hasPalette);
    bool LoadDib(wxImage *image, wxInputStream& stream, bool verbose, bool IsBmp);
#endif // wxUSE_STREAMS

private:
     DECLARE_DYNAMIC_CLASS(wxBMPHandler)
};

#if wxUSE_ICO_CUR
// ----------------------------------------------------------------------------
// wxICOHandler
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxICOHandler : public wxBMPHandler
{
public:
    wxICOHandler()
    {
        m_name = wxT("Windows icon file");
        m_extension = wxT("ico");
        m_type = wxBITMAP_TYPE_ICO;
        m_mime = wxT("image/x-ico");
    }

#if wxUSE_STREAMS
    virtual bool SaveFile( wxImage *image, wxOutputStream& stream, bool verbose=true );
    virtual bool LoadFile( wxImage *image, wxInputStream& stream, bool verbose=true, int index=-1 );
    virtual bool DoLoadFile( wxImage *image, wxInputStream& stream, bool verbose, int index );
    virtual int GetImageCount( wxInputStream& stream );
protected:
    virtual bool DoCanRead( wxInputStream& stream );
#endif // wxUSE_STREAMS

private:
    DECLARE_DYNAMIC_CLASS(wxICOHandler)
};


// ----------------------------------------------------------------------------
// wxCURHandler
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxCURHandler : public wxICOHandler
{
public:
    wxCURHandler()
    {
        m_name = wxT("Windows cursor file");
        m_extension = wxT("cur");
        m_type = wxBITMAP_TYPE_CUR;
        m_mime = wxT("image/x-cur");
    }

    // VS: This handler's meat is implemented inside wxICOHandler (the two
    //     formats are almost identical), but we hide this fact at
    //     the API level, since it is a mere implementation detail.

protected:
#if wxUSE_STREAMS
    virtual bool DoCanRead( wxInputStream& stream );
#endif // wxUSE_STREAMS

private:
    DECLARE_DYNAMIC_CLASS(wxCURHandler)
};
// ----------------------------------------------------------------------------
// wxANIHandler
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxANIHandler : public wxCURHandler
{
public:
    wxANIHandler()
    {
        m_name = wxT("Windows animated cursor file");
        m_extension = wxT("ani");
        m_type = wxBITMAP_TYPE_ANI;
        m_mime = wxT("image/x-ani");
    }


#if wxUSE_STREAMS
    virtual bool SaveFile( wxImage *WXUNUSED(image), wxOutputStream& WXUNUSED(stream), bool WXUNUSED(verbose=true) ){return false ;}
    virtual bool LoadFile( wxImage *image, wxInputStream& stream, bool verbose=true, int index=-1 );
    virtual int GetImageCount( wxInputStream& stream );
protected:
    virtual bool DoCanRead( wxInputStream& stream );
#endif // wxUSE_STREAMS

private:
    DECLARE_DYNAMIC_CLASS(wxANIHandler)
};

#endif // wxUSE_ICO_CUR
#endif // _WX_IMAGBMP_H_
