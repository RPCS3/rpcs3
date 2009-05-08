/*  Pcsx2 - Pc Ps2 Emulator
*  Copyright (C) 2009  Pcsx2-Playground Team
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

#pragma once
#ifdef PCSX2_MICROVU

//------------------------------------------------------------------
// Dispatcher Functions
//------------------------------------------------------------------

void testFunction() { mVUprint("microVU: Entered Execution Mode"); }

// Generates the code for entering recompiled blocks
microVUt(void) mVUdispatcherA() {
	static u32 PCSX2_ALIGNED16(vuMXCSR);
	microVU* mVU = mVUx;
	mVU->startFunct = x86Ptr;

	// __fastcall = The first two DWORD or smaller arguments are passed in ECX and EDX registers; all other arguments are passed right to left.
	if (!vuIndex) { CALLFunc((uptr)mVUexecuteVU0); }
	else		  { CALLFunc((uptr)mVUexecuteVU1); }

	// Backup cpu state
	PUSH32R(EBX);
	PUSH32R(EBP);
	PUSH32R(ESI);
	PUSH32R(EDI);

	// Load VU's MXCSR state
	vuMXCSR = g_sseVUMXCSR;
	SSE_LDMXCSR((uptr)&vuMXCSR);

	// Load Regs
	MOV32MtoR(gprR,  (uptr)&mVU->regs->VI[REG_R].UL);
	MOV32MtoR(gprF0, (uptr)&mVU->regs->VI[REG_STATUS_FLAG].UL);
	MOV32MtoR(gprF1, (uptr)&mVU->regs->VI[REG_MAC_FLAG].UL);
	SHL32ItoR(gprF0, 16);
	AND32ItoR(gprF1, 0xffff);
	OR32RtoR (gprF0, gprF1);
	MOV32RtoR(gprF1, gprF0);
	MOV32RtoR(gprF2, gprF0);
	MOV32RtoR(gprF3, gprF0);

	for (int i = 1; i < 16; i++) {
		if (isMMX(i)) { MOVQMtoR(mmVI(i), (uptr)&mVU->regs->VI[i].UL); }
	}

	SSE_MOVAPS_M128_to_XMM(xmmACC, (uptr)&mVU->regs->ACC.UL[0]);
	SSE_MOVAPS_M128_to_XMM(xmmMax, (uptr)mVU_maxvals);
	SSE_MOVAPS_M128_to_XMM(xmmMin, (uptr)mVU_minvals);
	SSE_MOVAPS_M128_to_XMM(xmmT1, (uptr)&mVU->regs->VI[REG_P].UL);
	SSE_MOVAPS_M128_to_XMM(xmmPQ, (uptr)&mVU->regs->VI[REG_Q].UL);
	SSE_SHUFPS_XMM_to_XMM(xmmPQ, xmmT1, 0); // wzyx = PPQQ

	//PUSH32R(EAX);
	//CALLFunc((uptr)testFunction);
	//POP32R(EAX);
	//write8(0xcc);

	// Jump to Recompiled Code Block
	JMPR(EAX);
}

// Generates the code to exit from recompiled blocks
microVUt(void) mVUdispatcherB() {
	static u32 PCSX2_ALIGNED16(eeMXCSR);
	microVU* mVU = mVUx;
	mVU->exitFunct = x86Ptr;

	// Load EE's MXCSR state
	eeMXCSR = g_sseMXCSR;
	SSE_LDMXCSR((uptr)&eeMXCSR);
	
	// Save Regs (Other Regs Saved in mVUcompile)
	MOV32RtoM((uptr)&mVU->regs->VI[REG_R].UL, gprR);
	SSE_MOVAPS_XMM_to_M128((uptr)&mVU->regs->ACC.UL[0], xmmACC);

	for (int i = 1; i < 16; i++) {
		if (isMMX(i)) { MOVDMMXtoM((uptr)&mVU->regs->VI[i].UL, mmVI(i)); }
	}

	// __fastcall = The first two DWORD or smaller arguments are passed in ECX and EDX registers; all other arguments are passed right to left.
	if (!vuIndex) { CALLFunc((uptr)mVUcleanUpVU0); }
	else		  { CALLFunc((uptr)mVUcleanUpVU1); }

	// Restore cpu state
	POP32R(EDI);
	POP32R(ESI);
	POP32R(EBP);
	POP32R(EBX);

	if (isMMX(1)) EMMS();
	RET();

	mVUcacheCheck(x86Ptr, mVU->cache, 512);
}

//------------------------------------------------------------------
// Execution Functions
//------------------------------------------------------------------

// Executes for number of cycles
microVUt(void*) __fastcall mVUexecute(u32 startPC, u32 cycles) {

	microVU* mVU = mVUx;
	//mVUprint("microVU%x: startPC = 0x%x, cycles = 0x%x", params vuIndex, startPC, cycles);
	
	mVUsearchProg<vuIndex>(); // Find and set correct program
	mVU->cycles		 = cycles;
	mVU->totalCycles = cycles;

	x86SetPtr(mVUcurProg.x86ptr); // Set x86ptr to where program left off
	if (!vuIndex)	return mVUcompileVU0(startPC, (uptr)&mVU->prog.lpState);
	else			return mVUcompileVU1(startPC, (uptr)&mVU->prog.lpState);
}

//------------------------------------------------------------------
// Cleanup Functions
//------------------------------------------------------------------

microVUt(void) mVUcleanUp() {
	microVU* mVU = mVUx;
	//mVUprint("microVU: Program exited successfully!");
	//mVUprint("microVU: VF0 = {%x,%x,%x,%x}", params mVU->regs->VF[0].UL[0], mVU->regs->VF[0].UL[1], mVU->regs->VF[0].UL[2], mVU->regs->VF[0].UL[3]);
	//mVUprint("microVU: VI0 = %x", params mVU->regs->VI[0].UL);
	mVUcurProg.x86ptr = x86Ptr;
	mVUcacheCheck(x86Ptr, mVUcurProg.x86start, (uptr)(mVUcurProg.x86end - mVUcurProg.x86start));
	mVU->regs->cycle += mVU->totalCycles - mVU->cycles;
}

//------------------------------------------------------------------
// Caller Functions
//------------------------------------------------------------------

void  __fastcall startVU0(u32 startPC, u32 cycles) { ((mVUrecCall)microVU0.startFunct)(startPC, cycles); }
void  __fastcall startVU1(u32 startPC, u32 cycles) { ((mVUrecCall)microVU1.startFunct)(startPC, cycles); }
void* __fastcall mVUexecuteVU0(u32 startPC, u32 cycles) { return mVUexecute<0>(startPC, cycles); }
void* __fastcall mVUexecuteVU1(u32 startPC, u32 cycles) { return mVUexecute<1>(startPC, cycles); }
void  __fastcall mVUcleanUpVU0() { mVUcleanUp<0>(); }
void  __fastcall mVUcleanUpVU1() { mVUcleanUp<1>(); }

#endif //PCSX2_MICROVU
