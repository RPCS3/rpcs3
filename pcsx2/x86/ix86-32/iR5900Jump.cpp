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
#include "ix86/ix86.h"
#include "iR5900.h"

namespace R5900 { 
namespace Dynarec { 
namespace OpcodeImpl
{

/*********************************************************
* Jump to target                                         *
* Format:  OP target                                     *
*********************************************************/
#ifndef JUMP_RECOMPILE

namespace Interp = R5900::Interpreter::OpcodeImpl;

REC_SYS(J);
REC_SYS(JAL);
REC_SYS(JR);
REC_SYS(JALR);

#else

////////////////////////////////////////////////////
void recJ( void ) 
{	
	// SET_FPUSTATE;
	u32 newpc = (_Target_ << 2) + ( pc & 0xf0000000 );
	recompileNextInstruction(1);
	SetBranchImm(newpc);
}

////////////////////////////////////////////////////
void recJAL( void ) 
{
	u32 newpc = (_Target_ << 2) + ( pc & 0xf0000000 );
	_deleteEEreg(31, 0);
	GPR_SET_CONST(31);
	g_cpuConstRegs[31].UL[0] = pc + 4;
	g_cpuConstRegs[31].UL[1] = 0;

	recompileNextInstruction(1);
	SetBranchImm(newpc);
}

/*********************************************************
* Register jump                                          *
* Format:  OP rs, rd                                     *
*********************************************************/

////////////////////////////////////////////////////
void recJR( void ) 
{
	SetBranchReg( _Rs_);
}

////////////////////////////////////////////////////
void recJALR( void ) 
{
	_allocX86reg(ESI, X86TYPE_PCWRITEBACK, 0, MODE_WRITE);
	_eeMoveGPRtoR(ESI, _Rs_);
	// uncomment when there are NO instructions that need to call interpreter
//	int mmreg;
//	if( GPR_IS_CONST1(_Rs_) )
//		MOV32ItoM( (u32)&cpuRegs.pc, g_cpuConstRegs[_Rs_].UL[0] );
//	else {
//		int mmreg;
//		
//		if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, _Rs_, MODE_READ)) >= 0 ) {
//			SSE_MOVSS_XMM_to_M32((u32)&cpuRegs.pc, mmreg);
//		}
//		else if( (mmreg = _checkMMXreg(MMX_GPR+_Rs_, MODE_READ)) >= 0 ) {
//			MOVDMMXtoM((u32)&cpuRegs.pc, mmreg);
//			SetMMXstate();
//		}
//		else {
//			MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
//			MOV32RtoM((u32)&cpuRegs.pc, EAX);
//		}
//	}

	if ( _Rd_ ) 
	{
		_deleteEEreg(_Rd_, 0);
		GPR_SET_CONST(_Rd_);
		g_cpuConstRegs[_Rd_].UL[0] = pc + 4;
		g_cpuConstRegs[_Rd_].UL[1] = 0;
	}

	_clearNeededMMXregs();
	_clearNeededXMMregs();
	recompileNextInstruction(1);

	if( x86regs[ESI].inuse ) {
		assert( x86regs[ESI].type == X86TYPE_PCWRITEBACK );
		MOV32RtoM((int)&cpuRegs.pc, ESI);
		x86regs[ESI].inuse = 0;
	}
	else {
		MOV32MtoR(EAX, (u32)&g_recWriteback);
		MOV32RtoM((int)&cpuRegs.pc, EAX);
	}

	SetBranchReg(0xffffffff);
}

#endif

} } }
