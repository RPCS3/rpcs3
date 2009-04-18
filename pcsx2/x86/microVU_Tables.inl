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
*  You should have meived a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#pragma once
#ifdef PCSX2_MICROVU

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
// mVULOWER_OPCODE
//------------------------------------------------------------------
void (* mVULOWER_OPCODE00 [128])() = {													
	mVU_LQ<0,0>		, mVU_SQ<0,0>		, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVU_ILW<0,0>	, mVU_ISW<0,0>		, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVU_IADDIU<0,0>	, mVU_ISUBIU<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVU_FCEQ<0,0>	, mVU_FCSET<0,0>	, mVU_FCAND<0,0>	, mVU_FCOR<0,0>,	/* 0x10 */
	mVU_FSEQ<0,0>	, mVU_FSSET<0,0>	, mVU_FSAND<0,0>	, mVU_FSOR<0,0>,
	mVU_FMEQ<0,0>	, mVUunknown<0,0>	, mVU_FMAND<0,0>	, mVU_FMOR<0,0>,
	mVU_FCGET<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVU_B<0,0>		, mVU_BAL<0,0>		, mVUunknown<0,0>	, mVUunknown<0,0>,	/* 0x20 */
	mVU_JR<0,0>		, mVU_JALR<0,0>		, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVU_IBEQ<0,0>	, mVU_IBNE<0,0>		, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVU_IBLTZ<0,0>	, mVU_IBGTZ<0,0>	, mVU_IBLEZ<0,0>	, mVU_IBGEZ<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,	/* 0x30 */
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVULowerOP<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,	/* 0x40*/
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,	/* 0x50 */
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,	/* 0x60 */
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,	/* 0x70 */
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0> , mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
};

void (* mVULOWER_OPCODE01 [128])() = {													
	mVU_LQ<0,1>		, mVU_SQ<0,1>		, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVU_ILW<0,1>	, mVU_ISW<0,1>		, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVU_IADDIU<0,1>	, mVU_ISUBIU<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVU_FCEQ<0,1>	, mVU_FCSET<0,1>	, mVU_FCAND<0,1>	, mVU_FCOR<0,1>,	/* 0x10 */
	mVU_FSEQ<0,1>	, mVU_FSSET<0,1>	, mVU_FSAND<0,1>	, mVU_FSOR<0,1>,
	mVU_FMEQ<0,1>	, mVUunknown<0,1>	, mVU_FMAND<0,1>	, mVU_FMOR<0,1>,
	mVU_FCGET<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVU_B<0,1>		, mVU_BAL<0,1>		, mVUunknown<0,1>	, mVUunknown<0,1>,	/* 0x20 */
	mVU_JR<0,1>		, mVU_JALR<0,1>		, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVU_IBEQ<0,1>	, mVU_IBNE<0,1>		, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVU_IBLTZ<0,1>	, mVU_IBGTZ<0,1>	, mVU_IBLEZ<0,1>	, mVU_IBGEZ<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,	/* 0x30 */
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVULowerOP<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,	/* 0x40*/
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,	/* 0x50 */
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,	/* 0x60 */
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,	/* 0x70 */
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1> , mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
};

void (* mVULOWER_OPCODE10 [128])() = {													
	mVU_LQ<1,0>		, mVU_SQ<1,0>		, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVU_ILW<1,0>	, mVU_ISW<1,0>		, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVU_IADDIU<1,0>	, mVU_ISUBIU<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVU_FCEQ<1,0>	, mVU_FCSET<1,0>	, mVU_FCAND<1,0>	, mVU_FCOR<1,0>,	/* 0x10 */
	mVU_FSEQ<1,0>	, mVU_FSSET<1,0>	, mVU_FSAND<1,0>	, mVU_FSOR<1,0>,
	mVU_FMEQ<1,0>	, mVUunknown<1,0>	, mVU_FMAND<1,0>	, mVU_FMOR<1,0>,
	mVU_FCGET<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVU_B<1,0>		, mVU_BAL<1,0>		, mVUunknown<1,0>	, mVUunknown<1,0>,	/* 0x20 */
	mVU_JR<1,0>		, mVU_JALR<1,0>		, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVU_IBEQ<1,0>	, mVU_IBNE<1,0>		, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVU_IBLTZ<1,0>	, mVU_IBGTZ<1,0>	, mVU_IBLEZ<1,0>	, mVU_IBGEZ<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,	/* 0x30 */
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVULowerOP<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,	/* 0x40*/
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,	/* 0x50 */
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,	/* 0x60 */
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,	/* 0x70 */
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0> , mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
};

void (* mVULOWER_OPCODE11 [128])() = {													
	mVU_LQ<1,1>		, mVU_SQ<1,1>		, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVU_ILW<1,1>	, mVU_ISW<1,1>		, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVU_IADDIU<1,1>	, mVU_ISUBIU<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVU_FCEQ<1,1>	, mVU_FCSET<1,1>	, mVU_FCAND<1,1>	, mVU_FCOR<1,1>,	/* 0x10 */
	mVU_FSEQ<1,1>	, mVU_FSSET<1,1>	, mVU_FSAND<1,1>	, mVU_FSOR<1,1>,
	mVU_FMEQ<1,1>	, mVUunknown<1,1>	, mVU_FMAND<1,1>	, mVU_FMOR<1,1>,
	mVU_FCGET<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVU_B<1,1>		, mVU_BAL<1,1>		, mVUunknown<1,1>	, mVUunknown<1,1>,	/* 0x20 */
	mVU_JR<1,1>		, mVU_JALR<1,1>		, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVU_IBEQ<1,1>	, mVU_IBNE<1,1>		, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVU_IBLTZ<1,1>	, mVU_IBGTZ<1,1>	, mVU_IBLEZ<1,1>	, mVU_IBGEZ<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,	/* 0x30 */
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVULowerOP<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,	/* 0x40*/
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,	/* 0x50 */
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,	/* 0x60 */
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,	/* 0x70 */
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1> , mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
};
//------------------------------------------------------------------

//------------------------------------------------------------------
// mVULowerOP_T3_00_OPCODE
//------------------------------------------------------------------
void (* mVULowerOP_T3_00_OPCODE00 [32])() = {
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVU_MOVE<0,0>	, mVU_LQI<0,0>		, mVU_DIV<0,0>		, mVU_MTIR<0,0>,
	mVU_RNEXT<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>, /* 0x10 */
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVU_MFP<0,0>		, mVU_XTOP<0,0>		, mVU_XGKICK<0,0>,
	mVU_ESADD<0,0>	, mVU_EATANxy<0,0>	, mVU_ESQRT<0,0>	, mVU_ESIN<0,0>,
};

void (* mVULowerOP_T3_00_OPCODE01 [32])() = {
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVU_MOVE<0,1>	, mVU_LQI<0,1>		, mVU_DIV<0,1>		, mVU_MTIR<0,1>,
	mVU_RNEXT<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>, /* 0x10 */
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVU_MFP<0,1>		, mVU_XTOP<0,1>		, mVU_XGKICK<0,1>,
	mVU_ESADD<0,1>	, mVU_EATANxy<0,1>	, mVU_ESQRT<0,1>	, mVU_ESIN<0,1>,
};

void (* mVULowerOP_T3_00_OPCODE10 [32])() = {
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVU_MOVE<1,0>	, mVU_LQI<1,0>		, mVU_DIV<1,0>		, mVU_MTIR<1,0>,
	mVU_RNEXT<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>, /* 0x10 */
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVU_MFP<1,0>		, mVU_XTOP<1,0>		, mVU_XGKICK<1,0>,
	mVU_ESADD<1,0>	, mVU_EATANxy<1,0>	, mVU_ESQRT<1,0>	, mVU_ESIN<1,0>,
};

void (* mVULowerOP_T3_00_OPCODE11 [32])() = {
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVU_MOVE<1,1>	, mVU_LQI<1,1>		, mVU_DIV<1,1>		, mVU_MTIR<1,1>,
	mVU_RNEXT<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>, /* 0x10 */
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVU_MFP<1,1>		, mVU_XTOP<1,1>		, mVU_XGKICK<1,1>,
	mVU_ESADD<1,1>	, mVU_EATANxy<1,1>	, mVU_ESQRT<1,1>	, mVU_ESIN<1,1>,
};
//------------------------------------------------------------------

//------------------------------------------------------------------
// mVULowerOP_T3_01_OPCODE
//------------------------------------------------------------------
void (* mVULowerOP_T3_01_OPCODE00 [32])() = {
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVU_MR32<0,0>	, mVU_SQI<0,0>		, mVU_SQRT<0,0>		, mVU_MFIR<0,0>,
	mVU_RGET<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>, /* 0x10 */
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVU_XITOP<0,0>	, mVUunknown<0,0>,
	mVU_ERSADD<0,0>	, mVU_EATANxz<0,0>	, mVU_ERSQRT<0,0>	, mVU_EATAN<0,0>,
};

void (* mVULowerOP_T3_01_OPCODE01 [32])() = {
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVU_MR32<0,1>	, mVU_SQI<0,1>		, mVU_SQRT<0,1>		, mVU_MFIR<0,1>,
	mVU_RGET<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>, /* 0x10 */
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVU_XITOP<0,1>	, mVUunknown<0,1>,
	mVU_ERSADD<0,1>	, mVU_EATANxz<0,1>	, mVU_ERSQRT<0,1>	, mVU_EATAN<0,1>,
};

void (* mVULowerOP_T3_01_OPCODE10 [32])() = {
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVU_MR32<1,0>	, mVU_SQI<1,0>		, mVU_SQRT<1,0>		, mVU_MFIR<1,0>,
	mVU_RGET<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>, /* 0x10 */
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVU_XITOP<1,0>	, mVUunknown<1,0>,
	mVU_ERSADD<1,0>	, mVU_EATANxz<1,0>	, mVU_ERSQRT<1,0>	, mVU_EATAN<1,0>,
};

void (* mVULowerOP_T3_01_OPCODE11 [32])() = {
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVU_MR32<1,1>	, mVU_SQI<1,1>		, mVU_SQRT<1,1>		, mVU_MFIR<1,1>,
	mVU_RGET<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>, /* 0x10 */
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVU_XITOP<1,1>	, mVUunknown<1,1>,
	mVU_ERSADD<1,1>	, mVU_EATANxz<1,1>	, mVU_ERSQRT<1,1>	, mVU_EATAN<1,1>,
};
//------------------------------------------------------------------

//------------------------------------------------------------------
// mVULowerOP_T3_10_OPCODE
//------------------------------------------------------------------
void (* mVULowerOP_T3_10_OPCODE00 [32])() = {
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVU_LQD<0,0>		, mVU_RSQRT<0,0>	, mVU_ILWR<0,0>,
	mVU_RINIT<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>, /* 0x10 */
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVU_ELENG<0,0>	, mVU_ESUM<0,0>		, mVU_ERCPR<0,0>	, mVU_EEXP<0,0>,
};

void (* mVULowerOP_T3_10_OPCODE01 [32])() = {
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVU_LQD<0,1>		, mVU_RSQRT<0,1>	, mVU_ILWR<0,1>,
	mVU_RINIT<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>, /* 0x10 */
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVU_ELENG<0,1>	, mVU_ESUM<0,1>		, mVU_ERCPR<0,1>	, mVU_EEXP<0,1>,
};

void (* mVULowerOP_T3_10_OPCODE10 [32])() = {
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVU_LQD<1,0>		, mVU_RSQRT<1,0>	, mVU_ILWR<1,0>,
	mVU_RINIT<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>, /* 0x10 */
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVU_ELENG<1,0>	, mVU_ESUM<1,0>		, mVU_ERCPR<1,0>	, mVU_EEXP<1,0>,
};

void (* mVULowerOP_T3_10_OPCODE11 [32])() = {
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVU_LQD<1,1>		, mVU_RSQRT<1,1>	, mVU_ILWR<1,1>,
	mVU_RINIT<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>, /* 0x10 */
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVU_ELENG<1,1>	, mVU_ESUM<1,1>		, mVU_ERCPR<1,1>	, mVU_EEXP<1,1>,
};
//------------------------------------------------------------------

//------------------------------------------------------------------
// mVULowerOP_T3_11_OPCODE
//------------------------------------------------------------------
void (* mVULowerOP_T3_11_OPCODE00 [32])() = {
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVU_SQD<0,0>		, mVU_WAITQ<0,0>	, mVU_ISWR<0,0>,
	mVU_RXOR<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>, /* 0x10 */
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVU_ERLENG<0,0>	, mVUunknown<0,0>	, mVU_WAITP<0,0>	, mVUunknown<0,0>,
};

void (* mVULowerOP_T3_11_OPCODE01 [32])() = {
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVU_SQD<0,1>		, mVU_WAITQ<0,1>	, mVU_ISWR<0,1>,
	mVU_RXOR<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>, /* 0x10 */
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVU_ERLENG<0,1>	, mVUunknown<0,1>	, mVU_WAITP<0,1>	, mVUunknown<0,1>,
};

void (* mVULowerOP_T3_11_OPCODE10 [32])() = {
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVU_SQD<1,0>		, mVU_WAITQ<1,0>	, mVU_ISWR<1,0>,
	mVU_RXOR<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>, /* 0x10 */
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVU_ERLENG<1,0>	, mVUunknown<1,0>	, mVU_WAITP<1,0>	, mVUunknown<1,0>,
};

void (* mVULowerOP_T3_11_OPCODE11 [32])() = {
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVU_SQD<1,1>		, mVU_WAITQ<1,1>	, mVU_ISWR<1,1>,
	mVU_RXOR<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>, /* 0x10 */
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVU_ERLENG<1,1>	, mVUunknown<1,1>	, mVU_WAITP<1,1>	, mVUunknown<1,1>,
};
//------------------------------------------------------------------

//------------------------------------------------------------------
// mVULowerOP_OPCODE
//------------------------------------------------------------------
void (* mVULowerOP_OPCODE00 [64])() = {	
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>, /* 0x10 */
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>, /* 0x20 */
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVU_IADD<0,0>	, mVU_ISUB<0,0>		, mVU_IADDI<0,0>	, mVUunknown<0,0>, /* 0x30 */
	mVU_IAND<0,0>	, mVU_IOR<0,0>		, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVULowerOP_T3_00<0,0>, mVULowerOP_T3_01<0,0>, mVULowerOP_T3_10<0,0>, mVULowerOP_T3_11<0,0>,
};

void (* mVULowerOP_OPCODE01 [64])() = {	
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>, /* 0x10 */
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>, /* 0x20 */
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVU_IADD<0,1>	, mVU_ISUB<0,1>		, mVU_IADDI<0,1>	, mVUunknown<0,1>, /* 0x30 */
	mVU_IAND<0,1>	, mVU_IOR<0,1>		, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVULowerOP_T3_00<0,1>, mVULowerOP_T3_01<0,1>, mVULowerOP_T3_10<0,1>, mVULowerOP_T3_11<0,1>,
};

void (* mVULowerOP_OPCODE10 [64])() = {	
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>, /* 0x10 */
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>, /* 0x20 */
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVU_IADD<1,0>	, mVU_ISUB<1,0>		, mVU_IADDI<1,0>	, mVUunknown<1,0>, /* 0x30 */
	mVU_IAND<1,0>	, mVU_IOR<1,0>		, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVULowerOP_T3_00<1,0>, mVULowerOP_T3_01<1,0>, mVULowerOP_T3_10<1,0>, mVULowerOP_T3_11<1,0>,
};

void (* mVULowerOP_OPCODE11 [64])() = {	
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>, /* 0x10 */
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>, /* 0x20 */
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVU_IADD<1,1>	, mVU_ISUB<1,1>		, mVU_IADDI<1,1>	, mVUunknown<1,1>, /* 0x30 */
	mVU_IAND<1,1>	, mVU_IOR<1,1>		, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVULowerOP_T3_00<1,1>, mVULowerOP_T3_01<1,1>, mVULowerOP_T3_10<1,1>, mVULowerOP_T3_11<1,1>,
};
//------------------------------------------------------------------

//------------------------------------------------------------------
// mVU_UPPER_OPCODE
//------------------------------------------------------------------
void (* mVU_UPPER_OPCODE00 [64])() = {
	mVU_ADDx<0,0>	, mVU_ADDy<0,0>		, mVU_ADDz<0,0>		, mVU_ADDw<0,0>,
	mVU_SUBx<0,0>	, mVU_SUBy<0,0>		, mVU_SUBz<0,0>		, mVU_SUBw<0,0>,
	mVU_MADDx<0,0>	, mVU_MADDy<0,0>	, mVU_MADDz<0,0>	, mVU_MADDw<0,0>,
	mVU_MSUBx<0,0>	, mVU_MSUBy<0,0>	, mVU_MSUBz<0,0>	, mVU_MSUBw<0,0>,
	mVU_MAXx<0,0>	, mVU_MAXy<0,0>		, mVU_MAXz<0,0>		, mVU_MAXw<0,0>,	/* 0x10 */ 
	mVU_MINIx<0,0>	, mVU_MINIy<0,0>	, mVU_MINIz<0,0>	, mVU_MINIw<0,0>,
	mVU_MULx<0,0>	, mVU_MULy<0,0>		, mVU_MULz<0,0>		, mVU_MULw<0,0>,
	mVU_MULq<0,0>	, mVU_MAXi<0,0>		, mVU_MULi<0,0>		, mVU_MINIi<0,0>,
	mVU_ADDq<0,0>	, mVU_MADDq<0,0>	, mVU_ADDi<0,0>		, mVU_MADDi<0,0>,	/* 0x20 */
	mVU_SUBq<0,0>	, mVU_MSUBq<0,0>	, mVU_SUBi<0,0>		, mVU_MSUBi<0,0>,
	mVU_ADD<0,0>	, mVU_MADD<0,0>		, mVU_MUL<0,0>		, mVU_MAX<0,0>,
	mVU_SUB<0,0>	, mVU_MSUB<0,0>		, mVU_OPMSUB<0,0>	, mVU_MINI<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,	/* 0x30 */
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVU_UPPER_FD_00<0,0>, mVU_UPPER_FD_01<0,0>, mVU_UPPER_FD_10<0,0>, mVU_UPPER_FD_11<0,0>,
};

void (* mVU_UPPER_OPCODE01 [64])() = {
	mVU_ADDx<0,1>	, mVU_ADDy<0,1>		, mVU_ADDz<0,1>		, mVU_ADDw<0,1>,
	mVU_SUBx<0,1>	, mVU_SUBy<0,1>		, mVU_SUBz<0,1>		, mVU_SUBw<0,1>,
	mVU_MADDx<0,1>	, mVU_MADDy<0,1>	, mVU_MADDz<0,1>	, mVU_MADDw<0,1>,
	mVU_MSUBx<0,1>	, mVU_MSUBy<0,1>	, mVU_MSUBz<0,1>	, mVU_MSUBw<0,1>,
	mVU_MAXx<0,1>	, mVU_MAXy<0,1>		, mVU_MAXz<0,1>		, mVU_MAXw<0,1>,	/* 0x10 */ 
	mVU_MINIx<0,1>	, mVU_MINIy<0,1>	, mVU_MINIz<0,1>	, mVU_MINIw<0,1>,
	mVU_MULx<0,1>	, mVU_MULy<0,1>		, mVU_MULz<0,1>		, mVU_MULw<0,1>,
	mVU_MULq<0,1>	, mVU_MAXi<0,1>		, mVU_MULi<0,1>		, mVU_MINIi<0,1>,
	mVU_ADDq<0,1>	, mVU_MADDq<0,1>	, mVU_ADDi<0,1>		, mVU_MADDi<0,1>,	/* 0x20 */
	mVU_SUBq<0,1>	, mVU_MSUBq<0,1>	, mVU_SUBi<0,1>		, mVU_MSUBi<0,1>,
	mVU_ADD<0,1>	, mVU_MADD<0,1>		, mVU_MUL<0,1>		, mVU_MAX<0,1>,
	mVU_SUB<0,1>	, mVU_MSUB<0,1>		, mVU_OPMSUB<0,1>	, mVU_MINI<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,	/* 0x30 */
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVU_UPPER_FD_00<0,1>, mVU_UPPER_FD_01<0,1>, mVU_UPPER_FD_10<0,1>, mVU_UPPER_FD_11<0,1>,
};

void (* mVU_UPPER_OPCODE10 [64])() = {
	mVU_ADDx<1,0>	, mVU_ADDy<1,0>		, mVU_ADDz<1,0>		, mVU_ADDw<1,0>,
	mVU_SUBx<1,0>	, mVU_SUBy<1,0>		, mVU_SUBz<1,0>		, mVU_SUBw<1,0>,
	mVU_MADDx<1,0>	, mVU_MADDy<1,0>	, mVU_MADDz<1,0>	, mVU_MADDw<1,0>,
	mVU_MSUBx<1,0>	, mVU_MSUBy<1,0>	, mVU_MSUBz<1,0>	, mVU_MSUBw<1,0>,
	mVU_MAXx<1,0>	, mVU_MAXy<1,0>		, mVU_MAXz<1,0>		, mVU_MAXw<1,0>,	/* 0x10 */ 
	mVU_MINIx<1,0>	, mVU_MINIy<1,0>	, mVU_MINIz<1,0>	, mVU_MINIw<1,0>,
	mVU_MULx<1,0>	, mVU_MULy<1,0>		, mVU_MULz<1,0>		, mVU_MULw<1,0>,
	mVU_MULq<1,0>	, mVU_MAXi<1,0>		, mVU_MULi<1,0>		, mVU_MINIi<1,0>,
	mVU_ADDq<1,0>	, mVU_MADDq<1,0>	, mVU_ADDi<1,0>		, mVU_MADDi<1,0>,	/* 0x20 */
	mVU_SUBq<1,0>	, mVU_MSUBq<1,0>	, mVU_SUBi<1,0>		, mVU_MSUBi<1,0>,
	mVU_ADD<1,0>	, mVU_MADD<1,0>		, mVU_MUL<1,0>		, mVU_MAX<1,0>,
	mVU_SUB<1,0>	, mVU_MSUB<1,0>		, mVU_OPMSUB<1,0>	, mVU_MINI<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,	/* 0x30 */
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVU_UPPER_FD_00<1,0>, mVU_UPPER_FD_01<1,0>, mVU_UPPER_FD_10<1,0>, mVU_UPPER_FD_11<1,0>,
};

void (* mVU_UPPER_OPCODE11 [64])() = {
	mVU_ADDx<1,1>	, mVU_ADDy<1,1>		, mVU_ADDz<1,1>		, mVU_ADDw<1,1>,
	mVU_SUBx<1,1>	, mVU_SUBy<1,1>		, mVU_SUBz<1,1>		, mVU_SUBw<1,1>,
	mVU_MADDx<1,1>	, mVU_MADDy<1,1>	, mVU_MADDz<1,1>	, mVU_MADDw<1,1>,
	mVU_MSUBx<1,1>	, mVU_MSUBy<1,1>	, mVU_MSUBz<1,1>	, mVU_MSUBw<1,1>,
	mVU_MAXx<1,1>	, mVU_MAXy<1,1>		, mVU_MAXz<1,1>		, mVU_MAXw<1,1>,	/* 0x10 */ 
	mVU_MINIx<1,1>	, mVU_MINIy<1,1>	, mVU_MINIz<1,1>	, mVU_MINIw<1,1>,
	mVU_MULx<1,1>	, mVU_MULy<1,1>		, mVU_MULz<1,1>		, mVU_MULw<1,1>,
	mVU_MULq<1,1>	, mVU_MAXi<1,1>		, mVU_MULi<1,1>		, mVU_MINIi<1,1>,
	mVU_ADDq<1,1>	, mVU_MADDq<1,1>	, mVU_ADDi<1,1>		, mVU_MADDi<1,1>,	/* 0x20 */
	mVU_SUBq<1,1>	, mVU_MSUBq<1,1>	, mVU_SUBi<1,1>		, mVU_MSUBi<1,1>,
	mVU_ADD<1,1>	, mVU_MADD<1,1>		, mVU_MUL<1,1>		, mVU_MAX<1,1>,
	mVU_SUB<1,1>	, mVU_MSUB<1,1>		, mVU_OPMSUB<1,1>	, mVU_MINI<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,	/* 0x30 */
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVU_UPPER_FD_00<1,1>, mVU_UPPER_FD_01<1,1>, mVU_UPPER_FD_10<1,1>, mVU_UPPER_FD_11<1,1>,
};
//------------------------------------------------------------------

//------------------------------------------------------------------
// mVU_UPPER_FD_00_TABLE
//------------------------------------------------------------------
void (* mVU_UPPER_FD_00_TABLE00 [32])() = {
	mVU_ADDAx<0,0>	, mVU_SUBAx<0,0>	, mVU_MADDAx<0,0>	, mVU_MSUBAx<0,0>,
	mVU_ITOF0<0,0>	, mVU_FTOI0<0,0>	, mVU_MULAx<0,0>	, mVU_MULAq<0,0>,
	mVU_ADDAq<0,0>	, mVU_SUBAq<0,0>	, mVU_ADDA<0,0>		, mVU_SUBA<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
};

void (* mVU_UPPER_FD_00_TABLE01 [32])() = {
	mVU_ADDAx<0,1>	, mVU_SUBAx<0,1>	, mVU_MADDAx<0,1>	, mVU_MSUBAx<0,1>,
	mVU_ITOF0<0,1>	, mVU_FTOI0<0,1>	, mVU_MULAx<0,1>	, mVU_MULAq<0,1>,
	mVU_ADDAq<0,1>	, mVU_SUBAq<0,1>	, mVU_ADDA<0,1>		, mVU_SUBA<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
};

void (* mVU_UPPER_FD_00_TABLE10 [32])() = {
	mVU_ADDAx<1,0>	, mVU_SUBAx<1,0>	, mVU_MADDAx<1,0>	, mVU_MSUBAx<1,0>,
	mVU_ITOF0<1,0>	, mVU_FTOI0<1,0>	, mVU_MULAx<1,0>	, mVU_MULAq<1,0>,
	mVU_ADDAq<1,0>	, mVU_SUBAq<1,0>	, mVU_ADDA<1,0>		, mVU_SUBA<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
};

void (* mVU_UPPER_FD_00_TABLE11 [32])() = {
	mVU_ADDAx<1,1>	, mVU_SUBAx<1,1>	, mVU_MADDAx<1,1>	, mVU_MSUBAx<1,1>,
	mVU_ITOF0<1,1>	, mVU_FTOI0<1,1>	, mVU_MULAx<1,1>	, mVU_MULAq<1,1>,
	mVU_ADDAq<1,1>	, mVU_SUBAq<1,1>	, mVU_ADDA<1,1>		, mVU_SUBA<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
};
//------------------------------------------------------------------

//------------------------------------------------------------------
// mVU_UPPER_FD_01_TABLE
//------------------------------------------------------------------
void (* mVU_UPPER_FD_01_TABLE00 [32])() = {
	mVU_ADDAy<0,0>	, mVU_SUBAy<0,0>	, mVU_MADDAy<0,0>	, mVU_MSUBAy<0,0>,
	mVU_ITOF4<0,0>	, mVU_FTOI4<0,0>	, mVU_MULAy<0,0>	, mVU_ABS<0,0>,
	mVU_MADDAq<0,0>	, mVU_MSUBAq<0,0>	, mVU_MADDA<0,0>	, mVU_MSUBA<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
};

void (* mVU_UPPER_FD_01_TABLE01 [32])() = {
	mVU_ADDAy<0,1>	, mVU_SUBAy<0,1>	, mVU_MADDAy<0,1>	, mVU_MSUBAy<0,1>,
	mVU_ITOF4<0,1>	, mVU_FTOI4<0,1>	, mVU_MULAy<0,1>	, mVU_ABS<0,1>,
	mVU_MADDAq<0,1>	, mVU_MSUBAq<0,1>	, mVU_MADDA<0,1>	, mVU_MSUBA<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
};

void (* mVU_UPPER_FD_01_TABLE10 [32])() = {
	mVU_ADDAy<1,0>	, mVU_SUBAy<1,0>	, mVU_MADDAy<1,0>	, mVU_MSUBAy<1,0>,
	mVU_ITOF4<1,0>	, mVU_FTOI4<1,0>	, mVU_MULAy<1,0>	, mVU_ABS<1,0>,
	mVU_MADDAq<1,0>	, mVU_MSUBAq<1,0>	, mVU_MADDA<1,0>	, mVU_MSUBA<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
};

void (* mVU_UPPER_FD_01_TABLE11 [32])() = {
	mVU_ADDAy<1,1>	, mVU_SUBAy<1,1>	, mVU_MADDAy<1,1>	, mVU_MSUBAy<1,1>,
	mVU_ITOF4<1,1>	, mVU_FTOI4<1,1>	, mVU_MULAy<1,1>	, mVU_ABS<1,1>,
	mVU_MADDAq<1,1>	, mVU_MSUBAq<1,1>	, mVU_MADDA<1,1>	, mVU_MSUBA<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
};
//------------------------------------------------------------------

//------------------------------------------------------------------
// mVU_UPPER_FD_10_TABLE
//------------------------------------------------------------------
void (* mVU_UPPER_FD_10_TABLE00 [32])() = {
	mVU_ADDAz<0,0>	, mVU_SUBAz<0,0>	, mVU_MADDAz<0,0>	, mVU_MSUBAz<0,0>,
	mVU_ITOF12<0,0>	, mVU_FTOI12<0,0>	, mVU_MULAz<0,0>	, mVU_MULAi<0,0>,
	mVU_ADDAi<0,0>	, mVU_SUBAi<0,0>	, mVU_MULA<0,0>		, mVU_OPMULA<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
};

void (* mVU_UPPER_FD_10_TABLE01 [32])() = {
	mVU_ADDAz<0,1>	, mVU_SUBAz<0,1>	, mVU_MADDAz<0,1>	, mVU_MSUBAz<0,1>,
	mVU_ITOF12<0,1>	, mVU_FTOI12<0,1>	, mVU_MULAz<0,1>	, mVU_MULAi<0,1>,
	mVU_ADDAi<0,1>	, mVU_SUBAi<0,1>	, mVU_MULA<0,1>		, mVU_OPMULA<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
};

void (* mVU_UPPER_FD_10_TABLE10 [32])() = {
	mVU_ADDAz<1,0>	, mVU_SUBAz<1,0>	, mVU_MADDAz<1,0>	, mVU_MSUBAz<1,0>,
	mVU_ITOF12<1,0>	, mVU_FTOI12<1,0>	, mVU_MULAz<1,0>	, mVU_MULAi<1,0>,
	mVU_ADDAi<1,0>	, mVU_SUBAi<1,0>	, mVU_MULA<1,0>		, mVU_OPMULA<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
};

void (* mVU_UPPER_FD_10_TABLE11 [32])() = {
	mVU_ADDAz<1,1>	, mVU_SUBAz<1,1>	, mVU_MADDAz<1,1>	, mVU_MSUBAz<1,1>,
	mVU_ITOF12<1,1>	, mVU_FTOI12<1,1>	, mVU_MULAz<1,1>	, mVU_MULAi<1,1>,
	mVU_ADDAi<1,1>	, mVU_SUBAi<1,1>	, mVU_MULA<1,1>		, mVU_OPMULA<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
};
//------------------------------------------------------------------

//------------------------------------------------------------------
// mVU_UPPER_FD_11_TABLE
//------------------------------------------------------------------
void (* mVU_UPPER_FD_11_TABLE00 [32])() = {
	mVU_ADDAw<0,0>	, mVU_SUBAw<0,0>	, mVU_MADDAw<0,0>	, mVU_MSUBAw<0,0>,
	mVU_ITOF15<0,0>	, mVU_FTOI15<0,0>	, mVU_MULAw<0,0>	, mVU_CLIP<0,0>,
	mVU_MADDAi<0,0>	, mVU_MSUBAi<0,0>	, mVUunknown<0,0>	, mVU_NOP<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
	mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>	, mVUunknown<0,0>,
};

void (* mVU_UPPER_FD_11_TABLE01 [32])() = {
	mVU_ADDAw<0,1>	, mVU_SUBAw<0,1>	, mVU_MADDAw<0,1>	, mVU_MSUBAw<0,1>,
	mVU_ITOF15<0,1>	, mVU_FTOI15<0,1>	, mVU_MULAw<0,1>	, mVU_CLIP<0,1>,
	mVU_MADDAi<0,1>	, mVU_MSUBAi<0,1>	, mVUunknown<0,1>	, mVU_NOP<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
	mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>	, mVUunknown<0,1>,
};

void (* mVU_UPPER_FD_11_TABLE10 [32])() = {
	mVU_ADDAw<1,0>	, mVU_SUBAw<1,0>	, mVU_MADDAw<1,0>	, mVU_MSUBAw<1,0>,
	mVU_ITOF15<1,0>	, mVU_FTOI15<1,0>	, mVU_MULAw<1,0>	, mVU_CLIP<1,0>,
	mVU_MADDAi<1,0>	, mVU_MSUBAi<1,0>	, mVUunknown<1,0>	, mVU_NOP<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
	mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>	, mVUunknown<1,0>,
};

void (* mVU_UPPER_FD_11_TABLE11 [32])() = {
	mVU_ADDAw<1,1>	, mVU_SUBAw<1,1>	, mVU_MADDAw<1,1>	, mVU_MSUBAw<1,1>,
	mVU_ITOF15<1,1>	, mVU_FTOI15<1,1>	, mVU_MULAw<1,1>	, mVU_CLIP<1,1>,
	mVU_MADDAi<1,1>	, mVU_MSUBAi<1,1>	, mVUunknown<1,1>	, mVU_NOP<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
	mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>	, mVUunknown<1,1>,
};
//------------------------------------------------------------------

//------------------------------------------------------------------
// Table Functions
//------------------------------------------------------------------
#define doTableStuff(tableName, args) {			\
	if (recPass) {								\
		if (vuIndex) tableName##11[ args ]();	\
		else tableName##01[ args ]();			\
	}											\
	else {										\
		if (vuIndex) tableName##10[ args ]();	\
		else tableName##00[ args ]();			\
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
microVUf(void) mVUunknown() { SysPrintf("mVUunknown<%d,%d> : Unknown Micro VU opcode called\n", vuIndex, recPass); }
microVUf(void) mVUopU() { doTableStuff(mVU_UPPER_OPCODE, (mVUgetCode & 0x3f)); } // Gets Upper Opcode
microVUf(void) mVUopL() { doTableStuff(mVULOWER_OPCODE,  (mVUgetCode >>  25)); } // Gets Lower Opcode

#endif //PCSX2_MICROVU
