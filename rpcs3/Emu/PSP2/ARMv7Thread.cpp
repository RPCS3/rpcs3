#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "ARMv7Thread.h"
#include "ARMv7Opcodes.h"
#include "ARMv7Interpreter.h"

namespace vm { using namespace psv; }

const arm_decoder<arm_interpreter> s_arm_interpreter;

std::string ARMv7Thread::get_name() const
{
	return fmt::format("ARMv7[0x%x] Thread (%s)", id, m_name);
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
	TLS = 0;
}

extern thread_local std::string(*g_tls_log_prefix)();

void ARMv7Thread::cpu_task()
{
	return custom_task ? custom_task(*this) : fast_call(PC);
}

void ARMv7Thread::cpu_task_main()
{
	g_tls_log_prefix = []
	{
		const auto cpu = static_cast<ARMv7Thread*>(get_current_cpu_thread());

		return fmt::format("%s [0x%08x]", cpu->get_name(), cpu->PC);
	};

	while (!state.load() || !check_state())
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
	if (stack_addr)
	{
		vm::dealloc_verbose_nothrow(stack_addr, vm::main);
	}
}

ARMv7Thread::ARMv7Thread(const std::string& name)
	: cpu_thread(cpu_type::arm)
	, m_name(name)
{
}

void ARMv7Thread::fast_call(u32 addr)
{
	const auto old_PC = PC;
	const auto old_SP = SP;
	const auto old_LR = LR;
	const auto old_task = std::move(custom_task);
	const auto old_func = last_function;

	PC = addr;
	LR = Emu.GetCPUThreadStop();
	custom_task = nullptr;
	last_function = nullptr;

	try
	{
		cpu_task_main();

		if (SP != old_SP && !state.test(cpu_state::ret) && !state.test(cpu_state::exit)) // SP shouldn't change
		{
			throw fmt::exception("Stack inconsistency (addr=0x%x, SP=0x%x, old=0x%x)", addr, SP, old_SP);
		}
	}
	catch (cpu_state _s)
	{
		state += _s;
		if (_s != cpu_state::ret) throw;
	}
	catch (EmulationStopped)
	{
		if (last_function) LOG_WARNING(ARMv7, "'%s' aborted", last_function);
		last_function = old_func;
		throw;
	}
	catch (...)
	{
		if (last_function) LOG_ERROR(ARMv7, "'%s' aborted", last_function);
		last_function = old_func;
		throw;
	}

	state -= cpu_state::ret;

	PC = old_PC;
	SP = old_SP;
	LR = old_LR;
	custom_task = std::move(old_task);
	last_function = old_func;
}
