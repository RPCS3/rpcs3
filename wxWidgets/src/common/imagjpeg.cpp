/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/imagjpeg.cpp
// Purpose:     wxImage JPEG handler
// Author:      Vaclav Slavik
// RCS-ID:      $Id: imagjpeg.cpp 43781 2006-12-03 21:59:47Z MW $
// Copyright:   (c) Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_IMAGE && wxUSE_LIBJPEG

#include "wx/imagjpeg.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/app.h"
    #include "wx/intl.h"
    #include "wx/bitmap.h"
    #include "wx/module.h"
#endif

// A hack based on one from tif_jpeg.c to overcome the problem on Windows
// of rpcndr.h defining boolean with a different type to the jpeg headers.
// 
// This hack is only necessary for an external jpeg library, the builtin one
// usually used on Windows doesn't use the type boolean, so always works.
//
#ifdef wxHACK_BOOLEAN
    #define HAVE_BOOLEAN
    #define boolean wxHACK_BOOLEAN
#endif

extern "C"
{
    #if defined(__WXMSW__)
        #define XMD_H
    #endif
    #include "jpeglib.h"
}

#ifndef HAVE_WXJPEG_BOOLEAN
typedef boolean wxjpeg_boolean;
#endif

#include "wx/filefn.h"
#include "wx/wfstream.h"

// For memcpy
#include <string.h>
// For JPEG library error handling
#include <setjmp.h>

#ifdef __SALFORDC__
#undef FAR
#endif

// ----------------------------------------------------------------------------
// types
// ----------------------------------------------------------------------------

// the standard definition of METHODDEF(type) from jmorecfg.h is "static type"
// which means that we can't declare the method functions as extern "C" - the
// compiler (rightfully) complains about the multiple storage classes in
// declaration
//
// so we only add extern "C" when using our own, modified, jmorecfg.h - and use
// whatever we have in the system headers if this is what we use hoping that it
// should be ok (can't do anything else)
#ifdef JPEG_METHOD_LINKAGE
    #define CPP_METHODDEF(type) extern "C" METHODDEF(type)
#else // not using our jmorecfg.h header
    #define CPP_METHODDEF(type) METHODDEF(type)
#endif

//-----------------------------------------------------------------------------
// wxJPEGHandler
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxJPEGHandler,wxImageHandler)

#if wxUSE_STREAMS

//------------- JPEG Data Source Manager

#define JPEG_IO_BUFFER_SIZE   2048

typedef struct {
    struct jpeg_source_mgr pub;   /* public fields */

    JOCTET* buffer;               /* start of buffer */
    wxInputStream *stream;
} wx_source_mgr;

typedef wx_source_mgr * wx_src_ptr;

CPP_METHODDEF(void) wx_init_source ( j_decompress_ptr WXUNUSED(cinfo) )
{
}

CPP_METHODDEF(wxjpeg_boolean) wx_fill_input_buffer ( j_decompress_ptr cinfo )
{
    wx_src_ptr src = (wx_src_ptr) cinfo->src;

    src->pub.next_input_byte = src->buffer;
    src->pub.bytes_in_buffer = src->stream->Read(src->buffer, JPEG_IO_BUFFER_SIZE).LastRead();

    if (src->pub.bytes_in_buffer == 0) // check for end-of-stream
    {
        // Insert a fake EOI marker
        src->buffer[0] = 0xFF;
        src->buffer[1] = JPEG_EOI;
        src->pub.bytes_in_buffer = 2;
    }
    return TRUE;
}

CPP_METHODDEF(void) wx_skip_input_data ( j_decompress_ptr cinfo, long num_bytes )
{
    if (num_bytes > 0)
    {
        wx_src_ptr src = (wx_src_ptr) cinfo->src;

        while (num_bytes > (long)src->pub.bytes_in_buffer)
        {
            num_bytes -= (long) src->pub.bytes_in_buffer;
            src->pub.fill_input_buffer(cinfo);
        }
        src->pub.next_input_byte += (size_t) num_bytes;
        src->pub.bytes_in_buffer -= (size_t) num_bytes;
    }
}

CPP_METHODDEF(void) wx_term_source ( j_decompress_ptr cinfo )
{
    wx_src_ptr src = (wx_src_ptr) cinfo->src;

    if (src->pub.bytes_in_buffer > 0)
        src->stream->SeekI(-(long)src->pub.bytes_in_buffer, wxFromCurrent);
    delete[] src->buffer;
}


// JPEG error manager:

struct wx_error_mgr {
  struct jpeg_error_mgr pub;    /* "public" fields */

  jmp_buf setjmp_buffer;    /* for return to caller */
};

typedef struct wx_error_mgr * wx_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

CPP_METHODDEF(void) wx_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a wx_error_mgr struct, so coerce pointer */
  wx_error_ptr myerr = (wx_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

/*
 * This will replace the standard output_message method when the user
 * wants us to be silent (verbose==false). We must have such method instead of
 * simply using NULL for cinfo->err->output_message because it's called
 * unconditionally from within libjpeg when there's "garbage input".
 */
CPP_METHODDEF(void) wx_ignore_message (j_common_ptr WXUNUSED(cinfo))
{
}

void wx_jpeg_io_src( j_decompress_ptr cinfo, wxInputStream& infile )
{
    wx_src_ptr src;

    if (cinfo->src == NULL) {    /* first time for this JPEG object? */
        cinfo->src = (struct jpeg_source_mgr *)
            (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
            sizeof(wx_source_mgr));
    }
    src = (wx_src_ptr) cinfo->src;
    src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
    src->buffer = new JOCTET[JPEG_IO_BUFFER_SIZE];
    src->pub.next_input_byte = NULL; /* until buffer loaded */
    src->stream = &infile;

    src->pub.init_source = wx_init_source;
    src->pub.fill_input_buffer = wx_fill_input_buffer;
    src->pub.skip_input_data = wx_skip_input_data;
    src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->pub.term_source = wx_term_source;
}

static inline void wx_cmyk_to_rgb(unsigned char* rgb, const unsigned char* cmyk)
{
    register int k = 255 - cmyk[3];
    register int k2 = cmyk[3];
    register int c;

    c = k + k2 * (255 - cmyk[0]) / 255;
    rgb[0] = (unsigned char)((c > 255) ? 0 : (255 - c));

    c = k + k2 * (255 - cmyk[1]) / 255;
    rgb[1] = (unsigned char)((c > 255) ? 0 : (255 - c));

    c = k + k2 * (255 - cmyk[2]) / 255;
    rgb[2] = (unsigned char)((c > 255) ? 0 : (255 - c));
}

// temporarily disable the warning C4611 (interaction between '_setjmp' and
// C++ object destruction is non-portable) - I don't see any dtors here
#ifdef __VISUALC__
    #pragma warning(disable:4611)
#endif /* VC++ */

bool wxJPEGHandler::LoadFile( wxImage *image, wxInputStream& stream, bool verbose, int WXUNUSED(index) )
{
    struct jpeg_decompress_struct cinfo;
    struct wx_error_mgr jerr;
    unsigned char *ptr;

    image->Destroy();
    cinfo.err = jpeg_std_error( &jerr.pub );
    jerr.pub.error_exit = wx_error_exit;

    if (!verbose)
        cinfo.err->output_message = wx_ignore_message;

    /* Establish the setjmp return context for wx_error_exit to use. */
    if (setjmp(jerr.setjmp_buffer)) {
      /* If we get here, the JPEG code has signaled an error.
       * We need to clean up the JPEG object, close the input file, and return.
       */
      if (verbose)
        wxLogError(_("JPEG: Couldn't load - file is probably corrupted."));
      (cinfo.src->term_source)(&cinfo);
      jpeg_destroy_decompress(&cinfo);
      if (image->Ok()) image->Destroy();
      return false;
    }

    jpeg_create_decompress( &cinfo );
    wx_jpeg_io_src( &cinfo, stream );
    jpeg_read_header( &cinfo, TRUE );

    int bytesPerPixel;
    if ((cinfo.out_color_space == JCS_CMYK) || (cinfo.out_color_space == JCS_YCCK))
    {
        cinfo.out_color_space = JCS_CMYK;
        bytesPerPixel = 4;
    }
    else // all the rest is treated as RGB
    {
        cinfo.out_color_space = JCS_RGB;
        bytesPerPixel = 3;
    }

    jpeg_start_decompress( &cinfo );

    image->Create( cinfo.image_width, cinfo.image_height );
    if (!image->Ok()) {
        jpeg_finish_decompress( &cinfo );
        jpeg_destroy_decompress( &cinfo );
        return false;
    }
    image->SetMask( false );
    ptr = image->GetData();

    unsigned stride = cinfo.output_width * bytesPerPixel;
    JSAMPARRAY tempbuf = (*cinfo.mem->alloc_sarray)
                            ((j_common_ptr) &cinfo, JPOOL_IMAGE, stride, 1 );

    while ( cinfo.output_scanline < cinfo.output_height )
    {
        jpeg_read_scanlines( &cinfo, tempbuf, 1 );
        if (cinfo.out_color_space == JCS_RGB)
        {
            memcpy( ptr, tempbuf[0], stride );
            ptr += stride;
        }
        else // CMYK
        {
            const unsigned char* inptr = (const unsigned char*) tempbuf[0];
            for (size_t i = 0; i < cinfo.output_width; i++)
            {
                wx_cmyk_to_rgb(ptr, inptr);
                ptr += 3;
                inptr += 4;
            }
        }
    }

    jpeg_finish_decompress( &cinfo );
    jpeg_destroy_decompress( &cinfo );
    return true;
}

typedef struct {
    struct jpeg_destination_mgr pub;

    wxOutputStream *stream;
    JOCTET * buffer;
} wx_destination_mgr;

typedef wx_destination_mgr * wx_dest_ptr;

#define OUTPUT_BUF_SIZE  4096    /* choose an efficiently fwrite'able size */

CPP_METHODDEF(void) wx_init_destination (j_compress_ptr cinfo)
{
    wx_dest_ptr dest = (wx_dest_ptr) cinfo->dest;

    /* Allocate the output buffer --- it will be released when done with image */
    dest->buffer = (JOCTET *)
        (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
        OUTPUT_BUF_SIZE * sizeof(JOCTET));
    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

CPP_METHODDEF(wxjpeg_boolean) wx_empty_output_buffer (j_compress_ptr cinfo)
{
    wx_dest_ptr dest = (wx_dest_ptr) cinfo->dest;

    dest->stream->Write(dest->buffer, OUTPUT_BUF_SIZE);
    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
    return TRUE;
}

CPP_METHODDEF(void) wx_term_destination (j_compress_ptr cinfo)
{
    wx_dest_ptr dest = (wx_dest_ptr) cinfo->dest;
    size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;
    /* Write any data remaining in the buffer */
    if (datacount > 0)
        dest->stream->Write(dest->buffer, datacount);
}

GLOBAL(void) wx_jpeg_io_dest (j_compress_ptr cinfo, wxOutputStream& outfile)
{
    wx_dest_ptr dest;

    if (cinfo->dest == NULL) {    /* first time for this JPEG object? */
        cinfo->dest = (struct jpeg_destination_mgr *)
            (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
            sizeof(wx_destination_mgr));
    }

    dest = (wx_dest_ptr) cinfo->dest;
    dest->pub.init_destination = wx_init_destination;
    dest->pub.empty_output_buffer = wx_empty_output_buffer;
    dest->pub.term_destination = wx_term_destination;
    dest->stream = &outfile;
}

bool wxJPEGHandler::SaveFile( wxImage *image, wxOutputStream& stream, bool verbose )
{
    struct jpeg_compress_struct cinfo;
    struct wx_error_mgr jerr;
    JSAMPROW row_pointer[1];    /* pointer to JSAMPLE row[s] */
    JSAMPLE *image_buffer;
    int stride;                /* physical row width in image buffer */

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = wx_error_exit;

    if (!verbose)
        cinfo.err->output_message = wx_ignore_message;

    /* Establish the setjmp return context for wx_error_exit to use. */
    if (setjmp(jerr.setjmp_buffer))
    {
        /* If we get here, the JPEG code has signaled an error.
         * We need to clean up the JPEG object, close the input file, and return.
         */
         if (verbose)
            wxLogError(_("JPEG: Couldn't save image."));
         jpeg_destroy_compress(&cinfo);
         return false;
    }

    jpeg_create_compress(&cinfo);
    wx_jpeg_io_dest(&cinfo, stream);

    cinfo.image_width = image->GetWidth();
    cinfo.image_height = image->GetHeight();
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);

    // TODO: 3rd parameter is force_baseline, what value should this be?
    // Code says: "If force_baseline is TRUE, the computed quantization table entries
    // are limited to 1..255 for JPEG baseline compatibility."
    // 'Quality' is a number between 0 (terrible) and 100 (very good).
    // The default (in jcparam.c, jpeg_set_defaults) is 75,
    // and force_baseline is TRUE.
    if (image->HasOption(wxIMAGE_OPTION_QUALITY))
        jpeg_set_quality(&cinfo, image->GetOptionInt(wxIMAGE_OPTION_QUALITY), TRUE);

    // set the resolution fields in the output file
    UINT16 resX,
           resY;
    if ( image->HasOption(wxIMAGE_OPTION_RESOLUTIONX) &&
         image->HasOption(wxIMAGE_OPTION_RESOLUTIONY) )
    {
        resX = (UINT16)image->GetOptionInt(wxIMAGE_OPTION_RESOLUTIONX);
        resY = (UINT16)image->GetOptionInt(wxIMAGE_OPTION_RESOLUTIONY);
    }
    else if ( image->HasOption(wxIMAGE_OPTION_RESOLUTION) )
    {
        resX =
        resY = (UINT16)image->GetOptionInt(wxIMAGE_OPTION_RESOLUTION);
    }
    else
    {
        resX =
        resY = 0;
    }

    if ( resX && resY )
    {
        cinfo.X_density = resX;
        cinfo.Y_density = resY;
    }

    // sets the resolution unit field in the output file
    // wxIMAGE_RESOLUTION_INCHES for inches
    // wxIMAGE_RESOLUTION_CM for centimeters
    if ( image->HasOption(wxIMAGE_OPTION_RESOLUTIONUNIT) )
    {
        cinfo.density_unit = (UINT8)image->GetOptionInt(wxIMAGE_OPTION_RESOLUTIONUNIT);
    }

    jpeg_start_compress(&cinfo, TRUE);

    stride = cinfo.image_width * 3;    /* JSAMPLEs per row in image_buffer */
    image_buffer = image->GetData();
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &image_buffer[cinfo.next_scanline * stride];
        jpeg_write_scanlines( &cinfo, row_pointer, 1 );
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    return true;
}

#ifdef __VISUALC__
    #pragma warning(default:4611)
#endif /* VC++ */

bool wxJPEGHandler::DoCanRead( wxInputStream& stream )
{
    unsigned char hdr[2];

    if ( !stream.Read(hdr, WXSIZEOF(hdr)) )
        return false;

    return hdr[0] == 0xFF && hdr[1] == 0xD8;
}

#endif   // wxUSE_STREAMS

#endif   // wxUSE_LIBJPEG
