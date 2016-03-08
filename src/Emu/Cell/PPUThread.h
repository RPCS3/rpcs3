#pragma once

#include "Emu/Cell/Common.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Memory/vm.h"

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

	FPSCR_VX_ALL    = FPSCR_VXSNAN | FPSCR_VXISI | FPSCR_VXIDI | FPSCR_VXZDZ | FPSCR_VXIMZ | FPSCR_VXVC | FPSCR_VXSOFT | FPSCR_VXSQRT | FPSCR_VXCVI,
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
		u32 ZE      :1; //IEEE floating-point zero divide exception enable
		u32 UE      :1; //IEEE floating-point underflow exception enable
		u32 OE      :1; //IEEE floating-point overflow exception enable
		u32 VE      :1; //Floating-point invalid operation exception enable
		u32 VXCVI   :1; //Floating-point invalid operation exception for invalid integer convert
		u32 VXSQRT  :1; //Floating-point invalid operation exception for invalid square root
		u32 VXSOFT  :1; //Floating-point invalid operation exception for software request
		u32         :1; //Reserved
		u32 FPRF    :5; //Floating-point result flags
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
		u32 UX      :1; //Floating-point underflow exception
		u32 OX      :1; //Floating-point overflow exception
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

		//Exception prefix. The setting of this bit specifies whether an exception vector offset 
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
		//0      The processor prevents dispatch of floating-point instructions, including 
		//floating-point loads, stores, and moves.
		//1      The processor can execute floating-point instructions.
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
		u32 L  : 29;
		u32 CA : 1;
		u32 OV : 1;
		u32 SO : 1;
		u32    : 32;
	};
};

union VSCRhdr
{
	u32 VSCR;

	struct
	{
		/*
		Saturation. A sticky status bit indicating that some field in a saturating instruction saturated since the last
		time SAT was cleared. In other words when SAT = '1' it remains set to '1' until it is cleared to '0' by an
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
		in a Java-IEEE-C9X-compliant mode or a possibly faster non-Java/non-IEEE mode.
		0	The Java-IEEE-C9X-compliant mode is selected. Denormalized values are handled as specified
			by Java, IEEE, and C9X standard.
		1	The non-Java/non-IEEE-compliant mode is selected. If an element in a source vector register
			contains a denormalized value, the value '0' is used instead. If an instruction causes an underflow
			exception, the corresponding element in the target VR is cleared to '0'. In both cases, the '0'
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
		default: throw EXCEPTION("Unknown fpclass (0x%04x)", fpc);
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
	}

	FPRType GetType() const
	{
		return type;
	}

	u32 To32() const
	{
		float res = (float)_double;

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
		type = UpdateType();
	}

	PPCdouble(double val) : _double(val)
	{
		type = UpdateType();
	}

	PPCdouble(u64 val) : _u64(val)
	{
		type = UpdateType();
	}

	PPCdouble(u32 val) : _u64(val)
	{
		type = UpdateType();
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

class PPUThread final : public CPUThread
{
public:
	PPCdouble FPR[32]{}; //Floating Point Register
	FPSCRhdr FPSCR{}; //Floating Point Status and Control Register
	u64 GPR[32]{}; //General-Purpose Register
	v128 VPR[32]{};
	u32 vpcr = 0;

	CRhdr CR{}; //Condition Register
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

	XERhdr XER{}; //SPR 0x001 : Fixed-Point Expection Register
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

	MSRhdr MSR{}; //Machine State Register
	PVRhdr PVR{}; //Processor Version Register

	VSCRhdr VSCR{}; // Vector Status and Control Register

	u64 LR = 0;     //SPR 0x008 : Link Register
	u64 CTR = 0;    //SPR 0x009 : Count Register

	u32 VRSAVE = 0; //SPR 0x100: VR Save/Restore Register (32 bits)

	u64 SPRG[8]{}; //SPR 0x110 - 0x117 : SPR General-Purpose Registers

	//TBR : Time-Base Registers
	u64 TB = 0; //TBR 0x10C - 0x10D

	u32 PC = 0;
	s32 prio = 0; // thread priority
	u32 stack_addr = 0; // stack address
	u32 stack_size = 0; // stack size
	bool is_joinable = true;
	bool is_joining = false;

	u64 hle_code = 0; // current syscall (~0..~1023) or function id (1..UINT32_MAX)

	std::function<void(PPUThread& CPU)> custom_task;

	// When a thread has met an exception, this variable is used to retro propagate it through stack call.
	std::exception_ptr pending_exception;

public:
	PPUThread(const std::string& name);
	virtual ~PPUThread() override;

	virtual std::string get_name() const override;
	virtual void dump_info() const override;
	virtual u32 get_pc() const override { return PC; }
	virtual u32 get_offset() const override { return 0; }
	virtual void do_run() override;
	virtual void cpu_task() override;

	virtual void init_regs() override;
	virtual void init_stack() override;
	virtual void close_stack() override;

	virtual bool handle_interrupt() override;

	u8 GetCR(const u8 n) const
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

	void SetCR(const u8 n, const u32 value)
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

	void SetCRBit(const u8 n, const u32 bit, const bool value)
	{
		switch(n)
		{
		case 0: CR.cr0 = (value ? CR.cr0 | bit : CR.cr0 & ~bit); break;
		case 1: CR.cr1 = (value ? CR.cr1 | bit : CR.cr1 & ~bit); break;
		case 2: CR.cr2 = (value ? CR.cr2 | bit : CR.cr2 & ~bit); break;
		case 3: CR.cr3 = (value ? CR.cr3 | bit : CR.cr3 & ~bit); break;
		case 4: CR.cr4 = (value ? CR.cr4 | bit : CR.cr4 & ~bit); break;
		case 5: CR.cr5 = (value ? CR.cr5 | bit : CR.cr5 & ~bit); break;
		case 6: CR.cr6 = (value ? CR.cr6 | bit : CR.cr6 & ~bit); break;
		case 7: CR.cr7 = (value ? CR.cr7 | bit : CR.cr7 & ~bit); break;
		}
	}

	void SetCR_EQ(const u8 n, const bool value) { SetCRBit(n, CR_EQ, value); }
	void SetCR_GT(const u8 n, const bool value) { SetCRBit(n, CR_GT, value); }
	void SetCR_LT(const u8 n, const bool value) { SetCRBit(n, CR_LT, value); }	
	void SetCR_SO(const u8 n, const bool value) { SetCRBit(n, CR_SO, value); }

	bool IsCR_EQ(const u8 n) const { return (GetCR(n) & CR_EQ) ? 1 : 0; }
	bool IsCR_GT(const u8 n) const { return (GetCR(n) & CR_GT) ? 1 : 0; }
	bool IsCR_LT(const u8 n) const { return (GetCR(n) & CR_LT) ? 1 : 0; }

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
			UpdateCRn<u32>(n, (u32)a, (u32)b);
		}
	}

	void UpdateCRnS(const u8 l, const u8 n, const u64 a, const u64 b)
	{
		if(l)
		{
			UpdateCRn<s64>(n, (s64)a, (s64)b);
		}
		else
		{
			UpdateCRn<s32>(n, (s32)a, (s32)b);
		}
	}

	template<typename T> void UpdateCR0(const T val)
	{
		UpdateCRn<T>(0, val, 0);
	}

	void UpdateCR1()
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

	bool IsCarry(const u64 a, const u64 b) { return (a + b) < a; }
	bool IsCarry(const u64 a, const u64 b, const u64 c) { return IsCarry(a, b) || IsCarry(a + b, c); }

	void SetOV(const bool set)
	{
		XER.OV = set;
		XER.SO |= set;
	}

	void UpdateFPSCR_FEX()
	{
		const u32 exceptions = (FPSCR.FPSCR >> 25) & 0x1F;
		const u32 enabled = (FPSCR.FPSCR >> 3) & 0x1F;
		if (exceptions & enabled) FPSCR.FEX = 1;
	}

	void UpdateFPSCR_VX()
	{
		if (FPSCR.FPSCR & FPSCR_VX_ALL) FPSCR.VX = 1;
	}

	void SetFPSCR(const u32 val)
	{
		FPSCR.FPSCR = val & ~(FPSCR_FEX | FPSCR_VX);
		UpdateFPSCR_VX();
		UpdateFPSCR_FEX();
	}

	void SetFPSCRException(const FPSCR_EXP mask)
	{
		if ((FPSCR.FPSCR & mask) != mask) FPSCR.FX = 1;
		FPSCR.FPSCR |= mask;
		UpdateFPSCR_VX();
		UpdateFPSCR_FEX();
	}

	void SetFPSCR_FI(const u32 val)
	{
		if(val) SetFPSCRException(FPSCR_XX);
		FPSCR.FI = val;
	}

	virtual std::string RegsToString() const override
	{
		std::string ret = "Registers:\n=========\n";

		for(uint i=0; i<32; ++i) ret += fmt::format("GPR[%d] = 0x%llx\n", i, GPR[i]);
		for(uint i=0; i<32; ++i) ret += fmt::format("FPR[%d] = %.6G\n", i, (double)FPR[i]);
		for(uint i=0; i<32; ++i) ret += fmt::format("VPR[%d] = 0x%s [%s]\n", i, VPR[i].to_hex().c_str(), VPR[i].to_xyzw().c_str());
		ret += fmt::format("CR = 0x%08x\n", CR.CR);
		ret += fmt::format("LR = 0x%llx\n", LR);
		ret += fmt::format("CTR = 0x%llx\n", CTR);
		ret += fmt::format("XER = 0x%llx [CA=%lld | OV=%lld | SO=%lld]\n", XER.XER, u32{ XER.CA }, u32{ XER.OV }, u32{ XER.SO });
		ret += fmt::format("FPSCR = 0x%x "
			"[RN=%d | NI=%d | XE=%d | ZE=%d | UE=%d | OE=%d | VE=%d | "
			"VXCVI=%d | VXSQRT=%d | VXSOFT=%d | FPRF=%d | "
			"FI=%d | FR=%d | VXVC=%d | VXIMZ=%d | "
			"VXZDZ=%d | VXIDI=%d | VXISI=%d | VXSNAN=%d | "
			"XX=%d | ZX=%d | UX=%d | OX=%d | VX=%d | FEX=%d | FX=%d]\n",
			FPSCR.FPSCR,
			u32{ FPSCR.RN },
			u32{ FPSCR.NI }, u32{ FPSCR.XE }, u32{ FPSCR.ZE }, u32{ FPSCR.UE }, u32{ FPSCR.OE }, u32{ FPSCR.VE },
			u32{ FPSCR.VXCVI }, u32{ FPSCR.VXSQRT }, u32{ FPSCR.VXSOFT }, u32{ FPSCR.FPRF },
			u32{ FPSCR.FI }, u32{ FPSCR.FR }, u32{ FPSCR.VXVC }, u32{ FPSCR.VXIMZ },
			u32{ FPSCR.VXZDZ }, u32{ FPSCR.VXIDI }, u32{ FPSCR.VXISI }, u32{ FPSCR.VXSNAN },
			u32{ FPSCR.XX }, u32{ FPSCR.ZX }, u32{ FPSCR.UX }, u32{ FPSCR.OX }, u32{ FPSCR.VX }, u32{ FPSCR.FEX }, u32{ FPSCR.FX });

		return ret;
	}

	virtual std::string ReadRegString(const std::string& reg) const override
	{
		std::string::size_type first_brk = reg.find('[');
		if (first_brk != std::string::npos)
		{
			long reg_index = atol(reg.substr(first_brk+1,reg.length()-first_brk-2).c_str());
			if (reg.find("GPR")==0) return fmt::format("%016llx", GPR[reg_index]);
			if (reg.find("FPR")==0) return fmt::format("%016llx", (double)FPR[reg_index]);
			if (reg.find("VPR")==0) return fmt::format("%016llx%016llx", VPR[reg_index]._u64[1], VPR[reg_index]._u64[0]);
		}
		if (reg == "CR")    return fmt::format("%08x", CR.CR);
		if (reg == "LR")    return fmt::format("%016llx", LR);
		if (reg == "CTR")   return fmt::format("%016llx", CTR);
		if (reg == "XER")   return fmt::format("%016llx", XER.XER);
		if (reg == "FPSCR") return fmt::format("%08x", FPSCR.FPSCR);

		return "";
	}

	bool WriteRegString(const std::string& reg, std::string value) override
	{
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
				unsigned long long reg_value;
				reg_value = std::stoull(value.substr(24, 31), 0, 16);
				if (reg == "CR") CR.CR = (u32)reg_value;
				if (reg == "FPSCR") FPSCR.FPSCR = (u32)reg_value;
				return true;
			}
		}
		catch (std::invalid_argument&)//if any of the stoull conversion fail
		{
			return false;
		}
		return false;
	}

	u64 get_next_gpr_arg(u32& g_count, u32& f_count, u32& v_count)
	{
		assert(!f_count && !v_count); // not supported

		if (g_count < 8)
		{
			return GPR[g_count++ + 3];
		}
		else
		{
			return get_stack_arg(++g_count);
		}
	}

public:
	u64 get_stack_arg(s32 i);
	void fast_call(u32 addr, u32 rtoc);
	void fast_stop();
};

class ppu_thread : cpu_thread
{
	static const u32 stack_align = 0x10;
	vm::_ptr_base<be_t<u64>> argv;
	u32 argc;
	vm::_ptr_base<be_t<u64>> envp;

public:
	ppu_thread(u32 entry, const std::string& name = "", u32 stack_size = 0, s32 prio = 0);

	cpu_thread& args(std::initializer_list<std::string> values) override;
	cpu_thread& run() override;
	ppu_thread& gpr(uint index, u64 value);
};

template<typename T, bool is_enum = std::is_enum<T>::value>
struct cast_ppu_gpr
{
	static_assert(is_enum, "Invalid type for cast_ppu_gpr");

	force_inline static u64 to_gpr(const T& value)
	{
		return cast_ppu_gpr<std::underlying_type_t<T>>::to_gpr(static_cast<std::underlying_type_t<T>>(value));
	}

	force_inline static T from_gpr(const u64 reg)
	{
		return static_cast<T>(cast_ppu_gpr<std::underlying_type_t<T>>::from_gpr(reg));
	}
};

template<>
struct cast_ppu_gpr<u8, false>
{
	force_inline static u64 to_gpr(const u8& value)
	{
		return value;
	}

	force_inline static u8 from_gpr(const u64 reg)
	{
		return static_cast<u8>(reg);
	}
};

template<>
struct cast_ppu_gpr<u16, false>
{
	force_inline static u64 to_gpr(const u16& value)
	{
		return value;
	}

	force_inline static u16 from_gpr(const u64 reg)
	{
		return static_cast<u16>(reg);
	}
};

template<>
struct cast_ppu_gpr<u32, false>
{
	force_inline static u64 to_gpr(const u32& value)
	{
		return value;
	}

	force_inline static u32 from_gpr(const u64 reg)
	{
		return static_cast<u32>(reg);
	}
};

#ifdef __APPLE__
template<>
struct cast_ppu_gpr<unsigned long, false>
{
	force_inline static u64 to_gpr(const unsigned long& value)
	{
		return value;
	}

	force_inline static unsigned long from_gpr(const u64 reg)
	{
		return static_cast<unsigned long>(reg);
	}
};
#endif

template<>
struct cast_ppu_gpr<u64, false>
{
	force_inline static u64 to_gpr(const u64& value)
	{
		return value;
	}

	force_inline static u64 from_gpr(const u64 reg)
	{
		return reg;
	}
};

template<>
struct cast_ppu_gpr<s8, false>
{
	force_inline static u64 to_gpr(const s8& value)
	{
		return value;
	}

	force_inline static s8 from_gpr(const u64 reg)
	{
		return static_cast<s8>(reg);
	}
};

template<>
struct cast_ppu_gpr<s16, false>
{
	force_inline static u64 to_gpr(const s16& value)
	{
		return value;
	}

	force_inline static s16 from_gpr(const u64 reg)
	{
		return static_cast<s16>(reg);
	}
};

template<>
struct cast_ppu_gpr<s32, false>
{
	force_inline static u64 to_gpr(const s32& value)
	{
		return value;
	}

	force_inline static s32 from_gpr(const u64 reg)
	{
		return static_cast<s32>(reg);
	}
};

template<>
struct cast_ppu_gpr<s64, false>
{
	force_inline static u64 to_gpr(const s64& value)
	{
		return value;
	}

	force_inline static s64 from_gpr(const u64 reg)
	{
		return static_cast<s64>(reg);
	}
};

template<>
struct cast_ppu_gpr<b8, false>
{
	force_inline static u64 to_gpr(const b8& value)
	{
		return value;
	}

	force_inline static b8 from_gpr(const u64& reg)
	{
		return static_cast<u32>(reg) != 0;
	}
};

template<typename T>
force_inline u64 cast_to_ppu_gpr(const T& value)
{
	return cast_ppu_gpr<T>::to_gpr(value);
}

template<typename T>
force_inline T cast_from_ppu_gpr(const u64 reg)
{
	return cast_ppu_gpr<T>::from_gpr(reg);
}

// flags set in ModuleFunc
enum : u32
{
	MFF_FORCED_HLE = (1 << 0), // always call HLE function
	MFF_NO_RETURN  = (1 << 1), // uses EIF_USE_BRANCH flag with LLE, ignored with MFF_FORCED_HLE

	MFF_PERFECT    = /* 0 */ MFF_FORCED_HLE, // can be set for fully implemented functions with LLE compatibility
};

// flags passed with index
enum : u32
{
	EIF_SAVE_RTOC   = (1 << 25), // save RTOC in [SP+0x28] before calling HLE/LLE function
	EIF_PERFORM_BLR = (1 << 24), // do BLR after calling HLE/LLE function
	EIF_USE_BRANCH  = (1 << 23), // do only branch, LLE must be set, last_syscall must be zero

	EIF_FLAGS       = 0x3800000, // all flags
};
