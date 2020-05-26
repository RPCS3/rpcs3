#pragma once

// Include asmjit with warnings ignored
#define ASMJIT_EMBED
#define ASMJIT_DEBUG

#ifdef _MSC_VER
#pragma warning(push, 0)
#include <asmjit/asmjit.h>
#pragma warning(pop)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <asmjit/asmjit.h>
#pragma GCC diagnostic pop
#endif

#include <array>
#include <functional>

enum class jit_class
{
	ppu_code,
	ppu_data,
	spu_code,
	spu_data,
};

// ASMJIT runtime for emitting code in a single 2G region
struct jit_runtime final : asmjit::HostRuntime
{
	jit_runtime();
	~jit_runtime() override;

	// Allocate executable memory
	asmjit::Error _add(void** dst, asmjit::CodeHolder* code) noexcept override;

	// Do nothing (deallocation is delayed)
	asmjit::Error _release(void* p) noexcept override;

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
	asmjit::Runtime& get_global_runtime();

	// Emit xbegin and adjacent loop, return label at xbegin (don't use xabort please)
	template <typename F>
	[[nodiscard]] inline asmjit::Label build_transaction_enter(asmjit::X86Assembler& c, asmjit::Label fallback, F func)
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
		c.align(kAlignCode, 16);
		c.bind(begin);
		return fall;

		// xbegin should be issued manually, allows to add more check before entering transaction
	}

	// Helper to spill RDX (EDX) register for RDTSC
	inline void build_swap_rdx_with(asmjit::X86Assembler& c, std::array<X86Gp, 4>& args, const asmjit::X86Gp& with)
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
	inline void build_get_tsc(asmjit::X86Assembler& c, const asmjit::X86Gp& to = asmjit::x86::rax)
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
}

// Build runtime function with asmjit::X86Assembler
template <typename FT, typename F>
inline FT build_function_asm(F&& builder)
{
	using namespace asmjit;

	auto& rt = get_global_runtime();

	CodeHolder code;
	code.init(rt.getCodeInfo());
	code._globalHints = asmjit::CodeEmitter::kHintOptimizedAlign;

	std::array<X86Gp, 4> args;
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

	X86Assembler compiler(&code);
	builder(std::ref(compiler), args);
	ensure(compiler.getLastError() == 0);

	FT result;

	if (rt.add(&result, &code))
	{
		return nullptr;
	}

	return result;
}

#ifdef LLVM_AVAILABLE

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "util/types.hpp"
#include "mutex.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

// Temporary compiler interface
class jit_compiler final
{
	// Local LLVM context
	llvm::LLVMContext m_context;

	// Aux
	std::unique_ptr<llvm::JITEventListener> m_jite;

	// Execution instance
	std::unique_ptr<llvm::ExecutionEngine> m_engine;

	// Arch
	std::string m_cpu;

public:
	jit_compiler(const std::unordered_map<std::string, u64>& _link, const std::string& _cpu, u32 flags = 0);
	~jit_compiler();

	// Get LLVM context
	auto& get_context()
	{
		return m_context;
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
