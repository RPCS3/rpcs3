/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/imaglist.h
// Purpose:
// Author:      Robert Roebling
// Created:     01/02/97
// Id:
// Copyright:   (c) 1998 Robert Roebling and Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef __IMAGELISTH_G__
#define __IMAGELISTH_G__

#include "wx/defs.h"
#include "wx/list.h"
#include "wx/icon.h"

class WXDLLEXPORT wxDC;
class WXDLLEXPORT wxBitmap;
class WXDLLEXPORT wxColour;


class WXDLLEXPORT wxGenericImageList: public wxObject
{
public:
    wxGenericImageList() { m_width = m_height = 0; }
    wxGenericImageList( int width, int height, bool mask = true, int initialCount = 1 );
    virtual ~wxGenericImageList();
    bool Create( int width, int height, bool mask = true, int initialCount = 1 );
    bool Create();

    virtual int GetImageCount() const;
    virtual bool GetSize( int index, int &width, int &height ) const;

    int Add( const wxBitmap& bitmap );
    int Add( const wxBitmap& bitmap, const wxBitmap& mask );
    int Add( const wxBitmap& bitmap, const wxColour& maskColour );
    wxBitmap GetBitmap(int index) const;
    wxIcon GetIcon(int index) const;
    bool Replace( int index, const wxBitmap &bitmap );
    bool Replace( int index, const wxBitmap &bitmap, const wxBitmap& mask );
    bool Remove( int index );
    bool RemoveAll();

    virtual bool Draw(int index, wxDC& dc, int x, int y,
              int flags = wxIMAGELIST_DRAW_NORMAL,
              bool solidBackground = false);

    // Internal use only
    const wxBitmap *GetBitmapPtr(int index) const;
private:
    wxList  m_images;

    int     m_width;
    int     m_height;

    DECLARE_DYNAMIC_CLASS(wxGenericImageList)
};

#ifndef wxHAS_NATIVE_IMAGELIST

/*
 * wxImageList has to be a real class or we have problems with
 * the run-time information.
 */

class WXDLLEXPORT wxImageList: public wxGenericImageList
{
    DECLARE_DYNAMIC_CLASS(wxImageList)

public:
    wxImageList() {}

    wxImageList( int width, int height, bool mask = true, int initialCount = 1 )
        : wxGenericImageList(width, height, mask, initialCount)
    {
    }
};
#endif // !wxHAS_NATIVE_IMAGELIST

#endif  // __IMAGELISTH_G__

