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
/*
inline int fixmul(int x, int y)
{
#ifndef _M_AMD64
	__asm
	{
		mov		eax, x
		imul	y
		add		eax, 0x8000
		adc		edx, 0
		shrd	eax, edx, 16
	}
#else
	return (int)(((__int64)x * y + 0x8000) >> 16);
#endif
}
*/
inline int fixmul(int x, int y)
{
#ifndef _M_AMD64
	_asm
	{
		mov eax, x
		imul y
		shrd eax, edx, 16
		sar edx, 15
		jz Out2
		cmp edx, -1
		jz Out2
		; call _set_errno_erange
		mov eax, 0x7FFFFFFF
		cmp x, 0
		jge Out1
		neg eax
	Out1:
		cmp y, 0
		jge Out2
		neg eax
	Out2:
	}
#else
	return (int)(((__int64)x * y + 0x8000) >> 16);
#endif
}

inline int fixdiv(int x, int y)
{
#ifndef _M_AMD64
	_asm
	{
		push esi

		mov ecx, y
		xor esi, esi
		mov eax, x
		or eax, eax
		jns Out1
		neg eax
		inc esi
	Out1:
		or ecx, ecx
		jns Out2
		neg ecx
		inc esi
	Out2:
		mov edx, eax
		shr edx, 0x10
		shl eax, 0x10
		cmp edx, ecx
		jae Out3
		div ecx
		or eax, eax
		jns Out4
	Out3:
		; call _set_errno_erange
		mov eax, 0x7FFFFFFF
	Out4:
		test esi, 1
		je Out5
		neg eax
	Out5:
		pop esi
	}
#else
	return (int)(((__int64)x << 16) / y);
#endif
}

//
// GSSoftVertexFX
//

__declspec(align(16)) union GSSoftVertexFX
{
	class __declspec(novtable) Scalar
	{
		int val;

	public:
		Scalar() {}
		explicit Scalar(float f) {val = (int)(f*65536);}
		explicit Scalar(int i) {val = i;}

		int Value() const {return val;}

#if _M_IX86_FP >= 2 || defined(_M_AMD64)
		void sat() {ASSERT(0); val = val < 0 ? 0 : val > (255<<16) ? (255<<16) : val;} // TODO
		void rcp() {val = (int)(65536.0f*65536.0f / val);} // TODO
#else
		void sat() {val = val < 0 ? 0 : val > (255<<16) ? (255<<16) : val;}
		void rcp() {val = (int)(65536.0f*65536.0f / val);}
#endif
		void abss() {val = abs(val);}

		Scalar floors() const {Scalar s; s.val = val & 0xffff0000; return s;}
		int floori() const {return val >> 16;}

		Scalar ceils() const {Scalar s; s.val = (val + 0xffff) & 0xffff0000; return s;}
		int ceili() const {return (val + 0xffff) >> 16;}

		void operator = (float f) {val = (int)(f*65536);}
		void operator = (int i) {val = i;}

		operator float() const {return (float)val / 65536;}
		operator int() const {return val >> 16;}

		void operator += (const Scalar& s) {val += s.val;}
		void operator -= (const Scalar& s) {val -= s.val;}
		void operator *= (const Scalar& s) {val = fixmul(val, s.val);}
		void operator /= (const Scalar& s) {val = fixdiv(val, s.val);}

		friend Scalar operator + (const Scalar& s1, const Scalar& s2) {return Scalar(s1.val + s2.val);}
		friend Scalar operator - (const Scalar& s1, const Scalar& s2) {return Scalar(s1.val - s2.val);}
		friend Scalar operator * (const Scalar& s1, const Scalar& s2) {return Scalar(fixmul(s1.val, s2.val));}
		friend Scalar operator / (const Scalar& s1, const Scalar& s2) {return Scalar(fixdiv(s1.val, s2.val));}

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
			union {__m128i xyzq; __m128i rgba;};
#endif
		};

		Vector() {}
		Vector(const Vector& v) {*this = v;}
		Vector(Scalar s) {*this = s;}
		Vector(Scalar s0, Scalar s1, Scalar s2, Scalar s3) {x = s0; y = s1; z = s2; q = s3;}
		explicit Vector(DWORD dw) {*this = dw;}
#if _M_IX86_FP >= 2 || defined(_M_AMD64)
		Vector(__m128i i0123) {*this = i0123;}
#endif

#if _M_IX86_FP >= 2 || defined(_M_AMD64)

		void operator = (const Vector& v) {xyzq = v.xyzq;}
		void operator = (__m128i i0123) {xyzq = i0123;}
		void operator = (Scalar s) {xyzq = _mm_set1_epi32(s.Value());}
		void operator = (DWORD dw)
		{
			__m128i zero = _mm_setzero_si128();
			xyzq = _mm_unpacklo_epi16(zero, _mm_unpacklo_epi8(_mm_cvtsi32_si128(dw), zero));
		}

		operator __m128i() const {return xyzq;}
		operator DWORD() const
		{
			__m128i r0 = xyzq;
			r0 = _mm_srli_si128(r0, 2);
			r0 = _mm_packs_epi32(r0, r0);
			r0 = _mm_packus_epi16(r0, r0);
			return (DWORD)_mm_cvtsi128_si32(r0);
		}

		void sat() {xyzq = _mm_and_si128(_mm_min_epi16(_mm_max_epi16(xyzq, _mm_setzero_si128()), _mm_set1_epi32(0x00ff0000)), _mm_set1_epi32(0xffff0000));}
		void rcp() {ASSERT(0);} // TODO {xyzq = _mm_rcp_ps(xyzq);}

#else

		void operator = (const Vector& v) {x = v.x; y = v.y; z = v.z; q = v.q;}
		void operator = (Scalar s) {x = y = z = q = s;}
		void operator = (DWORD dw)
		{
			x = (int)((dw & 0x000000ff) << 16);
			y = (int)((dw & 0x0000ff00) << 8);
			z = (int)((dw & 0x00ff0000) << 0);
			q = (int)((dw & 0xff000000) >> 8);
		}

		operator DWORD() const
		{
			return (DWORD)((((DWORD)(int)x&0xff)<<0) | (((DWORD)(int)y&0xff)<<8) | (((DWORD)(int)z&0xff)<<16) | (((DWORD)(int)q&0xff)<<24));
		}

		void sat() {x.sat(); y.sat(); z.sat(); q.sat();}
		void rcp() {x.rcp(); y.rcp(); z.rcp(); q.rcp();}

#endif
		// TODO
		void operator += (const Vector& v) {*this = *this + v;}
		void operator -= (const Vector& v) {*this = *this - v;}
		void operator *= (const Vector& v) {*this = *this * v;}
		void operator /= (const Vector& v) {*this = *this / v;}

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

	GSSoftVertexFX() {}
	GSSoftVertexFX(const GSSoftVertexFX& v) {*this = v;}

	void operator = (const GSSoftVertexFX& v) {c = v.c; p = v.p; t = v.t;}
	void operator += (const GSSoftVertexFX& v) {c += v.c; p += v.p; t += v.t;}

	operator CPoint() const {return CPoint((int)p.x, (int)p.y);}

	friend GSSoftVertexFX operator + (const GSSoftVertexFX& v1, const GSSoftVertexFX& v2);
	friend GSSoftVertexFX operator - (const GSSoftVertexFX& v1, const GSSoftVertexFX& v2);

	friend GSSoftVertexFX operator * (const GSSoftVertexFX& v, Scalar s);
	friend GSSoftVertexFX operator / (const GSSoftVertexFX& v, Scalar s);

	static void Exchange(GSSoftVertexFX* __restrict v1, GSSoftVertexFX* __restrict v2)
	{
		// GSSoftVertexFX v = *v1; *v1 = *v2; *v2 = v;
		Vector c = v1->c, p = v1->p, t = v1->t;
		v1->c = v2->c; v1->p = v2->p; v1->t = v2->t;
		v2->c = c; v2->p = p; v2->t = t;
	}

	__forceinline DWORD GetZ() const 
	{
		ASSERT((int)p.z >= 0 && (int)p.q >= 0); 
		return ((DWORD)(int)p.z << 1) + ((DWORD)(int)p.q >> 15);
	}
};

#if _M_IX86_FP >= 2 || defined(_M_AMD64)

__forceinline GSSoftVertexFX::Vector operator + (const GSSoftVertexFX::Vector& v1, const GSSoftVertexFX::Vector& v2) {return GSSoftVertexFX::Vector(_mm_add_epi32(v1, v2));}
__forceinline GSSoftVertexFX::Vector operator - (const GSSoftVertexFX::Vector& v1, const GSSoftVertexFX::Vector& v2) {return GSSoftVertexFX::Vector(_mm_sub_epi32(v1, v2));}
__forceinline GSSoftVertexFX::Vector operator * (const GSSoftVertexFX::Vector& v1, const GSSoftVertexFX::Vector& v2) 
{
	// TODO
	__m128 r1 = _mm_mul_ps(_mm_cvtepi32_ps(v1), _mm_set1_ps(1.0f/65536));
	__m128 r2 = _mm_cvtepi32_ps(v2);
	__m128i r0 = _mm_cvtps_epi32(_mm_mul_ps(r1, r2));
	return GSSoftVertexFX::Vector(r0);
}
__forceinline GSSoftVertexFX::Vector operator / (const GSSoftVertexFX::Vector& v1, const GSSoftVertexFX::Vector& v2)
{
	// TODO
	__m128 r1 = _mm_mul_ps(_mm_cvtepi32_ps(v1), _mm_set1_ps(65536));
	__m128 r2 = _mm_cvtepi32_ps(v2);
	__m128i r0 = _mm_cvtps_epi32(_mm_div_ps(r1, r2));
	return GSSoftVertexFX::Vector(r0);
}

__forceinline GSSoftVertexFX::Vector operator + (const GSSoftVertexFX::Vector& v, const GSSoftVertexFX::Scalar s) {return GSSoftVertexFX::Vector(_mm_add_epi32(v, _mm_set1_epi32(s.Value())));}
__forceinline GSSoftVertexFX::Vector operator - (const GSSoftVertexFX::Vector& v, const GSSoftVertexFX::Scalar s) {return GSSoftVertexFX::Vector(_mm_sub_epi32(v, _mm_set1_epi32(s.Value())));}
__forceinline GSSoftVertexFX::Vector operator * (const GSSoftVertexFX::Vector& v, const GSSoftVertexFX::Scalar s) 
{
	// TODO
	__m128 r1 = _mm_mul_ps(_mm_cvtepi32_ps(v), _mm_set1_ps(1.0f/65536));
	__m128 r2 = _mm_cvtepi32_ps(_mm_set1_epi32(s.Value()));
	__m128i r0 = _mm_cvtps_epi32(_mm_mul_ps(r1, r2));
	return GSSoftVertexFX::Vector(r0);
}
__forceinline GSSoftVertexFX::Vector operator / (const GSSoftVertexFX::Vector& v, const GSSoftVertexFX::Scalar s)
{
	// TODO
	__m128 r1 = _mm_mul_ps(_mm_cvtepi32_ps(v), _mm_set1_ps(65536));
	__m128 r2 = _mm_cvtepi32_ps(_mm_set1_epi32(s.Value()));
	__m128i r0 = _mm_cvtps_epi32(_mm_div_ps(r1, r2));
	return GSSoftVertexFX::Vector(r0);
}

#else

__forceinline GSSoftVertexFX::Vector operator + (const GSSoftVertexFX::Vector& v1, const GSSoftVertexFX::Vector& v2) {return GSSoftVertexFX::Vector(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.q + v2.q);}
__forceinline GSSoftVertexFX::Vector operator - (const GSSoftVertexFX::Vector& v1, const GSSoftVertexFX::Vector& v2) {return GSSoftVertexFX::Vector(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.q - v2.q);}
__forceinline GSSoftVertexFX::Vector operator * (const GSSoftVertexFX::Vector& v1, const GSSoftVertexFX::Vector& v2) {return GSSoftVertexFX::Vector(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.q * v2.q);}
__forceinline GSSoftVertexFX::Vector operator / (const GSSoftVertexFX::Vector& v1, const GSSoftVertexFX::Vector& v2) {return GSSoftVertexFX::Vector(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.q / v2.q);}

__forceinline GSSoftVertexFX::Vector operator + (const GSSoftVertexFX::Vector& v, GSSoftVertexFX::Scalar s) {return GSSoftVertexFX::Vector(v.x + s, v.y + s, v.z + s, v.q + s);}
__forceinline GSSoftVertexFX::Vector operator - (const GSSoftVertexFX::Vector& v, GSSoftVertexFX::Scalar s) {return GSSoftVertexFX::Vector(v.x - s, v.y - s, v.z - s, v.q - s);}
__forceinline GSSoftVertexFX::Vector operator * (const GSSoftVertexFX::Vector& v, GSSoftVertexFX::Scalar s) {return GSSoftVertexFX::Vector(v.x * s, v.y * s, v.z * s, v.q * s);}
__forceinline GSSoftVertexFX::Vector operator / (const GSSoftVertexFX::Vector& v, GSSoftVertexFX::Scalar s) {return GSSoftVertexFX::Vector(v.x / s, v.y / s, v.z / s, v.q / s);}

#endif

__forceinline GSSoftVertexFX operator + (const GSSoftVertexFX& v1, const GSSoftVertexFX& v2)
{
	GSSoftVertexFX v0;
	v0.c = v1.c + v2.c;
	v0.p = v1.p + v2.p;
	v0.t = v1.t + v2.t;
	return v0;
}

__forceinline GSSoftVertexFX operator - (const GSSoftVertexFX& v1, const GSSoftVertexFX& v2)
{
	GSSoftVertexFX v0;
	v0.c = v1.c - v2.c;
	v0.p = v1.p - v2.p;
	v0.t = v1.t - v2.t;
	return v0;
}

__forceinline GSSoftVertexFX operator * (const GSSoftVertexFX& v, GSSoftVertexFX::Scalar s)
{
	GSSoftVertexFX v0;
	GSSoftVertexFX::Vector vs(s);
	v0.c = v.c * vs;
	v0.p = v.p * vs;
	v0.t = v.t * vs;
	return v0;
}

__forceinline GSSoftVertexFX operator / (const GSSoftVertexFX& v, GSSoftVertexFX::Scalar s)
{
	GSSoftVertexFX v0;
	GSSoftVertexFX::Vector vs(s);
	v0.c = v.c / vs;
	v0.p = v.p / vs;
	v0.t = v.t / vs;
	return v0;
}
