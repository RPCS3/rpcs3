/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/imagiff.h
// Purpose:     wxImage handler for Amiga IFF images
// Author:      Steffen Gutmann, Thomas Meyer
// RCS-ID:      $Id: imagiff.cpp 38787 2006-04-18 07:24:35Z ABX $
// Copyright:   (c) Steffen Gutmann, 2002
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// Parts of this source are based on the iff loading algorithm found
// in xviff.c.  Permission by the original author, Thomas Meyer, and
// by the author of xv, John Bradley for using the iff loading part
// in wxWidgets has been gratefully given.

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_IMAGE && wxUSE_IFF

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/intl.h"
#endif

#include "wx/imagiff.h"
#include "wx/wfstream.h"

#if wxUSE_PALETTE
    #include "wx/palette.h"
#endif // wxUSE_PALETTE

#include <stdlib.h>
#include <string.h>


// --------------------------------------------------------------------------
// Constants
// --------------------------------------------------------------------------

// Error codes:
//  Note that the error code wxIFF_TRUNCATED means that the image itself
//  is most probably OK, but the decoder didn't reach the end of the data
//  stream; this means that if it was not reading directly from file,
//  the stream will not be correctly positioned.
//

enum
{
    wxIFF_OK = 0,                   /* everything was OK */
    wxIFF_INVFORMAT,                /* error in iff header */
    wxIFF_MEMERR,                   /* error allocating memory */
    wxIFF_TRUNCATED                 /* file appears to be truncated */
};

// --------------------------------------------------------------------------
// wxIFFDecoder class
// --------------------------------------------------------------------------

// internal class for storing IFF image data
class IFFImage
{
public:
    unsigned int w;                 /* width */
    unsigned int h;                 /* height */
    int transparent;                /* transparent color (-1 = none) */
    int colors;                     /* number of colors */
    unsigned char *p;               /* bitmap */
    unsigned char *pal;             /* palette */

    IFFImage() : w(0), h(0), colors(0), p(0), pal(0) {}
    ~IFFImage() { delete [] p; delete [] pal; }
};

class WXDLLEXPORT wxIFFDecoder
{
private:
    IFFImage *m_image;        // image data
    wxInputStream *m_f;       // input stream
    unsigned char *databuf;
    unsigned char *picptr;
    unsigned char *decomp_mem;

    void Destroy();

public:
    // get data of current frame
    unsigned char* GetData() const;
    unsigned char* GetPalette() const;
    int GetNumColors() const;
    unsigned int GetWidth() const;
    unsigned int GetHeight() const;
    int GetTransparentColour() const;

    // constructor, destructor, etc.
    wxIFFDecoder(wxInputStream *s);
    ~wxIFFDecoder() { Destroy(); }
    bool CanRead();
    int ReadIFF();
    bool ConvertToImage(wxImage *image) const;
};


//---------------------------------------------------------------------------
// wxIFFDecoder constructor and destructor
//---------------------------------------------------------------------------

wxIFFDecoder::wxIFFDecoder(wxInputStream *s)
{
    m_f = s;
    m_image = 0;
    databuf = 0;
    decomp_mem = 0;
}

void wxIFFDecoder::Destroy()
{
    delete m_image;
    m_image = 0;
    delete [] databuf;
    databuf = 0;
    delete [] decomp_mem;
    decomp_mem = 0;
}

//---------------------------------------------------------------------------
// Convert this image to a wxImage object
//---------------------------------------------------------------------------

// This function was designed by Vaclav Slavik

bool wxIFFDecoder::ConvertToImage(wxImage *image) const
{
    // just in case...
    image->Destroy();

    // create the image
    image->Create(GetWidth(), GetHeight());

    if (!image->Ok())
        return false;

    unsigned char *pal = GetPalette();
    unsigned char *src = GetData();
    unsigned char *dst = image->GetData();
    int colors = GetNumColors();
    int transparent = GetTransparentColour();
    long i;

    // set transparent colour mask
    if (transparent != -1)
    {
        for (i = 0; i < colors; i++)
        {
            if ((pal[3 * i + 0] == 255) &&
                (pal[3 * i + 1] == 0) &&
                (pal[3 * i + 2] == 255))
            {
                pal[3 * i + 2] = 254;
            }
        }

        pal[3 * transparent + 0] = 255,
        pal[3 * transparent + 1] = 0,
        pal[3 * transparent + 2] = 255;

        image->SetMaskColour(255, 0, 255);
    }
    else
        image->SetMask(false);

#if wxUSE_PALETTE
    if (pal && colors > 0)
    {
        unsigned char* r = new unsigned char[colors];
        unsigned char* g = new unsigned char[colors];
        unsigned char* b = new unsigned char[colors];

        for (i = 0; i < colors; i++)
        {
            r[i] = pal[3*i + 0];
            g[i] = pal[3*i + 1];
            b[i] = pal[3*i + 2];
        }

        image->SetPalette(wxPalette(colors, r, g, b));

        delete [] r;
        delete [] g;
        delete [] b;
    }
#endif // wxUSE_PALETTE

    // copy image data
    for (i = 0; i < (long)(GetWidth() * GetHeight()); i++, src += 3, dst += 3)
    {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    }

    return true;
}


//---------------------------------------------------------------------------
// Data accessors
//---------------------------------------------------------------------------

// Get data for current frame

unsigned char* wxIFFDecoder::GetData() const    { return (m_image->p); }
unsigned char* wxIFFDecoder::GetPalette() const { return (m_image->pal); }
int wxIFFDecoder::GetNumColors() const          { return m_image->colors; }
unsigned int wxIFFDecoder::GetWidth() const     { return (m_image->w); }
unsigned int wxIFFDecoder::GetHeight() const    { return (m_image->h); }
int wxIFFDecoder::GetTransparentColour() const { return m_image->transparent; }

//---------------------------------------------------------------------------
// IFF reading and decoding
//---------------------------------------------------------------------------

//
// CanRead:
//  Returns true if the file looks like a valid IFF, false otherwise.
//
bool wxIFFDecoder::CanRead()
{
    unsigned char buf[12];

    if ( !m_f->Read(buf, WXSIZEOF(buf)) )
        return false;

    m_f->SeekI(-(wxFileOffset)WXSIZEOF(buf), wxFromCurrent);

    return (memcmp(buf, "FORM", 4) == 0) && (memcmp(buf+8, "ILBM", 4) == 0);
}


// ReadIFF:
// Based on xv source code by Thomas Meyer
// Permission for use in wxWidgets has been gratefully given.

typedef unsigned char byte;
#define IFFDEBUG 0

/*************************************************************************
  void decomprle(source, destination, source length, buffer size)

  Decompress run-length encoded data from source to destination. Terminates
  when source is decoded completely or destination buffer is full.

  The decruncher is as optimized as I could make it, without risking
  safety in case of corrupt BODY chunks.
**************************************************************************/

static void decomprle(const byte *sptr, byte *dptr, long slen, long dlen)
{
    byte codeByte, dataByte;

    while ((slen > 0) && (dlen > 0)) {
    // read control byte
    codeByte = *sptr++;

    if (codeByte < 0x80) {
        codeByte++;
        if ((slen > (long) codeByte) && (dlen >= (long) codeByte)) {
        slen -= codeByte + 1;
        dlen -= codeByte;
        while (codeByte > 0) {
            *dptr++ = *sptr++;
            codeByte--;
        }
        }
        else slen = 0;
    }

    else if (codeByte > 0x80) {
        codeByte = 0x81 - (codeByte & 0x7f);
        if ((slen > (long) 0) && (dlen >= (long) codeByte)) {
        dataByte = *sptr++;
        slen -= 2;
        dlen -= codeByte;
        while (codeByte > 0) {
            *dptr++ = dataByte;
            codeByte--;
        }
        }
        else slen = 0;
    }
    }
}

/******************************************/
static unsigned int iff_getword(const byte *ptr)
{
    unsigned int v;

    v = *ptr++;
    v = (v << 8) + *ptr;
    return v;
}

/******************************************/
static unsigned long iff_getlong(const byte *ptr)
{
    unsigned long l;

    l = *ptr++;
    l = (l << 8) + *ptr++;
    l = (l << 8) + *ptr++;
    l = (l << 8) + *ptr;
    return l;
}

// Define internal ILBM types
#define ILBM_NORMAL     0
#define ILBM_EHB        1
#define ILBM_HAM        2
#define ILBM_HAM8       3
#define ILBM_24BIT      4

int wxIFFDecoder::ReadIFF()
{
    Destroy();

    m_image = new IFFImage();
    if (m_image == 0) {
    Destroy();
    return wxIFF_MEMERR;
    }

    // compute file length
    wxFileOffset currentPos = m_f->TellI();
    m_f->SeekI(0, wxFromEnd);
    long filesize = m_f->TellI();
    m_f->SeekI(currentPos, wxFromStart);

    // allocate memory for complete file
    if ((databuf = new byte[filesize]) == 0) {
    Destroy();
    return wxIFF_MEMERR;
    }

    m_f->Read(databuf, filesize);
    const byte *dataend = databuf + filesize;

    // initialize work pointer. used to trace the buffer for IFF chunks
    const byte *dataptr = databuf;

    // check for minmal size
    if (dataptr + 12 > dataend) {
    Destroy();
    return wxIFF_INVFORMAT;
    }

    // check if we really got an IFF file
    if (strncmp((char *)dataptr, "FORM", 4) != 0) {
    Destroy();
    return wxIFF_INVFORMAT;
    }

    dataptr = dataptr + 8;                  // skip ID and length of FORM

    // check if the IFF file is an ILBM (picture) file
    if (strncmp((char *) dataptr, "ILBM", 4) != 0) {
    Destroy();
    return wxIFF_INVFORMAT;
    }

    wxLogTrace(_T("iff"), _T("IFF ILBM file recognized"));

    dataptr = dataptr + 4;                                // skip ID

    //
    // main decoding loop. searches IFF chunks and handles them.
    // terminates when BODY chunk was found or dataptr ran over end of file
    //
    bool BMHDok = false, CMAPok = false, CAMGok = false;
    int bmhd_width = 0, bmhd_height = 0, bmhd_bitplanes = 0, bmhd_transcol = -1;
    byte bmhd_masking = 0, bmhd_compression = 0;
    long camg_viewmode = 0;
    int colors = 0;
    while (dataptr + 8 <= dataend) {
    // get chunk length and make even
    size_t chunkLen = (iff_getlong(dataptr + 4) + 1) & 0xfffffffe;
#ifdef __VMS
       // Silence compiler warning
       int chunkLen_;
       chunkLen_ = chunkLen;
       if (chunkLen_ < 0) {     // format error?
#else
       if (chunkLen < 0) {     // format error?
#endif
         break;
    }
    bool truncated = (dataptr + 8 + chunkLen > dataend);

    if (strncmp((char *)dataptr, "BMHD", 4) == 0) { // BMHD chunk?
        if (chunkLen < 12 + 2 || truncated) {
        break;
        }
        bmhd_width = iff_getword(dataptr + 8);      // width of picture
        bmhd_height= iff_getword(dataptr + 8 + 2);  // height of picture
        bmhd_bitplanes = *(dataptr + 8 + 8);        // # of bitplanes
        bmhd_masking  = *(dataptr + 8 + 9);
        bmhd_compression = *(dataptr + 8 + 10);     // get compression
        bmhd_transcol    = iff_getword(dataptr + 8 + 12);
        BMHDok = true;                              // got BMHD
        dataptr += 8 + chunkLen;                    // to next chunk
    }
    else if (strncmp((char *)dataptr, "CMAP", 4) == 0) { // CMAP ?
        if (truncated) {
        break;
        }
        const byte *cmapptr = dataptr + 8;
        colors = chunkLen / 3;                  // calc no of colors

        delete m_image->pal;
        m_image->pal = 0;
        m_image->colors = colors;
        if (colors > 0) {
        m_image->pal = new byte[3*colors];
        if (!m_image->pal) {
            Destroy();
            return wxIFF_MEMERR;
        }

        // copy colors to color map
        for (int i=0; i < colors; i++) {
            m_image->pal[3*i + 0] = *cmapptr++;
            m_image->pal[3*i + 1] = *cmapptr++;
            m_image->pal[3*i + 2] = *cmapptr++;
        }
        }

        wxLogTrace(_T("iff"), _T("Read %d colors from IFF file."),
            colors);

        CMAPok = true;                              // got CMAP
        dataptr += 8 + chunkLen;                    // to next chunk
    } else if (strncmp((char *)dataptr, "CAMG", 4) == 0) { // CAMG ?
        if (chunkLen < 4 || truncated) {
        break;
        }
        camg_viewmode = iff_getlong(dataptr + 8);   // get viewmodes
        CAMGok = true;                              // got CAMG
        dataptr += 8 + chunkLen;                    // to next chunk
    }
    else if (strncmp((char *)dataptr, "BODY", 4) == 0) { // BODY ?
        if (!BMHDok) {                              // BMHD found?
        break;
        }
        const byte *bodyptr = dataptr + 8;          // -> BODY data

        if (truncated) {
        chunkLen = dataend - dataptr;
        }

        //
            // if BODY is compressed, allocate buffer for decrunched BODY
        // and decompress it (run length encoding)
        //
        if (bmhd_compression == 1) {
        // calc size of decrunch buffer - (size of the actual pic.
        // decompressed in interleaved Amiga bitplane format)

        size_t decomp_bufsize = (((bmhd_width + 15) >> 4) << 1)
            * bmhd_height * bmhd_bitplanes;

        if ((decomp_mem = new byte[decomp_bufsize]) == 0) {
            Destroy();
            return wxIFF_MEMERR;
        }

        decomprle(bodyptr, decomp_mem, chunkLen, decomp_bufsize);
        bodyptr = decomp_mem;                 // -> uncompressed BODY
        chunkLen = decomp_bufsize;
        delete [] databuf;
        databuf = 0;
        }

        // the following determines the type of the ILBM file.
        // it's either NORMAL, EHB, HAM, HAM8 or 24BIT

        int fmt = ILBM_NORMAL;                 // assume normal ILBM
        if (bmhd_bitplanes == 24) {
        fmt = ILBM_24BIT;
        } else if (bmhd_bitplanes == 8) {
        if (CAMGok && (camg_viewmode & 0x800)) {
            fmt = ILBM_HAM8;
        }
        } else if ((bmhd_bitplanes > 5) && CAMGok) {
        if (camg_viewmode & 0x80) {
            fmt = ILBM_EHB;
        } else if (camg_viewmode & 0x800) {
            fmt = ILBM_HAM;
        }
        }

        wxLogTrace(_T("iff"),
            _T("LoadIFF: %s %dx%d, planes=%d (%d cols), comp=%d"),
            (fmt==ILBM_NORMAL) ? "Normal ILBM" :
            (fmt==ILBM_HAM)    ? "HAM ILBM" :
            (fmt==ILBM_HAM8)   ? "HAM8 ILBM" :
            (fmt==ILBM_EHB)    ? "EHB ILBM" :
            (fmt==ILBM_24BIT)  ? "24BIT ILBM" : "unknown ILBM",
            bmhd_width, bmhd_height, bmhd_bitplanes,
            1<<bmhd_bitplanes, bmhd_compression);

        if ((fmt==ILBM_NORMAL) || (fmt==ILBM_EHB) || (fmt==ILBM_HAM)) {
        wxLogTrace(_T("iff"),
            _T("Converting CMAP from normal ILBM CMAP"));

        switch(fmt) {
            case ILBM_NORMAL: colors = 1 << bmhd_bitplanes; break;
            case ILBM_EHB:    colors = 32*2; break;
            case ILBM_HAM:    colors = 16; break;
        }

        if (colors > m_image->colors) {
            byte *pal = new byte[colors*3];
            if (!pal) {
            Destroy();
            return wxIFF_MEMERR;
            }
            int i;
            for (i = 0; i < m_image->colors; i++) {
            pal[3*i + 0] = m_image->pal[3*i + 0];
            pal[3*i + 1] = m_image->pal[3*i + 1];
            pal[3*i + 2] = m_image->pal[3*i + 2];
            }
            for (; i < colors; i++) {
            pal[3*i + 0] = 0;
            pal[3*i + 1] = 0;
            pal[3*i + 2] = 0;
            }
            delete m_image->pal;
            m_image->pal = pal;
            m_image->colors = colors;
        }

            for (int i=0; i < colors; i++) {
            m_image->pal[3*i + 0] = (m_image->pal[3*i + 0] >> 4) * 17;
            m_image->pal[3*i + 1] = (m_image->pal[3*i + 1] >> 4) * 17;
            m_image->pal[3*i + 2] = (m_image->pal[3*i + 2] >> 4) * 17;
        }
        }

        m_image->p = new byte[bmhd_width * bmhd_height * 3];
            byte *picptr = m_image->p;
        if (!picptr) {
        Destroy();
        return wxIFF_MEMERR;
        }

        byte *pal = m_image->pal;
        int lineskip = ((bmhd_width + 15) >> 4) << 1;
            int height = chunkLen / (lineskip * bmhd_bitplanes);

        if (bmhd_height < height) {
            height = bmhd_height;
        }

        if (fmt == ILBM_HAM || fmt == ILBM_HAM8 || fmt == ILBM_24BIT) {
        byte *pic = picptr;
        const byte *workptr = bodyptr;

        for (int i=0; i < height; i++) {
            byte bitmsk = 0x80;
            const byte *workptr2 = workptr;

            // at start of each line, init RGB values to background
            byte rval = pal[0];
            byte gval = pal[1];
            byte bval = pal[2];

            for (int j=0; j < bmhd_width; j++) {
            long col = 0;
            long colbit = 1;
            const byte *workptr3 = workptr2;
            for (int k=0; k < bmhd_bitplanes; k++) {
                if (*workptr3 & bitmsk) {
                col += colbit;
                }
                workptr3 += lineskip;
                colbit <<= 1;
            }

            if (fmt==ILBM_HAM) {
                int c = (col & 0x0f);
                switch (col & 0x30) {
                case 0x00: if (c >= 0 && c < colors) {
                           rval = pal[3*c + 0];
                           gval = pal[3*c + 1];
                           bval = pal[3*c + 2];
                       }
                       break;

                case 0x10: bval = c * 17;
                       break;

                case 0x20: rval = c * 17;
                       break;

                case 0x30: gval = c * 17;
                       break;
                }
            } else if (fmt == ILBM_HAM8) {
                int c = (col & 0x3f);
                switch(col & 0xc0) {
                case 0x00: if (c >= 0 && c < colors) {
                           rval = pal[3*c + 0];
                           gval = pal[3*c + 1];
                           bval = pal[3*c + 2];
                       }
                       break;

                case 0x40: bval = (bval & 3) | (c << 2);
                       break;

                case 0x80: rval = (rval & 3) | (c << 2);
                       break;

                case 0xc0: gval = (rval & 3) | (c << 2);
                }
            } else {
                rval = col & 0xff;
                gval = (col >> 8) & 0xff;
                bval = (col >> 16) & 0xff;
            }

            *pic++ = rval;
            *pic++ = gval;
            *pic++ = bval;

            bitmsk = bitmsk >> 1;
            if (bitmsk == 0) {
                bitmsk = 0x80;
                workptr2++;
            }
            }
            workptr += lineskip * bmhd_bitplanes;
        }
        }  else if ((fmt == ILBM_NORMAL) || (fmt == ILBM_EHB)) {
        if (fmt == ILBM_EHB) {
            wxLogTrace(_T("iff"), _T("Doubling CMAP for EHB mode"));

            for (int i=0; i<32; i++) {
            pal[3*(i + 32) + 0] = pal[3*i + 0] >> 1;
            pal[3*(i + 32) + 1] = pal[3*i + 1] >> 1;
            pal[3*(i + 32) + 2] = pal[3*i + 2] >> 1;
            }
        }

        byte *pic = picptr;         // ptr to buffer
        const byte *workptr = bodyptr;  // ptr to pic, planar format

        if (bmhd_height < height) {
            height = bmhd_height;
        }

        for (int i=0; i < height; i++) {
            byte bitmsk = 0x80;                 // left most bit (mask)
            const byte *workptr2 = workptr;     // work ptr to source
            for (int j=0; j < bmhd_width; j++) {
            long col = 0;
            long colbit = 1;
            const byte *workptr3 = workptr2;  // 1st byte in 1st pln

            for (int k=0; k < bmhd_bitplanes; k++) {
                if (*workptr3 & bitmsk) { // if bit set in this pln
                col = col + colbit; // add bit to chunky byte
                }
                workptr3 += lineskip;   // go to next line
                colbit <<= 1;           // shift color bit
            }

            if (col >= 0 && col < colors) {
                pic[0] = pal[3*col + 0];
                pic[1] = pal[3*col + 1];
                pic[2] = pal[3*col + 2];
            } else {
                pic[0] = pic[1] = pic[2] = 0;
            }
            pic += 3;
            bitmsk = bitmsk >> 1;   // shift mask to next bit
            if (bitmsk == 0) {      // if mask is zero
                bitmsk = 0x80;      // reset mask
                workptr2++;         // mv ptr to next byte
            }
            }

            workptr += lineskip * bmhd_bitplanes;  // to next line
        }
        } else {
        break;      // unknown format
        }

        m_image->w = bmhd_width;
        m_image->h = height;
        m_image->transparent = bmhd_transcol;

        wxLogTrace(_T("iff"), _T("Loaded IFF picture %s"),
            truncated? "truncated" : "completely");

        return (truncated? wxIFF_TRUNCATED : wxIFF_OK);
    } else {
        wxLogTrace(_T("iff"), _T("Skipping unknown chunk '%c%c%c%c'"),
                *dataptr, *(dataptr+1), *(dataptr+2), *(dataptr+3));

        dataptr = dataptr + 8 + chunkLen;      // skip unknown chunk
    }
    }

    Destroy();
    return wxIFF_INVFORMAT;
}



//-----------------------------------------------------------------------------
// wxIFFHandler
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxIFFHandler, wxImageHandler)

#if wxUSE_STREAMS

bool wxIFFHandler::LoadFile(wxImage *image, wxInputStream& stream,
                            bool verbose, int WXUNUSED(index))
{
    wxIFFDecoder *decod;
    int error;
    bool ok;

    decod = new wxIFFDecoder(&stream);
    error = decod->ReadIFF();

    if ((error != wxIFF_OK) && (error != wxIFF_TRUNCATED))
    {
        if (verbose)
        {
            switch (error)
            {
                case wxIFF_INVFORMAT:
                    wxLogError(_("IFF: error in IFF image format."));
                    break;
                case wxIFF_MEMERR:
                    wxLogError(_("IFF: not enough memory."));
                    break;
                default:
                    wxLogError(_("IFF: unknown error!!!"));
                    break;
            }
        }
        delete decod;
        return false;
    }

    if ((error == wxIFF_TRUNCATED) && verbose)
    {
        wxLogError(_("IFF: data stream seems to be truncated."));
        /* go on; image data is OK */
    }

    ok = decod->ConvertToImage(image);
    delete decod;

    return ok;
}

bool wxIFFHandler::SaveFile(wxImage * WXUNUSED(image),
                            wxOutputStream& WXUNUSED(stream), bool verbose)
{
    if (verbose)
        wxLogDebug(wxT("IFF: the handler is read-only!!"));

    return false;
}

bool wxIFFHandler::DoCanRead(wxInputStream& stream)
{
    wxIFFDecoder decod(&stream);

    return decod.CanRead();
}

#endif // wxUSE_STREAMS

#endif // wxUSE_IFF
