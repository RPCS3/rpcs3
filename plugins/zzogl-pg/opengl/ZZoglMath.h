/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef ZZOGLMATH_H_INCLUDED
#define ZZOGLMATH_H_INCLUDED

//Remind me to check and see if this is necessary, and what uses it. --arcum42
#ifndef _WIN32
#include <alloca.h>
#endif

#include <assert.h>

template <class T>
class Vector4
{
	public:
		T x, y, z, w;
		
		Vector4(T x1 = 0, T y1 = 0, T z1 = 0, T w1 = 0) 
		{ 
			x = x1; 
			y = y1; 
			z = z1; 
			w = w1; 
		}
		
		Vector4(Vector4<T> &f) 
		{ 
			x = f.x; 
			y = f.y; 
			z = f.z; 
			w = f.w; 
		}
		
		Vector4(T* f) 
		{
			x = f[0]; 
			y = f[1]; 
			z = f[2]; 
			w = f[3]; // For some reason, the old code set this to 0. 
		}
		
		T& operator[](int i)
		{
			switch(i)
			{
				case 0: return x;
				case 1: return y;
				case 2: return z;
				case 3: return w;
				default: assert(0);
			}
		}
		
		operator T*()
		{
			return (T*) this;
		}
		
		operator const T*() const
		{
			return (const T*) this;
		}
		
		Vector4<T>& operator =(const Vector4<T>& v)
		{
			x = v.x;
			y = v.y;
			z = v.z;
			w = v.w;
			return *this;
		}
		
		bool operator ==(const Vector4<T>& v)
		{
			return !!(	x == v.x &&
						y == v.y &&
						z == v.z &&
						w == v.w	);
		}
		
		Vector4<T> operator +(const Vector4<T>& v) const
		{
			return Vector4<T>(x + v.x, y + v.y, z + v.z, w + v.w);
		}
		
		Vector4<T> operator -(const Vector4<T>& v) const
		{
			return Vector4<T>(x - v.x, y - v.y, z - v.z, w - v.w);
		}
		
		Vector4<T> operator *(const Vector4<T>& v) const
		{
			return Vector4<T>(x * v.x, y * v.y, z * v.z, w * v.w);
		}
		
		Vector4<T> operator /(const Vector4<T>& v) const
		{
			return Vector4<T>(x / v.x, y / v.y, z / v.z, w / v.w);
		}
		Vector4<T> operator +(T val) const
		{
			return Vector4<T>(x + val, y + val, z + val, w + val);
		}
		
		Vector4<T> operator -(T val) const
		{
			return Vector4<T>(x - val, y - val, z - val, w - val);
		}
		
		Vector4<T> operator *(T val) const
		{
			return Vector4<T>(x * val, y * val, z * val, w * val);
		}
		
		Vector4<T> operator /(T val) const
		{
			return Vector4<T>(x / val, y / val, z / val, w / val);
		}
		
		Vector4<T>& operator +=(const Vector4<T>& v)
		{
			*this = *this + v;
			return *this;
		}
		
		Vector4<T>& operator -=(const Vector4<T>& v)
		{
			*this = *this - v;
			return *this;
		}
		
		Vector4<T>& operator *=(const Vector4<T>& v)
		{
			*this = *this * v;
			return *this;
		}
		
		Vector4<T>& operator /=(const Vector4<T>& v)
		{
			*this = *this - v;
			return *this;
		}
		
		Vector4<T>& operator +=(T val)
		{
			*this = *this + (T)val;
			return *this;
		}
		
		Vector4<T>& operator -=(T val)
		{
			*this = *this - (T)val;
			return *this;
		}
		
		Vector4<T>& operator *=(T val)
		{
			*this = *this * (T)val;
			return *this;
		}
		
		Vector4<T>& operator /=(T val)
		{
			*this = *this / (T)val;
			return *this;
		}
		
		// Probably doesn't belong here, but I'll leave it in for the moment.
		void SetColor(u32 color)
		{
			x = (color & 0xff) / 255.0f;
			y = ((color >> 8) & 0xff) / 255.0f;
			z = ((color >> 16) & 0xff) / 255.0f;
		}
};

typedef Vector4<float> float4;

#endif
