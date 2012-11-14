#pragma once
#include "Emu/Cell/PPCThread.h"

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
	FPSCR_FX		= 0x80000000,
	FPSCR_FEX		= 0x40000000,
	FPSCR_VX		= 0x20000000,
	FPSCR_OX		= 0x10000000,

	FPSCR_UX		= 0x08000000,
	FPSCR_ZX		= 0x04000000,
	FPSCR_XX		= 0x02000000,
	FPSCR_VXSNAN	= 0x01000000,

	FPSCR_VXISI		= 0x00800000,
	FPSCR_VXIDI		= 0x00400000,
	FPSCR_VXZDZ		= 0x00200000,
	FPSCR_VXIMZ		= 0x00100000,

	FPSCR_VXVC		= 0x00080000,
	FPSCR_FR		= 0x00040000,
	FPSCR_FI		= 0x00020000,

	FPSCR_VXSOFT	= 0x00000400,
	FPSCR_VXSQRT	= 0x00000200,
	FPSCR_VXCVI		= 0x00000100,
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
		u32 RN		:2; //Floating-point rounding control
		u32 NI		:1; //Floating-point non-IEEE mode
		u32 XE		:1; //Floating-point inexact exception enable
		u32 ZE		:1; //IEEE ﬂoating-point zero divide exception enable
		u32 UE		:1; //IEEE ﬂoating-point underﬂow exception enable
		u32 OE		:1; //IEEE ﬂoating-point overﬂow exception enable
		u32 VE		:1; //Floating-point invalid operation exception enable
		u32 VXCVI	:1; //Floating-point invalid operation exception for invalid integer convert
		u32 VXSQRT	:1; //Floating-point invalid operation exception for invalid square root
		u32 VXSOFT	:1; //Floating-point invalid operation exception for software request
		u32			:1; //Reserved
		u32 FPRF	:5; //Floating-point result ﬂags
		u32 FI		:1; //Floating-point fraction inexact
		u32 FR		:1; //Floating-point fraction rounded
		u32 VXVC	:1; //Floating-point invalid operation exception for invalid compare
		u32 VXIMZ	:1; //Floating-point invalid operation exception for * * 0
		u32 VXZDZ	:1; //Floating-point invalid operation exception for 0 / 0
		u32 VXIDI	:1; //Floating-point invalid operation exception for * + *
		u32 VXISI	:1; //Floating-point invalid operation exception for * - *
		u32 VXSNAN	:1; //Floating-point invalid operation exception for SNaN
		u32 XX		:1; //Floating-point inexact exception
		u32 ZX		:1; //Floating-point zero divide exception
		u32 UX		:1; //Floating-point underﬂow exception
		u32 OX		:1; //Floating-point overﬂow exception
		u32 VX		:1; //Floating-point invalid operation exception summary
		u32 FEX		:1; //Floating-point enabled exception summary
		u32 FX		:1; //Floating-point exception summary
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
		u64 LE	: 1;
		
		//Recoverable exception (for system reset and machine check exceptions).
		//0      Exception is not recoverable. 
		//1      Exception is recoverable.
		u64 RI	: 1;
		
		//Reserved
		u64		: 2;
		
		//Data address translation   
		//0      Data address translation is disabled. 
		//1      Data address translation is enabled.
		u64	DR	: 1;
		
		//Instruction address translation   
		//0      Instruction address translation is disabled. 
		//1      Instruction address translation is enabled.
		u64	IR	: 1;

		//Exception preﬁx. The setting of this bit speciﬁes whether an exception vector offset 
		//is prepended with Fs or 0s. In the following description, nnnnn is the offset of the 
		//exception.
		//0      Exceptions are vectored to the physical address 0x0000_0000_000n_nnnn in 64-bit implementations.
		//1      Exceptions are vectored to the physical address 0xFFFF_FFFF_FFFn_nnnn in 64-bit implementations.
		u64	IP	: 1;

		//Reserved
		u64		: 1;
		
		//Floating-point exception mode 1
		u64	FE1	: 1;

		//Branch trace enable (Optional)
		//0      The processor executes branch instructions normally. 
		//1      The processor generates a branch trace exception after completing the 
		//execution of a branch instruction, regardless of whether or not the branch was 
		//taken. 
		//Note: If the function is not implemented, this bit is treated as reserved.
		u64	BE	: 1;

		//Single-step trace enable (Optional)
		//0      The processor executes instructions normally. 
		//1      The processor generates a single-step trace exception upon the successful 
		//execution of the next instruction.
		//Note: If the function is not implemented, this bit is treated as reserved.
		u64	SE	: 1;

		//Floating-point exception mode 0
		u64	FE0	: 1;

		//Machine check enable 
		//0      Machine check exceptions are disabled. 
		//1      Machine check exceptions are enabled.
		u64	ME	: 1;

		//Floating-point available 
		//0      The processor prevents dispatch of ﬂoating-point instructions, including 
		//ﬂoating-point loads, stores, and moves.
		//1      The processor can execute ﬂoating-point instructions.
		u64	FP	: 1;

		//Privilege level 
		//0      The processor can execute both user- and supervisor-level instructions.
		//1      The processor can only execute user-level instructions.
		u64	PR	: 1;

		//External interrupt enable 
		//0      While the bit is cleared the processor delays recognition of external interrupts 
		//and decrementer exception conditions. 
		//1      The processor is enabled to take an external interrupt or the decrementer 
		//exception.
		u64	EE	: 1;

		//Exception little-endian mode. When an exception occurs, this bit is copied into 
		//MSR[LE] to select the endian mode for the context established by the exception
		u64	ILE	: 1;

		//Reserved
		u64		: 1;
		
		//Power management enable
		//0      Power management disabled (normal operation mode).
		//1      Power management enabled (reduced power mode).
		//Note: Power management functions are implementation-dependent. If the function 
		//is not implemented, this bit is treated as reserved.
		u64	POW	: 1;

		//Reserved
		u64		: 44;
		
		//Sixty-four bit mode
		//0      The 64-bit processor runs in 32-bit mode.
		//1      The 64-bit processor runs in 64-bit mode. Note that this is the default setting.
		u64	SF	: 1;
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
		u8 cr0	: 4;
		u8 cr1	: 4;
		u8 cr2	: 4;
		u8 cr3	: 4;
		u8 cr4	: 4;
		u8 cr5	: 4;
		u8 cr6	: 4;
		u8 cr7	: 4;
	};
};

union XERhdr
{
	u64 XER;

	struct
	{
		u64	L	: 61;
		u64 CA	: 1;
		u64 OV	: 1;
		u64 SO	: 1;
	};
};

enum FPRType
{
	FPR_NORM,
	FPR_ZERO,
	FPR_SNAN,
	//FPR_QNAN,
	FPR_INF,
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

struct PPCdouble
{
	union
	{
		double _double;
		u64 _u64;

		struct
		{
			u64 frac	: 52;
			u64 exp		: 11;
			u64 sign	: 1;
		};

		struct
		{
			u64		: 51;
			u64 nan : 1;
			u64		: 12;
		};
	};

	FPRType type;

	u32 GetType()
	{
		if(exp > 0 && exp < 0x7ff) return sign ? FPR_NN : FPR_PN;

		if(frac)
		{
			if(exp) return FPR_QNAN;

			return sign ? FPR_INF : FPR_PINF;
		}

		return sign ? FPR_NZ : FPR_PZ;
	}

	u32 To32()
	{
		if (exp > 896 || (!frac && !exp))
		{
			return ((_u64 >> 32) & 0xc0000000) | ((_u64 >> 29) & 0x3fffffff);
		}

		if (exp >= 874)
		{
			return ((0x80000000 | (frac >> 21)) >> (905 - exp)) | (_u64 >> 32) & 0x80000000;
		}

		//?
		return ((_u64 >> 32) & 0xc0000000) | ((_u64 >> 29) & 0x3fffffff);
	}

	u32 GetZerosCount()
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
	static PPCdouble ConvertToIntegerMode(const PPCdouble& d, FPSCRhdr& fpscr, bool is_64, u32 round_mode)
	{
		PPCdouble ret;

		if(d.exp == 2047)
		{
			if (d.frac == 0)
			{
				ret.type = FPR_INF;

				fpscr.FI = 0;
				fpscr.FR = 0;
				fpscr.VXCVI = 1;

				if(fpscr.VE == 0)
				{
					if(is_64)
					{
						return d.sign ? 0x8000000000000000 : 0x7FFFFFFFFFFFFFFF;
					}
					else
					{
						return d.sign ? 0x80000000 : 0x7FFFFFFF;
					}

					fpscr.FPRF = 0;
				}
			}
			else if(d.nan == 0)
			{
				ret.type = FPR_SNAN;

				fpscr.FI = 0;
				fpscr.FR = 0;
				fpscr.VXCVI = 1;
				fpscr.VXSNAN = 1;

				if(fpscr.VE == 0)
				{
					return is_64 ? 0x8000000000000000 : 0x80000000;
					fpscr.FPRF = 0;
				}
			}
			else
			{
				ret.type = FPR_QNAN;

				fpscr.FI = 0;
				fpscr.FR = 0;
				fpscr.VXCVI = 1;

				if(fpscr.VE == 0)
				{
					return is_64 ? 0x8000000000000000 : 0x80000000;
					fpscr.FPRF = 0;
				}
			}
		}
		else if(d.exp > 1054)
		{
			fpscr.FI = 0;
			fpscr.FR = 0;
			fpscr.VXCVI = 1;

			if(fpscr.VE == 0)
			{
				if(is_64)
				{
					return d.sign ? 0x8000000000000000 : 0x7FFFFFFFFFFFFFFF;
				}
				else
				{
					return d.sign ? 0x80000000 : 0x7FFFFFFF;
				}

				fpscr.FPRF = 0;
			}
		}

		ret.sign = d.sign;

		if(d.exp > 0)
		{
			ret.exp = d.exp - 1023;
			ret.frac = 1 | d.frac;
		}
		else if(d.exp == 0)
		{
			ret.exp = -1022;
			ret.frac = d.frac;
		}
		/*
		if(d.exp == 0)
		{
			if (d.frac == 0)
			{
				d.type = FPR_ZERO;
			}
			else
			{
				const u32 z = d.GetZerosCount() - 8;
				d.frac <<= z + 3;
				d.exp -= 1023 - 1 + z;
				d.type = FPR_NORM;
			}
		}
		else
		{
			d.exp -= 1023;
			d.type = FPR_NORM;
			d.nan = 1;
			d.frac <<= 3;
		}
		*/

		return ret;
	}

	static u32 ConvertToFloatMode(PPCdouble& d, u32 RN)
	{
	/*
		u32 fpscr = 0;
		switch (d.type)
		{
		case FPR_NORM:
			d.exp += 1023;
			if (d.exp > 0)
			{
				fpscr |= Round(d, RN);
				if(d.nan)
				{
					d.exp++;
					d.frac >>= 4;
				}
				else
				{
					d.frac >>= 3;
				}

				if(d.exp >= 2047)
				{
					d.exp = 2047;
					d.frac = 0;
					fpscr |= FPSCR_OX;
				}
			}
			else
			{
				d.exp = -(s64)d.exp + 1;

				if(d.exp <= 56)
				{
					d.frac >>= d.exp;
					fpscr |= Round(d, RN);
					d.frac <<= 1;
					if(d.nan)
					{
						d.exp = 1;
						d.frac = 0;
					}
					else
					{
						d.exp = 0;
						d.frac >>= 4;
						fpscr |= FPSCR_UX;
					}
				}
				else
				{
					d.exp = 0;
					d.frac = 0;
					fpscr |= FPSCR_UX;
				}
			}
		break;

		case FPR_ZERO:
			d.exp = 0;
			d.frac = 0;
		break;

		case FPR_NAN:
			d.exp = 2047;
			d.frac = 1;		
		break;

		case FPR_INF:
			d.exp = 2047;
			d.frac = 0;
		break;
		}

		return fpscr;
		*/
		return 0;
	}

	static u32 Round(PPCdouble& d, u32 RN)
	{
		switch(RN)
		{
		case FPSCR_RN_NEAR:
			if(d.frac & 0x7)
			{
				if((d.frac & 0x7) != 4 || d.frac & 0x8)
				{
					d.frac += 4;
				}

				return FPSCR_XX;
			}
		return 0;

		case FPSCR_RN_ZERO:
			if(d.frac & 0x7) return FPSCR_XX;
		return 0;

		case FPSCR_RN_PINF:
			if(!d.sign && (d.frac & 0x7))
			{
				d.frac += 8;
				return FPSCR_XX;
			}
		return 0;

		case FPSCR_RN_MINF:
			if(d.sign && (d.frac & 0x7))
			{
				d.frac += 8;
				return FPSCR_XX;
			}
		return 0;
		}

		return 0;
	}

	static const u64 double_sign = 0x8000000000000000ULL;
	static const u64 double_frac = 0x000FFFFFFFFFFFFFULL;
	
	static bool IsINF(double d);
	static bool IsNaN(double d);
	static bool IsQNaN(double d);
	static bool IsSNaN(double d);

	static int Cmp(double a, double b);
};

union VPR_reg
{
	//__m128i _m128i;
	u128 _u128;
	s128 _i128;
	u64 _u64[2];
	s64 _i64[2];
	u32 _u32[4];
	s32 _i32[4];
	u16 _u16[8];
	s16 _i16[8];
	u8  _u8[16];
	s8  _i8[16];

	//struct { float x, y, z, w; };

	VPR_reg() { Clear(); }

	VPR_reg(const __m128i val){_u128._u64[0] = val.m128i_u64[0]; _u128._u64[1] = val.m128i_u64[1];}
	VPR_reg(const u128 val) {			_u128	= val; }
	VPR_reg(const u64  val) { Clear();	_u64[0] = val; }
	VPR_reg(const u32  val) { Clear();	_u32[0] = val; }
	VPR_reg(const u16  val) { Clear();	_u16[0] = val; }
	VPR_reg(const u8   val) { Clear();	_u8[0]	= val; }
	VPR_reg(const s128 val) {			_i128	= val; }
	VPR_reg(const s64  val) { Clear();	_i64[0] = val; }
	VPR_reg(const s32  val) { Clear();	_i32[0] = val; }
	VPR_reg(const s16  val) { Clear();	_i16[0] = val; }
	VPR_reg(const s8   val) { Clear();	_i8[0]	= val; }

	operator u128() const { return _u128; }
	operator s128() const { return _i128; }
	operator u64() const { return _u64[0]; }
	operator s64() const { return _i64[0]; }
	operator u32() const { return _u32[0]; }
	operator s32() const { return _i32[0]; }
	operator u16() const { return _u16[0]; }
	operator s16() const { return _i16[0]; }
	operator u8() const { return _u8[0]; }
	operator s8() const { return _i8[0]; }
	operator __m128i() { __m128i ret; ret.m128i_u64[0]=_u128._u64[0]; ret.m128i_u64[1]=_u128._u64[1]; return ret; }
	operator bool() const { return _u64[0] != 0 || _u64[1] != 0; }

	wxString ToString() const
	{
		return wxString::Format("%08x%08x%08x%08x", _u32[3], _u32[2], _u32[1], _u32[0]);
	}

	VPR_reg operator ^  (VPR_reg right) { return _mm_xor_si128(*this, right); }
	VPR_reg operator |  (VPR_reg right) { return _mm_or_si128 (*this, right); }
	VPR_reg operator &  (VPR_reg right) { return _mm_and_si128(*this, right); }

	VPR_reg operator ^  (__m128i right) { return _mm_xor_si128(*this, right); }
	VPR_reg operator |  (__m128i right) { return _mm_or_si128 (*this, right); }
	VPR_reg operator &  (__m128i right) { return _mm_and_si128(*this, right); }

	bool operator == (const VPR_reg& right){ return _u64[0] == right._u64[0] && _u64[1] == right._u64[1]; }

	bool operator == (const u128 right)	{ return _u64[0] == right._u64[0] && _u64[1] == right._u64[1]; }
	bool operator == (const s128 right)	{ return _i64[0] == right._i64[0] && _i64[1] == right._i64[1]; }
	bool operator == (const u64 right)	{ return _u64[0] == (u64)right && _u64[1] == 0; }
	bool operator == (const s64 right)	{ return _i64[0] == (s64)right && _i64[1] == 0; }
	bool operator == (const u32 right)	{ return _u64[0] == (u64)right && _u64[1] == 0; }
	bool operator == (const s32 right)	{ return _i64[0] == (s64)right && _i64[1] == 0; }
	bool operator == (const u16 right)	{ return _u64[0] == (u64)right && _u64[1] == 0; }
	bool operator == (const s16 right)	{ return _i64[0] == (s64)right && _i64[1] == 0; }
	bool operator == (const u8 right)	{ return _u64[0] == (u64)right && _u64[1] == 0; }
	bool operator == (const s8 right)	{ return _i64[0] == (s64)right && _i64[1] == 0; }

	bool operator != (const VPR_reg& right){ return !(*this == right); }
	bool operator != (const u128 right)	{ return !(*this == right); }
	bool operator != (const u64 right)	{ return !(*this == right); }
	bool operator != (const u32 right)	{ return !(*this == right); }
	bool operator != (const u16 right)	{ return !(*this == right); }
	bool operator != (const u8 right)	{ return !(*this == right); }
	bool operator != (const s128 right)	{ return !(*this == right); }
	bool operator != (const s64 right)	{ return !(*this == right); }
	bool operator != (const s32 right)	{ return !(*this == right); }
	bool operator != (const s16 right)	{ return !(*this == right); }
	bool operator != (const s8 right)	{ return !(*this == right); }

	s64& d(const u32 c) { return _i64[1 - c]; }
	u64& ud(const u32 c) { return _u64[1 - c]; }

	s32& w(const u32 c) { return _i32[3 - c]; }
	u32& uw(const u32 c) { return _u32[3 - c]; }

	s16& h(const u32 c) { return _i16[7 - c]; }
	u16& uh(const u32 c) { return _u16[7 - c]; }

	s8& b(const u32 c) { return _i8[15 - c]; }
	u8& ub(const u32 c) { return _u8[15 - c]; }

	void Clear() { memset(this, 0, sizeof(*this)); }
};

/*
struct VPR_table
{
	VPR_reg t[32];

	operator VPR_reg*() { return t; }

	VPR_reg& operator [] (int index)
	{
		return t[index];
	}
}
*/

static const s32 MAX_INT_VALUE = 0x7fffffff;

class PPUThread : public PPCThread
{
public:
	double FPR[32]; //Floating Point Register
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

	u64 LR;		//SPR 0x008 : Link Register
	u64 CTR;	//SPR 0x009 : Count Register

	s32 USPRG;	//SPR 0x100 : User-SPR General-Purpose Registers

	s32 SPRG[8]; //SPR 0x100 - 0x107 : SPR General-Purpose Registers

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

	u64 reserve_addr;
	bool reserve;

	u64 cycle;

public:
	PPUThread();
	~PPUThread();

	inline u8 GetCR(const u8 n) const
	{
		switch(n)
		{
		case 7: return CR.cr0;
		case 6: return CR.cr1;
		case 5: return CR.cr2;
		case 4: return CR.cr3;
		case 3: return CR.cr4;
		case 2: return CR.cr5;
		case 1: return CR.cr6;
		case 0: return CR.cr7;
		}

		return 0;
	}

	inline void SetCR(const u8 n, const u32 value)
	{
		switch(n)
		{
		case 7: CR.cr0 = value; break;
		case 6: CR.cr1 = value; break;
		case 5: CR.cr2 = value; break;
		case 4: CR.cr3 = value; break;
		case 3: CR.cr4 = value; break;
		case 2: CR.cr5 = value; break;
		case 1: CR.cr6 = value; break;
		case 0: CR.cr7 = value; break;
		}
	}

	inline void SetCRBit(const u8 n, const u32 bit, const bool value)
	{
		switch(n)
		{
		case 7: value ? CR.cr0 |= bit : CR.cr0 &= ~bit; break;
		case 6: value ? CR.cr1 |= bit : CR.cr1 &= ~bit; break;
		case 5: value ? CR.cr2 |= bit : CR.cr2 &= ~bit; break;
		case 4: value ? CR.cr3 |= bit : CR.cr3 &= ~bit; break;
		case 3: value ? CR.cr4 |= bit : CR.cr4 &= ~bit; break;
		case 2: value ? CR.cr5 |= bit : CR.cr5 &= ~bit; break;
		case 1: value ? CR.cr6 |= bit : CR.cr6 &= ~bit; break;
		case 0: value ? CR.cr7 |= bit : CR.cr7 &= ~bit; break;
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
		if		(a <  b) SetCR(n, CR_LT);
		else if	(a >  b) SetCR(n, CR_GT);
		else if	(a == b) SetCR(n, CR_EQ);

		SetCR_SO(n, XER.SO);
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

	bool IsCarry(const u64 a, const u64 b) { return a > (~b); }

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

	virtual wxString RegsToString()
	{
		wxString ret = PPCThread::RegsToString();
		for(uint i=0; i<32; ++i) ret += wxString::Format("GPR[%d] = 0x%llx\n", i, GPR[i]);
		for(uint i=0; i<32; ++i) ret += wxString::Format("FPR[%d] = %.6G\n", i, FPR[i]);
		ret += wxString::Format("CR = 0x%08x\n", CR);
		ret += wxString::Format("LR = 0x%llx\n", LR);
		ret += wxString::Format("CTR = 0x%llx\n", CTR);
		ret += wxString::Format("XER = 0x%llx\n", XER);
		ret += wxString::Format("FPSCR = 0x%x\n", FPSCR);
		return ret;
	}

	void SetBranch(const u64 pc);
	virtual void AddArgv(const wxString& arg);

public:
	virtual void InitRegs(); 
	virtual u64 GetFreeStackSize() const;

protected:
	virtual void DoReset();
	virtual void DoRun();
	virtual void DoPause();
	virtual void DoResume();
	virtual void DoStop();

private:
	virtual void DoCode(const s32 code);
};