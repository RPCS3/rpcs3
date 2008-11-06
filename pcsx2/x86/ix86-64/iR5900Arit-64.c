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
* Register arithmetic                                    *
* Format:  OP rd, rs, rt                                 *
*********************************************************/

#ifndef ARITHMETIC_RECOMPILE

REC_FUNC(ADD);
REC_FUNC(ADDU);
REC_FUNC(DADD);
REC_FUNC(DADDU);
REC_FUNC(SUB);
REC_FUNC(SUBU);
REC_FUNC(DSUB);
REC_FUNC(DSUBU);
REC_FUNC(AND);
REC_FUNC(OR);
REC_FUNC(XOR);
REC_FUNC(NOR);
REC_FUNC(SLT);
REC_FUNC(SLTU);

#else

////////////////////////////////////////////////////
void recADD( void ) {
	if (!_Rd_) return;

	MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if (_Rt_ != 0) {
		   ADD32MtoR( EAX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	}
	CDQ( );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
}

////////////////////////////////////////////////////
void recADDU( void ) 
{
	recADD( );
}

////////////////////////////////////////////////////
void recDADD( void ) {
	if (!_Rd_) return;
    
    MOV64MtoR( EAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
    if ( _Rt_ != 0 ) {
        ADD64MtoR( EAX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
    }
    MOV64RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
}

////////////////////////////////////////////////////
void recDADDU( void )
{
	recDADD( );
}

////////////////////////////////////////////////////
void recSUB( void ) {
	MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
    if ( _Rt_ != 0 ) {
        SUB32MtoR( EAX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
    }
    CDQ( );
    MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
    MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
}

////////////////////////////////////////////////////
void recSUBU( void ) 
{
	recSUB( );
}

////////////////////////////////////////////////////
void recDSUB( void ) {
	if (!_Rd_) return;
    
    MOV64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
    if ( _Rt_ != 0 ) {
        SUB64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
    }
    MOV64RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], RAX );
}

////////////////////////////////////////////////////
void recDSUBU( void ) 
{
	recDSUB( );
}

////////////////////////////////////////////////////
void recAND( void ) {
	if (!_Rd_) return;

	if (_Rt_ == _Rd_) { // Rd&= Rs
		MOV64MtoR(RAX, (uptr)&cpuRegs.GPR.r[_Rs_].UL[0]);
		AND64RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UL[0], RAX);
	} else if (_Rs_ == _Rd_) { // Rd&= Rt
		MOV64MtoR(RAX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
		AND64RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UL[0], RAX);
	} else { // Rd = Rs & Rt
		MOV64MtoR(RAX, (uptr)&cpuRegs.GPR.r[_Rs_].UL[0]);
		AND64MtoR(RAX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
		MOV64RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UL[0], RAX);
	}
}

////////////////////////////////////////////////////
void recOR( void ) {
	if (!_Rd_) return;

	if ( ( _Rs_ == 0 ) && ( _Rt_ == 0  ) ) {
		XOR64RtoR(RAX, RAX);
		MOV64RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[0], RAX );
	} else if ( _Rs_ == 0 ) {
        MOV64MtoR(RAX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
        MOV64RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UL[0], RAX);
    } 
	else if ( _Rt_ == 0 ) {
		MOV64MtoR(RAX, (uptr)&cpuRegs.GPR.r[_Rs_].UL[0]);
		MOV64RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UL[0], RAX);
    }
    else {
		MOV64MtoR(RAX, (uptr)&cpuRegs.GPR.r[_Rs_].UL[0]);
		OR64MtoR(RAX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
		MOV64RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UL[0], RAX);
    }
}

////////////////////////////////////////////////////
void recXOR( void ) {
	if (!_Rd_) return;

	MOV64MtoR(RAX, (uptr)&cpuRegs.GPR.r[_Rs_].UL[0]);
	XOR64MtoR(RAX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
	MOV64RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UL[0], RAX);
}

////////////////////////////////////////////////////
void recNOR( void ) {
	if (!_Rd_) return;

	MOV64MtoR(RAX, (uptr)&cpuRegs.GPR.r[_Rs_].UL[0]);
	OR64MtoR(RAX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
	NOT64R(RAX);
	MOV64RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UL[0], RAX);
}

////////////////////////////////////////////////////
void recSLT( void ) {
	if (!_Rd_) return;

	MOV64MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UL[0]);
    CMP64MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
    SETL8R   (EAX);
    AND64I32toR(EAX, 0xff);
	MOV64RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UL[0], EAX);
}

////////////////////////////////////////////////////
void recSLTU( void ) {
	if (!_Rd_) return;

	MOV64MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UL[0]);
    CMP64MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
	SBB64RtoR(EAX, EAX);
	NEG64R   (EAX);
	MOV64RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UL[0], RAX);
}

#endif

#endif // PCSX2_NORECBUILD
