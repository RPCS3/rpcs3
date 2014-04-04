#pragma once
#include "Emu/Cell/PPCThread.h"
#include "Emu/SysCalls/SysCalls.h"
#include "rpcs3.h"
#include <cmath>

enum
{
	XER_SO = 0x80000000,
	XER_OV = 0x40000000,
	XER_CA = 0x20000000,
};

enum
{
	CR_LT = 0x8,
	CR_GT = 0x4,
	CR_EQ = 0x2,
	CR_SO = 0x1,
};

enum
{
	PPU_THREAD_STATUS_IDLE      = (1 << 0),
	PPU_THREAD_STATUS_RUNNABLE  = (1 << 1),
	PPU_THREAD_STATUS_ONPROC    = (1 << 2),
	PPU_THREAD_STATUS_SLEEP     = (1 << 3),
	PPU_THREAD_STATUS_STOP      = (1 << 4),
	PPU_THREAD_STATUS_ZOMBIE    = (1 << 5),
	PPU_THREAD_STATUS_DELETED   = (1 << 6),
	PPU_THREAD_STATUS_UNKNOWN   = (1 << 7),
};

enum FPSCR_EXP
{
	FPSCR_FX        = 0x80000000,
	FPSCR_FEX       = 0x40000000,
	FPSCR_VX        = 0x20000000,
	FPSCR_OX        = 0x10000000,

	FPSCR_UX        = 0x08000000,
	FPSCR_ZX        = 0x04000000,
	FPSCR_XX        = 0x02000000,
	FPSCR_VXSNAN    = 0x01000000,

	FPSCR_VXISI     = 0x00800000,
	FPSCR_VXIDI     = 0x00400000,
	FPSCR_VXZDZ     = 0x00200000,
	FPSCR_VXIMZ     = 0x00100000,

	FPSCR_VXVC      = 0x00080000,
	FPSCR_FR        = 0x00040000,
	FPSCR_FI        = 0x00020000,

	FPSCR_VXSOFT    = 0x00000400,
	FPSCR_VXSQRT    = 0x00000200,
	FPSCR_VXCVI     = 0x00000100,
};

enum FPSCR_RN
{
	FPSCR_RN_NEAR = 0,
	FPSCR_RN_ZERO = 1,
	FPSCR_RN_PINF = 2,
	FPSCR_RN_MINF = 3,
};

static const u64 DOUBLE_SIGN = 0x8000000000000000ULL;
static const u64 DOUBLE_EXP  = 0x7FF0000000000000ULL;
static const u64 DOUBLE_FRAC = 0x000FFFFFFFFFFFFFULL;
static const u64 DOUBLE_ZERO = 0x0000000000000000ULL;

union FPSCRhdr
{
	struct
	{
		u32 RN      :2; //Floating-point rounding control
		u32 NI      :1; //Floating-point non-IEEE mode
		u32 XE      :1; //Floating-point inexact exception enable
		u32 ZE      :1; //IEEE ﬂoating-point zero divide exception enable
		u32 UE      :1; //IEEE ﬂoating-point underﬂow exception enable
		u32 OE      :1; //IEEE ﬂoating-point overﬂow exception enable
		u32 VE      :1; //Floating-point invalid operation exception enable
		u32 VXCVI   :1; //Floating-point invalid operation exception for invalid integer convert
		u32 VXSQRT  :1; //Floating-point invalid operation exception for invalid square root
		u32 VXSOFT  :1; //Floating-point invalid operation exception for software request
		u32         :1; //Reserved
		u32 FPRF    :5; //Floating-point result ﬂags
		u32 FI      :1; //Floating-point fraction inexact
		u32 FR      :1; //Floating-point fraction rounded
		u32 VXVC    :1; //Floating-point invalid operation exception for invalid compare
		u32 VXIMZ   :1; //Floating-point invalid operation exception for * * 0
		u32 VXZDZ   :1; //Floating-point invalid operation exception for 0 / 0
		u32 VXIDI   :1; //Floating-point invalid operation exception for * + *
		u32 VXISI   :1; //Floating-point invalid operation exception for * - *
		u32 VXSNAN  :1; //Floating-point invalid operation exception for SNaN
		u32 XX      :1; //Floating-point inexact exception
		u32 ZX      :1; //Floating-point zero divide exception
		u32 UX      :1; //Floating-point underﬂow exception
		u32 OX      :1; //Floating-point overﬂow exception
		u32 VX      :1; //Floating-point invalid operation exception summary
		u32 FEX     :1; //Floating-point enabled exception summary
		u32 FX      :1; //Floating-point exception summary
	};

	u32 FPSCR;
};

union MSRhdr
{
	struct
	{
		//Little-endian mode enable
		//0      The processor runs in big-endian mode. 
		//1      The processor runs in little-endian mode.
		u64 LE  : 1;

		//Recoverable exception (for system reset and machine check exceptions).
		//0      Exception is not recoverable. 
		//1      Exception is recoverable.
		u64 RI  : 1;

		//Reserved
		u64     : 2;

		//Data address translation   
		//0      Data address translation is disabled. 
		//1      Data address translation is enabled.
		u64 DR  : 1;

		//Instruction address translation   
		//0      Instruction address translation is disabled. 
		//1      Instruction address translation is enabled.
		u64 IR  : 1;

		//Exception preﬁx. The setting of this bit speciﬁes whether an exception vector offset 
		//is prepended with Fs or 0s. In the following description, nnnnn is the offset of the 
		//exception.
		//0      Exceptions are vectored to the physical address 0x0000_0000_000n_nnnn in 64-bit implementations.
		//1      Exceptions are vectored to the physical address 0xFFFF_FFFF_FFFn_nnnn in 64-bit implementations.
		u64 IP  : 1;

		//Reserved
		u64     : 1;
		
		//Floating-point exception mode 1
		u64 FE1 : 1;

		//Branch trace enable (Optional)
		//0      The processor executes branch instructions normally. 
		//1      The processor generates a branch trace exception after completing the 
		//execution of a branch instruction, regardless of whether or not the branch was 
		//taken. 
		//Note: If the function is not implemented, this bit is treated as reserved.
		u64 BE  : 1;

		//Single-step trace enable (Optional)
		//0      The processor executes instructions normally. 
		//1      The processor generates a single-step trace exception upon the successful 
		//execution of the next instruction.
		//Note: If the function is not implemented, this bit is treated as reserved.
		u64 SE  : 1;

		//Floating-point exception mode 0
		u64 FE0 : 1;

		//Machine check enable 
		//0      Machine check exceptions are disabled. 
		//1      Machine check exceptions are enabled.
		u64 ME  : 1;

		//Floating-point available 
		//0      The processor prevents dispatch of ﬂoating-point instructions, including 
		//ﬂoating-point loads, stores, and moves.
		//1      The processor can execute ﬂoating-point instructions.
		u64 FP  : 1;

		//Privilege level 
		//0      The processor can execute both user- and supervisor-level instructions.
		//1      The processor can only execute user-level instructions.
		u64 PR  : 1;

		//External interrupt enable 
		//0      While the bit is cleared the processor delays recognition of external interrupts 
		//and decrementer exception conditions. 
		//1      The processor is enabled to take an external interrupt or the decrementer 
		//exception.
		u64 EE  : 1;

		//Exception little-endian mode. When an exception occurs, this bit is copied into 
		//MSR[LE] to select the endian mode for the context established by the exception
		u64 ILE : 1;

		//Reserved
		u64     : 1;
		
		//Power management enable
		//0      Power management disabled (normal operation mode).
		//1      Power management enabled (reduced power mode).
		//Note: Power management functions are implementation-dependent. If the function 
		//is not implemented, this bit is treated as reserved.
		u64	POW : 1;

		//Reserved
		u64     : 44;
		
		//Sixty-four bit mode
		//0      The 64-bit processor runs in 32-bit mode.
		//1      The 64-bit processor runs in 64-bit mode. Note that this is the default setting.
		u64	SF  : 1;
	};

	u64 MSR;
};

union PVRhdr
{
	struct
	{
		u16 revision;
		u16 version;
	};

	u32 PVR;
};

union CRhdr
{
	u32 CR;

	struct
	{
		u8 cr7  : 4;
		u8 cr6  : 4;
		u8 cr5  : 4;
		u8 cr4  : 4;
		u8 cr3  : 4;
		u8 cr2  : 4;
		u8 cr1  : 4;
		u8 cr0  : 4;
	};
};

union XERhdr
{
	u64 XER;

	struct
	{
		u64 L   : 61;
		u64 CA  : 1;
		u64 OV  : 1;
		u64 SO  : 1;
	};
};

union VSCRhdr
{
	u32 VSCR;

	struct
	{
		/*
		Saturation. A sticky status bit indicating that some field in a saturating instruction saturated since the last
		time SAT was cleared. In other words when SAT = ‘1’ it remains set to ‘1’ until it is cleared to ‘0’ by an
		mtvscr instruction.
		1	The vector saturate instruction implicitly sets when saturation has occurred on the results one of
			the vector instructions having saturate in its name:
			Move To VSCR (mtvscr)
			Vector Add Integer with Saturation (vaddubs, vadduhs, vadduws, vaddsbs, vaddshs,
			vaddsws)
			Vector Subtract Integer with Saturation (vsububs, vsubuhs, vsubuws, vsubsbs, vsubshs,
			vsubsws)
			Vector Multiply-Add Integer with Saturation (vmhaddshs, vmhraddshs)
			Vector Multiply-Sum with Saturation (vmsumuhs, vmsumshs, vsumsws)
			Vector Sum-Across with Saturation (vsumsws, vsum2sws, vsum4sbs, vsum4shs,
			vsum4ubs)
			Vector Pack with Saturation (vpkuhus, vpkuwus, vpkshus, vpkswus, vpkshss, vpkswss)
			Vector Convert to Fixed-Point with Saturation (vctuxs, vctsxs)
		0	Indicates no saturation occurred; mtvscr can explicitly clear this bit.
		*/
		u32 SAT : 1;
		u32 X   : 15;

		/*
		Non-Java. A mode control bit that determines whether vector floating-point operations will be performed
		in a Java-IEEE-C9X–compliant mode or a possibly faster non-Java/non-IEEE mode.
		0	The Java-IEEE-C9X–compliant mode is selected. Denormalized values are handled as specified
			by Java, IEEE, and C9X standard.
		1	The non-Java/non-IEEE–compliant mode is selected. If an element in a source vector register
			contains a denormalized value, the value ‘0’ is used instead. If an instruction causes an underflow
			exception, the corresponding element in the target VR is cleared to ‘0’. In both cases, the ‘0’
			has the same sign as the denormalized or underflowing value.
		*/
		u32 NJ  : 1;
		u32 Y   : 15;
	};
};

enum FPRType
{
	//FPR_NORM,
	//FPR_ZERO,
	//FPR_SNAN,
	//FPR_QNAN,
	//FPR_INF,
	FPR_PZ   = 0x2,
	FPR_PN   = 0x4,
	FPR_PINF = 0x5,
	FPR_NN   = 0x8,
	FPR_NINF = 0x9,
	FPR_QNAN = 0x11,
	FPR_NZ   = 0x12,
	FPR_PD   = 0x14,
	FPR_ND   = 0x18,
};

static const u64 FPR_NAN_I = 0x7FF8000000000000ULL;
static const double& FPR_NAN = (double&)FPR_NAN_I;

struct PPCdouble
{
	union
	{
		double _double;
		u64 _u64;

		struct
		{
			u64 frac  : 52;
			u64 exp   : 11;
			u64 sign  : 1;
		};

		struct
		{
			u64     : 51;
			u64 nan : 1;
			u64     : 12;
		};
	};

	FPRType type;
	
	operator double&() { return _double; }
	operator const double&() const { return _double; }

	PPCdouble& operator = (const PPCdouble& r)
	{
		_u64 = r._u64;
		type = UpdateType();
		return *this;
	}

	FPRType UpdateType() const
	{
		const int fpc = _fpclass(_double);

#ifndef __GNUG__
		switch(fpc)
		{
		case _FPCLASS_SNAN://		return FPR_SNAN;
		case _FPCLASS_QNAN:  return FPR_QNAN;
		case _FPCLASS_NINF:  return FPR_NINF;
		case _FPCLASS_NN:    return FPR_NN;
		case _FPCLASS_ND:    return FPR_ND;
		case _FPCLASS_NZ:    return FPR_NZ;
		case _FPCLASS_PZ:    return FPR_PZ;
		case _FPCLASS_PD:    return FPR_PD;
		case _FPCLASS_PN:    return FPR_PN;
		case _FPCLASS_PINF:  return FPR_PINF;
		}
#else
		switch (fpc)
		{
		case FP_NAN:        return FPR_QNAN;
		case FP_INFINITE:   return std::signbit(_double) ? FPR_NINF : FPR_PINF;
		case FP_SUBNORMAL:  return std::signbit(_double) ? FPR_ND : FPR_PD;
		case FP_ZERO:       return std::signbit(_double) ? FPR_NZ : FPR_PZ;
		default:            return std::signbit(_double) ? FPR_NN : FPR_PN;
		}
#endif

		throw fmt::Format("PPCdouble::UpdateType() -> unknown fpclass (0x%04x).", fpc);
	}

	FPRType GetType() const
	{
		return type;
	}

	u32 To32() const
	{
		float res = _double;

		return (u32&)res;
	}

	u64 To64() const
	{
		return (u64&)_double;
	}

	u32 GetZerosCount() const
	{
		u32 ret;
		u32 dd = frac >> 32;
		if(dd)
		{
			ret = 31;
		}
		else
		{
			dd = frac;
			ret = 63;
		}

		if(dd > 0xffff)
		{
			ret -= 16;
			dd >>= 16;
		}
		if(dd > 0xff)
		{
			ret -= 8;
			dd >>= 8;
		}
		if(dd & 0xf0)
		{
			ret -= 4;
			dd >>= 4;
		}
		if(dd & 0xc)
		{
			ret -= 2;
			dd >>= 2;
		}
		if(dd & 0x2) ret--;

		return ret;
	}

	PPCdouble() : _u64(0)
	{
	}

	PPCdouble(double val) : _double(val)
	{
	}

	PPCdouble(u64 val) : _u64(val)
	{
	}

	PPCdouble(u32 val) : _u64(val)
	{
	}
};

struct FPRdouble
{
	static const u64 double_sign = 0x8000000000000000ULL;
	static const u64 double_frac = 0x000FFFFFFFFFFFFFULL;

	static bool IsINF(PPCdouble d);
	static bool IsNaN(PPCdouble d);
	static bool IsQNaN(PPCdouble d);
	static bool IsSNaN(PPCdouble d);

	static int Cmp(PPCdouble a, PPCdouble b);
};

union VPR_reg
{
	//__m128i _m128i;
	u128 _u128;
	s128 _s128;
	u64 _u64[2];
	s64 _s64[2];
	u32 _u32[4];
	s32 _s32[4];
	u16 _u16[8];
	s16 _s16[8];
	u8  _u8[16];
	s8  _s8[16];
	float _f[4];
	double _d[2];

	VPR_reg() { Clear(); }

	std::string ToString(bool hex = false) const
	{
		if(hex) return fmt::Format("%08x%08x%08x%08x", _u32[3], _u32[2], _u32[1], _u32[0]);

		return fmt::Format("x: %g y: %g z: %g w: %g", _f[3], _f[2], _f[1], _f[0]);
	}

	u8 GetBit(u8 bit)
	{
		if(bit < 64) return (_u64[0] >> bit) & 0x1;

		return (_u64[1] >> (bit - 64)) & 0x1;
	}

	void SetBit(u8 bit, u8 value)
	{
		if(bit < 64)
		{
			_u64[0] &= ~(1 << bit);
			_u64[0] |= (value & 0x1) << bit;

			return;
		}

		bit -= 64;

		_u64[1] &= ~(1 << bit);
		_u64[1] |= (value & 0x1) << bit;
	}

	void Clear() { memset(this, 0, sizeof(*this)); }
};

static const s32 MAX_INT_VALUE = 0x7fffffff;

class PPUThread : public PPCThread
{
public:
	u32 owned_mutexes;

public:
	PPCdouble FPR[32]; //Floating Point Register
	FPSCRhdr FPSCR; //Floating Point Status and Control Register
	u64 GPR[32]; //General-Purpose Register
	VPR_reg VPR[32];
	u32 vpcr;

	CRhdr CR; //Condition Register
	//CR0
	// 0 : LT - Negative (is negative)
	// : 0 - Result is not negative
	// : 1 - Result is negative
	// 1 : GT - Positive (is positive)
	// : 0 - Result is not positive
	// : 1 - Result is positive
	// 2 : EQ - Zero (is zero)
	// : 0 - Result is not equal to zero
	// : 1 - Result is equal to zero
	// 3 : SO - Summary overflow (copy of the final state XER[S0])
	// : 0 - No overflow occurred
	// : 1 - Overflow occurred

	//CRn 
	// 0 : LT - Less than (rA > X)
	// : 0 - rA is not less than
	// : 1 - rA is less than
	// 1 : GT - Greater than (rA < X)
	// : 0 - rA is not greater than
	// : 1 - rA is greater than
	// 2 : EQ - Equal to (rA == X)
	// : 0 - Result is not equal to zero
	// : 1 - Result is equal to zero
	// 3 : SO - Summary overflow (copy of the final state XER[S0])
	// : 0 - No overflow occurred
	// : 1 - Overflow occurred

	//SPR : Special-Purpose Registers

	XERhdr XER; //SPR 0x001 : Fixed-Point Expection Register
	// 0 : SO - Summary overflow
	// : 0 - No overflow occurred
	// : 1 - Overflow occurred
	// 1 : OV - Overflow
	// : 0 - No overflow occurred
	// : 1 - Overflow occurred
	// 2 : CA - Carry
	// : 0 - Carry did not occur
	// : 1 - Carry occured
	// 3 - 24 : Reserved
	// 25 - 31 : TBC
	// Transfer-byte count

	MSRhdr MSR; //Machine State Register
	PVRhdr PVR; //Processor Version Register

	VSCRhdr VSCR; // Vector Status and Control Register

	u64 LR;     //SPR 0x008 : Link Register
	u64 CTR;    //SPR 0x009 : Count Register

	union
	{
		u64 USPRG0;	//SPR 0x100 : User-SPR General-Purpose Register 0
		u64 SPRG[8]; //SPR 0x100 - 0x107 : SPR General-Purpose Registers
	};

	//TBR : Time-Base Registers
	union
	{
		u64 TB;	//TBR 0x10C - 0x10D

		struct
		{
			u32 TBH;
			u32 TBL;
		};
	};

	u64 cycle;

public:
	PPUThread();
	virtual ~PPUThread();

	inline u8 GetCR(const u8 n) const
	{
		switch(n)
		{
		case 0: return CR.cr0;
		case 1: return CR.cr1;
		case 2: return CR.cr2;
		case 3: return CR.cr3;
		case 4: return CR.cr4;
		case 5: return CR.cr5;
		case 6: return CR.cr6;
		case 7: return CR.cr7;
		}

		return 0;
	}

	inline void SetCR(const u8 n, const u32 value)
	{
		switch(n)
		{
		case 0: CR.cr0 = value; break;
		case 1: CR.cr1 = value; break;
		case 2: CR.cr2 = value; break;
		case 3: CR.cr3 = value; break;
		case 4: CR.cr4 = value; break;
		case 5: CR.cr5 = value; break;
		case 6: CR.cr6 = value; break;
		case 7: CR.cr7 = value; break;
		}
	}

	inline void SetCRBit(const u8 n, const u32 bit, const bool value)
	{
		switch(n)
		{
		case 0: value ? CR.cr0 |= bit : CR.cr0 &= ~bit; break;
		case 1: value ? CR.cr1 |= bit : CR.cr1 &= ~bit; break;
		case 2: value ? CR.cr2 |= bit : CR.cr2 &= ~bit; break;
		case 3: value ? CR.cr3 |= bit : CR.cr3 &= ~bit; break;
		case 4: value ? CR.cr4 |= bit : CR.cr4 &= ~bit; break;
		case 5: value ? CR.cr5 |= bit : CR.cr5 &= ~bit; break;
		case 6: value ? CR.cr6 |= bit : CR.cr6 &= ~bit; break;
		case 7: value ? CR.cr7 |= bit : CR.cr7 &= ~bit; break;
		}
	}

	inline void SetCR_EQ(const u8 n, const bool value) { SetCRBit(n, CR_EQ, value); }
	inline void SetCR_GT(const u8 n, const bool value) { SetCRBit(n, CR_GT, value); }
	inline void SetCR_LT(const u8 n, const bool value) { SetCRBit(n, CR_LT, value); }	
	inline void SetCR_SO(const u8 n, const bool value) { SetCRBit(n, CR_SO, value); }

	inline bool IsCR_EQ(const u8 n) const { return (GetCR(n) & CR_EQ) ? 1 : 0; }
	inline bool IsCR_GT(const u8 n) const { return (GetCR(n) & CR_GT) ? 1 : 0; }
	inline bool IsCR_LT(const u8 n) const { return (GetCR(n) & CR_LT) ? 1 : 0; }

	template<typename T> void UpdateCRn(const u8 n, const T a, const T b)
	{
		if      (a <  b) SetCR(n, CR_LT);
		else if (a >  b) SetCR(n, CR_GT);
		else if (a == b) SetCR(n, CR_EQ);

		SetCR_SO(n, XER.SO);
	}

	void UpdateCRnU(const u8 l, const u8 n, const u64 a, const u64 b)
	{
		if(l)
		{
			UpdateCRn<u64>(n, a, b);
		}
		else
		{
			UpdateCRn<u32>(n, a, b);
		}
	}

	void UpdateCRnS(const u8 l, const u8 n, const u64 a, const u64 b)
	{
		if(l)
		{
			UpdateCRn<s64>(n, a, b);
		}
		else
		{
			UpdateCRn<s32>(n, a, b);
		}
	}

	template<typename T> void UpdateCR0(const T val)
	{
		UpdateCRn<T>(0, val, 0);
	}

	template<typename T> void UpdateCR1()
	{
		SetCR_LT(1, FPSCR.FX);
		SetCR_GT(1, FPSCR.FEX);
		SetCR_EQ(1, FPSCR.VX);
		SetCR_SO(1, FPSCR.OX);
	}

	const u8 GetCRBit(const u32 bit) const { return 1 << (3 - (bit % 4)); }

	void SetCRBit (const u32 bit, bool set) { SetCRBit(bit >> 2, GetCRBit(bit), set); }
	void SetCRBit2(const u32 bit, bool set) { SetCRBit(bit >> 2, 0x8 >> (bit & 3), set); }

	const u8 IsCR(const u32 bit) const { return (GetCR(bit >> 2) & GetCRBit(bit)) ? 1 : 0; }

	bool IsCarry(const u64 a, const u64 b) { return a > (a + b); }

	void SetFPSCRException(const FPSCR_EXP mask)
	{
		if ((FPSCR.FPSCR & mask) != mask) FPSCR.FX = 1;
		FPSCR.FPSCR |= mask;
	}

	void SetFPSCR_FI(const u32 val)
	{
		if(val) SetFPSCRException(FPSCR_XX);
		FPSCR.FI = val;
	}

	virtual std::string RegsToString()
	{
		std::string ret = "Registers:\n=========\n";

		for(uint i=0; i<32; ++i) ret += fmt::Format("GPR[%d] = 0x%llx\n", i, GPR[i]);
		for(uint i=0; i<32; ++i) ret += fmt::Format("FPR[%d] = %.6G\n", i, (double)FPR[i]);
		for(uint i=0; i<32; ++i) ret += fmt::Format("VPR[%d] = 0x%s [%s]\n", i, (const char*)VPR[i].ToString(true).c_str(), (const char*)VPR[i].ToString().c_str());
		ret += fmt::Format("CR = 0x%08x\n", CR.CR);
		ret += fmt::Format("LR = 0x%llx\n", LR);
		ret += fmt::Format("CTR = 0x%llx\n", CTR);
		ret += fmt::Format("XER = 0x%llx [CA=%lld | OV=%lld | SO=%lld]\n", XER.XER, fmt::by_value(XER.CA), fmt::by_value(XER.OV), fmt::by_value(XER.SO));
		ret += fmt::Format("FPSCR = 0x%x "
			"[RN=%d | NI=%d | XE=%d | ZE=%d | UE=%d | OE=%d | VE=%d | "
			"VXCVI=%d | VXSQRT=%d | VXSOFT=%d | FPRF=%d | "
			"FI=%d | FR=%d | VXVC=%d | VXIMZ=%d | "
			"VXZDZ=%d | VXIDI=%d | VXISI=%d | VXSNAN=%d | "
			"XX=%d | ZX=%d | UX=%d | OX=%d | VX=%d | FEX=%d | FX=%d]\n",
			FPSCR.FPSCR,
			fmt::by_value(FPSCR.RN),
			fmt::by_value(FPSCR.NI), fmt::by_value(FPSCR.XE), fmt::by_value(FPSCR.ZE), fmt::by_value(FPSCR.UE), fmt::by_value(FPSCR.OE), fmt::by_value(FPSCR.VE),
			fmt::by_value(FPSCR.VXCVI), fmt::by_value(FPSCR.VXSQRT), fmt::by_value(FPSCR.VXSOFT), fmt::by_value(FPSCR.FPRF),
			fmt::by_value(FPSCR.FI), fmt::by_value(FPSCR.FR), fmt::by_value(FPSCR.VXVC), fmt::by_value(FPSCR.VXIMZ),
			fmt::by_value(FPSCR.VXZDZ), fmt::by_value(FPSCR.VXIDI), fmt::by_value(FPSCR.VXISI), fmt::by_value(FPSCR.VXSNAN),
			fmt::by_value(FPSCR.XX), fmt::by_value(FPSCR.ZX), fmt::by_value(FPSCR.UX), fmt::by_value(FPSCR.OX), fmt::by_value(FPSCR.VX), fmt::by_value(FPSCR.FEX), fmt::by_value(FPSCR.FX));

		return ret;
	}

	virtual std::string ReadRegString(const std::string& reg)
	{
		std::string::size_type first_brk = reg.find('[');
		if (first_brk != std::string::npos)
		{
			long reg_index = atol(reg.substr(first_brk+1,reg.length()-first_brk-2).c_str());
			if (reg.find("GPR")==0) return fmt::Format("%016llx", GPR[reg_index]);
			if (reg.find("FPR")==0) return fmt::Format("%016llx", (double)FPR[reg_index]);
			if (reg.find("VPR")==0) return fmt::Format("%016llx%016llx", VPR[reg_index]._u64[1], VPR[reg_index]._u64[0]);
		}
		if (reg == "CR")    return fmt::Format("%08x", CR.CR);
		if (reg == "LR")    return fmt::Format("%016llx", LR);
		if (reg == "CTR")   return fmt::Format("%016llx", CTR);
		if (reg == "XER")   return fmt::Format("%016llx", XER.XER);
		if (reg == "FPSCR") return fmt::Format("%08x", FPSCR.FPSCR);

		return "";
	}

	bool WriteRegString(const std::string& reg, std::string value) {
		while (value.length() < 32) value = "0"+value;
		std::string::size_type first_brk = reg.find('[');
		try
		{
			if (first_brk != std::string::npos)
			{
				long reg_index = atol(reg.substr(first_brk + 1, reg.length() - first_brk - 2).c_str());
				if (reg.find("GPR")==0 || reg.find("FPR")==0 )
				{
					unsigned long long reg_value;
					reg_value = std::stoull(value.substr(16, 31),0,16);
					if (reg.find("GPR")==0) GPR[reg_index] = (u64)reg_value;
					if (reg.find("FPR")==0) FPR[reg_index] = (u64)reg_value;
					return true;
				}
				if (reg.find("VPR")==0)
				{
					unsigned long long reg_value0;
					unsigned long long reg_value1;
					reg_value0 = std::stoull(value.substr(16, 31), 0, 16);
					reg_value1 = std::stoull(value.substr(0, 15), 0, 16);
					VPR[reg_index]._u64[0] = (u64)reg_value0;
					VPR[reg_index]._u64[1] = (u64)reg_value1;
					return true;
				}
			}
			if (reg == "LR" || reg == "CTR" || reg == "XER")
			{
				unsigned long long reg_value;
				reg_value = std::stoull(value.substr(16, 31), 0, 16);
				if (reg == "LR") LR = (u64)reg_value;
				if (reg == "CTR") CTR = (u64)reg_value;
				if (reg == "XER") XER.XER = (u64)reg_value;
				return true;
			}
			if (reg == "CR" || reg == "FPSCR")
			{
				unsigned long reg_value;
				reg_value = std::stoull(value.substr(24, 31), 0, 16);
				if (reg == "CR") CR.CR = (u32)reg_value;
				if (reg == "FPSCR") FPSCR.FPSCR = (u32)reg_value;
				return true;
			}
		}
		catch (std::invalid_argument& e)//if any of the stoull conversion fail
		{
			return false;
		}
		return false;
	}

	virtual void AddArgv(const std::string& arg) override;

public:
	virtual void InitRegs(); 
	virtual u64 GetFreeStackSize() const;

protected:
	virtual void DoReset() override;
	virtual void DoRun() override;
	virtual void DoPause() override;
	virtual void DoResume() override;
	virtual void DoStop() override;

protected:
	virtual void Step() override
	{
		//if(++cycle > 20)
		{
			TB++;
			//cycle = 0;
		}
	}
};

PPUThread& GetCurrentPPUThread();
