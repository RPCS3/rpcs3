#pragma once

#include "Emu/CPU/CPUDisAsm.h"
#include "util/shared_ptr.hpp"

struct lv2_rsx_context;

class RSXDisAsm final : public CPUDisAsm
{
public:
	RSXDisAsm(cpu_disasm_mode mode, shared_ptr<lv2_rsx_context> context, u32 start_pc, const cpu_thread* cpu) : CPUDisAsm(mode, get_offset_from_context(context.get()), start_pc, cpu)
	{
		rsx_context_ptr = context;
	}

	RSXDisAsm(cpu_disasm_mode mode, u32 context, u32 start_pc, const cpu_thread* cpu) : RSXDisAsm(mode, get_context_from_id(cpu, context), start_pc, cpu)
	{
	}

private:
	void Write(std::string_view str, s32 count, bool is_non_inc = false, u32 id = 0);

	shared_ptr<lv2_rsx_context> rsx_context_ptr;

	lv2_rsx_context* get_context() const;
	const u8* get_offset_from_context(lv2_rsx_context* context) const;
	shared_ptr<lv2_rsx_context> get_context_from_id(const  cpu_thread* _this, u32 id) const;

public:
	u32 disasm(u32 pc) override;
	std::pair<const void*, usz> get_memory_span() const override;
	std::unique_ptr<CPUDisAsm> copy_type_erased() const override;
};
