#pragma once

#include "Emu/CPU/CPUDisAsm.h"

class RSXDisAsm final : public CPUDisAsm
{
public:
	RSXDisAsm(cpu_disasm_mode mode, const u8* offset, u32 start_pc, const cpu_thread* cpu) : CPUDisAsm(mode, offset, start_pc, cpu)
	{
	}

private:
	void Write(std::string_view str, s32 count, bool is_non_inc = false, u32 id = 0);

public:
	u32 disasm(u32 pc) override;
	std::pair<const void*, usz> get_memory_span() const override;
	std::unique_ptr<CPUDisAsm> copy_type_erased() const override;
};
