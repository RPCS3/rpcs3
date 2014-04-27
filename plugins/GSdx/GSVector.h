/*
 *	Copyright (C) 2007-2009 Gabest
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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"

#pragma once

enum Align_Mode
{
	Align_Outside,
	Align_Inside,
	Align_NegInf,
	Align_PosInf
};

enum Round_Mode
{
	Round_NearestInt = 8,
	Round_NegInf = 9,
	Round_PosInf = 10,
	Round_Truncate = 11
};

#pragma pack(push, 1)

template<class T> class GSVector2T
{
public:
	union
	{
		struct {T x, y;};
		struct {T r, g;};
		struct {T v[2];};
	};

	GSVector2T()
	{
	}

	GSVector2T(T x, T y)
	{
		this->x = x;
		this->y = y;
	}

	bool operator == (const GSVector2T& v) const
	{
		return x == v.x && y == v.y;
	}

	bool operator != (const GSVector2T& v) const
	{
		return x != v.x || y != v.y;
	}
};

typedef GSVector2T<float> GSVector2;
typedef GSVector2T<int> GSVector2i;

class GSVector4;
class GSVector4i;

#if _M_SSE >= 0x500

class GSVector8;

#endif

#if _M_SSE >= 0x501

class GSVector8i;

#endif

__aligned(class, 16) GSVector4i
{
	static const GSVector4i m_xff[17];
	static const GSVector4i m_x0f[17];

public:
	union
	{
		struct {int x, y, z, w;};
		struct {int r, g, b, a;};
		struct {int left, top, right, bottom;};
		int v[4];
		float f32[4];
		int8 i8[16];
		int16 i16[8];
		int32 i32[4];
		int64  i64[2];
		uint8 u8[16];
		uint16 u16[8];
		uint32 u32[4];
		uint64 u64[2];
		__m128i m;
	};

	__forceinline GSVector4i()
	{
	}

	__forceinline GSVector4i(int x, int y, int z, int w)
	{
		// 4 gprs

		// m = _mm_set_epi32(w, z, y, x);

		// 2 gprs

		GSVector4i xz = load(x).upl32(load(z));
		GSVector4i yw = load(y).upl32(load(w));

		*this = xz.upl32(yw);
	}

	__forceinline GSVector4i(int x, int y)
	{
		*this = load(x).upl32(load(y));
	}

	__forceinline GSVector4i(short s0, short s1, short s2, short s3, short s4, short s5, short s6, short s7)
	{
		m = _mm_set_epi16(s7, s6, s5, s4, s3, s2, s1, s0);
	}

	__forceinline GSVector4i(char b0, char b1, char b2, char b3, char b4, char b5, char b6, char b7, char b8, char b9, char b10, char b11, char b12, char b13, char b14, char b15)
	{
		m = _mm_set_epi8(b15, b14, b13, b12, b11, b10, b9, b8, b7, b6, b5, b4, b3, b2, b1, b0);
	}

	__forceinline GSVector4i(const GSVector4i& v)
	{
		m = v.m;
	}

	__forceinline explicit GSVector4i(const GSVector2i& v)
	{
		m = _mm_loadl_epi64((__m128i*)&v);
	}

	__forceinline explicit GSVector4i(int i)
	{
		*this = i;
	}

	__forceinline explicit GSVector4i(__m128i m)
	{
		this->m = m;
	}

	__forceinline explicit GSVector4i(const GSVector4& v, bool truncate = true);

	__forceinline static GSVector4i cast(const GSVector4& v);

	#if _M_SSE >= 0x500

	__forceinline static GSVector4i cast(const GSVector8& v);

	#endif

	#if _M_SSE >= 0x501

	__forceinline static GSVector4i cast(const GSVector8i& v);

	#endif

	__forceinline void operator = (const GSVector4i& v)
	{
		m = v.m;
	}

	__forceinline void operator = (int i)
	{
		#if _M_SSE >= 0x501

		m = _mm_broadcastd_epi32(_mm_cvtsi32_si128(i));

		#else

		m = _mm_set1_epi32(i);

		#endif
	}

	__forceinline void operator = (__m128i m)
	{
		this->m = m;
	}

	__forceinline operator __m128i() const
	{
		return m;
	}

	// rect

	__forceinline int width() const
	{
		return right - left;
	}

	__forceinline int height() const
	{
		return bottom - top;
	}

	__forceinline GSVector4i rsize() const
	{
		return *this - xyxy(); // same as GSVector4i(0, 0, width(), height());
	}

	__forceinline bool rempty() const
	{
		return (*this < zwzw()).mask() != 0x00ff;
	}

	__forceinline GSVector4i runion(const GSVector4i& a) const
	{
		int i = (upl64(a) < uph64(a)).mask();

		if(i == 0xffff)
		{
			#if _M_SSE >= 0x401

			return min_i32(a).upl64(max_i32(a).srl<8>());

			#else

			return GSVector4i(min(x, a.x), min(y, a.y), max(z, a.z), max(w, a.w));

			#endif
		}

		if((i & 0x00ff) == 0x00ff)
		{
			return *this;
		}

		if((i & 0xff00) == 0xff00)
		{
			return a;
		}

		return GSVector4i::zero();
	}

	__forceinline GSVector4i rintersect(const GSVector4i& a) const
	{
		return sat_i32(a);
	}

	template<int mode> __forceinline GSVector4i ralign(const GSVector2i& a) const
	{
		// a must be 1 << n

		GSVector4i mask = GSVector4i(a) - GSVector4i(1, 1);

		GSVector4i v;

		switch(mode)
		{
		case Align_Inside: v = *this + mask; break;
		case Align_Outside: v = *this + mask.zwxy(); break;
		case Align_NegInf: v = *this; break;
		case Align_PosInf: v = *this + mask.zwzw(); break;
		default: ASSERT(0); break;
		}

		return v.andnot(mask.xyxy());
	}

	GSVector4i fit(int arx, int ary) const;

	GSVector4i fit(int preset) const;

	#ifdef _WINDOWS

	__forceinline operator LPCRECT() const
	{
		return (LPCRECT)this;
	}

	__forceinline operator LPRECT()
	{
		return (LPRECT)this;
	}

	#endif

	//

	__forceinline uint32 rgba32() const
	{
		GSVector4i v = *this;

		v = v.ps32(v);
		v = v.pu16(v);

		return (uint32)store(v);
	}

	#if _M_SSE >= 0x401

	__forceinline GSVector4i sat_i8(const GSVector4i& a, const GSVector4i& b) const
	{
		return max_i8(a).min_i8(b);
	}

	__forceinline GSVector4i sat_i8(const GSVector4i& a) const
	{
		return max_i8(a.xyxy()).min_i8(a.zwzw());
	}

	#endif

	__forceinline GSVector4i sat_i16(const GSVector4i& a, const GSVector4i& b) const
	{
		return max_i16(a).min_i16(b);
	}

	__forceinline GSVector4i sat_i16(const GSVector4i& a) const
	{
		return max_i16(a.xyxy()).min_i16(a.zwzw());
	}

	#if _M_SSE >= 0x401

	__forceinline GSVector4i sat_i32(const GSVector4i& a, const GSVector4i& b) const
	{
		return max_i32(a).min_i32(b);
	}

	__forceinline GSVector4i sat_i32(const GSVector4i& a) const
	{
		return max_i32(a.xyxy()).min_i32(a.zwzw());
	}

	#else

	__forceinline GSVector4i sat_i32(const GSVector4i& a, const GSVector4i& b) const
	{
		GSVector4i v;

		v.x = min(max(x, a.x), b.x);
		v.y = min(max(y, a.y), b.y);
		v.z = min(max(z, a.z), b.z);
		v.w = min(max(w, a.w), b.w);

		return v;
	}

	__forceinline GSVector4i sat_i32(const GSVector4i& a) const
	{
		GSVector4i v;

		v.x = min(max(x, a.x), a.z);
		v.y = min(max(y, a.y), a.w);
		v.z = min(max(z, a.x), a.z);
		v.w = min(max(w, a.y), a.w);

		return v;
	}

	#endif

	__forceinline GSVector4i sat_u8(const GSVector4i& a, const GSVector4i& b) const
	{
		return max_u8(a).min_u8(b);
	}

	__forceinline GSVector4i sat_u8(const GSVector4i& a) const
	{
		return max_u8(a.xyxy()).min_u8(a.zwzw());
	}

	#if _M_SSE >= 0x401

	__forceinline GSVector4i sat_u16(const GSVector4i& a, const GSVector4i& b) const
	{
		return max_u16(a).min_u16(b);
	}

	__forceinline GSVector4i sat_u16(const GSVector4i& a) const
	{
		return max_u16(a.xyxy()).min_u16(a.zwzw());
	}

	#endif

	#if _M_SSE >= 0x401

	__forceinline GSVector4i sat_u32(const GSVector4i& a, const GSVector4i& b) const
	{
		return max_u32(a).min_u32(b);
	}

	__forceinline GSVector4i sat_u32(const GSVector4i& a) const
	{
		return max_u32(a.xyxy()).min_u32(a.zwzw());
	}

	#endif

	#if _M_SSE >= 0x401

	__forceinline GSVector4i min_i8(const GSVector4i& a) const
	{
		return GSVector4i(_mm_min_epi8(m, a));
	}

	__forceinline GSVector4i max_i8(const GSVector4i& a) const
	{
		return GSVector4i(_mm_max_epi8(m, a));
	}

	#endif

	__forceinline GSVector4i min_i16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_min_epi16(m, a));
	}

	__forceinline GSVector4i max_i16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_max_epi16(m, a));
	}

	#if _M_SSE >= 0x401

	__forceinline GSVector4i min_i32(const GSVector4i& a) const
	{
		return GSVector4i(_mm_min_epi32(m, a));
	}

	__forceinline GSVector4i max_i32(const GSVector4i& a) const
	{
		return GSVector4i(_mm_max_epi32(m, a));
	}

	#endif

	__forceinline GSVector4i min_u8(const GSVector4i& a) const
	{
		return GSVector4i(_mm_min_epu8(m, a));
	}

	__forceinline GSVector4i max_u8(const GSVector4i& a) const
	{
		return GSVector4i(_mm_max_epu8(m, a));
	}

	#if _M_SSE >= 0x401

	__forceinline GSVector4i min_u16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_min_epu16(m, a));
	}

	__forceinline GSVector4i max_u16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_max_epu16(m, a));
	}

	__forceinline GSVector4i min_u32(const GSVector4i& a) const
	{
		return GSVector4i(_mm_min_epu32(m, a));
	}

	__forceinline GSVector4i max_u32(const GSVector4i& a) const
	{
		return GSVector4i(_mm_max_epu32(m, a));
	}

	#endif

	__forceinline static int min_i16(int a, int b)
	{
		 return store(load(a).min_i16(load(b)));
	}

	__forceinline GSVector4i clamp8() const
	{
		return pu16().upl8();
	}

	__forceinline GSVector4i blend8(const GSVector4i& a, const GSVector4i& mask) const
	{
		#if _M_SSE >= 0x401

		return GSVector4i(_mm_blendv_epi8(m, a, mask));

		#else

		return GSVector4i(_mm_or_si128(_mm_andnot_si128(mask, m), _mm_and_si128(mask, a)));

		#endif
	}

	#if _M_SSE >= 0x401

	template<int mask> __forceinline GSVector4i blend16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_blend_epi16(m, a, mask));
	}

	#endif

	#if _M_SSE >= 0x501

	template<int mask> __forceinline GSVector4i blend32(const GSVector4i& v) const
	{
		return GSVector4i(_mm_blend_epi32(m, v.m, mask));
	}

	#endif

	__forceinline GSVector4i blend(const GSVector4i& a, const GSVector4i& mask) const
	{
		return GSVector4i(_mm_or_si128(_mm_andnot_si128(mask, m), _mm_and_si128(mask, a)));
	}

	__forceinline GSVector4i mix16(const GSVector4i& a) const
	{
		#if _M_SSE >= 0x401

		return blend16<0xaa>(a);

		#else

		return blend8(a, GSVector4i::xffff0000());

		#endif
	}

	#if _M_SSE >= 0x301

	__forceinline GSVector4i shuffle8(const GSVector4i& mask) const
	{
		return GSVector4i(_mm_shuffle_epi8(m, mask));
	}

	#endif

	__forceinline GSVector4i ps16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_packs_epi16(m, a));
	}

	__forceinline GSVector4i ps16() const
	{
		return GSVector4i(_mm_packs_epi16(m, m));
	}

	__forceinline GSVector4i pu16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_packus_epi16(m, a));
	}

	__forceinline GSVector4i pu16() const
	{
		return GSVector4i(_mm_packus_epi16(m, m));
	}

	__forceinline GSVector4i ps32(const GSVector4i& a) const
	{
		return GSVector4i(_mm_packs_epi32(m, a));
	}

	__forceinline GSVector4i ps32() const
	{
		return GSVector4i(_mm_packs_epi32(m, m));
	}

	#if _M_SSE >= 0x401

	__forceinline GSVector4i pu32(const GSVector4i& a) const
	{
		return GSVector4i(_mm_packus_epi32(m, a));
	}

	__forceinline GSVector4i pu32() const
	{
		return GSVector4i(_mm_packus_epi32(m, m));
	}

	#endif

	__forceinline GSVector4i upl8(const GSVector4i& a) const
	{
		return GSVector4i(_mm_unpacklo_epi8(m, a));
	}

	__forceinline GSVector4i uph8(const GSVector4i& a) const
	{
		return GSVector4i(_mm_unpackhi_epi8(m, a));
	}

	__forceinline GSVector4i upl16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_unpacklo_epi16(m, a));
	}

	__forceinline GSVector4i uph16(const GSVector4i& a) const
	{
		return GSVector4i(_mm_unpackhi_epi16(m, a));
	}

	__forceinline GSVector4i upl32(const GSVector4i& a) const
	{
		return GSVector4i(_mm_unpacklo_epi32(m, a));
	}

	__forceinline GSVector4i uph32(const GSVector4i& a) const
	{
		return GSVector4i(_mm_unpackhi_epi32(m, a));
	}

	__forceinline GSVector4i upl64(const GSVector4i& a) const
	{
		return GSVector4i(_mm_unpacklo_epi64(m, a));
	}

	__forceinline GSVector4i uph64(const GSVector4i& a) const
	{
		return GSVector4i(_mm_unpackhi_epi64(m, a));
	}

	__forceinline GSVector4i upl8() const
	{
		#if 0 // _M_SSE >= 0x401 // TODO: compiler bug

		return GSVector4i(_mm_cvtepu8_epi16(m));

		#else

		return GSVector4i(_mm_unpacklo_epi8(m, _mm_setzero_si128()));

		#endif
	}

	__forceinline GSVector4i uph8() const
	{
		return GSVector4i(_mm_unpackhi_epi8(m, _mm_setzero_si128()));
	}

	__forceinline GSVector4i upl16() const
	{
		#if 0 //_M_SSE >= 0x401 // TODO: compiler bug

		return GSVector4i(_mm_cvtepu16_epi32(m));

		#else

		return GSVector4i(_mm_unpacklo_epi16(m, _mm_setzero_si128()));

		#endif
	}

	__forceinline GSVector4i uph16() const
	{
		return GSVector4i(_mm_unpackhi_epi16(m, _mm_setzero_si128()));
	}

	__forceinline GSVector4i upl32() const
	{
		#if 0 //_M_SSE >= 0x401 // TODO: compiler bug

		return GSVector4i(_mm_cvtepu32_epi64(m));

		#else

		return GSVector4i(_mm_unpacklo_epi32(m, _mm_setzero_si128()));

		#endif
	}

	__forceinline GSVector4i uph32() const
	{
		return GSVector4i(_mm_unpackhi_epi32(m, _mm_setzero_si128()));
	}

	__forceinline GSVector4i upl64() const
	{
		return GSVector4i(_mm_unpacklo_epi64(m, _mm_setzero_si128()));
	}

	__forceinline GSVector4i uph64() const
	{
		return GSVector4i(_mm_unpackhi_epi64(m, _mm_setzero_si128()));
	}

	#if _M_SSE >= 0x401

	// WARNING!!!
	//
	// MSVC (2008, 2010 ctp) believes that there is a "mem, reg" form of the pmovz/sx* instructions,
	// turning these intrinsics into a minefield, don't spill regs when using them...

	__forceinline GSVector4i i8to16() const
	{
		return GSVector4i(_mm_cvtepi8_epi16(m));
	}

	__forceinline GSVector4i u8to16() const
	{
		return GSVector4i(_mm_cvtepu8_epi16(m));
	}

	__forceinline GSVector4i i8to32() const
	{
		return GSVector4i(_mm_cvtepi8_epi32(m));
	}

	__forceinline GSVector4i u8to32() const
	{
		return GSVector4i(_mm_cvtepu8_epi32(m));
	}

	__forceinline GSVector4i i8to64() const
	{
		return GSVector4i(_mm_cvtepi8_epi64(m));
	}

	__forceinline GSVector4i u8to64() const
	{
		return GSVector4i(_mm_cvtepu16_epi64(m));
	}

	__forceinline GSVector4i i16to32() const
	{
		return GSVector4i(_mm_cvtepi16_epi32(m));
	}

	__forceinline GSVector4i u16to32() const
	{
		return GSVector4i(_mm_cvtepu16_epi32(m));
	}

	__forceinline GSVector4i i16to64() const
	{
		return GSVector4i(_mm_cvtepi16_epi64(m));
	}

	__forceinline GSVector4i u16to64() const
	{
		return GSVector4i(_mm_cvtepu16_epi64(m));
	}

	__forceinline GSVector4i i32to64() const
	{
		return GSVector4i(_mm_cvtepi32_epi64(m));
	}

	__forceinline GSVector4i u32to64() const
	{
		return GSVector4i(_mm_cvtepu32_epi64(m));
	}

	#else

	__forceinline GSVector4i u8to16() const
	{
		return upl8();
	}

	__forceinline GSVector4i u8to32() const
	{
		return upl8().upl16();
	}

	__forceinline GSVector4i u8to64() const
	{
		return upl8().upl16().upl32();
	}

	__forceinline GSVector4i u16to32() const
	{
		return upl16();
	}

	__forceinline GSVector4i u16to64() const
	{
		return upl16().upl32();
	}

	__forceinline GSVector4i u32to64() const
	{
		return upl32();
	}

	__forceinline GSVector4i i8to16() const
	{
		return zero().upl8(*this).sra16(8);
	}

	__forceinline GSVector4i i16to32() const
	{
		return zero().upl16(*this).sra32(16);
	}

	#endif

	template<int i> __forceinline GSVector4i srl() const
	{
		return GSVector4i(_mm_srli_si128(m, i));
	}

	template<int i> __forceinline GSVector4i srl(const GSVector4i& v)
	{
		#if _M_SSE >= 0x301

		return GSVector4i(_mm_alignr_epi8(v.m, m, i));

		#else

		if(i == 0) return *this;
		else if(i < 16) return srl<i>() | v.sll<16 - i>();
		else if(i == 16) return v;
		else if(i < 32) return v.srl<i - 16>();
		else return zero();

		#endif
	}

	template<int i> __forceinline GSVector4i sll() const
	{
		return GSVector4i(_mm_slli_si128(m, i));
	}

	__forceinline GSVector4i sra16(int i) const
	{
		return GSVector4i(_mm_srai_epi16(m, i));
	}

	__forceinline GSVector4i sra16(__m128i i) const
	{
		return GSVector4i(_mm_sra_epi16(m, i));
	}

	__forceinline GSVector4i sra32(int i) const
	{
		return GSVector4i(_mm_srai_epi32(m, i));
	}

	__forceinline GSVector4i sra32(__m128i i) const
	{
		return GSVector4i(_mm_sra_epi32(m, i));
	}

	__forceinline GSVector4i sll16(int i) const
	{
		return GSVector4i(_mm_slli_epi16(m, i));
	}

	__forceinline GSVector4i sll16(__m128i i) const
	{
		return GSVector4i(_mm_sll_epi16(m, i));
	}

	__forceinline GSVector4i sll32(int i) const
	{
		return GSVector4i(_mm_slli_epi32(m, i));
	}

	__forceinline GSVector4i sll32(__m128i i) const
	{
		return GSVector4i(_mm_sll_epi32(m, i));
	}

	__forceinline GSVector4i sll64(int i) const
	{
		return GSVector4i(_mm_slli_epi64(m, i));
	}

	__forceinline GSVector4i sll64(__m128i i) const
	{
		return GSVector4i(_mm_sll_epi64(m, i));
	}

	__forceinline GSVector4i srl16(int i) const
	{
		return GSVector4i(_mm_srli_epi16(m, i));
	}

	__forceinline GSVector4i srl16(__m128i i) const
	{
		return GSVector4i(_mm_srl_epi16(m, i));
	}

	__forceinline GSVector4i srl32(int i) const
	{
		return GSVector4i(_mm_srli_epi32(m, i));
	}

	__forceinline GSVector4i srl32(__m128i i) const
	{
		return GSVector4i(_mm_srl_epi32(m, i));
	}

	__forceinline GSVector4i srl64(int i) const
	{
		return GSVector4i(_mm_srli_epi64(m, i));
	}

	__forceinline GSVector4i srl64(__m128i i) const
	{
		return GSVector4i(_mm_srl_epi64(m, i));
	}

	__forceinline GSVector4i add8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_add_epi8(m, v.m));
	}

	__forceinline GSVector4i add16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_add_epi16(m, v.m));
	}

	__forceinline GSVector4i add32(const GSVector4i& v) const
	{
		return GSVector4i(_mm_add_epi32(m, v.m));
	}

	__forceinline GSVector4i adds8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_adds_epi8(m, v.m));
	}

	__forceinline GSVector4i adds16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_adds_epi16(m, v.m));
	}

	__forceinline GSVector4i addus8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_adds_epu8(m, v.m));
	}

	__forceinline GSVector4i addus16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_adds_epu16(m, v.m));
	}

	__forceinline GSVector4i sub8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_sub_epi8(m, v.m));
	}

	__forceinline GSVector4i sub16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_sub_epi16(m, v.m));
	}

	__forceinline GSVector4i sub32(const GSVector4i& v) const
	{
		return GSVector4i(_mm_sub_epi32(m, v.m));
	}

	__forceinline GSVector4i subs8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_subs_epi8(m, v.m));
	}

	__forceinline GSVector4i subs16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_subs_epi16(m, v.m));
	}

	__forceinline GSVector4i subus8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_subs_epu8(m, v.m));
	}

	__forceinline GSVector4i subus16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_subs_epu16(m, v.m));
	}

	__forceinline GSVector4i avg8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_avg_epu8(m, v.m));
	}

	__forceinline GSVector4i avg16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_avg_epu16(m, v.m));
	}

	__forceinline GSVector4i mul16hs(const GSVector4i& v) const
	{
		return GSVector4i(_mm_mulhi_epi16(m, v.m));
	}

	__forceinline GSVector4i mul16hu(const GSVector4i& v) const
	{
		return GSVector4i(_mm_mulhi_epu16(m, v.m));
	}

	__forceinline GSVector4i mul16l(const GSVector4i& v) const
	{
		return GSVector4i(_mm_mullo_epi16(m, v.m));
	}

	#if _M_SSE >= 0x301

	__forceinline GSVector4i mul16hrs(const GSVector4i& v) const
	{
		return GSVector4i(_mm_mulhrs_epi16(m, v.m));
	}

	#endif

	GSVector4i madd(const GSVector4i& v) const
	{
		return GSVector4i(_mm_madd_epi16(m, v.m));
	}

	template<int shift> __forceinline GSVector4i lerp16(const GSVector4i& a, const GSVector4i& f) const
	{
		// (a - this) * f << shift + this

		return add16(a.sub16(*this).modulate16<shift>(f));
	}

	template<int shift> __forceinline static GSVector4i lerp16(const GSVector4i& a, const GSVector4i& b, const GSVector4i& c)
	{
		// (a - b) * c << shift

		return a.sub16(b).modulate16<shift>(c);
	}

	template<int shift> __forceinline static GSVector4i lerp16(const GSVector4i& a, const GSVector4i& b, const GSVector4i& c, const GSVector4i& d)
	{
		// (a - b) * c << shift + d

		return d.add16(a.sub16(b).modulate16<shift>(c));
	}

	__forceinline GSVector4i lerp16_4(const GSVector4i& a, const GSVector4i& f) const
	{
		// (a - this) * f >> 4 + this (a, this: 8-bit, f: 4-bit)

		return add16(a.sub16(*this).mul16l(f).sra16(4));
	}

	template<int shift> __forceinline GSVector4i modulate16(const GSVector4i& f) const
	{
		// a * f << shift

		#if _M_SSE >= 0x301

		if(shift == 0)
		{
			return mul16hrs(f);
		}

		#endif

		return sll16(shift + 1).mul16hs(f);
	}

	__forceinline bool eq(const GSVector4i& v) const
	{
		#if _M_SSE >= 0x401
		
		// pxor, ptest, je
		
		GSVector4i t = *this ^ v;
		
		return _mm_testz_si128(t, t) != 0;

		#else

		// pcmpeqd, pmovmskb, cmp, je

		return eq32(v).alltrue();

		#endif
	}

	__forceinline GSVector4i eq8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmpeq_epi8(m, v.m));
	}

	__forceinline GSVector4i eq16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmpeq_epi16(m, v.m));
	}

	__forceinline GSVector4i eq32(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmpeq_epi32(m, v.m));
	}

	__forceinline GSVector4i neq8(const GSVector4i& v) const
	{
		return ~eq8(v);
	}

	__forceinline GSVector4i neq16(const GSVector4i& v) const
	{
		return ~eq16(v);
	}

	__forceinline GSVector4i neq32(const GSVector4i& v) const
	{
		return ~eq32(v);
	}

	__forceinline GSVector4i gt8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmpgt_epi8(m, v.m));
	}

	__forceinline GSVector4i gt16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmpgt_epi16(m, v.m));
	}

	__forceinline GSVector4i gt32(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmpgt_epi32(m, v.m));
	}

	__forceinline GSVector4i lt8(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmplt_epi8(m, v.m));
	}

	__forceinline GSVector4i lt16(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmplt_epi16(m, v.m));
	}

	__forceinline GSVector4i lt32(const GSVector4i& v) const
	{
		return GSVector4i(_mm_cmplt_epi32(m, v.m));
	}

	__forceinline GSVector4i andnot(const GSVector4i& v) const
	{
		return GSVector4i(_mm_andnot_si128(v.m, m));
	}

	__forceinline int mask() const
	{
		return _mm_movemask_epi8(m);
	}

	__forceinline bool alltrue() const
	{
		return mask() == 0xffff;
	}

	__forceinline bool allfalse() const
	{
		#if _M_SSE >= 0x401

		return _mm_testz_si128(m, m) != 0;

		#else

		return mask() == 0;

		#endif
	}

	#if _M_SSE >= 0x401

	template<int i> __forceinline GSVector4i insert8(int a) const
	{
		return GSVector4i(_mm_insert_epi8(m, a, i));
	}

	#endif

	template<int i> __forceinline int extract8() const
	{
		#if _M_SSE >= 0x401

		return _mm_extract_epi8(m, i);

		#else

		return (int)u8[i];

		#endif
	}

	template<int i> __forceinline GSVector4i insert16(int a) const
	{
		return GSVector4i(_mm_insert_epi16(m, a, i));
	}

	template<int i> __forceinline int extract16() const
	{
		return _mm_extract_epi16(m, i);
	}

	#if _M_SSE >= 0x401

	template<int i> __forceinline GSVector4i insert32(int a) const
	{
		return GSVector4i(_mm_insert_epi32(m, a, i));
	}

	#endif

	template<int i> __forceinline int extract32() const
	{
		if(i == 0) return GSVector4i::store(*this);

		#if _M_SSE >= 0x401

		return _mm_extract_epi32(m, i);

		#else

		return i32[i];

		#endif
	}

	#ifdef _M_AMD64

	#if _M_SSE >= 0x401

	template<int i> __forceinline GSVector4i insert64(int64 a) const
	{
		return GSVector4i(_mm_insert_epi64(m, a, i));
	}

	#endif

	template<int i> __forceinline int64 extract64() const
	{
		if(i == 0) return GSVector4i::storeq(*this);

		#if _M_SSE >= 0x401

		return _mm_extract_epi64(m, i);

		#else

		return i64[i];

		#endif
	}

	#endif

	#if _M_SSE >= 0x401

	template<int src, class T> __forceinline GSVector4i gather8_4(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract8<src + 0>() & 0xf]);
		v = v.insert8<1>((int)ptr[extract8<src + 0>() >> 4]);
		v = v.insert8<2>((int)ptr[extract8<src + 1>() & 0xf]);
		v = v.insert8<3>((int)ptr[extract8<src + 1>() >> 4]);
		v = v.insert8<4>((int)ptr[extract8<src + 2>() & 0xf]);
		v = v.insert8<5>((int)ptr[extract8<src + 2>() >> 4]);
		v = v.insert8<6>((int)ptr[extract8<src + 3>() & 0xf]);
		v = v.insert8<7>((int)ptr[extract8<src + 3>() >> 4]);
		v = v.insert8<8>((int)ptr[extract8<src + 4>() & 0xf]);
		v = v.insert8<9>((int)ptr[extract8<src + 4>() >> 4]);
		v = v.insert8<10>((int)ptr[extract8<src + 5>() & 0xf]);
		v = v.insert8<11>((int)ptr[extract8<src + 5>() >> 4]);
		v = v.insert8<12>((int)ptr[extract8<src + 6>() & 0xf]);
		v = v.insert8<13>((int)ptr[extract8<src + 6>() >> 4]);
		v = v.insert8<14>((int)ptr[extract8<src + 7>() & 0xf]);
		v = v.insert8<15>((int)ptr[extract8<src + 7>() >> 4]);

		return v;
	}

	template<class T> __forceinline GSVector4i gather8_8(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract8<0>()]);
		v = v.insert8<1>((int)ptr[extract8<1>()]);
		v = v.insert8<2>((int)ptr[extract8<2>()]);
		v = v.insert8<3>((int)ptr[extract8<3>()]);
		v = v.insert8<4>((int)ptr[extract8<4>()]);
		v = v.insert8<5>((int)ptr[extract8<5>()]);
		v = v.insert8<6>((int)ptr[extract8<6>()]);
		v = v.insert8<7>((int)ptr[extract8<7>()]);
		v = v.insert8<8>((int)ptr[extract8<8>()]);
		v = v.insert8<9>((int)ptr[extract8<9>()]);
		v = v.insert8<10>((int)ptr[extract8<10>()]);
		v = v.insert8<11>((int)ptr[extract8<11>()]);
		v = v.insert8<12>((int)ptr[extract8<12>()]);
		v = v.insert8<13>((int)ptr[extract8<13>()]);
		v = v.insert8<14>((int)ptr[extract8<14>()]);
		v = v.insert8<15>((int)ptr[extract8<15>()]);

		return v;
	}

	template<int dst, class T> __forceinline GSVector4i gather8_16(const T* ptr, const GSVector4i& a) const
	{
		GSVector4i v = a;

		v = v.insert8<dst + 0>((int)ptr[extract16<0>()]);
		v = v.insert8<dst + 1>((int)ptr[extract16<1>()]);
		v = v.insert8<dst + 2>((int)ptr[extract16<2>()]);
		v = v.insert8<dst + 3>((int)ptr[extract16<3>()]);
		v = v.insert8<dst + 4>((int)ptr[extract16<4>()]);
		v = v.insert8<dst + 5>((int)ptr[extract16<5>()]);
		v = v.insert8<dst + 6>((int)ptr[extract16<6>()]);
		v = v.insert8<dst + 7>((int)ptr[extract16<7>()]);

		return v;
	}

	template<int dst, class T> __forceinline GSVector4i gather8_32(const T* ptr, const GSVector4i& a) const
	{
		GSVector4i v = a;

		v = v.insert8<dst + 0>((int)ptr[extract32<0>()]);
		v = v.insert8<dst + 1>((int)ptr[extract32<1>()]);
		v = v.insert8<dst + 2>((int)ptr[extract32<2>()]);
		v = v.insert8<dst + 3>((int)ptr[extract32<3>()]);

		return v;
	}

	#endif

	template<int src, class T> __forceinline GSVector4i gather16_4(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract8<src + 0>() & 0xf]);
		v = v.insert16<1>((int)ptr[extract8<src + 0>() >> 4]);
		v = v.insert16<2>((int)ptr[extract8<src + 1>() & 0xf]);
		v = v.insert16<3>((int)ptr[extract8<src + 1>() >> 4]);
		v = v.insert16<4>((int)ptr[extract8<src + 2>() & 0xf]);
		v = v.insert16<5>((int)ptr[extract8<src + 2>() >> 4]);
		v = v.insert16<6>((int)ptr[extract8<src + 3>() & 0xf]);
		v = v.insert16<7>((int)ptr[extract8<src + 3>() >> 4]);

		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather16_8(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract8<src + 0>()]);
		v = v.insert16<1>((int)ptr[extract8<src + 1>()]);
		v = v.insert16<2>((int)ptr[extract8<src + 2>()]);
		v = v.insert16<3>((int)ptr[extract8<src + 3>()]);
		v = v.insert16<4>((int)ptr[extract8<src + 4>()]);
		v = v.insert16<5>((int)ptr[extract8<src + 5>()]);
		v = v.insert16<6>((int)ptr[extract8<src + 6>()]);
		v = v.insert16<7>((int)ptr[extract8<src + 7>()]);

		return v;
	}

	template<class T>__forceinline GSVector4i gather16_16(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract16<0>()]);
		v = v.insert16<1>((int)ptr[extract16<1>()]);
		v = v.insert16<2>((int)ptr[extract16<2>()]);
		v = v.insert16<3>((int)ptr[extract16<3>()]);
		v = v.insert16<4>((int)ptr[extract16<4>()]);
		v = v.insert16<5>((int)ptr[extract16<5>()]);
		v = v.insert16<6>((int)ptr[extract16<6>()]);
		v = v.insert16<7>((int)ptr[extract16<7>()]);

		return v;
	}

	template<class T1, class T2>__forceinline GSVector4i gather16_16(const T1* ptr1, const T2* ptr2) const
	{
		GSVector4i v;

		v = load((int)ptr2[ptr1[extract16<0>()]]);
		v = v.insert16<1>((int)ptr2[ptr1[extract16<1>()]]);
		v = v.insert16<2>((int)ptr2[ptr1[extract16<2>()]]);
		v = v.insert16<3>((int)ptr2[ptr1[extract16<3>()]]);
		v = v.insert16<4>((int)ptr2[ptr1[extract16<4>()]]);
		v = v.insert16<5>((int)ptr2[ptr1[extract16<5>()]]);
		v = v.insert16<6>((int)ptr2[ptr1[extract16<6>()]]);
		v = v.insert16<7>((int)ptr2[ptr1[extract16<7>()]]);

		return v;
	}

	template<int dst, class T> __forceinline GSVector4i gather16_32(const T* ptr, const GSVector4i& a) const
	{
		GSVector4i v = a;

		v = v.insert16<dst + 0>((int)ptr[extract32<0>()]);
		v = v.insert16<dst + 1>((int)ptr[extract32<1>()]);
		v = v.insert16<dst + 2>((int)ptr[extract32<2>()]);
		v = v.insert16<dst + 3>((int)ptr[extract32<3>()]);

		return v;
	}

	#if _M_SSE >= 0x401

	template<int src, class T> __forceinline GSVector4i gather32_4(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract8<src + 0>() & 0xf]);
		v = v.insert32<1>((int)ptr[extract8<src + 0>() >> 4]);
		v = v.insert32<2>((int)ptr[extract8<src + 1>() & 0xf]);
		v = v.insert32<3>((int)ptr[extract8<src + 1>() >> 4]);
		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather32_8(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract8<src + 0>()]);
		v = v.insert32<1>((int)ptr[extract8<src + 1>()]);
		v = v.insert32<2>((int)ptr[extract8<src + 2>()]);
		v = v.insert32<3>((int)ptr[extract8<src + 3>()]);

		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather32_16(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract16<src + 0>()]);
		v = v.insert32<1>((int)ptr[extract16<src + 1>()]);
		v = v.insert32<2>((int)ptr[extract16<src + 2>()]);
		v = v.insert32<3>((int)ptr[extract16<src + 3>()]);

		return v;
	}

	template<class T> __forceinline GSVector4i gather32_32(const T* ptr) const
	{
		GSVector4i v;

		v = load((int)ptr[extract32<0>()]);
		v = v.insert32<1>((int)ptr[extract32<1>()]);
		v = v.insert32<2>((int)ptr[extract32<2>()]);
		v = v.insert32<3>((int)ptr[extract32<3>()]);

		return v;
	}

	template<class T1, class T2> __forceinline GSVector4i gather32_32(const T1* ptr1, const T2* ptr2) const
	{
		GSVector4i v;

		v = load((int)ptr2[ptr1[extract32<0>()]]);
		v = v.insert32<1>((int)ptr2[ptr1[extract32<1>()]]);
		v = v.insert32<2>((int)ptr2[ptr1[extract32<2>()]]);
		v = v.insert32<3>((int)ptr2[ptr1[extract32<3>()]]);

		return v;
	}

	#else

	template<int src, class T> __forceinline GSVector4i gather32_4(const T* ptr) const
	{
		return GSVector4i(
			(int)ptr[extract8<src + 0>() & 0xf],
			(int)ptr[extract8<src + 0>() >> 4],
			(int)ptr[extract8<src + 1>() & 0xf],
			(int)ptr[extract8<src + 1>() >> 4]);
	}

	template<int src, class T> __forceinline GSVector4i gather32_8(const T* ptr) const
	{
		return GSVector4i(
			(int)ptr[extract8<src + 0>()],
			(int)ptr[extract8<src + 1>()],
			(int)ptr[extract8<src + 2>()],
			(int)ptr[extract8<src + 3>()]);
	}

	template<int src, class T> __forceinline GSVector4i gather32_16(const T* ptr) const
	{
		return GSVector4i(
			(int)ptr[extract16<src + 0>()],
			(int)ptr[extract16<src + 1>()],
			(int)ptr[extract16<src + 2>()],
			(int)ptr[extract16<src + 3>()]);
	}

	template<class T> __forceinline GSVector4i gather32_32(const T* ptr) const
	{
		return GSVector4i(
			(int)ptr[extract32<0>()],
			(int)ptr[extract32<1>()],
			(int)ptr[extract32<2>()],
			(int)ptr[extract32<3>()]);
	}

	template<class T1, class T2> __forceinline GSVector4i gather32_32(const T1* ptr1, const T2* ptr2) const
	{
		return GSVector4i(
			(int)ptr2[ptr1[extract32<0>()]],
			(int)ptr2[ptr1[extract32<1>()]],
			(int)ptr2[ptr1[extract32<2>()]],
			(int)ptr2[ptr1[extract32<3>()]]);
	}

	#endif

	#if defined(_M_AMD64) && _M_SSE >= 0x401

	template<int src, class T> __forceinline GSVector4i gather64_4(const T* ptr) const
	{
		GSVector4i v;

		v = loadq((int64)ptr[extract8<src + 0>() & 0xf]);
		v = v.insert64<1>((int64)ptr[extract8<src + 0>() >> 4]);

		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather64_8(const T* ptr) const
	{
		GSVector4i v;

		v = loadq((int64)ptr[extract8<src + 0>()]);
		v = v.insert64<1>((int64)ptr[extract8<src + 1>()]);

		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather64_16(const T* ptr) const
	{
		GSVector4i v;

		v = loadq((int64)ptr[extract16<src + 0>()]);
		v = v.insert64<1>((int64)ptr[extract16<src + 1>()]);

		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather64_32(const T* ptr) const
	{
		GSVector4i v;

		v = loadq((int64)ptr[extract32<src + 0>()]);
		v = v.insert64<1>((int64)ptr[extract32<src + 1>()]);

		return v;
	}

	template<class T> __forceinline GSVector4i gather64_64(const T* ptr) const
	{
		GSVector4i v;

		v = loadq((int64)ptr[extract64<0>()]);
		v = v.insert64<1>((int64)ptr[extract64<1>()]);

		return v;
	}

	#else

	template<int src, class T> __forceinline GSVector4i gather64_4(const T* ptr) const
	{
		GSVector4i v;

		v = loadu(&ptr[extract8<src + 0>() & 0xf], &ptr[extract8<src + 0>() >> 4]);

		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather64_8(const T* ptr) const
	{
		GSVector4i v;

		v = load(&ptr[extract8<src + 0>()], &ptr[extract8<src + 1>()]);

		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather64_16(const T* ptr) const
	{
		GSVector4i v;

		v = load(&ptr[extract16<src + 0>()], &ptr[extract16<src + 1>()]);

		return v;
	}

	template<int src, class T> __forceinline GSVector4i gather64_32(const T* ptr) const
	{
		GSVector4i v;

		v = load(&ptr[extract32<src + 0>()], &ptr[extract32<src + 1>()]);

		return v;
	}

	#endif

	#if _M_SSE >= 0x401

	template<class T> __forceinline void gather8_4(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather8_4<0>(ptr);
		dst[1] = gather8_4<8>(ptr);
	}

	__forceinline void gather8_8(const uint8* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather8_8<>(ptr);
	}

	#endif

	template<class T> __forceinline void gather16_4(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather16_4<0>(ptr);
		dst[1] = gather16_4<4>(ptr);
		dst[2] = gather16_4<8>(ptr);
		dst[3] = gather16_4<12>(ptr);
	}

	template<class T> __forceinline void gather16_8(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather16_8<0>(ptr);
		dst[1] = gather16_8<8>(ptr);
	}

	template<class T> __forceinline void gather16_16(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather16_16<>(ptr);
	}

	template<class T> __forceinline void gather32_4(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather32_4<0>(ptr);
		dst[1] = gather32_4<2>(ptr);
		dst[2] = gather32_4<4>(ptr);
		dst[3] = gather32_4<6>(ptr);
		dst[4] = gather32_4<8>(ptr);
		dst[5] = gather32_4<10>(ptr);
		dst[6] = gather32_4<12>(ptr);
		dst[7] = gather32_4<14>(ptr);
	}

	template<class T> __forceinline void gather32_8(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather32_8<0>(ptr);
		dst[1] = gather32_8<4>(ptr);
		dst[2] = gather32_8<8>(ptr);
		dst[3] = gather32_8<12>(ptr);
	}

	template<class T> __forceinline void gather32_16(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather32_16<0>(ptr);
		dst[1] = gather32_16<4>(ptr);
	}

	template<class T> __forceinline void gather32_32(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather32_32<>(ptr);
	}

	template<class T> __forceinline void gather64_4(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather64_4<0>(ptr);
		dst[1] = gather64_4<1>(ptr);
		dst[2] = gather64_4<2>(ptr);
		dst[3] = gather64_4<3>(ptr);
		dst[4] = gather64_4<4>(ptr);
		dst[5] = gather64_4<5>(ptr);
		dst[6] = gather64_4<6>(ptr);
		dst[7] = gather64_4<7>(ptr);
		dst[8] = gather64_4<8>(ptr);
		dst[9] = gather64_4<9>(ptr);
		dst[10] = gather64_4<10>(ptr);
		dst[11] = gather64_4<11>(ptr);
		dst[12] = gather64_4<12>(ptr);
		dst[13] = gather64_4<13>(ptr);
		dst[14] = gather64_4<14>(ptr);
		dst[15] = gather64_4<15>(ptr);
	}

	template<class T> __forceinline void gather64_8(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather64_8<0>(ptr);
		dst[1] = gather64_8<2>(ptr);
		dst[2] = gather64_8<4>(ptr);
		dst[3] = gather64_8<6>(ptr);
		dst[4] = gather64_8<8>(ptr);
		dst[5] = gather64_8<10>(ptr);
		dst[6] = gather64_8<12>(ptr);
		dst[7] = gather64_8<14>(ptr);
	}

	template<class T> __forceinline void gather64_16(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather64_16<0>(ptr);
		dst[1] = gather64_16<2>(ptr);
		dst[2] = gather64_16<4>(ptr);
		dst[3] = gather64_16<8>(ptr);
	}

	template<class T> __forceinline void gather64_32(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather64_32<0>(ptr);
		dst[1] = gather64_32<2>(ptr);
	}

	#ifdef _M_AMD64

	template<class T> __forceinline void gather64_64(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
	{
		dst[0] = gather64_64<>(ptr);
	}

	#endif

	__forceinline static GSVector4i loadnt(const void* p)
	{
		#if _M_SSE >= 0x401

		return GSVector4i(_mm_stream_load_si128((__m128i*)p));

		#else

		return GSVector4i(_mm_load_si128((__m128i*)p));

		#endif
	}

	__forceinline static GSVector4i loadl(const void* p)
	{
		return GSVector4i(_mm_loadl_epi64((__m128i*)p));
	}

	__forceinline static GSVector4i loadh(const void* p)
	{
		return GSVector4i(_mm_castps_si128(_mm_loadh_pi(_mm_setzero_ps(), (__m64*)p)));
	}

	__forceinline static GSVector4i loadh(const void* p, const GSVector4i& v)
	{
		return GSVector4i(_mm_castps_si128(_mm_loadh_pi(_mm_castsi128_ps(v.m), (__m64*)p)));
	}

	__forceinline static GSVector4i load(const void* pl, const void* ph)
	{
		return loadh(ph, loadl(pl));
	}
/*
	__forceinline static GSVector4i load(const void* pl, const void* ph)
	{
		__m128i lo = _mm_loadl_epi64((__m128i*)pl);
		__m128i hi = _mm_loadl_epi64((__m128i*)ph);

		return GSVector4i(_mm_unpacklo_epi64(lo, hi));
	}
*/
	template<bool aligned> __forceinline static GSVector4i load(const void* p)
	{
		return GSVector4i(aligned ? _mm_load_si128((__m128i*)p) : _mm_loadu_si128((__m128i*)p));
	}

	__forceinline static GSVector4i load(int i)
	{
		return GSVector4i(_mm_cvtsi32_si128(i));
	}

	#ifdef _M_AMD64

	__forceinline static GSVector4i loadq(int64 i)
	{
		return GSVector4i(_mm_cvtsi64_si128(i));
	}

	#endif

	__forceinline static void storent(void* p, const GSVector4i& v)
	{
		_mm_stream_si128((__m128i*)p, v.m);
	}

	__forceinline static void storel(void* p, const GSVector4i& v)
	{
		_mm_storel_epi64((__m128i*)p, v.m);
	}

	__forceinline static void storeh(void* p, const GSVector4i& v)
	{
		_mm_storeh_pi((__m64*)p, _mm_castsi128_ps(v.m));
	}

	__forceinline static void store(void* pl, void* ph, const GSVector4i& v)
	{
		GSVector4i::storel(pl, v);
		GSVector4i::storeh(ph, v);
	}

	template<bool aligned> __forceinline static void store(void* p, const GSVector4i& v)
	{
		if(aligned) _mm_store_si128((__m128i*)p, v.m);
		else _mm_storeu_si128((__m128i*)p, v.m);
	}

	__forceinline static int store(const GSVector4i& v)
	{
		return _mm_cvtsi128_si32(v.m);
	}

	#ifdef _M_AMD64

	__forceinline static int64 storeq(const GSVector4i& v)
	{
		return _mm_cvtsi128_si64(v.m);
	}

	#endif

	__forceinline static void storent(void* RESTRICT dst, const void* RESTRICT src, size_t size)
	{
		const GSVector4i* s = (const GSVector4i*)src;
		GSVector4i* d = (GSVector4i*)dst;

		if(size == 0) return;

		size_t i = 0;
		size_t j = size >> 6;

		for(; i < j; i++, s += 4, d += 4)
		{
			storent(&d[0], s[0]);
			storent(&d[1], s[1]);
			storent(&d[2], s[2]);
			storent(&d[3], s[3]);
		}

		size &= 63;

		if(size == 0) return;

		memcpy(d, s, size);
	}

	__forceinline static void transpose(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
	{
		_MM_TRANSPOSE4_SI128(a.m, b.m, c.m, d.m);
	}

	__forceinline static void sw4(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
	{
		const __m128i epi32_0f0f0f0f = _mm_set1_epi32(0x0f0f0f0f);

		GSVector4i mask(epi32_0f0f0f0f);

		GSVector4i e = (b << 4).blend(a, mask);
		GSVector4i f = b.blend(a >> 4, mask);
		GSVector4i g = (d << 4).blend(c, mask);
		GSVector4i h = d.blend(c >> 4, mask);

		a = e.upl8(f);
		c = e.uph8(f);
		b = g.upl8(h);
		d = g.uph8(h);
	}

	__forceinline static void sw8(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
	{
		GSVector4i e = a;
		GSVector4i f = c;

		a = e.upl8(b);
		c = e.uph8(b);
		b = f.upl8(d);
		d = f.uph8(d);
	}

	__forceinline static void sw16(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
	{
		GSVector4i e = a;
		GSVector4i f = c;

		a = e.upl16(b);
		c = e.uph16(b);
		b = f.upl16(d);
		d = f.uph16(d);
	}

	__forceinline static void sw16rl(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
	{
		GSVector4i e = a;
		GSVector4i f = c;

		a = b.upl16(e);
		c = e.uph16(b);
		b = d.upl16(f);
		d = f.uph16(d);
	}

	__forceinline static void sw16rh(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
	{
		GSVector4i e = a;
		GSVector4i f = c;

		a = e.upl16(b);
		c = b.uph16(e);
		b = f.upl16(d);
		d = d.uph16(f);
	}

	__forceinline static void sw32(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
	{
		GSVector4i e = a;
		GSVector4i f = c;

		a = e.upl32(b);
		c = e.uph32(b);
		b = f.upl32(d);
		d = f.uph32(d);
	}

	__forceinline static void sw64(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
	{
		GSVector4i e = a;
		GSVector4i f = c;

		a = e.upl64(b);
		c = e.uph64(b);
		b = f.upl64(d);
		d = f.uph64(d);
	}

	__forceinline static bool compare16(const void* dst, const void* src, size_t size)
	{
		ASSERT((size & 15) == 0);

		size >>= 4;

		GSVector4i* s = (GSVector4i*)src;
		GSVector4i* d = (GSVector4i*)dst;

		for(size_t i = 0; i < size; i++)
		{
			if(!d[i].eq(s[i]))
			{
				return false;
			}
		}

		return true;
	}

	__forceinline static bool compare64(const void* dst, const void* src, size_t size)
	{
		ASSERT((size & 63) == 0);

		size >>= 6;

		GSVector4i* s = (GSVector4i*)src;
		GSVector4i* d = (GSVector4i*)dst;

		for(size_t i = 0; i < size; i += 4)
		{
			GSVector4i v0 = (d[i * 4 + 0] == s[i * 4 + 0]);
			GSVector4i v1 = (d[i * 4 + 1] == s[i * 4 + 1]);
			GSVector4i v2 = (d[i * 4 + 2] == s[i * 4 + 2]);
			GSVector4i v3 = (d[i * 4 + 3] == s[i * 4 + 3]);

			v0 = v0 & v1;
			v2 = v2 & v3;

			if(!(v0 & v2).alltrue())
			{
				return false;
			}
		}

		return true;
	}

	__forceinline static bool update(const void* dst, const void* src, size_t size)
	{
		ASSERT((size & 15) == 0);

		size >>= 4;

		GSVector4i* s = (GSVector4i*)src;
		GSVector4i* d = (GSVector4i*)dst;

		GSVector4i v = GSVector4i::xffffffff();

		for(size_t i = 0; i < size; i++)
		{
			v &= d[i] == s[i];

			d[i] = s[i];
		}

		return v.alltrue();
	}

	__forceinline void operator += (const GSVector4i& v)
	{
		m = _mm_add_epi32(m, v);
	}

	__forceinline void operator -= (const GSVector4i& v)
	{
		m = _mm_sub_epi32(m, v);
	}

	__forceinline void operator += (int i)
	{
		*this += GSVector4i(i);
	}

	__forceinline void operator -= (int i)
	{
		*this -= GSVector4i(i);
	}

	__forceinline void operator <<= (const int i)
	{
		m = _mm_slli_epi32(m, i);
	}

	__forceinline void operator >>= (const int i)
	{
		m = _mm_srli_epi32(m, i);
	}

	__forceinline void operator &= (const GSVector4i& v)
	{
		m = _mm_and_si128(m, v);
	}

	__forceinline void operator |= (const GSVector4i& v)
	{
		m = _mm_or_si128(m, v);
	}

	__forceinline void operator ^= (const GSVector4i& v)
	{
		m = _mm_xor_si128(m, v);
	}

	__forceinline friend GSVector4i operator + (const GSVector4i& v1, const GSVector4i& v2)
	{
		return GSVector4i(_mm_add_epi32(v1, v2));
	}

	__forceinline friend GSVector4i operator - (const GSVector4i& v1, const GSVector4i& v2)
	{
		return GSVector4i(_mm_sub_epi32(v1, v2));
	}

	__forceinline friend GSVector4i operator + (const GSVector4i& v, int i)
	{
		return v + GSVector4i(i);
	}

	__forceinline friend GSVector4i operator - (const GSVector4i& v, int i)
	{
		return v - GSVector4i(i);
	}

	__forceinline friend GSVector4i operator << (const GSVector4i& v, const int i)
	{
		return GSVector4i(_mm_slli_epi32(v, i));
	}

	__forceinline friend GSVector4i operator >> (const GSVector4i& v, const int i)
	{
		return GSVector4i(_mm_srli_epi32(v, i));
	}

	__forceinline friend GSVector4i operator & (const GSVector4i& v1, const GSVector4i& v2)
	{
		return GSVector4i(_mm_and_si128(v1, v2));
	}

	__forceinline friend GSVector4i operator | (const GSVector4i& v1, const GSVector4i& v2)
	{
		return GSVector4i(_mm_or_si128(v1, v2));
	}

	__forceinline friend GSVector4i operator ^ (const GSVector4i& v1, const GSVector4i& v2)
	{
		return GSVector4i(_mm_xor_si128(v1, v2));
	}

	__forceinline friend GSVector4i operator & (const GSVector4i& v, int i)
	{
		return v & GSVector4i(i);
	}

	__forceinline friend GSVector4i operator | (const GSVector4i& v, int i)
	{
		return v | GSVector4i(i);
	}

	__forceinline friend GSVector4i operator ^ (const GSVector4i& v, int i)
	{
		return v ^ GSVector4i(i);
	}

	__forceinline friend GSVector4i operator ~ (const GSVector4i& v)
	{
		return v ^ (v == v);
	}

	__forceinline friend GSVector4i operator == (const GSVector4i& v1, const GSVector4i& v2)
	{
		return GSVector4i(_mm_cmpeq_epi32(v1, v2));
	}

	__forceinline friend GSVector4i operator != (const GSVector4i& v1, const GSVector4i& v2)
	{
		return ~(v1 == v2);
	}

	__forceinline friend GSVector4i operator > (const GSVector4i& v1, const GSVector4i& v2)
	{
		return GSVector4i(_mm_cmpgt_epi32(v1, v2));
	}

	__forceinline friend GSVector4i operator < (const GSVector4i& v1, const GSVector4i& v2)
	{
		return GSVector4i(_mm_cmplt_epi32(v1, v2));
	}

	__forceinline friend GSVector4i operator >= (const GSVector4i& v1, const GSVector4i& v2)
	{
		return (v1 > v2) | (v1 == v2);
	}

	__forceinline friend GSVector4i operator <= (const GSVector4i& v1, const GSVector4i& v2)
	{
		return (v1 < v2) | (v1 == v2);
	}

	#define VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, ws, wn) \
		__forceinline GSVector4i xs##ys##zs##ws() const {return GSVector4i(_mm_shuffle_epi32(m, _MM_SHUFFLE(wn, zn, yn, xn)));} \
		__forceinline GSVector4i xs##ys##zs##ws##l() const {return GSVector4i(_mm_shufflelo_epi16(m, _MM_SHUFFLE(wn, zn, yn, xn)));} \
		__forceinline GSVector4i xs##ys##zs##ws##h() const {return GSVector4i(_mm_shufflehi_epi16(m, _MM_SHUFFLE(wn, zn, yn, xn)));} \
		__forceinline GSVector4i xs##ys##zs##ws##lh() const {return GSVector4i(_mm_shufflehi_epi16(_mm_shufflelo_epi16(m, _MM_SHUFFLE(wn, zn, yn, xn)), _MM_SHUFFLE(wn, zn, yn, xn)));} \

	#define VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, zs, zn) \
		VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, x, 0) \
		VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, y, 1) \
		VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, z, 2) \
		VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, w, 3) \

	#define VECTOR4i_SHUFFLE_2(xs, xn, ys, yn) \
		VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, x, 0) \
		VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, y, 1) \
		VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, z, 2) \
		VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, w, 3) \

	#define VECTOR4i_SHUFFLE_1(xs, xn) \
		VECTOR4i_SHUFFLE_2(xs, xn, x, 0) \
		VECTOR4i_SHUFFLE_2(xs, xn, y, 1) \
		VECTOR4i_SHUFFLE_2(xs, xn, z, 2) \
		VECTOR4i_SHUFFLE_2(xs, xn, w, 3) \

	VECTOR4i_SHUFFLE_1(x, 0)
	VECTOR4i_SHUFFLE_1(y, 1)
	VECTOR4i_SHUFFLE_1(z, 2)
	VECTOR4i_SHUFFLE_1(w, 3)

	__forceinline static GSVector4i zero() {return GSVector4i(_mm_setzero_si128());}

	__forceinline static GSVector4i xffffffff() {return zero() == zero();}

	__forceinline static GSVector4i x00000001() {return xffffffff().srl32(31);}
	__forceinline static GSVector4i x00000003() {return xffffffff().srl32(30);}
	__forceinline static GSVector4i x00000007() {return xffffffff().srl32(29);}
	__forceinline static GSVector4i x0000000f() {return xffffffff().srl32(28);}
	__forceinline static GSVector4i x0000001f() {return xffffffff().srl32(27);}
	__forceinline static GSVector4i x0000003f() {return xffffffff().srl32(26);}
	__forceinline static GSVector4i x0000007f() {return xffffffff().srl32(25);}
	__forceinline static GSVector4i x000000ff() {return xffffffff().srl32(24);}
	__forceinline static GSVector4i x000001ff() {return xffffffff().srl32(23);}
	__forceinline static GSVector4i x000003ff() {return xffffffff().srl32(22);}
	__forceinline static GSVector4i x000007ff() {return xffffffff().srl32(21);}
	__forceinline static GSVector4i x00000fff() {return xffffffff().srl32(20);}
	__forceinline static GSVector4i x00001fff() {return xffffffff().srl32(19);}
	__forceinline static GSVector4i x00003fff() {return xffffffff().srl32(18);}
	__forceinline static GSVector4i x00007fff() {return xffffffff().srl32(17);}
	__forceinline static GSVector4i x0000ffff() {return xffffffff().srl32(16);}
	__forceinline static GSVector4i x0001ffff() {return xffffffff().srl32(15);}
	__forceinline static GSVector4i x0003ffff() {return xffffffff().srl32(14);}
	__forceinline static GSVector4i x0007ffff() {return xffffffff().srl32(13);}
	__forceinline static GSVector4i x000fffff() {return xffffffff().srl32(12);}
	__forceinline static GSVector4i x001fffff() {return xffffffff().srl32(11);}
	__forceinline static GSVector4i x003fffff() {return xffffffff().srl32(10);}
	__forceinline static GSVector4i x007fffff() {return xffffffff().srl32( 9);}
	__forceinline static GSVector4i x00ffffff() {return xffffffff().srl32( 8);}
	__forceinline static GSVector4i x01ffffff() {return xffffffff().srl32( 7);}
	__forceinline static GSVector4i x03ffffff() {return xffffffff().srl32( 6);}
	__forceinline static GSVector4i x07ffffff() {return xffffffff().srl32( 5);}
	__forceinline static GSVector4i x0fffffff() {return xffffffff().srl32( 4);}
	__forceinline static GSVector4i x1fffffff() {return xffffffff().srl32( 3);}
	__forceinline static GSVector4i x3fffffff() {return xffffffff().srl32( 2);}
	__forceinline static GSVector4i x7fffffff() {return xffffffff().srl32( 1);}

	__forceinline static GSVector4i x80000000() {return xffffffff().sll32(31);}
	__forceinline static GSVector4i xc0000000() {return xffffffff().sll32(30);}
	__forceinline static GSVector4i xe0000000() {return xffffffff().sll32(29);}
	__forceinline static GSVector4i xf0000000() {return xffffffff().sll32(28);}
	__forceinline static GSVector4i xf8000000() {return xffffffff().sll32(27);}
	__forceinline static GSVector4i xfc000000() {return xffffffff().sll32(26);}
	__forceinline static GSVector4i xfe000000() {return xffffffff().sll32(25);}
	__forceinline static GSVector4i xff000000() {return xffffffff().sll32(24);}
	__forceinline static GSVector4i xff800000() {return xffffffff().sll32(23);}
	__forceinline static GSVector4i xffc00000() {return xffffffff().sll32(22);}
	__forceinline static GSVector4i xffe00000() {return xffffffff().sll32(21);}
	__forceinline static GSVector4i xfff00000() {return xffffffff().sll32(20);}
	__forceinline static GSVector4i xfff80000() {return xffffffff().sll32(19);}
	__forceinline static GSVector4i xfffc0000() {return xffffffff().sll32(18);}
	__forceinline static GSVector4i xfffe0000() {return xffffffff().sll32(17);}
	__forceinline static GSVector4i xffff0000() {return xffffffff().sll32(16);}
	__forceinline static GSVector4i xffff8000() {return xffffffff().sll32(15);}
	__forceinline static GSVector4i xffffc000() {return xffffffff().sll32(14);}
	__forceinline static GSVector4i xffffe000() {return xffffffff().sll32(13);}
	__forceinline static GSVector4i xfffff000() {return xffffffff().sll32(12);}
	__forceinline static GSVector4i xfffff800() {return xffffffff().sll32(11);}
	__forceinline static GSVector4i xfffffc00() {return xffffffff().sll32(10);}
	__forceinline static GSVector4i xfffffe00() {return xffffffff().sll32( 9);}
	__forceinline static GSVector4i xffffff00() {return xffffffff().sll32( 8);}
	__forceinline static GSVector4i xffffff80() {return xffffffff().sll32( 7);}
	__forceinline static GSVector4i xffffffc0() {return xffffffff().sll32( 6);}
	__forceinline static GSVector4i xffffffe0() {return xffffffff().sll32( 5);}
	__forceinline static GSVector4i xfffffff0() {return xffffffff().sll32( 4);}
	__forceinline static GSVector4i xfffffff8() {return xffffffff().sll32( 3);}
	__forceinline static GSVector4i xfffffffc() {return xffffffff().sll32( 2);}
	__forceinline static GSVector4i xfffffffe() {return xffffffff().sll32( 1);}

	__forceinline static GSVector4i x0001() {return xffffffff().srl16(15);}
	__forceinline static GSVector4i x0003() {return xffffffff().srl16(14);}
	__forceinline static GSVector4i x0007() {return xffffffff().srl16(13);}
	__forceinline static GSVector4i x000f() {return xffffffff().srl16(12);}
	__forceinline static GSVector4i x001f() {return xffffffff().srl16(11);}
	__forceinline static GSVector4i x003f() {return xffffffff().srl16(10);}
	__forceinline static GSVector4i x007f() {return xffffffff().srl16( 9);}
	__forceinline static GSVector4i x00ff() {return xffffffff().srl16( 8);}
	__forceinline static GSVector4i x01ff() {return xffffffff().srl16( 7);}
	__forceinline static GSVector4i x03ff() {return xffffffff().srl16( 6);}
	__forceinline static GSVector4i x07ff() {return xffffffff().srl16( 5);}
	__forceinline static GSVector4i x0fff() {return xffffffff().srl16( 4);}
	__forceinline static GSVector4i x1fff() {return xffffffff().srl16( 3);}
	__forceinline static GSVector4i x3fff() {return xffffffff().srl16( 2);}
	__forceinline static GSVector4i x7fff() {return xffffffff().srl16( 1);}

	__forceinline static GSVector4i x8000() {return xffffffff().sll16(15);}
	__forceinline static GSVector4i xc000() {return xffffffff().sll16(14);}
	__forceinline static GSVector4i xe000() {return xffffffff().sll16(13);}
	__forceinline static GSVector4i xf000() {return xffffffff().sll16(12);}
	__forceinline static GSVector4i xf800() {return xffffffff().sll16(11);}
	__forceinline static GSVector4i xfc00() {return xffffffff().sll16(10);}
	__forceinline static GSVector4i xfe00() {return xffffffff().sll16( 9);}
	__forceinline static GSVector4i xff00() {return xffffffff().sll16( 8);}
	__forceinline static GSVector4i xff80() {return xffffffff().sll16( 7);}
	__forceinline static GSVector4i xffc0() {return xffffffff().sll16( 6);}
	__forceinline static GSVector4i xffe0() {return xffffffff().sll16( 5);}
	__forceinline static GSVector4i xfff0() {return xffffffff().sll16( 4);}
	__forceinline static GSVector4i xfff8() {return xffffffff().sll16( 3);}
	__forceinline static GSVector4i xfffc() {return xffffffff().sll16( 2);}
	__forceinline static GSVector4i xfffe() {return xffffffff().sll16( 1);}

	__forceinline static GSVector4i xffffffff(const GSVector4i& v) {return v == v;}

	__forceinline static GSVector4i x00000001(const GSVector4i& v) {return xffffffff(v).srl32(31);}
	__forceinline static GSVector4i x00000003(const GSVector4i& v) {return xffffffff(v).srl32(30);}
	__forceinline static GSVector4i x00000007(const GSVector4i& v) {return xffffffff(v).srl32(29);}
	__forceinline static GSVector4i x0000000f(const GSVector4i& v) {return xffffffff(v).srl32(28);}
	__forceinline static GSVector4i x0000001f(const GSVector4i& v) {return xffffffff(v).srl32(27);}
	__forceinline static GSVector4i x0000003f(const GSVector4i& v) {return xffffffff(v).srl32(26);}
	__forceinline static GSVector4i x0000007f(const GSVector4i& v) {return xffffffff(v).srl32(25);}
	__forceinline static GSVector4i x000000ff(const GSVector4i& v) {return xffffffff(v).srl32(24);}
	__forceinline static GSVector4i x000001ff(const GSVector4i& v) {return xffffffff(v).srl32(23);}
	__forceinline static GSVector4i x000003ff(const GSVector4i& v) {return xffffffff(v).srl32(22);}
	__forceinline static GSVector4i x000007ff(const GSVector4i& v) {return xffffffff(v).srl32(21);}
	__forceinline static GSVector4i x00000fff(const GSVector4i& v) {return xffffffff(v).srl32(20);}
	__forceinline static GSVector4i x00001fff(const GSVector4i& v) {return xffffffff(v).srl32(19);}
	__forceinline static GSVector4i x00003fff(const GSVector4i& v) {return xffffffff(v).srl32(18);}
	__forceinline static GSVector4i x00007fff(const GSVector4i& v) {return xffffffff(v).srl32(17);}
	__forceinline static GSVector4i x0000ffff(const GSVector4i& v) {return xffffffff(v).srl32(16);}
	__forceinline static GSVector4i x0001ffff(const GSVector4i& v) {return xffffffff(v).srl32(15);}
	__forceinline static GSVector4i x0003ffff(const GSVector4i& v) {return xffffffff(v).srl32(14);}
	__forceinline static GSVector4i x0007ffff(const GSVector4i& v) {return xffffffff(v).srl32(13);}
	__forceinline static GSVector4i x000fffff(const GSVector4i& v) {return xffffffff(v).srl32(12);}
	__forceinline static GSVector4i x001fffff(const GSVector4i& v) {return xffffffff(v).srl32(11);}
	__forceinline static GSVector4i x003fffff(const GSVector4i& v) {return xffffffff(v).srl32(10);}
	__forceinline static GSVector4i x007fffff(const GSVector4i& v) {return xffffffff(v).srl32( 9);}
	__forceinline static GSVector4i x00ffffff(const GSVector4i& v) {return xffffffff(v).srl32( 8);}
	__forceinline static GSVector4i x01ffffff(const GSVector4i& v) {return xffffffff(v).srl32( 7);}
	__forceinline static GSVector4i x03ffffff(const GSVector4i& v) {return xffffffff(v).srl32( 6);}
	__forceinline static GSVector4i x07ffffff(const GSVector4i& v) {return xffffffff(v).srl32( 5);}
	__forceinline static GSVector4i x0fffffff(const GSVector4i& v) {return xffffffff(v).srl32( 4);}
	__forceinline static GSVector4i x1fffffff(const GSVector4i& v) {return xffffffff(v).srl32( 3);}
	__forceinline static GSVector4i x3fffffff(const GSVector4i& v) {return xffffffff(v).srl32( 2);}
	__forceinline static GSVector4i x7fffffff(const GSVector4i& v) {return xffffffff(v).srl32( 1);}

	__forceinline static GSVector4i x80000000(const GSVector4i& v) {return xffffffff(v).sll32(31);}
	__forceinline static GSVector4i xc0000000(const GSVector4i& v) {return xffffffff(v).sll32(30);}
	__forceinline static GSVector4i xe0000000(const GSVector4i& v) {return xffffffff(v).sll32(29);}
	__forceinline static GSVector4i xf0000000(const GSVector4i& v) {return xffffffff(v).sll32(28);}
	__forceinline static GSVector4i xf8000000(const GSVector4i& v) {return xffffffff(v).sll32(27);}
	__forceinline static GSVector4i xfc000000(const GSVector4i& v) {return xffffffff(v).sll32(26);}
	__forceinline static GSVector4i xfe000000(const GSVector4i& v) {return xffffffff(v).sll32(25);}
	__forceinline static GSVector4i xff000000(const GSVector4i& v) {return xffffffff(v).sll32(24);}
	__forceinline static GSVector4i xff800000(const GSVector4i& v) {return xffffffff(v).sll32(23);}
	__forceinline static GSVector4i xffc00000(const GSVector4i& v) {return xffffffff(v).sll32(22);}
	__forceinline static GSVector4i xffe00000(const GSVector4i& v) {return xffffffff(v).sll32(21);}
	__forceinline static GSVector4i xfff00000(const GSVector4i& v) {return xffffffff(v).sll32(20);}
	__forceinline static GSVector4i xfff80000(const GSVector4i& v) {return xffffffff(v).sll32(19);}
	__forceinline static GSVector4i xfffc0000(const GSVector4i& v) {return xffffffff(v).sll32(18);}
	__forceinline static GSVector4i xfffe0000(const GSVector4i& v) {return xffffffff(v).sll32(17);}
	__forceinline static GSVector4i xffff0000(const GSVector4i& v) {return xffffffff(v).sll32(16);}
	__forceinline static GSVector4i xffff8000(const GSVector4i& v) {return xffffffff(v).sll32(15);}
	__forceinline static GSVector4i xffffc000(const GSVector4i& v) {return xffffffff(v).sll32(14);}
	__forceinline static GSVector4i xffffe000(const GSVector4i& v) {return xffffffff(v).sll32(13);}
	__forceinline static GSVector4i xfffff000(const GSVector4i& v) {return xffffffff(v).sll32(12);}
	__forceinline static GSVector4i xfffff800(const GSVector4i& v) {return xffffffff(v).sll32(11);}
	__forceinline static GSVector4i xfffffc00(const GSVector4i& v) {return xffffffff(v).sll32(10);}
	__forceinline static GSVector4i xfffffe00(const GSVector4i& v) {return xffffffff(v).sll32( 9);}
	__forceinline static GSVector4i xffffff00(const GSVector4i& v) {return xffffffff(v).sll32( 8);}
	__forceinline static GSVector4i xffffff80(const GSVector4i& v) {return xffffffff(v).sll32( 7);}
	__forceinline static GSVector4i xffffffc0(const GSVector4i& v) {return xffffffff(v).sll32( 6);}
	__forceinline static GSVector4i xffffffe0(const GSVector4i& v) {return xffffffff(v).sll32( 5);}
	__forceinline static GSVector4i xfffffff0(const GSVector4i& v) {return xffffffff(v).sll32( 4);}
	__forceinline static GSVector4i xfffffff8(const GSVector4i& v) {return xffffffff(v).sll32( 3);}
	__forceinline static GSVector4i xfffffffc(const GSVector4i& v) {return xffffffff(v).sll32( 2);}
	__forceinline static GSVector4i xfffffffe(const GSVector4i& v) {return xffffffff(v).sll32( 1);}

	__forceinline static GSVector4i x0001(const GSVector4i& v) {return xffffffff(v).srl16(15);}
	__forceinline static GSVector4i x0003(const GSVector4i& v) {return xffffffff(v).srl16(14);}
	__forceinline static GSVector4i x0007(const GSVector4i& v) {return xffffffff(v).srl16(13);}
	__forceinline static GSVector4i x000f(const GSVector4i& v) {return xffffffff(v).srl16(12);}
	__forceinline static GSVector4i x001f(const GSVector4i& v) {return xffffffff(v).srl16(11);}
	__forceinline static GSVector4i x003f(const GSVector4i& v) {return xffffffff(v).srl16(10);}
	__forceinline static GSVector4i x007f(const GSVector4i& v) {return xffffffff(v).srl16( 9);}
	__forceinline static GSVector4i x00ff(const GSVector4i& v) {return xffffffff(v).srl16( 8);}
	__forceinline static GSVector4i x01ff(const GSVector4i& v) {return xffffffff(v).srl16( 7);}
	__forceinline static GSVector4i x03ff(const GSVector4i& v) {return xffffffff(v).srl16( 6);}
	__forceinline static GSVector4i x07ff(const GSVector4i& v) {return xffffffff(v).srl16( 5);}
	__forceinline static GSVector4i x0fff(const GSVector4i& v) {return xffffffff(v).srl16( 4);}
	__forceinline static GSVector4i x1fff(const GSVector4i& v) {return xffffffff(v).srl16( 3);}
	__forceinline static GSVector4i x3fff(const GSVector4i& v) {return xffffffff(v).srl16( 2);}
	__forceinline static GSVector4i x7fff(const GSVector4i& v) {return xffffffff(v).srl16( 1);}

	__forceinline static GSVector4i x8000(const GSVector4i& v) {return xffffffff(v).sll16(15);}
	__forceinline static GSVector4i xc000(const GSVector4i& v) {return xffffffff(v).sll16(14);}
	__forceinline static GSVector4i xe000(const GSVector4i& v) {return xffffffff(v).sll16(13);}
	__forceinline static GSVector4i xf000(const GSVector4i& v) {return xffffffff(v).sll16(12);}
	__forceinline static GSVector4i xf800(const GSVector4i& v) {return xffffffff(v).sll16(11);}
	__forceinline static GSVector4i xfc00(const GSVector4i& v) {return xffffffff(v).sll16(10);}
	__forceinline static GSVector4i xfe00(const GSVector4i& v) {return xffffffff(v).sll16( 9);}
	__forceinline static GSVector4i xff00(const GSVector4i& v) {return xffffffff(v).sll16( 8);}
	__forceinline static GSVector4i xff80(const GSVector4i& v) {return xffffffff(v).sll16( 7);}
	__forceinline static GSVector4i xffc0(const GSVector4i& v) {return xffffffff(v).sll16( 6);}
	__forceinline static GSVector4i xffe0(const GSVector4i& v) {return xffffffff(v).sll16( 5);}
	__forceinline static GSVector4i xfff0(const GSVector4i& v) {return xffffffff(v).sll16( 4);}
	__forceinline static GSVector4i xfff8(const GSVector4i& v) {return xffffffff(v).sll16( 3);}
	__forceinline static GSVector4i xfffc(const GSVector4i& v) {return xffffffff(v).sll16( 2);}
	__forceinline static GSVector4i xfffe(const GSVector4i& v) {return xffffffff(v).sll16( 1);}

	__forceinline static GSVector4i xff(int n) {return m_xff[n];}
	__forceinline static GSVector4i x0f(int n) {return m_x0f[n];}
};

__aligned(class, 16) GSVector4
{
public:
	union
	{
		struct {float x, y, z, w;};
		struct {float r, g, b, a;};
		struct {float left, top, right, bottom;};
		float v[4];
		float f32[4];
		int8 i8[16];
		int16 i16[8];
		int32 i32[4];
		int64 i64[2];
		uint8 u8[16];
		uint16 u16[8];
		uint32 u32[4];
		uint64 u64[2];
		__m128 m;
	};

	static const GSVector4 m_ps0123;
	static const GSVector4 m_ps4567;
	static const GSVector4 m_half;
	static const GSVector4 m_one;
	static const GSVector4 m_two;
	static const GSVector4 m_four;
	static const GSVector4 m_x4b000000;
	static const GSVector4 m_x4f800000;

	__forceinline GSVector4()
	{
	}

	__forceinline GSVector4(float x, float y, float z, float w)
	{
		m = _mm_set_ps(w, z, y, x);
	}

	__forceinline GSVector4(float x, float y)
	{
		m = _mm_unpacklo_ps(_mm_load_ss(&x), _mm_load_ss(&y));
	}

	__forceinline GSVector4(int x, int y, int z, int w)
	{
		GSVector4i v(x, y, z, w);

		m = _mm_cvtepi32_ps(v.m);
	}

	__forceinline GSVector4(int x, int y)
	{
		m = _mm_cvtepi32_ps(_mm_unpacklo_epi32(_mm_cvtsi32_si128(x), _mm_cvtsi32_si128(y)));
	}

	//Not currently used, just causes a compiler warning
	/*__forceinline GSVector4(const GSVector4& v)
	{
		m = v.m;
	}*/

	__forceinline explicit GSVector4(const GSVector2& v)
	{
		m = _mm_castsi128_ps(_mm_loadl_epi64((__m128i*)&v));
	}

	__forceinline explicit GSVector4(const GSVector2i& v)
	{
		m = _mm_cvtepi32_ps(_mm_loadl_epi64((__m128i*)&v));
	}

	__forceinline explicit GSVector4(__m128 m)
	{
		this->m = m;
	}

	__forceinline explicit GSVector4(float f)
	{
		*this = f;
	}

	__forceinline explicit GSVector4(int i)
	{
		#if _M_SSE >= 0x501

		m = _mm_cvtepi32_ps(_mm_broadcastd_epi32(_mm_cvtsi32_si128(i)));

		#else

		GSVector4i v((int)i);

		*this = GSVector4(v);

		#endif
	}
	
	__forceinline explicit GSVector4(uint32 u)
	{
		GSVector4i v((int)u);

		*this = GSVector4(v) + (m_x4f800000 & GSVector4::cast(v.sra32(31)));
	}

	__forceinline explicit GSVector4(const GSVector4i& v);

	__forceinline static GSVector4 cast(const GSVector4i& v);

	#if _M_SSE >= 0x500

	__forceinline static GSVector4 cast(const GSVector8& v);

	#endif

	#if _M_SSE >= 0x501

	__forceinline static GSVector4 cast(const GSVector8i& v);

	#endif

	__forceinline void operator = (const GSVector4& v)
	{
		m = v.m;
	}

	__forceinline void operator = (float f)
	{
		#if _M_SSE >= 0x501

		m =  _mm_broadcastss_ps(_mm_load_ss(&f));

		#else

		m = _mm_set1_ps(f);

		#endif
	}

	__forceinline void operator = (__m128 m)
	{
		this->m = m;
	}

	__forceinline operator __m128() const
	{
		return m;
	}

	__forceinline uint32 rgba32() const
	{
		return GSVector4i(*this).rgba32();
	}

	__forceinline static GSVector4 rgba32(uint32 rgba)
	{
		return GSVector4(GSVector4i::load((int)rgba).u8to32());
	}

	__forceinline static GSVector4 rgba32(uint32 rgba, int shift)
	{
		return GSVector4(GSVector4i::load((int)rgba).u8to32() << shift);
	}

	__forceinline GSVector4 abs() const
	{
		return *this & cast(GSVector4i::x7fffffff());
	}

	__forceinline GSVector4 neg() const
	{
		return *this ^ cast(GSVector4i::x80000000());
	}

	__forceinline GSVector4 rcp() const
	{
		return GSVector4(_mm_rcp_ps(m));
	}

	__forceinline GSVector4 rcpnr() const
	{
		GSVector4 v = rcp();

		return (v + v) - (v * v) * *this;
	}

	template<int mode> __forceinline GSVector4 round() const
	{
		#if _M_SSE >= 0x401

		return GSVector4(_mm_round_ps(m, mode));

		#else

		GSVector4 a = *this;

		GSVector4 b = (a & cast(GSVector4i::x80000000())) | m_x4b000000;

		b = a + b - b;

		if((mode & 7) == (Round_NegInf & 7))
		{
			return b - ((a < b) & m_one);
		}

		if((mode & 7) == (Round_PosInf & 7))
		{
			return b + ((a > b) & m_one);
		}

		ASSERT((mode & 7) == (Round_NearestInt & 7)); // other modes aren't implemented

		return b;

		#endif
	}

	__forceinline GSVector4 floor() const
	{
		return round<Round_NegInf>();
	}

	__forceinline GSVector4 ceil() const
	{
		return round<Round_PosInf>();
	}

	// http://jrfonseca.blogspot.com/2008/09/fast-sse2-pow-tables-or-polynomials.html

	#define LOG_POLY0(x, c0) GSVector4(c0)
	#define LOG_POLY1(x, c0, c1) (LOG_POLY0(x, c1).madd(x, GSVector4(c0)))
	#define LOG_POLY2(x, c0, c1, c2) (LOG_POLY1(x, c1, c2).madd(x, GSVector4(c0)))
	#define LOG_POLY3(x, c0, c1, c2, c3) (LOG_POLY2(x, c1, c2, c3).madd(x, GSVector4(c0)))
	#define LOG_POLY4(x, c0, c1, c2, c3, c4) (LOG_POLY3(x, c1, c2, c3, c4).madd(x, GSVector4(c0)))
	#define LOG_POLY5(x, c0, c1, c2, c3, c4, c5) (LOG_POLY4(x, c1, c2, c3, c4, c5).madd(x, GSVector4(c0)))

	__forceinline GSVector4 log2(int precision = 5) const
	{
		// NOTE: sign bit ignored, safe to pass negative numbers

		// The idea behind this algorithm is to split the float into two parts, log2(m * 2^e) => log2(m) + log2(2^e) => log2(m) + e, 
		// and then approximate the logarithm of the mantissa (it's 1.x when normalized, a nice short range).

		GSVector4 one = m_one;

		GSVector4i i = GSVector4i::cast(*this);

		GSVector4 e = GSVector4(((i << 1) >> 24) - GSVector4i::x0000007f());
		GSVector4 m = GSVector4::cast((i << 9) >> 9) | one;

		GSVector4 p;

		// Minimax polynomial fit of log2(x)/(x - 1), for x in range [1, 2[

		switch(precision)
		{
		case 3:
			p = LOG_POLY2(m, 2.28330284476918490682f, -1.04913055217340124191f, 0.204446009836232697516f);
			break;
		case 4:
			p = LOG_POLY3(m, 2.61761038894603480148f, -1.75647175389045657003f, 0.688243882994381274313f, -0.107254423828329604454f);
			break;
		default:
		case 5:
			p = LOG_POLY4(m, 2.8882704548164776201f, -2.52074962577807006663f, 1.48116647521213171641f, -0.465725644288844778798f, 0.0596515482674574969533f);
			break;
		case 6:
			p = LOG_POLY5(m, 3.1157899f, -3.3241990f, 2.5988452f, -1.2315303f,  3.1821337e-1f, -3.4436006e-2f);
			break;
		}

		// This effectively increases the polynomial degree by one, but ensures that log2(1) == 0

		p = p * (m - one);

		return p + e;
	}

	__forceinline GSVector4 madd(const GSVector4& a, const GSVector4& b) const
	{
		#if 0//_M_SSE >= 0x501

		return GSVector4(_mm_fmadd_ps(m, a, b));
		
		#else
		
		return *this * a + b;
		
		#endif
	}

	__forceinline GSVector4 msub(const GSVector4& a, const GSVector4& b) const
	{
		#if 0//_M_SSE >= 0x501

		return GSVector4(_mm_fmsub_ps(m, a, b));
		
		#else
		
		return *this * a - b;
		
		#endif
	}

	__forceinline GSVector4 nmadd(const GSVector4& a, const GSVector4& b) const
	{
		#if 0//_M_SSE >= 0x501

		return GSVector4(_mm_fnmadd_ps(m, a, b));
		
		#else
		
		return b - *this * a;
		
		#endif
	}

	__forceinline GSVector4 nmsub(const GSVector4& a, const GSVector4& b) const
	{
		#if 0//_M_SSE >= 0x501

		return GSVector4(_mm_fnmsub_ps(m, a, b));
		
		#else

		return -b - *this * a;

		#endif
	}

	__forceinline GSVector4 addm(const GSVector4& a, const GSVector4& b) const
	{
		return a.madd(b, *this); // *this + a * b
	}

	__forceinline GSVector4 subm(const GSVector4& a, const GSVector4& b) const
	{
		return a.nmadd(b, *this); // *this - a * b
	}

	__forceinline GSVector4 hadd() const
	{
		#if _M_SSE >= 0x300
		
		return GSVector4(_mm_hadd_ps(m, m));
		
		#else
		
		return xzxz() + ywyw();
		
		#endif
	}

	__forceinline GSVector4 hadd(const GSVector4& v) const
	{
		#if _M_SSE >= 0x300
		
		return GSVector4(_mm_hadd_ps(m, v.m));
		
		#else
		
		return xzxz(v) + ywyw(v);
		
		#endif
	}

	__forceinline GSVector4 hsub() const
	{
		#if _M_SSE >= 0x300
		
		return GSVector4(_mm_hsub_ps(m, m));
		
		#else
		
		return xzxz() - ywyw();
		
		#endif
	}

	__forceinline GSVector4 hsub(const GSVector4& v) const
	{
		#if _M_SSE >= 0x300
		
		return GSVector4(_mm_hsub_ps(m, v.m));
		
		#else
		
		return xzxz(v) - ywyw(v);

		#endif
	}

	#if _M_SSE >= 0x401

	template<int i> __forceinline GSVector4 dp(const GSVector4& v) const
	{
		return GSVector4(_mm_dp_ps(m, v.m, i));
	}

	#endif

	__forceinline GSVector4 sat(const GSVector4& a, const GSVector4& b) const
	{
		return GSVector4(_mm_min_ps(_mm_max_ps(m, a), b));
	}

	__forceinline GSVector4 sat(const GSVector4& a) const
	{
		return GSVector4(_mm_min_ps(_mm_max_ps(m, a.xyxy()), a.zwzw()));
	}

	__forceinline GSVector4 sat(const float scale = 255) const
	{
		return sat(zero(), GSVector4(scale));
	}

	__forceinline GSVector4 clamp(const float scale = 255) const
	{
		return min(GSVector4(scale));
	}

	__forceinline GSVector4 min(const GSVector4& a) const
	{
		return GSVector4(_mm_min_ps(m, a));
	}

	__forceinline GSVector4 max(const GSVector4& a) const
	{
		return GSVector4(_mm_max_ps(m, a));
	}

	#if _M_SSE >= 0x401

	template<int mask> __forceinline GSVector4 blend32(const GSVector4& a)  const
	{
		return GSVector4(_mm_blend_ps(m, a, mask));
	}

	#endif

	__forceinline GSVector4 blend32(const GSVector4& a, const GSVector4& mask)  const
	{
		#if _M_SSE >= 0x401

		return GSVector4(_mm_blendv_ps(m, a, mask));

		#else

		return GSVector4(_mm_or_ps(_mm_andnot_ps(mask, m), _mm_and_ps(mask, a)));

		#endif
	}

	__forceinline GSVector4 upl(const GSVector4& a) const
	{
		return GSVector4(_mm_unpacklo_ps(m, a));
	}

	__forceinline GSVector4 uph(const GSVector4& a) const
	{
		return GSVector4(_mm_unpackhi_ps(m, a));
	}

	__forceinline GSVector4 l2h(const GSVector4& a) const
	{
		return GSVector4(_mm_movelh_ps(m, a));
	}

	__forceinline GSVector4 h2l(const GSVector4& a) const
	{
		return GSVector4(_mm_movehl_ps(m, a));
	}

	__forceinline GSVector4 andnot(const GSVector4& v) const
	{
		return GSVector4(_mm_andnot_ps(v.m, m));
	}

	__forceinline int mask() const
	{
		return _mm_movemask_ps(m);
	}

	__forceinline bool alltrue() const
	{
		return mask() == 0xf;
	}

	__forceinline bool allfalse() const
	{
		#if _M_SSE >= 0x500

		return _mm_testz_ps(m, m) != 0;

		#elif _M_SSE >= 0x401

		__m128i a = _mm_castps_si128(m);

		return _mm_testz_si128(a, a) != 0;

		#else

		return mask() == 0;

		#endif
	}

	template<int src, int dst> __forceinline GSVector4 insert32(const GSVector4& v) const
	{
		// TODO: use blendps when src == dst

		#if 0 // _M_SSE >= 0x401

		// NOTE: it's faster with shuffles...

		return GSVector4(_mm_insert_ps(m, v.m, _MM_MK_INSERTPS_NDX(src, dst, 0)));

		#else

		switch(dst)
		{
		case 0:
			switch(src)
			{
			case 0: return yyxx(v).zxzw(*this);
			case 1: return yyyy(v).zxzw(*this);
			case 2: return yyzz(v).zxzw(*this);
			case 3: return yyww(v).zxzw(*this);
			default: __assume(0);
			}
			break;
		case 1:
			switch(src)
			{
			case 0: return xxxx(v).xzzw(*this);
			case 1: return xxyy(v).xzzw(*this);
			case 2: return xxzz(v).xzzw(*this);
			case 3: return xxww(v).xzzw(*this);
			default: __assume(0);
			}
			break;
		case 2:
			switch(src)
			{
			case 0: return xyzx(wwxx(v));
			case 1: return xyzx(wwyy(v));
			case 2: return xyzx(wwzz(v));
			case 3: return xyzx(wwww(v));
			default: __assume(0);
			}
			break;
		case 3:
			switch(src)
			{
			case 0: return xyxz(zzxx(v));
			case 1: return xyxz(zzyy(v));
			case 2: return xyxz(zzzz(v));
			case 3: return xyxz(zzww(v));
			default: __assume(0);
			}
			break;
		default:
			__assume(0);
		}

		#endif

	}

	template<int i> __forceinline int extract32() const
	{
		#if _M_SSE >= 0x401

		return _mm_extract_ps(m, i);

		#else

		return i32[i];

		#endif
	}

	__forceinline static GSVector4 zero()
	{
		return GSVector4(_mm_setzero_ps());
	}

	__forceinline static GSVector4 xffffffff()
	{
		return zero() == zero();
	}

	__forceinline static GSVector4 ps0123()
	{
		return GSVector4(m_ps0123);
	}

	__forceinline static GSVector4 ps4567()
	{
		return GSVector4(m_ps4567);
	}

	__forceinline static GSVector4 loadl(const void* p)
	{
		return GSVector4(_mm_castpd_ps(_mm_load_sd((double*)p)));
	}

	__forceinline static GSVector4 load(float f)
	{
		return GSVector4(_mm_load_ss(&f));
	}

	__forceinline static GSVector4 load(uint32 u)
	{
		GSVector4i v = GSVector4i::load((int)u);

		return GSVector4(v) + (m_x4f800000 & GSVector4::cast(v.sra32(31)));
	}

	template<bool aligned> __forceinline static GSVector4 load(const void* p)
	{
		return GSVector4(aligned ? _mm_load_ps((const float*)p) : _mm_loadu_ps((const float*)p));
	}

	__forceinline static void storent(void* p, const GSVector4& v)
	{
		_mm_stream_ps((float*)p, v.m);
	}

	__forceinline static void storel(void* p, const GSVector4& v)
	{
		_mm_store_sd((double*)p, _mm_castps_pd(v.m));
	}

	template<bool aligned> __forceinline static void store(void* p, const GSVector4& v)
	{
		if(aligned) _mm_store_ps((float*)p, v.m);
		else _mm_storeu_ps((float*)p, v.m);
	}

	__forceinline static void expand(const GSVector4i& v, GSVector4& a, GSVector4& b, GSVector4& c, GSVector4& d)
	{
		GSVector4i mask = GSVector4i::x000000ff();

		a = GSVector4(v & mask);
		b = GSVector4((v >> 8) & mask);
		c = GSVector4((v >> 16) & mask);
		d = GSVector4((v >> 24));
	}

	__forceinline static void transpose(GSVector4& a, GSVector4& b, GSVector4& c, GSVector4& d)
	{
		GSVector4 v0 = a.xyxy(b);
		GSVector4 v1 = c.xyxy(d);

		GSVector4 e = v0.xzxz(v1);
		GSVector4 f = v0.ywyw(v1);

		GSVector4 v2 = a.zwzw(b);
		GSVector4 v3 = c.zwzw(d);

		GSVector4 g = v2.xzxz(v3);
		GSVector4 h = v2.ywyw(v3);

		a = e;
		b = f;
		c = g;
		d = h;
/*
		GSVector4 v0 = a.xyxy(b);
		GSVector4 v1 = c.xyxy(d);
		GSVector4 v2 = a.zwzw(b);
		GSVector4 v3 = c.zwzw(d);

		a = v0.xzxz(v1);
		b = v0.ywyw(v1);
		c = v2.xzxz(v3);
		d = v2.ywyw(v3);
*/
/*
		GSVector4 v0 = a.upl(b);
		GSVector4 v1 = a.uph(b);
		GSVector4 v2 = c.upl(d);
		GSVector4 v3 = c.uph(d);

		a = v0.l2h(v2);
		b = v2.h2l(v0);
		c = v1.l2h(v3);
		d = v3.h2l(v1);
*/	}

	__forceinline GSVector4 operator - () const
	{
		return neg();
	}

	__forceinline void operator += (const GSVector4& v)
	{
		m = _mm_add_ps(m, v);
	}

	__forceinline void operator -= (const GSVector4& v)
	{
		m = _mm_sub_ps(m, v);
	}

	__forceinline void operator *= (const GSVector4& v)
	{
		m = _mm_mul_ps(m, v);
	}

	__forceinline void operator /= (const GSVector4& v)
	{
		m = _mm_div_ps(m, v);
	}

	__forceinline void operator += (float f)
	{
		*this += GSVector4(f);
	}

	__forceinline void operator -= (float f)
	{
		*this -= GSVector4(f);
	}

	__forceinline void operator *= (float f)
	{
		*this *= GSVector4(f);
	}

	__forceinline void operator /= (float f)
	{
		*this /= GSVector4(f);
	}

	__forceinline void operator &= (const GSVector4& v)
	{
		m = _mm_and_ps(m, v);
	}

	__forceinline void operator |= (const GSVector4& v)
	{
		m = _mm_or_ps(m, v);
	}

	__forceinline void operator ^= (const GSVector4& v)
	{
		m = _mm_xor_ps(m, v);
	}

	__forceinline friend GSVector4 operator + (const GSVector4& v1, const GSVector4& v2)
	{
		return GSVector4(_mm_add_ps(v1, v2));
	}

	__forceinline friend GSVector4 operator - (const GSVector4& v1, const GSVector4& v2)
	{
		return GSVector4(_mm_sub_ps(v1, v2));
	}

	__forceinline friend GSVector4 operator * (const GSVector4& v1, const GSVector4& v2)
	{
		return GSVector4(_mm_mul_ps(v1, v2));
	}

	__forceinline friend GSVector4 operator / (const GSVector4& v1, const GSVector4& v2)
	{
		return GSVector4(_mm_div_ps(v1, v2));
	}

	__forceinline friend GSVector4 operator + (const GSVector4& v, float f)
	{
		return v + GSVector4(f);
	}

	__forceinline friend GSVector4 operator - (const GSVector4& v, float f)
	{
		return v - GSVector4(f);
	}

	__forceinline friend GSVector4 operator * (const GSVector4& v, float f)
	{
		return v * GSVector4(f);
	}

	__forceinline friend GSVector4 operator / (const GSVector4& v, float f)
	{
		return v / GSVector4(f);
	}

	__forceinline friend GSVector4 operator & (const GSVector4& v1, const GSVector4& v2)
	{
		return GSVector4(_mm_and_ps(v1, v2));
	}

	__forceinline friend GSVector4 operator | (const GSVector4& v1, const GSVector4& v2)
	{
		return GSVector4(_mm_or_ps(v1, v2));
	}

	__forceinline friend GSVector4 operator ^ (const GSVector4& v1, const GSVector4& v2)
	{
		return GSVector4(_mm_xor_ps(v1, v2));
	}

	__forceinline friend GSVector4 operator == (const GSVector4& v1, const GSVector4& v2)
	{
		return GSVector4(_mm_cmpeq_ps(v1, v2));
	}

	__forceinline friend GSVector4 operator != (const GSVector4& v1, const GSVector4& v2)
	{
		return GSVector4(_mm_cmpneq_ps(v1, v2));
	}

	__forceinline friend GSVector4 operator > (const GSVector4& v1, const GSVector4& v2)
	{
		return GSVector4(_mm_cmpgt_ps(v1, v2));
	}

	__forceinline friend GSVector4 operator < (const GSVector4& v1, const GSVector4& v2)
	{
		return GSVector4(_mm_cmplt_ps(v1, v2));
	}

	__forceinline friend GSVector4 operator >= (const GSVector4& v1, const GSVector4& v2)
	{
		return GSVector4(_mm_cmpge_ps(v1, v2));
	}

	__forceinline friend GSVector4 operator <= (const GSVector4& v1, const GSVector4& v2)
	{
		return GSVector4(_mm_cmple_ps(v1, v2));
	}

	#define VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, ws, wn) \
		__forceinline GSVector4 xs##ys##zs##ws() const {return GSVector4(_mm_shuffle_ps(m, m, _MM_SHUFFLE(wn, zn, yn, xn)));} \
		__forceinline GSVector4 xs##ys##zs##ws(const GSVector4& v) const {return GSVector4(_mm_shuffle_ps(m, v.m, _MM_SHUFFLE(wn, zn, yn, xn)));} \

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
		VECTOR4_SHUFFLE_2(xs, xn, x, 0) \
		VECTOR4_SHUFFLE_2(xs, xn, y, 1) \
		VECTOR4_SHUFFLE_2(xs, xn, z, 2) \
		VECTOR4_SHUFFLE_2(xs, xn, w, 3) \

	VECTOR4_SHUFFLE_1(x, 0)
	VECTOR4_SHUFFLE_1(y, 1)
	VECTOR4_SHUFFLE_1(z, 2)
	VECTOR4_SHUFFLE_1(w, 3)

	#if _M_SSE >= 0x501

	__forceinline GSVector4 broadcast32() const
	{
		return GSVector4(_mm_broadcastss_ps(m));
	}

	__forceinline static GSVector4 broadcast32(const GSVector4& v)
	{
		return GSVector4(_mm_broadcastss_ps(v.m));
	}

	__forceinline static GSVector4 broadcast32(const void* f)
	{
		return GSVector4(_mm_broadcastss_ps(_mm_load_ss((const float*)f)));
	}

	#endif
};

#if _M_SSE >= 0x501

__aligned(class, 32) GSVector8i
{
	static const GSVector8i m_xff[33];
	static const GSVector8i m_x0f[33];

public:
	union
	{
		struct {int x0, y0, z0, w0, x1, y1, z1, w1;};
		struct {int r0, g0, b0, a0, r1, g1, b1, a1;};
		int v[8];
		float f32[8];
		int8 i8[32];
		int16 i16[16];
		int32 i32[8];
		int64 i64[4];
		uint8 u8[32];
		uint16 u16[16];
		uint32 u32[8];
		uint64 u64[4];
		__m256i m;
		__m128i m0, m1;
	};

	__forceinline GSVector8i() {}

	__forceinline explicit GSVector8i(const GSVector8& v, bool truncate = true);

	__forceinline static GSVector8i cast(const GSVector8& v);
	__forceinline static GSVector8i cast(const GSVector4& v);
	__forceinline static GSVector8i cast(const GSVector4i& v);

	__forceinline GSVector8i(int x0, int y0, int z0, int w0, int x1, int y1, int z1, int w1)
	{
		m = _mm256_set_epi32(w1, z1, y1, x1, w0, z0, y0, x0);
	}

	__forceinline GSVector8i(
		short s0, short s1, short s2, short s3, short s4, short s5, short s6, short s7,
		short s8, short s9, short s10, short s11, short s12, short s13, short s14, short s15)
	{
		m = _mm256_set_epi16(s15, s14, s13, s12, s11, s10, s9, s8, s7, s6, s5, s4, s3, s2, s1, s0);
	}

	__forceinline GSVector8i(
		char b0, char b1, char b2, char b3, char b4, char b5, char b6, char b7, 
		char b8, char b9, char b10, char b11, char b12, char b13, char b14, char b15,
		char b16, char b17, char b18, char b19, char b20, char b21, char b22, char b23,
		char b24, char b25, char b26, char b27, char b28, char b29, char b30, char b31
		)
	{
		m = _mm256_set_epi8(
			b31, b30, b29, b28, b27, b26, b25, b24, b23, b22, b21, b20, b19, b18, b17, b16,
			b15, b14, b13, b12, b11, b10, b9, b8, b7, b6, b5, b4, b3, b2, b1, b0);
	}

	__forceinline GSVector8i(__m128i m0, __m128i m1)
	{
		#if 0 // _MSC_VER >= 1700 
		
		this->m = _mm256_permute2x128_si256(_mm256_castsi128_si256(m0), _mm256_castsi128_si256(m1), 0);
		
		#else
		
		*this = zero().insert<0>(m0).insert<1>(m1);

		#endif
	}

	__forceinline GSVector8i(const GSVector8i& v)
	{
		m = v.m;
	}

	__forceinline explicit GSVector8i(int i)
	{
		*this = i;
	}

	__forceinline explicit GSVector8i(__m128i m)
	{
		*this = m;
	}

	__forceinline explicit GSVector8i(__m256i m)
	{
		this->m = m;
	}

	__forceinline void operator = (const GSVector8i& v)
	{
		m = v.m;
	}

	__forceinline void operator = (int i)
	{
		m = _mm256_broadcastd_epi32(_mm_cvtsi32_si128(i)); // m = _mm256_set1_epi32(i);
	}

	__forceinline void operator = (__m128i m)
	{
		this->m = _mm256_inserti128_si256(_mm256_castsi128_si256(m), m, 1);
	}

	__forceinline void operator = (__m256i m)
	{
		this->m = m;
	}

	__forceinline operator __m256i() const
	{
		return m;
	}

	//

	__forceinline GSVector8i sat_i8(const GSVector8i& a, const GSVector8i& b) const
	{
		return max_i8(a).min_i8(b);
	}

	__forceinline GSVector8i sat_i8(const GSVector8i& a) const
	{
		return max_i8(a.xyxy()).min_i8(a.zwzw());
	}

	__forceinline GSVector8i sat_i16(const GSVector8i& a, const GSVector8i& b) const
	{
		return max_i16(a).min_i16(b);
	}

	__forceinline GSVector8i sat_i16(const GSVector8i& a) const
	{
		return max_i16(a.xyxy()).min_i16(a.zwzw());
	}

	__forceinline GSVector8i sat_i32(const GSVector8i& a, const GSVector8i& b) const
	{
		return max_i32(a).min_i32(b);
	}

	__forceinline GSVector8i sat_i32(const GSVector8i& a) const
	{
		return max_i32(a.xyxy()).min_i32(a.zwzw());
	}
	
	__forceinline GSVector8i sat_u8(const GSVector8i& a, const GSVector8i& b) const
	{
		return max_u8(a).min_u8(b);
	}

	__forceinline GSVector8i sat_u8(const GSVector8i& a) const
	{
		return max_u8(a.xyxy()).min_u8(a.zwzw());
	}

	__forceinline GSVector8i sat_u16(const GSVector8i& a, const GSVector8i& b) const
	{
		return max_u16(a).min_u16(b);
	}

	__forceinline GSVector8i sat_u16(const GSVector8i& a) const
	{
		return max_u16(a.xyxy()).min_u16(a.zwzw());
	}

	__forceinline GSVector8i sat_u32(const GSVector8i& a, const GSVector8i& b) const
	{
		return max_u32(a).min_u32(b);
	}

	__forceinline GSVector8i sat_u32(const GSVector8i& a) const
	{
		return max_u32(a.xyxy()).min_u32(a.zwzw());
	}

	__forceinline GSVector8i min_i8(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_min_epi8(m, a));
	}

	__forceinline GSVector8i max_i8(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_max_epi8(m, a));
	}

	__forceinline GSVector8i min_i16(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_min_epi16(m, a));
	}

	__forceinline GSVector8i max_i16(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_max_epi16(m, a));
	}

	__forceinline GSVector8i min_i32(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_min_epi32(m, a));
	}

	__forceinline GSVector8i max_i32(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_max_epi32(m, a));
	}

	__forceinline GSVector8i min_u8(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_min_epu8(m, a));
	}

	__forceinline GSVector8i max_u8(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_max_epu8(m, a));
	}

	__forceinline GSVector8i min_u16(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_min_epu16(m, a));
	}

	__forceinline GSVector8i max_u16(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_max_epu16(m, a));
	}

	__forceinline GSVector8i min_u32(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_min_epu32(m, a));
	}

	__forceinline GSVector8i max_u32(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_max_epu32(m, a));
	}

	__forceinline GSVector8i clamp8() const
	{
		return pu16().upl8();
	}

	__forceinline GSVector8i blend8(const GSVector8i& a, const GSVector8i& mask) const
	{
		return GSVector8i(_mm256_blendv_epi8(m, a, mask));
	}

	template<int mask> __forceinline GSVector8i blend16(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_blend_epi16(m, a, mask));
	}

	__forceinline GSVector8i blend(const GSVector8i& a, const GSVector8i& mask) const
	{
		return GSVector8i(_mm256_or_si256(_mm256_andnot_si256(mask, m), _mm256_and_si256(mask, a)));
	}

	__forceinline GSVector8i mix16(const GSVector8i& a) const
	{
		return blend16<0xaa>(a);
	}

	__forceinline GSVector8i shuffle8(const GSVector8i& mask) const
	{
		return GSVector8i(_mm256_shuffle_epi8(m, mask));
	}

	__forceinline GSVector8i ps16(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_packs_epi16(m, a));
	}

	__forceinline GSVector8i ps16() const
	{
		return GSVector8i(_mm256_packs_epi16(m, m));
	}

	__forceinline GSVector8i pu16(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_packus_epi16(m, a));
	}

	__forceinline GSVector8i pu16() const
	{
		return GSVector8i(_mm256_packus_epi16(m, m));
	}

	__forceinline GSVector8i ps32(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_packs_epi32(m, a));
	}

	__forceinline GSVector8i ps32() const
	{
		return GSVector8i(_mm256_packs_epi32(m, m));
	}

	__forceinline GSVector8i pu32(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_packus_epi32(m, a));
	}

	__forceinline GSVector8i pu32() const
	{
		return GSVector8i(_mm256_packus_epi32(m, m));
	}

	__forceinline GSVector8i upl8(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_unpacklo_epi8(m, a));
	}

	__forceinline GSVector8i uph8(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_unpackhi_epi8(m, a));
	}

	__forceinline GSVector8i upl16(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_unpacklo_epi16(m, a));
	}

	__forceinline GSVector8i uph16(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_unpackhi_epi16(m, a));
	}

	__forceinline GSVector8i upl32(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_unpacklo_epi32(m, a));
	}

	__forceinline GSVector8i uph32(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_unpackhi_epi32(m, a));
	}

	__forceinline GSVector8i upl64(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_unpacklo_epi64(m, a));
	}

	__forceinline GSVector8i uph64(const GSVector8i& a) const
	{
		return GSVector8i(_mm256_unpackhi_epi64(m, a));
	}

	__forceinline GSVector8i upl8() const
	{
		return GSVector8i(_mm256_unpacklo_epi8(m, _mm256_setzero_si256()));
	}

	__forceinline GSVector8i uph8() const
	{
		return GSVector8i(_mm256_unpackhi_epi8(m, _mm256_setzero_si256()));
	}

	__forceinline GSVector8i upl16() const
	{
		return GSVector8i(_mm256_unpacklo_epi16(m, _mm256_setzero_si256()));
	}

	__forceinline GSVector8i uph16() const
	{
		return GSVector8i(_mm256_unpackhi_epi16(m, _mm256_setzero_si256()));
	}

	__forceinline GSVector8i upl32() const
	{
		return GSVector8i(_mm256_unpacklo_epi32(m, _mm256_setzero_si256()));
	}

	__forceinline GSVector8i uph32() const
	{
		return GSVector8i(_mm256_unpackhi_epi32(m, _mm256_setzero_si256()));
	}

	__forceinline GSVector8i upl64() const
	{
		return GSVector8i(_mm256_unpacklo_epi64(m, _mm256_setzero_si256()));
	}

	__forceinline GSVector8i uph64() const
	{
		return GSVector8i(_mm256_unpackhi_epi64(m, _mm256_setzero_si256()));
	}

	// cross lane! from 128-bit to full 256-bit range

	__forceinline GSVector8i i8to16c() const
	{
		return GSVector8i(_mm256_cvtepi8_epi16(_mm256_castsi256_si128(m)));
	}

	__forceinline GSVector8i u8to16c() const
	{
		return GSVector8i(_mm256_cvtepu8_epi16(_mm256_castsi256_si128(m)));
	}

	__forceinline GSVector8i i8to32c() const
	{
		return GSVector8i(_mm256_cvtepi8_epi32(_mm256_castsi256_si128(m)));
	}

	__forceinline GSVector8i u8to32c() const
	{
		return GSVector8i(_mm256_cvtepu8_epi32(_mm256_castsi256_si128(m)));
	}

	__forceinline GSVector8i i8to64c() const
	{
		return GSVector8i(_mm256_cvtepi8_epi64(_mm256_castsi256_si128(m)));
	}

	__forceinline GSVector8i u8to64c() const
	{
		return GSVector8i(_mm256_cvtepu16_epi64(_mm256_castsi256_si128(m)));
	}

	__forceinline GSVector8i i16to32c() const
	{
		return GSVector8i(_mm256_cvtepi16_epi32(_mm256_castsi256_si128(m)));
	}

	__forceinline GSVector8i u16to32c() const
	{
		return GSVector8i(_mm256_cvtepu16_epi32(_mm256_castsi256_si128(m)));
	}

	__forceinline GSVector8i i16to64c() const
	{
		return GSVector8i(_mm256_cvtepi16_epi64(_mm256_castsi256_si128(m)));
	}

	__forceinline GSVector8i u16to64c() const
	{
		return GSVector8i(_mm256_cvtepu16_epi64(_mm256_castsi256_si128(m)));
	}

	__forceinline GSVector8i i32to64c() const
	{
		return GSVector8i(_mm256_cvtepi32_epi64(_mm256_castsi256_si128(m)));
	}

	__forceinline GSVector8i u32to64c() const
	{
		return GSVector8i(_mm256_cvtepu32_epi64(_mm256_castsi256_si128(m)));
	}

	//

	static __forceinline GSVector8i i8to16c(const void* p) 
	{
		return  GSVector8i(_mm256_cvtepi8_epi16(_mm_load_si128((__m128i*)p)));
	}

	static __forceinline GSVector8i u8to16c(const void* p) 
	{
		return  GSVector8i(_mm256_cvtepu8_epi16(_mm_load_si128((__m128i*)p)));
	}

	static __forceinline GSVector8i i8to32c(const void* p) 
	{
		return  GSVector8i(_mm256_cvtepi8_epi32(_mm_loadl_epi64((__m128i*)p)));
	}

	static __forceinline GSVector8i u8to32c(const void* p) 
	{
		return  GSVector8i(_mm256_cvtepu8_epi32(_mm_loadl_epi64((__m128i*)p)));
	}

	static __forceinline GSVector8i i8to64c(int i) 
	{
		return  GSVector8i(_mm256_cvtepi8_epi64(_mm_cvtsi32_si128(i)));
	}

	static __forceinline GSVector8i u8to64c(int i) 
	{
		return  GSVector8i(_mm256_cvtepu8_epi64(_mm_cvtsi32_si128(i)));
	}

	static __forceinline GSVector8i i16to32c(const void* p) 
	{
		return  GSVector8i(_mm256_cvtepi16_epi32(_mm_load_si128((__m128i*)p)));
	}

	static __forceinline GSVector8i u16to32c(const void* p) 
	{
		return  GSVector8i(_mm256_cvtepu16_epi32(_mm_load_si128((__m128i*)p)));
	}

	static __forceinline GSVector8i i16to64c(const void* p) 
	{
		return  GSVector8i(_mm256_cvtepi16_epi64(_mm_loadl_epi64((__m128i*)p)));
	}

	static __forceinline GSVector8i u16to64c(const void* p) 
	{
		return  GSVector8i(_mm256_cvtepu16_epi64(_mm_loadl_epi64((__m128i*)p)));
	}

	static __forceinline GSVector8i i32to64c(const void* p) 
	{
		return  GSVector8i(_mm256_cvtepi32_epi64(_mm_load_si128((__m128i*)p)));
	}

	static __forceinline GSVector8i u32to64c(const void* p) 
	{
		return  GSVector8i(_mm256_cvtepu32_epi64(_mm_load_si128((__m128i*)p)));
	}

	//

	template<int i> __forceinline GSVector8i srl() const
	{
		return GSVector8i(_mm256_srli_si256(m, i));
	}

	template<int i> __forceinline GSVector8i srl(const GSVector8i& v)
	{
		return GSVector8i(_mm256_alignr_epi8(v.m, m, i));
	}

	template<int i> __forceinline GSVector8i sll() const
	{
		return GSVector8i(_mm256_slli_si128(m, i));
	}

	__forceinline GSVector8i sra16(int i) const
	{
		return GSVector8i(_mm256_srai_epi16(m, i));
	}

	__forceinline GSVector8i sra16(__m128i i) const
	{
		return GSVector8i(_mm256_sra_epi16(m, i));
	}

	__forceinline GSVector8i sra16(__m256i i) const
	{
		return GSVector8i(_mm256_sra_epi16(m, _mm256_castsi256_si128(i)));
	}

	__forceinline GSVector8i sra32(int i) const
	{
		return GSVector8i(_mm256_srai_epi32(m, i));
	}

	__forceinline GSVector8i sra32(__m128i i) const
	{
		return GSVector8i(_mm256_sra_epi32(m, i));
	}

	__forceinline GSVector8i sra32(__m256i i) const
	{
		return GSVector8i(_mm256_sra_epi32(m, _mm256_castsi256_si128(i)));
	}

	__forceinline GSVector8i srav32(__m256i i) const
	{
		return GSVector8i(_mm256_srav_epi32(m, i));
	}

	__forceinline GSVector8i sll16(int i) const
	{
		return GSVector8i(_mm256_slli_epi16(m, i));
	}

	__forceinline GSVector8i sll16(__m128i i) const
	{
		return GSVector8i(_mm256_sll_epi16(m, i));
	}

	__forceinline GSVector8i sll16(__m256i i) const
	{
		return GSVector8i(_mm256_sll_epi16(m, _mm256_castsi256_si128(i)));
	}

	__forceinline GSVector8i sll32(int i) const
	{
		return GSVector8i(_mm256_slli_epi32(m, i));
	}

	__forceinline GSVector8i sll32(__m128i i) const
	{
		return GSVector8i(_mm256_sll_epi32(m, i));
	}

	__forceinline GSVector8i sll32(__m256i i) const
	{
		return GSVector8i(_mm256_sll_epi32(m, _mm256_castsi256_si128(i)));
	}

	__forceinline GSVector8i sllv32(__m256i i) const
	{
		return GSVector8i(_mm256_sllv_epi32(m, i));
	}

	__forceinline GSVector8i sll64(int i) const
	{
		return GSVector8i(_mm256_slli_epi64(m, i));
	}

	__forceinline GSVector8i sll64(__m128i i) const
	{
		return GSVector8i(_mm256_sll_epi64(m, i));
	}

	__forceinline GSVector8i sll64(__m256i i) const
	{
		return GSVector8i(_mm256_sll_epi64(m, _mm256_castsi256_si128(i)));
	}

	__forceinline GSVector8i sllv64(__m256i i) const
	{
		return GSVector8i(_mm256_sllv_epi64(m, i));
	}

	__forceinline GSVector8i srl16(int i) const
	{
		return GSVector8i(_mm256_srli_epi16(m, i));
	}

	__forceinline GSVector8i srl16(__m128i i) const
	{
		return GSVector8i(_mm256_srl_epi16(m, i));
	}

	__forceinline GSVector8i srl16(__m256i i) const
	{
		return GSVector8i(_mm256_srl_epi16(m, _mm256_castsi256_si128(i)));
	}

	__forceinline GSVector8i srl32(int i) const
	{
		return GSVector8i(_mm256_srli_epi32(m, i));
	}

	__forceinline GSVector8i srl32(__m128i i) const
	{
		return GSVector8i(_mm256_srl_epi32(m, i));
	}

	__forceinline GSVector8i srl32(__m256i i) const
	{
		return GSVector8i(_mm256_srl_epi32(m, _mm256_castsi256_si128(i)));
	}

	__forceinline GSVector8i srlv32(__m256i i) const
	{
		return GSVector8i(_mm256_srlv_epi32(m, i));
	}

	__forceinline GSVector8i srl64(int i) const
	{
		return GSVector8i(_mm256_srli_epi64(m, i));
	}

	__forceinline GSVector8i srl64(__m128i i) const
	{
		return GSVector8i(_mm256_srl_epi64(m, i));
	}

	__forceinline GSVector8i srl64(__m256i i) const
	{
		return GSVector8i(_mm256_srl_epi64(m, _mm256_castsi256_si128(i)));
	}

	__forceinline GSVector8i srlv64(__m256i i) const
	{
		return GSVector8i(_mm256_srlv_epi64(m, i));
	}

	__forceinline GSVector8i add8(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_add_epi8(m, v.m));
	}

	__forceinline GSVector8i add16(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_add_epi16(m, v.m));
	}

	__forceinline GSVector8i add32(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_add_epi32(m, v.m));
	}

	__forceinline GSVector8i adds8(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_adds_epi8(m, v.m));
	}

	__forceinline GSVector8i adds16(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_adds_epi16(m, v.m));
	}

	__forceinline GSVector8i addus8(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_adds_epu8(m, v.m));
	}

	__forceinline GSVector8i addus16(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_adds_epu16(m, v.m));
	}

	__forceinline GSVector8i sub8(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_sub_epi8(m, v.m));
	}

	__forceinline GSVector8i sub16(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_sub_epi16(m, v.m));
	}

	__forceinline GSVector8i sub32(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_sub_epi32(m, v.m));
	}

	__forceinline GSVector8i subs8(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_subs_epi8(m, v.m));
	}

	__forceinline GSVector8i subs16(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_subs_epi16(m, v.m));
	}

	__forceinline GSVector8i subus8(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_subs_epu8(m, v.m));
	}

	__forceinline GSVector8i subus16(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_subs_epu16(m, v.m));
	}

	__forceinline GSVector8i avg8(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_avg_epu8(m, v.m));
	}

	__forceinline GSVector8i avg16(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_avg_epu16(m, v.m));
	}

	__forceinline GSVector8i mul16hs(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_mulhi_epi16(m, v.m));
	}

	__forceinline GSVector8i mul16hu(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_mulhi_epu16(m, v.m));
	}

	__forceinline GSVector8i mul16l(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_mullo_epi16(m, v.m));
	}

	__forceinline GSVector8i mul16hrs(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_mulhrs_epi16(m, v.m));
	}

	GSVector8i madd(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_madd_epi16(m, v.m));
	}

	template<int shift> __forceinline GSVector8i lerp16(const GSVector8i& a, const GSVector8i& f) const
	{
		// (a - this) * f << shift + this

		return add16(a.sub16(*this).modulate16<shift>(f));
	}

	template<int shift> __forceinline static GSVector8i lerp16(const GSVector8i& a, const GSVector8i& b, const GSVector8i& c)
	{
		// (a - b) * c << shift

		return a.sub16(b).modulate16<shift>(c);
	}

	template<int shift> __forceinline static GSVector8i lerp16(const GSVector8i& a, const GSVector8i& b, const GSVector8i& c, const GSVector8i& d)
	{
		// (a - b) * c << shift + d

		return d.add16(a.sub16(b).modulate16<shift>(c));
	}

	__forceinline GSVector8i lerp16_4(const GSVector8i& a, const GSVector8i& f) const
	{
		// (a - this) * f >> 4 + this (a, this: 8-bit, f: 4-bit)

		return add16(a.sub16(*this).mul16l(f).sra16(4));
	}

	template<int shift> __forceinline GSVector8i modulate16(const GSVector8i& f) const
	{
		// a * f << shift
		
		if(shift == 0)
		{
			return mul16hrs(f);
		}

		return sll16(shift + 1).mul16hs(f);
	}

	__forceinline bool eq(const GSVector8i& v) const
	{
		GSVector8i t = *this ^ v;
		
		return _mm256_testz_si256(t, t) != 0;
	}

	__forceinline GSVector8i eq8(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_cmpeq_epi8(m, v.m));
	}

	__forceinline GSVector8i eq16(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_cmpeq_epi16(m, v.m));
	}

	__forceinline GSVector8i eq32(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_cmpeq_epi32(m, v.m));
	}

	__forceinline GSVector8i neq8(const GSVector8i& v) const
	{
		return ~eq8(v);
	}

	__forceinline GSVector8i neq16(const GSVector8i& v) const
	{
		return ~eq16(v);
	}

	__forceinline GSVector8i neq32(const GSVector8i& v) const
	{
		return ~eq32(v);
	}

	__forceinline GSVector8i gt8(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_cmpgt_epi8(m, v.m));
	}

	__forceinline GSVector8i gt16(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_cmpgt_epi16(m, v.m));
	}

	__forceinline GSVector8i gt32(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_cmpgt_epi32(m, v.m));
	}

	__forceinline GSVector8i lt8(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_cmpgt_epi8(v.m, m));
	}

	__forceinline GSVector8i lt16(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_cmpgt_epi16(v.m, m));
	}

	__forceinline GSVector8i lt32(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_cmpgt_epi32(v.m, m));
	}

	__forceinline GSVector8i andnot(const GSVector8i& v) const
	{
		return GSVector8i(_mm256_andnot_si256(v.m, m));
	}

	__forceinline int mask() const
	{
		return _mm256_movemask_epi8(m);
	}

	__forceinline bool alltrue() const
	{
		return mask() == (int)0xffffffff;
	}

	__forceinline bool allfalse() const
	{
		return _mm256_testz_si256(m, m) != 0;
	}

	// TODO: extract/insert

	template<int i> __forceinline int extract8() const
	{
		ASSERT(i < 32);

		GSVector4i v = extract<i / 16>();

		return v.extract8<i & 15>();
	}

	template<int i> __forceinline int extract16() const
	{
		ASSERT(i < 16);

		GSVector4i v = extract<i / 8>();

		return v.extract16<i & 8>();
	}

	template<int i> __forceinline int extract32() const
	{
		ASSERT(i < 8);

		GSVector4i v = extract<i / 4>();

		if((i & 3) == 0) return GSVector4i::store(v);

		return v.extract32<i & 3>();
	}

	template<int i> __forceinline GSVector4i extract() const
	{
		ASSERT(i < 2);

		if(i == 0) return GSVector4i(_mm256_castsi256_si128(m));

		return GSVector4i(_mm256_extracti128_si256(m, i));
	}

	template<int i> __forceinline GSVector8i insert(__m128i m) const
	{
		ASSERT(i < 2);

		return GSVector8i(_mm256_inserti128_si256(this->m, m, i));
	}

	// TODO: gather

	template<class T> __forceinline GSVector8i gather32_32(const T* ptr) const
	{
		GSVector4i v0;
		GSVector4i v1;

		GSVector4i a0 = extract<0>();
		GSVector4i a1 = extract<1>();

		v0 = GSVector4i::load((int)ptr[a0.extract32<0>()]);
		v0 = v0.insert32<1>((int)ptr[a0.extract32<1>()]);
		v0 = v0.insert32<2>((int)ptr[a0.extract32<2>()]);
		v0 = v0.insert32<3>((int)ptr[a0.extract32<3>()]);

		v1 = GSVector4i::load((int)ptr[a1.extract32<0>()]);
		v1 = v1.insert32<1>((int)ptr[a1.extract32<1>()]);
		v1 = v1.insert32<2>((int)ptr[a1.extract32<2>()]);
		v1 = v1.insert32<3>((int)ptr[a1.extract32<3>()]);

		return cast(v0).insert<1>(v1);
	}

	template<> __forceinline GSVector8i gather32_32<uint8>(const uint8* ptr) const
	{
		return GSVector8i(_mm256_i32gather_epi32((const int*)ptr, m, 1)) & GSVector8i::x000000ff();
	}

	template<> __forceinline GSVector8i gather32_32<uint16>(const uint16* ptr) const
	{
		return GSVector8i(_mm256_i32gather_epi32((const int*)ptr, m, 2)) & GSVector8i::x0000ffff();
	}

	template<> __forceinline GSVector8i gather32_32<uint32>(const uint32* ptr) const
	{
		return GSVector8i(_mm256_i32gather_epi32((const int*)ptr, m, 4));
	}

	template<class T1, class T2> __forceinline GSVector8i gather32_32(const T1* ptr1, const T2* ptr2) const
	{
		GSVector4i v0;
		GSVector4i v1;

		GSVector4i a0 = extract<0>();
		GSVector4i a1 = extract<1>();

		v0 = GSVector4i::load((int)ptr2[ptr1[a0.extract32<0>()]]);
		v0 = v0.insert32<1>((int)ptr2[ptr1[a0.extract32<1>()]]);
		v0 = v0.insert32<2>((int)ptr2[ptr1[a0.extract32<2>()]]);
		v0 = v0.insert32<3>((int)ptr2[ptr1[a0.extract32<3>()]]);

		v1 = GSVector4i::load((int)ptr2[ptr1[a1.extract32<0>()]]);
		v1 = v1.insert32<1>((int)ptr2[ptr1[a1.extract32<1>()]]);
		v1 = v1.insert32<2>((int)ptr2[ptr1[a1.extract32<2>()]]);
		v1 = v1.insert32<3>((int)ptr2[ptr1[a1.extract32<3>()]]);

		return cast(v0).insert<1>(v1);
	}

	template<> __forceinline GSVector8i gather32_32<uint8, uint32>(const uint8* ptr1, const uint32* ptr2) const
	{
		return gather32_32<uint8>(ptr1).gather32_32<uint32>(ptr2);
	}

	template<> __forceinline GSVector8i gather32_32<uint32, uint32>(const uint32* ptr1, const uint32* ptr2) const
	{
		return gather32_32<uint32>(ptr1).gather32_32<uint32>(ptr2);
	}

	template<class T> __forceinline void gather32_32(const T* RESTRICT ptr, GSVector8i* RESTRICT dst) const
	{
		dst[0] = gather32_32<>(ptr);
	}

	//

	__forceinline static GSVector8i loadnt(const void* p)
	{
		return GSVector8i(_mm256_stream_load_si256((__m256i*)p));
	}

	__forceinline static GSVector8i loadl(const void* p)
	{
		return GSVector8i(_mm256_castsi128_si256(_mm_load_si128((__m128i*)p)));
	}

	__forceinline static GSVector8i loadh(const void* p)
	{
		return GSVector8i(_mm256_inserti128_si256(_mm256_setzero_si256(), _mm_load_si128((__m128i*)p), 1));

		/* TODO: this may be faster
		__m256i m = _mm256_castsi128_si256(_mm_load_si128((__m128i*)p));
		return GSVector8i(_mm256_permute2x128_si256(m, m, 0x08));
		*/
	}

	__forceinline static GSVector8i loadh(const void* p, const GSVector8i& v)
	{
		return GSVector8i(_mm256_inserti128_si256(v, _mm_load_si128((__m128i*)p), 1));
	}

	__forceinline static GSVector8i load(const void* pl, const void* ph)
	{
		return loadh(ph, loadl(pl));

		/* TODO: this may be faster
		__m256 m0 = _mm256_castsi128_si256(_mm_load_si128((__m128*)pl));
		__m256 m1 = _mm256_castsi128_si256(_mm_load_si128((__m128*)ph));
		return GSVector8i(_mm256_permute2x128_si256(m0, m1, 0x20));
		*/
	}

	__forceinline static GSVector8i load(const void* pll, const void* plh, const void* phl, const void* phh)
	{
		GSVector4i l = GSVector4i::load(pll, plh);
		GSVector4i h = GSVector4i::load(phl, phh);

		return cast(l).ac(cast(h));

		// return GSVector8i(l).insert<1>(h);
	}

	template<bool aligned> __forceinline static GSVector8i load(const void* p)
	{
		return GSVector8i(aligned ? _mm256_load_si256((__m256i*)p) : _mm256_loadu_si256((__m256i*)p));
	}

	__forceinline static GSVector8i load(int i)
	{
		return cast(GSVector4i::load(i));
	}

	#ifdef _M_AMD64

	__forceinline static GSVector8i loadq(int64 i)
	{
		return cast(GSVector4i::loadq(i));
	}

	#endif

	__forceinline static void storent(void* p, const GSVector8i& v)
	{
		_mm256_stream_si256((__m256i*)p, v.m);
	}

	__forceinline static void storel(void* p, const GSVector8i& v)
	{
		_mm_store_si128((__m128i*)p, _mm256_extracti128_si256(v.m, 0));
	}

	__forceinline static void storeh(void* p, const GSVector8i& v)
	{
		_mm_store_si128((__m128i*)p, _mm256_extracti128_si256(v.m, 1));
	}

	__forceinline static void store(void* pl, void* ph, const GSVector8i& v)
	{
		GSVector8i::storel(pl, v);
		GSVector8i::storeh(ph, v);
	}

	template<bool aligned> __forceinline static void store(void* p, const GSVector8i& v)
	{
		if(aligned) _mm256_store_si256((__m256i*)p, v.m);
		else _mm256_storeu_si256((__m256i*)p, v.m);
	}

	__forceinline static int store(const GSVector8i& v)
	{
		return GSVector4i::store(GSVector4i::cast(v));
	}

	#ifdef _M_AMD64

	__forceinline static int64 storeq(const GSVector8i& v)
	{
		return GSVector4i::storeq(GSVector4i::cast(v));
	}

	#endif

	__forceinline static void storent(void* RESTRICT dst, const void* RESTRICT src, size_t size)
	{
		const GSVector8i* s = (const GSVector8i*)src;
		GSVector8i* d = (GSVector8i*)dst;

		if(size == 0) return;

		size_t i = 0;
		size_t j = size >> 7;

		for(; i < j; i++, s += 4, d += 4)
		{
			storent(&d[0], s[0]);
			storent(&d[1], s[1]);
			storent(&d[2], s[2]);
			storent(&d[3], s[3]);
		}

		size &= 127;

		if(size == 0) return;

		memcpy(d, s, size);
	}

	// TODO: swizzling

	__forceinline static void sw8(GSVector8i& a, GSVector8i& b)
	{
		GSVector8i c = a;
		GSVector8i d = b;

		a = c.upl8(d);
		b = c.uph8(d);
	}

	__forceinline static void sw16(GSVector8i& a, GSVector8i& b)
	{
		GSVector8i c = a;
		GSVector8i d = b;

		a = c.upl16(d);
		b = c.uph16(d);
	}

	__forceinline static void sw32(GSVector8i& a, GSVector8i& b)
	{
		GSVector8i c = a;
		GSVector8i d = b;

		a = c.upl32(d);
		b = c.uph32(d);
	}

	__forceinline static void sw64(GSVector8i& a, GSVector8i& b)
	{
		GSVector8i c = a;
		GSVector8i d = b;

		a = c.upl64(d);
		b = c.uph64(d);
	}

	__forceinline static void sw128(GSVector8i& a, GSVector8i& b)
	{
		GSVector8i c = a;
		GSVector8i d = b;

		a = c.ac(d);
		b = c.bd(d);
	}

	__forceinline static void sw4(GSVector8i& a, GSVector8i& b, GSVector8i& c, GSVector8i& d)
	{
		const __m256i epi32_0f0f0f0f = _mm256_set1_epi32(0x0f0f0f0f);

		GSVector8i mask(epi32_0f0f0f0f);

		GSVector8i e = (b << 4).blend(a, mask);
		GSVector8i f = b.blend(a >> 4, mask);
		GSVector8i g = (d << 4).blend(c, mask);
		GSVector8i h = d.blend(c >> 4, mask);

		a = e.upl8(f);
		c = e.uph8(f);
		b = g.upl8(h);
		d = g.uph8(h);
	}

	__forceinline static void sw8(GSVector8i& a, GSVector8i& b, GSVector8i& c, GSVector8i& d)
	{
		GSVector8i e = a;
		GSVector8i f = c;

		a = e.upl8(b);
		c = e.uph8(b);
		b = f.upl8(d);
		d = f.uph8(d);
	}

	__forceinline static void sw16(GSVector8i& a, GSVector8i& b, GSVector8i& c, GSVector8i& d)
	{
		GSVector8i e = a;
		GSVector8i f = c;

		a = e.upl16(b);
		c = e.uph16(b);
		b = f.upl16(d);
		d = f.uph16(d);
	}

	__forceinline static void sw32(GSVector8i& a, GSVector8i& b, GSVector8i& c, GSVector8i& d)
	{
		GSVector8i e = a;
		GSVector8i f = c;

		a = e.upl32(b);
		c = e.uph32(b);
		b = f.upl32(d);
		d = f.uph32(d);
	}

	__forceinline static void sw64(GSVector8i& a, GSVector8i& b, GSVector8i& c, GSVector8i& d)
	{
		GSVector8i e = a;
		GSVector8i f = c;

		a = e.upl64(b);
		c = e.uph64(b);
		b = f.upl64(d);
		d = f.uph64(d);
	}

	__forceinline static void sw128(GSVector8i& a, GSVector8i& b, GSVector8i& c, GSVector8i& d)
	{
		GSVector8i e = a;
		GSVector8i f = c;

		a = e.ac(b);
		c = e.bd(b);
		b = f.ac(d);
		d = f.bd(d);
	}

	__forceinline void operator += (const GSVector8i& v)
	{
		m = _mm256_add_epi32(m, v);
	}

	__forceinline void operator -= (const GSVector8i& v)
	{
		m = _mm256_sub_epi32(m, v);
	}

	__forceinline void operator += (int i)
	{
		*this += GSVector8i(i);
	}

	__forceinline void operator -= (int i)
	{
		*this -= GSVector8i(i);
	}

	__forceinline void operator <<= (const int i)
	{
		m = _mm256_slli_epi32(m, i);
	}

	__forceinline void operator >>= (const int i)
	{
		m = _mm256_srli_epi32(m, i);
	}

	__forceinline void operator &= (const GSVector8i& v)
	{
		m = _mm256_and_si256(m, v);
	}

	__forceinline void operator |= (const GSVector8i& v)
	{
		m = _mm256_or_si256(m, v);
	}

	__forceinline void operator ^= (const GSVector8i& v)
	{
		m = _mm256_xor_si256(m, v);
	}

	__forceinline friend GSVector8i operator + (const GSVector8i& v1, const GSVector8i& v2)
	{
		return GSVector8i(_mm256_add_epi32(v1, v2));
	}

	__forceinline friend GSVector8i operator - (const GSVector8i& v1, const GSVector8i& v2)
	{
		return GSVector8i(_mm256_sub_epi32(v1, v2));
	}

	__forceinline friend GSVector8i operator + (const GSVector8i& v, int i)
	{
		return v + GSVector8i(i);
	}

	__forceinline friend GSVector8i operator - (const GSVector8i& v, int i)
	{
		return v - GSVector8i(i);
	}

	__forceinline friend GSVector8i operator << (const GSVector8i& v, const int i)
	{
		return GSVector8i(_mm256_slli_epi32(v, i));
	}

	__forceinline friend GSVector8i operator >> (const GSVector8i& v, const int i)
	{
		return GSVector8i(_mm256_srli_epi32(v, i));
	}

	__forceinline friend GSVector8i operator & (const GSVector8i& v1, const GSVector8i& v2)
	{
		return GSVector8i(_mm256_and_si256(v1, v2));
	}

	__forceinline friend GSVector8i operator | (const GSVector8i& v1, const GSVector8i& v2)
	{
		return GSVector8i(_mm256_or_si256(v1, v2));
	}

	__forceinline friend GSVector8i operator ^ (const GSVector8i& v1, const GSVector8i& v2)
	{
		return GSVector8i(_mm256_xor_si256(v1, v2));
	}

	__forceinline friend GSVector8i operator & (const GSVector8i& v, int i)
	{
		return v & GSVector8i(i);
	}

	__forceinline friend GSVector8i operator | (const GSVector8i& v, int i)
	{
		return v | GSVector8i(i);
	}

	__forceinline friend GSVector8i operator ^ (const GSVector8i& v, int i)
	{
		return v ^ GSVector8i(i);
	}

	__forceinline friend GSVector8i operator ~ (const GSVector8i& v)
	{
		return v ^ (v == v);
	}

	__forceinline friend GSVector8i operator == (const GSVector8i& v1, const GSVector8i& v2)
	{
		return GSVector8i(_mm256_cmpeq_epi32(v1, v2));
	}

	__forceinline friend GSVector8i operator != (const GSVector8i& v1, const GSVector8i& v2)
	{
		return ~(v1 == v2);
	}

	__forceinline friend GSVector8i operator > (const GSVector8i& v1, const GSVector8i& v2)
	{
		return GSVector8i(_mm256_cmpgt_epi32(v1, v2));
	}

	__forceinline friend GSVector8i operator < (const GSVector8i& v1, const GSVector8i& v2)
	{
		return GSVector8i(_mm256_cmpgt_epi32(v2, v1));
	}

	__forceinline friend GSVector8i operator >= (const GSVector8i& v1, const GSVector8i& v2)
	{
		return (v1 > v2) | (v1 == v2);
	}

	__forceinline friend GSVector8i operator <= (const GSVector8i& v1, const GSVector8i& v2)
	{
		return (v1 < v2) | (v1 == v2);
	}

	// x = v[31:0] / v[159:128]
	// y = v[63:32] / v[191:160]
	// z = v[95:64] / v[223:192]
	// w = v[127:96] / v[255:224]

	#define VECTOR8i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, ws, wn) \
		__forceinline GSVector8i xs##ys##zs##ws() const {return GSVector8i(_mm256_shuffle_epi32(m, _MM_SHUFFLE(wn, zn, yn, xn)));} \
		__forceinline GSVector8i xs##ys##zs##ws##l() const {return GSVector8i(_mm256_shufflelo_epi16(m, _MM_SHUFFLE(wn, zn, yn, xn)));} \
		__forceinline GSVector8i xs##ys##zs##ws##h() const {return GSVector8i(_mm256_shufflehi_epi16(m, _MM_SHUFFLE(wn, zn, yn, xn)));} \
		__forceinline GSVector8i xs##ys##zs##ws##lh() const {return GSVector8i(_mm256_shufflehi_epi16(_mm256_shufflelo_epi16(m, _MM_SHUFFLE(wn, zn, yn, xn)), _MM_SHUFFLE(wn, zn, yn, xn)));} \

	#define VECTOR8i_SHUFFLE_3(xs, xn, ys, yn, zs, zn) \
		VECTOR8i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, x, 0) \
		VECTOR8i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, y, 1) \
		VECTOR8i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, z, 2) \
		VECTOR8i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, w, 3) \

	#define VECTOR8i_SHUFFLE_2(xs, xn, ys, yn) \
		VECTOR8i_SHUFFLE_3(xs, xn, ys, yn, x, 0) \
		VECTOR8i_SHUFFLE_3(xs, xn, ys, yn, y, 1) \
		VECTOR8i_SHUFFLE_3(xs, xn, ys, yn, z, 2) \
		VECTOR8i_SHUFFLE_3(xs, xn, ys, yn, w, 3) \

	#define VECTOR8i_SHUFFLE_1(xs, xn) \
		VECTOR8i_SHUFFLE_2(xs, xn, x, 0) \
		VECTOR8i_SHUFFLE_2(xs, xn, y, 1) \
		VECTOR8i_SHUFFLE_2(xs, xn, z, 2) \
		VECTOR8i_SHUFFLE_2(xs, xn, w, 3) \

	VECTOR8i_SHUFFLE_1(x, 0)
	VECTOR8i_SHUFFLE_1(y, 1)
	VECTOR8i_SHUFFLE_1(z, 2)
	VECTOR8i_SHUFFLE_1(w, 3)

	// a = v0[127:0]
	// b = v0[255:128]
	// c = v1[127:0]
	// d = v1[255:128]
	// _ = 0

	#define VECTOR8i_PERMUTE128_2(as, an, bs, bn) \
		__forceinline GSVector8i as##bs() const {return GSVector8i(_mm256_permute2x128_si256(m, m, an | (bn << 4)));} \
		__forceinline GSVector8i as##bs(const GSVector8i& v) const {return GSVector8i(_mm256_permute2x128_si256(m, v.m, an | (bn << 4)));} \

	#define VECTOR8i_PERMUTE128_1(as, an) \
		VECTOR8i_PERMUTE128_2(as, an, a, 0) \
		VECTOR8i_PERMUTE128_2(as, an, b, 1) \
		VECTOR8i_PERMUTE128_2(as, an, c, 2) \
		VECTOR8i_PERMUTE128_2(as, an, d, 3) \
		VECTOR8i_PERMUTE128_2(as, an, _, 8) \

	VECTOR8i_PERMUTE128_1(a, 0)
	VECTOR8i_PERMUTE128_1(b, 1)
	VECTOR8i_PERMUTE128_1(c, 2)
	VECTOR8i_PERMUTE128_1(d, 3)
	VECTOR8i_PERMUTE128_1(_, 8)

	// a = v[63:0]
	// b = v[127:64]
	// c = v[191:128]
	// d = v[255:192]

	#define VECTOR8i_PERMUTE64_4(as, an, bs, bn, cs, cn, ds, dn) \
		__forceinline GSVector8i as##bs##cs##ds() const {return GSVector8i(_mm256_permute4x64_epi64(m, _MM_SHUFFLE(dn, cn, bn, an)));} \

	#define VECTOR8i_PERMUTE64_3(as, an, bs, bn, cs, cn) \
		VECTOR8i_PERMUTE64_4(as, an, bs, bn, cs, cn, a, 0) \
		VECTOR8i_PERMUTE64_4(as, an, bs, bn, cs, cn, b, 1) \
		VECTOR8i_PERMUTE64_4(as, an, bs, bn, cs, cn, c, 2) \
		VECTOR8i_PERMUTE64_4(as, an, bs, bn, cs, cn, d, 3) \

	#define VECTOR8i_PERMUTE64_2(as, an, bs, bn) \
		VECTOR8i_PERMUTE64_3(as, an, bs, bn, a, 0) \
		VECTOR8i_PERMUTE64_3(as, an, bs, bn, b, 1) \
		VECTOR8i_PERMUTE64_3(as, an, bs, bn, c, 2) \
		VECTOR8i_PERMUTE64_3(as, an, bs, bn, d, 3) \

	#define VECTOR8i_PERMUTE64_1(as, an) \
		VECTOR8i_PERMUTE64_2(as, an, a, 0) \
		VECTOR8i_PERMUTE64_2(as, an, b, 1) \
		VECTOR8i_PERMUTE64_2(as, an, c, 2) \
		VECTOR8i_PERMUTE64_2(as, an, d, 3) \

	VECTOR8i_PERMUTE64_1(a, 0)
	VECTOR8i_PERMUTE64_1(b, 1)
	VECTOR8i_PERMUTE64_1(c, 2)
	VECTOR8i_PERMUTE64_1(d, 3)

	__forceinline GSVector8i permute32(const GSVector8i& mask) const
	{
		return GSVector8i(_mm256_permutevar8x32_epi32(m, mask));
	}

	__forceinline GSVector8i broadcast8() const
	{
		return GSVector8i(_mm256_broadcastb_epi8(_mm256_castsi256_si128(m)));
	}

	__forceinline GSVector8i broadcast16() const
	{
		return GSVector8i(_mm256_broadcastw_epi16(_mm256_castsi256_si128(m)));
	}

	__forceinline GSVector8i broadcast32() const
	{
		return GSVector8i(_mm256_broadcastd_epi32(_mm256_castsi256_si128(m)));
	}

	__forceinline GSVector8i broadcast64() const
	{
		return GSVector8i(_mm256_broadcastq_epi64(_mm256_castsi256_si128(m)));
	}

	__forceinline static GSVector8i broadcast8(const GSVector4i& v)
	{
		return GSVector8i(_mm256_broadcastb_epi8(v.m));
	}

	__forceinline static GSVector8i broadcast16(const GSVector4i& v)
	{
		return GSVector8i(_mm256_broadcastw_epi16(v.m));
	}

	__forceinline static GSVector8i broadcast32(const GSVector4i& v)
	{
		return GSVector8i(_mm256_broadcastd_epi32(v.m));
	}

	__forceinline static GSVector8i broadcast64(const GSVector4i& v)
	{
		return GSVector8i(_mm256_broadcastq_epi64(v.m));
	}

	__forceinline static GSVector8i broadcast128(const GSVector4i& v)
	{
		// this one only has m128 source op, it will be saved to a temp on stack if the compiler is not smart enough and use the address of v directly (<= vs2012u3rc2)

		return GSVector8i(_mm256_broadcastsi128_si256(v)); // fastest
		//return GSVector8i(v); // almost as fast as broadcast
		//return cast(v).insert<1>(v); // slow
		//return cast(v).aa(); // slowest
	}

	__forceinline static GSVector8i broadcast8(const void* p)
	{
		return GSVector8i(_mm256_broadcastb_epi8(_mm_cvtsi32_si128(*(const int*)p)));
	}

	__forceinline static GSVector8i broadcast16(const void* p)
	{
		return GSVector8i(_mm256_broadcastw_epi16(_mm_cvtsi32_si128(*(const int*)p)));
	}

	__forceinline static GSVector8i broadcast32(const void* p)
	{
		return GSVector8i(_mm256_broadcastd_epi32(_mm_cvtsi32_si128(*(const int*)p)));
	}

	__forceinline static GSVector8i broadcast64(const void* p)
	{
		return GSVector8i(_mm256_broadcastq_epi64(_mm_loadl_epi64((const __m128i*)p)));
	}

	__forceinline static GSVector8i broadcast128(const void* p)
	{
		return GSVector8i(_mm256_broadcastsi128_si256(*(const __m128i*)p));
	}

	__forceinline static GSVector8i zero() {return GSVector8i(_mm256_setzero_si256());}

	__forceinline static GSVector8i xffffffff() {return zero() == zero();}

	__forceinline static GSVector8i x00000001() {return xffffffff().srl32(31);}
	__forceinline static GSVector8i x00000003() {return xffffffff().srl32(30);}
	__forceinline static GSVector8i x00000007() {return xffffffff().srl32(29);}
	__forceinline static GSVector8i x0000000f() {return xffffffff().srl32(28);}
	__forceinline static GSVector8i x0000001f() {return xffffffff().srl32(27);}
	__forceinline static GSVector8i x0000003f() {return xffffffff().srl32(26);}
	__forceinline static GSVector8i x0000007f() {return xffffffff().srl32(25);}
	__forceinline static GSVector8i x000000ff() {return xffffffff().srl32(24);}
	__forceinline static GSVector8i x000001ff() {return xffffffff().srl32(23);}
	__forceinline static GSVector8i x000003ff() {return xffffffff().srl32(22);}
	__forceinline static GSVector8i x000007ff() {return xffffffff().srl32(21);}
	__forceinline static GSVector8i x00000fff() {return xffffffff().srl32(20);}
	__forceinline static GSVector8i x00001fff() {return xffffffff().srl32(19);}
	__forceinline static GSVector8i x00003fff() {return xffffffff().srl32(18);}
	__forceinline static GSVector8i x00007fff() {return xffffffff().srl32(17);}
	__forceinline static GSVector8i x0000ffff() {return xffffffff().srl32(16);}
	__forceinline static GSVector8i x0001ffff() {return xffffffff().srl32(15);}
	__forceinline static GSVector8i x0003ffff() {return xffffffff().srl32(14);}
	__forceinline static GSVector8i x0007ffff() {return xffffffff().srl32(13);}
	__forceinline static GSVector8i x000fffff() {return xffffffff().srl32(12);}
	__forceinline static GSVector8i x001fffff() {return xffffffff().srl32(11);}
	__forceinline static GSVector8i x003fffff() {return xffffffff().srl32(10);}
	__forceinline static GSVector8i x007fffff() {return xffffffff().srl32( 9);}
	__forceinline static GSVector8i x00ffffff() {return xffffffff().srl32( 8);}
	__forceinline static GSVector8i x01ffffff() {return xffffffff().srl32( 7);}
	__forceinline static GSVector8i x03ffffff() {return xffffffff().srl32( 6);}
	__forceinline static GSVector8i x07ffffff() {return xffffffff().srl32( 5);}
	__forceinline static GSVector8i x0fffffff() {return xffffffff().srl32( 4);}
	__forceinline static GSVector8i x1fffffff() {return xffffffff().srl32( 3);}
	__forceinline static GSVector8i x3fffffff() {return xffffffff().srl32( 2);}
	__forceinline static GSVector8i x7fffffff() {return xffffffff().srl32( 1);}

	__forceinline static GSVector8i x80000000() {return xffffffff().sll32(31);}
	__forceinline static GSVector8i xc0000000() {return xffffffff().sll32(30);}
	__forceinline static GSVector8i xe0000000() {return xffffffff().sll32(29);}
	__forceinline static GSVector8i xf0000000() {return xffffffff().sll32(28);}
	__forceinline static GSVector8i xf8000000() {return xffffffff().sll32(27);}
	__forceinline static GSVector8i xfc000000() {return xffffffff().sll32(26);}
	__forceinline static GSVector8i xfe000000() {return xffffffff().sll32(25);}
	__forceinline static GSVector8i xff000000() {return xffffffff().sll32(24);}
	__forceinline static GSVector8i xff800000() {return xffffffff().sll32(23);}
	__forceinline static GSVector8i xffc00000() {return xffffffff().sll32(22);}
	__forceinline static GSVector8i xffe00000() {return xffffffff().sll32(21);}
	__forceinline static GSVector8i xfff00000() {return xffffffff().sll32(20);}
	__forceinline static GSVector8i xfff80000() {return xffffffff().sll32(19);}
	__forceinline static GSVector8i xfffc0000() {return xffffffff().sll32(18);}
	__forceinline static GSVector8i xfffe0000() {return xffffffff().sll32(17);}
	__forceinline static GSVector8i xffff0000() {return xffffffff().sll32(16);}
	__forceinline static GSVector8i xffff8000() {return xffffffff().sll32(15);}
	__forceinline static GSVector8i xffffc000() {return xffffffff().sll32(14);}
	__forceinline static GSVector8i xffffe000() {return xffffffff().sll32(13);}
	__forceinline static GSVector8i xfffff000() {return xffffffff().sll32(12);}
	__forceinline static GSVector8i xfffff800() {return xffffffff().sll32(11);}
	__forceinline static GSVector8i xfffffc00() {return xffffffff().sll32(10);}
	__forceinline static GSVector8i xfffffe00() {return xffffffff().sll32( 9);}
	__forceinline static GSVector8i xffffff00() {return xffffffff().sll32( 8);}
	__forceinline static GSVector8i xffffff80() {return xffffffff().sll32( 7);}
	__forceinline static GSVector8i xffffffc0() {return xffffffff().sll32( 6);}
	__forceinline static GSVector8i xffffffe0() {return xffffffff().sll32( 5);}
	__forceinline static GSVector8i xfffffff0() {return xffffffff().sll32( 4);}
	__forceinline static GSVector8i xfffffff8() {return xffffffff().sll32( 3);}
	__forceinline static GSVector8i xfffffffc() {return xffffffff().sll32( 2);}
	__forceinline static GSVector8i xfffffffe() {return xffffffff().sll32( 1);}

	__forceinline static GSVector8i x0001() {return xffffffff().srl16(15);}
	__forceinline static GSVector8i x0003() {return xffffffff().srl16(14);}
	__forceinline static GSVector8i x0007() {return xffffffff().srl16(13);}
	__forceinline static GSVector8i x000f() {return xffffffff().srl16(12);}
	__forceinline static GSVector8i x001f() {return xffffffff().srl16(11);}
	__forceinline static GSVector8i x003f() {return xffffffff().srl16(10);}
	__forceinline static GSVector8i x007f() {return xffffffff().srl16( 9);}
	__forceinline static GSVector8i x00ff() {return xffffffff().srl16( 8);}
	__forceinline static GSVector8i x01ff() {return xffffffff().srl16( 7);}
	__forceinline static GSVector8i x03ff() {return xffffffff().srl16( 6);}
	__forceinline static GSVector8i x07ff() {return xffffffff().srl16( 5);}
	__forceinline static GSVector8i x0fff() {return xffffffff().srl16( 4);}
	__forceinline static GSVector8i x1fff() {return xffffffff().srl16( 3);}
	__forceinline static GSVector8i x3fff() {return xffffffff().srl16( 2);}
	__forceinline static GSVector8i x7fff() {return xffffffff().srl16( 1);}

	__forceinline static GSVector8i x8000() {return xffffffff().sll16(15);}
	__forceinline static GSVector8i xc000() {return xffffffff().sll16(14);}
	__forceinline static GSVector8i xe000() {return xffffffff().sll16(13);}
	__forceinline static GSVector8i xf000() {return xffffffff().sll16(12);}
	__forceinline static GSVector8i xf800() {return xffffffff().sll16(11);}
	__forceinline static GSVector8i xfc00() {return xffffffff().sll16(10);}
	__forceinline static GSVector8i xfe00() {return xffffffff().sll16( 9);}
	__forceinline static GSVector8i xff00() {return xffffffff().sll16( 8);}
	__forceinline static GSVector8i xff80() {return xffffffff().sll16( 7);}
	__forceinline static GSVector8i xffc0() {return xffffffff().sll16( 6);}
	__forceinline static GSVector8i xffe0() {return xffffffff().sll16( 5);}
	__forceinline static GSVector8i xfff0() {return xffffffff().sll16( 4);}
	__forceinline static GSVector8i xfff8() {return xffffffff().sll16( 3);}
	__forceinline static GSVector8i xfffc() {return xffffffff().sll16( 2);}
	__forceinline static GSVector8i xfffe() {return xffffffff().sll16( 1);}

	__forceinline static GSVector8i xffffffff(const GSVector8i& v) {return v == v;}

	__forceinline static GSVector8i x00000001(const GSVector8i& v) {return xffffffff(v).srl32(31);}
	__forceinline static GSVector8i x00000003(const GSVector8i& v) {return xffffffff(v).srl32(30);}
	__forceinline static GSVector8i x00000007(const GSVector8i& v) {return xffffffff(v).srl32(29);}
	__forceinline static GSVector8i x0000000f(const GSVector8i& v) {return xffffffff(v).srl32(28);}
	__forceinline static GSVector8i x0000001f(const GSVector8i& v) {return xffffffff(v).srl32(27);}
	__forceinline static GSVector8i x0000003f(const GSVector8i& v) {return xffffffff(v).srl32(26);}
	__forceinline static GSVector8i x0000007f(const GSVector8i& v) {return xffffffff(v).srl32(25);}
	__forceinline static GSVector8i x000000ff(const GSVector8i& v) {return xffffffff(v).srl32(24);}
	__forceinline static GSVector8i x000001ff(const GSVector8i& v) {return xffffffff(v).srl32(23);}
	__forceinline static GSVector8i x000003ff(const GSVector8i& v) {return xffffffff(v).srl32(22);}
	__forceinline static GSVector8i x000007ff(const GSVector8i& v) {return xffffffff(v).srl32(21);}
	__forceinline static GSVector8i x00000fff(const GSVector8i& v) {return xffffffff(v).srl32(20);}
	__forceinline static GSVector8i x00001fff(const GSVector8i& v) {return xffffffff(v).srl32(19);}
	__forceinline static GSVector8i x00003fff(const GSVector8i& v) {return xffffffff(v).srl32(18);}
	__forceinline static GSVector8i x00007fff(const GSVector8i& v) {return xffffffff(v).srl32(17);}
	__forceinline static GSVector8i x0000ffff(const GSVector8i& v) {return xffffffff(v).srl32(16);}
	__forceinline static GSVector8i x0001ffff(const GSVector8i& v) {return xffffffff(v).srl32(15);}
	__forceinline static GSVector8i x0003ffff(const GSVector8i& v) {return xffffffff(v).srl32(14);}
	__forceinline static GSVector8i x0007ffff(const GSVector8i& v) {return xffffffff(v).srl32(13);}
	__forceinline static GSVector8i x000fffff(const GSVector8i& v) {return xffffffff(v).srl32(12);}
	__forceinline static GSVector8i x001fffff(const GSVector8i& v) {return xffffffff(v).srl32(11);}
	__forceinline static GSVector8i x003fffff(const GSVector8i& v) {return xffffffff(v).srl32(10);}
	__forceinline static GSVector8i x007fffff(const GSVector8i& v) {return xffffffff(v).srl32( 9);}
	__forceinline static GSVector8i x00ffffff(const GSVector8i& v) {return xffffffff(v).srl32( 8);}
	__forceinline static GSVector8i x01ffffff(const GSVector8i& v) {return xffffffff(v).srl32( 7);}
	__forceinline static GSVector8i x03ffffff(const GSVector8i& v) {return xffffffff(v).srl32( 6);}
	__forceinline static GSVector8i x07ffffff(const GSVector8i& v) {return xffffffff(v).srl32( 5);}
	__forceinline static GSVector8i x0fffffff(const GSVector8i& v) {return xffffffff(v).srl32( 4);}
	__forceinline static GSVector8i x1fffffff(const GSVector8i& v) {return xffffffff(v).srl32( 3);}
	__forceinline static GSVector8i x3fffffff(const GSVector8i& v) {return xffffffff(v).srl32( 2);}
	__forceinline static GSVector8i x7fffffff(const GSVector8i& v) {return xffffffff(v).srl32( 1);}

	__forceinline static GSVector8i x80000000(const GSVector8i& v) {return xffffffff(v).sll32(31);}
	__forceinline static GSVector8i xc0000000(const GSVector8i& v) {return xffffffff(v).sll32(30);}
	__forceinline static GSVector8i xe0000000(const GSVector8i& v) {return xffffffff(v).sll32(29);}
	__forceinline static GSVector8i xf0000000(const GSVector8i& v) {return xffffffff(v).sll32(28);}
	__forceinline static GSVector8i xf8000000(const GSVector8i& v) {return xffffffff(v).sll32(27);}
	__forceinline static GSVector8i xfc000000(const GSVector8i& v) {return xffffffff(v).sll32(26);}
	__forceinline static GSVector8i xfe000000(const GSVector8i& v) {return xffffffff(v).sll32(25);}
	__forceinline static GSVector8i xff000000(const GSVector8i& v) {return xffffffff(v).sll32(24);}
	__forceinline static GSVector8i xff800000(const GSVector8i& v) {return xffffffff(v).sll32(23);}
	__forceinline static GSVector8i xffc00000(const GSVector8i& v) {return xffffffff(v).sll32(22);}
	__forceinline static GSVector8i xffe00000(const GSVector8i& v) {return xffffffff(v).sll32(21);}
	__forceinline static GSVector8i xfff00000(const GSVector8i& v) {return xffffffff(v).sll32(20);}
	__forceinline static GSVector8i xfff80000(const GSVector8i& v) {return xffffffff(v).sll32(19);}
	__forceinline static GSVector8i xfffc0000(const GSVector8i& v) {return xffffffff(v).sll32(18);}
	__forceinline static GSVector8i xfffe0000(const GSVector8i& v) {return xffffffff(v).sll32(17);}
	__forceinline static GSVector8i xffff0000(const GSVector8i& v) {return xffffffff(v).sll32(16);}
	__forceinline static GSVector8i xffff8000(const GSVector8i& v) {return xffffffff(v).sll32(15);}
	__forceinline static GSVector8i xffffc000(const GSVector8i& v) {return xffffffff(v).sll32(14);}
	__forceinline static GSVector8i xffffe000(const GSVector8i& v) {return xffffffff(v).sll32(13);}
	__forceinline static GSVector8i xfffff000(const GSVector8i& v) {return xffffffff(v).sll32(12);}
	__forceinline static GSVector8i xfffff800(const GSVector8i& v) {return xffffffff(v).sll32(11);}
	__forceinline static GSVector8i xfffffc00(const GSVector8i& v) {return xffffffff(v).sll32(10);}
	__forceinline static GSVector8i xfffffe00(const GSVector8i& v) {return xffffffff(v).sll32( 9);}
	__forceinline static GSVector8i xffffff00(const GSVector8i& v) {return xffffffff(v).sll32( 8);}
	__forceinline static GSVector8i xffffff80(const GSVector8i& v) {return xffffffff(v).sll32( 7);}
	__forceinline static GSVector8i xffffffc0(const GSVector8i& v) {return xffffffff(v).sll32( 6);}
	__forceinline static GSVector8i xffffffe0(const GSVector8i& v) {return xffffffff(v).sll32( 5);}
	__forceinline static GSVector8i xfffffff0(const GSVector8i& v) {return xffffffff(v).sll32( 4);}
	__forceinline static GSVector8i xfffffff8(const GSVector8i& v) {return xffffffff(v).sll32( 3);}
	__forceinline static GSVector8i xfffffffc(const GSVector8i& v) {return xffffffff(v).sll32( 2);}
	__forceinline static GSVector8i xfffffffe(const GSVector8i& v) {return xffffffff(v).sll32( 1);}

	__forceinline static GSVector8i x0001(const GSVector8i& v) {return xffffffff(v).srl16(15);}
	__forceinline static GSVector8i x0003(const GSVector8i& v) {return xffffffff(v).srl16(14);}
	__forceinline static GSVector8i x0007(const GSVector8i& v) {return xffffffff(v).srl16(13);}
	__forceinline static GSVector8i x000f(const GSVector8i& v) {return xffffffff(v).srl16(12);}
	__forceinline static GSVector8i x001f(const GSVector8i& v) {return xffffffff(v).srl16(11);}
	__forceinline static GSVector8i x003f(const GSVector8i& v) {return xffffffff(v).srl16(10);}
	__forceinline static GSVector8i x007f(const GSVector8i& v) {return xffffffff(v).srl16( 9);}
	__forceinline static GSVector8i x00ff(const GSVector8i& v) {return xffffffff(v).srl16( 8);}
	__forceinline static GSVector8i x01ff(const GSVector8i& v) {return xffffffff(v).srl16( 7);}
	__forceinline static GSVector8i x03ff(const GSVector8i& v) {return xffffffff(v).srl16( 6);}
	__forceinline static GSVector8i x07ff(const GSVector8i& v) {return xffffffff(v).srl16( 5);}
	__forceinline static GSVector8i x0fff(const GSVector8i& v) {return xffffffff(v).srl16( 4);}
	__forceinline static GSVector8i x1fff(const GSVector8i& v) {return xffffffff(v).srl16( 3);}
	__forceinline static GSVector8i x3fff(const GSVector8i& v) {return xffffffff(v).srl16( 2);}
	__forceinline static GSVector8i x7fff(const GSVector8i& v) {return xffffffff(v).srl16( 1);}

	__forceinline static GSVector8i x8000(const GSVector8i& v) {return xffffffff(v).sll16(15);}
	__forceinline static GSVector8i xc000(const GSVector8i& v) {return xffffffff(v).sll16(14);}
	__forceinline static GSVector8i xe000(const GSVector8i& v) {return xffffffff(v).sll16(13);}
	__forceinline static GSVector8i xf000(const GSVector8i& v) {return xffffffff(v).sll16(12);}
	__forceinline static GSVector8i xf800(const GSVector8i& v) {return xffffffff(v).sll16(11);}
	__forceinline static GSVector8i xfc00(const GSVector8i& v) {return xffffffff(v).sll16(10);}
	__forceinline static GSVector8i xfe00(const GSVector8i& v) {return xffffffff(v).sll16( 9);}
	__forceinline static GSVector8i xff00(const GSVector8i& v) {return xffffffff(v).sll16( 8);}
	__forceinline static GSVector8i xff80(const GSVector8i& v) {return xffffffff(v).sll16( 7);}
	__forceinline static GSVector8i xffc0(const GSVector8i& v) {return xffffffff(v).sll16( 6);}
	__forceinline static GSVector8i xffe0(const GSVector8i& v) {return xffffffff(v).sll16( 5);}
	__forceinline static GSVector8i xfff0(const GSVector8i& v) {return xffffffff(v).sll16( 4);}
	__forceinline static GSVector8i xfff8(const GSVector8i& v) {return xffffffff(v).sll16( 3);}
	__forceinline static GSVector8i xfffc(const GSVector8i& v) {return xffffffff(v).sll16( 2);}
	__forceinline static GSVector8i xfffe(const GSVector8i& v) {return xffffffff(v).sll16( 1);}

	__forceinline static GSVector8i xff(int n) {return m_xff[n];}
	__forceinline static GSVector8i x0f(int n) {return m_x0f[n];}
};

#endif

#if _M_SSE >= 0x500

__aligned(class, 32) GSVector8
{
public:
	union
	{
		struct {float x0, y0, z0, w0, x1, y1, z1, w1;};
		struct {float r0, g0, b0, a0, r1, g1, b1, a1;};
		float v[8];
		float f32[8];
		int8 i8[32];
		int16 i16[16];
		int32 i32[8];
		int64 i64[4];
		uint8 u8[32];
		uint16 u16[16];
		uint32 u32[8];
		uint64 u64[4];
		__m256 m;
		__m128 m0, m1;
	};

	static const GSVector8 m_half;
	static const GSVector8 m_one;
	static const GSVector8 m_x7fffffff;
	static const GSVector8 m_x80000000;
	static const GSVector8 m_x4b000000;
	static const GSVector8 m_x4f800000;

	__forceinline GSVector8() 
	{
	}

	__forceinline GSVector8(float x0, float y0, float z0, float w0, float x1, float y1, float z1, float w1)
	{
		m = _mm256_set_ps(w1, z1, y1, x1, w0, z0, y0, x0);
	}

	__forceinline GSVector8(int x0, int y0, int z0, int w0, int x1, int y1, int z1, int w1)
	{
		m = _mm256_cvtepi32_ps(_mm256_set_epi32(w1, z1, y1, x1, w0, z0, y0, x0));
	}

	__forceinline GSVector8(__m128 m0, __m128 m1)
	{
		#if 0 // _MSC_VER >= 1700 
		
		this->m = _mm256_permute2f128_ps(_mm256_castps128_ps256(m0), _mm256_castps128_ps256(m1), 0x20);

		#else

		this->m = zero().insert<0>(m0).insert<1>(m1);

		#endif
	}

	__forceinline GSVector8(const GSVector8& v)
	{
		m = v.m;
	}

	__forceinline explicit GSVector8(float f)
	{
		*this = f;
	}

	__forceinline explicit GSVector8(int i)
	{
		#if _M_SSE >= 0x501

		m = _mm256_cvtepi32_ps(_mm256_broadcastd_epi32(_mm_cvtsi32_si128(i)));

		#else 

		GSVector4i v((int)i);

		*this = GSVector4(v);

		#endif
	}

	__forceinline explicit GSVector8(__m128 m)
	{
		*this = m;
	}

	__forceinline explicit GSVector8(__m256 m)
	{
		this->m = m;
	}

	#if _M_SSE >= 0x501

	__forceinline explicit GSVector8(const GSVector8i& v);

	__forceinline static GSVector8 cast(const GSVector8i& v);

	#endif

	__forceinline static GSVector8 cast(const GSVector4& v);
	__forceinline static GSVector8 cast(const GSVector4i& v);

	__forceinline void operator = (const GSVector8& v)
	{
		m = v.m;
	}

	__forceinline void operator = (float f)
	{
		#if _M_SSE >= 0x501

		m =  _mm256_broadcastss_ps(_mm_load_ss(&f));

		#else

		m = _mm256_set1_ps(f);

		#endif
	}

	__forceinline void operator = (__m128 m)
	{
		this->m = _mm256_insertf128_ps(_mm256_castps128_ps256(m), m, 1);
	}

	__forceinline void operator = (__m256 m)
	{
		this->m = m;
	}

	__forceinline operator __m256() const
	{
		return m;
	}

	__forceinline GSVector8 abs() const
	{
		#if _M_SSE >= 0x501

		return *this & cast(GSVector8i::x7fffffff());

		#else
		
		return *this & m_x7fffffff;

		#endif
	}

	__forceinline GSVector8 neg() const
	{
		#if _M_SSE >= 0x501

		return *this ^ cast(GSVector8i::x80000000());

		#else
		
		return *this ^ m_x80000000;

		#endif
	}

	__forceinline GSVector8 rcp() const
	{
		return GSVector8(_mm256_rcp_ps(m));
	}

	__forceinline GSVector8 rcpnr() const
	{
		GSVector8 v = rcp();

		return (v + v) - (v * v) * *this;
	}

	template<int mode> __forceinline GSVector8 round() const
	{
		return GSVector8(_mm256_round_ps(m, mode));
	}

	__forceinline GSVector8 floor() const
	{
		return round<Round_NegInf>();
	}

	__forceinline GSVector8 ceil() const
	{
		return round<Round_PosInf>();
	}

	#if _M_SSE >= 0x501

	#define LOG8_POLY0(x, c0) GSVector8(c0)
	#define LOG8_POLY1(x, c0, c1) (LOG8_POLY0(x, c1).madd(x, GSVector8(c0)))
	#define LOG8_POLY2(x, c0, c1, c2) (LOG8_POLY1(x, c1, c2).madd(x, GSVector8(c0)))
	#define LOG8_POLY3(x, c0, c1, c2, c3) (LOG8_POLY2(x, c1, c2, c3).madd(x, GSVector8(c0)))
	#define LOG8_POLY4(x, c0, c1, c2, c3, c4) (LOG8_POLY3(x, c1, c2, c3, c4).madd(x, GSVector8(c0)))
	#define LOG8_POLY5(x, c0, c1, c2, c3, c4, c5) (LOG8_POLY4(x, c1, c2, c3, c4, c5).madd(x, GSVector8(c0)))

	__forceinline GSVector8 log2(int precision = 5) const
	{
		// NOTE: see GSVector4::log2

		GSVector8 one = m_one;

		GSVector8i i = GSVector8i::cast(*this);

		GSVector8 e = GSVector8(((i << 1) >> 24) - GSVector8i::x0000007f());
		GSVector8 m = GSVector8::cast((i << 9) >> 9) | one;

		GSVector8 p;

		switch(precision)
		{
		case 3:
			p = LOG8_POLY2(m, 2.28330284476918490682f, -1.04913055217340124191f, 0.204446009836232697516f);
			break;
		case 4:
			p = LOG8_POLY3(m, 2.61761038894603480148f, -1.75647175389045657003f, 0.688243882994381274313f, -0.107254423828329604454f);
			break;
		default:
		case 5:
			p = LOG8_POLY4(m, 2.8882704548164776201f, -2.52074962577807006663f, 1.48116647521213171641f, -0.465725644288844778798f, 0.0596515482674574969533f);
			break;
		case 6:
			p = LOG8_POLY5(m, 3.1157899f, -3.3241990f, 2.5988452f, -1.2315303f,  3.1821337e-1f, -3.4436006e-2f);
			break;
		}

		// This effectively increases the polynomial degree by one, but ensures that log2(1) == 0

		p = p * (m - one);

		return p + e;
	}

	#endif

	__forceinline GSVector8 madd(const GSVector8& a, const GSVector8& b) const
	{
		#if 0//_M_SSE >= 0x501

		return GSVector8(_mm256_fmadd_ps(m, a, b));
		
		#else
		
		return *this * a + b;
		
		#endif
	}

	__forceinline GSVector8 msub(const GSVector8& a, const GSVector8& b) const
	{
		#if 0//_M_SSE >= 0x501

		return GSVector8(_mm256_fmsub_ps(m, a, b));
		
		#else
		
		return *this * a - b;
		
		#endif
	}

	__forceinline GSVector8 nmadd(const GSVector8& a, const GSVector8& b) const
	{
		#if 0//_M_SSE >= 0x501

		return GSVector8(_mm256_fnmadd_ps(m, a, b));
		
		#else
		
		return b - *this * a;
		
		#endif
	}

	__forceinline GSVector8 nmsub(const GSVector8& a, const GSVector8& b) const
	{
		#if 0//_M_SSE >= 0x501

		return GSVector8(_mm256_fnmsub_ps(m, a, b));
		
		#else

		return -b - *this * a;

		#endif
	}

	__forceinline GSVector8 addm(const GSVector8& a, const GSVector8& b) const
	{
		return a.madd(b, *this); // *this + a * b
	}

	__forceinline GSVector8 subm(const GSVector8& a, const GSVector8& b) const
	{
		return a.nmadd(b, *this); // *this - a * b
	}

	__forceinline GSVector8 hadd() const
	{
		return GSVector8(_mm256_hadd_ps(m, m));
	}

	__forceinline GSVector8 hadd(const GSVector8& v) const
	{
		return GSVector8(_mm256_hadd_ps(m, v.m));
	}

	__forceinline GSVector8 hsub() const
	{
		return GSVector8(_mm256_hsub_ps(m, m));
	}

	__forceinline GSVector8 hsub(const GSVector8& v) const
	{
		return GSVector8(_mm256_hsub_ps(m, v.m));
	}

	template<int i> __forceinline GSVector8 dp(const GSVector8& v) const
	{
		return GSVector8(_mm256_dp_ps(m, v.m, i));
	}

	__forceinline GSVector8 sat(const GSVector8& a, const GSVector8& b) const
	{
		return GSVector8(_mm256_min_ps(_mm256_max_ps(m, a), b));
	}

	__forceinline GSVector8 sat(const GSVector8& a) const
	{
		return GSVector8(_mm256_min_ps(_mm256_max_ps(m, a.xyxy()), a.zwzw()));
	}

	__forceinline GSVector8 sat(const float scale = 255) const
	{
		return sat(zero(), GSVector8(scale));
	}

	__forceinline GSVector8 clamp(const float scale = 255) const
	{
		return min(GSVector8(scale));
	}

	__forceinline GSVector8 min(const GSVector8& a) const
	{
		return GSVector8(_mm256_min_ps(m, a));
	}

	__forceinline GSVector8 max(const GSVector8& a) const
	{
		return GSVector8(_mm256_max_ps(m, a));
	}

	template<int mask> __forceinline GSVector8 blend32(const GSVector8& a)  const
	{
		return GSVector8(_mm256_blend_ps(m, a, mask));
	}

	__forceinline GSVector8 blend32(const GSVector8& a, const GSVector8& mask)  const
	{
		return GSVector8(_mm256_blendv_ps(m, a, mask));
	}

	__forceinline GSVector8 upl(const GSVector8& a) const
	{
		return GSVector8(_mm256_unpacklo_ps(m, a));
	}

	__forceinline GSVector8 uph(const GSVector8& a) const
	{
		return GSVector8(_mm256_unpackhi_ps(m, a));
	}

	__forceinline GSVector8 upl64(const GSVector8& a) const
	{
		return GSVector8(_mm256_castpd_ps(_mm256_unpacklo_pd(_mm256_castps_pd(m), _mm256_castps_pd(a))));
	}

	__forceinline GSVector8 uph64(const GSVector8& a) const
	{
		return GSVector8(_mm256_castpd_ps(_mm256_unpackhi_pd(_mm256_castps_pd(m), _mm256_castps_pd(a))));
	}

	__forceinline GSVector8 l2h() const
	{
		return xyxy();
	}

	__forceinline GSVector8 h2l() const
	{
		return zwzw();
	}

	__forceinline GSVector8 andnot(const GSVector8& v) const
	{
		return GSVector8(_mm256_andnot_ps(v.m, m));
	}

	__forceinline int mask() const
	{
		return _mm256_movemask_ps(m);
	}

	__forceinline bool alltrue() const
	{
		return mask() == 0xff;
	}

	__forceinline bool allfalse() const
	{
		return _mm256_testz_ps(m, m) != 0;
	}
	

	template<int src, int dst> __forceinline GSVector8 insert32(const GSVector8& v) const
	{
		// TODO: use blendps when src == dst

		ASSERT(src < 4 && dst < 4); // not cross lane like extract32()

		switch(dst)
		{
		case 0:
			switch(src)
			{
			case 0: return yyxx(v).zxzw(*this);
			case 1: return yyyy(v).zxzw(*this);
			case 2: return yyzz(v).zxzw(*this);
			case 3: return yyww(v).zxzw(*this);
			default: __assume(0);
			}
			break;
		case 1:
			switch(src)
			{
			case 0: return xxxx(v).xzzw(*this);
			case 1: return xxyy(v).xzzw(*this);
			case 2: return xxzz(v).xzzw(*this);
			case 3: return xxww(v).xzzw(*this);
			default: __assume(0);
			}
			break;
		case 2:
			switch(src)
			{
			case 0: return xyzx(wwxx(v));
			case 1: return xyzx(wwyy(v));
			case 2: return xyzx(wwzz(v));
			case 3: return xyzx(wwww(v));
			default: __assume(0);
			}
			break;
		case 3:
			switch(src)
			{
			case 0: return xyxz(zzxx(v));
			case 1: return xyxz(zzyy(v));
			case 2: return xyxz(zzzz(v));
			case 3: return xyxz(zzww(v));
			default: __assume(0);
			}
			break;
		default:
			__assume(0);
		}

		return *this;
	}

	template<int i> __forceinline int extract32() const
	{
		ASSERT(i < 8);

		return extract<i / 4>().extract32<i & 3>();
	}

	template<int i> __forceinline GSVector8 insert(__m128 m) const
	{
		ASSERT(i < 2);

		return GSVector8(_mm256_insertf128_ps(this->m, m, i));
	}

	template<int i> __forceinline GSVector4 extract() const
	{
		ASSERT(i < 2);

		if(i == 0) return GSVector4(_mm256_castps256_ps128(m));

		return GSVector4(_mm256_extractf128_ps(m, i));
	}

	__forceinline static GSVector8 zero()
	{
		return GSVector8(_mm256_setzero_ps());
	}

	__forceinline static GSVector8 xffffffff()
	{
		return zero() == zero();
	}

	// TODO

	__forceinline static GSVector8 loadl(const void* p)
	{
		return GSVector8(_mm256_castps128_ps256(_mm_load_ps((float*)p)));
	}

	__forceinline static GSVector8 loadh(const void* p)
	{
		return zero().insert<1>(_mm_load_ps((float*)p));
	}

	__forceinline static GSVector8 loadh(const void* p, const GSVector8& v)
	{
		return GSVector8(_mm256_insertf128_ps(v, _mm_load_ps((float*)p), 1));
	}

	__forceinline static GSVector8 load(const void* pl, const void* ph)
	{
		return loadh(ph, loadl(pl));
	}

	template<bool aligned> __forceinline static GSVector8 load(const void* p)
	{
		return GSVector8(aligned ? _mm256_load_ps((const float*)p) : _mm256_loadu_ps((const float*)p));
	}

	// TODO

	__forceinline static void storel(void* p, const GSVector8& v)
	{
		_mm_store_ps((float*)p, _mm256_extractf128_ps(v.m, 0));
	}

	__forceinline static void storeh(void* p, const GSVector8& v)
	{
		_mm_store_ps((float*)p, _mm256_extractf128_ps(v.m, 1));
	}

	template<bool aligned> __forceinline static void store(void* p, const GSVector8& v)
	{
		if(aligned) _mm256_store_ps((float*)p, v.m);
		else _mm256_storeu_ps((float*)p, v.m);
	}

	//

	__forceinline static void zeroupper()
	{
		_mm256_zeroupper();
	}

	__forceinline static void zeroall()
	{
		_mm256_zeroall();
	}

	//

	__forceinline GSVector8 operator - () const
	{
		return neg();
	}

	__forceinline void operator += (const GSVector8& v)
	{
		m = _mm256_add_ps(m, v);
	}

	__forceinline void operator -= (const GSVector8& v)
	{
		m = _mm256_sub_ps(m, v);
	}

	__forceinline void operator *= (const GSVector8& v)
	{
		m = _mm256_mul_ps(m, v);
	}

	__forceinline void operator /= (const GSVector8& v)
	{
		m = _mm256_div_ps(m, v);
	}

	__forceinline void operator += (float f)
	{
		*this += GSVector8(f);
	}

	__forceinline void operator -= (float f)
	{
		*this -= GSVector8(f);
	}

	__forceinline void operator *= (float f)
	{
		*this *= GSVector8(f);
	}

	__forceinline void operator /= (float f)
	{
		*this /= GSVector8(f);
	}

	__forceinline void operator &= (const GSVector8& v)
	{
		m = _mm256_and_ps(m, v);
	}

	__forceinline void operator |= (const GSVector8& v)
	{
		m = _mm256_or_ps(m, v);
	}

	__forceinline void operator ^= (const GSVector8& v)
	{
		m = _mm256_xor_ps(m, v);
	}

	__forceinline friend GSVector8 operator + (const GSVector8& v1, const GSVector8& v2)
	{
		return GSVector8(_mm256_add_ps(v1, v2));
	}

	__forceinline friend GSVector8 operator - (const GSVector8& v1, const GSVector8& v2)
	{
		return GSVector8(_mm256_sub_ps(v1, v2));
	}

	__forceinline friend GSVector8 operator * (const GSVector8& v1, const GSVector8& v2)
	{
		return GSVector8(_mm256_mul_ps(v1, v2));
	}

	__forceinline friend GSVector8 operator / (const GSVector8& v1, const GSVector8& v2)
	{
		return GSVector8(_mm256_div_ps(v1, v2));
	}

	__forceinline friend GSVector8 operator + (const GSVector8& v, float f)
	{
		return v + GSVector8(f);
	}

	__forceinline friend GSVector8 operator - (const GSVector8& v, float f)
	{
		return v - GSVector8(f);
	}

	__forceinline friend GSVector8 operator * (const GSVector8& v, float f)
	{
		return v * GSVector8(f);
	}

	__forceinline friend GSVector8 operator / (const GSVector8& v, float f)
	{
		return v / GSVector8(f);
	}

	__forceinline friend GSVector8 operator & (const GSVector8& v1, const GSVector8& v2)
	{
		return GSVector8(_mm256_and_ps(v1, v2));
	}

	__forceinline friend GSVector8 operator | (const GSVector8& v1, const GSVector8& v2)
	{
		return GSVector8(_mm256_or_ps(v1, v2));
	}

	__forceinline friend GSVector8 operator ^ (const GSVector8& v1, const GSVector8& v2)
	{
		return GSVector8(_mm256_xor_ps(v1, v2));
	}

	__forceinline friend GSVector8 operator == (const GSVector8& v1, const GSVector8& v2)
	{
		return GSVector8(_mm256_cmp_ps(v1, v2, _CMP_EQ_OQ));
	}

	__forceinline friend GSVector8 operator != (const GSVector8& v1, const GSVector8& v2)
	{
		return GSVector8(_mm256_cmp_ps(v1, v2, _CMP_NEQ_OQ));
	}

	__forceinline friend GSVector8 operator > (const GSVector8& v1, const GSVector8& v2)
	{
		return GSVector8(_mm256_cmp_ps(v1, v2, _CMP_GT_OQ));
	}

	__forceinline friend GSVector8 operator < (const GSVector8& v1, const GSVector8& v2)
	{
		return GSVector8(_mm256_cmp_ps(v1, v2, _CMP_LT_OQ));
	}

	__forceinline friend GSVector8 operator >= (const GSVector8& v1, const GSVector8& v2)
	{
		return GSVector8(_mm256_cmp_ps(v1, v2, _CMP_GE_OQ));
	}

	__forceinline friend GSVector8 operator <= (const GSVector8& v1, const GSVector8& v2)
	{
		return GSVector8(_mm256_cmp_ps(v1, v2, _CMP_LE_OQ));
	}

	// x = v[31:0] / v[159:128]
	// y = v[63:32] / v[191:160]
	// z = v[95:64] / v[223:192]
	// w = v[127:96] / v[255:224]


	#define VECTOR8_SHUFFLE_4(xs, xn, ys, yn, zs, zn, ws, wn) \
		__forceinline GSVector8 xs##ys##zs##ws() const {return GSVector8(_mm256_shuffle_ps(m, m, _MM_SHUFFLE(wn, zn, yn, xn)));} \
		__forceinline GSVector8 xs##ys##zs##ws(const GSVector8& v) const {return GSVector8(_mm256_shuffle_ps(m, v.m, _MM_SHUFFLE(wn, zn, yn, xn)));}

		// vs2012u3 cannot reuse the result of equivalent shuffles when it is done with _mm256_permute_ps (write v.xxxx() twice, and it will do it twice), but with _mm256_shuffle_ps it can.
		//__forceinline GSVector8 xs##ys##zs##ws() const {return GSVector8(_mm256_permute_ps(m, _MM_SHUFFLE(wn, zn, yn, xn)));}

	#define VECTOR8_SHUFFLE_3(xs, xn, ys, yn, zs, zn) \
		VECTOR8_SHUFFLE_4(xs, xn, ys, yn, zs, zn, x, 0) \
		VECTOR8_SHUFFLE_4(xs, xn, ys, yn, zs, zn, y, 1) \
		VECTOR8_SHUFFLE_4(xs, xn, ys, yn, zs, zn, z, 2) \
		VECTOR8_SHUFFLE_4(xs, xn, ys, yn, zs, zn, w, 3) \

	#define VECTOR8_SHUFFLE_2(xs, xn, ys, yn) \
		VECTOR8_SHUFFLE_3(xs, xn, ys, yn, x, 0) \
		VECTOR8_SHUFFLE_3(xs, xn, ys, yn, y, 1) \
		VECTOR8_SHUFFLE_3(xs, xn, ys, yn, z, 2) \
		VECTOR8_SHUFFLE_3(xs, xn, ys, yn, w, 3) \

	#define VECTOR8_SHUFFLE_1(xs, xn) \
		VECTOR8_SHUFFLE_2(xs, xn, x, 0) \
		VECTOR8_SHUFFLE_2(xs, xn, y, 1) \
		VECTOR8_SHUFFLE_2(xs, xn, z, 2) \
		VECTOR8_SHUFFLE_2(xs, xn, w, 3) \

	VECTOR8_SHUFFLE_1(x, 0)
	VECTOR8_SHUFFLE_1(y, 1)
	VECTOR8_SHUFFLE_1(z, 2)
	VECTOR8_SHUFFLE_1(w, 3)

	// a = v0[127:0]
	// b = v0[255:128]
	// c = v1[127:0]
	// d = v1[255:128]
	// _ = 0

	#define VECTOR8_PERMUTE128_2(as, an, bs, bn) \
		__forceinline GSVector8 as##bs() const {return GSVector8(_mm256_permute2f128_ps(m, m, an | (bn << 4)));} \
		__forceinline GSVector8 as##bs(const GSVector8& v) const {return GSVector8(_mm256_permute2f128_ps(m, v.m, an | (bn << 4)));} \

	#define VECTOR8_PERMUTE128_1(as, an) \
		VECTOR8_PERMUTE128_2(as, an, a, 0) \
		VECTOR8_PERMUTE128_2(as, an, b, 1) \
		VECTOR8_PERMUTE128_2(as, an, c, 2) \
		VECTOR8_PERMUTE128_2(as, an, d, 3) \
		VECTOR8_PERMUTE128_2(as, an, _, 8) \

	VECTOR8_PERMUTE128_1(a, 0)
	VECTOR8_PERMUTE128_1(b, 1)
	VECTOR8_PERMUTE128_1(c, 2)
	VECTOR8_PERMUTE128_1(d, 3)
	VECTOR8_PERMUTE128_1(_, 8)

	#if _M_SSE >= 0x501

	// a = v[63:0]
	// b = v[127:64]
	// c = v[191:128]
	// d = v[255:192]

	#define VECTOR8_PERMUTE64_4(as, an, bs, bn, cs, cn, ds, dn) \
		__forceinline GSVector8 as##bs##cs##ds() const {return GSVector8(_mm256_castpd_ps(_mm256_permute4x64_pd(_mm256_castps_pd(m), _MM_SHUFFLE(dn, cn, bn, an))));} \

	#define VECTOR8_PERMUTE64_3(as, an, bs, bn, cs, cn) \
		VECTOR8_PERMUTE64_4(as, an, bs, bn, cs, cn, a, 0) \
		VECTOR8_PERMUTE64_4(as, an, bs, bn, cs, cn, b, 1) \
		VECTOR8_PERMUTE64_4(as, an, bs, bn, cs, cn, c, 2) \
		VECTOR8_PERMUTE64_4(as, an, bs, bn, cs, cn, d, 3) \

	#define VECTOR8_PERMUTE64_2(as, an, bs, bn) \
		VECTOR8_PERMUTE64_3(as, an, bs, bn, a, 0) \
		VECTOR8_PERMUTE64_3(as, an, bs, bn, b, 1) \
		VECTOR8_PERMUTE64_3(as, an, bs, bn, c, 2) \
		VECTOR8_PERMUTE64_3(as, an, bs, bn, d, 3) \

	#define VECTOR8_PERMUTE64_1(as, an) \
		VECTOR8_PERMUTE64_2(as, an, a, 0) \
		VECTOR8_PERMUTE64_2(as, an, b, 1) \
		VECTOR8_PERMUTE64_2(as, an, c, 2) \
		VECTOR8_PERMUTE64_2(as, an, d, 3) \

	VECTOR8_PERMUTE64_1(a, 0)
	VECTOR8_PERMUTE64_1(b, 1)
	VECTOR8_PERMUTE64_1(c, 2)
	VECTOR8_PERMUTE64_1(d, 3)

	__forceinline GSVector8 permute32(const GSVector8i& mask) const
	{
		return GSVector8(_mm256_permutevar8x32_ps(m, mask));
	}

	__forceinline GSVector8 broadcast32() const
	{
		return GSVector8(_mm256_broadcastss_ps(_mm256_castps256_ps128(m)));
	}

	__forceinline static GSVector8 broadcast32(const GSVector4& v)
	{
		return GSVector8(_mm256_broadcastss_ps(v.m));
	}

	__forceinline static GSVector8 broadcast32(const void* f)
	{
		return GSVector8(_mm256_broadcastss_ps(_mm_load_ss((const float*)f)));
	}

	// TODO: v.(x0|y0|z0|w0|x1|y1|z1|w1) // broadcast element

	#endif
};

#endif

// conversion

__forceinline GSVector4i::GSVector4i(const GSVector4& v, bool truncate)
{
	m = truncate ? _mm_cvttps_epi32(v) : _mm_cvtps_epi32(v);
}

__forceinline GSVector4::GSVector4(const GSVector4i& v)
{
	m = _mm_cvtepi32_ps(v);
}

#if _M_SSE >= 0x501

__forceinline GSVector8i::GSVector8i(const GSVector8& v, bool truncate)
{
	m = truncate ? _mm256_cvttps_epi32(v) : _mm256_cvtps_epi32(v);
}

__forceinline GSVector8::GSVector8(const GSVector8i& v)
{
	m = _mm256_cvtepi32_ps(v);
}

#endif

// casting

__forceinline GSVector4i GSVector4i::cast(const GSVector4& v)
{
	return GSVector4i(_mm_castps_si128(v.m));
}

__forceinline GSVector4 GSVector4::cast(const GSVector4i& v)
{
	return GSVector4(_mm_castsi128_ps(v.m));
}

#if _M_SSE >= 0x500

__forceinline GSVector4i GSVector4i::cast(const GSVector8& v)
{
	return GSVector4i(_mm_castps_si128(_mm256_castps256_ps128(v)));
}

__forceinline GSVector4 GSVector4::cast(const GSVector8& v)
{
	return GSVector4(_mm256_castps256_ps128(v));
}

__forceinline GSVector8 GSVector8::cast(const GSVector4i& v)
{
	return GSVector8(_mm256_castps128_ps256(_mm_castsi128_ps(v.m)));
}

__forceinline GSVector8 GSVector8::cast(const GSVector4& v)
{
	return GSVector8(_mm256_castps128_ps256(v.m));
}

#endif

#if _M_SSE >= 0x501

__forceinline GSVector4i GSVector4i::cast(const GSVector8i& v)
{
	return GSVector4i(_mm256_castsi256_si128(v));
}

__forceinline GSVector4 GSVector4::cast(const GSVector8i& v)
{
	return GSVector4(_mm_castsi128_ps(_mm256_castsi256_si128(v)));
}

__forceinline GSVector8i GSVector8i::cast(const GSVector4i& v)
{
	return GSVector8i(_mm256_castsi128_si256(v.m));
}

__forceinline GSVector8i GSVector8i::cast(const GSVector4& v)
{
	return GSVector8i(_mm256_castsi128_si256(_mm_castps_si128(v.m)));
}

__forceinline GSVector8i GSVector8i::cast(const GSVector8& v)
{
	return GSVector8i(_mm256_castps_si256(v.m));
}

__forceinline GSVector8 GSVector8::cast(const GSVector8i& v)
{
	return GSVector8(_mm256_castsi256_ps(v.m));
}

#endif

#pragma pack(pop)
