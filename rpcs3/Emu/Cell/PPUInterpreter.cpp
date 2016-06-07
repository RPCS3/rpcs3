#include "stdafx.h"
#include "Emu/System.h"
#include "PPUThread.h"
#include "PPUInterpreter.h"

#include <cmath>

// TODO: fix rol8 and rol16 for __GNUG__ (probably with __asm__)
inline u8 rol8(const u8 x, const u8 n) { return x << n | x >> (8 - n); }
inline u16 rol16(const u16 x, const u16 n) { return x << n | x >> (16 - n); }
inline u32 rol32(const u32 x, const u32 n) { return x << n | x >> (32 - n); }
inline u64 rol64(const u64 x, const u64 n) { return x << n | x >> (64 - n); }
inline u64 dup32(const u32 x) { return x | static_cast<u64>(x) << 32; }

#if defined(__GNUG__)
inline u64 UMULH64(u64 a, u64 b)
{
	u64 result;
	__asm__("mulq %[b]" : "=d" (result) : [a] "a" (a), [b] "rm" (b));
	return result;
}

inline s64 MULH64(s64 a, s64 b)
{
	s64 result;
	__asm__("imulq %[b]" : "=d" (result) : [a] "a" (a), [b] "rm" (b));
	return result;
}
#endif

#if defined(_MSC_VER)
#define UMULH64 __umulh
#define MULH64 __mulh
#endif

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

extern __m128i sse_altivec_vperm(__m128i A, __m128i B, __m128i C)
{
	const auto index = _mm_andnot_si128(C, _mm_set1_epi8(0x1f));
	const auto mask = _mm_cmpgt_epi8(index, _mm_set1_epi8(0xf));
	const auto sa = _mm_shuffle_epi8(A, index);
	const auto sb = _mm_shuffle_epi8(B, index);
	return _mm_or_si128(_mm_and_si128(mask, sa), _mm_andnot_si128(mask, sb));
}

extern __m128i sse_altivec_lvsl(u64 addr)
{
	alignas(16) static const u64 lvsl_values[0x10][2] =
	{
		{ 0x08090A0B0C0D0E0F, 0x0001020304050607 },
		{ 0x090A0B0C0D0E0F10, 0x0102030405060708 },
		{ 0x0A0B0C0D0E0F1011, 0x0203040506070809 },
		{ 0x0B0C0D0E0F101112, 0x030405060708090A },
		{ 0x0C0D0E0F10111213, 0x0405060708090A0B },
		{ 0x0D0E0F1011121314, 0x05060708090A0B0C },
		{ 0x0E0F101112131415, 0x060708090A0B0C0D },
		{ 0x0F10111213141516, 0x0708090A0B0C0D0E },
		{ 0x1011121314151617, 0x08090A0B0C0D0E0F },
		{ 0x1112131415161718, 0x090A0B0C0D0E0F10 },
		{ 0x1213141516171819, 0x0A0B0C0D0E0F1011 },
		{ 0x131415161718191A, 0x0B0C0D0E0F101112 },
		{ 0x1415161718191A1B, 0x0C0D0E0F10111213 },
		{ 0x15161718191A1B1C, 0x0D0E0F1011121314 },
		{ 0x161718191A1B1C1D, 0x0E0F101112131415 },
		{ 0x1718191A1B1C1D1E, 0x0F10111213141516 },
	};

	return _mm_load_si128((__m128i*)lvsl_values[addr & 0xf]);
}

extern __m128i sse_altivec_lvsr(u64 addr)
{
	alignas(16) static const u64 lvsr_values[0x10][2] =
	{
		{ 0x18191A1B1C1D1E1F, 0x1011121314151617 },
		{ 0x1718191A1B1C1D1E, 0x0F10111213141516 },
		{ 0x161718191A1B1C1D, 0x0E0F101112131415 },
		{ 0x15161718191A1B1C, 0x0D0E0F1011121314 },
		{ 0x1415161718191A1B, 0x0C0D0E0F10111213 },
		{ 0x131415161718191A, 0x0B0C0D0E0F101112 },
		{ 0x1213141516171819, 0x0A0B0C0D0E0F1011 },
		{ 0x1112131415161718, 0x090A0B0C0D0E0F10 },
		{ 0x1011121314151617, 0x08090A0B0C0D0E0F },
		{ 0x0F10111213141516, 0x0708090A0B0C0D0E },
		{ 0x0E0F101112131415, 0x060708090A0B0C0D },
		{ 0x0D0E0F1011121314, 0x05060708090A0B0C },
		{ 0x0C0D0E0F10111213, 0x0405060708090A0B },
		{ 0x0B0C0D0E0F101112, 0x030405060708090A },
		{ 0x0A0B0C0D0E0F1011, 0x0203040506070809 },
		{ 0x090A0B0C0D0E0F10, 0x0102030405060708 },
	};

	return _mm_load_si128((__m128i*)lvsr_values[addr & 0xf]);
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

extern __m128i sse_cellbe_lvlx(u64 addr)
{
	return _mm_shuffle_epi8(_mm_load_si128((__m128i*)vm::base(addr & ~0xf)), lvlx_masks[addr & 0xf]);
}

extern void sse_cellbe_stvlx(u64 addr, __m128i a)
{
	_mm_maskmoveu_si128(_mm_shuffle_epi8(a, lvlx_masks[addr & 0xf]), lvrx_masks[addr & 0xf], (char*)vm::base(addr & ~0xf));
}

extern __m128i sse_cellbe_lvrx(u64 addr)
{
	return _mm_shuffle_epi8(_mm_load_si128((__m128i*)vm::base(addr & ~0xf)), lvrx_masks[addr & 0xf]);
}

extern void sse_cellbe_stvrx(u64 addr, __m128i a)
{
	_mm_maskmoveu_si128(_mm_shuffle_epi8(a, lvrx_masks[addr & 0xf]), lvlx_masks[addr & 0xf], (char*)vm::base(addr & ~0xf));
}

template<typename T>
struct add_flags_result_t
{
	T result;
	bool carry;
	bool zero;
	bool sign;

	add_flags_result_t() = default;

	// Straighforward ADD with flags
	add_flags_result_t(T a, T b)
		: result(a + b)
		, carry(result < a)
		, zero(result == 0)
		, sign(result >> (sizeof(T) * 8 - 1) != 0)
	{
	}

	// Straighforward ADC with flags
	add_flags_result_t(T a, T b, bool c)
		: add_flags_result_t(a, b)
	{
		add_flags_result_t r(result, c);
		result = r.result;
		carry |= r.carry;
		zero = r.zero;
		sign = r.sign;
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
extern void ppu_execute_syscall(PPUThread& ppu, u64 code);
extern void ppu_execute_function(PPUThread& ppu, u32 index);

namespace vm { using namespace ps3; }

class ppu_scale_table_t
{
	std::array<__m128, 32 + 31> m_data;

public:
	ppu_scale_table_t()
	{
		for (s32 i = -31; i < 32; i++)
		{
			m_data[i + 31] = _mm_set1_ps(static_cast<float>(std::exp2(i)));
		}
	}

	force_inline __m128 operator [] (s32 scale) const
	{
		return m_data[scale + 31];
	}
}
const g_ppu_scale_table;

bool ppu_interpreter::MFVSCR(PPUThread& ppu, ppu_opcode_t op)
{
	throw std::runtime_error("MFVSCR" HERE);
}

bool ppu_interpreter::MTVSCR(PPUThread& ppu, ppu_opcode_t op)
{
	LOG_WARNING(PPU, "MTVSCR");
	return true;
}

bool ppu_interpreter::VADDCUW(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va].vi;
	const auto b = ppu.VR[op.vb].vi;
	ppu.VR[op.vd].vi = _mm_srli_epi32(_mm_cmpgt_epi32(_mm_xor_si128(b, _mm_set1_epi32(0x80000000)), _mm_xor_si128(a, _mm_set1_epi32(0x7fffffff))), 31);
	return true;
}

bool ppu_interpreter::VADDFP(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd] = v128::addfs(ppu.VR[op.va], ppu.VR[op.vb]);
	return true;
}

bool ppu_interpreter::VADDSBS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_adds_epi8(ppu.VR[op.va].vi, ppu.VR[op.vb].vi);
	return true;
}

bool ppu_interpreter::VADDSHS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_adds_epi16(ppu.VR[op.va].vi, ppu.VR[op.vb].vi);
	return true;
}

bool ppu_interpreter::VADDSWS(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va];
	const auto b = ppu.VR[op.vb];
	const auto s = v128::add32(a, b); // a + b
	const auto m = (a ^ s) & (b ^ s); // overflow bit
	const auto x = _mm_srai_epi32(m.vi, 31); // saturation mask
	const auto y = _mm_srai_epi32(_mm_and_si128(s.vi, m.vi), 31); // positive saturation mask
	ppu.VR[op.vd].vi = _mm_xor_si128(_mm_xor_si128(_mm_srli_epi32(x, 1), y), _mm_or_si128(s.vi, x));
	return true;
}

bool ppu_interpreter::VADDUBM(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd] = v128::add8(ppu.VR[op.va], ppu.VR[op.vb]);
	return true;
}

bool ppu_interpreter::VADDUBS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_adds_epu8(ppu.VR[op.va].vi, ppu.VR[op.vb].vi);
	return true;
}

bool ppu_interpreter::VADDUHM(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd] = v128::add16(ppu.VR[op.va], ppu.VR[op.vb]);
	return true;
}

bool ppu_interpreter::VADDUHS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_adds_epu16(ppu.VR[op.va].vi, ppu.VR[op.vb].vi);
	return true;
}

bool ppu_interpreter::VADDUWM(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd] = v128::add32(ppu.VR[op.va], ppu.VR[op.vb]);
	return true;
}

bool ppu_interpreter::VADDUWS(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va].vi;
	const auto b = ppu.VR[op.vb].vi;
	ppu.VR[op.vd].vi = _mm_or_si128(_mm_add_epi32(a, b), _mm_cmpgt_epi32(_mm_xor_si128(b, _mm_set1_epi32(0x80000000)), _mm_xor_si128(a, _mm_set1_epi32(0x7fffffff))));
	return true;
}

bool ppu_interpreter::VAND(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd] = ppu.VR[op.va] & ppu.VR[op.vb];
	return true;
}

bool ppu_interpreter::VANDC(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd] = v128::andnot(ppu.VR[op.vb], ppu.VR[op.va]);
	return true;
}

bool ppu_interpreter::VAVGSB(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va];
	const auto b = v128::add8(ppu.VR[op.vb], v128::from8p(1)); // add 1
	const auto summ = v128::add8(a, b) & v128::from8p(0xfe);
	const auto sign = v128::from8p(0x80);
	const auto overflow = (((a ^ summ) & (b ^ summ)) ^ summ ^ v128::eq8(b, sign)) & sign; // calculate msb
	ppu.VR[op.vd].vi = _mm_or_si128(overflow.vi, _mm_srli_epi64(summ.vi, 1));
	return true;
}

bool ppu_interpreter::VAVGSH(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va];
	const auto b = v128::add16(ppu.VR[op.vb], v128::from16p(1)); // add 1
	const auto summ = v128::add16(a, b);
	const auto sign = v128::from16p(0x8000);
	const auto overflow = (((a ^ summ) & (b ^ summ)) ^ summ ^ v128::eq16(b, sign)) & sign; // calculate msb
	ppu.VR[op.vd].vi = _mm_or_si128(overflow.vi, _mm_srli_epi16(summ.vi, 1));
	return true;
}

bool ppu_interpreter::VAVGSW(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va];
	const auto b = v128::add32(ppu.VR[op.vb], v128::from32p(1)); // add 1
	const auto summ = v128::add32(a, b);
	const auto sign = v128::from32p(0x80000000);
	const auto overflow = (((a ^ summ) & (b ^ summ)) ^ summ ^ v128::eq32(b, sign)) & sign; // calculate msb
	ppu.VR[op.vd].vi = _mm_or_si128(overflow.vi, _mm_srli_epi32(summ.vi, 1));
	return true;
}

bool ppu_interpreter::VAVGUB(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_avg_epu8(ppu.VR[op.va].vi, ppu.VR[op.vb].vi);
	return true;
}

bool ppu_interpreter::VAVGUH(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_avg_epu16(ppu.VR[op.va].vi, ppu.VR[op.vb].vi);
	return true;
}

bool ppu_interpreter::VAVGUW(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va];
	const auto b = ppu.VR[op.vb];
	const auto summ = v128::add32(v128::add32(a, b), v128::from32p(1));
	const auto carry = _mm_xor_si128(_mm_slli_epi32(sse_cmpgt_epu32(summ.vi, a.vi), 31), _mm_set1_epi32(0x80000000));
	ppu.VR[op.vd].vi = _mm_or_si128(carry, _mm_srli_epi32(summ.vi, 1));
	return true;
}

bool ppu_interpreter::VCFSX(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vf = _mm_mul_ps(_mm_cvtepi32_ps(ppu.VR[op.vb].vi), g_ppu_scale_table[0 - op.vuimm]);
	return true;
}

bool ppu_interpreter::VCFUX(PPUThread& ppu, ppu_opcode_t op)
{
	const auto b = ppu.VR[op.vb].vi;
	const auto fix = _mm_and_ps(_mm_castsi128_ps(_mm_srai_epi32(b, 31)), _mm_set1_ps(0x80000000));
	ppu.VR[op.vd].vf = _mm_mul_ps(_mm_add_ps(_mm_cvtepi32_ps(_mm_and_si128(b, _mm_set1_epi32(0x7fffffff))), fix), g_ppu_scale_table[0 - op.vuimm]);
	return true;
}

bool ppu_interpreter::VCMPBFP(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va].vf;
	const auto b = ppu.VR[op.vb].vf;
	const auto sign = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
	const auto cmp1 = _mm_cmpnle_ps(a, b);
	const auto cmp2 = _mm_cmpnge_ps(a, _mm_xor_ps(b, sign));
	ppu.VR[op.vd].vf = _mm_or_ps(_mm_and_ps(cmp1, sign), _mm_and_ps(cmp2, _mm_castsi128_ps(_mm_set1_epi32(0x40000000))));
	if (UNLIKELY(op.oe)) ppu.SetCR(6, false, false, _mm_movemask_ps(_mm_or_ps(cmp1, cmp2)) == 0, false);
	return true;
}

bool ppu_interpreter::VCMPEQFP(PPUThread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_ps(ppu.VR[op.vd].vf = _mm_cmpeq_ps(ppu.VR[op.va].vf, ppu.VR[op.vb].vf));
	if (UNLIKELY(op.oe)) ppu.SetCR(6, rmask == 0xf, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPEQUB(PPUThread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8((ppu.VR[op.vd] = v128::eq8(ppu.VR[op.va], ppu.VR[op.vb])).vi);
	if (UNLIKELY(op.oe)) ppu.SetCR(6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPEQUH(PPUThread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8((ppu.VR[op.vd] = v128::eq16(ppu.VR[op.va], ppu.VR[op.vb])).vi);
	if (UNLIKELY(op.oe)) ppu.SetCR(6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPEQUW(PPUThread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8((ppu.VR[op.vd] = v128::eq32(ppu.VR[op.va], ppu.VR[op.vb])).vi);
	if (UNLIKELY(op.oe)) ppu.SetCR(6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPGEFP(PPUThread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_ps(ppu.VR[op.vd].vf = _mm_cmpge_ps(ppu.VR[op.va].vf, ppu.VR[op.vb].vf));
	if (UNLIKELY(op.oe)) ppu.SetCR(6, rmask == 0xf, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPGTFP(PPUThread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_ps(ppu.VR[op.vd].vf = _mm_cmpgt_ps(ppu.VR[op.va].vf, ppu.VR[op.vb].vf));
	if (UNLIKELY(op.oe)) ppu.SetCR(6, rmask == 0xf, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPGTSB(PPUThread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8(ppu.VR[op.vd].vi = _mm_cmpgt_epi8(ppu.VR[op.va].vi, ppu.VR[op.vb].vi));
	if (UNLIKELY(op.oe)) ppu.SetCR(6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPGTSH(PPUThread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8(ppu.VR[op.vd].vi = _mm_cmpgt_epi16(ppu.VR[op.va].vi, ppu.VR[op.vb].vi));
	if (UNLIKELY(op.oe)) ppu.SetCR(6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPGTSW(PPUThread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8(ppu.VR[op.vd].vi = _mm_cmpgt_epi32(ppu.VR[op.va].vi, ppu.VR[op.vb].vi));
	if (UNLIKELY(op.oe)) ppu.SetCR(6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPGTUB(PPUThread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8(ppu.VR[op.vd].vi = sse_cmpgt_epu8(ppu.VR[op.va].vi, ppu.VR[op.vb].vi));
	if (UNLIKELY(op.oe)) ppu.SetCR(6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPGTUH(PPUThread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8(ppu.VR[op.vd].vi = sse_cmpgt_epu16(ppu.VR[op.va].vi, ppu.VR[op.vb].vi));
	if (UNLIKELY(op.oe)) ppu.SetCR(6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCMPGTUW(PPUThread& ppu, ppu_opcode_t op)
{
	const auto rmask = _mm_movemask_epi8(ppu.VR[op.vd].vi = sse_cmpgt_epu32(ppu.VR[op.va].vi, ppu.VR[op.vb].vi));
	if (UNLIKELY(op.oe)) ppu.SetCR(6, rmask == 0xffff, false, rmask == 0, false);
	return true;
}

bool ppu_interpreter::VCTSXS(PPUThread& ppu, ppu_opcode_t op)
{
	const auto scaled = _mm_mul_ps(ppu.VR[op.vb].vf, g_ppu_scale_table[op.vuimm]);
	ppu.VR[op.vd].vi = _mm_xor_si128(_mm_cvttps_epi32(scaled), _mm_castps_si128(_mm_cmpge_ps(scaled, _mm_set1_ps(0x80000000))));
	return true;
}

bool ppu_interpreter::VCTUXS(PPUThread& ppu, ppu_opcode_t op)
{
	const auto scaled1 = _mm_max_ps(_mm_mul_ps(ppu.VR[op.vb].vf, g_ppu_scale_table[op.vuimm]), _mm_set1_ps(0.0f));
	const auto scaled2 = _mm_and_ps(_mm_sub_ps(scaled1, _mm_set1_ps(0x80000000)), _mm_cmpge_ps(scaled1, _mm_set1_ps(0x80000000)));
	ppu.VR[op.vd].vi = _mm_or_si128(_mm_or_si128(_mm_cvttps_epi32(scaled1), _mm_cvttps_epi32(scaled2)), _mm_castps_si128(_mm_cmpge_ps(scaled1, _mm_set1_ps(0x100000000))));
	return true;
}

bool ppu_interpreter::VEXPTEFP(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vf = sse_exp2_ps(ppu.VR[op.vb].vf);
	return true;
}

bool ppu_interpreter::VLOGEFP(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vf = sse_log2_ps(ppu.VR[op.vb].vf);
	return true;
}

bool ppu_interpreter::VMADDFP(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vf = _mm_add_ps(_mm_mul_ps(ppu.VR[op.va].vf, ppu.VR[op.vc].vf), ppu.VR[op.vb].vf);
	return true;
}

bool ppu_interpreter::VMAXFP(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vf = _mm_max_ps(ppu.VR[op.va].vf, ppu.VR[op.vb].vf);
	return true;
}

bool ppu_interpreter::VMAXSB(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va].vi;
	const auto b = ppu.VR[op.vb].vi;
	const auto m = _mm_cmpgt_epi8(a, b);
	ppu.VR[op.vd].vi = _mm_or_si128(_mm_and_si128(m, a), _mm_andnot_si128(m, b));
	return true;
}

bool ppu_interpreter::VMAXSH(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_max_epi16(ppu.VR[op.va].vi, ppu.VR[op.vb].vi);
	return true;
}

bool ppu_interpreter::VMAXSW(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va].vi;
	const auto b = ppu.VR[op.vb].vi;
	const auto m = _mm_cmpgt_epi32(a, b);
	ppu.VR[op.vd].vi = _mm_or_si128(_mm_and_si128(m, a), _mm_andnot_si128(m, b));
	return true;
}

bool ppu_interpreter::VMAXUB(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_max_epu8(ppu.VR[op.va].vi, ppu.VR[op.vb].vi);
	return true;
}

bool ppu_interpreter::VMAXUH(PPUThread& ppu, ppu_opcode_t op)
{
	const auto mask = _mm_set1_epi32(0x80008000);
	ppu.VR[op.vd].vi = _mm_xor_si128(_mm_max_epi16(_mm_xor_si128(ppu.VR[op.va].vi, mask), _mm_xor_si128(ppu.VR[op.vb].vi, mask)), mask);
	return true;
}

bool ppu_interpreter::VMAXUW(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va].vi;
	const auto b = ppu.VR[op.vb].vi;
	const auto m = sse_cmpgt_epu32(a, b);
	ppu.VR[op.vd].vi = _mm_or_si128(_mm_and_si128(m, a), _mm_andnot_si128(m, b));
	return true;
}

bool ppu_interpreter::VMHADDSHS(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va].vi;
	const auto b = ppu.VR[op.vb].vi;
	const auto c = ppu.VR[op.vc].vi;
	const auto m = _mm_or_si128(_mm_srli_epi16(_mm_mullo_epi16(a, b), 15), _mm_slli_epi16(_mm_mulhi_epi16(a, b), 1));
	const auto s = _mm_cmpeq_epi16(m, _mm_set1_epi16(-0x8000)); // detect special case (positive 0x8000)
	ppu.VR[op.vd].vi = _mm_adds_epi16(_mm_adds_epi16(_mm_xor_si128(m, s), c), _mm_srli_epi16(s, 15));
	return true;
}

bool ppu_interpreter::VMHRADDSHS(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va].vi;
	const auto b = ppu.VR[op.vb].vi;
	const auto c = ppu.VR[op.vc].vi;
	const auto m = _mm_mulhrs_epi16(a, b);
	const auto s = _mm_cmpeq_epi16(m, _mm_set1_epi16(-0x8000)); // detect special case (positive 0x8000)
	ppu.VR[op.vd].vi = _mm_adds_epi16(_mm_adds_epi16(_mm_xor_si128(m, s), c), _mm_srli_epi16(s, 15));
	return true;
}

bool ppu_interpreter::VMINFP(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vf = _mm_min_ps(ppu.VR[op.va].vf, ppu.VR[op.vb].vf);
	return true;
}

bool ppu_interpreter::VMINSB(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va].vi;
	const auto b = ppu.VR[op.vb].vi;
	const auto m = _mm_cmpgt_epi8(a, b);
	ppu.VR[op.vd].vi = _mm_or_si128(_mm_andnot_si128(m, a), _mm_and_si128(m, b));
	return true;
}

bool ppu_interpreter::VMINSH(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_min_epi16(ppu.VR[op.va].vi, ppu.VR[op.vb].vi);
	return true;
}

bool ppu_interpreter::VMINSW(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va].vi;
	const auto b = ppu.VR[op.vb].vi;
	const auto m = _mm_cmpgt_epi32(a, b);
	ppu.VR[op.vd].vi = _mm_or_si128(_mm_andnot_si128(m, a), _mm_and_si128(m, b));
	return true;
}

bool ppu_interpreter::VMINUB(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_min_epu8(ppu.VR[op.va].vi, ppu.VR[op.vb].vi);
	return true;
}

bool ppu_interpreter::VMINUH(PPUThread& ppu, ppu_opcode_t op)
{
	const auto mask = _mm_set1_epi32(0x80008000);
	ppu.VR[op.vd].vi = _mm_xor_si128(_mm_min_epi16(_mm_xor_si128(ppu.VR[op.va].vi, mask), _mm_xor_si128(ppu.VR[op.vb].vi, mask)), mask);
	return true;
}

bool ppu_interpreter::VMINUW(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va].vi;
	const auto b = ppu.VR[op.vb].vi;
	const auto m = sse_cmpgt_epu32(a, b);
	ppu.VR[op.vd].vi = _mm_or_si128(_mm_andnot_si128(m, a), _mm_and_si128(m, b));
	return true;
}

bool ppu_interpreter::VMLADDUHM(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_add_epi16(_mm_mullo_epi16(ppu.VR[op.va].vi, ppu.VR[op.vb].vi), ppu.VR[op.vc].vi);
	return true;
}

bool ppu_interpreter::VMRGHB(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_unpackhi_epi8(ppu.VR[op.vb].vi, ppu.VR[op.va].vi);
	return true;
}

bool ppu_interpreter::VMRGHH(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_unpackhi_epi16(ppu.VR[op.vb].vi, ppu.VR[op.va].vi);
	return true;
}

bool ppu_interpreter::VMRGHW(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_unpackhi_epi32(ppu.VR[op.vb].vi, ppu.VR[op.va].vi);
	return true;
}

bool ppu_interpreter::VMRGLB(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_unpacklo_epi8(ppu.VR[op.vb].vi, ppu.VR[op.va].vi);
	return true;
}

bool ppu_interpreter::VMRGLH(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_unpacklo_epi16(ppu.VR[op.vb].vi, ppu.VR[op.va].vi);
	return true;
}

bool ppu_interpreter::VMRGLW(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_unpacklo_epi32(ppu.VR[op.vb].vi, ppu.VR[op.va].vi);
	return true;
}

bool ppu_interpreter::VMSUMMBM(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va].vi; // signed bytes
	const auto b = ppu.VR[op.vb].vi; // unsigned bytes
	const auto c = ppu.VR[op.vc].vi;
	const auto ah = _mm_srai_epi16(a, 8);
	const auto bh = _mm_srli_epi16(b, 8);
	const auto al = _mm_srai_epi16(_mm_slli_epi16(a, 8), 8);
	const auto bl = _mm_and_si128(b, _mm_set1_epi16(0x00ff));
	const auto sh = _mm_madd_epi16(ah, bh);
	const auto sl = _mm_madd_epi16(al, bl);
	ppu.VR[op.vd].vi = _mm_add_epi32(_mm_add_epi32(c, sh), sl);
	return true;
}

bool ppu_interpreter::VMSUMSHM(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_add_epi32(_mm_madd_epi16(ppu.VR[op.va].vi, ppu.VR[op.vb].vi), ppu.VR[op.vc].vi);
	return true;
}

bool ppu_interpreter::VMSUMSHS(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];
	const auto& c = ppu.VR[op.vc];

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
		else if (result < (s64)(s32)0x80000000)
		{
			saturated = 0x80000000;
		}
		else
			saturated = (s32)result;

		d._s32[w] = saturated;
	}
	return true;
}

bool ppu_interpreter::VMSUMUBM(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va].vi;
	const auto b = ppu.VR[op.vb].vi;
	const auto c = ppu.VR[op.vc].vi;
	const auto mask = _mm_set1_epi16(0x00ff);
	const auto ah = _mm_srli_epi16(a, 8);
	const auto al = _mm_and_si128(a, mask);
	const auto bh = _mm_srli_epi16(b, 8);
	const auto bl = _mm_and_si128(b, mask);
	const auto sh = _mm_madd_epi16(ah, bh);
	const auto sl = _mm_madd_epi16(al, bl);
	ppu.VR[op.vd].vi = _mm_add_epi32(_mm_add_epi32(c, sh), sl);
	return true;
}

bool ppu_interpreter::VMSUMUHM(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va].vi;
	const auto b = ppu.VR[op.vb].vi;
	const auto c = ppu.VR[op.vc].vi;
	const auto ml = _mm_mullo_epi16(a, b); // low results
	const auto mh = _mm_mulhi_epu16(a, b); // high results
	const auto ls = _mm_add_epi32(_mm_srli_epi32(ml, 16), _mm_and_si128(ml, _mm_set1_epi32(0x0000ffff)));
	const auto hs = _mm_add_epi32(_mm_slli_epi32(mh, 16), _mm_and_si128(mh, _mm_set1_epi32(0xffff0000)));
	ppu.VR[op.vd].vi = _mm_add_epi32(_mm_add_epi32(c, ls), hs);
	return true;
}

bool ppu_interpreter::VMSUMUHS(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];
	const auto& c = ppu.VR[op.vc];

	for (uint w = 0; w < 4; w++)
	{
		u64 result = 0;
		u32 saturated = 0;

		for (uint h = 0; h < 2; h++)
		{
			result += (u64)a._u16[w * 2 + h] * (u64)b._u16[w * 2 + h];
		}

		result += c._u32[w];

		if (result > 0xffffffffu)
		{
			saturated = 0xffffffff;
		}
		else
			saturated = (u32)result;

		d._u32[w] = saturated;
	}
	return true;
}

bool ppu_interpreter::VMULESB(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_mullo_epi16(_mm_srai_epi16(ppu.VR[op.va].vi, 8), _mm_srai_epi16(ppu.VR[op.vb].vi, 8));
	return true;
}

bool ppu_interpreter::VMULESH(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_madd_epi16(_mm_srli_epi32(ppu.VR[op.va].vi, 16), _mm_srli_epi32(ppu.VR[op.vb].vi, 16));
	return true;
}

bool ppu_interpreter::VMULEUB(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_mullo_epi16(_mm_srli_epi16(ppu.VR[op.va].vi, 8), _mm_srli_epi16(ppu.VR[op.vb].vi, 8));
	return true;
}

bool ppu_interpreter::VMULEUH(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va].vi;
	const auto b = ppu.VR[op.vb].vi;
	const auto ml = _mm_mullo_epi16(a, b);
	const auto mh = _mm_mulhi_epu16(a, b);
	ppu.VR[op.vd].vi = _mm_or_si128(_mm_srli_epi32(ml, 16), _mm_and_si128(mh, _mm_set1_epi32(0xffff0000)));
	return true;
}

bool ppu_interpreter::VMULOSB(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_mullo_epi16(_mm_srai_epi16(_mm_slli_epi16(ppu.VR[op.va].vi, 8), 8), _mm_srai_epi16(_mm_slli_epi16(ppu.VR[op.vb].vi, 8), 8));
	return true;
}

bool ppu_interpreter::VMULOSH(PPUThread& ppu, ppu_opcode_t op)
{
	const auto mask = _mm_set1_epi32(0x0000ffff);
	ppu.VR[op.vd].vi = _mm_madd_epi16(_mm_and_si128(ppu.VR[op.va].vi, mask), _mm_and_si128(ppu.VR[op.vb].vi, mask));
	return true;
}

bool ppu_interpreter::VMULOUB(PPUThread& ppu, ppu_opcode_t op)
{
	const auto mask = _mm_set1_epi16(0x00ff);
	ppu.VR[op.vd].vi = _mm_mullo_epi16(_mm_and_si128(ppu.VR[op.va].vi, mask), _mm_and_si128(ppu.VR[op.vb].vi, mask));
	return true;
}

bool ppu_interpreter::VMULOUH(PPUThread& ppu, ppu_opcode_t op)
{
	const auto a = ppu.VR[op.va].vi;
	const auto b = ppu.VR[op.vb].vi;
	const auto ml = _mm_mullo_epi16(a, b);
	const auto mh = _mm_mulhi_epu16(a, b);
	ppu.VR[op.vd].vi = _mm_or_si128(_mm_slli_epi32(mh, 16), _mm_and_si128(ml, _mm_set1_epi32(0x0000ffff)));
	return true;
}

bool ppu_interpreter::VNMSUBFP(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vf = _mm_sub_ps(ppu.VR[op.vb].vf, _mm_mul_ps(ppu.VR[op.va].vf, ppu.VR[op.vc].vf));
	return true;
}

bool ppu_interpreter::VNOR(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd] = ~(ppu.VR[op.va] | ppu.VR[op.vb]);
	return true;
}

bool ppu_interpreter::VOR(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd] = ppu.VR[op.va] | ppu.VR[op.vb];
	return true;
}

bool ppu_interpreter::VPERM(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = sse_altivec_vperm(ppu.VR[op.va].vi, ppu.VR[op.vb].vi, ppu.VR[op.vc].vi);
	return true;
}

bool ppu_interpreter::VPKPX(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	v128 VA = ppu.VR[op.va];
	v128 VB = ppu.VR[op.vb];
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

bool ppu_interpreter::VPKSHSS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_packs_epi16(ppu.VR[op.vb].vi, ppu.VR[op.va].vi);
	return true;
}

bool ppu_interpreter::VPKSHUS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_packus_epi16(ppu.VR[op.vb].vi, ppu.VR[op.va].vi);
	return true;
}

bool ppu_interpreter::VPKSWSS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_packs_epi32(ppu.VR[op.vb].vi, ppu.VR[op.va].vi);
	return true;
}

bool ppu_interpreter::VPKSWUS(PPUThread& ppu, ppu_opcode_t op)
{
	//ppu.VR[op.vd].vi = _mm_packus_epi32(ppu.VR[op.vb].vi, ppu.VR[op.va].vi);
	auto& d = ppu.VR[op.vd];
	v128 VA = ppu.VR[op.va];
	v128 VB = ppu.VR[op.vb];
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

bool ppu_interpreter::VPKUHUM(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	v128 VA = ppu.VR[op.va];
	v128 VB = ppu.VR[op.vb];
	for (uint b = 0; b < 8; b++)
	{
		d._u8[b + 8] = VA._u8[b * 2];
		d._u8[b] = VB._u8[b * 2];
	}
	return true;
}

bool ppu_interpreter::VPKUHUS(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	v128 VA = ppu.VR[op.va];
	v128 VB = ppu.VR[op.vb];
	for (uint b = 0; b < 8; b++)
	{
		u16 result = VA._u16[b];

		if (result > UINT8_MAX)
		{
			result = UINT8_MAX;
		}

		d._u8[b + 8] = (u8)result;

		result = VB._u16[b];

		if (result > UINT8_MAX)
		{
			result = UINT8_MAX;
		}

		d._u8[b] = (u8)result;
	}
	return true;
}

bool ppu_interpreter::VPKUWUM(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	v128 VA = ppu.VR[op.va];
	v128 VB = ppu.VR[op.vb];
	for (uint h = 0; h < 4; h++)
	{
		d._u16[h + 4] = VA._u16[h * 2];
		d._u16[h] = VB._u16[h * 2];
	}
	return true;
}

bool ppu_interpreter::VPKUWUS(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	v128 VA = ppu.VR[op.va];
	v128 VB = ppu.VR[op.vb];
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

bool ppu_interpreter::VREFP(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vf = _mm_rcp_ps(ppu.VR[op.vb].vf);
	return true;
}

bool ppu_interpreter::VRFIM(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& b = ppu.VR[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._f[w] = std::floor(b._f[w]);
	}
	return true;
}

bool ppu_interpreter::VRFIN(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& b = ppu.VR[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._f[w] = std::nearbyint(b._f[w]);
	}
	return true;
}

bool ppu_interpreter::VRFIP(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& b = ppu.VR[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._f[w] = std::ceil(b._f[w]);
	}
	return true;
}

bool ppu_interpreter::VRFIZ(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& b = ppu.VR[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._f[w] = std::truncf(b._f[w]);
	}
	return true;
}

bool ppu_interpreter::VRLB(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint i = 0; i < 16; i++)
	{
		d._u8[i] = rol8(a._u8[i], b._u8[i] & 0x7);
	}
	return true;
}

bool ppu_interpreter::VRLH(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint i = 0; i < 8; i++)
	{
		d._u16[i] = rol16(a._u16[i], b._u8[i * 2] & 0xf);
	}
	return true;
}

bool ppu_interpreter::VRLW(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._u32[w] = rol32(a._u32[w], b._u8[w * 4] & 0x1f);
	}
	return true;
}

bool ppu_interpreter::VRSQRTEFP(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vf = _mm_rsqrt_ps(ppu.VR[op.vb].vf);
	return true;
}

bool ppu_interpreter::VSEL(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];
	const auto& c = ppu.VR[op.vc];

	d = (b & c) | v128::andnot(c, a);
	return true;
}

bool ppu_interpreter::VSL(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	v128 VA = ppu.VR[op.va];
	u8 sh = ppu.VR[op.vb]._u8[0] & 0x7;

	d._u8[0] = VA._u8[0] << sh;
	for (uint b = 1; b < 16; b++)
	{
		d._u8[b] = (VA._u8[b] << sh) | (VA._u8[b - 1] >> (8 - sh));
	}
	return true;
}

bool ppu_interpreter::VSLB(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint i = 0; i < 16; i++)
	{
		d._u8[i] = a._u8[i] << (b._u8[i] & 0x7);
	}
	return true;
}

bool ppu_interpreter::VSLDOI(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	u8 tmpSRC[32];
	std::memcpy(tmpSRC, &ppu.VR[op.vb], 16);
	std::memcpy(tmpSRC + 16, &ppu.VR[op.va], 16);

	for (uint b = 0; b<16; b++)
	{
		d._u8[15 - b] = tmpSRC[31 - (b + op.vsh)];
	}
	return true;
}

bool ppu_interpreter::VSLH(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint h = 0; h < 8; h++)
	{
		d._u16[h] = a._u16[h] << (b._u16[h] & 0xf);
	}
	return true;
}

bool ppu_interpreter::VSLO(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	v128 VA = ppu.VR[op.va];
	u8 nShift = (ppu.VR[op.vb]._u8[0] >> 3) & 0xf;

	d.clear();

	for (u8 b = 0; b < 16 - nShift; b++)
	{
		d._u8[15 - b] = VA._u8[15 - (b + nShift)];
	}
	return true;
}

bool ppu_interpreter::VSLW(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._u32[w] = a._u32[w] << (b._u32[w] & 0x1f);
	}
	return true;
}

bool ppu_interpreter::VSPLTB(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	u8 byte = ppu.VR[op.vb]._u8[15 - op.vuimm];

	for (uint b = 0; b < 16; b++)
	{
		d._u8[b] = byte;
	}
	return true;
}

bool ppu_interpreter::VSPLTH(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	EXPECTS(op.vuimm < 8);

	u16 hword = ppu.VR[op.vb]._u16[7 - op.vuimm];

	for (uint h = 0; h < 8; h++)
	{
		d._u16[h] = hword;
	}
	return true;
}

bool ppu_interpreter::VSPLTISB(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const s8 imm = op.vsimm;

	for (uint b = 0; b < 16; b++)
	{
		d._u8[b] = imm;
	}
	return true;
}

bool ppu_interpreter::VSPLTISH(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const s16 imm = op.vsimm;

	for (uint h = 0; h < 8; h++)
	{
		d._u16[h] = imm;
	}
	return true;
}

bool ppu_interpreter::VSPLTISW(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const s32 imm = op.vsimm;

	for (uint w = 0; w < 4; w++)
	{
		d._u32[w] = imm;
	}
	return true;
}

bool ppu_interpreter::VSPLTW(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	EXPECTS(op.vuimm < 4);

	u32 word = ppu.VR[op.vb]._u32[3 - op.vuimm];

	for (uint w = 0; w < 4; w++)
	{
		d._u32[w] = word;
	}
	return true;
}

bool ppu_interpreter::VSR(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	v128 VA = ppu.VR[op.va];
	u8 sh = ppu.VR[op.vb]._u8[0] & 0x7;

	d._u8[15] = VA._u8[15] >> sh;
	for (uint b = 14; ~b; b--)
	{
		d._u8[b] = (VA._u8[b] >> sh) | (VA._u8[b + 1] << (8 - sh));
	}
	return true;
}

bool ppu_interpreter::VSRAB(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint i = 0; i < 16; i++)
	{
		d._s8[i] = a._s8[i] >> (b._u8[i] & 0x7);
	}
	return true;
}

bool ppu_interpreter::VSRAH(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint h = 0; h < 8; h++)
	{
		d._s16[h] = a._s16[h] >> (b._u16[h] & 0xf);
	}
	return true;
}

bool ppu_interpreter::VSRAW(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._s32[w] = a._s32[w] >> (b._u32[w] & 0x1f);
	}
	return true;
}

bool ppu_interpreter::VSRB(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];
	
	for (uint i = 0; i < 16; i++)
	{
		d._u8[i] = a._u8[i] >> (b._u8[i] & 0x7);
	}
	return true;
}

bool ppu_interpreter::VSRH(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint h = 0; h < 8; h++)
	{
		d._u16[h] = a._u16[h] >> (b._u16[h] & 0xf);
	}
	return true;
}

bool ppu_interpreter::VSRO(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	v128 VA = ppu.VR[op.va];
	u8 nShift = (ppu.VR[op.vb]._u8[0] >> 3) & 0xf;

	d.clear();

	for (u8 b = 0; b < 16 - nShift; b++)
	{
		d._u8[b] = VA._u8[b + nShift];
	}
	return true;
}

bool ppu_interpreter::VSRW(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._u32[w] = a._u32[w] >> (b._u32[w] & 0x1f);
	}
	return true;
}

bool ppu_interpreter::VSUBCUW(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		d._u32[w] = a._u32[w] < b._u32[w] ? 0 : 1;
	}
	return true;
}

bool ppu_interpreter::VSUBFP(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd] = v128::subfs(ppu.VR[op.va], ppu.VR[op.vb]);
	return true;
}

bool ppu_interpreter::VSUBSBS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_subs_epi8(ppu.VR[op.va].vi, ppu.VR[op.vb].vi);
	return true;
}

bool ppu_interpreter::VSUBSHS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_subs_epi16(ppu.VR[op.va].vi, ppu.VR[op.vb].vi);
	return true;
}

bool ppu_interpreter::VSUBSWS(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		s64 result = (s64)a._s32[w] - (s64)b._s32[w];

		if (result < INT32_MIN)
		{
			d._s32[w] = (s32)INT32_MIN;
		}
		else if (result > INT32_MAX)
		{
			d._s32[w] = (s32)INT32_MAX;
		}
		else
			d._s32[w] = (s32)result;
	}
	return true;
}

bool ppu_interpreter::VSUBUBM(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd] = v128::sub8(ppu.VR[op.va], ppu.VR[op.vb]);
	return true;
}

bool ppu_interpreter::VSUBUBS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_subs_epu8(ppu.VR[op.va].vi, ppu.VR[op.vb].vi);
	return true;
}

bool ppu_interpreter::VSUBUHM(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd] = v128::sub16(ppu.VR[op.va], ppu.VR[op.vb]);
	return true;
}

bool ppu_interpreter::VSUBUHS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd].vi = _mm_subs_epu16(ppu.VR[op.va].vi, ppu.VR[op.vb].vi);
	return true;
}

bool ppu_interpreter::VSUBUWM(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd] = v128::sub32(ppu.VR[op.va], ppu.VR[op.vb]);
	return true;
}

bool ppu_interpreter::VSUBUWS(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		s64 result = (s64)a._u32[w] - (s64)b._u32[w];

		if (result < 0)
		{
			d._u32[w] = 0;
		}
		else
			d._u32[w] = (u32)result;
	}
	return true;
}

bool ppu_interpreter::VSUMSWS(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	s64 sum = b._s32[0];

	for (uint w = 0; w < 4; w++)
	{
		sum += a._s32[w];
	}

	d.clear();
	if (sum > INT32_MAX)
	{
		d._s32[0] = (s32)INT32_MAX;
	}
	else if (sum < INT32_MIN)
	{
		d._s32[0] = (s32)INT32_MIN;
	}
	else
		d._s32[0] = (s32)sum;
	return true;
}

bool ppu_interpreter::VSUM2SWS(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint n = 0; n < 2; n++)
	{
		s64 sum = (s64)a._s32[n * 2] + a._s32[n * 2 + 1] + b._s32[n * 2];

		if (sum > INT32_MAX)
		{
			d._s32[n * 2] = (s32)INT32_MAX;
		}
		else if (sum < INT32_MIN)
		{
			d._s32[n * 2] = (s32)INT32_MIN;
		}
		else
			d._s32[n * 2] = (s32)sum;
	}
	d._s32[1] = 0;
	d._s32[3] = 0;
	return true;
}

bool ppu_interpreter::VSUM4SBS(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		s64 sum = b._s32[w];

		for (uint b = 0; b < 4; b++)
		{
			sum += a._s8[w * 4 + b];
		}

		if (sum > INT32_MAX)
		{
			d._s32[w] = (s32)INT32_MAX;
		}
		else if (sum < INT32_MIN)
		{
			d._s32[w] = (s32)INT32_MIN;
		}
		else
			d._s32[w] = (s32)sum;
	}
	return true;
}

bool ppu_interpreter::VSUM4SHS(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		s64 sum = b._s32[w];

		for (uint h = 0; h < 2; h++)
		{
			sum += a._s16[w * 2 + h];
		}

		if (sum > INT32_MAX)
		{
			d._s32[w] = (s32)INT32_MAX;
		}
		else if (sum < INT32_MIN)
		{
			d._s32[w] = (s32)INT32_MIN;
		}
		else
			d._s32[w] = (s32)sum;
	}
	return true;
}

bool ppu_interpreter::VSUM4UBS(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	const auto& a = ppu.VR[op.va];
	const auto& b = ppu.VR[op.vb];

	for (uint w = 0; w < 4; w++)
	{
		u64 sum = b._u32[w];

		for (uint b = 0; b < 4; b++)
		{
			sum += a._u8[w * 4 + b];
		}

		if (sum > UINT32_MAX)
		{
			d._u32[w] = (u32)UINT32_MAX;
		}
		else
			d._u32[w] = (u32)sum;
	}
	return true;
}

bool ppu_interpreter::VUPKHPX(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	v128 VB = ppu.VR[op.vb];
	for (uint w = 0; w < 4; w++)
	{
		d._s8[w * 4 + 3] = VB._s8[8 + w * 2 + 1] >> 7;  // signed shift sign extends
		d._u8[w * 4 + 2] = (VB._u8[8 + w * 2 + 1] >> 2) & 0x1f;
		d._u8[w * 4 + 1] = ((VB._u8[8 + w * 2 + 1] & 0x3) << 3) | ((VB._u8[8 + w * 2 + 0] >> 5) & 0x7);
		d._u8[w * 4 + 0] = VB._u8[8 + w * 2 + 0] & 0x1f;
	}
	return true;
}

bool ppu_interpreter::VUPKHSB(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	v128 VB = ppu.VR[op.vb];
	for (uint h = 0; h < 8; h++)
	{
		d._s16[h] = VB._s8[8 + h];
	}
	return true;
}

bool ppu_interpreter::VUPKHSH(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	v128 VB = ppu.VR[op.vb];
	for (uint w = 0; w < 4; w++)
	{
		d._s32[w] = VB._s16[4 + w];
	}
	return true;
}

bool ppu_interpreter::VUPKLPX(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	v128 VB = ppu.VR[op.vb];
	for (uint w = 0; w < 4; w++)
	{
		d._s8[w * 4 + 3] = VB._s8[w * 2 + 1] >> 7;  // signed shift sign extends
		d._u8[w * 4 + 2] = (VB._u8[w * 2 + 1] >> 2) & 0x1f;
		d._u8[w * 4 + 1] = ((VB._u8[w * 2 + 1] & 0x3) << 3) | ((VB._u8[w * 2 + 0] >> 5) & 0x7);
		d._u8[w * 4 + 0] = VB._u8[w * 2 + 0] & 0x1f;
	}
	return true;
}

bool ppu_interpreter::VUPKLSB(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	v128 VB = ppu.VR[op.vb];
	for (uint h = 0; h < 8; h++)
	{
		d._s16[h] = VB._s8[h];
	}
	return true;
}

bool ppu_interpreter::VUPKLSH(PPUThread& ppu, ppu_opcode_t op)
{
	auto& d = ppu.VR[op.vd];
	v128 VB = ppu.VR[op.vb];
	for (uint w = 0; w < 4; w++)
	{
		d._s32[w] = VB._s16[w];
	}
	return true;
}

bool ppu_interpreter::VXOR(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.VR[op.vd] = ppu.VR[op.va] ^ ppu.VR[op.vb];
	return true;
}

bool ppu_interpreter::TDI(PPUThread& ppu, ppu_opcode_t op)
{
	const s64 a = ppu.GPR[op.ra], b = op.simm16;
	const u64 a_ = a, b_ = b;

	if (((op.bo & 0x10) && a < b) ||
		((op.bo & 0x8) && a > b) ||
		((op.bo & 0x4) && a == b) ||
		((op.bo & 0x2) && a_ < b_) ||
		((op.bo & 0x1) && a_ > b_))
	{
		throw std::runtime_error("Trap!" HERE);
	}

	return true;
}

bool ppu_interpreter::TWI(PPUThread& ppu, ppu_opcode_t op)
{
	const s32 a = u32(ppu.GPR[op.ra]), b = op.simm16;
	const u32 a_ = a, b_ = b;

	if (((op.bo & 0x10) && a < b) ||
		((op.bo & 0x8) && a > b) ||
		((op.bo & 0x4) && a == b) ||
		((op.bo & 0x2) && a_ < b_) ||
		((op.bo & 0x1) && a_ > b_))
	{
		throw std::runtime_error("Trap!" HERE);
	}

	return true;
}

bool ppu_interpreter::MULLI(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.rd] = (s64)ppu.GPR[op.ra] * op.simm16;
	return true;
}

bool ppu_interpreter::SUBFIC(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 a = ppu.GPR[op.ra];
	const s64 i = op.simm16;
	const auto r = add64_flags(~a, i, 1);
	ppu.GPR[op.rd] = r.result;
	ppu.CA = r.carry;
	return true;
}

bool ppu_interpreter::CMPLI(PPUThread& ppu, ppu_opcode_t op)
{
	if (op.l10)
	{
		ppu.SetCR<u64>(op.crfd, ppu.GPR[op.ra], op.uimm16);
	}
	else
	{
		ppu.SetCR<u32>(op.crfd, u32(ppu.GPR[op.ra]), op.uimm16);
	}
	return true;
}

bool ppu_interpreter::CMPI(PPUThread& ppu, ppu_opcode_t op)
{
	if (op.l10)
	{
		ppu.SetCR<s64>(op.crfd, ppu.GPR[op.ra], op.simm16);
	}
	else
	{
		ppu.SetCR<s32>(op.crfd, u32(ppu.GPR[op.ra]), op.simm16);
	}
	return true;
}

bool ppu_interpreter::ADDIC(PPUThread& ppu, ppu_opcode_t op)
{
	const s64 a = ppu.GPR[op.ra];
	const s64 i = op.simm16;
	const auto r = add64_flags(a, i);
	ppu.GPR[op.rd] = r.result;
	ppu.CA = r.carry;
	if (UNLIKELY(op.main & 1)) ppu.SetCR<s64>(0, r.result, 0);
	return true;
}

bool ppu_interpreter::ADDI(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.rd] = op.ra ? ((s64)ppu.GPR[op.ra] + op.simm16) : op.simm16;
	return true;
}

bool ppu_interpreter::ADDIS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.rd] = op.ra ? ((s64)ppu.GPR[op.ra] + (op.simm16 << 16)) : (op.simm16 << 16);
	return true;
}

bool ppu_interpreter::BC(PPUThread& ppu, ppu_opcode_t op)
{
	const bool bo0 = (op.bo & 0x10) != 0;
	const bool bo1 = (op.bo & 0x08) != 0;
	const bool bo2 = (op.bo & 0x04) != 0;
	const bool bo3 = (op.bo & 0x02) != 0;

	ppu.CTR -= (bo2 ^ true);

	const bool ctr_ok = bo2 | ((ppu.CTR != 0) ^ bo3);
	const bool cond_ok = bo0 | (ppu.CR[op.bi] ^ (bo1 ^ true));

	if (ctr_ok && cond_ok)
	{
		const u32 nextLR = ppu.pc + 4;
		ppu.pc = ppu_branch_target((op.aa ? 0 : ppu.pc), op.simm16);
		if (op.lk) ppu.LR = nextLR;
		return false;
	}
	else
	{
		return true;
	}
}

bool ppu_interpreter::HACK(PPUThread& ppu, ppu_opcode_t op)
{
	ppu_execute_function(ppu, op.opcode & 0x3ffffff);
	return true;
}

bool ppu_interpreter::SC(PPUThread& ppu, ppu_opcode_t op)
{
	switch (u32 lv = op.lev)
	{
	case 0x0: ppu_execute_syscall(ppu, ppu.GPR[11]); break;
	default: throw fmt::exception("SC lv%u", lv);
	}

	return true;
}

bool ppu_interpreter::B(PPUThread& ppu, ppu_opcode_t op)
{
	const u32 nextLR = ppu.pc + 4;
	ppu.pc = ppu_branch_target(op.aa ? 0 : ppu.pc, op.ll);
	if (op.lk) ppu.LR = nextLR;
	return false;
}

bool ppu_interpreter::MCRF(PPUThread& ppu, ppu_opcode_t op)
{
	CHECK_SIZE(PPUThread::CR, 32);
	reinterpret_cast<u32*>(ppu.CR)[op.crfd] = reinterpret_cast<u32*>(ppu.CR)[op.crfs];
	return true;
}

bool ppu_interpreter::BCLR(PPUThread& ppu, ppu_opcode_t op)
{
	const bool bo0 = (op.bo & 0x10) != 0;
	const bool bo1 = (op.bo & 0x08) != 0;
	const bool bo2 = (op.bo & 0x04) != 0;
	const bool bo3 = (op.bo & 0x02) != 0;

	ppu.CTR -= (bo2 ^ true);

	const bool ctr_ok = bo2 | ((ppu.CTR != 0) ^ bo3);
	const bool cond_ok = bo0 | (ppu.CR[op.bi] ^ (bo1 ^ true));

	if (ctr_ok && cond_ok)
	{
		const u32 nextLR = ppu.pc + 4;
		ppu.pc = ppu_branch_target(0, (u32)ppu.LR);
		if (op.lk) ppu.LR = nextLR;
		return false;
	}
	else
	{
		return true;
	}
}

bool ppu_interpreter::CRNOR(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.CR[op.crbd] = (ppu.CR[op.crba] | ppu.CR[op.crbb]) ^ true;
	return true;
}

bool ppu_interpreter::CRANDC(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.CR[op.crbd] = ppu.CR[op.crba] & (ppu.CR[op.crbb] ^ true);
	return true;
}

bool ppu_interpreter::ISYNC(PPUThread& ppu, ppu_opcode_t op)
{
	_mm_mfence();
	return true;
}

bool ppu_interpreter::CRXOR(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.CR[op.crbd] = ppu.CR[op.crba] ^ ppu.CR[op.crbb];
	return true;
}

bool ppu_interpreter::CRNAND(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.CR[op.crbd] = (ppu.CR[op.crba] & ppu.CR[op.crbb]) ^ true;
	return true;
}

bool ppu_interpreter::CRAND(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.CR[op.crbd] = ppu.CR[op.crba] & ppu.CR[op.crbb];
	return true;
}

bool ppu_interpreter::CREQV(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.CR[op.crbd] = (ppu.CR[op.crba] ^ ppu.CR[op.crbb]) ^ true;
	return true;
}

bool ppu_interpreter::CRORC(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.CR[op.crbd] = ppu.CR[op.crba] | (ppu.CR[op.crbb] ^ true);
	return true;
}

bool ppu_interpreter::CROR(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.CR[op.crbd] = ppu.CR[op.crba] | ppu.CR[op.crbb];
	return true;
}

bool ppu_interpreter::BCCTR(PPUThread& ppu, ppu_opcode_t op)
{
	if (op.bo & 0x10 || ppu.CR[op.bi] == ((op.bo & 0x8) != 0))
	{
		const u32 nextLR = ppu.pc + 4;
		ppu.pc = ppu_branch_target(0, (u32)ppu.CTR);
		if (op.lk) ppu.LR = nextLR;
		return false;
	}
	
	return true;
}

bool ppu_interpreter::RLWIMI(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 mask = ppu_rotate_mask(32 + op.mb32, 32 + op.me32);
	ppu.GPR[op.ra] = (ppu.GPR[op.ra] & ~mask) | (dup32(rol32(u32(ppu.GPR[op.rs]), op.sh32)) & mask);
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::RLWINM(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = dup32(rol32(u32(ppu.GPR[op.rs]), op.sh32)) & ppu_rotate_mask(32 + op.mb32, 32 + op.me32);
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::RLWNM(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = dup32(rol32(u32(ppu.GPR[op.rs]), ppu.GPR[op.rb] & 0x1f)) & ppu_rotate_mask(32 + op.mb32, 32 + op.me32);
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::ORI(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = ppu.GPR[op.rs] | op.uimm16;
	return true;
}

bool ppu_interpreter::ORIS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = ppu.GPR[op.rs] | ((u64)op.uimm16 << 16);
	return true;
}

bool ppu_interpreter::XORI(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = ppu.GPR[op.rs] ^ op.uimm16;
	return true;
}

bool ppu_interpreter::XORIS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = ppu.GPR[op.rs] ^ ((u64)op.uimm16 << 16);
	return true;
}

bool ppu_interpreter::ANDI(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = ppu.GPR[op.rs] & op.uimm16;
	ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::ANDIS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = ppu.GPR[op.rs] & ((u64)op.uimm16 << 16);
	ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::RLDICL(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = rol64(ppu.GPR[op.rs], op.sh64) & (~0ull >> op.mbe64);
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::RLDICR(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = rol64(ppu.GPR[op.rs], op.sh64) & (~0ull << (op.mbe64 ^ 63));
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::RLDIC(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = rol64(ppu.GPR[op.rs], op.sh64) & ppu_rotate_mask(op.mbe64, op.sh64 ^ 63);
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::RLDIMI(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 mask = ppu_rotate_mask(op.mbe64, op.sh64 ^ 63);
	ppu.GPR[op.ra] = (ppu.GPR[op.ra] & ~mask) | (rol64(ppu.GPR[op.rs], op.sh64) & mask);
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::RLDCL(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = rol64(ppu.GPR[op.rs], ppu.GPR[op.rb] & 0x3f) & (~0ull >> op.mbe64);
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::RLDCR(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = rol64(ppu.GPR[op.rs], ppu.GPR[op.rb] & 0x3f) & (~0ull << (op.mbe64 ^ 63));
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::CMP(PPUThread& ppu, ppu_opcode_t op)
{
	if (op.l10)
	{
		ppu.SetCR<s64>(op.crfd, ppu.GPR[op.ra], ppu.GPR[op.rb]);
	}
	else
	{
		ppu.SetCR<s32>(op.crfd, u32(ppu.GPR[op.ra]), u32(ppu.GPR[op.rb]));
	}
	return true;
}

bool ppu_interpreter::TW(PPUThread& ppu, ppu_opcode_t op)
{
	s32 a = (s32)ppu.GPR[op.ra];
	s32 b = (s32)ppu.GPR[op.rb];

	if ((a < b && (op.bo & 0x10)) ||
		(a > b && (op.bo & 0x8)) ||
		(a == b && (op.bo & 0x4)) ||
		((u32)a < (u32)b && (op.bo & 0x2)) ||
		((u32)a >(u32)b && (op.bo & 0x1)))
	{
		throw std::runtime_error("Trap!" HERE);
	}

	return true;
}

bool ppu_interpreter::LVSL(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.VR[op.vd].vi = sse_altivec_lvsl(addr);
	return true;
}

bool ppu_interpreter::LVEBX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.VR[op.vd]._u8[15 - (addr & 0xf)] = vm::read8(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::SUBFC(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.GPR[op.ra];
	const u64 RB = ppu.GPR[op.rb];
	const auto r = add64_flags(~RA, RB, 1);
	ppu.GPR[op.rd] = r.result;
	ppu.CA = r.carry;
	if (UNLIKELY(op.oe)) ppu.SetOV((~RA >> 63 == RB >> 63) && (~RA >> 63 != ppu.GPR[op.rd] >> 63));
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, r.result, 0);
	return true;
}

bool ppu_interpreter::MULHDU(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.rd] = UMULH64(ppu.GPR[op.ra], ppu.GPR[op.rb]);
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.rd], 0);
	return true;
}

bool ppu_interpreter::ADDC(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.GPR[op.ra];
	const u64 RB = ppu.GPR[op.rb];
	const auto r = add64_flags(RA, RB);
	ppu.GPR[op.rd] = r.result;
	ppu.CA = r.carry;
	if (UNLIKELY(op.oe)) ppu.SetOV((RA >> 63 == RB >> 63) && (RA >> 63 != ppu.GPR[op.rd] >> 63));
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, r.result, 0);
	return true;
}

bool ppu_interpreter::MULHWU(PPUThread& ppu, ppu_opcode_t op)
{
	u32 a = (u32)ppu.GPR[op.ra];
	u32 b = (u32)ppu.GPR[op.rb];
	ppu.GPR[op.rd] = ((u64)a * (u64)b) >> 32;
	if (UNLIKELY(op.rc)) ppu.SetCR(0, false, false, false, ppu.SO);
	return true;
}

bool ppu_interpreter::MFOCRF(PPUThread& ppu, ppu_opcode_t op)
{
	if (op.l11)
	{
		// MFOCRF
		const u32 n = cntlz32(op.crm) & 7;
		const u32 p = n * 4;
		const u32 v = ppu.CR[p + 0] << 3 | ppu.CR[p + 1] << 2 | ppu.CR[p + 2] << 1 | ppu.CR[p + 3] << 0;
		
		ppu.GPR[op.rd] = v << (p ^ 0x1c);
	}
	else
	{
		// MFCR
		auto* lanes = reinterpret_cast<be_t<v128>*>(ppu.CR);
		const u32 mh = _mm_movemask_epi8(_mm_slli_epi64(lanes[0].value().vi, 7));
		const u32 ml = _mm_movemask_epi8(_mm_slli_epi64(lanes[1].value().vi, 7));

		ppu.GPR[op.rd] = (mh << 16) | ml;
	}
	return true;
}

bool ppu_interpreter::LWARX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];

	be_t<u32> value;
	vm::reservation_acquire(&value, vm::cast(addr, HERE), SIZE_32(value));

	ppu.GPR[op.rd] = value;
	return true;
}

bool ppu_interpreter::LDX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.GPR[op.rd] = vm::read64(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LWZX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.GPR[op.rd] = vm::read32(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::SLW(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = u32(ppu.GPR[op.rs] << (ppu.GPR[op.rb] & 0x3f));
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::CNTLZW(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = cntlz32(u32(ppu.GPR[op.rs]));
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::SLD(PPUThread& ppu, ppu_opcode_t op)
{
	const u32 n = ppu.GPR[op.rb];
	ppu.GPR[op.ra] = UNLIKELY(n & 0x40) ? 0 : ppu.GPR[op.rs] << n;
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::AND(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = ppu.GPR[op.rs] & ppu.GPR[op.rb];
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::CMPL(PPUThread& ppu, ppu_opcode_t op)
{
	if (op.l10)
	{
		ppu.SetCR<u64>(op.crfd, ppu.GPR[op.ra], ppu.GPR[op.rb]);
	}
	else
	{
		ppu.SetCR<u32>(op.crfd, u32(ppu.GPR[op.ra]), u32(ppu.GPR[op.rb]));
	}
	return true;
}

bool ppu_interpreter::LVSR(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.VR[op.vd].vi = sse_altivec_lvsr(addr);
	return true;
}

bool ppu_interpreter::LVEHX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb]) & ~1ULL;
	ppu.VR[op.vd]._u16[7 - ((addr >> 1) & 0x7)] = vm::read16(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::SUBF(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.GPR[op.ra];
	const u64 RB = ppu.GPR[op.rb];
	ppu.GPR[op.rd] = RB - RA;
	if (UNLIKELY(op.oe)) ppu.SetOV((~RA >> 63 == RB >> 63) && (~RA >> 63 != ppu.GPR[op.rd] >> 63));
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.rd], 0);
	return true;
}

bool ppu_interpreter::LDUX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + ppu.GPR[op.rb];
	ppu.GPR[op.rd] = vm::read64(vm::cast(addr, HERE));
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::DCBST(PPUThread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::LWZUX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + ppu.GPR[op.rb];
	ppu.GPR[op.rd] = vm::read32(vm::cast(addr, HERE));
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::CNTLZD(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = cntlz64(ppu.GPR[op.rs]);
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::ANDC(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = ppu.GPR[op.rs] & ~ppu.GPR[op.rb];
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::TD(PPUThread& ppu, ppu_opcode_t op)
{
	const s64 a = ppu.GPR[op.ra], b = ppu.GPR[op.rb];
	const u64 a_ = a, b_ = b;

	if (((op.bo & 0x10) && a < b) ||
		((op.bo & 0x8) && a > b) ||
		((op.bo & 0x4) && a == b) ||
		((op.bo & 0x2) && a_ < b_) ||
		((op.bo & 0x1) && a_ > b_))
	{
		throw std::runtime_error("Trap!" HERE);
	}

	return true;
}

bool ppu_interpreter::LVEWX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb]) & ~3ULL;
	ppu.VR[op.vd]._u32[3 - ((addr >> 2) & 0x3)] = vm::read32(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::MULHD(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.rd] = MULH64(ppu.GPR[op.ra], ppu.GPR[op.rb]);
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.rd], 0);
	return true;
}

bool ppu_interpreter::MULHW(PPUThread& ppu, ppu_opcode_t op)
{
	s32 a = (s32)ppu.GPR[op.ra];
	s32 b = (s32)ppu.GPR[op.rb];
	ppu.GPR[op.rd] = ((s64)a * (s64)b) >> 32;
	if (UNLIKELY(op.rc)) ppu.SetCR(0, false, false, false, ppu.SO);
	return true;
}

bool ppu_interpreter::LDARX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];

	be_t<u64> value;
	vm::reservation_acquire(&value, vm::cast(addr, HERE), SIZE_32(value));

	ppu.GPR[op.rd] = value;
	return true;
}

bool ppu_interpreter::DCBF(PPUThread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::LBZX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.GPR[op.rd] = vm::read8(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LVX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb]) & ~0xfull;
	ppu.VR[op.vd] = vm::_ref<v128>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::NEG(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.GPR[op.ra];
	ppu.GPR[op.rd] = 0 - RA;
	if (UNLIKELY(op.oe)) ppu.SetOV((~RA >> 63 == 0) && (~RA >> 63 != ppu.GPR[op.rd] >> 63));
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.rd], 0);
	return true;
}

bool ppu_interpreter::LBZUX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + ppu.GPR[op.rb];
	ppu.GPR[op.rd] = vm::read8(vm::cast(addr, HERE));
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::NOR(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = ~(ppu.GPR[op.rs] | ppu.GPR[op.rb]);
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::STVEBX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	const u8 eb = addr & 0xf;
	vm::write8(vm::cast(addr, HERE), ppu.VR[op.vs]._u8[15 - eb]);
	return true;
}

bool ppu_interpreter::SUBFE(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.GPR[op.ra];
	const u64 RB = ppu.GPR[op.rb];
	const auto r = add64_flags(~RA, RB, ppu.CA);
	ppu.GPR[op.rd] = r.result;
	ppu.CA = r.carry;
	if (UNLIKELY(op.oe)) ppu.SetOV((~RA >> 63 == RB >> 63) && (~RA >> 63 != ppu.GPR[op.rd] >> 63));
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, r.result, 0);
	return true;
}

bool ppu_interpreter::ADDE(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.GPR[op.ra];
	const u64 RB = ppu.GPR[op.rb];
	const auto r = add64_flags(RA, RB, ppu.CA);
	ppu.GPR[op.rd] = r.result;
	ppu.CA = r.carry;
	if (UNLIKELY(op.oe)) ppu.SetOV((RA >> 63 == RB >> 63) && (RA >> 63 != ppu.GPR[op.rd] >> 63));
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, r.result, 0);
	return true;
}

bool ppu_interpreter::MTOCRF(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 s = ppu.GPR[op.rs];

	if (op.l11)
	{
		// MTOCRF

		const u32 n = cntlz32(op.crm) & 7;
		const u32 p = n * 4;
		const u64 v = s >> (p ^ 0x1c);
		ppu.CR[p + 0] = (v & 8) != 0;
		ppu.CR[p + 1] = (v & 4) != 0;
		ppu.CR[p + 2] = (v & 2) != 0;
		ppu.CR[p + 3] = (v & 1) != 0;
	}
	else
	{
		// MTCRF

		for (u32 i = 0; i < 8; i++)
		{
			const u32 p = i * 4;
			const u64 v = s >> (p ^ 0x1c);

			if (op.crm & (128 >> i))
			{
				ppu.CR[p + 0] = (v & 8) != 0;
				ppu.CR[p + 1] = (v & 4) != 0;
				ppu.CR[p + 2] = (v & 2) != 0;
				ppu.CR[p + 3] = (v & 1) != 0;
			}
		}
	}
	return true;
}

bool ppu_interpreter::STDX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	vm::write64(vm::cast(addr, HERE), ppu.GPR[op.rs]);
	return true;
}

bool ppu_interpreter::STWCX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];

	const be_t<u32> value = (u32)ppu.GPR[op.rs];
	ppu.SetCR(0, false, false, vm::reservation_update(vm::cast(addr, HERE), &value, SIZE_32(value)), ppu.SO);
	return true;
}

bool ppu_interpreter::STWX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	vm::write32(vm::cast(addr, HERE), (u32)ppu.GPR[op.rs]);
	return true;
}

bool ppu_interpreter::STVEHX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb]) & ~1ULL;
	const u8 eb = (addr & 0xf) >> 1;
	vm::write16(vm::cast(addr, HERE), ppu.VR[op.vs]._u16[7 - eb]);
	return true;
}

bool ppu_interpreter::STDUX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + ppu.GPR[op.rb];
	vm::write64(vm::cast(addr, HERE), ppu.GPR[op.rs]);
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STWUX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + ppu.GPR[op.rb];
	vm::write32(vm::cast(addr, HERE), (u32)ppu.GPR[op.rs]);
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STVEWX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb]) & ~3ULL;
	const u8 eb = (addr & 0xf) >> 2;
	vm::write32(vm::cast(addr, HERE), ppu.VR[op.vs]._u32[3 - eb]);
	return true;
}

bool ppu_interpreter::SUBFZE(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.GPR[op.ra];
	const auto r = add64_flags(~RA, 0, ppu.CA);
	ppu.GPR[op.rd] = r.result;
	ppu.CA = r.carry;
	if (UNLIKELY(op.oe)) ppu.SetOV((~RA >> 63 == 0) && (~RA >> 63 != ppu.GPR[op.rd] >> 63));
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, r.result, 0);
	return true;
}

bool ppu_interpreter::ADDZE(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.GPR[op.ra];
	const auto r = add64_flags(RA, 0, ppu.CA);
	ppu.GPR[op.rd] = r.result;
	ppu.CA = r.carry;
	if (UNLIKELY(op.oe)) ppu.SetOV((RA >> 63 == 0) && (RA >> 63 != ppu.GPR[op.rd] >> 63));
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, r.result, 0);
	return true;
}

bool ppu_interpreter::STDCX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];

	const be_t<u64> value = ppu.GPR[op.rs];
	ppu.SetCR(0, false, false, vm::reservation_update(vm::cast(addr, HERE), &value, SIZE_32(value)), ppu.SO);
	return true;
}

bool ppu_interpreter::STBX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	vm::write8(vm::cast(addr, HERE), (u8)ppu.GPR[op.rs]);
	return true;
}

bool ppu_interpreter::STVX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb]) & ~0xfull;
	vm::_ref<v128>(vm::cast(addr, HERE)) = ppu.VR[op.vs];
	return true;
}

bool ppu_interpreter::MULLD(PPUThread& ppu, ppu_opcode_t op)
{
	const s64 RA = ppu.GPR[op.ra];
	const s64 RB = ppu.GPR[op.rb];
	ppu.GPR[op.rd] = (s64)(RA * RB);
	if (UNLIKELY(op.oe))
	{
		const s64 high = MULH64(RA, RB);
		ppu.SetOV(high != s64(ppu.GPR[op.rd]) >> 63);
	}
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.rd], 0);
	return true;
}

bool ppu_interpreter::SUBFME(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.GPR[op.ra];
	const auto r = add64_flags(~RA, ~0ull, ppu.CA);
	ppu.GPR[op.rd] = r.result;
	ppu.CA = r.carry;
	if (UNLIKELY(op.oe)) ppu.SetOV((~RA >> 63 == 1) && (~RA >> 63 != ppu.GPR[op.rd] >> 63));
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, r.result, 0);
	return true;
}

bool ppu_interpreter::ADDME(PPUThread& ppu, ppu_opcode_t op)
{
	const s64 RA = ppu.GPR[op.ra];
	const auto r = add64_flags(RA, ~0ull, ppu.CA);
	ppu.GPR[op.rd] = r.result;
	ppu.CA = r.carry;
	if (UNLIKELY(op.oe)) ppu.SetOV((u64(RA) >> 63 == 1) && (u64(RA) >> 63 != ppu.GPR[op.rd] >> 63));
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, r.result, 0);
	return true;
}

bool ppu_interpreter::MULLW(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.rd] = (s64)((s64)(s32)ppu.GPR[op.ra] * (s64)(s32)ppu.GPR[op.rb]);
	if (UNLIKELY(op.oe)) ppu.SetOV(s64(ppu.GPR[op.rd]) < s64(-1) << 31 || s64(ppu.GPR[op.rd]) >= s64(1) << 31);
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::DCBTST(PPUThread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::STBUX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + ppu.GPR[op.rb];
	vm::write8(vm::cast(addr, HERE), (u8)ppu.GPR[op.rs]);
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::ADD(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.GPR[op.ra];
	const u64 RB = ppu.GPR[op.rb];
	ppu.GPR[op.rd] = RA + RB;
	if (UNLIKELY(op.oe)) ppu.SetOV((RA >> 63 == RB >> 63) && (RA >> 63 != ppu.GPR[op.rd] >> 63));
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.rd], 0);
	return true;
}

bool ppu_interpreter::DCBT(PPUThread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::LHZX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.GPR[op.rd] = vm::read16(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::EQV(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = ~(ppu.GPR[op.rs] ^ ppu.GPR[op.rb]);
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::ECIWX(PPUThread& ppu, ppu_opcode_t op)
{
	throw std::runtime_error("ECIWX" HERE);
}

bool ppu_interpreter::LHZUX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.GPR[op.rd] = vm::read16(vm::cast(addr, HERE));
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::XOR(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = ppu.GPR[op.rs] ^ ppu.GPR[op.rb];
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::MFSPR(PPUThread& ppu, ppu_opcode_t op)
{
	const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5);

	switch (n)
	{
	case 0x001: ppu.GPR[op.rd] = u32{ ppu.SO } << 31 | ppu.OV << 30 | ppu.CA << 29 | ppu.XCNT; break;
	case 0x008: ppu.GPR[op.rd] = ppu.LR; break;
	case 0x009: ppu.GPR[op.rd] = ppu.CTR; break;
	case 0x100: ppu.GPR[op.rd] = ppu.VRSAVE; break;

	case 0x10C: ppu.GPR[op.rd] = get_timebased_time() & 0xffffffff; break;
	case 0x10D: ppu.GPR[op.rd] = get_timebased_time() >> 32; break;
	default: throw fmt::exception("MFSPR 0x%x" HERE, n);
	}

	return true;
}

bool ppu_interpreter::LWAX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.GPR[op.rd] = (s64)(s32)vm::read32(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::DST(PPUThread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::LHAX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.GPR[op.rd] = (s64)(s16)vm::read16(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LVXL(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb]) & ~0xfull;
	ppu.VR[op.vd] = vm::_ref<v128>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::MFTB(PPUThread& ppu, ppu_opcode_t op)
{
	const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5);

	switch (n)
	{
	case 0x10C: ppu.GPR[op.rd] = get_timebased_time() & 0xffffffff; break;
	case 0x10D: ppu.GPR[op.rd] = get_timebased_time() >> 32; break;
	default: throw fmt::exception("MFSPR 0x%x" HERE, n);
	}

	return true;
}

bool ppu_interpreter::LWAUX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.GPR[op.rd] = (s64)(s32)vm::read32(vm::cast(addr, HERE));
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::DSTST(PPUThread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::LHAUX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.GPR[op.rd] = (s64)(s16)vm::read16(vm::cast(addr, HERE));
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STHX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	vm::write16(vm::cast(addr, HERE), (u16)ppu.GPR[op.rs]);
	return true;
}

bool ppu_interpreter::ORC(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = ppu.GPR[op.rs] | ~ppu.GPR[op.rb];
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::ECOWX(PPUThread& ppu, ppu_opcode_t op)
{
	throw std::runtime_error("ECOWX" HERE);
}

bool ppu_interpreter::STHUX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + ppu.GPR[op.rb];
	vm::write16(vm::cast(addr, HERE), (u16)ppu.GPR[op.rs]);
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::OR(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = ppu.GPR[op.rs] | ppu.GPR[op.rb];
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::DIVDU(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 RA = ppu.GPR[op.ra];
	const u64 RB = ppu.GPR[op.rb];
	ppu.GPR[op.rd] = RB == 0 ? 0 : RA / RB;
	if (UNLIKELY(op.oe)) ppu.SetOV(RB == 0);
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.rd], 0);
	return true;
}

bool ppu_interpreter::DIVWU(PPUThread& ppu, ppu_opcode_t op)
{
	const u32 RA = (u32)ppu.GPR[op.ra];
	const u32 RB = (u32)ppu.GPR[op.rb];
	ppu.GPR[op.rd] = RB == 0 ? 0 : RA / RB;
	if (UNLIKELY(op.oe)) ppu.SetOV(RB == 0);
	if (UNLIKELY(op.rc)) ppu.SetCR(0, false, false, false, ppu.SO);
	return true;
}

bool ppu_interpreter::MTSPR(PPUThread& ppu, ppu_opcode_t op)
{
	const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5);

	switch (n)
	{
	case 0x001:
	{
		const u64 value = ppu.GPR[op.rs];
		ppu.SO = (value & 0x80000000) != 0;
		ppu.OV = (value & 0x40000000) != 0;
		ppu.CA = (value & 0x20000000) != 0;
		ppu.XCNT = value & 0x7f;
		break;
	}
	case 0x008: ppu.LR = ppu.GPR[op.rs]; break;
	case 0x009: ppu.CTR = ppu.GPR[op.rs]; break;
	case 0x100: ppu.VRSAVE = (u32)ppu.GPR[op.rs]; break;
	default: throw fmt::exception("MTSPR 0x%x" HERE, n);
	}

	return true;
}

bool ppu_interpreter::DCBI(PPUThread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::NAND(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = ~(ppu.GPR[op.rs] & ppu.GPR[op.rb]);
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::STVXL(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb]) & ~0xfull;
	vm::_ref<v128>(vm::cast(addr, HERE)) = ppu.VR[op.vs];
	return true;
}

bool ppu_interpreter::DIVD(PPUThread& ppu, ppu_opcode_t op)
{
	const s64 RA = ppu.GPR[op.ra];
	const s64 RB = ppu.GPR[op.rb];
	const bool o = RB == 0 || ((u64)RA == (1ULL << 63) && RB == -1);
	ppu.GPR[op.rd] = o ? 0 : RA / RB;
	if (UNLIKELY(op.oe)) ppu.SetOV(o);
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.rd], 0);
	return true;
}

bool ppu_interpreter::DIVW(PPUThread& ppu, ppu_opcode_t op)
{
	const s32 RA = (s32)ppu.GPR[op.ra];
	const s32 RB = (s32)ppu.GPR[op.rb];
	const bool o = RB == 0 || ((u32)RA == (1 << 31) && RB == -1);
	ppu.GPR[op.rd] = o ? 0 : u32(RA / RB);
	if (UNLIKELY(op.oe)) ppu.SetOV(o);
	if (UNLIKELY(op.rc)) ppu.SetCR(0, false, false, false, ppu.SO);
	return true;
}

bool ppu_interpreter::LVLX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.VR[op.vd].vi = sse_cellbe_lvlx(addr);
	return true;
}

bool ppu_interpreter::LDBRX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.GPR[op.rd] = vm::_ref<le_t<u64>>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LSWX(PPUThread& ppu, ppu_opcode_t op)
{
	u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	u32 count = ppu.XCNT & 0x7f;
	for (; count >= 4; count -= 4, addr += 4, op.rd = (op.rd + 1) & 31)
	{
		ppu.GPR[op.rd] = vm::_ref<u32>(vm::cast(addr, HERE));
	}
	if (count)
	{
		u32 value = 0;
		for (u32 byte = 0; byte < count; byte++)
		{
			u32 byte_value = vm::_ref<u8>(vm::cast(addr + byte, HERE));
			value |= byte_value << ((3 ^ byte) * 8);
		}
		ppu.GPR[op.rd] = value;
	}
	return true;
}

bool ppu_interpreter::LWBRX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.GPR[op.rd] = vm::_ref<le_t<u32>>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LFSX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.FPR[op.frd] = vm::_ref<f32>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::SRW(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = (ppu.GPR[op.rs] & 0xffffffff) >> (ppu.GPR[op.rb] & 0x3f);
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::SRD(PPUThread& ppu, ppu_opcode_t op)
{
	const u32 n = ppu.GPR[op.rb];
	ppu.GPR[op.ra] = UNLIKELY(n & 0x40) ? 0 : ppu.GPR[op.rs] >> n;
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::LVRX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.VR[op.vd].vi = sse_cellbe_lvrx(addr);
	return true;
}

bool ppu_interpreter::LSWI(PPUThread& ppu, ppu_opcode_t op)
{
	u64 addr = op.ra ? ppu.GPR[op.ra] : 0;
	u64 N = op.rb ? op.rb : 32;
	u8 reg = op.rd;

	while (N > 0)
	{
		if (N > 3)
		{
			ppu.GPR[reg] = vm::read32(vm::cast(addr, HERE));
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
			ppu.GPR[reg] = buf;
		}
		reg = (reg + 1) % 32;
	}
	return true;
}

bool ppu_interpreter::LFSUX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + ppu.GPR[op.rb];
	ppu.FPR[op.frd] = vm::_ref<f32>(vm::cast(addr, HERE));
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::SYNC(PPUThread& ppu, ppu_opcode_t op)
{
	_mm_mfence();
	return true;
}

bool ppu_interpreter::LFDX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.FPR[op.frd] = vm::_ref<f64>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LFDUX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + ppu.GPR[op.rb];
	ppu.FPR[op.frd] = vm::_ref<f64>(vm::cast(addr, HERE));
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STVLX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	sse_cellbe_stvlx(addr, ppu.VR[op.vs].vi);
	return true;
}

bool ppu_interpreter::STDBRX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	vm::_ref<le_t<u64>>(vm::cast(addr, HERE)) = ppu.GPR[op.rs];
	return true;
}

bool ppu_interpreter::STSWX(PPUThread& ppu, ppu_opcode_t op)
{
	u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	u32 count = ppu.XCNT & 0x7F;
	for (; count >= 4; count -= 4, addr += 4, op.rs = (op.rs + 1) & 31)
	{
		vm::write32(vm::cast(addr, HERE), (u32)ppu.GPR[op.rs]);
	}
	if (count)
	{
		u32 value = (u32)ppu.GPR[op.rs];
		for (u32 byte = 0; byte < count; byte++)
		{
			u32 byte_value = (u8)(value >> ((3 ^ byte) * 8));
			vm::write8(vm::cast(addr + byte, HERE), byte_value);
		}
	}
	return true;
}

bool ppu_interpreter::STWBRX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	vm::_ref<le_t<u32>>(vm::cast(addr, HERE)) = (u32)ppu.GPR[op.rs];
	return true;
}

bool ppu_interpreter::STFSX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	vm::_ref<f32>(vm::cast(addr, HERE)) = static_cast<float>(ppu.FPR[op.frs]);
	return true;
}

bool ppu_interpreter::STVRX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	sse_cellbe_stvrx(addr, ppu.VR[op.vs].vi);
	return true;
}

bool ppu_interpreter::STFSUX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + ppu.GPR[op.rb];
	vm::_ref<f32>(vm::cast(addr, HERE)) = static_cast<float>(ppu.FPR[op.frs]);
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STSWI(PPUThread& ppu, ppu_opcode_t op)
{
	u64 addr = op.ra ? ppu.GPR[op.ra] : 0;
	u64 N = op.rb ? op.rb : 32;
	u8 reg = op.rd;

	while (N > 0)
	{
		if (N > 3)
		{
			vm::write32(vm::cast(addr, HERE), (u32)ppu.GPR[reg]);
			addr += 4;
			N -= 4;
		}
		else
		{
			u32 buf = (u32)ppu.GPR[reg];
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

bool ppu_interpreter::STFDX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	vm::_ref<f64>(vm::cast(addr, HERE)) = ppu.FPR[op.frs];
	return true;
}

bool ppu_interpreter::STFDUX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + ppu.GPR[op.rb];
	vm::_ref<f64>(vm::cast(addr, HERE)) = ppu.FPR[op.frs];
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::LVLXL(PPUThread& ppu, ppu_opcode_t op)
{
	return LVLX(ppu, op);
}

bool ppu_interpreter::LHBRX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	ppu.GPR[op.rd] = vm::_ref<le_t<u16>>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::SRAW(PPUThread& ppu, ppu_opcode_t op)
{
	s32 RS = (s32)ppu.GPR[op.rs];
	u8 shift = ppu.GPR[op.rb] & 63;
	if (shift > 31)
	{
		ppu.GPR[op.ra] = 0 - (RS < 0);
		ppu.CA = (RS < 0);
	}
	else
	{
		ppu.GPR[op.ra] = RS >> shift;
		ppu.CA = (RS < 0) && ((ppu.GPR[op.ra] << shift) != RS);
	}

	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::SRAD(PPUThread& ppu, ppu_opcode_t op)
{
	s64 RS = ppu.GPR[op.rs];
	u8 shift = ppu.GPR[op.rb] & 127;
	if (shift > 63)
	{
		ppu.GPR[op.ra] = 0 - (RS < 0);
		ppu.CA = (RS < 0);
	}
	else
	{
		ppu.GPR[op.ra] = RS >> shift;
		ppu.CA = (RS < 0) && ((ppu.GPR[op.ra] << shift) != RS);
	}

	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::LVRXL(PPUThread& ppu, ppu_opcode_t op)
{
	return LVRX(ppu, op);
}

bool ppu_interpreter::DSS(PPUThread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::SRAWI(PPUThread& ppu, ppu_opcode_t op)
{
	s32 RS = (u32)ppu.GPR[op.rs];
	ppu.GPR[op.ra] = RS >> op.sh32;
	ppu.CA = (RS < 0) && ((u32)(ppu.GPR[op.ra] << op.sh32) != RS);

	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::SRADI(PPUThread& ppu, ppu_opcode_t op)
{
	auto sh = op.sh64;
	s64 RS = ppu.GPR[op.rs];
	ppu.GPR[op.ra] = RS >> sh;
	ppu.CA = (RS < 0) && ((ppu.GPR[op.ra] << sh) != RS);

	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::EIEIO(PPUThread& ppu, ppu_opcode_t op)
{
	_mm_mfence();
	return true;
}

bool ppu_interpreter::STVLXL(PPUThread& ppu, ppu_opcode_t op)
{
	return STVLX(ppu, op);
}

bool ppu_interpreter::STHBRX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	vm::_ref<le_t<u16>>(vm::cast(addr, HERE)) = (u16)ppu.GPR[op.rs];
	return true;
}

bool ppu_interpreter::EXTSH(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = (s64)(s16)ppu.GPR[op.rs];
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::STVRXL(PPUThread& ppu, ppu_opcode_t op)
{
	return STVRX(ppu, op);
}

bool ppu_interpreter::EXTSB(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = (s64)(s8)ppu.GPR[op.rs];
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::STFIWX(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];
	vm::write32(vm::cast(addr, HERE), (u32&)ppu.FPR[op.frs]);
	return true;
}

bool ppu_interpreter::EXTSW(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.GPR[op.ra] = (s64)(s32)ppu.GPR[op.rs];
	if (UNLIKELY(op.rc)) ppu.SetCR<s64>(0, ppu.GPR[op.ra], 0);
	return true;
}

bool ppu_interpreter::ICBI(PPUThread& ppu, ppu_opcode_t op)
{
	return true;
}

bool ppu_interpreter::DCBZ(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + ppu.GPR[op.rb] : ppu.GPR[op.rb];

	std::memset(vm::base(vm::cast(addr, HERE) & ~127), 0, 128);
	return true;
}

bool ppu_interpreter::LWZ(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + op.simm16 : op.simm16;
	ppu.GPR[op.rd] = vm::read32(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LWZU(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + op.simm16;
	ppu.GPR[op.rd] = vm::read32(vm::cast(addr, HERE));
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::LBZ(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + op.simm16 : op.simm16;
	ppu.GPR[op.rd] = vm::read8(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LBZU(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + op.simm16;
	ppu.GPR[op.rd] = vm::read8(vm::cast(addr, HERE));
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STW(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + op.simm16 : op.simm16;
	vm::write32(vm::cast(addr, HERE), (u32)ppu.GPR[op.rs]);
	return true;
}

bool ppu_interpreter::STWU(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + op.simm16;
	vm::write32(vm::cast(addr, HERE), (u32)ppu.GPR[op.rs]);
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STB(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + op.simm16 : op.simm16;
	vm::write8(vm::cast(addr, HERE), (u8)ppu.GPR[op.rs]);
	return true;
}

bool ppu_interpreter::STBU(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + op.simm16;
	vm::write8(vm::cast(addr, HERE), (u8)ppu.GPR[op.rs]);
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::LHZ(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + op.simm16 : op.simm16;
	ppu.GPR[op.rd] = vm::read16(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LHZU(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + op.simm16;
	ppu.GPR[op.rd] = vm::read16(vm::cast(addr, HERE));
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::LHA(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + op.simm16 : op.simm16;
	ppu.GPR[op.rd] = (s64)(s16)vm::read16(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LHAU(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + op.simm16;
	ppu.GPR[op.rd] = (s64)(s16)vm::read16(vm::cast(addr, HERE));
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STH(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + op.simm16 : op.simm16;
	vm::write16(vm::cast(addr, HERE), (u16)ppu.GPR[op.rs]);
	return true;
}

bool ppu_interpreter::STHU(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + op.simm16;
	vm::write16(vm::cast(addr, HERE), (u16)ppu.GPR[op.rs]);
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::LMW(PPUThread& ppu, ppu_opcode_t op)
{
	u64 addr = op.ra ? ppu.GPR[op.ra] + op.simm16 : op.simm16;
	for (u32 i = op.rd; i<32; ++i, addr += 4)
	{
		ppu.GPR[i] = vm::read32(vm::cast(addr, HERE));
	}
	return true;
}

bool ppu_interpreter::STMW(PPUThread& ppu, ppu_opcode_t op)
{
	u64 addr = op.ra ? ppu.GPR[op.ra] + op.simm16 : op.simm16;
	for (u32 i = op.rs; i<32; ++i, addr += 4)
	{
		vm::write32(vm::cast(addr, HERE), (u32)ppu.GPR[i]);
	}
	return true;
}

bool ppu_interpreter::LFS(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + op.simm16 : op.simm16;
	ppu.FPR[op.frd] = vm::_ref<f32>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LFSU(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + op.simm16;
	ppu.FPR[op.frd] = vm::_ref<f32>(vm::cast(addr, HERE));
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::LFD(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + op.simm16 : op.simm16;
	ppu.FPR[op.frd] = vm::_ref<f64>(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LFDU(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + op.simm16;
	ppu.FPR[op.frd] = vm::_ref<f64>(vm::cast(addr, HERE));
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STFS(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + op.simm16 : op.simm16;
	vm::_ref<f32>(vm::cast(addr, HERE)) = static_cast<float>(ppu.FPR[op.frs]);
	return true;
}

bool ppu_interpreter::STFSU(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + op.simm16;
	vm::_ref<f32>(vm::cast(addr, HERE)) = static_cast<float>(ppu.FPR[op.frs]);
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::STFD(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = op.ra ? ppu.GPR[op.ra] + op.simm16 : op.simm16;
	vm::_ref<f64>(vm::cast(addr, HERE)) = ppu.FPR[op.frs];
	return true;
}

bool ppu_interpreter::STFDU(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + op.simm16;
	vm::_ref<f64>(vm::cast(addr, HERE)) = ppu.FPR[op.frs];
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::LD(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.simm16 & ~3) + (op.ra ? ppu.GPR[op.ra] : 0);
	ppu.GPR[op.rd] = vm::read64(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::LDU(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + (op.simm16 & ~3);
	ppu.GPR[op.rd] = vm::read64(vm::cast(addr, HERE));
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::LWA(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.simm16 & ~3) + (op.ra ? ppu.GPR[op.ra] : 0);
	ppu.GPR[op.rd] = (s64)(s32)vm::read32(vm::cast(addr, HERE));
	return true;
}

bool ppu_interpreter::STD(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = (op.simm16 & ~3) + (op.ra ? ppu.GPR[op.ra] : 0);
	vm::write64(vm::cast(addr, HERE), ppu.GPR[op.rs]);
	return true;
}

bool ppu_interpreter::STDU(PPUThread& ppu, ppu_opcode_t op)
{
	const u64 addr = ppu.GPR[op.ra] + (op.simm16 & ~3);
	vm::write64(vm::cast(addr, HERE), ppu.GPR[op.rs]);
	ppu.GPR[op.ra] = addr;
	return true;
}

bool ppu_interpreter::FDIVS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = f32(ppu.FPR[op.fra] / ppu.FPR[op.frb]);
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FSUBS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = f32(ppu.FPR[op.fra] - ppu.FPR[op.frb]);
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FADDS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = f32(ppu.FPR[op.fra] + ppu.FPR[op.frb]);
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FSQRTS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = f32(std::sqrt(ppu.FPR[op.frb]));
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FRES(PPUThread& ppu, ppu_opcode_t op)
{
	f32 value = f32(ppu.FPR[op.frb]);
	_mm_store_ss(&value, _mm_rcp_ss(_mm_load_ss(&value)));
	ppu.FPR[op.frd] = value;
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FMULS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = f32(ppu.FPR[op.fra] * ppu.FPR[op.frc]);
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FMADDS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = f32(ppu.FPR[op.fra] * ppu.FPR[op.frc] + ppu.FPR[op.frb]);
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FMSUBS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = f32(ppu.FPR[op.fra] * ppu.FPR[op.frc] - ppu.FPR[op.frb]);
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FNMSUBS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = f32(-(ppu.FPR[op.fra] * ppu.FPR[op.frc]) + ppu.FPR[op.frb]);
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FNMADDS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = f32(-(ppu.FPR[op.fra] * ppu.FPR[op.frc]) - ppu.FPR[op.frb]);
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::MTFSB1(PPUThread& ppu, ppu_opcode_t op)
{
	LOG_WARNING(PPU, "MTFSB1");
	if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::MCRFS(PPUThread& ppu, ppu_opcode_t op)
{
	LOG_WARNING(PPU, "MCRFS");
	ppu.SetCR(op.crfd, false, false, false, false);
	return true;
}

bool ppu_interpreter::MTFSB0(PPUThread& ppu, ppu_opcode_t op)
{
	LOG_WARNING(PPU, "MTFSB0");
	if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::MTFSFI(PPUThread& ppu, ppu_opcode_t op)
{
	LOG_WARNING(PPU, "MTFSFI");
	if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::MFFS(PPUThread& ppu, ppu_opcode_t op)
{
	LOG_WARNING(PPU, "MFFS");
	ppu.FPR[op.frd] = 0.0;
	if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::MTFSF(PPUThread& ppu, ppu_opcode_t op)
{
	LOG_WARNING(PPU, "MTFSF");
	if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FCMPU(PPUThread& ppu, ppu_opcode_t op)
{
	const f64 a = ppu.FPR[op.fra];
	const f64 b = ppu.FPR[op.frb];
	ppu.FG = a > b;
	ppu.FL = a < b;
	ppu.FE = a == b;
	//ppu.FU = a != a || b != b;
	ppu.SetCR(op.crfd, ppu.FL, ppu.FG, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FRSP(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = f32(ppu.FPR[op.frb]);
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FCTIW(PPUThread& ppu, ppu_opcode_t op)
{
	(s32&)ppu.FPR[op.frd] = s32(ppu.FPR[op.frb]);
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FCTIWZ(PPUThread& ppu, ppu_opcode_t op)
{
	(s32&)ppu.FPR[op.frd] = _mm_cvttsd_si32(_mm_load_sd(&ppu.FPR[op.frb]));
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FDIV(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = ppu.FPR[op.fra] / ppu.FPR[op.frb];
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FSUB(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = ppu.FPR[op.fra] - ppu.FPR[op.frb];
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FADD(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = ppu.FPR[op.fra] + ppu.FPR[op.frb];
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FSQRT(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = std::sqrt(ppu.FPR[op.frb]);
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FSEL(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = ppu.FPR[op.fra] >= 0.0 ? ppu.FPR[op.frc] : ppu.FPR[op.frb];
	if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FMUL(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = ppu.FPR[op.fra] * ppu.FPR[op.frc];
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FRSQRTE(PPUThread& ppu, ppu_opcode_t op)
{
	f32 value = f32(ppu.FPR[op.frb]);
	_mm_store_ss(&value, _mm_rsqrt_ss(_mm_load_ss(&value)));
	ppu.FPR[op.frd] = value;
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FMSUB(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = ppu.FPR[op.fra] * ppu.FPR[op.frc] - ppu.FPR[op.frb];
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FMADD(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = ppu.FPR[op.fra] * ppu.FPR[op.frc] + ppu.FPR[op.frb];
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FNMSUB(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = -(ppu.FPR[op.fra] * ppu.FPR[op.frc]) + ppu.FPR[op.frb];
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FNMADD(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = -(ppu.FPR[op.fra] * ppu.FPR[op.frc]) - ppu.FPR[op.frb];
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FCMPO(PPUThread& ppu, ppu_opcode_t op)
{
	return FCMPU(ppu, op);
	return true;
}

bool ppu_interpreter::FNEG(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = -ppu.FPR[op.frb];
	if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FMR(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = ppu.FPR[op.frb];
	if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FNABS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = -std::fabs(ppu.FPR[op.frb]);
	if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FABS(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = std::fabs(ppu.FPR[op.frb]);
	if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FCTID(PPUThread& ppu, ppu_opcode_t op)
{
	(s64&)ppu.FPR[op.frd] = s64(ppu.FPR[op.frb]);
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FCTIDZ(PPUThread& ppu, ppu_opcode_t op)
{
	(s64&)ppu.FPR[op.frd] = _mm_cvttsd_si64(_mm_load_sd(&ppu.FPR[op.frb]));
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::FCFID(PPUThread& ppu, ppu_opcode_t op)
{
	ppu.FPR[op.frd] = static_cast<double>((s64&)ppu.FPR[op.frb]);
	VERIFY(!op.rc); //if (UNLIKELY(op.rc)) ppu.SetCR(1, ppu.FG, ppu.FL, ppu.FE, ppu.FU);
	return true;
}

bool ppu_interpreter::UNK(PPUThread& ppu, ppu_opcode_t op)
{
	throw fmt::exception("Unknown/Illegal opcode: 0x%08x (pc=0x%x)" HERE, op.opcode, ppu.pc);
}
