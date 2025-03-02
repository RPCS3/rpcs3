#include "stdafx.h"

#include "Emu/IdManager.h"
#include "Emu/system_config.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/SPUDisAsm.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/RSX/RSXDisAsm.h"
#include "Emu/Memory/vm.h"
#include "Utilities/Thread.h"

bool is_using_interpreter(thread_class t_class)
{
	switch (t_class)
	{
	case thread_class::ppu: return g_cfg.core.ppu_decoder != ppu_decoder_type::llvm;
	case thread_class::spu: return g_cfg.core.spu_decoder != spu_decoder_type::asmjit && g_cfg.core.spu_decoder != spu_decoder_type::llvm;
	default: return true;
	}
}

std::shared_ptr<CPUDisAsm> make_disasm(const cpu_thread* cpu, shared_ptr<cpu_thread> handle)
{
	if (!handle)
	{
		switch (cpu->get_class())
		{
		case thread_class::ppu: handle = idm::get_unlocked<named_thread<ppu_thread>>(cpu->id); break;
		case thread_class::spu: handle = idm::get_unlocked<named_thread<spu_thread>>(cpu->id); break;
		default: break;
		}
	}

	std::shared_ptr<CPUDisAsm> result;

	switch (cpu->get_class())
	{
	case thread_class::ppu: result = std::make_shared<PPUDisAsm>(cpu_disasm_mode::interpreter, vm::g_sudo_addr); break;
	case thread_class::spu: result = std::make_shared<SPUDisAsm>(cpu_disasm_mode::interpreter, static_cast<const spu_thread*>(cpu)->ls); break;
	case thread_class::rsx: result = std::make_shared<RSXDisAsm>(cpu_disasm_mode::interpreter, vm::g_sudo_addr, 0, cpu); break;
	default: return result;
	}

	result->set_cpu_handle(std::move(handle));
	return result;
}
