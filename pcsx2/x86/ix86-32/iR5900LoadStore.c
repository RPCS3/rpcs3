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

PCSX2_ALIGNED16(u64 retValues[2]);
extern u32 maxrecmem;
static u32 s_bCachingMem = 0;
static u32 s_nAddMemOffset = 0;
static u32 s_tempaddr = 0;

void _eeOnLoadWrite(int reg)
{
	int regt;

	if( !reg ) return;

	_eeOnWriteReg(reg, 1);
	regt = _checkXMMreg(XMMTYPE_GPRREG, reg, MODE_READ);

	if( regt >= 0 ) {
		if( xmmregs[regt].mode & MODE_WRITE ) {
			if( cpucaps.hasStreamingSIMD2Extensions && (reg != _Rs_) ) {
				SSE2_PUNPCKHQDQ_XMM_to_XMM(regt, regt);
				SSE2_MOVQ_XMM_to_M64((u32)&cpuRegs.GPR.r[reg].UL[2], regt);
			}
			else SSE_MOVHPS_XMM_to_M64((u32)&cpuRegs.GPR.r[reg].UL[2], regt);
		}
		xmmregs[regt].inuse = 0;
	}
}

#ifdef PCSX2_VIRTUAL_MEM

extern void iMemRead32Check();

#define _Imm_co_ (*(s16*)PSM(pc))

// perf counters
#ifdef PCSX2_DEVBUILD
extern void StartPerfCounter();
extern void StopPerfCounter();
#else
#define StartPerfCounter()
#define StopPerfCounter()
#endif

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

#ifdef _DEBUG
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

#define SET_HWLOC() { \
	if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[2]); \
	else x86SetJ8(j8Ptr[0]); \
	if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[3]); \
	else x86SetJ8(j8Ptr[3]); \
	if (x86FpuState==MMX_STATE) { \
		if (cpucaps.has3DNOWInstructionExtensions) FEMMS(); \
		else EMMS(); \
	} \
	if( s_nAddMemOffset ) ADD32ItoR(ECX, s_nAddMemOffset); \
	if( s_bCachingMem & 4 ) AND32ItoR(ECX, 0x5fffffff); \
} \

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

static int s_bFastMemory = 0;
void SetFastMemory(int bSetFast)
{
	s_bFastMemory  = bSetFast;
	if( bSetFast) {
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

void assertmem()
{
	__asm mov s_tempaddr, ecx
	__asm mov s_bCachingMem, edx
	SysPrintf("%x(%x) not mem write!\n", s_tempaddr, s_bCachingMem);
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

int _eePrepareReg_coX(int gprreg, int num)
{
	int mmreg = _eePrepareReg(gprreg);

	if( (mmreg&MEM_MMXTAG) && num == 7 ) {
		if( mmxregs[mmreg&0xf].mode & MODE_WRITE ) {
			MOVQRtoM((u32)&cpuRegs.GPR.r[gprreg], mmreg&0xf);
			mmxregs[mmreg&0xf].mode &= ~MODE_WRITE;
			mmxregs[mmreg&0xf].needed = 0;
		}
	}
	
	return mmreg;
}

// returns true if should also include harware writes
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
		MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ regs ].UL[ 0 ] );		
	}

	if ( imm != 0 ) ADD32ItoR( ECX, imm );

	LoadCW();

#ifdef _DEBUG
	//CALLFunc((u32)testaddrs);
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
#ifdef _DEBUG
		MOV32RtoR(EAX, ECX);
		SHR32ItoR(EAX, 28);
		CMP32ItoR(EAX, 1);
		ptr = JNE8(0);
		MOV32ItoR(EDX, _Rs_);
		CALLFunc((u32)assertmem);
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

		OR16RtoR(EAX, EAX);

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

			SET_HWLOC();

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

void recLoad32_co(u32 bit, u32 sign)
{
	int nextrt = ((*(u32*)(PSM(pc)) >> 16) & 0x1F);
	int mmreg1 = -1, mmreg2 = -1;

#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		int ineax = 0;
		u32 written = 0;

		_eeOnLoadWrite(_Rt_);
		_eeOnLoadWrite(nextrt);

		if( bit == 32 ) {
			mmreg1 = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_WRITE);
			if( mmreg1 >= 0 ) mmreg1 |= MEM_MMXTAG;
			else mmreg1 = EBX;

			mmreg2 = _allocCheckGPRtoMMX(g_pCurInstInfo+1, nextrt, MODE_WRITE);
			if( mmreg2 >= 0 ) mmreg2 |= MEM_MMXTAG;
			else mmreg2 = EAX;
		}
		else {
			_deleteEEreg(_Rt_, 0);
			_deleteEEreg(nextrt, 0);
			mmreg1 = EBX;
			mmreg2 = EAX;
		}

		// do const processing
		switch(bit) {
			case 8:
				if( recMemConstRead8(mmreg1, g_cpuConstRegs[_Rs_].UL[0]+_Imm_, sign) ) {
					if( mmreg1&MEM_MMXTAG ) mmxregs[mmreg1&0xf].inuse = 0;
					if ( _Rt_ ) recTransferX86ToReg(EAX, _Rt_, sign);
					written = 1;
				}
				ineax = recMemConstRead8(mmreg2, g_cpuConstRegs[_Rs_].UL[0]+_Imm_co_, sign);
				break;
			case 16:
				if( recMemConstRead16(mmreg1, g_cpuConstRegs[_Rs_].UL[0]+_Imm_, sign) ) {
					if( mmreg1&MEM_MMXTAG ) mmxregs[mmreg1&0xf].inuse = 0;
					if ( _Rt_ ) recTransferX86ToReg(EAX, _Rt_, sign);
					written = 1;
				}
				ineax = recMemConstRead16(mmreg2, g_cpuConstRegs[_Rs_].UL[0]+_Imm_co_, sign);
				break;
			case 32:
				assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_) % 4 == 0 );
				if( recMemConstRead32(mmreg1, g_cpuConstRegs[_Rs_].UL[0]+_Imm_) ) {
					if( mmreg1&MEM_MMXTAG ) mmxregs[mmreg1&0xf].inuse = 0;
					if ( _Rt_ ) recTransferX86ToReg(EAX, _Rt_, sign);
					written = 1;
				}
				ineax = recMemConstRead32(mmreg2, g_cpuConstRegs[_Rs_].UL[0]+_Imm_co_);
				break;
		}

		if( !written && _Rt_ ) {
			if( mmreg1&MEM_MMXTAG ) {
				assert( mmxregs[mmreg1&0xf].mode & MODE_WRITE );
				if( sign ) _signExtendGPRtoMMX(mmreg1&0xf, _Rt_, 32-bit);
				else if( bit < 32 ) PSRLDItoR(mmreg1&0xf, 32-bit);
			}
			else recTransferX86ToReg(mmreg1, _Rt_, sign);
		}
		if( nextrt ) {
			g_pCurInstInfo++;
			if( !ineax && (mmreg2 & MEM_MMXTAG) ) {
				assert( mmxregs[mmreg2&0xf].mode & MODE_WRITE );
				if( sign ) _signExtendGPRtoMMX(mmreg2&0xf, nextrt, 32-bit);
				else if( bit < 32 ) PSRLDItoR(mmreg2&0xf, 32-bit);
			}
			else {
				if( mmreg2&MEM_MMXTAG ) mmxregs[mmreg2&0xf].inuse = 0;
				recTransferX86ToReg(mmreg2, nextrt, sign);
			}
			g_pCurInstInfo--;
		}
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);
		assert( !REC_FORCEMMX );

		_eeOnLoadWrite(_Rt_);
		_eeOnLoadWrite(nextrt);
		_deleteEEreg(_Rt_, 0);
		_deleteEEreg(nextrt, 0);

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 0, 0);

		switch(bit) {
			case 8:
				if( sign ) {
					MOVSX32Rm8toROffset(EBX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
					MOVSX32Rm8toROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				}
				else {
					MOVZX32Rm8toROffset(EBX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
					MOVZX32Rm8toROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				}
				break;
			case 16:
				if( sign ) {
					MOVSX32Rm16toROffset(EBX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
					MOVSX32Rm16toROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				}
				else {
					MOVZX32Rm16toROffset(EBX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
					MOVZX32Rm16toROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				}
				break;
			case 32:
				MOV32RmtoROffset(EBX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
				MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				break;
		}

		if ( _Rt_ ) recTransferX86ToReg(EBX, _Rt_, sign);

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);
			
			SET_HWLOC();

			switch(bit) {
				case 8:
					MOV32RtoM((u32)&s_tempaddr, ECX);
					CALLFunc( (int)recMemRead8 );
					if( sign ) MOVSX32R8toR(EAX, EAX);
					MOV32MtoR(ECX, (u32)&s_tempaddr);
					if ( _Rt_ ) recTransferX86ToReg(EAX, _Rt_, sign);
					
					ADD32ItoR(ECX, _Imm_co_-_Imm_);
					CALLFunc( (int)recMemRead8 );
					if( sign ) MOVSX32R8toR(EAX, EAX);
					break;
				case 16:
					MOV32RtoM((u32)&s_tempaddr, ECX);
					CALLFunc( (int)recMemRead16 );
					if( sign ) MOVSX32R16toR(EAX, EAX);
					MOV32MtoR(ECX, (u32)&s_tempaddr);
					if ( _Rt_ ) recTransferX86ToReg(EAX, _Rt_, sign);
					
					ADD32ItoR(ECX, _Imm_co_-_Imm_);
					CALLFunc( (int)recMemRead16 );
					if( sign ) MOVSX32R16toR(EAX, EAX);
					break;
				case 32:
					MOV32RtoM((u32)&s_tempaddr, ECX);
					iMemRead32Check();
					CALLFunc( (int)recMemRead32 );
					MOV32MtoR(ECX, (u32)&s_tempaddr);
					if ( _Rt_ ) recTransferX86ToReg(EAX, _Rt_, sign);
					
					ADD32ItoR(ECX, _Imm_co_-_Imm_);
					iMemRead32Check();
					CALLFunc( (int)recMemRead32 );
					break;
			}
			
			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
			else x86SetJ8(j8Ptr[2]);
		}

		if( nextrt ) {
			g_pCurInstInfo++;
			recTransferX86ToReg(EAX, nextrt, sign);
			g_pCurInstInfo--;
		}
	}
}

void recLB( void ) { recLoad32(8, _Imm_, 1); }
void recLB_co( void ) { recLoad32_co(8, 1); }
void recLBU( void ) { recLoad32(8, _Imm_, 0); }
void recLBU_co( void ) { recLoad32_co(8, 0); }
void recLH( void ) { recLoad32(16, _Imm_, 1); }
void recLH_co( void ) { recLoad32_co(16, 1); }
void recLHU( void ) { recLoad32(16, _Imm_, 0); }
void recLHU_co( void ) { recLoad32_co(16, 0); }
void recLW( void ) { recLoad32(32, _Imm_, 1); }
void recLW_co( void ) { recLoad32_co(32, 1); }
void recLWU( void ) { recLoad32(32, _Imm_, 0); }
void recLWU_co( void ) { recLoad32_co(32, 0); }

////////////////////////////////////////////////////

// paired with LWR
void recLWL_co(void) { recLoad32(32, _Imm_-3, 1); }

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
			SET_HWLOC();

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

// paired with LWL
void recLWR_co(void) { recLoad32(32, _Imm_, 1); }

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
			SET_HWLOC();

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

				SET_HWLOC();

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

				SET_HWLOC();

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

				SET_HWLOC();

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

void recLD_co( void )
{
#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		recLD();
		g_pCurInstInfo++;
		cpuRegs.code = *(u32*)PSM(pc);
		recLD();
		g_pCurInstInfo--; // incremented later
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);
		int mmreg1 = -1, mmreg2 = -1, t0reg = -1, t1reg = -1;
		int nextrt = ((*(u32*)(PSM(pc)) >> 16) & 0x1F);

		if( _Rt_ ) _eeOnWriteReg(_Rt_, 0);
		if( nextrt ) _eeOnWriteReg(nextrt, 0);

		if( _Rt_ ) {
			mmreg1 = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_WRITE);
			if( mmreg1 < 0 ) {
				mmreg1 = _allocCheckGPRtoXMM(g_pCurInstInfo, _Rt_, MODE_WRITE|MODE_READ);
				if( mmreg1 >= 0 ) mmreg1 |= 0x8000;
			}

			if( mmreg1 < 0 && _hasFreeMMXreg() ) mmreg1 = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_WRITE);
		}

		if( nextrt ) {
			mmreg2 = _allocCheckGPRtoMMX(g_pCurInstInfo+1, nextrt, MODE_WRITE);
			if( mmreg2 < 0 ) {
				mmreg2 = _allocCheckGPRtoXMM(g_pCurInstInfo+1, nextrt, MODE_WRITE|MODE_READ);
				if( mmreg2 >= 0 ) mmreg2 |= 0x8000;
			}

			if( mmreg2 < 0 && _hasFreeMMXreg() ) mmreg2 = _allocMMXreg(-1, MMX_GPR+nextrt, MODE_WRITE);
		}

		if( mmreg1 < 0 || mmreg2 < 0 ) {
			t0reg = _allocMMXreg(-1, MMX_TEMP, 0);

			if( mmreg1 < 0 && mmreg2 < 0 && _hasFreeMMXreg() ) {
				t1reg = _allocMMXreg(-1, MMX_TEMP, 0);
			}
			else t1reg = t0reg;
		}

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 1, 0);

		if( mmreg1 >= 0 ) {
			if( mmreg1 & 0x8000 ) SSE_MOVLPSRmtoROffset(mmreg1&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset);
			else MOVQRmtoROffset(mmreg1, ECX, PS2MEM_BASE_+s_nAddMemOffset);
		}
		else {
			if( _Rt_ ) {
				MOVQRmtoROffset(t0reg, ECX, PS2MEM_BASE_+s_nAddMemOffset);
				MOVQRtoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], t0reg);
			}
		}

		if( mmreg2 >= 0 ) {
			if( mmreg2 & 0x8000 ) SSE_MOVLPSRmtoROffset(mmreg2&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
			else MOVQRmtoROffset(mmreg2, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
		}
		else {
			if( nextrt ) {
				MOVQRmtoROffset(t1reg, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				MOVQRtoM((int)&cpuRegs.GPR.r[ nextrt ].UL[ 0 ], t1reg);
			}
		}
		
		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC();

			MOV32RtoM((u32)&s_tempaddr, ECX);

			if( mmreg1 >= 0 ) {
				PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
				CALLFunc( (int)recMemRead64 );

				if( mmreg1 & 0x8000 ) SSE_MOVLPS_M64_to_XMM(mmreg1&0xf, (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
				else MOVQMtoR(mmreg1, (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
			}
			else {
				if( _Rt_ ) {
					_deleteEEreg(_Rt_, 0);
					PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
				}
				else PUSH32I( (int)&retValues[0] );

				CALLFunc( (int)recMemRead64 );
			}

			MOV32MtoR(ECX, (u32)&s_tempaddr);

			if( mmreg2 >= 0 ) {
				MOV32ItoRmOffset(ESP, (int)&cpuRegs.GPR.r[ nextrt ].UD[ 0 ], 0 );
				ADD32ItoR(ECX, _Imm_co_-_Imm_);
				CALLFunc( (int)recMemRead64 );

				if( mmreg2 & 0x8000 ) SSE_MOVLPS_M64_to_XMM(mmreg2&0xf, (int)&cpuRegs.GPR.r[ nextrt ].UD[ 0 ]);
				else MOVQMtoR(mmreg2, (int)&cpuRegs.GPR.r[ nextrt ].UD[ 0 ]);
			}
			else {
				if( nextrt ) {
					_deleteEEreg(nextrt, 0);
					MOV32ItoRmOffset(ESP, (int)&cpuRegs.GPR.r[ nextrt ].UD[ 0 ], 0 );
				}
				else MOV32ItoRmOffset(ESP, (int)&retValues[0], 0 );

				ADD32ItoR(ECX, _Imm_co_-_Imm_);
				CALLFunc( (int)recMemRead64 );
			}

			ADD32ItoR(ESP, 4);
			
			if( s_bCachingMem & 2 ) x86SetJ32A(j32Ptr[4]);
			else x86SetJ8A(j8Ptr[2]);
		}

		if( mmreg1 < 0 || mmreg2 < 0 || !(mmreg1&0x8000) || !(mmreg2&0x8000) ) SetMMXstate();

		if( t0reg >= 0 ) _freeMMXreg(t0reg);
		if( t0reg != t1reg && t1reg >= 0 ) _freeMMXreg(t1reg);

		_clearNeededMMXregs();
		_clearNeededXMMregs();
	}
}

void recLD_coX( int num )
{
	int i;
	int mmreg = -1;
	int mmregs[XMMREGS];
	int nextrts[XMMREGS];

	assert( num > 1 && num < XMMREGS );

#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		recLD();

		for(i = 0; i < num; ++i) {
			g_pCurInstInfo++;
			cpuRegs.code = *(u32*)PSM(pc+i*4);
			recLD();
		}

		g_pCurInstInfo -= num; // incremented later
	}
	else
#endif
	{
		int dohw;
		int mmregS = _eePrepareReg_coX(_Rs_, num);

		if( _Rt_ ) _eeOnWriteReg(_Rt_, 0);
		for(i = 0; i < num; ++i) {
			nextrts[i] = ((*(u32*)(PSM(pc+i*4)) >> 16) & 0x1F);
			_eeOnWriteReg(nextrts[i], 0);
		}

		if( _Rt_ ) {
			mmreg = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_WRITE);
			if( mmreg < 0 ) {
				mmreg = _allocCheckGPRtoXMM(g_pCurInstInfo, _Rt_, MODE_WRITE|MODE_READ);
				if( mmreg >= 0 ) mmreg |= MEM_XMMTAG;
				else mmreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_WRITE)|MEM_MMXTAG;
			}
			else mmreg |= MEM_MMXTAG;
		}

		for(i = 0; i < num; ++i) {
			mmregs[i] = _allocCheckGPRtoMMX(g_pCurInstInfo+i+1, nextrts[i], MODE_WRITE);
			if( mmregs[i] < 0 ) {
				mmregs[i] = _allocCheckGPRtoXMM(g_pCurInstInfo+i+1, nextrts[i], MODE_WRITE|MODE_READ);
				if( mmregs[i] >= 0 ) mmregs[i] |= MEM_XMMTAG;
				else mmregs[i] = _allocMMXreg(-1, MMX_GPR+nextrts[i], MODE_WRITE)|MEM_MMXTAG;
			}
			else mmregs[i] |= MEM_MMXTAG;
		}

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregS, 1, 0);

		if( mmreg >= 0 ) {
			if( mmreg & MEM_XMMTAG ) SSE_MOVLPSRmtoROffset(mmreg&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset);
			else MOVQRmtoROffset(mmreg&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset);
		}

		for(i = 0; i < num; ++i) {
			u32 off = PS2MEM_BASE_+s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_;

			if( mmregs[i] >= 0 ) {
				if( mmregs[i] & MEM_XMMTAG ) SSE_MOVLPSRmtoROffset(mmregs[i]&0xf, ECX, off);
				else MOVQRmtoROffset(mmregs[i]&0xf, ECX, off);
			}
		}

		if( dohw ) {
			if( (s_bCachingMem & 2) || num > 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC();

			MOV32RtoM((u32)&s_tempaddr, ECX);

			if( mmreg >= 0 ) {
				PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
				CALLFunc( (int)recMemRead64 );

				if( mmreg & MEM_XMMTAG ) SSE_MOVLPS_M64_to_XMM(mmreg&0xf, (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
				else MOVQMtoR(mmreg&0xf, (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
			}
			else {
				PUSH32I( (int)&retValues[0] );
				CALLFunc( (int)recMemRead64 );
			}

			for(i = 0; i < num; ++i ) {
				MOV32MtoR(ECX, (u32)&s_tempaddr);
				ADD32ItoR(ECX, (*(s16*)PSM(pc+i*4))-_Imm_);

				if( mmregs[i] >= 0 ) {
					MOV32ItoRmOffset(ESP, (int)&cpuRegs.GPR.r[nextrts[i]].UL[0], 0);
					CALLFunc( (int)recMemRead64 );

					if( mmregs[i] & MEM_XMMTAG ) SSE_MOVLPS_M64_to_XMM(mmregs[i]&0xf, (int)&cpuRegs.GPR.r[ nextrts[i] ].UD[ 0 ]);
					else MOVQMtoR(mmregs[i]&0xf, (int)&cpuRegs.GPR.r[ nextrts[i] ].UD[ 0 ]);
				}
				else {
					MOV32ItoRmOffset(ESP, (int)&retValues[0], 0);
					CALLFunc( (int)recMemRead64 );
				}
			}

			ADD32ItoR(ESP, 4);
			
			if( (s_bCachingMem & 2) || num > 2 ) x86SetJ32A(j32Ptr[4]);
			else x86SetJ8A(j8Ptr[2]);
		}

		_clearNeededMMXregs();
		_clearNeededXMMregs();
	}
}

////////////////////////////////////////////////////
void recLDL_co(void) {
	recLoad64(_Imm_-7, 0); }

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
	CALLFunc( (int)LDL );
}

////////////////////////////////////////////////////
void recLDR_co(void) { recLoad64(_Imm_, 0); }

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
	CALLFunc( (int)LDR );
}

////////////////////////////////////////////////////
void recLQ( void ) 
{
	int mmreg = -1;
#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( cpucaps.hasStreamingSIMDExtensions && GPR_IS_CONST1( _Rs_ ) ) {
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
		
		if( !cpucaps.hasStreamingSIMDExtensions && GPR_IS_CONST1( _Rs_ ) )
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

				SET_HWLOC();

				PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
				CALLFunc( (int)recMemRead128 );

				if( mmreg >= 0 && (mmreg & MEM_MMXTAG) ) MOVQMtoR(mmreg&0xf, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
				else if( mmreg >= 0 && (mmreg & MEM_XMMTAG) ) SSEX_MOVDQA_M128_to_XMM(mmreg&0xf, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
				
				ADD32ItoR(ESP, 4);
			}
		}
		else {
			if( dohw ) {
				if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
				else j8Ptr[2] = JMP8(0);

				SET_HWLOC();

				PUSH32I( (int)&retValues[0] );
				CALLFunc( (int)recMemRead128 );
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

void recLQ_co( void )
{
#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		recLQ();
		g_pCurInstInfo++;
		cpuRegs.code = *(u32*)PSM(pc);
		recLQ();
		g_pCurInstInfo--; // incremented later
	}
	else
#endif
	{
		int dohw;
		int t0reg = -1;
		int mmregs = _eePrepareReg(_Rs_);

		int nextrt = ((*(u32*)(PSM(pc)) >> 16) & 0x1F);
		int mmreg1 = -1, mmreg2 = -1;

		if( _Rt_ ) {
			_eeOnWriteReg(_Rt_, 0);

			if( _hasFreeMMXreg() ) {
				mmreg1 = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_WRITE);
				if( mmreg1 >= 0 ) {
					mmreg1 |= MEM_MMXTAG;
					if( t0reg < 0 ) t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
				}
			}
			else _deleteMMXreg(MMX_GPR+_Rt_, 2);

			if( mmreg1 < 0 ) {
				mmreg1 = _allocGPRtoXMMreg(-1, _Rt_, MODE_WRITE);
				if( mmreg1 >= 0 ) mmreg1 |= MEM_XMMTAG;
			}
		}

		if( nextrt ) {
			_eeOnWriteReg(nextrt, 0);

			if( _hasFreeMMXreg() ) {
				mmreg2 = _allocCheckGPRtoMMX(g_pCurInstInfo+1, nextrt, MODE_WRITE);
				if( mmreg2 >= 0 ) {
					mmreg2 |= MEM_MMXTAG;
					if( t0reg < 0 ) t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
				}
			}
			else _deleteMMXreg(MMX_GPR+nextrt, 2);

			if( mmreg2 < 0 ) {
				mmreg2 = _allocGPRtoXMMreg(-1, nextrt, MODE_WRITE);
				if( mmreg2 >= 0 ) mmreg2 |= MEM_XMMTAG;
			}
		}

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 2, 0);

		if( _Rt_ ) {
			if( mmreg1 >= 0 && (mmreg1 & MEM_MMXTAG) ) {
				MOVQRmtoROffset(t0reg, ECX, PS2MEM_BASE_+s_nAddMemOffset+8);
				MOVQRmtoROffset(mmreg1&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset);
				MOVQRtoM((u32)&cpuRegs.GPR.r[_Rt_].UL[2], t0reg);
			}
			else if( mmreg1 >= 0 && (mmreg1 & MEM_XMMTAG) ) {
				SSEX_MOVDQARmtoROffset(mmreg1&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset);
			}
			else {
				_recMove128RmOffsettoM((u32)&cpuRegs.GPR.r[_Rt_].UL[0], PS2MEM_BASE_+s_nAddMemOffset);
			}
		}
			
		if( nextrt ) {
			if( mmreg2 >= 0 && (mmreg2 & MEM_MMXTAG) ) {
				MOVQRmtoROffset(t0reg, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_+8);
				MOVQRmtoROffset(mmreg2&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				MOVQRtoM((u32)&cpuRegs.GPR.r[nextrt].UL[2], t0reg);
			}
			else if( mmreg2 >= 0 && (mmreg2 & MEM_XMMTAG) ) {
				SSEX_MOVDQARmtoROffset(mmreg2&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
			}
			else {
				_recMove128RmOffsettoM((u32)&cpuRegs.GPR.r[nextrt].UL[0], PS2MEM_BASE_+s_nAddMemOffset);
			}
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC();

			MOV32RtoM((u32)&s_tempaddr, ECX);
			if( _Rt_ ) PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
			else PUSH32I( (int)&retValues[0] );
			CALLFunc( (int)recMemRead128 );

			MOV32MtoR(ECX, (u32)&s_tempaddr);
			if( nextrt ) MOV32ItoRmOffset(ESP, (int)&cpuRegs.GPR.r[nextrt].UL[0], 0);
			else MOV32ItoRmOffset(ESP, (int)&retValues[0], 0);
			ADD32ItoR(ECX, _Imm_co_-_Imm_);
			CALLFunc( (int)recMemRead128 );

			if( _Rt_) {
				if( mmreg1 >= 0 && (mmreg1 & MEM_MMXTAG) ) MOVQMtoR(mmreg1&0xf, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
				else if( mmreg1 >= 0 && (mmreg1 & MEM_XMMTAG) ) SSEX_MOVDQA_M128_to_XMM(mmreg1&0xf, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
			}
			if( nextrt ) {
				if( mmreg2 >= 0 && (mmreg2 & MEM_MMXTAG) ) MOVQMtoR(mmreg2&0xf, (int)&cpuRegs.GPR.r[ nextrt ].UL[ 0 ]);
				else if( mmreg2 >= 0 && (mmreg2 & MEM_XMMTAG) ) SSEX_MOVDQA_M128_to_XMM(mmreg2&0xf, (int)&cpuRegs.GPR.r[ nextrt ].UL[ 0 ] );
			}
			ADD32ItoR(ESP, 4);
			
			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
			else x86SetJ8(j8Ptr[2]);
		}

		if( t0reg >= 0 ) _freeMMXreg(t0reg);
	}
}

// coissues more than 2 LQs
void recLQ_coX(int num)
{
	int i;
	int mmreg = -1;
	int mmregs[XMMREGS];
	int nextrts[XMMREGS];

	assert( num > 1 && num < XMMREGS );

#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		recLQ();

		for(i = 0; i < num; ++i) {
			g_pCurInstInfo++;
			cpuRegs.code = *(u32*)PSM(pc+i*4);
			recLQ();
		}

		g_pCurInstInfo -= num; // incremented later
	}
	else
#endif
	{
		int dohw;
		int mmregS = _eePrepareReg_coX(_Rs_, num);


		if( _Rt_ ) _deleteMMXreg(MMX_GPR+_Rt_, 2);
		for(i = 0; i < num; ++i) {
			nextrts[i] = ((*(u32*)(PSM(pc+i*4)) >> 16) & 0x1F);
			if( nextrts[i] ) _deleteMMXreg(MMX_GPR+nextrts[i], 2);
		}
		
		if( _Rt_ ) {
			_eeOnWriteReg(_Rt_, 0);
			mmreg = _allocGPRtoXMMreg(-1, _Rt_, MODE_WRITE);
		}

		for(i = 0; i < num; ++i) {
			if( nextrts[i] ) {
				_eeOnWriteReg(nextrts[i], 0);
				mmregs[i] = _allocGPRtoXMMreg(-1, nextrts[i], MODE_WRITE);
			}
			else mmregs[i] = -1;
		}

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregS, 2, 1);

		if( _Rt_ ) SSEX_MOVDQARmtoROffset(mmreg, ECX, PS2MEM_BASE_+s_nAddMemOffset);

		for(i = 0; i < num; ++i) {
			u32 off = s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_;
			if( nextrts[i] ) SSEX_MOVDQARmtoROffset(mmregs[i], ECX, PS2MEM_BASE_+off&~0xf);
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC();

			MOV32RtoM((u32)&s_tempaddr, ECX);

			if( _Rt_ ) PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
			else PUSH32I( (int)&retValues[0] );
			CALLFunc( (int)recMemRead128 );
			if( _Rt_) SSEX_MOVDQA_M128_to_XMM(mmreg, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );

			for(i = 0; i < num; ++i) {
				MOV32MtoR(ECX, (u32)&s_tempaddr);
				ADD32ItoR(ECX, (*(s16*)PSM(pc+i*4))-_Imm_);

				if( nextrts[i] ) MOV32ItoRmOffset(ESP, (int)&cpuRegs.GPR.r[nextrts[i]].UL[0], 0);
				else MOV32ItoRmOffset(ESP, (int)&retValues[0], 0);
				CALLFunc( (int)recMemRead128 );		
				if( nextrts[i] ) SSEX_MOVDQA_M128_to_XMM(mmregs[i], (int)&cpuRegs.GPR.r[ nextrts[i] ].UL[ 0 ] );
			}

			ADD32ItoR(ESP, 4);
			
			if( s_bCachingMem & 2 ) x86SetJ32A(j32Ptr[4]);
			else x86SetJ8A(j8Ptr[2]);
		}
	}
}

////////////////////////////////////////////////////
extern void recClear64(BASEBLOCK* p);
extern void recClear128(BASEBLOCK* p);

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
		CALLFunc((u32)recClearMem);
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
		CALLFunc((u32)recClear64);
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
		CALLFunc((u32)recClear128);
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
				u32* ptr = (u32*)recAllocStackMem(8, 4);
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
	if( cpucaps.hasStreamingSIMDExtensions && GPR_IS_CONST1( _Rs_ ) ) {
		u32 addr = g_cpuConstRegs[_Rs_].UL[0]+imm;
		int doclear = 0;
		StopPerfCounter();
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

		StartPerfCounter();
	}
	else
#endif
	{
		int dohw;
		int mmregs;
		
		if( !cpucaps.hasStreamingSIMDExtensions && GPR_IS_CONST1( _Rs_ ) ) {
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
		if( bit <= 32 ) CALLFunc((u32)recWriteMemClear32);
		else if( bit == 64 ) CALLFunc((u32)recWriteMemClear64);
		else CALLFunc((u32)recWriteMemClear128);

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[5] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC();

			StopPerfCounter();
			recStore_call(bit, _Rt_, s_nAddMemOffset);
			StartPerfCounter();

			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[5]);
			else x86SetJ8(j8Ptr[2]);
		}

		if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
		else x86SetJ8(j8Ptr[1]);
	}

	_clearNeededMMXregs(); // needed since allocing
	_clearNeededXMMregs(); // needed since allocing
}

void recStore_co(int bit, int align)
{
	int nextrt = ((*(u32*)(PSM(pc)) >> 16) & 0x1F);

#ifdef REC_SLOWWRITE
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		u32 addr = g_cpuConstRegs[_Rs_].UL[0]+_Imm_;
		u32 coaddr = g_cpuConstRegs[_Rs_].UL[0]+_Imm_co_;
		int mmreg, t0reg = -1, mmreg2;
		int doclear = 0;

		switch(bit) {
			case 8:
				if( GPR_IS_CONST1(_Rt_) ) doclear |= recMemConstWrite8(addr, MEM_EECONSTTAG|(_Rt_<<16));
				else {
					_eeMoveGPRtoR(EAX, _Rt_);
					doclear |= recMemConstWrite8(addr, EAX);
				}
				if( GPR_IS_CONST1(nextrt) ) doclear |= recMemConstWrite8(coaddr, MEM_EECONSTTAG|(nextrt<<16));
				else {
					_eeMoveGPRtoR(EAX, nextrt);
					doclear |= recMemConstWrite8(coaddr, EAX);
				}
				break;
			case 16:
				assert( (addr)%2 == 0 );
				assert( (coaddr)%2 == 0 );

				if( GPR_IS_CONST1(_Rt_) ) doclear |= recMemConstWrite16(addr, MEM_EECONSTTAG|(_Rt_<<16));
				else {
					_eeMoveGPRtoR(EAX, _Rt_);
					doclear |= recMemConstWrite16(addr, EAX);
				}

				if( GPR_IS_CONST1(nextrt) ) doclear |= recMemConstWrite16(coaddr, MEM_EECONSTTAG|(nextrt<<16));
				else {
					_eeMoveGPRtoR(EAX, nextrt);
					doclear |= recMemConstWrite16(coaddr, EAX);
				}
				break;
			case 32:
				assert( (addr)%4 == 0 );
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

				if( GPR_IS_CONST1(nextrt) ) doclear |= recMemConstWrite32(coaddr, MEM_EECONSTTAG|(nextrt<<16));
				else if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, nextrt, MODE_READ)) >= 0 ) {
					doclear |= recMemConstWrite32(coaddr, mmreg|MEM_XMMTAG|(nextrt<<16));
				}
				else if( (mmreg = _checkMMXreg(MMX_GPR+nextrt, MODE_READ)) >= 0 ) {
					doclear |= recMemConstWrite32(coaddr, mmreg|MEM_MMXTAG|(nextrt<<16));
				}
				else {
					_eeMoveGPRtoR(EAX, nextrt);
					doclear |= recMemConstWrite32(coaddr, EAX);
				}

				break;
			case 64:
			{
				int mask = align ? ~7 : ~0;
				//assert( (addr)%8 == 0 );

				if( GPR_IS_CONST1(_Rt_) ) mmreg = MEM_EECONSTTAG|(_Rt_<<16);
				else if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG,  _Rt_, MODE_READ)) >= 0 ) {
					mmreg |= MEM_XMMTAG|(_Rt_<<16);
				}
				else mmreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_READ)|MEM_MMXTAG|(_Rt_<<16);

				if( GPR_IS_CONST1(nextrt) ) mmreg2 = MEM_EECONSTTAG|(nextrt<<16);
				else if( (mmreg2 = _checkXMMreg(XMMTYPE_GPRREG,  nextrt, MODE_READ)) >= 0 ) {
					mmreg2 |= MEM_XMMTAG|(nextrt<<16);
				}
				else mmreg2 = _allocMMXreg(-1, MMX_GPR+nextrt, MODE_READ)|MEM_MMXTAG|(nextrt<<16);

				doclear = recMemConstWrite64((addr)&mask, mmreg);
				doclear |= recMemConstWrite64((coaddr)&mask, mmreg2);
				doclear <<= 1;
				break;
			}
			case 128:
				assert( (addr)%16 == 0 );

				mmreg = _eePrepConstWrite128(_Rt_);
				mmreg2 = _eePrepConstWrite128(nextrt);
				doclear = recMemConstWrite128((addr)&~15, mmreg);
				doclear |= recMemConstWrite128((coaddr)&~15, mmreg2);
				doclear <<= 2;
				break;
		}

		if( doclear ) {
			u8* ptr;
			CMP32ItoM((u32)&maxrecmem, g_cpuConstRegs[_Rs_].UL[0]+(_Imm_ < _Imm_co_ ? _Imm_ : _Imm_co_));
			ptr = JB8(0);
			recMemConstClear((addr)&~(doclear*4-1), doclear);
			recMemConstClear((coaddr)&~(doclear*4-1), doclear);
			x86SetJ8A(ptr);
		}
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);
		int off = _Imm_co_-_Imm_;
		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, align ? bit/64 : 0, bit==128);

		recStore_raw(g_pCurInstInfo, bit, EAX, _Rt_, s_nAddMemOffset);
		recStore_raw(g_pCurInstInfo+1, bit, EBX, nextrt, s_nAddMemOffset+off);

		// clear the writes, do only one camera (with the lowest addr)
		if( off < 0 ) ADD32ItoR(ECX, s_nAddMemOffset+off);
		else if( s_nAddMemOffset ) ADD32ItoR(ECX, s_nAddMemOffset);
		CMP32MtoR(ECX, (u32)&maxrecmem);
		
		if( s_bCachingMem & 2 ) j32Ptr[5] = JAE32(0);
		else j8Ptr[1] = JAE8(0);

		MOV32RtoM((u32)&s_tempaddr, ECX);

		if( bit < 32 ) AND8ItoR(ECX, 0xfc);
		if( bit <= 32 ) CALLFunc((u32)recWriteMemClear32);
		else if( bit == 64 ) CALLFunc((u32)recWriteMemClear64);
		else CALLFunc((u32)recWriteMemClear128);

		MOV32MtoR(ECX, (u32)&s_tempaddr);
		if( off < 0 ) ADD32ItoR(ECX, -off);
		else ADD32ItoR(ECX, off);

		if( bit < 32 ) AND8ItoR(ECX, 0xfc);
		if( bit <= 32 ) CALLFunc((u32)recWriteMemClear32);
		else if( bit == 64 ) CALLFunc((u32)recWriteMemClear64);
		else CALLFunc((u32)recWriteMemClear128);
		
		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC();

			MOV32RtoM((u32)&s_tempaddr, ECX);
			recStore_call(bit, _Rt_, s_nAddMemOffset);
			MOV32MtoR(ECX, (u32)&s_tempaddr);
			recStore_call(bit, nextrt, s_nAddMemOffset+_Imm_co_-_Imm_);

			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
			else x86SetJ8(j8Ptr[2]);
		}

		if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[5]);
		else x86SetJ8(j8Ptr[1]);
	}

	_clearNeededMMXregs(); // needed since allocing
	_clearNeededXMMregs(); // needed since allocing
}

void recSB( void ) { recStore(8, _Imm_, 1); }
void recSB_co( void ) { recStore_co(8, 1); }
void recSH( void ) { recStore(16, _Imm_, 1); }
void recSH_co( void ) { recStore_co(16, 1); }
void recSW( void ) { recStore(32, _Imm_, 1); }
void recSW_co( void ) { recStore_co(32, 1); }

////////////////////////////////////////////////////
void recSWL_co(void) { recStore(32, _Imm_-3, 0); }

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
			SET_HWLOC();

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
void recSWR_co(void) { recStore(32, _Imm_, 0); }

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
			SET_HWLOC();

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
void recSD_co( void ) { recStore_co(64, 1); }

// coissues more than 2 SDs
void recSD_coX(int num, int align) 
{	
	int i;
	int mmreg = -1;
	int nextrts[XMMREGS];
	u32 mask = align ? ~7 : ~0;

	assert( num > 1 && num < XMMREGS );

	for(i = 0; i < num; ++i) {
		nextrts[i] = ((*(u32*)(PSM(pc+i*4)) >> 16) & 0x1F);
	}

#ifdef REC_SLOWWRITE
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		int minimm = _Imm_;
		int t0reg = -1;
		int doclear = 0;

		if( GPR_IS_CONST1(_Rt_) ) mmreg = MEM_EECONSTTAG|(_Rt_<<16);
		else if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG,  _Rt_, MODE_READ)) >= 0 ) {
			mmreg |= MEM_XMMTAG|(_Rt_<<16);
		}
		else mmreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_READ)|MEM_MMXTAG|(_Rt_<<16);
		doclear |= recMemConstWrite64((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&mask, mmreg);

		for(i = 0; i < num; ++i) {
			int imm = (*(s16*)PSM(pc+i*4));
			if( minimm > imm ) minimm = imm;

			if( GPR_IS_CONST1(nextrts[i]) ) mmreg = MEM_EECONSTTAG|(nextrts[i]<<16);
			else if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG,  nextrts[i], MODE_READ)) >= 0 ) {
				mmreg |= MEM_XMMTAG|(nextrts[i]<<16);
			}
			else mmreg = _allocMMXreg(-1, MMX_GPR+nextrts[i], MODE_READ)|MEM_MMXTAG|(nextrts[i]<<16);
			doclear |= recMemConstWrite64((g_cpuConstRegs[_Rs_].UL[0]+(*(s16*)PSM(pc+i*4)))&mask, mmreg);
		}

		if( doclear ) {
			u32* ptr;
			CMP32ItoM((u32)&maxrecmem, g_cpuConstRegs[_Rs_].UL[0]+minimm);
			ptr = JB32(0);
			recMemConstClear((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~7, 4);

			for(i = 0; i < num; ++i) {
				recMemConstClear((g_cpuConstRegs[_Rs_].UL[0]+(*(s16*)PSM(pc+i*4)))&~7, 2);
			}
			x86SetJ32A(ptr);
		}
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg_coX(_Rs_, num);
		int minoff = 0;

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, align, 1);

		recStore_raw(g_pCurInstInfo, 64, EAX, _Rt_, s_nAddMemOffset);

		for(i = 0; i < num; ++i) {
			recStore_raw(g_pCurInstInfo+i+1, 64, EAX, nextrts[i], s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_);
		}

		// clear the writes
		minoff = _Imm_;
		for(i = 0; i < num; ++i) {
			if( minoff > (*(s16*)PSM(pc+i*4)) ) minoff = (*(s16*)PSM(pc+i*4));
		}

		if( s_nAddMemOffset || minoff != _Imm_ ) ADD32ItoR(ECX, s_nAddMemOffset+minoff-_Imm_);
		CMP32MtoR(ECX, (u32)&maxrecmem);
		if( s_bCachingMem & 2 ) j32Ptr[5] = JAE32(0);
		else j8Ptr[1] = JAE8(0);
		
		MOV32RtoM((u32)&s_tempaddr, ECX);
		if( minoff != _Imm_ ) ADD32ItoR(ECX, _Imm_-minoff);
		CALLFunc((u32)recWriteMemClear64);

		for(i = 0; i < num; ++i) {
			MOV32MtoR(ECX, (u32)&s_tempaddr);
			if( minoff != (*(s16*)PSM(pc+i*4)) ) ADD32ItoR(ECX, (*(s16*)PSM(pc+i*4))-minoff);
			CALLFunc((u32)recWriteMemClear64);
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC();

			MOV32RtoM((u32)&s_tempaddr, ECX);
			recStore_call(64, _Rt_, s_nAddMemOffset);
			for(i = 0; i < num; ++i) {
				MOV32MtoR(ECX, (u32)&s_tempaddr);
				recStore_call(64, nextrts[i], s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_);
			}

			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
			else x86SetJ8(j8Ptr[2]);
		}

		if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[5]);
		else x86SetJ8(j8Ptr[1]);

		_clearNeededMMXregs(); // needed since allocing
		_clearNeededXMMregs(); // needed since allocing
	}
}

////////////////////////////////////////////////////
void recSDL_co(void) { recStore(64, _Imm_-7, 0); }

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
	CALLFunc( (int)SDL );   
}

////////////////////////////////////////////////////
void recSDR_co(void) { recStore(64, _Imm_, 0); }

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
	CALLFunc( (int)SDR );
}

////////////////////////////////////////////////////
void recSQ( void ) { recStore(128, _Imm_, 1); }
void recSQ_co( void ) {	recStore_co(128, 1); }

// coissues more than 2 SQs
void recSQ_coX(int num) 
{	
	int i;
	int mmreg = -1;
	int nextrts[XMMREGS];

	assert( num > 1 && num < XMMREGS );

	for(i = 0; i < num; ++i) {
		nextrts[i] = ((*(u32*)(PSM(pc+i*4)) >> 16) & 0x1F);
	}

#ifdef REC_SLOWWRITE
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		int minimm = _Imm_;
		int t0reg = -1;
		int doclear = 0;

		mmreg = _eePrepConstWrite128(_Rt_);
		doclear |= recMemConstWrite128((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~15, mmreg);

		for(i = 0; i < num; ++i) {
			int imm = (*(s16*)PSM(pc+i*4));
			if( minimm > imm ) minimm = imm;

			mmreg = _eePrepConstWrite128(nextrts[i]);
			doclear |= recMemConstWrite128((g_cpuConstRegs[_Rs_].UL[0]+imm)&~15, mmreg);
		}

		if( doclear ) {
			u32* ptr;
			CMP32ItoM((u32)&maxrecmem, g_cpuConstRegs[_Rs_].UL[0]+minimm);
			ptr = JB32(0);
			recMemConstClear((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~15, 4);

			for(i = 0; i < num; ++i) {
				recMemConstClear((g_cpuConstRegs[_Rs_].UL[0]+(*(s16*)PSM(pc+i*4)))&~15, 4);
			}
			x86SetJ32A(ptr);
		}
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg_coX(_Rs_, num);
		int minoff = 0;

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 2, 1);

		recStore_raw(g_pCurInstInfo, 128, EAX, _Rt_, s_nAddMemOffset);

		for(i = 0; i < num; ++i) {
			recStore_raw(g_pCurInstInfo+i+1, 128, EAX, nextrts[i], s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_);
		}

		// clear the writes
		minoff = _Imm_;
		for(i = 0; i < num; ++i) {
			if( minoff > (*(s16*)PSM(pc+i*4)) ) minoff = (*(s16*)PSM(pc+i*4));
		}

		if( s_nAddMemOffset || minoff != _Imm_ ) ADD32ItoR(ECX, s_nAddMemOffset+minoff-_Imm_);
		CMP32MtoR(ECX, (u32)&maxrecmem);
		if( s_bCachingMem & 2 ) j32Ptr[5] = JAE32(0);
		else j8Ptr[1] = JAE8(0);
		
		MOV32RtoM((u32)&s_tempaddr, ECX);
		if( minoff != _Imm_ ) ADD32ItoR(ECX, _Imm_-minoff);
		CALLFunc((u32)recWriteMemClear128);

		for(i = 0; i < num; ++i) {
			MOV32MtoR(ECX, (u32)&s_tempaddr);
			if( minoff != (*(s16*)PSM(pc+i*4)) ) ADD32ItoR(ECX, (*(s16*)PSM(pc+i*4))-minoff);
			CALLFunc((u32)recWriteMemClear128);
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC();

			MOV32RtoM((u32)&s_tempaddr, ECX);
			recStore_call(128, _Rt_, s_nAddMemOffset);
			for(i = 0; i < num; ++i) {
				MOV32MtoR(ECX, (u32)&s_tempaddr);
				recStore_call(128, nextrts[i], s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_);
			}

			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
			else x86SetJ8(j8Ptr[2]);
		}

		if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[5]);
		else x86SetJ8(j8Ptr[1]);

		_clearNeededMMXregs(); // needed since allocing
		_clearNeededXMMregs(); // needed since allocing
	}
}

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

			SET_HWLOC();

			iMemRead32Check();
			CALLFunc( (int)recMemRead32 );

			if( regt >= 0 ) SSE2_MOVD_R_to_XMM(regt, EAX);
			else MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );

			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
			else x86SetJ8(j8Ptr[2]);
		}
	}
}

void recLWC1_co( void )
{
	int nextrt = ((*(u32*)(PSM(pc)) >> 16) & 0x1F);

#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		u32 written = 0;
		int ineax, mmreg1, mmreg2;

		mmreg1 = _allocCheckFPUtoXMM(g_pCurInstInfo, _Rt_, MODE_WRITE);
		if( mmreg1 >= 0 ) mmreg1 |= MEM_XMMTAG;
		else mmreg1 = EBX;

		mmreg2 = _allocCheckFPUtoXMM(g_pCurInstInfo+1, nextrt, MODE_WRITE);
		if( mmreg2 >= 0 ) mmreg2 |= MEM_XMMTAG;
		else mmreg2 = EAX;

		assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_) % 4 == 0 );
		if( recMemConstRead32(mmreg1, g_cpuConstRegs[_Rs_].UL[0]+_Imm_) ) {
			if( mmreg1&MEM_XMMTAG ) xmmregs[mmreg1&0xf].inuse = 0;
			MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EBX );
			written = 1;
		}
		ineax = recMemConstRead32(mmreg2, g_cpuConstRegs[_Rs_].UL[0]+_Imm_co_);
		
		if( !written ) {
			if( !(mmreg1&MEM_XMMTAG) ) MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EBX );
		}
		
		if( ineax || !(mmreg2 & MEM_XMMTAG) ) {
			if( mmreg2&MEM_XMMTAG ) xmmregs[mmreg2&0xf].inuse = 0;
			MOV32RtoM( (int)&fpuRegs.fpr[ nextrt ].UL, EAX );
		}
	}
	else
#endif
	{
		int dohw;
		int regt, regtnext;
		int mmregs = _eePrepareReg(_Rs_);

		_deleteMMXreg(MMX_FPU+_Rt_, 2);
		regt = _allocCheckFPUtoXMM(g_pCurInstInfo, _Rt_, MODE_WRITE);
		regtnext = _allocCheckFPUtoXMM(g_pCurInstInfo+1, nextrt, MODE_WRITE);

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 0, 0);

		if( regt >= 0 ) {
			SSEX_MOVD_RmOffset_to_XMM(regt, ECX, PS2MEM_BASE_+s_nAddMemOffset);
		}
		else {
			MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
			MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );
		}

		if( regtnext >= 0 ) {
			SSEX_MOVD_RmOffset_to_XMM(regtnext, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
		}
		else {
			MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
			MOV32RtoM( (int)&fpuRegs.fpr[ nextrt ].UL, EAX );
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC();

			PUSH32R(ECX);
			CALLFunc( (int)recMemRead32 );
			POP32R(ECX);

			if( regt >= 0 ) {
				SSE2_MOVD_R_to_XMM(regt, EAX);
			}
			else {
				MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );
			}
			
			ADD32ItoR(ECX, _Imm_co_-_Imm_);
			CALLFunc( (int)recMemRead32 );

			if( regtnext >= 0 ) {
				SSE2_MOVD_R_to_XMM(regtnext, EAX);
			}
			else {
				MOV32RtoM( (int)&fpuRegs.fpr[ nextrt ].UL, EAX );
			}

			if( s_bCachingMem & 2 ) x86SetJ32A(j32Ptr[4]);
			else x86SetJ8A(j8Ptr[2]);
		}
	}
}

void recLWC1_coX(int num)
{
	int i;
	int mmreg = -1;
	int mmregs[XMMREGS];
	int nextrts[XMMREGS];

	assert( num > 1 && num < XMMREGS );

#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		int ineax;
		u32 written = 0;
		mmreg = _allocCheckFPUtoXMM(g_pCurInstInfo, _Rt_, MODE_WRITE);
		if( mmreg >= 0 ) mmreg |= MEM_XMMTAG;
		else mmreg = EAX;

		assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_) % 4 == 0 );
		if( recMemConstRead32(mmreg, g_cpuConstRegs[_Rs_].UL[0]+_Imm_) ) {
			if( mmreg&MEM_XMMTAG ) xmmregs[mmreg&0xf].inuse = 0;
			MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EBX );
			written = 1;
		}
		else if( !IS_XMMREG(mmreg) ) MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );

		// recompile two at a time
		for(i = 0; i < num-1; i += 2) {
			nextrts[0] = ((*(u32*)(PSM(pc+i*4)) >> 16) & 0x1F);
			nextrts[1] = ((*(u32*)(PSM(pc+i*4+4)) >> 16) & 0x1F);

			written = 0;
			mmregs[0] = _allocCheckFPUtoXMM(g_pCurInstInfo+i+1, nextrts[0], MODE_WRITE);
			if( mmregs[0] >= 0 ) mmregs[0] |= MEM_XMMTAG;
			else mmregs[0] = EBX;

			mmregs[1] = _allocCheckFPUtoXMM(g_pCurInstInfo+i+2, nextrts[1], MODE_WRITE);
			if( mmregs[1] >= 0 ) mmregs[1] |= MEM_XMMTAG;
			else mmregs[1] = EAX;

			assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_) % 4 == 0 );
			if( recMemConstRead32(mmregs[0], g_cpuConstRegs[_Rs_].UL[0]+(*(s16*)PSM(pc+i*4))) ) {
				if( mmregs[0]&MEM_XMMTAG ) xmmregs[mmregs[0]&0xf].inuse = 0;
				MOV32RtoM( (int)&fpuRegs.fpr[ nextrts[0] ].UL, EBX );
				written = 1;
			}
			ineax = recMemConstRead32(mmregs[1], g_cpuConstRegs[_Rs_].UL[0]+(*(s16*)PSM(pc+i*4+4)));
			
			if( !written ) {
				if( !(mmregs[0]&MEM_XMMTAG) ) MOV32RtoM( (int)&fpuRegs.fpr[ nextrts[0] ].UL, EBX );
			}
			
			if( ineax || !(mmregs[1] & MEM_XMMTAG) ) {
				if( mmregs[1]&MEM_XMMTAG ) xmmregs[mmregs[1]&0xf].inuse = 0;
				MOV32RtoM( (int)&fpuRegs.fpr[ nextrts[1] ].UL, EAX );
			}
		}

		if( i < num ) {
			// one left
			int nextrt = ((*(u32*)(PSM(pc+i*4)) >> 16) & 0x1F);

			mmreg = _allocCheckFPUtoXMM(g_pCurInstInfo+i+1, nextrt, MODE_WRITE);
			if( mmreg >= 0 ) mmreg |= MEM_XMMTAG;
			else mmreg = EAX;

			assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_) % 4 == 0 );
			if( recMemConstRead32(mmreg, g_cpuConstRegs[_Rs_].UL[0]+(*(s16*)PSM(pc+i*4))) ) {
				if( mmreg&MEM_XMMTAG ) xmmregs[mmreg&0xf].inuse = 0;
				MOV32RtoM( (int)&fpuRegs.fpr[ nextrt ].UL, EBX );
				written = 1;
			}
			else if( !IS_XMMREG(mmreg) ) MOV32RtoM( (int)&fpuRegs.fpr[ nextrt ].UL, EAX );
		}
	}
	else
#endif
	{
		int dohw;
		int mmregS = _eePrepareReg_coX(_Rs_, num);


		_deleteMMXreg(MMX_FPU+_Rt_, 2);
		for(i = 0; i < num; ++i) {
			nextrts[i] = ((*(u32*)(PSM(pc+i*4)) >> 16) & 0x1F);
			_deleteMMXreg(MMX_FPU+nextrts[i], 2);
		}
		
		mmreg = _allocCheckFPUtoXMM(g_pCurInstInfo, _Rt_, MODE_WRITE);

		for(i = 0; i < num; ++i) {
			mmregs[i] = _allocCheckFPUtoXMM(g_pCurInstInfo+i+1, nextrts[i], MODE_WRITE);
		}

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregS, 0, 1);

		if( mmreg >= 0 ) {
			SSEX_MOVD_RmOffset_to_XMM(mmreg, ECX, PS2MEM_BASE_+s_nAddMemOffset);
		}
		else {
			MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
			MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );
		}

		for(i = 0; i < num; ++i) {
			if( mmregs[i] >= 0 ) {
				SSEX_MOVD_RmOffset_to_XMM(mmregs[i], ECX, PS2MEM_BASE_+s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_);
			}
			else {
				MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_);
				MOV32RtoM( (int)&fpuRegs.fpr[ nextrts[i] ].UL, EAX );
			}
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC();

			MOV32RtoM((u32)&s_tempaddr, ECX);
			CALLFunc( (int)recMemRead32 );

			if( mmreg >= 0 ) SSE2_MOVD_R_to_XMM(mmreg, EAX);
			else MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );
			
			for(i = 0; i < num; ++i) {
				MOV32MtoR(ECX, (u32)&s_tempaddr);
				ADD32ItoR(ECX, (*(s16*)PSM(pc+i*4))-_Imm_);
				CALLFunc( (int)recMemRead32 );

				if( mmregs[i] >= 0 ) SSE2_MOVD_R_to_XMM(mmregs[i], EAX);
				else MOV32RtoM( (int)&fpuRegs.fpr[ nextrts[i] ].UL, EAX );
			}

			if( s_bCachingMem & 2 ) x86SetJ32A(j32Ptr[4]);
			else x86SetJ8A(j8Ptr[2]);
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

			SET_HWLOC();

			// some type of hardware write
			if( mmreg >= 0) SSE2_MOVD_XMM_to_R(EAX, mmreg);
			else MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);

			CALLFunc( (int)recMemWrite32 );

			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
			else x86SetJ8(j8Ptr[2]);
		}
	}
}

void recSWC1_co( void )
{
	int nextrt = ((*(u32*)(PSM(pc)) >> 16) & 0x1F);

#ifdef REC_SLOWWRITE
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		int mmreg;
		assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_)%4 == 0 );

		mmreg = _checkXMMreg(XMMTYPE_FPREG, _Rt_, MODE_READ);
		if( mmreg >= 0 ) mmreg |= MEM_XMMTAG|(_Rt_<<16);
		else {
			MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);
			mmreg = EAX;
		}
		recMemConstWrite32(g_cpuConstRegs[_Rs_].UL[0]+_Imm_, mmreg);

		mmreg = _checkXMMreg(XMMTYPE_FPREG, nextrt, MODE_READ);
		if( mmreg >= 0 ) mmreg |= MEM_XMMTAG|(nextrt<<16);
		else {
			MOV32MtoR(EAX, (int)&fpuRegs.fpr[ nextrt ].UL);
			mmreg = EAX;
		}
		recMemConstWrite32(g_cpuConstRegs[_Rs_].UL[0]+_Imm_co_, mmreg);
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);

		int mmreg1, mmreg2;
		assert( _checkMMXreg(MMX_FPU+_Rt_, MODE_READ) == -1 );
		assert( _checkMMXreg(MMX_FPU+nextrt, MODE_READ) == -1 );

		mmreg1 = _checkXMMreg(XMMTYPE_FPREG, _Rt_, MODE_READ);
		mmreg2 = _checkXMMreg(XMMTYPE_FPREG, nextrt, MODE_READ);

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 0, 0);

		if(mmreg1 >= 0 ) {
			if( mmreg2 >= 0 ) {
				SSEX_MOVD_XMM_to_RmOffset(ECX, mmreg1, PS2MEM_BASE_+s_nAddMemOffset);
				SSEX_MOVD_XMM_to_RmOffset(ECX, mmreg2, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
			}
			else {
				MOV32MtoR(EAX, (int)&fpuRegs.fpr[ nextrt ].UL);
				SSEX_MOVD_XMM_to_RmOffset(ECX, mmreg1, PS2MEM_BASE_+s_nAddMemOffset);
				MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
			}
		}
		else {
			if( mmreg2 >= 0 ) {
				MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);
				SSEX_MOVD_XMM_to_RmOffset(ECX, mmreg2, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset);
			}
			else {
				MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);
				MOV32MtoR(EDX, (int)&fpuRegs.fpr[ nextrt ].UL);
				MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset);
				MOV32RtoRmOffset(ECX, EDX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
			}
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC();

			MOV32RtoM((u32)&s_tempaddr, ECX);

			// some type of hardware write
			if( mmreg1 >= 0) SSE2_MOVD_XMM_to_R(EAX, mmreg1);
			else MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);
			CALLFunc( (int)recMemWrite32 );

			MOV32MtoR(ECX, (u32)&s_tempaddr);
			ADD32ItoR(ECX, _Imm_co_-_Imm_);
			if( mmreg2 >= 0) SSE2_MOVD_XMM_to_R(EAX, mmreg2);
			else MOV32MtoR(EAX, (int)&fpuRegs.fpr[ nextrt ].UL);
			CALLFunc( (int)recMemWrite32 );

			if( s_bCachingMem & 2 ) x86SetJ32A(j32Ptr[4]);
			else x86SetJ8A(j8Ptr[2]);
		}
	}
}

void recSWC1_coX(int num)
{
	int i;
	int mmreg = -1;
	int mmregs[XMMREGS];
	int nextrts[XMMREGS];

	assert( num > 1 && num < XMMREGS );

	for(i = 0; i < num; ++i) {
		nextrts[i] = ((*(u32*)(PSM(pc+i*4)) >> 16) & 0x1F);
	}

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

		for(i = 0; i < num; ++i) {
			mmreg = _checkXMMreg(XMMTYPE_FPREG, nextrts[i], MODE_READ);
			if( mmreg >= 0 ) mmreg |= MEM_XMMTAG|(nextrts[i]<<16);
			else {
				MOV32MtoR(EAX, (int)&fpuRegs.fpr[ nextrts[i] ].UL);
				mmreg = EAX;
			}
			recMemConstWrite32(g_cpuConstRegs[_Rs_].UL[0]+(*(s16*)PSM(pc+i*4)), mmreg);
		}
	}
	else
#endif
	{
		int dohw;
		int mmregS = _eePrepareReg_coX(_Rs_, num);

		assert( _checkMMXreg(MMX_FPU+_Rt_, MODE_READ) == -1 );
		mmreg = _checkXMMreg(XMMTYPE_FPREG, _Rt_, MODE_READ);

		for(i = 0; i < num; ++i) {
			assert( _checkMMXreg(MMX_FPU+nextrts[i], MODE_READ) == -1 );
			mmregs[i] = _checkXMMreg(XMMTYPE_FPREG, nextrts[i], MODE_READ);
		}

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregS, 0, 1);

		if( mmreg >= 0) {
			SSEX_MOVD_XMM_to_RmOffset(ECX, mmreg, PS2MEM_BASE_+s_nAddMemOffset);
		}
		else {
			MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);
			MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset);
		}

		for(i = 0; i < num; ++i) {
			if( mmregs[i] >= 0) {
				SSEX_MOVD_XMM_to_RmOffset(ECX, mmregs[i], PS2MEM_BASE_+s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_);
			}
			else {
				MOV32MtoR(EAX, (int)&fpuRegs.fpr[ nextrts[i] ].UL);
				MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_);
			}
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC();

			MOV32RtoM((u32)&s_tempaddr, ECX);

			// some type of hardware write
			if( mmreg >= 0) SSE2_MOVD_XMM_to_R(EAX, mmreg);
			else MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);
			CALLFunc( (int)recMemWrite32 );

			for(i = 0; i < num; ++i) {
				MOV32MtoR(ECX, (u32)&s_tempaddr);
				ADD32ItoR(ECX, (*(s16*)PSM(pc+i*4))-_Imm_);
				if( mmregs[i] >= 0 && (xmmregs[mmregs[i]].mode&MODE_WRITE) ) SSE2_MOVD_XMM_to_R(EAX, mmregs[i]);
				else MOV32MtoR(EAX, (int)&fpuRegs.fpr[ nextrts[i] ].UL);
				CALLFunc( (int)recMemWrite32 );
			}

			if( s_bCachingMem & 2 ) x86SetJ32A(j32Ptr[4]);
			else x86SetJ8A(j8Ptr[2]);
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
	if( cpucaps.hasStreamingSIMDExtensions && GPR_IS_CONST1( _Rs_ ) ) {
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

		if( !cpucaps.hasStreamingSIMDExtensions && GPR_IS_CONST1( _Rs_ ) ) {
			_flushConstReg(_Rs_);
		}

		mmregs = _eePrepareReg(_Rs_);

		if( _Ft_ ) mmreg = _allocVFtoXMMreg(&VU0, -1, _Ft_, MODE_WRITE);

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 2, 0);

		if( _Ft_ ) {
			s8* rawreadptr = x86Ptr;

			if( mmreg >= 0 ) {
				SSEX_MOVDQARmtoROffset(mmreg, ECX, PS2MEM_BASE_+s_nAddMemOffset);
			}
			else {
				_recMove128RmOffsettoM((u32)&VU0.VF[_Ft_].UL[0], PS2MEM_BASE_+s_nAddMemOffset);
			}

			if( dohw ) {
				j8Ptr[1] = JMP8(0);
				SET_HWLOC();

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
				SET_HWLOC();

				PUSH32I( (int)&retValues[0] );
				CALLFunc( (int)recMemRead128 );
				ADD32ItoR(ESP, 4);
				
				x86SetJ8(j8Ptr[1]);
			}
		}
	}

	_clearNeededXMMregs(); // needed since allocing
}

void recLQC2_co( void ) 
{
	int mmreg1 = -1, mmreg2 = -1, t0reg = -1;
	int nextrt = ((*(u32*)(PSM(pc)) >> 16) & 0x1F);

#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_) % 16 == 0 );

		if( _Ft_ ) mmreg1 = _allocVFtoXMMreg(&VU0, -1, _Ft_, MODE_WRITE);
		else t0reg = _allocTempXMMreg(XMMT_FPS, -1);
		recMemConstRead128((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~15, mmreg1 >= 0 ? mmreg1 : t0reg);

		if( nextrt ) mmreg2 = _allocVFtoXMMreg(&VU0, -1, nextrt, MODE_WRITE);
		else if( t0reg < 0 ) t0reg = _allocTempXMMreg(XMMT_FPS, -1);
		recMemConstRead128((g_cpuConstRegs[_Rs_].UL[0]+_Imm_co_)&~15, mmreg2 >= 0 ? mmreg2 : t0reg);

		if( t0reg >= 0 ) _freeXMMreg(t0reg);
	}
	else
#endif
	{
		s8* rawreadptr;
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);

		if( _Ft_ ) mmreg1 = _allocVFtoXMMreg(&VU0, -1, _Ft_, MODE_WRITE);
		if( nextrt ) mmreg2 = _allocVFtoXMMreg(&VU0, -1, nextrt, MODE_WRITE);

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 2, 0);

		rawreadptr = x86Ptr;
		if( mmreg1 >= 0 ) SSEX_MOVDQARmtoROffset(mmreg1, ECX, PS2MEM_BASE_+s_nAddMemOffset);
		if( mmreg2 >= 0 ) SSEX_MOVDQARmtoROffset(mmreg2, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);

		if( dohw ) {
			j8Ptr[1] = JMP8(0);
			SET_HWLOC();

			// check if writing to VUs
			CMP32ItoR(ECX, 0x11000000);
			JAE8(rawreadptr - (x86Ptr+2));

			MOV32RtoM((u32)&s_tempaddr, ECX);

			if( _Ft_ ) PUSH32I( (int)&VU0.VF[_Ft_].UD[0] );
			else PUSH32I( (int)&retValues[0] );
			CALLFunc( (int)recMemRead128 );

			if( mmreg1 >= 0 ) SSEX_MOVDQA_M128_to_XMM(mmreg1, (int)&VU0.VF[_Ft_].UD[0] );
			
			MOV32MtoR(ECX, (u32)&s_tempaddr);
			ADD32ItoR(ECX, _Imm_co_-_Imm_);

			if( nextrt ) MOV32ItoRmOffset(ESP, (int)&VU0.VF[nextrt].UD[0], 0 );
			else MOV32ItoRmOffset(ESP, (int)&retValues[0], 0 );
			CALLFunc( (int)recMemRead128 );

			if( mmreg2 >= 0 ) SSEX_MOVDQA_M128_to_XMM(mmreg2, (int)&VU0.VF[nextrt].UD[0] );

			ADD32ItoR(ESP, 4);
			x86SetJ8(j8Ptr[1]);
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
	if( cpucaps.hasStreamingSIMDExtensions && GPR_IS_CONST1( _Rs_ ) ) {
		assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_)%16 == 0 );

		mmreg = _allocVFtoXMMreg(&VU0, -1, _Ft_, MODE_READ)|MEM_XMMTAG;
		recMemConstWrite128((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~15, mmreg);
	}
	else
#endif
	{
		s8* rawreadptr;
		int dohw, mmregs;
		
		if( cpucaps.hasStreamingSIMDExtensions && GPR_IS_CONST1( _Rs_ ) ) {
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

			SET_HWLOC();

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

void recSQC2_co( void ) 
{
	int nextrt = ((*(u32*)(PSM(pc)) >> 16) & 0x1F);
	int mmreg1, mmreg2;

#ifdef REC_SLOWWRITE
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_)%16 == 0 );

		mmreg1 = _allocVFtoXMMreg(&VU0, -1, _Ft_, MODE_READ)|MEM_XMMTAG;
		mmreg2 = _allocVFtoXMMreg(&VU0, -1, nextrt, MODE_READ)|MEM_XMMTAG;
		recMemConstWrite128((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~15, mmreg1);
		recMemConstWrite128((g_cpuConstRegs[_Rs_].UL[0]+_Imm_co_)&~15, mmreg2);
	}
	else
#endif
	{
		s8* rawreadptr;
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);

		mmreg1 = _checkXMMreg(XMMTYPE_VFREG, _Ft_, MODE_READ);
		mmreg2 = _checkXMMreg(XMMTYPE_VFREG, nextrt, MODE_READ);

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 2, 0);

		rawreadptr = x86Ptr;

		if( mmreg1 >= 0 ) {
			SSEX_MOVDQARtoRmOffset(ECX, mmreg1, PS2MEM_BASE_+s_nAddMemOffset);
		}
		else {	
			if( _hasFreeXMMreg() ) {
				mmreg1 = _allocTempXMMreg(XMMT_FPS, -1);
				SSEX_MOVDQA_M128_to_XMM(mmreg1, (int)&VU0.VF[_Ft_].UD[0]);
				SSEX_MOVDQARtoRmOffset(ECX, mmreg1, PS2MEM_BASE_+s_nAddMemOffset);
				_freeXMMreg(mmreg1);
			}
			else if( _hasFreeMMXreg() ) {
				mmreg1 = _allocMMXreg(-1, MMX_TEMP, 0);
				MOVQMtoR(mmreg1, (int)&VU0.VF[_Ft_].UD[0]);
				MOVQRtoRmOffset(ECX, mmreg1, PS2MEM_BASE_+s_nAddMemOffset);
				MOVQMtoR(mmreg1, (int)&VU0.VF[_Ft_].UL[2]);
				MOVQRtoRmOffset(ECX, mmreg1, PS2MEM_BASE_+s_nAddMemOffset+8);
				SetMMXstate();
				_freeMMXreg(mmreg1);
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

		if( mmreg2 >= 0 ) {
			SSEX_MOVDQARtoRmOffset(ECX, mmreg2, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
		}
		else {	
			if( _hasFreeXMMreg() ) {
				mmreg2 = _allocTempXMMreg(XMMT_FPS, -1);
				SSEX_MOVDQA_M128_to_XMM(mmreg2, (int)&VU0.VF[nextrt].UD[0]);
				SSEX_MOVDQARtoRmOffset(ECX, mmreg2, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				_freeXMMreg(mmreg2);
			}
			else if( _hasFreeMMXreg() ) {
				mmreg2 = _allocMMXreg(-1, MMX_TEMP, 0);
				MOVQMtoR(mmreg2, (int)&VU0.VF[nextrt].UD[0]);
				MOVQRtoRmOffset(ECX, mmreg2, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				MOVQMtoR(mmreg2, (int)&VU0.VF[nextrt].UL[2]);
				MOVQRtoRmOffset(ECX, mmreg2, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_+8);
				SetMMXstate();
				_freeMMXreg(mmreg2);
			}
			else {
				MOV32MtoR(EAX, (int)&VU0.VF[nextrt].UL[0]);
				MOV32MtoR(EDX, (int)&VU0.VF[nextrt].UL[1]);
				MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				MOV32RtoRmOffset(ECX, EDX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_+4);
				MOV32MtoR(EAX, (int)&VU0.VF[nextrt].UL[2]);
				MOV32MtoR(EDX, (int)&VU0.VF[nextrt].UL[3]);
				MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_+8);
				MOV32RtoRmOffset(ECX, EDX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_+12);
			}
		}

		if( dohw ) {
			j8Ptr[1] = JMP8(0);

			SET_HWLOC();

			// check if writing to VUs
			CMP32ItoR(ECX, 0x11000000);
			JAE8(rawreadptr - (x86Ptr+2));

			// some type of hardware write
			if( mmreg1 >= 0) {
				if( xmmregs[mmreg1].mode & MODE_WRITE ) {
					SSEX_MOVDQA_XMM_to_M128((int)&VU0.VF[_Ft_].UD[0], mmreg1);
				}
			}

			MOV32RtoM((u32)&s_tempaddr, ECX);

			MOV32ItoR(EAX, (int)&VU0.VF[_Ft_].UD[0]);
			CALLFunc( (int)recMemWrite128 );

			if( mmreg2 >= 0) {
				if( xmmregs[mmreg2].mode & MODE_WRITE ) {
					SSEX_MOVDQA_XMM_to_M128((int)&VU0.VF[nextrt].UD[0], mmreg2);
				}
			}

			MOV32MtoR(ECX, (u32)&s_tempaddr);
			ADD32ItoR(ECX, _Imm_co_-_Imm_);

			MOV32ItoR(EAX, (int)&VU0.VF[nextrt].UD[0]);
			CALLFunc( (int)recMemWrite128 );

			x86SetJ8A(j8Ptr[1]);
		}
	}
}

#else

PCSX2_ALIGNED16(u32 dummyValue[4]);

void SetFastMemory(int bSetFast)
{
	// nothing
}

////////////////////////////////////////////////////
void recLB( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 0);

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_ );
	}
	PUSH32I( (int)&dummyValue[0] );
	PUSH32R( EAX );
	
	CALLFunc( (int)memRead8 );
	ADD32ItoR( ESP, 8 );
	if ( _Rt_ ) 
	{
		u8* linkEnd;
		TEST32RtoR( EAX, EAX );
		linkEnd = JNZ8( 0 );
		MOV32MtoR( EAX, (int)&dummyValue[0] );
		MOVSX32R8toR( EAX, EAX );
		CDQ( );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
		x86SetJ8( linkEnd );
	}
}

////////////////////////////////////////////////////
void recLBU( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 0);

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_ );
	}
	PUSH32I( (int)&dummyValue[0] );
	PUSH32R( EAX );

	CALLFunc( (int)memRead8 );
	ADD32ItoR( ESP, 8 );
	if ( _Rt_ ) 
	{
		u8* linkEnd;
		TEST32RtoR( EAX, EAX );
		linkEnd = JNZ8( 0 );
		MOV32MtoR( EAX, (int)&dummyValue[0] );
		MOVZX32R8toR( EAX, EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
		x86SetJ8( linkEnd );
	}
}

////////////////////////////////////////////////////
void recLH( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 0);

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_ );
	}
	PUSH32I( (int)&dummyValue[0] );
	PUSH32R( EAX );

	CALLFunc( (int)memRead16 );
	ADD32ItoR( ESP, 8 );
	if ( _Rt_ )
	{
		u8* linkEnd;
		TEST32RtoR( EAX, EAX );
		linkEnd = JNZ8( 0 );
		MOV32MtoR( EAX, (int)&dummyValue[0]);
		MOVSX32R16toR( EAX, EAX );
		CDQ( );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
		x86SetJ8( linkEnd );
	}
}

////////////////////////////////////////////////////
void recLHU( void )
{
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 0);

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_ );
	}
	PUSH32I( (int)&dummyValue[0] );
	PUSH32R( EAX );
	CALLFunc( (int)memRead16 );
	ADD32ItoR( ESP, 8 );
	if ( _Rt_ ) 
	{
		u8* linkEnd;
		TEST32RtoR( EAX, EAX );
		linkEnd = JNZ8( 0 );
		MOV32MtoR( EAX, (int)&dummyValue[0] );
		MOVZX32R16toR( EAX, EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
		x86SetJ8( linkEnd );
	}
}

////////////////////////////////////////////////////
void recLW( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 0);

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_ );
	}

	PUSH32I( (int)&dummyValue[0]);
	PUSH32R( EAX );

	CALLFunc( (int)memRead32 );
	ADD32ItoR( ESP, 8 );

	if ( _Rt_ ) 
	{
		u8* linkEnd;
		TEST32RtoR( EAX, EAX );
		linkEnd = JNZ8( 0 );
		MOV32MtoR( EAX, (int)&dummyValue[0]);
		CDQ( );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
		x86SetJ8( linkEnd );
	}
}

////////////////////////////////////////////////////
void recLWU( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 0);

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_ );
	}

	PUSH32I( (int)&dummyValue[0]);
	PUSH32R( EAX );
	CALLFunc( (int)memRead32 );
	ADD32ItoR( ESP, 8 );
	if ( _Rt_ )
	{
		u8* linkEnd;
		TEST32RtoR( EAX, EAX );
		linkEnd = JNZ8( 0 );
		MOV32MtoR( EAX, (int)&dummyValue[0]);
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
		x86SetJ8( linkEnd );
	}
}

////////////////////////////////////////////////////
void recLWL( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 0);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)LWL );
}

////////////////////////////////////////////////////
void recLWR( void ) 
{
	iFlushCall(FLUSH_EVERYTHING);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)LWR );   
}

////////////////////////////////////////////////////
extern void MOV64RmtoR( x86IntRegType to, x86IntRegType from );

void recLD( void )
{
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
    EEINST_RESETSIGNEXT(_Rt_); // remove the sign extension
	_deleteEEreg(_Rt_, 0);

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_ );
	}

	if ( _Rt_ )
	{
		PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	}
	else
	{
		PUSH32I( (int)&dummyValue[0] );
	}
	PUSH32R( EAX );
	CALLFunc( (int)memRead64 );
	ADD32ItoR( ESP, 8 );
}

////////////////////////////////////////////////////
void recLDL( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
    EEINST_RESETSIGNEXT(_Rt_); // remove the sign extension
	_deleteEEreg(_Rt_, 0);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)LDL );
}

////////////////////////////////////////////////////
void recLDR( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
    EEINST_RESETSIGNEXT(_Rt_); // remove the sign extension
	_deleteEEreg(_Rt_, 0);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)LDR );
}

////////////////////////////////////////////////////
void recLQ( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
    EEINST_RESETSIGNEXT(_Rt_); // remove the sign extension
	_deleteEEreg(_Rt_, 0);

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_);
	}
	AND32ItoR( EAX, ~0xf );

	if ( _Rt_ )
	{
		PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	}
	else
	{
		PUSH32I( (int)&dummyValue[0] );
	}
	PUSH32R( EAX );
	CALLFunc( (int)memRead128 );
	ADD32ItoR( ESP, 8 );
   
}

////////////////////////////////////////////////////
void recSB( void )
{
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_);
	}
	PUSH32M( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	PUSH32R( EAX );
	CALLFunc( (int)memWrite8 );
	ADD32ItoR( ESP, 8 );   
}

////////////////////////////////////////////////////
void recSH( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_ );
	}
	PUSH32M( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	PUSH32R( EAX );
	CALLFunc( (int)memWrite16 );
	ADD32ItoR( ESP, 8 );   
}

////////////////////////////////////////////////////
void recSW( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_ );
	}

	PUSH32M( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	PUSH32R( EAX );
	CALLFunc( (int)memWrite32 );
	ADD32ItoR( ESP, 8 );
}

////////////////////////////////////////////////////
void recSWL( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)SWL );   
}

////////////////////////////////////////////////////
void recSWR( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)SWR );
}

////////////////////////////////////////////////////
void recSD( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_ );
	}

	PUSH32M( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	PUSH32M( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	PUSH32R( EAX );
	CALLFunc( (int)memWrite64 );
	ADD32ItoR( ESP, 12 );   
}

////////////////////////////////////////////////////
void recSDL( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)SDL );
}

////////////////////////////////////////////////////
void recSDR( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	CALLFunc( (int)SDR );
}

////////////////////////////////////////////////////
void recSQ( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_ );
	}
	AND32ItoR( EAX, ~0xf );

	PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	PUSH32R( EAX );
	CALLFunc( (int)memWrite128 );
	ADD32ItoR( ESP, 8 );
}

/*********************************************************
* Load and store for COP1                                *
* Format:  OP rt, offset(base)                           *
*********************************************************/

////////////////////////////////////////////////////
void recLWC1( void )
{
	_deleteEEreg(_Rs_, 1);
	_deleteFPtoXMMreg(_Rt_, 2);

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )	
	{
		ADD32ItoR( EAX, _Imm_ );
	}

	PUSH32I( (int)&fpuRegs.fpr[ _Rt_ ].UL );
	PUSH32R( EAX );
	CALLFunc( (int)memRead32 );
	ADD32ItoR( ESP, 8 );
}

////////////////////////////////////////////////////
void recSWC1( void )
{
	_deleteEEreg(_Rs_, 1);
	_deleteFPtoXMMreg(_Rt_, 0);

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )	
	{
		ADD32ItoR( EAX, _Imm_ );
	}

	PUSH32M( (int)&fpuRegs.fpr[ _Rt_ ].UL );
	PUSH32R( EAX );
	CALLFunc( (int)memWrite32 );
	ADD32ItoR( ESP, 8 );
}

////////////////////////////////////////////////////

#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_

void recLQC2( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_deleteVFtoXMMreg(_Ft_, 0, 2);

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_);
	}

	if ( _Rt_ )
	{
		PUSH32I( (int)&VU0.VF[_Ft_].UD[0] );
	}
	else
	{
		PUSH32I( (int)&dummyValue[0] );
	}
	PUSH32R( EAX );
	CALLFunc( (int)memRead128 );
	ADD32ItoR( ESP, 8 );   
}

////////////////////////////////////////////////////
void recSQC2( void ) 
{
	_deleteEEreg(_Rs_, 1);
	_deleteVFtoXMMreg(_Ft_, 0, 0);

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_ );
	}

	PUSH32I( (int)&VU0.VF[_Ft_].UD[0] );
	PUSH32R( EAX );
	CALLFunc( (int)memWrite128 );
	ADD32ItoR( ESP, 8 );
}

#endif

#endif

#endif // PCSX2_NORECBUILD
