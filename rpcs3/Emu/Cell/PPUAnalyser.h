#pragma once

#include <map>
#include <set>

#include "Utilities/bit_set.h"
#include "Utilities/BEType.h"

// PPU Function Attributes
enum class ppu_attr : u32
{
	known_addr,
	known_size,
	no_return,
	no_size,
	uses_r0,
	entry_point,
	complex_stack,

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
	u32 gate_target = 0;

	std::map<u32, u32> blocks; // Basic blocks: addr -> size
	std::set<u32> called_from; // Set of called functions
};

// PPU Module Information
struct ppu_module
{
	std::string name;
	std::vector<ppu_function> funcs;
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
	std::size_t count;

	template <std::size_t N>
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
	std::size_t count;

	template <std::size_t N>
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

extern void ppu_validate(const std::string& fname, const std::vector<ppu_function>& funcs, u32 reloc);

extern std::vector<ppu_function> ppu_analyse(const std::vector<std::pair<u32, u32>>& segs, const std::vector<std::pair<u32, u32>>& secs, u32 lib_toc);

// PPU Instruction Type
struct ppu_itype
{
	enum type
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
		VCMPEQFP,
		VCMPEQUB,
		VCMPEQUH,
		VCMPEQUW,
		VCMPGEFP,
		VCMPGTFP,
		VCMPGTSB,
		VCMPGTSH,
		VCMPGTSW,
		VCMPGTUB,
		VCMPGTUH,
		VCMPGTUW,
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
		TDI,
		TWI,
		MULLI,
		SUBFIC,
		CMPLI,
		CMPI,
		ADDIC,
		ADDI,
		ADDIS,
		BC,
		SC,
		B,
		MCRF,
		BCLR,
		CRNOR,
		CRANDC,
		ISYNC,
		CRXOR,
		CRNAND,
		CRAND,
		CREQV,
		CRORC,
		CROR,
		BCCTR,
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
		TW,
		LVSL,
		LVEBX,
		SUBFC,
		ADDC,
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
		LDUX,
		DCBST,
		LWZUX,
		CNTLZD,
		ANDC,
		TD,
		LVEWX,
		MULHD,
		MULHW,
		LDARX,
		DCBF,
		LBZX,
		LVX,
		NEG,
		LBZUX,
		NOR,
		STVEBX,
		SUBFE,
		ADDE,
		MTOCRF,
		STDX,
		STWCX,
		STWX,
		STVEHX,
		STDUX,
		STWUX,
		STVEWX,
		SUBFZE,
		ADDZE,
		STDCX,
		STBX,
		STVX,
		SUBFME,
		MULLD,
		ADDME,
		MULLW,
		DCBTST,
		STBUX,
		ADD,
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
		DIVWU,
		MTSPR,
		DCBI,
		NAND,
		STVXL,
		DIVD,
		DIVW,
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
	};

	// Enable address-of operator for ppu_decoder<>
	friend constexpr type operator &(type value)
	{
		return value;
	}
};

// PPU Instruction Flags
struct ppu_iflag
{
	// Various flags (TODO)
	enum : u32
	{
		write_rd  = 1 << 0,
		write_ra  = 1 << 1,
		read_ra   = 1 << 2,
		read_rb   = 1 << 3,
		read_rs   = 1 << 4,
		write_vd  = 1 << 5,
		read_va   = 1 << 6,
		read_vb   = 1 << 7,
		read_vc   = 1 << 8,
		read_vs   = 1 << 9,
		write_frd = 1 << 10,
		read_fra  = 1 << 11,
		read_frb  = 1 << 12,
		read_frc  = 1 << 13,
		read_frs  = 1 << 14,
		rw_all    = 1 << 15,
	};

	enum flags : u32
	{
		UNK        = 0,

		MFVSCR     = 0,
		MTVSCR     = 0,
		VADDCUW    = 0,
		VADDFP     = 0,
		VADDSBS    = 0,
		VADDSHS    = 0,
		VADDSWS    = 0,
		VADDUBM    = 0,
		VADDUBS    = 0,
		VADDUHM    = 0,
		VADDUHS    = 0,
		VADDUWM    = 0,
		VADDUWS    = 0,
		VAND       = 0,
		VANDC      = 0,
		VAVGSB     = 0,
		VAVGSH     = 0,
		VAVGSW     = 0,
		VAVGUB     = 0,
		VAVGUH     = 0,
		VAVGUW     = 0,
		VCFSX      = 0,
		VCFUX      = 0,
		VCMPBFP    = 0,
		VCMPEQFP   = 0,
		VCMPEQUB   = 0,
		VCMPEQUH   = 0,
		VCMPEQUW   = 0,
		VCMPGEFP   = 0,
		VCMPGTFP   = 0,
		VCMPGTSB   = 0,
		VCMPGTSH   = 0,
		VCMPGTSW   = 0,
		VCMPGTUB   = 0,
		VCMPGTUH   = 0,
		VCMPGTUW   = 0,
		VCTSXS     = 0,
		VCTUXS     = 0,
		VEXPTEFP   = 0,
		VLOGEFP    = 0,
		VMADDFP    = 0,
		VMAXFP     = 0,
		VMAXSB     = 0,
		VMAXSH     = 0,
		VMAXSW     = 0,
		VMAXUB     = 0,
		VMAXUH     = 0,
		VMAXUW     = 0,
		VMHADDSHS  = 0,
		VMHRADDSHS = 0,
		VMINFP     = 0,
		VMINSB     = 0,
		VMINSH     = 0,
		VMINSW     = 0,
		VMINUB     = 0,
		VMINUH     = 0,
		VMINUW     = 0,
		VMLADDUHM  = 0,
		VMRGHB     = 0,
		VMRGHH     = 0,
		VMRGHW     = 0,
		VMRGLB     = 0,
		VMRGLH     = 0,
		VMRGLW     = 0,
		VMSUMMBM   = 0,
		VMSUMSHM   = 0,
		VMSUMSHS   = 0,
		VMSUMUBM   = 0,
		VMSUMUHM   = 0,
		VMSUMUHS   = 0,
		VMULESB    = 0,
		VMULESH    = 0,
		VMULEUB    = 0,
		VMULEUH    = 0,
		VMULOSB    = 0,
		VMULOSH    = 0,
		VMULOUB    = 0,
		VMULOUH    = 0,
		VNMSUBFP   = 0,
		VNOR       = 0,
		VOR        = 0,
		VPERM      = 0,
		VPKPX      = 0,
		VPKSHSS    = 0,
		VPKSHUS    = 0,
		VPKSWSS    = 0,
		VPKSWUS    = 0,
		VPKUHUM    = 0,
		VPKUHUS    = 0,
		VPKUWUM    = 0,
		VPKUWUS    = 0,
		VREFP      = 0,
		VRFIM      = 0,
		VRFIN      = 0,
		VRFIP      = 0,
		VRFIZ      = 0,
		VRLB       = 0,
		VRLH       = 0,
		VRLW       = 0,
		VRSQRTEFP  = 0,
		VSEL       = 0,
		VSL        = 0,
		VSLB       = 0,
		VSLDOI     = 0,
		VSLH       = 0,
		VSLO       = 0,
		VSLW       = 0,
		VSPLTB     = 0,
		VSPLTH     = 0,
		VSPLTISB   = 0,
		VSPLTISH   = 0,
		VSPLTISW   = 0,
		VSPLTW     = 0,
		VSR        = 0,
		VSRAB      = 0,
		VSRAH      = 0,
		VSRAW      = 0,
		VSRB       = 0,
		VSRH       = 0,
		VSRO       = 0,
		VSRW       = 0,
		VSUBCUW    = 0,
		VSUBFP     = 0,
		VSUBSBS    = 0,
		VSUBSHS    = 0,
		VSUBSWS    = 0,
		VSUBUBM    = 0,
		VSUBUBS    = 0,
		VSUBUHM    = 0,
		VSUBUHS    = 0,
		VSUBUWM    = 0,
		VSUBUWS    = 0,
		VSUMSWS    = 0,
		VSUM2SWS   = 0,
		VSUM4SBS   = 0,
		VSUM4SHS   = 0,
		VSUM4UBS   = 0,
		VUPKHPX    = 0,
		VUPKHSB    = 0,
		VUPKHSH    = 0,
		VUPKLPX    = 0,
		VUPKLSB    = 0,
		VUPKLSH    = 0,
		VXOR       = 0,
		TDI        = read_ra,
		TWI        = read_ra,
		MULLI      = read_ra,
		SUBFIC     = read_ra,
		CMPLI      = read_ra,
		CMPI       = read_ra,
		ADDIC      = read_ra,
		ADDI       = read_ra,
		ADDIS      = read_ra,
		BC         = 0,
		SC         = 0,
		B          = 0,
		MCRF       = 0,
		BCLR       = 0,
		CRNOR      = 0,
		CRANDC     = 0,
		ISYNC      = 0,
		CRXOR      = 0,
		CRNAND     = 0,
		CRAND      = 0,
		CREQV      = 0,
		CRORC      = 0,
		CROR       = 0,
		BCCTR      = 0,
		RLWIMI     = read_ra | read_rs,
		RLWINM     = read_rs,
		RLWNM      = read_rs | read_rb,
		ORI        = read_rs,
		ORIS       = read_rs,
		XORI       = read_rs,
		XORIS      = read_rs,
		ANDI       = read_rs,
		ANDIS      = read_rs,
		RLDICL     = read_rs,
		RLDICR     = read_rs,
		RLDIC      = read_rs,
		RLDIMI     = read_ra | read_rs,
		RLDCL      = read_rs | read_rb,
		RLDCR      = read_rs | read_rb,
		CMP        = read_ra | read_rb,
		TW         = read_ra | read_rb,
		LVSL       = read_ra | read_rb,
		LVEBX      = read_ra | read_rb,
		SUBFC      = read_ra | read_rb,
		ADDC       = read_ra | read_rb,
		MULHDU     = read_ra | read_rb,
		MULHWU     = read_ra | read_rb,
		MFOCRF     = 0,
		LWARX      = read_ra | read_rb,
		LDX        = read_ra | read_rb,
		LWZX       = read_ra | read_rb,
		SLW        = read_rs | read_rb,
		CNTLZW     = read_rs,
		SLD        = read_rs | read_rb,
		AND        = read_rs | read_rb,
		CMPL       = read_ra | read_rb,
		LVSR       = read_ra | read_rb,
		LVEHX      = read_ra | read_rb,
		SUBF       = read_ra | read_rb,
		LDUX       = read_ra | read_rb,
		DCBST      = 0,
		LWZUX      = read_ra | read_rb,
		CNTLZD     = read_rs,
		ANDC       = read_rs | read_rb,
		TD         = read_ra | read_rb,
		LVEWX      = read_ra | read_rb,
		MULHD      = read_ra | read_rb,
		MULHW      = read_ra | read_rb,
		LDARX      = read_ra | read_rb,
		DCBF       = 0,
		LBZX       = read_ra | read_rb,
		LVX        = read_ra | read_rb,
		NEG        = read_ra,
		LBZUX      = read_ra | read_rb,
		NOR        = read_rs | read_rb,
		STVEBX     = read_ra | read_rb,
		SUBFE      = read_ra | read_rb,
		ADDE       = read_ra | read_rb,
		MTOCRF     = read_rs,
		STDX       = read_rs | read_ra | read_rb,
		STWCX      = read_rs | read_ra | read_rb,
		STWX       = read_rs | read_ra | read_rb,
		STVEHX     = read_ra | read_rb,
		STDUX      = read_rs | read_ra | read_rb,
		STWUX      = read_rs | read_ra | read_rb,
		STVEWX     = read_ra | read_rb,
		SUBFZE     = read_ra,
		ADDZE      = read_ra,
		STDCX      = read_rs | read_ra | read_rb,
		STBX       = read_rs | read_ra | read_rb,
		STVX       = read_ra | read_rb,
		SUBFME     = read_ra,
		MULLD      = read_ra | read_rb,
		ADDME      = read_ra | read_rb,
		MULLW      = read_ra | read_rb,
		DCBTST     = 0,
		STBUX      = read_rs | read_ra | read_rb,
		ADD        = read_ra | read_rb,
		DCBT       = 0,
		LHZX       = read_ra | read_rb,
		EQV        = read_rs | read_rb,
		ECIWX      = read_ra | read_rb,
		LHZUX      = read_ra | read_rb,
		XOR        = read_rs | read_rb,
		MFSPR      = 0,
		LWAX       = read_ra | read_rb,
		DST        = 0,
		LHAX       = read_ra | read_rb,
		LVXL       = LVX,
		MFTB       = MFSPR,
		LWAUX      = read_ra | read_rb,
		DSTST      = 0,
		LHAUX      = read_ra | read_rb,
		STHX       = read_rs | read_ra | read_rb,
		ORC        = read_rs | read_rb,
		ECOWX      = read_rs | read_ra | read_rb,
		STHUX      = read_rs | read_ra | read_rb,
		OR         = read_rs | read_rb,
		DIVDU      = read_ra | read_rb,
		DIVWU      = read_ra | read_rb,
		MTSPR      = read_rs,
		DCBI       = 0,
		NAND       = read_rs | read_rb,
		STVXL      = STVX,
		DIVD       = read_ra | read_rb,
		DIVW       = read_ra | read_rb,
		LVLX       = read_ra | read_rb,
		LDBRX      = read_ra | read_rb,
		LSWX       = read_ra | read_rb | rw_all,
		LWBRX      = read_ra | read_rb,
		LFSX       = read_ra | read_rb,
		SRW        = read_rs | read_rb,
		SRD        = read_rs | read_rb,
		LVRX       = read_ra | read_rb,
		LSWI       = rw_all,
		LFSUX      = read_ra | read_rb,
		SYNC       = 0,
		LFDX       = read_ra | read_rb,
		LFDUX      = read_ra | read_rb,
		STVLX      = read_ra | read_rb,
		STDBRX     = read_rs | read_ra | read_rb,
		STSWX      = rw_all,
		STWBRX     = read_rs | read_ra | read_rb,
		STFSX      = read_ra | read_rb,
		STVRX      = read_ra | read_rb,
		STFSUX     = read_ra | read_rb,
		STSWI      = rw_all,
		STFDX      = read_ra | read_rb,
		STFDUX     = read_ra | read_rb,
		LVLXL      = LVLX,
		LHBRX      = read_ra | read_rb,
		SRAW       = read_rs | read_rb,
		SRAD       = read_rs | read_rb,
		LVRXL      = LVRX,
		DSS        = 0,
		SRAWI      = read_rs,
		SRADI      = read_rs,
		EIEIO      = 0,
		STVLXL     = STVLX,
		STHBRX     = read_rs | read_ra | read_rb,
		EXTSH      = read_rs,
		STVRXL     = STVRX,
		EXTSB      = read_rs,
		STFIWX     = read_ra | read_rb,
		EXTSW      = read_rs,
		ICBI       = 0,
		DCBZ       = read_ra | read_rb,
		LWZ        = read_ra,
		LWZU       = read_ra,
		LBZ        = read_ra,
		LBZU       = read_ra,
		STW        = read_rs | read_ra,
		STWU       = read_rs | read_ra,
		STB        = read_rs | read_ra,
		STBU       = read_rs | read_ra,
		LHZ        = read_ra,
		LHZU       = read_ra,
		LHA        = read_ra,
		LHAU       = read_ra,
		STH        = read_rs | read_ra,
		STHU       = read_rs | read_ra,
		LMW        = rw_all,
		STMW       = rw_all,
		LFS        = read_ra,
		LFSU       = read_ra,
		LFD        = read_ra,
		LFDU       = read_ra,
		STFS       = read_ra,
		STFSU      = read_ra,
		STFD       = read_ra,
		STFDU      = read_ra,
		LD         = read_ra,
		LDU        = read_ra,
		LWA        = read_ra,
		STD        = read_rs | read_ra,
		STDU       = read_rs | read_ra,
		FDIVS      = 0,
		FSUBS      = 0,
		FADDS      = 0,
		FSQRTS     = 0,
		FRES       = 0,
		FMULS      = 0,
		FMADDS     = 0,
		FMSUBS     = 0,
		FNMSUBS    = 0,
		FNMADDS    = 0,
		MTFSB1     = 0,
		MCRFS      = 0,
		MTFSB0     = 0,
		MTFSFI     = 0,
		MFFS       = 0,
		MTFSF      = 0,
		FCMPU      = 0,
		FRSP       = 0,
		FCTIW      = 0,
		FCTIWZ     = 0,
		FDIV       = 0,
		FSUB       = 0,
		FADD       = 0,
		FSQRT      = 0,
		FSEL       = 0,
		FMUL       = 0,
		FRSQRTE    = 0,
		FMSUB      = 0,
		FMADD      = 0,
		FNMSUB     = 0,
		FNMADD     = 0,
		FCMPO      = 0,
		FNEG       = 0,
		FMR        = 0,
		FNABS      = 0,
		FABS       = 0,
		FCTID      = 0,
		FCTIDZ     = 0,
		FCFID      = 0,
	};

	// Enable address-of operator for ppu_decoder<>
	friend constexpr flags operator &(flags value)
	{
		return value;
	}
};

// Encode instruction name: 6 bits per character (0x20..0x5f), max 10
static constexpr u64 ppu_iname_encode(const char* ptr, u64 value = 0)
{
	return *ptr == '\0' ? value : ppu_iname_encode(ptr + 1, (*ptr - 0x20) | (value << 6));
}

struct ppu_iname
{
#define NAME(x) x = ppu_iname_encode(#x),

	enum type : u64
	{
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
	NAME(VCMPEQFP)
	NAME(VCMPEQUB)
	NAME(VCMPEQUH)
	NAME(VCMPEQUW)
	NAME(VCMPGEFP)
	NAME(VCMPGTFP)
	NAME(VCMPGTSB)
	NAME(VCMPGTSH)
	NAME(VCMPGTSW)
	NAME(VCMPGTUB)
	NAME(VCMPGTUH)
	NAME(VCMPGTUW)
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
	};

#undef NAME

	// Enable address-of operator for ppu_decoder<>
	friend constexpr type operator &(type value)
	{
		return value;
	}
};
