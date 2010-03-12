/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 * 
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */


/*********************************************************
*   cached MMI opcodes                                   *
*                                                        *
*********************************************************/

#include "PrecompiledHeader.h"

#include "Common.h"
#include "R5900OpcodeTables.h"
#include "iR5900.h"
#include "iMMI.h"

namespace Interp = R5900::Interpreter::OpcodeImpl::MMI;

namespace R5900 {
namespace Dynarec {
namespace OpcodeImpl {
namespace MMI
{

#ifndef MMI_RECOMPILE

REC_FUNC_DEL( PLZCW, _Rd_ );

REC_FUNC_DEL( PMFHL, _Rd_ );
REC_FUNC_DEL( PMTHL, _Rd_ );

REC_FUNC_DEL( PSRLW, _Rd_ );
REC_FUNC_DEL( PSRLH, _Rd_ );

REC_FUNC_DEL( PSRAH, _Rd_ );
REC_FUNC_DEL( PSRAW, _Rd_ );

REC_FUNC_DEL( PSLLH, _Rd_ );
REC_FUNC_DEL( PSLLW, _Rd_ );

#else

void recPLZCW()
{
	//int regd = -1;
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
	else if( (regs = _checkMMXreg(MMX_GPR+_Rs_, MODE_READ)) >= 0 ) {
		MOVD32MMXtoR(EAX, regs);
		SetMMXstate();
		regs |= MEM_MMXTAG;
	}
	else {
		MOV32MtoR(EAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
		regs = 0;
	}

	_deleteEEreg(_Rd_, 0);

	// Count the number of leading bits (MSB) that match the sign bit, excluding the sign
	// bit itself.

	// Strategy: If the sign bit is set, then negate the value.  And that way the same
	// bitcompare can be used for either bit status.  but be warned!  BSR returns undefined
	// results if the EAX is zero, so we need to have special checks for zeros before
	// using it.

	// --- first word ---

	MOV32ItoR(ECX,31);
	TEST32RtoR(EAX, EAX);		// TEST sets the sign flag accordingly.
	u8* label_notSigned = JNS8(0);
	NOT32R(EAX);
	x86SetJ8(label_notSigned);

	BSRRtoR(EAX, EAX);
	u8* label_Zeroed = JZ8(0);	// If BSR sets the ZF, eax is "trash"
	SUB32RtoR(ECX, EAX);
	DEC32R(ECX);			// PS2 doesn't count the first bit

	x86SetJ8(label_Zeroed);
	MOV32RtoM((uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], ECX);

	// second word

	if( regs >= 0 && (regs & MEM_XMMTAG) ) {
		SSE2_PSHUFD_XMM_to_XMM(regs&0xf, regs&0xf, 0x4e);
		SSE2_MOVD_XMM_to_R(EAX, regs&0xf);
		SSE2_PSHUFD_XMM_to_XMM(regs&0xf, regs&0xf, 0x4e);
	}
	else if( regs >= 0 && (regs & MEM_MMXTAG) ) {
		PSHUFWRtoR(regs&0xf, regs&0xf, 0x4e);
		MOVD32MMXtoR(EAX, regs&0xf);
		PSHUFWRtoR(regs&0xf, regs&0xf, 0x4e);
		SetMMXstate();
	}
	else MOV32MtoR(EAX, (uptr)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ]);

	MOV32ItoR(ECX, 31);
	TEST32RtoR(EAX, EAX);		// TEST sets the sign flag accordingly.
	label_notSigned = JNS8(0);
	NOT32R(EAX);
	x86SetJ8(label_notSigned);

	BSRRtoR(EAX, EAX);
	label_Zeroed = JZ8(0);	// If BSR sets the ZF, eax is "trash"
	SUB32RtoR(ECX, EAX);
	DEC32R(ECX);			// PS2 doesn't count the first bit

	x86SetJ8(label_Zeroed);
	MOV32RtoM((uptr)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], ECX);

	GPR_DEL_CONST(_Rd_);
}

void recPMFHL()
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_WRITED|XMMINFO_READLO|XMMINFO_READHI );

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
			CALLFunc( (uptr)R5900::Interpreter::OpcodeImpl::MMI::PMFHL );
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
				SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_LO);
				SSE2_PACKSSDW_XMM_to_XMM(EEREC_D, EEREC_HI);
				
				// shuffle so a1a0b1b0->a1b1a0b0
				SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_D, 0xd8);
			}
			break;
		default:
			Console.Error("PMFHL??  *pcsx2 head esplode!*");
			pxFail("PMFHL??  *pcsx2 head esplode!*");
	}

	_clearNeededXMMregs();
}

void recPMTHL()
{
	if ( _Sa_ != 0 ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READLO|XMMINFO_READHI|XMMINFO_WRITELO|XMMINFO_WRITEHI );

	if ( x86caps.hasStreamingSIMD4Extensions ) {
		SSE4_BLENDPS_XMM_to_XMM(EEREC_LO, EEREC_S, 0x5);
		SSE_SHUFPS_XMM_to_XMM(EEREC_HI, EEREC_S, 0xdd);
		SSE_SHUFPS_XMM_to_XMM(EEREC_HI, EEREC_HI, 0x72);
	}
	else {
		SSE_SHUFPS_XMM_to_XMM(EEREC_LO, EEREC_S, 0x8d);
		SSE_SHUFPS_XMM_to_XMM(EEREC_HI, EEREC_S, 0xdd);
		SSE_SHUFPS_XMM_to_XMM(EEREC_LO, EEREC_LO, 0x72);
		SSE_SHUFPS_XMM_to_XMM(EEREC_HI, EEREC_HI, 0x72);
	}

	_clearNeededXMMregs();
}

// MMX helper routines
/*#define MMX_ALLOC_TEMP1(code) { \
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
} \*/

////////////////////////////////////////////////////
void recPSRLH( void )
{
	if ( !_Rd_ ) return;
	
	int info = eeRecompileCodeXMM( XMMINFO_READT|XMMINFO_WRITED );
	if( (_Sa_&0xf) == 0 ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSRLW_I8_to_XMM(EEREC_D,_Sa_&0xf );
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPSRLW( void )
{
	if( !_Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READT|XMMINFO_WRITED );
	if( _Sa_ == 0 ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSRLD_I8_to_XMM(EEREC_D,_Sa_ );
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPSRAH( void )
{
	if ( !_Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READT|XMMINFO_WRITED );
	if( (_Sa_&0xf) == 0 ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSRAW_I8_to_XMM(EEREC_D,_Sa_&0xf );
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPSRAW( void )
{
	if ( !_Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READT|XMMINFO_WRITED );
	if( _Sa_ == 0 ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSRAD_I8_to_XMM(EEREC_D,_Sa_ );
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPSLLH( void )
{
	if ( !_Rd_ ) return;
	
	int info = eeRecompileCodeXMM( XMMINFO_READT|XMMINFO_WRITED );
	if( (_Sa_&0xf) == 0 ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSLLW_I8_to_XMM(EEREC_D,_Sa_&0xf );
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPSLLW( void )
{
	if ( !_Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READT|XMMINFO_WRITED );
	if( _Sa_ == 0 ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSLLD_I8_to_XMM(EEREC_D,_Sa_ );
	}
	_clearNeededXMMregs();
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

#endif

/*********************************************************
*   MMI0 opcodes                                         *
*                                                        *
*********************************************************/
#ifndef MMI0_RECOMPILE

REC_FUNC_DEL( PADDB, _Rd_);
REC_FUNC_DEL( PADDH, _Rd_);
REC_FUNC_DEL( PADDW, _Rd_);
REC_FUNC_DEL( PADDSB, _Rd_);
REC_FUNC_DEL( PADDSH, _Rd_);
REC_FUNC_DEL( PADDSW, _Rd_);
REC_FUNC_DEL( PSUBB, _Rd_);
REC_FUNC_DEL( PSUBH, _Rd_);
REC_FUNC_DEL( PSUBW, _Rd_);
REC_FUNC_DEL( PSUBSB, _Rd_);
REC_FUNC_DEL( PSUBSH, _Rd_);
REC_FUNC_DEL( PSUBSW, _Rd_);

REC_FUNC_DEL( PMAXW, _Rd_);
REC_FUNC_DEL( PMAXH, _Rd_);        

REC_FUNC_DEL( PCGTW, _Rd_);
REC_FUNC_DEL( PCGTH, _Rd_);
REC_FUNC_DEL( PCGTB, _Rd_);

REC_FUNC_DEL( PEXTLW, _Rd_);

REC_FUNC_DEL( PPACW, _Rd_);        
REC_FUNC_DEL( PEXTLH, _Rd_);
REC_FUNC_DEL( PPACH, _Rd_);        
REC_FUNC_DEL( PEXTLB, _Rd_);
REC_FUNC_DEL( PPACB, _Rd_);
REC_FUNC_DEL( PEXT5, _Rd_);
REC_FUNC_DEL( PPAC5, _Rd_);

#else

////////////////////////////////////////////////////
void recPMAXW()
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	if ( x86caps.hasStreamingSIMD4Extensions ) {
		if( EEREC_S == EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		else if( EEREC_D == EEREC_S ) SSE4_PMAXSD_XMM_to_XMM(EEREC_D, EEREC_T);
		else if ( EEREC_D == EEREC_T ) SSE4_PMAXSD_XMM_to_XMM(EEREC_D, EEREC_S);
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE4_PMAXSD_XMM_to_XMM(EEREC_D, EEREC_T);
		}
	}
	else {
		int t0reg;

		if( EEREC_S == EEREC_T ) {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		}
		else {
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
		}
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPPACW()
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( ((_Rs_!=0)?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED );

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

	_clearNeededXMMregs();
}

void recPPACH( void )
{
	if (!_Rd_) return;

	int info = eeRecompileCodeXMM( (_Rs_!=0?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED );
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

		SSE2_PSRLDQ_I8_to_XMM(t0reg, 4);
		SSE2_PSRLDQ_I8_to_XMM(EEREC_D, 4);
		SSE2_PUNPCKLQDQ_XMM_to_XMM(EEREC_D, t0reg);

		_freeXMMreg(t0reg);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPPACB()
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( (_Rs_!=0?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED );
	if( _Rs_ == 0 ) {
		if( _hasFreeXMMreg() ) {
			int t0reg = _allocTempXMMreg(XMMT_INT, -1);
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PSLLW_I8_to_XMM(EEREC_D, 8);
			SSEX_PXOR_XMM_to_XMM(t0reg, t0reg);
			SSE2_PSRLW_I8_to_XMM(EEREC_D, 8);
			SSE2_PACKUSWB_XMM_to_XMM(EEREC_D, t0reg);
			_freeXMMreg(t0reg);
		}
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PSLLW_I8_to_XMM(EEREC_D, 8);
			SSE2_PSRLW_I8_to_XMM(EEREC_D, 8);
			SSE2_PACKUSWB_XMM_to_XMM(EEREC_D, EEREC_D);
			SSE2_PSRLDQ_I8_to_XMM(EEREC_D, 8);
		}
	}
	else {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);

		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSLLW_I8_to_XMM(t0reg, 8);
		SSE2_PSLLW_I8_to_XMM(EEREC_D, 8);
		SSE2_PSRLW_I8_to_XMM(t0reg, 8);
		SSE2_PSRLW_I8_to_XMM(EEREC_D, 8);

		SSE2_PACKUSWB_XMM_to_XMM(EEREC_D, t0reg);
		_freeXMMreg(t0reg);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPEXT5()
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READT|XMMINFO_WRITED );
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	int t1reg = _allocTempXMMreg(XMMT_INT, -1);

	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T); // for bit 5..9
	SSEX_MOVDQA_XMM_to_XMM(t1reg, EEREC_T); // for bit 15

	SSE2_PSLLD_I8_to_XMM(t0reg, 22);
	SSE2_PSRLW_I8_to_XMM(t1reg, 15);
	SSE2_PSRLD_I8_to_XMM(t0reg, 27);
	SSE2_PSLLD_I8_to_XMM(t1reg, 20);
	SSE2_POR_XMM_to_XMM(t0reg, t1reg);

	SSEX_MOVDQA_XMM_to_XMM(t1reg, EEREC_T); // for bit 10..14
	SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T); // for bit 0..4

	SSE2_PSLLD_I8_to_XMM(EEREC_D, 27);
	SSE2_PSLLD_I8_to_XMM(t1reg, 17);
	SSE2_PSRLD_I8_to_XMM(EEREC_D, 27);
	SSE2_PSRLW_I8_to_XMM(t1reg, 11);
	SSE2_POR_XMM_to_XMM(EEREC_D, t1reg);

	SSE2_PSLLW_I8_to_XMM(EEREC_D, 3);
	SSE2_PSLLW_I8_to_XMM(t0reg, 11);
	SSE2_POR_XMM_to_XMM(EEREC_D, t0reg);

	_freeXMMreg(t0reg);
	_freeXMMreg(t1reg);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPPAC5()
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READT|XMMINFO_WRITED );
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	int t1reg = _allocTempXMMreg(XMMT_INT, -1);

	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T); // for bit 10..14
	SSEX_MOVDQA_XMM_to_XMM(t1reg, EEREC_T); // for bit 15

	SSE2_PSLLD_I8_to_XMM(t0reg, 8);
	SSE2_PSRLD_I8_to_XMM(t1reg, 31);
	SSE2_PSRLD_I8_to_XMM(t0reg, 17);
	SSE2_PSLLD_I8_to_XMM(t1reg, 15);
	SSE2_POR_XMM_to_XMM(t0reg, t1reg);

	SSEX_MOVDQA_XMM_to_XMM(t1reg, EEREC_T); // for bit 5..9
	SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T); // for bit 0..4

	SSE2_PSLLD_I8_to_XMM(EEREC_D, 24);
	SSE2_PSRLD_I8_to_XMM(t1reg, 11);
	SSE2_PSRLD_I8_to_XMM(EEREC_D, 27);
	SSE2_PSLLD_I8_to_XMM(t1reg, 5);
	SSE2_POR_XMM_to_XMM(EEREC_D, t1reg);

	SSE2_PCMPEQD_XMM_to_XMM(t1reg, t1reg);
	SSE2_PSRLD_I8_to_XMM(t1reg, 22);
	SSE2_PAND_XMM_to_XMM(EEREC_D, t1reg);
	SSE2_PANDN_XMM_to_XMM(t1reg, t0reg);
	SSE2_POR_XMM_to_XMM(EEREC_D, t1reg);

	_freeXMMreg(t0reg);
	_freeXMMreg(t1reg);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPMAXH( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	if( EEREC_D == EEREC_S ) SSE2_PMAXSW_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PMAXSW_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PMAXSW_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPCGTB( void )
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	if( EEREC_D != EEREC_T ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPGTB_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPGTB_XMM_to_XMM(EEREC_D, t0reg);
		_freeXMMreg(t0reg);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPCGTH( void )
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	if( EEREC_D != EEREC_T ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPGTW_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPGTW_XMM_to_XMM(EEREC_D, t0reg);
		_freeXMMreg(t0reg);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPCGTW( void )
{
	//TODO:optimize RS | RT== 0
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	if( EEREC_D != EEREC_T ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPGTD_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPGTD_XMM_to_XMM(EEREC_D, t0reg);
		_freeXMMreg(t0reg);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPADDSB( void ) 
{
	if ( ! _Rd_ ) return;
	
	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	if( EEREC_D == EEREC_S ) SSE2_PADDSB_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PADDSB_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PADDSB_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPADDSH( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	if( EEREC_D == EEREC_S ) SSE2_PADDSW_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PADDSW_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PADDSW_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
//NOTE: check kh2 movies if changing this
void recPADDSW( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	int t1reg = _allocTempXMMreg(XMMT_INT, -1);
	int t2reg = _allocTempXMMreg(XMMT_INT, -1);

	// The idea is:
	//  s = x + y; (wrap-arounded)
	//  if Sign(x) == Sign(y) && Sign(s) != Sign(x) && Sign(x) == 0 then positive overflow (clamp with 0x7fffffff)
	//  if Sign(x) == Sign(y) && Sign(s) != Sign(x) && Sign(x) == 1 then negative overflow (clamp with 0x80000000)

	// get sign bit
	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
	SSEX_MOVDQA_XMM_to_XMM(t1reg, EEREC_T);
	SSE2_PSRLD_I8_to_XMM(t0reg, 31);
	SSE2_PSRLD_I8_to_XMM(t1reg, 31);

	// normal addition
	if( EEREC_D == EEREC_S ) SSE2_PADDD_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PADDD_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PADDD_XMM_to_XMM(EEREC_D, EEREC_T);
	}

	// overflow check
	// t2reg = 0xffffffff if overflow, else 0
	SSEX_MOVDQA_XMM_to_XMM(t2reg, EEREC_D);
	SSE2_PSRLD_I8_to_XMM(t2reg, 31);
	SSE2_PCMPEQD_XMM_to_XMM(t1reg, t0reg); // Sign(Rs) == Sign(Rt)
	SSE2_PCMPEQD_XMM_to_XMM(t2reg, t0reg); // Sign(Rs) == Sign(Rd)
	SSE2_PANDN_XMM_to_XMM(t2reg, t1reg); // (Sign(Rs) == Sign(Rt)) & ~(Sign(Rs) == Sign(Rd))
	SSE2_PCMPEQD_XMM_to_XMM(t1reg, t1reg);
	SSE2_PSRLD_I8_to_XMM(t1reg, 1); // 0x7fffffff
	SSE2_PADDD_XMM_to_XMM(t1reg, t0reg); // t1reg = (Rs < 0) ? 0x80000000 : 0x7fffffff

	// saturation
	SSE2_PAND_XMM_to_XMM(t1reg, t2reg);
	SSE2_PANDN_XMM_to_XMM(t2reg, EEREC_D);
	SSE2_POR_XMM_to_XMM(t1reg, t2reg);
	SSEX_MOVDQA_XMM_to_XMM(EEREC_D, t1reg);

	_freeXMMreg(t0reg);
	_freeXMMreg(t1reg);
	_freeXMMreg(t2reg);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPSUBSB( void ) 
{
   if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
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
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPSUBSH( void ) 
{
	if ( ! _Rd_ ) return;
   
	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
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
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
//NOTE: check kh2 movies if changing this
void recPSUBSW( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	int t1reg = _allocTempXMMreg(XMMT_INT, -1);
	int t2reg = _allocTempXMMreg(XMMT_INT, -1);

	// The idea is:
	//  s = x - y; (wrap-arounded)
	//  if Sign(x) != Sign(y) && Sign(s) != Sign(x) && Sign(x) == 0 then positive overflow (clamp with 0x7fffffff)
	//  if Sign(x) != Sign(y) && Sign(s) != Sign(x) && Sign(x) == 1 then negative overflow (clamp with 0x80000000)

	// get sign bit
	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
	SSEX_MOVDQA_XMM_to_XMM(t1reg, EEREC_T);
	SSE2_PSRLD_I8_to_XMM(t0reg, 31);
	SSE2_PSRLD_I8_to_XMM(t1reg, 31);

	// normal subtraction
	if( EEREC_D == EEREC_S ) SSE2_PSUBD_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) {
		SSEX_MOVDQA_XMM_to_XMM(t2reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBD_XMM_to_XMM(EEREC_D, t2reg);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBD_XMM_to_XMM(EEREC_D, EEREC_T);
	}

	// overflow check
	// t2reg = 0xffffffff if NOT overflow, else 0
	SSEX_MOVDQA_XMM_to_XMM(t2reg, EEREC_D);
	SSE2_PSRLD_I8_to_XMM(t2reg, 31);
	SSE2_PCMPEQD_XMM_to_XMM(t1reg, t0reg); // Sign(Rs) == Sign(Rt)
	SSE2_PCMPEQD_XMM_to_XMM(t2reg, t0reg); // Sign(Rs) == Sign(Rd)
	SSE2_POR_XMM_to_XMM(t2reg, t1reg); // (Sign(Rs) == Sign(Rt)) | (Sign(Rs) == Sign(Rd))
	SSE2_PCMPEQD_XMM_to_XMM(t1reg, t1reg);
	SSE2_PSRLD_I8_to_XMM(t1reg, 1); // 0x7fffffff
	SSE2_PADDD_XMM_to_XMM(t1reg, t0reg); // t1reg = (Rs < 0) ? 0x80000000 : 0x7fffffff

	// saturation
	SSE2_PAND_XMM_to_XMM(EEREC_D, t2reg);
	SSE2_PANDN_XMM_to_XMM(t2reg, t1reg);
	SSE2_POR_XMM_to_XMM(EEREC_D, t2reg);

	_freeXMMreg(t0reg);
	_freeXMMreg(t1reg);
	_freeXMMreg(t2reg);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPADDB( void )
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	if( EEREC_D == EEREC_S ) SSE2_PADDB_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PADDB_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PADDB_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPADDH( void )
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( (_Rs_!=0?XMMINFO_READS:0)|(_Rt_!=0?XMMINFO_READT:0)|XMMINFO_WRITED );
	if( _Rs_ == 0 ) {
		if( _Rt_ == 0 ) SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
		else SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else if( _Rt_ == 0 ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
	}
	else {
		if( EEREC_D == EEREC_S ) SSE2_PADDW_XMM_to_XMM(EEREC_D, EEREC_T);
		else if( EEREC_D == EEREC_T ) SSE2_PADDW_XMM_to_XMM(EEREC_D, EEREC_S);
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE2_PADDW_XMM_to_XMM(EEREC_D, EEREC_T);
		}
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPADDW( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( (_Rs_!=0?XMMINFO_READS:0)|(_Rt_!=0?XMMINFO_READT:0)|XMMINFO_WRITED );
	if( _Rs_ == 0 ) {
		if( _Rt_ == 0 ) SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
		else SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else if( _Rt_ == 0 ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
	}
	else {
		if( EEREC_D == EEREC_S ) SSE2_PADDD_XMM_to_XMM(EEREC_D, EEREC_T);
		else if( EEREC_D == EEREC_T ) SSE2_PADDD_XMM_to_XMM(EEREC_D, EEREC_S);
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE2_PADDD_XMM_to_XMM(EEREC_D, EEREC_T);
		}
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPSUBB( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
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
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPSUBH( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
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
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPSUBW( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
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
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPEXTLW( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( (_Rs_!=0?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED );
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
	_clearNeededXMMregs();
}

void recPEXTLB( void ) 
{
	if (!_Rd_) return;

	int info = eeRecompileCodeXMM( (_Rs_!=0?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED );
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
	_clearNeededXMMregs();
}

void recPEXTLH( void )
{
	if (!_Rd_) return;

	int info = eeRecompileCodeXMM( (_Rs_!=0?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED );
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
	_clearNeededXMMregs();
}

#endif

/*********************************************************
*   MMI1 opcodes                                         *
*                                                        *
*********************************************************/
#ifndef MMI1_RECOMPILE

REC_FUNC_DEL( PABSW, _Rd_);
REC_FUNC_DEL( PABSH, _Rd_);

REC_FUNC_DEL( PMINW, _Rd_); 
REC_FUNC_DEL( PADSBH, _Rd_);
REC_FUNC_DEL( PMINH, _Rd_);
REC_FUNC_DEL( PCEQB, _Rd_);   
REC_FUNC_DEL( PCEQH, _Rd_);
REC_FUNC_DEL( PCEQW, _Rd_);

REC_FUNC_DEL( PADDUB, _Rd_);
REC_FUNC_DEL( PADDUH, _Rd_);
REC_FUNC_DEL( PADDUW, _Rd_);

REC_FUNC_DEL( PSUBUB, _Rd_);
REC_FUNC_DEL( PSUBUH, _Rd_);
REC_FUNC_DEL( PSUBUW, _Rd_);

REC_FUNC_DEL( PEXTUW, _Rd_);   
REC_FUNC_DEL( PEXTUH, _Rd_);
REC_FUNC_DEL( PEXTUB, _Rd_);
REC_FUNC_DEL( QFSRV, _Rd_); 

#else

////////////////////////////////////////////////////

void recPABSW() //needs clamping
{
	if( !_Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READT|XMMINFO_WRITED );
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	SSE2_PCMPEQD_XMM_to_XMM(t0reg, t0reg);
	SSE2_PSLLD_I8_to_XMM(t0reg, 31);
	SSE2_PCMPEQD_XMM_to_XMM(t0reg, EEREC_T); //0xffffffff if equal to 0x80000000
	if( x86caps.hasSupplementalStreamingSIMD3Extensions ) {
		SSSE3_PABSD_XMM_to_XMM(EEREC_D, EEREC_T); //0x80000000 -> 0x80000000
	}
	else {
		int t1reg = _allocTempXMMreg(XMMT_INT, -1); 
		SSEX_MOVDQA_XMM_to_XMM(t1reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSRAD_I8_to_XMM(t1reg, 31);
		SSEX_PXOR_XMM_to_XMM(EEREC_D, t1reg);
		SSE2_PSUBD_XMM_to_XMM(EEREC_D, t1reg); //0x80000000 -> 0x80000000
		_freeXMMreg(t1reg);
	}
	SSE2_PXOR_XMM_to_XMM(EEREC_D, t0reg); //0x80000000 -> 0x7fffffff
	_freeXMMreg(t0reg);
	_clearNeededXMMregs();
}


////////////////////////////////////////////////////
void recPABSH() 
{
	if( !_Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READT|XMMINFO_WRITED );
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	SSE2_PCMPEQW_XMM_to_XMM(t0reg, t0reg);
	SSE2_PSLLW_I8_to_XMM(t0reg, 15);
	SSE2_PCMPEQW_XMM_to_XMM(t0reg, EEREC_T); //0xffff if equal to 0x8000
	if( x86caps.hasSupplementalStreamingSIMD3Extensions ) {
		SSSE3_PABSW_XMM_to_XMM(EEREC_D, EEREC_T); //0x8000 -> 0x8000
	}
	else {
		int t1reg = _allocTempXMMreg(XMMT_INT, -1);
		SSEX_MOVDQA_XMM_to_XMM(t1reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
		SSE2_PSRAW_I8_to_XMM(t1reg, 15);
		SSEX_PXOR_XMM_to_XMM(EEREC_D, t1reg);
		SSE2_PSUBW_XMM_to_XMM(EEREC_D, t1reg); //0x8000 -> 0x8000
		_freeXMMreg(t1reg);
	}
	SSE2_PXOR_XMM_to_XMM(EEREC_D, t0reg); //0x8000 -> 0x7fff
	_freeXMMreg(t0reg);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPMINW()
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	if ( x86caps.hasStreamingSIMD4Extensions ) {
		if( EEREC_S == EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		else if( EEREC_D == EEREC_S ) SSE4_PMINSD_XMM_to_XMM(EEREC_D, EEREC_T);
		else if ( EEREC_D == EEREC_T ) SSE4_PMINSD_XMM_to_XMM(EEREC_D, EEREC_S);
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE4_PMINSD_XMM_to_XMM(EEREC_D, EEREC_T);
		}
	}
	else {
		int t0reg;

		if( EEREC_S == EEREC_T ) {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		}
		else {
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
		}
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPADSBH()
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	int t0reg;

	if( EEREC_S == EEREC_T ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PADDW_XMM_to_XMM(EEREC_D, EEREC_D);
		// reset lower bits to 0s
		SSE2_PSRLDQ_I8_to_XMM(EEREC_D, 8);
		SSE2_PSLLDQ_I8_to_XMM(EEREC_D, 8);
	}
	else {
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
	}

	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPADDUW()
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( (_Rs_?XMMINFO_READS:0)|(_Rt_?XMMINFO_READT:0)|XMMINFO_WRITED );

	if( _Rt_ == 0 ) {
		if( _Rs_ == 0 ) {
			SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
		}
		else SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
	}
	else if( _Rs_ == 0 ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		int t1reg = _allocTempXMMreg(XMMT_INT, -1);

		SSE2_PCMPEQB_XMM_to_XMM(t0reg, t0reg);
		SSE2_PSLLD_I8_to_XMM(t0reg, 31); // 0x80000000
		SSEX_MOVDQA_XMM_to_XMM(t1reg, t0reg);
		SSE2_PXOR_XMM_to_XMM(t0reg, EEREC_S); // invert MSB of Rs (for unsigned comparison)

		// normal 32-bit addition
		if( EEREC_D == EEREC_S ) SSE2_PADDD_XMM_to_XMM(EEREC_D, EEREC_T);
		else if( EEREC_D == EEREC_T ) SSE2_PADDD_XMM_to_XMM(EEREC_D, EEREC_S);
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE2_PADDD_XMM_to_XMM(EEREC_D, EEREC_T);
		}

		// unsigned 32-bit comparison
		SSE2_PXOR_XMM_to_XMM(t1reg, EEREC_D); // invert MSB of Rd (for unsigned comparison)
		SSE2_PCMPGTD_XMM_to_XMM(t0reg, t1reg);

		// saturate
		SSE2_POR_XMM_to_XMM(EEREC_D, t0reg); // clear word with 0xFFFFFFFF if (Rd < Rs)

		_freeXMMreg(t0reg);
		_freeXMMreg(t1reg);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPSUBUB()
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	if( EEREC_D == EEREC_S ) SSE2_PSUBUSB_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBUSB_XMM_to_XMM(EEREC_D, t0reg);
		_freeXMMreg(t0reg);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBUSB_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPSUBUH()
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	if( EEREC_D == EEREC_S ) SSE2_PSUBUSW_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBUSW_XMM_to_XMM(EEREC_D, t0reg);
		_freeXMMreg(t0reg);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBUSW_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPSUBUW()
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	int t1reg = _allocTempXMMreg(XMMT_INT, -1);

	SSE2_PCMPEQB_XMM_to_XMM(t0reg, t0reg);
	SSE2_PSLLD_I8_to_XMM(t0reg, 31); // 0x80000000

	// normal 32-bit subtraction
	// and invert MSB of Rs and Rt (for unsigned comparison)
	if( EEREC_D == EEREC_S ) {
		SSEX_MOVDQA_XMM_to_XMM(t1reg, t0reg);
		SSE2_PXOR_XMM_to_XMM(t0reg, EEREC_S);
		SSE2_PXOR_XMM_to_XMM(t1reg, EEREC_T);
		SSE2_PSUBD_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	else if( EEREC_D == EEREC_T ) {
		SSEX_MOVDQA_XMM_to_XMM(t1reg, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBD_XMM_to_XMM(EEREC_D, t1reg);
		SSE2_PXOR_XMM_to_XMM(t1reg, t0reg);
		SSE2_PXOR_XMM_to_XMM(t0reg, EEREC_S);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSUBD_XMM_to_XMM(EEREC_D, EEREC_T);
		SSEX_MOVDQA_XMM_to_XMM(t1reg, t0reg);
		SSE2_PXOR_XMM_to_XMM(t0reg, EEREC_S);
		SSE2_PXOR_XMM_to_XMM(t1reg, EEREC_T);
	}

	// unsigned 32-bit comparison
	SSE2_PCMPGTD_XMM_to_XMM(t0reg, t1reg);

	// saturate
	SSE2_PAND_XMM_to_XMM(EEREC_D, t0reg); // clear word with zero if (Rs <= Rt)
	
	_freeXMMreg(t0reg);
	_freeXMMreg(t1reg);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPEXTUH()
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( (_Rs_!=0?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED );
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
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
// Both Macros are 16 bytes so we can use a shift instead of a Mul instruction
#define QFSRVhelper0() {  \
	ajmp[0] = JMP32(0);  \
	x86Ptr += 11;  \
}

#define QFSRVhelper(shift1, shift2) {  \
	SSE2_PSRLDQ_I8_to_XMM(EEREC_D, shift1);  \
	SSE2_PSLLDQ_I8_to_XMM(t0reg, shift2);  \
	ajmp[shift1] = JMP32(0);  \
	x86Ptr += 1;  \
}

void recQFSRV()
{
	if ( !_Rd_ ) return;
	//Console.WriteLn("recQFSRV()");

	int info = eeRecompileCodeXMM( XMMINFO_READS | XMMINFO_READT | XMMINFO_WRITED );

	u32 *ajmp[16];
	int i, j;
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);

	SSE2_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
	SSE2_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);

	MOV32MtoR(EAX, (uptr)&cpuRegs.sa);
	SHL32ItoR(EAX, 4); // Multiply SA bytes by 16 bytes (the amount of bytes in QFSRVhelper() macros)
	AND32ItoR(EAX, 0xf0); // This can possibly be removed but keeping it incase theres garbage in SA (cottonvibes)
	ADD32ItoR(EAX, (uptr)x86Ptr + 7); // ADD32 = 5 bytes, JMPR = 2 bytes
	JMPR(EAX); // Jumps to a QFSRVhelper() case below (a total of 16 different cases)

	// Case 0:
	QFSRVhelper0();

	// Cases 1 to 15:
	for (i = 1, j = 15; i < 16; i++, j--) {
		QFSRVhelper(i, j);
	}

	// Set jump addresses for the JMP32's in QFSRVhelper()
	for (i = 1; i < 16; i++) {
		x86SetJ32(ajmp[i]);
	}

	// Concatenate the regs after appropriate shifts have been made
	SSE2_POR_XMM_to_XMM(EEREC_D, t0reg);
	
	x86SetJ32(ajmp[0]); // Case 0 jumps to here (to skip the POR)
	_freeXMMreg(t0reg);

	_clearNeededXMMregs();
}


void recPEXTUB( void )
{
	if (!_Rd_) return;

	int info = eeRecompileCodeXMM( (_Rs_!=0?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED );

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
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPEXTUW( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( (_Rs_!=0?XMMINFO_READS:0)|XMMINFO_READT|XMMINFO_WRITED );
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
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPMINH( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	if( EEREC_D == EEREC_S ) SSE2_PMINSW_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PMINSW_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PMINSW_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPCEQB( void )
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED ); 
	if( EEREC_D == EEREC_S ) SSE2_PCMPEQB_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PCMPEQB_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPEQB_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPCEQH( void )
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	if( EEREC_D == EEREC_S ) SSE2_PCMPEQW_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PCMPEQW_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPEQW_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPCEQW( void )
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	if( EEREC_D == EEREC_S ) SSE2_PCMPEQD_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PCMPEQD_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PCMPEQD_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPADDUB( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|(_Rt_?XMMINFO_READT:0)|XMMINFO_WRITED );
	if( _Rt_ ) {
		if( EEREC_D == EEREC_S ) SSE2_PADDUSB_XMM_to_XMM(EEREC_D, EEREC_T);
		else if( EEREC_D == EEREC_T ) SSE2_PADDUSB_XMM_to_XMM(EEREC_D, EEREC_S);
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE2_PADDUSB_XMM_to_XMM(EEREC_D, EEREC_T);
		}
	}
	else SSE2_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPADDUH( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
	if( EEREC_D == EEREC_S ) SSE2_PADDUSW_XMM_to_XMM(EEREC_D, EEREC_T);
	else if( EEREC_D == EEREC_T ) SSE2_PADDUSW_XMM_to_XMM(EEREC_D, EEREC_S);
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PADDUSW_XMM_to_XMM(EEREC_D, EEREC_T);
	}
	_clearNeededXMMregs();
}

#endif
/*********************************************************
*   MMI2 opcodes                                         *
*                                                        *
*********************************************************/
#ifndef MMI2_RECOMPILE

REC_FUNC_DEL( PMFHI, _Rd_);
REC_FUNC_DEL( PMFLO, _Rd_);
REC_FUNC_DEL( PCPYLD, _Rd_);
REC_FUNC_DEL( PAND, _Rd_);
REC_FUNC_DEL( PXOR, _Rd_); 

REC_FUNC_DEL( PMADDW, _Rd_);
REC_FUNC_DEL( PSLLVW, _Rd_);
REC_FUNC_DEL( PSRLVW, _Rd_); 
REC_FUNC_DEL( PMSUBW, _Rd_);
REC_FUNC_DEL( PINTH, _Rd_);
REC_FUNC_DEL( PMULTW, _Rd_);
REC_FUNC_DEL( PDIVW, _Rd_);
REC_FUNC_DEL( PMADDH, _Rd_);
REC_FUNC_DEL( PHMADH, _Rd_);
REC_FUNC_DEL( PMSUBH, _Rd_);
REC_FUNC_DEL( PHMSBH, _Rd_);
REC_FUNC_DEL( PEXEH, _Rd_);
REC_FUNC_DEL( PREVH, _Rd_); 
REC_FUNC_DEL( PMULTH, _Rd_);
REC_FUNC_DEL( PDIVBW, _Rd_);
REC_FUNC_DEL( PEXEW, _Rd_);
REC_FUNC_DEL( PROT3W, _Rd_ ); 

#else

////////////////////////////////////////////////////
void recPMADDW()
{
	if( !x86caps.hasStreamingSIMD4Extensions ) {
		recCall( Interp::PMADDW, _Rd_ );
		return;
	}

	int info = eeRecompileCodeXMM( (((_Rs_)&&(_Rt_))?XMMINFO_READS:0)|(((_Rs_)&&(_Rt_))?XMMINFO_READT:0)|(_Rd_?XMMINFO_WRITED:0)|XMMINFO_WRITELO|XMMINFO_WRITEHI|XMMINFO_READLO|XMMINFO_READHI );
	SSE_SHUFPS_XMM_to_XMM(EEREC_LO, EEREC_HI, 0x88);
	SSE2_PSHUFD_XMM_to_XMM(EEREC_LO, EEREC_LO, 0xd8); // LO = {LO[0], HI[0], LO[2], HI[2]}
	if( _Rd_ ) {
		if( !_Rs_ || !_Rt_ ) SSE2_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
		else if( EEREC_D == EEREC_S ) SSE4_PMULDQ_XMM_to_XMM(EEREC_D, EEREC_T);
		else if( EEREC_D == EEREC_T ) SSE4_PMULDQ_XMM_to_XMM(EEREC_D, EEREC_S);
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE4_PMULDQ_XMM_to_XMM(EEREC_D, EEREC_T);
		}
	}
	else {
		if( !_Rs_ || !_Rt_ ) SSE2_PXOR_XMM_to_XMM(EEREC_HI, EEREC_HI);
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_S);
			SSE4_PMULDQ_XMM_to_XMM(EEREC_HI, EEREC_T);
		}
	}

	// add from LO/HI
	if ( _Rd_ ) SSE2_PADDQ_XMM_to_XMM(EEREC_D, EEREC_LO);
	else SSE2_PADDQ_XMM_to_XMM(EEREC_HI, EEREC_LO);

	// interleave & sign extend
	if ( _Rd_ ) {
		SSE2_PSHUFD_XMM_to_XMM(EEREC_LO, EEREC_D, 0x88);
		SSE2_PSHUFD_XMM_to_XMM(EEREC_HI, EEREC_D, 0xdd);
	}
	else {
		SSE2_PSHUFD_XMM_to_XMM(EEREC_LO, EEREC_HI, 0x88);
		SSE2_PSHUFD_XMM_to_XMM(EEREC_HI, EEREC_HI, 0xdd);
	}
	SSE4_PMOVSXDQ_XMM_to_XMM(EEREC_LO, EEREC_LO);
	SSE4_PMOVSXDQ_XMM_to_XMM(EEREC_HI, EEREC_HI);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPSLLVW()
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( (_Rs_?XMMINFO_READS:0)|(_Rt_?XMMINFO_READT:0)|XMMINFO_WRITED );
	if( _Rs_ == 0 ) {
		if( _Rt_ == 0 ) {
			SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
		}
		else {
			if ( x86caps.hasStreamingSIMD4Extensions ) {
				SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_T, 0x88);
				SSE4_PMOVSXDQ_XMM_to_XMM(EEREC_D, EEREC_D);
			}
			else {
				int t0reg = _allocTempXMMreg(XMMT_INT, -1);
				SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_T, 0x88);
				SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_D);
				SSE2_PSRAD_I8_to_XMM(t0reg, 31);
				SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t0reg);
				_freeXMMreg(t0reg);
			}
		}
	}
	else if( _Rt_ == 0 ) {
		SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
	}
	else {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		int t1reg = _allocTempXMMreg(XMMT_INT, -1);

		// shamt is 5-bit
		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
		SSE2_PSLLQ_I8_to_XMM(t0reg, 27+32);
		SSE2_PSRLQ_I8_to_XMM(t0reg, 27+32);

		// EEREC_D[0] <- Rt[0], t1reg[0] <- Rt[2]
		SSE_MOVHLPS_XMM_to_XMM(t1reg, EEREC_T);
		if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);

		// shift (left) Rt[0]
		SSE2_PSLLD_XMM_to_XMM(EEREC_D, t0reg);

		// shift (left) Rt[2]
		SSE_MOVHLPS_XMM_to_XMM(t0reg, t0reg);
		SSE2_PSLLD_XMM_to_XMM(t1reg, t0reg);

		// merge & sign extend
		if ( x86caps.hasStreamingSIMD4Extensions ) {
			SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t1reg);
			SSE4_PMOVSXDQ_XMM_to_XMM(EEREC_D, EEREC_D);
		}
		else {
			SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t1reg);
			SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_D);
			SSE2_PSRAD_I8_to_XMM(t0reg, 31); // get the signs
			SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t0reg);
		}

		_freeXMMreg(t0reg);
		_freeXMMreg(t1reg);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPSRLVW()
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( (_Rs_?XMMINFO_READS:0)|(_Rt_?XMMINFO_READT:0)|XMMINFO_WRITED );
	if( _Rs_ == 0 ) {
		if( _Rt_ == 0 ) {
			SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
		}
		else {
			if ( x86caps.hasStreamingSIMD4Extensions ) {
				SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_T, 0x88);
				SSE4_PMOVSXDQ_XMM_to_XMM(EEREC_D, EEREC_D);
			}
			else {
				int t0reg = _allocTempXMMreg(XMMT_INT, -1);
				SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_T, 0x88);
				SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_D);
				SSE2_PSRAD_I8_to_XMM(t0reg, 31);
				SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t0reg);
				_freeXMMreg(t0reg);
			}
		}
	}
	else if( _Rt_ == 0 ) {
		SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
	}
	else {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		int t1reg = _allocTempXMMreg(XMMT_INT, -1);

		// shamt is 5-bit
		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
		SSE2_PSLLQ_I8_to_XMM(t0reg, 27+32);
		SSE2_PSRLQ_I8_to_XMM(t0reg, 27+32);

		// EEREC_D[0] <- Rt[0], t1reg[0] <- Rt[2]
		SSE_MOVHLPS_XMM_to_XMM(t1reg, EEREC_T);
		if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);

		// shift (right logical) Rt[0]
		SSE2_PSRLD_XMM_to_XMM(EEREC_D, t0reg);

		// shift (right logical) Rt[2]
		SSE_MOVHLPS_XMM_to_XMM(t0reg, t0reg);
		SSE2_PSRLD_XMM_to_XMM(t1reg, t0reg);

		// merge & sign extend
		if ( x86caps.hasStreamingSIMD4Extensions ) {
			SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t1reg);
			SSE4_PMOVSXDQ_XMM_to_XMM(EEREC_D, EEREC_D);
		}
		else {
			SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t1reg);
			SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_D);
			SSE2_PSRAD_I8_to_XMM(t0reg, 31); // get the signs
			SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t0reg);
		}

		_freeXMMreg(t0reg);
		_freeXMMreg(t1reg);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPMSUBW()
{
	if( !x86caps.hasStreamingSIMD4Extensions ) {
		recCall( Interp::PMSUBW, _Rd_ );
		return;
	}
	int info = eeRecompileCodeXMM( (((_Rs_)&&(_Rt_))?XMMINFO_READS:0)|(((_Rs_)&&(_Rt_))?XMMINFO_READT:0)|(_Rd_?XMMINFO_WRITED:0)|XMMINFO_WRITELO|XMMINFO_WRITEHI|XMMINFO_READLO|XMMINFO_READHI );
	SSE_SHUFPS_XMM_to_XMM(EEREC_LO, EEREC_HI, 0x88);
	SSE2_PSHUFD_XMM_to_XMM(EEREC_LO, EEREC_LO, 0xd8); // LO = {LO[0], HI[0], LO[2], HI[2]}
	if( _Rd_ ) {
		if( !_Rs_ || !_Rt_ ) SSE2_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
		else if( EEREC_D == EEREC_S ) SSE4_PMULDQ_XMM_to_XMM(EEREC_D, EEREC_T);
		else if( EEREC_D == EEREC_T ) SSE4_PMULDQ_XMM_to_XMM(EEREC_D, EEREC_S);
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE4_PMULDQ_XMM_to_XMM(EEREC_D, EEREC_T);
		}
	}
	else {
		if( !_Rs_ || !_Rt_ ) SSE2_PXOR_XMM_to_XMM(EEREC_HI, EEREC_HI);
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_S);
			SSE4_PMULDQ_XMM_to_XMM(EEREC_HI, EEREC_T);
		}
	}

	// sub from LO/HI
	if ( _Rd_ ) {
		SSE2_PSUBQ_XMM_to_XMM(EEREC_LO, EEREC_D);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_LO);
	}
	else {
		SSE2_PSUBQ_XMM_to_XMM(EEREC_LO, EEREC_HI);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_LO);
	}

	// interleave & sign extend
	if ( _Rd_ ) {
		SSE2_PSHUFD_XMM_to_XMM(EEREC_LO, EEREC_D, 0x88);
		SSE2_PSHUFD_XMM_to_XMM(EEREC_HI, EEREC_D, 0xdd);
	}
	else {
		SSE2_PSHUFD_XMM_to_XMM(EEREC_LO, EEREC_HI, 0x88);
		SSE2_PSHUFD_XMM_to_XMM(EEREC_HI, EEREC_HI, 0xdd);
	}
	SSE4_PMOVSXDQ_XMM_to_XMM(EEREC_LO, EEREC_LO);
	SSE4_PMOVSXDQ_XMM_to_XMM(EEREC_HI, EEREC_HI);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPMULTW()
{
	if( !x86caps.hasStreamingSIMD4Extensions ) {
		recCall( Interp::PMULTW, _Rd_ );
		return;
	}
	int info = eeRecompileCodeXMM( (((_Rs_)&&(_Rt_))?XMMINFO_READS:0)|(((_Rs_)&&(_Rt_))?XMMINFO_READT:0)|(_Rd_?XMMINFO_WRITED:0)|XMMINFO_WRITELO|XMMINFO_WRITEHI );
	if( !_Rs_ || !_Rt_ ) {
		if( _Rd_ ) SSE2_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
		SSE2_PXOR_XMM_to_XMM(EEREC_LO, EEREC_LO);
		SSE2_PXOR_XMM_to_XMM(EEREC_HI, EEREC_HI);
	}
	else {
		if( _Rd_ ) {
			if( EEREC_D == EEREC_S ) SSE4_PMULDQ_XMM_to_XMM(EEREC_D, EEREC_T);
			else if( EEREC_D == EEREC_T ) SSE4_PMULDQ_XMM_to_XMM(EEREC_D, EEREC_S);
			else {
				SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE4_PMULDQ_XMM_to_XMM(EEREC_D, EEREC_T);
			}
		}
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_S);
			SSE4_PMULDQ_XMM_to_XMM(EEREC_HI, EEREC_T);
		}

		// interleave & sign extend
		if ( _Rd_ ) {
			SSE2_PSHUFD_XMM_to_XMM(EEREC_LO, EEREC_D, 0x88);
			SSE2_PSHUFD_XMM_to_XMM(EEREC_HI, EEREC_D, 0xdd);
		}
		else {
			SSE2_PSHUFD_XMM_to_XMM(EEREC_LO, EEREC_HI, 0x88);
			SSE2_PSHUFD_XMM_to_XMM(EEREC_HI, EEREC_HI, 0xdd);
		}
		SSE4_PMOVSXDQ_XMM_to_XMM(EEREC_LO, EEREC_LO);
		SSE4_PMOVSXDQ_XMM_to_XMM(EEREC_HI, EEREC_HI);
	}
	_clearNeededXMMregs();
}
////////////////////////////////////////////////////
void recPDIVW()
{
	recCall( Interp::PDIVW, _Rd_ );
}

////////////////////////////////////////////////////
void recPDIVBW()
{
	recCall( Interp::PDIVBW, _Rd_ ); //--
}

////////////////////////////////////////////////////

//upper word of each doubleword in LO and HI is undocumented/undefined
//contains the upper multiplication result (before the addition with the lower multiplication result)
void recPHMADH()
{
	int info = eeRecompileCodeXMM( (_Rd_?XMMINFO_WRITED:0)|XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITELO|XMMINFO_WRITEHI );
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);

	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
	SSE2_PSRLD_I8_to_XMM(t0reg, 16);
	SSE2_PSLLD_I8_to_XMM(t0reg, 16);
	SSE2_PMADDWD_XMM_to_XMM(t0reg, EEREC_T);

	if( _Rd_ ) {
		if( EEREC_D == EEREC_S ) {
			SSE2_PMADDWD_XMM_to_XMM(EEREC_D, EEREC_T);
		}
		else if( EEREC_D == EEREC_T ) {
			SSE2_PMADDWD_XMM_to_XMM(EEREC_D, EEREC_S);
		}
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PMADDWD_XMM_to_XMM(EEREC_D, EEREC_S);
		}
		SSEX_MOVDQA_XMM_to_XMM(EEREC_LO, EEREC_D);
	}
	else {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_LO, EEREC_T);
		SSE2_PMADDWD_XMM_to_XMM(EEREC_LO, EEREC_S);
	}
	
	SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_LO);

	SSE_SHUFPS_XMM_to_XMM(EEREC_LO, t0reg, 0x88);
	SSE_SHUFPS_XMM_to_XMM(EEREC_LO, EEREC_LO, 0xd8);

	SSE_SHUFPS_XMM_to_XMM(EEREC_HI, t0reg, 0xdd);
	SSE_SHUFPS_XMM_to_XMM(EEREC_HI, EEREC_HI, 0xd8);

	_freeXMMreg(t0reg);
	_clearNeededXMMregs();
}

void recPMSUBH()
{
	int info = eeRecompileCodeXMM( (_Rd_?XMMINFO_WRITED:0)|XMMINFO_READS|XMMINFO_READT|XMMINFO_READLO|XMMINFO_READHI|XMMINFO_WRITELO|XMMINFO_WRITEHI );
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	int t1reg = _allocTempXMMreg(XMMT_INT, -1);
 
	if( !_Rd_ ) {
		SSE2_PXOR_XMM_to_XMM(t0reg, t0reg);
		SSE2_PSHUFD_XMM_to_XMM(t1reg, EEREC_S, 0xd8); //S0, S1, S4, S5, S2, S3, S6, S7
		SSE2_PUNPCKLWD_XMM_to_XMM(t1reg, t0reg); //S0, 0, S1, 0, S4, 0, S5, 0
		SSE2_PSHUFD_XMM_to_XMM(t0reg, EEREC_T, 0xd8); //T0, T1, T4, T5, T2, T3, T6, T7
		SSE2_PUNPCKLWD_XMM_to_XMM(t0reg, t0reg); //T0, T0, T1, T1, T4, T4, T5, T5
		SSE2_PMADDWD_XMM_to_XMM(t0reg, t1reg); //S0*T0+0*T0, S1*T1+0*T1, S4*T4+0*T4, S5*T5+0*T5
 
		SSE2_PSUBD_XMM_to_XMM(EEREC_LO, t0reg);
 
		SSE2_PXOR_XMM_to_XMM(t0reg, t0reg);
		SSE2_PSHUFD_XMM_to_XMM(t1reg, EEREC_S, 0xd8); //S0, S1, S4, S5, S2, S3, S6, S7
		SSE2_PUNPCKHWD_XMM_to_XMM(t1reg, t0reg); //S2, 0, S3, 0, S6, 0, S7, 0
		SSE2_PSHUFD_XMM_to_XMM(t0reg, EEREC_T, 0xd8); //T0, T1, T4, T5, T2, T3, T6, T7
		SSE2_PUNPCKHWD_XMM_to_XMM(t0reg, t0reg); //T2, T2, T3, T3, T6, T6, T7, T7
		SSE2_PMADDWD_XMM_to_XMM(t0reg, t1reg); //S2*T2+0*T2, S3*T3+0*T3, S6*T6+0*T6, S7*T7+0*T7
 
		SSE2_PSUBD_XMM_to_XMM(EEREC_HI, t0reg);
	}
	else {
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
 
		// 0,2,4,6, L->H
		SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_LO, 0x88);
		SSE2_PSHUFD_XMM_to_XMM(t0reg, EEREC_HI, 0x88);
		SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t0reg);
	}
 
	_freeXMMreg(t0reg);
	_freeXMMreg(t1reg);
 
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
//upper word of each doubleword in LO and HI is undocumented/undefined
//it contains the NOT of the upper multiplication result (before the substraction of the lower multiplication result)
void recPHMSBH()
{
	int info = eeRecompileCodeXMM( (_Rd_?XMMINFO_WRITED:0)|XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITELO|XMMINFO_WRITEHI );
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);

	SSE2_PCMPEQD_XMM_to_XMM(EEREC_LO, EEREC_LO);
	SSE2_PSRLD_I8_to_XMM(EEREC_LO, 16);
	SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_S);
	SSE2_PAND_XMM_to_XMM(EEREC_HI, EEREC_LO);
	SSE2_PMADDWD_XMM_to_XMM(EEREC_HI, EEREC_T);
	SSE2_PSLLD_I8_to_XMM(EEREC_LO, 16);
	SSE2_PAND_XMM_to_XMM(EEREC_LO, EEREC_S);
	SSE2_PMADDWD_XMM_to_XMM(EEREC_LO, EEREC_T);
	SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_LO);
	SSE2_PSUBD_XMM_to_XMM(EEREC_LO, EEREC_HI);
	if( _Rd_ ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_LO);

	SSE2_PCMPEQD_XMM_to_XMM(EEREC_HI, EEREC_HI);
	SSE2_PXOR_XMM_to_XMM(t0reg, EEREC_HI); 

	SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_LO);
	
	SSE_SHUFPS_XMM_to_XMM(EEREC_LO, t0reg, 0x88);
	SSE_SHUFPS_XMM_to_XMM(EEREC_LO, EEREC_LO, 0xd8);

	SSE_SHUFPS_XMM_to_XMM(EEREC_HI, t0reg, 0xdd);
	SSE_SHUFPS_XMM_to_XMM(EEREC_HI, EEREC_HI, 0xd8);

	_freeXMMreg(t0reg);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPEXEH( void )
{
	if (!_Rd_) return;

	int info = eeRecompileCodeXMM( XMMINFO_READT|XMMINFO_WRITED );
	SSE2_PSHUFLW_XMM_to_XMM(EEREC_D, EEREC_T, 0xc6);
	SSE2_PSHUFHW_XMM_to_XMM(EEREC_D, EEREC_D, 0xc6);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPREVH( void )
{
	if (!_Rd_) return;

	
	int info = eeRecompileCodeXMM( XMMINFO_READT|XMMINFO_WRITED );
	SSE2_PSHUFLW_XMM_to_XMM(EEREC_D, EEREC_T, 0x1B);
	SSE2_PSHUFHW_XMM_to_XMM(EEREC_D, EEREC_D, 0x1B);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPINTH( void )
{
	if (!_Rd_) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED );
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
	_clearNeededXMMregs();
}

void recPEXEW( void )
{
	if (!_Rd_) return;

	int info = eeRecompileCodeXMM( XMMINFO_READT|XMMINFO_WRITED );
	SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_T, 0xc6);
	_clearNeededXMMregs();
}

void recPROT3W( void )
{
	if (!_Rd_) return;

	int info = eeRecompileCodeXMM( XMMINFO_READT|XMMINFO_WRITED );
	SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_T, 0xc9);
	_clearNeededXMMregs();
}

void recPMULTH( void )
{
	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_READT|(_Rd_?XMMINFO_WRITED:0)|XMMINFO_WRITELO|XMMINFO_WRITEHI );
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
	_clearNeededXMMregs();
}

void recPMFHI( void )
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_WRITED|XMMINFO_READHI );
	SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_HI);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPMFLO( void )
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_WRITED|XMMINFO_READLO );
	SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_LO);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPAND( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT );
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
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPXOR( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT );
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
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPCPYLD( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_WRITED|(( _Rs_== 0) ? 0:XMMINFO_READS)|XMMINFO_READT );
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
	_clearNeededXMMregs();
}

void recPMADDH( void ) 
{
	int info = eeRecompileCodeXMM( (_Rd_?XMMINFO_WRITED:0)|XMMINFO_READS|XMMINFO_READT|XMMINFO_READLO|XMMINFO_READHI|XMMINFO_WRITELO|XMMINFO_WRITEHI );
	int t0reg = _allocTempXMMreg(XMMT_INT, -1);
	int t1reg = _allocTempXMMreg(XMMT_INT, -1);
 
	if( !_Rd_ ) {
		SSE2_PXOR_XMM_to_XMM(t0reg, t0reg);
		SSE2_PSHUFD_XMM_to_XMM(t1reg, EEREC_S, 0xd8); //S0, S1, S4, S5, S2, S3, S6, S7
		SSE2_PUNPCKLWD_XMM_to_XMM(t1reg, t0reg); //S0, 0, S1, 0, S4, 0, S5, 0
		SSE2_PSHUFD_XMM_to_XMM(t0reg, EEREC_T, 0xd8); //T0, T1, T4, T5, T2, T3, T6, T7
		SSE2_PUNPCKLWD_XMM_to_XMM(t0reg, t0reg); //T0, T0, T1, T1, T4, T4, T5, T5
		SSE2_PMADDWD_XMM_to_XMM(t0reg, t1reg); //S0*T0+0*T0, S1*T1+0*T1, S4*T4+0*T4, S5*T5+0*T5
 
		SSE2_PADDD_XMM_to_XMM(EEREC_LO, t0reg);
 
		SSE2_PXOR_XMM_to_XMM(t0reg, t0reg);
		SSE2_PSHUFD_XMM_to_XMM(t1reg, EEREC_S, 0xd8); //S0, S1, S4, S5, S2, S3, S6, S7
		SSE2_PUNPCKHWD_XMM_to_XMM(t1reg, t0reg); //S2, 0, S3, 0, S6, 0, S7, 0
		SSE2_PSHUFD_XMM_to_XMM(t0reg, EEREC_T, 0xd8); //T0, T1, T4, T5, T2, T3, T6, T7
		SSE2_PUNPCKHWD_XMM_to_XMM(t0reg, t0reg); //T2, T2, T3, T3, T6, T6, T7, T7
		SSE2_PMADDWD_XMM_to_XMM(t0reg, t1reg); //S2*T2+0*T2, S3*T3+0*T3, S6*T6+0*T6, S7*T7+0*T7
 
		SSE2_PADDD_XMM_to_XMM(EEREC_HI, t0reg);
	}
	else {
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
 
		// 0,2,4,6, L->H
		SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_LO, 0x88);
		SSE2_PSHUFD_XMM_to_XMM(t0reg, EEREC_HI, 0x88);
		SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t0reg);
	}
 
	_freeXMMreg(t0reg);
	_freeXMMreg(t1reg);
 
	_clearNeededXMMregs();
}

#endif
/*********************************************************
*   MMI3 opcodes                                         *
*                                                        *
*********************************************************/
#ifndef MMI3_RECOMPILE

REC_FUNC_DEL( PMADDUW, _Rd_);
REC_FUNC_DEL( PSRAVW, _Rd_); 
REC_FUNC_DEL( PMTHI, _Rd_);
REC_FUNC_DEL( PMTLO, _Rd_);
REC_FUNC_DEL( PINTEH, _Rd_);
REC_FUNC_DEL( PMULTUW, _Rd_);
REC_FUNC_DEL( PDIVUW, _Rd_);
REC_FUNC_DEL( PCPYUD, _Rd_);
REC_FUNC_DEL( POR, _Rd_);
REC_FUNC_DEL( PNOR, _Rd_);  
REC_FUNC_DEL( PCPYH, _Rd_); 
REC_FUNC_DEL( PEXCW, _Rd_);
REC_FUNC_DEL( PEXCH, _Rd_);

#else

////////////////////////////////////////////////////
//REC_FUNC( PSRAVW, _Rd_ ); 

void recPSRAVW()
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( (_Rs_?XMMINFO_READS:0)|(_Rt_?XMMINFO_READT:0)|XMMINFO_WRITED );
	if( _Rs_ == 0 ) {
		if( _Rt_ == 0 ) {
			SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
		}
		else {
			if ( x86caps.hasStreamingSIMD4Extensions ) {
				SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_T, 0x88);
				SSE4_PMOVSXDQ_XMM_to_XMM(EEREC_D, EEREC_D);
			}
			else {
				int t0reg = _allocTempXMMreg(XMMT_INT, -1);
				SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_T, 0x88);
				SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_D);
				SSE2_PSRAD_I8_to_XMM(t0reg, 31);
				SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t0reg);
				_freeXMMreg(t0reg);
			}
		}
	}
	else if( _Rt_ == 0 ) {
		SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
	}
	else {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		int t1reg = _allocTempXMMreg(XMMT_INT, -1);

		// shamt is 5-bit
		SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
		SSE2_PSLLQ_I8_to_XMM(t0reg, 27+32);
		SSE2_PSRLQ_I8_to_XMM(t0reg, 27+32);

		// EEREC_D[0] <- Rt[0], t1reg[0] <- Rt[2]
		SSE_MOVHLPS_XMM_to_XMM(t1reg, EEREC_T);
		if( EEREC_D != EEREC_T ) SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);

		// shift (right arithmetic) Rt[0]
		SSE2_PSRAD_XMM_to_XMM(EEREC_D, t0reg);

		// shift (right arithmetic) Rt[2]
		SSE_MOVHLPS_XMM_to_XMM(t0reg, t0reg);
		SSE2_PSRAD_XMM_to_XMM(t1reg, t0reg);

		// merge & sign extend
		if ( x86caps.hasStreamingSIMD4Extensions ) {
			SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t1reg);
			SSE4_PMOVSXDQ_XMM_to_XMM(EEREC_D, EEREC_D);
		}
		else {
			SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t1reg);
			SSEX_MOVDQA_XMM_to_XMM(t0reg, EEREC_D);
			SSE2_PSRAD_I8_to_XMM(t0reg, 31); // get the signs
			SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_D, t0reg);
		}

		_freeXMMreg(t0reg);
		_freeXMMreg(t1reg);
	}

	_clearNeededXMMregs();
}


////////////////////////////////////////////////////
static const __aligned16 u32 s_tempPINTEH[4] = {0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff };

void recPINTEH()
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( (_Rs_?XMMINFO_READS:0)|(_Rt_?XMMINFO_READT:0)|XMMINFO_WRITED );

	int t0reg = -1;

	if( _Rs_ == 0 ) {
		if( _Rt_ == 0 ) {
			SSEX_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
		}
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_T);
			SSE2_PAND_M128_to_XMM(EEREC_D, (uptr)s_tempPINTEH);
		}
	}
	else if( _Rt_ == 0 ) {
		SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
		SSE2_PSLLD_I8_to_XMM(EEREC_D, 16);
	}
	else {
		if( EEREC_S == EEREC_T ) {
			SSE2_PSHUFLW_XMM_to_XMM(EEREC_D, EEREC_S, 0xa0);
			SSE2_PSHUFHW_XMM_to_XMM(EEREC_D, EEREC_D, 0xa0);
		}
		else if( EEREC_D == EEREC_T ) {
			pxAssert( EEREC_D != EEREC_S );
			t0reg = _allocTempXMMreg(XMMT_INT, -1);
			SSE2_PSLLD_I8_to_XMM(EEREC_D, 16);
			SSE2_MOVDQA_XMM_to_XMM(t0reg, EEREC_S);
			SSE2_PSRLD_I8_to_XMM(EEREC_D, 16);
			SSE2_PSLLD_I8_to_XMM(t0reg, 16);
			SSE2_POR_XMM_to_XMM(EEREC_D, t0reg);
		}
		else {
			t0reg = _allocTempXMMreg(XMMT_INT, -1);
			SSE2_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE2_MOVDQA_XMM_to_XMM(t0reg, EEREC_T);
			SSE2_PSLLD_I8_to_XMM(t0reg, 16);
			SSE2_PSLLD_I8_to_XMM(EEREC_D, 16);
			SSE2_PSRLD_I8_to_XMM(t0reg, 16);
			SSE2_POR_XMM_to_XMM(EEREC_D, t0reg);
		}
	}

	if( t0reg >= 0 ) _freeXMMreg(t0reg);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPMULTUW()
{
	int info = eeRecompileCodeXMM( (((_Rs_)&&(_Rt_))?XMMINFO_READS:0)|(((_Rs_)&&(_Rt_))?XMMINFO_READT:0)|(_Rd_?XMMINFO_WRITED:0)|XMMINFO_WRITELO|XMMINFO_WRITEHI );
	if( !_Rs_ || !_Rt_ ) {
		if( _Rd_ ) SSE2_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
		SSE2_PXOR_XMM_to_XMM(EEREC_LO, EEREC_LO);
		SSE2_PXOR_XMM_to_XMM(EEREC_HI, EEREC_HI);
	}
	else {
		if( _Rd_ ) {
			if( EEREC_D == EEREC_S ) SSE2_PMULUDQ_XMM_to_XMM(EEREC_D, EEREC_T);
			else if( EEREC_D == EEREC_T ) SSE2_PMULUDQ_XMM_to_XMM(EEREC_D, EEREC_S);
			else {
				SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE2_PMULUDQ_XMM_to_XMM(EEREC_D, EEREC_T);
			}
			SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_D);
		}
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_S);
			SSE2_PMULUDQ_XMM_to_XMM(EEREC_HI, EEREC_T);
		}

		// interleave & sign extend
		if ( x86caps.hasStreamingSIMD4Extensions ) {
			SSE2_PSHUFD_XMM_to_XMM(EEREC_LO, EEREC_HI, 0x88);
			SSE2_PSHUFD_XMM_to_XMM(EEREC_HI, EEREC_HI, 0xdd);
			SSE4_PMOVSXDQ_XMM_to_XMM(EEREC_LO, EEREC_LO);
			SSE4_PMOVSXDQ_XMM_to_XMM(EEREC_HI, EEREC_HI);
		}
		else {
			int t0reg = _allocTempXMMreg(XMMT_INT, -1);
			SSE2_PSHUFD_XMM_to_XMM(t0reg, EEREC_HI, 0xd8);
			SSEX_MOVDQA_XMM_to_XMM(EEREC_LO, t0reg);
			SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, t0reg);
			SSE2_PSRAD_I8_to_XMM(t0reg, 31); // get the signs

			SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_LO, t0reg);
			SSE2_PUNPCKHDQ_XMM_to_XMM(EEREC_HI, t0reg);
			_freeXMMreg(t0reg);
		}
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPMADDUW()
{
	int info = eeRecompileCodeXMM( (((_Rs_)&&(_Rt_))?XMMINFO_READS:0)|(((_Rs_)&&(_Rt_))?XMMINFO_READT:0)|(_Rd_?XMMINFO_WRITED:0)|XMMINFO_WRITELO|XMMINFO_WRITEHI|XMMINFO_READLO|XMMINFO_READHI );
	SSE_SHUFPS_XMM_to_XMM(EEREC_LO, EEREC_HI, 0x88);
	SSE2_PSHUFD_XMM_to_XMM(EEREC_LO, EEREC_LO, 0xd8); // LO = {LO[0], HI[0], LO[2], HI[2]}
	if( _Rd_ ) {
		if( !_Rs_ || !_Rt_ ) SSE2_PXOR_XMM_to_XMM(EEREC_D, EEREC_D);
		else if( EEREC_D == EEREC_S ) SSE2_PMULUDQ_XMM_to_XMM(EEREC_D, EEREC_T);
		else if( EEREC_D == EEREC_T ) SSE2_PMULUDQ_XMM_to_XMM(EEREC_D, EEREC_S);
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE2_PMULUDQ_XMM_to_XMM(EEREC_D, EEREC_T);
		}
	}
	else {
		if( !_Rs_ || !_Rt_ ) SSE2_PXOR_XMM_to_XMM(EEREC_HI, EEREC_HI);
		else {
			SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_S);
			SSE2_PMULUDQ_XMM_to_XMM(EEREC_HI, EEREC_T);
		}
	}

	// add from LO/HI
	if ( _Rd_ ) {
		SSE2_PADDQ_XMM_to_XMM(EEREC_D, EEREC_LO);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_D);
	}
	else SSE2_PADDQ_XMM_to_XMM(EEREC_HI, EEREC_LO);

	// interleave & sign extend
	if ( x86caps.hasStreamingSIMD4Extensions ) {
		SSE2_PSHUFD_XMM_to_XMM(EEREC_LO, EEREC_HI, 0x88);
		SSE2_PSHUFD_XMM_to_XMM(EEREC_HI, EEREC_HI, 0xdd);
		SSE4_PMOVSXDQ_XMM_to_XMM(EEREC_LO, EEREC_LO);
		SSE4_PMOVSXDQ_XMM_to_XMM(EEREC_HI, EEREC_HI);
	}
	else {
		int t0reg = _allocTempXMMreg(XMMT_INT, -1);
		SSE2_PSHUFD_XMM_to_XMM(t0reg, EEREC_HI, 0xd8);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_LO, t0reg);
		SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, t0reg);
		SSE2_PSRAD_I8_to_XMM(t0reg, 31); // get the signs

		SSE2_PUNPCKLDQ_XMM_to_XMM(EEREC_LO, t0reg);
		SSE2_PUNPCKHDQ_XMM_to_XMM(EEREC_HI, t0reg);
		_freeXMMreg(t0reg);
	}
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPDIVUW()
{
	recCall( Interp::PDIVUW, _Rd_ );
}

////////////////////////////////////////////////////
void recPEXCW()
{
	if (!_Rd_) return;

	int info = eeRecompileCodeXMM( XMMINFO_READT|XMMINFO_WRITED );
	SSE2_PSHUFD_XMM_to_XMM(EEREC_D, EEREC_T, 0xd8);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPEXCH( void )
{
	if (!_Rd_) return;

	int info = eeRecompileCodeXMM( XMMINFO_READT|XMMINFO_WRITED );
	SSE2_PSHUFLW_XMM_to_XMM(EEREC_D, EEREC_T, 0xd8);
	SSE2_PSHUFHW_XMM_to_XMM(EEREC_D, EEREC_D, 0xd8);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPNOR( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( (_Rs_!=0?XMMINFO_READS:0)|(_Rt_!=0?XMMINFO_READT:0)|XMMINFO_WRITED );

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
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPMTHI( void ) 
{
	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_WRITEHI );
	SSEX_MOVDQA_XMM_to_XMM(EEREC_HI, EEREC_S);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPMTLO( void ) 
{
	int info = eeRecompileCodeXMM( XMMINFO_READS|XMMINFO_WRITELO );
	SSEX_MOVDQA_XMM_to_XMM(EEREC_LO, EEREC_S);
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPCPYUD( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READS|(( _Rt_ == 0) ? 0:XMMINFO_READT)|XMMINFO_WRITED );

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
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPOR( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( (_Rs_!=0?XMMINFO_READS:0)|(_Rt_!=0?XMMINFO_READT:0)|XMMINFO_WRITED );

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
	_clearNeededXMMregs();
}

////////////////////////////////////////////////////
void recPCPYH( void ) 
{
	if ( ! _Rd_ ) return;

	int info = eeRecompileCodeXMM( XMMINFO_READT|XMMINFO_WRITED );
	SSE2_PSHUFLW_XMM_to_XMM(EEREC_D, EEREC_T, 0);
	SSE2_PSHUFHW_XMM_to_XMM(EEREC_D, EEREC_D, 0);
	_clearNeededXMMregs();
}

#endif	// else MMI3_RECOMPILE

} } } }
