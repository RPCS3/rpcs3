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

// recompiler reworked to add dynamic linking zerofrog(@gmail.com) Jan06

#include "PrecompiledHeader.h"

#include "Common.h"
#include "R5900OpcodeTables.h"
#include "iR5900.h"

namespace Interp = R5900::Interpreter::OpcodeImpl;

namespace R5900 { 
namespace Dynarec { 
namespace OpcodeImpl
{

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, rt, offset                             *
*********************************************************/
#ifndef BRANCH_RECOMPILE

REC_SYS(BEQ);
REC_SYS(BEQL);
REC_SYS(BNE);
REC_SYS(BNEL);
REC_SYS(BLTZ);
REC_SYS(BGTZ);
REC_SYS(BLEZ);
REC_SYS(BGEZ);
REC_SYS(BGTZL);
REC_SYS(BLTZL);
REC_SYS(BLTZAL);
REC_SYS(BLTZALL);
REC_SYS(BLEZL);
REC_SYS(BGEZL);
REC_SYS(BGEZAL);
REC_SYS(BGEZALL);

#else

#if defined(EE_CONST_PROP)

void recSetBranchEQ(int info, int bne, int process)
{
	if( info & PROCESS_EE_MMX ) {
		int t0reg;

		SetMMXstate();

		if( process & PROCESS_CONSTS ) {
			if( (g_pCurInstInfo->regs[_Rt_] & EEINST_LASTUSE) || !EEINST_ISLIVE64(_Rt_) ) {
				_deleteMMXreg(_Rt_, 1);
				mmxregs[EEREC_T].inuse = 0;
				t0reg = EEREC_T;
			}
			else {
				t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
				MOVQRtoR(t0reg, EEREC_T);
			}
		
			_flushConstReg(_Rs_);
			PCMPEQDMtoR(t0reg, (u32)&cpuRegs.GPR.r[_Rs_].UL[0]);
			
			if( t0reg != EEREC_T ) _freeMMXreg(t0reg);
		}
		else if( process & PROCESS_CONSTT ) {
			if( (g_pCurInstInfo->regs[_Rs_] & EEINST_LASTUSE) || !EEINST_ISLIVE64(_Rs_) ) {
				_deleteMMXreg(_Rs_, 1);
				mmxregs[EEREC_S].inuse = 0;
				t0reg = EEREC_S;
			}
			else {
				t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
				MOVQRtoR(t0reg, EEREC_S);
			}
		
			_flushConstReg(_Rt_);
			PCMPEQDMtoR(t0reg, (u32)&cpuRegs.GPR.r[_Rt_].UL[0]);

			if( t0reg != EEREC_S ) _freeMMXreg(t0reg);
		}
		else {
			
			if( (g_pCurInstInfo->regs[_Rs_] & EEINST_LASTUSE) || !EEINST_ISLIVE64(_Rs_) ) {
				_deleteMMXreg(_Rs_, 1);
				mmxregs[EEREC_S].inuse = 0;
				t0reg = EEREC_S;
				PCMPEQDRtoR(t0reg, EEREC_T);
			}
			else if( (g_pCurInstInfo->regs[_Rt_] & EEINST_LASTUSE) || !EEINST_ISLIVE64(_Rt_) ) {
				_deleteMMXreg(_Rt_, 1);
				mmxregs[EEREC_T].inuse = 0;
				t0reg = EEREC_T;
				PCMPEQDRtoR(t0reg, EEREC_S);
			}
			else {
				t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
				MOVQRtoR(t0reg, EEREC_S);
				PCMPEQDRtoR(t0reg, EEREC_T);
			}

			if( t0reg != EEREC_S && t0reg != EEREC_T ) _freeMMXreg(t0reg);
		}

		PMOVMSKBMMXtoR(EAX, t0reg);

		_eeFlushAllUnused();

		CMP8ItoR( EAX, 0xff );

		if( bne ) j32Ptr[ 1 ] = JE32( 0 );
		else j32Ptr[ 0 ] = j32Ptr[ 1 ] = JNE32( 0 );
	}
	else if( info & PROCESS_EE_XMM ) {
		int t0reg;

		if( process & PROCESS_CONSTS ) {
			if( (g_pCurInstInfo->regs[_Rt_] & EEINST_LASTUSE) || !EEINST_ISLIVEXMM(_Rt_) ) {
				_deleteGPRtoXMMreg(_Rt_, 1);
				xmmregs[EEREC_T].inuse = 0;
				t0reg = EEREC_T;
			}
			else {
				t0reg = _allocTempXMMreg(XMMT_INT, -1);
				SSE2_MOVQ_XMM_to_XMM(t0reg, EEREC_T);
			}
		
			_flushConstReg(_Rs_);
			SSE2_PCMPEQD_M128_to_XMM(t0reg, (u32)&cpuRegs.GPR.r[_Rs_].UL[0]);
			

			if( t0reg != EEREC_T ) _freeXMMreg(t0reg);
		}
		else if( process & PROCESS_CONSTT ) {
			if( (g_pCurInstInfo->regs[_Rs_] & EEINST_LASTUSE) || !EEINST_ISLIVEXMM(_Rs_) ) {
				_deleteGPRtoXMMreg(_Rs_, 1);
				xmmregs[EEREC_S].inuse = 0;
				t0reg = EEREC_S;
			}
			else {
				t0reg = _allocTempXMMreg(XMMT_INT, -1);
				SSE2_MOVQ_XMM_to_XMM(t0reg, EEREC_S);
			}
		
			_flushConstReg(_Rt_);
			SSE2_PCMPEQD_M128_to_XMM(t0reg, (u32)&cpuRegs.GPR.r[_Rt_].UL[0]);

			if( t0reg != EEREC_S ) _freeXMMreg(t0reg);
		}
		else {
			
			if( (g_pCurInstInfo->regs[_Rs_] & EEINST_LASTUSE) || !EEINST_ISLIVEXMM(_Rs_) ) {
				_deleteGPRtoXMMreg(_Rs_, 1);
				xmmregs[EEREC_S].inuse = 0;
				t0reg = EEREC_S;
				SSE2_PCMPEQD_XMM_to_XMM(t0reg, EEREC_T);
			}
			else if( (g_pCurInstInfo->regs[_Rt_] & EEINST_LASTUSE) || !EEINST_ISLIVEXMM(_Rt_) ) {
				_deleteGPRtoXMMreg(_Rt_, 1);
				xmmregs[EEREC_T].inuse = 0;
				t0reg = EEREC_T;
				SSE2_PCMPEQD_XMM_to_XMM(t0reg, EEREC_S);
			}
			else {
				t0reg = _allocTempXMMreg(XMMT_INT, -1);
				SSE2_MOVQ_XMM_to_XMM(t0reg, EEREC_S);
				SSE2_PCMPEQD_XMM_to_XMM(t0reg, EEREC_T);
			}

			if( t0reg != EEREC_S && t0reg != EEREC_T ) _freeXMMreg(t0reg);
		}

		SSE_MOVMSKPS_XMM_to_R32(EAX, t0reg);

		_eeFlushAllUnused();

		AND8ItoR(EAX, 3);
		CMP8ItoR( EAX, 0x3 );

		if( bne ) j32Ptr[ 1 ] = JE32( 0 );
		else j32Ptr[ 0 ] = j32Ptr[ 1 ] = JNE32( 0 );
	}
	else {

		_eeFlushAllUnused();

		if( bne ) {
			if( process & PROCESS_CONSTS ) {
				CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], g_cpuConstRegs[_Rs_].UL[0] );
				j8Ptr[ 0 ] = JNE8( 0 );

				CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], g_cpuConstRegs[_Rs_].UL[1] );
				j32Ptr[ 1 ] = JE32( 0 );
			}
			else if( process & PROCESS_CONSTT ) {
				CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], g_cpuConstRegs[_Rt_].UL[0] );
				j8Ptr[ 0 ] = JNE8( 0 );

				CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], g_cpuConstRegs[_Rt_].UL[1] );
				j32Ptr[ 1 ] = JE32( 0 );
			}
			else {
				MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
				CMP32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
				j8Ptr[ 0 ] = JNE8( 0 );

				MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
				CMP32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
				j32Ptr[ 1 ] = JE32( 0 );
			}

			x86SetJ8( j8Ptr[0] );
		}
		else {
			// beq
			if( process & PROCESS_CONSTS ) {
				CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], g_cpuConstRegs[_Rs_].UL[0] );
				j32Ptr[ 0 ] = JNE32( 0 );

				CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], g_cpuConstRegs[_Rs_].UL[1] );
				j32Ptr[ 1 ] = JNE32( 0 );
			}
			else if( process & PROCESS_CONSTT ) {
				CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], g_cpuConstRegs[_Rt_].UL[0] );
				j32Ptr[ 0 ] = JNE32( 0 );

				CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], g_cpuConstRegs[_Rt_].UL[1] );
				j32Ptr[ 1 ] = JNE32( 0 );
			}
			else {
				MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
				CMP32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
				j32Ptr[ 0 ] = JNE32( 0 );

				MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
				CMP32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
				j32Ptr[ 1 ] = JNE32( 0 );
			}
		}
	}

	_clearNeededMMXregs();
	_clearNeededXMMregs();
}

void recSetBranchL(int ltz)
{
	int regs = _checkMMXreg(MMX_GPR+_Rs_, MODE_READ);

	if( regs >= 0 ) {

		int t0reg = _allocMMXreg(-1, MMX_TEMP, 0);

		SetMMXstate();

		PXORRtoR(t0reg, t0reg);
		PCMPGTDRtoR(t0reg, regs);
		PMOVMSKBMMXtoR(EAX, t0reg);

		_freeMMXreg(t0reg);
		_eeFlushAllUnused();

		TEST8ItoR( EAX, 0x80 );

		if( ltz ) j32Ptr[ 0 ] = JZ32( 0 );
		else j32Ptr[ 0 ] = JNZ32( 0 );

		return;
	}
	
	regs = _checkXMMreg(XMMTYPE_GPRREG, _Rs_, MODE_READ);

	if( regs >= 0 ) {
		
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		SSE_XORPS_XMM_to_XMM(t0reg, t0reg);
		SSE2_PCMPGTD_XMM_to_XMM(t0reg, regs);
		SSE_MOVMSKPS_XMM_to_R32(EAX, t0reg);

		_freeXMMreg(t0reg);
		_eeFlushAllUnused();

		TEST8ItoR( EAX, 2 );

		if( ltz ) j32Ptr[ 0 ] = JZ32( 0 );
		else j32Ptr[ 0 ] = JNZ32( 0 );

		return;
	}
	
	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], 0 );
	if( ltz ) j32Ptr[ 0 ] = JGE32( 0 );
	else j32Ptr[ 0 ] = JL32( 0 );

	_clearNeededMMXregs();
	_clearNeededXMMregs();
}

//// BEQ
void recBEQ_const()
{
	u32 branchTo;
	
	if( g_cpuConstRegs[_Rs_].SD[0] == g_cpuConstRegs[_Rt_].SD[0] )
		branchTo = ((s32)_Imm_ * 4) + pc;
	else
		branchTo = pc+4;

	recompileNextInstruction(1);
	SetBranchImm( branchTo );
}

void recBEQ_process(int info, int process)
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	if ( _Rs_ == _Rt_ )
	{
		recompileNextInstruction(1);
		SetBranchImm( branchTo );
	}
	else
	{
		recSetBranchEQ(info, 0, process);
		
		SaveBranchState();
		recompileNextInstruction(1);

		SetBranchImm(branchTo);

		x86SetJ32( j32Ptr[ 0 ] ); 
		x86SetJ32( j32Ptr[ 1 ] );

		// recopy the next inst
		pc -= 4;
		LoadBranchState();
		recompileNextInstruction(1);

		SetBranchImm(pc);
	}
}

void recBEQ_(int info) { recBEQ_process(info, 0); }
void recBEQ_consts(int info) { recBEQ_process(info, PROCESS_CONSTS); }
void recBEQ_constt(int info) { recBEQ_process(info, PROCESS_CONSTT); }

EERECOMPILE_CODE0(BEQ, XMMINFO_READS|XMMINFO_READT);

//// BNE
void recBNE_const()
{
	u32 branchTo;
	
	if( g_cpuConstRegs[_Rs_].SD[0] != g_cpuConstRegs[_Rt_].SD[0] )
		branchTo = ((s32)_Imm_ * 4) + pc;
	else
		branchTo = pc+4;

	recompileNextInstruction(1);
	SetBranchImm( branchTo );
}

void recBNE_process(int info, int process)
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	if ( _Rs_ == _Rt_ )
	{
		recompileNextInstruction(1);
		SetBranchImm(pc);
		return;
	}

	recSetBranchEQ(info, 1, process);

	SaveBranchState();
	recompileNextInstruction(1);
	
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 1 ] );

	// recopy the next inst
	pc -= 4;
	LoadBranchState();
	recompileNextInstruction(1);

	SetBranchImm(pc);
}

void recBNE_(int info) { recBNE_process(info, 0); }
void recBNE_consts(int info) { recBNE_process(info, PROCESS_CONSTS); }
void recBNE_constt(int info) { recBNE_process(info, PROCESS_CONSTT); }

EERECOMPILE_CODE0(BNE, XMMINFO_READS|XMMINFO_READT);

//// BEQL
void recBEQL_const()
{
	if( g_cpuConstRegs[_Rs_].SD[0] == g_cpuConstRegs[_Rt_].SD[0] ) {
		u32 branchTo = ((s32)_Imm_ * 4) + pc;
		recompileNextInstruction(1);
		SetBranchImm( branchTo );
	}
	else {
		SetBranchImm( pc+4 );
	}
}

void recBEQL_process(int info, int process)
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;
	recSetBranchEQ(info, 0, process);

	SaveBranchState();
	recompileNextInstruction(1);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] ); 
	x86SetJ32( j32Ptr[ 1 ] );
	
	LoadBranchState();
	SetBranchImm(pc);
}

void recBEQL_(int info) { recBEQL_process(info, 0); }
void recBEQL_consts(int info) { recBEQL_process(info, PROCESS_CONSTS); }
void recBEQL_constt(int info) { recBEQL_process(info, PROCESS_CONSTT); }

EERECOMPILE_CODE0(BEQL, XMMINFO_READS|XMMINFO_READT);

//// BNEL
void recBNEL_const()
{
	if( g_cpuConstRegs[_Rs_].SD[0] != g_cpuConstRegs[_Rt_].SD[0] ) {
		u32 branchTo = ((s32)_Imm_ * 4) + pc;
		recompileNextInstruction(1);
		SetBranchImm(branchTo);
	}
	else {
		SetBranchImm( pc+4 );
	}
}

void recBNEL_process(int info, int process) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	recSetBranchEQ(info, 0, process);

	SaveBranchState();
	SetBranchImm(pc+4);

	x86SetJ32( j32Ptr[ 0 ] ); 
	x86SetJ32( j32Ptr[ 1 ] );

	// recopy the next inst
	LoadBranchState();
	recompileNextInstruction(1);
	SetBranchImm(branchTo);
}

void recBNEL_(int info) { recBNEL_process(info, 0); }
void recBNEL_consts(int info) { recBNEL_process(info, PROCESS_CONSTS); }
void recBNEL_constt(int info) { recBNEL_process(info, PROCESS_CONSTT); }

EERECOMPILE_CODE0(BNEL, XMMINFO_READS|XMMINFO_READT);

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, offset                                 *
*********************************************************/

////////////////////////////////////////////////////
//void recBLTZAL( void ) 
//{
//	Console::WriteLn("BLTZAL");
//	_eeFlushAllUnused();
//	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
//	MOV32ItoM( (int)&cpuRegs.pc, pc );
//	iFlushCall(FLUSH_EVERYTHING);
//	CALLFunc( (int)BLTZAL );
//	branch = 2;    
//}

////////////////////////////////////////////////////
void recBLTZAL() 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	_eeOnWriteReg(31, 0);
	_eeFlushAllUnused();

	_deleteEEreg(31, 0);
	MOV32ItoM((uptr)&cpuRegs.GPR.r[31].UL[0], pc+4);
	MOV32ItoM((uptr)&cpuRegs.GPR.r[31].UL[1], 0);

	if( GPR_IS_CONST1(_Rs_) ) {
		if( !(g_cpuConstRegs[_Rs_].SD[0] < 0) )
			branchTo = pc+4;

		recompileNextInstruction(1);
		SetBranchImm( branchTo );
		return;
	}

	recSetBranchL(1);

	SaveBranchState();

	recompileNextInstruction(1);

	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] );

	// recopy the next inst
	pc -= 4;
	LoadBranchState();
	recompileNextInstruction(1);

	SetBranchImm(pc);
}

////////////////////////////////////////////////////
void recBGEZAL( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	_eeOnWriteReg(31, 0);
	_eeFlushAllUnused();

	_deleteEEreg(31, 0);
	MOV32ItoM((uptr)&cpuRegs.GPR.r[31].UL[0], pc+4);
	MOV32ItoM((uptr)&cpuRegs.GPR.r[31].UL[1], 0);

	if( GPR_IS_CONST1(_Rs_) ) {
		if( !(g_cpuConstRegs[_Rs_].SD[0] >= 0) )
			branchTo = pc+4;

		recompileNextInstruction(1);
		SetBranchImm( branchTo );
		return;
	}

	recSetBranchL(0);

	SaveBranchState();

	recompileNextInstruction(1);

	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] );

	// recopy the next inst
	pc -= 4;
	LoadBranchState();
	recompileNextInstruction(1);

	SetBranchImm(pc);
}

////////////////////////////////////////////////////
void recBLTZALL( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	_eeOnWriteReg(31, 0);
	_eeFlushAllUnused();

	_deleteEEreg(31, 0);
	MOV32ItoM((uptr)&cpuRegs.GPR.r[31].UL[0], pc+4);
	MOV32ItoM((uptr)&cpuRegs.GPR.r[31].UL[1], 0);

	if( GPR_IS_CONST1(_Rs_) ) {
		if( !(g_cpuConstRegs[_Rs_].SD[0] < 0) )
			SetBranchImm( pc + 4);
		else {
			recompileNextInstruction(1);
			SetBranchImm( branchTo );
		}
		return;
	}

	recSetBranchL(1);

	SaveBranchState();
	recompileNextInstruction(1);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] );

	LoadBranchState();
	SetBranchImm(pc);
}

////////////////////////////////////////////////////
void recBGEZALL( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	_eeOnWriteReg(31, 0);
	_eeFlushAllUnused();

	_deleteEEreg(31, 0);
	MOV32ItoM((uptr)&cpuRegs.GPR.r[31].UL[0], pc+4);
	MOV32ItoM((uptr)&cpuRegs.GPR.r[31].UL[1], 0);

	if( GPR_IS_CONST1(_Rs_) ) {
		if( !(g_cpuConstRegs[_Rs_].SD[0] >= 0) )
			SetBranchImm( pc + 4);
		else {
			recompileNextInstruction(1);
			SetBranchImm( branchTo );
		}
		return;
	}

	recSetBranchL(0);

	SaveBranchState();
	recompileNextInstruction(1);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] );

	LoadBranchState();
	SetBranchImm(pc);
}

#else


////////////////////////////////////////////////////
void recBEQ( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	if ( _Rs_ == _Rt_ )
	{
		_clearNeededMMXregs();
		_clearNeededXMMregs();
		recompileNextInstruction(1);
		SetBranchImm( branchTo );
	}
	else
	{
		MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
		CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		j32Ptr[ 0 ] = JNE32( 0 );

		MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
		CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
		j32Ptr[ 1 ] = JNE32( 0 );
		
		_clearNeededMMXregs();
		_clearNeededXMMregs();

		SaveBranchState();
		recompileNextInstruction(1);

		SetBranchImm(branchTo);

		x86SetJ32( j32Ptr[ 0 ] ); 
		x86SetJ32( j32Ptr[ 1 ] );

		// recopy the next inst
		pc -= 4;
		LoadBranchState();
		recompileNextInstruction(1);

		SetBranchImm(pc);
	}
}

////////////////////////////////////////////////////
void recBNE( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	if ( _Rs_ == _Rt_ )
	{
		_clearNeededMMXregs();
		_clearNeededXMMregs();
		recompileNextInstruction(1);
		SetBranchImm(pc);
		return;
	}

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	j32Ptr[ 0 ] = JNE32( 0 );

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	j32Ptr[ 1 ] = JE32( 0 );

	x86SetJ32( j32Ptr[ 0 ] ); 

	_clearNeededMMXregs();
	_clearNeededXMMregs();

	SaveBranchState();
	recompileNextInstruction(1);
	
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 1 ] );

	// recopy the next inst
	pc -= 4;
	LoadBranchState();
	recompileNextInstruction(1);

	SetBranchImm(pc);
}

////////////////////////////////////////////////////
void recBEQL( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	CMP32RtoR( ECX, EDX );
	j32Ptr[ 0 ] = JNE32( 0 );

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	CMP32RtoR( ECX, EDX );
	j32Ptr[ 1 ] = JNE32( 0 );

	_clearNeededMMXregs();
	_clearNeededXMMregs();

	SaveBranchState();
	recompileNextInstruction(1);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] ); 
	x86SetJ32( j32Ptr[ 1 ] );
	
	LoadBranchState();
	SetBranchImm(pc);
}

////////////////////////////////////////////////////
void recBNEL( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	CMP32RtoR( ECX, EDX );
	j32Ptr[ 0 ] = JNE32( 0 );

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	CMP32RtoR( ECX, EDX );
	j32Ptr[ 1 ] = JNE32( 0 );

	_clearNeededMMXregs();
	_clearNeededXMMregs();

	SaveBranchState();
	SetBranchImm(pc+4);

	x86SetJ32( j32Ptr[ 0 ] ); 
	x86SetJ32( j32Ptr[ 1 ] );

	// recopy the next inst
	LoadBranchState();
	recompileNextInstruction(1);
	SetBranchImm(branchTo);
}

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, offset                                 *
*********************************************************/

////////////////////////////////////////////////////
void recBLTZAL( void ) 
{
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall(FLUSH_EVERYTHING);
	CALLFunc( (int)BLTZAL );
	branch = 2;    
}

////////////////////////////////////////////////////
void recBGEZAL( void ) 
{
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall(FLUSH_EVERYTHING);
	CALLFunc( (int)BGEZAL );
	branch = 2; 
}

////////////////////////////////////////////////////
void recBLTZALL( void ) 
{
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall(FLUSH_EVERYTHING);
	CALLFunc( (int)BLTZALL );
	branch = 2; 
}

////////////////////////////////////////////////////
void recBGEZALL( void ) 
{
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall(FLUSH_EVERYTHING);
	CALLFunc( (int)BGEZALL );
	branch = 2; 
}

#endif

//// BLEZ
void recBLEZ( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	_eeFlushAllUnused();

	if( GPR_IS_CONST1(_Rs_) ) {
		if( !(g_cpuConstRegs[_Rs_].SD[0] <= 0) )
			branchTo = pc+4;

		recompileNextInstruction(1);
		SetBranchImm( branchTo );
		return;
	}

	_deleteEEreg(_Rs_, 1);

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], 0 );
	j8Ptr[ 0 ] = JL8( 0 );
	j32Ptr[ 1 ] = JG32( 0 );

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], 0 );
	j32Ptr[ 2 ] = JNZ32( 0 );

	x86SetJ8( j8Ptr[ 0 ] );

	_clearNeededMMXregs();
	_clearNeededXMMregs();

	SaveBranchState();
	recompileNextInstruction(1);

	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 1 ] );
	x86SetJ32( j32Ptr[ 2 ] );

	// recopy the next inst
	pc -= 4;
	LoadBranchState();
	recompileNextInstruction(1);

	SetBranchImm(pc);
}

//// BGTZ
void recBGTZ( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	_eeFlushAllUnused();

	if( GPR_IS_CONST1(_Rs_) ) {
		if( !(g_cpuConstRegs[_Rs_].SD[0] > 0) )
			branchTo = pc+4;

		recompileNextInstruction(1);
		SetBranchImm( branchTo );
		return;
	}

	_deleteEEreg(_Rs_, 1);

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], 0 );
	j8Ptr[ 0 ] = JG8( 0 );
	j32Ptr[ 1 ] = JL32( 0 );

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], 0 );
	j32Ptr[ 2 ] = JZ32( 0 );

	x86SetJ8( j8Ptr[ 0 ] );

	_clearNeededMMXregs();
	_clearNeededXMMregs();

	SaveBranchState();
	recompileNextInstruction(1);

	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 1 ] );
	x86SetJ32( j32Ptr[ 2 ] );

	// recopy the next inst
	pc -= 4;
	LoadBranchState();
	recompileNextInstruction(1);

	SetBranchImm(pc);
}

////////////////////////////////////////////////////
#ifdef EE_CONST_PROP
void recBLTZ() 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;
	
	_eeFlushAllUnused();

	if( GPR_IS_CONST1(_Rs_) ) {
		if( !(g_cpuConstRegs[_Rs_].SD[0] < 0) )
			branchTo = pc+4;

		recompileNextInstruction(1);
		SetBranchImm( branchTo );
		return;
	}

	recSetBranchL(1);

	SaveBranchState();
	recompileNextInstruction(1);

	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] );

	// recopy the next inst
	pc -= 4;
	LoadBranchState();
	recompileNextInstruction(1);

	SetBranchImm(pc);
}

////////////////////////////////////////////////////
void recBGEZ( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	_eeFlushAllUnused();

	if( GPR_IS_CONST1(_Rs_) ) {
		if( !(g_cpuConstRegs[_Rs_].SD[0] >= 0) )
			branchTo = pc+4;

		recompileNextInstruction(1);
		SetBranchImm( branchTo );
		return;
	}

	recSetBranchL(0);

	SaveBranchState();
	recompileNextInstruction(1);

	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] );

	// recopy the next inst
	pc -= 4;
	LoadBranchState();
	recompileNextInstruction(1);

	SetBranchImm(pc);
}

////////////////////////////////////////////////////
void recBLTZL( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	_eeFlushAllUnused();

	if( GPR_IS_CONST1(_Rs_) ) {
		if( !(g_cpuConstRegs[_Rs_].SD[0] < 0) )
			SetBranchImm( pc + 4);
		else {
			recompileNextInstruction(1);
			SetBranchImm( branchTo );
		}
		return;
	}

	recSetBranchL(1);

	SaveBranchState();
	recompileNextInstruction(1);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] );

	LoadBranchState();
	SetBranchImm(pc);
}

	
////////////////////////////////////////////////////
void recBGEZL( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	_eeFlushAllUnused();

	if( GPR_IS_CONST1(_Rs_) ) {
		if( !(g_cpuConstRegs[_Rs_].SD[0] >= 0) )
			SetBranchImm( pc + 4);
		else {
			recompileNextInstruction(1);
			SetBranchImm( branchTo );
		}
		return;
	}

	recSetBranchL(0);

	SaveBranchState();
	recompileNextInstruction(1);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] );

	LoadBranchState();
	SetBranchImm(pc);
}

#else
void recBLTZ( void ) 
{
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall(FLUSH_EVERYTHING);
	CALLFunc( (int)BLTZ );
	branch = 2;    
}

void recBGEZ( void ) 
{
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall(FLUSH_EVERYTHING);
	CALLFunc( (int)BGEZ );
	branch = 2;    
}

void recBLTZL( void ) 
{
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall(FLUSH_EVERYTHING);
	CALLFunc( (int)BLTZL );
	branch = 2;    
}

void recBGEZL( void ) 
{
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall(FLUSH_EVERYTHING);
    CALLFunc( (int)BGEZL );
	branch = 2;    
}

#endif



/*********************************************************
* Register branch logic  Likely                          *
* Format:  OP rs, offset                                 *
*********************************************************/

////////////////////////////////////////////////////
void recBLEZL( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	_eeFlushAllUnused();

	if( GPR_IS_CONST1(_Rs_) ) {
		if( !(g_cpuConstRegs[_Rs_].SD[0] <= 0) )
			SetBranchImm( pc + 4);
		else {
			_clearNeededMMXregs();
			_clearNeededXMMregs();
			recompileNextInstruction(1);
			SetBranchImm( branchTo );
		}
		return;
	}

	_deleteEEreg(_Rs_, 1);

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], 0 );
	j32Ptr[ 0 ] = JL32( 0 );
	j32Ptr[ 1 ] = JG32( 0 );

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], 0 );
	j32Ptr[ 2 ] = JNZ32( 0 );

	x86SetJ32( j32Ptr[ 0 ] );

	_clearNeededMMXregs();
	_clearNeededXMMregs();

	SaveBranchState();
	recompileNextInstruction(1);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 1 ] );
	x86SetJ32( j32Ptr[ 2 ] );

	LoadBranchState();
	SetBranchImm(pc);
}

////////////////////////////////////////////////////
void recBGTZL( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	_eeFlushAllUnused();

	if( GPR_IS_CONST1(_Rs_) ) {
		if( !(g_cpuConstRegs[_Rs_].SD[0] > 0) )
			SetBranchImm( pc + 4);
		else {
			_clearNeededMMXregs();
			_clearNeededXMMregs();
			recompileNextInstruction(1);
			SetBranchImm( branchTo );
		}
		return;
	}

	_deleteEEreg(_Rs_, 1);

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], 0 );
	j32Ptr[ 0 ] = JG32( 0 );
	j32Ptr[ 1 ] = JL32( 0 );

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], 0 );
	j32Ptr[ 2 ] = JZ32( 0 );

	x86SetJ32( j32Ptr[ 0 ] );

	_clearNeededMMXregs();
	_clearNeededXMMregs();

	SaveBranchState();
	recompileNextInstruction(1);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 1 ] );
	x86SetJ32( j32Ptr[ 2 ] );

	LoadBranchState();
	SetBranchImm(pc);
}

#endif

} } }
