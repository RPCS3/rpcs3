#pragma once
#include "Utilities/Thread.h"

enum CPUThreadType : unsigned char
{
	CPU_THREAD_PPU,
	CPU_THREAD_SPU,
	CPU_THREAD_RAW_SPU,
	CPU_THREAD_ARMv7,
};

enum CPUThreadStatus
{
	CPUThread_Ready,
	CPUThread_Running,
	CPUThread_Paused,
	CPUThread_Stopped,
	CPUThread_Sleeping,
	CPUThread_Break,
	CPUThread_Step,
};

// CPU Thread Events
enum : u64
{
	CPU_EVENT_STOP = (1ull << 0),
};

class CPUDecoder;

class CPUThread : public ThreadBase
{
protected:
	std::atomic<u64> m_events; // flags

	u32 m_status;
	u32 m_id;
	u64 m_prio;
	CPUThreadType m_type;
	bool m_joinable;
	bool m_joining;
	bool m_is_step;

	u32 m_stack_addr;
	u32 m_stack_size;

	u64 m_exit_status;

	CPUDecoder* m_dec;

	bool m_trace_call_stack;

	virtual void DumpInformation() override;

public:
	void AddEvent(const u64 event) { m_events |= event; }

	virtual void InitRegs() = 0;

	virtual void InitStack() = 0;
	virtual void CloseStack() = 0;

	u32 GetStackAddr() const { return m_stack_addr; }
	u32 GetStackSize() const { return m_stack_size; }

	void SetStackAddr(u32 stack_addr) { m_stack_addr = stack_addr; }
	void SetStackSize(u32 stack_size) { m_stack_size = stack_size; }

	void SetId(const u32 id);
	void SetName(const std::string& name);
	void SetPrio(const u64 prio) { m_prio = prio; }
	void SetExitStatus(const u64 status) { m_exit_status = status; }

	u64 GetPrio() const { return m_prio; }
	u64 GetExitStatus() const { return m_exit_status; }

	std::string GetName() const { return NamedThreadBase::GetThreadName(); }
	std::string GetFName() const
	{
		return fmt::format("%s[0x%x] Thread (%s)", GetTypeString(), m_id, GetName());
	}

	static std::string CPUThreadTypeToString(CPUThreadType type)
	{
		switch(type)
		{
		case CPU_THREAD_PPU: return "PPU";
		case CPU_THREAD_SPU: return "SPU";
		case CPU_THREAD_RAW_SPU: return "RawSPU";
		case CPU_THREAD_ARMv7: return "ARMv7";
		}

		return "Unknown";
	}

	std::string ThreadStatusToString()
	{
		switch (ThreadStatus())
		{
		case CPUThread_Ready: return "Ready";
		case CPUThread_Running: return "Running";
		case CPUThread_Paused: return "Paused";
		case CPUThread_Stopped: return "Stopped";
		case CPUThread_Sleeping: return "Sleeping";
		case CPUThread_Break: return "Break";
		case CPUThread_Step: return "Step";

		default: return "Unknown status";
		}
	}

	std::string GetTypeString() const { return CPUThreadTypeToString(m_type); }

	virtual std::string GetThreadName() const
	{
		return fmt::format("%s[0x%08x]", GetFName(), PC);
	}

	CPUDecoder * GetDecoder() { return m_dec; };

public:
	u32 entry;
	u32 PC;
	u32 nPC;
	u32 index;
	u32 offset;
	bool m_is_branch;
	bool m_trace_enabled;
	u64 m_last_syscall;

protected:
	CPUThread(CPUThreadType type);

public:
	virtual ~CPUThread();

	int ThreadStatus();

	void NextPc(u32 instr_size);
	void SetBranch(const u32 pc, bool record_branch = false);
	void SetPc(const u32 pc);
	void SetEntry(const u32 entry);

	bool IsRunning() const;
	bool IsPaused() const;
	bool IsStopped() const;

	bool IsJoinable() const { return m_joinable; }
	bool IsJoining() const { return m_joining; }
	void SetJoinable(bool joinable) { m_joinable = joinable; }
	void SetJoining(bool joining) { m_joining = joining; }

	u32 GetId() const { return m_id; }
	CPUThreadType GetType()	const { return m_type; }

	void SetCallStackTracing(bool trace_call_stack) { m_trace_call_stack = trace_call_stack; }

	void Reset();
	void Close();
	void Run();
	void Pause();
	void Resume();
	void Stop();

	virtual std::string RegsToString() = 0;
	virtual std::string ReadRegString(const std::string& reg) = 0;
	virtual bool WriteRegString(const std::string& reg, std::string value) = 0;

	virtual void Exec();
	void ExecOnce();

	struct CallStackItem
	{
		u32 pc;
		u32 branch_pc;
	};

	std::vector<CallStackItem> m_call_stack;

	std::string CallStackToString()
	{
		std::string ret = "Call Stack:\n==========\n";

		for(uint i=0; i<m_call_stack.size(); ++i)
		{
			ret += fmt::Format("0x%x -> 0x%x\n", m_call_stack[i].pc, m_call_stack[i].branch_pc);
		}

		return ret;
	}

	void CallStackBranch(u32 pc)
	{
		//look if we're jumping back and if so pop the stack back to that position
		auto res = std::find_if(m_call_stack.rbegin(), m_call_stack.rend(),
			[&pc, this](CallStackItem &it)
			{
				return CallStackGetNextPC(it.pc) == pc;
			});
		if (res != m_call_stack.rend())
		{
			m_call_stack.erase((res + 1).base(), m_call_stack.end());
			return;
		}

		//add a new entry otherwise
		CallStackItem new_item;

		new_item.branch_pc = pc;
		new_item.pc = PC;

		m_call_stack.push_back(new_item);
	}

	virtual u32 CallStackGetNextPC(u32 pc)
	{
		return pc + 4;
	}

protected:
	virtual void DoReset()=0;
	virtual void DoRun()=0;
	virtual void DoPause()=0;
	virtual void DoResume()=0;
	virtual void DoStop()=0;

protected:
	virtual void Step() {}
	virtual void Task();
};

CPUThread* GetCurrentCPUThread();

class cpu_thread
{
protected:
	std::shared_ptr<CPUThread> thread;

public:
	u32 get_entry() const
	{
		return thread->entry;
	}

	virtual cpu_thread& args(std::initializer_list<std::string> values) = 0;

	virtual cpu_thread& run() = 0;

	u64 join()
	{
		if (!joinable())
			throw "thread must be joinable for join";

		thread->SetJoinable(false);

		while (thread->IsRunning())
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack

		return thread->GetExitStatus();
	}

	bool joinable() const
	{
		return thread->IsJoinable();
	}

	u32 get_id() const
	{
		return thread->GetId();
	}
};
