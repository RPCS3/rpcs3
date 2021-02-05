#pragma once

#include "Emu/Cell/PPCDisAsm.h"

class RSXDisAsm final : public CPUDisAsm
{
public:
	RSXDisAsm(cpu_disasm_mode mode, const u8* offset, const cpu_thread* cpu) : CPUDisAsm(mode, offset, cpu)
	{
	}

private:
	void Write(const std::string& str, s32 count, bool is_non_inc = false, u32 id = 0);

public:
	u32 disasm(u32 pc) override;
};
