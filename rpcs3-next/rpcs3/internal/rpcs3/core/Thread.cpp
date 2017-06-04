#include "thread.h"
#include "state.h"
#include "log.h"
#include "events.h"

#include <rpcs3/vm/vm.h>
#include <rpcs3/cell/spu/raw_thread.h>

#ifdef _WIN32
#include <windows.h>
#else
#ifdef __APPLE__
#define _XOPEN_SOURCE
#define __USE_GNU
#endif
#include <signal.h>
#include <ucontext.h>
#endif

namespace rpcs3
{
	inline namespace core
	{
		void set_current_thread_debug_name(const char* threadName)
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

			X64R_ECX = X64R_CL,
		};

		enum x64_op_t : u32
		{
			X64OP_NONE,
			X64OP_LOAD, // obtain and put the value into x64 register
			X64OP_STORE, // take the value from x64 register or an immediate and use it
			X64OP_MOVS,
			X64OP_STOS,
			X64OP_XCHG,
			X64OP_CMPXCHG,
			X64OP_LOAD_AND_STORE, // lock and [mem],reg
			X64OP_LOAD_OR_STORE, // TODO: lock or [mem], reg
			X64OP_INC, // TODO: lock inc [mem]
			X64OP_DEC, // TODO: lock dec [mem]
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
						LOG_ERROR(memory, "decode_x64_reg_op(%016llxh): LOCK prefix found twice", (size_t)code - out_length);
					}
			
					lock = true;
					continue;
				}
				case REPNE: // group 1
				{
					if (repne)
					{
						LOG_ERROR(memory, "decode_x64_reg_op(%016llxh): REPNE/REPNZ prefix found twice", (size_t)code - out_length);
					}
			
					repne = true;
					continue;
				}
				case REPE: // group 1
				{
					if (repe)
					{
						LOG_ERROR(memory, "decode_x64_reg_op(%016llxh): REP/REPE/REPZ prefix found twice", (size_t)code - out_length);
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
						LOG_ERROR(memory, "decode_x64_reg_op(%016llxh): 0x%02x (group 2 prefix) found after 0x%02x", (size_t)code - out_length, prefix, pg2);
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
						LOG_ERROR(memory, "decode_x64_reg_op(%016llxh): operand-size override prefix found twice", (size_t)code - out_length);
					}
			
					oso = true;
					continue;
				}

				case 0x67: // group 4
				{
					LOG_ERROR(memory, "decode_x64_reg_op(%016llxh): address-size override prefix found", (size_t)code - out_length, prefix);
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
							LOG_ERROR(memory, "decode_x64_reg_op(%016llxh): 0x%02x (REX prefix) found after 0x%02x", (size_t)code - out_length, prefix, rex);
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
				return (x64_reg_t)(((*code & 0x38) >> 3 | (/* check REX.R bit */ rex & 4 ? 8 : 0)) + X64R_RAX);
			};

			auto get_modRM_reg_xmm = [](const u8* code, const u8 rex) -> x64_reg_t
			{
				return (x64_reg_t)(((*code & 0x38) >> 3 | (/* check REX.R bit */ rex & 4 ? 8 : 0)) + X64R_XMM0);
			};

			auto get_modRM_reg_lh = [](const u8* code) -> x64_reg_t
			{
				return (x64_reg_t)(((*code & 0x38) >> 3) + X64R_AL);
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

			const u8 op1 = *code++;
			const u8 op2 = code[0];

			out_length++;

			switch (op1)
			{
			case 0x0f:
			{
				out_length++, code++;

				switch (op2)
				{
				case 0x11:
				{
					if (!repe && !repne && !oso) // MOVUPS xmm/m, xmm
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
				}

				break;
			}
			case 0x20:
			{
				if (!oso)
				{
					out_op = X64OP_LOAD_AND_STORE;
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
					out_op = X64OP_LOAD_AND_STORE;
					out_reg = get_modRM_reg(code, rex);
					out_size = get_op_size(rex, oso);
					out_length += get_modRM_size(code);
					return;
				}
				break;
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
			case 0xc6:
			{
				if (!lock && !oso && get_modRM_reg(code, 0) == X64R_RAX) // MOV r8/m8, imm8
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
				if (!lock && get_modRM_reg(code, 0) == X64R_RAX) // MOV r/m, imm16/imm32 (16, 32, 64)
				{
					out_op = X64OP_STORE;
					out_reg = oso ? X64_IMM16 : X64_IMM32;
					out_size = get_op_size(rex, oso);
					out_length += get_modRM_size(code) + (oso ? 2 : 4);
					return;
				}
				break;
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
			case 0: // RAX
				return &state->__rax;
			case 1: // RCX
				return &state->__rcx;
			case 2: // RDX
				return &state->__rdx;
			case 3: // RBX
				return &state->__rbx;
			case 4: // RSP
				return &state->__rsp;
			case 5: // RBP
				return &state->__rbp;
			case 6: // RSI
				return &state->__rsi;
			case 7: // RDI
				return &state->__rdi;
			case 8: // R8
				return &state->__r8;
			case 9: // R9
				return &state->__r9;
			case 10: // R10
				return &state->__r10;
			case 11: // R11
				return &state->__r11;
			case 12: // R12
				return &state->__r12;
			case 13: // R13
				return &state->__r13;
			case 14: // R14
				return &state->__r14;
			case 15: // R15
				return &state->__r15;
			case 16: // RIP
				return &state->__rip;
			default: // FAIL
				assert(0);
			}
		}

		#else

		typedef decltype(REG_RIP) reg_table_t;

		static const reg_table_t reg_table[17] =
		{
			REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RBP, REG_RSI, REG_RDI,
			REG_R8, REG_R9, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15, REG_RIP
		};

		#define X64REG(context, reg) (&(context)->uc_mcontext.gregs[reg_table[reg]])
		#define XMMREG(context, reg) (reinterpret_cast<v128*>(&(context)->uc_mcontext.fpregs->_xmm[reg]))
		#define EFLAGS(context) ((context)->uc_mcontext.gregs[REG_EFL])

		#endif // __APPLE__

		#endif

		#define RAX(c) (*X64REG((c), 0))
		#define RCX(c) (*X64REG((c), 1))
		#define RDX(c) (*X64REG((c), 2))
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
				case 1: out_value = (u8)reg_value; return true;
				case 2: out_value = (u16)reg_value; return true;
				case 4: out_value = (u32)reg_value; return true;
				case 8: out_value = reg_value; return true;
				}
			}
			else if (reg - X64R_AL < 4 && d_size == 1)
			{
				out_value = (u8)(*X64REG(context, reg - X64R_AL));
				return true;
			}
			else if (reg - X64R_AH < 4 && d_size == 1)
			{
				out_value = (u8)(*X64REG(context, reg - X64R_AH) >> 8);
				return true;
			}
			else if (reg == X64_IMM8)
			{
				// load the immediate value (assuming it's at the end of the instruction)
				const s8 imm_value = *(s8*)(RIP(context) + i_size - 1);

				switch (d_size)
				{
				case 1: out_value = (u8)imm_value; return true;
				}
			}
			else if (reg == X64_IMM16)
			{
				const s16 imm_value = *(s16*)(RIP(context) + i_size - 2);

				switch (d_size)
				{
				case 2: out_value = (u16)imm_value; return true;
				}
			}
			else if (reg == X64_IMM32)
			{
				const s32 imm_value = *(s32*)(RIP(context) + i_size - 4);
		
				switch (d_size)
				{
				case 4: out_value = (u32)imm_value; return true;
				case 8: out_value = (u64)imm_value; return true; // sign-extended
				}
			}
			else if (reg == X64R_ECX)
			{
				out_value = (u32)RCX(context);
				return true;
			}

			LOG_ERROR(memory, "get_x64_reg_value(): invalid arguments (reg=%d, d_size=%lld, i_size=%lld)", reg, d_size, i_size);
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
				case 4: *X64REG(context, reg - X64R_RAX) = (value & 0xffffffff); return true;
				case 8: *X64REG(context, reg - X64R_RAX) = value; return true;
				}
			}

			LOG_ERROR(memory, "put_x64_reg_value(): invalid destination (reg=%d, d_size=%lld, value=0x%llx)", reg, d_size, value);
			return false;
		}

		bool set_x64_cmp_flags(x64_context* context, size_t d_size, u64 x, u64 y)
		{
			switch (d_size)
			{
			case 1: break;
			case 2: break;
			case 4: break;
			case 8: break;
			default: LOG_ERROR(memory, "set_x64_cmp_flags(): invalid d_size (%lld)", d_size); return false;
			}

			const u64 sign = 1ull << (d_size * 8 - 1); // sign mask
			const u64 diff = x - y;
			const u64 summ = x + y;

			if (((x & y) | ((x ^ y) & ~summ)) & sign)
			{
				EFLAGS(context) |= 0x1; // set CF
			}
			else
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

			const u8 p1 = (u8)diff ^ ((u8)diff >> 4);
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
					// skip reservation bound check (TODO)
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

			if (op == X64OP_CMPXCHG)
			{
				// detect whether this instruction can't actually modify memory to avoid breaking reservation;
				// this may theoretically cause endless loop, but it shouldn't be a problem if only load_sync() generates such instruction
				u64 cmp, exch;
				if (!get_x64_reg_value(context, reg, d_size, i_size, cmp) || !get_x64_reg_value(context, X64R_RAX, d_size, i_size, exch))
				{
					return -1;
				}

				if (cmp == exch)
				{
					// skip reservation bound check
					return 0;
				}
			}

			return d_size;
		}

		/**
		 * Callback that can be customised by GSRender backends to track memory access.
		 * Backends can protect memory pages and get this callback called when an access
		 * violation is met.
		 * Should return true if the backend handles the access violation.
		 */
		std::function<bool(u32 addr)> gfxHandler = [](u32) { return false; };

		bool handle_access_violation(u32 addr, bool is_writing, x64_context* context)
		{
			auto code = (const u8*)RIP(context);

			x64_op_t op;
			x64_reg_t reg;
			size_t d_size;
			size_t i_size;

			if (gfxHandler(addr))
				return true;

			// decode single x64 instruction that causes memory access
			decode_x64_reg_op(code, op, reg, d_size, i_size);

			auto report_opcode = [=]()
			{
				if (op == X64OP_NONE)
				{
					LOG_ERROR(memory, "decode_x64_reg_op(%016llxh): unsupported opcode found (%016llX%016llX)", code, *(be_t<u64>*)(code), *(be_t<u64>*)(code + 8));
				}
			};

			if ((d_size | d_size + addr) >= 0x100000000ull)
			{
				LOG_ERROR(memory, "Invalid d_size (0x%llx)", d_size);
				report_opcode();
				return false;
			}

			// get length of data being accessed
			size_t a_size = get_x64_access_size(context, op, reg, d_size, i_size);

			if ((a_size | a_size + addr) >= 0x100000000ull)
			{
				LOG_ERROR(memory, "Invalid a_size (0x%llx)", a_size);
				report_opcode();
				return false;
			}

			using namespace cell;

			// check if address is RawSPU MMIO register
			if (addr - spu::raw::constants::base_address < (6 * spu::raw::constants::offset) && (addr % spu::raw::constants::offset) >= spu::raw::constants::prob_offset)
			{
				std::shared_ptr<spu::raw::thread> thread = spu::raw::get_thread((addr - spu::raw::constants::base_address) / spu::raw::constants::offset);

				if (!thread)
				{
					return false;
				}

				if (a_size != 4 || !d_size || !i_size)
				{
					LOG_ERROR(memory, "Invalid or unsupported instruction (op=%d, reg=%d, d_size=%lld, a_size=0x%llx, i_size=%lld)", op, reg, d_size, a_size, i_size);
					report_opcode();
					return false;
				}

				switch (op)
				{
				case X64OP_LOAD:
				{
					u32 value;
					if (is_writing || !thread->read_reg(addr, value) || !put_x64_reg_value(context, reg, d_size, _byteswap_ulong(value)))
					{
						return false;
					}

					break;
				}
				case X64OP_STORE:
				{
					u64 reg_value;
					if (!is_writing || !get_x64_reg_value(context, reg, d_size, i_size, reg_value) || !thread->write_reg(addr, _byteswap_ulong((u32)reg_value)))
					{
						return false;
					}

					break;
				}
				case X64OP_MOVS: // possibly, TODO
				case X64OP_STOS:
				default:
				{
					LOG_ERROR(memory, "Invalid or unsupported operation (op=%d, reg=%d, d_size=%lld, i_size=%lld)", op, reg, d_size, i_size);
					report_opcode();
					return false;
				}
				}

				// skip processed instruction
				RIP(context) += i_size;
				return true;
			}

			// check if fault is caused by the reservation
			return vm::reservation_query(addr, (u32)a_size, is_writing, [&]() -> bool
			{
				// write memory using "privileged" access to avoid breaking reservation
				if (!d_size || !i_size)
				{
					LOG_ERROR(memory, "Invalid or unsupported instruction (op=%d, reg=%d, d_size=%lld, a_size=0x%llx, i_size=%lld)", op, reg, d_size, a_size, i_size);
					report_opcode();
					return false;
				}

				switch (op)
				{
				case X64OP_STORE:
				{
					if (d_size == 16)
					{
						if (reg - X64R_XMM0 >= 16)
						{
							LOG_ERROR(memory, "X64OP_STORE: d_size=16, reg=%d", reg);
							return false;
						}

						std::memcpy(vm::base_priv(addr), XMMREG(context, reg - X64R_XMM0), 16);
						break;
					}

					u64 reg_value;
					if (!get_x64_reg_value(context, reg, d_size, i_size, reg_value))
					{
						return false;
					}

					std::memcpy(vm::base_priv(addr), &reg_value, d_size);
					break;
				}
				case X64OP_MOVS:
				{
					if (d_size > 8)
					{
						LOG_ERROR(memory, "X64OP_MOVS: d_size=%lld", d_size);
						return false;
					}

					if (vm::base(addr) != (void*)RDI(context))
					{
						LOG_ERROR(memory, "X64OP_MOVS: rdi=0x%llx, rsi=0x%llx, addr=0x%x", (u64)RDI(context), (u64)RSI(context), addr);
						return false;
					}

					u32 a_addr = addr;

					while (a_addr >> 12 == addr >> 12)
					{
						u64 value;

						// copy data
						std::memcpy(&value, (void*)RSI(context), d_size);
						std::memcpy(vm::base_priv(a_addr), &value, d_size);

						// shift pointers
						if (EFLAGS(context) & 0x400 /* direction flag */)
						{
							LOG_ERROR(memory, "X64OP_MOVS TODO: reversed direction");
							return false;
							//RSI(context) -= d_size;
							//RDI(context) -= d_size;
							//a_addr -= (u32)d_size;
						}
						else
						{
							RSI(context) += d_size;
							RDI(context) += d_size;
							a_addr += (u32)d_size;
						}

						// decrement counter
						if (reg == X64_NOT_SET || !--RCX(context))
						{
							break;
						}
					}

					if (reg == X64_NOT_SET || !RCX(context))
					{
						break;
					}

					// don't skip partially processed instruction
					return true;
				}
				case X64OP_STOS:
				{
					if (d_size > 8)
					{
						LOG_ERROR(memory, "X64OP_STOS: d_size=%lld", d_size);
						return false;
					}

					if (vm::base(addr) != (void*)RDI(context))
					{
						LOG_ERROR(memory, "X64OP_STOS: rdi=0x%llx, addr=0x%x", (u64)RDI(context), addr);
						return false;
					}

					u64 value;
					if (!get_x64_reg_value(context, X64R_RAX, d_size, i_size, value))
					{
						return false;
					}

					u32 a_addr = addr;

					while (a_addr >> 12 == addr >> 12)
					{
						// fill data with value
						std::memcpy(vm::base_priv(a_addr), &value, d_size);

						// shift pointers
						if (EFLAGS(context) & 0x400 /* direction flag */)
						{
							LOG_ERROR(memory, "X64OP_STOS TODO: reversed direction");
							return false;
							//RDI(context) -= d_size;
							//a_addr -= (u32)d_size;
						}
						else
						{
							RDI(context) += d_size;
							a_addr += (u32)d_size;
						}

						// decrement counter
						if (reg == X64_NOT_SET || !--RCX(context))
						{
							break;
						}
					}

					if (reg == X64_NOT_SET || !RCX(context))
					{
						break;
					}

					// don't skip partially processed instruction
					return true;
				}
				case X64OP_XCHG:
				{
					u64 reg_value;
					if (!get_x64_reg_value(context, reg, d_size, i_size, reg_value))
					{
						return false;
					}

					switch (d_size)
					{
					case 1: reg_value = sync_lock_test_and_set((u8*)vm::base_priv(addr), (u8)reg_value); break;
					case 2: reg_value = sync_lock_test_and_set((u16*)vm::base_priv(addr), (u16)reg_value); break;
					case 4: reg_value = sync_lock_test_and_set((u32*)vm::base_priv(addr), (u32)reg_value); break;
					case 8: reg_value = sync_lock_test_and_set((u64*)vm::base_priv(addr), (u64)reg_value); break;
					default: return false;
					}

					if (!put_x64_reg_value(context, reg, d_size, reg_value))
					{
						return false;
					}
					break;
				}
				case X64OP_CMPXCHG:
				{
					u64 reg_value, old_value, cmp_value;
					if (!get_x64_reg_value(context, reg, d_size, i_size, reg_value) || !get_x64_reg_value(context, X64R_RAX, d_size, i_size, cmp_value))
					{
						return false;
					}

					switch (d_size)
					{
					case 1: old_value = sync_val_compare_and_swap((u8*)vm::base_priv(addr), (u8)cmp_value, (u8)reg_value); break;
					case 2: old_value = sync_val_compare_and_swap((u16*)vm::base_priv(addr), (u16)cmp_value, (u16)reg_value); break;
					case 4: old_value = sync_val_compare_and_swap((u32*)vm::base_priv(addr), (u32)cmp_value, (u32)reg_value); break;
					case 8: old_value = sync_val_compare_and_swap((u64*)vm::base_priv(addr), (u64)cmp_value, (u64)reg_value); break;
					default: return false;
					}

					if (!put_x64_reg_value(context, X64R_RAX, d_size, old_value) || !set_x64_cmp_flags(context, d_size, cmp_value, old_value))
					{
						return false;
					}
					break;
				}
				case X64OP_LOAD_AND_STORE:
				{
					u64 value;
					if (!get_x64_reg_value(context, reg, d_size, i_size, value))
					{
						return false;
					}

					switch (d_size)
					{
					case 1: value &= sync_fetch_and_and((u8*)vm::base_priv(addr), (u8)value); break;
					case 2: value &= sync_fetch_and_and((u16*)vm::base_priv(addr), (u16)value); break;
					case 4: value &= sync_fetch_and_and((u32*)vm::base_priv(addr), (u32)value); break;
					case 8: value &= sync_fetch_and_and((u64*)vm::base_priv(addr), (u64)value); break;
					default: return false;
					}

					if (!set_x64_cmp_flags(context, d_size, value, 0))
					{
						return false;
					}
					break;
				}
				default:
				{
					LOG_ERROR(memory, "Invalid or unsupported operation (op=%d, reg=%d, d_size=%lld, a_size=0x%llx, i_size=%lld)", op, reg, d_size, a_size, i_size);
					report_opcode();
					return false;
				}
				}

				// skip processed instruction
				RIP(context) += i_size;
				return true;
			});

			// TODO: allow recovering from a page fault as a feature of PS3 virtual memory
		}

		#ifdef _WIN32

		void _se_translator(unsigned int u, EXCEPTION_POINTERS* pExp)
		{
			const u64 addr64 = (u64)pExp->ExceptionRecord->ExceptionInformation[1] - (u64)vm::base(0);
			const bool is_writing = pExp->ExceptionRecord->ExceptionInformation[0] != 0;

			if (u == EXCEPTION_ACCESS_VIOLATION && (u32)addr64 == addr64)
			{
				throw EXCEPTION("Access violation %s location 0x%llx", is_writing ? "writing" : "reading", addr64);
			}

			__debugbreak(); // if it reached there, there should probably be a possibility to check the callstack

			throw EXCEPTION("Fatal error occured %s location %p at %p", is_writing ? "writing" : "reading", pExp->ExceptionRecord->ExceptionInformation[1], pExp->ExceptionRecord->ExceptionAddress);
		}

		const PVOID exception_handler = (atexit([]{ RemoveVectoredExceptionHandler(exception_handler); }), AddVectoredExceptionHandler(1, [](PEXCEPTION_POINTERS pExp) -> LONG
		{
			const u64 addr64 = (u64)pExp->ExceptionRecord->ExceptionInformation[1] - (u64)vm::base(0);
			const bool is_writing = pExp->ExceptionRecord->ExceptionInformation[0] != 0;

			if (pExp->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
				(u32)addr64 == addr64 &&
				get_current_thread_ctrl() &&
				handle_access_violation((u32)addr64, is_writing, pExp->ContextRecord))
			{
				return EXCEPTION_CONTINUE_EXECUTION;
			}
			else
			{
				return EXCEPTION_CONTINUE_SEARCH;
			}
		}));

		const auto exception_filter = SetUnhandledExceptionFilter([](PEXCEPTION_POINTERS pExp) -> LONG
		{
			_se_translator(pExp->ExceptionRecord->ExceptionCode, pExp);

			return EXCEPTION_CONTINUE_SEARCH;
		});

		#else

		void signal_handler(int sig, siginfo_t* info, void* uct)
		{
			const u64 addr64 = (u64)info->si_addr - (u64)vm::base(0);

		#ifdef __APPLE__
			const bool is_writing = ((ucontext_t*)uct)->uc_mcontext->__es.__err & 0x2;
		#else
			const bool is_writing = ((ucontext_t*)uct)->uc_mcontext.gregs[REG_ERR] & 0x2;
		#endif

			if ((u32)addr64 == addr64 && get_current_thread_ctrl())
			{
				if (handle_access_violation((u32)addr64, is_writing, (ucontext_t*)uct))
				{
					return; // proceed execution
				}

				// TODO: this may be wrong
				throw EXCEPTION("Access violation %s location 0x%llx", is_writing ? "writing" : "reading", addr64);
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

		thread_local thread_ctrl_t* g_tls_this_thread = nullptr;

		const thread_ctrl_t* get_current_thread_ctrl()
		{
			return g_tls_this_thread;
		}

		std::string thread_ctrl_t::get_name() const
		{
			return m_name();
		}

		named_thread_t::named_thread_t(std::function<std::string()> name, std::function<void()> func)
		{
			start(std::move(name), std::move(func));
		}

		named_thread_t::~named_thread_t()
		{
			if (m_thread)
			{
				std::printf("Fatal: thread neither joined nor detached\n");
				std::terminate();
			}
		}

		std::string named_thread_t::get_name() const
		{
			if (!m_thread)
			{
				throw EXCEPTION("Invalid thread");
			}

			if (!m_thread->m_name)
			{
				throw EXCEPTION("Invalid name getter");
			}

			return m_thread->m_name();
		}

		std::atomic<u32> g_thread_count{ 0 };

		void named_thread_t::start(std::function<std::string()> name, std::function<void()> func)
		{
			if (m_thread)
			{
				throw EXCEPTION("Thread already exists");
			}

			// create new thread control variable
			m_thread = std::make_shared<thread_ctrl_t>(std::move(name));

			// start thread
			m_thread->m_thread = std::thread([](std::shared_ptr<thread_ctrl_t> ctrl, std::function<void()> func)
			{
				g_tls_this_thread = ctrl.get();

				set_current_thread_debug_name(ctrl->get_name().c_str());

		#if defined(_MSC_VER)
				_set_se_translator(_se_translator);
		#endif

		#ifdef _WIN32
				if (!exception_handler || !exception_filter)
				{
					LOG_ERROR(general, "exception_handler not set");
					return;
				}
		#else
				if (sigaction_result == -1)
				{
					printf("sigaction() failed");
					exit(EXIT_FAILURE);
				}
		#endif

				try
				{
					g_thread_count++;

					if (rpcs3::config.misc.log.hle_logging.value())
					{
						LOG_NOTICE(general, "Thread started");
					}

					func();

					if (rpcs3::config.misc.log.hle_logging.value())
					{
						LOG_NOTICE(general, "Thread ended");
					}
				}
				catch (const std::exception& e)
				{
					LOG_ERROR(general, "Exception: %s", e.what());
					rpcs3::onpause();
				}
				catch (const closing_exception&)
				{
					LOG_ERROR(general, "Thread '%s' exiting...", ctrl->get_name());
				}

				for (auto& func : ctrl->m_atexit)
				{
					func();
					func = nullptr;
				}

				vm::reservation_free();

				g_thread_count--;

			}, m_thread, std::move(func));
		}

		void named_thread_t::detach()
		{
			if (!m_thread)
			{
				throw EXCEPTION("Invalid thread");
			}

			// +clear m_thread
			const auto ctrl = std::move(m_thread);

			// notify if detached by another thread
			if (g_tls_this_thread != m_thread.get())
			{
				// lock for reliable notification
				std::lock_guard<std::mutex> lock(mutex);

				cv.notify_one();
			}

			ctrl->m_thread.detach();
		}

		void named_thread_t::join()
		{
			if (!m_thread)
			{
				throw EXCEPTION("Invalid thread");
			}

			if (g_tls_this_thread == m_thread.get())
			{
				throw EXCEPTION("Deadlock");
			}

			// +clear m_thread
			const auto ctrl = std::move(m_thread);

			{
				// lock for reliable notification
				std::lock_guard<std::mutex> lock(mutex);

				cv.notify_one();
			}

			ctrl->m_thread.join();
		}

		bool named_thread_t::is_current() const
		{
			if (!m_thread)
			{
				throw EXCEPTION("Invalid thread");
			}

			return g_tls_this_thread == m_thread.get();
		}

		const std::function<bool()> SQUEUE_ALWAYS_EXIT = [](){ return true; };
		const std::function<bool()> SQUEUE_NEVER_EXIT = [](){ return false; };

		bool squeue_test_exit()
		{
			return rpcs3::state.closing;
		}
	}
}