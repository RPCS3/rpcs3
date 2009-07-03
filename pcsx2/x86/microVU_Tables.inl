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
// Declarations
//------------------------------------------------------------------
#define mVUgetCode (mVU->code)

mVUop(mVU_UPPER_FD_00);
mVUop(mVU_UPPER_FD_01);
mVUop(mVU_UPPER_FD_10);
mVUop(mVU_UPPER_FD_11);
mVUop(mVULowerOP);
mVUop(mVULowerOP_T3_00);
mVUop(mVULowerOP_T3_01);
mVUop(mVULowerOP_T3_10);
mVUop(mVULowerOP_T3_11);
mVUop(mVUunknown);
//------------------------------------------------------------------

//------------------------------------------------------------------
// Opcode Tables
//------------------------------------------------------------------
#define microVU_LOWER_OPCODE(x) void (*mVULOWER_OPCODE##x [128])(mP) = {				\
	mVU_LQ		, mVU_SQ		, mVUunknown	, mVUunknown,							\
	mVU_ILW		, mVU_ISW		, mVUunknown	, mVUunknown,							\
	mVU_IADDIU	, mVU_ISUBIU	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVU_FCEQ	, mVU_FCSET		, mVU_FCAND		, mVU_FCOR,		/* 0x10 */				\
	mVU_FSEQ	, mVU_FSSET		, mVU_FSAND		, mVU_FSOR,								\
	mVU_FMEQ	, mVUunknown	, mVU_FMAND		, mVU_FMOR,								\
	mVU_FCGET	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVU_B		, mVU_BAL		, mVUunknown	, mVUunknown,	/* 0x20 */				\
	mVU_JR		, mVU_JALR		, mVUunknown	, mVUunknown,							\
	mVU_IBEQ	, mVU_IBNE		, mVUunknown	, mVUunknown,							\
	mVU_IBLTZ	, mVU_IBGTZ		, mVU_IBLEZ		, mVU_IBGEZ,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,	/* 0x30 */				\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVULowerOP	, mVUunknown	, mVUunknown	, mVUunknown,	/* 0x40*/				\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,	/* 0x50 */				\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,	/* 0x60 */				\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,	/* 0x70 */				\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
};

#define microVU_LowerOP_T3_00_OPCODE(x) void (*mVULowerOP_T3_00_OPCODE##x [32])(mP) = {	\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVU_MOVE	, mVU_LQI		, mVU_DIV		, mVU_MTIR,								\
	mVU_RNEXT	, mVUunknown	, mVUunknown	, mVUunknown, /* 0x10 */				\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVU_MFP		, mVU_XTOP		, mVU_XGKICK,							\
	mVU_ESADD	, mVU_EATANxy	, mVU_ESQRT		, mVU_ESIN,								\
};

#define microVU_LowerOP_T3_01_OPCODE(x) void (*mVULowerOP_T3_01_OPCODE##x [32])(mP) = {	\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVU_MR32	, mVU_SQI		, mVU_SQRT		, mVU_MFIR,								\
	mVU_RGET	, mVUunknown	, mVUunknown	, mVUunknown, /* 0x10 */				\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVU_XITOP		, mVUunknown,							\
	mVU_ERSADD	, mVU_EATANxz	, mVU_ERSQRT	, mVU_EATAN,							\
};

#define microVU_LowerOP_T3_10_OPCODE(x) void (*mVULowerOP_T3_10_OPCODE##x [32])(mP) = {	\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVU_LQD		, mVU_RSQRT		, mVU_ILWR,								\
	mVU_RINIT	, mVUunknown	, mVUunknown	, mVUunknown, /* 0x10 */				\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVU_ELENG	, mVU_ESUM		, mVU_ERCPR		, mVU_EEXP,								\
};

#define microVU_LowerOP_T3_11_OPCODE(x) void (*mVULowerOP_T3_11_OPCODE##x [32])(mP) = {	\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVU_SQD		, mVU_WAITQ		, mVU_ISWR,								\
	mVU_RXOR	, mVUunknown	, mVUunknown	, mVUunknown, /* 0x10 */				\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVU_ERLENG	, mVUunknown	, mVU_WAITP		, mVUunknown,							\
};

#define microVU_LowerOP_OPCODE(x) void (*mVULowerOP_OPCODE##x [64])(mP) = {				\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown, /* 0x10 */				\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown, /* 0x20 */				\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVU_IADD	, mVU_ISUB		, mVU_IADDI		, mVUunknown, /* 0x30 */				\
	mVU_IAND	, mVU_IOR		, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVULowerOP_T3_00, mVULowerOP_T3_01, mVULowerOP_T3_10, mVULowerOP_T3_11,				\
};

#define microVU_UPPER_OPCODE(x) void (*mVU_UPPER_OPCODE##x [64])(mP) = {				\
	mVU_ADDx	, mVU_ADDy		, mVU_ADDz		, mVU_ADDw,								\
	mVU_SUBx	, mVU_SUBy		, mVU_SUBz		, mVU_SUBw,								\
	mVU_MADDx	, mVU_MADDy		, mVU_MADDz		, mVU_MADDw,							\
	mVU_MSUBx	, mVU_MSUBy		, mVU_MSUBz		, mVU_MSUBw,							\
	mVU_MAXx	, mVU_MAXy		, mVU_MAXz		, mVU_MAXw,		/* 0x10 */				\
	mVU_MINIx	, mVU_MINIy		, mVU_MINIz		, mVU_MINIw,							\
	mVU_MULx	, mVU_MULy		, mVU_MULz		, mVU_MULw,								\
	mVU_MULq	, mVU_MAXi		, mVU_MULi		, mVU_MINIi,							\
	mVU_ADDq	, mVU_MADDq		, mVU_ADDi		, mVU_MADDi,	/* 0x20 */				\
	mVU_SUBq	, mVU_MSUBq		, mVU_SUBi		, mVU_MSUBi,							\
	mVU_ADD		, mVU_MADD		, mVU_MUL		, mVU_MAX,								\
	mVU_SUB		, mVU_MSUB		, mVU_OPMSUB	, mVU_MINI,								\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,	/* 0x30 */				\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVU_UPPER_FD_00, mVU_UPPER_FD_01, mVU_UPPER_FD_10, mVU_UPPER_FD_11,					\
};

#define microVU_UPPER_FD_00_TABLE(x) void (*mVU_UPPER_FD_00_TABLE##x [32])(mP) = {		\
	mVU_ADDAx	, mVU_SUBAx		, mVU_MADDAx	, mVU_MSUBAx,							\
	mVU_ITOF0	, mVU_FTOI0		, mVU_MULAx		, mVU_MULAq,							\
	mVU_ADDAq	, mVU_SUBAq		, mVU_ADDA		, mVU_SUBA,								\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
};

#define microVU_UPPER_FD_01_TABLE(x) void (* mVU_UPPER_FD_01_TABLE##x [32])(mP) = {		\
	mVU_ADDAy	, mVU_SUBAy		, mVU_MADDAy	, mVU_MSUBAy,							\
	mVU_ITOF4	, mVU_FTOI4		, mVU_MULAy		, mVU_ABS,								\
	mVU_MADDAq	, mVU_MSUBAq	, mVU_MADDA		, mVU_MSUBA,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
};

#define microVU_UPPER_FD_10_TABLE(x) void (* mVU_UPPER_FD_10_TABLE##x [32])(mP) = {		\
	mVU_ADDAz	, mVU_SUBAz		, mVU_MADDAz	, mVU_MSUBAz,							\
	mVU_ITOF12	, mVU_FTOI12	, mVU_MULAz		, mVU_MULAi,							\
	mVU_ADDAi	, mVU_SUBAi		, mVU_MULA		, mVU_OPMULA,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
};

#define microVU_UPPER_FD_11_TABLE(x) void (* mVU_UPPER_FD_11_TABLE##x [32])(mP) = {		\
	mVU_ADDAw	, mVU_SUBAw		, mVU_MADDAw	, mVU_MSUBAw,							\
	mVU_ITOF15	, mVU_FTOI15	, mVU_MULAw		, mVU_CLIP,								\
	mVU_MADDAi	, mVU_MSUBAi	, mVUunknown	, mVU_NOP,								\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,							\
};

//------------------------------------------------------------------

//------------------------------------------------------------------
// Create Table Instances
//------------------------------------------------------------------
#define mVUcreateTable(x)		\
microVU_LOWER_OPCODE(x)			\
microVU_LowerOP_T3_00_OPCODE(x)	\
microVU_LowerOP_T3_01_OPCODE(x)	\
microVU_LowerOP_T3_10_OPCODE(x)	\
microVU_LowerOP_T3_11_OPCODE(x)	\
microVU_LowerOP_OPCODE(x)		\
microVU_UPPER_OPCODE(x)			\
microVU_UPPER_FD_00_TABLE(x)	\
microVU_UPPER_FD_01_TABLE(x)	\
microVU_UPPER_FD_10_TABLE(x)	\
microVU_UPPER_FD_11_TABLE(x)

mVUcreateTable(0)

//------------------------------------------------------------------
// Table Functions
//------------------------------------------------------------------
#define doTableStuff(tableName, args) {	\
	tableName##0[ args ](mX);			\
}

mVUop(mVU_UPPER_FD_00)	{ doTableStuff(mVU_UPPER_FD_00_TABLE,	((mVUgetCode >> 6) & 0x1f)); } 
mVUop(mVU_UPPER_FD_01)	{ doTableStuff(mVU_UPPER_FD_01_TABLE,	((mVUgetCode >> 6) & 0x1f)); } 
mVUop(mVU_UPPER_FD_10)	{ doTableStuff(mVU_UPPER_FD_10_TABLE,	((mVUgetCode >> 6) & 0x1f)); } 
mVUop(mVU_UPPER_FD_11)	{ doTableStuff(mVU_UPPER_FD_11_TABLE,	((mVUgetCode >> 6) & 0x1f)); } 
mVUop(mVULowerOP)		{ doTableStuff(mVULowerOP_OPCODE,		 (mVUgetCode & 0x3f)); } 
mVUop(mVULowerOP_T3_00)	{ doTableStuff(mVULowerOP_T3_00_OPCODE,	((mVUgetCode >> 6) & 0x1f)); } 
mVUop(mVULowerOP_T3_01)	{ doTableStuff(mVULowerOP_T3_01_OPCODE,	((mVUgetCode >> 6) & 0x1f)); } 
mVUop(mVULowerOP_T3_10)	{ doTableStuff(mVULowerOP_T3_10_OPCODE,	((mVUgetCode >> 6) & 0x1f)); } 
mVUop(mVULowerOP_T3_11)	{ doTableStuff(mVULowerOP_T3_11_OPCODE,	((mVUgetCode >> 6) & 0x1f)); }
mVUop(mVUopU)			{ doTableStuff(mVU_UPPER_OPCODE,		 (mVUgetCode & 0x3f)); } // Gets Upper Opcode
mVUop(mVUopL)			{ doTableStuff(mVULOWER_OPCODE,			 (mVUgetCode >>  25)); } // Gets Lower Opcode
mVUop(mVUunknown) { 
	pass2 { Console::Error("microVU%d: Unknown Micro VU opcode called (%08x)\n", params getIndex, mVUgetCode); }
	pass3 { mVUlog("Unknown", mVUgetCode); }
}

