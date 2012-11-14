/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/graphcmn.cpp
// Purpose:     graphics context methods common to all platforms
// Author:      Stefan Csomor
// Modified by:
// Created:
// RCS-ID:      $Id: dcgraph.cpp 60190 2009-04-16 00:57:35Z KO $
// Copyright:   (c) Stefan Csomor
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if defined(__BORLANDC__)
    #pragma hdrstop
#endif

#if wxUSE_GRAPHICS_CONTEXT

#include "wx/graphics.h"

#ifndef WX_PRECOMP
    #include "wx/icon.h"
    #include "wx/bitmap.h"
    #include "wx/dcmemory.h"
    #include "wx/region.h"
#endif

#ifdef __WXMAC__
#include "wx/mac/private.h"
#endif
//-----------------------------------------------------------------------------
// constants
//-----------------------------------------------------------------------------

static const double RAD2DEG = 180.0 / M_PI;

//-----------------------------------------------------------------------------
// Local functions
//-----------------------------------------------------------------------------

static inline double DegToRad(double deg)
{
    return (deg * M_PI) / 180.0;
}

//-----------------------------------------------------------------------------
// wxDC bridge class
//-----------------------------------------------------------------------------

#ifdef __WXMAC__
IMPLEMENT_DYNAMIC_CLASS(wxGCDC, wxDCBase)
#else
IMPLEMENT_DYNAMIC_CLASS(wxGCDC, wxDC)
#endif

wxGCDC::wxGCDC()
{
    Init();
}

void wxGCDC::SetGraphicsContext( wxGraphicsContext* ctx )
{
    delete m_graphicContext;
    m_graphicContext = ctx;
    if ( m_graphicContext )
    {
        m_matrixOriginal = m_graphicContext->GetTransform();
        m_ok = true;
        // apply the stored transformations to the passed in context
        ComputeScaleAndOrigin();
        m_graphicContext->SetFont( m_font , m_textForegroundColour );
        m_graphicContext->SetPen( m_pen );
        m_graphicContext->SetBrush( m_brush);
    }
}

wxGCDC::wxGCDC(const wxWindowDC& dc)
{
    Init();
    wxGraphicsContext* context;
#if wxUSE_CAIRO
    wxGraphicsRenderer* renderer = wxGraphicsRenderer::GetCairoRenderer();
    context = renderer->CreateContext(dc);
#else
    context = wxGraphicsContext::Create(dc);
#endif

    SetGraphicsContext( context );
}

#ifdef __WXMSW__
wxGCDC::wxGCDC(const wxMemoryDC& dc)
{
    Init();
    SetGraphicsContext( wxGraphicsContext::Create(dc) );
}
#endif    

void wxGCDC::Init()
{
    m_ok = false;
    m_colour = true;
    m_mm_to_pix_x = mm2pt;
    m_mm_to_pix_y = mm2pt;

    m_pen = *wxBLACK_PEN;
    m_font = *wxNORMAL_FONT;
    m_brush = *wxWHITE_BRUSH;

    m_graphicContext = NULL;
    m_logicalFunctionSupported = true;
}


wxGCDC::~wxGCDC()
{
    delete m_graphicContext;
}

void wxGCDC::DoDrawBitmap( const wxBitmap &bmp, wxCoord x, wxCoord y, bool WXUNUSED(useMask) )
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoDrawBitmap - invalid DC") );
    wxCHECK_RET( bmp.Ok(), wxT("wxGCDC(cg)::DoDrawBitmap - invalid bitmap") );

    if ( bmp.GetDepth() == 1 )
    {
        m_graphicContext->SetPen(*wxTRANSPARENT_PEN);
        m_graphicContext->SetBrush( wxBrush( m_textBackgroundColour , wxSOLID ) );
        m_graphicContext->DrawRectangle( x , y , bmp.GetWidth() , bmp.GetHeight() );        
        m_graphicContext->SetBrush( wxBrush( m_textForegroundColour , wxSOLID ) );
        m_graphicContext->DrawBitmap( bmp, x , y , bmp.GetWidth() , bmp.GetHeight() );
        m_graphicContext->SetBrush( m_graphicContext->CreateBrush(m_brush));
        m_graphicContext->SetPen( m_graphicContext->CreatePen(m_pen));
    }
    else
        m_graphicContext->DrawBitmap( bmp, x , y , bmp.GetWidth() , bmp.GetHeight() );
}

void wxGCDC::DoDrawIcon( const wxIcon &icon, wxCoord x, wxCoord y )
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoDrawIcon - invalid DC") );
    wxCHECK_RET( icon.Ok(), wxT("wxGCDC(cg)::DoDrawIcon - invalid icon") );

    wxCoord w = icon.GetWidth();
    wxCoord h = icon.GetHeight();

    m_graphicContext->DrawIcon( icon , x, y, w, h );
}

bool wxGCDC::StartDoc( const wxString& WXUNUSED(message) ) 
{
    return true;
}

void wxGCDC::EndDoc() 
{
}

void wxGCDC::StartPage()
{
}

void wxGCDC::EndPage() 
{
}
    
void wxGCDC::Flush()
{
#ifdef __WXMAC__
    CGContextFlush( (CGContextRef) m_graphicContext->GetNativeContext() );
#endif
}

void wxGCDC::DoSetClippingRegion( wxCoord x, wxCoord y, wxCoord w, wxCoord h )
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoSetClippingRegion - invalid DC") );

    m_graphicContext->Clip( x, y, w, h );
    if ( m_clipping )
    {
        m_clipX1 = wxMax( m_clipX1, x );
        m_clipY1 = wxMax( m_clipY1, y );
        m_clipX2 = wxMin( m_clipX2, (x + w) );
        m_clipY2 = wxMin( m_clipY2, (y + h) );
    }
    else
    {
        m_clipping = true;

        m_clipX1 = x;
        m_clipY1 = y;
        m_clipX2 = x + w;
        m_clipY2 = y + h;
    }
}

void wxGCDC::DoSetClippingRegionAsRegion( const wxRegion &region )
{
    // region is in device coordinates
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoSetClippingRegionAsRegion - invalid DC") );

    if (region.Empty())
    {
        //DestroyClippingRegion();
        return;
    }

    wxRegion logRegion( region );
    wxCoord x, y, w, h;

    logRegion.Offset( DeviceToLogicalX(0), DeviceToLogicalY(0) );
    logRegion.GetBox( x, y, w, h );

    m_graphicContext->Clip( logRegion );
    if ( m_clipping )
    {
        m_clipX1 = wxMax( m_clipX1, x );
        m_clipY1 = wxMax( m_clipY1, y );
        m_clipX2 = wxMin( m_clipX2, (x + w) );
        m_clipY2 = wxMin( m_clipY2, (y + h) );
    }
    else
    {
        m_clipping = true;

        m_clipX1 = x;
        m_clipY1 = y;
        m_clipX2 = x + w;
        m_clipY2 = y + h;
    }
}

void wxGCDC::DestroyClippingRegion()
{
    m_graphicContext->ResetClip();
    // currently the clip eg of a window extends to the area between the scrollbars
    // so we must explicitely make sure it only covers the area we want it to draw
    int width, height ;
    GetSize( &width , &height ) ;
    m_graphicContext->Clip( DeviceToLogicalX(0) , DeviceToLogicalY(0) , DeviceToLogicalXRel(width), DeviceToLogicalYRel(height) );
    
    m_graphicContext->SetPen( m_pen );
    m_graphicContext->SetBrush( m_brush );

    m_clipping = false;
}

void wxGCDC::DoGetSizeMM( int* width, int* height ) const
{
    int w = 0, h = 0;

    GetSize( &w, &h );
    if (width)
        *width = long( double(w) / (m_scaleX * m_mm_to_pix_x) );
    if (height)
        *height = long( double(h) / (m_scaleY * m_mm_to_pix_y) );
}

void wxGCDC::SetTextForeground( const wxColour &col )
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::SetTextForeground - invalid DC") );

    if ( col != m_textForegroundColour )
    {
        m_textForegroundColour = col;
        m_graphicContext->SetFont( m_font, m_textForegroundColour );
    }
}

void wxGCDC::SetTextBackground( const wxColour &col )
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::SetTextBackground - invalid DC") );

    m_textBackgroundColour = col;
}

void wxGCDC::SetMapMode( int mode )
{
    switch (mode)
    {
    case wxMM_TWIPS:
        SetLogicalScale( twips2mm * m_mm_to_pix_x, twips2mm * m_mm_to_pix_y );
        break;

    case wxMM_POINTS:
        SetLogicalScale( pt2mm * m_mm_to_pix_x, pt2mm * m_mm_to_pix_y );
        break;

    case wxMM_METRIC:
        SetLogicalScale( m_mm_to_pix_x, m_mm_to_pix_y );
        break;

    case wxMM_LOMETRIC:
        SetLogicalScale( m_mm_to_pix_x / 10.0, m_mm_to_pix_y / 10.0 );
        break;

    case wxMM_TEXT:
    default:
        SetLogicalScale( 1.0, 1.0 );
        break;
    }

    ComputeScaleAndOrigin();
}

void wxGCDC::SetUserScale( double x, double y )
{
    // allow negative ? -> no

    m_userScaleX = x;
    m_userScaleY = y;
    ComputeScaleAndOrigin();
}

void wxGCDC::SetLogicalScale( double x, double y )
{
    // allow negative ?
    m_logicalScaleX = x;
    m_logicalScaleY = y;
    ComputeScaleAndOrigin();
}

void wxGCDC::SetLogicalOrigin( wxCoord x, wxCoord y )
{
    m_logicalOriginX = x * m_signX;   // is this still correct ?
    m_logicalOriginY = y * m_signY;
    ComputeScaleAndOrigin();
}

void wxGCDC::SetDeviceOrigin( wxCoord x, wxCoord y )
{
    m_deviceOriginX = x;
    m_deviceOriginY = y;
    ComputeScaleAndOrigin();
}

void wxGCDC::SetAxisOrientation( bool xLeftRight, bool yBottomUp )
{
    m_signX = (xLeftRight ?  1 : -1);
    m_signY = (yBottomUp ? -1 :  1);
    ComputeScaleAndOrigin();
}

wxSize wxGCDC::GetPPI() const
{
    return wxSize(72, 72);
}

int wxGCDC::GetDepth() const
{
    return 32;
}

void wxGCDC::ComputeScaleAndOrigin()
{
    m_scaleX = m_logicalScaleX * m_userScaleX;
    m_scaleY = m_logicalScaleY * m_userScaleY;

    if ( m_graphicContext )
    {
        m_matrixCurrent = m_graphicContext->CreateMatrix();
        m_matrixCurrent.Translate( m_deviceOriginX, m_deviceOriginY );
        m_matrixCurrent.Scale( m_scaleX, m_scaleY );
        // the logical origin sets the origin to have new coordinates
        m_matrixCurrent.Translate( -m_logicalOriginX, -m_logicalOriginY );

        m_graphicContext->SetTransform( m_matrixOriginal );
        m_graphicContext->ConcatTransform( m_matrixCurrent );
    }
}

void wxGCDC::SetPalette( const wxPalette& WXUNUSED(palette) )
{

}

void wxGCDC::SetBackgroundMode( int mode )
{
    m_backgroundMode = mode;
}

void wxGCDC::SetFont( const wxFont &font )
{
    m_font = font;
    if ( m_graphicContext )
    {
        wxFont f = font;
        if ( f.Ok() )
            f.SetPointSize( /*LogicalToDeviceYRel*/(font.GetPointSize()));
        m_graphicContext->SetFont( f, m_textForegroundColour );
    }
}

void wxGCDC::SetPen( const wxPen &pen )
{
    if ( m_pen == pen )
        return;

    m_pen = pen;
    if ( m_graphicContext )
    {
        m_graphicContext->SetPen( m_pen );
    }
}

void wxGCDC::SetBrush( const wxBrush &brush )
{
    if (m_brush == brush)
        return;

    m_brush = brush;
    if ( m_graphicContext )
    {
        m_graphicContext->SetBrush( m_brush );
    }
}
 
void wxGCDC::SetBackground( const wxBrush &brush )
{
    if (m_backgroundBrush == brush)
        return;

    m_backgroundBrush = brush;
    if (!m_backgroundBrush.Ok())
        return;
}

void wxGCDC::SetLogicalFunction( int function )
{
    if (m_logicalFunction == function)
        return;

    m_logicalFunction = function;
    if ( m_graphicContext->SetLogicalFunction( function ) )
        m_logicalFunctionSupported=true;
    else
        m_logicalFunctionSupported=false;
}

bool wxGCDC::DoFloodFill(wxCoord WXUNUSED(x), wxCoord WXUNUSED(y),
                         const wxColour& WXUNUSED(col), int WXUNUSED(style))
{
    return false;
}

bool wxGCDC::DoGetPixel( wxCoord WXUNUSED(x), wxCoord WXUNUSED(y), wxColour *WXUNUSED(col) ) const
{
    //  wxCHECK_MSG( 0 , false, wxT("wxGCDC(cg)::DoGetPixel - not implemented") );
    return false;
}

void wxGCDC::DoDrawLine( wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2 )
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoDrawLine - invalid DC") );

    if ( !m_logicalFunctionSupported )
        return;

    m_graphicContext->StrokeLine(x1,y1,x2,y2);

    CalcBoundingBox(x1, y1);
    CalcBoundingBox(x2, y2);
}

void wxGCDC::DoCrossHair( wxCoord x, wxCoord y )
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoCrossHair - invalid DC") );

    if ( !m_logicalFunctionSupported )
        return;

    int w = 0, h = 0;

    GetSize( &w, &h );

    m_graphicContext->StrokeLine(0,y,w,y);
    m_graphicContext->StrokeLine(x,0,x,h);

    CalcBoundingBox(0, 0);
    CalcBoundingBox(0+w, 0+h);
}

void wxGCDC::DoDrawArc( wxCoord x1, wxCoord y1,
                        wxCoord x2, wxCoord y2,
                        wxCoord xc, wxCoord yc )
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoDrawArc - invalid DC") );

    if ( !m_logicalFunctionSupported )
        return;

    double dx = x1 - xc;
    double dy = y1 - yc;
    double radius = sqrt((double)(dx * dx + dy * dy));
    wxCoord rad = (wxCoord)radius;
    double sa, ea;
    if (x1 == x2 && y1 == y2)
    {
        sa = 0.0;
        ea = 360.0;
    }
    else if (radius == 0.0)
    {
        sa = ea = 0.0;
    }
    else
    {
        sa = (x1 - xc == 0) ?
     (y1 - yc < 0) ? 90.0 : -90.0 :
             -atan2(double(y1 - yc), double(x1 - xc)) * RAD2DEG;
        ea = (x2 - xc == 0) ?
     (y2 - yc < 0) ? 90.0 : -90.0 :
             -atan2(double(y2 - yc), double(x2 - xc)) * RAD2DEG;
    }

    bool fill = m_brush.GetStyle() != wxTRANSPARENT;

    wxGraphicsPath path = m_graphicContext->CreatePath();
    if ( fill && ((x1!=x2)||(y1!=y2)) )
        path.MoveToPoint( xc, yc );
    // since these angles (ea,sa) are measured counter-clockwise, we invert them to
    // get clockwise angles
    path.AddArc( xc, yc , rad , DegToRad(-sa) , DegToRad(-ea), false );
    if ( fill && ((x1!=x2)||(y1!=y2)) )
        path.AddLineToPoint( xc, yc );
    m_graphicContext->DrawPath(path);
}

void wxGCDC::DoDrawEllipticArc( wxCoord x, wxCoord y, wxCoord w, wxCoord h,
                                double sa, double ea )
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoDrawEllipticArc - invalid DC") );

    if ( !m_logicalFunctionSupported )
        return;

    m_graphicContext->PushState();
    m_graphicContext->Translate(x+w/2.0,y+h/2.0);
    wxDouble factor = ((wxDouble) w) / h;
    m_graphicContext->Scale( factor , 1.0);

    // since these angles (ea,sa) are measured counter-clockwise, we invert them to
    // get clockwise angles
    if ( m_brush.GetStyle() != wxTRANSPARENT )
    {
        wxGraphicsPath path = m_graphicContext->CreatePath();
        path.MoveToPoint( 0, 0 );
        path.AddArc( 0, 0, h/2.0 , DegToRad(-sa) , DegToRad(-ea), sa > ea );
        path.AddLineToPoint( 0, 0 );
        m_graphicContext->FillPath( path );

        path = m_graphicContext->CreatePath();
        path.AddArc( 0, 0, h/2.0 , DegToRad(-sa) , DegToRad(-ea), sa > ea );
        m_graphicContext->StrokePath( path );
    }
    else
    {
        wxGraphicsPath path = m_graphicContext->CreatePath();
        path.AddArc( 0, 0, h/2.0 , DegToRad(-sa) , DegToRad(-ea), sa > ea );
        m_graphicContext->DrawPath( path );
    }

    m_graphicContext->PopState();
}

void wxGCDC::DoDrawPoint( wxCoord x, wxCoord y )
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoDrawPoint - invalid DC") );

    DoDrawLine( x , y , x + 1 , y + 1 );
}

void wxGCDC::DoDrawLines(int n, wxPoint points[],
                         wxCoord xoffset, wxCoord yoffset)
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoDrawLines - invalid DC") );

    if ( !m_logicalFunctionSupported )
        return;

    wxPoint2DDouble* pointsD = new wxPoint2DDouble[n];
    for( int i = 0; i < n; ++i)
    {
        pointsD[i].m_x = points[i].x + xoffset;
        pointsD[i].m_y = points[i].y + yoffset;
    }

    m_graphicContext->StrokeLines( n , pointsD);
    delete[] pointsD;
}

#if wxUSE_SPLINES
void wxGCDC::DoDrawSpline(wxList *points)
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoDrawSpline - invalid DC") );

    if ( !m_logicalFunctionSupported )
        return;

    wxGraphicsPath path = m_graphicContext->CreatePath();

    wxList::compatibility_iterator node = points->GetFirst();
    if (node == wxList::compatibility_iterator())
        // empty list
        return;

    wxPoint *p = (wxPoint *)node->GetData();

    wxCoord x1 = p->x;
    wxCoord y1 = p->y;

    node = node->GetNext();
    p = (wxPoint *)node->GetData();

    wxCoord x2 = p->x;
    wxCoord y2 = p->y;
    wxCoord cx1 = ( x1 + x2 ) / 2;
    wxCoord cy1 = ( y1 + y2 ) / 2;

    path.MoveToPoint( x1 , y1 );
    path.AddLineToPoint( cx1 , cy1 );
#if !wxUSE_STL

    while ((node = node->GetNext()) != NULL)
#else

    while ((node = node->GetNext()))
#endif // !wxUSE_STL

    {
        p = (wxPoint *)node->GetData();
        x1 = x2;
        y1 = y2;
        x2 = p->x;
        y2 = p->y;
        wxCoord cx4 = (x1 + x2) / 2;
        wxCoord cy4 = (y1 + y2) / 2;

        path.AddQuadCurveToPoint(x1 , y1 ,cx4 , cy4 );

        cx1 = cx4;
        cy1 = cy4;
    }

    path.AddLineToPoint( x2 , y2 );

    m_graphicContext->StrokePath( path );
}
#endif // wxUSE_SPLINES

void wxGCDC::DoDrawPolygon( int n, wxPoint points[],
                            wxCoord xoffset, wxCoord yoffset,
                            int fillStyle )
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoDrawPolygon - invalid DC") );

    if ( n <= 0 || (m_brush.GetStyle() == wxTRANSPARENT && m_pen.GetStyle() == wxTRANSPARENT ) )
        return;
    if ( !m_logicalFunctionSupported )
        return;

    bool closeIt = false;
    if (points[n-1] != points[0])
        closeIt = true;

    wxPoint2DDouble* pointsD = new wxPoint2DDouble[n+(closeIt?1:0)];
    for( int i = 0; i < n; ++i)
    {
        pointsD[i].m_x = points[i].x + xoffset;
        pointsD[i].m_y = points[i].y + yoffset;
    }
    if ( closeIt )
        pointsD[n] = pointsD[0];

    m_graphicContext->DrawLines( n+(closeIt?1:0) , pointsD, fillStyle);
    delete[] pointsD;
}

void wxGCDC::DoDrawPolyPolygon(int n,
                               int count[],
                               wxPoint points[],
                               wxCoord xoffset,
                               wxCoord yoffset,
                               int fillStyle)
{
    wxASSERT(n > 1);
    wxGraphicsPath path = m_graphicContext->CreatePath();

    int i = 0;
    for ( int j = 0; j < n; ++j)
    {
        wxPoint start = points[i];
        path.MoveToPoint( start.x+ xoffset, start.y+ yoffset);
        ++i;
        int l = count[j];
        for ( int k = 1; k < l; ++k)
        {
            path.AddLineToPoint( points[i].x+ xoffset, points[i].y+ yoffset);
            ++i;
        }
        // close the polygon
        if ( start != points[i-1])
            path.AddLineToPoint( start.x+ xoffset, start.y+ yoffset);
    }
    m_graphicContext->DrawPath( path , fillStyle);
}

void wxGCDC::DoDrawRectangle(wxCoord x, wxCoord y, wxCoord w, wxCoord h)
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoDrawRectangle - invalid DC") );

    if ( !m_logicalFunctionSupported )
        return;

    // CMB: draw nothing if transformed w or h is 0
    if (w == 0 || h == 0)
        return;

    if ( m_graphicContext->ShouldOffset() )
    {
        // if we are offsetting the entire rectangle is moved 0.5, so the
        // border line gets off by 1
        w -= 1;
        h -= 1;
    }
    m_graphicContext->DrawRectangle(x,y,w,h);
}

void wxGCDC::DoDrawRoundedRectangle(wxCoord x, wxCoord y,
                                    wxCoord w, wxCoord h,
                                    double radius)
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoDrawRoundedRectangle - invalid DC") );

    if ( !m_logicalFunctionSupported )
        return;

    if (radius < 0.0)
        radius = - radius * ((w < h) ? w : h);

    // CMB: draw nothing if transformed w or h is 0
    if (w == 0 || h == 0)
        return;

    if ( m_graphicContext->ShouldOffset() )
    {
        // if we are offsetting the entire rectangle is moved 0.5, so the
        // border line gets off by 1
        w -= 1;
        h -= 1;
    }
    m_graphicContext->DrawRoundedRectangle( x,y,w,h,radius);
}

void wxGCDC::DoDrawEllipse(wxCoord x, wxCoord y, wxCoord w, wxCoord h)
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoDrawEllipse - invalid DC") );

    if ( !m_logicalFunctionSupported )
        return;

    if ( m_graphicContext->ShouldOffset() )
    {
        // if we are offsetting the entire rectangle is moved 0.5, so the
        // border line gets off by 1
        w -= 1;
        h -= 1;
    }
    m_graphicContext->DrawEllipse(x,y,w,h);
}

bool wxGCDC::CanDrawBitmap() const
{
    return true;
}

bool wxGCDC::DoBlit(
    wxCoord xdest, wxCoord ydest, wxCoord width, wxCoord height,
    wxDC *source, wxCoord xsrc, wxCoord ysrc, int logical_func , bool WXUNUSED(useMask),
    wxCoord xsrcMask, wxCoord ysrcMask )
{
    wxCHECK_MSG( Ok(), false, wxT("wxGCDC(cg)::DoBlit - invalid DC") );
    wxCHECK_MSG( source->Ok(), false, wxT("wxGCDC(cg)::DoBlit - invalid source DC") );

    if ( logical_func == wxNO_OP )
        return true;
    else if ( !m_graphicContext->SetLogicalFunction( logical_func ) )
    
    {
        wxFAIL_MSG( wxT("Logical function is not supported by the graphics context.") );
        return false;
    }

    if (xsrcMask == -1 && ysrcMask == -1)
    {
        xsrcMask = xsrc;
        ysrcMask = ysrc;
    }

    wxRect subrect(source->LogicalToDeviceX(xsrc),
                   source->LogicalToDeviceY(ysrc),
                   source->LogicalToDeviceXRel(width),
                   source->LogicalToDeviceYRel(height));

    // if needed clip the subrect down to the size of the source DC
    wxCoord sw, sh;
    source->GetSize(&sw, &sh);
    sw = source->LogicalToDeviceXRel(sw);
    sh = source->LogicalToDeviceYRel(sh);
    if (subrect.x + subrect.width > sw)
        subrect.width = sw - subrect.x;
    if (subrect.y + subrect.height > sh)
        subrect.height = sh - subrect.y;

    wxBitmap blit = source->GetAsBitmap( &subrect );

    if ( blit.Ok() )
    {
        m_graphicContext->DrawBitmap( blit, xdest, ydest,
                                      wxMin(width, blit.GetWidth()),
                                      wxMin(height, blit.GetHeight()));
    }
    else
    {
        wxFAIL_MSG( wxT("Cannot Blit. Unable to get contents of DC as bitmap.") );
        return false;
    }

    // reset logical function
    m_graphicContext->SetLogicalFunction( m_logicalFunction );

    return true;
}

void wxGCDC::DoDrawRotatedText(const wxString& str, wxCoord x, wxCoord y,
                               double angle)
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoDrawRotatedText - invalid DC") );

    if ( str.length() == 0 )
        return;
    if ( !m_logicalFunctionSupported )
        return;

    if ( m_backgroundMode == wxTRANSPARENT )
        m_graphicContext->DrawText( str, x ,y , DegToRad(angle ));
    else
        m_graphicContext->DrawText( str, x ,y , DegToRad(angle ), m_graphicContext->CreateBrush( wxBrush(m_textBackgroundColour,wxSOLID) ) );
}

void wxGCDC::DoDrawText(const wxString& str, wxCoord x, wxCoord y)
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoDrawRotatedText - invalid DC") );

    if ( str.length() == 0 )
        return;

    if ( !m_logicalFunctionSupported )
        return;

    if ( m_backgroundMode == wxTRANSPARENT )
        m_graphicContext->DrawText( str, x ,y);
    else
        m_graphicContext->DrawText( str, x ,y , m_graphicContext->CreateBrush( wxBrush(m_textBackgroundColour,wxSOLID) ) );
}

bool wxGCDC::CanGetTextExtent() const
{
    wxCHECK_MSG( Ok(), false, wxT("wxGCDC(cg)::CanGetTextExtent - invalid DC") );

    return true;
}

void wxGCDC::DoGetTextExtent( const wxString &str, wxCoord *width, wxCoord *height,
                              wxCoord *descent, wxCoord *externalLeading ,
                              wxFont *theFont ) const
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::DoGetTextExtent - invalid DC") );

    if ( theFont )
    {
        m_graphicContext->SetFont( *theFont, m_textForegroundColour );
    }

    wxDouble h , d , e , w;

    m_graphicContext->GetTextExtent( str, &w, &h, &d, &e );

    if ( height )
        *height = (wxCoord)(h+0.5);
    if ( descent )
        *descent = (wxCoord)(d+0.5);
    if ( externalLeading )
        *externalLeading = (wxCoord)(e+0.5);
    if ( width )
        *width = (wxCoord)(w+0.5);

    if ( theFont )
    {
        m_graphicContext->SetFont( m_font, m_textForegroundColour );
    }
}

bool wxGCDC::DoGetPartialTextExtents(const wxString& text, wxArrayInt& widths) const
{
    wxCHECK_MSG( Ok(), false, wxT("wxGCDC(cg)::DoGetPartialTextExtents - invalid DC") );
    widths.Clear();
    widths.Add(0,text.Length());
    if ( text.IsEmpty() )
        return true;

    wxArrayDouble widthsD;

    m_graphicContext->GetPartialTextExtents( text, widthsD );
    for ( size_t i = 0; i < widths.GetCount(); ++i )
        widths[i] = (wxCoord)(widthsD[i] + 0.5);

    return true;
}

wxCoord wxGCDC::GetCharWidth(void) const
{
    wxCoord width;
    DoGetTextExtent( wxT("g") , &width , NULL , NULL , NULL , NULL );

    return width;
}

wxCoord wxGCDC::GetCharHeight(void) const
{
    wxCoord height;
    DoGetTextExtent( wxT("g") , NULL , &height , NULL , NULL , NULL );

    return height;
}

void wxGCDC::Clear(void)
{
    wxCHECK_RET( Ok(), wxT("wxGCDC(cg)::Clear - invalid DC") );
    // TODO better implementation / incorporate size info into wxGCDC or context
    m_graphicContext->SetBrush( m_backgroundBrush );
    wxPen p = *wxTRANSPARENT_PEN;
    m_graphicContext->SetPen( p );
    DoDrawRectangle( 0, 0, 32000 , 32000 );
    m_graphicContext->SetPen( m_pen );
    m_graphicContext->SetBrush( m_brush );
}

void wxGCDC::DoGetSize(int *width, int *height) const
{
    *width = 10000;
    *height = 10000;
}

void wxGCDC::DoGradientFillLinear(const wxRect& rect,
                                  const wxColour& initialColour,
                                  const wxColour& destColour,
                                  wxDirection nDirection )
{
    wxPoint start;
    wxPoint end;
    switch( nDirection)
    {
    case wxWEST :
        start = rect.GetRightBottom();
        start.x++;
        end = rect.GetLeftBottom();
        break;
    case wxEAST :
        start = rect.GetLeftBottom();
        end = rect.GetRightBottom();
        end.x++;
        break;
    case wxNORTH :
        start = rect.GetLeftBottom();
        start.y++;
        end = rect.GetLeftTop();
        break;
    case wxSOUTH :
        start = rect.GetLeftTop();
        end = rect.GetLeftBottom();
        end.y++;
        break;
    default :
        break;
    }

    if (rect.width == 0 || rect.height == 0)
        return;

    m_graphicContext->SetBrush( m_graphicContext->CreateLinearGradientBrush(
        start.x,start.y,end.x,end.y, initialColour, destColour));
    m_graphicContext->SetPen(*wxTRANSPARENT_PEN);
    m_graphicContext->DrawRectangle(rect.x,rect.y,rect.width,rect.height);
    m_graphicContext->SetPen(m_pen);
}

void wxGCDC::DoGradientFillConcentric(const wxRect& rect,
                                      const wxColour& initialColour,
                                      const wxColour& destColour,
                                      const wxPoint& circleCenter)
{
    //Radius
    wxInt32 cx = rect.GetWidth() / 2;
    wxInt32 cy = rect.GetHeight() / 2;
    wxInt32 nRadius;
    if (cx < cy)
        nRadius = cx;
    else
        nRadius = cy;

    // make sure the background is filled (todo move into specific platform implementation ?)
    m_graphicContext->SetPen(*wxTRANSPARENT_PEN);
    m_graphicContext->SetBrush( wxBrush( destColour) );
    m_graphicContext->DrawRectangle(rect.x,rect.y,rect.width,rect.height);

    m_graphicContext->SetBrush( m_graphicContext->CreateRadialGradientBrush(
        rect.x+circleCenter.x,rect.y+circleCenter.y,
        rect.x+circleCenter.x,rect.y+circleCenter.y,
        nRadius,initialColour,destColour));

    m_graphicContext->DrawRectangle(rect.x,rect.y,rect.width,rect.height);
    m_graphicContext->SetPen(m_pen);
}

void wxGCDC::DoDrawCheckMark(wxCoord x, wxCoord y,
                             wxCoord width, wxCoord height)
{
    wxDCBase::DoDrawCheckMark(x,y,width,height);
}

#endif // wxUSE_GRAPHICS_CONTEXT
