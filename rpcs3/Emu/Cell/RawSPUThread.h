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

	void on_init(const std::shared_ptr<void>&) override;

	RawSPUThread(const std::string& name);

	bool read_reg(const u32 addr, u32& value);
	bool write_reg(const u32 addr, const u32 value);
};
