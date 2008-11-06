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
* Jump to target                                         *
* Format:  OP target                                     *
*********************************************************/
#ifndef JUMP_RECOMPILE

REC_SYS(J);
REC_SYS(JAL);
REC_SYS(JR);
REC_SYS(JALR);

#else

////////////////////////////////////////////////////
void recJ( void ) 
{
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
	SetBranchReg( _Rs_ );
}

////////////////////////////////////////////////////
void recJALR( void ) 
{
    _allocX86reg(ESI, X86TYPE_PCWRITEBACK, 0, MODE_WRITE);
	_eeMoveGPRtoR(ESI, _Rs_);

	if ( _Rd_ ) {
		_deleteEEreg(_Rd_, 0);
		GPR_SET_CONST(_Rd_);
		g_cpuConstRegs[_Rd_].UL[0] = pc + 4;
		g_cpuConstRegs[_Rd_].UL[1] = 0;
	}

	_clearNeededX86regs();
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

#endif // PCSX2_NORECBUILD
