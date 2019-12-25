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
				data[i] = 0x7ff0'0000 | (sign << 31);
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

				data[i] = ppu_frsqrte_mantissas[idx] | exp;
			}
		}
	}
} ppu_frqrte_lut;
