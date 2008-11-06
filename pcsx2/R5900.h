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

#ifndef __R5900_H__
#define __R5900_H__

#include <stdio.h>

typedef struct {
	int  (*Init)();
	void (*Reset)();
	void (*Step)();
	void (*Execute)();			/* executes up to a break */
	void (*ExecuteBlock)();		/* executes up to a jump */
	void (*ExecuteVU0Block)();	/* executes up to a jump */
	void (*ExecuteVU1Block)();	/* executes up to a jump */
	void (*EnableVU0micro)(int enable);
	void (*EnableVU1micro)(int enable);
	void (*Clear)(u32 Addr, u32 Size);
	void (*ClearVU0)(u32 Addr, u32 Size);
	void (*ClearVU1)(u32 Addr, u32 Size);
	void (*Shutdown)();
} R5900cpu;

extern R5900cpu *Cpu;
extern R5900cpu intCpu;
extern R5900cpu recCpu;
extern u32 bExecBIOS;

typedef union {   // Declare union type GPR register
	u64 UD[2];      //128 bits
	s64 SD[2];
	u32 UL[4];
	s32 SL[4];
	u16 US[8];
	s16 SS[8];
	u8  UC[16];
	s8  SC[16];
} GPR_reg;

typedef union {
	struct {
		GPR_reg r0, at, v0, v1, a0, a1, a2, a3,
				t0, t1, t2, t3, t4, t5, t6, t7,
				s0, s1, s2, s3, s4, s5, s6, s7,
				t8, t9, k0, k1, gp, sp, s8, ra;
	} n;
	GPR_reg r[32];
} GPRregs;

typedef union {
	struct {
		u32 pccr, pcr0, pcr1, pad;
	} n;
	u32 r[4];
} PERFregs;

typedef union {
	struct {
		u32	Index,    Random,    EntryLo0,  EntryLo1,
			Context,  PageMask,  Wired,     Reserved0,
			BadVAddr, Count,     EntryHi,   Compare;
		union {
			struct {
				int IE:1;
				int EXL:1;
				int ERL:1;
				int KSU:2;
				int unused0:3;
				int IM:8;
				int EIE:1;
				int _EDI:1;
				int CH:1;
				int unused1:3;
				int BEV:1;
				int DEV:1;
				int unused2:2;
				int FR:1;
				int unused3:1;
				int CU:4;
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
} CP0regs;

typedef struct {
    GPRregs GPR;		// GPR regs
	// NOTE: don't change order since recompiler uses it
	GPR_reg HI;
	GPR_reg LO;			// hi & log 128bit wide
	CP0regs CP0;		// is COP0 32bit?
	u32 sa;				// shift amount (32bit), needs to be 16 byte aligned
	u32 constzero;		// always 0, for MFSA
    u32 pc;				// Program counter, when changing offset in struct, check iR5900-X.S to make sure offset is correct
    u32 code;			// The instruction
    PERFregs PERF;
	u32 eCycle[32];
	u32 sCycle[32];		// for internal counters
	u32 cycle;			// calculate cpucycles..
	u32 interrupt;
	int branch;
	int opmode;			// operating mode
	u32 tempcycles;	
} cpuRegisters;

extern int EEsCycle;
extern u32 EEoCycle, IOPoCycle;
extern PCSX2_ALIGNED16_DECL(cpuRegisters cpuRegs);

// used for optimization
typedef union {
	u64 UD[1];      //64 bits
	s64 SD[1];
	u32 UL[2];
	s32 SL[3];
	u16 US[4];
	s16 SS[4];
	u8  UC[8];
	s8  SC[8];
} GPR_reg64;

#define GPR_IS_CONST1(reg) ((reg)<32 && (g_cpuHasConstReg&(1<<(reg))))
#define GPR_IS_CONST2(reg1, reg2) ((g_cpuHasConstReg&(1<<(reg1)))&&(g_cpuHasConstReg&(1<<(reg2))))
#define GPR_SET_CONST(reg) { \
	if( (reg) < 32 ) { \
		g_cpuHasConstReg |= (1<<(reg)); \
		g_cpuFlushedConstReg &= ~(1<<(reg)); \
	} \
}

#define GPR_DEL_CONST(reg) { \
	if( (reg) < 32 ) g_cpuHasConstReg &= ~(1<<(reg)); \
}

extern PCSX2_ALIGNED16_DECL(GPR_reg64 g_cpuConstRegs[32]);
extern u32 g_cpuHasConstReg, g_cpuFlushedConstReg;

typedef union {
	float f;
	u32 UL;
} FPRreg;

typedef struct {
	FPRreg fpr[32];		// 32bit floating point registers
	u32 fprc[32];		// 32bit floating point control registers
	FPRreg ACC;			// 32 bit accumulator 
} fpuRegisters;

extern PCSX2_ALIGNED16_DECL(fpuRegisters fpuRegs);


typedef struct {
	u32 PageMask,EntryHi;
	u32 EntryLo0,EntryLo1;
	u32 Mask, nMask;
	u32 G;
	u32 ASID;
	u32 VPN2;
	u32 PFN0;
	u32 PFN1;
} tlbs;

extern PCSX2_ALIGNED16_DECL(tlbs tlb[48]);

#ifndef _PC_

#define _i64(x) (s64)x
#define _u64(x) (u64)x

#define _i32(x) (s32)x
#define _u32(x) (u32)x

#define _i16(x) (s16)x
#define _u16(x) (u16)x

#define _i8(x) (s8)x
#define _u8(x) (u8)x

/**** R3000A Instruction Macros ****/
#define _PC_       cpuRegs.pc       // The next PC to be executed

#define _Funct_  ((cpuRegs.code      ) & 0x3F)  // The funct part of the instruction register 
#define _Rd_     ((cpuRegs.code >> 11) & 0x1F)  // The rd part of the instruction register 
#define _Rt_     ((cpuRegs.code >> 16) & 0x1F)  // The rt part of the instruction register 
#define _Rs_     ((cpuRegs.code >> 21) & 0x1F)  // The rs part of the instruction register 
#define _Sa_     ((cpuRegs.code >>  6) & 0x1F)  // The sa part of the instruction register
#define _Im_     ((u16)cpuRegs.code) // The immediate part of the instruction register
#define _Target_ (cpuRegs.code & 0x03ffffff)    // The target part of the instruction register

#define _Imm_	((s16)cpuRegs.code) // sign-extended immediate
#define _ImmU_	(cpuRegs.code&0xffff) // zero-extended immediate


//#define _JumpTarget_     ((_Target_ * 4) + (_PC_ & 0xf0000000))   // Calculates the target during a jump instruction
//#define _BranchTarget_  ((s16)_Im_ * 4 + _PC_)                 // Calculates the target during a branch instruction

#define _JumpTarget_     ((_Target_ << 2) + (_PC_ & 0xf0000000))   // Calculates the target during a jump instruction
#define _BranchTarget_  (((s32)(s16)_Im_ * 4) + _PC_)                 // Calculates the target during a branch instruction

#define _SetLink(x)     cpuRegs.GPR.r[x].UD[0] = _PC_ + 4;       // Sets the return address in the link register

#endif

int  cpuInit();
void cpuReset();
void cpuShutdown();
void cpuException(u32 code, u32 bd);
void cpuTlbMissR(u32 addr, u32 bd);
void cpuTlbMissW(u32 addr, u32 bd);
void IntcpuBranchTest();
void cpuBranchTest();
void cpuTestHwInts();
void cpuTestINTCInts();
void cpuTestDMACInts();
void cpuTestTIMRInts();
void _cpuTestInterrupts();
void cpuExecuteBios();
void cpuRestartCPU();

u32  VirtualToPhysicalR(u32 addr);
u32  VirtualToPhysicalW(u32 addr);

void intDoBranch(u32 target);
void intSetBranch();
void intExecuteVU0Block();
void intExecuteVU1Block();

void JumpCheckSym(u32 addr, u32 pc);
void JumpCheckSymRet(u32 addr);

extern u32 g_EEFreezeRegs;

//exception code
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

#define EXC_TLB_STORE 1
#define EXC_TLB_LOAD  0

//#define EE_PROFILING //EE Profiling enable

#ifdef EE_PROFILING //EE Profiling code

extern u64 profile_starttick;
extern u64 profile_totalticks;

#define START_EE_PROFILE() \
		profile_starttick = GetCPUTick();

#define END_EE_PROFILE() \
		profile_totalticks += GetCPUTick()-profile_starttick;

#define CLEAR_EE_PROFILE() \
		profile_totalticks = 0;

#else
#define START_EE_PROFILE()

#define END_EE_PROFILE()

#define CLEAR_EE_PROFILE()
#endif

#endif /* __R5900_H__ */
