/* 
 *	Copyright (C) 2003-2005 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

//
// GSSoftVertexFP
//

__declspec(align(16)) union GSSoftVertexFP
{
	class __declspec(novtable) Scalar
	{
		float val;

	public:
		Scalar() {}
		explicit Scalar(float f) {val = f;}
		explicit Scalar(int i) {val = (float)i;}

		float Value() const {return val;}

#if _M_IX86_FP >= 2 || defined(_M_AMD64)
		void sat() {_mm_store_ss(&val, _mm_min_ss(_mm_max_ss(_mm_set_ss(val), _mm_setzero_ps()), _mm_set_ss(255)));}
		void rcp() {_mm_store_ss(&val, _mm_rcp_ss(_mm_set_ss(val)));}
#else
		void sat() {val = val < 0 ? 0 : val > 255 ? 255 : val;}
		void rcp() {val = 1.0f / val;}
#endif

		void abss() {val = abs(val);}

		Scalar floors() const {return Scalar(floor(val));}
		int floori() const {return (int)floor(val);}

		Scalar ceils() const {return Scalar(-floor(-val));}
		int ceili() const {return -(int)floor(-val);}

		void operator = (float f) {val = f;}
		void operator = (int i) {val = (float)i;}

		operator float() const {return val;}
		operator int() const {return (int)val;}

		void operator += (const Scalar& s) {val += s.val;}
		void operator -= (const Scalar& s) {val -= s.val;}
		void operator *= (const Scalar& s) {val *= s.val;}
		void operator /= (const Scalar& s) {val /= s.val;}

		friend Scalar operator + (const Scalar& s1, const Scalar& s2) {return Scalar(s1.val + s2.val);}
		friend Scalar operator - (const Scalar& s1, const Scalar& s2) {return Scalar(s1.val - s2.val);}
		friend Scalar operator * (const Scalar& s1, const Scalar& s2) {return Scalar(s1.val * s2.val);}
		friend Scalar operator / (const Scalar& s1, const Scalar& s2) {return Scalar(s1.val / s2.val);}

		friend Scalar operator + (const Scalar& s1, int i) {return Scalar(s1.val + i);}
		friend Scalar operator - (const Scalar& s1, int i) {return Scalar(s1.val - i);}
		friend Scalar operator * (const Scalar& s1, int i) {return Scalar(s1.val * i);}
		friend Scalar operator / (const Scalar& s1, int i) {return Scalar(s1.val / i);}

		friend bool operator == (const Scalar& s1, const Scalar& s2) {return s1.val == s2.val;}
		friend bool operator <= (const Scalar& s1, const Scalar& s2) {return s1.val <= s2.val;}
		friend bool operator < (const Scalar& s1, const Scalar& s2) {return s1.val < s2.val;}
	};

	__declspec(align(16)) class __declspec(novtable) Vector
	{
	public:
		union
		{
			union {struct {Scalar x, y, z, q;}; struct {Scalar r, g, b, a;};};
			union {struct {Scalar v[4];}; struct {Scalar c[4];};};
#if _M_IX86_FP >= 2 || defined(_M_AMD64)
			union {__m128 xyzq; __m128 rgba;};
#endif
		};

		Vector() {}
		Vector(const Vector& v) {*this = v;}
		Vector(Scalar s) {*this = s;}
		Vector(Scalar s0, Scalar s1, Scalar s2, Scalar s3) {x = s0; y = s1; z = s2; q = s3;}
		explicit Vector(DWORD dw) {*this = dw;}
#if _M_IX86_FP >= 2 || defined(_M_AMD64)
		Vector(__m128 f0123) {*this = f0123;}
#endif

#if _M_IX86_FP >= 2 || defined(_M_AMD64)

		void operator = (const Vector& v) {xyzq = v.xyzq;}
		void operator = (__m128 f0123) {xyzq = f0123;}
		void operator = (Scalar s) {xyzq = _mm_set1_ps(s);}
		void operator = (DWORD dw) {__m128i zero = _mm_setzero_si128(); xyzq = _mm_cvtepi32_ps(_mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(dw), zero), zero));}

		operator __m128() const {return xyzq;}
		operator DWORD() const {__m128i r0 = _mm_cvttps_epi32(xyzq); r0 = _mm_packs_epi32(r0, r0); r0 = _mm_packus_epi16(r0, r0); return (DWORD)_mm_cvtsi128_si32(r0);}

		void sat() {xyzq = _mm_min_ps(_mm_max_ps(xyzq, _mm_setzero_ps()), _mm_set1_ps(255));}
		void rcp() {xyzq = _mm_rcp_ps(xyzq);}

		void operator += (const Vector& v) {xyzq = _mm_add_ps(xyzq, v);}
		void operator -= (const Vector& v) {xyzq = _mm_sub_ps(xyzq, v);}
		void operator *= (const Vector& v) {xyzq = _mm_mul_ps(xyzq, v);}
		void operator /= (const Vector& v) {xyzq = _mm_div_ps(xyzq, v);}

#else

		void operator = (const Vector& v) {x = v.x; y = v.y; z = v.z; q = v.q;}
		void operator = (Scalar s) {x = y = z = q = s;}
		void operator = (DWORD dw)
		{
			x = Scalar((int)(dw&0xff));
			y = Scalar((int)((dw>>8)&0xff));
			z = Scalar((int)((dw>>16)&0xff));
			q = Scalar((int)((dw>>24)&0xff));
		}

		operator DWORD() const
		{
			return (DWORD)((((DWORD)(int)x&0xff)<<0) | (((DWORD)(int)y&0xff)<<8) | (((DWORD)(int)z&0xff)<<16) | (((DWORD)(int)q&0xff)<<24));
		}

		void sat() {x.sat(); y.sat(); z.sat(); q.sat();}
		void rcp() {x.rcp(); y.rcp(); z.rcp(); q.rcp();}

		void operator += (const Vector& v) {*this = *this + v;}
		void operator -= (const Vector& v) {*this = *this - v;}
		void operator *= (const Vector& v) {*this = *this * v;}
		void operator /= (const Vector& v) {*this = *this / v;}

#endif

		void absv() {x.abss(); y.abss(); z.abss(); q.abss();}

		friend Vector operator + (const Vector& v1, const Vector& v2);
		friend Vector operator - (const Vector& v1, const Vector& v2);
		friend Vector operator * (const Vector& v1, const Vector& v2);
		friend Vector operator / (const Vector& v1, const Vector& v2);

		friend Vector operator + (const Vector& v, Scalar s);
		friend Vector operator - (const Vector& v, Scalar s);
		friend Vector operator * (const Vector& v, Scalar s);
		friend Vector operator / (const Vector& v, Scalar s);
	};

	struct {__declspec(align(16)) Vector c, p, t;};
	struct {__declspec(align(16)) Vector sv[3];};
	struct {__declspec(align(16)) Scalar s[12];};

	GSSoftVertexFP() {}
	GSSoftVertexFP(const GSSoftVertexFP& v) {*this = v;}

	void operator = (const GSSoftVertexFP& v) {c = v.c; p = v.p; t = v.t;}
	void operator += (const GSSoftVertexFP& v) {c += v.c; p += v.p; t += v.t;}

	operator CPoint() const {return CPoint((int)p.x, (int)p.y);}

	friend GSSoftVertexFP operator + (const GSSoftVertexFP& v1, const GSSoftVertexFP& v2);
	friend GSSoftVertexFP operator - (const GSSoftVertexFP& v1, const GSSoftVertexFP& v2);

	friend GSSoftVertexFP operator * (const GSSoftVertexFP& v, Scalar s);
	friend GSSoftVertexFP operator / (const GSSoftVertexFP& v, Scalar s);

	static void Exchange(GSSoftVertexFP* __restrict v1, GSSoftVertexFP* __restrict v2)
	{
		// GSSoftVertexFP v = *v1; *v1 = *v2; *v2 = v;
		Vector c = v1->c, p = v1->p, t = v1->t;
		v1->c = v2->c; v1->p = v2->p; v1->t = v2->t;
		v2->c = c; v2->p = p; v2->t = t;
	}

	//__forceinline DWORD GetZ() const {return ((DWORD)p.z<<16) + (DWORD)((p.z - floorf(p.z))*65536 + p.q);}
	__forceinline DWORD GetZ() const 
	{
		ASSERT((float)p.z >= 0 && (float)p.q >= 0); 
		int z = (int)p.z;
		return ((DWORD)z << 16) + (DWORD)(((float)p.z - z)*65536 + (float)p.q);
	}
};

#if _M_IX86_FP >= 2 || defined(_M_AMD64)

__forceinline GSSoftVertexFP::Vector operator + (const GSSoftVertexFP::Vector& v1, const GSSoftVertexFP::Vector& v2) {return GSSoftVertexFP::Vector(_mm_add_ps(v1, v2));}
__forceinline GSSoftVertexFP::Vector operator - (const GSSoftVertexFP::Vector& v1, const GSSoftVertexFP::Vector& v2) {return GSSoftVertexFP::Vector(_mm_sub_ps(v1, v2));}
__forceinline GSSoftVertexFP::Vector operator * (const GSSoftVertexFP::Vector& v1, const GSSoftVertexFP::Vector& v2) {return GSSoftVertexFP::Vector(_mm_mul_ps(v1, v2));}
__forceinline GSSoftVertexFP::Vector operator / (const GSSoftVertexFP::Vector& v1, const GSSoftVertexFP::Vector& v2) {return GSSoftVertexFP::Vector(_mm_div_ps(v1, v2));}

__forceinline GSSoftVertexFP::Vector operator + (const GSSoftVertexFP::Vector& v, GSSoftVertexFP::Scalar s) {return GSSoftVertexFP::Vector(_mm_add_ps(v, _mm_set1_ps(s)));}
__forceinline GSSoftVertexFP::Vector operator - (const GSSoftVertexFP::Vector& v, GSSoftVertexFP::Scalar s) {return GSSoftVertexFP::Vector(_mm_sub_ps(v, _mm_set1_ps(s)));}
__forceinline GSSoftVertexFP::Vector operator * (const GSSoftVertexFP::Vector& v, GSSoftVertexFP::Scalar s) {return GSSoftVertexFP::Vector(_mm_mul_ps(v, _mm_set1_ps(s)));}
__forceinline GSSoftVertexFP::Vector operator / (const GSSoftVertexFP::Vector& v, GSSoftVertexFP::Scalar s) {return GSSoftVertexFP::Vector(_mm_div_ps(v, _mm_set1_ps(s)));}

#else

__forceinline GSSoftVertexFP::Vector operator + (const GSSoftVertexFP::Vector& v1, const GSSoftVertexFP::Vector& v2) {return GSSoftVertexFP::Vector(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.q + v2.q);}
__forceinline GSSoftVertexFP::Vector operator - (const GSSoftVertexFP::Vector& v1, const GSSoftVertexFP::Vector& v2) {return GSSoftVertexFP::Vector(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.q - v2.q);}
__forceinline GSSoftVertexFP::Vector operator * (const GSSoftVertexFP::Vector& v1, const GSSoftVertexFP::Vector& v2) {return GSSoftVertexFP::Vector(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.q * v2.q);}
__forceinline GSSoftVertexFP::Vector operator / (const GSSoftVertexFP::Vector& v1, const GSSoftVertexFP::Vector& v2) {return GSSoftVertexFP::Vector(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.q / v2.q);}

__forceinline GSSoftVertexFP::Vector operator + (const GSSoftVertexFP::Vector& v, GSSoftVertexFP::Scalar s) {return GSSoftVertexFP::Vector(v.x + s, v.y + s, v.z + s, v.q + s);}
__forceinline GSSoftVertexFP::Vector operator - (const GSSoftVertexFP::Vector& v, GSSoftVertexFP::Scalar s) {return GSSoftVertexFP::Vector(v.x - s, v.y - s, v.z - s, v.q - s);}
__forceinline GSSoftVertexFP::Vector operator * (const GSSoftVertexFP::Vector& v, GSSoftVertexFP::Scalar s) {return GSSoftVertexFP::Vector(v.x * s, v.y * s, v.z * s, v.q * s);}
__forceinline GSSoftVertexFP::Vector operator / (const GSSoftVertexFP::Vector& v, GSSoftVertexFP::Scalar s) {return GSSoftVertexFP::Vector(v.x / s, v.y / s, v.z / s, v.q / s);}

#endif

__forceinline GSSoftVertexFP operator + (const GSSoftVertexFP& v1, const GSSoftVertexFP& v2)
{
	GSSoftVertexFP v0;
	v0.c = v1.c + v2.c;
	v0.p = v1.p + v2.p;
	v0.t = v1.t + v2.t;
	return v0;
}

__forceinline GSSoftVertexFP operator - (const GSSoftVertexFP& v1, const GSSoftVertexFP& v2)
{
	GSSoftVertexFP v0;
	v0.c = v1.c - v2.c;
	v0.p = v1.p - v2.p;
	v0.t = v1.t - v2.t;
	return v0;
}

__forceinline GSSoftVertexFP operator * (const GSSoftVertexFP& v, GSSoftVertexFP::Scalar s)
{
	GSSoftVertexFP v0;
	GSSoftVertexFP::Vector vs(s);
	v0.c = v.c * vs;
	v0.p = v.p * vs;
	v0.t = v.t * vs;
	return v0;
}

__forceinline GSSoftVertexFP operator / (const GSSoftVertexFP& v, GSSoftVertexFP::Scalar s)
{
	GSSoftVertexFP v0;
	GSSoftVertexFP::Vector vs(s);
	v0.c = v.c / vs;
	v0.p = v.p / vs;
	v0.t = v.t / vs;
	return v0;
}
