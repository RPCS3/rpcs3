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

#ifdef CHECK_MACROVU0

#pragma once
#include "iR5900.h"
#include "R5900OpcodeTables.h"

extern void _vu0WaitMicro();

//------------------------------------------------------------------
// Macro VU - Helper Macros
//------------------------------------------------------------------

#undef _Ft_
#undef _Fs_
#undef _Fd_
#undef _Fsf_
#undef _Ftf_
#undef _Cc_

#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_

#define _Fsf_ ((cpuRegs.code >> 21) & 0x03)
#define _Ftf_ ((cpuRegs.code >> 23) & 0x03)
#define _Cc_   (cpuRegs.code & 0x03)

#define REC_COP2_VU0(f)									\
	void recV##f( s32 info ) {							\
		recVUMI_##f( &VU0, info );						\
	}

#define INTERPRETATE_COP2_FUNC(f)						\
void recV##f(s32 info) {								\
	MOV32ItoM((uptr)&cpuRegs.code, cpuRegs.code);		\
	MOV32ItoM((uptr)&cpuRegs.pc, pc);					\
	iFlushCall(FLUSH_EVERYTHING);						\
	CALLFunc((uptr)V##f);								\
	_freeX86regs();										\
}

void recCOP2(s32 info);
void recCOP2_SPECIAL(s32 info);
void recCOP2_BC2(s32 info);
void recCOP2_SPECIAL2(s32 info);
void rec_C2UNK( s32 info ) {
	Console::Error("Cop2 bad opcode: %x", params cpuRegs.code);
}
void _vuRegs_C2UNK(VURegs * VU, _VURegsNum *VUregsn) {
	Console::Error("Cop2 bad _vuRegs code:%x", params cpuRegs.code);
}

static void recCFC2(s32 info)
{
	int mmreg;

	if (cpuRegs.code & 1) {
		iFlushCall(FLUSH_NOCONST);
		CALLFunc((uptr)_vu0WaitMicro);
	}

	if(!_Rt_) return;

	_deleteGPRtoXMMreg(_Rt_, 2);
	mmreg = _checkMMXreg(MMX_GPR+_Rt_, MODE_WRITE);
	
	if (mmreg >= 0) {
		if( _Fs_ >= 16 ) {
			MOVDMtoMMX(mmreg, (uptr)&VU0.VI[ _Fs_ ].UL);
			if (EEINST_ISLIVE1(_Rt_)) { _signExtendGPRtoMMX(mmreg, _Rt_, 0); }
			else					  { EEINST_RESETHASLIVE1(_Rt_); }
		}
		else MOVDMtoMMX(mmreg, (uptr)&VU0.VI[ _Fs_ ].UL);
		SetMMXstate();
	}
	else {
		MOV32MtoR(EAX, (uptr)&VU0.VI[ _Fs_ ].UL);
		MOV32RtoM((uptr)&cpuRegs.GPR.r[_Rt_].UL[0],EAX);

		if(EEINST_ISLIVE1(_Rt_)) {
			if( _Fs_ < 16 ) {
				// no sign extending
				MOV32ItoM((uptr)&cpuRegs.GPR.r[_Rt_].UL[1],0);
			}
			else {
				CDQ();
				MOV32RtoM((uptr)&cpuRegs.GPR.r[_Rt_].UL[1], EDX);
			}
		}
		else { EEINST_RESETHASLIVE1(_Rt_); }
	}

	_eeOnWriteReg(_Rt_, 1);
}

static void recCTC2(s32 info)
{
	if (cpuRegs.code & 1) {
		iFlushCall(FLUSH_NOCONST);
		CALLFunc((uptr)_vu0WaitMicro);
	}

	if(!_Fs_) return;

	if( GPR_IS_CONST1(_Rt_) ) 
	{
		switch(_Fs_) {
			case REG_MAC_FLAG: // read-only
			case REG_TPC:      // read-only
			case REG_VPU_STAT: // read-only
				break;
			case REG_FBRST:
				if( g_cpuConstRegs[_Rt_].UL[0] & 0x202 )
					iFlushCall(FLUSH_FREE_TEMPX86);
				
				_deleteX86reg(X86TYPE_VI, REG_FBRST, 2);

				if( g_cpuConstRegs[_Rt_].UL[0] & 2 ) 
					CALLFunc((uptr)vu0ResetRegs);
				if( g_cpuConstRegs[_Rt_].UL[0] & 0x200 ) 
					CALLFunc((uptr)vu1ResetRegs);
				MOV16ItoM((uptr)&VU0.VI[REG_FBRST].UL,g_cpuConstRegs[_Rt_].UL[0]&0x0c0c);
				break;
			case REG_CMSAR1: // REG_CMSAR1
				iFlushCall(FLUSH_NOCONST);// since CALLFunc
				assert( _checkX86reg(X86TYPE_VI, REG_VPU_STAT, 0) < 0 &&
					    _checkX86reg(X86TYPE_VI, REG_TPC, 0) < 0 );
				// Execute VU1 Micro SubRoutine
				_callFunctionArg1((uptr)vu1ExecMicro, MEM_CONSTTAG, g_cpuConstRegs[_Rt_].UL[0]&0xffff);
				break;
			default:
			{
				if( _Fs_ < 16 )
					assert( (g_cpuConstRegs[_Rt_].UL[0]&0xffff0000)==0);

				// a lot of games have vu0 spinning on some integer
				// then they modify the register and expect vu0 to stop spinning within 10 cycles (donald duck)

				// Use vu0ExecMicro instead because it properly stalls for already-running micro
				// instructions, and also sets the nextBranchCycle as needed. (air)

				MOV32ItoM((uptr)&VU0.VI[_Fs_].UL,g_cpuConstRegs[_Rt_].UL[0]);
				//PUSH32I( -1 );
				iFlushCall(FLUSH_NOCONST);
				CALLFunc((uptr)CpuVU0.ExecuteBlock);
				//CALLFunc((uptr)vu0ExecMicro);
				//ADD32ItoR( ESP, 4 );
				break;
			}
		}
	}
	else 
	{
		switch(_Fs_) {
			case REG_MAC_FLAG: // read-only
			case REG_TPC:      // read-only
			case REG_VPU_STAT: // read-only
				break;
			case REG_FBRST:
				iFlushCall(FLUSH_FREE_TEMPX86);
				assert( _checkX86reg(X86TYPE_VI, REG_FBRST, 0) < 0 );

				_eeMoveGPRtoR(EAX, _Rt_);

				TEST32ItoR(EAX,0x2);
				j8Ptr[0] = JZ8(0);
				CALLFunc((uptr)vu0ResetRegs);
				_eeMoveGPRtoR(EAX, _Rt_);
				x86SetJ8(j8Ptr[0]);

				TEST32ItoR(EAX,0x200);
				j8Ptr[0] = JZ8(0);
				CALLFunc((uptr)vu1ResetRegs);
				_eeMoveGPRtoR(EAX, _Rt_);
				x86SetJ8(j8Ptr[0]);

				AND32ItoR(EAX,0x0C0C);
				MOV16RtoM((uptr)&VU0.VI[REG_FBRST].UL,EAX);
				break;
			case REG_CMSAR1: // REG_CMSAR1
				iFlushCall(FLUSH_NOCONST);
				_eeMoveGPRtoR(EAX, _Rt_);
				_callFunctionArg1((uptr)vu1ExecMicro, MEM_X86TAG|EAX, 0);	// Execute VU1 Micro SubRoutine
				break;
			default:
			_eeMoveGPRtoM((uptr)&VU0.VI[_Fs_].UL,_Rt_);
			
			// a lot of games have vu0 spinning on some integer
			// then they modify the register and expect vu0 to stop spinning within 10 cycles (donald duck)
			iFlushCall(FLUSH_NOCONST);
			break;
		}
	}
}

static void recQMFC2(s32 info)
{
	int t0reg, fsreg;

	if (cpuRegs.code & 1) {
		iFlushCall(FLUSH_NOCONST);
		CALLFunc((uptr)_vu0WaitMicro);
	}

	if(!_Rt_) return;

	_deleteMMXreg(MMX_GPR+_Rt_, 2);
	_deleteX86reg(X86TYPE_GPR, _Rt_, 2);
	_eeOnWriteReg(_Rt_, 0);
	
	// could 'borrow' the reg
	fsreg = _checkXMMreg(XMMTYPE_VFREG, _Fs_, MODE_READ);

	if( fsreg >= 0 ) {
		if ( xmmregs[fsreg].mode & MODE_WRITE ) {
			_xmmregs temp;
			
			t0reg = _allocGPRtoXMMreg(-1, _Rt_, MODE_WRITE);
			SSEX_MOVDQA_XMM_to_XMM(t0reg, fsreg);

			// change regs
			temp = xmmregs[t0reg];
			xmmregs[t0reg] = xmmregs[fsreg];
			xmmregs[fsreg] = temp;
		}
		else {
			// swap regs
			t0reg = _allocGPRtoXMMreg(-1, _Rt_, MODE_WRITE);

			xmmregs[fsreg] = xmmregs[t0reg];
			xmmregs[t0reg].inuse = 0;
		}
	}
	else {
		t0reg = _allocGPRtoXMMreg(-1, _Rt_, MODE_WRITE);
		
		if (t0reg >= 0) SSE_MOVAPS_M128_to_XMM( t0reg, (uptr)&VU0.VF[_Fs_].UD[0]);
		else			_recMove128MtoM((uptr)&cpuRegs.GPR.r[_Rt_].UL[0], (uptr)&VU0.VF[_Fs_].UL[0]);
	}

	_clearNeededXMMregs();
}

static void recQMTC2(s32 info)
{
	int mmreg;

	if (cpuRegs.code & 1) {
		iFlushCall(FLUSH_NOCONST);
		CALLFunc((uptr)_vu0WaitMicro);
	}

	if (!_Fs_) return;

	mmreg = _checkXMMreg(XMMTYPE_GPRREG, _Rt_, MODE_READ);
	
	if( mmreg >= 0) {
		int fsreg = _checkXMMreg(XMMTYPE_VFREG, _Fs_, MODE_WRITE);
		int flag = ((xmmregs[mmreg].mode&MODE_WRITE) && (g_pCurInstInfo->regs[_Rt_]&(EEINST_LIVE0|EEINST_LIVE1|EEINST_LIVE2)));
		
		if( fsreg >= 0 ) {

			if (flag) {
				SSE_MOVAPS_XMM_to_XMM(fsreg, mmreg);
			}
			else {
				// swap regs
				xmmregs[mmreg] = xmmregs[fsreg];
				xmmregs[mmreg].mode = MODE_WRITE;
				xmmregs[fsreg].inuse = 0;
				g_xmmtypes[mmreg] = XMMT_FPS;
			}
		}
		else {
			if (flag) SSE_MOVAPS_XMM_to_M128((uptr)&cpuRegs.GPR.r[_Rt_], mmreg);

			// swap regs
			xmmregs[mmreg].type = XMMTYPE_VFREG;
			xmmregs[mmreg].VU = 0;
			xmmregs[mmreg].reg = _Fs_;
			xmmregs[mmreg].mode = MODE_WRITE;
			g_xmmtypes[mmreg] = XMMT_FPS;
		}
	}
	else {
		int fsreg = _allocVFtoXMMreg(&VU0, -1, _Fs_, MODE_WRITE);

		if( fsreg >= 0 ) {
			mmreg = _checkMMXreg(MMX_GPR+_Rt_, MODE_READ);
			
			if( mmreg >= 0) {
				SetMMXstate();
				SSE2_MOVQ2DQ_MM_to_XMM(fsreg, mmreg);
				SSE_MOVHPS_M64_to_XMM(fsreg, (uptr)&cpuRegs.GPR.r[_Rt_].UL[2]);
			}
			else {
				if( GPR_IS_CONST1( _Rt_ ) ) {
					assert( _checkXMMreg(XMMTYPE_GPRREG, _Rt_, MODE_READ) == -1 );
					_flushConstReg(_Rt_);	
				}

				SSE_MOVAPS_M128_to_XMM(fsreg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
			}
		}
		else {
			_deleteEEreg(_Rt_, 0);
			_recMove128MtoM((uptr)&VU0.VF[_Fs_].UL[0], (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
		}
	}

	_clearNeededXMMregs();
}


//------------------------------------------------------------------
// Macro VU - Instructions
//------------------------------------------------------------------

using namespace R5900::Dynarec;

void printCOP2(const char* text) {
	Console::Status(text);
}

//------------------------------------------------------------------
// Macro VU - Branches
//------------------------------------------------------------------

static void _setupBranchTest() {
	_eeFlushAllUnused();
	TEST32ItoM((uptr)&VU0.VI[REG_VPU_STAT].UL, 0x100);
	printCOP2("_setupBranchTest()");
}

void recBC2F(s32 info) {
	_setupBranchTest();
	recDoBranchImm(JNZ32(0));
}

void recBC2T(s32 info) {
	_setupBranchTest();
	recDoBranchImm(JZ32(0));
}

void recBC2FL(s32 info) {
	_setupBranchTest();
	recDoBranchImm_Likely(JNZ32(0));
}

void recBC2TL(s32 info) {
	_setupBranchTest();
	recDoBranchImm_Likely(JZ32(0));
}

#define REC_COP2_mVU0(f, opName)													\
void recV##f(s32 info) {															\
	microVU0.prog.IRinfo.curPC = 0;													\
	microVU0.code = cpuRegs.code;													\
	memset(&microVU0.prog.IRinfo.info[0], 0, sizeof(microVU0.prog.IRinfo.info[0]));	\
	iFlushCall(FLUSH_EVERYTHING);													\
	microVU0.regAlloc->reset();														\
	/*mVU_##f(&microVU0, 0);*/														\
	mVU_##f(&microVU0, 1);															\
	microVU0.regAlloc->flushAll();													\
	printCOP2(opName);																\
}

//------------------------------------------------------------------
// Macro VU - Redirect Upper Instructions
//------------------------------------------------------------------

REC_COP2_mVU0(ABS,		"ABS");
REC_COP2_mVU0(ITOF0,	"ITOF0");
REC_COP2_mVU0(ITOF4,	"ITOF4");
REC_COP2_mVU0(ITOF12,	"ITOF12");
REC_COP2_mVU0(ITOF15,	"ITOF15");
REC_COP2_mVU0(FTOI0,	"FTOI0");
REC_COP2_mVU0(FTOI4,	"FTOI4");
REC_COP2_mVU0(FTOI12,	"FTOI12");
REC_COP2_mVU0(FTOI15,	"FTOI15");
REC_COP2_mVU0(ADD,		"ADD");
REC_COP2_mVU0(ADDi,		"ADDi");
REC_COP2_VU0 (ADDq);
REC_COP2_mVU0(ADDx,		"ADDx");
REC_COP2_mVU0(ADDy,		"ADDy");
REC_COP2_mVU0(ADDz,		"ADDz");
REC_COP2_mVU0(ADDw,		"ADDw");
REC_COP2_mVU0(ADDA,		"ADDA");
REC_COP2_mVU0(ADDAi,	"ADDAi");
REC_COP2_VU0 (ADDAq);
REC_COP2_mVU0(ADDAx,	"ADDAx");
REC_COP2_mVU0(ADDAy,	"ADDAy");
REC_COP2_mVU0(ADDAz,	"ADDAz");
REC_COP2_mVU0(ADDAw,	"ADDAw");
REC_COP2_mVU0(SUB,		"SUB");
REC_COP2_mVU0(SUBi,		"SUBi");
REC_COP2_VU0 (SUBq);
REC_COP2_mVU0(SUBx,		"SUBx");
REC_COP2_mVU0(SUBy,		"SUBy");
REC_COP2_mVU0(SUBz,		"SUBz");
REC_COP2_mVU0(SUBw,		"SUBw");
REC_COP2_mVU0(SUBA,		"SUBA");
REC_COP2_mVU0(SUBAi,	"SUBAi");
REC_COP2_VU0 (SUBAq);
REC_COP2_mVU0(SUBAx,	"SUBAx");
REC_COP2_mVU0(SUBAy,	"SUBAy");
REC_COP2_mVU0(SUBAz,	"SUBAz");
REC_COP2_mVU0(SUBAw,	"SUBAw");
REC_COP2_mVU0(MUL,		"MUL");
REC_COP2_mVU0(MULi,		"MULi");
REC_COP2_VU0 (MULq);
REC_COP2_mVU0(MULx,		"MULx");
REC_COP2_mVU0(MULy,		"MULy");
REC_COP2_mVU0(MULz,		"MULz");
REC_COP2_mVU0(MULw,		"MULw");
REC_COP2_mVU0(MULA,		"MULA");
REC_COP2_mVU0(MULAi,	"MULAi");
REC_COP2_VU0 (MULAq);
REC_COP2_mVU0(MULAx,	"MULAx");
REC_COP2_mVU0(MULAy,	"MULAy");
REC_COP2_mVU0(MULAz,	"MULAz");
REC_COP2_mVU0(MULAw,	"MULAw");
REC_COP2_mVU0(MAX,		"MAX");
REC_COP2_mVU0(MAXi,		"MAXi");
REC_COP2_mVU0(MAXx,		"MAXx");
REC_COP2_mVU0(MAXy,		"MAXy");
REC_COP2_mVU0(MAXz,		"MAXz");
REC_COP2_mVU0(MAXw,		"MAXw");
REC_COP2_mVU0(MINI,		"MINI");
REC_COP2_mVU0(MINIi,	"MINIi");
REC_COP2_mVU0(MINIx,	"MINIx");
REC_COP2_mVU0(MINIy,	"MINIy");
REC_COP2_mVU0(MINIz,	"MINIz");
REC_COP2_mVU0(MINIw,	"MINIw");
REC_COP2_mVU0(MADD,		"MADD");
REC_COP2_mVU0(MADDi,	"MADDi");
REC_COP2_VU0 (MADDq);
REC_COP2_mVU0(MADDx,	"MADDx");
REC_COP2_mVU0(MADDy,	"MADDy");
REC_COP2_mVU0(MADDz,	"MADDz");
REC_COP2_mVU0(MADDw,	"MADDw");
REC_COP2_mVU0(MADDA,	"MADDA");
REC_COP2_mVU0(MADDAi,	"MADDAi");
REC_COP2_VU0 (MADDAq);
REC_COP2_mVU0(MADDAx,	"MADDAx");
REC_COP2_mVU0(MADDAy,	"MADDAy");
REC_COP2_mVU0(MADDAz,	"MADDAz");
REC_COP2_mVU0(MADDAw,	"MADDAw");
REC_COP2_mVU0(MSUB,		"MSUB");
REC_COP2_mVU0(MSUBi,	"MSUBi");
REC_COP2_VU0 (MSUBq);
REC_COP2_mVU0(MSUBx,	"MSUBx");
REC_COP2_mVU0(MSUBy,	"MSUBy");
REC_COP2_mVU0(MSUBz,	"MSUBz");
REC_COP2_mVU0(MSUBw,	"MSUBw");
REC_COP2_mVU0(MSUBA,	"MSUBA");
REC_COP2_mVU0(MSUBAi,	"MSUBAi");
REC_COP2_VU0 (MSUBAq);
REC_COP2_mVU0(MSUBAx,	"MSUBAx");
REC_COP2_mVU0(MSUBAy,	"MSUBAy");
REC_COP2_mVU0(MSUBAz,	"MSUBAz");
REC_COP2_mVU0(MSUBAw,	"MSUBAw");
REC_COP2_mVU0(OPMULA,	"OPMULA");
REC_COP2_mVU0(OPMSUB,	"OPMSUB");
REC_COP2_VU0 (CLIP);

//------------------------------------------------------------------
// Macro VU - Redirect Lower Instructions
//------------------------------------------------------------------

REC_COP2_VU0(DIV);
REC_COP2_VU0(SQRT);
REC_COP2_VU0(RSQRT);
REC_COP2_VU0(IADD);
REC_COP2_VU0(IADDI);
REC_COP2_VU0(IAND);
REC_COP2_VU0(IOR);
REC_COP2_VU0(ISUB);
REC_COP2_VU0(ILWR);
REC_COP2_VU0(ISWR);
REC_COP2_VU0(LQI);
REC_COP2_VU0(LQD);
REC_COP2_VU0(SQI);
REC_COP2_VU0(SQD);
REC_COP2_VU0(MOVE);
REC_COP2_VU0(MFIR);
REC_COP2_VU0(MTIR);
REC_COP2_VU0(MR32);
REC_COP2_VU0(RINIT);
REC_COP2_VU0(RGET);
REC_COP2_VU0(RNEXT);
REC_COP2_VU0(RXOR);
/*
REC_COP2_mVU0(IADD,		"IADD");
REC_COP2_mVU0(IADDI,	"IADDI");
REC_COP2_mVU0(IAND,		"IAND");
REC_COP2_mVU0(IOR,		"IOR");
REC_COP2_mVU0(ISUB,		"ISUB");
REC_COP2_mVU0(ILWR,		"ILWR");
REC_COP2_mVU0(ISWR,		"ISWR");
REC_COP2_mVU0(LQI,		"LQI");
REC_COP2_mVU0(LQD,		"LQD");
REC_COP2_mVU0(SQI,		"SQI");
REC_COP2_mVU0(SQD,		"SQD");
REC_COP2_mVU0(MOVE,		"MOVE");
REC_COP2_mVU0(MFIR,		"MFIR");
REC_COP2_mVU0(MTIR,		"MTIR");
REC_COP2_mVU0(MR32,		"MR32");
REC_COP2_mVU0(RINIT,	"RINIT");
REC_COP2_mVU0(RGET,		"RGET");
REC_COP2_mVU0(RNEXT,	"RNEXT");
REC_COP2_mVU0(RXOR,		"RXOR");
*/

void recVNOP(s32 info){}
void recVWAITQ(s32 info){}
INTERPRETATE_COP2_FUNC(CALLMS);
INTERPRETATE_COP2_FUNC(CALLMSR);

void _vuRegsCOP2_SPECIAL (VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsCOP2_SPECIAL2(VURegs * VU, _VURegsNum *VUregsn);

// information
void _vuRegsQMFC2(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->VFread0 = _Fs_;
	VUregsn->VFr0xyzw= 0xf;
}

void _vuRegsCFC2(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->VIread = 1<<_Fs_;
}

void _vuRegsQMTC2(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->VFwrite = _Fs_;
	VUregsn->VFwxyzw= 0xf;
}

void _vuRegsCTC2(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->VIwrite = 1<<_Fs_;
}

void (*_vuRegsCOP2t[32])(VURegs * VU, _VURegsNum *VUregsn) = {
    _vuRegs_C2UNK,	_vuRegsQMFC2,	_vuRegsCFC2,	_vuRegs_C2UNK,	_vuRegs_C2UNK,	_vuRegsQMTC2,	_vuRegsCTC2,	_vuRegs_C2UNK,
    _vuRegsNOP,		_vuRegs_C2UNK,	_vuRegs_C2UNK,	_vuRegs_C2UNK,	_vuRegs_C2UNK,	_vuRegs_C2UNK,	_vuRegs_C2UNK,	_vuRegs_C2UNK,
    _vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,
	_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,
};

void (*_vuRegsCOP2SPECIAL1t[64])(VURegs * VU, _VURegsNum *VUregsn) = { 
 _vuRegsADDx,	_vuRegsADDy,	_vuRegsADDz,	_vuRegsADDw,	_vuRegsSUBx,	_vuRegsSUBy,	_vuRegsSUBz,	_vuRegsSUBw,  
 _vuRegsMADDx,	_vuRegsMADDy,	_vuRegsMADDz,	_vuRegsMADDw,	_vuRegsMSUBx,	_vuRegsMSUBy,	_vuRegsMSUBz,	_vuRegsMSUBw, 
 _vuRegsMAXx,	_vuRegsMAXy,	_vuRegsMAXz,	_vuRegsMAXw,	_vuRegsMINIx,	_vuRegsMINIy,	_vuRegsMINIz,	_vuRegsMINIw, 
 _vuRegsMULx,	_vuRegsMULy,	_vuRegsMULz,	_vuRegsMULw,	_vuRegsMULq,	_vuRegsMAXi,	_vuRegsMULi,	_vuRegsMINIi,
 _vuRegsADDq,	_vuRegsMADDq,	_vuRegsADDi,	_vuRegsMADDi,	_vuRegsSUBq,	_vuRegsMSUBq,	_vuRegsSUBi,	_vuRegsMSUBi, 
 _vuRegsADD,	_vuRegsMADD,	_vuRegsMUL,		_vuRegsMAX,		_vuRegsSUB,		_vuRegsMSUB,	_vuRegsOPMSUB,	_vuRegsMINI,  
 _vuRegsIADD,	_vuRegsISUB,	_vuRegsIADDI,	_vuRegs_C2UNK,	_vuRegsIAND,	_vuRegsIOR,		_vuRegs_C2UNK,	_vuRegs_C2UNK,
 _vuRegsNOP,	_vuRegsNOP,		_vuRegs_C2UNK,	_vuRegs_C2UNK,	_vuRegsCOP2_SPECIAL2,_vuRegsCOP2_SPECIAL2,_vuRegsCOP2_SPECIAL2,_vuRegsCOP2_SPECIAL2,  
};

void (*_vuRegsCOP2SPECIAL2t[128])(VURegs * VU, _VURegsNum *VUregsn) = {
 _vuRegsADDAx	,_vuRegsADDAy	,_vuRegsADDAz	,_vuRegsADDAw	,_vuRegsSUBAx	,_vuRegsSUBAy	,_vuRegsSUBAz	,_vuRegsSUBAw,
 _vuRegsMADDAx	,_vuRegsMADDAy	,_vuRegsMADDAz	,_vuRegsMADDAw	,_vuRegsMSUBAx	,_vuRegsMSUBAy	,_vuRegsMSUBAz	,_vuRegsMSUBAw,
 _vuRegsITOF0	,_vuRegsITOF4	,_vuRegsITOF12	,_vuRegsITOF15	,_vuRegsFTOI0	,_vuRegsFTOI4	,_vuRegsFTOI12	,_vuRegsFTOI15,
 _vuRegsMULAx	,_vuRegsMULAy	,_vuRegsMULAz	,_vuRegsMULAw	,_vuRegsMULAq	,_vuRegsABS		,_vuRegsMULAi	,_vuRegsCLIP,
 _vuRegsADDAq	,_vuRegsMADDAq	,_vuRegsADDAi	,_vuRegsMADDAi	,_vuRegsSUBAq	,_vuRegsMSUBAq	,_vuRegsSUBAi	,_vuRegsMSUBAi,
 _vuRegsADDA	,_vuRegsMADDA	,_vuRegsMULA	,_vuRegs_C2UNK	,_vuRegsSUBA	,_vuRegsMSUBA	,_vuRegsOPMULA	,_vuRegsNOP,   
 _vuRegsMOVE	,_vuRegsMR32	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegsLQI		,_vuRegsSQI		,_vuRegsLQD		,_vuRegsSQD,   
 _vuRegsDIV		,_vuRegsSQRT	,_vuRegsRSQRT	,_vuRegsWAITQ	,_vuRegsMTIR	,_vuRegsMFIR	,_vuRegsILWR	,_vuRegsISWR,  
 _vuRegsRNEXT	,_vuRegsRGET	,_vuRegsRINIT	,_vuRegsRXOR	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK, 
 _vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK,
 _vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK,
 _vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK,
 _vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK,
 _vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK,
 _vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK,
 _vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK	,_vuRegs_C2UNK,
};

#define cParams VURegs* VU, _VURegsNum* VUregsn
void _vuRegsCOP22(cParams)		   { _vuRegsCOP2t[_Rs_](VU, VUregsn); }
void _vuRegsCOP2_SPECIAL (cParams) { _vuRegsCOP2SPECIAL1t[_Funct_](VU, VUregsn); }
void _vuRegsCOP2_SPECIAL2(cParams) { _vuRegsCOP2SPECIAL2t[(cpuRegs.code&3)|((cpuRegs.code>>4)&0x7c)](VU, VUregsn); }

// recompilation
void (*recCOP2t[32])(s32 info) = {
    rec_C2UNK,		recQMFC2,       recCFC2,        rec_C2UNK,		rec_C2UNK,		recQMTC2,       recCTC2,        rec_C2UNK,
    recCOP2_BC2,    rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,
    recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,
	recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,
};

void (*recCOP2_BC2t[32])(s32 info) = {
    recBC2F,        recBC2T,        recBC2FL,       recBC2TL,       rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,
    rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,
    rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,
    rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,
}; 

void (*recCOP2SPECIAL1t[64])(s32 info) = { 
 recVADDx,       recVADDy,       recVADDz,       recVADDw,       recVSUBx,        recVSUBy,        recVSUBz,        recVSUBw,  
 recVMADDx,      recVMADDy,      recVMADDz,      recVMADDw,      recVMSUBx,       recVMSUBy,       recVMSUBz,       recVMSUBw, 
 recVMAXx,       recVMAXy,       recVMAXz,       recVMAXw,       recVMINIx,       recVMINIy,       recVMINIz,       recVMINIw, 
 recVMULx,       recVMULy,       recVMULz,       recVMULw,       recVMULq,        recVMAXi,        recVMULi,        recVMINIi,
 recVADDq,       recVMADDq,      recVADDi,       recVMADDi,      recVSUBq,        recVMSUBq,       recVSUBi,        recVMSUBi, 
 recVADD,        recVMADD,       recVMUL,        recVMAX,        recVSUB,         recVMSUB,        recVOPMSUB,      recVMINI,  
 recVIADD,       recVISUB,       recVIADDI,      rec_C2UNK,      recVIAND,        recVIOR,         rec_C2UNK,       rec_C2UNK,
 recVCALLMS,     recVCALLMSR,    rec_C2UNK,      rec_C2UNK,      recCOP2_SPECIAL2,recCOP2_SPECIAL2,recCOP2_SPECIAL2,recCOP2_SPECIAL2,  
};

void (*recCOP2SPECIAL2t[128])(s32 info) = {
 recVADDAx      ,recVADDAy      ,recVADDAz      ,recVADDAw      ,recVSUBAx      ,recVSUBAy      ,recVSUBAz      ,recVSUBAw,
 recVMADDAx     ,recVMADDAy     ,recVMADDAz     ,recVMADDAw     ,recVMSUBAx     ,recVMSUBAy     ,recVMSUBAz     ,recVMSUBAw,
 recVITOF0      ,recVITOF4      ,recVITOF12     ,recVITOF15     ,recVFTOI0      ,recVFTOI4      ,recVFTOI12     ,recVFTOI15,
 recVMULAx      ,recVMULAy      ,recVMULAz      ,recVMULAw      ,recVMULAq      ,recVABS        ,recVMULAi      ,recVCLIP,
 recVADDAq      ,recVMADDAq     ,recVADDAi      ,recVMADDAi     ,recVSUBAq      ,recVMSUBAq     ,recVSUBAi      ,recVMSUBAi,
 recVADDA       ,recVMADDA      ,recVMULA       ,rec_C2UNK      ,recVSUBA       ,recVMSUBA      ,recVOPMULA     ,recVNOP,   
 recVMOVE       ,recVMR32       ,rec_C2UNK      ,rec_C2UNK      ,recVLQI        ,recVSQI        ,recVLQD        ,recVSQD,   
 recVDIV        ,recVSQRT       ,recVRSQRT      ,recVWAITQ      ,recVMTIR       ,recVMFIR       ,recVILWR       ,recVISWR,  
 recVRNEXT      ,recVRGET       ,recVRINIT      ,recVRXOR       ,rec_C2UNK      ,rec_C2UNK      ,rec_C2UNK      ,rec_C2UNK, 
 rec_C2UNK      ,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK,
 rec_C2UNK      ,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK,
 rec_C2UNK      ,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK,
 rec_C2UNK      ,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK,
 rec_C2UNK      ,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK,
 rec_C2UNK      ,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK,
 rec_C2UNK      ,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK		,rec_C2UNK,
};

namespace R5900 {
namespace Dynarec {
namespace OpcodeImpl
{
	void recCOP2()
	{
		VU0.code = cpuRegs.code;

		g_pCurInstInfo->vuregs.pipe = 0xff; // to notify eeVURecompileCode that COP2
		s32 info = eeVURecompileCode(&VU0, &g_pCurInstInfo->vuregs);
		
		info |= PROCESS_VU_COP2;
		info |= PROCESS_VU_UPDATEFLAGS;

		recCOP2t[_Rs_]( info );

		_freeX86regs();
	}
}}}

void recCOP2_BC2(s32 info)		{ recCOP2_BC2t[_Rt_](info); }
void recCOP2_SPECIAL(s32 info)	{ recCOP2SPECIAL1t[_Funct_]( info ); }
void recCOP2_SPECIAL2(s32 info) { recCOP2SPECIAL2t[(cpuRegs.code&3)|((cpuRegs.code>>4)&0x7c)](info); }

#endif // CHECK_MACROVU0
