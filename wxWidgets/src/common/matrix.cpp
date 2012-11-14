///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/matrix.cpp
// Purpose:     wxTransformMatrix class
// Author:      Chris Breeze, Julian Smart
// Modified by: Klaas Holwerda
// Created:     01/02/97
// RCS-ID:      $Id: matrix.cpp 39745 2006-06-15 17:58:49Z ABX $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// Note: this is intended to be used in wxDC at some point to replace
// the current system of scaling/translation. It is not yet used.

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/matrix.h"

#ifndef WX_PRECOMP
    #include "wx/math.h"
#endif

static const double pi = M_PI;

wxTransformMatrix::wxTransformMatrix(void)
{
    m_isIdentity = false;

    Identity();
}

wxTransformMatrix::wxTransformMatrix(const wxTransformMatrix& mat)
    : wxObject()
{
    (*this) = mat;
}

double wxTransformMatrix::GetValue(int col, int row) const
{
    if (row < 0 || row > 2 || col < 0 || col > 2)
        return 0.0;

    return m_matrix[col][row];
}

void wxTransformMatrix::SetValue(int col, int row, double value)
{
    if (row < 0 || row > 2 || col < 0 || col > 2)
        return;

    m_matrix[col][row] = value;
    m_isIdentity = IsIdentity1();
}

void wxTransformMatrix::operator = (const wxTransformMatrix& mat)
{
    int i, j;
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            m_matrix[i][j] = mat.m_matrix[i][j];
        }
    }
    m_isIdentity = mat.m_isIdentity;
}

bool wxTransformMatrix::operator == (const wxTransformMatrix& mat) const
{
    if (m_isIdentity && mat.m_isIdentity)
        return true;

    int i, j;
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            if ( !wxIsSameDouble(m_matrix[i][j], mat.m_matrix[i][j]) )
                return false;
        }
    }
    return true;
}

bool wxTransformMatrix::operator != (const wxTransformMatrix& mat) const
{
    return (! ((*this) == mat));
}

double& wxTransformMatrix::operator()(int col, int row)
{
    if (row < 0 || row > 2 || col < 0 || col > 2)
        return m_matrix[0][0];

    return m_matrix[col][row];
}

double wxTransformMatrix::operator()(int col, int row) const
{
    if (row < 0 || row > 2 || col < 0 || col > 2)
        return 0.0;

    return m_matrix[col][row];
}

// Invert matrix
bool wxTransformMatrix::Invert(void)
{
    double inverseMatrix[3][3];

    // calculate the adjoint
    inverseMatrix[0][0] =  wxCalculateDet(m_matrix[1][1],m_matrix[2][1],m_matrix[1][2],m_matrix[2][2]);
    inverseMatrix[0][1] = -wxCalculateDet(m_matrix[0][1],m_matrix[2][1],m_matrix[0][2],m_matrix[2][2]);
    inverseMatrix[0][2] =  wxCalculateDet(m_matrix[0][1],m_matrix[1][1],m_matrix[0][2],m_matrix[1][2]);

    inverseMatrix[1][0] = -wxCalculateDet(m_matrix[1][0],m_matrix[2][0],m_matrix[1][2],m_matrix[2][2]);
    inverseMatrix[1][1] =  wxCalculateDet(m_matrix[0][0],m_matrix[2][0],m_matrix[0][2],m_matrix[2][2]);
    inverseMatrix[1][2] = -wxCalculateDet(m_matrix[0][0],m_matrix[1][0],m_matrix[0][2],m_matrix[1][2]);

    inverseMatrix[2][0] =  wxCalculateDet(m_matrix[1][0],m_matrix[2][0],m_matrix[1][1],m_matrix[2][1]);
    inverseMatrix[2][1] = -wxCalculateDet(m_matrix[0][0],m_matrix[2][0],m_matrix[0][1],m_matrix[2][1]);
    inverseMatrix[2][2] =  wxCalculateDet(m_matrix[0][0],m_matrix[1][0],m_matrix[0][1],m_matrix[1][1]);

    // now divide by the determinant
    double det = m_matrix[0][0] * inverseMatrix[0][0] + m_matrix[0][1] * inverseMatrix[1][0] + m_matrix[0][2] * inverseMatrix[2][0];
    if ( wxIsNullDouble(det) )
        return false;

    inverseMatrix[0][0] /= det; inverseMatrix[1][0] /= det; inverseMatrix[2][0] /= det;
    inverseMatrix[0][1] /= det; inverseMatrix[1][1] /= det; inverseMatrix[2][1] /= det;
    inverseMatrix[0][2] /= det; inverseMatrix[1][2] /= det; inverseMatrix[2][2] /= det;

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            m_matrix[i][j] = inverseMatrix[i][j];
        }
    }
    m_isIdentity = IsIdentity1();
    return true;
}

// Make into identity matrix
bool wxTransformMatrix::Identity(void)
{
    m_matrix[0][0] = m_matrix[1][1] = m_matrix[2][2] = 1.0;
    m_matrix[1][0] = m_matrix[2][0] = m_matrix[0][1] = m_matrix[2][1] = m_matrix[0][2] = m_matrix[1][2] = 0.0;
    m_isIdentity = true;

    return true;
}

// Scale by scale (isotropic scaling i.e. the same in x and y):
//           | scale  0      0      |
// matrix' = |  0     scale  0      | x matrix
//           |  0     0      scale  |
//
bool wxTransformMatrix::Scale(double scale)
{
    int i, j;
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            m_matrix[i][j] *= scale;
        }
    }
    m_isIdentity = IsIdentity1();

    return true;
}


// scale a matrix in 2D
//
//     xs    0      xc(1-xs)
//     0    ys      yc(1-ys)
//     0     0      1
//
wxTransformMatrix&  wxTransformMatrix::Scale(const double &xs, const double &ys,const double &xc, const double &yc)
{
    double r00,r10,r20,r01,r11,r21;

    if (m_isIdentity)
    {
        double tx = xc*(1-xs);
        double ty = yc*(1-ys);
        r00 = xs;
        r10 = 0;
        r20 = tx;
        r01 = 0;
        r11 = ys;
        r21 = ty;
    }
    else if ( !wxIsNullDouble(xc) || !wxIsNullDouble(yc) )
    {
        double tx = xc*(1-xs);
        double ty = yc*(1-ys);
        r00 = xs * m_matrix[0][0];
        r10 = xs * m_matrix[1][0];
        r20 = xs * m_matrix[2][0] + tx;
        r01 = ys * m_matrix[0][1];
        r11 = ys * m_matrix[1][1];
        r21 = ys * m_matrix[2][1] + ty;
    }
    else
    {
        r00 = xs * m_matrix[0][0];
        r10 = xs * m_matrix[1][0];
        r20 = xs * m_matrix[2][0];
        r01 = ys * m_matrix[0][1];
        r11 = ys * m_matrix[1][1];
        r21 = ys * m_matrix[2][1];
    }

    m_matrix[0][0] = r00;
    m_matrix[1][0] = r10;
    m_matrix[2][0] = r20;
    m_matrix[0][1] = r01;
    m_matrix[1][1] = r11;
    m_matrix[2][1] = r21;

/* or like this
    // first translate to origin O
    (*this).Translate(-x_cen, -y_cen);

    // now do the scaling
    wxTransformMatrix scale;
    scale.m_matrix[0][0] = x_fac;
    scale.m_matrix[1][1] = y_fac;
   scale.m_isIdentity = IsIdentity1();

    *this = scale * (*this);

    // translate back from origin to x_cen, y_cen
    (*this).Translate(x_cen, y_cen);
*/

    m_isIdentity = IsIdentity1();

    return *this;
}


// mirror a matrix in x, y
//
//     -1      0      0     Y-mirror
//      0     -1      0     X-mirror
//      0      0     -1     Z-mirror
wxTransformMatrix&  wxTransformMatrix::Mirror(bool x, bool y)
{
    wxTransformMatrix temp;
    if (x)
    {
        temp.m_matrix[1][1] = -1;
        temp.m_isIdentity=false;
    }
    if (y)
    {
        temp.m_matrix[0][0] = -1;
        temp.m_isIdentity=false;
    }

    *this = temp * (*this);
    m_isIdentity = IsIdentity1();
    return *this;
}

// Translate by dx, dy:
//           | 1  0 dx |
// matrix' = | 0  1 dy | x matrix
//           | 0  0  1 |
//
bool wxTransformMatrix::Translate(double dx, double dy)
{
    int i;
    for (i = 0; i < 3; i++)
        m_matrix[i][0] += dx * m_matrix[i][2];
    for (i = 0; i < 3; i++)
        m_matrix[i][1] += dy * m_matrix[i][2];

    m_isIdentity = IsIdentity1();

    return true;
}

// Rotate clockwise by the given number of degrees:
//           |  cos sin 0 |
// matrix' = | -sin cos 0 | x matrix
//           |   0   0  1 |
bool wxTransformMatrix::Rotate(double degrees)
{
    Rotate(-degrees,0,0);
    return true;
}

// counter clockwise rotate around a point
//
//  cos(r) -sin(r)    x(1-cos(r))+y(sin(r)
//  sin(r)  cos(r)    y(1-cos(r))-x(sin(r)
//    0      0        1
wxTransformMatrix&  wxTransformMatrix::Rotate(const double &degrees, const double &x, const double &y)
{
    double angle = degrees * pi / 180.0;
    double c = cos(angle);
    double s = sin(angle);
    double r00,r10,r20,r01,r11,r21;

    if (m_isIdentity)
    {
        double tx = x*(1-c)+y*s;
        double ty = y*(1-c)-x*s;
        r00 = c ;
        r10 = -s;
        r20 = tx;
        r01 = s;
        r11 = c;
        r21 = ty;
    }
    else if ( !wxIsNullDouble(x) || !wxIsNullDouble(y) )
    {
        double tx = x*(1-c)+y*s;
        double ty = y*(1-c)-x*s;
        r00 = c * m_matrix[0][0] - s * m_matrix[0][1] + tx * m_matrix[0][2];
        r10 = c * m_matrix[1][0] - s * m_matrix[1][1] + tx * m_matrix[1][2];
        r20 = c * m_matrix[2][0] - s * m_matrix[2][1] + tx;// * m_matrix[2][2];
        r01 = c * m_matrix[0][1] + s * m_matrix[0][0] + ty * m_matrix[0][2];
        r11 = c * m_matrix[1][1] + s * m_matrix[1][0] + ty * m_matrix[1][2];
        r21 = c * m_matrix[2][1] + s * m_matrix[2][0] + ty;// * m_matrix[2][2];
    }
    else
    {
        r00 = c * m_matrix[0][0] - s * m_matrix[0][1];
        r10 = c * m_matrix[1][0] - s * m_matrix[1][1];
        r20 = c * m_matrix[2][0] - s * m_matrix[2][1];
        r01 = c * m_matrix[0][1] + s * m_matrix[0][0];
        r11 = c * m_matrix[1][1] + s * m_matrix[1][0];
        r21 = c * m_matrix[2][1] + s * m_matrix[2][0];
    }

    m_matrix[0][0] = r00;
    m_matrix[1][0] = r10;
    m_matrix[2][0] = r20;
    m_matrix[0][1] = r01;
    m_matrix[1][1] = r11;
    m_matrix[2][1] = r21;

/* or like this
    wxTransformMatrix rotate;
    rotate.m_matrix[2][0] = tx;
    rotate.m_matrix[2][1] = ty;

    rotate.m_matrix[0][0] = c;
    rotate.m_matrix[0][1] = s;

    rotate.m_matrix[1][0] = -s;
    rotate.m_matrix[1][1] = c;

   rotate.m_isIdentity=false;
   *this = rotate * (*this);
*/
    m_isIdentity = IsIdentity1();

    return *this;
}

// Transform a point from logical to device coordinates
bool wxTransformMatrix::TransformPoint(double x, double y, double& tx, double& ty) const
{
    if (IsIdentity())
    {
        tx = x; ty = y; return true;
    }

    tx = x * m_matrix[0][0] + y * m_matrix[1][0] + m_matrix[2][0];
    ty = x * m_matrix[0][1] + y * m_matrix[1][1] + m_matrix[2][1];

    return true;
}

// Transform a point from device to logical coordinates.

// Example of use:
//   wxTransformMatrix mat = dc.GetTransformation();
//   mat.Invert();
//   mat.InverseTransformPoint(x, y, x1, y1);
// OR (shorthand:)
//   dc.LogicalToDevice(x, y, x1, y1);
// The latter is slightly less efficient if we're doing several
// conversions, since the matrix is inverted several times.
bool wxTransformMatrix::InverseTransformPoint(double x, double y, double& tx, double& ty) const
{
    if (IsIdentity())
    {
        tx = x;
        ty = y;
        return true;
    }

    const double z = (1.0 - m_matrix[0][2] * x - m_matrix[1][2] * y) / m_matrix[2][2];
    if ( wxIsNullDouble(z) )
        return false;

    tx = x * m_matrix[0][0] + y * m_matrix[1][0] + z * m_matrix[2][0];
    ty = x * m_matrix[0][1] + y * m_matrix[1][1] + z * m_matrix[2][1];
    return true;
}

wxTransformMatrix& wxTransformMatrix::operator*=(const double& t)
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            m_matrix[i][j]*= t;
    m_isIdentity = IsIdentity1();
    return *this;
}

wxTransformMatrix& wxTransformMatrix::operator/=(const double& t)
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            m_matrix[i][j]/= t;
    m_isIdentity = IsIdentity1();
    return *this;
}

wxTransformMatrix& wxTransformMatrix::operator+=(const wxTransformMatrix& mat)
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            m_matrix[i][j] += mat.m_matrix[i][j];
    m_isIdentity = IsIdentity1();
    return *this;
}

wxTransformMatrix& wxTransformMatrix::operator-=(const wxTransformMatrix& mat)
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            m_matrix[i][j] -= mat.m_matrix[i][j];
    m_isIdentity = IsIdentity1();
    return *this;
}

wxTransformMatrix& wxTransformMatrix::operator*=(const wxTransformMatrix& mat)
{

    if (mat.m_isIdentity)
        return *this;
    if (m_isIdentity)
    {
        *this = mat;
        return *this;
    }
    else
    {
        wxTransformMatrix  result;
        for (int i = 0; i < 3; i++)
        {
           for (int j = 0; j < 3; j++)
           {
               double sum = 0;
               for (int k = 0; k < 3; k++)
                   sum += m_matrix[k][i] * mat.m_matrix[j][k];
               result.m_matrix[j][i] = sum;
           }
        }
        *this = result;
    }

    m_isIdentity = IsIdentity1();
    return *this;
}


// constant operators
wxTransformMatrix  wxTransformMatrix::operator*(const double& t) const
{
    wxTransformMatrix result = *this;
    result *= t;
    result.m_isIdentity = result.IsIdentity1();
    return result;
}

wxTransformMatrix  wxTransformMatrix::operator/(const double& t) const
{
    wxTransformMatrix result = *this;
//    wxASSERT(t!=0);
    result /= t;
    result.m_isIdentity = result.IsIdentity1();
    return result;
}

wxTransformMatrix  wxTransformMatrix::operator+(const wxTransformMatrix& m) const
{
    wxTransformMatrix result = *this;
    result += m;
    result.m_isIdentity = result.IsIdentity1();
    return result;
}

wxTransformMatrix  wxTransformMatrix::operator-(const wxTransformMatrix& m) const
{
    wxTransformMatrix result = *this;
    result -= m;
    result.m_isIdentity = result.IsIdentity1();
    return result;
}


wxTransformMatrix  wxTransformMatrix::operator*(const wxTransformMatrix& m) const
{
    wxTransformMatrix result = *this;
    result *= m;
    result.m_isIdentity = result.IsIdentity1();
    return result;
}


wxTransformMatrix  wxTransformMatrix::operator-() const
{
    wxTransformMatrix result = *this;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            result.m_matrix[i][j] = -(this->m_matrix[i][j]);
    result.m_isIdentity = result.IsIdentity1();
    return result;
}

static double CheckInt(double getal)
{
    // check if the number is very close to an integer
    if ( (ceil(getal) - getal) < 0.0001)
        return ceil(getal);

    else if ( (getal - floor(getal)) < 0.0001)
        return floor(getal);

    return getal;

}

double wxTransformMatrix::Get_scaleX()
{
    double scale_factor;
    double rot_angle = CheckInt(atan2(m_matrix[1][0],m_matrix[0][0])*180/pi);
    if ( !wxIsSameDouble(rot_angle, 90) && !wxIsSameDouble(rot_angle, -90) )
        scale_factor = m_matrix[0][0]/cos((rot_angle/180)*pi);
    else
        scale_factor = m_matrix[0][0]/sin((rot_angle/180)*pi);  // er kan nl. niet door 0 gedeeld worden !

    scale_factor = CheckInt(scale_factor);
    if (scale_factor < 0)
        scale_factor = -scale_factor;

    return scale_factor;
}

double wxTransformMatrix::Get_scaleY()
{
    double scale_factor;
    double rot_angle = CheckInt(atan2(m_matrix[1][0],m_matrix[0][0])*180/pi);
    if ( !wxIsSameDouble(rot_angle, 90) && !wxIsSameDouble(rot_angle, -90) )
       scale_factor = m_matrix[1][1]/cos((rot_angle/180)*pi);
    else
       scale_factor = m_matrix[1][1]/sin((rot_angle/180)*pi);   // er kan nl. niet door 0 gedeeld worden !

    scale_factor = CheckInt(scale_factor);
    if (scale_factor < 0)

        scale_factor = -scale_factor;

    return scale_factor;

}

double wxTransformMatrix::GetRotation()
{
    double temp1 = GetValue(0,0);   // for angle calculation
    double temp2 = GetValue(0,1);   //

    // Rotation
    double rot_angle = atan2(temp2,temp1)*180/pi;

    rot_angle = CheckInt(rot_angle);
    return rot_angle;
}

void wxTransformMatrix::SetRotation(double rotation)
{
    double x=GetValue(2,0);
    double y=GetValue(2,1);
    Rotate(-GetRotation(), x, y);
    Rotate(rotation, x, y);
}
