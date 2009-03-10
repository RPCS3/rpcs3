///////////////////////////////////////////////////////////////////////////////
// Name:        include/wx/msw/gdiimage.h
// Purpose:     wxGDIImage class: base class for wxBitmap, wxIcon, wxCursor
//              under MSW
// Author:      Vadim Zeitlin
// Modified by:
// Created:     20.11.99
// RCS-ID:      $Id: gdiimage.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// NB: this is a private header, it is not intended to be directly included by
//     user code (but may be included from other, public, wxWin headers

#ifndef _WX_MSW_GDIIMAGE_H_
#define _WX_MSW_GDIIMAGE_H_

#include "wx/gdiobj.h"          // base class
#include "wx/gdicmn.h"          // wxBITMAP_TYPE_INVALID
#include "wx/list.h"

class WXDLLIMPEXP_FWD_CORE wxGDIImageRefData;
class WXDLLIMPEXP_FWD_CORE wxGDIImageHandler;
class WXDLLIMPEXP_FWD_CORE wxGDIImage;

WX_DECLARE_EXPORTED_LIST(wxGDIImageHandler, wxGDIImageHandlerList);

// ----------------------------------------------------------------------------
// wxGDIImageRefData: common data fields for all derived classes
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxGDIImageRefData : public wxGDIRefData
{
public:
    wxGDIImageRefData()
    {
        m_width = m_height = m_depth = 0;

        m_handle = 0;
    }

    wxGDIImageRefData(const wxGDIImageRefData& data) : wxGDIRefData(data)
    {
        m_width = data.m_width;
        m_height = data.m_height;
        m_depth = data.m_depth;

        // can't copy handles like this, derived class copy ctor must do it!
        m_handle = NULL;
    }

    // accessors
    bool IsOk() const { return m_handle != 0; }

    void SetSize(int w, int h) { m_width = w; m_height = h; }

    // free the ressources we allocated
    virtual void Free() = 0;

    // for compatibility, the member fields are public

    // the size of the image
    int m_width, m_height;

    // the depth of the image
    int m_depth;

    // the handle to it
    union
    {
        WXHANDLE  m_handle;     // for untyped access
        WXHBITMAP m_hBitmap;
        WXHICON   m_hIcon;
        WXHCURSOR m_hCursor;
    };
};

// ----------------------------------------------------------------------------
// wxGDIImageHandler: a class which knows how to load/save wxGDIImages.
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxGDIImageHandler : public wxObject
{
public:
    // ctor
    wxGDIImageHandler() { m_type = wxBITMAP_TYPE_INVALID; }
    wxGDIImageHandler(const wxString& name,
                      const wxString& ext,
                      long type)
        : m_name(name), m_extension(ext)
    {
        m_type = type;
    }

    // accessors
    void SetName(const wxString& name) { m_name = name; }
    void SetExtension(const wxString& ext) { m_extension = ext; }
    void SetType(long type) { m_type = type; }

    const wxString& GetName() const { return m_name; }
    const wxString& GetExtension() const { return m_extension; }
    long GetType() const { return m_type; }

    // real handler operations: to implement in derived classes
    virtual bool Create(wxGDIImage *image,
                        const void* data,
                        long flags,
                        int width, int height, int depth = 1) = 0;
    virtual bool Load(wxGDIImage *image,
                      const wxString& name,
                      long flags,
                      int desiredWidth, int desiredHeight) = 0;
    virtual bool Save(wxGDIImage *image,
                      const wxString& name,
                      int type) = 0;

protected:
    wxString  m_name;
    wxString  m_extension;
    long      m_type;
};

// ----------------------------------------------------------------------------
// wxGDIImage: this class supports GDI image handlers which may be registered
// dynamically and will be used for loading/saving the images in the specified
// format. It also falls back to wxImage if no appropriate image is found.
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxGDIImage : public wxGDIObject
{
public:
    // handlers list interface
    static wxGDIImageHandlerList& GetHandlers() { return ms_handlers; }

    static void AddHandler(wxGDIImageHandler *handler);
    static void InsertHandler(wxGDIImageHandler *handler);
    static bool RemoveHandler(const wxString& name);

    static wxGDIImageHandler *FindHandler(const wxString& name);
    static wxGDIImageHandler *FindHandler(const wxString& extension, long type);
    static wxGDIImageHandler *FindHandler(long type);

    static void InitStandardHandlers();
    static void CleanUpHandlers();

    // access to the ref data casted to the right type
    wxGDIImageRefData *GetGDIImageData() const
        { return (wxGDIImageRefData *)m_refData; }

    // accessors
    WXHANDLE GetHandle() const
        { return IsNull() ? 0 : GetGDIImageData()->m_handle; }
    void SetHandle(WXHANDLE handle)
        { AllocExclusive(); GetGDIImageData()->m_handle = handle; }

    bool Ok() const { return IsOk(); }
    bool IsOk() const { return GetHandle() != 0; }

    int GetWidth() const { return IsNull() ? 0 : GetGDIImageData()->m_width; }
    int GetHeight() const { return IsNull() ? 0 : GetGDIImageData()->m_height; }
    int GetDepth() const { return IsNull() ? 0 : GetGDIImageData()->m_depth; }

    void SetWidth(int w) { AllocExclusive(); GetGDIImageData()->m_width = w; }
    void SetHeight(int h) { AllocExclusive(); GetGDIImageData()->m_height = h; }
    void SetDepth(int d) { AllocExclusive(); GetGDIImageData()->m_depth = d; }

    void SetSize(int w, int h)
    {
        AllocExclusive();
        GetGDIImageData()->SetSize(w, h);
    }
    void SetSize(const wxSize& size) { SetSize(size.x, size.y); }

    // forward some of base class virtuals to wxGDIImageRefData
    bool FreeResource(bool force = false);
    virtual WXHANDLE GetResourceHandle() const;

protected:
    // create the data for the derived class here
    virtual wxGDIImageRefData *CreateData() const = 0;

    // implement the wxObject method in terms of our, more specific, one
    virtual wxObjectRefData *CreateRefData() const { return CreateData(); }

    static wxGDIImageHandlerList ms_handlers;
};

#endif // _WX_MSW_GDIIMAGE_H_
