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

/*********************************************************
*   cached MMI opcodes                                   *
*                                                        *
*********************************************************/
// stop compiling if NORECBUILD build (only for Visual Studio)
#if !(defined(_MSC_VER) && defined(PCSX2_NORECBUILD))

#include "Common.h"
#include "InterTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"
#include "iMMI.h"

#ifndef MMI_RECOMPILE

REC_FUNC( PLZCW, _Rd_ );

#ifndef MMI0_RECOMPILE

REC_FUNC( MMI0, _Rd_ );

#endif

#ifndef MMI1_RECOMPILE

REC_FUNC( MMI1, _Rd_ );

#endif

#ifndef MMI2_RECOMPILE

REC_FUNC( MMI2, _Rd_ );

#endif

#ifndef MMI3_RECOMPILE

REC_FUNC( MMI3, _Rd_ );

#endif

REC_FUNC( PMFHL, _Rd_ );
REC_FUNC( PMTHL, _Rd_ );

REC_FUNC( PSRLW, _Rd_ );
REC_FUNC( PSRLH, _Rd_ );

REC_FUNC( PSRAH, _Rd_ );
REC_FUNC( PSRAW, _Rd_ );

REC_FUNC( PSLLH, _Rd_ );
REC_FUNC( PSLLW, _Rd_ );

#else

void recPLZCW()
{
	int regd = -1;
	int regs = 0;

	if ( ! _Rd_ ) return;

	if( GPR_IS_CONST1(_Rs_) ) {
		_eeOnWriteReg(_Rd_, 0);
		_deleteEEreg(_Rd_, 0);
		GPR_SET_CONST(_Rd_);

		for(regs = 0; regs < 2; ++regs) {
			u32 val = g_cpuConstRegs[_Rs_].UL[regs];

			if( val != 0 ) {
				u32 setbit = val&0x80000000;
				g_cpuConstRegs[_Rd_].UL[regs] = 0;
				val <<= 1;

				while((val & 0x80000000) == setbit) {
					g_cpuConstRegs[_Rd_].UL[regs]++;
					val <<= 1;
				}
			}
			else {
				g_cpuConstRegs[_Rd_].UL[regs] = 31;
			}
		}
		return;
	}

	_eeOnWriteReg(_Rd_, 0);

	if( (regs = _checkXMMreg(XMMTYPE_GPRREG, _Rs_, MODE_READ)) >= 0 ) {
		SSE2_MOVD_XMM_to_R(EAX, regs);
		regs |= MEM_XMMTAG;
	}
#ifndef __x86_64__
	else if( (regs = _checkMMXreg(MMX_GPR+_Rs_, MODE_READ)) >= 0 ) {
		MOVD32MMXtoR(EAX, regs);
		SetMMXstate();
		regs |= MEM_MMXTAG;
	}
#endif
	else {
		MOV32MtoR(EAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
		regs = 0;
	}

	if( EEINST_ISLIVE1(_Rd_) )
		_deleteEEreg(_Rd_, 0);
#ifndef __x86_64__
	else {
		if( (regd = _checkMMXreg(MMX_GPR+_Rd_, MODE_WRITE)) < 0 ) {
			_deleteEEreg(_Rd_, 0);
		}
	}
#endif

	// first word
	TEST32RtoR(EAX, EAX);
	j8Ptr[0] = JNZ8(0);

	// zero, so put 31
	if( EEINST_ISLIVE1(_Rd_) || regd < 0 ) {
		MOV32ItoM((uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], 31);
	}
#ifndef __x86_64__
	else {
		SetMMXstate();
		PCMPEQDRtoR(regd, regd);
		PSRLQItoR(regd, 59);
	}
#endif

	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	TEST32ItoR(EAX, 0x80000000);
	j8Ptr[0] = JZ8(0);
	NOT32R(EAX);
	x86SetJ8(j8Ptr[0]);

	// not zero
	x86SetJ8(j8Ptr[0]);
	BSRRtoR(EAX, EAX);
	MOV32ItoR(ECX, 30);
	SUB32RtoR(ECX, EAX);
	if( EEINST_ISLIVE1(_Rd_) || regd < 0 ) {
		MOV32RtoM((uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], ECX);
	}
#ifndef __x86_64__
	else {
		SetMMXstate();
		MOVD32RtoMMX(regd, ECX);
	}
#endif

	x86SetJ8(j8Ptr[1]);

	// second word
	if( EEINST_ISLIVE1(_Rd_) ) {
		if( regs >= 0 && (regs & MEM_XMMTAG) ) {
			SSE2_PSHUFD_XMM_to_XMM(regs&0xf, regs&0xf, 0x4e);
			SSE2_MOVD_XMM_to_R(EAX, regs&0xf);
			SSE2_PSHUFD_XMM_to_XMM(regs&0xf, regs&0xf, 0x4e);
		}
#ifndef __x86_64__
		else if( regs >= 0 && (regs & MEM_MMXTAG) ) {
			PSHUFWRtoR(regs, regs, 0x4e);
			MOVD32MMXtoR(EAX, regs&0xf);
			PSHUFWRtoR(regs&0xf, regs&0xf, 0x4e);
			SetMMXstate();
		}
#endif
		else MOV32MtoR(EAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ]);

		TEST32RtoR(EAX, EAX);
		j8Ptr[0] = JNZ8(0);

		// zero, so put 31
		MOV32ItoM((uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], 31);
		j8Ptr[1] = JMP8(0);
		x86SetJ8(j8Ptr[0]);

		TEST32ItoR(EAX, 0x80000000);
		j8Ptr[0] = JZ8(0);
		NOT32R(EAX);
		x86SetJ8(j8Ptr[0]);

		// not zero
		x86SetJ8(j8Ptr[0]);
		BSRRtoR(EAX, EAX);
		MOV32ItoR(ECX, 30);
		SUB32RtoR(ECX, EAX);
		MOV32RtoM((uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], ECX);
		x86SetJ8(j8Ptr[1]);
	}
	else {
		EEINST_RESETHASLIVE1(_Rd_);
	}

	GPR_DEL_CONST(_Rd_);
}

static u32 PCSX2_ALIGNED16(s_CmpMasks[]) = { 0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff };

void recPMFHL()
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_WRITED|XMMINFO_READLO|XMMINFO_READHI)

	int t0reg;

	switch (_Sa_) {
		case 0x00: // LW

			t0reg = _allocTempXMMreg(XMMT_INT, -1);
			SSE2_PSHUFD_XMM_to_XMM(t0reg, EEREC_HI, 0x88);
			SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_LO, 0x88);			
			SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t0reg);
			
			_freeXMMreg(t0reg);
			break;

		case 0x01: // UW
			t0reg = _allocTempXMMreg(XMMT_INT, -1);
			SSE2_PSHUFD_XMM_to_XMM(t0reg, EEREC_HI, 0xdd);
			SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_LO, 0xdd);
			SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t0reg);
			_freeXMMreg(t0reg);
			break;

		case 0x02: // SLW
			// fall to interp
			MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
			MOV32ItoM( (uptr)&cpuRegs.pc, pc );
			_flushCachedRegs();
			_deleteEEreg(_Rd_, 0);
			_deleteEEreg(XMMGPR_LO, 1);
			_deleteEEreg(XMMGPR_HI, 1);
			iFlushCall(FLUSH_CACHED_REGS); // since calling CALLFunc
			CALLFunc( (u32)PMFHL );
			break;

		case 0x03: // LH
			t0reg = _allocTempXMMreg(XMMT_INT, -1);
			SSE2_PSHUFLW_XMM_to_XMM(t0reg, EEREC_HI, 0x88);
			SSE2_PSHUFLW_XMM_to_XMM(EEREC_D, EEREC_LO, 0x88);
			SSE2_PSHUFHW_XMM_to_XMM(t0reg, t0reg, 0x88);
			SSE2_PSHUFHW_XMM_to_XMM(EEREC_D, EEREC_D, 0x88);
			SSE2_PSRLDQ_I8_to_XMM(t0reg, 4);
			SSE2_PSRLDQ_I8_to_XMM(EEREC_D, 4);
			SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t0reg);
			_freeXMMreg(t0reg);
			break;

		case 0x04: // SH
			if( EEREC_D == EEREC_HI ) {
				SSE2_PACKSSDW_XMM_to_XMM(EEREC_D, EEREC_LO);
				SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_D, 0x72);
			}
			else {
				if( EEREC_D != EEREC_LO ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_LO);
				SSE2_PACKSSDW_XMM_to_XMM(EEREC_D, EEREC_HI);
				
				// shuffle so a1a0b1b0->a1b1a0b0
				SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_D, 0xd8);
			}
			break;
		default:
			SysPrintf("PMFHL??\n");
			assert(0);
	}

CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE( PMFHL, _Rd_ );
}

void recPMTHL()
{
	REC_FUNC_INLINE( PMTHL, 0 );
}

#ifndef MMI0_RECOMPILE

REC_FUNC( MMI0, _Rd_ );

#endif

#ifndef MMI1_RECOMPILE

REC_FUNC( MMI1, _Rd_ );

#endif

#ifndef MMI2_RECOMPILE

REC_FUNC( MMI2, _Rd_ );

#endif

#ifndef MMI3_RECOMPILE

REC_FUNC( MMI3, _Rd_ );

#endif

#ifdef __x86_64__

#define MMX_ALLOC_TEMP1(code)
#define MMX_ALLOC_TEMP2(code)
#define MMX_ALLOC_TEMP3(code)
#define MMX_ALLOC_TEMP4(code)

#else

// MMX helper routines
#define MMX_ALLOC_TEMP1(code) { \
		int t0reg; \
		t0reg = _allocMMXreg(-1, MMX_TEMP, 0); \
		code; \
		_freeMMXreg(t0reg); \
} \

#define MMX_ALLOC_TEMP2(code) { \
		int t0reg, t1reg; \
		t0reg = _allocMMXreg(-1, MMX_TEMP, 0); \
		t1reg = _allocMMXreg(-1, MMX_TEMP, 0); \
		code; \
		_freeMMXreg(t0reg); \
		_freeMMXreg(t1reg); \
} \

#define MMX_ALLOC_TEMP3(code) { \
		int t0reg, t1reg, t2reg; \
		t0reg = _allocMMXreg(-1, MMX_TEMP, 0); \
		t1reg = _allocMMXreg(-1, MMX_TEMP, 0); \
		t2reg = _allocMMXreg(-1, MMX_TEMP, 0); \
		code; \
		_freeMMXreg(t0reg); \
		_freeMMXreg(t1reg); \
		_freeMMXreg(t2reg); \
} \

#define MMX_ALLOC_TEMP4(code) { \
		int t0reg, t1reg, t2reg, t3reg; \
		t0reg = _allocMMXreg(-1, MMX_TEMP, 0); \
		t1reg = _allocMMXreg(-1, MMX_TEMP, 0); \
		t2reg = _allocMMXreg(-1, MMX_TEMP, 0); \
		t3reg = _allocMMXreg(-1, MMX_TEMP, 0); \
		code; \
		_freeMMXreg(t0reg); \
		_freeMMXreg(t1reg); \
		_freeMMXreg(t2reg); \
		_freeMMXreg(t3reg); \
} \

#endif // __x86_64__

////////////////////////////////////////////////////
void recPSRLH( void )
{
	if ( !_Rd_ ) return;
	
	CPU_SSE2_XMMCACHE_START(XMMINFO_READT|XMMINFO_WRITED)
		if( (_Sa_&0xf) == 0 ) {
			if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			return;
		}

		if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSRLW_I8_to_XMM(EEREC_D,_Sa_&0xf );
	CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP2(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PSRLWItoR( t0reg, _Sa_&0xf );
		PSRLWItoR( t1reg, _Sa_&0xf );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPSRLW( void )
{
	if( !_Rd_ ) return;

	CPU_SSE2_XMMCACHE_START(XMMINFO_READT|XMMINFO_WRITED)
		if( _Sa_ == 0 ) {
			if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			return;
		}

		if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSRLD_I8_to_XMM(EEREC_D,_Sa_ );
	CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP2(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PSRLDItoR( t0reg, _Sa_ );
		PSRLDItoR( t1reg, _Sa_ );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPSRAH( void )
{
	if ( !_Rd_ ) return;

	CPU_SSE2_XMMCACHE_START(XMMINFO_READT|XMMINFO_WRITED)
		if( (_Sa_&0xf) == 0 ) {
			if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			return;
		}

		if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSRAW_I8_to_XMM(EEREC_D,_Sa_&0xf );
	CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP2(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PSRAWItoR( t0reg, _Sa_&0xf );
		PSRAWItoR( t1reg, _Sa_&0xf );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPSRAW( void )
{
	if ( !_Rd_ ) return;

	CPU_SSE2_XMMCACHE_START(XMMINFO_READT|XMMINFO_WRITED)
		if( _Sa_ == 0 ) {
			if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			return;
		}

		if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSRAD_I8_to_XMM(EEREC_D,_Sa_ );
	CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP2(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PSRADItoR( t0reg, _Sa_ );
		PSRADItoR( t1reg, _Sa_ );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPSLLH( void )
{
	if ( !_Rd_ ) return;
	
	CPU_SSE2_XMMCACHE_START(XMMINFO_READT|XMMINFO_WRITED)
		if( (_Sa_&0xf) == 0 ) {
			if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			return;
		}

		if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSLLW_I8_to_XMM(EEREC_D,_Sa_&0xf );
	CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP2(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PSLLWItoR( t0reg, _Sa_&0xf );
		PSLLWItoR( t1reg, _Sa_&0xf );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPSLLW( void )
{
	if ( !_Rd_ ) return;

	CPU_SSE2_XMMCACHE_START(XMMINFO_READT|XMMINFO_WRITED)
		if( _Sa_ == 0 ) {
			if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			return;
		}

		if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSLLD_I8_to_XMM(EEREC_D,_Sa_ );
	CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP2(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PSLLDItoR( t0reg, _Sa_ );
		PSLLDItoR( t1reg, _Sa_ );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

/*
void recMADD( void ) 
{
}

void recMADDU( void ) 
{
}

void recPLZCW( void ) 
{
}
*/  

#ifdef MMI0_RECOMPILE

void recMMI0( void )
{
	recMMI0t[ _Sa_ ]( );
}

#endif

#ifdef MMI1_RECOMPILE

void recMMI1( void )
{
	recMMI1t[ _Sa_ ]( );
}

#endif

#ifdef MMI2_RECOMPILE

void recMMI2( void )
{
	recMMI2t[ _Sa_ ]( );
}

#endif

#ifdef MMI3_RECOMPILE

void recMMI3( void )
{
	recMMI3t[ _Sa_ ]( );
}

#endif

#endif

/*********************************************************
*   MMI0 opcodes                                         *
*                                                        *
*********************************************************/
#ifndef MMI0_RECOMPILE

REC_FUNC( PADDB, _Rd_);
REC_FUNC( PADDH, _Rd_);
REC_FUNC( PADDW, _Rd_);
REC_FUNC( PADDSB, _Rd_);
REC_FUNC( PADDSH, _Rd_);
REC_FUNC( PADDSW, _Rd_);
REC_FUNC( PSUBB, _Rd_);
REC_FUNC( PSUBH, _Rd_);
REC_FUNC( PSUBW, _Rd_);
REC_FUNC( PSUBSB, _Rd_);
REC_FUNC( PSUBSH, _Rd_);
REC_FUNC( PSUBSW, _Rd_);

REC_FUNC( PMAXW, _Rd_);
REC_FUNC( PMAXH, _Rd_);        

REC_FUNC( PCGTW, _Rd_);
REC_FUNC( PCGTH, _Rd_);
REC_FUNC( PCGTB, _Rd_);

REC_FUNC( PEXTLW, _Rd_);

REC_FUNC( PPACW, _Rd_);        
REC_FUNC( PEXTLH, _Rd_);
REC_FUNC( PPACH, _Rd_);        
REC_FUNC( PEXTLB, _Rd_);
REC_FUNC( PPACB, _Rd_);
REC_FUNC( PEXT5, _Rd_);
REC_FUNC( PPAC5, _Rd_);

#else

////////////////////////////////////////////////////
void recPMAXW()
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	int t0reg;

	if( EEREC_S == EEREC_T ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		return;
	}

	t0reg = _allocTempXMMreg(XMMT_INT, -1);
	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
	SSE2_PCMPGTD_XMM_to_XMM(t0reg, EEREC_T);

	if( EEREC_D == EEREC_S ) {
		SSEX_PAND_XMM_to_XMM(EEREC_D, t0reg);
		SSEX_PANDN_XMM_to_XMM(t0reg, EEREC_T);
	}
	else if( EEREC_D == EEREC_T ) {
		int t1reg = _allocTempXMMreg(XMMT_INT, -1);
		SSEX_MOVDQA_XMM_to_XMM(t1reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSEX_PAND_XMM_to_XMM(EEREC_D, t0reg);
		SSEX_PANDN_XMM_to_XMM(t0reg, t1reg);
		_freeXMMreg(t1reg);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSEX_PAND_XMM_to_XMM(EEREC_D, t0reg);
		SSEX_PANDN_XMM_to_XMM(t0reg, EEREC_T);
	}
	
	SSEX_POR_XMM_to_XMM(EEREC_D, t0reg);
	_freeXMMreg(t0reg);
CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE( PMAXW, _Rd_ );
}

////////////////////////////////////////////////////
void recPPACW()
{
	if ( ! _Rd_ ) return;

CPU_SSE_XMMCACHE_START(((_Rs_!=0||!cpucaps.hasStreamingSIMD2Extensions)?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED)
	if( cpucaps.hasStreamingSIMD2Extensions ) {
		if( _Rs_ == 0 ) {
			SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_T, 0x88);
			SSE2_PSRLDQ_I8_to_XMM(EEREC_D, 8);
		}
		else {
			int t0reg = _allocTempXMMreg(XMMT_INT, -1);
			if( EEREC_D == EEREC_T ) {
				SSE2_PSHUFD_XMM_to_XMM(t0reg, EEREC_S, 0x88);
				SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_T, 0x88);
				SSE2_PUNPCKLQDQ_XMM_to_XMM(EEREC_D, t0reg);
				_freeXMMreg(t0reg);
			}
			else {
				SSE2_PSHUFD_XMM_to_XMM(t0reg, EEREC_T, 0x88);
				SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_S, 0x88);
				SSE2_PUNPCKLQDQ_XMM_to_XMM(t0reg, EEREC_D);

				// swap mmx regs.. don't ask
				xmmregs[t0reg] = xmmregs[EEREC_D];
				xmmregs[EEREC_D].inuse = 0;
			}
		}
	}
	else {
		if( EEREC_D != EEREC_S ) {
			if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE_SHUFPS_XMM_to_XMM(EEREC_D, EEREC_S, 0x88 );
		}
		else {
			SSE_SHUFPS_XMM_to_XMM(EEREC_D, EEREC_T, 0x88 );
			SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_D, 0x4e);
		}
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	//Done - Refraction - Crude but quicker than int
	MOV32MtoR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[2]); //Copy this one cos it could get overwritten

	MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UL[2]);
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[3], EAX);
	MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UL[0]);
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[2], EAX);
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[1], ECX); //This is where we bring it back
	MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[0], EAX);
}

void recPPACH( void )
{
	if (!_Rd_) return;

CPU_SSE2_XMMCACHE_START((_Rs_!=0?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED)
	if( _Rs_ == 0 ) {
		SSE2_PSHUFLW_XMM_to_XMM(EEREC_D, EEREC_T, 0x88);
		SSE2_PSHUFHW_XMM_to_XMM(EEREC_D, EEREC_D, 0x88);
		SSE2_PSLLDQ_I8_to_XMM(EEREC_D, 4);
		SSE2_PSRLDQ_I8_to_XMM(EEREC_D, 8);
	}
	else {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		SSE2_PSHUFLW_XMM_to_XMM(t0reg, EEREC_S, 0x88);
		SSE2_PSHUFLW_XMM_to_XMM(EEREC_D, EEREC_T, 0x88);
		SSE2_PSHUFHW_XMM_to_XMM(t0reg, t0reg, 0x88);
		SSE2_PSHUFHW_XMM_to_XMM(EEREC_D, EEREC_D, 0x88);

		if( cpucaps.hasStreamingSIMD2Extensions ) {
			SSE2_PSRLDQ_I8_to_XMM(t0reg, 4);
			SSE2_PSRLDQ_I8_to_XMM(EEREC_D, 4);
			SSE2_PUNPCKLQDQ_XMM_to_XMM(EEREC_D, t0reg);
		}
		else {
			SSE_SHUFPS_XMM_to_XMM(EEREC_D, t0reg, 0x88);
		}
		_freeXMMreg(t0reg);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	//Done - Refraction - Crude but quicker than int
	MOV16MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].US[6]);
	MOV16RtoM((uptr)&cpuRegs.GPR.r[_Rd_].US[7], EAX);
	MOV16MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].US[6]);
	MOV16RtoM((uptr)&cpuRegs.GPR.r[_Rd_].US[3], EAX);
	MOV16MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].US[2]);
	MOV16RtoM((uptr)&cpuRegs.GPR.r[_Rd_].US[5], EAX);
	MOV16MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].US[2]);
	MOV16RtoM((uptr)&cpuRegs.GPR.r[_Rd_].US[1], EAX);
	MOV16MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].US[4]);
	MOV16RtoM((uptr)&cpuRegs.GPR.r[_Rd_].US[6], EAX);
	MOV16MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].US[4]);
	MOV16RtoM((uptr)&cpuRegs.GPR.r[_Rd_].US[2], EAX);
	MOV16MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].US[0]);
	MOV16RtoM((uptr)&cpuRegs.GPR.r[_Rd_].US[4], EAX);
	MOV16MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].US[0]);
	MOV16RtoM((uptr)&cpuRegs.GPR.r[_Rd_].US[0], EAX);	
}

////////////////////////////////////////////////////
void recPPACB()
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START((_Rs_!=0?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED)
	if( _Rs_ == 0 ) {
		if( _hasFreeXMMreg() ) {
			int t0reg = _allocTempXMMreg(XMMT_INT, -1);
			if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PSLLW_I8_to_XMM(EEREC_D, 8);
			SSEX_PXOR_XMM_to_XMM(t0reg, t0reg);
			SSE2_PSRLW_I8_to_XMM(EEREC_D, 8);
			SSE2_PACKUSWB_XMM_to_XMM(EEREC_D, t0reg);
			_freeXMMreg(t0reg);
		}
		else {
			if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PSLLW_I8_to_XMM(EEREC_D, 8);
			SSE2_PSRLW_I8_to_XMM(EEREC_D, 8);
			SSE2_PACKUSWB_XMM_to_XMM(EEREC_D, EEREC_D);
			SSE2_PSRLDQ_I8_to_XMM(EEREC_D, 8);
		}
	}
	else {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);

		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
		if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSLLW_I8_to_XMM(t0reg, 8);
		SSE2_PSLLW_I8_to_XMM(EEREC_D, 8);
		SSE2_PSRLW_I8_to_XMM(t0reg, 8);
		SSE2_PSRLW_I8_to_XMM(EEREC_D, 8);

		SSE2_PACKUSWB_XMM_to_XMM(EEREC_D, t0reg);
		_freeXMMreg(t0reg);
	}
CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE( PPACB, _Rd_ );
}

////////////////////////////////////////////////////
void recPEXT5()
{
	REC_FUNC_INLINE( PEXT5, _Rd_ );
}

////////////////////////////////////////////////////
void recPPAC5()
{
	REC_FUNC_INLINE( PPAC5, _Rd_ );
}

////////////////////////////////////////////////////
void recPMAXH( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D == EEREC_S ) SSE2_PMAXSW_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PMAXSW_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PMAXSW_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	if( cpucaps.hasStreamingSIMDExtensions ) {
		MMX_ALLOC_TEMP4(
			MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
			MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			SSE_PMAXSW_MM_to_MM( t0reg, t1reg );
			MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
			MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
			SSE_PMAXSW_MM_to_MM( t2reg, t3reg);
			MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
			MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t2reg);
			SetMMXstate();
			)
	}
	else {
		MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
		MOV32ItoM( (uptr)&cpuRegs.pc, pc );
		CALLFunc( (u32)PMAXH ); 
	}
}

////////////////////////////////////////////////////
void recPCGTB( void )
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D != EEREC_T ) {
		if( EEREC_D != EEREC_S ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPGTB_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPGTB_XMM_to_XMM(EEREC_D, t0reg);
		_freeXMMreg(t0reg);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		PCMPGTBRtoR( t0reg, t1reg );

		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PCMPGTBRtoR( t2reg, t3reg);

		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t2reg);
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPCGTH( void )
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D != EEREC_T ) {
		if( EEREC_D != EEREC_S ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPGTW_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPGTW_XMM_to_XMM(EEREC_D, t0reg);
		_freeXMMreg(t0reg);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );

		PCMPGTWRtoR( t0reg, t1reg );
		PCMPGTWRtoR( t2reg, t3reg);

		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t2reg);
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPCGTW( void )
{
	//TODO:optimize RS | RT== 0
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D != EEREC_T ) {
		if( EEREC_D != EEREC_S ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPGTD_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPGTD_XMM_to_XMM(EEREC_D, t0reg);
		_freeXMMreg(t0reg);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );

		PCMPGTDRtoR( t0reg, t1reg );
		PCMPGTDRtoR( t2reg, t3reg);

		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t2reg);
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPADDSB( void ) 
{
	if ( ! _Rd_ ) return;
	
CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D == EEREC_S ) SSE2_PADDSB_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PADDSB_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PADDSB_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PADDSBRtoR( t0reg, t2reg);
		PADDSBRtoR( t1reg, t3reg);
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPADDSH( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D == EEREC_S ) SSE2_PADDSW_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PADDSW_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PADDSW_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PADDSWRtoR( t0reg, t2reg);
		PADDSWRtoR( t1reg, t3reg);
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
//NOTE: check kh2 movies if changing this
void recPADDSW( void ) 
{
	if ( ! _Rd_ ) return;

//CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
//CPU_SSE_XMMCACHE_END

	if( _Rd_ ) _deleteEEreg(_Rd_, 0);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	_flushConstRegs();

	MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (uptr)&cpuRegs.pc, pc );
	CALLFunc( (u32)PADDSW ); 
}

////////////////////////////////////////////////////
void recPSUBSB( void ) 
{
   if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D == EEREC_S ) SSE2_PSUBSB_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBSB_XMM_to_XMM(EEREC_D, t0reg);
		_freeXMMreg(t0reg);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBSB_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PSUBSBRtoR( t0reg, t2reg);
		PSUBSBRtoR( t1reg, t3reg);
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPSUBSH( void ) 
{
	if ( ! _Rd_ ) return;
   
CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D == EEREC_S ) SSE2_PSUBSW_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBSW_XMM_to_XMM(EEREC_D, t0reg);
		_freeXMMreg(t0reg);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBSW_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PSUBSWRtoR( t0reg, t2reg);
		PSUBSWRtoR( t1reg, t3reg);
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
//NOTE: check kh2 movies if changing this
void recPSUBSW( void ) 
{
	if ( ! _Rd_ ) return;

//CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
//CPU_SSE_XMMCACHE_END

	if( _Rd_ ) _deleteEEreg(_Rd_, 0);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	_flushConstRegs();

	MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (uptr)&cpuRegs.pc, pc );
	CALLFunc( (u32)PSUBSW ); 
}

////////////////////////////////////////////////////
void recPADDB( void )
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D == EEREC_S ) SSE2_PADDB_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PADDB_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PADDB_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PADDBRtoR( t0reg, t2reg );
		PADDBRtoR( t1reg, t3reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPADDH( void )
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START((_Rs_!=0?XMMINFO_READS:0)|(_Rt_!=0?XMMINFO_READT:0)|XMMINFO_WRITED)
	if( _Rs_ == 0 ) {
		if( _Rt_ == 0 ) SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
		else if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else if( _Rt_ == 0 ) {
		if( EEREC_D != EEREC_S ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
	}
	else {
		if( EEREC_D == EEREC_S ) SSE2_PADDW_XMM_to_XMM(EEREC_D, EEREC_T);
		else if( EEREC_D == EEREC_T ) SSE2_PADDW_XMM_to_XMM(EEREC_D, EEREC_S);
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE2_PADDW_XMM_to_XMM(EEREC_D, EEREC_T);
		}
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PADDWRtoR( t0reg, t2reg );
		PADDWRtoR( t1reg, t3reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPADDW( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START((_Rs_!=0?XMMINFO_READS:0)|(_Rt_!=0?XMMINFO_READT:0)|XMMINFO_WRITED)
	if( _Rs_ == 0 ) {
		if( _Rt_ == 0 ) SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
		else if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else if( _Rt_ == 0 ) {
		if( EEREC_D != EEREC_S ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
	}
	else {
		if( EEREC_D == EEREC_S ) SSE2_PADDD_XMM_to_XMM(EEREC_D, EEREC_T);
		else if( EEREC_D == EEREC_T ) SSE2_PADDD_XMM_to_XMM(EEREC_D, EEREC_S);
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE2_PADDD_XMM_to_XMM(EEREC_D, EEREC_T);
		}
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PADDDRtoR( t0reg, t2reg );
		PADDDRtoR( t1reg, t3reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPSUBB( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D == EEREC_S ) SSE2_PSUBB_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBB_XMM_to_XMM(EEREC_D, t0reg);
		_freeXMMreg(t0reg);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBB_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PSUBBRtoR( t0reg, t2reg );
		PSUBBRtoR( t1reg, t3reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPSUBH( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D == EEREC_S ) SSE2_PSUBW_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBW_XMM_to_XMM(EEREC_D, t0reg);
		_freeXMMreg(t0reg);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBW_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PSUBWRtoR( t0reg, t2reg );
		PSUBWRtoR( t1reg, t3reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPSUBW( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D == EEREC_S ) SSE2_PSUBD_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBD_XMM_to_XMM(EEREC_D, t0reg);
		_freeXMMreg(t0reg);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBD_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PSUBDRtoR( t0reg, t2reg);
		PSUBDRtoR( t1reg, t3reg);
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPEXTLW( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START((_Rs_!=0?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED)
	if( _Rs_ == 0 ) {
		SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSRLQ_I8_to_XMM(EEREC_D, 32);
	}
	else {
		if( EEREC_D == EEREC_T ) SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, EEREC_S);
		else if( EEREC_D == EEREC_S ) {
			int t0reg = _allocTempXMMreg(XMMT_INT, -1);
			SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t0reg);
			_freeXMMreg(t0reg);
		}
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, EEREC_S);
		}
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	MOV32MtoR( ECX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 3 ], EAX );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 2 ], ECX );

	MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	MOV32MtoR( ECX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EAX );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], ECX );
}

void recPEXTLB( void ) 
{
	if (!_Rd_) return;

CPU_SSE2_XMMCACHE_START((_Rs_!=0?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED)
	if( _Rs_ == 0 ) {
		SSE2_PUNPCKLBW_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSRLW_I8_to_XMM(EEREC_D, 8);
	}
	else {
		if( EEREC_D == EEREC_T ) SSE2_PUNPCKLBW_XMM_to_XMM(EEREC_D, EEREC_S);
		else if( EEREC_D == EEREC_S ) {
			int t0reg = _allocTempXMMreg(XMMT_INT, -1);
			SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PUNPCKLBW_XMM_to_XMM(EEREC_D, t0reg);
			_freeXMMreg(t0reg);
		}
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PUNPCKLBW_XMM_to_XMM(EEREC_D, EEREC_S);
		}
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	//Done - Refraction - Crude but quicker than int
	//SysPrintf("PEXTLB\n");
	//Rs = cpuRegs.GPR.r[_Rs_]; Rt = cpuRegs.GPR.r[_Rt_];
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UC[7]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[15], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UC[7]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[14], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UC[6]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[13], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UC[6]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[12], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UC[5]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[11], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UC[5]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[10], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UC[4]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[9], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UC[4]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[8], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UC[3]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[7], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UC[3]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[6], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UC[2]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[5], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UC[2]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[4], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UC[1]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[3], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UC[1]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[2], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UC[0]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[1], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UC[0]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[0], EAX);	
}

void recPEXTLH( void )
{
	if (!_Rd_) return;

CPU_SSE2_XMMCACHE_START((_Rs_!=0?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED)
	if( _Rs_ == 0 ) {
		SSE2_PUNPCKLWD_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSRLD_I8_to_XMM(EEREC_D, 16);
	}
	else {
		if( EEREC_D == EEREC_T ) SSE2_PUNPCKLWD_XMM_to_XMM(EEREC_D, EEREC_S);
		else if( EEREC_D == EEREC_S ) {
			int t0reg = _allocTempXMMreg(XMMT_INT, -1);
			SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PUNPCKLWD_XMM_to_XMM(EEREC_D, t0reg);
			_freeXMMreg(t0reg);
		}
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PUNPCKLWD_XMM_to_XMM(EEREC_D, EEREC_S);
		}
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	//Done - Refraction - Crude but quicker than int
	MOV16MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].US[3]);
	MOV16RtoM((uptr)&cpuRegs.GPR.r[_Rd_].US[7], EAX);
	MOV16MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].US[3]);
	MOV16RtoM((uptr)&cpuRegs.GPR.r[_Rd_].US[6], EAX);
	MOV16MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].US[2]);
	MOV16RtoM((uptr)&cpuRegs.GPR.r[_Rd_].US[5], EAX);
	MOV16MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].US[2]);
	MOV16RtoM((uptr)&cpuRegs.GPR.r[_Rd_].US[4], EAX);
	MOV16MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].US[1]);
	MOV16RtoM((uptr)&cpuRegs.GPR.r[_Rd_].US[3], EAX);
	MOV16MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].US[1]);
	MOV16RtoM((uptr)&cpuRegs.GPR.r[_Rd_].US[2], EAX);
	MOV16MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].US[0]);
	MOV16RtoM((uptr)&cpuRegs.GPR.r[_Rd_].US[1], EAX);
	MOV16MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].US[0]);
	MOV16RtoM((uptr)&cpuRegs.GPR.r[_Rd_].US[0], EAX);
}

#endif

/*********************************************************
*   MMI1 opcodes                                         *
*                                                        *
*********************************************************/
#ifndef MMI1_RECOMPILE

REC_FUNC( PABSW, _Rd_);
REC_FUNC( PABSH, _Rd_);

REC_FUNC( PMINW, _Rd_); 
REC_FUNC( PADSBH, _Rd_);
REC_FUNC( PMINH, _Rd_);
REC_FUNC( PCEQB, _Rd_);   
REC_FUNC( PCEQH, _Rd_);
REC_FUNC( PCEQW, _Rd_);

REC_FUNC( PADDUB, _Rd_);
REC_FUNC( PADDUH, _Rd_);
REC_FUNC( PADDUW, _Rd_);

REC_FUNC( PSUBUB, _Rd_);
REC_FUNC( PSUBUH, _Rd_);
REC_FUNC( PSUBUW, _Rd_);

REC_FUNC( PEXTUW, _Rd_);   
REC_FUNC( PEXTUH, _Rd_);
REC_FUNC( PEXTUB, _Rd_);
REC_FUNC( QFSRV, _Rd_); 

#else

////////////////////////////////////////////////////
PCSX2_ALIGNED16(int s_MaskHighBitD[4]) = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
PCSX2_ALIGNED16(int s_MaskHighBitW[4]) = { 0x80008000, 0x80008000, 0x80008000, 0x80008000 };

void recPABSW()
{
	if( !_Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READT|XMMINFO_WRITED)
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
	if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
	SSE2_PSRAD_I8_to_XMM(t0reg, 31);
	SSEX_PXOR_XMM_to_XMM(EEREC_D, t0reg);
	SSE2_PSUBD_XMM_to_XMM(EEREC_D, t0reg);
	_freeXMMreg(t0reg);
CPU_SSE_XMMCACHE_END

	_deleteEEreg(_Rt_, 1);
	_deleteEEreg(_Rd_, 0);
	_flushConstRegs();

	MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (uptr)&cpuRegs.pc, pc );
	CALLFunc( (u32)PABSW ); 
}

////////////////////////////////////////////////////
void recPABSH()
{
if( !_Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READT|XMMINFO_WRITED)
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
	if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
	SSE2_PSRAW_I8_to_XMM(t0reg, 15);
	SSEX_PXOR_XMM_to_XMM(EEREC_D, t0reg);
	SSE2_PSUBW_XMM_to_XMM(EEREC_D, t0reg);
	_freeXMMreg(t0reg);
CPU_SSE_XMMCACHE_END

	_deleteEEreg(_Rt_, 1);
	_deleteEEreg(_Rd_, 0);
	_flushConstRegs();

	MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (uptr)&cpuRegs.pc, pc );
	CALLFunc( (u32)PABSW );
}

////////////////////////////////////////////////////
void recPMINW()
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	int t0reg;

	if( EEREC_S == EEREC_T ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		return;
	}

	t0reg = _allocTempXMMreg(XMMT_INT, -1);
	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
	SSE2_PCMPGTD_XMM_to_XMM(t0reg, EEREC_S);

	if( EEREC_D == EEREC_S ) {
		SSEX_PAND_XMM_to_XMM(EEREC_D, t0reg);
		SSEX_PANDN_XMM_to_XMM(t0reg, EEREC_T);
	}
	else if( EEREC_D == EEREC_T ) {
		int t1reg = _allocTempXMMreg(XMMT_INT, -1);
		SSEX_MOVDQA_XMM_to_XMM(t1reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSEX_PAND_XMM_to_XMM(EEREC_D, t0reg);
		SSEX_PANDN_XMM_to_XMM(t0reg, t1reg);
		_freeXMMreg(t1reg);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSEX_PAND_XMM_to_XMM(EEREC_D, t0reg);
		SSEX_PANDN_XMM_to_XMM(t0reg, EEREC_T);
	}
	
	SSEX_POR_XMM_to_XMM(EEREC_D, t0reg);
	_freeXMMreg(t0reg);
CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE( PMINW, _Rd_ );
}

////////////////////////////////////////////////////
void recPADSBH()
{
CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	int t0reg;

	if( EEREC_S == EEREC_T ) {
		if( EEREC_D != EEREC_S ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PADDW_XMM_to_XMM(EEREC_D, EEREC_D);
		// reset lower bits to 0s
		SSE2_PSRLDQ_I8_to_XMM(EEREC_D, 8);
		SSE2_PSLLDQ_I8_to_XMM(EEREC_D, 8);
		return;
	}

	t0reg = _allocTempXMMreg(XMMT_INT, -1);

	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);

	if( EEREC_D == EEREC_S ) {
		SSE2_PADDW_XMM_to_XMM(t0reg, EEREC_S);
		SSE2_PSUBW_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBW_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PADDW_XMM_to_XMM(t0reg, EEREC_S);
	}

	// t0reg - adds, EEREC_D - subs
	SSE2_PSRLDQ_I8_to_XMM(t0reg, 8);
	SSE_MOVLHPS_XMM_to_XMM(EEREC_D, t0reg);
	_freeXMMreg(t0reg);

CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE(PADSBH, _Rd_);
}

////////////////////////////////////////////////////
void recPADDUW()
{
CPU_SSE2_XMMCACHE_START((_Rs_?XMMINFO_READS:0)|(_Rt_?XMMINFO_READT:0)|XMMINFO_WRITED)

	if( _Rt_ == 0 ) {
		if( _Rs_ == 0 ) {
			SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
		}
		else if( EEREC_D != EEREC_S ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
	}
	else if( _Rs_ == 0 ) {
		if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else {

		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		int t1reg = _allocTempXMMreg(XMMT_INT, -1);
		int t2reg = _allocTempXMMreg(XMMT_INT, -1);

		if( _hasFreeXMMreg() ) {
			int t3reg = _allocTempXMMreg(XMMT_INT, -1);
			SSEX_PXOR_XMM_to_XMM(t0reg, t0reg);
			SSE2_MOVQ_XMM_to_XMM(t1reg, EEREC_S);
			SSEX_MOVDQA_XMM_to_XMM(t2reg, EEREC_S);
			if( EEREC_D != EEREC_T ) SSE2_MOVQ_XMM_to_XMM(EEREC_D, EEREC_T);
			SSEX_MOVDQA_XMM_to_XMM(t3reg, EEREC_T);
			SSE2_PUNPCKLDQ_XMM_to_XMM(t1reg, t0reg);
			SSE2_PUNPCKHDQ_XMM_to_XMM(t2reg, t0reg);
			SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t0reg);
			SSE2_PUNPCKHDQ_XMM_to_XMM(t3reg, t0reg);
			SSE2_PADDQ_XMM_to_XMM(t1reg, EEREC_D);
			SSE2_PADDQ_XMM_to_XMM(t2reg, t3reg);
			_freeXMMreg(t3reg);
		}
		else {
			SSEX_MOVDQA_XMM_to_XMM(t2reg, EEREC_S);
			SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);

			SSE2_MOVQ_XMM_to_XMM(t1reg, EEREC_S);
			SSE2_PSRLDQ_I8_to_XMM(t2reg, 8);
			if( EEREC_D != EEREC_T ) SSE2_MOVQ_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PSRLDQ_I8_to_XMM(t0reg, 8);
			SSE2_PSHUFD_XMM_to_XMM(t1reg, t1reg, 0xE8);
			SSE2_PSHUFD_XMM_to_XMM(t2reg, t2reg, 0xE8);
			SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_D, 0xE8);
			SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0xE8);
			SSE2_PADDQ_XMM_to_XMM(t1reg, EEREC_D);
			SSE2_PADDQ_XMM_to_XMM(t2reg, t0reg);
			SSEX_PXOR_XMM_to_XMM(t0reg, t0reg);
		}
		
		SSE2_PSHUFD_XMM_to_XMM(t1reg, t1reg, 0xd8);
		SSE2_PSHUFD_XMM_to_XMM(t2reg, t2reg, 0xd8);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, t1reg);
		SSE2_PUNPCKHQDQ_XMM_to_XMM(t1reg, t2reg);
		SSE2_PUNPCKLQDQ_XMM_to_XMM(EEREC_D, t2reg);
		SSE2_PCMPGTD_XMM_to_XMM(t1reg, t0reg);
		SSEX_POR_XMM_to_XMM(EEREC_D, t1reg);

		_freeXMMreg(t0reg);
		_freeXMMreg(t1reg);
		_freeXMMreg(t2reg);
	}

CPU_SSE_XMMCACHE_END
	REC_FUNC_INLINE( PADDUW, _Rd_ );
}

////////////////////////////////////////////////////
void recPSUBUB()
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D == EEREC_S ) SSE2_PSUBUSB_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PSUBUSB_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBUSB_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE( PSUBUB, _Rd_ );
}

////////////////////////////////////////////////////
void recPSUBUH()
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D == EEREC_S ) SSE2_PSUBUSW_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PSUBUSW_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBUSW_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE( PSUBUH, _Rd_ );
}

////////////////////////////////////////////////////
void recPSUBUW()
{
	REC_FUNC_INLINE( PSUBUW, _Rd_ );
}

////////////////////////////////////////////////////
void recPEXTUH()
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START((_Rs_!=0?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED)
	if( _Rs_ == 0 ) {
		SSE2_PUNPCKHWD_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSRLD_I8_to_XMM(EEREC_D, 16);
	}
	else {
		if( EEREC_D == EEREC_T ) SSE2_PUNPCKHWD_XMM_to_XMM(EEREC_D, EEREC_S);
		else if( EEREC_D == EEREC_S ) {
			int t0reg = _allocTempXMMreg(XMMT_INT, -1);
			SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PUNPCKHWD_XMM_to_XMM(EEREC_D, t0reg);
			_freeXMMreg(t0reg);
		}
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PUNPCKHWD_XMM_to_XMM(EEREC_D, EEREC_S);
		}
	}
CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE( PEXTUH, _Rd_ );
}

////////////////////////////////////////////////////
void recQFSRV()
{
	//u8* pshift1, *pshift2, *poldptr, *pnewptr;

	if ( ! _Rd_ ) return;
/*
CPU_SSE2_XMMCACHE_START((_Rs_!=0?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED)

	if( _Rs_ == 0 ) {
		MOV32MtoR(EAX, (uptr)&cpuRegs.sa);
		SHR32ItoR(EAX, 3);
		
		poldptr = x86Ptr;
		x86Ptr += 6;

		if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSRLDQ_I8_to_XMM(EEREC_D, 0);
		pshift1 = x86Ptr-1;

		pnewptr = x86Ptr;
		x86Ptr = poldptr;

		MOV8RtoM((uptr)pshift1, EAX);
		x86Ptr = pnewptr;
	}
	else {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);

		MOV32MtoR(EAX, (uptr)&cpuRegs.sa);
		SHR32ItoR(EAX, 3);
		MOV32ItoR(ECX, 16);
		SUB32RtoR(ECX, EAX);
		
		poldptr = x86Ptr;
		x86Ptr += 12;

		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
		SSE2_PSRLDQ_I8_to_XMM(t0reg, 0);
		pshift1 = x86Ptr-1;

		if( EEREC_D != EEREC_S ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSLLDQ_I8_to_XMM(EEREC_D, 0);
		pshift2 = x86Ptr-1;

		pnewptr = x86Ptr;
		x86Ptr = poldptr;

		MOV8RtoM((uptr)pshift1, EAX);
		MOV8RtoM((uptr)pshift2, ECX);

		x86Ptr = pnewptr;

		SSEX_POR_XMM_to_XMM(EEREC_D, t0reg);

		_freeXMMreg(t0reg);
	}

CPU_SSE_XMMCACHE_END*/

	REC_FUNC_INLINE( QFSRV, _Rd_ );
}


void recPEXTUB( void )
{
	if (!_Rd_) return;

CPU_SSE2_XMMCACHE_START((_Rs_!=0?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED)
	if( _Rs_ == 0 ) {
		SSE2_PUNPCKHBW_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSRLW_I8_to_XMM(EEREC_D, 8);
	}
	else {
		if( EEREC_D == EEREC_T ) SSE2_PUNPCKHBW_XMM_to_XMM(EEREC_D, EEREC_S);
		else if( EEREC_D == EEREC_S ) {
			int t0reg = _allocTempXMMreg(XMMT_INT, -1);
			SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PUNPCKHBW_XMM_to_XMM(EEREC_D, t0reg);
			_freeXMMreg(t0reg);
		}
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PUNPCKHBW_XMM_to_XMM(EEREC_D, EEREC_S);
		}
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	//Done - Refraction - Crude but faster than int
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UC[8]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[0], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UC[8]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[1], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UC[9]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[2], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UC[9]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[3], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UC[10]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[4], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UC[10]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[5], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UC[11]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[6], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UC[11]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[7], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UC[12]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[8], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UC[12]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[9], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UC[13]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[10], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UC[13]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[11], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UC[14]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[12], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UC[14]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[13], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UC[15]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[14], EAX);
	MOV8MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UC[15]);
	MOV8RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UC[15], EAX);
}

////////////////////////////////////////////////////
void recPEXTUW( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START((_Rs_!=0?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED)
	if( _Rs_ == 0 ) {
		SSE2_PUNPCKHDQ_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSRLQ_I8_to_XMM(EEREC_D, 32);
	}
	else {
		if( EEREC_D == EEREC_T ) SSE2_PUNPCKHDQ_XMM_to_XMM(EEREC_D, EEREC_S);
		else if( EEREC_D == EEREC_S ) {
			int t0reg = _allocTempXMMreg(XMMT_INT, -1);
			SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PUNPCKHDQ_XMM_to_XMM(EEREC_D, t0reg);
			_freeXMMreg(t0reg);
		}
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PUNPCKHDQ_XMM_to_XMM(EEREC_D, EEREC_S);
		}
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 2 ] );
	MOV32MtoR( ECX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 2 ] );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EAX );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], ECX );
	
	MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 3 ] );
	MOV32MtoR( ECX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 3 ] );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 3 ], EAX );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 2 ], ECX );
}

////////////////////////////////////////////////////
void recPMINH( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D == EEREC_S ) SSE2_PMINSW_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PMINSW_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PMINSW_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	if( cpucaps.hasStreamingSIMDExtensions ) {
		MMX_ALLOC_TEMP4(
			MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
			MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
			MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
			SSE_PMINSW_MM_to_MM( t0reg, t2reg );
			SSE_PMINSW_MM_to_MM( t1reg, t3reg );
			MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
			MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
			SetMMXstate();
			)
	}
	else {
		MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
		MOV32ItoM( (uptr)&cpuRegs.pc, pc );
		CALLFunc( (u32)PMINH ); 
	}
}

////////////////////////////////////////////////////
void recPCEQB( void )
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D == EEREC_S ) SSE2_PCMPEQB_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PCMPEQB_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPEQB_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PCMPEQBRtoR( t0reg, t2reg );
		PCMPEQBRtoR( t1reg, t3reg );

		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPCEQH( void )
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D == EEREC_S ) SSE2_PCMPEQW_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PCMPEQW_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPEQW_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PCMPEQWRtoR( t0reg, t2reg );
		PCMPEQWRtoR( t1reg, t3reg );
		
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPCEQW( void )
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D == EEREC_S ) SSE2_PCMPEQD_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PCMPEQD_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPEQD_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PCMPEQDRtoR( t0reg, t2reg );
		PCMPEQDRtoR( t1reg, t3reg );
		
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPADDUB( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|(_Rt_?XMMINFO_READT:0)|XMMINFO_WRITED)
	if( _Rt_ ) {
		if( EEREC_D == EEREC_S ) SSE2_PADDUSB_XMM_to_XMM(EEREC_D, EEREC_T);
		else if( EEREC_D == EEREC_T ) SSE2_PADDUSB_XMM_to_XMM(EEREC_D, EEREC_S);
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE2_PADDUSB_XMM_to_XMM(EEREC_D, EEREC_T);
		}
	}
	else {
		if( EEREC_D != EEREC_S ) SSE2_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PADDUSBRtoR( t0reg, t2reg );
		PADDUSBRtoR( t1reg, t3reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPADDUH( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D == EEREC_S ) SSE2_PADDUSW_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PADDUSW_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PADDUSW_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP4(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t2reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( t3reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PADDUSWRtoR( t0reg, t2reg );
		PADDUSWRtoR( t1reg, t3reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

#endif
/*********************************************************
*   MMI2 opcodes                                         *
*                                                        *
*********************************************************/
#ifndef MMI2_RECOMPILE

REC_FUNC( PMFHI, _Rd_);
REC_FUNC( PMFLO, _Rd_);
REC_FUNC( PCPYLD, _Rd_);
REC_FUNC( PAND, _Rd_);
REC_FUNC( PXOR, _Rd_); 

REC_FUNC( PMADDW, _Rd_);
REC_FUNC( PSLLVW, _Rd_);
REC_FUNC( PSRLVW, _Rd_); 
REC_FUNC( PMSUBW, _Rd_);
REC_FUNC( PINTH, _Rd_);
REC_FUNC( PMULTW, _Rd_);
REC_FUNC( PDIVW, _Rd_);
REC_FUNC( PMADDH, _Rd_);
REC_FUNC( PHMADH, _Rd_);
REC_FUNC( PMSUBH, _Rd_);
REC_FUNC( PHMSBH, _Rd_);
REC_FUNC( PEXEH, _Rd_);
REC_FUNC( PREVH, _Rd_); 
REC_FUNC( PMULTH, _Rd_);
REC_FUNC( PDIVBW, _Rd_);
REC_FUNC( PEXEW, _Rd_);
REC_FUNC( PROT3W, _Rd_ ); 

#else

////////////////////////////////////////////////////
void recPMADDW()
{
	EEINST_SETSIGNEXT(_Rs_);
	EEINST_SETSIGNEXT(_Rt_);
	if( _Rd_ ) EEINST_SETSIGNEXT(_Rd_);
	REC_FUNC_INLINE( PMADDW, _Rd_ );
}

////////////////////////////////////////////////////
void recPSLLVW()
{
	REC_FUNC_INLINE( PSLLVW, _Rd_ );
}

////////////////////////////////////////////////////
void recPSRLVW()
{
	REC_FUNC_INLINE( PSRLVW, _Rd_ ); 
}

////////////////////////////////////////////////////
void recPMSUBW()
{
	EEINST_SETSIGNEXT(_Rs_);
	EEINST_SETSIGNEXT(_Rt_);
	if( _Rd_ ) EEINST_SETSIGNEXT(_Rd_);
//CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED|XMMINFO_WRITELO|XMMINFO_WRITEHI|XMMINFO_READLO|XMMINFO_READHI)
//	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
//
//	if( EEREC_D == EEREC_S ) SSE2_PMULUDQ_XMM_to_XMM(EEREC_D, EEREC_T);
//	else if( EEREC_D == EEREC_T ) SSE2_PMULUDQ_XMM_to_XMM(EEREC_D, EEREC_S);
//	else {
//		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
//		SSE2_PMULUDQ_XMM_to_XMM(EEREC_D, EEREC_T);
//	}
//
//	// add from LO/HI
//	SSE_SHUFPS_XMM_to_XMM(EEREC_LO, EEREC_HI, 0x88);
//	SSE2_PSHUFD_XMM_to_XMM(EEREC_LO, EEREC_LO, 0xd8);
//	SSE2_PSUBQ_XMM_to_XMM(EEREC_LO, EEREC_D);
//
//	// get the signs
//	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_LO);
//	SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_LO);
//	SSE2_PSRAD_I8_to_XMM(t0reg, 31);
//
//	// interleave
//	SSE2_PSHUFD_XMM_to_XMM(EEREC_LO, EEREC_LO, 0xd8);
//	SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0xd8);
//	SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_LO);
//
//	SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_LO, t0reg);
//	SSE2_PUNPCKHDQ_XMM_to_XMM(EEREC_HI, t0reg);
//	
//	_freeXMMreg(t0reg);
//CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE( PMSUBW, _Rd_ );
}

////////////////////////////////////////////////////
void recPMULTW()
{
	EEINST_SETSIGNEXT(_Rs_);
	EEINST_SETSIGNEXT(_Rt_);
	if( _Rd_ ) EEINST_SETSIGNEXT(_Rd_);
	REC_FUNC_INLINE( PMULTW, _Rd_ );
}
////////////////////////////////////////////////////
void recPDIVW()
{
	EEINST_SETSIGNEXT(_Rs_);
	EEINST_SETSIGNEXT(_Rt_);
	REC_FUNC_INLINE( PDIVW, _Rd_ );
}

////////////////////////////////////////////////////
void recPDIVBW()
{
	REC_FUNC_INLINE( PDIVBW, _Rd_ ); //--
}

////////////////////////////////////////////////////
PCSX2_ALIGNED16(int s_mask[4]) = {~0, 0, ~0, 0};

void recPHMADH()
{
CPU_SSE2_XMMCACHE_START((_Rd_?XMMINFO_WRITED:0)|XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITELO|XMMINFO_WRITEHI)
	int t0reg = _Rd_ ? EEREC_D : _allocTempXMMreg(XMMT_INT, -1);

	if( t0reg == EEREC_S ) {

		SSEX_MOVDQA_XMM_to_XMM(EEREC_LO, EEREC_S);

		if( t0reg == EEREC_T ) {
			SSE2_PMULHW_XMM_to_XMM(EEREC_LO, EEREC_T);
			SSE2_PMULLW_XMM_to_XMM(t0reg, EEREC_T);
		}
		else {
			SSE2_PMULLW_XMM_to_XMM(t0reg, EEREC_T);
			SSE2_PMULHW_XMM_to_XMM(EEREC_LO, EEREC_T);
		}
		SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, t0reg);
	}
	else {
		if( t0reg != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_LO, EEREC_T);

		SSE2_PMULLW_XMM_to_XMM(t0reg, EEREC_S);
		SSE2_PMULHW_XMM_to_XMM(EEREC_LO, EEREC_S);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, t0reg);
	}

	// 0-3
	SSE2_PUNPCKLWD_XMM_to_XMM(t0reg, EEREC_LO);
	// 4-7
	SSE2_PUNPCKHWD_XMM_to_XMM(EEREC_HI, EEREC_LO);

	SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0xd8); // 0,2,1,3, L->H
	SSE2_PSHUFD_XMM_to_XMM(EEREC_HI, EEREC_HI, 0xd8); // 4,6,5,7, L->H
	SSEX_MOVDQA_XMM_to_XMM(EEREC_LO, t0reg);
	
	SSE2_PUNPCKLQDQ_XMM_to_XMM(t0reg, EEREC_HI);
	SSE2_PUNPCKHQDQ_XMM_to_XMM(EEREC_LO, EEREC_HI);

	SSE2_PADDD_XMM_to_XMM(EEREC_LO, t0reg);

	if( _Rd_ ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_LO);
	}

	SSE2_PSHUFD_XMM_to_XMM(EEREC_HI, EEREC_LO, 0xf5);

	SSE2_PAND_M128_to_XMM(EEREC_LO, (u32)s_mask);
	SSE2_PAND_M128_to_XMM(EEREC_HI, (u32)s_mask);

	if( !_Rd_ ) _freeXMMreg(t0reg);

CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE( PHMADH, _Rd_ );
}

////////////////////////////////////////////////////
void recPMSUBH()
{
CPU_SSE2_XMMCACHE_START((_Rd_?XMMINFO_WRITED:0)|XMMINFO_READS|XMMINFO_READT|XMMINFO_READLO|XMMINFO_READHI|XMMINFO_WRITELO|XMMINFO_WRITEHI)
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	int t1reg = _allocTempXMMreg(XMMT_INT, -1);

	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
	SSEX_MOVDQA_XMM_to_XMM(t1reg, EEREC_S);

	SSE2_PMULLW_XMM_to_XMM(t0reg, EEREC_T);
	SSE2_PMULHW_XMM_to_XMM(t1reg, EEREC_T);
	SSEX_MOVDQA_XMM_to_XMM(EEREC_D, t0reg);

	// 0-3
	SSE2_PUNPCKLWD_XMM_to_XMM(t0reg, t1reg);
	// 4-7
	SSE2_PUNPCKHWD_XMM_to_XMM(EEREC_D, t1reg);
	SSEX_MOVDQA_XMM_to_XMM(t1reg, t0reg);

	// 0,1,4,5, L->H
	SSE2_PUNPCKLQDQ_XMM_to_XMM(t0reg, EEREC_D);
	// 2,3,6,7, L->H
	SSE2_PUNPCKHQDQ_XMM_to_XMM(t1reg, EEREC_D);

	SSE2_PSUBD_XMM_to_XMM(EEREC_LO, t0reg);
	SSE2_PSUBD_XMM_to_XMM(EEREC_HI, t1reg);

	if( _Rd_ ) {
		// 0,2,4,6, L->H
		SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_LO, 0x88);
		SSE2_PSHUFD_XMM_to_XMM(t0reg, EEREC_HI, 0x88);
		SSE2_PUNPCKLQDQ_XMM_to_XMM(EEREC_D, t0reg);
	}

	_freeXMMreg(t0reg);
	_freeXMMreg(t1reg);

CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE( PMSUBH, _Rd_ );
}

////////////////////////////////////////////////////
void recPHMSBH()
{
CPU_SSE2_XMMCACHE_START((_Rd_?XMMINFO_WRITED:0)|XMMINFO_READS|XMMINFO_READT|XMMINFO_READLO|XMMINFO_READHI|XMMINFO_WRITELO|XMMINFO_WRITEHI)
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);

	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
	SSEX_MOVDQA_XMM_to_XMM(EEREC_LO, EEREC_S);

	SSE2_PMULLW_XMM_to_XMM(t0reg, EEREC_T);
	SSE2_PMULHW_XMM_to_XMM(EEREC_LO, EEREC_T);
	SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, t0reg);

	// 0-3
	SSE2_PUNPCKLWD_XMM_to_XMM(t0reg, EEREC_LO);
	// 4-7
	SSE2_PUNPCKHWD_XMM_to_XMM(EEREC_HI, EEREC_LO);

	SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0xd8); // 0,2,1,3, L->H
	SSE2_PSHUFD_XMM_to_XMM(EEREC_HI, EEREC_HI, 0xd8); // 4,6,5,7, L->H
	SSEX_MOVDQA_XMM_to_XMM(EEREC_LO, t0reg);
	
	SSE2_PUNPCKLDQ_XMM_to_XMM(t0reg, EEREC_HI);
	SSE2_PUNPCKHDQ_XMM_to_XMM(EEREC_LO, EEREC_HI);

	SSE2_PSUBD_XMM_to_XMM(EEREC_LO, t0reg);

	if( _Rd_ ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_LO);
	}

	SSE2_PSHUFD_XMM_to_XMM(EEREC_HI, EEREC_LO, 0xf5);

	_freeXMMreg(t0reg);

CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE( PHMSBH, _Rd_ );
}

////////////////////////////////////////////////////
void recPEXEH( void )
{
	if (!_Rd_) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READT|XMMINFO_WRITED)
	SSE2_PSHUFLW_XMM_to_XMM(EEREC_D, EEREC_T, 0xc6);
	SSE2_PSHUFHW_XMM_to_XMM(EEREC_D, EEREC_D, 0xc6);
CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE( PEXEH, _Rd_ );
}

////////////////////////////////////////////////////
void recPREVH( void )
{
	if (!_Rd_) return;

	
CPU_SSE2_XMMCACHE_START(XMMINFO_READT|XMMINFO_WRITED)
	SSE2_PSHUFLW_XMM_to_XMM(EEREC_D, EEREC_T, 0x1B);
	SSE2_PSHUFHW_XMM_to_XMM(EEREC_D, EEREC_D, 0x1B);
CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE( PREVH, _Rd_ );
}

////////////////////////////////////////////////////
void recPINTH( void )
{
	if (!_Rd_) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	if( EEREC_D == EEREC_S ) {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		SSE_MOVHLPS_XMM_to_XMM(t0reg, EEREC_S);
		if( EEREC_D != EEREC_T ) SSE2_MOVQ_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PUNPCKLWD_XMM_to_XMM(EEREC_D, t0reg);
		_freeXMMreg(t0reg);
	}
	else {
		SSE_MOVLHPS_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PUNPCKHWD_XMM_to_XMM(EEREC_D, EEREC_S);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	//Done - Refraction
	MOV16MtoR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].US[4]);
	MOV16MtoR( EBX, (uptr)&cpuRegs.GPR.r[_Rt_].US[1]);
	MOV16MtoR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].US[2]);
	MOV16MtoR( EDX, (uptr)&cpuRegs.GPR.r[_Rt_].US[0]);

	MOV16RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].US[1], EAX);
	MOV16RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].US[2], EBX);
	MOV16RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].US[4], ECX);
	MOV16RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].US[0], EDX);

	MOV16MtoR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].US[5]);
	MOV16MtoR( EBX, (uptr)&cpuRegs.GPR.r[_Rs_].US[6]);
	MOV16MtoR( ECX, (uptr)&cpuRegs.GPR.r[_Rs_].US[7]);
	MOV16MtoR( EDX, (uptr)&cpuRegs.GPR.r[_Rt_].US[3]);

	MOV16RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].US[3], EAX);
	MOV16RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].US[5], EBX);
	MOV16RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].US[7], ECX);
	MOV16RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].US[6], EDX);
}

void recPEXEW( void )
{
	if (!_Rd_) return;

CPU_SSE_XMMCACHE_START(XMMINFO_READT|XMMINFO_WRITED)
	SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_T, 0xc6);
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[2]);
	MOV32MtoR( EBX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[1]);
	MOV32MtoR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
	MOV32MtoR( EDX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[3]);

	MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[0], EAX);
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[1], EBX);
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[2], ECX);
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[3], EDX);
}

void recPROT3W( void )
{
	if (!_Rd_) return;

CPU_SSE_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED)
	SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_T, 0xc9);
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MOV32MtoR( EAX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[1]);
	MOV32MtoR( EBX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[2]);
	MOV32MtoR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
	MOV32MtoR( EDX, (uptr)&cpuRegs.GPR.r[_Rt_].UL[3]);

	MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[0], EAX);
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[1], EBX);
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[2], ECX);
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[3], EDX);
}

void recPMULTH( void )
{
CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|(_Rd_?XMMINFO_WRITED:0)|XMMINFO_WRITELO|XMMINFO_WRITEHI)
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);

	SSEX_MOVDQA_XMM_to_XMM(EEREC_LO, EEREC_S);
	SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_S);

	SSE2_PMULLW_XMM_to_XMM(EEREC_LO, EEREC_T);
	SSE2_PMULHW_XMM_to_XMM(EEREC_HI, EEREC_T);
	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_LO);

	// 0-3
	SSE2_PUNPCKLWD_XMM_to_XMM(EEREC_LO, EEREC_HI);
	// 4-7
	SSE2_PUNPCKHWD_XMM_to_XMM(t0reg, EEREC_HI);

	if( _Rd_ ) {
		// 0,2,4,6, L->H
		SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_LO, 0x88);
		SSE2_PSHUFD_XMM_to_XMM(EEREC_HI, t0reg, 0x88);
		SSE2_PUNPCKLQDQ_XMM_to_XMM(EEREC_D, EEREC_HI);
	}

	SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_LO);

	// 0,1,4,5, L->H
	SSE2_PUNPCKLQDQ_XMM_to_XMM(EEREC_LO, t0reg);
	// 2,3,6,7, L->H
	SSE2_PUNPCKHQDQ_XMM_to_XMM(EEREC_HI, t0reg);

	_freeXMMreg(t0reg);
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);
	_deleteEEreg(XMMGPR_LO, 0);
	_deleteEEreg(XMMGPR_HI, 0);
	_deleteGPRtoXMMreg(_Rs_, 1);
	_deleteGPRtoXMMreg(_Rt_, 1);

	if(!_Rt_ || !_Rs_) {
		MOV32ItoM( (uptr)&cpuRegs.LO.UL[0], 0);
		MOV32ItoM( (uptr)&cpuRegs.LO.UL[1], 0);
		MOV32ItoM( (uptr)&cpuRegs.LO.UL[2], 0);
		MOV32ItoM( (uptr)&cpuRegs.LO.UL[3], 0);
		MOV32ItoM( (uptr)&cpuRegs.HI.UL[0], 0);
		MOV32ItoM( (uptr)&cpuRegs.HI.UL[1], 0);
		MOV32ItoM( (uptr)&cpuRegs.HI.UL[2], 0);
		MOV32ItoM( (uptr)&cpuRegs.HI.UL[3], 0);

		if( _Rd_ ) {
			MOV32ItoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[0], 0);
			MOV32ItoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[1], 0);
			MOV32ItoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[2], 0);
			MOV32ItoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[3], 0);
		}
		return;
	}

	//Done - Refraction
	MOVSX32M16toR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].SS[0]);
	MOVSX32M16toR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].SS[0]);
	IMUL32RtoR( EAX, ECX);
	MOV32RtoM( (uptr)&cpuRegs.LO.UL[0], EAX);

    MOVSX32M16toR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].SS[1]);
	MOVSX32M16toR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].SS[1]);
	IMUL32RtoR( EAX, ECX);
	MOV32RtoM( (uptr)&cpuRegs.LO.UL[1], EAX);

    MOVSX32M16toR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].SS[2]);
	MOVSX32M16toR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].SS[2]);
	IMUL32RtoR( EAX, ECX);
	MOV32RtoM( (uptr)&cpuRegs.HI.UL[0], EAX);

    MOVSX32M16toR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].SS[3]);
	MOVSX32M16toR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].SS[3]);
	IMUL32RtoR( EAX, ECX);
	MOV32RtoM( (uptr)&cpuRegs.HI.UL[1], EAX);

    MOVSX32M16toR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].SS[4]);
	MOVSX32M16toR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].SS[4]);
	IMUL32RtoR( EAX, ECX);
	MOV32RtoM( (uptr)&cpuRegs.LO.UL[2], EAX);

    MOVSX32M16toR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].SS[5]);
	MOVSX32M16toR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].SS[5]);
	IMUL32RtoR( EAX, ECX);
	MOV32RtoM( (uptr)&cpuRegs.LO.UL[3], EAX);

    MOVSX32M16toR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].SS[6]);
	MOVSX32M16toR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].SS[6]);
	IMUL32RtoR( EAX, ECX);
	MOV32RtoM( (uptr)&cpuRegs.HI.UL[2], EAX);

    MOVSX32M16toR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].SS[7]);
	MOVSX32M16toR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].SS[7]);
	IMUL32RtoR( EAX, ECX);
	MOV32RtoM( (uptr)&cpuRegs.HI.UL[3], EAX);

	if (_Rd_) {
		MOV32MtoR( EAX, (uptr)&cpuRegs.LO.UL[0]);
		MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[0], EAX);
		MOV32MtoR( EAX, (uptr)&cpuRegs.HI.UL[0]);
		MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[1], EAX);
		MOV32MtoR( EAX, (uptr)&cpuRegs.LO.UL[2]);
		MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[2], EAX);
		MOV32MtoR( EAX, (uptr)&cpuRegs.HI.UL[2]);
		MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[3], EAX);
	}
}

void recPMFHI( void )
{
	if ( ! _Rd_ ) return;

CPU_SSE_XMMCACHE_START(XMMINFO_WRITED|XMMINFO_READHI)
	SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_HI);
CPU_SSE_XMMCACHE_END

	MMX_ALLOC_TEMP2(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.HI.UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.HI.UD[ 1 ] );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPMFLO( void )
{
	if ( ! _Rd_ ) return;

CPU_SSE_XMMCACHE_START(XMMINFO_WRITED|XMMINFO_READLO)
	SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_LO);
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP2(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.LO.UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.LO.UD[ 1 ] );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPAND( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE_XMMCACHE_START(XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT)
	if( EEREC_D == EEREC_T ) {
		SSEX_PAND_XMM_to_XMM(EEREC_D, EEREC_S);
	}
	else if( EEREC_D == EEREC_S ) {
		SSEX_PAND_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSEX_PAND_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP2(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		PANDMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		PANDMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPXOR( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE_XMMCACHE_START(XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT)
	if( EEREC_D == EEREC_T ) {
		SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_S);
	}
	else if( EEREC_D == EEREC_S ) {
		SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_T);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP2(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		PXORMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		PXORMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );

		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPCPYLD( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE_XMMCACHE_START(XMMINFO_WRITED|((cpucaps.hasStreamingSIMD2Extensions&&_Rs_==0)?0:XMMINFO_READS)|XMMINFO_READT)
	if( cpucaps.hasStreamingSIMD2Extensions ) {
		if( _Rs_ == 0 ) {
			SSE2_MOVQ_XMM_to_XMM(EEREC_D, EEREC_T);
		}
		else {
			if( EEREC_D == EEREC_T ) SSE2_PUNPCKLQDQ_XMM_to_XMM(EEREC_D, EEREC_S);
			else if( EEREC_S == EEREC_T ) SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_S, 0x44);
			else if( EEREC_D == EEREC_S ) {
				SSE2_PUNPCKLQDQ_XMM_to_XMM(EEREC_D, EEREC_T);
				SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_D, 0x4e);
			}
			else {
				SSE2_MOVQ_XMM_to_XMM(EEREC_D, EEREC_T);
				SSE2_PUNPCKLQDQ_XMM_to_XMM(EEREC_D, EEREC_S);
			}
		}
	}
	else {
		if( EEREC_D == EEREC_T ) SSE_MOVLHPS_XMM_to_XMM(EEREC_D, EEREC_S);
		else if( EEREC_D == EEREC_S ) {
			SSE_SHUFPS_XMM_to_XMM(EEREC_D, EEREC_T, 0x44);
			SSE_SHUFPS_XMM_to_XMM(EEREC_D, EEREC_D, 0x4e);
		}
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE_SHUFPS_XMM_to_XMM(EEREC_D, EEREC_S, 0x44);
		}
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP2(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t1reg );
		SetMMXstate();
		)
}


void recPMADDH( void ) 
{
CPU_SSE2_XMMCACHE_START((_Rd_?XMMINFO_WRITED:0)|XMMINFO_READS|XMMINFO_READT|XMMINFO_READLO|XMMINFO_READHI|XMMINFO_WRITELO|XMMINFO_WRITEHI)
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	int t1reg = _allocTempXMMreg(XMMT_INT, -1);

	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
	SSEX_MOVDQA_XMM_to_XMM(t1reg, EEREC_S);

	SSE2_PMULLW_XMM_to_XMM(t0reg, EEREC_T);
	SSE2_PMULHW_XMM_to_XMM(t1reg, EEREC_T);
	SSEX_MOVDQA_XMM_to_XMM(EEREC_D, t0reg);

	// 0-3
	SSE2_PUNPCKLWD_XMM_to_XMM(t0reg, t1reg);
	// 4-7
	SSE2_PUNPCKHWD_XMM_to_XMM(EEREC_D, t1reg);
	SSEX_MOVDQA_XMM_to_XMM(t1reg, t0reg);

	// 0,1,4,5, L->H
	SSE2_PUNPCKLQDQ_XMM_to_XMM(t0reg, EEREC_D);
	// 2,3,6,7, L->H
	SSE2_PUNPCKHQDQ_XMM_to_XMM(t1reg, EEREC_D);

	SSE2_PADDD_XMM_to_XMM(EEREC_LO, t0reg);
	SSE2_PADDD_XMM_to_XMM(EEREC_HI, t1reg);

	if( _Rd_ ) {
		// 0,2,4,6, L->H
		SSE2_PSHUFD_XMM_to_XMM(t0reg, EEREC_LO, 0x88);
		SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_HI, 0x88);
		SSE2_PUNPCKLQDQ_XMM_to_XMM(EEREC_D, t0reg);
	}

	_freeXMMreg(t0reg);
	_freeXMMreg(t1reg);

CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);
	_deleteEEreg(XMMGPR_LO, 1);
	_deleteEEreg(XMMGPR_HI, 1);
	_deleteGPRtoXMMreg(_Rs_, 1);
	_deleteGPRtoXMMreg(_Rt_, 1);

	if(_Rt_ && _Rs_){

	MOVSX32M16toR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].SS[0]);
	MOVSX32M16toR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].SS[0]);
	IMUL32RtoR( EAX, ECX);
	ADD32RtoM( (uptr)&cpuRegs.LO.UL[0], EAX);

    MOVSX32M16toR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].SS[1]);
	MOVSX32M16toR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].SS[1]);
	IMUL32RtoR( EAX, ECX);
	ADD32RtoM( (uptr)&cpuRegs.LO.UL[1], EAX);

    MOVSX32M16toR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].SS[2]);
	MOVSX32M16toR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].SS[2]);
	IMUL32RtoR( EAX, ECX);
	ADD32RtoM( (uptr)&cpuRegs.HI.UL[0], EAX);

    MOVSX32M16toR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].SS[3]);
	MOVSX32M16toR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].SS[3]);
	IMUL32RtoR( EAX, ECX);
	ADD32RtoM( (uptr)&cpuRegs.HI.UL[1], EAX);

    MOVSX32M16toR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].SS[4]);
	MOVSX32M16toR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].SS[4]);
	IMUL32RtoR( EAX, ECX);
	ADD32RtoM( (uptr)&cpuRegs.LO.UL[2], EAX);

    MOVSX32M16toR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].SS[5]);
	MOVSX32M16toR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].SS[5]);
	IMUL32RtoR( EAX, ECX);
	ADD32RtoM( (uptr)&cpuRegs.LO.UL[3], EAX);

    MOVSX32M16toR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].SS[6]);
	MOVSX32M16toR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].SS[6]);
	IMUL32RtoR( EAX, ECX);
	ADD32RtoM( (uptr)&cpuRegs.HI.UL[2], EAX);

    MOVSX32M16toR( EAX, (uptr)&cpuRegs.GPR.r[_Rs_].SS[7]);
	MOVSX32M16toR( ECX, (uptr)&cpuRegs.GPR.r[_Rt_].SS[7]);
	IMUL32RtoR( EAX, ECX);
	ADD32RtoM( (uptr)&cpuRegs.HI.UL[3], EAX);

	}

	if (_Rd_) {
		MOV32MtoR( EAX, (uptr)&cpuRegs.LO.UL[0]);
		MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[0], EAX);
		MOV32MtoR( EAX, (uptr)&cpuRegs.HI.UL[0]);
		MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[1], EAX);
		MOV32MtoR( EAX, (uptr)&cpuRegs.LO.UL[2]);
		MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[2], EAX);
		MOV32MtoR( EAX, (uptr)&cpuRegs.HI.UL[2]);
		MOV32RtoM( (uptr)&cpuRegs.GPR.r[_Rd_].UL[3], EAX);
	}
}

#endif
/*********************************************************
*   MMI3 opcodes                                         *
*                                                        *
*********************************************************/
#ifndef MMI3_RECOMPILE

REC_FUNC( PMADDUW, _Rd_);
REC_FUNC( PSRAVW, _Rd_); 
REC_FUNC( PMTHI, _Rd_);
REC_FUNC( PMTLO, _Rd_);
REC_FUNC( PINTEH, _Rd_);
REC_FUNC( PMULTUW, _Rd_);
REC_FUNC( PDIVUW, _Rd_);
REC_FUNC( PCPYUD, _Rd_);
REC_FUNC( POR, _Rd_);
REC_FUNC( PNOR, _Rd_);  
REC_FUNC( PCPYH, _Rd_); 
REC_FUNC( PEXCW, _Rd_);
REC_FUNC( PEXCH, _Rd_);

#else

////////////////////////////////////////////////////
REC_FUNC( PSRAVW, _Rd_ ); 

////////////////////////////////////////////////////
PCSX2_ALIGNED16(u32 s_tempPINTEH[4]) = {0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff };

void recPINTEH()
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START((_Rs_?XMMINFO_READS:0)|(_Rt_?XMMINFO_READT:0)|XMMINFO_WRITED)

	int t0reg = -1;

	if( _Rs_ == 0 ) {
		if( _Rt_ == 0 ) {
			SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
		}
		else {
			if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PAND_M128_to_XMM(EEREC_D, (u32)s_tempPINTEH);
		}
	}
	else if( _Rt_ == 0 ) {
		if( EEREC_D != EEREC_S ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSLLD_I8_to_XMM(EEREC_D, 16);
	}
	else {
		if( EEREC_S == EEREC_T ) {
			SSE2_PSHUFLW_XMM_to_XMM(EEREC_D, EEREC_S, 0xa0);
			SSE2_PSHUFHW_XMM_to_XMM(EEREC_D, EEREC_D, 0xa0);
		}
		else if( EEREC_D == EEREC_T ) {
			assert( EEREC_D != EEREC_S );
			t0reg = _allocTempXMMreg(XMMT_INT, -1);
			SSE2_PSLLD_I8_to_XMM(EEREC_D, 16);
			SSE2_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
			SSE2_PSRLD_I8_to_XMM(EEREC_D, 16);
			SSE2_PSLLD_I8_to_XMM(t0reg, 16);
			SSE2_POR_XMM_to_XMM(EEREC_D, t0reg);
		}
		else {
			t0reg = _allocTempXMMreg(XMMT_INT, -1);
			if( EEREC_D != EEREC_S) SSE2_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE2_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
			SSE2_PSLLD_I8_to_XMM(t0reg, 16);
			SSE2_PSLLD_I8_to_XMM(EEREC_D, 16);
			SSE2_PSRLD_I8_to_XMM(t0reg, 16);
			SSE2_POR_XMM_to_XMM(EEREC_D, t0reg);
		}
	}

	if( t0reg >= 0 ) _freeXMMreg(t0reg);
CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE( PINTEH, _Rd_ );
}

////////////////////////////////////////////////////
void recPMULTUW()
{
CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED|XMMINFO_WRITELO|XMMINFO_WRITEHI)
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	EEINST_SETSIGNEXT(_Rs_);
	EEINST_SETSIGNEXT(_Rt_);
	if( _Rd_ ) EEINST_SETSIGNEXT(_Rd_);

	if( EEREC_D == EEREC_S ) SSE2_PMULUDQ_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PMULUDQ_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PMULUDQ_XMM_to_XMM(EEREC_D, EEREC_T);
	}

	// get the signs
	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_D);
	SSE2_PSRAD_I8_to_XMM(t0reg, 31);

	// interleave
	SSE2_PSHUFD_XMM_to_XMM(EEREC_LO, EEREC_D, 0xd8);
	SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0xd8);
	SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_LO);

	SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_LO, t0reg);
	SSE2_PUNPCKHDQ_XMM_to_XMM(EEREC_HI, t0reg);
	
	_freeXMMreg(t0reg);
CPU_SSE_XMMCACHE_END
	REC_FUNC_INLINE( PMULTUW, _Rd_ );
}

////////////////////////////////////////////////////
void recPMADDUW()
{
CPU_SSE2_XMMCACHE_START(XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED|XMMINFO_WRITELO|XMMINFO_WRITEHI|XMMINFO_READLO|XMMINFO_READHI)
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	EEINST_SETSIGNEXT(_Rs_);
	EEINST_SETSIGNEXT(_Rt_);

	if( EEREC_D == EEREC_S ) SSE2_PMULUDQ_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PMULUDQ_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PMULUDQ_XMM_to_XMM(EEREC_D, EEREC_T);
	}

	// add from LO/HI
	SSE2_PSHUFD_XMM_to_XMM(EEREC_LO, EEREC_LO, 0x88);
	SSE2_PSHUFD_XMM_to_XMM(EEREC_HI, EEREC_HI, 0x88);
	SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_LO, EEREC_HI);
	SSE2_PADDQ_XMM_to_XMM(EEREC_D, EEREC_LO);

	// get the signs
	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_D);
	SSE2_PSRAD_I8_to_XMM(t0reg, 31);

	// interleave
	SSE2_PSHUFD_XMM_to_XMM(EEREC_LO, EEREC_D, 0xd8);
	SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0xd8);
	SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_LO);

	SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_LO, t0reg);
	SSE2_PUNPCKHDQ_XMM_to_XMM(EEREC_HI, t0reg);
	
	_freeXMMreg(t0reg);
CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE( PMADDUW, _Rd_ );
}

////////////////////////////////////////////////////
//do EEINST_SETSIGNEXT
REC_FUNC( PDIVUW, _Rd_ );

////////////////////////////////////////////////////
void recPEXCW()
{
	if (!_Rd_) return;

CPU_SSE_XMMCACHE_START(XMMINFO_READT|XMMINFO_WRITED)
	SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_T, 0xd8);
CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE( PEXCW, _Rd_ );
}

////////////////////////////////////////////////////
void recPEXCH( void )
{
	if (!_Rd_) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READT|XMMINFO_WRITED)
	SSE2_PSHUFLW_XMM_to_XMM(EEREC_D, EEREC_T, 0xd8);
	SSE2_PSHUFHW_XMM_to_XMM(EEREC_D, EEREC_D, 0xd8);
CPU_SSE_XMMCACHE_END

	REC_FUNC_INLINE( PEXCH, _Rd_ );
}

////////////////////////////////////////////////////
void recPNOR( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START((_Rs_!=0?XMMINFO_READS:0)|(_Rt_!=0?XMMINFO_READT:0)|XMMINFO_WRITED)

	if( _Rs_ == 0 ) {
		if( _Rt_ == 0 ) {
			SSE2_PCMPEQD_XMM_to_XMM( EEREC_D, EEREC_D );
		}
		else {
			if( EEREC_D == EEREC_T ) {
				int t0reg = _allocTempXMMreg(XMMT_INT, -1);
				SSE2_PCMPEQD_XMM_to_XMM( t0reg, t0reg);
				SSEX_PXOR_XMM_to_XMM(EEREC_D, t0reg);
				_freeXMMreg(t0reg);
			}
			else {
				SSE2_PCMPEQD_XMM_to_XMM( EEREC_D, EEREC_D );
				if( _Rt_ != 0 ) SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_T);
			}
		}
	}
	else if( _Rt_ == 0 ) {
		if( EEREC_D == EEREC_S ) {
			int t0reg = _allocTempXMMreg(XMMT_INT, -1);
			SSE2_PCMPEQD_XMM_to_XMM( t0reg, t0reg);
			SSEX_PXOR_XMM_to_XMM(EEREC_D, t0reg);
			_freeXMMreg(t0reg);
		}
		else {
			SSE2_PCMPEQD_XMM_to_XMM( EEREC_D, EEREC_D );
			SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_S);
		}
	}
	else {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);

		if( EEREC_D == EEREC_S ) SSEX_POR_XMM_to_XMM(EEREC_D, EEREC_T);
		else if( EEREC_D == EEREC_T ) SSEX_POR_XMM_to_XMM(EEREC_D, EEREC_S);
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
			if( EEREC_S != EEREC_T ) SSEX_POR_XMM_to_XMM(EEREC_D, EEREC_T);
		}

		SSE2_PCMPEQD_XMM_to_XMM( t0reg, t0reg );
		SSEX_PXOR_XMM_to_XMM( EEREC_D, t0reg );
		_freeXMMreg(t0reg);
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP3(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		PORMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		PORMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PCMPEQDRtoR( t2reg, t2reg );
		PXORRtoR( t0reg, t2reg );
		PXORRtoR( t1reg, t2reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPMTHI( void ) 
{
CPU_SSE_XMMCACHE_START(XMMINFO_READS|XMMINFO_WRITEHI)
	SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_S);
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(XMMGPR_HI, 0);

	MMX_ALLOC_TEMP2(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQRtoM( (uptr)&cpuRegs.HI.UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.HI.UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPMTLO( void ) 
{
CPU_SSE_XMMCACHE_START(XMMINFO_READS|XMMINFO_WRITELO)
	SSEX_MOVDQA_XMM_to_XMM(EEREC_LO, EEREC_S);
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(XMMGPR_LO, 0);
	_deleteGPRtoXMMreg(_Rs_, 1);

	MMX_ALLOC_TEMP2(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQRtoM( (uptr)&cpuRegs.LO.UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.LO.UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPCPYUD( void ) 
{
   if ( ! _Rd_ ) return;

CPU_SSE_XMMCACHE_START(XMMINFO_READS|((cpucaps.hasStreamingSIMD2Extensions&&_Rs_==0)?0:XMMINFO_READT)|XMMINFO_WRITED)

	if( cpucaps.hasStreamingSIMD2Extensions ) {
		if( _Rt_ == 0 ) {
			if( EEREC_D == EEREC_S ) {
				SSE2_PUNPCKHQDQ_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE2_MOVQ_XMM_to_XMM(EEREC_D, EEREC_D);
			}
			else {
				SSE_MOVHLPS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE2_MOVQ_XMM_to_XMM(EEREC_D, EEREC_D);
			}
		}
		else {
			if( EEREC_D == EEREC_S ) SSE2_PUNPCKHQDQ_XMM_to_XMM(EEREC_D, EEREC_T);
			else if( EEREC_D == EEREC_T ) {
				//TODO
				SSE2_PUNPCKHQDQ_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_D, 0x4e);
			}
			else {
				if( EEREC_S == EEREC_T ) {
					SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_S, 0xee);
				}
				else {
					SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
					SSE2_PUNPCKHQDQ_XMM_to_XMM(EEREC_D, EEREC_T);
				}
			}
		}
	}
	else {
		if( EEREC_D == EEREC_S ) {
			SSE_SHUFPS_XMM_to_XMM(EEREC_D, EEREC_T, 0xee);
		}
		else if( EEREC_D == EEREC_T ) {
			SSE_MOVHLPS_XMM_to_XMM(EEREC_D, EEREC_S);
		}
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE_MOVHLPS_XMM_to_XMM(EEREC_D, EEREC_S);
		}
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP2(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPOR( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE_XMMCACHE_START((_Rs_!=0?XMMINFO_READS:0)|(_Rt_!=0?XMMINFO_READT:0)|XMMINFO_WRITED)

	if( _Rs_ == 0 ) {
		if( _Rt_ == 0 ) {
			SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
		}
		else SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else if( _Rt_ == 0 ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
	}
	else {
		if( EEREC_D == EEREC_S ) {
			SSEX_POR_XMM_to_XMM(EEREC_D, EEREC_T);
		}
		else if( EEREC_D == EEREC_T ) {
			SSEX_POR_XMM_to_XMM(EEREC_D, EEREC_S);
		}
		else {
			if( _Rs_ == 0 ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			else if( _Rt_ == 0 ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
			else {
				SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
				if( EEREC_S != EEREC_T ) {
					SSEX_POR_XMM_to_XMM(EEREC_D, EEREC_S);
				}
			}
		}
	}
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	MMX_ALLOC_TEMP2(
		MOVQMtoR( t0reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( t1reg, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		if ( _Rt_ != 0 )
		{
			PORMtoR ( t0reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			PORMtoR ( t1reg, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		}
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], t0reg );
		MOVQRtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], t1reg );
		SetMMXstate();
		)
}

////////////////////////////////////////////////////
void recPCPYH( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE2_XMMCACHE_START(XMMINFO_READT|XMMINFO_WRITED)
	SSE2_PSHUFLW_XMM_to_XMM(EEREC_D, EEREC_T, 0);
	SSE2_PSHUFHW_XMM_to_XMM(EEREC_D, EEREC_D, 0);
CPU_SSE_XMMCACHE_END

	_flushCachedRegs();
	_deleteEEreg(_Rd_, 0);

	//PUSH32R( EBX );
	MOVZX32M16toR( EAX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	MOV32RtoR( ECX, EAX );
	SHL32ItoR( ECX, 16 );
	OR32RtoR( EAX, ECX );
	MOVZX32M16toR( EDX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	MOV32RtoR( ECX, EDX );
	SHL32ItoR( ECX, 16 );
	OR32RtoR( EDX, ECX );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EAX );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 2 ], EDX );
	MOV32RtoM( (uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 3 ], EDX );
	//POP32R( EBX );
}

#endif

#endif // PCSX2_NORECBUILD
