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
#include "PrecompiledHeader.h"

#include "Misc.h"
#include "iR5900.h"
#include "Vif.h"
#include "VU.h"
#include "ix86/ix86.h"
#include "iR3000A.h"

#include <vector>

using namespace std;

u16 x86FpuState, iCWstate;
u16 g_mmxAllocCounter = 0;

// X86 caching
int g_x86checknext;

// use special x86 register allocation for ia32

void _initX86regs() {
	memzero_obj(x86regs);
	g_x86AllocCounter = 0;
	g_x86checknext = 0;
}

u32 _x86GetAddr(int type, int reg)
{
	switch(type&~X86TYPE_VU1) {
		case X86TYPE_GPR: return (u32)&cpuRegs.GPR.r[reg];
		case X86TYPE_VI: {
			//assert( reg < 16 || reg == REG_R );
			return (type&X86TYPE_VU1)?(u32)&VU1.VI[reg]:(u32)&VU0.VI[reg];
		}
		case X86TYPE_MEMOFFSET: return 0;
		case X86TYPE_VIMEMOFFSET: return 0;
		case X86TYPE_VUQREAD: return (type&X86TYPE_VU1)?(u32)&VU1.VI[REG_Q]:(u32)&VU0.VI[REG_Q];
		case X86TYPE_VUPREAD: return (type&X86TYPE_VU1)?(u32)&VU1.VI[REG_P]:(u32)&VU0.VI[REG_P];
		case X86TYPE_VUQWRITE: return (type&X86TYPE_VU1)?(u32)&VU1.q:(u32)&VU0.q;
		case X86TYPE_VUPWRITE: return (type&X86TYPE_VU1)?(u32)&VU1.p:(u32)&VU0.p;
		case X86TYPE_PSX: return (u32)&psxRegs.GPR.r[reg];
		case X86TYPE_PCWRITEBACK:
			return (u32)&g_recWriteback;
		case X86TYPE_VUJUMP:
			return (u32)&g_recWriteback;

		jNO_DEFAULT;
	}

	return 0;
}

int _getFreeX86reg(int mode)
{
	int i, tempi;
	u32 bestcount = 0x10000;

	int maxreg = (mode&MODE_8BITREG)?4:X86REGS;

	for (i=0; i<X86REGS; i++) {
		int reg = (g_x86checknext+i)%X86REGS;
		if( reg == 0 || reg == ESP ) continue;
		if( reg >= maxreg ) continue;
		if( (mode&MODE_NOFRAME) && reg==EBP ) continue;

		if (x86regs[reg].inuse == 0) {
			g_x86checknext = (reg+1)%X86REGS;
			return reg;
		}
	}

	tempi = -1;
	for (i=1; i<maxreg; i++) {
		if( i == ESP ) continue;
		if( (mode&MODE_NOFRAME) && i==EBP ) continue;

		if (x86regs[i].needed) continue;
		if (x86regs[i].type != X86TYPE_TEMP) {

			if( x86regs[i].counter < bestcount ) {
				tempi = i;
				bestcount = x86regs[i].counter;
			}
			continue;
		}

		_freeX86reg(i);
		return i;
	}

	if( tempi != -1 ) {
		_freeX86reg(tempi);
		return tempi;
	}
	Console::Error("*PCSX2*: x86 error");
	assert(0);

	return -1;
}

void _flushCachedRegs()
{
	_flushConstRegs();
	_flushMMXregs();
	_flushXMMregs();
}

void _flushConstReg(int reg)
{
	if( GPR_IS_CONST1( reg ) && !(g_cpuFlushedConstReg&(1<<reg)) ) {
		MOV32ItoM((int)&cpuRegs.GPR.r[reg].UL[0], g_cpuConstRegs[reg].UL[0]);
		MOV32ItoM((int)&cpuRegs.GPR.r[reg].UL[1], g_cpuConstRegs[reg].UL[1]);
		g_cpuFlushedConstReg |= (1<<reg);
	}
}

void _flushConstRegs()
{
	int i;

	// flush constants

	// ignore r0
	for(i = 1; i < 32; ++i) {
		if( g_cpuHasConstReg & (1<<i) ) {
			
			if( !(g_cpuFlushedConstReg&(1<<i)) ) {
				MOV32ItoM((uptr)&cpuRegs.GPR.r[i].UL[0], g_cpuConstRegs[i].UL[0]);
				MOV32ItoM((uptr)&cpuRegs.GPR.r[i].UL[1], g_cpuConstRegs[i].UL[1]);
				g_cpuFlushedConstReg |= 1<<i;
			}
#if defined(_DEBUG)&&0
			else {
				// make sure the const regs are the same
				u8* ptemp[3];
				CMP32ItoM((u32)&cpuRegs.GPR.r[i].UL[0], g_cpuConstRegs[i].UL[0]);
				ptemp[0] = JNE8(0);
				if( EEINST_ISLIVE1(i) ) {
					CMP32ItoM((u32)&cpuRegs.GPR.r[i].UL[1], g_cpuConstRegs[i].UL[1]);
					ptemp[1] = JNE8(0);
				}
				ptemp[2] = JMP8(0);

				x86SetJ8( ptemp[0] );
				if( EEINST_ISLIVE1(i) ) x86SetJ8( ptemp[1] );
				CALLFunc((uptr)checkconstreg);

				x86SetJ8( ptemp[2] );
			}
#else
			if( g_cpuHasConstReg == g_cpuFlushedConstReg )
				break;
#endif
		}
	}
}

int _allocX86reg(int x86reg, int type, int reg, int mode)
{
	int i;
	assert( reg >= 0 && reg < 32 );

//	if( X86_ISVI(type) )
//		assert( reg < 16 || reg == REG_R );
	 
	// don't alloc EAX and ESP,EBP if MODE_NOFRAME
	int oldmode = mode;
	int noframe = mode&MODE_NOFRAME;
	int maxreg = (mode&MODE_8BITREG)?4:X86REGS;
	mode &= ~(MODE_NOFRAME|MODE_8BITREG);
	int readfromreg = -1;

	if( type != X86TYPE_TEMP ) {

		if( maxreg < X86REGS ) {
			// make sure reg isn't in the higher regs
			
			for(i = maxreg; i < X86REGS; ++i) {
				if (!x86regs[i].inuse || x86regs[i].type != type || x86regs[i].reg != reg) continue;

				if( mode & MODE_READ ) {
					readfromreg = i;
					x86regs[i].inuse = 0;
					break;
				}
				else if( mode & MODE_WRITE ) {
					x86regs[i].inuse = 0;
					break;
				}
			}
		}

		for (i=1; i<maxreg; i++) {
			if( i == ESP ) continue;

			if (!x86regs[i].inuse || x86regs[i].type != type || x86regs[i].reg != reg) continue;

			if( (noframe && i == EBP) || (i >= maxreg) ) {
				if( x86regs[i].mode & MODE_READ )
					readfromreg = i;
				//if( xmmregs[i].mode & MODE_WRITE ) mode |= MODE_WRITE;
				mode |= x86regs[i].mode&MODE_WRITE;
				x86regs[i].inuse = 0;
				break;
			}

			if( x86reg >= 0 ) {
				// requested specific reg, so return that instead
				if( i != x86reg ) {
					if( x86regs[i].mode & MODE_READ ) readfromreg = i;
					//if( x86regs[i].mode & MODE_WRITE ) mode |= MODE_WRITE;
					mode |= x86regs[i].mode&MODE_WRITE;
					x86regs[i].inuse = 0;
					break;
				}
			}

			if( type != X86TYPE_TEMP && !(x86regs[i].mode & MODE_READ) && (mode&MODE_READ)) {

				if( type == X86TYPE_GPR ) _flushConstReg(reg);
				
				if( X86_ISVI(type) && reg < 16 ) MOVZX32M16toR(i, _x86GetAddr(type, reg));
				else MOV32MtoR(i, _x86GetAddr(type, reg));

				x86regs[i].mode |= MODE_READ;
			}

			x86regs[i].needed = 1;
			x86regs[i].mode|= mode;
			return i;
		}
	}

	if (x86reg == -1) {
		x86reg = _getFreeX86reg(oldmode);
	}
	else {
		_freeX86reg(x86reg);
	}

	x86regs[x86reg].type = type;
	x86regs[x86reg].reg = reg;
	x86regs[x86reg].mode = mode;
	x86regs[x86reg].needed = 1;
	x86regs[x86reg].inuse = 1;

	if( mode & MODE_READ ) {
		if( readfromreg >= 0 ) MOV32RtoR(x86reg, readfromreg);
		else {
			if( type == X86TYPE_GPR ) {

				if( reg == 0 ) {
					XOR32RtoR(x86reg, x86reg);
				}
				else {
					_flushConstReg(reg);
					_deleteMMXreg(MMX_GPR+reg, 1);
					_deleteGPRtoXMMreg(reg, 1);
					
					_eeMoveGPRtoR(x86reg, reg);
					
					_deleteMMXreg(MMX_GPR+reg, 0);
					_deleteGPRtoXMMreg(reg, 0);
				}
			}
			else {
				if( X86_ISVI(type) && reg < 16 ) {
					if( reg == 0 ) XOR32RtoR(x86reg, x86reg);
					else MOVZX32M16toR(x86reg, _x86GetAddr(type, reg));
				}
				else MOV32MtoR(x86reg, _x86GetAddr(type, reg));
			}
		}
	}

	return x86reg;
}

int _checkX86reg(int type, int reg, int mode)
{
	int i;

	for (i=0; i<X86REGS; i++) {
		if (x86regs[i].inuse && x86regs[i].reg == reg && x86regs[i].type == type) {

			if( !(x86regs[i].mode & MODE_READ) && (mode&MODE_READ) ) {
				if( X86_ISVI(type) ) MOVZX32M16toR(i, _x86GetAddr(type, reg));
				else MOV32MtoR(i, _x86GetAddr(type, reg));
			}

			x86regs[i].mode |= mode;
			x86regs[i].counter = g_x86AllocCounter++;
			x86regs[i].needed = 1;
			return i;
		}
	}

	return -1;
}

void _addNeededX86reg(int type, int reg)
{
	int i;

	for (i=0; i<X86REGS; i++) {
		if (!x86regs[i].inuse || x86regs[i].reg != reg || x86regs[i].type != type ) continue;

		x86regs[i].counter = g_x86AllocCounter++;
		x86regs[i].needed = 1;
	}
}

void _clearNeededX86regs() {
	int i;

	for (i=0; i<X86REGS; i++) {
		if (x86regs[i].needed ) {
			if( x86regs[i].inuse && (x86regs[i].mode&MODE_WRITE) )
				x86regs[i].mode |= MODE_READ;
		}
		x86regs[i].needed = 0;
	}
}

void _deleteX86reg(int type, int reg, int flush)
{
	int i;

	for (i=0; i<X86REGS; i++) {
		if (x86regs[i].inuse && x86regs[i].reg == reg && x86regs[i].type == type) {
			switch(flush) {
				case 0:
					_freeX86reg(i);
					break;
				case 1:
					if( x86regs[i].mode & MODE_WRITE) {

						if( X86_ISVI(type) && x86regs[i].reg < 16 ) MOV16RtoM(_x86GetAddr(type, x86regs[i].reg), i);
						else MOV32RtoM(_x86GetAddr(type, x86regs[i].reg), i);
						
						// get rid of MODE_WRITE since don't want to flush again
						x86regs[i].mode &= ~MODE_WRITE;
						x86regs[i].mode |= MODE_READ;
					}
					return;
				case 2:
					x86regs[i].inuse = 0;
					break;
			}
		}
	}
}

void _freeX86reg(int x86reg)
{
	assert( x86reg >= 0 && x86reg < X86REGS );

	if( x86regs[x86reg].inuse && (x86regs[x86reg].mode&MODE_WRITE) ) {
		x86regs[x86reg].mode &= ~MODE_WRITE;

		if( X86_ISVI(x86regs[x86reg].type) && x86regs[x86reg].reg < 16 ) {
			MOV16RtoM(_x86GetAddr(x86regs[x86reg].type, x86regs[x86reg].reg), x86reg);
		}
		else
			MOV32RtoM(_x86GetAddr(x86regs[x86reg].type, x86regs[x86reg].reg), x86reg);
	}

	x86regs[x86reg].inuse = 0;
}

void _freeX86regs() {
	int i;

	for (i=0; i<X86REGS; i++) {
		if (!x86regs[i].inuse) continue;

		_freeX86reg(i);
	}
}

// MMX Caching
_mmxregs mmxregs[8], s_saveMMXregs[8];
static int s_mmxchecknext = 0;

void _initMMXregs()
{
	memzero_obj(mmxregs);
	g_mmxAllocCounter = 0;
	s_mmxchecknext = 0;
}

__forceinline void* _MMXGetAddr(int reg)
{
	assert( reg != MMX_TEMP );
	
	if( reg == MMX_LO ) return &cpuRegs.LO;
	if( reg == MMX_HI ) return &cpuRegs.HI;
	if( reg == MMX_FPUACC ) return &fpuRegs.ACC;

	if( reg >= MMX_GPR && reg < MMX_GPR+32 ) return &cpuRegs.GPR.r[reg&31];
	if( reg >= MMX_FPU && reg < MMX_FPU+32 ) return &fpuRegs.fpr[reg&31];
	if( reg >= MMX_COP0 && reg < MMX_COP0+32 ) return &cpuRegs.CP0.r[reg&31];
	
	assert( 0 );
	return NULL;
}

int  _getFreeMMXreg()
{
	int i, tempi;
	u32 bestcount = 0x10000;

	for (i=0; i<MMXREGS; i++) {
		if (mmxregs[(s_mmxchecknext+i)%MMXREGS].inuse == 0) {
			int ret = (s_mmxchecknext+i)%MMXREGS;
			s_mmxchecknext = (s_mmxchecknext+i+1)%MMXREGS;
			return ret;
		}
	}

	// check for dead regs
	for (i=0; i<MMXREGS; i++) {
		if (mmxregs[i].needed) continue;
		if (mmxregs[i].reg >= MMX_GPR && mmxregs[i].reg < MMX_GPR+34 ) {
			if( !(g_pCurInstInfo->regs[mmxregs[i].reg-MMX_GPR] & (EEINST_LIVE0|EEINST_LIVE1)) ) {
				_freeMMXreg(i);
				return i;
			}
			if( !(g_pCurInstInfo->regs[mmxregs[i].reg-MMX_GPR]&EEINST_USED) ) {
				_freeMMXreg(i);
				return i;
			}
		}
	}

	// check for future xmm usage
	for (i=0; i<MMXREGS; i++) {
		if (mmxregs[i].needed) continue;
		if (mmxregs[i].reg >= MMX_GPR && mmxregs[i].reg < MMX_GPR+34 ) {
			if( !(g_pCurInstInfo->regs[mmxregs[i].reg] & EEINST_MMX) ) {
				_freeMMXreg(i);
				return i;
			}
		}
	}

	tempi = -1;
	for (i=0; i<MMXREGS; i++) {
		if (mmxregs[i].needed) continue;
		if (mmxregs[i].reg != MMX_TEMP) {

			if( mmxregs[i].counter < bestcount ) {
				tempi = i;
				bestcount = mmxregs[i].counter;
			}
			continue;
		}

		_freeMMXreg(i);
		return i;
	}

	if( tempi != -1 ) {
		_freeMMXreg(tempi);
		return tempi;
	}
	Console::Error("*PCSX2*: mmx error");
	assert(0);

	return -1;
}

int _allocMMXreg(int mmxreg, int reg, int mode)
{
	int i;

	if( reg != MMX_TEMP ) {
		for (i=0; i<MMXREGS; i++) {
			if (mmxregs[i].inuse == 0 || mmxregs[i].reg != reg ) continue;

			if( MMX_ISGPR(reg)) {
				assert( _checkXMMreg(XMMTYPE_GPRREG, reg-MMX_GPR, 0) == -1 );
			}

			mmxregs[i].needed = 1;

			if( !(mmxregs[i].mode & MODE_READ) && (mode&MODE_READ) && reg != MMX_TEMP ) {

				SetMMXstate();
				if( reg == MMX_GPR ) {
					// moving in 0s
					PXORRtoR(i, i);
				}
				else {
					if( MMX_ISGPR(reg) ) _flushConstReg(reg-MMX_GPR);
					if( (mode & MODE_READHALF) || (MMX_IS32BITS(reg)&&(mode&MODE_READ)) )
						MOVDMtoMMX(i, (u32)_MMXGetAddr(reg));
					else {
						MOVQMtoR(i, (u32)_MMXGetAddr(reg));
					}
				}

				mmxregs[i].mode |= MODE_READ;
			}

			mmxregs[i].counter = g_mmxAllocCounter++;
			mmxregs[i].mode|= mode;
			return i;
		}
	}

	if (mmxreg == -1) {
		mmxreg = _getFreeMMXreg();
	}

	mmxregs[mmxreg].inuse = 1;
	mmxregs[mmxreg].reg = reg;
	mmxregs[mmxreg].mode = mode&~MODE_READHALF;
	mmxregs[mmxreg].needed = 1;
	mmxregs[mmxreg].counter = g_mmxAllocCounter++;

	SetMMXstate();
	if( reg == MMX_GPR ) {
		// moving in 0s
		PXORRtoR(mmxreg, mmxreg);
	}
	else {
		int xmmreg;
		if( MMX_ISGPR(reg) && (xmmreg = _checkXMMreg(XMMTYPE_GPRREG, reg-MMX_GPR, 0)) >= 0 ) {
			SSE_MOVHPS_XMM_to_M64((u32)_MMXGetAddr(reg)+8, xmmreg);
			if( mode & MODE_READ )
				SSE2_MOVDQ2Q_XMM_to_MM(mmxreg, xmmreg);

			if( xmmregs[xmmreg].mode & MODE_WRITE )
				mmxregs[mmxreg].mode |= MODE_WRITE;

			// don't flush
			xmmregs[xmmreg].inuse = 0;
		}
		else {
			if( MMX_ISGPR(reg) ) {
				if(mode&(MODE_READHALF|MODE_READ)) _flushConstReg(reg-MMX_GPR);
			}

			if( (mode & MODE_READHALF) || (MMX_IS32BITS(reg)&&(mode&MODE_READ)) ) {
				MOVDMtoMMX(mmxreg, (u32)_MMXGetAddr(reg));
			}
			else if( mode & MODE_READ ) {
				MOVQMtoR(mmxreg, (u32)_MMXGetAddr(reg));
			}
		}
	}

	return mmxreg;
}

int _checkMMXreg(int reg, int mode)
{
	int i;
	for (i=0; i<MMXREGS; i++) {
		if (mmxregs[i].inuse && mmxregs[i].reg == reg ) {

			if( !(mmxregs[i].mode & MODE_READ) && (mode&MODE_READ) ) {

				if( reg == MMX_GPR ) {
					// moving in 0s
					PXORRtoR(i, i);
				}
				else {
					if( MMX_ISGPR(reg) && (mode&(MODE_READHALF|MODE_READ)) ) _flushConstReg(reg-MMX_GPR);
					if( (mode & MODE_READHALF) || (MMX_IS32BITS(reg)&&(mode&MODE_READ)) )
						MOVDMtoMMX(i, (u32)_MMXGetAddr(reg));
					else
						MOVQMtoR(i, (u32)_MMXGetAddr(reg));
				}
				SetMMXstate();
			}

			mmxregs[i].mode |= mode;
			mmxregs[i].counter = g_mmxAllocCounter++;
			mmxregs[i].needed = 1;
			return i;
		}
	}

	return -1;
}

void _addNeededMMXreg(int reg)
{
	int i;

	for (i=0; i<MMXREGS; i++) {
		if (mmxregs[i].inuse == 0) continue;
		if (mmxregs[i].reg != reg) continue;

		mmxregs[i].counter = g_mmxAllocCounter++;
		mmxregs[i].needed = 1;
	}
}

void _clearNeededMMXregs()
{
	int i;

	for (i=0; i<MMXREGS; i++) {

		if( mmxregs[i].needed ) {

			// setup read to any just written regs
			if( mmxregs[i].inuse && (mmxregs[i].mode&MODE_WRITE) )
				mmxregs[i].mode |= MODE_READ;
			mmxregs[i].needed = 0;
		}
	}
}

// when flush is 0 - frees all of the reg, when flush is 1, flushes all of the reg, when
// it is 2, just stops using the reg (no flushing)
void _deleteMMXreg(int reg, int flush)
{
	int i;
	for (i=0; i<MMXREGS; i++) {

		if (mmxregs[i].inuse && mmxregs[i].reg == reg ) {

			switch(flush) {
				case 0:
					_freeMMXreg(i);
					break;
				case 1:
					if( mmxregs[i].mode & MODE_WRITE) {
						assert( mmxregs[i].reg != MMX_GPR );

						if( MMX_IS32BITS(reg) )
							MOVDMMXtoM((u32)_MMXGetAddr(mmxregs[i].reg), i);
						else
							MOVQRtoM((u32)_MMXGetAddr(mmxregs[i].reg), i);
						SetMMXstate();

						// get rid of MODE_WRITE since don't want to flush again
						mmxregs[i].mode &= ~MODE_WRITE;
						mmxregs[i].mode |= MODE_READ;
					}
					return;
				case 2:
					mmxregs[i].inuse = 0;
					break;
			}

			
			return;
		}
	}
}

int _getNumMMXwrite()
{
	int num = 0, i;
	for (i=0; i<MMXREGS; i++) {
		if( mmxregs[i].inuse && (mmxregs[i].mode&MODE_WRITE) ) ++num;
	}

	return num;
}

u8 _hasFreeMMXreg()
{
	int i;
	for (i=0; i<MMXREGS; i++) {
		if (!mmxregs[i].inuse) return 1;
	}

	// check for dead regs
	for (i=0; i<MMXREGS; i++) {
		if (mmxregs[i].needed) continue;
		if (mmxregs[i].reg >= MMX_GPR && mmxregs[i].reg < MMX_GPR+34 ) {
			if( !EEINST_ISLIVE64(mmxregs[i].reg-MMX_GPR) ) {
				return 1;
			}
		}
	}

	// check for dead regs
	for (i=0; i<MMXREGS; i++) {
		if (mmxregs[i].needed) continue;
		if (mmxregs[i].reg >= MMX_GPR && mmxregs[i].reg < MMX_GPR+34 ) {
			if( !(g_pCurInstInfo->regs[mmxregs[i].reg-MMX_GPR]&EEINST_USED) ) {
				return 1;
			}
		}
	}

	return 0;
}

void _freeMMXreg(int mmxreg)
{
	assert( mmxreg < MMXREGS );
	if (!mmxregs[mmxreg].inuse) return;
	
	if (mmxregs[mmxreg].mode & MODE_WRITE ) {

		if( mmxregs[mmxreg].reg >= MMX_GPR && mmxregs[mmxreg].reg < MMX_GPR+32 )
			assert( !(g_cpuHasConstReg & (1<<(mmxregs[mmxreg].reg-MMX_GPR))) );

		assert( mmxregs[mmxreg].reg != MMX_GPR );
		
		if( MMX_IS32BITS(mmxregs[mmxreg].reg) )
			MOVDMMXtoM((u32)_MMXGetAddr(mmxregs[mmxreg].reg), mmxreg);
		else
			MOVQRtoM((u32)_MMXGetAddr(mmxregs[mmxreg].reg), mmxreg);

		SetMMXstate();
	}

	mmxregs[mmxreg].mode &= ~MODE_WRITE;
	mmxregs[mmxreg].inuse = 0;
}

void _moveMMXreg(int mmxreg)
{
	int i;
	if( !mmxregs[mmxreg].inuse ) return;

	for (i=0; i<MMXREGS; i++) {
		if (mmxregs[i].inuse) continue;
		break;
	}

	if( i == MMXREGS ) {
		_freeMMXreg(mmxreg);
		return;
	}

	// move
	mmxregs[i] = mmxregs[mmxreg];
	mmxregs[mmxreg].inuse = 0;
	MOVQRtoR(i, mmxreg);
}

// write all active regs
void _flushMMXregs()
{
	int i;

	for (i=0; i<MMXREGS; i++) {
		if (mmxregs[i].inuse == 0) continue;

		if( mmxregs[i].mode & MODE_WRITE ) {

			assert( !(g_cpuHasConstReg & (1<<mmxregs[i].reg)) );

			assert( mmxregs[i].reg != MMX_TEMP );
			assert( mmxregs[i].mode & MODE_READ );
			assert( mmxregs[i].reg != MMX_GPR );

			if( MMX_IS32BITS(mmxregs[i].reg) )
				MOVDMMXtoM((u32)_MMXGetAddr(mmxregs[i].reg), i);
			else
				MOVQRtoM((u32)_MMXGetAddr(mmxregs[i].reg), i);
			SetMMXstate();

			mmxregs[i].mode &= ~MODE_WRITE;
			mmxregs[i].mode |= MODE_READ;
		}
	}
}

void _freeMMXregs()
{
	int i;
	for (i=0; i<MMXREGS; i++) {
		if (mmxregs[i].inuse == 0) continue;

		assert( mmxregs[i].reg != MMX_TEMP );
		assert( mmxregs[i].mode & MODE_READ );

		_freeMMXreg(i);
	}
}

void SetFPUstate() {
	_freeMMXreg(6);
	_freeMMXreg(7);

	if (x86FpuState==MMX_STATE) {
		if (cpucaps.has3DNOWInstructionExtensions) FEMMS();
		else EMMS();
		x86FpuState=FPU_STATE;
	}
}

__forceinline void _callPushArg(u32 arg, uptr argmem)
{
    if( IS_X86REG(arg) ) PUSH32R(arg&0xff);
    else if( IS_CONSTREG(arg) ) PUSH32I(argmem);
    else if( IS_GPRREG(arg) ) {
        SUB32ItoR(ESP, 4);
		_eeMoveGPRtoRm(ESP, arg&0xff);
    }
    else if( IS_XMMREG(arg) ) {
		SUB32ItoR(ESP, 4);
		SSEX_MOVD_XMM_to_Rm(ESP, arg&0xf);
	}
	else if( IS_MMXREG(arg) ) {
		SUB32ItoR(ESP, 4);
		MOVD32MMXtoRm(ESP, arg&0xf);
	}
	else if( IS_EECONSTREG(arg) ) {
		PUSH32I(g_cpuConstRegs[(arg>>16)&0x1f].UL[0]);
	}
	else if( IS_PSXCONSTREG(arg) ) {
		PUSH32I(g_psxConstRegs[(arg>>16)&0x1f]);
	}
    else if( IS_MEMORYREG(arg) ) PUSH32M(argmem);
    else {
        assert( (arg&0xfff0) == 0 );
        // assume it is a GPR reg
        PUSH32R(arg&0xf);
    }
}

__forceinline void _callFunctionArg1(uptr fn, u32 arg1, uptr arg1mem)
{
    _callPushArg(arg1, arg1mem);
    CALLFunc((uptr)fn);
    ADD32ItoR(ESP, 4);
}

__forceinline void _callFunctionArg2(uptr fn, u32 arg1, u32 arg2, uptr arg1mem, uptr arg2mem)
{
    _callPushArg(arg2, arg2mem);
    _callPushArg(arg1, arg1mem);
    CALLFunc((uptr)fn);
    ADD32ItoR(ESP, 8);
}

__forceinline void _callFunctionArg3(uptr fn, u32 arg1, u32 arg2, u32 arg3, uptr arg1mem, uptr arg2mem, uptr arg3mem)
{
    _callPushArg(arg3, arg3mem);
    _callPushArg(arg2, arg2mem);
    _callPushArg(arg1, arg1mem);
    CALLFunc((uptr)fn);
    ADD32ItoR(ESP, 12);
}

void _recPushReg(int mmreg)
{
	if( IS_XMMREG(mmreg) ) {
		SUB32ItoR(ESP, 4);
		SSEX_MOVD_XMM_to_Rm(ESP, mmreg&0xf);
	}
	else if( IS_MMXREG(mmreg) ) {
		SUB32ItoR(ESP, 4);
		MOVD32MMXtoRm(ESP, mmreg&0xf);
	}
	else if( IS_EECONSTREG(mmreg) ) {
		PUSH32I(g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0]);
	}
	else if( IS_PSXCONSTREG(mmreg) ) {
		PUSH32I(g_psxConstRegs[(mmreg>>16)&0x1f]);
	}
	else {
        assert( (mmreg&0xfff0) == 0 );
        PUSH32R(mmreg);
    }
}

void _signExtendSFtoM(u32 mem)
{
	LAHF();
	SAR16ItoR(EAX, 15);
	CWDE();
	MOV32RtoM(mem, EAX );
}

int _signExtendMtoMMX(x86MMXRegType to, u32 mem)
{
	int t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
	MOVDMtoMMX(t0reg, mem);
	MOVQRtoR(to, t0reg);
	PSRADItoR(t0reg, 31);
	PUNPCKLDQRtoR(to, t0reg);
	_freeMMXreg(t0reg);
	return to;
}

int _signExtendGPRMMXtoMMX(x86MMXRegType to, u32 gprreg, x86MMXRegType from, u32 gprfromreg)
{
	assert( to >= 0 && from >= 0 );
	if( !EEINST_ISLIVE1(gprreg) ) {
		EEINST_RESETHASLIVE1(gprreg);
		if( to != from ) MOVQRtoR(to, from);
		return to;
	}

	if( to == from ) return _signExtendGPRtoMMX(to, gprreg, 0);
	if( !(g_pCurInstInfo->regs[gprfromreg]&EEINST_LASTUSE) ) {
		if( EEINST_ISLIVE64(gprfromreg) ) {
			MOVQRtoR(to, from);
			return _signExtendGPRtoMMX(to, gprreg, 0);
		}
	}

	// from is free for use
	SetMMXstate();

	if( g_pCurInstInfo->regs[gprreg] & EEINST_MMX ) {
		
		if( EEINST_ISLIVE64(gprfromreg) ) {
			_freeMMXreg(from);
		}

		MOVQRtoR(to, from);
		PSRADItoR(from, 31);
		PUNPCKLDQRtoR(to, from);
		return to;
	}
	else {
		MOVQRtoR(to, from);
		MOVDMMXtoM((u32)&cpuRegs.GPR.r[gprreg].UL[0], from);
		PSRADItoR(from, 31);
		MOVDMMXtoM((u32)&cpuRegs.GPR.r[gprreg].UL[1], from);
		mmxregs[to].inuse = 0;
		return -1;
	}

	assert(0);
}

int _signExtendGPRtoMMX(x86MMXRegType to, u32 gprreg, int shift)
{
	assert( to >= 0 && shift >= 0 );
	if( !EEINST_ISLIVE1(gprreg) ) {
		if( shift > 0 ) PSRADItoR(to, shift);
		EEINST_RESETHASLIVE1(gprreg);
		return to;
	}

	SetMMXstate();

	if( g_pCurInstInfo->regs[gprreg] & EEINST_MMX ) {
		if( _hasFreeMMXreg() ) {
			int t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
			MOVQRtoR(t0reg, to);
			PSRADItoR(to, 31);
			if( shift > 0 ) PSRADItoR(t0reg, shift);
			PUNPCKLDQRtoR(t0reg, to);

			// swap mmx regs.. don't ask
			mmxregs[t0reg] = mmxregs[to];
			mmxregs[to].inuse = 0;
			return t0reg;
		}
		else {
			// will be used in the future as mmx
			if( shift > 0 ) PSRADItoR(to, shift);
			MOVDMMXtoM((u32)&cpuRegs.GPR.r[gprreg].UL[0], to);
			PSRADItoR(to, 31);
			MOVDMMXtoM((u32)&cpuRegs.GPR.r[gprreg].UL[1], to);

			// read again
			MOVQMtoR(to, (u32)&cpuRegs.GPR.r[gprreg].UL[0]);
			mmxregs[to].mode &= ~MODE_WRITE;
			return to;
		}
	}
	else {
		if( shift > 0 ) PSRADItoR(to, shift);
		MOVDMMXtoM((u32)&cpuRegs.GPR.r[gprreg].UL[0], to);
		PSRADItoR(to, 31);
		MOVDMMXtoM((u32)&cpuRegs.GPR.r[gprreg].UL[1], to);
		mmxregs[to].inuse = 0;
		return -1;
	}

	assert(0);
}

int _allocCheckGPRtoMMX(EEINST* pinst, int reg, int mode)
{
	if( pinst->regs[reg] & EEINST_MMX ) return _allocMMXreg(-1, MMX_GPR+reg, mode);
		
	return _checkMMXreg(MMX_GPR+reg, mode);
}

// fixme - yay stupid?  This sucks, and is used form iCOp2.cpp only.
// Surely there is a better way!
void _recMove128MtoM(u32 to, u32 from)
{
	MOV32MtoR(EAX, from);
	MOV32MtoR(EDX, from+4);
	MOV32RtoM(to, EAX);
	MOV32RtoM(to+4, EDX);
	MOV32MtoR(EAX, from+8);
	MOV32MtoR(EDX, from+12);
	MOV32RtoM(to+8, EAX);
	MOV32RtoM(to+12, EDX);
}

// fixme - see above function!
void _recMove128RmOffsettoM(u32 to, u32 offset)
{
	MOV32RmtoROffset(EAX, ECX, offset);
	MOV32RmtoROffset(EDX, ECX, offset+4);
	MOV32RtoM(to, EAX);
	MOV32RtoM(to+4, EDX);
	MOV32RmtoROffset(EAX, ECX, offset+8);
	MOV32RmtoROffset(EDX, ECX, offset+12);
	MOV32RtoM(to+8, EAX);
	MOV32RtoM(to+12, EDX);
}

// fixme - see above function again!
void _recMove128MtoRmOffset(u32 offset, u32 from)
{
	MOV32MtoR(EAX, from);
	MOV32MtoR(EDX, from+4);
	MOV32RtoRmOffset(ECX, EAX, offset);
	MOV32RtoRmOffset(ECX, EDX, offset+4);
	MOV32MtoR(EAX, from+8);
	MOV32MtoR(EDX, from+12);
	MOV32RtoRmOffset(ECX, EAX, offset+8);
	MOV32RtoRmOffset(ECX, EDX, offset+12);
}

static PCSX2_ALIGNED16(u32 s_ones[2]) = {0xffffffff, 0xffffffff};

void LogicalOpRtoR(x86MMXRegType to, x86MMXRegType from, int op)
{
	switch(op) {
		case 0: PANDRtoR(to, from); break;
		case 1: PORRtoR(to, from); break;
		case 2: PXORRtoR(to, from); break;
		case 3:
			PORRtoR(to, from);
			PXORMtoR(to, (u32)&s_ones[0]);
			break;
	}
}

void LogicalOpMtoR(x86MMXRegType to, u32 from, int op)
{
	switch(op) {
		case 0: PANDMtoR(to, from); break;
		case 1: PORMtoR(to, from); break;
		case 2: PXORMtoR(to, from); break;
		case 3:
			PORRtoR(to, from);
			PXORMtoR(to, (u32)&s_ones[0]);
			break;
	}
}

void LogicalOp32RtoM(u32 to, x86IntRegType from, int op)
{
	switch(op) {
		case 0: AND32RtoM(to, from); break;
		case 1: OR32RtoM(to, from); break;
		case 2: XOR32RtoM(to, from); break;
		case 3: OR32RtoM(to, from); break;
	}
}

void LogicalOp32MtoR(x86IntRegType to, u32 from, int op)
{
	switch(op) {
		case 0: AND32MtoR(to, from); break;
		case 1: OR32MtoR(to, from); break;
		case 2: XOR32MtoR(to, from); break;
		case 3: OR32MtoR(to, from); break;
	}
}

void LogicalOp32ItoR(x86IntRegType to, u32 from, int op)
{
	switch(op) {
		case 0: AND32ItoR(to, from); break;
		case 1: OR32ItoR(to, from); break;
		case 2: XOR32ItoR(to, from); break;
		case 3: OR32ItoR(to, from); break;
	}
}

void LogicalOp32ItoM(u32 to, u32 from, int op)
{
	switch(op) {
		case 0: AND32ItoM(to, from); break;
		case 1: OR32ItoM(to, from); break;
		case 2: XOR32ItoM(to, from); break;
		case 3: OR32ItoM(to, from); break;
	}
}
