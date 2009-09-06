/////////////////////////////////////////////////////////////////////////////
// Name:        wx/imaglist.h
// Purpose:     wxImageList base header
// Author:      Julian Smart
// Modified by:
// Created:
// Copyright:   (c) Julian Smart
// RCS-ID:      $Id: imaglist.h 41288 2006-09-18 23:06:35Z VZ $
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_IMAGLIST_H_BASE_
#define _WX_IMAGLIST_H_BASE_

/*
 * wxImageList is used for wxListCtrl, wxTreeCtrl. These controls refer to
 * images for their items by an index into an image list.
 * A wxImageList is capable of creating images with optional masks from
 * a variety of sources - a single bitmap plus a colour to indicate the mask,
 * two bitmaps, or an icon.
 *
 * Image lists can also create and draw images used for drag and drop functionality.
 * This is not yet implemented in wxImageList. We need to discuss a generic API
 * for doing drag and drop and see whether it ties in with the Win95 view of it.
 * See below for candidate functions and an explanation of how they might be
 * used.
 */

// Flag values for Set/GetImageList
enum
{
    wxIMAGE_LIST_NORMAL, // Normal icons
    wxIMAGE_LIST_SMALL,  // Small icons
    wxIMAGE_LIST_STATE   // State icons: unimplemented (see WIN32 documentation)
};

// Flags for Draw
#define wxIMAGELIST_DRAW_NORMAL         0x0001
#define wxIMAGELIST_DRAW_TRANSPARENT    0x0002
#define wxIMAGELIST_DRAW_SELECTED       0x0004
#define wxIMAGELIST_DRAW_FOCUSED        0x0008

#if defined(__WXMSW__) || defined(__WXMAC_CARBON__)
    #define wxHAS_NATIVE_IMAGELIST
#endif

#if !defined(wxHAS_NATIVE_IMAGELIST)
    #include "wx/generic/imaglist.h"
#elif defined(__WXMSW__)
    #include "wx/msw/imaglist.h"
#elif defined(__WXMAC_CARBON__)
    #include "wx/mac/imaglist.h"
#endif

#endif // _WX_IMAGLIST_H_BASE_
