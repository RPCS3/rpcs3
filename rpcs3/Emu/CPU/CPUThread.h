#pragma once

#include "Utilities/Thread.h"

// CPU Thread Type
enum class cpu_type : u32
{
	ppu, // PPU Thread
	spu, // SPU Thread
	arm, // ARMv7 Thread
};

// CPU Thread State flags
enum struct cpu_state : u32
{
	stop, // Thread not running (HLE, initial state)
	exit, // Irreversible exit
	suspend, // Thread paused
	ret, // Callback return requested
	signal, // Thread received a signal (HLE)
	interrupt, // Thread interrupted

	dbg_global_pause, // Emulation paused
	dbg_global_stop, // Emulation stopped
	dbg_pause, // Thread paused
	dbg_step, // Thread forced to pause after one step (one instruction, etc)
};

// CPU Thread State flags: pause state union
constexpr mset<cpu_state> cpu_state_pause = to_mset(cpu_state::suspend, cpu_state::dbg_global_pause, cpu_state::dbg_pause);

class cpu_thread : public named_thread
{
	void on_task() override;

public:
	virtual void on_stop() override;
	virtual ~cpu_thread() override;

	const std::string name;
	const u32 id = -1;
	const cpu_type type;

	cpu_thread(cpu_type type, const std::string& name);

	// Public thread state
	atomic_t<mset<cpu_state>> state{ cpu_state::stop };

	// Recursively enter sleep state
	void sleep()
	{
		if (!++m_sleep) xsleep();
	}

	// Leave sleep state
	void awake()
	{
		if (!m_sleep--) xsleep();
	}

	// Process thread state, return true if the checker must return
	bool check_status();

	virtual std::string dump() const = 0; // Print CPU state
	virtual void cpu_init() {}
	virtual void cpu_task() = 0;
	virtual bool handle_interrupt() { return false; }

private:
	[[noreturn]] void xsleep();

	// Sleep/Awake counter
	atomic_t<u32> m_sleep{};
};

extern std::mutex& get_current_thread_mutex();
extern std::condition_variable& get_current_thread_cv();

inline cpu_thread* get_current_cpu_thread() noexcept
{
	extern thread_local cpu_thread* g_tls_current_cpu_thread;

	return g_tls_current_cpu_thread;
}

extern std::vector<std::shared_ptr<cpu_thread>> get_all_cpu_threads();
