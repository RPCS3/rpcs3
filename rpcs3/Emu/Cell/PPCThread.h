#pragma once
#include "Emu/CPU/CPUThread.h"

class PPCThread : public CPUThread
{
public:
	virtual std::string GetThreadName() const
	{
		return fmt::format("%s[0x%08x]", GetFName(), PC);
	}

protected:
	PPCThread(CPUThreadType type);

public:
	virtual ~PPCThread();

protected:
	virtual void DoReset() override;
};

PPCThread* GetCurrentPPCThread();
