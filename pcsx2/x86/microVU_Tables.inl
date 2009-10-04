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

#pragma once

//------------------------------------------------------------------
// Declarations
//------------------------------------------------------------------
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
void (*mVULOWER_OPCODE [128])(mP) = {
	mVU_LQ		, mVU_SQ		, mVUunknown	, mVUunknown,
	mVU_ILW		, mVU_ISW		, mVUunknown	, mVUunknown,
	mVU_IADDIU	, mVU_ISUBIU	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVU_FCEQ	, mVU_FCSET		, mVU_FCAND		, mVU_FCOR,
	mVU_FSEQ	, mVU_FSSET		, mVU_FSAND		, mVU_FSOR,	
	mVU_FMEQ	, mVUunknown	, mVU_FMAND		, mVU_FMOR,	
	mVU_FCGET	, mVUunknown	, mVUunknown	, mVUunknown,
	mVU_B		, mVU_BAL		, mVUunknown	, mVUunknown,
	mVU_JR		, mVU_JALR		, mVUunknown	, mVUunknown,
	mVU_IBEQ	, mVU_IBNE		, mVUunknown	, mVUunknown,
	mVU_IBLTZ	, mVU_IBGTZ		, mVU_IBLEZ		, mVU_IBGEZ,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVULowerOP	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
};

void (*mVULowerOP_T3_00_OPCODE [32])(mP) = {
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVU_MOVE	, mVU_LQI		, mVU_DIV		, mVU_MTIR,	
	mVU_RNEXT	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVU_MFP		, mVU_XTOP		, mVU_XGKICK,
	mVU_ESADD	, mVU_EATANxy	, mVU_ESQRT		, mVU_ESIN,	
};

void (*mVULowerOP_T3_01_OPCODE [32])(mP) = {
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVU_MR32	, mVU_SQI		, mVU_SQRT		, mVU_MFIR,	
	mVU_RGET	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVU_XITOP		, mVUunknown,
	mVU_ERSADD	, mVU_EATANxz	, mVU_ERSQRT	, mVU_EATAN,
};

void (*mVULowerOP_T3_10_OPCODE [32])(mP) = {
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVU_LQD		, mVU_RSQRT		, mVU_ILWR,	
	mVU_RINIT	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVU_ELENG	, mVU_ESUM		, mVU_ERCPR		, mVU_EEXP,	
};

void (*mVULowerOP_T3_11_OPCODE [32])(mP) = {
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVU_SQD		, mVU_WAITQ		, mVU_ISWR,	
	mVU_RXOR	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVU_ERLENG	, mVUunknown	, mVU_WAITP		, mVUunknown,
};

void (*mVULowerOP_OPCODE [64])(mP) = {
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVU_IADD	, mVU_ISUB		, mVU_IADDI		, mVUunknown,
	mVU_IAND	, mVU_IOR		, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVULowerOP_T3_00, mVULowerOP_T3_01, mVULowerOP_T3_10, mVULowerOP_T3_11,
};

void (*mVU_UPPER_OPCODE [64])(mP) = {
	mVU_ADDx	, mVU_ADDy		, mVU_ADDz		, mVU_ADDw,	
	mVU_SUBx	, mVU_SUBy		, mVU_SUBz		, mVU_SUBw,	
	mVU_MADDx	, mVU_MADDy		, mVU_MADDz		, mVU_MADDw,
	mVU_MSUBx	, mVU_MSUBy		, mVU_MSUBz		, mVU_MSUBw,
	mVU_MAXx	, mVU_MAXy		, mVU_MAXz		, mVU_MAXw,
	mVU_MINIx	, mVU_MINIy		, mVU_MINIz		, mVU_MINIw,
	mVU_MULx	, mVU_MULy		, mVU_MULz		, mVU_MULw,	
	mVU_MULq	, mVU_MAXi		, mVU_MULi		, mVU_MINIi,
	mVU_ADDq	, mVU_MADDq		, mVU_ADDi		, mVU_MADDi,
	mVU_SUBq	, mVU_MSUBq		, mVU_SUBi		, mVU_MSUBi,
	mVU_ADD		, mVU_MADD		, mVU_MUL		, mVU_MAX,	
	mVU_SUB		, mVU_MSUB		, mVU_OPMSUB	, mVU_MINI,	
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVU_UPPER_FD_00, mVU_UPPER_FD_01, mVU_UPPER_FD_10, mVU_UPPER_FD_11,
};

void (*mVU_UPPER_FD_00_TABLE [32])(mP) = {
	mVU_ADDAx	, mVU_SUBAx		, mVU_MADDAx	, mVU_MSUBAx,
	mVU_ITOF0	, mVU_FTOI0		, mVU_MULAx		, mVU_MULAq,
	mVU_ADDAq	, mVU_SUBAq		, mVU_ADDA		, mVU_SUBA,	
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
};

void (* mVU_UPPER_FD_01_TABLE [32])(mP) = {
	mVU_ADDAy	, mVU_SUBAy		, mVU_MADDAy	, mVU_MSUBAy,
	mVU_ITOF4	, mVU_FTOI4		, mVU_MULAy		, mVU_ABS,	
	mVU_MADDAq	, mVU_MSUBAq	, mVU_MADDA		, mVU_MSUBA,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
};

void (* mVU_UPPER_FD_10_TABLE [32])(mP) = {
	mVU_ADDAz	, mVU_SUBAz		, mVU_MADDAz	, mVU_MSUBAz,
	mVU_ITOF12	, mVU_FTOI12	, mVU_MULAz		, mVU_MULAi,
	mVU_ADDAi	, mVU_SUBAi		, mVU_MULA		, mVU_OPMULA,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
};

void (* mVU_UPPER_FD_11_TABLE [32])(mP) = {
	mVU_ADDAw	, mVU_SUBAw		, mVU_MADDAw	, mVU_MSUBAw,
	mVU_ITOF15	, mVU_FTOI15	, mVU_MULAw		, mVU_CLIP,
	mVU_MADDAi	, mVU_MSUBAi	, mVUunknown	, mVU_NOP,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
	mVUunknown	, mVUunknown	, mVUunknown	, mVUunknown,
};


//------------------------------------------------------------------
// Table Functions
//------------------------------------------------------------------

mVUop(mVU_UPPER_FD_00)	{ mVU_UPPER_FD_00_TABLE		[((mVU->code >> 6) & 0x1f)](mX); }
mVUop(mVU_UPPER_FD_01)	{ mVU_UPPER_FD_01_TABLE		[((mVU->code >> 6) & 0x1f)](mX); }
mVUop(mVU_UPPER_FD_10)	{ mVU_UPPER_FD_10_TABLE		[((mVU->code >> 6) & 0x1f)](mX); }
mVUop(mVU_UPPER_FD_11)	{ mVU_UPPER_FD_11_TABLE		[((mVU->code >> 6) & 0x1f)](mX); }
mVUop(mVULowerOP)		{ mVULowerOP_OPCODE			[ (mVU->code & 0x3f) ](mX); }
mVUop(mVULowerOP_T3_00)	{ mVULowerOP_T3_00_OPCODE	[((mVU->code >> 6) & 0x1f)](mX); }
mVUop(mVULowerOP_T3_01)	{ mVULowerOP_T3_01_OPCODE	[((mVU->code >> 6) & 0x1f)](mX); }
mVUop(mVULowerOP_T3_10)	{ mVULowerOP_T3_10_OPCODE	[((mVU->code >> 6) & 0x1f)](mX); }
mVUop(mVULowerOP_T3_11)	{ mVULowerOP_T3_11_OPCODE	[((mVU->code >> 6) & 0x1f)](mX); }
mVUop(mVUopU)			{ mVU_UPPER_OPCODE			[ (mVU->code & 0x3f) ](mX); } // Gets Upper Opcode
mVUop(mVUopL)			{ mVULOWER_OPCODE			[ (mVU->code >>  25) ](mX); } // Gets Lower Opcode
mVUop(mVUunknown) {
	pass2 { Console.Error("microVU%d: Unknown Micro VU opcode called (%x) [%04x]\n", getIndex, mVU->code, xPC); }
	pass3 { mVUlog("Unknown", mVU->code); }
}

