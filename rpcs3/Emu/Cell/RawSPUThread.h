#pragma once
#include "SPUThread.h"

enum : u32
{
	RAW_SPU_OFFSET = 0x00100000,
	RAW_SPU_BASE_ADDR = 0xE0000000,
	RAW_SPU_LS_OFFSET = 0x00000000,
	RAW_SPU_PROB_OFFSET = 0x00040000,
};

force_inline static u32 GetRawSPURegAddrByNum(int num, int offset)
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
