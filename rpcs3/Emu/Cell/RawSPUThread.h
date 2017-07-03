#pragma once

#include "SPUThread.h"

class RawSPUThread final : public SPUThread
{
	void cpu_task() override;

public:
	static const u32 id_base = 0;
	static const u32 id_step = 1;
	static const u32 id_count = 5;

	void on_init(const std::shared_ptr<void>&) override;

	RawSPUThread(const std::string& name);

	bool read_reg(const u32 addr, u32& value);
	bool write_reg(const u32 addr, const u32 value);
};
