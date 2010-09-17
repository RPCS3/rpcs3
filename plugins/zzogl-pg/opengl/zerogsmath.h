 /* ZeroGS KOSMOS
  *
  * Zerofrog's ZeroGS KOSMOS (c)2005-2008
  *
  * Zerofrog forgot to write any copyright notice after release the plugin into GPLv2
  * If someone can contact him successfully to clarify this matter that would be great.
  */

#ifndef ZEROGS_MATH_H
#define ZEROGS_MATH_H

#ifndef _WIN32
#include <alloca.h>
#endif

#include <string.h>
#include <math.h>
#include <assert.h>

typedef float dReal;

// class used for 3 and 4 dim vectors and quaternions
// It is better to use this for a 3 dim vector because it is 16byte aligned and SIMD instructions can be used

class Vector
{
	public:
		dReal x, y, z, w;

		Vector() : x(0), y(0), z(0), w(0) {}
		Vector(dReal x, dReal y, dReal z) : x(x), y(y), z(z), w(0) {}
		Vector(dReal x, dReal y, dReal z, dReal w) : x(x), y(y), z(z), w(w) {}
		Vector(const Vector &vec) : x(vec.x), y(vec.y), z(vec.z), w(vec.w) {}
		Vector(const dReal* pf) { assert(pf != NULL); x = pf[0]; y = pf[1]; z = pf[2]; w = 0; }
		dReal  operator[](int i) const	   { return (&x)[i]; }
		dReal& operator[](int i)			 { return (&x)[i]; }
		
		// casting operators
		operator dReal*() { return &x; }
		operator const dReal*() const { return (const dReal*)&x; }
		
		// SCALAR FUNCTIONS
		inline dReal dot(const Vector &v) const { return x*v.x + y*v.y + z*v.z + w*v.w; }
		inline void Set3(const float* pvals) { x = pvals[0]; y = pvals[1]; z = pvals[2]; }
		inline void Set4(const float* pvals) { x = pvals[0]; y = pvals[1]; z = pvals[2]; w = pvals[3]; }
		inline void SetColor(u32 color)
		{
			x = (color & 0xff) / 255.0f;
			y = ((color >> 8) & 0xff) / 255.0f;
			z = ((color >> 16) & 0xff) / 255.0f;
		}

		// 3 dim cross product, w is not touched
		/// this = this x v
		/// this = u x v
		inline Vector operator-() const { Vector v; v.x = -x; v.y = -y; v.z = -z; v.w = -w; return v; }
		inline Vector operator+(const Vector &r) const { Vector v; v.x = x + r.x; v.y = y + r.y; v.z = z + r.z; v.w = w + r.w; return v; }
		inline Vector operator-(const Vector &r) const { Vector v; v.x = x - r.x; v.y = y - r.y; v.z = z - r.z; v.w = w - r.w; return v; }
		inline Vector operator*(const Vector &r) const { Vector v; v.x = r.x * x; v.y = r.y * y; v.z = r.z * z; v.w = r.w * w; return v; }
		inline Vector operator*(dReal k) const { Vector v; v.x = k * x; v.y = k * y; v.z = k * z; v.w = k * w; return v; }
		inline Vector& operator += (const Vector& r) { x += r.x; y += r.y; z += r.z; w += r.w; return *this; }
		inline Vector& operator -= (const Vector& r) { x -= r.x; y -= r.y; z -= r.z; w -= r.w; return *this; }
		inline Vector& operator *= (const Vector& r) { x *= r.x; y *= r.y; z *= r.z; w *= r.w; return *this; }
		inline Vector& operator *= (const dReal k) { x *= k; y *= k; z *= k; w *= k; return *this; }
		inline Vector& operator /= (const dReal _k) { dReal k = 1 / _k; x *= k; y *= k; z *= k; w *= k; return *this; }
		friend Vector operator*(float f, const Vector& v);
		//friend ostream& operator<<(ostream& O, const Vector& v);
		//friend istream& operator>>(istream& I, Vector& v);
};

inline Vector operator*(float f, const Vector& left)
{
	Vector v;
	v.x = f * left.x;
	v.y = f * left.y;
	v.z = f * left.z;
	return v;
}

#endif
