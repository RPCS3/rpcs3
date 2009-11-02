/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 * 
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

 
#include "PrecompiledHeader.h"
 
#include "Common.h"
#include "R5900OpcodeTables.h"
#include "x86emitter/x86emitter.h"
#include "iR5900.h"
#include "iFPU.h"

/* Version of the FPU that emulates an exponent of 0xff and overflow/underflow flags */

/* Can be made faster by not converting stuff back and forth between instructions. */
 

using namespace x86Emitter;
 
//set overflow flag (set only if FPU_RESULT is 1)
#define FPU_FLAGS_OVERFLOW 1 
//set underflow flag (set only if FPU_RESULT is 1)
#define FPU_FLAGS_UNDERFLOW 1 
 
//if 1, result is not clamped (Gives correct results as in PS2,	
//but can cause problems due to insufficient clamping levels in the VUs)
#define FPU_RESULT 1 
 
//set I&D flags. also impacts other aspects of DIV/R/SQRT correctness
#define FPU_FLAGS_ID 1 

#ifdef FPU_RECOMPILE 

//------------------------------------------------------------------
namespace R5900 {
namespace Dynarec {
namespace OpcodeImpl {
namespace COP1 {
	
u32 __fastcall FPU_MUL_MANTISSA(u32 s, u32 t);

namespace DOUBLE {
 
//------------------------------------------------------------------
// Helper Macros
//------------------------------------------------------------------
#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_
 
// FCR31 Flags
#define FPUflagC	0X00800000
#define FPUflagI	0X00020000
#define FPUflagD	0X00010000
#define FPUflagO	0X00008000
#define FPUflagU	0X00004000
#define FPUflagSI	0X00000040
#define FPUflagSD	0X00000020
#define FPUflagSO	0X00000010
#define FPUflagSU	0X00000008
 
#define FPU_ADD_SUB_HACK 1 // Add/Sub opcodes produce more ps2-like results if set to 1
 
#define REC_FPUBRANCH(f) \
	void f(); \
	void rec##f() { \
	MOV32ItoM((uptr)&cpuRegs.code, cpuRegs.code); \
	MOV32ItoM((uptr)&cpuRegs.pc, pc); \
	iFlushCall(FLUSH_EVERYTHING); \
	CALLFunc((uptr)R5900::Interpreter::OpcodeImpl::COP1::f); \
	branch = 2; \
}
 
#define REC_FPUFUNC(f) \
	void f(); \
	void rec##f() { \
	MOV32ItoM((uptr)&cpuRegs.code, cpuRegs.code); \
	MOV32ItoM((uptr)&cpuRegs.pc, pc); \
	iFlushCall(FLUSH_EVERYTHING); \
	CALLFunc((uptr)R5900::Interpreter::OpcodeImpl::COP1::f); \
}
//------------------------------------------------------------------
 
//------------------------------------------------------------------
// *FPU Opcodes!*
//------------------------------------------------------------------ 
  
//------------------------------------------------------------------
// PS2 -> DOUBLE
//------------------------------------------------------------------

#define SINGLE(sign, exp, mant) (((sign)<<31) | ((exp)<<23) | (mant))
#define DOUBLE(sign, exp, mant) (((sign ## ULL)<<63) | ((exp ## ULL)<<52) | (mant ## ULL))

struct FPUd_Globals
{
	u32		neg[4], pos[4];

	u32		pos_inf[4], neg_inf[4],
			one_exp[4];

	u64		dbl_one_exp[2];
	
	u64		dbl_cvt_overflow,	// needs special code if above or equal
			dbl_ps2_overflow,	// overflow & clamp if above or equal
			dbl_underflow;		// underflow if below

	u64		padding;

	u64		dbl_s_pos[2];
	//u64		dlb_s_neg[2];
};

static const __aligned(32) FPUd_Globals s_const = 
{
	{ 0x80000000, 0xffffffff, 0xffffffff, 0xffffffff },
	{ 0x7fffffff, 0xffffffff, 0xffffffff, 0xffffffff },

	{SINGLE(0,0xff,0), 0, 0, 0},
	{SINGLE(1,0xff,0), 0, 0, 0},
	{SINGLE(0,1,0), 0, 0, 0},

	{DOUBLE(0,1,0), 0},

	DOUBLE(0,1151,0),	// cvt_overflow
	DOUBLE(0,1152,0),	// ps2_overflow
	DOUBLE(0,897,0),	// underflow

	0,					// Padding!!

	{0x7fffffffffffffffULL, 0},
	//{0x8000000000000000ULL, 0},
};

 
// converts small normal numbers to double equivalent
// converts large normal numbers (which represent NaN/inf in IEEE) to double equivalent

//mustn't use EAX/ECX/EDX/x86regs (MUL)
void ToDouble(int reg)
{
	SSE_UCOMISS_M32_to_XMM(reg, (uptr)s_const.pos_inf); //sets ZF if equal or incomparable
	u8 *to_complex = JE8(0); //complex conversion if positive infinity or NaN
	SSE_UCOMISS_M32_to_XMM(reg, (uptr)s_const.neg_inf); 
	u8 *to_complex2 = JE8(0); //complex conversion if negative infinity
 
	SSE2_CVTSS2SD_XMM_to_XMM(reg, reg); //simply convert
	u8 *end = JMP8(0);
 
	x86SetJ8(to_complex);
	x86SetJ8(to_complex2);
 
	SSE2_PSUBD_M128_to_XMM(reg, (uptr)s_const.one_exp); //lower exponent
	SSE2_CVTSS2SD_XMM_to_XMM(reg, reg); 
	SSE2_PADDQ_M128_to_XMM(reg, (uptr)s_const.dbl_one_exp); //raise exponent
 
	x86SetJ8(end);
}
 
//------------------------------------------------------------------
// DOUBLE -> PS2
//------------------------------------------------------------------
 
/* 
	if FPU_RESULT, results are more like the real PS2's FPU. But if the VU doesn't clamp all operands,
		new issues may happen if a game transfers the FPU results into the VU and continues operations there.
		ar tonelico 1 does this with the result from DIV/RSQRT (when a division by zero occurs)
	otherwise, results are still usually better than iFPU.cpp.
*/
 
//mustn't use EAX/ECX/EDX/x86regs (MUL)
 
// converts small normal numbers to PS2 equivalent
// converts large normal numbers to PS2 equivalent (which represent NaN/inf in IEEE)
// converts really large normal numbers to PS2 signed max
// converts really small normal numbers to zero (flush)
// doesn't handle inf/nan/denormal
void ToPS2FPU_Full(int reg, bool flags, int absreg, bool acc, bool addsub)
{
	if (flags)
		AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagO | FPUflagU));
	if (flags && acc)
		AND32ItoM((uptr)&fpuRegs.ACCflag, ~1);

	SSE_MOVAPS_XMM_to_XMM(absreg, reg);
	SSE2_ANDPD_M128_to_XMM(absreg, (uptr)&s_const.dbl_s_pos); 
 
	SSE2_UCOMISD_M64_to_XMM(absreg, (uptr)&s_const.dbl_cvt_overflow);
	u8 *to_complex = JAE8(0);
 
	SSE2_UCOMISD_M64_to_XMM(absreg, (uptr)&s_const.dbl_underflow);
	u8 *to_underflow = JB8(0); 
 
	SSE2_CVTSD2SS_XMM_to_XMM(reg, reg); //simply convert
	u8 *end = JMP8(0);
 
	x86SetJ8(to_complex); 
	SSE2_UCOMISD_M64_to_XMM(absreg, (uptr)&s_const.dbl_ps2_overflow);
	u8 *to_overflow = JAE8(0);
 
	SSE2_PSUBQ_M128_to_XMM(reg, (uptr)&s_const.dbl_one_exp); //lower exponent
	SSE2_CVTSD2SS_XMM_to_XMM(reg, reg); //convert
	SSE2_PADDD_M128_to_XMM(reg, (uptr)s_const.one_exp); //raise exponent
	u8 *end2 = JMP8(0);
 
	x86SetJ8(to_overflow); 
	SSE2_CVTSD2SS_XMM_to_XMM(reg, reg); 
	SSE_ORPS_M128_to_XMM(reg, (uptr)&s_const.pos); //clamp
	if (flags && FPU_FLAGS_OVERFLOW)
		OR32ItoM((uptr)&fpuRegs.fprc[31], (FPUflagO | FPUflagSO));
	if (flags && FPU_FLAGS_OVERFLOW && acc)
		OR32ItoM((uptr)&fpuRegs.ACCflag, 1);
	u8 *end3 = JMP8(0);
 
	x86SetJ8(to_underflow);
	u8 *end4;
	if (flags && FPU_FLAGS_UNDERFLOW) //set underflow flags if not zero
	{
		SSE2_XORPD_XMM_to_XMM(absreg, absreg);
		SSE2_UCOMISD_XMM_to_XMM(reg, absreg);
		u8 *is_zero = JE8(0);
 
		OR32ItoM((uptr)&fpuRegs.fprc[31], (FPUflagU | FPUflagSU));
		if (addsub)
		{
			//On ADD/SUB, the PS2 simply leaves the mantissa bits as they are (after normalization)
			//IEEE either clears them (FtZ) or returns the denormalized result.
			//not thoroughly tested : other operations such as MUL and DIV seem to clear all mantissa bits?
			SSE_MOVAPS_XMM_to_XMM(absreg, reg);
			SSE2_PSLLQ_I8_to_XMM(reg, 12); //mantissa bits
			SSE2_PSRLQ_I8_to_XMM(reg, 41);
			SSE2_PSRLQ_I8_to_XMM(absreg, 63); //sign bit
			SSE2_PSLLQ_I8_to_XMM(absreg, 31);
			SSE2_POR_XMM_to_XMM(reg, absreg);
			end4 = JMP8(0);
		}
 
		x86SetJ8(is_zero);
	}
	SSE2_CVTSD2SS_XMM_to_XMM(reg, reg);
	SSE_ANDPS_M128_to_XMM(reg, (uptr)s_const.neg); //flush to zero
 
	x86SetJ8(end);
	x86SetJ8(end2);
	x86SetJ8(end3);
	if (flags && FPU_FLAGS_UNDERFLOW && addsub)
		x86SetJ8(end4);
}
 
//mustn't use EAX/ECX/EDX/x86regs (MUL)
void ToPS2FPU(int reg, bool flags, int absreg, bool acc, bool addsub = false)
{
	if (FPU_RESULT)
		ToPS2FPU_Full(reg, flags, absreg, acc, addsub);
	else
	{
		SSE2_CVTSD2SS_XMM_to_XMM(reg, reg); //clamp
		SSE_MINSS_M32_to_XMM(reg, (uptr)&g_maxvals[0]); 
		SSE_MAXSS_M32_to_XMM(reg, (uptr)&g_minvals[0]);
	}
}
 
//sets the maximum (positive or negative) value into regd.
void SetMaxValue(int regd)
{
	if (FPU_RESULT)
		SSE_ORPS_M128_to_XMM(regd, (uptr)&s_const.pos[0]); // set regd to maximum
	else
	{
		SSE_ANDPS_M128_to_XMM(regd, (uptr)&s_const.neg[0]); // Get the sign bit
		SSE_ORPS_M128_to_XMM(regd, (uptr)&g_maxvals[0]); // regd = +/- Maximum  (CLAMP)!
	}
}
 
#define GET_S(sreg) { \
	if( info & PROCESS_EE_S ) SSE_MOVSS_XMM_to_XMM(sreg, EEREC_S); \
	else SSE_MOVSS_M32_to_XMM(sreg, (uptr)&fpuRegs.fpr[_Fs_]); }
 
#define ALLOC_S(sreg) { sreg = _allocTempXMMreg(XMMT_FPS, -1);  GET_S(sreg); }
 
#define GET_T(treg) { \
	if( info & PROCESS_EE_T ) SSE_MOVSS_XMM_to_XMM(treg, EEREC_T); \
	else SSE_MOVSS_M32_to_XMM(treg, (uptr)&fpuRegs.fpr[_Ft_]); }
 
#define ALLOC_T(treg) { treg = _allocTempXMMreg(XMMT_FPS, -1);  GET_T(treg); }
 
#define GET_ACC(areg) { \
	if( info & PROCESS_EE_ACC ) SSE_MOVSS_XMM_to_XMM(areg, EEREC_ACC); \
	else SSE_MOVSS_M32_to_XMM(areg, (uptr)&fpuRegs.ACC); }
 
#define ALLOC_ACC(areg) { areg = _allocTempXMMreg(XMMT_FPS, -1);  GET_ACC(areg); }
 
#define CLEAR_OU_FLAGS { AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagO | FPUflagU)); }
 
 
//------------------------------------------------------------------
// ABS XMM
//------------------------------------------------------------------
void recABS_S_xmm(int info)
{	
	GET_S(EEREC_D);
 
	CLEAR_OU_FLAGS;
 
	SSE_ANDPS_M128_to_XMM(EEREC_D, (uptr)s_const.pos);
}
 
FPURECOMPILE_CONSTCODE(ABS_S, XMMINFO_WRITED|XMMINFO_READS);
//------------------------------------------------------------------
 
 
//------------------------------------------------------------------
// FPU_ADD_SUB (Used to mimic PS2's FPU add/sub behavior)
//------------------------------------------------------------------
// Compliant IEEE FPU uses, in computations, uses additional "guard" bits to the right of the mantissa
// but EE-FPU doesn't. Substraction (and addition of positive and negative) may shift the mantissa left,
// causing those bits to appear in the result; this function masks out the bits of the mantissa that will
// get shifted right to the guard bits to ensure that the guard bits are empty.
// The difference of the exponents = the amount that the smaller operand will be shifted right by.
// Modification - the PS2 uses a single guard bit? (Coded by Nneeve)
//------------------------------------------------------------------
void FPU_ADD_SUB(int tempd, int tempt) //tempd and tempt are overwritten, they are floats
{
	int tempecx = _allocX86reg(ECX, X86TYPE_TEMP, 0, 0); //receives regd
	int temp2 = _allocX86reg(-1, X86TYPE_TEMP, 0, 0); //receives regt
	int xmmtemp = _allocTempXMMreg(XMMT_FPS, -1); //temporary for anding with regd/regt
 
	if (tempecx != ECX)	{ Console.Error("FPU: ADD/SUB Allocation Error!"); tempecx = ECX;}
	if (temp2 == -1)	{ Console.Error("FPU: ADD/SUB Allocation Error!"); temp2 = EAX;}
	if (xmmtemp == -1)	{ Console.Error("FPU: ADD/SUB Allocation Error!"); xmmtemp = XMM0;}
 
	SSE2_MOVD_XMM_to_R(tempecx, tempd); 
	SSE2_MOVD_XMM_to_R(temp2, tempt);
 
	//mask the exponents
	SHR32ItoR(tempecx, 23); 
	SHR32ItoR(temp2, 23);
	AND32ItoR(tempecx, 0xff);
	AND32ItoR(temp2, 0xff); 
 
	SUB32RtoR(tempecx, temp2); //tempecx = exponent difference
	CMP32ItoR(tempecx, 25);
	j8Ptr[0] = JGE8(0);
	CMP32ItoR(tempecx, 0);
	j8Ptr[1] = JG8(0);
	j8Ptr[2] = JE8(0);
	CMP32ItoR(tempecx, -25);
	j8Ptr[3] = JLE8(0);
 
	//diff = -24 .. -1 , expd < expt
	NEG32R(tempecx); 
	DEC32R(tempecx);
	MOV32ItoR(temp2, 0xffffffff); 
	SHL32CLtoR(temp2); //temp2 = 0xffffffff << tempecx
	SSE2_MOVD_R_to_XMM(xmmtemp, temp2);
	SSE_ANDPS_XMM_to_XMM(tempd, xmmtemp); 
	j8Ptr[4] = JMP8(0);
 
	x86SetJ8(j8Ptr[0]);
	//diff = 25 .. 255 , expt < expd
	SSE_ANDPS_M128_to_XMM(tempt, (uptr)s_const.neg);
	j8Ptr[5] = JMP8(0);
 
	x86SetJ8(j8Ptr[1]);
	//diff = 1 .. 24, expt < expd
	DEC32R(tempecx);
	MOV32ItoR(temp2, 0xffffffff);
	SHL32CLtoR(temp2); //temp2 = 0xffffffff << tempecx
	SSE2_MOVD_R_to_XMM(xmmtemp, temp2);
	SSE_ANDPS_XMM_to_XMM(tempt, xmmtemp);
	j8Ptr[6] = JMP8(0);
 
	x86SetJ8(j8Ptr[3]);
	//diff = -255 .. -25, expd < expt
	SSE_ANDPS_M128_to_XMM(tempd, (uptr)s_const.neg); 
	j8Ptr[7] = JMP8(0);
 
	x86SetJ8(j8Ptr[2]);
	//diff == 0
 
	x86SetJ8(j8Ptr[4]);
	x86SetJ8(j8Ptr[5]);
	x86SetJ8(j8Ptr[6]);
	x86SetJ8(j8Ptr[7]);
 
	_freeXMMreg(xmmtemp);
	_freeX86reg(temp2);
	_freeX86reg(tempecx);
}
 
 
void FPU_MUL(int info, int regd, int sreg, int treg, bool acc)
{
	if (CHECK_FPUMULHACK)
	{
		SSE2_MOVD_XMM_to_R(ECX, sreg);
		SSE2_MOVD_XMM_to_R(EDX, treg);
		CALLFunc( (uptr)&FPU_MUL_MANTISSA );
		ToDouble(sreg); ToDouble(treg); 
		SSE2_MULSD_XMM_to_XMM(sreg, treg);
		ToPS2FPU(sreg, true, treg, acc);
		SSE_MOVSS_XMM_to_XMM(regd, sreg);
		SSE2_MOVD_XMM_to_R(ECX, regd);
		AND32ItoR(ECX, 0xff800000);
		OR32RtoR(EAX, ECX);
		SSE2_MOVD_R_to_XMM(regd, EAX);
	}
	else
	{
		ToDouble(sreg); ToDouble(treg); 
		SSE2_MULSD_XMM_to_XMM(sreg, treg);
		ToPS2FPU(sreg, true, treg, acc);
		SSE_MOVSS_XMM_to_XMM(regd, sreg);
	}
}

//------------------------------------------------------------------
// CommutativeOp XMM (used for ADD and SUB opcodes. that's it.)
//------------------------------------------------------------------
static void (*recFPUOpXMM_to_XMM[] )(x86SSERegType, x86SSERegType) = {
	SSE2_ADDSD_XMM_to_XMM, SSE2_SUBSD_XMM_to_XMM };
 
void recFPUOp(int info, int regd, int op, bool acc) 
{
	int sreg, treg;
	ALLOC_S(sreg); ALLOC_T(treg);
 
	if (FPU_ADD_SUB_HACK) //ADD or SUB
		FPU_ADD_SUB(sreg, treg);
 
	ToDouble(sreg); ToDouble(treg);
 
	recFPUOpXMM_to_XMM[op](sreg, treg);
 
	ToPS2FPU(sreg, true, treg, acc, true);
	SSE_MOVSS_XMM_to_XMM(regd, sreg);
 
	_freeXMMreg(sreg); _freeXMMreg(treg);
}
//------------------------------------------------------------------
 
 
//------------------------------------------------------------------
// ADD XMM
//------------------------------------------------------------------
void recADD_S_xmm(int info)
{
    recFPUOp(info, EEREC_D, 0, false);
}
 
FPURECOMPILE_CONSTCODE(ADD_S, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);
 
void recADDA_S_xmm(int info)
{
    recFPUOp(info, EEREC_ACC, 0, true);
}
 
FPURECOMPILE_CONSTCODE(ADDA_S, XMMINFO_WRITEACC|XMMINFO_READS|XMMINFO_READT);
//------------------------------------------------------------------
 
void recCMP(int info)
{
	int sreg, treg;
	ALLOC_S(sreg); ALLOC_T(treg);
	ToDouble(sreg); ToDouble(treg);
 
	SSE2_UCOMISD_XMM_to_XMM(sreg, treg);
 
	_freeXMMreg(sreg); _freeXMMreg(treg);
}
 
//------------------------------------------------------------------
// C.x.S XMM
//------------------------------------------------------------------
void recC_EQ_xmm(int info)
{
	recCMP(info);
 
	j8Ptr[0] = JZ8(0);
		AND32ItoM( (uptr)&fpuRegs.fprc[31], ~FPUflagC );
		j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);
		OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagC);
	x86SetJ8(j8Ptr[1]);
}
 
FPURECOMPILE_CONSTCODE(C_EQ, XMMINFO_READS|XMMINFO_READT);
  
void recC_LE_xmm(int info )
{
	recCMP(info);
 
	j8Ptr[0] = JBE8(0);
		AND32ItoM( (uptr)&fpuRegs.fprc[31], ~FPUflagC );
		j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);
		OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagC);
	x86SetJ8(j8Ptr[1]);
}
 
FPURECOMPILE_CONSTCODE(C_LE, XMMINFO_READS|XMMINFO_READT);
 
void recC_LT_xmm(int info)
{
	recCMP(info);
 
	j8Ptr[0] = JB8(0);
		AND32ItoM( (uptr)&fpuRegs.fprc[31], ~FPUflagC );
		j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);
		OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagC);
	x86SetJ8(j8Ptr[1]);
}
 
FPURECOMPILE_CONSTCODE(C_LT, XMMINFO_READS|XMMINFO_READT);
//------------------------------------------------------------------
 
 
//------------------------------------------------------------------
// CVT.x XMM
//------------------------------------------------------------------
void recCVT_S_xmm(int info)
{
	if( !(info&PROCESS_EE_S) || (EEREC_D != EEREC_S && !(info&PROCESS_EE_MODEWRITES)) ) {
		SSE_CVTSI2SS_M32_to_XMM(EEREC_D, (uptr)&fpuRegs.fpr[_Fs_]);
	}
	else {
		SSE2_CVTDQ2PS_XMM_to_XMM(EEREC_D, EEREC_S);	
	}
}
 
FPURECOMPILE_CONSTCODE(CVT_S, XMMINFO_WRITED|XMMINFO_READS);
 
void recCVT_W() //called from iFPU.cpp's recCVT_W
{
	int regs = _checkXMMreg(XMMTYPE_FPREG, _Fs_, MODE_READ);
 
	if( regs >= 0 )
	{		
		SSE_CVTTSS2SI_XMM_to_R32(EAX, regs);
		SSE_MOVMSKPS_XMM_to_R32(EDX, regs);	//extract the signs
		AND32ItoR(EDX,1);					//keep only LSB
	}
	else 
	{
		SSE_CVTTSS2SI_M32_to_R32(EAX, (uptr)&fpuRegs.fpr[ _Fs_ ]);
		MOV32MtoR(EDX, (uptr)&fpuRegs.fpr[ _Fs_ ]);	
		SHR32ItoR(EDX, 31);	//mov sign to lsb
	}
 
	//kill register allocation for dst because we write directly to fpuRegs.fpr[_Fd_]
	_deleteFPtoXMMreg(_Fd_, 2);
 
	ADD32ItoR(EDX, 0x7FFFFFFF);	//0x7FFFFFFF if positive, 0x8000 0000 if negative
 
	CMP32ItoR(EAX, 0x80000000);	//If the result is indefinitive 
	CMOVE32RtoR(EAX, EDX);		//Saturate it
 
	//Write the result
	MOV32RtoM((uptr)&fpuRegs.fpr[_Fd_], EAX);
}
//------------------------------------------------------------------
 
 
//------------------------------------------------------------------
// DIV XMM
//------------------------------------------------------------------
void recDIVhelper1(int regd, int regt) // Sets flags
{
	u8 *pjmp1, *pjmp2;
	u32 *ajmp32, *bjmp32;
	int t1reg = _allocTempXMMreg(XMMT_FPS, -1);
	int tempReg = _allocX86reg(-1, X86TYPE_TEMP, 0, 0);
	if (t1reg == -1) {Console.Error("FPU: DIV Allocation Error!");}
	if (tempReg == -1) {Console.Error("FPU: DIV Allocation Error!"); tempReg = EAX;}
 
	AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagI|FPUflagD)); // Clear I and D flags
 
	//--- Check for divide by zero ---
	SSE_XORPS_XMM_to_XMM(t1reg, t1reg);
	SSE_CMPEQSS_XMM_to_XMM(t1reg, regt);
	SSE_MOVMSKPS_XMM_to_R32(tempReg, t1reg);
	AND32ItoR(tempReg, 1);  //Check sign (if regt == zero, sign will be set)
	ajmp32 = JZ32(0); //Skip if not set
 
		//--- Check for 0/0 ---
		SSE_XORPS_XMM_to_XMM(t1reg, t1reg);
		SSE_CMPEQSS_XMM_to_XMM(t1reg, regd);
		SSE_MOVMSKPS_XMM_to_R32(tempReg, t1reg);
		AND32ItoR(tempReg, 1);  //Check sign (if regd == zero, sign will be set)
		pjmp1 = JZ8(0); //Skip if not set
			OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagI|FPUflagSI); // Set I and SI flags ( 0/0 )
			pjmp2 = JMP8(0);
		x86SetJ8(pjmp1); //x/0 but not 0/0
			OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagD|FPUflagSD); // Set D and SD flags ( x/0 )
		x86SetJ8(pjmp2);
 
		//--- Make regd +/- Maximum ---
		SSE_XORPS_XMM_to_XMM(regd, regt); // Make regd Positive or Negative	
		SetMaxValue(regd); //clamp to max
		bjmp32 = JMP32(0);
 
	x86SetJ32(ajmp32);
 
	//--- Normal Divide ---
	ToDouble(regd); ToDouble(regt);
 
	SSE2_DIVSD_XMM_to_XMM(regd, regt);
 
	ToPS2FPU(regd, false, regt, false);
 
	x86SetJ32(bjmp32);
 
	_freeXMMreg(t1reg);
	_freeX86reg(tempReg);
}
 
void recDIVhelper2(int regd, int regt) // Doesn't sets flags
{
	ToDouble(regd); ToDouble(regt);
 
	SSE2_DIVSD_XMM_to_XMM(regd, regt);
 
	ToPS2FPU(regd, false, regt, false);
}

static __aligned16 SSE_MXCSR roundmode_nearest, roundmode_neg;

void recDIV_S_xmm(int info)
{
	int roundmodeFlag = 0;
    //if (t0reg == -1) {Console.Error("FPU: DIV Allocation Error!");}
    //Console.WriteLn("DIV");
 
	if (g_sseMXCSR.GetRoundMode() != SSEround_Nearest)
	{
		// Set roundmode to nearest since it isn't already
		//Console.WriteLn("div to nearest");

		if( CHECK_FPUNEGDIVHACK )
		{
			roundmode_neg = g_sseMXCSR;
			roundmode_neg.SetRoundMode( SSEround_NegInf );
			xLDMXCSR( roundmode_neg );
		}
		else
		{
			roundmode_nearest = g_sseMXCSR;
			roundmode_nearest.SetRoundMode( SSEround_Nearest );
			xLDMXCSR( roundmode_nearest );
		}
		roundmodeFlag = 1;
	}
 
	int sreg, treg;
 
	ALLOC_S(sreg); ALLOC_T(treg);
 
	if (FPU_FLAGS_ID)
		recDIVhelper1(sreg, treg);
	else
		recDIVhelper2(sreg, treg);
 
	SSE_MOVSS_XMM_to_XMM(EEREC_D, sreg);
 
	if (roundmodeFlag == 1) { // Set roundmode back if it was changed
		xLDMXCSR (g_sseMXCSR);
	}
	_freeXMMreg(sreg); _freeXMMreg(treg);
}
 
FPURECOMPILE_CONSTCODE(DIV_S, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);
//------------------------------------------------------------------
 
 
//------------------------------------------------------------------
// MADD/MSUB XMM
//------------------------------------------------------------------

// Unlike what the documentation implies, it seems that MADD/MSUB support all numbers just like other operations
// The complex overflow conditions the document describes apparently test whether the multiplication's result
// has overflowed and whether the last operation that used ACC as a destination has overflowed.
// For example,   { adda.s -MAX, 0.0 ; madd.s fd, MAX, 1.0 } -> fd = 0
// while          { adda.s -MAX, -MAX ; madd.s fd, MAX, 1.0 } -> fd = -MAX
// (where MAX is 0x7fffffff and -MAX is 0xffffffff)
void recMaddsub(int info, int regd, int op, bool acc)
{	
	int sreg, treg;
	ALLOC_S(sreg); ALLOC_T(treg);

	FPU_MUL(info, sreg, sreg, treg, false); 

	GET_ACC(treg); 

	if (FPU_ADD_SUB_HACK) //ADD or SUB
		FPU_ADD_SUB(treg, sreg); //might be problematic for something!!!!

	//          TEST FOR ACC/MUL OVERFLOWS, PROPOGATE THEM IF THEY OCCUR

	TEST32ItoM((uptr)&fpuRegs.fprc[31], FPUflagO);
	u8 *mulovf = JNZ8(0); 	
	ToDouble(sreg); //else, convert

	TEST32ItoM((uptr)&fpuRegs.ACCflag, 1);
	u8 *accovf = JNZ8(0); 	
	ToDouble(treg); //else, convert
	u8 *operation = JMP8(0);

	x86SetJ8(mulovf);
	if (op == 1) //sub
		SSE_XORPS_M128_to_XMM(sreg, (uptr)s_const.neg);
	SSE_MOVAPS_XMM_to_XMM(treg, sreg);  //fall through below

	x86SetJ8(accovf);
	SetMaxValue(treg); //just in case... I think it has to be a MaxValue already here
	CLEAR_OU_FLAGS; //clear U flag
	if (FPU_FLAGS_OVERFLOW) 
		OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagO | FPUflagSO);
	if (FPU_FLAGS_OVERFLOW && acc) 
		OR32ItoM((uptr)&fpuRegs.ACCflag, 1);
	u32 *skipall = JMP32(0);

	//			PERFORM THE ACCUMULATION AND TEST RESULT. CONVERT TO SINGLE 

	x86SetJ8(operation);
	if (op == 1)
		SSE2_SUBSD_XMM_to_XMM(treg, sreg);
	else
		SSE2_ADDSD_XMM_to_XMM(treg, sreg);

	ToPS2FPU(treg, true, sreg, acc, true);
	x86SetJ32(skipall);

	SSE_MOVSS_XMM_to_XMM(regd, treg);

	_freeXMMreg(sreg); _freeXMMreg(treg);
}
 
void recMADD_S_xmm(int info)
{
	recMaddsub(info, EEREC_D, 0, false);
}
 
FPURECOMPILE_CONSTCODE(MADD_S, XMMINFO_WRITED|XMMINFO_READACC|XMMINFO_READS|XMMINFO_READT);
 
void recMADDA_S_xmm(int info)
{
	recMaddsub(info, EEREC_ACC, 0, true);
}
 
FPURECOMPILE_CONSTCODE(MADDA_S, XMMINFO_WRITEACC|XMMINFO_READACC|XMMINFO_READS|XMMINFO_READT);
//------------------------------------------------------------------
 
 
//------------------------------------------------------------------
// MAX / MIN XMM
//------------------------------------------------------------------
 
static const __aligned16 u32 minmax_mask[8] =
{
	0xffffffff,	0x80000000, 0, 0,
	0,			0x40000000, 0, 0
};
// FPU's MAX/MIN work with all numbers (including "denormals"). Check VU's logical min max for more info.
void recMINMAX(int info, bool ismin)
{
	int sreg, treg;
	ALLOC_S(sreg); ALLOC_T(treg);

	CLEAR_OU_FLAGS;
 
	SSE2_PSHUFD_XMM_to_XMM(sreg, sreg, 0x00);
	SSE2_PAND_M128_to_XMM(sreg, (uptr)minmax_mask);
	SSE2_POR_M128_to_XMM(sreg, (uptr)&minmax_mask[4]);
	SSE2_PSHUFD_XMM_to_XMM(treg, treg, 0x00);
	SSE2_PAND_M128_to_XMM(treg, (uptr)minmax_mask);
	SSE2_POR_M128_to_XMM(treg, (uptr)&minmax_mask[4]);
	if (ismin)
		SSE2_MINSD_XMM_to_XMM(sreg, treg);
	else
		SSE2_MAXSD_XMM_to_XMM(sreg, treg);

	SSE_MOVSS_XMM_to_XMM(EEREC_D, sreg);
 
	_freeXMMreg(sreg); _freeXMMreg(treg);
}

void recMAX_S_xmm(int info)
{
	recMINMAX(info, false);
}
 
FPURECOMPILE_CONSTCODE(MAX_S, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);
 
void recMIN_S_xmm(int info)
{
	recMINMAX(info, true);
}
 
FPURECOMPILE_CONSTCODE(MIN_S, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);
//------------------------------------------------------------------
 
 
//------------------------------------------------------------------
// MOV XMM
//------------------------------------------------------------------
void recMOV_S_xmm(int info)
{
	GET_S(EEREC_D);
}
 
FPURECOMPILE_CONSTCODE(MOV_S, XMMINFO_WRITED|XMMINFO_READS);
//------------------------------------------------------------------
 
 
//------------------------------------------------------------------
// MSUB XMM
//------------------------------------------------------------------
 
void recMSUB_S_xmm(int info)
{
	recMaddsub(info, EEREC_D, 1, false);
}
 
FPURECOMPILE_CONSTCODE(MSUB_S, XMMINFO_WRITED|XMMINFO_READACC|XMMINFO_READS|XMMINFO_READT);
 
void recMSUBA_S_xmm(int info)
{
	recMaddsub(info, EEREC_ACC, 1, true);
}
 
FPURECOMPILE_CONSTCODE(MSUBA_S, XMMINFO_WRITEACC|XMMINFO_READACC|XMMINFO_READS|XMMINFO_READT);
//------------------------------------------------------------------

//------------------------------------------------------------------
// MUL XMM
//------------------------------------------------------------------
void recMUL_S_xmm(int info)
{			
	int sreg, treg;
	ALLOC_S(sreg); ALLOC_T(treg);

	FPU_MUL(info, EEREC_D, sreg, treg, false);
	_freeXMMreg(sreg); _freeXMMreg(treg); 
}
 
FPURECOMPILE_CONSTCODE(MUL_S, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);
 
void recMULA_S_xmm(int info) 
{ 
	int sreg, treg;
	ALLOC_S(sreg); ALLOC_T(treg);

	FPU_MUL(info, EEREC_ACC, sreg, treg, true);
	_freeXMMreg(sreg); _freeXMMreg(treg);
}
 
FPURECOMPILE_CONSTCODE(MULA_S, XMMINFO_WRITEACC|XMMINFO_READS|XMMINFO_READT);
//------------------------------------------------------------------
 
 
//------------------------------------------------------------------
// NEG XMM
//------------------------------------------------------------------
void recNEG_S_xmm(int info) {
 
	GET_S(EEREC_D);
 
	CLEAR_OU_FLAGS;
 
	SSE_XORPS_M128_to_XMM(EEREC_D, (uptr)&s_const.neg[0]);
}
 
FPURECOMPILE_CONSTCODE(NEG_S, XMMINFO_WRITED|XMMINFO_READS);
//------------------------------------------------------------------
 
 
//------------------------------------------------------------------
// SUB XMM
//------------------------------------------------------------------
 
void recSUB_S_xmm(int info)
{
	recFPUOp(info, EEREC_D, 1, false);
}
 
FPURECOMPILE_CONSTCODE(SUB_S, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);
 
 
void recSUBA_S_xmm(int info) 
{ 
	recFPUOp(info, EEREC_ACC, 1, true);
}
 
FPURECOMPILE_CONSTCODE(SUBA_S, XMMINFO_WRITEACC|XMMINFO_READS|XMMINFO_READT);
//------------------------------------------------------------------
 
 
//------------------------------------------------------------------
// SQRT XMM
//------------------------------------------------------------------
void recSQRT_S_xmm(int info)
{
	u8 *pjmp;
	int roundmodeFlag = 0;
	int tempReg = _allocX86reg(-1, X86TYPE_TEMP, 0, 0);
	if (tempReg == -1) {Console.Error("FPU: SQRT Allocation Error!"); tempReg = EAX;}
	int t1reg = _allocTempXMMreg(XMMT_FPS, -1);
	if (t1reg == -1) {Console.Error("FPU: SQRT Allocation Error!");}
	//Console.WriteLn("FPU: SQRT");
 
	if (g_sseMXCSR.GetRoundMode() != SSEround_Nearest)
	{
		// Set roundmode to nearest if it isn't already
		//Console.WriteLn("sqrt to nearest");
		roundmode_nearest = g_sseMXCSR;
		roundmode_nearest.SetRoundMode( SSEround_Nearest );
		xLDMXCSR (roundmode_nearest);
		roundmodeFlag = 1;
	}
 
	GET_T(EEREC_D);
 
	if (FPU_FLAGS_ID) {
		AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagI|FPUflagD)); // Clear I and D flags
  
		//--- Check for negative SQRT --- (sqrt(-0) = 0, unlike what the docs say)
		SSE_MOVMSKPS_XMM_to_R32(tempReg, EEREC_D);
		AND32ItoR(tempReg, 1);  //Check sign
		pjmp = JZ8(0); //Skip if none are
			OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagI|FPUflagSI); // Set I and SI flags
			SSE_ANDPS_M128_to_XMM(EEREC_D, (uptr)&s_const.pos[0]); // Make EEREC_D Positive
		x86SetJ8(pjmp);
	}
	else 
	{
		SSE_ANDPS_M128_to_XMM(EEREC_D, (uptr)&s_const.pos[0]); // Make EEREC_D Positive
	}
 
 
	ToDouble(EEREC_D);
 
	SSE2_SQRTSD_XMM_to_XMM(EEREC_D, EEREC_D);
 
	ToPS2FPU(EEREC_D, false, t1reg, false);
  
	if (roundmodeFlag == 1) {
		xLDMXCSR (g_sseMXCSR);
	}

	_freeX86reg(tempReg);
	_freeXMMreg(t1reg);
}
 
FPURECOMPILE_CONSTCODE(SQRT_S, XMMINFO_WRITED|XMMINFO_READT);
//------------------------------------------------------------------
 
 
//------------------------------------------------------------------
// RSQRT XMM
//------------------------------------------------------------------
void recRSQRThelper1(int regd, int regt) // Preforms the RSQRT function when regd <- Fs and regt <- Ft (Sets correct flags)
{
	u8 *pjmp1, *pjmp2;
	u8 *qjmp1, *qjmp2;
	u32 *pjmp32;
	int t1reg = _allocTempXMMreg(XMMT_FPS, -1);
	int tempReg = _allocX86reg(-1, X86TYPE_TEMP, 0, 0);
	if (t1reg == -1) {Console.Error("FPU: RSQRT Allocation Error!");}
	if (tempReg == -1) {Console.Error("FPU: RSQRT Allocation Error!"); tempReg = EAX;}
 
	AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagI|FPUflagD)); // Clear I and D flags
 
	//--- (first) Check for negative SQRT ---
	SSE_MOVMSKPS_XMM_to_R32(tempReg, regt);
	AND32ItoR(tempReg, 1);  //Check sign
	pjmp2 = JZ8(0); //Skip if not set
		OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagI|FPUflagSI); // Set I and SI flags
		SSE_ANDPS_M128_to_XMM(regt, (uptr)&s_const.pos[0]); // Make regt Positive
	x86SetJ8(pjmp2);
 
	//--- Check for zero ---
	SSE_XORPS_XMM_to_XMM(t1reg, t1reg);
	SSE_CMPEQSS_XMM_to_XMM(t1reg, regt);
	SSE_MOVMSKPS_XMM_to_R32(tempReg, t1reg);
	AND32ItoR(tempReg, 1);  //Check sign (if regt == zero, sign will be set)
	pjmp1 = JZ8(0); //Skip if not set
	
		//--- Check for 0/0 ---
		SSE_XORPS_XMM_to_XMM(t1reg, t1reg);
		SSE_CMPEQSS_XMM_to_XMM(t1reg, regd);
		SSE_MOVMSKPS_XMM_to_R32(tempReg, t1reg);
		AND32ItoR(tempReg, 1);  //Check sign (if regd == zero, sign will be set)
		qjmp1 = JZ8(0); //Skip if not set
			OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagI|FPUflagSI); // Set I and SI flags ( 0/0 )
			qjmp2 = JMP8(0);
		x86SetJ8(qjmp1); //x/0 but not 0/0
			OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagD|FPUflagSD); // Set D and SD flags ( x/0 )
		x86SetJ8(qjmp2);

		SetMaxValue(regd); //clamp to max
		pjmp32 = JMP32(0);
	x86SetJ8(pjmp1);
 
	ToDouble(regt); ToDouble(regd);
 
	SSE2_SQRTSD_XMM_to_XMM(regt, regt);
	SSE2_DIVSD_XMM_to_XMM(regd, regt);
 
	ToPS2FPU(regd, false, regt, false);
	x86SetJ32(pjmp32);
 
	_freeXMMreg(t1reg);
	_freeX86reg(tempReg);
}
 
void recRSQRThelper2(int regd, int regt) // Preforms the RSQRT function when regd <- Fs and regt <- Ft (Doesn't set flags)
{
	SSE_ANDPS_M128_to_XMM(regt, (uptr)&s_const.pos[0]); // Make regt Positive
 
	ToDouble(regt); ToDouble(regd);
 
	SSE2_SQRTSD_XMM_to_XMM(regt, regt);
	SSE2_DIVSD_XMM_to_XMM(regd, regt);
 
	ToPS2FPU(regd, false, regt, false);
}
 
void recRSQRT_S_xmm(int info)
{
	int sreg, treg;
 
	bool roundmodeFlag = false;
	if (g_sseMXCSR.GetRoundMode() != SSEround_Nearest)
	{
		// Set roundmode to nearest if it isn't already
		//Console.WriteLn("sqrt to nearest");
		roundmode_nearest = g_sseMXCSR;
		roundmode_nearest.SetRoundMode( SSEround_Nearest );
		xLDMXCSR (roundmode_nearest);
		roundmodeFlag = true;
	}
 
	ALLOC_S(sreg); ALLOC_T(treg);
 
	if (FPU_FLAGS_ID)
		recRSQRThelper1(sreg, treg);
	else
		recRSQRThelper2(sreg, treg);
 
	SSE_MOVSS_XMM_to_XMM(EEREC_D, sreg);
 
	_freeXMMreg(treg); _freeXMMreg(sreg);
 
	if (roundmodeFlag) xLDMXCSR (g_sseMXCSR);
}
 
FPURECOMPILE_CONSTCODE(RSQRT_S, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);
 
 
} } } } }
#endif