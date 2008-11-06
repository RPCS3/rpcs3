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

//all tables for R5900 are define here..

#include "InterTables.h"

void (*Int_OpcodePrintTable[64])() = 
{
    SPECIAL,       REGIMM,       J,             JAL,           BEQ,          BNE,           BLEZ,  BGTZ,
    ADDI,          ADDIU,        SLTI,          SLTIU,         ANDI,         ORI,           XORI,  LUI,
    COP0,          COP1,         COP2,          UnknownOpcode, BEQL,         BNEL,          BLEZL, BGTZL,
    DADDI,         DADDIU,       LDL,           LDR,           MMI,          UnknownOpcode, LQ,    SQ,
    LB,            LH,           LWL,           LW,            LBU,          LHU,           LWR,   LWU,
    SB,            SH,           SWL,           SW,            SDL,          SDR,           SWR,   CACHE,
    UnknownOpcode, LWC1,         UnknownOpcode, PREF,          UnknownOpcode,UnknownOpcode, LQC2,  LD,
    UnknownOpcode, SWC1,         UnknownOpcode, UnknownOpcode, UnknownOpcode,UnknownOpcode, SQC2,  SD
};


void (*Int_SpecialPrintTable[64])() = 
{
    SLL,           UnknownOpcode, SRL,           SRA,           SLLV,    UnknownOpcode, SRLV,          SRAV,
    JR,            JALR,          MOVZ,          MOVN,          SYSCALL, BREAK,         UnknownOpcode, SYNC,
    MFHI,          MTHI,          MFLO,          MTLO,          DSLLV,   UnknownOpcode, DSRLV,         DSRAV,
    MULT,          MULTU,         DIV,           DIVU,          UnknownOpcode,UnknownOpcode,UnknownOpcode,UnknownOpcode,
    ADD,           ADDU,          SUB,           SUBU,          AND,     OR,            XOR,           NOR,
    MFSA ,         MTSA ,         SLT,           SLTU,          DADD,    DADDU,         DSUB,          DSUBU,
    TGE,           TGEU,          TLT,           TLTU,          TEQ,     UnknownOpcode, TNE,           UnknownOpcode,
    DSLL,          UnknownOpcode, DSRL,          DSRA,          DSLL32,  UnknownOpcode, DSRL32,        DSRA32
};

void (*Int_REGIMMPrintTable[32])() = {
    BLTZ,   BGEZ,   BLTZL,            BGEZL,         UnknownOpcode, UnknownOpcode, UnknownOpcode, UnknownOpcode,
    TGEI,   TGEIU,  TLTI,             TLTIU,         TEQI,          UnknownOpcode, TNEI,          UnknownOpcode,
    BLTZAL, BGEZAL, BLTZALL,          BGEZALL,       UnknownOpcode, UnknownOpcode, UnknownOpcode, UnknownOpcode,
    MTSAB,  MTSAH , UnknownOpcode,    UnknownOpcode, UnknownOpcode, UnknownOpcode, UnknownOpcode, UnknownOpcode,
};

void (*Int_MMIPrintTable[64])() = 
{
    MADD,                    MADDU,                  MMI_Unknown,          MMI_Unknown,          PLZCW,            MMI_Unknown,       MMI_Unknown,          MMI_Unknown,
    MMI0,                    MMI2,                   MMI_Unknown,          MMI_Unknown,          MMI_Unknown,      MMI_Unknown,       MMI_Unknown,          MMI_Unknown,
    MFHI1,                   MTHI1,                  MFLO1,                MTLO1,                MMI_Unknown,      MMI_Unknown,       MMI_Unknown,          MMI_Unknown,
    MULT1,                   MULTU1,                 DIV1,                 DIVU1,                MMI_Unknown,      MMI_Unknown,       MMI_Unknown,          MMI_Unknown,
    MADD1,                   MADDU1,                 MMI_Unknown,          MMI_Unknown,          MMI_Unknown,      MMI_Unknown,       MMI_Unknown,          MMI_Unknown,
    MMI1 ,                   MMI3,                   MMI_Unknown,          MMI_Unknown,          MMI_Unknown,      MMI_Unknown,       MMI_Unknown,          MMI_Unknown,
    PMFHL,                   PMTHL,                  MMI_Unknown,          MMI_Unknown,          PSLLH,            MMI_Unknown,       PSRLH,                PSRAH,
    MMI_Unknown,             MMI_Unknown,            MMI_Unknown,          MMI_Unknown,          PSLLW,            MMI_Unknown,       PSRLW,                PSRAW,
};

void (*Int_MMI0PrintTable[32])() = 
{ 
 PADDW,         PSUBW,         PCGTW,          PMAXW,       
 PADDH,         PSUBH,         PCGTH,          PMAXH,        
 PADDB,         PSUBB,         PCGTB,          MMI_Unknown,
 MMI_Unknown,   MMI_Unknown,   MMI_Unknown,    MMI_Unknown,
 PADDSW,        PSUBSW,        PEXTLW,         PPACW,        
 PADDSH,        PSUBSH,        PEXTLH,         PPACH,        
 PADDSB,        PSUBSB,        PEXTLB,         PPACB,        
 MMI_Unknown,   MMI_Unknown,   PEXT5,          PPAC5,        
};

void (*Int_MMI1PrintTable[32])() =
{ 
 MMI_Unknown,   PABSW,         PCEQW,         PMINW, 
 PADSBH,        PABSH,         PCEQH,         PMINH, 
 MMI_Unknown,   MMI_Unknown,   PCEQB,         MMI_Unknown, 
 MMI_Unknown,   MMI_Unknown,   MMI_Unknown,   MMI_Unknown, 
 PADDUW,        PSUBUW,        PEXTUW,        MMI_Unknown,  
 PADDUH,        PSUBUH,        PEXTUH,        MMI_Unknown, 
 PADDUB,        PSUBUB,        PEXTUB,        QFSRV, 
 MMI_Unknown,   MMI_Unknown,   MMI_Unknown,   MMI_Unknown, 
};


void (*Int_MMI2PrintTable[32])() = 
{ 
 PMADDW,        MMI_Unknown,   PSLLVW,        PSRLVW, 
 PMSUBW,        MMI_Unknown,   MMI_Unknown,   MMI_Unknown,
 PMFHI,         PMFLO,         PINTH,         MMI_Unknown,
 PMULTW,        PDIVW,         PCPYLD,        MMI_Unknown,
 PMADDH,        PHMADH,        PAND,          PXOR, 
 PMSUBH,        PHMSBH,        MMI_Unknown,   MMI_Unknown, 
 MMI_Unknown,   MMI_Unknown,   PEXEH,         PREVH, 
 PMULTH,        PDIVBW,        PEXEW,         PROT3W, 
};

void (*Int_MMI3PrintTable[32])() = 
{ 
 PMADDUW,       MMI_Unknown,   MMI_Unknown,   PSRAVW, 
 MMI_Unknown,   MMI_Unknown,   MMI_Unknown,   MMI_Unknown,
 PMTHI,         PMTLO,         PINTEH,        MMI_Unknown,
 PMULTUW,       PDIVUW,        PCPYUD,        MMI_Unknown,
 MMI_Unknown,   MMI_Unknown,   POR,           PNOR,  
 MMI_Unknown,   MMI_Unknown,   MMI_Unknown,   MMI_Unknown,
 MMI_Unknown,   MMI_Unknown,   PEXCH,         PCPYH, 
 MMI_Unknown,   MMI_Unknown,   PEXCW,         MMI_Unknown,
};

void (*Int_COP0PrintTable[32])() = 
{
    MFC0,         COP0_Unknown, COP0_Unknown, COP0_Unknown, MTC0,         COP0_Unknown, COP0_Unknown, COP0_Unknown,
    COP0_BC0,     COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown,
    COP0_Func,    COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown,
    COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown,
};

void (*Int_COP0BC0PrintTable[32])() = 
{
    BC0F,         BC0T,         BC0FL,        BC0TL,        COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown,
    COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown,
    COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown,
    COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown,
};

void (*Int_COP0C0PrintTable[64])() = {
    COP0_Unknown, TLBR,         TLBWI,        COP0_Unknown, COP0_Unknown, COP0_Unknown, TLBWR,        COP0_Unknown,
    TLBP,         COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown,
    COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown,
    ERET,         COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown,
    COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown,
    COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown,
    COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown,
    EI,           DI,           COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown, COP0_Unknown
};

void (*Int_COP1PrintTable[32])() = {
    MFC1,         COP1_Unknown, CFC1,         COP1_Unknown, MTC1,         COP1_Unknown, CTC1,         COP1_Unknown,
    COP1_BC1,     COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown,
    COP1_S,       COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_W,       COP1_Unknown, COP1_Unknown, COP1_Unknown,
    COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown,
};

void (*Int_COP1BC1PrintTable[32])() = {
    BC1F,         BC1T,         BC1FL,        BC1TL,        COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown,
    COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown,
    COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown,
    COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown, COP1_Unknown,
};

void (*Int_COP1SPrintTable[64])() = {
ADD_S,       SUB_S,       MUL_S,       DIV_S,       SQRT_S,      ABS_S,       MOV_S,       NEG_S, 
COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,   
COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,RSQRT_S,     COP1_Unknown,  
ADDA_S,      SUBA_S,      MULA_S,      COP1_Unknown,MADD_S,      MSUB_S,      MADDA_S,     MSUBA_S,
COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,CVT_W,       COP1_Unknown,COP1_Unknown,COP1_Unknown, 
MAX_S,       MIN_S,       COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown, 
C_F,         COP1_Unknown,C_EQ,        COP1_Unknown,C_LT,        COP1_Unknown,C_LE,        COP1_Unknown, 
COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown, 
};
 
void (*Int_COP1WPrintTable[64])() = { 
COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,   	
COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,   
COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,   
COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,   
CVT_S,       COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,   
COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,   
COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,   
COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,COP1_Unknown,   
};


void (*Int_COP2PrintTable[32])() = {
    COP2_Unknown, QMFC2,        CFC2,         COP2_Unknown, COP2_Unknown, QMTC2,        CTC2,         COP2_Unknown,
    COP2_BC2,     COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown,
    COP2_SPECIAL, COP2_SPECIAL, COP2_SPECIAL, COP2_SPECIAL, COP2_SPECIAL, COP2_SPECIAL, COP2_SPECIAL, COP2_SPECIAL,
	COP2_SPECIAL, COP2_SPECIAL, COP2_SPECIAL, COP2_SPECIAL, COP2_SPECIAL, COP2_SPECIAL, COP2_SPECIAL, COP2_SPECIAL,
};

void (*Int_COP2BC2PrintTable[32])() = {
    BC2F,         BC2T,         BC2FL,        BC2TL,        COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown,
    COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown,
    COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown,
    COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown, COP2_Unknown,
}; 

void (*Int_COP2SPECIAL1PrintTable[64])() = 
{ 
 VADDx,       VADDy,       VADDz,       VADDw,       VSUBx,        VSUBy,        VSUBz,        VSUBw,  
 VMADDx,      VMADDy,      VMADDz,      VMADDw,      VMSUBx,       VMSUBy,       VMSUBz,       VMSUBw, 
 VMAXx,       VMAXy,       VMAXz,       VMAXw,       VMINIx,       VMINIy,       VMINIz,       VMINIw, 
 VMULx,       VMULy,       VMULz,       VMULw,       VMULq,        VMAXi,        VMULi,        VMINIi,
 VADDq,       VMADDq,      VADDi,       VMADDi,      VSUBq,        VMSUBq,       VSUBi,        VMSUBi, 
 VADD,        VMADD,       VMUL,        VMAX,        VSUB,         VMSUB,        VOPMSUB,      VMINI,  
 VIADD,       VISUB,       VIADDI,      COP2_Unknown,VIAND,        VIOR,         COP2_Unknown, COP2_Unknown,
 VCALLMS,     VCALLMSR,    COP2_Unknown,COP2_Unknown,COP2_SPECIAL2,COP2_SPECIAL2,COP2_SPECIAL2,COP2_SPECIAL2,  
};

void (*Int_COP2SPECIAL2PrintTable[128])() = 
{
 VADDAx      ,VADDAy      ,VADDAz      ,VADDAw      ,VSUBAx      ,VSUBAy      ,VSUBAz      ,VSUBAw,
 VMADDAx     ,VMADDAy     ,VMADDAz     ,VMADDAw     ,VMSUBAx     ,VMSUBAy     ,VMSUBAz     ,VMSUBAw,
 VITOF0      ,VITOF4      ,VITOF12     ,VITOF15     ,VFTOI0      ,VFTOI4      ,VFTOI12     ,VFTOI15,
 VMULAx      ,VMULAy      ,VMULAz      ,VMULAw      ,VMULAq      ,VABS        ,VMULAi      ,VCLIPw,
 VADDAq      ,VMADDAq     ,VADDAi      ,VMADDAi     ,VSUBAq      ,VMSUBAq     ,VSUBAi      ,VMSUBAi,
 VADDA       ,VMADDA      ,VMULA       ,COP2_Unknown,VSUBA       ,VMSUBA      ,VOPMULA     ,VNOP,   
 VMOVE       ,VMR32       ,COP2_Unknown,COP2_Unknown,VLQI        ,VSQI        ,VLQD        ,VSQD,   
 VDIV        ,VSQRT       ,VRSQRT      ,VWAITQ      ,VMTIR       ,VMFIR       ,VILWR       ,VISWR,  
 VRNEXT      ,VRGET       ,VRINIT      ,VRXOR       ,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown, 
 COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,
 COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,
 COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,
 COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,
 COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,
 COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,
 COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,COP2_Unknown,
};
