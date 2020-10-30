#pragma once

#include "../Utilities/Thread.h"
#include "../Utilities/bit_set.h"

#include <vector>

// Thread state flags
enum class cpu_flag : u32
{
	stop, // Thread not running (HLE, initial state)
	exit, // Irreversible exit
	wait, // Indicates waiting state, set by the thread itself
	temp, // Indicates that the thread cannot properly return after next check_state()
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
	static atomic_t<u64> g_threads_created, g_threads_deleted, g_suspend_counter;

	// Get thread name (as assigned to named_thread)
	std::string get_name() const;

	// Get CPU state dump (everything)
	virtual std::string dump_all() const;

	// Get CPU register dump
	virtual std::string dump_regs() const;

	// Get CPU call stack dump
	virtual std::string dump_callstack() const;

	// Get CPU call stack list
	virtual std::vector<std::pair<u32, u32>> dump_callstack_list() const;

	// Get CPU dump of misc information
	virtual std::string dump_misc() const;

	// Thread entry point function
	virtual void cpu_task() = 0;

	// Callback for cpu_flag::suspend
	virtual void cpu_sleep() {}

	// Callback for cpu_flag::ret
	virtual void cpu_return() {}

	// For internal use
	struct suspend_work
	{
		// Task priority
		s8 prio;

		// Size of prefetch list workload
		u32 prf_size;
		void* const* prf_list;

		void* func_ptr;
		void* res_buf;

		// Type-erased op executor
		void (*exec)(void* func, void* res);

		// Next object in the linked list
		suspend_work* next;

		// Internal method
		bool push(cpu_thread* _this, bool cancel_if_not_suspended = false) noexcept;
	};

	// Suspend all threads and execute op (may be executed by other thread than caller!)
	template <s8 Prio = 0, typename F>
	static auto suspend_all(cpu_thread* _this, std::initializer_list<void*> hints, F op)
	{
		if constexpr (std::is_void_v<std::invoke_result_t<F>>)
		{
			suspend_work work{Prio, ::size32(hints), hints.begin(), &op, nullptr, [](void* func, void*)
			{
				std::invoke(*static_cast<F*>(func));
			}};

			work.push(_this);
			return;
		}
		else
		{
			std::invoke_result_t<F> result;

			suspend_work work{Prio, ::size32(hints), hints.begin(), &op, &result, [](void* func, void* res_buf)
			{
				*static_cast<std::invoke_result_t<F>*>(res_buf) = std::invoke(*static_cast<F*>(func));
			}};

			work.push(_this);
			return result;
		}
	}

	// Push the workload only if threads are being suspended by suspend_all()
	template <s8 Prio = 0, typename F>
	static bool if_suspended(cpu_thread* _this, std::initializer_list<void*> hints, F op)
	{
		static_assert(std::is_void_v<std::invoke_result_t<F>>, "Unimplemented (must return void)");
		{
			suspend_work work{Prio, ::size32(hints), hints.begin(), &op, nullptr, [](void* func, void*)
			{
				std::invoke(*static_cast<F*>(func));
			}};

			return work.push(_this, true);
		}
	}

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
