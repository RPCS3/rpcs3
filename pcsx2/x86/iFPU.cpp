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
#include "iR5900.h"
#include "iFPU.h"

using namespace x86Emitter;

//------------------------------------------------------------------
namespace R5900 {
namespace Dynarec {
namespace OpcodeImpl {
namespace COP1 {

namespace DOUBLE {

void recABS_S_xmm(int info);
void recADD_S_xmm(int info);
void recADDA_S_xmm(int info);
void recC_EQ_xmm(int info);
void recC_LE_xmm(int info);
void recC_LT_xmm(int info);
void recCVT_S_xmm(int info);
void recCVT_W();
void recDIV_S_xmm(int info);
void recMADD_S_xmm(int info);
void recMADDA_S_xmm(int info);
void recMAX_S_xmm(int info);
void recMIN_S_xmm(int info);
void recMOV_S_xmm(int info);
void recMSUB_S_xmm(int info);
void recMSUBA_S_xmm(int info);
void recMUL_S_xmm(int info);
void recMULA_S_xmm(int info);
void recNEG_S_xmm(int info);
void recSUB_S_xmm(int info);
void recSUBA_S_xmm(int info);
void recSQRT_S_xmm(int info);
void recRSQRT_S_xmm(int info);

};

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

static const __aligned16 u32 s_neg[4] = { 0x80000000, 0xffffffff, 0xffffffff, 0xffffffff };
static const __aligned16 u32 s_pos[4] = { 0x7fffffff, 0xffffffff, 0xffffffff, 0xffffffff };

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


#ifndef FPU_RECOMPILE // If FPU_RECOMPILE is not defined, then use the interpreter opcodes. (CFC1, CTC1, MFC1, and MTC1 are special because they work specifically with the EE rec so they're defined above)

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

#else // FPU_RECOMPILE

//------------------------------------------------------------------
// Clamp Functions (Converts NaN's and Infinities to Normal Numbers)
//------------------------------------------------------------------

static __aligned16 u64 FPU_FLOAT_TEMP[2];
__forceinline void fpuFloat4(int regd) { // +NaN -> +fMax, -NaN -> -fMax, +Inf -> +fMax, -Inf -> -fMax
	int t1reg = _allocTempXMMreg(XMMT_FPS, -1);
	if (t1reg >= 0) {
		SSE_MOVSS_XMM_to_XMM(t1reg, regd);
		SSE_ANDPS_M128_to_XMM(t1reg, (uptr)&s_neg[0]);
		SSE_MINSS_M32_to_XMM(regd, (uptr)&g_maxvals[0]);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)&g_minvals[0]);
		SSE_ORPS_XMM_to_XMM(regd, t1reg);
		_freeXMMreg(t1reg);
	}
	else {
		Console.Error("fpuFloat2() allocation error"); 
		t1reg = (regd == 0) ? 1 : 0; // get a temp reg thats not regd
		SSE_MOVAPS_XMM_to_M128( (uptr)&FPU_FLOAT_TEMP[0], t1reg ); // backup data in t1reg to a temp address
		SSE_MOVSS_XMM_to_XMM(t1reg, regd);
		SSE_ANDPS_M128_to_XMM(t1reg, (uptr)&s_neg[0]);
		SSE_MINSS_M32_to_XMM(regd, (uptr)&g_maxvals[0]);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)&g_minvals[0]);
		SSE_ORPS_XMM_to_XMM(regd, t1reg);
		SSE_MOVAPS_M128_to_XMM( t1reg, (uptr)&FPU_FLOAT_TEMP[0] ); // restore t1reg data
	}
}

__forceinline void fpuFloat(int regd) {  // +/-NaN -> +fMax, +Inf -> +fMax, -Inf -> -fMax
	if (CHECK_FPU_OVERFLOW) {
		SSE_MINSS_M32_to_XMM(regd, (uptr)&g_maxvals[0]); // MIN() must be before MAX()! So that NaN's become +Maximum
		SSE_MAXSS_M32_to_XMM(regd, (uptr)&g_minvals[0]);
	}
}

__forceinline void fpuFloat2(int regd) { // +NaN -> +fMax, -NaN -> -fMax, +Inf -> +fMax, -Inf -> -fMax
	if (CHECK_FPU_OVERFLOW) {
		fpuFloat4(regd);
	}
}

__forceinline void fpuFloat3(int regd) {
	// This clamp function is used in the recC_xx opcodes
	// Rule of Rose needs clamping or else it crashes (minss or maxss both fix the crash)
	// Tekken 5 has disappearing characters unless preserving NaN sign (fpuFloat4() preserves NaN sign).
	// Digimon Rumble Arena 2 needs MAXSS clamping (if you only use minss, it spins on the intro-menus; 
	// it also doesn't like preserving NaN sign with fpuFloat4, so the only way to make Digimon work
	// is by calling MAXSS first)
	if (CHECK_FPUCOMPAREHACK) {
		//SSE_MINSS_M32_to_XMM(regd, (uptr)&g_maxvals[0]);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)&g_minvals[0]);
	}
	else fpuFloat4(regd);
}

void ClampValues(int regd) { 
	fpuFloat(regd);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ABS XMM
//------------------------------------------------------------------
void recABS_S_xmm(int info)
{	
	if( info & PROCESS_EE_S ) SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
	else SSE_MOVSS_M32_to_XMM(EEREC_D, (uptr)&fpuRegs.fpr[_Fs_]);

	SSE_ANDPS_M128_to_XMM(EEREC_D, (uptr)&s_pos[0]);
	//AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagO|FPUflagU)); // Clear O and U flags

	if (CHECK_FPU_OVERFLOW) // Only need to do positive clamp, since EEREC_D is positive
		SSE_MINSS_M32_to_XMM(EEREC_D, (uptr)&g_maxvals[0]);
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
void FPU_ADD_SUB(int regd, int regt, int issub)
{
	int tempecx = _allocX86reg(ECX, X86TYPE_TEMP, 0, 0); //receives regd
	int temp2 = _allocX86reg(-1, X86TYPE_TEMP, 0, 0); //receives regt
	int xmmtemp = _allocTempXMMreg(XMMT_FPS, -1); //temporary for anding with regd/regt
 
	if (tempecx != ECX)	{ Console.Error("FPU: ADD/SUB Allocation Error!"); tempecx = ECX;}
	if (temp2 == -1)	{ Console.Error("FPU: ADD/SUB Allocation Error!"); temp2 = EAX;}
	if (xmmtemp == -1)	{ Console.Error("FPU: ADD/SUB Allocation Error!"); xmmtemp = XMM0;}

	SSE2_MOVD_XMM_to_R(tempecx, regd); 
	SSE2_MOVD_XMM_to_R(temp2, regt);
 
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
	SSE_ANDPS_XMM_to_XMM(regd, xmmtemp); 
	if (issub)
		SSE_SUBSS_XMM_to_XMM(regd, regt);
	else
		SSE_ADDSS_XMM_to_XMM(regd, regt);
	j8Ptr[4] = JMP8(0);
 
	x86SetJ8(j8Ptr[0]);
	//diff = 25 .. 255 , expt < expd
	SSE_MOVAPS_XMM_to_XMM(xmmtemp, regt);
	SSE_ANDPS_M128_to_XMM(xmmtemp, (uptr)s_neg);
	if (issub)
		SSE_SUBSS_XMM_to_XMM(regd, xmmtemp);
	else
		SSE_ADDSS_XMM_to_XMM(regd, xmmtemp);
	j8Ptr[5] = JMP8(0);
 
	x86SetJ8(j8Ptr[1]);
	//diff = 1 .. 24, expt < expd
	DEC32R(tempecx);
	MOV32ItoR(temp2, 0xffffffff);
	SHL32CLtoR(temp2); //temp2 = 0xffffffff << tempecx
	SSE2_MOVD_R_to_XMM(xmmtemp, temp2);
	SSE_ANDPS_XMM_to_XMM(xmmtemp, regt);
	if (issub)
		SSE_SUBSS_XMM_to_XMM(regd, xmmtemp);
	else
		SSE_ADDSS_XMM_to_XMM(regd, xmmtemp);
	j8Ptr[6] = JMP8(0);
 
	x86SetJ8(j8Ptr[3]);
	//diff = -255 .. -25, expd < expt
	SSE_ANDPS_M128_to_XMM(regd, (uptr)s_neg); 
	if (issub)
		SSE_SUBSS_XMM_to_XMM(regd, regt);
	else
		SSE_ADDSS_XMM_to_XMM(regd, regt);
	j8Ptr[7] = JMP8(0);
 
	x86SetJ8(j8Ptr[2]);
	//diff == 0
	if (issub)
		SSE_SUBSS_XMM_to_XMM(regd, regt);
	else
		SSE_ADDSS_XMM_to_XMM(regd, regt);
 
	x86SetJ8(j8Ptr[4]);
	x86SetJ8(j8Ptr[5]);
	x86SetJ8(j8Ptr[6]);
	x86SetJ8(j8Ptr[7]);

	_freeXMMreg(xmmtemp);
	_freeX86reg(temp2);
	_freeX86reg(tempecx);
}

void FPU_ADD(int regd, int regt) {
	if (FPU_ADD_SUB_HACK) FPU_ADD_SUB(regd, regt, 0);
	else SSE_ADDSS_XMM_to_XMM(regd, regt);
}
 
void FPU_SUB(int regd, int regt) {
	if (FPU_ADD_SUB_HACK) FPU_ADD_SUB(regd, regt, 1);
	else SSE_SUBSS_XMM_to_XMM(regd, regt);
}

//------------------------------------------------------------------
// FPU_MUL (Used to approximate PS2's FPU mul behavior)
//------------------------------------------------------------------
// PS2's multiplication uses some modification (possibly not the one used in this function)
// of booth multiplication with wallace trees (not used in this function)
// it cuts of some bits, resulting in inaccurate and non-commutative results.
// This function attempts to replicate this. It is currently inaccurate. But still not too bad.
//------------------------------------------------------------------
// Tales of Destiny hangs in a (very) certain place without this function. Probably its only use.
// Can be optimized, of course. 
// shouldn't be compiled with SSE/MMX optimizations (but none of PCSX2 should be, right?)
u32 __fastcall FPU_MUL_MANTISSA(u32 s, u32 t)
{
	s = (s & 0x7fffff) | 0x800000;
	t = (t & 0x7fffff) | 0x800000;
	t<<=1;
	u32 part[13]; //partial products
	u32 bit[13]; //more partial products. 0 or 1.
	for (int i = 0; i <= 12; i++, t>>=2)
	{
		u32 test = t & 7;
		if (test == 0 || test == 7)
		{
			part[i] = 0;
			bit[i] = 0;
		}
		else if (test == 3)
		{
			part[i] = (s<<1);
			bit[i] = 0;
		}
		else if (test == 4)
		{
			part[i] = ~(s<<1);
			bit[i] = 1;
		}
		else if (test < 4)
		{
			part[i] = s;
			bit[i] = 0;
		}
		else
		{
			part[i] = ~s;
			bit[i] = 1;
		}
	}
	s64 res = 0;
	u64 mask = 0;
	mask = (~mask) << 12; //mask
	for (int i=0; i<=12; i++)
	{
		res += (s64)(s32)part[i]<<(i*2);
		res &= mask;
		res += bit[i]<<(i*2);
	}
	u32 man_res = (res >> 23);
	if (man_res & (1 << 24))
		man_res >>= 1;
	man_res &= 0x7fffff;
	return man_res;
}

void FPU_MUL(int regd, int regt)
{
	if (CHECK_FPUMULHACK)
	{
		SSE2_MOVD_XMM_to_R(ECX, regd);
		SSE2_MOVD_XMM_to_R(EDX, regt);
		SSE_MULSS_XMM_to_XMM(regd, regt);
		CALLFunc( (uptr)&FPU_MUL_MANTISSA );
		SSE2_MOVD_XMM_to_R(ECX, regd);
		AND32ItoR(ECX, 0xff800000);
		OR32RtoR(EAX, ECX);
		SSE2_MOVD_R_to_XMM(regd, EAX);
	}
	else
		SSE_MULSS_XMM_to_XMM(regd, regt);
}


//------------------------------------------------------------------
// CommutativeOp XMM (used for ADD, MUL, MAX, and MIN opcodes)
//------------------------------------------------------------------
static void (*recComOpXMM_to_XMM[] )(x86SSERegType, x86SSERegType) = {
	FPU_ADD, FPU_MUL, SSE_MAXSS_XMM_to_XMM, SSE_MINSS_XMM_to_XMM };

//static void (*recComOpM32_to_XMM[] )(x86SSERegType, uptr) = {
//	SSE_ADDSS_M32_to_XMM, SSE_MULSS_M32_to_XMM, SSE_MAXSS_M32_to_XMM, SSE_MINSS_M32_to_XMM };

int recCommutativeOp(int info, int regd, int op) 
{
	int t0reg = _allocTempXMMreg(XMMT_FPS, -1);
    //if (t0reg == -1) {Console.WriteLn("FPU: CommutativeOp Allocation Error!");}

	switch(info & (PROCESS_EE_S|PROCESS_EE_T) ) {
		case PROCESS_EE_S:
			if (regd == EEREC_S) {
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Ft_]);
				if (CHECK_FPU_EXTRA_OVERFLOW  /*&& !CHECK_FPUCLAMPHACK */ || (op >= 2)) { fpuFloat2(regd); fpuFloat2(t0reg); }
				recComOpXMM_to_XMM[op](regd, t0reg);
			}
			else {
				SSE_MOVSS_M32_to_XMM(regd, (uptr)&fpuRegs.fpr[_Ft_]);
				if (CHECK_FPU_EXTRA_OVERFLOW || (op >= 2)) { fpuFloat2(regd); fpuFloat2(EEREC_S); }
				recComOpXMM_to_XMM[op](regd, EEREC_S);
			}
			break;
		case PROCESS_EE_T:
			if (regd == EEREC_T) {
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Fs_]);
				if (CHECK_FPU_EXTRA_OVERFLOW || (op >= 2)) { fpuFloat2(regd); fpuFloat2(t0reg); }
				recComOpXMM_to_XMM[op](regd, t0reg);
			}
			else {
				SSE_MOVSS_M32_to_XMM(regd, (uptr)&fpuRegs.fpr[_Fs_]);
				if (CHECK_FPU_EXTRA_OVERFLOW || (op >= 2)) { fpuFloat2(regd); fpuFloat2(EEREC_T); }
				recComOpXMM_to_XMM[op](regd, EEREC_T);
			}
			break;
		case (PROCESS_EE_S|PROCESS_EE_T):
			if (regd == EEREC_T) {
				if (CHECK_FPU_EXTRA_OVERFLOW || (op >= 2)) { fpuFloat2(regd); fpuFloat2(EEREC_S); }
				recComOpXMM_to_XMM[op](regd, EEREC_S);
			}
			else {
				SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
				if (CHECK_FPU_EXTRA_OVERFLOW || (op >= 2)) { fpuFloat2(regd); fpuFloat2(EEREC_T); }
				recComOpXMM_to_XMM[op](regd, EEREC_T);
			}
			break;
		default:
			Console.WriteLn(Color_Magenta, "FPU: recCommutativeOp case 4");
			SSE_MOVSS_M32_to_XMM(regd, (uptr)&fpuRegs.fpr[_Fs_]);
			SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Ft_]);
			if (CHECK_FPU_EXTRA_OVERFLOW || (op >= 2)) { fpuFloat2(regd); fpuFloat2(t0reg); }
			recComOpXMM_to_XMM[op](regd, t0reg);
			break;
	}

	_freeXMMreg(t0reg);
	return regd;
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ADD XMM
//------------------------------------------------------------------
void recADD_S_xmm(int info)
{
	//AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagO|FPUflagU)); // Clear O and U flags
    ClampValues(recCommutativeOp(info, EEREC_D, 0));
	//REC_FPUOP(ADD_S);
}

FPURECOMPILE_CONSTCODE(ADD_S, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);

void recADDA_S_xmm(int info)
{
	//AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagO|FPUflagU)); // Clear O and U flags
    ClampValues(recCommutativeOp(info, EEREC_ACC, 0));
}

FPURECOMPILE_CONSTCODE(ADDA_S, XMMINFO_WRITEACC|XMMINFO_READS|XMMINFO_READT);
//------------------------------------------------------------------

//------------------------------------------------------------------
// BC1x XMM
//------------------------------------------------------------------

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
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// C.x.S XMM
//------------------------------------------------------------------
void recC_EQ_xmm(int info)
{
	int tempReg;
	int t0reg;

	//Console.WriteLn("recC_EQ_xmm()");

	switch(info & (PROCESS_EE_S|PROCESS_EE_T) ) {
		case PROCESS_EE_S: 
			fpuFloat3(EEREC_S);
			t0reg = _allocTempXMMreg(XMMT_FPS, -1);
			if (t0reg >= 0) {
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Ft_]); 
				fpuFloat3(t0reg);
				SSE_UCOMISS_XMM_to_XMM(EEREC_S, t0reg); 
				_freeXMMreg(t0reg);
			}
			else SSE_UCOMISS_M32_to_XMM(EEREC_S, (uptr)&fpuRegs.fpr[_Ft_]); 
			break;
		case PROCESS_EE_T: 
			fpuFloat3(EEREC_T);
			t0reg = _allocTempXMMreg(XMMT_FPS, -1);
			if (t0reg >= 0) {
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Fs_]); 
				fpuFloat3(t0reg);
				SSE_UCOMISS_XMM_to_XMM(t0reg, EEREC_T); 
				_freeXMMreg(t0reg);
			}
			else SSE_UCOMISS_M32_to_XMM(EEREC_T, (uptr)&fpuRegs.fpr[_Fs_]);
			break;
		case (PROCESS_EE_S|PROCESS_EE_T): 
			fpuFloat3(EEREC_S);
			fpuFloat3(EEREC_T);
			SSE_UCOMISS_XMM_to_XMM(EEREC_S, EEREC_T); 
			break;
		default: 
			Console.WriteLn(Color_Magenta, "recC_EQ_xmm: Default");
			tempReg = _allocX86reg(-1, X86TYPE_TEMP, 0, 0);
			if (tempReg < 0) {Console.Error("FPU: DIV Allocation Error!"); tempReg = EAX;}
			MOV32MtoR(tempReg, (uptr)&fpuRegs.fpr[_Fs_]);
			CMP32MtoR(tempReg, (uptr)&fpuRegs.fpr[_Ft_]); 

			j8Ptr[0] = JZ8(0);
				AND32ItoM( (uptr)&fpuRegs.fprc[31], ~FPUflagC );
				j8Ptr[1] = JMP8(0);
			x86SetJ8(j8Ptr[0]);
				OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagC);
			x86SetJ8(j8Ptr[1]);

			if (tempReg >= 0) _freeX86reg(tempReg);
			return;
	}

	j8Ptr[0] = JZ8(0);
		AND32ItoM( (uptr)&fpuRegs.fprc[31], ~FPUflagC );
		j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);
		OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagC);
	x86SetJ8(j8Ptr[1]);
}

FPURECOMPILE_CONSTCODE(C_EQ, XMMINFO_READS|XMMINFO_READT);
//REC_FPUFUNC(C_EQ);

void recC_F()
{
	AND32ItoM( (uptr)&fpuRegs.fprc[31], ~FPUflagC );
}
//REC_FPUFUNC(C_F);

void recC_LE_xmm(int info )
{
	int tempReg; //tempX86reg
	int t0reg; //tempXMMreg

	//Console.WriteLn("recC_LE_xmm()");

	switch(info & (PROCESS_EE_S|PROCESS_EE_T) ) {
		case PROCESS_EE_S: 
			fpuFloat3(EEREC_S);
			t0reg = _allocTempXMMreg(XMMT_FPS, -1);
			if (t0reg >= 0) {
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Ft_]); 
				fpuFloat3(t0reg);
				SSE_UCOMISS_XMM_to_XMM(EEREC_S, t0reg); 
				_freeXMMreg(t0reg);
			}
			else SSE_UCOMISS_M32_to_XMM(EEREC_S, (uptr)&fpuRegs.fpr[_Ft_]); 
			break;
		case PROCESS_EE_T: 
			fpuFloat3(EEREC_T);
			t0reg = _allocTempXMMreg(XMMT_FPS, -1);
			if (t0reg >= 0) {
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Fs_]); 
				fpuFloat3(t0reg);
				SSE_UCOMISS_XMM_to_XMM(t0reg, EEREC_T); 
				_freeXMMreg(t0reg);
			}
			else {
				SSE_UCOMISS_M32_to_XMM(EEREC_T, (uptr)&fpuRegs.fpr[_Fs_]);

				j8Ptr[0] = JAE8(0);
					AND32ItoM( (uptr)&fpuRegs.fprc[31], ~FPUflagC );
					j8Ptr[1] = JMP8(0);
				x86SetJ8(j8Ptr[0]);
					OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagC);
				x86SetJ8(j8Ptr[1]);
				return;
			}
			break;
		case (PROCESS_EE_S|PROCESS_EE_T):
			fpuFloat3(EEREC_S);
			fpuFloat3(EEREC_T);
			SSE_UCOMISS_XMM_to_XMM(EEREC_S, EEREC_T); 
			break;
		default: // Untested and incorrect, but this case is never reached AFAIK (cottonvibes)
			Console.WriteLn(Color_Magenta, "recC_LE_xmm: Default");
			tempReg = _allocX86reg(-1, X86TYPE_TEMP, 0, 0);
			if (tempReg < 0) {Console.Error("FPU: DIV Allocation Error!"); tempReg = EAX;}
			MOV32MtoR(tempReg, (uptr)&fpuRegs.fpr[_Fs_]);
			CMP32MtoR(tempReg, (uptr)&fpuRegs.fpr[_Ft_]); 
			
			j8Ptr[0] = JLE8(0);
				AND32ItoM( (uptr)&fpuRegs.fprc[31], ~FPUflagC );
				j8Ptr[1] = JMP8(0);
			x86SetJ8(j8Ptr[0]);
				OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagC);
			x86SetJ8(j8Ptr[1]);

			if (tempReg >= 0) _freeX86reg(tempReg);
			return;
	}

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
	int tempReg;
	int t0reg;

	//Console.WriteLn("recC_LT_xmm()");
	
	switch(info & (PROCESS_EE_S|PROCESS_EE_T) ) {
		case PROCESS_EE_S:
			fpuFloat3(EEREC_S);
			t0reg = _allocTempXMMreg(XMMT_FPS, -1);
			if (t0reg >= 0) {
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Ft_]); 
				fpuFloat3(t0reg);
				SSE_UCOMISS_XMM_to_XMM(EEREC_S, t0reg); 
				_freeXMMreg(t0reg);
			}
			else SSE_UCOMISS_M32_to_XMM(EEREC_S, (uptr)&fpuRegs.fpr[_Ft_]);
			break;
		case PROCESS_EE_T:
			fpuFloat3(EEREC_T);
			t0reg = _allocTempXMMreg(XMMT_FPS, -1);
			if (t0reg >= 0) {
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Fs_]); 
				fpuFloat3(t0reg);
				SSE_UCOMISS_XMM_to_XMM(t0reg, EEREC_T); 
				_freeXMMreg(t0reg);
			}
			else {
				SSE_UCOMISS_M32_to_XMM(EEREC_T, (uptr)&fpuRegs.fpr[_Fs_]);
				
				j8Ptr[0] = JA8(0);
					AND32ItoM( (uptr)&fpuRegs.fprc[31], ~FPUflagC );
					j8Ptr[1] = JMP8(0);
				x86SetJ8(j8Ptr[0]);
					OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagC);
				x86SetJ8(j8Ptr[1]);
				return;
			}
			break;
		case (PROCESS_EE_S|PROCESS_EE_T):
			// Clamp NaNs
			// Note: This fixes a crash in Rule of Rose.
			fpuFloat3(EEREC_S);
			fpuFloat3(EEREC_T);
			SSE_UCOMISS_XMM_to_XMM(EEREC_S, EEREC_T); 
			break;
		default:
			Console.WriteLn(Color_Magenta, "recC_LT_xmm: Default");
			tempReg = _allocX86reg(-1, X86TYPE_TEMP, 0, 0);
			if (tempReg < 0) {Console.Error("FPU: DIV Allocation Error!"); tempReg = EAX;}
			MOV32MtoR(tempReg, (uptr)&fpuRegs.fpr[_Fs_]);
			CMP32MtoR(tempReg, (uptr)&fpuRegs.fpr[_Ft_]); 
			
			j8Ptr[0] = JL8(0);
				AND32ItoM( (uptr)&fpuRegs.fprc[31], ~FPUflagC );
				j8Ptr[1] = JMP8(0);
			x86SetJ8(j8Ptr[0]);
				OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagC);
			x86SetJ8(j8Ptr[1]);

			if (tempReg >= 0) _freeX86reg(tempReg);
			return;
	}

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
	if (CHECK_FPU_FULL)
	{
		DOUBLE::recCVT_W();
		return;
	}

	int regs = _checkXMMreg(XMMTYPE_FPREG, _Fs_, MODE_READ);

	if( regs >= 0 )
	{		
		if (CHECK_FPU_EXTRA_OVERFLOW) fpuFloat2(regs);
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
	//if (t1reg == -1) {Console.Error("FPU: DIV Allocation Error!");}
	if (tempReg == -1) {Console.Error("FPU: DIV Allocation Error!"); tempReg = EAX;}

	AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagI|FPUflagD)); // Clear I and D flags

	/*--- Check for divide by zero ---*/
	SSE_XORPS_XMM_to_XMM(t1reg, t1reg);
	SSE_CMPEQSS_XMM_to_XMM(t1reg, regt);
	SSE_MOVMSKPS_XMM_to_R32(tempReg, t1reg);
	AND32ItoR(tempReg, 1);  //Check sign (if regt == zero, sign will be set)
	ajmp32 = JZ32(0); //Skip if not set

		/*--- Check for 0/0 ---*/
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

		/*--- Make regd +/- Maximum ---*/
		SSE_XORPS_XMM_to_XMM(regd, regt); // Make regd Positive or Negative
		SSE_ANDPS_M128_to_XMM(regd, (uptr)&s_neg[0]); // Get the sign bit
		SSE_ORPS_M128_to_XMM(regd, (uptr)&g_maxvals[0]); // regd = +/- Maximum
		//SSE_MOVSS_M32_to_XMM(regd, (uptr)&g_maxvals[0]);
		bjmp32 = JMP32(0);

	x86SetJ32(ajmp32);

	/*--- Normal Divide ---*/
	if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(regt); }
	SSE_DIVSS_XMM_to_XMM(regd, regt);

	ClampValues(regd);
	x86SetJ32(bjmp32);

	_freeXMMreg(t1reg);
	_freeX86reg(tempReg);
}

void recDIVhelper2(int regd, int regt) // Doesn't sets flags
{
	if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(regt); }
	SSE_DIVSS_XMM_to_XMM(regd, regt);
	ClampValues(regd);
}

static __aligned16 SSE_MXCSR roundmode_nearest, roundmode_neg;

void recDIV_S_xmm(int info)
{
	bool roundmodeFlag = false;
	int t0reg = _allocTempXMMreg(XMMT_FPS, -1);
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
		roundmodeFlag = true;
	}

	switch(info & (PROCESS_EE_S|PROCESS_EE_T) ) {
		case PROCESS_EE_S:
			//Console.WriteLn("FPU: DIV case 1");
			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Ft_]);
			if (CHECK_FPU_EXTRA_FLAGS) recDIVhelper1(EEREC_D, t0reg);
			else recDIVhelper2(EEREC_D, t0reg);
			break;
		case PROCESS_EE_T:
			//Console.WriteLn("FPU: DIV case 2");
			if (EEREC_D == EEREC_T) {
				SSE_MOVSS_XMM_to_XMM(t0reg, EEREC_T);
				SSE_MOVSS_M32_to_XMM(EEREC_D, (uptr)&fpuRegs.fpr[_Fs_]);
				if (CHECK_FPU_EXTRA_FLAGS) recDIVhelper1(EEREC_D, t0reg);
				else recDIVhelper2(EEREC_D, t0reg);
			}
			else {
				SSE_MOVSS_M32_to_XMM(EEREC_D, (uptr)&fpuRegs.fpr[_Fs_]);
				if (CHECK_FPU_EXTRA_FLAGS) recDIVhelper1(EEREC_D, EEREC_T);
				else recDIVhelper2(EEREC_D, EEREC_T);
			}
			break;
		case (PROCESS_EE_S|PROCESS_EE_T):
			//Console.WriteLn("FPU: DIV case 3");
			if (EEREC_D == EEREC_T) {
				SSE_MOVSS_XMM_to_XMM(t0reg, EEREC_T);
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				if (CHECK_FPU_EXTRA_FLAGS) recDIVhelper1(EEREC_D, t0reg);
				else recDIVhelper2(EEREC_D, t0reg);
			}
			else {
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				if (CHECK_FPU_EXTRA_FLAGS) recDIVhelper1(EEREC_D, EEREC_T);
				else recDIVhelper2(EEREC_D, EEREC_T);
			}
			break;
		default:
			//Console.WriteLn("FPU: DIV case 4");
			SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Ft_]);
			SSE_MOVSS_M32_to_XMM(EEREC_D, (uptr)&fpuRegs.fpr[_Fs_]);
			if (CHECK_FPU_EXTRA_FLAGS) recDIVhelper1(EEREC_D, t0reg);
			else recDIVhelper2(EEREC_D, t0reg);
			break;
	}
	if (roundmodeFlag) xLDMXCSR (g_sseMXCSR);
	_freeXMMreg(t0reg);
}

FPURECOMPILE_CONSTCODE(DIV_S, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);
//------------------------------------------------------------------



//------------------------------------------------------------------
// MADD XMM
//------------------------------------------------------------------
void recMADDtemp(int info, int regd)
{	
	int t1reg;
	int t0reg = _allocTempXMMreg(XMMT_FPS, -1);
	
	switch(info & (PROCESS_EE_S|PROCESS_EE_T) ) {
		case PROCESS_EE_S:
			if(regd == EEREC_S) {
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Ft_]);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(t0reg); }
				FPU_MUL(regd, t0reg);
				if (info & PROCESS_EE_ACC) {
					if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(EEREC_ACC); fpuFloat(regd); }
					FPU_ADD(regd, EEREC_ACC);
				}
				else {
					SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.ACC);
					if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(EEREC_ACC); fpuFloat(t0reg); }
					FPU_ADD(regd, t0reg);
				}
			} 
			else if (regd == EEREC_ACC){
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Ft_]);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(EEREC_S); fpuFloat2(t0reg); }
				FPU_MUL(t0reg, EEREC_S);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
				FPU_ADD(regd, t0reg);
			} 
			else {
				SSE_MOVSS_M32_to_XMM(regd, (uptr)&fpuRegs.fpr[_Ft_]);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(EEREC_S); }
				FPU_MUL(regd, EEREC_S);
				if (info & PROCESS_EE_ACC) {
					if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(EEREC_ACC); fpuFloat(regd); }
					FPU_ADD(regd, EEREC_ACC);
				}
				else {
					SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.ACC);
					if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(EEREC_ACC); fpuFloat(t0reg); }
					FPU_ADD(regd, t0reg);
				}
			}
			break;
		case PROCESS_EE_T:	
			if(regd == EEREC_T) {
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Fs_]);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(t0reg); }
				FPU_MUL(regd, t0reg);
				if (info & PROCESS_EE_ACC) {
					if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(EEREC_ACC); fpuFloat(regd); }
					FPU_ADD(regd, EEREC_ACC);
				}
				else {
					SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.ACC);
					if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(EEREC_ACC); fpuFloat(t0reg); }
					FPU_ADD(regd, t0reg);
				}
			} 
			else if (regd == EEREC_ACC){
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Fs_]);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(EEREC_T); fpuFloat2(t0reg); }
				FPU_MUL(t0reg, EEREC_T);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
				FPU_ADD(regd, t0reg);
			} 
			else {
				SSE_MOVSS_M32_to_XMM(regd, (uptr)&fpuRegs.fpr[_Fs_]);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(EEREC_T); }
				FPU_MUL(regd, EEREC_T);
				if (info & PROCESS_EE_ACC) {
					if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(EEREC_ACC); fpuFloat(regd); }
					FPU_ADD(regd, EEREC_ACC);
				}
				else {
					SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.ACC);
					if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(EEREC_ACC); fpuFloat(t0reg); }
					FPU_ADD(regd, t0reg);
				}
			}
			break;
		case (PROCESS_EE_S|PROCESS_EE_T):
			if(regd == EEREC_S) {
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(EEREC_T); }
				FPU_MUL(regd, EEREC_T);
				if (info & PROCESS_EE_ACC) {
					if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(EEREC_ACC); }
					FPU_ADD(regd, EEREC_ACC);
				}
				else {
					SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.ACC);
					if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
					FPU_ADD(regd, t0reg);
				}
			} 
			else if(regd == EEREC_T) {
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(EEREC_S); }
				FPU_MUL(regd, EEREC_S);
				if (info & PROCESS_EE_ACC) {
					if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(EEREC_ACC); }
					FPU_ADD(regd, EEREC_ACC);
				}
				else {
					SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.ACC);
					if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
					FPU_ADD(regd, t0reg);
				}
			} 
			else if(regd == EEREC_ACC) {
				SSE_MOVSS_XMM_to_XMM(t0reg, EEREC_S);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(t0reg); fpuFloat2(EEREC_T); }
				FPU_MUL(t0reg, EEREC_T);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
				FPU_ADD(regd, t0reg);
			} 
			else {
				SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(EEREC_T); }
				FPU_MUL(regd, EEREC_T);
				if (info & PROCESS_EE_ACC) {
					if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(EEREC_ACC); }
					FPU_ADD(regd, EEREC_ACC);
				}
				else {
					SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.ACC);
					if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
					FPU_ADD(regd, t0reg);
				}			
			}
			break;
		default:
			if(regd == EEREC_ACC){
				t1reg = _allocTempXMMreg(XMMT_FPS, -1);
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Fs_]);
				SSE_MOVSS_M32_to_XMM(t1reg, (uptr)&fpuRegs.fpr[_Ft_]);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(t0reg); fpuFloat2(t1reg); }
				FPU_MUL(t0reg, t1reg);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
				FPU_ADD(regd, t0reg);
				_freeXMMreg(t1reg);
			} 
			else
			{
				SSE_MOVSS_M32_to_XMM(regd, (uptr)&fpuRegs.fpr[_Fs_]);
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Ft_]);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(t0reg); }
				FPU_MUL(regd, t0reg);
				if (info & PROCESS_EE_ACC) {
					if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(EEREC_ACC); }
					FPU_ADD(regd, EEREC_ACC);
				}
				else {
					SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.ACC);
					if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
					FPU_ADD(regd, t0reg);
				}
			}
			break;
	}

     ClampValues(regd);
	 _freeXMMreg(t0reg);
}

void recMADD_S_xmm(int info)
{
	//AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagO|FPUflagU)); // Clear O and U flags
	recMADDtemp(info, EEREC_D);
}

FPURECOMPILE_CONSTCODE(MADD_S, XMMINFO_WRITED|XMMINFO_READACC|XMMINFO_READS|XMMINFO_READT);

void recMADDA_S_xmm(int info)
{
	//AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagO|FPUflagU)); // Clear O and U flags
	recMADDtemp(info, EEREC_ACC);
}

FPURECOMPILE_CONSTCODE(MADDA_S, XMMINFO_WRITEACC|XMMINFO_READACC|XMMINFO_READS|XMMINFO_READT);
//------------------------------------------------------------------


//------------------------------------------------------------------
// MAX / MIN XMM
//------------------------------------------------------------------
void recMAX_S_xmm(int info)
{
	//AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagO|FPUflagU)); // Clear O and U flags
    recCommutativeOp(info, EEREC_D, 2);
}

FPURECOMPILE_CONSTCODE(MAX_S, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);

void recMIN_S_xmm(int info)
{
	//AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagO|FPUflagU)); // Clear O and U flags
    recCommutativeOp(info, EEREC_D, 3);
}

FPURECOMPILE_CONSTCODE(MIN_S, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);
//------------------------------------------------------------------


//------------------------------------------------------------------
// MOV XMM
//------------------------------------------------------------------
void recMOV_S_xmm(int info)
{
	if( info & PROCESS_EE_S ) SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
	else SSE_MOVSS_M32_to_XMM(EEREC_D, (uptr)&fpuRegs.fpr[_Fs_]);
}

FPURECOMPILE_CONSTCODE(MOV_S, XMMINFO_WRITED|XMMINFO_READS);
//------------------------------------------------------------------


//------------------------------------------------------------------
// MSUB XMM
//------------------------------------------------------------------
void recMSUBtemp(int info, int regd)
{
int t1reg;
	int t0reg = _allocTempXMMreg(XMMT_FPS, -1);
	
	switch(info & (PROCESS_EE_S|PROCESS_EE_T) ) {
		case PROCESS_EE_S:
			if(regd == EEREC_S) {
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Ft_]);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(t0reg); }
				FPU_MUL(regd, t0reg);
				if (info & PROCESS_EE_ACC) { SSE_MOVSS_XMM_to_XMM(t0reg, EEREC_ACC); }
				else { SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.ACC); }
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
				FPU_SUB(t0reg, regd);
				SSE_MOVSS_XMM_to_XMM(regd, t0reg);
			} 
			else if (regd == EEREC_ACC){
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Ft_]);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(EEREC_S); fpuFloat2(t0reg); }
				FPU_MUL(t0reg, EEREC_S);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
				FPU_SUB(regd, t0reg);
			} 
			else {
				SSE_MOVSS_M32_to_XMM(regd, (uptr)&fpuRegs.fpr[_Ft_]);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(EEREC_S); }
				FPU_MUL(regd, EEREC_S);
				if (info & PROCESS_EE_ACC) { SSE_MOVSS_XMM_to_XMM(t0reg, EEREC_ACC); }
				else { SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.ACC); }
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
				FPU_SUB(t0reg, regd);
				SSE_MOVSS_XMM_to_XMM(regd, t0reg);
			}
			break;
		case PROCESS_EE_T:	
			if(regd == EEREC_T) {
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Fs_]);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(t0reg); }
				FPU_MUL(regd, t0reg);
				if (info & PROCESS_EE_ACC) { SSE_MOVSS_XMM_to_XMM(t0reg, EEREC_ACC); }
				else { SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.ACC); }
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
				FPU_SUB(t0reg, regd);
				SSE_MOVSS_XMM_to_XMM(regd, t0reg);
			} 
			else if (regd == EEREC_ACC){
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Fs_]);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(EEREC_T); fpuFloat2(t0reg); }
				FPU_MUL(t0reg, EEREC_T);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
				FPU_SUB(regd, t0reg);
			} 
			else {
				SSE_MOVSS_M32_to_XMM(regd, (uptr)&fpuRegs.fpr[_Fs_]);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(EEREC_T); }
				FPU_MUL(regd, EEREC_T);
				if (info & PROCESS_EE_ACC) { SSE_MOVSS_XMM_to_XMM(t0reg, EEREC_ACC); }
				else { SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.ACC); }
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
				FPU_SUB(t0reg, regd);
				SSE_MOVSS_XMM_to_XMM(regd, t0reg);
			}
			break;
		case (PROCESS_EE_S|PROCESS_EE_T):
			if(regd == EEREC_S) {
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(EEREC_T); }
				FPU_MUL(regd, EEREC_T);
				if (info & PROCESS_EE_ACC) { SSE_MOVSS_XMM_to_XMM(t0reg, EEREC_ACC); }
				else { SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.ACC); }
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
				FPU_SUB(t0reg, regd);
				SSE_MOVSS_XMM_to_XMM(regd, t0reg);
			} 
			else if(regd == EEREC_T) {
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(EEREC_S); }
				FPU_MUL(regd, EEREC_S);
				if (info & PROCESS_EE_ACC) { SSE_MOVSS_XMM_to_XMM(t0reg, EEREC_ACC); }
				else { SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.ACC); }
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
				FPU_SUB(t0reg, regd);
				SSE_MOVSS_XMM_to_XMM(regd, t0reg);
			} 
			else if(regd == EEREC_ACC) {
				SSE_MOVSS_XMM_to_XMM(t0reg, EEREC_S);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(t0reg); fpuFloat2(EEREC_T); }
				FPU_MUL(t0reg, EEREC_T);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
				FPU_SUB(regd, t0reg);
			} 
			else {
				SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(EEREC_T); }
				FPU_MUL(regd, EEREC_T);
				if (info & PROCESS_EE_ACC) { SSE_MOVSS_XMM_to_XMM(t0reg, EEREC_ACC); }
				else { SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.ACC); }
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
				FPU_SUB(t0reg, regd);	
				SSE_MOVSS_XMM_to_XMM(regd, t0reg);
			}
			break;
		default:
			if(regd == EEREC_ACC){
				t1reg = _allocTempXMMreg(XMMT_FPS, -1);
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Fs_]);
				SSE_MOVSS_M32_to_XMM(t1reg, (uptr)&fpuRegs.fpr[_Ft_]);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(t0reg); fpuFloat2(t1reg); }
				FPU_MUL(t0reg, t1reg);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
				FPU_SUB(regd, t0reg);
				_freeXMMreg(t1reg);
			} 
			else
			{
				SSE_MOVSS_M32_to_XMM(regd, (uptr)&fpuRegs.fpr[_Fs_]);
				SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Ft_]);
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat2(regd); fpuFloat2(t0reg); }
				FPU_MUL(regd, t0reg);
				if (info & PROCESS_EE_ACC)  { SSE_MOVSS_XMM_to_XMM(t0reg, EEREC_ACC); } 
				else { SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.ACC); }
				if (CHECK_FPU_EXTRA_OVERFLOW) { fpuFloat(regd); fpuFloat(t0reg); }
				FPU_SUB(t0reg, regd);
				SSE_MOVSS_XMM_to_XMM(regd, t0reg);	
			}
			break;
	}

     ClampValues(regd);
	 _freeXMMreg(t0reg);

}

void recMSUB_S_xmm(int info)
{
	//AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagO|FPUflagU)); // Clear O and U flags
	recMSUBtemp(info, EEREC_D);
}

FPURECOMPILE_CONSTCODE(MSUB_S, XMMINFO_WRITED|XMMINFO_READACC|XMMINFO_READS|XMMINFO_READT);

void recMSUBA_S_xmm(int info)
{
	//AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagO|FPUflagU)); // Clear O and U flags
	recMSUBtemp(info, EEREC_ACC);
}

FPURECOMPILE_CONSTCODE(MSUBA_S, XMMINFO_WRITEACC|XMMINFO_READACC|XMMINFO_READS|XMMINFO_READT);
//------------------------------------------------------------------


//------------------------------------------------------------------
// MUL XMM
//------------------------------------------------------------------
void recMUL_S_xmm(int info)
{			
	//AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagO|FPUflagU)); // Clear O and U flags
    ClampValues(recCommutativeOp(info, EEREC_D, 1)); 
}

FPURECOMPILE_CONSTCODE(MUL_S, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);

void recMULA_S_xmm(int info) 
{ 
	//AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagO|FPUflagU)); // Clear O and U flags
	ClampValues(recCommutativeOp(info, EEREC_ACC, 1));
}

FPURECOMPILE_CONSTCODE(MULA_S, XMMINFO_WRITEACC|XMMINFO_READS|XMMINFO_READT);
//------------------------------------------------------------------


//------------------------------------------------------------------
// NEG XMM
//------------------------------------------------------------------
void recNEG_S_xmm(int info) {
	if( info & PROCESS_EE_S ) SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
	else SSE_MOVSS_M32_to_XMM(EEREC_D, (uptr)&fpuRegs.fpr[_Fs_]);

	//AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagO|FPUflagU)); // Clear O and U flags
	SSE_XORPS_M128_to_XMM(EEREC_D, (uptr)&s_neg[0]);
	ClampValues(EEREC_D);
}

FPURECOMPILE_CONSTCODE(NEG_S, XMMINFO_WRITED|XMMINFO_READS);
//------------------------------------------------------------------


//------------------------------------------------------------------
// SUB XMM
//------------------------------------------------------------------
void recSUBhelper(int regd, int regt)
{
	if (CHECK_FPU_EXTRA_OVERFLOW /*&& !CHECK_FPUCLAMPHACK*/) { fpuFloat2(regd); fpuFloat2(regt); }
	FPU_SUB(regd, regt);
}

void recSUBop(int info, int regd)
{
	int t0reg = _allocTempXMMreg(XMMT_FPS, -1);
    //if (t0reg == -1) {Console.Error("FPU: SUB Allocation Error!");}

	//AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagO|FPUflagU)); // Clear O and U flags

	switch(info & (PROCESS_EE_S|PROCESS_EE_T) ) {
		case PROCESS_EE_S:
			//Console.WriteLn("FPU: SUB case 1");
			if (regd != EEREC_S) SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
			SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Ft_]);
			recSUBhelper(regd, t0reg);
			break;
		case PROCESS_EE_T:
			//Console.WriteLn("FPU: SUB case 2");
			if (regd == EEREC_T) {
				SSE_MOVSS_XMM_to_XMM(t0reg, EEREC_T);
				SSE_MOVSS_M32_to_XMM(regd, (uptr)&fpuRegs.fpr[_Fs_]);
				recSUBhelper(regd, t0reg);
			}
			else {
				SSE_MOVSS_M32_to_XMM(regd, (uptr)&fpuRegs.fpr[_Fs_]);
				recSUBhelper(regd, EEREC_T);
			}
			break;
		case (PROCESS_EE_S|PROCESS_EE_T):
			//Console.WriteLn("FPU: SUB case 3");
			if (regd == EEREC_T) {
				SSE_MOVSS_XMM_to_XMM(t0reg, EEREC_T);
				SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
				recSUBhelper(regd, t0reg);
			}
			else {
				SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
				recSUBhelper(regd, EEREC_T);
			}
			break;
		default:
			Console.Warning("FPU: SUB case 4");
			SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Ft_]);
			SSE_MOVSS_M32_to_XMM(regd, (uptr)&fpuRegs.fpr[_Fs_]);
			recSUBhelper(regd, t0reg);
			break;
	}

	ClampValues(regd);
	_freeXMMreg(t0reg);
}

void recSUB_S_xmm(int info)
{
	recSUBop(info, EEREC_D);
}

FPURECOMPILE_CONSTCODE(SUB_S, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);


void recSUBA_S_xmm(int info) 
{ 
	recSUBop(info, EEREC_ACC);
}

FPURECOMPILE_CONSTCODE(SUBA_S, XMMINFO_WRITEACC|XMMINFO_READS|XMMINFO_READT);
//------------------------------------------------------------------


//------------------------------------------------------------------
// SQRT XMM
//------------------------------------------------------------------
void recSQRT_S_xmm(int info)
{
	u8* pjmp;
	bool roundmodeFlag = false;
	//Console.WriteLn("FPU: SQRT");
	
	if (g_sseMXCSR.GetRoundMode() != SSEround_Nearest)
	{
		// Set roundmode to nearest if it isn't already
		//Console.WriteLn("sqrt to nearest");
		roundmode_nearest = g_sseMXCSR;
		roundmode_nearest.SetRoundMode( SSEround_Nearest );
		xLDMXCSR (roundmode_nearest);
		roundmodeFlag = true;
	}

	if( info & PROCESS_EE_T ) SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_T); 
	else SSE_MOVSS_M32_to_XMM(EEREC_D, (uptr)&fpuRegs.fpr[_Ft_]);

	if (CHECK_FPU_EXTRA_FLAGS) {
		int tempReg = _allocX86reg(-1, X86TYPE_TEMP, 0, 0);
		if (tempReg == -1) {Console.Error("FPU: SQRT Allocation Error!"); tempReg = EAX;}

		AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagI|FPUflagD)); // Clear I and D flags

		/*--- Check for negative SQRT ---*/
		SSE_MOVMSKPS_XMM_to_R32(tempReg, EEREC_D);
		AND32ItoR(tempReg, 1);  //Check sign
		pjmp = JZ8(0); //Skip if none are
			OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagI|FPUflagSI); // Set I and SI flags
			SSE_ANDPS_M128_to_XMM(EEREC_D, (uptr)&s_pos[0]); // Make EEREC_D Positive
		x86SetJ8(pjmp);

		_freeX86reg(tempReg);
	}
	else SSE_ANDPS_M128_to_XMM(EEREC_D, (uptr)&s_pos[0]); // Make EEREC_D Positive
	
	if (CHECK_FPU_OVERFLOW) SSE_MINSS_M32_to_XMM(EEREC_D, (uptr)&g_maxvals[0]);// Only need to do positive clamp, since EEREC_D is positive
	SSE_SQRTSS_XMM_to_XMM(EEREC_D, EEREC_D);
	if (CHECK_FPU_EXTRA_OVERFLOW) ClampValues(EEREC_D); // Shouldn't need to clamp again since SQRT of a number will always be smaller than the original number, doing it just incase :/
	
	if (roundmodeFlag) xLDMXCSR (g_sseMXCSR);
}

FPURECOMPILE_CONSTCODE(SQRT_S, XMMINFO_WRITED|XMMINFO_READT);
//------------------------------------------------------------------


//------------------------------------------------------------------
// RSQRT XMM
//------------------------------------------------------------------
void recRSQRThelper1(int regd, int t0reg) // Preforms the RSQRT function when regd <- Fs and t0reg <- Ft (Sets correct flags)
{
	u8 *pjmp1, *pjmp2;
	u32 *pjmp32;
	u8 *qjmp1, *qjmp2;
	int t1reg = _allocTempXMMreg(XMMT_FPS, -1);
	int tempReg = _allocX86reg(-1, X86TYPE_TEMP, 0, 0);
	//if (t1reg == -1) {Console.Error("FPU: RSQRT Allocation Error!");}
	if (tempReg == -1) {Console.Error("FPU: RSQRT Allocation Error!"); tempReg = EAX;}

	AND32ItoM((uptr)&fpuRegs.fprc[31], ~(FPUflagI|FPUflagD)); // Clear I and D flags

	/*--- (first) Check for negative SQRT ---*/
	SSE_MOVMSKPS_XMM_to_R32(tempReg, t0reg);
	AND32ItoR(tempReg, 1);  //Check sign
	pjmp2 = JZ8(0); //Skip if not set
		OR32ItoM((uptr)&fpuRegs.fprc[31], FPUflagI|FPUflagSI); // Set I and SI flags
		SSE_ANDPS_M128_to_XMM(t0reg, (uptr)&s_pos[0]); // Make t0reg Positive
	x86SetJ8(pjmp2);

	/*--- Check for zero ---*/
	SSE_XORPS_XMM_to_XMM(t1reg, t1reg);
	SSE_CMPEQSS_XMM_to_XMM(t1reg, t0reg);
	SSE_MOVMSKPS_XMM_to_R32(tempReg, t1reg);
	AND32ItoR(tempReg, 1);  //Check sign (if t0reg == zero, sign will be set)
	pjmp1 = JZ8(0); //Skip if not set
		/*--- Check for 0/0 ---*/
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

		/*--- Make regd +/- Maximum ---*/
		SSE_ANDPS_M128_to_XMM(regd, (uptr)&s_neg[0]); // Get the sign bit
		SSE_ORPS_M128_to_XMM(regd, (uptr)&g_maxvals[0]); // regd = +/- Maximum
		pjmp32 = JMP32(0);
	x86SetJ8(pjmp1);

	if (CHECK_FPU_EXTRA_OVERFLOW) {
		SSE_MINSS_M32_to_XMM(t0reg, (uptr)&g_maxvals[0]); // Only need to do positive clamp, since t0reg is positive
		fpuFloat2(regd);
	}

	SSE_SQRTSS_XMM_to_XMM(t0reg, t0reg);
	SSE_DIVSS_XMM_to_XMM(regd, t0reg);

	ClampValues(regd);
	x86SetJ32(pjmp32);

	_freeXMMreg(t1reg);
	_freeX86reg(tempReg);
}

void recRSQRThelper2(int regd, int t0reg) // Preforms the RSQRT function when regd <- Fs and t0reg <- Ft (Doesn't set flags)
{
	SSE_ANDPS_M128_to_XMM(t0reg, (uptr)&s_pos[0]); // Make t0reg Positive
	if (CHECK_FPU_EXTRA_OVERFLOW) {
		SSE_MINSS_M32_to_XMM(t0reg, (uptr)&g_maxvals[0]); // Only need to do positive clamp, since t0reg is positive
		fpuFloat2(regd);
	}
	SSE_SQRTSS_XMM_to_XMM(t0reg, t0reg);
	SSE_DIVSS_XMM_to_XMM(regd, t0reg);
	ClampValues(regd);
}

void recRSQRT_S_xmm(int info)
{
	int t0reg = _allocTempXMMreg(XMMT_FPS, -1);
	//if (t0reg == -1) {Console.Error("FPU: RSQRT Allocation Error!");}
	//Console.WriteLn("FPU: RSQRT");

	switch(info & (PROCESS_EE_S|PROCESS_EE_T) ) {
		case PROCESS_EE_S:
			//Console.WriteLn("FPU: RSQRT case 1");
			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Ft_]);
			if (CHECK_FPU_EXTRA_FLAGS) recRSQRThelper1(EEREC_D, t0reg);
			else recRSQRThelper2(EEREC_D, t0reg);
			break;
		case PROCESS_EE_T:	
			//Console.WriteLn("FPU: RSQRT case 2");
			SSE_MOVSS_XMM_to_XMM(t0reg, EEREC_T);
			SSE_MOVSS_M32_to_XMM(EEREC_D, (uptr)&fpuRegs.fpr[_Fs_]);
			if (CHECK_FPU_EXTRA_FLAGS) recRSQRThelper1(EEREC_D, t0reg);
			else recRSQRThelper2(EEREC_D, t0reg);
			break;
		case (PROCESS_EE_S|PROCESS_EE_T):
			//Console.WriteLn("FPU: RSQRT case 3");
			SSE_MOVSS_XMM_to_XMM(t0reg, EEREC_T);		
			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			if (CHECK_FPU_EXTRA_FLAGS) recRSQRThelper1(EEREC_D, t0reg);
			else recRSQRThelper2(EEREC_D, t0reg);
			break;
		default:
			//Console.WriteLn("FPU: RSQRT case 4");
			SSE_MOVSS_M32_to_XMM(t0reg, (uptr)&fpuRegs.fpr[_Ft_]);
			SSE_MOVSS_M32_to_XMM(EEREC_D, (uptr)&fpuRegs.fpr[_Fs_]);		
			if (CHECK_FPU_EXTRA_FLAGS) recRSQRThelper1(EEREC_D, t0reg);
			else recRSQRThelper2(EEREC_D, t0reg);
			break;
	}
	_freeXMMreg(t0reg);
}

FPURECOMPILE_CONSTCODE(RSQRT_S, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);

#endif // FPU_RECOMPILE

} } } }
