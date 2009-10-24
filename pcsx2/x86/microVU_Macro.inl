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

#ifdef CHECK_MACROVU0

#pragma once
#include "iR5900.h"
#include "R5900OpcodeTables.h"

extern void _vu0WaitMicro();
extern void _vu0FinishMicro();

//------------------------------------------------------------------
// Macro VU - Helper Macros / Functions
//------------------------------------------------------------------

using namespace R5900::Dynarec;

#define printCOP2 0&&
//#define printCOP2 DevCon.Status

void setupMacroOp(int mode, const char* opName) {
	printCOP2(opName);
	microVU0.prog.IRinfo.curPC = 0;
	microVU0.code = cpuRegs.code;
	memset(&microVU0.prog.IRinfo.info[0], 0, sizeof(microVU0.prog.IRinfo.info[0]));
	iFlushCall(FLUSH_EVERYTHING);
	microVU0.regAlloc->reset();
	if (mode & 0x01) { // Q-Reg will be Read
		SSE_MOVSS_M32_to_XMM(xmmPQ, (uptr)&microVU0.regs->VI[REG_Q].UL);
	}
	if (mode & 0x08) { // Clip Instruction
		microVU0.prog.IRinfo.info[0].cFlag.write	 = 0xff;
		microVU0.prog.IRinfo.info[0].cFlag.lastWrite = 0xff;
	}
	if (mode & 0x10) { // Update Status/Mac Flags
		microVU0.prog.IRinfo.info[0].sFlag.doFlag		= 1;
		microVU0.prog.IRinfo.info[0].sFlag.doNonSticky	= 1;
		microVU0.prog.IRinfo.info[0].sFlag.write		= 0;
		microVU0.prog.IRinfo.info[0].sFlag.lastWrite	= 0;
		microVU0.prog.IRinfo.info[0].mFlag.doFlag		= 1;
		microVU0.prog.IRinfo.info[0].mFlag.write		= 0xff;
		MOV32MtoR(gprF0, (uptr)&microVU0.regs->VI[REG_STATUS_FLAG].UL);
	}
}

void endMacroOp(int mode) {
	if (mode & 0x02) { // Q-Reg was Written To
		SSE_MOVSS_XMM_to_M32((uptr)&microVU0.regs->VI[REG_Q].UL, xmmPQ);
	}
	if (mode & 0x10) { // Status/Mac Flags were Updated
		MOV32RtoM((uptr)&microVU0.regs->VI[REG_STATUS_FLAG].UL, gprF0);
	}
	microVU0.regAlloc->flushAll();
}

#define REC_COP2_mVU0(f, opName, mode)						\
	void recV##f() {										\
		setupMacroOp(mode, opName);							\
		if (mode & 4) {										\
			mVU_##f(&microVU0, 0);							\
			if (!microVU0.prog.IRinfo.info[0].lOp.isNOP) {	\
				mVU_##f(&microVU0, 1);						\
			}												\
		}													\
		else { mVU_##f(&microVU0, 1); }						\
		endMacroOp(mode);									\
	}

#define INTERPRETATE_COP2_FUNC(f)							\
	void recV##f() {										\
		MOV32ItoM((uptr)&cpuRegs.code, cpuRegs.code);		\
		MOV32ItoM((uptr)&cpuRegs.pc, pc);					\
		iFlushCall(FLUSH_EVERYTHING);						\
		CALLFunc((uptr)V##f);								\
		_freeX86regs();										\
	}

//------------------------------------------------------------------
// Macro VU - Instructions
//------------------------------------------------------------------

//------------------------------------------------------------------
// Macro VU - Redirect Upper Instructions
//------------------------------------------------------------------

REC_COP2_mVU0(ABS,		"ABS",		0x00);
REC_COP2_mVU0(ITOF0,	"ITOF0",	0x00);
REC_COP2_mVU0(ITOF4,	"ITOF4",	0x00);
REC_COP2_mVU0(ITOF12,	"ITOF12",	0x00);
REC_COP2_mVU0(ITOF15,	"ITOF15",	0x00);
REC_COP2_mVU0(FTOI0,	"FTOI0",	0x00);
REC_COP2_mVU0(FTOI4,	"FTOI4",	0x00);
REC_COP2_mVU0(FTOI12,	"FTOI12",	0x00);
REC_COP2_mVU0(FTOI15,	"FTOI15",	0x00);
REC_COP2_mVU0(ADD,		"ADD",		0x10);
REC_COP2_mVU0(ADDi,		"ADDi",		0x10);
REC_COP2_mVU0(ADDq,		"ADDq",		0x11);
REC_COP2_mVU0(ADDx,		"ADDx",		0x10);
REC_COP2_mVU0(ADDy,		"ADDy",		0x10);
REC_COP2_mVU0(ADDz,		"ADDz",		0x10);
REC_COP2_mVU0(ADDw,		"ADDw",		0x10);
REC_COP2_mVU0(ADDA,		"ADDA",		0x10);
REC_COP2_mVU0(ADDAi,	"ADDAi",	0x10);
REC_COP2_mVU0(ADDAq,	"ADDAq",	0x11);
REC_COP2_mVU0(ADDAx,	"ADDAx",	0x10);
REC_COP2_mVU0(ADDAy,	"ADDAy",	0x10);
REC_COP2_mVU0(ADDAz,	"ADDAz",	0x10);
REC_COP2_mVU0(ADDAw,	"ADDAw",	0x10);
REC_COP2_mVU0(SUB,		"SUB",		0x10);
REC_COP2_mVU0(SUBi,		"SUBi",		0x10);
REC_COP2_mVU0(SUBq,		"SUBq",		0x11);
REC_COP2_mVU0(SUBx,		"SUBx",		0x10);
REC_COP2_mVU0(SUBy,		"SUBy",		0x10);
REC_COP2_mVU0(SUBz,		"SUBz",		0x10);
REC_COP2_mVU0(SUBw,		"SUBw",		0x10);
REC_COP2_mVU0(SUBA,		"SUBA",		0x10);
REC_COP2_mVU0(SUBAi,	"SUBAi",	0x10);
REC_COP2_mVU0(SUBAq,	"SUBAq",	0x11);
REC_COP2_mVU0(SUBAx,	"SUBAx",	0x10);
REC_COP2_mVU0(SUBAy,	"SUBAy",	0x10);
REC_COP2_mVU0(SUBAz,	"SUBAz",	0x10);
REC_COP2_mVU0(SUBAw,	"SUBAw",	0x10);
REC_COP2_mVU0(MUL,		"MUL",		0x10);
REC_COP2_mVU0(MULi,		"MULi",		0x10);
REC_COP2_mVU0(MULq,		"MULq",		0x11);
REC_COP2_mVU0(MULx,		"MULx",		0x10);
REC_COP2_mVU0(MULy,		"MULy",		0x10);
REC_COP2_mVU0(MULz,		"MULz",		0x10);
REC_COP2_mVU0(MULw,		"MULw",		0x10);
REC_COP2_mVU0(MULA,		"MULA",		0x10);
REC_COP2_mVU0(MULAi,	"MULAi",	0x10);
REC_COP2_mVU0(MULAq,	"MULAq",	0x11);
REC_COP2_mVU0(MULAx,	"MULAx",	0x10);
REC_COP2_mVU0(MULAy,	"MULAy",	0x10);
REC_COP2_mVU0(MULAz,	"MULAz",	0x10);
REC_COP2_mVU0(MULAw,	"MULAw",	0x10);
REC_COP2_mVU0(MAX,		"MAX",		0x00);
REC_COP2_mVU0(MAXi,		"MAXi",		0x00);
REC_COP2_mVU0(MAXx,		"MAXx",		0x00);
REC_COP2_mVU0(MAXy,		"MAXy",		0x00);
REC_COP2_mVU0(MAXz,		"MAXz",		0x00);
REC_COP2_mVU0(MAXw,		"MAXw",		0x00);
REC_COP2_mVU0(MINI,		"MINI",		0x00);
REC_COP2_mVU0(MINIi,	"MINIi",	0x00);
REC_COP2_mVU0(MINIx,	"MINIx",	0x00);
REC_COP2_mVU0(MINIy,	"MINIy",	0x00);
REC_COP2_mVU0(MINIz,	"MINIz",	0x00);
REC_COP2_mVU0(MINIw,	"MINIw",	0x00);
REC_COP2_mVU0(MADD,		"MADD",		0x10);
REC_COP2_mVU0(MADDi,	"MADDi",	0x10);
REC_COP2_mVU0(MADDq,	"MADDq",	0x11);
REC_COP2_mVU0(MADDx,	"MADDx",	0x10);
REC_COP2_mVU0(MADDy,	"MADDy",	0x10);
REC_COP2_mVU0(MADDz,	"MADDz",	0x10);
REC_COP2_mVU0(MADDw,	"MADDw",	0x10);
REC_COP2_mVU0(MADDA,	"MADDA",	0x10);
REC_COP2_mVU0(MADDAi,	"MADDAi",	0x10);
REC_COP2_mVU0(MADDAq,	"MADDAq",	0x11);
REC_COP2_mVU0(MADDAx,	"MADDAx",	0x10);
REC_COP2_mVU0(MADDAy,	"MADDAy",	0x10);
REC_COP2_mVU0(MADDAz,	"MADDAz",	0x10);
REC_COP2_mVU0(MADDAw,	"MADDAw",	0x10);
REC_COP2_mVU0(MSUB,		"MSUB",		0x10);
REC_COP2_mVU0(MSUBi,	"MSUBi",	0x10);
REC_COP2_mVU0(MSUBq,	"MSUBq",	0x11);
REC_COP2_mVU0(MSUBx,	"MSUBx",	0x10);
REC_COP2_mVU0(MSUBy,	"MSUBy",	0x10);
REC_COP2_mVU0(MSUBz,	"MSUBz",	0x10);
REC_COP2_mVU0(MSUBw,	"MSUBw",	0x10);
REC_COP2_mVU0(MSUBA,	"MSUBA",	0x10);
REC_COP2_mVU0(MSUBAi,	"MSUBAi",	0x10);
REC_COP2_mVU0(MSUBAq,	"MSUBAq",	0x11);
REC_COP2_mVU0(MSUBAx,	"MSUBAx",	0x10);
REC_COP2_mVU0(MSUBAy,	"MSUBAy",	0x10);
REC_COP2_mVU0(MSUBAz,	"MSUBAz",	0x10);
REC_COP2_mVU0(MSUBAw,	"MSUBAw",	0x10);
REC_COP2_mVU0(OPMULA,	"OPMULA",	0x10);
REC_COP2_mVU0(OPMSUB,	"OPMSUB",	0x10);
REC_COP2_mVU0(CLIP,		"CLIP",		0x08);

//------------------------------------------------------------------
// Macro VU - Redirect Lower Instructions
//------------------------------------------------------------------

REC_COP2_mVU0(DIV,		"DIV",		0x02);
REC_COP2_mVU0(SQRT,		"SQRT",		0x02);
REC_COP2_mVU0(RSQRT,	"RSQRT",	0x02);
REC_COP2_mVU0(IADD,		"IADD",		0x04);
REC_COP2_mVU0(IADDI,	"IADDI",	0x04);
REC_COP2_mVU0(IAND,		"IAND",		0x04);
REC_COP2_mVU0(IOR,		"IOR",		0x04);
REC_COP2_mVU0(ISUB,		"ISUB",		0x04);
REC_COP2_mVU0(ILWR,		"ILWR",		0x04);
REC_COP2_mVU0(ISWR,		"ISWR",		0x00);
REC_COP2_mVU0(LQI,		"LQI",		0x04);
REC_COP2_mVU0(LQD,		"LQD",		0x04);
REC_COP2_mVU0(SQI,		"SQI",		0x00);
REC_COP2_mVU0(SQD,		"SQD",		0x00);
REC_COP2_mVU0(MFIR,		"MFIR",		0x04);
REC_COP2_mVU0(MTIR,		"MTIR",		0x04);
REC_COP2_mVU0(MOVE,		"MOVE",		0x00);
REC_COP2_mVU0(MR32,		"MR32",		0x00);
REC_COP2_mVU0(RINIT,	"RINIT",	0x00);
REC_COP2_mVU0(RGET,		"RGET",		0x04);
REC_COP2_mVU0(RNEXT,	"RNEXT",	0x04);
REC_COP2_mVU0(RXOR,		"RXOR",		0x00);

//------------------------------------------------------------------
// Macro VU - Misc...
//------------------------------------------------------------------

void recVNOP()	{}
void recVWAITQ(){}
INTERPRETATE_COP2_FUNC(CALLMS);
INTERPRETATE_COP2_FUNC(CALLMSR);

//------------------------------------------------------------------
// Macro VU - Branches
//------------------------------------------------------------------

void _setupBranchTest(u32*(jmpType)(u32), bool isLikely) {
	printCOP2("COP2 Branch");
	_eeFlushAllUnused();
	TEST32ItoM((uptr)&VU0.VI[REG_VPU_STAT].UL, 0x100);
	recDoBranchImm(jmpType(0), isLikely);
}

void recBC2F()  { _setupBranchTest(JNZ32, false); }
void recBC2T()  { _setupBranchTest(JZ32,  false); }
void recBC2FL() { _setupBranchTest(JNZ32, true);  }
void recBC2TL() { _setupBranchTest(JZ32,  true);  }

//------------------------------------------------------------------
// Macro VU - COP2 Transfer Instructions
//------------------------------------------------------------------

void COP2_Interlock(bool mBitSync) {
	if (cpuRegs.code & 1) {
		iFlushCall(FLUSH_NOCONST);
		if (mBitSync) CALLFunc((uptr)_vu0WaitMicro);
		else		  CALLFunc((uptr)_vu0FinishMicro);
	}
}

void TEST_FBRST_RESET(uptr resetFunct, int vuIndex) {
	TEST32ItoR(EAX, (vuIndex) ? 0x200 : 0x002);
	j8Ptr[0] = JZ8(0);
		CALLFunc(resetFunct);
		MOV32MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
	x86SetJ8(j8Ptr[0]);
}

static void recCFC2() {

	printCOP2("CFC2");
	COP2_Interlock(0);
	if (!_Rt_) return;
	iFlushCall(FLUSH_EVERYTHING);

	if (_Rd_ == REG_STATUS_FLAG) { // Normalize Status Flag
		MOV32MtoR(gprF0, (uptr)&microVU0.regs->VI[REG_STATUS_FLAG].UL);
		mVUallocSFLAGc(EAX, gprF0, 0);
	}
	else MOV32MtoR(EAX, (uptr)&microVU0.regs->VI[_Rd_].UL);

	// FixMe: Should R-Reg have upper 9 bits 0?
	MOV32RtoM((uptr)&cpuRegs.GPR.r[_Rt_].UL[0], EAX);

	if (_Rd_ >= 16) {
		CDQ(); // Sign Extend
		MOV32RtoM ((uptr)&cpuRegs.GPR.r[_Rt_].UL[1], EDX);
	}
	else MOV32ItoM((uptr)&cpuRegs.GPR.r[_Rt_].UL[1], 0);

	// FixMe: I think this is needed, but not sure how it works
	_eeOnWriteReg(_Rt_, 1);
}

static void recCTC2() {

	printCOP2("CTC2");
	COP2_Interlock(1);
	if (!_Rd_) return;
	iFlushCall(FLUSH_EVERYTHING);

	switch(_Rd_) {
		case REG_MAC_FLAG: case REG_TPC:
		case REG_VPU_STAT: break; // Read Only Regs
		case REG_R:
			MOV32MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
			OR32ItoR (EAX, 0x3f800000);
			MOV32RtoM((uptr)&microVU0.regs->VI[REG_R].UL, EAX);
			break;
		case REG_STATUS_FLAG:
			if (_Rt_) { // Denormalizes flag into gprF1
				mVUallocSFLAGd((uptr)&cpuRegs.GPR.r[_Rt_].UL[0], 0);
				MOV32RtoM((uptr)&microVU0.regs->VI[_Rd_].UL, gprF1);
			}
			else MOV32ItoM((uptr)&microVU0.regs->VI[_Rd_].UL, 0);
			break;
		case REG_CMSAR1:	// Execute VU1 Micro SubRoutine
			if (_Rt_) {
				MOV32MtoR(ECX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
			}
			else XOR32RtoR(ECX,ECX);
			xCALL(vu1ExecMicro);
			break;
		case REG_FBRST:
			if (!_Rt_) { 
				MOV32ItoM((uptr)&microVU0.regs->VI[REG_FBRST].UL, 0); 
				return;
			}
			else MOV32MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);

			TEST_FBRST_RESET((uptr)vu0ResetRegs, 0);
			TEST_FBRST_RESET((uptr)vu1ResetRegs, 1);

			AND32ItoR(EAX, 0x0C0C);
			MOV32RtoM((uptr)&microVU0.regs->VI[REG_FBRST].UL, EAX);
			break;
		default: 
			_eeMoveGPRtoM((uptr)&microVU0.regs->VI[_Rd_].UL, _Rt_); 
			break;
	}
}

static void recQMFC2() {

	printCOP2("QMFC2");
	COP2_Interlock(0);
	if (!_Rt_) return;
	iFlushCall(FLUSH_EVERYTHING);

	// FixMe: For some reason this line is needed or else games break:
	_eeOnWriteReg(_Rt_, 0);

	SSE_MOVAPS_M128_to_XMM(xmmT1, (uptr)&microVU0.regs->VF[_Rd_].UL[0]);
	SSE_MOVAPS_XMM_to_M128((uptr)&cpuRegs.GPR.r[_Rt_].UL[0], xmmT1);
}

static void recQMTC2() {

	printCOP2("QMTC2");
	COP2_Interlock(1);
	if (!_Rd_) return;
	iFlushCall(FLUSH_EVERYTHING);

	SSE_MOVAPS_M128_to_XMM(xmmT1, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
	SSE_MOVAPS_XMM_to_M128((uptr)&microVU0.regs->VF[_Rd_].UL[0], xmmT1);
}

//------------------------------------------------------------------
// Macro VU - Tables
//------------------------------------------------------------------

void recCOP2();
void recCOP2_BC2();
void recCOP2_SPEC1();
void recCOP2_SPEC2();
void rec_C2UNK() {
	Console.Error("Cop2 bad opcode: %x", cpuRegs.code);
}

// This is called by EE Recs to setup sVU info, this isn't needed for mVU Macro (cottonvibes)
void _vuRegsCOP22(VURegs* VU, _VURegsNum* VUregsn) {}

// Recompilation
void (*recCOP2t[32])() = {
    rec_C2UNK,	   recQMFC2,      recCFC2,		 rec_C2UNK,		rec_C2UNK,	   recQMTC2,      recCTC2,       rec_C2UNK,
    recCOP2_BC2,   rec_C2UNK,	  rec_C2UNK,	 rec_C2UNK,		rec_C2UNK,	   rec_C2UNK,	  rec_C2UNK,	 rec_C2UNK,
    recCOP2_SPEC1, recCOP2_SPEC1, recCOP2_SPEC1, recCOP2_SPEC1, recCOP2_SPEC1, recCOP2_SPEC1, recCOP2_SPEC1, recCOP2_SPEC1,
	recCOP2_SPEC1, recCOP2_SPEC1, recCOP2_SPEC1, recCOP2_SPEC1, recCOP2_SPEC1, recCOP2_SPEC1, recCOP2_SPEC1, recCOP2_SPEC1,
};

void (*recCOP2_BC2t[32])() = {
    recBC2F,	recBC2T,	recBC2FL,	recBC2TL,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,
    rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,
    rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,
    rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,
}; 

void (*recCOP2SPECIAL1t[64])() = { 
	recVADDx,	recVADDy,	recVADDz,	recVADDw,	recVSUBx,		recVSUBy,		recVSUBz,		recVSUBw,  
	recVMADDx,	recVMADDy,	recVMADDz,	recVMADDw,	recVMSUBx,		recVMSUBy,		recVMSUBz,		recVMSUBw, 
	recVMAXx,	recVMAXy,	recVMAXz,	recVMAXw,	recVMINIx,		recVMINIy,		recVMINIz,		recVMINIw, 
	recVMULx,	recVMULy,	recVMULz,	recVMULw,	recVMULq,		recVMAXi,		recVMULi,		recVMINIi,
	recVADDq,	recVMADDq,	recVADDi,	recVMADDi,	recVSUBq,		recVMSUBq,		recVSUBi,		recVMSUBi, 
	recVADD,	recVMADD,	recVMUL,	recVMAX,	recVSUB,		recVMSUB,		recVOPMSUB,		recVMINI,  
	recVIADD,	recVISUB,	recVIADDI,	rec_C2UNK,	recVIAND,		recVIOR,		rec_C2UNK,		rec_C2UNK,
	recVCALLMS,	recVCALLMSR,rec_C2UNK,	rec_C2UNK,	recCOP2_SPEC2,	recCOP2_SPEC2,	recCOP2_SPEC2,	recCOP2_SPEC2,  
};

void (*recCOP2SPECIAL2t[128])() = {
	recVADDAx,	recVADDAy,	recVADDAz,	recVADDAw,	recVSUBAx,	recVSUBAy,	recVSUBAz,	recVSUBAw,
	recVMADDAx,recVMADDAy,	recVMADDAz,	recVMADDAw,	recVMSUBAx,	recVMSUBAy,	recVMSUBAz,	recVMSUBAw,
	recVITOF0,	recVITOF4,	recVITOF12,	recVITOF15,	recVFTOI0,	recVFTOI4,	recVFTOI12,	recVFTOI15,
	recVMULAx,	recVMULAy,	recVMULAz,	recVMULAw,	recVMULAq,	recVABS,	recVMULAi,	recVCLIP,
	recVADDAq,	recVMADDAq,	recVADDAi,	recVMADDAi,	recVSUBAq,	recVMSUBAq,	recVSUBAi,	recVMSUBAi,
	recVADDA,	recVMADDA,	recVMULA,	rec_C2UNK,	recVSUBA,	recVMSUBA,	recVOPMULA,	recVNOP,   
	recVMOVE,	recVMR32,	rec_C2UNK,	rec_C2UNK,	recVLQI,	recVSQI,	recVLQD,	recVSQD,   
	recVDIV,	recVSQRT,	recVRSQRT,	recVWAITQ,	recVMTIR,	recVMFIR,	recVILWR,	recVISWR,  
	recVRNEXT,	recVRGET,	recVRINIT,	recVRXOR,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK, 
	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,
	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,
	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,
	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,
	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,
	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,
	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,	rec_C2UNK,
};

namespace R5900 {
namespace Dynarec {
namespace OpcodeImpl { void recCOP2() { recCOP2t[_Rs_](); }}}}
void recCOP2_BC2  () { recCOP2_BC2t[_Rt_](); }
void recCOP2_SPEC1() { recCOP2SPECIAL1t[_Funct_](); }
void recCOP2_SPEC2() { recCOP2SPECIAL2t[(cpuRegs.code&3)|((cpuRegs.code>>4)&0x7c)](); }

#endif // CHECK_MACROVU0
