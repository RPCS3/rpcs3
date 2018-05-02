#pragma once

#include "SPUThread.h"
#include <bitset>

// SPU Recompiler instance base class
class spu_recompiler_base
{
protected:
	SPUThread& m_spu;

	u32 m_pos;

	std::bitset<0x10000> m_block_info;

public:
	spu_recompiler_base(SPUThread& spu);

	virtual ~spu_recompiler_base();

	// Get pointer to the trampoline at given position
	virtual spu_function_t get(u32 lsa) = 0;

	// Compile function
	virtual spu_function_t compile(const std::vector<u32>& func) = 0;

	// Default dispatch function fallback (second pointer is unused)
	static void dispatch(SPUThread&, void*, u8*);

	// Direct branch fallback for non-compiled destination
	static void branch(SPUThread&, void*, u8*);

	// Get the block at specified address
	static std::vector<u32> block(SPUThread&, u32 lsa, std::bitset<0x10000>* = nullptr);

	// Create recompiler instance (ASMJIT)
	static std::unique_ptr<spu_recompiler_base> make_asmjit_recompiler(SPUThread& spu);

	// Create recompiler instance (LLVM)
	static std::unique_ptr<spu_recompiler_base> make_llvm_recompiler(SPUThread& spu);
};
