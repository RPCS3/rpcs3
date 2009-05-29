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
#define mVUgetCode (vuIndex ? microVU1.code : microVU0.code)

microVUf(void) mVU_UPPER_FD_00(mF);
microVUf(void) mVU_UPPER_FD_01(mF);
microVUf(void) mVU_UPPER_FD_10(mF);
microVUf(void) mVU_UPPER_FD_11(mF);
microVUf(void) mVULowerOP(mF);
microVUf(void) mVULowerOP_T3_00(mF);
microVUf(void) mVULowerOP_T3_01(mF);
microVUf(void) mVULowerOP_T3_10(mF);
microVUf(void) mVULowerOP_T3_11(mF);
microVUf(void) mVUunknown(mF);
//------------------------------------------------------------------

//------------------------------------------------------------------
// Opcode Tables
//------------------------------------------------------------------
#define microVU_LOWER_OPCODE(x) void (*mVULOWER_OPCODE##x [128])(mF) = {					\
	mVU_LQ<x>		, mVU_SQ<x>		, mVUunknown<x>	, mVUunknown<x>,						\
	mVU_ILW<x>		, mVU_ISW<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVU_IADDIU<x>	, mVU_ISUBIU<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVU_FCEQ<x>		, mVU_FCSET<x>	, mVU_FCAND<x>	, mVU_FCOR<x>,		/* 0x10 */			\
	mVU_FSEQ<x>		, mVU_FSSET<x>	, mVU_FSAND<x>	, mVU_FSOR<x>,							\
	mVU_FMEQ<x>		, mVUunknown<x>	, mVU_FMAND<x>	, mVU_FMOR<x>,							\
	mVU_FCGET<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVU_B<x>		, mVU_BAL<x>	, mVUunknown<x>	, mVUunknown<x>,	/* 0x20 */			\
	mVU_JR<x>		, mVU_JALR<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVU_IBEQ<x>		, mVU_IBNE<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVU_IBLTZ<x>	, mVU_IBGTZ<x>	, mVU_IBLEZ<x>	, mVU_IBGEZ<x>,							\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,	/* 0x30 */			\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVULowerOP<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,	/* 0x40*/			\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,	/* 0x50 */			\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,	/* 0x60 */			\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,	/* 0x70 */			\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
};

#define microVU_LowerOP_T3_00_OPCODE(x) void (*mVULowerOP_T3_00_OPCODE##x [32])(mF) = {		\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVU_MOVE<x>		, mVU_LQI<x>	, mVU_DIV<x>	, mVU_MTIR<x>,							\
	mVU_RNEXT<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>, /* 0x10 */				\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVU_MFP<x>	, mVU_XTOP<x>	, mVU_XGKICK<x>,						\
	mVU_ESADD<x>	, mVU_EATANxy<x>, mVU_ESQRT<x>	, mVU_ESIN<x>,							\
};

#define microVU_LowerOP_T3_01_OPCODE(x) void (*mVULowerOP_T3_01_OPCODE##x [32])(mF) = {		\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVU_MR32<x>		, mVU_SQI<x>	, mVU_SQRT<x>	, mVU_MFIR<x>,							\
	mVU_RGET<x>		, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>, /* 0x10 */				\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVU_XITOP<x>	, mVUunknown<x>,						\
	mVU_ERSADD<x>	, mVU_EATANxz<x>, mVU_ERSQRT<x>	, mVU_EATAN<x>,							\
};

#define microVU_LowerOP_T3_10_OPCODE(x) void (*mVULowerOP_T3_10_OPCODE##x [32])(mF) = {		\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVU_LQD<x>	, mVU_RSQRT<x>	, mVU_ILWR<x>,							\
	mVU_RINIT<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>, /* 0x10 */				\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVU_ELENG<x>	, mVU_ESUM<x>	, mVU_ERCPR<x>	, mVU_EEXP<x>,							\
};

#define microVU_LowerOP_T3_11_OPCODE(x) void (*mVULowerOP_T3_11_OPCODE##x [32])(mF) = {		\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVU_SQD<x>	, mVU_WAITQ<x>	, mVU_ISWR<x>,							\
	mVU_RXOR<x>		, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>, /* 0x10 */				\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVU_ERLENG<x>	, mVUunknown<x>	, mVU_WAITP<x>	, mVUunknown<x>,						\
};

#define microVU_LowerOP_OPCODE(x) void (*mVULowerOP_OPCODE##x [64])(mF) = {					\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>, /* 0x10 */				\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>, /* 0x20 */				\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVU_IADD<x>		, mVU_ISUB<x>	, mVU_IADDI<x>	, mVUunknown<x>, /* 0x30 */				\
	mVU_IAND<x>		, mVU_IOR<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVULowerOP_T3_00<x>, mVULowerOP_T3_01<x>, mVULowerOP_T3_10<x>, mVULowerOP_T3_11<x>,		\
};

#define microVU_UPPER_OPCODE(x) void (*mVU_UPPER_OPCODE##x [64])(mF) = {					\
	mVU_ADDx<x>		, mVU_ADDy<x>	, mVU_ADDz<x>	, mVU_ADDw<x>,							\
	mVU_SUBx<x>		, mVU_SUBy<x>	, mVU_SUBz<x>	, mVU_SUBw<x>,							\
	mVU_MADDx<x>	, mVU_MADDy<x>	, mVU_MADDz<x>	, mVU_MADDw<x>,							\
	mVU_MSUBx<x>	, mVU_MSUBy<x>	, mVU_MSUBz<x>	, mVU_MSUBw<x>,							\
	mVU_MAXx<x>		, mVU_MAXy<x>	, mVU_MAXz<x>	, mVU_MAXw<x>,	/* 0x10 */				\
	mVU_MINIx<x>	, mVU_MINIy<x>	, mVU_MINIz<x>	, mVU_MINIw<x>,							\
	mVU_MULx<x>		, mVU_MULy<x>	, mVU_MULz<x>	, mVU_MULw<x>,							\
	mVU_MULq<x>		, mVU_MAXi<x>	, mVU_MULi<x>	, mVU_MINIi<x>,							\
	mVU_ADDq<x>		, mVU_MADDq<x>	, mVU_ADDi<x>	, mVU_MADDi<x>,	/* 0x20 */				\
	mVU_SUBq<x>		, mVU_MSUBq<x>	, mVU_SUBi<x>	, mVU_MSUBi<x>,							\
	mVU_ADD<x>		, mVU_MADD<x>	, mVU_MUL<x>	, mVU_MAX<x>,							\
	mVU_SUB<x>		, mVU_MSUB<x>	, mVU_OPMSUB<x>	, mVU_MINI<x>,							\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,	/* 0x30 */			\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVU_UPPER_FD_00<x>, mVU_UPPER_FD_01<x>, mVU_UPPER_FD_10<x>, mVU_UPPER_FD_11<x>,			\
};

#define microVU_UPPER_FD_00_TABLE(x) void (*mVU_UPPER_FD_00_TABLE##x [32])(mF) = {			\
	mVU_ADDAx<x>	, mVU_SUBAx<x>	, mVU_MADDAx<x>	, mVU_MSUBAx<x>,						\
	mVU_ITOF0<x>	, mVU_FTOI0<x>	, mVU_MULAx<x>	, mVU_MULAq<x>,							\
	mVU_ADDAq<x>	, mVU_SUBAq<x>	, mVU_ADDA<x>		, mVU_SUBA<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
};

#define microVU_UPPER_FD_01_TABLE(x) void (* mVU_UPPER_FD_01_TABLE##x [32])(mF) = {			\
	mVU_ADDAy<x>	, mVU_SUBAy<x>	, mVU_MADDAy<x>	, mVU_MSUBAy<x>,						\
	mVU_ITOF4<x>	, mVU_FTOI4<x>	, mVU_MULAy<x>	, mVU_ABS<x>,							\
	mVU_MADDAq<x>	, mVU_MSUBAq<x>	, mVU_MADDA<x>	, mVU_MSUBA<x>,							\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
};

#define microVU_UPPER_FD_10_TABLE(x) void (* mVU_UPPER_FD_10_TABLE##x [32])(mF) = {			\
	mVU_ADDAz<x>	, mVU_SUBAz<x>	, mVU_MADDAz<x>	, mVU_MSUBAz<x>,						\
	mVU_ITOF12<x>	, mVU_FTOI12<x>	, mVU_MULAz<x>	, mVU_MULAi<x>,							\
	mVU_ADDAi<x>	, mVU_SUBAi<x>	, mVU_MULA<x>	, mVU_OPMULA<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
};

#define microVU_UPPER_FD_11_TABLE(x) void (* mVU_UPPER_FD_11_TABLE##x [32])(mF) = {			\
	mVU_ADDAw<x>	, mVU_SUBAw<x>	, mVU_MADDAw<x>	, mVU_MSUBAw<x>,						\
	mVU_ITOF15<x>	, mVU_FTOI15<x>	, mVU_MULAw<x>	, mVU_CLIP<x>,							\
	mVU_MADDAi<x>	, mVU_MSUBAi<x>	, mVUunknown<x>	, mVU_NOP<x>,							\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
	mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>	, mVUunknown<x>,						\
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
mVUcreateTable(1)

//------------------------------------------------------------------
// Table Functions
//------------------------------------------------------------------
#define doTableStuff(tableName, args) {			\
	if (vuIndex) tableName##1[ args ](recPass);	\
	else		 tableName##0[ args ](recPass);	\
}

microVUf(void) mVU_UPPER_FD_00(mF)	{ doTableStuff(mVU_UPPER_FD_00_TABLE,	((mVUgetCode >> 6) & 0x1f)); } 
microVUf(void) mVU_UPPER_FD_01(mF)	{ doTableStuff(mVU_UPPER_FD_01_TABLE,	((mVUgetCode >> 6) & 0x1f)); } 
microVUf(void) mVU_UPPER_FD_10(mF)	{ doTableStuff(mVU_UPPER_FD_10_TABLE,	((mVUgetCode >> 6) & 0x1f)); } 
microVUf(void) mVU_UPPER_FD_11(mF)	{ doTableStuff(mVU_UPPER_FD_11_TABLE,	((mVUgetCode >> 6) & 0x1f)); } 
microVUf(void) mVULowerOP(mF)		{ doTableStuff(mVULowerOP_OPCODE,		 (mVUgetCode & 0x3f)); } 
microVUf(void) mVULowerOP_T3_00(mF)	{ doTableStuff(mVULowerOP_T3_00_OPCODE,	((mVUgetCode >> 6) & 0x1f)); } 
microVUf(void) mVULowerOP_T3_01(mF)	{ doTableStuff(mVULowerOP_T3_01_OPCODE,	((mVUgetCode >> 6) & 0x1f)); } 
microVUf(void) mVULowerOP_T3_10(mF)	{ doTableStuff(mVULowerOP_T3_10_OPCODE,	((mVUgetCode >> 6) & 0x1f)); } 
microVUf(void) mVULowerOP_T3_11(mF)	{ doTableStuff(mVULowerOP_T3_11_OPCODE,	((mVUgetCode >> 6) & 0x1f)); }
microVUf(void) mVUopU(mF)			{ doTableStuff(mVU_UPPER_OPCODE, (mVUgetCode & 0x3f)); } // Gets Upper Opcode
microVUf(void) mVUopL(mF)			{ doTableStuff(mVULOWER_OPCODE,  (mVUgetCode >>  25)); } // Gets Lower Opcode
microVUf(void) mVUunknown(mF) { 
	pass2 { SysPrintf("microVU%d: Unknown Micro VU opcode called (%x)\n", vuIndex, mVUgetCode); }
	pass3 { mVUlog("Unknown", mVUgetCode); }
}

