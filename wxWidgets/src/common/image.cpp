/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/image.cpp
// Purpose:     wxImage
// Author:      Robert Roebling
// RCS-ID:      $Id: image.cpp 59197 2009-02-28 15:44:53Z VZ $
// Copyright:   (c) Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_IMAGE

#include "wx/image.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/hash.h"
    #include "wx/utils.h"
    #include "wx/math.h"
    #include "wx/module.h"
    #include "wx/palette.h"
    #include "wx/intl.h"
#endif

#include "wx/filefn.h"
#include "wx/wfstream.h"
#include "wx/xpmdecod.h"

// For memcpy
#include <string.h>

// make the code compile with either wxFile*Stream or wxFFile*Stream:
#define HAS_FILE_STREAMS (wxUSE_STREAMS && (wxUSE_FILE || wxUSE_FFILE))

#if HAS_FILE_STREAMS
    #if wxUSE_FFILE
        typedef wxFFileInputStream wxImageFileInputStream;
        typedef wxFFileOutputStream wxImageFileOutputStream;
    #elif wxUSE_FILE
        typedef wxFileInputStream wxImageFileInputStream;
        typedef wxFileOutputStream wxImageFileOutputStream;
    #endif // wxUSE_FILE/wxUSE_FFILE
#endif // HAS_FILE_STREAMS

#if wxUSE_VARIANT
IMPLEMENT_VARIANT_OBJECT_EXPORTED_SHALLOWCMP(wxImage,WXDLLEXPORT)
#endif

//-----------------------------------------------------------------------------
// wxImage
//-----------------------------------------------------------------------------

class wxImageRefData: public wxObjectRefData
{
public:
    wxImageRefData();
    virtual ~wxImageRefData();

    int             m_width;
    int             m_height;
    unsigned char  *m_data;

    bool            m_hasMask;
    unsigned char   m_maskRed,m_maskGreen,m_maskBlue;

    // alpha channel data, may be NULL for the formats without alpha support
    unsigned char  *m_alpha;

    bool            m_ok;

    // if true, m_data is pointer to static data and shouldn't be freed
    bool            m_static;

    // same as m_static but for m_alpha
    bool            m_staticAlpha;

#if wxUSE_PALETTE
    wxPalette       m_palette;
#endif // wxUSE_PALETTE

    wxArrayString   m_optionNames;
    wxArrayString   m_optionValues;

    DECLARE_NO_COPY_CLASS(wxImageRefData)
};

wxImageRefData::wxImageRefData()
{
    m_width = 0;
    m_height = 0;
    m_data =
    m_alpha = (unsigned char *) NULL;

    m_maskRed = 0;
    m_maskGreen = 0;
    m_maskBlue = 0;
    m_hasMask = false;

    m_ok = false;
    m_static =
    m_staticAlpha = false;
}

wxImageRefData::~wxImageRefData()
{
    if ( !m_static )
        free( m_data );
    if ( !m_staticAlpha )
        free( m_alpha );
}

wxList wxImage::sm_handlers;

wxImage wxNullImage;

//-----------------------------------------------------------------------------

#define M_IMGDATA wx_static_cast(wxImageRefData*, m_refData)

IMPLEMENT_DYNAMIC_CLASS(wxImage, wxObject)

wxImage::wxImage( int width, int height, bool clear )
{
    Create( width, height, clear );
}

wxImage::wxImage( int width, int height, unsigned char* data, bool static_data )
{
    Create( width, height, data, static_data );
}

wxImage::wxImage( int width, int height, unsigned char* data, unsigned char* alpha, bool static_data )
{
    Create( width, height, data, alpha, static_data );
}

wxImage::wxImage( const wxString& name, long type, int index )
{
    LoadFile( name, type, index );
}

wxImage::wxImage( const wxString& name, const wxString& mimetype, int index )
{
    LoadFile( name, mimetype, index );
}

#if wxUSE_STREAMS
wxImage::wxImage( wxInputStream& stream, long type, int index )
{
    LoadFile( stream, type, index );
}

wxImage::wxImage( wxInputStream& stream, const wxString& mimetype, int index )
{
    LoadFile( stream, mimetype, index );
}
#endif // wxUSE_STREAMS

wxImage::wxImage(const char* const* xpmData)
{
    Create(xpmData);
}

bool wxImage::Create(const char* const* xpmData)
{
#if wxUSE_XPM
    UnRef();

    wxXPMDecoder decoder;
    (*this) = decoder.ReadData(xpmData);
    return Ok();
#else
    return false;
#endif
}

bool wxImage::Create( int width, int height, bool clear )
{
    UnRef();

    m_refData = new wxImageRefData();

    M_IMGDATA->m_data = (unsigned char *) malloc( width*height*3 );
    if (!M_IMGDATA->m_data)
    {
        UnRef();
        return false;
    }

    if (clear)
        memset(M_IMGDATA->m_data, 0, width*height*3);

    M_IMGDATA->m_width = width;
    M_IMGDATA->m_height = height;
    M_IMGDATA->m_ok = true;

    return true;
}

bool wxImage::Create( int width, int height, unsigned char* data, bool static_data )
{
    UnRef();

    wxCHECK_MSG( data, false, _T("NULL data in wxImage::Create") );

    m_refData = new wxImageRefData();

    M_IMGDATA->m_data = data;
    M_IMGDATA->m_width = width;
    M_IMGDATA->m_height = height;
    M_IMGDATA->m_ok = true;
    M_IMGDATA->m_static = static_data;

    return true;
}

bool wxImage::Create( int width, int height, unsigned char* data, unsigned char* alpha, bool static_data )
{
    UnRef();

    wxCHECK_MSG( data, false, _T("NULL data in wxImage::Create") );

    m_refData = new wxImageRefData();

    M_IMGDATA->m_data = data;
    M_IMGDATA->m_alpha = alpha;
    M_IMGDATA->m_width = width;
    M_IMGDATA->m_height = height;
    M_IMGDATA->m_ok = true;
    M_IMGDATA->m_static = static_data;
    M_IMGDATA->m_staticAlpha = static_data;

    return true;
}

void wxImage::Destroy()
{
    UnRef();
}

wxObjectRefData* wxImage::CreateRefData() const
{
    return new wxImageRefData;
}

wxObjectRefData* wxImage::CloneRefData(const wxObjectRefData* that) const
{
    const wxImageRefData* refData = wx_static_cast(const wxImageRefData*, that);
    wxCHECK_MSG(refData->m_ok, NULL, wxT("invalid image") );

    wxImageRefData* refData_new = new wxImageRefData;
    refData_new->m_width = refData->m_width;
    refData_new->m_height = refData->m_height;
    refData_new->m_maskRed = refData->m_maskRed;
    refData_new->m_maskGreen = refData->m_maskGreen;
    refData_new->m_maskBlue = refData->m_maskBlue;
    refData_new->m_hasMask = refData->m_hasMask;
    refData_new->m_ok = true;
    unsigned size = unsigned(refData->m_width) * unsigned(refData->m_height);
    if (refData->m_alpha != NULL)
    {
        refData_new->m_alpha = (unsigned char*)malloc(size);
        memcpy(refData_new->m_alpha, refData->m_alpha, size);
    }
    size *= 3;
    refData_new->m_data = (unsigned char*)malloc(size);
    memcpy(refData_new->m_data, refData->m_data, size);
#if wxUSE_PALETTE
    refData_new->m_palette = refData->m_palette;
#endif
    refData_new->m_optionNames = refData->m_optionNames;
    refData_new->m_optionValues = refData->m_optionValues;
    return refData_new;
}

wxImage wxImage::Copy() const
{
    wxImage image;

    wxCHECK_MSG( Ok(), image, wxT("invalid image") );

    image.m_refData = CloneRefData(m_refData);

    return image;
}

wxImage wxImage::ShrinkBy( int xFactor , int yFactor ) const
{
    if( xFactor == 1 && yFactor == 1 )
        return *this;

    wxImage image;

    wxCHECK_MSG( Ok(), image, wxT("invalid image") );

    // can't scale to/from 0 size
    wxCHECK_MSG( (xFactor > 0) && (yFactor > 0), image,
                 wxT("invalid new image size") );

    long old_height = M_IMGDATA->m_height,
         old_width  = M_IMGDATA->m_width;

    wxCHECK_MSG( (old_height > 0) && (old_width > 0), image,
                 wxT("invalid old image size") );

    long width = old_width / xFactor ;
    long height = old_height / yFactor ;

    image.Create( width, height, false );

    char unsigned *data = image.GetData();

    wxCHECK_MSG( data, image, wxT("unable to create image") );

    bool hasMask = false ;
    unsigned char maskRed = 0;
    unsigned char maskGreen = 0;
    unsigned char maskBlue =0 ;

    unsigned char *source_data = M_IMGDATA->m_data;
    unsigned char *target_data = data;
    unsigned char *source_alpha = 0 ;
    unsigned char *target_alpha = 0 ;
    if (M_IMGDATA->m_hasMask)
    {
        hasMask = true ;
        maskRed = M_IMGDATA->m_maskRed;
        maskGreen = M_IMGDATA->m_maskGreen;
        maskBlue =M_IMGDATA->m_maskBlue ;

        image.SetMaskColour( M_IMGDATA->m_maskRed,
                             M_IMGDATA->m_maskGreen,
                             M_IMGDATA->m_maskBlue );
    }
    else
    {
        source_alpha = M_IMGDATA->m_alpha ;
        if ( source_alpha )
        {
            image.SetAlpha() ;
            target_alpha = image.GetAlpha() ;
        }
    }

    for (long y = 0; y < height; y++)
    {
        for (long x = 0; x < width; x++)
        {
            unsigned long avgRed = 0 ;
            unsigned long avgGreen = 0;
            unsigned long avgBlue = 0;
            unsigned long avgAlpha = 0 ;
            unsigned long counter = 0 ;
            // determine average
            for ( int y1 = 0 ; y1 < yFactor ; ++y1 )
            {
                long y_offset = (y * yFactor + y1) * old_width;
                for ( int x1 = 0 ; x1 < xFactor ; ++x1 )
                {
                    unsigned char *pixel = source_data + 3 * ( y_offset + x * xFactor + x1 ) ;
                    unsigned char red = pixel[0] ;
                    unsigned char green = pixel[1] ;
                    unsigned char blue = pixel[2] ;
                    unsigned char alpha = 255  ;
                    if ( source_alpha )
                        alpha = *(source_alpha + y_offset + x * xFactor + x1) ;
                    if ( !hasMask || red != maskRed || green != maskGreen || blue != maskBlue )
                    {
                        if ( alpha > 0 )
                        {
                            avgRed += red ;
                            avgGreen += green ;
                            avgBlue += blue ;
                        }
                        avgAlpha += alpha ;
                        counter++ ;
                    }
                }
            }
            if ( counter == 0 )
            {
                *(target_data++) = M_IMGDATA->m_maskRed ;
                *(target_data++) = M_IMGDATA->m_maskGreen ;
                *(target_data++) = M_IMGDATA->m_maskBlue ;
            }
            else
            {
                if ( source_alpha )
                    *(target_alpha++) = (unsigned char)(avgAlpha / counter ) ;
                *(target_data++) = (unsigned char)(avgRed / counter);
                *(target_data++) = (unsigned char)(avgGreen / counter);
                *(target_data++) = (unsigned char)(avgBlue / counter);
            }
        }
    }

    // In case this is a cursor, make sure the hotspot is scaled accordingly:
    if ( HasOption(wxIMAGE_OPTION_CUR_HOTSPOT_X) )
        image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X,
                (GetOptionInt(wxIMAGE_OPTION_CUR_HOTSPOT_X))/xFactor);
    if ( HasOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y) )
        image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y,
                (GetOptionInt(wxIMAGE_OPTION_CUR_HOTSPOT_Y))/yFactor);

    return image;
}

wxImage wxImage::Scale( int width, int height, int quality ) const
{
    wxImage image;

    wxCHECK_MSG( Ok(), image, wxT("invalid image") );

    // can't scale to/from 0 size
    wxCHECK_MSG( (width > 0) && (height > 0), image,
                 wxT("invalid new image size") );

    long old_height = M_IMGDATA->m_height,
         old_width  = M_IMGDATA->m_width;
    wxCHECK_MSG( (old_height > 0) && (old_width > 0), image,
                 wxT("invalid old image size") );

    // If the image's new width and height are the same as the original, no
    // need to waste time or CPU cycles
    if ( old_width == width && old_height == height )
        return *this;

    // Scale the image (...or more appropriately, resample the image) using
    // either the high-quality or normal method as specified
    if ( quality == wxIMAGE_QUALITY_HIGH )
    {
        // We need to check whether we are downsampling or upsampling the image
        if ( width < old_width && height < old_height )
        {
            // Downsample the image using the box averaging method for best results
            image = ResampleBox(width, height);
        }
        else
        {
            // For upsampling or other random/wierd image dimensions we'll use
            // a bicubic b-spline scaling method
            image = ResampleBicubic(width, height);
        }
    }
    else    // Default scaling method == simple pixel replication
    {
        if ( old_width % width == 0 && old_width >= width &&
            old_height % height == 0 && old_height >= height )
        {
            return ShrinkBy( old_width / width , old_height / height ) ;
        }
        image.Create( width, height, false );

        unsigned char *data = image.GetData();

        wxCHECK_MSG( data, image, wxT("unable to create image") );

        unsigned char *source_data = M_IMGDATA->m_data;
        unsigned char *target_data = data;
        unsigned char *source_alpha = 0 ;
        unsigned char *target_alpha = 0 ;

        if ( !M_IMGDATA->m_hasMask )
        {
            source_alpha = M_IMGDATA->m_alpha ;
            if ( source_alpha )
            {
                image.SetAlpha() ;
                target_alpha = image.GetAlpha() ;
            }
        }

        long x_delta = (old_width<<16) / width;
        long y_delta = (old_height<<16) / height;

        unsigned char* dest_pixel = target_data;

        long y = 0;
        for ( long j = 0; j < height; j++ )
        {
            unsigned char* src_line = &source_data[(y>>16)*old_width*3];
            unsigned char* src_alpha_line = source_alpha ? &source_alpha[(y>>16)*old_width] : 0 ;

            long x = 0;
            for ( long i = 0; i < width; i++ )
            {
                unsigned char* src_pixel = &src_line[(x>>16)*3];
                unsigned char* src_alpha_pixel = source_alpha ? &src_alpha_line[(x>>16)] : 0 ;
                dest_pixel[0] = src_pixel[0];
                dest_pixel[1] = src_pixel[1];
                dest_pixel[2] = src_pixel[2];
                dest_pixel += 3;
                if ( source_alpha )
                    *(target_alpha++) = *src_alpha_pixel ;
                x += x_delta;
            }

            y += y_delta;
        }
    }

    // If the original image has a mask, apply the mask to the new image
    if (M_IMGDATA->m_hasMask)
    {
        image.SetMaskColour( M_IMGDATA->m_maskRed,
                            M_IMGDATA->m_maskGreen,
                            M_IMGDATA->m_maskBlue );
    }

    // In case this is a cursor, make sure the hotspot is scaled accordingly:
    if ( HasOption(wxIMAGE_OPTION_CUR_HOTSPOT_X) )
        image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X,
                (GetOptionInt(wxIMAGE_OPTION_CUR_HOTSPOT_X)*width)/old_width);
    if ( HasOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y) )
        image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y,
                (GetOptionInt(wxIMAGE_OPTION_CUR_HOTSPOT_Y)*height)/old_height);

    return image;
}

wxImage wxImage::ResampleBox(int width, int height) const
{
    // This function implements a simple pre-blur/box averaging method for
    // downsampling that gives reasonably smooth results To scale the image
    // down we will need to gather a grid of pixels of the size of the scale
    // factor in each direction and then do an averaging of the pixels.

    wxImage ret_image(width, height, false);

    const double scale_factor_x = double(M_IMGDATA->m_width) / width;
    const double scale_factor_y = double(M_IMGDATA->m_height) / height;

    const int scale_factor_x_2 = (int)(scale_factor_x / 2);
    const int scale_factor_y_2 = (int)(scale_factor_y / 2);

    unsigned char* src_data = M_IMGDATA->m_data;
    unsigned char* src_alpha = M_IMGDATA->m_alpha;
    unsigned char* dst_data = ret_image.GetData();
    unsigned char* dst_alpha = NULL;

    if ( src_alpha )
    {
        ret_image.SetAlpha();
        dst_alpha = ret_image.GetAlpha();
    }

    int averaged_pixels, src_pixel_index;
    double sum_r, sum_g, sum_b, sum_a;

    for ( int y = 0; y < height; y++ )         // Destination image - Y direction
    {
        // Source pixel in the Y direction
        int src_y = (int)(y * scale_factor_y);

        for ( int x = 0; x < width; x++ )      // Destination image - X direction
        {
            // Source pixel in the X direction
            int src_x = (int)(x * scale_factor_x);

            // Box of pixels to average
            averaged_pixels = 0;
            sum_r = sum_g = sum_b = sum_a = 0.0;

            for ( int j = int(src_y - scale_factor_y/2.0 + 1);
                  j <= int(src_y + scale_factor_y_2);
                  j++ )
            {
                // We don't care to average pixels that don't exist (edges)
                if ( j < 0 || j > M_IMGDATA->m_height - 1 )
                    continue;

                for ( int i = int(src_x - scale_factor_x/2.0 + 1);
                      i <= src_x + scale_factor_x_2;
                      i++ )
                {
                    // Don't average edge pixels
                    if ( i < 0 || i > M_IMGDATA->m_width - 1 )
                        continue;

                    // Calculate the actual index in our source pixels
                    src_pixel_index = j * M_IMGDATA->m_width + i;

                    sum_r += src_data[src_pixel_index * 3 + 0];
                    sum_g += src_data[src_pixel_index * 3 + 1];
                    sum_b += src_data[src_pixel_index * 3 + 2];
                    if ( src_alpha )
                        sum_a += src_alpha[src_pixel_index];

                    averaged_pixels++;
                }
            }

            // Calculate the average from the sum and number of averaged pixels
            dst_data[0] = (unsigned char)(sum_r / averaged_pixels);
            dst_data[1] = (unsigned char)(sum_g / averaged_pixels);
            dst_data[2] = (unsigned char)(sum_b / averaged_pixels);
            dst_data += 3;
            if ( src_alpha )
                *dst_alpha++ = (unsigned char)(sum_a / averaged_pixels);
        }
    }

    return ret_image;
}

// The following two local functions are for the B-spline weighting of the
// bicubic sampling algorithm
static inline double spline_cube(double value)
{
    return value <= 0.0 ? 0.0 : value * value * value;
}

static inline double spline_weight(double value)
{
    return (spline_cube(value + 2) -
            4 * spline_cube(value + 1) +
            6 * spline_cube(value) -
            4 * spline_cube(value - 1)) / 6;
}

// This is the bicubic resampling algorithm
wxImage wxImage::ResampleBicubic(int width, int height) const
{
    // This function implements a Bicubic B-Spline algorithm for resampling.
    // This method is certainly a little slower than wxImage's default pixel
    // replication method, however for most reasonably sized images not being
    // upsampled too much on a fairly average CPU this difference is hardly
    // noticeable and the results are far more pleasing to look at.
    //
    // This particular bicubic algorithm does pixel weighting according to a
    // B-Spline that basically implements a Gaussian bell-like weighting
    // kernel. Because of this method the results may appear a bit blurry when
    // upsampling by large factors.  This is basically because a slight
    // gaussian blur is being performed to get the smooth look of the upsampled
    // image.

    // Edge pixels: 3-4 possible solutions
    // - (Wrap/tile) Wrap the image, take the color value from the opposite
    // side of the image.
    // - (Mirror)    Duplicate edge pixels, so that pixel at coordinate (2, n),
    // where n is nonpositive, will have the value of (2, 1).
    // - (Ignore)    Simply ignore the edge pixels and apply the kernel only to
    // pixels which do have all neighbours.
    // - (Clamp)     Choose the nearest pixel along the border. This takes the
    // border pixels and extends them out to infinity.
    //
    // NOTE: below the y_offset and x_offset variables are being set for edge
    // pixels using the "Mirror" method mentioned above

    wxImage ret_image;

    ret_image.Create(width, height, false);

    unsigned char* src_data = M_IMGDATA->m_data;
    unsigned char* src_alpha = M_IMGDATA->m_alpha;
    unsigned char* dst_data = ret_image.GetData();
    unsigned char* dst_alpha = NULL;

    if ( src_alpha )
    {
        ret_image.SetAlpha();
        dst_alpha = ret_image.GetAlpha();
    }

    for ( int dsty = 0; dsty < height; dsty++ )
    {
        // We need to calculate the source pixel to interpolate from - Y-axis
        double srcpixy = double(dsty * M_IMGDATA->m_height) / height;
        double dy = srcpixy - (int)srcpixy;

        for ( int dstx = 0; dstx < width; dstx++ )
        {
            // X-axis of pixel to interpolate from
            double srcpixx = double(dstx * M_IMGDATA->m_width) / width;
            double dx = srcpixx - (int)srcpixx;

            // Sums for each color channel
            double sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0;

            // Here we actually determine the RGBA values for the destination pixel
            for ( int k = -1; k <= 2; k++ )
            {
                // Y offset
                int y_offset = srcpixy + k < 0.0
                                ? 0
                                : srcpixy + k >= M_IMGDATA->m_height
                                       ? M_IMGDATA->m_height - 1
                                       : (int)(srcpixy + k);

                // Loop across the X axis
                for ( int i = -1; i <= 2; i++ )
                {
                    // X offset
                    int x_offset = srcpixx + i < 0.0
                                    ? 0
                                    : srcpixx + i >= M_IMGDATA->m_width
                                            ? M_IMGDATA->m_width - 1
                                            : (int)(srcpixx + i);

                    // Calculate the exact position where the source data
                    // should be pulled from based on the x_offset and y_offset
                    int src_pixel_index = y_offset*M_IMGDATA->m_width + x_offset;

                    // Calculate the weight for the specified pixel according
                    // to the bicubic b-spline kernel we're using for
                    // interpolation
                    double
                        pixel_weight = spline_weight(i - dx)*spline_weight(k - dy);

                    // Create a sum of all velues for each color channel
                    // adjusted for the pixel's calculated weight
                    sum_r += src_data[src_pixel_index * 3 + 0] * pixel_weight;
                    sum_g += src_data[src_pixel_index * 3 + 1] * pixel_weight;
                    sum_b += src_data[src_pixel_index * 3 + 2] * pixel_weight;
                    if ( src_alpha )
                        sum_a += src_alpha[src_pixel_index] * pixel_weight;
                }
            }

            // Put the data into the destination image.  The summed values are
            // of double data type and are rounded here for accuracy
            dst_data[0] = (unsigned char)(sum_r + 0.5);
            dst_data[1] = (unsigned char)(sum_g + 0.5);
            dst_data[2] = (unsigned char)(sum_b + 0.5);
            dst_data += 3;

            if ( src_alpha )
                *dst_alpha++ = (unsigned char)sum_a;
        }
    }

    return ret_image;
}

// Blur in the horizontal direction
wxImage wxImage::BlurHorizontal(int blurRadius)
{
    wxImage ret_image;
    ret_image.Create(M_IMGDATA->m_width, M_IMGDATA->m_height, false);

    unsigned char* src_data = M_IMGDATA->m_data;
    unsigned char* dst_data = ret_image.GetData();
    unsigned char* src_alpha = M_IMGDATA->m_alpha;
    unsigned char* dst_alpha = NULL;

    // Check for a mask or alpha
    if ( M_IMGDATA->m_hasMask )
    {
        ret_image.SetMaskColour(M_IMGDATA->m_maskRed,
                                M_IMGDATA->m_maskGreen,
                                M_IMGDATA->m_maskBlue);
    }
    else
    {
        if ( src_alpha )
        {
            ret_image.SetAlpha();
            dst_alpha = ret_image.GetAlpha();
        }
    }

    // number of pixels we average over
    const int blurArea = blurRadius*2 + 1;

    // Horizontal blurring algorithm - average all pixels in the specified blur
    // radius in the X or horizontal direction
    for ( int y = 0; y < M_IMGDATA->m_height; y++ )
    {
        // Variables used in the blurring algorithm
        long sum_r = 0,
             sum_g = 0,
             sum_b = 0,
             sum_a = 0;

        long pixel_idx;
        const unsigned char *src;
        unsigned char *dst;

        // Calculate the average of all pixels in the blur radius for the first
        // pixel of the row
        for ( int kernel_x = -blurRadius; kernel_x <= blurRadius; kernel_x++ )
        {
            // To deal with the pixels at the start of a row so it's not
            // grabbing GOK values from memory at negative indices of the
            // image's data or grabbing from the previous row
            if ( kernel_x < 0 )
                pixel_idx = y * M_IMGDATA->m_width;
            else
                pixel_idx = kernel_x + y * M_IMGDATA->m_width;

            src = src_data + pixel_idx*3;
            sum_r += src[0];
            sum_g += src[1];
            sum_b += src[2];
            if ( src_alpha )
                sum_a += src_alpha[pixel_idx];
        }

        dst = dst_data + y * M_IMGDATA->m_width*3;
        dst[0] = (unsigned char)(sum_r / blurArea);
        dst[1] = (unsigned char)(sum_g / blurArea);
        dst[2] = (unsigned char)(sum_b / blurArea);
        if ( src_alpha )
            dst_alpha[y * M_IMGDATA->m_width] = (unsigned char)(sum_a / blurArea);

        // Now average the values of the rest of the pixels by just moving the
        // blur radius box along the row
        for ( int x = 1; x < M_IMGDATA->m_width; x++ )
        {
            // Take care of edge pixels on the left edge by essentially
            // duplicating the edge pixel
            if ( x - blurRadius - 1 < 0 )
                pixel_idx = y * M_IMGDATA->m_width;
            else
                pixel_idx = (x - blurRadius - 1) + y * M_IMGDATA->m_width;

            // Subtract the value of the pixel at the left side of the blur
            // radius box
            src = src_data + pixel_idx*3;
            sum_r -= src[0];
            sum_g -= src[1];
            sum_b -= src[2];
            if ( src_alpha )
                sum_a -= src_alpha[pixel_idx];

            // Take care of edge pixels on the right edge
            if ( x + blurRadius > M_IMGDATA->m_width - 1 )
                pixel_idx = M_IMGDATA->m_width - 1 + y * M_IMGDATA->m_width;
            else
                pixel_idx = x + blurRadius + y * M_IMGDATA->m_width;

            // Add the value of the pixel being added to the end of our box
            src = src_data + pixel_idx*3;
            sum_r += src[0];
            sum_g += src[1];
            sum_b += src[2];
            if ( src_alpha )
                sum_a += src_alpha[pixel_idx];

            // Save off the averaged data
            dst = dst_data + x*3 + y*M_IMGDATA->m_width*3;
            dst[0] = (unsigned char)(sum_r / blurArea);
            dst[1] = (unsigned char)(sum_g / blurArea);
            dst[2] = (unsigned char)(sum_b / blurArea);
            if ( src_alpha )
                dst_alpha[x + y * M_IMGDATA->m_width] = (unsigned char)(sum_a / blurArea);
        }
    }

    return ret_image;
}

// Blur in the vertical direction
wxImage wxImage::BlurVertical(int blurRadius)
{
    wxImage ret_image;
    ret_image.Create(M_IMGDATA->m_width, M_IMGDATA->m_height, false);

    unsigned char* src_data = M_IMGDATA->m_data;
    unsigned char* dst_data = ret_image.GetData();
    unsigned char* src_alpha = M_IMGDATA->m_alpha;
    unsigned char* dst_alpha = NULL;

    // Check for a mask or alpha
    if ( M_IMGDATA->m_hasMask )
    {
        ret_image.SetMaskColour(M_IMGDATA->m_maskRed,
                                M_IMGDATA->m_maskGreen,
                                M_IMGDATA->m_maskBlue);
    }
    else
    {
        if ( src_alpha )
        {
            ret_image.SetAlpha();
            dst_alpha = ret_image.GetAlpha();
        }
    }

    // number of pixels we average over
    const int blurArea = blurRadius*2 + 1;

    // Vertical blurring algorithm - same as horizontal but switched the
    // opposite direction
    for ( int x = 0; x < M_IMGDATA->m_width; x++ )
    {
        // Variables used in the blurring algorithm
        long sum_r = 0,
             sum_g = 0,
             sum_b = 0,
             sum_a = 0;

        long pixel_idx;
        const unsigned char *src;
        unsigned char *dst;

        // Calculate the average of all pixels in our blur radius box for the
        // first pixel of the column
        for ( int kernel_y = -blurRadius; kernel_y <= blurRadius; kernel_y++ )
        {
            // To deal with the pixels at the start of a column so it's not
            // grabbing GOK values from memory at negative indices of the
            // image's data or grabbing from the previous column
            if ( kernel_y < 0 )
                pixel_idx = x;
            else
                pixel_idx = x + kernel_y * M_IMGDATA->m_width;

            src = src_data + pixel_idx*3;
            sum_r += src[0];
            sum_g += src[1];
            sum_b += src[2];
            if ( src_alpha )
                sum_a += src_alpha[pixel_idx];
        }

        dst = dst_data + x*3;
        dst[0] = (unsigned char)(sum_r / blurArea);
        dst[1] = (unsigned char)(sum_g / blurArea);
        dst[2] = (unsigned char)(sum_b / blurArea);
        if ( src_alpha )
            dst_alpha[x] = (unsigned char)(sum_a / blurArea);

        // Now average the values of the rest of the pixels by just moving the
        // box along the column from top to bottom
        for ( int y = 1; y < M_IMGDATA->m_height; y++ )
        {
            // Take care of pixels that would be beyond the top edge by
            // duplicating the top edge pixel for the column
            if ( y - blurRadius - 1 < 0 )
                pixel_idx = x;
            else
                pixel_idx = x + (y - blurRadius - 1) * M_IMGDATA->m_width;

            // Subtract the value of the pixel at the top of our blur radius box
            src = src_data + pixel_idx*3;
            sum_r -= src[0];
            sum_g -= src[1];
            sum_b -= src[2];
            if ( src_alpha )
                sum_a -= src_alpha[pixel_idx];

            // Take care of the pixels that would be beyond the bottom edge of
            // the image similar to the top edge
            if ( y + blurRadius > M_IMGDATA->m_height - 1 )
                pixel_idx = x + (M_IMGDATA->m_height - 1) * M_IMGDATA->m_width;
            else
                pixel_idx = x + (blurRadius + y) * M_IMGDATA->m_width;

            // Add the value of the pixel being added to the end of our box
            src = src_data + pixel_idx*3;
            sum_r += src[0];
            sum_g += src[1];
            sum_b += src[2];
            if ( src_alpha )
                sum_a += src_alpha[pixel_idx];

            // Save off the averaged data
            dst = dst_data + (x + y * M_IMGDATA->m_width) * 3;
            dst[0] = (unsigned char)(sum_r / blurArea);
            dst[1] = (unsigned char)(sum_g / blurArea);
            dst[2] = (unsigned char)(sum_b / blurArea);
            if ( src_alpha )
                dst_alpha[x + y * M_IMGDATA->m_width] = (unsigned char)(sum_a / blurArea);
        }
    }

    return ret_image;
}

// The new blur function
wxImage wxImage::Blur(int blurRadius)
{
    wxImage ret_image;
    ret_image.Create(M_IMGDATA->m_width, M_IMGDATA->m_height, false);

    // Blur the image in each direction
    ret_image = BlurHorizontal(blurRadius);
    ret_image = ret_image.BlurVertical(blurRadius);

    return ret_image;
}

wxImage wxImage::Rotate90( bool clockwise ) const
{
    wxImage image;

    wxCHECK_MSG( Ok(), image, wxT("invalid image") );

    image.Create( M_IMGDATA->m_height, M_IMGDATA->m_width, false );

    unsigned char *data = image.GetData();

    wxCHECK_MSG( data, image, wxT("unable to create image") );

    unsigned char *source_data = M_IMGDATA->m_data;
    unsigned char *target_data;
    unsigned char *alpha_data = 0 ;
    unsigned char *source_alpha = 0 ;
    unsigned char *target_alpha = 0 ;

    if (M_IMGDATA->m_hasMask)
    {
        image.SetMaskColour( M_IMGDATA->m_maskRed, M_IMGDATA->m_maskGreen, M_IMGDATA->m_maskBlue );
    }
    else
    {
        source_alpha = M_IMGDATA->m_alpha ;
        if ( source_alpha )
        {
            image.SetAlpha() ;
            alpha_data = image.GetAlpha() ;
        }
    }

    long height = M_IMGDATA->m_height;
    long width  = M_IMGDATA->m_width;

    for (long j = 0; j < height; j++)
    {
        for (long i = 0; i < width; i++)
        {
            if (clockwise)
            {
                target_data = data + (((i+1)*height) - j - 1)*3;
                if(source_alpha)
                    target_alpha = alpha_data + (((i+1)*height) - j - 1);
            }
            else
            {
                target_data = data + ((height*(width-1)) + j - (i*height))*3;
                if(source_alpha)
                    target_alpha = alpha_data + ((height*(width-1)) + j - (i*height));
            }
            memcpy( target_data, source_data, 3 );
            source_data += 3;

            if(source_alpha)
            {
                memcpy( target_alpha, source_alpha, 1 );
                source_alpha += 1;
            }
        }
    }

    return image;
}

wxImage wxImage::Mirror( bool horizontally ) const
{
    wxImage image;

    wxCHECK_MSG( Ok(), image, wxT("invalid image") );

    image.Create( M_IMGDATA->m_width, M_IMGDATA->m_height, false );

    unsigned char *data = image.GetData();
    unsigned char *alpha = NULL;

    wxCHECK_MSG( data, image, wxT("unable to create image") );

    if (M_IMGDATA->m_alpha != NULL) {
        image.SetAlpha();
        alpha = image.GetAlpha();
        wxCHECK_MSG( alpha, image, wxT("unable to create alpha channel") );
    }

    if (M_IMGDATA->m_hasMask)
        image.SetMaskColour( M_IMGDATA->m_maskRed, M_IMGDATA->m_maskGreen, M_IMGDATA->m_maskBlue );

    long height = M_IMGDATA->m_height;
    long width  = M_IMGDATA->m_width;

    unsigned char *source_data = M_IMGDATA->m_data;
    unsigned char *target_data;

    if (horizontally)
    {
        for (long j = 0; j < height; j++)
        {
            data += width*3;
            target_data = data-3;
            for (long i = 0; i < width; i++)
            {
                memcpy( target_data, source_data, 3 );
                source_data += 3;
                target_data -= 3;
            }
        }

        if (alpha != NULL)
        {
            // src_alpha starts at the first pixel and increases by 1 after each step
            // (a step here is the copy of the alpha value of one pixel)
            const unsigned char *src_alpha = M_IMGDATA->m_alpha;
            // dest_alpha starts just beyond the first line, decreases before each step,
            // and after each line is finished, increases by 2 widths (skipping the line
            // just copied and the line that will be copied next)
            unsigned char *dest_alpha = alpha + width;

            for (long jj = 0; jj < height; ++jj)
            {
                for (long i = 0; i < width; ++i) {
                    *(--dest_alpha) = *(src_alpha++); // copy one pixel
                }
                dest_alpha += 2 * width; // advance beyond the end of the next line
            }
        }
    }
    else
    {
        for (long i = 0; i < height; i++)
        {
            target_data = data + 3*width*(height-1-i);
            memcpy( target_data, source_data, (size_t)3*width );
            source_data += 3*width;
        }

        if (alpha != NULL)
        {
            // src_alpha starts at the first pixel and increases by 1 width after each step
            // (a step here is the copy of the alpha channel of an entire line)
            const unsigned char *src_alpha = M_IMGDATA->m_alpha;
            // dest_alpha starts just beyond the last line (beyond the whole image)
            // and decreases by 1 width before each step
            unsigned char *dest_alpha = alpha + width * height;

            for (long jj = 0; jj < height; ++jj)
            {
                dest_alpha -= width;
                memcpy( dest_alpha, src_alpha, (size_t)width );
                src_alpha += width;
            }
        }
    }

    return image;
}

wxImage wxImage::GetSubImage( const wxRect &rect ) const
{
    wxImage image;

    wxCHECK_MSG( Ok(), image, wxT("invalid image") );

    wxCHECK_MSG( (rect.GetLeft()>=0) && (rect.GetTop()>=0) &&
                 (rect.GetRight()<=GetWidth()) && (rect.GetBottom()<=GetHeight()),
                 image, wxT("invalid subimage size") );

    const int subwidth = rect.GetWidth();
    const int subheight = rect.GetHeight();

    image.Create( subwidth, subheight, false );

    const unsigned char *src_data = GetData();
    const unsigned char *src_alpha = M_IMGDATA->m_alpha;
    unsigned char *subdata = image.GetData();
    unsigned char *subalpha = NULL;

    wxCHECK_MSG( subdata, image, wxT("unable to create image") );

    if (src_alpha != NULL) {
        image.SetAlpha();
        subalpha = image.GetAlpha();
        wxCHECK_MSG( subalpha, image, wxT("unable to create alpha channel"));
    }

    if (M_IMGDATA->m_hasMask)
        image.SetMaskColour( M_IMGDATA->m_maskRed, M_IMGDATA->m_maskGreen, M_IMGDATA->m_maskBlue );

    const int width = GetWidth();
    const int pixsoff = rect.GetLeft() + width * rect.GetTop();

    src_data += 3 * pixsoff;
    src_alpha += pixsoff; // won't be used if was NULL, so this is ok

    for (long j = 0; j < subheight; ++j)
    {
        memcpy( subdata, src_data, 3 * subwidth );
        subdata += 3 * subwidth;
        src_data += 3 * width;
        if (subalpha != NULL) {
            memcpy( subalpha, src_alpha, subwidth );
            subalpha += subwidth;
            src_alpha += width;
        }
    }

    return image;
}

wxImage wxImage::Size( const wxSize& size, const wxPoint& pos,
                       int r_, int g_, int b_ ) const
{
    wxImage image;

    wxCHECK_MSG( Ok(), image, wxT("invalid image") );
    wxCHECK_MSG( (size.GetWidth() > 0) && (size.GetHeight() > 0), image, wxT("invalid size") );

    int width = GetWidth(), height = GetHeight();
    image.Create(size.GetWidth(), size.GetHeight(), false);

    unsigned char r = (unsigned char)r_;
    unsigned char g = (unsigned char)g_;
    unsigned char b = (unsigned char)b_;
    if ((r_ == -1) && (g_ == -1) && (b_ == -1))
    {
        GetOrFindMaskColour( &r, &g, &b );
        image.SetMaskColour(r, g, b);
    }

    image.SetRGB(wxRect(), r, g, b);

    wxRect subRect(pos.x, pos.y, width, height);
    wxRect finalRect(0, 0, size.GetWidth(), size.GetHeight());
    if (pos.x < 0)
        finalRect.width -= pos.x;
    if (pos.y < 0)
        finalRect.height -= pos.y;

    subRect.Intersect(finalRect);

    if (!subRect.IsEmpty())
    {
        if ((subRect.GetWidth() == width) && (subRect.GetHeight() == height))
            image.Paste(*this, pos.x, pos.y);
        else
            image.Paste(GetSubImage(subRect), pos.x, pos.y);
    }

    return image;
}

void wxImage::Paste( const wxImage &image, int x, int y )
{
    wxCHECK_RET( Ok(), wxT("invalid image") );
    wxCHECK_RET( image.Ok(), wxT("invalid image") );

    AllocExclusive();

    int xx = 0;
    int yy = 0;
    int width = image.GetWidth();
    int height = image.GetHeight();

    if (x < 0)
    {
        xx = -x;
        width += x;
    }
    if (y < 0)
    {
        yy = -y;
        height += y;
    }

    if ((x+xx)+width > M_IMGDATA->m_width)
        width = M_IMGDATA->m_width - (x+xx);
    if ((y+yy)+height > M_IMGDATA->m_height)
        height = M_IMGDATA->m_height - (y+yy);

    if (width < 1) return;
    if (height < 1) return;

    if ((!HasMask() && !image.HasMask()) ||
        (HasMask() && !image.HasMask()) ||
       ((HasMask() && image.HasMask() &&
         (GetMaskRed()==image.GetMaskRed()) &&
         (GetMaskGreen()==image.GetMaskGreen()) &&
         (GetMaskBlue()==image.GetMaskBlue()))))
    {
        unsigned char* source_data = image.GetData() + xx*3 + yy*3*image.GetWidth();
        int source_step = image.GetWidth()*3;

        unsigned char* target_data = GetData() + (x+xx)*3 + (y+yy)*3*M_IMGDATA->m_width;
        int target_step = M_IMGDATA->m_width*3;
        for (int j = 0; j < height; j++)
        {
            memcpy( target_data, source_data, width*3 );
            source_data += source_step;
            target_data += target_step;
        }
    }

    // Copy over the alpha channel from the original image
    if ( image.HasAlpha() )
    {
        if ( !HasAlpha() )
            InitAlpha();

        unsigned char* source_data = image.GetAlpha() + xx + yy*image.GetWidth();
        int source_step = image.GetWidth();

        unsigned char* target_data = GetAlpha() + (x+xx) + (y+yy)*M_IMGDATA->m_width;
        int target_step = M_IMGDATA->m_width;

        for (int j = 0; j < height; j++,
                                    source_data += source_step,
                                    target_data += target_step)
        {
            memcpy( target_data, source_data, width );
        }
    }

    if (!HasMask() && image.HasMask())
    {
        unsigned char r = image.GetMaskRed();
        unsigned char g = image.GetMaskGreen();
        unsigned char b = image.GetMaskBlue();

        unsigned char* source_data = image.GetData() + xx*3 + yy*3*image.GetWidth();
        int source_step = image.GetWidth()*3;

        unsigned char* target_data = GetData() + (x+xx)*3 + (y+yy)*3*M_IMGDATA->m_width;
        int target_step = M_IMGDATA->m_width*3;

        for (int j = 0; j < height; j++)
        {
            for (int i = 0; i < width*3; i+=3)
            {
                if ((source_data[i]   != r) ||
                    (source_data[i+1] != g) ||
                    (source_data[i+2] != b))
                {
                    memcpy( target_data+i, source_data+i, 3 );
                }
            }
            source_data += source_step;
            target_data += target_step;
        }
    }
}

void wxImage::Replace( unsigned char r1, unsigned char g1, unsigned char b1,
                       unsigned char r2, unsigned char g2, unsigned char b2 )
{
    wxCHECK_RET( Ok(), wxT("invalid image") );

    AllocExclusive();

    unsigned char *data = GetData();

    const int w = GetWidth();
    const int h = GetHeight();

    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
        {
            if ((data[0] == r1) && (data[1] == g1) && (data[2] == b1))
            {
                data[0] = r2;
                data[1] = g2;
                data[2] = b2;
            }
            data += 3;
        }
}

wxImage wxImage::ConvertToGreyscale( double lr, double lg, double lb ) const
{
    wxImage image;

    wxCHECK_MSG( Ok(), image, wxT("invalid image") );

    image.Create(M_IMGDATA->m_width, M_IMGDATA->m_height, false);

    unsigned char *dest = image.GetData();

    wxCHECK_MSG( dest, image, wxT("unable to create image") );

    unsigned char *src = M_IMGDATA->m_data;
    bool hasMask = M_IMGDATA->m_hasMask;
    unsigned char maskRed = M_IMGDATA->m_maskRed;
    unsigned char maskGreen = M_IMGDATA->m_maskGreen;
    unsigned char maskBlue = M_IMGDATA->m_maskBlue;

    if ( hasMask )
        image.SetMaskColour(maskRed, maskGreen, maskBlue);

    const long size = M_IMGDATA->m_width * M_IMGDATA->m_height;
    for ( long i = 0; i < size; i++, src += 3, dest += 3 )
    {
        // don't modify the mask
        if ( hasMask && src[0] == maskRed && src[1] == maskGreen && src[2] == maskBlue )
        {
            memcpy(dest, src, 3);
        }
        else
        {
            // calculate the luma
            double luma = (src[0] * lr + src[1] * lg + src[2] * lb) + 0.5;
            dest[0] = dest[1] = dest[2] = wx_static_cast(unsigned char, luma);
        }
    }

    // copy the alpha channel, if any
    if (HasAlpha())
    {
        const size_t alphaSize = GetWidth() * GetHeight();
        unsigned char *alpha = (unsigned char*)malloc(alphaSize);
        memcpy(alpha, GetAlpha(), alphaSize);
        image.InitAlpha();
        image.SetAlpha(alpha);
    }

    return image;
}

wxImage wxImage::ConvertToMono( unsigned char r, unsigned char g, unsigned char b ) const
{
    wxImage image;

    wxCHECK_MSG( Ok(), image, wxT("invalid image") );

    image.Create( M_IMGDATA->m_width, M_IMGDATA->m_height, false );

    unsigned char *data = image.GetData();

    wxCHECK_MSG( data, image, wxT("unable to create image") );

    if (M_IMGDATA->m_hasMask)
    {
        if (M_IMGDATA->m_maskRed == r && M_IMGDATA->m_maskGreen == g &&
                                         M_IMGDATA->m_maskBlue == b)
            image.SetMaskColour( 255, 255, 255 );
        else
            image.SetMaskColour( 0, 0, 0 );
    }

    long size = M_IMGDATA->m_height * M_IMGDATA->m_width;

    unsigned char *srcd = M_IMGDATA->m_data;
    unsigned char *tard = image.GetData();

    for ( long i = 0; i < size; i++, srcd += 3, tard += 3 )
    {
        if (srcd[0] == r && srcd[1] == g && srcd[2] == b)
            tard[0] = tard[1] = tard[2] = 255;
        else
            tard[0] = tard[1] = tard[2] = 0;
    }

    return image;
}

int wxImage::GetWidth() const
{
    wxCHECK_MSG( Ok(), 0, wxT("invalid image") );

    return M_IMGDATA->m_width;
}

int wxImage::GetHeight() const
{
    wxCHECK_MSG( Ok(), 0, wxT("invalid image") );

    return M_IMGDATA->m_height;
}

long wxImage::XYToIndex(int x, int y) const
{
    if ( Ok() &&
            x >= 0 && y >= 0 &&
                x < M_IMGDATA->m_width && y < M_IMGDATA->m_height )
    {
        return y*M_IMGDATA->m_width + x;
    }

    return -1;
}

void wxImage::SetRGB( int x, int y, unsigned char r, unsigned char g, unsigned char b )
{
    long pos = XYToIndex(x, y);
    wxCHECK_RET( pos != -1, wxT("invalid image coordinates") );

    AllocExclusive();

    pos *= 3;

    M_IMGDATA->m_data[ pos   ] = r;
    M_IMGDATA->m_data[ pos+1 ] = g;
    M_IMGDATA->m_data[ pos+2 ] = b;
}

void wxImage::SetRGB( const wxRect& rect_, unsigned char r, unsigned char g, unsigned char b )
{
    wxCHECK_RET( Ok(), wxT("invalid image") );

    AllocExclusive();

    wxRect rect(rect_);
    wxRect imageRect(0, 0, GetWidth(), GetHeight());
    if ( rect == wxRect() )
    {
        rect = imageRect;
    }
    else
    {
        wxCHECK_RET( imageRect.Contains(rect.GetTopLeft()) &&
                     imageRect.Contains(rect.GetBottomRight()),
                     wxT("invalid bounding rectangle") );
    }

    int x1 = rect.GetLeft(),
        y1 = rect.GetTop(),
        x2 = rect.GetRight() + 1,
        y2 = rect.GetBottom() + 1;

    unsigned char *data wxDUMMY_INITIALIZE(NULL);
    int x, y, width = GetWidth();
    for (y = y1; y < y2; y++)
    {
        data = M_IMGDATA->m_data + (y*width + x1)*3;
        for (x = x1; x < x2; x++)
        {
            *data++ = r;
            *data++ = g;
            *data++ = b;
        }
    }
}

unsigned char wxImage::GetRed( int x, int y ) const
{
    long pos = XYToIndex(x, y);
    wxCHECK_MSG( pos != -1, 0, wxT("invalid image coordinates") );

    pos *= 3;

    return M_IMGDATA->m_data[pos];
}

unsigned char wxImage::GetGreen( int x, int y ) const
{
    long pos = XYToIndex(x, y);
    wxCHECK_MSG( pos != -1, 0, wxT("invalid image coordinates") );

    pos *= 3;

    return M_IMGDATA->m_data[pos+1];
}

unsigned char wxImage::GetBlue( int x, int y ) const
{
    long pos = XYToIndex(x, y);
    wxCHECK_MSG( pos != -1, 0, wxT("invalid image coordinates") );

    pos *= 3;

    return M_IMGDATA->m_data[pos+2];
}

bool wxImage::IsOk() const
{
    // image of 0 width or height can't be considered ok - at least because it
    // causes crashes in ConvertToBitmap() if we don't catch it in time
    wxImageRefData *data = M_IMGDATA;
    return data && data->m_ok && data->m_width && data->m_height;
}

unsigned char *wxImage::GetData() const
{
    wxCHECK_MSG( Ok(), (unsigned char *)NULL, wxT("invalid image") );

    return M_IMGDATA->m_data;
}

void wxImage::SetData( unsigned char *data, bool static_data  )
{
    wxCHECK_RET( Ok(), wxT("invalid image") );

    wxImageRefData *newRefData = new wxImageRefData();

    newRefData->m_width = M_IMGDATA->m_width;
    newRefData->m_height = M_IMGDATA->m_height;
    newRefData->m_data = data;
    newRefData->m_ok = true;
    newRefData->m_maskRed = M_IMGDATA->m_maskRed;
    newRefData->m_maskGreen = M_IMGDATA->m_maskGreen;
    newRefData->m_maskBlue = M_IMGDATA->m_maskBlue;
    newRefData->m_hasMask = M_IMGDATA->m_hasMask;
    newRefData->m_static = static_data;

    UnRef();

    m_refData = newRefData;
}

void wxImage::SetData( unsigned char *data, int new_width, int new_height, bool static_data )
{
    wxImageRefData *newRefData = new wxImageRefData();

    if (m_refData)
    {
        newRefData->m_width = new_width;
        newRefData->m_height = new_height;
        newRefData->m_data = data;
        newRefData->m_ok = true;
        newRefData->m_maskRed = M_IMGDATA->m_maskRed;
        newRefData->m_maskGreen = M_IMGDATA->m_maskGreen;
        newRefData->m_maskBlue = M_IMGDATA->m_maskBlue;
        newRefData->m_hasMask = M_IMGDATA->m_hasMask;
    }
    else
    {
        newRefData->m_width = new_width;
        newRefData->m_height = new_height;
        newRefData->m_data = data;
        newRefData->m_ok = true;
    }
    newRefData->m_static = static_data;

    UnRef();

    m_refData = newRefData;
}

// ----------------------------------------------------------------------------
// alpha channel support
// ----------------------------------------------------------------------------

void wxImage::SetAlpha(int x, int y, unsigned char alpha)
{
    wxCHECK_RET( HasAlpha(), wxT("no alpha channel") );

    long pos = XYToIndex(x, y);
    wxCHECK_RET( pos != -1, wxT("invalid image coordinates") );

    AllocExclusive();

    M_IMGDATA->m_alpha[pos] = alpha;
}

unsigned char wxImage::GetAlpha(int x, int y) const
{
    wxCHECK_MSG( HasAlpha(), 0, wxT("no alpha channel") );

    long pos = XYToIndex(x, y);
    wxCHECK_MSG( pos != -1, 0, wxT("invalid image coordinates") );

    return M_IMGDATA->m_alpha[pos];
}

bool
wxImage::ConvertColourToAlpha(unsigned char r, unsigned char g, unsigned char b)
{
    SetAlpha(NULL);

    const int w = M_IMGDATA->m_width;
    const int h = M_IMGDATA->m_height;

    unsigned char *alpha = GetAlpha();
    unsigned char *data = GetData();

    for ( int y = 0; y < h; y++ )
    {
        for ( int x = 0; x < w; x++ )
        {
            *alpha++ = *data;
            *data++ = r;
            *data++ = g;
            *data++ = b;
        }
    }

    return true;
}

void wxImage::SetAlpha( unsigned char *alpha, bool static_data )
{
    wxCHECK_RET( Ok(), wxT("invalid image") );

    AllocExclusive();

    if ( !alpha )
    {
        alpha = (unsigned char *)malloc(M_IMGDATA->m_width*M_IMGDATA->m_height);
    }

    if( !M_IMGDATA->m_staticAlpha )
        free(M_IMGDATA->m_alpha);

    M_IMGDATA->m_alpha = alpha;
    M_IMGDATA->m_staticAlpha = static_data;
}

unsigned char *wxImage::GetAlpha() const
{
    wxCHECK_MSG( Ok(), (unsigned char *)NULL, wxT("invalid image") );

    return M_IMGDATA->m_alpha;
}

void wxImage::InitAlpha()
{
    wxCHECK_RET( !HasAlpha(), wxT("image already has an alpha channel") );

    // initialize memory for alpha channel
    SetAlpha();

    unsigned char *alpha = M_IMGDATA->m_alpha;
    const size_t lenAlpha = M_IMGDATA->m_width * M_IMGDATA->m_height;

    if ( HasMask() )
    {
        // use the mask to initialize the alpha channel.
        const unsigned char * const alphaEnd = alpha + lenAlpha;

        const unsigned char mr = M_IMGDATA->m_maskRed;
        const unsigned char mg = M_IMGDATA->m_maskGreen;
        const unsigned char mb = M_IMGDATA->m_maskBlue;
        for ( unsigned char *src = M_IMGDATA->m_data;
              alpha < alphaEnd;
              src += 3, alpha++ )
        {
            *alpha = (src[0] == mr && src[1] == mg && src[2] == mb)
                            ? wxIMAGE_ALPHA_TRANSPARENT
                            : wxIMAGE_ALPHA_OPAQUE;
        }

        M_IMGDATA->m_hasMask = false;
    }
    else // no mask
    {
        // make the image fully opaque
        memset(alpha, wxIMAGE_ALPHA_OPAQUE, lenAlpha);
    }
}

// ----------------------------------------------------------------------------
// mask support
// ----------------------------------------------------------------------------

void wxImage::SetMaskColour( unsigned char r, unsigned char g, unsigned char b )
{
    wxCHECK_RET( Ok(), wxT("invalid image") );

    AllocExclusive();

    M_IMGDATA->m_maskRed = r;
    M_IMGDATA->m_maskGreen = g;
    M_IMGDATA->m_maskBlue = b;
    M_IMGDATA->m_hasMask = true;
}

bool wxImage::GetOrFindMaskColour( unsigned char *r, unsigned char *g, unsigned char *b ) const
{
    wxCHECK_MSG( Ok(), false, wxT("invalid image") );

    if (M_IMGDATA->m_hasMask)
    {
        if (r) *r = M_IMGDATA->m_maskRed;
        if (g) *g = M_IMGDATA->m_maskGreen;
        if (b) *b = M_IMGDATA->m_maskBlue;
        return true;
    }
    else
    {
        FindFirstUnusedColour(r, g, b);
        return false;
    }
}

unsigned char wxImage::GetMaskRed() const
{
    wxCHECK_MSG( Ok(), 0, wxT("invalid image") );

    return M_IMGDATA->m_maskRed;
}

unsigned char wxImage::GetMaskGreen() const
{
    wxCHECK_MSG( Ok(), 0, wxT("invalid image") );

    return M_IMGDATA->m_maskGreen;
}

unsigned char wxImage::GetMaskBlue() const
{
    wxCHECK_MSG( Ok(), 0, wxT("invalid image") );

    return M_IMGDATA->m_maskBlue;
}

void wxImage::SetMask( bool mask )
{
    wxCHECK_RET( Ok(), wxT("invalid image") );

    AllocExclusive();

    M_IMGDATA->m_hasMask = mask;
}

bool wxImage::HasMask() const
{
    wxCHECK_MSG( Ok(), false, wxT("invalid image") );

    return M_IMGDATA->m_hasMask;
}

bool wxImage::IsTransparent(int x, int y, unsigned char threshold) const
{
    long pos = XYToIndex(x, y);
    wxCHECK_MSG( pos != -1, false, wxT("invalid image coordinates") );

    // check mask
    if ( M_IMGDATA->m_hasMask )
    {
        const unsigned char *p = M_IMGDATA->m_data + 3*pos;
        if ( p[0] == M_IMGDATA->m_maskRed &&
                p[1] == M_IMGDATA->m_maskGreen &&
                    p[2] == M_IMGDATA->m_maskBlue )
        {
            return true;
        }
    }

    // then check alpha
    if ( M_IMGDATA->m_alpha )
    {
        if ( M_IMGDATA->m_alpha[pos] < threshold )
        {
            // transparent enough
            return true;
        }
    }

    // not transparent
    return false;
}

bool wxImage::SetMaskFromImage(const wxImage& mask,
                               unsigned char mr, unsigned char mg, unsigned char mb)
{
    // check that the images are the same size
    if ( (M_IMGDATA->m_height != mask.GetHeight() ) || (M_IMGDATA->m_width != mask.GetWidth () ) )
    {
        wxLogError( _("Image and mask have different sizes.") );
        return false;
    }

    // find unused colour
    unsigned char r,g,b ;
    if (!FindFirstUnusedColour(&r, &g, &b))
    {
        wxLogError( _("No unused colour in image being masked.") );
        return false ;
    }

    AllocExclusive();

    unsigned char *imgdata = GetData();
    unsigned char *maskdata = mask.GetData();

    const int w = GetWidth();
    const int h = GetHeight();

    for (int j = 0; j < h; j++)
    {
        for (int i = 0; i < w; i++)
        {
            if ((maskdata[0] == mr) && (maskdata[1]  == mg) && (maskdata[2] == mb))
            {
                imgdata[0] = r;
                imgdata[1] = g;
                imgdata[2] = b;
            }
            imgdata  += 3;
            maskdata += 3;
        }
    }

    SetMaskColour(r, g, b);
    SetMask(true);

    return true;
}

bool wxImage::ConvertAlphaToMask(unsigned char threshold)
{
    if (!HasAlpha())
        return true;

    unsigned char mr, mg, mb;
    if (!FindFirstUnusedColour(&mr, &mg, &mb))
    {
        wxLogError( _("No unused colour in image being masked.") );
        return false;
    }

    AllocExclusive();

    SetMask(true);
    SetMaskColour(mr, mg, mb);

    unsigned char *imgdata = GetData();
    unsigned char *alphadata = GetAlpha();

    int w = GetWidth();
    int h = GetHeight();

    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++, imgdata += 3, alphadata++)
        {
            if (*alphadata < threshold)
            {
                imgdata[0] = mr;
                imgdata[1] = mg;
                imgdata[2] = mb;
            }
        }
    }

    if( !M_IMGDATA->m_staticAlpha )
        free(M_IMGDATA->m_alpha);

    M_IMGDATA->m_alpha = NULL;
    M_IMGDATA->m_staticAlpha = false;

    return true;
}

// ----------------------------------------------------------------------------
// Palette functions
// ----------------------------------------------------------------------------

#if wxUSE_PALETTE

bool wxImage::HasPalette() const
{
    if (!Ok())
        return false;

    return M_IMGDATA->m_palette.Ok();
}

const wxPalette& wxImage::GetPalette() const
{
    wxCHECK_MSG( Ok(), wxNullPalette, wxT("invalid image") );

    return M_IMGDATA->m_palette;
}

void wxImage::SetPalette(const wxPalette& palette)
{
    wxCHECK_RET( Ok(), wxT("invalid image") );

    AllocExclusive();

    M_IMGDATA->m_palette = palette;
}

#endif // wxUSE_PALETTE

// ----------------------------------------------------------------------------
// Option functions (arbitrary name/value mapping)
// ----------------------------------------------------------------------------

void wxImage::SetOption(const wxString& name, const wxString& value)
{
    wxCHECK_RET( Ok(), wxT("invalid image") );

    AllocExclusive();

    int idx = M_IMGDATA->m_optionNames.Index(name, false);
    if (idx == wxNOT_FOUND)
    {
        M_IMGDATA->m_optionNames.Add(name);
        M_IMGDATA->m_optionValues.Add(value);
    }
    else
    {
        M_IMGDATA->m_optionNames[idx] = name;
        M_IMGDATA->m_optionValues[idx] = value;
    }
}

void wxImage::SetOption(const wxString& name, int value)
{
    wxString valStr;
    valStr.Printf(wxT("%d"), value);
    SetOption(name, valStr);
}

wxString wxImage::GetOption(const wxString& name) const
{
    wxCHECK_MSG( Ok(), wxEmptyString, wxT("invalid image") );

    int idx = M_IMGDATA->m_optionNames.Index(name, false);
    if (idx == wxNOT_FOUND)
        return wxEmptyString;
    else
        return M_IMGDATA->m_optionValues[idx];
}

int wxImage::GetOptionInt(const wxString& name) const
{
    return wxAtoi(GetOption(name));
}

bool wxImage::HasOption(const wxString& name) const
{
    wxCHECK_MSG( Ok(), false, wxT("invalid image") );

    return (M_IMGDATA->m_optionNames.Index(name, false) != wxNOT_FOUND);
}

// ----------------------------------------------------------------------------
// image I/O
// ----------------------------------------------------------------------------

bool wxImage::LoadFile( const wxString& WXUNUSED_UNLESS_STREAMS(filename),
                        long WXUNUSED_UNLESS_STREAMS(type),
                        int WXUNUSED_UNLESS_STREAMS(index) )
{
#if HAS_FILE_STREAMS
    if (wxFileExists(filename))
    {
        wxImageFileInputStream stream(filename);
        wxBufferedInputStream bstream( stream );
        return LoadFile(bstream, type, index);
    }
    else
    {
        wxLogError( _("Can't load image from file '%s': file does not exist."), filename.c_str() );

        return false;
    }
#else // !HAS_FILE_STREAMS
    return false;
#endif // HAS_FILE_STREAMS
}

bool wxImage::LoadFile( const wxString& WXUNUSED_UNLESS_STREAMS(filename),
                        const wxString& WXUNUSED_UNLESS_STREAMS(mimetype),
                        int WXUNUSED_UNLESS_STREAMS(index) )
{
#if HAS_FILE_STREAMS
    if (wxFileExists(filename))
    {
        wxImageFileInputStream stream(filename);
        wxBufferedInputStream bstream( stream );
        return LoadFile(bstream, mimetype, index);
    }
    else
    {
        wxLogError( _("Can't load image from file '%s': file does not exist."), filename.c_str() );

        return false;
    }
#else // !HAS_FILE_STREAMS
    return false;
#endif // HAS_FILE_STREAMS
}



bool wxImage::SaveFile( const wxString& filename ) const
{
    wxString ext = filename.AfterLast('.').Lower();

    wxImageHandler * pHandler = FindHandler(ext, -1);
    if (pHandler)
    {
        return SaveFile(filename, pHandler->GetType());
    }

    wxLogError(_("Can't save image to file '%s': unknown extension."), filename.c_str());

    return false;
}

bool wxImage::SaveFile( const wxString& WXUNUSED_UNLESS_STREAMS(filename),
                        int WXUNUSED_UNLESS_STREAMS(type) ) const
{
#if HAS_FILE_STREAMS
    wxCHECK_MSG( Ok(), false, wxT("invalid image") );

    ((wxImage*)this)->SetOption(wxIMAGE_OPTION_FILENAME, filename);

    wxImageFileOutputStream stream(filename);

    if ( stream.IsOk() )
    {
        wxBufferedOutputStream bstream( stream );
        return SaveFile(bstream, type);
    }
#endif // HAS_FILE_STREAMS

    return false;
}

bool wxImage::SaveFile( const wxString& WXUNUSED_UNLESS_STREAMS(filename),
                        const wxString& WXUNUSED_UNLESS_STREAMS(mimetype) ) const
{
#if HAS_FILE_STREAMS
    wxCHECK_MSG( Ok(), false, wxT("invalid image") );

    ((wxImage*)this)->SetOption(wxIMAGE_OPTION_FILENAME, filename);

    wxImageFileOutputStream stream(filename);

    if ( stream.IsOk() )
    {
        wxBufferedOutputStream bstream( stream );
        return SaveFile(bstream, mimetype);
    }
#endif // HAS_FILE_STREAMS

    return false;
}

bool wxImage::CanRead( const wxString& WXUNUSED_UNLESS_STREAMS(name) )
{
#if HAS_FILE_STREAMS
    wxImageFileInputStream stream(name);
    return CanRead(stream);
#else
    return false;
#endif
}

int wxImage::GetImageCount( const wxString& WXUNUSED_UNLESS_STREAMS(name),
                            long WXUNUSED_UNLESS_STREAMS(type) )
{
#if HAS_FILE_STREAMS
    wxImageFileInputStream stream(name);
    if (stream.Ok())
        return GetImageCount(stream, type);
#endif

  return 0;
}

#if wxUSE_STREAMS

bool wxImage::CanRead( wxInputStream &stream )
{
    const wxList& list = GetHandlers();

    for ( wxList::compatibility_iterator node = list.GetFirst(); node; node = node->GetNext() )
    {
        wxImageHandler *handler=(wxImageHandler*)node->GetData();
        if (handler->CanRead( stream ))
            return true;
    }

    return false;
}

int wxImage::GetImageCount( wxInputStream &stream, long type )
{
    wxImageHandler *handler;

    if ( type == wxBITMAP_TYPE_ANY )
    {
        wxList &list=GetHandlers();

        for (wxList::compatibility_iterator node = list.GetFirst(); node; node = node->GetNext())
        {
             handler=(wxImageHandler*)node->GetData();
             if ( handler->CanRead(stream) )
                 return handler->GetImageCount(stream);

        }

        wxLogWarning(_("No handler found for image type."));
        return 0;
    }

    handler = FindHandler(type);

    if ( !handler )
    {
        wxLogWarning(_("No image handler for type %ld defined."), type);
        return false;
    }

    if ( handler->CanRead(stream) )
    {
        return handler->GetImageCount(stream);
    }
    else
    {
        wxLogError(_("Image file is not of type %ld."), type);
        return 0;
    }
}

bool wxImage::LoadFile( wxInputStream& stream, long type, int index )
{
    UnRef();

    m_refData = new wxImageRefData;

    wxImageHandler *handler;

    if ( type == wxBITMAP_TYPE_ANY )
    {
        wxList &list=GetHandlers();

        for ( wxList::compatibility_iterator node = list.GetFirst(); node; node = node->GetNext() )
        {
             handler=(wxImageHandler*)node->GetData();
             if ( handler->CanRead(stream) )
                 return handler->LoadFile(this, stream, true/*verbose*/, index);

        }

        wxLogWarning( _("No handler found for image type.") );
        return false;
    }

    handler = FindHandler(type);

    if (handler == 0)
    {
        wxLogWarning( _("No image handler for type %ld defined."), type );

        return false;
    }

    if (stream.IsSeekable() && !handler->CanRead(stream))
    {
        wxLogError(_("Image file is not of type %ld."), type);
        return false;
    }
    else
        return handler->LoadFile(this, stream, true/*verbose*/, index);
}

bool wxImage::LoadFile( wxInputStream& stream, const wxString& mimetype, int index )
{
    UnRef();

    m_refData = new wxImageRefData;

    wxImageHandler *handler = FindHandlerMime(mimetype);

    if (handler == 0)
    {
        wxLogWarning( _("No image handler for type %s defined."), mimetype.GetData() );

        return false;
    }

    if (stream.IsSeekable() && !handler->CanRead(stream))
    {
        wxLogError(_("Image file is not of type %s."), (const wxChar*) mimetype);
        return false;
    }
    else
        return handler->LoadFile( this, stream, true/*verbose*/, index );
}

bool wxImage::SaveFile( wxOutputStream& stream, int type ) const
{
    wxCHECK_MSG( Ok(), false, wxT("invalid image") );

    wxImageHandler *handler = FindHandler(type);
    if ( !handler )
    {
        wxLogWarning( _("No image handler for type %d defined."), type );

        return false;
    }

    return handler->SaveFile( (wxImage*)this, stream );
}

bool wxImage::SaveFile( wxOutputStream& stream, const wxString& mimetype ) const
{
    wxCHECK_MSG( Ok(), false, wxT("invalid image") );

    wxImageHandler *handler = FindHandlerMime(mimetype);
    if ( !handler )
    {
        wxLogWarning( _("No image handler for type %s defined."), mimetype.GetData() );

        return false;
    }

    return handler->SaveFile( (wxImage*)this, stream );
}
#endif // wxUSE_STREAMS

// ----------------------------------------------------------------------------
// image I/O handlers
// ----------------------------------------------------------------------------

void wxImage::AddHandler( wxImageHandler *handler )
{
    // Check for an existing handler of the type being added.
    if (FindHandler( handler->GetType() ) == 0)
    {
        sm_handlers.Append( handler );
    }
    else
    {
        // This is not documented behaviour, merely the simplest 'fix'
        // for preventing duplicate additions.  If someone ever has
        // a good reason to add and remove duplicate handlers (and they
        // may) we should probably refcount the duplicates.
        //   also an issue in InsertHandler below.

        wxLogDebug( _T("Adding duplicate image handler for '%s'"),
                    handler->GetName().c_str() );
        delete handler;
    }
}

void wxImage::InsertHandler( wxImageHandler *handler )
{
    // Check for an existing handler of the type being added.
    if (FindHandler( handler->GetType() ) == 0)
    {
        sm_handlers.Insert( handler );
    }
    else
    {
        // see AddHandler for additional comments.
        wxLogDebug( _T("Inserting duplicate image handler for '%s'"),
                    handler->GetName().c_str() );
        delete handler;
    }
}

bool wxImage::RemoveHandler( const wxString& name )
{
    wxImageHandler *handler = FindHandler(name);
    if (handler)
    {
        sm_handlers.DeleteObject(handler);
        delete handler;
        return true;
    }
    else
        return false;
}

wxImageHandler *wxImage::FindHandler( const wxString& name )
{
    wxList::compatibility_iterator node = sm_handlers.GetFirst();
    while (node)
    {
        wxImageHandler *handler = (wxImageHandler*)node->GetData();
        if (handler->GetName().Cmp(name) == 0) return handler;

        node = node->GetNext();
    }
    return 0;
}

wxImageHandler *wxImage::FindHandler( const wxString& extension, long bitmapType )
{
    wxList::compatibility_iterator node = sm_handlers.GetFirst();
    while (node)
    {
        wxImageHandler *handler = (wxImageHandler*)node->GetData();
        if ( (handler->GetExtension().Cmp(extension) == 0) &&
            (bitmapType == -1 || handler->GetType() == bitmapType) )
            return handler;
        node = node->GetNext();
    }
    return 0;
}

wxImageHandler *wxImage::FindHandler( long bitmapType )
{
    wxList::compatibility_iterator node = sm_handlers.GetFirst();
    while (node)
    {
        wxImageHandler *handler = (wxImageHandler *)node->GetData();
        if (handler->GetType() == bitmapType) return handler;
        node = node->GetNext();
    }
    return 0;
}

wxImageHandler *wxImage::FindHandlerMime( const wxString& mimetype )
{
    wxList::compatibility_iterator node = sm_handlers.GetFirst();
    while (node)
    {
        wxImageHandler *handler = (wxImageHandler *)node->GetData();
        if (handler->GetMimeType().IsSameAs(mimetype, false)) return handler;
        node = node->GetNext();
    }
    return 0;
}

void wxImage::InitStandardHandlers()
{
#if wxUSE_STREAMS
    AddHandler(new wxBMPHandler);
#endif // wxUSE_STREAMS
}

void wxImage::CleanUpHandlers()
{
    wxList::compatibility_iterator node = sm_handlers.GetFirst();
    while (node)
    {
        wxImageHandler *handler = (wxImageHandler *)node->GetData();
        wxList::compatibility_iterator next = node->GetNext();
        delete handler;
        node = next;
    }

    sm_handlers.Clear();
}

wxString wxImage::GetImageExtWildcard()
{
    wxString fmts;

    wxList& Handlers = wxImage::GetHandlers();
    wxList::compatibility_iterator Node = Handlers.GetFirst();
    while ( Node )
    {
        wxImageHandler* Handler = (wxImageHandler*)Node->GetData();
        fmts += wxT("*.") + Handler->GetExtension();
        Node = Node->GetNext();
        if ( Node ) fmts += wxT(";");
    }

    return wxT("(") + fmts + wxT(")|") + fmts;
}

wxImage::HSVValue wxImage::RGBtoHSV(const RGBValue& rgb)
{
    const double red = rgb.red / 255.0,
                 green = rgb.green / 255.0,
                 blue = rgb.blue / 255.0;

    // find the min and max intensity (and remember which one was it for the
    // latter)
    double minimumRGB = red;
    if ( green < minimumRGB )
        minimumRGB = green;
    if ( blue < minimumRGB )
        minimumRGB = blue;

    enum { RED, GREEN, BLUE } chMax = RED;
    double maximumRGB = red;
    if ( green > maximumRGB )
    {
        chMax = GREEN;
        maximumRGB = green;
    }
    if ( blue > maximumRGB )
    {
        chMax = BLUE;
        maximumRGB = blue;
    }

    const double value = maximumRGB;

    double hue = 0.0, saturation;
    const double deltaRGB = maximumRGB - minimumRGB;
    if ( wxIsNullDouble(deltaRGB) )
    {
        // Gray has no color
        hue = 0.0;
        saturation = 0.0;
    }
    else
    {
        switch ( chMax )
        {
            case RED:
                hue = (green - blue) / deltaRGB;
                break;

            case GREEN:
                hue = 2.0 + (blue - red) / deltaRGB;
                break;

            case BLUE:
                hue = 4.0 + (red - green) / deltaRGB;
                break;

            default:
                wxFAIL_MSG(wxT("hue not specified"));
                break;
        }

        hue /= 6.0;

        if ( hue < 0.0 )
            hue += 1.0;

        saturation = deltaRGB / maximumRGB;
    }

    return HSVValue(hue, saturation, value);
}

wxImage::RGBValue wxImage::HSVtoRGB(const HSVValue& hsv)
{
    double red, green, blue;

    if ( wxIsNullDouble(hsv.saturation) )
    {
        // Grey
        red = hsv.value;
        green = hsv.value;
        blue = hsv.value;
    }
    else // not grey
    {
        double hue = hsv.hue * 6.0;      // sector 0 to 5
        int i = (int)floor(hue);
        double f = hue - i;          // fractional part of h
        double p = hsv.value * (1.0 - hsv.saturation);

        switch (i)
        {
            case 0:
                red = hsv.value;
                green = hsv.value * (1.0 - hsv.saturation * (1.0 - f));
                blue = p;
                break;

            case 1:
                red = hsv.value * (1.0 - hsv.saturation * f);
                green = hsv.value;
                blue = p;
                break;

            case 2:
                red = p;
                green = hsv.value;
                blue = hsv.value * (1.0 - hsv.saturation * (1.0 - f));
                break;

            case 3:
                red = p;
                green = hsv.value * (1.0 - hsv.saturation * f);
                blue = hsv.value;
                break;

            case 4:
                red = hsv.value * (1.0 - hsv.saturation * (1.0 - f));
                green = p;
                blue = hsv.value;
                break;

            default:    // case 5:
                red = hsv.value;
                green = p;
                blue = hsv.value * (1.0 - hsv.saturation * f);
                break;
        }
    }

    return RGBValue((unsigned char)(red * 255.0),
                    (unsigned char)(green * 255.0),
                    (unsigned char)(blue * 255.0));
}

/*
 * Rotates the hue of each pixel of the image. angle is a double in the range
 * -1.0..1.0 where -1.0 is -360 degrees and 1.0 is 360 degrees
 */
void wxImage::RotateHue(double angle)
{
    AllocExclusive();

    unsigned char *srcBytePtr;
    unsigned char *dstBytePtr;
    unsigned long count;
    wxImage::HSVValue hsv;
    wxImage::RGBValue rgb;

    wxASSERT (angle >= -1.0 && angle <= 1.0);
    count = M_IMGDATA->m_width * M_IMGDATA->m_height;
    if ( count > 0 && !wxIsNullDouble(angle) )
    {
        srcBytePtr = M_IMGDATA->m_data;
        dstBytePtr = srcBytePtr;
        do
        {
            rgb.red = *srcBytePtr++;
            rgb.green = *srcBytePtr++;
            rgb.blue = *srcBytePtr++;
            hsv = RGBtoHSV(rgb);

            hsv.hue = hsv.hue + angle;
            if (hsv.hue > 1.0)
                hsv.hue = hsv.hue - 1.0;
            else if (hsv.hue < 0.0)
                hsv.hue = hsv.hue + 1.0;

            rgb = HSVtoRGB(hsv);
            *dstBytePtr++ = rgb.red;
            *dstBytePtr++ = rgb.green;
            *dstBytePtr++ = rgb.blue;
        } while (--count != 0);
    }
}

//-----------------------------------------------------------------------------
// wxImageHandler
//-----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxImageHandler,wxObject)

#if wxUSE_STREAMS
bool wxImageHandler::LoadFile( wxImage *WXUNUSED(image), wxInputStream& WXUNUSED(stream), bool WXUNUSED(verbose), int WXUNUSED(index) )
{
    return false;
}

bool wxImageHandler::SaveFile( wxImage *WXUNUSED(image), wxOutputStream& WXUNUSED(stream), bool WXUNUSED(verbose) )
{
    return false;
}

int wxImageHandler::GetImageCount( wxInputStream& WXUNUSED(stream) )
{
    return 1;
}

bool wxImageHandler::CanRead( const wxString& name )
{
    if (wxFileExists(name))
    {
        wxImageFileInputStream stream(name);
        return CanRead(stream);
    }

    wxLogError( _("Can't check image format of file '%s': file does not exist."), name.c_str() );

    return false;
}

bool wxImageHandler::CallDoCanRead(wxInputStream& stream)
{
    wxFileOffset posOld = stream.TellI();
    if ( posOld == wxInvalidOffset )
    {
        // can't test unseekable stream
        return false;
    }

    bool ok = DoCanRead(stream);

    // restore the old position to be able to test other formats and so on
    if ( stream.SeekI(posOld) == wxInvalidOffset )
    {
        wxLogDebug(_T("Failed to rewind the stream in wxImageHandler!"));

        // reading would fail anyhow as we're not at the right position
        return false;
    }

    return ok;
}

#endif // wxUSE_STREAMS

// ----------------------------------------------------------------------------
// image histogram stuff
// ----------------------------------------------------------------------------

bool
wxImageHistogram::FindFirstUnusedColour(unsigned char *r,
                                        unsigned char *g,
                                        unsigned char *b,
                                        unsigned char r2,
                                        unsigned char b2,
                                        unsigned char g2) const
{
    unsigned long key = MakeKey(r2, g2, b2);

    while ( find(key) != end() )
    {
        // color already used
        r2++;
        if ( r2 >= 255 )
        {
            r2 = 0;
            g2++;
            if ( g2 >= 255 )
            {
                g2 = 0;
                b2++;
                if ( b2 >= 255 )
                {
                    wxLogError(_("No unused colour in image.") );
                    return false;
                }
            }
        }

        key = MakeKey(r2, g2, b2);
    }

    if ( r )
        *r = r2;
    if ( g )
        *g = g2;
    if ( b )
        *b = b2;

    return true;
}

bool
wxImage::FindFirstUnusedColour(unsigned char *r,
                               unsigned char *g,
                               unsigned char *b,
                               unsigned char r2,
                               unsigned char b2,
                               unsigned char g2) const
{
    wxImageHistogram histogram;

    ComputeHistogram(histogram);

    return histogram.FindFirstUnusedColour(r, g, b, r2, g2, b2);
}



// GRG, Dic/99
// Counts and returns the number of different colours. Optionally stops
// when it exceeds 'stopafter' different colours. This is useful, for
// example, to see if the image can be saved as 8-bit (256 colour or
// less, in this case it would be invoked as CountColours(256)). Default
// value for stopafter is -1 (don't care).
//
unsigned long wxImage::CountColours( unsigned long stopafter ) const
{
    wxHashTable h;
    wxObject dummy;
    unsigned char r, g, b;
    unsigned char *p;
    unsigned long size, nentries, key;

    p = GetData();
    size = GetWidth() * GetHeight();
    nentries = 0;

    for (unsigned long j = 0; (j < size) && (nentries <= stopafter) ; j++)
    {
        r = *(p++);
        g = *(p++);
        b = *(p++);
        key = wxImageHistogram::MakeKey(r, g, b);

        if (h.Get(key) == NULL)
        {
            h.Put(key, &dummy);
            nentries++;
        }
    }

    return nentries;
}


unsigned long wxImage::ComputeHistogram( wxImageHistogram &h ) const
{
    unsigned char *p = GetData();
    unsigned long nentries = 0;

    h.clear();

    const unsigned long size = GetWidth() * GetHeight();

    unsigned char r, g, b;
    for ( unsigned long n = 0; n < size; n++ )
    {
        r = *p++;
        g = *p++;
        b = *p++;

        wxImageHistogramEntry& entry = h[wxImageHistogram::MakeKey(r, g, b)];

        if ( entry.value++ == 0 )
            entry.index = nentries++;
    }

    return nentries;
}

/*
 * Rotation code by Carlos Moreno
 */

static const double wxROTATE_EPSILON = 1e-10;

// Auxiliary function to rotate a point (x,y) with respect to point p0
// make it inline and use a straight return to facilitate optimization
// also, the function receives the sine and cosine of the angle to avoid
// repeating the time-consuming calls to these functions -- sin/cos can
// be computed and stored in the calling function.

static inline wxRealPoint
wxRotatePoint(const wxRealPoint& p, double cos_angle, double sin_angle,
              const wxRealPoint& p0)
{
    return wxRealPoint(p0.x + (p.x - p0.x) * cos_angle - (p.y - p0.y) * sin_angle,
                       p0.y + (p.y - p0.y) * cos_angle + (p.x - p0.x) * sin_angle);
}

static inline wxRealPoint
wxRotatePoint(double x, double y, double cos_angle, double sin_angle,
              const wxRealPoint & p0)
{
    return wxRotatePoint (wxRealPoint(x,y), cos_angle, sin_angle, p0);
}

wxImage wxImage::Rotate(double angle, const wxPoint & centre_of_rotation, bool interpolating, wxPoint * offset_after_rotation) const
{
    int i;
    angle = -angle;     // screen coordinates are a mirror image of "real" coordinates

    bool has_alpha = HasAlpha();

    const int w = GetWidth(),
              h = GetHeight();

    // Create pointer-based array to accelerate access to wxImage's data
    unsigned char ** data = new unsigned char * [h];
    data[0] = GetData();
    for (i = 1; i < h; i++)
        data[i] = data[i - 1] + (3 * w);

    // Same for alpha channel
    unsigned char ** alpha = NULL;
    if (has_alpha)
    {
        alpha = new unsigned char * [h];
        alpha[0] = GetAlpha();
        for (i = 1; i < h; i++)
            alpha[i] = alpha[i - 1] + w;
    }

    // precompute coefficients for rotation formula
    // (sine and cosine of the angle)
    const double cos_angle = cos(angle);
    const double sin_angle = sin(angle);

    // Create new Image to store the result
    // First, find rectangle that covers the rotated image;  to do that,
    // rotate the four corners

    const wxRealPoint p0(centre_of_rotation.x, centre_of_rotation.y);

    wxRealPoint p1 = wxRotatePoint (0, 0, cos_angle, sin_angle, p0);
    wxRealPoint p2 = wxRotatePoint (0, h, cos_angle, sin_angle, p0);
    wxRealPoint p3 = wxRotatePoint (w, 0, cos_angle, sin_angle, p0);
    wxRealPoint p4 = wxRotatePoint (w, h, cos_angle, sin_angle, p0);

    int x1a = (int) floor (wxMin (wxMin(p1.x, p2.x), wxMin(p3.x, p4.x)));
    int y1a = (int) floor (wxMin (wxMin(p1.y, p2.y), wxMin(p3.y, p4.y)));
    int x2a = (int) ceil (wxMax (wxMax(p1.x, p2.x), wxMax(p3.x, p4.x)));
    int y2a = (int) ceil (wxMax (wxMax(p1.y, p2.y), wxMax(p3.y, p4.y)));

    // Create rotated image
    wxImage rotated (x2a - x1a + 1, y2a - y1a + 1, false);
    // With alpha channel
    if (has_alpha)
        rotated.SetAlpha();

    if (offset_after_rotation != NULL)
    {
        *offset_after_rotation = wxPoint (x1a, y1a);
    }

    // GRG: The rotated (destination) image is always accessed
    //      sequentially, so there is no need for a pointer-based
    //      array here (and in fact it would be slower).
    //
    unsigned char * dst = rotated.GetData();

    unsigned char * alpha_dst = NULL;
    if (has_alpha)
        alpha_dst = rotated.GetAlpha();

    // GRG: if the original image has a mask, use its RGB values
    //      as the blank pixel, else, fall back to default (black).
    //
    unsigned char blank_r = 0;
    unsigned char blank_g = 0;
    unsigned char blank_b = 0;

    if (HasMask())
    {
        blank_r = GetMaskRed();
        blank_g = GetMaskGreen();
        blank_b = GetMaskBlue();
        rotated.SetMaskColour( blank_r, blank_g, blank_b );
    }

    // Now, for each point of the rotated image, find where it came from, by
    // performing an inverse rotation (a rotation of -angle) and getting the
    // pixel at those coordinates

    const int rH = rotated.GetHeight();
    const int rW = rotated.GetWidth();

    // GRG: I've taken the (interpolating) test out of the loops, so that
    //      it is done only once, instead of repeating it for each pixel.

    if (interpolating)
    {
        for (int y = 0; y < rH; y++)
        {
            for (int x = 0; x < rW; x++)
            {
                wxRealPoint src = wxRotatePoint (x + x1a, y + y1a, cos_angle, -sin_angle, p0);

                if (-0.25 < src.x && src.x < w - 0.75 &&
                    -0.25 < src.y && src.y < h - 0.75)
                {
                    // interpolate using the 4 enclosing grid-points.  Those
                    // points can be obtained using floor and ceiling of the
                    // exact coordinates of the point
                    int x1, y1, x2, y2;

                    if (0 < src.x && src.x < w - 1)
                    {
                        x1 = wxRound(floor(src.x));
                        x2 = wxRound(ceil(src.x));
                    }
                    else    // else means that x is near one of the borders (0 or width-1)
                    {
                        x1 = x2 = wxRound (src.x);
                    }

                    if (0 < src.y && src.y < h - 1)
                    {
                        y1 = wxRound(floor(src.y));
                        y2 = wxRound(ceil(src.y));
                    }
                    else
                    {
                        y1 = y2 = wxRound (src.y);
                    }

                    // get four points and the distances (square of the distance,
                    // for efficiency reasons) for the interpolation formula

                    // GRG: Do not calculate the points until they are
                    //      really needed -- this way we can calculate
                    //      just one, instead of four, if d1, d2, d3
                    //      or d4 are < wxROTATE_EPSILON

                    const double d1 = (src.x - x1) * (src.x - x1) + (src.y - y1) * (src.y - y1);
                    const double d2 = (src.x - x2) * (src.x - x2) + (src.y - y1) * (src.y - y1);
                    const double d3 = (src.x - x2) * (src.x - x2) + (src.y - y2) * (src.y - y2);
                    const double d4 = (src.x - x1) * (src.x - x1) + (src.y - y2) * (src.y - y2);

                    // Now interpolate as a weighted average of the four surrounding
                    // points, where the weights are the distances to each of those points

                    // If the point is exactly at one point of the grid of the source
                    // image, then don't interpolate -- just assign the pixel

                    // d1,d2,d3,d4 are positive -- no need for abs()
                    if (d1 < wxROTATE_EPSILON)
                    {
                        unsigned char *p = data[y1] + (3 * x1);
                        *(dst++) = *(p++);
                        *(dst++) = *(p++);
                        *(dst++) = *p;

                        if (has_alpha)
                            *(alpha_dst++) = *(alpha[y1] + x1);
                    }
                    else if (d2 < wxROTATE_EPSILON)
                    {
                        unsigned char *p = data[y1] + (3 * x2);
                        *(dst++) = *(p++);
                        *(dst++) = *(p++);
                        *(dst++) = *p;

                        if (has_alpha)
                            *(alpha_dst++) = *(alpha[y1] + x2);
                    }
                    else if (d3 < wxROTATE_EPSILON)
                    {
                        unsigned char *p = data[y2] + (3 * x2);
                        *(dst++) = *(p++);
                        *(dst++) = *(p++);
                        *(dst++) = *p;

                        if (has_alpha)
                            *(alpha_dst++) = *(alpha[y2] + x2);
                    }
                    else if (d4 < wxROTATE_EPSILON)
                    {
                        unsigned char *p = data[y2] + (3 * x1);
                        *(dst++) = *(p++);
                        *(dst++) = *(p++);
                        *(dst++) = *p;

                        if (has_alpha)
                            *(alpha_dst++) = *(alpha[y2] + x1);
                    }
                    else
                    {
                        // weights for the weighted average are proportional to the inverse of the distance
                        unsigned char *v1 = data[y1] + (3 * x1);
                        unsigned char *v2 = data[y1] + (3 * x2);
                        unsigned char *v3 = data[y2] + (3 * x2);
                        unsigned char *v4 = data[y2] + (3 * x1);

                        const double w1 = 1/d1, w2 = 1/d2, w3 = 1/d3, w4 = 1/d4;

                        // GRG: Unrolled.

                        *(dst++) = (unsigned char)
                            ( (w1 * *(v1++) + w2 * *(v2++) +
                               w3 * *(v3++) + w4 * *(v4++)) /
                              (w1 + w2 + w3 + w4) );
                        *(dst++) = (unsigned char)
                            ( (w1 * *(v1++) + w2 * *(v2++) +
                               w3 * *(v3++) + w4 * *(v4++)) /
                              (w1 + w2 + w3 + w4) );
                        *(dst++) = (unsigned char)
                            ( (w1 * *v1 + w2 * *v2 +
                               w3 * *v3 + w4 * *v4) /
                              (w1 + w2 + w3 + w4) );

                        if (has_alpha)
                        {
                            v1 = alpha[y1] + (x1);
                            v2 = alpha[y1] + (x2);
                            v3 = alpha[y2] + (x2);
                            v4 = alpha[y2] + (x1);

                            *(alpha_dst++) = (unsigned char)
                                ( (w1 * *v1 + w2 * *v2 +
                                   w3 * *v3 + w4 * *v4) /
                                  (w1 + w2 + w3 + w4) );
                        }
                    }
                }
                else
                {
                    *(dst++) = blank_r;
                    *(dst++) = blank_g;
                    *(dst++) = blank_b;

                    if (has_alpha)
                        *(alpha_dst++) = 0;
                }
            }
        }
    }
    else    // not interpolating
    {
        for (int y = 0; y < rH; y++)
        {
            for (int x = 0; x < rW; x++)
            {
                wxRealPoint src = wxRotatePoint (x + x1a, y + y1a, cos_angle, -sin_angle, p0);

                const int xs = wxRound (src.x);      // wxRound rounds to the
                const int ys = wxRound (src.y);      // closest integer

                if (0 <= xs && xs < w && 0 <= ys && ys < h)
                {
                    unsigned char *p = data[ys] + (3 * xs);
                    *(dst++) = *(p++);
                    *(dst++) = *(p++);
                    *(dst++) = *p;

                    if (has_alpha)
                        *(alpha_dst++) = *(alpha[ys] + (xs));
                }
                else
                {
                    *(dst++) = blank_r;
                    *(dst++) = blank_g;
                    *(dst++) = blank_b;

                    if (has_alpha)
                        *(alpha_dst++) = 255;
                }
            }
        }
    }

    delete [] data;

    if (has_alpha)
        delete [] alpha;

    return rotated;
}





// A module to allow wxImage initialization/cleanup
// without calling these functions from app.cpp or from
// the user's application.

class wxImageModule: public wxModule
{
DECLARE_DYNAMIC_CLASS(wxImageModule)
public:
    wxImageModule() {}
    bool OnInit() { wxImage::InitStandardHandlers(); return true; }
    void OnExit() { wxImage::CleanUpHandlers(); }
};

IMPLEMENT_DYNAMIC_CLASS(wxImageModule, wxModule)


#endif // wxUSE_IMAGE
