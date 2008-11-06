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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Common.h"
#include "InterTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"

// stop compiling if NORECBUILD build (only for Visual Studio)
#if !(defined(_MSC_VER) && defined(PCSX2_NORECBUILD))

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

#elif defined(EE_CONST_PROP)

//// ADDI
void recADDI_const( void ) 
{
   g_cpuConstRegs[_Rt_].UD[0] = g_cpuConstRegs[_Rs_].SL[0] + _Imm_;
}

void recADDI_(int info) 
{
	assert( !(info&PROCESS_EE_XMM) );
	EEINST_SETSIGNEXT(_Rt_);
	EEINST_SETSIGNEXT(_Rs_);

	if( info & PROCESS_EE_MMX ) {
		if( _Imm_ != 0 ) {

			u32* ptempmem = recAllocStackMem(4, 4);
			ptempmem[0] = _Imm_;

			if( EEREC_T != EEREC_S ) MOVQRtoR(EEREC_T, EEREC_S);
			PADDDMtoR(EEREC_T, (u32)ptempmem);
			if( EEINST_ISLIVE1(_Rt_) ) _signExtendGPRtoMMX(EEREC_T, _Rt_, 0);
			else EEINST_RESETHASLIVE1(_Rt_);
		}
		else {
			// just move and sign extend
			if( !EEINST_HASLIVE1(_Rs_) ) {

				if( EEINST_ISLIVE1(_Rt_) )
					_signExtendGPRMMXtoMMX(EEREC_T, _Rt_, EEREC_S, _Rs_);
				else
					EEINST_RESETHASLIVE1(_Rt_);
			}
			else {
				if( EEREC_T != EEREC_S ) MOVQRtoR(EEREC_T, EEREC_S);
			}
		}
		return;
	}

	if( (g_pCurInstInfo->regs[_Rt_]&EEINST_MMX) && (_Rt_ != _Rs_) ) {
		int rtreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_WRITE);
		SetMMXstate();

		if( _Imm_ != 0 ) {
			u32* ptempmem = recAllocStackMem(4, 4);
			ptempmem[0] = _Imm_;

			MOVDMtoMMX(rtreg, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
			PADDDMtoR(rtreg, (u32)ptempmem);

			if( EEINST_ISLIVE1(_Rt_) ) _signExtendGPRtoMMX(rtreg, _Rt_, 0);
			else EEINST_RESETHASLIVE1(_Rt_);
		}
		else {
			// just move and sign extend
			if( !EEINST_HASLIVE1(_Rs_) ) {
				MOVDMtoMMX(rtreg, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
				if( EEINST_ISLIVE1(_Rt_) ) _signExtendGPRtoMMX(rtreg, _Rt_, 0);
				else EEINST_RESETHASLIVE1(_Rt_);
			}
			else {
				MOVQMtoR(rtreg, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
			}
		}
	}
	else {
		if( _Rt_ == _Rs_ ) {
			ADD32ItoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], _Imm_);
			if( EEINST_ISLIVE1(_Rt_) ) _signExtendSFtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ]);
			else EEINST_RESETHASLIVE1(_Rt_);
		}
		else {
			MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
			if ( _Imm_ != 0 )
				ADD32ItoR( EAX, _Imm_ );

			if( EEINST_ISLIVE1(_Rt_) ) {
				CDQ( );
				MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
				MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
			}
			else {
				EEINST_RESETHASLIVE1(_Rt_);
				MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
			}
		}
	}
}

EERECOMPILE_CODEX(eeRecompileCode1, ADDI);

////////////////////////////////////////////////////
void recADDIU( void ) 
{
	recADDI( );
}

////////////////////////////////////////////////////
void recDADDI_const( void ) 
{
	g_cpuConstRegs[_Rt_].UD[0] = g_cpuConstRegs[_Rs_].SD[0] + _Imm_;
}

void recDADDI_(int info) 
{
	assert( !(info&PROCESS_EE_XMM) );

	if( info & PROCESS_EE_MMX ) {
		assert( cpucaps.hasStreamingSIMD2Extensions );

		if( _Imm_ != 0 ) {

			// flush
			u32* ptempmem = recAllocStackMem(8, 8);
			ptempmem[0] = _Imm_;
			ptempmem[1] = _Imm_ >= 0 ? 0 : 0xffffffff;
			if( EEREC_T != EEREC_S ) MOVQRtoR(EEREC_T, EEREC_S);
			PADDQMtoR(EEREC_T, (u32)ptempmem);
		}
		else {
			if( EEREC_T != EEREC_S ) MOVQRtoR(EEREC_T, EEREC_S);
		}
		return;
	}
	
	if( (g_pCurInstInfo->regs[_Rt_]&EEINST_MMX) && cpucaps.hasStreamingSIMD2Extensions ) {
		int rtreg;
		u32* ptempmem = recAllocStackMem(8, 8);
		ptempmem[0] = _Imm_;
		ptempmem[1] = _Imm_ >= 0 ? 0 : 0xffffffff;

		rtreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_WRITE);
		SetMMXstate();
		
		MOVQMtoR(rtreg, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
		PADDQMtoR(rtreg, (u32)ptempmem);
	}
	else {
		if( _Rt_ == _Rs_ ) {
			ADD32ItoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], _Imm_);
			ADC32ItoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], _Imm_<0?0xffffffff:0);
		}
		else {
			MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );

			if( EEINST_ISLIVE1(_Rt_) )
				MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );

			if ( _Imm_ != 0 )
			{
				ADD32ItoR( EAX, _Imm_ );

				if( EEINST_ISLIVE1(_Rt_) ) {
					ADC32ItoR( EDX, _Imm_ < 0?0xffffffff:0);
				}
			}

			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );

			if( EEINST_ISLIVE1(_Rt_) )
				MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
			else
				EEINST_RESETHASLIVE1(_Rt_);
		}
	}
}

EERECOMPILE_CODEX(eeRecompileCode1, DADDI);

//// DADDIU
void recDADDIU( void ) 
{
	recDADDI( );
}

//// SLTIU
void recSLTIU_const()
{
	g_cpuConstRegs[_Rt_].UD[0] = g_cpuConstRegs[_Rs_].UD[0] < (u64)(_Imm_);
}

extern void recSLTmemconstt(int regd, int regs, u32 mem, int sign);
extern u32 s_sltone;

void recSLTIU_(int info)
{
	if( info & PROCESS_EE_MMX ) {
		if( EEINST_ISSIGNEXT(_Rs_) ) {
			u32* ptempmem = recAllocStackMem(8,4);
			ptempmem[0] = ((s32)(_Imm_))^0x80000000;
			ptempmem[1] = 0;
			recSLTmemconstt(EEREC_T, EEREC_S, (u32)ptempmem, 0);
			EEINST_SETSIGNEXT(_Rt_);
			return;
		}

		if( info & PROCESS_EE_MODEWRITES ) {
			MOVQRtoM((u32)&cpuRegs.GPR.r[_Rs_], EEREC_S);
			if( mmxregs[EEREC_S].reg == MMX_GPR+_Rs_ ) mmxregs[EEREC_S].mode &= ~MODE_WRITE;
		}
		mmxregs[EEREC_T].mode |= MODE_WRITE; // in case EEREC_T==EEREC_S
	}

	if( info & PROCESS_EE_MMX ) MOVDMtoMMX(EEREC_T, (u32)&s_sltone);
	else MOV32ItoR(EAX, 1);

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], _Imm_ >= 0 ? 0 : 0xffffffff);
	j8Ptr[0] = JB8( 0 );
	j8Ptr[2] = JA8( 0 );

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], (s32)_Imm_ );
	j8Ptr[1] = JB8(0);
	
	x86SetJ8(j8Ptr[2]);
	if( info & PROCESS_EE_MMX ) PXORRtoR(EEREC_T, EEREC_T);
	else XOR32RtoR(EAX, EAX);
	
	x86SetJ8(j8Ptr[0]);
	x86SetJ8(j8Ptr[1]);

	if( !(info & PROCESS_EE_MMX) ) {
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		if( EEINST_ISLIVE1(_Rt_) ) MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
		else EEINST_RESETHASLIVE1(_Rt_);
	}
	EEINST_SETSIGNEXT(_Rt_);
}

EERECOMPILE_CODEX(eeRecompileCode1, SLTIU);

//// SLTI
void recSLTI_const()
{
	g_cpuConstRegs[_Rt_].UD[0] = g_cpuConstRegs[_Rs_].SD[0] < (s64)(_Imm_);
}

void recSLTI_(int info)
{
	if( info & PROCESS_EE_MMX) {
		
		if( EEINST_ISSIGNEXT(_Rs_) ) {
			u32* ptempmem = recAllocStackMem(8,4);
			ptempmem[0] = _Imm_;
			ptempmem[1] = 0;
			recSLTmemconstt(EEREC_T, EEREC_S, (u32)ptempmem, 1);
			EEINST_SETSIGNEXT(_Rt_);
			return;
		}

		if( info & PROCESS_EE_MODEWRITES ) {
			MOVQRtoM((u32)&cpuRegs.GPR.r[_Rs_], EEREC_S);
			if( mmxregs[EEREC_S].reg == MMX_GPR+_Rs_ ) mmxregs[EEREC_S].mode &= ~MODE_WRITE;
		}
		mmxregs[EEREC_T].mode |= MODE_WRITE; // in case EEREC_T==EEREC_S
	}

	// test silent hill if modding
	if( info & PROCESS_EE_MMX ) MOVDMtoMMX(EEREC_T, (u32)&s_sltone);
	else MOV32ItoR(EAX, 1);

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], _Imm_ >= 0 ? 0 : 0xffffffff);
	j8Ptr[0] = JL8( 0 );
	j8Ptr[2] = JG8( 0 );

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], (s32)_Imm_ );
	j8Ptr[1] = JB8(0);
	
	x86SetJ8(j8Ptr[2]);
	if( info & PROCESS_EE_MMX ) PXORRtoR(EEREC_T, EEREC_T);
	else XOR32RtoR(EAX, EAX);
	
	x86SetJ8(j8Ptr[0]);
	x86SetJ8(j8Ptr[1]);
	
	if( !(info & PROCESS_EE_MMX) ) {
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		if( EEINST_ISLIVE1(_Rt_) ) MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
		else EEINST_RESETHASLIVE1(_Rt_);
	}
	EEINST_SETSIGNEXT(_Rt_);
}

EERECOMPILE_CODEX(eeRecompileCode1, SLTI);

//// ANDI
void recANDI_const()
{
	g_cpuConstRegs[_Rt_].UD[0] = g_cpuConstRegs[_Rs_].UD[0] & (s64)_ImmU_;
}

extern void LogicalOpRtoR(x86MMXRegType to, x86MMXRegType from, int op);
extern void LogicalOpMtoR(x86MMXRegType to, u32 from, int op);
extern void LogicalOp32RtoM(u32 to, x86IntRegType from, int op);
extern void LogicalOp32MtoR(x86IntRegType to, u32 from, int op);
extern void LogicalOp32ItoR(x86IntRegType to, u32 from, int op);
extern void LogicalOp32ItoM(u32 to, u32 from, int op);

void recLogicalOpI(int info, int op)
{
	if( info & PROCESS_EE_MMX ) {
		SetMMXstate();

		if( _ImmU_ != 0 ) {
			u32* ptempmem = recAllocStackMem(8, 8);
			ptempmem[0] = _ImmU_;
			ptempmem[1] = 0;

			if( EEREC_T != EEREC_S ) MOVQRtoR(EEREC_T, EEREC_S);
			LogicalOpMtoR(EEREC_T, (u32)ptempmem, op);
		}
		else {
			if( op == 0 ) PXORRtoR(EEREC_T, EEREC_T);
			else {
				if( EEREC_T != EEREC_S ) MOVQRtoR(EEREC_T, EEREC_S);
			}
		}
		return;
	}

	if( (g_pCurInstInfo->regs[_Rt_]&EEINST_MMX) && ((_Rt_ != _Rs_) || (_ImmU_==0)) ) {
		int rtreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_WRITE);
		u32* ptempmem;

		SetMMXstate();

		ptempmem = recAllocStackMem(8, 8);
		ptempmem[0] = _ImmU_;
		ptempmem[1] = 0;

		if( op == 0 ) {
			if ( _ImmU_ != 0 ) {
				if( _ImmU_ == 0xffff ) {
					// take a shortcut
					MOVDMtoMMX(rtreg, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] - 2);
					PSRLDItoR(rtreg, 16);
				}
				else {
					MOVDMtoMMX(rtreg, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
					PANDMtoR(rtreg, (u32)ptempmem);
				}
			}
			else PXORRtoR(rtreg, rtreg);
		}
		else {
			MOVQMtoR(rtreg, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
			if ( _ImmU_ != 0 ) LogicalOpMtoR(rtreg, (u32)ptempmem, op);
		}
	}
	else {
		if ( _ImmU_ != 0 )
		{
			if( _Rt_ == _Rs_ ) {
				LogicalOp32ItoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], _ImmU_, op);
			}
			else {
				MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
				if( op != 0 && EEINST_ISLIVE1(_Rt_) )
					MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
				LogicalOp32ItoR( EAX, _ImmU_, op);
				if( op != 0 && EEINST_ISLIVE1(_Rt_) )
					MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
				MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
			}

			if( op == 0 ) {
				if( EEINST_ISLIVE1(_Rt_ ) ) MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
				else EEINST_RESETHASLIVE1(_Rt_);
			}
		}
		else
		{
			if( op == 0 ) {
				MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], 0 );
				if( EEINST_ISLIVE1(_Rt_ ) )
					MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
			}
			else {
				if( _Rt_ != _Rs_ ) {
					MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
					if( EEINST_ISLIVE1(_Rt_ ) ) 
						MOV32MtoR(EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
					MOV32RtoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
					if( EEINST_ISLIVE1(_Rt_ ) ) 
						MOV32RtoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
				}
			}

			if( !EEINST_ISLIVE1(_Rt_) ) EEINST_RESETHASLIVE1(_Rt_);
		}
	}
}

void recANDI_(int info)
{
	recLogicalOpI(info, 0);
}

EERECOMPILE_CODEX(eeRecompileCode1, ANDI);

////////////////////////////////////////////////////
void recORI_const()
{
	g_cpuConstRegs[_Rt_].UD[0] = g_cpuConstRegs[_Rs_].UD[0] | (s64)_ImmU_;
}

void recORI_(int info)
{
	recLogicalOpI(info, 1);
}

EERECOMPILE_CODEX(eeRecompileCode1, ORI);

////////////////////////////////////////////////////
void recXORI_const()
{
	g_cpuConstRegs[_Rt_].UD[0] = g_cpuConstRegs[_Rs_].UD[0] ^ (s64)_ImmU_;
}

void recXORI_(int info)
{
	recLogicalOpI(info, 2);
}

EERECOMPILE_CODEX(eeRecompileCode1, XORI);

#else

////////////////////////////////////////////////////
void recADDI( void ) 
{
   if ( ! _Rt_ )
   {
      return;
   }

      MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );

	   if ( _Imm_ != 0 )
	   {
		   ADD32ItoR( EAX, _Imm_ );
	   }

	   CDQ( );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
  
}

////////////////////////////////////////////////////
void recADDIU( void ) 
{
	recADDI( );
}

////////////////////////////////////////////////////
void recDADDI( void ) 
{
#ifdef __x86_64_
   if ( ! _Rt_ )
   {
      return;
   }

	MOV64MtoR( RAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	   {
		   ADD64ItoR( EAX, _Imm_ );
	   }
	MOV64RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], RAX );
#else
   if ( ! _Rt_ )
   {
      return;
   }

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	   if ( _Imm_ != 0 )
	   {
		   ADD32ItoR( EAX, _Imm_ );
		   if ( _Imm_ < 0 )
         {
			   ADC32ItoR( EDX, 0xffffffff );
         }
		   else
         {
			   ADC32ItoR( EDX, 0 );
         }
	   }
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
#endif  
}

////////////////////////////////////////////////////
void recDADDIU( void ) 
{
	recDADDI( );
}

////////////////////////////////////////////////////
void recSLTIU( void )
{
	if ( ! _Rt_ )
		return;

	MOV32ItoR(EAX, 1);
	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], _Imm_ >= 0 ? 0 : 0xffffffff);
	j8Ptr[0] = JB8( 0 );
	j8Ptr[2] = JA8( 0 );

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], (s32)_Imm_ );
	j8Ptr[1] = JB8(0);
	
	x86SetJ8(j8Ptr[2]);
	XOR32RtoR(EAX, EAX);
	
	x86SetJ8(j8Ptr[0]);
	x86SetJ8(j8Ptr[1]);
	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
}

////////////////////////////////////////////////////
void recSLTI( void )
{
	if ( ! _Rt_ )
		return;

	// test silent hill if modding
	MOV32ItoR(EAX, 1);
	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], _Imm_ >= 0 ? 0 : 0xffffffff);
	j8Ptr[0] = JL8( 0 );
	j8Ptr[2] = JG8( 0 );

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], (s32)_Imm_ );
	j8Ptr[1] = JB8(0);
	
	x86SetJ8(j8Ptr[2]);
	XOR32RtoR(EAX, EAX);
	
	x86SetJ8(j8Ptr[0]);
	x86SetJ8(j8Ptr[1]);
	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
}

////////////////////////////////////////////////////
void recANDI( void )
{
	if ( ! _Rt_ )
   {
      return;
   }
	   if ( _ImmU_ != 0 )
	   {
		   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
		   AND32ItoR( EAX, _ImmU_ );
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
		   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	   }
	   else
	   {
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], 0 );
	   }
   
}

////////////////////////////////////////////////////
static u64 _imm = 0; // temp immediate

void recORI( void )
{
	if ( ! _Rt_ )
		return;

	MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	if ( _ImmU_ != 0 )
	{
		MOV32ItoM( (int)&_imm, _ImmU_ );
		MOVQMtoR( MM1, (int)&_imm );
		PORRtoR( MM0, MM1 );
	}
	MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ], MM0 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recXORI( void )
{
	if ( ! _Rt_ )
		return;

	MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ] );
	if ( _ImmU_ != 0 )
	{
		MOV32ItoM( (int)&_imm, _ImmU_ );
		MOVQMtoR( MM1, (int)&_imm );
		PXORRtoR( MM0, MM1 );
	}
	MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rt_ ], MM0 );
	SetMMXstate();
}

#endif

#endif // PCSX2_NORECBUILD
