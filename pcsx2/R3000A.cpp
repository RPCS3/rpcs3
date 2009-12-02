/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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


#include "PrecompiledHeader.h"
#include "IopCommon.h"

#include "Sio.h"
#include "Sif.h"

using namespace R3000A;

R3000Acpu *psxCpu;

// used for constant propagation
u32 g_psxConstRegs[32];
u32 g_psxHasConstReg, g_psxFlushedConstReg;

// Controls when branch tests are performed.
u32 g_psxNextBranchCycle = 0;

// This value is used when the IOP execution is broken to return control to the EE.
// (which happens when the IOP throws EE-bound interrupts).  It holds the value of
// psxCycleEE (which is set to zero to facilitate the code break), so that the unrun
// cycles can be accounted for later.
s32 psxBreak = 0;

// tracks the IOP's current sync status with the EE.  When it dips below zero,
// control is returned to the EE.
s32 psxCycleEE = -1;

// Used to signal to the EE when important actions that need IOP-attention have
// happened (hsyncs, vsyncs, IOP exceptions, etc).  IOP runs code whenever this
// is true, even if it's already running ahead a bit.
bool iopBranchAction = false;

bool iopEventTestIsActive = false;

__aligned16 psxRegisters psxRegs;

void psxReset()
{
	memzero(psxRegs);

	psxRegs.pc = 0xbfc00000; // Start in bootstrap
	psxRegs.CP0.n.Status = 0x10900000; // COP0 enabled | BEV = 1 | TS = 1
	psxRegs.CP0.n.PRid   = 0x0000001f; // PRevID = Revision ID, same as the IOP R3000A

	psxBreak = 0;
	psxCycleEE = -1;
	g_psxNextBranchCycle = psxRegs.cycle + 4;

	psxHwReset();
	psxBiosInit();
}

void psxShutdown() {
	psxBiosShutdown();
	//psxCpu->Shutdown();
}

void __fastcall psxException(u32 code, u32 bd)
{
//	PSXCPU_LOG("psxException %x: %x, %x", code, psxHu32(0x1070), psxHu32(0x1074));
	//Console.WriteLn("!! psxException %x: %x, %x", code, psxHu32(0x1070), psxHu32(0x1074));
	// Set the Cause
	psxRegs.CP0.n.Cause &= ~0x7f;
	psxRegs.CP0.n.Cause |= code;

	// Set the EPC & PC
	if (bd)
	{
		PSXCPU_LOG("bd set");
		psxRegs.CP0.n.Cause|= 0x80000000;
		psxRegs.CP0.n.EPC = (psxRegs.pc - 4);
	}
	else
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

	if (!CHECK_IOPREC)
	{
		// HLE Bios Handlers, enabled for interpreters only.

		u32 call = psxRegs.GPR.n.t1 & 0xff;
		switch (psxRegs.pc & 0x1fffff) {
			case 0xa0:

				if (call != 0x28 && call != 0xe)
					PSXBIOS_LOG("Bios call a0: %s (%x) %x,%x,%x,%x", biosA0n[call], call, psxRegs.GPR.n.a0, psxRegs.GPR.n.a1, psxRegs.GPR.n.a2, psxRegs.GPR.n.a3);

				if (biosA0[call])
			   		biosA0[call]();
				break;

			case 0xb0:
				if (call != 0x17 && call != 0xb)
					PSXBIOS_LOG("Bios call b0: %s (%x) %x,%x,%x,%x", biosB0n[call], call, psxRegs.GPR.n.a0, psxRegs.GPR.n.a1, psxRegs.GPR.n.a2, psxRegs.GPR.n.a3);

				if (biosB0[call])
			   		biosB0[call]();
				break;

			case 0xc0:
				PSXBIOS_LOG("Bios call c0: %s (%x) %x,%x,%x,%x", biosC0n[call], call, psxRegs.GPR.n.a0, psxRegs.GPR.n.a1, psxRegs.GPR.n.a2, psxRegs.GPR.n.a3);
			
				if (biosC0[call])
			   		biosC0[call]();
				break;
		}
	}

	/*if (psxRegs.CP0.n.Cause == 0x400 && (!(psxHu32(0x1450) & 0x8))) {
		hwIntcIrq(INTC_SBUS);
	}*/
}

__forceinline void psxSetNextBranch( u32 startCycle, s32 delta )
{
	// typecast the conditional to signed so that things don't blow up
	// if startCycle is greater than our next branch cycle.

	if( (int)(g_psxNextBranchCycle - startCycle) > delta )
		g_psxNextBranchCycle = startCycle + delta;
}

__forceinline void psxSetNextBranchDelta( s32 delta )
{
	psxSetNextBranch( psxRegs.cycle, delta );
}

__forceinline int psxTestCycle( u32 startCycle, s32 delta )
{
	// typecast the conditional to signed so that things don't explode
	// if the startCycle is ahead of our current cpu cycle.

	return (int)(psxRegs.cycle - startCycle) >= delta;
}

__forceinline void PSX_INT( IopEventId n, s32 ecycle )
{
	// Generally speaking games shouldn't throw ints that haven't been cleared yet.
	// It's usually indicative of something amiss in our emulation, so uncomment this
	// code to help trap those sort of things.

	// Exception: IRQ16 - SIO - it drops ints like crazy when handling PAD stuff.
	//if( /*n!=16 &&*/ psxRegs.interrupt & (1<<n) )
	//	Console.WriteLn( "***** IOP > Twice-thrown int on IRQ %d", n );

	psxRegs.interrupt |= 1 << n;

	psxRegs.sCycle[n] = psxRegs.cycle;
	psxRegs.eCycle[n] = ecycle;

	psxSetNextBranchDelta( ecycle );

	if( psxCycleEE < 0 )
	{
		// The EE called this int, so inform it to branch as needed:
		// fixme - this doesn't take into account EE/IOP sync (the IOP may be running
		// ahead or behind the EE as per the EEsCycles value)
		s32 iopDelta = (g_psxNextBranchCycle-psxRegs.cycle)*8;
		cpuSetNextBranchDelta( iopDelta );
	}
}

static __forceinline void IopTestEvent( IopEventId n, void (*callback)() )
{
	if( !(psxRegs.interrupt & (1 << n)) ) return;

	if( psxTestCycle( psxRegs.sCycle[n], psxRegs.eCycle[n] ) )
	{
		psxRegs.interrupt &= ~(1 << n);
		callback();
	}
	else
		psxSetNextBranch( psxRegs.sCycle[n], psxRegs.eCycle[n] );
}

// Change to 1 to enable SIF wakeup hack:
#define IOP_ENABLE_SIF_HACK 0

static __forceinline void sifHackInterrupt()
{
	// No reason -- just that sometimes the SIF fell asleep, and this wakes it up.

	hwIntcIrq(INTC_SBUS);
	//PSX_INT( IopEvt_SIFhack, 128 );
}

static __forceinline void _psxTestInterrupts()
{
	IopTestEvent(IopEvt_SIF0,		sif0Interrupt);	// SIF0
	IopTestEvent(IopEvt_SIF1,		sif1Interrupt);	// SIF1
	IopTestEvent(IopEvt_SIO,		sioInterrupt);
	IopTestEvent(IopEvt_CdvdRead,	cdvdReadInterrupt);

#if IOP_ENABLE_SIF_HACK
	IopTestEvent(IopEvt_SIFhack,	sifHackInterrupt);
#endif

	// Profile-guided Optimization (sorta)
	// The following ints are rarely called.  Encasing them in a conditional
	// as follows helps speed up most games.

	if( psxRegs.interrupt & ( (1ul<<5) | (3ul<<11) | (3ul<<20) | (3ul<<17) ) )
	{
		IopTestEvent(IopEvt_Cdvd,		cdvdActionInterrupt);
		IopTestEvent(IopEvt_Dma11,		psxDMA11Interrupt);	// SIO2
		IopTestEvent(IopEvt_Dma12,		psxDMA12Interrupt);	// SIO2
		IopTestEvent(IopEvt_Cdrom,		cdrInterrupt);
		IopTestEvent(IopEvt_CdromRead,	cdrReadInterrupt);
		IopTestEvent(IopEvt_DEV9,		dev9Interrupt);
		IopTestEvent(IopEvt_USB,		usbInterrupt);
	}
}

__releaseinline void psxBranchTest()
{
	if( psxTestCycle( psxNextsCounter, psxNextCounter ) )
	{
		psxRcntUpdate();
		iopBranchAction = true;
	}
	else
	{
	// start the next branch at the next counter event by default
	// the interrupt code below will assign nearer branches if needed.
		g_psxNextBranchCycle = psxNextsCounter+psxNextCounter;
	}
	
	
	if (psxRegs.interrupt)
	{
		iopEventTestIsActive = true;
		_psxTestInterrupts();
		iopEventTestIsActive = false;
	}

	if( psxHu32(0x1078) == 0 ) return;
	if( (psxHu32(0x1070) & psxHu32(0x1074)) == 0 ) return;

	if ((psxRegs.CP0.n.Status & 0xFE01) >= 0x401)
	{
		PSXCPU_LOG("Interrupt: %x  %x", psxHu32(0x1070), psxHu32(0x1074));
		psxException(0, 0);
		iopBranchAction = true;

		// No need to execute the SIFhack after cpuExceptions, since these by nature break SIF's
		// thread sleep hangs and allow the IOP to "come back to life."
		psxRegs.interrupt &= ~IopEvt_SIFhack;
	}
	else if( IopEvt_SIFhack && (psxRegs.interrupt & IopEvt_SIFhack) )
	{
		// Safeguard: since we're not executing an exception vector, we should schedule a SIF wakeup
		// just in case.

		// (TODO: The endless loop in question is a branch instruction that branches to itself endlessly,
		//  waiting for SIF to wake it up via any cpuException.  We could check for that instruction
		//  location and only schedule a SIF fix when it's detected...  But for now this is easy and gives
		//  us good control over testing parameters...)

		PSX_INT( IopEvt_SIFhack, 128 );
	}
}

void iopTestIntc()
{
	if( psxHu32(0x1078) == 0 ) return;
	if( (psxHu32(0x1070) & psxHu32(0x1074)) == 0 ) return;

	if( !eeEventTestIsActive )
	{
		// An iop exception has occurred while the EE is running code.
		// Inform the EE to branch so the IOP can handle it promptly:

		cpuSetNextBranchDelta( 16 );
		iopBranchAction = true;
		//Console.Error( "** IOP Needs an EE EventText, kthx **  %d", psxCycleEE );

		// Note: No need to set the iop's branch delta here, since the EE
		// will run an IOP branch test regardless.
	}
	else if( !iopEventTestIsActive )
		psxSetNextBranchDelta( 2 );
}

