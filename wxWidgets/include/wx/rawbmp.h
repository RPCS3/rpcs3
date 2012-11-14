///////////////////////////////////////////////////////////////////////////////
// Name:        wx/rawbmp.h
// Purpose:     macros for fast, raw bitmap data access
// Author:      Eric Kidd, Vadim Zeitlin
// Modified by:
// Created:     10.03.03
// RCS-ID:      $Id: rawbmp.h 41661 2006-10-06 16:34:45Z PC $
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_RAWBMP_H_BASE_
#define _WX_RAWBMP_H_BASE_

#include "wx/image.h"

// ----------------------------------------------------------------------------
// Abstract Pixel API
//
// We need to access our raw bitmap data (1) portably and (2) efficiently.
// We do this using a two-dimensional "iteration" interface.  Performance
// is extremely important here: these functions will be called hundreds
// of thousands of times in a row, and even small inefficiencies will
// make applications seem slow.
//
// We can't always rely on inline functions, because not all compilers actually
// bother to inline them unless we crank the optimization levels way up.
// Therefore, we also provide macros to wring maximum speed out of compiler
// unconditionally (e.g. even in debug builds). Of course, if the performance
// isn't absolutely crucial for you you shouldn't be using them but the inline
// functions instead.
// ----------------------------------------------------------------------------

/*
   Usage example:

    typedef wxPixelData<wxBitmap, wxNativePixelFormat> PixelData;

    wxBitmap bmp;
    PixelData data(bmp);
    if ( !data )
    {
        ... raw access to bitmap data unavailable, do something else ...
        return;
    }

    if ( data.GetWidth() < 20 || data.GetHeight() < 20 )
    {
        ... complain: the bitmap it too small ...
        return;
    }

    PixelData::Iterator p(data);

    // we draw a (10, 10)-(20, 20) rect manually using the given r, g, b
    p.Offset(data, 10, 10);

    for ( int y = 0; y < 10; ++y )
    {
        PixelData::Iterator rowStart = p;

        for ( int x = 0; x < 10; ++x, ++p )
        {
            p.Red() = r;
            p.Green() = g;
            p.Blue() = b;
        }

        p = rowStart;
        p.OffsetY(data, 1);
    }
 */

/*
    Note: we do not use WXDLLEXPORT with classes in this file because VC++ has
    problems with exporting inner class defined inside a specialization of a
    template class from a DLL. Besides, as all the methods are inline it's not
    really necessary to put them in DLL at all.
 */

// ----------------------------------------------------------------------------
// wxPixelFormat
// ----------------------------------------------------------------------------

/*
    wxPixelFormat is a template class describing the bitmap data format. It
    contains the constants describing the format of pixel data, but does not
    describe how the entire bitmap is stored (i.e. top-to-bottom,
    bottom-to-top, ...). It is also a "traits"-like class, i.e. it only
    contains some constants and maybe static methods but nothing more, so it
    can be safely used without incurring any overhead as all accesses to it are
    done at compile-time.

    Current limitations: we don't support RAGABA and ARAGAB formats supported
    by Mac OS X. If there is sufficient interest, these classes could be
    extended to deal with them. Neither do we support alpha channel having
    different representation from the RGB ones (happens under QNX/Photon I
    think), but again this could be achieved with some small extra effort.

    Template parameters are:
        - type of a single pixel component
        - size of the single pixel in bits
        - indices of red, green and blue pixel components inside the pixel
        - index of the alpha component or -1 if none
        - type which can contain the full pixel value (all channels)
 */

template <class Channel,
          size_t Bpp, int R, int G, int B, int A = -1,
          class Pixel = wxUint32>

struct wxPixelFormat
{
    // iterator over pixels is usually of type "ChannelType *"
    typedef Channel ChannelType;

    // the type which may hold the entire pixel value
    typedef Pixel PixelType;

    // NB: using static ints initialized inside the class declaration is not
    //     portable as it doesn't work with VC++ 6, so we must use enums

    // size of one pixel in bits
    enum { BitsPerPixel = Bpp };

    // size of one pixel in ChannelType units (usually bytes)
    enum { SizePixel = Bpp / (8 * sizeof(Channel)) };

    // the channels indices inside the pixel
    enum
    {
        RED = R,
        GREEN = G,
        BLUE = B,
        ALPHA = A
    };

    // true if we have an alpha channel (together with the other channels, this
    // doesn't cover the case of wxImage which stores alpha separately)
    enum { HasAlpha = A != -1 };
};

// some "predefined" pixel formats
// -------------------------------

// wxImage format is common to all platforms
typedef wxPixelFormat<unsigned char, 24, 0, 1, 2> wxImagePixelFormat;

// the (most common) native bitmap format without alpha support
#if defined(__WXMSW__)
    // under MSW the RGB components are reversed, they're in BGR order
    typedef wxPixelFormat<unsigned char, 24, 2, 1, 0> wxNativePixelFormat;

    #define wxPIXEL_FORMAT_ALPHA 3
#elif defined(__WXMAC__)
    // under Mac, first component is unused but still present, hence we use
    // 32bpp, not 24
    typedef wxPixelFormat<unsigned char, 32, 1, 2, 3> wxNativePixelFormat;

    #define wxPIXEL_FORMAT_ALPHA 0
#elif defined(__WXCOCOA__)
    // Cocoa is standard RGB or RGBA (normally it is RGBA)
    typedef wxPixelFormat<unsigned char, 24, 0, 1, 2> wxNativePixelFormat;

    #define wxPIXEL_FORMAT_ALPHA 3
#elif defined(__WXGTK__)
    // Under GTK+ 2.X we use GdkPixbuf, which is standard RGB or RGBA
    typedef wxPixelFormat<unsigned char, 24, 0, 1, 2> wxNativePixelFormat;

    #define wxPIXEL_FORMAT_ALPHA 3
#endif

// the (most common) native format for bitmaps with alpha channel
#ifdef wxPIXEL_FORMAT_ALPHA
    typedef wxPixelFormat<unsigned char, 32,
                          wxNativePixelFormat::RED,
                          wxNativePixelFormat::GREEN,
                          wxNativePixelFormat::BLUE,
                          wxPIXEL_FORMAT_ALPHA> wxAlphaPixelFormat;
#endif // wxPIXEL_FORMAT_ALPHA

// we also define the (default/best) pixel format for the given class: this is
// used as default value for the pixel format in wxPixelIterator template
template <class T> struct wxPixelFormatFor;

#if wxUSE_IMAGE
// wxPixelFormatFor is only defined for wxImage, attempt to use it with other
// classes (wxBitmap...) will result in compile errors which is exactly what we
// want
template <>
struct wxPixelFormatFor<wxImage>
{
    typedef wxImagePixelFormat Format;
};
#endif //wxUSE_IMAGE

// ----------------------------------------------------------------------------
// wxPixelData
// ----------------------------------------------------------------------------

/*
    wxPixelDataBase is just a helper for wxPixelData: it contains things common
    to both wxImage and wxBitmap specializations.
 */
class wxPixelDataBase
{
public:
    // origin of the rectangular region we represent
    wxPoint GetOrigin() const { return m_ptOrigin; }

    // width and height of the region we represent
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    wxSize GetSize() const { return wxSize(m_width, m_height); }

    // the distance between two rows
    int GetRowStride() const { return m_stride; }

// private: -- see comment in the beginning of the file

    // the origin of this image inside the bigger bitmap (usually (0, 0))
    wxPoint m_ptOrigin;

    // the size of the image we address, in pixels
    int m_width,
        m_height;

    // this parameter is the offset of the start of the (N+1)st row from the
    // Nth one and can be different from m_bypp*width in some cases:
    //  a) the most usual one is to force 32/64 bit alignment of rows
    //  b) another one is for bottom-to-top images where it's negative
    //  c) finally, it could conceivably be 0 for the images with all
    //     lines being identical
    int m_stride;

protected:
    // ctor is protected because this class is only meant to be used as the
    // base class by wxPixelData
    wxPixelDataBase()
    {
        m_width =
        m_height =
        m_stride = 0;
    }
};

/*
    wxPixelData represents the entire bitmap data, i.e. unlike
    wxPixelFormat (which it uses) it also stores the global bitmap
    characteristics such as its size, inter-row separation and so on.

    Because of this it can be used to move the pixel iterators (which don't
    have enough information about the bitmap themselves). This may seem a bit
    unnatural but must be done in this way to keep the iterator objects as
    small as possible for maximum efficiency as otherwise they wouldn't be put
    into the CPU registers by the compiler any more.

    Implementation note: we use the standard workaround for lack of partial
    template specialization support in VC (both 6 and 7): instead of partly
    specializing the class Foo<T, U> for some T we introduce FooOut<T> and
    FooIn<U> nested in it, make Foo<T, U> equivalent to FooOut<T>::FooIn<U> and
    fully specialize FooOut.

    Also note that this class doesn't have any default definition because we
    can't really do anything without knowing the exact image class. We do
    provide wxPixelDataBase to make it simpler to write new wxPixelData
    specializations.
 */

// we need to define this skeleton template to mollify VC++
template <class Image>
struct wxPixelDataOut
{
    template <class PixelFormat>
    class wxPixelDataIn
    {
    public:
        class Iterator { };
    };
};

#if wxUSE_IMAGE
// wxPixelData specialization for wxImage: this is the simplest case as we
// don't have to care about different pixel formats here
template <>
struct wxPixelDataOut<wxImage>
{
    // NB: this is a template class even though it doesn't use its template
    //     parameter because otherwise wxPixelData couldn't compile
    template <class dummyPixelFormat>
    class wxPixelDataIn : public wxPixelDataBase
    {
    public:
        // the type of the class we're working with
        typedef wxImage ImageType;

        // the iterator which should be used for working with data in this
        // format
        class Iterator
        {
        public:
            // the pixel format we use
            typedef wxImagePixelFormat PixelFormat;

            // the type of the pixel components
            typedef typename dummyPixelFormat::ChannelType ChannelType;

            // the pixel data we're working with
            typedef
                wxPixelDataOut<wxImage>::wxPixelDataIn<PixelFormat> PixelData;

            // go back to (0, 0)
            void Reset(const PixelData& data)
            {
                *this = data.GetPixels();
            }

            // creates the iterator pointing to the beginning of data
            Iterator(PixelData& data)
            {
                Reset(data);
            }

            // creates the iterator initially pointing to the image origin
            Iterator(const wxImage& image)
            {
                m_pRGB = image.GetData();

                if ( image.HasAlpha() )
                {
                    m_pAlpha = image.GetAlpha();
                }
                else // alpha is not used at all
                {
                    m_pAlpha = NULL;
                }
            }

            // true if the iterator is valid
            bool IsOk() const { return m_pRGB != NULL; }


            // navigation
            // ----------

            // advance the iterator to the next pixel, prefix version
            Iterator& operator++()
            {
                m_pRGB += PixelFormat::SizePixel;
                if ( m_pAlpha )
                    ++m_pAlpha;

                return *this;
            }

            // postfix (hence less efficient -- don't use it unless you
            // absolutely must) version
            Iterator operator++(int)
            {
                Iterator p(*this);
                ++*this;
                return p;
            }

            // move x pixels to the right and y down
            //
            // note that the rows don't wrap!
            void Offset(const PixelData& data, int x, int y)
            {
                m_pRGB += data.GetRowStride()*y + PixelFormat::SizePixel*x;
                if ( m_pAlpha )
                    m_pAlpha += data.GetWidth() + x;
            }

            // move x pixels to the right (again, no row wrapping)
            void OffsetX(const PixelData& WXUNUSED(data), int x)
            {
                m_pRGB += PixelFormat::SizePixel*x;
                if ( m_pAlpha )
                    m_pAlpha += x;
            }

            // move y rows to the bottom
            void OffsetY(const PixelData& data, int y)
            {
                m_pRGB += data.GetRowStride()*y;
                if ( m_pAlpha )
                    m_pAlpha += data.GetWidth();
            }

            // go to the given position
            void MoveTo(const PixelData& data, int x, int y)
            {
                Reset(data);
                Offset(data, x, y);
            }


            // data access
            // -----------

            // access to invidividual colour components
            ChannelType& Red() { return m_pRGB[PixelFormat::RED]; }
            ChannelType& Green() { return m_pRGB[PixelFormat::GREEN]; }
            ChannelType& Blue() { return m_pRGB[PixelFormat::BLUE]; }
            ChannelType& Alpha() { return *m_pAlpha; }

        // private: -- see comment in the beginning of the file

            // pointer into RGB buffer
            unsigned char *m_pRGB;

            // pointer into alpha buffer or NULL if alpha isn't used
            unsigned char *m_pAlpha;
        };

        // initializes us with the data of the given image
        wxPixelDataIn(ImageType& image) : m_image(image), m_pixels(image)
        {
            m_width = image.GetWidth();
            m_height = image.GetHeight();
            m_stride = Iterator::SizePixel * m_width;
        }

        // initializes us with the given region of the specified image
        wxPixelDataIn(ImageType& image,
                      const wxPoint& pt,
                      const wxSize& sz) : m_image(image), m_pixels(image)
        {
            m_stride = Iterator::SizePixel * m_width;

            InitRect(pt, sz);
        }

        // initializes us with the given region of the specified image
        wxPixelDataIn(ImageType& image,
                      const wxRect& rect) : m_image(image), m_pixels(image)
        {
            m_stride = Iterator::SizePixel * m_width;

            InitRect(rect.GetPosition(), rect.GetSize());
        }

        // we evaluate to true only if we could get access to bitmap data
        // successfully
        operator bool() const { return m_pixels.IsOk(); }

        // get the iterator pointing to the origin
        Iterator GetPixels() const { return m_pixels; }

    private:
        void InitRect(const wxPoint& pt, const wxSize& sz)
        {
            m_width = sz.x;
            m_height = sz.y;

            m_ptOrigin = pt;
            m_pixels.Offset(*this, pt.x, pt.y);
        }

        // the image we're working with
        ImageType& m_image;

        // the iterator pointing to the image origin
        Iterator m_pixels;
    };
};
#endif //wxUSE_IMAGE

#if wxUSE_GUI
// wxPixelData specialization for wxBitmap: here things are more interesting as
// we also have to support different pixel formats
template <>
struct wxPixelDataOut<wxBitmap>
{
    template <class Format>
    class wxPixelDataIn : public wxPixelDataBase
    {
    public:
        // the type of the class we're working with
        typedef wxBitmap ImageType;

        class Iterator
        {
        public:
            // the pixel format we use
            typedef Format PixelFormat;

            // the type of the pixel components
            typedef typename PixelFormat::ChannelType ChannelType;

            // the pixel data we're working with
            typedef wxPixelDataOut<wxBitmap>::wxPixelDataIn<Format> PixelData;


            // go back to (0, 0)
            void Reset(const PixelData& data)
            {
                *this = data.GetPixels();
            }

            // initializes the iterator to point to the origin of the given
            // pixel data
            Iterator(PixelData& data)
            {
                Reset(data);
            }

            // initializes the iterator to point to the origin of the given
            // bitmap
            Iterator(wxBitmap& bmp, PixelData& data)
            {
                // using cast here is ugly but it should be safe as
                // GetRawData() real return type should be consistent with
                // BitsPerPixel (which is in turn defined by ChannelType) and
                // this is the only thing we can do without making GetRawData()
                // a template function which is undesirable
                m_ptr = (ChannelType *)
                            bmp.GetRawData(data, PixelFormat::BitsPerPixel);
            }

            // default constructor
            Iterator()
            {
                m_ptr = NULL;
            }
            
            // return true if this iterator is valid
            bool IsOk() const { return m_ptr != NULL; }


            // navigation
            // ----------

            // advance the iterator to the next pixel, prefix version
            Iterator& operator++()
            {
                m_ptr += PixelFormat::SizePixel;

                return *this;
            }

            // postfix (hence less efficient -- don't use it unless you
            // absolutely must) version
            Iterator operator++(int)
            {
                Iterator p(*this);
                ++*this;
                return p;
            }

            // move x pixels to the right and y down
            //
            // note that the rows don't wrap!
            void Offset(const PixelData& data, int x, int y)
            {
                m_ptr += data.GetRowStride()*y + PixelFormat::SizePixel*x;
            }

            // move x pixels to the right (again, no row wrapping)
            void OffsetX(const PixelData& WXUNUSED(data), int x)
            {
                m_ptr += PixelFormat::SizePixel*x;
            }

            // move y rows to the bottom
            void OffsetY(const PixelData& data, int y)
            {
                m_ptr += data.GetRowStride()*y;
            }

            // go to the given position
            void MoveTo(const PixelData& data, int x, int y)
            {
                Reset(data);
                Offset(data, x, y);
            }


            // data access
            // -----------

            // access to invidividual colour components
            ChannelType& Red() { return m_ptr[PixelFormat::RED]; }
            ChannelType& Green() { return m_ptr[PixelFormat::GREEN]; }
            ChannelType& Blue() { return m_ptr[PixelFormat::BLUE]; }
            ChannelType& Alpha() { return m_ptr[PixelFormat::ALPHA]; }

            // address the pixel contents directly
            //
            // warning: the format is platform dependent
            typename PixelFormat::PixelType& Data()
                { return *(typename PixelFormat::PixelType *)m_ptr; }

        // private: -- see comment in the beginning of the file

            // for efficiency reasons this class should not have any other
            // fields, otherwise it won't be put into a CPU register (as it
            // should inside the inner loops) by some compilers, notably gcc
            ChannelType *m_ptr;
        };

        // ctor associates this pointer with a bitmap and locks the bitmap for
        // raw access, it will be unlocked only by our dtor and so these
        // objects should normally be only created on the stack, i.e. have
        // limited life-time
        wxPixelDataIn(wxBitmap& bmp) : m_bmp(bmp), m_pixels(bmp, *this)
        {
        }

        wxPixelDataIn(wxBitmap& bmp, const wxRect& rect)
            : m_bmp(bmp), m_pixels(bmp, *this)
        {
            InitRect(rect.GetPosition(), rect.GetSize());
        }

        wxPixelDataIn(wxBitmap& bmp, const wxPoint& pt, const wxSize& sz)
            : m_bmp(bmp), m_pixels(bmp, *this)
        {
            InitRect(pt, sz);
        }

        // we evaluate to true only if we could get access to bitmap data
        // successfully
        operator bool() const { return m_pixels.IsOk(); }

        // get the iterator pointing to the origin
        Iterator GetPixels() const { return m_pixels; }

        // dtor unlocks the bitmap
        ~wxPixelDataIn()
        {
            m_bmp.UngetRawData(*this);
        }

        // call this to indicate that we should use the alpha channel
        void UseAlpha() { m_bmp.UseAlpha(); }

    // private: -- see comment in the beginning of the file

        // the bitmap we're associated with
        wxBitmap m_bmp;

        // the iterator pointing to the image origin
        Iterator m_pixels;

    private:
        void InitRect(const wxPoint& pt, const wxSize& sz)
        {
            m_pixels.Offset(*this, pt.x, pt.y);

            m_ptOrigin = pt;
            m_width = sz.x;
            m_height = sz.y;
        }
    };
};
#endif //wxUSE_GUI

template <class Image, class PixelFormat = wxPixelFormatFor<Image> >
class wxPixelData :
    public wxPixelDataOut<Image>::template wxPixelDataIn<PixelFormat>
{
public:
    typedef
        typename wxPixelDataOut<Image>::template wxPixelDataIn<PixelFormat>
        Base;

    wxPixelData(Image& image) : Base(image) { }

    wxPixelData(Image& i, const wxRect& rect) : Base(i, rect) { }

    wxPixelData(Image& i, const wxPoint& pt, const wxSize& sz)
        : Base(i, pt, sz)
    {
    }
};


// some "predefined" pixel data classes
#if wxUSE_IMAGE
typedef wxPixelData<wxImage> wxImagePixelData;
#endif //wxUSE_IMAGE
#if wxUSE_GUI
typedef wxPixelData<wxBitmap, wxNativePixelFormat> wxNativePixelData;
typedef wxPixelData<wxBitmap, wxAlphaPixelFormat> wxAlphaPixelData;

#endif //wxUSE_GUI

// ----------------------------------------------------------------------------
// wxPixelIterator
// ----------------------------------------------------------------------------

/*
    wxPixel::Iterator represents something which points to the pixel data and
    allows us to iterate over it. In the simplest case of wxBitmap it is,
    indeed, just a pointer, but it can be something more complicated and,
    moreover, you are free to specialize it for other image classes and bitmap
    formats.

    Note that although it would have been much more intuitive to have a real
    class here instead of what we have now, this class would need two template
    parameters, and this can't be done because we'd need compiler support for
    partial template specialization then and neither VC6 nor VC7 provide it.
 */
template < class Image, class PixelFormat = wxPixelFormatFor<Image> >
struct wxPixelIterator : public wxPixelData<Image, PixelFormat>::Iterator
{
};

#endif // _WX_RAWBMP_H_BASE_

