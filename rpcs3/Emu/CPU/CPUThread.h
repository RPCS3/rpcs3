#pragma once

#include "../Utilities/Thread.h"
#include "../Utilities/bit_set.h"

// Thread state flags
enum class cpu_flag : u32
{
	stop, // Thread not running (HLE, initial state)
	exit, // Irreversible exit
	suspend, // Thread suspended
	ret, // Callback return requested
	signal, // Thread received a signal (HLE)
	memory, // Thread must unlock memory mutex

	jit_return, // JIT compiler event (forced return)
	dbg_global_pause, // Emulation paused
	dbg_global_stop, // Emulation stopped
	dbg_pause, // Thread paused
	dbg_step, // Thread forced to pause after one step (one instruction, etc)

	__bitset_enum_max
};

class cpu_thread
{
	// PPU cache backward compatibility hack
	char dummy[sizeof(std::shared_ptr<void>)];

protected:
	cpu_thread(u32 id);

public:
	virtual ~cpu_thread();
	void operator()();
	void on_abort();

	// Self identifier
	const u32 id;

	// Public thread state
	atomic_bs_t<cpu_flag> state{+cpu_flag::stop};

	// Process thread state, return true if the checker must return
	bool check_state();

	// Process thread state (pause)
	[[nodiscard]] bool test_stopped()
	{
		if (UNLIKELY(state))
		{
			if (check_state())
			{
				return true;
			}
		}

		return false;
	}

	// Test stopped state
	bool is_stopped()
	{
		return !!(state & (cpu_flag::stop + cpu_flag::exit + cpu_flag::jit_return + cpu_flag::dbg_global_stop));
	}

	// Test paused state
	bool is_paused()
	{
		return !!(state & (cpu_flag::suspend + cpu_flag::dbg_global_pause + cpu_flag::dbg_pause));
	}

	// Check thread type
	u32 id_type()
	{
		return id >> 24;
	}

	// Upcast and notify
	void notify();

	// Thread stats for external observation
	static atomic_t<u64> g_threads_created, g_threads_deleted;

	// Get thread name
	virtual std::string get_name() const = 0;

	// Get CPU state dump
	virtual std::string dump() const;

	// Thread entry point function
	virtual void cpu_task() = 0;

	// Callback for cpu_flag::suspend
	virtual void cpu_sleep() {}

	// Callback for cpu_flag::memory
	virtual void cpu_mem() {}

	// Callback for vm::temporary_unlock
	virtual void cpu_unmem() {}
};

inline cpu_thread* get_current_cpu_thread() noexcept
{
	extern thread_local cpu_thread* g_tls_current_cpu_thread;

	return g_tls_current_cpu_thread;
}

class ppu_thread;
class spu_thread;
