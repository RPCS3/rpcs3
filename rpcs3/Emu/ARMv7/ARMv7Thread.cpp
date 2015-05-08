#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/CPU/CPUThreadManager.h"

#include "ARMv7Thread.h"
#include "ARMv7Decoder.h"
#include "ARMv7DisAsm.h"
#include "ARMv7Interpreter.h"

void ARMv7Context::write_pc(u32 value)
{
	ISET = value & 1 ? Thumb : ARM;
	thread.SetBranch(value & ~1);
}

u32 ARMv7Context::read_pc()
{
	return ISET == ARM ? thread.PC + 8 : thread.PC + 4;
}

u32 ARMv7Context::get_stack_arg(u32 pos)
{
	return vm::psv::read32(SP + sizeof(u32) * (pos - 5));
}

void ARMv7Context::fast_call(u32 addr)
{
	return thread.FastCall(addr);
}

#define TLS_MAX 128

u32 g_armv7_tls_start;

std::array<std::atomic<u32>, TLS_MAX> g_armv7_tls_owners;

void armv7_init_tls()
{
	g_armv7_tls_start = Emu.GetTLSMemsz() ? Memory.PSV.RAM.AllocAlign(Emu.GetTLSMemsz() * TLS_MAX, 4096) : 0;

	for (auto& v : g_armv7_tls_owners)
	{
		v.store(0, std::memory_order_relaxed);
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
			memcpy(vm::get_ptr(addr), vm::get_ptr(Emu.GetTLSAddr()), Emu.GetTLSFilesz()); // initialize from TLS image
			memset(vm::get_ptr(addr + Emu.GetTLSFilesz()), 0, Emu.GetTLSMemsz() - Emu.GetTLSFilesz()); // fill the rest with zeros
			return addr;
		}
	}

	throw "Out of TLS memory";
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

ARMv7Thread::ARMv7Thread()
	: CPUThread(CPU_THREAD_ARMv7)
	, context(*this)
	//, m_arg(0)
	//, m_last_instr_size(0)
	//, m_last_instr_name("UNK")
{
}

ARMv7Thread::~ARMv7Thread()
{
	armv7_free_tls(GetId());
}

void ARMv7Thread::InitRegs()
{
	memset(context.GPR, 0, sizeof(context.GPR));
	context.APSR.APSR = 0;
	context.IPSR.IPSR = 0;
	context.ISET = PC & 1 ? Thumb : ARM; // select instruction set
	context.thread.SetPc(PC & ~1); // and fix PC
	context.ITSTATE.IT = 0;
	context.SP = m_stack_addr + m_stack_size;
	context.TLS = armv7_get_tls(GetId());
	context.debug |= DF_DISASM | DF_PRINT;
}

void ARMv7Thread::InitStack()
{
	if (!m_stack_addr)
	{
		assert(m_stack_size);
		m_stack_addr = Memory.Alloc(m_stack_size, 4096);
	}
}

void ARMv7Thread::CloseStack()
{
	if (m_stack_addr)
	{
		Memory.Free(m_stack_addr);
		m_stack_addr = 0;
	}
}

std::string ARMv7Thread::RegsToString()
{
	std::string result = "Registers:\n=========\n";
	for(int i=0; i<15; ++i)
	{
		result += fmt::Format("%s\t= 0x%08x\n", g_arm_reg_name[i], context.GPR[i]);
	}

	result += fmt::Format("APSR\t= 0x%08x [N: %d, Z: %d, C: %d, V: %d, Q: %d]\n", 
		context.APSR.APSR,
		fmt::by_value(context.APSR.N),
		fmt::by_value(context.APSR.Z),
		fmt::by_value(context.APSR.C),
		fmt::by_value(context.APSR.V),
		fmt::by_value(context.APSR.Q));
	
	return result;
}

std::string ARMv7Thread::ReadRegString(const std::string& reg)
{
	return "";
}

bool ARMv7Thread::WriteRegString(const std::string& reg, std::string value)
{
	return true;
}

void ARMv7Thread::DoReset()
{
}

void ARMv7Thread::DoRun()
{
	m_dec = nullptr;

	switch(Ini.CPUDecoderMode.GetValue())
	{
	case 0:
	case 1:
		m_dec = new ARMv7Decoder(context);
		break;
	default:
		LOG_ERROR(PPU, "Invalid CPU decoder mode: %d", Ini.CPUDecoderMode.GetValue());
		Emu.Pause();
	}
}

void ARMv7Thread::DoPause()
{
}

void ARMv7Thread::DoResume()
{
}

void ARMv7Thread::DoStop()
{
}

void ARMv7Thread::DoCode()
{
}

void ARMv7Thread::FastCall(u32 addr)
{
	auto old_status = m_status;
	auto old_PC = PC;
	auto old_stack = context.SP;
	auto old_LR = context.LR;
	auto old_thread = GetCurrentNamedThread();

	m_status = Running;
	PC = addr;
	context.LR = Emu.GetCPUThreadStop();
	SetCurrentNamedThread(this);

	CPUThread::Task();

	m_status = old_status;
	PC = old_PC;
	context.SP = old_stack;
	context.LR = old_LR;
	SetCurrentNamedThread(old_thread);
}

void ARMv7Thread::FastStop()
{
	m_status = Stopped;
	m_events |= CPU_EVENT_STOP;
}

armv7_thread::armv7_thread(u32 entry, const std::string& name, u32 stack_size, s32 prio)
{
	thread = Emu.GetCPU().AddThread(CPU_THREAD_ARMv7);

	thread->SetName(name);
	thread->SetEntry(entry);
	thread->SetStackSize(stack_size);
	thread->SetPrio(prio);

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

	argv = Memory.PSV.RAM.AllocAlign(argv_size, 4096); // allocate arg list
	memcpy(vm::get_ptr(argv), argv_data.data(), argv_size); // copy arg list
	
	return *this;
}

cpu_thread& armv7_thread::run()
{
	auto& armv7 = static_cast<ARMv7Thread&>(*thread);

	armv7.Run();

	// set arguments
	armv7.context.GPR[0] = argc;
	armv7.context.GPR[1] = argv;

	return *this;
}
