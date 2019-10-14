#pragma once

#include "Utilities/File.h"
#include "Utilities/mutex.h"
#include "Utilities/cond.h"
#include "Utilities/JIT.h"
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
	spu_cache(const std::string& loc);

	spu_cache(spu_cache&&) noexcept = default;

	spu_cache& operator=(spu_cache&&) noexcept = default;

	~spu_cache();

	operator bool() const
	{
		return m_file.operator bool();
	}

	std::deque<std::vector<u32>> get();

	void add(const std::vector<u32>& func);

	static void initialize();
};

// Helper class
class spu_runtime
{
	mutable shared_mutex m_mutex;

	mutable cond_variable m_cond;

	mutable atomic_t<u64> m_passive_locks{0};

	atomic_t<u64> m_reset_count{0};

	struct func_compare
	{
		// Comparison function for SPU programs
		bool operator()(const std::vector<u32>& lhs, const std::vector<u32>& rhs) const;
	};

	// All functions
	std::map<std::vector<u32>, spu_function_t, func_compare> m_map;

	// All functions as PIC
	std::map<std::basic_string_view<u32>, spu_function_t> m_pic_map;

	// Debug module output location
	std::string m_cache_path;

	// Scratch vector
	std::vector<std::pair<std::basic_string_view<u32>, spu_function_t>> m_flat_list;

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

	// Add compiled function and generate trampoline if necessary
	bool add(u64 last_reset_count, void* where, spu_function_t compiled);

private:
	spu_function_t rebuild_ubertrampoline(u32 id_inst);

	friend class spu_cache;
public:

	// Return opaque pointer for add()
	void* find(u64 last_reset_count, const std::vector<u32>&);

	// Find existing function
	spu_function_t find(const u32* ls, u32 addr) const;

	// Generate a patchable trampoline to spu_recompiler_base::branch
	spu_function_t make_branch_patchpoint() const;

	// reset() arg retriever, for race avoidance (can result in double reset)
	u64 get_reset_count() const
	{
		return m_reset_count.load();
	}

	// Remove all compiled function and free JIT memory
	u64 reset(std::size_t last_reset_count);

	// Handle cpu_flag::jit_return
	void handle_return(spu_thread* _spu);

	// All dispatchers (array allocated in jit memory)
	static std::array<atomic_t<spu_function_t>, (1 << 20)>* const g_dispatcher;

	// Recompiler entry point
	static const spu_function_t g_gateway;

	// Longjmp to the end of the gateway function (native CC)
	static void(*const g_escape)(spu_thread*);

	// Similar to g_escape, but doing tail call to the new function.
	static void(*const g_tail_escape)(spu_thread*, spu_function_t, u8*);

	// Interpreter entry point
	static spu_function_t g_interpreter;

	struct passive_lock
	{
		spu_runtime& _this;

		passive_lock(const passive_lock&) = delete;

		passive_lock(spu_runtime& _this)
			: _this(_this)
		{
			std::lock_guard lock(_this.m_mutex);
			_this.m_passive_locks++;
		}

		~passive_lock()
		{
			_this.m_passive_locks--;
		}
	};

	// Exclusive lock within passive_lock scope
	struct writer_lock
	{
		spu_runtime& _this;
		bool notify = false;

		writer_lock(const writer_lock&) = delete;

		writer_lock(spu_runtime& _this)
			: _this(_this)
		{
			// Temporarily release the passive lock
			_this.m_passive_locks--;
			_this.m_mutex.lock();
		}

		~writer_lock()
		{
			_this.m_passive_locks++;
			_this.m_mutex.unlock();

			if (notify)
			{
				_this.m_cond.notify_all();
			}
		}
	};

	struct reader_lock
	{
		const spu_runtime& _this;

		reader_lock(const reader_lock&) = delete;

		reader_lock(const spu_runtime& _this)
			: _this(_this)
		{
			_this.m_passive_locks--;
			_this.m_mutex.lock_shared();
		}

		~reader_lock()
		{
			_this.m_passive_locks++;
			_this.m_mutex.unlock_shared();
		}
	};
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

protected:
	spu_runtime* m_spurt{};

	u32 m_pos;
	u32 m_size;
	u64 m_hash_start;

	// Bit indicating start of the block
	std::bitset<0x10000> m_block_info;

	// GPR modified by the instruction (-1 = not set)
	std::array<u8, 0x10000> m_regmod;

	std::array<u8, 0x10000> m_use_ra;
	std::array<u8, 0x10000> m_use_rb;
	std::array<u8, 0x10000> m_use_rc;

	// List of possible targets for the instruction (entry shouldn't exist for simple instructions)
	std::unordered_map<u32, std::basic_string<u32>, value_hash<u32, 2>> m_targets;

	// List of block predecessors
	std::unordered_map<u32, std::basic_string<u32>, value_hash<u32, 2>> m_preds;

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
		std::basic_string<u32> targets;

		// All predeccessor blocks
		std::basic_string<u32> preds;
	};

	// Sorted basic block info
	std::map<u32, block_info> m_bbs;

	// Sorted advanced block (chunk) list
	std::basic_string<u32> m_chunks;

	// Function information
	struct func_info
	{
		// Size to the end of last basic block
		u16 size = 0;

		// Determines whether a function is eligible for optimizations
		bool good = false;

		// Call targets
		std::basic_string<u32> calls;

		// Register save info (stack offset)
		std::array<u32, s_reg_max> reg_save_off{};
	};

	// Sorted function info
	std::map<u32, func_info> m_funcs;

private:
	// For private use
	std::bitset<0x10000> m_bits;

	// For private use
	std::vector<u32> workload;

	// Result of analyse(), to avoid copying and allocation
	std::vector<u32> result;

public:
	spu_recompiler_base();

	virtual ~spu_recompiler_base();

	// Initialize
	virtual void init() = 0;

	// Compile function (may fail)
	virtual spu_function_t compile(u64 last_reset_count, const std::vector<u32>&) = 0;

	// Compile function, handle failure
	void make_function(const std::vector<u32>&);

	// Default dispatch function fallback (second arg is unused)
	static void dispatch(spu_thread&, void*, u8* rip);

	// Target for the unresolved patch point (second arg is unused)
	static void branch(spu_thread&, void*, u8* rip);

	// Legacy interpreter loop
	static void old_interpreter(spu_thread&, void* ls, u8*);

	// Get the function data at specified address
	const std::vector<u32>& analyse(const be_t<u32>* ls, u32 lsa);

	// Print analyser internal state
	void dump(std::string& out);

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
};
