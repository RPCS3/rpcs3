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

	// All functions
	std::map<std::vector<u32>, spu_function_t> m_map;

	// Debug module output location
	std::string m_cache_path;

	// Trampoline generation workload helper
	struct work
	{
		u32 size;
		u16 from;
		u16 level;
		u8* rel32;
		std::map<std::vector<u32>, spu_function_t>::iterator beg;
		std::map<std::vector<u32>, spu_function_t>::iterator end;
	};

	// Scratch vector
	static thread_local std::vector<work> workload;

	// Scratch vector
	static thread_local std::vector<u32> addrv;

	// Trampoline to spu_recompiler_base::dispatch
	static const spu_function_t tr_dispatch;

	// Trampoline to spu_recompiler_base::branch
	static const spu_function_t tr_branch;

public:
	spu_runtime();

	const std::string& get_cache_path() const
	{
		return m_cache_path;
	}

	// Add compiled function and generate trampoline if necessary
	bool add(u64 last_reset_count, void* where, spu_function_t compiled);

	// Return opaque pointer for add()
	void* find(u64 last_reset_count, const std::vector<u32>&);

	// Find existing function
	spu_function_t find(const se_t<u32, false>* ls, u32 addr) const;

	// Generate a patchable trampoline to spu_recompiler_base::branch
	spu_function_t make_branch_patchpoint(u32 target) const;

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
	static atomic_t<spu_function_t>* const g_dispatcher;

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
protected:
	std::shared_ptr<spu_runtime> m_spurt;

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

	// Result of analyse(), to avoid copying and allocation
	std::vector<u32> result;

public:
	spu_recompiler_base();

	virtual ~spu_recompiler_base();

	// Initialize
	virtual void init() = 0;

	// Compile function (may fail)
	virtual bool compile(u64 last_reset_count, const std::vector<u32>&) = 0;

	// Compile function, handle failure
	void make_function(const std::vector<u32>&);

	// Default dispatch function fallback (second arg is unused)
	static void dispatch(spu_thread&, void*, u8* rip);

	// Target for the unresolved patch point (second arg is unused)
	static void branch(spu_thread&, void*, u8* rip);

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
