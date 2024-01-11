#include "stdafx.h"
#include "Utilities/JIT.h"
#include "Utilities/StrUtil.h"
#include "util/serialization.hpp"
#include "Crypto/sha1.h"
#include "Crypto/unself.h"
#include "Loader/ELF.h"
#include "Loader/mself.hpp"
#include "Emu/perf_meter.hpp"
#include "Emu/Memory/vm_reservation.h"
#include "Emu/Memory/vm_locking.h"
#include "Emu/RSX/Core/RSXReservationLock.hpp"
#include "Emu/VFS.h"
#include "Emu/vfs_config.h"
#include "Emu/system_progress.hpp"
#include "Emu/system_utils.hpp"
#include "PPUThread.h"
#include "PPUInterpreter.h"
#include "PPUAnalyser.h"
#include "PPUModule.h"
#include "PPUDisAsm.h"
#include "SPURecompiler.h"
#include "timers.hpp"
#include "lv2/sys_sync.h"
#include "lv2/sys_prx.h"
#include "lv2/sys_overlay.h"
#include "lv2/sys_process.h"
#include "lv2/sys_spu.h"

#ifdef LLVM_AVAILABLE
#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#endif
#include "llvm/Support/FormattedStream.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Object/ObjectFile.h"
#if LLVM_VERSION_MAJOR < 17
#include "llvm/ADT/Triple.h"
#endif
#include "llvm/IR/Verifier.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Scalar.h"
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
#include <span>
#include <optional>

#include "util/asm.hpp"
#include "util/vm.hpp"
#include "util/v128.hpp"
#include "util/simd.hpp"
#include "util/sysinfo.hpp"

#ifdef __APPLE__
#include <libkern/OSCacheControl.h>
#endif

extern atomic_t<u64> g_watchdog_hold_ctr;

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

template <>
void fmt_class_string<ppu_thread_status>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](ppu_thread_status s)
	{
		switch (s)
		{
		case PPU_THREAD_STATUS_IDLE: return "IDLE";
		case PPU_THREAD_STATUS_RUNNABLE: return "RUN";
		case PPU_THREAD_STATUS_ONPROC: return "ONPROC";
		case PPU_THREAD_STATUS_SLEEP: return "SLEEP";
		case PPU_THREAD_STATUS_STOP: return "STOP";
		case PPU_THREAD_STATUS_ZOMBIE: return "Zombie";
		case PPU_THREAD_STATUS_DELETED: return "Deleted";
		case PPU_THREAD_STATUS_UNKNOWN: break;
		}

		return unknown;
	});
}

template <>
void fmt_class_string<typename ppu_thread::call_history_t>::format(std::string& out, u64 arg)
{
	const auto& history = get_object(arg);

	PPUDisAsm dis_asm(cpu_disasm_mode::normal, vm::g_sudo_addr);

	for (u64 count = 0, idx = history.index - 1; idx != umax && count < history.data.size(); count++, idx--)
	{
		const u32 pc = history.data[idx % history.data.size()];
		dis_asm.disasm(pc);
		fmt::append(out, "\n(%u) 0x%08x: %s", count, pc, dis_asm.last_opcode);
	}
}

template <>
void fmt_class_string<typename ppu_thread::syscall_history_t>::format(std::string& out, u64 arg)
{
	const auto& history = get_object(arg);

	for (u64 count = 0, idx = history.index - 1; idx != umax && count < history.data.size(); count++, idx--)
	{
		const auto& entry = history.data[idx % history.data.size()];
		fmt::append(out, "\n(%u) 0x%08x: %s, 0x%x, r3=0x%x, r4=0x%x, r5=0x%x, r6=0x%x", count, entry.cia, entry.func_name, entry.error, entry.args[0], entry.args[1], entry.args[2], entry.args[3]);
	}
}

extern const ppu_decoder<ppu_itype> g_ppu_itype{};
extern const ppu_decoder<ppu_iname> g_ppu_iname{};

template <>
bool serialize<ppu_thread::cr_bits>(utils::serial& ar, typename ppu_thread::cr_bits& o)
{
	if (ar.is_writing())
	{
		ar(o.pack());
	}
	else
	{
		o.unpack(ar);
	}

	return true;
}

extern void ppu_initialize();
extern void ppu_finalize(const ppu_module& info);
extern bool ppu_initialize(const ppu_module& info, bool check_only = false, u64 file_size = 0);
static void ppu_initialize2(class jit_compiler& jit, const ppu_module& module_part, const std::string& cache_path, const std::string& obj_name);
extern bool ppu_load_exec(const ppu_exec_object&, bool virtual_load, const std::string&, utils::serial* = nullptr);
extern std::pair<std::shared_ptr<lv2_overlay>, CellError> ppu_load_overlay(const ppu_exec_object&, bool virtual_load, const std::string& path, s64 file_offset, utils::serial* = nullptr);
extern void ppu_unload_prx(const lv2_prx&);
extern std::shared_ptr<lv2_prx> ppu_load_prx(const ppu_prx_object&, bool virtual_load, const std::string&, s64 file_offset, utils::serial* = nullptr);
extern void ppu_execute_syscall(ppu_thread& ppu, u64 code);
static void ppu_break(ppu_thread&, ppu_opcode_t, be_t<u32>*, ppu_intrp_func*);

extern void do_cell_atomic_128_store(u32 addr, const void* to_write);

const auto ppu_gateway = build_function_asm<void(*)(ppu_thread*)>("ppu_gateway", [](native_asm& c, auto& args)
{
	// Gateway for PPU, converts from native to GHC calling convention, also saves RSP value for escape
	using namespace asmjit;

#if defined(ARCH_X64)
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
	c.shl(x86::rax, 16);
	c.shr(x86::rax, 16);
	c.shr(x86::rdx, 48);
	c.shl(x86::edx, 13);
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
#else
	// See https://github.com/ghc/ghc/blob/master/rts/include/stg/MachRegs.h
	// for GHC calling convention definitions on Aarch64
	// and https://developer.arm.com/documentation/den0024/a/The-ABI-for-ARM-64-bit-Architecture/Register-use-in-the-AArch64-Procedure-Call-Standard/Parameters-in-general-purpose-registers
	// for AArch64 calling convention

	// Save sp for native longjmp emulation
	Label native_sp_offset = c.newLabel();
	c.ldr(a64::x10, arm::Mem(native_sp_offset));
	// sp not allowed to be used in load/stores directly
	c.mov(a64::x15, a64::sp);
	c.str(a64::x15, arm::Mem(args[0], a64::x10));

	// Push callee saved registers to the stack
	// We need to save x18-x30 = 13 x 8B each + 8 bytes for 16B alignment = 112B
	c.sub(a64::sp, a64::sp, Imm(112));
	c.stp(a64::x18, a64::x19, arm::Mem(a64::sp));
	c.stp(a64::x20, a64::x21, arm::Mem(a64::sp, 16));
	c.stp(a64::x22, a64::x23, arm::Mem(a64::sp, 32));
	c.stp(a64::x24, a64::x25, arm::Mem(a64::sp, 48));
	c.stp(a64::x26, a64::x27, arm::Mem(a64::sp, 64));
	c.stp(a64::x28, a64::x29, arm::Mem(a64::sp, 80));
	c.str(a64::x30, arm::Mem(a64::sp, 96));

	// Load REG_Base - use absolute jump target to bypass rel jmp range limits
	Label exec_addr = c.newLabel();
	c.ldr(a64::x19, arm::Mem(exec_addr));
	c.ldr(a64::x19, arm::Mem(a64::x19));
	// Load PPUThread struct base -> REG_Sp
	const arm::GpX ppu_t_base = a64::x20;
	c.mov(ppu_t_base, args[0]);
	// Load PC
	const arm::GpX pc = a64::x15;
	Label cia_offset = c.newLabel();
	const arm::GpX cia_addr_reg = a64::x11;
	// Load offset value
	c.ldr(cia_addr_reg, arm::Mem(cia_offset));
	// Load cia
	c.ldr(a64::w15, arm::Mem(ppu_t_base, cia_addr_reg));
	// Multiply by 2 to index into ptr table
	const arm::GpX index_shift = a64::x12;
	c.mov(index_shift, Imm(2));
	c.mul(pc, pc, index_shift);

	// Load call target
	const arm::GpX call_target = a64::x13;
	c.ldr(call_target, arm::Mem(a64::x19, pc));
	// Compute REG_Hp
	const arm::GpX reg_hp = a64::x21;
	c.mov(reg_hp, call_target);
	c.lsr(reg_hp, reg_hp, 48);
	c.lsl(a64::w21, a64::w21, 13);

	// Zero top 16 bits of call target
	c.lsl(call_target, call_target, Imm(16));
	c.lsr(call_target, call_target, Imm(16));

	// Load registers
	Label base_addr = c.newLabel();
	c.ldr(a64::x22, arm::Mem(base_addr));
	c.ldr(a64::x22, arm::Mem(a64::x22));

	Label gpr_addr_offset = c.newLabel();
	const arm::GpX gpr_addr_reg = a64::x9;
	c.ldr(gpr_addr_reg, arm::Mem(gpr_addr_offset));
	c.add(gpr_addr_reg, gpr_addr_reg, ppu_t_base);
	c.ldr(a64::x23, arm::Mem(gpr_addr_reg));
	c.ldr(a64::x24, arm::Mem(gpr_addr_reg, 8));
	c.ldr(a64::x25, arm::Mem(gpr_addr_reg, 16));

	// Execute LLE call
	c.blr(call_target);

	// Restore registers from the stack
	c.ldp(a64::x18, a64::x19, arm::Mem(a64::sp));
	c.ldp(a64::x20, a64::x21, arm::Mem(a64::sp, 16));
	c.ldp(a64::x22, a64::x23, arm::Mem(a64::sp, 32));
	c.ldp(a64::x24, a64::x25, arm::Mem(a64::sp, 48));
	c.ldp(a64::x26, a64::x27, arm::Mem(a64::sp, 64));
	c.ldp(a64::x28, a64::x29, arm::Mem(a64::sp, 80));
	c.ldr(a64::x30, arm::Mem(a64::sp, 96));
	// Restore stack ptr
	c.add(a64::sp, a64::sp, Imm(112));
	// Return
	c.ret(a64::x30);

	c.bind(exec_addr);
	c.embedUInt64(reinterpret_cast<u64>(&vm::g_exec_addr));
	c.bind(base_addr);
	c.embedUInt64(reinterpret_cast<u64>(&vm::g_base_addr));
	c.bind(cia_offset);
	c.embedUInt64(static_cast<u64>(::offset32(&ppu_thread::cia)));
	c.bind(gpr_addr_offset);
	c.embedUInt64(static_cast<u64>(::offset32(&ppu_thread::gpr)));
	c.bind(native_sp_offset);
	c.embedUInt64(static_cast<u64>(::offset32(&ppu_thread::saved_native_sp)));
#endif
});

const extern auto ppu_escape = build_function_asm<void(*)(ppu_thread*)>("ppu_escape", [](native_asm& c, auto& args)
{
	using namespace asmjit;

#if defined(ARCH_X64)
	// Restore native stack pointer (longjmp emulation)
	c.mov(x86::rsp, x86::qword_ptr(args[0], ::offset32(&ppu_thread::saved_native_sp)));

	// Return to the return location
	c.sub(x86::rsp, 8);
	c.ret();
#endif
});

void ppu_recompiler_fallback(ppu_thread& ppu);

#if defined(ARCH_X64)
const auto ppu_recompiler_fallback_ghc = build_function_asm<void(*)(ppu_thread& ppu)>("", [](native_asm& c, auto& args)
{
	using namespace asmjit;

	c.mov(args[0], x86::rbp);
	c.jmp(ppu_recompiler_fallback);
});
#elif defined(ARCH_ARM64)
const auto ppu_recompiler_fallback_ghc = &ppu_recompiler_fallback;
#endif

// Get pointer to executable cache
static ppu_intrp_func_t& ppu_ref(u32 addr)
{
	return *reinterpret_cast<ppu_intrp_func_t*>(vm::g_exec_addr + u64{addr} * 2);
}

// Get interpreter cache value
static ppu_intrp_func_t ppu_cache(u32 addr)
{
	if (g_cfg.core.ppu_decoder != ppu_decoder_type::_static)
	{
		fmt::throw_exception("Invalid PPU decoder");
	}

	return g_fxo->get<ppu_interpreter_rt>().decode(vm::read32(addr));
}

static ppu_intrp_func ppu_ret = {[](ppu_thread& ppu, ppu_opcode_t, be_t<u32>* this_op, ppu_intrp_func*)
{
	// Fix PC and return (step execution)
	ppu.cia = vm::get_addr(this_op);
	return;
}};

static void ppu_fallback(ppu_thread& ppu, ppu_opcode_t op, be_t<u32>* this_op, ppu_intrp_func* next_fn)
{
	const auto _pc = vm::get_addr(this_op);
	const auto _fn = ppu_cache(_pc);
	ppu_ref(_pc) = _fn;
	return _fn(ppu, op, this_op, next_fn);
}

// TODO: Make this a dispatch call
void ppu_recompiler_fallback(ppu_thread& ppu)
{
	perf_meter<"PPUFALL1"_u64> perf0;

	if (g_cfg.core.ppu_debug)
	{
		ppu_log.error("Unregistered PPU Function (LR=0x%x)", ppu.lr);
	}

	const auto& table = g_fxo->get<ppu_interpreter_rt>();

	while (true)
	{
		if (uptr func = uptr(ppu_ref(ppu.cia)); (func << 16 >> 16) != reinterpret_cast<uptr>(ppu_recompiler_fallback_ghc))
		{
			// We found a recompiler function at cia, return
			break;
		}

		// Run one instruction in interpreter (TODO)
		const u32 op = vm::read32(ppu.cia);
		table.decode(op)(ppu, {op}, vm::_ptr<u32>(ppu.cia), &ppu_ret);

		if (ppu.test_stopped())
		{
			break;
		}
	}
}

void ppu_reservation_fallback(ppu_thread& ppu)
{
	perf_meter<"PPUFALL2"_u64> perf0;

	const auto& table = g_fxo->get<ppu_interpreter_rt>();

	while (true)
	{
		// Run one instruction in interpreter (TODO)
		const u32 op = vm::read32(ppu.cia);
		table.decode(op)(ppu, {op}, vm::_ptr<u32>(ppu.cia), &ppu_ret);

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

u32 ppu_read_mmio_aware_u32(u8* vm_base, u32 eal)
{
	if (eal >= RAW_SPU_BASE_ADDR)
	{
		// RawSPU MMIO
		auto thread = idm::get<named_thread<spu_thread>>(spu_thread::find_raw_spu((eal - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET));

		if (!thread)
		{
			// Access Violation
		}
		else if ((eal - RAW_SPU_BASE_ADDR) % RAW_SPU_OFFSET + sizeof(u32) - 1 < SPU_LS_SIZE) // LS access
		{
		}
		else if (u32 value{}; thread->read_reg(eal, value))
		{
			return std::bit_cast<be_t<u32>>(value);
		}
		else
		{
			fmt::throw_exception("Invalid RawSPU MMIO offset (addr=0x%x)", eal);
		}
	}

	// Value is assumed to be swapped
	return read_from_ptr<u32>(vm_base + eal);
}

void ppu_write_mmio_aware_u32(u8* vm_base, u32 eal, u32 value)
{
	if (eal >= RAW_SPU_BASE_ADDR)
	{
		// RawSPU MMIO
		auto thread = idm::get<named_thread<spu_thread>>(spu_thread::find_raw_spu((eal - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET));

		if (!thread)
		{
			// Access Violation
		}
		else if ((eal - RAW_SPU_BASE_ADDR) % RAW_SPU_OFFSET + sizeof(u32) - 1 < SPU_LS_SIZE) // LS access
		{
		}
		else if (thread->write_reg(eal, std::bit_cast<be_t<u32>>(value)))
		{
			return;
		}
		else
		{
			fmt::throw_exception("Invalid RawSPU MMIO offset (addr=0x%x)", eal);
		}
	}

	// Value is assumed swapped
	write_to_ptr<u32>(vm_base + eal, value);
}

extern bool ppu_test_address_may_be_mmio(std::span<const be_t<u32>> insts)
{
	std::set<u32> reg_offsets;
	bool found_raw_spu_base = false;
	bool found_spu_area_offset_element = false;

	for (u32 inst : insts)
	{
		// Common around MMIO (orders IO)
		if (inst == ppu_instructions::EIEIO())
		{
			return true;
		}

		const u32 op_imm16 = (inst & 0xfc00ffff);

		// RawSPU MMIO base
		// 0xe00000000 is a common constant so try to find an ORIS 0x10 or ADDIS 0x10 nearby (for multiplying SPU ID by it)
		if (op_imm16 == ppu_instructions::ADDIS({}, {}, -0x2000) || op_imm16 == ppu_instructions::ORIS({}, {}, 0xe000)  || op_imm16 == ppu_instructions::XORIS({}, {}, 0xe000))
		{
			found_raw_spu_base = true;

			if (found_spu_area_offset_element)
			{
				// Found both
				return true;
			}
		}
		else if (op_imm16 == ppu_instructions::ORIS({}, {}, 0x10) || op_imm16 == ppu_instructions::ADDIS({}, {}, 0x10))
		{
			found_spu_area_offset_element = true;

			if (found_raw_spu_base)
			{
				// Found both
				return true;
			}
		}
		// RawSPU MMIO base + problem state offset
		else if (op_imm16 == ppu_instructions::ADDIS({}, {}, -0x1ffc))
		{
			return true;
		}
		else if (op_imm16 == ppu_instructions::ORIS({}, {}, 0xe004))
		{
			return true;
		}
		else if (op_imm16 == ppu_instructions::XORIS({}, {}, 0xe004))
		{
			return true;
		}
		// RawSPU MMIO base + problem state offset + 64k of SNR1 offset
		else if (op_imm16 == ppu_instructions::ADDIS({}, {}, -0x1ffb))
		{
			return true;
		}
		else if (op_imm16 == ppu_instructions::ORIS({}, {}, 0xe005))
		{
			return true;
		}
		else if (op_imm16 == ppu_instructions::XORIS({}, {}, 0xe005))
		{
			return true;
		}
		// RawSPU MMIO base + problem state offset + 264k of SNR2 offset (STW allows 32K+- offset so in order to access SNR2 it needs to first add another 64k)
		// SNR2 is the only register currently implemented that has its 0x80000 bit is set so its the only one its hardcoded access is done this way
		else if (op_imm16 == ppu_instructions::ADDIS({}, {}, -0x1ffa))
		{
			return true;
		}
		else if (op_imm16 == ppu_instructions::ORIS({}, {}, 0xe006))
		{
			return true;
		}
		else if (op_imm16 == ppu_instructions::XORIS({}, {}, 0xe006))
		{
			return true;
		}
		// Try to detect a function that receives RawSPU problem state base pointer as an argument
		else if ((op_imm16 & ~0xffff) == ppu_instructions::LWZ({}, {}, 0) ||
			(op_imm16 & ~0xffff) == ppu_instructions::STW({}, {}, 0) ||
			(op_imm16 & ~0xffff) == ppu_instructions::ADDI({}, {}, 0))
		{
			const bool is_load = (op_imm16 & ~0xffff) == ppu_instructions::LWZ({}, {}, 0);
			const bool is_store = (op_imm16 & ~0xffff) == ppu_instructions::STW({}, {}, 0);
			const bool is_neither = !is_store && !is_load;
			const bool is_snr = (is_store || is_neither) && ((op_imm16 & 0xffff) == (SPU_RdSigNotify2_offs & 0xffff) || (op_imm16 & 0xffff) == (SPU_RdSigNotify1_offs & 0xffff));

			if (is_snr || spu_thread::test_is_problem_state_register_offset(op_imm16 & 0xffff, is_load || is_neither, is_store || is_neither))
			{
				reg_offsets.insert(op_imm16 & 0xffff);

				if (reg_offsets.size() >= 2)
				{
					// Assume high MMIO likelyhood if more than one offset appears in nearby code
					// Such as common IN_MBOX + OUT_MBOX
					return true;
				}
			}
		}
	}

	return false;
}

struct ppu_toc_manager
{
	std::unordered_map<u32, u32> toc_map;

	shared_mutex mutex;
};

static void ppu_check_toc(ppu_thread& ppu, ppu_opcode_t op, be_t<u32>* this_op, ppu_intrp_func* next_fn)
{
	ppu.cia = vm::get_addr(this_op);

	{
		auto& toc_manager = g_fxo->get<ppu_toc_manager>();

		reader_lock lock(toc_manager.mutex);

		auto& ppu_toc = toc_manager.toc_map;

		const auto found = ppu_toc.find(ppu.cia);

		if (found != ppu_toc.end())
		{
			const u32 toc = atomic_storage<u32>::load(found->second);

			// Compare TOC with expected value
			if (toc != umax && ppu.gpr[2] != toc)
			{
				ppu_log.error("Unexpected TOC (0x%x, expected 0x%x)", ppu.gpr[2], toc);
				atomic_storage<u32>::exchange(found->second, u32{umax});
			}
		}
	}

	// Fallback to the interpreter function
	return ppu_cache(ppu.cia)(ppu, op, this_op, next_fn);
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
	ensure(vm::page_protect(addr, size, 0, vm::page_executable));

	if (g_cfg.core.ppu_debug)
	{
		utils::memory_commit(vm::g_stat_addr + addr, size);
	}

	const u64 seg_base = addr;

	while (size)
	{
		if (g_cfg.core.ppu_decoder == ppu_decoder_type::llvm)
		{
			// Assume addr is the start of first segment of PRX
			ppu_ref(addr) = reinterpret_cast<ppu_intrp_func_t>(reinterpret_cast<uptr>(ppu_recompiler_fallback_ghc) | (seg_base << (32 + 3)));
		}
		else
		{
			ppu_ref(addr) = ppu_fallback;
		}

		addr += 4;
		size -= 4;
	}
}

static void ppu_far_jump(ppu_thread&, ppu_opcode_t, be_t<u32>*, ppu_intrp_func*);

extern void ppu_register_function_at(u32 addr, u32 size, ppu_intrp_func_t ptr = nullptr)
{
	// Initialize specific function
	if (ptr)
	{
		ppu_ref(addr) = reinterpret_cast<ppu_intrp_func_t>((reinterpret_cast<uptr>(ptr) & 0xffff'ffff'ffffu) | (uptr(ppu_ref(addr)) & ~0xffff'ffff'ffffu));
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
	while (size)
	{
		if (ppu_ref(addr) != ppu_break && ppu_ref(addr) != ppu_far_jump)
		{
			ppu_ref(addr) = ppu_cache(addr);
		}

		addr += 4;
		size -= 4;
	}
}

extern void ppu_register_function_at(u32 addr, u32 size, u64 ptr)
{
	return ppu_register_function_at(addr, size, reinterpret_cast<ppu_intrp_func_t>(ptr));
}

u32 ppu_get_exported_func_addr(u32 fnid, const std::string& module_name);

void ppu_return_from_far_jump(ppu_thread& ppu, ppu_opcode_t, be_t<u32>*, ppu_intrp_func*)
{
	auto& calls_info = ppu.hle_func_calls_with_toc_info;
	ensure(!calls_info.empty());

	// Branch to next instruction after far jump call entry with restored R2 and LR
	const auto restore_info = &calls_info.back();
	ppu.cia = restore_info->cia + 4;
	ppu.lr = restore_info->saved_lr;
	ppu.gpr[2] = restore_info->saved_r2;

	calls_info.pop_back();
}

static const bool s_init_return_far_jump_func = []
{
	REG_HIDDEN_FUNC_PURE(ppu_return_from_far_jump);
	return true;
}();

struct ppu_far_jumps_t
{
	struct all_info_t
	{
		u32 target;
		bool link;
		bool with_toc;
		std::string module_name;
		ppu_intrp_func_t func;

		u32 get_target(u32 pc, ppu_thread* ppu = nullptr) const
		{
			u32 direct_target = this->target;

			bool to_link = this->link;
			bool from_opd = this->with_toc;

			if (!this->module_name.empty())
			{
				direct_target = ppu_get_exported_func_addr(direct_target, this->module_name);
			}

			if (from_opd && !vm::check_addr<sizeof(ppu_func_opd_t)>(direct_target))
			{
				// Avoid reading unmapped memory under mutex
				from_opd = false;
			}

			if (from_opd)
			{
				auto& opd = vm::_ref<ppu_func_opd_t>(direct_target);
				direct_target = opd.addr;

				// We modify LR to custom values here
				to_link = false;

				if (ppu)
				{
					auto& calls_info = ppu->hle_func_calls_with_toc_info;

					// Save LR and R2
					// Set LR to the this ppu_return_from_far_jump branch for restoration of registers
					// NOTE: In order to clean up this information all calls must return in order
					auto& saved_info = calls_info.emplace_back();
					saved_info.cia = pc;
					saved_info.saved_lr = std::exchange(ppu->lr, g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(ppu_return_from_far_jump), true));
					saved_info.saved_r2 = std::exchange(ppu->gpr[2], opd.rtoc);
				}
			}

			if (to_link && ppu)
			{
				ppu->lr = pc + 4;
			}

			return direct_target;
		}
	};

	ppu_far_jumps_t(int) noexcept {}

	std::map<u32, all_info_t> vals;
	::jit_runtime rt;

	mutable shared_mutex mutex;

	// Get target address, 'ppu' is used in ppu_far_jump in order to modify registers
	u32 get_target(u32 pc, ppu_thread* ppu = nullptr)
	{
		reader_lock lock(mutex);

		if (auto it = vals.find(pc); it != vals.end())
		{
			all_info_t& all_info = it->second;
			return all_info.get_target(pc, ppu);
		}

		return {};
	}

	// Get function patches in range (entry -> target)
	std::vector<std::pair<u32, u32>> get_targets(u32 pc, u32 size)
	{
		std::vector<std::pair<u32, u32>> targets;

		reader_lock lock(mutex);

		auto it = vals.lower_bound(pc);

		if (it == vals.end())
		{
			return targets;
		}

		if (it->first >= pc + size)
		{
			return targets;
		}

		for (auto end = vals.lower_bound(pc + size); it != end; it++)
		{
			all_info_t& all_info = it->second;

			if (u32 target = all_info.get_target(it->first))
			{
				targets.emplace_back(it->first, target);
			}
		}

		return targets;
	}

	// Generate a mini-function which updates PC (for LLVM) and jumps to ppu_far_jump to handle redirections
	template <bool Locked = true>
	ppu_intrp_func_t gen_jump(u32 pc)
	{
		[[maybe_unused]] std::conditional_t<Locked, std::lock_guard<shared_mutex>, const shared_mutex&> lock(mutex);

		auto it = vals.find(pc);

		if (it == vals.end())
		{
			return nullptr;
		}

		if (!it->second.func)
		{
			it->second.func = build_function_asm<ppu_intrp_func_t>("", [&](native_asm& c, auto& args)
			{
				using namespace asmjit;

#ifdef ARCH_X64
				c.mov(args[0], x86::rbp);
				c.mov(x86::dword_ptr(args[0], ::offset32(&ppu_thread::cia)), pc);
				c.jmp(ppu_far_jump);
#else
				Label jmp_address = c.newLabel();
				Label imm_address = c.newLabel();

				c.ldr(args[1].w(), arm::ptr(imm_address));
				c.str(args[1].w(), arm::Mem(args[0], ::offset32(&ppu_thread::cia)));
				c.ldr(args[1], arm::ptr(jmp_address));
				c.br(args[1]);

				c.align(AlignMode::kCode, 16);
				c.bind(jmp_address);
				c.embedUInt64(reinterpret_cast<u64>(ppu_far_jump));
				c.bind(imm_address);
				c.embedUInt32(pc);
#endif
			}, &rt);
		}

		return it->second.func;
	}
};

u32 ppu_get_far_jump(u32 pc)
{
	if (!g_fxo->is_init<ppu_far_jumps_t>())
	{
		return 0;
	}

	return g_fxo->get<ppu_far_jumps_t>().get_target(pc);
}

static void ppu_far_jump(ppu_thread& ppu, ppu_opcode_t, be_t<u32>*, ppu_intrp_func*)
{
	const u32 cia = g_fxo->get<ppu_far_jumps_t>().get_target(ppu.cia, &ppu);

	if (!vm::check_addr(cia, vm::page_executable))
	{
		fmt::throw_exception("PPU far jump failed! (returned cia = 0x%08x)", cia);
	}

	ppu.cia = cia;
}

bool ppu_form_branch_to_code(u32 entry, u32 target, bool link, bool with_toc, std::string module_name)
{
	// Force align entry and target
	entry &= -4;

	// Exported functions are using target as FNID, must not be changed
	if (module_name.empty())
	{
		target &= -4;

		u32 cia_target = target;

		if (with_toc)
		{
			ppu_func_opd_t opd{};
			if (!vm::try_access(target, &opd, sizeof(opd), false))
			{
				// Cannot access function descriptor
				return false;
			}

			// For now allow situations where OPD is changed later by patches or by the program itself
			//cia_target = opd.addr;

			// So force a valid target (executable, yet not equal to entry)
			cia_target = entry ^ 8;
		}

		// Target CIA must be aligned, executable and not equal with
		if (cia_target % 4 || entry == cia_target || !vm::check_addr(cia_target, vm::page_executable))
		{
			return false;
		}
	}

	// Entry must be executable
	if (!vm::check_addr(entry, vm::page_executable))
	{
		return false;
	}

	g_fxo->init<ppu_far_jumps_t>(0);

	if (!module_name.empty())
	{
		// Always use function descriptor for exported functions
		with_toc = true;
	}

	if (with_toc)
	{
		// Always link for calls with function descriptor
		link = true;
	}

	// Register branch target in host memory, not guest memory
	auto& jumps = g_fxo->get<ppu_far_jumps_t>();

	std::lock_guard lock(jumps.mutex);
	jumps.vals.insert_or_assign(entry, ppu_far_jumps_t::all_info_t{target, link, with_toc, std::move(module_name)});
	ppu_register_function_at(entry, 4, g_cfg.core.ppu_decoder == ppu_decoder_type::_static ? &ppu_far_jump : ensure(g_fxo->get<ppu_far_jumps_t>().gen_jump<false>(entry)));

	return true;
}

bool ppu_form_branch_to_code(u32 entry, u32 target, bool link, bool with_toc)
{
	return ppu_form_branch_to_code(entry, target, link, with_toc, std::string{});
}

bool ppu_form_branch_to_code(u32 entry, u32 target, bool link)
{
	return ppu_form_branch_to_code(entry, target, link, false);
}

bool ppu_form_branch_to_code(u32 entry, u32 target)
{
	return ppu_form_branch_to_code(entry, target, false);
}

void ppu_remove_hle_instructions(u32 addr, u32 size)
{
	if (Emu.IsStopped() || !g_fxo->is_init<ppu_far_jumps_t>())
	{
		return;
	}

	auto& jumps = g_fxo->get<ppu_far_jumps_t>();

	std::lock_guard lock(jumps.mutex);

	for (auto it = jumps.vals.begin(); it != jumps.vals.end();)
	{
		if (it->first >= addr && it->first <= addr + size - 1 && size)
		{
			it = jumps.vals.erase(it);
			continue;
		}

		it++;
	}
}

atomic_t<bool> g_debugger_pause_all_threads_on_bp = false;

// Breakpoint entry point
static void ppu_break(ppu_thread& ppu, ppu_opcode_t, be_t<u32>* this_op, ppu_intrp_func* next_fn)
{
	const bool pause_all = g_debugger_pause_all_threads_on_bp;

	const u32 old_cia = vm::get_addr(this_op);
	ppu.cia = old_cia;

	// Pause
	ppu.state.atomic_op([&](bs_t<cpu_flag>& state)
	{
		if (pause_all) state += cpu_flag::dbg_global_pause;
		if (pause_all || !(state & cpu_flag::dbg_step)) state += cpu_flag::dbg_pause;
	});

	if (pause_all)
	{
		// Pause all other threads
		Emu.CallFromMainThread([]() { Emu.Pause(); });
	}

	if (ppu.check_state() || old_cia != atomic_storage<u32>::load(ppu.cia))
	{
		// Do not execute if PC changed
		return;
	}

	// Fallback to the interpreter function
	return ppu_cache(ppu.cia)(ppu, {*this_op}, this_op, ppu.state ? &ppu_ret : next_fn);
}

// Set or remove breakpoint
extern bool ppu_breakpoint(u32 addr, bool is_adding)
{
	if (addr % 4 || !vm::check_addr(addr, vm::page_executable) || g_cfg.core.ppu_decoder == ppu_decoder_type::llvm)
	{
		return false;
	}

	// Remove breakpoint parameters
	ppu_intrp_func_t to_set = 0;
	ppu_intrp_func_t expected = &ppu_break;

	if (u32 hle_addr{}; g_fxo->is_init<ppu_function_manager>() && (hle_addr = g_fxo->get<ppu_function_manager>().addr))
	{
		// HLE function index
		const u32 index = (addr - hle_addr) / 8;

		if (addr % 8 == 4 && index < ppu_function_manager::get().size())
		{
			// HLE function placement
			to_set = ppu_function_manager::get()[index];
		}
	}

	if (!to_set)
	{
		// If not an HLE function use regular instruction function
		to_set = ppu_cache(addr);
	}

	ppu_intrp_func_t& _ref = ppu_ref(addr);

	if (is_adding)
	{
		// Swap if adding
		std::swap(to_set, expected);

		if (_ref == &ppu_fallback)
		{
			ppu_log.error("Unregistered instruction replaced with a breakpoint at 0x%08x", addr);
			expected = ppu_fallback;
		}
	}

	return atomic_storage<ppu_intrp_func_t>::compare_exchange(_ref, expected, to_set);
}

extern bool ppu_patch(u32 addr, u32 value)
{
	if (addr % 4)
	{
		ppu_log.fatal("Patch failed at 0x%x: unanligned memory address.", addr);
		return false;
	}

	vm::writer_lock rlock;

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

	if (is_exec)
	{
		if (ppu_ref(addr) != ppu_break && ppu_ref(addr) != ppu_fallback)
		{
			ppu_ref(addr) = ppu_cache(addr);
		}
	}

	return true;
}

std::array<u32, 2> op_branch_targets(u32 pc, ppu_opcode_t op)
{
	std::array<u32, 2> res{pc + 4, umax};

	if (u32 target = g_fxo->is_init<ppu_far_jumps_t>() ? g_fxo->get<ppu_far_jumps_t>().get_target(pc) : 0)
	{
		res[0] = target;
		return res;
	}

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
		res[0] = umax;
		break;
	}
	default: break;
	}

	return res;
}

void ppu_thread::dump_regs(std::string& ret, std::any& custom_data) const
{
	const system_state emu_state = Emu.GetStatus(false);
	const bool is_stopped_or_frozen = state & cpu_flag::exit || emu_state == system_state::frozen || emu_state <= system_state::stopping;
	const ppu_debugger_mode mode = debugger_mode.load();

	const bool is_decimal = !is_stopped_or_frozen && mode == ppu_debugger_mode::is_decimal;

	struct dump_registers_data_t
	{
		u32 preferred_cr_field_index = 7;
	};

	dump_registers_data_t* func_data = nullptr;

	func_data = std::any_cast<dump_registers_data_t>(&custom_data);

	if (!func_data)
	{
		custom_data.reset();
		custom_data = std::make_any<dump_registers_data_t>();
		func_data = ensure(std::any_cast<dump_registers_data_t>(&custom_data));
	}

	PPUDisAsm dis_asm(cpu_disasm_mode::normal, vm::g_sudo_addr);

	for (uint i = 0; i < 32; ++i)
	{
		auto reg = gpr[i];

		// Fixup for syscall arguments
		if (current_function && i >= 3 && i <= 10) reg = syscall_args[i - 3];

		auto [is_const, const_value] = dis_asm.try_get_const_gpr_value(i, cia);

		if (const_value != reg)
		{
			// Expectation of predictable code path has not been met (such as a branch directly to the instruction)
			is_const = false;
		}

		fmt::append(ret, "r%d%s%s ", i, i <= 9 ? " " : "", is_const ? "©" : ":");

		bool printed_error = false;

		if ((reg >> 31) == 0x1'ffff'ffff)
		{
			const usz old_size = ret.size();

			fmt::append(ret, "%s (0x%x)", CellError{static_cast<u32>(reg)}, reg);

			// Test if failed to format (appended " 0x8".. in such case)
			if (ret[old_size] == '0')
			{
				// Failed
				ret.resize(old_size);
			}
			else
			{
				printed_error = true;
			}
		}

		if (!printed_error)
		{
			if (is_decimal)
			{
				fmt::append(ret, "%-11d", reg);
			}
			else
			{
				fmt::append(ret, "0x%-8llx", reg);
			}
		}

		constexpr u32 max_str_len = 32;
		constexpr u32 hex_count = 8;

		if (reg <= u32{umax} && vm::check_addr<max_str_len>(static_cast<u32>(reg)))
		{
			bool is_function = false;
			u32 toc = 0;

			auto is_exec_code = [&](u32 addr)
			{
				return addr % 4 == 0 && vm::check_addr(addr, vm::page_executable) && g_ppu_itype.decode(*vm::get_super_ptr<u32>(addr)) != ppu_itype::UNK;
			};

			if (const u32 reg_ptr = *vm::get_super_ptr<be_t<u32, 1>>(static_cast<u32>(reg));
				vm::check_addr<8>(reg_ptr) && !vm::check_addr(toc, vm::page_executable))
			{
				// Check executability and alignment
				if (reg % 4 == 0 && is_exec_code(reg_ptr))
				{
					toc = *vm::get_super_ptr<u32>(static_cast<u32>(reg + 4));

					if (toc % 4 == 0 && (toc >> 29) == (reg_ptr >> 29) && vm::check_addr(toc) && !vm::check_addr(toc, vm::page_executable))
					{
						is_function = true;
						reg = reg_ptr;
					}
				}
			}
			else if (is_exec_code(reg))
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

		fmt::trim_back(ret);
		ret += '\n';
	}

	const u32 current_cia = cia;
	const u32 cr_packed = cr.pack();

	for (u32 addr :
	{
		current_cia,
		current_cia + 4,
		current_cia + 8,
		current_cia - 4,
		current_cia + 12,
	})
	{
		dis_asm.disasm(addr);

		if (dis_asm.last_opcode.size() <= 4)
		{
			continue;
		}

		if (usz index = dis_asm.last_opcode.rfind(",cr"); index < dis_asm.last_opcode.size() - 4)
		{
			const char result = dis_asm.last_opcode[index + 3];

			if (result >= '0' && result <= '7')
			{
				func_data->preferred_cr_field_index = result - '0';
				break;
			}
		}

		if (usz index = dis_asm.last_opcode.rfind(" cr"); index < dis_asm.last_opcode.size() - 4)
		{
			const char result = dis_asm.last_opcode[index + 3];

			if (result >= '0' && result <= '7')
			{
				func_data->preferred_cr_field_index = result - '0';
				break;
			}
		}

		if (dis_asm.last_opcode.find("stdcx.") != umax || dis_asm.last_opcode.find("stwcx.") != umax)
		{
			// Modifying CR0
			func_data->preferred_cr_field_index = 0;
			break;
		}
	}

	const u32 displayed_cr_field = (cr_packed >> ((7 - func_data->preferred_cr_field_index) * 4)) & 0xf;

	fmt::append(ret, "CR: 0x%08x, CR%d: [LT=%u GT=%u EQ=%u SO=%u]\n", cr_packed, func_data->preferred_cr_field_index, displayed_cr_field >> 3, (displayed_cr_field >> 2) & 1, (displayed_cr_field >> 1) & 1, displayed_cr_field & 1);

	for (uint i = 0; i < 32; ++i)
	{
		const f64 r = fpr[i];

		if (!std::bit_cast<u64>(r))
		{
			fmt::append(ret, "f%d%s: %-12.6G [%-18s] (f32=0x%x)\n", i, i <= 9 ? " " : "", r, "", std::bit_cast<u32>(f32(r)));
			continue;
		}

		fmt::append(ret, "f%d%s: %-12.6G [0x%016x] (f32=0x%x)\n", i, i <= 9 ? " " : "", r, std::bit_cast<u64>(r), std::bit_cast<u32>(f32(r)));
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

	fmt::append(ret, "CIA: 0x%x\n", current_cia);
	fmt::append(ret, "LR: 0x%llx\n", lr);
	fmt::append(ret, "CTR: 0x%llx\n", ctr);
	fmt::append(ret, "VRSAVE: 0x%08x\n", vrsave);
	fmt::append(ret, "XER: [CA=%u | OV=%u | SO=%u | CNT=%u]\n", xer.ca, xer.ov, xer.so, xer.cnt);
	fmt::append(ret, "VSCR: [SAT=%u | NJ=%u]\n", sat, nj);
	fmt::append(ret, "FPSCR: [FL=%u | FG=%u | FE=%u | FU=%u]\n", fpscr.fl, fpscr.fg, fpscr.fe, fpscr.fu);

	const u32 addr = raddr;

	if (addr)
		fmt::append(ret, "Reservation Addr: 0x%x", addr);
	else
		fmt::append(ret, "Reservation Addr: none");

	fmt::append(ret, "\nReservation Data (entire cache line):\n");

	be_t<u32> data[32]{};
	std::memcpy(data, rdata, sizeof(rdata)); // Show the data even if the reservation was lost inside the atomic loop

	if (addr && !use_full_rdata)
	{
		const u32 offset = addr & 0x78;

		fmt::append(ret, "[0x%02x] %08x %08x\n", offset, data[offset / sizeof(u32)], data[offset / sizeof(u32) + 1]);

		// Asterisk marks the offset of data that had been given to the guest PPU code
		*(&ret.back() - (addr & 4 ? 9 : 18)) = '*';
	}
	else
	{
		for (usz i = 0; i < std::size(data); i += 4)
		{
			fmt::append(ret, "[0x%02x] %08x %08x %08x %08x\n", i * sizeof(data[0])
				, data[i + 0], data[i + 1], data[i + 2], data[i + 3]);
		}

		if (addr)
		{
			// See the note above
			*(&ret.back() - (4 - (addr % 16 / 4)) * 9 - (8 - (addr % 128 / 16)) * std::size("[0x00]"sv)) = '*';
		}
	}
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

	if (r1 > u32{umax} || r1 % 0x10)
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

	bool is_first = true;
	bool skip_single_frame = false;

	const u64 _lr = this->lr;
	const u32 _cia = this->cia;
	const u64 gpr0 = this->gpr[0];

	for (
		u64 sp = r1;
		sp % 0x10 == 0u && sp >= stack_min && sp <= stack_max - ppu_stack_start_offset;
		is_first = false
		)
	{
		auto is_invalid = [](u64 addr)
		{
			if (addr > u32{umax} || addr % 4 || !vm::check_addr(static_cast<u32>(addr), vm::page_executable))
			{
				return true;
			}

			// Ignore HLE stop address
			return addr == g_fxo->get<ppu_function_manager>().func_addr(1, true);
		};

		if (is_first && !is_invalid(_lr))
		{
			// Detect functions with no stack or before LR has been stored

			// Tracking if instruction has already been passed through
			// Instead of using map or set, use two vectors relative to CIA and resize as needed
			std::vector<be_t<u32>> inst_neg;
			std::vector<be_t<u32>> inst_pos;

			auto get_inst = [&](u32 pos) -> be_t<u32>&
			{
				static be_t<u32> s_inst_empty{};

				if (pos < _cia)
				{
					const u32 neg_dist = (_cia - pos - 4) / 4;

					if (neg_dist >= inst_neg.size())
					{
						const u32 inst_bound = pos & -256;

						const usz old_size = inst_neg.size();
						const usz new_size = neg_dist + (pos - inst_bound) / 4 + 1;

						if (new_size >= 0x8000)
						{
							// Gross lower limit for the function (if it is that size it is unlikely that it is even a leaf function)
							return s_inst_empty;
						}

						inst_neg.resize(new_size);

						if (!vm::try_access(inst_bound, &inst_neg[old_size], (new_size - old_size) * sizeof(be_t<u32>), false))
						{
							// Failure (this would be detected as failure by zeroes)
						}

						// Reverse the array (because this buffer directs backwards in address)

						for (usz start = old_size, end = new_size - 1; start < end; start++, end--)
						{
							std::swap(inst_neg[start], inst_neg[end]);
						}
					}

					return inst_neg[neg_dist];
				}

				const u32 pos_dist = (pos - _cia) / 4;

				if (pos_dist >= inst_pos.size())
				{
					const u32 inst_bound = utils::align<u32>(pos, 256);

					const usz old_size = inst_pos.size();
					const usz new_size = pos_dist + (inst_bound - pos) / 4 + 1;

					if (new_size >= 0x8000)
					{
						// Gross upper limit for the function (if it is that size it is unlikely that it is even a leaf function)
						return s_inst_empty;
					}

					inst_pos.resize(new_size);

					if (!vm::try_access(pos, &inst_pos[old_size], (new_size - old_size) * sizeof(be_t<u32>), false))
					{
						// Failure (this would be detected as failure by zeroes)
					}
				}

				return inst_pos[pos_dist];
			};

			bool upper_abort = false;

			struct context_t
			{
				u32 start_point;
				bool maybe_leaf = false; // True if the function is leaf or at the very end/start of non-leaf
				bool non_leaf = false; // Absolutely not a leaf
				bool about_to_push_frame = false; // STDU incoming
				bool about_to_store_lr = false; // Link is about to be stored on stack
				bool about_to_pop_frame = false; // ADDI R1 is about to be issued
				bool about_to_load_link = false; // MTLR is about to be issued
				bool maybe_use_reg0_instead_of_lr = false; // Use R0 at the end of a non-leaf function if ADDI has been issued before MTLR
			};

			// Start with CIA
			std::deque<context_t> workload{context_t{_cia}};

			usz start = 0;

			for (; start < workload.size(); start++)
			{
				for (u32 wa = workload[start].start_point; vm::check_addr(wa, vm::page_executable);)
				{
					be_t<u32>& opcode = get_inst(wa);

					auto& [_, maybe_leaf, non_leaf, about_to_push_frame, about_to_store_lr,
						about_to_pop_frame, about_to_load_link, maybe_use_reg0_instead_of_lr] = workload[start];

					if (!opcode)
					{
						// Already passed or failure of reading
						break;
					}

					const ppu_opcode_t op{opcode};

					// Mark as passed through
					opcode = 0;

					const auto type = g_ppu_itype.decode(op.opcode);

					if (workload.size() == 1 && type == ppu_itype::STDU && op.rs == 1u && op.ra == 1u)
					{
						if (op.simm16 >= 0)
						{
							// Against ABI
							non_leaf = true;
							upper_abort = true;
							break;
						}

						// Saving LR to register: this is indeed a new function (ok because LR has not been saved yet)
						maybe_leaf = true;
						about_to_push_frame = true;
						about_to_pop_frame = false;
						upper_abort = true;
						break;
					}

					if (workload.size() == 1 && type == ppu_itype::STD && op.ra == 1u && op.rs == 0u)
					{
						bool found_matching_stdu = false;

						for (u32 back = 1; back < 20; back++)
						{
							be_t<u32>& opcode = get_inst(utils::sub_saturate<u32>(_cia, back * 4));

							if (!opcode)
							{
								// Already passed or failure of reading
								break;
							}

							const ppu_opcode_t test_op{opcode};

							const auto type = g_ppu_itype.decode(test_op.opcode);

							if (type == ppu_itype::BCLR)
							{
								break;
							}

							if (type == ppu_itype::STDU && test_op.rs == 1u && test_op.ra == 1u)
							{
								if (0 - (test_op.ds << 2) == (op.ds << 2) - 0x10)
								{
									found_matching_stdu = true;
								}

								break;
							}
						}

						if (found_matching_stdu)
						{
							// Saving LR to stack: this is indeed a new function (ok because LR has not been saved yet)
							maybe_leaf = true;
							about_to_store_lr = true;
							about_to_pop_frame = true;
							upper_abort = true;
							break;
						}
					}

					const u32 spr = ((op.spr >> 5) | ((op.spr & 0x1f) << 5));

					// It can be placed before or after STDU, ignore for now
					// if (workload.size() == 1 && type == ppu_itype::MFSPR && op.rs == 0u && spr == 0x8)
					// {
					// 	// Saving LR to register: this is indeed a new function (ok because LR has not been saved yet)
					// 	maybe_leaf = true;
					// 	about_to_store_lr = true;
					// 	about_to_pop_frame = true;
					// }

					if (type == ppu_itype::MTSPR && spr == 0x8 && op.rs == 0u)
					{
						// Test for special case: if ADDI R1 is not found later in code, it means that LR is not restored and R0 should be used instead
						// Can also search for ADDI R1 backwards and pull the value from stack (needs more research if it is more reliable)
						maybe_use_reg0_instead_of_lr = true;
					}

					if (type == ppu_itype::UNK)
					{
						// Ignore for now
						break;
					}

					if ((type & ppu_itype::branch) && op.lk)
					{
						// Gave up on LR before saving
						non_leaf = true;
						about_to_pop_frame = true;
						upper_abort = true;
						break;
					}

					// Even if BCLR is conditional, it still counts because LR value is ready for return
					if (type == ppu_itype::BCLR)
					{
						// Returned
						maybe_leaf = true;
						upper_abort = true;
						break;
					}

					if (type == ppu_itype::ADDI && op.ra == 1u && op.rd == 1u)
					{
						if (op.simm16 < 0)
						{
							// Against ABI
							non_leaf = true;
							upper_abort = true;
							break;
						}
						else if (op.simm16 > 0)
						{
							// Remember that SP is about to be restored
							about_to_pop_frame = true;
							non_leaf = true;
							upper_abort = true;
							break;
						}
					}

					const auto results = op_branch_targets(wa, op);

					bool proceeded = false;

					for (usz res_i = 0; res_i < results.size(); res_i++)
					{
						const u32 route_pc = results[res_i];

						if (route_pc == umax)
						{
							continue;
						}

						if (vm::check_addr(route_pc, vm::page_executable) && get_inst(route_pc))
						{
							if (proceeded)
							{
								// Remember next route start point
								workload.push_back(context_t{route_pc});
							}
							else
							{
								// Next PC
								wa = route_pc;
								proceeded = true;
							}
						}
					}
				}

				if (upper_abort)
				{
					break;
				}
			}

			const context_t& res = workload[std::min<usz>(start, workload.size() - 1)];

			if (res.maybe_leaf && !res.non_leaf)
			{
				const u32 result = res.maybe_use_reg0_instead_of_lr ? static_cast<u32>(gpr0) : static_cast<u32>(_lr);

				// Same stack as far as we know
				call_stack_list.emplace_back(result, static_cast<u32>(sp));

				if (res.about_to_store_lr)
				{
					// LR has yet to be stored on stack, ignore the stack value
					skip_single_frame = true;
				}
			}

			if (res.about_to_pop_frame || (res.maybe_leaf && !res.non_leaf))
			{
				const u64 temp_sp = *vm::get_super_ptr<u64>(static_cast<u32>(sp));

				if (temp_sp <= sp)
				{
					// Ensure inequality and that the old stack pointer is higher than current
					break;
				}

				// Read the first stack frame so caller addresses can be obtained
				sp = temp_sp;
				continue;
			}
		}

		u64 addr = *vm::get_super_ptr<u64>(static_cast<u32>(sp + 16));

		if (skip_single_frame)
		{
			skip_single_frame = false;
		}
		else if (!is_invalid(addr))
		{
			// TODO: function addresses too
			call_stack_list.emplace_back(static_cast<u32>(addr), static_cast<u32>(sp));
		}
		else if (!is_first)
		{
			break;
		}

		const u64 temp_sp = *vm::get_super_ptr<u64>(static_cast<u32>(sp));

		if (temp_sp <= sp)
		{
			// Ensure inequality and that the old stack pointer is higher than current
			break;
		}

		sp = temp_sp;

		is_first = false;
	}

	return call_stack_list;
}

std::string ppu_thread::dump_misc() const
{
	std::string ret = cpu_thread::dump_misc();

	if (ack_suspend)
	{
		if (ret.ends_with("\n"))
		{
			ret.pop_back();
		}

		fmt::append(ret, " (LV2 suspended)\n");
	}

	fmt::append(ret, "Priority: %d\n", prio.load().prio);
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

		for (u32 i = 3; i <= 10; i++)
			if (u64 v = gpr[i]; v != syscall_args[i - 3])
				fmt::append(ret, " ** r%d: 0x%llx\n", i, v);
	}
	else if (is_paused() || is_stopped())
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

void ppu_thread::dump_all(std::string& ret) const
{
	cpu_thread::dump_all(ret);

	if (call_history.data.size() > 1)
	{
		ret +=
			"\nCalling History:"
			"\n================";

		fmt::append(ret, "%s", call_history);
	}

	if (syscall_history.data.size() > 1)
	{
		ret +=
			"\nHLE/LV2 History:"
			"\n================";

		fmt::append(ret, "%s", syscall_history);
	}
}

extern thread_local std::string(*g_tls_log_prefix)();

void ppu_thread::cpu_task()
{
	std::fesetround(FE_TONEAREST);

	if (g_cfg.core.set_daz_and_ftz)
	{
		gv_set_zeroing_denormals();
	}
	else
	{
		gv_unset_zeroing_denormals();
	}

	// Execute cmd_queue
	while (cmd64 cmd = cmd_wait())
	{
		const u32 arg = cmd.arg2<u32>(); // 32-bit arg extracted

		switch (auto type = cmd.arg1<ppu_cmd>())
		{
		case ppu_cmd::opcode:
		{
			cmd_pop(), g_fxo->get<ppu_interpreter_rt>().decode(arg)(*this, {arg}, vm::_ptr<u32>(cia - 4), &ppu_ret);
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
#ifdef __APPLE__
			pthread_jit_write_protect_np(true);
#endif
			const vm::ptr<u32> opd(arg < 32 ? vm::cast(gpr[arg]) : vm::cast(arg));
			cmd_pop(), fast_call(opd[0], opd[1]);
			break;
		}
		case ppu_cmd::entry_call:
		{
#ifdef __APPLE__
			pthread_jit_write_protect_np(true);
#endif
			cmd_pop(), fast_call(entry_func.addr, entry_func.rtoc, true);
			break;
		}
		case ppu_cmd::hle_call:
		{
			cmd_pop(), ::at32(ppu_function_manager::get(), arg)(*this, {arg}, vm::_ptr<u32>(cia - 4), &ppu_ret);
			break;
		}
		case ppu_cmd::opd_call:
		{
#ifdef __APPLE__
			pthread_jit_write_protect_np(true);
#endif
			const ppu_func_opd_t opd = cmd_get(1).as<ppu_func_opd_t>();
			cmd_pop(1), fast_call(opd.addr, opd.rtoc);
			break;
		}
		case ppu_cmd::ptr_call:
		{
			const ppu_intrp_func_t func = cmd_get(1).as<ppu_intrp_func_t>();
			cmd_pop(1), func(*this, {}, vm::_ptr<u32>(cia - 4), &ppu_ret);
			break;
		}
		case ppu_cmd::cia_call:
		{
			loaded_from_savestate = true;
			cmd_pop(), fast_call(std::exchange(cia, 0), gpr[2], true);
			break;
		}
		case ppu_cmd::initialize:
		{
#ifdef __APPLE__
			pthread_jit_write_protect_np(false);
#endif
			cmd_pop();

			ppu_initialize();

			if (Emu.IsStopped())
			{
				return;
			}

			spu_cache::initialize();

#ifdef __APPLE__
			pthread_jit_write_protect_np(true);
#endif
#ifdef ARCH_ARM64
			// Flush all cache lines after potentially writing executable code
			asm("ISB");
			asm("DSB ISH");
#endif

			// Wait until the progress dialog is closed.
			// We don't want to open a cell dialog while a native progress dialog is still open.
			while (u32 v = g_progr_ptotal)
			{
				if (Emu.IsStopped())
				{
					return;
				}

				g_progr_ptotal.wait(v);
			}

			g_fxo->get<progress_dialog_workaround>().show_overlay_message_only = true;

			// Sadly we can't postpone initializing guest time because we need to run PPU threads
			// (the farther it's postponed, the less accuracy of guest time has been lost)
			Emu.FixGuestTime();

			// Run SPUs waiting on a syscall (savestates related)
			idm::select<named_thread<spu_thread>>([&](u32, named_thread<spu_thread>& spu)
			{
				if (spu.group && spu.index == spu.group->waiter_spu_index)
				{
					if (std::exchange(spu.stop_flag_removal_protection, false))
					{
						return;
					}

					ensure(spu.state.test_and_reset(cpu_flag::stop));
					spu.state.notify_one();
				}
			});

			// Check if this is the only PPU left to initialize (savestates related)
			if (lv2_obj::is_scheduler_ready())
			{
				if (Emu.IsStarting())
				{
					Emu.FinalizeRunRequest();
				}
			}

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

	// TODO: More conditions
	if (Emu.IsStopped() && g_cfg.core.ppu_debug)
	{
		std::string ret;
		dump_all(ret);
		ppu_log.notice("thread context: %s", ret);
	}
}

void ppu_thread::exec_task()
{
	if (g_cfg.core.ppu_decoder != ppu_decoder_type::_static)
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
	const auto mem_ = vm::g_base_addr;

	while (true)
	{
		if (test_stopped()) [[unlikely]]
		{
			return;
		}

		gv_zeroupper();

		// Execute instruction (may be step; execute only one instruction if state)
		const auto op = reinterpret_cast<be_t<u32>*>(mem_ + u64{cia});
		const auto fn = reinterpret_cast<ppu_intrp_func*>(cache + u64{cia} * 2);
		fn->fn(*this, {*op}, op, state ? &ppu_ret : fn + 1);
	}
}

ppu_thread::~ppu_thread()
{
	perf_log.notice("Perf stats for STCX reload: success %u, failure %u", last_succ, last_fail);
	perf_log.notice("Perf stats for instructions: total %u", exec_bytes / 4);
}

ppu_thread::ppu_thread(const ppu_thread_params& param, std::string_view name, u32 _prio, int detached)
	: cpu_thread(idm::last_id())
	, stack_size(param.stack_size)
	, stack_addr(param.stack_addr)
	, joiner(detached != 0 ? ppu_join_status::detached : ppu_join_status::joinable)
	, entry_func(param.entry)
	, start_time(get_guest_system_time())
	, is_interrupt_thread(detached < 0)
	, ppu_tname(make_single<std::string>(name))
{
	prio.raw().prio = _prio;

	gpr[1] = stack_addr + stack_size - ppu_stack_start_offset;

	gpr[13] = param.tls_addr;

	if (detached >= 0)
	{
		// Initialize thread args
		gpr[3] = param.arg0;
		gpr[4] = param.arg1;
	}

	optional_savestate_state = std::make_shared<utils::serial>();

	// Trigger the scheduler
	state += cpu_flag::suspend;

	if (!g_use_rtm)
	{
		state += cpu_flag::memory;
	}

	call_history.data.resize(g_cfg.core.ppu_call_history ? call_history_max_size : 1);
	syscall_history.data.resize(g_cfg.core.ppu_call_history ? syscall_history_max_size : 1);
	syscall_history.count_debug_arguments = static_cast<u32>(g_cfg.core.ppu_call_history ? std::size(syscall_history.data[0].args) : 0);

#ifdef __APPLE__
	pthread_jit_write_protect_np(true);
#endif
#ifdef ARCH_ARM64
	// Flush all cache lines after potentially writing executable code
	asm("ISB");
	asm("DSB ISH");
#endif
}

struct disable_precomp_t
{
	atomic_t<bool> disable = false;
};

void vdecEntry(ppu_thread& ppu, u32 vid);

bool ppu_thread::savable() const
{
	if (joiner == ppu_join_status::exited)
	{
		return false;
	}

	if (cia == g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(vdecEntry)))
	{
		// Do not attempt to save the state of HLE VDEC threads
		return false;
	}

	return true;
}

void ppu_thread::serialize_common(utils::serial& ar)
{
	[[maybe_unused]] const s32 version = GET_OR_USE_SERIALIZATION_VERSION(ar.is_writing(), ppu);

	ar(gpr, fpr, cr, fpscr.bits, lr, ctr, vrsave, cia, xer, sat, nj, prio.raw().all);

	ar(optional_savestate_state, vr);

	if (optional_savestate_state->data.empty())
	{
		optional_savestate_state->clear();
	}
}

ppu_thread::ppu_thread(utils::serial& ar)
	: cpu_thread(idm::last_id()) // last_id() is showed to constructor on serialization
	, stack_size(ar)
	, stack_addr(ar)
	, joiner(ar.operator ppu_join_status())
	, entry_func(std::bit_cast<ppu_func_opd_t, u64>(ar))
	, is_interrupt_thread(ar)
{
	struct init_pushed
	{
		bool pushed = false;
		atomic_t<u32> inited = false;
	};

	call_history.data.resize(g_cfg.core.ppu_call_history ? call_history_max_size : 1);
	syscall_history.data.resize(g_cfg.core.ppu_call_history ? syscall_history_max_size : 1);
	syscall_history.count_debug_arguments = static_cast<u32>(g_cfg.core.ppu_call_history ? std::size(syscall_history.data[0].args) : 0);

	serialize_common(ar);

	// Restore jm_mask
	jm_mask = nj ? 0x7F800000 : 0x7fff'ffff;

	auto queue_intr_entry = [&]()
	{
		if (is_interrupt_thread)
		{
			void ppu_interrupt_thread_entry(ppu_thread&, ppu_opcode_t, be_t<u32>*, struct ppu_intrp_func*);

			cmd_list
			({
				{ ppu_cmd::ptr_call, 0 },
				std::bit_cast<u64>(&ppu_interrupt_thread_entry)
			});
		}
	};

	switch (const u32 status = ar.operator u32())
	{
	case PPU_THREAD_STATUS_IDLE:
	{
		stop_flag_removal_protection = true;
		break;
	}
	case PPU_THREAD_STATUS_RUNNABLE:
	case PPU_THREAD_STATUS_ONPROC:
	{
		lv2_obj::awake(this);
		[[fallthrough]];
	}
	case PPU_THREAD_STATUS_SLEEP:
	{
		if (std::exchange(g_fxo->get<init_pushed>().pushed, true))
		{
			cmd_list
			({
				{ppu_cmd::ptr_call, 0}, +[](ppu_thread&) -> bool
				{
					while (!Emu.IsStopped() && !g_fxo->get<init_pushed>().inited)
					{
						thread_ctrl::wait_on(g_fxo->get<init_pushed>().inited, 0);
					}
					return false;
				}
			});
		}
		else
		{
			g_fxo->init<disable_precomp_t>();
			g_fxo->get<disable_precomp_t>().disable = true;

			cmd_push({ppu_cmd::initialize, 0});
			cmd_list
			({
				{ppu_cmd::ptr_call, 0}, +[](ppu_thread&) -> bool
				{
					auto& inited = g_fxo->get<init_pushed>().inited;
					inited = 1;
					inited.notify_all();
					return true;
				}
			});
		}

		if (status == PPU_THREAD_STATUS_SLEEP)
		{
			cmd_list
			({
				{ppu_cmd::ptr_call, 0},

				+[](ppu_thread& ppu) -> bool
				{
					const u32 op = vm::read32(ppu.cia);
					const auto& table = g_fxo->get<ppu_interpreter_rt>();
					ppu.loaded_from_savestate = true;
					table.decode(op)(ppu, {op}, vm::_ptr<u32>(ppu.cia), &ppu_ret);

					ppu.optional_savestate_state->clear(); // Reset to writing state
					ppu.loaded_from_savestate = false;
					return true;
				}
			});

			lv2_obj::set_future_sleep(this);
		}

		queue_intr_entry();
		cmd_push({ppu_cmd::cia_call, 0});
		break;
	}
	case PPU_THREAD_STATUS_ZOMBIE:
	{
		state += cpu_flag::exit;
		break;
	}
	case PPU_THREAD_STATUS_STOP:
	{
		queue_intr_entry();
		break;
	}
	}

	// Trigger the scheduler
	state += cpu_flag::suspend;

	if (!g_use_rtm)
	{
		state += cpu_flag::memory;
	}

	ppu_tname = make_single<std::string>(ar.operator std::string());
}

void ppu_thread::save(utils::serial& ar)
{
	USING_SERIALIZATION_VERSION(ppu);

	const u64 entry = std::bit_cast<u64>(entry_func);

	ppu_join_status _joiner = joiner;
	if (_joiner >= ppu_join_status::max)
	{
		// Joining thread should recover this member properly
		_joiner = ppu_join_status::joinable;
	}

	if (state & cpu_flag::again)
	{
		std::memcpy(&gpr[3], syscall_args, sizeof(syscall_args));
		cia -= 4;
	}

	ar(stack_size, stack_addr, _joiner, entry, is_interrupt_thread);
	serialize_common(ar);

	ppu_thread_status status = lv2_obj::ppu_state(this, false);

	if (status == PPU_THREAD_STATUS_SLEEP && cpu_flag::again - state)
	{
		// Hack for sys_fs
		status = PPU_THREAD_STATUS_RUNNABLE;
	}

	ar(status);

	ar(*ppu_tname.load());
}

ppu_thread::thread_name_t::operator std::string() const
{
	std::string thread_name = fmt::format("PPU[0x%x]", _this->id);

	if (const std::string name = *_this->ppu_tname.load(); !name.empty())
	{
		fmt::append(thread_name, " %s", name);
	}

	return thread_name;
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

void ppu_thread::fast_call(u32 addr, u64 rtoc, bool is_thread_entry)
{
	const auto old_cia = cia;
	const auto old_rtoc = gpr[2];
	const auto old_lr = lr;
	const auto old_func = current_function;
	const auto old_fmt = g_tls_log_prefix;

	interrupt_thread_executing = true;
	cia = addr;
	gpr[2] = rtoc;
	lr = g_fxo->get<ppu_function_manager>().func_addr(1, true); // HLE stop address
	current_function = nullptr;

	if (std::exchange(loaded_from_savestate, false))
	{
		lr = old_lr;
	}

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

		extern const char* get_prx_name_by_cia(u32 addr);

		if (auto name = get_prx_name_by_cia(cia))
		{
			return fmt::format("PPU[0x%x] Thread (%s) [%s: 0x%08x]", _this->id, *name_cache.get(), name, cia);
		}

		return fmt::format("PPU[0x%x] Thread (%s) [0x%08x]", _this->id, *name_cache.get(), cia);
	};

	auto at_ret = [&]()
	{
		if (old_cia)
		{
			if (state & cpu_flag::again)
			{
				ppu_log.error("HLE callstack savestate is not implemented!");
			}

			cia = old_cia;
			gpr[2] = old_rtoc;
			lr = old_lr;
		}
		else if (state & cpu_flag::ret && cia == g_fxo->get<ppu_function_manager>().func_addr(1, true) + 4 && is_thread_entry)
		{
			std::string ret;
			dump_all(ret);

			ppu_log.error("Returning from the thread entry function! (func=0x%x)", entry_func.addr);
			ppu_log.notice("Thread context: %s", ret);

			lv2_obj::sleep(*this);

			// For savestates
			state += cpu_flag::again;
			std::memcpy(syscall_args, &gpr[3], sizeof(syscall_args));
		}

		current_function = old_func;
		g_tls_log_prefix = old_fmt;
		state -= cpu_flag::ret;
	};

	exec_task();
	at_ret();
}

std::pair<vm::addr_t, u32> ppu_thread::stack_push(u32 size, u32 align_v)
{
	if (auto cpu = get_current_cpu_thread<ppu_thread>())
	{
		ppu_thread& context = static_cast<ppu_thread&>(*cpu);

		const u32 old_pos = vm::cast(context.gpr[1]);
		context.gpr[1] -= size; // room minimal possible size
		context.gpr[1] &= ~(u64{align_v} - 1); // fix stack alignment

		auto is_stack = [&](u64 addr)
		{
			return addr >= context.stack_addr && addr < context.stack_addr + context.stack_size;
		};

		// TODO: This check does not care about custom stack memory
		if (is_stack(old_pos) != is_stack(context.gpr[1]))
		{
			fmt::throw_exception("Stack overflow (size=0x%x, align=0x%x, SP=0x%llx, stack=*0x%x)", size, align_v, old_pos, context.stack_addr);
		}
		else
		{
			const u32 addr = static_cast<u32>(context.gpr[1]);
			std::memset(vm::base(addr), 0, size);
			return {vm::cast(addr), old_pos - addr};
		}
	}

	fmt::throw_exception("Invalid thread");
}

void ppu_thread::stack_pop_verbose(u32 addr, u32 size) noexcept
{
	if (auto cpu = get_current_cpu_thread<ppu_thread>())
	{
		ppu_thread& context = static_cast<ppu_thread&>(*cpu);

		if (context.gpr[1] != addr)
		{
			ppu_log.error("Stack inconsistency (addr=0x%x, SP=0x%llx, size=0x%x)", addr, context.gpr[1], size);
			return;
		}

		context.gpr[1] += size;
		return;
	}

	ppu_log.error("Invalid thread");
}

extern ppu_intrp_func_t ppu_get_syscall(u64 code);

void ppu_trap(ppu_thread& ppu, u64 addr)
{
	ensure((addr & (~u64{0xffff'ffff} | 0x3)) == 0);
	ppu.cia = static_cast<u32>(addr);

	u32 add = static_cast<u32>(g_cfg.core.stub_ppu_traps) * 4;

	// If stubbing is enabled, check current instruction and the following
	if (!add || !vm::check_addr(ppu.cia, vm::page_executable) || !vm::check_addr(ppu.cia + add, vm::page_executable))
	{
		fmt::throw_exception("PPU Trap! Sometimes tweaking the setting \"Stub PPU Traps\" can be a workaround to this crash.\nBest values depend on game code, if unsure try 1.");
	}

	ppu_log.error("PPU Trap: Stubbing %d instructions %s.", std::abs(static_cast<s32>(add) / 4), add >> 31 ? "backwards" : "forwards");
	ppu.cia += add; // Skip instructions, hope for valid code (interprter may be invoked temporarily)
}

static void ppu_error(ppu_thread& ppu, u64 addr, u32 /*op*/)
{
	ppu.cia = ::narrow<u32>(addr);
	ppu_recompiler_fallback(ppu);
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

				if (!gv_testz(gv_eq32(_inst, store_vec)))
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

	if (ppu_log.trace && (addr & addr_mask) == (ppu.last_faddr & addr_mask))
	{
		ppu_log.trace(u8"LARX after fail: addr=0x%x, faddr=0x%x, time=%u c", addr, ppu.last_faddr, (perf0.get() - ppu.last_ftsc));
	}

	if ((addr & addr_mask) == (ppu.last_faddr & addr_mask) && (perf0.get() - ppu.last_ftsc) < 600 && (vm::reservation_acquire(addr) & -128) == ppu.last_ftime)
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

	ppu.rtime = vm::reservation_acquire(addr) & -128;

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

const auto ppu_stcx_accurate_tx = build_function_asm<u64(*)(u32 raddr, u64 rtime, const void* _old, u64 _new)>("ppu_stcx_accurate_tx", [](native_asm& c, auto& args)
{
	using namespace asmjit;

#if defined(ARCH_X64)
	Label fall = c.newLabel();
	Label fail = c.newLabel();
	Label _ret = c.newLabel();
	Label load = c.newLabel();

	//if (utils::has_avx() && !s_tsx_avx)
	//{
	//	c.vzeroupper();
	//}

	// Create stack frame if necessary (Windows ABI has only 6 volatile vector registers)
	c.push(x86::rbp);
	c.push(x86::r14);
	c.sub(x86::rsp, 40);
#ifdef _WIN32
	if (!s_tsx_avx)
	{
		c.movups(x86::oword_ptr(x86::rsp, 0), x86::xmm6);
		c.movups(x86::oword_ptr(x86::rsp, 16), x86::xmm7);
	}
#endif

	// Prepare registers
	build_swap_rdx_with(c, args, x86::r10);
	c.mov(x86::rbp, x86::qword_ptr(reinterpret_cast<u64>(&vm::g_sudo_addr)));
	c.lea(x86::rbp, x86::qword_ptr(x86::rbp, args[0]));
	c.and_(x86::rbp, -128);
	c.prefetchw(x86::byte_ptr(x86::rbp, 0));
	c.prefetchw(x86::byte_ptr(x86::rbp, 64));
	c.movzx(args[0].r32(), args[0].r16());
	c.shr(args[0].r32(), 1);
	c.lea(x86::r11, x86::qword_ptr(reinterpret_cast<u64>(+vm::g_reservations), args[0]));
	c.and_(x86::r11, -128 / 2);
	c.and_(args[0].r32(), 63);

	// Prepare data
	if (s_tsx_avx)
	{
		c.vmovups(x86::ymm0, x86::ymmword_ptr(args[2], 0));
		c.vmovups(x86::ymm1, x86::ymmword_ptr(args[2], 32));
		c.vmovups(x86::ymm2, x86::ymmword_ptr(args[2], 64));
		c.vmovups(x86::ymm3, x86::ymmword_ptr(args[2], 96));
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
	build_get_tsc(c, stamp0);

	Label fail2 = c.newLabel();

	Label tx1 = build_transaction_enter(c, fall, [&]()
	{
		build_get_tsc(c);
		c.sub(x86::rax, stamp0);
		c.cmp(x86::rax, x86::qword_ptr(reinterpret_cast<u64>(&g_rtm_tx_limit2)));
		c.jae(fall);
	});

	// Check pause flag
	c.bt(x86::dword_ptr(args[2], ::offset32(&ppu_thread::state) - ::offset32(&ppu_thread::rdata)), static_cast<u32>(cpu_flag::pause));
	c.jc(fall);
	c.xbegin(tx1);

	if (s_tsx_avx)
	{
		c.vxorps(x86::ymm0, x86::ymm0, x86::ymmword_ptr(x86::rbp, 0));
		c.vxorps(x86::ymm1, x86::ymm1, x86::ymmword_ptr(x86::rbp, 32));
		c.vxorps(x86::ymm2, x86::ymm2, x86::ymmword_ptr(x86::rbp, 64));
		c.vxorps(x86::ymm3, x86::ymm3, x86::ymmword_ptr(x86::rbp, 96));
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

	c.xend();
	c.lock().add(x86::qword_ptr(x86::r11), 64);
	build_get_tsc(c);
	c.sub(x86::rax, stamp0);
	c.jmp(_ret);

	// XABORT is expensive so try to finish with xend instead
	c.bind(fail);

	// Load old data to store back in rdata
	if (s_tsx_avx)
	{
		c.vmovaps(x86::ymm0, x86::ymmword_ptr(x86::rbp, 0));
		c.vmovaps(x86::ymm1, x86::ymmword_ptr(x86::rbp, 32));
		c.vmovaps(x86::ymm2, x86::ymmword_ptr(x86::rbp, 64));
		c.vmovaps(x86::ymm3, x86::ymmword_ptr(x86::rbp, 96));
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

	c.bind(fall);
	c.mov(x86::rax, -1);
	c.jmp(_ret);

	c.bind(fail2);
	c.lock().sub(x86::qword_ptr(x86::r11), 64);
	c.bind(load);

	// Store previous data back to rdata
	if (s_tsx_avx)
	{
		c.vmovaps(x86::ymmword_ptr(args[2], 0), x86::ymm0);
		c.vmovaps(x86::ymmword_ptr(args[2], 32), x86::ymm1);
		c.vmovaps(x86::ymmword_ptr(args[2], 64), x86::ymm2);
		c.vmovaps(x86::ymmword_ptr(args[2], 96), x86::ymm3);
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
	c.pop(x86::r14);
	c.pop(x86::rbp);

	maybe_flush_lbr(c);
	c.ret();
#else
	// Unimplemented should fail.
	c.brk(Imm(0x42));
	c.ret(a64::x30);
#endif
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
	auto& res = vm::reservation_acquire(addr);
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

			if (g_use_rtm) [[likely]]
			{
				switch (u64 count = ppu_stcx_accurate_tx(addr & -8, rtime, ppu.rdata, std::bit_cast<u64>(new_data)))
				{
				case umax:
				{
					auto& all_data = *vm::get_super_ptr<spu_rdata_t>(addr & -128);
					auto& sdata = *vm::get_super_ptr<atomic_be_t<u64>>(addr & -8);

					const bool ok = cpu_thread::suspend_all<+3>(&ppu, {all_data, all_data + 64, &res}, [&]
					{
						if ((res & -128) == rtime && cmp_rdata(ppu.rdata, all_data))
						{
							sdata.release(new_data);
							res += 64;
							return true;
						}

						mov_rdata_nt(ppu.rdata, all_data);
						res -= 64;
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
					ppu.last_ftsc = utils::get_tsc();
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

		const u64 lock_bits = vm::rsrv_unique_lock;

		auto [_oldd, _ok] = res.fetch_op([&](u64& r)
		{
			if ((r & -128) != rtime || (r & 127))
			{
				return false;
			}

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
			ppu.last_ftsc = utils::get_tsc();
			std::memcpy(&ppu.rdata[addr & 0x78], &old_data, 8);
		}

		return false;
	}())
	{
		extern atomic_t<u32> liblv2_begin, liblv2_end;

		const u32 notify = ppu.res_notify;

		if (notify)
		{
			vm::reservation_notifier(notify).notify_all();
			ppu.res_notify = 0;
		}

		// Avoid notifications from lwmutex or sys_spinlock
		if (ppu.cia < liblv2_begin || ppu.cia >= liblv2_end)
		{
			if (!notify)
			{
				// Try to postpone notification to when PPU is asleep or join notifications on the same address
				// This also optimizes a mutex - won't notify after lock is aqcuired (prolonging the critical section duration), only notifies on unlock
				ppu.res_notify = addr;
			}
			else if ((addr ^ notify) & -128)
			{
				res.notify_all();
			}
		}

		if (addr == ppu.last_faddr)
		{
			ppu.last_succ++;
		}

		ppu.last_faddr = 0;
		return true;
	}

	const u32 notify = ppu.res_notify;

	// Do not risk postponing too much (because this is probably an indefinite loop)
	// And on failure it has some time to do something else
	if (notify && ((addr ^ notify) & -128))
	{
		vm::reservation_notifier(notify).notify_all();
		ppu.res_notify = 0;
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

struct jit_core_allocator
{
	const s16 thread_count = g_cfg.core.llvm_threads ? std::min<s32>(g_cfg.core.llvm_threads, limit()) : limit();

	// Initialize global semaphore with the max number of threads
	::semaphore<0x7fff> sem{std::max<s16>(thread_count, 1)};

	static s16 limit()
	{
		return static_cast<s16>(std::min<s32>(0x7fff, utils::get_thread_count()));
	}
};

#ifdef LLVM_AVAILABLE
namespace
{
	// Compiled PPU module info
	struct jit_module
	{
		std::vector<ppu_intrp_func_t> funcs;
		std::shared_ptr<jit_compiler> pjit;
		bool init = false;
	};

	struct jit_module_manager
	{
		struct bucket_t
		{
			shared_mutex mutex;
			std::unordered_map<std::string, jit_module> map;
		};

		std::array<bucket_t, 30> buckets;

		bucket_t& get_bucket(std::string_view sv)
		{
			return buckets[std::hash<std::string_view>()(sv) % std::size(buckets)];
		}

		jit_module& get(const std::string& name)
		{
			bucket_t& bucket = get_bucket(name);
			std::lock_guard lock(bucket.mutex);
			return bucket.map.emplace(name, jit_module{}).first->second;
		}

		void remove(const std::string& name) noexcept
		{
			bucket_t& bucket = get_bucket(name);

			jit_module to_destroy{};

			std::lock_guard lock(bucket.mutex);
			const auto found = bucket.map.find(name);

			if (found == bucket.map.end()) [[unlikely]]
			{
				ppu_log.error("Failed to remove module %s", name);
				return;
			}

			to_destroy.funcs = std::move(found->second.funcs);
			to_destroy.pjit = std::move(found->second.pjit);

			bucket.map.erase(found);
		}
	};
}
#endif

namespace
{
	// Read-only file view starting with specified offset (for MSELF)
	struct file_view : fs::file_base
	{
		const fs::file m_storage;
		const fs::file& m_file;
		const u64 m_off;
		const u64 m_max_size;
		u64 m_pos;

		explicit file_view(const fs::file& _file, u64 offset, u64 max_size) noexcept
			: m_storage(fs::file())
			, m_file(_file)
			, m_off(offset)
			, m_max_size(max_size)
			, m_pos(0)
		{
		}

		explicit file_view(fs::file&& _file, u64 offset, u64 max_size) noexcept
			: m_storage(std::move(_file))
			, m_file(m_storage)
			, m_off(offset)
			, m_max_size(max_size)
			, m_pos(0)
		{
		}

		~file_view() override
		{
		}

		fs::stat_t get_stat() override
		{
			return m_file.get_stat();
		}

		bool trunc(u64) override
		{
			return false;
		}

		u64 read(void* buffer, u64 size) override
		{
			const u64 result = file_view::read_at(m_pos, buffer, size);
			m_pos += result;
			return result;
		}

		u64 read_at(u64 offset, void* buffer, u64 size) override
		{
			return m_file.read_at(offset + m_off, buffer, std::min<u64>(size, utils::sub_saturate<u64>(m_max_size, m_pos)));
		}

		u64 write(const void*, u64) override
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

extern fs::file make_file_view(const fs::file& _file, u64 offset, u64 max_size = umax)
{
	fs::file file;
	file.reset(std::make_unique<file_view>(_file, offset, max_size));
	return file;
}

extern fs::file make_file_view(fs::file&& _file, u64 offset, u64 max_size = umax)
{
	fs::file file;
	file.reset(std::make_unique<file_view>(std::move(_file), offset, max_size));
	return file;
}

extern void ppu_finalize(const ppu_module& info)
{
	if (info.name.empty())
	{
		// Don't remove main module from memory
		return;
	}

	const std::string dev_flash = vfs::get("/dev_flash/sys/");

	if (info.path.starts_with(dev_flash) || Emu.GetCat() == "1P")
	{
		// Don't remove dev_flash prx from memory
		return;
	}

	// Get cache path for this executable
	std::string cache_path = fs::get_cache_dir() + "cache/";

	if (!Emu.GetTitleID().empty())
	{
		cache_path += Emu.GetTitleID();
		cache_path += '/';
	}

	// Add PPU hash and filename
	fmt::append(cache_path, "ppu-%s-%s/", fmt::base57(info.sha1), info.path.substr(info.path.find_last_of('/') + 1));

#ifdef LLVM_AVAILABLE
	g_fxo->get<jit_module_manager>().remove(cache_path + "_" + std::to_string(std::bit_cast<usz>(info.segs[0].ptr)));
#endif
}

extern void ppu_precompile(std::vector<std::string>& dir_queue, std::vector<ppu_module*>* loaded_modules)
{
	if (g_cfg.core.ppu_decoder != ppu_decoder_type::llvm)
	{
		return;
	}

	if (auto dis = g_fxo->try_get<disable_precomp_t>(); dis && dis->disable)
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

	struct file_info
	{
		std::string path;
		u64 offset;
		u64 file_size;

		file_info() noexcept = default;

		file_info(std::string _path, u64 offs, u64 size) noexcept
			: path(std::move(_path))
			, offset(offs)
			, file_size(size)
		{
		}
	};

	std::vector<file_info> file_queue;
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

			// SCE header size
			if (entry.size <= 0x20)
			{
				continue;
			}

			std::string upper = fmt::to_upper(entry.name);

			// Skip already loaded modules or HLEd ones
			auto is_ignored = [&](s64 /*offset*/) -> bool
			{
				if (dir_queue[i] != firmware_sprx_path)
				{
					return false;
				}

				if (loaded_modules)
				{
					if (std::any_of(loaded_modules->begin(), loaded_modules->end(), [&](ppu_module* obj)
					{
						return obj->name == entry.name;
					}))
					{
						return true;
					}
				}

				if (g_cfg.core.libraries_control.get_set().count(entry.name + ":lle"))
				{
					// Force LLE
					return false;
				}
				else if (g_cfg.core.libraries_control.get_set().count(entry.name + ":hle"))
				{
					// Force HLE
					return true;
				}

				extern const std::map<std::string_view, int> g_prx_list;

				// Use list
				return g_prx_list.count(entry.name) && ::at32(g_prx_list, entry.name) != 0;
			};

			// Check PRX filename
			if (upper.ends_with(".PRX") || (upper.ends_with(".SPRX") && entry.name != "libfs_utility_init.sprx"sv))
			{
				if (is_ignored(0))
				{
					continue;
				}

				// Get full path
				file_queue.emplace_back(dir_queue[i] + entry.name, 0, entry.size);
				continue;
			}

			// Check ELF filename
			if ((upper.ends_with(".ELF") || upper.ends_with(".SELF")) && Emu.GetBoot() != dir_queue[i] + entry.name)
			{
				// Get full path
				file_queue.emplace_back(dir_queue[i] + entry.name, 0, entry.size);
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
									file_queue.emplace_back(dir_queue[i] + entry.name, rec.off, rec.size);
									continue;
								}

								if (upper.ends_with(".SELF"))
								{
									// .self inside .mself found
									file_queue.emplace_back(dir_queue[i] + entry.name, rec.off, rec.size);
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

	g_progr_ftotal += file_queue.size();

	u64 total_files_size = 0;

	for (const file_info& info : file_queue)
	{
		total_files_size += info.file_size;
	}

	g_progr_ftotal_bits += total_files_size;

	scoped_progress_dialog progr = "Compiling PPU modules...";

	atomic_t<usz> fnext = 0;

	lf_queue<file_info> possible_exec_file_paths;

	::semaphore<2> ovl_sema;

	named_thread_group workers("SPRX Worker ", std::min<u32>(utils::get_thread_count(), ::size32(file_queue)), [&]
	{
#ifdef __APPLE__
		pthread_jit_write_protect_np(false);
#endif
		// Set low priority
		thread_ctrl::scoped_priority low_prio(-1);

		for (usz func_i = fnext++, inc_fdone = 1; func_i < file_queue.size(); func_i = fnext++, g_progr_fdone += std::exchange(inc_fdone, 1))
		{
			if (Emu.IsStopped())
			{
				continue;
			}

			auto& [path, offset, file_size] = file_queue[func_i];

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
				src = make_file_view(std::move(src), offset);

				// Adjust path for MSELF too
				fmt::append(path, "_x%x", off);
			}

			// Some files may fail to decrypt due to the lack of klic
			src = decrypt_self(std::move(src));

			if (!src)
			{
				ppu_log.notice("Failed to decrypt '%s'", path);
				continue;
			}

			elf_error prx_err{}, ovl_err{};

			if (ppu_prx_object obj = src; (prx_err = obj, obj == elf_error::ok))
			{
				if (auto prx = ppu_load_prx(obj, true, path, offset))
				{
					obj.clear(), src.close(); // Clear decrypted file and elf object memory
					ppu_initialize(*prx, false, file_size);
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
					// Try not to process too many files at once because it seems to reduce performance
					// Concurrently compiling more OVL files does not have much theoretical benefit
					std::lock_guard lock(ovl_sema);

					if (Emu.IsStopped())
					{
						break;
					}

					const auto [ovlm, error] = ppu_load_overlay(obj, true, path, offset);

					if (error)
					{
						if (error == CELL_CANCEL + 0u)
						{
							// Emulation stopped
							break;
						}

						// Abort
						ovl_err = elf_error::header_type;
						break;
					}

					// Participate in thread execution limitation (takes a long time)
					if (std::lock_guard lock(g_fxo->get<jit_core_allocator>().sem); !ovlm->analyse(0, ovlm->entry, ovlm->seg0_code_end, ovlm->applied_patches, []()
					{
						return Emu.IsStopped();
					}))
					{
						// Emulation stopped
						break;
					}

					obj.clear(), src.close(); // Clear decrypted file and elf object memory
					ppu_initialize(*ovlm, false, file_size);
					ppu_finalize(*ovlm);
					break;
				}

				if (ovl_err == elf_error::ok)
				{
					continue;
				}
			}

			ppu_log.notice("Failed to precompile '%s' (prx: %s, ovl: %s): Attempting tratment as executable file", path, prx_err, ovl_err);
			possible_exec_file_paths.push(path, offset, file_size);
			inc_fdone = 0;
		}
	});

	// Join every thread
	workers.join();

	named_thread exec_worker("PPU Exec Worker", [&]
	{
		if (!possible_exec_file_paths)
		{
			return;
		}

#ifdef __APPLE__
		pthread_jit_write_protect_np(false);
#endif
		// Set low priority
		thread_ctrl::scoped_priority low_prio(-1);

		auto slice = possible_exec_file_paths.pop_all();

		auto main_module = std::move(g_fxo->get<main_ppu_module>());

		for (; slice; slice.pop_front(), g_progr_fdone++)
		{
			if (Emu.IsStopped())
			{
				continue;
			}

			const auto& [path, _, file_size] = *slice;

			ppu_log.notice("Trying to load as executable: %s", path);

			// Load SELF
			fs::file src{path};

			if (!src)
			{
				ppu_log.error("Failed to open '%s' (%s)", path, fs::g_tls_error);
				continue;
			}

			// Some files may fail to decrypt due to the lack of klic
			src = decrypt_self(std::move(src), nullptr, nullptr, true);

			if (!src)
			{
				ppu_log.notice("Failed to decrypt '%s'", path);
				continue;
			}

			elf_error exec_err{};

			if (ppu_exec_object obj = src; (exec_err = obj, obj == elf_error::ok))
			{
				while (exec_err == elf_error::ok)
				{
					main_ppu_module& _main = g_fxo->get<main_ppu_module>();
					_main = {};

					auto current_cache = std::move(g_fxo->get<spu_cache>());

					if (!ppu_load_exec(obj, true, path))
					{
						// Abort
						exec_err = elf_error::header_type;
						break;
					}

					if (std::memcmp(main_module.sha1, _main.sha1, sizeof(_main.sha1)) == 0)
					{
						g_fxo->get<spu_cache>() = std::move(current_cache);
						break;
					}

					if (!_main.analyse(0, _main.elf_entry, _main.seg0_code_end, _main.applied_patches, [](){ return Emu.IsStopped(); }))
					{
						g_fxo->get<spu_cache>() = std::move(current_cache);
						break;
					}

					obj.clear(), src.close(); // Clear decrypted file and elf object memory

					_main.name = ' '; // Make ppu_finalize work
					Emu.ConfigurePPUCache(!Emu.IsPathInsideDir(_main.path, g_cfg_vfs.get_dev_flash()));
					ppu_initialize(_main, false, file_size);
					spu_cache::initialize(false);
					ppu_finalize(_main);
					_main = {};
					g_fxo->get<spu_cache>() = std::move(current_cache);
					break;
				}

				if (exec_err == elf_error::ok)
				{
					continue;
				}
			}

			ppu_log.notice("Failed to precompile '%s' as executable (%s)", path, exec_err);
		}

		g_fxo->get<main_ppu_module>() = std::move(main_module);
		g_fxo->get<spu_cache>().collect_funcs_to_precompile = true;
		Emu.ConfigurePPUCache();
	});

	exec_worker();
}

extern void ppu_initialize()
{
	if (!g_fxo->is_init<main_ppu_module>())
	{
		return;
	}

	if (Emu.IsStopped())
	{
		return;
	}

	auto& _main = g_fxo->get<main_ppu_module>();

	scoped_progress_dialog progr = "Analyzing PPU Executable...";

	// Analyse executable
	if (!_main.analyse(0, _main.elf_entry, _main.seg0_code_end, _main.applied_patches, [](){ return Emu.IsStopped(); }))
	{
		return;
	}

	// Validate analyser results (not required)
	_main.validate(0);

	progr = "Scanning PPU Modules...";

	bool compile_main = false;

	// Check main module cache
	if (!_main.segs.empty())
	{
		compile_main = ppu_initialize(_main, true);
	}

	std::vector<ppu_module*> module_list;

	const std::string firmware_sprx_path = vfs::get("/dev_flash/sys/external/");

	// If empty we have no indication for firmware cache state, check everything
	bool compile_fw = !Emu.IsVsh();

	idm::select<lv2_obj, lv2_prx>([&](u32, lv2_prx& _module)
	{
		if (_module.funcs.empty())
		{
			return;
		}

		if (_module.path.starts_with(firmware_sprx_path))
		{
			// Postpone testing
			compile_fw = false;
		}

		module_list.emplace_back(&_module);
	});

	idm::select<lv2_obj, lv2_overlay>([&](u32, lv2_overlay& _module)
	{
		module_list.emplace_back(&_module);
	});

	// Check preloaded libraries cache
	if (!compile_fw)
	{
		for (auto ptr : module_list)
		{
			if (ptr->path.starts_with(firmware_sprx_path))
			{
				compile_fw |= ppu_initialize(*ptr, true);

				// Fixup for compatibility with old savestates
				if (Emu.DeserialManager() && ptr->name == "liblv2.sprx")
				{
					static_cast<lv2_prx*>(ptr)->state = PRX_STATE_STARTED;
					static_cast<lv2_prx*>(ptr)->load_exports();
				}
			}
		}
	}

	std::vector<std::string> dir_queue;

	const std::string mount_point = vfs::get("/dev_flash/");

	bool dev_flash_located = !Emu.GetCat().ends_with('P') && Emu.IsPathInsideDir(Emu.GetBoot(), mount_point) && g_cfg.core.llvm_precompilation;

	if (compile_fw || dev_flash_located)
	{
		if (dev_flash_located)
		{
			const std::string eseibrd = mount_point + "/vsh/module/eseibrd.sprx";

			if (auto prx = ppu_load_prx(ppu_prx_object{decrypt_self(fs::file{eseibrd})}, true, eseibrd, 0))
			{
				// Check if cache exists for this infinitesimally small prx
				dev_flash_located = ppu_initialize(*prx, true);
			}
		}

		const std::string firmware_sprx_path = vfs::get(dev_flash_located ? "/dev_flash/"sv : "/dev_flash/sys/"sv);
		dir_queue.emplace_back(firmware_sprx_path);
	}

	// Avoid compilation if main's cache exists or it is a standalone SELF with no PARAM.SFO
	if (compile_main && g_cfg.core.llvm_precompilation && !Emu.GetTitleID().empty() && !Emu.IsChildProcess())
	{
		// Try to add all related directories
		const std::set<std::string> dirs = Emu.GetGameDirs();
		dir_queue.insert(std::end(dir_queue), std::begin(dirs), std::end(dirs));
	}

	ppu_precompile(dir_queue, &module_list);

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
	for (auto ptr : module_list)
	{
		if (Emu.IsStopped())
		{
			return;
		}

		ppu_initialize(*ptr);
	}
}

bool ppu_initialize(const ppu_module& info, bool check_only, u64 file_size)
{
	if (g_cfg.core.ppu_decoder != ppu_decoder_type::llvm)
	{
		if (check_only || vm::base(info.segs[0].addr) != info.segs[0].ptr)
		{
			return false;
		}

		auto& toc_manager = g_fxo->get<ppu_toc_manager>();

		std::lock_guard lock(toc_manager.mutex);

		auto& ppu_toc = toc_manager.toc_map;

		for (const auto& func : info.funcs)
		{
			for (auto& block : func.blocks)
			{
				if (g_fxo->is_init<ppu_far_jumps_t>() && !g_fxo->get<ppu_far_jumps_t>().get_targets(block.first, block.second).empty())
				{
					// Replace the block with ppu_far_jump
					continue;
				}

				ppu_register_function_at(block.first, block.second);
			}

			if (g_cfg.core.ppu_debug && func.size && func.toc != umax && !ppu_get_far_jump(func.addr))
			{
				ppu_toc[func.addr] = func.toc;
				ppu_ref(func.addr) = &ppu_check_toc;
			}
		}

		return false;
	}

	// Link table
	static const std::unordered_map<std::string, u64> s_link_table = []()
	{
		std::unordered_map<std::string, u64> link_table
		{
			{ "sys_game_set_system_sw_version", reinterpret_cast<u64>(ppu_execute_syscall) },
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
			{ "__dcbz", reinterpret_cast<u64>(+[](u32 addr){ alignas(64) static constexpr u8 z[128]{}; do_cell_atomic_128_store(addr, z); }) },
			{ "__resupdate", reinterpret_cast<u64>(vm::reservation_update) },
			{ "__resinterp", reinterpret_cast<u64>(ppu_reservation_fallback) },
			{ "__escape", reinterpret_cast<u64>(+ppu_escape) },
			{ "__read_maybe_mmio32", reinterpret_cast<u64>(+ppu_read_mmio_aware_u32) },
			{ "__write_maybe_mmio32", reinterpret_cast<u64>(+ppu_write_mmio_aware_u32) },
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

	if (!info.cache.empty())
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
	std::optional<scoped_progress_dialog> progr;

	if (!check_only)
	{
		// Initialize progress dialog
		progr.emplace("Loading PPU modules...");
	}

	// Permanently loaded compiled PPU modules (name -> data)
	jit_module& jit_mod = g_fxo->get<jit_module_manager>().get(cache_path + "_" + std::to_string(std::bit_cast<usz>(info.segs[0].ptr)));

	// Compiler instance (deferred initialization)
	std::shared_ptr<jit_compiler>& jit = jit_mod.pjit;

	// Split module into fragments <= 1 MiB
	usz fpos = 0;

	// Difference between function name and current location
	const u32 reloc = info.relocs.empty() ? 0 : ::at32(info.segs, 0).addr;

	// Info sent to threads
	std::vector<std::pair<std::string, ppu_module>> workload;

	// Info to load to main JIT instance (true - compiled)
	std::vector<std::pair<std::string, bool>> link_workload;

	// Sync variable to acquire workloads
	atomic_t<u32> work_cv = 0;

	bool compiled_new = false;

	bool has_mfvscr = false;

	const bool is_being_used_in_emulation = vm::base(info.segs[0].addr) == info.segs[0].ptr;

	const cpu_thread* cpu = cpu_thread::get_current();

	for (auto& func : info.funcs)
	{
		if (func.size == 0)
		{
			continue;
		}

		for (const auto& [addr, size] : func.blocks)
		{
			if (size == 0)
			{
				continue;
			}

			auto i_ptr = ensure(info.get_ptr<u32>(addr));

			for (u32 i = addr; i < addr + size; i += 4, i_ptr++)
			{
				if (g_ppu_itype.decode(*i_ptr) == ppu_itype::MFVSCR)
				{
					ppu_log.warning("MFVSCR found");
					has_mfvscr = true;
					break;
				}
			}

			if (has_mfvscr)
			{
				break;
			}
		}

		if (has_mfvscr)
		{
			break;
		}
	}

	u32 total_compile = 0;

	while (!jit_mod.init && fpos < info.funcs.size())
	{
		// Initialize compiler instance
		if (!jit && is_being_used_in_emulation)
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

			if (g_fxo->is_init<ppu_far_jumps_t>())
			{
				auto targets = g_fxo->get<ppu_far_jumps_t>().get_targets(func.addr, func.size);

				for (auto [source, target] : targets)
				{
					auto far_jump = ensure(g_fxo->get<ppu_far_jumps_t>().gen_jump(source));

					if (source == func.addr && jit)
					{
						jit->update_global_mapping(fmt::format("__0x%x", func.addr - reloc), reinterpret_cast<u64>(far_jump));
					}

					ppu_register_function_at(source, 4, far_jump);
				}

				if (!targets.empty())
				{
					// Replace the function with ppu_far_jump
					fpos++;
					continue;
				}
			}

			// Copy block or function entry
			ppu_function& entry = part.funcs.emplace_back(func);

			// Fixup some information
			entry.name = fmt::format("__0x%x", entry.addr - reloc);

			if (has_mfvscr && g_cfg.core.ppu_set_sat_bit)
			{
				// TODO
				entry.attr += ppu_attr::has_mfvscr;
			}

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
							sha1_update(&ctx, ensure(info.get_ptr<const u8>(addr)), roff - addr);
						}

						// Hash relocation type instead
						const be_t<u32> type = low->type;
						sha1_update(&ctx, reinterpret_cast<const u8*>(&type), sizeof(type));

						// Set the next addr
						addr = roff + 4;
					}

					if (has_dcbz == 1)
					{
						auto i_ptr = ensure(info.get_ptr<u32>(addr));

						for (u32 i = addr, end = block.second + block.first - 1; i <= end; i += 4, i_ptr++)
						{
							if (g_ppu_itype.decode(*i_ptr) == ppu_itype::DCBZ)
							{
								has_dcbz = 2;
								break;
							}
						}
					}

					// Hash from addr to the end of the block
					sha1_update(&ctx, ensure(info.get_ptr<const u8>(addr)), block.second - (addr - block.first));
				}

				if (reloc)
				{
					continue;
				}

				if (has_dcbz == 1)
				{
					auto i_ptr = ensure(info.get_ptr<u32>(func.addr));

					for (u32 i = func.addr, end = func.addr + func.size - 1; i <= end; i += 4, i_ptr++)
					{
						if (g_ppu_itype.decode(*i_ptr) == ppu_itype::DCBZ)
						{
							has_dcbz = 2;
							break;
						}
					}
				}

				sha1_update(&ctx, ensure(info.get_ptr<const u8>(func.addr)), func.size);
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
				platform_bit,
				accurate_dfma,
				fixup_vnan,
				fixup_nj_denormals,
				accurate_cache_line_stores,
				reservations_128_byte,
				greedy_mode,
				accurate_sat,
				accurate_fpcc,
				accurate_vnan,
				accurate_nj_mode,

				__bitset_enum_max
			};

			be_t<bs_t<ppu_settings>> settings{};

#if !defined(_WIN32) && !defined(__APPLE__)
			settings += ppu_settings::platform_bit;
#endif
			if (g_cfg.core.use_accurate_dfma)
				settings += ppu_settings::accurate_dfma;
			if (g_cfg.core.ppu_fix_vnan)
				settings += ppu_settings::fixup_vnan;
			if (g_cfg.core.ppu_llvm_nj_fixup)
				settings += ppu_settings::fixup_nj_denormals;
			if (has_dcbz == 2)
				settings += ppu_settings::accurate_cache_line_stores;
			if (g_cfg.core.ppu_128_reservations_loop_max_length)
				settings += ppu_settings::reservations_128_byte;
			if (g_cfg.core.ppu_llvm_greedy_mode)
				settings += ppu_settings::greedy_mode;
			if (has_mfvscr && g_cfg.core.ppu_set_sat_bit)
				settings += ppu_settings::accurate_sat;
			if (g_cfg.core.ppu_set_fpcc)
				settings += ppu_settings::accurate_fpcc, fmt::throw_exception("FPCC Not implemented");
			if (g_cfg.core.ppu_set_vnan)
				settings += ppu_settings::accurate_vnan, settings -= ppu_settings::fixup_vnan, fmt::throw_exception("VNAN Not implemented");
			if (g_cfg.core.ppu_use_nj_bit)
				settings += ppu_settings::accurate_nj_mode, settings -= ppu_settings::fixup_nj_denormals, fmt::throw_exception("NJ Not implemented");

			// Write version, hash, CPU, settings
			fmt::append(obj_name, "v6-kusa-%s-%s-%s.obj", fmt::base57(output, 16), fmt::base57(settings), jit_compiler::cpu(g_cfg.core.llvm_cpu));
		}

		if (cpu ? cpu->state.all_of(cpu_flag::exit) : Emu.IsStopped())
		{
			break;
		}

		if (!check_only)
		{
			total_compile++;

			link_workload.emplace_back(obj_name, false);
		}

		// Check object file
		if (jit_compiler::check(cache_path + obj_name))
		{
			if (!jit && !check_only)
			{
				ppu_log.success("LLVM: Module exists: %s", obj_name);

				// Done already, revert total amount increase
				// Avoid incrementing "pdone" instead because it creates false appreciation for both the progress dialog and the user
				total_compile--;
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
	}

	if (check_only)
	{
		return false;
	}

	// Update progress dialog
	if (total_compile)
	{
		g_progr_ptotal += total_compile;
	}

	if (g_progr_ftotal_bits && file_size)
	{
		g_progr_fknown_bits += file_size;
	}

	// Create worker threads for compilation
	if (!workload.empty())
	{
		*progr = "Compiling PPU modules...";

		u32 thread_count = rpcs3::utils::get_max_threads();

		if (workload.size() < thread_count)
		{
			thread_count = ::size32(workload);
		}

		struct thread_index_allocator
		{
			atomic_t<u64> index = 0;
		};

		struct thread_op
		{
			atomic_t<u32>& work_cv;
			std::vector<std::pair<std::string, ppu_module>>& workload;
			const std::string& cache_path;
			const cpu_thread* cpu;

			std::unique_lock<decltype(jit_core_allocator::sem)> core_lock;

			thread_op(atomic_t<u32>& work_cv, std::vector<std::pair<std::string, ppu_module>>& workload
				, const cpu_thread* cpu, const std::string& cache_path, decltype(jit_core_allocator::sem)& sem) noexcept

				: work_cv(work_cv)
				, workload(workload)
				, cache_path(cache_path)
				, cpu(cpu)
			{
				// Save mutex
				core_lock = std::unique_lock{sem, std::defer_lock};
			}

			thread_op(const thread_op& other) noexcept
				: work_cv(other.work_cv)
				, workload(other.workload)
				, cache_path(other.cache_path)
				, cpu(other.cpu)
			{
				if (auto mtx = other.core_lock.mutex())
				{
					// Save mutex
					core_lock = std::unique_lock{*mtx, std::defer_lock};
				}
			}

			thread_op(thread_op&& other) noexcept = default;

			void operator()()
			{
				// Set low priority
				thread_ctrl::scoped_priority low_prio(-1);

	#ifdef __APPLE__
				pthread_jit_write_protect_np(false);
	#endif
				for (u32 i = work_cv++; i < workload.size(); i = work_cv++, g_progr_pdone++)
				{
					if (cpu ? cpu->state.all_of(cpu_flag::exit) : Emu.IsStopped())
					{
						continue;
					}

					// Keep allocating workload
					const auto& [obj_name, part] = std::as_const(workload)[i];

					ppu_log.warning("LLVM: Compiling module %s%s", cache_path, obj_name);

					// Use another JIT instance
					jit_compiler jit2({}, g_cfg.core.llvm_cpu, 0x1);
					ppu_initialize2(jit2, part, cache_path, obj_name);

					ppu_log.success("LLVM: Compiled module %s", obj_name);
				}

				core_lock.unlock();
			}
		};

		// Prevent watchdog thread from terminating
		g_watchdog_hold_ctr++;

		named_thread_group threads(fmt::format("PPUW.%u.", ++g_fxo->get<thread_index_allocator>().index), thread_count
			, thread_op(work_cv, workload, cpu, cache_path, g_fxo->get<jit_core_allocator>().sem)
			, [&](u32 /*thread_index*/, thread_op& op)
		{
			// Allocate "core"
			op.core_lock.lock();

			// Second check before creating another thread
			return work_cv < workload.size() && (cpu ? !cpu->state.all_of(cpu_flag::exit) : !Emu.IsStopped());
		});

		threads.join();

		g_watchdog_hold_ctr--;
	}

	{
		if (!is_being_used_in_emulation || (cpu ? cpu->state.all_of(cpu_flag::exit) : Emu.IsStopped()))
		{
			return compiled_new;
		}

		if (workload.size() < link_workload.size())
		{
			// Only show this message if this task is relevant
			*progr = "Linking PPU modules...";
		}

		for (const auto& [obj_name, is_compiled] : link_workload)
		{
			if (cpu ? cpu->state.all_of(cpu_flag::exit) : Emu.IsStopped())
			{
				break;
			}

			jit->add(cache_path + obj_name);

			if (!is_compiled)
			{
				ppu_log.success("LLVM: Loaded module %s", obj_name);
				g_progr_pdone++;
			}
		}
	}

	if (!is_being_used_in_emulation || (cpu ? cpu->state.all_of(cpu_flag::exit) : Emu.IsStopped()))
	{
		return compiled_new;
	}

	// Jit can be null if the loop doesn't ever enter.
#ifdef __APPLE__
	pthread_jit_write_protect_np(false);
#endif
	// Try to patch all single and unregistered BLRs with the same function (TODO: Maybe generalize it into PIC code detection and patching)
	ppu_intrp_func_t BLR_func = nullptr;

	if (jit && !jit_mod.init)
	{
		jit->fin();

		// Get and install function addresses
		for (const auto& func : info.funcs)
		{
			if (!func.size) continue;

			const auto name = fmt::format("__0x%x", func.addr - reloc);
			const auto addr = ensure(reinterpret_cast<ppu_intrp_func_t>(jit->get(name)));
			jit_mod.funcs.emplace_back(addr);

			if (func.size == 4 && !BLR_func && *info.get_ptr<u32>(func.addr) == ppu_instructions::BLR())
			{
				BLR_func = addr;
			}

			ppu_register_function_at(func.addr, 4, addr);

			if (g_cfg.core.ppu_debug)
				ppu_log.trace("Installing function %s at 0x%x: %p (reloc = 0x%x)", name, func.addr, ppu_ref(func.addr), reloc);
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

			const u64 addr = reinterpret_cast<uptr>(ensure(jit_mod.funcs[index++]));

			if (func.size == 4 && !BLR_func && *info.get_ptr<u32>(func.addr) == ppu_instructions::BLR())
			{
				BLR_func = reinterpret_cast<ppu_intrp_func_t>(addr);
			}

			ppu_register_function_at(func.addr, 4, addr);

			if (g_cfg.core.ppu_debug)
				ppu_log.trace("Reinstalling function at 0x%x: %p (reloc=0x%x)", func.addr, ppu_ref(func.addr), reloc);
		}

		index = 0;
	}

	if (BLR_func)
	{
		auto inst_ptr = info.get_ptr<u32>(info.segs[0].addr);

		for (u32 addr = info.segs[0].addr; addr < info.segs[0].addr + info.segs[0].size; addr += 4, inst_ptr++)
		{
			if (*inst_ptr == ppu_instructions::BLR() && (reinterpret_cast<uptr>(ppu_ref(addr)) << 16 >> 16) == reinterpret_cast<uptr>(ppu_recompiler_fallback_ghc))
			{
				ppu_register_function_at(addr, 4, BLR_func);
			}
		}
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
	_module->setTargetTriple(jit_compiler::triple1());
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
			f->addParamAttr(1, llvm::Attribute::NoAlias);
			f->addFnAttr(Attribute::NoUnwind);
		}
	}

	{
		if (g_cfg.core.ppu_debug)
		{
			translator.build_interpreter();
		}

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
		//pm.add(createDeadStoreEliminationPass());
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
			Emu.CallFromMainThread([]{ Emu.GracefulShutdown(false, true); });
			return;
		}

		ppu_log.notice("LLVM: %zu functions generated", _module->getFunctionList().size());
	}

	// Load or compile module
	jit.add(std::move(_module), cache_path);
#endif // LLVM_AVAILABLE
}
