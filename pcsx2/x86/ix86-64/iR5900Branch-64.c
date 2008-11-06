/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

// stop compiling if NORECBUILD build (only for Visual Studio)
#if !(defined(_MSC_VER) && defined(PCSX2_NORECBUILD))

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Common.h"
#include "InterTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"


#ifdef _WIN32
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

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

u32 target;
////////////////////////////////////////////////////
void recBEQ( void ) 
{
    u32 branchTo = ((s32)_Imm_ * 4) + pc;

	if ( _Rs_ == _Rt_ )
	{
		_clearNeededX86regs();
		_clearNeededXMMregs();
		recompileNextInstruction(1);
		SetBranchImm( branchTo );
	}
	else
	{
        MOV64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
        CMP64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
        j32Ptr[ 0 ] = JNE32( 0 );

        _clearNeededX86regs();
		_clearNeededXMMregs();

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
}

////////////////////////////////////////////////////
void recBNE( void ) 
{
    u32 branchTo = ((s32)_Imm_ * 4) + pc;

	if ( _Rs_ == _Rt_ )
	{
		_clearNeededX86regs();
		_clearNeededXMMregs();
		recompileNextInstruction(1);
		SetBranchImm(pc);
		return;
	}

    MOV64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
    CMP64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
    j32Ptr[ 0 ] = JE32( 0 );

	_clearNeededX86regs();
	_clearNeededXMMregs();

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

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, offset                                 *
*********************************************************/

////////////////////////////////////////////////////
void recBLTZAL( void ) 
{
    SysPrintf("BLTZAL\n");
    MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
    MOV32ItoM( (uptr)&cpuRegs.pc, pc );
    iFlushCall(FLUSH_EVERYTHING);
    CALLFunc( (u32)BLTZAL );
    branch = 2; 
}

////////////////////////////////////////////////////
void recBGEZAL( void ) 
{
    SysPrintf("BGEZAL\n");
    MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
    MOV32ItoM( (uptr)&cpuRegs.pc, pc );
    iFlushCall(FLUSH_EVERYTHING);
    CALLFunc( (u32)BGEZAL );
    branch = 2; 
}

////////////////////////////////////////////////////
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

    XOR64RtoR(RAX, RAX);
	CMP64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
	j32Ptr[ 0 ] = JL32( 0 );

	_clearNeededX86regs();
	_clearNeededXMMregs();

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
    
    XOR64RtoR(RAX, RAX);
	CMP64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
	j32Ptr[ 0 ] = JGE32( 0 );

	_clearNeededX86regs();
	_clearNeededXMMregs();

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
void recBLTZ( void ) 
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

    XOR64RtoR(RAX, RAX);
	CMP64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
	j32Ptr[ 0 ] = JLE32( 0 );

    _clearNeededX86regs();
	_clearNeededXMMregs();

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

    XOR64RtoR(RAX, RAX);
	CMP64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
	j32Ptr[ 0 ] = JG32( 0 );

	_clearNeededX86regs();
	_clearNeededXMMregs();

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
			_clearNeededX86regs();
			_clearNeededXMMregs();
			recompileNextInstruction(1);
			SetBranchImm( branchTo );
		}
		return;
	}

	_deleteEEreg(_Rs_, 1);

    XOR64RtoR(RAX, RAX);
	CMP64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
	j32Ptr[ 0 ] = JL32( 0 );

	_clearNeededX86regs();
	_clearNeededXMMregs();

	SaveBranchState();
	recompileNextInstruction(1);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] );

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
			_clearNeededX86regs();
			_clearNeededXMMregs();
			recompileNextInstruction(1);
			SetBranchImm( branchTo );
		}
		return;
	}

	_deleteEEreg(_Rs_, 1);

	XOR64RtoR(RAX, RAX);
	CMP64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
	j32Ptr[ 0 ] = JGE32( 0 );

	_clearNeededX86regs();
	_clearNeededXMMregs();

	SaveBranchState();
	recompileNextInstruction(1);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] );

	LoadBranchState();
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

    XOR64RtoR(RAX, RAX);
	CMP64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
	j32Ptr[ 0 ] = JLE32( 0 );

    _clearNeededX86regs();
	_clearNeededXMMregs();

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

	XOR64RtoR(RAX, RAX);
	CMP64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
	j32Ptr[ 0 ] = JG32( 0 );

	_clearNeededX86regs();
	_clearNeededXMMregs();

	SaveBranchState();
	recompileNextInstruction(1);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] );

	LoadBranchState();
	SetBranchImm(pc);
}

////////////////////////////////////////////////////
void recBLTZALL( void ) 
{
	SysPrintf("BLTZALL\n");
	   MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
	   MOV32ItoM( (uptr)&cpuRegs.pc, pc );
	   iFlushCall(FLUSH_EVERYTHING);
	   CALLFunc( (u32)BLTZALL );
	   branch = 2; 
}

////////////////////////////////////////////////////
void recBGEZALL( void ) 
{
    SysPrintf("BGEZALL\n");
	   MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
	   MOV32ItoM( (uptr)&cpuRegs.pc, pc );
	   iFlushCall(FLUSH_EVERYTHING);
	   CALLFunc( (u32)BGEZALL );
	   branch = 2; 
}

////////////////////////////////////////////////////
void recBEQL( void ) 
{
    u32 branchTo = ((s32)_Imm_ * 4) + pc;
    
    MOV64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
    CMP64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
    j32Ptr[ 0 ] = JNE32( 0 );

	_clearNeededX86regs();
	_clearNeededXMMregs();

	SaveBranchState();
	recompileNextInstruction(1);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] ); 
	
	LoadBranchState();
	SetBranchImm(pc);
}

////////////////////////////////////////////////////
void recBNEL( void ) 
{
    u32 branchTo = ((s32)_Imm_ * 4) + pc;

	MOV64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	CMP64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	j32Ptr[ 0 ] = JNE32( 0 );

	_clearNeededX86regs();
	_clearNeededXMMregs();

	SaveBranchState();
	SetBranchImm(pc+4);

	x86SetJ32( j32Ptr[ 0 ] ); 

	// recopy the next inst
	LoadBranchState();
	recompileNextInstruction(1);
	SetBranchImm(branchTo);
}

#endif

#endif // PCSX2_NORECBUILD
