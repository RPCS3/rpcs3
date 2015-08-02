#pragma once

#include "Utilities/Thread.h"

namespace vm
{
	class waiter_lock_t;
}

enum CPUThreadType
{
	CPU_THREAD_PPU,
	CPU_THREAD_SPU,
	CPU_THREAD_RAW_SPU,
	CPU_THREAD_ARMv7,
};

// CPU Thread State Flags
enum : u64
{
	CPU_STATE_STOPPED = (1ull << 0), // basic execution state (stopped by default), removed by Exec()
	CPU_STATE_PAUSED  = (1ull << 1), // pauses thread execution, set by the debugger (manually or after step execution)
	CPU_STATE_SLEEP   = (1ull << 2), // shouldn't affect thread execution, set by Sleep() call, removed by the latest Awake() call, may possibly indicate waiting state of the thread
	CPU_STATE_STEP    = (1ull << 3), // forces the thread to pause after executing just one instruction or something appropriate, set by the debugger
	CPU_STATE_DEAD    = (1ull << 4), // indicates irreversible exit of the thread
	CPU_STATE_RETURN  = (1ull << 5), // used for callback return
	CPU_STATE_SIGNAL  = (1ull << 6), // used for HLE signaling
	CPU_STATE_INTR    = (1ull << 7), // thread interrupted

	CPU_STATE_MAX     = (1ull << 8), // added to (subtracted from) m_state by Sleep()/Awake() calls to trigger status check
};

// "HLE return" exception event
class CPUThreadReturn{};

// CPUThread::Stop exception event
class CPUThreadStop{};

// CPUThread::Exit exception event
class CPUThreadExit{};

class CPUDecoder;

class CPUThread : protected thread_t, public std::enable_shared_from_this<CPUThread>
{
protected:
	atomic_t<u64> m_state; // thread state flags

	std::unique_ptr<CPUDecoder> m_dec;

	const u32 m_id;
	const CPUThreadType m_type;
	const std::string m_name; // changing m_name would be terribly thread-unsafe in current implementation

public:
	using thread_t::mutex;
	using thread_t::cv;
	using thread_t::is_current;
	using thread_t::get_thread_ctrl;

	friend vm::waiter_lock_t;

protected:
	CPUThread(CPUThreadType type, const std::string& name, std::function<std::string()> thread_name);

public:
	virtual ~CPUThread() override;

	u32 get_id() const { return m_id; }
	CPUThreadType get_type() const { return m_type; }
	std::string get_name() const { return m_name; }

	bool is_alive() const { return (m_state.load() & CPU_STATE_DEAD) == 0; }
	bool is_stopped() const { return (m_state.load() & CPU_STATE_STOPPED) != 0; }
	virtual bool is_paused() const;

	virtual void dump_info() const;
	virtual u32 get_pc() const = 0;
	virtual u32 get_offset() const = 0;
	virtual void do_run() = 0;
	virtual void task() = 0;

	virtual void init_regs() = 0;
	virtual void init_stack() = 0;
	virtual void close_stack() = 0;

	// initialize thread
	void run();

	// called by the debugger, don't use
	void pause();

	// called by the debugger, don't use
	void resume();

	// stop thread execution
	void stop();

	// start thread execution (removing STOP status)
	void exec();

	// exit thread execution
	void exit();

	// called by the debugger, don't use
	void step();

	// trigger thread status check
	void sleep();

	// untrigger thread status check
	void awake();

	// set SIGNAL and notify (returns true if set)
	bool signal();

	// test SIGNAL and reset
	bool unsignal();

	// process m_state flags, returns true if the checker must return
	bool check_status();

	std::string GetFName() const
	{
		return fmt::format("%s[0x%x] Thread (%s)", GetTypeString(), m_id, m_name);
	}

	static const char* CPUThreadTypeToString(CPUThreadType type)
	{
		switch (type)
		{
		case CPU_THREAD_PPU: return "PPU";
		case CPU_THREAD_SPU: return "SPU";
		case CPU_THREAD_RAW_SPU: return "RawSPU";
		case CPU_THREAD_ARMv7: return "ARMv7";
		}

		return "Unknown";
	}

	const char* ThreadStatusToString()
	{
		// TODO

		//switch (ThreadStatus())
		//{
		//case CPUThread_Ready: return "Ready";
		//case CPUThread_Running: return "Running";
		//case CPUThread_Paused: return "Paused";
		//case CPUThread_Stopped: return "Stopped";
		//case CPUThread_Sleeping: return "Sleeping";
		//case CPUThread_Break: return "Break";
		//case CPUThread_Step: return "Step";
		//}

		return "Unknown";
	}

	const char* GetTypeString() const
	{
		return CPUThreadTypeToString(m_type);
	}

	CPUDecoder* GetDecoder()
	{
		return m_dec.get();
	};

	virtual std::string RegsToString() const = 0;
	virtual std::string ReadRegString(const std::string& reg) const = 0;
	virtual bool WriteRegString(const std::string& reg, std::string value) = 0;
};

class cpu_thread
{
protected:
	std::shared_ptr<CPUThread> thread;

public:
	//u32 get_entry() const
	//{
	//	return thread->entry;
	//}

	virtual cpu_thread& args(std::initializer_list<std::string> values) = 0;

	virtual cpu_thread& run() = 0;

	//u64 join()
	//{
	//	if (!joinable())
	//		throw EXCEPTION("thread must be joinable for join");

	//	thread->SetJoinable(false);

	//	while (thread->IsRunning())
	//		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack

	//	return thread->GetExitStatus();
	//}

	//bool joinable() const
	//{
	//	return thread->IsJoinable();
	//}

	//u32 get_id() const
	//{
	//	return thread->GetId();
	//}
};
