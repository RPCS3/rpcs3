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

   
#include "PrecompiledHeader.h"
#include "Common.h"

#include "R5900.h"
#include "R3000A.h"
#include "VUmicro.h"
#include "COP0.h"
#include "MTVU.h"

#include "System/SysThreads.h"
#include "R5900Exceptions.h"

#include "Hardware.h"
#include "IPU/IPUdma.h"

#include "Elfheader.h"
#include "CDVD/CDVD.h"
#include "Patch.h"
#include "GameDatabase.h"

using namespace R5900;	// for R5900 disasm tools

s32 EEsCycle;		// used to sync the IOP to the EE
u32 EEoCycle;

__aligned16 cpuRegisters cpuRegs;
__aligned16 fpuRegisters fpuRegs;
__aligned16 tlbs tlb[48];
R5900cpu *Cpu = NULL;

bool g_SkipBiosHack; // set at boot if the skip bios hack is on, reset before the game has started
bool g_GameStarted; // set when we reach the game's entry point or earlier if the entry point cannot be determined

static const uint eeWaitCycles = 3072;

bool eeEventTestIsActive = false;

extern SysMainMemory& GetVmMemory();

void cpuReset()
{
	vu1Thread.WaitVU();
	if (GetMTGS().IsOpen())
		GetMTGS().WaitGS();		// GS better be done processing before we reset the EE, just in case.

	GetVmMemory().ResetAll();

	memzero(cpuRegs);
	memzero(fpuRegs);
	memzero(tlb);

	cpuRegs.pc				= 0xbfc00000; //set pc reg to stack
	cpuRegs.CP0.n.Config	= 0x440;
	cpuRegs.CP0.n.Status.val= 0x70400004; //0x10900000 <-- wrong; // COP0 enabled | BEV = 1 | TS = 1
	cpuRegs.CP0.n.PRid		= 0x00002e20; // PRevID = Revision ID, same as R5900
	fpuRegs.fprc[0]			= 0x00002e00; // fpu Revision..
	fpuRegs.fprc[31]		= 0x01000001; // fpu Status/Control

	g_nextEventCycle = cpuRegs.cycle + 4;
	EEsCycle = 0;
	EEoCycle = cpuRegs.cycle;

	hwReset();
	rcntInit();
	psxReset();
	
	extern void Deci2Reset();		// lazy, no good header for it yet.
	Deci2Reset();

	g_GameStarted = false;
	g_SkipBiosHack = EmuConfig.UseBOOT2Injection;

	ElfCRC = 0;
	DiscSerial = L"";
	ElfEntry = -1;

	// Probably not the right place, but it has to be done when the ram is actually initialized
	if(USBsetRAM != 0)
		USBsetRAM(iopMem->Main);

	// FIXME: LastELF should be reset on media changes as well as on CPU resets, in
	// the very unlikely case that a user swaps to another media source that "looks"
	// the same (identical ELF names) but is actually different (devs actually could
	// run into this while testing minor binary hacked changes to ISO images, which
	// is why I found out about this) --air
	LastELF = L"";
}

void cpuShutdown()
{
	hwShutdown();
}

__ri void cpuException(u32 code, u32 bd)
{
	bool errLevel2, checkStatus;
	u32 offset;

    cpuRegs.branch = 0;		// Tells the interpreter that an exception occurred during a branch.
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

		Console.Error("*PCSX2* FIX ME: Level 2 cpuException");
		if ((code & 0x38000) <= 0x8000 )
		{
			//Reset / NMI
			cpuRegs.pc = 0xBFC00000;
			Console.Warning("Reset request");
			cpuUpdateOperationMode();
			return;
		}
		else if((code & 0x38000) == 0x10000)
			offset = 0x80; //Performance Counter
		else if((code & 0x38000) == 0x18000)
			offset = 0x100; //Debug
		else
			Console.Error("Unknown Level 2 Exception!! Cause %x", code);
	}

	if (cpuRegs.CP0.n.Status.b.EXL == 0)
	{
		cpuRegs.CP0.n.Status.b.EXL = 1;
		if (bd)
		{
			Console.Warning("branch delay!!");
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
		if (errLevel2) Console.Warning("cpuException: Status.EXL = 1 cause %x", code);
	}

	if (checkStatus)
		cpuRegs.pc = 0x80000000 + offset;
	else
		cpuRegs.pc = 0xBFC00200 + offset;

	cpuUpdateOperationMode();
}

void cpuTlbMiss(u32 addr, u32 bd, u32 excode)
{
	Console.Error("cpuTlbMiss pc:%x, cycl:%x, addr: %x, status=%x, code=%x",
		cpuRegs.pc, cpuRegs.cycle, addr, cpuRegs.CP0.n.Status.val, excode);

	if (bd) Console.Warning("branch delay!!");

	pxFail( "TLB Miss handler is uninished code." ); // temporary

	cpuRegs.CP0.n.BadVAddr = addr;
	cpuRegs.CP0.n.Context &= 0xFF80000F;
	cpuRegs.CP0.n.Context |= (addr >> 9) & 0x007FFFF0;
	cpuRegs.CP0.n.EntryHi = (addr & 0xFFFFE000) | (cpuRegs.CP0.n.EntryHi & 0x1FFF);

	cpuRegs.CP0.n.Cause = excode;
	if (!(cpuRegs.CP0.n.Status.val & 0x2)) { // EXL bit
		cpuRegs.CP0.n.EPC = cpuRegs.pc - 4;
	}

	if (!cpuRegs.CP0.n.Status.b.IE) {
		cpuRegs.pc = 0x80000000;
	} else {
		cpuRegs.pc = 0x80000180;
	}

	cpuRegs.CP0.n.Status.b.EXL = 1;
	cpuUpdateOperationMode();
//	Log=1; varLog|= 0x40000000;
}

void cpuTlbMissR(u32 addr, u32 bd) {
	cpuTlbMiss(addr, bd, EXC_CODE_TLBL);
}

void cpuTlbMissW(u32 addr, u32 bd) {
	cpuTlbMiss(addr, bd, EXC_CODE_TLBS);
}

// sets a branch test to occur some time from an arbitrary starting point.
__fi void cpuSetNextEvent( u32 startCycle, s32 delta )
{
	// typecast the conditional to signed so that things don't blow up
	// if startCycle is greater than our next branch cycle.

	if( (int)(g_nextEventCycle - startCycle) > delta )
	{
		g_nextEventCycle = startCycle + delta;
	}
}

// sets a branch to occur some time from the current cycle
__fi void cpuSetNextEventDelta( s32 delta )
{
	cpuSetNextEvent( cpuRegs.cycle, delta );
}

// tests the cpu cycle against the given start and delta values.
// Returns true if the delta time has passed.
__fi int cpuTestCycle( u32 startCycle, s32 delta )
{
	// typecast the conditional to signed so that things don't explode
	// if the startCycle is ahead of our current cpu cycle.

	return (int)(cpuRegs.cycle - startCycle) >= delta;
}

// tells the EE to run the branch test the next time it gets a chance.
__fi void cpuSetEvent()
{
	g_nextEventCycle = cpuRegs.cycle;
}

__fi void cpuClearInt( uint i )
{
	jASSUME( i < 32 );
	cpuRegs.interrupt &= ~(1 << i);
}

static __fi void TESTINT( u8 n, void (*callback)() )
{
	if( !(cpuRegs.interrupt & (1 << n)) ) return;

	if( cpuTestCycle( cpuRegs.sCycle[n], cpuRegs.eCycle[n] ) )
	{
		cpuClearInt( n );
		callback();
	}
	else
		cpuSetNextEvent( cpuRegs.sCycle[n], cpuRegs.eCycle[n] );
}

// [TODO] move this function to LegacyDmac.cpp, and remove most of the DMAC-related headers from
// being included into R5900.cpp.
static __fi void _cpuTestInterrupts()
{
	if (!dmacRegs.ctrl.DMAE || (psHu8(DMAC_ENABLER+2) & 1))
	{
		//Console.Write("DMAC Disabled or suspended");
		return;
	}
	/* These are 'pcsx2 interrupts', they handle asynchronous stuff
	   that depends on the cycle timings */

	TESTINT(DMAC_VIF1,		vif1Interrupt);	
	TESTINT(DMAC_GIF,		gifInterrupt);
	TESTINT(DMAC_SIF0,		EEsif0Interrupt);
	TESTINT(DMAC_SIF1,		EEsif1Interrupt);
	
	// Profile-guided Optimization (sorta)
	// The following ints are rarely called.  Encasing them in a conditional
	// as follows helps speed up most games.

	if( cpuRegs.interrupt & 0xF19 ) // Bits 0 3 4 8 9 10 11 ( 111100011001 )
	{
		TESTINT(DMAC_VIF0,		vif0Interrupt);

		TESTINT(DMAC_FROM_IPU,	ipu0Interrupt);
		TESTINT(DMAC_TO_IPU,	ipu1Interrupt);

		TESTINT(DMAC_FROM_SPR,	SPRFROMinterrupt);
		TESTINT(DMAC_TO_SPR,	SPRTOinterrupt);

		TESTINT(DMAC_MFIFO_VIF, vifMFIFOInterrupt);
		TESTINT(DMAC_MFIFO_GIF, gifMFIFOInterrupt);
	}
}

static __fi void _cpuTestTIMR()
{
	cpuRegs.CP0.n.Count += cpuRegs.cycle-s_iLastCOP0Cycle;
	s_iLastCOP0Cycle = cpuRegs.cycle;

	// fixme: this looks like a hack to make up for the fact that the TIMR
	// doesn't yet have a proper mechanism for setting itself up on a nextEventCycle.
	// A proper fix would schedule the TIMR to trigger at a specific cycle anytime
	// the Count or Compare registers are modified.

	if ( (cpuRegs.CP0.n.Status.val & 0x8000) &&
		cpuRegs.CP0.n.Count >= cpuRegs.CP0.n.Compare && cpuRegs.CP0.n.Count < cpuRegs.CP0.n.Compare+1000 )
	{
		Console.WriteLn( Color_Magenta, "timr intr: %x, %x", cpuRegs.CP0.n.Count, cpuRegs.CP0.n.Compare);
		cpuException(0x808000, cpuRegs.branch);
	}
}

static __fi void _cpuTestPERF()
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

static bool cpuIntsEnabled(int Interrupt)
{
	bool IntType = !!(cpuRegs.CP0.n.Status.val & Interrupt); //Choose either INTC or DMAC, depending on what called it

	return IntType && cpuRegs.CP0.n.Status.b.EIE && cpuRegs.CP0.n.Status.b.IE &&
		!cpuRegs.CP0.n.Status.b.EXL && (cpuRegs.CP0.n.Status.b.ERL == 0);
}

// if cpuRegs.cycle is greater than this cycle, should check cpuEventTest for updates
u32 g_nextEventCycle = 0;

// Shared portion of the branch test, called from both the Interpreter
// and the recompiler.  (moved here to help alleviate redundant code)
__fi void _cpuEventTest_Shared()
{
	ScopedBool etest(eeEventTestIsActive);
	g_nextEventCycle = cpuRegs.cycle + eeWaitCycles;

	// ---- INTC / DMAC (CPU-level Exceptions) -----------------
	// Done first because exceptions raised during event tests need to be postponed a few
	// cycles (fixes Grandia II [PAL], which does a spin loop on a vsync and expects to
	// be able to read the value before the exception handler clears it).

	uint mask = intcInterrupt() | dmacInterrupt();
	if (cpuIntsEnabled(mask)) cpuException(mask, cpuRegs.branch);


	// ---- Counters -------------
	// Important: the vsync counter must be the first to be checked.  It includes emulation
	// escape/suspend hooks, and it's really a good idea to suspend/resume emulation before
	// doing any actual meaningful branchtest logic.

	if( cpuTestCycle( nextsCounter, nextCounter ) )
	{
		rcntUpdate();
		_cpuTestPERF();
	}

	rcntUpdate_hScanline();

	_cpuTestTIMR();

	// ---- Interrupts -------------
	// These are basically just DMAC-related events, which also piggy-back the same bits as
	// the PS2's own DMA channel IRQs and IRQ Masks.

	_cpuTestInterrupts();

	// ---- IOP -------------
	// * It's important to run a iopEventTest before calling ExecuteBlock. This
	//   is because the IOP does not always perform branch tests before returning
	//   (during the prev branch) and also so it can act on the state the EE has
	//   given it before executing any code.
	//
	// * The IOP cannot always be run.  If we run IOP code every time through the
	//   cpuEventTest, the IOP generally starts to run way ahead of the EE.

	EEsCycle += cpuRegs.cycle - EEoCycle;
	EEoCycle = cpuRegs.cycle;

	if( EEsCycle > 0 )
		iopEventAction = true;

	iopEventTest();

	if( iopEventAction )
	{
		//if( EEsCycle < -450 )
		//	Console.WriteLn( " IOP ahead by: %d cycles", -EEsCycle );

		EEsCycle = psxCpu->ExecuteBlock( EEsCycle );

		iopEventAction = false;
	}

	// ---- VU0 -------------
	// We're in a EventTest.  All dynarec registers are flushed
	// so there is no need to freeze registers here.
	CpuVU0->ExecuteBlock();

	// Note:  We don't update the VU1 here because it runs it's micro-programs in
	// one shot always.  That is, when a program is executed the VU1 doesn't even
	// bother to return until the program is completely finished.

	// ---- Schedule Next Event Test --------------

	if( EEsCycle > 192 )
	{
		// EE's running way ahead of the IOP still, so we should branch quickly to give the
		// IOP extra timeslices in short order.

		cpuSetNextEventDelta( 48 );
		//Console.Warning( "EE ahead of the IOP -- Rapid Event!  %d", EEsCycle );
	}

	// The IOP could be running ahead/behind of us, so adjust the iop's next branch by its
	// relative position to the EE (via EEsCycle)
	cpuSetNextEventDelta( ((g_iopNextEventCycle-psxRegs.cycle)*8) - EEsCycle );

	// Apply the hsync counter's nextCycle
	cpuSetNextEvent( hsyncCounter.sCycle, hsyncCounter.CycleT );

	// Apply vsync and other counter nextCycles
	cpuSetNextEvent( nextsCounter, nextCounter );
}

__ri void cpuTestINTCInts()
{
	// Check the COP0's Status register for general interrupt disables, and the 0x400
	// bit (which is INTC master toggle).
	if( !cpuIntsEnabled(0x400) ) return;

	if( (psHu32(INTC_STAT) & psHu32(INTC_MASK)) == 0 ) return;

	cpuSetNextEventDelta( 4 );
	if(eeEventTestIsActive && (iopCycleEE > 0))
	{
		iopBreak += iopCycleEE;		// record the number of cycles the IOP didn't run.
		iopCycleEE = 0;
	}
}

__fi void cpuTestDMACInts()
{
	// Check the COP0's Status register for general interrupt disables, and the 0x800
	// bit (which is the DMAC master toggle).
	if( !cpuIntsEnabled(0x800) ) return;

	if ( ( (psHu16(0xe012) & psHu16(0xe010)) == 0) &&
		 ( (psHu16(0xe010) & 0x8000) == 0) ) return;

	cpuSetNextEventDelta( 4 );
	if(eeEventTestIsActive && (iopCycleEE > 0))
	{
		iopBreak += iopCycleEE;		// record the number of cycles the IOP didn't run.
		iopCycleEE = 0;
	}
}

__fi void cpuTestTIMRInts() {
	if ((cpuRegs.CP0.n.Status.val & 0x10007) == 0x10001) {
		_cpuTestPERF();
		_cpuTestTIMR();
	}
}

__fi void cpuTestHwInts() {
	cpuTestINTCInts();
	cpuTestDMACInts();
	cpuTestTIMRInts();
}

__fi void CPU_INT( EE_EventType n, s32 ecycle)
{
	if( n != 2 && cpuRegs.interrupt & (1<<n) ){ // 2 is Gif, and every path 3 masking game triggers this :/
		DevCon.Warning( "***** EE > Twice-thrown int on IRQ %d", n );
	}

	// EE events happen 8 cycles in the future instead of whatever was requested.
	// This can be used on games with PATH3 masking issues for example, or when
	// some FMV look bad.
	if(CHECK_EETIMINGHACK) ecycle = 8;

	cpuRegs.interrupt|= 1 << n;
	cpuRegs.sCycle[n] = cpuRegs.cycle;
	cpuRegs.eCycle[n] = ecycle;

	// Interrupt is happening soon: make sure both EE and IOP are aware.

	if( ecycle <= 28 && iopCycleEE > 0 )
	{
		// If running in the IOP, force it to break immediately into the EE.
		// the EE's branch test is due to run.

		iopBreak += iopCycleEE;		// record the number of cycles the IOP didn't run.
		iopCycleEE = 0;
	}

	cpuSetNextEventDelta( cpuRegs.eCycle[n] );
}

// Called from recompilers; __fastcall define is mandatory.
void __fastcall eeGameStarting()
{
	if (!g_GameStarted)
	{
		//Console.WriteLn( Color_Green, "(R5900) ELF Entry point! [addr=0x%08X]", ElfEntry );
		g_GameStarted = true;
		GetCoreThread().GameStartingInThread();

		// GameStartingInThread may issue a reset of the cpu and/or recompilers.  Check for and
		// handle such things here:
		Cpu->CheckExecutionState();
	}
	else
	{
		Console.WriteLn( Color_Green, "(R5900) Re-executed ELF Entry point (ignored) [addr=0x%08X]", ElfEntry );
	}
}

// Called from recompilers; __fastcall define is mandatory.
void __fastcall eeloadReplaceOSDSYS()
{
	g_SkipBiosHack = false;

	const wxString &elf_override = GetCoreThread().GetElfOverride();

	if (!elf_override.IsEmpty())
		cdvdReloadElfInfo(L"host:" + elf_override);
	else
		cdvdReloadElfInfo();

	// didn't recognize an ELF
	if (ElfEntry == -1) {
		eeGameStarting();
		return;
	}

	static u32 osdsys = 0, osdsys_p = 0;
	// Memory this high is safe before the game's running presumably
	// Other options are kernel memory (first megabyte) or the scratchpad
	// PS2LOGO is loaded at 16MB, let's use 17MB
	const u32 safemem = 0x1100000;

	// The strings are all 64-bit aligned.  Why? I don't know, but they are
	for (u32 i = EELOAD_START; i < EELOAD_START + EELOAD_SIZE; i += 8) {
		if (!strcmp((char*)PSM(i), "rom0:OSDSYS")) {
			osdsys = i;
			break;
		}
	}
	pxAssert(osdsys);

	for (u32 i = osdsys - 4; i >= EELOAD_START; i -= 4) {
		if (memRead32(i) == osdsys) {
			osdsys_p = i;
			break;
		}
	}
	pxAssert(osdsys_p);

	std::string elfname;

	if (!elf_override.IsEmpty())
	{
		elfname += "host:";
		elfname += elf_override.ToUTF8();
	}
	else
	{
		wxString boot2;
		if (GetPS2ElfName(boot2) == 2)
			elfname = boot2.ToUTF8();
	}

	if (!elfname.empty())
	{
		strcpy((char*)PSM(safemem), elfname.c_str());
		memWrite32(osdsys_p, safemem);
	}
	// else... uh...?
}
