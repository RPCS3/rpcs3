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

// Holds instruction tables for the r5900 recompiler

// stop compiling if NORECBUILD build (only for Visual Studio)
#if !(defined(_MSC_VER) && defined(PCSX2_NORECBUILD))

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>

#include "Common.h"
#include "Memory.h"
#include "InterTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"
#include "iR5900AritImm.h"
#include "iR5900Arit.h"
#include "iR5900MultDiv.h"
#include "iR5900Shift.h"
#include "iR5900Branch.h"
#include "iR5900Jump.h"
#include "iR5900LoadStore.h"
#include "iR5900Move.h"
#include "iMMI.h"
#include "iFPU.h"
#include "iCP0.h"

////////////////////////////////////////////////////
static void recNULL( void ) 
{
	SysPrintf("EE: Unimplemented op %x\n", cpuRegs.code);
}

////////////////////////////////////////////////////
static void recREGIMM( void ) 
{
	recREG[ _Rt_ ]( );
}

////////////////////////////////////////////////////
static void recSPECIAL( void )
{
	recSPC[ _Funct_ ]( );
}

////////////////////////////////////////////////////
static void recCOP0( void )
{
	recCP0[ _Rs_ ]( );
}

////////////////////////////////////////////////////
static void recCOP0BC0( void )
{
	recCP0BC0[ ( cpuRegs.code >> 16 ) & 0x03 ]( );
}

////////////////////////////////////////////////////
static void recCOP0C0( void ) 
{
	recCP0C0[ _Funct_ ]( );
}

////////////////////////////////////////////////////
static void recCOP1( void ) {
	recCP1[ _Rs_ ]( );
}

////////////////////////////////////////////////////
static void recMMI( void ) 
{
	recMMIt[ _Funct_ ]( );
}


/**********************************************************
*    UNHANDLED YET OPCODES
*
**********************************************************/

////////////////////////////////////////////////////
//REC_SYS(PREF);
////////////////////////////////////////////////////
//REC_SYS(MFSA);
////////////////////////////////////////////////////
//REC_SYS(MTSA);
////////////////////////////////////////////////////
REC_SYS(TGE);
////////////////////////////////////////////////////
REC_SYS(TGEU);
////////////////////////////////////////////////////
REC_SYS(TLT);
////////////////////////////////////////////////////
REC_SYS(TLTU);
////////////////////////////////////////////////////
REC_SYS(TEQ);
////////////////////////////////////////////////////
REC_SYS(TNE);
////////////////////////////////////////////////////
REC_SYS(TGEI);
////////////////////////////////////////////////////
REC_SYS(TGEIU);
////////////////////////////////////////////////////
REC_SYS(TLTI);
////////////////////////////////////////////////////
REC_SYS(TLTIU);
////////////////////////////////////////////////////
REC_SYS(TEQI);
////////////////////////////////////////////////////
REC_SYS(TNEI);
////////////////////////////////////////////////////
//REC_SYS(MTSAB);
////////////////////////////////////////////////////
//REC_SYS(MTSAH);
////////////////////////////////////////////////////
REC_SYS(CACHE);

/*
void recTGE( void ) 
{
}

void recTGEU( void ) 
{
}

void recTLT( void ) 
{
}

void recTLTU( void ) 
{
}

void recTEQ( void ) 
{
}

void recTNE( void ) 
{
}

void recTGEI( void )
{
}

void recTGEIU( void ) 
{
}

void recTLTI( void ) 
{
}

void recTLTIU( void ) 
{
}

void recTEQI( void ) 
{
}

void recTNEI( void ) 
{
}

*/

/////////////////////////////////
// Foward-Prob Function Tables //
/////////////////////////////////
extern void recCOP2( void );
extern void recSYSCALL( void );
extern void recBREAK( void );
extern void recPREF( void );
extern void recSYNC( void );
extern void recMFSA( void );
extern void recMTSA( void );
extern void recMTSAB( void );
extern void recMTSAH( void );

void (*recBSC[64] )() = {
    recSPECIAL, recREGIMM, recJ,    recJAL,   recBEQ,  recBNE,  recBLEZ,  recBGTZ,
    recADDI,    recADDIU,  recSLTI, recSLTIU, recANDI, recORI,  recXORI,  recLUI,
    recCOP0,    recCOP1,   recCOP2, recNULL,  recBEQL, recBNEL, recBLEZL, recBGTZL,
    recDADDI,   recDADDIU, recLDL,  recLDR,   recMMI,  recNULL, recLQ,    recSQ,
    recLB,      recLH,     recLWL,  recLW,    recLBU,  recLHU,  recLWR,   recLWU,
    recSB,      recSH,     recSWL,  recSW,    recSDL,  recSDR,  recSWR,   recCACHE,
    recNULL,    recLWC1,   recNULL, recPREF,  recNULL, recNULL, recLQC2,  recLD,
    recNULL,    recSWC1,   recNULL, recNULL,  recNULL, recNULL, recSQC2,  recSD
};

#ifdef PCSX2_VIRTUAL_MEM
// coissued insts
void (*recBSC_co[64] )() = {
    recNULL,	recNULL,   recNULL, recNULL,  recNULL, recNULL, recNULL,  recNULL,
    recNULL,	recNULL,   recNULL, recNULL,  recNULL, recNULL, recNULL,  recNULL,
    recNULL,	recNULL,   recNULL, recNULL,  recNULL, recNULL, recNULL,  recNULL,
    recNULL,    recNULL,   recLDL_co,  recLDR_co,   recNULL, recNULL, recLQ_co,    recSQ_co,
    recLB_co,      recLH_co,     recLWL_co,  recLW_co,    recLBU_co,  recLHU_co,  recLWR_co,   recLWU_co,
    recSB_co,      recSH_co,     recSWL_co,  recSW_co,    recSDL_co,  recSDR_co,  recSWR_co,   recNULL,
    recNULL,    recLWC1_co,   recNULL, recNULL,  recNULL, recNULL, recLQC2_co,  recLD_co,
    recNULL,    recSWC1_co,   recNULL, recNULL,  recNULL, recNULL, recSQC2_co,  recSD_co
};
#endif

void (*recSPC[64] )() = {
    recSLL,  recNULL,  recSRL,  recSRA,  recSLLV,    recNULL,  recSRLV,   recSRAV,
    recJR,   recJALR,  recMOVZ, recMOVN, recSYSCALL, recBREAK, recNULL,   recSYNC,
    recMFHI, recMTHI,  recMFLO, recMTLO, recDSLLV,   recNULL,  recDSRLV,  recDSRAV,
    recMULT, recMULTU, recDIV,  recDIVU, recNULL,    recNULL,  recNULL,   recNULL,
    recADD,  recADDU,  recSUB,  recSUBU, recAND,     recOR,    recXOR,    recNOR,
    recMFSA, recMTSA,  recSLT,  recSLTU, recDADD,    recDADDU, recDSUB,   recDSUBU,
    recTGE,  recTGEU,  recTLT,  recTLTU, recTEQ,     recNULL,  recTNE,    recNULL,
    recDSLL, recNULL,  recDSRL, recDSRA, recDSLL32,  recNULL,  recDSRL32, recDSRA32
};

void (*recREG[32] )() = {
    recBLTZ,   recBGEZ,   recBLTZL,   recBGEZL,   recNULL, recNULL, recNULL, recNULL,
    recTGEI,   recTGEIU,  recTLTI,    recTLTIU,   recTEQI, recNULL, recTNEI, recNULL,
    recBLTZAL, recBGEZAL, recBLTZALL, recBGEZALL, recNULL, recNULL, recNULL, recNULL,
    recMTSAB,  recMTSAH,  recNULL,    recNULL,    recNULL, recNULL, recNULL, recNULL,
};

void (*recCP0[32] )() = {
    recMFC0,    recNULL, recNULL, recNULL, recMTC0, recNULL, recNULL, recNULL,
    recCOP0BC0, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
    recCOP0C0,  recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
    recNULL,    recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
};

void (*recCP0BC0[32] )() = {
    recBC0F, recBC0T, recBC0FL, recBC0TL, recNULL, recNULL, recNULL, recNULL,
    recNULL, recNULL, recNULL,  recNULL,  recNULL, recNULL, recNULL, recNULL,
    recNULL, recNULL, recNULL,  recNULL,  recNULL, recNULL, recNULL, recNULL,
    recNULL, recNULL, recNULL,  recNULL,  recNULL, recNULL, recNULL, recNULL,
};

void (*recCP0C0[64] )() = {
    recNULL, recTLBR, recTLBWI, recNULL, recNULL, recNULL, recTLBWR, recNULL,
    recTLBP, recNULL, recNULL,  recNULL, recNULL, recNULL, recNULL,  recNULL,
    recNULL, recNULL, recNULL,  recNULL, recNULL, recNULL, recNULL,  recNULL,
    recERET, recNULL, recNULL,  recNULL, recNULL, recNULL, recNULL,  recNULL,
    recNULL, recNULL, recNULL,  recNULL, recNULL, recNULL, recNULL,  recNULL,
    recNULL, recNULL, recNULL,  recNULL, recNULL, recNULL, recNULL,  recNULL,
    recNULL, recNULL, recNULL,  recNULL, recNULL, recNULL, recNULL,  recNULL,
    recEI,   recDI,   recNULL,  recNULL, recNULL, recNULL, recNULL,  recNULL,
};

void (*recCP1[32] )() = {
    recMFC1,     recNULL, recCFC1, recNULL, recMTC1,   recNULL, recCTC1, recNULL,
    recCOP1_BC1, recNULL, recNULL, recNULL, recNULL,   recNULL, recNULL, recNULL,
    recCOP1_S,   recNULL, recNULL, recNULL, recCOP1_W, recNULL, recNULL, recNULL,
    recNULL,     recNULL, recNULL, recNULL, recNULL,   recNULL, recNULL, recNULL,
};

void (*recCP1BC1[32] )() = {
    recBC1F, recBC1T, recBC1FL, recBC1TL, recNULL, recNULL, recNULL, recNULL,
    recNULL, recNULL, recNULL,  recNULL,  recNULL, recNULL, recNULL, recNULL,
    recNULL, recNULL, recNULL,  recNULL,  recNULL, recNULL, recNULL, recNULL,
    recNULL, recNULL, recNULL,  recNULL,  recNULL, recNULL, recNULL, recNULL,
};

void (*recCP1S[64] )() = {
	recADD_S,  recSUB_S,  recMUL_S,  recDIV_S, recSQRT_S, recABS_S,  recMOV_S,   recNEG_S, 
	recNULL,   recNULL,   recNULL,   recNULL,  recNULL,   recNULL,   recNULL,    recNULL,   
	recNULL,   recNULL,   recNULL,   recNULL,  recNULL,   recNULL,   recRSQRT_S, recNULL,  
	recADDA_S, recSUBA_S, recMULA_S, recNULL,  recMADD_S, recMSUB_S, recMADDA_S, recMSUBA_S,
	recNULL,   recNULL,   recNULL,   recNULL,  recCVT_W,  recNULL,   recNULL,    recNULL, 
	recMAX_S,  recMIN_S,  recNULL,   recNULL,  recNULL,   recNULL,   recNULL,    recNULL, 
	recC_F,    recNULL,   recC_EQ,   recNULL,  recC_LT,   recNULL,   recC_LE,    recNULL, 
	recNULL,   recNULL,   recNULL,   recNULL,  recNULL,   recNULL,   recNULL,    recNULL, 
};
 
void (*recCP1W[64] )() = { 
	recNULL,  recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,   	
	recNULL,  recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,   
	recNULL,  recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,   
	recNULL,  recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,   
	recCVT_S, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,   
	recNULL,  recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,   
	recNULL,  recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,   
	recNULL,  recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
};

void (*recMMIt[64] )() = {
	 recMADD,  recMADDU,  recNULL,  recNULL,  recPLZCW, recNULL, recNULL,  recNULL,
    recMMI0,  recMMI2,   recNULL,  recNULL,  recNULL,  recNULL, recNULL,  recNULL,
    recMFHI1, recMTHI1,  recMFLO1, recMTLO1, recNULL,  recNULL, recNULL,  recNULL,
    recMULT1, recMULTU1, recDIV1,  recDIVU1, recNULL,  recNULL, recNULL,  recNULL,
    recMADD1, recMADDU1, recNULL,  recNULL,  recNULL,  recNULL, recNULL,  recNULL,
    recMMI1 , recMMI3,   recNULL,  recNULL,  recNULL,  recNULL, recNULL,  recNULL,
    recPMFHL, recPMTHL,  recNULL,  recNULL,  recPSLLH, recNULL, recPSRLH, recPSRAH,
    recNULL,  recNULL,   recNULL,  recNULL,  recPSLLW, recNULL, recPSRLW, recPSRAW,
};

void (*recMMI0t[32] )() = { 
	recPADDW,  recPSUBW,  recPCGTW,  recPMAXW,       
	recPADDH,  recPSUBH,  recPCGTH,  recPMAXH,        
	recPADDB,  recPSUBB,  recPCGTB,  recNULL,
	recNULL,   recNULL,   recNULL,   recNULL,
	recPADDSW, recPSUBSW, recPEXTLW, recPPACW,        
	recPADDSH, recPSUBSH, recPEXTLH, recPPACH,        
	recPADDSB, recPSUBSB, recPEXTLB, recPPACB,        
	recNULL,   recNULL,   recPEXT5,  recPPAC5,
};

void (*recMMI1t[32] )() = { 
	recNULL,   recPABSW,  recPCEQW,  recPMINW, 
	recPADSBH, recPABSH,  recPCEQH,  recPMINH, 
	recNULL,   recNULL,   recPCEQB,  recNULL, 
	recNULL,   recNULL,   recNULL,   recNULL, 
	recPADDUW, recPSUBUW, recPEXTUW, recNULL,  
	recPADDUH, recPSUBUH, recPEXTUH, recNULL, 
	recPADDUB, recPSUBUB, recPEXTUB, recQFSRV, 
	recNULL,   recNULL,   recNULL,   recNULL, 
};

void (*recMMI2t[32] )() = { 
	recPMADDW, recNULL,   recPSLLVW, recPSRLVW, 
	recPMSUBW, recNULL,   recNULL,   recNULL,
	recPMFHI,  recPMFLO,  recPINTH,  recNULL,
	recPMULTW, recPDIVW,  recPCPYLD, recNULL,
	recPMADDH, recPHMADH, recPAND,   recPXOR, 
	recPMSUBH, recPHMSBH, recNULL,   recNULL, 
	recNULL,   recNULL,   recPEXEH,  recPREVH, 
	recPMULTH, recPDIVBW, recPEXEW,  recPROT3W, 
};

void (*recMMI3t[32] )() = { 
	recPMADDUW, recNULL,   recNULL,   recPSRAVW, 
	recNULL,    recNULL,   recNULL,   recNULL,
	recPMTHI,   recPMTLO,  recPINTEH, recNULL,
	recPMULTUW, recPDIVUW, recPCPYUD, recNULL,
	recNULL,    recNULL,   recPOR,    recPNOR,  
	recNULL,    recNULL,   recNULL,   recNULL,
	recNULL,    recNULL,   recPEXCH,  recPCPYH, 
	recNULL,    recNULL,   recPEXCW,  recNULL,
};

////////////////////////////////////////////////
// Back-Prob Function Tables - Gathering Info //
////////////////////////////////////////////////
#define rpropSetRead(reg, mask) { \
	if( !(pinst->regs[reg] & EEINST_USED) ) \
		pinst->regs[reg] |= EEINST_LASTUSE; \
	prev->regs[reg] |= EEINST_LIVE0|(mask)|EEINST_USED; \
	pinst->regs[reg] |= EEINST_USED; \
	if( reg ) pinst->info = ((mask)&(EEINST_MMX|EEINST_XMM)); \
	_recFillRegister(pinst, XMMTYPE_GPRREG, reg, 0); \
} \

#define rpropSetWrite0(reg, mask, live) { \
	prev->regs[reg] &= ~((mask)|live|EEINST_XMM|EEINST_MMX); \
	if( !(pinst->regs[reg] & EEINST_USED) ) \
		pinst->regs[reg] |= EEINST_LASTUSE; \
	pinst->regs[reg] |= EEINST_USED; \
	prev->regs[reg] |= EEINST_USED; \
	_recFillRegister(pinst, XMMTYPE_GPRREG, reg, 1); \
}

#define rpropSetWrite(reg, mask) { \
	rpropSetWrite0(reg, mask, EEINST_LIVE0); \
} \

#define rpropSetFast(write1, read1, read2, mask) { \
	if( write1 ) { rpropSetWrite(write1, mask); } \
	if( read1 ) { rpropSetRead(read1, mask); } \
	if( read2 ) { rpropSetRead(read2, mask); } \
} \

#define rpropSetLOHI(write1, read1, read2, mask, lo, hi) { \
	if( write1 ) { rpropSetWrite(write1, mask); } \
	if( (lo) & MODE_WRITE ) { rpropSetWrite(XMMGPR_LO, mask); } \
	if( (hi) & MODE_WRITE ) { rpropSetWrite(XMMGPR_HI, mask); } \
	if( read1 ) { rpropSetRead(read1, mask); } \
	if( read2 ) { rpropSetRead(read2, mask); } \
	if( (lo) & MODE_READ ) { rpropSetRead(XMMGPR_LO, mask); } \
	if( (hi) & MODE_READ ) { rpropSetRead(XMMGPR_HI, mask); } \
} \

// FPU regs
#define rpropSetFPURead(reg, mask) { \
	if( !(pinst->fpuregs[reg] & EEINST_USED) ) \
		pinst->fpuregs[reg] |= EEINST_LASTUSE; \
	prev->fpuregs[reg] |= EEINST_LIVE0|(mask)|EEINST_USED; \
	pinst->fpuregs[reg] |= EEINST_USED; \
	if( reg ) pinst->info = ((mask)&(EEINST_MMX|EEINST_XMM)); \
	if( reg == XMMFPU_ACC ) _recFillRegister(pinst, XMMTYPE_FPACC, 0, 0); \
	else _recFillRegister(pinst, XMMTYPE_FPREG, reg, 0); \
} \

#define rpropSetFPUWrite0(reg, mask, live) { \
	prev->fpuregs[reg] &= ~((mask)|live|EEINST_XMM|EEINST_MMX); \
	if( !(pinst->fpuregs[reg] & EEINST_USED) ) \
		pinst->fpuregs[reg] |= EEINST_LASTUSE; \
	pinst->fpuregs[reg] |= EEINST_USED; \
	prev->fpuregs[reg] |= EEINST_USED; \
	if( reg == XMMFPU_ACC ) _recFillRegister(pinst, XMMTYPE_FPACC, 0, 1); \
	else _recFillRegister(pinst, XMMTYPE_FPREG, reg, 1); \
}

#define rpropSetFPUWrite(reg, mask) { \
	rpropSetFPUWrite0(reg, mask, EEINST_LIVE0); \
} \


void rpropBSC(EEINST* prev, EEINST* pinst);
void rpropSPECIAL(EEINST* prev, EEINST* pinst);
void rpropREGIMM(EEINST* prev, EEINST* pinst);
void rpropCP0(EEINST* prev, EEINST* pinst);
void rpropCP1(EEINST* prev, EEINST* pinst);
void rpropCP2(EEINST* prev, EEINST* pinst);
void rpropMMI(EEINST* prev, EEINST* pinst);
void rpropMMI0(EEINST* prev, EEINST* pinst);
void rpropMMI1(EEINST* prev, EEINST* pinst);
void rpropMMI2(EEINST* prev, EEINST* pinst);
void rpropMMI3(EEINST* prev, EEINST* pinst);

#define EEINST_REALXMM (cpucaps.hasStreamingSIMDExtensions?EEINST_XMM:0)

//SPECIAL, REGIMM, J,    JAL,   BEQ,  BNE,  BLEZ,  BGTZ,
//ADDI,    ADDIU,  SLTI, SLTIU, ANDI, ORI,  XORI,  LUI,
//COP0,    COP1,   COP2, NULL,  BEQL, BNEL, BLEZL, BGTZL,
//DADDI,   DADDIU, LDL,  LDR,   MMI,  NULL, LQ,    SQ,
//LB,      LH,     LWL,  LW,    LBU,  LHU,  LWR,   LWU,
//SB,      SH,     SWL,  SW,    SDL,  SDR,  SWR,   CACHE,
//NULL,    LWC1,   NULL, PREF,  NULL, NULL, LQC2,  LD,
//NULL,    SWC1,   NULL, NULL,  NULL, NULL, SQC2,  SD
void rpropBSC(EEINST* prev, EEINST* pinst)
{
	switch(cpuRegs.code >> 26) {
		case 0: rpropSPECIAL(prev, pinst); break;
		case 1: rpropREGIMM(prev, pinst); break;
		case 2: // j
			break;
		case 3: // jal
			rpropSetWrite(31, EEINST_LIVE1);
			break;
		case 4: // beq
		case 5: // bne
			rpropSetRead(_Rs_, EEINST_LIVE1);
			rpropSetRead(_Rt_, EEINST_LIVE1);
			pinst->info |= (cpucaps.hasStreamingSIMD2Extensions?(EEINST_REALXMM|EEINST_MMX):0);
			break;

		case 20: // beql
		case 21: // bnel
			// reset since don't know which path to go
			_recClearInst(prev);
			prev->info = 0;
			rpropSetRead(_Rs_, EEINST_LIVE1);
			rpropSetRead(_Rt_, EEINST_LIVE1);
			pinst->info |= (cpucaps.hasStreamingSIMD2Extensions?(EEINST_REALXMM|EEINST_MMX):0);
			break;

		case 6: // blez
		case 7: // bgtz
			rpropSetRead(_Rs_, EEINST_LIVE1);
			break;

		case 22: // blezl
		case 23: // bgtzl
			// reset since don't know which path to go
			_recClearInst(prev);
			prev->info = 0;
			rpropSetRead(_Rs_, EEINST_LIVE1);
			break;

		case 24: // daddi
		case 25: // daddiu
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1|((_Rs_!=0&&cpucaps.hasStreamingSIMD2Extensions)?EEINST_MMX:0));
			break;

		case 8: // addi
		case 9: // addiu
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rs_, 0);
			pinst->info |= EEINST_MMX;
			break;

		case 10: // slti
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1);
			pinst->info |= EEINST_MMX;
			break;
		case 11: // sltiu
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1);
			pinst->info |= EEINST_MMX;
			break;

		case 12: // andi
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rs_, (_Rs_!=_Rt_?EEINST_MMX:0));
			pinst->info |= EEINST_MMX;
			break;
		case 13: // ori
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1|(_Rs_!=_Rt_?EEINST_MMX:0));
			pinst->info |= EEINST_MMX;
			break;
		case 14: // xori
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1|(_Rs_!=_Rt_?EEINST_MMX:0));
			pinst->info |= EEINST_MMX;
			break;

		case 15: // lui
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			break;

		case 16: rpropCP0(prev, pinst); break;
		case 17: rpropCP1(prev, pinst); break;
		case 18: rpropCP2(prev, pinst); break;

		// loads	
		case 34: // lwl
		case 38: // lwr
		case 26: // ldl
		case 27: // ldr
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rs_, 0);
			break;

		case 32: case 33: case 35: case 36: case 37: case 39:
		case 55: // LD
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rs_, 0);
			break;

		case 28: rpropMMI(prev, pinst); break;

		case 30: // lq
			rpropSetWrite(_Rt_, EEINST_LIVE1|EEINST_LIVE2);
			rpropSetRead(_Rs_, 0);
			pinst->info |= EEINST_MMX;
			pinst->info |= EEINST_REALXMM;
			break;

		case 31: // sq
			rpropSetRead(_Rt_, EEINST_LIVE1|EEINST_LIVE2|EEINST_REALXMM);
			rpropSetRead(_Rs_, 0);
			pinst->info |= EEINST_MMX;
			pinst->info |= EEINST_REALXMM;
			break;

		// 4 byte stores
		case 40: case 41: case 42: case 43: case 46:
			rpropSetRead(_Rt_, 0);
			rpropSetRead(_Rs_, 0);
			pinst->info |= EEINST_MMX;
			pinst->info |= EEINST_REALXMM;
			break;

		case 44: // sdl
		case 45: // sdr
		case 63: // sd
			rpropSetRead(_Rt_, EEINST_LIVE1|EEINST_MMX|EEINST_REALXMM);
			rpropSetRead(_Rs_, 0);
			break;

		case 49: // lwc1
			rpropSetFPUWrite(_Rt_, EEINST_REALXMM);
			rpropSetRead(_Rs_, 0);
			break;

		case 57: // swc1
			rpropSetFPURead(_Rt_, EEINST_REALXMM);
			rpropSetRead(_Rs_, 0);
			break;

		case 54: // lqc2
		case 62: // sqc2
			rpropSetRead(_Rs_, 0);
			break;

		default:
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1|(_Rs_!=0?EEINST_MMX:0));
			break;
	}
}

//SLL,  NULL,  SRL,  SRA,  SLLV,    NULL,  SRLV,   SRAV,
//JR,   JALR,  MOVZ, MOVN, SYSCALL, BREAK, NULL,   SYNC,
//MFHI, MTHI,  MFLO, MTLO, DSLLV,   NULL,  DSRLV,  DSRAV,
//MULT, MULTU, DIV,  DIVU, NULL,    NULL,  NULL,   NULL,
//ADD,  ADDU,  SUB,  SUBU, AND,     OR,    XOR,    NOR,
//MFSA, MTSA,  SLT,  SLTU, DADD,    DADDU, DSUB,   DSUBU,
//TGE,  TGEU,  TLT,  TLTU, TEQ,     NULL,  TNE,    NULL,
//DSLL, NULL,  DSRL, DSRA, DSLL32,  NULL,  DSRL32, DSRA32
void rpropSPECIAL(EEINST* prev, EEINST* pinst)
{
	int temp;
	switch(_Funct_) {
		case 0: // SLL
		case 2: // SRL
		case 3: // SRA
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rt_, EEINST_MMX);
			break;

		case 4: // sllv
		case 6: // srlv
		case 7: // srav
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_MMX);
			rpropSetRead(_Rt_, EEINST_MMX);
			break;

		case 8: // JR
			rpropSetRead(_Rs_, 0);
			break;
		case 9: // JALR
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rs_, 0);
			break;

		case 10: // movz
		case 11: // movn
			// do not write _Rd_!
			rpropSetRead(_Rs_, EEINST_LIVE1);
			rpropSetRead(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rd_, EEINST_LIVE1);
			pinst->info |= EEINST_MMX;
			_recFillRegister(pinst, XMMTYPE_GPRREG, _Rd_, 1);
			break;

		case 12: // syscall
		case 13: // break
			_recClearInst(prev);
			prev->info = 0;
			break;
		case 15: // sync
			break;

		case 16: // mfhi
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(XMMGPR_HI, (pinst->regs[_Rd_]&EEINST_MMX|EEINST_REALXMM)|EEINST_LIVE1);
			break;
		case 17: // mthi
			rpropSetWrite(XMMGPR_HI, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1);
			break;
		case 18: // mflo
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(XMMGPR_LO, (pinst->regs[_Rd_]&EEINST_MMX|EEINST_REALXMM)|EEINST_LIVE1);
			break;
		case 19: // mtlo
			rpropSetWrite(XMMGPR_LO, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1);
			break;

		case 20: // dsllv
		case 22: // dsrlv
		case 23: // dsrav
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_MMX);
			rpropSetRead(_Rt_, EEINST_LIVE1|EEINST_MMX);
			break;

		case 24: // mult
			// can do unsigned mult only if HI isn't used
			//temp = (pinst->regs[XMMGPR_HI]&(EEINST_LIVE0|EEINST_LIVE1))?0:(cpucaps.hasStreamingSIMD2Extensions?EEINST_MMX:0);
			temp = 0;
			rpropSetWrite(XMMGPR_LO, EEINST_LIVE1);
			rpropSetWrite(XMMGPR_HI, EEINST_LIVE1);
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rs_, temp);
			rpropSetRead(_Rt_, temp);
			pinst->info |= temp;
			break;
		case 25: // multu
			rpropSetWrite(XMMGPR_LO, EEINST_LIVE1);
			rpropSetWrite(XMMGPR_HI, EEINST_LIVE1);
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rs_, (cpucaps.hasStreamingSIMD2Extensions?EEINST_MMX:0));
			rpropSetRead(_Rt_, (cpucaps.hasStreamingSIMD2Extensions?EEINST_MMX:0));
			if( cpucaps.hasStreamingSIMD2Extensions ) pinst->info |= EEINST_MMX;
			break;

		case 26: // div
		case 27: // divu
			rpropSetWrite(XMMGPR_LO, EEINST_LIVE1);
			rpropSetWrite(XMMGPR_HI, EEINST_LIVE1);
			rpropSetRead(_Rs_, 0);
			rpropSetRead(_Rt_, 0);
			//pinst->info |= EEINST_REALXMM|EEINST_MMX;
			break;

		case 32: // add
		case 33: // addu
		case 34: // sub
		case 35: // subu
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			if( _Rs_ ) rpropSetRead(_Rs_, 0);
			if( _Rt_ ) rpropSetRead(_Rt_, 0);
			pinst->info |= EEINST_MMX;
			break;

		case 36: // and
		case 37: // or
		case 38: // xor
		case 39: // nor
			// if rd == rs or rt, keep live1
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rs_, (pinst->regs[_Rd_]&EEINST_LIVE1)|EEINST_MMX);
			rpropSetRead(_Rt_, (pinst->regs[_Rd_]&EEINST_LIVE1)|EEINST_MMX);
			break;

		case 40: // mfsa
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			break;
		case 41: // mtsa
			rpropSetRead(_Rs_, EEINST_LIVE1|EEINST_MMX);
			break;

		case 42: // slt
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1);
			rpropSetRead(_Rt_, EEINST_LIVE1);
			pinst->info |= EEINST_MMX;
			break;

		case 43: // sltu
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1);
			rpropSetRead(_Rt_, EEINST_LIVE1);
			pinst->info |= EEINST_MMX;
			break;

		case 44: // dadd
		case 45: // daddu
		case 46: // dsub
		case 47: // dsubu
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			if( _Rs_ == 0 || _Rt_ == 0 ) {
				// just a copy, so don't force mmx
				rpropSetRead(_Rs_, (pinst->regs[_Rd_]&EEINST_LIVE1));
				rpropSetRead(_Rt_, (pinst->regs[_Rd_]&EEINST_LIVE1));
			}
			else {
				rpropSetRead(_Rs_, (pinst->regs[_Rd_]&EEINST_LIVE1)|(cpucaps.hasStreamingSIMD2Extensions?EEINST_MMX:0));
				rpropSetRead(_Rt_, (pinst->regs[_Rd_]&EEINST_LIVE1)|(cpucaps.hasStreamingSIMD2Extensions?EEINST_MMX:0));
			}
			if( cpucaps.hasStreamingSIMD2Extensions ) pinst->info |= EEINST_MMX;
			break;

		// traps
		case 48: case 49: case 50: case 51: case 52: case 54:
			rpropSetRead(_Rs_, EEINST_LIVE1);
			rpropSetRead(_Rt_, EEINST_LIVE1);
			break;

		case 56: // dsll
		case 58: // dsrl
		case 59: // dsra
		case 62: // dsrl32
		case 63: // dsra32
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rt_, EEINST_LIVE1|(cpucaps.hasStreamingSIMD2Extensions?EEINST_MMX:0));
			pinst->info |= EEINST_MMX;
			break;

		case 60: // dsll32
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rt_, (cpucaps.hasStreamingSIMD2Extensions?EEINST_MMX:0));
			pinst->info |= EEINST_MMX;
			break;

		default:
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1|EEINST_MMX);
			rpropSetRead(_Rt_, EEINST_LIVE1|EEINST_MMX);
			break;
	}
}

//BLTZ,   BGEZ,   BLTZL,   BGEZL,   NULL, NULL, NULL, NULL,
//TGEI,   TGEIU,  TLTI,    TLTIU,   TEQI, NULL, TNEI, NULL,
//BLTZAL, BGEZAL, BLTZALL, BGEZALL, NULL, NULL, NULL, NULL,
//MTSAB,  MTSAH,  NULL,    NULL,    NULL, NULL, NULL, NULL,
void rpropREGIMM(EEINST* prev, EEINST* pinst)
{
	switch(_Rt_) {
		case 0: // bltz
		case 1: // bgez
			rpropSetRead(_Rs_, EEINST_LIVE1);
			pinst->info |= EEINST_MMX;
			pinst->info |= EEINST_REALXMM;
			break;

		case 2: // bltzl
		case 3: // bgezl
			// reset since don't know which path to go
			_recClearInst(prev);
			prev->info = 0;
			rpropSetRead(_Rs_, EEINST_LIVE1);
			pinst->info |= EEINST_MMX;
			pinst->info |= EEINST_REALXMM;
			break;

		// traps
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 14:
			rpropSetRead(_Rs_, EEINST_LIVE1);
			break;

		case 16: // bltzal
		case 17: // bgezal
			// do not write 31
			rpropSetRead(_Rs_, EEINST_LIVE1);
			break;

		case 18: // bltzall
		case 19: // bgezall
			// reset since don't know which path to go
			_recClearInst(prev);
			prev->info = 0;
			// do not write 31
			rpropSetRead(_Rs_, EEINST_LIVE1);
			break;

		case 24: // mtsab
		case 25: // mtsah
			rpropSetRead(_Rs_, 0);
			break;
		default:
			assert(0);
			break;
	}
}

//MFC0,    NULL, NULL, NULL, MTC0, NULL, NULL, NULL,
//COP0BC0, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
//COP0C0,  NULL, NULL, NULL, NULL, NULL, NULL, NULL,
//NULL,    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
void rpropCP0(EEINST* prev, EEINST* pinst)
{
	switch(_Rs_) {
		case 0: // mfc0
			rpropSetWrite(_Rt_, EEINST_LIVE1|EEINST_REALXMM);
			break;
		case 4:
			rpropSetRead(_Rt_, EEINST_LIVE1|EEINST_REALXMM);
			break;
		case 8: // cop0bc0
			_recClearInst(prev);
			prev->info = 0;
			break;
		case 16: // cop0c0
			_recClearInst(prev);
			prev->info = 0;
			break;
	}
}

#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_

//ADD_S,  SUB_S,  MUL_S,  DIV_S, SQRT_S, ABS_S,  MOV_S,   NEG_S, 
//NULL,   NULL,   NULL,   NULL,  NULL,   NULL,   NULL,    NULL,   
//NULL,   NULL,   NULL,   NULL,  NULL,   NULL,   RSQRT_S, NULL,  
//ADDA_S, SUBA_S, MULA_S, NULL,  MADD_S, MSUB_S, MADDA_S, MSUBA_S,
//NULL,   NULL,   NULL,   NULL,  CVT_W,  NULL,   NULL,    NULL, 
//MAX_S,  MIN_S,  NULL,   NULL,  NULL,   NULL,   NULL,    NULL, 
//C_F,    NULL,   C_EQ,   NULL,  C_LT,   NULL,   C_LE,    NULL, 
//NULL,   NULL,   NULL,   NULL,  NULL,   NULL,   NULL,    NULL, 
void rpropCP1(EEINST* prev, EEINST* pinst)
{
	switch(_Rs_) {
		case 0: // mfc1
			rpropSetWrite(_Rt_, EEINST_LIVE1|EEINST_REALXMM);
			rpropSetFPURead(_Fs_, EEINST_REALXMM);
			break;
		case 2: // cfc1
			rpropSetWrite(_Rt_, EEINST_LIVE1|EEINST_REALXMM|EEINST_MMX);
			break;
		case 4: // mtc1
			rpropSetFPUWrite(_Fs_, EEINST_REALXMM);
			rpropSetRead(_Rt_, EEINST_LIVE1|EEINST_REALXMM);
			break;
		case 6: // ctc1
			rpropSetRead(_Rt_, EEINST_LIVE1|EEINST_REALXMM|EEINST_MMX);
			break;
		case 8: // bc1
			// reset since don't know which path to go
			_recClearInst(prev);
			prev->info = 0;
			break;
		case 16:
			// floating point ops
			pinst->info |= EEINST_REALXMM;
			switch( _Funct_ ) {
				case 0: // add.s
				case 1: // sub.s
				case 2: // mul.s
				case 3: // div.s
				case 22: // rsqrt.s
				case 40: // max.s
				case 41: // min.s
					rpropSetFPUWrite(_Fd_, EEINST_REALXMM);
					rpropSetFPURead(_Fs_, EEINST_REALXMM);
					rpropSetFPURead(_Ft_, EEINST_REALXMM);
					break;
				case 4: // sqrt.s
					rpropSetFPUWrite(_Fd_, EEINST_REALXMM);
					rpropSetFPURead(_Ft_, EEINST_REALXMM);
					break;
				case 5: // abs.s
				case 6: // mov.s
				case 7: // neg.s
				case 36: // cvt.w
					rpropSetFPUWrite(_Fd_, EEINST_REALXMM);
					rpropSetFPURead(_Fs_, EEINST_REALXMM);
					break;
				case 24: // adda.s
				case 25: // suba.s
				case 26: // mula.s
					rpropSetFPUWrite(XMMFPU_ACC, EEINST_REALXMM);
					rpropSetFPURead(_Fs_, EEINST_REALXMM);
					rpropSetFPURead(_Ft_, EEINST_REALXMM);
					break;
				case 28: // madd.s
				case 29: // msub.s
					rpropSetFPUWrite(_Fd_, EEINST_REALXMM);
					rpropSetFPURead(XMMFPU_ACC, EEINST_REALXMM);
					rpropSetFPURead(_Fs_, EEINST_REALXMM);
					rpropSetFPURead(_Ft_, EEINST_REALXMM);
					break;

				case 30: // madda.s
				case 31: // msuba.s
					rpropSetFPUWrite(XMMFPU_ACC, EEINST_REALXMM);
					rpropSetFPURead(XMMFPU_ACC, EEINST_REALXMM);
					rpropSetFPURead(_Fs_, EEINST_REALXMM);
					rpropSetFPURead(_Ft_, EEINST_REALXMM);
					break;

				case 48: // c.f
				case 50: // c.eq
				case 52: // c.lt
				case 54: // c.le
					rpropSetFPURead(_Fs_, EEINST_REALXMM);
					rpropSetFPURead(_Ft_, EEINST_REALXMM);
					break;
				default: assert(0);
			}
			break;
		case 20:
			assert( _Funct_ == 32 ); // CVT.S.W
			rpropSetFPUWrite(_Fd_, EEINST_REALXMM);
			rpropSetFPURead(_Fs_, EEINST_REALXMM);
			break;
		default:
			assert(0);
	}
}

#undef _Ft_
#undef _Fs_
#undef _Fd_

void rpropCP2(EEINST* prev, EEINST* pinst)
{
	switch(_Rs_) {
		case 1: // qmfc2
			rpropSetWrite(_Rt_, EEINST_LIVE2|EEINST_LIVE1);
			break;

		case 2: // cfc2
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			break;

		case 5: // qmtc2
			rpropSetRead(_Rt_, EEINST_LIVE1|EEINST_LIVE2);
			break;

		case 6: // ctc2
			rpropSetRead(_Rt_, 0);
			break;

		case 8: // bc2
			break;

		default:
			// vu macro mode insts
			pinst->info |= 2;
			break;
	}
}

//MADD,  MADDU,  NULL,  NULL,  PLZCW, NULL, NULL,  NULL,
//MMI0,  MMI2,   NULL,  NULL,  NULL,  NULL, NULL,  NULL,
//MFHI1, MTHI1,  MFLO1, MTLO1, NULL,  NULL, NULL,  NULL,
//MULT1, MULTU1, DIV1,  DIVU1, NULL,  NULL, NULL,  NULL,
//MADD1, MADDU1, NULL,  NULL,  NULL,  NULL, NULL,  NULL,
//MMI1 , MMI3,   NULL,  NULL,  NULL,  NULL, NULL,  NULL,
//PMFHL, PMTHL,  NULL,  NULL,  PSLLH, NULL, PSRLH, PSRAH,
//NULL,  NULL,   NULL,  NULL,  PSLLW, NULL, PSRLW, PSRAW,
void rpropMMI(EEINST* prev, EEINST* pinst)
{
	int temp;
	switch(cpuRegs.code&0x3f) {
		case 0: // madd
		case 1: // maddu
			rpropSetLOHI(_Rd_, _Rs_, _Rt_, EEINST_LIVE1, MODE_READ|MODE_WRITE, MODE_READ|MODE_WRITE);
			break;
		case 4: // plzcw
			rpropSetFast(_Rd_, _Rs_, 0, EEINST_LIVE1);
			break;
		case 8: rpropMMI0(prev, pinst); break;
		case 9: rpropMMI2(prev, pinst); break;

		case 16: // mfhi1
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			temp = ((pinst->regs[_Rd_]&(EEINST_MMX|EEINST_REALXMM))?EEINST_MMX:EEINST_REALXMM);
			rpropSetRead(XMMGPR_HI, temp|EEINST_LIVE2);
			break;
		case 17: // mthi1
			rpropSetWrite0(XMMGPR_HI, EEINST_LIVE2, 0);
			rpropSetRead(_Rs_, EEINST_LIVE1);
			break;
		case 18: // mflo1
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			temp = ((pinst->regs[_Rd_]&(EEINST_MMX|EEINST_REALXMM))?EEINST_MMX:EEINST_REALXMM);
			rpropSetRead(XMMGPR_LO, temp|EEINST_LIVE2);
			break;
		case 19: // mtlo1
			rpropSetWrite0(XMMGPR_LO, EEINST_LIVE2, 0);
			rpropSetRead(_Rs_, EEINST_LIVE1);
			break;

		case 24: // mult1
			temp = (pinst->regs[XMMGPR_HI]&(EEINST_LIVE2))?0:(cpucaps.hasStreamingSIMD2Extensions?EEINST_MMX:0);
			rpropSetWrite0(XMMGPR_LO, EEINST_LIVE2, 0);
			rpropSetWrite0(XMMGPR_HI, EEINST_LIVE2, 0);
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rs_, temp);
			rpropSetRead(_Rt_, temp);
			pinst->info |= temp;
			break;
		case 25: // multu1
			rpropSetWrite0(XMMGPR_LO, EEINST_LIVE2, 0);
			rpropSetWrite0(XMMGPR_HI, EEINST_LIVE2, 0);
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rs_, (cpucaps.hasStreamingSIMD2Extensions?EEINST_MMX:0));
			rpropSetRead(_Rt_, (cpucaps.hasStreamingSIMD2Extensions?EEINST_MMX:0));
			if( cpucaps.hasStreamingSIMD2Extensions ) pinst->info |= EEINST_MMX;
			break;

		case 26: // div1
		case 27: // divu1
			rpropSetWrite0(XMMGPR_LO, EEINST_LIVE2, 0);
			rpropSetWrite0(XMMGPR_HI, EEINST_LIVE2, 0);
			rpropSetRead(_Rs_, 0);
			rpropSetRead(_Rt_, 0);
			//pinst->info |= EEINST_REALXMM|EEINST_MMX;
			break;

		case 32: // madd1
		case 33: // maddu1
			rpropSetWrite0(XMMGPR_LO, EEINST_LIVE2, 0);
			rpropSetWrite0(XMMGPR_HI, EEINST_LIVE2, 0);
			rpropSetFast(_Rd_, _Rs_, _Rt_, EEINST_LIVE1);
			rpropSetRead(XMMGPR_LO, EEINST_LIVE2);
			rpropSetRead(XMMGPR_HI, EEINST_LIVE2);
			break;

		case 40: rpropMMI1(prev, pinst); break;
		case 41: rpropMMI3(prev, pinst); break;

		case 48: // pmfhl
			rpropSetWrite(_Rd_, EEINST_LIVE2|EEINST_LIVE1);
			rpropSetRead(XMMGPR_LO, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM);
			rpropSetRead(XMMGPR_HI, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM);
			break;

		case 49: // pmthl
			rpropSetWrite(XMMGPR_LO, EEINST_LIVE2|EEINST_LIVE1);
			rpropSetWrite(XMMGPR_HI, EEINST_LIVE2|EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM);
			break;

		default:
			rpropSetFast(_Rd_, _Rs_, _Rt_, EEINST_LIVE1|EEINST_LIVE2|EEINST_REALXMM);
			break;
	}
}

//recPADDW,  PSUBW,  PCGTW,  PMAXW,
//PADDH,  PSUBH,  PCGTH,  PMAXH,
//PADDB,  PSUBB,  PCGTB,  NULL,
//NULL,   NULL,   NULL,   NULL,
//PADDSW, PSUBSW, PEXTLW, PPACW,
//PADDSH, PSUBSH, PEXTLH, PPACH,
//PADDSB, PSUBSB, PEXTLB, PPACB,
//NULL,   NULL,   PEXT5,  PPAC5,
void rpropMMI0(EEINST* prev, EEINST* pinst)
{
	switch((cpuRegs.code>>6)&0x1f) {
		case 16: // paddsw
		case 17: // psubsw
			rpropSetFast(_Rd_, _Rs_, _Rt_, EEINST_LIVE1|EEINST_LIVE2);
			break;

		case 18: // pextlw
		case 22: // pextlh
		case 26: // pextlb
			rpropSetWrite(_Rd_, EEINST_LIVE2|EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1|EEINST_REALXMM);
			rpropSetRead(_Rt_, EEINST_LIVE1|EEINST_REALXMM);
			break;

		case 30: // pext5
		case 31: // ppac5
			rpropSetFast(_Rd_, _Rs_, _Rt_, EEINST_LIVE1|EEINST_LIVE2);
			break;

		default:
			rpropSetFast(_Rd_, _Rs_, _Rt_, EEINST_LIVE1|EEINST_LIVE2|EEINST_REALXMM);
			break;
	}
}

void rpropMMI1(EEINST* prev, EEINST* pinst)
{
	switch((cpuRegs.code>>6)&0x1f) {
		case 1: // pabsw
		case 5: // pabsh
			rpropSetWrite(_Rd_, EEINST_LIVE2|EEINST_LIVE1);
			rpropSetRead(_Rt_, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM);
			break;

		case 17: // psubuw
			rpropSetFast(_Rd_, _Rs_, _Rt_, EEINST_LIVE1|EEINST_LIVE2);
			break;

		case 18: // pextuw
		case 22: // pextuh
		case 26: // pextub
			rpropSetWrite(_Rd_, EEINST_LIVE2|EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE2|EEINST_REALXMM);
			rpropSetRead(_Rt_, EEINST_LIVE2|EEINST_REALXMM);
			break;

		default:
			rpropSetFast(_Rd_, _Rs_, _Rt_, EEINST_LIVE1|EEINST_LIVE2|EEINST_REALXMM);
			break;
	}
}

void rpropMMI2(EEINST* prev, EEINST* pinst)
{
	switch((cpuRegs.code>>6)&0x1f) {
		case 0: // pmaddw
		case 4: // pmsubw
			rpropSetLOHI(_Rd_, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1, MODE_READ|MODE_WRITE, MODE_READ|MODE_WRITE);
			break;
		case 2: // psllvw
		case 3: // psllvw
			rpropSetFast(_Rd_, _Rs_, _Rt_, EEINST_LIVE2);
			break;
		case 8: // pmfhi
			rpropSetWrite(_Rd_, EEINST_LIVE2|EEINST_LIVE1);
			rpropSetRead(XMMGPR_HI, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM);
			break;
		case 9: // pmflo
			rpropSetWrite(_Rd_, EEINST_LIVE2|EEINST_LIVE1);
			rpropSetRead(XMMGPR_HI, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM);
			break;
		case 10: // pinth
			rpropSetWrite(_Rd_, EEINST_LIVE2|EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE2|EEINST_REALXMM);
			rpropSetRead(_Rt_, EEINST_LIVE1|EEINST_REALXMM);
			break;
		case 12: // pmultw, 
			rpropSetLOHI(_Rd_, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1, MODE_WRITE, MODE_WRITE);
			break;
		case 13: // pdivw
			rpropSetLOHI(0, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1, MODE_WRITE, MODE_WRITE);
			break;
		case 14: // pcpyld
			rpropSetWrite(_Rd_, EEINST_LIVE2|EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1|EEINST_REALXMM);
			rpropSetRead(_Rt_, EEINST_LIVE1|EEINST_REALXMM);
			break;
		case 16: // pmaddh
		case 17: // phmadh
		case 20: // pmsubh
		case 21: // phmsbh
			rpropSetLOHI(_Rd_, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM, MODE_READ|MODE_WRITE, MODE_READ|MODE_WRITE);
			break;

		case 26: // pexeh
		case 27: // prevh
		case 30: // pexew
		case 31: // prot3w
			rpropSetWrite(_Rd_, EEINST_LIVE2|EEINST_LIVE1);
			rpropSetRead(_Rt_, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM);
			break;

		case 28: // pmulth
			rpropSetLOHI(_Rd_, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM, MODE_WRITE, MODE_WRITE);
			break;
		case 29: // pdivbw
			rpropSetLOHI(_Rd_, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1, MODE_WRITE, MODE_WRITE);
			break;

		default:
			rpropSetFast(_Rd_, _Rs_, _Rt_, EEINST_LIVE1|EEINST_LIVE2|EEINST_REALXMM);
			break;
	}
}

void rpropMMI3(EEINST* prev, EEINST* pinst)
{
	switch((cpuRegs.code>>6)&0x1f) {
		case 0: // pmadduw
			rpropSetLOHI(_Rd_, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM, MODE_READ|MODE_WRITE, MODE_READ|MODE_WRITE);
			break;
		case 3: // psravw
			rpropSetFast(_Rd_, _Rs_, _Rt_, EEINST_LIVE2);
			break;

		case 8: // pmthi
			rpropSetWrite(XMMGPR_HI, EEINST_LIVE2|EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM);
			break;
		case 9: // pmtlo
			rpropSetWrite(XMMGPR_LO, EEINST_LIVE2|EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM);
			break;
		case 12: // pmultuw, 
			rpropSetLOHI(_Rd_, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM, MODE_WRITE, MODE_WRITE);
			break;
		case 13: // pdivuw
			rpropSetLOHI(0, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1, MODE_WRITE, MODE_WRITE);
			break;
		case 14: // pcpyud
			rpropSetWrite(_Rd_, EEINST_LIVE2|EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE2|EEINST_REALXMM);
			rpropSetRead(_Rt_, EEINST_LIVE2|EEINST_REALXMM);
			break;

		case 26: // pexch
		case 27: // pcpyh
		case 30: // pexcw
			rpropSetWrite(_Rd_, EEINST_LIVE2|EEINST_LIVE1);
			rpropSetRead(_Rt_, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM);
			break;
		
		default:
			rpropSetFast(_Rd_, _Rs_, _Rt_, EEINST_LIVE1|EEINST_LIVE2|EEINST_REALXMM);
			break;
	}
}

#endif // PCSX2_NORECBUILD
