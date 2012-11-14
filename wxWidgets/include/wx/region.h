/////////////////////////////////////////////////////////////////////////////
// Name:        wx/region.h
// Purpose:     Base header for wxRegion
// Author:      Julian Smart
// Modified by:
// Created:
// RCS-ID:      $Id: region.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows Licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_REGION_H_BASE_
#define _WX_REGION_H_BASE_

#include "wx/gdiobj.h"
#include "wx/gdicmn.h"

class WXDLLIMPEXP_FWD_CORE wxBitmap;
class WXDLLIMPEXP_FWD_CORE wxColour;
class WXDLLIMPEXP_FWD_CORE wxRegion;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// result of wxRegion::Contains() call
enum wxRegionContain
{
    wxOutRegion = 0,
    wxPartRegion = 1,
    wxInRegion = 2
};

// these constants are used with wxRegion::Combine() in the ports which have
// this method
enum wxRegionOp
{
    // Creates the intersection of the two combined regions.
    wxRGN_AND,

    // Creates a copy of the region
    wxRGN_COPY,

    // Combines the parts of first region that are not in the second one
    wxRGN_DIFF,

    // Creates the union of two combined regions.
    wxRGN_OR,

    // Creates the union of two regions except for any overlapping areas.
    wxRGN_XOR
};

// ----------------------------------------------------------------------------
// wxRegionBase defines wxRegion API
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxRegionBase : public wxGDIObject
{
public:
    // ctors
    // -----

    // none are defined here but the following should be available:
#if 0
    wxRegion();
    wxRegion(wxCoord x, wxCoord y, wxCoord w, wxCoord h);
    wxRegion(const wxPoint& topLeft, const wxPoint& bottomRight);
    wxRegion(const wxRect& rect);
    wxRegion(size_t n, const wxPoint *points, int fillStyle = wxODDEVEN_RULE);
    wxRegion(const wxBitmap& bmp);
    wxRegion(const wxBitmap& bmp, const wxColour& transp, int tolerance = 0);
#endif // 0

    // operators
    // ---------

    bool operator==(const wxRegion& region) const { return IsEqual(region); }
    bool operator!=(const wxRegion& region) const { return !(*this == region); }


    // accessors
    // ---------

    bool Ok() const { return IsOk(); }
    bool IsOk() const { return m_refData != NULL; }

    // Is region empty?
    virtual bool IsEmpty() const = 0;
    bool Empty() const { return IsEmpty(); }

    // Is region equal (i.e. covers the same area as another one)?
    bool IsEqual(const wxRegion& region) const;

    // Get the bounding box
    bool GetBox(wxCoord& x, wxCoord& y, wxCoord& w, wxCoord& h) const
        { return DoGetBox(x, y, w, h); }
    wxRect GetBox() const
    {
        wxCoord x, y, w, h;
        return DoGetBox(x, y, w, h) ? wxRect(x, y, w, h) : wxRect();
    }

    // Test if the given point or rectangle is inside this region
    wxRegionContain Contains(wxCoord x, wxCoord y) const
        { return DoContainsPoint(x, y); }
    wxRegionContain Contains(const wxPoint& pt) const
        { return DoContainsPoint(pt.x, pt.y); }
    wxRegionContain Contains(wxCoord x, wxCoord y, wxCoord w, wxCoord h) const
        { return DoContainsRect(wxRect(x, y, w, h)); }
    wxRegionContain Contains(const wxRect& rect) const
        { return DoContainsRect(rect); }


    // operations
    // ----------

    virtual void Clear() = 0;

    // Move the region
    bool Offset(wxCoord x, wxCoord y)
        { return DoOffset(x, y); }
    bool Offset(const wxPoint& pt)
        { return DoOffset(pt.x, pt.y); }

    // Union rectangle or region with this region.
    bool Union(wxCoord x, wxCoord y, wxCoord w, wxCoord h)
        { return DoUnionWithRect(wxRect(x, y, w, h)); }
    bool Union(const wxRect& rect)
        { return DoUnionWithRect(rect); }
    bool Union(const wxRegion& region)
        { return DoUnionWithRegion(region); }

#if wxUSE_IMAGE
    // Use the non-transparent pixels of a wxBitmap for the region to combine
    // with this region.  First version takes transparency from bitmap's mask,
    // second lets the user specify the colour to be treated as transparent
    // along with an optional tolerance value.
    // NOTE: implemented in common/rgncmn.cpp
    bool Union(const wxBitmap& bmp);
    bool Union(const wxBitmap& bmp, const wxColour& transp, int tolerance = 0);
#endif // wxUSE_IMAGE

    // Intersect rectangle or region with this one.
    bool Intersect(wxCoord x, wxCoord y, wxCoord w, wxCoord h);
    bool Intersect(const wxRect& rect);
    bool Intersect(const wxRegion& region)
        { return DoIntersect(region); }

    // Subtract rectangle or region from this:
    // Combines the parts of 'this' that are not part of the second region.
    bool Subtract(wxCoord x, wxCoord y, wxCoord w, wxCoord h);
    bool Subtract(const wxRect& rect);
    bool Subtract(const wxRegion& region)
        { return DoSubtract(region); }

    // XOR: the union of two combined regions except for any overlapping areas.
    bool Xor(wxCoord x, wxCoord y, wxCoord w, wxCoord h);
    bool Xor(const wxRect& rect);
    bool Xor(const wxRegion& region)
        { return DoXor(region); }


    // Convert the region to a B&W bitmap with the white pixels being inside
    // the region.
    wxBitmap ConvertToBitmap() const;

protected:
    virtual bool DoIsEqual(const wxRegion& region) const = 0;
    virtual bool DoGetBox(wxCoord& x, wxCoord& y, wxCoord& w, wxCoord& h) const = 0;
    virtual wxRegionContain DoContainsPoint(wxCoord x, wxCoord y) const = 0;
    virtual wxRegionContain DoContainsRect(const wxRect& rect) const = 0;

    virtual bool DoOffset(wxCoord x, wxCoord y) = 0;

    virtual bool DoUnionWithRect(const wxRect& rect) = 0;
    virtual bool DoUnionWithRegion(const wxRegion& region) = 0;

    virtual bool DoIntersect(const wxRegion& region) = 0;
    virtual bool DoSubtract(const wxRegion& region) = 0;
    virtual bool DoXor(const wxRegion& region) = 0;
};

// some ports implement a generic Combine() function while others only
// implement individual wxRegion operations, factor out the common code for the
// ports with Combine() in this class
#if defined(__WXPALMOS__) || \
    defined(__WXMSW__) || \
    defined(__WXMAC__) || \
    defined(__WXPM__)

#define wxHAS_REGION_COMBINE

class WXDLLEXPORT wxRegionWithCombine : public wxRegionBase
{
public:
    // these methods are not part of public API as they're not implemented on
    // all ports
    bool Combine(wxCoord x, wxCoord y, wxCoord w, wxCoord h, wxRegionOp op);
    bool Combine(const wxRect& rect, wxRegionOp op);
    bool Combine(const wxRegion& region, wxRegionOp op)
        { return DoCombine(region, op); }


protected:
    // the real Combine() method, to be defined in the derived class
    virtual bool DoCombine(const wxRegion& region, wxRegionOp op) = 0;

    // implement some wxRegionBase pure virtuals in terms of Combine()
    virtual bool DoUnionWithRect(const wxRect& rect);
    virtual bool DoUnionWithRegion(const wxRegion& region);
    virtual bool DoIntersect(const wxRegion& region);
    virtual bool DoSubtract(const wxRegion& region);
    virtual bool DoXor(const wxRegion& region);
};

#endif // ports with wxRegion::Combine()

#if defined(__WXPALMOS__)
    #include "wx/palmos/region.h"
#elif defined(__WXMSW__)
    #include "wx/msw/region.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/region.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/region.h"
#elif defined(__WXMOTIF__) || defined(__WXX11__)
    #include "wx/x11/region.h"
#elif defined(__WXMGL__)
    #include "wx/mgl/region.h"
#elif defined(__WXDFB__)
    #include "wx/dfb/region.h"
#elif defined(__WXMAC__)
    #include "wx/mac/region.h"
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/region.h"
#elif defined(__WXPM__)
    #include "wx/os2/region.h"
#endif

// ----------------------------------------------------------------------------
// inline functions implementation
// ----------------------------------------------------------------------------

// NB: these functions couldn't be defined in the class declaration as they use
//     wxRegion and so can be only defined after including the header declaring
//     the real class

inline bool wxRegionBase::Intersect(const wxRect& rect)
{
    return DoIntersect(wxRegion(rect));
}

inline bool wxRegionBase::Subtract(const wxRect& rect)
{
    return DoSubtract(wxRegion(rect));
}

inline bool wxRegionBase::Xor(const wxRect& rect)
{
    return DoXor(wxRegion(rect));
}

// ...and these functions are here because they call the ones above, and its
// not really proper to call an inline function before its defined inline.

inline bool wxRegionBase::Intersect(wxCoord x, wxCoord y, wxCoord w, wxCoord h)
{
    return Intersect(wxRect(x, y, w, h));
}

inline bool wxRegionBase::Subtract(wxCoord x, wxCoord y, wxCoord w, wxCoord h)
{
    return Subtract(wxRect(x, y, w, h));
}

inline bool wxRegionBase::Xor(wxCoord x, wxCoord y, wxCoord w, wxCoord h)
{
    return Xor(wxRect(x, y, w, h));
}

#ifdef wxHAS_REGION_COMBINE

inline bool wxRegionWithCombine::Combine(wxCoord x,
                                         wxCoord y,
                                         wxCoord w,
                                         wxCoord h,
                                         wxRegionOp op)
{
    return DoCombine(wxRegion(x, y, w, h), op);
}

inline bool wxRegionWithCombine::Combine(const wxRect& rect, wxRegionOp op)
{
    return DoCombine(wxRegion(rect), op);
}

#endif // wxHAS_REGION_COMBINE

#endif // _WX_REGION_H_BASE_
