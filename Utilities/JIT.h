#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"

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
#include <util/v128.hpp>

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
	void* _add(asmjit::CodeHolder* code, usz align = 64) noexcept;
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
	static u8* alloc(usz size, usz align, bool exec = true) noexcept;

	// Allocate 0 bytes, observe memory location
	// Same as alloc(0, 1, exec)
	static u8* peek(bool exec = true) noexcept;

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
	struct simd_builder : native_asm
	{
		std::unordered_map<v128, Label> consts;

		Operand v0, v1, v2, v3, v4, v5;

		uint vsize = 16;
		uint vmask = 0;

		simd_builder(CodeHolder* ch) noexcept;
		~simd_builder();

		void operator()() noexcept;

		void _init(uint new_vsize = 0);
		void vec_cleanup_ret();
		void vec_set_all_zeros(const Operand& v);
		void vec_set_all_ones(const Operand& v);
		void vec_set_const(const Operand& v, const v128& value);
		void vec_clobbering_test(u32 esize, const Operand& v, const Operand& rhs);
		void vec_broadcast_gpr(u32 esize, const Operand& v, const x86::Gp& r);

		// return x86::ptr(base, ctr, X, 0) where X is set for esize accordingly
		x86::Mem ptr_scale_for_vec(u32 esize, const x86::Gp& base, const x86::Gp& index);

		void vec_load_unaligned(u32 esize, const Operand& v, const x86::Mem& src);
		void vec_store_unaligned(u32 esize, const Operand& v, const x86::Mem& dst);
		void vec_partial_move(u32 esize, const Operand& dst, const Operand& src);

		void _vec_binary_op(x86::Inst::Id sse_op, x86::Inst::Id vex_op, x86::Inst::Id evex_op, const Operand& dst, const Operand& lhs, const Operand& rhs);

		void vec_shuffle_xi8(const Operand& dst, const Operand& lhs, const Operand& rhs)
		{
			using enum x86::Inst::Id;
			_vec_binary_op(kIdPshufb, kIdVpshufb, kIdVpshufb, dst, lhs, rhs);
		}

		void vec_xor(u32, const Operand& dst, const Operand& lhs, const Operand& rhs)
		{
			using enum x86::Inst::Id;
			_vec_binary_op(kIdPxor, kIdVpxor, kIdVpxord, dst, lhs, rhs);
		}

		void vec_or(u32, const Operand& dst, const Operand& lhs, const Operand& rhs)
		{
			using enum x86::Inst::Id;
			_vec_binary_op(kIdPor, kIdVpor, kIdVpord, dst, lhs, rhs);
		}

		void vec_andn(u32, const Operand& dst, const Operand& lhs, const Operand& rhs)
		{
			using enum x86::Inst::Id;
			_vec_binary_op(kIdPandn, kIdVpandn, kIdVpandnd, dst, lhs, rhs);
		}

		void vec_umin(u32 esize, const Operand& dst, const Operand& lhs, const Operand& rhs);
		void vec_umax(u32 esize, const Operand& dst, const Operand& lhs, const Operand& rhs);
		void vec_cmp_eq(u32 esize, const Operand& dst, const Operand& lhs, const Operand& rhs);

		void vec_extract_high(u32 esize, const Operand& dst, const Operand& src);
		void vec_extract_gpr(u32 esize, const x86::Gp& dst, const Operand& src);

		simd_builder& keep_if_not_masked()
		{
			if (vmask && vmask < 8)
			{
				this->k(x86::KReg(vmask));
			}

			return *this;
		}

		simd_builder& zero_if_not_masked()
		{
			if (vmask && vmask < 8)
			{
				this->k(x86::KReg(vmask));
				this->z();
			}

			return *this;
		}

		void build_loop(u32 esize, const x86::Gp& reg_ctr, const x86::Gp& reg_cnt, auto&& build, auto&& reduce)
		{
			ensure((esize & (esize - 1)) == 0);
			ensure(esize <= vsize);

			Label body = this->newLabel();
			Label next = this->newLabel();
			Label exit = this->newLabel();

			const u32 step = vsize / esize;

			this->xor_(reg_ctr.r32(), reg_ctr.r32()); // Reset counter reg
			this->cmp(reg_cnt, step);
			this->jb(next); // If count < step, skip main loop body
			this->align(AlignMode::kCode, 16);
			this->bind(body);
			this->sub(reg_cnt, step);
			build();
			this->add(reg_ctr, step);
			this->cmp(reg_cnt, step);
			this->jae(body);
			this->bind(next);

			if (vmask)
			{
				// Build single last iteration (masked)
				this->test(reg_cnt, reg_cnt);
				this->jz(exit);

				if (esize == 1 && vsize == 64)
				{
					this->bzhi(reg_cnt.r64(), x86::Mem(consts[~u128()], 0), reg_cnt.r64());
					this->kmovq(x86::k7, reg_cnt.r64());
				}
				else
				{
					this->bzhi(reg_cnt.r32(), x86::Mem(consts[~u128()], 0), reg_cnt.r32());
					this->kmovd(x86::k7, reg_cnt.r32());
				}

				vmask = 7;
				build();

				// Rollout reduction step
				this->bind(exit);
				while (true)
				{
					vsize /= 2;
					if (vsize < esize)
						break;
					this->_init(vsize);
					reduce();
				}
			}
			else
			{
				// Build unrolled loop tail (reduced vector width)
				while (true)
				{
					vsize /= 2;
					if (vsize < esize)
						break;

					// Shall not clobber flags
					this->_init(vsize);
					reduce();

					if (vsize == esize)
					{
						// Last "iteration"
						this->test(reg_cnt, reg_cnt);
						this->jz(exit);
						build();
					}
					else
					{
						const u32 step = vsize / esize;
						Label next = this->newLabel();
						this->cmp(reg_cnt, step);
						this->jb(next);
						build();
						this->add(reg_ctr, step);
						this->sub(reg_cnt, step);
						this->bind(next);
					}
				}

				this->bind(exit);
			}

			this->_init(0);
		}
	};

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

	inline void maybe_flush_lbr(native_asm& c, uint count = 2)
	{
		// Workaround for bad LBR callstacks which happen in some situations (mainly TSX) - execute additional RETs
		Label next = c.newLabel();
		c.lea(x86::rcx, x86::qword_ptr(next));

		for (u32 i = 0; i < count; i++)
		{
			c.push(x86::rcx);
			c.sub(x86::rcx, 16);
		}

		for (u32 i = 0; i < count; i++)
		{
			c.ret();
			c.align(asmjit::AlignMode::kCode, 16);
		}

		c.bind(next);
	}
#endif
}

// Build runtime function with asmjit::X86Assembler
template <typename FT, typename Asm = native_asm, typename F>
inline FT build_function_asm(std::string_view name, F&& builder, ::jit_runtime* custom_runtime = nullptr, bool reduced_size = false)
{
#ifdef __APPLE__
	pthread_jit_write_protect_np(false);
#endif
	using namespace asmjit;

	auto& rt = custom_runtime ? *custom_runtime : get_global_runtime();

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
	compiler.addEncodingOptions(EncodingOptions::kOptimizeForSize);
	if constexpr (std::is_invocable_r_v<bool, F, Asm&, native_args&>)
	{
		if (!builder(compiler, args))
			return nullptr;
	}
	else
	{
		builder(compiler, args);
	}

	if constexpr (std::is_invocable_r_v<void, Asm>)
	{
		// Finalization
		compiler();
	}

	const auto result = rt._add(&code, reduced_size ? 16 : 64);
	jit_announce(result, code.codeSize(), name);
	return reinterpret_cast<FT>(uptr(result));
}

#ifdef LLVM_AVAILABLE

namespace llvm
{
	class LLVMContext;
	class ExecutionEngine;
	class Module;
	class StringRef;
}

enum class thread_state : u32;

// Temporary compiler interface
class jit_compiler final
{
	// Local LLVM context
	std::unique_ptr<llvm::LLVMContext> m_context{};

	// Execution instance
	std::unique_ptr<llvm::ExecutionEngine> m_engine{};

	// Arch
	std::string m_cpu{};

	// Disk Space left
	atomic_t<usz> m_disk_space = umax;

public:
	jit_compiler(const std::unordered_map<std::string, u64>& _link, const std::string& _cpu, u32 flags = 0, std::function<u64(const std::string&)> symbols_cement = {}) noexcept;
	jit_compiler& operator=(thread_state) noexcept;
	~jit_compiler() noexcept;

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
	bool add(const std::string& path);

	// Update global mapping for a single value
	void update_global_mapping(const std::string& name, u64 addr);

	// Check object file
	static bool check(const std::string& path);

	// Finalize
	void fin();

	// Get compiled function address
	u64 get(const std::string& name);

	// Get CPU info
	static std::string cpu(const std::string& _cpu);

	// Get system triple (PPU)
	static std::string triple1();

	// Get system triple (SPU)
	static std::string triple2();

	bool add_sub_disk_space(ssz space);
};

const char *fallback_cpu_detection();

#endif // LLVM_AVAILABLE
