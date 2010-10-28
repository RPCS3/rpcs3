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

class BaseR5900Exception;

// --------------------------------------------------------------------------------------
//  Recompiler Stuffs
// --------------------------------------------------------------------------------------
// This code section contains recompiler vars that are used in "shared" code. Placing
// them in iR5900.h would mean having to include that into more files than I care to
// right now, so we're sticking them here for now until a better solution comes along.

extern bool g_SkipBiosHack;
extern bool g_GameStarted;

namespace Exception
{
	// Implementation Note: this exception has no meaningful type information and we don't
	// care to have it be caught by any BaseException handlers lying about, so let's not
	// derive from BaseException :D
	class ExitCpuExecute
	{
	public:
		explicit ExitCpuExecute() { }
	};
}

// --------------------------------------------------------------------------------------
//  EE Bios function name tables.
// --------------------------------------------------------------------------------------
namespace R5900 {
extern const char* const bios[256];
}

extern s32 EEsCycle;
extern u32 EEoCycle;

union GPR_reg {   // Declare union type GPR register
	u128 UQ;
	s128 SQ;
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

extern __aligned16 cpuRegisters cpuRegs;
extern __aligned16 fpuRegisters fpuRegs;
extern __aligned16 tlbs tlb[48];

extern u32 g_nextEventCycle;
extern bool eeEventTestIsActive;
extern u32 s_iLastCOP0Cycle;
extern u32 s_iLastPERFCycle[2];

void intSetBranch();

// This is a special form of the interpreter's doBranch that is run from various
// parts of the Recs (namely COP0's branch codes and stuff).
void __fastcall intDoBranch(u32 target);

// modules loaded at hardcoded addresses by the kernel
const u32 EEKERNEL_START	= 0;
const u32 EENULL_START		= 0x81FC0;
const u32 EELOAD_START		= 0x82000;
const u32 EELOAD_SIZE		= 0x20000; // overestimate for searching

extern void __fastcall eeGameStarting();
extern void __fastcall eeloadReplaceOSDSYS();

// --------------------------------------------------------------------------------------
//  R5900cpu
// --------------------------------------------------------------------------------------
// [TODO] : This is on the list to get converted to a proper C++ class.  I'm putting it
// off until I get my new IOPint and IOPrec re-merged. --air
//
struct R5900cpu
{
	// Memory allocation function, for allocating virtual memory spaces needed by
	// the virtual cpu provider.  Allocating additional heap memory from this method is
	// NOT recommended.  Heap allocations should be performed by Reset only.  This
	// maximizes the likeliness of reservations claiming addresses they prefer.
	// 
	// Thread Affinity:
	//   Called from the main/UI thread only.  Cpu execution status is guaranteed to
	//   be inactive.  No locking is necessary.
	//
	// Exception Throws:
	//   HardwareDeficiency - The host machine's hardware does not support this CPU provider.
	//   OutOfMemory - Not enough memory, or the memory areas required were already
	//                 reserved.
	void (*Reserve)();

	// Deallocates ram allocated by Allocate, Reserve, and/or by runtime code execution.
	//
	// Thread Affinity:
	//   Called from the main/UI thread only.  Cpu execution status is guaranteed to
	//   be inactive.  No locking is necessary.
	//
	// Exception Throws:  None.  This function is a destructor, and should not throw.
	//
	void (*Shutdown)();

	// Initializes / Resets code execution states. Typically implementation is only
	// needed for recompilers, as interpreters have no internal execution states and
	// rely on the CPU/VM states almost entirely.
	//
	// Thread Affinity:
	//   Can be called from any thread.  CPU execution status is indeterminate and may
	//   already be in progress.  Implementations should be sure to queue and execute
	//   resets at the earliest safe convenience (typically right before recompiling a
	//   new block of code, or after a vsync event).
	//
	// Exception Throws:  Emulator-defined.  Common exception types to expect are
	//   OutOfMemory, Stream Exceptions
	//
	void (*Reset)();

	// Steps a single instruction.  Meant to be used by debuggers.  Is currently unused
	// and unimplemented.  Future note: recompiler "step" should *always* fall back
	// on interpreters.
	//
	// Exception Throws:  [TODO] (possible execution-related throws to be added)
	//
	void (*Step)();

	// Executes code until a break is signaled.  Execution can be paused or suspended
	// via thread-style signals that are handled by CheckExecutionState callbacks.
	// Execution Breakages are handled the same way, where-by a signal causes the Execute
	// call to return at the nearest state check (typically handled internally using
	// either C++ exceptions or setjmp/longjmp).
	//
	// Exception Throws: 
	//   Throws BaseR5900Exception and all derivatives.
	//   Throws FileNotFound or other Streaming errors (typically related to BIOS MEC/NVM)
	//
	void (*Execute)();

	// Checks for execution suspension or cancellation.  In pthreads terms this provides
	// a "cancellation point."  Execution state checks are typically performed at Vsyncs
	// by the generic VM event handlers in R5900.cpp/Counters.cpp (applies to both recs
	// and ints).
	//
	// Implementation note: Because of the nuances of recompiled code execution, setjmp
	// may be used in place of thread cancellation or C++ exceptions (non-SEH exceptions
	// cannot unwind through the recompiled code stackframes, thus longjmp must be used).
	//
	// Thread Affinity:
	//   Must be called on the same thread as Execute.
	//
	// Exception Throws:
	//   May throw Execution/Pthreads cancellations if the compiler supports SEH.
	//
	void (*CheckExecutionState)();

	// Safely throws host exceptions from executing code (either recompiled or interpreted).
	// If this function is called outside the context of the CPU's code execution, then the
	// given exception will be re-thrown automatically.
	// 
	// Exception Throws:
	//   (SEH) Rethrows the given exception immediately.
	//   (setjmp) Re-throws immediately if called from outside the context of dynamically
	//      generated code (either non-executing contexts or interpreters).  Does not throw
	//      otherwise.
	//
	void (*ThrowException)( const BaseException& ex );
	void (*ThrowCpuException)( const BaseR5900Exception& ex );

	// Manual recompiled code cache clear; typically useful to recompilers only.  Size is
	// in MIPS words (32 bits).  Dev note: this callback is nearly obsolete, and might be
	// better off replaced with some generic API callbacks from VTLB block protection.
	// Also: the calls from COP0's TLB remap code should be replaced with full recompiler
	// resets, since TLB remaps affect more than just the code they contain (code that
	// may reference the remapped blocks via memory loads/stores, for example).
	//
	// Thread Affinity Rule:
	//   Can be called from any thread (namely for being called from debugging threads)
	//
	// Exception Throws: [TODO] Emulator defined?  (probably shouldn't throw, probably
	//   doesn't matter if we're stripping it out soon. ;)
	//
	void (*Clear)(u32 Addr, u32 Size);
	
	uint (*GetCacheReserve)();
	void (*SetCacheReserve)( uint reserveInMegs );
};

extern R5900cpu *Cpu;
extern R5900cpu intCpu;
extern R5900cpu recCpu;

enum EE_EventType
{
	DMAC_VIF0	= 0,
	DMAC_VIF1,
	DMAC_GIF,
	DMAC_FROM_IPU,
	DMAC_TO_IPU,
	DMAC_SIF0,
	DMAC_SIF1,
	DMAC_SIF2,
	DMAC_FROM_SPR,
	DMAC_TO_SPR,

	DMAC_MFIFO_VIF,
	DMAC_MFIFO_GIF,

	// We're setting error conditions through hwDmacIrq, so these correspond to the conditions above.
	DMAC_STALL_SIS		= 13, // SIS
	DMAC_MFIFO_EMPTY	= 14, // MEIS
	DMAC_BUS_ERROR	= 15      // BEIS
};

extern void CPU_INT( EE_EventType n, s32 ecycle );
extern uint intcInterrupt();
extern uint dmacInterrupt();


extern void cpuInit();
extern void cpuReset();		// can throw Exception::FileNotFound.
extern void cpuException(u32 code, u32 bd);
extern void cpuTlbMissR(u32 addr, u32 bd);
extern void cpuTlbMissW(u32 addr, u32 bd);
extern void cpuTestHwInts();
extern void cpuClearInt(uint n);

extern void cpuSetNextEvent( u32 startCycle, s32 delta );
extern void cpuSetNextEventDelta( s32 delta );
extern int  cpuTestCycle( u32 startCycle, s32 delta );
extern void cpuSetEvent();

extern void _cpuEventTest_Shared();		// for internal use by the Dynarecs and Ints inside R5900:

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
