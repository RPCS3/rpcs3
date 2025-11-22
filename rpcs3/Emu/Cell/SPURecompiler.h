#pragma once

#include "Utilities/File.h"
#include "Utilities/lockless.h"
#include "Utilities/address_range.h"
#include "SPUThread.h"
#include <vector>
#include <bitset>
#include <memory>
#include <string>
#include <deque>

// Helper class
class spu_cache
{
	fs::file m_file;

public:
	spu_cache() = default;

	spu_cache(const std::string& loc);

	spu_cache(spu_cache&&) noexcept = default;

	spu_cache& operator=(spu_cache&&) noexcept = default;

	~spu_cache();

	operator bool() const
	{
		return m_file.operator bool();
	}

	std::deque<struct spu_program> get();

	void add(const struct spu_program& func);

	static void initialize(bool build_existing_cache = true);

	struct precompile_data_t
	{
		u32 vaddr;
		std::vector<u32> inst_data;
		std::vector<u32> funcs;
	};

	bool collect_funcs_to_precompile = true;

	lf_queue<precompile_data_t> precompile_funcs;
};

struct spu_program
{
	// Address of the entry point in LS
	u32 entry_point;

	// Address of the data in LS
	u32 lower_bound;

	// Program data with intentionally wrong endianness (on LE platform opcode values are swapped)
	std::vector<u32> data;

	bool operator==(const spu_program& rhs) const noexcept;

	bool operator<(const spu_program& rhs) const noexcept;
};

class spu_item
{
public:
	// SPU program
	const spu_program data;

	// Compiled function pointer
	atomic_t<spu_function_t> compiled = nullptr;

	// Ubertrampoline generated for this item when it was latest
	atomic_t<spu_function_t> trampoline = nullptr;

	atomic_t<u8> cached = false;
	atomic_t<u8> logged = false;

	spu_item(spu_program&& data)
		: data(std::move(data))
	{
	}

	spu_item(const spu_item&) = delete;

	spu_item& operator=(const spu_item&) = delete;
};

// Helper class
class spu_runtime
{
	// All functions (2^20 bunches)
	std::array<lf_bunch<spu_item>, (1 << 20)> m_stuff;

	// Debug module output location
	std::string m_cache_path;

public:
	// Trampoline to spu_recompiler_base::dispatch
	static const spu_function_t tr_dispatch;

	// Trampoline to spu_recompiler_base::branch
	static const spu_function_t tr_branch;

	// Trampoline to legacy interpreter
	static const spu_function_t tr_interpreter;

	// Detect and call any recompiled function
	static const spu_function_t tr_all;

public:
	spu_runtime();

	spu_runtime(const spu_runtime&) = delete;

	spu_runtime& operator=(const spu_runtime&) = delete;

	const std::string& get_cache_path() const
	{
		return m_cache_path;
	}

	// Rebuild ubertrampoline for given identifier (first instruction)
	spu_function_t rebuild_ubertrampoline(u32 id_inst);

private:
	friend class spu_cache;

public:
	// Return new pointer for add()
	spu_item* add_empty(spu_program&&);

	// Find existing function
	spu_function_t find(const u32* ls, u32 addr) const;

	// Generate a patchable trampoline to spu_recompiler_base::branch
	spu_function_t make_branch_patchpoint(u16 data = 0) const;

	// All dispatchers (array allocated in jit memory)
	static std::array<atomic_t<spu_function_t>, (1 << 20)>* const g_dispatcher;

	// Recompiler entry point
	static const spu_function_t g_gateway;

	// Longjmp to the end of the gateway function (native CC)
	static void(*const g_escape)(spu_thread*);

	// Similar to g_escape, but doing tail call to the new function.
	static void(*const g_tail_escape)(spu_thread*, spu_function_t, u8*);

	// Interpreter table (spu_itype -> ptr)
	static std::array<u64, 256> g_interpreter_table;

	// Interpreter entry point
	static spu_function_t g_interpreter;
};

// SPU Recompiler instance base class
class spu_recompiler_base
{
public:
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

	// Classify terminator instructions
	enum class term_type : unsigned char
	{
		br,
		ret,
		call,
		fallthrough,
		indirect_call,
		interrupt_call,
	};

	// Value flags (TODO: only is_const is implemented)
	enum class vf : u32
	{
		is_const,
		is_mask,
		is_rel,
		is_null,

		__bitset_enum_max
	};

	struct reg_state_t
	{
		bs_t<vf> flag{+vf::is_null};
		u32 value{};
		u32 tag = umax;
		u32 known_ones{};
		u32 known_zeroes{};
		u32 origin = SPU_LS_SIZE;
		bool is_instruction = false;

		bool is_const() const;

		bool operator&(vf to_test) const;

		bool is_less_than(u32 imm) const;
		bool operator==(const reg_state_t& r) const;
		bool operator==(u32 imm) const;

		// Compare equality but try to ignore changes in unmasked bits
		bool compare_with_mask_indifference(const reg_state_t& r, u32 mask_bits) const;
		bool compare_with_mask_indifference(u32 imm, u32 mask_bits) const;
		bool unequal_with_mask_indifference(const reg_state_t& r, u32 mask_bits) const;

		// Convert constant-based value to mask-based value
		reg_state_t downgrade() const;

		// Connect two register states between different blocks
		reg_state_t merge(const reg_state_t& rhs, u32 current_pc) const;

		// Override value with newer value if needed
		reg_state_t build_on_top_of(const reg_state_t& rhs) const;

		// Get known zeroes mask
		u32 get_known_zeroes() const;

		// Get known ones mask
		u32 get_known_ones() const;

		// Invalidate value if non-constant and reached the point in history of its creation
		void invalidate_if_created(u32 current_pc);

		template <usz Count = 1>
		static std::conditional_t<Count == 1, reg_state_t, std::array<reg_state_t, Count>> make_unknown(u32 pc, u32 current_pc = SPU_LS_SIZE) noexcept
		{
			if constexpr (Count == 1)
			{
				reg_state_t v{};
				v.tag = alloc_tag();
				v.flag = {};
				v.origin = pc;
				v.is_instruction = pc == current_pc;
				return v;
			}
			else
			{
				std::array<reg_state_t, Count> result{};

				for (reg_state_t& state : result)
				{
					state = make_unknown<1>(pc, current_pc);
				}

				return result;
			}
		}

		bool compare_tags(const reg_state_t& rhs) const;

		static reg_state_t from_value(u32 value) noexcept;
		static u32 alloc_tag(bool reset = false) noexcept;
	};

protected:
	spu_runtime* m_spurt{};

	u32 m_pos;
	u32 m_size;
	u64 m_hash_start;

	// Bit indicating start of the block
	std::bitset<0x10000> m_block_info;

	// GPR modified by the instruction (-1 = not set)
	std::array<u8, 0x10000> m_regmod;

	std::bitset<0x10000> m_use_ra;
	std::bitset<0x10000> m_use_rb;
	std::bitset<0x10000> m_use_rc;

	// List of possible targets for the instruction (entry shouldn't exist for simple instructions)
	std::unordered_map<u32, std::vector<u32>, value_hash<u32, 2>> m_targets;

	// List of block predecessors
	std::unordered_map<u32, std::vector<u32>, value_hash<u32, 2>> m_preds;

	// List of function entry points and return points (set after BRSL, BRASL, BISL, BISLED)
	std::bitset<0x10000> m_entry_info;

	// Set after return points and disjoint chunks
	std::bitset<0x10000> m_ret_info;

	// Basic block information
	struct block_info
	{
		// Address of the chunk entry point (chunk this block belongs to)
		u32 chunk = 0x40000;

		// Number of instructions
		u16 size = 0;

		// Internal use flag
		bool analysed = false;

		// Terminator instruction type
		term_type terminator;

		// Bit mask of the registers modified in the block
		std::bitset<s_reg_max> reg_mod{};

		// Set if last modifying instruction produces xfloat
		std::bitset<s_reg_max> reg_mod_xf{};

		// Set if the initial register value in this block may be xfloat
		std::bitset<s_reg_max> reg_maybe_xf{};

		// Bit mask of the registers used (before modified)
		std::bitset<s_reg_max> reg_use{};

		// Bit mask of the trivial (u32 x 4) constant value resulting in this block
		std::bitset<s_reg_max> reg_const{};

		// Bit mask of register saved onto the stack before use
		std::bitset<s_reg_max> reg_save_dom{};

		// Address of the function
		u32 func = 0x40000;

		// Value subtracted from $SP in this block, negative if something funny is done on $SP
		u32 stack_sub = 0;

		// Constant values associated with reg_const
		std::array<u32, s_reg_max> reg_val32;

		// Registers loaded from the stack in this block (stack offset)
		std::array<u32, s_reg_max> reg_load_mod{};

		// Single source of the reg value (dominating block address within the same chunk) or a negative number
		std::array<u32, s_reg_max> reg_origin, reg_origin_abs;

		// All possible successor blocks
		std::vector<u32> targets;

		// All predeccessor blocks
		std::vector<u32> preds;
	};

	// Sorted basic block info
	std::map<u32, block_info> m_bbs;

	// Sorted advanced block (chunk) list
	std::vector<u32> m_chunks;

	// Function information
	struct func_info
	{
		// Size to the end of last basic block
		u16 size = 0;

		// Determines whether a function is eligible for optimizations
		bool good = false;

		// Call targets
		std::vector<u32> calls;

		// Register save info (stack offset)
		std::array<u32, s_reg_max> reg_save_off{};
	};

	// Sorted function info
	std::map<u32, func_info> m_funcs;

	// TODO: Add patterns
	// Not a bitset to allow more possibilities
	enum class inst_attr : u8
	{
		none,
		omit,
		putllc16,
		putllc0,
		rchcnt_loop,
	};

	std::vector<inst_attr> m_inst_attrs;

	struct pattern_info
	{
		u64 info;
	};

	std::unordered_map<u32, pattern_info> m_patterns;

	void add_pattern(bool fill_all, inst_attr attr, u32 start, u64 info);

private:
	// For private use
	std::bitset<0x10000> m_bits;

	// For private use
	std::vector<u32> workload;

public:
	spu_recompiler_base();

	virtual ~spu_recompiler_base();

	// Initialize
	virtual void init() = 0;

	// Compile function
	virtual spu_function_t compile(spu_program&&) = 0;

	// Default dispatch function fallback (second arg is unused)
	static void dispatch(spu_thread&, void*, u8* rip);

	// Target for the unresolved patch point (second arg is unused)
	static void branch(spu_thread&, void*, u8* rip);

	// Legacy interpreter loop
	static void old_interpreter(spu_thread&, void* ls, u8*);

	// Get the function data at specified address
	spu_program analyse(const be_t<u32>* ls, u32 entry_point, std::map<u32, std::vector<u32>>* out_target_list = nullptr);

	// Print analyser internal state
	void dump(const spu_program& result, std::string& out);

	// Get SPU Runtime
	spu_runtime& get_runtime()
	{
		if (!m_spurt)
		{
			init();
		}

		return *m_spurt;
	}

	// Create recompiler instance (ASMJIT)
	static std::unique_ptr<spu_recompiler_base> make_asmjit_recompiler();

	// Create recompiler instance (LLVM)
	static std::unique_ptr<spu_recompiler_base> make_llvm_recompiler(u8 magn = 0);

	// Create recompiler instance (interpreter-based LLVM)
	static std::unique_ptr<spu_recompiler_base> make_fast_llvm_recompiler();
};
