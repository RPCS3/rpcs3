/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/region.h
// Purpose:     generic wxRegion class
// Author:      David Elliott
// Modified by:
// Created:     2004/04/12
// RCS-ID:      $Id: region.h 41429 2006-09-25 11:47:23Z VZ $
// Copyright:   (c) 2004 David Elliott
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_REGION_H__
#define _WX_GENERIC_REGION_H__

class WXDLLEXPORT wxRegionGeneric : public wxRegionBase
{
public:
    wxRegionGeneric(wxCoord x, wxCoord y, wxCoord w, wxCoord h);
    wxRegionGeneric(const wxPoint& topLeft, const wxPoint& bottomRight);
    wxRegionGeneric(const wxRect& rect);
    wxRegionGeneric();
    virtual ~wxRegionGeneric();

    // wxRegionBase pure virtuals
    virtual void Clear();
    virtual bool IsEmpty() const;

protected:
    virtual wxObjectRefData *CreateRefData() const;
    virtual wxObjectRefData *CloneRefData(const wxObjectRefData *data) const;

    // wxRegionBase pure virtuals
    virtual bool DoIsEqual(const wxRegion& region) const;
    virtual bool DoGetBox(wxCoord& x, wxCoord& y, wxCoord& w, wxCoord& h) const;
    virtual wxRegionContain DoContainsPoint(wxCoord x, wxCoord y) const;
    virtual wxRegionContain DoContainsRect(const wxRect& rect) const;

    virtual bool DoOffset(wxCoord x, wxCoord y);
    virtual bool DoUnionWithRect(const wxRect& rect);
    virtual bool DoUnionWithRegion(const wxRegion& region);
    virtual bool DoIntersect(const wxRegion& region);
    virtual bool DoSubtract(const wxRegion& region);
    virtual bool DoXor(const wxRegion& region);

    friend class WXDLLEXPORT wxRegionIteratorGeneric;
};

class WXDLLEXPORT wxRegionIteratorGeneric : public wxObject
{
public:
    wxRegionIteratorGeneric();
    wxRegionIteratorGeneric(const wxRegionGeneric& region);
    wxRegionIteratorGeneric(const wxRegionIteratorGeneric& iterator);
    virtual ~wxRegionIteratorGeneric();

    wxRegionIteratorGeneric& operator=(const wxRegionIteratorGeneric& iterator);

    void Reset() { m_current = 0; }
    void Reset(const wxRegionGeneric& region);

    operator bool () const { return HaveRects(); }
    bool HaveRects() const;

    wxRegionIteratorGeneric& operator++();
    wxRegionIteratorGeneric operator++(int);

    long GetX() const;
    long GetY() const;
    long GetW() const;
    long GetWidth() const { return GetW(); }
    long GetH() const;
    long GetHeight() const { return GetH(); }
    wxRect GetRect() const;
private:
    long     m_current;
    wxRegionGeneric m_region;
};

#endif // _WX_GENERIC_REGION_H__
