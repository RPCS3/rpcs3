///////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/enhmeta.h
// Purpose:     wxEnhMetaFile class for Win32
// Author:      Vadim Zeitlin
// Modified by:
// Created:     13.01.00
// RCS-ID:      $Id: enhmeta.h 60850 2009-06-01 10:16:13Z JS $
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_ENHMETA_H_
#define _WX_MSW_ENHMETA_H_

#include "wx/dc.h"

#if wxUSE_DRAG_AND_DROP
    #include "wx/dataobj.h"
#endif

// Change this to 1 if you set wxUSE_HIGH_QUALITY_PREVIEW_IN_WXMSW to 1 in prntbase.cpp
#define wxUSE_ENH_METAFILE_FROM_DC 0

// ----------------------------------------------------------------------------
// wxEnhMetaFile: encapsulation of Win32 HENHMETAFILE
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxEnhMetaFile : public wxObject
{
public:
    wxEnhMetaFile(const wxString& file = wxEmptyString) : m_filename(file)
        { Init(); }
    wxEnhMetaFile(const wxEnhMetaFile& metafile) : wxObject()
        { Init(); Assign(metafile); }
    wxEnhMetaFile& operator=(const wxEnhMetaFile& metafile)
        { Free(); Assign(metafile); return *this; }

    virtual ~wxEnhMetaFile()
        { Free(); }

    // display the picture stored in the metafile on the given DC
    bool Play(wxDC *dc, wxRect *rectBound = (wxRect *)NULL);

    // accessors
    bool Ok() const { return IsOk(); }
    bool IsOk() const { return m_hMF != 0; }

    wxSize GetSize() const;
    int GetWidth() const { return GetSize().x; }
    int GetHeight() const { return GetSize().y; }

    const wxString& GetFileName() const { return m_filename; }

    // copy the metafile to the clipboard: the width and height parameters are
    // for backwards compatibility (with wxMetaFile) only, they are ignored by
    // this method
    bool SetClipboard(int width = 0, int height = 0);

    // implementation
    WXHANDLE GetHENHMETAFILE() const { return m_hMF; }
    void SetHENHMETAFILE(WXHANDLE hMF) { Free(); m_hMF = hMF; }

protected:
    void Init();
    void Free();
    void Assign(const wxEnhMetaFile& mf);

private:
    wxString m_filename;
    WXHANDLE m_hMF;

    DECLARE_DYNAMIC_CLASS(wxEnhMetaFile)
};

// ----------------------------------------------------------------------------
// wxEnhMetaFileDC: allows to create a wxEnhMetaFile
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxEnhMetaFileDC : public wxDC
{
public:
    // the ctor parameters specify the filename (empty for memory metafiles),
    // the metafile picture size and the optional description/comment
    wxEnhMetaFileDC(const wxString& filename = wxEmptyString,
                    int width = 0, int height = 0,
                    const wxString& description = wxEmptyString);

#if wxUSE_ENH_METAFILE_FROM_DC
    // as above, but takes reference DC as first argument to take resolution,
    // size, font metrics etc. from
    wxEnhMetaFileDC(const wxDC& referenceDC,
                    const wxString& filename = wxEmptyString,
                    int width = 0, int height = 0,
                    const wxString& description = wxEmptyString);
#endif

    virtual ~wxEnhMetaFileDC();

    // obtain a pointer to the new metafile (caller should delete it)
    wxEnhMetaFile *Close();

protected:
    virtual void DoGetSize(int *width, int *height) const;

private:
    // size passed to ctor and returned by DoGetSize()
    int m_width,
        m_height;

    DECLARE_DYNAMIC_CLASS_NO_COPY(wxEnhMetaFileDC)
};

#if wxUSE_DRAG_AND_DROP

// ----------------------------------------------------------------------------
// wxEnhMetaFileDataObject is a specialization of wxDataObject for enh metafile
// ----------------------------------------------------------------------------

// notice that we want to support both CF_METAFILEPICT and CF_ENHMETAFILE and
// so we derive from wxDataObject and not from wxDataObjectSimple
class WXDLLEXPORT wxEnhMetaFileDataObject : public wxDataObject
{
public:
    // ctors
    wxEnhMetaFileDataObject() { }
    wxEnhMetaFileDataObject(const wxEnhMetaFile& metafile)
        : m_metafile(metafile) { }

    // virtual functions which you may override if you want to provide data on
    // demand only - otherwise, the trivial default versions will be used
    virtual void SetMetafile(const wxEnhMetaFile& metafile)
        { m_metafile = metafile; }
    virtual wxEnhMetaFile GetMetafile() const
        { return m_metafile; }

    // implement base class pure virtuals
    virtual wxDataFormat GetPreferredFormat(Direction dir) const;
    virtual size_t GetFormatCount(Direction dir) const;
    virtual void GetAllFormats(wxDataFormat *formats, Direction dir) const;
    virtual size_t GetDataSize(const wxDataFormat& format) const;
    virtual bool GetDataHere(const wxDataFormat& format, void *buf) const;
    virtual bool SetData(const wxDataFormat& format, size_t len,
                         const void *buf);

protected:
    wxEnhMetaFile m_metafile;

    DECLARE_NO_COPY_CLASS(wxEnhMetaFileDataObject)
};


// ----------------------------------------------------------------------------
// wxEnhMetaFileSimpleDataObject does derive from wxDataObjectSimple which
// makes it more convenient to use (it can be used with wxDataObjectComposite)
// at the price of not supoprting any more CF_METAFILEPICT but only
// CF_ENHMETAFILE
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxEnhMetaFileSimpleDataObject : public wxDataObjectSimple
{
public:
    // ctors
    wxEnhMetaFileSimpleDataObject() : wxDataObjectSimple(wxDF_ENHMETAFILE) { }
    wxEnhMetaFileSimpleDataObject(const wxEnhMetaFile& metafile)
        : wxDataObjectSimple(wxDF_ENHMETAFILE), m_metafile(metafile) { }

    // virtual functions which you may override if you want to provide data on
    // demand only - otherwise, the trivial default versions will be used
    virtual void SetEnhMetafile(const wxEnhMetaFile& metafile)
        { m_metafile = metafile; }
    virtual wxEnhMetaFile GetEnhMetafile() const
        { return m_metafile; }

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

protected:
    wxEnhMetaFile m_metafile;

    DECLARE_NO_COPY_CLASS(wxEnhMetaFileSimpleDataObject)
};

#endif // wxUSE_DRAG_AND_DROP

#endif // _WX_MSW_ENHMETA_H_
