#pragma once

#include <string>
#include <map>
#include <deque>
#include <span>
#include "util/types.hpp"
#include "util/asm.hpp"
#include "util/to_endian.hpp"

#include "Utilities/bit_set.h"
#include "PPUOpcodes.h"

// PPU Function Attributes
enum class ppu_attr : u8
{
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

	std::map<u32, u32> blocks{}; // Basic blocks: addr -> size

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

struct ppua_reg_mask_t
{
	u64 mask;
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
	ppu_module* parent = nullptr; // For compilation: refers to original structure (is whole, not partitioned) 
	std::pair<u32, u32> local_bounds{0, u32{umax}}; // Module addresses range
	std::shared_ptr<std::pair<u32, u32>> jit_bounds; // JIT instance modules addresses range
	std::unordered_map<u32, void*> imports; // Imports information for release upon unload (TODO: OVL implementation!) 
	std::map<u32, std::vector<std::pair<ppua_reg_mask_t, u64>>> stub_addr_to_constant_state_of_registers; // Tells possible constant states of registers of functions
	bool is_relocatable = false; // Is code relocatable(?)

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
		is_relocatable = info.is_relocatable;
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
	static constexpr struct trap_tag{} trap{}; // Trap Instructions
	static constexpr struct store_tag{} store{}; // Memory store Instructions
	static constexpr struct load_tag{} load{}; // Memory load Instructions (TODO: Fill it)
	static constexpr struct memory_tag{} memory{}; // Memory Instructions

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
		LVSR,
		LVEHX,
		SUBF,
		SUBFO,
		LDUX,
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
		SUBFE,
		SUBFEO,
		ADDE,
		ADDEO,
		MTOCRF,
		SUBFZE,
		SUBFZEO,
		ADDZE,
		ADDZEO,
		SUBFME,
		SUBFMEO,
		MULLD,
		MULLDO,
		ADDME,
		ADDMEO,
		MULLW,
		MULLWO,
		ADD,
		ADDO,
		LHZX,
		EQV,
		ECIWX,
		LHZUX,
		XOR,
		MFSPR,
		LWAX,
		LHAX,
		LVXL,
		MFTB,
		LWAUX,
		LHAUX,
		ORC,
		ECOWX,
		OR,
		DIVDU,
		DIVDUO,
		DIVWU,
		DIVWUO,
		MTSPR,
		NAND,
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
		LFDX,
		LFDUX,
		LVLXL,
		LHBRX,
		SRAW,
		SRAD,
		LVRXL,
		SRAWI,
		SRADI,
		EXTSH,
		EXTSB,
		EXTSW,
		LWZ,
		LWZU,
		LBZ,
		LBZU,
		LHZ,
		LHZU,
		LHA,
		LHAU,
		LMW,
		LFS,
		LFSU,
		LFD,
		LFDU,
		LD,
		LDU,
		LWA,
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

		CMPLI,
		CMPI,
		CMP,
		CMPL,

		STMW, // store_tag first
		STSWX,
		STSWI,
		STVXL,
		STVLX,
		STVRX,
		STVEBX,
		STVEHX,
		STVEWX,
		STVX,
		STVLXL,
		STVRXL,
		STDX,
		STDUX,
		STDCX,
		STDBRX,
		STD,
		STDU,
		STFDUX,
		STFDX,
		STFDU,
		STFD,
		STFS,
		STFSU,
		STFSX,
		STFSUX,
		STFIWX,
		STWCX,
		STWX,
		STWUX,
		STWBRX,
		STWU,
		STW,
		STHBRX,
		STHX,
		STHUX,
		STH,
		STBX,
		STBU,
		STB,
		STHU,
		STBUX,
		DCBZ,
		DCBI, // Perceive memory barrier or flag instructions as stores
		DCBTST,
		DCBT,
		DCBST,
		DST,
		DSS,
		ICBI,
		SYNC,
		EIEIO,
		DSTST, // store_tag last

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

	friend constexpr bool operator &(type value, store_tag)
	{
		return value >= STMW && value <= DSTST;
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
