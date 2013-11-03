#pragma once
#include "Emu/Memory/MemoryBlock.h"
#include "Emu/Cell/PPCDecoder.h"

enum CPUThreadType
{
	CPU_THREAD_PPU,
	CPU_THREAD_SPU,
	CPU_THREAD_RAW_SPU,
	CPU_THREAD_ARM9,
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

class CPUThread : public ThreadBase
{
protected:
	u32 m_status;
	u32 m_error;
	u32 m_id;
	u64 m_prio;
	u64 m_offset;
	CPUThreadType m_type;
	bool m_joinable;
	bool m_joining;
	bool m_free_data;
	bool m_is_step;

	u64 m_stack_addr;
	u64 m_stack_size;
	u64 m_stack_point;

	u32 m_exit_status;

public:
	virtual void InitRegs()=0;

	virtual void InitStack()=0;
	virtual void CloseStack();

	u64 GetStackAddr() const { return m_stack_addr; }
	u64 GetStackSize() const { return m_stack_size; }
	virtual u64 GetFreeStackSize() const=0;

	void SetStackAddr(u64 stack_addr) { m_stack_addr = stack_addr; }
	void SetStackSize(u64 stack_size) { m_stack_size = stack_size; }

	virtual void SetArg(const uint pos, const u64 arg) = 0;

	void SetId(const u32 id);
	void SetName(const wxString& name);
	void SetPrio(const u64 prio) { m_prio = prio; }
	void SetOffset(const u64 offset) { m_offset = offset; }
	void SetExitStatus(const u32 status) { m_exit_status = status; }

	u64 GetOffset() const { return m_offset; }
	u32 GetExitStatus() const { return m_exit_status; }
	u64 GetPrio() const { return m_prio; }

	wxString GetName() const { return m_name; }
	wxString GetFName() const
	{
		return 
			wxString::Format("%s[%d] Thread%s", 
				GetTypeString(),
				m_id,
				(GetName().IsEmpty() ? "" : " (" + GetName() + ")")
			);
	}

	static wxString CPUThreadTypeToString(CPUThreadType type)
	{
		switch(type)
		{
		case CPU_THREAD_PPU: return "PPU";
		case CPU_THREAD_SPU: return "SPU";
		case CPU_THREAD_RAW_SPU: return "RawSPU";
		case CPU_THREAD_ARM9: return "ARM9";
		}

		return "Unknown";
	}

	wxString GetTypeString() const { return CPUThreadTypeToString(m_type); }

	virtual wxString GetThreadName() const
	{
		return GetFName() + wxString::Format("[0x%08llx]", PC);
	}

public:
	u64 entry;
	u64 PC;
	u64 nPC;
	u64 cycle;

protected:
	CPUThread(CPUThreadType type);

public:
	~CPUThread();

	u32 m_wait_thread_id;

	wxCriticalSection m_cs_sync;
	bool m_sync_wait;
	void Wait(bool wait);
	void Wait(const CPUThread& thr);
	bool Sync();

	template<typename T>
	void WaitFor(T func)
	{
		while(func(ThreadStatus()))
		{
			Sleep(1);
		}
	}

	int ThreadStatus();

	virtual void NextPc();
	virtual void SetBranch(const u64 pc);
	virtual void SetPc(const u64 pc) = 0;
	virtual void SetEntry(const u64 entry);

	void SetError(const u32 error);

	static wxArrayString ErrorToString(const u32 error);
	wxArrayString ErrorToString() { return ErrorToString(m_error); }

	bool IsOk()		const { return m_error == 0; }
	bool IsRunning()	const { return m_status == Running; }
	bool IsPaused()		const { return m_status == Paused; }
	bool IsStopped()	const { return m_status == Stopped; }

	bool IsJoinable() const { return m_joinable; }
	bool IsJoining()  const { return m_joining; }
	void SetJoinable(bool joinable) { m_joinable = joinable; }
	void SetJoining(bool joining) { m_joining = joining; }

	u32 GetError() const { return m_error; }
	u32 GetId() const { return m_id; }
	CPUThreadType GetType()	const { return m_type; }

	void Reset();
	void Close();
	void Run();
	void Pause();
	void Resume();
	void Stop();

	virtual void AddArgv(const wxString& arg) {}

	virtual wxString RegsToString() = 0;
	virtual wxString ReadRegString(wxString reg) = 0;
	virtual bool WriteRegString(wxString reg, wxString value) = 0;

	virtual void Exec();
	void ExecOnce();

protected:
	virtual void DoReset()=0;
	virtual void DoRun()=0;
	virtual void DoPause()=0;
	virtual void DoResume()=0;
	virtual void DoStop()=0;

protected:
	virtual void Task();
	virtual void DoCode() = 0;
};

CPUThread* GetCurrentCPUThread();