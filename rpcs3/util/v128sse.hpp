#pragma once

#include "util/types.hpp"
#include "util/v128.hpp"
#include "util/sysinfo.hpp"

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include <immintrin.h>
#include <emmintrin.h>

#include <cmath>

inline bool v128_use_fma = utils::has_fma3();

inline v128 v128::fromV(const __m128i& value)
{
	v128 ret;
	ret.vi = value;
	return ret;
}

inline v128 v128::fromF(const __m128& value)
{
	v128 ret;
	ret.vf = value;
	return ret;
}

inline v128 v128::fromD(const __m128d& value)
{
	v128 ret;
	ret.vd = value;
	return ret;
}

inline v128 v128::add8(const v128& left, const v128& right)
{
	return fromV(_mm_add_epi8(left.vi, right.vi));
}

inline v128 v128::add16(const v128& left, const v128& right)
{
	return fromV(_mm_add_epi16(left.vi, right.vi));
}

inline v128 v128::add32(const v128& left, const v128& right)
{
	return fromV(_mm_add_epi32(left.vi, right.vi));
}

inline v128 v128::addfs(const v128& left, const v128& right)
{
	return fromF(_mm_add_ps(left.vf, right.vf));
}

inline v128 v128::addfd(const v128& left, const v128& right)
{
	return fromD(_mm_add_pd(left.vd, right.vd));
}

inline v128 v128::sub8(const v128& left, const v128& right)
{
	return fromV(_mm_sub_epi8(left.vi, right.vi));
}

inline v128 v128::sub16(const v128& left, const v128& right)
{
	return fromV(_mm_sub_epi16(left.vi, right.vi));
}

inline v128 v128::sub32(const v128& left, const v128& right)
{
	return fromV(_mm_sub_epi32(left.vi, right.vi));
}

inline v128 v128::subfs(const v128& left, const v128& right)
{
	return fromF(_mm_sub_ps(left.vf, right.vf));
}

inline v128 v128::subfd(const v128& left, const v128& right)
{
	return fromD(_mm_sub_pd(left.vd, right.vd));
}

inline v128 v128::maxu8(const v128& left, const v128& right)
{
	return fromV(_mm_max_epu8(left.vi, right.vi));
}

inline v128 v128::minu8(const v128& left, const v128& right)
{
	return fromV(_mm_min_epu8(left.vi, right.vi));
}

inline v128 v128::eq8(const v128& left, const v128& right)
{
	return fromV(_mm_cmpeq_epi8(left.vi, right.vi));
}

inline v128 v128::eq16(const v128& left, const v128& right)
{
	return fromV(_mm_cmpeq_epi16(left.vi, right.vi));
}

inline v128 v128::eq32(const v128& left, const v128& right)
{
	return fromV(_mm_cmpeq_epi32(left.vi, right.vi));
}

inline v128 v128::eq32f(const v128& left, const v128& right)
{
	return fromF(_mm_cmpeq_ps(left.vf, right.vf));
}

inline v128 v128::fma32f(v128 a, const v128& b, const v128& c)
{
#ifndef __FMA__
	if (v128_use_fma) [[likely]]
	{
#ifdef _MSC_VER
		a.vf = _mm_fmadd_ps(a.vf, b.vf, c.vf);
		return a;
#else
		__asm__("vfmadd213ps %[c], %[b], %[a]"
			: [a] "+x" (a.vf)
			: [b] "x" (b.vf)
			, [c] "x" (c.vf));
		return a;
#endif
	}

	for (int i = 0; i < 4; i++)
	{
		a._f[i] = std::fmaf(a._f[i], b._f[i], c._f[i]);
	}
	return a;
#else
	a.vf = _mm_fmadd_ps(a.vf, b.vf, c.vf);
	return a;
#endif
}

inline bool v128::operator==(const v128& right) const
{
	return _mm_movemask_epi8(v128::eq32(*this, right).vi) == 0xffff;
}

// result = (~left) & (right)
inline v128 v128::andnot(const v128& left, const v128& right)
{
	return fromV(_mm_andnot_si128(left.vi, right.vi));
}

inline v128 operator|(const v128& left, const v128& right)
{
	return v128::fromV(_mm_or_si128(left.vi, right.vi));
}

inline v128 operator&(const v128& left, const v128& right)
{
	return v128::fromV(_mm_and_si128(left.vi, right.vi));
}

inline v128 operator^(const v128& left, const v128& right)
{
	return v128::fromV(_mm_xor_si128(left.vi, right.vi));
}

inline v128 operator~(const v128& other)
{
	return other ^ v128::from32p(UINT32_MAX); // XOR with ones
}
