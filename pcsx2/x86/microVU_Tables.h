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

//------------------------------------------------------------------
// Micro VU Micromode Upper instructions
//------------------------------------------------------------------

microVUf(void) mVU_ABS();
microVUf(void) mVU_ADD();
microVUf(void) mVU_ADDi();
microVUf(void) mVU_ADDq();
microVUf(void) mVU_ADDx();
microVUf(void) mVU_ADDy();
microVUf(void) mVU_ADDz();
microVUf(void) mVU_ADDw();
microVUf(void) mVU_ADDA();
microVUf(void) mVU_ADDAi();
microVUf(void) mVU_ADDAq();
microVUf(void) mVU_ADDAx();
microVUf(void) mVU_ADDAy();
microVUf(void) mVU_ADDAz();
microVUf(void) mVU_ADDAw();
microVUf(void) mVU_SUB();
microVUf(void) mVU_SUBi();
microVUf(void) mVU_SUBq();
microVUf(void) mVU_SUBx();
microVUf(void) mVU_SUBy();
microVUf(void) mVU_SUBz();
microVUf(void) mVU_SUBw();
microVUf(void) mVU_SUBA();
microVUf(void) mVU_SUBAi();
microVUf(void) mVU_SUBAq();
microVUf(void) mVU_SUBAx();
microVUf(void) mVU_SUBAy();
microVUf(void) mVU_SUBAz();
microVUf(void) mVU_SUBAw();
microVUf(void) mVU_MUL();
microVUf(void) mVU_MULi();
microVUf(void) mVU_MULq();
microVUf(void) mVU_MULx();
microVUf(void) mVU_MULy();
microVUf(void) mVU_MULz();
microVUf(void) mVU_MULw();
microVUf(void) mVU_MULA();
microVUf(void) mVU_MULAi();
microVUf(void) mVU_MULAq();
microVUf(void) mVU_MULAx();
microVUf(void) mVU_MULAy();
microVUf(void) mVU_MULAz();
microVUf(void) mVU_MULAw();
microVUf(void) mVU_MADD();
microVUf(void) mVU_MADDi();
microVUf(void) mVU_MADDq();
microVUf(void) mVU_MADDx();
microVUf(void) mVU_MADDy();
microVUf(void) mVU_MADDz();
microVUf(void) mVU_MADDw();
microVUf(void) mVU_MADDA();
microVUf(void) mVU_MADDAi();
microVUf(void) mVU_MADDAq();
microVUf(void) mVU_MADDAx();
microVUf(void) mVU_MADDAy();
microVUf(void) mVU_MADDAz();
microVUf(void) mVU_MADDAw();
microVUf(void) mVU_MSUB();
microVUf(void) mVU_MSUBi();
microVUf(void) mVU_MSUBq();
microVUf(void) mVU_MSUBx();
microVUf(void) mVU_MSUBy();
microVUf(void) mVU_MSUBz();
microVUf(void) mVU_MSUBw();
microVUf(void) mVU_MSUBA();
microVUf(void) mVU_MSUBAi();
microVUf(void) mVU_MSUBAq();
microVUf(void) mVU_MSUBAx();
microVUf(void) mVU_MSUBAy();
microVUf(void) mVU_MSUBAz();
microVUf(void) mVU_MSUBAw();
microVUf(void) mVU_MAX();
microVUf(void) mVU_MAXi();
microVUf(void) mVU_MAXx();
microVUf(void) mVU_MAXy();
microVUf(void) mVU_MAXz();
microVUf(void) mVU_MAXw();
microVUf(void) mVU_MINI();
microVUf(void) mVU_MINIi();
microVUf(void) mVU_MINIx();
microVUf(void) mVU_MINIy();
microVUf(void) mVU_MINIz();
microVUf(void) mVU_MINIw();
microVUf(void) mVU_OPMULA();
microVUf(void) mVU_OPMSUB();
microVUf(void) mVU_NOP();
microVUf(void) mVU_FTOI0();
microVUf(void) mVU_FTOI4();
microVUf(void) mVU_FTOI12();
microVUf(void) mVU_FTOI15();
microVUf(void) mVU_ITOF0();
microVUf(void) mVU_ITOF4();
microVUf(void) mVU_ITOF12();
microVUf(void) mVU_ITOF15();
microVUf(void) mVU_CLIP();

//------------------------------------------------------------------
// Micro VU Micromode Lower instructions
//------------------------------------------------------------------

microVUf(void) mVU_DIV();
microVUf(void) mVU_SQRT();
microVUf(void) mVU_RSQRT();
microVUf(void) mVU_IADD();
microVUf(void) mVU_IADDI();
microVUf(void) mVU_IADDIU();
microVUf(void) mVU_IAND();
microVUf(void) mVU_IOR();
microVUf(void) mVU_ISUB();
microVUf(void) mVU_ISUBIU();
microVUf(void) mVU_MOVE();
microVUf(void) mVU_MFIR();
microVUf(void) mVU_MTIR();
microVUf(void) mVU_MR32();
microVUf(void) mVU_LQ();
microVUf(void) mVU_LQD();
microVUf(void) mVU_LQI();
microVUf(void) mVU_SQ();
microVUf(void) mVU_SQD();
microVUf(void) mVU_SQI();
microVUf(void) mVU_ILW();
microVUf(void) mVU_ISW();
microVUf(void) mVU_ILWR();
microVUf(void) mVU_ISWR();
microVUf(void) mVU_LOI();
microVUf(void) mVU_RINIT();
microVUf(void) mVU_RGET();
microVUf(void) mVU_RNEXT();
microVUf(void) mVU_RXOR();
microVUf(void) mVU_WAITQ();
microVUf(void) mVU_FSAND();
microVUf(void) mVU_FSEQ();
microVUf(void) mVU_FSOR();
microVUf(void) mVU_FSSET();
microVUf(void) mVU_FMAND();
microVUf(void) mVU_FMEQ();
microVUf(void) mVU_FMOR();
microVUf(void) mVU_FCAND();
microVUf(void) mVU_FCEQ();
microVUf(void) mVU_FCOR();
microVUf(void) mVU_FCSET();
microVUf(void) mVU_FCGET();
microVUf(void) mVU_IBEQ();
microVUf(void) mVU_IBGEZ();
microVUf(void) mVU_IBGTZ();
microVUf(void) mVU_IBLTZ();
microVUf(void) mVU_IBLEZ();
microVUf(void) mVU_IBNE();
microVUf(void) mVU_B();
microVUf(void) mVU_BAL();
microVUf(void) mVU_JR();
microVUf(void) mVU_JALR();
microVUf(void) mVU_MFP();
microVUf(void) mVU_WAITP();
microVUf(void) mVU_ESADD();
microVUf(void) mVU_ERSADD();
microVUf(void) mVU_ELENG();
microVUf(void) mVU_ERLENG();
microVUf(void) mVU_EATANxy();
microVUf(void) mVU_EATANxz();
microVUf(void) mVU_ESUM();
microVUf(void) mVU_ERCPR();
microVUf(void) mVU_ESQRT();
microVUf(void) mVU_ERSQRT();
microVUf(void) mVU_ESIN(); 
microVUf(void) mVU_EATAN();
microVUf(void) mVU_EEXP();
microVUf(void) mVU_XGKICK();
microVUf(void) mVU_XTOP();
microVUf(void) mVU_XITOP();

