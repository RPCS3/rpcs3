#include "stdafx.h"
#include "PPUInterpreter.h"

#include "Emu/Memory/vm_reservation.h"
#include "Emu/System.h"
#include "PPUThread.h"
#include "Utilities/asm.h"
#include "Emu/Cell/Common.h"

#include <cmath>
#include <atomic>

#if defined(_MSC_VER)
#define SSSE3_FUNC
#else
#define SSSE3_FUNC __attribute__((__target__("ssse3")))
#endif

inline u64 dup32(u32 x) { return x | static_cast<u64>(x) << 32; }

// Write values to CR field
inline void ppu_cr_set(ppu_thread& ppu, u32 field, bool le, bool gt, bool eq, bool so)
{
	ppu.cr[field * 4 + 0] = le;
	ppu.cr[field * 4 + 1] = gt;
	ppu.cr[field * 4 + 2] = eq;
	ppu.cr[field * 4 + 3] = so;

	if (g_cfg.core.ppu_debug) [[unlikely]]
	{
		*reinterpret_cast<u32*>(vm::g_stat_addr + ppu.cia) |= *reinterpret_cast<u32*>(ppu.cr.bits + field * 4);
	}
}

// Write comparison results to CR field
template<typename T>
inline void ppu_cr_set(ppu_thread& ppu, u32 field, const T& a, const T& b)
{
	ppu_cr_set(ppu, field, a < b, a > b, a == b, ppu.xer.so);
}

// Set XER.OV bit (overflow)
inline void ppu_ov_set(ppu_thread& ppu, bool bit)
{
	ppu.xer.ov = bit;
	ppu.xer.so |= bit;
}

// Write comparison results to FPCC field with optional CR field update
template<typename T>
inline void ppu_fpcc_set(ppu_thread& ppu, const T& a, const T& b, const bool rc, const u64 cr_field = 1)
{
	// TODO: Do not hardcode to be endian dependant
	u32 fpcc = u32{a < b} << (8 * 0) | u32{a > b} << (8 * 1) | u32{a == b} << (8 * 2);

	// Test FU
	if (fpcc == 0) [[unlikely]] fpcc = 1 << (8 * 3);

	// Write FPCC
	ppu.fpscr.fields[4] = fpcc;

	if (rc) [[unlikely]]
	{
		ppu.cr.fields[cr_field] = fpcc;

		if (g_cfg.core.ppu_debug) [[unlikely]]
		{
			*reinterpret_cast<u32*>(vm::g_stat_addr + ppu.cia) |= ppu.cr.fields[cr_field];
		}
	}
}

// Compare 16 packed unsigned bytes (greater than)
inline __m128i sse_cmpgt_epu8(__m128i A, __m128i B)
{
	// (A xor 0x80) > (B xor 0x80)
	const auto sign = _mm_set1_epi32(0x80808080);
	return _mm_cmpgt_epi8(_mm_xor_si128(A, sign), _mm_xor_si128(B, sign));
}

inline __m128i sse_cmpgt_epu16(__m128i A, __m128i B)
{
	const auto sign = _mm_set1_epi32(0x80008000);
	return _mm_cmpgt_epi16(_mm_xor_si128(A, sign), _mm_xor_si128(B, sign));
}

inline __m128i sse_cmpgt_epu32(__m128i A, __m128i B)
{
	const auto sign = _mm_set1_epi32(0x80000000);
	return _mm_cmpgt_epi32(_mm_xor_si128(A, sign), _mm_xor_si128(B, sign));
}

extern __m128 sse_exp2_ps(__m128 A)
{
	const auto x0 = _mm_max_ps(_mm_min_ps(A, _mm_set1_ps(127.4999961f)), _mm_set1_ps(-127.4999961f));
	const auto x1 = _mm_add_ps(x0, _mm_set1_ps(0.5f));
	const auto x2 = _mm_sub_epi32(_mm_cvtps_epi32(x1), _mm_and_si128(_mm_castps_si128(_mm_cmpnlt_ps(_mm_setzero_ps(), x1)), _mm_set1_epi32(1)));
	const auto x3 = _mm_sub_ps(x0, _mm_cvtepi32_ps(x2));
	const auto x4 = _mm_mul_ps(x3, x3);
	const auto x5 = _mm_mul_ps(x3, _mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(x4, _mm_set1_ps(0.023093347705f)), _mm_set1_ps(20.20206567f)), x4), _mm_set1_ps(1513.906801f)));
	const auto x6 = _mm_mul_ps(x5, _mm_rcp_ps(_mm_sub_ps(_mm_add_ps(_mm_mul_ps(_mm_set1_ps(233.1842117f), x4), _mm_set1_ps(4368.211667f)), x5)));
	return _mm_mul_ps(_mm_add_ps(_mm_add_ps(x6, x6), _mm_set1_ps(1.0f)), _mm_castsi128_ps(_mm_slli_epi32(_mm_add_epi32(x2, _mm_set1_epi32(127)), 23)));
}

extern __m128 sse_log2_ps(__m128 A)
{
	const auto _1 = _mm_set1_ps(1.0f);
	const auto _c = _mm_set1_ps(1.442695040f);
	const auto x0 = _mm_max_ps(A, _mm_castsi128_ps(_mm_set1_epi32(0x00800000)));
	const auto x1 = _mm_or_ps(_mm_and_ps(x0, _mm_castsi128_ps(_mm_set1_epi32(0x807fffff))), _1);
	const auto x2 = _mm_rcp_ps(_mm_add_ps(x1, _1));
	const auto x3 = _mm_mul_ps(_mm_sub_ps(x1, _1), x2);
	const auto x4 = _mm_add_ps(x3, x3);
	const auto x5 = _mm_mul_ps(x4, x4);
	const auto x6 = _mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_set1_ps(-0.7895802789f), x5), _mm_set1_ps(16.38666457f)), x5), _mm_set1_ps(-64.1409953f));
	const auto x7 = _mm_rcp_ps(_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_set1_ps(-35.67227983f), x5), _mm_set1_ps(312.0937664f)), x5), _mm_set1_ps(-769.6919436f)));
	const auto x8 = _mm_cvtepi32_ps(_mm_sub_epi32(_mm_srli_epi32(_mm_castps_si128(x0), 23), _mm_set1_epi32(127)));
	return _mm_add_ps(_mm_mul_ps(_mm_mul_ps(_mm_mul_ps(_mm_mul_ps(x5, x6), x7), x4), _c), _mm_add_ps(_mm_mul_ps(x4, _c), x8));
}

extern __m128i sse_pshufb(__m128i data, __m128i index)
{
	v128 m = v128::fromV(_mm_and_si128(index, _mm_set1_epi8(0xf)));
	v128 a = v128::fromV(data);
	v128 r;

	for (int i = 0; i < 16; i++)
	{
		r._u8[i] = a._u8[m._u8[i]];
	}

	return _mm_and_si128(r.vi, _mm_cmpgt_epi8(index, _mm_set1_epi8(-1)));
}

extern SSSE3_FUNC __m128i sse_altivec_vperm(__m128i A, __m128i B, __m128i C)
{
	const auto index = _mm_andnot_si128(C, _mm_set1_epi8(0x1f));
	const auto mask = _mm_cmpgt_epi8(index, _mm_set1_epi8(0xf));
	const auto sa = _mm_shuffle_epi8(A, index);
	const auto sb = _mm_shuffle_epi8(B, index);
	return _mm_or_si128(_mm_and_si128(mask, sa), _mm_andnot_si128(mask, sb));
}

extern __m128i sse_altivec_vperm_v0(__m128i A, __m128i B, __m128i C)
{
	__m128i ab[2]{B, A};
	v128 index = v128::fromV(_mm_andnot_si128(C, _mm_set1_epi8(0x1f)));
	v128 res;

	for (int i = 0; i < 16; i++)
	{
		res._u8[i] = reinterpret_cast<u8*>(+ab)[index._u8[i]];
	}

	return res.vi;
}

extern __m128i sse_altivec_lvsl(u64 addr)
{
	alignas(16) static const u8 lvsl_values[0x10][0x10] =
	{
		{ 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 },
		{ 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 },
		{ 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02 },
		{ 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03 },
		{ 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04 },
		{ 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05 },
		{ 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06 },
		{ 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07 },
		{ 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08 },
		{ 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09 },
		{ 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a },
		{ 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b },
		{ 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c },
		{ 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d },
		{ 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e },
		{ 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f },
	};

	return _mm_load_si128(reinterpret_cast<const __m128i*>(+lvsl_values[addr & 0xf]));
}

extern __m128i sse_altivec_lvsr(u64 addr)
{
	alignas(16) static const u8 lvsr_values[0x10][0x10] =
	{
		{ 0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10 },
		{ 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f },
		{ 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e },
		{ 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d },
		{ 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c },
		{ 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b },
		{ 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a },
		{ 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09 },
		{ 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08 },
		{ 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07 },
		{ 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06 },
		{ 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05 },
		{ 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04 },
		{ 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03 },
		{ 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02 },
		{ 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 },
	};

	return _mm_load_si128(reinterpret_cast<const __m128i*>(+lvsr_values[addr & 0xf]));
}

static const __m128i lvlx_masks[0x10] =
{
	_mm_set_epi8(0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf),
	_mm_set_epi8(0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, -1),
	_mm_set_epi8(0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, -1, -1),
	_mm_set_epi8(0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, -1, -1, -1),
	_mm_set_epi8(0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, -1, -1, -1, -1),
	_mm_set_epi8(0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, -1, -1, -1, -1, -1),
	_mm_set_epi8(0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, -1, -1, -1, -1, -1, -1),
	_mm_set_epi8(0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, -1, -1, -1, -1, -1, -1, -1),
	_mm_set_epi8(0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, -1, -1, -1, -1, -1, -1, -1, -1),
	_mm_set_epi8(0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, -1, -1, -1, -1, -1, -1, -1, -1, -1),
	_mm_set_epi8(0xa, 0xb, 0xc, 0xd, 0xe, 0xf, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
	_mm_set_epi8(0xb, 0xc, 0xd, 0xe, 0xf, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
	_mm_set_epi8(0xc, 0xd, 0xe, 0xf, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
	_mm_set_epi8(0xd, 0xe, 0xf, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
	_mm_set_epi8(0xe, 0xf, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
	_mm_set_epi8(0xf, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
};

static const __m128i lvrx_masks[0x10] =
{
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1),
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x0),
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x0, 0x1),
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x0, 0x1, 0x2),
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x0, 0x1, 0x2, 0x3),
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x0, 0x1, 0x2, 0x3, 0x4),
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5),
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6),
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7),
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8),
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9),
	_mm_set_epi8(-1, -1, -1, -1, -1, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa),
	_mm_set_epi8(-1, -1, -1, -1, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb),
	_mm_set_epi8(-1, -1, -1, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc),
	_mm_set_epi8(-1, -1, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd),
	_mm_set_epi8(-1, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe),
};

extern SSSE3_FUNC __m128i sse_cellbe_lvlx(u64 addr)
{
	return _mm_shuffle_epi8(_mm_load_si128(vm::_ptr<const __m128i>(addr & ~0xf)), lvlx_masks[addr & 0xf]);
}

extern SSSE3_FUNC void sse_cellbe_stvlx(u64 addr, __m128i a)
{
	_mm_maskmoveu_si128(_mm_shuffle_epi8(a, lvlx_masks[addr & 0xf]), lvrx_masks[addr & 0xf], vm::_ptr<char>(addr & ~0xf));
}

extern SSSE3_FUNC __m128i sse_cellbe_lvrx(u64 addr)
{
	return _mm_shuffle_epi8(_mm_load_si128(vm::_ptr<const __m128i>(addr & ~0xf)), lvrx_masks[addr & 0xf]);
}

extern SSSE3_FUNC void sse_cellbe_stvrx(u64 addr, __m128i a)
{
	_mm_maskmoveu_si128(_mm_shuffle_epi8(a, lvrx_masks[addr & 0xf]), lvlx_masks[addr & 0xf], vm::_ptr<char>(addr & ~0xf));
}

extern __m128i sse_cellbe_lvlx_v0(u64 addr)
{
	return sse_pshufb(_mm_load_si128(vm::_ptr<const __m128i>(addr & ~0xf)), lvlx_masks[addr & 0xf]);
}

extern void sse_cellbe_stvlx_v0(u64 addr, __m128i a)
{
	_mm_maskmoveu_si128(sse_pshufb(a, lvlx_masks[addr & 0xf]), lvrx_masks[addr & 0xf], vm::_ptr<char>(addr & ~0xf));
}

extern __m128i sse_cellbe_lvrx_v0(u64 addr)
{
	return sse_pshufb(_mm_load_si128(vm::_ptr<const __m128i>(addr & ~0xf)), lvrx_masks[addr & 0xf]);
}

extern void sse_cellbe_stvrx_v0(u64 addr, __m128i a)
{
	_mm_maskmoveu_si128(sse_pshufb(a, lvrx_masks[addr & 0xf]), lvlx_masks[addr & 0xf], vm::_ptr<char>(addr & ~0xf));
}

template<typename T>
struct add_flags_result_t
{
	T result;
	bool carry;

	add_flags_result_t() = default;

	// Straighforward ADD with flags
	add_flags_result_t(T a, T b)
		: result(a + b)
		, carry(result < a)
	{
	}

	// Straighforward ADC with flags
	add_flags_result_t(T a, T b, bool c)
		: add_flags_result_t(a, b)
	{
		add_flags_result_t r(result, c);
		result = r.result;
		carry |= r.carry;
	}
};

static add_flags_result_t<u64> add64_flags(u64 a, u64 b)
{
	return{ a, b };
}

static add_flags_result_t<u64> add64_flags(u64 a, u64 b, bool c)
{
	return{ a, b, c };
}

extern u64 get_timebased_time();
extern void ppu_execute_syscall(ppu_thread& ppu, u64 code);

extern u32 ppu_lwarx(ppu_thread& ppu, u32 addr);
extern u64 ppu_ldarx(ppu_thread& ppu, u32 addr);
extern bool ppu_stwcx(ppu_thread& ppu, u32 addr, u32 reg_value);
extern bool ppu_stdcx(ppu_thread& ppu, u32 addr, u64 reg_value);



class ppu_scale_table_t
{
	std::array<v128, 32 + 31> m_data;

public:
	ppu_scale_table_t()
	{
		for (s32 i = -31; i < 32; i++)
		{
			m_data[i + 31].vf = _mm_set1_ps(static_cast<float>(std::exp2(i)));
		}
	}

	FORCE_INLINE __m128 operator [] (s32 scale) const
	{
		return m_data[scale + 31].vf;
	}
}
const g_ppu_scale_table;

bool ppu_interpreter::MFVSCR(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd] = v128::from32(0, 0, 0, u32{ppu.sat} | (u32{ppu.nj} << 16));
	return true;
}

bool ppu_interpreter::MTVSCR(ppu_thread& ppu, ppu_opcode_t op)
{
	const u32 vscr = ppu.vr[op.vb]._u32[3];
	ppu.sat = (vscr & 1) != 0;
	ppu.nj  = (vscr & 0x10000) != 0;
	return true;
}

bool ppu_interpreter::VADDCUW(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va].vi;
	const auto b = ppu.vr[op.vb].vi;
	ppu.vr[op.vd].vi = _mm_srli_epi32(_mm_cmpgt_epi32(_mm_xor_si128(b, _mm_set1_epi32(0x80000000)), _mm_xor_si128(a, _mm_set1_epi32(0x7fffffff))), 31);
	return true;
}

bool ppu_interpreter::VADDFP(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd] = v128::addfs(ppu.vr[op.va], ppu.vr[op.vb]);
	return true;
}

bool ppu_interpreter_fast::VADDSBS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_adds_epi8(ppu.vr[op.va].vi, ppu.vr[op.vb].vi);
	return true;
}

bool ppu_interpreter_precise::VADDSBS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	auto& d = ppu.vr[op.vd];

	for (u8 i = 0; i < 16; i++)
	{
		const s16 sum = a._s8[i] + b._s8[i];

		if (sum < INT8_MIN)
		{
			d._s8[i] = INT8_MIN;
			ppu.sat = true;
		}
		else if (sum > INT8_MAX)
		{
			d._s8[i] = INT8_MAX;
			ppu.sat = true;
		}
		else
		{
			d._s8[i] = static_cast<s8>(sum);
		}
	}

	return true;
}

bool ppu_interpreter_fast::VADDSHS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_adds_epi16(ppu.vr[op.va].vi, ppu.vr[op.vb].vi);
	return true;
}

bool ppu_interpreter_precise::VADDSHS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	auto& d = ppu.vr[op.vd];

	for (u8 i = 0; i < 8; i++)
	{
		const s32 sum = a._s16[i] + b._s16[i];

		if (sum < INT16_MIN)
		{
			d._s16[i] = INT16_MIN;
			ppu.sat = true;
		}
		else if (sum > INT16_MAX)
		{
			d._s16[i] = INT16_MAX;
			ppu.sat = true;
		}
		else
		{
			d._s16[i] = static_cast<s16>(sum);
		}
	}

	return true;
}

// TODO: fix
bool ppu_interpreter_fast::VADDSWS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va];
	const auto b = ppu.vr[op.vb];
	const auto s = v128::add32(a, b); // a + b
	const auto m = (a ^ s) & (b ^ s); // overflow bit
	const auto x = _mm_srai_epi32(m.vi, 31); // saturation mask
	const auto y = _mm_srai_epi32(_mm_and_si128(s.vi, m.vi), 31); // positive saturation mask
	ppu.vr[op.vd].vi = _mm_xor_si128(_mm_xor_si128(_mm_srli_epi32(x, 1), y), _mm_or_si128(s.vi, x));
	return true;
}

bool ppu_interpreter_precise::VADDSWS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	auto& d = ppu.vr[op.vd];

	for (u8 i = 0; i < 4; i++)
	{
		const s64 sum = s64{a._s32[i]} + b._s32[i];

		if (sum < INT32_MIN)
		{
			d._s32[i] = INT32_MIN;
			ppu.sat = true;
		}
		else if (sum > INT32_MAX)
		{
			d._s32[i] = INT32_MAX;
			ppu.sat = true;
		}
		else
		{
			d._s32[i] = static_cast<s32>(sum);
		}
	}

	return true;
}

bool ppu_interpreter::VADDUBM(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd] = v128::add8(ppu.vr[op.va], ppu.vr[op.vb]);
	return true;
}

bool ppu_interpreter_fast::VADDUBS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_adds_epu8(ppu.vr[op.va].vi, ppu.vr[op.vb].vi);
	return true;
}

bool ppu_interpreter_precise::VADDUBS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	auto& d = ppu.vr[op.vd];

	for (u8 i = 0; i < 16; i++)
	{
		const u16 sum = a._u8[i] + b._u8[i];

		if (sum > UINT8_MAX)
		{
			d._u8[i] = UINT8_MAX;
			ppu.sat = true;
		}
		else
		{
			d._u8[i] = static_cast<u8>(sum);
		}
	}

	return true;
}

bool ppu_interpreter::VADDUHM(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd] = v128::add16(ppu.vr[op.va], ppu.vr[op.vb]);
	return true;
}

bool ppu_interpreter_fast::VADDUHS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_adds_epu16(ppu.vr[op.va].vi, ppu.vr[op.vb].vi);
	return true;
}

bool ppu_interpreter_precise::VADDUHS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	auto& d = ppu.vr[op.vd];

	for (u8 i = 0; i < 8; i++)
	{
		const u32 sum = a._u16[i] + b._u16[i];

		if (sum > UINT16_MAX)
		{
			d._u16[i] = UINT16_MAX;
			ppu.sat = true;
		}
		else
		{
			d._u16[i] = static_cast<u16>(sum);
		}
	}

	return true;
}

bool ppu_interpreter::VADDUWM(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd] = v128::add32(ppu.vr[op.va], ppu.vr[op.vb]);
	return true;
}

// TODO: fix
bool ppu_interpreter_fast::VADDUWS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va].vi;
	const auto b = ppu.vr[op.vb].vi;
	ppu.vr[op.vd].vi = _mm_or_si128(_mm_add_epi32(a, b), _mm_cmpgt_epi32(_mm_xor_si128(b, _mm_set1_epi32(0x80000000)), _mm_xor_si128(a, _mm_set1_epi32(0x7fffffff))));
	return true;
}

bool ppu_interpreter_precise::VADDUWS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	auto& d = ppu.vr[op.vd];

	for (u8 i = 0; i < 4; i++)
	{
		const u64 sum = u64{a._u32[i]} + b._u32[i];

		if (sum > UINT32_MAX)
		{
			d._u32[i] = UINT32_MAX;
			ppu.sat = true;
		}
		else
		{
			d._u32[i] = static_cast<u32>(sum);
		}
	}

	return true;
}

bool ppu_interpreter::VAND(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd] = ppu.vr[op.va] & ppu.vr[op.vb];
	return true;
}

bool ppu_interpreter::VANDC(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd] = v128::andnot(ppu.vr[op.vb], ppu.vr[op.va]);
	return true;
}

bool ppu_interpreter::VAVGSB(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va];
	const auto b = v128::add8(ppu.vr[op.vb], v128::from8p(1)); // add 1
	const auto summ = v128::add8(a, b) & v128::from8p(0xfe);
	const auto sign = v128::from8p(0x80);
	const auto overflow = (((a ^ summ) & (b ^ summ)) ^ summ ^ v128::eq8(b, sign)) & sign; // calculate msb
	ppu.vr[op.vd].vi = _mm_or_si128(overflow.vi, _mm_srli_epi64(summ.vi, 1));
	return true;
}

bool ppu_interpreter::VAVGSH(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va];
	const auto b = v128::add16(ppu.vr[op.vb], v128::from16p(1)); // add 1
	const auto summ = v128::add16(a, b);
	const auto sign = v128::from16p(0x8000);
	const auto overflow = (((a ^ summ) & (b ^ summ)) ^ summ ^ v128::eq16(b, sign)) & sign; // calculate msb
	ppu.vr[op.vd].vi = _mm_or_si128(overflow.vi, _mm_srli_epi16(summ.vi, 1));
	return true;
}

bool ppu_interpreter::VAVGSW(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va];
	const auto b = v128::add32(ppu.vr[op.vb], v128::from32p(1)); // add 1
	const auto summ = v128::add32(a, b);
	const auto sign = v128::from32p(0x80000000);
	const auto overflow = (((a ^ summ) & (b ^ summ)) ^ summ ^ v128::eq32(b, sign)) & sign; // calculate msb
	ppu.vr[op.vd].vi = _mm_or_si128(overflow.vi, _mm_srli_epi32(summ.vi, 1));
	return true;
}

bool ppu_interpreter::VAVGUB(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_avg_epu8(ppu.vr[op.va].vi, ppu.vr[op.vb].vi);
	return true;
}

bool ppu_interpreter::VAVGUH(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_avg_epu16(ppu.vr[op.va].vi, ppu.vr[op.vb].vi);
	return true;
}

bool ppu_interpreter::VAVGUW(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va];
	const auto b = ppu.vr[op.vb];
	const auto summ = v128::add32(v128::add32(a, b), v128::from32p(1));
	const auto carry = _mm_xor_si128(_mm_slli_epi32(sse_cmpgt_epu32(summ.vi, a.vi), 31), _mm_set1_epi32(0x80000000));
	ppu.vr[op.vd].vi = _mm_or_si128(carry, _mm_srli_epi32(summ.vi, 1));
	return true;
}

bool ppu_interpreter::VCFSX(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vf = _mm_mul_ps(_mm_cvtepi32_ps(ppu.vr[op.vb].vi), g_ppu_scale_table[0 - op.vuimm]);
	return true;
}

bool ppu_interpreter::VCFUX(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto b = ppu.vr[op.vb].vi;
	const auto fix = _mm_and_ps(_mm_castsi128_ps(_mm_srai_epi32(b, 31)), _mm_set1_ps(0x80000000));
	ppu.vr[op.vd].vf = _mm_mul_ps(_mm_add_ps(_mm_cvtepi32_ps(_mm_and_si128(b, _mm_set1_epi32(0x7fffffff))), fix), g_ppu_scale_table[0 - op.vuimm]);
	return true;
}

bool ppu_interpreter::VCMPBFP(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va].vf;
	const auto b = ppu.vr[op.vb].vf;
	const auto sign = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
	const auto cmp1 = _mm_cmpnle_ps(a, b);
	const auto cmp2 = _mm_cmpnge_ps(a, _mm_xor_ps(b, sign));
	ppu.vr[op.vd].vf = _mm_or_ps(_mm_and_ps(cmp1, sign), _mm_and_ps(cmp2, _mm_castsi128_ps(_mm_set1_epi32(0x40000000))));
	if (op.oe) [[unlikely]] ppu_cr_set(ppu, 6, false, false, _mm_movemask_ps(_mm_or_ps(cmp1, cmp2)) == 0, false);
	return true;
}

bool ppu_interpreter::VCMPEQFP(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_ps(ppu.vr[op.vd].vf = _mm_cmpeq_ps(ppu.vr[op.va].vf, ppu.vr[op.vb].vf));
	if (op.oe) [[unlikely]] ppu_cr_set(ppu, 6, rmask == 0xf, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPEQUB(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8((ppu.vr[op.vd] = v128::eq8(ppu.vr[op.va], ppu.vr[op.vb])).vi);
	if (op.oe) [[unlikely]] ppu_cr_set(ppu, 6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPEQUH(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8((ppu.vr[op.vd] = v128::eq16(ppu.vr[op.va], ppu.vr[op.vb])).vi);
	if (op.oe) [[unlikely]] ppu_cr_set(ppu, 6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPEQUW(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8((ppu.vr[op.vd] = v128::eq32(ppu.vr[op.va], ppu.vr[op.vb])).vi);
	if (op.oe) [[unlikely]] ppu_cr_set(ppu, 6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPGEFP(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_ps(ppu.vr[op.vd].vf = _mm_cmpge_ps(ppu.vr[op.va].vf, ppu.vr[op.vb].vf));
	if (op.oe) [[unlikely]] ppu_cr_set(ppu, 6, rmask == 0xf, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPGTFP(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_ps(ppu.vr[op.vd].vf = _mm_cmpgt_ps(ppu.vr[op.va].vf, ppu.vr[op.vb].vf));
	if (op.oe) [[unlikely]] ppu_cr_set(ppu, 6, rmask == 0xf, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPGTSB(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8(ppu.vr[op.vd].vi = _mm_cmpgt_epi8(ppu.vr[op.va].vi, ppu.vr[op.vb].vi));
	if (op.oe) [[unlikely]] ppu_cr_set(ppu, 6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPGTSH(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8(ppu.vr[op.vd].vi = _mm_cmpgt_epi16(ppu.vr[op.va].vi, ppu.vr[op.vb].vi));
	if (op.oe) [[unlikely]] ppu_cr_set(ppu, 6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPGTSW(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8(ppu.vr[op.vd].vi = _mm_cmpgt_epi32(ppu.vr[op.va].vi, ppu.vr[op.vb].vi));
	if (op.oe) [[unlikely]] ppu_cr_set(ppu, 6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPGTUB(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8(ppu.vr[op.vd].vi = sse_cmpgt_epu8(ppu.vr[op.va].vi, ppu.vr[op.vb].vi));
	if (op.oe) [[unlikely]] ppu_cr_set(ppu, 6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPGTUH(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8(ppu.vr[op.vd].vi = sse_cmpgt_epu16(ppu.vr[op.va].vi, ppu.vr[op.vb].vi));
	if (op.oe) [[unlikely]] ppu_cr_set(ppu, 6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPGTUW(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8(ppu.vr[op.vd].vi = sse_cmpgt_epu32(ppu.vr[op.va].vi, ppu.vr[op.vb].vi));
	if (op.oe) [[unlikely]] ppu_cr_set(ppu, 6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

// TODO: fix
bool ppu_interpreter_fast::VCTSXS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto scaled = _mm_mul_ps(ppu.vr[op.vb].vf, g_ppu_scale_table[op.vuimm]);
	ppu.vr[op.vd].vi = _mm_xor_si128(_mm_cvttps_epi32(scaled), _mm_castps_si128(_mm_cmpge_ps(scaled, _mm_set1_ps(0x80000000))));
	return true;
}

bool ppu_interpreter_precise::VCTSXS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto uim = op.vuimm;
	const auto& b = ppu.vr[op.vb];
	auto& d = ppu.vr[op.vd];

	for (u8 i = 0; i < 4; i++)
	{
		const f32 X = b._f[i];
		const bool sign = std::signbit(X);
		const s32 exp = fexpf(X);
		const u32 frac = std::bit_cast<u32>(X) << 9;
		const s32 exp2 = exp + uim - 127;

		if (exp == 255)
		{
			if (frac != 0)
			{
				d._s32[i] = 0;
			}
			else
			{
				ppu.sat = true;
				d._s32[i] = sign ? 0x80000000 : 0x7FFFFFFF;
			}
		}
		else if (exp2 > 30)
		{
			ppu.sat = true;
			d._s32[i] = sign ? 0x80000000 : 0x7FFFFFFF;
		}
		else if (exp2 < 0)
		{
			d._s32[i] = 0;
		}
		else
		{
			s32 significand = (0x80000000 | (frac >> 1)) >> (31 - exp2);
			d._s32[i] = sign ? -significand : significand;
		}
	}

	return true;
}

bool ppu_interpreter_fast::VCTUXS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto scaled1 = _mm_max_ps(_mm_mul_ps(ppu.vr[op.vb].vf, g_ppu_scale_table[op.vuimm]), _mm_set1_ps(0.0f));
	const auto scaled2 = _mm_and_ps(_mm_sub_ps(scaled1, _mm_set1_ps(0x80000000)), _mm_cmpge_ps(scaled1, _mm_set1_ps(0x80000000)));
	ppu.vr[op.vd].vi = _mm_or_si128(_mm_or_si128(_mm_cvttps_epi32(scaled1), _mm_cvttps_epi32(scaled2)), _mm_castps_si128(_mm_cmpge_ps(scaled1, _mm_set1_ps(0x100000000))));
	return true;
}

bool ppu_interpreter_precise::VCTUXS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto uim = op.vuimm;
	const auto& b = ppu.vr[op.vb];
	auto& d = ppu.vr[op.vd];

	for (u8 i = 0; i < 4; i++)
	{
		const f32 X = b._f[i];
		const bool sign = std::signbit(X);
		const s32 exp = fexpf(X);
		const u32 frac = std::bit_cast<u32>(X) << 9;
		const s32 exp2 = exp + uim - 127;

		if (exp == 255)
		{
			if (frac != 0)
			{
				d._u32[i] = 0;
			}
			else
			{
				ppu.sat = true;
				d._u32[i] = sign ? 0 : 0xFFFFFFFF;
			}
		}
		else if (exp2 > 31)
		{
			ppu.sat = true;
			d._u32[i] = sign ? 0 : 0xFFFFFFFF;
		}
		else if (exp2 < 0)
		{
			d._u32[i] = 0;
		}
		else if (sign)
		{
			ppu.sat = true;
			d._u32[i] = 0;
		}
		else
		{
			d._u32[i] = (0x80000000 | (frac >> 1)) >> (31 - exp2);
		}
	}

	return true;
}

bool ppu_interpreter::VEXPTEFP(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vf = sse_exp2_ps(ppu.vr[op.vb].vf);
	return true;
}

bool ppu_interpreter::VLOGEFP(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vf = sse_log2_ps(ppu.vr[op.vb].vf);
	return true;
}

bool ppu_interpreter_fast::VMADDFP(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vf = _mm_add_ps(_mm_mul_ps(ppu.vr[op.va].vf, ppu.vr[op.vc].vf), ppu.vr[op.vb].vf);
	return true;
}

bool ppu_interpreter_precise::VMADDFP(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto& a = ppu.vr[op.va]._f;
	const auto& b = ppu.vr[op.vb]._f;
	const auto& c = ppu.vr[op.vc]._f;
	auto& d = ppu.vr[op.rd]._f;

	// TODO: Optimize
	for (u32 i = 0; i < 4; i++)
	{
		d[i] = f32(f64{a[i]} * f64{c[i]} + f64{b[i]});
	}

	return true;
}

bool ppu_interpreter::VMAXFP(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vf = _mm_max_ps(ppu.vr[op.va].vf, ppu.vr[op.vb].vf);
	return true;
}

bool ppu_interpreter::VMAXSB(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va].vi;
	const auto b = ppu.vr[op.vb].vi;
	const auto m = _mm_cmpgt_epi8(a, b);
	ppu.vr[op.vd].vi = _mm_or_si128(_mm_and_si128(m, a), _mm_andnot_si128(m, b));
	return true;
}

bool ppu_interpreter::VMAXSH(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_max_epi16(ppu.vr[op.va].vi, ppu.vr[op.vb].vi);
	return true;
}

bool ppu_interpreter::VMAXSW(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va].vi;
	const auto b = ppu.vr[op.vb].vi;
	const auto m = _mm_cmpgt_epi32(a, b);
	ppu.vr[op.vd].vi = _mm_or_si128(_mm_and_si128(m, a), _mm_andnot_si128(m, b));
	return true;
}

bool ppu_interpreter::VMAXUB(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_max_epu8(ppu.vr[op.va].vi, ppu.vr[op.vb].vi);
	return true;
}

bool ppu_interpreter::VMAXUH(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto mask = _mm_set1_epi32(0x80008000);
	ppu.vr[op.vd].vi = _mm_xor_si128(_mm_max_epi16(_mm_xor_si128(ppu.vr[op.va].vi, mask), _mm_xor_si128(ppu.vr[op.vb].vi, mask)), mask);
	return true;
}

bool ppu_interpreter::VMAXUW(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va].vi;
	const auto b = ppu.vr[op.vb].vi;
	const auto m = sse_cmpgt_epu32(a, b);
	ppu.vr[op.vd].vi = _mm_or_si128(_mm_and_si128(m, a), _mm_andnot_si128(m, b));
	return true;
}

bool ppu_interpreter_fast::VMHADDSHS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va].vi;
	const auto b = ppu.vr[op.vb].vi;
	const auto c = ppu.vr[op.vc].vi;
	const auto m = _mm_or_si128(_mm_srli_epi16(_mm_mullo_epi16(a, b), 15), _mm_slli_epi16(_mm_mulhi_epi16(a, b), 1));
	const auto s = _mm_cmpeq_epi16(m, _mm_set1_epi16(-0x8000)); // detect special case (positive 0x8000)
	ppu.vr[op.vd].vi = _mm_adds_epi16(_mm_adds_epi16(_mm_xor_si128(m, s), c), _mm_srli_epi16(s, 15));
	return true;
}

bool ppu_interpreter_precise::VMHADDSHS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	const auto& c = ppu.vr[op.vc];
	auto& d = ppu.vr[op.vd];

	for (u8 i = 0; i < 8; i++)
	{
		const s32 prod = a._s16[i] * b._s16[i];
		const s32 sum = (prod >> 15) + c._s16[i];

		if (sum < INT16_MIN)
		{
			d._s16[i] = INT16_MIN;
			ppu.sat = true;
		}
		else if (sum > INT16_MAX)
		{
			d._s16[i] = INT16_MAX;
			ppu.sat = true;
		}
		else
		{
			d._s16[i] = static_cast<s16>(sum);
		}
	}

	return true;
}

bool ppu_interpreter_fast::VMHRADDSHS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va].vi;
	const auto b = ppu.vr[op.vb].vi;
	const auto c = ppu.vr[op.vc].vi;
	const auto x80 = _mm_set1_epi16(0x80); // 0x80 * 0x80 = 0x4000, add this to the product
	const auto al = _mm_unpacklo_epi16(a, x80);
	const auto ah = _mm_unpackhi_epi16(a, x80);
	const auto bl = _mm_unpacklo_epi16(b, x80);
	const auto bh = _mm_unpackhi_epi16(b, x80);
	const auto ml = _mm_srai_epi32(_mm_madd_epi16(al, bl), 15);
	const auto mh = _mm_srai_epi32(_mm_madd_epi16(ah, bh), 15);
	const auto cl = _mm_srai_epi32(_mm_unpacklo_epi16(_mm_setzero_si128(), c), 16);
	const auto ch = _mm_srai_epi32(_mm_unpackhi_epi16(_mm_setzero_si128(), c), 16);
	ppu.vr[op.vd].vi = _mm_packs_epi32(_mm_add_epi32(ml, cl), _mm_add_epi32(mh, ch));
	return true;
}

bool ppu_interpreter_precise::VMHRADDSHS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	const auto& c = ppu.vr[op.vc];
	auto& d = ppu.vr[op.vd];

	for (u8 i = 0; i < 8; i++)
	{
		const s32 prod = a._s16[i] * b._s16[i];
		const s32 sum = ((prod + 0x00004000) >> 15) + c._s16[i];

		if (sum < INT16_MIN)
		{
			d._s16[i] = INT16_MIN;
			ppu.sat = true;
		}
		else if (sum > INT16_MAX)
		{
			d._s16[i] = INT16_MAX;
			ppu.sat = true;
		}
		else
		{
			d._s16[i] = static_cast<s16>(sum);
		}
	}

	return true;
}

bool ppu_interpreter::VMINFP(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va].vf;
	const auto b = ppu.vr[op.vb].vf;
	ppu.vr[op.vd].vf = _mm_or_ps(_mm_min_ps(a, b),  _mm_min_ps(b, a));
	return true;
}

bool ppu_interpreter::VMINSB(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va].vi;
	const auto b = ppu.vr[op.vb].vi;
	const auto m = _mm_cmpgt_epi8(a, b);
	ppu.vr[op.vd].vi = _mm_or_si128(_mm_andnot_si128(m, a), _mm_and_si128(m, b));
	return true;
}

bool ppu_interpreter::VMINSH(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_min_epi16(ppu.vr[op.va].vi, ppu.vr[op.vb].vi);
	return true;
}

bool ppu_interpreter::VMINSW(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va].vi;
	const auto b = ppu.vr[op.vb].vi;
	const auto m = _mm_cmpgt_epi32(a, b);
	ppu.vr[op.vd].vi = _mm_or_si128(_mm_andnot_si128(m, a), _mm_and_si128(m, b));
	return true;
}

bool ppu_interpreter::VMINUB(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_min_epu8(ppu.vr[op.va].vi, ppu.vr[op.vb].vi);
	return true;
}

bool ppu_interpreter::VMINUH(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto mask = _mm_set1_epi32(0x80008000);
	ppu.vr[op.vd].vi = _mm_xor_si128(_mm_min_epi16(_mm_xor_si128(ppu.vr[op.va].vi, mask), _mm_xor_si128(ppu.vr[op.vb].vi, mask)), mask);
	return true;
}

bool ppu_interpreter::VMINUW(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va].vi;
	const auto b = ppu.vr[op.vb].vi;
	const auto m = sse_cmpgt_epu32(a, b);
	ppu.vr[op.vd].vi = _mm_or_si128(_mm_andnot_si128(m, a), _mm_and_si128(m, b));
	return true;
}

bool ppu_interpreter::VMLADDUHM(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_add_epi16(_mm_mullo_epi16(ppu.vr[op.va].vi, ppu.vr[op.vb].vi), ppu.vr[op.vc].vi);
	return true;
}

bool ppu_interpreter::VMRGHB(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_unpackhi_epi8(ppu.vr[op.vb].vi, ppu.vr[op.va].vi);
	return true;
}

bool ppu_interpreter::VMRGHH(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_unpackhi_epi16(ppu.vr[op.vb].vi, ppu.vr[op.va].vi);
	return true;
}

bool ppu_interpreter::VMRGHW(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_unpackhi_epi32(ppu.vr[op.vb].vi, ppu.vr[op.va].vi);
	return true;
}

bool ppu_interpreter::VMRGLB(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_unpacklo_epi8(ppu.vr[op.vb].vi, ppu.vr[op.va].vi);
	return true;
}

bool ppu_interpreter::VMRGLH(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_unpacklo_epi16(ppu.vr[op.vb].vi, ppu.vr[op.va].vi);
	return true;
}

bool ppu_interpreter::VMRGLW(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_unpacklo_epi32(ppu.vr[op.vb].vi, ppu.vr[op.va].vi);
	return true;
}

bool ppu_interpreter::VMSUMMBM(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va].vi; // signed bytes
	const auto b = ppu.vr[op.vb].vi; // unsigned bytes
	const auto c = ppu.vr[op.vc].vi;
	const auto ah = _mm_srai_epi16(a, 8);
	const auto bh = _mm_srli_epi16(b, 8);
	const auto al = _mm_srai_epi16(_mm_slli_epi16(a, 8), 8);
	const auto bl = _mm_and_si128(b, _mm_set1_epi16(0x00ff));
	const auto sh = _mm_madd_epi16(ah, bh);
	const auto sl = _mm_madd_epi16(al, bl);
	ppu.vr[op.vd].vi = _mm_add_epi32(_mm_add_epi32(c, sh), sl);
	return true;
}

bool ppu_interpreter::VMSUMSHM(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_add_epi32(_mm_madd_epi16(ppu.vr[op.va].vi, ppu.vr[op.vb].vi), ppu.vr[op.vc].vi);
	return true;
}

bool ppu_interpreter_fast::VMSUMSHS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	const auto& c = ppu.vr[op.vc];

	for (uint w = 0; w < 4; w++)
	{
		s64 result = 0;
		s32 saturated = 0;

		for (uint h = 0; h < 2; h++)
		{
			result += a._s16[w * 2 + h] * b._s16[w * 2 + h];
		}

		result += c._s32[w];

		if (result > 0x7fffffff)
		{
			saturated = 0x7fffffff;
		}
		else if (result < -0x80000000)
		{
			saturated = 0x80000000;
		}
		else
			saturated = static_cast<s32>(result);

		d._s32[w] = saturated;
	}
	return true;
}

bool ppu_interpreter_precise::VMSUMSHS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	const auto& c = ppu.vr[op.vc];

	for (uint w = 0; w < 4; w++)
	{
		s64 result = 0;
		s32 saturated = 0;

		for (uint h = 0; h < 2; h++)
		{
			result += a._s16[w * 2 + h] * b._s16[w * 2 + h];
		}

		result += c._s32[w];

		if (result > 0x7fffffff)
		{
			saturated = 0x7fffffff;
			ppu.sat = true;
		}
		else if (result < -0x80000000)
		{
			saturated = 0x80000000;
			ppu.sat = true;
		}
		else
			saturated = static_cast<s32>(result);

		d._s32[w] = saturated;
	}
	return true;
}

bool ppu_interpreter::VMSUMUBM(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va].vi;
	const auto b = ppu.vr[op.vb].vi;
	const auto c = ppu.vr[op.vc].vi;
	const auto mask = _mm_set1_epi16(0x00ff);
	const auto ah = _mm_srli_epi16(a, 8);
	const auto al = _mm_and_si128(a, mask);
	const auto bh = _mm_srli_epi16(b, 8);
	const auto bl = _mm_and_si128(b, mask);
	const auto sh = _mm_madd_epi16(ah, bh);
	const auto sl = _mm_madd_epi16(al, bl);
	ppu.vr[op.vd].vi = _mm_add_epi32(_mm_add_epi32(c, sh), sl);
	return true;
}

bool ppu_interpreter::VMSUMUHM(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va].vi;
	const auto b = ppu.vr[op.vb].vi;
	const auto c = ppu.vr[op.vc].vi;
	const auto ml = _mm_mullo_epi16(a, b); // low results
	const auto mh = _mm_mulhi_epu16(a, b); // high results
	const auto ls = _mm_add_epi32(_mm_srli_epi32(ml, 16), _mm_and_si128(ml, _mm_set1_epi32(0x0000ffff)));
	const auto hs = _mm_add_epi32(_mm_slli_epi32(mh, 16), _mm_and_si128(mh, _mm_set1_epi32(0xffff0000)));
	ppu.vr[op.vd].vi = _mm_add_epi32(_mm_add_epi32(c, ls), hs);
	return true;
}

bool ppu_interpreter_fast::VMSUMUHS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	const auto& c = ppu.vr[op.vc];

	for (uint w = 0; w < 4; w++)
	{
		u64 result = 0;
		u32 saturated = 0;

		for (uint h = 0; h < 2; h++)
		{
			result += u64{a._u16[w * 2 + h]} * b._u16[w * 2 + h];
		}

		result += c._u32[w];

		if (result > 0xffffffffu)
		{
			saturated = 0xffffffff;
		}
		else
			saturated = static_cast<u32>(result);

		d._u32[w] = saturated;
	}
	return true;
}

bool ppu_interpreter_precise::VMSUMUHS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	const auto& c = ppu.vr[op.vc];

	for (uint w = 0; w < 4; w++)
	{
		u64 result = 0;
		u32 saturated = 0;

		for (uint h = 0; h < 2; h++)
		{
			result += u64{a._u16[w * 2 + h]} * b._u16[w * 2 + h];
		}

		result += c._u32[w];

		if (result > 0xffffffffu)
		{
			saturated = 0xffffffff;
			ppu.sat = true;
		}
		else
			saturated = static_cast<u32>(result);

		d._u32[w] = saturated;
	}
	return true;
}

bool ppu_interpreter::VMULESB(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_mullo_epi16(_mm_srai_epi16(ppu.vr[op.va].vi, 8), _mm_srai_epi16(ppu.vr[op.vb].vi, 8));
	return true;
}

bool ppu_interpreter::VMULESH(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_madd_epi16(_mm_srli_epi32(ppu.vr[op.va].vi, 16), _mm_srli_epi32(ppu.vr[op.vb].vi, 16));
	return true;
}

bool ppu_interpreter::VMULEUB(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_mullo_epi16(_mm_srli_epi16(ppu.vr[op.va].vi, 8), _mm_srli_epi16(ppu.vr[op.vb].vi, 8));
	return true;
}

bool ppu_interpreter::VMULEUH(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va].vi;
	const auto b = ppu.vr[op.vb].vi;
	const auto ml = _mm_mullo_epi16(a, b);
	const auto mh = _mm_mulhi_epu16(a, b);
	ppu.vr[op.vd].vi = _mm_or_si128(_mm_srli_epi32(ml, 16), _mm_and_si128(mh, _mm_set1_epi32(0xffff0000)));
	return true;
}

bool ppu_interpreter::VMULOSB(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_mullo_epi16(_mm_srai_epi16(_mm_slli_epi16(ppu.vr[op.va].vi, 8), 8), _mm_srai_epi16(_mm_slli_epi16(ppu.vr[op.vb].vi, 8), 8));
	return true;
}

bool ppu_interpreter::VMULOSH(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto mask = _mm_set1_epi32(0x0000ffff);
	ppu.vr[op.vd].vi = _mm_madd_epi16(_mm_and_si128(ppu.vr[op.va].vi, mask), _mm_and_si128(ppu.vr[op.vb].vi, mask));
	return true;
}

bool ppu_interpreter::VMULOUB(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto mask = _mm_set1_epi16(0x00ff);
	ppu.vr[op.vd].vi = _mm_mullo_epi16(_mm_and_si128(ppu.vr[op.va].vi, mask), _mm_and_si128(ppu.vr[op.vb].vi, mask));
	return true;
}

bool ppu_interpreter::VMULOUH(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va].vi;
	const auto b = ppu.vr[op.vb].vi;
	const auto ml = _mm_mullo_epi16(a, b);
	const auto mh = _mm_mulhi_epu16(a, b);
	ppu.vr[op.vd].vi = _mm_or_si128(_mm_slli_epi32(mh, 16), _mm_and_si128(ml, _mm_set1_epi32(0x0000ffff)));
	return true;
}

bool ppu_interpreter::VNMSUBFP(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vf = _mm_xor_ps(_mm_sub_ps(_mm_mul_ps(ppu.vr[op.va].vf, ppu.vr[op.vc].vf), ppu.vr[op.vb].vf), _mm_set1_ps(-0.0f));
	return true;
}

bool ppu_interpreter::VNOR(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd] = ~(ppu.vr[op.va] | ppu.vr[op.vb]);
	return true;
}

bool ppu_interpreter::VOR(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd] = ppu.vr[op.va] | ppu.vr[op.vb];
	return true;
}

bool ppu_interpreter_precise::VPERM(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = sse_altivec_vperm_v0(ppu.vr[op.va].vi, ppu.vr[op.vb].vi, ppu.vr[op.vc].vi);
	return true;
}

bool ppu_interpreter_fast::VPERM(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = sse_altivec_vperm(ppu.vr[op.va].vi, ppu.vr[op.vb].vi, ppu.vr[op.vc].vi);
	return true;
}

bool ppu_interpreter::VPKPX(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	v128 VA = ppu.vr[op.va];
	v128 VB = ppu.vr[op.vb];
	for (uint h = 0; h < 4; h++)
	{
		u16 bb7 = VB._u8[15 - (h * 4 + 0)] & 0x1;
		u16 bb8 = VB._u8[15 - (h * 4 + 1)] >> 3;
		u16 bb16 = VB._u8[15 - (h * 4 + 2)] >> 3;
		u16 bb24 = VB._u8[15 - (h * 4 + 3)] >> 3;
		u16 ab7 = VA._u8[15 - (h * 4 + 0)] & 0x1;
		u16 ab8 = VA._u8[15 - (h * 4 + 1)] >> 3;
		u16 ab16 = VA._u8[15 - (h * 4 + 2)] >> 3;
		u16 ab24 = VA._u8[15 - (h * 4 + 3)] >> 3;

		d._u16[3 - h] = (bb7 << 15) | (bb8 << 10) | (bb16 << 5) | bb24;
		d._u16[4 + (3 - h)] = (ab7 << 15) | (ab8 << 10) | (ab16 << 5) | ab24;
	}
	return true;
}

bool ppu_interpreter_fast::VPKSHSS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_packs_epi16(ppu.vr[op.vb].vi, ppu.vr[op.va].vi);
	return true;
}

bool ppu_interpreter_precise::VPKSHSS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	v128 d;

	for (u8 i = 0; i < 8; i++)
	{
		s16 result = a._s16[i];

		if (result < INT8_MIN)
		{
			d._s8[i + 8] = INT8_MIN;
			ppu.sat = true;
		}
		else if (result > INT8_MAX)
		{
			d._s8[i + 8] = INT8_MAX;
			ppu.sat = true;
		}
		else
		{
			d._s8[i + 8] = static_cast<s8>(result);
		}

		result = b._s16[i];

		if (result < INT8_MIN)
		{
			d._s8[i] = INT8_MIN;
			ppu.sat = true;
		}
		else if (result > INT8_MAX)
		{
			d._s8[i] = INT8_MAX;
			ppu.sat = true;
		}
		else
		{
			d._s8[i] = static_cast<s8>(result);
		}
	}

	ppu.vr[op.vd] = d;
	return true;
}

bool ppu_interpreter_fast::VPKSHUS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_packus_epi16(ppu.vr[op.vb].vi, ppu.vr[op.va].vi);
	return true;
}

bool ppu_interpreter_precise::VPKSHUS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.vr[op.va];
	const auto b = ppu.vr[op.vb];

	// Detect saturation
	{
		const u64 mask = 0xFF00FF00FF00FF00ULL;
		const auto all_bits = a | b;
		if ((all_bits._u64[0] | all_bits._u64[1]) & mask)
		{
			ppu.sat = true;
		}
	}

	ppu.vr[op.vd].vi = _mm_packus_epi16(b.vi, a.vi);
	return true;
}

bool ppu_interpreter_fast::VPKSWSS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_packs_epi32(ppu.vr[op.vb].vi, ppu.vr[op.va].vi);
	return true;
}

bool ppu_interpreter_precise::VPKSWSS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	v128 d;

	for (u8 i = 0; i < 4; i++)
	{
		s32 result = a._s32[i];

		if (result < INT16_MIN)
		{
			d._s16[i + 4] = INT16_MIN;
			ppu.sat = true;
		}
		else if (result > INT16_MAX)
		{
			d._s16[i + 4] = INT16_MAX;
			ppu.sat = true;
		}
		else
		{
			d._s16[i + 4] = static_cast<s16>(result);
		}

		result = b._s32[i];

		if (result < INT16_MIN)
		{
			d._s16[i] = INT16_MIN;
			ppu.sat = true;
		}
		else if (result > INT16_MAX)
		{
			d._s16[i] = INT16_MAX;
			ppu.sat = true;
		}
		else
		{
			d._s16[i] = static_cast<s16>(result);
		}
	}

	ppu.vr[op.vd] = d;
	return true;
}

bool ppu_interpreter_fast::VPKSWUS(ppu_thread& ppu, ppu_opcode_t op)
{
	//ppu.vr[op.vd].vi = _mm_packus_epi32(ppu.vr[op.vb].vi, ppu.vr[op.va].vi);
	auto& d = ppu.vr[op.vd];
	v128 VA = ppu.vr[op.va];
	v128 VB = ppu.vr[op.vb];
	for (uint h = 0; h < 4; h++)
	{
		s32 result = VA._s32[h];

		if (result > UINT16_MAX)
		{
			result = UINT16_MAX;
		}
		else if (result < 0)
		{
			result = 0;
		}

		d._u16[h + 4] = result;

		result = VB._s32[h];

		if (result > UINT16_MAX)
		{
			result = UINT16_MAX;
		}
		else if (result < 0)
		{
			result = 0;
		}

		d._u16[h] = result;
	}
	return true;
}

bool ppu_interpreter_precise::VPKSWUS(ppu_thread& ppu, ppu_opcode_t op)
{
	//ppu.vr[op.vd].vi = _mm_packus_epi32(ppu.vr[op.vb].vi, ppu.vr[op.va].vi);
	auto& d = ppu.vr[op.vd];
	v128 VA = ppu.vr[op.va];
	v128 VB = ppu.vr[op.vb];
	for (uint h = 0; h < 4; h++)
	{
		s32 result = VA._s32[h];

		if (result > UINT16_MAX)
		{
			result = UINT16_MAX;
			ppu.sat = true;
		}
		else if (result < 0)
		{
			result = 0;
			ppu.sat = true;
		}

		d._u16[h + 4] = result;

		result = VB._s32[h];

		if (result > UINT16_MAX)
		{
			result = UINT16_MAX;
			ppu.sat = true;
		}
		else if (result < 0)
		{
			result = 0;
			ppu.sat = true;
		}

		d._u16[h] = result;
	}
	return true;
}

bool ppu_interpreter::VPKUHUM(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	v128 VA = ppu.vr[op.va];
	v128 VB = ppu.vr[op.vb];
	for (uint b = 0; b < 8; b++)
	{
		d._u8[b + 8] = VA._u8[b * 2];
		d._u8[b] = VB._u8[b * 2];
	}
	return true;
}

bool ppu_interpreter_fast::VPKUHUS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	v128 VA = ppu.vr[op.va];
	v128 VB = ppu.vr[op.vb];
	for (uint b = 0; b < 8; b++)
	{
		u16 result = VA._u16[b];

		if (result > UINT8_MAX)
		{
			result = UINT8_MAX;
		}

		d._u8[b + 8] = static_cast<u8>(result);

		result = VB._u16[b];

		if (result > UINT8_MAX)
		{
			result = UINT8_MAX;
		}

		d._u8[b] = static_cast<u8>(result);
	}
	return true;
}

bool ppu_interpreter_precise::VPKUHUS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	v128 VA = ppu.vr[op.va];
	v128 VB = ppu.vr[op.vb];
	for (uint b = 0; b < 8; b++)
	{
		u16 result = VA._u16[b];

		if (result > UINT8_MAX)
		{
			result = UINT8_MAX;
			ppu.sat = true;
		}

		d._u8[b + 8] = static_cast<u8>(result);

		result = VB._u16[b];

		if (result > UINT8_MAX)
		{
			result = UINT8_MAX;
			ppu.sat = true;
		}

		d._u8[b] = static_cast<u8>(result);
	}
	return true;
}

bool ppu_interpreter::VPKUWUM(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	v128 VA = ppu.vr[op.va];
	v128 VB = ppu.vr[op.vb];
	for (uint h = 0; h < 4; h++)
	{
		d._u16[h + 4] = VA._u16[h * 2];
		d._u16[h] = VB._u16[h * 2];
	}
	return true;
}

bool ppu_interpreter_fast::VPKUWUS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	v128 VA = ppu.vr[op.va];
	v128 VB = ppu.vr[op.vb];
	for (uint h = 0; h < 4; h++)
	{
		u32 result = VA._u32[h];

		if (result > UINT16_MAX)
		{
			result = UINT16_MAX;
		}

		d._u16[h + 4] = result;

		result = VB._u32[h];

		if (result > UINT16_MAX)
		{
			result = UINT16_MAX;
		}

		d._u16[h] = result;
	}
	return true;
}

bool ppu_interpreter_precise::VPKUWUS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	v128 VA = ppu.vr[op.va];
	v128 VB = ppu.vr[op.vb];
	for (uint h = 0; h < 4; h++)
	{
		u32 result = VA._u32[h];

		if (result > UINT16_MAX)
		{
			result = UINT16_MAX;
			ppu.sat = true;
		}

		d._u16[h + 4] = result;

		result = VB._u32[h];

		if (result > UINT16_MAX)
		{
			result = UINT16_MAX;
			ppu.sat = true;
		}

		d._u16[h] = result;
	}
	return true;
}

bool ppu_interpreter::VREFP(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vf = _mm_div_ps(_mm_set_ps(1.0f, 1.0f, 1.0f, 1.0f), ppu.vr[op.vb].vf);
	return true;
}

bool ppu_interpreter::VRFIM(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._f[w] = std::floor(b._f[w]);
	}
	return true;
}

bool ppu_interpreter::VRFIN(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._f[w] = std::nearbyint(b._f[w]);
	}
	return true;
}

bool ppu_interpreter::VRFIP(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._f[w] = std::ceil(b._f[w]);
	}
	return true;
}

bool ppu_interpreter::VRFIZ(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._f[w] = std::truncf(b._f[w]);
	}
	return true;
}

bool ppu_interpreter::VRLB(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint i = 0; i < 16; i++)
	{
		d._u8[i] = utils::rol8(a._u8[i], b._u8[i]);
	}
	return true;
}

bool ppu_interpreter::VRLH(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint i = 0; i < 8; i++)
	{
		d._u16[i] = utils::rol16(a._u16[i], b._u8[i * 2] & 0xf);
	}
	return true;
}

bool ppu_interpreter::VRLW(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._u32[w] = utils::rol32(a._u32[w], b._u8[w * 4] & 0x1f);
	}
	return true;
}

bool ppu_interpreter::VRSQRTEFP(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vf = _mm_div_ps(_mm_set_ps(1.0f, 1.0f, 1.0f, 1.0f), _mm_sqrt_ps(ppu.vr[op.vb].vf));
	return true;
}

bool ppu_interpreter::VSEL(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	const auto& c = ppu.vr[op.vc];

	d = (b & c) | v128::andnot(c, a);
	return true;
}

bool ppu_interpreter::VSL(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	v128 VA = ppu.vr[op.va];
	u8 sh = ppu.vr[op.vb]._u8[0] & 0x7;

	d._u8[0] = VA._u8[0] << sh;
	for (uint b = 1; b < 16; b++)
	{
		sh = ppu.vr[op.vb]._u8[b] & 0x7;
		d._u8[b] = (VA._u8[b] << sh) | (VA._u8[b - 1] >> (8 - sh));
	}
	return true;
}

bool ppu_interpreter::VSLB(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint i = 0; i < 16; i++)
	{
		d._u8[i] = a._u8[i] << (b._u8[i] & 0x7);
	}
	return true;
}

bool ppu_interpreter::VSLDOI(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	u8 tmpSRC[32];
	std::memcpy(tmpSRC, &ppu.vr[op.vb], 16);
	std::memcpy(tmpSRC + 16, &ppu.vr[op.va], 16);

	for (uint b = 0; b<16; b++)
	{
		d._u8[15 - b] = tmpSRC[31 - (b + op.vsh)];
	}
	return true;
}

bool ppu_interpreter::VSLH(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint h = 0; h < 8; h++)
	{
		d._u16[h] = a._u16[h] << (b._u16[h] & 0xf);
	}
	return true;
}

bool ppu_interpreter::VSLO(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	v128 VA = ppu.vr[op.va];
	u8 nShift = (ppu.vr[op.vb]._u8[0] >> 3) & 0xf;

	d.clear();

	for (u8 b = 0; b < 16 - nShift; b++)
	{
		d._u8[15 - b] = VA._u8[15 - (b + nShift)];
	}
	return true;
}

bool ppu_interpreter::VSLW(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._u32[w] = a._u32[w] << (b._u32[w] & 0x1f);
	}
	return true;
}

bool ppu_interpreter::VSPLTB(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	u8 byte = ppu.vr[op.vb]._u8[15 - op.vuimm];

	for (uint b = 0; b < 16; b++)
	{
		d._u8[b] = byte;
	}
	return true;
}

bool ppu_interpreter::VSPLTH(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	verify(HERE), (op.vuimm < 8);

	u16 hword = ppu.vr[op.vb]._u16[7 - op.vuimm];

	for (uint h = 0; h < 8; h++)
	{
		d._u16[h] = hword;
	}
	return true;
}

bool ppu_interpreter::VSPLTISB(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const s8 imm = op.vsimm;

	for (uint b = 0; b < 16; b++)
	{
		d._u8[b] = imm;
	}
	return true;
}

bool ppu_interpreter::VSPLTISH(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const s16 imm = op.vsimm;

	for (uint h = 0; h < 8; h++)
	{
		d._u16[h] = imm;
	}
	return true;
}

bool ppu_interpreter::VSPLTISW(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const s32 imm = op.vsimm;

	for (uint w = 0; w < 4; w++)
	{
		d._u32[w] = imm;
	}
	return true;
}

bool ppu_interpreter::VSPLTW(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	verify(HERE), (op.vuimm < 4);

	u32 word = ppu.vr[op.vb]._u32[3 - op.vuimm];

	for (uint w = 0; w < 4; w++)
	{
		d._u32[w] = word;
	}
	return true;
}

bool ppu_interpreter::VSR(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	v128 VA = ppu.vr[op.va];
	u8 sh = ppu.vr[op.vb]._u8[15] & 0x7;

	d._u8[15] = VA._u8[15] >> sh;
	for (uint b = 14; ~b; b--)
	{
		sh = ppu.vr[op.vb]._u8[b] & 0x7;
		d._u8[b] = (VA._u8[b] >> sh) | (VA._u8[b + 1] << (8 - sh));
	}
	return true;
}

bool ppu_interpreter::VSRAB(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint i = 0; i < 16; i++)
	{
		d._s8[i] = a._s8[i] >> (b._u8[i] & 0x7);
	}
	return true;
}

bool ppu_interpreter::VSRAH(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint h = 0; h < 8; h++)
	{
		d._s16[h] = a._s16[h] >> (b._u16[h] & 0xf);
	}
	return true;
}

bool ppu_interpreter::VSRAW(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._s32[w] = a._s32[w] >> (b._u32[w] & 0x1f);
	}
	return true;
}

bool ppu_interpreter::VSRB(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint i = 0; i < 16; i++)
	{
		d._u8[i] = a._u8[i] >> (b._u8[i] & 0x7);
	}
	return true;
}

bool ppu_interpreter::VSRH(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint h = 0; h < 8; h++)
	{
		d._u16[h] = a._u16[h] >> (b._u16[h] & 0xf);
	}
	return true;
}

bool ppu_interpreter::VSRO(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	v128 VA = ppu.vr[op.va];
	u8 nShift = (ppu.vr[op.vb]._u8[0] >> 3) & 0xf;

	d.clear();

	for (u8 b = 0; b < 16 - nShift; b++)
	{
		d._u8[b] = VA._u8[b + nShift];
	}
	return true;
}

bool ppu_interpreter::VSRW(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._u32[w] = a._u32[w] >> (b._u32[w] & 0x1f);
	}
	return true;
}

bool ppu_interpreter::VSUBCUW(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._u32[w] = a._u32[w] < b._u32[w] ? 0 : 1;
	}
	return true;
}

bool ppu_interpreter::VSUBFP(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd] = v128::subfs(ppu.vr[op.va], ppu.vr[op.vb]);
	return true;
}

bool ppu_interpreter_fast::VSUBSBS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_subs_epi8(ppu.vr[op.va].vi, ppu.vr[op.vb].vi);
	return true;
}

bool ppu_interpreter_precise::VSUBSBS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	auto& d = ppu.vr[op.vd];

	for (u8 i = 0; i < 16; i++)
	{
		const s16 diff = a._s8[i] - b._s8[i];

		if (diff < INT8_MIN)
		{
			d._s8[i] = INT8_MIN;
			ppu.sat = true;
		}
		else if (diff > INT8_MAX)
		{
			d._s8[i] = INT8_MAX;
			ppu.sat = true;
		}
		else
		{
			d._s8[i] = static_cast<s8>(diff);
		}
	}

	return true;
}

bool ppu_interpreter_fast::VSUBSHS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_subs_epi16(ppu.vr[op.va].vi, ppu.vr[op.vb].vi);
	return true;
}

bool ppu_interpreter_precise::VSUBSHS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	auto& d = ppu.vr[op.vd];

	for (u8 i = 0; i < 8; i++)
	{
		const s32 diff = a._s16[i] - b._s16[i];

		if (diff < INT16_MIN)
		{
			d._s16[i] = INT16_MIN;
			ppu.sat = true;
		}
		else if (diff > INT16_MAX)
		{
			d._s16[i] = INT16_MAX;
			ppu.sat = true;
		}
		else
		{
			d._s16[i] = static_cast<s16>(diff);
		}
	}

	return true;
}

bool ppu_interpreter_fast::VSUBSWS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		const s64 result = s64{a._s32[w]} - b._s32[w];

		if (result < INT32_MIN)
		{
			d._s32[w] = INT32_MIN;
		}
		else if (result > INT32_MAX)
		{
			d._s32[w] = INT32_MAX;
		}
		else
			d._s32[w] = static_cast<s32>(result);
	}
	return true;
}

bool ppu_interpreter_precise::VSUBSWS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		const s64 result = s64{a._s32[w]} - b._s32[w];

		if (result < INT32_MIN)
		{
			d._s32[w] = INT32_MIN;
			ppu.sat = true;
		}
		else if (result > INT32_MAX)
		{
			d._s32[w] = INT32_MAX;
			ppu.sat = true;
		}
		else
			d._s32[w] = static_cast<s32>(result);
	}
	return true;
}

bool ppu_interpreter::VSUBUBM(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd] = v128::sub8(ppu.vr[op.va], ppu.vr[op.vb]);
	return true;
}

bool ppu_interpreter_fast::VSUBUBS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_subs_epu8(ppu.vr[op.va].vi, ppu.vr[op.vb].vi);
	return true;
}

bool ppu_interpreter_precise::VSUBUBS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	auto& d = ppu.vr[op.vd];

	for (u8 i = 0; i < 16; i++)
	{
		const s16 diff = a._u8[i] - b._u8[i];

		if (diff < 0)
		{
			d._u8[i] = 0;
			ppu.sat = true;
		}
		else if (diff > UINT8_MAX)
		{
			d._u8[i] = UINT8_MAX;
			ppu.sat = true;
		}
		else
		{
			d._u8[i] = static_cast<u8>(diff);
		}
	}

	return true;
}

bool ppu_interpreter::VSUBUHM(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd] = v128::sub16(ppu.vr[op.va], ppu.vr[op.vb]);
	return true;
}

bool ppu_interpreter_fast::VSUBUHS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd].vi = _mm_subs_epu16(ppu.vr[op.va].vi, ppu.vr[op.vb].vi);
	return true;
}

bool ppu_interpreter_precise::VSUBUHS(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];
	auto& d = ppu.vr[op.vd];

	for (u8 i = 0; i < 8; i++)
	{
		const s32 diff = a._u16[i] - b._u16[i];

		if (diff < 0)
		{
			d._u16[i] = 0;
			ppu.sat = true;
		}
		else if (diff > UINT16_MAX)
		{
			d._u16[i] = UINT16_MAX;
			ppu.sat = true;
		}
		else
		{
			d._u16[i] = static_cast<u16>(diff);
		}
	}

	return true;
}

bool ppu_interpreter::VSUBUWM(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd] = v128::sub32(ppu.vr[op.va], ppu.vr[op.vb]);
	return true;
}

bool ppu_interpreter_fast::VSUBUWS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		const s64 result = s64{a._u32[w]} - b._u32[w];

		if (result < 0)
		{
			d._u32[w] = 0;
		}
		else
			d._u32[w] = static_cast<u32>(result);
	}
	return true;
}

bool ppu_interpreter_precise::VSUBUWS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		const s64 result = s64{a._u32[w]} - b._u32[w];

		if (result < 0)
		{
			d._u32[w] = 0;
			ppu.sat = true;
		}
		else
			d._u32[w] = static_cast<u32>(result);
	}
	return true;
}

bool ppu_interpreter_fast::VSUMSWS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	s64 sum = b._s32[0];

	for (uint w = 0; w < 4; w++)
	{
		sum += a._s32[w];
	}

	d.clear();
	if (sum > INT32_MAX)
	{
		d._s32[0] = INT32_MAX;
	}
	else if (sum < INT32_MIN)
	{
		d._s32[0] = INT32_MIN;
	}
	else
		d._s32[0] = static_cast<s32>(sum);
	return true;
}

bool ppu_interpreter_precise::VSUMSWS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	s64 sum = b._s32[0];

	for (uint w = 0; w < 4; w++)
	{
		sum += a._s32[w];
	}

	d.clear();
	if (sum > INT32_MAX)
	{
		d._s32[0] = INT32_MAX;
		ppu.sat = true;
	}
	else if (sum < INT32_MIN)
	{
		d._s32[0] = INT32_MIN;
		ppu.sat = true;
	}
	else
		d._s32[0] = static_cast<s32>(sum);
	return true;
}

bool ppu_interpreter_fast::VSUM2SWS(ppu_thread& ppu, ppu_opcode_t op)
{
	v128 d;
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint n = 0; n < 2; n++)
	{
		const s64 sum = s64{a._s32[n * 2]} + a._s32[n * 2 + 1] + b._s32[n * 2];

		if (sum > INT32_MAX)
		{
			d._s32[n * 2] = INT32_MAX;
		}
		else if (sum < INT32_MIN)
		{
			d._s32[n * 2] = INT32_MIN;
		}
		else
			d._s32[n * 2] = static_cast<s32>(sum);
	}
	d._s32[1] = 0;
	d._s32[3] = 0;
	ppu.vr[op.vd] = d;
	return true;
}

bool ppu_interpreter_precise::VSUM2SWS(ppu_thread& ppu, ppu_opcode_t op)
{
	v128 d;
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint n = 0; n < 2; n++)
	{
		const s64 sum = s64{a._s32[n * 2]} + a._s32[n * 2 + 1] + b._s32[n * 2];

		if (sum > INT32_MAX)
		{
			d._s32[n * 2] = INT32_MAX;
			ppu.sat = true;
		}
		else if (sum < INT32_MIN)
		{
			d._s32[n * 2] = INT32_MIN;
			ppu.sat = true;
		}
		else
			d._s32[n * 2] = static_cast<s32>(sum);
	}

	d._s32[1] = 0;
	d._s32[3] = 0;
	ppu.vr[op.vd] = d;
	return true;
}

bool ppu_interpreter_fast::VSUM4SBS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		s64 sum = b._s32[w];

		for (uint b = 0; b < 4; b++)
		{
			sum += a._s8[w * 4 + b];
		}

		if (sum > INT32_MAX)
		{
			d._s32[w] = INT32_MAX;
		}
		else if (sum < INT32_MIN)
		{
			d._s32[w] = INT32_MIN;
		}
		else
			d._s32[w] = static_cast<s32>(sum);
	}
	return true;
}

bool ppu_interpreter_precise::VSUM4SBS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		s64 sum = b._s32[w];

		for (uint b = 0; b < 4; b++)
		{
			sum += a._s8[w * 4 + b];
		}

		if (sum > INT32_MAX)
		{
			d._s32[w] = INT32_MAX;
			ppu.sat = true;
		}
		else if (sum < INT32_MIN)
		{
			d._s32[w] = INT32_MIN;
			ppu.sat = true;
		}
		else
			d._s32[w] = static_cast<s32>(sum);
	}
	return true;
}

bool ppu_interpreter_fast::VSUM4SHS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		s64 sum = b._s32[w];

		for (uint h = 0; h < 2; h++)
		{
			sum += a._s16[w * 2 + h];
		}

		if (sum > INT32_MAX)
		{
			d._s32[w] = INT32_MAX;
		}
		else if (sum < INT32_MIN)
		{
			d._s32[w] = INT32_MIN;
		}
		else
			d._s32[w] = static_cast<s32>(sum);
	}
	return true;
}

bool ppu_interpreter_precise::VSUM4SHS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		s64 sum = b._s32[w];

		for (uint h = 0; h < 2; h++)
		{
			sum += a._s16[w * 2 + h];
		}

		if (sum > INT32_MAX)
		{
			d._s32[w] = INT32_MAX;
			ppu.sat = true;
		}
		else if (sum < INT32_MIN)
		{
			d._s32[w] = INT32_MIN;
			ppu.sat = true;
		}
		else
			d._s32[w] = static_cast<s32>(sum);
	}
	return true;
}

bool ppu_interpreter_fast::VSUM4UBS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		u64 sum = b._u32[w];

		for (uint b = 0; b < 4; b++)
		{
			sum += a._u8[w * 4 + b];
		}

		if (sum > UINT32_MAX)
		{
			d._u32[w] = UINT32_MAX;
		}
		else
			d._u32[w] = static_cast<u32>(sum);
	}
	return true;
}

bool ppu_interpreter_precise::VSUM4UBS(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	const auto& a = ppu.vr[op.va];
	const auto& b = ppu.vr[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		u64 sum = b._u32[w];

		for (uint b = 0; b < 4; b++)
		{
			sum += a._u8[w * 4 + b];
		}

		if (sum > UINT32_MAX)
		{
			d._u32[w] = UINT32_MAX;
			ppu.sat = true;
		}
		else
			d._u32[w] = static_cast<u32>(sum);
	}
	return true;
}

bool ppu_interpreter::VUPKHPX(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	v128 VB = ppu.vr[op.vb];
	for (uint w = 0; w < 4; w++)
	{
		d._s8[w * 4 + 3] = VB._s8[8 + w * 2 + 1] >> 7;  // signed shift sign extends
		d._u8[w * 4 + 2] = (VB._u8[8 + w * 2 + 1] >> 2) & 0x1f;
		d._u8[w * 4 + 1] = ((VB._u8[8 + w * 2 + 1] & 0x3) << 3) | ((VB._u8[8 + w * 2 + 0] >> 5) & 0x7);
		d._u8[w * 4 + 0] = VB._u8[8 + w * 2 + 0] & 0x1f;
	}
	return true;
}

bool ppu_interpreter::VUPKHSB(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	v128 VB = ppu.vr[op.vb];
	for (uint h = 0; h < 8; h++)
	{
		d._s16[h] = VB._s8[8 + h];
	}
	return true;
}

bool ppu_interpreter::VUPKHSH(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	v128 VB = ppu.vr[op.vb];
	for (uint w = 0; w < 4; w++)
	{
		d._s32[w] = VB._s16[4 + w];
	}
	return true;
}

bool ppu_interpreter::VUPKLPX(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	v128 VB = ppu.vr[op.vb];
	for (uint w = 0; w < 4; w++)
	{
		d._s8[w * 4 + 3] = VB._s8[w * 2 + 1] >> 7;  // signed shift sign extends
		d._u8[w * 4 + 2] = (VB._u8[w * 2 + 1] >> 2) & 0x1f;
		d._u8[w * 4 + 1] = ((VB._u8[w * 2 + 1] & 0x3) << 3) | ((VB._u8[w * 2 + 0] >> 5) & 0x7);
		d._u8[w * 4 + 0] = VB._u8[w * 2 + 0] & 0x1f;
	}
	return true;
}

bool ppu_interpreter::VUPKLSB(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	v128 VB = ppu.vr[op.vb];
	for (uint h = 0; h < 8; h++)
	{
		d._s16[h] = VB._s8[h];
	}
	return true;
}

bool ppu_interpreter::VUPKLSH(ppu_thread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.vr[op.vd];
	v128 VB = ppu.vr[op.vb];
	for (uint w = 0; w < 4; w++)
	{
		d._s32[w] = VB._s16[w];
	}
	return true;
}

bool ppu_interpreter::VXOR(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.vr[op.vd] = ppu.vr[op.va] ^ ppu.vr[op.vb];
	return true;
}

bool ppu_interpreter::TDI(ppu_thread& ppu, ppu_opcode_t op)
{
	const s64 a = ppu.gpr[op.ra], b = op.simm16;
	const u64 a_ = a, b_ = b;

	if (((op.bo & 0x10) && a < b) ||
		((op.bo & 0x8) && a > b) ||
		((op.bo & 0x4) && a == b) ||
		((op.bo & 0x2) && a_ < b_) ||
		((op.bo & 0x1) && a_ > b_))
	{
		fmt::throw_exception("Trap!" HERE);
	}

	return true;
}

bool ppu_interpreter::TWI(ppu_thread& ppu, ppu_opcode_t op)
{
	const s32 a = static_cast<u32>(ppu.gpr[op.ra]), b = op.simm16;
	const u32 a_ = a, b_ = b;

	if (((op.bo & 0x10) && a < b) ||
		((op.bo & 0x8) && a > b) ||
		((op.bo & 0x4) && a == b) ||
		((op.bo & 0x2) && a_ < b_) ||
		((op.bo & 0x1) && a_ > b_))
	{
		fmt::throw_exception("Trap!" HERE);
	}

	return true;
}

bool ppu_interpreter::MULLI(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.rd] = static_cast<s64>(ppu.gpr[op.ra]) * op.simm16;
	return true;
}

bool ppu_interpreter::SUBFIC(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 a = ppu.gpr[op.ra];
	const s64 i = op.simm16;
	const auto r = add64_flags(~a, i, 1);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	return true;
}

bool ppu_interpreter::CMPLI(ppu_thread& ppu, ppu_opcode_t op)
{
	if (op.l10)
	{
		ppu_cr_set<u64>(ppu, op.crfd, ppu.gpr[op.ra], op.uimm16);
	}
	else
	{
		ppu_cr_set<u32>(ppu, op.crfd, static_cast<u32>(ppu.gpr[op.ra]), op.uimm16);
	}
	return true;
}

bool ppu_interpreter::CMPI(ppu_thread& ppu, ppu_opcode_t op)
{
	if (op.l10)
	{
		ppu_cr_set<s64>(ppu, op.crfd, ppu.gpr[op.ra], op.simm16);
	}
	else
	{
		ppu_cr_set<s32>(ppu, op.crfd, static_cast<u32>(ppu.gpr[op.ra]), op.simm16);
	}
	return true;
}

bool ppu_interpreter::ADDIC(ppu_thread& ppu, ppu_opcode_t op)
{
	const s64 a = ppu.gpr[op.ra];
	const s64 i = op.simm16;
	const auto r = add64_flags(a, i);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if (op.main & 1) [[unlikely]] ppu_cr_set<s64>(ppu, 0, r.result, 0);
	return true;
}

bool ppu_interpreter::ADDI(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.rd] = op.ra ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	return true;
}

bool ppu_interpreter::ADDIS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.rd] = op.ra ? ppu.gpr[op.ra] + (op.simm16 * 65536) : (op.simm16 * 65536);
	return true;
}

bool ppu_interpreter::BC(ppu_thread& ppu, ppu_opcode_t op)
{
	const bool bo0 = (op.bo & 0x10) != 0;
	const bool bo1 = (op.bo & 0x08) != 0;
	const bool bo2 = (op.bo & 0x04) != 0;
	const bool bo3 = (op.bo & 0x02) != 0;

	ppu.ctr -= (bo2 ^ true);
	if (op.lk) ppu.lr = ppu.cia + 4;

	const bool ctr_ok = bo2 | ((ppu.ctr != 0) ^ bo3);
	const bool cond_ok = bo0 | (ppu.cr[op.bi] ^ (bo1 ^ true));

	if (ctr_ok && cond_ok)
	{
		ppu.cia = (op.aa ? 0 : ppu.cia) + op.bt14;
		return false;
	}
	else
	{
		return true;
	}
}

bool ppu_interpreter::SC(ppu_thread& ppu, ppu_opcode_t op)
{
	if (op.opcode != ppu_instructions::SC(0))
	{
		return UNK(ppu, op);
	}

	ppu_execute_syscall(ppu, ppu.gpr[11]);
	return false;
}

bool ppu_interpreter::B(ppu_thread& ppu, ppu_opcode_t op)
{
	const u32 link = ppu.cia + 4;
	ppu.cia = (op.aa ? 0 : ppu.cia) + op.bt24;
	if (op.lk) ppu.lr = link;
	return false;
}

bool ppu_interpreter::MCRF(ppu_thread& ppu, ppu_opcode_t op)
{
	CHECK_SIZE(ppu_thread::cr, 32);
	ppu.cr.fields[op.crfd] = ppu.cr.fields[op.crfs];
	return true;
}

bool ppu_interpreter::BCLR(ppu_thread& ppu, ppu_opcode_t op)
{
	const bool bo0 = (op.bo & 0x10) != 0;
	const bool bo1 = (op.bo & 0x08) != 0;
	const bool bo2 = (op.bo & 0x04) != 0;
	const bool bo3 = (op.bo & 0x02) != 0;

	ppu.ctr -= (bo2 ^ true);

	const bool ctr_ok = bo2 | ((ppu.ctr != 0) ^ bo3);
	const bool cond_ok = bo0 | (ppu.cr[op.bi] ^ (bo1 ^ true));

	const u32 target = static_cast<u32>(ppu.lr) & ~3;
	if (op.lk) ppu.lr = ppu.cia + 4;

	if (ctr_ok && cond_ok)
	{
		ppu.cia = target;
		return false;
	}
	else
	{
		return true;
	}
}

bool ppu_interpreter::CRNOR(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.cr[op.crbd] = (ppu.cr[op.crba] | ppu.cr[op.crbb]) ^ true;
	return true;
}

bool ppu_interpreter::CRANDC(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.cr[op.crbd] = ppu.cr[op.crba] & (ppu.cr[op.crbb] ^ true);
	return true;
}

bool ppu_interpreter::ISYNC(ppu_thread& ppu, ppu_opcode_t op)
{
	std::atomic_thread_fence(std::memory_order_acquire);
	return true;
}

bool ppu_interpreter::CRXOR(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.cr[op.crbd] = ppu.cr[op.crba] ^ ppu.cr[op.crbb];
	return true;
}

bool ppu_interpreter::CRNAND(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.cr[op.crbd] = (ppu.cr[op.crba] & ppu.cr[op.crbb]) ^ true;
	return true;
}

bool ppu_interpreter::CRAND(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.cr[op.crbd] = ppu.cr[op.crba] & ppu.cr[op.crbb];
	return true;
}

bool ppu_interpreter::CREQV(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.cr[op.crbd] = (ppu.cr[op.crba] ^ ppu.cr[op.crbb]) ^ true;
	return true;
}

bool ppu_interpreter::CRORC(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.cr[op.crbd] = ppu.cr[op.crba] | (ppu.cr[op.crbb] ^ true);
	return true;
}

bool ppu_interpreter::CROR(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.cr[op.crbd] = ppu.cr[op.crba] | ppu.cr[op.crbb];
	return true;
}

bool ppu_interpreter::BCCTR(ppu_thread& ppu, ppu_opcode_t op)
{
	if (op.lk) ppu.lr = ppu.cia + 4;

	if (op.bo & 0x10 || ppu.cr[op.bi] == ((op.bo & 0x8) != 0))
	{
		ppu.cia = static_cast<u32>(ppu.ctr) & ~3;
		return false;
	}

	return true;
}

bool ppu_interpreter::RLWIMI(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 mask = ppu_rotate_mask(32 + op.mb32, 32 + op.me32);
	ppu.gpr[op.ra] = (ppu.gpr[op.ra] & ~mask) | (dup32(utils::rol32(static_cast<u32>(ppu.gpr[op.rs]), op.sh32)) & mask);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::RLWINM(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = dup32(utils::rol32(static_cast<u32>(ppu.gpr[op.rs]), op.sh32)) & ppu_rotate_mask(32 + op.mb32, 32 + op.me32);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::RLWNM(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = dup32(utils::rol32(static_cast<u32>(ppu.gpr[op.rs]), ppu.gpr[op.rb] & 0x1f)) & ppu_rotate_mask(32 + op.mb32, 32 + op.me32);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::ORI(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = ppu.gpr[op.rs] | op.uimm16;
	return true;
}

bool ppu_interpreter::ORIS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = ppu.gpr[op.rs] | (u64{op.uimm16} << 16);
	return true;
}

bool ppu_interpreter::XORI(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = ppu.gpr[op.rs] ^ op.uimm16;
	return true;
}

bool ppu_interpreter::XORIS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = ppu.gpr[op.rs] ^ (u64{op.uimm16} << 16);
	return true;
}

bool ppu_interpreter::ANDI(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = ppu.gpr[op.rs] & op.uimm16;
	ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::ANDIS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = ppu.gpr[op.rs] & (u64{op.uimm16} << 16);
	ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::RLDICL(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = utils::rol64(ppu.gpr[op.rs], op.sh64) & (~0ull >> op.mbe64);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::RLDICR(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = utils::rol64(ppu.gpr[op.rs], op.sh64) & (~0ull << (op.mbe64 ^ 63));
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::RLDIC(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = utils::rol64(ppu.gpr[op.rs], op.sh64) & ppu_rotate_mask(op.mbe64, op.sh64 ^ 63);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::RLDIMI(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 mask = ppu_rotate_mask(op.mbe64, op.sh64 ^ 63);
	ppu.gpr[op.ra] = (ppu.gpr[op.ra] & ~mask) | (utils::rol64(ppu.gpr[op.rs], op.sh64) & mask);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::RLDCL(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = utils::rol64(ppu.gpr[op.rs], ppu.gpr[op.rb] & 0x3f) & (~0ull >> op.mbe64);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::RLDCR(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = utils::rol64(ppu.gpr[op.rs], ppu.gpr[op.rb] & 0x3f) & (~0ull << (op.mbe64 ^ 63));
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::CMP(ppu_thread& ppu, ppu_opcode_t op)
{
	if (op.l10)
	{
		ppu_cr_set<s64>(ppu, op.crfd, ppu.gpr[op.ra], ppu.gpr[op.rb]);
	}
	else
	{
		ppu_cr_set<s32>(ppu, op.crfd, static_cast<u32>(ppu.gpr[op.ra]), static_cast<u32>(ppu.gpr[op.rb]));
	}
	return true;
}

bool ppu_interpreter::TW(ppu_thread& ppu, ppu_opcode_t op)
{
	s32 a = static_cast<s32>(ppu.gpr[op.ra]);
	s32 b = static_cast<s32>(ppu.gpr[op.rb]);

	if ((a < b && (op.bo & 0x10)) ||
		(a > b && (op.bo & 0x8)) ||
		(a == b && (op.bo & 0x4)) ||
		(static_cast<u32>(a) < static_cast<u32>(b) && (op.bo & 0x2)) ||
		(static_cast<u32>(a) > static_cast<u32>(b) && (op.bo & 0x1)))
	{
		fmt::throw_exception("Trap!" HERE);
	}

	return true;
}

bool ppu_interpreter::LVSL(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.vr[op.vd].vi = sse_altivec_lvsl(addr);
	return true;
}

bool ppu_interpreter::LVEBX(ppu_thread& ppu, ppu_opcode_t op)
{
	return LVX(ppu, op);
}

bool ppu_interpreter::SUBFC(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.gpr[op.ra];
	const u64 RB = ppu.gpr[op.rb];
	const auto r = add64_flags(~RA, RB, 1);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if (op.oe) [[unlikely]] ppu_ov_set(ppu, (~RA >> 63 == RB >> 63) && (~RA >> 63 != ppu.gpr[op.rd] >> 63));
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, r.result, 0);
	return true;
}

bool ppu_interpreter::MULHDU(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.rd] = utils::umulh64(ppu.gpr[op.ra], ppu.gpr[op.rb]);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	return true;
}

bool ppu_interpreter::ADDC(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.gpr[op.ra];
	const u64 RB = ppu.gpr[op.rb];
	const auto r = add64_flags(RA, RB);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if (op.oe) [[unlikely]] ppu_ov_set(ppu, (RA >> 63 == RB >> 63) && (RA >> 63 != ppu.gpr[op.rd] >> 63));
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, r.result, 0);
	return true;
}

bool ppu_interpreter::MULHWU(ppu_thread& ppu, ppu_opcode_t op)
{
	u32 a = static_cast<u32>(ppu.gpr[op.ra]);
	u32 b = static_cast<u32>(ppu.gpr[op.rb]);
	ppu.gpr[op.rd] = (u64{a} * b) >> 32;
	if (op.rc) [[unlikely]] ppu_cr_set(ppu, 0, false, false, false, ppu.xer.so);
	return true;
}

bool ppu_interpreter::MFOCRF(ppu_thread& ppu, ppu_opcode_t op)
{
	if (op.l11)
	{
		// MFOCRF
		const u32 n = utils::cntlz32(op.crm) & 7;
		const u32 p = n * 4;
		const u32 v = ppu.cr[p + 0] << 3 | ppu.cr[p + 1] << 2 | ppu.cr[p + 2] << 1 | ppu.cr[p + 3] << 0;

		ppu.gpr[op.rd] = v << (p ^ 0x1c);
	}
	else
	{
		// MFCR
		auto* lanes = reinterpret_cast<be_t<v128>*>(+ppu.cr.bits);
		const u32 mh = _mm_movemask_epi8(_mm_slli_epi64(lanes[0].value().vi, 7));
		const u32 ml = _mm_movemask_epi8(_mm_slli_epi64(lanes[1].value().vi, 7));

		ppu.gpr[op.rd] = (mh << 16) | ml;
	}

	return true;
}

bool ppu_interpreter::LWARX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_lwarx(ppu, vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LDX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = vm::read64(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LWZX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = vm::read32(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::SLW(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = static_cast<u32>(ppu.gpr[op.rs] << (ppu.gpr[op.rb] & 0x3f));
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::CNTLZW(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = utils::cntlz32(static_cast<u32>(ppu.gpr[op.rs]));
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::SLD(ppu_thread& ppu, ppu_opcode_t op)
{
	const u32 n = ppu.gpr[op.rb] & 0x7f;
	ppu.gpr[op.ra] = n & 0x40 ? 0 : ppu.gpr[op.rs] << n;
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::AND(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = ppu.gpr[op.rs] & ppu.gpr[op.rb];
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::CMPL(ppu_thread& ppu, ppu_opcode_t op)
{
	if (op.l10)
	{
		ppu_cr_set<u64>(ppu, op.crfd, ppu.gpr[op.ra], ppu.gpr[op.rb]);
	}
	else
	{
		ppu_cr_set<u32>(ppu, op.crfd, static_cast<u32>(ppu.gpr[op.ra]), static_cast<u32>(ppu.gpr[op.rb]));
	}
	return true;
}

bool ppu_interpreter::LVSR(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.vr[op.vd].vi = sse_altivec_lvsr(addr);
	return true;
}

bool ppu_interpreter::LVEHX(ppu_thread& ppu, ppu_opcode_t op)
{
	return LVX(ppu, op);
}

bool ppu_interpreter::SUBF(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.gpr[op.ra];
	const u64 RB = ppu.gpr[op.rb];
	ppu.gpr[op.rd] = RB - RA;
	if (op.oe) [[unlikely]] ppu_ov_set(ppu, (~RA >> 63 == RB >> 63) && (~RA >> 63 != ppu.gpr[op.rd] >> 63));
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	return true;
}

bool ppu_interpreter::LDUX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	ppu.gpr[op.rd] = vm::read64(vm::cast(addr, HERE));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::DCBST(ppu_thread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::LWZUX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	ppu.gpr[op.rd] = vm::read32(vm::cast(addr, HERE));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::CNTLZD(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = utils::cntlz64(ppu.gpr[op.rs]);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::ANDC(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = ppu.gpr[op.rs] & ~ppu.gpr[op.rb];
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::TD(ppu_thread& ppu, ppu_opcode_t op)
{
	const s64 a = ppu.gpr[op.ra], b = ppu.gpr[op.rb];
	const u64 a_ = a, b_ = b;

	if (((op.bo & 0x10) && a < b) ||
		((op.bo & 0x8) && a > b) ||
		((op.bo & 0x4) && a == b) ||
		((op.bo & 0x2) && a_ < b_) ||
		((op.bo & 0x1) && a_ > b_))
	{
		fmt::throw_exception("Trap!" HERE);
	}

	return true;
}

bool ppu_interpreter::LVEWX(ppu_thread& ppu, ppu_opcode_t op)
{
	return LVX(ppu, op);
}

bool ppu_interpreter::MULHD(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.rd] = utils::mulh64(ppu.gpr[op.ra], ppu.gpr[op.rb]);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	return true;
}

bool ppu_interpreter::MULHW(ppu_thread& ppu, ppu_opcode_t op)
{
	s32 a = static_cast<s32>(ppu.gpr[op.ra]);
	s32 b = static_cast<s32>(ppu.gpr[op.rb]);
	ppu.gpr[op.rd] = (s64{a} * b) >> 32;
	if (op.rc) [[unlikely]] ppu_cr_set(ppu, 0, false, false, false, ppu.xer.so);
	return true;
}

bool ppu_interpreter::LDARX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_ldarx(ppu, vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::DCBF(ppu_thread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::LBZX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = vm::read8(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LVX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb]) & ~0xfull;
	ppu.vr[op.vd] = vm::_ref<v128>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::NEG(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.gpr[op.ra];
	ppu.gpr[op.rd] = 0 - RA;
	if (op.oe) [[unlikely]] ppu_ov_set(ppu, (~RA >> 63 == 0) && (~RA >> 63 != ppu.gpr[op.rd] >> 63));
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	return true;
}

bool ppu_interpreter::LBZUX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	ppu.gpr[op.rd] = vm::read8(vm::cast(addr, HERE));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::NOR(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = ~(ppu.gpr[op.rs] | ppu.gpr[op.rb]);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::STVEBX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	const u8 eb = addr & 0xf;
	vm::write8(vm::cast(addr, HERE), ppu.vr[op.vs]._u8[15 - eb]);
	return true;
}

bool ppu_interpreter::SUBFE(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.gpr[op.ra];
	const u64 RB = ppu.gpr[op.rb];
	const auto r = add64_flags(~RA, RB, ppu.xer.ca);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if (op.oe) [[unlikely]] ppu_ov_set(ppu, (~RA >> 63 == RB >> 63) && (~RA >> 63 != ppu.gpr[op.rd] >> 63));
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, r.result, 0);
	return true;
}

bool ppu_interpreter::ADDE(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.gpr[op.ra];
	const u64 RB = ppu.gpr[op.rb];
	const auto r = add64_flags(RA, RB, ppu.xer.ca);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if (op.oe) [[unlikely]] ppu_ov_set(ppu, (RA >> 63 == RB >> 63) && (RA >> 63 != ppu.gpr[op.rd] >> 63));
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, r.result, 0);
	return true;
}

bool ppu_interpreter::MTOCRF(ppu_thread& ppu, ppu_opcode_t op)
{
	alignas(4) static const u8 s_table[16][4]
	{
		{0, 0, 0, 0},
		{0, 0, 0, 1},
		{0, 0, 1, 0},
		{0, 0, 1, 1},
		{0, 1, 0, 0},
		{0, 1, 0, 1},
		{0, 1, 1, 0},
		{0, 1, 1, 1},
		{1, 0, 0, 0},
		{1, 0, 0, 1},
		{1, 0, 1, 0},
		{1, 0, 1, 1},
		{1, 1, 0, 0},
		{1, 1, 0, 1},
		{1, 1, 1, 0},
		{1, 1, 1, 1},
	};

	const u64 s = ppu.gpr[op.rs];

	if (op.l11)
	{
		// MTOCRF

		const u32 n = utils::cntlz32(op.crm) & 7;
		const u64 v = (s >> ((n * 4) ^ 0x1c)) & 0xf;
		ppu.cr.fields[n] = *reinterpret_cast<const u32*>(s_table + v);
	}
	else
	{
		// MTCRF

		for (u32 i = 0; i < 8; i++)
		{
			if (op.crm & (128 >> i))
			{
				const u64 v = (s >> ((i * 4) ^ 0x1c)) & 0xf;
				ppu.cr.fields[i] = *reinterpret_cast<const u32*>(s_table + v);
			}
		}
	}
	return true;
}

bool ppu_interpreter::STDX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	vm::write64(vm::cast(addr, HERE), ppu.gpr[op.rs]);
	return true;
}

bool ppu_interpreter::STWCX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu_cr_set(ppu, 0, false, false, ppu_stwcx(ppu, vm::cast(addr, HERE), static_cast<u32>(ppu.gpr[op.rs])), ppu.xer.so);
	return true;
}

bool ppu_interpreter::STWX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	vm::write32(vm::cast(addr, HERE), static_cast<u32>(ppu.gpr[op.rs]));
	return true;
}

bool ppu_interpreter::STVEHX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb]) & ~1ULL;
	const u8 eb = (addr & 0xf) >> 1;
	vm::write16(vm::cast(addr, HERE), ppu.vr[op.vs]._u16[7 - eb]);
	return true;
}

bool ppu_interpreter::STDUX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	vm::write64(vm::cast(addr, HERE), ppu.gpr[op.rs]);
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STWUX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	vm::write32(vm::cast(addr, HERE), static_cast<u32>(ppu.gpr[op.rs]));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STVEWX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb]) & ~3ULL;
	const u8 eb = (addr & 0xf) >> 2;
	vm::write32(vm::cast(addr, HERE), ppu.vr[op.vs]._u32[3 - eb]);
	return true;
}

bool ppu_interpreter::SUBFZE(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.gpr[op.ra];
	const auto r = add64_flags(~RA, 0, ppu.xer.ca);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if (op.oe) [[unlikely]] ppu_ov_set(ppu, (~RA >> 63 == 0) && (~RA >> 63 != ppu.gpr[op.rd] >> 63));
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, r.result, 0);
	return true;
}

bool ppu_interpreter::ADDZE(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.gpr[op.ra];
	const auto r = add64_flags(RA, 0, ppu.xer.ca);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if (op.oe) [[unlikely]] ppu_ov_set(ppu, (RA >> 63 == 0) && (RA >> 63 != ppu.gpr[op.rd] >> 63));
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, r.result, 0);
	return true;
}

bool ppu_interpreter::STDCX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu_cr_set(ppu, 0, false, false, ppu_stdcx(ppu, vm::cast(addr, HERE), ppu.gpr[op.rs]), ppu.xer.so);
	return true;
}

bool ppu_interpreter::STBX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	vm::write8(vm::cast(addr, HERE), static_cast<u8>(ppu.gpr[op.rs]));
	return true;
}

bool ppu_interpreter::STVX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb]) & ~0xfull;
	vm::_ref<v128>(vm::cast(addr, HERE)) = ppu.vr[op.vs];
	return true;
}

bool ppu_interpreter::MULLD(ppu_thread& ppu, ppu_opcode_t op)
{
	const s64 RA = ppu.gpr[op.ra];
	const s64 RB = ppu.gpr[op.rb];
	ppu.gpr[op.rd] = RA * RB;
	if (op.oe) [[unlikely]]
	{
		const s64 high = utils::mulh64(RA, RB);
		ppu_ov_set(ppu, high != s64(ppu.gpr[op.rd]) >> 63);
	}
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	return true;
}

bool ppu_interpreter::SUBFME(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.gpr[op.ra];
	const auto r = add64_flags(~RA, ~0ull, ppu.xer.ca);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if (op.oe) [[unlikely]] ppu_ov_set(ppu, (~RA >> 63 == 1) && (~RA >> 63 != ppu.gpr[op.rd] >> 63));
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, r.result, 0);
	return true;
}

bool ppu_interpreter::ADDME(ppu_thread& ppu, ppu_opcode_t op)
{
	const s64 RA = ppu.gpr[op.ra];
	const auto r = add64_flags(RA, ~0ull, ppu.xer.ca);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if (op.oe) [[unlikely]] ppu_ov_set(ppu, (u64(RA) >> 63 == 1) && (u64(RA) >> 63 != ppu.gpr[op.rd] >> 63));
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, r.result, 0);
	return true;
}

bool ppu_interpreter::MULLW(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.rd] = s64{static_cast<s32>(ppu.gpr[op.ra])} * static_cast<s32>(ppu.gpr[op.rb]);
	if (op.oe) [[unlikely]] ppu_ov_set(ppu, s64(ppu.gpr[op.rd]) < INT32_MIN || s64(ppu.gpr[op.rd]) > INT32_MAX);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::DCBTST(ppu_thread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::STBUX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	vm::write8(vm::cast(addr, HERE), static_cast<u8>(ppu.gpr[op.rs]));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::ADD(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.gpr[op.ra];
	const u64 RB = ppu.gpr[op.rb];
	ppu.gpr[op.rd] = RA + RB;
	if (op.oe) [[unlikely]] ppu_ov_set(ppu, (RA >> 63 == RB >> 63) && (RA >> 63 != ppu.gpr[op.rd] >> 63));
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	return true;
}

bool ppu_interpreter::DCBT(ppu_thread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::LHZX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = vm::read16(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::EQV(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = ~(ppu.gpr[op.rs] ^ ppu.gpr[op.rb]);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::ECIWX(ppu_thread& ppu, ppu_opcode_t op)
{
	fmt::throw_exception("ECIWX" HERE);
}

bool ppu_interpreter::LHZUX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = vm::read16(vm::cast(addr, HERE));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::XOR(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = ppu.gpr[op.rs] ^ ppu.gpr[op.rb];
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::MFSPR(ppu_thread& ppu, ppu_opcode_t op)
{
	const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5);

	switch (n)
	{
	case 0x001: ppu.gpr[op.rd] = u32{ppu.xer.so} << 31 | ppu.xer.ov << 30 | ppu.xer.ca << 29 | ppu.xer.cnt; break;
	case 0x008: ppu.gpr[op.rd] = ppu.lr; break;
	case 0x009: ppu.gpr[op.rd] = ppu.ctr; break;
	case 0x100: ppu.gpr[op.rd] = ppu.vrsave; break;

	case 0x10C: ppu.gpr[op.rd] = get_timebased_time(); break;
	case 0x10D: ppu.gpr[op.rd] = get_timebased_time() >> 32; break;
	default: fmt::throw_exception("MFSPR 0x%x" HERE, n);
	}

	return true;
}

bool ppu_interpreter::LWAX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = static_cast<s32>(vm::read32(vm::cast(addr, HERE)));
	return true;
}

bool ppu_interpreter::DST(ppu_thread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::LHAX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = static_cast<s16>(vm::read16(vm::cast(addr, HERE)));
	return true;
}

bool ppu_interpreter::LVXL(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb]) & ~0xfull;
	ppu.vr[op.vd] = vm::_ref<v128>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::MFTB(ppu_thread& ppu, ppu_opcode_t op)
{
	const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5);

	switch (n)
	{
	case 0x10C: ppu.gpr[op.rd] = get_timebased_time(); break;
	case 0x10D: ppu.gpr[op.rd] = get_timebased_time() >> 32; break;
	default: fmt::throw_exception("MFTB 0x%x" HERE, n);
	}

	return true;
}

bool ppu_interpreter::LWAUX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = static_cast<s32>(vm::read32(vm::cast(addr, HERE)));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::DSTST(ppu_thread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::LHAUX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = static_cast<s16>(vm::read16(vm::cast(addr, HERE)));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STHX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	vm::write16(vm::cast(addr, HERE), static_cast<u16>(ppu.gpr[op.rs]));
	return true;
}

bool ppu_interpreter::ORC(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = ppu.gpr[op.rs] | ~ppu.gpr[op.rb];
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::ECOWX(ppu_thread& ppu, ppu_opcode_t op)
{
	fmt::throw_exception("ECOWX" HERE);
}

bool ppu_interpreter::STHUX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	vm::write16(vm::cast(addr, HERE), static_cast<u16>(ppu.gpr[op.rs]));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::OR(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = ppu.gpr[op.rs] | ppu.gpr[op.rb];
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::DIVDU(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.gpr[op.ra];
	const u64 RB = ppu.gpr[op.rb];
	ppu.gpr[op.rd] = RB == 0 ? 0 : RA / RB;
	if (op.oe) [[unlikely]] ppu_ov_set(ppu, RB == 0);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	return true;
}

bool ppu_interpreter::DIVWU(ppu_thread& ppu, ppu_opcode_t op)
{
	const u32 RA = static_cast<u32>(ppu.gpr[op.ra]);
	const u32 RB = static_cast<u32>(ppu.gpr[op.rb]);
	ppu.gpr[op.rd] = RB == 0 ? 0 : RA / RB;
	if (op.oe) [[unlikely]] ppu_ov_set(ppu, RB == 0);
	if (op.rc) [[unlikely]] ppu_cr_set(ppu, 0, false, false, false, ppu.xer.so);
	return true;
}

bool ppu_interpreter::MTSPR(ppu_thread& ppu, ppu_opcode_t op)
{
	const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5);

	switch (n)
	{
	case 0x001:
	{
		const u64 value = ppu.gpr[op.rs];
		ppu.xer.so = (value & 0x80000000) != 0;
		ppu.xer.ov = (value & 0x40000000) != 0;
		ppu.xer.ca = (value & 0x20000000) != 0;
		ppu.xer.cnt = value & 0x7f;
		break;
	}
	case 0x008: ppu.lr = ppu.gpr[op.rs]; break;
	case 0x009: ppu.ctr = ppu.gpr[op.rs]; break;
	case 0x100: ppu.vrsave = static_cast<u32>(ppu.gpr[op.rs]); break;
	default: fmt::throw_exception("MTSPR 0x%x" HERE, n);
	}

	return true;
}

bool ppu_interpreter::DCBI(ppu_thread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::NAND(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = ~(ppu.gpr[op.rs] & ppu.gpr[op.rb]);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::STVXL(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb]) & ~0xfull;
	vm::_ref<v128>(vm::cast(addr, HERE)) = ppu.vr[op.vs];
	return true;
}

bool ppu_interpreter::DIVD(ppu_thread& ppu, ppu_opcode_t op)
{
	const s64 RA = ppu.gpr[op.ra];
	const s64 RB = ppu.gpr[op.rb];
	const bool o = RB == 0 || (RA == INT64_MIN && RB == -1);
	ppu.gpr[op.rd] = o ? 0 : RA / RB;
	if (op.oe) [[unlikely]] ppu_ov_set(ppu, o);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	return true;
}

bool ppu_interpreter::DIVW(ppu_thread& ppu, ppu_opcode_t op)
{
	const s32 RA = static_cast<s32>(ppu.gpr[op.ra]);
	const s32 RB = static_cast<s32>(ppu.gpr[op.rb]);
	const bool o = RB == 0 || (RA == INT32_MIN && RB == -1);
	ppu.gpr[op.rd] = o ? 0 : static_cast<u32>(RA / RB);
	if (op.oe) [[unlikely]] ppu_ov_set(ppu, o);
	if (op.rc) [[unlikely]] ppu_cr_set(ppu, 0, false, false, false, ppu.xer.so);
	return true;
}

bool ppu_interpreter_precise::LVLX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.vr[op.vd].vi = sse_cellbe_lvlx_v0(addr);
	return true;
}

bool ppu_interpreter_fast::LVLX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.vr[op.vd].vi = sse_cellbe_lvlx(addr);
	return true;
}

bool ppu_interpreter::LDBRX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = vm::_ref<le_t<u64>>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LSWX(ppu_thread& ppu, ppu_opcode_t op)
{
	u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	u32 count = ppu.xer.cnt & 0x7f;
	for (; count >= 4; count -= 4, addr += 4, op.rd = (op.rd + 1) & 31)
	{
		ppu.gpr[op.rd] = vm::_ref<u32>(vm::cast(addr, HERE));
	}
	if (count)
	{
		u32 value = 0;
		for (u32 byte = 0; byte < count; byte++)
		{
			u32 byte_value = vm::_ref<u8>(vm::cast(addr + byte, HERE));
			value |= byte_value << ((3 ^ byte) * 8);
		}
		ppu.gpr[op.rd] = value;
	}
	return true;
}

bool ppu_interpreter::LWBRX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = vm::_ref<le_t<u32>>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LFSX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.fpr[op.frd] = vm::_ref<f32>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::SRW(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = (ppu.gpr[op.rs] & 0xffffffff) >> (ppu.gpr[op.rb] & 0x3f);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::SRD(ppu_thread& ppu, ppu_opcode_t op)
{
	const u32 n = ppu.gpr[op.rb] & 0x7f;
	ppu.gpr[op.ra] = n & 0x40 ? 0 : ppu.gpr[op.rs] >> n;
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter_precise::LVRX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.vr[op.vd].vi = sse_cellbe_lvrx_v0(addr);
	return true;
}

bool ppu_interpreter_fast::LVRX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.vr[op.vd].vi = sse_cellbe_lvrx(addr);
	return true;
}

bool ppu_interpreter::LSWI(ppu_thread& ppu, ppu_opcode_t op)
{
	u64 addr = op.ra ? ppu.gpr[op.ra] : 0;
	u64 N = op.rb ? op.rb : 32;
	u8 reg = op.rd;

	while (N > 0)
	{
		if (N > 3)
		{
			ppu.gpr[reg] = vm::read32(vm::cast(addr, HERE));
			addr += 4;
			N -= 4;
		}
		else
		{
			u32 buf = 0;
			u32 i = 3;
			while (N > 0)
			{
				N = N - 1;
				buf |= vm::read8(vm::cast(addr, HERE)) << (i * 8);
				addr++;
				i--;
			}
			ppu.gpr[reg] = buf;
		}
		reg = (reg + 1) % 32;
	}
	return true;
}

bool ppu_interpreter::LFSUX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	ppu.fpr[op.frd] = vm::_ref<f32>(vm::cast(addr, HERE));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::SYNC(ppu_thread& ppu, ppu_opcode_t op)
{
	std::atomic_thread_fence(std::memory_order_seq_cst);
	return true;
}

bool ppu_interpreter::LFDX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.fpr[op.frd] = vm::_ref<f64>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LFDUX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	ppu.fpr[op.frd] = vm::_ref<f64>(vm::cast(addr, HERE));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter_precise::STVLX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	sse_cellbe_stvlx_v0(addr, ppu.vr[op.vs].vi);
	return true;
}

bool ppu_interpreter_fast::STVLX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	sse_cellbe_stvlx(addr, ppu.vr[op.vs].vi);
	return true;
}

bool ppu_interpreter::STDBRX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	vm::_ref<le_t<u64>>(vm::cast(addr, HERE)) = ppu.gpr[op.rs];
	return true;
}

bool ppu_interpreter::STSWX(ppu_thread& ppu, ppu_opcode_t op)
{
	u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	u32 count = ppu.xer.cnt & 0x7F;
	for (; count >= 4; count -= 4, addr += 4, op.rs = (op.rs + 1) & 31)
	{
		vm::write32(vm::cast(addr, HERE), static_cast<u32>(ppu.gpr[op.rs]));
	}
	if (count)
	{
		u32 value = static_cast<u32>(ppu.gpr[op.rs]);
		for (u32 byte = 0; byte < count; byte++)
		{
			u8 byte_value = static_cast<u8>(value >> ((3 ^ byte) * 8));
			vm::write8(vm::cast(addr + byte, HERE), byte_value);
		}
	}
	return true;
}

bool ppu_interpreter::STWBRX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	vm::_ref<le_t<u32>>(vm::cast(addr, HERE)) = static_cast<u32>(ppu.gpr[op.rs]);
	return true;
}

bool ppu_interpreter::STFSX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	vm::_ref<f32>(vm::cast(addr, HERE)) = static_cast<float>(ppu.fpr[op.frs]);
	return true;
}

bool ppu_interpreter_precise::STVRX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	sse_cellbe_stvrx_v0(addr, ppu.vr[op.vs].vi);
	return true;
}

bool ppu_interpreter_fast::STVRX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	sse_cellbe_stvrx(addr, ppu.vr[op.vs].vi);
	return true;
}

bool ppu_interpreter::STFSUX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	vm::_ref<f32>(vm::cast(addr, HERE)) = static_cast<float>(ppu.fpr[op.frs]);
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STSWI(ppu_thread& ppu, ppu_opcode_t op)
{
	u64 addr = op.ra ? ppu.gpr[op.ra] : 0;
	u64 N = op.rb ? op.rb : 32;
	u8 reg = op.rd;

	while (N > 0)
	{
		if (N > 3)
		{
			vm::write32(vm::cast(addr, HERE), static_cast<u32>(ppu.gpr[reg]));
			addr += 4;
			N -= 4;
		}
		else
		{
			u32 buf = static_cast<u32>(ppu.gpr[reg]);
			while (N > 0)
			{
				N = N - 1;
				vm::write8(vm::cast(addr, HERE), (0xFF000000 & buf) >> 24);
				buf <<= 8;
				addr++;
			}
		}
		reg = (reg + 1) % 32;
	}
	return true;
}

bool ppu_interpreter::STFDX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	vm::_ref<f64>(vm::cast(addr, HERE)) = ppu.fpr[op.frs];
	return true;
}

bool ppu_interpreter::STFDUX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	vm::_ref<f64>(vm::cast(addr, HERE)) = ppu.fpr[op.frs];
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter_precise::LVLXL(ppu_thread& ppu, ppu_opcode_t op)
{
	return LVLX(ppu, op);
}

bool ppu_interpreter_fast::LVLXL(ppu_thread& ppu, ppu_opcode_t op)
{
	return LVLX(ppu, op);
}

bool ppu_interpreter::LHBRX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = vm::_ref<le_t<u16>>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::SRAW(ppu_thread& ppu, ppu_opcode_t op)
{
	s32 RS = static_cast<s32>(ppu.gpr[op.rs]);
	u8 shift = ppu.gpr[op.rb] & 63;
	if (shift > 31)
	{
		ppu.gpr[op.ra] = 0 - (RS < 0);
		ppu.xer.ca = (RS < 0);
	}
	else
	{
		ppu.gpr[op.ra] = RS >> shift;
		ppu.xer.ca = (RS < 0) && ((ppu.gpr[op.ra] << shift) != RS);
	}

	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::SRAD(ppu_thread& ppu, ppu_opcode_t op)
{
	s64 RS = ppu.gpr[op.rs];
	u8 shift = ppu.gpr[op.rb] & 127;
	if (shift > 63)
	{
		ppu.gpr[op.ra] = 0 - (RS < 0);
		ppu.xer.ca = (RS < 0);
	}
	else
	{
		ppu.gpr[op.ra] = RS >> shift;
		ppu.xer.ca = (RS < 0) && ((ppu.gpr[op.ra] << shift) != RS);
	}

	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter_precise::LVRXL(ppu_thread& ppu, ppu_opcode_t op)
{
	return LVRX(ppu, op);
}

bool ppu_interpreter_fast::LVRXL(ppu_thread& ppu, ppu_opcode_t op)
{
	return LVRX(ppu, op);
}

bool ppu_interpreter::DSS(ppu_thread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::SRAWI(ppu_thread& ppu, ppu_opcode_t op)
{
	s32 RS = static_cast<u32>(ppu.gpr[op.rs]);
	ppu.gpr[op.ra] = RS >> op.sh32;
	ppu.xer.ca = (RS < 0) && (static_cast<u32>(ppu.gpr[op.ra] << op.sh32) != RS);

	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::SRADI(ppu_thread& ppu, ppu_opcode_t op)
{
	auto sh = op.sh64;
	s64 RS = ppu.gpr[op.rs];
	ppu.gpr[op.ra] = RS >> sh;
	ppu.xer.ca = (RS < 0) && ((ppu.gpr[op.ra] << sh) != RS);

	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::EIEIO(ppu_thread& ppu, ppu_opcode_t op)
{
	std::atomic_thread_fence(std::memory_order_seq_cst);
	return true;
}

bool ppu_interpreter_precise::STVLXL(ppu_thread& ppu, ppu_opcode_t op)
{
	return STVLX(ppu, op);
}

bool ppu_interpreter_fast::STVLXL(ppu_thread& ppu, ppu_opcode_t op)
{
	return STVLX(ppu, op);
}

bool ppu_interpreter::STHBRX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	vm::_ref<le_t<u16>>(vm::cast(addr, HERE)) = static_cast<u16>(ppu.gpr[op.rs]);
	return true;
}

bool ppu_interpreter::EXTSH(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = static_cast<s16>(ppu.gpr[op.rs]);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter_precise::STVRXL(ppu_thread& ppu, ppu_opcode_t op)
{
	return STVRX(ppu, op);
}

bool ppu_interpreter_fast::STVRXL(ppu_thread& ppu, ppu_opcode_t op)
{
	return STVRX(ppu, op);
}

bool ppu_interpreter::EXTSB(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = static_cast<s8>(ppu.gpr[op.rs]);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::STFIWX(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	vm::write32(vm::cast(addr, HERE), std::bit_cast<u64>(ppu.fpr[op.frs]));
	return true;
}

bool ppu_interpreter::EXTSW(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.gpr[op.ra] = static_cast<s32>(ppu.gpr[op.rs]);
	if (op.rc) [[unlikely]] ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	return true;
}

bool ppu_interpreter::ICBI(ppu_thread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::DCBZ(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];

	std::memset(vm::base(vm::cast(addr, HERE) & ~127), 0, 128);
	return true;
}

bool ppu_interpreter::LWZ(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	ppu.gpr[op.rd] = vm::read32(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LWZU(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	ppu.gpr[op.rd] = vm::read32(vm::cast(addr, HERE));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::LBZ(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	ppu.gpr[op.rd] = vm::read8(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LBZU(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	ppu.gpr[op.rd] = vm::read8(vm::cast(addr, HERE));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STW(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	const u32 value = static_cast<u32>(ppu.gpr[op.rs]);
	vm::write32(vm::cast(addr, HERE), value);

	//Insomniac engine v3 & v4 (newer R&C, Fuse, Resitance 3)
	if (value == 0xAAAAAAAA) [[unlikely]]
	{
		vm::reservation_update(addr, 128);
	}

	return true;
}

bool ppu_interpreter::STWU(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	vm::write32(vm::cast(addr, HERE), static_cast<u32>(ppu.gpr[op.rs]));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STB(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	vm::write8(vm::cast(addr, HERE), static_cast<u8>(ppu.gpr[op.rs]));
	return true;
}

bool ppu_interpreter::STBU(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	vm::write8(vm::cast(addr, HERE), static_cast<u8>(ppu.gpr[op.rs]));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::LHZ(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	ppu.gpr[op.rd] = vm::read16(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LHZU(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	ppu.gpr[op.rd] = vm::read16(vm::cast(addr, HERE));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::LHA(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	ppu.gpr[op.rd] = static_cast<s16>(vm::read16(vm::cast(addr, HERE)));
	return true;
}

bool ppu_interpreter::LHAU(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	ppu.gpr[op.rd] = static_cast<s16>(vm::read16(vm::cast(addr, HERE)));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STH(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	vm::write16(vm::cast(addr, HERE), static_cast<u16>(ppu.gpr[op.rs]));
	return true;
}

bool ppu_interpreter::STHU(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	vm::write16(vm::cast(addr, HERE), static_cast<u16>(ppu.gpr[op.rs]));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::LMW(ppu_thread& ppu, ppu_opcode_t op)
{
	u64 addr = op.ra ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	for (u32 i = op.rd; i<32; ++i, addr += 4)
	{
		ppu.gpr[i] = vm::read32(vm::cast(addr, HERE));
	}
	return true;
}

bool ppu_interpreter::STMW(ppu_thread& ppu, ppu_opcode_t op)
{
	u64 addr = op.ra ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	for (u32 i = op.rs; i<32; ++i, addr += 4)
	{
		vm::write32(vm::cast(addr, HERE), static_cast<u32>(ppu.gpr[i]));
	}
	return true;
}

bool ppu_interpreter::LFS(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	ppu.fpr[op.frd] = vm::_ref<f32>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LFSU(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	ppu.fpr[op.frd] = vm::_ref<f32>(vm::cast(addr, HERE));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::LFD(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	ppu.fpr[op.frd] = vm::_ref<f64>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LFDU(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	ppu.fpr[op.frd] = vm::_ref<f64>(vm::cast(addr, HERE));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STFS(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	vm::_ref<f32>(vm::cast(addr, HERE)) = static_cast<float>(ppu.fpr[op.frs]);
	return true;
}

bool ppu_interpreter::STFSU(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	vm::_ref<f32>(vm::cast(addr, HERE)) = static_cast<float>(ppu.fpr[op.frs]);
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STFD(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	vm::_ref<f64>(vm::cast(addr, HERE)) = ppu.fpr[op.frs];
	return true;
}

bool ppu_interpreter::STFDU(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	vm::_ref<f64>(vm::cast(addr, HERE)) = ppu.fpr[op.frs];
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::LD(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.simm16 & ~3) + (op.ra ? ppu.gpr[op.ra] : 0);
	ppu.gpr[op.rd] = vm::read64(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LDU(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + (op.simm16 & ~3);
	ppu.gpr[op.rd] = vm::read64(vm::cast(addr, HERE));
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter::LWA(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.simm16 & ~3) + (op.ra ? ppu.gpr[op.ra] : 0);
	ppu.gpr[op.rd] = static_cast<s32>(vm::read32(vm::cast(addr, HERE)));
	return true;
}

bool ppu_interpreter::STD(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.simm16 & ~3) + (op.ra ? ppu.gpr[op.ra] : 0);
	vm::write64(vm::cast(addr, HERE), ppu.gpr[op.rs]);
	return true;
}

bool ppu_interpreter::STDU(ppu_thread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.gpr[op.ra] + (op.simm16 & ~3);
	vm::write64(vm::cast(addr, HERE), ppu.gpr[op.rs]);
	ppu.gpr[op.ra] = addr;
	return true;
}

bool ppu_interpreter_fast::FDIVS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] / ppu.fpr[op.frb]);
	return true;
}

bool ppu_interpreter_precise::FDIVS(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] / ppu.fpr[op.frb]);
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FSUBS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] - ppu.fpr[op.frb]);
	return true;
}

bool ppu_interpreter_precise::FSUBS(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] - ppu.fpr[op.frb]);
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FADDS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] + ppu.fpr[op.frb]);
	return true;
}

bool ppu_interpreter_precise::FADDS(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] + ppu.fpr[op.frb]);
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FSQRTS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = f32(std::sqrt(ppu.fpr[op.frb]));
	return true;
}

bool ppu_interpreter_precise::FSQRTS(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = f32(std::sqrt(ppu.fpr[op.frb]));
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FRES(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = f32(1.0 / ppu.fpr[op.frb]);
	return true;
}

bool ppu_interpreter_precise::FRES(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = f32(1.0 / ppu.fpr[op.frb]);
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FMULS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] * ppu.fpr[op.frc]);
	return true;
}

bool ppu_interpreter_precise::FMULS(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] * ppu.fpr[op.frc]);
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FMADDS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] * ppu.fpr[op.frc] + ppu.fpr[op.frb]);
	return true;
}

bool ppu_interpreter_precise::FMADDS(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] * ppu.fpr[op.frc] + ppu.fpr[op.frb]);
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FMSUBS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] * ppu.fpr[op.frc] - ppu.fpr[op.frb]);
	return true;
}

bool ppu_interpreter_precise::FMSUBS(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] * ppu.fpr[op.frc] - ppu.fpr[op.frb]);
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FNMSUBS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = f32(-(ppu.fpr[op.fra] * ppu.fpr[op.frc] - ppu.fpr[op.frb]));
	return true;
}

bool ppu_interpreter_precise::FNMSUBS(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = f32(-(ppu.fpr[op.fra] * ppu.fpr[op.frc] - ppu.fpr[op.frb]));
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FNMADDS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = f32(-(ppu.fpr[op.fra] * ppu.fpr[op.frc] + ppu.fpr[op.frb]));
	return true;
}

bool ppu_interpreter_precise::FNMADDS(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = f32(-(ppu.fpr[op.fra] * ppu.fpr[op.frc] + ppu.fpr[op.frb]));
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter::MTFSB1(ppu_thread& ppu, ppu_opcode_t op)
{
	const u32 bit = op.crbd;
	if (bit < 16 || bit > 19) ppu_log.warning("MTFSB1(%d)", bit);
	ppu.fpscr.bits[bit] = 1;
	if (op.rc) [[unlikely]] ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	return true;
}

bool ppu_interpreter::MCRFS(ppu_thread& ppu, ppu_opcode_t op)
{
	if (op.crfs != 4) ppu_log.warning("MCRFS(%d)", op.crfs);
	ppu.cr.fields[op.crfd] = ppu.fpscr.fields[op.crfs];
	return true;
}

bool ppu_interpreter::MTFSB0(ppu_thread& ppu, ppu_opcode_t op)
{
	const u32 bit = op.crbd;
	if (bit < 16 || bit > 19) ppu_log.warning("MTFSB0(%d)", bit);
	ppu.fpscr.bits[bit] = 0;
	if (op.rc) [[unlikely]] ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	return true;
}

bool ppu_interpreter::MTFSFI(ppu_thread& ppu, ppu_opcode_t op)
{
	const u32 bf = op.crfd;

	if (bf != 4)
	{
		// Do nothing on non-FPCC field (TODO)
		ppu_log.warning("MTFSFI(%d)", op.crfd);
	}
	else
	{
		static constexpr std::array<u32, 16> all_values = []() -> std::array<u32, 16>
		{
			std::array<u32, 16> values{};

			for (u32 i = 0; i < values.size(); i++)
			{
				u32 value = 0, im = i;
				value |= (im & 1) << (8 * 3); im >>= 1;
				value |= (im & 1) << (8 * 2); im >>= 1;
				value |= (im & 1) << (8 * 1); im >>= 1;
				value |= (im & 1) << (8 * 0);
				values[i] = value;
			}

			return values;
		}();

		ppu.fpscr.fields[bf] = all_values[op.i];
	}

	if (op.rc) [[unlikely]] ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	return true;
}

bool ppu_interpreter::MFFS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu_log.warning("MFFS");
	ppu.fpr[op.frd] = std::bit_cast<f64>(u64{ppu.fpscr.fl} << 15 | u64{ppu.fpscr.fg} << 14 | u64{ppu.fpscr.fe} << 13 | u64{ppu.fpscr.fu} << 12);
	if (op.rc) [[unlikely]] ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	return true;
}

bool ppu_interpreter::MTFSF(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu_log.warning("MTFSF");
	if (op.rc) [[unlikely]] ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	return true;
}

bool ppu_interpreter::FCMPU(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 a = ppu.fpr[op.fra];
	const f64 b = ppu.fpr[op.frb];
	ppu_fpcc_set(ppu, a, b, true, op.crfd);
	return true;
}

bool ppu_interpreter::FCTIW(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto b = _mm_load_sd(&ppu.fpr[op.frb]);
	const auto res = _mm_xor_si128(_mm_cvtpd_epi32(b), _mm_castpd_si128(_mm_cmpge_pd(b, _mm_set1_pd(0x80000000))));
	ppu.fpr[op.frd] = std::bit_cast<f64, s64>(_mm_cvtsi128_si32(res));
	if (op.rc) [[unlikely]] fmt::throw_exception("%s: op.rc", __func__); //ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	return true;
}

bool ppu_interpreter::FCTIWZ(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto b = _mm_load_sd(&ppu.fpr[op.frb]);
	const auto res = _mm_xor_si128(_mm_cvttpd_epi32(b), _mm_castpd_si128(_mm_cmpge_pd(b, _mm_set1_pd(0x80000000))));
	ppu.fpr[op.frd] = std::bit_cast<f64, s64>(_mm_cvtsi128_si32(res));
	if (op.rc) [[unlikely]] fmt::throw_exception("%s: op.rc", __func__); //ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	return true;
}

bool ppu_interpreter_fast::FRSP(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = f32(ppu.fpr[op.frb]);
	return true;
}

bool ppu_interpreter_precise::FRSP(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = f32(ppu.fpr[op.frb]);
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FDIV(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = ppu.fpr[op.fra] / ppu.fpr[op.frb];
	return true;
}

bool ppu_interpreter_precise::FDIV(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = ppu.fpr[op.fra] / ppu.fpr[op.frb];
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FSUB(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = ppu.fpr[op.fra] - ppu.fpr[op.frb];
	return true;
}

bool ppu_interpreter_precise::FSUB(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = ppu.fpr[op.fra] - ppu.fpr[op.frb];
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FADD(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = ppu.fpr[op.fra] + ppu.fpr[op.frb];
	return true;
}

bool ppu_interpreter_precise::FADD(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = ppu.fpr[op.fra] + ppu.fpr[op.frb];
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FSQRT(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = std::sqrt(ppu.fpr[op.frb]);
	return true;
}

bool ppu_interpreter_precise::FSQRT(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = std::sqrt(ppu.fpr[op.frb]);
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter::FSEL(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = ppu.fpr[op.fra] >= 0.0 ? ppu.fpr[op.frc] : ppu.fpr[op.frb];
	if (op.rc) [[unlikely]] ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	return true;
}

bool ppu_interpreter_fast::FMUL(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = ppu.fpr[op.fra] * ppu.fpr[op.frc];
	return true;
}

bool ppu_interpreter_precise::FMUL(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = ppu.fpr[op.fra] * ppu.fpr[op.frc];
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FRSQRTE(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = 1.0 / std::sqrt(ppu.fpr[op.frb]);
	return true;
}

bool ppu_interpreter_precise::FRSQRTE(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = 1.0 / std::sqrt(ppu.fpr[op.frb]);
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FMSUB(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = ppu.fpr[op.fra] * ppu.fpr[op.frc] - ppu.fpr[op.frb];
	return true;
}

bool ppu_interpreter_precise::FMSUB(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = std::fma(ppu.fpr[op.fra], ppu.fpr[op.frc], -ppu.fpr[op.frb]);
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FMADD(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = ppu.fpr[op.fra] * ppu.fpr[op.frc] + ppu.fpr[op.frb];
	return true;
}

bool ppu_interpreter_precise::FMADD(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = std::fma(ppu.fpr[op.fra], ppu.fpr[op.frc], ppu.fpr[op.frb]);
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FNMSUB(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = -(ppu.fpr[op.fra] * ppu.fpr[op.frc] - ppu.fpr[op.frb]);
	return true;
}

bool ppu_interpreter_precise::FNMSUB(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = -std::fma(ppu.fpr[op.fra], ppu.fpr[op.frc], -ppu.fpr[op.frb]);
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter_fast::FNMADD(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = -(ppu.fpr[op.fra] * ppu.fpr[op.frc] + ppu.fpr[op.frb]);
	return true;
}

bool ppu_interpreter_precise::FNMADD(ppu_thread& ppu, ppu_opcode_t op)
{
	const f64 res = ppu.fpr[op.frd] = -std::fma(ppu.fpr[op.fra], ppu.fpr[op.frc], ppu.fpr[op.frb]);
	ppu_fpcc_set(ppu, res, 0., op.rc);
	return true;
}

bool ppu_interpreter::FCMPO(ppu_thread& ppu, ppu_opcode_t op)
{
	return FCMPU(ppu, op);
}

bool ppu_interpreter::FNEG(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = -ppu.fpr[op.frb];
	if (op.rc) [[unlikely]] ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	return true;
}

bool ppu_interpreter::FMR(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = ppu.fpr[op.frb];
	if (op.rc) [[unlikely]] ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	return true;
}

bool ppu_interpreter::FNABS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = -std::fabs(ppu.fpr[op.frb]);
	if (op.rc) [[unlikely]] ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	return true;
}

bool ppu_interpreter::FABS(ppu_thread& ppu, ppu_opcode_t op)
{
	ppu.fpr[op.frd] = std::fabs(ppu.fpr[op.frb]);
	if (op.rc) [[unlikely]] ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	return true;
}

bool ppu_interpreter::FCTID(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto b = _mm_load_sd(&ppu.fpr[op.frb]);
	const auto res = _mm_xor_si128(_mm_set1_epi64x(_mm_cvtsd_si64(b)), _mm_castpd_si128(_mm_cmpge_pd(b, _mm_set1_pd(f64(1ull << 63)))));
	ppu.fpr[op.frd] = std::bit_cast<f64>(_mm_cvtsi128_si64(res));
	if (op.rc) [[unlikely]] fmt::throw_exception("%s: op.rc", __func__); //ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	return true;
}

bool ppu_interpreter::FCTIDZ(ppu_thread& ppu, ppu_opcode_t op)
{
	const auto b = _mm_load_sd(&ppu.fpr[op.frb]);
	const auto res = _mm_xor_si128(_mm_set1_epi64x(_mm_cvttsd_si64(b)), _mm_castpd_si128(_mm_cmpge_pd(b, _mm_set1_pd(f64(1ull << 63)))));
	ppu.fpr[op.frd] = std::bit_cast<f64>(_mm_cvtsi128_si64(res));
	if (op.rc) [[unlikely]] fmt::throw_exception("%s: op.rc", __func__); //ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	return true;
}

bool ppu_interpreter::FCFID(ppu_thread& ppu, ppu_opcode_t op)
{
	_mm_store_sd(&ppu.fpr[op.frd], _mm_cvtsi64_sd(_mm_setzero_pd(), std::bit_cast<s64>(ppu.fpr[op.frb])));
	if (op.rc) [[unlikely]] fmt::throw_exception("%s: op.rc", __func__); //ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	return true;
}

bool ppu_interpreter::UNK(ppu_thread& ppu, ppu_opcode_t op)
{
	fmt::throw_exception("Unknown/Illegal opcode: 0x%08x at 0x%x" HERE, op.opcode, ppu.cia);
}
