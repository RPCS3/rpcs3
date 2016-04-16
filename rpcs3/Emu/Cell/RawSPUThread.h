#pragma once

#include "SPUThread.h"

class RawSPUThread final : public SPUThread
{
	void cpu_task() override;

public:
	/* IdManager setups */

	using id_base = RawSPUThread;

	static constexpr u32 id_min = 0;
	static constexpr u32 id_max = 4;

	void on_init() override
	{
		if (!offset)
		{
			// Install correct SPU index and LS address
			const_cast<u32&>(index) = id;
			const_cast<u32&>(offset) = vm::falloc(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * index, 0x40000);
			ASSERT(offset);

			SPUThread::on_init();
		}
	}

	RawSPUThread(const std::string& name)
		: SPUThread(name)
	{
	}

	bool read_reg(const u32 addr, u32& value);
	bool write_reg(const u32 addr, const u32 value);
};
