#pragma once
#include "SPUThread.h"

__forceinline static u32 GetRawSPURegAddrByNum(int num, int offset)
{
	return RAW_SPU_OFFSET * num + RAW_SPU_BASE_ADDR + RAW_SPU_PROB_OFFSET + offset;
}

class RawSPUThread : public SPUThread
{
public:
	RawSPUThread(CPUThreadType type = CPU_THREAD_RAW_SPU);
	virtual ~RawSPUThread();

	void start();

	bool ReadReg(const u32 addr, u32& value);
	bool WriteReg(const u32 addr, const u32 value);

private:
	virtual void Task();
};

SPUThread& GetCurrentSPUThread();
