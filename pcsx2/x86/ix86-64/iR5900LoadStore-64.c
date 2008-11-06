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
#include "VU0.h"

#ifdef _WIN32
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

/*********************************************************
* Load and store for GPR                                 *
* Format:  OP rt, offset(base)                           *
*********************************************************/
#ifndef LOADSTORE_RECOMPILE

REC_FUNC(LB);
REC_FUNC(LBU);
REC_FUNC(LH);
REC_FUNC(LHU);
REC_FUNC(LW);
REC_FUNC(LWU);
REC_FUNC(LWL);
REC_FUNC(LWR);
REC_FUNC(LD);
REC_FUNC(LDR);
REC_FUNC(LDL);
REC_FUNC(LQ);
REC_FUNC(SB);
REC_FUNC(SH);
REC_FUNC(SW);
REC_FUNC(SWL);
REC_FUNC(SWR);
REC_FUNC(SD);
REC_FUNC(SDL);
REC_FUNC(SDR);
REC_FUNC(SQ);
REC_FUNC(LWC1);
REC_FUNC(SWC1);
REC_FUNC(LQC2);
REC_FUNC(SQC2);

void SetFastMemory(int bSetFast) {}

#else

static int s_bFastMemory = 0;
void SetFastMemory(int bSetFast)
{
	s_bFastMemory  = bSetFast;
}

u64 retValue;
u64 dummyValue[ 4 ];

////////////////////////////////////////////////////
void recLB( void ) {
	iFlushCall(FLUSH_EVERYTHING);

	MOV32MtoR( X86ARG1, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 ) {
		ADD32ItoR( X86ARG1, _Imm_ );
	}

	if ( _Rt_ ) {
		MOV64ItoR( X86ARG2, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	} else {
		MOV64ItoR( X86ARG2, (uptr)&dummyValue );
	}
	CALLFunc( (uptr)memRead8RS );
}

////////////////////////////////////////////////////
void recLBU( void ) {
	iFlushCall(FLUSH_EVERYTHING);

	MOV32MtoR( X86ARG1, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
   if ( _Imm_ != 0 ) {
	   ADD32ItoR( X86ARG1, _Imm_ );
   }
   if ( _Rt_ ) {
       MOV64ItoR( X86ARG2, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
   } else {
       MOV64ItoR( X86ARG2, (uptr)&dummyValue );
   }
   iFlushCall(FLUSH_EVERYTHING);
   CALLFunc((uptr)memRead8RU );
}

////////////////////////////////////////////////////
void recLH( void ) {
	iFlushCall(FLUSH_EVERYTHING);

   MOV32MtoR( X86ARG1, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
   if ( _Imm_ != 0 ){
       ADD32ItoR( X86ARG1, _Imm_ );
   }
   if ( _Rt_ ) {
       MOV64ItoR( X86ARG2, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
   } else {
       MOV64ItoR( X86ARG2, (uptr)&dummyValue );
   }
	iFlushCall(FLUSH_EVERYTHING);
	CALLFunc((uptr)memRead16RS );
}

////////////////////////////////////////////////////
void recLHU( void ) {
	iFlushCall(FLUSH_EVERYTHING);

	MOV32MtoR( X86ARG1, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 ) {
		ADD32ItoR( X86ARG1, _Imm_ );
	}
    
	if ( _Rt_ ) {
		MOV64ItoR( X86ARG2, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	} else {
		MOV64ItoR( X86ARG2, (uptr)&dummyValue );
	}
	CALLFunc((uptr)memRead16RU );
}

void tests() {
	SysPrintf("Err\n");
}

////////////////////////////////////////////////////
void recLW( void ) {
	int rsreg;
	int rtreg;
	int t0reg;
	int t1reg;
	int t2reg;

	iFlushCall(FLUSH_EVERYTHING);

	MOV32MtoR( X86ARG1, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 ) {
		ADD32ItoR( X86ARG1, _Imm_ );
	}

	if ( _Rt_ ) {
		MOV64ItoR( X86ARG2, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	} else {
		MOV64ItoR( X86ARG2, (uptr)&dummyValue );
	}
	CALLFunc((uptr)memRead32RS );
}

////////////////////////////////////////////////////
void recLWU( void ) {
	iFlushCall(FLUSH_EVERYTHING);

    MOV32MtoR( X86ARG1, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
    if ( _Imm_ != 0 ) {
        ADD32ItoR( X86ARG1, _Imm_ );
    }
    if ( _Rt_ ) {
		MOV64ItoR( X86ARG2, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
    } else {
		MOV64ItoR( X86ARG2, (uptr)&dummyValue );
    }
    CALLFunc((uptr)memRead32RU );
}

void recLWL( void ) 
{
	iFlushCall(FLUSH_EVERYTHING);
    
    MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
    MOV32ItoM( (uptr)&cpuRegs.pc, pc );
    CALLFunc((uptr)LWL );
}

void recLWR( void )
{
	iFlushCall(FLUSH_EVERYTHING);
    
    MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
    MOV32ItoM( (uptr)&cpuRegs.pc, pc );
    CALLFunc((uptr)LWR );
}

void recLD( void )
{
	iFlushCall(FLUSH_EVERYTHING);

    MOV32MtoR( X86ARG1, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
    if ( _Imm_ != 0 ) {
        ADD32ItoR( X86ARG1, _Imm_ );
    }
    if ( _Rt_ ) {
        MOV64ItoR( X86ARG2, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
    }
    else {
		MOV64ItoR( X86ARG2, (uptr)&dummyValue );
    }
    CALLFunc((uptr)memRead64 );
}

void recLDL( void )
{
	iFlushCall(FLUSH_EVERYTHING);
    
	MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (uptr)&cpuRegs.pc, pc );
	CALLFunc((uptr)LDL );
}

void recLDR( void )
{
	iFlushCall(FLUSH_EVERYTHING);
    
    MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
    MOV32ItoM( (uptr)&cpuRegs.pc, pc );
    CALLFunc((uptr)LDR );
}

void recLQ( void )
{
	iFlushCall(FLUSH_EVERYTHING);
    
    MOV32MtoR( X86ARG1, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
    if ( _Imm_ != 0 ) {
        ADD32ItoR( X86ARG1, _Imm_);
    }
    AND32ItoR( X86ARG1, ~0xf );
    
    if ( _Rt_ ) {
		MOV64ItoR( X86ARG2, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
    } else {
		MOV64ItoR( X86ARG2, (uptr)&dummyValue );
    }
    CALLFunc((uptr)memRead128 );
}

////////////////////////////////////////////////////
void recSB( void ) {
	iFlushCall(FLUSH_EVERYTHING);

	MOV32MtoR( X86ARG1, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
    if ( _Imm_ != 0 ) {
        ADD32ItoR( X86ARG1, _Imm_);
    }
	MOV32MtoR( X86ARG2, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
    CALLFunc((uptr)memWrite8 );
}

void recSH( void )
{
	iFlushCall(FLUSH_EVERYTHING);
    
    MOV32MtoR( X86ARG1, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
    if ( _Imm_ != 0 ) {
        ADD32ItoR( X86ARG1, _Imm_ );
    }
	MOV32MtoR( X86ARG2, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
    CALLFunc((uptr)memWrite16 );
}

void recSW( void )
{
	iFlushCall(FLUSH_EVERYTHING);

    MOV32MtoR( X86ARG1, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
    if ( _Imm_ != 0 ) {
        ADD32ItoR( X86ARG1, _Imm_ );
	   }
	MOV32MtoR( X86ARG2, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
    CALLFunc((uptr)memWrite32 );
}

void recSWL( void )
{
	iFlushCall(FLUSH_EVERYTHING);
    
    MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
    MOV32ItoM( (uptr)&cpuRegs.pc, pc );
    CALLFunc((uptr)SWL );
}

void recSWR( void )
{
	iFlushCall(FLUSH_EVERYTHING);
    
    MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
    MOV32ItoM( (uptr)&cpuRegs.pc, pc );
   	CALLFunc((uptr)SWR );
}

void recSD( void )
{
	iFlushCall(FLUSH_EVERYTHING);
    
    MOV32MtoR( X86ARG1, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
    if ( _Imm_ != 0 ) {
        ADD32ItoR( X86ARG1, _Imm_ );
    }
	MOV64MtoR( X86ARG2, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
    CALLFunc((uptr)memWrite64 );
}

void recSDL( void )
{
	iFlushCall(FLUSH_EVERYTHING);
    
    MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
    MOV32ItoM( (uptr)&cpuRegs.pc, pc );
    CALLFunc((uptr)SDL );
}

void recSDR( void )
{
	iFlushCall(FLUSH_EVERYTHING);
    
    MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
    MOV32ItoM( (uptr)&cpuRegs.pc, pc );
    CALLFunc((uptr)SDR );
}

void recSQ( void )
{
	iFlushCall(FLUSH_EVERYTHING);

    MOV32MtoR( X86ARG1, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
    if ( _Imm_ != 0 ) {
        ADD32ItoR( X86ARG1, _Imm_ );
    }
    AND32ItoR( X86ARG1, ~0xf );
    
	MOV32ItoR( X86ARG2, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
    CALLFunc((uptr)memWrite128 );
}

#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_

// Load and store for COP1
// Format:  OP rt, offset(base)
void recLWC1( void )
{
    iFlushCall(FLUSH_EVERYTHING);
    
    MOV32MtoR( X86ARG1, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
    if ( _Imm_ != 0 )
        ADD32ItoR( X86ARG1, _Imm_ );
    MOV64ItoR( X86ARG2, (uptr)&fpuRegs.fpr[ _Ft_ ].UL );

    CALLFunc((uptr)memRead32 );
}

void recSWC1( void )
{
    iFlushCall(FLUSH_EVERYTHING);
    
    MOV32MtoR( X86ARG1, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
    if ( _Imm_ != 0 )
        ADD32ItoR( X86ARG1, _Imm_ );
    
    MOV32MtoR( X86ARG2, (uptr)&fpuRegs.fpr[ _Ft_ ].UL );

    CALLFunc((uptr)memWrite32 );
}

void recLQC2( void ) 
{
    iFlushCall(FLUSH_EVERYTHING);

    MOV32MtoR( X86ARG1, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
    if ( _Imm_ != 0 )
        ADD32ItoR( X86ARG1, _Imm_);

    if ( _Rt_ )
        MOV64ItoR(X86ARG2, (uptr)&VU0.VF[_Ft_].UD[0] );
    else
        MOV64ItoR(X86ARG2, (uptr)&dummyValue );
    
    CALLFunc((uptr)memRead128 );
}

void recSQC2( void ) 
{
    iFlushCall(FLUSH_EVERYTHING);

    MOV32MtoR( X86ARG1, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
    if ( _Imm_ != 0 )
        ADD32ItoR( X86ARG1, _Imm_ );

    MOV64ItoR(X86ARG2, (uptr)&VU0.VF[_Ft_].UD[0] );
    
    CALLFunc((uptr)memWrite128 );
}

#endif

#endif // PCSX2_NORECBUILD
