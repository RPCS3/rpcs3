#pragma once

#include "../Utilities/Thread.h"
#include "../Utilities/bit_set.h"

// Thread state flags
enum class cpu_flag : u32
{
	stop, // Thread not running (HLE, initial state)
	exit, // Irreversible exit
	wait, // Indicates waiting state, set by the thread itself
	pause, // Thread suspended by suspend_all technique
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

class cpu_thread
{
	// PPU cache backward compatibility hack
	char dummy[sizeof(std::shared_ptr<void>) - 8];

public:
	u64 block_hash = 0;

protected:
	cpu_thread(u32 id);

public:
	virtual ~cpu_thread();
	void operator()();

	// Self identifier
	const u32 id;

	// Public thread state
	atomic_bs_t<cpu_flag> state{cpu_flag::stop + cpu_flag::wait};

	// Process thread state, return true if the checker must return
	bool check_state() noexcept;

	// Process thread state (pause)
	[[nodiscard]] bool test_stopped()
	{
		if (state)
		{
			if (check_state())
			{
				return true;
			}
		}

		return false;
	}

	// Test stopped state
	bool is_stopped() const
	{
		return !!(state & (cpu_flag::stop + cpu_flag::exit + cpu_flag::dbg_global_stop));
	}

	// Test paused state
	bool is_paused() const
	{
		return !!(state & (cpu_flag::suspend + cpu_flag::dbg_global_pause + cpu_flag::dbg_pause));
	}

	// Check thread type
	u32 id_type() const
	{
		return id >> 24;
	}

	void notify();

private:
	void abort();

public:
	// Thread stats for external observation
	static atomic_t<u64> g_threads_created, g_threads_deleted;

	// Get thread name (as assigned to named_thread)
	std::string get_name() const;

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

	// Thread locker
	class suspend_all
	{
		cpu_thread* m_this;

	public:
		suspend_all(cpu_thread* _this) noexcept;
		suspend_all(const suspend_all&) = delete;
		suspend_all& operator=(const suspend_all&) = delete;
		~suspend_all();
	};

	// Stop all threads with cpu_flag::dbg_global_stop
	static void stop_all() noexcept;

	// Send signal to the profiler(s) to flush results
	static void flush_profilers() noexcept;
};

inline cpu_thread* get_current_cpu_thread() noexcept
{
	extern thread_local cpu_thread* g_tls_current_cpu_thread;

	return g_tls_current_cpu_thread;
}

class ppu_thread;
class spu_thread;
