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
#include "iR5900.h"
#include "VUmicro.h"
#include "sVU_zerorec.h"

#define useMVU1 CHECK_MICROVU1

int mVUdebugNow = 0;

//#define DEBUG_COMPARE  // Run sVU or mVU and print results
//#define DEBUG_COMPARE2 // Runs both VU recs and breaks when results differ

#ifdef DEBUG_COMPARE

#include <windows.h>
static int runAmount = 0;

void VUtestPause() {

	runAmount++;
	if (runAmount < 654) return;

	if (useMVU1) SysPrintf("Micro VU - Pass %d\n", runAmount);
	else		 SysPrintf("Super VU - Pass %d\n", runAmount);

	for (int i = 0; i < 32; i++) {
		SysPrintf("VF%02d  = {%f, %f, %f, %f}\n", i, VU1.VF[i].F[0], VU1.VF[i].F[1], VU1.VF[i].F[2], VU1.VF[i].F[3]);
	}

	SysPrintf("ACC   = {%f, %f, %f, %f}\n", VU1.ACC.F[0], VU1.ACC.F[1], VU1.ACC.F[2], VU1.ACC.F[3]);

	for (int i = 0; i < 16; i++) {
		SysPrintf("VI%02d  = % 8d ($%08x)\n", i, (s16)VU1.VI[i].UL, (s16)VU1.VI[i].UL);
	}

	SysPrintf("Stat  = % 8d ($%08x)\n", (s16)VU1.VI[REG_STATUS_FLAG].UL, (s16)VU1.VI[REG_STATUS_FLAG].UL);
	SysPrintf("MAC   = % 8d ($%08x)\n", (s16)VU1.VI[REG_MAC_FLAG].UL,	  (s16)VU1.VI[REG_MAC_FLAG].UL);
	SysPrintf("CLIP  = % 8d ($%08x)\n", (s16)VU1.VI[REG_CLIP_FLAG].UL,   (s16)VU1.VI[REG_CLIP_FLAG].UL);

	SysPrintf("Q-reg = %f ($%08x)\n", VU1.VI[REG_Q].F, (s32)VU1.VI[REG_Q].UL);
	SysPrintf("P-reg = %f ($%08x)\n", VU1.VI[REG_P].F, (s32)VU1.VI[REG_P].UL);
	SysPrintf("I-reg = %f ($%08x)\n", VU1.VI[REG_I].F, (s32)VU1.VI[REG_I].UL);

	SysPrintf("_Stat = % 8d ($%08x)\n", (s16)VU1.statusflag,	  (s16)VU1.statusflag);
	SysPrintf("_MAC  = % 8d ($%08x)\n", (s16)VU1.macflag,	  (s16)VU1.macflag);
	SysPrintf("_CLIP = % 8d ($%08x)\n", (s16)VU1.clipflag,	  (s16)VU1.clipflag);

	u32 j = 0;
	for (int i = 0; i < (0x4000 / 4); i++) {
		j ^= ((u32*)(VU1.Mem))[i];
	}
	SysPrintf("VU Mem CRC = 0x%08x\n", j);
	SysPrintf("EndPC = 0x%04x\n", VU1.VI[REG_TPC].UL);

	for (int i = 0; i < 10000000; i++) {
		Sleep(1000);
	}
}
#else
void VUtestPause() {}
#endif

#ifdef PCSX2_DEBUG
extern u32 vudump;
#endif

#ifdef DEBUG_COMPARE2

#ifndef DEBUG_COMPARE
#include <windows.h>
#endif

PCSX2_ALIGNED16(u8 backVUregs[sizeof(VURegs)]);
PCSX2_ALIGNED16(u8 cmpVUregs [sizeof(VURegs)]);
PCSX2_ALIGNED16(u8 backVUmem [0x4000]);
PCSX2_ALIGNED16(u8 cmpVUmem  [0x4000]);
static u32 runCount = 0;
#define VU3 ((VURegs)*((VURegs*)cmpVUregs))
#define cmpA Console::Error
#define cmpB Console::WriteLn
#define cmpPrint(cond) {			\
	if (cond) {						\
		cmpA("%s", params str1);	\
		cmpA("%s", params str2);	\
	}								\
	else {							\
		cmpB("%s", params str1);	\
		cmpB("%s", params str2);	\
	}								\
}

namespace VU1micro
{
	void recAlloc()									{ SuperVUAlloc(1); initVUrec(&VU1, 1); }
	void __fastcall recClear(u32 Addr, u32 Size)	{ SuperVUClear(Addr, Size, 1); clearVUrec(Addr, Size, 1); }
	void recShutdown()								{ SuperVUDestroy(1); closeVUrec(1); }
	static void recReset()							{ SuperVUReset(1); resetVUrec(1); x86FpuState = FPU_STATE; }
	static void recStep()							{}

	static void recExecuteBlock(void)
	{
		if((VU0.VI[REG_VPU_STAT].UL & 0x100) == 0) return;
		assert((VU1.VI[ REG_TPC ].UL&7) == 0);
		if (VU1.VI[REG_TPC].UL >= VU1.maxmicro) { Console::Error("VU1 memory overflow!!: %x", params  VU1.VI[REG_TPC].UL); }

#ifdef DEBUG_COMPARE
		SysPrintf("(%08d) StartPC = 0x%04x\n", runAmount, VU1.VI[REG_TPC].UL);
#endif

		FreezeXMMRegs(1);

		runCount++;
		memcpy_fast((u8*)backVUregs, (u8*)&VU1,		sizeof(VURegs));
		memcpy_fast((u8*)backVUmem,	 (u8*)VU1.Mem,	0x4000);

		do { // while loop needed since not always will return finished
			SuperVUExecuteProgram(VU1.VI[ REG_TPC ].UL & 0x3fff, 1);
		} while( VU0.VI[ REG_VPU_STAT ].UL&0x100 );

		memcpy_fast((u8*)cmpVUregs,	(u8*)&VU1,			sizeof(VURegs));
		memcpy_fast((u8*)cmpVUmem,	(u8*)VU1.Mem,		0x4000);
		memcpy_fast((u8*)&VU1,		(u8*)backVUregs,	sizeof(VURegs));
		memcpy_fast((u8*)VU1.Mem,	(u8*)backVUmem,		0x4000);

		//Currently breaking mVU execution is disabled. Check mVUtestCycles<vuIndex>() in microVU_Compile.inl
		runVUrec(VU1.VI[REG_TPC].UL, 300000 /*0x7fffffff*/, 1);

		if ((memcmp((u8*)cmpVUregs, (u8*)&VU1, (16*32) + (16*16))) || (memcmp((u8*)cmpVUmem, (u8*)VU1.Mem, 0x4000))) {
			char str1[150];
			char str2[150];
			SysPrintf("\n\n");
			SysPrintf("-----------------------------------------------\n");
			Console::Notice("Problem Occurred!");
			SysPrintf("-----------------------------------------------\n");
			SysPrintf("runCount = %d\n", runCount);
			SysPrintf("StartPC [%04x]\n", ((VURegs*)backVUregs)->VI[REG_TPC].UL);
			SysPrintf("-----------------------------------------------\n\n");

			SysPrintf("-----------------------------------------------\n");
			Console::Notice("Super VU / Micro VU");
			SysPrintf("-----------------------------------------------\n");

			for (int i = 0; i < 32; i++) {
				sprintf(str1, "VF%02d  = {%f, %f, %f, %f}", i, VU3.VF[i].F[0], VU3.VF[i].F[1], VU3.VF[i].F[2], VU3.VF[i].F[3]);
				sprintf(str2, "VF%02d  = {%f, %f, %f, %f}", i, VU1.VF[i].F[0], VU1.VF[i].F[1], VU1.VF[i].F[2], VU1.VF[i].F[3]);
				cmpPrint(((VU1.VF[i].UL[0] != VU3.VF[i].UL[0]) || (VU1.VF[i].UL[1] != VU3.VF[i].UL[1]) || (VU1.VF[i].UL[2] != VU3.VF[i].UL[2]) || (VU1.VF[i].UL[3] != VU3.VF[i].UL[3])));
			}

			sprintf(str1, "ACC   = {%f, %f, %f, %f}", VU3.ACC.F[0], VU3.ACC.F[1], VU3.ACC.F[2], VU3.ACC.F[3]);
			sprintf(str2, "ACC   = {%f, %f, %f, %f}", VU1.ACC.F[0], VU1.ACC.F[1], VU1.ACC.F[2], VU1.ACC.F[3]);
			cmpPrint(((VU1.ACC.F[0] != VU3.ACC.F[0]) || (VU1.ACC.F[1] != VU3.ACC.F[1]) || (VU1.ACC.F[2] != VU3.ACC.F[2]) || (VU1.ACC.F[3] != VU3.ACC.F[3])));

			for (int i = 0; i < 16; i++) {
				sprintf(str1, "VI%02d  = % 8d ($%08x)", i, (s16)VU3.VI[i].UL, VU3.VI[i].UL);
				sprintf(str2, "VI%02d  = % 8d ($%08x)", i, (s16)VU1.VI[i].UL, VU1.VI[i].UL);
				cmpPrint((VU1.VI[i].UL != VU3.VI[i].UL));
			}

			sprintf(str1, "Stat  = % 8d ($%08x)", (s16)VU3.VI[REG_STATUS_FLAG].UL,	VU3.VI[REG_STATUS_FLAG].UL);
			sprintf(str2, "Stat  = % 8d ($%08x)", (s16)VU1.VI[REG_STATUS_FLAG].UL,	VU1.VI[REG_STATUS_FLAG].UL);
			cmpPrint((VU1.VI[REG_STATUS_FLAG].UL != VU3.VI[REG_STATUS_FLAG].UL));

			sprintf(str1, "MAC   = % 8d ($%08x)", (s16)VU3.VI[REG_MAC_FLAG].UL,		VU3.VI[REG_MAC_FLAG].UL);
			sprintf(str2, "MAC   = % 8d ($%08x)", (s16)VU1.VI[REG_MAC_FLAG].UL,		VU1.VI[REG_MAC_FLAG].UL);
			cmpPrint((VU1.VI[REG_MAC_FLAG].UL != VU3.VI[REG_MAC_FLAG].UL));

			sprintf(str1, "CLIP  = % 8d ($%08x)", (s16)VU3.VI[REG_CLIP_FLAG].UL,	VU3.VI[REG_CLIP_FLAG].UL);
			sprintf(str2, "CLIP  = % 8d ($%08x)", (s16)VU1.VI[REG_CLIP_FLAG].UL,	VU1.VI[REG_CLIP_FLAG].UL);
			cmpPrint((VU1.VI[REG_CLIP_FLAG].UL != VU3.VI[REG_CLIP_FLAG].UL));

			sprintf(str1, "Q-reg = %f ($%08x)", VU3.VI[REG_Q].F, VU3.VI[REG_Q].UL);
			sprintf(str2, "Q-reg = %f ($%08x)", VU1.VI[REG_Q].F, VU1.VI[REG_Q].UL);
			cmpPrint((VU1.VI[REG_Q].UL != VU3.VI[REG_Q].UL));

			sprintf(str1, "P-reg = %f ($%08x)", VU3.VI[REG_P].F, VU3.VI[REG_P].UL);
			sprintf(str2, "P-reg = %f ($%08x)", VU1.VI[REG_P].F, VU1.VI[REG_P].UL);
			cmpPrint((VU1.VI[REG_P].UL != VU3.VI[REG_P].UL));

			sprintf(str1, "I-reg = %f ($%08x)", VU3.VI[REG_I].F, VU3.VI[REG_I].UL);
			sprintf(str2, "I-reg = %f ($%08x)", VU1.VI[REG_I].F, VU1.VI[REG_I].UL);
			cmpPrint((VU1.VI[REG_I].UL != VU3.VI[REG_I].UL));

			sprintf(str1, "_Stat = % 8d ($%08x)", (s16)VU3.statusflag,	VU3.statusflag);
			sprintf(str2, "_Stat = % 8d ($%08x)", (s16)VU1.statusflag,	VU1.statusflag);
			cmpPrint((VU1.statusflag != VU3.statusflag));

			sprintf(str1, "_MAC  = % 8d ($%08x)", (s16)VU3.macflag,		VU3.macflag);
			sprintf(str2, "_MAC  = % 8d ($%08x)", (s16)VU1.macflag,		VU1.macflag);
			cmpPrint((VU1.macflag != VU3.macflag));

			sprintf(str1, "_CLIP = % 8d ($%08x)", (s16)VU3.clipflag,	VU3.clipflag);
			sprintf(str2, "_CLIP = % 8d ($%08x)", (s16)VU1.clipflag,	VU1.clipflag);
			cmpPrint((VU1.clipflag != VU3.clipflag));

			u32 j = 0;
			u32 z = 0;
			for (int i = 0; i < (0x4000 / 4); i++) {
				j ^= ((u32*)(cmpVUmem))[i];
				z ^= ((u32*)(VU1.Mem)) [i];
			}
			sprintf(str1, "VU Mem CRC = 0x%08x", j);
			sprintf(str2, "VU Mem CRC = 0x%08x", z);
			cmpPrint((j != z));

			sprintf(str1, "EndPC = 0x%04x", VU3.VI[REG_TPC].UL);
			sprintf(str2, "EndPC = 0x%04x", VU1.VI[REG_TPC].UL);
			cmpPrint((VU1.VI[REG_TPC].UL != VU3.VI[REG_TPC].UL));

			SysPrintf("-----------------------------------------------\n\n");

			mVUdebugNow = 1;
			resetVUrec(1);
			memcpy_fast((u8*)&VU1,		(u8*)backVUregs,	sizeof(VURegs));
			memcpy_fast((u8*)VU1.Mem,	(u8*)backVUmem,		0x4000);

			runVUrec(VU1.VI[REG_TPC].UL, 300000 /*0x7fffffff*/, 1);

			for (int i = 0; i < 10000000; i++) {
				Sleep(1000);
			}
		}
		VUtestPause();
		FreezeXMMRegs(0);
	}
}
#else 

namespace VU1micro
{
	void recAlloc()		{ SuperVUAlloc(1);   initVUrec(&VU1, 1); }
	void recShutdown()	{ SuperVUDestroy(1); closeVUrec(1);}
	
	void __fastcall recClear(u32 Addr, u32 Size) { 
		if (useMVU1) clearVUrec(Addr, Size, 1); 
		else		 SuperVUClear(Addr, Size, 1);
	}

	static void recReset() { 
		if (useMVU1) resetVUrec(1); 
		else		 SuperVUReset(1);
		x86FpuState = FPU_STATE; 
	}

	static void recStep() {}
	static void recExecuteBlock() {

		if ((VU0.VI[REG_VPU_STAT].UL & 0x100) == 0) return;
		assert( (VU1.VI[REG_TPC].UL&7) == 0 );

#ifdef DEBUG_COMPARE
		SysPrintf("(%08d) StartPC = 0x%04x\n", runAmount, VU1.VI[REG_TPC].UL);
#endif

		FreezeXMMRegs(1);
		if (useMVU1) runVUrec(VU1.VI[REG_TPC].UL, 3000000, 1);
		else {
			if (VU1.VI[REG_TPC].UL >= VU1.maxmicro) { 
				Console::Error("VU1 memory overflow!!: %x", params VU1.VI[REG_TPC].UL); 
			}
			do { // while loop needed since not always will return finished
				SuperVUExecuteProgram(VU1.VI[REG_TPC].UL & 0x3fff, 1);
			} while( VU0.VI[REG_VPU_STAT].UL&0x100 );
		}
		FreezeXMMRegs(0);
		VUtestPause();
	}
}
#endif

using namespace VU1micro;

const VUmicroCpu recVU1 = 
{
	recReset
,	recStep
,	recExecuteBlock
,	recClear
};
