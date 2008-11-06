/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#ifndef __IVU0MICRO_H__
#define __IVU0MICRO_H__

void recResetVU0();
void recExecuteVU0Block( void );
void recClearVU0( u32 Addr, u32 Size );

extern void (*recVU0_LOWER_OPCODE[128])();
extern void (*recVU0_UPPER_OPCODE[64])();

extern void (*recVU0_UPPER_FD_00_TABLE[32])();
extern void (*recVU0_UPPER_FD_01_TABLE[32])();
extern void (*recVU0_UPPER_FD_10_TABLE[32])();
extern void (*recVU0_UPPER_FD_11_TABLE[32])();

extern void (*recVU1_LOWER_OPCODE[128])();
extern void (*recVU1_UPPER_OPCODE[64])();

extern void (*recVU1_UPPER_FD_00_TABLE[32])();
extern void (*recVU1_UPPER_FD_01_TABLE[32])();
extern void (*recVU1_UPPER_FD_10_TABLE[32])();
extern void (*recVU1_UPPER_FD_11_TABLE[32])();

void recVU0_UPPER_FD_00();
void recVU0_UPPER_FD_01();
void recVU0_UPPER_FD_10();
void recVU0_UPPER_FD_11();

void recVU0LowerOP();
void recVU0LowerOP_T3_00();
void recVU0LowerOP_T3_01();
void recVU0LowerOP_T3_10();
void recVU0LowerOP_T3_11();

void recVU0unknown();



/*****************************************
   VU0 Micromode Upper instructions
*****************************************/

void recVU0MI_ABS();
void recVU0MI_ADD();
void recVU0MI_ADDi();
void recVU0MI_ADDq();
void recVU0MI_ADDx();
void recVU0MI_ADDy();
void recVU0MI_ADDz();
void recVU0MI_ADDw();
void recVU0MI_ADDA();
void recVU0MI_ADDAi();
void recVU0MI_ADDAq();
void recVU0MI_ADDAx();
void recVU0MI_ADDAy();
void recVU0MI_ADDAz();
void recVU0MI_ADDAw();
void recVU0MI_SUB();
void recVU0MI_SUBi();
void recVU0MI_SUBq();
void recVU0MI_SUBx();
void recVU0MI_SUBy();
void recVU0MI_SUBz();
void recVU0MI_SUBw();
void recVU0MI_SUBA();
void recVU0MI_SUBAi();
void recVU0MI_SUBAq();
void recVU0MI_SUBAx();
void recVU0MI_SUBAy();
void recVU0MI_SUBAz();
void recVU0MI_SUBAw();
void recVU0MI_MUL();
void recVU0MI_MULi();
void recVU0MI_MULq();
void recVU0MI_MULx();
void recVU0MI_MULy();
void recVU0MI_MULz();
void recVU0MI_MULw();
void recVU0MI_MULA();
void recVU0MI_MULAi();
void recVU0MI_MULAq();
void recVU0MI_MULAx();
void recVU0MI_MULAy();
void recVU0MI_MULAz();
void recVU0MI_MULAw();
void recVU0MI_MADD();
void recVU0MI_MADDi();
void recVU0MI_MADDq();
void recVU0MI_MADDx();
void recVU0MI_MADDy();
void recVU0MI_MADDz();
void recVU0MI_MADDw();
void recVU0MI_MADDA();
void recVU0MI_MADDAi();
void recVU0MI_MADDAq();
void recVU0MI_MADDAx();
void recVU0MI_MADDAy();
void recVU0MI_MADDAz();
void recVU0MI_MADDAw();
void recVU0MI_MSUB();
void recVU0MI_MSUBi();
void recVU0MI_MSUBq();
void recVU0MI_MSUBx();
void recVU0MI_MSUBy();
void recVU0MI_MSUBz();
void recVU0MI_MSUBw();
void recVU0MI_MSUBA();
void recVU0MI_MSUBAi();
void recVU0MI_MSUBAq();
void recVU0MI_MSUBAx();
void recVU0MI_MSUBAy();
void recVU0MI_MSUBAz();
void recVU0MI_MSUBAw();
void recVU0MI_MAX();
void recVU0MI_MAXi();
void recVU0MI_MAXx();
void recVU0MI_MAXy();
void recVU0MI_MAXz();
void recVU0MI_MAXw();
void recVU0MI_MINI();
void recVU0MI_MINIi();
void recVU0MI_MINIx();
void recVU0MI_MINIy();
void recVU0MI_MINIz();
void recVU0MI_MINIw();
void recVU0MI_OPMULA();
void recVU0MI_OPMSUB();
void recVU0MI_NOP();
void recVU0MI_FTOI0();
void recVU0MI_FTOI4();
void recVU0MI_FTOI12();
void recVU0MI_FTOI15();
void recVU0MI_ITOF0();
void recVU0MI_ITOF4();
void recVU0MI_ITOF12();
void recVU0MI_ITOF15();
void recVU0MI_CLIP();

/*****************************************
   VU0 Micromode Lower instructions
*****************************************/

void recVU0MI_DIV();
void recVU0MI_SQRT();
void recVU0MI_RSQRT();
void recVU0MI_IADD();
void recVU0MI_IADDI();
void recVU0MI_IADDIU();
void recVU0MI_IAND();
void recVU0MI_IOR();
void recVU0MI_ISUB();
void recVU0MI_ISUBIU();
void recVU0MI_MOVE();
void recVU0MI_MFIR();
void recVU0MI_MTIR();
void recVU0MI_MR32();
void recVU0MI_LQ();
void recVU0MI_LQD();
void recVU0MI_LQI();
void recVU0MI_SQ();
void recVU0MI_SQD();
void recVU0MI_SQI();
void recVU0MI_ILW();
void recVU0MI_ISW();
void recVU0MI_ILWR();
void recVU0MI_ISWR();
void recVU0MI_RINIT();
void recVU0MI_RGET();
void recVU0MI_RNEXT();
void recVU0MI_RXOR();
void recVU0MI_WAITQ();
void recVU0MI_FSAND();
void recVU0MI_FSEQ();
void recVU0MI_FSOR();
void recVU0MI_FSSET();
void recVU0MI_FMAND();
void recVU0MI_FMEQ();
void recVU0MI_FMOR();
void recVU0MI_FCAND();
void recVU0MI_FCEQ();
void recVU0MI_FCOR();
void recVU0MI_FCSET();
void recVU0MI_FCGET();
void recVU0MI_IBEQ();
void recVU0MI_IBGEZ();
void recVU0MI_IBGTZ();
void recVU0MI_IBLEZ();
void recVU0MI_IBLTZ();
void recVU0MI_IBNE();
void recVU0MI_B();
void recVU0MI_BAL();
void recVU0MI_JR();
void recVU0MI_JALR();
void recVU0MI_MFP();
void recVU0MI_WAITP();
void recVU0MI_ESADD();
void recVU0MI_ERSADD();
void recVU0MI_ELENG();
void recVU0MI_ERLENG();
void recVU0MI_EATANxy();
void recVU0MI_EATANxz();
void recVU0MI_ESUM();
void recVU0MI_ERCPR();
void recVU0MI_ESQRT();
void recVU0MI_ERSQRT();
void recVU0MI_ESIN(); 
void recVU0MI_EATAN();
void recVU0MI_EEXP();
void recVU0MI_XITOP();

#endif /* __IVU0MICRO_H__ */
