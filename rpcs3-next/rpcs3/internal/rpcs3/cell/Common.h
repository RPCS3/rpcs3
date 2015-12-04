#pragma once

// Floating-point rounding mode (for both PPU and SPU)
enum FPSCR_RN
{
	FPSCR_RN_NEAR = 0,
	FPSCR_RN_ZERO = 1,
	FPSCR_RN_PINF = 2,
	FPSCR_RN_MINF = 3,
};

using ppu_inter_func_t = void(*)(class PPUThread& CPU, union ppu_opcode_t opcode);

struct ppu_decoder_cache_t
{
	ppu_inter_func_t* const pointer;

	ppu_decoder_cache_t();

	~ppu_decoder_cache_t();

	void initialize(u32 addr, u32 size);
};
