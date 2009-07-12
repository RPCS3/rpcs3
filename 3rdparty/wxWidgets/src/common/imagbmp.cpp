/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/imagbmp.cpp
// Purpose:     wxImage BMP,ICO and CUR handlers
// Author:      Robert Roebling, Chris Elliott
// RCS-ID:      $Id: imagbmp.cpp 54942 2008-08-03 00:23:38Z VZ $
// Copyright:   (c) Robert Roebling, Chris Elliott
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_IMAGE

#include "wx/imagbmp.h"

#ifndef WX_PRECOMP
    #ifdef __WXMSW__
        #include "wx/msw/wrapwin.h"
    #endif
    #include "wx/log.h"
    #include "wx/app.h"
    #include "wx/bitmap.h"
    #include "wx/palette.h"
    #include "wx/intl.h"
#endif

#include "wx/filefn.h"
#include "wx/wfstream.h"
#include "wx/quantize.h"
#include "wx/anidecod.h"

// For memcpy
#include <string.h>

#ifdef __SALFORDC__
#ifdef FAR
#undef FAR
#endif
#endif

//-----------------------------------------------------------------------------
// wxBMPHandler
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxBMPHandler,wxImageHandler)

#if wxUSE_STREAMS

#ifndef BI_RGB
    #define BI_RGB       0
#endif

#ifndef BI_RLE8
#define BI_RLE8      1
#endif

#ifndef BI_RLE4
#define BI_RLE4      2
#endif

#ifndef BI_BITFIELDS
#define BI_BITFIELDS 3
#endif

#define poffset (line * width * 3 + column * 3)

bool wxBMPHandler::SaveFile(wxImage *image,
                            wxOutputStream& stream,
                            bool verbose)
{
    return SaveDib(image, stream, verbose, true/*IsBmp*/, false/*IsMask*/);
}

bool wxBMPHandler::SaveDib(wxImage *image,
                           wxOutputStream& stream,
                           bool verbose,
                           bool IsBmp,
                           bool IsMask)

{
    wxCHECK_MSG( image, false, _T("invalid pointer in wxBMPHandler::SaveFile") );

    if ( !image->Ok() )
    {
        if ( verbose )
            wxLogError(_("BMP: Couldn't save invalid image."));
        return false;
    }

    // get the format of the BMP file to save, else use 24bpp
    unsigned format = wxBMP_24BPP;
    if ( image->HasOption(wxIMAGE_OPTION_BMP_FORMAT) )
        format = image->GetOptionInt(wxIMAGE_OPTION_BMP_FORMAT);

    wxUint16 bpp;     // # of bits per pixel
    int palette_size; // # of color map entries, ie. 2^bpp colors

    // set the bpp and appropriate palette_size, and do additional checks
    if ( (format == wxBMP_1BPP) || (format == wxBMP_1BPP_BW) )
    {
        bpp = 1;
        palette_size = 2;
    }
    else if ( format == wxBMP_4BPP )
    {
        bpp = 4;
        palette_size = 16;
    }
    else if ( (format == wxBMP_8BPP) || (format == wxBMP_8BPP_GREY) ||
              (format == wxBMP_8BPP_RED) || (format == wxBMP_8BPP_PALETTE) )
    {
        // need to set a wxPalette to use this, HOW TO CHECK IF VALID, SIZE?
        if ((format == wxBMP_8BPP_PALETTE)
#if wxUSE_PALETTE
                && !image->HasPalette()
#endif // wxUSE_PALETTE
            )
        {
            if ( verbose )
                wxLogError(_("BMP: wxImage doesn't have own wxPalette."));
            return false;
        }
        bpp = 8;
        palette_size = 256;
    }
    else  // you get 24bpp
    {
        format = wxBMP_24BPP;
        bpp = 24;
        palette_size = 0;
    }

    unsigned width = image->GetWidth();
    unsigned row_padding = (4 - int(width*bpp/8.0) % 4) % 4; // # bytes to pad to dword
    unsigned row_width = int(width * bpp/8.0) + row_padding; // # of bytes per row

    struct
    {
        // BitmapHeader:
        wxUint16  magic;          // format magic, always 'BM'
        wxUint32  filesize;       // total file size, inc. headers
        wxUint32  reserved;       // for future use
        wxUint32  data_offset;    // image data offset in the file

        // BitmapInfoHeader:
        wxUint32  bih_size;       // 2nd part's size
        wxUint32  width, height;  // bitmap's dimensions
        wxUint16  planes;         // num of planes
        wxUint16  bpp;            // bits per pixel
        wxUint32  compression;    // compression method
        wxUint32  size_of_bmp;    // size of the bitmap
        wxUint32  h_res, v_res;   // image resolution in dpi
        wxUint32  num_clrs;       // number of colors used
        wxUint32  num_signif_clrs;// number of significant colors
    } hdr;

    wxUint32 hdr_size = 14/*BitmapHeader*/ + 40/*BitmapInfoHeader*/;

    hdr.magic = wxUINT16_SWAP_ON_BE(0x4D42/*'BM'*/);
    hdr.filesize = wxUINT32_SWAP_ON_BE( hdr_size + palette_size*4 +
                                        row_width * image->GetHeight() );
    hdr.reserved = 0;
    hdr.data_offset = wxUINT32_SWAP_ON_BE(hdr_size + palette_size*4);

    hdr.bih_size = wxUINT32_SWAP_ON_BE(hdr_size - 14);
    hdr.width = wxUINT32_SWAP_ON_BE(image->GetWidth());
    if ( IsBmp )
    {
        hdr.height = wxUINT32_SWAP_ON_BE(image->GetHeight());
    }
    else
    {
        hdr.height = wxUINT32_SWAP_ON_BE(2 * image->GetHeight());
    }
    hdr.planes = wxUINT16_SWAP_ON_BE(1); // always 1 plane
    hdr.bpp = wxUINT16_SWAP_ON_BE(bpp);
    hdr.compression = 0; // RGB uncompressed
    hdr.size_of_bmp = wxUINT32_SWAP_ON_BE(row_width * image->GetHeight());
    hdr.h_res = hdr.v_res = wxUINT32_SWAP_ON_BE(72);  // 72dpi is standard
    hdr.num_clrs = wxUINT32_SWAP_ON_BE(palette_size); // # colors in colormap
    hdr.num_signif_clrs = 0;     // all colors are significant

    if ( IsBmp )
    {
        if (// VS: looks ugly but compilers tend to do ugly things with structs,
            //     like aligning hdr.filesize's ofset to dword :(
            // VZ: we should add padding then...
            !stream.Write(&hdr.magic, 2) ||
            !stream.Write(&hdr.filesize, 4) ||
            !stream.Write(&hdr.reserved, 4) ||
            !stream.Write(&hdr.data_offset, 4)
           )
        {
            if (verbose)
                wxLogError(_("BMP: Couldn't write the file (Bitmap) header."));
            return false;
        }
    }
    if ( !IsMask )
    {
        if (
            !stream.Write(&hdr.bih_size, 4) ||
            !stream.Write(&hdr.width, 4) ||
            !stream.Write(&hdr.height, 4) ||
            !stream.Write(&hdr.planes, 2) ||
            !stream.Write(&hdr.bpp, 2) ||
            !stream.Write(&hdr.compression, 4) ||
            !stream.Write(&hdr.size_of_bmp, 4) ||
            !stream.Write(&hdr.h_res, 4) ||
            !stream.Write(&hdr.v_res, 4) ||
            !stream.Write(&hdr.num_clrs, 4) ||
            !stream.Write(&hdr.num_signif_clrs, 4)
           )
        {
            if (verbose)
                wxLogError(_("BMP: Couldn't write the file (BitmapInfo) header."));
            return false;
        }
    }

    wxPalette *palette = NULL; // entries for quantized images
    wxUint8 *rgbquad = NULL;   // for the RGBQUAD bytes for the colormap
    wxImage *q_image = NULL;   // destination for quantized image

    // if <24bpp use quantization to reduce colors for *some* of the formats
    if ( (format == wxBMP_1BPP) || (format == wxBMP_4BPP) ||
         (format == wxBMP_8BPP) || (format == wxBMP_8BPP_PALETTE) )
    {
        // make a new palette and quantize the image
        if (format != wxBMP_8BPP_PALETTE)
        {
            q_image = new wxImage();

            // I get a delete error using Quantize when desired colors > 236
            int quantize = ((palette_size > 236) ? 236 : palette_size);
            // fill the destination too, it gives much nicer 4bpp images
            wxQuantize::Quantize( *image, *q_image, &palette, quantize, 0,
                                  wxQUANTIZE_FILL_DESTINATION_IMAGE );
        }
        else
        {
#if wxUSE_PALETTE
            palette = new wxPalette(image->GetPalette());
#endif // wxUSE_PALETTE
        }

        int i;
        unsigned char r, g, b;
        rgbquad = new wxUint8 [palette_size*4];

        for (i = 0; i < palette_size; i++)
        {
#if wxUSE_PALETTE
            if ( !palette->GetRGB(i, &r, &g, &b) )
#endif // wxUSE_PALETTE
                r = g = b = 0;

            rgbquad[i*4] = b;
            rgbquad[i*4+1] = g;
            rgbquad[i*4+2] = r;
            rgbquad[i*4+3] = 0;
        }
    }
    // make a 256 entry greyscale colormap or 2 entry black & white
    else if ( (format == wxBMP_8BPP_GREY) || (format == wxBMP_8BPP_RED) ||
              (format == wxBMP_1BPP_BW) )
    {
        rgbquad = new wxUint8 [palette_size*4];

        for ( int i = 0; i < palette_size; i++ )
        {
            // if 1BPP_BW then the value should be either 0 or 255
            wxUint8 c = (wxUint8)((i > 0) && (format == wxBMP_1BPP_BW) ? 255 : i);

            rgbquad[i*4] =
            rgbquad[i*4+1] =
            rgbquad[i*4+2] = c;
            rgbquad[i*4+3] = 0;
        }
    }

    // if the colormap was made, then it needs to be written
    if (rgbquad)
    {
        if ( !IsMask )
        {
            if ( !stream.Write(rgbquad, palette_size*4) )
            {
                if (verbose)
                    wxLogError(_("BMP: Couldn't write RGB color map."));
                delete[] rgbquad;
#if wxUSE_PALETTE
                delete palette;
#endif // wxUSE_PALETTE
                delete q_image;
                return false;
            }
            }
        delete []rgbquad;
    }

    // pointer to the image data, use quantized if available
    wxUint8 *data = (wxUint8*) image->GetData();
    if (q_image) if (q_image->Ok()) data = (wxUint8*) q_image->GetData();

    wxUint8 *buffer = new wxUint8[row_width];
    memset(buffer, 0, row_width);
    int y; unsigned x;
    long int pixel;

    for (y = image->GetHeight() -1; y >= 0; y--)
    {
        if ( format == wxBMP_24BPP ) // 3 bytes per pixel red,green,blue
        {
            for ( x = 0; x < width; x++ )
            {
                pixel = 3*(y*width + x);

                buffer[3*x    ] = data[pixel+2];
                buffer[3*x + 1] = data[pixel+1];
                buffer[3*x + 2] = data[pixel];
            }
        }
        else if ((format == wxBMP_8BPP) ||       // 1 byte per pixel in color
                 (format == wxBMP_8BPP_PALETTE))
        {
            for (x = 0; x < width; x++)
            {
                pixel = 3*(y*width + x);
#if wxUSE_PALETTE
                buffer[x] = (wxUint8)palette->GetPixel( data[pixel],
                                                        data[pixel+1],
                                                        data[pixel+2] );
#else
                // FIXME: what should this be? use some std palette maybe?
                buffer[x] = 0;
#endif // wxUSE_PALETTE
            }
        }
        else if ( format == wxBMP_8BPP_GREY ) // 1 byte per pix, rgb ave to grey
        {
            for (x = 0; x < width; x++)
            {
                pixel = 3*(y*width + x);
                buffer[x] = (wxUint8)(.299*data[pixel] +
                                      .587*data[pixel+1] +
                                      .114*data[pixel+2]);
            }
        }
        else if ( format == wxBMP_8BPP_RED ) // 1 byte per pixel, red as greys
        {
            for (x = 0; x < width; x++)
            {
                buffer[x] = (wxUint8)data[3*(y*width + x)];
            }
        }
        else if ( format == wxBMP_4BPP ) // 4 bpp in color
        {
            for (x = 0; x < width; x+=2)
            {
                pixel = 3*(y*width + x);

                // fill buffer, ignore if > width
#if wxUSE_PALETTE
                buffer[x/2] = (wxUint8)(
                    ((wxUint8)palette->GetPixel(data[pixel],
                                                data[pixel+1],
                                                data[pixel+2]) << 4) |
                    (((x+1) > width)
                     ? 0
                     : ((wxUint8)palette->GetPixel(data[pixel+3],
                                                   data[pixel+4],
                                                   data[pixel+5]) ))    );
#else
                // FIXME: what should this be? use some std palette maybe?
                buffer[x/2] = 0;
#endif // wxUSE_PALETTE
            }
        }
        else if ( format == wxBMP_1BPP ) // 1 bpp in "color"
        {
            for (x = 0; x < width; x+=8)
            {
                pixel = 3*(y*width + x);

#if wxUSE_PALETTE
                buffer[x/8] = (wxUint8)(
                                           ((wxUint8)palette->GetPixel(data[pixel], data[pixel+1], data[pixel+2]) << 7) |
                    (((x+1) > width) ? 0 : ((wxUint8)palette->GetPixel(data[pixel+3], data[pixel+4], data[pixel+5]) << 6)) |
                    (((x+2) > width) ? 0 : ((wxUint8)palette->GetPixel(data[pixel+6], data[pixel+7], data[pixel+8]) << 5)) |
                    (((x+3) > width) ? 0 : ((wxUint8)palette->GetPixel(data[pixel+9], data[pixel+10], data[pixel+11]) << 4)) |
                    (((x+4) > width) ? 0 : ((wxUint8)palette->GetPixel(data[pixel+12], data[pixel+13], data[pixel+14]) << 3)) |
                    (((x+5) > width) ? 0 : ((wxUint8)palette->GetPixel(data[pixel+15], data[pixel+16], data[pixel+17]) << 2)) |
                    (((x+6) > width) ? 0 : ((wxUint8)palette->GetPixel(data[pixel+18], data[pixel+19], data[pixel+20]) << 1)) |
                    (((x+7) > width) ? 0 : ((wxUint8)palette->GetPixel(data[pixel+21], data[pixel+22], data[pixel+23])     ))    );
#else
                // FIXME: what should this be? use some std palette maybe?
                buffer[x/8] = 0;
#endif // wxUSE_PALETTE
            }
        }
        else if ( format == wxBMP_1BPP_BW ) // 1 bpp B&W colormap from red color ONLY
        {
            for (x = 0; x < width; x+=8)
            {
                pixel = 3*(y*width + x);

                buffer[x/8] = (wxUint8)(
                                          (((wxUint8)(data[pixel]   /128.)) << 7) |
                   (((x+1) > width) ? 0 : (((wxUint8)(data[pixel+3] /128.)) << 6)) |
                   (((x+2) > width) ? 0 : (((wxUint8)(data[pixel+6] /128.)) << 5)) |
                   (((x+3) > width) ? 0 : (((wxUint8)(data[pixel+9] /128.)) << 4)) |
                   (((x+4) > width) ? 0 : (((wxUint8)(data[pixel+12]/128.)) << 3)) |
                   (((x+5) > width) ? 0 : (((wxUint8)(data[pixel+15]/128.)) << 2)) |
                   (((x+6) > width) ? 0 : (((wxUint8)(data[pixel+18]/128.)) << 1)) |
                   (((x+7) > width) ? 0 : (((wxUint8)(data[pixel+21]/128.))     ))    );
            }
        }

        if ( !stream.Write(buffer, row_width) )
        {
            if (verbose)
                wxLogError(_("BMP: Couldn't write data."));
            delete[] buffer;
#if wxUSE_PALETTE
            delete palette;
#endif // wxUSE_PALETTE
            delete q_image;
            return false;
        }
    }
    delete[] buffer;
#if wxUSE_PALETTE
    delete palette;
#endif // wxUSE_PALETTE
    delete q_image;

    return true;
}


typedef struct
{
    unsigned char r, g, b;
}  _cmap;

bool wxBMPHandler::DoLoadDib(wxImage * image, int width, int height,
                             int bpp, int ncolors, int comp,
                             wxFileOffset bmpOffset, wxInputStream& stream,
                             bool verbose, bool IsBmp, bool hasPalette)
{
    wxInt32         aDword, rmask = 0, gmask = 0, bmask = 0, amask = 0;
    int             rshift = 0, gshift = 0, bshift = 0, ashift = 0;
    int             rbits = 0, gbits = 0, bbits = 0, abits = 0;
    wxInt32         dbuf[4];
    wxInt8          bbuf[4];
    wxUint8         aByte;
    wxUint16        aWord;

    // allocate space for palette if needed:
    _cmap *cmap;

    if ( bpp < 16 )
    {
        cmap = new _cmap[ncolors];
        if ( !cmap )
        {
            if (verbose)
                wxLogError(_("BMP: Couldn't allocate memory."));
            return false;
        }
    }
    else
        cmap = NULL;

    // destroy existing here instead of:
    image->Destroy();
    image->Create(width, height);

    unsigned char *ptr = image->GetData();

    if ( !ptr )
    {
        if ( verbose )
            wxLogError( _("BMP: Couldn't allocate memory.") );
        delete[] cmap;
        return false;
    }

    unsigned char *alpha;
    if ( bpp == 32 )
    {
        // tell the image to allocate an alpha buffer
        image->SetAlpha();
        alpha = image->GetAlpha();
        if ( !alpha )
        {
            if ( verbose )
                wxLogError(_("BMP: Couldn't allocate memory."));
            delete[] cmap;
            return false;
        }
    }
    else // no alpha
    {
        alpha = NULL;
    }

    // Reading the palette, if it exists:
    if ( bpp < 16 && ncolors != 0 )
    {
        unsigned char* r = new unsigned char[ncolors];
        unsigned char* g = new unsigned char[ncolors];
        unsigned char* b = new unsigned char[ncolors];
        for (int j = 0; j < ncolors; j++)
        {
            if (hasPalette)
            {
                stream.Read(bbuf, 4);
                cmap[j].b = bbuf[0];
                cmap[j].g = bbuf[1];
                cmap[j].r = bbuf[2];

                r[j] = cmap[j].r;
                g[j] = cmap[j].g;
                b[j] = cmap[j].b;
            }
            else
            {
                //used in reading .ico file mask
                r[j] = cmap[j].r =
                g[j] = cmap[j].g =
                b[j] = cmap[j].b = ( j ? 255 : 0 );
            }
        }

#if wxUSE_PALETTE
        // Set the palette for the wxImage
        image->SetPalette(wxPalette(ncolors, r, g, b));
#endif // wxUSE_PALETTE

        delete[] r;
        delete[] g;
        delete[] b;
    }
    else if ( bpp == 16 || bpp == 32 )
    {
        if ( comp == BI_BITFIELDS )
        {
            int bit = 0;
            stream.Read(dbuf, 4 * 3);
            rmask = wxINT32_SWAP_ON_BE(dbuf[0]);
            gmask = wxINT32_SWAP_ON_BE(dbuf[1]);
            bmask = wxINT32_SWAP_ON_BE(dbuf[2]);
            // find shift amount (Least significant bit of mask)
            for (bit = bpp-1; bit>=0; bit--)
            {
                if (bmask & (1 << bit))
                    bshift = bit;
                if (gmask & (1 << bit))
                    gshift = bit;
                if (rmask & (1 << bit))
                    rshift = bit;
            }
            // Find number of bits in mask (MSB-LSB+1)
            for (bit = 0; bit < bpp; bit++)
            {
                if (bmask & (1 << bit))
                    bbits = bit-bshift+1;
                if (gmask & (1 << bit))
                    gbits = bit-gshift+1;
                if (rmask & (1 << bit))
                    rbits = bit-rshift+1;
            }
        }
        else if ( bpp == 16 )
        {
            rmask = 0x7C00;
            gmask = 0x03E0;
            bmask = 0x001F;
            rshift = 10;
            gshift = 5;
            bshift = 0;
            rbits = 5;
            gbits = 5;
            bbits = 5;
        }
        else if ( bpp == 32 )
        {
            rmask = 0x00FF0000;
            gmask = 0x0000FF00;
            bmask = 0x000000FF;
            amask = 0xFF000000;

            ashift = 24;
            rshift = 16;
            gshift = 8;
            bshift = 0;
            abits = 8;
            rbits = 8;
            gbits = 8;
            bbits = 8;
        }
    }

    /*
     * Reading the image data
     */
    if ( IsBmp )
        stream.SeekI(bmpOffset); // else icon, just carry on

    unsigned char *data = ptr;

    /* set the whole image to the background color */
    if ( bpp < 16 && (comp == BI_RLE4 || comp == BI_RLE8) )
    {
        for (int i = 0; i < width * height; i++)
        {
            *ptr++ = cmap[0].r;
            *ptr++ = cmap[0].g;
            *ptr++ = cmap[0].b;
        }
        ptr = data;
    }

    int linesize = ((width * bpp + 31) / 32) * 4;

    /* BMPs are stored upside down */
    for ( int line = (height - 1); line >= 0; line-- )
    {
        int linepos = 0;
        for ( int column = 0; column < width ; )
        {
            if ( bpp < 16 )
            {
                linepos++;
                aByte = stream.GetC();
                if ( bpp == 1 )
                {
                    for (int bit = 0; bit < 8 && column < width; bit++)
                    {
                        int index = ((aByte & (0x80 >> bit)) ? 1 : 0);
                        ptr[poffset] = cmap[index].r;
                        ptr[poffset + 1] = cmap[index].g;
                        ptr[poffset + 2] = cmap[index].b;
                        column++;
                    }
                }
                else if ( bpp == 4 )
                {
                    if ( comp == BI_RLE4 )
                    {
                        wxUint8 first;
                        first = aByte;
                        aByte = stream.GetC();
                        if ( first == 0 )
                        {
                            if ( aByte == 0 )
                            {
                                if ( column > 0 )
                                    column = width;
                            }
                            else if ( aByte == 1 )
                            {
                                column = width;
                                line = -1;
                            }
                            else if ( aByte == 2 )
                            {
                                aByte = stream.GetC();
                                column += aByte;
                                linepos = column * bpp / 4;
                                aByte = stream.GetC();
                                line -= aByte; // upside down
                            }
                            else
                            {
                                int absolute = aByte;
                                wxUint8 nibble[2] ;
                                int readBytes = 0 ;
                                for (int k = 0; k < absolute; k++)
                                {
                                    if ( !(k % 2 ) )
                                    {
                                        ++readBytes ;
                                        aByte = stream.GetC();
                                        nibble[0] = (wxUint8)( (aByte & 0xF0) >> 4 ) ;
                                        nibble[1] = (wxUint8)( aByte & 0x0F ) ;
                                    }
                                    ptr[poffset    ] = cmap[nibble[k%2]].r;
                                    ptr[poffset + 1] = cmap[nibble[k%2]].g;
                                    ptr[poffset + 2] = cmap[nibble[k%2]].b;
                                    column++;
                                    if ( k % 2 )
                                        linepos++;
                                }
                                if ( readBytes & 0x01 )
                                    aByte = stream.GetC();
                            }
                        }
                        else
                        {
                            wxUint8 nibble[2] ;
                            nibble[0] = (wxUint8)( (aByte & 0xF0) >> 4 ) ;
                            nibble[1] = (wxUint8)( aByte & 0x0F ) ;

                            for ( int l = 0; l < first && column < width; l++ )
                            {
                                ptr[poffset    ] = cmap[nibble[l%2]].r;
                                ptr[poffset + 1] = cmap[nibble[l%2]].g;
                                ptr[poffset + 2] = cmap[nibble[l%2]].b;
                                column++;
                                if ( l % 2 )
                                    linepos++;
                            }
                        }
                    }
                    else
                    {
                        for (int nibble = 0; nibble < 2 && column < width; nibble++)
                        {
                            int index = ((aByte & (0xF0 >> nibble * 4)) >> (!nibble * 4));
                            if ( index >= 16 )
                                index = 15;
                            ptr[poffset] = cmap[index].r;
                            ptr[poffset + 1] = cmap[index].g;
                            ptr[poffset + 2] = cmap[index].b;
                            column++;
                        }
                    }
                }
                else if ( bpp == 8 )
                {
                    if ( comp == BI_RLE8 )
                    {
                        unsigned char first;
                        first = aByte;
                        aByte = stream.GetC();
                        if ( first == 0 )
                        {
                            if ( aByte == 0 )
                            {
                                /* column = width; */
                            }
                            else if ( aByte == 1 )
                            {
                                column = width;
                                line = -1;
                            }
                            else if ( aByte == 2 )
                            {
                                aByte = stream.GetC();
                                column += aByte;
                                linepos = column * bpp / 8;
                                aByte = stream.GetC();
                                line += aByte;
                            }
                            else
                            {
                                int absolute = aByte;
                                for (int k = 0; k < absolute; k++)
                                {
                                    linepos++;
                                    aByte = stream.GetC();
                                    ptr[poffset    ] = cmap[aByte].r;
                                    ptr[poffset + 1] = cmap[aByte].g;
                                    ptr[poffset + 2] = cmap[aByte].b;
                                    column++;
                                }
                                if ( absolute & 0x01 )
                                    aByte = stream.GetC();
                            }
                        }
                        else
                        {
                            for ( int l = 0; l < first && column < width; l++ )
                            {
                                ptr[poffset    ] = cmap[aByte].r;
                                ptr[poffset + 1] = cmap[aByte].g;
                                ptr[poffset + 2] = cmap[aByte].b;
                                column++;
                                linepos++;
                            }
                        }
                    }
                    else
                    {
                        ptr[poffset    ] = cmap[aByte].r;
                        ptr[poffset + 1] = cmap[aByte].g;
                        ptr[poffset + 2] = cmap[aByte].b;
                        column++;
                        // linepos += size;    seems to be wrong, RR
                    }
                }
            }
            else if ( bpp == 24 )
            {
                stream.Read(bbuf, 3);
                linepos += 3;
                ptr[poffset    ] = (unsigned char)bbuf[2];
                ptr[poffset + 1] = (unsigned char)bbuf[1];
                ptr[poffset + 2] = (unsigned char)bbuf[0];
                column++;
            }
            else if ( bpp == 16 )
            {
                unsigned char temp;
                stream.Read(&aWord, 2);
                aWord = wxUINT16_SWAP_ON_BE(aWord);
                linepos += 2;
                /* use the masks and calculated amonut of shift
                   to retrieve the color data out of the word.  Then
                   shift it left by (8 - number of bits) such that
                   the image has the proper dynamic range */
                temp = (unsigned char)((aWord & rmask) >> rshift << (8-rbits));
                ptr[poffset] = temp;
                temp = (unsigned char)((aWord & gmask) >> gshift << (8-gbits));
                ptr[poffset + 1] = temp;
                temp = (unsigned char)((aWord & bmask) >> bshift << (8-bbits));
                ptr[poffset + 2] = temp;
                column++;
            }
            else
            {
                unsigned char temp;
                stream.Read(&aDword, 4);
                aDword = wxINT32_SWAP_ON_BE(aDword);
                linepos += 4;
                temp = (unsigned char)((aDword & rmask) >> rshift);
                ptr[poffset] = temp;
                temp = (unsigned char)((aDword & gmask) >> gshift);
                ptr[poffset + 1] = temp;
                temp = (unsigned char)((aDword & bmask) >> bshift);
                ptr[poffset + 2] = temp;
                if ( alpha )
                {
                    temp = (unsigned char)((aDword & amask) >> ashift);
                    alpha[line * width + column] = temp;
                }
                column++;
            }
        }
        while ( (linepos < linesize) && (comp != 1) && (comp != 2) )
        {
            stream.Read(&aByte, 1);
            linepos += 1;
            if ( !stream )
                break;
        }
    }

    delete[] cmap;

    image->SetMask(false);

    const wxStreamError err = stream.GetLastError();
    return err == wxSTREAM_NO_ERROR || err == wxSTREAM_EOF;
}

bool wxBMPHandler::LoadDib(wxImage *image, wxInputStream& stream,
                           bool verbose, bool IsBmp)
{
    wxUint16        aWord;
    wxInt32         dbuf[4];
    wxInt8          bbuf[4];

    wxFileOffset offset = 0; // keep gcc quiet
    if ( IsBmp )
    {
        // read the header off the .BMP format file

        offset = stream.TellI();
        if (offset == wxInvalidOffset)
            offset = 0;

        stream.Read(bbuf, 2);
        stream.Read(dbuf, 16);
    }
    else
    {
        stream.Read(dbuf, 4);
    }
    #if 0 // unused
        wxInt32 size = wxINT32_SWAP_ON_BE(dbuf[0]);
    #endif
    offset = offset + wxINT32_SWAP_ON_BE(dbuf[2]);

    stream.Read(dbuf, 4 * 2);
    int width = wxINT32_SWAP_ON_BE((int)dbuf[0]);
    int height = wxINT32_SWAP_ON_BE((int)dbuf[1]);
    if ( !IsBmp)height = height  / 2; // for icons divide by 2

    if ( width > 32767 )
    {
        if (verbose)
            wxLogError( _("DIB Header: Image width > 32767 pixels for file.") );
        return false;
    }
    if ( height > 32767 )
    {
        if (verbose)
            wxLogError( _("DIB Header: Image height > 32767 pixels for file.") );
        return false;
    }

    stream.Read(&aWord, 2);
    /*
            TODO
            int planes = (int)wxUINT16_SWAP_ON_BE( aWord );
        */
    stream.Read(&aWord, 2);
    int bpp = wxUINT16_SWAP_ON_BE((int)aWord);
    if ( bpp != 1 && bpp != 4 && bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32 )
    {
        if (verbose)
            wxLogError( _("DIB Header: Unknown bitdepth in file.") );
        return false;
    }

    stream.Read(dbuf, 4 * 4);
    int comp = wxINT32_SWAP_ON_BE((int)dbuf[0]);
    if ( comp != BI_RGB && comp != BI_RLE4 && comp != BI_RLE8 &&
         comp != BI_BITFIELDS )
    {
        if (verbose)
            wxLogError( _("DIB Header: Unknown encoding in file.") );
        return false;
    }

    stream.Read(dbuf, 4 * 2);
    int ncolors = wxINT32_SWAP_ON_BE( (int)dbuf[0] );
    if (ncolors == 0)
        ncolors = 1 << bpp;
    /* some more sanity checks */
    if (((comp == BI_RLE4) && (bpp != 4)) ||
        ((comp == BI_RLE8) && (bpp != 8)) ||
        ((comp == BI_BITFIELDS) && (bpp != 16 && bpp != 32)))
    {
        if (verbose)
            wxLogError( _("DIB Header: Encoding doesn't match bitdepth.") );
        return false;
    }

    //read DIB; this is the BMP image or the XOR part of an icon image
    if ( !DoLoadDib(image, width, height, bpp, ncolors, comp, offset, stream,
                    verbose, IsBmp, true) )
    {
        if (verbose)
            wxLogError( _("Error in reading image DIB.") );
        return false;
    }

    if ( !IsBmp )
    {
        //read Icon mask which is monochrome
        //there is no palette, so we will create one
        wxImage mask;
        if ( !DoLoadDib(&mask, width, height, 1, 2, BI_RGB, offset, stream,
                        verbose, IsBmp, false) )
        {
            if (verbose)
                wxLogError( _("ICO: Error in reading mask DIB.") );
            return false;
        }
        image->SetMaskFromImage(mask, 255, 255, 255);

    }

    return true;
}

bool wxBMPHandler::LoadFile(wxImage *image, wxInputStream& stream,
                            bool verbose, int WXUNUSED(index))
{
    // Read a single DIB fom the file:
    return LoadDib(image, stream, verbose, true/*isBmp*/);
}

bool wxBMPHandler::DoCanRead(wxInputStream& stream)
{
    unsigned char hdr[2];

    if ( !stream.Read(hdr, WXSIZEOF(hdr)) )
        return false;

    // do we have the BMP file signature?
    return hdr[0] == 'B' && hdr[1] == 'M';
}

#endif // wxUSE_STREAMS


#if wxUSE_ICO_CUR
//-----------------------------------------------------------------------------
// wxICOHandler
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxICOHandler, wxBMPHandler)

#if wxUSE_STREAMS

struct ICONDIRENTRY
{
    wxUint8         bWidth;               // Width of the image
    wxUint8         bHeight;              // Height of the image (times 2)
    wxUint8         bColorCount;          // Number of colors in image (0 if >=8bpp)
    wxUint8         bReserved;            // Reserved

    // these two are different in icons and cursors:
                                          // icon           or  cursor
    wxUint16        wPlanes;              // Color Planes   or  XHotSpot
    wxUint16        wBitCount;            // Bits per pixel or  YHotSpot

    wxUint32        dwBytesInRes;         // how many bytes in this resource?
    wxUint32        dwImageOffset;        // where in the file is this image
};

struct ICONDIR
{
    wxUint16     idReserved;   // Reserved
    wxUint16     idType;       // resource type (1 for icons, 2 for cursors)
    wxUint16     idCount;      // how many images?
};


bool wxICOHandler::SaveFile(wxImage *image,
                            wxOutputStream& stream,
                            bool verbose)

{
    //sanity check; icon must be less than 127 pixels high and 255 wide
    if ( image->GetHeight () > 127 )
    {
        if ( verbose )
            wxLogError(_("ICO: Image too tall for an icon."));
        return false;
    }
    if ( image->GetWidth () > 255 )
    {
        if ( verbose )
            wxLogError(_("ICO: Image too wide for an icon."));
        return false;
    }

    const int images = 1; // only generate one image

    // VS: This is a hack of sort - since ICO and CUR files are almost
    //     identical, we have all the meat in wxICOHandler and check for
    //     the actual (handler) type when the code has to distinguish between
    //     the two formats
    int type = (this->GetType() == wxBITMAP_TYPE_CUR) ? 2 : 1;

    // write a header, (ICONDIR)
    // Calculate the header size
    wxUint32 offset = 3 * sizeof(wxUint16);

    ICONDIR IconDir;
    IconDir.idReserved = 0;
    IconDir.idType = wxUINT16_SWAP_ON_BE((wxUint16)type);
    IconDir.idCount = wxUINT16_SWAP_ON_BE((wxUint16)images);
    stream.Write(&IconDir.idReserved, sizeof(IconDir.idReserved));
    stream.Write(&IconDir.idType, sizeof(IconDir.idType));
    stream.Write(&IconDir.idCount, sizeof(IconDir.idCount));
    if ( !stream.IsOk() )
    {
        if ( verbose )
            wxLogError(_("ICO: Error writing the image file!"));
        return false;
    }

    // for each iamage write a description ICONDIRENTRY:
    ICONDIRENTRY icondirentry;
    for (int img = 0; img < images; img++)
    {
        wxImage mask;

        if ( image->HasMask() )
        {
            // make another image with black/white:
            mask = image->ConvertToMono (image->GetMaskRed(), image->GetMaskGreen(), image->GetMaskBlue() );

            // now we need to change the masked regions to black:
            unsigned char r = image->GetMaskRed();
            unsigned char g = image->GetMaskGreen();
            unsigned char b = image->GetMaskBlue();
            if ( (r != 0) || (g != 0) || (b != 0) )
            {
                // Go round and apply black to the masked bits:
                int i, j;
                for (i = 0; i < mask.GetWidth(); i++)
                {
                    for (j = 0; j < mask.GetHeight(); j++)
                    {
                        if ((r == mask.GetRed(i, j)) &&
                            (g == mask.GetGreen(i, j))&&
                            (b == mask.GetBlue(i, j)) )
                                image->SetRGB(i, j, 0, 0, 0 );
                    }
                }
            }
        }
        else
        {
            // just make a black mask all over:
            mask = image->Copy();
            int i, j;
            for (i = 0; i < mask.GetWidth(); i++)
                for (j = 0; j < mask.GetHeight(); j++)
                    mask.SetRGB(i, j, 0, 0, 0 );
        }
        // Set the formats for image and mask
        // (Windows never saves with more than 8 colors):
        image->SetOption(wxIMAGE_OPTION_BMP_FORMAT, wxBMP_8BPP);

        // monochome bitmap:
        mask.SetOption(wxIMAGE_OPTION_BMP_FORMAT, wxBMP_1BPP_BW);
        bool IsBmp = false;
        bool IsMask = false;

        //calculate size and offset of image and mask
        wxCountingOutputStream cStream;
        bool bResult = SaveDib(image, cStream, verbose, IsBmp, IsMask);
        if ( !bResult )
        {
            if ( verbose )
                wxLogError(_("ICO: Error writing the image file!"));
            return false;
        }
        IsMask = true;

        bResult = SaveDib(&mask, cStream, verbose, IsBmp, IsMask);
        if ( !bResult )
        {
            if ( verbose )
                wxLogError(_("ICO: Error writing the image file!"));
            return false;
        }
        wxUint32 Size = cStream.GetSize();

        // wxCountingOutputStream::IsOk() always returns true for now and this
        // "if" provokes VC++ warnings in optimized build
#if 0
        if ( !cStream.Ok() )
        {
            if ( verbose )
                wxLogError(_("ICO: Error writing the image file!"));
            return false;
        }
#endif // 0

        offset = offset + sizeof(ICONDIRENTRY);

        icondirentry.bWidth = (wxUint8)image->GetWidth();
        icondirentry.bHeight = (wxUint8)(2 * image->GetHeight());
        icondirentry.bColorCount = 0;
        icondirentry.bReserved = 0;
        icondirentry.wPlanes = wxUINT16_SWAP_ON_BE(1);
        icondirentry.wBitCount = wxUINT16_SWAP_ON_BE(wxBMP_8BPP);
        if ( type == 2 /*CUR*/)
        {
            int hx = image->HasOption(wxIMAGE_OPTION_CUR_HOTSPOT_X) ?
                         image->GetOptionInt(wxIMAGE_OPTION_CUR_HOTSPOT_X) :
                         image->GetWidth() / 2;
            int hy = image->HasOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y) ?
                         image->GetOptionInt(wxIMAGE_OPTION_CUR_HOTSPOT_Y) :
                         image->GetHeight() / 2;

            // actually write the values of the hot spot here:
            icondirentry.wPlanes = wxUINT16_SWAP_ON_BE((wxUint16)hx);
            icondirentry.wBitCount = wxUINT16_SWAP_ON_BE((wxUint16)hy);
        }
        icondirentry.dwBytesInRes = wxUINT32_SWAP_ON_BE(Size);
        icondirentry.dwImageOffset = wxUINT32_SWAP_ON_BE(offset);

        // increase size to allow for the data written:
        offset += Size;

        // write to stream:
        stream.Write(&icondirentry.bWidth, sizeof(icondirentry.bWidth));
        stream.Write(&icondirentry.bHeight, sizeof(icondirentry.bHeight));
        stream.Write(&icondirentry.bColorCount, sizeof(icondirentry.bColorCount));
        stream.Write(&icondirentry.bReserved, sizeof(icondirentry.bReserved));
        stream.Write(&icondirentry.wPlanes, sizeof(icondirentry.wPlanes));
        stream.Write(&icondirentry.wBitCount, sizeof(icondirentry.wBitCount));
        stream.Write(&icondirentry.dwBytesInRes, sizeof(icondirentry.dwBytesInRes));
        stream.Write(&icondirentry.dwImageOffset, sizeof(icondirentry.dwImageOffset));
        if ( !stream.IsOk() )
        {
            if ( verbose )
                wxLogError(_("ICO: Error writing the image file!"));
            return false;
        }

        // actually save it:
        IsMask = false;
        bResult = SaveDib(image, stream, verbose, IsBmp, IsMask);
        if ( !bResult )
        {
            if ( verbose )
                wxLogError(_("ICO: Error writing the image file!"));
            return false;
        }
        IsMask = true;

        bResult = SaveDib(&mask, stream, verbose, IsBmp, IsMask);
        if ( !bResult )
        {
            if ( verbose )
                wxLogError(_("ICO: Error writing the image file!"));
            return false;
        }

    } // end of for loop

    return true;
}

bool wxICOHandler::LoadFile(wxImage *image, wxInputStream& stream,
                            bool verbose, int index)
{
    stream.SeekI(0);
    return DoLoadFile(image, stream, verbose, index);
}

bool wxICOHandler::DoLoadFile(wxImage *image, wxInputStream& stream,
                            bool WXUNUSED(verbose), int index)
{
    bool bResult wxDUMMY_INITIALIZE(false);
    bool IsBmp = false;

    ICONDIR IconDir;

    wxFileOffset iPos = stream.TellI();
    stream.Read(&IconDir, sizeof(IconDir));
    wxUint16 nIcons = wxUINT16_SWAP_ON_BE(IconDir.idCount);
    // nType is 1 for Icons, 2 for Cursors:
    wxUint16 nType = wxUINT16_SWAP_ON_BE(IconDir.idType);

    // loop round the icons and choose the best one:
    ICONDIRENTRY *pIconDirEntry = new ICONDIRENTRY[nIcons];
    ICONDIRENTRY *pCurrentEntry = pIconDirEntry;
    int wMax = 0;
    int colmax = 0;
    int iSel = wxNOT_FOUND;

    for (int i = 0; i < nIcons; i++ )
    {
        stream.Read(pCurrentEntry, sizeof(ICONDIRENTRY));
        // bHeight and bColorCount are wxUint8
        if ( pCurrentEntry->bWidth >= wMax )
        {
            // see if we have more colors, ==0 indicates > 8bpp:
            if ( pCurrentEntry->bColorCount == 0 )
                pCurrentEntry->bColorCount = 255;
            if ( pCurrentEntry->bColorCount >= colmax )
            {
                iSel = i;
                wMax = pCurrentEntry->bWidth;
                colmax = pCurrentEntry->bColorCount;
            }
        }
        pCurrentEntry++;
    }

    if ( index != -1 )
    {
        // VS: Note that we *have* to run the loop above even if index != -1, because
        //     it reads ICONDIRENTRies.
        iSel = index;
    }

    if ( iSel == wxNOT_FOUND || iSel < 0 || iSel >= nIcons )
    {
        wxLogError(_("ICO: Invalid icon index."));
        bResult = false;
    }
    else
    {
        // seek to selected icon:
        pCurrentEntry = pIconDirEntry + iSel;
        stream.SeekI(iPos + wxUINT32_SWAP_ON_BE(pCurrentEntry->dwImageOffset), wxFromStart);
        bResult = LoadDib(image, stream, true, IsBmp);
        bool bIsCursorType = (this->GetType() == wxBITMAP_TYPE_CUR) || (this->GetType() == wxBITMAP_TYPE_ANI);
        if ( bResult && bIsCursorType && nType == 2 )
        {
            // it is a cursor, so let's set the hotspot:
            image->SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, wxUINT16_SWAP_ON_BE(pCurrentEntry->wPlanes));
            image->SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, wxUINT16_SWAP_ON_BE(pCurrentEntry->wBitCount));
        }
    }
    delete[] pIconDirEntry;
    return bResult;
}

int wxICOHandler::GetImageCount(wxInputStream& stream)
{
    ICONDIR IconDir;
    wxFileOffset iPos = stream.TellI();
    stream.SeekI(0);
    stream.Read(&IconDir, sizeof(IconDir));
    wxUint16 nIcons = wxUINT16_SWAP_ON_BE(IconDir.idCount);
    stream.SeekI(iPos);
    return (int)nIcons;
}

bool wxICOHandler::DoCanRead(wxInputStream& stream)
{
    stream.SeekI(0);
    unsigned char hdr[4];
    if ( !stream.Read(hdr, WXSIZEOF(hdr)) )
        return false;

    // hdr[2] is one for an icon and two for a cursor
    return hdr[0] == '\0' && hdr[1] == '\0' && hdr[2] == '\1' && hdr[3] == '\0';
}

#endif // wxUSE_STREAMS


//-----------------------------------------------------------------------------
// wxCURHandler
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxCURHandler, wxICOHandler)

#if wxUSE_STREAMS

bool wxCURHandler::DoCanRead(wxInputStream& stream)
{
    stream.SeekI(0);
    unsigned char hdr[4];
    if ( !stream.Read(hdr, WXSIZEOF(hdr)) )
        return false;

    // hdr[2] is one for an icon and two for a cursor
    return hdr[0] == '\0' && hdr[1] == '\0' && hdr[2] == '\2' && hdr[3] == '\0';
}

#endif // wxUSE_STREAMS

//-----------------------------------------------------------------------------
// wxANIHandler
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxANIHandler, wxCURHandler)

#if wxUSE_STREAMS

bool wxANIHandler::LoadFile(wxImage *image, wxInputStream& stream,
                            bool WXUNUSED(verbose), int index)
{
    wxANIDecoder decoder;
    if (!decoder.Load(stream))
        return false;

    return decoder.ConvertToImage(index != -1 ? (size_t)index : 0, image);
}

bool wxANIHandler::DoCanRead(wxInputStream& stream)
{
    wxANIDecoder decod;
    return decod.CanRead(stream);
}

int wxANIHandler::GetImageCount(wxInputStream& stream)
{
    wxANIDecoder decoder;
    if (!decoder.Load(stream))
        return wxNOT_FOUND;

    return decoder.GetFrameCount();
}

#endif // wxUSE_STREAMS

#endif // wxUSE_ICO_CUR

#endif // wxUSE_IMAGE
