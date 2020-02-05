#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/Cell/lv2/sys_mmapper.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "Thread.h"
#include "sysinfo.h"
#include <typeinfo>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#include <Psapi.h>
#include <process.h>
#include <sysinfoapi.h>
#else
#ifdef __APPLE__
#define _XOPEN_SOURCE
#define __USE_GNU
#include <mach/thread_act.h>
#include <mach/thread_policy.h>
#endif
#if defined(__DragonFly__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#include <pthread_np.h>
#define cpu_set_t cpuset_t
#endif
#include <errno.h>
#include <signal.h>
#ifndef __OpenBSD__
#include <ucontext.h>
#endif
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#endif
#ifdef __linux__
#include <sys/timerfd.h>
#endif

#include "sync.h"
#include "Log.h"

LOG_CHANNEL(sig_log);
LOG_CHANNEL(vm_log, "VM");

thread_local u64 g_tls_fault_all = 0;
thread_local u64 g_tls_fault_rsx = 0;
thread_local u64 g_tls_fault_spu = 0;
extern thread_local std::string(*g_tls_log_prefix)();

[[noreturn]] void catch_all_exceptions()
{
	try
	{
		throw;
	}
	catch (const std::exception& e)
	{
		report_fatal_error("{" + g_tls_log_prefix() + "} Unhandled exception of type '"s + typeid(e).name() + "': "s + e.what());
	}
	catch (...)
	{
		report_fatal_error("{" + g_tls_log_prefix() + "} Unhandled exception (unknown)");
	}
}

enum x64_reg_t : u32
{
	X64R_RAX = 0,
	X64R_RCX,
	X64R_RDX,
	X64R_RBX,
	X64R_RSP,
	X64R_RBP,
	X64R_RSI,
	X64R_RDI,
	X64R_R8,
	X64R_R9,
	X64R_R10,
	X64R_R11,
	X64R_R12,
	X64R_R13,
	X64R_R14,
	X64R_R15,

	X64R_XMM0 = 0,
	X64R_XMM1,
	X64R_XMM2,
	X64R_XMM3,
	X64R_XMM4,
	X64R_XMM5,
	X64R_XMM6,
	X64R_XMM7,
	X64R_XMM8,
	X64R_XMM9,
	X64R_XMM10,
	X64R_XMM11,
	X64R_XMM12,
	X64R_XMM13,
	X64R_XMM14,
	X64R_XMM15,

	X64R_AL,
	X64R_CL,
	X64R_DL,
	X64R_BL,
	X64R_AH,
	X64R_CH,
	X64R_DH,
	X64R_BH,

	X64_NOT_SET,
	X64_IMM8,
	X64_IMM16,
	X64_IMM32,

	X64_BIT_O = 0x90,
	X64_BIT_NO,
	X64_BIT_C,
	X64_BIT_NC,
	X64_BIT_Z,
	X64_BIT_NZ,
	X64_BIT_BE,
	X64_BIT_NBE,
	X64_BIT_S,
	X64_BIT_NS,
	X64_BIT_P,
	X64_BIT_NP,
	X64_BIT_L,
	X64_BIT_NL,
	X64_BIT_LE,
	X64_BIT_NLE,

	X64R_ECX = X64R_CL,
};

enum x64_op_t : u32
{
	X64OP_NONE,
	X64OP_LOAD, // obtain and put the value into x64 register
	X64OP_LOAD_BE,
	X64OP_LOAD_CMP,
	X64OP_LOAD_TEST,
	X64OP_STORE, // take the value from x64 register or an immediate and use it
	X64OP_STORE_BE,
	X64OP_MOVS,
	X64OP_STOS,
	X64OP_XCHG,
	X64OP_CMPXCHG,
	X64OP_AND, // lock and [mem], ...
	X64OP_OR,  // lock or  [mem], ...
	X64OP_XOR, // lock xor [mem], ...
	X64OP_INC, // lock inc [mem]
	X64OP_DEC, // lock dec [mem]
	X64OP_ADD, // lock add [mem], ...
	X64OP_ADC, // lock adc [mem], ...
	X64OP_SUB, // lock sub [mem], ...
	X64OP_SBB, // lock sbb [mem], ...
};

void decode_x64_reg_op(const u8* code, x64_op_t& out_op, x64_reg_t& out_reg, size_t& out_size, size_t& out_length)
{
	// simple analysis of x64 code allows to reinterpret MOV or other instructions in any desired way
	out_length = 0;

	u8 rex = 0, pg2 = 0;

	bool oso = false, lock = false, repne = false, repe = false;

	enum : u8
	{
		LOCK  = 0xf0,
		REPNE = 0xf2,
		REPE  = 0xf3,
	};

	// check prefixes:
	for (;; code++, out_length++)
	{
		switch (const u8 prefix = *code)
		{
		case LOCK: // group 1
		{
			if (lock)
			{
				sig_log.error("decode_x64_reg_op(%016llxh): LOCK prefix found twice", code - out_length);
			}

			lock = true;
			continue;
		}
		case REPNE: // group 1
		{
			if (repne)
			{
				sig_log.error("decode_x64_reg_op(%016llxh): REPNE/REPNZ prefix found twice", code - out_length);
			}

			repne = true;
			continue;
		}
		case REPE: // group 1
		{
			if (repe)
			{
				sig_log.error("decode_x64_reg_op(%016llxh): REP/REPE/REPZ prefix found twice", code - out_length);
			}

			repe = true;
			continue;
		}

		case 0x2e: // group 2
		case 0x36:
		case 0x3e:
		case 0x26:
		case 0x64:
		case 0x65:
		{
			if (pg2)
			{
				sig_log.error("decode_x64_reg_op(%016llxh): 0x%02x (group 2 prefix) found after 0x%02x", code - out_length, prefix, pg2);
			}
			else
			{
				pg2 = prefix; // probably, segment register
			}
			continue;
		}

		case 0x66: // group 3
		{
			if (oso)
			{
				sig_log.error("decode_x64_reg_op(%016llxh): operand-size override prefix found twice", code - out_length);
			}

			oso = true;
			continue;
		}

		case 0x67: // group 4
		{
			sig_log.error("decode_x64_reg_op(%016llxh): address-size override prefix found", code - out_length, prefix);
			out_op = X64OP_NONE;
			out_reg = X64_NOT_SET;
			out_size = 0;
			out_length = 0;
			return;
		}

		default:
		{
			if ((prefix & 0xf0) == 0x40) // check REX prefix
			{
				if (rex)
				{
					sig_log.error("decode_x64_reg_op(%016llxh): 0x%02x (REX prefix) found after 0x%02x", code - out_length, prefix, rex);
				}
				else
				{
					rex = prefix;
				}
				continue;
			}
		}
		}

		break;
	}

	auto get_modRM_reg = [](const u8* code, const u8 rex) -> x64_reg_t
	{
		return x64_reg_t{((*code & 0x38) >> 3 | (/* check REX.R bit */ rex & 4 ? 8 : 0)) + X64R_RAX};
	};

	auto get_modRM_reg_xmm = [](const u8* code, const u8 rex) -> x64_reg_t
	{
		return x64_reg_t{((*code & 0x38) >> 3 | (/* check REX.R bit */ rex & 4 ? 8 : 0)) + X64R_XMM0};
	};

	auto get_modRM_reg_lh = [](const u8* code) -> x64_reg_t
	{
		return x64_reg_t{((*code & 0x38) >> 3) + X64R_AL};
	};

	auto get_op_size = [](const u8 rex, const bool oso) -> size_t
	{
		return rex & 8 ? 8 : (oso ? 2 : 4);
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

	const u8 op1 = (out_length++, *code++), op2 = code[0], op3 = code[1];

	switch (op1)
	{
	case 0x0f:
	{
		out_length++, code++;

		switch (op2)
		{
		case 0x11:
		case 0x29:
		{
			if (!repe && !repne) // MOVUPS/MOVAPS/MOVUPD/MOVAPD xmm/m, xmm
			{
				out_op = X64OP_STORE;
				out_reg = get_modRM_reg_xmm(code, rex);
				out_size = 16;
				out_length += get_modRM_size(code);
				return;
			}
			break;
		}
		case 0x7f:
		{
			if ((repe && !oso) || (!repe && oso)) // MOVDQU/MOVDQA xmm/m, xmm
			{
				out_op = X64OP_STORE;
				out_reg = get_modRM_reg_xmm(code, rex);
				out_size = 16;
				out_length += get_modRM_size(code);
				return;
			}
			break;
		}
		case 0xb0:
		{
			if (!oso) // CMPXCHG r8/m8, r8
			{
				out_op = X64OP_CMPXCHG;
				out_reg = rex & 8 ? get_modRM_reg(code, rex) : get_modRM_reg_lh(code);
				out_size = 1;
				out_length += get_modRM_size(code);
				return;
			}
			break;
		}
		case 0xb1:
		{
			if (true) // CMPXCHG r/m, r (16, 32, 64)
			{
				out_op = X64OP_CMPXCHG;
				out_reg = get_modRM_reg(code, rex);
				out_size = get_op_size(rex, oso);
				out_length += get_modRM_size(code);
				return;
			}
			break;
		}
		case 0x90:
		case 0x91:
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x96:
		case 0x97:
		case 0x98:
		case 0x9a:
		case 0x9b:
		case 0x9c:
		case 0x9d:
		case 0x9e:
		case 0x9f:
		{
			if (!lock) // SETcc
			{
				out_op = X64OP_STORE;
				out_reg = x64_reg_t(X64_BIT_O + op2 - 0x90); // 0x90 .. 0x9f
				out_size = 1;
				out_length += get_modRM_size(code);
				return;
			}
			break;
		}
		case 0x38:
		{
			out_length++, code++;

			switch (op3)
			{
			case 0xf0:
			case 0xf1:
			{
				if (!repne) // MOVBE
				{
					out_op = op3 == 0xf0 ? X64OP_LOAD_BE : X64OP_STORE_BE;
					out_reg = get_modRM_reg(code, rex);
					out_size = get_op_size(rex, oso);
					out_length += get_modRM_size(code);
					return;
				}

				break;
			}
			}

			break;
		}
		}

		break;
	}
	case 0x20:
	{
		if (!oso)
		{
			out_op = X64OP_AND;
			out_reg = rex & 8 ? get_modRM_reg(code, rex) : get_modRM_reg_lh(code);
			out_size = 1;
			out_length += get_modRM_size(code);
			return;
		}
		break;
	}
	case 0x21:
	{
		if (true)
		{
			out_op = X64OP_AND;
			out_reg = get_modRM_reg(code, rex);
			out_size = get_op_size(rex, oso);
			out_length += get_modRM_size(code);
			return;
		}
		break;
	}
	case 0x80:
	{
		switch (auto mod_code = get_modRM_reg(code, 0))
		{
		//case 0: out_op = X64OP_ADD; break; // TODO: strange info in instruction manual
		case 1: out_op = X64OP_OR; break;
		case 2: out_op = X64OP_ADC; break;
		case 3: out_op = X64OP_SBB; break;
		case 4: out_op = X64OP_AND; break;
		case 5: out_op = X64OP_SUB; break;
		case 6: out_op = X64OP_XOR; break;
		default: out_op = X64OP_LOAD_CMP; break;
		}

		out_reg = X64_IMM8;
		out_size = 1;
		out_length += get_modRM_size(code) + 1;
		return;
	}
	case 0x81:
	{
		switch (auto mod_code = get_modRM_reg(code, 0))
		{
		case 0: out_op = X64OP_ADD; break;
		case 1: out_op = X64OP_OR; break;
		case 2: out_op = X64OP_ADC; break;
		case 3: out_op = X64OP_SBB; break;
		case 4: out_op = X64OP_AND; break;
		case 5: out_op = X64OP_SUB; break;
		case 6: out_op = X64OP_XOR; break;
		default: out_op = X64OP_LOAD_CMP; break;
		}

		out_reg = oso ? X64_IMM16 : X64_IMM32;
		out_size = get_op_size(rex, oso);
		out_length += get_modRM_size(code) + (oso ? 2 : 4);
		return;
	}
	case 0x83:
	{
		switch (auto mod_code = get_modRM_reg(code, 0))
		{
		case 0: out_op = X64OP_ADD; break;
		case 1: out_op = X64OP_OR; break;
		case 2: out_op = X64OP_ADC; break;
		case 3: out_op = X64OP_SBB; break;
		case 4: out_op = X64OP_AND; break;
		case 5: out_op = X64OP_SUB; break;
		case 6: out_op = X64OP_XOR; break;
		default: out_op = X64OP_LOAD_CMP; break;
		}

		out_reg = X64_IMM8;
		out_size = get_op_size(rex, oso);
		out_length += get_modRM_size(code) + 1;
		return;
	}
	case 0x86:
	{
		if (!oso) // XCHG r8/m8, r8
		{
			out_op = X64OP_XCHG;
			out_reg = rex & 8 ? get_modRM_reg(code, rex) : get_modRM_reg_lh(code);
			out_size = 1;
			out_length += get_modRM_size(code);
			return;
		}
		break;
	}
	case 0x87:
	{
		if (true) // XCHG r/m, r (16, 32, 64)
		{
			out_op = X64OP_XCHG;
			out_reg = get_modRM_reg(code, rex);
			out_size = get_op_size(rex, oso);
			out_length += get_modRM_size(code);
			return;
		}
		break;
	}
	case 0x88:
	{
		if (!lock && !oso) // MOV r8/m8, r8
		{
			out_op = X64OP_STORE;
			out_reg = rex & 8 ? get_modRM_reg(code, rex) : get_modRM_reg_lh(code);
			out_size = 1;
			out_length += get_modRM_size(code);
			return;
		}
		break;
	}
	case 0x89:
	{
		if (!lock) // MOV r/m, r (16, 32, 64)
		{
			out_op = X64OP_STORE;
			out_reg = get_modRM_reg(code, rex);
			out_size = get_op_size(rex, oso);
			out_length += get_modRM_size(code);
			return;
		}
		break;
	}
	case 0x8a:
	{
		if (!lock && !oso) // MOV r8, r8/m8
		{
			out_op = X64OP_LOAD;
			out_reg = rex & 8 ? get_modRM_reg(code, rex) : get_modRM_reg_lh(code);
			out_size = 1;
			out_length += get_modRM_size(code);
			return;
		}
		break;
	}
	case 0x8b:
	{
		if (!lock) // MOV r, r/m (16, 32, 64)
		{
			out_op = X64OP_LOAD;
			out_reg = get_modRM_reg(code, rex);
			out_size = get_op_size(rex, oso);
			out_length += get_modRM_size(code);
			return;
		}
		break;
	}
	case 0xa4:
	{
		if (!oso && !lock && !repe && !rex) // MOVS
		{
			out_op = X64OP_MOVS;
			out_reg = X64_NOT_SET;
			out_size = 1;
			return;
		}
		if (!oso && !lock && repe) // REP MOVS
		{
			out_op = X64OP_MOVS;
			out_reg = rex & 8 ? X64R_RCX : X64R_ECX;
			out_size = 1;
			return;
		}
		break;
	}
	case 0xaa:
	{
		if (!oso && !lock && !repe && !rex) // STOS
		{
			out_op = X64OP_STOS;
			out_reg = X64_NOT_SET;
			out_size = 1;
			return;
		}
		if (!oso && !lock && repe) // REP STOS
		{
			out_op = X64OP_STOS;
			out_reg = rex & 8 ? X64R_RCX : X64R_ECX;
			out_size = 1;
			return;
		}
		break;
	}
	case 0xc4: // 3-byte VEX prefix
	case 0xc5: // 2-byte VEX prefix
	{
		// Last prefix byte: op2 or op3
		const u8 opx = op1 == 0xc5 ? op2 : op3;

		// Implied prefixes
		rex |= op2 & 0x80 ? 0 : 0x4; // REX.R
		rex |= op1 == 0xc4 && op3 & 0x80 ? 0x8 : 0; // REX.W ???
		oso = (opx & 0x3) == 0x1;
		repe = (opx & 0x3) == 0x2;
		repne = (opx & 0x3) == 0x3;

		const u8 vopm = op1 == 0xc5 ? 1 : op2 & 0x1f;
		const u8 vop1 = op1 == 0xc5 ? op3 : code[2];
		const u8 vlen = (opx & 0x4) ? 32 : 16;
		const u8 vreg = (~opx >> 3) & 0xf;
		out_length += op1 == 0xc5 ? 2 : 3;
		code += op1 == 0xc5 ? 2 : 3;

		if (vopm == 0x1) switch (vop1) // Implied leading byte 0x0F
		{
		case 0x11:
		case 0x29:
		{
			if (!repe && !repne) // VMOVAPS/VMOVAPD/VMOVUPS/VMOVUPD mem,reg
			{
				out_op = X64OP_STORE;
				out_reg = get_modRM_reg_xmm(code, rex);
				out_size = vlen;
				out_length += get_modRM_size(code);
				return;
			}
			break;
		}
		case 0x7f:
		{
			if (repe || oso) // VMOVDQU/VMOVDQA mem,reg
			{
				out_op = X64OP_STORE;
				out_reg = get_modRM_reg_xmm(code, rex);
				out_size = vlen;
				out_length += get_modRM_size(code);
				return;
			}
			break;
		}
		}

		break;
	}
	case 0xc6:
	{
		if (!lock && !oso && get_modRM_reg(code, 0) == 0) // MOV r8/m8, imm8
		{
			out_op = X64OP_STORE;
			out_reg = X64_IMM8;
			out_size = 1;
			out_length += get_modRM_size(code) + 1;
			return;
		}
		break;
	}
	case 0xc7:
	{
		if (!lock && get_modRM_reg(code, 0) == 0) // MOV r/m, imm16/imm32 (16, 32, 64)
		{
			out_op = X64OP_STORE;
			out_reg = oso ? X64_IMM16 : X64_IMM32;
			out_size = get_op_size(rex, oso);
			out_length += get_modRM_size(code) + (oso ? 2 : 4);
			return;
		}
		break;
	}
	case 0xf6:
	{
		switch (auto mod_code = get_modRM_reg(code, 0))
		{
		case 0: out_op = X64OP_LOAD_TEST; break;
		default: out_op = X64OP_NONE; break; // TODO...
		}

		out_reg = X64_IMM8;
		out_size = 1;
		out_length += get_modRM_size(code) + 1;
		return;
	}
	case 0xf7:
	{
		switch (auto mod_code = get_modRM_reg(code, 0))
		{
		case 0: out_op = X64OP_LOAD_TEST; break;
		default: out_op = X64OP_NONE; break; // TODO...
		}

		out_reg = oso ? X64_IMM16 : X64_IMM32;
		out_size = get_op_size(rex, oso);
		out_length += get_modRM_size(code) + (oso ? 2 : 4);
		return;
	}
	}

	out_op = X64OP_NONE;
	out_reg = X64_NOT_SET;
	out_size = 0;
	out_length = 0;
}

#ifdef _WIN32

typedef CONTEXT x64_context;

#define X64REG(context, reg) (&(&(context)->Rax)[reg])
#define XMMREG(context, reg) (reinterpret_cast<v128*>(&(&(context)->Xmm0)[reg]))
#define EFLAGS(context) ((context)->EFlags)

#define ARG1(context) RCX(context)
#define ARG2(context) RDX(context)

#else

typedef ucontext_t x64_context;

#ifdef __APPLE__

#define X64REG(context, reg) (darwin_x64reg(context, reg))
#define XMMREG(context, reg) (reinterpret_cast<v128*>(&(context)->uc_mcontext->__fs.__fpu_xmm0.__xmm_reg[reg]))
#define EFLAGS(context) ((context)->uc_mcontext->__ss.__rflags)

uint64_t* darwin_x64reg(x64_context *context, int reg)
{
	auto *state = &context->uc_mcontext->__ss;
	switch(reg)
	{
	case 0: return &state->__rax;
	case 1: return &state->__rcx;
	case 2: return &state->__rdx;
	case 3: return &state->__rbx;
	case 4: return &state->__rsp;
	case 5: return &state->__rbp;
	case 6: return &state->__rsi;
	case 7: return &state->__rdi;
	case 8: return &state->__r8;
	case 9: return &state->__r9;
	case 10: return &state->__r10;
	case 11: return &state->__r11;
	case 12: return &state->__r12;
	case 13: return &state->__r13;
	case 14: return &state->__r14;
	case 15: return &state->__r15;
	case 16: return &state->__rip;
	default:
		sig_log.error("Invalid register index: %d", reg);
		return nullptr;
	}
}

#elif defined(__DragonFly__) || defined(__FreeBSD__)

#define X64REG(context, reg) (freebsd_x64reg(context, reg))
#ifdef __DragonFly__
#  define XMMREG(context, reg) (reinterpret_cast<v128*>((reinterpret_cast<union savefpu*>(context)->uc_mcontext.mc_fpregs)->sv_xmm.sv_xmm[reg]))
#else
#  define XMMREG(context, reg) (reinterpret_cast<v128*>((reinterpret_cast<struct savefpu*>(context)->uc_mcontext.mc_fpstate)->sv_xmm[reg]))
#endif
#define EFLAGS(context) ((context)->uc_mcontext.mc_rflags)

register_t* freebsd_x64reg(x64_context *context, int reg)
{
	auto *state = &context->uc_mcontext;
	switch(reg)
	{
	case 0: return &state->mc_rax;
	case 1: return &state->mc_rcx;
	case 2: return &state->mc_rdx;
	case 3: return &state->mc_rbx;
	case 4: return &state->mc_rsp;
	case 5: return &state->mc_rbp;
	case 6: return &state->mc_rsi;
	case 7: return &state->mc_rdi;
	case 8: return &state->mc_r8;
	case 9: return &state->mc_r9;
	case 10: return &state->mc_r10;
	case 11: return &state->mc_r11;
	case 12: return &state->mc_r12;
	case 13: return &state->mc_r13;
	case 14: return &state->mc_r14;
	case 15: return &state->mc_r15;
	case 16: return &state->mc_rip;
	default:
		sig_log.error("Invalid register index: %d", reg);
		return nullptr;
	}
}

#elif defined(__OpenBSD__)

#define X64REG(context, reg) (openbsd_x64reg(context, reg))
#define XMMREG(context, reg) (reinterpret_cast<v128*>((context)->sc_fpstate->fx_xmm[reg]))
#define EFLAGS(context) ((context)->sc_rflags)

long* openbsd_x64reg(x64_context *context, int reg)
{
	auto *state = &context;
	switch(reg)
	{
	case 0: return &state->sc_rax;
	case 1: return &state->sc_rcx;
	case 2: return &state->sc_rdx;
	case 3: return &state->sc_rbx;
	case 4: return &state->sc_rsp;
	case 5: return &state->sc_rbp;
	case 6: return &state->sc_rsi;
	case 7: return &state->sc_rdi;
	case 8: return &state->sc_r8;
	case 9: return &state->sc_r9;
	case 10: return &state->sc_r10;
	case 11: return &state->sc_r11;
	case 12: return &state->sc_r12;
	case 13: return &state->sc_r13;
	case 14: return &state->sc_r14;
	case 15: return &state->sc_r15;
	case 16: return &state->sc_rip;
	default:
		sig_log.error("Invalid register index: %d", reg);
		return nullptr;
	}
}

#elif defined(__NetBSD__)

static const decltype(_REG_RAX) reg_table[] =
{
	_REG_RAX, _REG_RCX, _REG_RDX, _REG_RBX, _REG_RSP, _REG_RBP, _REG_RSI, _REG_RDI,
	_REG_R8, _REG_R9, _REG_R10, _REG_R11, _REG_R12, _REG_R13, _REG_R14, _REG_R15, _REG_RIP
};

#define X64REG(context, reg) (&(context)->uc_mcontext.__gregs[reg_table[reg]])
#define XMM_sig(context, reg) (reinterpret_cast<v128*>(((struct fxsave64*)(context)->uc_mcontext.__fpregs)->fx_xmm[reg]))
#define EFLAGS(context) ((context)->uc_mcontext.__gregs[_REG_RFL])

#else

static const int reg_table[] =
{
	REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RBP, REG_RSI, REG_RDI,
	REG_R8, REG_R9, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15, REG_RIP
};

#define X64REG(context, reg) (&(context)->uc_mcontext.gregs[reg_table[reg]])
#ifdef __sun
#define XMMREG(context, reg) (reinterpret_cast<v128*>(&(context)->uc_mcontext.fpregs.fp_reg_set.fpchip_state.xmm[reg_table[reg]]))
#else
#define XMMREG(context, reg) (reinterpret_cast<v128*>(&(context)->uc_mcontext.fpregs->_xmm[reg]))
#endif // __sun
#define EFLAGS(context) ((context)->uc_mcontext.gregs[REG_EFL])

#endif // __APPLE__

#define ARG1(context) RDI(context)
#define ARG2(context) RSI(context)

#endif

#define RAX(c) (*X64REG((c), 0))
#define RCX(c) (*X64REG((c), 1))
#define RDX(c) (*X64REG((c), 2))
#define RSP(c) (*X64REG((c), 4))
#define RSI(c) (*X64REG((c), 6))
#define RDI(c) (*X64REG((c), 7))
#define RIP(c) (*X64REG((c), 16))

bool get_x64_reg_value(x64_context* context, x64_reg_t reg, size_t d_size, size_t i_size, u64& out_value)
{
	// get x64 reg value (for store operations)
	if (reg - X64R_RAX < 16)
	{
		// load the value from x64 register
		const u64 reg_value = *X64REG(context, reg - X64R_RAX);

		switch (d_size)
		{
		case 1: out_value = static_cast<u8>(reg_value); return true;
		case 2: out_value = static_cast<u16>(reg_value); return true;
		case 4: out_value = static_cast<u32>(reg_value); return true;
		case 8: out_value = reg_value; return true;
		}
	}
	else if (reg - X64R_AL < 4 && d_size == 1)
	{
		out_value = static_cast<u8>(*X64REG(context, reg - X64R_AL));
		return true;
	}
	else if (reg - X64R_AH < 4 && d_size == 1)
	{
		out_value = static_cast<u8>(*X64REG(context, reg - X64R_AH) >> 8);
		return true;
	}
	else if (reg == X64_IMM8)
	{
		// load the immediate value (assuming it's at the end of the instruction)
		const s8 imm_value = *reinterpret_cast<s8*>(RIP(context) + i_size - 1);

		switch (d_size)
		{
		case 1: out_value = static_cast<u8>(imm_value); return true;
		case 2: out_value = static_cast<u16>(imm_value); return true; // sign-extended
		case 4: out_value = static_cast<u32>(imm_value); return true; // sign-extended
		case 8: out_value = static_cast<u64>(imm_value); return true; // sign-extended
		}
	}
	else if (reg == X64_IMM16)
	{
		const s16 imm_value = *reinterpret_cast<s16*>(RIP(context) + i_size - 2);

		switch (d_size)
		{
		case 2: out_value = static_cast<u16>(imm_value); return true;
		}
	}
	else if (reg == X64_IMM32)
	{
		const s32 imm_value = *reinterpret_cast<s32*>(RIP(context) + i_size - 4);

		switch (d_size)
		{
		case 4: out_value = static_cast<u32>(imm_value); return true;
		case 8: out_value = static_cast<u64>(imm_value); return true; // sign-extended
		}
	}
	else if (reg == X64R_ECX)
	{
		out_value = static_cast<u32>(RCX(context));
		return true;
	}
	else if (reg >= X64_BIT_O && reg <= X64_BIT_NLE)
	{
		const u32 _cf = EFLAGS(context) & 0x1;
		const u32 _zf = EFLAGS(context) & 0x40;
		const u32 _sf = EFLAGS(context) & 0x80;
		const u32 _of = EFLAGS(context) & 0x800;
		const u32 _pf = EFLAGS(context) & 0x4;
		const u32 _l = (_sf << 4) ^ _of; // SF != OF

		switch (reg & ~1)
		{
		case X64_BIT_O: out_value = !!_of ^ (reg & 1); break;
		case X64_BIT_C: out_value = !!_cf ^ (reg & 1); break;
		case X64_BIT_Z: out_value = !!_zf ^ (reg & 1); break;
		case X64_BIT_BE: out_value = !!(_cf | _zf) ^ (reg & 1); break;
		case X64_BIT_S: out_value = !!_sf ^ (reg & 1); break;
		case X64_BIT_P: out_value = !!_pf ^ (reg & 1); break;
		case X64_BIT_L: out_value = !!_l ^ (reg & 1); break;
		case X64_BIT_LE: out_value = !!(_l | _zf) ^ (reg & 1); break;
		}

		return true;
	}

	sig_log.error("get_x64_reg_value(): invalid arguments (reg=%d, d_size=%lld, i_size=%lld)", +reg, d_size, i_size);
	return false;
}

bool put_x64_reg_value(x64_context* context, x64_reg_t reg, size_t d_size, u64 value)
{
	// save x64 reg value (for load operations)
	if (reg - X64R_RAX < 16)
	{
		// save the value into x64 register
		switch (d_size)
		{
		case 1: *X64REG(context, reg - X64R_RAX) = (value & 0xff) | (*X64REG(context, reg - X64R_RAX) & 0xffffff00); return true;
		case 2: *X64REG(context, reg - X64R_RAX) = (value & 0xffff) | (*X64REG(context, reg - X64R_RAX) & 0xffff0000); return true;
		case 4: *X64REG(context, reg - X64R_RAX) = value & 0xffffffff; return true;
		case 8: *X64REG(context, reg - X64R_RAX) = value; return true;
		}
	}

	sig_log.error("put_x64_reg_value(): invalid destination (reg=%d, d_size=%lld, value=0x%llx)", +reg, d_size, value);
	return false;
}

bool set_x64_cmp_flags(x64_context* context, size_t d_size, u64 x, u64 y, bool carry = true)
{
	switch (d_size)
	{
	case 1: break;
	case 2: break;
	case 4: break;
	case 8: break;
	default: sig_log.error("set_x64_cmp_flags(): invalid d_size (%lld)", d_size); return false;
	}

	const u64 sign = 1ull << (d_size * 8 - 1); // sign mask
	const u64 diff = x - y;
	const u64 summ = x + y;

	if (carry && ((x & y) | ((x ^ y) & ~summ)) & sign)
	{
		EFLAGS(context) |= 0x1; // set CF
	}
	else if (carry)
	{
		EFLAGS(context) &= ~0x1; // clear CF
	}

	if (x == y)
	{
		EFLAGS(context) |= 0x40; // set ZF
	}
	else
	{
		EFLAGS(context) &= ~0x40; // clear ZF
	}

	if (diff & sign)
	{
		EFLAGS(context) |= 0x80; // set SF
	}
	else
	{
		EFLAGS(context) &= ~0x80; // clear SF
	}

	if ((x ^ summ) & (y ^ summ) & sign)
	{
		EFLAGS(context) |= 0x800; // set OF
	}
	else
	{
		EFLAGS(context) &= ~0x800; // clear OF
	}

	const u8 p1 = static_cast<u8>(diff) ^ (static_cast<u8>(diff) >> 4);
	const u8 p2 = p1 ^ (p1 >> 2);
	const u8 p3 = p2 ^ (p2 >> 1);

	if ((p3 & 1) == 0)
	{
		EFLAGS(context) |= 0x4; // set PF
	}
	else
	{
		EFLAGS(context) &= ~0x4; // clear PF
	}

	if (((x & y) | ((x ^ y) & ~summ)) & 0x8)
	{
		EFLAGS(context) |= 0x10; // set AF
	}
	else
	{
		EFLAGS(context) &= ~0x10; // clear AF
	}

	return true;
}

size_t get_x64_access_size(x64_context* context, x64_op_t op, x64_reg_t reg, size_t d_size, size_t i_size)
{
	if (op == X64OP_MOVS || op == X64OP_STOS)
	{
		if (EFLAGS(context) & 0x400 /* direction flag */)
		{
			// TODO
			return 0;
		}

		if (reg != X64_NOT_SET) // get "full" access size from RCX register
		{
			u64 counter;
			if (!get_x64_reg_value(context, reg, 8, i_size, counter))
			{
				return -1;
			}

			return d_size * counter;
		}
	}

	return d_size;
}

namespace rsx
{
	extern std::function<bool(u32 addr, bool is_writing)> g_access_violation_handler;
}

bool handle_access_violation(u32 addr, bool is_writing, x64_context* context)
{
	g_tls_fault_all++;

	const auto cpu = get_current_cpu_thread();

	if (rsx::g_access_violation_handler)
	{
		bool handled = false;

		try
		{
			if (cpu)
			{
				vm::temporary_unlock(*cpu);
			}

			handled = rsx::g_access_violation_handler(addr, is_writing);
		}
		catch (const std::exception& e)
		{
			rsx_log.fatal("g_access_violation_handler(0x%x, %d): %s", addr, is_writing, e.what());

			if (cpu)
			{
				cpu->state += cpu_flag::dbg_pause;

				if (cpu->test_stopped())
				{
					std::terminate();
				}
			}

			return false;
		}

		if (handled)
		{
			g_tls_fault_rsx++;
			if (cpu && cpu->test_stopped())
			{
				//
			}

			return true;
		}

		if (cpu && cpu->test_stopped())
		{
		}
	}

	const u8* const code = reinterpret_cast<u8*>(RIP(context));

	x64_op_t op;
	x64_reg_t reg;
	size_t d_size;
	size_t i_size;

	// decode single x64 instruction that causes memory access
	decode_x64_reg_op(code, op, reg, d_size, i_size);

	auto report_opcode = [=]()
	{
		if (op == X64OP_NONE)
		{
			sig_log.error("decode_x64_reg_op(%p): unsupported opcode: %s", code, *reinterpret_cast<const be_t<v128, 1>*>(code));
		}
	};

	if ((d_size | (d_size + addr)) >= 0x100000000ull)
	{
		sig_log.error("Invalid d_size (0x%llx)", d_size);
		report_opcode();
		return false;
	}

	// get length of data being accessed
	size_t a_size = get_x64_access_size(context, op, reg, d_size, i_size);

	if ((a_size | (a_size + addr)) >= 0x100000000ull)
	{
		sig_log.error("Invalid a_size (0x%llx)", a_size);
		report_opcode();
		return false;
	}

	// check if address is RawSPU MMIO register
	if (addr - RAW_SPU_BASE_ADDR < (6 * RAW_SPU_OFFSET) && (addr % RAW_SPU_OFFSET) >= RAW_SPU_PROB_OFFSET)
	{
		auto thread = idm::get<named_thread<spu_thread>>(spu_thread::find_raw_spu((addr - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET));

		if (!thread)
		{
			return false;
		}

		if (a_size != 4 || !d_size || !i_size)
		{
			sig_log.error("Invalid or unsupported instruction (op=%d, reg=%d, d_size=%lld, a_size=0x%llx, i_size=%lld)", +op, +reg, d_size, a_size, i_size);
			report_opcode();
			return false;
		}

		switch (op)
		{
		case X64OP_LOAD:
		case X64OP_LOAD_BE:
		case X64OP_LOAD_CMP:
		case X64OP_LOAD_TEST:
		{
			u32 value;
			if (is_writing || !thread->read_reg(addr, value))
			{
				return false;
			}

			if (op != X64OP_LOAD_BE)
			{
				value = se_storage<u32>::swap(value);
			}

			if (op == X64OP_LOAD_CMP)
			{
				u64 rvalue;
				if (!get_x64_reg_value(context, reg, d_size, i_size, rvalue) || !set_x64_cmp_flags(context, d_size, value, rvalue))
				{
					return false;
				}

				break;
			}

			if (op == X64OP_LOAD_TEST)
			{
				u64 rvalue;
				if (!get_x64_reg_value(context, reg, d_size, i_size, rvalue) || !set_x64_cmp_flags(context, d_size, value & rvalue, 0))
				{
					return false;
				}

				break;
			}

			if (!put_x64_reg_value(context, reg, d_size, value))
			{
				return false;
			}

			break;
		}
		case X64OP_STORE:
		case X64OP_STORE_BE:
		{
			u64 reg_value;
			if (!is_writing || !get_x64_reg_value(context, reg, d_size, i_size, reg_value))
			{
				return false;
			}

			u32 val32 = static_cast<u32>(reg_value);
			if (!thread->write_reg(addr, op == X64OP_STORE ? se_storage<u32>::swap(val32) : val32))
			{
				return false;
			}

			break;
		}
		case X64OP_MOVS: // possibly, TODO
		case X64OP_STOS:
		default:
		{
			sig_log.error("Invalid or unsupported operation (op=%d, reg=%d, d_size=%lld, i_size=%lld)", +op, +reg, d_size, i_size);
			report_opcode();
			return false;
		}
		}

		// skip processed instruction
		RIP(context) += i_size;
		g_tls_fault_spu++;
		return true;
	}

	if (vm::check_addr(addr, std::max(1u, ::narrow<u32>(d_size)), is_writing ? vm::page_writable : vm::page_readable))
	{
		if (cpu && cpu->test_stopped())
		{
			//
		}

		return true;
	}

	if (cpu)
	{
		u32 pf_port_id = 0;

		if (auto pf_entries = g_fxo->get<page_fault_notification_entries>(); true)
		{
			if (auto mem = vm::get(vm::any, addr))
			{
				std::shared_lock lock(pf_entries->mutex);

				for (const auto& entry : pf_entries->entries)
				{
					if (entry.start_addr == mem->addr)
					{
						pf_port_id = entry.port_id;
						break;
					}
				}
			}
		}

		if (pf_port_id)
		{
			// We notify the game that a page fault occurred so it can rectify it.
			// Note, for data3, were the memory readable AND we got a page fault, it must be due to a write violation since reads are allowed.
			u64 data1 = addr;
			u64 data2;

			if (cpu->id_type() == 1)
			{
				data2 = (SYS_MEMORY_PAGE_FAULT_TYPE_PPU_THREAD << 32) | cpu->id;
			}
			else
			{
				const auto& spu = static_cast<spu_thread&>(*cpu);

				const u64 type = spu.offset < RAW_SPU_BASE_ADDR ?
					SYS_MEMORY_PAGE_FAULT_TYPE_SPU_THREAD :
					SYS_MEMORY_PAGE_FAULT_TYPE_RAW_SPU;

				data2 = (type << 32) | spu.lv2_id;
			}

			u64 data3;
			{
				vm::reader_lock rlock;
				if (vm::check_addr(addr, std::max(1u, ::narrow<u32>(d_size)), is_writing ? vm::page_writable : vm::page_readable))
				{
					// Memory was allocated inbetween, retry
					return true;
				}
				else if (vm::check_addr(addr, std::max(1u, ::narrow<u32>(d_size))))
				{
					data3 = SYS_MEMORY_PAGE_FAULT_CAUSE_READ_ONLY; // TODO
				}
				else
				{
					data3 = SYS_MEMORY_PAGE_FAULT_CAUSE_NON_MAPPED;
				}
			}

			// Now, place the page fault event onto table so that other functions [sys_mmapper_free_address and pagefault recovery funcs etc]
			// know that this thread is page faulted and where.

			auto pf_events = g_fxo->get<page_fault_event_entries>();
			{
				std::lock_guard pf_lock(pf_events->pf_mutex);
				pf_events->events.emplace(static_cast<u32>(data2), addr);
			}

			sig_log.error("Page_fault %s location 0x%x because of %s memory", is_writing ? "writing" : "reading",
				addr, data3 == SYS_MEMORY_PAGE_FAULT_CAUSE_READ_ONLY ? "writing read-only" : "using unmapped");

			error_code sending_error = sys_event_port_send(pf_port_id, data1, data2, data3);

			// If we fail due to being busy, wait a bit and try again.
			while (sending_error == CELL_EBUSY)
			{
				if (cpu->id_type() == 1)
				{
					lv2_obj::sleep(*cpu, 1000);
				}

				thread_ctrl::wait_for(1000);
				sending_error = sys_event_port_send(pf_port_id, data1, data2, data3);
			}

			if (sending_error)
			{
				fmt::throw_exception("Unknown error %x while trying to pass page fault.", sending_error.value);
			}

			if (cpu->id_type() == 1)
			{
				// Deschedule
				lv2_obj::sleep(*cpu);
			}

			// Wait until the thread is recovered
			for (std::shared_lock pf_lock(pf_events->pf_mutex);
				pf_events->events.find(static_cast<u32>(data2)) != pf_events->events.end();)
			{
				if (Emu.IsStopped())
				{
					break;
				}

				// Timeout in case the emulator is stopping
				pf_events->cond.wait(pf_lock, 10000);
			}

			// Reschedule
			if (cpu->test_stopped())
			{
				//
			}

			if (Emu.IsStopped())
			{
				// Hack: allocate memory in case the emulator is stopping
				vm::falloc(addr & -0x10000, 0x10000);
			}

			return true;
		}

		if (cpu->id_type() != 1)
		{
			vm_log.notice("\n%s", cpu->dump());
			vm_log.fatal("Access violation %s location 0x%x", is_writing ? "writing" : "reading", addr);

			// TODO:
			// RawSPU: Send appropriate interrupt
			// SPUThread: Send sys_spu exception event
			cpu->state += cpu_flag::dbg_pause;
			if (cpu->check_state())
			{
				// Hack: allocate memory in case the emulator is stopping
				auto area = vm::reserve_map(vm::any, addr & -0x10000, 0x10000);

				if (area->flags & 0x100)
				{
					// For 4kb pages
					utils::memory_protect(vm::base(addr & -0x1000), 0x1000, utils::protection::rw);
				}
				else
				{
					area->falloc(addr & -0x10000, 0x10000);
				}

				return true;
			}
		}
		else
		{
			if (auto last_func = static_cast<ppu_thread*>(cpu)->current_function)
			{
				ppu_log.fatal("Function aborted: %s", last_func);
			}

			lv2_obj::sleep(*cpu);
		}
	}

	Emu.Pause();

	if (cpu)
	{
		vm_log.notice("\n%s", cpu->dump());
	}

	vm_log.fatal("Access violation %s location 0x%x", is_writing ? "writing" : "reading", addr);

	while (Emu.IsPaused())
	{
		thread_ctrl::wait();
	}

	if (Emu.IsStopped())
	{
		std::terminate();
	}

	return true;
}

#ifdef _WIN32

static LONG exception_handler(PEXCEPTION_POINTERS pExp)
{
	const u64 addr64 = pExp->ExceptionRecord->ExceptionInformation[1] - reinterpret_cast<u64>(vm::g_base_addr);
	const u64 exec64 = (pExp->ExceptionRecord->ExceptionInformation[1] - reinterpret_cast<u64>(vm::g_exec_addr)) / 2;
	const bool is_writing = pExp->ExceptionRecord->ExceptionInformation[0] != 0;

	if (pExp->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && addr64 < 0x100000000ull)
	{
		if (thread_ctrl::get_current() && handle_access_violation(static_cast<u32>(addr64), is_writing, pExp->ContextRecord))
		{
			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}

	if (pExp->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && exec64 < 0x100000000ull)
	{
		if (thread_ctrl::get_current() && handle_access_violation(static_cast<u32>(exec64), is_writing, pExp->ContextRecord))
		{
			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

static LONG exception_filter(PEXCEPTION_POINTERS pExp)
{
	std::string msg = fmt::format("Unhandled Win32 exception 0x%08X.\n", pExp->ExceptionRecord->ExceptionCode);

	if (pExp->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		const auto cause = pExp->ExceptionRecord->ExceptionInformation[0] != 0 ? "writing" : "reading";

		msg += fmt::format("Segfault %s location %p at %p.\n", cause, pExp->ExceptionRecord->ExceptionInformation[1], pExp->ExceptionRecord->ExceptionAddress);
	}
	else
	{
		msg += fmt::format("Exception address: %p.\n", pExp->ExceptionRecord->ExceptionAddress);

		for (DWORD i = 0; i < pExp->ExceptionRecord->NumberParameters; i++)
		{
			msg += fmt::format("ExceptionInformation[0x%x]: %p.\n", i, pExp->ExceptionRecord->ExceptionInformation[i]);
		}
	}

	std::vector<HMODULE> modules;
	for (DWORD size = 256; modules.size() != size; size /= sizeof(HMODULE))
	{
		modules.resize(size);
		if (!EnumProcessModules(GetCurrentProcess(), modules.data(), size * sizeof(HMODULE), &size))
		{
			modules.clear();
			break;
		}
	}

	msg += fmt::format("Instruction address: %p.\n", pExp->ContextRecord->Rip);

	DWORD64 unwind_base;
	if (const auto rtf = RtlLookupFunctionEntry(pExp->ContextRecord->Rip, &unwind_base, nullptr))
	{
		// Get function address
		const DWORD64 func_addr = rtf->BeginAddress + unwind_base;
		msg += fmt::format("Function address: %p (base+0x%x).\n", func_addr, rtf->BeginAddress);

		// Access UNWIND_INFO structure
		//const auto uw = (u8*)(unwind_base + rtf->UnwindData);
	}

	for (HMODULE module : modules)
	{
		MODULEINFO info;
		if (GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(info)))
		{
			const DWORD64 base = reinterpret_cast<DWORD64>(info.lpBaseOfDll);

			if (pExp->ContextRecord->Rip >= base && pExp->ContextRecord->Rip < base + info.SizeOfImage)
			{
				std::string module_name;
				for (DWORD size = 15; module_name.size() != size;)
				{
					module_name.resize(size);
					size = GetModuleBaseNameA(GetCurrentProcess(), module, &module_name.front(), size + 1);
					if (!size)
					{
						module_name.clear();
						break;
					}
				}

				msg += fmt::format("Module name: '%s'.\n", module_name);
				msg += fmt::format("Module base: %p.\n", info.lpBaseOfDll);
			}
		}
	}

	msg += fmt::format("RPCS3 image base: %p.\n", GetModuleHandle(NULL));

	// TODO: print registers and the callstack

	// Report fatal error
	report_fatal_error(msg);
	return EXCEPTION_CONTINUE_SEARCH;
}

const bool s_exception_handler_set = []() -> bool
{
	if (!AddVectoredExceptionHandler(1, (PVECTORED_EXCEPTION_HANDLER)exception_handler))
	{
		report_fatal_error("AddVectoredExceptionHandler() failed.");
	}

	if (!SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)exception_filter))
	{
		report_fatal_error("SetUnhandledExceptionFilter() failed.");
	}

	return true;
}();

#else

static void signal_handler(int sig, siginfo_t* info, void* uct)
{
	x64_context* context = static_cast<ucontext_t*>(uct);

#ifdef __APPLE__
	const bool is_writing = context->uc_mcontext->__es.__err & 0x2;
#elif defined(__DragonFly__) || defined(__FreeBSD__)
	const bool is_writing = context->uc_mcontext.mc_err & 0x2;
#elif defined(__OpenBSD__)
	const bool is_writing = context->sc_err & 0x2;
#elif defined(__NetBSD__)
	const bool is_writing = context->uc_mcontext.__gregs[_REG_ERR] & 0x2;
#else
	const bool is_writing = context->uc_mcontext.gregs[REG_ERR] & 0x2;
#endif

	const u64 addr64 = reinterpret_cast<u64>(info->si_addr) - reinterpret_cast<u64>(vm::g_base_addr);
	const u64 exec64 = (reinterpret_cast<u64>(info->si_addr) - reinterpret_cast<u64>(vm::g_exec_addr)) / 2;
	const auto cause = is_writing ? "writing" : "reading";

	if (addr64 < 0x100000000ull)
	{
		// Try to process access violation
		if (thread_ctrl::get_current() && handle_access_violation(static_cast<u32>(addr64), is_writing, context))
		{
			return;
		}
	}

	if (exec64 < 0x100000000ull)
	{
		if (thread_ctrl::get_current() && handle_access_violation(static_cast<u32>(exec64), is_writing, context))
		{
			return;
		}
	}

	// TODO (debugger interaction)
	report_fatal_error(fmt::format("Segfault %s location %p at %p.", cause, info->si_addr, RIP(context)));
}

const bool s_exception_handler_set = []() -> bool
{
	struct ::sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = signal_handler;

	if (::sigaction(SIGSEGV, &sa, NULL) == -1)
	{
		std::printf("sigaction(SIGSEGV) failed (0x%x).", errno);
		std::abort();
	}

	return true;
}();

#endif

thread_local DECLARE(thread_ctrl::g_tls_this_thread) = nullptr;

DECLARE(thread_ctrl::g_native_core_layout) { native_core_arrangement::undefined };

void thread_base::start(native_entry entry)
{
#ifdef _WIN32
	m_thread = ::_beginthreadex(nullptr, 0, entry, this, CREATE_SUSPENDED, nullptr);
	verify("thread_ctrl::start" HERE), m_thread, ::ResumeThread(reinterpret_cast<HANDLE>(+m_thread)) != -1;
#else
	verify("thread_ctrl::start" HERE), pthread_create(reinterpret_cast<pthread_t*>(&m_thread.raw()), nullptr, entry, this) == 0;
#endif
}

void thread_base::initialize(bool(*wait_cb)(const void*))
{
	// Initialize TLS variable
	thread_ctrl::g_tls_this_thread = this;

	// Initialize atomic wait callback
	atomic_storage_futex::set_wait_callback(wait_cb);

	g_tls_log_prefix = []
	{
		return thread_ctrl::g_tls_this_thread->m_name.get();
	};

#ifdef _MSC_VER
	struct THREADNAME_INFO
	{
		DWORD dwType;
		LPCSTR szName;
		DWORD dwThreadID;
		DWORD dwFlags;
	};

	// Set thread name for VS debugger
	if (IsDebuggerPresent())
	{
		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = m_name.get().c_str();
		info.dwThreadID = -1;
		info.dwFlags = 0;

		__try
		{
			RaiseException(0x406D1388, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
#endif

#if defined(__APPLE__)
	pthread_setname_np(m_name.get().substr(0, 15).c_str());
#elif defined(__DragonFly__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	pthread_set_name_np(pthread_self(), m_name.get().c_str());
#elif defined(__NetBSD__)
	pthread_setname_np(pthread_self(), "%s", const_cast<char*>(m_name.get().c_str()));
#elif !defined(_WIN32)
	pthread_setname_np(pthread_self(), m_name.get().substr(0, 15).c_str());
#endif

#ifdef __linux__
	m_timer = timerfd_create(CLOCK_MONOTONIC, 0);
	if (m_timer == -1)
	{
		sig_log.error("Linux timer allocation failed, use wait_unlock() only");
	}
#endif
}

void thread_base::notify_abort() noexcept
{
	// For now
	notify();

	atomic_storage_futex::raw_notify(+m_state_notifier);
}

bool thread_base::finalize(int) noexcept
{
	// Report pending errors
	error_code::error_report(0, 0, 0, 0);

#ifdef __linux__
	if (m_timer != -1)
	{
		close(m_timer);
	}
#endif

#ifdef _WIN32
	ULONG64 cycles{};
	QueryThreadCycleTime(GetCurrentThread(), &cycles);
	FILETIME ctime, etime, ktime, utime;
	GetThreadTimes(GetCurrentThread(), &ctime, &etime, &ktime, &utime);
	const u64 time = ((ktime.dwLowDateTime | (u64)ktime.dwHighDateTime << 32) + (utime.dwLowDateTime | (u64)utime.dwHighDateTime << 32)) * 100ull;
#elif defined(RUSAGE_THREAD)
	const u64 cycles = 0; // Not supported
	struct ::rusage stats{};
	::getrusage(RUSAGE_THREAD, &stats);
	const u64 time = (stats.ru_utime.tv_sec + stats.ru_stime.tv_sec) * 1000000000ull + (stats.ru_utime.tv_usec + stats.ru_stime.tv_usec) * 1000ull;
#else
	const u64 cycles = 0;
	const u64 time = 0;
#endif

	g_tls_log_prefix = []
	{
		return thread_ctrl::g_tls_this_thread->m_name.get();
	};

	sig_log.notice("Thread time: %fs (%fGc); Faults: %u [rsx:%u, spu:%u];",
		time / 1000000000.,
		cycles / 1000000000.,
		g_tls_fault_all,
		g_tls_fault_rsx,
		g_tls_fault_spu);

	// Return true if need to delete thread object
	const bool result = m_state.exchange(thread_state::finished) == thread_state::detached;

	// Signal waiting threads
	m_state.notify_all();
	return result;
}

void thread_base::finalize() noexcept
{
	g_tls_log_prefix = []() -> std::string { return {}; };
	thread_ctrl::g_tls_this_thread = nullptr;
}

void thread_ctrl::_wait_for(u64 usec, bool alert /* true */)
{
	auto _this = g_tls_this_thread;

#ifdef __linux__
	if (!alert && _this->m_timer != -1 && usec > 0 && usec <= 1000)
	{
		struct itimerspec timeout;
		u64 missed;
		u64 nsec = usec * 1000ull;

		timeout.it_value.tv_nsec = (nsec % 1000000000ull);
		timeout.it_value.tv_sec = nsec / 1000000000ull;
		timeout.it_interval.tv_sec = 0;
		timeout.it_interval.tv_nsec = 0;
		timerfd_settime(_this->m_timer, 0, &timeout, NULL);
		if (read(_this->m_timer, &missed, sizeof(missed)) != sizeof(missed))
			sig_log.error("timerfd: read() failed");
		return;
	}
#endif

	std::unique_lock lock(_this->m_mutex, std::defer_lock);

	while (true)
	{
		// Mutex is unlocked at the start and after the waiting
		if (u32 sig = _this->m_signal.load())
		{
			if (sig & 1)
			{
				_this->m_signal &= ~1;
				return;
			}
		}

		if (usec == 0)
		{
			// No timeout: return immediately
			return;
		}

		if (!lock)
		{
			lock.lock();
		}

		// Double-check the value
		if (u32 sig = _this->m_signal.load())
		{
			if (sig & 1)
			{
				_this->m_signal &= ~1;
				return;
			}
		}

		_this->m_cond.wait_unlock(usec, lock);

		if (usec < cond_variable::max_timeout)
		{
			usec = 0;
		}
	}
}

thread_base::thread_base(std::string_view name)
	: m_name(name)
{
}

thread_base::~thread_base()
{
	if (m_thread)
	{
#ifdef _WIN32
		CloseHandle(reinterpret_cast<HANDLE>(m_thread.raw()));
#else
		pthread_detach(reinterpret_cast<pthread_t>(m_thread.raw()));
#endif
	}
}

void thread_base::join() const
{
	for (auto state = m_state.load(); state != thread_state::finished;)
	{
		m_state.wait(state);
		state = m_state;
	}
}

void thread_base::notify()
{
	if (!(m_signal & 1))
	{
		m_signal |= 1;
		m_mutex.lock_unlock();
		m_cond.notify_one();
	}
}

u64 thread_base::get_cycles()
{
	u64 cycles;

#ifdef _WIN32
	if (QueryThreadCycleTime(reinterpret_cast<HANDLE>(m_thread.load()), &cycles))
	{
#elif __APPLE__
	mach_port_name_t port = pthread_mach_thread_np(reinterpret_cast<pthread_t>(m_thread.load()));
	mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;
	thread_basic_info_data_t info;
	kern_return_t ret = thread_info(port, THREAD_BASIC_INFO, reinterpret_cast<thread_info_t>(&info), &count);
	if (ret == KERN_SUCCESS)
	{
		cycles = static_cast<u64>(info.user_time.seconds + info.system_time.seconds) * 1'000'000'000 +
			static_cast<u64>(info.user_time.microseconds + info.system_time.microseconds) * 1'000;
#else
	clockid_t _clock;
	struct timespec thread_time;
	if (!pthread_getcpuclockid(reinterpret_cast<pthread_t>(m_thread.load()), &_clock) && !clock_gettime(_clock, &thread_time))
	{
		cycles = static_cast<u64>(thread_time.tv_sec) * 1'000'000'000 + thread_time.tv_nsec;
#endif
		if (const u64 old_cycles = m_cycles.exchange(cycles))
		{
			return cycles - old_cycles;
		}

		// Report 0 the first time this function is called
		return 0;
	}
	else
	{
		return m_cycles;
	}
}

void thread_ctrl::detect_cpu_layout()
{
	if (!g_native_core_layout.compare_and_swap_test(native_core_arrangement::undefined, native_core_arrangement::generic))
		return;

	const auto system_id = utils::get_system_info();
	if (system_id.find("Ryzen") != std::string::npos)
	{
		g_native_core_layout.store(native_core_arrangement::amd_ccx);
	}
	else if (system_id.find("Intel") != std::string::npos)
	{
#ifdef _WIN32
		const LOGICAL_PROCESSOR_RELATIONSHIP relationship = LOGICAL_PROCESSOR_RELATIONSHIP::RelationProcessorCore;
		DWORD buffer_size = 0;

		// If buffer size is set to 0 bytes, it will be overwritten with the required size
		if (GetLogicalProcessorInformationEx(relationship, nullptr, &buffer_size))
		{
			sig_log.error("GetLogicalProcessorInformationEx returned 0 bytes");
			return;
		}
		DWORD error_code = GetLastError();
		if (error_code != ERROR_INSUFFICIENT_BUFFER)
		{
			sig_log.error("Unexpected windows error code when detecting CPU layout: %u", error_code);
			return;
		}

		std::vector<u8> buffer(buffer_size);

		if (!GetLogicalProcessorInformationEx(relationship,
			reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *>(buffer.data()), &buffer_size))
		{
			sig_log.error("GetLogicalProcessorInformationEx failed (size=%u, error=%u)", buffer_size, GetLastError());
		}
		else
		{
			// Iterate through the buffer until a core with hyperthreading is found
			auto ptr = reinterpret_cast<std::uintptr_t>(buffer.data());
			const std::uintptr_t end = ptr + buffer_size;

			while (ptr < end)
			{
				auto info = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *>(ptr);
				if (info->Relationship == relationship && info->Processor.Flags == LTP_PC_SMT)
				{
					g_native_core_layout.store(native_core_arrangement::intel_ht);
					break;
				}
				ptr += info->Size;
			}
		}
#else
		sig_log.todo("Thread scheduler is not implemented for Intel and this OS");
#endif
	}
}

u64 thread_ctrl::get_affinity_mask(thread_class group)
{
	detect_cpu_layout();

	if (const auto thread_count = std::thread::hardware_concurrency())
	{
		const u64 all_cores_mask = thread_count < 64 ? UINT64_MAX >> (64 - thread_count): UINT64_MAX;

		switch (g_native_core_layout)
		{
		default:
		case native_core_arrangement::generic:
		{
			return all_cores_mask;
		}
		case native_core_arrangement::amd_ccx:
		{
			u64 spu_mask, ppu_mask, rsx_mask;
			const auto system_id = utils::get_system_info();
			if (thread_count >= 32)
			{
				if (system_id.find("3950X") != std::string::npos)
				{
					// zen2
					// Ryzen 9 3950X
					// Assign threads 9-32
					ppu_mask = 0b11111111000000000000000000000000;
					spu_mask = 0b00000000111111110000000000000000;
					rsx_mask = 0b00000000000000001111111100000000;
				}
				else if (system_id.find("2970WX") != std::string::npos)
				{
					// zen+
					// Threadripper 2970WX
					// Assign threads 9-24
					ppu_mask = 0b000000111111000000000000;
					spu_mask = ppu_mask;
					rsx_mask = 0b111111000000000000000000;
				}
				else
				{
					// zen(+)
					// Threadripper 1950X/2950X/2990WX
					// Assign threads 17-32
					ppu_mask = 0b00000000111111110000000000000000;
					spu_mask = ppu_mask;
					rsx_mask = 0b11111111000000000000000000000000;
				}

			}
			else if (thread_count == 24)
			{
				if (system_id.find("3900X") != std::string::npos)
				{
					// zen2
					// Ryzen 9 3900X
					// Assign threads 7-22
					ppu_mask = 0b111111000000000000000000;
					spu_mask = 0b000000111111000000000000;
					rsx_mask = 0b000000000000111111000000;
				}
				else
				{
					// zen(+)
					// Threadripper 1920X/2920X
					// Assign threads 13-24
					ppu_mask = 0b000000111111000000000000;
					spu_mask = ppu_mask;
					rsx_mask = 0b111111000000000000000000;
				}
			}
			else if (thread_count == 16)
			{
				if (system_id.find("3700X") != std::string::npos || system_id.find("3800X") != std::string::npos)
				{
					// Ryzen 7 3700/3800 (x)
					// Assign threads 1-16
					ppu_mask = 0b0000000011110000;
					spu_mask = 0b1111111100000000;
					rsx_mask = 0b0000000000001111;
				}
				else
				{
					// zen(+)
					// Ryzen 7, Threadripper
					// Assign threads 3-16
					ppu_mask = 0b1111111100000000;
					spu_mask = ppu_mask;
					rsx_mask = 0b0000000000111100;
				}
			}
			else if (thread_count == 12)
			{
				if (system_id.find("3600") != std::string::npos)
				{
					// zen2
					// R5 3600 (x)
					// Assign threads 1-12
					ppu_mask = 0b000000111000;
					spu_mask = 0b111111000000;
					rsx_mask = 0b000000000111;
				}
				else
				{
					// zen(+)
					// R5 1600/2600 (x)
					// Assign threads 3-12
					ppu_mask = 0b111111000000;
					spu_mask = ppu_mask;
					rsx_mask = 0b000000111100;
				}
			}
			else
			{
				// R5 & R3 don't seem to improve performance no matter how these are shuffled
				ppu_mask = spu_mask = rsx_mask = 0b11111111 & all_cores_mask;
			}

			switch (group)
			{
			default:
			case thread_class::general:
				return all_cores_mask;
			case thread_class::rsx:
				return rsx_mask;
			case thread_class::ppu:
				return ppu_mask;
			case thread_class::spu:
				return spu_mask;
			}
		}
		case native_core_arrangement::intel_ht:
		{
			/* This has been disabled as it seems to degrade performance instead of improving it.
			if (thread_count <= 4)
			{
				//i3 or worse
				switch (group)
				{
				case thread_class::rsx:
				case thread_class::ppu:
					return (0b0101 & all_cores_mask);
				case thread_class::spu:
					return (0b1010 & all_cores_mask);
				case thread_class::general:
					return all_cores_mask;
				}
			}
			*/

			return all_cores_mask;
		}
		}
	}

	return UINT64_MAX;
}

void thread_ctrl::set_native_priority(int priority)
{
#ifdef _WIN32
	HANDLE _this_thread = GetCurrentThread();
	INT native_priority = THREAD_PRIORITY_NORMAL;

	if (priority > 0)
		native_priority = THREAD_PRIORITY_ABOVE_NORMAL;
	if (priority < 0)
		native_priority = THREAD_PRIORITY_BELOW_NORMAL;

	if (!SetThreadPriority(_this_thread, native_priority))
	{
		sig_log.error("SetThreadPriority() failed: 0x%x", GetLastError());
	}
#else
	int policy;
	struct sched_param param;

	pthread_getschedparam(pthread_self(), &policy, &param);

	if (priority > 0)
		param.sched_priority = sched_get_priority_max(policy);
	if (priority < 0)
		param.sched_priority = sched_get_priority_min(policy);

	if (int err = pthread_setschedparam(pthread_self(), policy, &param))
	{
		sig_log.error("pthraed_setschedparam() failed: %d", err);
	}
#endif
}

void thread_ctrl::set_thread_affinity_mask(u64 mask)
{
#ifdef _WIN32
	HANDLE _this_thread = GetCurrentThread();
	SetThreadAffinityMask(_this_thread, mask);
#elif __APPLE__
	// Supports only one core
	thread_affinity_policy_data_t policy = { static_cast<integer_t>(utils::cnttz64(mask)) };
	thread_port_t mach_thread = pthread_mach_thread_np(pthread_self());
	thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, reinterpret_cast<thread_policy_t>(&policy), 1);
#elif defined(__linux__) || defined(__DragonFly__) || defined(__FreeBSD__)
	cpu_set_t cs;
	CPU_ZERO(&cs);

	for (u32 core = 0; core < 64u; ++core)
	{
		const u64 shifted = mask >> core;

		if (shifted & 1)
		{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
			CPU_SET(core, &cs);
#pragma GCC diagnostic pop
		}

		if (shifted <= 1)
		{
			break;
		}
	}

	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cs);
#endif
}
