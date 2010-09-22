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

//#define ZZ_MMATH

#ifndef ZZ_MMATH

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

#else

// Reimplement, swiping a bunch of code from GSdx and adapting it. (specifically GSVector.h)
// This doesn't include more then half of the functions in there, as well as some of the structs...
#include <xmmintrin.h>

#include "Pcsx2Types.h"

class float4
{
	public:
		union
		{
			struct {float x, y, z, w;};
			struct {float r, g, b, a;};
			struct {float left, top, right, bottom;};
			float v[4];
			float f32[4];
			s8 _s8[16];
			s16 _s16[8];
			s32 _s32[4];
			s64 _s64[2];
			u8 _u8[16];
			u16 _u16[8];
			u32 _u32[4];
			u64 _u64[2];
			__m128 m;
		};
		
		float4() 
		{ 
			m = _mm_setzero_ps();
		}
		
		float4(float x, float y, float z, float w = 0) 
		{ 
			m = _mm_set_ps(w, z, y, x);
		}
		
		float4(float4 &f) 
		{ 
			m = f.m;
		}

		float4(float x, float y)
		{
			m = _mm_unpacklo_ps(_mm_load_ss(&x), _mm_load_ss(&y));
		}

		float4(int x, int y)
		{
			m = _mm_cvtepi32_ps(_mm_unpacklo_epi32(_mm_cvtsi32_si128(x), _mm_cvtsi32_si128(y)));
		}

		explicit float4(float f)
		{
			m = _mm_set1_ps(f);
		}

		explicit float4(__m128 m)
		{
			this->m = m;
		}

		float4(float* f) 
		{
			x = f[0]; 
			y = f[1]; 
			z = f[2]; 
			w = f[3]; // For some reason, the old code set this to 0. 
		}
		
		float& operator[](int i)
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
		
		operator float*()
		{
			return (float*) this;
		}
		
		operator const float*() const
		{
			return (const float*) this;
		}

		void operator = (float f)
		{
			m = _mm_set1_ps(f);
		}

		void operator = (__m128 m)
		{
			this->m = m;
		}

		
		void operator += (const float4& v)
		{
			m = _mm_add_ps(m, v.m);
		}

		void operator -= (const float4& v)
		{
			m = _mm_sub_ps(m, v.m);
		}

		void operator *= (const float4& v)
		{
			m = _mm_mul_ps(m, v.m);
		}

		void operator /= (const float4& v)
		{
			m = _mm_div_ps(m, v.m);
		}

		void operator += (float f)
		{
			*this += float4(f);
		}

		void operator -= (float f)
		{
			*this -= float4(f);
		}

		void operator *= (float f)
		{
			*this *= float4(f);
		}

		void operator /= (float f)
		{
			*this /= float4(f);
		}

		void operator &= (const float4& v)
		{
			m = _mm_and_ps(m, v.m);
		}

		void operator |= (const float4& v)
		{
			m = _mm_or_ps(m, v.m);
		}

		void operator ^= (const float4& v)
		{
			m = _mm_xor_ps(m, v.m);
		}

		friend float4 operator + (const float4& v1, const float4& v2)
		{
			return float4(_mm_add_ps(v1.m, v2.m));
		}

		friend float4 operator - (const float4& v1, const float4& v2)
		{
			return float4(_mm_sub_ps(v1.m, v2.m));
		}

		friend float4 operator * (const float4& v1, const float4& v2)
		{
			return float4(_mm_mul_ps(v1.m, v2.m));
		}

		friend float4 operator / (const float4& v1, const float4& v2)
		{
			return float4(_mm_div_ps(v1.m, v2.m));
		}

		friend float4 operator + (const float4& v, float f)
		{
			return v + float4(f);
		}

		friend float4 operator - (const float4& v, float f)
		{
			return v - float4(f);
		}

		friend float4 operator * (const float4& v, float f)
		{
			return v * float4(f);
		}

		friend float4 operator / (const float4& v, float f)
		{
			return v / float4(f);
		}

		friend float4 operator & (const float4& v1, const float4& v2)
		{
			return float4(_mm_and_ps(v1.m, v2.m));
		}

		friend float4 operator | (const float4& v1, const float4& v2)
		{
			return float4(_mm_or_ps(v1.m, v2.m));
		}

		friend float4 operator ^ (const float4& v1, const float4& v2)
		{
			return float4(_mm_xor_ps(v1.m, v2.m));
		}

		friend float4 operator == (const float4& v1, const float4& v2)
		{
			return float4(_mm_cmpeq_ps(v1.m, v2.m));
		}

		friend float4 operator != (const float4& v1, const float4& v2)
		{
			return float4(_mm_cmpneq_ps(v1.m, v2.m));
		}

		friend float4 operator > (const float4& v1, const float4& v2)
		{
			return float4(_mm_cmpgt_ps(v1.m, v2.m));
		}

		friend float4 operator < (const float4& v1, const float4& v2)
		{
			return float4(_mm_cmplt_ps(v1.m, v2.m));
		}

		friend float4 operator >= (const float4& v1, const float4& v2)
		{
			return float4(_mm_cmpge_ps(v1.m, v2.m));
		}

		friend float4 operator <= (const float4& v1, const float4& v2)
		{
			return float4(_mm_cmple_ps(v1.m, v2.m));
		}
		
		// This looked interesting, so I thought I'd include it...
		
		template<int i> float4 shuffle() const
		{
			return float4(_mm_shuffle_ps(m, m, _MM_SHUFFLE(i, i, i, i)));
		}

		#define VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, ws, wn) \
			float4 xs##ys##zs##ws() const {return float4(_mm_shuffle_ps(m, m, _MM_SHUFFLE(wn, zn, yn, xn)));} \
			float4 xs##ys##zs##ws(const float4& v) const {return float4(_mm_shuffle_ps(m, v.m, _MM_SHUFFLE(wn, zn, yn, xn)));} \

		#define VECTOR4_SHUFFLE_3(xs, xn, ys, yn, zs, zn) \
			VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, x, 0) \
			VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, y, 1) \
			VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, z, 2) \
			VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, w, 3) \

		#define VECTOR4_SHUFFLE_2(xs, xn, ys, yn) \
			VECTOR4_SHUFFLE_3(xs, xn, ys, yn, x, 0) \
			VECTOR4_SHUFFLE_3(xs, xn, ys, yn, y, 1) \
			VECTOR4_SHUFFLE_3(xs, xn, ys, yn, z, 2) \
			VECTOR4_SHUFFLE_3(xs, xn, ys, yn, w, 3) \

		#define VECTOR4_SHUFFLE_1(xs, xn) \
			float4 xs##4() const {return float4(_mm_shuffle_ps(m, m, _MM_SHUFFLE(xn, xn, xn, xn)));} \
			float4 xs##4(const float4& v) const {return float4(_mm_shuffle_ps(m, v.m, _MM_SHUFFLE(xn, xn, xn, xn)));} \
			VECTOR4_SHUFFLE_2(xs, xn, x, 0) \
			VECTOR4_SHUFFLE_2(xs, xn, y, 1) \
			VECTOR4_SHUFFLE_2(xs, xn, z, 2) \
			VECTOR4_SHUFFLE_2(xs, xn, w, 3) \

		VECTOR4_SHUFFLE_1(x, 0)
		VECTOR4_SHUFFLE_1(y, 1)
		VECTOR4_SHUFFLE_1(z, 2)
		VECTOR4_SHUFFLE_1(w, 3)
	
		// Probably doesn't belong here, but I'll leave it in for the moment.
		void SetColor(u32 color)
		{
			x = (color & 0xff) / 255.0f;
			y = ((color >> 8) & 0xff) / 255.0f;
			z = ((color >> 16) & 0xff) / 255.0f;
		}
};

#endif

#endif
