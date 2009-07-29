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
#include "iR5900LoadStore.h"
#include "iR5900.h"

// Implemented at the bottom of the module:
void SetFastMemory(int bSetFast);

namespace R5900 { 
namespace Dynarec { 
namespace OpcodeImpl {

/*********************************************************
* Load and store for GPR                                 *
* Format:  OP rt, offset(base)                           *
*********************************************************/
#ifndef LOADSTORE_RECOMPILE

namespace Interp = R5900::Interpreter::OpcodeImpl;

REC_FUNC_DEL(LB, _Rt_);
REC_FUNC_DEL(LBU, _Rt_);
REC_FUNC_DEL(LH, _Rt_);
REC_FUNC_DEL(LHU, _Rt_);
REC_FUNC_DEL(LW, _Rt_);
REC_FUNC_DEL(LWU, _Rt_);
REC_FUNC_DEL(LWL, _Rt_);
REC_FUNC_DEL(LWR, _Rt_);
REC_FUNC_DEL(LD, _Rt_);
REC_FUNC_DEL(LDR, _Rt_);
REC_FUNC_DEL(LDL, _Rt_);
REC_FUNC_DEL(LQ, _Rt_);
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

#else

PCSX2_ALIGNED16(u64 retValues[2]);

void _eeOnLoadWrite(int reg)
{
	int regt;

	if( !reg ) return;

	_eeOnWriteReg(reg, 1);
	regt = _checkXMMreg(XMMTYPE_GPRREG, reg, MODE_READ);

	if( regt >= 0 ) {
		if( xmmregs[regt].mode & MODE_WRITE ) {
			if( reg != _Rs_ ) {
				SSE2_PUNPCKHQDQ_XMM_to_XMM(regt, regt);
				SSE2_MOVQ_XMM_to_M64((u32)&cpuRegs.GPR.r[reg].UL[2], regt);
			}
			else SSE_MOVHPS_XMM_to_M64((u32)&cpuRegs.GPR.r[reg].UL[2], regt);
		}
		xmmregs[regt].inuse = 0;
	}
}

#ifdef PCSX2_VIRTUAL_MEM

static u32 s_bCachingMem = 0;
static u32 s_nAddMemOffset = 0;
static u32 s_tempaddr = 0;

////////////////////////////////////////////////////
//#define REC_SLOWREAD
//#define REC_SLOWWRITE
#define REC_FORCEMMX 0

// if sp, always mem write
int _eeIsMemWrite() { return _Rs_==29||_Rs_== 31||_Rs_==26||_Rs_==27; } // sp, ra, k0, k1
// gp can be 1000a020 (jak1)

void recTransferX86ToReg(int x86reg, int gprreg, int sign)
{
	//if( !REC_FORCEMMX ) assert( _checkMMXreg(MMX_GPR+gprreg, MODE_WRITE) == -1 );
	if( sign ) {
		if( x86reg == EAX && EEINST_ISLIVE1(gprreg) ) CDQ();

		MOV32RtoM( (int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ], x86reg );

		if(EEINST_ISLIVE1(gprreg)) {
			if( x86reg == EAX ) MOV32RtoM( (int)&cpuRegs.GPR.r[ gprreg ].UL[ 1 ], EDX );
			else {
				SAR32ItoR(x86reg, 31);
				MOV32RtoM( (int)&cpuRegs.GPR.r[ gprreg ].UL[ 1 ], x86reg );
			}
		}
		else {
			EEINST_RESETHASLIVE1(gprreg);
		}
	}
	else {
		MOV32RtoM( (int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ], x86reg );
		if(EEINST_ISLIVE1(gprreg)) MOV32ItoM( (int)&cpuRegs.GPR.r[ gprreg ].UL[ 1 ], 0 );
		else EEINST_RESETHASLIVE1(gprreg);
	}
}

#ifdef PCSX2_DEBUG
void testaddrs()
{
	register int tempaddr;

	__asm mov tempaddr, ecx
	//__asm mov incaddr, edx

	assert( (tempaddr < 0x40000000) || (tempaddr&0xd0000000)==0x50000000 || (tempaddr >= 0x80000000 && tempaddr < 0xc0000000) );
	//assert( (tempaddr>>28) == ((tempaddr+incaddr)>>28) );

	__asm mov ecx, tempaddr
}
#endif

static __forceinline void SET_HWLOC_R5900() { 
	if ( s_bCachingMem & 2 ) 
	{
		x86SetJ32(j32Ptr[2]); 
		x86SetJ32(j32Ptr[3]); 
	}
	else 
	{
		x86SetJ8(j8Ptr[0]); 
		x86SetJ8(j8Ptr[3]); 
	}
	
	if (x86FpuState==MMX_STATE) { 
		if (x86caps.has3DNOWInstructionExtensions) 
			FEMMS(); 
		else 
			EMMS(); 
	} 
	if( s_nAddMemOffset ) 
		ADD32ItoR(ECX, s_nAddMemOffset); 
	if( s_bCachingMem & 4 ) 
		AND32ItoR(ECX, 0x5fffffff); 
} 

static u16 g_MemMasks0[16] = {0x00f0, 0x80f1, 0x00f2, 0x00f3,
							  0x00f1, 0x00f5, 0x00f1, 0x00f5,
							  0x00f5, 0x80f5, 0x00f5, 0x80f5,
							  0x00f1, 0x00f1, 0x00f1, 0x00f5 };
static u16 g_MemMasks8[16] = {0x0080, 0x8081, 0x0082, 0x0083,
							  0x0081, 0x0085, 0x0081, 0x0085,
							  0x0085, 0x8085, 0x0085, 0x8085,
							  0x0081, 0x0081, 0x0081, 0x0085 };
static u16 g_MemMasks16[16] ={0x0000, 0x8001, 0x0002, 0x0003,
							  0x0001, 0x0005, 0x0001, 0x0005,
							  0x0005, 0x8005, 0x0005, 0x8005,
							  0x0001, 0x0001, 0x0001, 0x0005 };

void assertmem()
{
	__asm mov s_tempaddr, ecx
	__asm mov s_bCachingMem, edx
	Console::Error("%x(%x) not mem write!", params s_tempaddr, s_bCachingMem);
	assert(0);
}

int _eePrepareReg(int gprreg)
{
	int mmreg = _checkXMMreg(XMMTYPE_GPRREG, gprreg, MODE_READ);

	if( mmreg >= 0 && (xmmregs[mmreg].mode&MODE_WRITE) ) {
		mmreg |= MEM_XMMTAG;
	}
	else if( (mmreg = _checkMMXreg(MMX_GPR+gprreg, MODE_READ)) >= 0 ) {
		if( mmxregs[mmreg].mode&MODE_WRITE ) mmreg |= MEM_MMXTAG;
		else {
			mmxregs[mmreg].needed = 0; // coX can possibly use all regs
			mmreg = 0;
		}
	}
	else {
		mmreg = 0;
	}

	return mmreg;
}

// returns true if should also include hardware writes
int recSetMemLocation(int regs, int imm, int mmreg, int msize, int j32)
{
	s_bCachingMem = j32 ? 2 : 0;
	s_nAddMemOffset = 0;

	//int num;
	if( mmreg >= 0 && (mmreg & MEM_XMMTAG) ) {
		SSE2_MOVD_XMM_to_R(ECX, mmreg&0xf);
	}
	else if( mmreg >= 0 && (mmreg & MEM_MMXTAG) ) {
		MOVD32MMXtoR(ECX, mmreg&0xf);
		SetMMXstate();
	}
	else {
		MOV32MtoR( ECX, (uptr)&cpuRegs.GPR.r[ regs ].UL[ 0 ] );		
	}

	if ( imm != 0 ) ADD32ItoR( ECX, imm );

#ifdef PCSX2_DEBUG
	//CALLFunc((uptr)testaddrs);
#endif


	// 32bit version (faster?)
//		MOV32RtoR(EAX, ECX);
//		ROR32ItoR(ECX, 28);
//		SHR32ItoR(EAX, 28);
//		MOV32RmSOffsettoR(EAX, EAX, msize == 2 ? (u32)&g_MemMasks16[0] : (msize == 1 ? (u32)&g_MemMasks8[0] : (u32)&g_MemMasks0[0]), 2);
//		AND8RtoR(ECX, EAX);
//		ROR32ItoR(ECX, 4);
//		// do extra alignment masks here
//		OR32RtoR(EAX, EAX);

	if( _eeIsMemWrite() ) {
		u8* ptr;
		CMP32ItoR(ECX, 0x40000000);
		ptr = JB8(0);
		if( msize == 1 ) AND32ItoR(ECX, 0x5ffffff8);
		else if( msize == 2 ) AND32ItoR(ECX, 0x5ffffff0);
		else AND32ItoR(ECX, 0x5fffffff);
		x86SetJ8(ptr);
		if( msize == 1 ) AND8ItoR(ECX, 0xf8);
		else if( msize == 2 ) AND8ItoR(ECX, 0xf0);
#ifdef PCSX2_DEBUG
		MOV32RtoR(EAX, ECX);
		SHR32ItoR(EAX, 28);
		CMP32ItoR(EAX, 1);
		ptr = JNE8(0);
		MOV32ItoR(EDX, _Rs_);
		CALLFunc((uptr)assertmem);
		x86SetJ8(ptr);
#endif
		return 0;
	}
	else {
		// 16 bit version
		MOV32RtoR(EAX, ECX);
		ROR32ItoR(ECX, 28);
		SHR32ItoR(EAX, 28);
		MOV16RmSOffsettoR(EAX, EAX, msize == 2 ? (u32)&g_MemMasks16[0] : (msize == 1 ? (u32)&g_MemMasks8[0] : (u32)&g_MemMasks0[0]), 1);
		AND8RtoR(ECX, EAX);
		ROR32ItoR(ECX, 4);

		TEST16RtoR(EAX, EAX); // this is faster, because sets no dependend reg.

		if( s_bCachingMem & 2 ) j32Ptr[2] = j32Ptr[3] = JS32(0);
		else j8Ptr[0] = j8Ptr[3] = JS8(0);
	}

	return 1;
}


void recLoad32(u32 bit, u32 imm, u32 sign)
{
	int mmreg = -1;

#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		// do const processing
		int ineax = 0;

		_eeOnLoadWrite(_Rt_);
		if( bit == 32 ) {
			mmreg = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_WRITE);
			if( mmreg >= 0 ) mmreg |= MEM_MMXTAG;
			else mmreg = EAX;
		}
		else {
			_deleteEEreg(_Rt_, 0);
			mmreg = EAX;
		}

		switch(bit) {
			case 8: ineax = recMemConstRead8(mmreg, g_cpuConstRegs[_Rs_].UL[0]+imm, sign); break;
			case 16:
				assert( (g_cpuConstRegs[_Rs_].UL[0]+imm) % 2 == 0 );
				ineax = recMemConstRead16(mmreg, g_cpuConstRegs[_Rs_].UL[0]+imm, sign);
				break;
			case 32:
				// used by LWL/LWR
				//assert( (g_cpuConstRegs[_Rs_].UL[0]+imm) % 4 == 0 );
				ineax = recMemConstRead32(mmreg, g_cpuConstRegs[_Rs_].UL[0]+imm);
				break;
		}

		if( ineax || !(mmreg&MEM_MMXTAG) ) {
			if( mmreg&MEM_MMXTAG ) mmxregs[mmreg&0xf].inuse = 0;
			recTransferX86ToReg(EAX, _Rt_, sign);
		}
		else {
			assert( mmxregs[mmreg&0xf].mode & MODE_WRITE );
			if( sign ) _signExtendGPRtoMMX(mmreg&0xf, _Rt_, 32-bit);
			else if( bit < 32 ) PSRLDItoR(mmreg&0xf, 32-bit);
		}
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);

		_eeOnLoadWrite(_Rt_);

		if( REC_FORCEMMX ) mmreg = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_WRITE);
		else _deleteEEreg(_Rt_, 0);
		
		dohw = recSetMemLocation(_Rs_, imm, mmregs, 0, 0);

		if( mmreg >= 0 ) {
			MOVD32RmOffsettoMMX(mmreg, ECX, PS2MEM_BASE_+s_nAddMemOffset-(32-bit)/8);
			if( sign ) _signExtendGPRtoMMX(mmreg&0xf, _Rt_, 32-bit);
			else if( bit < 32 ) PSRLDItoR(mmreg&0xf, 32-bit);
		}
		else {
			switch(bit) {
				case 8: 
					if( sign ) MOVSX32Rm8toROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
					else MOVZX32Rm8toROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
					break;
				case 16:
					if( sign ) MOVSX32Rm16toROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
					else MOVZX32Rm16toROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
					break;
				case 32:
					MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
					break;
			}

			if ( _Rt_ ) recTransferX86ToReg(EAX, _Rt_, sign);
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC_R5900();

			switch(bit) {
				case 8:
					CALLFunc( (int)recMemRead8 );
					if( sign ) MOVSX32R8toR(EAX, EAX);
					else MOVZX32R8toR(EAX, EAX);
					break;
				case 16:
					CALLFunc( (int)recMemRead16 );
					if( sign ) MOVSX32R16toR(EAX, EAX);
					else MOVZX32R16toR(EAX, EAX);
					break;
				case 32:
					iMemRead32Check();
					CALLFunc( (int)recMemRead32 );
					break;
			}
			
			if ( _Rt_ ) recTransferX86ToReg(EAX, _Rt_, sign);

			if( mmreg >= 0 ) {
				if( EEINST_ISLIVE1(_Rt_) ) MOVQMtoR(mmreg, (u32)&cpuRegs.GPR.r[_Rt_]);
				else MOVD32RtoMMX(mmreg, EAX);
			}

			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
			else x86SetJ8(j8Ptr[2]);
		}
	}
}

void recLB( void ) { recLoad32(8, _Imm_, 1); }
void recLBU( void ) { recLoad32(8, _Imm_, 0); }
void recLH( void ) { recLoad32(16, _Imm_, 1); }
void recLHU( void ) { recLoad32(16, _Imm_, 0); }
void recLW( void ) { recLoad32(32, _Imm_, 1); }
void recLWU( void ) { recLoad32(32, _Imm_, 0); }

////////////////////////////////////////////////////

void recLWL( void ) 
{
#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);

		recMemConstRead32(EAX, (g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~3);

		if( _Rt_ ) {
			u32 shift = (g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&3;

			_eeMoveGPRtoR(ECX, _Rt_);
			SHL32ItoR(EAX, 24-shift*8);
			AND32ItoR(ECX, (0xffffff>>(shift*8)));
			OR32RtoR(EAX, ECX);

			if ( _Rt_ ) recTransferX86ToReg(EAX, _Rt_, 1);
		}
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);

		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 0, 0);

		MOV32ItoR(EDX, 0x3);
		AND32RtoR(EDX, ECX);
		AND32ItoR(ECX, ~3);
		SHL32ItoR(EDX, 3); // *8
		MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset);

		if( dohw ) {
			j8Ptr[1] = JMP8(0);
			SET_HWLOC_R5900();

			iMemRead32Check();

			// repeat
			MOV32ItoR(EDX, 0x3);
			AND32RtoR(EDX, ECX);
			AND32ItoR(ECX, ~3);
			SHL32ItoR(EDX, 3); // *8

			PUSH32R(EDX);
			CALLFunc( (int)recMemRead32 );
			POP32R(EDX);
			
			x86SetJ8(j8Ptr[1]);
		}

		if ( _Rt_ ) {
			// mem << LWL_SHIFT[shift]
			MOV32ItoR(ECX, 24);
			SUB32RtoR(ECX, EDX);
			SHL32CLtoR(EAX);

			// mov temp and compute _rt_ & LWL_MASK[shift]
			MOV32RtoR(ECX, EDX);
			MOV32ItoR(EDX, 0xffffff);
			SAR32CLtoR(EDX);

			_eeMoveGPRtoR(ECX, _Rt_);
			AND32RtoR(EDX, ECX);

			// combine
			OR32RtoR(EAX, EDX);
			recTransferX86ToReg(EAX, _Rt_, 1);
		}
	}
}

////////////////////////////////////////////////////

void recLWR( void ) 
{
#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);

		recMemConstRead32(EAX, (g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~3);

		if( _Rt_ ) {
			u32 shift = (g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&3;

			_eeMoveGPRtoR(ECX, _Rt_);
			SHL32ItoR(EAX, shift*8);
			AND32ItoR(ECX, (0xffffff00<<(24-shift*8)));
			OR32RtoR(EAX, ECX);

			recTransferX86ToReg(EAX, _Rt_, 1);
		}
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);

		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 0, 0);
		
		MOV32RtoR(EDX, ECX);
		AND32ItoR(ECX, ~3);
		MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset);

		if( dohw ) {
			j8Ptr[1] = JMP8(0);
			SET_HWLOC_R5900();

			iMemRead32Check();

			PUSH32R(ECX);
			AND32ItoR(ECX, ~3);
			CALLFunc( (int)recMemRead32 );
			POP32R(EDX);
			
			x86SetJ8(j8Ptr[1]);
		}

		if ( _Rt_ )
		{
			// mem << LWL_SHIFT[shift]
			MOV32RtoR(ECX, EDX);
			AND32ItoR(ECX, 3);
			SHL32ItoR(ECX, 3); // *8
			SHR32CLtoR(EAX);

			// mov temp and compute _rt_ & LWL_MASK[shift]
			MOV32RtoR(EDX, ECX);
			MOV32ItoR(ECX, 24);
			SUB32RtoR(ECX, EDX);
			MOV32ItoR(EDX, 0xffffff00);
			SHL32CLtoR(EDX);
			_eeMoveGPRtoR(ECX, _Rt_);
			AND32RtoR(ECX, EDX);

			// combine
			OR32RtoR(EAX, ECX);
			
			recTransferX86ToReg(EAX, _Rt_, 1);
		}
	}
}

////////////////////////////////////////////////////
void recLoad64(u32 imm, int align)
{
	int mmreg;
	int mask = align ? ~7 : ~0;

#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		// also used by LDL/LDR
		//assert( (g_cpuConstRegs[_Rs_].UL[0]+imm) % 8 == 0 );

		mmreg = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_WRITE);
		if( _Rt_ && mmreg >= 0 ) {
			recMemConstRead64((g_cpuConstRegs[_Rs_].UL[0]+imm)&mask, mmreg);
			assert( mmxregs[mmreg&0xf].mode & MODE_WRITE );
		}
		else if( _Rt_ && (mmreg = _allocCheckGPRtoXMM(g_pCurInstInfo, _Rt_, MODE_WRITE|MODE_READ)) >= 0 ) {
			recMemConstRead64((g_cpuConstRegs[_Rs_].UL[0]+imm)&mask, mmreg|0x8000);	
			assert( xmmregs[mmreg&0xf].mode & MODE_WRITE );
		}
		else {
			if( !_hasFreeMMXreg() && _hasFreeXMMreg() ) {
				mmreg = _allocGPRtoXMMreg(-1, _Rt_, MODE_READ|MODE_WRITE);
				recMemConstRead64((g_cpuConstRegs[_Rs_].UL[0]+imm)&mask, mmreg|0x8000);
				assert( xmmregs[mmreg&0xf].mode & MODE_WRITE );
			}
			else {
				int t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
				recMemConstRead64((g_cpuConstRegs[_Rs_].UL[0]+imm)&mask, t0reg);

				if( _Rt_ ) {
					SetMMXstate();
					MOVQRtoM((u32)&cpuRegs.GPR.r[_Rt_].UD[0], t0reg);
				}

				_freeMMXreg(t0reg);
			}
		}
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);

		if( _Rt_ && (mmreg = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_WRITE)) >= 0 ) {
			dohw = recSetMemLocation(_Rs_, imm, mmregs, align, 0);

			MOVQRmtoROffset(mmreg, ECX, PS2MEM_BASE_+s_nAddMemOffset);

			if( dohw ) {
				if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
				else j8Ptr[2] = JMP8(0);

				SET_HWLOC_R5900();

				PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
				CALLFunc( (int)recMemRead64 );
				MOVQMtoR(mmreg, (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
				ADD32ItoR(ESP, 4);
			}
			
			SetMMXstate();
		}
		else if( _Rt_ && (mmreg = _allocCheckGPRtoXMM(g_pCurInstInfo, _Rt_, MODE_WRITE|MODE_READ)) >= 0 ) {
			dohw = recSetMemLocation(_Rs_, imm, mmregs, align, 0);

			SSE_MOVLPSRmtoROffset(mmreg, ECX, PS2MEM_BASE_+s_nAddMemOffset);

			if( dohw ) {
				if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
				else j8Ptr[2] = JMP8(0);

				SET_HWLOC_R5900();

				PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
				CALLFunc( (int)recMemRead64 );
				SSE_MOVLPS_M64_to_XMM(mmreg, (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
				ADD32ItoR(ESP, 4);
			}
		}
		else {
			int t0reg = _Rt_ ? _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_WRITE) : -1;

			dohw = recSetMemLocation(_Rs_, imm, mmregs, align, 0);

			if( t0reg >= 0 ) {
				MOVQRmtoROffset(t0reg, ECX, PS2MEM_BASE_+s_nAddMemOffset);
				SetMMXstate();
			}

			if( dohw ) {
				if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
				else j8Ptr[2] = JMP8(0);

				SET_HWLOC_R5900();

				if( _Rt_ ) {
					//_deleteEEreg(_Rt_, 0);
					PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
				}
				else PUSH32I( (int)&retValues[0] );

				CALLFunc( (int)recMemRead64 );
				ADD32ItoR(ESP, 4);

				if( t0reg >= 0 ) MOVQMtoR(t0reg, (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
			}
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
			else x86SetJ8(j8Ptr[2]);
		}
	}

	_clearNeededMMXregs();
	_clearNeededXMMregs();
	if( _Rt_ ) _eeOnWriteReg(_Rt_, 0);
}

void recLD(void) { recLoad64(_Imm_, 1); }

////////////////////////////////////////////////////
void recLDL( void ) 
{
	iFlushCall(FLUSH_NOCONST);

	if( GPR_IS_CONST1( _Rs_ ) ) {
		// flush temporarily
		MOV32ItoM((int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], g_cpuConstRegs[_Rs_].UL[0]);
		//MOV32ItoM((int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], g_cpuConstRegs[_Rs_].UL[1]);
	}
	else {
		_deleteGPRtoXMMreg(_Rs_, 1);
		_deleteMMXreg(MMX_GPR+_Rs_, 1);
	}

	if( _Rt_ )
		_deleteEEreg(_Rt_, _Rt_==_Rs_);

	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)R5900::Interpreter::OpcodeImpl::LDL );
}

////////////////////////////////////////////////////
void recLDR( void ) 
{
	iFlushCall(FLUSH_NOCONST);

	if( GPR_IS_CONST1( _Rs_ ) ) {
		// flush temporarily
		MOV32ItoM((int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], g_cpuConstRegs[_Rs_].UL[0]);
		//MOV32ItoM((int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], g_cpuConstRegs[_Rs_].UL[1]);
	}
	else {
		_deleteGPRtoXMMreg(_Rs_, 1);
		_deleteMMXreg(MMX_GPR+_Rs_, 1);
	}

	if( _Rt_ )
		_deleteEEreg(_Rt_, _Rt_==_Rs_);

	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)R5900::Interpreter::OpcodeImpl::LDR );
}

////////////////////////////////////////////////////
void recLQ( void ) 
{
	int mmreg = -1;
#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
        // malice hits this
		assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_) % 16 == 0 );

		if( _Rt_ ) {
			if( (g_pCurInstInfo->regs[_Rt_]&EEINST_XMM) || !(g_pCurInstInfo->regs[_Rt_]&EEINST_MMX) ) {
				_deleteMMXreg(MMX_GPR+_Rt_, 2);
				_eeOnWriteReg(_Rt_, 0);
				mmreg = _allocGPRtoXMMreg(-1, _Rt_, MODE_WRITE);
			}
			else {
				int t0reg;
				_deleteGPRtoXMMreg(_Rt_, 2);
				_eeOnWriteReg(_Rt_, 0);
				mmreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_WRITE)|MEM_MMXTAG;
				t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
				mmreg |= t0reg<<4;
			}
		}
		else {
			mmreg = _allocTempXMMreg(XMMT_INT, -1);
		}

		recMemConstRead128((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~15, mmreg);

		if( !_Rt_ ) _freeXMMreg(mmreg);
		if( IS_MMXREG(mmreg) ) {
			// flush temp
			assert( mmxregs[mmreg&0xf].mode & MODE_WRITE );
			MOVQRtoM((u32)&cpuRegs.GPR.r[_Rt_].UL[2], (mmreg>>4)&0xf);
			_freeMMXreg((mmreg>>4)&0xf);
		}
		else assert( xmmregs[mmreg&0xf].mode & MODE_WRITE );
	}
	else
#endif
	{
		int dohw;
		int mmregs;
		int t0reg = -1;
		
		if( GPR_IS_CONST1( _Rs_ ) )
			_flushConstReg(_Rs_);

		mmregs = _eePrepareReg(_Rs_);

		if( _Rt_ ) {
			_eeOnWriteReg(_Rt_, 0);

			// check if can process mmx
			if( _hasFreeMMXreg() ) {
				mmreg = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_WRITE);
				if( mmreg >= 0 ) {
					mmreg |= MEM_MMXTAG;
					t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
				}
			}
			else _deleteMMXreg(MMX_GPR+_Rt_, 2);
			
			if( mmreg < 0 ) {
				mmreg = _allocGPRtoXMMreg(-1, _Rt_, MODE_WRITE);
				if( mmreg >= 0 ) mmreg |= MEM_XMMTAG;
			}
		}

		if( mmreg < 0 ) {
			_deleteEEreg(_Rt_, 1);
		}

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 2, 0);

		if( _Rt_ ) {
			if( mmreg >= 0 && (mmreg & MEM_MMXTAG) ) {
				MOVQRmtoROffset(t0reg, ECX, PS2MEM_BASE_+s_nAddMemOffset+8);
				MOVQRmtoROffset(mmreg&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset);
				MOVQRtoM((u32)&cpuRegs.GPR.r[_Rt_].UL[2], t0reg);
			}
			else if( mmreg >= 0 && (mmreg & MEM_XMMTAG) ) {
				SSEX_MOVDQARmtoROffset(mmreg&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset);
			}
			else {
				_recMove128RmOffsettoM((u32)&cpuRegs.GPR.r[_Rt_].UL[0], PS2MEM_BASE_+s_nAddMemOffset);
			}

			if( dohw ) {
				if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
				else j8Ptr[2] = JMP8(0);

				SET_HWLOC_R5900();

				PUSH32I( (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
				CALLFunc( (uptr)recMemRead128 );

				if( mmreg >= 0 && (mmreg & MEM_MMXTAG) ) MOVQMtoR(mmreg&0xf, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
				else if( mmreg >= 0 && (mmreg & MEM_XMMTAG) ) SSEX_MOVDQA_M128_to_XMM(mmreg&0xf, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
				
				ADD32ItoR(ESP, 4);
			}
		}
		else {
			if( dohw ) {
				if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
				else j8Ptr[2] = JMP8(0);

				SET_HWLOC_R5900();

				PUSH32I( (u32)&retValues[0] );
				CALLFunc( (uptr)recMemRead128 );
				ADD32ItoR(ESP, 4);
			}
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
			else x86SetJ8(j8Ptr[2]);
		}

		if( t0reg >= 0 ) _freeMMXreg(t0reg);
	}

	_clearNeededXMMregs(); // needed since allocing
}

////////////////////////////////////////////////////
static void recClear64(BASEBLOCK* p)
{
	int left = 4 - ((u32)p % 16)/sizeof(BASEBLOCK);
	recClearMem(p);

	if( left > 1 && *(u32*)(p+1) ) recClearMem(p+1);
}

static void recClear128(BASEBLOCK* p)
{
	int left = 4 - ((u32)p % 32)/sizeof(BASEBLOCK);
	recClearMem(p);

	if( left > 1 && *(u32*)(p+1) ) recClearMem(p+1);
	if( left > 2 && *(u32*)(p+2) ) recClearMem(p+2);
	if( left > 3 && *(u32*)(p+3) ) recClearMem(p+3);
}

// check if clearing executable code, size is in dwords
void recMemConstClear(u32 mem, u32 size)
{
	// NOTE! This assumes recLUT never changes its mapping
	if( !recLUT[mem>>16] )
		return;

	//iFlushCall(0); // just in case

	// check if mem is executable, and clear it
	//CMP32ItoM((u32)&maxrecmem, mem);
	//j8Ptr[5] = JBE8(0);

	// can clear now
	if( size == 1 ) {
		CMP32ItoM((u32)PC_GETBLOCK(mem), 0);
		j8Ptr[6] = JE8(0);

		PUSH32I((u32)PC_GETBLOCK(mem));
		CALLFunc((uptr)recClearMem);
		ADD32ItoR(ESP, 4);
		x86SetJ8(j8Ptr[6]);
	}
	else if( size == 2 ) {
		// need to clear 8 bytes

		CMP32I8toM((u32)PC_GETBLOCK(mem), 0);
		j8Ptr[6] = JNE8(0);

		CMP32I8toM((u32)PC_GETBLOCK(mem)+8, 0);
		j8Ptr[7] = JNE8(0);

		j8Ptr[8] = JMP8(0);

		// call clear
		x86SetJ8( j8Ptr[7] );
		x86SetJ8( j8Ptr[6] );

		PUSH32I((u32)PC_GETBLOCK(mem));
		CALLFunc((uptr)recClear64);
		ADD32ItoR(ESP, 4);

		x86SetJ8( j8Ptr[8] );
	}
	else {
		assert( size == 4 );
		// need to clear 16 bytes

		CMP32I8toM((u32)PC_GETBLOCK(mem), 0);
		j8Ptr[6] = JNE8(0);
		
		CMP32I8toM((u32)PC_GETBLOCK(mem)+sizeof(BASEBLOCK), 0);
		j8Ptr[7] = JNE8(0);
		
		CMP32I8toM((u32)PC_GETBLOCK(mem)+sizeof(BASEBLOCK)*2, 0);
		j8Ptr[8] = JNE8(0);
		
		CMP32I8toM((u32)PC_GETBLOCK(mem)+sizeof(BASEBLOCK)*3, 0);
		j8Ptr[9] = JNE8(0);

		j8Ptr[10] = JMP8(0);

		// call clear
		x86SetJ8( j8Ptr[6] );
		x86SetJ8( j8Ptr[7] );
		x86SetJ8( j8Ptr[8] );
		x86SetJ8( j8Ptr[9] );

		PUSH32I((u32)PC_GETBLOCK(mem));
		CALLFunc((uptr)recClear128);
		ADD32ItoR(ESP, 4);

		x86SetJ8( j8Ptr[10] );
	}

	//x86SetJ8(j8Ptr[5]);
}

// check if mem is executable, and clear it
__declspec(naked) void recWriteMemClear32()
{
	_asm {
		mov edx, ecx
		shr edx, 14
		and dl, 0xfc
		add edx, recLUT
		test dword ptr [edx], 0xffffffff
		jnz Clear32
		ret
Clear32:
		// recLUT[mem>>16] + (mem&0xfffc)
		mov edx, dword ptr [edx]
		mov eax, ecx
		and eax, 0xfffc
		// edx += 2*eax
		shl eax, 1
		add edx, eax
		cmp dword ptr [edx], 0
		je ClearRet
		sub esp, 4
		mov dword ptr [esp], edx
		call recClearMem
		add esp, 4
ClearRet:
		ret
	}
}

// check if mem is executable, and clear it
__declspec(naked) void recWriteMemClear64()
{
	__asm {
		// check if mem is executable, and clear it
		mov edx, ecx
		shr edx, 14
		and edx, 0xfffffffc
		add edx, recLUT
		cmp dword ptr [edx], 0
		jne Clear64
		ret
Clear64:
		// recLUT[mem>>16] + (mem&0xffff)
		mov edx, dword ptr [edx]
		mov eax, ecx
		and eax, 0xfffc
		// edx += 2*eax
		shl eax, 1
		add edx, eax

		// note: have to check both blocks
		cmp dword ptr [edx], 0
		jne DoClear0
		cmp dword ptr [edx+8], 0
		jne DoClear1
		ret

DoClear1:
		add edx, 8
DoClear0:
		sub esp, 4
		mov dword ptr [esp], edx
		call recClear64
		add esp, 4
		ret
	}
}

// check if mem is executable, and clear it
__declspec(naked) void recWriteMemClear128()
{
	__asm {
		// check if mem is executable, and clear it
		mov edx, ecx
		shr edx, 14
		and edx, 0xfffffffc
		add edx, recLUT
		cmp dword ptr [edx], 0
		jne Clear128
		ret
Clear128:
		// recLUT[mem>>16] + (mem&0xffff)
		mov edx, dword ptr [edx]
		mov eax, ecx
		and eax, 0xfffc
		// edx += 2*eax
		shl eax, 1
		add edx, eax

		// note: have to check all 4 blocks
		cmp dword ptr [edx], 0
		jne DoClear
		add edx, 8
		cmp dword ptr [edx], 0
		jne DoClear
		add edx, 8
		cmp dword ptr [edx], 0
		jne DoClear
		add edx, 8
		cmp dword ptr [edx], 0
		jne DoClear
		ret

DoClear:
		sub esp, 4
		mov dword ptr [esp], edx
		call recClear128
		add esp, 4
		ret
	}
}

void recStore_raw(EEINST* pinst, int bit, int x86reg, int gprreg, u32 offset)
{
	if( bit == 128 ) {
		int mmreg;
		if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, gprreg, MODE_READ)) >= 0) {
			SSEX_MOVDQARtoRmOffset(ECX, mmreg, PS2MEM_BASE_+offset);
		}
		else if( (mmreg = _checkMMXreg(MMX_GPR+gprreg, MODE_READ)) >= 0) {

			if( _hasFreeMMXreg() ) {
				int t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
				MOVQMtoR(t0reg, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 2 ]);
				MOVQRtoRmOffset(ECX, mmreg, PS2MEM_BASE_+offset);
				MOVQRtoRmOffset(ECX, t0reg, PS2MEM_BASE_+offset+8);
				_freeMMXreg(t0reg);
			}
			else {
				MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 2 ]);
				MOV32MtoR(EDX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 3 ]);
				MOVQRtoRmOffset(ECX, mmreg, PS2MEM_BASE_+offset);
				MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+offset+8);
				MOV32RtoRmOffset(ECX, EDX, PS2MEM_BASE_+offset+12);
			}

			SetMMXstate();
		}
		else {
			
			if( GPR_IS_CONST1( gprreg ) ) {
				assert( _checkXMMreg(XMMTYPE_GPRREG, gprreg, MODE_READ) == -1 );

				if( _hasFreeMMXreg() ) {
					int t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
					SetMMXstate();
					MOVQMtoR(t0reg, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 2 ]);
					MOV32ItoRmOffset(ECX, g_cpuConstRegs[gprreg].UL[0], PS2MEM_BASE_+offset);
					MOV32ItoRmOffset(ECX, g_cpuConstRegs[gprreg].UL[1], PS2MEM_BASE_+offset+4);
					MOVQRtoRmOffset(ECX, t0reg, PS2MEM_BASE_+offset+8);
					_freeMMXreg(t0reg);
				}
				else {
					MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 2 ]);
					MOV32MtoR(EDX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 3 ]);
					MOV32ItoRmOffset(ECX, g_cpuConstRegs[gprreg].UL[0], PS2MEM_BASE_+offset);
					MOV32ItoRmOffset(ECX, g_cpuConstRegs[gprreg].UL[1], PS2MEM_BASE_+offset+4);
					MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+offset+8);
					MOV32RtoRmOffset(ECX, EDX, PS2MEM_BASE_+offset+12);
				}
			}
			else {
				if( _hasFreeXMMreg() ) {
					mmreg = _allocGPRtoXMMreg(-1, gprreg, MODE_READ);
					SSEX_MOVDQARtoRmOffset(ECX, mmreg, PS2MEM_BASE_+offset);
					_freeXMMreg(mmreg);
				}
				else if( _hasFreeMMXreg() ) {
					mmreg = _allocMMXreg(-1, MMX_TEMP, 0);

					if( _hasFreeMMXreg() ) {
						int t0reg;
						t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
						MOVQMtoR(mmreg, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ]);
						MOVQMtoR(t0reg, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 2 ]);
						MOVQRtoRmOffset(ECX, mmreg, PS2MEM_BASE_+offset);
						MOVQRtoRmOffset(ECX, t0reg, PS2MEM_BASE_+offset+8);
						_freeMMXreg(t0reg);
					}
					else {
						MOVQMtoR(mmreg, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ]);
						MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 2 ]);
						MOV32MtoR(EDX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 3 ]);
						MOVQRtoRmOffset(ECX, mmreg, PS2MEM_BASE_+offset);
						MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+offset+8);
						MOV32RtoRmOffset(ECX, EDX, PS2MEM_BASE_+offset+12);
//						MOVQMtoR(mmreg, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 2 ]);
//						MOVQRtoRmOffset(ECX, mmreg, PS2MEM_BASE_+offset+8);
					}
					SetMMXstate();
					_freeMMXreg(mmreg);
				}
				else {
					MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ]);
					MOV32MtoR(EDX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 1 ]);
					MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+offset);
					MOV32RtoRmOffset(ECX, EDX, PS2MEM_BASE_+offset+4);
					MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 2 ]);
					MOV32MtoR(EDX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 3 ]);
					MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+offset+8);
					MOV32RtoRmOffset(ECX, EDX, PS2MEM_BASE_+offset+12);
				}
			}
		}
		return;
	}

	if( GPR_IS_CONST1( gprreg ) ) {
		switch(bit) {
			case 8: MOV8ItoRmOffset(ECX, g_cpuConstRegs[gprreg].UL[0], PS2MEM_BASE_+offset); break;
			case 16: MOV16ItoRmOffset(ECX, g_cpuConstRegs[gprreg].UL[0], PS2MEM_BASE_+offset); break;
			case 32: MOV32ItoRmOffset(ECX, g_cpuConstRegs[gprreg].UL[0], PS2MEM_BASE_+offset); break;
			case 64:
				MOV32ItoRmOffset(ECX, g_cpuConstRegs[gprreg].UL[0], PS2MEM_BASE_+offset);
				MOV32ItoRmOffset(ECX, g_cpuConstRegs[gprreg].UL[1], PS2MEM_BASE_+offset+4);
				break;
		}
	}
	else {
		int mmreg = _checkMMXreg(MMX_GPR+gprreg, MODE_READ);
		if( mmreg < 0 ) {
			mmreg = _checkXMMreg(XMMTYPE_GPRREG, gprreg, MODE_READ);
			if( mmreg >= 0 ) mmreg |= 0x8000;
		}

		if( bit == 64 ) {
			//sd
			if( mmreg >= 0 ) {
				if( mmreg & 0x8000 ) {
					SSE_MOVLPSRtoRmOffset(ECX, mmreg&0xf, PS2MEM_BASE_+offset);
				}
				else {
					MOVQRtoRmOffset(ECX, mmreg&0xf, PS2MEM_BASE_+offset);
					SetMMXstate();
				}
			}
			else {
				if( (mmreg = _allocCheckGPRtoMMX(pinst, gprreg, MODE_READ)) >= 0 ) {	
					MOVQRtoRmOffset(ECX, mmreg, PS2MEM_BASE_+offset);
                    SetMMXstate();
                    _freeMMXreg(mmreg);
				}
				else if( _hasFreeMMXreg() ) {
					mmreg = _allocMMXreg(-1, MMX_GPR+gprreg, MODE_READ);
					MOVQRtoRmOffset(ECX, mmreg, PS2MEM_BASE_+offset);
					SetMMXstate();
					_freeMMXreg(mmreg);
				}
				else {
					MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ]);
					MOV32MtoR(EDX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 1 ]);
					MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+offset);
					MOV32RtoRmOffset(ECX, EDX, PS2MEM_BASE_+offset+4);
				}
			}
		}
		else if( bit == 32 ) {
			// sw
			if( mmreg >= 0 ) {
				if( mmreg & 0x8000) {
					SSEX_MOVD_XMM_to_RmOffset(ECX, mmreg&0xf, PS2MEM_BASE_+offset);
				}
				else {
					MOVD32MMXtoRmOffset(ECX, mmreg&0xf, PS2MEM_BASE_+offset);
					SetMMXstate();
				}
			}
			else {
				MOV32MtoR(x86reg, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ]);
				MOV32RtoRmOffset(ECX, x86reg, PS2MEM_BASE_+offset);
			}
		}
		else {
			// sb, sh
			if( mmreg >= 0) {
				if( mmreg & 0x8000) {
					if( !(xmmregs[mmreg&0xf].mode&MODE_WRITE) ) mmreg = -1;
				}
				else {
					if( !(mmxregs[mmreg&0xf].mode&MODE_WRITE) ) mmreg = -1;
				}
			}

			if( mmreg >= 0) {
				if( mmreg & 0x8000 ) SSE2_MOVD_XMM_to_R(x86reg, mmreg&0xf);
				else {
					MOVD32MMXtoR(x86reg, mmreg&0xf);
					SetMMXstate();
				}
			}
			else {
				switch(bit) {
					case 8: MOV8MtoR(x86reg, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ]); break;
					case 16: MOV16MtoR(x86reg, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ]); break;
					case 32: MOV32MtoR(x86reg, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ]); break;
				}
			}

			switch(bit) {
				case 8: MOV8RtoRmOffset(ECX, x86reg, PS2MEM_BASE_+offset); break;
				case 16: MOV16RtoRmOffset(ECX, x86reg, PS2MEM_BASE_+offset); break;
				case 32: MOV32RtoRmOffset(ECX, x86reg, PS2MEM_BASE_+offset); break;
			}
		}
	}
}

void recStore_call(int bit, int gprreg, u32 offset)
{
	// some type of hardware write
	if( GPR_IS_CONST1( gprreg ) ) {
		if( bit == 128 ) {
			if( gprreg > 0 ) {
				assert( _checkXMMreg(XMMTYPE_GPRREG, gprreg, MODE_READ) == -1 );
				MOV32ItoM((int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ], g_cpuConstRegs[gprreg].UL[0]);
				MOV32ItoM((int)&cpuRegs.GPR.r[ gprreg ].UL[ 1 ], g_cpuConstRegs[gprreg].UL[1]);
			}

			MOV32ItoR(EAX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ]);
		}
		else if( bit == 64 ) {
			if( !(g_cpuFlushedConstReg&(1<<gprreg)) ) {
				// write to a temp loc, trick
				u32* ptr = recAllocStackMem(8, 4);
				ptr[0] = g_cpuConstRegs[gprreg].UL[0];
				ptr[1] = g_cpuConstRegs[gprreg].UL[1];
				MOV32ItoR(EAX, (u32)ptr);
			}
			else {
				MOV32ItoR(EAX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ]);
			}
		}
		else {
			switch(bit) {
				case 8: MOV8ItoR(EAX, g_cpuConstRegs[gprreg].UL[0]); break;
				case 16: MOV16ItoR(EAX, g_cpuConstRegs[gprreg].UL[0]); break;
				case 32: MOV32ItoR(EAX, g_cpuConstRegs[gprreg].UL[0]); break;
			}
		}
	}
	else {
		int mmreg;

		if( bit == 128 ) {
			if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, gprreg, MODE_READ)) >= 0) {

				if( xmmregs[mmreg].mode & MODE_WRITE ) {
					SSEX_MOVDQA_XMM_to_M128((int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ], mmreg);
				}
			}
			else if( (mmreg = _checkMMXreg(MMX_GPR+gprreg, MODE_READ)) >= 0) {
				if( mmxregs[mmreg].mode & MODE_WRITE ) {
					MOVQRtoM((int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ], mmreg);
				}
			}

			MOV32ItoR(EAX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ]);
		}
		else if( bit == 64 ) {
			// sd
			if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, gprreg, MODE_READ)) >= 0) {
				
				if( xmmregs[mmreg].mode & MODE_WRITE ) {
					SSE_MOVLPS_XMM_to_M64((int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ], mmreg);
				}
			}
			else if( (mmreg = _checkMMXreg(MMX_GPR+gprreg, MODE_READ)) >= 0) {
				if( mmxregs[mmreg].mode & MODE_WRITE ) {
					MOVQRtoM((int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ], mmreg);
					SetMMXstate();
				}
			}
			
			MOV32ItoR(EAX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ]);
		}
		else {
			// sb, sh, sw
			if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, gprreg, MODE_READ)) >= 0 && (xmmregs[mmreg].mode&MODE_WRITE) ) {
				SSE2_MOVD_XMM_to_R(EAX, mmreg);
			}
			else if( (mmreg = _checkMMXreg(MMX_GPR+gprreg, MODE_READ)) >= 0 && (mmxregs[mmreg].mode&MODE_WRITE)) {
				MOVD32MMXtoR(EAX, mmreg);
				SetMMXstate();
			}
			else {
				switch(bit) {
					case 8: MOV8MtoR(EAX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ]); break;
					case 16: MOV16MtoR(EAX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ]); break;
					case 32: MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ gprreg ].UL[ 0 ]); break;
				}
			}
		}
	}

	if( offset != 0 ) ADD32ItoR(ECX, offset);

	// some type of hardware write
	switch(bit) {
		case 8: CALLFunc( (int)recMemWrite8 ); break;
		case 16: CALLFunc( (int)recMemWrite16 ); break;
		case 32: CALLFunc( (int)recMemWrite32 ); break;
		case 64: CALLFunc( (int)recMemWrite64 ); break;
		case 128: CALLFunc( (int)recMemWrite128 ); break;
	}
}

int _eePrepConstWrite128(int gprreg)
{
	int mmreg = 0;

	if( GPR_IS_CONST1(gprreg) ) {
		if( gprreg ) {
			mmreg = _allocMMXreg(-1, MMX_TEMP, 0);
			MOVQMtoR(mmreg, (u32)&cpuRegs.GPR.r[gprreg].UL[2]);
			_freeMMXreg(mmreg);
			mmreg |= (gprreg<<16)|MEM_EECONSTTAG;
		}
		else {
			mmreg = _allocTempXMMreg(XMMT_INT, -1);
			SSEX_PXOR_XMM_to_XMM(mmreg, mmreg);
			_freeXMMreg(mmreg);
			mmreg |= MEM_XMMTAG;
		}
	}
	else {
		if( (mmreg = _checkMMXreg(MMX_GPR+gprreg, MODE_READ)) >= 0 ) { 
			int mmregtemp = _allocMMXreg(-1, MMX_TEMP, 0);
			MOVQMtoR(mmregtemp, (u32)&cpuRegs.GPR.r[gprreg].UL[2]);
			mmreg |= (mmregtemp<<4)|MEM_MMXTAG;
			_freeMMXreg(mmregtemp);
		}
		else mmreg = _allocGPRtoXMMreg(-1, gprreg, MODE_READ)|MEM_XMMTAG;
	}

	return mmreg;
}

void recStore(int bit, u32 imm, int align)
{
	int mmreg;

#ifdef REC_SLOWWRITE
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		u32 addr = g_cpuConstRegs[_Rs_].UL[0]+imm;
		int doclear = 0;
		switch(bit) {
			case 8:
				if( GPR_IS_CONST1(_Rt_) ) doclear = recMemConstWrite8(addr, MEM_EECONSTTAG|(_Rt_<<16));
				else {
					_eeMoveGPRtoR(EAX, _Rt_);
					doclear = recMemConstWrite8(addr, EAX);
				}
				
				if( doclear ) {
					recMemConstClear((addr)&~3, 1);
				}

				break;
			case 16:
				assert( (addr)%2 == 0 );
				if( GPR_IS_CONST1(_Rt_) ) doclear = recMemConstWrite16(addr, MEM_EECONSTTAG|(_Rt_<<16));
				else {
					_eeMoveGPRtoR(EAX, _Rt_);
					doclear = recMemConstWrite16(addr, EAX);
				}

				if( doclear ) {
					recMemConstClear((addr)&~3, 1);
				}

				break;
			case 32:
				// used by SWL/SWR
				//assert( (addr)%4 == 0 );
				if( GPR_IS_CONST1(_Rt_) ) doclear = recMemConstWrite32(addr, MEM_EECONSTTAG|(_Rt_<<16));
				else if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG,  _Rt_, MODE_READ)) >= 0 ) {
					doclear = recMemConstWrite32(addr, mmreg|MEM_XMMTAG|(_Rt_<<16));
				}
				else if( (mmreg = _checkMMXreg(MMX_GPR+_Rt_, MODE_READ)) >= 0 ) {
					doclear = recMemConstWrite32(addr, mmreg|MEM_MMXTAG|(_Rt_<<16));
				}
				else {
					_eeMoveGPRtoR(EAX, _Rt_);
					doclear = recMemConstWrite32(addr, EAX);
				}

				if( doclear ) {
					recMemConstClear((addr)&~3, 1);
				}
				break;
			case 64:
			{
				//assert( (addr)%8 == 0 );
				int mask = align ? ~7 : ~0;

				if( GPR_IS_CONST1(_Rt_) ) doclear = recMemConstWrite64(addr, MEM_EECONSTTAG|(_Rt_<<16));
				else if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, _Rt_, MODE_READ)) >= 0 ) {
					doclear = recMemConstWrite64(addr&mask, mmreg|MEM_XMMTAG|(_Rt_<<16));
				}
				else {
					mmreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_READ);
					doclear = recMemConstWrite64(addr&mask, mmreg|MEM_MMXTAG|(_Rt_<<16));
				}

				if( doclear ) {
					recMemConstClear((addr)&~7, 2);
				}

				break;
			}
			case 128:
				//assert( (addr)%16 == 0 );

				if( recMemConstWrite128((addr)&~15, _eePrepConstWrite128(_Rt_)) ) {
					CMP32ItoM((u32)&maxrecmem, addr);
					j8Ptr[0] = JB8(0);
					recMemConstClear((addr)&~15, 4);
					x86SetJ8(j8Ptr[0]);
				}

				break;
		}
	}
	else
#endif
	{
		int dohw;
		int mmregs;
		
		if( GPR_IS_CONST1( _Rs_ ) ) {
			_flushConstReg(_Rs_);
		}

		mmregs = _eePrepareReg(_Rs_);
		dohw = recSetMemLocation(_Rs_, imm, mmregs, align ? bit/64 : 0, 0);

		recStore_raw(g_pCurInstInfo, bit, EAX, _Rt_, s_nAddMemOffset);

		if( s_nAddMemOffset ) ADD32ItoR(ECX, s_nAddMemOffset);
		CMP32MtoR(ECX, (u32)&maxrecmem);
		
		if( s_bCachingMem & 2 ) j32Ptr[4] = JAE32(0);
		else j8Ptr[1] = JAE8(0);

		if( bit < 32 || !align ) AND8ItoR(ECX, 0xfc);
		if( bit <= 32 ) CALLFunc((uptr)recWriteMemClear32);
		else if( bit == 64 ) CALLFunc((uptr)recWriteMemClear64);
		else CALLFunc((uptr)recWriteMemClear128);

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[5] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC_R5900();

			recStore_call(bit, _Rt_, s_nAddMemOffset);

			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[5]);
			else x86SetJ8(j8Ptr[2]);
		}

		if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
		else x86SetJ8(j8Ptr[1]);
	}

	_clearNeededMMXregs(); // needed since allocing
	_clearNeededXMMregs(); // needed since allocing
}


void recSB( void ) { recStore(8, _Imm_, 1); }
void recSH( void ) { recStore(16, _Imm_, 1); }
void recSW( void ) { recStore(32, _Imm_, 1); }

////////////////////////////////////////////////////
void recSWL( void ) 
{
#ifdef REC_SLOWWRITE
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		int shift = (g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&3;

		recMemConstRead32(EAX, (g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~3);

		_eeMoveGPRtoR(ECX, _Rt_);
		AND32ItoR(EAX, 0xffffff00<<(shift*8));
		SHR32ItoR(ECX, 24-shift*8);
		OR32RtoR(EAX, ECX);

		if( recMemConstWrite32((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~3, EAX) )
			recMemConstClear((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~3, 1);
	}
	else 
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);
		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 0, 0);

		MOV32ItoR(EDX, 0x3);
		AND32RtoR(EDX, ECX);
		AND32ItoR(ECX, ~3);
		SHL32ItoR(EDX, 3); // *8
		PUSH32R(ECX);
		XOR32RtoR(EDI, EDI);

		MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset);

		if( dohw ) {
			j8Ptr[1] = JMP8(0);
			SET_HWLOC_R5900();

			// repeat
			MOV32ItoR(EDX, 0x3);
			AND32RtoR(EDX, ECX);
			AND32ItoR(ECX, ~3);
			SHL32ItoR(EDX, 3); // *8

			PUSH32R(ECX);
			PUSH32R(EDX);
			CALLFunc( (int)recMemRead32 );
			POP32R(EDX);
			MOV8ItoR(EDI, 0xff);

			x86SetJ8(j8Ptr[1]);
		}

		_eeMoveGPRtoR(EBX, _Rt_);

		// oldmem is in EAX
		// mem >> SWL_SHIFT[shift]
		MOV32ItoR(ECX, 24);
		SUB32RtoR(ECX, EDX);
		SHR32CLtoR(EBX);

		// mov temp and compute _rt_ & SWL_MASK[shift]
		MOV32RtoR(ECX, EDX);
		MOV32ItoR(EDX, 0xffffff00);
		SHL32CLtoR(EDX);
		AND32RtoR(EAX, EDX);

		// combine
		OR32RtoR(EAX, EBX);

		POP32R(ECX);

		// read the old mem again
		TEST32RtoR(EDI, EDI);
		j8Ptr[0] = JNZ8(0);

		// manual write
		MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset);

		j8Ptr[1] = JMP8(0);
		x86SetJ8(j8Ptr[0]);

		CALLFunc( (int)recMemWrite32 );

		x86SetJ8(j8Ptr[1]);
	}
}

////////////////////////////////////////////////////
void recSWR( void ) 
{
#ifdef REC_SLOWWRITE
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		int shift = (g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&3;

		recMemConstRead32(EAX, (g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~3);

		_eeMoveGPRtoR(ECX, _Rt_);
		AND32ItoR(EAX, 0x00ffffff>>(24-shift*8));
		SHL32ItoR(ECX, shift*8);
		OR32RtoR(EAX, ECX);

		if( recMemConstWrite32((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~3, EAX) )
			recMemConstClear((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~3, 1);
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);
		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 0, 0);

		MOV32ItoR(EDX, 0x3);
		AND32RtoR(EDX, ECX);
		AND32ItoR(ECX, ~3);
		SHL32ItoR(EDX, 3); // *8

		PUSH32R(ECX);
		XOR32RtoR(EDI, EDI);

		MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset);

		if( dohw ) {
			j8Ptr[1] = JMP8(0);
			SET_HWLOC_R5900();

			// repeat
			MOV32ItoR(EDX, 0x3);
			AND32RtoR(EDX, ECX);
			AND32ItoR(ECX, ~3);
			SHL32ItoR(EDX, 3); // *8

			PUSH32R(ECX);
			PUSH32R(EDX);
			CALLFunc( (int)recMemRead32 );
			POP32R(EDX);
			MOV8ItoR(EDI, 0xff);

			x86SetJ8(j8Ptr[1]);
		}

		_eeMoveGPRtoR(EBX, _Rt_);

		// oldmem is in EAX
		// mem << SWR_SHIFT[shift]
		MOV32RtoR(ECX, EDX);
		SHL32CLtoR(EBX);

		// mov temp and compute _rt_ & SWR_MASK[shift]
		MOV32ItoR(ECX, 24);
		SUB32RtoR(ECX, EDX);
		MOV32ItoR(EDX, 0x00ffffff);
		SHR32CLtoR(EDX);
		AND32RtoR(EAX, EDX);

		// combine
		OR32RtoR(EAX, EBX);

		POP32R(ECX);
		
		// read the old mem again
		TEST32RtoR(EDI, EDI);
		j8Ptr[0] = JNZ8(0);

		// manual write
		MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset);

		j8Ptr[1] = JMP8(0);
		x86SetJ8(j8Ptr[0]);

		CALLFunc( (int)recMemWrite32 );

		x86SetJ8(j8Ptr[1]);
	}
}

void recSD( void ) { recStore(64, _Imm_, 1); }

////////////////////////////////////////////////////
void recSDR( void ) 
{
	iFlushCall(FLUSH_NOCONST);

	if( GPR_IS_CONST1( _Rs_ ) ) {
		// flush temporarily
		MOV32ItoM((int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], g_cpuConstRegs[_Rs_].UL[0]);
		//MOV32ItoM((int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], g_cpuConstRegs[_Rs_].UL[1]);
	}

	if( GPR_IS_CONST1( _Rt_ ) && !(g_cpuFlushedConstReg&(1<<_Rt_)) ) {
		MOV32ItoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], g_cpuConstRegs[_Rt_].UL[0]);
		MOV32ItoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], g_cpuConstRegs[_Rt_].UL[1]);
		g_cpuFlushedConstReg |= (1<<_Rt_);
	}

	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)R5900::Interpreter::OpcodeImpl::SDR );
}

////////////////////////////////////////////////////
void recSDL( void ) 
{
	iFlushCall(FLUSH_NOCONST);

	if( GPR_IS_CONST1( _Rs_ ) ) {
		// flush temporarily
		MOV32ItoM((int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], g_cpuConstRegs[_Rs_].UL[0]);
		//MOV32ItoM((int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], g_cpuConstRegs[_Rs_].UL[1]);
	}

	if( GPR_IS_CONST1( _Rt_ ) && !(g_cpuFlushedConstReg&(1<<_Rt_)) ) {
		MOV32ItoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], g_cpuConstRegs[_Rt_].UL[0]);
		MOV32ItoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], g_cpuConstRegs[_Rt_].UL[1]);
		g_cpuFlushedConstReg |= (1<<_Rt_);
	}

	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)R5900::Interpreter::OpcodeImpl::SDL );
}

////////////////////////////////////////////////////
void recSQ( void ) { recStore(128, _Imm_, 1); }

/*********************************************************
* Load and store for COP1                                *
* Format:  OP rt, offset(base)                           *
*********************************************************/

////////////////////////////////////////////////////
void recLWC1( void )
{
#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		int mmreg;
		assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_) % 4 == 0 );

		mmreg = _allocCheckFPUtoXMM(g_pCurInstInfo, _Rt_, MODE_WRITE);
		if( mmreg >= 0 ) mmreg |= MEM_XMMTAG;
		else mmreg = EAX;

		if( recMemConstRead32(mmreg, g_cpuConstRegs[_Rs_].UL[0]+_Imm_) ) {
			if( mmreg&MEM_XMMTAG ) xmmregs[mmreg&0xf].inuse = 0;
			mmreg = EAX;
		}

		if( !(mmreg&MEM_XMMTAG) )
			MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );
	}
	else
#endif
	{
		int dohw;
		int regt = _allocCheckFPUtoXMM(g_pCurInstInfo, _Rt_, MODE_WRITE);
		int mmregs = _eePrepareReg(_Rs_);

		_deleteMMXreg(MMX_FPU+_Rt_, 2);
		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 0, 0);

		if( regt >= 0 ) {
			SSEX_MOVD_RmOffset_to_XMM(regt, ECX, PS2MEM_BASE_+s_nAddMemOffset);
		}
		else {
			MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
			MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC_R5900();

			iMemRead32Check();
			CALLFunc( (int)recMemRead32 );

			if( regt >= 0 ) SSE2_MOVD_R_to_XMM(regt, EAX);
			else MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );

			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
			else x86SetJ8(j8Ptr[2]);
		}
	}
}

////////////////////////////////////////////////////
void recSWC1( void )
{
	int mmreg;

#ifdef REC_SLOWWRITE
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_)%4 == 0 );

		mmreg = _checkXMMreg(XMMTYPE_FPREG, _Rt_, MODE_READ);

		if( mmreg >= 0 ) mmreg |= MEM_XMMTAG|(_Rt_<<16);
		else {
			MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);
			mmreg = EAX;
		}

		recMemConstWrite32(g_cpuConstRegs[_Rs_].UL[0]+_Imm_, mmreg);
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);

		assert( _checkMMXreg(MMX_FPU+_Rt_, MODE_READ) == -1 );

		mmreg = _checkXMMreg(XMMTYPE_FPREG, _Rt_, MODE_READ);
		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 0, 0);

		if( mmreg >= 0) {
			SSEX_MOVD_XMM_to_RmOffset(ECX, mmreg, PS2MEM_BASE_+s_nAddMemOffset);
		}
		else {
			MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);
			MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset);
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC_R5900();

			// some type of hardware write
			if( mmreg >= 0) SSE2_MOVD_XMM_to_R(EAX, mmreg);
			else MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);

			CALLFunc( (int)recMemWrite32 );

			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
			else x86SetJ8(j8Ptr[2]);
		}
	}
}

////////////////////////////////////////////////////

#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_

void recLQC2( void ) 
{
	int mmreg;

#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_) % 16 == 0 );

		if( _Ft_ ) mmreg = _allocVFtoXMMreg(&VU0, -1, _Ft_, MODE_WRITE);
		else mmreg = _allocTempXMMreg(XMMT_FPS, -1);
		recMemConstRead128((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~15, mmreg);

		if( !_Ft_ ) _freeXMMreg(mmreg);
	}
	else
#endif
	{
		int dohw, mmregs;

		if( GPR_IS_CONST1( _Rs_ ) ) {
			_flushConstReg(_Rs_);
		}

		mmregs = _eePrepareReg(_Rs_);

		if( _Ft_ ) mmreg = _allocVFtoXMMreg(&VU0, -1, _Ft_, MODE_WRITE);

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 2, 0);

		if( _Ft_ ) {
			u8* rawreadptr = x86Ptr;

			if( mmreg >= 0 ) {
				SSEX_MOVDQARmtoROffset(mmreg, ECX, PS2MEM_BASE_+s_nAddMemOffset);
			}
			else {
				_recMove128RmOffsettoM((u32)&VU0.VF[_Ft_].UL[0], PS2MEM_BASE_+s_nAddMemOffset);
			}

			if( dohw ) {
				j8Ptr[1] = JMP8(0);
				SET_HWLOC_R5900();

				// check if writing to VUs
				CMP32ItoR(ECX, 0x11000000);
				JAE8(rawreadptr - (x86Ptr+2));

				PUSH32I( (int)&VU0.VF[_Ft_].UD[0] );
				CALLFunc( (int)recMemRead128 );
				if( mmreg >= 0 ) SSEX_MOVDQA_M128_to_XMM(mmreg, (int)&VU0.VF[_Ft_].UD[0] );
				ADD32ItoR(ESP, 4);
				
				x86SetJ8(j8Ptr[1]);
			}
		}
		else {
			if( dohw ) {
				j8Ptr[1] = JMP8(0);
				SET_HWLOC_R5900();

				PUSH32I( (int)&retValues[0] );
				CALLFunc( (int)recMemRead128 );
				ADD32ItoR(ESP, 4);
				
				x86SetJ8(j8Ptr[1]);
			}
		}
	}

	_clearNeededXMMregs(); // needed since allocing
}

////////////////////////////////////////////////////
void recSQC2( void ) 
{
	int mmreg;

#ifdef REC_SLOWWRITE
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_)%16 == 0 );

		mmreg = _allocVFtoXMMreg(&VU0, -1, _Ft_, MODE_READ)|MEM_XMMTAG;
		recMemConstWrite128((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~15, mmreg);
	}
	else
#endif
	{
		u8* rawreadptr;
		int dohw, mmregs;
		
		if( GPR_IS_CONST1( _Rs_ ) ) {
			_flushConstReg(_Rs_);
		}

		mmregs = _eePrepareReg(_Rs_);
		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 2, 0);

		rawreadptr = x86Ptr;

		if( (mmreg = _checkXMMreg(XMMTYPE_VFREG, _Ft_, MODE_READ)) >= 0) {
			SSEX_MOVDQARtoRmOffset(ECX, mmreg, PS2MEM_BASE_+s_nAddMemOffset);
		}
		else {	
			if( _hasFreeXMMreg() ) {
				mmreg = _allocTempXMMreg(XMMT_FPS, -1);
				SSEX_MOVDQA_M128_to_XMM(mmreg, (int)&VU0.VF[_Ft_].UD[0]);
				SSEX_MOVDQARtoRmOffset(ECX, mmreg, PS2MEM_BASE_+s_nAddMemOffset);
				_freeXMMreg(mmreg);
			}
			else if( _hasFreeMMXreg() ) {
				mmreg = _allocMMXreg(-1, MMX_TEMP, 0);
				MOVQMtoR(mmreg, (int)&VU0.VF[_Ft_].UD[0]);
				MOVQRtoRmOffset(ECX, mmreg, PS2MEM_BASE_+s_nAddMemOffset);
				MOVQMtoR(mmreg, (int)&VU0.VF[_Ft_].UL[2]);
				MOVQRtoRmOffset(ECX, mmreg, PS2MEM_BASE_+s_nAddMemOffset+8);
				SetMMXstate();
				_freeMMXreg(mmreg);
			}
			else {
				MOV32MtoR(EAX, (int)&VU0.VF[_Ft_].UL[0]);
				MOV32MtoR(EDX, (int)&VU0.VF[_Ft_].UL[1]);
				MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset);
				MOV32RtoRmOffset(ECX, EDX, PS2MEM_BASE_+s_nAddMemOffset+4);
				MOV32MtoR(EAX, (int)&VU0.VF[_Ft_].UL[2]);
				MOV32MtoR(EDX, (int)&VU0.VF[_Ft_].UL[3]);
				MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset+8);
				MOV32RtoRmOffset(ECX, EDX, PS2MEM_BASE_+s_nAddMemOffset+12);
			}
		}

		if( dohw ) {
			j8Ptr[1] = JMP8(0);

			SET_HWLOC_R5900();

			// check if writing to VUs
			CMP32ItoR(ECX, 0x11000000);
			JAE8(rawreadptr - (x86Ptr+2));

			// some type of hardware write
			if( (mmreg = _checkXMMreg(XMMTYPE_VFREG, _Ft_, MODE_READ)) >= 0) {

				if( xmmregs[mmreg].mode & MODE_WRITE ) {
					SSEX_MOVDQA_XMM_to_M128((int)&VU0.VF[_Ft_].UD[0], mmreg);
				}
			}

			MOV32ItoR(EAX, (int)&VU0.VF[_Ft_].UD[0]);
			CALLFunc( (int)recMemWrite128 );

			x86SetJ8A(j8Ptr[1]);
		}
	}
}

#else

using namespace Interpreter::OpcodeImpl;

PCSX2_ALIGNED16(u32 dummyValue[4]);

void SetFastMemory(int bSetFast)
{
	// nothing
}

//////////////////////////////////////////////////////////////////////////////////////////
//
void recLoad64( u32 bits, bool sign )
{
	jASSUME( bits == 64 || bits == 128 );

	//no int 3? i love to get my hands dirty ;p - Raz
	//write8(0xCC);

	// Load EDX with the destination.
	// 64/128 bit modes load the result directly into the cpuRegs.GPR struct.

	if( _Rt_ )
		MOV32ItoR(EDX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	else
		MOV32ItoR(EDX, (uptr)&dummyValue[0] );

	if( GPR_IS_CONST1( _Rs_ ) )
	{
		_eeOnLoadWrite(_Rt_);
		EEINST_RESETSIGNEXT(_Rt_); // remove the sign extension
		_deleteEEreg(_Rt_, 0);
		u32 srcadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		if( bits == 128 ) srcadr &= ~0x0f;
		vtlb_DynGenRead64_Const( bits, srcadr );
	}
	else
	{
		// Load ECX with the source memory address that we're reading from.
		_eeMoveGPRtoR(ECX, _Rs_);
		if ( _Imm_ != 0 )
			ADD32ItoR( ECX, _Imm_ );
		if( bits == 128 )		// force 16 byte alignment on 128 bit reads
			AND32ItoR(ECX,~0x0F);	// emitter automatically encodes this as an 8-bit sign-extended imm8

		_eeOnLoadWrite(_Rt_);
		EEINST_RESETSIGNEXT(_Rt_); // remove the sign extension
		_deleteEEreg(_Rt_, 0);

		vtlb_DynGenRead64(bits);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//
void recLoad32( u32 bits, bool sign )
{
	jASSUME( bits <= 32 );

	//no int 3? i love to get my hands dirty ;p - Raz
	//write8(0xCC);

	// 8/16/32 bit modes return the loaded value in EAX.

	if( GPR_IS_CONST1( _Rs_ ) )
	{
		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);

		u32 srcadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		vtlb_DynGenRead32_Const( bits, sign, srcadr );
	}
	else
	{
		// Load ECX with the source memory address that we're reading from.
		_eeMoveGPRtoR(ECX, _Rs_);
		if ( _Imm_ != 0 )
			ADD32ItoR( ECX, _Imm_ );

		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);
		vtlb_DynGenRead32(bits, sign);
	}

	if( _Rt_ )
	{
		// EAX holds the loaded value, so sign extend as needed:
		if (sign)
			CDQ();
		else
			XOR32RtoR(EDX,EDX);

		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//

// edxAlreadyAssigned - set to true if edx already holds the value being written (used by SWL/SWR)
void recStore(u32 sz, bool edxAlreadyAssigned=false)
{
	// Performance note: Const prop for the store address is good, always.
	// Constprop for the value being stored is not really worthwhile (better to use register
	// allocation -- simpler code and just as fast)

	// Load EDX first with the value being written, or the address of the value
	// being written (64/128 bit modes).  TODO: use register allocation, if the
	// value is allocated to a register.

	if( !edxAlreadyAssigned )
	{
		if( sz < 64 )
		{
			_eeMoveGPRtoR(EDX, _Rt_);
		}
		else if (sz==128 || sz==64)
		{
			_deleteEEreg(_Rt_, 1);		// flush register to mem
			MOV32ItoR(EDX,(int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
		}
	}

	// Load ECX with the destination address, or issue a direct optimized write
	// if the address is a constant propagation.

	if( GPR_IS_CONST1( _Rs_ ) )
	{
		u32 dstadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		if( sz == 128 ) dstadr &= ~0x0f;
		vtlb_DynGenWrite_Const( sz, dstadr );
	}
	else
	{
		_eeMoveGPRtoR(ECX, _Rs_);

		if ( _Imm_ != 0 )
			ADD32ItoR(ECX, _Imm_);
		if (sz==128)
			AND32ItoR(ECX,~0x0F);

		vtlb_DynGenWrite(sz);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//
void recLB( void )  { recLoad32(8,true); }
void recLBU( void ) { recLoad32(8,false); }
void recLH( void )  { recLoad32(16,true); }
void recLHU( void ) { recLoad32(16,false); }
void recLW( void )  { recLoad32(32,true); }
void recLWU( void ) { recLoad32(32,false); }
void recLD( void )  { recLoad64(64,false); }
void recLQ( void )  { recLoad64(128,false); }

void recSB( void )  { recStore(8); }
void recSH( void )  { recStore(16); }
void recSW( void )  { recStore(32); }
void recSQ( void )  { recStore(128); }
void recSD( void )  { recStore(64); }

//////////////////////////////////////////////////////////////////////////////////////////
// Non-recompiled Implementations Start Here -->
// (LWL/SWL, LWR/SWR, etc)

////////////////////////////////////////////////////
void recLWL( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 1);

	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	//MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)LWL );
}

////////////////////////////////////////////////////
void recLWR( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 1);

	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	//MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)LWR );   
}

static const u32 SWL_MASK[4] = { 0xffffff00, 0xffff0000, 0xff000000, 0x00000000 };
static const u32 SWR_MASK[4] = { 0x00000000, 0x000000ff, 0x0000ffff, 0x00ffffff };

static const u8 SWR_SHIFT[4] = { 0, 8, 16, 24 };
static const u8 SWL_SHIFT[4] = { 24, 16, 8, 0 };

////////////////////////////////////////////////////
void recSWL( void ) 
{
	// Perform a translated memory read, followed by a translated memory write
	// of the "merged" result.
	
	// NOTE: Code incomplete. I'll fix/finish it soon. --air
	if( 0 ) //GPR_IS_CONST1( _Rs_ ) )
	{
		_eeOnLoadWrite(_Rt_);
		//_deleteEEreg(_Rt_, 0);

		u32 addr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		u32 shift = addr & 3;
		vtlb_DynGenRead32_Const( 32, false, addr & 3 );

		// Prep eax/edx for producing the writeback result:
		// equiv to:  (cpuRegs.GPR.r[_Rt_].UL[0] >> SWL_SHIFT[shift]) | (mem & SWL_MASK[shift])

		//_deleteEEreg(_Rt_, 1);
		//MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );

		_eeMoveGPRtoR(EDX, _Rt_);
		AND32ItoR( EAX, SWL_MASK[shift] );
		SHR32ItoR( EDX, SWL_SHIFT[shift] );
		OR32RtoR( EDX, EAX );

		recStore( 32, true );
	}
	else
	{
		_deleteEEreg(_Rs_, 1);
		_deleteEEreg(_Rt_, 1);
		MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
		//MOV32ItoM( (int)&cpuRegs.pc, pc );	// pc's not needed by SWL
		CALLFunc( (int)SWL );
	}
}

////////////////////////////////////////////////////
void recSWR( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	//MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)SWR );
}

////////////////////////////////////////////////////
void recLDL( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
	EEINST_RESETSIGNEXT(_Rt_); // remove the sign extension
	_deleteEEreg(_Rt_, 1);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	//MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)LDL );
}

////////////////////////////////////////////////////
void recLDR( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
	EEINST_RESETSIGNEXT(_Rt_); // remove the sign extension
	_deleteEEreg(_Rt_, 1);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	//MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)LDR );
}

////////////////////////////////////////////////////
void recSDL( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	//MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)SDL );
}

////////////////////////////////////////////////////
void recSDR( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	//MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)SDR );
}

//////////////////////////////////////////////////////////////////////////////////////////
/*********************************************************
* Load and store for COP1                                *
* Format:  OP rt, offset(base)                           *
*********************************************************/

////////////////////////////////////////////////////
void recLWC1( void )
{
	_deleteEEreg(_Rs_, 1);
	_deleteFPtoXMMreg(_Rt_, 2);

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )	
		ADD32ItoR( ECX, _Imm_ );

	vtlb_DynGenRead32(32, false);
	MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );
}

////////////////////////////////////////////////////
void recSWC1( void )
{
	_deleteEEreg(_Rs_, 1);
	_deleteFPtoXMMreg(_Rt_, 0);

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )	
		ADD32ItoR( ECX, _Imm_ );

	MOV32MtoR(EDX, (int)&fpuRegs.fpr[ _Rt_ ].UL );
	vtlb_DynGenWrite(32);
}

////////////////////////////////////////////////////

/*********************************************************
* Load and store for COP2 (VU0 unit)                     *
* Format:  OP rt, offset(base)                           *
*********************************************************/

#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_

void recLQC2( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_deleteVFtoXMMreg(_Ft_, 0, 2);

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
		ADD32ItoR( ECX, _Imm_);

	if ( _Rt_ )
		MOV32ItoR(EDX, (int)&VU0.VF[_Ft_].UD[0] );
	else
		MOV32ItoR(EDX, (int)&dummyValue[0] );

	vtlb_DynGenRead64(128);
}

////////////////////////////////////////////////////
void recSQC2( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_deleteVFtoXMMreg(_Ft_, 0, 0);

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
		ADD32ItoR( ECX, _Imm_ );

	MOV32ItoR(EDX, (int)&VU0.VF[_Ft_].UD[0] );
	vtlb_DynGenWrite(128);
}

#endif

#endif

} } }	// end namespace R5900::Dynarec::OpcodeImpl

using namespace R5900::Dynarec;
using namespace R5900::Dynarec::OpcodeImpl;

#ifdef PCSX2_VIRTUAL_MEM

#ifndef LOADSTORE_RECOMPILE
	void SetFastMemory(int bSetFast) {}
#else
	static int s_bFastMemory = 0;
	void SetFastMemory(int bSetFast)
	{
		s_bFastMemory  = bSetFast;
		if( bSetFast ) {
			g_MemMasks0[0] = 0x00f0; g_MemMasks0[1] = 0x80f1; g_MemMasks0[2] = 0x00f0; g_MemMasks0[3] = 0x00f1;
			g_MemMasks8[0] = 0x0080; g_MemMasks8[1] = 0x8081; g_MemMasks8[2] = 0x0080; g_MemMasks8[3] = 0x0081;
			g_MemMasks16[0] = 0x0000; g_MemMasks16[1] = 0x8001; g_MemMasks16[2] = 0x0000; g_MemMasks16[3] = 0x0001;
		}
		else {
			g_MemMasks0[0] = 0x00f0; g_MemMasks0[1] = 0x80f1; g_MemMasks0[2] = 0x00f2; g_MemMasks0[3] = 0x00f3;
			g_MemMasks8[0] = 0x0080; g_MemMasks8[1] = 0x8081; g_MemMasks8[2] = 0x0082; g_MemMasks8[3] = 0x0083;
			g_MemMasks16[0] = 0x0000; g_MemMasks16[1] = 0x8001; g_MemMasks16[2] = 0x0002; g_MemMasks16[3] = 0x0003;
		}
	}
#endif

#else	// VTLB version

	void SetFastMemory(int bSetFast) {}

#endif
