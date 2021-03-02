﻿#include "stdafx.h"
#include "Utilities/JIT.h"
#include "Utilities/StrUtil.h"
#include "Crypto/sha1.h"
#include "Crypto/unself.h"
#include "Loader/ELF.h"
#include "Loader/mself.hpp"
#include "Emu/perf_meter.hpp"
#include "Emu/Memory/vm_reservation.h"
#include "Emu/Memory/vm_locking.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/VFS.h"
#include "PPUThread.h"
#include "PPUInterpreter.h"
#include "PPUAnalyser.h"
#include "PPUModule.h"
#include "PPUDisAsm.h"
#include "SPURecompiler.h"
#include "lv2/sys_sync.h"
#include "lv2/sys_prx.h"
#include "lv2/sys_overlay.h"
#include "lv2/sys_process.h"
#include "lv2/sys_memory.h"

#ifdef LLVM_AVAILABLE
#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Host.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/LLVMContext.h"
//#include "llvm/IR/Dominators.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/LegacyPassManager.h"
//#include "llvm/IR/Module.h"
//#include "llvm/IR/Function.h"
//#include "llvm/Analysis/Passes.h"
//#include "llvm/Analysis/BasicAliasAnalysis.h"
//#include "llvm/Analysis/TargetTransformInfo.h"
//#include "llvm/Analysis/MemoryDependenceAnalysis.h"
//#include "llvm/Analysis/LoopInfo.h"
//#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/Lint.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Vectorize.h"
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

#include "PPUTranslator.h"
#endif

#include <thread>
#include <cfenv>
#include <cctype>
#include "util/asm.hpp"
#include "util/vm.hpp"
#include "util/v128.hpp"
#include "util/v128sse.hpp"
#include "util/sysinfo.hpp"

const bool s_use_ssse3 = utils::has_ssse3();

extern u64 get_guest_system_time();

extern atomic_t<u64> g_watchdog_hold_ctr;

extern atomic_t<const char*> g_progr;
extern atomic_t<u32> g_progr_ftotal;
extern atomic_t<u32> g_progr_fdone;
extern atomic_t<u32> g_progr_ptotal;
extern atomic_t<u32> g_progr_pdone;

// Should be of the same type
using spu_rdata_t = decltype(ppu_thread::rdata);

extern void mov_rdata(spu_rdata_t& _dst, const spu_rdata_t& _src);
extern void mov_rdata_nt(spu_rdata_t& _dst, const spu_rdata_t& _src);
extern bool cmp_rdata(const spu_rdata_t& _lhs, const spu_rdata_t& _rhs);

// Verify AVX availability for TSX transactions
static const bool s_tsx_avx = utils::has_avx();

template <>
void fmt_class_string<ppu_join_status>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](ppu_join_status js)
	{
		switch (js)
		{
		case ppu_join_status::joinable: return "none";
		case ppu_join_status::detached: return "detached";
		case ppu_join_status::zombie: return "zombie";
		case ppu_join_status::exited: return "exited";
		case ppu_join_status::max: break;
		}

		return unknown;
	});
}

const ppu_decoder<ppu_interpreter_precise> g_ppu_interpreter_precise;
const ppu_decoder<ppu_interpreter_fast> g_ppu_interpreter_fast;
const ppu_decoder<ppu_itype> g_ppu_itype;

extern void ppu_initialize();
extern void ppu_finalize(const ppu_module& info);
extern bool ppu_initialize(const ppu_module& info, bool = false);
static void ppu_initialize2(class jit_compiler& jit, const ppu_module& module_part, const std::string& cache_path, const std::string& obj_name);
extern std::pair<std::shared_ptr<lv2_overlay>, CellError> ppu_load_overlay(const ppu_exec_object&, const std::string& path);
extern void ppu_unload_prx(const lv2_prx&);
extern std::shared_ptr<lv2_prx> ppu_load_prx(const ppu_prx_object&, const std::string&);
extern void ppu_execute_syscall(ppu_thread& ppu, u64 code);
static bool ppu_break(ppu_thread& ppu, ppu_opcode_t op);

extern void do_cell_atomic_128_store(u32 addr, const void* to_write);

const auto ppu_gateway = build_function_asm<void(*)(ppu_thread*)>([](asmjit::X86Assembler& c, auto& args)
{
	// Gateway for PPU, converts from native to GHC calling convention, also saves RSP value for escape
	using namespace asmjit;

#ifdef _WIN32
	c.push(x86::r15);
	c.push(x86::r14);
	c.push(x86::r13);
	c.push(x86::r12);
	c.push(x86::rsi);
	c.push(x86::rdi);
	c.push(x86::rbp);
	c.push(x86::rbx);
	c.sub(x86::rsp, 0xa8);
	c.movaps(x86::oword_ptr(x86::rsp, 0x90), x86::xmm15);
	c.movaps(x86::oword_ptr(x86::rsp, 0x80), x86::xmm14);
	c.movaps(x86::oword_ptr(x86::rsp, 0x70), x86::xmm13);
	c.movaps(x86::oword_ptr(x86::rsp, 0x60), x86::xmm12);
	c.movaps(x86::oword_ptr(x86::rsp, 0x50), x86::xmm11);
	c.movaps(x86::oword_ptr(x86::rsp, 0x40), x86::xmm10);
	c.movaps(x86::oword_ptr(x86::rsp, 0x30), x86::xmm9);
	c.movaps(x86::oword_ptr(x86::rsp, 0x20), x86::xmm8);
	c.movaps(x86::oword_ptr(x86::rsp, 0x10), x86::xmm7);
	c.movaps(x86::oword_ptr(x86::rsp, 0), x86::xmm6);
#else
	c.push(x86::rbp);
	c.push(x86::r15);
	c.push(x86::r14);
	c.push(x86::r13);
	c.push(x86::r12);
	c.push(x86::rbx);
	c.push(x86::rax);
#endif

	// Save native stack pointer for longjmp emulation
	c.mov(x86::qword_ptr(args[0], ::offset32(&ppu_thread::saved_native_sp)), x86::rsp);

	// Initialize args
	c.mov(x86::r13, x86::qword_ptr(reinterpret_cast<u64>(&vm::g_exec_addr)));
	c.mov(x86::rbp, args[0]);
	c.mov(x86::edx, x86::dword_ptr(x86::rbp, ::offset32(&ppu_thread::cia))); // Load PC

	c.mov(x86::rax, x86::qword_ptr(x86::r13, x86::edx, 1, 0)); // Load call target
	c.mov(x86::rdx, x86::rax);
	c.shl(x86::rax, 17);
	c.shr(x86::rax, 17);
	c.shr(x86::rdx, 47);
	c.shl(x86::rdx, 12);
	c.mov(x86::r12d, x86::edx); // Load relocation base

	c.mov(x86::rbx, x86::qword_ptr(reinterpret_cast<u64>(&vm::g_base_addr)));
	c.mov(x86::r14, x86::qword_ptr(x86::rbp, ::offset32(&ppu_thread::gpr, 0))); // Load some registers
	c.mov(x86::rsi, x86::qword_ptr(x86::rbp, ::offset32(&ppu_thread::gpr, 1)));
	c.mov(x86::rdi, x86::qword_ptr(x86::rbp, ::offset32(&ppu_thread::gpr, 2)));

	if (utils::has_avx())
	{
		c.vzeroupper();
	}

	c.call(x86::rax);

	if (utils::has_avx())
	{
		c.vzeroupper();
	}

#ifdef _WIN32
	c.movaps(x86::xmm6, x86::oword_ptr(x86::rsp, 0));
	c.movaps(x86::xmm7, x86::oword_ptr(x86::rsp, 0x10));
	c.movaps(x86::xmm8, x86::oword_ptr(x86::rsp, 0x20));
	c.movaps(x86::xmm9, x86::oword_ptr(x86::rsp, 0x30));
	c.movaps(x86::xmm10, x86::oword_ptr(x86::rsp, 0x40));
	c.movaps(x86::xmm11, x86::oword_ptr(x86::rsp, 0x50));
	c.movaps(x86::xmm12, x86::oword_ptr(x86::rsp, 0x60));
	c.movaps(x86::xmm13, x86::oword_ptr(x86::rsp, 0x70));
	c.movaps(x86::xmm14, x86::oword_ptr(x86::rsp, 0x80));
	c.movaps(x86::xmm15, x86::oword_ptr(x86::rsp, 0x90));
	c.add(x86::rsp, 0xa8);
	c.pop(x86::rbx);
	c.pop(x86::rbp);
	c.pop(x86::rdi);
	c.pop(x86::rsi);
	c.pop(x86::r12);
	c.pop(x86::r13);
	c.pop(x86::r14);
	c.pop(x86::r15);
#else
	c.add(x86::rsp, +8);
	c.pop(x86::rbx);
	c.pop(x86::r12);
	c.pop(x86::r13);
	c.pop(x86::r14);
	c.pop(x86::r15);
	c.pop(x86::rbp);
#endif

	c.ret();
});

const extern auto ppu_escape = build_function_asm<void(*)(ppu_thread*)>([](asmjit::X86Assembler& c, auto& args)
{
	using namespace asmjit;

	// Restore native stack pointer (longjmp emulation)
	c.mov(x86::rsp, x86::qword_ptr(args[0], ::offset32(&ppu_thread::saved_native_sp)));

	// Return to the return location
	c.jmp(x86::qword_ptr(x86::rsp, -8));
});

void ppu_recompiler_fallback(ppu_thread& ppu);

const auto ppu_recompiler_fallback_ghc = build_function_asm<void(*)(ppu_thread& ppu)>([](asmjit::X86Assembler& c, auto& args)
{
	using namespace asmjit;

	c.mov(args[0], x86::rbp);
	c.jmp(imm_ptr(ppu_recompiler_fallback));
});

// Get pointer to executable cache
static u64& ppu_ref(u32 addr)
{
	return *reinterpret_cast<u64*>(vm::g_exec_addr + u64{addr} * 2);
}

// Get interpreter cache value
static u64 ppu_cache(u32 addr)
{
	if (g_cfg.core.ppu_decoder > ppu_decoder_type::fast)
	{
		fmt::throw_exception("Invalid PPU decoder");
	}

	// Select opcode table
	const auto& table = *(
		g_cfg.core.ppu_decoder == ppu_decoder_type::precise
		? &g_ppu_interpreter_precise.get_table()
		: &g_ppu_interpreter_fast.get_table());

	return reinterpret_cast<uptr>(table[ppu_decode(vm::read32(addr))]);
}

static bool ppu_fallback(ppu_thread& ppu, ppu_opcode_t op)
{
	if (g_cfg.core.ppu_debug)
	{
		ppu_log.error("Unregistered instruction: 0x%08x", op.opcode);
	}

	ppu_ref(ppu.cia) = ppu_cache(ppu.cia);
	return false;
}

// TODO: Make this a dispatch call
void ppu_recompiler_fallback(ppu_thread& ppu)
{
	if (g_cfg.core.ppu_debug)
	{
		ppu_log.error("Unregistered PPU Function (LR=0x%llx)", ppu.lr);
	}

	const auto& table = g_ppu_interpreter_fast.get_table();

	u64 ctr = 0;

	while (true)
	{
		// Run instructions in interpreter
		if (const u32 op = vm::read32(ppu.cia); ctr++, table[ppu_decode(op)](ppu, {op})) [[likely]]
		{
			ppu.cia += 4;
			continue;
		}

		if (uptr func = ppu_ref(ppu.cia); (func << 17 >> 17) != reinterpret_cast<uptr>(ppu_recompiler_fallback_ghc))
		{
			// We found a recompiler function at cia, return
			break;
		}

		if (ppu.test_stopped())
		{
			break;
		}
	}

	if (g_cfg.core.ppu_debug)
	{
		ppu_log.warning("Exiting interpreter at 0x%x (executed %u functions)", ppu.cia, ctr);
	}
}

void ppu_reservation_fallback(ppu_thread& ppu)
{
	const auto& table = g_ppu_interpreter_fast.get_table();

	while (true)
	{
		// Run instructions in interpreter
		const u32 op = vm::read32(ppu.cia);

		if (table[ppu_decode(op)](ppu, {op})) [[likely]]
		{
			ppu.cia += 4;
		}

		if (!ppu.raddr || !ppu.use_full_rdata)
		{
			// We've escaped from reservation, return.
			return;
		}

		if (ppu.test_stopped())
		{
			return;
		}
	}
}

static std::unordered_map<u32, u32>* s_ppu_toc;

static bool ppu_check_toc(ppu_thread& ppu, ppu_opcode_t op)
{
	// Compare TOC with expected value
	const auto found = s_ppu_toc->find(ppu.cia);

	if (ppu.gpr[2] != found->second)
	{
		ppu_log.error("Unexpected TOC (0x%x, expected 0x%x)", ppu.gpr[2], found->second);

		if (!ppu.state.test_and_set(cpu_flag::dbg_pause) && ppu.check_state())
		{
			return false;
		}
	}

	// Fallback to the interpreter function
	const u64 val = ppu_cache(ppu.cia);
	if (reinterpret_cast<decltype(&ppu_interpreter::UNK)>(val & 0xffffffff)(ppu, {static_cast<u32>(val >> 32)}))
	{
		ppu.cia += 4;
	}

	return false;
}

extern void ppu_register_range(u32 addr, u32 size)
{
	if (!size)
	{
		ppu_log.error("ppu_register_range(0x%x): empty range", addr);
		return;
	}

	size = utils::align(size + addr % 0x10000, 0x10000);
	addr &= -0x10000;

	// Register executable range at
	utils::memory_commit(&ppu_ref(addr), u64{size} * 2, utils::protection::rw);
	vm::page_protect(addr, size, 0, vm::page_executable);

	if (g_cfg.core.ppu_debug)
	{
		utils::memory_commit(vm::g_stat_addr + addr, size);
	}

	const u64 fallback = reinterpret_cast<uptr>(ppu_fallback);
	const u64 seg_base = addr;

	while (size)
	{
		if (g_cfg.core.ppu_decoder == ppu_decoder_type::llvm)
		{
			// Assume addr is the start of first segment of PRX
			ppu_ref(addr) = reinterpret_cast<uptr>(ppu_recompiler_fallback_ghc) | (seg_base << (32 + 3));
		}
		else
		{
			ppu_ref(addr) = fallback;
		}

		addr += 4;
		size -= 4;
	}
}

extern void ppu_register_function_at(u32 addr, u32 size, ppu_function_t ptr)
{
	// Initialize specific function
	if (ptr)
	{
		ppu_ref(addr) = (reinterpret_cast<uptr>(ptr) & 0x7fff'ffff'ffffu) | (ppu_ref(addr) & ~0x7fff'ffff'ffffu);
		return;
	}

	if (!size)
	{
		if (g_cfg.core.ppu_debug)
		{
			ppu_log.error("ppu_register_function_at(0x%x): empty range", addr);
		}

		return;
	}

	if (g_cfg.core.ppu_decoder == ppu_decoder_type::llvm)
	{
		return;
	}

	// Initialize interpreter cache
	const u64 _break = reinterpret_cast<uptr>(ppu_break);

	while (size)
	{
		if (ppu_ref(addr) != _break)
		{
			ppu_ref(addr) = ppu_cache(addr);
		}

		addr += 4;
		size -= 4;
	}
}

// Breakpoint entry point
static bool ppu_break(ppu_thread& ppu, ppu_opcode_t op)
{
	// Pause
	ppu.state.atomic_op([](bs_t<cpu_flag>& state) { if (!(state & cpu_flag::dbg_step)) state += cpu_flag::dbg_pause; });

	if (ppu.check_state())
	{
		return false;
	}

	// Fallback to the interpreter function
	const u64 val = ppu_cache(ppu.cia);
	if (reinterpret_cast<decltype(&ppu_interpreter::UNK)>(val)(ppu, {vm::read32(ppu.cia).get()}))
	{
		ppu.cia += 4;
	}

	return false;
}

// Set or remove breakpoint
extern void ppu_breakpoint(u32 addr, bool isAdding)
{
	if (g_cfg.core.ppu_decoder == ppu_decoder_type::llvm)
	{
		return;
	}

	const u64 _break = reinterpret_cast<uptr>(&ppu_break);

	if (isAdding)
	{
		// Set breakpoint
		ppu_ref(addr) = _break;
	}
	else
	{
		// Remove breakpoint
		ppu_ref(addr) = ppu_cache(addr);
	}
}

//sets breakpoint, does nothing if there is a breakpoint there already
extern void ppu_set_breakpoint(u32 addr)
{
	if (g_cfg.core.ppu_decoder == ppu_decoder_type::llvm)
	{
		return;
	}

	const u64 _break = reinterpret_cast<uptr>(&ppu_break);

	if (ppu_ref(addr) != _break)
	{
		ppu_ref(addr) = _break;
	}
}

//removes breakpoint, does nothing if there is no breakpoint at location
extern void ppu_remove_breakpoint(u32 addr)
{
	if (g_cfg.core.ppu_decoder == ppu_decoder_type::llvm)
	{
		return;
	}

	const auto _break = reinterpret_cast<uptr>(&ppu_break);

	if (ppu_ref(addr) == _break)
	{
		ppu_ref(addr) = ppu_cache(addr);
	}
}

extern bool ppu_patch(u32 addr, u32 value)
{
	if (addr % 4)
	{
		ppu_log.fatal("Patch failed at 0x%x: unanligned memory address.", addr);
		return false;
	}

	vm::reader_lock rlock;

	if (!vm::check_addr(addr))
	{
		ppu_log.fatal("Patch failed at 0x%x: invalid memory address.", addr);
		return false;
	}

	const bool is_exec = vm::check_addr(addr, vm::page_executable);

	if (is_exec && g_cfg.core.ppu_decoder == ppu_decoder_type::llvm && !Emu.IsReady())
	{
		// TODO: support recompilers
		ppu_log.fatal("Patch failed at 0x%x: LLVM recompiler is used.", addr);
		return false;
	}

	*vm::get_super_ptr<u32>(addr) = value;

	const u64 _break = reinterpret_cast<uptr>(&ppu_break);
	const u64 fallback = reinterpret_cast<uptr>(&ppu_fallback);

	if (is_exec)
	{
		if (ppu_ref(addr) != _break && ppu_ref(addr) != fallback)
		{
			ppu_ref(addr) = ppu_cache(addr);
		}
	}

	return true;
}

std::array<u32, 2> op_branch_targets(u32 pc, ppu_opcode_t op)
{
	std::array<u32, 2> res{pc + 4, UINT32_MAX};

	switch (const auto type = g_ppu_itype.decode(op.opcode))
	{
	case ppu_itype::B:
	case ppu_itype::BC:
	{
		res[type == ppu_itype::BC ? 1 : 0] = ((op.aa ? 0 : pc) + (type == ppu_itype::B ? +op.bt24 : +op.bt14));
		break;
	}
	case ppu_itype::BCCTR:
	case ppu_itype::BCLR:
	case ppu_itype::UNK:
	{
		res[0] = UINT32_MAX;
		break;
	}
	default: break;
	}

	return res;
}

std::string ppu_thread::dump_regs() const
{
	std::string ret;

	for (uint i = 0; i < 32; ++i)
	{
		auto reg = gpr[i];

		fmt::append(ret, "r%d%s: 0x%-8llx", i, i <= 9 ? " " : "", reg);

		constexpr u32 max_str_len = 32;
		constexpr u32 hex_count = 8;

		if (reg <= UINT32_MAX && vm::check_addr<max_str_len>(static_cast<u32>(reg)))
		{
			bool is_function = false;
			u32 toc = 0;

			if (const u32 reg_ptr = *vm::get_super_ptr<u32>(static_cast<u32>(reg));
				vm::check_addr<max_str_len>(reg_ptr))
			{
				if ((reg | reg_ptr) % 4 == 0 && vm::check_addr(reg_ptr, vm::page_executable))
				{
					toc = *vm::get_super_ptr<u32>(static_cast<u32>(reg + 4));

					if (toc % 4 == 0 && vm::check_addr(toc))
					{
						is_function = true;
						reg = reg_ptr;
					}
				}
			}
			else if (reg % 4 == 0 && vm::check_addr(reg, vm::page_executable))
			{
				is_function = true;
			}

			const auto gpr_buf = vm::get_super_ptr<u8>(reg);

			std::string buf_tmp(gpr_buf, gpr_buf + max_str_len);

			std::string_view sv(buf_tmp.data(), std::min<usz>(buf_tmp.size(), buf_tmp.find_first_of("\0\n"sv)));

			if (is_function)
			{
				if (toc)
				{
					fmt::append(ret, " -> func(at=0x%x, toc=0x%x)", reg, toc);
				}
				else
				{
					PPUDisAsm dis_asm(cpu_disasm_mode::normal, vm::g_sudo_addr);
					dis_asm.disasm(reg);
					fmt::append(ret, " -> %s", dis_asm.last_opcode);
				}
			}
			// NTS: size of 3 and above is required
			// If ends with a newline, only one character is required
			else if ((sv.size() == buf_tmp.size() || (sv.size() >= (buf_tmp[sv.size()] == '\n' ? 1 : 3))) &&
				std::all_of(sv.begin(), sv.end(), [](u8 c){ return std::isprint(c); }))
			{
				fmt::append(ret, " -> \"%s\"", sv);
			}
			else
			{
				fmt::append(ret, " -> ");

				for (u32 j = 0; j < hex_count; ++j)
				{
					fmt::append(ret, "%02x ", buf_tmp[j]);
				}
			}
		}

		fmt::append(ret, "\n");
	}

	for (uint i = 0; i < 32; ++i)
	{
		fmt::append(ret, "f%d%s: %.6G\n", i, i <= 9 ? " " : "", fpr[i]);
	}

	for (uint i = 0; i < 32; ++i, ret += '\n')
	{
		fmt::append(ret, "v%d%s: ", i, i <= 9 ? " " : "");

		const auto r = vr[i];
		const u32 i3 = r.u32r[0];

		if (v128::from32p(i3) == r)
		{
			// Shortand formatting
			fmt::append(ret, "%08x", i3);
			fmt::append(ret, " [x: %g]", r.fr[0]);
		}
		else
		{
			fmt::append(ret, "%08x %08x %08x %08x", r.u32r[0], r.u32r[1], r.u32r[2], r.u32r[3]);
			fmt::append(ret, " [x: %g y: %g z: %g w: %g]", r.fr[0], r.fr[1], r.fr[2], r.fr[3]);
		}
	}

	fmt::append(ret, "CR: 0x%08x\n", cr.pack());
	fmt::append(ret, "LR: 0x%llx\n", lr);
	fmt::append(ret, "CTR: 0x%llx\n", ctr);
	fmt::append(ret, "VRSAVE: 0x%08x\n", vrsave);
	fmt::append(ret, "XER: [CA=%u | OV=%u | SO=%u | CNT=%u]\n", xer.ca, xer.ov, xer.so, xer.cnt);
	fmt::append(ret, "VSCR: [SAT=%u | NJ=%u]\n", sat, nj);
	fmt::append(ret, "FPSCR: [FL=%u | FG=%u | FE=%u | FU=%u]\n", fpscr.fl, fpscr.fg, fpscr.fe, fpscr.fu);
	if (const u32 addr = raddr)
		fmt::append(ret, "Reservation Addr: 0x%x", addr);
	else
		fmt::append(ret, "Reservation Addr: none");

	return ret;
}

std::string ppu_thread::dump_callstack() const
{
	std::string ret;

	fmt::append(ret, "Call stack:\n=========\n0x%08x (0x0) called\n", cia);

	for (const auto& sp : dump_callstack_list())
	{
		// TODO: function addresses too
		fmt::append(ret, "> from 0x%08x (sp=0x%08x)\n", sp.first, sp.second);
	}

	return ret;
}

std::vector<std::pair<u32, u32>> ppu_thread::dump_callstack_list() const
{
	//std::shared_lock rlock(vm::g_mutex); // Needs optimizations

	// Determine stack range
	const u64 r1 = gpr[1];

	if (r1 > UINT32_MAX || r1 % 0x10)
	{
		return {};
	}

	const u32 stack_ptr = static_cast<u32>(r1);

	if (!vm::check_addr(stack_ptr, vm::page_writable))
	{
		// Normally impossible unless the code does not follow ABI rules
		return {};
	}

	u32 stack_min = stack_ptr & ~0xfff;
	u32 stack_max = stack_min + 4096;

	while (stack_min && vm::check_addr(stack_min - 4096, vm::page_writable))
	{
		stack_min -= 4096;
	}

	while (stack_max + 4096 && vm::check_addr(stack_max, vm::page_writable))
	{
		stack_max += 4096;
	}

	std::vector<std::pair<u32, u32>> call_stack_list;

	bool first = true;

	for (
		u64 sp = r1;
		sp % 0x10 == 0u && sp >= stack_min && sp <= stack_max - ppu_stack_start_offset;
		sp = *vm::get_super_ptr<u64>(static_cast<u32>(sp)), first = false
		)
	{
		u64 addr = *vm::get_super_ptr<u64>(static_cast<u32>(sp + 16));

		auto is_invalid = [](u64 addr)
		{
			if (addr > UINT32_MAX || addr % 4 || !vm::check_addr(static_cast<u32>(addr), vm::page_executable))
			{
				return true;
			}

			// Ignore HLE stop address
			return addr == ppu_function_manager::func_addr(1) + 4;
		};

		if (is_invalid(addr))
		{
			if (first)
			{
				// Function hasn't saved LR, could be because it's a leaf function
				// Use LR directly instead
				addr = lr;

				if (is_invalid(addr))
				{
					// Skip it, workaround
					continue;
				}
			}
			else
			{
				break;
			}
		}

		// TODO: function addresses too
		call_stack_list.emplace_back(static_cast<u32>(addr), static_cast<u32>(sp));
	}

	return call_stack_list;
}

std::string ppu_thread::dump_misc() const
{
	std::string ret;

	fmt::append(ret, "Priority: %d\n", +prio);
	fmt::append(ret, "Stack: 0x%x..0x%x\n", stack_addr, stack_addr + stack_size - 1);
	fmt::append(ret, "Joiner: %s\n", joiner.load());

	if (const auto size = cmd_queue.size())
		fmt::append(ret, "Commands: %u\n", size);

	const char* _func = current_function;

	if (_func)
	{
		ret += "In function: ";
		ret += _func;
		ret += '\n';

		for (u32 i = 3; i <= 6; i++)
			if (gpr[i] != syscall_args[i - 3])
				fmt::append(ret, " ** r%d: 0x%llx\n", i, syscall_args[i - 3]);
	}
	else if (is_paused())
	{
		if (const auto last_func = last_function)
		{
			_func = last_func;
			ret += "Last function: ";
			ret += _func;
			ret += '\n';
		}
	}

	if (const auto _time = start_time)
	{
		fmt::append(ret, "Waiting: %fs\n", (get_guest_system_time() - _time) / 1000000.);
	}
	else
	{
		ret += '\n';
	}

	if (!_func)
	{
		ret += '\n';
	}
	return ret;
}

extern thread_local std::string(*g_tls_log_prefix)();

void ppu_thread::cpu_task()
{
	std::fesetround(FE_TONEAREST);

	if (g_cfg.core.set_daz_and_ftz && g_cfg.core.ppu_decoder != ppu_decoder_type::precise)
	{
		// Set DAZ and FTZ
		_mm_setcsr(_mm_getcsr() | 0x8840);
	}

	// Execute cmd_queue
	while (cmd64 cmd = cmd_wait())
	{
		const u32 arg = cmd.arg2<u32>(); // 32-bit arg extracted

		switch (auto type = cmd.arg1<ppu_cmd>())
		{
		case ppu_cmd::opcode:
		{
			cmd_pop(), g_cfg.core.ppu_decoder == ppu_decoder_type::precise
				? g_ppu_interpreter_precise.decode(arg)(*this, {arg})
				: g_ppu_interpreter_fast.decode(arg)(*this, {arg});
			break;
		}
		case ppu_cmd::set_gpr:
		{
			if (arg >= 32)
			{
				fmt::throw_exception("Invalid ppu_cmd::set_gpr arg (0x%x)", arg);
			}

			gpr[arg % 32] = cmd_get(1).as<u64>();
			cmd_pop(1);
			break;
		}
		case ppu_cmd::set_args:
		{
			if (arg > 8)
			{
				fmt::throw_exception("Unsupported ppu_cmd::set_args size (0x%x)", arg);
			}

			for (u32 i = 0; i < arg; i++)
			{
				gpr[i + 3] = cmd_get(1 + i).as<u64>();
			}

			cmd_pop(arg);
			break;
		}
		case ppu_cmd::lle_call:
		{
			const vm::ptr<u32> opd(arg < 32 ? vm::cast(gpr[arg]) : vm::cast(arg));
			cmd_pop(), fast_call(opd[0], opd[1]);
			break;
		}
		case ppu_cmd::hle_call:
		{
			cmd_pop(), ppu_function_manager::get().at(arg)(*this);
			break;
		}
		case ppu_cmd::opd_call:
		{
			const ppu_func_opd_t opd = cmd_get(1).as<ppu_func_opd_t>();
			cmd_pop(1), fast_call(opd.addr, opd.rtoc);
			break;
		}
		case ppu_cmd::ptr_call:
		{
			const ppu_function_t func = cmd_get(1).as<ppu_function_t>();
			cmd_pop(1), func(*this);
			break;
		}
		case ppu_cmd::initialize:
		{
			cmd_pop(), ppu_initialize(), spu_cache::initialize();
			break;
		}
		case ppu_cmd::sleep:
		{
			cmd_pop(), lv2_obj::sleep(*this);
			break;
		}
		case ppu_cmd::reset_stack:
		{
			cmd_pop(), gpr[1] = stack_addr + stack_size - ppu_stack_start_offset;
			break;
		}
		default:
		{
			fmt::throw_exception("Unknown ppu_cmd(0x%x)", static_cast<u32>(type));
		}
		}
	}
}

void ppu_thread::cpu_sleep()
{
	// Clear reservation
	raddr = 0;

	// Setup wait flag and memory flags to relock itself
	state += g_use_rtm ? cpu_flag::wait : cpu_flag::wait + cpu_flag::memory;

	if (auto ptr = vm::g_tls_locked)
	{
		ptr->compare_and_swap(this, nullptr);
	}

	lv2_obj::awake(this);
}

void ppu_thread::cpu_on_stop()
{
	if (current_function)
	{
		if (start_time)
		{
			ppu_log.warning("'%s' aborted (%fs)", current_function, (get_guest_system_time() - start_time) / 1000000.);
		}
		else
		{
			ppu_log.warning("'%s' aborted", current_function);
		}

		current_function = {};
	}
}

void ppu_thread::exec_task()
{
	if (g_cfg.core.ppu_decoder == ppu_decoder_type::llvm)
	{
		while (true)
		{
			if (state) [[unlikely]]
			{
				if (check_state())
					break;
			}

			ppu_gateway(this);
		}

		return;
	}

	const auto cache = vm::g_exec_addr;
	using func_t = decltype(&ppu_interpreter::UNK);

	while (true)
	{
		const auto exec_op = [this](u64 op)
		{
			return reinterpret_cast<func_t>(op)(*this, {vm::read32(cia).get()});
		};

		if (cia % 8 || state) [[unlikely]]
		{
			if (test_stopped()) return;

			// Decode single instruction (may be step)
			if (exec_op(*reinterpret_cast<u64*>(cache + u64{cia} * 2))) { cia += 4; }
			continue;
		}

		u64 op0, op1, op2, op3;
		u64 _pos = u64{cia} * 2;

		// Reinitialize
		{
			const v128 _op0 = *reinterpret_cast<const v128*>(cache + _pos);
			const v128 _op1 = *reinterpret_cast<const v128*>(cache + _pos + 16);
			op0 = _op0._u64[0];
			op1 = _op0._u64[1];
			op2 = _op1._u64[0];
			op3 = _op1._u64[1];
		}

		while (exec_op(op0)) [[likely]]
		{
			cia += 4;

			if (exec_op(op1)) [[likely]]
			{
				cia += 4;

				if (exec_op(op2)) [[likely]]
				{
					cia += 4;

					if (exec_op(op3)) [[likely]]
					{
						cia += 4;

						if (state) [[unlikely]]
						{
							break;
						}

						_pos += 32;
						const v128 _op0 = *reinterpret_cast<const v128*>(cache + _pos);
						const v128 _op1 = *reinterpret_cast<const v128*>(cache + _pos + 16);
						op0 = _op0._u64[0];
						op1 = _op0._u64[1];
						op2 = _op1._u64[0];
						op3 = _op1._u64[1];
						continue;
					}
					break;
				}
				break;
			}
			break;
		}
	}
}

ppu_thread::~ppu_thread()
{
	perf_log.notice("Perf stats for STCX reload: successs %u, failure %u", last_succ, last_fail);
}

ppu_thread::ppu_thread(const ppu_thread_params& param, std::string_view name, u32 prio, int detached)
	: cpu_thread(idm::last_id())
	, prio(prio)
	, stack_size(param.stack_size)
	, stack_addr(param.stack_addr)
	, joiner(detached != 0 ? ppu_join_status::detached : ppu_join_status::joinable)
	, entry_func(param.entry)
	, start_time(get_guest_system_time())
	, ppu_tname(make_single<std::string>(name))
{
	gpr[1] = stack_addr + stack_size - ppu_stack_start_offset;

	gpr[13] = param.tls_addr;

	if (detached >= 0)
	{
		// Initialize thread args
		gpr[3] = param.arg0;
		gpr[4] = param.arg1;
	}

	// Trigger the scheduler
	state += cpu_flag::suspend;

	if (!g_use_rtm)
	{
		state += cpu_flag::memory;
	}
}

void ppu_thread::cmd_push(cmd64 cmd)
{
	// Reserve queue space
	const u32 pos = cmd_queue.push_begin();

	// Write single command
	cmd_queue[pos] = cmd;
}

void ppu_thread::cmd_list(std::initializer_list<cmd64> list)
{
	// Reserve queue space
	const u32 pos = cmd_queue.push_begin(static_cast<u32>(list.size()));

	// Write command tail in relaxed manner
	for (u32 i = 1; i < list.size(); i++)
	{
		cmd_queue[pos + i].raw() = list.begin()[i];
	}

	// Write command head after all
	cmd_queue[pos] = *list.begin();
}

void ppu_thread::cmd_pop(u32 count)
{
	// Get current position
	const u32 pos = cmd_queue.peek();

	// Clean command buffer for command tail
	for (u32 i = 1; i <= count; i++)
	{
		cmd_queue[pos + i].raw() = cmd64{};
	}

	// Free
	cmd_queue.pop_end(count + 1);
}

cmd64 ppu_thread::cmd_wait()
{
	while (true)
	{
		if (cmd64 result = cmd_queue[cmd_queue.peek()].exchange(cmd64{}))
		{
			return result;
		}

		if (is_stopped())
		{
			return {};
		}

		thread_ctrl::wait_on(cmd_notify, 0);
		cmd_notify = 0;
	}
}

be_t<u64>* ppu_thread::get_stack_arg(s32 i, u64 align)
{
	if (align != 1 && align != 2 && align != 4 && align != 8 && align != 16) fmt::throw_exception("Unsupported alignment: 0x%llx", align);
	return vm::_ptr<u64>(vm::cast((gpr[1] + 0x30 + 0x8 * (i - 1)) & (0 - align)));
}

void ppu_thread::fast_call(u32 addr, u32 rtoc)
{
	const auto old_cia = cia;
	const auto old_rtoc = gpr[2];
	const auto old_lr = lr;
	const auto old_func = current_function;
	const auto old_fmt = g_tls_log_prefix;

	cia = addr;
	gpr[2] = rtoc;
	lr = ppu_function_manager::func_addr(1) + 4; // HLE stop address
	current_function = nullptr;

	g_tls_log_prefix = []
	{
		const auto _this = static_cast<ppu_thread*>(get_current_cpu_thread());

		static thread_local shared_ptr<std::string> name_cache;

		if (!_this->ppu_tname.is_equal(name_cache)) [[unlikely]]
		{
			_this->ppu_tname.peek_op([&](const shared_ptr<std::string>& ptr)
			{
				if (ptr != name_cache)
				{
					name_cache = ptr;
				}
			});
		}

		const auto cia = _this->cia;

		if (_this->current_function && vm::read32(cia) != ppu_instructions::SC(0))
		{
			return fmt::format("PPU[0x%x] Thread (%s) [HLE:0x%08x, LR:0x%08x]", _this->id, *name_cache.get(), cia, _this->lr);
		}

		return fmt::format("PPU[0x%x] Thread (%s) [0x%08x]", _this->id, *name_cache.get(), cia);
	};

	auto at_ret = [&]()
	{
		if (std::uncaught_exceptions())
		{
			cpu_on_stop();
			current_function = old_func;
		}
		else
		{
			state -= cpu_flag::ret;
			cia = old_cia;
			gpr[2] = old_rtoc;
			lr = old_lr;
			current_function = old_func;
			g_tls_log_prefix = old_fmt;
		}
	};

	exec_task();
	at_ret();
}

u32 ppu_thread::stack_push(u32 size, u32 align_v)
{
	if (auto cpu = get_current_cpu_thread()) if (cpu->id_type() == 1)
	{
		ppu_thread& context = static_cast<ppu_thread&>(*cpu);

		const u32 old_pos = vm::cast(context.gpr[1]);
		context.gpr[1] -= utils::align(size + 4, 8); // room minimal possible size
		context.gpr[1] &= ~(u64{align_v} - 1); // fix stack alignment

		if (old_pos >= context.stack_addr && old_pos < context.stack_addr + context.stack_size && context.gpr[1] < context.stack_addr)
		{
			fmt::throw_exception("Stack overflow (size=0x%x, align=0x%x, SP=0x%llx, stack=*0x%x)", size, align_v, old_pos, context.stack_addr);
		}
		else
		{
			const u32 addr = static_cast<u32>(context.gpr[1]);
			vm::_ref<nse_t<u32>>(addr + size) = old_pos;
			std::memset(vm::base(addr), 0, size);
			return addr;
		}
	}

	fmt::throw_exception("Invalid thread");
}

void ppu_thread::stack_pop_verbose(u32 addr, u32 size) noexcept
{
	if (auto cpu = get_current_cpu_thread()) if (cpu->id_type() == 1)
	{
		ppu_thread& context = static_cast<ppu_thread&>(*cpu);

		if (context.gpr[1] != addr)
		{
			ppu_log.error("Stack inconsistency (addr=0x%x, SP=0x%llx, size=0x%x)", addr, context.gpr[1], size);
			return;
		}

		context.gpr[1] = vm::_ref<nse_t<u32>>(static_cast<u32>(context.gpr[1]) + size);
		return;
	}

	ppu_log.error("Invalid thread");
}

extern u64 get_timebased_time();
extern ppu_function_t ppu_get_syscall(u64 code);

extern __m128 sse_exp2_ps(__m128 A);
extern __m128 sse_log2_ps(__m128 A);
extern __m128i sse_altivec_vperm(__m128i A, __m128i B, __m128i C);
extern __m128i sse_altivec_vperm_v0(__m128i A, __m128i B, __m128i C);
extern __m128i sse_altivec_lvsl(u64 addr);
extern __m128i sse_altivec_lvsr(u64 addr);
extern __m128i sse_cellbe_lvlx(u64 addr);
extern __m128i sse_cellbe_lvrx(u64 addr);
extern void sse_cellbe_stvlx(u64 addr, __m128i a);
extern void sse_cellbe_stvrx(u64 addr, __m128i a);
extern __m128i sse_cellbe_lvlx_v0(u64 addr);
extern __m128i sse_cellbe_lvrx_v0(u64 addr);
extern void sse_cellbe_stvlx_v0(u64 addr, __m128i a);
extern void sse_cellbe_stvrx_v0(u64 addr, __m128i a);

void ppu_trap(ppu_thread& ppu, u64 addr)
{
	ensure((addr & (~u64{UINT32_MAX} | 0x3)) == 0);
	ppu.cia = static_cast<u32>(addr);

	u32 add = static_cast<u32>(g_cfg.core.stub_ppu_traps) * 4;

	// If stubbing is enabled, check current instruction and the following
	if (!add || !vm::check_addr(ppu.cia, vm::page_executable) || !vm::check_addr(ppu.cia + add, vm::page_executable))
	{
		fmt::throw_exception("PPU Trap!");
	}

	ppu_log.error("PPU Trap: Stubbing %d instructions %s.", std::abs(static_cast<s32>(add) / 4), add >> 31 ? "backwards" : "forwards");
	ppu.cia += add; // Skip instructions, hope for valid code (interprter may be invoked temporarily)
}

[[noreturn]] static void ppu_error(ppu_thread& ppu, u64 addr, u32 op)
{
	ppu.cia = ::narrow<u32>(addr);
	fmt::throw_exception("Unknown/Illegal opcode 0x08x (0x%llx)", op, addr);
}

static void ppu_check(ppu_thread& ppu, u64 addr)
{
	ppu.cia = ::narrow<u32>(addr);

	if (ppu.test_stopped())
	{
		return;
	}
}

static void ppu_trace(u64 addr)
{
	ppu_log.notice("Trace: 0x%llx", addr);
}

template <typename T>
static T ppu_load_acquire_reservation(ppu_thread& ppu, u32 addr)
{
	perf_meter<"LARX"_u32> perf0;

	// Do not allow stores accessed from the same cache line to past reservation load
	atomic_fence_seq_cst();

	if (addr % sizeof(T))
	{
		fmt::throw_exception("PPU %s: Unaligned address: 0x%08x", sizeof(T) == 4 ? "LWARX" : "LDARX", addr);
	}

	// Always load aligned 64-bit value
	auto& data = vm::_ref<const atomic_be_t<u64>>(addr & -8);
	const u64 size_off = (sizeof(T) * 8) & 63;
	const u64 data_off = (addr & 7) * 8;

	ppu.raddr = addr;

	u32 addr_mask = -1;

	if (const s32 max = g_cfg.core.ppu_128_reservations_loop_max_length)
	{
		// If we use it in HLE it means we want the accurate version
		ppu.use_full_rdata = max < 0 || ppu.current_function || [&]()
		{
			const u32 cia = ppu.cia;

			if ((cia & 0xffff) >= 0x10000u - max * 4)
			{
				// Do not cross 64k boundary
				return false;
			}

			const auto inst = vm::_ptr<const nse_t<u32>>(cia);

			// Search for STWCX or STDCX nearby (LDARX-STWCX and LWARX-STDCX loops will use accurate 128-byte reservations)
			constexpr u32 store_cond = stx::se_storage<u32>::swap(sizeof(T) == 8 ? 0x7C00012D : 0x7C0001AD);
			constexpr u32 mask = stx::se_storage<u32>::swap(0xFC0007FF);

			const auto store_vec = v128::from32p(store_cond);
			const auto mask_vec = v128::from32p(mask);

			s32 i = 2;

			for (const s32 _max = max - 3; i < _max; i += 4)
			{
				const auto _inst = v128::loadu(inst + i) & mask_vec;

				if (_mm_movemask_epi8(v128::eq32(_inst, store_vec).vi))
				{
					return false;
				}
			}

			for (; i < max; i++)
			{
				const u32 val = inst[i] & mask;

				if (val == store_cond)
				{
					return false;
				}
			}

			return true;
		}();

		if (ppu.use_full_rdata)
		{
			addr_mask = -128;
		}
	}
	else
	{
		ppu.use_full_rdata = false;
	}

	if ((addr & addr_mask) == (ppu.last_faddr & addr_mask))
	{
		ppu_log.trace(u8"LARX after fail: addr=0x%x, faddr=0x%x, time=%u c", addr, ppu.last_faddr, (perf0.get() - ppu.last_ftsc));
	}

	if ((addr & addr_mask) == (ppu.last_faddr & addr_mask) && (perf0.get() - ppu.last_ftsc) < 600 && (vm::reservation_acquire(addr, sizeof(T)) & -128) == ppu.last_ftime)
	{
		be_t<u64> rdata;
		std::memcpy(&rdata, &ppu.rdata[addr & 0x78], 8);

		if (rdata == data.load())
		{
			ppu.rtime = ppu.last_ftime;
			ppu.raddr = ppu.last_faddr;
			ppu.last_ftime = 0;
			return static_cast<T>(rdata << data_off >> size_off);
		}

		ppu.last_fail++;
		ppu.last_faddr = 0;
	}
	else
	{
		// Silent failure
		ppu.last_faddr = 0;
	}

	ppu.rtime = vm::reservation_acquire(addr, sizeof(T)) & -128;

	be_t<u64> rdata;

	if (!ppu.use_full_rdata)
	{
		rdata = data.load();

		// Store only 64 bits of reservation data
		std::memcpy(&ppu.rdata[addr & 0x78], &rdata, 8);
	}
	else
	{
		mov_rdata(ppu.rdata, vm::_ref<spu_rdata_t>(addr & -128));
		atomic_fence_acquire();

		// Load relevant 64 bits of reservation data
		std::memcpy(&rdata, &ppu.rdata[addr & 0x78], 8);
	}

	return static_cast<T>(rdata << data_off >> size_off);
}

extern u32 ppu_lwarx(ppu_thread& ppu, u32 addr)
{
	return ppu_load_acquire_reservation<u32>(ppu, addr);
}

extern u64 ppu_ldarx(ppu_thread& ppu, u32 addr)
{
	return ppu_load_acquire_reservation<u64>(ppu, addr);
}

const auto ppu_stcx_accurate_tx = build_function_asm<u64(*)(u32 raddr, u64 rtime, const void* _old, u64 _new)>([](asmjit::X86Assembler& c, auto& args)
{
	using namespace asmjit;

	Label fall = c.newLabel();
	Label fail = c.newLabel();
	Label _ret = c.newLabel();
	Label skip = c.newLabel();
	Label next = c.newLabel();
	Label load = c.newLabel();

	//if (utils::has_avx() && !s_tsx_avx)
	//{
	//	c.vzeroupper();
	//}

	// Create stack frame if necessary (Windows ABI has only 6 volatile vector registers)
	c.push(x86::rbp);
	c.push(x86::r13);
	c.push(x86::r12);
	c.push(x86::rbx);
	c.push(x86::r14);
	c.push(x86::r15);
	c.sub(x86::rsp, 40);
#ifdef _WIN32
	if (!s_tsx_avx)
	{
		c.movups(x86::oword_ptr(x86::rsp, 0), x86::xmm6);
		c.movups(x86::oword_ptr(x86::rsp, 16), x86::xmm7);
	}
#endif

	// Prepare registers
	build_swap_rdx_with(c, args, x86::r12);
	c.mov(x86::rbp, x86::qword_ptr(reinterpret_cast<u64>(&vm::g_sudo_addr)));
	c.lea(x86::rbp, x86::qword_ptr(x86::rbp, args[0]));
	c.and_(x86::rbp, -128);
	c.prefetchw(x86::byte_ptr(x86::rbp, 0));
	c.prefetchw(x86::byte_ptr(x86::rbp, 64));
	c.movzx(args[0].r32(), args[0].r16());
	c.shr(args[0].r32(), 1);
	c.lea(x86::rbx, x86::qword_ptr(reinterpret_cast<u64>(+vm::g_reservations), args[0]));
	c.and_(x86::rbx, -128 / 2);
	c.prefetchw(x86::byte_ptr(x86::rbx));
	c.and_(args[0].r32(), 63);
	c.mov(x86::r13, args[1]);

	// Prepare data
	if (s_tsx_avx)
	{
		c.vmovups(x86::ymm0, x86::yword_ptr(args[2], 0));
		c.vmovups(x86::ymm1, x86::yword_ptr(args[2], 32));
		c.vmovups(x86::ymm2, x86::yword_ptr(args[2], 64));
		c.vmovups(x86::ymm3, x86::yword_ptr(args[2], 96));
	}
	else
	{
		c.movaps(x86::xmm0, x86::oword_ptr(args[2], 0));
		c.movaps(x86::xmm1, x86::oword_ptr(args[2], 16));
		c.movaps(x86::xmm2, x86::oword_ptr(args[2], 32));
		c.movaps(x86::xmm3, x86::oword_ptr(args[2], 48));
		c.movaps(x86::xmm4, x86::oword_ptr(args[2], 64));
		c.movaps(x86::xmm5, x86::oword_ptr(args[2], 80));
		c.movaps(x86::xmm6, x86::oword_ptr(args[2], 96));
		c.movaps(x86::xmm7, x86::oword_ptr(args[2], 112));
	}

	// Alloc r14 to stamp0
	const auto stamp0 = x86::r14;
	const auto stamp1 = x86::r15;
	build_get_tsc(c, stamp0);

	// Begin transaction
	Label tx0 = build_transaction_enter(c, fall, [&]()
	{
		build_get_tsc(c, stamp1);
		c.sub(stamp1, stamp0);
		c.xor_(x86::eax, x86::eax);
		c.cmp(stamp1, x86::qword_ptr(reinterpret_cast<u64>(&g_rtm_tx_limit1)));
		c.jae(fall);
	});
	c.bt(x86::dword_ptr(args[2], ::offset32(&ppu_thread::state) - ::offset32(&ppu_thread::rdata)), static_cast<u32>(cpu_flag::pause));
	c.jc(fall);
	c.xbegin(tx0);
	c.mov(x86::rax, x86::qword_ptr(x86::rbx));
	c.test(x86::eax, vm::rsrv_unique_lock);
	c.jnz(skip);
	c.and_(x86::rax, -128);
	c.cmp(x86::rax, x86::r13);
	c.jne(fail);

	if (s_tsx_avx)
	{
		c.vxorps(x86::ymm0, x86::ymm0, x86::yword_ptr(x86::rbp, 0));
		c.vxorps(x86::ymm1, x86::ymm1, x86::yword_ptr(x86::rbp, 32));
		c.vxorps(x86::ymm2, x86::ymm2, x86::yword_ptr(x86::rbp, 64));
		c.vxorps(x86::ymm3, x86::ymm3, x86::yword_ptr(x86::rbp, 96));
		c.vorps(x86::ymm0, x86::ymm0, x86::ymm1);
		c.vorps(x86::ymm1, x86::ymm2, x86::ymm3);
		c.vorps(x86::ymm0, x86::ymm1, x86::ymm0);
		c.vptest(x86::ymm0, x86::ymm0);
	}
	else
	{
		c.xorps(x86::xmm0, x86::oword_ptr(x86::rbp, 0));
		c.xorps(x86::xmm1, x86::oword_ptr(x86::rbp, 16));
		c.xorps(x86::xmm2, x86::oword_ptr(x86::rbp, 32));
		c.xorps(x86::xmm3, x86::oword_ptr(x86::rbp, 48));
		c.xorps(x86::xmm4, x86::oword_ptr(x86::rbp, 64));
		c.xorps(x86::xmm5, x86::oword_ptr(x86::rbp, 80));
		c.xorps(x86::xmm6, x86::oword_ptr(x86::rbp, 96));
		c.xorps(x86::xmm7, x86::oword_ptr(x86::rbp, 112));
		c.orps(x86::xmm0, x86::xmm1);
		c.orps(x86::xmm2, x86::xmm3);
		c.orps(x86::xmm4, x86::xmm5);
		c.orps(x86::xmm6, x86::xmm7);
		c.orps(x86::xmm0, x86::xmm2);
		c.orps(x86::xmm4, x86::xmm6);
		c.orps(x86::xmm0, x86::xmm4);
		c.ptest(x86::xmm0, x86::xmm0);
	}

	c.jnz(fail);

	// Store 8 bytes
	c.mov(x86::qword_ptr(x86::rbp, args[0], 1, 0), args[3]);

	// Update reservation
	c.sub(x86::qword_ptr(x86::rbx), -128);
	c.xend();
	build_get_tsc(c);
	c.sub(x86::rax, stamp0);
	c.jmp(_ret);

	// XABORT is expensive so finish with xend instead
	c.bind(fail);

	// Load old data to store back in rdata
	if (s_tsx_avx)
	{
		c.vmovaps(x86::ymm0, x86::yword_ptr(x86::rbp, 0));
		c.vmovaps(x86::ymm1, x86::yword_ptr(x86::rbp, 32));
		c.vmovaps(x86::ymm2, x86::yword_ptr(x86::rbp, 64));
		c.vmovaps(x86::ymm3, x86::yword_ptr(x86::rbp, 96));
	}
	else
	{
		c.movaps(x86::xmm0, x86::oword_ptr(x86::rbp, 0));
		c.movaps(x86::xmm1, x86::oword_ptr(x86::rbp, 16));
		c.movaps(x86::xmm2, x86::oword_ptr(x86::rbp, 32));
		c.movaps(x86::xmm3, x86::oword_ptr(x86::rbp, 48));
		c.movaps(x86::xmm4, x86::oword_ptr(x86::rbp, 64));
		c.movaps(x86::xmm5, x86::oword_ptr(x86::rbp, 80));
		c.movaps(x86::xmm6, x86::oword_ptr(x86::rbp, 96));
		c.movaps(x86::xmm7, x86::oword_ptr(x86::rbp, 112));
	}

	c.xend();
	c.jmp(load);

	c.bind(skip);
	c.xend();
	build_get_tsc(c, stamp1);
	//c.jmp(fall);

	c.bind(fall);

	Label fall2 = c.newLabel();
	Label fail2 = c.newLabel();
	Label fail3 = c.newLabel();

	// Lightened transaction: only compare and swap data
	c.bind(next);

	// Try to "lock" reservation
	c.mov(x86::eax, 1);
	c.lock().xadd(x86::qword_ptr(x86::rbx), x86::rax);
	c.test(x86::eax, vm::rsrv_unique_lock);
	c.jnz(fail2);

	// Check if already updated
	c.and_(x86::rax, -128);
	c.cmp(x86::rax, x86::r13);
	c.jne(fail2);

	// Exclude some time spent on touching memory: stamp1 contains last success or failure
	c.mov(x86::rax, stamp1);
	c.sub(x86::rax, stamp0);
	c.cmp(x86::rax, x86::qword_ptr(reinterpret_cast<u64>(&g_rtm_tx_limit2)));
	c.jae(fall2);
	build_get_tsc(c, stamp1);
	c.sub(stamp1, x86::rax);

	Label tx1 = build_transaction_enter(c, fall2, [&]()
	{
		build_get_tsc(c);
		c.sub(x86::rax, stamp1);
		c.cmp(x86::rax, x86::qword_ptr(reinterpret_cast<u64>(&g_rtm_tx_limit2)));
		c.jae(fall2);
		c.test(x86::qword_ptr(x86::rbx), 127 - 1);
		c.jnz(fall2);
	});
	c.prefetchw(x86::byte_ptr(x86::rbp, 0));
	c.prefetchw(x86::byte_ptr(x86::rbp, 64));

	// Check pause flag
	c.bt(x86::dword_ptr(args[2], ::offset32(&ppu_thread::state) - ::offset32(&ppu_thread::rdata)), static_cast<u32>(cpu_flag::pause));
	c.jc(fall2);
	c.mov(x86::rax, x86::qword_ptr(x86::rbx));
	c.and_(x86::rax, -128);
	c.cmp(x86::rax, x86::r13);
	c.jne(fail2);
	c.xbegin(tx1);

	if (s_tsx_avx)
	{
		c.vxorps(x86::ymm0, x86::ymm0, x86::yword_ptr(x86::rbp, 0));
		c.vxorps(x86::ymm1, x86::ymm1, x86::yword_ptr(x86::rbp, 32));
		c.vxorps(x86::ymm2, x86::ymm2, x86::yword_ptr(x86::rbp, 64));
		c.vxorps(x86::ymm3, x86::ymm3, x86::yword_ptr(x86::rbp, 96));
		c.vorps(x86::ymm0, x86::ymm0, x86::ymm1);
		c.vorps(x86::ymm1, x86::ymm2, x86::ymm3);
		c.vorps(x86::ymm0, x86::ymm1, x86::ymm0);
		c.vptest(x86::ymm0, x86::ymm0);
	}
	else
	{
		c.xorps(x86::xmm0, x86::oword_ptr(x86::rbp, 0));
		c.xorps(x86::xmm1, x86::oword_ptr(x86::rbp, 16));
		c.xorps(x86::xmm2, x86::oword_ptr(x86::rbp, 32));
		c.xorps(x86::xmm3, x86::oword_ptr(x86::rbp, 48));
		c.xorps(x86::xmm4, x86::oword_ptr(x86::rbp, 64));
		c.xorps(x86::xmm5, x86::oword_ptr(x86::rbp, 80));
		c.xorps(x86::xmm6, x86::oword_ptr(x86::rbp, 96));
		c.xorps(x86::xmm7, x86::oword_ptr(x86::rbp, 112));
		c.orps(x86::xmm0, x86::xmm1);
		c.orps(x86::xmm2, x86::xmm3);
		c.orps(x86::xmm4, x86::xmm5);
		c.orps(x86::xmm6, x86::xmm7);
		c.orps(x86::xmm0, x86::xmm2);
		c.orps(x86::xmm4, x86::xmm6);
		c.orps(x86::xmm0, x86::xmm4);
		c.ptest(x86::xmm0, x86::xmm0);
	}

	c.jnz(fail3);

	// Store 8 bytes
	c.mov(x86::qword_ptr(x86::rbp, args[0], 1, 0), args[3]);

	c.xend();
	c.lock().add(x86::qword_ptr(x86::rbx), 127);
	build_get_tsc(c);
	c.sub(x86::rax, stamp0);
	c.jmp(_ret);

	// XABORT is expensive so try to finish with xend instead
	c.bind(fail3);

	// Load old data to store back in rdata
	if (s_tsx_avx)
	{
		c.vmovaps(x86::ymm0, x86::yword_ptr(x86::rbp, 0));
		c.vmovaps(x86::ymm1, x86::yword_ptr(x86::rbp, 32));
		c.vmovaps(x86::ymm2, x86::yword_ptr(x86::rbp, 64));
		c.vmovaps(x86::ymm3, x86::yword_ptr(x86::rbp, 96));
	}
	else
	{
		c.movaps(x86::xmm0, x86::oword_ptr(x86::rbp, 0));
		c.movaps(x86::xmm1, x86::oword_ptr(x86::rbp, 16));
		c.movaps(x86::xmm2, x86::oword_ptr(x86::rbp, 32));
		c.movaps(x86::xmm3, x86::oword_ptr(x86::rbp, 48));
		c.movaps(x86::xmm4, x86::oword_ptr(x86::rbp, 64));
		c.movaps(x86::xmm5, x86::oword_ptr(x86::rbp, 80));
		c.movaps(x86::xmm6, x86::oword_ptr(x86::rbp, 96));
		c.movaps(x86::xmm7, x86::oword_ptr(x86::rbp, 112));
	}

	c.xend();
	c.jmp(fail2);

	c.bind(fall2);
	c.mov(x86::rax, -1);
	c.jmp(_ret);

	c.bind(fail2);
	c.lock().sub(x86::qword_ptr(x86::rbx), 1);
	c.bind(load);

	// Store previous data back to rdata
	if (s_tsx_avx)
	{
		c.vmovaps(x86::yword_ptr(args[2], 0), x86::ymm0);
		c.vmovaps(x86::yword_ptr(args[2], 32), x86::ymm1);
		c.vmovaps(x86::yword_ptr(args[2], 64), x86::ymm2);
		c.vmovaps(x86::yword_ptr(args[2], 96), x86::ymm3);
	}
	else
	{
		c.movaps(x86::oword_ptr(args[2], 0), x86::xmm0);
		c.movaps(x86::oword_ptr(args[2], 16), x86::xmm1);
		c.movaps(x86::oword_ptr(args[2], 32), x86::xmm2);
		c.movaps(x86::oword_ptr(args[2], 48), x86::xmm3);
		c.movaps(x86::oword_ptr(args[2], 64), x86::xmm4);
		c.movaps(x86::oword_ptr(args[2], 80), x86::xmm5);
		c.movaps(x86::oword_ptr(args[2], 96), x86::xmm6);
		c.movaps(x86::oword_ptr(args[2], 112), x86::xmm7);
	}

	c.mov(x86::rax, -1);
	c.mov(x86::qword_ptr(args[2], ::offset32(&ppu_thread::last_ftime) - ::offset32(&ppu_thread::rdata)), x86::rax);
	c.xor_(x86::eax, x86::eax);
	//c.jmp(_ret);

	c.bind(_ret);

#ifdef _WIN32
	if (!s_tsx_avx)
	{
		c.vmovups(x86::xmm6, x86::oword_ptr(x86::rsp, 0));
		c.vmovups(x86::xmm7, x86::oword_ptr(x86::rsp, 16));
	}
#endif

	if (s_tsx_avx)
	{
		c.vzeroupper();
	}

	c.add(x86::rsp, 40);
	c.pop(x86::r15);
	c.pop(x86::r14);
	c.pop(x86::rbx);
	c.pop(x86::r12);
	c.pop(x86::r13);
	c.pop(x86::rbp);
	c.ret();
});

template <typename T>
static bool ppu_store_reservation(ppu_thread& ppu, u32 addr, u64 reg_value)
{
	perf_meter<"STCX"_u32> perf0;

	if (addr % sizeof(T))
	{
		fmt::throw_exception("PPU %s: Unaligned address: 0x%08x", sizeof(T) == 4 ? "STWCX" : "STDCX", addr);
	}

	auto& data = vm::_ref<atomic_be_t<u64>>(addr & -8);
	auto& res = vm::reservation_acquire(addr, sizeof(T));
	const u64 rtime = ppu.rtime;

	be_t<u64> old_data = 0;
	std::memcpy(&old_data, &ppu.rdata[addr & 0x78], sizeof(old_data));
	be_t<u64> new_data = old_data;

	if constexpr (sizeof(T) == sizeof(u32))
	{
		// Rebuild reg_value to be 32-bits of new data and 32-bits of old data
		const be_t<u32> reg32 = static_cast<u32>(reg_value);
		std::memcpy(reinterpret_cast<char*>(&new_data) + (addr & 4), &reg32, sizeof(u32));
	}
	else
	{
		new_data = reg_value;
	}

	// Test if store address is on the same aligned 8-bytes memory as load
	if (const u32 raddr = std::exchange(ppu.raddr, 0); raddr / 8 != addr / 8)
	{
		// If not and it is on the same aligned 128-byte memory, proceed only if 128-byte reservations are enabled
		// In realhw the store address can be at any address of the 128-byte cache line
		if (raddr / 128 != addr / 128 || !ppu.use_full_rdata)
		{
			// Even when the reservation address does not match the target address must be valid
			if (!vm::check_addr(addr, vm::page_writable))
			{
				// Access violate
				data += 0;
			}

			return false;
		}
	}

	if (old_data != data || rtime != (res & -128))
	{
		return false;
	}

	if ([&]()
	{
		if (ppu.use_full_rdata) [[unlikely]]
		{
			if (g_use_rtm) [[likely]]
			{
				switch (u64 count = ppu_stcx_accurate_tx(addr & -8, rtime, ppu.rdata, std::bit_cast<u64>(new_data)))
				{
				case UINT64_MAX:
				{
					auto& all_data = *vm::get_super_ptr<spu_rdata_t>(addr & -128);
					auto& sdata = *vm::get_super_ptr<atomic_be_t<u64>>(addr & -8);

					const bool ok = cpu_thread::suspend_all<+3>(&ppu, {all_data, all_data + 64, &res}, [&]
					{
						if ((res & -128) == rtime && cmp_rdata(ppu.rdata, all_data))
						{
							sdata.release(new_data);
							res += 127;
							return true;
						}

						mov_rdata_nt(ppu.rdata, all_data);
						res -= 1;
						return false;
					});

					if (ok)
					{
						break;
					}

					ppu.last_ftime = -1;
					[[fallthrough]];
				}
				case 0:
				{
					if (ppu.last_faddr == addr)
					{
						ppu.last_fail++;
					}

					if (ppu.last_ftime != umax)
					{
						ppu.last_faddr = 0;
						return false;
					}

					utils::prefetch_read(ppu.rdata);
					utils::prefetch_read(ppu.rdata + 64);
					ppu.last_faddr = addr;
					ppu.last_ftime = res.load() & -128;
					ppu.last_ftsc = __rdtsc();
					return false;
				}
				default:
				{
					if (count > 20000 && g_cfg.core.perf_report) [[unlikely]]
					{
						perf_log.warning(u8"STCX: took too long: %.3fµs (%u c)", count / (utils::get_tsc_freq() / 1000'000.), count);
					}

					break;
				}
				}

				if (ppu.last_faddr == addr)
				{
					ppu.last_succ++;
				}

				ppu.last_faddr = 0;
				return true;
			}

			auto [_oldd, _ok] = res.fetch_op([&](u64& r)
			{
				if ((r & -128) != rtime || (r & 127))
				{
					return false;
				}

				r += vm::rsrv_unique_lock;
				return true;
			});

			if (!_ok)
			{
				// Already locked or updated: give up
				return false;
			}

			// Align address: we do not need the lower 7 bits anymore
			addr &= -128;

			// Cache line data
			//auto& cline_data = vm::_ref<spu_rdata_t>(addr);

			data += 0;
			rsx::reservation_lock rsx_lock(addr, 128);

			auto& super_data = *vm::get_super_ptr<spu_rdata_t>(addr);
			const bool success = [&]()
			{
				// Full lock (heavyweight)
				// TODO: vm::check_addr
				vm::writer_lock lock(addr);

				if (cmp_rdata(ppu.rdata, super_data))
				{
					data.release(new_data);
					res += 64;
					return true;
				}

				res -= 64;
				return false;
			}();

			return success;
		}

		if (new_data == old_data)
		{
			ppu.last_faddr = 0;
			return res.compare_and_swap_test(rtime, rtime + 128);
		}

		// Aligned 8-byte reservations will be used here
		addr &= -8;

		const u64 lock_bits = g_cfg.core.spu_accurate_dma ? vm::rsrv_unique_lock : 1;

		auto [_oldd, _ok] = res.fetch_op([&](u64& r)
		{
			if ((r & -128) != rtime || (r & 127))
			{
				return false;
			}

			// Despite using shared lock, doesn't allow other shared locks (TODO)
			r += lock_bits;
			return true;
		});

		// Give up if reservation has been locked or updated
		if (!_ok)
		{
			ppu.last_faddr = 0;
			return false;
		}

		// Store previous value in old_data on failure
		if (data.compare_exchange(old_data, new_data))
		{
			res += 128 - lock_bits;
			return true;
		}

		const u64 old_rtime = res.fetch_sub(lock_bits);

		// TODO: disabled with this setting on, since it's dangerous to mix
		if (!g_cfg.core.ppu_128_reservations_loop_max_length)
		{
			// Store old_data on failure
			if (ppu.last_faddr == addr)
			{
				ppu.last_fail++;
			}

			ppu.last_faddr = addr;
			ppu.last_ftime = old_rtime & -128;
			ppu.last_ftsc = __rdtsc();
			std::memcpy(&ppu.rdata[addr & 0x78], &old_data, 8);
		}

		return false;
	}())
	{
		res.notify_all(-128);

		if (addr == ppu.last_faddr)
		{
			ppu.last_succ++;
		}

		ppu.last_faddr = 0;
		return true;
	}

	return false;
}

extern bool ppu_stwcx(ppu_thread& ppu, u32 addr, u32 reg_value)
{
	return ppu_store_reservation<u32>(ppu, addr, reg_value);
}

extern bool ppu_stdcx(ppu_thread& ppu, u32 addr, u64 reg_value)
{
	return ppu_store_reservation<u64>(ppu, addr, reg_value);
}

#ifdef LLVM_AVAILABLE
namespace
{
	// Compiled PPU module info
	struct jit_module
	{
		std::vector<ppu_function_t> funcs;
		std::shared_ptr<jit_compiler> pjit;
		bool init = false;
	};

	struct jit_module_manager
	{
		shared_mutex mutex;
		std::unordered_map<std::string, jit_module> map;

		jit_module& get(const std::string& name)
		{
			std::lock_guard lock(mutex);
			return map.emplace(name, jit_module{}).first->second;
		}

		void remove(const std::string& name) noexcept
		{
			std::lock_guard lock(mutex);

			const auto found = map.find(name);

			if (found == map.end()) [[unlikely]]
			{
				ppu_log.error("Failed to remove module %s", name);
				return;
			}

			map.erase(found);
		}
	};
}
#endif

namespace
{
	// Read-only file view starting with specified offset (for MSELF)
	struct file_view : fs::file_base
	{
		const fs::file m_file;
		const u64 m_off;
		u64 m_pos;

		explicit file_view(fs::file&& _file, u64 offset)
			: m_file(std::move(_file))
			, m_off(offset)
			, m_pos(0)
		{
		}

		~file_view() override
		{
		}

		fs::stat_t stat() override
		{
			return m_file.stat();
		}

		bool trunc(u64 length) override
		{
			return false;
		}

		u64 read(void* buffer, u64 size) override
		{
			const u64 old_pos = m_file.pos();
			m_file.seek(m_off + m_pos);
			const u64 result = m_file.read(buffer, size);
			ensure(old_pos == m_file.seek(old_pos));

			m_pos += result;
			return result;
		}

		u64 write(const void* buffer, u64 size) override
		{
			return 0;
		}

		u64 seek(s64 offset, fs::seek_mode whence) override
		{
			const s64 new_pos =
				whence == fs::seek_set ? offset :
				whence == fs::seek_cur ? offset + m_pos :
				whence == fs::seek_end ? offset + size() : -1;

			if (new_pos < 0)
			{
				fs::g_tls_error = fs::error::inval;
				return -1;
			}

			m_pos = new_pos;
			return m_pos;
		}

		u64 size() override
		{
			return m_file.size();
		}
	};
}

extern void ppu_finalize(const ppu_module& info)
{
	// Get cache path for this executable
	std::string cache_path;

	if (info.name.empty())
	{
		// Don't remove main module from memory
		return;
	}
	else
	{
		// Get PPU cache location
		cache_path = fs::get_cache_dir() + "cache/";

		const std::string dev_flash = vfs::get("/dev_flash/");

		if (info.path.starts_with(dev_flash) || Emu.GetCat() == "1P")
		{
			// Don't remove dev_flash prx from memory
			return;
		}
		else if (!Emu.GetTitleID().empty())
		{
			cache_path += Emu.GetTitleID();
			cache_path += '/';
		}

		// Add PPU hash and filename
		fmt::append(cache_path, "ppu-%s-%s/", fmt::base57(info.sha1), info.path.substr(info.path.find_last_of('/') + 1));
	}

#ifdef LLVM_AVAILABLE
	g_fxo->get<jit_module_manager>().remove(cache_path + info.name);
#endif
}

extern void ppu_precompile(std::vector<std::string>& dir_queue, std::vector<lv2_prx*>* loaded_prx)
{
	if (g_cfg.core.ppu_decoder != ppu_decoder_type::llvm)
	{
		return;
	}

	// Make sure we only have one '/' at the end and remove duplicates.
	for (std::string& dir : dir_queue)
	{
		while (dir.back() == '/' || dir.back() == '\\')
			dir.pop_back();
		dir += '/';
	}
	std::sort(dir_queue.begin(), dir_queue.end());
	dir_queue.erase(std::unique(dir_queue.begin(), dir_queue.end()), dir_queue.end());

	const std::string firmware_sprx_path = vfs::get("/dev_flash/sys/external/");

	// Map fixed address executables area, fake overlay support
	const bool had_ovl = !vm::map(0x3000'0000, 0x1000'0000, 0x202).operator bool();
	const u32 ppc_seg = std::exchange(g_ps3_process_info.ppc_seg, 0x3);

	std::vector<std::pair<std::string, u64>> file_queue;
	file_queue.reserve(2000);

	// Find all .sprx files recursively
	for (usz i = 0; i < dir_queue.size(); i++)
	{
		if (Emu.IsStopped())
		{
			file_queue.clear();
			break;
		}

		ppu_log.notice("Scanning directory: %s", dir_queue[i]);

		for (auto&& entry : fs::dir(dir_queue[i]))
		{
			if (Emu.IsStopped())
			{
				file_queue.clear();
				break;
			}

			if (entry.is_directory)
			{
				if (entry.name != "." && entry.name != "..")
				{
					dir_queue.emplace_back(dir_queue[i] + entry.name + '/');
				}

				continue;
			}

			std::string upper = fmt::to_upper(entry.name);

			// Check .sprx filename
			if (upper.ends_with(".SPRX"))
			{
				// Skip already loaded modules or HLEd ones
				if (dir_queue[i] == firmware_sprx_path)
				{
					bool ignore = false;

					if (loaded_prx)
					{
						for (auto* obj : *loaded_prx)
						{
							if (obj->name == entry.name)
							{
								ignore = true;
								break;
							}
						}

						if (ignore)
						{
							continue;
						}
					}

					if (g_cfg.core.libraries_control.get_set().count(entry.name + ":lle"))
					{
						// Force LLE
						ignore = false;
					}
					else if (g_cfg.core.libraries_control.get_set().count(entry.name + ":hle"))
					{
						// Force HLE
						ignore = true;
					}
					else
					{
						extern const std::map<std::string_view, int> g_prx_list;

						// Use list
						ignore = g_prx_list.count(entry.name) && g_prx_list.at(entry.name) != 0;
					}

					if (ignore)
					{
						continue;
					}
				}

				// Get full path
				file_queue.emplace_back(dir_queue[i] + entry.name, 0);
				continue;
			}

			// Check .self filename
			if (upper.ends_with(".SELF"))
			{
				// Get full path
				file_queue.emplace_back(dir_queue[i] + entry.name,  0);
				continue;
			}

			// Check .mself filename
			if (upper.ends_with(".MSELF"))
			{
				if (fs::file mself{dir_queue[i] + entry.name})
				{
					mself_header hdr{};

					if (mself.read(hdr) && hdr.get_count(mself.size()))
					{
						for (u32 j = 0; j < hdr.count; j++)
						{
							mself_record rec{};

							if (mself.read(rec) && rec.get_pos(mself.size()))
							{
								std::string name = rec.name;

								upper = fmt::to_upper(name);

								if (upper.ends_with(".SPRX"))
								{
									// .sprx inside .mself found
									file_queue.emplace_back(dir_queue[i] + entry.name, rec.off);
									continue;
								}

								if (upper.ends_with(".SELF"))
								{
									// .self inside .mself found
									file_queue.emplace_back(dir_queue[i] + entry.name, rec.off);
									continue;
								}
							}
							else
							{
								ppu_log.error("MSELF file is possibly truncated");
								break;
							}
						}
					}
				}
			}
		}
	}

	g_progr = "Compiling PPU modules";

	g_progr_ftotal += file_queue.size();

	atomic_t<usz> fnext = 0;

	shared_mutex sprx_mtx, ovl_mtx;

	named_thread_group workers("SPRX Worker ", std::min<u32>(utils::get_thread_count(), ::size32(file_queue)), [&]
	{
		// Set low priority
		thread_ctrl::scoped_priority low_prio(-1);

		for (usz func_i = fnext++; func_i < file_queue.size(); func_i = fnext++, g_progr_fdone++)
		{
			if (Emu.IsStopped())
			{
				continue;
			}

			auto [path, offset] = std::as_const(file_queue)[func_i];

			ppu_log.notice("Trying to load: %s", path);

			// Load MSELF, SPRX or SELF
			fs::file src{path};

			if (!src)
			{
				ppu_log.error("Failed to open '%s' (%s)", path, fs::g_tls_error);
				continue;
			}

			if (u64 off = offset)
			{
				// Adjust offset for MSELF
				src.reset(std::make_unique<file_view>(std::move(src), off));

				// Adjust path for MSELF too
				fmt::append(path, "_x%x", off);
			}

			// Some files may fail to decrypt due to the lack of klic
			src = decrypt_self(std::move(src));

			if (!src)
			{
				ppu_log.error("Failed to decrypt '%s'", path);
				continue;
			}

			elf_error prx_err{}, ovl_err{};

			if (ppu_prx_object obj = src; (prx_err = obj, obj == elf_error::ok))
			{
				std::unique_lock lock(sprx_mtx);

				if (auto prx = ppu_load_prx(obj, path))
				{
					lock.unlock();
					obj.clear(), src.close(); // Clear decrypted file and elf object memory
					ppu_initialize(*prx);
					idm::remove<lv2_obj, lv2_prx>(idm::last_id());
					lock.lock();
					ppu_unload_prx(*prx);
					lock.unlock();
					ppu_finalize(*prx);
					continue;
				}

				// Log error
				prx_err = elf_error::header_type;
			}

			if (ppu_exec_object obj = src; (ovl_err = obj, obj == elf_error::ok))
			{
				while (ovl_err == elf_error::ok)
				{
					// Only one thread compiles OVL atm, other can compile PRX cuncurrently
					std::unique_lock lock(ovl_mtx);

					auto [ovlm, error] = ppu_load_overlay(obj, path);

					if (error)
					{
						// Abort
						ovl_err = elf_error::header_type;
						break;
					}

					obj.clear(), src.close(); // Clear decrypted file and elf object memory

					ppu_initialize(*ovlm);

					for (auto& seg : ovlm->segs)
					{
						vm::dealloc(seg.addr);
					}

					lock.unlock();
					idm::remove<lv2_obj, lv2_overlay>(idm::last_id());
					ppu_finalize(*ovlm);
					break;
				}

				if (ovl_err == elf_error::ok)
				{
					continue;
				}
			}

			ppu_log.error("Failed to precompile '%s' (prx: %s, ovl: %s)", path, prx_err, ovl_err);
			continue;
		}
	});

	// Join every thread
	workers.join();

	// Revert changes

	if (!had_ovl)
	{
		ensure(vm::unmap(0x3000'0000));
	}

	g_ps3_process_info.ppc_seg = ppc_seg;
}

extern void ppu_initialize()
{
	auto& _main = g_fxo->get<ppu_module>();

	if (!g_fxo->is_init<ppu_module>())
	{
		return;
	}

	if (Emu.IsStopped())
	{
		return;
	}

	bool compile_main = false;

	// Check main module cache
	if (!_main.segs.empty())
	{
		compile_main = ppu_initialize(_main, true);
	}

	std::vector<lv2_prx*> prx_list;

	idm::select<lv2_obj, lv2_prx>([&](u32, lv2_prx& prx)
	{
		prx_list.emplace_back(&prx);
	});

	// If empty we have no indication for cache state, check everything
	bool compile_fw = prx_list.empty();

	// Check preloaded libraries cache
	for (auto ptr : prx_list)
	{
		compile_fw |= ppu_initialize(*ptr, true);
	}

	std::vector<std::string> dir_queue;

	if (compile_fw)
	{
		const std::string firmware_sprx_path = vfs::get("/dev_flash/sys/external/");
		dir_queue.emplace_back(firmware_sprx_path);
	}

	// Avoid compilation if main's cache exists or it is a standalone SELF with no PARAM.SFO
	if (compile_main && !Emu.GetTitleID().empty())
	{
		// Try to add all related directories
		const std::set<std::string> dirs = Emu.GetGameDirs();
		dir_queue.insert(std::end(dir_queue), std::begin(dirs), std::end(dirs));
	}

	ppu_precompile(dir_queue, &prx_list);

	if (Emu.IsStopped())
	{
		return;
	}

	// Initialize main module cache
	if (!_main.segs.empty())
	{
		ppu_initialize(_main);
	}

	// Initialize preloaded libraries
	for (auto ptr : prx_list)
	{
		if (Emu.IsStopped())
		{
			return;
		}

		ppu_initialize(*ptr);
	}
}

struct ppu_toc_manager
{
	std::unordered_map<u32, u32> toc_map;

	shared_mutex mutex;
};

bool ppu_initialize(const ppu_module& info, bool check_only)
{
	if (g_cfg.core.ppu_decoder != ppu_decoder_type::llvm)
	{
		if (check_only)
		{
			return false;
		}

		// Temporarily
		s_ppu_toc = &g_fxo->get<ppu_toc_manager>().toc_map;

		for (const auto& func : info.funcs)
		{
			for (auto& block : func.blocks)
			{
				ppu_register_function_at(block.first, block.second, nullptr);
			}

			if (g_cfg.core.ppu_debug && func.size && func.toc != umax)
			{
				s_ppu_toc->emplace(func.addr, func.toc);
				ppu_ref(func.addr) = reinterpret_cast<uptr>(&ppu_check_toc);
			}
		}

		return false;
	}

	// Link table
	static const std::unordered_map<std::string, u64> s_link_table = []()
	{
		std::unordered_map<std::string, u64> link_table
		{
			{ "__trap", reinterpret_cast<u64>(&ppu_trap) },
			{ "__error", reinterpret_cast<u64>(&ppu_error) },
			{ "__check", reinterpret_cast<u64>(&ppu_check) },
			{ "__trace", reinterpret_cast<u64>(&ppu_trace) },
			{ "__syscall", reinterpret_cast<u64>(ppu_execute_syscall) },
			{ "__get_tb", reinterpret_cast<u64>(get_timebased_time) },
			{ "__lwarx", reinterpret_cast<u64>(ppu_lwarx) },
			{ "__ldarx", reinterpret_cast<u64>(ppu_ldarx) },
			{ "__stwcx", reinterpret_cast<u64>(ppu_stwcx) },
			{ "__stdcx", reinterpret_cast<u64>(ppu_stdcx) },
			{ "__vexptefp", reinterpret_cast<u64>(sse_exp2_ps) },
			{ "__vlogefp", reinterpret_cast<u64>(sse_log2_ps) },
			{ "__lvsl", reinterpret_cast<u64>(sse_altivec_lvsl) },
			{ "__lvsr", reinterpret_cast<u64>(sse_altivec_lvsr) },
			{ "__lvlx", s_use_ssse3 ? reinterpret_cast<u64>(sse_cellbe_lvlx) : reinterpret_cast<u64>(sse_cellbe_lvlx_v0) },
			{ "__lvrx", s_use_ssse3 ? reinterpret_cast<u64>(sse_cellbe_lvrx) : reinterpret_cast<u64>(sse_cellbe_lvrx_v0) },
			{ "__stvlx", s_use_ssse3 ? reinterpret_cast<u64>(sse_cellbe_stvlx) : reinterpret_cast<u64>(sse_cellbe_stvlx_v0) },
			{ "__stvrx", s_use_ssse3 ? reinterpret_cast<u64>(sse_cellbe_stvrx) : reinterpret_cast<u64>(sse_cellbe_stvrx_v0) },
			{ "__dcbz", reinterpret_cast<u64>(+[](u32 addr){ alignas(64) static constexpr u8 z[128]{}; do_cell_atomic_128_store(addr, z); }) },
			{ "__resupdate", reinterpret_cast<u64>(vm::reservation_update) },
			{ "__resinterp", reinterpret_cast<u64>(ppu_reservation_fallback) },
		};

		for (u64 index = 0; index < 1024; index++)
		{
			if (ppu_get_syscall(index))
			{
				link_table.emplace(fmt::format("%s", ppu_syscall_code(index)), reinterpret_cast<u64>(ppu_execute_syscall));
				link_table.emplace(fmt::format("syscall_%u", index), reinterpret_cast<u64>(ppu_execute_syscall));
			}
		}

		return link_table;
	}();

	// Get cache path for this executable
	std::string cache_path;

	if (info.name.empty())
	{
		cache_path = info.cache;
	}
	else
	{
		// New PPU cache location
		cache_path = fs::get_cache_dir() + "cache/";

		const std::string dev_flash = vfs::get("/dev_flash/");

		if (!info.path.starts_with(dev_flash) && !Emu.GetTitleID().empty() && Emu.GetCat() != "1P")
		{
			// Add prefix for anything except dev_flash files, standalone elfs or PS1 classics
			cache_path += Emu.GetTitleID();
			cache_path += '/';
		}

		// Add PPU hash and filename
		fmt::append(cache_path, "ppu-%s-%s/", fmt::base57(info.sha1), info.path.substr(info.path.find_last_of('/') + 1));

		if (!fs::create_path(cache_path))
		{
			fmt::throw_exception("Failed to create cache directory: %s (%s)", cache_path, fs::g_tls_error);
		}
	}

#ifdef LLVM_AVAILABLE
	// Initialize progress dialog
	g_progr = "Compiling PPU modules...";

	struct jit_core_allocator
	{
		const s32 thread_count = g_cfg.core.llvm_threads ? std::min<s32>(g_cfg.core.llvm_threads, limit()) : limit();

		// Initialize global semaphore with the max number of threads
		::semaphore<0x7fffffff> sem{std::max<s32>(thread_count, 1)};

		static s32 limit()
		{
			return static_cast<s32>(utils::get_thread_count());
		}
	};

	// Permanently loaded compiled PPU modules (name -> data)
	jit_module& jit_mod = g_fxo->get<jit_module_manager>().get(cache_path + info.name);

	// Compiler instance (deferred initialization)
	std::shared_ptr<jit_compiler>& jit = jit_mod.pjit;

	// Split module into fragments <= 1 MiB
	usz fpos = 0;

	// Difference between function name and current location
	const u32 reloc = info.relocs.empty() ? 0 : info.segs.at(0).addr;

	// Info sent to threads
	std::vector<std::pair<std::string, ppu_module>> workload;

	// Info to load to main JIT instance (true - compiled)
	std::vector<std::pair<std::string, bool>> link_workload;

	// Sync variable to acquire workloads
	atomic_t<u32> work_cv = 0;

	bool compiled_new = false;

	while (!jit_mod.init && fpos < info.funcs.size())
	{
		// Initialize compiler instance
		if (!jit && get_current_cpu_thread())
		{
			jit = std::make_shared<jit_compiler>(s_link_table, g_cfg.core.llvm_cpu);
		}

		// Copy module information (TODO: optimize)
		ppu_module part;
		part.copy_part(info);
		part.funcs.reserve(16000);

		// Overall block size in bytes
		usz bsize = 0;
		usz bcount = 0;

		while (fpos < info.funcs.size())
		{
			auto& func = info.funcs[fpos];

			if (!func.size)
			{
				fpos++;
				continue;
			}

			if (bsize + func.size > 100 * 1024 && bsize)
			{
				if (bcount >= 1000)
				{
					break;
				}
			}

			// Copy block or function entry
			ppu_function& entry = part.funcs.emplace_back(func);

			// Fixup some information
			entry.name = fmt::format("__0x%x", entry.addr - reloc);

			if (entry.blocks.empty())
			{
				entry.blocks.emplace(func.addr, func.size);
			}

			bsize += func.size;

			fpos++;
			bcount++;
		}

		// Compute module hash to generate (hopefully) unique object name
		std::string obj_name;
		{
			sha1_context ctx;
			u8 output[20];
			sha1_starts(&ctx);

			int has_dcbz = !!g_cfg.core.accurate_cache_line_stores;

			for (const auto& func : part.funcs)
			{
				if (func.size == 0)
				{
					continue;
				}

				const be_t<u32> addr = func.addr - reloc;
				const be_t<u32> size = func.size;
				sha1_update(&ctx, reinterpret_cast<const u8*>(&addr), sizeof(addr));
				sha1_update(&ctx, reinterpret_cast<const u8*>(&size), sizeof(size));

				for (const auto& block : func.blocks)
				{
					if (block.second == 0 || reloc)
					{
						continue;
					}

					// Find relevant relocations
					auto low = std::lower_bound(part.relocs.cbegin(), part.relocs.cend(), block.first);
					auto high = std::lower_bound(low, part.relocs.cend(), block.first + block.second);
					auto addr = block.first;

					for (; low != high; ++low)
					{
						// Aligned relocation address
						const u32 roff = low->addr & ~3;

						if (roff > addr)
						{
							// Hash from addr to the beginning of the relocation
							sha1_update(&ctx, vm::_ptr<const u8>(addr), roff - addr);
						}

						// Hash relocation type instead
						const be_t<u32> type = low->type;
						sha1_update(&ctx, reinterpret_cast<const u8*>(&type), sizeof(type));

						// Set the next addr
						addr = roff + 4;
					}

					if (has_dcbz == 1)
					{
						for (u32 i = addr, end = block.second + block.first - 1; i <= end; i += 4)
						{
							if (g_ppu_itype.decode(vm::read32(i)) == ppu_itype::DCBZ)
							{
								has_dcbz = 2;
								break;
							}
						}
					}

					// Hash from addr to the end of the block
					sha1_update(&ctx, vm::_ptr<const u8>(addr), block.second - (addr - block.first));
				}

				if (reloc)
				{
					continue;
				}

				if (has_dcbz == 1)
				{
					for (u32 i = func.addr, end = func.addr + func.size - 1; i <= end; i += 4)
					{
						if (g_ppu_itype.decode(vm::read32(i)) == ppu_itype::DCBZ)
						{
							has_dcbz = 2;
							break;
						}
					}
				}

				sha1_update(&ctx, vm::_ptr<const u8>(func.addr), func.size);
			}

			if (false)
			{
				const be_t<u64> forced_upd = 3;
				sha1_update(&ctx, reinterpret_cast<const u8*>(&forced_upd), sizeof(forced_upd));
			}

			sha1_finish(&ctx, output);

			// Settings: should be populated by settings which affect codegen (TODO)
			enum class ppu_settings : u32
			{
				non_win32,
				accurate_fma,
				accurate_ppu_vector_nan,
				java_mode_handling,
				accurate_cache_line_stores,
				reservations_128_byte,
				greedy_mode,

				__bitset_enum_max
			};

			be_t<bs_t<ppu_settings>> settings{};

#ifndef _WIN32
			settings += ppu_settings::non_win32;
#endif
			if (g_cfg.core.llvm_accurate_dfma)
				settings += ppu_settings::accurate_fma;
			if (g_cfg.core.llvm_ppu_accurate_vector_nan)
				settings += ppu_settings::accurate_ppu_vector_nan;
			if (g_cfg.core.llvm_ppu_jm_handling)
				settings += ppu_settings::java_mode_handling;
			if (has_dcbz == 2)
				settings += ppu_settings::accurate_cache_line_stores;
			if (g_cfg.core.ppu_128_reservations_loop_max_length)
				settings += ppu_settings::reservations_128_byte;
			if (g_cfg.core.ppu_llvm_greedy_mode)
				settings += ppu_settings::greedy_mode;

			// Write version, hash, CPU, settings
			fmt::append(obj_name, "v4-kusa-%s-%s-%s.obj", fmt::base57(output, 16), fmt::base57(settings), jit_compiler::cpu(g_cfg.core.llvm_cpu));
		}

		if (Emu.IsStopped())
		{
			break;
		}

		if (!check_only)
		{
			link_workload.emplace_back(obj_name, false);
		}

		// Check object file
		if (jit_compiler::check(cache_path + obj_name))
		{
			if (!jit && !check_only)
			{
				ppu_log.success("LLVM: Module exists: %s", obj_name);
			}

			continue;
		}

		if (check_only)
		{
			return true;
		}

		// Remember, used in ppu_initialize(void)
		compiled_new = true;

		// Adjust information (is_compiled)
		link_workload.back().second = true;

		// Fill workload list for compilation
		workload.emplace_back(std::move(obj_name), std::move(part));

		// Update progress dialog
		g_progr_ptotal++;
	}

	if (check_only)
	{
		return false;
	}

	// Create worker threads for compilation (TODO: how many threads)
	{
		u32 thread_count = Emu.GetMaxThreads();

		if (workload.size() < thread_count)
		{
			thread_count = ::size32(workload);
		}

		struct thread_index_allocator
		{
			atomic_t<u64> index = 0;
		};

		// Prevent watchdog thread from terminating
		g_watchdog_hold_ctr++;

		named_thread_group threads(fmt::format("PPUW.%u.", ++g_fxo->get<thread_index_allocator>().index), thread_count, [&]()
		{
			// Set low priority
			thread_ctrl::scoped_priority low_prio(-1);

			for (u32 i = work_cv++; i < workload.size(); i = work_cv++, g_progr_pdone++)
			{
				if (Emu.IsStopped())
				{
					continue;
				}

				// Keep allocating workload
				const auto& [obj_name, part] = std::as_const(workload)[i];

				// Allocate "core"
				std::lock_guard jlock(g_fxo->get<jit_core_allocator>().sem);

				ppu_log.warning("LLVM: Compiling module %s%s", cache_path, obj_name);

				// Use another JIT instance
				jit_compiler jit2({}, g_cfg.core.llvm_cpu, 0x1);
				ppu_initialize2(jit2, part, cache_path, obj_name);

				ppu_log.success("LLVM: Compiled module %s", obj_name);
			}
		});

		threads.join();

		g_watchdog_hold_ctr--;

		if (Emu.IsStopped() || !get_current_cpu_thread())
		{
			return compiled_new;
		}

		for (auto [obj_name, is_compiled] : link_workload)
		{
			if (Emu.IsStopped())
			{
				break;
			}

			jit->add(cache_path + obj_name);

			if (!is_compiled)
			{
				ppu_log.success("LLVM: Loaded module %s", obj_name);
			}
		}
	}

	if (Emu.IsStopped() || !get_current_cpu_thread())
	{
		return compiled_new;
	}

	// Jit can be null if the loop doesn't ever enter.
	if (jit && !jit_mod.init)
	{
		jit->fin();

		// Get and install function addresses
		for (const auto& func : info.funcs)
		{
			if (!func.size) continue;

			const auto name = fmt::format("__0x%x", func.addr - reloc);
			const u64 addr = ensure(jit->get(name));
			jit_mod.funcs.emplace_back(reinterpret_cast<ppu_function_t>(addr));
			ppu_ref(func.addr) = (addr & 0x7fff'ffff'ffffu) | (ppu_ref(func.addr) & ~0x7fff'ffff'ffffu);

			if (g_cfg.core.ppu_debug)
				ppu_log.notice("Installing function %s at 0x%x: %p (reloc = 0x%x)", name, func.addr, ppu_ref(func.addr), reloc);
		}

		jit_mod.init = true;
	}
	else
	{
		usz index = 0;

		// Locate existing functions
		for (const auto& func : info.funcs)
		{
			if (!func.size) continue;

			const u64 addr = ensure(reinterpret_cast<uptr>(jit_mod.funcs[index++]));
			ppu_ref(func.addr) = (addr & 0x7fff'ffff'ffffu) | (ppu_ref(func.addr) & ~0x7fff'ffff'ffffu);

			if (g_cfg.core.ppu_debug)
				ppu_log.notice("Reinstalling function at 0x%x: %p (reloc=0x%x)", func.addr, ppu_ref(func.addr), reloc);
		}

		index = 0;
	}

	return compiled_new;
#else
	fmt::throw_exception("LLVM is not available in this build.");
#endif
}

static void ppu_initialize2(jit_compiler& jit, const ppu_module& module_part, const std::string& cache_path, const std::string& obj_name)
{
#ifdef LLVM_AVAILABLE
	using namespace llvm;

	// Create LLVM module
	std::unique_ptr<Module> _module = std::make_unique<Module>(obj_name, jit.get_context());

	// Initialize target
	_module->setTargetTriple(Triple::normalize(sys::getProcessTriple()));
	_module->setDataLayout(jit.get_engine().getTargetMachine()->createDataLayout());

	// Initialize translator
	PPUTranslator translator(jit.get_context(), _module.get(), module_part, jit.get_engine());

	// Define some types
	const auto _func = FunctionType::get(translator.get_type<void>(), {
		translator.get_type<u8*>(), // Exec base
		translator.GetContextType()->getPointerTo(), // PPU context
		translator.get_type<u64>(), // Segment address (for PRX)
		translator.get_type<u8*>(), // Memory base
		translator.get_type<u64>(), // r0
		translator.get_type<u64>(), // r1
		translator.get_type<u64>(), // r2
		}, false);

	// Initialize function list
	for (const auto& func : module_part.funcs)
	{
		if (func.size)
		{
			const auto f = cast<Function>(_module->getOrInsertFunction(func.name, _func).getCallee());
			f->setCallingConv(CallingConv::GHC);
			f->addAttribute(2, Attribute::NoAlias);
			f->addFnAttr(Attribute::NoUnwind);
		}
	}

	{
		legacy::FunctionPassManager pm(_module.get());

		// Basic optimizations
		//pm.add(createCFGSimplificationPass());
		//pm.add(createPromoteMemoryToRegisterPass());
		pm.add(createEarlyCSEPass());
		//pm.add(createTailCallEliminationPass());
		//pm.add(createInstructionCombiningPass());
		//pm.add(createBasicAAWrapperPass());
		//pm.add(new MemoryDependenceAnalysis());
		//pm.add(createLICMPass());
		//pm.add(createLoopInstSimplifyPass());
		//pm.add(createNewGVNPass());
		pm.add(createDeadStoreEliminationPass());
		//pm.add(createSCCPPass());
		//pm.add(createReassociatePass());
		//pm.add(createInstructionCombiningPass());
		//pm.add(createInstructionSimplifierPass());
		//pm.add(createAggressiveDCEPass());
		//pm.add(createCFGSimplificationPass());
		//pm.add(createLintPass()); // Check

		// Translate functions
		for (usz fi = 0, fmax = module_part.funcs.size(); fi < fmax; fi++)
		{
			if (Emu.IsStopped())
			{
				ppu_log.success("LLVM: Translation cancelled");
				return;
			}

			if (module_part.funcs[fi].size)
			{
				// Translate
				if (const auto func = translator.Translate(module_part.funcs[fi]))
				{
					// Run optimization passes
					pm.run(*func);
				}
				else
				{
					Emu.Pause();
					return;
				}
			}
		}

		//legacy::PassManager mpm;

		// Remove unused functions, structs, global variables, etc
		//mpm.add(createStripDeadPrototypesPass());
		//mpm.add(createFunctionInliningPass());
		//mpm.add(createDeadInstEliminationPass());
		//mpm.run(*module);

		std::string result;
		raw_string_ostream out(result);

		if (g_cfg.core.llvm_logs)
		{
			out << *_module; // print IR
			fs::file(cache_path + obj_name + ".log", fs::rewrite).write(out.str());
			result.clear();
		}

		if (verifyModule(*_module, &out))
		{
			out.flush();
			ppu_log.error("LLVM: Verification failed for %s:\n%s", obj_name, result);
			Emu.CallAfter([]{ Emu.Stop(); });
			return;
		}

		ppu_log.notice("LLVM: %zu functions generated", _module->getFunctionList().size());
	}

	// Load or compile module
	jit.add(std::move(_module), cache_path);
#endif // LLVM_AVAILABLE
}
