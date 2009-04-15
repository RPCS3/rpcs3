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

// Generates the code for entering recompiled blocks
microVUt(void) mVUdispatcherA() {
	static u32 PCSX2_ALIGNED16(vuMXCSR);
	microVU* mVU = mVUx;
	x86SetPtr(mVU->ptr);
	mVU->startFunct = mVU->ptr;

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
	MOV32MtoR(gprR,  (uptr)&mVU->regs->VI[REG_R]);
	MOV32MtoR(gprF0, (uptr)&mVU->regs->VI[REG_STATUS_FLAG]);
	MOV32MtoR(gprF1, (uptr)&mVU->regs->VI[REG_MAC_FLAG]);
	SHL32ItoR(gprF0, 16);
	AND32ItoR(gprF1, 0xffff);
	OR32RtoR (gprF0, gprF1);
	MOV32RtoR(gprF1, gprF0);
	MOV32RtoR(gprF2, gprF0);
	MOV32RtoR(gprF3, gprF0);

	for (int i = 0; i < 8; i++) {
		MOVQMtoR(i, (uptr)&mVU->regs->VI[i+1]);
	}

	SSE_MOVAPS_M128_to_XMM(xmmACC, (uptr)&mVU->regs->ACC);
	SSE_MOVAPS_M128_to_XMM(xmmMax, (uptr)&mVU_maxvals[0]);
	SSE_MOVAPS_M128_to_XMM(xmmMin, (uptr)&mVU_minvals[0]);
	SSE_MOVAPS_M128_to_XMM(xmmT1, (uptr)&mVU->regs->VI[REG_P]);
	SSE_MOVAPS_M128_to_XMM(xmmPQ, (uptr)&mVU->regs->VI[REG_Q]);
	SSE_SHUFPS_XMM_to_XMM(xmmPQ, xmmT1, 0); // wzyx = PPQQ

	// Jump to Recompiled Code Block
	JMPR(EAX);
	mVU->ptr = x86Ptr;
}

// Generates the code to exit from recompiled blocks
microVUt(void) mVUdispatcherB() {
	static u32 PCSX2_ALIGNED16(eeMXCSR);
	microVU* mVU = mVUx;
	x86SetPtr(mVU->ptr);
	mVU->exitFunct = mVU->ptr;

	// __fastcall = The first two DWORD or smaller arguments are passed in ECX and EDX registers; all other arguments are passed right to left.
	if (!vuIndex) { CALLFunc((uptr)mVUcleanUpVU0); }
	else		  { CALLFunc((uptr)mVUcleanUpVU1); }

	// Load EE's MXCSR state
	eeMXCSR = g_sseMXCSR;
	SSE_LDMXCSR((uptr)&eeMXCSR);
	
	// Save Regs
	MOV32RtoR(gprT1, gprF0); // ToDo: Ensure Correct Flag instances
	AND32ItoR(gprT1, 0xffff);
	SHR32ItoR(gprF0, 16);
	MOV32RtoM((uptr)&mVU->regs->VI[REG_R],			 gprR);
	MOV32RtoM((uptr)&mVU->regs->VI[REG_STATUS_FLAG], gprT1);
	MOV32RtoM((uptr)&mVU->regs->VI[REG_MAC_FLAG],	 gprF0);
	
	for (int i = 0; i < 8; i++) {
		MOVDMMXtoM((uptr)&mVU->regs->VI[i+1], i);
	}

	SSE_MOVAPS_XMM_to_M128((uptr)&mVU->regs->ACC, xmmACC);
	//SSE_MOVSS_XMM_to_M32((uptr)&mVU->regs->VI[REG_Q], xmmPQ); // ToDo: Ensure Correct Q/P instances
	//SSE2_PSHUFD_XMM_to_XMM(xmmPQ, xmmPQ, 0); // wzyx = PPPP
	//SSE_MOVSS_XMM_to_M32((uptr)&mVU->regs->VI[REG_P], xmmPQ);

	// Restore cpu state
	POP32R(EDI);
	POP32R(ESI);
	POP32R(EBP);
	POP32R(EBX);

	EMMS();
	RET();

	mVU->ptr = x86Ptr;
	mVUcachCheck(mVU->cache, 512);
}

//------------------------------------------------------------------
// Execution Functions
//------------------------------------------------------------------

// Executes for number of cycles
microVUt(void*) __fastcall mVUexecute(u32 startPC, u32 cycles) {
/*
	Pseudocode: (ToDo: implement # of cycles)
	1) Search for existing program
	2) If program not found, goto 5
	3) Search for recompiled block
	4) If recompiled block found, goto 6
	5) Recompile as much blocks as possible
	6) Return start execution address of block
*/
	microVU* mVU = mVUx;
	mVUlog("microVU%x: startPC = 0x%x, cycles = 0x%x", params vuIndex, startPC, cycles);
	if ( mVUsearchProg(mVU) ) { // Found Program
		//microBlock* block = mVU->prog.prog[mVU->prog.cur].block[startPC]->search(mVU->prog.lastPipelineState);
		//if (block) return block->x86ptrStart; // Found Block
	}
	// Recompile code
	return NULL;
}

//------------------------------------------------------------------
// Cleanup Functions
//------------------------------------------------------------------

microVUt(void) mVUcleanUp() {
	microVU* mVU = mVUx;
	mVU->ptr = mVUcurProg.x86ptr;
	mVUcachCheck(mVUcurProg.x86start, (uptr)(mVUcurProg.x86end - mVUcurProg.x86start));
}

//------------------------------------------------------------------
// Caller Functions
//------------------------------------------------------------------

void  __fastcall startVU0(u32 startPC, u32 cycles) { ((mVUrecCall)microVU0.startFunct)(startPC, cycles); }
void  __fastcall startVU1(u32 startPC, u32 cycles) { ((mVUrecCall)microVU1.startFunct)(startPC, cycles); }
void* __fastcall mVUexecuteVU0(u32 startPC, u32 cycles) { return mVUexecute<0>(startPC, cycles); }
void* __fastcall mVUexecuteVU1(u32 startPC, u32 cycles) { return mVUexecute<1>(startPC, cycles); }
void mVUcleanUpVU0() { mVUcleanUp<0>(); }
void mVUcleanUpVU1() { mVUcleanUp<1>(); }

#endif //PCSX2_MICROVU
