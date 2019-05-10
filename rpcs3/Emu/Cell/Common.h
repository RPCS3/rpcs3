#pragma once

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
	union
	{
		char data[4];
		u32 data32;
		float arg;
	};

	arg = x;
	return (data32 >> 23) & 0xFF;
}
