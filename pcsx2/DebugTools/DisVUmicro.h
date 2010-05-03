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


#define _disVUTables(VU) \
 \
/****************/ \
/* LOWER TABLES */ \
/****************/ \
 \
TdisR5900F dis##VU##LowerOP_T3_00_OPCODE[32] = { \
	disNULL     , disNULL     , disNULL    , disNULL   , \
	disNULL     , disNULL     , disNULL    , disNULL   , \
	disNULL     , disNULL     , disNULL    , disNULL   , \
	dis##VU##MI_MOVE  , dis##VU##MI_LQI   , dis##VU##MI_DIV  , dis##VU##MI_MTIR, \
	dis##VU##MI_RNEXT , disNULL     , disNULL    , disNULL   , /* 0x10 */ \
	disNULL     , disNULL     , disNULL    , disNULL   , \
	disNULL     , dis##VU##MI_MFP   , dis##VU##MI_XTOP , dis##VU##MI_XGKICK, \
	dis##VU##MI_ESADD , dis##VU##MI_EATANxy, dis##VU##MI_ESQRT, dis##VU##MI_ESIN,  \
}; \
 \
TdisR5900F dis##VU##LowerOP_T3_01_OPCODE[32] = { \
	disNULL     , disNULL     , disNULL    , disNULL   , \
	disNULL     , disNULL     , disNULL    , disNULL   , \
	disNULL     , disNULL     , disNULL    , disNULL   , \
	dis##VU##MI_MR32  , dis##VU##MI_SQI   , dis##VU##MI_SQRT , dis##VU##MI_MFIR, \
	dis##VU##MI_RGET  , disNULL     , disNULL    , disNULL   , /* 0x10 */ \
	disNULL     , disNULL     , disNULL    , disNULL   , \
	disNULL     , disNULL     , dis##VU##MI_XITOP, disNULL   , \
	dis##VU##MI_ERSADD, dis##VU##MI_EATANxz, dis##VU##MI_ERSQRT, dis##VU##MI_EATAN, \
}; \
 \
TdisR5900F dis##VU##LowerOP_T3_10_OPCODE[32] = { \
	disNULL     , disNULL     , disNULL    , disNULL   , \
	disNULL     , disNULL     , disNULL    , disNULL   , \
	disNULL     , disNULL     , disNULL    , disNULL   , \
	disNULL     , dis##VU##MI_LQD   , dis##VU##MI_RSQRT, dis##VU##MI_ILWR, \
	dis##VU##MI_RINIT , disNULL     , disNULL    , disNULL   , /* 0x10 */ \
	disNULL     , disNULL     , disNULL    , disNULL   ,  \
	disNULL     , disNULL     , disNULL    , disNULL   ,  \
	dis##VU##MI_ELENG , dis##VU##MI_ESUM  , dis##VU##MI_ERCPR, dis##VU##MI_EEXP,  \
}; \
 \
TdisR5900F dis##VU##LowerOP_T3_11_OPCODE[32] = { \
	disNULL     , disNULL     , disNULL    , disNULL   , \
	disNULL     , disNULL     , disNULL    , disNULL   , \
	disNULL     , disNULL     , disNULL    , disNULL   ,  \
	disNULL     , dis##VU##MI_SQD   , dis##VU##MI_WAITQ, dis##VU##MI_ISWR,  \
	dis##VU##MI_RXOR  , disNULL     , disNULL    , disNULL   , /* 0x10 */ \
	disNULL     , disNULL     , disNULL    , disNULL   ,  \
	disNULL     , disNULL     , disNULL    , disNULL   ,  \
	dis##VU##MI_ERLENG, disNULL     , dis##VU##MI_WAITP, disNULL   ,  \
}; \
 \
MakeDisF(dis##VU##LowerOP_T3_00,	dis##VU##LowerOP_T3_00_OPCODE[_Fd_] DisFInterfaceN) \
MakeDisF(dis##VU##LowerOP_T3_01,	dis##VU##LowerOP_T3_01_OPCODE[_Fd_] DisFInterfaceN) \
MakeDisF(dis##VU##LowerOP_T3_10,	dis##VU##LowerOP_T3_10_OPCODE[_Fd_] DisFInterfaceN) \
MakeDisF(dis##VU##LowerOP_T3_11,	dis##VU##LowerOP_T3_11_OPCODE[_Fd_] DisFInterfaceN) \
 \
TdisR5900F dis##VU##LowerOP_OPCODE[64] = { \
	disNULL     , disNULL     , disNULL    , disNULL   , \
	disNULL     , disNULL     , disNULL    , disNULL   , \
	disNULL     , disNULL     , disNULL    , disNULL   , \
	disNULL     , disNULL     , disNULL    , disNULL   , \
	disNULL     , disNULL     , disNULL    , disNULL   , /* 0x10 */  \
	disNULL     , disNULL     , disNULL    , disNULL   ,  \
	disNULL     , disNULL     , disNULL    , disNULL   ,  \
	disNULL     , disNULL     , disNULL    , disNULL   ,  \
	disNULL     , disNULL     , disNULL    , disNULL   , /* 0x20 */  \
	disNULL     , disNULL     , disNULL    , disNULL   ,  \
	disNULL     , disNULL     , disNULL    , disNULL   ,  \
	disNULL     , disNULL     , disNULL    , disNULL   ,  \
	dis##VU##MI_IADD  , dis##VU##MI_ISUB  , dis##VU##MI_IADDI, disNULL   , /* 0x30 */ \
	dis##VU##MI_IAND  , dis##VU##MI_IOR   , disNULL    , disNULL   ,  \
	disNULL     , disNULL     , disNULL    , disNULL   ,  \
	dis##VU##LowerOP_T3_00, dis##VU##LowerOP_T3_01, dis##VU##LowerOP_T3_10, dis##VU##LowerOP_T3_11,  \
}; \
 \
MakeDisF(dis##VU##LowerOP,		dis##VU##LowerOP_OPCODE[code & 0x3f] DisFInterfaceN) \
 \
TdisR5900F dis##VU##MicroL[] = { \
	dis##VU##MI_LQ    , dis##VU##MI_SQ    , disNULL    , disNULL,  \
	dis##VU##MI_ILW   , dis##VU##MI_ISW   , disNULL    , disNULL,  \
	dis##VU##MI_IADDIU, dis##VU##MI_ISUBIU, disNULL    , disNULL,  \
	disNULL     , disNULL     , disNULL    , disNULL, \
	dis##VU##MI_FCEQ  , dis##VU##MI_FCSET , dis##VU##MI_FCAND, dis##VU##MI_FCOR, /* 0x10 */ \
	dis##VU##MI_FSEQ  , dis##VU##MI_FSSET , dis##VU##MI_FSAND, dis##VU##MI_FSOR, \
	dis##VU##MI_FMEQ  , disNULL     , dis##VU##MI_FMAND, dis##VU##MI_FMOR, \
	dis##VU##MI_FCGET , disNULL     , disNULL    , disNULL, \
	dis##VU##MI_B     , dis##VU##MI_BAL   , disNULL    , disNULL, /* 0x20 */  \
	dis##VU##MI_JR    , dis##VU##MI_JALR  , disNULL    , disNULL, \
	dis##VU##MI_IBEQ  , dis##VU##MI_IBNE  , disNULL    , disNULL, \
	dis##VU##MI_IBLTZ , dis##VU##MI_IBGTZ , dis##VU##MI_IBLEZ, dis##VU##MI_IBGEZ, \
	disNULL     , disNULL     , disNULL    , disNULL, /* 0x30 */ \
	disNULL     , disNULL     , disNULL    , disNULL,  \
	disNULL     , disNULL     , disNULL    , disNULL,  \
	disNULL     , disNULL     , disNULL    , disNULL,  \
	dis##VU##LowerOP  , disNULL     , disNULL    , disNULL, /* 0x40*/  \
	disNULL     , disNULL     , disNULL    , disNULL,  \
	disNULL     , disNULL     , disNULL    , disNULL,  \
	disNULL     , disNULL     , disNULL    , disNULL,  \
	disNULL     , disNULL     , disNULL    , disNULL, /* 0x50 */ \
	disNULL     , disNULL     , disNULL    , disNULL,  \
	disNULL     , disNULL     , disNULL    , disNULL,  \
	disNULL     , disNULL     , disNULL    , disNULL,  \
	disNULL     , disNULL     , disNULL    , disNULL, /* 0x60 */ \
	disNULL     , disNULL     , disNULL    , disNULL,  \
	disNULL     , disNULL     , disNULL    , disNULL,  \
	disNULL     , disNULL     , disNULL    , disNULL,  \
	disNULL     , disNULL     , disNULL    , disNULL, /* 0x70 */ \
	disNULL     , disNULL     , disNULL    , disNULL,  \
	disNULL     , disNULL     , disNULL    , disNULL,  \
	disNULL     , disNULL     , disNULL    , disNULL,  \
}; \
 \
 \
MakeDisF(dis##VU##MicroLF,		dis##VU##MicroL[code >> 25] DisFInterfaceN) \
 \
 \
/****************/ \
/* UPPER TABLES */ \
/****************/ \
 \
TdisR5900F dis##VU##_UPPER_FD_00_TABLE[32] = { \
	dis##VU##MI_ADDAx, dis##VU##MI_SUBx , dis##VU##MI_MADDAx, dis##VU##MI_MSUBAx, \
	dis##VU##MI_ITOF0, dis##VU##MI_FTOI0, dis##VU##MI_MULAx , dis##VU##MI_MULAq , \
	dis##VU##MI_ADDAq, dis##VU##MI_SUBAq, dis##VU##MI_ADDA  , dis##VU##MI_SUBA  , \
	disNULL    , disNULL    , disNULL     , disNULL     , \
	disNULL    , disNULL    , disNULL     , disNULL     , \
	disNULL    , disNULL    , disNULL     , disNULL     , \
	disNULL    , disNULL    , disNULL     , disNULL     , \
	disNULL    , disNULL    , disNULL     , disNULL     , \
}; \
 \
TdisR5900F dis##VU##_UPPER_FD_01_TABLE[32] = { \
	dis##VU##MI_ADDAy , dis##VU##MI_SUBy  , dis##VU##MI_MADDAy, dis##VU##MI_MSUBAy, \
	dis##VU##MI_ITOF4 , dis##VU##MI_FTOI4 , dis##VU##MI_MULAy , dis##VU##MI_ABS   , \
	dis##VU##MI_MADDAq, dis##VU##MI_MSUBAq, dis##VU##MI_MADDA , dis##VU##MI_MSUBA , \
	disNULL     , disNULL     , disNULL     , disNULL     , \
	disNULL     , disNULL     , disNULL     , disNULL     , \
	disNULL     , disNULL     , disNULL     , disNULL     , \
	disNULL     , disNULL     , disNULL     , disNULL     , \
	disNULL     , disNULL     , disNULL     , disNULL     , \
}; \
 \
TdisR5900F dis##VU##_UPPER_FD_10_TABLE[32] = { \
	dis##VU##MI_ADDAz , dis##VU##MI_SUBz  , dis##VU##MI_MADDAz, dis##VU##MI_MSUBAz, \
	dis##VU##MI_ITOF12, dis##VU##MI_FTOI12, dis##VU##MI_MULAz , dis##VU##MI_MULAi , \
	dis##VU##MI_ADDAi, dis##VU##MI_SUBAi , dis##VU##MI_MULA  , dis##VU##MI_OPMULA, \
	disNULL     , disNULL     , disNULL     , disNULL     , \
	disNULL     , disNULL     , disNULL     , disNULL     , \
	disNULL     , disNULL     , disNULL     , disNULL     , \
	disNULL     , disNULL     , disNULL     , disNULL     , \
	disNULL     , disNULL     , disNULL     , disNULL     , \
}; \
 \
TdisR5900F dis##VU##_UPPER_FD_11_TABLE[32] = { \
	dis##VU##MI_ADDAw , dis##VU##MI_SUBw  , dis##VU##MI_MADDAw, dis##VU##MI_MSUBAw, \
	dis##VU##MI_ITOF15, dis##VU##MI_FTOI15, dis##VU##MI_MULAw , dis##VU##MI_CLIP  , \
	dis##VU##MI_MADDAi, dis##VU##MI_MSUBAi, disNULL     , dis##VU##MI_NOP   , \
	disNULL     , disNULL     , disNULL     , disNULL     , \
	disNULL     , disNULL     , disNULL     , disNULL     , \
	disNULL     , disNULL     , disNULL     , disNULL     , \
	disNULL     , disNULL     , disNULL     , disNULL     , \
	disNULL     , disNULL     , disNULL     , disNULL     , \
}; \
 \
MakeDisF(dis##VU##_UPPER_FD_00,	dis##VU##_UPPER_FD_00_TABLE[_Fd_] DisFInterfaceN) \
MakeDisF(dis##VU##_UPPER_FD_01,	dis##VU##_UPPER_FD_01_TABLE[_Fd_] DisFInterfaceN) \
MakeDisF(dis##VU##_UPPER_FD_10,	dis##VU##_UPPER_FD_10_TABLE[_Fd_] DisFInterfaceN) \
MakeDisF(dis##VU##_UPPER_FD_11,	dis##VU##_UPPER_FD_11_TABLE[_Fd_] DisFInterfaceN) \
 \
TdisR5900F dis##VU##MicroU[] = { \
	dis##VU##MI_ADDx  , dis##VU##MI_ADDy  , dis##VU##MI_ADDz  , dis##VU##MI_ADDw, \
	dis##VU##MI_SUBx  , dis##VU##MI_SUBy  , dis##VU##MI_SUBz  , dis##VU##MI_SUBw, \
	dis##VU##MI_MADDx , dis##VU##MI_MADDy , dis##VU##MI_MADDz , dis##VU##MI_MADDw, \
	dis##VU##MI_MSUBx , dis##VU##MI_MSUBy , dis##VU##MI_MSUBz , dis##VU##MI_MSUBw, \
	dis##VU##MI_MAXx  , dis##VU##MI_MAXy  , dis##VU##MI_MAXz  , dis##VU##MI_MAXw,  /* 0x10 */  \
	dis##VU##MI_MINIx , dis##VU##MI_MINIy , dis##VU##MI_MINIz , dis##VU##MI_MINIw, \
	dis##VU##MI_MULx  , dis##VU##MI_MULy  , dis##VU##MI_MULz  , dis##VU##MI_MULw, \
	dis##VU##MI_MULq  , dis##VU##MI_MAXi  , dis##VU##MI_MULi  , dis##VU##MI_MINIi, \
	dis##VU##MI_ADDq  , dis##VU##MI_MADDq , dis##VU##MI_ADDi  , dis##VU##MI_MADDi, /* 0x20 */ \
	dis##VU##MI_SUBq  , dis##VU##MI_MSUBq , dis##VU##MI_SUBi  , dis##VU##MI_MSUBi, \
	dis##VU##MI_ADD   , dis##VU##MI_MADD  , dis##VU##MI_MUL   , dis##VU##MI_MAX, \
	dis##VU##MI_SUB   , dis##VU##MI_MSUB  , dis##VU##MI_OPMSUB, dis##VU##MI_MINI, \
	disNULL     , disNULL     , disNULL     , disNULL   ,  /* 0x30 */ \
	disNULL     , disNULL     , disNULL     , disNULL   , \
	disNULL     , disNULL     , disNULL     , disNULL   , \
	dis##VU##_UPPER_FD_00, dis##VU##_UPPER_FD_01, dis##VU##_UPPER_FD_10, dis##VU##_UPPER_FD_11,  \
}; \
 \
 \
MakeDisF(dis##VU##MicroUF,	dis##VU##MicroU[code & 0x3f] DisFInterfaceN) \

