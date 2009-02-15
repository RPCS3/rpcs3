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
#include "R5900.h"
#include "R5900OpcodeTables.h"

u32 s_iLastCOP0Cycle = 0;
u32 s_iLastPERFCycle[2] = { 0, 0 };

void UpdateCP0Status() {
	u32 value = cpuRegs.CP0.n.Status.val;

	if (value & 0x06 ||
		(value & 0x18) == 0) { // Kernel Mode (KSU = 0 | EXL = 1 | ERL = 1)*/
		memSetKernelMode();	// Kernel memory always
	} else { // User Mode
		memSetUserMode();
	}
	cpuTestHwInts();
}

void WriteCP0Status(u32 value) {
	cpuRegs.CP0.n.Status.val = value;
    UpdateCP0Status();
}

void MapTLB(int i)
{
	u32 mask, addr;
	u32 saddr, eaddr;

	DevCon::WriteLn("MAP TLB %d: %08x-> [%08x %08x] S=%d G=%d ASID=%d Mask= %03X", params
		i,tlb[i].VPN2,tlb[i].PFN0,tlb[i].PFN1,tlb[i].S,tlb[i].G,tlb[i].ASID,tlb[i].Mask);

	if (tlb[i].S)
	{
		DevCon::WriteLn("OMG SPRAM MAPPING %08X %08X\n",params tlb[i].VPN2,tlb[i].Mask);
		vtlb_VMapBuffer(tlb[i].VPN2,psS,0x4000);
	}

	if (tlb[i].VPN2 == 0x70000000) return; //uh uhh right ...

	if (tlb[i].EntryLo0 & 0x2) {
		mask  = ((~tlb[i].Mask) << 1) & 0xfffff;
		saddr = tlb[i].VPN2 >> 12;
		eaddr = saddr + tlb[i].Mask + 1;

		for (addr=saddr; addr<eaddr; addr++) {
			if ((addr & mask) == ((tlb[i].VPN2 >> 12) & mask)) { //match
				memSetPageAddr(addr << 12, tlb[i].PFN0 + ((addr - saddr) << 12));
				Cpu->Clear(addr << 12, 1);
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
				Cpu->Clear(addr << 12, 1);
			}
		}
	}
}

void UnmapTLB(int i)
{
	//SysPrintf("Clear TLB %d: %08x-> [%08x %08x] S=%d G=%d ASID=%d Mask= %03X\n",i,tlb[i].VPN2,tlb[i].PFN0,tlb[i].PFN1,tlb[i].S,tlb[i].G,tlb[i].ASID,tlb[i].Mask);
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
	//	SysPrintf("Clear TLB: %08x ~ %08x\n",saddr,eaddr-1);
		for (addr=saddr; addr<eaddr; addr++) {
			if ((addr & mask) == ((tlb[i].VPN2 >> 12) & mask)) { //match
				memClearPageAddr(addr << 12);
				Cpu->Clear(addr << 12, 1);
			}
		}
	}

	if (tlb[i].EntryLo1 & 0x2) {
		mask  = ((~tlb[i].Mask) << 1) & 0xfffff;
		saddr = (tlb[i].VPN2 >> 12) + tlb[i].Mask + 1;
		eaddr = saddr + tlb[i].Mask + 1;
	//	SysPrintf("Clear TLB: %08x ~ %08x\n",saddr,eaddr-1);
		for (addr=saddr; addr<eaddr; addr++) {
			if ((addr & mask) == ((tlb[i].VPN2 >> 12) & mask)) { //match
				memClearPageAddr(addr << 12);
				Cpu->Clear(addr << 12, 1);
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

namespace R5900 {
namespace Interpreter {
namespace OpcodeImpl {
namespace COP0 {

void MFC0() {
	if (!_Rt_) return;
	if (_Rd_ != 9) { COP0_LOG("%s\n", disR5900Current.getCString() ); }
	
	//if(bExecBIOS == FALSE && _Rd_ == 25) SysPrintf("MFC0 _Rd_ %x = %x\n", _Rd_, cpuRegs.CP0.r[_Rd_]);
	switch (_Rd_) {
		
		case 12: cpuRegs.GPR.r[_Rt_].UD[0] = (s64)(cpuRegs.CP0.r[_Rd_] & 0xf0c79c1f); break;
		case 25: 
		    switch(_Imm_ & 0x3F){
			    case 0: cpuRegs.GPR.r[_Rt_].UD[0] = (s64)cpuRegs.PERF.n.pccr; break;
			    case 1:
					if((cpuRegs.PERF.n.pccr & 0x800003E0) == 0x80000020) {
						cpuRegs.PERF.n.pcr0 += cpuRegs.cycle-s_iLastPERFCycle[0];
						s_iLastPERFCycle[0] = cpuRegs.cycle;
					}
        
                    cpuRegs.GPR.r[_Rt_].UD[0] = (s64)cpuRegs.PERF.n.pcr0;
                    break;
			    case 3:
					if((cpuRegs.PERF.n.pccr & 0x800F8000) == 0x80008000) {
						cpuRegs.PERF.n.pcr1 += cpuRegs.cycle-s_iLastPERFCycle[1];
						s_iLastPERFCycle[1] = cpuRegs.cycle;
					}
					cpuRegs.GPR.r[_Rt_].UD[0] = (s64)cpuRegs.PERF.n.pcr1;
					break;
		    }
		    /*SysPrintf("MFC0 PCCR = %x PCR0 = %x PCR1 = %x IMM= %x\n", 
		    cpuRegs.PERF.n.pccr, cpuRegs.PERF.n.pcr0, cpuRegs.PERF.n.pcr1, _Imm_ & 0x3F);*/
		    break;
		case 24: 
			SysPrintf("MFC0 Breakpoint debug Registers code = %x\n", cpuRegs.code & 0x3FF);
			break;
		case 9:
			// update
			cpuRegs.CP0.n.Count += cpuRegs.cycle-s_iLastCOP0Cycle;
			s_iLastCOP0Cycle = cpuRegs.cycle;
		default: cpuRegs.GPR.r[_Rt_].UD[0] = (s64)cpuRegs.CP0.r[_Rd_];
	}
}

void MTC0() {
	COP0_LOG("%s\n", disR5900Current.getCString());
	//if(bExecBIOS == FALSE && _Rd_ == 25) SysPrintf("MTC0 _Rd_ %x = %x\n", _Rd_, cpuRegs.CP0.r[_Rd_]);
	switch (_Rd_) {
		case 25: 
			/*if(bExecBIOS == FALSE && _Rd_ == 25) SysPrintf("MTC0 PCCR = %x PCR0 = %x PCR1 = %x IMM= %x\n", 
				cpuRegs.PERF.n.pccr, cpuRegs.PERF.n.pcr0, cpuRegs.PERF.n.pcr1, _Imm_ & 0x3F);*/
			switch(_Imm_ & 0x3F){
				case 0:
					if((cpuRegs.PERF.n.pccr & 0x800003E0) == 0x80000020)
						cpuRegs.PERF.n.pcr0 += cpuRegs.cycle-s_iLastPERFCycle[0];
					if((cpuRegs.PERF.n.pccr & 0x800F8000) == 0x80008000)
						cpuRegs.PERF.n.pcr1 += cpuRegs.cycle-s_iLastPERFCycle[1];
					cpuRegs.PERF.n.pccr = cpuRegs.GPR.r[_Rt_].UL[0];
					s_iLastPERFCycle[0] = cpuRegs.cycle;
					s_iLastPERFCycle[1] = cpuRegs.cycle;
					break;
				case 1: cpuRegs.PERF.n.pcr0 = cpuRegs.GPR.r[_Rt_].UL[0]; s_iLastPERFCycle[0] = cpuRegs.cycle; break;
				case 3: cpuRegs.PERF.n.pcr1 = cpuRegs.GPR.r[_Rt_].UL[0]; s_iLastPERFCycle[1] = cpuRegs.cycle; break;
			}
			break;
		case 24: 
			SysPrintf("MTC0 Breakpoint debug Registers code = %x\n", cpuRegs.code & 0x3FF);
			break;
		case 12: WriteCP0Status(cpuRegs.GPR.r[_Rt_].UL[0]); break;
		case 9: s_iLastCOP0Cycle = cpuRegs.cycle; cpuRegs.CP0.r[9] = cpuRegs.GPR.r[_Rt_].UL[0]; break;
		default: cpuRegs.CP0.r[_Rd_] = cpuRegs.GPR.r[_Rt_].UL[0]; break;
	}
}

int CPCOND0() {
	return (((psHu16(DMAC_STAT) | ~psHu16(DMAC_PCR)) & 0x3ff) == 0x3ff);
}

//#define CPCOND0	1

#define BC0(cond) \
	if (CPCOND0() cond) { \
		intDoBranch(_BranchTarget_); \
	}

void BC0F() {
	BC0(== 0);
	COP0_LOG( "COP0 > BC0F\n" );
}

void BC0T() {
	BC0(== 1);
	COP0_LOG( "COP0 > BC0T\n" );
}

#define BC0L(cond) \
	if (CPCOND0() cond) { \
		intDoBranch(_BranchTarget_); \
	} else cpuRegs.pc+= 4;

void BC0FL() {
	BC0L(== 0);
	COP0_LOG( "COP0 > BC0FL\n" );
}

void BC0TL() {
	BC0L(== 1);
	COP0_LOG( "COP0 > BCOTL\n" );
}

void TLBR() {
/*	CPU_LOG("COP0_TLBR %d:%x,%x,%x,%x\n",
			cpuRegs.CP0.n.Random,   cpuRegs.CP0.n.PageMask, cpuRegs.CP0.n.EntryHi,
			cpuRegs.CP0.n.EntryLo0, cpuRegs.CP0.n.EntryLo1);*/

	int i = cpuRegs.CP0.n.Index&0x1f;

	COP0_LOG("COP0 > TLBR\n");
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

	EntryHi32.u=cpuRegs.CP0.n.EntryHi;

	cpuRegs.CP0.n.Index=0xFFFFFFFF;
	for(i=0;i<48;i++){
		if(tlb[i].VPN2==((~tlb[i].Mask)&(EntryHi32.s.VPN2))
		&&((tlb[i].G&1)||((tlb[i].ASID & 0xff) == EntryHi32.s.ASID))) {
			cpuRegs.CP0.n.Index=i;
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
	UpdateCP0Status();
	intSetBranch();
}

void DI() {
	if (cpuRegs.CP0.n.Status.b._EDI || cpuRegs.CP0.n.Status.b.EXL ||
		cpuRegs.CP0.n.Status.b.ERL || (cpuRegs.CP0.n.Status.b.KSU == 0)) {
		cpuRegs.CP0.n.Status.b.EIE = 0;
		//UpdateCP0Status();		// ints are disabled so checking for them is kinda silly...
	}
}

void EI() {
	if (cpuRegs.CP0.n.Status.b._EDI || cpuRegs.CP0.n.Status.b.EXL ||
		cpuRegs.CP0.n.Status.b.ERL || (cpuRegs.CP0.n.Status.b.KSU == 0)) {
		cpuRegs.CP0.n.Status.b.EIE = 1;
		UpdateCP0Status();
	}
}

} } } }	// end namespace R5900::Interpreter::OpcodeImpl