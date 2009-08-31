/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2009  Pcsx2 Team
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

#pragma once

//------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------

#define declareAllVariables											\
initVariable( _somePrefix_,	u32,   mVU_absclip,	0x7fffffff );		\
initVariable( _somePrefix_,	u32,   mVU_signbit,	0x80000000 );		\
initVariable( _somePrefix_,	u32,   mVU_minvals,	0xff7fffff );		\
initVariable( _somePrefix_,	u32,   mVU_maxvals,	0x7f7fffff );		\
initVariable( _somePrefix_, u32,   mVU_one,		0x3f800000 );		\
initVariable( _somePrefix_, u32,   mVU_T1,		0x3f7ffff5 );		\
initVariable( _somePrefix_, u32,   mVU_T2,		0xbeaaa61c );		\
initVariable( _somePrefix_, u32,   mVU_T3,		0x3e4c40a6 );		\
initVariable( _somePrefix_, u32,   mVU_T4,		0xbe0e6c63 );		\
initVariable( _somePrefix_, u32,   mVU_T5,		0x3dc577df );		\
initVariable( _somePrefix_, u32,   mVU_T6,		0xbd6501c4 );		\
initVariable( _somePrefix_, u32,   mVU_T7,		0x3cb31652 );		\
initVariable( _somePrefix_, u32,   mVU_T8,		0xbb84d7e7 );		\
initVariable( _somePrefix_, u32,   mVU_Pi4,		0x3f490fdb );		\
initVariable( _somePrefix_, u32,   mVU_S2,		0xbe2aaaa4 );		\
initVariable( _somePrefix_, u32,   mVU_S3,		0x3c08873e );		\
initVariable( _somePrefix_, u32,   mVU_S4,		0xb94fb21f );		\
initVariable( _somePrefix_, u32,   mVU_S5,		0x362e9c14 );		\
initVariable( _somePrefix_, u32,   mVU_E1,		0x3e7fffa8 );		\
initVariable( _somePrefix_, u32,   mVU_E2,		0x3d0007f4 );		\
initVariable( _somePrefix_, u32,   mVU_E3,		0x3b29d3ff );		\
initVariable( _somePrefix_, u32,   mVU_E4,		0x3933e553 );		\
initVariable( _somePrefix_, u32,   mVU_E5,		0x36b63510 );		\
initVariable( _somePrefix_,	u32,   mVU_E6,		0x353961ac );		\
initVariable( _somePrefix_, float, mVU_FTOI_4,	16.0 );				\
initVariable( _somePrefix_, float, mVU_FTOI_12,	4096.0 );			\
initVariable( _somePrefix_, float, mVU_FTOI_15,	32768.0 );			\
initVariable( _somePrefix_, float, mVU_ITOF_4,	0.0625f );			\
initVariable( _somePrefix_, float, mVU_ITOF_12,	0.000244140625 );	\
initVariable( _somePrefix_, float, mVU_ITOF_15,	0.000030517578125 );

#define _somePrefix_ PCSX2_ALIGNED16_EXTERN
#define initVariable(aprefix, atype, aname, avalue) aprefix (const atype aname [4]);
declareAllVariables
#undef	_somePrefix_
#undef	initVariable

#define _somePrefix_ PCSX2_ALIGNED16
#define initVariable(aprefix, atype, aname, avalue) aprefix (const atype aname [4])	= {avalue, avalue, avalue, avalue};

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

#define _Imm5_	(s16)(((mVU->code & 0x400) ? 0xfff0 : 0) | ((mVU->code >> 6) & 0xf))
#define _Imm11_	(s32)((mVU->code & 0x400) ? (0xfffffc00 | (mVU->code & 0x3ff)) : (mVU->code & 0x3ff))
#define _Imm12_	(((mVU->code >> 21) & 0x1) << 11) | (mVU->code & 0x7ff)
#define _Imm15_	(((mVU->code >> 10) & 0x7800) | (mVU->code & 0x7ff))
#define _Imm24_	(u32)(mVU->code & 0xffffff)

#define _Ibit_ (1<<31)
#define _Ebit_ (1<<30)
#define _Mbit_ (1<<29)
#define _Dbit_ (1<<28)
#define _Tbit_ (1<<27)
#define _DTbit_ 0 //( _Dbit_ | _Tbit_ ) // ToDo: Implement this stuff...

#define divI 0x1040000
#define divD 0x2080000

#define isVU1		(mVU->index != 0)
#define getIndex	(isVU1 ? 1 : 0)
#define getVUmem(x)	(((isVU1) ? (x & 0x3ff) : ((x >= 0x400) ? (x & 0x43f) : (x & 0xff))) * 16)
#define offsetSS	((_X) ? (0) : ((_Y) ? (4) : ((_Z) ? 8: 12)))
#define offsetReg	((_X) ? (0) : ((_Y) ? (1) : ((_Z) ? 2:  3)))

#define xmmT1	0 // Used for regAlloc
#define xmmT2	1 // Used for regAlloc
#define xmmT3	2 // Used for regAlloc
#define xmmT4	3 // Used for regAlloc
#define xmmT5	4 // Used for regAlloc
#define xmmT6	5 // Used for regAlloc
#define xmmT7	6 // Used for regAlloc
#define xmmPQ	7 // Holds the Value and Backup Values of P and Q regs

#define gprT1	0 // Temp Reg
#define gprT2	1 // Temp Reg
#define gprT3	2 // Temp Reg
#define gprF0	3 // Status Flag 0
#define gprESP	4 // Don't use?
#define gprF1	5 // Status Flag 1
#define gprF2	6 // Status Flag 2
#define gprF3	7 // Status Flag 3

// Function Params
#define mP microVU* mVU, int recPass
#define mV microVU* mVU
#define mF int recPass
#define mX mVU, recPass

// Recursive Inline
#ifndef __LINUX__
#define __recInline __releaseinline
#else
#define __recInline inline
#endif

// Function/Template Stuff
#define mVUx (vuIndex ? &microVU1 : &microVU0)
#define mVUop(opName)	void opName (mP)
#define microVUr(aType) __recInline aType
#define microVUt(aType) __forceinline aType
#define microVUx(aType) template<int vuIndex> aType
#define microVUf(aType) template<int vuIndex> __forceinline aType

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

// Define mVUquickSearch
#ifndef __LINUX__
PCSX2_ALIGNED16_EXTERN( u8 mVUsearchXMM[0x1000] );
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

// Misc Macros...
#define mVUprogI	 mVU->prog.prog[progIndex]
#define mVUcurProg	 mVU->prog.prog[mVU->prog.cur]
#define mVUblocks	 mVU->prog.prog[mVU->prog.cur].block
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
#define mVUrange	 mVUcurProg.ranges.range[mVUcurProg.ranges.total]
#define isEvilBlock	 (mVUpBlock->pState.blockType == 2)
#define isBadOrEvil  (mVUlow.badBranch || mVUlow.evilBranch)
#define xPC			 ((iPC / 2) * 8)
#define curI		 ((u32*)mVU->regs->Micro)[iPC] //mVUcurProg.data[iPC]
#define setCode()	 { mVU->code = curI; }
#define incPC(x)	 { iPC = ((iPC + x) & (mVU->progSize-1)); setCode(); }
#define incPC2(x)	 { iPC = ((iPC + x) & (mVU->progSize-1)); }
#define bSaveAddr	 (((xPC + 16) & (mVU->microMemSize-8)) / 8)
#define branchAddr	 ((xPC + 8  + (_Imm11_ * 8)) & (mVU->microMemSize-8))
#define branchAddrN	 ((xPC + 16 + (_Imm11_ * 8)) & (mVU->microMemSize-8))
#define shufflePQ	 (((mVU->p) ? 0xb0 : 0xe0) | ((mVU->q) ? 0x01 : 0x04))
#define cmpOffset(x) ((u8*)&(((u8*)x)[mVUprogI.ranges.range[i][0]]))
#define Rmem		 (uptr)&mVU->regs->VI[REG_R].UL
#define aWrap(x, m)	 ((x > m) ? 0 : x)
#define shuffleSS(x) ((x==1)?(0x27):((x==2)?(0xc6):((x==4)?(0xe1):(0xe4))))
#define _1mb		 (0x100000)

// Flag Info
#define __Status	 (mVUregs.needExactMatch & 1)
#define __Mac		 (mVUregs.needExactMatch & 2)
#define __Clip		 (mVUregs.needExactMatch & 4)

// Pass 3 Helper Macros
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

// Debug Stuff...
#ifdef mVUdebug
#define mVUprint Console::Status
#else
#define mVUprint 0&&
#endif

// Program Logging...
#ifdef mVUlogProg
#define mVUlog		((isVU1) ? __mVULog<1> : __mVULog<0>)
#define mVUdumpProg __mVUdumpProgram<vuIndex>
#else
#define mVUlog 0&&
#define mVUdumpProg 0&&
#endif

// Reg Alloc
#define doRegAlloc 1 // Set to 0 to flush every 64bit Instruction (Turns off regAlloc)

// Speed Hacks
#define CHECK_VU_CONSTHACK	1 // Disables Constant Propagation for Jumps
#define CHECK_VU_FLAGHACK	(u32)Config.Hacks.vuFlagHack	// (Can cause Infinite loops, SPS, etc...)
#define CHECK_VU_MINMAXHACK	(u32)Config.Hacks.vuMinMax		// (Can cause SPS, Black Screens,  etc...)

// Unknown Data
#define mVU_XGKICK_CYCLES ((CHECK_XGKICKHACK) ? 3 : 1)
// Its unknown at recompile time how long the xgkick transfer will take
// so give it a value that makes games happy :) (SO3 is fine at 1 cycle delay)


// Cache Limit Check
#define mVUcacheCheck(ptr, start, limit) {																 \
	uptr diff = ptr - start;																			 \
	if (diff >= limit) {																				 \
		Console::Status("microVU%d: Program cache limit reached. Size = 0x%x", params mVU->index, diff); \
		mVUreset(mVU);																					 \
	}																									 \
}

#define mVUdebugNOW(isEndPC) {							\
	if (mVUdebugNow) {									\
		MOV32ItoR(gprT2, xPC);							\
		if (isEndPC) { CALLFunc((uptr)mVUprintPC2); }	\
		else		 { CALLFunc((uptr)mVUprintPC1); }	\
	}													\
}
