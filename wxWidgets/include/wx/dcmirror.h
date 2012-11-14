///////////////////////////////////////////////////////////////////////////////
// Name:        wx/dcmirror.h
// Purpose:     wxMirrorDC class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     21.07.2003
// RCS-ID:      $Id: dcmirror.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DCMIRROR_H_
#define _WX_DCMIRROR_H_

#include "wx/dc.h"

// ----------------------------------------------------------------------------
// wxMirrorDC allows to write the same code for horz/vertical layout
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxMirrorDC : public wxDC
{
public:
    // constructs a mirror DC associated with the given real DC
    //
    // if mirror parameter is true, all vertical and horizontal coordinates are
    // exchanged, otherwise this class behaves in exactly the same way as a
    // plain DC
    //
    // the cast to wxMirrorDC is a dirty hack done to allow us to call the
    // protected methods of wxDCBase directly in our code below, without it it
    // would be impossible (this is correct from C++ point of view but doesn't
    // make any sense in this particular situation)
    wxMirrorDC(wxDC& dc, bool mirror) : m_dc((wxMirrorDC&)dc)
        { m_mirror = mirror; }

    // wxDCBase operations
    virtual void Clear() { m_dc.Clear(); }
    virtual void SetFont(const wxFont& font) { m_dc.SetFont(font); }
    virtual void SetPen(const wxPen& pen) { m_dc.SetPen(pen); }
    virtual void SetBrush(const wxBrush& brush) { m_dc.SetBrush(brush); }
    virtual void SetBackground(const wxBrush& brush)
        { m_dc.SetBackground(brush); }
    virtual void SetBackgroundMode(int mode) { m_dc.SetBackgroundMode(mode); }
#if wxUSE_PALETTE
    virtual void SetPalette(const wxPalette& palette)
        { m_dc.SetPalette(palette); }
#endif // wxUSE_PALETTE
    virtual void DestroyClippingRegion() { m_dc.DestroyClippingRegion(); }
    virtual wxCoord GetCharHeight() const { return m_dc.GetCharHeight(); }
    virtual wxCoord GetCharWidth() const { return m_dc.GetCharWidth(); }
    virtual bool CanDrawBitmap() const { return m_dc.CanDrawBitmap(); }
    virtual bool CanGetTextExtent() const { return m_dc.CanGetTextExtent(); }
    virtual int GetDepth() const { return m_dc.GetDepth(); }
    virtual wxSize GetPPI() const { return m_dc.GetPPI(); }
    virtual bool Ok() const { return IsOk(); }
    virtual bool IsOk() const { return m_dc.Ok(); }
    virtual void SetMapMode(int mode) { m_dc.SetMapMode(mode); }
    virtual void SetUserScale(double x, double y)
        { m_dc.SetUserScale(GetX(x, y), GetY(x, y)); }
    virtual void SetLogicalOrigin(wxCoord x, wxCoord y)
        { m_dc.SetLogicalOrigin(GetX(x, y), GetY(x, y)); }
    virtual void SetDeviceOrigin(wxCoord x, wxCoord y)
        { m_dc.SetDeviceOrigin(GetX(x, y), GetY(x, y)); }
    virtual void SetAxisOrientation(bool xLeftRight, bool yBottomUp)
        { m_dc.SetAxisOrientation(GetX(xLeftRight, yBottomUp),
                                  GetY(xLeftRight, yBottomUp)); }
    virtual void SetLogicalFunction(int function)
        { m_dc.SetLogicalFunction(function); }

    // helper functions which may be useful for the users of this class
    wxSize Reflect(const wxSize& sizeOrig)
    {
        return m_mirror ? wxSize(sizeOrig.y, sizeOrig.x) : sizeOrig;
    }

protected:
    // returns x and y if not mirroring or y and x if mirroring
    wxCoord GetX(wxCoord x, wxCoord y) const { return m_mirror ? y : x; }
    wxCoord GetY(wxCoord x, wxCoord y) const { return m_mirror ? x : y; }
    double GetX(double x, double y) const { return m_mirror ? y : x; }
    double GetY(double x, double y) const { return m_mirror ? x : y; }
    bool GetX(bool x, bool y) const { return m_mirror ? y : x; }
    bool GetY(bool x, bool y) const { return m_mirror ? x : y; }

    // same thing but for pointers
    wxCoord *GetX(wxCoord *x, wxCoord *y) const { return m_mirror ? y : x; }
    wxCoord *GetY(wxCoord *x, wxCoord *y) const { return m_mirror ? x : y; }

    // exchange x and y unconditionally
    static void Swap(wxCoord& x, wxCoord& y)
    {
        wxCoord t = x;
        x = y;
        y = t;
    }

    // exchange x and y components of all points in the array if necessary
    void Mirror(int n, wxPoint points[]) const
    {
        if ( m_mirror )
        {
            for ( int i = 0; i < n; i++ )
            {
                Swap(points[i].x, points[i].y);
            }
        }
    }


    // wxDCBase functions
    virtual bool DoFloodFill(wxCoord x, wxCoord y, const wxColour& col,
                             int style = wxFLOOD_SURFACE)
    {
        return m_dc.DoFloodFill(GetX(x, y), GetY(x, y), col, style);
    }

    virtual bool DoGetPixel(wxCoord x, wxCoord y, wxColour *col) const
    {
        return m_dc.DoGetPixel(GetX(x, y), GetY(x, y), col);
    }


    virtual void DoDrawPoint(wxCoord x, wxCoord y)
    {
        m_dc.DoDrawPoint(GetX(x, y), GetY(x, y));
    }

    virtual void DoDrawLine(wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2)
    {
        m_dc.DoDrawLine(GetX(x1, y1), GetY(x1, y1), GetX(x2, y2), GetY(x2, y2));
    }

    virtual void DoDrawArc(wxCoord x1, wxCoord y1,
                           wxCoord x2, wxCoord y2,
                           wxCoord xc, wxCoord yc)
    {
        wxFAIL_MSG( wxT("this is probably wrong") );

        m_dc.DoDrawArc(GetX(x1, y1), GetY(x1, y1),
                       GetX(x2, y2), GetY(x2, y2),
                       xc, yc);
    }

    virtual void DoDrawCheckMark(wxCoord x, wxCoord y,
                                 wxCoord w, wxCoord h)
    {
        m_dc.DoDrawCheckMark(GetX(x, y), GetY(x, y),
                             GetX(w, h), GetY(w, h));
    }

    virtual void DoDrawEllipticArc(wxCoord x, wxCoord y, wxCoord w, wxCoord h,
                                   double sa, double ea)
    {
        wxFAIL_MSG( wxT("this is probably wrong") );

        m_dc.DoDrawEllipticArc(GetX(x, y), GetY(x, y),
                               GetX(w, h), GetY(w, h),
                               sa, ea);
    }

    virtual void DoDrawRectangle(wxCoord x, wxCoord y, wxCoord w, wxCoord h)
    {
        m_dc.DoDrawRectangle(GetX(x, y), GetY(x, y), GetX(w, h), GetY(w, h));
    }

    virtual void DoDrawRoundedRectangle(wxCoord x, wxCoord y,
                                        wxCoord w, wxCoord h,
                                        double radius)
    {
        m_dc.DoDrawRoundedRectangle(GetX(x, y), GetY(x, y),
                                    GetX(w, h), GetY(w, h),
                                    radius);
    }

    virtual void DoDrawEllipse(wxCoord x, wxCoord y, wxCoord w, wxCoord h)
    {
        m_dc.DoDrawEllipse(GetX(x, y), GetY(x, y), GetX(w, h), GetY(w, h));
    }

    virtual void DoCrossHair(wxCoord x, wxCoord y)
    {
        m_dc.DoCrossHair(GetX(x, y), GetY(x, y));
    }

    virtual void DoDrawIcon(const wxIcon& icon, wxCoord x, wxCoord y)
    {
        m_dc.DoDrawIcon(icon, GetX(x, y), GetY(x, y));
    }

    virtual void DoDrawBitmap(const wxBitmap &bmp, wxCoord x, wxCoord y,
                              bool useMask = false)
    {
        m_dc.DoDrawBitmap(bmp, GetX(x, y), GetY(x, y), useMask);
    }

    virtual void DoDrawText(const wxString& text, wxCoord x, wxCoord y)
    {
        // this is never mirrored
        m_dc.DoDrawText(text, x, y);
    }

    virtual void DoDrawRotatedText(const wxString& text,
                                   wxCoord x, wxCoord y, double angle)
    {
        // this is never mirrored
        m_dc.DoDrawRotatedText(text, x, y, angle);
    }

    virtual bool DoBlit(wxCoord xdest, wxCoord ydest,
                        wxCoord w, wxCoord h,
                        wxDC *source, wxCoord xsrc, wxCoord ysrc,
                        int rop = wxCOPY, bool useMask = false,
                        wxCoord xsrcMask = wxDefaultCoord, wxCoord ysrcMask = wxDefaultCoord)
    {
        return m_dc.DoBlit(GetX(xdest, ydest), GetY(xdest, ydest),
                           GetX(w, h), GetY(w, h),
                           source, GetX(xsrc, ysrc), GetY(xsrc, ysrc),
                           rop, useMask,
                           GetX(xsrcMask, ysrcMask), GetX(xsrcMask, ysrcMask));
    }

    virtual void DoGetSize(int *w, int *h) const
    {
        m_dc.DoGetSize(GetX(w, h), GetY(w, h));
    }

    virtual void DoGetSizeMM(int *w, int *h) const
    {
        m_dc.DoGetSizeMM(GetX(w, h), GetY(w, h));
    }

    virtual void DoDrawLines(int n, wxPoint points[],
                             wxCoord xoffset, wxCoord yoffset)
    {
        Mirror(n, points);

        m_dc.DoDrawLines(n, points,
                         GetX(xoffset, yoffset), GetY(xoffset, yoffset));

        Mirror(n, points);
    }

    virtual void DoDrawPolygon(int n, wxPoint points[],
                               wxCoord xoffset, wxCoord yoffset,
                               int fillStyle = wxODDEVEN_RULE)
    {
        Mirror(n, points);

        m_dc.DoDrawPolygon(n, points,
                           GetX(xoffset, yoffset), GetY(xoffset, yoffset),
                           fillStyle);

        Mirror(n, points);
    }

    virtual void DoSetClippingRegionAsRegion(const wxRegion& WXUNUSED(region))
    {
        wxFAIL_MSG( wxT("not implemented") );
    }

    virtual void DoSetClippingRegion(wxCoord x, wxCoord y,
                                     wxCoord w, wxCoord h)
    {
        m_dc.DoSetClippingRegion(GetX(x, y), GetY(x, y), GetX(w, h), GetY(w, h));
    }

    virtual void DoGetTextExtent(const wxString& string,
                                 wxCoord *x, wxCoord *y,
                                 wxCoord *descent = NULL,
                                 wxCoord *externalLeading = NULL,
                                 wxFont *theFont = NULL) const
    {
        // never mirrored
        m_dc.DoGetTextExtent(string, x, y, descent, externalLeading, theFont);
    }

private:
    wxMirrorDC& m_dc;

    bool m_mirror;

    DECLARE_NO_COPY_CLASS(wxMirrorDC)
};

#endif // _WX_DCMIRROR_H_

