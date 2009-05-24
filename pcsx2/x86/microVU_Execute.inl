/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2009  Pcsx2 Team
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

// Generates the code for entering recompiled blocks
microVUt(void) mVUdispatcherA() {
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
	SSE_LDMXCSR((uptr)&g_sseVUMXCSR);

	// Load Regs
	MOV32ItoR(gprR, Roffset); // Load VI Reg Offset
	MOV32MtoR(gprF0, (uptr)&mVU->regs->VI[REG_STATUS_FLAG].UL);
	AND32ItoR(gprF0, 0xffff);
	MOV32RtoR(gprF1, gprF0);
	MOV32RtoR(gprF2, gprF0);
	MOV32RtoR(gprF3, gprF0);

	SSE_MOVAPS_M128_to_XMM(xmmT1, (uptr)&mVU->regs->VI[REG_MAC_FLAG].UL);
	SSE_SHUFPS_XMM_to_XMM (xmmT1, xmmT1, 0);
	SSE_MOVAPS_XMM_to_M128((uptr)mVU->macFlag, xmmT1);

	SSE_MOVAPS_M128_to_XMM(xmmT1, (uptr)&mVU->regs->VI[REG_CLIP_FLAG].UL);
	SSE_SHUFPS_XMM_to_XMM (xmmT1, xmmT1, 0);
	SSE_MOVAPS_XMM_to_M128((uptr)mVU->clipFlag, xmmT1);

	SSE_MOVAPS_M128_to_XMM(xmmACC, (uptr)&mVU->regs->ACC.UL[0]);
	SSE_MOVAPS_M128_to_XMM(xmmMax, (uptr)mVU_maxvals);
	SSE_MOVAPS_M128_to_XMM(xmmMin, (uptr)mVU_minvals);
	SSE_MOVAPS_M128_to_XMM(xmmT1, (uptr)&mVU->regs->VI[REG_P].UL);
	SSE_MOVAPS_M128_to_XMM(xmmPQ, (uptr)&mVU->regs->VI[REG_Q].UL);
	SSE_SHUFPS_XMM_to_XMM(xmmPQ, xmmT1, 0); // wzyx = PPQQ

	for (int i = 1; i < 16; i++) {
		if (isMMX(i)) { MOVQMtoR(mmVI(i), (uptr)&mVU->regs->VI[i].UL); }
	}

	// Jump to Recompiled Code Block
	JMPR(EAX);
}

// Generates the code to exit from recompiled blocks
microVUt(void) mVUdispatcherB() {
	microVU* mVU = mVUx;
	mVU->exitFunct = x86Ptr;

	// Load EE's MXCSR state
	SSE_LDMXCSR((uptr)&g_sseMXCSR);
	
	// Save Regs (Other Regs Saved in mVUcompile)
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

	mVUcacheCheck(x86Ptr, mVU->cache, 0x1000);
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
	mVU->cycles = mVU->totalCycles - mVU->cycles;
	mVU->regs->cycle += mVU->cycles;
	cpuRegs.cycle += ((mVU->cycles < 3000) ? mVU->cycles : 3000) * Config.Hacks.VUCycleSteal;
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
