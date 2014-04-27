/////////////////////////////////////////////////////////////////////////////
// Name:         wx/matrix.h
// Purpose:      wxTransformMatrix class. NOT YET USED
// Author:       Chris Breeze, Julian Smart
// Modified by:  Klaas Holwerda
// Created:      01/02/97
// RCS-ID:       $Id: matrix.h 45498 2007-04-16 13:03:05Z VZ $
// Copyright:    (c) Julian Smart, Chris Breeze
// Licence:      wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MATRIXH__
#define _WX_MATRIXH__

//! headerfiles="matrix.h wx/object.h"
#include "wx/object.h"
#include "wx/math.h"

//! codefiles="matrix.cpp"

// A simple 3x3 matrix. This may be replaced by a more general matrix
// class some day.
//
// Note: this is intended to be used in wxDC at some point to replace
// the current system of scaling/translation. It is not yet used.

//:definition
//  A 3x3 matrix to do 2D transformations.
//  It can be used to map data to window coordinates,
//  and also for manipulating your own data.
//  For example drawing a picture (composed of several primitives)
//  at a certain coordinate and angle within another parent picture.
//  At all times m_isIdentity is set if the matrix itself is an Identity matrix.
//  It is used where possible to optimize calculations.
class WXDLLEXPORT wxTransformMatrix: public wxObject
{
public:
    wxTransformMatrix(void);
    wxTransformMatrix(const wxTransformMatrix& mat);

    //get the value in the matrix at col,row
    //rows are horizontal (second index of m_matrix member)
    //columns are vertical (first index of m_matrix member)
    double GetValue(int col, int row) const;

    //set the value in the matrix at col,row
    //rows are horizontal (second index of m_matrix member)
    //columns are vertical (first index of m_matrix member)
    void SetValue(int col, int row, double value);

    void operator = (const wxTransformMatrix& mat);
    bool operator == (const wxTransformMatrix& mat) const;
    bool operator != (const wxTransformMatrix& mat) const;

    //multiply every element by t
    wxTransformMatrix&          operator*=(const double& t);
    //divide every element by t
    wxTransformMatrix&          operator/=(const double& t);
    //add matrix m to this t
    wxTransformMatrix&          operator+=(const wxTransformMatrix& m);
    //subtract matrix m from this
    wxTransformMatrix&          operator-=(const wxTransformMatrix& m);
    //multiply matrix m with this
    wxTransformMatrix&          operator*=(const wxTransformMatrix& m);

    // constant operators

    //multiply every element by t  and return result
    wxTransformMatrix           operator*(const double& t) const;
    //divide this matrix by t and return result
    wxTransformMatrix           operator/(const double& t) const;
    //add matrix m to this and return result
    wxTransformMatrix           operator+(const wxTransformMatrix& m) const;
    //subtract matrix m from this and return result
    wxTransformMatrix           operator-(const wxTransformMatrix& m) const;
    //multiply this by matrix m and return result
    wxTransformMatrix           operator*(const wxTransformMatrix& m) const;
    wxTransformMatrix           operator-() const;

    //rows are horizontal (second index of m_matrix member)
    //columns are vertical (first index of m_matrix member)
    double& operator()(int col, int row);

    //rows are horizontal (second index of m_matrix member)
    //columns are vertical (first index of m_matrix member)
    double operator()(int col, int row) const;

    // Invert matrix
    bool Invert(void);

    // Make into identity matrix
    bool Identity(void);

    // Is the matrix the identity matrix?
    // Only returns a flag, which is set whenever an operation
    // is done.
    inline bool IsIdentity(void) const { return m_isIdentity; }

    // This does an actual check.
    inline bool IsIdentity1(void) const ;

    //Scale by scale (isotropic scaling i.e. the same in x and y):
    //!ex:
    //!code:           | scale  0      0      |
    //!code: matrix' = |  0     scale  0      | x matrix
    //!code:           |  0     0      scale  |
    bool Scale(double scale);

    //Scale with center point and x/y scale
    //
    //!ex:
    //!code:           |  xs    0      xc(1-xs) |
    //!code: matrix' = |  0    ys      yc(1-ys) | x matrix
    //!code:           |  0     0      1        |
    wxTransformMatrix&  Scale(const double &xs, const double &ys,const double &xc, const double &yc);

    // mirror a matrix in x, y
    //!ex:
    //!code:           | -1     0      0 |
    //!code: matrix' = |  0    -1      0 | x matrix
    //!code:           |  0     0      1 |
    wxTransformMatrix&  Mirror(bool x=true, bool y=false);
    // Translate by dx, dy:
    //!ex:
    //!code:           | 1  0 dx |
    //!code: matrix' = | 0  1 dy | x matrix
    //!code:           | 0  0  1 |
    bool Translate(double x, double y);

    // Rotate clockwise by the given number of degrees:
    //!ex:
    //!code:           |  cos sin 0 |
    //!code: matrix' = | -sin cos 0 | x matrix
    //!code:           |   0   0  1 |
    bool Rotate(double angle);

    //Rotate counter clockwise with point of rotation
    //
    //!ex:
    //!code:           |  cos(r) -sin(r)    x(1-cos(r))+y(sin(r)|
    //!code: matrix' = |  sin(r)  cos(r)    y(1-cos(r))-x(sin(r)| x matrix
    //!code:           |   0          0                       1 |
    wxTransformMatrix&  Rotate(const double &r, const double &x, const double &y);

    // Transform X value from logical to device
    inline double TransformX(double x) const;

    // Transform Y value from logical to device
    inline double TransformY(double y) const;

    // Transform a point from logical to device coordinates
    bool TransformPoint(double x, double y, double& tx, double& ty) const;

    // Transform a point from device to logical coordinates.
    // Example of use:
    //   wxTransformMatrix mat = dc.GetTransformation();
    //   mat.Invert();
    //   mat.InverseTransformPoint(x, y, x1, y1);
    // OR (shorthand:)
    //   dc.LogicalToDevice(x, y, x1, y1);
    // The latter is slightly less efficient if we're doing several
    // conversions, since the matrix is inverted several times.
    // N.B. 'this' matrix is the inverse at this point
    bool InverseTransformPoint(double x, double y, double& tx, double& ty) const;

    double Get_scaleX();
    double Get_scaleY();
    double GetRotation();
    void   SetRotation(double rotation);


public:
    double  m_matrix[3][3];
    bool    m_isIdentity;
};


/*
Chris Breeze reported, that
some functions of wxTransformMatrix cannot work because it is not
known if he matrix has been inverted. Be careful when using it.
*/

// Transform X value from logical to device
// warning: this function can only be used for this purpose
// because no rotation is involved when mapping logical to device coordinates
// mirror and scaling for x and y will be part of the matrix
// if you have a matrix that is rotated, eg a shape containing a matrix to place
// it in the logical coordinate system, use TransformPoint
inline double wxTransformMatrix::TransformX(double x) const
{
    //normally like this, but since no rotation is involved (only mirror and scale)
    //we can do without Y -> m_matrix[1]{0] is -sin(rotation angle) and therefore zero
    //(x * m_matrix[0][0] + y * m_matrix[1][0] + m_matrix[2][0]))
    return (m_isIdentity ? x : (x * m_matrix[0][0] +  m_matrix[2][0]));
}

// Transform Y value from logical to device
// warning: this function can only be used for this purpose
// because no rotation is involved when mapping logical to device coordinates
// mirror and scaling for x and y will be part of the matrix
// if you have a matrix that is rotated, eg a shape containing a matrix to place
// it in the logical coordinate system, use TransformPoint
inline double wxTransformMatrix::TransformY(double y) const
{
    //normally like this, but since no rotation is involved (only mirror and scale)
    //we can do without X -> m_matrix[0]{1] is sin(rotation angle) and therefore zero
    //(x * m_matrix[0][1] + y * m_matrix[1][1] + m_matrix[2][1]))
    return (m_isIdentity ? y : (y * m_matrix[1][1] + m_matrix[2][1]));
}


// Is the matrix the identity matrix?
// Each operation checks whether the result is still the identity matrix and sets a flag.
inline bool wxTransformMatrix::IsIdentity1(void) const
{
    return
    ( wxIsSameDouble(m_matrix[0][0], 1.0) &&
      wxIsSameDouble(m_matrix[1][1], 1.0) &&
      wxIsSameDouble(m_matrix[2][2], 1.0) &&
      wxIsSameDouble(m_matrix[1][0], 0.0) &&
      wxIsSameDouble(m_matrix[2][0], 0.0) &&
      wxIsSameDouble(m_matrix[0][1], 0.0) &&
      wxIsSameDouble(m_matrix[2][1], 0.0) &&
      wxIsSameDouble(m_matrix[0][2], 0.0) &&
      wxIsSameDouble(m_matrix[1][2], 0.0) );
}

// Calculates the determinant of a 2 x 2 matrix
inline double wxCalculateDet(double a11, double a21, double a12, double a22)
{
    return a11 * a22 - a12 * a21;
}

#endif // _WX_MATRIXH__
