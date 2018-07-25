#pragma once

#include "Utilities/File.h"
#include "SPUThread.h"
#include <vector>
#include <bitset>
#include <memory>
#include <string>

// Helper class
class spu_cache
{
	fs::file m_file;

public:
	spu_cache(const std::string& loc);

	~spu_cache();

	operator bool() const
	{
		return m_file.operator bool();
	}

	std::vector<std::vector<u32>> get();

	void add(const std::vector<u32>& func);

	static void initialize();
};

// SPU Recompiler instance base class
class spu_recompiler_base
{
protected:
	u32 m_pos;
	u32 m_size;

	// Bit indicating start of the block
	std::bitset<0x10000> m_block_info;

	// GPR modified by the instruction (-1 = not set)
	std::array<u8, 0x10000> m_regmod;

	// List of possible targets for the instruction (entry shouldn't exist for simple instructions)
	std::unordered_map<u32, std::basic_string<u32>, value_hash<u32, 2>> m_targets;

	// List of block predecessors
	std::unordered_map<u32, std::basic_string<u32>, value_hash<u32, 2>> m_preds;

	// List of function entry points and return points (set after BRSL, BRASL, BISL, BISLED)
	std::bitset<0x10000> m_entry_info;

	// Compressed address of unique entry point for each instruction
	std::array<u16, 0x10000> m_entry_map{};

	std::shared_ptr<spu_cache> m_cache;

private:
	// For private use
	std::bitset<0x10000> m_bits;

public:
	spu_recompiler_base();

	virtual ~spu_recompiler_base();

	// Initialize
	virtual void init() = 0;

	// Get pointer to the trampoline at given position
	virtual spu_function_t get(u32 lsa) = 0;

	// Compile function
	virtual spu_function_t compile(std::vector<u32>&&) = 0;

	// Default dispatch function fallback (second arg is unused)
	static void dispatch(SPUThread&, void*, u8* rip);

	// Target for the unresolved patch point (second arg is unused)
	static void branch(SPUThread&, void*, u8* rip);

	// Get the block at specified address
	std::vector<u32> block(const be_t<u32>* ls, u32 lsa);

	// Print analyser internal state
	void dump(std::string& out);

	// Create recompiler instance (ASMJIT)
	static std::unique_ptr<spu_recompiler_base> make_asmjit_recompiler();

	// Create recompiler instance (LLVM)
	static std::unique_ptr<spu_recompiler_base> make_llvm_recompiler();

	enum : u8
	{
		s_reg_lr = 0,
		s_reg_sp = 1,
		s_reg_80 = 80,
		s_reg_127 = 127,

		s_reg_mfc_eal,
		s_reg_mfc_lsa,
		s_reg_mfc_tag,
		s_reg_mfc_size,

		// Max number of registers (for m_regmod)
		s_reg_max
	};
};
