#include "stdafx.h"
#include "SPURecompiler.h"

#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/system_progress.hpp"
#include "Emu/system_utils.hpp"
#include "Emu/cache_utils.hpp"
#include "Emu/IdManager.h"
#include "Crypto/sha1.h"
#include "Utilities/StrUtil.h"
#include "Utilities/JIT.h"
#include "util/init_mutex.hpp"
#include "util/shared_ptr.hpp"

#include "SPUThread.h"
#include "SPUAnalyser.h"
#include "SPUInterpreter.h"
#include "SPUDisAsm.h"
#include <algorithm>
#include <optional>
#include <unordered_set>

#include "util/v128.hpp"
#include "util/simd.hpp"
#include "util/sysinfo.hpp"

const extern spu_decoder<spu_itype> g_spu_itype;
const extern spu_decoder<spu_iname> g_spu_iname;
const extern spu_decoder<spu_iflag> g_spu_iflag;

// Move 4 args for calling native function from a GHC calling convention function
#if defined(ARCH_X64)
static u8* move_args_ghc_to_native(u8* raw)
{
#ifdef _WIN32
	// mov  rcx, r13
	// mov  rdx, rbp
	// mov  r8,  r12
	// mov  r9,  rbx
	std::memcpy(raw, "\x4C\x89\xE9\x48\x89\xEA\x4D\x89\xE0\x49\x89\xD9", 12);
#else
	// mov  rdi, r13
	// mov  rsi, rbp
	// mov  rdx, r12
	// mov  rcx, rbx
	std::memcpy(raw, "\x4C\x89\xEF\x48\x89\xEE\x4C\x89\xE2\x48\x89\xD9", 12);
#endif

	return raw + 12;
}
#elif defined(ARCH_ARM64)
static void ghc_cpp_trampoline(u64 fn_target, native_asm& c, auto& args)
{
	using namespace asmjit;

	Label target = c.newLabel();
	c.mov(args[0], a64::x19);
	c.mov(args[1], a64::x20);
	c.mov(args[2], a64::x21);
	c.mov(args[3], a64::x22);

	c.ldr(a64::x15, arm::Mem(target));
	c.br(a64::x15);

	c.brk(Imm(0x42)); // Unreachable

	c.bind(target);
	c.embedUInt64(fn_target);
}
#endif

DECLARE(spu_runtime::tr_dispatch) = []
{
#ifdef __APPLE__
	pthread_jit_write_protect_np(false);
#endif
#if defined(ARCH_X64)
	// Generate a special trampoline to spu_recompiler_base::dispatch with pause instruction
	u8* const trptr = jit_runtime::alloc(32, 16);
	u8* raw = move_args_ghc_to_native(trptr);
	*raw++ = 0xf3; // pause
	*raw++ = 0x90;
	*raw++ = 0xff; // jmp [rip]
	*raw++ = 0x25;
	std::memset(raw, 0, 4);
	const u64 target = reinterpret_cast<u64>(&spu_recompiler_base::dispatch);
	std::memcpy(raw + 4, &target, 8);
	return reinterpret_cast<spu_function_t>(trptr);
#elif defined(ARCH_ARM64)
	auto trptr = build_function_asm<spu_function_t>("tr_dispatch",
		[](native_asm& c, auto& args)
		{
			c.yield();
			ghc_cpp_trampoline(reinterpret_cast<u64>(&spu_recompiler_base::dispatch), c, args);

			c.embed("tr_dispatch", 11);
		});
	return trptr;
#else
#error "Unimplemented"
#endif
}();

DECLARE(spu_runtime::tr_branch) = []
{
#if defined(ARCH_X64)
	// Generate a trampoline to spu_recompiler_base::branch
	u8* const trptr = jit_runtime::alloc(32, 16);
	u8* raw = move_args_ghc_to_native(trptr);
	*raw++ = 0xff; // jmp [rip]
	*raw++ = 0x25;
	std::memset(raw, 0, 4);
	const u64 target = reinterpret_cast<u64>(&spu_recompiler_base::branch);
	std::memcpy(raw + 4, &target, 8);
	return reinterpret_cast<spu_function_t>(trptr);

#elif defined(ARCH_ARM64)
	auto trptr = build_function_asm<spu_function_t>("tr_branch",
		[](native_asm& c, auto& args)
		{
			ghc_cpp_trampoline(reinterpret_cast<u64>(&spu_recompiler_base::branch), c, args);

			c.embed("tr_branch", 9);
		});
	return trptr;
#else
#error "Unimplemented"
#endif
}();

DECLARE(spu_runtime::tr_interpreter) = []
{
#if defined(ARCH_X64)
	u8* const trptr = jit_runtime::alloc(32, 16);
	u8* raw = move_args_ghc_to_native(trptr);
	*raw++ = 0xff; // jmp [rip]
	*raw++ = 0x25;
	std::memset(raw, 0, 4);
	const u64 target = reinterpret_cast<u64>(&spu_recompiler_base::old_interpreter);
	std::memcpy(raw + 4, &target, 8);
	return reinterpret_cast<spu_function_t>(trptr);
#elif defined(ARCH_ARM64)
	auto trptr = build_function_asm<spu_function_t>("tr_interpreter",
		[](native_asm& c, auto& args)
		{
			ghc_cpp_trampoline(reinterpret_cast<u64>(&spu_recompiler_base::old_interpreter), c, args);

			c.embed("tr_interpreter", 14);
		});
	return trptr;
#endif
}();

DECLARE(spu_runtime::g_dispatcher) = []
{
	// Allocate 2^20 positions in data area
	const auto ptr = reinterpret_cast<std::remove_const_t<decltype(spu_runtime::g_dispatcher)>>(jit_runtime::alloc(sizeof(*g_dispatcher), 64, false));

	for (auto& x : *ptr)
	{
		x.raw() = tr_dispatch;
	}

	return ptr;
}();

DECLARE(spu_runtime::tr_all) = []
{
#if defined(ARCH_X64)
	u8* const trptr = jit_runtime::alloc(32, 16);
	u8* raw = trptr;

	// Load PC: mov eax, [r13 + spu_thread::pc]
	*raw++ = 0x41;
	*raw++ = 0x8b;
	*raw++ = 0x45;
	*raw++ = ::narrow<s8>(::offset32(&spu_thread::pc));

	// Get LS address starting from PC: lea rcx, [rbp + rax]
	*raw++ = 0x48;
	*raw++ = 0x8d;
	*raw++ = 0x4c;
	*raw++ = 0x05;
	*raw++ = 0x00;

	// mov eax, [rcx]
	*raw++ = 0x8b;
	*raw++ = 0x01;

	// shr eax, (32 - 20)
	*raw++ = 0xc1;
	*raw++ = 0xe8;
	*raw++ = 0x0c;

	// Load g_dispatcher to rdx
	*raw++ = 0x48;
	*raw++ = 0x8d;
	*raw++ = 0x15;
	const s32 r32 = ::narrow<s32>(reinterpret_cast<u64>(g_dispatcher) - reinterpret_cast<u64>(raw) - 4);
	std::memcpy(raw, &r32, 4);
	raw += 4;

	// Update block_hash (set zero): mov [r13 + spu_thread::m_block_hash], 0
	*raw++ = 0x49;
	*raw++ = 0xc7;
	*raw++ = 0x45;
	*raw++ = ::narrow<s8>(::offset32(&spu_thread::block_hash));
	*raw++ = 0x00;
	*raw++ = 0x00;
	*raw++ = 0x00;
	*raw++ = 0x00;

	// jmp [rdx + rax * 8]
	*raw++ = 0xff;
	*raw++ = 0x24;
	*raw++ = 0xc2;

	return reinterpret_cast<spu_function_t>(trptr);

#elif defined(ARCH_ARM64)
	auto trptr = build_function_asm<spu_function_t>("tr_all",
		[](native_asm& c, auto& args)
		{
			using namespace asmjit;

			// w1: PC (eax in x86 SPU)
			// x7: lsa (rcx in x86 SPU)

			// Load PC
			Label pc_offset = c.newLabel();
			c.ldr(a64::x0, arm::Mem(pc_offset));
			c.ldr(a64::w1, arm::Mem(a64::x19, a64::x0)); // REG_Base + offset(spu_thread::pc)
			// Compute LS address = REG_Sp + PC, store into x7 (use later)
			c.add(a64::x7, a64::x20, a64::x1);
			// Load 32b from LS address
			c.ldr(a64::w3, arm::Mem(a64::x7));
			// shr (32 - 20)
			c.lsr(a64::w3, a64::w3, Imm(32 - 20));
			// Load g_dispatcher
			Label g_dispatcher_offset = c.newLabel();
			c.ldr(a64::x4, arm::Mem(g_dispatcher_offset));
			// Update block hash
			Label block_hash_offset = c.newLabel();
			c.mov(a64::x5, Imm(0));
			c.ldr(a64::x6, arm::Mem(block_hash_offset));
			c.str(a64::x5, arm::Mem(a64::x19, a64::x6)); // REG_Base + offset(spu_thread::block_hash)
			// Jump to [g_dispatcher + idx * 8]
			c.mov(a64::x6, Imm(8));
			c.mul(a64::x6, a64::x3, a64::x6);
			c.add(a64::x4, a64::x4, a64::x6);
			c.ldr(a64::x4, arm::Mem(a64::x4));
			c.br(a64::x4);

			c.bind(pc_offset);
			c.embedUInt64(::offset32(&spu_thread::pc));
			c.bind(g_dispatcher_offset);
			c.embedUInt64(reinterpret_cast<u64>(g_dispatcher));
			c.bind(block_hash_offset);
			c.embedUInt64(::offset32(&spu_thread::block_hash));
			c.embed("tr_all", 6);
		});
	return trptr;
#else
#error "Unimplemented"
#endif
}();

DECLARE(spu_runtime::g_gateway) = build_function_asm<spu_function_t>("spu_gateway", [](native_asm& c, auto& args)
{
	// Gateway for SPU dispatcher, converts from native to GHC calling convention, also saves RSP value for spu_escape
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
	c.mov(x86::qword_ptr(args[0], ::offset32(&spu_thread::saved_native_sp)), x86::rsp);

	// Move 4 args (despite spu_function_t def)
	c.mov(x86::r13, args[0]);
	c.mov(x86::rbp, args[1]);
	c.mov(x86::r12, args[2]);
	c.mov(x86::rbx, args[3]);

	if (utils::has_avx())
	{
		c.vzeroupper();
	}

	c.call(spu_runtime::tr_all);

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
#elif defined(ARCH_ARM64)
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

	// Save native stack pointer for longjmp emulation
	Label sp_offset = c.newLabel();
	c.ldr(a64::x26, arm::Mem(sp_offset));
	// sp not allowed to be used in load/stores directly
	c.mov(a64::x15, a64::sp);
	c.str(a64::x15, arm::Mem(args[0], a64::x26));

	// Move 4 args (despite spu_function_t def)
	c.mov(a64::x19, args[0]);
	c.mov(a64::x20, args[1]);
	c.mov(a64::x21, args[2]);
	c.mov(a64::x22, args[3]);

	// Save ret address to stack
	// since non-tail calls to cpp fns may corrupt lr and
	// g_tail_escape may jump out of a fn before the epilogue can restore lr
	Label ret_addr = c.newLabel();
	c.adr(a64::x0, ret_addr);
	c.str(a64::x0, arm::Mem(a64::sp, 104));

	Label call_target = c.newLabel();
	c.ldr(a64::x0, arm::Mem(call_target));
	c.blr(a64::x0);

	c.bind(ret_addr);

	// Restore stack ptr
	c.ldr(a64::x26, arm::Mem(sp_offset));
	c.ldr(a64::x15, arm::Mem(a64::x19, a64::x26));
	c.mov(a64::sp, a64::x15);

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

	c.bind(sp_offset);
	c.embedUInt64(::offset32(&spu_thread::saved_native_sp));
	c.bind(call_target);
	c.embedUInt64(reinterpret_cast<u64>(spu_runtime::tr_all));
	c.embed("spu_gateway", 11);
#else
#error "Unimplemented"
#endif
});

DECLARE(spu_runtime::g_escape) = build_function_asm<void(*)(spu_thread*)>("spu_escape", [](native_asm& c, auto& args)
{
	using namespace asmjit;

#if defined(ARCH_X64)
	// Restore native stack pointer (longjmp emulation)
	c.mov(x86::rsp, x86::qword_ptr(args[0], ::offset32(&spu_thread::saved_native_sp)));

	// Return to the return location
	c.sub(x86::rsp, 8);
	c.ret();
#elif defined(ARCH_ARM64)
	// Restore native stack pointer (longjmp emulation)
	Label sp_offset = c.newLabel();
	c.ldr(a64::x15, arm::Mem(sp_offset));
	c.ldr(a64::x15, arm::Mem(args[0], a64::x15));
	c.mov(a64::sp, a64::x15);

	c.ldr(a64::x30, arm::Mem(a64::sp, 104));
	c.ret(a64::x30);

	c.bind(sp_offset);
	c.embedUInt64(::offset32(&spu_thread::saved_native_sp));

	c.embed("spu_escape", 10);
#else
#error "Unimplemented"
#endif
});

DECLARE(spu_runtime::g_tail_escape) = build_function_asm<void(*)(spu_thread*, spu_function_t, u8*)>("spu_tail_escape", [](native_asm& c, auto& args)
{
	using namespace asmjit;

#if defined(ARCH_X64)
	// Restore native stack pointer (longjmp emulation)
	c.mov(x86::rsp, x86::qword_ptr(args[0], ::offset32(&spu_thread::saved_native_sp)));

	// Adjust stack for initial call instruction in the gateway
	c.sub(x86::rsp, 16);

	// Tail call, GHC CC (second arg)
	c.mov(x86::r13, args[0]);
	c.mov(x86::rbp, x86::qword_ptr(args[0], ::offset32(&spu_thread::ls)));
	c.mov(x86::r12, args[2]);
	c.xor_(x86::ebx, x86::ebx);
	c.mov(x86::qword_ptr(x86::rsp), args[1]);
	c.ret();
#elif defined(ARCH_ARM64)
	// Restore native stack pointer (longjmp emulation)
	Label sp_offset = c.newLabel();
	c.ldr(a64::x15, arm::Mem(sp_offset));
	c.ldr(a64::x15, arm::Mem(args[0], a64::x15));
	c.mov(a64::sp, a64::x15);

	// Reload lr, since it might've been clobbered by a cpp fn
	// and g_tail_escape runs before epilogue
	c.ldr(a64::x30, arm::Mem(a64::sp, 104));

	// Tail call, GHC CC
	c.mov(a64::x19, args[0]); // REG_Base
	Label ls_offset = c.newLabel();
	c.ldr(a64::x20, arm::Mem(ls_offset));
	c.ldr(a64::x20, arm::Mem(args[0], a64::x20)); // REG_Sp
	c.mov(a64::x21, args[2]); // REG_Hp
	c.eor(a64::w22, a64::w22, a64::w22); // REG_R1

	c.br(args[1]);

	c.bind(ls_offset);
	c.embedUInt64(::offset32(&spu_thread::ls));
	c.bind(sp_offset);
	c.embedUInt64(::offset32(&spu_thread::saved_native_sp));

	c.embed("spu_tail_escape", 15);
#else
#error "Unimplemented"
#endif
});

DECLARE(spu_runtime::g_interpreter_table) = {};

DECLARE(spu_runtime::g_interpreter) = nullptr;

spu_cache::spu_cache(const std::string& loc)
	: m_file(loc, fs::read + fs::write + fs::create + fs::append)
{
}

spu_cache::~spu_cache()
{
}

extern void utilize_spu_data_segment(u32 vaddr, const void* ls_data_vaddr, u32 size)
{
	if (vaddr % 4)
	{
		return;
	}

	size &= -4;

	if (!size || vaddr + size > SPU_LS_SIZE)
	{
		return;
	}

	if (!g_cfg.core.llvm_precompilation)
	{
		return;
	}

	g_fxo->need<spu_cache>();

	if (!g_fxo->get<spu_cache>().collect_funcs_to_precompile)
	{
		return;
	}

	std::basic_string<u32> data(size / 4, 0);
	std::memcpy(data.data(), ls_data_vaddr, size);

	spu_cache::precompile_data_t obj{vaddr, std::move(data)};

	obj.funcs = spu_thread::discover_functions(vaddr, { reinterpret_cast<const u8*>(ls_data_vaddr), size }, vaddr != 0, umax);

	if (obj.funcs.empty())
	{
		// Nothing to add
		return;
	}

	for (u32 addr : obj.funcs)
	{
		spu_log.notice("Found SPU function at: 0x%05x", addr);
	}

	spu_log.notice("Found %u SPU functions", obj.funcs.size());

	g_fxo->get<spu_cache>().precompile_funcs.push(std::move(obj));
}

// For SPU cache validity check
static u16 calculate_crc16(const uchar* data, usz length)
{
	u16 crc = umax;

	while (length--)
	{
		u8 x = (crc >> 8) ^ *data++;
		x ^= (x >> 4);
		crc = static_cast<u16>((crc << 8) ^ (x << 12) ^ (x << 5) ^ x);
	}

	return crc;
}

std::deque<spu_program> spu_cache::get()
{
	std::deque<spu_program> result;

	if (!m_file)
	{
		return result;
	}

	m_file.seek(0);

	// TODO: signal truncated or otherwise broken file
	while (true)
	{
		struct block_info_t
		{
			be_t<u16> crc;
			be_t<u16> size;
			be_t<u32> addr;
		} block_info{};

		if (!m_file.read(block_info))
		{
			break;
		}

		const u32 crc = block_info.crc;
		const u32 size = block_info.size;
		const u32 addr = block_info.addr;

		if (utils::add_saturate<u32>(addr, size * 4) > SPU_LS_SIZE)
		{
			break;
		}

		std::vector<u32> func;

		if (!m_file.read(func, size))
		{
			break;
		}

		if (!size || !func[0])
		{
			// Skip old format Giga entries
			continue;
		}

		// CRC check is optional to be compatible with old format
		if (crc && std::max<u32>(calculate_crc16(reinterpret_cast<const uchar*>(func.data()), size * 4), 1) != crc)
		{
			// Invalid, but continue anyway
			continue;
		}

		spu_program res;
		res.entry_point = addr;
		res.lower_bound = addr;
		res.data = std::move(func);
		result.emplace_front(std::move(res));
	}

	return result;
}

void spu_cache::add(const spu_program& func)
{
	if (!m_file)
	{
		return;
	}

	be_t<u32> size = ::size32(func.data);
	be_t<u32> addr = func.entry_point;

	// Add CRC (forced non-zero)
	size |= std::max<u32>(calculate_crc16(reinterpret_cast<const uchar*>(func.data.data()), size * 4), 1) << 16;

	const fs::iovec_clone gather[3]
	{
		{&size, sizeof(size)},
		{&addr, sizeof(addr)},
		{func.data.data(), func.data.size() * 4}
	};

	// Append data
	m_file.write_gather(gather, 3);
}

void spu_cache::initialize(bool build_existing_cache)
{
	spu_runtime::g_interpreter = spu_runtime::g_gateway;

	if (g_cfg.core.spu_decoder == spu_decoder_type::_static || g_cfg.core.spu_decoder == spu_decoder_type::dynamic)
	{
		for (auto& x : *spu_runtime::g_dispatcher)
		{
			x.raw() = spu_runtime::tr_interpreter;
		}
	}

	const std::string ppu_cache = rpcs3::cache::get_ppu_cache();

	if (ppu_cache.empty())
	{
		return;
	}

	// SPU cache file (version + block size type)
	const std::string loc = ppu_cache + "spu-" + fmt::to_lower(g_cfg.core.spu_block_size.to_string()) + "-v1-tane.dat";

	spu_cache cache(loc);

	if (!cache)
	{
		spu_log.error("Failed to initialize SPU cache at: %s", loc);
		return;
	}

	// Read cache
	auto func_list = cache.get();
	atomic_t<usz> fnext{};
	atomic_t<u8> fail_flag{0};

	auto data_list = g_fxo->get<spu_cache>().precompile_funcs.pop_all();
	g_fxo->get<spu_cache>().collect_funcs_to_precompile = false;

	usz total_precompile = 0;

	for (auto& sec : data_list)
	{
		total_precompile += sec.funcs.size();
	}

	const bool spu_precompilation_enabled = func_list.empty() && g_cfg.core.spu_cache && g_cfg.core.llvm_precompilation;

	if (spu_precompilation_enabled)
	{
		// What compiles in this case goes straight to disk
		g_fxo->get<spu_cache>() = std::move(cache);
	}
	else if (!build_existing_cache)
	{
		return;
	}
	else
	{
		total_precompile = 0;
		data_list = {};
	}

	atomic_t<usz> data_indexer = 0;

	if (g_cfg.core.spu_decoder == spu_decoder_type::dynamic || g_cfg.core.spu_decoder == spu_decoder_type::llvm)
	{
		if (auto compiler = spu_recompiler_base::make_llvm_recompiler(11))
		{
			compiler->init();

			if (compiler->compile({}) && spu_runtime::g_interpreter)
			{
				spu_log.success("SPU Runtime: Built the interpreter.");

				if (g_cfg.core.spu_decoder != spu_decoder_type::llvm)
				{
					return;
				}
			}
			else
			{
				spu_log.fatal("SPU Runtime: Failed to build the interpreter.");
			}
		}
	}

	u32 worker_count = 0;

	std::optional<scoped_progress_dialog> progr;

	u32 total_funcs = 0;

	if (g_cfg.core.spu_decoder == spu_decoder_type::asmjit || g_cfg.core.spu_decoder == spu_decoder_type::llvm)
	{
		const usz add_count = func_list.size() + total_precompile;

		if (add_count)
		{
			total_funcs = build_existing_cache ? ::narrow<u32>(add_count) : 0;
		}

		worker_count = std::min<u32>(rpcs3::utils::get_max_threads(), ::narrow<u32>(add_count));
	}

	atomic_t<u32> pending_progress = 0;
	atomic_t<bool> showing_progress = false;

	if (!g_progr_ptotal)
	{
		g_progr_ptotal += total_funcs;
		showing_progress.release(true);
		progr.emplace("Building SPU cache...");
	}

	named_thread_group workers("SPU Worker ", worker_count, [&]() -> uint
	{
#ifdef __APPLE__
		pthread_jit_write_protect_np(false);
#endif
		// Set low priority
		thread_ctrl::scoped_priority low_prio(-1);

		// Initialize compiler instances for parallel compilation
		std::unique_ptr<spu_recompiler_base> compiler;

		if (g_cfg.core.spu_decoder == spu_decoder_type::asmjit)
		{
			compiler = spu_recompiler_base::make_asmjit_recompiler();
		}
		else if (g_cfg.core.spu_decoder == spu_decoder_type::llvm)
		{
			compiler = spu_recompiler_base::make_llvm_recompiler();
		}

		compiler->init();

		// Counter for error reporting
		u32 logged_error = 0;

		// How much every thread compiled
		uint result = 0;

		// Fake LS
		std::vector<be_t<u32>> ls(0x10000);

		usz func_i = fnext++;

		// Ensure some actions are performed on a single thread
		const bool is_first_thread = func_i == 0;

		// Build functions
		for (; func_i < func_list.size(); func_i = fnext++, (showing_progress ? g_progr_pdone : pending_progress) += build_existing_cache ? 1 : 0)
		{
			const spu_program& func = std::as_const(func_list)[func_i];

			if (Emu.IsStopped() || fail_flag)
			{
				continue;
			}

			// Get data start
			const u32 start = func.lower_bound;
			const u32 size0 = ::size32(func.data);

			be_t<u64> hash_start;
			{
				sha1_context ctx;
				u8 output[20];

				sha1_starts(&ctx);
				sha1_update(&ctx, reinterpret_cast<const u8*>(func.data.data()), func.data.size() * 4);
				sha1_finish(&ctx, output);
				std::memcpy(&hash_start, output, sizeof(hash_start));
			}

			// Check hash against allowed bounds
			const bool inverse_bounds = g_cfg.core.spu_llvm_lower_bound > g_cfg.core.spu_llvm_upper_bound;

			if ((!inverse_bounds && (hash_start < g_cfg.core.spu_llvm_lower_bound || hash_start > g_cfg.core.spu_llvm_upper_bound)) ||
				(inverse_bounds && (hash_start < g_cfg.core.spu_llvm_lower_bound && hash_start > g_cfg.core.spu_llvm_upper_bound)))
			{
				spu_log.error("[Debug] Skipped function %s", fmt::base57(hash_start));
				result++;
				continue;
			}

			// Initialize LS with function data only
			for (u32 i = 0, pos = start; i < size0; i++, pos += 4)
			{
				ls[pos / 4] = std::bit_cast<be_t<u32>>(func.data[i]);
			}

			// Call analyser
			spu_program func2 = compiler->analyse(ls.data(), func.entry_point);

			if (func2 != func)
			{
				spu_log.error("[0x%05x] SPU Analyser failed, %u vs %u", func2.entry_point, func2.data.size(), size0);

				if (logged_error < 2)
				{
					std::string log;
					compiler->dump(func, log);
					spu_log.notice("[0x%05x] Function: %s", func.entry_point, log);
					logged_error++;
				}
			}
			else if (!compiler->compile(std::move(func2)))
			{
				// Likely, out of JIT memory. Signal to prevent further building.
				fail_flag |= 1;
				continue;
			}

			// Clear fake LS
			std::memset(ls.data() + start / 4, 0, 4 * (size0 - 1));

			result++;

			if (is_first_thread && !showing_progress)
			{
				if (!g_progr.load() && !g_progr_ptotal && !g_progr_ftotal)
				{
					showing_progress = true;
					g_progr_pdone += pending_progress.exchange(0);
					g_progr_ptotal += total_funcs;
					progr.emplace("Building SPU cache...");
				}
			}
			else if (showing_progress && pending_progress)
			{
				// Cover missing progress due to a race
				g_progr_pdone += pending_progress.exchange(0);
			}
		}

		u32 last_sec_idx = umax;

		for (func_i = data_indexer++;; func_i = data_indexer++, (showing_progress ? g_progr_pdone : pending_progress) += build_existing_cache ? 1 : 0)
		{
			usz passed_count = 0;
			u32 func_addr = 0;
			u32 next_func = 0;
			u32 sec_addr = umax;
			u32 sec_idx = 0;
			std::basic_string_view<u32> inst_data;

			// Try to get the data this index points to
			for (auto& sec : data_list)
			{
				if (func_i < passed_count + sec.funcs.size())
				{
					const usz func_idx = func_i - passed_count;
					sec_addr = sec.vaddr;
					func_addr = ::at32(sec.funcs, func_idx);
					inst_data = sec.inst_data;
					next_func = sec.funcs.size() >= func_idx ? ::narrow<u32>(sec_addr + inst_data.size() * 4) : sec.funcs[func_idx];
					break;
				}

				passed_count += sec.funcs.size();
				sec_idx++;
			}

			if (sec_addr == umax)
			{
				// End of compilation for thread
				break;
			}

			if (Emu.IsStopped() || fail_flag)
			{
				continue;
			}

			if (last_sec_idx != sec_idx)
			{
				if (last_sec_idx != umax)
				{
					// Clear fake LS of previous section
					auto& sec = data_list[last_sec_idx];
					std::memset(ls.data() + sec.vaddr / 4, 0, sec.inst_data.size() * 4);
				}

				// Initialize LS with the entire section data
				for (u32 i = 0, pos = sec_addr; i < inst_data.size(); i++, pos += 4)
				{
					ls[pos / 4] =  std::bit_cast<be_t<u32>>(inst_data[i]);
				}

				last_sec_idx = sec_idx;
			}

			u32 block_addr = func_addr;

			std::map<u32, std::basic_string<u32>> targets;

			// Call analyser
			spu_program func2 = compiler->analyse(ls.data(), block_addr, &targets);

			while (!func2.data.empty())
			{
				const u32 last_inst = std::bit_cast<be_t<u32>>(func2.data.back());
				const u32 prog_size = ::size32(func2.data);

				if (!compiler->compile(std::move(func2)))
				{
					// Likely, out of JIT memory. Signal to prevent further building.
					fail_flag |= 1;
					break;
				}

				result++;

				const u32 start_new = block_addr + prog_size * 4;

				if (start_new >= next_func || (start_new == next_func - 4 && ls[start_new / 4] == 0x200000u))
				{
					// Completed
					break;
				}

				if (auto type = g_spu_itype.decode(last_inst);
					type == spu_itype::BRSL || type == spu_itype::BRASL || type == spu_itype::BISL || type == spu_itype::SYNC)
				{
					if (ls[start_new / 4] && g_spu_itype.decode(ls[start_new / 4]) != spu_itype::UNK)
					{
						spu_log.notice("Precompiling fallthrough to 0x%05x", start_new);
						func2 = compiler->analyse(ls.data(), start_new, &targets);
						block_addr = start_new;
						continue;
					}
				}

				if (targets.empty())
				{
					break;
				}

				const auto upper = targets.upper_bound(func_addr);

				if (upper == targets.begin())
				{
					break;
				}

				u32 new_entry = umax;

				// Find the lowest target in the space in-between
				for (auto it = std::prev(upper); it != targets.end() && it->first < start_new && new_entry > start_new; it++)
				{
					for (u32 target : it->second)
					{
						if (target >= start_new && target < next_func)
						{
							if (target < new_entry)
							{
								new_entry = target;

								if (new_entry == start_new)
								{
									// Cannot go lower
									break;
								}
							}
						}
					}
				}

				if (new_entry != umax && !spu_thread::is_exec_code(new_entry, { reinterpret_cast<const u8*>(ls.data()), SPU_LS_SIZE }, 0, true))
				{
					new_entry = umax;
				}

				if (new_entry == umax)
				{
					new_entry = start_new;

					while (new_entry < next_func && (ls[start_new / 4] < 0x3fffc || !spu_thread::is_exec_code(new_entry, { reinterpret_cast<const u8*>(ls.data()), SPU_LS_SIZE }, 0, true)))
					{
						new_entry += 4;
					}

					if (new_entry >= next_func || (new_entry == next_func - 4 && ls[new_entry / 4] == 0x200000u))
					{
						// Completed
						break;
					}
				}


				spu_log.notice("Precompiling filler space at 0x%05x (next=0x%05x)", new_entry, next_func);
				func2 = compiler->analyse(ls.data(), new_entry, &targets);
				block_addr = new_entry;
			}

			if (is_first_thread && !showing_progress)
			{
				if (!g_progr.load() && !g_progr_ptotal && !g_progr_ftotal)
				{
					showing_progress = true;
					g_progr_pdone += pending_progress.exchange(0);
					g_progr_ptotal += total_funcs;
					progr.emplace("Building SPU cache...");
				}
			}
			else if (showing_progress && pending_progress)
			{
				// Cover missing progress due to a race
				g_progr_pdone += pending_progress.exchange(0);
			}
		}

		if (showing_progress && pending_progress)
		{
			// Cover missing progress due to a race
			g_progr_pdone += pending_progress.exchange(0);
		}

		return result;
	});

	u32 built_total = 0;

	// Join (implicitly) and print individual results
	for (u32 i = 0; i < workers.size(); i++)
	{
		spu_log.notice("SPU Runtime: Worker %u built %u programs.", i + 1, workers[i]);
		built_total += workers[i];
	}

	spu_log.notice("SPU Runtime: Workers built %u programs.", built_total);

	if (Emu.IsStopped())
	{
		spu_log.error("SPU Runtime: Cache building aborted.");
		return;
	}

	if (fail_flag)
	{
		spu_log.fatal("SPU Runtime: Cache building failed (out of memory).");
		return;
	}

	if ((g_cfg.core.spu_decoder == spu_decoder_type::asmjit || g_cfg.core.spu_decoder == spu_decoder_type::llvm) && !func_list.empty())
	{
		spu_log.success("SPU Runtime: Built %u functions.", func_list.size());

		if (g_cfg.core.spu_debug)
		{
			std::string dump;
			dump.reserve(10'000'000);

			std::map<std::basic_string_view<u8>, spu_program*> sorted;

			for (auto&& f : func_list)
			{
				// Interpret as a byte string
				std::basic_string_view<u8> data = {reinterpret_cast<u8*>(f.data.data()), f.data.size() * sizeof(u32)};

				sorted[data] = &f;
			}

			std::unordered_set<u32> depth_n;

			u32 n_max = 0;

			for (auto&& [bytes, f] : sorted)
			{
				{
					sha1_context ctx;
					u8 output[20];

					sha1_starts(&ctx);
					sha1_update(&ctx, bytes.data(), bytes.size());
					sha1_finish(&ctx, output);
					fmt::append(dump, "\n\t[%s] ", fmt::base57(output));
				}

				u32 depth_m = 0;

				for (auto&& [data, f2] : sorted)
				{
					u32 depth = 0;

					if (f2 == f)
					{
						continue;
					}

					for (u32 i = 0; i < bytes.size(); i++)
					{
						if (i < data.size() && data[i] == bytes[i])
						{
							depth++;
						}
						else
						{
							break;
						}
					}

					depth_n.emplace(depth);
					depth_m = std::max(depth, depth_m);
				}

				fmt::append(dump, "c=%06d,d=%06d ", depth_n.size(), depth_m);

				bool sk = false;

				for (u32 i = 0; i < bytes.size(); i++)
				{
					if (depth_m == i)
					{
						dump += '|';
						sk = true;
					}

					fmt::append(dump, "%02x", bytes[i]);

					if (i % 4 == 3)
					{
						if (sk)
						{
							sk = false;
						}
						else
						{
							dump += ' ';
						}

						dump += ' ';
					}
				}

				fmt::append(dump, "\n\t%49s", "");

				for (u32 i = 0; i < f->data.size(); i++)
				{
					fmt::append(dump, "%-10s", g_spu_iname.decode(std::bit_cast<be_t<u32>>(f->data[i])));
				}

				n_max = std::max(n_max, ::size32(depth_n));

				depth_n.clear();
			}

			spu_log.notice("SPU Cache Dump (max_c=%d): %s", n_max, dump);
		}
	}

	// Initialize global cache instance
	if (g_cfg.core.spu_cache && cache)
	{
		g_fxo->get<spu_cache>() = std::move(cache);
	}
}

bool spu_program::operator==(const spu_program& rhs) const noexcept
{
	// TODO
	return entry_point - lower_bound == rhs.entry_point - rhs.lower_bound && data == rhs.data;
}

bool spu_program::operator<(const spu_program& rhs) const noexcept
{
	const u32 lhs_offs = (entry_point - lower_bound) / 4;
	const u32 rhs_offs = (rhs.entry_point - rhs.lower_bound) / 4;

	// Select range for comparison
	std::basic_string_view<u32> lhs_data(data.data() + lhs_offs, data.size() - lhs_offs);
	std::basic_string_view<u32> rhs_data(rhs.data.data() + rhs_offs, rhs.data.size() - rhs_offs);
	const auto cmp0 = lhs_data.compare(rhs_data);

	if (cmp0 < 0)
		return true;
	else if (cmp0 > 0)
		return false;

	// Compare from address 0 to the point before the entry point (TODO: undesirable)
	lhs_data = {data.data(), lhs_offs};
	rhs_data = {rhs.data.data(), rhs_offs};
	const auto cmp1 = lhs_data.compare(rhs_data);

	if (cmp1 < 0)
		return true;
	else if (cmp1 > 0)
		return false;

	// TODO
	return lhs_offs < rhs_offs;
}

spu_runtime::spu_runtime()
{
	// Clear LLVM output
	m_cache_path = rpcs3::cache::get_ppu_cache();

	if (m_cache_path.empty())
	{
		return;
	}

	fs::create_dir(m_cache_path + "llvm/");
	fs::remove_all(m_cache_path + "llvm/", false);

	if (g_cfg.core.spu_debug)
	{
		fs::file(m_cache_path + "spu.log", fs::rewrite);
		fs::file(m_cache_path + "spu-ir.log", fs::rewrite);
	}
}

spu_item* spu_runtime::add_empty(spu_program&& data)
{
	if (data.data.empty())
	{
		return nullptr;
	}

	// Store previous item if already added
	spu_item* prev = nullptr;

	//Try to add item that doesn't exist yet
	const auto ret = m_stuff[data.data[0] >> 12].push_if([&](spu_item& _new, spu_item& _old)
	{
		if (_new.data == _old.data)
		{
			prev = &_old;
			return false;
		}

		return true;
	}, std::move(data));

	if (ret)
	{
		return ret;
	}

	return prev;
}

spu_function_t spu_runtime::rebuild_ubertrampoline(u32 id_inst)
{
	// Prepare sorted list
	static thread_local std::vector<std::pair<std::basic_string_view<u32>, spu_function_t>> m_flat_list;

	// Remember top position
	auto stuff_it = ::at32(m_stuff, id_inst >> 12).begin();
	auto stuff_end = ::at32(m_stuff, id_inst >> 12).end();
	{
		if (stuff_it->trampoline)
		{
			return stuff_it->trampoline;
		}

		m_flat_list.clear();

		for (auto it = stuff_it; it != stuff_end; ++it)
		{
			if (const auto ptr = it->compiled.load())
			{
				std::basic_string_view<u32> range{it->data.data.data(), it->data.data.size()};
				range.remove_prefix((it->data.entry_point - it->data.lower_bound) / 4);
				m_flat_list.emplace_back(range, ptr);
			}
			else
			{
				// Pull oneself deeper (TODO)
				++stuff_it;
			}
		}
	}

	std::sort(m_flat_list.begin(), m_flat_list.end(), FN(x.first < y.first));

	struct work
	{
		u32 size;
		u16 from;
		u16 level;
		u8* rel32;
		decltype(m_flat_list)::iterator beg;
		decltype(m_flat_list)::iterator end;
	};

	// Scratch vector
	static thread_local std::vector<work> workload;

	// Generate a dispatcher (übertrampoline)
	const auto beg = m_flat_list.begin();
	const auto _end = m_flat_list.end();
	const u32 size0 = ::size32(m_flat_list);

	auto result = beg->second;

	if (size0 != 1)
	{
#if defined(ARCH_ARM64)
		// Allocate some writable executable memory
		u8* const wxptr = jit_runtime::alloc(size0 * 128 + 16, 16);

		if (!wxptr)
		{
			return nullptr;
		}

		// Raw assembly pointer
		u8* raw = wxptr;

		auto make_jump = [&](asmjit::arm::CondCode op, auto target)
		{
			// 36 bytes
			// Fallback to dispatch if no target
			const u64 taddr = target ? reinterpret_cast<u64>(target) : reinterpret_cast<u64>(tr_dispatch);

			// ldr x9, #16 -> ldr x9, taddr
			*raw++ = 0x89;
			*raw++ = 0x00;
			*raw++ = 0x00;
			*raw++ = 0x58;

			if (op == asmjit::arm::CondCode::kAlways)
			{
				// br x9
				*raw++ = 0x20;
				*raw++ = 0x01;
				*raw++ = 0x1F;
				*raw++ = 0xD6;

				// nop
				*raw++ = 0x1F;
				*raw++ = 0x20;
				*raw++ = 0x03;
				*raw++ = 0xD5;

				// nop
				*raw++ = 0x1F;
				*raw++ = 0x20;
				*raw++ = 0x03;
				*raw++ = 0xD5;
			}
			else
			{
				// b.COND #8 -> b.COND do_branch
				switch (op)
				{
				case asmjit::arm::CondCode::kUnsignedLT:
					*raw++ = 0x43;
					break;
				case asmjit::arm::CondCode::kUnsignedGT:
					*raw++ = 0x48;
					break;
				default:
					asm("brk 0x42");
				}

				*raw++ = 0x00;
				*raw++ = 0x00;
				*raw++ = 0x54;

				// b #16 -> b cont
				*raw++ = 0x04;
				*raw++ = 0x00;
				*raw++ = 0x00;
				*raw++ = 0x14;

				// do_branch: br x9
				*raw++ = 0x20;
				*raw++ = 0x01;
				*raw++ = 0x1f;
				*raw++ = 0xD6;
			}

			// taddr
			std::memcpy(raw, &taddr, 8);
			raw += 8;

			// cont: next instruction
		};
#elif defined(ARCH_X64)
		// Allocate some writable executable memory
		u8* const wxptr = jit_runtime::alloc(size0 * 22 + 14, 16);

		if (!wxptr)
		{
			return nullptr;
		}

		// Raw assembly pointer
		u8* raw = wxptr;

		// Write jump instruction with rel32 immediate
		auto make_jump = [&](u8 op, auto target)
		{
			ensure(raw + 8 <= wxptr + size0 * 22 + 16);

			// Fallback to dispatch if no target
			const u64 taddr = target ? reinterpret_cast<u64>(target) : reinterpret_cast<u64>(tr_dispatch);

			// Compute the distance
			const s64 rel = taddr - reinterpret_cast<u64>(raw) - (op != 0xe9 ? 6 : 5);

			ensure(rel >= s32{smin} && rel <= s32{smax});

			if (op != 0xe9)
			{
				// First jcc byte
				*raw++ = 0x0f;
				ensure((op >> 4) == 0x8);
			}

			*raw++ = op;

			const s32 r32 = static_cast<s32>(rel);

			std::memcpy(raw, &r32, 4);
			raw += 4;
		};
#endif

		workload.clear();
		workload.reserve(size0);
		workload.emplace_back();
		workload.back().size  = size0;
		workload.back().level = 0;
		workload.back().from  = -1;
		workload.back().rel32 = nullptr;
		workload.back().beg   = beg;
		workload.back().end   = _end;

		// LS address starting from PC is already loaded into rcx (see spu_runtime::tr_all)

		for (usz i = 0; i < workload.size(); i++)
		{
			// Get copy of the workload info
			auto w = workload[i];

			// Split range in two parts
			auto it = w.beg;
			auto it2 = w.beg;
			u32 size1 = w.size / 2;
			u32 size2 = w.size - size1;
			std::advance(it2, w.size / 2);

			while (ensure(w.level < umax))
			{
				it = it2;
				size1 = w.size - size2;

				if (w.level >= w.beg->first.size())
				{
					// Cannot split: smallest function is a prefix of bigger ones (TODO)
					break;
				}

				const u32 x1 = ::at32(w.beg->first, w.level);

				if (!x1)
				{
					// Cannot split: some functions contain holes at this level
					w.level++;

					// Resort subrange starting from the new level
					std::stable_sort(w.beg, w.end, [&](const auto& a, const auto& b)
					{
						std::basic_string_view<u32> lhs = a.first;
						std::basic_string_view<u32> rhs = b.first;

						lhs.remove_prefix(w.level);
						rhs.remove_prefix(w.level);

						return lhs < rhs;
					});

					continue;
				}

				// Adjust ranges (forward)
				while (it != w.end && x1 == ::at32(it->first, w.level))
				{
					it++;
					size1++;
				}

				if (it == w.end)
				{
					// Cannot split: words are identical within the range at this level
					w.level++;
				}
				else
				{
					size2 = w.size - size1;
					break;
				}
			}

			if (w.rel32)
			{
#if defined(ARCH_X64)
				// Patch rel32 linking it to the current location if necessary
				const s32 r32 = ::narrow<s32>(raw - w.rel32);
				std::memcpy(w.rel32 - 4, &r32, 4);
#elif defined(ARCH_ARM64)
				//	Rewrite jump address
				{
					u64 raw64 = reinterpret_cast<u64>(raw);
					memcpy(w.rel32 - 8, &raw64, 8);
				}
#else
#error "Unimplemented"
#endif
			}

			if (w.level >= w.beg->first.size() || w.level >= it->first.size())
			{
				// If functions cannot be compared, assume smallest function
				spu_log.error("Trampoline simplified at ??? (level=%u)", w.level);
#if defined(ARCH_X64)
				make_jump(0xe9, w.beg->second); // jmp rel32
#elif defined(ARCH_ARM64)
				u64 branch_target = reinterpret_cast<u64>(w.beg->second);
				make_jump(asmjit::arm::CondCode::kAlways, branch_target);
#else
#error "Unimplemented"
#endif
				continue;
			}

			// Value for comparison
			const u32 x = ::at32(it->first, w.level);

			// Adjust ranges (backward)
			while (it != m_flat_list.begin())
			{
				it--;

				if (w.level >= it->first.size())
				{
					it = m_flat_list.end();
					break;
				}

				if (::at32(it->first, w.level) != x)
				{
					it++;
					break;
				}

				ensure(it != w.beg);
				size1--;
				size2++;
			}

			if (it == m_flat_list.end())
			{
				spu_log.error("Trampoline simplified (II) at ??? (level=%u)", w.level);
#if defined(ARCH_X64)
				make_jump(0xe9, w.beg->second); // jmp rel32
#elif defined(ARCH_ARM64)
				u64 branch_target = reinterpret_cast<u64>(w.beg->second);
				make_jump(asmjit::arm::CondCode::kAlways, branch_target);
#else
#error "Unimplemented"
#endif
				continue;
			}

			// Emit 32-bit comparison
#if defined(ARCH_X64)
			ensure(raw + 12 <= wxptr + size0 * 22 + 16); // "Asm overflow"
#elif defined(ARCH_ARM64)
			ensure(raw + (4 * 4) <= wxptr + size0 * 128 + 16);
#else
#error "Unimplemented"
#endif

			if (w.from != w.level)
			{
				// If necessary (level has advanced), emit load: mov eax, [rcx + addr]
				const u32 cmp_lsa = w.level * 4u;
#if defined(ARCH_X64)
				if (cmp_lsa < 0x80)
				{
					*raw++ = 0x8b;
					*raw++ = 0x41;
					*raw++ = ::narrow<s8>(cmp_lsa);
				}
				else
				{
					*raw++ = 0x8b;
					*raw++ = 0x81;
					std::memcpy(raw, &cmp_lsa, 4);
					raw += 4;
				}
#elif defined(ARCH_ARM64)
				// ldr w9, #8
				*raw++ = 0x49;
				*raw++ = 0x00;
				*raw++ = 0x00;
				*raw++ = 0x18;

				// b #8
				*raw++ = 0x02;
				*raw++ = 0x00;
				*raw++ = 0x00;
				*raw++ = 0x14;

				// cmp_lsa
				std::memcpy(raw, &cmp_lsa, 4);
				raw += 4;

				// ldr w1, [x7, x9]
				*raw++ = 0xE1;
				*raw++ = 0x68;
				*raw++ = 0x69;
				*raw++ = 0xB8;
#else
#error "Unimplemented"
#endif
			}

			// Emit comparison: cmp eax, imm32
#if defined(ARCH_X64)
			*raw++ = 0x3d;
			std::memcpy(raw, &x, 4);
			raw += 4;
#elif defined(ARCH_ARM64)
			// ldr w9, #8
			*raw++ = 0x49;
			*raw++ = 0x00;
			*raw++ = 0x00;
			*raw++ = 0x18;

			// b #8
			*raw++ = 0x02;
			*raw++ = 0x00;
			*raw++ = 0x00;
			*raw++ = 0x14;

			// x
			std::memcpy(raw, &x, 4);
			raw += 4;

			// cmp w1, w9
			*raw++ = 0x3f;
			*raw++ = 0x00;
			*raw++ = 0x09;
			*raw++ = 0x6B;
#else
#error "Unimplemented"
#endif

			// Low subrange target
			if (size1 == 1)
			{
#if defined(ARCH_X64)
				make_jump(0x82, w.beg->second); // jb rel32
#elif defined(ARCH_ARM64)
				u64 branch_target = reinterpret_cast<u64>(w.beg->second);
				make_jump(asmjit::arm::CondCode::kUnsignedLT, branch_target);
#else
#error "Unimplemented"
#endif
			}
			else
			{
#if defined(ARCH_X64)
				make_jump(0x82, raw); // jb rel32 (stub)
#elif defined(ARCH_ARM64)
				make_jump(asmjit::arm::CondCode::kUnsignedLT, raw);
#else
#error "Unimplemented"
#endif
				auto& to = workload.emplace_back(w);
				to.end   = it;
				to.size  = size1;
				to.rel32 = raw;
				to.from  = w.level;
			}

			// Second subrange target
			if (size2 == 1)
			{
#if defined(ARCH_X64)
				make_jump(0xe9, it->second); // jmp rel32
#elif defined(ARCH_ARM64)
				u64 branch_target = reinterpret_cast<u64>(it->second);
				make_jump(asmjit::arm::CondCode::kAlways, branch_target);
#else
#error "Unimplemented"
#endif
			}
			else
			{
				it2 = it;

				// Select additional midrange for equality comparison
				while (it2 != w.end && ::at32(it2->first, w.level) == x)
				{
					size2--;
					it2++;
				}

				if (it2 != w.end)
				{
					// High subrange target
					if (size2 == 1)
					{
#if defined(ARCH_X64)
						make_jump(0x87, it2->second); // ja rel32
#elif defined(ARCH_ARM64)
						u64 branch_target = reinterpret_cast<u64>(it2->second);
						make_jump(asmjit::arm::CondCode::kUnsignedGT, branch_target);
#else
#throw "Unimplemented"
#endif
					}
					else
					{
#if defined(ARCH_X64)
						make_jump(0x87, raw); // ja rel32 (stub)
#elif defined(ARCH_ARM64)
						make_jump(asmjit::arm::CondCode::kUnsignedGT, raw);
#else
#error "Unimplemented"
#endif
						auto& to = workload.emplace_back(w);
						to.beg   = it2;
						to.size  = size2;
						to.rel32 = raw;
						to.from  = w.level;
					}

					const u32 size3 = w.size - size1 - size2;

					if (size3 == 1)
					{
#if defined(ARCH_X64)
						make_jump(0xe9, it->second); // jmp rel32
#elif defined(ARCH_ARM64)
						u64 branch_target = reinterpret_cast<u64>(it->second);
						make_jump(asmjit::arm::CondCode::kAlways, branch_target);
#else
#error "Unimplemented"
#endif
					}
					else
					{
#if defined(ARCH_X64)
						make_jump(0xe9, raw); // jmp rel32 (stub)
#elif defined(ARCH_ARM64)
						make_jump(asmjit::arm::CondCode::kAlways, raw);
#else
#error "Unimplemented"
#endif
						auto& to = workload.emplace_back(w);
						to.beg   = it;
						to.end   = it2;
						to.size  = size3;
						to.rel32 = raw;
						to.from  = w.level;
					}
				}
				else
				{
#if defined(ARCH_X64)
					make_jump(0xe9, raw); // jmp rel32 (stub)
#elif defined(ARCH_ARM64)
					make_jump(asmjit::arm::CondCode::kAlways, raw);
#else
#error "Unimplemented"
#endif
					auto& to = workload.emplace_back(w);
					to.beg   = it;
					to.size  = w.size - size1;
					to.rel32 = raw;
					to.from  = w.level;
				}
			}
		}

		workload.clear();
		result = reinterpret_cast<spu_function_t>(reinterpret_cast<u64>(wxptr));

		std::string fname;
		fmt::append(fname, "__ub%u", m_flat_list.size());
		jit_announce(wxptr, raw - wxptr, fname);
	}

	if (auto _old = stuff_it->trampoline.compare_and_swap(nullptr, result))
	{
		return _old;
	}

	// Install ubertrampoline
	auto& insert_to = ::at32(*spu_runtime::g_dispatcher, id_inst >> 12);

	auto _old = insert_to.load();

	do
	{
		// Make sure we are replacing an older ubertrampoline but not newer one
		if (_old != tr_dispatch)
		{
			bool ok = false;

			for (auto it = stuff_it; it != stuff_end; ++it)
			{
				if (it->trampoline == _old)
				{
					ok = true;
					break;
				}
			}

			if (!ok)
			{
				return result;
			}
		}
	}
	while (!insert_to.compare_exchange(_old, result));

	return result;
}

spu_function_t spu_runtime::find(const u32* ls, u32 addr) const
{
	const u32 index = ls[addr / 4] >> 12;
	for (const auto& item : ::at32(m_stuff, index))
	{
		if (const auto ptr = item.compiled.load())
		{
			std::basic_string_view<u32> range{item.data.data.data(), item.data.data.size()};
			range.remove_prefix((item.data.entry_point - item.data.lower_bound) / 4);

			if (addr / 4 + range.size() > 0x10000)
			{
				continue;
			}

			if (range.compare(0, range.size(), ls + addr / 4, range.size()) == 0)
			{
				return ptr;
			}
		}
	}

	return nullptr;
}

spu_function_t spu_runtime::make_branch_patchpoint(u16 data) const
{
#if defined(ARCH_X64)
	u8* const raw = jit_runtime::alloc(16, 16);

	if (!raw)
	{
		return nullptr;
	}

	// Save address of the following jmp (GHC CC 3rd argument)
	raw[0] = 0x4c; // lea r12, [rip+1]
	raw[1] = 0x8d;
	raw[2] = 0x25;
	raw[3] = 0x01;
	raw[4] = 0x00;
	raw[5] = 0x00;
	raw[6] = 0x00;

	raw[7] = 0x90; // nop

	// Jump to spu_recompiler_base::branch
	raw[8] = 0xe9;
	// Compute the distance
	const s64 rel = reinterpret_cast<u64>(tr_branch) - reinterpret_cast<u64>(raw + 8) - 5;
	std::memcpy(raw + 9, &rel, 4);
	raw[13] = 0xcc;
	raw[14] = data >> 8;
	raw[15] = data & 0xff;

	return reinterpret_cast<spu_function_t>(raw);
#elif defined(ARCH_ARM64)
#if defined(__APPLE__)
	pthread_jit_write_protect_np(false);
#endif

	u8* const patch_fn = ensure(jit_runtime::alloc(36, 16));
	u8* raw = patch_fn;

	// adr x21, #16
	*raw++ = 0x95;
	*raw++ = 0x00;
	*raw++ = 0x00;
	*raw++ = 0x10;

	// nop x3
	for (int i = 0; i < 3; i++)
	{
		*raw++ = 0x1F;
		*raw++ = 0x20;
		*raw++ = 0x03;
		*raw++ = 0xD5;
	}

	// ldr x9, #8
	*raw++ = 0x49;
	*raw++ = 0x00;
	*raw++ = 0x00;
	*raw++ = 0x58;

	// br x9
	*raw++ = 0x20;
	*raw++ = 0x01;
	*raw++ = 0x1F;
	*raw++ = 0xD6;

	u64 branch_target = reinterpret_cast<u64>(tr_branch);
	std::memcpy(raw, &branch_target, 8);
	raw += 8;

	*raw++ = static_cast<u8>(data >> 8);
	*raw++ = static_cast<u8>(data & 0xff);

#if defined(__APPLE__)
	pthread_jit_write_protect_np(true);
#endif

	// Flush all cache lines after potentially writing executable code
	asm("ISB");
	asm("DSB ISH");

	return reinterpret_cast<spu_function_t>(patch_fn);
#else
#error "Unimplemented"
#endif
}

spu_recompiler_base::spu_recompiler_base()
{
}

spu_recompiler_base::~spu_recompiler_base()
{
}

void spu_recompiler_base::dispatch(spu_thread& spu, void*, u8* rip)
{
	// If code verification failed from a patched patchpoint, clear it with a dispatcher jump
	if (rip)
	{
#if defined(ARCH_X64)
		const s64 rel = reinterpret_cast<u64>(spu_runtime::tr_all) - reinterpret_cast<u64>(rip - 8) - 5;

		union
		{
			u8 bytes[8];
			u64 result;
		};

		bytes[0] = 0xe9; // jmp rel32
		std::memcpy(bytes + 1, &rel, 4);
		bytes[5] = 0x66; // lnop (2 bytes)
		bytes[6] = 0x90;
		bytes[7] = 0x90;

		atomic_storage<u64>::release(*reinterpret_cast<u64*>(rip - 8), result);
#elif defined(ARCH_ARM64)
		union
		{
			u8 bytes[16];
			u128 result;
		};

		// ldr x9, #8
		bytes[0] = 0x49;
		bytes[1] = 0x00;
		bytes[2] = 0x00;
		bytes[3] = 0x58;

		// br x9
		bytes[4] = 0x20;
		bytes[5] = 0x01;
		bytes[6] = 0x1F;
		bytes[7] = 0xD6;

		const u64 target = reinterpret_cast<u64>(spu_runtime::tr_all);
		std::memcpy(bytes + 8, &target, 8);
#if defined(__APPLE__)
		pthread_jit_write_protect_np(false);
#endif
		atomic_storage<u128>::release(*reinterpret_cast<u128*>(rip), result);
#if defined(__APPLE__)
		pthread_jit_write_protect_np(true);
#endif

		// Flush all cache lines after potentially writing executable code
		asm("ISB");
		asm("DSB ISH");
#else
#error "Unimplemented"
#endif
	}

	// Second attempt (recover from the recursion after repeated unsuccessful trampoline call)
	if (spu.block_counter != spu.block_recover && &dispatch != ::at32(*spu_runtime::g_dispatcher, spu._ref<nse_t<u32>>(spu.pc) >> 12))
	{
		spu.block_recover = spu.block_counter;
		return;
	}

	spu.jit->init();

	// Compile
	if (spu._ref<u32>(spu.pc) == 0u)
	{
		spu_runtime::g_escape(&spu);
		return;
	}

	const auto func = spu.jit->compile(spu.jit->analyse(spu._ptr<u32>(0), spu.pc));

	if (!func)
	{
		spu_log.fatal("[0x%05x] Compilation failed.", spu.pc);
		return;
	}

	// Diagnostic
	if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
	{
		const v128 _info = spu.stack_mirror[(spu.gpr[1]._u32[3] & 0x3fff0) >> 4];

		if (_info._u64[0] + 1)
		{
			spu_log.trace("Called from 0x%x", _info._u32[2] - 4);
		}
	}
#if defined(__APPLE__)
	pthread_jit_write_protect_np(true);
#endif

#if defined(ARCH_ARM64)
	// Flush all cache lines after potentially writing executable code
	asm("ISB");
	asm("DSB ISH");
#endif
	spu_runtime::g_tail_escape(&spu, func, nullptr);
}

void spu_recompiler_base::branch(spu_thread& spu, void*, u8* rip)
{
#if defined(ARCH_X64)
	if (const u32 ls_off = ((rip[6] << 8) | rip[7]) * 4)
#elif defined(ARCH_ARM64)
	if (const u32 ls_off = ((rip[16] << 8) | rip[17]) * 4) // See branch_patchpoint `data`
#else
#error "Unimplemented"
#endif
	{
		spu_log.todo("Special branch patchpoint hit.\nPlease report to the developer (0x%05x).", ls_off);
	}

	// Find function
	const auto func = spu.jit->get_runtime().find(static_cast<u32*>(spu._ptr<void>(0)), spu.pc);

	if (!func)
	{
		return;
	}

#if defined(ARCH_X64)
	// Overwrite jump to this function with jump to the compiled function
	const s64 rel = reinterpret_cast<u64>(func) - reinterpret_cast<u64>(rip) - 5;

	union
	{
		u8 bytes[8];
		u64 result;
	};

	if (rel >= s32{smin} && rel <= s32{smax})
	{
		const s64 rel8 = (rel + 5) - 2;

		if (rel8 >= s8{smin} && rel8 <= s8{smax})
		{
			bytes[0] = 0xeb; // jmp rel8
			bytes[1] = static_cast<s8>(rel8);
			std::memset(bytes + 2, 0xcc, 4);
		}
		else
		{
			bytes[0] = 0xe9; // jmp rel32
			std::memcpy(bytes + 1, &rel, 4);
			bytes[5] = 0xcc;
		}

		bytes[6] = rip[6];
		bytes[7] = rip[7];
	}
	else
	{
		fmt::throw_exception("Impossible far jump: %p -> %p", rip, func);
	}

	atomic_storage<u64>::release(*reinterpret_cast<u64*>(rip), result);
#elif defined(ARCH_ARM64)
	union
	{
		u8 bytes[16];
		u128 result;
	};

	// ldr x9, #8
	bytes[0] = 0x49;
	bytes[1] = 0x00;
	bytes[2] = 0x00;
	bytes[3] = 0x58;

	// br x9
	bytes[4] = 0x20;
	bytes[5] = 0x01;
	bytes[6] = 0x1F;
	bytes[7] = 0xD6;

	const u64 target = reinterpret_cast<u64>(func);
	std::memcpy(bytes + 8, &target, 8);
#if defined(__APPLE__)
	pthread_jit_write_protect_np(false);
#endif
	atomic_storage<u128>::release(*reinterpret_cast<u128*>(rip), result);
#if defined(__APPLE__)
	pthread_jit_write_protect_np(true);
#endif

	// Flush all cache lines after potentially writing executable code
	asm("ISB");
	asm("DSB ISH");
#else
#error "Unimplemented"
#endif

	spu_runtime::g_tail_escape(&spu, func, rip);
}

void spu_recompiler_base::old_interpreter(spu_thread& spu, void* ls, u8* /*rip*/)
{
	if (g_cfg.core.spu_decoder != spu_decoder_type::_static)
	{
		fmt::throw_exception("Invalid SPU decoder");
	}

	// Select opcode table
	const auto& table = g_fxo->get<spu_interpreter_rt>();

	// LS pointer
	const auto base = static_cast<const u8*>(ls);

	while (true)
	{
		if (spu.state) [[unlikely]]
		{
			if (spu.check_state())
				break;
		}

		const u32 op = *reinterpret_cast<const be_t<u32>*>(base + spu.pc);
		if (table.decode(op)(spu, {op}))
			spu.pc += 4;
	}
}

std::vector<u32> spu_thread::discover_functions(u32 base_addr, std::span<const u8> ls, bool is_known_addr, u32 /*entry*/)
{
	std::vector<u32> calls;
	std::vector<u32> branches;

	calls.reserve(100);

	// Discover functions
	// Use the most simple method: search for instructions that calls them
	// And then filter invalid cases
	// TODO: Does not detect jumptables or fixed-addr indirect calls
	const v128 brasl_mask = is_known_addr ? v128::from32p(0x62u << 23) : v128::from32p(umax);

	for (u32 i = utils::align<u32>(base_addr, 0x10); i < std::min<u32>(base_addr + ::size32(ls), 0x3FFF0); i += 0x10)
	{
		// Search for BRSL LR and BRASL LR or BR
		// TODO: BISL
		const v128 inst = read_from_ptr<be_t<v128>>(ls.data(), i - base_addr);
		const v128 cleared_i16 = gv_and32(inst, v128::from32p(utils::rol32(~0xffff, 7)));
		const v128 eq_brsl = gv_eq32(cleared_i16, v128::from32p(0x66u << 23));
		const v128 eq_brasl = gv_eq32(cleared_i16, brasl_mask);
		const v128 eq_br = gv_eq32(cleared_i16, v128::from32p(0x64u << 23));
		const v128 result = eq_brsl | eq_brasl;

		if (!gv_testz(result))
		{
			for (u32 j = 0; j < 4; j++)
			{
				if (result.u32r[j])
				{
					calls.push_back(i + j * 4);
				}
			}
		}

		if (!gv_testz(eq_br))
		{
			for (u32 j = 0; j < 4; j++)
			{
				if (eq_br.u32r[j])
				{
					branches.push_back(i + j * 4);
				}
			}
		}
	}

	calls.erase(std::remove_if(calls.begin(), calls.end(), [&](u32 caller)
	{
		// Check the validity of both the callee code and the following caller code
		return !is_exec_code(caller, ls, base_addr, true) || !is_exec_code(caller + 4, ls, base_addr, true);
	}), calls.end());

	branches.erase(std::remove_if(branches.begin(), branches.end(), [&](u32 caller)
	{
		// Check the validity of the callee code
		return !is_exec_code(caller, ls, base_addr, true);
	}), branches.end());

	std::vector<u32> addrs;

	for (u32 addr : calls)
	{
		const spu_opcode_t op{read_from_ptr<be_t<u32>>(ls, addr - base_addr)};

		const u32 func = op_branch_targets(addr, op)[0];

		if (func == umax || addr + 4 == func || func == addr || std::count(addrs.begin(), addrs.end(), func))
		{
			continue;
		}

		if (std::count(calls.begin(), calls.end(), func))
		{
			// Cannot call another call instruction (link is overwritten)
			continue;
		}

		addrs.push_back(func);

		// Detect an "arguments passing" block, possible queue another function
		for (u32 next = func, it = 10; it && next >= base_addr && next < std::min<u32>(base_addr + ::size32(ls), 0x3FFF0); it--, next += 4)
		{
			const spu_opcode_t test_op{read_from_ptr<be_t<u32>>(ls, next - base_addr)};
			const auto type = g_spu_itype.decode(test_op.opcode);

			if (type & spu_itype::branch && type != spu_itype::BR)
			{
				break;
			}

			if (type == spu_itype::UNK || !test_op.opcode)
			{
				break;
			}

			if (type != spu_itype::BR)
			{
				continue;
			}

			const u32 target = op_branch_targets(next, op)[0];

			if (target == umax || addr + 4 == target || target == addr || std::count(addrs.begin(), addrs.end(), target))
			{
				break;
			}

			// Detect backwards branch to the block in examination
			if (target >= func && target <= next)
			{
				break;
			}

			if (!is_exec_code(target, ls, base_addr, true))
			{
				break;
			}

			addrs.push_back(target);
			break;
		}
	}

	for (u32 addr : branches)
	{
		const spu_opcode_t op{read_from_ptr<be_t<u32>>(ls, addr - base_addr)};

		const u32 func = op_branch_targets(addr, op)[0];

		if (func == umax || addr + 4 == func || func == addr || !addr)
		{
			continue;
		}

		// Search for AI R1, -x in the called code
		// Reasoning: AI R1, -x means stack frame creation, this is likely be a function
		for (u32 next = func, it = 10; it && next >= base_addr && next < std::min<u32>(base_addr + ::size32(ls), 0x3FFF0); it--, next += 4)
		{
			const spu_opcode_t test_op{read_from_ptr<be_t<u32>>(ls, next - base_addr)};
			const auto type = g_spu_itype.decode(test_op.opcode);

			if (type & spu_itype::branch)
			{
				break;
			}

			if (type == spu_itype::UNK || !test_op.opcode)
			{
				break;
			}

			bool is_func = false;

			if (type == spu_itype::AI && test_op.rt == 1u && test_op.ra == 1u)
			{
				if (test_op.si10 >= 0)
				{
					break;
				}

				is_func = true;
			}

			if (!is_func)
			{
				continue;
			}

			addr = SPU_LS_SIZE + 4; // Terminate the next condition, no further checks needed

			if (std::count(addrs.begin(), addrs.end(), func))
			{
				break;
			}

			addrs.push_back(func);
			break;
		}

		// Search for AI R1, +x or OR R3/4, Rx, 0 before the branch
		// Reasoning: AI R1, +x means stack pointer restoration, branch after that is likely a tail call
		// R3 and R4 are common function arguments because they are the first two
		for (u32 back = addr - 4, it = 10; it && back >= base_addr && back < std::min<u32>(base_addr + ::size32(ls), 0x3FFF0); it--, back -= 4)
		{
			const spu_opcode_t test_op{read_from_ptr<be_t<u32>>(ls, back - base_addr)};
			const auto type = g_spu_itype.decode(test_op.opcode);

			if (type & spu_itype::branch)
			{
				break;
			}

			bool is_tail = false;

			if (type == spu_itype::AI && test_op.rt == 1u && test_op.ra == 1u)
			{
				if (test_op.si10 <= 0)
				{
					break;
				}

				is_tail = true;
			}
			else if (!(type & spu_itype::zregmod))
			{
				const u32 op_rt = type & spu_itype::_quadrop ? +test_op.rt4 : +test_op.rt;

				if (op_rt >= 80u && (type != spu_itype::LQD || test_op.ra != 1u))
				{
					// Modifying non-volatile registers, not a call (and not context restoration)
					break;
				}

				//is_tail = op_rt == 3u || op_rt == 4u;
			}

			if (!is_tail)
			{
				continue;
			}

			if (std::count(addrs.begin(), addrs.end(), func))
			{
				break;
			}

			addrs.push_back(func);
			break;
		}
	}

	std::sort(addrs.begin(), addrs.end());

	return addrs;
}

spu_program spu_recompiler_base::analyse(const be_t<u32>* ls, u32 entry_point, std::map<u32, std::basic_string<u32>>* out_target_list)
{
	// Result: addr + raw instruction data
	spu_program result;
	result.data.reserve(10000);
	result.entry_point = entry_point;
	result.lower_bound = entry_point;

	// Initialize block entries
	m_block_info.reset();
	m_block_info.set(entry_point / 4);
	m_entry_info.reset();
	m_entry_info.set(entry_point / 4);
	m_ret_info.reset();

	// Simple block entry workload list
	workload.clear();
	workload.push_back(entry_point);

	std::memset(m_regmod.data(), 0xff, sizeof(m_regmod));
	m_use_ra.reset();
	m_use_rb.reset();
	m_use_rc.reset();
	m_targets.clear();
	m_preds.clear();
	m_preds[entry_point];
	m_bbs.clear();
	m_chunks.clear();
	m_funcs.clear();

	// Value flags (TODO: only is_const is implemented)
	enum class vf : u32
	{
		is_const,
		is_mask,
		is_rel,

		__bitset_enum_max
	};

	// Weak constant propagation context (for guessing branch targets)
	std::array<bs_t<vf>, 128> vflags{};

	// Associated constant values for 32-bit preferred slot
	std::array<u32, 128> values;

	// SYNC instruction found
	bool sync = false;

	u32 hbr_loc = 0;
	u32 hbr_tg = -1;

	// Result bounds
	u32 lsa = entry_point;
	u32 limit = 0x40000;

	if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
	{
	}

	for (u32 wi = 0, wa = workload[0]; wi < workload.size();)
	{
		const auto next_block = [&]
		{
			// Reset value information
			vflags.fill({});
			sync = false;
			hbr_loc = 0;
			hbr_tg = -1;
			wi++;

			if (wi < workload.size())
			{
				wa = workload[wi];
			}
		};

		const u32 pos = wa;

		const auto add_block = [&](u32 target)
		{
			// Validate new target (TODO)
			if (target >= lsa && target < limit)
			{
				// Check for redundancy
				if (!m_block_info[target / 4])
				{
					m_block_info[target / 4] = true;
					workload.push_back(target);
				}

				// Add predecessor
				if (m_preds[target].find_first_of(pos) + 1 == 0)
				{
					m_preds[target].push_back(pos);
				}
			}
		};

		if (pos < lsa || pos >= limit)
		{
			// Don't analyse if already beyond the limit
			next_block();
			continue;
		}

		const u32 data = ls[pos / 4];
		const auto op = spu_opcode_t{data};

		wa += 4;

		m_targets.erase(pos);

		// Fill register access info
		if (auto iflags = g_spu_iflag.decode(data))
		{
			if (+iflags & +spu_iflag::use_ra)
				m_use_ra.set(pos / 4);
			if (+iflags & +spu_iflag::use_rb)
				m_use_rb.set(pos / 4);
			if (+iflags & +spu_iflag::use_rc)
				m_use_rc.set(pos / 4);
		}

		// Analyse instruction
		switch (const auto type = g_spu_itype.decode(data))
		{
		case spu_itype::UNK:
		case spu_itype::DFCEQ:
		case spu_itype::DFCMEQ:
		case spu_itype::DFCGT:
		case spu_itype::DFCMGT:
		case spu_itype::DFTSV:
		{
			// Stop before invalid instructions (TODO)
			next_block();
			continue;
		}

		case spu_itype::SYNC:
		case spu_itype::STOP:
		case spu_itype::STOPD:
		{
			if (data == 0)
			{
				// Stop before null data
				next_block();
				continue;
			}

			if (g_cfg.core.spu_block_size == spu_block_size_type::safe)
			{
				// Stop on special instructions (TODO)
				m_targets[pos];
				next_block();
				break;
			}

			if (type == spu_itype::SYNC)
			{
				// Remember
				sync = true;
			}

			break;
		}

		case spu_itype::IRET:
		{
			if (op.d && op.e)
			{
				spu_log.error("[0x%x] Invalid interrupt flags (DE)", pos);
			}

			m_targets[pos];
			next_block();
			break;
		}

		case spu_itype::BI:
		case spu_itype::BISL:
		case spu_itype::BISLED:
		case spu_itype::BIZ:
		case spu_itype::BINZ:
		case spu_itype::BIHZ:
		case spu_itype::BIHNZ:
		{
			if (op.d && op.e)
			{
				spu_log.error("[0x%x] Invalid interrupt flags (DE)", pos);
			}

			const auto af = vflags[op.ra];
			const auto av = values[op.ra];
			const bool sl = type == spu_itype::BISL || type == spu_itype::BISLED;

			if (sl)
			{
				m_regmod[pos / 4] = op.rt;
				vflags[op.rt] = +vf::is_const;
				values[op.rt] = pos + 4;
			}

			if (af & vf::is_const)
			{
				const u32 target = spu_branch_target(av);

				spu_log.warning("[0x%x] At 0x%x: indirect branch to 0x%x%s", entry_point, pos, target, op.d ? " (D)" : op.e ? " (E)" : "");

				if (type == spu_itype::BI && target == pos + 4 && op.d)
				{
					// Disable interrupts idiom
					break;
				}

				m_targets[pos].push_back(target);

				if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
				{
					if (sync)
					{
						spu_log.notice("[0x%x] At 0x%x: ignoring %scall to 0x%x (SYNC)", entry_point, pos, sl ? "" : "tail ", target);

						if (target > entry_point)
						{
							limit = std::min<u32>(limit, target);
						}
					}
					else
					{
						m_entry_info[target / 4] = true;
						add_block(target);
					}
				}
				else if (target > entry_point)
				{
					limit = std::min<u32>(limit, target);
				}

				if (sl && g_cfg.core.spu_block_size != spu_block_size_type::safe)
				{
					m_ret_info[pos / 4 + 1] = true;
					m_entry_info[pos / 4 + 1] = true;
					m_targets[pos].push_back(pos + 4);
					add_block(pos + 4);
				}
			}
			else if (type == spu_itype::BI && g_cfg.core.spu_block_size != spu_block_size_type::safe && !op.d && !op.e && !sync)
			{
				// Analyse jump table (TODO)
				std::basic_string<u32> jt_abs;
				std::basic_string<u32> jt_rel;
				const u32 start = pos + 4;
				u64 dabs = 0;
				u64 drel = 0;

				for (u32 i = start; i < limit; i += 4)
				{
					const u32 target = ls[i / 4];

					if (target == 0 || target % 4)
					{
						// Address cannot be misaligned: abort
						break;
					}

					if (target >= lsa && target < 0x40000)
					{
						// Possible jump table entry (absolute)
						jt_abs.push_back(target);
					}

					if (target + start >= lsa && target + start < 0x40000)
					{
						// Possible jump table entry (relative)
						jt_rel.push_back(target + start);
					}

					if (std::max(jt_abs.size(), jt_rel.size()) * 4 + start <= i)
					{
						// Neither type of jump table completes
						jt_abs.clear();
						jt_rel.clear();
						break;
					}
				}

				// Choose position after the jt as an anchor and compute the average distance
				for (u32 target : jt_abs)
				{
					dabs += std::abs(static_cast<s32>(target - start - jt_abs.size() * 4));
				}

				for (u32 target : jt_rel)
				{
					drel += std::abs(static_cast<s32>(target - start - jt_rel.size() * 4));
				}

				// Add detected jump table blocks
				if (jt_abs.size() >= 3 || jt_rel.size() >= 3)
				{
					if (jt_abs.size() == jt_rel.size())
					{
						if (dabs < drel)
						{
							jt_rel.clear();
						}

						if (dabs > drel)
						{
							jt_abs.clear();
						}

						ensure(jt_abs.size() != jt_rel.size());
					}

					if (jt_abs.size() >= jt_rel.size())
					{
						const u32 new_size = (start - lsa) / 4 + ::size32(jt_abs);

						if (result.data.size() < new_size)
						{
							result.data.resize(new_size);
						}

						for (u32 i = 0; i < jt_abs.size(); i++)
						{
							add_block(jt_abs[i]);
							result.data[(start - lsa) / 4 + i] = std::bit_cast<u32, be_t<u32>>(jt_abs[i]);
							m_targets[start + i * 4];
						}

						m_targets.emplace(pos, std::move(jt_abs));
					}

					if (jt_rel.size() >= jt_abs.size())
					{
						const u32 new_size = (start - lsa) / 4 + ::size32(jt_rel);

						if (result.data.size() < new_size)
						{
							result.data.resize(new_size);
						}

						for (u32 i = 0; i < jt_rel.size(); i++)
						{
							add_block(jt_rel[i]);
							result.data[(start - lsa) / 4 + i] = std::bit_cast<u32, be_t<u32>>(jt_rel[i] - start);
							m_targets[start + i * 4];
						}

						m_targets.emplace(pos, std::move(jt_rel));
					}
				}
				else if (start + 12 * 4 < limit &&
					ls[start / 4 + 0] == 0x1ce00408u &&
					ls[start / 4 + 1] == 0x24000389u &&
					ls[start / 4 + 2] == 0x24004809u &&
					ls[start / 4 + 3] == 0x24008809u &&
					ls[start / 4 + 4] == 0x2400c809u &&
					ls[start / 4 + 5] == 0x24010809u &&
					ls[start / 4 + 6] == 0x24014809u &&
					ls[start / 4 + 7] == 0x24018809u &&
					ls[start / 4 + 8] == 0x1c200807u &&
					ls[start / 4 + 9] == 0x2401c809u)
				{
					spu_log.warning("[0x%x] Pattern 1 detected (hbr=0x%x:0x%x)", pos, hbr_loc, hbr_tg);

					// Add 8 targets (TODO)
					for (u32 addr = start + 4; addr < start + 36; addr += 4)
					{
						m_targets[pos].push_back(addr);
						add_block(addr);
					}
				}
				else if (hbr_loc > start && hbr_loc < limit && hbr_tg == start)
				{
					spu_log.warning("[0x%x] No patterns detected (hbr=0x%x:0x%x)", pos, hbr_loc, hbr_tg);
				}
			}
			else if (type == spu_itype::BI && sync)
			{
				spu_log.notice("[0x%x] At 0x%x: ignoring indirect branch (SYNC)", entry_point, pos);
			}

			if (type == spu_itype::BI || sl)
			{
				if (type == spu_itype::BI || g_cfg.core.spu_block_size == spu_block_size_type::safe)
				{
					m_targets[pos];
				}
				else
				{
					m_ret_info[pos / 4 + 1] = true;
					m_entry_info[pos / 4 + 1] = true;
					m_targets[pos].push_back(pos + 4);
					add_block(pos + 4);
				}
			}
			else
			{
				m_targets[pos].push_back(pos + 4);
				add_block(pos + 4);
			}

			next_block();
			break;
		}

		case spu_itype::BRSL:
		case spu_itype::BRASL:
		{
			const u32 target = spu_branch_target(type == spu_itype::BRASL ? 0 : pos, op.i16);

			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = pos + 4;

			if (type == spu_itype::BRSL && target == pos + 4)
			{
				// Get next instruction address idiom
				break;
			}

			m_targets[pos].push_back(target);

			if (g_cfg.core.spu_block_size != spu_block_size_type::safe)
			{
				m_ret_info[pos / 4 + 1] = true;
				m_entry_info[pos / 4 + 1] = true;
				m_targets[pos].push_back(pos + 4);
				add_block(pos + 4);
			}

			if (g_cfg.core.spu_block_size == spu_block_size_type::giga && !sync)
			{
				m_entry_info[target / 4] = true;
				add_block(target);
			}
			else
			{
				if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
				{
					spu_log.notice("[0x%x] At 0x%x: ignoring fixed call to 0x%x (SYNC)", entry_point, pos, target);
				}

				if (target > entry_point)
				{
					limit = std::min<u32>(limit, target);
				}
			}

			next_block();
			break;
		}

		case spu_itype::BRA:
		{
			const u32 target = spu_branch_target(0, op.i16);

			if (g_cfg.core.spu_block_size == spu_block_size_type::giga && !sync)
			{
				m_entry_info[target / 4] = true;
			}
			else
			{
				if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
				{
					spu_log.notice("[0x%x] At 0x%x: ignoring fixed tail call to 0x%x (SYNC)", entry_point, pos, target);
				}
			}

			add_block(target);
			next_block();
			break;
		}

		case spu_itype::BR:
		case spu_itype::BRZ:
		case spu_itype::BRNZ:
		case spu_itype::BRHZ:
		case spu_itype::BRHNZ:
		{
			const u32 target = spu_branch_target(pos, op.i16);

			if (target == pos + 4)
			{
				// Nop
				break;
			}

			m_targets[pos].push_back(target);
			add_block(target);

			if (type != spu_itype::BR)
			{
				m_targets[pos].push_back(pos + 4);
				add_block(pos + 4);
			}

			next_block();
			break;
		}

		case spu_itype::DSYNC:
		case spu_itype::HEQ:
		case spu_itype::HEQI:
		case spu_itype::HGT:
		case spu_itype::HGTI:
		case spu_itype::HLGT:
		case spu_itype::HLGTI:
		case spu_itype::LNOP:
		case spu_itype::NOP:
		case spu_itype::MTSPR:
		case spu_itype::FSCRWR:
		case spu_itype::STQA:
		case spu_itype::STQD:
		case spu_itype::STQR:
		case spu_itype::STQX:
		{
			// Do nothing
			break;
		}

		case spu_itype::WRCH:
		{
			switch (op.ra)
			{
			case MFC_EAL:
			{
				m_regmod[pos / 4] = s_reg_mfc_eal;
				break;
			}
			case MFC_LSA:
			{
				m_regmod[pos / 4] = s_reg_mfc_lsa;
				break;
			}
			case MFC_TagID:
			{
				m_regmod[pos / 4] = s_reg_mfc_tag;
				break;
			}
			case MFC_Size:
			{
				m_regmod[pos / 4] = s_reg_mfc_size;
				break;
			}
			default: break;
			}

			break;
		}

		case spu_itype::LQA:
		case spu_itype::LQD:
		case spu_itype::LQR:
		case spu_itype::LQX:
		{
			// Unconst
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = {};
			break;
		}

		case spu_itype::HBR:
		{
			hbr_loc = spu_branch_target(pos, op.roh << 7 | op.rt);
			hbr_tg  = vflags[op.ra] & vf::is_const && !op.c ? values[op.ra] & 0x3fffc : -1;
			break;
		}

		case spu_itype::HBRA:
		{
			hbr_loc = spu_branch_target(pos, op.r0h << 7 | op.rt);
			hbr_tg  = spu_branch_target(0x0, op.i16);
			break;
		}

		case spu_itype::HBRR:
		{
			hbr_loc = spu_branch_target(pos, op.r0h << 7 | op.rt);
			hbr_tg  = spu_branch_target(pos, op.i16);
			break;
		}

		case spu_itype::IL:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.si16;
			break;
		}
		case spu_itype::ILA:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.i18;
			break;
		}
		case spu_itype::ILH:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.i16 << 16 | op.i16;
			break;
		}
		case spu_itype::ILHU:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.i16 << 16;
			break;
		}
		case spu_itype::IOHL:
		{
			m_regmod[pos / 4] = op.rt;
			values[op.rt] = values[op.rt] | op.i16;
			break;
		}
		case spu_itype::ORI:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = values[op.ra] | op.si10;
			break;
		}
		case spu_itype::OR:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vflags[op.rb] & vf::is_const;
			values[op.rt] = values[op.ra] | values[op.rb];
			break;
		}
		case spu_itype::ANDI:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = values[op.ra] & op.si10;
			break;
		}
		case spu_itype::AND:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vflags[op.rb] & vf::is_const;
			values[op.rt] = values[op.ra] & values[op.rb];
			break;
		}
		case spu_itype::AI:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = values[op.ra] + op.si10;
			break;
		}
		case spu_itype::A:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vflags[op.rb] & vf::is_const;
			values[op.rt] = values[op.ra] + values[op.rb];
			break;
		}
		case spu_itype::SFI:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = op.si10 - values[op.ra];
			break;
		}
		case spu_itype::SF:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vflags[op.rb] & vf::is_const;
			values[op.rt] = values[op.rb] - values[op.ra];
			break;
		}
		case spu_itype::ROTMI:
		{
			m_regmod[pos / 4] = op.rt;

			if ((0 - op.i7) & 0x20)
			{
				vflags[op.rt] = +vf::is_const;
				values[op.rt] = 0;
				break;
			}

			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = values[op.ra] >> ((0 - op.i7) & 0x1f);
			break;
		}
		case spu_itype::SHLI:
		{
			m_regmod[pos / 4] = op.rt;

			if (op.i7 & 0x20)
			{
				vflags[op.rt] = +vf::is_const;
				values[op.rt] = 0;
				break;
			}

			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = values[op.ra] << (op.i7 & 0x1f);
			break;
		}

		default:
		{
			// Unconst
			const u32 op_rt = type & spu_itype::_quadrop ? +op.rt4 : +op.rt;
			m_regmod[pos / 4] = op_rt;
			vflags[op_rt] = {};
			break;
		}
		}

		// Insert raw instruction value
		const u32 new_size = (pos - lsa) / 4;

		if (result.data.size() <= new_size)
		{
			if (result.data.size() < new_size)
			{
				result.data.resize(new_size);
			}

			result.data.emplace_back(std::bit_cast<u32, be_t<u32>>(data));
		}
		else if (u32& raw_val = result.data[new_size])
		{
			ensure(raw_val == std::bit_cast<u32, be_t<u32>>(data));
		}
		else
		{
			raw_val = std::bit_cast<u32, be_t<u32>>(data);
		}
	}

	while (lsa > 0 || limit < 0x40000)
	{
		const u32 initial_size = ::size32(result.data);

		// Check unreachable blocks
		limit = std::min<u32>(limit, lsa + initial_size * 4);

		for (auto& pair : m_preds)
		{
			bool reachable = false;

			if (pair.first >= limit)
			{
				continue;
			}

			// All (direct and indirect) predecessors to check
			std::basic_string<u32> workload;

			// Bit array used to deduplicate workload list
			workload.push_back(pair.first);
			m_bits[pair.first / 4] = true;

			for (usz i = 0; !reachable && i < workload.size(); i++)
			{
				for (u32 j = workload[i];; j -= 4)
				{
					// Go backward from an address until the entry point is reached
					if (j == entry_point)
					{
						reachable = true;
						break;
					}

					const auto found = m_preds.find(j);

					bool had_fallthrough = false;

					if (found != m_preds.end())
					{
						for (u32 new_pred : found->second)
						{
							// Check whether the predecessor is previous instruction
							if (new_pred == j - 4)
							{
								had_fallthrough = true;
								continue;
							}

							// Check whether in range and not already added
							if (new_pred >= lsa && new_pred < limit && !m_bits[new_pred / 4])
							{
								workload.push_back(new_pred);
								m_bits[new_pred / 4] = true;
							}
						}
					}

					// Check for possible fallthrough predecessor
					if (!had_fallthrough)
					{
						if (::at32(result.data, (j - lsa) / 4 - 1) == 0 || m_targets.count(j - 4))
						{
							break;
						}
					}

					if (i == 0)
					{
						// TODO
					}
				}
			}

			for (u32 pred : workload)
			{
				m_bits[pred / 4] = false;
			}

			if (!reachable && pair.first < limit)
			{
				limit = pair.first;
			}
		}

		result.data.resize((limit - lsa) / 4);

		// Check holes in safe mode (TODO)
		u32 valid_size = 0;

		for (u32 i = 0; i < result.data.size(); i++)
		{
			if (result.data[i] == 0)
			{
				const u32 pos  = lsa + i * 4;
				const u32 data = ls[pos / 4];

				// Allow only NOP or LNOP instructions in holes
				if (data == 0x200000 || (data & 0xffffff80) == 0x40200000)
				{
					continue;
				}

				if (g_cfg.core.spu_block_size != spu_block_size_type::giga)
				{
					result.data.resize(valid_size);
					break;
				}
			}
			else
			{
				valid_size = i + 1;
			}
		}

		// Even if NOP or LNOP, should be removed at the end
		result.data.resize(valid_size);

		// Repeat if blocks were removed
		if (result.data.size() == initial_size)
		{
			break;
		}
	}

	limit = std::min<u32>(limit, lsa + ::size32(result.data) * 4);

	// Cleanup block info
	for (u32 i = 0; i < workload.size(); i++)
	{
		const u32 addr = workload[i];

		if (addr < lsa || addr >= limit || !result.data[(addr - lsa) / 4])
		{
			m_block_info[addr / 4] = false;
			m_entry_info[addr / 4] = false;
			m_ret_info[addr / 4] = false;
			m_preds.erase(addr);
		}
	}

	// Complete m_preds and associated m_targets for adjacent blocks
	for (auto it = m_preds.begin(); it != m_preds.end();)
	{
		if (it->first < lsa || it->first >= limit)
		{
			it = m_preds.erase(it);
			continue;
		}

		// Erase impossible predecessors
		const auto new_end = std::remove_if(it->second.begin(), it->second.end(), [&](u32 addr)
		{
			return addr < lsa || addr >= limit;
		});

		it->second.erase(new_end, it->second.end());

		// Don't add fallthrough target if all predecessors are removed
		if (it->second.empty() && !m_entry_info[it->first / 4])
		{
			// If not an entry point, remove the block completely
			m_block_info[it->first / 4] = false;
			it = m_preds.erase(it);
			continue;
		}

		// Previous instruction address
		const u32 prev = (it->first - 4) & 0x3fffc;

		// TODO: check the correctness
		if (m_targets.count(prev) == 0 && prev >= lsa && prev < limit && result.data[(prev - lsa) / 4])
		{
			// Add target and the predecessor
			m_targets[prev].push_back(it->first);
			it->second.push_back(prev);
		}

		it++;
	}

	if (out_target_list)
	{
		out_target_list->insert(m_targets.begin(), m_targets.end());
	}

	// Remove unnecessary target lists
	for (auto it = m_targets.begin(); it != m_targets.end();)
	{
		if (it->first < lsa || it->first >= limit)
		{
			it = m_targets.erase(it);
			continue;
		}

		it++;
	}

	// Fill holes which contain only NOP and LNOP instructions (TODO: compile)
	for (u32 i = 0, nnop = 0, vsize = 0; i <= result.data.size(); i++)
	{
		if (i >= result.data.size() || result.data[i])
		{
			if (nnop && nnop == i - vsize)
			{
				// Write only complete NOP sequence
				for (u32 j = vsize; j < i; j++)
				{
					result.data[j] = std::bit_cast<u32, be_t<u32>>(ls[lsa / 4 + j]);
				}
			}

			nnop  = 0;
			vsize = i + 1;
		}
		else
		{
			const u32 pos  = lsa + i * 4;
			const u32 data = ls[pos / 4];

			if (data == 0x200000 || (data & 0xffffff80) == 0x40200000)
			{
				nnop++;
			}
		}
	}

	// Fill block info
	for (auto& pred : m_preds)
	{
		auto& block = m_bbs[pred.first];

		// Copy predeccessors (wrong at this point, needs a fixup later)
		block.preds = pred.second;

		// Fill register usage info
		for (u32 ia = pred.first; ia < limit; ia += 4)
		{
			block.size++;

			// Decode instruction
			const spu_opcode_t op{std::bit_cast<be_t<u32>>(result.data[(ia - lsa) / 4])};

			const auto type = g_spu_itype.decode(op.opcode);

			u8 reg_save = 255;

			if (type == spu_itype::STQD && op.ra == s_reg_sp && !block.reg_mod[op.rt] && !block.reg_use[op.rt])
			{
				// Register saved onto the stack before use
				block.reg_save_dom[op.rt] = true;

				reg_save = op.rt;
			}

			for (auto _use : std::initializer_list<std::pair<u32, bool>>{{op.ra, m_use_ra.test(ia / 4)}
				, {op.rb, m_use_rb.test(ia / 4)}, {op.rc, m_use_rc.test(ia / 4)}})
			{
				if (_use.second)
				{
					const u32 reg = _use.first;

					// Register reg use only if it happens before reg mod
					if (!block.reg_mod[reg])
					{
						block.reg_use.set(reg);

						if (reg_save != reg && block.reg_save_dom[reg])
						{
							// Register is still used after saving; probably not eligible for optimization
							block.reg_save_dom[reg] = false;
						}
					}
				}
			}

			if (type == spu_itype::WRCH && op.ra == MFC_Cmd)
			{
				// Expand MFC_Cmd reg use
				for (u8 reg : {s_reg_mfc_lsa, s_reg_mfc_tag, s_reg_mfc_size})
				{
					if (!block.reg_mod[reg])
						block.reg_use.set(reg);
				}
			}

			// Register reg modification
			if (u8 reg = m_regmod[ia / 4]; reg < s_reg_max)
			{
				block.reg_mod.set(reg);
				block.reg_mod_xf.set(reg, type & spu_itype::xfloat);

				if (type == spu_itype::SELB && (block.reg_mod_xf[op.ra] || block.reg_mod_xf[op.rb]))
					block.reg_mod_xf.set(reg);

				// Possible post-dominating register load
				if (type == spu_itype::LQD && op.ra == s_reg_sp)
					block.reg_load_mod[reg] = ia + 1;
				else
					block.reg_load_mod[reg] = 0;
			}

			// Find targets (also means end of the block)
			const auto tfound = m_targets.find(ia);

			if (tfound != m_targets.end())
			{
				// Copy targets
				block.targets = tfound->second;

				// Assume that the call reads and modifies all volatile registers (TODO)
				bool is_call = false;
				bool is_tail = false;
				switch (type)
				{
				case spu_itype::BRSL:
					is_call = spu_branch_target(ia, op.i16) != ia + 4;
					break;
				case spu_itype::BRASL:
					is_call = spu_branch_target(0, op.i16) != ia + 4;
					break;
				case spu_itype::BISL:
				case spu_itype::BISLED:
					is_call = true;
					break;
				default:
					break;
				}

				if (is_call)
				{
					for (u32 i = 0; i < s_reg_max; ++i)
					{
						if (i == s_reg_lr || (i >= 2 && i < s_reg_80) || i > s_reg_127)
						{
							if (!block.reg_mod[i])
								block.reg_use.set(i);

							if (!is_tail)
							{
								block.reg_mod.set(i);
								block.reg_mod_xf[i] = false;
							}
						}
					}
				}

				break;
			}
		}
	}

	// Fixup block predeccessors to point to basic blocks, not last instructions
	for (auto& bb : m_bbs)
	{
		const u32 addr = bb.first;

		for (u32& pred : bb.second.preds)
		{
			pred = std::prev(m_bbs.upper_bound(pred))->first;
		}

		if (m_entry_info[addr / 4] && g_cfg.core.spu_block_size == spu_block_size_type::giga)
		{
			// Register empty chunk
			m_chunks.push_back(addr);

			// Register function if necessary
			if (!m_ret_info[addr / 4])
			{
				m_funcs[addr];
			}
		}
	}

	// Ensure there is a function at the lowest address
	if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
	{
		if (auto emp = m_funcs.try_emplace(m_bbs.begin()->first); emp.second)
		{
			const u32 addr = emp.first->first;
			spu_log.error("[0x%05x] Fixed first function at 0x%05x", entry_point, addr);
			m_entry_info[addr / 4] = true;
			m_ret_info[addr / 4] = false;
		}
	}

	// Split functions
	while (g_cfg.core.spu_block_size == spu_block_size_type::giga)
	{
		bool need_repeat = false;

		u32 start = 0;
		u32 limit = 0x40000;

		// Walk block list in ascending order
		for (auto& block : m_bbs)
		{
			const u32 addr = block.first;

			if (m_entry_info[addr / 4] && !m_ret_info[addr / 4])
			{
				const auto upper = m_funcs.upper_bound(addr);
				start = addr;
				limit = upper == m_funcs.end() ? 0x40000 : upper->first;
			}

			// Find targets that exceed [start; limit) range and make new functions from them
			for (u32 target : block.second.targets)
			{
				const auto tfound = m_bbs.find(target);

				if (tfound == m_bbs.end())
				{
					continue;
				}

				if (target < start || target >= limit)
				{
					if (!m_entry_info[target / 4] || m_ret_info[target / 4])
					{
						// Create new function entry (likely a tail call)
						m_entry_info[target / 4] = true;

						m_ret_info[target / 4] = false;

						m_funcs.try_emplace(target);

						if (target < limit)
						{
							need_repeat = true;
						}
					}
				}
			}

			block.second.func = start;
		}

		if (!need_repeat)
		{
			break;
		}
	}

	if (!m_bbs.count(entry_point))
	{
		// Invalid code
		spu_log.error("[0x%x] Invalid code", entry_point);
		return {};
	}

	// Fill entry map
	while (true)
	{
		workload.clear();
		workload.push_back(entry_point);
		ensure(m_bbs.count(entry_point));

		std::basic_string<u32> new_entries;

		for (u32 wi = 0; wi < workload.size(); wi++)
		{
			const u32 addr = workload[wi];
			auto& block = ::at32(m_bbs, addr);
			const u32 _new = block.chunk;

			if (!m_entry_info[addr / 4])
			{
				// Check block predecessors
				for (u32 pred : block.preds)
				{
					const u32 _old = ::at32(m_bbs, pred).chunk;

					if (_old < 0x40000 && _old != _new)
					{
						// If block has multiple 'entry' points, it becomes an entry point itself
						new_entries.push_back(addr);
					}
				}
			}

			// Update chunk address
			block.chunk = m_entry_info[addr / 4] ? addr : _new;

			// Process block targets
			for (u32 target : block.targets)
			{
				const auto tfound = m_bbs.find(target);

				if (tfound == m_bbs.end())
				{
					continue;
				}

				auto& tb = tfound->second;

				const u32 value = m_entry_info[target / 4] ? target : block.chunk;

				if (u32& tval = tb.chunk; tval < 0x40000)
				{
					// TODO: fix condition
					if (tval != value && !m_entry_info[target / 4])
					{
						new_entries.push_back(target);
					}
				}
				else
				{
					tval = value;
					workload.emplace_back(target);
				}
			}
		}

		if (new_entries.empty())
		{
			break;
		}

		for (u32 entry : new_entries)
		{
			m_entry_info[entry / 4] = true;

			// Acknowledge artificial (reversible) chunk entry point
			m_ret_info[entry / 4] = true;
		}

		for (auto& bb : m_bbs)
		{
			// Reset chunk info
			bb.second.chunk = 0x40000;
		}
	}

	workload.clear();
	workload.push_back(entry_point);

	// Fill workload adding targets
	for (u32 wi = 0; wi < workload.size(); wi++)
	{
		const u32 addr = workload[wi];
		auto& block = ::at32(m_bbs, addr);
		block.analysed = true;

		for (u32 target : block.targets)
		{
			const auto tfound = m_bbs.find(target);

			if (tfound == m_bbs.end())
			{
				continue;
			}

			auto& tb = tfound->second;

			if (!tb.analysed)
			{
				workload.push_back(target);
				tb.analysed = true;
			}

			// Limited xfloat hint propagation (possibly TODO)
			if (tb.chunk == block.chunk)
			{
				tb.reg_maybe_xf &= block.reg_mod_xf;
			}
			else
			{
				tb.reg_maybe_xf.reset();
			}
		}

		block.reg_origin.fill(0x80000000);
		block.reg_origin_abs.fill(0x80000000);
	}

	// Fill register origin info
	while (true)
	{
		bool must_repeat = false;

		for (u32 wi = 0; wi < workload.size(); wi++)
		{
			const u32 addr = workload[wi];
			auto& block = ::at32(m_bbs, addr);

			// Initialize entry point with default value: unknown origin (requires load)
			if (m_entry_info[addr / 4])
			{
				for (u32 i = 0; i < s_reg_max; i++)
				{
					if (block.reg_origin[i] == 0x80000000)
						block.reg_origin[i] = 0x40000;
				}
			}

			if (g_cfg.core.spu_block_size == spu_block_size_type::giga && m_entry_info[addr / 4] && !m_ret_info[addr / 4])
			{
				for (u32 i = 0; i < s_reg_max; i++)
				{
					if (block.reg_origin_abs[i] == 0x80000000)
						block.reg_origin_abs[i] = 0x40000;
					else if (block.reg_origin_abs[i] + 1 == 0)
						block.reg_origin_abs[i] = -2;
				}
			}

			for (u32 target : block.targets)
			{
				const auto tfound = m_bbs.find(target);

				if (tfound == m_bbs.end())
				{
					continue;
				}

				auto& tb = tfound->second;

				for (u32 i = 0; i < s_reg_max; i++)
				{
					if (tb.chunk == block.chunk && tb.reg_origin[i] + 1)
					{
						const u32 expected = block.reg_mod[i] ? addr : block.reg_origin[i];

						if (tb.reg_origin[i] == 0x80000000)
						{
							tb.reg_origin[i] = expected;
						}
						else if (tb.reg_origin[i] != expected)
						{
							// Set -1 if multiple origins merged (requires PHI node)
							tb.reg_origin[i] = -1;

							must_repeat |= !tb.targets.empty();
						}
					}

					if (g_cfg.core.spu_block_size == spu_block_size_type::giga && tb.func == block.func && tb.reg_origin_abs[i] + 2)
					{
						const u32 expected = block.reg_mod[i] ? addr : block.reg_origin_abs[i];

						if (tb.reg_origin_abs[i] == 0x80000000)
						{
							tb.reg_origin_abs[i] = expected;
						}
						else if (tb.reg_origin_abs[i] != expected)
						{
							if (tb.reg_origin_abs[i] == 0x40000 || expected + 2 == 0 || expected == 0x40000)
							{
								// Set -2: sticky value indicating possible external reg origin (0x40000)
								tb.reg_origin_abs[i] = -2;

								must_repeat |= !tb.targets.empty();
							}
							else if (tb.reg_origin_abs[i] + 1)
							{
								tb.reg_origin_abs[i] = -1;

								must_repeat |= !tb.targets.empty();
							}
						}
					}
				}
			}
		}

		if (!must_repeat)
		{
			break;
		}

		for (u32 wi = 0; wi < workload.size(); wi++)
		{
			const u32 addr = workload[wi];
			auto& block = ::at32(m_bbs, addr);

			// Reset values for the next attempt (keep negative values)
			for (u32 i = 0; i < s_reg_max; i++)
			{
				if (block.reg_origin[i] <= 0x40000)
					block.reg_origin[i] = 0x80000000;
				if (block.reg_origin_abs[i] <= 0x40000)
					block.reg_origin_abs[i] = 0x80000000;
			}
		}
	}

	// Fill more block info
	for (u32 wi = 0; wi < workload.size(); wi++)
	{
		if (g_cfg.core.spu_block_size != spu_block_size_type::giga)
		{
			break;
		}

		const u32 addr = workload[wi];
		auto& bb = ::at32(m_bbs, addr);
		auto& func = ::at32(m_funcs, bb.func);

		// Update function size
		func.size = std::max<u16>(func.size, bb.size + (addr - bb.func) / 4);

		// Copy constants according to reg origin info
		for (u32 i = 0; i < s_reg_max; i++)
		{
			const u32 orig = bb.reg_origin_abs[i];

			if (orig < 0x40000)
			{
				auto& src = ::at32(m_bbs, orig);
				bb.reg_const[i] = src.reg_const[i];
				bb.reg_val32[i] = src.reg_val32[i];
			}

			if (!bb.reg_save_dom[i] && bb.reg_use[i] && (orig == 0x40000 || orig + 2 == 0))
			{
				// Destroy offset if external reg value is used
				func.reg_save_off[i] = -1;
			}
		}

		if (u32 orig = bb.reg_origin_abs[s_reg_sp]; orig < 0x40000)
		{
			auto& prologue = ::at32(m_bbs, orig);

			// Copy stack offset (from the assumed prologue)
			bb.stack_sub = prologue.stack_sub;
		}
		else if (orig > 0x40000)
		{
			// Unpredictable stack
			bb.stack_sub = 0x80000000;
		}

		spu_opcode_t op{};

		auto last_inst = spu_itype::UNK;

		for (u32 ia = addr; ia < addr + bb.size * 4; ia += 4)
		{
			// Decode instruction again
			op.opcode = std::bit_cast<be_t<u32>>(result.data[(ia - lsa) / 41]);
			last_inst = g_spu_itype.decode(op.opcode);

			// Propagate some constants
			switch (last_inst)
			{
			case spu_itype::IL:
			{
				bb.reg_const[op.rt] = true;
				bb.reg_val32[op.rt] = op.si16;
				break;
			}
			case spu_itype::ILA:
			{
				bb.reg_const[op.rt] = true;
				bb.reg_val32[op.rt] = op.i18;
				break;
			}
			case spu_itype::ILHU:
			{
				bb.reg_const[op.rt] = true;
				bb.reg_val32[op.rt] = op.i16 << 16;
				break;
			}
			case spu_itype::ILH:
			{
				bb.reg_const[op.rt] = true;
				bb.reg_val32[op.rt] = op.i16 << 16 | op.i16;
				break;
			}
			case spu_itype::IOHL:
			{
				bb.reg_val32[op.rt] = bb.reg_val32[op.rt] | op.i16;
				break;
			}
			case spu_itype::ORI:
			{
				bb.reg_const[op.rt] = bb.reg_const[op.ra];
				bb.reg_val32[op.rt] = bb.reg_val32[op.ra] | op.si10;
				break;
			}
			case spu_itype::OR:
			{
				bb.reg_const[op.rt] = bb.reg_const[op.ra] && bb.reg_const[op.rb];
				bb.reg_val32[op.rt] = bb.reg_val32[op.ra] | bb.reg_val32[op.rb];
				break;
			}
			case spu_itype::AI:
			{
				bb.reg_const[op.rt] = bb.reg_const[op.ra];
				bb.reg_val32[op.rt] = bb.reg_val32[op.ra] + op.si10;
				break;
			}
			case spu_itype::A:
			{
				bb.reg_const[op.rt] = bb.reg_const[op.ra] && bb.reg_const[op.rb];
				bb.reg_val32[op.rt] = bb.reg_val32[op.ra] + bb.reg_val32[op.rb];
				break;
			}
			case spu_itype::SFI:
			{
				bb.reg_const[op.rt] = bb.reg_const[op.ra];
				bb.reg_val32[op.rt] = op.si10 - bb.reg_val32[op.ra];
				break;
			}
			case spu_itype::SF:
			{
				bb.reg_const[op.rt] = bb.reg_const[op.ra] && bb.reg_const[op.rb];
				bb.reg_val32[op.rt] = bb.reg_val32[op.rb] - bb.reg_val32[op.ra];
				break;
			}
			case spu_itype::STQD:
			{
				if (op.ra == s_reg_sp && bb.stack_sub != 0x80000000 && bb.reg_save_dom[op.rt])
				{
					const u32 offset = 0x80000000 + op.si10 * 16 - bb.stack_sub;

					if (func.reg_save_off[op.rt] == 0)
					{
						// Store reg save offset
						func.reg_save_off[op.rt] = offset;
					}
					else if (func.reg_save_off[op.rt] != offset)
					{
						// Conflict of different offsets
						func.reg_save_off[op.rt] = -1;
					}
				}

				break;
			}
			case spu_itype::LQD:
			{
				if (op.ra == s_reg_sp && bb.stack_sub != 0x80000000 && bb.reg_load_mod[op.rt] == ia + 1)
				{
					// Adjust reg load offset
					bb.reg_load_mod[op.rt] = 0x80000000 + op.si10 * 16 - bb.stack_sub;
				}

				// Clear const
				bb.reg_const[op.rt] = false;
				break;
			}
			default:
			{
				// Clear const if reg is modified here
				if (u8 reg = m_regmod[ia / 4]; reg < s_reg_max)
					bb.reg_const[reg] = false;
				break;
			}
			}

			// $SP is modified
			if (m_regmod[ia / 4] == s_reg_sp)
			{
				if (bb.reg_const[s_reg_sp])
				{
					// Making $SP a constant is a funny thing too.
					bb.stack_sub = 0x80000000;
				}

				if (bb.stack_sub != 0x80000000)
				{
					switch (last_inst)
					{
					case spu_itype::AI:
					{
						if (op.ra == s_reg_sp)
							bb.stack_sub -= op.si10;
						else
							bb.stack_sub = 0x80000000;
						break;
					}
					case spu_itype::A:
					{
						if (op.ra == s_reg_sp && bb.reg_const[op.rb])
							bb.stack_sub -= bb.reg_val32[op.rb];
						else if (op.rb == s_reg_sp && bb.reg_const[op.ra])
							bb.stack_sub -= bb.reg_val32[op.ra];
						else
							bb.stack_sub = 0x80000000;
						break;
					}
					case spu_itype::SF:
					{
						if (op.rb == s_reg_sp && bb.reg_const[op.ra])
							bb.stack_sub += bb.reg_val32[op.ra];
						else
							bb.stack_sub = 0x80000000;
						break;
					}
					default:
					{
						bb.stack_sub = 0x80000000;
						break;
					}
					}
				}

				// Check for funny values.
				if (bb.stack_sub >= 0x40000 || bb.stack_sub % 16)
				{
					bb.stack_sub = 0x80000000;
				}
			}
		}

		// Analyse terminator instruction
		const u32 tia = addr + bb.size * 4 - 4;

		switch (last_inst)
		{
		case spu_itype::BR:
		case spu_itype::BRNZ:
		case spu_itype::BRZ:
		case spu_itype::BRHNZ:
		case spu_itype::BRHZ:
		case spu_itype::BRSL:
		{
			const u32 target = spu_branch_target(tia, op.i16);

			if (target == tia + 4)
			{
				bb.terminator = term_type::fallthrough;
			}
			else if (last_inst != spu_itype::BRSL)
			{
				// No-op terminator or simple branch instruction
				bb.terminator = term_type::br;

				if (target == bb.func)
				{
					// Recursive tail call
					bb.terminator = term_type::ret;
				}
			}
			else if (op.rt == s_reg_lr)
			{
				bb.terminator = term_type::call;
			}
			else
			{
				bb.terminator = term_type::interrupt_call;
			}

			break;
		}
		case spu_itype::BRA:
		case spu_itype::BRASL:
		{
			bb.terminator = term_type::indirect_call;
			break;
		}
		case spu_itype::BI:
		{
			if (op.d || op.e || bb.targets.size() == 1)
			{
				bb.terminator = term_type::interrupt_call;
			}
			else if (bb.targets.size() > 1)
			{
				// Jump table
				bb.terminator = term_type::br;
			}
			else if (op.ra == s_reg_lr)
			{
				// Return (TODO)
				bb.terminator = term_type::ret;
			}
			else
			{
				// Indirect tail call (TODO)
				bb.terminator = term_type::interrupt_call;
			}

			break;
		}
		case spu_itype::BISLED:
		case spu_itype::IRET:
		{
			bb.terminator = term_type::interrupt_call;
			break;
		}
		case spu_itype::BISL:
		case spu_itype::BIZ:
		case spu_itype::BINZ:
		case spu_itype::BIHZ:
		case spu_itype::BIHNZ:
		{
			if (op.d || op.e || bb.targets.size() != 1)
			{
				bb.terminator = term_type::interrupt_call;
			}
			else if (last_inst != spu_itype::BISL && bb.targets[0] == tia + 4 && op.ra == s_reg_lr)
			{
				// Conditional return (TODO)
				bb.terminator = term_type::ret;
			}
			else if (last_inst == spu_itype::BISL)
			{
				// Indirect call
				bb.terminator = term_type::indirect_call;
			}
			else
			{
				// TODO
				bb.terminator = term_type::interrupt_call;
			}

			break;
		}
		default:
		{
			// Normal instruction
			bb.terminator = term_type::fallthrough;
			break;
		}
		}
	}

	// Check function blocks, verify and print some reasons
	for (auto& f : m_funcs)
	{
		if (g_cfg.core.spu_block_size != spu_block_size_type::giga)
		{
			break;
		}

		bool is_ok = true;

		u32 used_stack = 0;

		for (auto it = m_bbs.lower_bound(f.first); it != m_bbs.end() && it->second.func == f.first; ++it)
		{
			auto& bb = it->second;
			auto& func = ::at32(m_funcs, bb.func);
			const u32 addr = it->first;
			const u32 flim = bb.func + func.size * 4;

			used_stack |= bb.stack_sub;

			if (is_ok && bb.terminator >= term_type::indirect_call)
			{
				is_ok = false;
			}

			if (is_ok && bb.terminator == term_type::ret)
			{
				// Check $LR (alternative return registers are currently not supported)
				if (u32 lr_orig = bb.reg_mod[s_reg_lr] ? addr : bb.reg_origin_abs[s_reg_lr]; lr_orig < 0x40000)
				{
					auto& src = ::at32(m_bbs, lr_orig);

					if (src.reg_load_mod[s_reg_lr] != func.reg_save_off[s_reg_lr])
					{
						spu_log.error("Function 0x%05x: [0x%05x] $LR mismatch (src=0x%x; 0x%x vs 0x%x)", f.first, addr, lr_orig, src.reg_load_mod[0], func.reg_save_off[0]);
						is_ok = false;
					}
					else if (src.reg_load_mod[s_reg_lr] == 0)
					{
						spu_log.error("Function 0x%05x: [0x%05x] $LR modified (src=0x%x)", f.first, addr, lr_orig);
						is_ok = false;
					}
				}
				else if (lr_orig > 0x40000)
				{
					spu_log.todo("Function 0x%05x: [0x%05x] $LR unpredictable (src=0x%x)", f.first, addr, lr_orig);
					is_ok = false;
				}

				// Check $80..$127 (should be restored or unmodified)
				for (u32 i = s_reg_80; is_ok && i <= s_reg_127; i++)
				{
					if (u32 orig = bb.reg_mod[i] ? addr : bb.reg_origin_abs[i]; orig < 0x40000)
					{
						auto& src = ::at32(m_bbs, orig);

						if (src.reg_load_mod[i] != func.reg_save_off[i])
						{
							spu_log.error("Function 0x%05x: [0x%05x] $%u mismatch (src=0x%x; 0x%x vs 0x%x)", f.first, addr, i, orig, src.reg_load_mod[i], func.reg_save_off[i]);
							is_ok = false;
						}
					}
					else if (orig > 0x40000)
					{
						spu_log.todo("Function 0x%05x: [0x%05x] $%u unpredictable (src=0x%x)", f.first, addr, i, orig);
						is_ok = false;
					}

					if (func.reg_save_off[i] + 1 == 0)
					{
						spu_log.error("Function 0x%05x: [0x%05x] $%u used incorrectly", f.first, addr, i);
						is_ok = false;
					}
				}

				// Check $SP (should be restored or unmodified)
				if (bb.stack_sub != 0 && bb.stack_sub != 0x80000000)
				{
					spu_log.error("Function 0x%05x: [0x%05x] return with stack frame 0x%x", f.first, addr, bb.stack_sub);
					is_ok = false;
				}
			}

			if (is_ok && bb.terminator == term_type::call)
			{
				// Check call instruction (TODO)
				if (bb.stack_sub == 0)
				{
					// Call without a stack frame
					spu_log.error("Function 0x%05x: [0x%05x] frameless call", f.first, addr);
					is_ok = false;
				}
			}

			if (is_ok && bb.terminator == term_type::fallthrough)
			{
				// Can't just fall out of the function
				if (bb.targets.size() != 1 || bb.targets[0] >= flim)
				{
					spu_log.error("Function 0x%05x: [0x%05x] bad fallthrough to 0x%x", f.first, addr, bb.targets[0]);
					is_ok = false;
				}
			}

			if (is_ok && bb.stack_sub == 0x80000000)
			{
				spu_log.error("Function 0x%05x: [0x%05x] bad stack frame", f.first, addr);
				is_ok = false;
			}

			// Fill external function targets (calls, possibly tail calls)
			for (u32 target : bb.targets)
			{
				if (target < bb.func || target >= flim || (bb.terminator == term_type::call && target == bb.func))
				{
					if (func.calls.find_first_of(target) + 1 == 0)
					{
						func.calls.push_back(target);
					}
				}
			}
		}

		if (is_ok && used_stack && f.first == entry_point)
		{
			spu_log.error("Function 0x%05x: considered possible chunk", f.first);
			is_ok = false;
		}

		// if (is_ok && f.first > 0x1d240 && f.first < 0x1e000)
		// {
		// 	spu_log.error("Function 0x%05x: manually disabled", f.first);
		// 	is_ok = false;
		// }

		f.second.good = is_ok;
	}

	// Check function call graph
	while (g_cfg.core.spu_block_size == spu_block_size_type::giga)
	{
		bool need_repeat = false;

		for (auto& f : m_funcs)
		{
			if (!f.second.good)
			{
				continue;
			}

			for (u32 call : f.second.calls)
			{
				const auto ffound = std::as_const(m_funcs).find(call);

				if (ffound == m_funcs.cend() || ffound->second.good == false)
				{
					need_repeat = true;

					if (f.second.good)
					{
						spu_log.error("Function 0x%05x: calls bad function (0x%05x)", f.first, call);
						f.second.good = false;
					}
				}
			}
		}

		if (!need_repeat)
		{
			break;
		}
	}

	if (result.data.empty())
	{
		// Blocks starting from 0x0 or invalid instruction won't be compiled, may need special interpreter fallback
	}

	return result;
}

void spu_recompiler_base::dump(const spu_program& result, std::string& out)
{
	SPUDisAsm dis_asm(cpu_disasm_mode::dump, reinterpret_cast<const u8*>(result.data.data()), result.lower_bound);

	std::string hash;

	if (!result.data.empty())
	{
		sha1_context ctx;
		u8 output[20];

		sha1_starts(&ctx);
		sha1_update(&ctx, reinterpret_cast<const u8*>(result.data.data()), result.data.size() * 4);
		sha1_finish(&ctx, output);
		fmt::append(hash, "%s", fmt::base57(output));
	}
	else
	{
		hash = "N/A";
	}

	fmt::append(out, "========== SPU BLOCK 0x%05x (size %u, %s) ==========\n\n", result.entry_point, result.data.size(), hash);

	for (auto& bb : m_bbs)
	{
		for (u32 pos = bb.first, end = bb.first + bb.second.size * 4; pos < end; pos += 4)
		{
			dis_asm.disasm(pos);

			if (!dis_asm.last_opcode.ends_with('\n'))
			{
				dis_asm.last_opcode += '\n';
			}

			fmt::append(out, ">%s", dis_asm.last_opcode);
		}

		out += '\n';

		if (m_block_info[bb.first / 4])
		{
			fmt::append(out, "A: [0x%05x] %s\n", bb.first, m_entry_info[bb.first / 4] ? (m_ret_info[bb.first / 4] ? "Chunk" : "Entry") : "Block");

			fmt::append(out, "\tF: 0x%05x\n", bb.second.func);

			for (u32 pred : bb.second.preds)
			{
				fmt::append(out, "\t<- 0x%05x\n", pred);
			}

			for (u32 target : bb.second.targets)
			{
				fmt::append(out, "\t-> 0x%05x%s\n", target, m_bbs.count(target) ? "" : " (null)");
			}
		}
		else
		{
			fmt::append(out, "A: [0x%05x] ?\n", bb.first);
		}

		out += '\n';
	}

	for (auto& f : m_funcs)
	{
		fmt::append(out, "F: [0x%05x]%s\n", f.first, f.second.good ? " (good)" : " (bad)");

		fmt::append(out, "\tN: 0x%05x\n", f.second.size * 4 + f.first);

		for (u32 call : f.second.calls)
		{
			fmt::append(out, "\t>> 0x%05x%s\n", call, m_funcs.count(call) ? "" : " (null)");
		}
	}

	out += '\n';
}

struct spu_llvm_worker
{
	lf_queue<std::pair<u64, const spu_program*>> registered;

	void operator()()
	{
		// SPU LLVM Recompiler instance
		std::unique_ptr<spu_recompiler_base> compiler;

		// Fake LS
		std::vector<be_t<u32>> ls;

		bool set_relax_flag = false;

		for (auto slice = registered.pop_all();; [&]
		{
			if (slice)
			{
				slice.pop_front();
			}

			if (slice || thread_ctrl::state() == thread_state::aborting)
			{
				return;
			}

			if (set_relax_flag)
			{
				spu_thread::g_spu_work_count--;
				set_relax_flag = false;
			}

			thread_ctrl::wait_on(utils::bless<atomic_t<u32>>(&registered)[1], 0);
			slice = registered.pop_all();
		}())
		{
			auto* prog = slice.get();

			if (thread_ctrl::state() == thread_state::aborting)
			{
				break;
			}

			if (!prog)
			{
				continue;
			}

			if (!prog->second)
			{
				break;
			}

			if (!compiler)
			{
				// Postponed initialization
				compiler = spu_recompiler_base::make_llvm_recompiler();
				compiler->init();

				ls.resize(SPU_LS_SIZE / sizeof(be_t<u32>));
			}

			if (!set_relax_flag)
			{
				spu_thread::g_spu_work_count++;
				set_relax_flag = true;
			}

			const auto& func = *prog->second;

			// Get data start
			const u32 start = func.lower_bound;
			const u32 size0 = ::size32(func.data);

			// Initialize LS with function data only
			for (u32 i = 0, pos = start; i < size0; i++, pos += 4)
			{
				ls[pos / 4] = std::bit_cast<be_t<u32>>(func.data[i]);
			}

			// Call analyser
			spu_program func2 = compiler->analyse(ls.data(), func.entry_point);

			if (func2 != func)
			{
				spu_log.error("[0x%05x] SPU Analyser failed, %u vs %u", func2.entry_point, func2.data.size(), size0);
			}
			else if (const auto target = compiler->compile(std::move(func2)))
			{
				// Redirect old function (TODO: patch in multiple places)
				const s64 rel = reinterpret_cast<u64>(target) - prog->first - 5;

				union
				{
					u8 bytes[8];
					u64 result;
				};

				bytes[0] = 0xe9; // jmp rel32
				std::memcpy(bytes + 1, &rel, 4);
				bytes[5] = 0x90;
				bytes[6] = 0x90;
				bytes[7] = 0x90;

				atomic_storage<u64>::release(*reinterpret_cast<u64*>(prog->first), result);
			}
			else
			{
				spu_log.fatal("[0x%05x] Compilation failed.", func.entry_point);
				break;
			}

			// Clear fake LS
			std::memset(ls.data() + start / 4, 0, 4 * (size0 - 1));
		}

		if (set_relax_flag)
		{
			spu_thread::g_spu_work_count--;
			set_relax_flag = false;
		}
	}
};

// SPU LLVM recompiler thread context
struct spu_llvm
{
	// Workload
	lf_queue<std::pair<const u64, spu_item*>> registered;
	atomic_ptr<named_thread_group<spu_llvm_worker>> m_workers;

	spu_llvm()
	{
		// Dependency
		g_fxo->init<spu_cache>();
	}

	void operator()()
	{
		if (g_cfg.core.spu_decoder != spu_decoder_type::llvm)
		{
			return;
		}

		while (!registered && thread_ctrl::state() != thread_state::aborting)
		{
			// Wait for the first SPU block before launching any thread
			thread_ctrl::wait_on(utils::bless<atomic_t<u32>>(&registered)[1], 0);
		}

		if (thread_ctrl::state() == thread_state::aborting)
		{
			return;
		}

		// To compile (hash -> item)
		std::unordered_multimap<u64, spu_item*, value_hash<u64>> enqueued;

		// Mini-profiler (hash -> number of occurrences)
		std::unordered_map<u64, atomic_t<u64>, value_hash<u64>> samples;

		// For synchronization with profiler thread
		stx::init_mutex prof_mutex;

		named_thread profiler("SPU LLVM Profiler"sv, [&]()
		{
			while (thread_ctrl::state() != thread_state::aborting)
			{
				{
					// Lock if enabled
					const auto lock = prof_mutex.access();

					if (!lock)
					{
						// Wait when the profiler is disabled
						prof_mutex.wait_for_initialized();
						continue;
					}

					// Collect profiling samples
					idm::select<named_thread<spu_thread>>([&](u32 /*id*/, spu_thread& spu)
					{
						const u64 name = atomic_storage<u64>::load(spu.block_hash);

						if (auto state = +spu.state; !::is_paused(state) && !::is_stopped(state) && cpu_flag::wait - state)
						{
							const auto found = std::as_const(samples).find(name);

							if (found != std::as_const(samples).end())
							{
								const_cast<atomic_t<u64>&>(found->second)++;
							}
						}
					});
				}

				// Sleep for a short period if enabled
				thread_ctrl::wait_for(20, false);
			}
		});

		u32 worker_count = 1;

		if (uint hc = utils::get_thread_count(); hc >= 12)
		{
			worker_count = hc - 12 + 3;
		}
		else if (hc >= 6)
		{
			worker_count = 2;
		}

		u32 worker_index = 0;
		u32 notify_compile_count = 0;
		u32 compile_pending = 0;
		std::vector<u8> notify_compile(worker_count);

		m_workers = make_single<named_thread_group<spu_llvm_worker>>("SPUW.", worker_count);
		auto workers_ptr = m_workers.load();
		auto& workers = *workers_ptr;

		while (thread_ctrl::state() != thread_state::aborting)
		{
			for (const auto& pair : registered.pop_all())
			{
				enqueued.emplace(pair);

				// Interrupt and kick profiler thread
				const auto lock = prof_mutex.init_always([&]{});

				// Register new blocks to collect samples
				samples.emplace(pair.first, 0);
			}

			if (enqueued.empty())
			{
				// Send pending notifications
				if (notify_compile_count)
				{
					for (usz i = 0; i < worker_count; i++)
					{
						if (notify_compile[i])
						{
							(workers.begin() + i)->registered.notify();
						}
					}
				}

				// Interrupt profiler thread and put it to sleep
				static_cast<void>(prof_mutex.reset());
				thread_ctrl::wait_on(utils::bless<atomic_t<u32>>(&registered)[1], 0);
				std::fill(notify_compile.begin(), notify_compile.end(), 0); // Reset notification flags
				notify_compile_count = 0;
				compile_pending = 0;
				continue;
			}

			// Find the most used enqueued item
			u64 sample_max = 0;
			auto found_it  = enqueued.begin();

			for (auto it = enqueued.begin(), end = enqueued.end(); it != end; ++it)
			{
				const u64 cur = ::at32(std::as_const(samples), it->first);

				if (cur > sample_max)
				{
					sample_max = cur;
					found_it = it;
				}
			}

			// Start compiling
			const spu_program& func = found_it->second->data;

			// Old function pointer (pre-recompiled)
			const spu_function_t _old = found_it->second->compiled;

			// Remove item from the queue
			enqueued.erase(found_it);

			// Prefer using an inactive thread
			for (usz i = 0; i < worker_count && !!(workers.begin() + (worker_index % worker_count))->registered; i++)
			{
				worker_index++;
			}

			// Push the workload
			const bool notify = (workers.begin() + (worker_index % worker_count))->registered.template push<false>(reinterpret_cast<u64>(_old), &func);

			if (notify && !notify_compile[worker_index % worker_count])
			{
				notify_compile[worker_index % worker_count] = 1;
				notify_compile_count++;
			}

			compile_pending++;

			// Notify all before queue runs out if there is considerable excess
			// Optimized that: if there are many workers, it acts soon
			// If there are only a few workers, it postpones notifications until there is some more workload
			if (notify_compile_count && std::min<u32>(7, utils::aligned_div<u32>(worker_count * 2, 3) + 2) <= compile_pending)
			{
				for (usz i = 0; i < worker_count; i++)
				{
					if (notify_compile[i])
					{
						(workers.begin() + i)->registered.notify();
					}
				}

				std::fill(notify_compile.begin(), notify_compile.end(), 0); // Reset notification flags
				notify_compile_count = 0;
				compile_pending = 0;
			}

			worker_index++;
		}

		static_cast<void>(prof_mutex.init_always([&]{ samples.clear(); }));

		m_workers.reset();

		for (u32 i = 0; i < worker_count; i++)
		{
			(workers.begin() + i)->operator=(thread_state::aborting);
		}
	}

	spu_llvm& operator=(thread_state)
	{
		if (const auto workers = m_workers.load())
		{
			for (u32 i = 0; i < workers->size(); i++)
			{
				(workers->begin() + i)->operator=(thread_state::aborting);
			}
		}

		return *this;
	}

	static constexpr auto thread_name = "SPU LLVM"sv;
};

using spu_llvm_thread = named_thread<spu_llvm>;

struct spu_fast : public spu_recompiler_base
{
	virtual void init() override
	{
		if (!m_spurt)
		{
			m_spurt = &g_fxo->get<spu_runtime>();
		}
	}

	virtual spu_function_t compile(spu_program&& _func) override
	{
		const auto add_loc = m_spurt->add_empty(std::move(_func));

		if (!add_loc)
		{
			return nullptr;
		}

		if (add_loc->compiled)
		{
			return add_loc->compiled;
		}

		const spu_program& func = add_loc->data;

		if (g_cfg.core.spu_debug && !add_loc->logged.exchange(1))
		{
			std::string log;
			this->dump(func, log);
			fs::write_file(m_spurt->get_cache_path() + "spu.log", fs::create + fs::write + fs::append, log);
		}

		// Allocate executable area with necessary size
		const auto result = jit_runtime::alloc(22 + 1 + 9 + ::size32(func.data) * (16 + 16) + 36 + 47, 16);

		if (!result)
		{
			return nullptr;
		}

		m_pos = func.lower_bound;
		m_size = ::size32(func.data) * 4;

		{
			sha1_context ctx;
			u8 output[20];

			sha1_starts(&ctx);
			sha1_update(&ctx, reinterpret_cast<const u8*>(func.data.data()), func.data.size() * 4);
			sha1_finish(&ctx, output);

			be_t<u64> hash_start;
			std::memcpy(&hash_start, output, sizeof(hash_start));
			m_hash_start = hash_start;
		}

		u8* raw = result;

		// 8-byte intruction for patching (long NOP)
		*raw++ = 0x0f;
		*raw++ = 0x1f;
		*raw++ = 0x84;
		*raw++ = 0;
		*raw++ = 0;
		*raw++ = 0;
		*raw++ = 0;
		*raw++ = 0;

		// mov rax, m_hash_start
		*raw++ = 0x48;
		*raw++ = 0xb8;
		std::memcpy(raw, &m_hash_start, sizeof(m_hash_start));
		raw += 8;

		// Update block_hash: mov [r13 + spu_thread::m_block_hash], rax
		*raw++ = 0x49;
		*raw++ = 0x89;
		*raw++ = 0x45;
		*raw++ = ::narrow<s8>(::offset32(&spu_thread::block_hash));

		// Load PC: mov eax, [r13 + spu_thread::pc]
		*raw++ = 0x41;
		*raw++ = 0x8b;
		*raw++ = 0x45;
		*raw++ = ::narrow<s8>(::offset32(&spu_thread::pc));

		// Get LS address starting from PC: lea rcx, [rbp + rax]
		*raw++ = 0x48;
		*raw++ = 0x8d;
		*raw++ = 0x4c;
		*raw++ = 0x05;
		*raw++ = 0x00;

		// Verification (slow)
		for (u32 i = 0; i < func.data.size(); i++)
		{
			if (!func.data[i])
			{
				continue;
			}

			// cmp dword ptr [rcx + off], opc
			*raw++ = 0x81;
			*raw++ = 0xb9;
			const u32 off = i * 4;
			const u32 opc = func.data[i];
			std::memcpy(raw + 0, &off, 4);
			std::memcpy(raw + 4, &opc, 4);
			raw += 8;

			// jne tr_dispatch
			const s64 rel = reinterpret_cast<u64>(spu_runtime::tr_dispatch) - reinterpret_cast<u64>(raw) - 6;
			*raw++ = 0x0f;
			*raw++ = 0x85;
			std::memcpy(raw + 0, &rel, 4);
			raw += 4;
		}

		// trap
		//*raw++ = 0xcc;

		// Secondary prologue: sub rsp,0x28
		*raw++ = 0x48;
		*raw++ = 0x83;
		*raw++ = 0xec;
		*raw++ = 0x28;

		// Fix args: xchg r13,rbp
		*raw++ = 0x49;
		*raw++ = 0x87;
		*raw++ = 0xed;

		// mov r12d, eax
		*raw++ = 0x41;
		*raw++ = 0x89;
		*raw++ = 0xc4;

		// mov esi, 0x7f0
		*raw++ = 0xbe;
		*raw++ = 0xf0;
		*raw++ = 0x07;
		*raw++ = 0x00;
		*raw++ = 0x00;

		// lea rdi, [rbp + spu_thread::gpr]
		*raw++ = 0x48;
		*raw++ = 0x8d;
		*raw++ = 0x7d;
		*raw++ = ::narrow<s8>(::offset32(&spu_thread::gpr));

		// Save base pc: mov [rbp + spu_thread::base_pc], eax
		*raw++ = 0x89;
		*raw++ = 0x45;
		*raw++ = ::narrow<s8>(::offset32(&spu_thread::base_pc));

		// inc block_counter
		*raw++ = 0x48;
		*raw++ = 0xff;
		*raw++ = 0x85;
		const u32 blc_off = ::offset32(&spu_thread::block_counter);
		std::memcpy(raw, &blc_off, 4);
		raw += 4;

		// lea r14, [local epilogue]
		*raw++ = 0x4c;
		*raw++ = 0x8d;
		*raw++ = 0x35;
		const u32 epi_off = ::size32(func.data) * 16;
		std::memcpy(raw, &epi_off, 4);
		raw += 4;

		// Instructions (each instruction occupies fixed number of bytes)
		for (u32 i = 0; i < func.data.size(); i++)
		{
			const u32 pos = m_pos + i * 4;

			if (!func.data[i])
			{
				// Save pc: mov [rbp + spu_thread::pc], r12d
				*raw++ = 0x44;
				*raw++ = 0x89;
				*raw++ = 0x65;
				*raw++ = ::narrow<s8>(::offset32(&spu_thread::pc));

				// Epilogue: add rsp,0x28
				*raw++ = 0x48;
				*raw++ = 0x83;
				*raw++ = 0xc4;
				*raw++ = 0x28;

				// ret (TODO)
				*raw++ = 0xc3;
				std::memset(raw, 0xcc, 16 - 9);
				raw += 16 - 9;
				continue;
			}

			// Fix endianness
			const spu_opcode_t op{std::bit_cast<be_t<u32>>(func.data[i])};

			switch (auto type = g_spu_itype.decode(op.opcode))
			{
			case spu_itype::BRZ:
			case spu_itype::BRHZ:
			case spu_itype::BRNZ:
			case spu_itype::BRHNZ:
			{
				const u32 target = spu_branch_target(pos, op.i16);

				if (0 && target >= m_pos && target < m_pos + m_size)
				{
					*raw++ = type == spu_itype::BRHZ || type == spu_itype::BRHNZ ? 0x66 : 0x90;
					*raw++ = 0x83;
					*raw++ = 0xbd;
					const u32 off = ::offset32(&spu_thread::gpr, op.rt) + 12;
					std::memcpy(raw, &off, 4);
					raw += 4;
					*raw++ = 0x00;

					*raw++ = 0x0f;
					*raw++ = type == spu_itype::BRZ || type == spu_itype::BRHZ ? 0x84 : 0x85;
					const u32 dif = (target - (pos + 4)) / 4 * 16 + 2;
					std::memcpy(raw, &dif, 4);
					raw += 4;

					*raw++ = 0x66;
					*raw++ = 0x90;
					break;
				}

				[[fallthrough]];
			}
			default:
			{
				// Ballast: mov r15d, pos
				*raw++ = 0x41;
				*raw++ = 0xbf;
				std::memcpy(raw, &pos, 4);
				raw += 4;

				// mov ebx, opc
				*raw++ = 0xbb;
				std::memcpy(raw, &op, 4);
				raw += 4;

				// call spu_* (specially built interpreter function)
				const s64 rel = spu_runtime::g_interpreter_table[static_cast<usz>(type)] - reinterpret_cast<u64>(raw) - 5;
				*raw++ = 0xe8;
				std::memcpy(raw, &rel, 4);
				raw += 4;
				break;
			}
			}
		}

		// Local dispatcher/epilogue: fix stack after branch instruction, then dispatch or return

		// add rsp, 8
		*raw++ = 0x48;
		*raw++ = 0x83;
		*raw++ = 0xc4;
		*raw++ = 0x08;

		// and rsp, -16
		*raw++ = 0x48;
		*raw++ = 0x83;
		*raw++ = 0xe4;
		*raw++ = 0xf0;

		// lea rax, [r12 - size]
		*raw++ = 0x49;
		*raw++ = 0x8d;
		*raw++ = 0x84;
		*raw++ = 0x24;
		const u32 msz = 0u - m_size;
		std::memcpy(raw, &msz, 4);
		raw += 4;

		// sub eax, [rbp + spu_thread::base_pc]
		*raw++ = 0x2b;
		*raw++ = 0x45;
		*raw++ = ::narrow<s8>(::offset32(&spu_thread::base_pc));

		// cmp eax, (0 - size)
		*raw++ = 0x3d;
		std::memcpy(raw, &msz, 4);
		raw += 4;

		// jb epilogue
		*raw++ = 0x72;
		*raw++ = +12;

		// movsxd rax, eax
		*raw++ = 0x48;
		*raw++ = 0x63;
		*raw++ = 0xc0;

		// shl rax, 2
		*raw++ = 0x48;
		*raw++ = 0xc1;
		*raw++ = 0xe0;
		*raw++ = 0x02;

		// add rax, r14
		*raw++ = 0x4c;
		*raw++ = 0x01;
		*raw++ = 0xf0;

		// jmp rax
		*raw++ = 0xff;
		*raw++ = 0xe0;

		// Save pc: mov [rbp + spu_thread::pc], r12d
		*raw++ = 0x44;
		*raw++ = 0x89;
		*raw++ = 0x65;
		*raw++ = ::narrow<s8>(::offset32(&spu_thread::pc));

		// Epilogue: add rsp,0x28 ; ret
		*raw++ = 0x48;
		*raw++ = 0x83;
		*raw++ = 0xc4;
		*raw++ = 0x28;
		*raw++ = 0xc3;

		const auto fn = reinterpret_cast<spu_function_t>(result);

		// Install pointer carefully
		const bool added = !add_loc->compiled && add_loc->compiled.compare_and_swap_test(nullptr, fn);

		// Check hash against allowed bounds
		const bool inverse_bounds = g_cfg.core.spu_llvm_lower_bound > g_cfg.core.spu_llvm_upper_bound;

		if ((!inverse_bounds && (m_hash_start < g_cfg.core.spu_llvm_lower_bound || m_hash_start > g_cfg.core.spu_llvm_upper_bound)) ||
			(inverse_bounds && (m_hash_start < g_cfg.core.spu_llvm_lower_bound && m_hash_start > g_cfg.core.spu_llvm_upper_bound)))
		{
			spu_log.error("[Debug] Skipped function %s", fmt::base57(be_t<u64>{m_hash_start}));
		}
		else if (added)
		{
			// Send work to LLVM compiler thread
			g_fxo->get<spu_llvm_thread>().registered.push(m_hash_start, add_loc);
		}

		// Rebuild trampoline if necessary
		if (!m_spurt->rebuild_ubertrampoline(func.data[0]))
		{
			return nullptr;
		}

		if (added)
		{
			add_loc->compiled.notify_all();
		}

		return fn;
	}
};

std::unique_ptr<spu_recompiler_base> spu_recompiler_base::make_fast_llvm_recompiler()
{
	return std::make_unique<spu_fast>();
}

extern std::string format_spu_func_info(u32 addr, cpu_thread* spu)
{
	spu_thread* _spu = static_cast<spu_thread*>(spu);

	std::unique_ptr<spu_recompiler_base> compiler = spu_recompiler_base::make_asmjit_recompiler();
	compiler->init();
	auto func = compiler->analyse(reinterpret_cast<const be_t<u32>*>(_spu->ls), addr);

	std::string info;
	{
		sha1_context ctx;
		u8 output[20];

		sha1_starts(&ctx);
		sha1_update(&ctx, reinterpret_cast<const u8*>(func.data.data()), func.data.size() * 4);
		sha1_finish(&ctx, output);
		fmt::append(info, "size=%d, end=0x%x, hash=%s", func.data.size(), addr + func.data.size() * 4, fmt::base57(output));
	}

	return info;
}
