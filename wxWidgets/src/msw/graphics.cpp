/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/graphics.cpp
// Purpose:     wxGCDC class
// Author:      Stefan Csomor
// Modified by:
// Created:     2006-09-30
// RCS-ID:      $Id: graphics.cpp 61747 2009-08-23 21:36:09Z VZ $
// Copyright:   (c) 2006 Stefan Csomor
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#include "wx/dc.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/msw/wrapcdlg.h"
#include "wx/image.h"
#include "wx/window.h"
#include "wx/dc.h"
#include "wx/utils.h"
#include "wx/dialog.h"
#include "wx/app.h"
#include "wx/bitmap.h"
#include "wx/dcmemory.h"
#include "wx/log.h"
#include "wx/icon.h"
#include "wx/dcprint.h"
#include "wx/module.h"
#endif

#include "wx/graphics.h"

#if wxUSE_GRAPHICS_CONTEXT

#include <vector>

using namespace std;

//-----------------------------------------------------------------------------
// constants
//-----------------------------------------------------------------------------

const double RAD2DEG = 180.0 / M_PI;

//-----------------------------------------------------------------------------
// Local functions
//-----------------------------------------------------------------------------

static inline double dmin(double a, double b) { return a < b ? a : b; }
static inline double dmax(double a, double b) { return a > b ? a : b; }

static inline double DegToRad(double deg) { return (deg * M_PI) / 180.0; }
static inline double RadToDeg(double deg) { return (deg * 180.0) / M_PI; }

//-----------------------------------------------------------------------------
// device context implementation
//
// more and more of the dc functionality should be implemented by calling
// the appropricate wxGDIPlusContext, but we will have to do that step by step
// also coordinate conversions should be moved to native matrix ops
//-----------------------------------------------------------------------------

// we always stock two context states, one at entry, to be able to preserve the
// state we were called with, the other one after changing to HI Graphics orientation
// (this one is used for getting back clippings etc)

//-----------------------------------------------------------------------------
// wxGraphicsPath implementation
//-----------------------------------------------------------------------------

#include "wx/msw/private.h" // needs to be before #include <commdlg.h>

#if wxUSE_COMMON_DIALOGS && !defined(__WXMICROWIN__)
#include <commdlg.h>
#endif

// TODO remove this dependency (gdiplus needs the macros)

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#include "gdiplus.h"
using namespace Gdiplus;

class WXDLLIMPEXP_CORE wxGDIPlusPathData : public wxGraphicsPathData
{
public :
    wxGDIPlusPathData(wxGraphicsRenderer* renderer, GraphicsPath* path = NULL);
    ~wxGDIPlusPathData();

    virtual wxGraphicsObjectRefData *Clone() const;

    //
    // These are the path primitives from which everything else can be constructed
    //

    // begins a new subpath at (x,y)
    virtual void MoveToPoint( wxDouble x, wxDouble y );

    // adds a straight line from the current point to (x,y)
    virtual void AddLineToPoint( wxDouble x, wxDouble y );

    // adds a cubic Bezier curve from the current point, using two control points and an end point
    virtual void AddCurveToPoint( wxDouble cx1, wxDouble cy1, wxDouble cx2, wxDouble cy2, wxDouble x, wxDouble y );


    // adds an arc of a circle centering at (x,y) with radius (r) from startAngle to endAngle
    virtual void AddArc( wxDouble x, wxDouble y, wxDouble r, wxDouble startAngle, wxDouble endAngle, bool clockwise ) ;

    // gets the last point of the current path, (0,0) if not yet set
    virtual void GetCurrentPoint( wxDouble* x, wxDouble* y) const;

    // adds another path
    virtual void AddPath( const wxGraphicsPathData* path );

    // closes the current sub-path
    virtual void CloseSubpath();

    //
    // These are convenience functions which - if not available natively will be assembled
    // using the primitives from above
    //

    // appends a rectangle as a new closed subpath
    virtual void AddRectangle( wxDouble x, wxDouble y, wxDouble w, wxDouble h ) ;
    /*

    // appends an ellipsis as a new closed subpath fitting the passed rectangle
    virtual void AddEllipsis( wxDouble x, wxDouble y, wxDouble w , wxDouble h ) ;

    // draws a an arc to two tangents connecting (current) to (x1,y1) and (x1,y1) to (x2,y2), also a straight line from (current) to (x1,y1)
    virtual void AddArcToPoint( wxDouble x1, wxDouble y1 , wxDouble x2, wxDouble y2, wxDouble r )  ;
*/

    // returns the native path
    virtual void * GetNativePath() const { return m_path; }

    // give the native path returned by GetNativePath() back (there might be some deallocations necessary)
    virtual void UnGetNativePath(void * WXUNUSED(path)) const {}

    // transforms each point of this path by the matrix
    virtual void Transform( const wxGraphicsMatrixData* matrix ) ;

    // gets the bounding box enclosing all points (possibly including control points)
    virtual void GetBox(wxDouble *x, wxDouble *y, wxDouble *w, wxDouble *h) const;

    virtual bool Contains( wxDouble x, wxDouble y, int fillStyle = wxODDEVEN_RULE) const;

private :
    GraphicsPath* m_path;
};

class WXDLLIMPEXP_CORE wxGDIPlusMatrixData : public wxGraphicsMatrixData
{
public :
    wxGDIPlusMatrixData(wxGraphicsRenderer* renderer, Matrix* matrix = NULL) ;
    virtual ~wxGDIPlusMatrixData() ;

    virtual wxGraphicsObjectRefData* Clone() const ;

    // concatenates the matrix
    virtual void Concat( const wxGraphicsMatrixData *t );

    // sets the matrix to the respective values
    virtual void Set(wxDouble a=1.0, wxDouble b=0.0, wxDouble c=0.0, wxDouble d=1.0,
        wxDouble tx=0.0, wxDouble ty=0.0);

    // gets the component valuess of the matrix
    virtual void Get(wxDouble* a=NULL, wxDouble* b=NULL,  wxDouble* c=NULL,
                     wxDouble* d=NULL, wxDouble* tx=NULL, wxDouble* ty=NULL) const;
       
    // makes this the inverse matrix
    virtual void Invert();

    // returns true if the elements of the transformation matrix are equal ?
    virtual bool IsEqual( const wxGraphicsMatrixData* t) const ;

    // return true if this is the identity matrix
    virtual bool IsIdentity() const;

    //
    // transformation
    //

    // add the translation to this matrix
    virtual void Translate( wxDouble dx , wxDouble dy );

    // add the scale to this matrix
    virtual void Scale( wxDouble xScale , wxDouble yScale );

    // add the rotation to this matrix (radians)
    virtual void Rotate( wxDouble angle );

    //
    // apply the transforms
    //

    // applies that matrix to the point
    virtual void TransformPoint( wxDouble *x, wxDouble *y ) const;

    // applies the matrix except for translations
    virtual void TransformDistance( wxDouble *dx, wxDouble *dy ) const;

    // returns the native representation
    virtual void * GetNativeMatrix() const;
private:
    Matrix* m_matrix ;
} ;

class WXDLLIMPEXP_CORE wxGDIPlusPenData : public wxGraphicsObjectRefData
{
public:
    wxGDIPlusPenData( wxGraphicsRenderer* renderer, const wxPen &pen );
    ~wxGDIPlusPenData();

    void Init();

    virtual wxDouble GetWidth() { return m_width; }
    virtual Pen* GetGDIPlusPen() { return m_pen; }

protected :
    Pen* m_pen;
    Image* m_penImage;
    Brush* m_penBrush;

    wxDouble m_width;
};

class WXDLLIMPEXP_CORE wxGDIPlusBrushData : public wxGraphicsObjectRefData
{
public:
    wxGDIPlusBrushData( wxGraphicsRenderer* renderer );
    wxGDIPlusBrushData( wxGraphicsRenderer* renderer, const wxBrush &brush );
    ~wxGDIPlusBrushData ();

    void CreateLinearGradientBrush( wxDouble x1, wxDouble y1, wxDouble x2, wxDouble y2,
        const wxColour&c1, const wxColour&c2 );
    void CreateRadialGradientBrush( wxDouble xo, wxDouble yo, wxDouble xc, wxDouble yc, wxDouble radius,
        const wxColour &oColor, const wxColour &cColor );
    virtual Brush* GetGDIPlusBrush() { return m_brush; }

protected:
    virtual void Init();

private :
    Brush* m_brush;
    Image* m_brushImage;
    GraphicsPath* m_brushPath;
};

class WXDLLIMPEXP_CORE wxGDIPlusBitmapData : public wxGraphicsObjectRefData
{
public:
    wxGDIPlusBitmapData( wxGraphicsRenderer* renderer );
    wxGDIPlusBitmapData( wxGraphicsRenderer* renderer, const wxBitmap &bmp );
    ~wxGDIPlusBitmapData ();

    virtual Bitmap* GetGDIPlusBitmap() { return m_bitmap; }

private :
    Bitmap* m_bitmap;
    Bitmap* m_helper;
};

class WXDLLIMPEXP_CORE wxGDIPlusFontData : public wxGraphicsObjectRefData
{
public:
    wxGDIPlusFontData( wxGraphicsRenderer* renderer, const wxFont &font, const wxColour& col );
    ~wxGDIPlusFontData();

    virtual Brush* GetGDIPlusBrush() { return m_textBrush; }
    virtual Font* GetGDIPlusFont() { return m_font; }
private :
    Brush* m_textBrush;
    Font* m_font;
};

class WXDLLIMPEXP_CORE wxGDIPlusContext : public wxGraphicsContext
{
public:
    wxGDIPlusContext( wxGraphicsRenderer* renderer, HDC hdc );
    wxGDIPlusContext( wxGraphicsRenderer* renderer, HWND hwnd );
    wxGDIPlusContext( wxGraphicsRenderer* renderer, Graphics* gr);
    wxGDIPlusContext();

    virtual ~wxGDIPlusContext();

    virtual void Clip( const wxRegion &region );
    // clips drawings to the rect
    virtual void Clip( wxDouble x, wxDouble y, wxDouble w, wxDouble h );

    // resets the clipping to original extent
    virtual void ResetClip();

    virtual void * GetNativeContext();

    virtual void StrokePath( const wxGraphicsPath& p );
    virtual void FillPath( const wxGraphicsPath& p , int fillStyle = wxODDEVEN_RULE );

    // stroke lines connecting each of the points
    virtual void StrokeLines( size_t n, const wxPoint2DDouble *points);

    // draws a polygon
    virtual void DrawLines( size_t n, const wxPoint2DDouble *points, int fillStyle = wxODDEVEN_RULE );

    virtual void Translate( wxDouble dx , wxDouble dy );
    virtual void Scale( wxDouble xScale , wxDouble yScale );
    virtual void Rotate( wxDouble angle );

    // concatenates this transform with the current transform of this context
    virtual void ConcatTransform( const wxGraphicsMatrix& matrix );

    // sets the transform of this context
    virtual void SetTransform( const wxGraphicsMatrix& matrix );

    // gets the matrix of this context
    virtual wxGraphicsMatrix GetTransform() const;

    void DrawGraphicsBitmapInternal( const wxGraphicsBitmap &bmp, wxDouble x, wxDouble y, wxDouble w, wxDouble h );
    virtual void DrawBitmap( const wxBitmap &bmp, wxDouble x, wxDouble y, wxDouble w, wxDouble h );
    virtual void DrawIcon( const wxIcon &icon, wxDouble x, wxDouble y, wxDouble w, wxDouble h );
    virtual void PushState();
    virtual void PopState();

    virtual void DrawText( const wxString &str, wxDouble x, wxDouble y);
    virtual void GetTextExtent( const wxString &str, wxDouble *width, wxDouble *height,
        wxDouble *descent, wxDouble *externalLeading ) const;
    virtual void GetPartialTextExtents(const wxString& text, wxArrayDouble& widths) const;
    virtual bool ShouldOffset() const;

private:
    void    Init();
    void    SetDefaults();

    Graphics* m_context;
    vector<GraphicsState> m_stateStack;
    GraphicsState m_state1;
    GraphicsState m_state2;

    DECLARE_DYNAMIC_CLASS_NO_COPY(wxGDIPlusContext)
};

class WXDLLIMPEXP_CORE wxGDIPlusMeasuringContext : public wxGDIPlusContext
{
public:
    wxGDIPlusMeasuringContext( wxGraphicsRenderer* renderer ) : wxGDIPlusContext( renderer , m_hdc = GetDC(NULL) )
    {
    }
    wxGDIPlusMeasuringContext()
    {
    }

    virtual ~wxGDIPlusMeasuringContext()
    {
        ReleaseDC( NULL, m_hdc );
    }

private:
    HDC m_hdc ;
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxGDIPlusMeasuringContext)
} ;

//-----------------------------------------------------------------------------
// wxGDIPlusPen implementation
//-----------------------------------------------------------------------------

wxGDIPlusPenData::~wxGDIPlusPenData()
{
    delete m_pen;
    delete m_penImage;
    delete m_penBrush;
}

void wxGDIPlusPenData::Init()
{
    m_pen = NULL ;
    m_penImage = NULL;
    m_penBrush = NULL;
}

wxGDIPlusPenData::wxGDIPlusPenData( wxGraphicsRenderer* renderer, const wxPen &pen )
: wxGraphicsObjectRefData(renderer)
{
    Init();
    m_width = pen.GetWidth();
    if (m_width <= 0.0)
        m_width = 0.1;

    m_pen = new Pen(Color( pen.GetColour().Alpha() , pen.GetColour().Red() ,
        pen.GetColour().Green() , pen.GetColour().Blue() ), m_width );

    LineCap cap;
    switch ( pen.GetCap() )
    {
    case wxCAP_ROUND :
        cap = LineCapRound;
        break;

    case wxCAP_PROJECTING :
        cap = LineCapSquare;
        break;

    case wxCAP_BUTT :
        cap = LineCapFlat; // TODO verify
        break;

    default :
        cap = LineCapFlat;
        break;
    }
    m_pen->SetLineCap(cap,cap, DashCapFlat);

    LineJoin join;
    switch ( pen.GetJoin() )
    {
    case wxJOIN_BEVEL :
        join = LineJoinBevel;
        break;

    case wxJOIN_MITER :
        join = LineJoinMiter;
        break;

    case wxJOIN_ROUND :
        join = LineJoinRound;
        break;

    default :
        join = LineJoinMiter;
        break;
    }

    m_pen->SetLineJoin(join);

    m_pen->SetDashStyle(DashStyleSolid);

    DashStyle dashStyle = DashStyleSolid;
    switch ( pen.GetStyle() )
    {
    case wxSOLID :
        break;

    case wxDOT :
        dashStyle = DashStyleDot;
        break;

    case wxLONG_DASH :
        dashStyle = DashStyleDash; // TODO verify
        break;

    case wxSHORT_DASH :
        dashStyle = DashStyleDash;
        break;

    case wxDOT_DASH :
        dashStyle = DashStyleDashDot;
        break;
    case wxUSER_DASH :
        {
            dashStyle = DashStyleCustom;
            wxDash *dashes;
            int count = pen.GetDashes( &dashes );
            if ((dashes != NULL) && (count > 0))
            {
                REAL *userLengths = new REAL[count];
                for ( int i = 0; i < count; ++i )
                {
                    userLengths[i] = dashes[i];
                }
                m_pen->SetDashPattern( userLengths, count);
                delete[] userLengths;
            }
        }
        break;
    case wxSTIPPLE :
        {
            wxBitmap* bmp = pen.GetStipple();
            if ( bmp && bmp->Ok() )
            {
                m_penImage = Bitmap::FromHBITMAP((HBITMAP)bmp->GetHBITMAP(),(HPALETTE)bmp->GetPalette()->GetHPALETTE());
                m_penBrush = new TextureBrush(m_penImage);
                m_pen->SetBrush( m_penBrush );
            }

        }
        break;
    default :
        if ( pen.GetStyle() >= wxFIRST_HATCH && pen.GetStyle() <= wxLAST_HATCH )
        {
            HatchStyle style = HatchStyleHorizontal;
            switch( pen.GetStyle() )
            {
            case wxBDIAGONAL_HATCH :
                style = HatchStyleBackwardDiagonal;
                break ;
            case wxCROSSDIAG_HATCH :
                style = HatchStyleDiagonalCross;
                break ;
            case wxFDIAGONAL_HATCH :
                style = HatchStyleForwardDiagonal;
                break ;
            case wxCROSS_HATCH :
                style = HatchStyleCross;
                break ;
            case wxHORIZONTAL_HATCH :
                style = HatchStyleHorizontal;
                break ;
            case wxVERTICAL_HATCH :
                style = HatchStyleVertical;
                break ;

            }
            m_penBrush = new HatchBrush(style,Color( pen.GetColour().Alpha() , pen.GetColour().Red() ,
                pen.GetColour().Green() , pen.GetColour().Blue() ), Color::Transparent );
            m_pen->SetBrush( m_penBrush );
        }
        break;
    }
    if ( dashStyle != DashStyleSolid )
        m_pen->SetDashStyle(dashStyle);
}

//-----------------------------------------------------------------------------
// wxGDIPlusBrush implementation
//-----------------------------------------------------------------------------

wxGDIPlusBrushData::wxGDIPlusBrushData( wxGraphicsRenderer* renderer )
: wxGraphicsObjectRefData(renderer)
{
    Init();
}

wxGDIPlusBrushData::wxGDIPlusBrushData( wxGraphicsRenderer* renderer , const wxBrush &brush )
: wxGraphicsObjectRefData(renderer)
{
    Init();
    if ( brush.GetStyle() == wxSOLID)
    {
        m_brush = new SolidBrush( Color( brush.GetColour().Alpha() , brush.GetColour().Red() ,
            brush.GetColour().Green() , brush.GetColour().Blue() ) );
    }
    else if ( brush.IsHatch() )
    {
        HatchStyle style = HatchStyleHorizontal;
        switch( brush.GetStyle() )
        {
        case wxBDIAGONAL_HATCH :
            style = HatchStyleBackwardDiagonal;
            break ;
        case wxCROSSDIAG_HATCH :
            style = HatchStyleDiagonalCross;
            break ;
        case wxFDIAGONAL_HATCH :
            style = HatchStyleForwardDiagonal;
            break ;
        case wxCROSS_HATCH :
            style = HatchStyleCross;
            break ;
        case wxHORIZONTAL_HATCH :
            style = HatchStyleHorizontal;
            break ;
        case wxVERTICAL_HATCH :
            style = HatchStyleVertical;
            break ;

        }
        m_brush = new HatchBrush(style,Color( brush.GetColour().Alpha() , brush.GetColour().Red() ,
            brush.GetColour().Green() , brush.GetColour().Blue() ), Color::Transparent );
    }
    else
    {
        wxBitmap* bmp = brush.GetStipple();
        if ( bmp && bmp->Ok() )
        {
            wxDELETE( m_brushImage );
            m_brushImage = Bitmap::FromHBITMAP((HBITMAP)bmp->GetHBITMAP(),(HPALETTE)bmp->GetPalette()->GetHPALETTE());
            m_brush = new TextureBrush(m_brushImage);
        }
    }
}

wxGDIPlusBrushData::~wxGDIPlusBrushData()
{
    delete m_brush;
    delete m_brushImage;
    delete m_brushPath;
};

void wxGDIPlusBrushData::Init()
{
    m_brush = NULL;
    m_brushImage= NULL;
    m_brushPath= NULL;
}

void wxGDIPlusBrushData::CreateLinearGradientBrush( wxDouble x1, wxDouble y1, wxDouble x2, wxDouble y2, const wxColour&c1, const wxColour&c2)
{
    m_brush = new LinearGradientBrush( PointF( x1,y1) , PointF( x2,y2),
        Color( c1.Alpha(), c1.Red(),c1.Green() , c1.Blue() ),
        Color( c2.Alpha(), c2.Red(),c2.Green() , c2.Blue() ));
}

void wxGDIPlusBrushData::CreateRadialGradientBrush( wxDouble xo, wxDouble yo, wxDouble xc, wxDouble yc, wxDouble radius,
                                               const wxColour &oColor, const wxColour &cColor)
{
    // Create a path that consists of a single circle.
    m_brushPath = new GraphicsPath();
    m_brushPath->AddEllipse( (REAL)(xc-radius), (REAL)(yc-radius), (REAL)(2*radius), (REAL)(2*radius));

    PathGradientBrush *b = new PathGradientBrush(m_brushPath);
    m_brush = b;
    b->SetCenterPoint( PointF(xo,yo));
    b->SetCenterColor(Color( oColor.Alpha(), oColor.Red(),oColor.Green() , oColor.Blue() ));

    Color colors[] = {Color( cColor.Alpha(), cColor.Red(),cColor.Green() , cColor.Blue() )};
    int count = 1;
    b->SetSurroundColors(colors, &count);
}

wxGDIPlusBitmapData::wxGDIPlusBitmapData( wxGraphicsRenderer* renderer, 
                        const wxBitmap &bmp) : wxGraphicsObjectRefData( renderer )
{
    m_bitmap = NULL;
    m_helper = NULL;
    Bitmap* image = NULL;
    if ( bmp.GetMask() )
    {
        Bitmap interim((HBITMAP)bmp.GetHBITMAP(),(HPALETTE)bmp.GetPalette()->GetHPALETTE()) ;

        size_t width = interim.GetWidth();
        size_t height = interim.GetHeight();
        Rect bounds(0,0,width,height);

        image = new Bitmap(width,height,PixelFormat32bppPARGB) ;

        Bitmap interimMask((HBITMAP)bmp.GetMask()->GetMaskBitmap(),NULL);
        wxASSERT(interimMask.GetPixelFormat() == PixelFormat1bppIndexed);

        BitmapData dataMask ;
        interimMask.LockBits(&bounds,ImageLockModeRead,
            interimMask.GetPixelFormat(),&dataMask);


        BitmapData imageData ;
        image->LockBits(&bounds,ImageLockModeWrite, PixelFormat32bppPARGB, &imageData);

        BYTE maskPattern = 0 ;
        BYTE maskByte = 0;
        size_t maskIndex ;

        for ( size_t y = 0 ; y < height ; ++y)
        {
            maskIndex = 0 ;
            for( size_t x = 0 ; x < width; ++x)
            {
                if ( x % 8 == 0)
                {
                    maskPattern = 0x80;
                    maskByte = *((BYTE*)dataMask.Scan0 + dataMask.Stride*y + maskIndex);
                    maskIndex++;
                }
                else
                    maskPattern = maskPattern >> 1;

                ARGB *dest = (ARGB*)((BYTE*)imageData.Scan0 + imageData.Stride*y + x*4);
                if ( (maskByte & maskPattern) == 0 )
                    *dest = 0x00000000;
                else
                {
                    Color c ;
                    interim.GetPixel(x,y,&c) ;
                    *dest = (c.GetValue() | Color::AlphaMask);
                }
            }
        }

        image->UnlockBits(&imageData);

        interimMask.UnlockBits(&dataMask);
        interim.UnlockBits(&dataMask);
    }
    else
    {
        image = Bitmap::FromHBITMAP((HBITMAP)bmp.GetHBITMAP(),(HPALETTE)bmp.GetPalette()->GetHPALETTE());
        if ( bmp.HasAlpha() && GetPixelFormatSize(image->GetPixelFormat()) == 32 )
        {
            size_t width = image->GetWidth();
            size_t height = image->GetHeight();
            Rect bounds(0,0,width,height);
            static BitmapData data ;

            m_helper = image ;
            image = NULL ;
            m_helper->LockBits(&bounds, ImageLockModeRead,
                m_helper->GetPixelFormat(),&data);

            image = new Bitmap(data.Width, data.Height, data.Stride,
                PixelFormat32bppPARGB , (BYTE*) data.Scan0);

            m_helper->UnlockBits(&data);
        }
    }
    if ( image )
        m_bitmap = image;
}

wxGDIPlusBitmapData::~wxGDIPlusBitmapData()
{
    delete m_bitmap;
    delete m_helper;
}

//-----------------------------------------------------------------------------
// wxGDIPlusFont implementation
//-----------------------------------------------------------------------------

wxGDIPlusFontData::wxGDIPlusFontData( wxGraphicsRenderer* renderer, const wxFont &font,
                             const wxColour& col ) : wxGraphicsObjectRefData( renderer )
{
    m_textBrush = NULL;
    m_font = NULL;

    wxWCharBuffer s = font.GetFaceName().wc_str( *wxConvUI );
    int size = font.GetPointSize();
    int style = FontStyleRegular;
    if ( font.GetStyle() == wxFONTSTYLE_ITALIC )
        style |= FontStyleItalic;
    if ( font.GetUnderlined() )
        style |= FontStyleUnderline;
    if ( font.GetWeight() == wxFONTWEIGHT_BOLD )
        style |= FontStyleBold;
    m_font = new Font( s , size , style );
    m_textBrush = new SolidBrush( Color( col.Alpha() , col.Red() ,
        col.Green() , col.Blue() ));
}

wxGDIPlusFontData::~wxGDIPlusFontData()
{
    delete m_textBrush;
    delete m_font;
}

//-----------------------------------------------------------------------------
// wxGDIPlusPath implementation
//-----------------------------------------------------------------------------

wxGDIPlusPathData::wxGDIPlusPathData(wxGraphicsRenderer* renderer, GraphicsPath* path ) : wxGraphicsPathData(renderer)
{
    if ( path )
        m_path = path;
    else
        m_path = new GraphicsPath();
}

wxGDIPlusPathData::~wxGDIPlusPathData()
{
    delete m_path;
}

wxGraphicsObjectRefData* wxGDIPlusPathData::Clone() const
{
    return new wxGDIPlusPathData( GetRenderer() , m_path->Clone());
}

//
// The Primitives
//

void wxGDIPlusPathData::MoveToPoint( wxDouble x , wxDouble y )
{
    m_path->StartFigure();
    m_path->AddLine((REAL) x,(REAL) y,(REAL) x,(REAL) y);
}

void wxGDIPlusPathData::AddLineToPoint( wxDouble x , wxDouble y )
{
    m_path->AddLine((REAL) x,(REAL) y,(REAL) x,(REAL) y);
}

void wxGDIPlusPathData::CloseSubpath()
{
    m_path->CloseFigure();
}

void wxGDIPlusPathData::AddCurveToPoint( wxDouble cx1, wxDouble cy1, wxDouble cx2, wxDouble cy2, wxDouble x, wxDouble y )
{
    PointF c1(cx1,cy1);
    PointF c2(cx2,cy2);
    PointF end(x,y);
    PointF start;
    m_path->GetLastPoint(&start);
    m_path->AddBezier(start,c1,c2,end);
}

// gets the last point of the current path, (0,0) if not yet set
void wxGDIPlusPathData::GetCurrentPoint( wxDouble* x, wxDouble* y) const
{
    PointF start;
    m_path->GetLastPoint(&start);
    *x = start.X ;
    *y = start.Y ;
}

void wxGDIPlusPathData::AddArc( wxDouble x, wxDouble y, wxDouble r, double startAngle, double endAngle, bool clockwise )
{
    double sweepAngle = endAngle - startAngle ;
    if( fabs(sweepAngle) >= 2*M_PI)
    {
        sweepAngle = 2 * M_PI;
    }
    else
    {
        if ( clockwise )
        {
            if( sweepAngle < 0 )
                sweepAngle += 2 * M_PI;
        }
        else
        {
            if( sweepAngle > 0 )
                sweepAngle -= 2 * M_PI;

        }
   }
   m_path->AddArc((REAL) (x-r),(REAL) (y-r),(REAL) (2*r),(REAL) (2*r),RadToDeg(startAngle),RadToDeg(sweepAngle));
}

void wxGDIPlusPathData::AddRectangle( wxDouble x, wxDouble y, wxDouble w, wxDouble h )
{
    m_path->AddRectangle(RectF(x,y,w,h));
}

void wxGDIPlusPathData::AddPath( const wxGraphicsPathData* path )
{
    m_path->AddPath( (GraphicsPath*) path->GetNativePath(), FALSE);
}


// transforms each point of this path by the matrix
void wxGDIPlusPathData::Transform( const wxGraphicsMatrixData* matrix )
{
    m_path->Transform( (Matrix*) matrix->GetNativeMatrix() );
}

// gets the bounding box enclosing all points (possibly including control points)
void wxGDIPlusPathData::GetBox(wxDouble *x, wxDouble *y, wxDouble *w, wxDouble *h) const
{
    RectF bounds;
    m_path->GetBounds( &bounds, NULL, NULL) ;
    *x = bounds.X;
    *y = bounds.Y;
    *w = bounds.Width;
    *h = bounds.Height;
}

bool wxGDIPlusPathData::Contains( wxDouble x, wxDouble y, int fillStyle ) const
{
    m_path->SetFillMode( fillStyle == wxODDEVEN_RULE ? FillModeAlternate : FillModeWinding);
    return m_path->IsVisible( (FLOAT) x,(FLOAT) y) == TRUE ;
}

//-----------------------------------------------------------------------------
// wxGDIPlusMatrixData implementation
//-----------------------------------------------------------------------------

wxGDIPlusMatrixData::wxGDIPlusMatrixData(wxGraphicsRenderer* renderer, Matrix* matrix )
    : wxGraphicsMatrixData(renderer)
{
    if ( matrix )
        m_matrix = matrix ;
    else
        m_matrix = new Matrix();
}

wxGDIPlusMatrixData::~wxGDIPlusMatrixData()
{
    delete m_matrix;
}

wxGraphicsObjectRefData *wxGDIPlusMatrixData::Clone() const
{
    return new wxGDIPlusMatrixData( GetRenderer(), m_matrix->Clone());
}

// concatenates the matrix
void wxGDIPlusMatrixData::Concat( const wxGraphicsMatrixData *t )
{
    m_matrix->Multiply( (Matrix*) t->GetNativeMatrix());
}

// sets the matrix to the respective values
void wxGDIPlusMatrixData::Set(wxDouble a, wxDouble b, wxDouble c, wxDouble d,
                 wxDouble tx, wxDouble ty)
{
    m_matrix->SetElements(a,b,c,d,tx,ty);
}

// gets the component valuess of the matrix
void wxGDIPlusMatrixData::Get(wxDouble* a, wxDouble* b,  wxDouble* c,
                              wxDouble* d, wxDouble* tx, wxDouble* ty) const
{
    REAL elements[6];
    m_matrix->GetElements(elements);
    if (a)  *a = elements[0];
    if (b)  *b = elements[1];
    if (c)  *c = elements[2];
    if (d)  *d = elements[3];
    if (tx) *tx= elements[4];
    if (ty) *ty= elements[5];
}

// makes this the inverse matrix
void wxGDIPlusMatrixData::Invert()
{
    m_matrix->Invert();
}

// returns true if the elements of the transformation matrix are equal ?
bool wxGDIPlusMatrixData::IsEqual( const wxGraphicsMatrixData* t) const
{
    return m_matrix->Equals((Matrix*) t->GetNativeMatrix())== TRUE ;
}

// return true if this is the identity matrix
bool wxGDIPlusMatrixData::IsIdentity() const
{
    return m_matrix->IsIdentity() == TRUE ;
}

//
// transformation
//

// add the translation to this matrix
void wxGDIPlusMatrixData::Translate( wxDouble dx , wxDouble dy )
{
    m_matrix->Translate(dx,dy);
}

// add the scale to this matrix
void wxGDIPlusMatrixData::Scale( wxDouble xScale , wxDouble yScale )
{
    m_matrix->Scale(xScale,yScale);
}

// add the rotation to this matrix (radians)
void wxGDIPlusMatrixData::Rotate( wxDouble angle )
{
    m_matrix->Rotate( angle );
}

//
// apply the transforms
//

// applies that matrix to the point
void wxGDIPlusMatrixData::TransformPoint( wxDouble *x, wxDouble *y ) const
{
    PointF pt(*x,*y);
    m_matrix->TransformPoints(&pt);
    *x = pt.X;
    *y = pt.Y;
}

// applies the matrix except for translations
void wxGDIPlusMatrixData::TransformDistance( wxDouble *dx, wxDouble *dy ) const
{
    PointF pt(*dx,*dy);
    m_matrix->TransformVectors(&pt);
    *dx = pt.X;
    *dy = pt.Y;
}

// returns the native representation
void * wxGDIPlusMatrixData::GetNativeMatrix() const
{
    return m_matrix;
}

//-----------------------------------------------------------------------------
// wxGDIPlusContext implementation
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxGDIPlusContext,wxGraphicsContext)
IMPLEMENT_DYNAMIC_CLASS(wxGDIPlusMeasuringContext,wxGDIPlusContext)

class wxGDIPlusOffsetHelper
{
public :
    wxGDIPlusOffsetHelper( Graphics* gr , bool offset )
    {
        m_gr = gr;
        m_offset = offset;
        if ( m_offset )
            m_gr->TranslateTransform( 0.5, 0.5 );
    }
    ~wxGDIPlusOffsetHelper( )
    {
        if ( m_offset )
            m_gr->TranslateTransform( -0.5, -0.5 );
    }
public :
    Graphics* m_gr;
    bool m_offset;
} ;

wxGDIPlusContext::wxGDIPlusContext( wxGraphicsRenderer* renderer, HDC hdc  )
    : wxGraphicsContext(renderer)
{
    Init();
    m_context = new Graphics( hdc);
    SetDefaults();
}

wxGDIPlusContext::wxGDIPlusContext( wxGraphicsRenderer* renderer, HWND hwnd  )
    : wxGraphicsContext(renderer)
{
    Init();
    m_context = new Graphics( hwnd);
    SetDefaults();
}

wxGDIPlusContext::wxGDIPlusContext( wxGraphicsRenderer* renderer, Graphics* gr  )
    : wxGraphicsContext(renderer)
{
    Init();
    m_context = gr;
    SetDefaults();
}

wxGDIPlusContext::wxGDIPlusContext() : wxGraphicsContext(NULL)
{
    Init();
}

void wxGDIPlusContext::Init()
{
    m_context = NULL;
    m_state1 = 0;
    m_state2= 0;
}

void wxGDIPlusContext::SetDefaults()
{
    m_context->SetTextRenderingHint(TextRenderingHintSystemDefault);
    m_context->SetPixelOffsetMode(PixelOffsetModeHalf);
    m_context->SetSmoothingMode(SmoothingModeHighQuality);
    m_state1 = m_context->Save();
    m_state2 = m_context->Save();
}

wxGDIPlusContext::~wxGDIPlusContext()
{
    if ( m_context )
    {
        m_context->Restore( m_state2 );
        m_context->Restore( m_state1 );
        delete m_context;
    }
}


void wxGDIPlusContext::Clip( const wxRegion &region )
{
    Region rgn((HRGN)region.GetHRGN());
    m_context->SetClip(&rgn,CombineModeIntersect);
}

void wxGDIPlusContext::Clip( wxDouble x, wxDouble y, wxDouble w, wxDouble h )
{
    m_context->SetClip(RectF(x,y,w,h),CombineModeIntersect);
}

void wxGDIPlusContext::ResetClip()
{
    m_context->ResetClip();
}

void wxGDIPlusContext::StrokeLines( size_t n, const wxPoint2DDouble *points)
{
    if ( !m_pen.IsNull() )
    {
        wxGDIPlusOffsetHelper helper( m_context , ShouldOffset() );
        Point *cpoints = new Point[n];
        for (size_t i = 0; i < n; i++)
        {
            cpoints[i].X = (int)(points[i].m_x );
            cpoints[i].Y = (int)(points[i].m_y );

        } // for (size_t i = 0; i < n; i++)
        m_context->DrawLines( ((wxGDIPlusPenData*)m_pen.GetGraphicsData())->GetGDIPlusPen() , cpoints , n ) ;
        delete[] cpoints;
    }
}

void wxGDIPlusContext::DrawLines( size_t n, const wxPoint2DDouble *points, int WXUNUSED(fillStyle) )
{
    wxGDIPlusOffsetHelper helper( m_context , ShouldOffset() );
    Point *cpoints = new Point[n];
    for (size_t i = 0; i < n; i++)
    {
        cpoints[i].X = (int)(points[i].m_x );
        cpoints[i].Y = (int)(points[i].m_y );

    } // for (int i = 0; i < n; i++)
    if ( !m_brush.IsNull() )
        m_context->FillPolygon( ((wxGDIPlusBrushData*)m_brush.GetRefData())->GetGDIPlusBrush() , cpoints , n ) ;
    if ( !m_pen.IsNull() )
        m_context->DrawLines( ((wxGDIPlusPenData*)m_pen.GetGraphicsData())->GetGDIPlusPen() , cpoints , n ) ;
    delete[] cpoints;
}

void wxGDIPlusContext::StrokePath( const wxGraphicsPath& path )
{
    if ( !m_pen.IsNull() )
    {
        wxGDIPlusOffsetHelper helper( m_context , ShouldOffset() );
        m_context->DrawPath( ((wxGDIPlusPenData*)m_pen.GetGraphicsData())->GetGDIPlusPen() , (GraphicsPath*) path.GetNativePath() );
    }
}

void wxGDIPlusContext::FillPath( const wxGraphicsPath& path , int fillStyle )
{
    if ( !m_brush.IsNull() )
    {
        wxGDIPlusOffsetHelper helper( m_context , ShouldOffset() );
        ((GraphicsPath*) path.GetNativePath())->SetFillMode( fillStyle == wxODDEVEN_RULE ? FillModeAlternate : FillModeWinding);
        m_context->FillPath( ((wxGDIPlusBrushData*)m_brush.GetRefData())->GetGDIPlusBrush() ,
            (GraphicsPath*) path.GetNativePath());
    }
}

void wxGDIPlusContext::Rotate( wxDouble angle )
{
    m_context->RotateTransform( RadToDeg(angle) );
}

void wxGDIPlusContext::Translate( wxDouble dx , wxDouble dy )
{
    m_context->TranslateTransform( dx , dy );
}

void wxGDIPlusContext::Scale( wxDouble xScale , wxDouble yScale )
{
    m_context->ScaleTransform(xScale,yScale);
}

void wxGDIPlusContext::PushState()
{
    GraphicsState state = m_context->Save();
    m_stateStack.push_back(state);
}

void wxGDIPlusContext::PopState()
{
    GraphicsState state = m_stateStack.back();
    m_stateStack.pop_back();
    m_context->Restore(state);
}

// the built-in conversions functions create non-premultiplied bitmaps, while GDIPlus needs them in the
// premultiplied format, therefore in the failing cases we create a new bitmap using the non-premultiplied
// bytes as parameter

void wxGraphicsContext::DrawGraphicsBitmap( const wxGraphicsBitmap &bmp, wxDouble x, wxDouble y, wxDouble w, wxDouble h )
{
    static_cast<wxGDIPlusContext*>(this)->DrawGraphicsBitmapInternal(bmp, x, y, w, h);
}

void wxGDIPlusContext::DrawGraphicsBitmapInternal( const wxGraphicsBitmap &bmp, wxDouble x, wxDouble y, wxDouble w, wxDouble h )
{
    Bitmap* image = static_cast<wxGDIPlusBitmapData*>(bmp.GetRefData())->GetGDIPlusBitmap();
    if ( image )
    {
        if( image->GetWidth() != (UINT) w || image->GetHeight() != (UINT) h )
        {
            Rect drawRect((REAL) x, (REAL)y, (REAL)w, (REAL)h);
            m_context->SetPixelOffsetMode( PixelOffsetModeNone );
            m_context->DrawImage(image, drawRect, 0 , 0 , image->GetWidth()-1, image->GetHeight()-1, UnitPixel ) ;
            m_context->SetPixelOffsetMode( PixelOffsetModeHalf );
        }
        else
            m_context->DrawImage(image,(REAL) x,(REAL) y,(REAL) w,(REAL) h) ;
    }
}

void wxGDIPlusContext::DrawBitmap( const wxBitmap &bmp, wxDouble x, wxDouble y, wxDouble w, wxDouble h )
{
    wxGraphicsBitmap bitmap = GetRenderer()->CreateBitmap(bmp);
    DrawGraphicsBitmapInternal(bitmap, x, y, w, h);
}

void wxGDIPlusContext::DrawIcon( const wxIcon &icon, wxDouble x, wxDouble y, wxDouble w, wxDouble h )
{
    // the built-in conversion fails when there is alpha in the HICON (eg XP style icons), we can only
    // find out by looking at the bitmap data whether there really was alpha in it
    HICON hIcon = (HICON)icon.GetHICON();
    ICONINFO iconInfo ;
    // IconInfo creates the bitmaps for color and mask, we must dispose of them after use
    if (!GetIconInfo(hIcon,&iconInfo))
        return;

    Bitmap interim(iconInfo.hbmColor,NULL);

    Bitmap* image = NULL ;

    // if it's not 32 bit, it doesn't have an alpha channel, note that since the conversion doesn't
    // work correctly, asking IsAlphaPixelFormat at this point fails as well
    if( GetPixelFormatSize(interim.GetPixelFormat())!= 32 )
    {
        image = Bitmap::FromHICON(hIcon);
    }
    else
    {
        size_t width = interim.GetWidth();
        size_t height = interim.GetHeight();
        Rect bounds(0,0,width,height);
        BitmapData data ;

        interim.LockBits(&bounds, ImageLockModeRead,
            interim.GetPixelFormat(),&data);
        
        bool hasAlpha = false;
        for ( size_t y = 0 ; y < height && !hasAlpha ; ++y)
        {
            for( size_t x = 0 ; x < width && !hasAlpha; ++x)
            {
                ARGB *dest = (ARGB*)((BYTE*)data.Scan0 + data.Stride*y + x*4);
                if ( ( *dest & Color::AlphaMask ) != 0 )
                    hasAlpha = true;
            }
        }

        if ( hasAlpha )
        {
            image = new Bitmap(data.Width, data.Height, data.Stride,
                PixelFormat32bppARGB , (BYTE*) data.Scan0);
        }
        else
        {
            image = Bitmap::FromHICON(hIcon);
        }

        interim.UnlockBits(&data);
    }

    m_context->DrawImage(image,(REAL) x,(REAL) y,(REAL) w,(REAL) h) ;

    delete image ;
    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);
}

void wxGDIPlusContext::DrawText( const wxString &str, wxDouble x, wxDouble y )
{
    wxCHECK_RET( !m_font.IsNull(), wxT("wxGDIPlusContext::DrawText - no valid font set") );

    if ( str.IsEmpty())
        return ;

    wxWCharBuffer s = str.wc_str( *wxConvUI );
    m_context->DrawString( s , -1 , ((wxGDIPlusFontData*)m_font.GetRefData())->GetGDIPlusFont() ,
            PointF( x , y ) , StringFormat::GenericTypographic() , ((wxGDIPlusFontData*)m_font.GetRefData())->GetGDIPlusBrush() );
}

void wxGDIPlusContext::GetTextExtent( const wxString &str, wxDouble *width, wxDouble *height,
                                     wxDouble *descent, wxDouble *externalLeading ) const
{
    wxCHECK_RET( !m_font.IsNull(), wxT("wxGDIPlusContext::GetTextExtent - no valid font set") );

    wxWCharBuffer s = str.wc_str( *wxConvUI );
    FontFamily ffamily ;
    Font* f = ((wxGDIPlusFontData*)m_font.GetRefData())->GetGDIPlusFont();

    f->GetFamily(&ffamily) ;

    REAL factorY = m_context->GetDpiY() / 72.0 ;

    REAL rDescent = ffamily.GetCellDescent(FontStyleRegular) *
        f->GetSize() / ffamily.GetEmHeight(FontStyleRegular);
    REAL rAscent = ffamily.GetCellAscent(FontStyleRegular) *
        f->GetSize() / ffamily.GetEmHeight(FontStyleRegular);
    REAL rHeight = ffamily.GetLineSpacing(FontStyleRegular) *
        f->GetSize() / ffamily.GetEmHeight(FontStyleRegular);

    if ( height )
        *height = rHeight * factorY;
    if ( descent )
        *descent = rDescent * factorY;
    if ( externalLeading )
        *externalLeading = (rHeight - rAscent - rDescent) * factorY;
    // measuring empty strings is not guaranteed, so do it by hand
    if ( str.IsEmpty())
    {
        if ( width )
            *width = 0 ;
    }
    else
    {
        RectF layoutRect(0,0, 100000.0f, 100000.0f);
        StringFormat strFormat( StringFormat::GenericTypographic() );
        strFormat.SetFormatFlags( StringFormatFlagsMeasureTrailingSpaces | strFormat.GetFormatFlags() ); 

        RectF bounds ;
        m_context->MeasureString((const wchar_t *) s , wcslen(s) , f, layoutRect, &strFormat, &bounds ) ;
        if ( width )
            *width = bounds.Width;
    }
}

void wxGDIPlusContext::GetPartialTextExtents(const wxString& text, wxArrayDouble& widths) const
{
    widths.Empty();
    widths.Add(0, text.length());

    wxCHECK_RET( !m_font.IsNull(), wxT("wxGDIPlusContext::GetPartialTextExtents - no valid font set") );

    if (text.empty())
        return;

    Font* f = ((wxGDIPlusFontData*)m_font.GetRefData())->GetGDIPlusFont();
    wxWCharBuffer ws = text.wc_str( *wxConvUI );
    size_t len = wcslen( ws ) ;
    wxASSERT_MSG(text.length() == len , wxT("GetPartialTextExtents not yet implemented for multichar situations"));

    RectF layoutRect(0,0, 100000.0f, 100000.0f);
    StringFormat strFormat( StringFormat::GenericTypographic() );

    CharacterRange* ranges = new CharacterRange[len] ;
    Region* regions = new Region[len];
    for( size_t i = 0 ; i < len ; ++i)
    {
        ranges[i].First = i ;
        ranges[i].Length = 1 ;
    }
    strFormat.SetMeasurableCharacterRanges(len,ranges);
    strFormat.SetFormatFlags( StringFormatFlagsMeasureTrailingSpaces | strFormat.GetFormatFlags() ); 
    m_context->MeasureCharacterRanges(ws, -1 , f,layoutRect, &strFormat,1,regions) ;

    RectF bbox ;
    for ( size_t i = 0 ; i < len ; ++i)
    {
        regions[i].GetBounds(&bbox,m_context);
        widths[i] = bbox.GetRight()-bbox.GetLeft();
    }
}

bool wxGDIPlusContext::ShouldOffset() const
{     
    int penwidth = 0 ;
    if ( !m_pen.IsNull() )
    {
        penwidth = (int)((wxGDIPlusPenData*)m_pen.GetRefData())->GetWidth();
        if ( penwidth == 0 )
            penwidth = 1;
    }
    return ( penwidth % 2 ) == 1;
}

void* wxGDIPlusContext::GetNativeContext()
{
    return m_context;
}

// concatenates this transform with the current transform of this context
void wxGDIPlusContext::ConcatTransform( const wxGraphicsMatrix& matrix )
{
    m_context->MultiplyTransform((Matrix*) matrix.GetNativeMatrix());
}

// sets the transform of this context
void wxGDIPlusContext::SetTransform( const wxGraphicsMatrix& matrix )
{
    m_context->SetTransform((Matrix*) matrix.GetNativeMatrix());
}

// gets the matrix of this context
wxGraphicsMatrix wxGDIPlusContext::GetTransform() const
{
    wxGraphicsMatrix matrix = CreateMatrix();
    m_context->GetTransform((Matrix*) matrix.GetNativeMatrix());
    return matrix;
}
//-----------------------------------------------------------------------------
// wxGDIPlusRenderer declaration
//-----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxGDIPlusRenderer : public wxGraphicsRenderer
{
public :
    wxGDIPlusRenderer()
    {
        m_loaded = -1;
        m_gditoken = 0;
    }

    virtual ~wxGDIPlusRenderer()
    {
        if ( m_loaded == 1 )
        {
            Unload();
        }
    }

    // Context

    virtual wxGraphicsContext * CreateContext( const wxWindowDC& dc);

    virtual wxGraphicsContext * CreateContext( const wxMemoryDC& dc);

    virtual wxGraphicsContext * CreateContextFromNativeContext( void * context );

    virtual wxGraphicsContext * CreateContextFromNativeWindow( void * window );

    virtual wxGraphicsContext * CreateContext( wxWindow* window );

    virtual wxGraphicsContext * CreateMeasuringContext();

    // Path

    virtual wxGraphicsPath CreatePath();

    // Matrix

    virtual wxGraphicsMatrix CreateMatrix( wxDouble a=1.0, wxDouble b=0.0, wxDouble c=0.0, wxDouble d=1.0,
        wxDouble tx=0.0, wxDouble ty=0.0);


    virtual wxGraphicsPen CreatePen(const wxPen& pen) ;

    virtual wxGraphicsBrush CreateBrush(const wxBrush& brush ) ;

    // sets the brush to a linear gradient, starting at (x1,y1) with color c1 to (x2,y2) with color c2
    virtual wxGraphicsBrush CreateLinearGradientBrush( wxDouble x1, wxDouble y1, wxDouble x2, wxDouble y2,
        const wxColour&c1, const wxColour&c2) ;

    // sets the brush to a radial gradient originating at (xo,yc) with color oColor and ends on a circle around (xc,yc)
    // with radius r and color cColor
    virtual wxGraphicsBrush CreateRadialGradientBrush( wxDouble xo, wxDouble yo, wxDouble xc, wxDouble yc, wxDouble radius,
        const wxColour &oColor, const wxColour &cColor) ;

    // sets the font
    virtual wxGraphicsFont CreateFont( const wxFont &font , const wxColour &col = *wxBLACK ) ;
    
    wxGraphicsBitmap CreateBitmap( const wxBitmap &bmp ) ;
protected :
    bool EnsureIsLoaded();
    void Load();
    void Unload();
    friend class wxGDIPlusRendererModule;

private :
    int m_loaded;
    ULONG_PTR m_gditoken;

    DECLARE_DYNAMIC_CLASS_NO_COPY(wxGDIPlusRenderer)
} ;

//-----------------------------------------------------------------------------
// wxGDIPlusRenderer implementation
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxGDIPlusRenderer,wxGraphicsRenderer)

static wxGDIPlusRenderer gs_GDIPlusRenderer;

wxGraphicsRenderer* wxGraphicsRenderer::GetDefaultRenderer()
{
    return &gs_GDIPlusRenderer;
}

bool wxGDIPlusRenderer::EnsureIsLoaded()
{
    // load gdiplus.dll if not yet loaded, but don't bother doing it again
    // if we already tried and failed (we don't want to spend lot of time
    // returning NULL from wxGraphicsContext::Create(), which may be called
    // relatively frequently):
    if ( m_loaded == -1 )
    {
        Load();
    }

    return m_loaded == 1;
}

// call EnsureIsLoaded() and return returnOnFail value if it fails
#define ENSURE_LOADED_OR_RETURN(returnOnFail)  \
    if ( !EnsureIsLoaded() )                   \
        return (returnOnFail)


void wxGDIPlusRenderer::Load()
{
    GdiplusStartupInput input;
    GdiplusStartupOutput output;
    if ( GdiplusStartup(&m_gditoken,&input,&output) == Gdiplus::Ok )
    {
        wxLogTrace(wxT("gdiplus"), wxT("successfully initialized GDI+"));
        m_loaded = 1;
    }
    else
    {
        wxLogTrace(wxT("gdiplus"), wxT("failed to initialize GDI+, missing gdiplus.dll?"));
        m_loaded = 0;
    }
}

void wxGDIPlusRenderer::Unload()
{
    if ( m_gditoken )
    {
        GdiplusShutdown(m_gditoken);
        m_gditoken = NULL;
    }
    m_loaded = -1; // next Load() will try again
}

wxGraphicsContext * wxGDIPlusRenderer::CreateContext( const wxWindowDC& dc)
{
    ENSURE_LOADED_OR_RETURN(NULL);
    return new wxGDIPlusContext(this,(HDC) dc.GetHDC());
}

wxGraphicsContext * wxGDIPlusRenderer::CreateContext( const wxMemoryDC& dc)
{
    ENSURE_LOADED_OR_RETURN(NULL);
    return new wxGDIPlusContext(this,(HDC) dc.GetHDC());
}

wxGraphicsContext * wxGDIPlusRenderer::CreateMeasuringContext()
{
    ENSURE_LOADED_OR_RETURN(NULL);
    return new wxGDIPlusMeasuringContext(this);
}

wxGraphicsContext * wxGDIPlusRenderer::CreateContextFromNativeContext( void * context )
{
    ENSURE_LOADED_OR_RETURN(NULL);
    return new wxGDIPlusContext(this,(Graphics*) context);
}


wxGraphicsContext * wxGDIPlusRenderer::CreateContextFromNativeWindow( void * window )
{
    ENSURE_LOADED_OR_RETURN(NULL);
    return new wxGDIPlusContext(this,(HWND) window);
}

wxGraphicsContext * wxGDIPlusRenderer::CreateContext( wxWindow* window )
{
    ENSURE_LOADED_OR_RETURN(NULL);
    return new wxGDIPlusContext(this, (HWND) window->GetHWND() );
}

// Path

wxGraphicsPath wxGDIPlusRenderer::CreatePath()
{
    ENSURE_LOADED_OR_RETURN(wxNullGraphicsPath);
    wxGraphicsPath m;
    m.SetRefData( new wxGDIPlusPathData(this));
    return m;
}


// Matrix

wxGraphicsMatrix wxGDIPlusRenderer::CreateMatrix( wxDouble a, wxDouble b, wxDouble c, wxDouble d,
                                                           wxDouble tx, wxDouble ty)

{
    ENSURE_LOADED_OR_RETURN(wxNullGraphicsMatrix);
    wxGraphicsMatrix m;
    wxGDIPlusMatrixData* data = new wxGDIPlusMatrixData( this );
    data->Set( a,b,c,d,tx,ty ) ;
    m.SetRefData(data);
    return m;
}

wxGraphicsPen wxGDIPlusRenderer::CreatePen(const wxPen& pen)
{
    ENSURE_LOADED_OR_RETURN(wxNullGraphicsPen);
    if ( !pen.Ok() || pen.GetStyle() == wxTRANSPARENT )
        return wxNullGraphicsPen;
    else
    {
        wxGraphicsPen p;
        p.SetRefData(new wxGDIPlusPenData( this, pen ));
        return p;
    }
}

wxGraphicsBrush wxGDIPlusRenderer::CreateBrush(const wxBrush& brush )
{
    ENSURE_LOADED_OR_RETURN(wxNullGraphicsBrush);
    if ( !brush.Ok() || brush.GetStyle() == wxTRANSPARENT )
        return wxNullGraphicsBrush;
    else
    {
        wxGraphicsBrush p;
        p.SetRefData(new wxGDIPlusBrushData( this, brush ));
        return p;
    }
}

// sets the brush to a linear gradient, starting at (x1,y1) with color c1 to (x2,y2) with color c2
wxGraphicsBrush wxGDIPlusRenderer::CreateLinearGradientBrush( wxDouble x1, wxDouble y1, wxDouble x2, wxDouble y2,
                                                                      const wxColour&c1, const wxColour&c2)
{
    ENSURE_LOADED_OR_RETURN(wxNullGraphicsBrush);
    wxGraphicsBrush p;
    wxGDIPlusBrushData* d = new wxGDIPlusBrushData( this );
    d->CreateLinearGradientBrush(x1, y1, x2, y2, c1, c2);
    p.SetRefData(d);
    return p;
 }

// sets the brush to a radial gradient originating at (xo,yc) with color oColor and ends on a circle around (xc,yc)
// with radius r and color cColor
wxGraphicsBrush wxGDIPlusRenderer::CreateRadialGradientBrush( wxDouble xo, wxDouble yo, wxDouble xc, wxDouble yc, wxDouble radius,
                                                                      const wxColour &oColor, const wxColour &cColor)
{
    ENSURE_LOADED_OR_RETURN(wxNullGraphicsBrush);
    wxGraphicsBrush p;
    wxGDIPlusBrushData* d = new wxGDIPlusBrushData( this );
    d->CreateRadialGradientBrush(xo,yo,xc,yc,radius,oColor,cColor);
    p.SetRefData(d);
    return p;
}

// sets the font
wxGraphicsFont wxGDIPlusRenderer::CreateFont( const wxFont &font , const wxColour &col )
{
    ENSURE_LOADED_OR_RETURN(wxNullGraphicsFont);
    if ( font.Ok() )
    {
        wxGraphicsFont p;
        p.SetRefData(new wxGDIPlusFontData( this , font, col ));
        return p;
    }
    else
        return wxNullGraphicsFont;
}

wxGraphicsBitmap wxGraphicsRenderer::CreateBitmap( const wxBitmap& bmp )
{
    if ( bmp.Ok() )
    {
        wxGraphicsBitmap p;
        p.SetRefData(new wxGDIPlusBitmapData( this , bmp ));
        return p;
    }
    else
        return wxNullGraphicsBitmap;
}

// Shutdown GDI+ at app exit, before possible dll unload
class wxGDIPlusRendererModule : public wxModule
{
public:
    virtual bool OnInit() { return true; }
    virtual void OnExit() { gs_GDIPlusRenderer.Unload(); }

private:
    DECLARE_DYNAMIC_CLASS(wxGDIPlusRendererModule)
};

IMPLEMENT_DYNAMIC_CLASS(wxGDIPlusRendererModule, wxModule)

#endif  // wxUSE_GRAPHICS_CONTEXT
