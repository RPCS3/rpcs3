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

#if !(defined(_MSC_VER) && defined(PCSX2_NORECBUILD))

#include "Common.h"
#include "InterTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"
#include "iCP0.h"

/*********************************************************
*   COP0 opcodes                                         *
*                                                        *
*********************************************************/

#ifndef CP0_RECOMPILE

REC_SYS(MFC0);
REC_SYS(MTC0);
REC_SYS(BC0F);
REC_SYS(BC0T);
REC_SYS(BC0FL);
REC_SYS(BC0TL);
REC_SYS(TLBR);
REC_SYS(TLBWI);
REC_SYS(TLBWR);
REC_SYS(TLBP);
REC_SYS(ERET);
REC_SYS(DI);
REC_SYS(EI);

#else

////////////////////////////////////////////////////
//REC_SYS(MTC0);
////////////////////////////////////////////////////
REC_SYS(BC0F);
////////////////////////////////////////////////////
REC_SYS(BC0T);
////////////////////////////////////////////////////
REC_SYS(BC0FL);
////////////////////////////////////////////////////
REC_SYS(BC0TL);
////////////////////////////////////////////////////
REC_SYS(TLBR);
////////////////////////////////////////////////////
REC_SYS(TLBWI);
////////////////////////////////////////////////////
REC_SYS(TLBWR);
////////////////////////////////////////////////////
REC_SYS(TLBP);
////////////////////////////////////////////////////
REC_SYS(ERET);
////////////////////////////////////////////////////
REC_SYS(DI);
////////////////////////////////////////////////////
REC_SYS(EI);

////////////////////////////////////////////////////
extern u32 s_iLastCOP0Cycle;
extern u32 s_iLastPERFCycle[2];

void recMFC0( void )
{
	int mmreg;

	if ( ! _Rt_ ) return;

	if( _Rd_ == 9 ) {
        MOV32MtoR(ECX, (uptr)&cpuRegs.cycle);
        MOV32RtoR(EAX,ECX);
		SUB32MtoR(EAX, (uptr)&s_iLastCOP0Cycle);
        ADD32RtoM((uptr)&cpuRegs.CP0.n.Count, EAX);
		MOV32RtoM((uptr)&s_iLastCOP0Cycle, ECX);
        MOV32MtoR( EAX, (uptr)&cpuRegs.CP0.r[ _Rd_ ] );
		
		_deleteEEreg(_Rt_, 0);
		MOV32RtoM((uptr)&cpuRegs.GPR.r[_Rt_].UL[0],EAX);

		if(EEINST_ISLIVE1(_Rt_)) {
			CDQ();
			MOV32RtoM((uptr)&cpuRegs.GPR.r[_Rt_].UL[1], EDX);
		}
		else EEINST_RESETHASLIVE1(_Rt_);
		return;
	}
	if( _Rd_ == 25 ) {		
		
		_deleteEEreg(_Rt_, 0);
		switch(_Imm_ & 0x3F){
			case 0:
				MOV32MtoR(EAX, (uptr)&cpuRegs.PERF.n.pccr);

                break;
			case 1:
				// check if needs to be incremented
				MOV32MtoR(ECX, (uptr)&cpuRegs.PERF.n.pccr);
				MOV32MtoR(EAX, (uptr)&cpuRegs.PERF.n.pcr0);
				AND32ItoR(ECX, 0x800003E0);

				CMP32ItoR(ECX, 0x80000020);
				j8Ptr[0] = JNE8(0);
				
				MOV32MtoR(EDX, (uptr)&cpuRegs.cycle);
				SUB32MtoR(EAX, (uptr)&s_iLastPERFCycle[0]);
				ADD32RtoR(EAX, EDX);
				MOV32RtoM((uptr)&s_iLastPERFCycle[0], EDX);
				MOV32RtoM((uptr)&cpuRegs.PERF.n.pcr0, EAX);

				x86SetJ8(j8Ptr[0]);
				break;
			case 3:
				// check if needs to be incremented
				MOV32MtoR(ECX, (uptr)&cpuRegs.PERF.n.pccr);
				MOV32MtoR(EAX, (uptr)&cpuRegs.PERF.n.pcr1);
				AND32ItoR(ECX, 0x800F8000);

				CMP32ItoR(ECX, 0x80008000);
				j8Ptr[0] = JNE8(0);
				
				MOV32MtoR(EDX, (uptr)&cpuRegs.cycle);
				SUB32MtoR(EAX, (uptr)&s_iLastPERFCycle[1]);
				ADD32RtoR(EAX, EDX);
				MOV32RtoM((uptr)&s_iLastPERFCycle[1], EDX);
				MOV32RtoM((uptr)&cpuRegs.PERF.n.pcr1, EAX);

				x86SetJ8(j8Ptr[0]);
				
				break;
		}
	
        MOV32RtoM((uptr)&cpuRegs.GPR.r[_Rt_].UL[0],EAX);

		if(EEINST_ISLIVE1(_Rt_)) {
			CDQ();
			MOV32RtoM((uptr)&cpuRegs.GPR.r[_Rt_].UL[1], EDX);
		}
		else EEINST_RESETHASLIVE1(_Rt_);

#ifdef PCSX2_DEVBUILD
		SysPrintf("MFC0 PCCR = %x PCR0 = %x PCR1 = %x IMM= %x\n", 
				cpuRegs.PERF.n.pccr, cpuRegs.PERF.n.pcr0, cpuRegs.PERF.n.pcr1, _Imm_ & 0x3F);
#endif
		return;
	}
	else if( _Rd_ == 24){
		SysPrintf("MFC0 Breakpoint debug Registers code = %x\n", cpuRegs.code & 0x3FF);
        return;
	}
	_eeOnWriteReg(_Rt_, 1);

	if( EEINST_ISLIVE1(_Rt_) ) {
		_deleteEEreg(_Rt_, 0);
		MOV32MtoR(EAX, (uptr)&cpuRegs.CP0.r[ _Rd_ ]);
		CDQ();
		MOV32RtoM((uptr)&cpuRegs.GPR.r[_Rt_].UL[0], EAX);
		MOV32RtoM((uptr)&cpuRegs.GPR.r[_Rt_].UL[1], EDX);
	}
	else {
		EEINST_RESETHASLIVE1(_Rt_);

#ifndef __x86_64__
		if( (mmreg = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_WRITE)) >= 0 ) {
			MOVDMtoMMX(mmreg, (uptr)&cpuRegs.CP0.r[ _Rd_ ]);
			SetMMXstate();
		}
		else
#endif
        if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, _Rt_, MODE_READ)) >= 0) {

			if( EEINST_ISLIVE2(_Rt_) ) {
				if( xmmregs[mmreg].mode & MODE_WRITE ) {
					SSE_MOVHPS_XMM_to_M64((uptr)&cpuRegs.GPR.r[_Rt_].UL[2], mmreg);
				}
				xmmregs[mmreg].inuse = 0;

				MOV32MtoR(EAX, (uptr)&cpuRegs.CP0.r[ _Rd_ ]);
				MOV32RtoM((uptr)&cpuRegs.GPR.r[_Rt_].UL[0],EAX);
			}
			else {
				SSE_MOVLPS_M64_to_XMM(mmreg, (uptr)&cpuRegs.CP0.r[ _Rd_ ]);
			}
		}
		else {
			MOV32MtoR(EAX, (uptr)&cpuRegs.CP0.r[ _Rd_ ]);
			if(_Rd_ == 12) AND32ItoR(EAX, 0xf0c79c1f);
			MOV32RtoM((uptr)&cpuRegs.GPR.r[_Rt_].UL[0],EAX);
			if(EEINST_ISLIVE1(_Rt_)) {
				CDQ();
				MOV32RtoM((uptr)&cpuRegs.GPR.r[_Rt_].UL[1], EDX);
			}
			else {
				EEINST_RESETHASLIVE1(_Rt_);
			}
		}
	}
}

void updatePCCR()
{
	// read the old pccr and update pcr0/1
	MOV32MtoR(EAX, (uptr)&cpuRegs.PERF.n.pccr);
	MOV32RtoR(EDX, EAX);
	MOV32MtoR(ECX, (uptr)&cpuRegs.cycle);

	AND32ItoR(EAX, 0x800003E0);
	CMP32ItoR(EAX, 0x80000020);
	j8Ptr[0] = JNE8(0);
	MOV32MtoR(EAX, (uptr)&s_iLastPERFCycle[0]);
	ADD32RtoM((uptr)&cpuRegs.PERF.n.pcr0, ECX);
	SUB32RtoM((uptr)&cpuRegs.PERF.n.pcr0, EAX);
	x86SetJ8(j8Ptr[0]);

	AND32ItoR(EDX, 0x800F8000);
	CMP32ItoR(EDX, 0x80008000);
	j8Ptr[0] = JNE8(0);
	MOV32MtoR(EAX, (uptr)&s_iLastPERFCycle[1]);
	ADD32RtoM((uptr)&cpuRegs.PERF.n.pcr1, ECX);
	SUB32RtoM((uptr)&cpuRegs.PERF.n.pcr1, EAX);
	x86SetJ8(j8Ptr[0]);
}

void recMTC0()
{
	if( GPR_IS_CONST1(_Rt_) ) {
		switch (_Rd_) {
			case 12: 
				iFlushCall(FLUSH_NODESTROY);
				//_flushCachedRegs(); //NOTE: necessary?
				_callFunctionArg1((uptr)WriteCP0Status, MEM_CONSTTAG, g_cpuConstRegs[_Rt_].UL[0]);
				break;
			case 9:
				MOV32MtoR(ECX, (uptr)&cpuRegs.cycle);
				MOV32RtoM((uptr)&s_iLastCOP0Cycle, ECX);
				MOV32ItoM((uptr)&cpuRegs.CP0.r[9], g_cpuConstRegs[_Rt_].UL[0]);
				break;
			case 25:
				SysPrintf("MTC0 PCCR = %x PCR0 = %x PCR1 = %x IMM= %x\n", 
				cpuRegs.PERF.n.pccr, cpuRegs.PERF.n.pcr0, cpuRegs.PERF.n.pcr1, _Imm_ & 0x3F);
				switch(_Imm_ & 0x3F){
					case 0:
						
						updatePCCR();
						MOV32ItoM((uptr)&cpuRegs.PERF.n.pccr, g_cpuConstRegs[_Rt_].UL[0]);

						// update the cycles
						MOV32RtoM((uptr)&s_iLastPERFCycle[0], ECX);
						MOV32RtoM((uptr)&s_iLastPERFCycle[1], ECX);
						break;
					case 1:
						MOV32MtoR(EAX, (uptr)&cpuRegs.cycle);
						MOV32ItoM((uptr)&cpuRegs.PERF.n.pcr0, g_cpuConstRegs[_Rt_].UL[0]);
						MOV32RtoM((uptr)&s_iLastPERFCycle[0], EAX);
						break;
					case 3:
						MOV32MtoR(EAX, (uptr)&cpuRegs.cycle);
						MOV32ItoM((uptr)&cpuRegs.PERF.n.pcr1, g_cpuConstRegs[_Rt_].UL[0]);
						MOV32RtoM((uptr)&s_iLastPERFCycle[1], EAX);
						break;
				}
				break;
			case 24: 
				SysPrintf("MTC0 Breakpoint debug Registers code = %x\n", cpuRegs.code & 0x3FF);
				break;
			default:
				MOV32ItoM((uptr)&cpuRegs.CP0.r[_Rd_], g_cpuConstRegs[_Rt_].UL[0]);
				break;
		}
	}
	else {
		switch (_Rd_) {
			case 12: 
				iFlushCall(FLUSH_NODESTROY);
				//_flushCachedRegs(); //NOTE: necessary?
				_callFunctionArg1((uptr)WriteCP0Status, MEM_GPRTAG|_Rt_, 0);
				break;
			case 9:
				MOV32MtoR(ECX, (uptr)&cpuRegs.cycle);
				_eeMoveGPRtoM((uptr)&cpuRegs.CP0.r[9], _Rt_);
				MOV32RtoM((uptr)&s_iLastCOP0Cycle, ECX);
				break;
			case 25:
				SysPrintf("MTC0 PCCR = %x PCR0 = %x PCR1 = %x IMM= %x\n", 
				cpuRegs.PERF.n.pccr, cpuRegs.PERF.n.pcr0, cpuRegs.PERF.n.pcr1, _Imm_ & 0x3F);
				switch(_Imm_ & 0x3F){
					case 0:
						updatePCCR();
						_eeMoveGPRtoM((uptr)&cpuRegs.PERF.n.pccr, _Rt_);

						// update the cycles
						MOV32RtoM((uptr)&s_iLastPERFCycle[0], ECX);
						MOV32RtoM((uptr)&s_iLastPERFCycle[1], ECX);
						break;
					case 1:
						MOV32MtoR(ECX, (uptr)&cpuRegs.cycle);
						_eeMoveGPRtoM((uptr)&cpuRegs.PERF.n.pcr0, _Rt_);
						MOV32RtoM((uptr)&s_iLastPERFCycle[0], ECX);
						break;
					case 3:
						MOV32MtoR(ECX, (uptr)&cpuRegs.cycle);
						_eeMoveGPRtoM((uptr)&cpuRegs.PERF.n.pcr1, _Rt_);
						MOV32RtoM((uptr)&s_iLastPERFCycle[1], ECX);
						break;
				}
				break;
			case 24: 
				SysPrintf("MTC0 Breakpoint debug Registers code = %x\n", cpuRegs.code & 0x3FF);
				break;
			default:
				_eeMoveGPRtoM((uptr)&cpuRegs.CP0.r[_Rd_], _Rt_);
				break;
		}
	}
}

/*void rec(COP0) {
}

void rec(BC0F) {
}

void rec(BC0T) {
}

void rec(BC0FL) {
}

void rec(BC0TL) {
}

void rec(TLBR) {
}

void rec(TLBWI) {
}

void rec(TLBWR) {
}

void rec(TLBP) {
}

void rec(ERET) {
}
*/

#endif

#endif // PCSX2_NORECBUILD
