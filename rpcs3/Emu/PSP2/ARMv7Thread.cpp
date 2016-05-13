#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "ARMv7Thread.h"
#include "ARMv7Opcodes.h"
#include "ARMv7Interpreter.h"

namespace vm { using namespace psv; }

const arm_decoder<arm_interpreter> s_arm_interpreter;

#define TLS_MAX 128

u32 g_armv7_tls_start;

std::array<atomic_t<u32>, TLS_MAX> g_armv7_tls_owners;

void armv7_init_tls()
{
	g_armv7_tls_start = Emu.GetTLSMemsz() ? vm::alloc(Emu.GetTLSMemsz() * TLS_MAX, vm::main) : 0;

	for (auto& v : g_armv7_tls_owners)
	{
		v = 0;
	}
}

u32 armv7_get_tls(u32 thread)
{
	if (!Emu.GetTLSMemsz() || !thread)
	{
		return 0;
	}

	for (u32 i = 0; i < TLS_MAX; i++)
	{
		if (g_armv7_tls_owners[i] == thread)
		{
			return g_armv7_tls_start + i * Emu.GetTLSMemsz(); // if already initialized, return TLS address
		}
	}

	for (u32 i = 0; i < TLS_MAX; i++)
	{
		if (g_armv7_tls_owners[i].compare_and_swap_test(0, thread))
		{
			const u32 addr = g_armv7_tls_start + i * Emu.GetTLSMemsz(); // get TLS address
			std::memcpy(vm::base(addr), vm::base(Emu.GetTLSAddr()), Emu.GetTLSFilesz()); // initialize from TLS image
			std::memset(vm::base(addr + Emu.GetTLSFilesz()), 0, Emu.GetTLSMemsz() - Emu.GetTLSFilesz()); // fill the rest with zeros
			return addr;
		}
	}

	throw EXCEPTION("Out of TLS memory");
}

void armv7_free_tls(u32 thread)
{
	if (!Emu.GetTLSMemsz())
	{
		return;
	}

	for (auto& v : g_armv7_tls_owners)
	{
		if (v.compare_and_swap_test(thread, 0))
		{
			return;
		}
	}
}

std::string ARMv7Thread::get_name() const
{
	return fmt::format("ARMv7[0x%x] Thread (%s)", id, name);
}

std::string ARMv7Thread::dump() const
{
	std::string result = "Registers:\n=========\n";
	for(int i=0; i<15; ++i)
	{
		result += fmt::format("r%u\t= 0x%08x\n", i, GPR[i]);
	}

	result += fmt::format("APSR\t= 0x%08x [N: %d, Z: %d, C: %d, V: %d, Q: %d]\n", 
		APSR.APSR,
		u32{ APSR.N },
		u32{ APSR.Z },
		u32{ APSR.C },
		u32{ APSR.V },
		u32{ APSR.Q });
	
	return result;
}

void ARMv7Thread::cpu_init()
{
	if (!stack_addr)
	{
		if (!stack_size)
		{
			throw EXCEPTION("Invalid stack size");
		}

		stack_addr = vm::alloc(stack_size, vm::main);

		if (!stack_addr)
		{
			throw EXCEPTION("Out of stack memory");
		}
	}

	memset(GPR, 0, sizeof(GPR));
	APSR.APSR = 0;
	IPSR.IPSR = 0;
	ISET = PC & 1 ? Thumb : ARM; // select instruction set
	PC = PC & ~1; // and fix PC
	ITSTATE.IT = 0;
	SP = stack_addr + stack_size;
	TLS = armv7_get_tls(id);
}

extern thread_local std::string(*g_tls_log_prefix)();

void ARMv7Thread::cpu_task()
{
	if (custom_task)
	{
		if (check_status()) return;

		return custom_task(*this);
	}

	g_tls_log_prefix = []
	{
		const auto cpu = static_cast<ARMv7Thread*>(get_current_cpu_thread());

		return fmt::format("%s [0x%08x]", cpu->get_name(), cpu->PC);
	};

	while (!state.load() || !check_status())
	{
		if (ISET == Thumb)
		{
			const u16 op16 = vm::read16(PC);
			const u32 cond = ITSTATE.advance();

			if (const auto func16 = s_arm_interpreter.decode_thumb(op16))
			{
				func16(*this, op16, cond);
				PC += 2;
			}
			else
			{
				const u32 op32 = (op16 << 16) | vm::read16(PC + 2);

				s_arm_interpreter.decode_thumb(op32)(*this, op32, cond);
				PC += 4;
			}
		}
		else if (ISET == ARM)
		{
			const u32 op = vm::read32(PC);

			s_arm_interpreter.decode_arm(op)(*this, op, op >> 28);
			PC += 4;
		}
		else
		{
			throw fmt::exception("Invalid instruction set" HERE);
		}
	}
}

ARMv7Thread::~ARMv7Thread()
{
	armv7_free_tls(id);

	if (stack_addr)
	{
		vm::dealloc_verbose_nothrow(stack_addr, vm::main);
	}
}

ARMv7Thread::ARMv7Thread(const std::string& name)
	: cpu_thread(cpu_type::arm, name)
{
}

void ARMv7Thread::fast_call(u32 addr)
{
	auto old_PC = PC;
	auto old_stack = SP;
	auto old_LR = LR;
	auto old_task = std::move(custom_task);

	PC = addr;
	LR = Emu.GetCPUThreadStop();
	custom_task = nullptr;

	try
	{
		cpu_task();
	}
	catch (cpu_state _s)
	{
		state += _s;
		if (_s != cpu_state::ret) throw;
	}

	state -= cpu_state::ret;

	PC = old_PC;

	if (SP != old_stack) // SP shouldn't change
	{
		throw EXCEPTION("Stack inconsistency (addr=0x%x, SP=0x%x, old=0x%x)", addr, SP, old_stack);
	}

	LR = old_LR;
	custom_task = std::move(old_task);
}
