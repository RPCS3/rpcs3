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

u32 s_iLastCOP0Cycle = 0;
u32 s_iLastPERFCycle[2] = { 0, 0 };

// Updates the CPU's mode of operation (either, Kernel, Supervisor, or User modes).
// Currently the different modes are not implemented.
// Given this function is called so much, it's commented out for now. (rama)
__ri void cpuUpdateOperationMode() {

	//u32 value = cpuRegs.CP0.n.Status.val;

	//if (value & 0x06 ||
	//	(value & 0x18) == 0) { // Kernel Mode (KSU = 0 | EXL = 1 | ERL = 1)*/
	//	memSetKernelMode();	// Kernel memory always
	//} else { // User Mode
	//	memSetUserMode();
	//}
}

void __fastcall WriteCP0Status(u32 value) {

	//DMA_LOG("COP0 Status write = 0x%08x", value);

	cpuRegs.CP0.n.Status.val = value;
    cpuUpdateOperationMode();
    cpuSetNextEventDelta(4);
}

void MapTLB(int i)
{
	u32 mask, addr;
	u32 saddr, eaddr;

	DevCon.WriteLn("MAP TLB %d: 0x%08X-> [0x%08X 0x%08X] S=0x%08X G=%d ASID=%d Mask=0x%03X",
		i, tlb[i].VPN2, tlb[i].PFN0, tlb[i].PFN1, tlb[i].S, tlb[i].G, tlb[i].ASID, tlb[i].Mask);

	if (tlb[i].S)
	{
		vtlb_VMapBuffer(tlb[i].VPN2, eeMem->Scratch, Ps2MemSize::Scratch);
	}

	if (tlb[i].VPN2 == 0x70000000) return; //uh uhh right ...
	if (tlb[i].EntryLo0 & 0x2) {
		mask  = ((~tlb[i].Mask) << 1) & 0xfffff;
		saddr = tlb[i].VPN2 >> 12;
		eaddr = saddr + tlb[i].Mask + 1;

		for (addr=saddr; addr<eaddr; addr++) {
			if ((addr & mask) == ((tlb[i].VPN2 >> 12) & mask)) { //match
				memSetPageAddr(addr << 12, tlb[i].PFN0 + ((addr - saddr) << 12));
				Cpu->Clear(addr << 12, 0x400);
			}
		}
	}

	if (tlb[i].EntryLo1 & 0x2) {
		mask  = ((~tlb[i].Mask) << 1) & 0xfffff;
		saddr = (tlb[i].VPN2 >> 12) + tlb[i].Mask + 1;
		eaddr = saddr + tlb[i].Mask + 1;

		for (addr=saddr; addr<eaddr; addr++) {
			if ((addr & mask) == ((tlb[i].VPN2 >> 12) & mask)) { //match
				memSetPageAddr(addr << 12, tlb[i].PFN1 + ((addr - saddr) << 12));
				Cpu->Clear(addr << 12, 0x400);
			}
		}
	}
}

void UnmapTLB(int i)
{
	//Console.WriteLn("Clear TLB %d: %08x-> [%08x %08x] S=%d G=%d ASID=%d Mask= %03X", i,tlb[i].VPN2,tlb[i].PFN0,tlb[i].PFN1,tlb[i].S,tlb[i].G,tlb[i].ASID,tlb[i].Mask);
	u32 mask, addr;
	u32 saddr, eaddr;

	if (tlb[i].S)
	{
		vtlb_VMapUnmap(tlb[i].VPN2,0x4000);
		return;
	}

	if (tlb[i].EntryLo0 & 0x2)
	{
		mask  = ((~tlb[i].Mask) << 1) & 0xfffff;
		saddr = tlb[i].VPN2 >> 12;
		eaddr = saddr + tlb[i].Mask + 1;
	//	Console.WriteLn("Clear TLB: %08x ~ %08x",saddr,eaddr-1);
		for (addr=saddr; addr<eaddr; addr++) {
			if ((addr & mask) == ((tlb[i].VPN2 >> 12) & mask)) { //match
				memClearPageAddr(addr << 12);
				Cpu->Clear(addr << 12, 0x400);
			}
		}
	}

	if (tlb[i].EntryLo1 & 0x2) {
		mask  = ((~tlb[i].Mask) << 1) & 0xfffff;
		saddr = (tlb[i].VPN2 >> 12) + tlb[i].Mask + 1;
		eaddr = saddr + tlb[i].Mask + 1;
	//	Console.WriteLn("Clear TLB: %08x ~ %08x",saddr,eaddr-1);
		for (addr=saddr; addr<eaddr; addr++) {
			if ((addr & mask) == ((tlb[i].VPN2 >> 12) & mask)) { //match
				memClearPageAddr(addr << 12);
				Cpu->Clear(addr << 12, 0x400);
			}
		}
	}
}

void WriteTLB(int i)
{
	tlb[i].PageMask = cpuRegs.CP0.n.PageMask;
	tlb[i].EntryHi = cpuRegs.CP0.n.EntryHi;
	tlb[i].EntryLo0 = cpuRegs.CP0.n.EntryLo0;
	tlb[i].EntryLo1 = cpuRegs.CP0.n.EntryLo1;

	tlb[i].Mask = (cpuRegs.CP0.n.PageMask >> 13) & 0xfff;
	tlb[i].nMask = (~tlb[i].Mask) & 0xfff;
	tlb[i].VPN2 = ((cpuRegs.CP0.n.EntryHi >> 13) & (~tlb[i].Mask)) << 13;
	tlb[i].ASID = cpuRegs.CP0.n.EntryHi & 0xfff;
	tlb[i].G = cpuRegs.CP0.n.EntryLo0 & cpuRegs.CP0.n.EntryLo1 & 0x1;
	tlb[i].PFN0 = (((cpuRegs.CP0.n.EntryLo0 >> 6) & 0xFFFFF) & (~tlb[i].Mask)) << 12;
	tlb[i].PFN1 = (((cpuRegs.CP0.n.EntryLo1 >> 6) & 0xFFFFF) & (~tlb[i].Mask)) << 12;
	tlb[i].S = cpuRegs.CP0.n.EntryLo0&0x80000000;

	MapTLB(i);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Performance Counters Update Stuff!
//
// Note regarding updates of PERF and TIMR registers: never allow increment to be 0.
// That happens when a game loads the MFC0 twice in the same recompiled block (before the
// cpuRegs.cycles update), and can cause games to lock up since it's an unexpected result.
//
// PERF Overflow exceptions:  The exception is raised when the MSB of the Performance
// Counter Register is set.  I'm assuming the exception continues to re-raise until the
// app clears the bit manually (needs testing).
//
// PERF Events:
//  * Event 0 on PCR 0 is unused (counter disable)
//  * Event 16 is usable as a specific counter disable bit (since CTE affects both counters)
//  * Events 17-31 are reserved (act as counter disable)
//
// Most event mode aren't supported, and issue a warning and do a standard instruction
// count.  But only mode 1 (instruction counter) has been found to be used by games thus far.
//

static __fi bool PERF_ShouldCountEvent( uint evt )
{
	switch( evt )
	{
		// This is a rough table of actions for various PCR modes.  Some of these
		// can be implemented more accurately later.  Others (WBBs in particular)
		// probably cannot without some severe complications.

		// left sides are PCR0 / right sides are PCR1

		case 1:		// cpu cycle counter.
		case 2:		// single/dual instruction issued
		case 3:		// Branch issued / Branch mispredicated
			return true;

		case 4:		// BTAC/TLB miss
		case 5:		// ITLB/DTLB miss
		case 6:		// Data/Instruction cache miss
			return false;

		case 7:		// Access to DTLB / WBB single request fail
		case 8:		// Non-blocking load / WBB burst request fail
		case 9:
		case 10:
			return false;

		case 11:	// CPU address bus busy / CPU data bus busy
			return false;

		case 12:	// Instruction completed
		case 13:	// non-delayslot instruction completed
		case 14:	// COP2/COP1 instruction complete
		case 15:	// Load/Store completed
			return true;
	}

	return false;
}

// Diagnostics for event modes that we just ignore for now.  Using these perf units could
// cause compat issues in some very odd/rare games, so if this msg comes up who knows,
// might save some debugging effort. :)
void COP0_DiagnosticPCCR()
{
	if( cpuRegs.PERF.n.pccr.b.Event0 >= 7 && cpuRegs.PERF.n.pccr.b.Event0 <= 10 )
		Console.Warning( "PERF/PCR0 Unsupported Update Event Mode = 0x%x", cpuRegs.PERF.n.pccr.b.Event0 );

	if( cpuRegs.PERF.n.pccr.b.Event1 >= 7 && cpuRegs.PERF.n.pccr.b.Event1 <= 10 )
		Console.Warning( "PERF/PCR1 Unsupported Update Event Mode = 0x%x", cpuRegs.PERF.n.pccr.b.Event1 );
}
extern int branch;
__fi void COP0_UpdatePCCR()
{
	//if( cpuRegs.CP0.n.Status.b.ERL || !cpuRegs.PERF.n.pccr.b.CTE ) return;

	// TODO : Implement memory mode checks here (kernel/super/user)
	// For now we just assume kernel mode.

	if( cpuRegs.PERF.n.pccr.val & 0xf )
	{
		// ----------------------------------
		//    Update Performance Counter 0
		// ----------------------------------

		if( PERF_ShouldCountEvent( cpuRegs.PERF.n.pccr.b.Event0 ) )
		{
			u32 incr = cpuRegs.cycle - s_iLastPERFCycle[0];
			if( incr == 0 ) incr++;

			// use prev/XOR method for one-time exceptions (but likely less correct)
			//u32 prev = cpuRegs.PERF.n.pcr0;
			cpuRegs.PERF.n.pcr0 += incr;
			s_iLastPERFCycle[0] = cpuRegs.cycle;

			//prev ^= (1UL<<31);		// XOR is fun!
			//if( (prev & cpuRegs.PERF.n.pcr0) & (1UL<<31) )
			if( (cpuRegs.PERF.n.pcr0 & 0x80000000) && (cpuRegs.CP0.n.Status.b.ERL == 1) && cpuRegs.PERF.n.pccr.b.CTE)
			{
				// TODO: Vector to the appropriate exception here.
				// This code *should* be correct, but is untested (and other parts of the emu are
				// not prepared to handle proper Level 2 exception vectors yet)

				//branch == 1 is probably not the best way to check for the delay slot, but it beats nothing! (Refraction)
			/*	if( branch == 1 )
				{
					cpuRegs.CP0.n.ErrorEPC = cpuRegs.pc - 4;
					cpuRegs.CP0.n.Cause |= 0x40000000;
				}
				else
				{
					cpuRegs.CP0.n.ErrorEPC = cpuRegs.pc;
					cpuRegs.CP0.n.Cause &= ~0x40000000;
				}

				if( cpuRegs.CP0.n.Status.b.DEV )
				{
					// Bootstrap vector
					cpuRegs.pc = 0xbfc00280;
				}
				else
				{
					cpuRegs.pc = 0x80000080;
				}
				cpuRegs.CP0.n.Status.b.ERL = 1;
				cpuRegs.CP0.n.Cause |= 0x20000;*/
			}
		}
	}

	if( cpuRegs.PERF.n.pccr.b.U1 )
	{
		// ----------------------------------
		//    Update Performance Counter 1
		// ----------------------------------

		if( PERF_ShouldCountEvent( cpuRegs.PERF.n.pccr.b.Event1 ) )
		{
			u32 incr = cpuRegs.cycle - s_iLastPERFCycle[1];
			if( incr == 0 ) incr++;

			cpuRegs.PERF.n.pcr1 += incr;
			s_iLastPERFCycle[1] = cpuRegs.cycle;

			if( (cpuRegs.PERF.n.pcr1 & 0x80000000) && (cpuRegs.CP0.n.Status.b.ERL == 1) && cpuRegs.PERF.n.pccr.b.CTE)
			{
				// TODO: Vector to the appropriate exception here.
				// This code *should* be correct, but is untested (and other parts of the emu are
				// not prepared to handle proper Level 2 exception vectors yet)

				//branch == 1 is probably not the best way to check for the delay slot, but it beats nothing! (Refraction)

				/*if( branch == 1 )
				{
					cpuRegs.CP0.n.ErrorEPC = cpuRegs.pc - 4;
					cpuRegs.CP0.n.Cause |= 0x40000000;
				}
				else
				{
					cpuRegs.CP0.n.ErrorEPC = cpuRegs.pc;
					cpuRegs.CP0.n.Cause &= ~0x40000000;
				}

				if( cpuRegs.CP0.n.Status.b.DEV )
				{
					// Bootstrap vector
					cpuRegs.pc = 0xbfc00280;
				}
				else
				{
					cpuRegs.pc = 0x80000080;
				}
				cpuRegs.CP0.n.Status.b.ERL = 1;
				cpuRegs.CP0.n.Cause |= 0x20000;*/
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//

namespace R5900 {
namespace Interpreter {
namespace OpcodeImpl {
namespace COP0 {

void MFC0()
{
	// Note on _Rd_ Condition 9: CP0.Count should be updated even if _Rt_ is 0.
	if ((_Rd_ != 9) && !_Rt_ ) return;
	if (_Rd_ != 9) { COP0_LOG("%s", disR5900Current.getCString() ); }

	//if(bExecBIOS == FALSE && _Rd_ == 25) Console.WriteLn("MFC0 _Rd_ %x = %x", _Rd_, cpuRegs.CP0.r[_Rd_]);
	switch (_Rd_)
	{
		case 12:
			cpuRegs.GPR.r[_Rt_].SD[0] = (s32)(cpuRegs.CP0.r[_Rd_] & 0xf0c79c1f);
		break;

		case 25:
		    switch(_Imm_ & 0x3F)
		    {
			    case 0:		// MFPS  [LSB is clear]
					cpuRegs.GPR.r[_Rt_].SD[0] = (s32)cpuRegs.PERF.n.pccr.val;
				break;

			    case 1:		// MFPC [LSB is set] - read PCR0
					COP0_UpdatePCCR();
                    cpuRegs.GPR.r[_Rt_].SD[0] = (s32)cpuRegs.PERF.n.pcr0;
				break;

			    case 3:		// MFPC [LSB is set] - read PCR1
					COP0_UpdatePCCR();
					cpuRegs.GPR.r[_Rt_].SD[0] = (s32)cpuRegs.PERF.n.pcr1;
				break;
		    }
		    /*Console.WriteLn("MFC0 PCCR = %x PCR0 = %x PCR1 = %x IMM= %x",  params
		    cpuRegs.PERF.n.pccr, cpuRegs.PERF.n.pcr0, cpuRegs.PERF.n.pcr1, _Imm_ & 0x3F);*/
		break;

		case 24:
			Console.WriteLn("MFC0 Breakpoint debug Registers code = %x", cpuRegs.code & 0x3FF);
		break;

		case 9:
		{
			u32 incr = cpuRegs.cycle-s_iLastCOP0Cycle;
			if( incr == 0 ) incr++;
			cpuRegs.CP0.n.Count += incr;
			s_iLastCOP0Cycle = cpuRegs.cycle;
			if( !_Rt_ ) break;
		}

		default:
			cpuRegs.GPR.r[_Rt_].UD[0] = (s64)cpuRegs.CP0.r[_Rd_];
	}
}

void MTC0()
{
	COP0_LOG("%s\n", disR5900Current.getCString());
	//if(bExecBIOS == FALSE && _Rd_ == 25) Console.WriteLn("MTC0 _Rd_ %x = %x", _Rd_, cpuRegs.CP0.r[_Rd_]);
	switch (_Rd_)
	{
		case 9:
			s_iLastCOP0Cycle = cpuRegs.cycle;
			cpuRegs.CP0.r[9] = cpuRegs.GPR.r[_Rt_].UL[0];
		break;

		case 12:
			WriteCP0Status(cpuRegs.GPR.r[_Rt_].UL[0]);
		break;

		case 24:
			Console.WriteLn("MTC0 Breakpoint debug Registers code = %x", cpuRegs.code & 0x3FF);
		break;

		case 25:
			/*if(bExecBIOS == FALSE && _Rd_ == 25) Console.WriteLn("MTC0 PCCR = %x PCR0 = %x PCR1 = %x IMM= %x", params
				cpuRegs.PERF.n.pccr, cpuRegs.PERF.n.pcr0, cpuRegs.PERF.n.pcr1, _Imm_ & 0x3F);*/
			switch(_Imm_ & 0x3F)
			{
				case 0:		// MTPS  [LSB is clear]
					// Updates PCRs and sets the PCCR.
					COP0_UpdatePCCR();
					cpuRegs.PERF.n.pccr.val = cpuRegs.GPR.r[_Rt_].UL[0];
					COP0_DiagnosticPCCR();
				break;

				case 1:		// MTPC [LSB is set] - set PCR0
					cpuRegs.PERF.n.pcr0 = cpuRegs.GPR.r[_Rt_].UL[0];
					s_iLastPERFCycle[0] = cpuRegs.cycle;
				break;

				case 3:		// MTPC [LSB is set] - set PCR0
					cpuRegs.PERF.n.pcr1 = cpuRegs.GPR.r[_Rt_].UL[0];
					s_iLastPERFCycle[1] = cpuRegs.cycle;
				break;
			}
		break;

		default:
			cpuRegs.CP0.r[_Rd_] = cpuRegs.GPR.r[_Rt_].UL[0];
		break;
	}
}

int CPCOND0() {
	return ((dmacRegs.stat.CIS | ~dmacRegs.pcr.CPC) == 0x3ff);
}

//#define CPCOND0	1

void BC0F() {
	if (CPCOND0() == 0) intDoBranch(_BranchTarget_);
}

void BC0T() {
	if (CPCOND0() == 1) intDoBranch(_BranchTarget_);
}

void BC0FL() {
	if (CPCOND0() == 0)
		intDoBranch(_BranchTarget_);
	else
		cpuRegs.pc+= 4;

}

void BC0TL() {
	if (CPCOND0() == 1)
		intDoBranch(_BranchTarget_);
	else
		cpuRegs.pc+= 4;
}

void TLBR() {
/*	CPU_LOG("COP0_TLBR %d:%x,%x,%x,%x\n",
			cpuRegs.CP0.n.Random,   cpuRegs.CP0.n.PageMask, cpuRegs.CP0.n.EntryHi,
			cpuRegs.CP0.n.EntryLo0, cpuRegs.CP0.n.EntryLo1);*/

	int i = cpuRegs.CP0.n.Index&0x1f;

	cpuRegs.CP0.n.PageMask = tlb[i].PageMask;
	cpuRegs.CP0.n.EntryHi = tlb[i].EntryHi&~(tlb[i].PageMask|0x1f00);
	cpuRegs.CP0.n.EntryLo0 = (tlb[i].EntryLo0&~1)|((tlb[i].EntryHi>>12)&1);
	cpuRegs.CP0.n.EntryLo1 =(tlb[i].EntryLo1&~1)|((tlb[i].EntryHi>>12)&1);
}

void TLBWI() {
	int j = cpuRegs.CP0.n.Index & 0x3f;

	if (j > 48) return;

/*	CPU_LOG("COP0_TLBWI %d:%x,%x,%x,%x\n",
			cpuRegs.CP0.n.Index,    cpuRegs.CP0.n.PageMask, cpuRegs.CP0.n.EntryHi,
			cpuRegs.CP0.n.EntryLo0, cpuRegs.CP0.n.EntryLo1);*/

	UnmapTLB(j);
	WriteTLB(j);
}

void TLBWR() {
	int j = cpuRegs.CP0.n.Random & 0x3f;

	if (j > 48) return;

/*	CPU_LOG("COP0_TLBWR %d:%x,%x,%x,%x\n",
			cpuRegs.CP0.n.Random,   cpuRegs.CP0.n.PageMask, cpuRegs.CP0.n.EntryHi,
			cpuRegs.CP0.n.EntryLo0, cpuRegs.CP0.n.EntryLo1);*/

//	if( !bExecBIOS )
//		__Log("TLBWR %d\n", j);

	UnmapTLB(j);
	WriteTLB(j);
}

void TLBP() {
	int i;

	union {
		struct {
			u32 VPN2:19;
			u32 VPN2X:2;
			u32 G:3;
			u32 ASID:8;
		} s;
		u32 u;
	} EntryHi32;

	EntryHi32.u = cpuRegs.CP0.n.EntryHi;

	cpuRegs.CP0.n.Index=0xFFFFFFFF;
	for(i=0;i<48;i++){
		if (tlb[i].VPN2 == ((~tlb[i].Mask) & (EntryHi32.s.VPN2))
		&& ((tlb[i].G&1) || ((tlb[i].ASID & 0xff) == EntryHi32.s.ASID))) {
			cpuRegs.CP0.n.Index = i;
			break;
		}
	}
	 if(cpuRegs.CP0.n.Index == 0xFFFFFFFF) cpuRegs.CP0.n.Index = 0x80000000;
}

void ERET() {
	if (cpuRegs.CP0.n.Status.b.ERL) {
		cpuRegs.pc = cpuRegs.CP0.n.ErrorEPC;
		cpuRegs.CP0.n.Status.b.ERL = 0;
	} else {
		cpuRegs.pc = cpuRegs.CP0.n.EPC;
		cpuRegs.CP0.n.Status.b.EXL = 0;
	}
	cpuUpdateOperationMode();
	cpuSetNextEventDelta(4);
	intSetBranch();
}

void DI() {
	if (cpuRegs.CP0.n.Status.b._EDI || cpuRegs.CP0.n.Status.b.EXL ||
		cpuRegs.CP0.n.Status.b.ERL || (cpuRegs.CP0.n.Status.b.KSU == 0)) {
		cpuRegs.CP0.n.Status.b.EIE = 0;
		// IRQs are disabled so no need to do a cpu exception/event test...
		//cpuSetNextEventDelta();
	}
}

void EI() {
	if (cpuRegs.CP0.n.Status.b._EDI || cpuRegs.CP0.n.Status.b.EXL ||
		cpuRegs.CP0.n.Status.b.ERL || (cpuRegs.CP0.n.Status.b.KSU == 0)) {
		cpuRegs.CP0.n.Status.b.EIE = 1;
		// schedule an event test, which will check for and raise pending IRQs.
		cpuSetNextEventDelta(4);
	}
}

} } } }	// end namespace R5900::Interpreter::OpcodeImpl
