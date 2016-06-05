#pragma once

#include "Utilities/SharedMutex.h"

#include <set>

// SPU Instruction Type
struct spu_itype
{
	enum
	{
		memory   = 1 << 8,  // Memory Load/Store Instructions
		constant = 1 << 9,  // Constant Formation Instructions
		integer  = 1 << 10, // Integer and Logical Instructions
		shiftrot = 1 << 11, // Shift and Rotate Instructions
		compare  = 1 << 12, // Compare Instructions
		branch   = 1 << 13, // Branch Instructions
		floating = 1 << 14, // Floating-Point Instructions

		_quadrop = 1 << 15, // 4-op Instructions
	};

	enum type
	{
		UNK = 0,

		HEQ,
		HEQI,
		HGT,
		HGTI,
		HLGT,
		HLGTI,

		HBR,
		HBRA,
		HBRR,

		STOP,
		STOPD,
		LNOP,
		NOP,
		SYNC,
		DSYNC,
		MFSPR,
		MTSPR,
		RDCH,
		RCHCNT,
		WRCH,

		LQD = memory,
		LQX,
		LQA,
		LQR,
		STQD,
		STQX,
		STQA,
		STQR,

		CBD = constant,
		CBX,
		CHD,
		CHX,
		CWD,
		CWX,
		CDD,
		CDX,
		ILH,
		ILHU,
		IL,
		ILA,
		IOHL,
		FSMBI,

		AH = integer,
		AHI,
		A,
		AI,
		SFH,
		SFHI,
		SF,
		SFI,
		ADDX,
		CG,
		CGX,
		SFX,
		BG,
		BGX,
		MPY,
		MPYU,
		MPYI,
		MPYUI,
		MPYH,
		MPYS,
		MPYHH,
		MPYHHA,
		MPYHHU,
		MPYHHAU,
		CLZ,
		CNTB,
		FSMB,
		FSMH,
		FSM,
		GBB,
		GBH,
		GB,
		AVGB,
		ABSDB,
		SUMB,
		XSBH,
		XSHW,
		XSWD,
		AND,
		ANDC,
		ANDBI,
		ANDHI,
		ANDI,
		OR,
		ORC,
		ORBI,
		ORHI,
		ORI,
		ORX,
		XOR,
		XORBI,
		XORHI,
		XORI,
		NAND,
		NOR,
		EQV,

		MPYA = integer | _quadrop,
		SELB,
		SHUFB,

		SHLH = shiftrot,
		SHLHI,
		SHL,
		SHLI,
		SHLQBI,
		SHLQBII,
		SHLQBY,
		SHLQBYI,
		SHLQBYBI,
		ROTH,
		ROTHI,
		ROT,
		ROTI,
		ROTQBY,
		ROTQBYI,
		ROTQBYBI,
		ROTQBI,
		ROTQBII,
		ROTHM,
		ROTHMI,
		ROTM,
		ROTMI,
		ROTQMBY,
		ROTQMBYI,
		ROTQMBYBI,
		ROTQMBI,
		ROTQMBII,
		ROTMAH,
		ROTMAHI,
		ROTMA,
		ROTMAI,

		CEQB = compare,
		CEQBI,
		CEQH,
		CEQHI,
		CEQ,
		CEQI,
		CGTB,
		CGTBI,
		CGTH,
		CGTHI,
		CGT,
		CGTI,
		CLGTB,
		CLGTBI,
		CLGTH,
		CLGTHI,
		CLGT,
		CLGTI,

		BR = branch,
		BRA,
		BRSL,
		BRASL,
		BI,
		IRET,
		BISLED,
		BISL,
		BRNZ,
		BRZ,
		BRHNZ,
		BRHZ,
		BIZ,
		BINZ,
		BIHZ,
		BIHNZ,

		FA = floating,
		DFA,
		FS,
		DFS,
		FM,
		DFM,
		DFMA,
		DFNMS,
		DFMS,
		DFNMA,
		FREST,
		FRSQEST,
		FI,
		CSFLT,
		CFLTS,
		CUFLT,
		CFLTU,
		FRDS,
		FESD,
		FCEQ,
		FCMEQ,
		FCGT,
		FCMGT,
		FSCRWR,
		FSCRRD,

		DFCEQ,
		DFCMEQ,
		DFCGT,
		DFCMGT,
		DFTSV,

		FMA = floating | _quadrop,
		FNMS,
		FMS,
	};

	// Enable address-of operator for spu_decoder<>
	friend constexpr type operator &(type value)
	{
		return value;
	}
};

class SPUThread;

// SPU basic function information structure
struct spu_function_t
{
	// Entry point (LS address)
	const u32 addr;

	// Function size (in bytes)
	const u32 size;

	// Function contents (binary copy)
	std::vector<be_t<u32>> data;

	// Basic blocks (start addresses)
	std::set<u32> blocks;

	// Functions possibly called by this function (may not be available)
	std::set<u32> adjacent;

	// Jump table values (start addresses)
	std::set<u32> jtable;

	// Whether ila $SP,* instruction found
	bool does_reset_stack;

	// Pointer to the compiled function
	u32(*compiled)(SPUThread* _spu, be_t<u32>* _ls) = nullptr;

	spu_function_t(u32 addr, u32 size)
		: addr(addr)
		, size(size)
	{
	}
};

// SPU Function Database (must be global or PS3 process-local)
class SPUDatabase final : spu_itype
{
	shared_mutex m_mutex;

	// All registered functions (uses addr and first instruction as a key)
	std::unordered_multimap<u64, std::shared_ptr<spu_function_t>> m_db;

	// For internal use
	std::shared_ptr<spu_function_t> find(const be_t<u32>* data, u64 key, u32 max_size);

public:
	SPUDatabase();
	~SPUDatabase();

	// Try to retrieve SPU function information
	std::shared_ptr<spu_function_t> analyse(const be_t<u32>* ls, u32 entry, u32 limit = 0x40000);
};
