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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "PsxCommon.h"
#include "Misc.h"

// used for constant propagation
R3000Acpu *psxCpu;
u32 g_psxConstRegs[32];
u32 g_psxHasConstReg, g_psxFlushedConstReg;
u32 g_psxNextBranchCycle = 0;

PCSX2_ALIGNED16(psxRegisters psxRegs);

int psxInit()
{
	psxCpu = CHECK_EEREC ? &psxRec : &psxInt;

#ifdef PCSX2_DEVBUILD
	Log=0;
#endif

	if (psxMemInit() == -1) return -1;

	return psxCpu->Init();
}

void psxReset() {

	psxCpu->Reset();

	psxMemReset();

	memset(&psxRegs, 0, sizeof(psxRegs));

	psxRegs.pc = 0xbfc00000; // Start in bootstrap

	psxRegs.CP0.n.Status = 0x10900000; // COP0 enabled | BEV = 1 | TS = 1
	psxRegs.CP0.n.PRid   = 0x0000001f; // PRevID = Revision ID, same as the IOP R3000A

	psxHwReset();
	psxBiosInit();
	psxExecuteBios();
}

void psxShutdown() {
	psxMemShutdown();
	psxBiosShutdown();
	psxSIOShutdown();
	psxCpu->Shutdown();
}

void psxException(u32 code, u32 bd) {
//	PSXCPU_LOG("psxException %x: %x, %x\n", code, psxHu32(0x1070), psxHu32(0x1074));
//	SysPrintf("psxException %x: %x, %x\n", code, psxHu32(0x1070), psxHu32(0x1074));
	// Set the Cause
	psxRegs.CP0.n.Cause &= ~0x7f;
	psxRegs.CP0.n.Cause |= code;

#ifdef PSXCPU_LOG
	if (bd) { PSXCPU_LOG("bd set\n"); }
#endif
	// Set the EPC & PC
	if (bd) {
		psxRegs.CP0.n.Cause|= 0x80000000;
		psxRegs.CP0.n.EPC = (psxRegs.pc - 4);
	} else
		psxRegs.CP0.n.EPC = (psxRegs.pc);

	if (psxRegs.CP0.n.Status & 0x400000)
		psxRegs.pc = 0xbfc00180;
	else
		psxRegs.pc = 0x80000080;

	// Set the Status
	psxRegs.CP0.n.Status = (psxRegs.CP0.n.Status &~0x3f) |
						  ((psxRegs.CP0.n.Status & 0xf) << 2);

	/*if ((((PSXMu32(psxRegs.CP0.n.EPC) >> 24) & 0xfe) == 0x4a)) {
		// "hokuto no ken" / "Crash Bandicot 2" ... fix
		PSXMu32(psxRegs.CP0.n.EPC)&= ~0x02000000;
	}*/

	if (Config.PsxOut && !CHECK_EEREC) {
		u32 call = psxRegs.GPR.n.t1 & 0xff;
		switch (psxRegs.pc & 0x1fffff) {
			case 0xa0:
#ifdef PSXBIOS_LOG
				if (call != 0x28 && call != 0xe) {
					PSXBIOS_LOG("Bios call a0: %s (%x) %x,%x,%x,%x\n", biosA0n[call], call, psxRegs.GPR.n.a0, psxRegs.GPR.n.a1, psxRegs.GPR.n.a2, psxRegs.GPR.n.a3); }
#endif
				if (biosA0[call])
			   		biosA0[call]();
				break;
			case 0xb0:
#ifdef PSXBIOS_LOG
				if (call != 0x17 && call != 0xb) {
					PSXBIOS_LOG("Bios call b0: %s (%x) %x,%x,%x,%x\n", biosB0n[call], call, psxRegs.GPR.n.a0, psxRegs.GPR.n.a1, psxRegs.GPR.n.a2, psxRegs.GPR.n.a3); }
#endif
				if (biosB0[call])
			   		biosB0[call]();
				break;
			case 0xc0:
#ifdef PSXBIOS_LOG
				PSXBIOS_LOG("Bios call c0: %s (%x) %x,%x,%x,%x\n", biosC0n[call], call, psxRegs.GPR.n.a0, psxRegs.GPR.n.a1, psxRegs.GPR.n.a2, psxRegs.GPR.n.a3);
#endif
				if (biosC0[call])
			   		biosC0[call]();
				break;
		}
	}

	/*if (psxRegs.CP0.n.Cause == 0x400 && (!(psxHu32(0x1450) & 0x8))) {
		hwIntcIrq(1);
	}*/
}

#define PSX_TESTINT(n, callback) \
	if (psxRegs.interrupt & (1 << n)) { \
		if ((int)(psxRegs.cycle - psxRegs.sCycle[n]) >= psxRegs.eCycle[n]) { \
			callback(); \
		} \
		else if( (int)(g_psxNextBranchCycle - psxRegs.sCycle[n]) > psxRegs.eCycle[n] ) \
			g_psxNextBranchCycle = psxRegs.sCycle[n] + psxRegs.eCycle[n]; \
	}

static void _psxTestInterrupts() {
    // uncommenting until refraction can give a good reason as to why it shouldn't be here
	// Good reason given, why have something in there we dont use anymore?
	/*PSX_TESTINT(4, psxDma4Interrupt);
	PSX_TESTINT(7, psxDma7Interrupt);*/

	PSX_TESTINT(9, sif0Interrupt);	// SIF0
	PSX_TESTINT(10, sif1Interrupt);	// SIF1
	PSX_TESTINT(11, psxDMA11Interrupt);	// SIO2
	PSX_TESTINT(12, psxDMA12Interrupt);	// SIO2
	PSX_TESTINT(16, sioInterrupt);
	PSX_TESTINT(17, cdrInterrupt);
	PSX_TESTINT(18, cdrReadInterrupt);
	PSX_TESTINT(19, cdvdReadInterrupt);
	PSX_TESTINT(20, dev9Interrupt);
	PSX_TESTINT(21, usbInterrupt);
}

#define IOP_WAIT_CYCLE 64

void psxBranchTest()
{
	EEsCycle -= (psxRegs.cycle - IOPoCycle) << 3;
	IOPoCycle = psxRegs.cycle;
	if( EEsCycle > 0 )
		g_psxNextBranchCycle = psxRegs.cycle + min(IOP_WAIT_CYCLE, (EEsCycle>>3));
	else
		g_psxNextBranchCycle = psxRegs.cycle;

	if ((int)(psxRegs.cycle - psxNextsCounter) >= psxNextCounter)
		psxRcntUpdate();

	if (psxRegs.interrupt) {
		_psxTestInterrupts();
	}

//	if( (int)psxRegs.cycle-(int)g_psxNextBranchCycle > 0 )
//		g_psxNextBranchCycle = psxRegs.cycle+1;
//	else
	if( (int)(g_psxNextBranchCycle-psxNextsCounter) >= (u32)psxNextCounter )
		g_psxNextBranchCycle = (u32)psxNextsCounter+(u32)psxNextCounter;

	if (psxHu32(0x1078)) {
		if(psxHu32(0x1070) & psxHu32(0x1074)){
			if ((psxRegs.CP0.n.Status & 0xFE01) >= 0x401) {
//#ifdef PSXCPU_LOG
//			PSXCPU_LOG("Interrupt: %x  %x\n", HWMu32(0x1070), HWMu32(0x1074));
//#endif
				psxException(0, 0);
			}
		}
	}
}

void psxExecuteBios() {
/*	while (psxRegs.pc != 0x80030000)
		psxCpu->ExecuteBlock();
#ifdef PSX_LOG
	PSX_LOG("*BIOS END*\n");
#endif*/
}

void psxRestartCPU()
{
	psxCpu->Shutdown();
	psxCpu = CHECK_EEREC ? &psxRec : &psxInt;

	if (psxCpu->Init() == -1) {
		SysClose();
		exit(1);
	}
	psxCpu->Reset();
}
