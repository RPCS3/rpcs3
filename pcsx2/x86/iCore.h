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

#ifndef _PCSX2_CORE_RECOMPILER_
#define _PCSX2_CORE_RECOMPILER_

#include "x86emitter/x86emitter.h"
#include "sVU_Micro.h"

// Namespace Note : iCore32 contains all of the Register Allocation logic, in addition to a handful
// of utility functions for emitting frequent code.

////////////////////////////////////////////////////////////////////////////////
// Shared Register allocation flags (apply to X86, XMM, MMX, etc).

#define MODE_READ		1
#define MODE_WRITE		2
#define MODE_READHALF	4 // read only low 64 bits
#define MODE_VUXY		0x8	// vector only has xy valid (real zw are in mem), not the same as MODE_READHALF
#define MODE_VUZ		0x10 // z only doesn't work for now
#define MODE_VUXYZ		(MODE_VUZ|MODE_VUXY) // vector only has xyz valid (real w is in memory)
#define MODE_NOFLUSH	0x20	// can't flush reg to mem
#define MODE_NOFRAME	0x40	// when allocating x86regs, don't use ebp reg
#define MODE_8BITREG	0x80	// when allocating x86regs, use only eax, ecx, edx, and ebx

//#define PROCESS_EE_MMX 0x01 // removed
#define PROCESS_EE_XMM 0x02

// currently only used in FPU
#define PROCESS_EE_S		0x04 // S is valid, otherwise take from mem
#define PROCESS_EE_T		0x08 // T is valid, otherwise take from mem

// not used in VU recs
#define PROCESS_EE_MODEWRITES 0x10 // if s is a reg, set if not in cpuRegs
#define PROCESS_EE_MODEWRITET 0x20 // if t is a reg, set if not in cpuRegs
#define PROCESS_EE_LO		0x40 // lo reg is valid
#define PROCESS_EE_HI		0x80 // hi reg is valid
#define PROCESS_EE_ACC		0x40 // acc reg is valid

// used in VU recs
#define PROCESS_VU_UPDATEFLAGS 0x10
#define PROCESS_VU_SUPER	0x40 // set if using supervu recompilation
#define PROCESS_VU_COP2		0x80 // simple cop2

#define EEREC_S (((info)>>8)&0xf)
#define EEREC_T (((info)>>12)&0xf)
#define EEREC_D (((info)>>16)&0xf)
#define EEREC_LO (((info)>>20)&0xf)
#define EEREC_HI (((info)>>24)&0xf)
#define EEREC_ACC (((info)>>20)&0xf)
#define EEREC_TEMP (((info)>>24)&0xf)
#define VUREC_FMAC ((info)&0x80000000)

#define PROCESS_EE_SET_S(reg) ((reg)<<8)
#define PROCESS_EE_SET_T(reg) ((reg)<<12)
#define PROCESS_EE_SET_D(reg) ((reg)<<16)
#define PROCESS_EE_SET_LO(reg) ((reg)<<20)
#define PROCESS_EE_SET_HI(reg) ((reg)<<24)
#define PROCESS_EE_SET_ACC(reg) ((reg)<<20)

#define PROCESS_VU_SET_ACC(reg) PROCESS_EE_SET_ACC(reg)
#define PROCESS_VU_SET_TEMP(reg) ((reg)<<24)

#define PROCESS_VU_SET_FMAC() 0x80000000

// special info not related to above flags
#define PROCESS_CONSTS 1
#define PROCESS_CONSTT 2

////////////////////////////////////////////////////////////////////////////////
//   X86 (32-bit) Register Allocation Tools

#define X86TYPE_TEMP 0
#define X86TYPE_GPR 1
#define X86TYPE_VI 2
#define X86TYPE_MEMOFFSET 3
#define X86TYPE_VIMEMOFFSET 4
#define X86TYPE_VUQREAD 5
#define X86TYPE_VUPREAD 6
#define X86TYPE_VUQWRITE 7
#define X86TYPE_VUPWRITE 8
#define X86TYPE_PSX 9
#define X86TYPE_PCWRITEBACK 10
#define X86TYPE_VUJUMP 12		// jump from random mem (g_recWriteback)
#define X86TYPE_VITEMP 13
#define X86TYPE_FNARG 14        // function parameter, max is 4

#define X86TYPE_VU1 0x80

//#define X86_ISVI(type) ((type&~X86TYPE_VU1) == X86TYPE_VI)
static __forceinline int X86_ISVI(int type)
{
	return ((type&~X86TYPE_VU1) == X86TYPE_VI);
}

struct _x86regs {
	u8 inuse;
	u8 reg; // value of 0 - not used
	u8 mode;
	u8 needed;
	u8 type; // X86TYPE_
	u16 counter;
	u32 extra; // extra info assoc with the reg
};

extern _x86regs x86regs[iREGCNT_GPR], s_saveX86regs[iREGCNT_GPR];

uptr _x86GetAddr(int type, int reg);
void _initX86regs();
int  _getFreeX86reg(int mode);
int  _allocX86reg(int x86reg, int type, int reg, int mode);
void _deleteX86reg(int type, int reg, int flush);
int _checkX86reg(int type, int reg, int mode);
void _addNeededX86reg(int type, int reg);
void _clearNeededX86regs();
void _freeX86reg(int x86reg);
void _freeX86regs();
void _flushCachedRegs();
void _flushConstRegs();
void _flushConstReg(int reg);

////////////////////////////////////////////////////////////////////////////////
//   XMM (128-bit) Register Allocation Tools

#define XMM_CONV_VU(VU) (VU==&VU1)

#define XMMTYPE_TEMP	0 // has to be 0
#define XMMTYPE_VFREG	1
#define XMMTYPE_ACC		2
#define XMMTYPE_FPREG	3
#define XMMTYPE_FPACC	4
#define XMMTYPE_GPRREG	5

// lo and hi regs
#define XMMGPR_LO	33
#define XMMGPR_HI	32
#define XMMFPU_ACC	32

struct _xmmregs {
	u8 inuse;
	u8 reg;
	u8 type;
	u8 mode;
	u8 needed;
	u8 VU; // 0 = VU0, 1 = VU1
	u16 counter;
};

void _initXMMregs();
int  _getFreeXMMreg();
int  _allocTempXMMreg(XMMSSEType type, int xmmreg);
int  _allocVFtoXMMreg(VURegs *VU, int xmmreg, int vfreg, int mode);
int  _allocFPtoXMMreg(int xmmreg, int fpreg, int mode);
int  _allocGPRtoXMMreg(int xmmreg, int gprreg, int mode);
int  _allocACCtoXMMreg(VURegs *VU, int xmmreg, int mode);
int  _allocFPACCtoXMMreg(int xmmreg, int mode);
int  _checkXMMreg(int type, int reg, int mode);
void _addNeededVFtoXMMreg(int vfreg);
void _addNeededACCtoXMMreg();
void _addNeededFPtoXMMreg(int fpreg);
void _addNeededFPACCtoXMMreg();
void _addNeededGPRtoXMMreg(int gprreg);
void _clearNeededXMMregs();
void _deleteVFtoXMMreg(int reg, int vu, int flush);
//void _deleteACCtoXMMreg(int vu, int flush);
void _deleteGPRtoXMMreg(int reg, int flush);
void _deleteFPtoXMMreg(int reg, int flush);
void _freeXMMreg(int xmmreg);
//void _moveXMMreg(int xmmreg); // instead of freeing, moves it to a diff location
void _flushXMMregs();
u8 _hasFreeXMMreg();
void _freeXMMregs();
int _getNumXMMwrite();

void _signExtendSFtoM(u32 mem);

// returns new index of reg, lower 32 bits already in mmx
// shift is used when the data is in the top bits of the mmx reg to begin with
// a negative shift is for sign extension
int _signExtendXMMtoM(u32 to, x86SSERegType from, int candestroy); // returns true if reg destroyed

static const int MEM_MMXTAG = 0x0800;	// mmreg is mmxreg
static const int MEM_XMMTAG = 0x8000;	// mmreg is xmmreg

//////////////////////
// Instruction Info //
//////////////////////
#define EEINST_LIVE0	1	// if var is ever used (read or write)
#define EEINST_LIVE1	2	// if cur var's next 32 bits are needed
#define EEINST_LIVE2	4	// if cur var's next 64 bits are needed
#define EEINST_LASTUSE	8	// if var isn't written/read anymore
//#define EEINST_MMX		0x10 // removed
#define EEINST_XMM		0x20	// var will be used in xmm ops
#define EEINST_USED		0x40

#define EEINSTINFO_COP1		1
#define EEINSTINFO_COP2		2

struct EEINST
{
	u8 regs[34]; // includes HI/LO (HI=32, LO=33)
	u8 fpuregs[33]; // ACC=32
	u8 info; // extra info, if 1 inst is COP1, 2 inst is COP2. Also uses EEINST_XMM

	// uses XMMTYPE_ flags; if type == XMMTYPE_TEMP, not used
	u8 writeType[3], writeReg[3]; // reg written in this inst, 0 if no reg
	u8 readType[4], readReg[4];

	// valid if info & EEINSTINFO_COP2
	int cycle; // cycle of inst (at offset from block)
	_VURegsNum vuregs;

	u8 numpeeps; // number of peephole optimizations
};

extern EEINST* g_pCurInstInfo; // info for the cur instruction
extern void _recClearInst(EEINST* pinst);

// returns the number of insts + 1 until written (0 if not written)
extern u32 _recIsRegWritten(EEINST* pinst, int size, u8 xmmtype, u8 reg);
// returns the number of insts + 1 until used (0 if not used)
extern u32 _recIsRegUsed(EEINST* pinst, int size, u8 xmmtype, u8 reg);
extern void _recFillRegister(EEINST& pinst, int type, int reg, int write);

static __forceinline bool EEINST_ISLIVE64(u32 reg)	{ return !!(g_pCurInstInfo->regs[reg] & (EEINST_LIVE0|EEINST_LIVE1)); }
static __forceinline bool EEINST_ISLIVEXMM(u32 reg)	{ return !!(g_pCurInstInfo->regs[reg] & (EEINST_LIVE0|EEINST_LIVE1|EEINST_LIVE2)); }
static __forceinline bool EEINST_ISLIVE1(u32 reg)	{ return !!(g_pCurInstInfo->regs[reg] & EEINST_LIVE1); }
static __forceinline bool EEINST_ISLIVE2(u32 reg)	{ return !!(g_pCurInstInfo->regs[reg] & EEINST_LIVE2); }

static __forceinline bool FPUINST_ISLIVE(u32 reg)	{ return !!(g_pCurInstInfo->fpuregs[reg] & EEINST_LIVE0); }
static __forceinline bool FPUINST_LASTUSE(u32 reg)	{ return !!(g_pCurInstInfo->fpuregs[reg] & EEINST_LASTUSE); }

// if set, then the variable at this inst really has its upper 32 bits valid
// The difference between EEINST_LIVE1 is that the latter is used in back propagation
// The former is set at recompile time.
#define EEINST_RESETHASLIVE1(reg) { if( (reg) < 32 ) g_cpuRegHasLive1 &= ~(1<<(reg)); }
#define EEINST_HASLIVE1(reg) (g_cpuPrevRegHasLive1&(1<<(reg)))

extern u32 g_recWriteback; // used for jumps (VUrec mess!)
extern u32 g_cpuRegHasLive1, g_cpuPrevRegHasLive1;

extern _xmmregs xmmregs[iREGCNT_XMM], s_saveXMMregs[iREGCNT_XMM];

extern __tls_emit u8  *j8Ptr[32];		// depreciated item.  use local u8* vars instead.
extern __tls_emit u32 *j32Ptr[32];		// depreciated item.  use local u32* vars instead.

extern u16 g_x86AllocCounter;
extern u16 g_xmmAllocCounter;

// allocates only if later insts use XMM, otherwise checks
int _allocCheckGPRtoXMM(EEINST* pinst, int gprreg, int mode);
int _allocCheckFPUtoXMM(EEINST* pinst, int fpureg, int mode);

// allocates only if later insts use this register
int _allocCheckGPRtoX86(EEINST* pinst, int gprreg, int mode);

////////////////////////////////////////////////////////////////////////////////
//   MMX (64-bit) Register Allocation Tools

#define FPU_STATE 0
#define MMX_STATE 1

void SetMMXstate();
void SetFPUstate();

// max is 0x7f, when 0x80 is set, need to flush reg
//#define MMX_GET_CACHE(ptr, index) ((u8*)ptr)[index]
//#define MMX_SET_CACHE(ptr, ind3, ind2, ind1, ind0) ((u32*)ptr)[0] = (ind3<<24)|(ind2<<16)|(ind1<<8)|ind0;
#define MMX_GPR 0
#define MMX_HI	XMMGPR_HI
#define MMX_LO	XMMGPR_LO
#define MMX_FPUACC 34
#define MMX_FPU	64
#define MMX_COP0 96
#define MMX_TEMP 0x7f

static __forceinline bool MMX_IS32BITS(s32 x) 
{
	return (((x >= MMX_FPU) && (x < MMX_COP0 + 32)) || (x == MMX_FPUACC));
}

static __forceinline bool MMX_ISGPR(s32 x) 
{
	return ((x >= MMX_GPR) && (x < MMX_GPR + 34)); 
}

static __forceinline bool MMX_ISGPR(u32 x) 
{
	return (x < MMX_GPR + 34);
}

struct _mmxregs {
	u8 inuse;
	u8 reg; // value of 0 - not used
	u8 mode;
	u8 needed;
	u16 counter;
};

void _initMMXregs();
int  _getFreeMMXreg();
int  _allocMMXreg(int MMXreg, int reg, int mode);
void _addNeededMMXreg(int reg);
int _checkMMXreg(int reg, int mode);
void _clearNeededMMXregs();
void _deleteMMXreg(int reg, int flush);
void _freeMMXreg(int mmxreg);
void _moveMMXreg(int mmxreg); // instead of freeing, moves it to a diff location
void _flushMMXregs();
u8 _hasFreeMMXreg();
void _freeMMXregs();
int _getNumMMXwrite();

int _signExtendMtoMMX(x86MMXRegType to, u32 mem);
int _signExtendGPRMMXtoMMX(x86MMXRegType to, u32 gprreg, x86MMXRegType from, u32 gprfromreg);
int _allocCheckGPRtoMMX(EEINST* pinst, int reg, int mode);

void _recMove128RmOffsettoM(u32 to, u32 offset);
void _recMove128MtoRmOffset(u32 offset, u32 from);

// returns new index of reg, lower 32 bits already in mmx
// shift is used when the data is in the top bits of the mmx reg to begin with
// a negative shift is for sign extension
extern int _signExtendGPRtoMMX(x86MMXRegType to, u32 gprreg, int shift);

extern _mmxregs mmxregs[iREGCNT_MMX], s_saveMMXregs[iREGCNT_MMX];
extern u16 x86FpuState;

// extern void iDumpRegisters(u32 startpc, u32 temp);

//////////////////////////////////////////////////////////////////////////
// iFlushCall / _psxFlushCall Parameters

// Flushing vs. Freeing, as understood by Air (I could be wrong still....)

// "Freeing" registers means that the contents of the registers are flushed to memory.
// This is good for any sort of C code function that plans to modify the actual
// registers.  When the Recs resume, they'll reload the registers with values saved
// as needed.  (similar to a "FreezeXMMRegs")

// "Flushing" means that in addition to the standard free (which is actually a flush)
// the register allocations are additionally wiped.  This should only be necessary if 
// the code being called is going to modify register allocations -- ie, be doing
// some kind of recompiling of its own.

#define FLUSH_CACHED_REGS 1
#define FLUSH_FLUSH_XMM 2
#define FLUSH_FREE_XMM 4 // both flushes and frees
#define FLUSH_FLUSH_MMX 8
#define FLUSH_FREE_MMX 16  // both flushes and frees
#define FLUSH_FLUSH_ALLX86 32 // flush x86
#define FLUSH_FREE_TEMPX86 64 // flush and free temporary x86 regs
#define FLUSH_FREE_ALLX86 128 // free all x86 regs
#define FLUSH_FREE_VU0 0x100  // free all vu0 related regs

#define FLUSH_EVERYTHING 0xfff
// no freeing, used when callee won't destroy mmx/xmm regs
#define FLUSH_NODESTROY (FLUSH_CACHED_REGS|FLUSH_FLUSH_XMM|FLUSH_FLUSH_MMX|FLUSH_FLUSH_ALLX86)
// used when regs aren't going to be changed be callee
#define FLUSH_NOCONST	(FLUSH_FREE_XMM|FLUSH_FREE_MMX|FLUSH_FREE_TEMPX86)


//////////////////////////////////////////////////////////////////////////
// Utility Functions -- that should probably be part of the Emitter.

// Moves 128 bits of data using EAX/EDX (used by iCOP2 only currently)
extern void _recMove128MtoM(u32 to, u32 from);

// op = 0, and
// op = 1, or
// op = 2, xor
// op = 3, nor (the 32bit versoins only do OR)
extern void LogicalOpRtoR(x86MMXRegType to, x86MMXRegType from, int op);
extern void LogicalOpMtoR(x86MMXRegType to, u32 from, int op);

extern void LogicalOp32RtoM(uptr to, x86IntRegType from, int op);
extern void LogicalOp32MtoR(x86IntRegType to, uptr from, int op);
extern void LogicalOp32ItoR(x86IntRegType to, u32 from, int op);
extern void LogicalOp32ItoM(uptr to, u32 from, int op);

#ifdef ARITHMETICIMM_RECOMPILE
extern void LogicalOpRtoR(x86MMXRegType to, x86MMXRegType from, int op);
extern void LogicalOpMtoR(x86MMXRegType to, u32 from, int op);
#endif

#endif
