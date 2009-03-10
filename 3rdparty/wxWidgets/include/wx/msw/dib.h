/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/dib.h
// Purpose:     wxDIB class representing Win32 device independent bitmaps
// Author:      Vadim Zeitlin
// Modified by:
// Created:     03.03.03 (replaces the old file with the same name)
// RCS-ID:      $Id: dib.h 49804 2007-11-10 01:09:42Z VZ $
// Copyright:   (c) 1997-2003 wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_DIB_H_
#define _WX_MSW_DIB_H_

class WXDLLIMPEXP_FWD_CORE wxBitmap;
class WXDLLIMPEXP_FWD_CORE wxPalette;

#include "wx/msw/private.h"

#if wxUSE_WXDIB

// ----------------------------------------------------------------------------
// wxDIB: represents a DIB section
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxDIB
{
public:
    // ctors and such
    // --------------

    // create an uninitialized DIB with the given width, height and depth (only
    // 24 and 32 bpp DIBs are currently supported)
    //
    // after using this ctor, GetData() and GetHandle() may be used if IsOk()
    // returns true
    wxDIB(int width, int height, int depth)
        { Init(); (void)Create(width, height, depth); }

    // create a DIB from the DDB
    wxDIB(const wxBitmap& bmp)
        { Init(); (void)Create(bmp); }

    // create a DIB from the Windows DDB
    wxDIB(HBITMAP hbmp)
        { Init(); (void)Create(hbmp); }

    // load a DIB from file (any depth is supoprted here unlike above)
    //
    // as above, use IsOk() to see if the bitmap was loaded successfully
    wxDIB(const wxString& filename)
        { Init(); (void)Load(filename); }

    // same as the corresponding ctors but with return value
    bool Create(int width, int height, int depth);
    bool Create(const wxBitmap& bmp);
    bool Create(HBITMAP hbmp);
    bool Load(const wxString& filename);

    // dtor is not virtual, this class is not meant to be used polymorphically
    ~wxDIB();


    // operations
    // ----------

#ifndef __WXWINCE__
    // create a bitmap compatible with the given HDC (or screen by default) and
    // return its handle, the caller is responsible for freeing it (using
    // DeleteObject())
    HBITMAP CreateDDB(HDC hdc = 0) const;
#endif // !__WXWINCE__

    // get the handle from the DIB and reset it, i.e. this object won't destroy
    // the DIB after this (but the caller should do it)
    HBITMAP Detach() { HBITMAP hbmp = m_handle; m_handle = 0; return hbmp; }

#if wxUSE_PALETTE
    // create a palette for this DIB (always a trivial/default one for 24bpp)
    wxPalette *CreatePalette() const;
#endif // wxUSE_PALETTE

    // save the DIB as a .BMP file to the file with the given name
    bool Save(const wxString& filename);


    // accessors
    // ---------

    // return true if DIB was successfully created, false otherwise
    bool IsOk() const { return m_handle != 0; }

    // get the bitmap size
    wxSize GetSize() const { DoGetObject(); return wxSize(m_width, m_height); }
    int GetWidth() const { DoGetObject(); return m_width; }
    int GetHeight() const { DoGetObject(); return m_height; }

    // get the number of bits per pixel, or depth
    int GetDepth() const { DoGetObject(); return m_depth; }

    // get the DIB handle
    HBITMAP GetHandle() const { return m_handle; }

    // get raw pointer to bitmap bits, you should know what you do if you
    // decide to use it
    unsigned char *GetData() const
        { DoGetObject(); return (unsigned char *)m_data; }


    // HBITMAP conversion
    // ------------------

    // these functions are only used by wxWidgets internally right now, please
    // don't use them directly if possible as they're subject to change

#ifndef __WXWINCE__
    // creates a DDB compatible with the given (or screen) DC from either
    // a plain DIB or a DIB section (in which case the last parameter must be
    // non NULL)
    static HBITMAP ConvertToBitmap(const BITMAPINFO *pbi,
                                   HDC hdc = 0,
                                   void *bits = NULL);

    // create a plain DIB (not a DIB section) from a DDB, the caller is
    // responsable for freeing it using ::GlobalFree()
    static HGLOBAL ConvertFromBitmap(HBITMAP hbmp);

    // creates a DIB from the given DDB or calculates the space needed by it:
    // if pbi is NULL, only the space is calculated, otherwise pbi is supposed
    // to point at BITMAPINFO of the correct size which is filled by this
    // function (this overload is needed for wxBitmapDataObject code in
    // src/msw/ole/dataobj.cpp)
    static size_t ConvertFromBitmap(BITMAPINFO *pbi, HBITMAP hbmp);
#endif // __WXWINCE__


    // wxImage conversion
    // ------------------

#if wxUSE_IMAGE
    // create a DIB from the given image, the DIB will be either 24 or 32 (if
    // the image has alpha channel) bpp
    wxDIB(const wxImage& image) { Init(); (void)Create(image); }

    // same as the above ctor but with the return code
    bool Create(const wxImage& image);

    // create wxImage having the same data as this DIB
    wxImage ConvertToImage() const;
#endif // wxUSE_IMAGE


    // helper functions
    // ----------------

    // return the size of one line in a DIB with given width and depth: the
    // point here is that as the scan lines need to be DWORD aligned so we may
    // need to add some padding
    static unsigned long GetLineSize(int width, int depth)
    {
        return ((width*depth + 31) & ~31) >> 3;
    }

private:
    // common part of all ctors
    void Init();

    // free resources
    void Free();

    // initialize the contents from the provided DDB (Create() must have been
    // already called)
    bool CopyFromDDB(HBITMAP hbmp);


    // the DIB section handle, 0 if invalid
    HBITMAP m_handle;

    // NB: we could store only m_handle and not any of the other fields as
    //     we may always retrieve them from it using ::GetObject(), but we
    //     decide to still store them for efficiency concerns -- however if we
    //     don't have them from the very beginning (e.g. DIB constructed from a
    //     bitmap), we only retrieve them when necessary and so these fields
    //     should *never* be accessed directly, even from inside wxDIB code

    // function which must be called before accessing any members and which
    // gets their values from m_handle, if not done yet
    void DoGetObject() const;

    // pointer to DIB bits, may be NULL
    void *m_data;

    // size and depth of the image
    int m_width,
        m_height,
        m_depth;

    // in some cases we could be using a handle which we didn't create and in
    // this case we shouldn't free it neither -- this flag tell us if this is
    // the case
    bool m_ownsHandle;

    // if true, we have alpha, if false we don't (note that we can still have
    // m_depth == 32 but the last component is then simply padding and not
    // alpha)
    bool m_hasAlpha;


    // DIBs can't be copied
    wxDIB(const wxDIB&);
    wxDIB& operator=(const wxDIB&);
};

// ----------------------------------------------------------------------------
// inline functions implementation
// ----------------------------------------------------------------------------

inline
void wxDIB::Init()
{
    m_handle = 0;
    m_ownsHandle = true;
    m_hasAlpha = false;

    m_data = NULL;

    m_width =
    m_height =
    m_depth = 0;
}

inline
void wxDIB::Free()
{
    if ( m_handle && m_ownsHandle )
    {
        if ( !::DeleteObject(m_handle) )
        {
            wxLogLastError(wxT("DeleteObject(hDIB)"));
        }

        Init();
    }
}

inline wxDIB::~wxDIB()
{
    Free();
}

#endif
    // wxUSE_WXDIB

#endif // _WX_MSW_DIB_H_

