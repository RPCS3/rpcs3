#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/DbgCommand.h"

#include "CPUDecoder.h"
#include "CPUThread.h"

CPUThread* GetCurrentCPUThread()
{
	return dynamic_cast<CPUThread*>(GetCurrentNamedThread());
}

CPUThread::CPUThread(CPUThreadType type)
	: ThreadBase("CPUThread")
	, m_type(type)
	, m_stack_size(0)
	, m_stack_addr(0)
	, m_offset(0)
	, m_prio(0)
	, m_dec(nullptr)
	, m_is_step(false)
	, m_is_branch(false)
	, m_status(Stopped)
	, m_last_syscall(0)
	, m_trace_enabled(false)
	, m_trace_call_stack(true)
{
}

CPUThread::~CPUThread()
{
	safe_delete(m_dec);
}

bool CPUThread::IsRunning() const { return m_status == Running; }
bool CPUThread::IsPaused() const { return m_status == Paused; }
bool CPUThread::IsStopped() const { return m_status == Stopped; }

void CPUThread::Close()
{
	ThreadBase::Stop(false);
	DoStop();

	delete m_dec;
	m_dec = nullptr;
}

void CPUThread::Reset()
{
	CloseStack();

	SetPc(0);
	cycle = 0;
	m_is_branch = false;

	m_status = Stopped;
	m_error = 0;
	
	DoReset();
}

void CPUThread::CloseStack()
{
	if(m_stack_addr)
	{
		Memory.StackMem.Free(m_stack_addr);
		m_stack_addr = 0;
	}

	m_stack_size = 0;
}

void CPUThread::SetId(const u32 id)
{
	m_id = id;
}

void CPUThread::SetName(const std::string& name)
{
	NamedThreadBase::SetThreadName(name);
}

int CPUThread::ThreadStatus()
{
	if(Emu.IsStopped() || IsStopped() || IsPaused())
	{
		return CPUThread_Stopped;
	}

	if(TestDestroy())
	{
		return CPUThread_Break;
	}

	if(m_is_step)
	{
		return CPUThread_Step;
	}

	if (Emu.IsPaused())
	{
		return CPUThread_Sleeping;
	}

	return CPUThread_Running;
}

void CPUThread::SetEntry(const u32 pc)
{
	entry = pc;
}

void CPUThread::NextPc(u8 instr_size)
{
	if(m_is_branch)
	{
		m_is_branch = false;

		SetPc(nPC);
	}
	else
	{
		PC += instr_size;
	}
}

void CPUThread::SetBranch(const u32 pc, bool record_branch)
{
	m_is_branch = true;
	nPC = pc;

	if(m_trace_call_stack && record_branch)
		CallStackBranch(pc);
}

void CPUThread::SetPc(const u32 pc)
{
	PC = pc;
}

void CPUThread::SetError(const u32 error)
{
	if(error == 0)
	{
		m_error = 0;
	}
	else
	{
		m_error |= error;
	}
}

std::vector<std::string> CPUThread::ErrorToString(const u32 error)
{
	std::vector<std::string> earr;

	if(error == 0) return earr;

	earr.push_back("Unknown error");

	return earr;
}

void CPUThread::Run()
{
	if(!IsStopped())
		Stop();

	Reset();
	
	SendDbgCommand(DID_START_THREAD, this);

	m_status = Running;

	SetPc(entry);
	InitStack();
	InitRegs();
	DoRun();
	Emu.CheckStatus();

	SendDbgCommand(DID_STARTED_THREAD, this);
}

void CPUThread::Resume()
{
	if(!IsPaused()) return;

	SendDbgCommand(DID_RESUME_THREAD, this);

	m_status = Running;
	DoResume();
	Emu.CheckStatus();

	ThreadBase::Start();

	SendDbgCommand(DID_RESUMED_THREAD, this);
}

void CPUThread::Pause()
{
	if(!IsRunning()) return;

	SendDbgCommand(DID_PAUSE_THREAD, this);

	m_status = Paused;
	DoPause();
	Emu.CheckStatus();

	// ThreadBase::Stop(); // "Abort() called" exception
	SendDbgCommand(DID_PAUSED_THREAD, this);
}

void CPUThread::Stop()
{
	if(IsStopped()) return;

	SendDbgCommand(DID_STOP_THREAD, this);

	m_status = Stopped;

	if(static_cast<NamedThreadBase*>(this) != GetCurrentNamedThread())
	{
		ThreadBase::Stop();
	}

	Emu.CheckStatus();

	SendDbgCommand(DID_STOPED_THREAD, this);
}

void CPUThread::Exec()
{
	m_is_step = false;
	SendDbgCommand(DID_EXEC_THREAD, this);

	if(IsRunning())
		ThreadBase::Start();
}

void CPUThread::ExecOnce()
{
	m_is_step = true;
	SendDbgCommand(DID_EXEC_THREAD, this);

	m_status = Running;
	ThreadBase::Start();
	ThreadBase::Stop(true,false);
	m_status = Paused;
	SendDbgCommand(DID_PAUSE_THREAD, this);
	SendDbgCommand(DID_PAUSED_THREAD, this);
}

enum x64_reg_t : u32
{
	X64R_EAX,
	X64R_ECX,
	X64R_EDX,
	X64R_EBX,
	X64R_ESP,
	X64R_EBP,
	X64R_ESI,
	X64R_EDI,
	X64R_R8D,
	X64R_R9D,
	X64R_R10D,
	X64R_R11D,
	X64R_R12D,
	X64R_R13D,
	X64R_R14D,
	X64R_R15D,
	X64R32 = X64R_EAX,

	X64_IMM8,
	X64_IMM16,
	X64_IMM32,
	X64_IMM64,
};

enum x64_op_t : u32
{
	X64OP_LOAD,
	X64OP_STORE,
};

void decode_x64_reg_op(const u8* code, x64_op_t& decoded_op, x64_reg_t& decoded_reg, size_t& decoded_size)
{
	decoded_size = 0;

	u8 reg = 0;
	if ((*code & 0xf0) == 0x40) // check REX prefix
	{
		if (*code & 0x80) // check REX.W bit
		{
			throw fmt::Format("decode_x64_reg_op(%.16llXh): REX.W bit found", code - decoded_size);
		}
		if (*code & 0x04) // check REX.R bit
		{
			reg = 8;
		}
		code++;
		decoded_size++;
	}

	if (*code == 0x66)
	{
		throw fmt::Format("decode_x64_reg_op(%.16llXh): 0x66 prefix found", code - decoded_size);
		code++;
		decoded_size++;
	}

	auto get_modRM_r32 = [](const u8* code, const u8 reg_base) -> x64_reg_t
	{
		return (x64_reg_t)((((*code & 0x38) >> 3) | reg_base) + X64R32);
	};

	auto get_modRM_size = [](const u8* code) -> size_t
	{
		switch (*code >> 6) // check Mod
		{
		case 0: return (*code & 0x07) == 4 ? 2 : 1; // check SIB
		case 1: return (*code & 0x07) == 4 ? 3 : 2; // check SIB (disp8)
		case 2: return (*code & 0x07) == 4 ? 6 : 5; // check SIB (disp32)
		default: return 1;
		}
	};

	decoded_size++;
	switch (const u8 op1 = *code++)
	{
	case 0x89: // MOV r/m32, r32
	{
		decoded_op = X64OP_STORE;
		decoded_reg = get_modRM_r32(code, reg);
		decoded_size += get_modRM_size(code);
		return;
	}
	case 0x8b: // MOV r32, r/m32
	{
		decoded_op = X64OP_LOAD;
		decoded_reg = get_modRM_r32(code, reg);
		decoded_size += get_modRM_size(code);
		return;
	}
	case 0xc7:
	{
		if (get_modRM_r32(code, 0) == X64R_EAX) // MOV r/m32, imm32
		{
			decoded_op = X64OP_STORE;
			decoded_reg = X64_IMM32;
			decoded_size = get_modRM_size(code) + 4;
			return;
		}
	}
	default:
	{
		throw fmt::Format("decode_x64_reg_op(%.16llX): unsupported opcode found (0x%.2X, 0x%.2X, 0x%.2X)", code - decoded_size, op1, code[0], code[1]);
	}
	}
}

#ifdef _WIN32
void _se_translator(unsigned int u, EXCEPTION_POINTERS* pExp)
{
	const u64 addr64 = (u64)pExp->ExceptionRecord->ExceptionInformation[1] - (u64)Memory.GetBaseAddr();
	const bool is_writing = pExp->ExceptionRecord->ExceptionInformation[0] != 0;
	CPUThread* t = GetCurrentCPUThread();
	if (u == EXCEPTION_ACCESS_VIOLATION && addr64 < 0x100000000 && t)
	{
		const u32 addr = (u32)addr64;
		if (addr >= RAW_SPU_BASE_ADDR && (addr % RAW_SPU_OFFSET) >= RAW_SPU_PROB_OFFSET) // RawSPU MMIO registers
		{
			// one x64 instruction is manually decoded and interpreted
			x64_op_t op;
			x64_reg_t reg;
			size_t size;
			decode_x64_reg_op((const u8*)pExp->ContextRecord->Rip, op, reg, size);

			// get x64 reg value (for store operations)
			u64 reg_value;
			if (reg - X64R32 < 16)
			{
				// load the value from x64 register
				reg_value = (u32)(&pExp->ContextRecord->Rax)[reg - X64R32];
			}
			else if (reg == X64_IMM32)
			{
				// load the immediate value (assuming it's at the end of the instruction)
				reg_value = *(u32*)(pExp->ContextRecord->Rip + size - 4);
			}
			else
			{
				assert(!"Invalid x64_reg_t value");
			}

			bool save_reg = false;

			switch (op)
			{
			case X64OP_LOAD:
			{
				assert(!is_writing);
				reg_value = re32(Memory.ReadMMIO32(addr));
				save_reg = true;
				break;
			}
			case X64OP_STORE:
			{
				assert(is_writing);
				Memory.WriteMMIO32(addr, re32((u32)reg_value));
				break;
			}
			default: assert(!"Invalid x64_op_t value");
			}
			
			// save x64 reg value (for load operations)
			if (save_reg)
			{
				if (reg - X64R32 < 16)
				{
					// store the value into x64 register
					(&pExp->ContextRecord->Rax)[reg - X64R32] = (u32)reg_value;
				}
				else
				{
					assert(!"Invalid x64_reg_t value (saving)");
				}
			}

			// skip decoded instruction
			pExp->ContextRecord->Rip += size;
			// restore context (further code shouldn't be reached)
			RtlRestoreContext(pExp->ContextRecord, pExp->ExceptionRecord);

			// it's dangerous because destructors won't be executed
		}
		// TODO: allow recovering from a page fault as a feature of PS3 virtual memory
		throw fmt::Format("Access violation %s location 0x%x (is_alive=%d, last_syscall=0x%llx (%s))",
			is_writing ? "writing" : "reading", (u32)addr, t->IsAlive() ? 1 : 0, t->m_last_syscall, SysCalls::GetHLEFuncName((u32)t->m_last_syscall).c_str());
	}

	// else some fatal error (should crash)
}
#else
// TODO: linux version
#endif

void CPUThread::Task()
{
	if (Ini.HLELogging.GetValue()) LOG_NOTICE(GENERAL, "%s enter", CPUThread::GetFName().c_str());

	const std::vector<u64>& bp = Emu.GetBreakPoints();

	for (uint i = 0; i<bp.size(); ++i)
	{
		if (bp[i] == m_offset + PC)
		{
			Emu.Pause();
			break;
		}
	}

	std::vector<u32> trace;

#ifdef _WIN32
	auto old_se_translator = _set_se_translator(_se_translator);
#else
	// TODO: linux version
#endif

	try
	{
		while (true)
		{
			int status = ThreadStatus();

			if (status == CPUThread_Stopped || status == CPUThread_Break)
			{
				break;
			}

			if (status == CPUThread_Sleeping)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
				continue;
			}

			Step();
			//if (m_trace_enabled) trace.push_back(PC);
			NextPc(m_dec->DecodeMemory(PC + m_offset));

			if (status == CPUThread_Step)
			{
				m_is_step = false;
				break;
			}

			for (uint i = 0; i < bp.size(); ++i)
			{
				if (bp[i] == PC)
				{
					Emu.Pause();
					break;
				}
			}
		}
	}
	catch (const std::string& e)
	{
		LOG_ERROR(GENERAL, "Exception: %s", e.c_str());
		Emu.Pause();
	}
	catch (const char* e)
	{
		LOG_ERROR(GENERAL, "Exception: %s", e);
		Emu.Pause();
	}

#ifdef _WIN32
	_set_se_translator(old_se_translator);
#else
	// TODO: linux version
#endif

	if (trace.size())
	{
		LOG_NOTICE(GENERAL, "Trace begin (%d elements)", trace.size());

		u32 start = trace[0], prev = trace[0] - 4;

		for (auto& v : trace) //LOG_NOTICE(GENERAL, "PC = 0x%x", v);
		{
			if (v - prev != 4)
			{
				LOG_NOTICE(GENERAL, "Trace: 0x%08x .. 0x%08x", start, prev);
				start = v;
			}
			prev = v;
		}

		LOG_NOTICE(GENERAL, "Trace end: 0x%08x .. 0x%08x", start, prev);
	}


	if (Ini.HLELogging.GetValue()) LOG_NOTICE(GENERAL, "%s leave", CPUThread::GetFName().c_str());
}
