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

#ifndef __R5900_H__
#define __R5900_H__

extern bool g_EEFreezeRegs;

// EE Bios function name tables.
namespace R5900 {
extern const char* const bios[256];
}

extern s32 EEsCycle;
extern u32 EEoCycle;
extern bool g_ExecBiosHack;

union GPR_reg {   // Declare union type GPR register
	u64 UD[2];      //128 bits
	s64 SD[2];
	u32 UL[4];
	s32 SL[4];
	u16 US[8];
	s16 SS[8];
	u8  UC[16];
	s8  SC[16];
};

union GPRregs {
	struct {
		GPR_reg r0, at, v0, v1, a0, a1, a2, a3,
				t0, t1, t2, t3, t4, t5, t6, t7,
				s0, s1, s2, s3, s4, s5, s6, s7,
				t8, t9, k0, k1, gp, sp, s8, ra;
	} n;
	GPR_reg r[32];
};

union PERFregs {
	struct
	{
		union
		{
			struct  
			{
				u32 pad0:1;			// LSB should always be zero (or undefined)
				u32 EXL0:1;			// enable PCR0 during Level 1 exception handling
				u32 K0:1;			// enable PCR0 during Kernel Mode execution
				u32 S0:1;			// enable PCR0 during Supervisor mode execution
				u32 U0:1;			// enable PCR0 during User-mode execution
				u32 Event0:5;		// PCR0 event counter (all values except 1 ignored at this time)

				u32 pad1:1;			// more zero/undefined padding [bit 10]

				u32 EXL1:1;			// enable PCR1 during Level 1 exception handling
				u32 K1:1;			// enable PCR1 during Kernel Mode execution
				u32 S1:1;			// enable PCR1 during Supervisor mode execution
				u32 U1:1;			// enable PCR1 during User-mode execution
				u32 Event1:5;		// PCR1 event counter (all values except 1 ignored at this time)
				
				u32 Reserved:11;
				u32 CTE:1;			// Counter enable bit, no counting if set to zero.
			} b;
			
			u32 val;
		} pccr;

		u32 pcr0, pcr1, pad;
	} n;
	u32 r[4];
};

union CP0regs {
	struct {
		u32	Index,    Random,    EntryLo0,  EntryLo1,
			Context,  PageMask,  Wired,     Reserved0,
			BadVAddr, Count,     EntryHi,   Compare;
		union {
			struct {
				u32 IE:1;		// Bit 0: Interrupt Enable flag.
				u32 EXL:1;		// Bit 1: Exception Level, set on any exception not covered by ERL.
				u32 ERL:1;		// Bit 2: Error level, set on Resetm NMI, perf/debug exceptions.
				u32 KSU:2;		// Bits 3-4: Kernel [clear] / Supervisor [set] mode 
				u32 unused0:3;
				u32 IM:8;		// Bits 10-15: Interrupt mask (bits 12,13,14 are unused)
				u32 EIE:1;		// Bit 16: IE bit enabler.  When cleared, ints are disabled regardless of IE status.
				u32 _EDI:1;		// Bit 17: Interrupt Enable (set enables ints in all modes, clear enables ints in kernel mode only)
				u32 CH:1;		// Bit 18: Status of most recent cache instruction (set for hit, clear for miss)
				u32 unused1:3;
				u32 BEV:1;		// Bit 22: if set, use bootstrap for TLB/general exceptions
				u32 DEV:1;		// Bit 23: if set, use bootstrap for perf/debug exceptions
				u32 unused2:2;
				u32 FR:1;		// (?)
				u32 unused3:1;
				u32 CU:4;		// Bits 28-31: Co-processor Usable flag
			} b;
			u32 val;
		} Status;
		u32   Cause,    EPC,       PRid,
			Config,   LLAddr,    WatchLO,   WatchHI,
			XContext, Reserved1, Reserved2, Debug,
			DEPC,     PerfCnt,   ErrCtl,    CacheErr,
			TagLo,    TagHi,     ErrorEPC,  DESAVE;
	} n;
	u32 r[32];
};

struct cpuRegisters {
	GPRregs GPR;		// GPR regs
	// NOTE: don't change order since recompiler uses it
	GPR_reg HI;
	GPR_reg LO;			// hi & log 128bit wide
	CP0regs CP0;		// is COP0 32bit?
	u32 sa;				// shift amount (32bit), needs to be 16 byte aligned
	u32 IsDelaySlot;	// set true when the current instruction is a delay slot.
	u32 pc;				// Program counter, when changing offset in struct, check iR5900-X.S to make sure offset is correct
	u32 code;			// current instruction
	PERFregs PERF;
	u32 eCycle[32];
	u32 sCycle[32];		// for internal counters
	u32 cycle;			// calculate cpucycles..
	u32 interrupt;
	int branch;
	int opmode;			// operating mode
	u32 tempcycles;
};

// used for optimization
union GPR_reg64 {
	u64 UD[1];      //64 bits
	s64 SD[1];
	u32 UL[2];
	s32 SL[3];
	u16 US[4];
	s16 SS[4];
	u8  UC[8];
	s8  SC[8];
};

union FPRreg {
	float f;
	u32 UL;
	s32 SL;				// signed 32bit used for sign extension in interpreters.
};

struct fpuRegisters {
	FPRreg fpr[32];		// 32bit floating point registers
	u32 fprc[32];		// 32bit floating point control registers
	FPRreg ACC;			// 32 bit accumulator 
	u32 ACCflag;        // an internal accumulator overflow flag
};

struct tlbs
{
	u32 PageMask,EntryHi;
	u32 EntryLo0,EntryLo1;
	u32 Mask, nMask;
	u32 G;
	u32 ASID;
	u32 VPN2;
	u32 PFN0;
	u32 PFN1;
	u32 S;
};

#ifndef _PC_

/*#define _i64(x) (s64)x
#define _u64(x) (u64)x

#define _i32(x) (s32)x
#define _u32(x) (u32)x

#define _i16(x) (s16)x
#define _u16(x) (u16)x

#define _i8(x) (s8)x
#define _u8(x) (u8)x*/

////////////////////////////////////////////////////////////////////
// R5900 Instruction Macros

#define _PC_       cpuRegs.pc       // The next PC to be executed - only used in this header and R3000A.h

#define _Funct_  ((cpuRegs.code      ) & 0x3F)  // The funct part of the instruction register 
#define _Rd_     ((cpuRegs.code >> 11) & 0x1F)  // The rd part of the instruction register 
#define _Rt_     ((cpuRegs.code >> 16) & 0x1F)  // The rt part of the instruction register 
#define _Rs_     ((cpuRegs.code >> 21) & 0x1F)  // The rs part of the instruction register 
#define _Sa_     ((cpuRegs.code >>  6) & 0x1F)  // The sa part of the instruction register
#define _Im_     ((u16)cpuRegs.code) // The immediate part of the instruction register
#define _Target_ (cpuRegs.code & 0x03ffffff)    // The target part of the instruction register

#define _Imm_	((s16)cpuRegs.code) // sign-extended immediate
#define _ImmU_	(cpuRegs.code&0xffff) // zero-extended immediate
#define _ImmSB_	(cpuRegs.code&0x8000) // gets the sign-bit of the immediate value

#define _Opcode_ (cpuRegs.code >> 26 )

#define _JumpTarget_     ((_Target_ << 2) + (_PC_ & 0xf0000000))   // Calculates the target during a jump instruction
#define _BranchTarget_   (((s32)(s16)_Im_ * 4) + _PC_)                 // Calculates the target during a branch instruction
#define _TrapCode_       ((u16)cpuRegs.code >> 6)	// error code for non-immediate trap instructions.

#define _SetLink(x)     (cpuRegs.GPR.r[x].UD[0] = _PC_ + 4)       // Sets the return address in the link register

#endif

PCSX2_ALIGNED16_EXTERN(cpuRegisters cpuRegs);
PCSX2_ALIGNED16_EXTERN(fpuRegisters fpuRegs);
PCSX2_ALIGNED16_EXTERN(tlbs tlb[48]);

extern u32 g_nextBranchCycle;
extern bool eeEventTestIsActive;
extern u32 s_iLastCOP0Cycle;
extern u32 s_iLastPERFCycle[2];

void intEventTest();
void intSetBranch();

// This is a special form of the interpreter's doBranch that is run from various
// parts of the Recs (namely COP0's branch codes and stuff).
void __fastcall intDoBranch(u32 target);

////////////////////////////////////////////////////////////////////
// R5900 Public Interface / API

struct R5900cpu
{
	void (*Allocate)();		// throws exceptions on failure.
	void (*Reset)();
	void (*Step)();
	void (*Execute)();
	void (*Clear)(u32 Addr, u32 Size);
	void (*Shutdown)();		// deallocates memory reserved by Allocate
};

extern R5900cpu *Cpu;
extern R5900cpu intCpu;
extern R5900cpu recCpu;

extern void cpuInit();
extern void cpuReset();		// can throw Exception::FileNotFound.
extern void cpuShutdown();
extern void cpuExecuteBios();
extern void cpuException(u32 code, u32 bd);
extern void cpuTlbMissR(u32 addr, u32 bd);
extern void cpuTlbMissW(u32 addr, u32 bd);
extern void cpuTestHwInts();

extern void cpuSetNextBranch( u32 startCycle, s32 delta );
extern void cpuSetNextBranchDelta( s32 delta );
extern int  cpuTestCycle( u32 startCycle, s32 delta );
extern void cpuSetBranch();

extern void _cpuBranchTest_Shared();		// for internal use by the Dynarecs and Ints inside R5900:

extern void cpuTestINTCInts();
extern void cpuTestDMACInts();
extern void cpuTestTIMRInts();

////////////////////////////////////////////////////////////////////
// Exception Codes

#define EXC_CODE(x)     ((x)<<2)

#define EXC_CODE_Int    EXC_CODE(0)
#define EXC_CODE_Mod    EXC_CODE(1)     /* TLB Modification exception */
#define EXC_CODE_TLBL   EXC_CODE(2)     /* TLB Miss exception (load or instruction fetch) */
#define EXC_CODE_TLBS   EXC_CODE(3)     /* TLB Miss exception (store) */
#define EXC_CODE_AdEL   EXC_CODE(4)
#define EXC_CODE_AdES   EXC_CODE(5)
#define EXC_CODE_IBE    EXC_CODE(6)
#define EXC_CODE_DBE    EXC_CODE(7)
#define EXC_CODE_Sys    EXC_CODE(8)
#define EXC_CODE_Bp     EXC_CODE(9)
#define EXC_CODE_Ri     EXC_CODE(10)
#define EXC_CODE_CpU    EXC_CODE(11)
#define EXC_CODE_Ov     EXC_CODE(12)
#define EXC_CODE_Tr     EXC_CODE(13)
#define EXC_CODE_FPE    EXC_CODE(15)
#define EXC_CODE_WATCH  EXC_CODE(23)
#define EXC_CODE__MASK  0x0000007c
#define EXC_CODE__SHIFT 2

#endif /* __R5900_H__ */
