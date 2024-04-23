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
