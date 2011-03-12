/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#pragma once

//------------------------------------------------------------------
// Dispatcher Functions
//------------------------------------------------------------------

#ifdef __GNUC__
extern void mVUreset(microVU& mVU, bool resetReserve);
#endif

// Generates the code for entering recompiled blocks
void mVUdispatcherA(mV) {
	mVU->startFunct = x86Ptr;

	// Backup cpu state
	xPUSH(ebp);
	xPUSH(ebx);
	xPUSH(esi);
	xPUSH(edi);

	// Align the stackframe (GCC only, since GCC assumes stackframe is always aligned)
	#ifdef __GNUC__
	xSUB(esp, 12);
	#endif

	// __fastcall = The caller has already put the needed parameters in ecx/edx:
	if (!isVU1)	{ xCALL(mVUexecuteVU0); }
	else		{ xCALL(mVUexecuteVU1); }

	// Load VU's MXCSR state
	xLDMXCSR(g_sseVUMXCSR);

	// Load Regs
#if 1 // CHECK_MACROVU0 - Always on now
	xMOV(gprF0, ptr32[&mVU->regs().VI[REG_STATUS_FLAG].UL]);
	xMOV(gprF1, gprF0);
	xMOV(gprF2, gprF0);
	xMOV(gprF3, gprF0);
#else
	mVUallocSFLAGd((uptr)&mVU->regs().VI[REG_STATUS_FLAG].UL, 1);
#endif

	xMOVAPS (xmmT1, ptr128[&mVU->regs().VI[REG_MAC_FLAG].UL]);
	xSHUF.PS(xmmT1, xmmT1, 0);
	xMOVAPS (ptr128[mVU->macFlag],  xmmT1);

	xMOVAPS (xmmT1, ptr128[&mVU->regs().VI[REG_CLIP_FLAG].UL]);
	xSHUF.PS(xmmT1, xmmT1, 0);
	xMOVAPS (ptr128[mVU->clipFlag], xmmT1);

	xMOVAPS (xmmT1, ptr128[&mVU->regs().VI[REG_P].UL]);
	xMOVAPS (xmmPQ, ptr128[&mVU->regs().VI[REG_Q].UL]);
	xSHUF.PS(xmmPQ, xmmT1, 0); // wzyx = PPQQ

	// Jump to Recompiled Code Block
	xJMP(eax);
	pxAssertDev(xGetPtr() < (mVU->dispCache + mVUdispCacheSize),
		"microVU: Dispatcher generation exceeded reserved cache area!");
}

// Generates the code to exit from recompiled blocks
void mVUdispatcherB(mV) {
	mVU->exitFunct = x86Ptr;

	// Load EE's MXCSR state
	xLDMXCSR(g_sseMXCSR);

	// __fastcall = The first two DWORD or smaller arguments are passed in ECX and EDX registers;
	//              all other arguments are passed right to left.
	if (!isVU1) { xCALL(mVUcleanUpVU0); }
	else		{ xCALL(mVUcleanUpVU1); }

	// Unalign the stackframe:
	#ifdef __GNUC__
	xADD( esp, 12 );
	#endif

	// Restore cpu state
	xPOP(edi);
	xPOP(esi);
	xPOP(ebx);
	xPOP(ebp);

	xRET();
	pxAssertDev(xGetPtr() < (mVU->dispCache + mVUdispCacheSize),
		"microVU: Dispatcher generation exceeded reserved cache area!");
}

// Generates the code for resuming xgkick
void mVUdispatcherC(mV) {
	mVU->startFunctXG = x86Ptr;

	// Backup cpu state
	xPUSH(ebp);
	xPUSH(ebx);
	xPUSH(esi);
	xPUSH(edi);

	// Align the stackframe (GCC only, since GCC assumes stackframe is always aligned)
	#ifdef __GNUC__
	xSUB(esp, 12);
	#endif

	// Load VU's MXCSR state
	xLDMXCSR(g_sseVUMXCSR);

	mVUrestoreRegs(mVU);

	xMOV(gprF0, ptr32[&mVU->statFlag[0]]);
	xMOV(gprF1, ptr32[&mVU->statFlag[1]]);
	xMOV(gprF2, ptr32[&mVU->statFlag[2]]);
	xMOV(gprF3, ptr32[&mVU->statFlag[3]]);

	// Jump to Recompiled Code Block
	xJMP(ptr32[&mVU->resumePtrXG]);
	pxAssertDev(xGetPtr() < (mVU->dispCache + mVUdispCacheSize),
		"microVU: Dispatcher generation exceeded reserved cache area!");
}

// Generates the code to exit from xgkick
void mVUdispatcherD(mV) {
	mVU->exitFunctXG = x86Ptr;

	//xPOP(gprT1); // Pop return address
	//xMOV(ptr32[&mVU->resumePtrXG], gprT1);

	// Backup Status Flag (other regs were backed up on xgkick)
	xMOV(ptr32[&mVU->statFlag[0]], gprF0);
	xMOV(ptr32[&mVU->statFlag[1]], gprF1);
	xMOV(ptr32[&mVU->statFlag[2]], gprF2);
	xMOV(ptr32[&mVU->statFlag[3]], gprF3);

	// Load EE's MXCSR state
	xLDMXCSR(g_sseMXCSR);

	// Unalign the stackframe:
	#ifdef __GNUC__
	xADD( esp, 12 );
	#endif

	// Restore cpu state
	xPOP(edi);
	xPOP(esi);
	xPOP(ebx);
	xPOP(ebp);

	xRET();
	pxAssertDev(xGetPtr() < (mVU->dispCache + mVUdispCacheSize),
		"microVU: Dispatcher generation exceeded reserved cache area!");
}

//------------------------------------------------------------------
// Execution Functions
//------------------------------------------------------------------

// Executes for number of cycles
_mVUt void* __fastcall mVUexecute(u32 startPC, u32 cycles) {

	microVU* mVU = mVUx;
	//DevCon.WriteLn("microVU%x: startPC = 0x%x, cycles = 0x%x", vuIndex, startPC, cycles);

	mVU->cycles		 = cycles;
	mVU->totalCycles = cycles;

	xSetPtr(mVU->prog.x86ptr); // Set x86ptr to where last program left off
	return mVUsearchProg<vuIndex>(startPC, (uptr)&mVU->prog.lpState); // Find and set correct program
}

//------------------------------------------------------------------
// Cleanup Functions
//------------------------------------------------------------------

_mVUt void mVUcleanUp() {
	microVU* mVU = mVUx;
	//mVUprint("microVU: Program exited successfully!");
	//mVUprint("microVU: VF0 = {%x,%x,%x,%x}", mVU->regs().VF[0].UL[0], mVU->regs().VF[0].UL[1], mVU->regs().VF[0].UL[2], mVU->regs().VF[0].UL[3]);
	//mVUprint("microVU: VI0 = %x", mVU->regs().VI[0].UL);

	mVU->prog.x86ptr = x86Ptr;

	if ((xGetPtr() < mVU->prog.x86start) || (xGetPtr() >= mVU->prog.x86end)) {
		Console.WriteLn(vuIndex ? Color_Orange : Color_Magenta, "microVU%d: Program cache limit reached.", mVU->index);
		mVUreset(*mVU, false);
	}

	mVU->cycles = mVU->totalCycles - mVU->cycles;
	mVU->regs().cycle += mVU->cycles;
	cpuRegs.cycle += ((mVU->cycles < 3000) ? mVU->cycles : 3000) * EmuConfig.Speedhacks.VUCycleSteal;
	//static int ax = 0; ax++;
	//if (!(ax % 100000)) {
	//	for (u32 i = 0; i < (mVU->progSize / 2); i++) {
	//		if (mVUcurProg.block[i]) {
	//			mVUcurProg.block[i]->printInfo(i*8);
	//		}
	//	}
	//}
}

//------------------------------------------------------------------
// Caller Functions
//------------------------------------------------------------------

void* __fastcall mVUexecuteVU0(u32 startPC, u32 cycles) { return mVUexecute<0>(startPC, cycles); }
void* __fastcall mVUexecuteVU1(u32 startPC, u32 cycles) { return mVUexecute<1>(startPC, cycles); }
void  __fastcall mVUcleanUpVU0() { mVUcleanUp<0>(); }
void  __fastcall mVUcleanUpVU1() { mVUcleanUp<1>(); }
