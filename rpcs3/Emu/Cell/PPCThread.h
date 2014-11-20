#pragma once
#include "Emu/CPU/CPUThread.h"

class PPCThread : public CPUThread
{
public:
	virtual void InitRegs()=0;
	virtual void InitStack();

	virtual std::string GetThreadName() const
	{
		return (GetFName() + fmt::Format("[0x%08x]", PC));
	}

protected:
	PPCThread(CPUThreadType type);

public:
	virtual ~PPCThread();

protected:
	virtual void DoReset() override;
};

PPCThread* GetCurrentPPCThread();
