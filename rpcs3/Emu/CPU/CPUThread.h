#pragma once

#include "../Utilities/Thread.h"
#include "../Utilities/BitSet.h"

// CPU Thread Type
enum class cpu_type : u8
{
	ppu, // PPU Thread
	spu, // SPU Thread
	arm, // ARMv7 Thread
};

// CPU Thread State flags
enum struct cpu_state : u16
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
constexpr bitset_t<cpu_state> cpu_state_pause = make_bitset(cpu_state::suspend, cpu_state::dbg_global_pause, cpu_state::dbg_pause);

class cpu_thread : public named_thread
{
	void on_task() override;

public:
	virtual void on_stop() override;
	virtual ~cpu_thread() override;

	const id_value<> id{};
	const cpu_type type;

	cpu_thread(cpu_type type);

	// Public recursive sleep state counter
	atomic_t<u8> sleep_counter{};

	// Public thread state
	atomic_t<bitset_t<cpu_state>> state{ cpu_state::stop };

	// Object associated with sleep state, possibly synchronization primitive (mutex, semaphore, etc.)
	atomic_t<void*> owner{};

	// Process thread state, return true if the checker must return
	bool check_status();

	// Increse sleep counter
	void sleep()
	{
		if (!sleep_counter++) return; //handle_interrupt();
	}

	// Decrese sleep counter
	void awake()
	{
		if (!--sleep_counter) owner = nullptr;
	}

	// Print CPU state
	virtual std::string dump() const = 0;
	virtual void cpu_init() {}
	virtual void cpu_task() = 0;
	virtual bool handle_interrupt() { return false; }
};

inline cpu_thread* get_current_cpu_thread() noexcept
{
	extern thread_local cpu_thread* g_tls_current_cpu_thread;

	return g_tls_current_cpu_thread;
}

// Helper for cpu_thread.
// 1) Calls sleep() and locks the thread in the constructor.
// 2) Calls awake() and unlocks the thread in the destructor.
class cpu_thread_lock final
{
	cpu_thread& m_thread;

public:
	cpu_thread_lock(const cpu_thread_lock&) = delete;

	cpu_thread_lock(cpu_thread& thread)
		: m_thread(thread)
	{
		m_thread.sleep();
		m_thread->lock();
	}

	~cpu_thread_lock()
	{
		m_thread.awake();
		m_thread->unlock();
	}
};
