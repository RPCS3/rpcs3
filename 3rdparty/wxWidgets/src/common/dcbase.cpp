/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/dcbase.cpp
// Purpose:     generic methods of the wxDC Class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     05/25/99
// RCS-ID:      $Id: dcbase.cpp 56135 2008-10-06 21:04:02Z VZ $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/dc.h"
#include "wx/dcbuffer.h" // for IMPLEMENT_DYNAMIC_CLASS

#ifndef WX_PRECOMP
    #include "wx/math.h"
#endif

// bool wxDCBase::sm_cacheing = false;

IMPLEMENT_ABSTRACT_CLASS(wxDCBase, wxObject)

// ============================================================================
// implementation
// ============================================================================

IMPLEMENT_DYNAMIC_CLASS(wxBufferedDC, wxMemoryDC)
IMPLEMENT_ABSTRACT_CLASS(wxBufferedPaintDC, wxBufferedDC)

#if WXWIN_COMPATIBILITY_2_6
void wxDCBase::BeginDrawing()
{
}

void wxDCBase::EndDrawing()
{
}
#endif // WXWIN_COMPATIBILITY_2_6

// ----------------------------------------------------------------------------
// special symbols
// ----------------------------------------------------------------------------

void wxDCBase::DoDrawCheckMark(wxCoord x1, wxCoord y1,
                               wxCoord width, wxCoord height)
{
    wxCHECK_RET( Ok(), wxT("invalid window dc") );

    wxCoord x2 = x1 + width,
            y2 = y1 + height;

    // the pen width is calibrated to give 3 for width == height == 10
    wxDCPenChanger pen((wxDC&)*this,
                        wxPen(GetTextForeground(), (width + height + 1)/7));

    // we're drawing a scaled version of wx/generic/tick.xpm here
    wxCoord x3 = x1 + (4*width) / 10,   // x of the tick bottom
            y3 = y1 + height / 2;       // y of the left tick branch
    DoDrawLine(x1, y3, x3, y2);
    DoDrawLine(x3, y2, x2, y1);

    CalcBoundingBox(x1, y1);
    CalcBoundingBox(x2, y2);
}

// ----------------------------------------------------------------------------
// line/polygons
// ----------------------------------------------------------------------------

void wxDCBase::DrawLines(const wxList *list, wxCoord xoffset, wxCoord yoffset)
{
    int n = list->GetCount();
    wxPoint *points = new wxPoint[n];

    int i = 0;
    for ( wxList::compatibility_iterator node = list->GetFirst(); node; node = node->GetNext(), i++ )
    {
        wxPoint *point = (wxPoint *)node->GetData();
        points[i].x = point->x;
        points[i].y = point->y;
    }

    DoDrawLines(n, points, xoffset, yoffset);

    delete [] points;
}


void wxDCBase::DrawPolygon(const wxList *list,
                           wxCoord xoffset, wxCoord yoffset,
                           int fillStyle)
{
    int n = list->GetCount();
    wxPoint *points = new wxPoint[n];

    int i = 0;
    for ( wxList::compatibility_iterator node = list->GetFirst(); node; node = node->GetNext(), i++ )
    {
        wxPoint *point = (wxPoint *)node->GetData();
        points[i].x = point->x;
        points[i].y = point->y;
    }

    DoDrawPolygon(n, points, xoffset, yoffset, fillStyle);

    delete [] points;
}

void
wxDCBase::DoDrawPolyPolygon(int n,
                            int count[],
                            wxPoint points[],
                            wxCoord xoffset, wxCoord yoffset,
                            int fillStyle)
{
    if ( n == 1 )
    {
        DoDrawPolygon(count[0], points, xoffset, yoffset, fillStyle);
        return;
    }

    int      i, j, lastOfs;
    wxPoint* pts;
    wxPen    pen;

    for (i = j = lastOfs = 0; i < n; i++)
    {
        lastOfs = j;
        j      += count[i];
    }
    pts = new wxPoint[j+n-1];
    for (i = 0; i < j; i++)
        pts[i] = points[i];
    for (i = 2; i <= n; i++)
    {
        lastOfs -= count[n-i];
        pts[j++] = pts[lastOfs];
    }

    pen = GetPen();
    SetPen(wxPen(*wxBLACK, 0, wxTRANSPARENT));
    DoDrawPolygon(j, pts, xoffset, yoffset, fillStyle);
    SetPen(pen);
    for (i = j = 0; i < n; i++)
    {
        DoDrawLines(count[i], pts+j, xoffset, yoffset);
        j += count[i];
    }
    delete[] pts;
}

// ----------------------------------------------------------------------------
// splines
// ----------------------------------------------------------------------------

#if wxUSE_SPLINES

// TODO: this API needs fixing (wxPointList, why (!const) "wxList *"?)
void wxDCBase::DrawSpline(wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2, wxCoord x3, wxCoord y3)
{
    wxList point_list;

    wxPoint *point1 = new wxPoint;
    point1->x = x1; point1->y = y1;
    point_list.Append((wxObject*)point1);

    wxPoint *point2 = new wxPoint;
    point2->x = x2; point2->y = y2;
    point_list.Append((wxObject*)point2);

    wxPoint *point3 = new wxPoint;
    point3->x = x3; point3->y = y3;
    point_list.Append((wxObject*)point3);

    DrawSpline(&point_list);

    for( wxList::compatibility_iterator node = point_list.GetFirst(); node; node = node->GetNext() )
    {
        wxPoint *p = (wxPoint *)node->GetData();
        delete p;
    }
}

void wxDCBase::DrawSpline(int n, wxPoint points[])
{
    wxList list;
    for (int i =0; i < n; i++)
    {
        list.Append((wxObject*)&points[i]);
    }

    DrawSpline(&list);
}

// ----------------------------------- spline code ----------------------------------------

void wx_quadratic_spline(double a1, double b1, double a2, double b2,
                         double a3, double b3, double a4, double b4);
void wx_clear_stack();
int wx_spline_pop(double *x1, double *y1, double *x2, double *y2, double *x3,
        double *y3, double *x4, double *y4);
void wx_spline_push(double x1, double y1, double x2, double y2, double x3, double y3,
          double x4, double y4);
static bool wx_spline_add_point(double x, double y);
static void wx_spline_draw_point_array(wxDCBase *dc);

wxList wx_spline_point_list;

#define                half(z1, z2)        ((z1+z2)/2.0)
#define                THRESHOLD        5

/* iterative version */

void wx_quadratic_spline(double a1, double b1, double a2, double b2, double a3, double b3, double a4,
                 double b4)
{
    register double  xmid, ymid;
    double           x1, y1, x2, y2, x3, y3, x4, y4;

    wx_clear_stack();
    wx_spline_push(a1, b1, a2, b2, a3, b3, a4, b4);

    while (wx_spline_pop(&x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4)) {
        xmid = (double)half(x2, x3);
        ymid = (double)half(y2, y3);
        if (fabs(x1 - xmid) < THRESHOLD && fabs(y1 - ymid) < THRESHOLD &&
            fabs(xmid - x4) < THRESHOLD && fabs(ymid - y4) < THRESHOLD) {
            wx_spline_add_point( x1, y1 );
            wx_spline_add_point( xmid, ymid );
        } else {
            wx_spline_push(xmid, ymid, (double)half(xmid, x3), (double)half(ymid, y3),
                 (double)half(x3, x4), (double)half(y3, y4), x4, y4);
            wx_spline_push(x1, y1, (double)half(x1, x2), (double)half(y1, y2),
                 (double)half(x2, xmid), (double)half(y2, ymid), xmid, ymid);
        }
    }
}

/* utilities used by spline drawing routines */

typedef struct wx_spline_stack_struct {
    double           x1, y1, x2, y2, x3, y3, x4, y4;
} Stack;

#define         SPLINE_STACK_DEPTH             20
static Stack    wx_spline_stack[SPLINE_STACK_DEPTH];
static Stack   *wx_stack_top;
static int      wx_stack_count;

void wx_clear_stack()
{
    wx_stack_top = wx_spline_stack;
    wx_stack_count = 0;
}

void wx_spline_push(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4)
{
    wx_stack_top->x1 = x1;
    wx_stack_top->y1 = y1;
    wx_stack_top->x2 = x2;
    wx_stack_top->y2 = y2;
    wx_stack_top->x3 = x3;
    wx_stack_top->y3 = y3;
    wx_stack_top->x4 = x4;
    wx_stack_top->y4 = y4;
    wx_stack_top++;
    wx_stack_count++;
}

int wx_spline_pop(double *x1, double *y1, double *x2, double *y2,
                  double *x3, double *y3, double *x4, double *y4)
{
    if (wx_stack_count == 0)
        return (0);
    wx_stack_top--;
    wx_stack_count--;
    *x1 = wx_stack_top->x1;
    *y1 = wx_stack_top->y1;
    *x2 = wx_stack_top->x2;
    *y2 = wx_stack_top->y2;
    *x3 = wx_stack_top->x3;
    *y3 = wx_stack_top->y3;
    *x4 = wx_stack_top->x4;
    *y4 = wx_stack_top->y4;
    return (1);
}

static bool wx_spline_add_point(double x, double y)
{
  wxPoint *point = new wxPoint ;
  point->x = (int) x;
  point->y = (int) y;
  wx_spline_point_list.Append((wxObject*)point);
  return true;
}

static void wx_spline_draw_point_array(wxDCBase *dc)
{
  dc->DrawLines(&wx_spline_point_list, 0, 0 );
  wxList::compatibility_iterator node = wx_spline_point_list.GetFirst();
  while (node)
  {
    wxPoint *point = (wxPoint *)node->GetData();
    delete point;
    wx_spline_point_list.Erase(node);
    node = wx_spline_point_list.GetFirst();
  }
}

void wxDCBase::DoDrawSpline( wxList *points )
{
    wxCHECK_RET( Ok(), wxT("invalid window dc") );

    wxPoint *p;
    double           cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4;
    double           x1, y1, x2, y2;

    wxList::compatibility_iterator node = points->GetFirst();
    if (!node)
        // empty list
        return;

    p = (wxPoint *)node->GetData();

    x1 = p->x;
    y1 = p->y;

    node = node->GetNext();
    p = (wxPoint *)node->GetData();

    x2 = p->x;
    y2 = p->y;
    cx1 = (double)((x1 + x2) / 2);
    cy1 = (double)((y1 + y2) / 2);
    cx2 = (double)((cx1 + x2) / 2);
    cy2 = (double)((cy1 + y2) / 2);

    wx_spline_add_point(x1, y1);

    while ((node = node->GetNext())
#if !wxUSE_STL
           != NULL
#endif // !wxUSE_STL
          )
    {
        p = (wxPoint *)node->GetData();
        x1 = x2;
        y1 = y2;
        x2 = p->x;
        y2 = p->y;
        cx4 = (double)(x1 + x2) / 2;
        cy4 = (double)(y1 + y2) / 2;
        cx3 = (double)(x1 + cx4) / 2;
        cy3 = (double)(y1 + cy4) / 2;

        wx_quadratic_spline(cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4);

        cx1 = cx4;
        cy1 = cy4;
        cx2 = (double)(cx1 + x2) / 2;
        cy2 = (double)(cy1 + y2) / 2;
    }

    wx_spline_add_point( cx1, cy1 );
    wx_spline_add_point( x2, y2 );

    wx_spline_draw_point_array( this );
}

#endif // wxUSE_SPLINES

// ----------------------------------------------------------------------------
// Partial Text Extents
// ----------------------------------------------------------------------------


// Each element of the widths array will be the width of the string up to and
// including the corresponding character in text.  This is the generic
// implementation, the port-specific classes should do this with native APIs
// if available and if faster.  Note: pango_layout_index_to_pos is much slower
// than calling GetTextExtent!!

#define FWC_SIZE 256

class FontWidthCache
{
public:
    FontWidthCache() : m_scaleX(1), m_widths(NULL) { }
    ~FontWidthCache() { delete []m_widths; }

    void Reset()
    {
        if (!m_widths)
            m_widths = new int[FWC_SIZE];

        memset(m_widths, 0, sizeof(int)*FWC_SIZE);
    }

    wxFont m_font;
    double m_scaleX;
    int *m_widths;
};

static FontWidthCache s_fontWidthCache;

bool wxDCBase::DoGetPartialTextExtents(const wxString& text, wxArrayInt& widths) const
{
    int totalWidth = 0;

    const size_t len = text.length();
    widths.Empty();
    widths.Add(0, len);

    // reset the cache if font or horizontal scale have changed
    if ( !s_fontWidthCache.m_widths ||
         !wxIsSameDouble(s_fontWidthCache.m_scaleX, m_scaleX) ||
         (s_fontWidthCache.m_font != GetFont()) )
    {
        s_fontWidthCache.Reset();
        s_fontWidthCache.m_font = GetFont();
        s_fontWidthCache.m_scaleX = m_scaleX;
    }

    // Calculate the position of each character based on the widths of
    // the previous characters
    int w, h;
    for ( size_t i = 0; i < len; i++ )
    {
        const wxChar c = text[i];
        unsigned int c_int = (unsigned int)c;

        if ((c_int < FWC_SIZE) && (s_fontWidthCache.m_widths[c_int] != 0))
        {
            w = s_fontWidthCache.m_widths[c_int];
        }
        else
        {
            GetTextExtent(c, &w, &h);
            if (c_int < FWC_SIZE)
                s_fontWidthCache.m_widths[c_int] = w;
        }

        totalWidth += w;
        widths[i] = totalWidth;
    }

    return true;
}


// ----------------------------------------------------------------------------
// enhanced text drawing
// ----------------------------------------------------------------------------

void wxDCBase::GetMultiLineTextExtent(const wxString& text,
                                      wxCoord *x,
                                      wxCoord *y,
                                      wxCoord *h,
                                      wxFont *font) const
{
    wxCoord widthTextMax = 0, widthLine,
            heightTextTotal = 0, heightLineDefault = 0, heightLine = 0;

    wxString curLine;
    for ( const wxChar *pc = text; ; pc++ )
    {
        if ( *pc == _T('\n') || *pc == _T('\0') )
        {
            if ( curLine.empty() )
            {
                // we can't use GetTextExtent - it will return 0 for both width
                // and height and an empty line should count in height
                // calculation

                // assume that this line has the same height as the previous
                // one
                if ( !heightLineDefault )
                    heightLineDefault = heightLine;

                if ( !heightLineDefault )
                {
                    // but we don't know it yet - choose something reasonable
                    GetTextExtent(_T("W"), NULL, &heightLineDefault,
                                  NULL, NULL, font);
                }

                heightTextTotal += heightLineDefault;
            }
            else
            {
                GetTextExtent(curLine, &widthLine, &heightLine,
                              NULL, NULL, font);
                if ( widthLine > widthTextMax )
                    widthTextMax = widthLine;
                heightTextTotal += heightLine;
            }

            if ( *pc == _T('\n') )
            {
               curLine.clear();
            }
            else
            {
               // the end of string
               break;
            }
        }
        else
        {
            curLine += *pc;
        }
    }

    if ( x )
        *x = widthTextMax;
    if ( y )
        *y = heightTextTotal;
    if ( h )
        *h = heightLine;
}

void wxDCBase::DrawLabel(const wxString& text,
                         const wxBitmap& bitmap,
                         const wxRect& rect,
                         int alignment,
                         int indexAccel,
                         wxRect *rectBounding)
{
    // find the text position
    wxCoord widthText, heightText, heightLine;
    GetMultiLineTextExtent(text, &widthText, &heightText, &heightLine);

    wxCoord width, height;
    if ( bitmap.Ok() )
    {
        width = widthText + bitmap.GetWidth();
        height = bitmap.GetHeight();
    }
    else // no bitmap
    {
        width = widthText;
        height = heightText;
    }

    wxCoord x, y;
    if ( alignment & wxALIGN_RIGHT )
    {
        x = rect.GetRight() - width;
    }
    else if ( alignment & wxALIGN_CENTRE_HORIZONTAL )
    {
        x = (rect.GetLeft() + rect.GetRight() + 1 - width) / 2;
    }
    else // alignment & wxALIGN_LEFT
    {
        x = rect.GetLeft();
    }

    if ( alignment & wxALIGN_BOTTOM )
    {
        y = rect.GetBottom() - height;
    }
    else if ( alignment & wxALIGN_CENTRE_VERTICAL )
    {
        y = (rect.GetTop() + rect.GetBottom() + 1 - height) / 2;
    }
    else // alignment & wxALIGN_TOP
    {
        y = rect.GetTop();
    }

    // draw the bitmap first
    wxCoord x0 = x,
            y0 = y,
            width0 = width;
    if ( bitmap.Ok() )
    {
        DrawBitmap(bitmap, x, y, true /* use mask */);

        wxCoord offset = bitmap.GetWidth() + 4;
        x += offset;
        width -= offset;

        y += (height - heightText) / 2;
    }

    // we will draw the underscore under the accel char later
    wxCoord startUnderscore = 0,
            endUnderscore = 0,
            yUnderscore = 0;

    // split the string into lines and draw each of them separately
    wxString curLine;
    for ( const wxChar *pc = text; ; pc++ )
    {
        if ( *pc == _T('\n') || *pc == _T('\0') )
        {
            int xRealStart = x; // init it here to avoid compielr warnings

            if ( !curLine.empty() )
            {
                // NB: can't test for !(alignment & wxALIGN_LEFT) because
                //     wxALIGN_LEFT is 0
                if ( alignment & (wxALIGN_RIGHT | wxALIGN_CENTRE_HORIZONTAL) )
                {
                    wxCoord widthLine;
                    GetTextExtent(curLine, &widthLine, NULL);

                    if ( alignment & wxALIGN_RIGHT )
                    {
                        xRealStart += width - widthLine;
                    }
                    else // if ( alignment & wxALIGN_CENTRE_HORIZONTAL )
                    {
                        xRealStart += (width - widthLine) / 2;
                    }
                }
                //else: left aligned, nothing to do

                DrawText(curLine, xRealStart, y);
            }

            y += heightLine;

            // do we have underscore in this line? we can check yUnderscore
            // because it is set below to just y + heightLine if we do
            if ( y == yUnderscore )
            {
                // adjust the horz positions to account for the shift
                startUnderscore += xRealStart;
                endUnderscore += xRealStart;
            }

            if ( *pc == _T('\0') )
                break;

            curLine.clear();
        }
        else // not end of line
        {
            if ( pc - text.c_str() == indexAccel )
            {
                // remeber to draw underscore here
                GetTextExtent(curLine, &startUnderscore, NULL);
                curLine += *pc;
                GetTextExtent(curLine, &endUnderscore, NULL);

                yUnderscore = y + heightLine;
            }
            else
            {
                curLine += *pc;
            }
        }
    }

    // draw the underscore if found
    if ( startUnderscore != endUnderscore )
    {
        // it should be of the same colour as text
        SetPen(wxPen(GetTextForeground(), 0, wxSOLID));

        yUnderscore--;

        DrawLine(startUnderscore, yUnderscore, endUnderscore, yUnderscore);
    }

    // return bounding rect if requested
    if ( rectBounding )
    {
        *rectBounding = wxRect(x, y - heightText, widthText, heightText);
    }

    CalcBoundingBox(x0, y0);
    CalcBoundingBox(x0 + width0, y0 + height);
}


void wxDCBase::DoGradientFillLinear(const wxRect& rect,
                                    const wxColour& initialColour,
                                    const wxColour& destColour,
                                    wxDirection nDirection)
{
    // save old pen
    wxPen oldPen = m_pen;
    wxBrush oldBrush = m_brush;

    wxUint8 nR1 = initialColour.Red();
    wxUint8 nG1 = initialColour.Green();
    wxUint8 nB1 = initialColour.Blue();
    wxUint8 nR2 = destColour.Red();
    wxUint8 nG2 = destColour.Green();
    wxUint8 nB2 = destColour.Blue();
    wxUint8 nR, nG, nB;

    if ( nDirection == wxEAST || nDirection == wxWEST )
    {
        wxInt32 x = rect.GetWidth();
        wxInt32 w = x;              // width of area to shade
        wxInt32 xDelta = w/256;     // height of one shade bend
        if (xDelta < 1)
            xDelta = 1;

        while (x >= xDelta)
        {
            x -= xDelta;
            if (nR1 > nR2)
                nR = nR1 - (nR1-nR2)*(w-x)/w;
            else
                nR = nR1 + (nR2-nR1)*(w-x)/w;

            if (nG1 > nG2)
                nG = nG1 - (nG1-nG2)*(w-x)/w;
            else
                nG = nG1 + (nG2-nG1)*(w-x)/w;

            if (nB1 > nB2)
                nB = nB1 - (nB1-nB2)*(w-x)/w;
            else
                nB = nB1 + (nB2-nB1)*(w-x)/w;

            wxColour colour(nR,nG,nB);
            SetPen(wxPen(colour, 1, wxSOLID));
            SetBrush(wxBrush(colour));
            if(nDirection == wxEAST)
                DrawRectangle(rect.GetRight()-x-xDelta+1, rect.GetTop(),
                        xDelta, rect.GetHeight());
            else //nDirection == wxWEST
                DrawRectangle(rect.GetLeft()+x, rect.GetTop(),
                        xDelta, rect.GetHeight());
        }
    }
    else  // nDirection == wxNORTH || nDirection == wxSOUTH
    {
        wxInt32 y = rect.GetHeight();
        wxInt32 w = y;              // height of area to shade
        wxInt32 yDelta = w/255;     // height of one shade bend
        if (yDelta < 1)
            yDelta = 1;

        while (y > 0)
        {
            y -= yDelta;
            if (nR1 > nR2)
                nR = nR1 - (nR1-nR2)*(w-y)/w;
            else
                nR = nR1 + (nR2-nR1)*(w-y)/w;

            if (nG1 > nG2)
                nG = nG1 - (nG1-nG2)*(w-y)/w;
            else
                nG = nG1 + (nG2-nG1)*(w-y)/w;

            if (nB1 > nB2)
                nB = nB1 - (nB1-nB2)*(w-y)/w;
            else
                nB = nB1 + (nB2-nB1)*(w-y)/w;

            wxColour colour(nR,nG,nB);
            SetPen(wxPen(colour, 1, wxSOLID));
            SetBrush(wxBrush(colour));
            if(nDirection == wxNORTH)
                DrawRectangle(rect.GetLeft(), rect.GetTop()+y,
                        rect.GetWidth(), yDelta);
            else //nDirection == wxSOUTH
                DrawRectangle(rect.GetLeft(), rect.GetBottom()-y-yDelta+1,
                        rect.GetWidth(), yDelta);
        }
    }

    SetPen(oldPen);
    SetBrush(oldBrush);
}

void wxDCBase::DoGradientFillConcentric(const wxRect& rect,
                                      const wxColour& initialColour,
                                      const wxColour& destColour,
                                      const wxPoint& circleCenter)
{
    //save the old pen color
    wxColour oldPenColour = m_pen.GetColour();

    wxUint8 nR1 = destColour.Red();
    wxUint8 nG1 = destColour.Green();
    wxUint8 nB1 = destColour.Blue();
    wxUint8 nR2 = initialColour.Red();
    wxUint8 nG2 = initialColour.Green();
    wxUint8 nB2 = initialColour.Blue();
    wxUint8 nR, nG, nB;


    //Radius
    wxInt32 cx = rect.GetWidth() / 2;
    wxInt32 cy = rect.GetHeight() / 2;
    wxInt32 nRadius;
    if (cx < cy)
        nRadius = cx;
    else
        nRadius = cy;

    //Offset of circle
    wxInt32 nCircleOffX = circleCenter.x - (rect.GetWidth() / 2);
    wxInt32 nCircleOffY = circleCenter.y - (rect.GetHeight() / 2);

    for ( wxInt32 x = 0; x < rect.GetWidth(); x++ )
    {
        for ( wxInt32 y = 0; y < rect.GetHeight(); y++ )
        {
            //get color difference
            wxInt32 nGradient = ((nRadius -
                                  (wxInt32)sqrt(
                                    pow((double)(x - cx - nCircleOffX), 2) +
                                    pow((double)(y - cy - nCircleOffY), 2)
                                  )) * 100) / nRadius;

            //normalize Gradient
            if (nGradient < 0 )
                nGradient = 0;

            //get dest colors
            nR = (wxUint8)(nR1 + ((nR2 - nR1) * nGradient / 100));
            nG = (wxUint8)(nG1 + ((nG2 - nG1) * nGradient / 100));
            nB = (wxUint8)(nB1 + ((nB2 - nB1) * nGradient / 100));

            //set the pixel
            m_pen.SetColour(wxColour(nR,nG,nB));
            DrawPoint(wxPoint(x + rect.GetLeft(), y + rect.GetTop()));
        }
    }
    //return old pen color
    m_pen.SetColour(oldPenColour);
}

/*
Notes for wxWidgets DrawEllipticArcRot(...)

wxDCBase::DrawEllipticArcRot(...) draws a rotated elliptic arc or an ellipse.
It uses wxDCBase::CalculateEllipticPoints(...) and wxDCBase::Rotate(...),
which are also new.

All methods are generic, so they can be implemented in wxDCBase.
DoDrawEllipticArcRot(...) is virtual, so it can be called from deeper
methods like (WinCE) wxDC::DoDrawArc(...).

CalculateEllipticPoints(...) fills a given list of wxPoints with some points
of an elliptic arc. The algorithm is pixel-based: In every row (in flat
parts) or every column (in steep parts) only one pixel is calculated.
Trigonometric calculation (sin, cos, tan, atan) is only done if the
starting angle is not equal to the ending angle. The calculation of the
pixels is done using simple arithmetic only and should perform not too
bad even on devices without floating point processor. I didn't test this yet.

Rotate(...) rotates a list of point pixel-based, you will see rounding errors.
For instance: an ellipse rotated 180 degrees is drawn
slightly different from the original.

The points are then moved to an array and used to draw a polyline and/or polygon
(with center added, the pie).
The result looks quite similar to the native ellipse, only e few pixels differ.

The performance on a desktop system (Athlon 1800, WinXP) is about 7 times
slower as DrawEllipse(...), which calls the native API.
An rotated ellipse outside the clipping region takes nearly the same time,
while an native ellipse outside takes nearly no time to draw.

If you draw an arc with this new method, you will see the starting and ending angles
are calculated properly.
If you use DrawEllipticArc(...), you will see they are only correct for circles
and not properly calculated for ellipses.

Peter Lenhard
p.lenhard@t-online.de
*/

#ifdef __WXWINCE__
void wxDCBase::DoDrawEllipticArcRot( wxCoord x, wxCoord y,
                                     wxCoord w, wxCoord h,
                                     double sa, double ea, double angle )
{
    wxList list;

    CalculateEllipticPoints( &list, x, y, w, h, sa, ea );
    Rotate( &list, angle, wxPoint( x+w/2, y+h/2 ) );

    // Add center (for polygon/pie)
    list.Append( (wxObject*) new wxPoint( x+w/2, y+h/2 ) );

    // copy list into array and delete list elements
    int n = list.GetCount();
    wxPoint *points = new wxPoint[n];
    int i = 0;
    wxNode* node = 0;
    for ( node = list.GetFirst(); node; node = node->GetNext(), i++ )
    {
        wxPoint *point = (wxPoint *)node->GetData();
        points[i].x = point->x;
        points[i].y = point->y;
        delete point;
    }

    // first draw the pie without pen, if necessary
    if( GetBrush() != *wxTRANSPARENT_BRUSH )
    {
        wxPen tempPen( GetPen() );
        SetPen( *wxTRANSPARENT_PEN );
        DoDrawPolygon( n, points, 0, 0 );
        SetPen( tempPen );
    }

    // then draw the arc without brush, if necessary
    if( GetPen() != *wxTRANSPARENT_PEN )
    {
        // without center
        DoDrawLines( n-1, points, 0, 0 );
    }

    delete [] points;

} // DrawEllipticArcRot

void wxDCBase::Rotate( wxList* points, double angle, wxPoint center )
{
    if( angle != 0.0 )
    {
        double pi(M_PI);
        double dSinA = -sin(angle*2.0*pi/360.0);
        double dCosA = cos(angle*2.0*pi/360.0);
        for ( wxNode* node = points->GetFirst(); node; node = node->GetNext() )
        {
            wxPoint* point = (wxPoint*)node->GetData();

            // transform coordinates, if necessary
            if( center.x ) point->x -= center.x;
            if( center.y ) point->y -= center.y;

            // calculate rotation, rounding simply by implicit cast to integer
            int xTemp = point->x * dCosA - point->y * dSinA;
            point->y = point->x * dSinA + point->y * dCosA;
            point->x = xTemp;

            // back transform coordinates, if necessary
            if( center.x ) point->x += center.x;
            if( center.y ) point->y += center.y;
        }
    }
}

void wxDCBase::CalculateEllipticPoints( wxList* points,
                                        wxCoord xStart, wxCoord yStart,
                                        wxCoord w, wxCoord h,
                                        double sa, double ea )
{
    double pi = M_PI;
    double sar = 0;
    double ear = 0;
    int xsa = 0;
    int ysa = 0;
    int xea = 0;
    int yea = 0;
    int sq = 0;
    int eq = 0;
    bool bUseAngles = false;
    if( w<0 ) w = -w;
    if( h<0 ) h = -h;
    // half-axes
    wxCoord a = w/2;
    wxCoord b = h/2;
    // decrement 1 pixel if ellipse is smaller than 2*a, 2*b
    int decrX = 0;
    if( 2*a == w ) decrX = 1;
    int decrY = 0;
    if( 2*b == h ) decrY = 1;
    // center
    wxCoord xCenter = xStart + a;
    wxCoord yCenter = yStart + b;
    // calculate data for start and end, if necessary
    if( sa != ea )
    {
        bUseAngles = true;
        // normalisation of angles
        while( sa<0 ) sa += 360;
        while( ea<0 ) ea += 360;
        while( sa>=360 ) sa -= 360;
        while( ea>=360 ) ea -= 360;
        // calculate quadrant numbers
        if( sa > 270 ) sq = 3;
        else if( sa > 180 ) sq = 2;
        else if( sa > 90 ) sq = 1;
        if( ea > 270 ) eq = 3;
        else if( ea > 180 ) eq = 2;
        else if( ea > 90 ) eq = 1;
        sar = sa * pi / 180.0;
        ear = ea * pi / 180.0;
        // correct angle circle -> ellipse
        sar = atan( -a/(double)b * tan( sar ) );
        if ( sq == 1 || sq == 2 ) sar += pi;
        ear = atan( -a/(double)b * tan( ear ) );
        if ( eq == 1 || eq == 2 ) ear += pi;
        // coordinates of points
        xsa = xCenter + a * cos( sar );
        if( sq == 0 || sq == 3 ) xsa -= decrX;
        ysa = yCenter + b * sin( sar );
        if( sq == 2 || sq == 3 ) ysa -= decrY;
        xea = xCenter + a * cos( ear );
        if( eq == 0 || eq == 3 ) xea -= decrX;
        yea = yCenter + b * sin( ear );
        if( eq == 2 || eq == 3 ) yea -= decrY;
    } // if iUseAngles
    // calculate c1 = b^2, c2 = b^2/a^2 with a = w/2, b = h/2
    double c1 = b * b;
    double c2 = 2.0 / w;
    c2 *= c2;
    c2 *= c1;
    wxCoord x = 0;
    wxCoord y = b;
    long x2 = 1;
    long y2 = y*y;
    long y2_old = 0;
    long y_old = 0;
    // Lists for quadrant 1 to 4
    wxList pointsarray[4];
    // Calculate points for first quadrant and set in all quadrants
    for( x = 0; x <= a; ++x )
    {
        x2 = x2+x+x-1;
        y2_old = y2;
        y_old = y;
        bool bNewPoint = false;
        while( y2 > c1 - c2 * x2 && y > 0 )
        {
            bNewPoint = true;
            y2 = y2-y-y+1;
            --y;
        }
        // old y now to big: set point with old y, old x
        if( bNewPoint && x>1)
        {
            int x1 = x - 1;
            // remove points on the same line
            pointsarray[0].Insert( (wxObject*) new wxPoint( xCenter + x1 - decrX, yCenter - y_old ) );
            pointsarray[1].Append( (wxObject*) new wxPoint( xCenter - x1, yCenter - y_old ) );
            pointsarray[2].Insert( (wxObject*) new wxPoint( xCenter - x1, yCenter + y_old - decrY ) );
            pointsarray[3].Append( (wxObject*) new wxPoint( xCenter + x1 - decrX, yCenter + y_old - decrY ) );
        } // set point
    } // calculate point

    // Starting and/or ending points for the quadrants, first quadrant gets both.
    pointsarray[0].Insert( (wxObject*) new wxPoint( xCenter + a - decrX, yCenter ) );
    pointsarray[0].Append( (wxObject*) new wxPoint( xCenter, yCenter - b ) );
    pointsarray[1].Append( (wxObject*) new wxPoint( xCenter - a, yCenter ) );
    pointsarray[2].Append( (wxObject*) new wxPoint( xCenter, yCenter + b - decrY ) );
    pointsarray[3].Append( (wxObject*) new wxPoint( xCenter + a - decrX, yCenter ) );

    // copy quadrants in original list
    if( bUseAngles )
    {
        // Copy the right part of the points in the lists
        // and delete the wxPoints, because they do not leave this method.
        points->Append( (wxObject*) new wxPoint( xsa, ysa ) );
        int q = sq;
        bool bStarted = false;
        bool bReady = false;
        bool bForceTurn = ( sq == eq && sa > ea );
        while( !bReady )
        {
            for( wxNode *node = pointsarray[q].GetFirst(); node; node = node->GetNext() )
            {
                // once: go to starting point in start quadrant
                if( !bStarted &&
                    (
                      ( (wxPoint*) node->GetData() )->x < xsa+1 && q <= 1
                      ||
                      ( (wxPoint*) node->GetData() )->x > xsa-1 && q >= 2
                    )
                  )
                {
                    bStarted = true;
                }

                // copy point, if not at ending point
                if( bStarted )
                {
                    if( q != eq || bForceTurn
                        ||
                        ( (wxPoint*) node->GetData() )->x > xea+1 && q <= 1
                        ||
                        ( (wxPoint*) node->GetData() )->x < xea-1 && q >= 2
                      )
                    {
                        // copy point
                        wxPoint* pPoint = new wxPoint( *((wxPoint*) node->GetData() ) );
                        points->Append( (wxObject*) pPoint );
                    }
                    else if( q == eq && !bForceTurn || ( (wxPoint*) node->GetData() )->x == xea)
                    {
                        bReady = true;
                    }
                }
            } // for node
            ++q;
            if( q > 3 ) q = 0;
            bForceTurn = false;
            bStarted = true;
        } // while not bReady
        points->Append( (wxObject*) new wxPoint( xea, yea ) );

        // delete points
        for( q = 0; q < 4; ++q )
        {
            for( wxNode *node = pointsarray[q].GetFirst(); node; node = node->GetNext() )
            {
                wxPoint *p = (wxPoint *)node->GetData();
                delete p;
            }
        }
    }
    else
    {
        wxNode* node;
        // copy whole ellipse, wxPoints will be deleted outside
        for( node = pointsarray[0].GetFirst(); node; node = node->GetNext() )
        {
            wxObject *p = node->GetData();
            points->Append( p );
        }
        for( node = pointsarray[1].GetFirst(); node; node = node->GetNext() )
        {
            wxObject *p = node->GetData();
            points->Append( p );
        }
        for( node = pointsarray[2].GetFirst(); node; node = node->GetNext() )
        {
            wxObject *p = node->GetData();
            points->Append( p );
        }
        for( node = pointsarray[3].GetFirst(); node; node = node->GetNext() )
        {
            wxObject *p = node->GetData();
            points->Append( p );
        }
    } // not iUseAngles
} // CalculateEllipticPoints

#endif


float wxDCBase::GetFontPointSizeAdjustment(float dpi)
{
    // wxMSW has long-standing bug where wxFont point size is interpreted as
    // "pixel size corresponding to given point size *on screen*". In other
    // words, on a typical 600dpi printer and a typical 96dpi screen, fonts
    // are ~6 times smaller when printing. Unfortunately, this bug is so severe
    // that *all* printing code has to account for it and consequently, other
    // ports need to emulate this bug too:
    const wxSize screenPixels = wxGetDisplaySize();
    const wxSize screenMM = wxGetDisplaySizeMM();
    const float screenPPI_y = (screenPixels.y * 25.4) / screenMM.y;
    return screenPPI_y / dpi;
}
