#pragma once

#include "util/types.hpp"

// Include asmjit with warnings ignored
#define ASMJIT_EMBED
#define ASMJIT_STATIC
#define ASMJIT_BUILD_DEBUG
#undef Bool

#ifdef _MSC_VER
#pragma warning(push, 0)
#include <asmjit/asmjit.h>
#pragma warning(pop)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Weffc++"
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdeprecated-anon-enum-enum-conversion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#else
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#endif
#include <asmjit/asmjit.h>
#if defined(ARCH_ARM64)
#include <asmjit/a64.h>
#endif
#pragma GCC diagnostic pop
#endif

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#if defined(ARCH_X64)
using native_asm = asmjit::x86::Assembler;
using native_args = std::array<asmjit::x86::Gp, 4>;
#elif defined(ARCH_ARM64)
using native_asm = asmjit::a64::Assembler;
using native_args = std::array<asmjit::a64::Gp, 4>;
#endif

void jit_announce(uptr func, usz size, std::string_view name);

void jit_announce(auto* func, usz size, std::string_view name)
{
	jit_announce(uptr(func), size, name);
}

enum class jit_class
{
	ppu_code,
	ppu_data,
	spu_code,
	spu_data,
};

struct jit_runtime_base
{
	jit_runtime_base() noexcept = default;
	virtual ~jit_runtime_base() = default;

	jit_runtime_base(const jit_runtime_base&) = delete;
	jit_runtime_base& operator=(const jit_runtime_base&) = delete;

	const asmjit::Environment& environment() const noexcept;
	void* _add(asmjit::CodeHolder* code) noexcept;
	virtual uchar* _alloc(usz size, usz align) noexcept = 0;
};

// ASMJIT runtime for emitting code in a single 2G region
struct jit_runtime final : jit_runtime_base
{
	jit_runtime();
	~jit_runtime() override;

	// Allocate executable memory
	uchar* _alloc(usz size, usz align) noexcept override;

	// Allocate memory
	static u8* alloc(usz size, uint align, bool exec = true) noexcept;

	// Should be called at least once after global initialization
	static void initialize();

	// Deallocate all memory
	static void finalize() noexcept;
};

namespace asmjit
{
	// Should only be used to build global functions
	jit_runtime_base& get_global_runtime();

	// Don't use directly
	class inline_runtime : public jit_runtime_base
	{
		uchar* m_data;
		usz m_size;

	public:
		inline_runtime(uchar* data, usz size);

		~inline_runtime();

		uchar* _alloc(usz size, usz align) noexcept override;
	};

	// Emit xbegin and adjacent loop, return label at xbegin (don't use xabort please)
	template <typename F>
	[[nodiscard]] inline asmjit::Label build_transaction_enter(asmjit::x86::Assembler& c, asmjit::Label fallback, F func)
	{
		Label fall = c.newLabel();
		Label begin = c.newLabel();
		c.jmp(begin);
		c.bind(fall);

		// Don't repeat on zero status (may indicate syscall or interrupt)
		c.test(x86::eax, x86::eax);
		c.jz(fallback);

		// First invoked after failure (can fallback to proceed, or jump anywhere else)
		func();

		// Other bad statuses are ignored regardless of repeat flag (TODO)
		c.align(AlignMode::kCode, 16);
		c.bind(begin);
		return fall;

		// xbegin should be issued manually, allows to add more check before entering transaction
	}

	// Helper to spill RDX (EDX) register for RDTSC
	inline void build_swap_rdx_with(asmjit::x86::Assembler& c, std::array<x86::Gp, 4>& args, const asmjit::x86::Gp& with)
	{
#ifdef _WIN32
		c.xchg(args[1], with);
		args[1] = with;
#else
		c.xchg(args[2], with);
		args[2] = with;
#endif
	}

	// Get full RDTSC value into chosen register (clobbers rax/rdx or saves only rax with other target)
	inline void build_get_tsc(asmjit::x86::Assembler& c, const asmjit::x86::Gp& to = asmjit::x86::rax)
	{
		if (&to != &x86::rax && &to != &x86::rdx)
		{
			// Swap to save its contents
			c.xchg(x86::rax, to);
		}

		c.rdtsc();
		c.shl(x86::rdx, 32);

		if (&to == &x86::rax)
		{
			c.or_(x86::rax, x86::rdx);
		}
		else if (&to == &x86::rdx)
		{
			c.or_(x86::rdx, x86::rax);
		}
		else
		{
			// Swap back, maybe there is more effective way to do it
			c.xchg(x86::rax, to);
			c.mov(to.r32(), to.r32());
			c.or_(to.r64(), x86::rdx);
		}
	}

	inline void build_init_args_from_ghc(native_asm& c, native_args& args)
	{
#if defined(ARCH_X64)
		// TODO: handle case when args don't overlap with r13/rbp/r12/rbx
		c.mov(args[0], x86::r13);
		c.mov(args[1], x86::rbp);
		c.mov(args[2], x86::r12);
		c.mov(args[3], x86::rbx);
#else
		static_cast<void>(c);
		static_cast<void>(args);
#endif
	}

	inline void build_init_ghc_args(native_asm& c, native_args& args)
	{
#if defined(ARCH_X64)
		// TODO: handle case when args don't overlap with r13/rbp/r12/rbx
		c.mov(x86::r13, args[0]);
		c.mov(x86::rbp, args[1]);
		c.mov(x86::r12, args[2]);
		c.mov(x86::rbx, args[3]);
#else
		static_cast<void>(c);
		static_cast<void>(args);
#endif
	}

#if defined(ARCH_X64)
	template <uint Size>
	struct native_vec;

	template <>
	struct native_vec<16> { using type = x86::Xmm; };

	template <>
	struct native_vec<32> { using type = x86::Ymm; };

	template <>
	struct native_vec<64> { using type = x86::Zmm; };

	template <uint Size>
	using native_vec_t = typename native_vec<Size>::type;

	// if (count > step) { for (; ctr < (count - step); ctr += step) {...} count -= ctr; }
	inline void build_incomplete_loop(native_asm& c, auto ctr, auto count, u32 step, auto&& build)
	{
		asmjit::Label body = c.newLabel();
		asmjit::Label exit = c.newLabel();

		ensure((step & (step - 1)) == 0);
		c.cmp(count, step);
		c.jbe(exit);
		c.sub(count, step);
		c.align(asmjit::AlignMode::kCode, 16);
		c.bind(body);
		build();
		c.add(ctr, step);
		c.sub(count, step);
		c.ja(body);
		c.add(count, step);
		c.bind(exit);
	}

	// for (; count > 0; ctr++, count--)
	inline void build_loop(native_asm& c, auto ctr, auto count, auto&& build)
	{
		asmjit::Label body = c.newLabel();
		asmjit::Label exit = c.newLabel();

		c.test(count, count);
		c.jz(exit);
		c.align(asmjit::AlignMode::kCode, 16);
		c.bind(body);
		build();
		c.inc(ctr);
		c.sub(count, 1);
		c.ja(body);
		c.bind(exit);
	}
#endif
}

// Build runtime function with asmjit::X86Assembler
template <typename FT, typename Asm = native_asm, typename F>
inline FT build_function_asm(std::string_view name, F&& builder)
{
#ifdef __APPLE__
	pthread_jit_write_protect_np(false);
#endif
	using namespace asmjit;

	auto& rt = get_global_runtime();

	CodeHolder code;
	code.init(rt.environment());

#if defined(ARCH_X64)
	native_args args;
#ifdef _WIN32
	args[0] = x86::rcx;
	args[1] = x86::rdx;
	args[2] = x86::r8;
	args[3] = x86::r9;
#else
	args[0] = x86::rdi;
	args[1] = x86::rsi;
	args[2] = x86::rdx;
	args[3] = x86::rcx;
#endif
#elif defined(ARCH_ARM64)
	native_args args;
	args[0] = a64::x0;
	args[1] = a64::x1;
	args[2] = a64::x2;
	args[3] = a64::x3;
#endif

	Asm compiler(&code);
	compiler.addEncodingOptions(EncodingOptions::kOptimizedAlign);
	if constexpr (std::is_invocable_r_v<bool, F, Asm&, native_args&>)
	{
		if (!builder(compiler, args))
			return nullptr;
	}
	else
	{
		builder(compiler, args);
	}

	const auto result = rt._add(&code);
	jit_announce(result, code.codeSize(), name);
	return reinterpret_cast<FT>(uptr(result));
}

#ifdef LLVM_AVAILABLE

namespace llvm
{
	class LLVMContext;
	class ExecutionEngine;
	class Module;
}

// Temporary compiler interface
class jit_compiler final
{
	// Local LLVM context
	std::unique_ptr<llvm::LLVMContext> m_context{};

	// Execution instance
	std::unique_ptr<llvm::ExecutionEngine> m_engine{};

	// Arch
	std::string m_cpu{};

public:
	jit_compiler(const std::unordered_map<std::string, u64>& _link, const std::string& _cpu, u32 flags = 0);
	~jit_compiler();

	// Get LLVM context
	auto& get_context()
	{
		return *m_context;
	}

	auto& get_engine() const
	{
		return *m_engine;
	}

	// Add module (path to obj cache dir)
	void add(std::unique_ptr<llvm::Module> _module, const std::string& path);

	// Add module (not cached)
	void add(std::unique_ptr<llvm::Module> _module);

	// Add object (path to obj file)
	void add(const std::string& path);

	// Check object file
	static bool check(const std::string& path);

	// Finalize
	void fin();

	// Get compiled function address
	u64 get(const std::string& name);

	// Get CPU info
	static std::string cpu(const std::string& _cpu);
};

#endif
