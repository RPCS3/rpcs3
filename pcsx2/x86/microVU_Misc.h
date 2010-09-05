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

using namespace x86Emitter;

typedef xRegisterSSE xmm;
typedef xRegister32 x32;

struct microVU;

//------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------

struct mVU_Globals {
	u32		absclip[4], signbit[4], minvals[4], maxvals[4];
	u32		one[4];
	u32		Pi4[4];
	u32		T1[4], T2[4], T3[4], T4[4], T5[4], T6[4], T7[4], T8[4];
	u32		S2[4], S3[4], S4[4], S5[4];
	u32		E1[4], E2[4], E3[4], E4[4], E5[4], E6[4];
	float	FTOI_4[4], FTOI_12[4], FTOI_15[4];
	float	ITOF_4[4], ITOF_12[4], ITOF_15[4];
};

extern const __aligned(32) mVU_Globals mVUglob;

static const uint _Ibit_ = 1 << 31;
static const uint _Ebit_ = 1 << 30;
static const uint _Mbit_ = 1 << 29;
static const uint _Dbit_ = 1 << 28;
static const uint _Tbit_ = 1 << 27;
static const uint _DTbit_ = 0; //( _Dbit_ | _Tbit_ ) // ToDo: Implement this stuff...

static const uint divI = 0x1040000;
static const uint divD = 0x2080000;

//------------------------------------------------------------------
// Helper Macros
//------------------------------------------------------------------

#define _Ft_ ((mVU->code >> 16) & 0x1F)  // The ft part of the instruction register
#define _Fs_ ((mVU->code >> 11) & 0x1F)  // The fs part of the instruction register
#define _Fd_ ((mVU->code >>  6) & 0x1F)  // The fd part of the instruction register

#define _It_ ((mVU->code >> 16) & 0xF)   // The it part of the instruction register
#define _Is_ ((mVU->code >> 11) & 0xF)   // The is part of the instruction register
#define _Id_ ((mVU->code >>  6) & 0xF)   // The id part of the instruction register

#define _X	 ((mVU->code>>24) & 0x1)
#define _Y	 ((mVU->code>>23) & 0x1)
#define _Z	 ((mVU->code>>22) & 0x1)
#define _W	 ((mVU->code>>21) & 0x1)

#define _X_Y_Z_W	(((mVU->code >> 21 ) & 0xF))
#define _XYZW_SS	(_X+_Y+_Z+_W==1)
#define _XYZW_SS2	(_XYZW_SS && (_X_Y_Z_W != 8))
#define _XYZW_PS	(_X_Y_Z_W == 0xf)
#define _XYZWss(x)	((x==8) || (x==4) || (x==2) || (x==1))

#define _bc_	 (mVU->code & 0x3)
#define _bc_x	((mVU->code & 0x3) == 0)
#define _bc_y	((mVU->code & 0x3) == 1)
#define _bc_z	((mVU->code & 0x3) == 2)
#define _bc_w	((mVU->code & 0x3) == 3)

#define _Fsf_	((mVU->code >> 21) & 0x03)
#define _Ftf_	((mVU->code >> 23) & 0x03)

#define _Imm5_	(mVU->Imm5())
#define _Imm11_	(mVU->Imm11())
#define _Imm12_	(mVU->Imm12())
#define _Imm15_	(mVU->Imm15())
#define _Imm24_	(mVU->Imm24())

#define isCOP2		(mVU->cop2  != 0)
#define isVU1		(mVU->index != 0)
#define isVU0		(mVU->index == 0)
#define getIndex	(isVU1 ? 1 : 0)
#define getVUmem(x)	(((isVU1) ? (x & 0x3ff) : ((x >= 0x400) ? (x & 0x43f) : (x & 0xff))) * 16)
#define offsetSS	((_X) ? (0) : ((_Y) ? (4) : ((_Z) ? 8: 12)))
#define offsetReg	((_X) ? (0) : ((_Y) ? (1) : ((_Z) ? 2:  3)))

#define xmmT1  xmm0 // Used for regAlloc
#define xmmT2  xmm1 // Used for regAlloc
#define xmmT3  xmm2 // Used for regAlloc
#define xmmT4  xmm3 // Used for regAlloc
#define xmmT5  xmm4 // Used for regAlloc
#define xmmT6  xmm5 // Used for regAlloc
#define xmmT7  xmm6 // Used for regAlloc
#define xmmPQ  xmm7 // Holds the Value and Backup Values of P and Q regs

#define gprT1  eax // eax - Temp Reg
#define gprT2  ecx // ecx - Temp Reg
#define gprT3  edx // edx - Temp Reg
#define gprT1b ax  // Low 16-bit of gprT1 (eax)
#define gprT2b cx  // Low 16-bit of gprT2 (ecx)
#define gprT3b dx  // Low 16-bit of gprT3 (edx)

#define gprF0  ebx // Status Flag 0
#define gprF1  ebp // Status Flag 1
#define gprF2  esi // Status Flag 2
#define gprF3  edi // Status Flag 3

// Function Params
#define mP microVU* mVU, int recPass
#define mV microVU* mVU
#define mF int recPass
#define mX mVU, recPass

typedef void __fastcall Fntype_mVUrecInst( microVU* mVU, int recPass );
typedef Fntype_mVUrecInst* Fnptr_mVUrecInst;

// Recursive Inline
#ifndef __LINUX__
#define __recInline __ri
#else
#define __recInline inline
#endif

// Function/Template Stuff
#define  mVUx (vuIndex ? &microVU1 : &microVU0)
#define  mVUop(opName)	static void __fastcall opName (mP)
#define _mVUt template<int vuIndex>
#define _r	  static __recInline

// Define Passes
#define pass1 if (recPass == 0)
#define pass2 if (recPass == 1)
#define pass3 if (recPass == 2)
#define pass4 if (recPass == 3)

// Upper Opcode Cases
#define opCase1 if (opCase == 1) // Normal Opcodes
#define opCase2 if (opCase == 2) // BC Opcodes
#define opCase3 if (opCase == 3) // I  Opcodes
#define opCase4 if (opCase == 4) // Q  Opcodes

//------------------------------------------------------------------
// Define mVUquickSearch
//------------------------------------------------------------------
#ifndef __LINUX__
extern __pagealigned u8 mVUsearchXMM[__pagesize];
typedef u32 (__fastcall *mVUCall)(void*, void*);
#define mVUquickSearch(dest, src, size) ((((mVUCall)((void*)mVUsearchXMM))(dest, src)) == 0xf)
#define mVUemitSearch() { mVUcustomSearch(); }
#else
// Note: GCC builds crash with custom search function, because
// they're not guaranteeing 16-byte alignment on the structs :(
// #define mVUquickSearch(dest, src, size) (!memcmp(dest, src, size))
#define mVUquickSearch(dest, src, size) (!memcmp_mmx(dest, src, size))
#define mVUemitSearch()
#endif
//------------------------------------------------------------------

// Misc Macros...
#define __four(val)	{ val, val, val, val }
#define mVUcurProg   mVU->prog.cur[0]
#define mVUblocks	 mVU->prog.cur->block
#define mVUir		 mVU->prog.IRinfo
#define mVUbranch	 mVU->prog.IRinfo.branch
#define mVUcycles	 mVU->prog.IRinfo.cycles
#define mVUcount	 mVU->prog.IRinfo.count
#define mVUpBlock	 mVU->prog.IRinfo.pBlock
#define mVUblock	 mVU->prog.IRinfo.block
#define mVUregs		 mVU->prog.IRinfo.block.pState
#define mVUregsTemp	 mVU->prog.IRinfo.regsTemp
#define iPC			 mVU->prog.IRinfo.curPC
#define mVUsFlagHack mVU->prog.IRinfo.sFlagHack
#define mVUconstReg	 mVU->prog.IRinfo.constReg
#define mVUstartPC	 mVU->prog.IRinfo.startPC
#define mVUinfo		 mVU->prog.IRinfo.info[iPC / 2]
#define mVUstall	 mVUinfo.stall
#define mVUup		 mVUinfo.uOp
#define mVUlow		 mVUinfo.lOp
#define sFLAG		 mVUinfo.sFlag
#define mFLAG		 mVUinfo.mFlag
#define cFLAG		 mVUinfo.cFlag
#define mVUrange     (mVUcurProg.ranges[0])[0]
#define isEvilBlock	 (mVUpBlock->pState.blockType == 2)
#define isBadOrEvil  (mVUlow.badBranch || mVUlow.evilBranch)
#define xPC			 ((iPC / 2) * 8)
#define curI		 ((u32*)mVU->regs().Micro)[iPC] //mVUcurProg.data[iPC]
#define setCode()	 { mVU->code = curI; }

#define incPC(x)	 (mVU->advancePC(x))
#define branchAddr	 mVU->getBranchAddr()
#define branchAddrN	 mVU->getBranchAddrN()

#define incPC2(x)	 { iPC = ((iPC + (x)) & mVU->progMemMask); }
#define bSaveAddr	 (((xPC + 16) & (mVU->microMemSize-8)) / 8)
#define shufflePQ	 (((mVU->p) ? 0xb0 : 0xe0) | ((mVU->q) ? 0x01 : 0x04))
#define cmpOffset(x) ((u8*)&(((u8*)x)[it[0].start]))
#define Rmem		 &mVU->regs().VI[REG_R].UL
#define aWrap(x, m)	 ((x > m) ? 0 : x)
#define shuffleSS(x) ((x==1)?(0x27):((x==2)?(0xc6):((x==4)?(0xe1):(0xe4))))
#define clampE       CHECK_VU_EXTRA_OVERFLOW
#define elif		 else if

// Flag Info (Set if next-block's first 4 ops will read current-block's flags)
#define __Status	 (mVUregs.needExactMatch & 1)
#define __Mac		 (mVUregs.needExactMatch & 2)
#define __Clip		 (mVUregs.needExactMatch & 4)

// Pass 3 Helper Macros (Used for program logging)
#define _Fsf_String	 ((_Fsf_ == 3) ? "w" : ((_Fsf_ == 2) ? "z" : ((_Fsf_ == 1) ? "y" : "x")))
#define _Ftf_String	 ((_Ftf_ == 3) ? "w" : ((_Ftf_ == 2) ? "z" : ((_Ftf_ == 1) ? "y" : "x")))
#define xyzwStr(x,s) (_X_Y_Z_W == x) ? s :
#define _XYZW_String (xyzwStr(1, "w") (xyzwStr(2, "z") (xyzwStr(3, "zw") (xyzwStr(4, "y") (xyzwStr(5, "yw") (xyzwStr(6, "yz") (xyzwStr(7, "yzw") (xyzwStr(8, "x") (xyzwStr(9, "xw") (xyzwStr(10, "xz") (xyzwStr(11, "xzw") (xyzwStr(12, "xy") (xyzwStr(13, "xyw") (xyzwStr(14, "xyz") "xyzw"))))))))))))))
#define _BC_String	 (_bc_x ? "x" : (_bc_y ? "y" : (_bc_z ? "z" : "w")))
#define mVUlogFtFs() { mVUlog(".%s vf%02d, vf%02d", _XYZW_String, _Ft_, _Fs_); }
#define mVUlogFd()	 { mVUlog(".%s vf%02d, vf%02d", _XYZW_String, _Fd_, _Fs_); }
#define mVUlogACC()	 { mVUlog(".%s ACC, vf%02d", _XYZW_String, _Fs_); }
#define mVUlogFt()	 { mVUlog(", vf%02d", _Ft_); }
#define mVUlogBC()	 { mVUlog(", vf%02d%s", _Ft_, _BC_String); }
#define mVUlogI()	 { mVUlog(", I"); }
#define mVUlogQ()	 { mVUlog(", Q"); }
#define mVUlogCLIP() { mVUlog("w.xyz vf%02d, vf%02dw", _Fs_, _Ft_); }

// Program Logging...
#ifdef mVUlogProg
#define mVUlog		((isVU1) ? __mVULog<1> : __mVULog<0>)
#define mVUdumpProg __mVUdumpProgram<vuIndex>
#else
#define mVUlog(...)		 if (0) {}
#define mVUdumpProg(...) if (0) {}
#endif

//------------------------------------------------------------------
// Optimization Options
//------------------------------------------------------------------

// Reg Alloc
static const bool doRegAlloc = 1; // Set to 0 to flush every 32bit Instruction
// This turns off reg alloc for the most part, but reg alloc will still
// be done within instructions... Also on doSwapOp() regAlloc is needed between
// Lower and Upper instructions, so in this case it flushes after the full
// 64bit instruction (lower and upper)

// No Flag Optimizations
static const bool noFlagOpts = 0; // Set to 1 to disable all flag setting optimizations
// Note: The flag optimizations this disables should all be harmless, so
// this option is mainly just for debugging... it effectively forces mVU
// to always update Mac and Status Flags (both sticky and non-sticky) whenever
// an Upper Instruction updates them. It also always transfers the 4 possible
// flag instances between blocks...

// Constant Propagation
static const bool doConstProp = 0; // Set to 1 to turn on vi15 const propagation
// Enables Constant Propagation for Jumps based on vi15 'link-register'
// allowing us to know many indirect jump target addresses.
// Makes GoW a lot slower due to extra recompilation time and extra code-gen!

//------------------------------------------------------------------
// Speed Hacks (can cause infinite loops, SPS, Black Screens, etc...)
//------------------------------------------------------------------

// Status Flag Speed Hack
#define CHECK_VU_FLAGHACK	(EmuConfig.Speedhacks.vuFlagHack)
// This hack only updates the Status Flag on blocks that will read it.
// Most blocks do not read status flags, so this is a big speedup.

// Block Flag Instance No-Propagation Hack
#define CHECK_VU_BLOCKHACK (EmuConfig.Speedhacks.vuBlockHack)
// There are times when it is unknown if future blocks will need old
// flag instance data (due to indirect jumps). This hack assumes
// that they won't need old flag data. This effectively removes a lot
// of end-of-block flag instance shuffling, causing nice speedups.

// Min/Max Speed Hack
#define CHECK_VU_MINMAXHACK	(EmuConfig.Speedhacks.vuMinMax)
// This hack uses SSE min/max instructions instead of emulated "logical min/max"
// The PS2 does not consider denormals as zero on the mini/max opcodes.
// This speedup is minor, but on AMD X2 CPUs it can be a 1~3% speedup

//------------------------------------------------------------------
// Unknown Data
//------------------------------------------------------------------

// XG Kick Transfer Delay Amount
#define mVU_XGKICK_CYCLES ((CHECK_XGKICKHACK) ? 3 : 1)
// Its unknown at recompile time how long the xgkick transfer will take
// so give it a value that makes games happy :) (SO3 is fine at 1 cycle delay)

//------------------------------------------------------------------

// Cache Limit Check
#define mVUcacheCheck(ptr, start, limit) {														  \
	uptr diff = ptr - start;																	  \
	if (diff >= limit) {																		  \
		DevCon.WriteLn("microVU%d: Program cache limit reached. Size = 0x%x", mVU->index, diff); \
		mVUresizeCache(mVU, mVU->cacheSize + mVUcacheGrowBy);									  \
	}																							  \
}

extern void mVUmergeRegs(const xmm& dest, const xmm& src,  int xyzw, bool modXYZW=false);
extern void mVUsaveReg(const xmm& reg, xAddressVoid ptr, int xyzw, bool modXYZW);
extern void mVUloadReg(const xmm& reg, xAddressVoid ptr, int xyzw);
