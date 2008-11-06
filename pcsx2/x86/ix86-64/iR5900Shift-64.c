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
* Shift arithmetic with constant shift                   *
* Format:  OP rd, rt, sa                                 *
*********************************************************/
#ifndef SHIFT_RECOMPILE

REC_FUNC(SLL);
REC_FUNC(SRL);
REC_FUNC(SRA);
REC_FUNC(DSLL);
REC_FUNC(DSRL);
REC_FUNC(DSRA);
REC_FUNC(DSLL32);
REC_FUNC(DSRL32);
REC_FUNC(DSRA32);

REC_FUNC(SLLV);
REC_FUNC(SRLV);
REC_FUNC(SRAV);
REC_FUNC(DSLLV);
REC_FUNC(DSRLV);
REC_FUNC(DSRAV);

#else


////////////////////////////////////////////////////
void recDSRA( void ) {
	if (!_Rd_) return;

	MOV64MtoR( RAX, (u64)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	if ( _Sa_ != 0 ) {
		SAR64ItoR( RAX, _Sa_ );
	}
	MOV64RtoM( (u64)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], RAX );
}

////////////////////////////////////////////////////
void recDSRA32(void) {
	if (!_Rd_) return;

	MOV64MtoR( RAX, (u64)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	SAR64ItoR( RAX, _Sa_ + 32 );
	MOV64RtoM( (u64)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], RAX );
}

////////////////////////////////////////////////////
void recSLL(void) {
	if (!_Rd_) return;

	MOV32MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
	if (_Sa_ != 0) {
		SHL32ItoR(EAX, _Sa_);
	}
	CDQ();
	MOV32RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UL[0], EAX);
	MOV32RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UL[1], EDX);
}

////////////////////////////////////////////////////
void recSRL(void) {
	if (!_Rd_) return;

	MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
	if (_Sa_ != 0) {
		SHR32ItoR(EAX, _Sa_);
	}
	CDQ();
	MOV32RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UL[0], EAX);
	MOV32RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UL[1], EDX);
}

////////////////////////////////////////////////////
void recSRA(void) {
	if (!_Rd_) return;

	MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	if ( _Sa_ != 0 ) {
		SAR32ItoR( EAX, _Sa_);
	}
	CDQ();
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
}

////////////////////////////////////////////////////
void recDSLL(void) {
	if (!_Rd_) return;

	MOV64MtoR( RAX, (u64)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	if ( _Sa_ != 0 ) {
		SHL64ItoR( RAX, _Sa_ );
	}
	MOV64RtoM( (u64)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], RAX );
}

////////////////////////////////////////////////////
void recDSRL( void ) {
	if (!_Rd_) return;

	MOV64MtoR( RAX, (u64)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	if ( _Sa_ != 0 ) {
		SHR64ItoR( RAX, _Sa_ );
	}
	MOV64RtoM( (u64)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], RAX );
}

////////////////////////////////////////////////////
void recDSLL32(void) {

	if (!_Rd_) return;

	MOV64MtoR( RAX, (u64)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	SHL64ItoR( RAX, _Sa_ + 32 );
	MOV64RtoM( (u64)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], RAX );
}

////////////////////////////////////////////////////
void recDSRL32( void ) {

	if (!_Rd_) return;

	MOV64MtoR( RAX, (u64)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	SHR64ItoR( RAX, _Sa_ + 32 );
	MOV64RtoM( (u64)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], RAX );
}

/*********************************************************
* Shift arithmetic with variant register shift           *
* Format:  OP rd, rt, rs                                 *
*********************************************************/


////////////////////////////////////////////////////
void recSLLV( void ) {
	if (!_Rd_) return;

	MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	if ( _Rs_ != 0 ) {
	   MOV32MtoR( ECX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   AND32ItoR( ECX, 0x1f );
	   SHL32CLtoR( EAX );
	}
	CDQ();
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
}

////////////////////////////////////////////////////
void recSRLV( void ) {
	if (!_Rd_) return;


   MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
   if ( _Rs_ != 0 )	
   {
	   MOV32MtoR( ECX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   AND32ItoR( ECX, 0x1f );
	   SHR32CLtoR( EAX );
   }
   CDQ( );
   MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
   MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
}

////////////////////////////////////////////////////
void recSRAV( void ) {
	if (!_Rd_) return;

   MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
   if ( _Rs_ != 0 )
   {
	   MOV32MtoR( ECX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   AND32ItoR( ECX, 0x1f );
	   SAR32CLtoR( EAX );
   }
   CDQ( );
   MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
   MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
}

////////////////////////////////////////////////////
void recDSLLV( void ) {
	if (!_Rd_) return;

	MOV64MtoR( RAX, (u64)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
   if ( _Rs_ != 0 )
   {
	   MOV32MtoR( ECX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   AND32ItoR( ECX, 0x3f );
	   SHL64CLtoR( RAX );
   }
	MOV64RtoM( (u64)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], RAX );
}

////////////////////////////////////////////////////
void recDSRLV( void ) {

	if (!_Rd_) return;

	MOV64MtoR( RAX, (u64)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
   if ( _Rs_ != 0 )
   {
	   MOV32MtoR( ECX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   AND32ItoR( ECX, 0x3f );
	   SHR64CLtoR( RAX );
   }
	MOV64RtoM( (u64)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], RAX );
}

////////////////////////////////////////////////////
void recDSRAV( void ) {
	if (!_Rd_) return;

	MOV64MtoR( RAX, (u64)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
   if ( _Rs_ != 0 )
   {
	   MOV32MtoR( ECX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   AND32ItoR( ECX, 0x3f );
	   SAR64CLtoR( RAX );
   }
	MOV64RtoM( (u64)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], RAX );
}

#endif


#endif // PCSX2_NORECBUILD
