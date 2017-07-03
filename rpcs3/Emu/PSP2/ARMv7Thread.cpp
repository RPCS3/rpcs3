#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"

#include "ARMv7Thread.h"
#include "ARMv7Opcodes.h"
#include "ARMv7Interpreter.h"
#include "ARMv7Function.h"

#include "Utilities/GSL.h"

namespace vm { using namespace psv; }

const arm_decoder<arm_interpreter> s_arm_interpreter;

std::string ARMv7Thread::get_name() const
{
	return fmt::format("ARMv7[0x%x] Thread (%s)", id, m_name);
}

std::string ARMv7Thread::dump() const
{
	std::string result = cpu_thread::dump();
	result += "Last function: ";
	result += last_function ? last_function : "";
	result += "\n\n";
	result += "Registers:\n=========\n";
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

extern thread_local std::string(*g_tls_log_prefix)();

void ARMv7Thread::cpu_task()
{
	return fast_call(PC);
}

void ARMv7Thread::cpu_task_main()
{
	g_tls_log_prefix = []
	{
		const auto cpu = static_cast<ARMv7Thread*>(get_current_cpu_thread());

		return fmt::format("%s [0x%08x]", cpu->get_name(), cpu->PC);
	};

	while (!test(state) || !check_state())
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
			fmt::throw_exception("Invalid instruction set" HERE);
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

ARMv7Thread::ARMv7Thread(const std::string& name, u32 prio, u32 stack)
	: cpu_thread(idm::last_id())
	, m_name(name)
	, prio(prio)
	, stack_addr(vm::alloc(stack, vm::main))
	, stack_size(stack)
{
	verify(HERE), stack_size, stack_addr;

	std::memset(GPR, 0, sizeof(GPR));
	APSR.APSR = 0;
	IPSR.IPSR = 0;
	ITSTATE.IT = 0;
	SP = stack_addr + stack_size;
}

void ARMv7Thread::fast_call(u32 addr)
{
	const auto old_PC = PC;
	const auto old_LR = LR;
	const auto old_func = last_function;

	PC = addr;
	LR = arm_function_manager::addr; // TODO
	last_function = nullptr;

	auto at_ret = gsl::finally([&]()
	{
		if (std::uncaught_exception())
		{
			if (last_function)
			{
				LOG_ERROR(ARMv7, "'%s' aborted", last_function);
			}

			last_function = old_func;
		}
		else
		{
			state -= cpu_flag::ret;
			PC = old_PC;
			LR = old_LR;
			last_function = old_func;
		}
	});

	try
	{
		cpu_task_main();
	}
	catch (cpu_flag _s)
	{
		state += _s;

		if (_s != cpu_flag::ret)
		{
			throw;
		}
	}
}

u32 ARMv7Thread::stack_push(u32 size, u32 align_v)
{
	if (auto cpu = get_current_cpu_thread())
	{
		ARMv7Thread& context = static_cast<ARMv7Thread&>(*cpu);

		const u32 old_pos = context.SP;
		context.SP -= align(size + 4, 4); // room minimal possible size
		context.SP &= ~(align_v - 1); // fix stack alignment

		if (old_pos >= context.stack_addr && old_pos < context.stack_addr + context.stack_size && context.SP < context.stack_addr)
		{
			fmt::throw_exception("Stack overflow (size=0x%x, align=0x%x, SP=0x%x, stack=*0x%x)" HERE, size, align_v, context.SP, context.stack_addr);
		}
		else
		{
			vm::psv::_ref<nse_t<u32>>(context.SP + size) = old_pos;
			return context.SP;
		}
	}

	fmt::throw_exception("Invalid thread" HERE);
}

void ARMv7Thread::stack_pop_verbose(u32 addr, u32 size) noexcept
{
	if (auto cpu = get_current_cpu_thread())
	{
		ARMv7Thread& context = static_cast<ARMv7Thread&>(*cpu);

		if (context.SP != addr)
		{
			LOG_ERROR(ARMv7, "Stack inconsistency (addr=0x%x, SP=0x%x, size=0x%x)", addr, context.SP, size);
			return;
		}

		context.SP = vm::psv::_ref<nse_t<u32>>(context.SP + size);
		return;
	}

	LOG_ERROR(ARMv7, "Invalid thread" HERE);
}
