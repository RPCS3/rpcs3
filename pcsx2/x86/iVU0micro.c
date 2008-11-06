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

// stop compiling if NORECBUILD build (only for Visual Studio)
#if !(defined(_MSC_VER) && defined(PCSX2_NORECBUILD))

#include <stdlib.h>
#include <string.h>

#include "Common.h"
#include "InterTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"
#include "iMMI.h"
#include "iFPU.h"
#include "iCP0.h"
#include "VUmicro.h"
#include "iVUmicro.h"
#include "iVU0micro.h"
#include "iVUops.h"
#include "VUops.h"

#include "iVUzerorec.h"

#ifdef _WIN32
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

static VURegs * const VU = (VURegs*)&VU0;

//u32 vu0time = 0;
//static LARGE_INTEGER vu0base, vu0final;

#ifdef _DEBUG
extern u32 vudump;
#endif

static u32 vuprogcount = 0;

void recExecuteVU0Block( void )
{
	//SysPrintf("executeVU0 %x\n", VU0.VI[ REG_TPC ].UL);
	//QueryPerformanceCounter(&vu0base);

//	assert( VU0.VI[REG_VPU_STAT].UL & 1 );
	if((VU0.VI[REG_VPU_STAT].UL & 1) == 0){
		//SysPrintf("Execute block VU0, VU0 not busy\n");
		return;
	}
#ifdef _DEBUG
	vuprogcount++;

//	__Log("VU: %x %x\n", VU0.VI[ REG_TPC ].UL, vuprogcount);
//	iDumpVU0Registers();

	//vudump |= 0x10;
	//vudump |= 0x80;

	if( (vudump&0x80) && !CHECK_VU0REC ) {
		__Log("tVU: %x\n", VU0.VI[ REG_TPC ].UL);
		iDumpVU0Registers();
	}
#endif

	//while( (VU0.VI[ REG_VPU_STAT ].UL&1) ) {
		if( CHECK_VU0REC) {		
			FreezeXMMRegs(1);
			SuperVUExecuteProgram(VU0.VI[ REG_TPC ].UL&0xfff, 0);
			FreezeXMMRegs(0);
		}
		else {
			intExecuteVU0Block();
		}
	//}

//	__Log("eVU: %x %x\n", VU0.VI[ REG_TPC ].UL, vuprogcount);
//	iDumpVU0Registers();
//	QueryPerformanceCounter(&vu0final);
//	vu0time += (u32)(vu0final.QuadPart-vu0final.QuadPart);
}

void recClearVU0( u32 Addr, u32 Size )
{
	if( CHECK_VU0REC ) {
		SuperVUClear(Addr, Size*4, 0);
	}
}

void (*recVU0_LOWER_OPCODE[128])() = { 
	recVU0MI_LQ    , recVU0MI_SQ    , recVU0unknown , recVU0unknown,  
	recVU0MI_ILW   , recVU0MI_ISW   , recVU0unknown , recVU0unknown,  
	recVU0MI_IADDIU, recVU0MI_ISUBIU, recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, 
	recVU0MI_FCEQ  , recVU0MI_FCSET , recVU0MI_FCAND, recVU0MI_FCOR, /* 0x10 */ 
	recVU0MI_FSEQ  , recVU0MI_FSSET , recVU0MI_FSAND, recVU0MI_FSOR, 
	recVU0MI_FMEQ  , recVU0unknown  , recVU0MI_FMAND, recVU0MI_FMOR, 
	recVU0MI_FCGET , recVU0unknown  , recVU0unknown , recVU0unknown, 
	recVU0MI_B     , recVU0MI_BAL   , recVU0unknown , recVU0unknown, /* 0x20 */  
	recVU0MI_JR    , recVU0MI_JALR  , recVU0unknown , recVU0unknown, 
	recVU0MI_IBEQ  , recVU0MI_IBNE  , recVU0unknown , recVU0unknown, 
	recVU0MI_IBLTZ , recVU0MI_IBGTZ , recVU0MI_IBLEZ, recVU0MI_IBGEZ, 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x30 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0LowerOP  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x40*/  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x50 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x60 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x70 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
}; 
 
void (*recVU0LowerOP_T3_00_OPCODE[32])() = { 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0MI_MOVE  , recVU0MI_LQI   , recVU0MI_DIV  , recVU0MI_MTIR,  
	recVU0MI_RNEXT , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x10 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0MI_MFP   , recVU0unknown , recVU0unknown,  
	recVU0MI_ESADD , recVU0MI_EATANxy, recVU0MI_ESQRT, recVU0MI_ESIN,  
}; 
 
void (*recVU0LowerOP_T3_01_OPCODE[32])() = { 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0MI_MR32  , recVU0MI_SQI   , recVU0MI_SQRT , recVU0MI_MFIR,  
	recVU0MI_RGET  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x10 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0MI_XITOP, recVU0unknown,  
	recVU0MI_ERSADD, recVU0MI_EATANxz, recVU0MI_ERSQRT, recVU0MI_EATAN, 
}; 
 
void (*recVU0LowerOP_T3_10_OPCODE[32])() = { 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0MI_LQD   , recVU0MI_RSQRT, recVU0MI_ILWR,  
	recVU0MI_RINIT , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x10 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0MI_ELENG , recVU0MI_ESUM  , recVU0MI_ERCPR, recVU0MI_EEXP,  
}; 
 
void (*recVU0LowerOP_T3_11_OPCODE[32])() = { 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0MI_SQD   , recVU0MI_WAITQ, recVU0MI_ISWR,  
	recVU0MI_RXOR  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x10 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0MI_ERLENG, recVU0unknown  , recVU0MI_WAITP, recVU0unknown,  
}; 
 
void (*recVU0LowerOP_OPCODE[64])() = { 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x10 */  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x20 */  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0MI_IADD  , recVU0MI_ISUB  , recVU0MI_IADDI, recVU0unknown, /* 0x30 */ 
	recVU0MI_IAND  , recVU0MI_IOR   , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0LowerOP_T3_00, recVU0LowerOP_T3_01, recVU0LowerOP_T3_10, recVU0LowerOP_T3_11,  
}; 
 
void (*recVU0_UPPER_OPCODE[64])() = { 
	recVU0MI_ADDx  , recVU0MI_ADDy  , recVU0MI_ADDz  , recVU0MI_ADDw, 
	recVU0MI_SUBx  , recVU0MI_SUBy  , recVU0MI_SUBz  , recVU0MI_SUBw, 
	recVU0MI_MADDx , recVU0MI_MADDy , recVU0MI_MADDz , recVU0MI_MADDw, 
	recVU0MI_MSUBx , recVU0MI_MSUBy , recVU0MI_MSUBz , recVU0MI_MSUBw, 
	recVU0MI_MAXx  , recVU0MI_MAXy  , recVU0MI_MAXz  , recVU0MI_MAXw,  /* 0x10 */  
	recVU0MI_MINIx , recVU0MI_MINIy , recVU0MI_MINIz , recVU0MI_MINIw, 
	recVU0MI_MULx  , recVU0MI_MULy  , recVU0MI_MULz  , recVU0MI_MULw, 
	recVU0MI_MULq  , recVU0MI_MAXi  , recVU0MI_MULi  , recVU0MI_MINIi, 
	recVU0MI_ADDq  , recVU0MI_MADDq , recVU0MI_ADDi  , recVU0MI_MADDi, /* 0x20 */ 
	recVU0MI_SUBq  , recVU0MI_MSUBq , recVU0MI_SUBi  , recVU0MI_MSUBi, 
	recVU0MI_ADD   , recVU0MI_MADD  , recVU0MI_MUL   , recVU0MI_MAX, 
	recVU0MI_SUB   , recVU0MI_MSUB  , recVU0MI_OPMSUB, recVU0MI_MINI, 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown,  /* 0x30 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown, 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown, 
	recVU0_UPPER_FD_00, recVU0_UPPER_FD_01, recVU0_UPPER_FD_10, recVU0_UPPER_FD_11,  
}; 
 
void (*recVU0_UPPER_FD_00_TABLE[32])() = { 
	recVU0MI_ADDAx, recVU0MI_SUBAx , recVU0MI_MADDAx, recVU0MI_MSUBAx, 
	recVU0MI_ITOF0, recVU0MI_FTOI0, recVU0MI_MULAx , recVU0MI_MULAq , 
	recVU0MI_ADDAq, recVU0MI_SUBAq, recVU0MI_ADDA  , recVU0MI_SUBA  , 
	recVU0unknown , recVU0unknown , recVU0unknown  , recVU0unknown  , 
	recVU0unknown , recVU0unknown , recVU0unknown  , recVU0unknown  , 
	recVU0unknown , recVU0unknown , recVU0unknown  , recVU0unknown  , 
	recVU0unknown , recVU0unknown , recVU0unknown  , recVU0unknown  , 
	recVU0unknown , recVU0unknown , recVU0unknown  , recVU0unknown  , 
}; 
 
void (*recVU0_UPPER_FD_01_TABLE[32])() = { 
	recVU0MI_ADDAy , recVU0MI_SUBAy  , recVU0MI_MADDAy, recVU0MI_MSUBAy, 
	recVU0MI_ITOF4 , recVU0MI_FTOI4 , recVU0MI_MULAy , recVU0MI_ABS   , 
	recVU0MI_MADDAq, recVU0MI_MSUBAq, recVU0MI_MADDA , recVU0MI_MSUBA , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
}; 
 
void (*recVU0_UPPER_FD_10_TABLE[32])() = { 
	recVU0MI_ADDAz , recVU0MI_SUBAz  , recVU0MI_MADDAz, recVU0MI_MSUBAz, 
	recVU0MI_ITOF12, recVU0MI_FTOI12, recVU0MI_MULAz , recVU0MI_MULAi , 
	recVU0MI_ADDAi, recVU0MI_SUBAi , recVU0MI_MULA  , recVU0MI_OPMULA, 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
}; 
 
void (*recVU0_UPPER_FD_11_TABLE[32])() = { 
	recVU0MI_ADDAw , recVU0MI_SUBAw  , recVU0MI_MADDAw, recVU0MI_MSUBAw, 
	recVU0MI_ITOF15, recVU0MI_FTOI15, recVU0MI_MULAw , recVU0MI_CLIP  , 
	recVU0MI_MADDAi, recVU0MI_MSUBAi, recVU0unknown  , recVU0MI_NOP   , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
}; 

void recVU0_UPPER_FD_00( void )
{ 
	recVU0_UPPER_FD_00_TABLE[ ( VU0.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU0_UPPER_FD_01( void ) 
{ 
	recVU0_UPPER_FD_01_TABLE[ ( VU0.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU0_UPPER_FD_10( void )
{ 
	recVU0_UPPER_FD_10_TABLE[ ( VU0.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU0_UPPER_FD_11( void )
{ 
	recVU0_UPPER_FD_11_TABLE[ ( VU0.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU0LowerOP( void )
{ 
	recVU0LowerOP_OPCODE[ VU0.code & 0x3f ]( ); 
} 
 
void recVU0LowerOP_T3_00( void )
{ 
	recVU0LowerOP_T3_00_OPCODE[ ( VU0.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU0LowerOP_T3_01( void )
{ 
	recVU0LowerOP_T3_01_OPCODE[ ( VU0.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU0LowerOP_T3_10( void )
{ 
	recVU0LowerOP_T3_10_OPCODE[ ( VU0.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU0LowerOP_T3_11( void )
{ 
	recVU0LowerOP_T3_11_OPCODE[ ( VU0.code >> 6 ) & 0x1f ]( ); 
}


void recVU0unknown( void )
{ 
#ifdef CPU_LOG
	CPU_LOG("Unknown VU0 micromode opcode calledn"); 
#endif
}  
 
 
 
/****************************************/ 
/*   VU Micromode Upper instructions    */ 
/****************************************/ 

#ifdef RECOMPILE_VUMI_ABS
void recVU0MI_ABS()   { recVUMI_ABS(VU, VUREC_INFO); } 
#else
void recVU0MI_ABS()   { REC_VUOP(VU0, ABS); } 
#endif

#ifdef RECOMPILE_VUMI_ADD
void recVU0MI_ADD()  { recVUMI_ADD(VU, VUREC_INFO); } 
void recVU0MI_ADDi() { recVUMI_ADDi(VU, VUREC_INFO); } 
void recVU0MI_ADDq() { recVUMI_ADDq(VU, VUREC_INFO); } 
void recVU0MI_ADDx() { recVUMI_ADDx(VU, VUREC_INFO); } 
void recVU0MI_ADDy() { recVUMI_ADDy(VU, VUREC_INFO); } 
void recVU0MI_ADDz() { recVUMI_ADDz(VU, VUREC_INFO); } 
void recVU0MI_ADDw() { recVUMI_ADDw(VU, VUREC_INFO); } 
#else
void recVU0MI_ADD()   { REC_VUOP(VU0, ADD); } 
void recVU0MI_ADDi()  { REC_VUOP(VU0, ADDi); } 
void recVU0MI_ADDq()  { REC_VUOP(VU0, ADDq); } 
void recVU0MI_ADDx()  { REC_VUOP(VU0, ADDx); } 
void recVU0MI_ADDy()  { REC_VUOP(VU0, ADDy); } 
void recVU0MI_ADDz()  { REC_VUOP(VU0, ADDz); } 
void recVU0MI_ADDw()  { REC_VUOP(VU0, ADDw); } 
#endif

#ifdef RECOMPILE_VUMI_ADDA
void recVU0MI_ADDA()  { recVUMI_ADDA(VU, VUREC_INFO); } 
void recVU0MI_ADDAi() { recVUMI_ADDAi(VU, VUREC_INFO); } 
void recVU0MI_ADDAq() { recVUMI_ADDAq(VU, VUREC_INFO); } 
void recVU0MI_ADDAx() { recVUMI_ADDAx(VU, VUREC_INFO); } 
void recVU0MI_ADDAy() { recVUMI_ADDAy(VU, VUREC_INFO); } 
void recVU0MI_ADDAz() { recVUMI_ADDAz(VU, VUREC_INFO); } 
void recVU0MI_ADDAw() { recVUMI_ADDAw(VU, VUREC_INFO); } 
#else
void recVU0MI_ADDA()  { REC_VUOP(VU0, ADDA); } 
void recVU0MI_ADDAi() { REC_VUOP(VU0, ADDAi); } 
void recVU0MI_ADDAq() { REC_VUOP(VU0, ADDAq); } 
void recVU0MI_ADDAx() { REC_VUOP(VU0, ADDAx); } 
void recVU0MI_ADDAy() { REC_VUOP(VU0, ADDAy); } 
void recVU0MI_ADDAz() { REC_VUOP(VU0, ADDAz); } 
void recVU0MI_ADDAw() { REC_VUOP(VU0, ADDAw); } 
#endif

#ifdef RECOMPILE_VUMI_SUB
void recVU0MI_SUB()  { recVUMI_SUB(VU, VUREC_INFO); } 
void recVU0MI_SUBi() { recVUMI_SUBi(VU, VUREC_INFO); } 
void recVU0MI_SUBq() { recVUMI_SUBq(VU, VUREC_INFO); } 
void recVU0MI_SUBx() { recVUMI_SUBx(VU, VUREC_INFO); } 
void recVU0MI_SUBy() { recVUMI_SUBy(VU, VUREC_INFO); } 
void recVU0MI_SUBz() { recVUMI_SUBz(VU, VUREC_INFO); } 
void recVU0MI_SUBw() { recVUMI_SUBw(VU, VUREC_INFO); } 
#else
void recVU0MI_SUB()   { REC_VUOP(VU0, SUB); } 
void recVU0MI_SUBi()  { REC_VUOP(VU0, SUBi); } 
void recVU0MI_SUBq()  { REC_VUOP(VU0, SUBq); } 
void recVU0MI_SUBx()  { REC_VUOP(VU0, SUBx); } 
void recVU0MI_SUBy()  { REC_VUOP(VU0, SUBy); } 
void recVU0MI_SUBz()  { REC_VUOP(VU0, SUBz); } 
void recVU0MI_SUBw()  { REC_VUOP(VU0, SUBw); } 
#endif

#ifdef RECOMPILE_VUMI_SUBA
void recVU0MI_SUBA()  { recVUMI_SUBA(VU, VUREC_INFO); } 
void recVU0MI_SUBAi() { recVUMI_SUBAi(VU, VUREC_INFO); } 
void recVU0MI_SUBAq() { recVUMI_SUBAq(VU, VUREC_INFO); } 
void recVU0MI_SUBAx() { recVUMI_SUBAx(VU, VUREC_INFO); } 
void recVU0MI_SUBAy() { recVUMI_SUBAy(VU, VUREC_INFO); } 
void recVU0MI_SUBAz() { recVUMI_SUBAz(VU, VUREC_INFO); } 
void recVU0MI_SUBAw() { recVUMI_SUBAw(VU, VUREC_INFO); } 
#else
void recVU0MI_SUBA()  { REC_VUOP(VU0, SUBA); } 
void recVU0MI_SUBAi() { REC_VUOP(VU0, SUBAi); } 
void recVU0MI_SUBAq() { REC_VUOP(VU0, SUBAq); } 
void recVU0MI_SUBAx() { REC_VUOP(VU0, SUBAx); } 
void recVU0MI_SUBAy() { REC_VUOP(VU0, SUBAy); } 
void recVU0MI_SUBAz() { REC_VUOP(VU0, SUBAz); } 
void recVU0MI_SUBAw() { REC_VUOP(VU0, SUBAw); } 
#endif

#ifdef RECOMPILE_VUMI_MUL
void recVU0MI_MUL()  { recVUMI_MUL(VU, VUREC_INFO); } 
void recVU0MI_MULi() { recVUMI_MULi(VU, VUREC_INFO); } 
void recVU0MI_MULq() { recVUMI_MULq(VU, VUREC_INFO); }
void recVU0MI_MULx() { recVUMI_MULx(VU, VUREC_INFO); } 
void recVU0MI_MULy() { recVUMI_MULy(VU, VUREC_INFO); } 
void recVU0MI_MULz() { recVUMI_MULz(VU, VUREC_INFO); } 
void recVU0MI_MULw() { recVUMI_MULw(VU, VUREC_INFO); } 
#else
void recVU0MI_MUL()   { REC_VUOP(VU0, MUL); } 
void recVU0MI_MULi()  { REC_VUOP(VU0, MULi); } 
void recVU0MI_MULq()  { REC_VUOP(VU0, MULq); } 
void recVU0MI_MULx()  { REC_VUOP(VU0, MULx); } 
void recVU0MI_MULy()  { REC_VUOP(VU0, MULy); } 
void recVU0MI_MULz()  { REC_VUOP(VU0, MULz); } 
void recVU0MI_MULw()  { REC_VUOP(VU0, MULw); } 
#endif

#ifdef RECOMPILE_VUMI_MULA
void recVU0MI_MULA()  { recVUMI_MULA(VU, VUREC_INFO); } 
void recVU0MI_MULAi() { recVUMI_MULAi(VU, VUREC_INFO); } 
void recVU0MI_MULAq() { recVUMI_MULAq(VU, VUREC_INFO); } 
void recVU0MI_MULAx() { recVUMI_MULAx(VU, VUREC_INFO); } 
void recVU0MI_MULAy() { recVUMI_MULAy(VU, VUREC_INFO); } 
void recVU0MI_MULAz() { recVUMI_MULAz(VU, VUREC_INFO); } 
void recVU0MI_MULAw() { recVUMI_MULAw(VU, VUREC_INFO); } 
#else
void recVU0MI_MULA()  { REC_VUOP(VU0, MULA); } 
void recVU0MI_MULAi() { REC_VUOP(VU0, MULAi); } 
void recVU0MI_MULAq() { REC_VUOP(VU0, MULAq); } 
void recVU0MI_MULAx() { REC_VUOP(VU0, MULAx); } 
void recVU0MI_MULAy() { REC_VUOP(VU0, MULAy); } 
void recVU0MI_MULAz() { REC_VUOP(VU0, MULAz); } 
void recVU0MI_MULAw() { REC_VUOP(VU0, MULAw); } 
#endif

#ifdef RECOMPILE_VUMI_MADD
void recVU0MI_MADD()  { recVUMI_MADD(VU, VUREC_INFO); } 
void recVU0MI_MADDi() { recVUMI_MADDi(VU, VUREC_INFO); } 
void recVU0MI_MADDq() { recVUMI_MADDq(VU, VUREC_INFO); } 
void recVU0MI_MADDx() { recVUMI_MADDx(VU, VUREC_INFO); } 
void recVU0MI_MADDy() { recVUMI_MADDy(VU, VUREC_INFO); } 
void recVU0MI_MADDz() { recVUMI_MADDz(VU, VUREC_INFO); } 
void recVU0MI_MADDw() { recVUMI_MADDw(VU, VUREC_INFO); } 
#else
void recVU0MI_MADD()  { REC_VUOP(VU0, MADD); } 
void recVU0MI_MADDi() { REC_VUOP(VU0, MADDi); } 
void recVU0MI_MADDq() { REC_VUOP(VU0, MADDq); } 
void recVU0MI_MADDx() { REC_VUOP(VU0, MADDx); } 
void recVU0MI_MADDy() { REC_VUOP(VU0, MADDy); } 
void recVU0MI_MADDz() { REC_VUOP(VU0, MADDz); } 
void recVU0MI_MADDw() { REC_VUOP(VU0, MADDw); } 
#endif

#ifdef RECOMPILE_VUMI_MADDA
void recVU0MI_MADDA()  { recVUMI_MADDA(VU, VUREC_INFO); } 
void recVU0MI_MADDAi() { recVUMI_MADDAi(VU, VUREC_INFO); } 
void recVU0MI_MADDAq() { recVUMI_MADDAq(VU, VUREC_INFO); } 
void recVU0MI_MADDAx() { recVUMI_MADDAx(VU, VUREC_INFO); } 
void recVU0MI_MADDAy() { recVUMI_MADDAy(VU, VUREC_INFO); } 
void recVU0MI_MADDAz() { recVUMI_MADDAz(VU, VUREC_INFO); } 
void recVU0MI_MADDAw() { recVUMI_MADDAw(VU, VUREC_INFO); } 
#else
void recVU0MI_MADDA()  { REC_VUOP(VU0, MADDA); } 
void recVU0MI_MADDAi() { REC_VUOP(VU0, MADDAi); } 
void recVU0MI_MADDAq() { REC_VUOP(VU0, MADDAq); } 
void recVU0MI_MADDAx() { REC_VUOP(VU0, MADDAx); } 
void recVU0MI_MADDAy() { REC_VUOP(VU0, MADDAy); } 
void recVU0MI_MADDAz() { REC_VUOP(VU0, MADDAz); } 
void recVU0MI_MADDAw() { REC_VUOP(VU0, MADDAw); } 
#endif

#ifdef RECOMPILE_VUMI_MSUB
void recVU0MI_MSUB()  { recVUMI_MSUB(VU, VUREC_INFO); } 
void recVU0MI_MSUBi() { recVUMI_MSUBi(VU, VUREC_INFO); } 
void recVU0MI_MSUBq() { recVUMI_MSUBq(VU, VUREC_INFO); } 
void recVU0MI_MSUBx() { recVUMI_MSUBx(VU, VUREC_INFO); } 
void recVU0MI_MSUBy() { recVUMI_MSUBy(VU, VUREC_INFO); } 
void recVU0MI_MSUBz() { recVUMI_MSUBz(VU, VUREC_INFO); } 
void recVU0MI_MSUBw() { recVUMI_MSUBw(VU, VUREC_INFO); } 
#else
void recVU0MI_MSUB()  { REC_VUOP(VU0, MSUB); } 
void recVU0MI_MSUBi() { REC_VUOP(VU0, MSUBi); } 
void recVU0MI_MSUBq() { REC_VUOP(VU0, MSUBq); } 
void recVU0MI_MSUBx() { REC_VUOP(VU0, MSUBx); } 
void recVU0MI_MSUBy() { REC_VUOP(VU0, MSUBy); } 
void recVU0MI_MSUBz() { REC_VUOP(VU0, MSUBz); } 
void recVU0MI_MSUBw() { REC_VUOP(VU0, MSUBw); } 
#endif

#ifdef RECOMPILE_VUMI_MSUBA
void recVU0MI_MSUBA()  { recVUMI_MSUBA(VU, VUREC_INFO); } 
void recVU0MI_MSUBAi() { recVUMI_MSUBAi(VU, VUREC_INFO); } 
void recVU0MI_MSUBAq() { recVUMI_MSUBAq(VU, VUREC_INFO); } 
void recVU0MI_MSUBAx() { recVUMI_MSUBAx(VU, VUREC_INFO); } 
void recVU0MI_MSUBAy() { recVUMI_MSUBAy(VU, VUREC_INFO); } 
void recVU0MI_MSUBAz() { recVUMI_MSUBAz(VU, VUREC_INFO); } 
void recVU0MI_MSUBAw() { recVUMI_MSUBAw(VU, VUREC_INFO); } 
#else
void recVU0MI_MSUBA()  { REC_VUOP(VU0, MSUBA); } 
void recVU0MI_MSUBAi() { REC_VUOP(VU0, MSUBAi); } 
void recVU0MI_MSUBAq() { REC_VUOP(VU0, MSUBAq); } 
void recVU0MI_MSUBAx() { REC_VUOP(VU0, MSUBAx); } 
void recVU0MI_MSUBAy() { REC_VUOP(VU0, MSUBAy); } 
void recVU0MI_MSUBAz() { REC_VUOP(VU0, MSUBAz); } 
void recVU0MI_MSUBAw() { REC_VUOP(VU0, MSUBAw); } 
#endif

#ifdef RECOMPILE_VUMI_MAX
void recVU0MI_MAX()  { recVUMI_MAX(VU, VUREC_INFO); } 
void recVU0MI_MAXi() { recVUMI_MAXi(VU, VUREC_INFO); } 
void recVU0MI_MAXx() { recVUMI_MAXx(VU, VUREC_INFO); } 
void recVU0MI_MAXy() { recVUMI_MAXy(VU, VUREC_INFO); } 
void recVU0MI_MAXz() { recVUMI_MAXz(VU, VUREC_INFO); } 
void recVU0MI_MAXw() { recVUMI_MAXw(VU, VUREC_INFO); } 
#else
void recVU0MI_MAX()  { REC_VUOP(VU0, MAX); } 
void recVU0MI_MAXi() { REC_VUOP(VU0, MAXi); } 
void recVU0MI_MAXx() { REC_VUOP(VU0, MAXx); } 
void recVU0MI_MAXy() { REC_VUOP(VU0, MAXy); } 
void recVU0MI_MAXz() { REC_VUOP(VU0, MAXz); } 
void recVU0MI_MAXw() { REC_VUOP(VU0, MAXw); } 
#endif

#ifdef RECOMPILE_VUMI_MINI
void recVU0MI_MINI()  { recVUMI_MINI(VU, VUREC_INFO); } 
void recVU0MI_MINIi() { recVUMI_MINIi(VU, VUREC_INFO); } 
void recVU0MI_MINIx() { recVUMI_MINIx(VU, VUREC_INFO); } 
void recVU0MI_MINIy() { recVUMI_MINIy(VU, VUREC_INFO); } 
void recVU0MI_MINIz() { recVUMI_MINIz(VU, VUREC_INFO); } 
void recVU0MI_MINIw() { recVUMI_MINIw(VU, VUREC_INFO); }
#else
void recVU0MI_MINI()  { REC_VUOP(VU0, MINI); } 
void recVU0MI_MINIi() { REC_VUOP(VU0, MINIi); } 
void recVU0MI_MINIx() { REC_VUOP(VU0, MINIx); } 
void recVU0MI_MINIy() { REC_VUOP(VU0, MINIy); } 
void recVU0MI_MINIz() { REC_VUOP(VU0, MINIz); } 
void recVU0MI_MINIw() { REC_VUOP(VU0, MINIw); } 
#endif

#ifdef RECOMPILE_VUMI_FTOI
void recVU0MI_FTOI0()  { recVUMI_FTOI0(VU, VUREC_INFO); }
void recVU0MI_FTOI4()  { recVUMI_FTOI4(VU, VUREC_INFO); } 
void recVU0MI_FTOI12() { recVUMI_FTOI12(VU, VUREC_INFO); } 
void recVU0MI_FTOI15() { recVUMI_FTOI15(VU, VUREC_INFO); } 
void recVU0MI_ITOF0()  { recVUMI_ITOF0(VU, VUREC_INFO); } 
void recVU0MI_ITOF4()  { recVUMI_ITOF4(VU, VUREC_INFO); } 
void recVU0MI_ITOF12() { recVUMI_ITOF12(VU, VUREC_INFO); } 
void recVU0MI_ITOF15() { recVUMI_ITOF15(VU, VUREC_INFO); } 
#else
void recVU0MI_FTOI0()  { REC_VUOP(VU0, FTOI0); } 
void recVU0MI_FTOI4()  { REC_VUOP(VU0, FTOI4); } 
void recVU0MI_FTOI12() { REC_VUOP(VU0, FTOI12); } 
void recVU0MI_FTOI15() { REC_VUOP(VU0, FTOI15); } 
void recVU0MI_ITOF0()  { REC_VUOP(VU0, ITOF0); } 
void recVU0MI_ITOF4()  { REC_VUOP(VU0, ITOF4); } 
void recVU0MI_ITOF12() { REC_VUOP(VU0, ITOF12); } 
void recVU0MI_ITOF15() { REC_VUOP(VU0, ITOF15); } 
#endif

void recVU0MI_OPMULA() { recVUMI_OPMULA(VU, VUREC_INFO); } 
void recVU0MI_OPMSUB() { recVUMI_OPMSUB(VU, VUREC_INFO); } 
void recVU0MI_NOP()    { } 
void recVU0MI_CLIP() { recVUMI_CLIP(VU, VUREC_INFO); }

/*****************************************/ 
/*   VU Micromode Lower instructions    */ 
/*****************************************/ 

#ifdef RECOMPILE_VUMI_MISC

void recVU0MI_MTIR()    { recVUMI_MTIR(VU, VUREC_INFO); }
void recVU0MI_MR32()    { recVUMI_MR32(VU, VUREC_INFO); } 
void recVU0MI_MFIR()    { recVUMI_MFIR(VU, VUREC_INFO); }
void recVU0MI_MOVE()    { recVUMI_MOVE(VU, VUREC_INFO); } 
void recVU0MI_WAITQ()   { recVUMI_WAITQ(VU, VUREC_INFO); }
void recVU0MI_MFP()     { recVUMI_MFP(VU, VUREC_INFO); } 
void recVU0MI_WAITP()   { SysPrintf("vu0 wait p?\n"); } 

#else

void recVU0MI_MOVE()    { REC_VUOP(VU0, MOVE); } 
void recVU0MI_MFIR()    { REC_VUOP(VU0, MFIR); } 
void recVU0MI_MTIR()    { REC_VUOP(VU0, MTIR); } 
void recVU0MI_MR32()    { REC_VUOP(VU0, MR32); } 
void recVU0MI_WAITQ()   { } 
void recVU0MI_MFP()     { REC_VUOP(VU0, MFP); } 
void recVU0MI_WAITP()   { REC_VUOP(VU0, WAITP); } 

#endif

#ifdef RECOMPILE_VUMI_MATH

void recVU0MI_SQRT() { recVUMI_SQRT(VU, VUREC_INFO); }
void recVU0MI_RSQRT() { recVUMI_RSQRT(VU, VUREC_INFO); }
void recVU0MI_DIV() { recVUMI_DIV(VU, VUREC_INFO); }

#else

void recVU0MI_DIV()     { REC_VUOP(VU0, DIV);} 
void recVU0MI_SQRT()    { REC_VUOP(VU0, SQRT); } 
void recVU0MI_RSQRT()   { REC_VUOP(VU0, RSQRT); } 

#endif

#ifdef RECOMPILE_VUMI_E

void recVU0MI_ESADD()   { REC_VUOP(VU0, ESADD); } 
void recVU0MI_ERSADD()  { REC_VUOP(VU0, ERSADD); } 
void recVU0MI_ELENG()   { recVUMI_ELENG(VU, VUREC_INFO); } 
void recVU0MI_ERLENG()  { REC_VUOP(VU0, ERLENG); } 
void recVU0MI_EATANxy() { REC_VUOP(VU0, EATANxy); } 
void recVU0MI_EATANxz() { REC_VUOP(VU0, EATANxz); } 
void recVU0MI_ESUM()    { REC_VUOP(VU0, ESUM); } 
void recVU0MI_ERCPR()   { REC_VUOP(VU0, ERCPR); } 
void recVU0MI_ESQRT()   { REC_VUOP(VU0, ESQRT); } 
void recVU0MI_ERSQRT()  { REC_VUOP(VU0, ERSQRT); } 
void recVU0MI_ESIN()    { REC_VUOP(VU0, ESIN); } 
void recVU0MI_EATAN()   { REC_VUOP(VU0, EATAN); } 
void recVU0MI_EEXP()    { REC_VUOP(VU0, EEXP); } 

#else

void recVU0MI_ESADD()   { REC_VUOP(VU0, ESADD); } 
void recVU0MI_ERSADD()  { REC_VUOP(VU0, ERSADD); } 
void recVU0MI_ELENG()   { REC_VUOP(VU0, ELENG); } 
void recVU0MI_ERLENG()  { REC_VUOP(VU0, ERLENG); } 
void recVU0MI_EATANxy() { REC_VUOP(VU0, EATANxy); } 
void recVU0MI_EATANxz() { REC_VUOP(VU0, EATANxz); } 
void recVU0MI_ESUM()    { REC_VUOP(VU0, ESUM); } 
void recVU0MI_ERCPR()   { REC_VUOP(VU0, ERCPR); } 
void recVU0MI_ESQRT()   { REC_VUOP(VU0, ESQRT); } 
void recVU0MI_ERSQRT()  { REC_VUOP(VU0, ERSQRT); } 
void recVU0MI_ESIN()    { REC_VUOP(VU0, ESIN); } 
void recVU0MI_EATAN()   { REC_VUOP(VU0, EATAN); } 
void recVU0MI_EEXP()    { REC_VUOP(VU0, EEXP); } 

#endif

#ifdef RECOMPILE_VUMI_X

void recVU0MI_XITOP()   { recVUMI_XITOP(VU, VUREC_INFO); }
void recVU0MI_XGKICK()  { recVUMI_XGKICK(VU, VUREC_INFO); }
void recVU0MI_XTOP()    { recVUMI_XTOP(VU, VUREC_INFO); }

#else

void recVU0MI_XITOP()   { REC_VUOP(VU0, XITOP); }
void recVU0MI_XGKICK()  { REC_VUOP(VU0, XGKICK);}
void recVU0MI_XTOP()    { REC_VUOP(VU0, XTOP);}

#endif

#ifdef RECOMPILE_VUMI_RANDOM

void recVU0MI_RINIT()     { recVUMI_RINIT(VU, VUREC_INFO); }
void recVU0MI_RGET()      { recVUMI_RGET(VU, VUREC_INFO); }
void recVU0MI_RNEXT()     { recVUMI_RNEXT(VU, VUREC_INFO); }
void recVU0MI_RXOR()      { recVUMI_RXOR(VU, VUREC_INFO); }

#else

void recVU0MI_RINIT()     { REC_VUOP(VU0, RINIT); }
void recVU0MI_RGET()      { REC_VUOP(VU0, RGET); }
void recVU0MI_RNEXT()     { REC_VUOP(VU0, RNEXT); }
void recVU0MI_RXOR()      { REC_VUOP(VU0, RXOR); }

#endif

#ifdef RECOMPILE_VUMI_FLAG

void recVU0MI_FSAND()   { recVUMI_FSAND(VU, VUREC_INFO); } 
void recVU0MI_FSEQ()    { recVUMI_FSEQ(VU, VUREC_INFO); }
void recVU0MI_FSOR()    { recVUMI_FSOR(VU, VUREC_INFO); }
void recVU0MI_FSSET()   { recVUMI_FSSET(VU, VUREC_INFO); } 
void recVU0MI_FMEQ()    { recVUMI_FMEQ(VU, VUREC_INFO); }
void recVU0MI_FMOR()    { recVUMI_FMOR(VU, VUREC_INFO); }
void recVU0MI_FCEQ()    { recVUMI_FCEQ(VU, VUREC_INFO); }
void recVU0MI_FCOR()    { recVUMI_FCOR(VU, VUREC_INFO); }
void recVU0MI_FCSET()   { recVUMI_FCSET(VU, VUREC_INFO); }
void recVU0MI_FCGET()   { recVUMI_FCGET(VU, VUREC_INFO); }
void recVU0MI_FCAND()   { recVUMI_FCAND(VU, VUREC_INFO); }
void recVU0MI_FMAND()   { recVUMI_FMAND(VU, VUREC_INFO); }

#else

void recVU0MI_FSAND()   { REC_VUOP(VU0, FSAND); } 
void recVU0MI_FSEQ()    { REC_VUOP(VU0, FSEQ); } 
void recVU0MI_FSOR()    { REC_VUOP(VU0, FSOR); } 
void recVU0MI_FSSET()   { REC_VUOP(VU0, FSSET); } 
void recVU0MI_FMAND()   { REC_VUOP(VU0, FMAND); } 
void recVU0MI_FMEQ()    { REC_VUOP(VU0, FMEQ); } 
void recVU0MI_FMOR()    { REC_VUOP(VU0, FMOR); } 
void recVU0MI_FCAND()   { REC_VUOP(VU0, FCAND); } 
void recVU0MI_FCEQ()    { REC_VUOP(VU0, FCEQ); } 
void recVU0MI_FCOR()    { REC_VUOP(VU0, FCOR); } 
void recVU0MI_FCSET()   { REC_VUOP(VU0, FCSET); } 
void recVU0MI_FCGET()   { REC_VUOP(VU0, FCGET); } 

#endif

#ifdef RECOMPILE_VUMI_LOADSTORE

void recVU0MI_LQ()      { recVUMI_LQ(VU, VUREC_INFO); } 
void recVU0MI_LQD()     { recVUMI_LQD(VU, VUREC_INFO); } 
void recVU0MI_LQI()     { recVUMI_LQI(VU, VUREC_INFO); } 
void recVU0MI_SQ()      { recVUMI_SQ(VU, VUREC_INFO); }
void recVU0MI_SQD()     { recVUMI_SQD(VU, VUREC_INFO); }
void recVU0MI_SQI()     { recVUMI_SQI(VU, VUREC_INFO); }
void recVU0MI_ILW()     { recVUMI_ILW(VU, VUREC_INFO); }  
void recVU0MI_ISW()     { recVUMI_ISW(VU, VUREC_INFO); }
void recVU0MI_ILWR()    { recVUMI_ILWR(VU, VUREC_INFO); }
void recVU0MI_ISWR()    { recVUMI_ISWR(VU, VUREC_INFO); }

#else

void recVU0MI_LQ()      { REC_VUOP(VU0, LQ); } 
void recVU0MI_LQD()     { REC_VUOP(VU0, LQD); } 
void recVU0MI_LQI()     { REC_VUOP(VU0, LQI); } 
void recVU0MI_SQ()      { REC_VUOP(VU0, SQ); } 
void recVU0MI_SQD()     { REC_VUOP(VU0, SQD); } 
void recVU0MI_SQI()     { REC_VUOP(VU0, SQI); } 
void recVU0MI_ILW()     { REC_VUOP(VU0, ILW); } 
void recVU0MI_ISW()     { REC_VUOP(VU0, ISW); } 
void recVU0MI_ILWR()    { REC_VUOP(VU0, ILWR); } 
void recVU0MI_ISWR()    { REC_VUOP(VU0, ISWR); } 

#endif

#ifdef RECOMPILE_VUMI_ARITHMETIC

void recVU0MI_IADD()    { recVUMI_IADD(VU, VUREC_INFO); }
void recVU0MI_IADDI()   { recVUMI_IADDI(VU, VUREC_INFO); }
void recVU0MI_IADDIU()  { recVUMI_IADDIU(VU, VUREC_INFO); } 
void recVU0MI_IOR()     { recVUMI_IOR(VU, VUREC_INFO); }
void recVU0MI_ISUB()    { recVUMI_ISUB(VU, VUREC_INFO); }
void recVU0MI_IAND()    { recVUMI_IAND(VU, VUREC_INFO); }
void recVU0MI_ISUBIU()  { recVUMI_ISUBIU(VU, VUREC_INFO); } 

#else

void recVU0MI_IADD()    { REC_VUOP(VU0, IADD); }
void recVU0MI_IADDI()   { REC_VUOP(VU0, IADDI); }
void recVU0MI_IADDIU()  { REC_VUOP(VU0, IADDIU); } 
void recVU0MI_IOR()     { REC_VUOP(VU0, IOR); }
void recVU0MI_ISUB()    { REC_VUOP(VU0, ISUB); }
void recVU0MI_IAND()    { REC_VUOP(VU0, IAND); }
void recVU0MI_ISUBIU()  { REC_VUOP(VU0, ISUBIU); } 

#endif                                

#ifdef RECOMPILE_VUMI_BRANCH

void recVU0MI_IBEQ()    { recVUMI_IBEQ(VU, VUREC_INFO); } 
void recVU0MI_IBGEZ()   { recVUMI_IBGEZ(VU, VUREC_INFO); } 
void recVU0MI_IBLTZ()   { recVUMI_IBLTZ(VU, VUREC_INFO); } 
void recVU0MI_IBLEZ()   { recVUMI_IBLEZ(VU, VUREC_INFO); } 
void recVU0MI_IBGTZ()   { recVUMI_IBGTZ(VU, VUREC_INFO); } 
void recVU0MI_IBNE()    { recVUMI_IBNE(VU, VUREC_INFO); } 
void recVU0MI_B()       { recVUMI_B(VU, VUREC_INFO); } 
void recVU0MI_BAL()     { recVUMI_BAL(VU, VUREC_INFO); } 
void recVU0MI_JR()      { recVUMI_JR(VU, VUREC_INFO); } 
void recVU0MI_JALR()    { recVUMI_JALR(VU, VUREC_INFO); } 

#else

void recVU0MI_IBEQ()    { REC_VUOP(VU0, IBEQ); } 
void recVU0MI_IBGEZ()   { REC_VUOP(VU0, IBGEZ); } 
void recVU0MI_IBGTZ()   { REC_VUOP(VU0, IBGTZ); } 
void recVU0MI_IBLTZ()   { REC_VUOP(VU0, IBLTZ); } 
void recVU0MI_IBLEZ()   { REC_VUOP(VU0, IBLEZ); } 
void recVU0MI_IBNE()    { REC_VUOP(VU0, IBNE); } 
void recVU0MI_B()       { REC_VUOP(VU0, B); } 
void recVU0MI_BAL()     { REC_VUOP(VU0, BAL); } 
void recVU0MI_JR()      { REC_VUOP(VU0, JR); } 
void recVU0MI_JALR()    { REC_VUOP(VU0, JALR); } 

#endif

#endif // PCSX2_NORECBUILD
