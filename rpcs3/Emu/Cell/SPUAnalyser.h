#pragma once

// SPU Instruction Type
struct spu_itype
{
	static constexpr struct memory_tag{} memory{}; // Memory Load/Store Instructions
	static constexpr struct constant_tag{} constant{}; // Constant Formation Instructions
	static constexpr struct integer_tag{} integer{}; // Integer and Logical Instructions
	static constexpr struct shiftrot_tag{} shiftrot{}; // Shift and Rotate Instructions
	static constexpr struct compare_tag{} compare{}; // Compare Instructions
	static constexpr struct branch_tag{} branch{}; // Branch Instructions
	static constexpr struct floating_tag{} floating{}; // Floating-Point Instructions
	static constexpr struct quadrop_tag{} _quadrop{}; // 4-op Instructions
	static constexpr struct xfloat_tag{} xfloat{}; // Instructions producing xfloat values
	static constexpr struct zregmod_tag{} zregmod{}; // Instructions not modifying any GPR

	enum class type : unsigned char
	{
		UNK = 0,

		HEQ, // zregmod_tag first
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
		MTSPR,
		WRCH,

		STQD, // memory_tag first
		STQX,
		STQA,
		STQR, // zregmod_tag last
		LQD,
		LQX,
		LQA,
		LQR, // memory_tag last

		MFSPR,
		RDCH,
		RCHCNT,

		BR, // branch_tag first
		BRA,
		BRNZ,
		BRZ,
		BRHNZ,
		BRHZ,
		BRSL,
		BRASL,
		IRET,
		BI,
		BISLED,
		BISL,
		BIZ,
		BINZ,
		BIHZ,
		BIHNZ, // branch_tag last

		ILH, // constant_tag_first
		ILHU,
		IL,
		ILA,
		FSMBI, // constant_tag last

		AH, // integer_tag first
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
		CBD,
		CHD,
		CWD,
		CDD,
		CBX,
		CHX,
		CWX,
		CDX,
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
		IOHL,
		ORI,
		ORX,
		XOR,
		XORBI,
		XORHI,
		XORI,
		NAND,
		NOR,
		EQV,

		MPYA, // quadrop_tag first
		SELB,
		SHUFB, // integer_tag last

		FMA, // floating_tag first
		FNMS,
		FMS, // quadrop_tag last

		FA,
		FS,
		FM,
		FREST,
		FRSQEST,
		FI,
		CSFLT,
		CUFLT,
		FRDS, // xfloat_tag last

		DFA,
		DFS,
		DFM,
		DFMA,
		DFNMS,
		DFMS,
		DFNMA,
		FESD,

		CFLTS,
		CFLTU,
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
		DFTSV, // floating_tag last

		SHLH, // shiftrot_tag first
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
		ROTMAI, // shiftrot_tag last

		CEQB, // compare_tag first
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
		CLGTI, // compare_tag last
	};

	using enum type;

	// Enable address-of operator for spu_decoder<>
	friend constexpr type operator &(type value)
	{
		return value;
	}

	// Test for branch instruction
	friend constexpr bool operator &(type value, branch_tag)
	{
		return value >= BR && value <= BIHNZ;
	}

	// Test for floating point instruction
	friend constexpr bool operator &(type value, floating_tag)
	{
		return value >= FMA && value <= DFTSV;
	}

	// Test for 4-op instruction
	friend constexpr bool operator &(type value, quadrop_tag)
	{
		return value >= MPYA && value <= FMS;
	}

	// Test for xfloat instruction
	friend constexpr bool operator &(type value, xfloat_tag)
	{
		return value >= FMA && value <= FRDS;
	}

	// Test for memory instruction
	friend constexpr bool operator &(type value, memory_tag)
	{
		return value >= STQD && value <= LQR;
	}

	// Test for compare instruction
	friend constexpr bool operator &(type value, compare_tag)
	{
		return value >= CEQB && value <= CLGTI;
	}

	// Test for integer instruction
	friend constexpr bool operator &(type value, integer_tag)
	{
		return value >= AH && value <= SHUFB;
	}

	// Test for shift or rotate instruction
	friend constexpr bool operator &(type value, shiftrot_tag)
	{
		return value >= SHLH && value <= ROTMAI;
	}

	// Test for constant loading instruction
	friend constexpr bool operator &(type value, constant_tag)
	{
		return value >= ILH && value <= FSMBI;
	}

	// Test for non register-modifying instruction
	friend constexpr bool operator &(type value, zregmod_tag)
	{
		return value >= HEQ && value <= STQR;
	}
};

struct spu_iflag
{
	enum
	{
		use_ra = 1 << 8,
		use_rb = 1 << 9,
		use_rc = 1 << 10,
	};

	enum flag
	{
		UNK = 0,
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
		DFCEQ,
		DFCMEQ,
		DFCGT,
		DFCMGT,
		DFTSV,
		RDCH,
		RCHCNT,
		LQA,
		LQR,
		ILH,
		ILHU,
		IL,
		ILA,
		FSMBI,
		BR,
		BRA,
		BRSL,
		BRASL,
		IRET,
		FSCRRD,

		WRCH = use_rc,
		IOHL,
		STQA,
		STQR,
		BRNZ,
		BRZ,
		BRHNZ,
		BRHZ,

		STQD = use_ra | use_rc,
		BIZ,
		BINZ,
		BIHZ,
		BIHNZ,

		STQX = use_ra | use_rb | use_rc,
		ADDX,
		CGX,
		SFX,
		BGX,
		MPYHHA,
		MPYHHAU,
		MPYA,
		SELB,
		SHUFB,
		DFMA,
		DFNMS,
		DFMS,
		DFNMA,
		FMA,
		FNMS,
		FMS,

		HEQI = use_ra,
		HGTI,
		HLGTI,
		LQD,
		CBD,
		CHD,
		CWD,
		CDD,
		AHI,
		AI,
		SFHI,
		SFI,
		MPYI,
		MPYUI,
		CLZ,
		CNTB,
		FSMB,
		FSMH,
		FSM,
		GBB,
		GBH,
		GB,
		XSBH,
		XSHW,
		XSWD,
		ANDBI,
		ANDHI,
		ANDI,
		ORBI,
		ORHI,
		ORI,
		ORX,
		XORBI,
		XORHI,
		XORI,
		SHLHI,
		SHLI,
		SHLQBII,
		SHLQBYI,
		ROTHI,
		ROTI,
		ROTQBYI,
		ROTQBII,
		ROTHMI,
		ROTMI,
		ROTQMBYI,
		ROTQMBII,
		ROTMAHI,
		ROTMAI,
		CEQBI,
		CEQHI,
		CEQI,
		CGTBI,
		CGTHI,
		CGTI,
		CLGTBI,
		CLGTHI,
		CLGTI,
		BI,
		BISLED,
		BISL,
		FREST,
		FRSQEST,
		CSFLT,
		CFLTS,
		CUFLT,
		CFLTU,
		FRDS,
		FESD,
		FSCRWR,

		HEQ = use_ra | use_rb,
		HGT,
		HLGT,
		LQX,
		CBX,
		CHX,
		CWX,
		CDX,
		AH,
		A,
		SFH,
		SF,
		CG,
		BG,
		MPYHHU,
		MPY,
		MPYU,
		MPYH,
		MPYS,
		MPYHH,
		AVGB,
		ABSDB,
		SUMB,
		AND,
		ANDC,
		OR,
		ORC,
		XOR,
		NAND,
		NOR,
		EQV,
		SHLH,
		SHL,
		SHLQBI,
		SHLQBY,
		SHLQBYBI,
		ROTH,
		ROT,
		ROTQBY,
		ROTQBYBI,
		ROTQBI,
		ROTHM,
		ROTM,
		ROTQMBY,
		ROTQMBYBI,
		ROTQMBI,
		ROTMAH,
		ROTMA,
		CEQB,
		CEQH,
		CEQ,
		CGTB,
		CGTH,
		CGT,
		CLGTB,
		CLGTH,
		CLGT,
		FA,
		DFA,
		FS,
		DFS,
		FM,
		DFM,
		FI,
		FCEQ,
		FCMEQ,
		FCGT,
		FCMGT,
	};

	// Enable address-of operator for spu_decoder<>
	friend constexpr flag operator &(flag value)
	{
		return value;
	}
};

#define NAME(x) static constexpr const char& x = *#x

struct spu_iname
{
	NAME(UNK);
	NAME(HEQ);
	NAME(HEQI);
	NAME(HGT);
	NAME(HGTI);
	NAME(HLGT);
	NAME(HLGTI);
	NAME(HBR);
	NAME(HBRA);
	NAME(HBRR);
	NAME(STOP);
	NAME(STOPD);
	NAME(LNOP);
	NAME(NOP);
	NAME(SYNC);
	NAME(DSYNC);
	NAME(MFSPR);
	NAME(MTSPR);
	NAME(RDCH);
	NAME(RCHCNT);
	NAME(WRCH);
	NAME(LQD);
	NAME(LQX);
	NAME(LQA);
	NAME(LQR);
	NAME(STQD);
	NAME(STQX);
	NAME(STQA);
	NAME(STQR);
	NAME(CBD);
	NAME(CBX);
	NAME(CHD);
	NAME(CHX);
	NAME(CWD);
	NAME(CWX);
	NAME(CDD);
	NAME(CDX);
	NAME(ILH);
	NAME(ILHU);
	NAME(IL);
	NAME(ILA);
	NAME(IOHL);
	NAME(FSMBI);
	NAME(AH);
	NAME(AHI);
	NAME(A);
	NAME(AI);
	NAME(SFH);
	NAME(SFHI);
	NAME(SF);
	NAME(SFI);
	NAME(ADDX);
	NAME(CG);
	NAME(CGX);
	NAME(SFX);
	NAME(BG);
	NAME(BGX);
	NAME(MPY);
	NAME(MPYU);
	NAME(MPYI);
	NAME(MPYUI);
	NAME(MPYH);
	NAME(MPYS);
	NAME(MPYHH);
	NAME(MPYHHA);
	NAME(MPYHHU);
	NAME(MPYHHAU);
	NAME(CLZ);
	NAME(CNTB);
	NAME(FSMB);
	NAME(FSMH);
	NAME(FSM);
	NAME(GBB);
	NAME(GBH);
	NAME(GB);
	NAME(AVGB);
	NAME(ABSDB);
	NAME(SUMB);
	NAME(XSBH);
	NAME(XSHW);
	NAME(XSWD);
	NAME(AND);
	NAME(ANDC);
	NAME(ANDBI);
	NAME(ANDHI);
	NAME(ANDI);
	NAME(OR);
	NAME(ORC);
	NAME(ORBI);
	NAME(ORHI);
	NAME(ORI);
	NAME(ORX);
	NAME(XOR);
	NAME(XORBI);
	NAME(XORHI);
	NAME(XORI);
	NAME(NAND);
	NAME(NOR);
	NAME(EQV);
	NAME(MPYA);
	NAME(SELB);
	NAME(SHUFB);
	NAME(SHLH);
	NAME(SHLHI);
	NAME(SHL);
	NAME(SHLI);
	NAME(SHLQBI);
	NAME(SHLQBII);
	NAME(SHLQBY);
	NAME(SHLQBYI);
	NAME(SHLQBYBI);
	NAME(ROTH);
	NAME(ROTHI);
	NAME(ROT);
	NAME(ROTI);
	NAME(ROTQBY);
	NAME(ROTQBYI);
	NAME(ROTQBYBI);
	NAME(ROTQBI);
	NAME(ROTQBII);
	NAME(ROTHM);
	NAME(ROTHMI);
	NAME(ROTM);
	NAME(ROTMI);
	NAME(ROTQMBY);
	NAME(ROTQMBYI);
	NAME(ROTQMBYBI);
	NAME(ROTQMBI);
	NAME(ROTQMBII);
	NAME(ROTMAH);
	NAME(ROTMAHI);
	NAME(ROTMA);
	NAME(ROTMAI);
	NAME(CEQB);
	NAME(CEQBI);
	NAME(CEQH);
	NAME(CEQHI);
	NAME(CEQ);
	NAME(CEQI);
	NAME(CGTB);
	NAME(CGTBI);
	NAME(CGTH);
	NAME(CGTHI);
	NAME(CGT);
	NAME(CGTI);
	NAME(CLGTB);
	NAME(CLGTBI);
	NAME(CLGTH);
	NAME(CLGTHI);
	NAME(CLGT);
	NAME(CLGTI);
	NAME(BR);
	NAME(BRA);
	NAME(BRSL);
	NAME(BRASL);
	NAME(BI);
	NAME(IRET);
	NAME(BISLED);
	NAME(BISL);
	NAME(BRNZ);
	NAME(BRZ);
	NAME(BRHNZ);
	NAME(BRHZ);
	NAME(BIZ);
	NAME(BINZ);
	NAME(BIHZ);
	NAME(BIHNZ);
	NAME(FA);
	NAME(DFA);
	NAME(FS);
	NAME(DFS);
	NAME(FM);
	NAME(DFM);
	NAME(DFMA);
	NAME(DFNMS);
	NAME(DFMS);
	NAME(DFNMA);
	NAME(FREST);
	NAME(FRSQEST);
	NAME(FI);
	NAME(CSFLT);
	NAME(CFLTS);
	NAME(CUFLT);
	NAME(CFLTU);
	NAME(FRDS);
	NAME(FESD);
	NAME(FCEQ);
	NAME(FCMEQ);
	NAME(FCGT);
	NAME(FCMGT);
	NAME(FSCRWR);
	NAME(FSCRRD);
	NAME(DFCEQ);
	NAME(DFCMEQ);
	NAME(DFCGT);
	NAME(DFCMGT);
	NAME(DFTSV);
	NAME(FMA);
	NAME(FNMS);
	NAME(FMS);
};

#undef NAME
