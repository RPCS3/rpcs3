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
#include <malloc.h>

extern "C" {

#if defined(_WIN32)
#include <windows.h>
#endif

#include "PS2Etypes.h"
#include "System.h"
#include "R5900.h"
#include "Vif.h"
#include "VU.h"
#include "ix86/ix86.h"
#include "iCore.h"
#include "R3000A.h"

u16 x86FpuState, iCWstate;
u16 g_mmxAllocCounter = 0;

// X86 caching
extern _x86regs x86regs[X86REGS];
extern u16 g_x86AllocCounter;

} // end extern "C"

u32 g_recFnArgs[4];

#include <vector>
using namespace std;

// use special x86 register allocation for ia32
void _initX86regs() {
	memset(x86regs, 0, sizeof(x86regs));
	g_x86AllocCounter = 0;
}

uptr _x86GetAddr(int type, int reg)
{
	switch(type&~X86TYPE_VU1) {
		case X86TYPE_GPR: return (uptr)&cpuRegs.GPR.r[reg];
		case X86TYPE_VI: {
			//assert( reg < 16 || reg == REG_R );
			return (type&X86TYPE_VU1)?(uptr)&VU1.VI[reg]:(uptr)&VU0.VI[reg];
		}
		case X86TYPE_MEMOFFSET: return 0;
		case X86TYPE_VIMEMOFFSET: return 0;
		case X86TYPE_VUQREAD: return (type&X86TYPE_VU1)?(uptr)&VU1.VI[REG_Q]:(uptr)&VU0.VI[REG_Q];
		case X86TYPE_VUPREAD: return (type&X86TYPE_VU1)?(uptr)&VU1.VI[REG_P]:(uptr)&VU0.VI[REG_P];
		case X86TYPE_VUQWRITE: return (type&X86TYPE_VU1)?(uptr)&VU1.q:(uptr)&VU0.q;
		case X86TYPE_VUPWRITE: return (type&X86TYPE_VU1)?(uptr)&VU1.p:(uptr)&VU0.p;
		case X86TYPE_PSX: return (uptr)&psxRegs.GPR.r[reg];
		case X86TYPE_PCWRITEBACK:
			return (uptr)&g_recWriteback;
		case X86TYPE_VUJUMP:
			return (uptr)&g_recWriteback;
        case X86TYPE_FNARG:
            return (uptr)&g_recFnArgs[reg];
		default: assert(0);
	}

	return 0;
}

int _getFreeX86reg(int mode)
{
	int i, tempi;
	u32 bestcount = 0x10000;
    x86IntRegType* pregs = (mode&MODE_8BITREG)?g_x868bitregs:g_x86allregs;
	int maxreg = (mode&MODE_8BITREG)?ARRAYSIZE(g_x868bitregs):ARRAYSIZE(g_x86allregs);

    if( !(mode&MODE_8BITREG) && (mode&0x80000000) ) {
        // prioritize the temp registers
        for (i=0; i<ARRAYSIZE(g_x86tempregs); i++) {
            int reg = g_x86tempregs[i];
            if( (mode&MODE_NOFRAME) && reg==EBP ) continue;
            
            if (x86regs[reg].inuse == 0) {
                return reg;
            }
        }
    }

	for (i=0; i<maxreg; i++) {
		int reg = pregs[i];
		if( (mode&MODE_NOFRAME) && reg==EBP ) continue;

		if (x86regs[reg].inuse == 0) {
			return reg;
		}
	}

	tempi = -1;
	for (i=0; i<maxreg; i++) {
        int reg = pregs[i];
		if( (mode&MODE_NOFRAME) && reg==EBP ) continue;

		if (x86regs[reg].needed) continue;
		if (x86regs[reg].type != X86TYPE_TEMP) {

			if( x86regs[reg].counter < bestcount ) {
				tempi = reg;
				bestcount = x86regs[reg].counter;
			}
			continue;
		}

		_freeX86reg(reg);
		return i;
	}

	if( tempi != -1 ) {
		_freeX86reg(tempi);
		return tempi;
	}
	SysPrintf("*PCSX2*: x86 error\n");
	assert(0);

	return -1;
}

int _allocX86reg(int x86reg, int type, int reg, int mode)
{
	int i, j;
	assert( reg >= 0 && reg < 32 );

//	if( X86_ISVI(type) )
//		assert( reg < 16 || reg == REG_R );
	 
	// don't alloc EAX and ESP,EBP if MODE_NOFRAME
	int oldmode = mode;
	int noframe = mode&MODE_NOFRAME;
    int mode8bit  = mode&MODE_8BITREG;
    x86IntRegType* pregs = (mode&MODE_8BITREG)?g_x868bitregs:g_x86allregs;
	int maxreg = (mode&MODE_8BITREG)?ARRAYSIZE(g_x868bitregs):ARRAYSIZE(g_x86allregs);
	mode &= ~(MODE_NOFRAME|MODE_8BITREG);
	int readfromreg = -1;
    

	if( type != X86TYPE_TEMP ) {

		if( mode8bit ) {
			// make sure reg isn't in the non8bit regs
			
			for(j = 0; j < ARRAYSIZE(g_x86non8bitregs); ++j) {
                int i = g_x86non8bitregs[j];
				if (!x86regs[i].inuse || x86regs[i].type != type || x86regs[i].reg != reg)
                    continue;

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

		for (j=0; j<maxreg; j++) {
            i = pregs[j];

			if (!x86regs[i].inuse || x86regs[i].type != type || x86regs[i].reg != reg) continue;

			if( (noframe && i == EBP) ) {
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

				if( type == X86TYPE_GPR ) {

                    if( reg == 0 ) XOR64RtoR(i, i);
                    else {
                        if( GPR_IS_CONST1(reg) )
                            MOV64ItoR(i, g_cpuConstRegs[reg].UD[0]);
                        else
                            MOV64MtoR(i, _x86GetAddr(type, reg));
                    }
				}
				else if( X86_ISVI(type) && reg < 16 ) MOVZX32M16toR(i, _x86GetAddr(type, reg));
				else // the rest are 32 bit
                    MOV32MtoR(i, _x86GetAddr(type, reg));

				x86regs[i].mode |= MODE_READ;
			}

			x86regs[i].needed = 1;
			x86regs[i].mode|= mode;
			return i;
		}
	}

	// currently only gpr regs are const
	if( type == X86TYPE_GPR && (mode & MODE_WRITE) && reg < 32 ) {
		//assert( !(g_cpuHasConstReg & (1<<gprreg)) );
		g_cpuHasConstReg &= ~(1<<reg);
	}

	if (x86reg == -1) {
		x86reg = _getFreeX86reg(oldmode|(type==X86TYPE_TEMP?0x80000000:0));
	}
	else {
		_freeX86reg(x86reg);
	}

	x86regs[x86reg].type = type;
	x86regs[x86reg].reg = reg;
	x86regs[x86reg].mode = mode;
	x86regs[x86reg].needed = 1;
	x86regs[x86reg].inuse = 1;

    if( readfromreg >= 0 ) MOV64RtoR(x86reg, readfromreg);
    else {
        if( type == X86TYPE_GPR ) {

            if( reg == 0 ) {
                if( mode & MODE_READ )
                    XOR64RtoR(x86reg, x86reg);
                return x86reg;
            }

            int xmmreg;
            if( (xmmreg = _checkXMMreg(XMMTYPE_GPRREG, reg, 0)) >= 0 ) {
                // destroy the xmm reg, but don't flush
                SSE_MOVHPS_XMM_to_M64(_x86GetAddr(type, reg)+8, xmmreg);

                if( mode & MODE_READ )
                    SSE2_MOVQ_XMM_to_R(x86reg, xmmreg);
                
                if( xmmregs[xmmreg].mode & MODE_WRITE )
                    x86regs[x86reg].mode |= MODE_WRITE;
                
                // don't flush
                xmmregs[xmmreg].inuse = 0;
            }
            else {
                if( mode & MODE_READ ) {
                    if( GPR_IS_CONST1(reg) )
                        MOV64ItoR(x86reg, g_cpuConstRegs[reg].UD[0]);
                    else
                        MOV64MtoR(x86reg, _x86GetAddr(type, reg));
                }
            }
        }
        else if( mode & MODE_READ ) {
            if( X86_ISVI(type) && reg < 16 ) {
                if( reg == 0 ) XOR32RtoR(x86reg, x86reg);
                else MOVZX32M16toR(x86reg, _x86GetAddr(type, reg));
            }
            else MOV32MtoR(x86reg, _x86GetAddr(type, reg));
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
                if( type == X86TYPE_GPR ) {
                    if( reg == 0 ) XOR64RtoR(i, i);
                    else MOV64MtoR(i, _x86GetAddr(type, reg));
                }
				else if( X86_ISVI(type) ) MOVZX32M16toR(i, _x86GetAddr(type, reg));
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

            x86regs[i].needed = 0;
		}

        if( x86regs[i].inuse ) {
			assert( x86regs[i].type != X86TYPE_TEMP );
		}
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

                        if( type == X86TYPE_GPR ) MOV64RtoM(_x86GetAddr(type, x86regs[i].reg), i);
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
		
        if( x86regs[x86reg].type == X86TYPE_GPR )
            MOV64RtoM(_x86GetAddr(x86regs[x86reg].type, x86regs[x86reg].reg), x86reg);
		if( X86_ISVI(x86regs[x86reg].type) && x86regs[x86reg].reg < 16 )
			MOV16RtoM(_x86GetAddr(x86regs[x86reg].type, x86regs[x86reg].reg), x86reg);
		else
			MOV32RtoM(_x86GetAddr(x86regs[x86reg].type, x86regs[x86reg].reg), x86reg);
	}

    x86regs[x86reg].mode &= ~MODE_WRITE;
	x86regs[x86reg].inuse = 0;
}

void _flushX86regs()
{
	int i;

	for (i=0; i<X86REGS; i++) {
		if (x86regs[i].inuse == 0) continue;

		assert( x86regs[i].type != X86TYPE_TEMP );
		assert( x86regs[i].mode & (MODE_READ|MODE_WRITE) );

		_freeX86reg(i);
		x86regs[i].inuse = 1;
		x86regs[i].mode &= ~MODE_WRITE;
		x86regs[i].mode |= MODE_READ;
	}
}

void _freeX86regs() {
	int i;

	for (i=0; i<X86REGS; i++) {
		if (!x86regs[i].inuse) continue;

        assert( x86regs[i].type != X86TYPE_TEMP );
		_freeX86reg(i);
	}
}

//void _flushX86tempregs()
//{
//    int i, j;
//    
//	for (j=0; j<ARRAYSIZE(g_x86tempregs); j++) {
//        i = g_x86tempregs[j];
//		if (x86regs[i].inuse == 0) continue;
//
//		assert( x86regs[i].type != X86TYPE_TEMP );
//		assert( x86regs[i].mode & (MODE_READ|MODE_WRITE) );
//
//		_freeX86reg(i);
//		x86regs[i].inuse = 1;
//		x86regs[i].mode &= ~MODE_WRITE;
//		x86regs[i].mode |= MODE_READ;
//	}
//}

void _freeX86tempregs()
{
	int i, j;

	for (j=0; j<ARRAYSIZE(g_x86tempregs); j++) {
        i = g_x86tempregs[j];
		if (!x86regs[i].inuse) continue;

        assert( x86regs[i].type != X86TYPE_TEMP );
		_freeX86reg(i);
	}
}

u8 _hasFreeX86reg()
{
    int i, j;

	for (j=0; j<ARRAYSIZE(g_x86allregs); j++) {
        i = g_x86allregs[j];
		if (!x86regs[i].inuse) return 1;
	}

	// check for dead regs
	for (j=0; j<ARRAYSIZE(g_x86allregs); j++) {
        i = g_x86allregs[j];
		if (x86regs[i].needed) continue;
		if (x86regs[i].type == X86TYPE_GPR ) {
			if( !EEINST_ISLIVEXMM(x86regs[i].reg) ) {
				return 1;
			}
		}
	}

	// check for dead regs
	for (j=0; j<ARRAYSIZE(g_x86allregs); j++) {
        i = g_x86allregs[j];
		if (x86regs[i].needed) continue;
		if (x86regs[i].type == X86TYPE_GPR  ) {
			if( !(g_pCurInstInfo->regs[x86regs[i].reg]&EEINST_USED) ) {
				return 1;
			}
		}
	}
	return 0;
}

// EE
void _eeMoveGPRtoR(x86IntRegType to, int fromgpr)
{
	if( GPR_IS_CONST1(fromgpr) )
		MOV64ItoR( to, g_cpuConstRegs[fromgpr].UD[0] );
	else {
		int mmreg;

        if( (mmreg = _checkX86reg(X86TYPE_GPR, fromgpr, MODE_READ)) >= 0) {
            MOV64RtoR(to, mmreg);
        }
		if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, fromgpr, MODE_READ)) >= 0 && (xmmregs[mmreg].mode&MODE_WRITE)) {
			SSE2_MOVQ_XMM_to_R(to, mmreg);
		}
		else {
			MOV64MtoR(to, (uptr)&cpuRegs.GPR.r[ fromgpr ].UD[ 0 ] );
		}
	}
}

// 32 bit move
void _eeMoveGPRtoM(u32 to, int fromgpr)
{
	if( GPR_IS_CONST1(fromgpr) )
		MOV32ItoM( to, g_cpuConstRegs[fromgpr].UL[0] );
	else {
		int mmreg;
		
        if( (mmreg = _checkX86reg(X86TYPE_GPR, fromgpr, MODE_READ)) >= 0 ) {
            MOV32RtoM(to, mmreg);
        }
		if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, fromgpr, MODE_READ)) >= 0 ) {
			SSEX_MOVD_XMM_to_M32(to, mmreg);
		}
		else {
			MOV32MtoR(EAX, (uptr)&cpuRegs.GPR.r[ fromgpr ].UD[ 0 ] );
			MOV32RtoM(to, EAX );
		}
	}
}

void _eeMoveGPRtoRm(x86IntRegType to, int fromgpr)
{
	if( GPR_IS_CONST1(fromgpr) )
		MOV64ItoRmOffset( to, g_cpuConstRegs[fromgpr].UD[0], 0 );
	else {
		int mmreg;

        if( (mmreg = _checkX86reg(X86TYPE_GPR, fromgpr, MODE_READ)) >= 0 ) {
            MOV64RtoRmOffset(to, mmreg, 0);
        }		
		if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, fromgpr, MODE_READ)) >= 0 ) {
			SSEX_MOVD_XMM_to_Rm(to, mmreg);
		}
		else {
			MOV64MtoR(RAX, (uptr)&cpuRegs.GPR.r[ fromgpr ].UD[ 0 ] );
			MOV64RtoRmOffset(to, RAX, 0 );
		}
	}
}

void _callPushArg(u32 arg, uptr argmem, x86IntRegType X86ARG)
{
    if( IS_X86REG(arg) ) {
        if( (arg&0xff) != X86ARG ) {
            _freeX86reg(X86ARG);
            MOV64RtoR(X86ARG, (arg&0xf));
        }
    }
    else if( IS_GPRREG(arg) ) {
        _allocX86reg(X86ARG, X86TYPE_GPR, arg&0xff, MODE_READ);
    }
    else if( IS_CONSTREG(arg) ) {
        _freeX86reg(X86ARG);
        MOV32ItoR(X86ARG, argmem);
    }
    else if( IS_EECONSTREG(arg) ) {
        _freeX86reg(X86ARG);
        MOV32ItoR(X86ARG, g_cpuConstRegs[(arg>>16)&0x1f].UD[0]);
    }
	else if( IS_PSXCONSTREG(arg) ) {
        _freeX86reg(X86ARG);
        MOV32ItoR(X86ARG, g_psxConstRegs[(arg>>16)&0x1f]);
	}
    else if( IS_MEMORYREG(arg) ) {
        _freeX86reg(X86ARG);
        MOV64MtoR(X86ARG, argmem);
    }
    else if( IS_XMMREG(arg) ) {
        _freeX86reg(X86ARG);
		SSEX_MOVD_XMM_to_Rm(X86ARG, arg&0xf);
	}
    else {
        assert((arg&0xfff0)==0);
        _freeX86reg(X86ARG);
        MOV64RtoR(X86ARG, (arg&0xf));
    }
}

void _callFunctionArg1(uptr fn, u32 arg1, uptr arg1mem)
{
    _callPushArg(arg1, arg1mem, X86ARG1);
    CALLFunc((uptr)fn);
}

void _callFunctionArg2(uptr fn, u32 arg1, u32 arg2, uptr arg1mem, uptr arg2mem)
{
    _callPushArg(arg1, arg1mem, X86ARG1);
    _callPushArg(arg2, arg2mem, X86ARG2);
    CALLFunc((uptr)fn);
}

void _callFunctionArg3(uptr fn, u32 arg1, u32 arg2, u32 arg3, uptr arg1mem, uptr arg2mem, uptr arg3mem)
{
    _callPushArg(arg1, arg1mem, X86ARG1);
    _callPushArg(arg2, arg2mem, X86ARG2);
    _callPushArg(arg3, arg3mem, X86ARG3);
    CALLFunc((uptr)fn);
}

void _recPushReg(int mmreg)
{
    assert(0);
}

void _signExtendSFtoM(u32 mem)
{
	assert(0);
}

void _recMove128MtoM(u32 to, u32 from)
{
	MOV64MtoR(RAX, from);
	MOV64RtoM(to, RAX);
    MOV64MtoR(RAX, from+8);
	MOV64RtoM(to+8, RAX);
}

void _recMove128RmOffsettoM(u32 to, u32 offset)
{
	MOV64RmOffsettoR(RAX, RCX, offset);
	MOV64RtoM(to, RAX);
	MOV64RmOffsettoR(RAX, RCX, offset+8);
	MOV64RtoM(to+8, RAX);
}

void _recMove128MtoRmOffset(u32 offset, u32 from)
{
	MOV64MtoR(RAX, from);
	MOV64RtoRmOffset(RCX, RAX, offset);
	MOV64MtoR(RAX, from+8);
	MOV64RtoRmOffset(RCX, RAX, offset+8);
}

// 32 bit
void LogicalOp32RtoM(uptr to, x86IntRegType from, int op)
{
	switch(op) {
		case 0: AND32RtoM(to, from); break;
		case 1: OR32RtoM(to, from); break;
		case 2: XOR32RtoM(to, from); break;
		case 3: OR32RtoM(to, from); break;
	}
}

void LogicalOp32MtoR(x86IntRegType to, uptr from, int op)
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

void LogicalOp32ItoM(uptr to, u32 from, int op)
{
	switch(op) {
		case 0: AND32ItoM(to, from); break;
		case 1: OR32ItoM(to, from); break;
		case 2: XOR32ItoM(to, from); break;
		case 3: OR32ItoM(to, from); break;
	}
}

// 64 bit
void LogicalOp64RtoR(x86IntRegType to, x86IntRegType from, int op)
{
    switch(op) {
		case 0: AND64RtoR(to, from); break;
		case 1: OR64RtoR(to, from); break;
		case 2: XOR64RtoR(to, from); break;
		case 3: OR64RtoR(to, from); break;
	}
}

void LogicalOp64RtoM(uptr to, x86IntRegType from, int op)
{
	switch(op) {
		case 0: AND64RtoM(to, from); break;
		case 1: OR64RtoM(to, from); break;
		case 2: XOR64RtoM(to, from); break;
		case 3: OR64RtoM(to, from); break;
	}
}

void LogicalOp64MtoR(x86IntRegType to, uptr from, int op)
{
	switch(op) {
		case 0: AND64MtoR(to, from); break;
		case 1: OR64MtoR(to, from); break;
		case 2: XOR64MtoR(to, from); break;
		case 3: OR64MtoR(to, from); break;
	}
}

#endif // PCSX2_NORECBUILD
