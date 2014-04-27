/////////////////////////////////////////////////////////////////////////////
// Name:        wx/geometry.h
// Purpose:     Common Geometry Classes
// Author:      Stefan Csomor
// Modified by:
// Created:     08/05/99
// RCS-ID:      $Id: geometry.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) 1999 Stefan Csomor
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GEOMETRY_H_
#define _WX_GEOMETRY_H_

#include "wx/defs.h"

#if wxUSE_GEOMETRY

#include "wx/utils.h"
#include "wx/gdicmn.h"
#include "wx/math.h"

class WXDLLIMPEXP_FWD_BASE wxDataInputStream;
class WXDLLIMPEXP_FWD_BASE wxDataOutputStream;

// clipping from Cohen-Sutherland

enum wxOutCode
{
    wxInside = 0x00 ,
    wxOutLeft = 0x01 ,
    wxOutRight = 0x02 ,
    wxOutTop = 0x08 ,
    wxOutBottom = 0x04
};

class WXDLLEXPORT wxPoint2DInt
{
public :
    inline wxPoint2DInt();
    inline wxPoint2DInt( wxInt32 x , wxInt32 y );
    inline wxPoint2DInt( const wxPoint2DInt &pt );
    inline wxPoint2DInt( const wxPoint &pt );

    // noops for this class, just return the coords
    inline void GetFloor( wxInt32 *x , wxInt32 *y ) const;
    inline void GetRounded( wxInt32 *x , wxInt32 *y ) const;

    inline wxDouble GetVectorLength() const;
           wxDouble GetVectorAngle() const;
    inline void SetVectorLength( wxDouble length );
           void SetVectorAngle( wxDouble degrees );
           void SetPolarCoordinates( wxInt32 angle , wxInt32 length );
    // set the vector length to 1.0, preserving the angle
    inline void Normalize();

    inline wxDouble GetDistance( const wxPoint2DInt &pt ) const;
    inline wxDouble GetDistanceSquare( const wxPoint2DInt &pt ) const;
    inline wxInt32 GetDotProduct( const wxPoint2DInt &vec ) const;
    inline wxInt32 GetCrossProduct( const wxPoint2DInt &vec ) const;

    // the reflection of this point
    inline wxPoint2DInt operator-();

    inline wxPoint2DInt& operator=(const wxPoint2DInt& pt);
    inline wxPoint2DInt& operator+=(const wxPoint2DInt& pt);
    inline wxPoint2DInt& operator-=(const wxPoint2DInt& pt);
    inline wxPoint2DInt& operator*=(const wxPoint2DInt& pt);
    inline wxPoint2DInt& operator*=(wxDouble n);
    inline wxPoint2DInt& operator*=(wxInt32 n);
    inline wxPoint2DInt& operator/=(const wxPoint2DInt& pt);
    inline wxPoint2DInt& operator/=(wxDouble n);
    inline wxPoint2DInt& operator/=(wxInt32 n);
    inline operator wxPoint() const;
    inline bool operator==(const wxPoint2DInt& pt) const;
    inline bool operator!=(const wxPoint2DInt& pt) const;

#if wxUSE_STREAMS
    void WriteTo( wxDataOutputStream &stream ) const;
    void ReadFrom( wxDataInputStream &stream );
#endif // wxUSE_STREAMS

    wxInt32 m_x;
    wxInt32 m_y;
};

inline wxPoint2DInt operator+(const wxPoint2DInt& pt1 , const wxPoint2DInt& pt2);
inline wxPoint2DInt operator-(const wxPoint2DInt& pt1 , const wxPoint2DInt& pt2);
inline wxPoint2DInt operator*(const wxPoint2DInt& pt1 , const wxPoint2DInt& pt2);
inline wxPoint2DInt operator*(wxInt32 n , const wxPoint2DInt& pt);
inline wxPoint2DInt operator*(wxInt32 n , const wxPoint2DInt& pt);
inline wxPoint2DInt operator*(const wxPoint2DInt& pt , wxInt32 n);
inline wxPoint2DInt operator*(const wxPoint2DInt& pt , wxInt32 n);
inline wxPoint2DInt operator/(const wxPoint2DInt& pt1 , const wxPoint2DInt& pt2);
inline wxPoint2DInt operator/(const wxPoint2DInt& pt , wxInt32 n);
inline wxPoint2DInt operator/(const wxPoint2DInt& pt , wxInt32 n);

inline wxPoint2DInt::wxPoint2DInt()
{
    m_x = 0;
    m_y = 0;
}

inline wxPoint2DInt::wxPoint2DInt( wxInt32 x , wxInt32 y )
{
    m_x = x;
    m_y = y;
}

inline wxPoint2DInt::wxPoint2DInt( const wxPoint2DInt &pt )
{
    m_x = pt.m_x;
    m_y = pt.m_y;
}

inline wxPoint2DInt::wxPoint2DInt( const wxPoint &pt )
{
    m_x = pt.x;
    m_y = pt.y;
}

inline void wxPoint2DInt::GetFloor( wxInt32 *x , wxInt32 *y ) const
{
    if ( x )
        *x = m_x;
    if ( y )
        *y = m_y;
}

inline void wxPoint2DInt::GetRounded( wxInt32 *x , wxInt32 *y ) const
{
    GetFloor(x, y);
}

inline wxDouble wxPoint2DInt::GetVectorLength() const
{
    // cast needed MIPSpro compiler under SGI
    return sqrt( (double)(m_x)*(m_x) + (m_y)*(m_y) );
}

inline void wxPoint2DInt::SetVectorLength( wxDouble length )
{
    wxDouble before = GetVectorLength();
    m_x = (wxInt32)(m_x * length / before);
    m_y = (wxInt32)(m_y * length / before);
}

inline void wxPoint2DInt::Normalize()
{
    SetVectorLength( 1 );
}

inline wxDouble wxPoint2DInt::GetDistance( const wxPoint2DInt &pt ) const
{
    return sqrt( GetDistanceSquare( pt ) );
}

inline wxDouble wxPoint2DInt::GetDistanceSquare( const wxPoint2DInt &pt ) const
{
    return ( (wxDouble)(pt.m_x-m_x)*(pt.m_x-m_x) + (wxDouble)(pt.m_y-m_y)*(pt.m_y-m_y) );
}

inline wxInt32 wxPoint2DInt::GetDotProduct( const wxPoint2DInt &vec ) const
{
    return ( m_x * vec.m_x + m_y * vec.m_y );
}

inline wxInt32 wxPoint2DInt::GetCrossProduct( const wxPoint2DInt &vec ) const
{
    return ( m_x * vec.m_y - vec.m_x * m_y );
}

inline wxPoint2DInt::operator wxPoint() const
{
    return wxPoint( m_x, m_y);
}

inline wxPoint2DInt wxPoint2DInt::operator-()
{
    return wxPoint2DInt( -m_x, -m_y);
}

inline wxPoint2DInt& wxPoint2DInt::operator=(const wxPoint2DInt& pt)
{
    m_x = pt.m_x;
    m_y = pt.m_y;
    return *this;
}

inline wxPoint2DInt& wxPoint2DInt::operator+=(const wxPoint2DInt& pt)
{
    m_x = m_x + pt.m_x;
    m_y = m_y + pt.m_y;
    return *this;
}

inline wxPoint2DInt& wxPoint2DInt::operator-=(const wxPoint2DInt& pt)
{
    m_x = m_x - pt.m_x;
    m_y = m_y - pt.m_y;
    return *this;
}

inline wxPoint2DInt& wxPoint2DInt::operator*=(const wxPoint2DInt& pt)
{
    m_x = m_x + pt.m_x;
    m_y = m_y + pt.m_y;
    return *this;
}

inline wxPoint2DInt& wxPoint2DInt::operator/=(const wxPoint2DInt& pt)
{
    m_x = m_x - pt.m_x;
    m_y = m_y - pt.m_y;
    return *this;
}

inline bool wxPoint2DInt::operator==(const wxPoint2DInt& pt) const
{
    return m_x == pt.m_x && m_y == pt.m_y;
}

inline bool wxPoint2DInt::operator!=(const wxPoint2DInt& pt) const
{
    return m_x != pt.m_x || m_y != pt.m_y;
}

inline wxPoint2DInt operator+(const wxPoint2DInt& pt1 , const wxPoint2DInt& pt2)
{
    return wxPoint2DInt( pt1.m_x + pt2.m_x , pt1.m_y + pt2.m_y );
}

inline wxPoint2DInt operator-(const wxPoint2DInt& pt1 , const wxPoint2DInt& pt2)
{
    return wxPoint2DInt( pt1.m_x - pt2.m_x , pt1.m_y - pt2.m_y );
}


inline wxPoint2DInt operator*(const wxPoint2DInt& pt1 , const wxPoint2DInt& pt2)
{
    return wxPoint2DInt( pt1.m_x * pt2.m_x , pt1.m_y * pt2.m_y );
}

inline wxPoint2DInt operator*(wxInt32 n , const wxPoint2DInt& pt)
{
    return wxPoint2DInt( pt.m_x * n , pt.m_y * n );
}

inline wxPoint2DInt operator*(wxDouble n , const wxPoint2DInt& pt)
{
    return wxPoint2DInt( (int) (pt.m_x * n) , (int) (pt.m_y * n) );
}

inline wxPoint2DInt operator*(const wxPoint2DInt& pt , wxInt32 n)
{
    return wxPoint2DInt( pt.m_x * n , pt.m_y * n );
}

inline wxPoint2DInt operator*(const wxPoint2DInt& pt , wxDouble n)
{
    return wxPoint2DInt( (int) (pt.m_x * n) , (int) (pt.m_y * n) );
}

inline wxPoint2DInt operator/(const wxPoint2DInt& pt1 , const wxPoint2DInt& pt2)
{
    return wxPoint2DInt( pt1.m_x / pt2.m_x , pt1.m_y / pt2.m_y );
}

inline wxPoint2DInt operator/(const wxPoint2DInt& pt , wxInt32 n)
{
    return wxPoint2DInt( pt.m_x / n , pt.m_y / n );
}

inline wxPoint2DInt operator/(const wxPoint2DInt& pt , wxDouble n)
{
    return wxPoint2DInt( (int) (pt.m_x / n) , (int) (pt.m_y / n) );
}

// wxPoint2Ds represent a point or a vector in a 2d coordinate system

class WXDLLEXPORT wxPoint2DDouble
{
public :
    inline wxPoint2DDouble();
    inline wxPoint2DDouble( wxDouble x , wxDouble y );
    inline wxPoint2DDouble( const wxPoint2DDouble &pt );
    wxPoint2DDouble( const wxPoint2DInt &pt )
        { m_x = (wxDouble) pt.m_x ; m_y = (wxDouble) pt.m_y ; }
    wxPoint2DDouble( const wxPoint &pt )
        { m_x = (wxDouble) pt.x ; m_y = (wxDouble) pt.y ; }

    // two different conversions to integers, floor and rounding
    inline void GetFloor( wxInt32 *x , wxInt32 *y ) const;
    inline void GetRounded( wxInt32 *x , wxInt32 *y ) const;

    inline wxDouble GetVectorLength() const;
     wxDouble GetVectorAngle() const ;
    void SetVectorLength( wxDouble length );
    void SetVectorAngle( wxDouble degrees );
    void SetPolarCoordinates( wxDouble angle , wxDouble length );
    // set the vector length to 1.0, preserving the angle
    void Normalize();

    inline wxDouble GetDistance( const wxPoint2DDouble &pt ) const;
    inline wxDouble GetDistanceSquare( const wxPoint2DDouble &pt ) const;
    inline wxDouble GetDotProduct( const wxPoint2DDouble &vec ) const;
    inline wxDouble GetCrossProduct( const wxPoint2DDouble &vec ) const;

    // the reflection of this point
    inline wxPoint2DDouble operator-();

    inline wxPoint2DDouble& operator=(const wxPoint2DDouble& pt);
    inline wxPoint2DDouble& operator+=(const wxPoint2DDouble& pt);
    inline wxPoint2DDouble& operator-=(const wxPoint2DDouble& pt);
    inline wxPoint2DDouble& operator*=(const wxPoint2DDouble& pt);
    inline wxPoint2DDouble& operator*=(wxDouble n);
    inline wxPoint2DDouble& operator*=(wxInt32 n);
    inline wxPoint2DDouble& operator/=(const wxPoint2DDouble& pt);
    inline wxPoint2DDouble& operator/=(wxDouble n);
    inline wxPoint2DDouble& operator/=(wxInt32 n);

    inline bool operator==(const wxPoint2DDouble& pt) const;
    inline bool operator!=(const wxPoint2DDouble& pt) const;

    wxDouble m_x;
    wxDouble m_y;
};

inline wxPoint2DDouble operator+(const wxPoint2DDouble& pt1 , const wxPoint2DDouble& pt2);
inline wxPoint2DDouble operator-(const wxPoint2DDouble& pt1 , const wxPoint2DDouble& pt2);
inline wxPoint2DDouble operator*(const wxPoint2DDouble& pt1 , const wxPoint2DDouble& pt2);
inline wxPoint2DDouble operator*(wxDouble n , const wxPoint2DDouble& pt);
inline wxPoint2DDouble operator*(wxInt32 n , const wxPoint2DDouble& pt);
inline wxPoint2DDouble operator*(const wxPoint2DDouble& pt , wxDouble n);
inline wxPoint2DDouble operator*(const wxPoint2DDouble& pt , wxInt32 n);
inline wxPoint2DDouble operator/(const wxPoint2DDouble& pt1 , const wxPoint2DDouble& pt2);
inline wxPoint2DDouble operator/(const wxPoint2DDouble& pt , wxDouble n);
inline wxPoint2DDouble operator/(const wxPoint2DDouble& pt , wxInt32 n);

inline wxPoint2DDouble::wxPoint2DDouble()
{
    m_x = 0.0;
    m_y = 0.0;
}

inline wxPoint2DDouble::wxPoint2DDouble( wxDouble x , wxDouble y )
{
    m_x = x;
    m_y = y;
}

inline wxPoint2DDouble::wxPoint2DDouble( const wxPoint2DDouble &pt )
{
    m_x = pt.m_x;
    m_y = pt.m_y;
}

inline void wxPoint2DDouble::GetFloor( wxInt32 *x , wxInt32 *y ) const
{
    *x = (wxInt32) floor( m_x );
    *y = (wxInt32) floor( m_y );
}

inline void wxPoint2DDouble::GetRounded( wxInt32 *x , wxInt32 *y ) const
{
    *x = (wxInt32) floor( m_x + 0.5 );
    *y = (wxInt32) floor( m_y + 0.5);
}

inline wxDouble wxPoint2DDouble::GetVectorLength() const
{
    return sqrt( (m_x)*(m_x) + (m_y)*(m_y) ) ;
}

inline void wxPoint2DDouble::SetVectorLength( wxDouble length )
{
    wxDouble before = GetVectorLength() ;
    m_x = (m_x * length / before) ;
    m_y = (m_y * length / before) ;
}

inline void wxPoint2DDouble::Normalize()
{
    SetVectorLength( 1 );
}

inline wxDouble wxPoint2DDouble::GetDistance( const wxPoint2DDouble &pt ) const
{
    return sqrt( GetDistanceSquare( pt ) );
}

inline wxDouble wxPoint2DDouble::GetDistanceSquare( const wxPoint2DDouble &pt ) const
{
    return ( (pt.m_x-m_x)*(pt.m_x-m_x) + (pt.m_y-m_y)*(pt.m_y-m_y) );
}

inline wxDouble wxPoint2DDouble::GetDotProduct( const wxPoint2DDouble &vec ) const
{
    return ( m_x * vec.m_x + m_y * vec.m_y );
}

inline wxDouble wxPoint2DDouble::GetCrossProduct( const wxPoint2DDouble &vec ) const
{
    return ( m_x * vec.m_y - vec.m_x * m_y );
}

inline wxPoint2DDouble wxPoint2DDouble::operator-()
{
    return wxPoint2DDouble( -m_x, -m_y);
}

inline wxPoint2DDouble& wxPoint2DDouble::operator=(const wxPoint2DDouble& pt)
{
    m_x = pt.m_x;
    m_y = pt.m_y;
    return *this;
}

inline wxPoint2DDouble& wxPoint2DDouble::operator+=(const wxPoint2DDouble& pt)
{
    m_x = m_x + pt.m_x;
    m_y = m_y + pt.m_y;
    return *this;
}

inline wxPoint2DDouble& wxPoint2DDouble::operator-=(const wxPoint2DDouble& pt)
{
    m_x = m_x - pt.m_x;
    m_y = m_y - pt.m_y;
    return *this;
}

inline wxPoint2DDouble& wxPoint2DDouble::operator*=(const wxPoint2DDouble& pt)
{
    m_x = m_x * pt.m_x;
    m_y = m_y * pt.m_y;
    return *this;
}

inline wxPoint2DDouble& wxPoint2DDouble::operator/=(const wxPoint2DDouble& pt)
{
    m_x = m_x / pt.m_x;
    m_y = m_y / pt.m_y;
    return *this;
}

inline bool wxPoint2DDouble::operator==(const wxPoint2DDouble& pt) const
{
    return wxIsSameDouble(m_x, pt.m_x) && wxIsSameDouble(m_y, pt.m_y);
}

inline bool wxPoint2DDouble::operator!=(const wxPoint2DDouble& pt) const
{
    return !(*this == pt);
}

inline wxPoint2DDouble operator+(const wxPoint2DDouble& pt1 , const wxPoint2DDouble& pt2)
{
    return wxPoint2DDouble( pt1.m_x + pt2.m_x , pt1.m_y + pt2.m_y );
}

inline wxPoint2DDouble operator-(const wxPoint2DDouble& pt1 , const wxPoint2DDouble& pt2)
{
    return wxPoint2DDouble( pt1.m_x - pt2.m_x , pt1.m_y - pt2.m_y );
}


inline wxPoint2DDouble operator*(const wxPoint2DDouble& pt1 , const wxPoint2DDouble& pt2)
{
    return wxPoint2DDouble( pt1.m_x * pt2.m_x , pt1.m_y * pt2.m_y );
}

inline wxPoint2DDouble operator*(wxDouble n , const wxPoint2DDouble& pt)
{
    return wxPoint2DDouble( pt.m_x * n , pt.m_y * n );
}

inline wxPoint2DDouble operator*(wxInt32 n , const wxPoint2DDouble& pt)
{
    return wxPoint2DDouble( pt.m_x * n , pt.m_y * n );
}

inline wxPoint2DDouble operator*(const wxPoint2DDouble& pt , wxDouble n)
{
    return wxPoint2DDouble( pt.m_x * n , pt.m_y * n );
}

inline wxPoint2DDouble operator*(const wxPoint2DDouble& pt , wxInt32 n)
{
    return wxPoint2DDouble( pt.m_x * n , pt.m_y * n );
}

inline wxPoint2DDouble operator/(const wxPoint2DDouble& pt1 , const wxPoint2DDouble& pt2)
{
    return wxPoint2DDouble( pt1.m_x / pt2.m_x , pt1.m_y / pt2.m_y );
}

inline wxPoint2DDouble operator/(const wxPoint2DDouble& pt , wxDouble n)
{
    return wxPoint2DDouble( pt.m_x / n , pt.m_y / n );
}

inline wxPoint2DDouble operator/(const wxPoint2DDouble& pt , wxInt32 n)
{
    return wxPoint2DDouble( pt.m_x / n , pt.m_y / n );
}

// wxRect2Ds are a axis-aligned rectangles, each side of the rect is parallel to the x- or m_y- axis. The rectangle is either defined by the
// top left and bottom right corner, or by the top left corner and size. A point is contained within the rectangle if
// left <= x < right  and top <= m_y < bottom , thus it is a half open interval.

class WXDLLEXPORT wxRect2DDouble
{
public:
    wxRect2DDouble()
        { m_x = m_y = m_width = m_height = 0; }
    wxRect2DDouble(wxDouble x, wxDouble y, wxDouble w, wxDouble h)
        { m_x = x; m_y = y; m_width = w;  m_height = h; }
/*
    wxRect2DDouble(const wxPoint2DDouble& topLeft, const wxPoint2DDouble& bottomRight);
    wxRect2DDouble(const wxPoint2DDouble& pos, const wxSize& size);
    wxRect2DDouble(const wxRect2DDouble& rect);
*/
        // single attribute accessors

    inline wxPoint2DDouble GetPosition()
        { return wxPoint2DDouble(m_x, m_y); }
    inline wxSize GetSize()
        { return wxSize((int) m_width, (int) m_height); }

    // for the edge and corner accessors there are two setters conterparts, the Set.. functions keep the other corners at their
        // position whenever sensible, the Move.. functions keep the size of the rect and move the other corners apropriately

    inline wxDouble GetLeft() const { return m_x; }
    inline void SetLeft( wxDouble n ) { m_width += m_x - n; m_x = n; }
    inline void MoveLeftTo( wxDouble n ) { m_x = n; }
    inline wxDouble GetTop() const { return m_y; }
    inline void SetTop( wxDouble n ) { m_height += m_y - n; m_y = n; }
    inline void MoveTopTo( wxDouble n ) { m_y = n; }
    inline wxDouble GetBottom() const { return m_y + m_height; }
    inline void SetBottom( wxDouble n ) { m_height += n - (m_y+m_height);}
    inline void MoveBottomTo( wxDouble n ) { m_y = n - m_height; }
    inline wxDouble GetRight() const { return m_x + m_width; }
    inline void SetRight( wxDouble n ) { m_width += n - (m_x+m_width) ; }
    inline void MoveRightTo( wxDouble n ) { m_x = n - m_width; }

    inline wxPoint2DDouble GetLeftTop() const
        { return wxPoint2DDouble( m_x , m_y ); }
    inline void SetLeftTop( const wxPoint2DDouble &pt )
        { m_width += m_x - pt.m_x; m_height += m_y - pt.m_y; m_x = pt.m_x; m_y = pt.m_y; }
    inline void MoveLeftTopTo( const wxPoint2DDouble &pt )
        { m_x = pt.m_x; m_y = pt.m_y; }
    inline wxPoint2DDouble GetLeftBottom() const
        { return wxPoint2DDouble( m_x , m_y + m_height ); }
    inline void SetLeftBottom( const wxPoint2DDouble &pt )
        { m_width += m_x - pt.m_x; m_height += pt.m_y - (m_y+m_height) ; m_x = pt.m_x; }
    inline void MoveLeftBottomTo( const wxPoint2DDouble &pt )
        { m_x = pt.m_x; m_y = pt.m_y - m_height; }
    inline wxPoint2DDouble GetRightTop() const
        { return wxPoint2DDouble( m_x+m_width , m_y ); }
    inline void SetRightTop( const wxPoint2DDouble &pt )
        { m_width += pt.m_x - ( m_x + m_width ); m_height += m_y - pt.m_y; m_y = pt.m_y; }
    inline void MoveRightTopTo( const wxPoint2DDouble &pt )
        { m_x = pt.m_x - m_width; m_y = pt.m_y; }
    inline wxPoint2DDouble GetRightBottom() const
        { return wxPoint2DDouble( m_x+m_width , m_y + m_height ); }
    inline void SetRightBottom( const wxPoint2DDouble &pt )
        { m_width += pt.m_x - ( m_x + m_width ); m_height += pt.m_y - (m_y+m_height);}
    inline void MoveRightBottomTo( const wxPoint2DDouble &pt )
        { m_x = pt.m_x - m_width; m_y = pt.m_y - m_height; }
    inline wxPoint2DDouble GetCentre() const
        { return wxPoint2DDouble( m_x+m_width/2 , m_y+m_height/2 ); }
    inline void SetCentre( const wxPoint2DDouble &pt )
        { MoveCentreTo( pt ); }    // since this is impossible without moving...
    inline void MoveCentreTo( const wxPoint2DDouble &pt )
        { m_x += pt.m_x - (m_x+m_width/2) , m_y += pt.m_y -(m_y+m_height/2); }
    inline wxOutCode GetOutCode( const wxPoint2DDouble &pt ) const
        { return (wxOutCode) (( ( pt.m_x < m_x ) ? wxOutLeft : 0 ) +
                     ( ( pt.m_x > m_x + m_width ) ? wxOutRight : 0 ) +
                     ( ( pt.m_y < m_y ) ? wxOutTop : 0 )  +
                     ( ( pt.m_y > m_y + m_height ) ? wxOutBottom : 0 )); }
    inline wxOutCode GetOutcode(const wxPoint2DDouble &pt) const
        { return GetOutCode(pt) ; }
    inline bool Contains( const wxPoint2DDouble &pt ) const
        { return  GetOutCode( pt ) == wxInside; }
    inline bool Contains( const wxRect2DDouble &rect ) const
        { return ( ( ( m_x <= rect.m_x ) && ( rect.m_x + rect.m_width <= m_x + m_width ) ) &&
                ( ( m_y <= rect.m_y ) && ( rect.m_y + rect.m_height <= m_y + m_height ) ) ); }
    inline bool IsEmpty() const
        { return m_width <= 0 || m_height <= 0; }
    inline bool HaveEqualSize( const wxRect2DDouble &rect ) const
        { return wxIsSameDouble(rect.m_width, m_width) && wxIsSameDouble(rect.m_height, m_height); }

    inline void Inset( wxDouble x , wxDouble y )
        { m_x += x; m_y += y; m_width -= 2 * x; m_height -= 2 * y; }
    inline void Inset( wxDouble left , wxDouble top ,wxDouble right , wxDouble bottom  )
        { m_x += left; m_y += top; m_width -= left + right; m_height -= top + bottom;}
    inline void Offset( const wxPoint2DDouble &pt )
        { m_x += pt.m_x; m_y += pt.m_y; }

    void ConstrainTo( const wxRect2DDouble &rect );

    inline wxPoint2DDouble Interpolate( wxInt32 widthfactor , wxInt32 heightfactor )
        { return wxPoint2DDouble( m_x + m_width * widthfactor , m_y + m_height * heightfactor ); }

    static void Intersect( const wxRect2DDouble &src1 , const wxRect2DDouble &src2 , wxRect2DDouble *dest );
    inline void Intersect( const wxRect2DDouble &otherRect )
        { Intersect( *this , otherRect , this ); }
    inline wxRect2DDouble CreateIntersection( const wxRect2DDouble &otherRect ) const
        { wxRect2DDouble result; Intersect( *this , otherRect , &result); return result; }
    bool Intersects( const wxRect2DDouble &rect ) const;

    static void Union( const wxRect2DDouble &src1 , const wxRect2DDouble &src2 , wxRect2DDouble *dest );
    void Union( const wxRect2DDouble &otherRect )
        { Union( *this , otherRect , this ); }
    void Union( const wxPoint2DDouble &pt );
    inline wxRect2DDouble CreateUnion( const wxRect2DDouble &otherRect ) const
        { wxRect2DDouble result; Union( *this , otherRect , &result); return result; }

    inline void Scale( wxDouble f )
        { m_x *= f; m_y *= f; m_width *= f; m_height *= f;}
    inline void Scale( wxInt32 num , wxInt32 denum )
        { m_x *= ((wxDouble)num)/((wxDouble)denum); m_y *= ((wxDouble)num)/((wxDouble)denum);
                m_width *= ((wxDouble)num)/((wxDouble)denum); m_height *= ((wxDouble)num)/((wxDouble)denum);}

    wxRect2DDouble& operator = (const wxRect2DDouble& rect);
    inline bool operator == (const wxRect2DDouble& rect) const
        { return wxIsSameDouble(m_x, rect.m_x) && wxIsSameDouble(m_y, rect.m_y) && HaveEqualSize(rect); }
    inline bool operator != (const wxRect2DDouble& rect) const
        { return !(*this == rect); }

    wxDouble  m_x;
    wxDouble  m_y;
    wxDouble  m_width;
    wxDouble m_height;
};


// wxRect2Ds are a axis-aligned rectangles, each side of the rect is parallel to the x- or m_y- axis. The rectangle is either defined by the
// top left and bottom right corner, or by the top left corner and size. A point is contained within the rectangle if
// left <= x < right  and top <= m_y < bottom , thus it is a half open interval.

class WXDLLEXPORT wxRect2DInt
{
public:
       wxRect2DInt() { m_x = m_y = m_width = m_height = 0; }
       wxRect2DInt( const wxRect& r ) { m_x = r.x ; m_y = r.y ; m_width = r.width ; m_height = r.height ; }
       wxRect2DInt(wxInt32 x, wxInt32 y, wxInt32 w, wxInt32 h) { m_x = x; m_y = y; m_width = w;  m_height = h; }
       wxRect2DInt(const wxPoint2DInt& topLeft, const wxPoint2DInt& bottomRight);
       inline wxRect2DInt(const wxPoint2DInt& pos, const wxSize& size);
       inline wxRect2DInt(const wxRect2DInt& rect);

        // single attribute accessors

      inline wxPoint2DInt GetPosition() { return wxPoint2DInt(m_x, m_y); }
       inline wxSize GetSize() { return wxSize(m_width, m_height); }

        // for the edge and corner accessors there are two setters conterparts, the Set.. functions keep the other corners at their
        // position whenever sensible, the Move.. functions keep the size of the rect and move the other corners apropriately

      inline wxInt32 GetLeft() const { return m_x; }
       inline void SetLeft( wxInt32 n ) { m_width += m_x - n; m_x = n; }
       inline void MoveLeftTo( wxInt32 n ) { m_x = n; }
       inline wxInt32 GetTop() const { return m_y; }
       inline void SetTop( wxInt32 n ) { m_height += m_y - n; m_y = n; }
       inline void MoveTopTo( wxInt32 n ) { m_y = n; }
       inline wxInt32 GetBottom() const { return m_y + m_height; }
       inline void SetBottom( wxInt32 n ) { m_height += n - (m_y+m_height);}
       inline void MoveBottomTo( wxInt32 n ) { m_y = n - m_height; }
       inline wxInt32 GetRight() const { return m_x + m_width; }
       inline void SetRight( wxInt32 n ) { m_width += n - (m_x+m_width) ; }
       inline void MoveRightTo( wxInt32 n ) { m_x = n - m_width; }

        inline wxPoint2DInt GetLeftTop() const { return wxPoint2DInt( m_x , m_y ); }
        inline void SetLeftTop( const wxPoint2DInt &pt ) { m_width += m_x - pt.m_x; m_height += m_y - pt.m_y; m_x = pt.m_x; m_y = pt.m_y; }
        inline void MoveLeftTopTo( const wxPoint2DInt &pt ) { m_x = pt.m_x; m_y = pt.m_y; }
        inline wxPoint2DInt GetLeftBottom() const { return wxPoint2DInt( m_x , m_y + m_height ); }
        inline void SetLeftBottom( const wxPoint2DInt &pt ) { m_width += m_x - pt.m_x; m_height += pt.m_y - (m_y+m_height) ; m_x = pt.m_x; }
        inline void MoveLeftBottomTo( const wxPoint2DInt &pt ) { m_x = pt.m_x; m_y = pt.m_y - m_height; }
        inline wxPoint2DInt GetRightTop() const { return wxPoint2DInt( m_x+m_width , m_y ); }
        inline void SetRightTop( const wxPoint2DInt &pt ) { m_width += pt.m_x - ( m_x + m_width ); m_height += m_y - pt.m_y; m_y = pt.m_y; }
        inline void MoveRightTopTo( const wxPoint2DInt &pt ) { m_x = pt.m_x - m_width; m_y = pt.m_y; }
        inline wxPoint2DInt GetRightBottom() const { return wxPoint2DInt( m_x+m_width , m_y + m_height ); }
        inline void SetRightBottom( const wxPoint2DInt &pt ) { m_width += pt.m_x - ( m_x + m_width ); m_height += pt.m_y - (m_y+m_height);}
        inline void MoveRightBottomTo( const wxPoint2DInt &pt ) { m_x = pt.m_x - m_width; m_y = pt.m_y - m_height; }
        inline wxPoint2DInt GetCentre() const { return wxPoint2DInt( m_x+m_width/2 , m_y+m_height/2 ); }
        inline void SetCentre( const wxPoint2DInt &pt ) { MoveCentreTo( pt ); }    // since this is impossible without moving...
        inline void MoveCentreTo( const wxPoint2DInt &pt ) { m_x += pt.m_x - (m_x+m_width/2) , m_y += pt.m_y -(m_y+m_height/2); }
        inline wxOutCode GetOutCode( const wxPoint2DInt &pt ) const
            { return (wxOutCode) (( ( pt.m_x < m_x ) ? wxOutLeft : 0 ) +
                     ( ( pt.m_x >= m_x + m_width ) ? wxOutRight : 0 ) +
                     ( ( pt.m_y < m_y ) ? wxOutTop : 0 )  +
                     ( ( pt.m_y >= m_y + m_height ) ? wxOutBottom : 0 )); }
        inline wxOutCode GetOutcode( const wxPoint2DInt &pt ) const
            { return GetOutCode( pt ) ; }
        inline bool Contains( const wxPoint2DInt &pt ) const
            { return  GetOutCode( pt ) == wxInside; }
        inline bool Contains( const wxRect2DInt &rect ) const
            { return ( ( ( m_x <= rect.m_x ) && ( rect.m_x + rect.m_width <= m_x + m_width ) ) &&
                ( ( m_y <= rect.m_y ) && ( rect.m_y + rect.m_height <= m_y + m_height ) ) ); }
        inline bool IsEmpty() const
            { return ( m_width <= 0 || m_height <= 0 ); }
        inline bool HaveEqualSize( const wxRect2DInt &rect ) const
            { return ( rect.m_width == m_width && rect.m_height == m_height ); }

        inline void Inset( wxInt32 x , wxInt32 y ) { m_x += x; m_y += y; m_width -= 2 * x; m_height -= 2 * y; }
        inline void Inset( wxInt32 left , wxInt32 top ,wxInt32 right , wxInt32 bottom  )
            { m_x += left; m_y += top; m_width -= left + right; m_height -= top + bottom;}
        inline void Offset( const wxPoint2DInt &pt ) { m_x += pt.m_x; m_y += pt.m_y; }
        void ConstrainTo( const wxRect2DInt &rect );
        inline wxPoint2DInt Interpolate( wxInt32 widthfactor , wxInt32 heightfactor ) { return wxPoint2DInt( m_x + m_width * widthfactor , m_y + m_height * heightfactor ); }

        static void Intersect( const wxRect2DInt &src1 , const wxRect2DInt &src2 , wxRect2DInt *dest );
        inline void Intersect( const wxRect2DInt &otherRect ) { Intersect( *this , otherRect , this ); }
        inline wxRect2DInt CreateIntersection( const wxRect2DInt &otherRect ) const { wxRect2DInt result; Intersect( *this , otherRect , &result); return result; }
        bool Intersects( const wxRect2DInt &rect ) const;

        static void Union( const wxRect2DInt &src1 , const wxRect2DInt &src2 , wxRect2DInt *dest );
        void Union( const wxRect2DInt &otherRect )  { Union( *this , otherRect , this ); }
        void Union( const wxPoint2DInt &pt );
        inline wxRect2DInt CreateUnion( const wxRect2DInt &otherRect ) const { wxRect2DInt result; Union( *this , otherRect , &result); return result; }

        inline void Scale( wxInt32 f ) { m_x *= f; m_y *= f; m_width *= f; m_height *= f;}
        inline void Scale( wxInt32 num , wxInt32 denum )
            { m_x *= ((wxInt32)num)/((wxInt32)denum); m_y *= ((wxInt32)num)/((wxInt32)denum);
                m_width *= ((wxInt32)num)/((wxInt32)denum); m_height *= ((wxInt32)num)/((wxInt32)denum);}

       wxRect2DInt& operator = (const wxRect2DInt& rect);
       bool operator == (const wxRect2DInt& rect) const;
       bool operator != (const wxRect2DInt& rect) const;

#if wxUSE_STREAMS
       void WriteTo( wxDataOutputStream &stream ) const;
       void ReadFrom( wxDataInputStream &stream );
#endif // wxUSE_STREAMS

       wxInt32 m_x;
       wxInt32 m_y;
       wxInt32 m_width;
       wxInt32 m_height;
};

inline wxRect2DInt::wxRect2DInt( const wxRect2DInt &r )
{
    m_x = r.m_x;
    m_y = r.m_y;
    m_width = r.m_width;
    m_height = r.m_height;
}

inline wxRect2DInt::wxRect2DInt( const wxPoint2DInt &a , const wxPoint2DInt &b)
{
    m_x = wxMin( a.m_x , b.m_x );
    m_y = wxMin( a.m_y , b.m_y );
    m_width = abs( a.m_x - b.m_x );
    m_height = abs( a.m_y - b.m_y );
}

inline wxRect2DInt::wxRect2DInt( const wxPoint2DInt& pos, const wxSize& size)
{
    m_x = pos.m_x;
    m_y = pos.m_y;
    m_width = size.x;
    m_height = size.y;
}

inline bool wxRect2DInt::operator == (const wxRect2DInt& rect) const
{
    return (m_x==rect.m_x && m_y==rect.m_y &&
            m_width==rect.m_width && m_height==rect.m_height);
}

inline bool wxRect2DInt::operator != (const wxRect2DInt& rect) const
{
    return !(*this == rect);
}

class wxTransform2D
{
public :
    virtual ~wxTransform2D() { }
    virtual void                    Transform( wxPoint2DInt* pt )const  = 0;
    virtual void                    Transform( wxRect2DInt* r ) const;
    virtual wxPoint2DInt    Transform( const wxPoint2DInt &pt ) const;
    virtual wxRect2DInt        Transform( const wxRect2DInt &r ) const ;

    virtual void                    InverseTransform( wxPoint2DInt* pt ) const  = 0;
    virtual void                    InverseTransform( wxRect2DInt* r ) const ;
    virtual wxPoint2DInt    InverseTransform( const wxPoint2DInt &pt ) const ;
    virtual wxRect2DInt        InverseTransform( const wxRect2DInt &r ) const ;
};

inline void    wxTransform2D::Transform( wxRect2DInt* r ) const
    { wxPoint2DInt a = r->GetLeftTop() , b = r->GetRightBottom(); Transform( &a ); Transform( &b ); *r = wxRect2DInt( a , b ); }

inline wxPoint2DInt    wxTransform2D::Transform( const wxPoint2DInt &pt ) const
    { wxPoint2DInt res = pt; Transform( &res ); return res; }

inline wxRect2DInt     wxTransform2D::Transform( const wxRect2DInt &r ) const
    { wxRect2DInt res = r; Transform( &res ); return res; }

inline void    wxTransform2D::InverseTransform( wxRect2DInt* r ) const
    { wxPoint2DInt a = r->GetLeftTop() , b = r->GetRightBottom(); InverseTransform( &a ); InverseTransform( &b ); *r = wxRect2DInt( a , b ); }

inline wxPoint2DInt    wxTransform2D::InverseTransform( const wxPoint2DInt &pt ) const
    { wxPoint2DInt res = pt; InverseTransform( &res ); return res; }

inline wxRect2DInt     wxTransform2D::InverseTransform( const wxRect2DInt &r ) const
    { wxRect2DInt res = r; InverseTransform( &res ); return res; }


#endif // wxUSE_GEOMETRY

#endif // _WX_GEOMETRY_H_
