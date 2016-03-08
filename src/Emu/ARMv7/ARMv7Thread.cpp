#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/state.h"
#include "Emu/IdManager.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "ARMv7Thread.h"
#include "ARMv7Decoder.h"
#include "ARMv7DisAsm.h"
#include "ARMv7Interpreter.h"

void ARMv7Context::fast_call(u32 addr)
{
	return static_cast<ARMv7Thread*>(this)->fast_call(addr);
}

#define TLS_MAX 128

u32 g_armv7_tls_start;

std::array<std::atomic<u32>, TLS_MAX> g_armv7_tls_owners;

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
		u32 old = 0;
		if (g_armv7_tls_owners[i].compare_exchange_strong(old, thread))
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
		u32 old = thread;
		if (v.compare_exchange_strong(old, 0))
		{
			return;
		}
	}
}

ARMv7Thread::ARMv7Thread(const std::string& name)
	: CPUThread(CPU_THREAD_ARMv7, name)
	, ARMv7Context({})
{
}

ARMv7Thread::~ARMv7Thread()
{
	close_stack();
	armv7_free_tls(m_id);
}

std::string ARMv7Thread::get_name() const
{
	return fmt::format("ARMv7 Thread[0x%x] (%s)[0x%08x]", m_id, CPUThread::get_name(), PC);
}

void ARMv7Thread::dump_info() const
{
	if (hle_func)
	{
		const auto func = get_psv_func_by_nid(hle_func);
		
		LOG_SUCCESS(HLE, "Last function: %s (0x%x)", func ? func->name : "?????????", hle_func);
	}

	CPUThread::dump_info();
}

void ARMv7Thread::init_regs()
{
	memset(GPR, 0, sizeof(GPR));
	APSR.APSR = 0;
	IPSR.IPSR = 0;
	ISET = PC & 1 ? Thumb : ARM; // select instruction set
	PC = PC & ~1; // and fix PC
	ITSTATE.IT = 0;
	SP = stack_addr + stack_size;
	TLS = armv7_get_tls(m_id);
	debug = DF_DISASM | DF_PRINT;
}

void ARMv7Thread::init_stack()
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
}

void ARMv7Thread::close_stack()
{
	if (stack_addr)
	{
		vm::dealloc_verbose_nothrow(stack_addr, vm::main);
		stack_addr = 0;
	}
}

std::string ARMv7Thread::RegsToString() const
{
	std::string result = "Registers:\n=========\n";
	for(int i=0; i<15; ++i)
	{
		result += fmt::format("%s\t= 0x%08x\n", g_arm_reg_name[i], GPR[i]);
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

std::string ARMv7Thread::ReadRegString(const std::string& reg) const
{
	return "";
}

bool ARMv7Thread::WriteRegString(const std::string& reg, std::string value)
{
	return true;
}

void ARMv7Thread::do_run()
{
	m_dec.reset();

	switch((int)rpcs3::state.config.core.ppu_decoder.value())
	{
	case 0:
	case 1:
		m_dec.reset(new ARMv7Decoder(*this));
		break;
	default:
		LOG_ERROR(ARMv7, "Invalid CPU decoder mode: %d", (int)rpcs3::state.config.core.ppu_decoder.value());
		Emu.Pause();
	}
}

void ARMv7Thread::cpu_task()
{
	if (custom_task)
	{
		if (check_status()) return;

		return custom_task(*this);
	}

	while (!m_state || !check_status())
	{
		// decode instruction using specified decoder
		PC += m_dec->DecodeMemory(PC);
	}
}

void ARMv7Thread::fast_call(u32 addr)
{
	if (!is_current())
	{
		throw EXCEPTION("Called from the wrong thread");
	}

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
	catch (CPUThreadReturn)
	{
	}

	m_state &= ~CPU_STATE_RETURN;

	PC = old_PC;

	if (SP != old_stack) // SP shouldn't change
	{
		throw EXCEPTION("Stack inconsistency (addr=0x%x, SP=0x%x, old=0x%x)", addr, SP, old_stack);
	}

	LR = old_LR;
	custom_task = std::move(old_task);
}

void ARMv7Thread::fast_stop()
{
	m_state |= CPU_STATE_RETURN;
}

armv7_thread::armv7_thread(u32 entry, const std::string& name, u32 stack_size, s32 prio)
{
	std::shared_ptr<ARMv7Thread> armv7 = idm::make_ptr<ARMv7Thread>(name);

	armv7->PC = entry;
	armv7->stack_size = stack_size;
	armv7->prio = prio;

	thread = std::move(armv7);

	argc = 0;
}

cpu_thread& armv7_thread::args(std::initializer_list<std::string> values)
{
	assert(argc == 0);

	if (!values.size())
	{
		return *this;
	}

	std::vector<char> argv_data;
	u32 argv_size = 0;

	for (auto& arg : values)
	{
		const u32 arg_size = arg.size(); // get arg size

		for (char c : arg)
		{
			argv_data.push_back(c); // append characters
		}

		argv_data.push_back('\0'); // append null terminator

		argv_size += arg_size + 1;
		argc++;
	}

	argv = vm::alloc(argv_size, vm::main); // allocate arg list
	std::memcpy(vm::base(argv), argv_data.data(), argv_size); // copy arg list
	
	return *this;
}

cpu_thread& armv7_thread::run()
{
	auto& armv7 = static_cast<ARMv7Thread&>(*thread);

	armv7.run();

	// set arguments
	armv7.GPR[0] = argc;
	armv7.GPR[1] = argv;

	return *this;
}
