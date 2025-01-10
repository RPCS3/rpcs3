#pragma once

#include <string>
#include <map>
#include <set>
#include <deque>
#include <span>
#include "util/types.hpp"
#include "util/endian.hpp"
#include "util/asm.hpp"
#include "util/to_endian.hpp"

#include "Utilities/bit_set.h"
#include "PPUOpcodes.h"

// PPU Function Attributes
enum class ppu_attr : u32
{
	known_addr,
	known_size,
	no_return,
	no_size,
	has_mfvscr,

	__bitset_enum_max
};

// PPU Function Information
struct ppu_function
{
	u32 addr = 0;
	u32 toc = 0;
	u32 size = 0;
	bs_t<ppu_attr> attr{};

	u32 stack_frame = 0;
	u32 trampoline = 0;

	std::map<u32, u32> blocks{}; // Basic blocks: addr -> size
	std::set<u32> calls{}; // Set of called functions
	std::set<u32> callers{};
	mutable std::string name{}; // Function name

	struct iterator
	{
		const ppu_function* _this;
		typename std::map<u32, u32>::const_iterator it;
		usz index = 0;

		std::pair<const u32, u32> operator*() const
		{
			return _this->blocks.empty() ? std::pair<const u32, u32>(_this->addr, _this->size) : *it;
		}

		iterator& operator++()
		{
			index++;

			if (it != _this->blocks.end())
			{
				it++;
			}

			return *this;
		}

		bool operator==(const iterator& rhs) const noexcept
		{
			return it == rhs.it || (rhs.index == index && _this->blocks.empty());
		}

		bool operator!=(const iterator& rhs) const noexcept
		{
			return !operator==(rhs);
		}
	};

	iterator begin() const
	{
		return iterator{this, blocks.begin()};
	}

	iterator end() const
	{
		return iterator{this, blocks.end(), 1};
	}
};

// PPU Relocation Information
struct ppu_reloc
{
	u32 addr;
	u32 type;
	u64 data;

	// Operator for sorting
	bool operator <(const ppu_reloc& rhs) const
	{
		return addr < rhs.addr;
	}

	bool operator <(u32 rhs) const
	{
		return addr < rhs;
	}
};

// PPU Segment Information
struct ppu_segment
{
	u32 addr;
	u32 size;
	u32 type;
	u32 flags;
	u32 filesz;
	void* ptr{};
};

// PPU Module Information
template <typename Type>
struct ppu_module : public Type
{
	using Type::Type;

	ppu_module() noexcept = default;

	ppu_module(const ppu_module&) = delete;

	ppu_module(ppu_module&&) noexcept = default;

	ppu_module& operator=(const ppu_module&) = delete;

	ppu_module& operator=(ppu_module&&) noexcept = default;

	uchar sha1[20]{}; // Hash
	std::string name{}; // Filename
	std::string path{}; // Filepath
	s64 offset = 0; // Offset of file
	mutable bs_t<ppu_attr> attr{}; // Shared module attributes
	std::string cache{}; // Cache file path
	std::vector<ppu_reloc> relocs{}; // Relocations
	std::vector<ppu_segment> segs{}; // Segments
	std::vector<ppu_segment> secs{}; // Segment sections
	std::vector<ppu_function> funcs{}; // Function list
	std::vector<u32> applied_patches; // Patch addresses
	std::deque<std::shared_ptr<void>> allocations; // Segment memory allocations
	std::map<u32, u32> addr_to_seg_index; // address->segment ordered translator map
	ppu_module* parent = nullptr;
	std::pair<u32, u32> local_bounds{0, u32{umax}}; // Module addresses range
	std::shared_ptr<std::pair<u32, u32>> jit_bounds; // JIT instance modules addresses range

	template <typename T>
	auto as_span(T&& arg, bool bound_local, bool bound_jit) const
	{
		using unref = std::remove_reference_t<T>;
		using type = std::conditional_t<std::is_const_v<unref>, std::add_const_t<typename unref::value_type>, typename unref::value_type>;

		if (bound_local || bound_jit)
		{
			// Return span bound to specified bounds
			const auto [min_addr, max_addr] = bound_jit ? *jit_bounds : local_bounds;
			constexpr auto compare = [](const type& a, u32 addr) { return a.addr < addr; };
			const auto end = arg.data() + arg.size();
			const auto start = std::lower_bound(arg.data(), end, min_addr, compare);
			return std::span<type>{ start, std::lower_bound(start, end, max_addr, compare) };
		}

		return std::span<type>(arg.data(), arg.size());
	}

	auto get_funcs(bool bound_local = true, bool bound_jit = false)
	{
		return as_span(parent ? parent->funcs : funcs, bound_local, bound_jit);
	}

	auto get_funcs(bool bound_local = true, bool bound_jit = false) const
	{
		return as_span(parent ? parent->funcs : funcs, bound_local, bound_jit);
	}

	auto get_relocs(bool bound_local = false) const
	{
		return as_span(parent ? parent->relocs : relocs, bound_local, false);
	}

	// Copy info without functions
	void copy_part(const ppu_module& info)
	{
		std::memcpy(sha1, info.sha1, sizeof(sha1));
		name = info.name;
		path = info.path;
		segs = info.segs;
		allocations = info.allocations;
		addr_to_seg_index = info.addr_to_seg_index;
		parent = const_cast<ppu_module*>(&info);
		attr = info.attr;
		local_bounds = {u32{umax}, 0}; // Initially empty range
	}

	bool analyse(u32 lib_toc, u32 entry, u32 end, const std::vector<u32>& applied, const std::vector<u32>& exported_funcs = std::vector<u32>{}, std::function<bool()> check_aborted = {});
	void validate(u32 reloc);

	template <typename T>
	to_be_t<T>* get_ptr(u32 addr, u32 size_bytes) const
	{
		auto it = addr_to_seg_index.upper_bound(addr);

		if (it == addr_to_seg_index.begin())
		{
			return nullptr;
		}

		it--;

		const auto& seg = segs[it->second];
		const u32 seg_size = seg.size;
		const u32 seg_addr = seg.addr;

		if (seg_size >= std::max<usz>(size_bytes, 1) && addr <= utils::align<u32>(seg_addr + seg_size, 0x10000) - size_bytes)
		{
			return reinterpret_cast<to_be_t<T>*>(static_cast<u8*>(seg.ptr) + (addr - seg_addr));
		}

		return nullptr;
	}

	template <typename T>
	to_be_t<T>* get_ptr(u32 addr) const
	{
		constexpr usz size_element = std::is_void_v<T> ? 0 : sizeof(std::conditional_t<std::is_void_v<T>, char, T>);
		return get_ptr<T>(addr, u32{size_element});
	}

	template <typename T, typename U> requires requires (const U& obj) { obj.get_ptr(); }
	to_be_t<T>* get_ptr(U&& addr) const
	{
		constexpr usz size_element = std::is_void_v<T> ? 0 : sizeof(std::conditional_t<std::is_void_v<T>, char, T>);
		return get_ptr<T>(addr.addr(), u32{size_element});
	}

	template <typename T>
	to_be_t<T>& get_ref(u32 addr, std::source_location src_loc = std::source_location::current()) const
	{
		constexpr usz size_element = std::is_void_v<T> ? 0 : sizeof(std::conditional_t<std::is_void_v<T>, char, T>);
		if (auto ptr = get_ptr<T>(addr, u32{size_element}))
		{
			return *ptr;
		}

		fmt::throw_exception("get_ref(): Failure! (addr=0x%x)%s", addr, src_loc);
		return *std::add_pointer_t<to_be_t<T>>{};
	}

	template <typename T, typename U> requires requires (const U& obj) { obj.get_ptr(); }
	to_be_t<T>& get_ref(U&& addr, u32 index = 0, std::source_location src_loc = std::source_location::current()) const
	{
		constexpr usz size_element = std::is_void_v<T> ? 0 : sizeof(std::conditional_t<std::is_void_v<T>, char, T>);
		if (auto ptr = get_ptr<T>((addr + index).addr(), u32{size_element}))
		{
			return *ptr;
		}

		fmt::throw_exception("get_ref(): Failure! (addr=0x%x)%s", (addr + index).addr(), src_loc);
		return *std::add_pointer_t<to_be_t<T>>{};
	}
};

template <typename T>
struct main_ppu_module : public ppu_module<T>
{
	u32 elf_entry{};
	u32 seg0_code_end{};

	// Disable inherited savestate ordering
	void save(utils::serial&) = delete;
	static constexpr double savestate_init_pos = double{};
};

// Aux
struct ppu_pattern
{
	be_t<u32> opcode;
	be_t<u32> mask;

	ppu_pattern() = default;

	ppu_pattern(u32 op)
		: opcode(op)
		, mask(0xffffffff)
	{
	}

	ppu_pattern(u32 op, u32 ign)
		: opcode(op & ~ign)
		, mask(~ign)
	{
	}
};

struct ppu_pattern_array
{
	const ppu_pattern* ptr;
	usz count;

	template <usz N>
	constexpr ppu_pattern_array(const ppu_pattern(&array)[N])
		: ptr(array)
		, count(N)
	{
	}

	constexpr const ppu_pattern* begin() const
	{
		return ptr;
	}

	constexpr const ppu_pattern* end() const
	{
		return ptr + count;
	}
};

struct ppu_pattern_matrix
{
	const ppu_pattern_array* ptr;
	usz count;

	template <usz N>
	constexpr ppu_pattern_matrix(const ppu_pattern_array(&array)[N])
		: ptr(array)
		, count(N)
	{
	}

	constexpr const ppu_pattern_array* begin() const
	{
		return ptr;
	}

	constexpr const ppu_pattern_array* end() const
	{
		return ptr + count;
	}
};

// PPU Instruction Type
struct ppu_itype
{
	static constexpr struct branch_tag{} branch{}; // Branch Instructions
	static constexpr struct trap_tag{} trap{}; // Branch Instructions

	enum class type
	{
		UNK = 0,

		MFVSCR,
		MTVSCR,
		VADDCUW,
		VADDFP,
		VADDSBS,
		VADDSHS,
		VADDSWS,
		VADDUBM,
		VADDUBS,
		VADDUHM,
		VADDUHS,
		VADDUWM,
		VADDUWS,
		VAND,
		VANDC,
		VAVGSB,
		VAVGSH,
		VAVGSW,
		VAVGUB,
		VAVGUH,
		VAVGUW,
		VCFSX,
		VCFUX,
		VCMPBFP,
		VCMPBFP_,
		VCMPEQFP,
		VCMPEQFP_,
		VCMPEQUB,
		VCMPEQUB_,
		VCMPEQUH,
		VCMPEQUH_,
		VCMPEQUW,
		VCMPEQUW_,
		VCMPGEFP,
		VCMPGEFP_,
		VCMPGTFP,
		VCMPGTFP_,
		VCMPGTSB,
		VCMPGTSB_,
		VCMPGTSH,
		VCMPGTSH_,
		VCMPGTSW,
		VCMPGTSW_,
		VCMPGTUB,
		VCMPGTUB_,
		VCMPGTUH,
		VCMPGTUH_,
		VCMPGTUW,
		VCMPGTUW_,
		VCTSXS,
		VCTUXS,
		VEXPTEFP,
		VLOGEFP,
		VMADDFP,
		VMAXFP,
		VMAXSB,
		VMAXSH,
		VMAXSW,
		VMAXUB,
		VMAXUH,
		VMAXUW,
		VMHADDSHS,
		VMHRADDSHS,
		VMINFP,
		VMINSB,
		VMINSH,
		VMINSW,
		VMINUB,
		VMINUH,
		VMINUW,
		VMLADDUHM,
		VMRGHB,
		VMRGHH,
		VMRGHW,
		VMRGLB,
		VMRGLH,
		VMRGLW,
		VMSUMMBM,
		VMSUMSHM,
		VMSUMSHS,
		VMSUMUBM,
		VMSUMUHM,
		VMSUMUHS,
		VMULESB,
		VMULESH,
		VMULEUB,
		VMULEUH,
		VMULOSB,
		VMULOSH,
		VMULOUB,
		VMULOUH,
		VNMSUBFP,
		VNOR,
		VOR,
		VPERM,
		VPKPX,
		VPKSHSS,
		VPKSHUS,
		VPKSWSS,
		VPKSWUS,
		VPKUHUM,
		VPKUHUS,
		VPKUWUM,
		VPKUWUS,
		VREFP,
		VRFIM,
		VRFIN,
		VRFIP,
		VRFIZ,
		VRLB,
		VRLH,
		VRLW,
		VRSQRTEFP,
		VSEL,
		VSL,
		VSLB,
		VSLDOI,
		VSLH,
		VSLO,
		VSLW,
		VSPLTB,
		VSPLTH,
		VSPLTISB,
		VSPLTISH,
		VSPLTISW,
		VSPLTW,
		VSR,
		VSRAB,
		VSRAH,
		VSRAW,
		VSRB,
		VSRH,
		VSRO,
		VSRW,
		VSUBCUW,
		VSUBFP,
		VSUBSBS,
		VSUBSHS,
		VSUBSWS,
		VSUBUBM,
		VSUBUBS,
		VSUBUHM,
		VSUBUHS,
		VSUBUWM,
		VSUBUWS,
		VSUMSWS,
		VSUM2SWS,
		VSUM4SBS,
		VSUM4SHS,
		VSUM4UBS,
		VUPKHPX,
		VUPKHSB,
		VUPKHSH,
		VUPKLPX,
		VUPKLSB,
		VUPKLSH,
		VXOR,
		MULLI,
		SUBFIC,
		CMPLI,
		CMPI,
		ADDIC,
		ADDI,
		ADDIS,
		SC,
		MCRF,
		CRNOR,
		CRANDC,
		ISYNC,
		CRXOR,
		CRNAND,
		CRAND,
		CREQV,
		CRORC,
		CROR,
		RLWIMI,
		RLWINM,
		RLWNM,
		ORI,
		ORIS,
		XORI,
		XORIS,
		ANDI,
		ANDIS,
		RLDICL,
		RLDICR,
		RLDIC,
		RLDIMI,
		RLDCL,
		RLDCR,
		CMP,
		LVSL,
		LVEBX,
		SUBFC,
		SUBFCO,
		ADDC,
		ADDCO,
		MULHDU,
		MULHWU,
		MFOCRF,
		LWARX,
		LDX,
		LWZX,
		SLW,
		CNTLZW,
		SLD,
		AND,
		CMPL,
		LVSR,
		LVEHX,
		SUBF,
		SUBFO,
		LDUX,
		DCBST,
		LWZUX,
		CNTLZD,
		ANDC,
		LVEWX,
		MULHD,
		MULHW,
		LDARX,
		DCBF,
		LBZX,
		LVX,
		NEG,
		NEGO,
		LBZUX,
		NOR,
		STVEBX,
		SUBFE,
		SUBFEO,
		ADDE,
		ADDEO,
		MTOCRF,
		STDX,
		STWCX,
		STWX,
		STVEHX,
		STDUX,
		STWUX,
		STVEWX,
		SUBFZE,
		SUBFZEO,
		ADDZE,
		ADDZEO,
		STDCX,
		STBX,
		STVX,
		SUBFME,
		SUBFMEO,
		MULLD,
		MULLDO,
		ADDME,
		ADDMEO,
		MULLW,
		MULLWO,
		DCBTST,
		STBUX,
		ADD,
		ADDO,
		DCBT,
		LHZX,
		EQV,
		ECIWX,
		LHZUX,
		XOR,
		MFSPR,
		LWAX,
		DST,
		LHAX,
		LVXL,
		MFTB,
		LWAUX,
		DSTST,
		LHAUX,
		STHX,
		ORC,
		ECOWX,
		STHUX,
		OR,
		DIVDU,
		DIVDUO,
		DIVWU,
		DIVWUO,
		MTSPR,
		DCBI,
		NAND,
		STVXL,
		DIVD,
		DIVDO,
		DIVW,
		DIVWO,
		LVLX,
		LDBRX,
		LSWX,
		LWBRX,
		LFSX,
		SRW,
		SRD,
		LVRX,
		LSWI,
		LFSUX,
		SYNC,
		LFDX,
		LFDUX,
		STVLX,
		STDBRX,
		STSWX,
		STWBRX,
		STFSX,
		STVRX,
		STFSUX,
		STSWI,
		STFDX,
		STFDUX,
		LVLXL,
		LHBRX,
		SRAW,
		SRAD,
		LVRXL,
		DSS,
		SRAWI,
		SRADI,
		EIEIO,
		STVLXL,
		STHBRX,
		EXTSH,
		STVRXL,
		EXTSB,
		STFIWX,
		EXTSW,
		ICBI,
		DCBZ,
		LWZ,
		LWZU,
		LBZ,
		LBZU,
		STW,
		STWU,
		STB,
		STBU,
		LHZ,
		LHZU,
		LHA,
		LHAU,
		STH,
		STHU,
		LMW,
		STMW,
		LFS,
		LFSU,
		LFD,
		LFDU,
		STFS,
		STFSU,
		STFD,
		STFDU,
		LD,
		LDU,
		LWA,
		STD,
		STDU,
		FDIVS,
		FSUBS,
		FADDS,
		FSQRTS,
		FRES,
		FMULS,
		FMADDS,
		FMSUBS,
		FNMSUBS,
		FNMADDS,
		MTFSB1,
		MCRFS,
		MTFSB0,
		MTFSFI,
		MFFS,
		MTFSF,
		FCMPU,
		FRSP,
		FCTIW,
		FCTIWZ,
		FDIV,
		FSUB,
		FADD,
		FSQRT,
		FSEL,
		FMUL,
		FRSQRTE,
		FMSUB,
		FMADD,
		FNMSUB,
		FNMADD,
		FCMPO,
		FNEG,
		FMR,
		FNABS,
		FABS,
		FCTID,
		FCTIDZ,
		FCFID,

		SUBFCO_,
		ADDCO_,
		SUBFO_,
		NEGO_,
		SUBFEO_,
		ADDEO_,
		SUBFZEO_,
		ADDZEO_,
		SUBFMEO_,
		MULLDO_,
		ADDMEO_,
		MULLWO_,
		ADDO_,
		DIVDUO_,
		DIVWUO_,
		DIVDO_,
		DIVWO_,

		RLWIMI_,
		RLWINM_,
		RLWNM_,
		RLDICL_,
		RLDICR_,
		RLDIC_,
		RLDIMI_,
		RLDCL_,
		RLDCR_,
		SUBFC_,
		MULHDU_,
		ADDC_,
		MULHWU_,
		SLW_,
		CNTLZW_,
		SLD_,
		AND_,
		SUBF_,
		CNTLZD_,
		ANDC_,
		MULHD_,
		MULHW_,
		NEG_,
		NOR_,
		SUBFE_,
		ADDE_,
		SUBFZE_,
		ADDZE_,
		MULLD_,
		SUBFME_,
		ADDME_,
		MULLW_,
		ADD_,
		EQV_,
		XOR_,
		ORC_,
		OR_,
		DIVDU_,
		DIVWU_,
		NAND_,
		DIVD_,
		DIVW_,
		SRW_,
		SRD_,
		SRAW_,
		SRAD_,
		SRAWI_,
		SRADI_,
		EXTSH_,
		EXTSB_,
		EXTSW_,
		FDIVS_,
		FSUBS_,
		FADDS_,
		FSQRTS_,
		FRES_,
		FMULS_,
		FMADDS_,
		FMSUBS_,
		FNMSUBS_,
		FNMADDS_,
		MTFSB1_,
		MTFSB0_,
		MTFSFI_,
		MFFS_,
		MTFSF_,
		FRSP_,
		FCTIW_,
		FCTIWZ_,
		FDIV_,
		FSUB_,
		FADD_,
		FSQRT_,
		FSEL_,
		FMUL_,
		FRSQRTE_,
		FMSUB_,
		FMADD_,
		FNMSUB_,
		FNMADD_,
		FNEG_,
		FMR_,
		FNABS_,
		FABS_,
		FCTID_,
		FCTIDZ_,
		FCFID_,

		B, // branch_tag first
		BC,
		BCLR,
		BCCTR, // branch_tag last

		TD, // trap_tag first
		TW,
		TDI,
		TWI, // trap_tag last
	};

	using enum type;

	// Enable address-of operator for ppu_decoder<>
	friend constexpr type operator &(type value)
	{
		return value;
	}

	friend constexpr bool operator &(type value, branch_tag)
	{
		return value >= B && value <= BCCTR;
	}

	friend constexpr bool operator &(type value, trap_tag)
	{
		return value >= TD && value <= TWI;
	}
};

struct ppu_iname
{
#define NAME(x) static constexpr const char& x = *#x;
#define NAME_(x) static constexpr const char& x##_ = *#x ".";
	NAME(UNK)
	NAME(MFVSCR)
	NAME(MTVSCR)
	NAME(VADDCUW)
	NAME(VADDFP)
	NAME(VADDSBS)
	NAME(VADDSHS)
	NAME(VADDSWS)
	NAME(VADDUBM)
	NAME(VADDUBS)
	NAME(VADDUHM)
	NAME(VADDUHS)
	NAME(VADDUWM)
	NAME(VADDUWS)
	NAME(VAND)
	NAME(VANDC)
	NAME(VAVGSB)
	NAME(VAVGSH)
	NAME(VAVGSW)
	NAME(VAVGUB)
	NAME(VAVGUH)
	NAME(VAVGUW)
	NAME(VCFSX)
	NAME(VCFUX)
	NAME(VCMPBFP)
	NAME_(VCMPBFP)
	NAME(VCMPEQFP)
	NAME_(VCMPEQFP)
	NAME(VCMPEQUB)
	NAME_(VCMPEQUB)
	NAME(VCMPEQUH)
	NAME_(VCMPEQUH)
	NAME(VCMPEQUW)
	NAME_(VCMPEQUW)
	NAME(VCMPGEFP)
	NAME_(VCMPGEFP)
	NAME(VCMPGTFP)
	NAME_(VCMPGTFP)
	NAME(VCMPGTSB)
	NAME_(VCMPGTSB)
	NAME(VCMPGTSH)
	NAME_(VCMPGTSH)
	NAME(VCMPGTSW)
	NAME_(VCMPGTSW)
	NAME(VCMPGTUB)
	NAME_(VCMPGTUB)
	NAME(VCMPGTUH)
	NAME_(VCMPGTUH)
	NAME(VCMPGTUW)
	NAME_(VCMPGTUW)
	NAME(VCTSXS)
	NAME(VCTUXS)
	NAME(VEXPTEFP)
	NAME(VLOGEFP)
	NAME(VMADDFP)
	NAME(VMAXFP)
	NAME(VMAXSB)
	NAME(VMAXSH)
	NAME(VMAXSW)
	NAME(VMAXUB)
	NAME(VMAXUH)
	NAME(VMAXUW)
	NAME(VMHADDSHS)
	NAME(VMHRADDSHS)
	NAME(VMINFP)
	NAME(VMINSB)
	NAME(VMINSH)
	NAME(VMINSW)
	NAME(VMINUB)
	NAME(VMINUH)
	NAME(VMINUW)
	NAME(VMLADDUHM)
	NAME(VMRGHB)
	NAME(VMRGHH)
	NAME(VMRGHW)
	NAME(VMRGLB)
	NAME(VMRGLH)
	NAME(VMRGLW)
	NAME(VMSUMMBM)
	NAME(VMSUMSHM)
	NAME(VMSUMSHS)
	NAME(VMSUMUBM)
	NAME(VMSUMUHM)
	NAME(VMSUMUHS)
	NAME(VMULESB)
	NAME(VMULESH)
	NAME(VMULEUB)
	NAME(VMULEUH)
	NAME(VMULOSB)
	NAME(VMULOSH)
	NAME(VMULOUB)
	NAME(VMULOUH)
	NAME(VNMSUBFP)
	NAME(VNOR)
	NAME(VOR)
	NAME(VPERM)
	NAME(VPKPX)
	NAME(VPKSHSS)
	NAME(VPKSHUS)
	NAME(VPKSWSS)
	NAME(VPKSWUS)
	NAME(VPKUHUM)
	NAME(VPKUHUS)
	NAME(VPKUWUM)
	NAME(VPKUWUS)
	NAME(VREFP)
	NAME(VRFIM)
	NAME(VRFIN)
	NAME(VRFIP)
	NAME(VRFIZ)
	NAME(VRLB)
	NAME(VRLH)
	NAME(VRLW)
	NAME(VRSQRTEFP)
	NAME(VSEL)
	NAME(VSL)
	NAME(VSLB)
	NAME(VSLDOI)
	NAME(VSLH)
	NAME(VSLO)
	NAME(VSLW)
	NAME(VSPLTB)
	NAME(VSPLTH)
	NAME(VSPLTISB)
	NAME(VSPLTISH)
	NAME(VSPLTISW)
	NAME(VSPLTW)
	NAME(VSR)
	NAME(VSRAB)
	NAME(VSRAH)
	NAME(VSRAW)
	NAME(VSRB)
	NAME(VSRH)
	NAME(VSRO)
	NAME(VSRW)
	NAME(VSUBCUW)
	NAME(VSUBFP)
	NAME(VSUBSBS)
	NAME(VSUBSHS)
	NAME(VSUBSWS)
	NAME(VSUBUBM)
	NAME(VSUBUBS)
	NAME(VSUBUHM)
	NAME(VSUBUHS)
	NAME(VSUBUWM)
	NAME(VSUBUWS)
	NAME(VSUMSWS)
	NAME(VSUM2SWS)
	NAME(VSUM4SBS)
	NAME(VSUM4SHS)
	NAME(VSUM4UBS)
	NAME(VUPKHPX)
	NAME(VUPKHSB)
	NAME(VUPKHSH)
	NAME(VUPKLPX)
	NAME(VUPKLSB)
	NAME(VUPKLSH)
	NAME(VXOR)
	NAME(TDI)
	NAME(TWI)
	NAME(MULLI)
	NAME(SUBFIC)
	NAME(CMPLI)
	NAME(CMPI)
	NAME(ADDIC)
	NAME(ADDI)
	NAME(ADDIS)
	NAME(BC)
	NAME(SC)
	NAME(B)
	NAME(MCRF)
	NAME(BCLR)
	NAME(CRNOR)
	NAME(CRANDC)
	NAME(ISYNC)
	NAME(CRXOR)
	NAME(CRNAND)
	NAME(CRAND)
	NAME(CREQV)
	NAME(CRORC)
	NAME(CROR)
	NAME(BCCTR)
	NAME(RLWIMI)
	NAME(RLWINM)
	NAME(RLWNM)
	NAME(ORI)
	NAME(ORIS)
	NAME(XORI)
	NAME(XORIS)
	NAME(ANDI)
	NAME(ANDIS)
	NAME(RLDICL)
	NAME(RLDICR)
	NAME(RLDIC)
	NAME(RLDIMI)
	NAME(RLDCL)
	NAME(RLDCR)
	NAME(CMP)
	NAME(TW)
	NAME(LVSL)
	NAME(LVEBX)
	NAME(SUBFC)
	NAME(ADDC)
	NAME(MULHDU)
	NAME(MULHWU)
	NAME(MFOCRF)
	NAME(LWARX)
	NAME(LDX)
	NAME(LWZX)
	NAME(SLW)
	NAME(CNTLZW)
	NAME(SLD)
	NAME(AND)
	NAME(CMPL)
	NAME(LVSR)
	NAME(LVEHX)
	NAME(SUBF)
	NAME(LDUX)
	NAME(DCBST)
	NAME(LWZUX)
	NAME(CNTLZD)
	NAME(ANDC)
	NAME(TD)
	NAME(LVEWX)
	NAME(MULHD)
	NAME(MULHW)
	NAME(LDARX)
	NAME(DCBF)
	NAME(LBZX)
	NAME(LVX)
	NAME(NEG)
	NAME(LBZUX)
	NAME(NOR)
	NAME(STVEBX)
	NAME(SUBFE)
	NAME(ADDE)
	NAME(MTOCRF)
	NAME(STDX)
	NAME(STWCX)
	NAME(STWX)
	NAME(STVEHX)
	NAME(STDUX)
	NAME(STWUX)
	NAME(STVEWX)
	NAME(SUBFZE)
	NAME(ADDZE)
	NAME(STDCX)
	NAME(STBX)
	NAME(STVX)
	NAME(SUBFME)
	NAME(MULLD)
	NAME(ADDME)
	NAME(MULLW)
	NAME(DCBTST)
	NAME(STBUX)
	NAME(ADD)
	NAME(DCBT)
	NAME(LHZX)
	NAME(EQV)
	NAME(ECIWX)
	NAME(LHZUX)
	NAME(XOR)
	NAME(MFSPR)
	NAME(LWAX)
	NAME(DST)
	NAME(LHAX)
	NAME(LVXL)
	NAME(MFTB)
	NAME(LWAUX)
	NAME(DSTST)
	NAME(LHAUX)
	NAME(STHX)
	NAME(ORC)
	NAME(ECOWX)
	NAME(STHUX)
	NAME(OR)
	NAME(DIVDU)
	NAME(DIVWU)
	NAME(MTSPR)
	NAME(DCBI)
	NAME(NAND)
	NAME(STVXL)
	NAME(DIVD)
	NAME(DIVW)
	NAME(LVLX)
	NAME(LDBRX)
	NAME(LSWX)
	NAME(LWBRX)
	NAME(LFSX)
	NAME(SRW)
	NAME(SRD)
	NAME(LVRX)
	NAME(LSWI)
	NAME(LFSUX)
	NAME(SYNC)
	NAME(LFDX)
	NAME(LFDUX)
	NAME(STVLX)
	NAME(STDBRX)
	NAME(STSWX)
	NAME(STWBRX)
	NAME(STFSX)
	NAME(STVRX)
	NAME(STFSUX)
	NAME(STSWI)
	NAME(STFDX)
	NAME(STFDUX)
	NAME(LVLXL)
	NAME(LHBRX)
	NAME(SRAW)
	NAME(SRAD)
	NAME(LVRXL)
	NAME(DSS)
	NAME(SRAWI)
	NAME(SRADI)
	NAME(EIEIO)
	NAME(STVLXL)
	NAME(STHBRX)
	NAME(EXTSH)
	NAME(STVRXL)
	NAME(EXTSB)
	NAME(STFIWX)
	NAME(EXTSW)
	NAME(ICBI)
	NAME(DCBZ)
	NAME(LWZ)
	NAME(LWZU)
	NAME(LBZ)
	NAME(LBZU)
	NAME(STW)
	NAME(STWU)
	NAME(STB)
	NAME(STBU)
	NAME(LHZ)
	NAME(LHZU)
	NAME(LHA)
	NAME(LHAU)
	NAME(STH)
	NAME(STHU)
	NAME(LMW)
	NAME(STMW)
	NAME(LFS)
	NAME(LFSU)
	NAME(LFD)
	NAME(LFDU)
	NAME(STFS)
	NAME(STFSU)
	NAME(STFD)
	NAME(STFDU)
	NAME(LD)
	NAME(LDU)
	NAME(LWA)
	NAME(STD)
	NAME(STDU)
	NAME(FDIVS)
	NAME(FSUBS)
	NAME(FADDS)
	NAME(FSQRTS)
	NAME(FRES)
	NAME(FMULS)
	NAME(FMADDS)
	NAME(FMSUBS)
	NAME(FNMSUBS)
	NAME(FNMADDS)
	NAME(MTFSB1)
	NAME(MCRFS)
	NAME(MTFSB0)
	NAME(MTFSFI)
	NAME(MFFS)
	NAME(MTFSF)
	NAME(FCMPU)
	NAME(FRSP)
	NAME(FCTIW)
	NAME(FCTIWZ)
	NAME(FDIV)
	NAME(FSUB)
	NAME(FADD)
	NAME(FSQRT)
	NAME(FSEL)
	NAME(FMUL)
	NAME(FRSQRTE)
	NAME(FMSUB)
	NAME(FMADD)
	NAME(FNMSUB)
	NAME(FNMADD)
	NAME(FCMPO)
	NAME(FNEG)
	NAME(FMR)
	NAME(FNABS)
	NAME(FABS)
	NAME(FCTID)
	NAME(FCTIDZ)
	NAME(FCFID)

	NAME(SUBFCO)
	NAME(ADDCO)
	NAME(SUBFO)
	NAME(NEGO)
	NAME(SUBFEO)
	NAME(ADDEO)
	NAME(SUBFZEO)
	NAME(ADDZEO)
	NAME(SUBFMEO)
	NAME(MULLDO)
	NAME(ADDMEO)
	NAME(MULLWO)
	NAME(ADDO)
	NAME(DIVDUO)
	NAME(DIVWUO)
	NAME(DIVDO)
	NAME(DIVWO)

	NAME_(SUBFCO)
	NAME_(ADDCO)
	NAME_(SUBFO)
	NAME_(NEGO)
	NAME_(SUBFEO)
	NAME_(ADDEO)
	NAME_(SUBFZEO)
	NAME_(ADDZEO)
	NAME_(SUBFMEO)
	NAME_(MULLDO)
	NAME_(ADDMEO)
	NAME_(MULLWO)
	NAME_(ADDO)
	NAME_(DIVDUO)
	NAME_(DIVWUO)
	NAME_(DIVDO)
	NAME_(DIVWO)

	NAME_(RLWIMI)
	NAME_(RLWINM)
	NAME_(RLWNM)
	NAME_(RLDICL)
	NAME_(RLDICR)
	NAME_(RLDIC)
	NAME_(RLDIMI)
	NAME_(RLDCL)
	NAME_(RLDCR)
	NAME_(SUBFC)
	NAME_(MULHDU)
	NAME_(ADDC)
	NAME_(MULHWU)
	NAME_(SLW)
	NAME_(CNTLZW)
	NAME_(SLD)
	NAME_(AND)
	NAME_(SUBF)
	NAME_(CNTLZD)
	NAME_(ANDC)
	NAME_(MULHD)
	NAME_(MULHW)
	NAME_(NEG)
	NAME_(NOR)
	NAME_(SUBFE)
	NAME_(ADDE)
	NAME_(SUBFZE)
	NAME_(ADDZE)
	NAME_(MULLD)
	NAME_(SUBFME)
	NAME_(ADDME)
	NAME_(MULLW)
	NAME_(ADD)
	NAME_(EQV)
	NAME_(XOR)
	NAME_(ORC)
	NAME_(OR)
	NAME_(DIVDU)
	NAME_(DIVWU)
	NAME_(NAND)
	NAME_(DIVD)
	NAME_(DIVW)
	NAME_(SRW)
	NAME_(SRD)
	NAME_(SRAW)
	NAME_(SRAD)
	NAME_(SRAWI)
	NAME_(SRADI)
	NAME_(EXTSH)
	NAME_(EXTSB)
	NAME_(EXTSW)
	NAME_(FDIVS)
	NAME_(FSUBS)
	NAME_(FADDS)
	NAME_(FSQRTS)
	NAME_(FRES)
	NAME_(FMULS)
	NAME_(FMADDS)
	NAME_(FMSUBS)
	NAME_(FNMSUBS)
	NAME_(FNMADDS)
	NAME_(MTFSB1)
	NAME_(MTFSB0)
	NAME_(MTFSFI)
	NAME_(MFFS)
	NAME_(MTFSF)
	NAME_(FRSP)
	NAME_(FCTIW)
	NAME_(FCTIWZ)
	NAME_(FDIV)
	NAME_(FSUB)
	NAME_(FADD)
	NAME_(FSQRT)
	NAME_(FSEL)
	NAME_(FMUL)
	NAME_(FRSQRTE)
	NAME_(FMSUB)
	NAME_(FMADD)
	NAME_(FNMSUB)
	NAME_(FNMADD)
	NAME_(FNEG)
	NAME_(FMR)
	NAME_(FNABS)
	NAME_(FABS)
	NAME_(FCTID)
	NAME_(FCTIDZ)
	NAME_(FCFID)
#undef NAME
#undef NAME_
};

// PPU Analyser Context
struct ppu_acontext
{
	// General-purpose register range
	struct spec_gpr
	{
		// Integral range: normalized undef = (0;UINT64_MAX), unnormalized undefs are possible (when max = min - 1)
		// Bit range: constant 0 = (0;0), constant 1 = (1;1), normalized undef = (0;1), unnormalized undef = (1;0)

		u64 imin = 0ull; // Integral range begin
		u64 imax = ~0ull; // Integral range end
		u64 bmin = 0ull; // Bit range begin
		u64 bmax = ~0ull; // Bit range end

		void set_undef()
		{
			imin = 0;
			imax = -1;
			bmin = 0;
			bmax = -1;
		}

		// (Number of possible values - 1), 0 = const
		u64 div() const
		{
			return imax - imin;
		}

		// Return zero bits for zeros, ones for ones or undefs
		u64 mask() const
		{
			return bmin | bmax;
		}

		// Return one bits for ones, zeros for zeros or undefs
		u64 ones() const
		{
			return bmin & bmax;
		}

		// Return one bits for undefs
		u64 undefs() const
		{
			return bmin ^ bmax;
		}

		// Return number of trailing zero bits
		u64 tz() const
		{
			return std::countr_zero(mask());
		}

		// Range NOT
		spec_gpr operator ~() const
		{
			spec_gpr r;
			r.imin = ~imax;
			r.imax = ~imin;
			r.bmin = ~bmax;
			r.bmax = ~bmin;
			return r;
		}

		// Range ADD
		spec_gpr operator +(const spec_gpr& rhs) const
		{
			spec_gpr r{};

			const u64 adiv = div();
			const u64 bdiv = rhs.div();

			// Check overflow, generate normalized range
			if (adiv != umax && bdiv != umax && adiv <= adiv + bdiv)
			{
				r = range(imin + rhs.imin, imax + rhs.imax);
			}

			// Carry for bitrange computation
			u64 cmin = 0;
			u64 cmax = 0;

			const u64 amask = mask();
			const u64 bmask = rhs.mask();
			const u64 aones = ones();
			const u64 bones = rhs.ones();

			for (u32 i = 0; i < 64; i++)
			{
				cmin += ((amask >> i) & 1) + ((bmask >> i) & 1);
				cmax += ((aones >> i) & 1) + ((bones >> i) & 1);

				// Discover some constant bits
				if (cmin == cmax)
				{
					r.bmin |= (cmin & 1) << i;
					r.bmax &= ~((~cmin & 1) << i);
				}

				cmin >>= 1;
				cmax >>= 1;
			}

			return r;
		}

		// Range AND
		spec_gpr operator &(const spec_gpr& rhs) const
		{
			// Ignore inverted ranges (TODO)
			if (imin > imax || rhs.imin > rhs.imax)
			{
				return approx(ones() & rhs.ones(), mask() & rhs.mask());
			}

			// Const (TODO: remove when unnecessary)
			if (imin == imax && rhs.imin == rhs.imax)
			{
				return fixed(imin & rhs.imin);
			}

			// Swap (TODO: remove when unnecessary)
			if (imin == imax || rhs.undefs() > undefs())
			{
				return rhs & *this;
			}

			// Copy and attempt to partially preserve integral range
			spec_gpr r = *this;

			for (u32 i = 63; ~i; i--)
			{
				const u64 m = 1ull << i;

				if (!(rhs.mask() & m))
				{
					if (r.undefs() & m)
					{
						// undef -> 0
						r.imin &= ~(m - 1);
						r.imax |= (m - 1);
						r.imin &= ~m;
						r.imax &= ~m;
					}
					else if (r.ones() & m)
					{
						// 1 -> 0
						if ((r.imin ^ r.imax) > (m - 1))
						{
							r.imin &= ~(m - 1);
							r.imax |= (m - 1);
						}

						r.imin &= ~m;
						r.imax &= ~m;
					}
				}
				else if (rhs.undefs() & m)
				{
					// -> undef
					r.imin &= ~(m - 1);
					r.imax |= (m - 1);
					r.imin &= ~m;
					r.imax |= m;
				}
			}

			r.bmin = ones() & rhs.ones();
			r.bmax = mask() & rhs.mask();
			return r;
		}

		// Range OR
		spec_gpr operator |(const spec_gpr& rhs) const
		{
			// Ignore inverted ranges (TODO)
			if (imin > imax || rhs.imin > rhs.imax)
			{
				return approx(ones() | rhs.ones(), mask() | rhs.mask());
			}

			// Const (TODO: remove when unnecessary)
			if (imin == imax && rhs.imin == rhs.imax)
			{
				return fixed(imin | rhs.imin);
			}

			// Swap (TODO: remove when unnecessary)
			if (imin == imax || rhs.undefs() > undefs())
			{
				return rhs | *this;
			}

			// Copy and attempt to partially preserve integral range
			spec_gpr r = *this;

			for (u32 i = 63; ~i; i--)
			{
				const u64 m = 1ull << i;

				if (rhs.ones() & m)
				{
					if (r.undefs() & m)
					{
						// undef -> 1
						r.imin &= ~(m - 1);
						r.imax |= (m - 1);
						r.imin |= m;
						r.imax |= m;
					}
					else if (!(r.mask() & m))
					{
						// 0 -> 1
						if ((r.imin ^ r.imax) > (m - 1))
						{
							r.imin &= ~(m - 1);
							r.imax |= (m - 1);
						}

						r.imin |= m;
						r.imax |= m;
					}
				}
				else if (rhs.undefs() & m)
				{
					// -> undef
					r.imin &= ~(m - 1);
					r.imax |= (m - 1);
					r.imin &= ~m;
					r.imax |= m;
				}
			}

			r.bmin = ones() | rhs.ones();
			r.bmax = mask() | rhs.mask();
			return r;
		}

		// Range XOR
		spec_gpr operator ^(const spec_gpr& rhs) const
		{
			return (~*this & rhs) | (*this & ~rhs);
		}

		// Check whether the value is in range
		bool test(u64 value) const
		{
			if (imin <= imax)
			{
				if (value < imin || value > imax)
				{
					return false;
				}
			}
			else
			{
				if (value < imin && value > imax)
				{
					return false;
				}
			}

			if ((value & mask()) != value)
			{
				return false;
			}

			if ((value | ones()) != value)
			{
				return false;
			}

			return true;
		}

		// Constant value
		static spec_gpr fixed(u64 value)
		{
			spec_gpr r;
			r.imin = value;
			r.imax = value;
			r.bmin = value;
			r.bmax = value;
			return r;
		}

		// Range (tz = number of constant trailing zeros)
		static spec_gpr range(u64 min, u64 max, u64 tz = 0)
		{
			const u64 mask = tz < 64 ? ~0ull << tz : 0ull;

			spec_gpr r;
			r.bmin = 0;
			r.bmax = mask;

			// Normalize min/max for tz (TODO)
			if (min < max)
			{
				// Inverted constant MSB mask
				const u64 mix = ~0ull >> std::countl_zero(min ^ max);
				r.bmin |= min & ~mix;
				r.bmax &= max | mix;

				r.imin = (min + ~mask) & mask;
				r.imax = max & mask;
				ensure(r.imin <= r.imax); // "Impossible range"
			}
			else
			{
				r.imin = min & mask;
				r.imax = (max + ~mask) & mask;
				ensure(r.imin >= r.imax); // "Impossible range"
			}

			// Fix const values
			if (r.imin == r.imax)
			{
				r.bmin = r.imin;
				r.bmax = r.imax;
			}

			return r;
		}

		// Make from bitrange (normalize, approximate range values)
		static spec_gpr approx(u64 bmin, u64 bmax)
		{
			spec_gpr r;
			r.imin = bmin & ~(bmin ^ bmax);
			r.imax = bmax | (bmin ^ bmax);
			r.bmin = bmin & ~(bmin ^ bmax);
			r.bmax = bmax | (bmin ^ bmax);
			return r;
		}
	} gpr[32]{};

	// Vector registers (draft)
	struct spec_vec
	{
		u8 imin8[16]{};
		u8 imax8[16]{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
		u16 imin16[8]{};
		u16 imax16[8]{0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
		u32 imin32[4]{};
		u32 imax32[4]{0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu};
		u64 bmin64[2]{};
		u64 bmax64[2]{0xffffffffffffffffull, 0xffffffffffffffffull};
	};

	// Info
	u32 cia;

	// Analyser step
	void UNK(ppu_opcode_t);

	void MFVSCR(ppu_opcode_t);
	void MTVSCR(ppu_opcode_t);
	void VADDCUW(ppu_opcode_t);
	void VADDFP(ppu_opcode_t);
	void VADDSBS(ppu_opcode_t);
	void VADDSHS(ppu_opcode_t);
	void VADDSWS(ppu_opcode_t);
	void VADDUBM(ppu_opcode_t);
	void VADDUBS(ppu_opcode_t);
	void VADDUHM(ppu_opcode_t);
	void VADDUHS(ppu_opcode_t);
	void VADDUWM(ppu_opcode_t);
	void VADDUWS(ppu_opcode_t);
	void VAND(ppu_opcode_t);
	void VANDC(ppu_opcode_t);
	void VAVGSB(ppu_opcode_t);
	void VAVGSH(ppu_opcode_t);
	void VAVGSW(ppu_opcode_t);
	void VAVGUB(ppu_opcode_t);
	void VAVGUH(ppu_opcode_t);
	void VAVGUW(ppu_opcode_t);
	void VCFSX(ppu_opcode_t);
	void VCFUX(ppu_opcode_t);
	void VCMPBFP(ppu_opcode_t);
	void VCMPEQFP(ppu_opcode_t);
	void VCMPEQUB(ppu_opcode_t);
	void VCMPEQUH(ppu_opcode_t);
	void VCMPEQUW(ppu_opcode_t);
	void VCMPGEFP(ppu_opcode_t);
	void VCMPGTFP(ppu_opcode_t);
	void VCMPGTSB(ppu_opcode_t);
	void VCMPGTSH(ppu_opcode_t);
	void VCMPGTSW(ppu_opcode_t);
	void VCMPGTUB(ppu_opcode_t);
	void VCMPGTUH(ppu_opcode_t);
	void VCMPGTUW(ppu_opcode_t);
	void VCTSXS(ppu_opcode_t);
	void VCTUXS(ppu_opcode_t);
	void VEXPTEFP(ppu_opcode_t);
	void VLOGEFP(ppu_opcode_t);
	void VMADDFP(ppu_opcode_t);
	void VMAXFP(ppu_opcode_t);
	void VMAXSB(ppu_opcode_t);
	void VMAXSH(ppu_opcode_t);
	void VMAXSW(ppu_opcode_t);
	void VMAXUB(ppu_opcode_t);
	void VMAXUH(ppu_opcode_t);
	void VMAXUW(ppu_opcode_t);
	void VMHADDSHS(ppu_opcode_t);
	void VMHRADDSHS(ppu_opcode_t);
	void VMINFP(ppu_opcode_t);
	void VMINSB(ppu_opcode_t);
	void VMINSH(ppu_opcode_t);
	void VMINSW(ppu_opcode_t);
	void VMINUB(ppu_opcode_t);
	void VMINUH(ppu_opcode_t);
	void VMINUW(ppu_opcode_t);
	void VMLADDUHM(ppu_opcode_t);
	void VMRGHB(ppu_opcode_t);
	void VMRGHH(ppu_opcode_t);
	void VMRGHW(ppu_opcode_t);
	void VMRGLB(ppu_opcode_t);
	void VMRGLH(ppu_opcode_t);
	void VMRGLW(ppu_opcode_t);
	void VMSUMMBM(ppu_opcode_t);
	void VMSUMSHM(ppu_opcode_t);
	void VMSUMSHS(ppu_opcode_t);
	void VMSUMUBM(ppu_opcode_t);
	void VMSUMUHM(ppu_opcode_t);
	void VMSUMUHS(ppu_opcode_t);
	void VMULESB(ppu_opcode_t);
	void VMULESH(ppu_opcode_t);
	void VMULEUB(ppu_opcode_t);
	void VMULEUH(ppu_opcode_t);
	void VMULOSB(ppu_opcode_t);
	void VMULOSH(ppu_opcode_t);
	void VMULOUB(ppu_opcode_t);
	void VMULOUH(ppu_opcode_t);
	void VNMSUBFP(ppu_opcode_t);
	void VNOR(ppu_opcode_t);
	void VOR(ppu_opcode_t);
	void VPERM(ppu_opcode_t);
	void VPKPX(ppu_opcode_t);
	void VPKSHSS(ppu_opcode_t);
	void VPKSHUS(ppu_opcode_t);
	void VPKSWSS(ppu_opcode_t);
	void VPKSWUS(ppu_opcode_t);
	void VPKUHUM(ppu_opcode_t);
	void VPKUHUS(ppu_opcode_t);
	void VPKUWUM(ppu_opcode_t);
	void VPKUWUS(ppu_opcode_t);
	void VREFP(ppu_opcode_t);
	void VRFIM(ppu_opcode_t);
	void VRFIN(ppu_opcode_t);
	void VRFIP(ppu_opcode_t);
	void VRFIZ(ppu_opcode_t);
	void VRLB(ppu_opcode_t);
	void VRLH(ppu_opcode_t);
	void VRLW(ppu_opcode_t);
	void VRSQRTEFP(ppu_opcode_t);
	void VSEL(ppu_opcode_t);
	void VSL(ppu_opcode_t);
	void VSLB(ppu_opcode_t);
	void VSLDOI(ppu_opcode_t);
	void VSLH(ppu_opcode_t);
	void VSLO(ppu_opcode_t);
	void VSLW(ppu_opcode_t);
	void VSPLTB(ppu_opcode_t);
	void VSPLTH(ppu_opcode_t);
	void VSPLTISB(ppu_opcode_t);
	void VSPLTISH(ppu_opcode_t);
	void VSPLTISW(ppu_opcode_t);
	void VSPLTW(ppu_opcode_t);
	void VSR(ppu_opcode_t);
	void VSRAB(ppu_opcode_t);
	void VSRAH(ppu_opcode_t);
	void VSRAW(ppu_opcode_t);
	void VSRB(ppu_opcode_t);
	void VSRH(ppu_opcode_t);
	void VSRO(ppu_opcode_t);
	void VSRW(ppu_opcode_t);
	void VSUBCUW(ppu_opcode_t);
	void VSUBFP(ppu_opcode_t);
	void VSUBSBS(ppu_opcode_t);
	void VSUBSHS(ppu_opcode_t);
	void VSUBSWS(ppu_opcode_t);
	void VSUBUBM(ppu_opcode_t);
	void VSUBUBS(ppu_opcode_t);
	void VSUBUHM(ppu_opcode_t);
	void VSUBUHS(ppu_opcode_t);
	void VSUBUWM(ppu_opcode_t);
	void VSUBUWS(ppu_opcode_t);
	void VSUMSWS(ppu_opcode_t);
	void VSUM2SWS(ppu_opcode_t);
	void VSUM4SBS(ppu_opcode_t);
	void VSUM4SHS(ppu_opcode_t);
	void VSUM4UBS(ppu_opcode_t);
	void VUPKHPX(ppu_opcode_t);
	void VUPKHSB(ppu_opcode_t);
	void VUPKHSH(ppu_opcode_t);
	void VUPKLPX(ppu_opcode_t);
	void VUPKLSB(ppu_opcode_t);
	void VUPKLSH(ppu_opcode_t);
	void VXOR(ppu_opcode_t);
	void TDI(ppu_opcode_t);
	void TWI(ppu_opcode_t);
	void MULLI(ppu_opcode_t);
	void SUBFIC(ppu_opcode_t);
	void CMPLI(ppu_opcode_t);
	void CMPI(ppu_opcode_t);
	void ADDIC(ppu_opcode_t);
	void ADDI(ppu_opcode_t);
	void ADDIS(ppu_opcode_t);
	void BC(ppu_opcode_t);
	void SC(ppu_opcode_t);
	void B(ppu_opcode_t);
	void MCRF(ppu_opcode_t);
	void BCLR(ppu_opcode_t);
	void CRNOR(ppu_opcode_t);
	void CRANDC(ppu_opcode_t);
	void ISYNC(ppu_opcode_t);
	void CRXOR(ppu_opcode_t);
	void CRNAND(ppu_opcode_t);
	void CRAND(ppu_opcode_t);
	void CREQV(ppu_opcode_t);
	void CRORC(ppu_opcode_t);
	void CROR(ppu_opcode_t);
	void BCCTR(ppu_opcode_t);
	void RLWIMI(ppu_opcode_t);
	void RLWINM(ppu_opcode_t);
	void RLWNM(ppu_opcode_t);
	void ORI(ppu_opcode_t);
	void ORIS(ppu_opcode_t);
	void XORI(ppu_opcode_t);
	void XORIS(ppu_opcode_t);
	void ANDI(ppu_opcode_t);
	void ANDIS(ppu_opcode_t);
	void RLDICL(ppu_opcode_t);
	void RLDICR(ppu_opcode_t);
	void RLDIC(ppu_opcode_t);
	void RLDIMI(ppu_opcode_t);
	void RLDCL(ppu_opcode_t);
	void RLDCR(ppu_opcode_t);
	void CMP(ppu_opcode_t);
	void TW(ppu_opcode_t);
	void LVSL(ppu_opcode_t);
	void LVEBX(ppu_opcode_t);
	void SUBFC(ppu_opcode_t);
	void ADDC(ppu_opcode_t);
	void MULHDU(ppu_opcode_t);
	void MULHWU(ppu_opcode_t);
	void MFOCRF(ppu_opcode_t);
	void LWARX(ppu_opcode_t);
	void LDX(ppu_opcode_t);
	void LWZX(ppu_opcode_t);
	void SLW(ppu_opcode_t);
	void CNTLZW(ppu_opcode_t);
	void SLD(ppu_opcode_t);
	void AND(ppu_opcode_t);
	void CMPL(ppu_opcode_t);
	void LVSR(ppu_opcode_t);
	void LVEHX(ppu_opcode_t);
	void SUBF(ppu_opcode_t);
	void LDUX(ppu_opcode_t);
	void DCBST(ppu_opcode_t);
	void LWZUX(ppu_opcode_t);
	void CNTLZD(ppu_opcode_t);
	void ANDC(ppu_opcode_t);
	void TD(ppu_opcode_t);
	void LVEWX(ppu_opcode_t);
	void MULHD(ppu_opcode_t);
	void MULHW(ppu_opcode_t);
	void LDARX(ppu_opcode_t);
	void DCBF(ppu_opcode_t);
	void LBZX(ppu_opcode_t);
	void LVX(ppu_opcode_t);
	void NEG(ppu_opcode_t);
	void LBZUX(ppu_opcode_t);
	void NOR(ppu_opcode_t);
	void STVEBX(ppu_opcode_t);
	void SUBFE(ppu_opcode_t);
	void ADDE(ppu_opcode_t);
	void MTOCRF(ppu_opcode_t);
	void STDX(ppu_opcode_t);
	void STWCX(ppu_opcode_t);
	void STWX(ppu_opcode_t);
	void STVEHX(ppu_opcode_t);
	void STDUX(ppu_opcode_t);
	void STWUX(ppu_opcode_t);
	void STVEWX(ppu_opcode_t);
	void SUBFZE(ppu_opcode_t);
	void ADDZE(ppu_opcode_t);
	void STDCX(ppu_opcode_t);
	void STBX(ppu_opcode_t);
	void STVX(ppu_opcode_t);
	void SUBFME(ppu_opcode_t);
	void MULLD(ppu_opcode_t);
	void ADDME(ppu_opcode_t);
	void MULLW(ppu_opcode_t);
	void DCBTST(ppu_opcode_t);
	void STBUX(ppu_opcode_t);
	void ADD(ppu_opcode_t);
	void DCBT(ppu_opcode_t);
	void LHZX(ppu_opcode_t);
	void EQV(ppu_opcode_t);
	void ECIWX(ppu_opcode_t);
	void LHZUX(ppu_opcode_t);
	void XOR(ppu_opcode_t);
	void MFSPR(ppu_opcode_t);
	void LWAX(ppu_opcode_t);
	void DST(ppu_opcode_t);
	void LHAX(ppu_opcode_t);
	void LVXL(ppu_opcode_t);
	void MFTB(ppu_opcode_t);
	void LWAUX(ppu_opcode_t);
	void DSTST(ppu_opcode_t);
	void LHAUX(ppu_opcode_t);
	void STHX(ppu_opcode_t);
	void ORC(ppu_opcode_t);
	void ECOWX(ppu_opcode_t);
	void STHUX(ppu_opcode_t);
	void OR(ppu_opcode_t);
	void DIVDU(ppu_opcode_t);
	void DIVWU(ppu_opcode_t);
	void MTSPR(ppu_opcode_t);
	void DCBI(ppu_opcode_t);
	void NAND(ppu_opcode_t);
	void STVXL(ppu_opcode_t);
	void DIVD(ppu_opcode_t);
	void DIVW(ppu_opcode_t);
	void LVLX(ppu_opcode_t);
	void LDBRX(ppu_opcode_t);
	void LSWX(ppu_opcode_t);
	void LWBRX(ppu_opcode_t);
	void LFSX(ppu_opcode_t);
	void SRW(ppu_opcode_t);
	void SRD(ppu_opcode_t);
	void LVRX(ppu_opcode_t);
	void LSWI(ppu_opcode_t);
	void LFSUX(ppu_opcode_t);
	void SYNC(ppu_opcode_t);
	void LFDX(ppu_opcode_t);
	void LFDUX(ppu_opcode_t);
	void STVLX(ppu_opcode_t);
	void STDBRX(ppu_opcode_t);
	void STSWX(ppu_opcode_t);
	void STWBRX(ppu_opcode_t);
	void STFSX(ppu_opcode_t);
	void STVRX(ppu_opcode_t);
	void STFSUX(ppu_opcode_t);
	void STSWI(ppu_opcode_t);
	void STFDX(ppu_opcode_t);
	void STFDUX(ppu_opcode_t);
	void LVLXL(ppu_opcode_t);
	void LHBRX(ppu_opcode_t);
	void SRAW(ppu_opcode_t);
	void SRAD(ppu_opcode_t);
	void LVRXL(ppu_opcode_t);
	void DSS(ppu_opcode_t);
	void SRAWI(ppu_opcode_t);
	void SRADI(ppu_opcode_t);
	void EIEIO(ppu_opcode_t);
	void STVLXL(ppu_opcode_t);
	void STHBRX(ppu_opcode_t);
	void EXTSH(ppu_opcode_t);
	void STVRXL(ppu_opcode_t);
	void EXTSB(ppu_opcode_t);
	void STFIWX(ppu_opcode_t);
	void EXTSW(ppu_opcode_t);
	void ICBI(ppu_opcode_t);
	void DCBZ(ppu_opcode_t);
	void LWZ(ppu_opcode_t);
	void LWZU(ppu_opcode_t);
	void LBZ(ppu_opcode_t);
	void LBZU(ppu_opcode_t);
	void STW(ppu_opcode_t);
	void STWU(ppu_opcode_t);
	void STB(ppu_opcode_t);
	void STBU(ppu_opcode_t);
	void LHZ(ppu_opcode_t);
	void LHZU(ppu_opcode_t);
	void LHA(ppu_opcode_t);
	void LHAU(ppu_opcode_t);
	void STH(ppu_opcode_t);
	void STHU(ppu_opcode_t);
	void LMW(ppu_opcode_t);
	void STMW(ppu_opcode_t);
	void LFS(ppu_opcode_t);
	void LFSU(ppu_opcode_t);
	void LFD(ppu_opcode_t);
	void LFDU(ppu_opcode_t);
	void STFS(ppu_opcode_t);
	void STFSU(ppu_opcode_t);
	void STFD(ppu_opcode_t);
	void STFDU(ppu_opcode_t);
	void LD(ppu_opcode_t);
	void LDU(ppu_opcode_t);
	void LWA(ppu_opcode_t);
	void STD(ppu_opcode_t);
	void STDU(ppu_opcode_t);
	void FDIVS(ppu_opcode_t);
	void FSUBS(ppu_opcode_t);
	void FADDS(ppu_opcode_t);
	void FSQRTS(ppu_opcode_t);
	void FRES(ppu_opcode_t);
	void FMULS(ppu_opcode_t);
	void FMADDS(ppu_opcode_t);
	void FMSUBS(ppu_opcode_t);
	void FNMSUBS(ppu_opcode_t);
	void FNMADDS(ppu_opcode_t);
	void MTFSB1(ppu_opcode_t);
	void MCRFS(ppu_opcode_t);
	void MTFSB0(ppu_opcode_t);
	void MTFSFI(ppu_opcode_t);
	void MFFS(ppu_opcode_t);
	void MTFSF(ppu_opcode_t);
	void FCMPU(ppu_opcode_t);
	void FRSP(ppu_opcode_t);
	void FCTIW(ppu_opcode_t);
	void FCTIWZ(ppu_opcode_t);
	void FDIV(ppu_opcode_t);
	void FSUB(ppu_opcode_t);
	void FADD(ppu_opcode_t);
	void FSQRT(ppu_opcode_t);
	void FSEL(ppu_opcode_t);
	void FMUL(ppu_opcode_t);
	void FRSQRTE(ppu_opcode_t);
	void FMSUB(ppu_opcode_t);
	void FMADD(ppu_opcode_t);
	void FNMSUB(ppu_opcode_t);
	void FNMADD(ppu_opcode_t);
	void FCMPO(ppu_opcode_t);
	void FNEG(ppu_opcode_t);
	void FMR(ppu_opcode_t);
	void FNABS(ppu_opcode_t);
	void FABS(ppu_opcode_t);
	void FCTID(ppu_opcode_t);
	void FCTIDZ(ppu_opcode_t);
	void FCFID(ppu_opcode_t);
};
