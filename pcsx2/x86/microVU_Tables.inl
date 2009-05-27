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

microVUf(void) mVU_UPPER_FD_00();
microVUf(void) mVU_UPPER_FD_01();
microVUf(void) mVU_UPPER_FD_10();
microVUf(void) mVU_UPPER_FD_11();
microVUf(void) mVULowerOP();
microVUf(void) mVULowerOP_T3_00();
microVUf(void) mVULowerOP_T3_01();
microVUf(void) mVULowerOP_T3_10();
microVUf(void) mVULowerOP_T3_11();
microVUf(void) mVUunknown();
//------------------------------------------------------------------

//------------------------------------------------------------------
// Opcode Tables
//------------------------------------------------------------------
#define microVU_LOWER_OPCODE(x, y) void (* mVULOWER_OPCODE##x##y [128])() = {					\
	mVU_LQ<x,y>		, mVU_SQ<x,y>		, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVU_ILW<x,y>	, mVU_ISW<x,y>		, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVU_IADDIU<x,y>	, mVU_ISUBIU<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVU_FCEQ<x,y>	, mVU_FCSET<x,y>	, mVU_FCAND<x,y>	, mVU_FCOR<x,y>,	/* 0x10 */		\
	mVU_FSEQ<x,y>	, mVU_FSSET<x,y>	, mVU_FSAND<x,y>	, mVU_FSOR<x,y>,					\
	mVU_FMEQ<x,y>	, mVUunknown<x,y>	, mVU_FMAND<x,y>	, mVU_FMOR<x,y>,					\
	mVU_FCGET<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVU_B<x,y>		, mVU_BAL<x,y>		, mVUunknown<x,y>	, mVUunknown<x,y>,	/* 0x20 */		\
	mVU_JR<x,y>		, mVU_JALR<x,y>		, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVU_IBEQ<x,y>	, mVU_IBNE<x,y>		, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVU_IBLTZ<x,y>	, mVU_IBGTZ<x,y>	, mVU_IBLEZ<x,y>	, mVU_IBGEZ<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,	/* 0x30 */		\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVULowerOP<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,	/* 0x40*/		\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,	/* 0x50 */		\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,	/* 0x60 */		\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,	/* 0x70 */		\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y> , mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
};

#define microVU_LowerOP_T3_00_OPCODE(x, y) void (* mVULowerOP_T3_00_OPCODE##x##y [32])() = {	\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVU_MOVE<x,y>	, mVU_LQI<x,y>		, mVU_DIV<x,y>		, mVU_MTIR<x,y>,					\
	mVU_RNEXT<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>, /* 0x10 */		\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVU_MFP<x,y>		, mVU_XTOP<x,y>		, mVU_XGKICK<x,y>,					\
	mVU_ESADD<x,y>	, mVU_EATANxy<x,y>	, mVU_ESQRT<x,y>	, mVU_ESIN<x,y>,					\
};

#define microVU_LowerOP_T3_01_OPCODE(x, y) void (* mVULowerOP_T3_01_OPCODE##x##y [32])() = {	\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVU_MR32<x,y>	, mVU_SQI<x,y>		, mVU_SQRT<x,y>		, mVU_MFIR<x,y>,					\
	mVU_RGET<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>, /* 0x10 */		\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVU_XITOP<x,y>	, mVUunknown<x,y>,					\
	mVU_ERSADD<x,y>	, mVU_EATANxz<x,y>	, mVU_ERSQRT<x,y>	, mVU_EATAN<x,y>,					\
};

#define microVU_LowerOP_T3_10_OPCODE(x, y) void (* mVULowerOP_T3_10_OPCODE##x##y [32])() = {	\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVU_LQD<x,y>		, mVU_RSQRT<x,y>	, mVU_ILWR<x,y>,					\
	mVU_RINIT<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>, /* 0x10 */		\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVU_ELENG<x,y>	, mVU_ESUM<x,y>		, mVU_ERCPR<x,y>	, mVU_EEXP<x,y>,					\
};

#define microVU_LowerOP_T3_11_OPCODE(x, y) void (* mVULowerOP_T3_11_OPCODE##x##y [32])() = {	\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVU_SQD<x,y>		, mVU_WAITQ<x,y>	, mVU_ISWR<x,y>,					\
	mVU_RXOR<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>, /* 0x10 */		\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVU_ERLENG<x,y>	, mVUunknown<x,y>	, mVU_WAITP<x,y>	, mVUunknown<x,y>,					\
};

#define microVU_LowerOP_OPCODE(x, y) void (* mVULowerOP_OPCODE##x##y [64])() = {				\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>, /* 0x10 */		\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>, /* 0x20 */		\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVU_IADD<x,y>	, mVU_ISUB<x,y>		, mVU_IADDI<x,y>	, mVUunknown<x,y>, /* 0x30 */		\
	mVU_IAND<x,y>	, mVU_IOR<x,y>		, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVULowerOP_T3_00<x,y>, mVULowerOP_T3_01<x,y>, mVULowerOP_T3_10<x,y>, mVULowerOP_T3_11<x,y>,	\
};

#define microVU_UPPER_OPCODE(x, y) void (* mVU_UPPER_OPCODE##x##y [64])() = {					\
	mVU_ADDx<x,y>	, mVU_ADDy<x,y>		, mVU_ADDz<x,y>		, mVU_ADDw<x,y>,					\
	mVU_SUBx<x,y>	, mVU_SUBy<x,y>		, mVU_SUBz<x,y>		, mVU_SUBw<x,y>,					\
	mVU_MADDx<x,y>	, mVU_MADDy<x,y>	, mVU_MADDz<x,y>	, mVU_MADDw<x,y>,					\
	mVU_MSUBx<x,y>	, mVU_MSUBy<x,y>	, mVU_MSUBz<x,y>	, mVU_MSUBw<x,y>,					\
	mVU_MAXx<x,y>	, mVU_MAXy<x,y>		, mVU_MAXz<x,y>		, mVU_MAXw<x,y>,	/* 0x10 */		\
	mVU_MINIx<x,y>	, mVU_MINIy<x,y>	, mVU_MINIz<x,y>	, mVU_MINIw<x,y>,					\
	mVU_MULx<x,y>	, mVU_MULy<x,y>		, mVU_MULz<x,y>		, mVU_MULw<x,y>,					\
	mVU_MULq<x,y>	, mVU_MAXi<x,y>		, mVU_MULi<x,y>		, mVU_MINIi<x,y>,					\
	mVU_ADDq<x,y>	, mVU_MADDq<x,y>	, mVU_ADDi<x,y>		, mVU_MADDi<x,y>,	/* 0x20 */		\
	mVU_SUBq<x,y>	, mVU_MSUBq<x,y>	, mVU_SUBi<x,y>		, mVU_MSUBi<x,y>,					\
	mVU_ADD<x,y>	, mVU_MADD<x,y>		, mVU_MUL<x,y>		, mVU_MAX<x,y>,						\
	mVU_SUB<x,y>	, mVU_MSUB<x,y>		, mVU_OPMSUB<x,y>	, mVU_MINI<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,	/* 0x30 */		\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVU_UPPER_FD_00<x,y>, mVU_UPPER_FD_01<x,y>, mVU_UPPER_FD_10<x,y>, mVU_UPPER_FD_11<x,y>,		\
};

#define microVU_UPPER_FD_00_TABLE(x, y) void (* mVU_UPPER_FD_00_TABLE##x##y [32])() = {			\
	mVU_ADDAx<x,y>	, mVU_SUBAx<x,y>	, mVU_MADDAx<x,y>	, mVU_MSUBAx<x,y>,					\
	mVU_ITOF0<x,y>	, mVU_FTOI0<x,y>	, mVU_MULAx<x,y>	, mVU_MULAq<x,y>,					\
	mVU_ADDAq<x,y>	, mVU_SUBAq<x,y>	, mVU_ADDA<x,y>		, mVU_SUBA<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
};

#define microVU_UPPER_FD_01_TABLE(x, y) void (* mVU_UPPER_FD_01_TABLE##x##y [32])() = {			\
	mVU_ADDAy<x,y>	, mVU_SUBAy<x,y>	, mVU_MADDAy<x,y>	, mVU_MSUBAy<x,y>,					\
	mVU_ITOF4<x,y>	, mVU_FTOI4<x,y>	, mVU_MULAy<x,y>	, mVU_ABS<x,y>,						\
	mVU_MADDAq<x,y>	, mVU_MSUBAq<x,y>	, mVU_MADDA<x,y>	, mVU_MSUBA<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
};

#define microVU_UPPER_FD_10_TABLE(x, y) void (* mVU_UPPER_FD_10_TABLE##x##y [32])() = {			\
	mVU_ADDAz<x,y>	, mVU_SUBAz<x,y>	, mVU_MADDAz<x,y>	, mVU_MSUBAz<x,y>,					\
	mVU_ITOF12<x,y>	, mVU_FTOI12<x,y>	, mVU_MULAz<x,y>	, mVU_MULAi<x,y>,					\
	mVU_ADDAi<x,y>	, mVU_SUBAi<x,y>	, mVU_MULA<x,y>		, mVU_OPMULA<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
};

#define microVU_UPPER_FD_11_TABLE(x, y) void (* mVU_UPPER_FD_11_TABLE##x##y [32])() = {			\
	mVU_ADDAw<x,y>	, mVU_SUBAw<x,y>	, mVU_MADDAw<x,y>	, mVU_MSUBAw<x,y>,					\
	mVU_ITOF15<x,y>	, mVU_FTOI15<x,y>	, mVU_MULAw<x,y>	, mVU_CLIP<x,y>,					\
	mVU_MADDAi<x,y>	, mVU_MSUBAi<x,y>	, mVUunknown<x,y>	, mVU_NOP<x,y>,						\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
	mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>	, mVUunknown<x,y>,					\
};

//------------------------------------------------------------------

//------------------------------------------------------------------
// Create Table Instances
//------------------------------------------------------------------
#define mVUcreateTable(x,y)			\
microVU_LOWER_OPCODE(x,y)			\
microVU_LowerOP_T3_00_OPCODE(x,y)	\
microVU_LowerOP_T3_01_OPCODE(x,y)	\
microVU_LowerOP_T3_10_OPCODE(x,y)	\
microVU_LowerOP_T3_11_OPCODE(x,y)	\
microVU_LowerOP_OPCODE(x,y)			\
microVU_UPPER_OPCODE(x,y)			\
microVU_UPPER_FD_00_TABLE(x,y)		\
microVU_UPPER_FD_01_TABLE(x,y)		\
microVU_UPPER_FD_10_TABLE(x,y)		\
microVU_UPPER_FD_11_TABLE(x,y)

mVUcreateTable(0,0)
mVUcreateTable(0,1)
mVUcreateTable(0,2)
mVUcreateTable(0,3)
mVUcreateTable(1,0)
mVUcreateTable(1,1)
mVUcreateTable(1,2)
mVUcreateTable(1,3)

//------------------------------------------------------------------
// Table Functions
//------------------------------------------------------------------
#define doTableStuff(tableName, args) {			\
	pass1 {										\
		if (vuIndex) tableName##10[ args ]();	\
		else		 tableName##00[ args ]();	\
	}											\
	pass2 {										\
		if (vuIndex) tableName##11[ args ]();	\
		else		 tableName##01[ args ]();	\
	}											\
	pass3 {										\
		if (vuIndex) tableName##12[ args ]();	\
		else		 tableName##02[ args ]();	\
	}											\
	pass4 {										\
		if (vuIndex) tableName##13[ args ]();	\
		else		 tableName##03[ args ]();	\
	}											\
}

microVUf(void) mVU_UPPER_FD_00()	{ doTableStuff(mVU_UPPER_FD_00_TABLE,	((mVUgetCode >> 6) & 0x1f)); } 
microVUf(void) mVU_UPPER_FD_01()	{ doTableStuff(mVU_UPPER_FD_01_TABLE,	((mVUgetCode >> 6) & 0x1f)); } 
microVUf(void) mVU_UPPER_FD_10()	{ doTableStuff(mVU_UPPER_FD_10_TABLE,	((mVUgetCode >> 6) & 0x1f)); } 
microVUf(void) mVU_UPPER_FD_11()	{ doTableStuff(mVU_UPPER_FD_11_TABLE,	((mVUgetCode >> 6) & 0x1f)); } 
microVUf(void) mVULowerOP()			{ doTableStuff(mVULowerOP_OPCODE,		 (mVUgetCode & 0x3f)); } 
microVUf(void) mVULowerOP_T3_00()	{ doTableStuff(mVULowerOP_T3_00_OPCODE,	((mVUgetCode >> 6) & 0x1f)); } 
microVUf(void) mVULowerOP_T3_01()	{ doTableStuff(mVULowerOP_T3_01_OPCODE,	((mVUgetCode >> 6) & 0x1f)); } 
microVUf(void) mVULowerOP_T3_10()	{ doTableStuff(mVULowerOP_T3_10_OPCODE,	((mVUgetCode >> 6) & 0x1f)); } 
microVUf(void) mVULowerOP_T3_11()	{ doTableStuff(mVULowerOP_T3_11_OPCODE,	((mVUgetCode >> 6) & 0x1f)); }
microVUf(void) mVUopU() { doTableStuff(mVU_UPPER_OPCODE, (mVUgetCode & 0x3f)); } // Gets Upper Opcode
microVUf(void) mVUopL() { doTableStuff(mVULOWER_OPCODE,  (mVUgetCode >>  25)); } // Gets Lower Opcode
microVUf(void) mVUunknown() { 
	pass2 { SysPrintf("microVU%d: Unknown Micro VU opcode called (%x)\n", vuIndex, mVUgetCode); }
	pass3 { mVUlog("Unknown", mVUgetCode); }
}

