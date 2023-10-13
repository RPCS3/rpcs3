#include "util/types.hpp"
#include "util/sysinfo.hpp"
#include "JIT.h"
#include "StrFmt.h"
#include "File.h"
#include "util/logs.hpp"
#include "mutex.h"
#include "util/vm.hpp"
#include "util/asm.hpp"
#include "util/v128.hpp"
#include "util/simd.hpp"
#include "Crypto/unzip.h"
#include <charconv>

#ifdef __linux__
#define CAN_OVERCOMMIT
#endif

LOG_CHANNEL(jit_log, "JIT");

void jit_announce(uptr func, usz size, std::string_view name)
{
#ifdef __linux__
	static const struct tmp_perf_map
	{
		std::string name{fmt::format("/tmp/perf-%d.map", getpid())};
		fs::file data{name, fs::rewrite + fs::append};

		tmp_perf_map() = default;
		tmp_perf_map(const tmp_perf_map&) = delete;
		tmp_perf_map& operator=(const tmp_perf_map&) = delete;

		~tmp_perf_map()
		{
			fs::remove_file(name);
		}
	} s_map;

	if (size && name.size())
	{
		s_map.data.write(fmt::format("%x %x %s\n", func, size, name));
	}

	if (!func && !size && !name.size())
	{
		fs::remove_file(s_map.name);
		return;
	}
#endif

	if (!size)
	{
		jit_log.error("Empty function announced: %s (%p)", name, func);
		return;
	}

	// If directory ASMJIT doesn't exist, nothing will be written
	static constexpr u64 c_dump_size = 0x1'0000'0000;
	static constexpr u64 c_index_size = c_dump_size / 16;
	static atomic_t<u64> g_index_off = 0;
	static atomic_t<u64> g_data_off = c_index_size;

	static void* g_asm = []() -> void*
	{
		fs::remove_all(fs::get_cache_dir() + "/ASMJIT/", false);

		fs::file objs(fmt::format("%s/ASMJIT/.objects", fs::get_cache_dir()), fs::read + fs::rewrite);

		if (!objs || !objs.trunc(c_dump_size))
		{
			return nullptr;
		}

		return utils::memory_map_fd(objs.get_handle(), c_dump_size, utils::protection::rw);
	}();

	if (g_asm && size < c_index_size)
	{
		struct entry
		{
			u64 addr; // RPCS3 process address
			u32 size; // Function size
			u32 off; // Function offset
		};

		// Write index entry at the beginning of file, and data + NTS name at fixed offset
		const u64 index_off = g_index_off.fetch_add(1);
		const u64 size_all = size + name.size() + 1;
		const u64 data_off = g_data_off.fetch_add(size_all);

		// If either index or data area is exhausted, nothing will be written
		if (index_off < c_index_size / sizeof(entry) && data_off + size_all < c_dump_size)
		{
			entry& index = static_cast<entry*>(g_asm)[index_off];

			std::memcpy(static_cast<char*>(g_asm) + data_off, reinterpret_cast<char*>(func), size);
			std::memcpy(static_cast<char*>(g_asm) + data_off + size, name.data(), name.size());
			index.size = static_cast<u32>(size);
			index.off = static_cast<u32>(data_off);
			atomic_storage<u64>::store(index.addr, func);
		}
	}

	if (g_asm && !name.empty() && name[0] != '_')
	{
		// Save some objects separately
		fs::file dump(fmt::format("%s/ASMJIT/%s", fs::get_cache_dir(), name), fs::rewrite);

		if (dump)
		{
			dump.write(reinterpret_cast<uchar*>(func), size);
		}
	}
}

static u8* get_jit_memory()
{
	// Reserve 2G memory (magic static)
	static void* const s_memory2 = []() -> void*
	{
		void* ptr = utils::memory_reserve(0x80000000);
#ifdef CAN_OVERCOMMIT
		utils::memory_commit(ptr, 0x80000000);
		utils::memory_protect(ptr, 0x40000000, utils::protection::wx);
#endif
		return ptr;
	}();

	return static_cast<u8*>(s_memory2);
}

// Allocation counters (1G code, 1G data subranges)
static atomic_t<u64> s_code_pos{0}, s_data_pos{0};

// Snapshot of code generated before main()
static std::vector<u8> s_code_init, s_data_init;

template <atomic_t<u64>& Ctr, uint Off, utils::protection Prot>
static u8* add_jit_memory(usz size, usz align)
{
	// Select subrange
	u8* pointer = get_jit_memory() + Off;

	if (!size && !align) [[unlikely]]
	{
		// Return subrange info
		return pointer;
	}

	u64 olda, newa;

	// Simple allocation by incrementing pointer to the next free data
	const u64 pos = Ctr.atomic_op([&](u64& ctr) -> u64
	{
		const u64 _pos = utils::align(ctr & 0xffff'ffff, align);
		const u64 _new = utils::align(_pos + size, align);

		if (_new > 0x40000000) [[unlikely]]
		{
			// Sorry, we failed, and further attempts should fail too.
			ctr |= 0x40000000;
			return -1;
		}

		// Last allocation is stored in highest bits
		olda = ctr >> 32;
		newa = olda;

		// Check the necessity to commit more memory
		if (_new > olda) [[unlikely]]
		{
			newa = utils::align(_new, 0x200000);
		}

		ctr += _new - (ctr & 0xffff'ffff);
		return _pos;
	});

	if (pos == umax) [[unlikely]]
	{
		jit_log.error("Out of memory (size=0x%x, align=0x%x, off=0x%x)", size, align, Off);
		return nullptr;
	}

	if (olda != newa) [[unlikely]]
	{
#ifndef CAN_OVERCOMMIT
		// Commit more memory
		utils::memory_commit(pointer + olda, newa - olda, Prot);
#endif
		// Acknowledge committed memory
		Ctr.atomic_op([&](u64& ctr)
		{
			if ((ctr >> 32) < newa)
			{
				ctr += (newa - (ctr >> 32)) << 32;
			}
		});
	}

	ensure(pointer + pos >= get_jit_memory() + Off);
	ensure(pointer + pos < get_jit_memory() + Off + 0x40000000);

	return pointer + pos;
}

const asmjit::Environment& jit_runtime_base::environment() const noexcept
{
	static const asmjit::Environment g_env = asmjit::Environment::host();

	return g_env;
}

void* jit_runtime_base::_add(asmjit::CodeHolder* code) noexcept
{
	ensure(!code->flatten());
	ensure(!code->resolveUnresolvedLinks());
	usz codeSize = code->codeSize();
	if (!codeSize)
		return nullptr;

	auto p = ensure(this->_alloc(codeSize, 64));
	ensure(!code->relocateToBase(uptr(p)));

	{
		// We manage rw <-> rx transitions manually on Apple
		// because it's easier to keep track of when and where we need to toggle W^X
#if !(defined(ARCH_ARM64) && defined(__APPLE__))
		asmjit::VirtMem::ProtectJitReadWriteScope rwScope(p, codeSize);
#endif

		for (asmjit::Section* section : code->_sections)
		{
			std::memcpy(p + section->offset(), section->data(), section->bufferSize());
		}
	}

	return p;
}

jit_runtime::jit_runtime()
{
}

jit_runtime::~jit_runtime()
{
}

uchar* jit_runtime::_alloc(usz size, usz align) noexcept
{
	return jit_runtime::alloc(size, align, true);
}

u8* jit_runtime::alloc(usz size, usz align, bool exec) noexcept
{
	if (exec)
	{
		return add_jit_memory<s_code_pos, 0x0, utils::protection::wx>(size, align);
	}
	else
	{
		return add_jit_memory<s_data_pos, 0x40000000, utils::protection::rw>(size, align);
	}
}

void jit_runtime::initialize()
{
	if (!s_code_init.empty() || !s_data_init.empty())
	{
		return;
	}

	// Create code/data snapshot
	s_code_init.resize(s_code_pos & 0xffff'ffff);
	std::memcpy(s_code_init.data(), alloc(0, 0, true), s_code_init.size());
	s_data_init.resize(s_data_pos & 0xffff'ffff);
	std::memcpy(s_data_init.data(), alloc(0, 0, false), s_data_init.size());
}

void jit_runtime::finalize() noexcept
{
#ifdef __APPLE__
	pthread_jit_write_protect_np(false);
#endif
	// Reset JIT memory
#ifdef CAN_OVERCOMMIT
	utils::memory_reset(get_jit_memory(), 0x80000000);
	utils::memory_protect(get_jit_memory(), 0x40000000, utils::protection::wx);
#else
	utils::memory_decommit(get_jit_memory(), 0x80000000);
#endif

	s_code_pos = 0;
	s_data_pos = 0;

	// Restore code/data snapshot
	std::memcpy(alloc(s_code_init.size(), 1, true), s_code_init.data(), s_code_init.size());
	std::memcpy(alloc(s_data_init.size(), 1, false), s_data_init.data(), s_data_init.size());

#ifdef __APPLE__
	pthread_jit_write_protect_np(true);
#endif
#ifdef ARCH_ARM64
	// Flush all cache lines after potentially writing executable code
	asm("ISB");
	asm("DSB ISH");
#endif
}

jit_runtime_base& asmjit::get_global_runtime()
{
	// 16 MiB for internal needs
	static constexpr u64 size = 1024 * 1024 * 16;

	struct custom_runtime final : jit_runtime_base
	{
		custom_runtime() noexcept
		{
			// Search starting in first 2 GiB of memory
			for (u64 addr = size;; addr += size)
			{
				if (auto ptr = utils::memory_reserve(size, reinterpret_cast<void*>(addr)))
				{
					m_pos.raw() = static_cast<uchar*>(ptr);
					break;
				}
			}

			// Initialize "end" pointer
			m_max = m_pos + size;

			// Make memory writable + executable
			utils::memory_commit(m_pos, size, utils::protection::wx);
		}

		uchar* _alloc(usz size, usz align) noexcept override
		{
			return m_pos.atomic_op([&](uchar*& pos) -> uchar*
			{
				const auto r = reinterpret_cast<uchar*>(utils::align(uptr(pos), align));

				if (r >= pos && r + size > pos && r + size <= m_max)
				{
					pos = r + size;
					return r;
				}

				return nullptr;
			});
		}

	private:
		atomic_t<uchar*> m_pos{};

		uchar* m_max{};
	};

	// Magic static
	static custom_runtime g_rt;
	return g_rt;
}

asmjit::inline_runtime::inline_runtime(uchar* data, usz size)
	: m_data(data)
	, m_size(size)
{
}

uchar* asmjit::inline_runtime::_alloc(usz size, usz align) noexcept
{
	ensure(align <= 4096);

	return size <= m_size ? m_data : nullptr;
}

asmjit::inline_runtime::~inline_runtime()
{
	utils::memory_protect(m_data, m_size, utils::protection::rx);
}

#if defined(ARCH_X64)
asmjit::simd_builder::simd_builder(CodeHolder* ch) noexcept
	: native_asm(ch)
{
	_init(0);
	consts[~v128()] = this->newLabel();
}

asmjit::simd_builder::~simd_builder()
{
}

void asmjit::simd_builder::_init(uint new_vsize)
{
	if ((!new_vsize && utils::has_avx512_icl()) || new_vsize == 64)
	{
		v0 = x86::zmm0;
		v1 = x86::zmm1;
		v2 = x86::zmm2;
		v3 = x86::zmm3;
		v4 = x86::zmm4;
		v5 = x86::zmm5;
		vsize = 64;
	}
	else if ((!new_vsize && utils::has_avx2()) || new_vsize == 32)
	{
		v0 = x86::ymm0;
		v1 = x86::ymm1;
		v2 = x86::ymm2;
		v3 = x86::ymm3;
		v4 = x86::ymm4;
		v5 = x86::ymm5;
		vsize = 32;
	}
	else
	{
		v0 = x86::xmm0;
		v1 = x86::xmm1;
		v2 = x86::xmm2;
		v3 = x86::xmm3;
		v4 = x86::xmm4;
		v5 = x86::xmm5;
		vsize = new_vsize ? new_vsize : 16;
	}

	if (utils::has_avx512())
	{
		if (!new_vsize)
			vmask = -1;
	}
	else
	{
		vmask = 0;
	}
}

void asmjit::simd_builder::operator()() noexcept
{
	for (auto&& [x, y] : consts)
	{
		this->align(AlignMode::kData, 16);
		this->bind(y);
		this->embed(&x, 16);
	}
}

void asmjit::simd_builder::vec_cleanup_ret()
{
	if (utils::has_avx() && vsize > 16)
		this->vzeroupper();
	this->ret();
}

void asmjit::simd_builder::vec_set_all_zeros(const Operand& v)
{
	x86::Xmm reg(v.id());
	if (utils::has_avx())
		this->vpxor(reg, reg, reg);
	else
		this->xorps(reg, reg);
}

void asmjit::simd_builder::vec_set_all_ones(const Operand& v)
{
	x86::Xmm reg(v.id());
	if (x86::Zmm zr(v.id()); zr == v)
		this->vpternlogd(zr, zr, zr, 0xff);
	else if (x86::Ymm yr(v.id()); yr == v)
		this->vpcmpeqd(yr, yr, yr);
	else if (utils::has_avx())
		this->vpcmpeqd(reg, reg, reg);
	else
		this->pcmpeqd(reg, reg);
}

void asmjit::simd_builder::vec_set_const(const Operand& v, const v128& val)
{
	if (!val._u)
		return vec_set_all_zeros(v);
	if (!~val._u)
		return vec_set_all_ones(v);
	else
	{
		Label co = consts[val];
		if (!co.isValid())
			co = consts[val] = this->newLabel();
		if (x86::Zmm zr(v.id()); zr == v)
			this->vbroadcasti32x4(zr, x86::oword_ptr(co));
		else if (x86::Ymm yr(v.id()); yr == v)
			this->vbroadcasti128(yr, x86::oword_ptr(co));
		else if (utils::has_avx())
			this->vmovaps(x86::Xmm(v.id()), x86::oword_ptr(co));
		else
			this->movaps(x86::Xmm(v.id()), x86::oword_ptr(co));
	}
}

void asmjit::simd_builder::vec_clobbering_test(u32 esize, const Operand& v, const Operand& rhs)
{
	if (esize == 64)
	{
		this->emit(x86::Inst::kIdVptestmd, x86::k0, v, rhs);
		this->ktestw(x86::k0, x86::k0);
	}
	else if (esize == 32)
	{
		this->emit(x86::Inst::kIdVptest, v, rhs);
	}
	else if (esize == 16 && utils::has_avx())
	{
		this->emit(x86::Inst::kIdVptest, v, rhs);
	}
	else if (esize == 16 && utils::has_sse41())
	{
		this->emit(x86::Inst::kIdPtest, v, rhs);
	}
	else
	{
		if (v != rhs)
			this->emit(x86::Inst::kIdPand, v, rhs);
		if (esize == 16)
			this->emit(x86::Inst::kIdPacksswb, v, v);
		this->emit(x86::Inst::kIdMovq, x86::rax, v);
		if (esize == 16 || esize == 8)
			this->test(x86::rax, x86::rax);
		else if (esize == 4)
			this->test(x86::eax, x86::eax);
		else if (esize == 2)
			this->test(x86::ax, x86::ax);
		else if (esize == 1)
			this->test(x86::al, x86::al);
		else
			fmt::throw_exception("Unimplemented");
	}
}

void asmjit::simd_builder::vec_broadcast_gpr(u32 esize, const Operand& v, const x86::Gp& r)
{
	if (esize == 2)
	{
		if (utils::has_avx512())
			this->emit(x86::Inst::kIdVpbroadcastw, v, r.r32());
		else if (utils::has_avx())
		{
			this->emit(x86::Inst::kIdVmovd, v, r.r32());
			if (utils::has_avx2())
				this->emit(x86::Inst::kIdVpbroadcastw, v, v);
			else
			{
				this->emit(x86::Inst::kIdVpunpcklwd, v, v, v);
				this->emit(x86::Inst::kIdVpshufd, v, v, Imm(0));
			}
		}
		else
		{
			this->emit(x86::Inst::kIdMovd, v, r.r32());
			this->emit(x86::Inst::kIdPunpcklwd, v, v);
			this->emit(x86::Inst::kIdPshufd, v, v, Imm(0));
		}
	}
	else if (esize == 4)
	{
		if (utils::has_avx512())
			this->emit(x86::Inst::kIdVpbroadcastd, v, r.r32());
		else if (utils::has_avx())
		{
			this->emit(x86::Inst::kIdVmovd, v, r.r32());
			if (utils::has_avx2())
				this->emit(x86::Inst::kIdVpbroadcastd, v, v);
			else
				this->emit(x86::Inst::kIdVpshufd, v, v, Imm(0));
		}
		else
		{
			this->emit(x86::Inst::kIdMovd, v, r.r32());
			this->emit(x86::Inst::kIdPshufd, v, v, Imm(0));
		}
	}
	else
	{
		fmt::throw_exception("Unimplemented");
	}
}

asmjit::x86::Mem asmjit::simd_builder::ptr_scale_for_vec(u32 esize, const x86::Gp& base, const x86::Gp& index)
{
	switch (ensure(esize))
	{
	case 1: return x86::ptr(base, index, 0, 0);
	case 2: return x86::ptr(base, index, 1, 0);
	case 4: return x86::ptr(base, index, 2, 0);
	case 8: return x86::ptr(base, index, 3, 0);
	default: fmt::throw_exception("Bad esize");
	}
}

void asmjit::simd_builder::vec_load_unaligned(u32 esize, const Operand& v, const x86::Mem& src)
{
	ensure(std::has_single_bit(esize));
	ensure(std::has_single_bit(vsize));

	if (esize == 2)
	{
		ensure(vsize >= 2);
		if (vsize == 2)
			vec_set_all_zeros(v);
		if (vsize == 2 && utils::has_avx())
			this->emit(x86::Inst::kIdVpinsrw, x86::Xmm(v.id()), x86::Xmm(v.id()), src, Imm(0));
		else if (vsize == 2)
			this->emit(x86::Inst::kIdPinsrw, v, src, Imm(0));
		else if ((vmask && vmask < 8) || vsize >= 64)
			this->emit(x86::Inst::kIdVmovdqu16, v, src);
		else
			return vec_load_unaligned(vsize, v, src);
	}
	else if (esize == 4)
	{
		ensure(vsize >= 4);
		if (vsize == 4 && utils::has_avx())
			this->emit(x86::Inst::kIdVmovd, x86::Xmm(v.id()), src);
		else if (vsize == 4)
			this->emit(x86::Inst::kIdMovd, v, src);
		else if ((vmask && vmask < 8) || vsize >= 64)
			this->emit(x86::Inst::kIdVmovdqu32, v, src);
		else
			return vec_load_unaligned(vsize, v, src);
	}
	else if (esize == 8)
	{
		ensure(vsize >= 8);
		if (vsize == 8 && utils::has_avx())
			this->emit(x86::Inst::kIdVmovq, x86::Xmm(v.id()), src);
		else if (vsize == 8)
			this->emit(x86::Inst::kIdMovq, v, src);
		else if ((vmask && vmask < 8) || vsize >= 64)
			this->emit(x86::Inst::kIdVmovdqu64, v, src);
		else
			return vec_load_unaligned(vsize, v, src);
	}
	else if (esize >= 16)
	{
		ensure(vsize >= 16);
		if ((vmask && vmask < 8) || vsize >= 64)
			this->emit(x86::Inst::kIdVmovdqu64, v, src); // Not really needed
		else if (utils::has_avx())
			this->emit(x86::Inst::kIdVmovdqu, v, src);
		else
			this->emit(x86::Inst::kIdMovups, v, src);
	}
	else
	{
		fmt::throw_exception("Unimplemented");
	}
}

void asmjit::simd_builder::vec_store_unaligned(u32 esize, const Operand& v, const x86::Mem& dst)
{
	ensure(std::has_single_bit(esize));
	ensure(std::has_single_bit(vsize));

	if (esize == 2)
	{
		ensure(vsize >= 2);
		if (vsize == 2 && utils::has_avx())
			this->emit(x86::Inst::kIdVpextrw, dst, x86::Xmm(v.id()), Imm(0));
		else if (vsize == 2 && utils::has_sse41())
			this->emit(x86::Inst::kIdPextrw, dst, v, Imm(0));
		else if (vsize == 2)
			this->push(x86::rax), this->pextrw(x86::eax, x86::Xmm(v.id()), 0), this->mov(dst, x86::ax), this->pop(x86::rax);
		else if ((vmask && vmask < 8) || vsize >= 64)
			this->emit(x86::Inst::kIdVmovdqu16, dst, v);
		else
			return vec_store_unaligned(vsize, v, dst);
	}
	else if (esize == 4)
	{
		ensure(vsize >= 4);
		if (vsize == 4 && utils::has_avx())
			this->emit(x86::Inst::kIdVmovd, dst, x86::Xmm(v.id()));
		else if (vsize == 4)
			this->emit(x86::Inst::kIdMovd, dst, v);
		else if ((vmask && vmask < 8) || vsize >= 64)
			this->emit(x86::Inst::kIdVmovdqu32, dst, v);
		else
			return vec_store_unaligned(vsize, v, dst);
	}
	else if (esize == 8)
	{
		ensure(vsize >= 8);
		if (vsize == 8 && utils::has_avx())
			this->emit(x86::Inst::kIdVmovq, dst, x86::Xmm(v.id()));
		else if (vsize == 8)
			this->emit(x86::Inst::kIdMovq, dst, v);
		else if ((vmask && vmask < 8) || vsize >= 64)
			this->emit(x86::Inst::kIdVmovdqu64, dst, v);
		else
			return vec_store_unaligned(vsize, v, dst);
	}
	else if (esize >= 16)
	{
		ensure(vsize >= 16);
		if ((vmask && vmask < 8) || vsize >= 64)
			this->emit(x86::Inst::kIdVmovdqu64, dst, v); // Not really needed
		else if (utils::has_avx())
			this->emit(x86::Inst::kIdVmovdqu, dst, v);
		else
			this->emit(x86::Inst::kIdMovups, dst, v);
	}
	else
	{
		fmt::throw_exception("Unimplemented");
	}
}

void asmjit::simd_builder::_vec_binary_op(x86::Inst::Id sse_op, x86::Inst::Id vex_op, x86::Inst::Id evex_op, const Operand& dst, const Operand& lhs, const Operand& rhs)
{
	if (utils::has_avx())
	{
		if (evex_op != x86::Inst::kIdNone && (vex_op == x86::Inst::kIdNone || this->_extraReg.isReg() || vsize >= 64))
		{
			this->evex().emit(evex_op, dst, lhs, rhs);
		}
		else
		{
			this->emit(vex_op, dst, lhs, rhs);
		}
	}
	else if (dst == lhs)
	{
		this->emit(sse_op, dst, rhs);
	}
	else if (dst == rhs)
	{
		fmt::throw_exception("Unimplemented");
	}
	else
	{
		this->emit(x86::Inst::kIdMovaps, dst, lhs);
		this->emit(sse_op, dst, rhs);
	}
}

void asmjit::simd_builder::vec_umin(u32 esize, const Operand& dst, const Operand& lhs, const Operand& rhs)
{
	using enum x86::Inst::Id;
	if (esize == 2)
	{
		if (utils::has_sse41())
			return _vec_binary_op(kIdPminuw, kIdVpminuw, kIdVpminuw, dst, lhs, rhs);
	}
	else if (esize == 4)
	{
		if (utils::has_sse41())
			return _vec_binary_op(kIdPminud, kIdVpminud, kIdVpminud, dst, lhs, rhs);
	}

	fmt::throw_exception("Unimplemented");
}

void asmjit::simd_builder::vec_umax(u32 esize, const Operand& dst, const Operand& lhs, const Operand& rhs)
{
	using enum x86::Inst::Id;
	if (esize == 2)
	{
		if (utils::has_sse41())
			return _vec_binary_op(kIdPmaxuw, kIdVpmaxuw, kIdVpmaxuw, dst, lhs, rhs);
	}
	else if (esize == 4)
	{
		if (utils::has_sse41())
			return _vec_binary_op(kIdPmaxud, kIdVpmaxud, kIdVpmaxud, dst, lhs, rhs);
	}

	fmt::throw_exception("Unimplemented");
}

void asmjit::simd_builder::vec_cmp_eq(u32 esize, const Operand& dst, const Operand& lhs, const Operand& rhs)
{
	using enum x86::Inst::Id;
	if (esize == 2)
	{
		if (vsize == 64)
		{
			this->evex().emit(kIdVpcmpeqw, x86::k0, lhs, rhs);
			this->evex().emit(kIdVpmovm2w, dst, x86::k0);
		}
		else
		{
			_vec_binary_op(kIdPcmpeqw, kIdVpcmpeqw, kIdNone, dst, lhs, rhs);
		}
	}
	else if (esize == 4)
	{
		if (vsize == 64)
		{
			this->evex().emit(kIdVpcmpeqd, x86::k0, lhs, rhs);
			this->evex().emit(kIdVpmovm2d, dst, x86::k0);
		}
		else
		{
			_vec_binary_op(kIdPcmpeqd, kIdVpcmpeqd, kIdNone, dst, lhs, rhs);
		}
	}
	else
	{
		fmt::throw_exception("Unimplemented");
	}
}

void asmjit::simd_builder::vec_extract_high(u32, const Operand& dst, const Operand& src)
{
	if (vsize == 32)
		this->vextracti32x8(x86::Ymm(dst.id()), x86::Zmm(src.id()), 1);
	else if (vsize == 16)
		this->vextracti128(x86::Xmm(dst.id()), x86::Ymm(src.id()), 1);
	else
	{
		if (utils::has_avx())
			this->vpsrldq(x86::Xmm(dst.id()), x86::Xmm(src.id()), vsize);
		else
		{
			this->movdqa(x86::Xmm(dst.id()), x86::Xmm(src.id()));
			this->psrldq(x86::Xmm(dst.id()), vsize);
		}
	}
}

void asmjit::simd_builder::vec_extract_gpr(u32 esize, const x86::Gp& dst, const Operand& src)
{
	if (esize == 8 && utils::has_avx())
		this->vmovq(dst.r64(), x86::Xmm(src.id()));
	else if (esize == 8)
		this->movq(dst.r64(), x86::Xmm(src.id()));
	else if (esize == 4 && utils::has_avx())
		this->vmovd(dst.r32(), x86::Xmm(src.id()));
	else if (esize == 4)
		this->movd(dst.r32(), x86::Xmm(src.id()));
	else if (esize == 2 && utils::has_avx())
		this->vpextrw(dst.r32(), x86::Xmm(src.id()), 0);
	else if (esize == 2)
		this->pextrw(dst.r32(), x86::Xmm(src.id()), 0);
	else
		fmt::throw_exception("Unimplemented");
}

#endif /* X86 */

#ifdef LLVM_AVAILABLE

#include <unordered_map>
#include <unordered_set>
#include <deque>

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#endif
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/ObjectCache.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/SymbolSize.h"
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

const bool jit_initialize = []() -> bool
{
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();
	LLVMLinkInMCJIT();
	return true;
}();

[[noreturn]] static void null(const char* name)
{
	fmt::throw_exception("Null function: %s", name);
}

namespace vm
{
	extern u8* const g_sudo_addr;
}

static shared_mutex null_mtx;

static std::unordered_map<std::string, u64> null_funcs;

static u64 make_null_function(const std::string& name)
{
	if (name.starts_with("__0x"))
	{
		u32 addr = -1;
		auto res = std::from_chars(name.c_str() + 4, name.c_str() + name.size(), addr, 16);

		if (res.ec == std::errc() && res.ptr == name.c_str() + name.size() && addr < 0x8000'0000)
		{
			// Point the garbage to reserved, non-executable memory
			return reinterpret_cast<u64>(vm::g_sudo_addr + addr);
		}
	}

	std::lock_guard lock(null_mtx);

	if (u64& func_ptr = null_funcs[name]) [[likely]]
	{
		// Already exists
		return func_ptr;
	}
	else
	{
		using namespace asmjit;

		// Build a "null" function that contains its name
		const auto func = build_function_asm<void (*)()>("NULL", [&](native_asm& c, auto& args)
		{
#if defined(ARCH_X64)
			Label data = c.newLabel();
			c.lea(args[0], x86::qword_ptr(data, 0));
			c.jmp(Imm(&null));
			c.align(AlignMode::kCode, 16);
			c.bind(data);

			// Copy function name bytes
			for (char ch : name)
				c.db(ch);
			c.db(0);
			c.align(AlignMode::kData, 16);
#else
			// AArch64 implementation
			Label jmp_address = c.newLabel();
			Label data = c.newLabel();
			// Force absolute jump to prevent out of bounds PC-rel jmp
			c.ldr(args[0], arm::ptr(jmp_address));
			c.br(args[0]);
			c.align(AlignMode::kCode, 16);

			c.bind(data);
			c.embed(name.c_str(), name.size());
			c.embedUInt8(0U);
			c.bind(jmp_address);
			c.embedUInt64(reinterpret_cast<u64>(&null));
			c.align(AlignMode::kData, 16);
#endif
		});

		func_ptr = reinterpret_cast<u64>(func);
		return func_ptr;
	}
}

struct JITAnnouncer : llvm::JITEventListener
{
	void notifyObjectLoaded(u64, const llvm::object::ObjectFile& obj, const llvm::RuntimeDyld::LoadedObjectInfo& info) override
	{
		using namespace llvm;

		object::OwningBinary<object::ObjectFile> debug_obj_ = info.getObjectForDebug(obj);
		if (!debug_obj_.getBinary())
		{
#ifdef __linux__
			jit_log.error("LLVM: Failed to announce JIT events (no debug object)");
#endif
			return;
		}

		const object::ObjectFile& debug_obj = *debug_obj_.getBinary();

		for (const auto& [sym, size] : computeSymbolSizes(debug_obj))
		{
			Expected<object::SymbolRef::Type> type_ = sym.getType();
			if (!type_ || *type_ != object::SymbolRef::ST_Function)
				continue;

			Expected<StringRef> name = sym.getName();
			if (!name)
				continue;

			Expected<u64> addr = sym.getAddress();
			if (!addr)
				continue;

			jit_announce(*addr, size, {name->data(), name->size()});
		}
	}
};

// Simple memory manager
struct MemoryManager1 : llvm::RTDyldMemoryManager
{
	// 256 MiB for code or data
	static constexpr u64 c_max_size = 0x20000000 / 2;

	// Allocation unit (2M)
	static constexpr u64 c_page_size = 2 * 1024 * 1024;

	// Reserve 512 MiB
	u8* const ptr = static_cast<u8*>(utils::memory_reserve(c_max_size * 2));

	u64 code_ptr = 0;
	u64 data_ptr = c_max_size;

	MemoryManager1() = default;

	MemoryManager1(const MemoryManager1&) = delete;

	MemoryManager1& operator=(const MemoryManager1&) = delete;

	~MemoryManager1() override
	{
		// Hack: don't release to prevent reuse of address space, see jit_announce
		utils::memory_decommit(ptr, c_max_size * 2);
	}

	llvm::JITSymbol findSymbol(const std::string& name) override
	{
		u64 addr = RTDyldMemoryManager::getSymbolAddress(name);

		if (!addr)
		{
			addr = make_null_function(name);

			if (!addr)
			{
				fmt::throw_exception("Failed to link '%s'", name);
			}
		}

		return {addr, llvm::JITSymbolFlags::Exported};
	}

	u8* allocate(u64& oldp, uptr size, uint align, utils::protection prot)
	{
		if (align > c_page_size)
		{
			jit_log.fatal("Unsupported alignment (size=0x%x, align=0x%x)", size, align);
			return nullptr;
		}

		const u64 olda = utils::align(oldp, align);
		const u64 newp = utils::align(olda + size, align);

		if ((newp - 1) / c_max_size != oldp / c_max_size)
		{
			jit_log.fatal("Out of memory (size=0x%x, align=0x%x)", size, align);
			return nullptr;
		}

		if ((oldp - 1) / c_page_size != (newp - 1) / c_page_size)
		{
			// Allocate pages on demand
			const u64 pagea = utils::align(oldp, c_page_size);
			const u64 psize = utils::align(newp - pagea, c_page_size);
			utils::memory_commit(this->ptr + pagea, psize, prot);
		}

		// Update allocation counter
		oldp = newp;

		return this->ptr + olda;
	}

	u8* allocateCodeSection(uptr size, uint align, uint /*sec_id*/, llvm::StringRef /*sec_name*/) override
	{
		return allocate(code_ptr, size, align, utils::protection::wx);
	}

	u8* allocateDataSection(uptr size, uint align, uint /*sec_id*/, llvm::StringRef /*sec_name*/, bool /*is_ro*/) override
	{
		return allocate(data_ptr, size, align, utils::protection::rw);
	}

	bool finalizeMemory(std::string* = nullptr) override
	{
		return false;
	}

	void registerEHFrames(u8*, u64, usz) override
	{
	}

	void deregisterEHFrames() override
	{
	}
};

// Simple memory manager
struct MemoryManager2 : llvm::RTDyldMemoryManager
{
	MemoryManager2() = default;

	~MemoryManager2() override
	{
	}

	llvm::JITSymbol findSymbol(const std::string& name) override
	{
		u64 addr = RTDyldMemoryManager::getSymbolAddress(name);

		if (!addr)
		{
			addr = make_null_function(name);

			if (!addr)
			{
				fmt::throw_exception("Failed to link '%s' (MM2)", name);
			}
		}

		return {addr, llvm::JITSymbolFlags::Exported};
	}

	u8* allocateCodeSection(uptr size, uint align, uint /*sec_id*/, llvm::StringRef /*sec_name*/) override
	{
		return jit_runtime::alloc(size, align, true);
	}

	u8* allocateDataSection(uptr size, uint align, uint /*sec_id*/, llvm::StringRef /*sec_name*/, bool /*is_ro*/) override
	{
		return jit_runtime::alloc(size, align, false);
	}

	bool finalizeMemory(std::string* = nullptr) override
	{
		return false;
	}

	void registerEHFrames(u8*, u64, usz) override
	{
	}

	void deregisterEHFrames() override
	{
	}
};

// Helper class
class ObjectCache final : public llvm::ObjectCache
{
	const std::string& m_path;

public:
	ObjectCache(const std::string& path)
		: m_path(path)
	{
	}

	~ObjectCache() override = default;

	void notifyObjectCompiled(const llvm::Module* _module, llvm::MemoryBufferRef obj) override
	{
		std::string name = m_path;
		name.append(_module->getName().data());
		//fs::file(name, fs::rewrite).write(obj.getBufferStart(), obj.getBufferSize());
		name.append(".gz");

		const std::vector<u8> zbuf = zip(reinterpret_cast<const u8*>(obj.getBufferStart()), obj.getBufferSize());

		if (zbuf.empty())
		{
			jit_log.error("LLVM: Failed to compress module: %s", _module->getName().data());
			return;
		}

		if (!fs::write_file(name, fs::rewrite, zbuf.data(), zbuf.size()))
		{
			jit_log.error("LLVM: Failed to create module file: %s (%s)", name, fs::g_tls_error);
			return;
		}

		jit_log.notice("LLVM: Created module: %s", _module->getName().data());
	}

	static std::unique_ptr<llvm::MemoryBuffer> load(const std::string& path)
	{
		if (fs::file cached{path + ".gz", fs::read})
		{
			const std::vector<u8> cached_data = cached.to_vector<u8>();

			if (cached_data.empty()) [[unlikely]]
			{
				return nullptr;
			}

			const std::vector<u8> out = unzip(cached_data);

			if (out.empty())
			{
				jit_log.error("LLVM: Failed to unzip module: '%s'", path);
				return nullptr;
			}

			auto buf = llvm::WritableMemoryBuffer::getNewUninitMemBuffer(out.size());
			std::memcpy(buf->getBufferStart(), out.data(), out.size());
			return buf;
		}

		if (fs::file cached{path, fs::read})
		{
			if (cached.size() == 0) [[unlikely]]
			{
				return nullptr;
			}

			auto buf = llvm::WritableMemoryBuffer::getNewUninitMemBuffer(cached.size());
			cached.read(buf->getBufferStart(), buf->getBufferSize());
			return buf;
		}

		return nullptr;
	}

	std::unique_ptr<llvm::MemoryBuffer> getObject(const llvm::Module* _module) override
	{
		std::string path = m_path;
		path.append(_module->getName().data());

		if (auto buf = load(path))
		{
			jit_log.notice("LLVM: Loaded module: %s", _module->getName().data());
			return buf;
		}

		return nullptr;
	}
};

std::string jit_compiler::cpu(const std::string& _cpu)
{
	std::string m_cpu = _cpu;

	if (m_cpu.empty())
	{
		m_cpu = llvm::sys::getHostCPUName().str();

		if (m_cpu == "sandybridge" ||
			m_cpu == "ivybridge" ||
			m_cpu == "haswell" ||
			m_cpu == "broadwell" ||
			m_cpu == "skylake" ||
			m_cpu == "skylake-avx512" ||
			m_cpu == "cascadelake" ||
			m_cpu == "cooperlake" ||
			m_cpu == "cannonlake" ||
			m_cpu == "icelake" ||
			m_cpu == "icelake-client" ||
			m_cpu == "icelake-server" ||
			m_cpu == "tigerlake" ||
			m_cpu == "rocketlake" ||
			m_cpu == "alderlake" ||
			m_cpu == "raptorlake" ||
			m_cpu == "meteorlake")
		{
			// Downgrade if AVX is not supported by some chips
			if (!utils::has_avx())
			{
				m_cpu = "nehalem";
			}
		}

		if (m_cpu == "skylake-avx512" ||
			m_cpu == "cascadelake" ||
			m_cpu == "cooperlake" ||
			m_cpu == "cannonlake" ||
			m_cpu == "icelake" ||
			m_cpu == "icelake-client" ||
			m_cpu == "icelake-server" ||
			m_cpu == "tigerlake" ||
			m_cpu == "rocketlake")
		{
			// Downgrade if AVX-512 is disabled or not supported
			if (!utils::has_avx512())
			{
				m_cpu = "skylake";
			}
		}

		if (m_cpu == "znver1" && utils::has_clwb())
		{
			// Upgrade
			m_cpu = "znver2";
		}

		if ((m_cpu == "znver3" || m_cpu == "goldmont" || m_cpu == "alderlake" || m_cpu == "raptorlake" || m_cpu == "meteorlake") && utils::has_avx512_icl())
		{
			// Upgrade
			m_cpu = "icelake-client";
		}

		if (m_cpu == "goldmont" && utils::has_avx2())
		{
			// Upgrade
			m_cpu = "alderlake";
		}
	}

	return m_cpu;
}

std::string jit_compiler::triple1()
{
#if defined(_WIN32)
	return llvm::Triple::normalize(llvm::sys::getProcessTriple());
#elif defined(__APPLE__) && defined(ARCH_X64)
	return llvm::Triple::normalize("x86_64-unknown-linux-gnu");
#elif defined(__APPLE__) && defined(ARCH_ARM64)
	return llvm::Triple::normalize("aarch64-unknown-linux-gnu");
#else
	return llvm::Triple::normalize(llvm::sys::getProcessTriple());
#endif
}

std::string jit_compiler::triple2()
{
#if defined(_WIN32) && defined(ARCH_X64)
	return llvm::Triple::normalize("x86_64-unknown-linux-gnu");
#elif defined(_WIN32) && defined(ARCH_ARM64)
	return llvm::Triple::normalize("aarch64-unknown-linux-gnu");
#elif defined(__APPLE__) && defined(ARCH_X64)
	return llvm::Triple::normalize("x86_64-unknown-linux-gnu");
#elif defined(__APPLE__) && defined(ARCH_ARM64)
	return llvm::Triple::normalize("aarch64-unknown-linux-gnu");
#else
	return llvm::Triple::normalize(llvm::sys::getProcessTriple());
#endif
}

jit_compiler::jit_compiler(const std::unordered_map<std::string, u64>& _link, const std::string& _cpu, u32 flags)
	: m_context(new llvm::LLVMContext)
	, m_cpu(cpu(_cpu))
{
	std::string result;

	auto null_mod = std::make_unique<llvm::Module> ("null_", *m_context);
	null_mod->setTargetTriple(jit_compiler::triple1());

	std::unique_ptr<llvm::RTDyldMemoryManager> mem;

	if (_link.empty())
	{
		// Auxiliary JIT (does not use custom memory manager, only writes the objects)
		if (flags & 0x1)
		{
			mem = std::make_unique<MemoryManager1>();
		}
		else
		{
			mem = std::make_unique<MemoryManager2>();
			null_mod->setTargetTriple(jit_compiler::triple2());
		}
	}
	else
	{
		mem = std::make_unique<MemoryManager1>();
	}

	{
		m_engine.reset(llvm::EngineBuilder(std::move(null_mod))
			.setErrorStr(&result)
			.setEngineKind(llvm::EngineKind::JIT)
			.setMCJITMemoryManager(std::move(mem))
			.setOptLevel(llvm::CodeGenOpt::Aggressive)
			.setCodeModel(flags & 0x2 ? llvm::CodeModel::Large : llvm::CodeModel::Small)
#ifdef __APPLE__
			//.setCodeModel(llvm::CodeModel::Large)
#endif
			.setRelocationModel(llvm::Reloc::Model::PIC_)
			.setMCPU(m_cpu)
			.create());
	}

	if (!_link.empty())
	{
		for (auto&& [name, addr] : _link)
		{
			m_engine->updateGlobalMapping(name, addr);
		}
	}

	if (!_link.empty() || !(flags & 0x1))
	{
		m_engine->RegisterJITEventListener(llvm::JITEventListener::createIntelJITEventListener());
		m_engine->RegisterJITEventListener(new JITAnnouncer);
	}

	if (!m_engine)
	{
		fmt::throw_exception("LLVM: Failed to create ExecutionEngine: %s", result);
	}
}

jit_compiler::~jit_compiler()
{
}

void jit_compiler::add(std::unique_ptr<llvm::Module> _module, const std::string& path)
{
	ObjectCache cache{path};
	m_engine->setObjectCache(&cache);

	const auto ptr = _module.get();
	m_engine->addModule(std::move(_module));
	m_engine->generateCodeForModule(ptr);
	m_engine->setObjectCache(nullptr);

	for (auto& func : ptr->functions())
	{
		// Delete IR to lower memory consumption
		func.deleteBody();
	}
}

void jit_compiler::add(std::unique_ptr<llvm::Module> _module)
{
	const auto ptr = _module.get();
	m_engine->addModule(std::move(_module));
	m_engine->generateCodeForModule(ptr);

	for (auto& func : ptr->functions())
	{
		// Delete IR to lower memory consumption
		func.deleteBody();
	}
}

void jit_compiler::add(const std::string& path)
{
	auto cache = ObjectCache::load(path);

	if (auto object_file = llvm::object::ObjectFile::createObjectFile(*cache))
	{
		m_engine->addObjectFile(llvm::object::OwningBinary<llvm::object::ObjectFile>(std::move(*object_file), std::move(cache)));
	}
	else
	{
		jit_log.error("ObjectCache: Adding failed: %s", path);
	}
}

bool jit_compiler::check(const std::string& path)
{
	if (auto cache = ObjectCache::load(path))
	{
		if (auto object_file = llvm::object::ObjectFile::createObjectFile(*cache))
		{
			return true;
		}

		if (fs::remove_file(path))
		{
			jit_log.error("ObjectCache: Removed damaged file: %s", path);
		}
	}

	return false;
}

void jit_compiler::update_global_mapping(const std::string& name, u64 addr)
{
	m_engine->updateGlobalMapping(name, addr);
}

void jit_compiler::fin()
{
	m_engine->finalizeObject();
}

u64 jit_compiler::get(const std::string& name)
{
	return m_engine->getGlobalValueAddress(name);
}

#endif
