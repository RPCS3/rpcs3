#pragma once
#include "util/types.hpp"

// RawSPU address space layout. Lives in this leaf header (instead of SPUThread.h)
// so that vm.h can see it without a circular include (SPUThread.h includes vm.h).
enum : u32
{
	RAW_SPU_BASE_ADDR   = 0xE0000000,
	RAW_SPU_OFFSET      = 0x00100000,
	RAW_SPU_LS_OFFSET   = 0x00000000,
	RAW_SPU_PROB_OFFSET = 0x00040000,

	SPU_FAKE_BASE_ADDR  = 0xE8000000,
};

// Forward declaration — implemented in PPUThread.cpp
// Routes guest u32 reads/writes through RawSPU MMIO when addr >= RAW_SPU_BASE_ADDR.
// Used by vm.h to intercept vm::read32 / vm::write32 for RawSPU problem-state registers.
u32  ppu_read_mmio_aware_u32(u8* vm_base, u32 addr);
void ppu_write_mmio_aware_u32(u8* vm_base, u32 addr, u32 value);
