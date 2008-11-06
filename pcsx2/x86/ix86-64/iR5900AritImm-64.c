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
* Arithmetic with immediate operand                      *
* Format:  OP rt, rs, immediate                          *
*********************************************************/
#ifndef ARITHMETICIMM_RECOMPILE

REC_FUNC(ADDI);
REC_FUNC(ADDIU);
REC_FUNC(DADDI);
REC_FUNC(DADDIU);
REC_FUNC(ANDI);
REC_FUNC(ORI);
REC_FUNC(XORI);

REC_FUNC(SLTI);
REC_FUNC(SLTIU);

#else

////////////////////////////////////////////////////
void recADDI( void ) {
	if (!_Rt_) return;

	MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if (_Imm_ != 0) {
		ADD32ItoR( EAX, _Imm_ );
	}

	CDQ( );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
}

////////////////////////////////////////////////////
void recADDIU( void ) 
{
	recADDI( );
}

////////////////////////////////////////////////////
void recDADDI( void ) {
	int rsreg;
	int rtreg;

	if (!_Rt_) return;

	MOV64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 ) {
        ADD64ItoR( EAX, _Imm_ );
    }
	MOV64RtoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], RAX );
}

////////////////////////////////////////////////////
void recDADDIU( void ) 
{
	recDADDI( );
}

////////////////////////////////////////////////////
void recSLTIU( void )
{
   if ( ! _Rt_ ) return;
   
	MOV64MtoR(RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
    CMP64I32toR(RAX, _Imm_);
    SETB8R   (EAX);
    AND64I32toR(EAX, 0xff);
	MOV64RtoM((uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX);
}

////////////////////////////////////////////////////
void recSLTI( void )
{
    if ( ! _Rt_ )
        return;
    
	MOV64MtoR(RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
    CMP64I32toR(RAX, _Imm_);
    SETL8R   (EAX);
    AND64I32toR(EAX, 0xff);
	MOV64RtoM((uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX);
}

////////////////////////////////////////////////////
void recANDI( void ) {
	if (!_Rt_) return;
    
	if ( _ImmU_ != 0 ) {
		if (_Rs_ == _Rt_) {
			MOV32ItoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
			AND32ItoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], _ImmU_ );
		}
        else {
			MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
			AND32ItoR( EAX, _ImmU_ );
			MOV32ItoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
			MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		}
	}
    else {
        MOV32ItoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
        MOV32ItoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], 0 );
    }
}

////////////////////////////////////////////////////
void recORI( void ) {
	if (!_Rt_) return;

	if (_Rs_ == _Rt_) {
		OR32ItoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ], _ImmU_ );
	} else {
		MOV64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
        if ( _ImmU_ != 0 ) {
            OR64ItoR( RAX, _ImmU_ );
        }
		MOV64RtoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ], RAX );
	}
}

////////////////////////////////////////////////////
void recXORI( void ) {
	if (!_Rt_) return;

	MOV64MtoR( RAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	XOR64ItoR( RAX, _ImmU_ );
	MOV64RtoM( (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ], RAX );
}

#endif

#endif // PCSX2_NORECBUILD
