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

	dbg_global_pause, // Emulation paused
	dbg_global_stop, // Emulation stopped
	dbg_pause, // Thread paused
	dbg_step, // Thread forced to pause after one step (one instruction, etc)

	__bitset_enum_max
};

// Flag set for pause state
constexpr bs_t<cpu_flag> cpu_state_pause = cpu_flag::suspend + cpu_flag::dbg_global_pause + cpu_flag::dbg_pause;

class cpu_thread : public named_thread
{
	void on_task() override final;

public:
	virtual void on_stop() override;
	virtual ~cpu_thread() override;

	const u32 id;

	cpu_thread(u32 id);

	// Public thread state
	atomic_t<bs_t<cpu_flag>> state{+cpu_flag::stop};

	// Process thread state, return true if the checker must return
	bool check_state();

	// Process thread state
	void test_state();

	// Run thread
	void run();

	// Check thread type
	u32 id_type()
	{
		return id >> 24;
	}

	// Thread stats for external observation
	static atomic_t<u64> g_threads_created, g_threads_deleted;

	// Print CPU state
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
