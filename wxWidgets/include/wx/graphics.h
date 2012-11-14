/////////////////////////////////////////////////////////////////////////////
// Name:        wx/graphics.h
// Purpose:     graphics context header
// Author:      Stefan Csomor
// Modified by:
// Created:
// Copyright:   (c) Stefan Csomor
// RCS-ID:      $Id: graphics.h 60190 2009-04-16 00:57:35Z KO $
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GRAPHICS_H_
#define _WX_GRAPHICS_H_

#include "wx/defs.h"

#if wxUSE_GRAPHICS_CONTEXT

#include "wx/geometry.h"
#include "wx/dynarray.h"

class WXDLLIMPEXP_CORE wxWindowDC;
class WXDLLIMPEXP_CORE wxMemoryDC;
class WXDLLIMPEXP_CORE wxGraphicsContext;
class WXDLLIMPEXP_CORE wxGraphicsPath;
class WXDLLIMPEXP_CORE wxGraphicsMatrix;
class WXDLLIMPEXP_CORE wxGraphicsFigure;
class WXDLLIMPEXP_CORE wxGraphicsRenderer;
class WXDLLIMPEXP_CORE wxGraphicsPen;
class WXDLLIMPEXP_CORE wxGraphicsBrush;
class WXDLLIMPEXP_CORE wxGraphicsFont;
class WXDLLIMPEXP_CORE wxGraphicsBitmap;

/*
 * notes about the graphics context apis
 *
 * angles : are measured in radians, 0.0 being in direction of positiv x axis, PI/2 being
 * in direction of positive y axis.
 */

// Base class of all objects used for drawing in the new graphics API, the always point back to their
// originating rendering engine, there is no dynamic unloading of a renderer currently allowed,
// these references are not counted

//
// The data used by objects like graphics pens etc is ref counted, in order to avoid unnecessary expensive
// duplication. Any operation on a shared instance that results in a modified state, uncouples this
// instance from the other instances that were shared - using copy on write semantics
//

class WXDLLIMPEXP_CORE wxGraphicsObjectRefData : public wxObjectRefData
{
public :
    wxGraphicsObjectRefData( wxGraphicsRenderer* renderer );
    wxGraphicsObjectRefData( const wxGraphicsObjectRefData* data );
    wxGraphicsRenderer* GetRenderer() const ;
    virtual wxGraphicsObjectRefData* Clone() const ;

protected :
    wxGraphicsRenderer* m_renderer;
} ;

class WXDLLIMPEXP_CORE wxGraphicsObject : public wxObject
{
public :
    wxGraphicsObject() ;
#if wxABI_VERSION >= 20810
    wxGraphicsObject( const wxGraphicsObject& other) : wxObject( other ) {}
    wxGraphicsObject& operator= (const wxGraphicsObject & other) { Ref(other); return *this;}
#endif
    wxGraphicsObject( wxGraphicsRenderer* renderer ) ;
    virtual ~wxGraphicsObject() ;

    bool IsNull() const ;

    // returns the renderer that was used to create this instance, or NULL if it has not been initialized yet
    wxGraphicsRenderer* GetRenderer() const ;
    wxGraphicsObjectRefData* GetGraphicsData() const ;
protected :
    virtual wxObjectRefData* CreateRefData() const;
    virtual wxObjectRefData* CloneRefData(const wxObjectRefData* data) const;

    DECLARE_DYNAMIC_CLASS(wxGraphicsObject)
} ;

class WXDLLIMPEXP_CORE wxGraphicsPen : public wxGraphicsObject
{
public :
    wxGraphicsPen() {}
#if wxABI_VERSION >= 20810
    wxGraphicsPen( const wxGraphicsPen& other) : wxGraphicsObject( other ) {}
    wxGraphicsPen& operator= (const wxGraphicsPen & other) { Ref(other); return *this;}
#endif
    virtual ~wxGraphicsPen() {}
private :
    DECLARE_DYNAMIC_CLASS(wxGraphicsPen)
} ;

extern WXDLLEXPORT_DATA(wxGraphicsPen) wxNullGraphicsPen;

class WXDLLIMPEXP_CORE wxGraphicsBrush : public wxGraphicsObject
{
public :
    wxGraphicsBrush() {}
#if wxABI_VERSION >= 20810
    wxGraphicsBrush( const wxGraphicsBrush& other) : wxGraphicsObject( other ) {}
    wxGraphicsBrush& operator= (const wxGraphicsBrush & other) { Ref(other); return *this;}
#endif
    virtual ~wxGraphicsBrush() {}
private :
    DECLARE_DYNAMIC_CLASS(wxGraphicsBrush)
} ;

extern WXDLLEXPORT_DATA(wxGraphicsBrush) wxNullGraphicsBrush;

class WXDLLIMPEXP_CORE wxGraphicsFont : public wxGraphicsObject
{
public :
    wxGraphicsFont() {}
#if wxABI_VERSION >= 20810
    wxGraphicsFont( const wxGraphicsFont& other) : wxGraphicsObject( other ) {}
    wxGraphicsFont& operator= (const wxGraphicsFont & other) { Ref(other); return *this;}
#endif
    virtual ~wxGraphicsFont() {}
private :
    DECLARE_DYNAMIC_CLASS(wxGraphicsFont)
} ;

extern WXDLLEXPORT_DATA(wxGraphicsFont) wxNullGraphicsFont;

class WXDLLIMPEXP_CORE wxGraphicsBitmap : public wxGraphicsObject
{
public :
    wxGraphicsBitmap() {}
#if wxABI_VERSION >= 20810
    wxGraphicsBitmap( const wxGraphicsBitmap& other) : wxGraphicsObject( other ) {}
    wxGraphicsBitmap& operator= (const wxGraphicsBitmap & other) { Ref(other); return *this;}
#endif
    virtual ~wxGraphicsBitmap() {}
private :
    DECLARE_DYNAMIC_CLASS(wxGraphicsBitmap)
} ;

extern WXDLLEXPORT_DATA(wxGraphicsBitmap) wxNullGraphicsBitmap;

class WXDLLIMPEXP_CORE wxGraphicsMatrixData : public wxGraphicsObjectRefData
{
public :
    wxGraphicsMatrixData( wxGraphicsRenderer* renderer) :
       wxGraphicsObjectRefData(renderer) {}

       virtual ~wxGraphicsMatrixData() {}

       // concatenates the matrix
       virtual void Concat( const wxGraphicsMatrixData *t ) = 0;

       // sets the matrix to the respective values
       virtual void Set(wxDouble a=1.0, wxDouble b=0.0, wxDouble c=0.0, wxDouble d=1.0,
           wxDouble tx=0.0, wxDouble ty=0.0) = 0;

       // gets the component valuess of the matrix
       virtual void Get(wxDouble* a=NULL, wxDouble* b=NULL,  wxDouble* c=NULL,
                        wxDouble* d=NULL, wxDouble* tx=NULL, wxDouble* ty=NULL) const = 0;

       // makes this the inverse matrix
       virtual void Invert() = 0;

       // returns true if the elements of the transformation matrix are equal ?
       virtual bool IsEqual( const wxGraphicsMatrixData* t) const  = 0;

       // return true if this is the identity matrix
       virtual bool IsIdentity() const = 0;

       //
       // transformation
       //

       // add the translation to this matrix
       virtual void Translate( wxDouble dx , wxDouble dy ) = 0;

       // add the scale to this matrix
       virtual void Scale( wxDouble xScale , wxDouble yScale ) = 0;

       // add the rotation to this matrix (radians)
       virtual void Rotate( wxDouble angle ) = 0;

       //
       // apply the transforms
       //

       // applies that matrix to the point
       virtual void TransformPoint( wxDouble *x, wxDouble *y ) const = 0;

       // applies the matrix except for translations
       virtual void TransformDistance( wxDouble *dx, wxDouble *dy ) const =0;

       // returns the native representation
       virtual void * GetNativeMatrix() const = 0;
} ;

class WXDLLIMPEXP_CORE wxGraphicsMatrix : public wxGraphicsObject
{
public :
    wxGraphicsMatrix() {}
#if wxABI_VERSION >= 20810
    wxGraphicsMatrix( const wxGraphicsMatrix& other) : wxGraphicsObject( other ) {}
    wxGraphicsMatrix& operator= (const wxGraphicsMatrix & other) { Ref(other); return *this;}
#endif

    virtual ~wxGraphicsMatrix() {}

    // concatenates the matrix
    virtual void Concat( const wxGraphicsMatrix *t );
    void Concat( const wxGraphicsMatrix &t ) { Concat( &t ); }

    // sets the matrix to the respective values
    virtual void Set(wxDouble a=1.0, wxDouble b=0.0, wxDouble c=0.0, wxDouble d=1.0,
        wxDouble tx=0.0, wxDouble ty=0.0);

    // gets the component valuess of the matrix
    virtual void Get(wxDouble* a=NULL, wxDouble* b=NULL,  wxDouble* c=NULL,
                     wxDouble* d=NULL, wxDouble* tx=NULL, wxDouble* ty=NULL) const;

    // makes this the inverse matrix
    virtual void Invert();

    // returns true if the elements of the transformation matrix are equal ?
    virtual bool IsEqual( const wxGraphicsMatrix* t) const;
    bool IsEqual( const wxGraphicsMatrix& t) const { return IsEqual( &t ); }

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

    const wxGraphicsMatrixData* GetMatrixData() const
    { return (const wxGraphicsMatrixData*) GetRefData(); }
    wxGraphicsMatrixData* GetMatrixData()
    { return (wxGraphicsMatrixData*) GetRefData(); }

private :
    DECLARE_DYNAMIC_CLASS(wxGraphicsMatrix)
} ;

extern WXDLLEXPORT_DATA(wxGraphicsMatrix) wxNullGraphicsMatrix;

class WXDLLIMPEXP_CORE wxGraphicsPathData : public wxGraphicsObjectRefData
{
public :
    wxGraphicsPathData(wxGraphicsRenderer* renderer) : wxGraphicsObjectRefData(renderer) {}
    virtual ~wxGraphicsPathData() {}

    //
    // These are the path primitives from which everything else can be constructed
    //

    // begins a new subpath at (x,y)
    virtual void MoveToPoint( wxDouble x, wxDouble y ) = 0;

    // adds a straight line from the current point to (x,y)
    virtual void AddLineToPoint( wxDouble x, wxDouble y ) = 0;

    // adds a cubic Bezier curve from the current point, using two control points and an end point
    virtual void AddCurveToPoint( wxDouble cx1, wxDouble cy1, wxDouble cx2, wxDouble cy2, wxDouble x, wxDouble y ) = 0;

    // adds another path
    virtual void AddPath( const wxGraphicsPathData* path ) =0;

    // closes the current sub-path
    virtual void CloseSubpath() = 0;

    // gets the last point of the current path, (0,0) if not yet set
    virtual void GetCurrentPoint( wxDouble* x, wxDouble* y) const = 0;

    // adds an arc of a circle centering at (x,y) with radius (r) from startAngle to endAngle
    virtual void AddArc( wxDouble x, wxDouble y, wxDouble r, wxDouble startAngle, wxDouble endAngle, bool clockwise ) = 0;

    //
    // These are convenience functions which - if not available natively will be assembled
    // using the primitives from above
    //

    // adds a quadratic Bezier curve from the current point, using a control point and an end point
    virtual void AddQuadCurveToPoint( wxDouble cx, wxDouble cy, wxDouble x, wxDouble y );

    // appends a rectangle as a new closed subpath
    virtual void AddRectangle( wxDouble x, wxDouble y, wxDouble w, wxDouble h );

    // appends an ellipsis as a new closed subpath fitting the passed rectangle
    virtual void AddCircle( wxDouble x, wxDouble y, wxDouble r );

    // appends a an arc to two tangents connecting (current) to (x1,y1) and (x1,y1) to (x2,y2), also a straight line from (current) to (x1,y1)
    virtual void AddArcToPoint( wxDouble x1, wxDouble y1 , wxDouble x2, wxDouble y2, wxDouble r ) ;

    // appends an ellipse
    virtual void AddEllipse( wxDouble x, wxDouble y, wxDouble w, wxDouble h);

    // appends a rounded rectangle
    virtual void AddRoundedRectangle( wxDouble x, wxDouble y, wxDouble w, wxDouble h, wxDouble radius);

    // returns the native path
    virtual void * GetNativePath() const = 0;

    // give the native path returned by GetNativePath() back (there might be some deallocations necessary)
    virtual void UnGetNativePath(void *p) const= 0;

    // transforms each point of this path by the matrix
    virtual void Transform( const wxGraphicsMatrixData* matrix ) =0;

    // gets the bounding box enclosing all points (possibly including control points)
    virtual void GetBox(wxDouble *x, wxDouble *y, wxDouble *w, wxDouble *h) const=0;

    virtual bool Contains( wxDouble x, wxDouble y, int fillStyle = wxODDEVEN_RULE) const=0;
};

class WXDLLIMPEXP_CORE wxGraphicsPath : public wxGraphicsObject
{
public :
    wxGraphicsPath()  {}
#if wxABI_VERSION >= 20810
    wxGraphicsPath( const wxGraphicsPath& other) : wxGraphicsObject( other ) {}
    wxGraphicsPath& operator= (const wxGraphicsPath & other) { Ref(other); return *this;}
#endif
    virtual ~wxGraphicsPath() {}

    //
    // These are the path primitives from which everything else can be constructed
    //

    // begins a new subpath at (x,y)
    virtual void MoveToPoint( wxDouble x, wxDouble y );
    void MoveToPoint( const wxPoint2DDouble& p);

    // adds a straight line from the current point to (x,y)
    virtual void AddLineToPoint( wxDouble x, wxDouble y );
    void AddLineToPoint( const wxPoint2DDouble& p);

    // adds a cubic Bezier curve from the current point, using two control points and an end point
    virtual void AddCurveToPoint( wxDouble cx1, wxDouble cy1, wxDouble cx2, wxDouble cy2, wxDouble x, wxDouble y ) ;
    void AddCurveToPoint( const wxPoint2DDouble& c1, const wxPoint2DDouble& c2, const wxPoint2DDouble& e);

    // adds another path
    virtual void AddPath( const wxGraphicsPath& path );

    // closes the current sub-path
    virtual void CloseSubpath() ;

    // gets the last point of the current path, (0,0) if not yet set
    virtual void GetCurrentPoint( wxDouble* x, wxDouble* y) const;
    wxPoint2DDouble GetCurrentPoint() const;

    // adds an arc of a circle centering at (x,y) with radius (r) from startAngle to endAngle
    virtual void AddArc( wxDouble x, wxDouble y, wxDouble r, wxDouble startAngle, wxDouble endAngle, bool clockwise ) ;
    void AddArc( const wxPoint2DDouble& c, wxDouble r, wxDouble startAngle, wxDouble endAngle, bool clockwise);

    //
    // These are convenience functions which - if not available natively will be assembled
    // using the primitives from above
    //

    // adds a quadratic Bezier curve from the current point, using a control point and an end point
    virtual void AddQuadCurveToPoint( wxDouble cx, wxDouble cy, wxDouble x, wxDouble y );

    // appends a rectangle as a new closed subpath
    virtual void AddRectangle( wxDouble x, wxDouble y, wxDouble w, wxDouble h );

    // appends an ellipsis as a new closed subpath fitting the passed rectangle
    virtual void AddCircle( wxDouble x, wxDouble y, wxDouble r );

    // appends a an arc to two tangents connecting (current) to (x1,y1) and (x1,y1) to (x2,y2), also a straight line from (current) to (x1,y1)
    virtual void AddArcToPoint( wxDouble x1, wxDouble y1 , wxDouble x2, wxDouble y2, wxDouble r ) ;

    // appends an ellipse
    virtual void AddEllipse( wxDouble x, wxDouble y, wxDouble w, wxDouble h);

    // appends a rounded rectangle
    virtual void AddRoundedRectangle( wxDouble x, wxDouble y, wxDouble w, wxDouble h, wxDouble radius);

    // returns the native path
    virtual void * GetNativePath() const;

    // give the native path returned by GetNativePath() back (there might be some deallocations necessary)
    virtual void UnGetNativePath(void *p)const;

    // transforms each point of this path by the matrix
    virtual void Transform( const wxGraphicsMatrix& matrix );

    // gets the bounding box enclosing all points (possibly including control points)
    virtual void GetBox(wxDouble *x, wxDouble *y, wxDouble *w, wxDouble *h)const;
    wxRect2DDouble GetBox()const;

    virtual bool Contains( wxDouble x, wxDouble y, int fillStyle = wxODDEVEN_RULE)const;
    bool Contains( const wxPoint2DDouble& c, int fillStyle = wxODDEVEN_RULE)const;

    const wxGraphicsPathData* GetPathData() const
    { return (const wxGraphicsPathData*) GetRefData(); }
    wxGraphicsPathData* GetPathData()
    { return (wxGraphicsPathData*) GetRefData(); }

private :
    DECLARE_DYNAMIC_CLASS(wxGraphicsPath)
} ;

extern WXDLLEXPORT_DATA(wxGraphicsPath) wxNullGraphicsPath;


class WXDLLIMPEXP_CORE wxGraphicsContext : public wxGraphicsObject
{
public:
    wxGraphicsContext(wxGraphicsRenderer* renderer);

    virtual ~wxGraphicsContext();

    static wxGraphicsContext* Create( const wxWindowDC& dc) ;

#ifdef __WXMSW__
    static wxGraphicsContext * Create( const wxMemoryDC& dc) ;
#endif

    static wxGraphicsContext* CreateFromNative( void * context ) ;

    static wxGraphicsContext* CreateFromNativeWindow( void * window ) ;

    static wxGraphicsContext* Create( wxWindow* window ) ;

    // create a context that can be used for measuring texts only, no drawing allowed
    static wxGraphicsContext * Create();

    wxGraphicsPath CreatePath() const;

    virtual wxGraphicsPen CreatePen(const wxPen& pen) const;

    virtual wxGraphicsBrush CreateBrush(const wxBrush& brush ) const;

    // sets the brush to a linear gradient, starting at (x1,y1) with color c1 to (x2,y2) with color c2
    virtual wxGraphicsBrush CreateLinearGradientBrush( wxDouble x1, wxDouble y1, wxDouble x2, wxDouble y2,
        const wxColour&c1, const wxColour&c2) const;

    // sets the brush to a radial gradient originating at (xo,yc) with color oColor and ends on a circle around (xc,yc)
    // with radius r and color cColor
    virtual wxGraphicsBrush CreateRadialGradientBrush( wxDouble xo, wxDouble yo, wxDouble xc, wxDouble yc, wxDouble radius,
        const wxColour &oColor, const wxColour &cColor) const;

    // sets the font
    virtual wxGraphicsFont CreateFont( const wxFont &font , const wxColour &col = *wxBLACK ) const;
    
#if wxABI_VERSION >= 20809
    wxGraphicsBitmap CreateBitmap( const wxBitmap &bitmap ) const;
#endif

    //virtual wxGraphicsBitmap CreateSubBitmap( const wxGraphicsBitmap &bitmap, wxDouble x, wxDouble y, wxDouble w, wxDouble h  ) const;

    // create a 'native' matrix corresponding to these values
    virtual wxGraphicsMatrix CreateMatrix( wxDouble a=1.0, wxDouble b=0.0, wxDouble c=0.0, wxDouble d=1.0,
        wxDouble tx=0.0, wxDouble ty=0.0) const;

    // push the current state of the context, ie the transformation matrix on a stack
    virtual void PushState() = 0;

    // pops a stored state from the stack
    virtual void PopState() = 0;

    // clips drawings to the region intersected with the current clipping region
    virtual void Clip( const wxRegion &region ) = 0;

    // clips drawings to the rect intersected with the current clipping region
    virtual void Clip( wxDouble x, wxDouble y, wxDouble w, wxDouble h ) = 0;

    // resets the clipping to original extent
    virtual void ResetClip() = 0 ;

    // returns the native context
    virtual void * GetNativeContext() = 0;

    // returns the current logical function
    virtual int GetLogicalFunction() const { return m_logicalFunction; }

    // sets the current logical function, returns true if it supported
    virtual bool SetLogicalFunction(int function) ;

    //
    // transformation : changes the current transformation matrix CTM of the context
    //

    // translate
    virtual void Translate( wxDouble dx , wxDouble dy ) = 0;

    // scale
    virtual void Scale( wxDouble xScale , wxDouble yScale ) = 0;

    // rotate (radians)
    virtual void Rotate( wxDouble angle ) = 0;

    // concatenates this transform with the current transform of this context
    virtual void ConcatTransform( const wxGraphicsMatrix& matrix ) = 0;

    // sets the transform of this context
    virtual void SetTransform( const wxGraphicsMatrix& matrix ) = 0;

    // gets the matrix of this context
    virtual wxGraphicsMatrix GetTransform() const = 0;
    //
    // setting the paint
    //

    // sets the pen
    virtual void SetPen( const wxGraphicsPen& pen );

    void SetPen( const wxPen& pen );

    // sets the brush for filling
    virtual void SetBrush( const wxGraphicsBrush& brush );

    void SetBrush( const wxBrush& brush );

    // sets the font
    virtual void SetFont( const wxGraphicsFont& font );

    void SetFont( const wxFont& font, const wxColour& colour );


    // strokes along a path with the current pen
    virtual void StrokePath( const wxGraphicsPath& path ) = 0;

    // fills a path with the current brush
    virtual void FillPath( const wxGraphicsPath& path, int fillStyle = wxODDEVEN_RULE ) = 0;

    // draws a path by first filling and then stroking
    virtual void DrawPath( const wxGraphicsPath& path, int fillStyle = wxODDEVEN_RULE );

    //
    // text
    //

    virtual void DrawText( const wxString &str, wxDouble x, wxDouble y ) = 0;

    virtual void DrawText( const wxString &str, wxDouble x, wxDouble y, wxDouble angle );

    virtual void DrawText( const wxString &str, wxDouble x, wxDouble y, const wxGraphicsBrush& backgroundBrush ) ;

    virtual void DrawText( const wxString &str, wxDouble x, wxDouble y, wxDouble angle, const wxGraphicsBrush& backgroundBrush );

    virtual void GetTextExtent( const wxString &text, wxDouble *width, wxDouble *height,
        wxDouble *descent, wxDouble *externalLeading ) const  = 0;

    virtual void GetPartialTextExtents(const wxString& text, wxArrayDouble& widths) const = 0;

    //
    // image support
    //
#if wxABI_VERSION >= 20809
    void DrawGraphicsBitmap( const wxGraphicsBitmap &bmp, wxDouble x, wxDouble y, wxDouble w, wxDouble h );
#endif
    
    virtual void DrawBitmap( const wxBitmap &bmp, wxDouble x, wxDouble y, wxDouble w, wxDouble h ) = 0;

    virtual void DrawIcon( const wxIcon &icon, wxDouble x, wxDouble y, wxDouble w, wxDouble h ) = 0;

    //
    // convenience methods
    //

    // strokes a single line
    virtual void StrokeLine( wxDouble x1, wxDouble y1, wxDouble x2, wxDouble y2);

    // stroke lines connecting each of the points
    virtual void StrokeLines( size_t n, const wxPoint2DDouble *points);

    // stroke disconnected lines from begin to end points
    virtual void StrokeLines( size_t n, const wxPoint2DDouble *beginPoints, const wxPoint2DDouble *endPoints);

    // draws a polygon
    virtual void DrawLines( size_t n, const wxPoint2DDouble *points, int fillStyle = wxODDEVEN_RULE );

    // draws a polygon
    virtual void DrawRectangle( wxDouble x, wxDouble y, wxDouble w, wxDouble h);

    // draws an ellipse
    virtual void DrawEllipse( wxDouble x, wxDouble y, wxDouble w, wxDouble h);

    // draws a rounded rectangle
    virtual void DrawRoundedRectangle( wxDouble x, wxDouble y, wxDouble w, wxDouble h, wxDouble radius);

     // wrappers using wxPoint2DDouble TODO

    // helper to determine if a 0.5 offset should be applied for the drawing operation
    virtual bool ShouldOffset() const { return false; }

protected :

    wxGraphicsPen m_pen;
    wxGraphicsBrush m_brush;
    wxGraphicsFont m_font;
    int m_logicalFunction;

private :
    DECLARE_NO_COPY_CLASS(wxGraphicsContext)
    DECLARE_ABSTRACT_CLASS(wxGraphicsContext)
};

#if 0

//
// A graphics figure allows to cache path, pen etc creations, also will be a basis for layering/grouping elements
//

class WXDLLIMPEXP_CORE wxGraphicsFigure : public wxGraphicsObject
{
public :
    wxGraphicsFigure(wxGraphicsRenderer* renderer) ;
#if wxABI_VERSION >= 20810
    wxGraphicsFigure( const wxGraphicsFigure& other) : wxGraphicsObject( other ) {}
    wxGraphicsFigure& operator= (const wxGraphicsFigure & other) { Ref(other); return *this;}
#endif

    virtual ~wxGraphicsFigure() ;

    void SetPath( wxGraphicsMatrix* matrix );

    void SetMatrix( wxGraphicsPath* path);

    // draws this object on the context
    virtual void Draw( wxGraphicsContext* cg );

    // returns the path of this object
    wxGraphicsPath* GetPath() { return m_path; }

    // returns the transformation matrix of this object, may be null if there is no transformation necessary
    wxGraphicsMatrix* GetMatrix() { return m_matrix; }

private :
    wxGraphicsMatrix* m_matrix;
    wxGraphicsPath* m_path;

    DECLARE_DYNAMIC_CLASS(wxGraphicsFigure)
} ;

#endif

//
// The graphics renderer is the instance corresponding to the rendering engine used, eg there is ONE core graphics renderer
// instance on OSX. This instance is pointed back to by all objects created by it. Therefore you can create eg additional
// paths at any point from a given matrix etc.
//

class WXDLLIMPEXP_CORE wxGraphicsRenderer : public wxObject
{
public :
    wxGraphicsRenderer() {}

    virtual ~wxGraphicsRenderer() {}

    static wxGraphicsRenderer* GetDefaultRenderer();

#if wxABI_VERSION >= 20811
    static wxGraphicsRenderer* GetCairoRenderer();
#endif
    // Context

    virtual wxGraphicsContext * CreateContext( const wxWindowDC& dc) = 0 ;
#ifdef __WXMSW__
    virtual wxGraphicsContext * CreateContext( const wxMemoryDC& dc) = 0 ;
#endif
    virtual wxGraphicsContext * CreateContextFromNativeContext( void * context ) = 0;

    virtual wxGraphicsContext * CreateContextFromNativeWindow( void * window ) = 0;

    virtual wxGraphicsContext * CreateContext( wxWindow* window ) = 0;

    // create a context that can be used for measuring texts only, no drawing allowed
    virtual wxGraphicsContext * CreateMeasuringContext() = 0;

    // Path

    virtual wxGraphicsPath CreatePath() = 0;

    // Matrix

    virtual wxGraphicsMatrix CreateMatrix( wxDouble a=1.0, wxDouble b=0.0, wxDouble c=0.0, wxDouble d=1.0,
        wxDouble tx=0.0, wxDouble ty=0.0) = 0;

    // Paints

    virtual wxGraphicsPen CreatePen(const wxPen& pen) = 0 ;

    virtual wxGraphicsBrush CreateBrush(const wxBrush& brush ) = 0 ;

    // sets the brush to a linear gradient, starting at (x1,y1) with color c1 to (x2,y2) with color c2
    virtual wxGraphicsBrush CreateLinearGradientBrush( wxDouble x1, wxDouble y1, wxDouble x2, wxDouble y2,
        const wxColour&c1, const wxColour&c2) = 0;

    // sets the brush to a radial gradient originating at (xo,yc) with color oColor and ends on a circle around (xc,yc)
    // with radius r and color cColor
    virtual wxGraphicsBrush CreateRadialGradientBrush( wxDouble xo, wxDouble yo, wxDouble xc, wxDouble yc, wxDouble radius,
        const wxColour &oColor, const wxColour &cColor) = 0;

   // sets the font
    virtual wxGraphicsFont CreateFont( const wxFont &font , const wxColour &col = *wxBLACK ) = 0;

#if wxABI_VERSION >= 20809
    wxGraphicsBitmap CreateBitmap( const wxBitmap &bmp );
#endif

private :
    DECLARE_NO_COPY_CLASS(wxGraphicsRenderer)
    DECLARE_ABSTRACT_CLASS(wxGraphicsRenderer)
} ;

#endif

#endif // _WX_GRAPHICS_H_
