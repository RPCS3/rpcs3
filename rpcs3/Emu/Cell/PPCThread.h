#pragma once
#include "Emu/Memory/MemoryBlock.h"
#include "Emu/Cell/Decoder.h"

enum PPCThreadType
{
	PPC_THREAD_PPU,
	PPC_THREAD_SPU,
};

class PPCThread// : public StepThread
{
protected:
	u32 m_status;
	u32 m_error;
	Decoder* m_dec;
	wxWindow* DisAsmFrame;
	u32 m_id;
	PPCThreadType m_type;
	u64 m_arg;
	u64 m_prio;
	wxString m_name;
	bool m_joinable;
	bool m_joining;
	Array<u64> argv_addr;
	u64 m_offset;

public:
	u64 stack_size;
	u64 stack_addr;
	u64 stack_point;

	virtual void InitRegs()=0;

	virtual void InitStack();
	virtual void CloseStack();
	
	virtual u64 GetStackAddr() const { return stack_addr; }
	virtual u64 GetStackSize() const { return stack_size; }
	virtual u64 GetFreeStackSize() const=0;
	void SetArg(const u64 arg) { m_arg = arg; }

	void SetId(const u32 id);
	void SetName(const wxString& name);
	void SetPrio(const u64 prio) { m_prio = prio; }
	void SetOffset(const u64 offset) { m_offset = offset; }

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

	static wxString PPCThreadTypeToString(PPCThreadType type)
	{
		switch(type)
		{
		case PPC_THREAD_PPU: return "PPU";
		case PPC_THREAD_SPU: return "SPU";
		}

		return "Unknown";
	}

	wxString GetTypeString() const { return PPCThreadTypeToString(m_type); }

public:
	bool isBranch;

	u64 PC;
	u64 nPC;
	u64 cycle;

protected:
	PPCThread(PPCThreadType type);

public:
	~PPCThread();

	void NextPc();
	void NextBranchPc();
	void PrevPc();
	void SetBranch(const u64 pc);
	void SetPc(const u64 pc);

	void SetError(const u32 error);

	static wxArrayString ErrorToString(const u32 error);
	wxArrayString ErrorToString() { return ErrorToString(m_error); }

	bool IsSPU()	const { return m_type == PPC_THREAD_SPU; }
	bool IsOk()		const { return m_error == 0; }
	bool IsRunned()		const { return m_status == Runned; }
	bool IsPaused()		const { return m_status == Paused; }
	bool IsStopped()	const { return m_status == Stopped; }

	bool IsJoinable() const { return m_joinable; }
	bool IsJoining()  const { return m_joining; }
	void SetJoinable(bool joinable) { m_joinable = joinable; }
	void SetJoining(bool joining) { m_joining = joining; }

	u32 GetError() const { return m_error; }
	u32 GetId() const { return m_id; }

	void Reset();
	void Close();
	void Run();
	void Pause();
	void Resume();
	void Stop();

	virtual wxString RegsToString() { return wxEmptyString; }

	virtual void Exec();

	virtual void AddArgv(const wxString& arg) {}
	
protected:
	virtual void DoReset()=0;
	virtual void DoRun()=0;
	virtual void DoPause()=0;
	virtual void DoResume()=0;
	virtual void DoStop()=0;
	
private:
	virtual void DoCode(const s32 code)=0;
};