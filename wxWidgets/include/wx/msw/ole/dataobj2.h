///////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/ole/dataobj2.h
// Purpose:     second part of platform specific wxDataObject header -
//              declarations of predefined wxDataObjectSimple-derived classes
// Author:      Vadim Zeitlin
// Modified by:
// Created:     21.10.99
// RCS-ID:      $Id: dataobj2.h 40772 2006-08-23 13:38:45Z VZ $
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_OLE_DATAOBJ2_H
#define _WX_MSW_OLE_DATAOBJ2_H

// ----------------------------------------------------------------------------
// wxBitmapDataObject is a specialization of wxDataObject for bitmap data
//
// NB: in fact, under MSW we support CF_DIB (and not CF_BITMAP) clipboard
//     format and we also provide wxBitmapDataObject2 for CF_BITMAP (which is
//     rarely used). This is ugly, but I haven't found a solution for it yet.
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxBitmapDataObject : public wxBitmapDataObjectBase
{
public:
    // ctors
    wxBitmapDataObject(const wxBitmap& bitmap = wxNullBitmap)
        : wxBitmapDataObjectBase(bitmap)
        {
            SetFormat(wxDF_DIB);

            m_data = NULL;
        }

    // implement base class pure virtuals
    virtual size_t GetDataSize() const;
    virtual bool GetDataHere(void *buf) const;
    virtual bool SetData(size_t len, const void *buf);

    virtual size_t GetDataSize(const wxDataFormat& WXUNUSED(format)) const
        { return GetDataSize(); }
    virtual bool GetDataHere(const wxDataFormat& WXUNUSED(format),
                             void *buf) const
        { return GetDataHere(buf); }
    virtual bool SetData(const wxDataFormat& WXUNUSED(format),
                         size_t len, const void *buf)
        { return SetData(len, buf); }

private:
    // the DIB data
    void /* BITMAPINFO */ *m_data;

    DECLARE_NO_COPY_CLASS(wxBitmapDataObject)
};

// ----------------------------------------------------------------------------
// wxBitmapDataObject2 - a data object for CF_BITMAP
//
// FIXME did I already mention it was ugly?
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxBitmapDataObject2 : public wxBitmapDataObjectBase
{
public:
    // ctors
    wxBitmapDataObject2(const wxBitmap& bitmap = wxNullBitmap)
        : wxBitmapDataObjectBase(bitmap)
        {
        }

    // implement base class pure virtuals
    virtual size_t GetDataSize() const;
    virtual bool GetDataHere(void *buf) const;
    virtual bool SetData(size_t len, const void *buf);

    virtual size_t GetDataSize(const wxDataFormat& WXUNUSED(format)) const
        { return GetDataSize(); }
    virtual bool GetDataHere(const wxDataFormat& WXUNUSED(format),
                             void *buf) const
        { return GetDataHere(buf); }
    virtual bool SetData(const wxDataFormat& WXUNUSED(format),
                         size_t len, const void *buf)
        { return SetData(len, buf); }

private:
    DECLARE_NO_COPY_CLASS(wxBitmapDataObject2)
};

// ----------------------------------------------------------------------------
// wxFileDataObject - data object for CF_HDROP
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxFileDataObject : public wxFileDataObjectBase
{
public:
    wxFileDataObject() { }

    // implement base class pure virtuals
    virtual bool SetData(size_t len, const void *buf);
    virtual size_t GetDataSize() const;
    virtual bool GetDataHere(void *pData) const;
    virtual void AddFile(const wxString& file);

    virtual size_t GetDataSize(const wxDataFormat& WXUNUSED(format)) const
        { return GetDataSize(); }
    virtual bool GetDataHere(const wxDataFormat& WXUNUSED(format),
                             void *buf) const
        { return GetDataHere(buf); }
    virtual bool SetData(const wxDataFormat& WXUNUSED(format),
                         size_t len, const void *buf)
        { return SetData(len, buf); }

private:
    DECLARE_NO_COPY_CLASS(wxFileDataObject)
};

// ----------------------------------------------------------------------------
// wxURLDataObject: data object for URLs
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxURLDataObject : public wxDataObjectComposite
{
public:
    // initialize with URL in ctor or use SetURL later
    wxURLDataObject(const wxString& url = wxEmptyString);

    // return the URL as string
    wxString GetURL() const;

    // Set a string as the URL in the data object
    void SetURL(const wxString& url);

    // override to set m_textFormat
    virtual bool SetData(const wxDataFormat& format,
                         size_t len,
                         const void *buf);

private:
    // last data object we got data in
    wxDataObjectSimple *m_dataObjectLast;

    DECLARE_NO_COPY_CLASS(wxURLDataObject)
};

#endif // _WX_MSW_OLE_DATAOBJ2_H
