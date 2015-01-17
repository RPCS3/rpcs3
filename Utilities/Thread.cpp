#include "stdafx.h"
#include "Log.h"
#include "rpcs3/Ini.h"
#include "Emu/System.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Thread.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#include <ucontext.h>
#endif

void SetCurrentThreadDebugName(const char* threadName)
{
#if defined(_MSC_VER) // this is VS-specific way to set thread names for the debugger

	#pragma pack(push,8)

	struct THREADNAME_INFO
	{
		DWORD dwType;
		LPCSTR szName;
		DWORD dwThreadID;
		DWORD dwFlags;
	} info;

	#pragma pack(pop)

	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = -1;
	info.dwFlags = 0;

	__try
	{
		RaiseException(0x406D1388, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}

#endif
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

	X64_IMM32,
};

enum x64_op_t : u32
{
	X64OP_LOAD, // obtain and put the value into x64 register (from Memory.ReadMMIO32, for example)
	X64OP_STORE, // take the value from x64 register or an immediate and use it (pass in Memory.WriteMMIO32, for example)
	// example: add eax,[rax] -> X64OP_LOAD_ADD (add the value to x64 register)
	// example: add [rax],eax -> X64OP_LOAD_ADD_STORE (this will probably never happen for MMIO registers)
};

void decode_x64_reg_op(const u8* code, x64_op_t& decoded_op, x64_reg_t& decoded_reg, size_t& decoded_size)
{
	// simple analysis of x64 code allows to reinterpret MOV or other instructions in any desired way
	decoded_size = 0;

	u8 rex = 0;
	u8 reg = 0; // set to 8 by REX prefix
	u8 pg2 = 0;

	// check prefixes:
	for (;; code++, decoded_size++)
	{
		switch (const u8 prefix = *code)
		{
		case 0xf0: throw fmt::Format("decode_x64_reg_op(%.16llXh): 0x%.2X (LOCK prefix) found", code - decoded_size, prefix); // group 1
		case 0xf2: throw fmt::Format("decode_x64_reg_op(%.16llXh): 0x%.2X (REPNE/REPNZ prefix) found", code - decoded_size, prefix); // group 1
		case 0xf3: throw fmt::Format("decode_x64_reg_op(%.16llXh): 0x%.2X (REP/REPE/REPZ prefix) found", code - decoded_size, prefix); // group 1

		case 0x2e: // group 2
		case 0x36:
		case 0x3e:
		case 0x26:
		case 0x64:
		case 0x65:
		{
			if (!pg2)
			{
				pg2 = prefix; // probably, segment register
				continue;
			}
			else
			{
				throw fmt::Format("decode_x64_reg_op(%.16llXh): 0x%.2X (group 2 prefix) found after 0x%.2X", code - decoded_size, prefix, pg2);
			}
		}

		case 0x66: throw fmt::Format("decode_x64_reg_op(%.16llXh): 0x%.2X (operand-size override prefix) found", code - decoded_size, prefix); // group 3
		case 0x67: throw fmt::Format("decode_x64_reg_op(%.16llXh): 0x%.2X (address-size override prefix) found", code - decoded_size, prefix); // group 4

		default:
		{
			if ((prefix & 0xf0) == 0x40) // check REX prefix
			{
				if (rex)
				{
					throw fmt::Format("decode_x64_reg_op(%.16llXh): 0x%.2X (REX prefix) found after 0x%.2X", code - decoded_size, prefix, rex);
				}
				if (prefix & 0x80) // check REX.W bit
				{
					throw fmt::Format("decode_x64_reg_op(%.16llXh): 0x%.2X (REX.W bit) found", code - decoded_size, prefix);
				}
				if (prefix & 0x04) // check REX.R bit
				{
					reg = 8;
				}
				rex = prefix;
				continue;
			}
		}
		}

		break;
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
		if (get_modRM_r32(code, 0) == X64R_EAX) // MOV r/m32, imm32 (not tested)
		{
			decoded_op = X64OP_STORE;
			decoded_reg = X64_IMM32;
			decoded_size = get_modRM_size(code) + 4;
			return;
		}
	}
	default:
	{
		throw fmt::Format("decode_x64_reg_op(%.16llXh): unsupported opcode found (0x%.2X, 0x%.2X, 0x%.2X)", code - decoded_size, op1, code[0], code[1]);
	}
	}
}

#ifdef _WIN32

void _se_translator(unsigned int u, EXCEPTION_POINTERS* pExp)
{
	const u64 addr64 = (u64)pExp->ExceptionRecord->ExceptionInformation[1] - (u64)Memory.GetBaseAddr();
	const bool is_writing = pExp->ExceptionRecord->ExceptionInformation[0] != 0;
	if (u == EXCEPTION_ACCESS_VIOLATION && addr64 < 0x100000000ull)
	{
		const u32 addr = (u32)addr64;
		if (addr - RAW_SPU_BASE_ADDR < (6 * RAW_SPU_OFFSET) && (addr % RAW_SPU_OFFSET) >= RAW_SPU_PROB_OFFSET) // RawSPU MMIO registers
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
			RtlRestoreContext(pExp->ContextRecord, nullptr);

			// it's dangerous because destructors won't be executed
		}
		// TODO: allow recovering from a page fault as a feature of PS3 virtual memory
		throw fmt::Format("Access violation %s location 0x%x", is_writing ? "writing" : "reading", addr);
	}

	// else some fatal error (should crash)
}

#else

typedef decltype(REG_RIP) reg_table_t;
static const reg_table_t reg_table[16] =
{
	REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RBP, REG_RSI, REG_RDI,
	REG_R8, REG_R9, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15
};

void signal_handler(int sig, siginfo_t* info, void* uct)
{
	ucontext_t* const ctx = (ucontext_t*)uct;
	const u64 addr64 = (u64)info->si_addr - (u64)Memory.GetBaseAddr();
	//const bool is_writing = false; // TODO: get it correctly
	if (addr64 < 0x100000000ull && GetCurrentNamedThread())
	{
		const u32 addr = (u32)addr64;
		if (addr - RAW_SPU_BASE_ADDR < (6 * RAW_SPU_OFFSET) && (addr % RAW_SPU_OFFSET) >= RAW_SPU_PROB_OFFSET) // RawSPU MMIO registers
		{
			// one x64 instruction is manually decoded and interpreted
			x64_op_t op;
			x64_reg_t reg;
			size_t size;
			decode_x64_reg_op((const u8*)ctx->uc_mcontext.gregs[REG_RIP], op, reg, size);

			// get x64 reg value (for store operations)
			u64 reg_value;
			if (reg - X64R32 < 16)
			{
				// load the value from x64 register
				reg_value = (u32)ctx->uc_mcontext.gregs[reg_table[reg - X64R32]];
			}
			else if (reg == X64_IMM32)
			{
				// load the immediate value (assuming it's at the end of the instruction)
				reg_value = *(u32*)(ctx->uc_mcontext.gregs[REG_RIP] + size - 4);
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
				//assert(!is_writing);
				reg_value = re32(Memory.ReadMMIO32(addr));
				save_reg = true;
				break;
			}
			case X64OP_STORE:
			{
				//assert(is_writing);
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
					ctx->uc_mcontext.gregs[reg_table[reg - X64R32]] = (u32)reg_value;
				}
				else
				{
					assert(!"Invalid x64_reg_t value (saving)");
				}
			}

			// skip decoded instruction
			ctx->uc_mcontext.gregs[REG_RIP] += size;

			return; // now execution should proceed
			//setcontext(ctx);
		}

		// TODO: allow recovering from a page fault as a feature of PS3 virtual memory
		throw fmt::Format("Access violation %s location 0x%x", /*is_writing ? "writing" : "reading"*/ "at", addr);
	}

	// else some fatal error
	exit(EXIT_FAILURE);
}

const int sigaction_result = []() -> int
{
	struct sigaction sa;

	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = signal_handler;
	return sigaction(SIGSEGV, &sa, NULL);
}();

#endif

thread_local NamedThreadBase* g_tls_this_thread = nullptr;
std::atomic<u32> g_thread_count(0);

NamedThreadBase* GetCurrentNamedThread()
{
	return g_tls_this_thread;
}

void SetCurrentNamedThread(NamedThreadBase* value)
{
	const auto old_value = g_tls_this_thread;

	if (old_value == value)
	{
		return;
	}

	if (value && value->m_tls_assigned.exchange(true))
	{
		LOG_ERROR(GENERAL, "Thread '%s' was already assigned to g_tls_this_thread of another thread", value->GetThreadName().c_str());
		g_tls_this_thread = nullptr;
	}
	else
	{
		g_tls_this_thread = value;
	}

	if (old_value)
	{
		old_value->m_tls_assigned = false;
	}
}

std::string NamedThreadBase::GetThreadName() const
{
	return m_name;
}

void NamedThreadBase::SetThreadName(const std::string& name)
{
	m_name = name;
}

void NamedThreadBase::WaitForAnySignal(u64 time) // wait for Notify() signal or sleep
{
	std::unique_lock<std::mutex> lock(m_signal_mtx);
	m_signal_cv.wait_for(lock, std::chrono::milliseconds(time));
}

void NamedThreadBase::Notify() // wake up waiting thread or nothing
{
	m_signal_cv.notify_one();
}

ThreadBase::ThreadBase(const std::string& name)
	: NamedThreadBase(name)
	, m_executor(nullptr)
	, m_destroy(false)
	, m_alive(false)
{
}

ThreadBase::~ThreadBase()
{
	if(IsAlive())
		Stop(false);

	delete m_executor;
	m_executor = nullptr;
}

void ThreadBase::Start()
{
	if(m_executor) Stop();

	std::lock_guard<std::mutex> lock(m_main_mutex);

	m_destroy = false;
	m_alive = true;

	m_executor = new std::thread([this]()
	{
		SetCurrentThreadDebugName(GetThreadName().c_str());

#ifdef _WIN32
		auto old_se_translator = _set_se_translator(_se_translator);
#else
		if (sigaction_result == -1) assert(!"sigaction() failed");
#endif

		SetCurrentNamedThread(this);
		g_thread_count++;

		try
		{
			Task();
		}
		catch (const char* e)
		{
			LOG_ERROR(GENERAL, "%s: %s", GetThreadName().c_str(), e);
		}
		catch (const std::string& e)
		{
			LOG_ERROR(GENERAL, "%s: %s", GetThreadName().c_str(), e.c_str());
		}

		m_alive = false;
		SetCurrentNamedThread(nullptr);
		g_thread_count--;

#ifdef _WIN32
		_set_se_translator(old_se_translator);
#endif
	});
}

void ThreadBase::Stop(bool wait, bool send_destroy)
{
	std::lock_guard<std::mutex> lock(m_main_mutex);

	if (send_destroy)
		m_destroy = true;

	if(!m_executor)
		return;

	if(wait && m_executor->joinable() && m_alive)
	{
		m_executor->join();
	}
	else
	{
		m_executor->detach();
	}

	delete m_executor;
	m_executor = nullptr;
}

bool ThreadBase::Join() const
{
	std::lock_guard<std::mutex> lock(m_main_mutex);
	if(m_executor->joinable() && m_alive && m_executor != nullptr)
	{
		m_executor->join();
		return true;
	}

	return false;
}

bool ThreadBase::IsAlive() const
{
	std::lock_guard<std::mutex> lock(m_main_mutex);
	return m_alive;
}

bool ThreadBase::TestDestroy() const
{
	return m_destroy;
}

thread_t::thread_t(const std::string& name, bool autojoin, std::function<void()> func)
	: m_name(name)
	, m_state(TS_NON_EXISTENT)
	, m_autojoin(autojoin)
{
	start(func);
}

thread_t::thread_t(const std::string& name, std::function<void()> func)
	: m_name(name)
	, m_state(TS_NON_EXISTENT)
	, m_autojoin(false)
{
	start(func);
}

thread_t::thread_t(const std::string& name)
	: m_name(name)
	, m_state(TS_NON_EXISTENT)
	, m_autojoin(false)
{
}

thread_t::thread_t()
	: m_state(TS_NON_EXISTENT)
	, m_autojoin(false)
{
}

void thread_t::set_name(const std::string& name)
{
	m_name = name;
}

thread_t::~thread_t()
{
	if (m_state == TS_JOINABLE)
	{
		if (m_autojoin)
		{
			m_thr.join();
		}
		else
		{
			m_thr.detach();
		}
	}
}

void thread_t::start(std::function<void()> func)
{
	if (m_state.exchange(TS_NON_EXISTENT) == TS_JOINABLE)
	{
		m_thr.join(); // forcefully join previously created thread
	}

	std::string name = m_name;
	m_thr = std::thread([func, name]()
	{
		SetCurrentThreadDebugName(name.c_str());

#ifdef _WIN32
		auto old_se_translator = _set_se_translator(_se_translator);
#else
		if (sigaction_result == -1) assert(!"sigaction() failed");
#endif

		NamedThreadBase info(name);
		SetCurrentNamedThread(&info);
		g_thread_count++;

		if (Ini.HLELogging.GetValue())
		{
			LOG_NOTICE(HLE, name + " started");
		}

		try
		{
			func();
		}
		catch (const char* e)
		{
			LOG_ERROR(GENERAL, "%s: %s", name.c_str(), e);
		}
		catch (const std::string& e)
		{
			LOG_ERROR(GENERAL, "%s: %s", name.c_str(), e.c_str());
		}

		if (Emu.IsStopped())
		{
			LOG_NOTICE(HLE, name + " aborted");
		}
		else if (Ini.HLELogging.GetValue())
		{
			LOG_NOTICE(HLE, name + " ended");
		}

		SetCurrentNamedThread(nullptr);
		g_thread_count--;

#ifdef _WIN32
		_set_se_translator(old_se_translator);
#endif
	});

	if (m_state.exchange(TS_JOINABLE) == TS_JOINABLE)
	{
		assert(!"thread_t::start() failed"); // probably started from another thread
	}
}

void thread_t::detach()
{
	if (m_state.exchange(TS_NON_EXISTENT) == TS_JOINABLE)
	{
		m_thr.detach();
	}
	else
	{
		assert(!"thread_t::detach() failed"); // probably joined or detached
	}
}

void thread_t::join()
{
	if (m_state.exchange(TS_NON_EXISTENT) == TS_JOINABLE)
	{
		m_thr.join();
	}
	else
	{
		assert(!"thread_t::join() failed"); // probably joined or detached
	}
}

bool thread_t::joinable() const
{
	//return m_thr.joinable();
	return m_state == TS_JOINABLE;
}

bool waiter_map_t::is_stopped(u64 signal_id)
{
	if (Emu.IsStopped())
	{
		LOG_WARNING(Log::HLE, "%s: waiter_op() aborted (signal_id=0x%llx)", m_name.c_str(), signal_id);
		return true;
	}
	return false;
}

void waiter_map_t::waiter_reg_t::init()
{
	if (!thread)
	{
		thread = GetCurrentNamedThread();

		std::lock_guard<std::mutex> lock(map.m_mutex);

		// add waiter
		map.m_waiters.push_back({ signal_id, thread });
	}
}

waiter_map_t::waiter_reg_t::~waiter_reg_t()
{
	if (thread)
	{
		std::lock_guard<std::mutex> lock(map.m_mutex);

		// remove waiter
		for (s64 i = map.m_waiters.size() - 1; i >= 0; i--)
		{
			if (map.m_waiters[i].signal_id == signal_id && map.m_waiters[i].thread == thread)
			{
				map.m_waiters.erase(map.m_waiters.begin() + i);
				return;
			}
		}

		LOG_ERROR(HLE, "%s(): waiter not found (signal_id=0x%llx, map='%s')", __FUNCTION__, signal_id, map.m_name.c_str());
		Emu.Pause();
	}
}

void waiter_map_t::notify(u64 signal_id)
{
	if (m_waiters.size())
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		// find waiter and signal
		for (auto& v : m_waiters)
		{
			if (v.signal_id == signal_id)
			{
				v.thread->Notify();
			}
		}
	}
}

static const std::function<bool()> SQUEUE_ALWAYS_EXIT = [](){ return true; };
static const std::function<bool()> SQUEUE_NEVER_EXIT = [](){ return false; };

bool squeue_test_exit()
{
	return Emu.IsStopped();
}
