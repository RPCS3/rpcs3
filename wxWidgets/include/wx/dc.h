/////////////////////////////////////////////////////////////////////////////
// Name:        wx/dc.h
// Purpose:     wxDC class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     05/25/99
// RCS-ID:      $Id: dc.h 59768 2009-03-23 12:35:12Z VZ $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DC_H_BASE_
#define _WX_DC_H_BASE_

// ----------------------------------------------------------------------------
// headers which we must include here
// ----------------------------------------------------------------------------

#include "wx/object.h"          // the base class

#include "wx/intl.h"            // for wxLayoutDirection
#include "wx/cursor.h"          // we have member variables of these classes
#include "wx/font.h"            // so we can't do without them
#include "wx/colour.h"
#include "wx/bitmap.h"          // for wxNullBitmap
#include "wx/brush.h"
#include "wx/pen.h"
#include "wx/palette.h"
#include "wx/list.h"            // we use wxList in inline functions
#include "wx/dynarray.h"
#include "wx/math.h"

class WXDLLIMPEXP_FWD_CORE wxDC;
class WXDLLIMPEXP_FWD_CORE wxDCBase;

class WXDLLEXPORT wxDrawObject
{
public:

    wxDrawObject()
        : m_isBBoxValid(false)
        , m_minX(0), m_minY(0), m_maxX(0), m_maxY(0)
    { }

    virtual ~wxDrawObject() { }

    virtual void Draw(wxDCBase&) const { }

    virtual void CalcBoundingBox(wxCoord x, wxCoord y)
    {
      if ( m_isBBoxValid )
      {
         if ( x < m_minX ) m_minX = x;
         if ( y < m_minY ) m_minY = y;
         if ( x > m_maxX ) m_maxX = x;
         if ( y > m_maxY ) m_maxY = y;
      }
      else
      {
         m_isBBoxValid = true;

         m_minX = x;
         m_minY = y;
         m_maxX = x;
         m_maxY = y;
      }
    }

    void ResetBoundingBox()
    {
        m_isBBoxValid = false;

        m_minX = m_maxX = m_minY = m_maxY = 0;
    }

    // Get the final bounding box of the PostScript or Metafile picture.

    wxCoord MinX() const { return m_minX; }
    wxCoord MaxX() const { return m_maxX; }
    wxCoord MinY() const { return m_minY; }
    wxCoord MaxY() const { return m_maxY; }

    //to define the type of object for derived objects
    virtual int GetType()=0;

protected:
    //for boundingbox calculation
    bool m_isBBoxValid:1;
    //for boundingbox calculation
    wxCoord m_minX, m_minY, m_maxX, m_maxY;
};

// ---------------------------------------------------------------------------
// global variables
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// wxDC is the device context - object on which any drawing is done
// ---------------------------------------------------------------------------

class WXDLLEXPORT wxDCBase : public wxObject
{
public:
    wxDCBase()
        : m_colour(wxColourDisplay())
        , m_ok(true)
        , m_clipping(false)
        , m_isInteractive(0)
        , m_isBBoxValid(false)
        , m_logicalOriginX(0), m_logicalOriginY(0)
        , m_deviceOriginX(0), m_deviceOriginY(0)
        , m_logicalScaleX(1.0), m_logicalScaleY(1.0)
        , m_userScaleX(1.0), m_userScaleY(1.0)
        , m_scaleX(1.0), m_scaleY(1.0)
        , m_signX(1), m_signY(1)
        , m_minX(0), m_minY(0), m_maxX(0), m_maxY(0)
        , m_clipX1(0), m_clipY1(0), m_clipX2(0), m_clipY2(0)
        , m_logicalFunction(wxCOPY)
        , m_backgroundMode(wxTRANSPARENT)
        , m_mappingMode(wxMM_TEXT)
        , m_pen()
        , m_brush()
        , m_backgroundBrush(*wxTRANSPARENT_BRUSH)
        , m_textForegroundColour(*wxBLACK)
        , m_textBackgroundColour(*wxWHITE)
        , m_font()
#if wxUSE_PALETTE
        , m_palette()
        , m_hasCustomPalette(false)
#endif // wxUSE_PALETTE
    {
        ResetBoundingBox();
        ResetClipping();
    }

    virtual ~wxDCBase() { }

    // graphic primitives
    // ------------------

    virtual void DrawObject(wxDrawObject* drawobject)
    {
        drawobject->Draw(*this);
        CalcBoundingBox(drawobject->MinX(),drawobject->MinY());
        CalcBoundingBox(drawobject->MaxX(),drawobject->MaxY());
    }

    bool FloodFill(wxCoord x, wxCoord y, const wxColour& col,
                   int style = wxFLOOD_SURFACE)
        { return DoFloodFill(x, y, col, style); }
    bool FloodFill(const wxPoint& pt, const wxColour& col,
                   int style = wxFLOOD_SURFACE)
        { return DoFloodFill(pt.x, pt.y, col, style); }

    // fill the area specified by rect with a radial gradient, starting from
    // initialColour in the centre of the cercle and fading to destColour.
    void GradientFillConcentric(const wxRect& rect,
                                const wxColour& initialColour,
                                const wxColour& destColour)
        { GradientFillConcentric(rect, initialColour, destColour,
                                 wxPoint(rect.GetWidth() / 2,
                                         rect.GetHeight() / 2)); }

    void GradientFillConcentric(const wxRect& rect,
                                const wxColour& initialColour,
                                const wxColour& destColour,
                                const wxPoint& circleCenter)
        { DoGradientFillConcentric(rect, initialColour, destColour, circleCenter); }

    // fill the area specified by rect with a linear gradient
    void GradientFillLinear(const wxRect& rect,
                            const wxColour& initialColour,
                            const wxColour& destColour,
                            wxDirection nDirection = wxEAST)
        { DoGradientFillLinear(rect, initialColour, destColour, nDirection); }

    bool GetPixel(wxCoord x, wxCoord y, wxColour *col) const
        { return DoGetPixel(x, y, col); }
    bool GetPixel(const wxPoint& pt, wxColour *col) const
        { return DoGetPixel(pt.x, pt.y, col); }

    void DrawLine(wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2)
        { DoDrawLine(x1, y1, x2, y2); }
    void DrawLine(const wxPoint& pt1, const wxPoint& pt2)
        { DoDrawLine(pt1.x, pt1.y, pt2.x, pt2.y); }

    void CrossHair(wxCoord x, wxCoord y)
        { DoCrossHair(x, y); }
    void CrossHair(const wxPoint& pt)
        { DoCrossHair(pt.x, pt.y); }

    void DrawArc(wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2,
                 wxCoord xc, wxCoord yc)
        { DoDrawArc(x1, y1, x2, y2, xc, yc); }
    void DrawArc(const wxPoint& pt1, const wxPoint& pt2, const wxPoint& centre)
        { DoDrawArc(pt1.x, pt1.y, pt2.x, pt2.y, centre.x, centre.y); }

    void DrawCheckMark(wxCoord x, wxCoord y,
                       wxCoord width, wxCoord height)
        { DoDrawCheckMark(x, y, width, height); }
    void DrawCheckMark(const wxRect& rect)
        { DoDrawCheckMark(rect.x, rect.y, rect.width, rect.height); }

    void DrawEllipticArc(wxCoord x, wxCoord y, wxCoord w, wxCoord h,
                         double sa, double ea)
        { DoDrawEllipticArc(x, y, w, h, sa, ea); }
    void DrawEllipticArc(const wxPoint& pt, const wxSize& sz,
                         double sa, double ea)
        { DoDrawEllipticArc(pt.x, pt.y, sz.x, sz.y, sa, ea); }

    void DrawPoint(wxCoord x, wxCoord y)
        { DoDrawPoint(x, y); }
    void DrawPoint(const wxPoint& pt)
        { DoDrawPoint(pt.x, pt.y); }

    void DrawLines(int n, wxPoint points[],
                   wxCoord xoffset = 0, wxCoord yoffset = 0)
        { DoDrawLines(n, points, xoffset, yoffset); }
    void DrawLines(const wxList *list,
                   wxCoord xoffset = 0, wxCoord yoffset = 0);

    void DrawPolygon(int n, wxPoint points[],
                     wxCoord xoffset = 0, wxCoord yoffset = 0,
                     int fillStyle = wxODDEVEN_RULE)
        { DoDrawPolygon(n, points, xoffset, yoffset, fillStyle); }

    void DrawPolygon(const wxList *list,
                     wxCoord xoffset = 0, wxCoord yoffset = 0,
                     int fillStyle = wxODDEVEN_RULE);

    void DrawPolyPolygon(int n, int count[], wxPoint points[],
                         wxCoord xoffset = 0, wxCoord yoffset = 0,
                         int fillStyle = wxODDEVEN_RULE)
        { DoDrawPolyPolygon(n, count, points, xoffset, yoffset, fillStyle); }

    void DrawRectangle(wxCoord x, wxCoord y, wxCoord width, wxCoord height)
        { DoDrawRectangle(x, y, width, height); }
    void DrawRectangle(const wxPoint& pt, const wxSize& sz)
        { DoDrawRectangle(pt.x, pt.y, sz.x, sz.y); }
    void DrawRectangle(const wxRect& rect)
        { DoDrawRectangle(rect.x, rect.y, rect.width, rect.height); }

    void DrawRoundedRectangle(wxCoord x, wxCoord y, wxCoord width, wxCoord height,
                              double radius)
        { DoDrawRoundedRectangle(x, y, width, height, radius); }
    void DrawRoundedRectangle(const wxPoint& pt, const wxSize& sz,
                             double radius)
        { DoDrawRoundedRectangle(pt.x, pt.y, sz.x, sz.y, radius); }
    void DrawRoundedRectangle(const wxRect& r, double radius)
        { DoDrawRoundedRectangle(r.x, r.y, r.width, r.height, radius); }

    void DrawCircle(wxCoord x, wxCoord y, wxCoord radius)
        { DoDrawEllipse(x - radius, y - radius, 2*radius, 2*radius); }
    void DrawCircle(const wxPoint& pt, wxCoord radius)
        { DrawCircle(pt.x, pt.y, radius); }

    void DrawEllipse(wxCoord x, wxCoord y, wxCoord width, wxCoord height)
        { DoDrawEllipse(x, y, width, height); }
    void DrawEllipse(const wxPoint& pt, const wxSize& sz)
        { DoDrawEllipse(pt.x, pt.y, sz.x, sz.y); }
    void DrawEllipse(const wxRect& rect)
        { DoDrawEllipse(rect.x, rect.y, rect.width, rect.height); }

    void DrawIcon(const wxIcon& icon, wxCoord x, wxCoord y)
        { DoDrawIcon(icon, x, y); }
    void DrawIcon(const wxIcon& icon, const wxPoint& pt)
        { DoDrawIcon(icon, pt.x, pt.y); }

    void DrawBitmap(const wxBitmap &bmp, wxCoord x, wxCoord y,
                    bool useMask = false)
        { DoDrawBitmap(bmp, x, y, useMask); }
    void DrawBitmap(const wxBitmap &bmp, const wxPoint& pt,
                    bool useMask = false)
        { DoDrawBitmap(bmp, pt.x, pt.y, useMask); }

    void DrawText(const wxString& text, wxCoord x, wxCoord y)
        { DoDrawText(text, x, y); }
    void DrawText(const wxString& text, const wxPoint& pt)
        { DoDrawText(text, pt.x, pt.y); }

    void DrawRotatedText(const wxString& text, wxCoord x, wxCoord y, double angle)
        { DoDrawRotatedText(text, x, y, angle); }
    void DrawRotatedText(const wxString& text, const wxPoint& pt, double angle)
        { DoDrawRotatedText(text, pt.x, pt.y, angle); }

    // this version puts both optional bitmap and the text into the given
    // rectangle and aligns is as specified by alignment parameter; it also
    // will emphasize the character with the given index if it is != -1 and
    // return the bounding rectangle if required
    virtual void DrawLabel(const wxString& text,
                           const wxBitmap& image,
                           const wxRect& rect,
                           int alignment = wxALIGN_LEFT | wxALIGN_TOP,
                           int indexAccel = -1,
                           wxRect *rectBounding = NULL);

    void DrawLabel(const wxString& text, const wxRect& rect,
                   int alignment = wxALIGN_LEFT | wxALIGN_TOP,
                   int indexAccel = -1)
        { DrawLabel(text, wxNullBitmap, rect, alignment, indexAccel); }

    bool Blit(wxCoord xdest, wxCoord ydest, wxCoord width, wxCoord height,
              wxDC *source, wxCoord xsrc, wxCoord ysrc,
              int rop = wxCOPY, bool useMask = false, wxCoord xsrcMask = wxDefaultCoord, wxCoord ysrcMask = wxDefaultCoord)
    {
        return DoBlit(xdest, ydest, width, height,
                      source, xsrc, ysrc, rop, useMask, xsrcMask, ysrcMask);
    }
    bool Blit(const wxPoint& destPt, const wxSize& sz,
              wxDC *source, const wxPoint& srcPt,
              int rop = wxCOPY, bool useMask = false, const wxPoint& srcPtMask = wxDefaultPosition)
    {
        return DoBlit(destPt.x, destPt.y, sz.x, sz.y,
                      source, srcPt.x, srcPt.y, rop, useMask, srcPtMask.x, srcPtMask.y);
    }
    
    wxBitmap GetAsBitmap(const wxRect *subrect = (const wxRect *) NULL) const
    {
        return DoGetAsBitmap(subrect);
    }

#if wxUSE_SPLINES
    // TODO: this API needs fixing (wxPointList, why (!const) "wxList *"?)
    void DrawSpline(wxCoord x1, wxCoord y1,
                    wxCoord x2, wxCoord y2,
                    wxCoord x3, wxCoord y3);
    void DrawSpline(int n, wxPoint points[]);

    void DrawSpline(wxList *points) { DoDrawSpline(points); }
#endif // wxUSE_SPLINES

    // Eventually we will have wxUSE_GENERIC_DRAWELLIPSE
#ifdef __WXWINCE__
    //! Generic method to draw ellipses, circles and arcs with current pen and brush.
    /*! \param x Upper left corner of bounding box.
     *  \param y Upper left corner of bounding box.
     *  \param w Width of bounding box.
     *  \param h Height of bounding box.
     *  \param sa Starting angle of arc
     *            (counterclockwise, start at 3 o'clock, 360 is full circle).
     *  \param ea Ending angle of arc.
     *  \param angle Rotation angle, the Arc will be rotated after
     *               calculating begin and end.
     */
    void DrawEllipticArcRot( wxCoord x, wxCoord y,
                             wxCoord width, wxCoord height,
                             double sa = 0, double ea = 0, double angle = 0 )
    { DoDrawEllipticArcRot( x, y, width, height, sa, ea, angle ); }

    void DrawEllipticArcRot( const wxPoint& pt,
                             const wxSize& sz,
                             double sa = 0, double ea = 0, double angle = 0 )
    { DoDrawEllipticArcRot( pt.x, pt.y, sz.x, sz.y, sa, ea, angle ); }

    void DrawEllipticArcRot( const wxRect& rect,
                             double sa = 0, double ea = 0, double angle = 0 )
    { DoDrawEllipticArcRot( rect.x, rect.y, rect.width, rect.height, sa, ea, angle ); }

    virtual void DoDrawEllipticArcRot( wxCoord x, wxCoord y,
                                       wxCoord w, wxCoord h,
                                       double sa = 0, double ea = 0, double angle = 0 );

    //! Rotates points around center.
    /*! This is a quite straight method, it calculates in pixels
     *  and so it produces rounding errors.
     *  \param points The points inside will be rotated.
     *  \param angle Rotating angle (counterclockwise, start at 3 o'clock, 360 is full circle).
     *  \param center Center of rotation.
     */
    void Rotate( wxList* points, double angle, wxPoint center = wxPoint(0,0) );

    // used by DrawEllipticArcRot
    // Careful: wxList gets filled with points you have to delete later.
    void CalculateEllipticPoints( wxList* points,
                                  wxCoord xStart, wxCoord yStart,
                                  wxCoord w, wxCoord h,
                                  double sa, double ea );
#endif

    // global DC operations
    // --------------------

    virtual void Clear() = 0;

    virtual bool StartDoc(const wxString& WXUNUSED(message)) { return true; }
    virtual void EndDoc() { }

    virtual void StartPage() { }
    virtual void EndPage() { }

#if WXWIN_COMPATIBILITY_2_6
    wxDEPRECATED( void BeginDrawing() );
    wxDEPRECATED( void EndDrawing() );
#endif // WXWIN_COMPATIBILITY_2_6


    // set objects to use for drawing
    // ------------------------------

    virtual void SetFont(const wxFont& font) = 0;
    virtual void SetPen(const wxPen& pen) = 0;
    virtual void SetBrush(const wxBrush& brush) = 0;
    virtual void SetBackground(const wxBrush& brush) = 0;
    virtual void SetBackgroundMode(int mode) = 0;
#if wxUSE_PALETTE
    virtual void SetPalette(const wxPalette& palette) = 0;
#endif // wxUSE_PALETTE

    // clipping region
    // ---------------

    void SetClippingRegion(wxCoord x, wxCoord y, wxCoord width, wxCoord height)
        { DoSetClippingRegion(x, y, width, height); }
    void SetClippingRegion(const wxPoint& pt, const wxSize& sz)
        { DoSetClippingRegion(pt.x, pt.y, sz.x, sz.y); }
    void SetClippingRegion(const wxRect& rect)
        { DoSetClippingRegion(rect.x, rect.y, rect.width, rect.height); }
    void SetClippingRegion(const wxRegion& region)
        { DoSetClippingRegionAsRegion(region); }

#if wxABI_VERSION >= 20811
    void SetDeviceClippingRegion(const wxRegion& region)
        { DoSetClippingRegionAsRegion(region); }
#endif

    virtual void DestroyClippingRegion() { ResetClipping(); }

    void GetClippingBox(wxCoord *x, wxCoord *y, wxCoord *w, wxCoord *h) const
        { DoGetClippingBox(x, y, w, h); }
    void GetClippingBox(wxRect& rect) const
        {
          DoGetClippingBox(&rect.x, &rect.y, &rect.width, &rect.height);
        }

    // text extent
    // -----------

    virtual wxCoord GetCharHeight() const = 0;
    virtual wxCoord GetCharWidth() const = 0;

    // only works for single line strings
    void GetTextExtent(const wxString& string,
                       wxCoord *x, wxCoord *y,
                       wxCoord *descent = NULL,
                       wxCoord *externalLeading = NULL,
                       wxFont *theFont = NULL) const
        { DoGetTextExtent(string, x, y, descent, externalLeading, theFont); }

    wxSize GetTextExtent(const wxString& string) const
    {
        wxCoord w, h;
        DoGetTextExtent(string, &w, &h);
        return wxSize(w, h);
    }

    // works for single as well as multi-line strings
    virtual void GetMultiLineTextExtent(const wxString& string,
                                        wxCoord *width,
                                        wxCoord *height,
                                        wxCoord *heightLine = NULL,
                                        wxFont *font = NULL) const;

    wxSize GetMultiLineTextExtent(const wxString& string) const
    {
        wxCoord w, h;
        GetMultiLineTextExtent(string, &w, &h);
        return wxSize(w, h);
    }

    // Measure cumulative width of text after each character
    bool GetPartialTextExtents(const wxString& text, wxArrayInt& widths) const
        { return DoGetPartialTextExtents(text, widths); }

    // size and resolution
    // -------------------

    // in device units
    void GetSize(int *width, int *height) const
        { DoGetSize(width, height); }
    wxSize GetSize() const
    {
        int w, h;
        DoGetSize(&w, &h);

        return wxSize(w, h);
    }

    // in mm
    void GetSizeMM(int* width, int* height) const
        { DoGetSizeMM(width, height); }
    wxSize GetSizeMM() const
    {
        int w, h;
        DoGetSizeMM(&w, &h);

        return wxSize(w, h);
    }

    // coordinates conversions
    // -----------------------

    // This group of functions does actual conversion of the input, as you'd
    // expect.
    wxCoord DeviceToLogicalX(wxCoord x) const;
    wxCoord DeviceToLogicalY(wxCoord y) const;
    wxCoord DeviceToLogicalXRel(wxCoord x) const;
    wxCoord DeviceToLogicalYRel(wxCoord y) const;
    wxCoord LogicalToDeviceX(wxCoord x) const;
    wxCoord LogicalToDeviceY(wxCoord y) const;
    wxCoord LogicalToDeviceXRel(wxCoord x) const;
    wxCoord LogicalToDeviceYRel(wxCoord y) const;

    // query DC capabilities
    // ---------------------

    virtual bool CanDrawBitmap() const = 0;
    virtual bool CanGetTextExtent() const = 0;

    // colour depth
    virtual int GetDepth() const = 0;

    // Resolution in Pixels per inch
    virtual wxSize GetPPI() const = 0;

    virtual bool Ok() const { return IsOk(); }
    virtual bool IsOk() const { return m_ok; }

    // accessors and setters
    // ---------------------

    virtual int GetBackgroundMode() const { return m_backgroundMode; }
    virtual const wxBrush&  GetBackground() const { return m_backgroundBrush; }
    virtual const wxBrush&  GetBrush() const { return m_brush; }
    virtual const wxFont&   GetFont() const { return m_font; }
    virtual const wxPen&    GetPen() const { return m_pen; }

    virtual const wxColour& GetTextForeground() const { return m_textForegroundColour; }
    virtual const wxColour& GetTextBackground() const { return m_textBackgroundColour; }
    virtual void SetTextForeground(const wxColour& colour)
        { m_textForegroundColour = colour; }
    virtual void SetTextBackground(const wxColour& colour)
        { m_textBackgroundColour = colour; }

    virtual int GetMapMode() const { return m_mappingMode; }
    virtual void SetMapMode(int mode) = 0;

    virtual void GetUserScale(double *x, double *y) const
    {
        if ( x ) *x = m_userScaleX;
        if ( y ) *y = m_userScaleY;
    }
    virtual void SetUserScale(double x, double y) = 0;

    virtual void GetLogicalScale(double *x, double *y)
    {
        if ( x ) *x = m_logicalScaleX;
        if ( y ) *y = m_logicalScaleY;
    }
    virtual void SetLogicalScale(double x, double y)
    {
        m_logicalScaleX = x;
        m_logicalScaleY = y;
    }

    void GetLogicalOrigin(wxCoord *x, wxCoord *y) const
        { DoGetLogicalOrigin(x, y); }
    wxPoint GetLogicalOrigin() const
        { wxCoord x, y; DoGetLogicalOrigin(&x, &y); return wxPoint(x, y); }
    virtual void SetLogicalOrigin(wxCoord x, wxCoord y) = 0;

    void GetDeviceOrigin(wxCoord *x, wxCoord *y) const
        { DoGetDeviceOrigin(x, y); }
    wxPoint GetDeviceOrigin() const
        { wxCoord x, y; DoGetDeviceOrigin(&x, &y); return wxPoint(x, y); }
    virtual void SetDeviceOrigin(wxCoord x, wxCoord y) = 0;

    virtual void ComputeScaleAndOrigin() {}

    virtual void SetAxisOrientation(bool xLeftRight, bool yBottomUp) = 0;

    virtual int GetLogicalFunction() const { return m_logicalFunction; }
    virtual void SetLogicalFunction(int function) = 0;

#if WXWIN_COMPATIBILITY_2_4
    virtual void SetOptimization(bool WXUNUSED(opt)) { }
    virtual bool GetOptimization() { return false; }
#endif

    // bounding box
    // ------------

    virtual void CalcBoundingBox(wxCoord x, wxCoord y)
    {
      if ( m_isBBoxValid )
      {
         if ( x < m_minX ) m_minX = x;
         if ( y < m_minY ) m_minY = y;
         if ( x > m_maxX ) m_maxX = x;
         if ( y > m_maxY ) m_maxY = y;
      }
      else
      {
         m_isBBoxValid = true;

         m_minX = x;
         m_minY = y;
         m_maxX = x;
         m_maxY = y;
      }
    }

    void ResetBoundingBox()
    {
        m_isBBoxValid = false;

        m_minX = m_maxX = m_minY = m_maxY = 0;
    }

    // Get the final bounding box of the PostScript or Metafile picture.
    wxCoord MinX() const { return m_minX; }
    wxCoord MaxX() const { return m_maxX; }
    wxCoord MinY() const { return m_minY; }
    wxCoord MaxY() const { return m_maxY; }

    // misc old functions
    // ------------------

    // for compatibility with the old code when wxCoord was long everywhere
    void GetTextExtent(const wxString& string,
                       long *x, long *y,
                       long *descent = NULL,
                       long *externalLeading = NULL,
                       wxFont *theFont = NULL) const
    {
        wxCoord x2, y2, descent2, externalLeading2;
        DoGetTextExtent(string, &x2, &y2,
                        &descent2, &externalLeading2,
                        theFont);
        if ( x )
            *x = x2;
        if ( y )
            *y = y2;
        if ( descent )
            *descent = descent2;
        if ( externalLeading )
            *externalLeading = externalLeading2;
    }

    void GetLogicalOrigin(long *x, long *y) const
    {
        wxCoord x2, y2;
        DoGetLogicalOrigin(&x2, &y2);
        if ( x )
            *x = x2;
        if ( y )
            *y = y2;
    }

    void GetDeviceOrigin(long *x, long *y) const
    {
        wxCoord x2, y2;
        DoGetDeviceOrigin(&x2, &y2);
        if ( x )
            *x = x2;
        if ( y )
            *y = y2;
    }
    void GetClippingBox(long *x, long *y, long *w, long *h) const
    {
        wxCoord xx,yy,ww,hh;
        DoGetClippingBox(&xx, &yy, &ww, &hh);
        if (x) *x = xx;
        if (y) *y = yy;
        if (w) *w = ww;
        if (h) *h = hh;
    }

    // RTL related functions
    // ---------------------

    // get or change the layout direction (LTR or RTL) for this dc,
    // wxLayout_Default is returned if layout direction is not supported
    virtual wxLayoutDirection GetLayoutDirection() const
        { return wxLayout_Default; }
    virtual void SetLayoutDirection(wxLayoutDirection WXUNUSED(dir))
        { }

protected:
    // the pure virtual functions which should be implemented by wxDC
    virtual bool DoFloodFill(wxCoord x, wxCoord y, const wxColour& col,
                             int style = wxFLOOD_SURFACE) = 0;

    virtual void DoGradientFillLinear(const wxRect& rect,
                                      const wxColour& initialColour,
                                      const wxColour& destColour,
                                      wxDirection nDirection = wxEAST);

    virtual void DoGradientFillConcentric(const wxRect& rect,
                                        const wxColour& initialColour,
                                        const wxColour& destColour,
                                        const wxPoint& circleCenter);

    virtual bool DoGetPixel(wxCoord x, wxCoord y, wxColour *col) const = 0;

    virtual void DoDrawPoint(wxCoord x, wxCoord y) = 0;
    virtual void DoDrawLine(wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2) = 0;

    virtual void DoDrawArc(wxCoord x1, wxCoord y1,
                           wxCoord x2, wxCoord y2,
                           wxCoord xc, wxCoord yc) = 0;
    virtual void DoDrawCheckMark(wxCoord x, wxCoord y,
                                 wxCoord width, wxCoord height);
    virtual void DoDrawEllipticArc(wxCoord x, wxCoord y, wxCoord w, wxCoord h,
                                   double sa, double ea) = 0;

    virtual void DoDrawRectangle(wxCoord x, wxCoord y, wxCoord width, wxCoord height) = 0;
    virtual void DoDrawRoundedRectangle(wxCoord x, wxCoord y,
                                        wxCoord width, wxCoord height,
                                        double radius) = 0;
    virtual void DoDrawEllipse(wxCoord x, wxCoord y,
                               wxCoord width, wxCoord height) = 0;

    virtual void DoCrossHair(wxCoord x, wxCoord y) = 0;

    virtual void DoDrawIcon(const wxIcon& icon, wxCoord x, wxCoord y) = 0;
    virtual void DoDrawBitmap(const wxBitmap &bmp, wxCoord x, wxCoord y,
                              bool useMask = false) = 0;

    virtual void DoDrawText(const wxString& text, wxCoord x, wxCoord y) = 0;
    virtual void DoDrawRotatedText(const wxString& text,
                                   wxCoord x, wxCoord y, double angle) = 0;

    virtual bool DoBlit(wxCoord xdest, wxCoord ydest,
                        wxCoord width, wxCoord height,
                        wxDC *source, wxCoord xsrc, wxCoord ysrc,
                        int rop = wxCOPY, bool useMask = false, wxCoord xsrcMask = wxDefaultCoord, wxCoord ysrcMask = wxDefaultCoord) = 0;

    virtual wxBitmap DoGetAsBitmap(const wxRect *WXUNUSED(subrect)) const { return wxNullBitmap; }

    virtual void DoGetSize(int *width, int *height) const = 0;
    virtual void DoGetSizeMM(int* width, int* height) const = 0;

    virtual void DoDrawLines(int n, wxPoint points[],
                             wxCoord xoffset, wxCoord yoffset) = 0;
    virtual void DoDrawPolygon(int n, wxPoint points[],
                               wxCoord xoffset, wxCoord yoffset,
                               int fillStyle = wxODDEVEN_RULE) = 0;
    virtual void DoDrawPolyPolygon(int n, int count[], wxPoint points[],
                               wxCoord xoffset, wxCoord yoffset,
                               int fillStyle);

    virtual void DoSetClippingRegionAsRegion(const wxRegion& region) = 0;
    virtual void DoSetClippingRegion(wxCoord x, wxCoord y,
                                     wxCoord width, wxCoord height) = 0;

#if WXWIN_COMPATIBILITY_2_4
    // this was only for confusing people, use DoGetClippingBox only
    virtual void DoGetClippingRegion(wxCoord *x, wxCoord *y,
                                     wxCoord *w, wxCoord *h)
        { DoGetClippingBox(x, y, w, h); }
#endif

    virtual void DoGetClippingBox(wxCoord *x, wxCoord *y,
                                  wxCoord *w, wxCoord *h) const
    {
        if ( x )
            *x = m_clipX1;
        if ( y )
            *y = m_clipY1;
        if ( w )
            *w = m_clipX2 - m_clipX1;
        if ( h )
            *h = m_clipY2 - m_clipY1;
    }

    virtual void DoGetLogicalOrigin(wxCoord *x, wxCoord *y) const
    {
        if ( x ) *x = m_logicalOriginX;
        if ( y ) *y = m_logicalOriginY;
    }

    virtual void DoGetDeviceOrigin(wxCoord *x, wxCoord *y) const
    {
        if ( x ) *x = m_deviceOriginX;
        if ( y ) *y = m_deviceOriginY;
    }

    virtual void DoGetTextExtent(const wxString& string,
                                 wxCoord *x, wxCoord *y,
                                 wxCoord *descent = NULL,
                                 wxCoord *externalLeading = NULL,
                                 wxFont *theFont = NULL) const = 0;

    virtual bool DoGetPartialTextExtents(const wxString& text, wxArrayInt& widths) const;

#if wxUSE_SPLINES
    virtual void DoDrawSpline(wxList *points);
#endif

#if wxABI_VERSION >= 20810
    // returns adjustment factor for converting wxFont "point size"; in wx
    // it is point size on screen and needs to be multiplied by this value
    // for rendering on higher-resolution DCs such as printer ones
    static float GetFontPointSizeAdjustment(float dpi);
#endif

protected:
    // unset clipping variables (after clipping region was destroyed)
    void ResetClipping()
    {
        m_clipping = false;

        m_clipX1 = m_clipX2 = m_clipY1 = m_clipY2 = 0;
    }

    // flags
    bool m_colour:1;
    bool m_ok:1;
    bool m_clipping:1;
    bool m_isInteractive:1;
    bool m_isBBoxValid:1;

    // coordinate system variables

    // TODO short descriptions of what exactly they are would be nice...

    wxCoord m_logicalOriginX, m_logicalOriginY;
    wxCoord m_deviceOriginX, m_deviceOriginY;

    double m_logicalScaleX, m_logicalScaleY;
    double m_userScaleX, m_userScaleY;
    double m_scaleX, m_scaleY;

    // Used by SetAxisOrientation() to invert the axes
    int m_signX, m_signY;

    // bounding and clipping boxes
    wxCoord m_minX, m_minY, m_maxX, m_maxY;
    wxCoord m_clipX1, m_clipY1, m_clipX2, m_clipY2;

    int m_logicalFunction;
    int m_backgroundMode;
    int m_mappingMode;

    // GDI objects
    wxPen             m_pen;
    wxBrush           m_brush;
    wxBrush           m_backgroundBrush;
    wxColour          m_textForegroundColour;
    wxColour          m_textBackgroundColour;
    wxFont            m_font;

#if wxUSE_PALETTE
    wxPalette         m_palette;
    bool              m_hasCustomPalette;
#endif // wxUSE_PALETTE

private:
    DECLARE_NO_COPY_CLASS(wxDCBase)
    DECLARE_ABSTRACT_CLASS(wxDCBase)
};

// ----------------------------------------------------------------------------
// now include the declaration of wxDC class
// ----------------------------------------------------------------------------

#if defined(__WXPALMOS__)
    #include "wx/palmos/dc.h"
#elif defined(__WXMSW__)
    #include "wx/msw/dc.h"
#elif defined(__WXMOTIF__)
    #include "wx/motif/dc.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/dc.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/dc.h"
#elif defined(__WXX11__)
    #include "wx/x11/dc.h"
#elif defined(__WXMGL__)
    #include "wx/mgl/dc.h"
#elif defined(__WXDFB__)
    #include "wx/dfb/dc.h"
#elif defined(__WXMAC__)
    #include "wx/mac/dc.h"
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/dc.h"
#elif defined(__WXPM__)
    #include "wx/os2/dc.h"
#endif

#if wxUSE_GRAPHICS_CONTEXT
    #include "wx/dcgraph.h"
#endif

// ----------------------------------------------------------------------------
// helper class: you can use it to temporarily change the DC text colour and
// restore it automatically when the object goes out of scope
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxDCTextColourChanger
{
public:
    wxDCTextColourChanger(wxDC& dc) : m_dc(dc), m_colFgOld() { }

    wxDCTextColourChanger(wxDC& dc, const wxColour& col) : m_dc(dc)
    {
        Set(col);
    }

    ~wxDCTextColourChanger()
    {
        if ( m_colFgOld.Ok() )
            m_dc.SetTextForeground(m_colFgOld);
    }

    void Set(const wxColour& col)
    {
        if ( !m_colFgOld.Ok() )
            m_colFgOld = m_dc.GetTextForeground();
        m_dc.SetTextForeground(col);
    }

private:
    wxDC& m_dc;

    wxColour m_colFgOld;

    DECLARE_NO_COPY_CLASS(wxDCTextColourChanger)
};

// ----------------------------------------------------------------------------
// helper class: you can use it to temporarily change the DC pen and
// restore it automatically when the object goes out of scope
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxDCPenChanger
{
public:
    wxDCPenChanger(wxDC& dc, const wxPen& pen) : m_dc(dc), m_penOld(dc.GetPen())
    {
        m_dc.SetPen(pen);
    }

    ~wxDCPenChanger()
    {
        if ( m_penOld.Ok() )
            m_dc.SetPen(m_penOld);
    }

private:
    wxDC& m_dc;

    wxPen m_penOld;

    DECLARE_NO_COPY_CLASS(wxDCPenChanger)
};

// ----------------------------------------------------------------------------
// helper class: you can use it to temporarily change the DC brush and
// restore it automatically when the object goes out of scope
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxDCBrushChanger
{
public:
    wxDCBrushChanger(wxDC& dc, const wxBrush& brush) : m_dc(dc), m_brushOld(dc.GetBrush())
    {
        m_dc.SetBrush(brush);
    }

    ~wxDCBrushChanger()
    {
        if ( m_brushOld.Ok() )
            m_dc.SetBrush(m_brushOld);
    }

private:
    wxDC& m_dc;

    wxBrush m_brushOld;

    DECLARE_NO_COPY_CLASS(wxDCBrushChanger)
};

// ----------------------------------------------------------------------------
// another small helper class: sets the clipping region in its ctor and
// destroys it in the dtor
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxDCClipper
{
public:
    wxDCClipper(wxDC& dc, const wxRegion& r) : m_dc(dc)
        { dc.SetClippingRegion(r); }
    wxDCClipper(wxDC& dc, const wxRect& r) : m_dc(dc)
        { dc.SetClippingRegion(r.x, r.y, r.width, r.height); }
    wxDCClipper(wxDC& dc, wxCoord x, wxCoord y, wxCoord w, wxCoord h) : m_dc(dc)
        { dc.SetClippingRegion(x, y, w, h); }

    ~wxDCClipper() { m_dc.DestroyClippingRegion(); }

private:
    wxDC& m_dc;

    DECLARE_NO_COPY_CLASS(wxDCClipper)
};

#endif // _WX_DC_H_BASE_
