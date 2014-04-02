#pragma once
#include "Emu/Memory/MemoryBlock.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/PPCDecoder.h"

class PPCThread : public CPUThread
{
protected:
	u64 m_args[4];
	Array<u64> m_argv_addr;

public:
	virtual void InitRegs()=0;
	virtual void InitStack();

	virtual void SetArg(const uint pos, const u64 arg) { assert(pos < 4); m_args[pos] = arg; }

	virtual std::string GetThreadName() const
	{
		return (GetFName() + fmt::Format("[0x%08llx]", PC));
	}

protected:
	PPCThread(CPUThreadType type);

public:
	virtual ~PPCThread();

protected:
	virtual void DoReset() override;
};

PPCThread* GetCurrentPPCThread();