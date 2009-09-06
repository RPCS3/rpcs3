/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/imagpcx.cpp
// Purpose:     wxImage PCX handler
// Author:      Guillermo Rodriguez Garcia <guille@iies.es>
// Version:     1.1
// CVS-ID:      $Id: imagpcx.cpp 54766 2008-07-22 20:16:03Z VZ $
// Copyright:   (c) 1999 Guillermo Rodriguez Garcia
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_IMAGE && wxUSE_PCX

#ifndef WX_PRECOMP
    #include "wx/object.h"
    #include "wx/list.h"
    #include "wx/log.h"
    #include "wx/intl.h"
    #include "wx/palette.h"
    #include "wx/hash.h"
    #include "wx/module.h"
#endif

#include "wx/imagpcx.h"
#include "wx/wfstream.h"

//-----------------------------------------------------------------------------
// wxPCXHandler
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxPCXHandler,wxImageHandler)

#if wxUSE_STREAMS

//-----------------------------------------------------------------------------
// RLE encoding and decoding
//-----------------------------------------------------------------------------

void RLEencode(unsigned char *p, unsigned int size, wxOutputStream& s)
{
    unsigned int data, last, cont;

    // Write 'size' bytes. The PCX official specs say there will be
    // a decoding break at the end of each scanline, so in order to
    // force this decoding break use this function to write, at most,
    // _one_ complete scanline at a time.

    last = (unsigned char) *(p++);
    cont = 1;
    size--;

    while (size-- > 0)
    {
        data = (unsigned char) *(p++);

        // Up to 63 bytes with the same value can be stored using
        // a single { cont, value } pair.
        //
        if ((data == last) && (cont < 63))
        {
            cont++;
        }
        else
        {
            // need to write a 'counter' byte?
            if ((cont > 1) || ((last & 0xC0) == 0xC0))
                s.PutC((char) (cont | 0xC0));

            s.PutC((char) last);
            last = data;
            cont = 1;
        }
    }

    // write the last one and return;
    if ((cont > 1) || ((last & 0xC0) == 0xC0))
        s.PutC((char) (cont | 0xC0));

    s.PutC((char) last);
}

void RLEdecode(unsigned char *p, unsigned int size, wxInputStream& s)
{
    // Read 'size' bytes. The PCX official specs say there will be
    // a decoding break at the end of each scanline (but not at the
    // end of each plane inside a scanline). Only use this function
    // to read one or more _complete_ scanlines. Else, more than
    // 'size' bytes might be read and the buffer might overflow.

    while (size != 0)
    {
        unsigned int data = (unsigned char)s.GetC();

        // If ((data & 0xC0) != 0xC0), then the value read is a data
        // byte. Else, it is a counter (cont = val & 0x3F) and the
        // next byte is the data byte.

        if ((data & 0xC0) != 0xC0)
        {
            *(p++) = (unsigned char)data;
            size--;
        }
        else
        {
            unsigned int cont = data & 0x3F;
            if (cont > size) // can happen only if the file is malformed
                break;
            data = (unsigned char)s.GetC();
            for (unsigned int i = 1; i <= cont; i++)
                *(p++) = (unsigned char)data;
            size -= cont;
        }
    }
}


//-----------------------------------------------------------------------------
// PCX reading and saving
//-----------------------------------------------------------------------------

// PCX header
#define HDR_MANUFACTURER    0
#define HDR_VERSION         1
#define HDR_ENCODING        2
#define HDR_BITSPERPIXEL    3
#define HDR_XMIN            4
#define HDR_YMIN            6
#define HDR_XMAX            8
#define HDR_YMAX            10
#define HDR_NPLANES         65
#define HDR_BYTESPERLINE    66
#define HDR_PALETTEINFO     68

// image formats
enum {
    wxPCX_8BIT,             // 8 bpp, 1 plane (8 bit)
    wxPCX_24BIT             // 8 bpp, 3 planes (24 bit)
};

// error codes
enum {
    wxPCX_OK = 0,           // everything was OK
    wxPCX_INVFORMAT = 1,    // error in pcx file format
    wxPCX_MEMERR = 2,       // error allocating memory
    wxPCX_VERERR = 3        // error in pcx version number
};


// ReadPCX:
//  Loads a PCX file into the wxImage object pointed by image.
//  Returns wxPCX_OK on success, or an error code otherwise
//  (see above for error codes)
//
int ReadPCX(wxImage *image, wxInputStream& stream)
{
    unsigned char hdr[128];         // PCX header
    unsigned char pal[768];         // palette for 8 bit images
    unsigned char *p;               // space to store one scanline
    unsigned char *dst;             // pointer into wxImage data
    unsigned int width, height;     // size of the image
    unsigned int bytesperline;      // bytes per line (each plane)
    int bitsperpixel;               // bits per pixel (each plane)
    int nplanes;                    // number of planes
    int encoding;                   // is the image RLE encoded?
    int format;                     // image format (8 bit, 24 bit)
    unsigned int i, j;

    // Read PCX header and check the version number (it must
    // be at least 5 or higher for 8 bit and 24 bit images).

    stream.Read(hdr, 128);

    if (hdr[HDR_VERSION] < 5) return wxPCX_VERERR;

    // Extract all image info from the PCX header.

    encoding     = hdr[HDR_ENCODING];
    nplanes      = hdr[HDR_NPLANES];
    bitsperpixel = hdr[HDR_BITSPERPIXEL];
    bytesperline = hdr[HDR_BYTESPERLINE] + 256 * hdr[HDR_BYTESPERLINE + 1];
    width        = (hdr[HDR_XMAX] + 256 * hdr[HDR_XMAX + 1]) -
                   (hdr[HDR_XMIN] + 256 * hdr[HDR_XMIN + 1]) + 1;
    height       = (hdr[HDR_YMAX] + 256 * hdr[HDR_YMAX + 1]) -
                   (hdr[HDR_YMIN] + 256 * hdr[HDR_YMIN + 1]) + 1;

    // Check image format. Currently supported formats are
    // 8 bits (8 bpp, 1 plane) and 24 bits (8 bpp, 3 planes).

    if ((nplanes == 3) && (bitsperpixel == 8))
        format = wxPCX_24BIT;
    else if ((nplanes == 1) && (bitsperpixel == 8))
        format = wxPCX_8BIT;
    else
        return wxPCX_INVFORMAT;

    // If the image is of type wxPCX_8BIT, then there is
    // a palette at the end of the image data. If we were
    // working with a file, we could seek at the end to the
    // end (SeekI(-769, wxFromEnd) and read the palette
    // before proceeding. Unfortunately, this would prevent
    // loading several PCXs in a single stream, so we can't
    // do it. Thus, 8-bit images will have to be decoded in
    // two passes: one to read and decode the image data,
    // and another to replace 'colour indexes' with RGB
    // values.

    // Resize the image and allocate memory for a scanline.

    image->Create(width, height);

    if (!image->Ok())
        return wxPCX_MEMERR;

    if ((p = (unsigned char *) malloc(bytesperline * nplanes)) == NULL)
        return wxPCX_MEMERR;

    // Now start reading the file, line by line, and store
    // the data in the format required by wxImage.

    dst = image->GetData();

    for (j = height; j; j--)
    {
        if (encoding)
            RLEdecode(p, bytesperline * nplanes, stream);
        else
            stream.Read(p, bytesperline * nplanes);

        switch (format)
        {
            case wxPCX_8BIT:
            {
                for (i = 0; i < width; i++)
                {
                    // first pass, just store the colour index
                    *dst = p[i];
                    dst += 3;
                }
                break;
            }
            case wxPCX_24BIT:
            {
                for (i = 0; i < width; i++)
                {
                    *(dst++) = p[i];
                    *(dst++) = p[i + bytesperline];
                    *(dst++) = p[i + 2 * bytesperline];
                }
                break;
            }
        }
    }

    free(p);

    // For 8 bit images, we read the palette, and then do a second
    // pass replacing indexes with their RGB values;

    if (format == wxPCX_8BIT)
    {
        unsigned char index;

        if (stream.GetC() != 12)
            return wxPCX_INVFORMAT;

        stream.Read(pal, 768);

        p = image->GetData();
        for (unsigned long k = height * width; k; k--)
        {
            index = *p;
            *(p++) = pal[3 * index];
            *(p++) = pal[3 * index + 1];
            *(p++) = pal[3 * index + 2];
        }

#if wxUSE_PALETTE
        unsigned char r[256];
        unsigned char g[256];
        unsigned char b[256];
        for (i = 0; i < 256; i++)
        {
            r[i] = pal[3*i + 0];
            g[i] = pal[3*i + 1];
            b[i] = pal[3*i + 2];
        }
        image->SetPalette(wxPalette(256, r, g, b));
#endif // wxUSE_PALETTE
    }

    return wxPCX_OK;
}

// SavePCX:
//  Saves a PCX file into the wxImage object pointed by image.
//  Returns wxPCX_OK on success, or an error code otherwise
//  (see above for error codes). Will try to save as 8-bit
//  PCX if possible, and then fall back to 24-bit if there
//  are more than 256 different colours.
//
int SavePCX(wxImage *image, wxOutputStream& stream)
{
    unsigned char hdr[128];         // PCX header
    unsigned char pal[768];         // palette for 8 bit images
    unsigned char *p;               // space to store one scanline
    unsigned char *src;             // pointer into wxImage data
    unsigned int width, height;     // size of the image
    unsigned int bytesperline;      // bytes per line (each plane)
    unsigned char nplanes = 3;      // number of planes
    int format = wxPCX_24BIT;       // image format (8 bit, 24 bit)
    wxImageHistogram histogram;     // image histogram
    unsigned long key;              // key in the hashtable
    unsigned int i;

    // See if we can save as 8 bit.

    if (image->CountColours(256) <= 256)
    {
        image->ComputeHistogram(histogram);
        format = wxPCX_8BIT;
        nplanes = 1;
    }

    // Get image dimensions, calculate bytesperline (must be even,
    // according to PCX specs) and allocate space for one complete
    // scanline.

    if (!image->Ok())
        return wxPCX_INVFORMAT;

    width = image->GetWidth();
    height = image->GetHeight();
    bytesperline = width;
    if (bytesperline % 2)
        bytesperline++;

    if ((p = (unsigned char *) malloc(bytesperline * nplanes)) == NULL)
        return wxPCX_MEMERR;

    // Build header data and write it to the stream. Initially,
    // set all bytes to zero (most values default to zero).

    memset(hdr, 0, sizeof(hdr));

    hdr[HDR_MANUFACTURER]     = 10;
    hdr[HDR_VERSION]          = 5;
    hdr[HDR_ENCODING]         = 1;
    hdr[HDR_NPLANES]          = nplanes;
    hdr[HDR_BITSPERPIXEL]     = 8;
    hdr[HDR_BYTESPERLINE]     = (unsigned char)(bytesperline % 256);
    hdr[HDR_BYTESPERLINE + 1] = (unsigned char)(bytesperline / 256);
    hdr[HDR_XMAX]             = (unsigned char)((width - 1)  % 256);
    hdr[HDR_XMAX + 1]         = (unsigned char)((width - 1)  / 256);
    hdr[HDR_YMAX]             = (unsigned char)((height - 1) % 256);
    hdr[HDR_YMAX + 1]         = (unsigned char)((height - 1) / 256);
    hdr[HDR_PALETTEINFO]      = 1;

    stream.Write(hdr, 128);

    // Encode image data line by line and write it to the stream

    src = image->GetData();

    for (; height; height--)
    {
        switch (format)
        {
            case wxPCX_8BIT:
            {
                unsigned char r, g, b;

                for (i = 0; i < width; i++)
                {
                    r = *(src++);
                    g = *(src++);
                    b = *(src++);
                    key = (r << 16) | (g << 8) | b;

                    p[i] = (unsigned char)histogram[key].index;
                }
                break;
            }
            case wxPCX_24BIT:
            {
                for (i = 0; i < width; i++)
                {
                    p[i] = *(src++);
                    p[i + bytesperline] = *(src++);
                    p[i + 2 * bytesperline] = *(src++);
                }
                break;
            }
        }

        RLEencode(p, bytesperline * nplanes, stream);
    }

    free(p);

    // For 8 bit images, build the palette and write it to the stream:
    if (format == wxPCX_8BIT)
    {
        // zero unused colours
        memset(pal, 0, sizeof(pal));

        unsigned long index;

        for (wxImageHistogram::iterator entry = histogram.begin();
             entry != histogram.end(); ++entry )
        {
            key = entry->first;
            index = entry->second.index;
            pal[3 * index]     = (unsigned char)(key >> 16);
            pal[3 * index + 1] = (unsigned char)(key >> 8);
            pal[3 * index + 2] = (unsigned char)(key);
        }

        stream.PutC(12);
        stream.Write(pal, 768);
    }

    return wxPCX_OK;
}

//-----------------------------------------------------------------------------
// wxPCXHandler
//-----------------------------------------------------------------------------

bool wxPCXHandler::LoadFile( wxImage *image, wxInputStream& stream, bool verbose, int WXUNUSED(index) )
{
    int error;

    if (!CanRead(stream))
    {
        if (verbose)
            wxLogError(_("PCX: this is not a PCX file."));

        return false;
    }

    image->Destroy();

    if ((error = ReadPCX(image, stream)) != wxPCX_OK)
    {
        if (verbose)
        {
            switch (error)
            {
                case wxPCX_INVFORMAT: wxLogError(_("PCX: image format unsupported")); break;
                case wxPCX_MEMERR:    wxLogError(_("PCX: couldn't allocate memory")); break;
                case wxPCX_VERERR:    wxLogError(_("PCX: version number too low")); break;
                default:              wxLogError(_("PCX: unknown error !!!"));
            }
        }
        image->Destroy();
        return false;
    }

    return true;
}

bool wxPCXHandler::SaveFile( wxImage *image, wxOutputStream& stream, bool verbose )
{
    int error;

    if ((error = SavePCX(image, stream)) != wxPCX_OK)
    {
        if (verbose)
        {
            switch (error)
            {
                case wxPCX_INVFORMAT: wxLogError(_("PCX: invalid image")); break;
                case wxPCX_MEMERR:    wxLogError(_("PCX: couldn't allocate memory")); break;
                default:              wxLogError(_("PCX: unknown error !!!"));
            }
        }
    }

    return (error == wxPCX_OK);
}

bool wxPCXHandler::DoCanRead( wxInputStream& stream )
{
    unsigned char c = stream.GetC();
    if ( !stream )
        return false;

    // not very safe, but this is all we can get from PCX header :-(
    return c == 10;
}

#endif // wxUSE_STREAMS

#endif // wxUSE_IMAGE && wxUSE_PCX
