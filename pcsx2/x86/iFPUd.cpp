/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
 
#include "PrecompiledHeader.h"
 
#include "Common.h"
#include "R5900OpcodeTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"
#include "iFPU.h"

/* Version of the FPU that emulates an exponent of 0xff and overflow/underflow flags */
 
//set overflow flag (set only if FPU_RESULT is 1)
#define FPU_FLAGS_OVERFLOW 1 
//set underflow flag (set only if FPU_RESULT is 1)
#define FPU_FLAGS_UNDERFLOW 1 
 
//if 1, result is not clamped (MORE correct,	
//but can cause problems due to insuffecient clamping levels in the VUs)
#define FPU_RESULT 1 
 
//also impacts other aspects of DIV/R/SQRT correctness
#define FPU_FLAGS_ID 1 
 
//------------------------------------------------------------------
// Misc...
//------------------------------------------------------------------
//static u32 _mxcsr = 0x7F80;
//static u32 _mxcsrs;
/*static u32 fpucw = 0x007f;
static u32 fpucws = 0;
 
void SaveCW(int type) {
	if (iCWstate & type) return;
 
	if (type == 2) {
//		SSE_STMXCSR((uptr)&_mxcsrs);
//		SSE_LDMXCSR((uptr)&_mxcsr);
	} else {
		FNSTCW( (uptr)&fpucws );
		FLDCW( (uptr)&fpucw );
	}
	iCWstate|= type;
}
 
void LoadCW() {
	if (iCWstate == 0) return;
 
	if (iCWstate & 2) {
		//SSE_LDMXCSR((uptr)&_mxcsrs);
	}
	if (iCWstate & 1) {
		FLDCW( (uptr)&fpucws );
	}
	iCWstate = 0;
}
*/
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
 
static u32 PCSX2_ALIGNED16(s_neg[4]) = { 0x80000000, 0xffffffff, 0xffffffff, 0xffffffff };
static u32 PCSX2_ALIGNED16(s_pos[4]) = { 0x7fffffff, 0xffffffff, 0xffffffff, 0xffffffff };
 
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
// CFC1 / CTC1
//------------------------------------------------------------------
void recCFC1(void)
{
	if ( !_Rt_ || ( (_Fs_ != 0) && (_Fs_ != 31) ) ) return;
 
	_eeOnWriteReg(_Rt_, 1);
 
	MOV32MtoR( EAX, (uptr)&fpuRegs.fprc[ _Fs_ ] );
	_deleteEEreg(_Rt_, 0);
	
	if (_Fs_ == 31)
	{
		AND32ItoR(EAX, 0x0083c078); //remove always-zero bits
		OR32ItoR(EAX,  0x01000001); //set always-one bits
	}
 
	if(EEINST_ISLIVE1(_Rt_)) 
	{
		CDQ( );
		MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
	}
	else 
	{
		EEINST_RESETHASLIVE1(_Rt_);
		MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	}
	
}
 
void recCTC1( void )
{
	if ( _Fs_ != 31 ) return;
 
	if ( GPR_IS_CONST1(_Rt_) ) 
	{
		MOV32ItoM((uptr)&fpuRegs.fprc[ _Fs_ ], g_cpuConstRegs[_Rt_].UL[0]);
	}
	else 
	{
		int mmreg = _checkXMMreg(XMMTYPE_GPRREG, _Rt_, MODE_READ);
 
		if( mmreg >= 0 ) 
		{
			SSEX_MOVD_XMM_to_M32((uptr)&fpuRegs.fprc[ _Fs_ ], mmreg);
		}
 
		else 
		{
			mmreg = _checkMMXreg(MMX_GPR+_Rt_, MODE_READ);
 
			if ( mmreg >= 0 ) 
			{
				MOVDMMXtoM((uptr)&fpuRegs.fprc[ _Fs_ ], mmreg);
				SetMMXstate();
			}
			else 
			{
				_deleteGPRtoXMMreg(_Rt_, 1);
				_deleteMMXreg(MMX_GPR+_Rt_, 1);
 
				MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
				MOV32RtoM( (uptr)&fpuRegs.fprc[ _Fs_ ], EAX );
			}
		}
	}
}
//------------------------------------------------------------------
 
 
//------------------------------------------------------------------
// MFC1
//------------------------------------------------------------------
 
void recMFC1(void) 
{
	int regt, regs;
	if ( ! _Rt_ ) return;
 
	_eeOnWriteReg(_Rt_, 1);
 
	regs = _checkXMMreg(XMMTYPE_FPREG, _Fs_, MODE_READ);
 
	if( regs >= 0 ) 
	{
		_deleteGPRtoXMMreg(_Rt_, 2);
		regt = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_WRITE);
 
		if( regt >= 0 ) 
		{
			SSE2_MOVDQ2Q_XMM_to_MM(regt, regs);
 
			if(EEINST_ISLIVE1(_Rt_)) 
				_signExtendGPRtoMMX(regt, _Rt_, 0);
			else 
				EEINST_RESETHASLIVE1(_Rt_);
		}
		else 
		{
			if(EEINST_ISLIVE1(_Rt_)) 
			{
				_signExtendXMMtoM((uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], regs, 0);
			}
			else 
			{
				EEINST_RESETHASLIVE1(_Rt_);
				SSE_MOVSS_XMM_to_M32((uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], regs);
			}
		}
	}
	else 
	{
		regs = _checkMMXreg(MMX_FPU+_Fs_, MODE_READ);
 
		if( regs >= 0 ) 
		{
			// convert to mmx reg
			mmxregs[regs].reg = MMX_GPR+_Rt_;
			mmxregs[regs].mode |= MODE_READ|MODE_WRITE;
			_signExtendGPRtoMMX(regs, _Rt_, 0);
		}
		else 
		{
			regt = _checkXMMreg(XMMTYPE_GPRREG, _Rt_, MODE_READ);
 
			if( regt >= 0 ) 
			{
				if( xmmregs[regt].mode & MODE_WRITE ) 
				{
					SSE_MOVHPS_XMM_to_M64((uptr)&cpuRegs.GPR.r[_Rt_].UL[2], regt);
				}
				xmmregs[regt].inuse = 0;
			}
 
			_deleteEEreg(_Rt_, 0);
			MOV32MtoR( EAX, (uptr)&fpuRegs.fpr[ _Fs_ ].UL );
 
			if(EEINST_ISLIVE1(_Rt_)) 
			{
				CDQ( );
				MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
				MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
			}
			else 
			{
				EEINST_RESETHASLIVE1(_Rt_);
				MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
			}
		}
	}
}
 
//------------------------------------------------------------------
 
 
//------------------------------------------------------------------
// MTC1
//------------------------------------------------------------------
void recMTC1(void)
{
	if( GPR_IS_CONST1(_Rt_) ) 
	{
		_deleteFPtoXMMreg(_Fs_, 0);
		MOV32ItoM((uptr)&fpuRegs.fpr[ _Fs_ ].UL, g_cpuConstRegs[_Rt_].UL[0]);
	}
	else 
	{
		int mmreg = _checkXMMreg(XMMTYPE_GPRREG, _Rt_, MODE_READ);
 
		if( mmreg >= 0 ) 
		{
			if( g_pCurInstInfo->regs[_Rt_] & EEINST_LASTUSE ) 
			{
				// transfer the reg directly
				_deleteGPRtoXMMreg(_Rt_, 2);
				_deleteFPtoXMMreg(_Fs_, 2);
				_allocFPtoXMMreg(mmreg, _Fs_, MODE_WRITE);
			}
			else 
			{
				int mmreg2 = _allocCheckFPUtoXMM(g_pCurInstInfo, _Fs_, MODE_WRITE);
 
				if( mmreg2 >= 0 ) 
					SSE_MOVSS_XMM_to_XMM(mmreg2, mmreg);
				else 
					SSE_MOVSS_XMM_to_M32((uptr)&fpuRegs.fpr[ _Fs_ ].UL, mmreg);
			}
		}
		else 
		{
			int mmreg2;
 
			mmreg = _checkMMXreg(MMX_GPR+_Rt_, MODE_READ);
			mmreg2 = _allocCheckFPUtoXMM(g_pCurInstInfo, _Fs_, MODE_WRITE);
 
			if( mmreg >= 0 ) 
			{
				if( mmreg2 >= 0 ) 
				{
					SetMMXstate();
					SSE2_MOVQ2DQ_MM_to_XMM(mmreg2, mmreg);
				}
				else 
				{
					SetMMXstate();
					MOVDMMXtoM((uptr)&fpuRegs.fpr[ _Fs_ ].UL, mmreg);
				}	
			}
			else 
			{
				if( mmreg2 >= 0 ) 
				{
					SSE_MOVSS_M32_to_XMM(mmreg2, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
				}
				else 
				{
					MOV32MtoR(EAX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
					MOV32RtoM((uptr)&fpuRegs.fpr[ _Fs_ ].UL, EAX);
				}
			}
		}
	}
}
//------------------------------------------------------------------
 
 
/*#ifndef FPU_RECOMPILE // If FPU_RECOMPILE is not defined, then use the interpreter opcodes. (CFC1, CTC1, MFC1, and MTC1 are special because they work specifically with the EE rec so they're defined above)
 
REC_FPUFUNC(ABS_S);
REC_FPUFUNC(ADD_S);
REC_FPUFUNC(ADDA_S);
REC_FPUBRANCH(BC1F);
REC_FPUBRANCH(BC1T);
REC_FPUBRANCH(BC1FL);
REC_FPUBRANCH(BC1TL);
REC_FPUFUNC(C_EQ);
REC_FPUFUNC(C_F);
REC_FPUFUNC(C_LE);
REC_FPUFUNC(C_LT);
REC_FPUFUNC(CVT_S);
REC_FPUFUNC(CVT_W);
REC_FPUFUNC(DIV_S);
REC_FPUFUNC(MAX_S);
REC_FPUFUNC(MIN_S);
REC_FPUFUNC(MADD_S);
REC_FPUFUNC(MADDA_S);
REC_FPUFUNC(MOV_S);
REC_FPUFUNC(MSUB_S);
REC_FPUFUNC(MSUBA_S);
REC_FPUFUNC(MUL_S);
REC_FPUFUNC(MULA_S);
REC_FPUFUNC(NEG_S);
REC_FPUFUNC(SUB_S);
REC_FPUFUNC(SUBA_S);
REC_FPUFUNC(SQRT_S);
REC_FPUFUNC(RSQRT_S);
 
#else // FPU_RECOMPILE*/
 
//------------------------------------------------------------------
// PS2 -> DOUBLE
//------------------------------------------------------------------
 
#define SINGLE(sign, exp, mant) (((sign)<<31) | ((exp)<<23) | (mant))
#define DOUBLE(sign, exp, mant) (((sign ## ULL)<<63) | ((exp ## ULL)<<52) | (mant ## ULL))
 
static u32 PCSX2_ALIGNED16(pos_inf[4]) = {SINGLE(0,0xff,0), 0, 0, 0};
static u32 PCSX2_ALIGNED16(neg_inf[4]) = {SINGLE(1,0xff,0), 0, 0, 0};
static u32 PCSX2_ALIGNED16(one_exp[4]) = {SINGLE(0,1,0), 0, 0, 0};
static u64 PCSX2_ALIGNED16(dbl_one_exp[2]) = {DOUBLE(0,1,0), 0};
 
static u64 PCSX2_ALIGNED16(dbl_cvt_overflow) = DOUBLE(0,1151,0); //needs special code if above or equal
static u64 PCSX2_ALIGNED16(dbl_ps2_overflow) = DOUBLE(0,1152,0); //overflow & clamp if above or equal
static u64 PCSX2_ALIGNED16(dbl_underflow) = DOUBLE(0,897,0); //underflow if below
 
static u64 PCSX2_ALIGNED16(dbl_s_pos[2]) = {0x7fffffffffffffffULL, 0}; 
static u64 PCSX2_ALIGNED16(dbl_s_neg[2]) = {0x8000000000000000ULL, 0}; 
 
// converts small normal numbers to double equivalent
// converts large normal numbers (which represent NaN/inf in IEEE) to double equivalent

//mustn't use EAX/ECX/EDX/x86regs (MUL)
void ToDouble(int reg)
{
	SSE_UCOMISS_M32_to_XMM(reg, (uptr)&pos_inf); //sets ZF if equal or uncomparable
	u8 *to_complex = JE8(0); //complex conversion if positive infinity or NaN
	SSE_UCOMISS_M32_to_XMM(reg, (uptr)&neg_inf); 
	u8 *to_complex2 = JE8(0); //complex conversion if negative infinity
 
	SSE2_CVTSS2SD_XMM_to_XMM(reg, reg); //simply convert
	u8 *end = JMP8(0);
 
	x86SetJ8(to_complex);
	x86SetJ8(to_complex2);
 
	SSE2_PSUBD_M128_to_XMM(reg, (uptr)&one_exp); //lower exponent
	SSE2_CVTSS2SD_XMM_to_XMM(reg, reg); 
	SSE2_PADDQ_M128_to_XMM(reg, (uptr)&dbl_one_exp); //raise exponent
 
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
void ToPS2FPU_Full(int reg, bool flags, int absreg, bool acc)
{
	if (flags)
		AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagO | FPUflagU));
	if (flags && acc)
		AND32ItoM((uptr)&fpuRegs.ACCflag, ~1);

	SSE_MOVAPS_XMM_to_XMM(absreg, reg);
	SSE2_ANDPD_M128_to_XMM(absreg, (uptr)&dbl_s_pos); 
 
	SSE2_UCOMISD_M64_to_XMM(absreg, (uptr)&dbl_cvt_overflow);
	u8 *to_complex = JAE8(0);
 
	SSE2_UCOMISD_M64_to_XMM(absreg, (uptr)&dbl_underflow);
	u8 *to_underflow = JB8(0); 
 
	SSE2_CVTSD2SS_XMM_to_XMM(reg, reg); //simply convert
	u8 *end = JMP8(0);
 
	x86SetJ8(to_complex); 
	SSE2_UCOMISD_M64_to_XMM(absreg, (uptr)&dbl_ps2_overflow);
	u8 *to_overflow = JAE8(0);
 
	SSE2_PSUBQ_M128_to_XMM(reg, (uptr)&dbl_one_exp); //lower exponent
	SSE2_CVTSD2SS_XMM_to_XMM(reg, reg); //convert
	SSE2_PADDD_M128_to_XMM(reg, (uptr)one_exp); //raise exponent
	u8 *end2 = JMP8(0);
 
	x86SetJ8(to_overflow); 
	SSE2_CVTSD2SS_XMM_to_XMM(reg, reg); 
	SSE_ORPS_M128_to_XMM(reg, (uptr)&s_pos); //clamp
	if (flags && FPU_FLAGS_OVERFLOW)
		OR32ItoM((uptr)&fpuRegs.fprc[31], (FPUflagO | FPUflagSO));
	if (flags && FPU_FLAGS_OVERFLOW && acc)
		OR32ItoM((uptr)&fpuRegs.ACCflag, 1);
	u8 *end3 = JMP8(0);
 
	x86SetJ8(to_underflow);
	if (flags && FPU_FLAGS_UNDERFLOW) //set underflow flags if not zero
	{
		SSE2_XORPD_XMM_to_XMM(absreg, absreg);
		SSE2_UCOMISD_XMM_to_XMM(reg, absreg);
		u8 *is_zero = JE8(0);
 
		OR32ItoM((uptr)&fpuRegs.fprc[31], (FPUflagU | FPUflagSU));
 
		x86SetJ8(is_zero);
	}
	SSE2_CVTSD2SS_XMM_to_XMM(reg, reg);
	SSE_ANDPS_M128_to_XMM(reg, (uptr)&s_neg); //flush to zero
 
	x86SetJ8(end);
	x86SetJ8(end2);
	x86SetJ8(end3);
}
 
//mustn't use EAX/ECX/EDX/x86regs (MUL)
void ToPS2FPU(int reg, bool flags, int absreg, bool acc)
{
	if (FPU_RESULT)
		ToPS2FPU_Full(reg, flags, absreg, acc);
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
		SSE_ORPS_M128_to_XMM(regd, (uptr)&s_pos[0]); // set regd to maximum
	else
	{
		SSE_ANDPS_M128_to_XMM(regd, (uptr)&s_neg[0]); // Get the sign bit
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
 
	SSE_ANDPS_M128_to_XMM(EEREC_D, (uptr)&s_pos[0]);
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
 
	if (tempecx != ECX)	{ Console::Error("FPU: ADD/SUB Allocation Error!"); tempecx = ECX;}
	if (temp2 == -1)	{ Console::Error("FPU: ADD/SUB Allocation Error!"); temp2 = EAX;}
	if (xmmtemp == -1)	{ Console::Error("FPU: ADD/SUB Allocation Error!"); xmmtemp = XMM0;}
 
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
	SSE_ANDPS_M128_to_XMM(tempt, (uptr)s_neg);
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
	SSE_ANDPS_M128_to_XMM(tempd, (uptr)s_neg); 
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
	if (CHECK_FPU_ATTEMPT_MUL)
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
// CommutativeOp XMM (used for ADD, MUL, MAX, MIN and SUB opcodes)
//------------------------------------------------------------------
static void (*recFPUOpXMM_to_XMM[] )(x86SSERegType, x86SSERegType) = {
	SSE2_ADDSD_XMM_to_XMM, NULL, SSE2_MAXSD_XMM_to_XMM, SSE2_MINSD_XMM_to_XMM, SSE2_SUBSD_XMM_to_XMM };
 
void recFPUOp(int info, int regd, int op, bool acc) 
{
	int sreg, treg;
	ALLOC_S(sreg); ALLOC_T(treg);
 
	if (FPU_ADD_SUB_HACK && (op == 0 || op == 4)) //ADD or SUB
		FPU_ADD_SUB(sreg, treg);
 
	ToDouble(sreg); ToDouble(treg);
 
	recFPUOpXMM_to_XMM[op](sreg, treg);
 
	ToPS2FPU(sreg, true, treg, acc);
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
 
//------------------------------------------------------------------
// BC1x XMM
//------------------------------------------------------------------
 /*
static void _setupBranchTest()
{
	_eeFlushAllUnused();
 
	// COP1 branch conditionals are based on the following equation:
	// (fpuRegs.fprc[31] & 0x00800000)
	// BC2F checks if the statement is false, BC2T checks if the statement is true.
 
	MOV32MtoR(EAX, (uptr)&fpuRegs.fprc[31]);
	TEST32ItoR(EAX, FPUflagC);
}
 
void recBC1F( void )
{
	_setupBranchTest();
	recDoBranchImm(JNZ32(0));
}
 
void recBC1T( void )
{
	_setupBranchTest();
	recDoBranchImm(JZ32(0));
}
 
void recBC1FL( void )
{
	_setupBranchTest();
	recDoBranchImm_Likely(JNZ32(0));
}
 
void recBC1TL( void )
{
	_setupBranchTest();
	recDoBranchImm_Likely(JZ32(0));
}*/
//------------------------------------------------------------------
 
//TOKNOW : how does C.??.S behave with denormals?
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
 
/*void recC_F()
{
	AND32ItoM( (uptr)&fpuRegs.fprc[31], ~FPUflagC );
}*/
 
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
//REC_FPUFUNC(C_LE);
 
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
//REC_FPUFUNC(C_LT);
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
 
void recCVT_W() 
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
	if (t1reg == -1) {Console::Error("FPU: DIV Allocation Error!");}
	if (tempReg == -1) {Console::Error("FPU: DIV Allocation Error!"); tempReg = EAX;}
 
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
 
void recDIV_S_xmm(int info)
{
	static u32 PCSX2_ALIGNED16(roundmode_temp[4]) = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
	int roundmodeFlag = 0;
    //if (t0reg == -1) {Console::Error("FPU: DIV Allocation Error!");}
    //SysPrintf("DIV\n");
 
	if ((g_sseMXCSR & 0x00006000) != 0x00000000) { // Set roundmode to nearest if it isn't already
		//SysPrintf("div to nearest\n");
		roundmode_temp[0] = (g_sseMXCSR & 0xFFFF9FFF); // Set new roundmode
		roundmode_temp[1] = g_sseMXCSR; // Backup old Roundmode
		SSE_LDMXCSR ((uptr)&roundmode_temp[0]); // Recompile Roundmode Change
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
		SSE_LDMXCSR ((uptr)&roundmode_temp[1]);
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
		SSE_XORPS_M128_to_XMM(sreg, (uptr)&s_neg);
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

	ToPS2FPU(treg, true, sreg, acc);
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
 
//TOKNOW : handles denormals like VU, maybe?
void recMAX_S_xmm(int info)
{
    recFPUOp(info, EEREC_D, 2, false);
}
 
FPURECOMPILE_CONSTCODE(MAX_S, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);
 
void recMIN_S_xmm(int info)
{
    recFPUOp(info, EEREC_D, 3, false);
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
 
	SSE_XORPS_M128_to_XMM(EEREC_D, (uptr)&s_neg[0]);
}
 
FPURECOMPILE_CONSTCODE(NEG_S, XMMINFO_WRITED|XMMINFO_READS);
//------------------------------------------------------------------
 
 
//------------------------------------------------------------------
// SUB XMM
//------------------------------------------------------------------
 
void recSUB_S_xmm(int info)
{
	recFPUOp(info, EEREC_D, 4, false);
}
 
FPURECOMPILE_CONSTCODE(SUB_S, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);
 
 
void recSUBA_S_xmm(int info) 
{ 
	recFPUOp(info, EEREC_ACC, 4, true);
}
 
FPURECOMPILE_CONSTCODE(SUBA_S, XMMINFO_WRITEACC|XMMINFO_READS|XMMINFO_READT);
//------------------------------------------------------------------
 
 
//------------------------------------------------------------------
// SQRT XMM
//------------------------------------------------------------------
void recSQRT_S_xmm(int info)
{
	u8 *pjmp;
	u32 *pjmpx;
	static u32 PCSX2_ALIGNED16(roundmode_temp[4]) = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
	int roundmodeFlag = 0;
	int tempReg = _allocX86reg(-1, X86TYPE_TEMP, 0, 0);
	if (tempReg == -1) {Console::Error("FPU: SQRT Allocation Error!"); tempReg = EAX;}
	int t1reg = _allocTempXMMreg(XMMT_FPS, -1);
	if (t1reg == -1) {Console::Error("FPU: SQRT Allocation Error!");}
	//SysPrintf("FPU: SQRT\n");
 
	if ((g_sseMXCSR & 0x00006000) != 0x00000000) { // Set roundmode to nearest if it isn't already
		//SysPrintf("sqrt to nearest\n");
		roundmode_temp[0] = (g_sseMXCSR & 0xFFFF9FFF); // Set new roundmode
		roundmode_temp[1] = g_sseMXCSR; // Backup old Roundmode
		SSE_LDMXCSR ((uptr)&roundmode_temp[0]); // Recompile Roundmode Change
		roundmodeFlag = 1;
	}
 
	GET_T(EEREC_D);
 
	if (FPU_FLAGS_ID) {
		AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagI|FPUflagD)); // Clear I and D flags
 
		//--- Check for zero (skip sqrt if zero)
		SSE_XORPS_XMM_to_XMM(t1reg, t1reg);
		SSE_CMPEQSS_XMM_to_XMM(t1reg, EEREC_D);
		SSE_MOVMSKPS_XMM_to_R32(tempReg, t1reg);
		AND32ItoR(tempReg, 1); 
		pjmpx = JNE32(0);
 
		//--- Check for negative SQRT ---
		SSE_MOVMSKPS_XMM_to_R32(tempReg, EEREC_D);
		AND32ItoR(tempReg, 1);  //Check sign
		pjmp = JZ8(0); //Skip if none are
			OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagI|FPUflagSI); // Set I and SI flags
			SSE_ANDPS_M128_to_XMM(EEREC_D, (uptr)&s_pos[0]); // Make EEREC_D Positive
		x86SetJ8(pjmp);
	}
	else 
	{
		SSE_ANDPS_M128_to_XMM(EEREC_D, (uptr)&s_pos[0]); // Make EEREC_D Positive
	}
 
 
	ToDouble(EEREC_D);
 
	SSE2_SQRTSD_XMM_to_XMM(EEREC_D, EEREC_D);
 
	ToPS2FPU(EEREC_D, false, t1reg, false);
 
	x86SetJ32(pjmpx);
 
	if (roundmodeFlag == 1) { // Set roundmode back if it was changed
		SSE_LDMXCSR ((uptr)&roundmode_temp[1]);
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
	u32 *pjmp32;
	int t1reg = _allocTempXMMreg(XMMT_FPS, -1);
	int tempReg = _allocX86reg(-1, X86TYPE_TEMP, 0, 0);
	if (t1reg == -1) {Console::Error("FPU: RSQRT Allocation Error!");}
	if (tempReg == -1) {Console::Error("FPU: RSQRT Allocation Error!"); tempReg = EAX;}
 
	AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagI|FPUflagD)); // Clear I and D flags
 
	//--- Check for zero ---
	SSE_XORPS_XMM_to_XMM(t1reg, t1reg);
	SSE_CMPEQSS_XMM_to_XMM(t1reg, regt);
	SSE_MOVMSKPS_XMM_to_R32(tempReg, t1reg);
	AND32ItoR(tempReg, 1);  //Check sign (if regt == zero, sign will be set)
	pjmp1 = JZ8(0); //Skip if not set
		OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagD|FPUflagSD); // Set D and SD flags (even when 0/0)
		SSE_XORPS_XMM_to_XMM(regd, regt); // Make regd Positive or Negative
		SetMaxValue(regd); //clamp to max
		pjmp32 = JMP32(0);
	x86SetJ8(pjmp1);
 
	//--- Check for negative SQRT ---
	SSE_MOVMSKPS_XMM_to_R32(tempReg, regt);
	AND32ItoR(tempReg, 1);  //Check sign
	pjmp2 = JZ8(0); //Skip if not set
		OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagI|FPUflagSI); // Set I and SI flags
		SSE_ANDPS_M128_to_XMM(regt, (uptr)&s_pos[0]); // Make regt Positive
	x86SetJ8(pjmp2);
 
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
	SSE_ANDPS_M128_to_XMM(regt, (uptr)&s_pos[0]); // Make regt Positive
 
	ToDouble(regt); ToDouble(regd);
 
	SSE2_SQRTSD_XMM_to_XMM(regt, regt);
	SSE2_DIVSD_XMM_to_XMM(regd, regt);
 
	ToPS2FPU(regd, false, regt, false);
}
 
void recRSQRT_S_xmm(int info)
{
	int sreg, treg;
 
	static u32 PCSX2_ALIGNED16(roundmode_temp[4]) = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
	int roundmodeFlag = 0;
	if ((g_sseMXCSR & 0x00006000) != 0x00000000) { // Set roundmode to nearest if it isn't already
		//SysPrintf("rsqrt to nearest\n");
		roundmode_temp[0] = (g_sseMXCSR & 0xFFFF9FFF); // Set new roundmode
		roundmode_temp[1] = g_sseMXCSR; // Backup old Roundmode
		SSE_LDMXCSR ((uptr)&roundmode_temp[0]); // Recompile Roundmode Change
		roundmodeFlag = 1;
	}
 
	ALLOC_S(sreg); ALLOC_T(treg);
 
	if (FPU_FLAGS_ID)
		recRSQRThelper1(sreg, treg);
	else
		recRSQRThelper2(sreg, treg);
 
	SSE_MOVSS_XMM_to_XMM(EEREC_D, sreg);
 
	_freeXMMreg(treg); _freeXMMreg(sreg);
 
	if (roundmodeFlag == 1) { // Set roundmode back if it was changed
		SSE_LDMXCSR ((uptr)&roundmode_temp[1]);
	}
}
 
FPURECOMPILE_CONSTCODE(RSQRT_S, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);
 
//#endif // FPU_RECOMPILE
 
} } } } }