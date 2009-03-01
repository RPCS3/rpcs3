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

// Important Note to Future Developers:
//   None of the COP0 instructions are really critical performance items,
//   so don't waste time converting any more them into recompiled code
//   unless it can make them nicely compact.  Calling the C versions will
//   suffice.

#include "PrecompiledHeader.h"

#include "Common.h"
#include "R5900OpcodeTables.h"
#include "iR5900.h"
#include "iCOP0.h"

namespace Interp = R5900::Interpreter::OpcodeImpl::COP0;

namespace R5900 {
namespace Dynarec {
namespace OpcodeImpl {
namespace COP0 {

/*********************************************************
*   COP0 opcodes                                         *
*                                                        *
*********************************************************/

// emits "setup" code for a COP0 branch test.  The instruction immediately following
// this should be a conditional Jump -- JZ or JNZ normally.
static void _setupBranchTest()
{
	_eeFlushAllUnused();

	// COP0 branch conditionals are based on the following equation:
	//  (((psHu16(DMAC_STAT) | ~psHu16(DMAC_PCR)) & 0x3ff) == 0x3ff)
	// BC0F checks if the statement is false, BC0T checks if the statement is true.

	// note: We only want to compare the 16 bit values of DMAC_STAT and PCR.
	// But using 32-bit loads here is ok (and faster), because we mask off
	// everything except the lower 10 bits away.

	MOV32MtoR( EAX, (uptr)&psHu32(DMAC_PCR) );
	MOV32ItoR( ECX, 0x3ff );		// ECX is our 10-bit mask var
	NOT32R( EAX );
	OR32MtoR( EAX, (uptr)&psHu32(DMAC_STAT) );
	AND32RtoR( EAX, ECX );
	CMP32RtoR( EAX, ECX );
}

void recBC0F()
{
	_setupBranchTest();
	recDoBranchImm(JE32(0));
}

void recBC0T()
{
	_setupBranchTest();
	recDoBranchImm(JNE32(0));
}

void recBC0FL()
{
	_setupBranchTest();
	recDoBranchImm_Likely(JE32(0));
}

void recBC0TL()
{
	_setupBranchTest();
	recDoBranchImm_Likely(JNE32(0));
}

void recTLBR() { recCall( Interp::TLBR, -1 ); }
void recTLBP() { recCall( Interp::TLBP, -1 ); }
void recTLBWI() { recCall( Interp::TLBWI, -1 ); }
void recTLBWR() { recCall( Interp::TLBWR, -1 ); }

void recERET()
{
	recBranchCall( Interp::ERET );
}

void recEI()
{
	// must branch after enabling interrupts, so that anything
	// pending gets triggered properly.
	recBranchCall( Interp::EI );
}

void recDI()
{
	// No need to branch after disabling interrupts...

	iFlushCall(0);

	MOV32MtoR( EAX, (uptr)&cpuRegs.cycle );
	MOV32RtoM( (uptr)&g_nextBranchCycle, EAX );

	CALLFunc( (uptr)Interp::DI );
}


#ifndef CP0_RECOMPILE

REC_SYS( MFC0 );
REC_SYS( MTC0 );

#else

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
		COP0_LOG("MFC0 PCCR = %x PCR0 = %x PCR1 = %x IMM= %x\n", 
				cpuRegs.PERF.n.pccr, cpuRegs.PERF.n.pcr0, cpuRegs.PERF.n.pcr1, _Imm_ & 0x3F);
#endif
		return;
	}
	else if( _Rd_ == 24){
		COP0_LOG("MFC0 Breakpoint debug Registers code = %x\n", cpuRegs.code & 0x3FF);
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

		if( (mmreg = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_WRITE)) >= 0 ) {
			MOVDMtoMMX(mmreg, (uptr)&cpuRegs.CP0.r[ _Rd_ ]);
			SetMMXstate();
		}
		else if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, _Rt_, MODE_READ)) >= 0) {

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
				COP0_LOG("MTC0 PCCR = %x PCR0 = %x PCR1 = %x IMM= %x\n", 
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
				COP0_LOG("MTC0 Breakpoint debug Registers code = %x\n", cpuRegs.code & 0x3FF);
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
				COP0_LOG("MTC0 PCCR = %x PCR0 = %x PCR1 = %x IMM= %x\n", 
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
				COP0_LOG("MTC0 Breakpoint debug Registers code = %x\n", cpuRegs.code & 0x3FF);
				break;
			default:
				_eeMoveGPRtoM((uptr)&cpuRegs.CP0.r[_Rd_], _Rt_);
				break;
		}
	}
}
#endif


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
}*/

}}}}
