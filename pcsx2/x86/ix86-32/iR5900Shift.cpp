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
* Shift arithmetic with constant shift                   *
* Format:  OP rd, rt, sa                                 *
*********************************************************/
#ifndef SHIFT_RECOMPILE

namespace Interp = R5900::Interpreter::OpcodeImpl;

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

#elif defined(EE_CONST_PROP)

//// SLL
void recSLL_const()
{
	g_cpuConstRegs[_Rd_].SD[0] = (s32)(g_cpuConstRegs[_Rt_].UL[0] << _Sa_);
}

void recSLLs_(int info, int sa)
{
	int rtreg, rdreg, t0reg;
	assert( !(info & PROCESS_EE_XMM) );

	if( info & PROCESS_EE_MMX ) {
		rtreg = EEREC_T;
		rdreg = EEREC_D;
	}
	else if( g_pCurInstInfo->regs[_Rd_]&EEINST_MMX ) {
		_addNeededMMXreg(MMX_GPR+_Rd_);
		rtreg = rdreg = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
		SetMMXstate();
		MOVDMtoMMX(rtreg, (u32)&cpuRegs.GPR.r[_Rt_].UL[0]);
	}
	else {
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		if ( sa != 0 )
		{
			SHL32ItoR( EAX, sa );
		}

		if( EEINST_ISLIVE1(_Rd_) ) {
			CDQ( );
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
		}
		else {
			EEINST_RESETHASLIVE1(_Rd_);
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		}
		return;
	}

	if( rtreg != rdreg ) MOVQRtoR(rdreg, rtreg);

	if( !EEINST_ISLIVE1(_Rd_) ) {
		EEINST_RESETHASLIVE1(_Rd_);
		PSLLDItoR(rdreg, sa);
		return;
	}

    if ( sa != 0 ) {
		t0reg = _allocMMXreg(-1, MMX_TEMP, 0);	

		// it is a signed shift
		PSLLDItoR(rdreg, sa);
		MOVQRtoR(t0reg, rdreg);
		PSRADItoR(t0reg, 31);

		// take lower dword of rdreg and lower dword of t0reg		
		PUNPCKLDQRtoR(rdreg, t0reg);
		_freeMMXreg(t0reg);
	}
	else {
		if( EEINST_ISLIVE1(_Rd_) ) _signExtendGPRtoMMX(rdreg, _Rd_, 0);
		else EEINST_RESETHASLIVE1(_Rd_);
	}
}

void recSLL_(int info)
{
	recSLLs_(info, _Sa_);
	EEINST_SETSIGNEXT(_Rd_);
}

EERECOMPILE_CODEX(eeRecompileCode2, SLL);

//// SRL
void recSRL_const()
{
	g_cpuConstRegs[_Rd_].SD[0] = (s32)(g_cpuConstRegs[_Rt_].UL[0] >> _Sa_);
}

void recSRLs_(int info, int sa) 
{
	int rtreg, rdreg;
	assert( !(info & PROCESS_EE_XMM) );

	if( info & PROCESS_EE_MMX ) {
		rtreg = EEREC_T;
		rdreg = EEREC_D;
	}
	else if( (g_pCurInstInfo->regs[_Rt_]&EEINST_MMX) || (g_pCurInstInfo->regs[_Rd_]&EEINST_MMX) ) {
		_addNeededMMXreg(MMX_GPR+_Rd_);
		rtreg = rdreg = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
		SetMMXstate();
		MOVDMtoMMX(rtreg, (u32)&cpuRegs.GPR.r[_Rt_].UL[0]);
	}
	else {
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		if ( sa != 0 ) SHR32ItoR( EAX, sa);

		if( EEINST_ISLIVE1(_Rd_) ) {
			CDQ( );
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
		}
		else {
			EEINST_RESETHASLIVE1(_Rd_);
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		}
		return;
	}

	if( rtreg != rdreg ) MOVQRtoR(rdreg, rtreg);

	if( !EEINST_ISLIVE1(_Rd_) ) {
		EEINST_RESETHASLIVE1(_Rd_);
		PSRLDItoR(rdreg, sa);
		return;
	}

    if ( sa != 0 ) {
		// rdreg already sign extended
		PSLLQItoR(rdreg, 32);
		PSRLQItoR(rdreg, 32+sa);
//		t0reg = _allocMMXreg(-1, MMX_TEMP, 0);	
//
//		// it is a signed shift
//		PSRLDItoR(rdreg, sa);
//		MOVQRtoR(t0reg, rdreg);
//		PSRADItoR(t0reg, 31);
//
//		 take lower dword of rdreg and lower dword of t0reg		
//		PUNPCKLDQRtoR(rdreg, t0reg);
//		_freeMMXreg(t0reg);
	}
}

void recSRL_(int info) 
{
	recSRLs_(info, _Sa_);
	EEINST_SETSIGNEXT(_Rd_);
}

EERECOMPILE_CODEX(eeRecompileCode2, SRL);

//// SRA
void recSRA_const()
{
	g_cpuConstRegs[_Rd_].SD[0] = (s32)(g_cpuConstRegs[_Rt_].SL[0] >> _Sa_);
}

void recSRAs_(int info, int sa) 
{
	int rtreg, rdreg, t0reg;
	assert( !(info & PROCESS_EE_XMM) );

	if( info & PROCESS_EE_MMX ) {
		rtreg = EEREC_T;
		rdreg = EEREC_D;
	}
	else if( (g_pCurInstInfo->regs[_Rt_]&EEINST_MMX) || (g_pCurInstInfo->regs[_Rd_]&EEINST_MMX) ) {
		_addNeededMMXreg(MMX_GPR+_Rd_);
		rtreg = rdreg = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
		SetMMXstate();
		MOVDMtoMMX(rtreg, (u32)&cpuRegs.GPR.r[_Rt_].UL[0]);
	}
	else {
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		if ( sa != 0 ) SAR32ItoR( EAX, sa);

		if( EEINST_ISLIVE1(_Rd_) ) {
			CDQ();
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
		}
		else {
			EEINST_RESETHASLIVE1(_Rd_);
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		}
		return;
	}

	if( rtreg != rdreg ) MOVQRtoR(rdreg, rtreg);

	if( EEINST_ISSIGNEXT(_Rt_) && EEINST_HASLIVE1(_Rt_) ) {
		PSRADItoR(rdreg, sa);
		return;
	}

	if( !EEINST_ISLIVE1(_Rd_) ) {
		EEINST_RESETHASLIVE1(_Rd_);
		PSRADItoR(rdreg, sa);
		return;
	}

    if ( sa != 0 ) {
		t0reg = _allocMMXreg(-1, MMX_TEMP, 0);	

		// it is a signed shift
		PSRADItoR(rdreg, sa);
		MOVQRtoR(t0reg, rdreg);
		PSRADItoR(rdreg, 31);

		// take lower dword of rdreg and lower dword of t0reg		
		PUNPCKLDQRtoR(t0reg, rdreg);
		
		// swap regs
		mmxregs[t0reg] = mmxregs[rdreg];
		mmxregs[rdreg].inuse = 0;
	}
}

void recSRA_(int info) 
{
	recSRAs_(info, _Sa_);
	EEINST_SETSIGNEXT(_Rd_);
}

EERECOMPILE_CODEX(eeRecompileCode2, SRA);

////////////////////////////////////////////////////
void recDSLL_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = (u64)(g_cpuConstRegs[_Rt_].UD[0] << _Sa_);
}

void recDSLLs_(int info, int sa)
{
	int rtreg, rdreg;
	assert( !(info & PROCESS_EE_XMM) );

	if( info & PROCESS_EE_MMX ) {
		rtreg = EEREC_T;
		rdreg = EEREC_D;
	}
	else {
		_addNeededMMXreg(MMX_GPR+_Rt_);
		_addNeededMMXreg(MMX_GPR+_Rd_);
		rtreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_READ);
		rdreg = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
		SetMMXstate();
	}

	if( rtreg != rdreg ) MOVQRtoR(rdreg, rtreg);
	PSLLQItoR(rdreg, sa);
}

void recDSLL_(int info)
{
	recDSLLs_(info, _Sa_);
}

EERECOMPILE_CODEX(eeRecompileCode2, DSLL);

////////////////////////////////////////////////////
void recDSRL_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = (u64)(g_cpuConstRegs[_Rt_].UD[0] >> _Sa_);
}

void recDSRLs_(int info, int sa)
{
	int rtreg, rdreg;
	assert( !(info & PROCESS_EE_XMM) );

	if( info & PROCESS_EE_MMX ) {
		rtreg = EEREC_T;
		rdreg = EEREC_D;
	}
	else {
		_addNeededMMXreg(MMX_GPR+_Rt_);
		_addNeededMMXreg(MMX_GPR+_Rd_);
		rtreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_READ);
		rdreg = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
		SetMMXstate();
	}

	if( rtreg != rdreg ) MOVQRtoR(rdreg, rtreg);
	PSRLQItoR(rdreg, sa);
}

void recDSRL_(int info) 
{
	recDSRLs_(info, _Sa_);
}

EERECOMPILE_CODEX(eeRecompileCode2, DSRL);

//// DSRA
void recDSRA_const()
{
	g_cpuConstRegs[_Rd_].SD[0] = (u64)(g_cpuConstRegs[_Rt_].SD[0] >> _Sa_);
}

void recDSRAs_(int info, int sa)
{
	int rtreg, rdreg, t0reg;
	assert( !(info & PROCESS_EE_XMM) );

	if( info & PROCESS_EE_MMX ) {
		rtreg = EEREC_T;
		rdreg = EEREC_D;
	}
	else {
		_addNeededMMXreg(MMX_GPR+_Rt_);
		_addNeededMMXreg(MMX_GPR+_Rd_);
		rtreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_READ);
		rdreg = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
		SetMMXstate();
	}

	if( rtreg != rdreg ) MOVQRtoR(rdreg, rtreg);

	if( EEINST_ISSIGNEXT(_Rt_) && EEINST_HASLIVE1(_Rt_) ) {
		PSRADItoR(rdreg, sa);
		return;
	}

	if( !EEINST_ISLIVE1(_Rd_) ) {
		EEINST_RESETHASLIVE1(_Rd_);
		PSRLQItoR(rdreg, sa);
		return;
	}

    if ( sa != 0 ) {
		t0reg = _allocMMXreg(-1, MMX_TEMP, 0);	
		MOVQRtoR(t0reg, rtreg);

		// it is a signed shift
		PSRADItoR(t0reg, sa);
		PSRLQItoR(rdreg, sa);

		PUNPCKHDQRtoR(t0reg, t0reg); // shift to lower
		// take lower dword of rdreg and lower dword of t0reg		
		PUNPCKLDQRtoR(rdreg, t0reg);

		_freeMMXreg(t0reg);
	}
}

void recDSRA_(int info)
{
	recDSRAs_(info, _Sa_);
}

EERECOMPILE_CODEX(eeRecompileCode2, DSRA);

///// DSLL32
void recDSLL32_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = (u64)(g_cpuConstRegs[_Rt_].UD[0] << (_Sa_+32));
}

void recDSLL32s_(int info, int sa)
{
	int rtreg, rdreg;
	assert( !(info & PROCESS_EE_XMM) );

	if( info & PROCESS_EE_MMX ) {
		rtreg = EEREC_T;
		rdreg = EEREC_D;
	}
	else if( (g_pCurInstInfo->regs[_Rt_]&EEINST_MMX) || (g_pCurInstInfo->regs[_Rd_]&EEINST_MMX) ) {
		_addNeededMMXreg(MMX_GPR+_Rt_);
		_addNeededMMXreg(MMX_GPR+_Rd_);
		rtreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_READ);
		rdreg = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
		SetMMXstate();
	}
	else {
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		if ( sa != 0 )
		{
			SHL32ItoR( EAX, sa );
		}
		MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], 0 );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EAX );  
		return;
	}

	if( rtreg != rdreg ) MOVQRtoR(rdreg, rtreg);
	PSLLQItoR(rdreg, sa+32);
}

void recDSLL32_(int info)
{
	recDSLL32s_(info, _Sa_);
}

EERECOMPILE_CODEX(eeRecompileCode2, DSLL32);

//// DSRL32
void recDSRL32_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = (u64)(g_cpuConstRegs[_Rt_].UD[0] >> (_Sa_+32));
}

void recDSRL32s_(int info, int sa)
{
	int rtreg, rdreg;
	assert( !(info & PROCESS_EE_XMM) );

	if( info & PROCESS_EE_MMX ) {
		rtreg = EEREC_T;
		rdreg = EEREC_D;
	}
	else if( (g_pCurInstInfo->regs[_Rt_]&EEINST_MMX) || (g_pCurInstInfo->regs[_Rd_]&EEINST_MMX) ) {
		_addNeededMMXreg(MMX_GPR+_Rt_);
		_addNeededMMXreg(MMX_GPR+_Rd_);
		rtreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_READ);
		rdreg = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
		SetMMXstate();
	}
	else {
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
		if ( sa != 0 ) SHR32ItoR( EAX, sa );
		
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		if( EEINST_ISLIVE1(_Rd_) ) MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], 0 );  
		else EEINST_RESETHASLIVE1(_Rd_);
		return;
	}

	if( rtreg != rdreg ) MOVQRtoR(rdreg, rtreg);
	PSRLQItoR(rdreg, sa+32);
}

void recDSRL32_(int info)
{
	recDSRL32s_(info, _Sa_);
}

EERECOMPILE_CODEX(eeRecompileCode2, DSRL32);

//// DSRA32
void recDSRA32_const()
{
	g_cpuConstRegs[_Rd_].SD[0] = (u64)(g_cpuConstRegs[_Rt_].SD[0] >> (_Sa_+32));
}

void recDSRA32s_(int info, int sa)
{
	int rtreg, rdreg, t0reg;
	assert( !(info & PROCESS_EE_XMM) );

	if( info & PROCESS_EE_MMX ) {
		rtreg = EEREC_T;
		rdreg = EEREC_D;
	}
	else if( (g_pCurInstInfo->regs[_Rt_]&EEINST_MMX) || (g_pCurInstInfo->regs[_Rd_]&EEINST_MMX) ) {
		_addNeededMMXreg(MMX_GPR+_Rt_);
		_addNeededMMXreg(MMX_GPR+_Rd_);
		rtreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_READ);
		rdreg = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
		SetMMXstate();
	}
	else {
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
		CDQ( );
		if ( sa != 0 ) SAR32ItoR( EAX, sa );
		
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		if( EEINST_ISLIVE1(_Rd_) ) MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );  
		else EEINST_RESETHASLIVE1(_Rd_);
		return;
	}

	if( rtreg != rdreg ) MOVQRtoR(rdreg, rtreg);

	if( EEINST_ISSIGNEXT(_Rt_) && EEINST_HASLIVE1(_Rt_) ) {
		PSRADItoR(rdreg, 31);
		return;
	}

	if( !EEINST_ISLIVE1(_Rd_) ) {
		EEINST_RESETHASLIVE1(_Rd_);
		if( sa ) PSRADItoR(rdreg, sa);
		PUNPCKHDQRtoR(rdreg, rdreg);
		return;
	}

    t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
	MOVQRtoR(t0reg, rtreg);

	// it is a signed shift
	if( sa ) {
		PSRADItoR(rdreg, sa);
		PSRADItoR(t0reg, 31);

		// take higher dword of rdreg and lower dword of t0reg
		PUNPCKHDQRtoR(rdreg, t0reg);
		_freeMMXreg(t0reg);
	}
	else {
		// better timing
		PSRADItoR(rdreg, 31);

		// take higher dword of rdreg and lower dword of t0reg
		PUNPCKHDQRtoR(t0reg, rdreg);

		// swap
		mmxregs[t0reg] = mmxregs[rdreg];
		mmxregs[rdreg].inuse = 0;
	}
}

void recDSRA32_(int info)
{
	recDSRA32s_(info, _Sa_);
}

EERECOMPILE_CODEX(eeRecompileCode2, DSRA32);

/*********************************************************
* Shift arithmetic with variant register shift           *
* Format:  OP rd, rt, rs                                 *
*********************************************************/

PCSX2_ALIGNED16(u32 s_sa[4]) = {0x1f, 0, 0x3f, 0};

int recSetShiftV(int info, int* rsreg, int* rtreg, int* rdreg, int* rstemp, int forcemmx, int shift64)
{
	assert( !(info & PROCESS_EE_XMM) );

	if( info & PROCESS_EE_MMX ) {
		*rtreg = EEREC_T;
		*rdreg = EEREC_D;
		*rsreg = EEREC_S;

		// make sure to take only low 5 bits of *rsreg
		if( !(g_pCurInstInfo->regs[_Rs_]&EEINST_LASTUSE) && EEINST_ISLIVE64(_Rs_)) {
			*rstemp = _allocMMXreg(-1, MMX_TEMP, 0);
			MOVQRtoR(*rstemp, *rsreg);
			*rsreg = *rstemp;
		}
		else {
			if( *rsreg != *rdreg ) {
				_freeMMXreg(*rsreg);
				mmxregs[*rsreg].inuse = 0;
			}
		}

		PANDMtoR(*rsreg, (u32)&s_sa[shift64?2:0]);

		if( EEREC_D == EEREC_S ) {
			// need to be separate
			int mmreg = _allocMMXreg(-1, MMX_TEMP, 0);
			*rdreg = mmreg;
			mmxregs[mmreg] = mmxregs[EEREC_S];
			mmxregs[EEREC_S].inuse = 0;
		}
	}
	else if( forcemmx || (g_pCurInstInfo->regs[_Rt_]&EEINST_MMX) || (g_pCurInstInfo->regs[_Rd_]&EEINST_MMX) ) {
		_addNeededMMXreg(MMX_GPR+_Rt_);
		_addNeededMMXreg(MMX_GPR+_Rd_);
		*rtreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_READ);
		*rdreg = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
		SetMMXstate();

		*rstemp = _allocMMXreg(-1, MMX_TEMP, 0);
		MOV32MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UL[0]);
		AND32ItoR(EAX, shift64?0x3f:0x1f);
		MOVD32RtoMMX(*rstemp, EAX);
		*rsreg = *rstemp;
	}
	else {
		return 0;
	}

	if( *rtreg != *rdreg ) MOVQRtoR(*rdreg, *rtreg);
	return 1;
}

void recSetConstShiftV(int info, int* rsreg, int* rdreg, int* rstemp, int shift64)
{
	if( info & PROCESS_EE_MMX ) {
		*rdreg = EEREC_D;
		*rsreg = EEREC_S;

		// make sure to take only low 5 bits of *rsreg
		if( !(g_pCurInstInfo->regs[_Rs_]&EEINST_LASTUSE) && EEINST_ISLIVE64(_Rs_) ) {
			*rstemp = _allocMMXreg(-1, MMX_TEMP, 0);
			MOVQRtoR(*rstemp, *rsreg);
			*rsreg = *rstemp;
		}
		else {
			if( *rsreg != *rdreg ) {
				_freeMMXreg(*rsreg);
				mmxregs[*rsreg].inuse = 0;
			}
		}

		PANDMtoR(*rsreg, (u32)&s_sa[shift64?2:0]);

		
		if( EEREC_D == EEREC_S ) {
			// need to be separate
			int mmreg = _allocMMXreg(-1, MMX_TEMP, 0);
			*rdreg = mmreg;
			mmxregs[mmreg] = mmxregs[EEREC_S];
			mmxregs[EEREC_S].inuse = 0;
		}
	}
	else {
		_addNeededMMXreg(MMX_GPR+_Rd_);
		*rdreg = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
		SetMMXstate();

		*rstemp = _allocMMXreg(-1, MMX_TEMP, 0);
		MOV32MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UL[0]);
		AND32ItoR(EAX, shift64?0x3f:0x1f);
		MOVD32RtoMMX(*rstemp, EAX);
		*rsreg = *rstemp;
	}

	_flushConstReg(_Rt_);
}

void recMoveSignToRd(int info)
{
	if( EEINST_ISLIVE1(_Rd_) ) {
		CDQ();
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
	}
	else {
		EEINST_RESETHASLIVE1(_Rd_);
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	}
	
	if( info & PROCESS_EE_MMX ) {
		mmxregs[EEREC_D].inuse = 0;
	}
}

//// SLLV
void recSLLV_const()
{
	g_cpuConstRegs[_Rd_].SD[0] = (s32)(g_cpuConstRegs[_Rt_].UL[0] << (g_cpuConstRegs[_Rs_].UL[0] &0x1f));
}

void recSLLV_consts(int info)
{
	recSLLs_(info, g_cpuConstRegs[_Rs_].UL[0]&0x1f);
	EEINST_SETSIGNEXT(_Rd_);
}

void recSLLV_constt(int info)
{
	if( (info & PROCESS_EE_MMX) && (info & PROCESS_EE_MODEWRITES) ) MOVD32MMXtoR(ECX, EEREC_S);
	else MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );

	MOV32ItoR( EAX, g_cpuConstRegs[_Rt_].UL[0] );
	AND32ItoR( ECX, 0x1f );
	SHL32CLtoR( EAX );

	recMoveSignToRd(info);
	EEINST_SETSIGNEXT(_Rd_);
}

void recSLLV_(int info)
{
	int rsreg, rtreg, rdreg, rstemp = -1, t0reg;
	EEINST_SETSIGNEXT(_Rd_);

	if( recSetShiftV(info, &rsreg, &rtreg, &rdreg, &rstemp, 0, 0) == 0 ) {
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		if ( _Rs_ != 0 )	
		{
			MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
			AND32ItoR( ECX, 0x1f );
			SHL32CLtoR( EAX );
		}
		CDQ();
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
		return;
	}

	if( !EEINST_ISLIVE1(_Rd_) ) {
		EEINST_RESETHASLIVE1(_Rd_);
		PSLLDRtoR(rdreg, rsreg);
		if( rstemp != -1 ) _freeMMXreg(rstemp);
		return;
	}

    t0reg = _allocMMXreg(-1, MMX_TEMP, 0);	

	// it is a signed shift
	PSLLDRtoR(rdreg, rsreg);
	MOVQRtoR(t0reg, rdreg);
	PSRADItoR(t0reg, 31);

	// take lower dword of rdreg and lower dword of t0reg		
	PUNPCKLDQRtoR(rdreg, t0reg);
	_freeMMXreg(t0reg);
	if( rstemp != -1 ) _freeMMXreg(rstemp);
}

EERECOMPILE_CODE0(SLLV, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

//// SRLV
void recSRLV_const()
{
	g_cpuConstRegs[_Rd_].SD[0] = (s32)(g_cpuConstRegs[_Rt_].UL[0] >> (g_cpuConstRegs[_Rs_].UL[0] &0x1f));
}

void recSRLV_consts(int info)
{ 
	recSRLs_(info, g_cpuConstRegs[_Rs_].UL[0]&0x1f);
	EEINST_SETSIGNEXT(_Rd_);
}

void recSRLV_constt(int info)
{
	if( (info & PROCESS_EE_MMX) && (info&PROCESS_EE_MODEWRITES) ) MOVD32MMXtoR(ECX, EEREC_S);
	else MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );

	MOV32ItoR( EAX, g_cpuConstRegs[_Rt_].UL[0] );
	AND32ItoR( ECX, 0x1f );
	SHR32CLtoR( EAX );

	recMoveSignToRd(info);
	EEINST_SETSIGNEXT(_Rd_);
}

void recSRLV_(int info)
{ 
	int rsreg, rtreg, rdreg, rstemp = -1, t0reg;
	EEINST_SETSIGNEXT(_Rd_);

	if( recSetShiftV(info, &rsreg, &rtreg, &rdreg, &rstemp, 0, 0) == 0 ) {
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		if ( _Rs_ != 0 )	
		{
			MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
			AND32ItoR( ECX, 0x1f );
			SHR32CLtoR( EAX );
		}
		CDQ( );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
		return;
	}

	if( !EEINST_ISLIVE1(_Rd_) ) {
		EEINST_RESETHASLIVE1(_Rd_);
		PSRLDRtoR(rdreg, rsreg);
		if( rstemp != -1 ) _freeMMXreg(rstemp);
		return;
	}

    t0reg = _allocMMXreg(-1, MMX_TEMP, 0);	

	// it is a signed shift
	PSRLDRtoR(rdreg, rsreg);
	MOVQRtoR(t0reg, rdreg);
	PSRADItoR(t0reg, 31);

	// take lower dword of rdreg and lower dword of t0reg		
	PUNPCKLDQRtoR(rdreg, t0reg);
	_freeMMXreg(t0reg);
	if( rstemp != -1 ) _freeMMXreg(rstemp);
}

EERECOMPILE_CODE0(SRLV, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

//// SRAV
void recSRAV_const()
{
	g_cpuConstRegs[_Rd_].SD[0] = (s32)(g_cpuConstRegs[_Rt_].SL[0] >> (g_cpuConstRegs[_Rs_].UL[0] &0x1f));
}

void recSRAV_consts(int info)
{
	recSRAs_(info, g_cpuConstRegs[_Rs_].UL[0]&0x1f);
	EEINST_SETSIGNEXT(_Rd_);
}

void recSRAV_constt(int info)
{
	if( (info & PROCESS_EE_MMX) && (info&PROCESS_EE_MODEWRITES) ) MOVD32MMXtoR(ECX, EEREC_S);
	else MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );

	MOV32ItoR( EAX, g_cpuConstRegs[_Rt_].UL[0] );
	AND32ItoR( ECX, 0x1f );
	SAR32CLtoR( EAX );
	
	recMoveSignToRd(info);
	EEINST_SETSIGNEXT(_Rd_);
}

void recSRAV_(int info)
{
	int rsreg, rtreg, rdreg, rstemp = -1, t0reg;
	EEINST_SETSIGNEXT(_Rd_);

	if( recSetShiftV(info, &rsreg, &rtreg, &rdreg, &rstemp, 0, 0) == 0 ) {
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		if ( _Rs_ != 0 )
		{
			MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
			AND32ItoR( ECX, 0x1f );
			SAR32CLtoR( EAX );
		}
		CDQ( );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
		return;
	}

	if( !EEINST_ISLIVE1(_Rd_) ) {
		EEINST_RESETHASLIVE1(_Rd_);
		PSRADRtoR(rdreg, rsreg);
		if( rstemp != -1 ) _freeMMXreg(rstemp);
		return;
	}

    t0reg = _allocMMXreg(-1, MMX_TEMP, 0);	

	// it is a signed shift
	PSRADRtoR(rdreg, rsreg);
	MOVQRtoR(t0reg, rdreg);
	PSRADItoR(t0reg, 31);

	// take lower dword of rdreg and lower dword of t0reg		
	PUNPCKLDQRtoR(rdreg, t0reg);
	_freeMMXreg(t0reg);
	if( rstemp != -1 ) _freeMMXreg(rstemp);
}

EERECOMPILE_CODE0(SRAV, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

//// DSLLV
void recDSLLV_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = (u64)(g_cpuConstRegs[_Rt_].UD[0] << (g_cpuConstRegs[_Rs_].UL[0] &0x3f));
}

void recDSLLV_consts(int info)
{
	int sa = g_cpuConstRegs[_Rs_].UL[0]&0x3f;
	if( sa < 32 ) recDSLLs_(info, sa);
	else recDSLL32s_(info, sa-32);
}

void recDSLLV_constt(int info)
{
	int rsreg, rdreg, rstemp = -1;
	recSetConstShiftV(info, &rsreg, &rdreg, &rstemp, 1);

	MOVQMtoR(rdreg, (u32)&cpuRegs.GPR.r[_Rt_]);
	PSLLQRtoR(rdreg, rsreg);
	if( rstemp != -1 ) _freeMMXreg(rstemp);
}

void recDSLLV_(int info)
{
	int rsreg, rtreg, rdreg, rstemp = -1;
	recSetShiftV(info, &rsreg, &rtreg, &rdreg, &rstemp, 1, 1);

	PSLLQRtoR(rdreg, rsreg);
	if( rstemp != -1 ) _freeMMXreg(rstemp);
}

EERECOMPILE_CODE0(DSLLV, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

//// DSRLV
void recDSRLV_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = (u64)(g_cpuConstRegs[_Rt_].UD[0] >> (g_cpuConstRegs[_Rs_].UL[0] &0x3f));
}

void recDSRLV_consts(int info)
{
	int sa = g_cpuConstRegs[_Rs_].UL[0]&0x3f;
	if( sa < 32 ) recDSRLs_(info, sa);
	else recDSRL32s_(info, sa-32);
}

void recDSRLV_constt(int info)
{
	int rsreg, rdreg, rstemp = -1;
	recSetConstShiftV(info, &rsreg, &rdreg, &rstemp, 1);

	MOVQMtoR(rdreg, (u32)&cpuRegs.GPR.r[_Rt_]);
	PSRLQRtoR(rdreg, rsreg);
	if( rstemp != -1 ) _freeMMXreg(rstemp);
}

void recDSRLV_(int info)
{
	int rsreg, rtreg, rdreg, rstemp = -1;
	recSetShiftV(info, &rsreg, &rtreg, &rdreg, &rstemp, 1, 1);

	PSRLQRtoR(rdreg, rsreg);
	if( rstemp != -1 ) _freeMMXreg(rstemp);
}

EERECOMPILE_CODE0(DSRLV, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

//// DSRAV
void recDSRAV_const()
{
	g_cpuConstRegs[_Rd_].SD[0] = (s64)(g_cpuConstRegs[_Rt_].SD[0] >> (g_cpuConstRegs[_Rs_].UL[0] &0x3f));
}

void recDSRAV_consts(int info)
{
	int sa = g_cpuConstRegs[_Rs_].UL[0]&0x3f;
	if( sa < 32 ) recDSRAs_(info, sa);
	else recDSRA32s_(info, sa-32);
}

void recDSRAV_constt(int info)
{
	int rsreg, rdreg, rstemp = -1, t0reg, t1reg;
	t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
	t1reg = _allocMMXreg(-1, MMX_TEMP, 0);

	recSetConstShiftV(info, &rsreg, &rdreg, &rstemp, 1);

	MOVQMtoR(rdreg, (u32)&cpuRegs.GPR.r[_Rt_]);
	PXORRtoR(t0reg, t0reg);

	// calc high bit
	MOVQRtoR(t1reg, rdreg);
	PCMPGTDRtoR(t0reg, rdreg);
	PUNPCKHDQRtoR(t0reg, t0reg); // shift to lower

	// shift highest bit, 64 - eax
	MOV32ItoR(EAX, 64);
	MOVD32RtoMMX(t1reg, EAX);
	PSUBDRtoR(t1reg, rsreg);

	// right logical shift 
	PSRLQRtoR(rdreg, rsreg);
	PSLLQRtoR(t0reg, t1reg); // highest bits

	PORRtoR(rdreg, t0reg);
	
	_freeMMXreg(t0reg);
	_freeMMXreg(t1reg);
	if( rstemp != -1 ) _freeMMXreg(rstemp);
}

void recDSRAV_(int info)
{
	int rsreg, rtreg, rdreg, rstemp = -1, t0reg, t1reg;
	t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
	t1reg = _allocMMXreg(-1, MMX_TEMP, 0);
	recSetShiftV(info, &rsreg, &rtreg, &rdreg, &rstemp, 1, 1);

	PXORRtoR(t0reg, t0reg);

	// calc high bit
	MOVQRtoR(t1reg, rdreg);
	PCMPGTDRtoR(t0reg, rdreg);
	PUNPCKHDQRtoR(t0reg, t0reg); // shift to lower

	// shift highest bit, 64 - eax
	MOV32ItoR(EAX, 64);
	MOVD32RtoMMX(t1reg, EAX);
	PSUBDRtoR(t1reg, rsreg);

	// right logical shift 
	PSRLQRtoR(rdreg, rsreg);
	PSLLQRtoR(t0reg, t1reg); // highest bits

	PORRtoR(rdreg, t0reg);
	
	_freeMMXreg(t0reg);
	_freeMMXreg(t1reg);
	if( rstemp != -1 ) _freeMMXreg(rstemp);
}

EERECOMPILE_CODE0(DSRAV, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

#else

////////////////////////////////////////////////////
void recDSRA( void )
{
	if( !_Rd_ ) return; //?

	if ( _Sa_ != 0 ) {
		// it is a signed shift
		MOVQMtoR(MM0, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		MOVQRtoR(MM1, MM0);
		PSRADItoR(MM0, _Sa_);
		PSRLQItoR(MM1, _Sa_);

		PUNPCKHDQRtoR(MM0, MM0); // shift to lower
		// take lower dword of MM1 and lower dword of MM0
		
		PUNPCKLDQRtoR(MM1, MM0);

		MOVQRtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], MM1);
	}
	else {
		MOVQMtoR(MM0, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		MOVQRtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], MM0);
	}

	SetMMXstate();
}

////////////////////////////////////////////////////
void recDSRA32(void)
{
	if( !_Rd_ ) return; //?

	MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	CDQ();

	if ( _Sa_ != 0 )
	{
		SAR32ItoR( EAX, _Sa_ );
	}
	MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX);
	MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX);
}

////////////////////////////////////////////////////
void recSLL( void )
{
	if ( ! _Rd_ )
		return;

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	if ( _Sa_ != 0 )
	{
		SHL32ItoR( EAX, _Sa_ );
	}
	CDQ( );
	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );   
}

////////////////////////////////////////////////////
void recSRL( void ) 
{
	if ( ! _Rd_ )
   {
      return;
   }

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   if ( _Sa_ != 0 )
	   {
		   SHR32ItoR( EAX, _Sa_);
	   }
	   CDQ( );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
  
}

////////////////////////////////////////////////////
void recSRA( void ) 
{
	if ( ! _Rd_ )
   {
      return;
   }

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   if ( _Sa_ != 0 )
	   {
		   SAR32ItoR( EAX, _Sa_);
	   }
	   CDQ();
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
   
}

////////////////////////////////////////////////////
void recDSLL( void )
{
	if ( ! _Rd_ )
   {
      return;
   }
	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	   if ( _Sa_ != 0 )
	   {
		   PSLLQItoR( MM0, _Sa_ );
	   }
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	   SetMMXstate();

}

////////////////////////////////////////////////////
void recDSRL( void ) 
{
	if ( ! _Rd_ )
   {
      return;
   }
	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	   if ( _Sa_ != 0 )
	   {
		   PSRLQItoR( MM0, _Sa_ );
	   }
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	   SetMMXstate();
}

////////////////////////////////////////////////////
void recDSLL32( void ) 
{
   if ( ! _Rd_ )
   {
      return;
   }

	   if ( _Sa_ == 0 )
	   {
		   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], 0 );
		   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EAX );
		   return;
	   }
	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	   PSLLQItoR( MM0, _Sa_ + 32 );
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	   SetMMXstate();

}

////////////////////////////////////////////////////
void recDSRL32( void ) 
{
	if ( ! _Rd_ )
   {
      return;
   }


	   if ( _Sa_ == 0 )
	   {
		   MOV32MtoR( EAX,(int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], 0 );
		   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		   return;
	   }

	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	   PSRLQItoR( MM0, _Sa_ + 32 );
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	   SetMMXstate();
 
}


/*********************************************************
* Shift arithmetic with variant register shift           *
* Format:  OP rd, rt, rs                                 *
*********************************************************/

////////////////////////////////////////////////////


////////////////////////////////////////////////////
void recSLLV( void ) 
{

      if ( ! _Rd_ ) return;

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   if ( _Rs_ != 0 )	
	   {
		   MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
		   AND32ItoR( ECX, 0x1f );
		   SHL32CLtoR( EAX );
	   }
	   CDQ();
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
   
}

////////////////////////////////////////////////////
void recSRLV( void ) 
{
 
      if ( ! _Rd_ ) return;

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   if ( _Rs_ != 0 )	
	   {
		   MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
		   AND32ItoR( ECX, 0x1f );
		   SHR32CLtoR( EAX );
	   }
	   CDQ( );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
   
}

////////////////////////////////////////////////////
void recSRAV( void ) 
{
      if ( ! _Rd_ ) return;
	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   if ( _Rs_ != 0 )
	   {
		   MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
		   AND32ItoR( ECX, 0x1f );
		   SAR32CLtoR( EAX );
	   }
	   CDQ( );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
   
}

////////////////////////////////////////////////////
static u64 _sa = 0;
void recDSLLV( void ) 
{
      if ( ! _Rd_ ) return;

	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	   if ( _Rs_ != 0 )
	   {
		   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ] );
		   AND32ItoR( EAX, 0x3f);
		   MOV32RtoM( (int)&_sa, EAX );
		   PSLLQMtoR( MM0, (int)&_sa );
	   }
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	   SetMMXstate();
  
}

////////////////////////////////////////////////////
void recDSRLV( void ) 
{
      if ( ! _Rd_ ) return;
      MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	   if ( _Rs_ != 0 )
	   {
		   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ] );
		   AND32ItoR( EAX, 0x3f);
		   MOV32RtoM( (int)&_sa, EAX );
		   PSRLQMtoR( MM0, (int)&_sa );
	   }
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	   SetMMXstate();

}
////////////////////////////////////////////////////////////////
void recDSRAV( void ) 
{
	MOVQMtoR(MM0, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );

	if ( _Rs_ != 0 ) {
		PXORRtoR(MM1, MM1);

		// calc high bit
		MOVQRtoR(MM2, MM0);
		PUNPCKHDQRtoR(MM2, MM2); // shift to lower
		PCMPGTDRtoR(MM1, MM2);

		// it is a signed shift
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ] );
		AND32ItoR( EAX, 0x3f);
		MOVD32RtoMMX(MM2, EAX); // amount to shift
		NOT32R(EAX);
		ADD32ItoR(EAX, 65);

		// right logical shift 
		PSRLQRtoR(MM0, MM2);

		// shift highest bit, 64 - eax
		MOVD32RtoMMX(MM2, EAX);
		PSLLQRtoR(MM1, MM2); // highest bits

		PORRtoR(MM0, MM1);
		MOVQRtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], MM0);
	}
	else {
		MOVQRtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], MM0);
	}

	SetMMXstate();
}
#endif

} } }
