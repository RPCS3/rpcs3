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
