#pragma once

#include "util/types.hpp"

// Floating-point rounding mode (for both PPU and SPU)
enum FPSCR_RN
{
	FPSCR_RN_NEAR = 0,
	FPSCR_RN_ZERO = 1,
	FPSCR_RN_PINF = 2,
	FPSCR_RN_MINF = 3,
};

// Get the exponent of a float
inline int fexpf(float x)
{
	return (std::bit_cast<u32>(x) >> 23) & 0xff;
}

constexpr u32 ppu_fres_mantissas[128] =
{
	0x007f0000,
	0x007d0800,
	0x007b1800,
	0x00793000,
	0x00775000,
	0x00757000,
	0x0073a000,
	0x0071e000,
	0x00700000,
	0x006e4000,
	0x006ca000,
	0x006ae000,
	0x00694000,
	0x00678000,
	0x00660000,
	0x00646000,
	0x0062c000,
	0x00614000,
	0x005fc000,
	0x005e4000,
	0x005cc000,
	0x005b4000,
	0x0059c000,
	0x00584000,
	0x00570000,
	0x00558000,
	0x00540000,
	0x0052c000,
	0x00518000,
	0x00500000,
	0x004ec000,
	0x004d8000,
	0x004c0000,
	0x004b0000,
	0x00498000,
	0x00488000,
	0x00474000,
	0x00460000,
	0x0044c000,
	0x00438000,
	0x00428000,
	0x00418000,
	0x00400000,
	0x003f0000,
	0x003e0000,
	0x003d0000,
	0x003bc000,
	0x003ac000,
	0x00398000,
	0x00388000,
	0x00378000,
	0x00368000,
	0x00358000,
	0x00348000,
	0x00338000,
	0x00328000,
	0x00318000,
	0x00308000,
	0x002f8000,
	0x002ec000,
	0x002e0000,
	0x002d0000,
	0x002c0000,
	0x002b0000,
	0x002a0000,
	0x00298000,
	0x00288000,
	0x00278000,
	0x0026c000,
	0x00260000,
	0x00250000,
	0x00244000,
	0x00238000,
	0x00228000,
	0x00220000,
	0x00210000,
	0x00200000,
	0x001f8000,
	0x001e8000,
	0x001e0000,
	0x001d0000,
	0x001c8000,
	0x001b8000,
	0x001b0000,
	0x001a0000,
	0x00198000,
	0x00190000,
	0x00180000,
	0x00178000,
	0x00168000,
	0x00160000,
	0x00158000,
	0x00148000,
	0x00140000,
	0x00138000,
	0x00128000,
	0x00120000,
	0x00118000,
	0x00108000,
	0x00100000,
	0x000f8000,
	0x000f0000,
	0x000e0000,
	0x000d8000,
	0x000d0000,
	0x000c8000,
	0x000b8000,
	0x000b0000,
	0x000a8000,
	0x000a0000,
	0x00098000,
	0x00090000,
	0x00080000,
	0x00078000,
	0x00070000,
	0x00068000,
	0x00060000,
	0x00058000,
	0x00050000,
	0x00048000,
	0x00040000,
	0x00038000,
	0x00030000,
	0x00028000,
	0x00020000,
	0x00018000,
	0x00010000,
	0x00000000,
};

constexpr u32 ppu_frsqrte_mantissas[16] =
{
	0x000f1000u, 0x000d8000u, 0x000c0000u, 0x000a8000u,
	0x00098000u, 0x00088000u, 0x00080000u, 0x00070000u,
	0x00060000u, 0x0004c000u, 0x0003c000u, 0x00030000u,
	0x00020000u, 0x00018000u, 0x00010000u, 0x00008000u,
};

// Large lookup table for FRSQRTE instruction
inline struct ppu_frsqrte_lut_t
{
	// Store only high 32 bits of doubles
	u32 data[0x8000]{};

	constexpr ppu_frsqrte_lut_t() noexcept
	{
		for (u64 i = 0; i < 0x8000; i++)
		{
			// Decomposed LUT index
			const u64 sign = i >> 14;
			const u64 expv = (i >> 3) & 0x7ff;

			// (0x3FF - (((EXP_BITS(b) - 0x3FF) >> 1) + 1)) << 52
			const u64 exp = 0x3fe0'0000 - (((expv + 0x1c01) >> 1) << (52 - 32));

			if (expv == 0) // ±INF on zero/denormal, not accurate
			{
				data[i] = static_cast<u32>(0x7ff0'0000 | (sign << 31));
			}
			else if (expv == 0x7ff)
			{
				if (i == (0x7ff << 3))
					data[i] = 0; // Zero on +INF, inaccurate
				else
					data[i] = 0x7ff8'0000; // QNaN
			}
			else if (sign)
			{
				data[i] = 0x7ff8'0000; // QNaN
			}
			else
			{
				// ((MAN_BITS(b) >> 49) & 7ull) + (!(EXP_BITS(b) & 1) << 3)
				const u64 idx = 8 ^ (i & 0xf);

				data[i] = static_cast<u32>(ppu_frsqrte_mantissas[idx] | exp);
			}
		}
	}
} ppu_frqrte_lut;

constexpr u32 ppu_vrefp_segments[32][2] =
{
	{0x7ff800, 0x3e1}, {0x783800, 0x3a7}, {0x70ea00, 0x371}, {0x6a0800, 0x340},
	{0x638800, 0x313}, {0x5d6200, 0x2ea}, {0x579000, 0x2c4}, {0x520800, 0x2a0},
	{0x4cc800, 0x27f}, {0x47ca00, 0x261}, {0x430800, 0x245}, {0x3e8000, 0x22a},
	{0x3a2c00, 0x212}, {0x360800, 0x1fb}, {0x321400, 0x1e5}, {0x2e4a00, 0x1d1},
	{0x2aa800, 0x1be}, {0x272c00, 0x1ac}, {0x23d600, 0x19b}, {0x209e00, 0x18b},
	{0x1d8800, 0x17c}, {0x1a9000, 0x16e}, {0x17ae00, 0x15b}, {0x14f800, 0x15b},
	{0x124400, 0x143}, {0x0fbe00, 0x143}, {0x0d3800, 0x12d}, {0x0ade00, 0x12d},
	{0x088400, 0x11a}, {0x065000, 0x11a}, {0x041c00, 0x108}, {0x020c00, 0x106},
};

constexpr u32 ppu_vrsqrtefp_segments[32][3] =
{
	{0x34fd00, 0x568, 0x00}, {0x2f9700, 0x4f3, 0x26}, {0x2aa500, 0x48d, 0xc8}, {0x261800, 0x435, 0xc8},
	{0x21e400, 0x3e7, 0x26}, {0x1dfe00, 0x3a2, 0x88}, {0x1a5c00, 0x365, 0xc8}, {0x16f800, 0x32e, 0x22},
	{0x13ca00, 0x2fc, 0x00}, {0x10ce00, 0x2d0, 0x00}, {0x0dfe00, 0x2a8, 0x00}, {0x0b5700, 0x283, 0x26},
	{0x08d400, 0x261, 0xc8}, {0x067300, 0x243, 0x26}, {0x043100, 0x226, 0x22}, {0x020b00, 0x20b, 0x26},
	{0x7ff400, 0x7a4, 0x00}, {0x785200, 0x700, 0x00}, {0x715400, 0x670, 0x00}, {0x6ae400, 0x5f2, 0x88},
	{0x64f200, 0x584, 0x00}, {0x5f6e00, 0x524, 0x00}, {0x5a4c00, 0x4cc, 0x00}, {0x558000, 0x47e, 0x22},
	{0x510200, 0x43a, 0x88}, {0x4cca00, 0x3fa, 0x88}, {0x48d000, 0x3c2, 0x88}, {0x450e00, 0x38e, 0x22},
	{0x418200, 0x35e, 0x22}, {0x3e2400, 0x332, 0x88}, {0x3af200, 0x30a, 0x88}, {0x37e800, 0x2e6, 0x22},
};

constexpr u16 ppu_vlogefp_segments[16][2] =
{
	{0x0000, 12}, {0x0600, 10}, {0x0b00, 10}, {0x1000, 9},
	{0x1480, 9}, {0x1900, 9}, {0x1d80, 8}, {0x2180, 8},
	{0x2580, 8}, {0x297f, 7}, {0x2cff, 7}, {0x307f, 7},
	{0x33fe, 6}, {0x36fe, 6}, {0x39fe, 6}, {0x3cfe, 6},
};

constexpr u16 ppu_vexptefp_segments[16][2] =
{
	{0x0006, 6}, {0x0306, 6}, {0x0606, 6}, {0x0906, 6},
	{0x0c07, 7}, {0x0f87, 7}, {0x1307, 7}, {0x1680, 8},
	{0x1a80, 8}, {0x1e80, 8}, {0x2288, 9}, {0x2708, 9},
	{0x2b88, 9}, {0x3008, 10}, {0x3508, 10}, {0x3a08, 12},
};

constexpr void ppu_vnormalize(s32& e, u32& m)
{
	const u32 shift = std::countl_zero(m) - 8;
	m = (m << shift) & 0x7fffff;
	e = 1 - static_cast<s32>(shift);
}

constexpr u32 ppu_vrefp(u32 x, bool nj)
{
	const u32 s = x & 0x80000000;
	s32 e = (x >> 23) & 0xff;
	u32 m = x & 0x7fffff;

	if (e == 0xff)
		return m ? x | 0x400000 : s;

	if (e == 0 && (nj || m == 0))
		return s | 0x7f800000;

	if (e == 0)
		ppu_vnormalize(e, m);

	const auto& seg = ppu_vrefp_segments[m >> 18];
	const u32 mant = seg[0] - seg[1] * ((m >> 9) & 0x1ff);
	const s32 exp = 253 - e;

	if (exp >= 0xff)
		return s | 0x7f800000;

	if (exp > 0)
		return s | exp << 23 | mant;

	if (nj)
		return s;

	const u32 full = mant | 0x800000, shift = 1 - exp, half = 1u << (shift - 1);
	const u32 rest = full & ((half << 1) - 1);
	return s | ((full >> shift) + (rest > half || (rest == half && (full >> shift) & 1)));
}

constexpr u32 ppu_vrsqrtefp(u32 x, bool nj)
{
	const u32 s = x & 0x80000000;
	s32 e = (x >> 23) & 0xff;
	u32 m = x & 0x7fffff;

	if (e == 0xff)
		return m ? x | 0x400000 : s ? 0x7fc00000 : 0;

	if (e == 0 && (nj || m == 0))
		return s | 0x7f800000;

	if (s)
		return 0x7fc00000;

	if (e == 0)
		ppu_vnormalize(e, m);

	const auto& seg = ppu_vrsqrtefp_segments[(e & 1) << 4 | m >> 19];
	const u32 key = (m >> 9) & 0x3ff;
	return (380 - e) >> 1 << 23 | (seg[0] - ((seg[1] * key) >> 2) - ((seg[2] >> (key & 7)) & 1));
}

constexpr u32 ppu_vlogefp(u32 x, bool nj)
{
	const u32 s = x & 0x80000000;
	s32 e = (x >> 23) & 0xff;
	u32 m = x & 0x7fffff;

	if (e == 0xff)
		return m ? x | 0x400000 : s ? 0x7fc00000 : x;

	if (e == 0 && (nj || m == 0))
		return 0xff800000;

	if (s)
		return 0x7fc00000;

	if (e == 0)
		ppu_vnormalize(e, m);

	const u32 key = m >> 12;
	const auto& seg = ppu_vlogefp_segments[key >> 7];
	const s32 fixed = (e - 127) * 2048 + ((seg[0] + seg[1] * (key & 0x7f)) >> 3);
	return std::bit_cast<u32>(static_cast<f32>(fixed) / 2048);
}

constexpr u32 ppu_vexptefp(u32 x, bool)
{
	const u32 s = x & 0x80000000, e = (x >> 23) & 0xff, m = x & 0x7fffff;

	if (e == 0xff)
		return m ? x | 0x400000 : s ? 0 : x;

	if (e > 150)
		return s ? 0 : 0x7f800000;

	const s64 mag = e >= 135 ? s64{m | 0x800000} << (e - 135) : e > 111 ? (m | 0x800000) >> (135 - e) : 0;
	const s64 fixed = (s ? -mag : mag) >> 4;
	const s64 exp = 127 + (fixed >> 11);

	if (exp >= 0xff)
		return 0x7f800000;

	if (exp <= 0)
		return 0;

	const auto& seg = ppu_vexptefp_segments[(fixed >> 7) & 0xf];
	return static_cast<u32>(exp) << 23 | ((seg[0] + seg[1] * (fixed & 0x7f)) >> 3) << 12;
}
