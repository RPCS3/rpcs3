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

void UpdateR5900op();
extern  void (*LT_OpcodePrintTable[64])();
extern  void (*LT_SpecialPrintTable[64])();
extern  void (*LT_REGIMMPrintTable[32])();
extern  void (*LT_MMIPrintTable[64])();
extern  void (*LT_MMI0PrintTable[32])();
extern  void (*LT_MMI1PrintTable[32])();
extern  void (*LT_MMI2PrintTable[32])();
extern  void (*LT_MMI3PrintTable[32])();
extern  void (*LT_COP0PrintTable[32])();
extern  void (*LT_COP0BC0PrintTable[32])();
extern  void (*LT_COP0C0PrintTable[64])();
extern  void (*LT_COP1PrintTable[32])();
extern  void (*LT_COP1BC1PrintTable[32])();
extern  void (*LT_COP1SPrintTable[64])();
extern  void (*LT_COP1WPrintTable[64])();
extern  void (*LT_COP2PrintTable[32])();
extern  void (*LT_COP2BC2PrintTable[32])();
extern  void (*LT_COP2SPECIAL1PrintTable[64])();
extern  void (*LT_COP2SPECIAL2PrintTable[128])();
// **********************Standard Opcodes**************************
int  L_ADD=0;
int  L_ADDI=0;
int  L_ADDIU=0;
int  L_ADDU=0;
int  L_AND=0;
int  L_ANDI=0;
int  L_BEQ=0;
int  L_BEQL=0;
int  L_BGEZ=0;
int  L_BGEZAL=0;
int  L_BGEZALL=0;
int  L_BGEZL=0;
int  L_BGTZ=0;
int  L_BGTZL=0;
int  L_BLEZ=0;
int  L_BLEZL=0;
int  L_BLTZ=0;
int  L_BLTZAL=0;
int  L_BLTZALL=0;
int  L_BLTZL=0;
int  L_BNE=0;
int  L_BNEL=0;
int  L_BREAK=0;
int  L_CACHE=0;
int  L_DADD=0;
int  L_DADDI=0;
int  L_DADDIU=0;
int  L_DADDU=0;
int  L_DIV=0;
int  L_DIVU=0;
int  L_DSLL=0;
int  L_DSLL32=0;
int  L_DSLLV=0;
int  L_DSRA=0;
int  L_DSRA32=0;
int  L_DSRAV=0;
int  L_DSRL=0;
int  L_DSRL32=0;
int  L_DSRLV=0;
int  L_DSUB=0;
int  L_DSUBU=0;
int  L_J=0;
int  L_JAL=0;
int  L_JALR=0;
int  L_JR=0;
int  L_LB=0;
int  L_LBU=0;
int  L_LD=0;
int  L_LDL=0;
int  L_LDR=0;
int  L_LH=0;
int  L_LHU=0;
int  L_LQ=0;
int  L_LQC2=0;
int  L_LUI=0;
int  L_LW=0;
int  L_LWC1=0;
int  L_LWL=0;
int  L_LWR=0;
int  L_LWU=0;
int  L_MFHI=0;
int  L_MFLO=0;
int  L_MFSA=0;
int  L_MOVN=0;
int  L_MOVZ=0;
int  L_MTHI=0;
int  L_MTLO=0;
int  L_MTSA=0;
int  L_MTSAB=0;
int  L_MTSAH=0;
int  L_MULT=0;
int  L_MULTU=0;
int  L_NOR=0;
int  L_OR=0;
int  L_ORI=0;
int  L_PREF=0;
int  L_SB=0;
int  L_SD=0;
int  L_SDL=0;
int  L_SDR=0;
int  L_SH=0;
int  L_SLL=0;
int  L_SLLV=0;
int  L_SLT=0;
int  L_SLTI=0;
int  L_SLTIU=0;
int  L_SLTU=0;
int  L_SQ=0;
int  L_SQC2=0;
int  L_SRA=0;
int  L_SRAV=0;
int  L_SRL=0;
int  L_SRLV=0;
int  L_SUB=0;
int  L_SUBU=0;
int  L_SW=0;
int  L_SWC1=0;
int  L_SWL=0;
int  L_SWR=0;
int  L_SYNC=0;
int  L_SYSCALL=0;
int  L_TEQ=0;
int  L_TEQI=0;
int  L_TGE=0;
int  L_TGEI=0;
int  L_TGEIU=0;
int  L_TGEU=0;
int  L_TLT=0;
int  L_TLTI=0;
int  L_TLTIU=0;
int  L_TLTU=0;
int  L_TNE=0;
int  L_TNEI=0;
int  L_XOR=0;
int  L_XORI=0;




//*****************MMI OPCODES*********************************
int  L_MADD=0;
int  L_MADDU=0;
int  L_PLZCW=0;
int  L_MADD1=0;
int  L_MADDU1=0;
int  L_MFHI1=0;
int  L_MTHI1=0;
int  L_MFLO1=0;
int  L_MTLO1=0;
int  L_MULT1=0;
int  L_MULTU1=0;
int  L_DIV1=0;
int  L_DIVU1=0;
int  L_PMFHL=0;
int  L_PMTHL=0;
int  L_PSLLH=0;
int  L_PSRLH=0;
int  L_PSRAH=0;
int  L_PSLLW=0;
int  L_PSRLW=0;
int  L_PSRAW=0;
//*****************END OF MMI OPCODES**************************
//*************************MMI0 OPCODES************************

int  L_PADDW=0;  
int  L_PSUBW=0;  
int  L_PCGTW=0;  
int  L_PMAXW=0; 
int  L_PADDH=0;  
int  L_PSUBH=0;  
int  L_PCGTH=0;  
int  L_PMAXH=0; 
int  L_PADDB=0;  
int  L_PSUBB=0;  
int  L_PCGTB=0;
int  L_PADDSW=0; 
int  L_PSUBSW=0; 
int  L_PEXTLW=0;  
int  L_PPACW=0; 
int  L_PADDSH=0;
int  L_PSUBSH=0;
int  L_PEXTLH=0; 
int  L_PPACH=0;
int  L_PADDSB=0;
int  L_PSUBSB=0;
int  L_PEXTLB=0; 
int  L_PPACB=0;
int  L_PEXT5=0; 
int  L_PPAC5=0;
//***END OF MMI0 OPCODES******************************************
//**********MMI1 OPCODES**************************************
int  L_PABSW=0;
int  L_PCEQW=0;
int  L_PMINW=0; 
int  L_PADSBH=0;
int  L_PABSH=0;
int  L_PCEQH=0;
int  L_PMINH=0;  
int  L_PCEQB=0; 
int  L_PADDUW=0;
int  L_PSUBUW=0; 
int  L_PEXTUW=0;  
int  L_PADDUH=0;
int  L_PSUBUH=0; 
int  L_PEXTUH=0; 
int  L_PADDUB=0;
int  L_PSUBUB=0;
int  L_PEXTUB=0;
int  L_QFSRV=0; 
//********END OF MMI1 OPCODES***********************************
//*********MMI2 OPCODES***************************************
int  L_PMADDW=0;
int  L_PSLLVW=0;
int  L_PSRLVW=0; 
int  L_PMSUBW=0;
int  L_PMFHI=0;
int  L_PMFLO=0;
int  L_PINTH=0;
int  L_PMULTW=0;
int  L_PDIVW=0;
int  L_PCPYLD=0;
int  L_PMADDH=0;
int  L_PHMADH=0;
int  L_PAND=0;
int  L_PXOR=0; 
int  L_PMSUBH=0;
int  L_PHMSBH=0;
int  L_PEXEH=0;
int  L_PREVH=0; 
int  L_PMULTH=0;
int  L_PDIVBW=0;
int  L_PEXEW=0;
int  L_PROT3W=0;
//*****END OF MMI2 OPCODES***********************************
//*************************MMI3 OPCODES************************
int  L_PMADDUW=0;
int  L_PSRAVW=0; 
int  L_PMTHI=0;
int  L_PMTLO=0;
int  L_PINTEH=0; 
int  L_PMULTUW=0;
int  L_PDIVUW=0;
int  L_PCPYUD=0; 
int  L_POR=0;
int  L_PNOR=0;  
int  L_PEXCH=0;
int  L_PCPYH=0; 
int  L_PEXCW=0;
//**********************END OF MMI3 OPCODES******************** 
//****************************************************************************
//** COP0                                                                   **
//****************************************************************************
int  L_MFC0=0;
int  L_MTC0=0;
int  L_BC0F=0;
int  L_BC0T=0;
int  L_BC0FL=0;
int  L_BC0TL=0;
int  L_TLBR=0;
int  L_TLBWI=0;
int  L_TLBWR=0;
int  L_TLBP=0;
int  L_ERET=0;
int  L_DI=0;
int  L_EI=0;
//****************************************************************************
//** END OF COP0                                                            **
//****************************************************************************
//****************************************************************************
//** COP1 - Floating Point Unit (FPU)                                       **
//****************************************************************************
int  L_MFC1=0;
int  L_CFC1=0;
int  L_MTC1=0;
int  L_CTC1=0;
int  L_BC1F=0;
int  L_BC1T=0;
int  L_BC1FL=0;
int  L_BC1TL=0;
int  L_ADD_S=0;  
int  L_SUB_S=0;  
int  L_MUL_S=0;  
int  L_DIV_S=0;  
int  L_SQRT_S=0; 
int  L_ABS_S=0;  
int  L_MOV_S=0; 
int  L_NEG_S=0; 
int  L_RSQRT_S=0;  
int  L_ADDA_S=0; 
int  L_SUBA_S=0; 
int  L_MULA_S=0;
int  L_MADD_S=0; 
int  L_MSUB_S=0; 
int  L_MADDA_S=0; 
int  L_MSUBA_S=0;
int  L_CVT_W=0;
int  L_MAX_S=0;
int  L_MIN_S=0;
int  L_C_F=0;
int  L_C_EQ=0;
int  L_C_LT=0;
int  L_C_LE=0;
 int  L_CVT_S=0;
//****************************************************************************
//** END OF COP1                                                            **
//****************************************************************************
//****************************************************************************
//** COP2 - (VU0)                                                           **
//****************************************************************************
int L_QMFC2=0; 
int L_CFC2=0; 
int L_QMTC2=0;
int L_CTC2=0;  
int L_BC2F=0;
int L_BC2T=0;
int L_BC2FL=0;
int L_BC2TL=0;
int L_VADDx=0;       
int L_VADDy=0;       
int L_VADDz=0;       
int L_VADDw=0;       
int L_VSUBx=0;        
int L_VSUBy=0;        
int L_VSUBz=0;
int L_VSUBw=0; 
int L_VMADDx=0;
int L_VMADDy=0;
int L_VMADDz=0;
int L_VMADDw=0;
int L_VMSUBx=0;
int L_VMSUBy=0;
int L_VMSUBz=0;       
int L_VMSUBw=0; 
int L_VMAXx=0;       
int L_VMAXy=0;       
int L_VMAXz=0;       
int L_VMAXw=0;       
int L_VMINIx=0;       
int L_VMINIy=0;       
int L_VMINIz=0;       
int L_VMINIw=0; 
int L_VMULx=0;       
int L_VMULy=0;       
int L_VMULz=0;       
int L_VMULw=0;       
int L_VMULq=0;        
int L_VMAXi=0;        
int L_VMULi=0;        
int L_VMINIi=0;
int L_VADDq=0;
int L_VMADDq=0;      
int L_VADDi=0;       
int L_VMADDi=0;      
int L_VSUBq=0;        
int L_VMSUBq=0;       
int L_VSUBi=0;        
int L_VMSUBi=0; 
int L_VADD=0;        
int L_VMADD=0;       
int L_VMUL=0;        
int L_VMAX=0;        
int L_VSUB=0;         
int L_VMSUB=0;       
int L_VOPMSUB=0;      
int L_VMINI=0;  
int L_VIADD=0;       
int L_VISUB=0;       
int L_VIADDI=0;       
int L_VIAND=0;        
int L_VIOR=0;        
int L_VCALLMS=0;     
int L_VCALLMSR=0;   
int L_VADDAx=0;      
int L_VADDAy=0;      
int L_VADDAz=0;      
int L_VADDAw=0;      
int L_VSUBAx=0;      
int L_VSUBAy=0;      
int L_VSUBAz=0;      
int L_VSUBAw=0;
int L_VMADDAx=0;     
int L_VMADDAy=0;     
int L_VMADDAz=0;     
int L_VMADDAw=0;     
int L_VMSUBAx=0;     
int L_VMSUBAy=0;     
int L_VMSUBAz=0;     
int L_VMSUBAw=0;
int L_VITOF0=0;      
int L_VITOF4=0;      
int L_VITOF12=0;     
int L_VITOF15=0;     
int L_VFTOI0=0;      
int L_VFTOI4=0;      
int L_VFTOI12=0;     
int L_VFTOI15=0;
int L_VMULAx=0;      
int L_VMULAy=0;      
int L_VMULAz=0;      
int L_VMULAw=0;      
int L_VMULAq=0;      
int L_VABS=0;        
int L_VMULAi=0;      
int L_VCLIPw=0;
int L_VADDAq=0;      
int L_VMADDAq=0;     
int L_VADDAi=0;      
int L_VMADDAi=0;     
int L_VSUBAq=0;      
int L_VMSUBAq=0;     
int L_VSUBAi=0;      
int L_VMSUBAi=0;
int L_VADDA=0;       
int L_VMADDA=0;      
int L_VMULA=0;       
int L_VSUBA=0;       
int L_VMSUBA=0;      
int L_VOPMULA=0;     
int L_VNOP=0;   
int L_VMOVE=0;       
int L_VMR32=0;       
int L_VLQI=0;        
int L_VSQI=0;        
int L_VLQD=0;        
int L_VSQD=0;   
int L_VDIV=0;        
int L_VSQRT=0;       
int L_VRSQRT=0;      
int L_VWAITQ=0;     
int L_VMTIR=0;       
int L_VMFIR=0;       
int L_VILWR=0;       
int L_VISWR=0;  
int L_VRNEXT=0;      
int L_VRGET=0;       
int L_VRINIT=0;      
int L_VRXOR=0;  
