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
#include "Counters.h"

#include "Memory.h"
#include "Hw.h"
#include "DebugTools/Debug.h"
#include "R3000A.h"
#include "VUmicro.h"
#include "COP0.h"

#include "GS.h"
#include "IPU/IPU.h"
#include "Vif.h"
#include "VifDma.h"
#include "SPR.h"
#include "Sif.h"

#include "Paths.h"

#include "R5900Exceptions.h"

using namespace R5900;	// for R5900 disasm tools

s32 EEsCycle;		// used to sync the IOP to the EE
u32 EEoCycle;

PCSX2_ALIGNED16(cpuRegisters cpuRegs);
PCSX2_ALIGNED16(fpuRegisters fpuRegs);
PCSX2_ALIGNED16(tlbs tlb[48]);
R5900cpu *Cpu = NULL;

u32 bExecBIOS = 0; // set if the BIOS has already been executed

static bool cpuIsInitialized = false;
static uint eeWaitCycles = 1024;

bool eeEventTestIsActive = false;

R5900Exception::BaseExcept::~BaseExcept() throw (){}


void cpuReset()
{
	mtgsWaitGS();		// GS better be done processing before we reset the EE, just in case.
	cpuIsInitialized = true;

	memReset();
	psxMemReset();
	vuMicroMemReset();

	memzero_obj(cpuRegs);
	memzero_obj(fpuRegs);
	memzero_obj(tlb);

	cpuRegs.pc = 0xbfc00000; ///set pc reg to stack 
	cpuRegs.CP0.n.Config = 0x440;
	cpuRegs.CP0.n.Status.val = 0x70400004; //0x10900000 <-- wrong; // COP0 enabled | BEV = 1 | TS = 1
	cpuRegs.CP0.n.PRid   = 0x00002e20; // PRevID = Revision ID, same as R5900
	fpuRegs.fprc[0]   = 0x00002e00; // fpu Revision..
	fpuRegs.fprc[31]  = 0x01000001; // fpu Status/Control

	g_nextBranchCycle = cpuRegs.cycle + 4;
	EEsCycle = 0;
	EEoCycle = cpuRegs.cycle;
	eeWaitCycles = CHECK_WAITCYCLE_HACK ? 3072 : 768;

	// Cyclerate hacks effectively speed up the rate of event tests, so we can safely boost
	// the WaitCycles value here for x2 and x3 modes:
	if( CHECK_EE_CYCLERATE > 1 )
		eeWaitCycles += 1024;

	hwReset();
	vif0Reset();
	vif1Reset();
	rcntInit();
	psxReset();
}

void cpuShutdown()
{
	mtgsWaitGS();

	hwShutdown();
//	biosShutdown();
	psxShutdown();
	disR5900FreeSyms();
}

void cpuException(u32 code, u32 bd)
{
	cpuRegs.branch = 0;		// Tells the interpreter that an exception occurred during a branch.
	bool errLevel2, checkStatus;
	u32 offset;
	cpuRegs.CP0.n.Cause = code & 0xffff;

	if(cpuRegs.CP0.n.Status.b.ERL == 0)
	{
		//Error Level 0-1
		errLevel2 = FALSE;
		checkStatus = (cpuRegs.CP0.n.Status.b.BEV == 0); //  for TLB/general exceptions
		
		if (((code & 0x7C) >= 0x8) && ((code & 0x7C) <= 0xC)) 
			offset = 0x0; //TLB Refill
		else if ((code & 0x7C) == 0x0) 
			offset = 0x200; //Interrupt
		else	
			offset = 0x180; // Everything else
	}
	else
	{
		//Error Level 2
		errLevel2 = TRUE;
		checkStatus = (cpuRegs.CP0.n.Status.b.DEV == 0); // for perf/debug exceptions
		
		Console::Error("*PCSX2* FIX ME: Level 2 cpuException");
		if ((code & 0x38000) <= 0x8000 ) 
		{
			//Reset / NMI
			cpuRegs.pc = 0xBFC00000;
			Console::Notice("Reset request");
			UpdateCP0Status();
			return;
		} 
		else if((code & 0x38000) == 0x10000) 
			offset = 0x80; //Performance Counter
		else if((code & 0x38000) == 0x18000)  
			offset = 0x100; //Debug
		else 
			Console::Error("Unknown Level 2 Exception!! Cause %x", params code);
	}
	
	if (cpuRegs.CP0.n.Status.b.EXL == 0) 
	{
		cpuRegs.CP0.n.Status.b.EXL = 1;
		if (bd) 
		{
			Console::Notice("branch delay!!");
			cpuRegs.CP0.n.EPC = cpuRegs.pc - 4;
			cpuRegs.CP0.n.Cause |= 0x80000000;
		} 
		else 
		{
			cpuRegs.CP0.n.EPC = cpuRegs.pc;
			cpuRegs.CP0.n.Cause &= ~0x80000000;
		}
	} 
	else 
	{
		offset = 0x180; //Override the cause		
		if (errLevel2) Console::Notice("cpuException: Status.EXL = 1 cause %x", params code);
	}
	
	if (checkStatus)
		cpuRegs.pc = 0x80000000 + offset;
	else 
		cpuRegs.pc = 0xBFC00200 + offset;
	
	UpdateCP0Status();
}

void cpuTlbMiss(u32 addr, u32 bd, u32 excode) 
{
	Console::Error("cpuTlbMiss pc:%x, cycl:%x, addr: %x, status=%x, code=%x",
		params cpuRegs.pc, cpuRegs.cycle, addr, cpuRegs.CP0.n.Status.val, excode);
		
	if (bd) Console::Notice("branch delay!!");

	assert(0); // temporary

	cpuRegs.CP0.n.BadVAddr = addr;
	cpuRegs.CP0.n.Context &= 0xFF80000F;
	cpuRegs.CP0.n.Context |= (addr >> 9) & 0x007FFFF0;
	cpuRegs.CP0.n.EntryHi = (addr & 0xFFFFE000) | (cpuRegs.CP0.n.EntryHi & 0x1FFF);

	cpuRegs.CP0.n.Cause = excode;
	if (!(cpuRegs.CP0.n.Status.val & 0x2)) { // EXL bit
		cpuRegs.CP0.n.EPC = cpuRegs.pc - 4;
	}

	if ((cpuRegs.CP0.n.Status.val & 0x1) == 0) {
		cpuRegs.pc = 0x80000000;
	} else {
		cpuRegs.pc = 0x80000180;
	}

	cpuRegs.CP0.n.Status.b.EXL = 1;
	UpdateCP0Status();
//	Log=1; varLog|= 0x40000000;
}

void cpuTlbMissR(u32 addr, u32 bd) {
	cpuTlbMiss(addr, bd, EXC_CODE_TLBL);
}

void cpuTlbMissW(u32 addr, u32 bd) {
	cpuTlbMiss(addr, bd, EXC_CODE_TLBS);
}

__forceinline void _cpuTestMissingINTC() {
	if (cpuRegs.CP0.n.Status.val & 0x400 &&
		psHu32(INTC_STAT) & psHu32(INTC_MASK)) {
		if ((cpuRegs.interrupt & (1 << 30)) == 0) {
			SysPrintf("*PCSX2*: Error, missing INTC Interrupt\n");
		}
	}
}

__forceinline void _cpuTestMissingDMAC() {
	if (cpuRegs.CP0.n.Status.val & 0x800 &&
		(psHu16(0xe012) & psHu16(0xe010) || 
		 psHu16(0xe010) & 0x8000)) {
		if ((cpuRegs.interrupt & (1 << 31)) == 0) {
			SysPrintf("*PCSX2*: Error, missing DMAC Interrupt\n");
		}
	}
}

void cpuTestMissingHwInts() {
	if ((cpuRegs.CP0.n.Status.val & 0x10007) == 0x10001) {
		_cpuTestMissingINTC();
		_cpuTestMissingDMAC();
//		_cpuTestTIMR();
	}
}

// sets a branch test to occur some time from an arbitrary starting point.
__forceinline int cpuSetNextBranch( u32 startCycle, s32 delta )
{
	// typecast the conditional to signed so that things don't blow up
	// if startCycle is greater than our next branch cycle.

	if( (int)(g_nextBranchCycle - startCycle) > delta )
	{
		g_nextBranchCycle = startCycle + delta;
		return 1;
	}
	return 0;
}

// sets a branch to occur some time from the current cycle
__forceinline int cpuSetNextBranchDelta( s32 delta )
{
	return cpuSetNextBranch( cpuRegs.cycle, delta );
}

// tests the cpu cycle agaisnt the given start and delta values.
// Returns true if the delta time has passed.
__forceinline int cpuTestCycle( u32 startCycle, s32 delta )
{
	// typecast the conditional to signed so that things don't explode
	// if the startCycle is ahead of our current cpu cycle.

	return (int)(cpuRegs.cycle - startCycle) >= delta;
}

// tells the EE to run the branch test the next time it gets a chance.
__forceinline void cpuSetBranch()
{
	g_nextBranchCycle = cpuRegs.cycle;
}

void cpuClearInt( uint i )
{
	jASSUME( i < 32 );
	cpuRegs.interrupt &= ~(1 << i);
}

static __forceinline void TESTINT( u8 n, void (*callback)() )
{
	if( !(cpuRegs.interrupt & (1 << n)) ) return;

	if( cpuTestCycle( cpuRegs.sCycle[n], cpuRegs.eCycle[n] ) )
	{
		cpuClearInt( n );
		callback();
	}
	else
		cpuSetNextBranch( cpuRegs.sCycle[n], cpuRegs.eCycle[n] );
}

static __forceinline void _cpuTestInterrupts()
{
	/* These are 'pcsx2 interrupts', they handle asynchronous stuff
	   that depends on the cycle timings */

	TESTINT(1, vif1Interrupt);
	TESTINT(2, gsInterrupt);
	TESTINT(5, EEsif0Interrupt);
	TESTINT(6, EEsif1Interrupt);

	// Profile-guided Optimization (sorta)
	// The following ints are rarely called.  Encasing them in a conditional
	// as follows helps speed up most games.

	if( cpuRegs.interrupt & ( 1 | (3 << 3) | (3<<8) | (3<<10)) )
	{
		TESTINT(0, vif0Interrupt);
#ifndef IPU_INLINE_IRQS
		TESTINT(3, ipu0Interrupt);
		TESTINT(4, ipu1Interrupt);
#endif
		TESTINT(8, SPRFROMinterrupt);
		TESTINT(9, SPRTOinterrupt);

		TESTINT(10, vifMFIFOInterrupt);
		TESTINT(11, gifMFIFOInterrupt);
	}
}

static __forceinline void _cpuTestTIMR()
{
	cpuRegs.CP0.n.Count += cpuRegs.cycle-s_iLastCOP0Cycle;
	s_iLastCOP0Cycle = cpuRegs.cycle;

	// fixme: this looks like a hack to make up for the fact that the TIMR
	// doesn't yet have a proper mechanism for setting itself up on a nextBranchCycle.
	// A proper fix would schedule the TIMR to trigger at a specific cycle anytime
	// the Count or Compare registers are modified.

	if ( (cpuRegs.CP0.n.Status.val & 0x8000) &&
		cpuRegs.CP0.n.Count >= cpuRegs.CP0.n.Compare && cpuRegs.CP0.n.Count < cpuRegs.CP0.n.Compare+1000 )
	{
		Console::Status("timr intr: %x, %x", params cpuRegs.CP0.n.Count, cpuRegs.CP0.n.Compare);
		cpuException(0x808000, cpuRegs.branch);
	}
}

static __forceinline void _cpuTestPERF()
{
	// Perfs are updated when read by games (COP0's MFC0/MTC0 instructions), so we need
	// only update them at semi-regular intervals to keep cpuRegs.cycle from wrapping
	// around twice on us btween updates.  Hence this function is called from the cpu's
	// Counters update.

	COP0_UpdatePCCR();
}

// Checks the COP0.Status for exception enablings.
// Exception handling for certain modes is *not* currently supported, this function filters
// them out.  Exceptions while the exception handler is active (EIE), or exceptions of any
// level other than 0 are ignored here.

static bool cpuIntsEnabled()
{
	return cpuRegs.CP0.n.Status.b.EIE && cpuRegs.CP0.n.Status.b.IE && 
		!cpuRegs.CP0.n.Status.b.EXL && (cpuRegs.CP0.n.Status.b.ERL == 0);
}

// if cpuRegs.cycle is greater than this cycle, should check cpuBranchTest for updates
u32 g_nextBranchCycle = 0;

// Shared portion of the branch test, called from both the Interpreter
// and the recompiler.  (moved here to help alleviate redundant code)
__forceinline bool _cpuBranchTest_Shared()
{
	eeEventTestIsActive = true;
	g_nextBranchCycle = cpuRegs.cycle + eeWaitCycles;

	EEsCycle += cpuRegs.cycle - EEoCycle;
	EEoCycle = cpuRegs.cycle;

	if( EEsCycle > 0 )
		iopBranchAction = true;

	// ---- Counters -------------
	bool vsyncEvent = false;
	rcntUpdate_hScanline();

	if( cpuTestCycle( nextsCounter, nextCounter ) )
	{
		vsyncEvent = rcntUpdate();
		_cpuTestPERF();
	}

	_cpuTestTIMR();

	// ---- Interrupts -------------
	// Handles all interrupts except 30 and 31, which are handled later.

	if( cpuRegs.interrupt & ~(3<<30) )
		_cpuTestInterrupts();

	// ---- IOP -------------
	// * It's important to run a psxBranchTest before calling ExecuteBlock. This
	//   is because the IOP does not always perform branch tests before returning
	//   (during the prev branch) and also so it can act on the state the EE has
	//   given it before executing any code.
	//
	// * The IOP cannot always be run.  If we run IOP code every time through the
	//   cpuBranchTest, the IOP generally starts to run way ahead of the EE.

	psxBranchTest();

	if( iopBranchAction )
	{
		//if( EEsCycle < -450 )
		//	Console::WriteLn( " IOP ahead by: %d cycles", params -EEsCycle );

		// Experimental and Probably Unnecessry Logic -->
		// Check if the EE already has an exception pending, and if so we shouldn't
		// waste too much time updating the IOP.  Theory being that the EE and IOP should
		// run closely in sync during raised exception events.  But in practice it didn't
		// seem to make much of a difference.

		// Note: The IOP is very good about chaining blocks together so it tends to
		// run lots of cycles, even with only 32 (4 IOP) cycles specified here.  That's
		// probably why it doesn't improve sync much.

		/*bool eeExceptPending = cpuIntsEnabled() &&
			//( cpuRegs.CP0.n.Status.b.EIE && cpuRegs.CP0.n.Status.b.IE && (cpuRegs.CP0.n.Status.b.ERL == 0) ) &&
			//( (cpuRegs.CP0.n.Status.val & 0x10007) == 0x10001 ) &&
			( (cpuRegs.interrupt & (3<<30)) != 0 );

		if( eeExceptPending )
		{
			// ExecuteBlock returns a negative value, so subtract it from the cycle count
			// specified to get the total cycles processed! :D
			int cycleCount = std::min( EEsCycle, (s32)(eeWaitCycles>>4) );
			int cyclesRun = cycleCount - psxCpu->ExecuteBlock( cycleCount );
			EEsCycle -= cyclesRun;
			//Console::Notice( "IOP Exception-Pending Execution -- EEsCycle: %d", params EEsCycle );
		}
		else*/
		{
			EEsCycle = psxCpu->ExecuteBlock( EEsCycle );
		}

		iopBranchAction = false;
	}

	// ---- VU0 -------------

	if (VU0.VI[REG_VPU_STAT].UL & 0x1)
	{
		// We're in a BranchTest.  All dynarec registers are flushed
		// so there is no need to freeze registers here.
		CpuVU0.ExecuteBlock();

		// This might be needed to keep the EE and VU0 in sync.
		// A better fix will require hefty changes to the VU recs. -_-
		if(VU0.VI[REG_VPU_STAT].UL & 0x1)
			cpuSetNextBranchDelta( 768 );
	}

	// Note:  We don't update the VU1 here because it runs it's micro-programs in
	// one shot always.  That is, when a program is executed the VU1 doesn't even
	// bother to return until the program is completely finished.

	// ---- Schedule Next Event Test --------------

	if( EEsCycle > 192 )
	{
		// EE's running way ahead of the IOP still, so we should branch quickly to give the
		// IOP extra timeslices in short order.

		cpuSetNextBranchDelta( 48 );
		//Console::Notice( "EE ahead of the IOP -- Rapid Branch!  %d", params EEsCycle );
	}

	// The IOP could be running ahead/behind of us, so adjust the iop's next branch by its
	// relative position to the EE (via EEsCycle)
	cpuSetNextBranchDelta( ((g_psxNextBranchCycle-psxRegs.cycle)*8) - EEsCycle );

	// Apply the hsync counter's nextCycle
	cpuSetNextBranch( hsyncCounter.sCycle, hsyncCounter.CycleT );

	// Apply vsync and other counter nextCycles
	cpuSetNextBranch( nextsCounter, nextCounter );

	eeEventTestIsActive = false;

	// ---- INTC / DMAC Exceptions -----------------
	// Raise the INTC and DMAC interrupts here, which usually throw exceptions.
	// This should be done last since the IOP and the VU0 can raise several EE
	// exceptions.

	//if ((cpuRegs.CP0.n.Status.val & 0x10007) == 0x10001)
	if( cpuIntsEnabled() )
	{
		TESTINT(30, intcInterrupt);
		TESTINT(31, dmacInterrupt);
	}

	return vsyncEvent;
}

void cpuTestINTCInts()
{
	if( cpuRegs.interrupt & (1 << 30) ) return;
	//if( (cpuRegs.CP0.n.Status.val & 0x10407) != 0x10401 ) return;
	if( !cpuIntsEnabled() ) return;
	if( (psHu32(INTC_STAT) & psHu32(INTC_MASK)) == 0 ) return;

	cpuRegs.interrupt|= 1 << 30;
	cpuRegs.sCycle[30] = cpuRegs.cycle;
	cpuRegs.eCycle[30] = 4;  //Needs to be 4 to account for bus delays/pipelines etc

	// only set the next branch delta if the exception won't be handled for
	// the current branch...
	if( !eeEventTestIsActive )
		cpuSetNextBranchDelta( 4 );
	else if(psxCycleEE > 0)
	{
		psxBreak += psxCycleEE;		// record the number of cycles the IOP didn't run.
		psxCycleEE = 0;
	}
}

__forceinline void cpuTestDMACInts()
{
	if ( cpuRegs.interrupt & (1 << 31) ) return;
	if ((cpuRegs.CP0.n.Status.val & 0x10807) != 0x10801) return;

	if ( ( (psHu16(0xe012) & psHu16(0xe010)) == 0) && 
		 ( (psHu16(0xe010) & 0x8000) == 0) ) return;

	cpuRegs.interrupt|= 1 << 31;
	cpuRegs.sCycle[31] = cpuRegs.cycle;
	cpuRegs.eCycle[31] = 4;  //Needs to be 4 to account for bus delays/pipelines etc

	// only set the next branch delta if the exception won't be handled for
	// the current branch...
	if( !eeEventTestIsActive )
		cpuSetNextBranchDelta( 4 );
	else if(psxCycleEE > 0)
	{
		psxBreak += psxCycleEE;		// record the number of cycles the IOP didn't run.
		psxCycleEE = 0;
	}
}

__forceinline void cpuTestTIMRInts() {
	if ((cpuRegs.CP0.n.Status.val & 0x10007) == 0x10001) {
		_cpuTestPERF();
		_cpuTestTIMR();
	}
}

void cpuTestHwInts() {
	cpuTestINTCInts();
	cpuTestDMACInts();
	cpuTestTIMRInts();
}

// This function performs a "hackish" execution of the BIOS stub, which initializes EE
// memory and hardware.  It forcefully breaks execution when the stub is finished, prior
// to the PS2 logos being displayed.  This allows us to "shortcut" right into a game
// without having to wait through the logos or endure game/bios localization checks.
void cpuExecuteBios()
{
	// Set the video mode to user's default request:
	// (right now we always default to NTSC)
	gsSetVideoRegionType( Config.PsxType & 1 );

	Console::Notice( "* PCSX2 *: ExecuteBios" );

	bExecBIOS = TRUE;
	while (cpuRegs.pc != 0x00200008 &&
		   cpuRegs.pc != 0x00100008) {
		g_nextBranchCycle = cpuRegs.cycle;
		Cpu->ExecuteBlock();
	}

	bExecBIOS = FALSE;
//    {
//        FILE* f = fopen("eebios.bin", "wb");
//        fwrite(PSM(0x80000000), 0x100000, 1, f);
//        fclose(f);
//        exit(0);

//        f = fopen("iopbios.bin", "wb");
//        fwrite(PS2MEM_PSX, 0x80000, 1, f);
//        fclose(f);
//    }

//	REC_CLEARM(0x00200008);
//	REC_CLEARM(0x00100008);
//	REC_CLEARM(cpuRegs.pc);

	// Reset the EErecs here, because the bios generates "slow" blocks that have hacky
	// bBiosEnd checks in them and stuff.  This deletes them so that the recs replace them
	// with new faster versions:
	Cpu->Reset();

	Console::Notice("* PCSX2 *: ExecuteBios Complete");
	//GSprintf(5, "PCSX2 " PCSX2_VERSION "\nExecuteBios Complete\n");
}

__forceinline void CPU_INT( u32 n, s32 ecycle)
{
	cpuRegs.interrupt|= 1 << n;
	cpuRegs.sCycle[n] = cpuRegs.cycle;
	cpuRegs.eCycle[n] = ecycle;

	// Interrupt is happening soon: make sure both EE and IOP are aware.

	if( ecycle <= 28 && psxCycleEE > 0 )
	{
		// If running in the IOP, force it to break immediately into the EE.
		// the EE's branch test is due to run.

		psxBreak += psxCycleEE;		// record the number of cycles the IOP didn't run.
		psxCycleEE = 0;
	}

	cpuSetNextBranchDelta( cpuRegs.eCycle[n] );
}
