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
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#endif
#include <asmjit/asmjit.h>
#pragma GCC diagnostic pop
#endif

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

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

	using imm_ptr = Imm;
}

// Build runtime function with asmjit::X86Assembler
template <typename FT, typename F>
inline FT build_function_asm(std::string_view name, F&& builder)
{
	using namespace asmjit;

	auto& rt = get_global_runtime();

	CodeHolder code;
	code.init(rt.environment());

	std::array<x86::Gp, 4> args;
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

	x86::Assembler compiler(&code);
	compiler.addEncodingOptions(EncodingOptions::kOptimizedAlign);
	builder(std::ref(compiler), args);
	const auto result = rt._add(&code);
	jit_announce(result, code.codeSize(), name);
	return reinterpret_cast<FT>(uptr(result));
}

#ifdef __APPLE__
template <typename FT, usz = 4096>
class built_function
{
	FT m_func;

public:
	built_function(const built_function&) = delete;

	built_function& operator=(const built_function&) = delete;

	template <typename F>
	built_function(std::string_view name, F&& builder)
		: m_func(ensure(build_function_asm<FT>(name, std::forward<F>(builder))))
	{
	}

	operator FT() const noexcept
	{
		return m_func;
	}

	template <typename... Args>
	auto operator()(Args&&... args) const noexcept
	{
		return m_func(std::forward<Args>(args)...);
	}
};
#else
template <typename FT, usz Size = 4096>
class built_function
{
	alignas(4096) uchar m_data[Size];

public:
	built_function(const built_function&) = delete;

	built_function& operator=(const built_function&) = delete;

	template <typename F>
	built_function(std::string_view name, F&& builder)
	{
		using namespace asmjit;

		inline_runtime rt(m_data, Size);

		CodeHolder code;
		code.init(rt.environment());

		std::array<x86::Gp, 4> args;
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

		x86::Assembler compiler(&code);
		compiler.addEncodingOptions(EncodingOptions::kOptimizedAlign);
		builder(std::ref(compiler), args);
		jit_announce(rt._add(&code), code.codeSize(), name);
	}

	operator FT() const noexcept
	{
		return FT(+m_data);
	}

	template <typename... Args>
	auto operator()(Args&&... args) const noexcept
	{
		return FT(+m_data)(std::forward<Args>(args)...);
	}
};
#endif

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
