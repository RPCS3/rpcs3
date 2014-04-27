///////////////////////////////////////////////////////////////////////////////
// Name:        wx/dataobj.h
// Purpose:     common data object classes
// Author:      Vadim Zeitlin, Robert Roebling
// Modified by:
// Created:     26.05.99
// RCS-ID:      $Id: dataobj.h 40772 2006-08-23 13:38:45Z VZ $
// Copyright:   (c) wxWidgets Team
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DATAOBJ_H_BASE_
#define _WX_DATAOBJ_H_BASE_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#include "wx/defs.h"

#if wxUSE_DATAOBJ

#include "wx/string.h"
#include "wx/bitmap.h"
#include "wx/list.h"
#include "wx/arrstr.h"

// ============================================================================
/*
   Generic data transfer related classes. The class hierarchy is as follows:

                                - wxDataObject-
                               /               \
                              /                 \
            wxDataObjectSimple                  wxDataObjectComposite
           /           |      \
          /            |       \
   wxTextDataObject    |     wxBitmapDataObject
                       |
               wxCustomDataObject

*/
// ============================================================================

// ----------------------------------------------------------------------------
// wxDataFormat class is declared in platform-specific headers: it represents
// a format for data which may be either one of the standard ones (text,
// bitmap, ...) or a custom one which is then identified by a unique string.
// ----------------------------------------------------------------------------

/* the class interface looks like this (pseudo code):

class wxDataFormat
{
public:
    typedef <integral type> NativeFormat;

    wxDataFormat(NativeFormat format = wxDF_INVALID);
    wxDataFormat(const wxChar *format);

    wxDataFormat& operator=(NativeFormat format);
    wxDataFormat& operator=(const wxDataFormat& format);

    bool operator==(NativeFormat format) const;
    bool operator!=(NativeFormat format) const;

    void SetType(NativeFormat format);
    NativeFormat GetType() const;

    wxString GetId() const;
    void SetId(const wxChar *format);
};

*/

#if defined(__WXMSW__)
    #include "wx/msw/ole/dataform.h"
#elif defined(__WXMOTIF__)
    #include "wx/motif/dataform.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/dataform.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/dataform.h"
#elif defined(__WXX11__)
    #include "wx/x11/dataform.h"
#elif defined(__WXMAC__)
    #include "wx/mac/dataform.h"
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/dataform.h"
#elif defined(__WXPM__)
    #include "wx/os2/dataform.h"
#endif

// the value for default argument to some functions (corresponds to
// wxDF_INVALID)
extern WXDLLEXPORT const wxDataFormat& wxFormatInvalid;

// ----------------------------------------------------------------------------
// wxDataObject represents a piece of data which knows which formats it
// supports and knows how to render itself in each of them - GetDataHere(),
// and how to restore data from the buffer (SetData()).
//
// Although this class may be used directly (i.e. custom classes may be
// derived from it), in many cases it might be simpler to use either
// wxDataObjectSimple or wxDataObjectComposite classes.
//
// A data object may be "read only", i.e. support only GetData() functions or
// "read-write", i.e. support both GetData() and SetData() (in principle, it
// might be "write only" too, but this is rare). Moreover, it doesn't have to
// support the same formats in Get() and Set() directions: for example, a data
// object containing JPEG image might accept BMPs in GetData() because JPEG
// image may be easily transformed into BMP but not in SetData(). Accordingly,
// all methods dealing with formats take an additional "direction" argument
// which is either SET or GET and which tells the function if the format needs
// to be supported by SetData() or GetDataHere().
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxDataObjectBase
{
public:
    enum Direction
    {
        Get  = 0x01,    // format is supported by GetDataHere()
        Set  = 0x02,    // format is supported by SetData()
        Both = 0x03     // format is supported by both (unused currently)
    };

    // this class is polymorphic, hence it needs a virtual dtor
    virtual ~wxDataObjectBase();

    // get the best suited format for rendering our data
    virtual wxDataFormat GetPreferredFormat(Direction dir = Get) const = 0;

    // get the number of formats we support
    virtual size_t GetFormatCount(Direction dir = Get) const = 0;

    // return all formats in the provided array (of size GetFormatCount())
    virtual void GetAllFormats(wxDataFormat *formats,
                               Direction dir = Get) const = 0;

    // get the (total) size of data for the given format
    virtual size_t GetDataSize(const wxDataFormat& format) const = 0;

    // copy raw data (in the specified format) to the provided buffer, return
    // true if data copied successfully, false otherwise
    virtual bool GetDataHere(const wxDataFormat& format, void *buf) const = 0;

    // get data from the buffer of specified length (in the given format),
    // return true if the data was read successfully, false otherwise
    virtual bool SetData(const wxDataFormat& WXUNUSED(format),
                         size_t WXUNUSED(len), const void * WXUNUSED(buf))
    {
        return false;
    }

    // returns true if this format is supported
    bool IsSupported(const wxDataFormat& format, Direction dir = Get) const;
};

// ----------------------------------------------------------------------------
// include the platform-specific declarations of wxDataObject
// ----------------------------------------------------------------------------

#if defined(__WXMSW__)
    #include "wx/msw/ole/dataobj.h"
#elif defined(__WXMOTIF__)
    #include "wx/motif/dataobj.h"
#elif defined(__WXX11__)
    #include "wx/x11/dataobj.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/dataobj.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/dataobj.h"
#elif defined(__WXMAC__)
    #include "wx/mac/dataobj.h"
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/dataobj.h"
#elif defined(__WXPM__)
    #include "wx/os2/dataobj.h"
#endif

// ----------------------------------------------------------------------------
// wxDataObjectSimple is a wxDataObject which only supports one format (in
// both Get and Set directions, but you may return false from GetDataHere() or
// SetData() if one of them is not supported). This is the simplest possible
// wxDataObject implementation.
//
// This is still an "abstract base class" (although it doesn't have any pure
// virtual functions), to use it you should derive from it and implement
// GetDataSize(), GetDataHere() and SetData() functions because the base class
// versions don't do anything - they just return "not implemented".
//
// This class should be used when you provide data in only one format (no
// conversion to/from other formats), either a standard or a custom one.
// Otherwise, you should use wxDataObjectComposite or wxDataObject directly.
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxDataObjectSimple : public wxDataObject
{
public:
    // ctor takes the format we support, but it can also be set later with
    // SetFormat()
    wxDataObjectSimple(const wxDataFormat& format = wxFormatInvalid)
        : m_format(format)
        {
        }

    // get/set the format we support
    const wxDataFormat& GetFormat() const { return m_format; }
    void SetFormat(const wxDataFormat& format) { m_format = format; }

    // virtual functions to override in derived class (the base class versions
    // just return "not implemented")
    // -----------------------------------------------------------------------

    // get the size of our data
    virtual size_t GetDataSize() const
        { return 0; }

    // copy our data to the buffer
    virtual bool GetDataHere(void *WXUNUSED(buf)) const
        { return false; }

    // copy data from buffer to our data
    virtual bool SetData(size_t WXUNUSED(len), const void *WXUNUSED(buf))
        { return false; }

    // implement base class pure virtuals
    // ----------------------------------
    virtual wxDataFormat GetPreferredFormat(wxDataObjectBase::Direction WXUNUSED(dir) = Get) const
        { return m_format; }
    virtual size_t GetFormatCount(wxDataObjectBase::Direction WXUNUSED(dir) = Get) const
        { return 1; }
    virtual void GetAllFormats(wxDataFormat *formats,
                               wxDataObjectBase::Direction WXUNUSED(dir) = Get) const
        { *formats = m_format; }
    virtual size_t GetDataSize(const wxDataFormat& WXUNUSED(format)) const
        { return GetDataSize(); }
    virtual bool GetDataHere(const wxDataFormat& WXUNUSED(format),
                             void *buf) const
        { return GetDataHere(buf); }
    virtual bool SetData(const wxDataFormat& WXUNUSED(format),
                         size_t len, const void *buf)
        { return SetData(len, buf); }

private:
    // the one and only format we support
    wxDataFormat m_format;

    DECLARE_NO_COPY_CLASS(wxDataObjectSimple)
};

// ----------------------------------------------------------------------------
// wxDataObjectComposite is the simplest way to implement wxDataObject
// supporting multiple formats. It contains several wxDataObjectSimple and
// supports all formats supported by any of them.
//
// This class shouldn't be (normally) derived from, but may be used directly.
// If you need more flexibility than what it provides, you should probably use
// wxDataObject directly.
// ----------------------------------------------------------------------------

WX_DECLARE_EXPORTED_LIST(wxDataObjectSimple, wxSimpleDataObjectList);

class WXDLLEXPORT wxDataObjectComposite : public wxDataObject
{
public:
    // ctor
    wxDataObjectComposite();
    virtual ~wxDataObjectComposite();

    // add data object (it will be deleted by wxDataObjectComposite, hence it
    // must be allocated on the heap) whose format will become the preferred
    // one if preferred == true
    void Add(wxDataObjectSimple *dataObject, bool preferred = false);

    // Report the format passed to the SetData method.  This should be the
    // format of the data object within the composite that recieved data from
    // the clipboard or the DnD operation.  You can use this method to find
    // out what kind of data object was recieved.
    wxDataFormat GetReceivedFormat() const;

    // implement base class pure virtuals
    // ----------------------------------
    virtual wxDataFormat GetPreferredFormat(wxDataObjectBase::Direction dir = Get) const;
    virtual size_t GetFormatCount(wxDataObjectBase::Direction dir = Get) const;
    virtual void GetAllFormats(wxDataFormat *formats, wxDataObjectBase::Direction dir = Get) const;
    virtual size_t GetDataSize(const wxDataFormat& format) const;
    virtual bool GetDataHere(const wxDataFormat& format, void *buf) const;
    virtual bool SetData(const wxDataFormat& format, size_t len, const void *buf);
#if defined(__WXMSW__)
    virtual const void* GetSizeFromBuffer( const void* buffer, size_t* size,
                                           const wxDataFormat& format );
    virtual void* SetSizeInBuffer( void* buffer, size_t size,
                                   const wxDataFormat& format );
    virtual size_t GetBufferOffset( const wxDataFormat& format );
#endif

protected:
    // returns the pointer to the object which supports this format or NULL
    wxDataObjectSimple *GetObject(const wxDataFormat& format) const;

private:
    // the list of all (simple) data objects whose formats we support
    wxSimpleDataObjectList m_dataObjects;

    // the index of the preferred one (0 initially, so by default the first
    // one is the preferred)
    size_t m_preferred;

    wxDataFormat m_receivedFormat;

    DECLARE_NO_COPY_CLASS(wxDataObjectComposite)
};

// ============================================================================
// Standard implementations of wxDataObjectSimple which can be used directly
// (i.e. without having to derive from them) for standard data type transfers.
//
// Note that although all of them can work with provided data, you can also
// override their virtual GetXXX() functions to only provide data on demand.
// ============================================================================

// ----------------------------------------------------------------------------
// wxTextDataObject contains text data
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxTextDataObject : public wxDataObjectSimple
{
public:
    // ctor: you can specify the text here or in SetText(), or override
    // GetText()
    wxTextDataObject(const wxString& text = wxEmptyString)
        : wxDataObjectSimple(
#if wxUSE_UNICODE
                             wxDF_UNICODETEXT
#else
                             wxDF_TEXT
#endif
                            ),
          m_text(text)
        {
        }

    // virtual functions which you may override if you want to provide text on
    // demand only - otherwise, the trivial default versions will be used
    virtual size_t GetTextLength() const { return m_text.Len() + 1; }
    virtual wxString GetText() const { return m_text; }
    virtual void SetText(const wxString& text) { m_text = text; }

    // implement base class pure virtuals
    // ----------------------------------

    // some platforms have 2 and not 1 format for text data
#if wxUSE_UNICODE && (defined(__WXGTK20__) || defined(__WXMAC__))
    virtual size_t GetFormatCount(Direction WXUNUSED(dir) = Get) const { return 2; }
    virtual void GetAllFormats(wxDataFormat *formats,
                               wxDataObjectBase::Direction WXUNUSED(dir) = Get) const;

    virtual size_t GetDataSize() const { return GetDataSize(GetPreferredFormat()); }
    virtual bool GetDataHere(void *buf) const { return GetDataHere(GetPreferredFormat(), buf); }
    virtual bool SetData(size_t len, const void *buf) { return SetData(GetPreferredFormat(), len, buf); }

    size_t GetDataSize(const wxDataFormat& format) const;
    bool GetDataHere(const wxDataFormat& format, void *pBuf) const;
    bool SetData(const wxDataFormat& format, size_t nLen, const void* pBuf);
#else
    virtual size_t GetDataSize() const;
    virtual bool GetDataHere(void *buf) const;
    virtual bool SetData(size_t len, const void *buf);
    // Must provide overloads to avoid hiding them (and warnings about it)
    virtual size_t GetDataSize(const wxDataFormat&) const
    {
        return GetDataSize();
    }
    virtual bool GetDataHere(const wxDataFormat&, void *buf) const
    {
        return GetDataHere(buf);
    }
    virtual bool SetData(const wxDataFormat&, size_t len, const void *buf)
    {
        return SetData(len, buf);
    }
#endif

private:
    wxString m_text;

    DECLARE_NO_COPY_CLASS(wxTextDataObject)
};

// ----------------------------------------------------------------------------
// wxBitmapDataObject contains a bitmap
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxBitmapDataObjectBase : public wxDataObjectSimple
{
public:
    // ctor: you can specify the bitmap here or in SetBitmap(), or override
    // GetBitmap()
    wxBitmapDataObjectBase(const wxBitmap& bitmap = wxNullBitmap)
        : wxDataObjectSimple(wxDF_BITMAP), m_bitmap(bitmap)
        {
        }

    // virtual functions which you may override if you want to provide data on
    // demand only - otherwise, the trivial default versions will be used
    virtual wxBitmap GetBitmap() const { return m_bitmap; }
    virtual void SetBitmap(const wxBitmap& bitmap) { m_bitmap = bitmap; }

protected:
    wxBitmap m_bitmap;

    DECLARE_NO_COPY_CLASS(wxBitmapDataObjectBase)
};

// ----------------------------------------------------------------------------
// wxFileDataObject contains a list of filenames
//
// NB: notice that this is a "write only" object, it can only be filled with
//     data from drag and drop operation.
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxFileDataObjectBase : public wxDataObjectSimple
{
public:
    // ctor: use AddFile() later to fill the array
    wxFileDataObjectBase() : wxDataObjectSimple(wxDF_FILENAME) { }

    // get a reference to our array
    const wxArrayString& GetFilenames() const { return m_filenames; }

protected:
    wxArrayString m_filenames;

    DECLARE_NO_COPY_CLASS(wxFileDataObjectBase)
};

// ----------------------------------------------------------------------------
// wxCustomDataObject contains arbitrary untyped user data.
//
// It is understood that this data can be copied bitwise.
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxCustomDataObject : public wxDataObjectSimple
{
public:
    // if you don't specify the format in the ctor, you can still use
    // SetFormat() later
    wxCustomDataObject(const wxDataFormat& format = wxFormatInvalid);

    // the dtor calls Free()
    virtual ~wxCustomDataObject();

    // you can call SetData() to set m_data: it will make a copy of the data
    // you pass - or you can use TakeData() which won't copy anything, but
    // will take ownership of data (i.e. will call Free() on it later)
    void TakeData(size_t size, void *data);

    // this function is called to allocate "size" bytes of memory from
    // SetData(). The default version uses operator new[].
    virtual void *Alloc(size_t size);

    // this function is called when the data is freed, you may override it to
    // anything you want (or may be nothing at all). The default version calls
    // operator delete[] on m_data
    virtual void Free();

    // get data: you may override these functions if you wish to provide data
    // only when it's requested
    virtual size_t GetSize() const { return m_size; }
    virtual void *GetData() const { return m_data; }

    // implement base class pure virtuals
    // ----------------------------------
    virtual size_t GetDataSize() const;
    virtual bool GetDataHere(void *buf) const;
    virtual bool SetData(size_t size, const void *buf);
    // Must provide overloads to avoid hiding them (and warnings about it)
    virtual size_t GetDataSize(const wxDataFormat&) const
    {
        return GetDataSize();
    }
    virtual bool GetDataHere(const wxDataFormat&, void *buf) const
    {
        return GetDataHere(buf);
    }
    virtual bool SetData(const wxDataFormat&, size_t len, const void *buf)
    {
        return SetData(len, buf);
    }

private:
    size_t m_size;
    void  *m_data;

    DECLARE_NO_COPY_CLASS(wxCustomDataObject)
};

// ----------------------------------------------------------------------------
// include platform-specific declarations of wxXXXBase classes
// ----------------------------------------------------------------------------

#if defined(__WXMSW__)
    #include "wx/msw/ole/dataobj2.h"

    // wxURLDataObject defined in msw/ole/dataobj2.h
#else // !__WXMSW__
    #if defined(__WXGTK20__)
        #include "wx/gtk/dataobj2.h"
    #elif defined(__WXGTK__)
        #include "wx/gtk1/dataobj2.h"
    #elif defined(__WXX11__)
        #include "wx/x11/dataobj2.h"
    #elif defined(__WXMOTIF__)
        #include "wx/motif/dataobj2.h"
    #elif defined(__WXMAC__)
        #include "wx/mac/dataobj2.h"
    #elif defined(__WXCOCOA__)
        #include "wx/cocoa/dataobj2.h"
    #elif defined(__WXPM__)
        #include "wx/os2/dataobj2.h"
    #endif

    // wxURLDataObject is simply wxTextDataObject with a different name
    class WXDLLEXPORT wxURLDataObject : public wxTextDataObject
    {
    public:
        wxURLDataObject(const wxString& url = wxEmptyString)
            : wxTextDataObject(url)
        {
        }

        wxString GetURL() const { return GetText(); }
        void SetURL(const wxString& url) { SetText(url); }
    };
#endif // __WXMSW__/!__WXMSW__

#endif // wxUSE_DATAOBJ

#endif // _WX_DATAOBJ_H_BASE_
