/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2009  Pcsx2-Playground Team
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
#define _Ft_ ((mVU->code >> 16) & 0x1F)  // The ft/it part of the instruction register 
#define _Fs_ ((mVU->code >> 11) & 0x1F)  // The fs/is part of the instruction register 
#define _Fd_ ((mVU->code >>  6) & 0x1F)  // The fd/id part of the instruction register

#define _X	 ((mVU->code>>24) & 0x1)
#define _Y	 ((mVU->code>>23) & 0x1)
#define _Z	 ((mVU->code>>22) & 0x1)
#define _W	 ((mVU->code>>21) & 0x1)

#define _XYZW_SS	(_X+_Y+_Z+_W==1)
#define _X_Y_Z_W	(((mVU->code >> 21 ) & 0xF ))
#define _xyzw_ACC	((_XYZW_SS && !_X) ? 15 : _X_Y_Z_W)

#define _bc_	 (mVU->code & 0x3)
#define _bc_x	((mVU->code & 0x3) == 0)
#define _bc_y	((mVU->code & 0x3) == 1)
#define _bc_z	((mVU->code & 0x3) == 2)
#define _bc_w	((mVU->code & 0x3) == 3)

#define _Fsf_	((mVU->code >> 21) & 0x03)
#define _Ftf_	((mVU->code >> 23) & 0x03)

#define _Imm5_	(((mVU->code & 0x400) ? 0xfff0 : 0) | ((mVU->code >> 6) & 0xf))
#define _Imm11_	(s32)(mVU->code & 0x400 ? 0xfffffc00 | (mVU->code & 0x3ff) : mVU->code & 0x3ff)
#define _Imm12_	(((mVU->code >> 21 ) & 0x1) << 11) | (mVU->code & 0x7ff)
#define _Imm15_	(((mVU->code >> 10) & 0x7800) | (mVU->code & 0x7ff))
#define _Imm24_	(u32)(mVU->code & 0xffffff)

#define _Ibit_ (1<<31)
#define _Ebit_ (1<<30)
#define _Mbit_ (1<<29)
#define _Dbit_ (1<<28)
#define _Tbit_ (1<<27)
#define _MDTbit_ 0 //( _Mbit_ | _Dbit_ | _Tbit_ ) // ToDo: Implement this stuff...

#define getVUmem(x)	(((vuIndex == 1) ? (x & 0x3ff) : ((x >= 0x400) ? (x & 0x43f) : (x & 0xff))) * 16)
#define offsetSS	((_X) ? (0) : ((_Y) ? (4) : ((_Z) ? 8: 12)))

#define xmmT1	0 // Temp Reg
#define xmmFs	1 // Holds the Value of Fs (writes back result Fd)
#define xmmFt	2 // Holds the Value of Ft
#define xmmACC	3 // Holds ACC
#define xmmMax	4 // Holds mVU_maxvals
#define xmmMin	5 // Holds mVU_minvals
#define xmmT2	6 // Temp Reg?
#define xmmPQ	7 // Holds the Value and Backup Values of P and Q regs

#define mmxVI1	0 // Holds VI 1
#define mmxVI2	1 // Holds VI 2
#define mmxVI3	2 // Holds VI 3
#define mmxVI4	3 // Holds VI 4
#define mmxVI5	4 // Holds VI 5
#define mmxVI6	5 // Holds VI 6
#define mmxVI7	6 // Holds VI 7
#define mmxVI8	7 // Holds VI 8

#define gprT1	0 // Temp Reg
#define gprT2	1 // Temp Reg
#define gprR	2 // R Reg
#define gprF0	3 // MAC Flag::Status Flag 0
#define gprESP	4 // Don't use?
#define gprF1	5 // MAC Flag::Status Flag 1
#define gprF2	6 // MAC Flag::Status Flag 2
#define gprF3	7 // MAC Flag::Status Flag 3

// Template Stuff
#define mVUx (vuIndex ? &microVU1 : &microVU0)
#define microVUt(aType) template<int vuIndex> __forceinline aType
#define microVUx(aType) template<int vuIndex> aType
#define microVUf(aType) template<int vuIndex, int recPass> aType
#define microVUq(aType) template<int vuIndex, int recPass>  __forceinline aType

#define mVUcurProg	 mVU->prog.prog[mVU->prog.cur]
#define mVUblock	 mVU->prog.prog[mVU->prog.cur].block
#define mVUallocInfo mVU->prog.prog[mVU->prog.cur].allocInfo
#define mVUbranch	 mVUallocInfo.branch
#define mVUcycles	 mVUallocInfo.cycles
#define mVUcount	 mVUallocInfo.count
//#define mVUstall	 mVUallocInfo.maxStall
#define mVUregs		 mVUallocInfo.regs
#define mVUregsTemp	 mVUallocInfo.regsTemp
#define iPC			 mVUallocInfo.curPC
#define mVUinfo		 mVUallocInfo.info[iPC / 2]
#define mVUstall	 mVUallocInfo.stall[iPC / 2]
#define mVUstartPC	 mVUallocInfo.startPC
#define xPC			 ((iPC / 2) * 8)
#define curI		 mVUcurProg.data[iPC]
#define setCode()	 { mVU->code = curI; }
#define incPC(x)	 { iPC = ((iPC + x) & (mVU->progSize-1)); setCode(); }
#define incPC2(x)	 { iPC = ((iPC + x) & (mVU->progSize-1)); }
#define incCycles(x) { mVUincCycles<vuIndex>(x); }

#define _isNOP		 (1<<0) // Skip Lower Instruction
#define _isBranch	 (1<<1) // Cur Instruction is a Branch
#define _isEOB		 (1<<2) // End of Block
#define _isBdelay	 (1<<3) // Cur Instruction in Branch Delay slot
#define _isSflag	 (1<<4) // Cur Instruction uses status flag
#define _writeQ		 (1<<5)
#define _readQ		 (1<<6)
#define _writeP		 (1<<7)
#define _readP		 (1<<7) // same as writeP
#define _doFlags	 (3<<8)
#define _doMac		 (1<<8)
#define _doStatus	 (1<<9)
#define _fmInstance	 (3<<10)
#define _fsInstance	 (3<<12)
#define _fpsInstance (3<<12)
#define _fcInstance	 (3<<14)
#define _fpcInstance (3<<14)
#define _fvmInstance (3<<16)
#define _fvsInstance (3<<18)
#define _fvcInstance (3<<20)
#define _noWriteVF	 (1<<21) // Don't write back the result of a lower op to VF reg if upper op writes to same reg (or if VF = 0)
#define _backupVI	 (1<<22) // Backup VI reg to memory if modified before branch (branch uses old VI value unless opcode is ILW or ILWR)
#define _memReadIs	 (1<<23) // Read Is (VI reg) from memory (used by branches)
#define _memReadIt	 (1<<24) // Read If (VI reg) from memory (used by branches)
#define _writesVI	 (1<<25) // Current Instruction writes to VI
#define _swapOps	 (1<<26) // Runs Lower Instruction Before Upper Instruction
#define _isFSSSET	 (1<<27) // Cur Instruction is FSSET
#define _doDivFlag	 (1<<28) // Transfer Div flag to Status Flag

//#define _isBranch2	 (1<<31) // Cur Instruction is a Branch that writes VI regs (BAL/JALR)

#define isNOP		 (mVUinfo & (1<<0))
#define isBranch	 (mVUinfo & (1<<1))
#define isEOB		 (mVUinfo & (1<<2))
#define isBdelay	 (mVUinfo & (1<<3))
#define isSflag		 (mVUinfo & (1<<4))
#define writeQ		((mVUinfo >> 5) & 1)
#define readQ		((mVUinfo >> 6) & 1)
#define writeP		((mVUinfo >> 7) & 1)
#define readP		((mVUinfo >> 7) & 1) // same as writeP
#define doFlags		 (mVUinfo & (3<<8))
#define doMac		 (mVUinfo & (1<<8))
#define doStatus	 (mVUinfo & (1<<9))
#define fmInstance	((mVUinfo >> 10) & 3)
#define fsInstance	((mVUinfo >> 12) & 3)
#define fpsInstance	((((mVUinfo>>12) & 3) - 1) & 0x3)
#define fcInstance	((mVUinfo >> 14) & 3)
#define fpcInstance	((((mVUinfo>>14) & 3) - 1) & 0x3)
#define fvmInstance	((mVUinfo >> 16) & 3)
#define fvsInstance	((mVUinfo >> 18) & 3)
#define fvcInstance	((mVUinfo >> 20) & 3)
#define noWriteVF	 (mVUinfo & (1<<21))
#define backupVI	 (mVUinfo & (1<<22))
#define memReadIs	 (mVUinfo & (1<<23))
#define memReadIt	 (mVUinfo & (1<<24))
#define writesVI	 (mVUinfo & (1<<25))
#define swapOps		 (mVUinfo & (1<<26))
#define isFSSET		 (mVUinfo & (1<<27))
#define doDivFlag	 (mVUinfo & (1<<28))
//#define isBranch2	 (mVUinfo & (1<<31))

#define isMMX(_VIreg_)	(_VIreg_ >= 1 && _VIreg_ <=9)
#define mmVI(_VIreg_)	(_VIreg_ - 1)

#ifdef mVUdebug
#define mVUlog Console::Notice
#define mVUdebug1() {											\
	if (curI & _Ibit_)	{ SysPrintf("microVU: I-bit set!\n"); }	\
	if (curI & _Ebit_)	{ SysPrintf("microVU: E-bit set!\n"); }	\
	if (curI & _Mbit_)	{ SysPrintf("microVU: M-bit set!\n"); }	\
	if (curI & _Dbit_)	{ SysPrintf("microVU: D-bit set!\n"); }	\
	if (curI & _Tbit_)	{ SysPrintf("microVU: T-bit set!\n"); }	\
}
#else
#define mVUlog 0&&
#define mVUdebug1() {}
#endif

#define mVUcachCheck(start, limit) {  \
	uptr diff = mVU->ptr - start; \
	if (diff >= limit) { Console::Error("microVU Error: Program went over it's cache limit. Size = %x", params diff); } \
}
