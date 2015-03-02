#pragma once
#include "SPUThread.h"

__forceinline static u32 GetRawSPURegAddrByNum(int num, int offset)
{
	return RAW_SPU_OFFSET * num + RAW_SPU_BASE_ADDR + RAW_SPU_PROB_OFFSET + offset;
}

class RawSPUThread
	: public SPUThread
	, public MemoryBlock
{
	u32 m_index;

public:
	RawSPUThread(CPUThreadType type = CPU_THREAD_RAW_SPU);
	virtual ~RawSPUThread();

	void start();

	bool Read32(const u32 addr, u32* value);
	bool Write32(const u32 addr, const u32 value);

public:
	virtual void InitRegs();
	u32 GetIndex() const;

private:
	virtual void Task();
};

SPUThread& GetCurrentSPUThread();