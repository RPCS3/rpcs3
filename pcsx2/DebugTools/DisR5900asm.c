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

#include <stdio.h>
#include <string.h>

#include "PS2Etypes.h"
#include "Debug.h"
#include "R5900.h"
#include "DisASM.h"

unsigned long opcode_addr;

/*
//DECODE PROCUDURES

//cop0
#define DECODE_FS           (DECODE_RD)
#define DECODE_FT           (DECODE_RT)
#define DECODE_FD           (DECODE_SA)
/// ********

#define DECODE_FUNCTION     ((cpuRegs.code) & 0x3F)
#define DECODE_RD     ((cpuRegs.code >> 11) & 0x1F) // The rd part of the instruction register 
#define DECODE_RT     ((cpuRegs.code >> 16) & 0x1F) // The rt part of the instruction register 
#define DECODE_RS     ((cpuRegs.code >> 21) & 0x1F) // The rs part of the instruction register 
#define DECODE_SA     ((cpuRegs.code >>  6) & 0x1F) // The sa part of the instruction register
#define DECODE_IMMED     ( cpuRegs.code & 0xFFFF)      // The immediate part of the instruction register
#define DECODE_OFFSET  ((((short)DECODE_IMMED * 4) + opcode_addr + 4))
#define DECODE_JUMP     (opcode_addr & 0xf0000000)|((cpuRegs.code&0x3ffffff)<<2)
#define DECODE_SYSCALL      ((opcode_addr & 0x03FFFFFF) >> 6)
#define DECODE_BREAK        (DECODE_SYSCALL)
#define DECODE_C0BC         ((cpuRegs.code >> 16) & 0x03)
#define DECODE_C1BC         ((cpuRegs.code >> 16) & 0x03)
#define DECODE_C2BC         ((cpuRegs.code >> 16) & 0x03)   
*/
/*************************CPUS REGISTERS**************************/
char *GPR_REG[32] = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};
char *COP0_REG[32] ={
	"Index","Random","EntryLo0","EntryLo1","Context","PageMask",
	"Wired","C0r7","BadVaddr","Count","EntryHi","Compare","Status",
	"Cause","EPC","PRId","Config","C0r17","C0r18","C0r19","C0r20",
	"C0r21","C0r22","C0r23","Debug","Perf","C0r26","C0r27","TagLo",
	"TagHi","ErrorPC","C0r31"
};
//floating point cop1 Floating point reg
char *COP1_REG_FP[32] ={
 	"f00","f01","f02","f03","f04","f05","f06","f07",
	"f08","f09","f10","f11","f12","f13","f14","f15",
	"f16","f17","f18","f19","f20","f21","f21","f23",
	"f24","f25","f26","f27","f28","f29","f30","f31"
};
//floating point cop1 control registers
char *COP1_REG_FCR[32] ={
 	"fcr00","fcr01","fcr02","fcr03","fcr04","fcr05","fcr06","fcr07",
	"fcr08","fcr09","fcr10","fcr11","fcr12","fcr13","fcr14","fcr15",
	"fcr16","fcr17","fcr18","fcr19","fcr20","fcr21","fcr21","fcr23",
	"fcr24","fcr25","fcr26","fcr27","fcr28","fcr29","fcr30","fcr31"
};

//floating point cop2 reg
char *COP2_REG_FP[32] ={
	"vf00","vf01","vf02","vf03","vf04","vf05","vf06","vf07",
	"vf08","vf09","vf10","vf11","vf12","vf13","vf14","vf15",
	"vf16","vf17","vf18","vf19","vf20","vf21","vf21","vf23",
	"vf24","vf25","vf26","vf27","vf28","vf29","vf30","vf31"
};
//cop2 control registers

char *COP2_REG_CTL[32] ={
	"vi00","vi01","vi02","vi03","vi04","vi05","vi06","vi07",
	"vi08","vi09","vi10","vi11","vi12","vi13","vi14","vi15",
	"Status","MACflag","ClipFlag","c2c19","R","I","Q","c2c23",
	"c2c24","c2c25","TPC","CMSAR0","FBRST","VPU-STAT","c2c30","CMSAR1"
};





//****************************************************************
void P_SpecialOpcode(char *buf);
void P_REGIMMOpcode(char *buf);
void P_UnknownOpcode(char *buf);
void P_COP0(char *buf);
void P_COP1(char *buf);
void P_COP2(char *buf);
void P_MMI_Unknown(char *buf);
void P_MMI(char *buf);
void P_MMI0(char *buf);
void P_MMI1(char *buf);
void P_MMI2(char *buf);
void P_MMI3(char *buf);
void P_COP0_Unknown(char *buf);
void P_COP0_BC0(char *buf);
void P_COP0_Func(char *buf); 
void P_COP1_BC1(char *buf);
void P_COP1_S(char *buf);
void P_COP1_W(char *buf);
void P_COP1_Unknown(char *buf);
void P_COP2_BC2(char *buf);
void P_COP2_SPECIAL(char *buf);
void P_COP2_Unknown(char *buf);
void P_COP2_SPECIAL2(char *buf);

// **********************Standard Opcodes**************************
void P_J(char *buf);
void P_JAL(char *buf);
void P_BEQ(char *buf);
void P_BNE(char *buf);
void P_BLEZ(char *buf);
void P_BGTZ(char *buf);
void P_ADDI(char *buf);
void P_ADDIU(char *buf);
void P_SLTI(char *buf);
void P_SLTIU(char *buf);
void P_ANDI(char *buf);
void P_ORI(char *buf);
void P_XORI(char *buf);
void P_LUI(char *buf);
void P_BEQL(char *buf);
void P_BNEL(char *buf);
void P_BLEZL(char *buf);
void P_BGTZL(char *buf);
void P_DADDI(char *buf);
void P_DADDIU(char *buf);
void P_LDL(char *buf);
void P_LDR(char *buf);
void P_LB(char *buf);
void P_LH(char *buf);
void P_LWL(char *buf);
void P_LW(char *buf);
void P_LBU(char *buf);
void P_LHU(char *buf);
void P_LWR(char *buf);
void P_LWU(char *buf);
void P_SB(char *buf);
void P_SH(char *buf);
void P_SWL(char *buf);
void P_SW(char *buf);
void P_SDL(char *buf);
void P_SDR(char *buf);
void P_SWR(char *buf);
void P_CACHE(char *buf);
void P_LWC1(char *buf);
void P_PREF(char *buf);
void P_LQC2(char *buf);
void P_LD(char *buf);
void P_SQC2(char *buf);
void P_SD(char *buf);
void P_LQ(char *buf);
void P_SQ(char *buf);
void P_SWC1(char *buf);
//***************end of standard opcodes*************************


//***************SPECIAL OPCODES**********************************
void P_SLL(char *buf);
void P_SRL(char *buf);
void P_SRA(char *buf);
void P_SLLV(char *buf);
void P_SRLV(char *buf);
void P_SRAV(char *buf);
void P_JR(char *buf);
void P_JALR(char *buf);
void P_SYSCALL(char *buf);
void P_BREAK(char *buf);
void P_SYNC(char *buf);
void P_MFHI(char *buf);
void P_MTHI(char *buf);
void P_MFLO(char *buf);
void P_MTLO(char *buf);
void P_DSLLV(char *buf);
void P_DSRLV(char *buf);
void P_DSRAV(char *buf);
void P_MULT(char *buf);
void P_MULTU(char *buf);
void P_DIV(char *buf);
void P_DIVU(char *buf);
void P_ADD(char *buf);
void P_ADDU(char *buf);
void P_SUB(char *buf);
void P_SUBU(char *buf);
void P_AND(char *buf);
void P_OR(char *buf);
void P_XOR(char *buf);
void P_NOR(char *buf);
void P_SLT(char *buf);
void P_SLTU(char *buf);
void P_DADD(char *buf);
void P_DADDU(char *buf);
void P_DSUB(char *buf);
void P_DSUBU(char *buf);
void P_TGE(char *buf);
void P_TGEU(char *buf);
void P_TLT(char *buf);
void P_TLTU(char *buf);
void P_TEQ(char *buf);
void P_TNE(char *buf);
void P_DSLL(char *buf);
void P_DSRL(char *buf);
void P_DSRA(char *buf);
void P_DSLL32(char *buf);
void P_DSRL32(char *buf);
void P_DSRA32(char *buf);
void P_MOVZ(char *buf);
void P_MOVN(char *buf);
void P_MFSA(char *buf);
void P_MTSA(char *buf);
//******************END OF SPECIAL OPCODES**************************

//******************REGIMM OPCODES**********************************
void P_BLTZ(char *buf);
void P_BGEZ(char *buf);
void P_BLTZL(char *buf);
void P_BGEZL(char *buf);
void P_TGEI(char *buf);
void P_TGEIU(char *buf);
void P_TLTI(char *buf);
void P_TLTIU(char *buf);
void P_TEQI(char *buf);
void P_TNEI(char *buf);
void P_BLTZAL(char *buf);
void P_BGEZAL(char *buf);
void P_BLTZALL(char *buf);
void P_BGEZALL(char *buf);
void P_MTSAB(char *buf);
void P_MTSAH(char *buf);
//*****************END OF REGIMM OPCODES*****************************
//*****************MMI OPCODES*********************************
void P_MADD(char *buf);
void P_MADDU(char *buf);
void P_PLZCW(char *buf);
void P_MADD1(char *buf);
void P_MADDU1(char *buf);
void P_MFHI1(char *buf);
void P_MTHI1(char *buf);
void P_MFLO1(char *buf);
void P_MTLO1(char *buf);
void P_MULT1(char *buf);
void P_MULTU1(char *buf);
void P_DIV1(char *buf);
void P_DIVU1(char *buf);
void P_PMFHL(char *buf);
void P_PMTHL(char *buf);
void P_PSLLH(char *buf);
void P_PSRLH(char *buf);
void P_PSRAH(char *buf);
void P_PSLLW(char *buf);
void P_PSRLW(char *buf);
void P_PSRAW(char *buf);
//*****************END OF MMI OPCODES**************************
//*************************MMI0 OPCODES************************

void P_PADDW(char *buf);  
void P_PSUBW(char *buf);  
void P_PCGTW(char *buf);  
void P_PMAXW(char *buf); 
void P_PADDH(char *buf);  
void P_PSUBH(char *buf);  
void P_PCGTH(char *buf);  
void P_PMAXH(char *buf); 
void P_PADDB(char *buf);  
void P_PSUBB(char *buf);  
void P_PCGTB(char *buf);
void P_PADDSW(char *buf); 
void P_PSUBSW(char *buf); 
void P_PEXTLW(char *buf);  
void P_PPACW(char *buf); 
void P_PADDSH(char *buf);
void P_PSUBSH(char *buf);
void P_PEXTLH(char *buf); 
void P_PPACH(char *buf);
void P_PADDSB(char *buf);
void P_PSUBSB(char *buf);
void P_PEXTLB(char *buf); 
void P_PPACB(char *buf);
void P_PEXT5(char *buf); 
void P_PPAC5(char *buf);
//***END OF MMI0 OPCODES******************************************
//**********MMI1 OPCODES**************************************
void P_PABSW(char *buf);
void P_PCEQW(char *buf);
void P_PMINW(char *buf); 
void P_PADSBH(char *buf);
void P_PABSH(char *buf);
void P_PCEQH(char *buf);
void P_PMINH(char *buf);  
void P_PCEQB(char *buf); 
void P_PADDUW(char *buf);
void P_PSUBUW(char *buf); 
void P_PEXTUW(char *buf);  
void P_PADDUH(char *buf);
void P_PSUBUH(char *buf); 
void P_PEXTUH(char *buf); 
void P_PADDUB(char *buf);
void P_PSUBUB(char *buf);
void P_PEXTUB(char *buf);
void P_QFSRV(char *buf); 
//********END OF MMI1 OPCODES***********************************
//*********MMI2 OPCODES***************************************
void P_PMADDW(char *buf);
void P_PSLLVW(char *buf);
void P_PSRLVW(char *buf); 
void P_PMSUBW(char *buf);
void P_PMFHI(char *buf);
void P_PMFLO(char *buf);
void P_PINTH(char *buf);
void P_PMULTW(char *buf);
void P_PDIVW(char *buf);
void P_PCPYLD(char *buf);
void P_PMADDH(char *buf);
void P_PHMADH(char *buf);
void P_PAND(char *buf);
void P_PXOR(char *buf); 
void P_PMSUBH(char *buf);
void P_PHMSBH(char *buf);
void P_PEXEH(char *buf);
void P_PREVH(char *buf); 
void P_PMULTH(char *buf);
void P_PDIVBW(char *buf);
void P_PEXEW(char *buf);
void P_PROT3W(char *buf);
//*****END OF MMI2 OPCODES***********************************
//*************************MMI3 OPCODES************************
void P_PMADDUW(char *buf);
void P_PSRAVW(char *buf); 
void P_PMTHI(char *buf);
void P_PMTLO(char *buf);
void P_PINTEH(char *buf); 
void P_PMULTUW(char *buf);
void P_PDIVUW(char *buf);
void P_PCPYUD(char *buf); 
void P_POR(char *buf);
void P_PNOR(char *buf);  
void P_PEXCH(char *buf);
void P_PCPYH(char *buf); 
void P_PEXCW(char *buf);
//**********************END OF MMI3 OPCODES******************** 
//****************************************************************************
//** COP0                                                                   **
//****************************************************************************
void P_MFC0(char *buf);
void P_MTC0(char *buf);
void P_BC0F(char *buf);
void P_BC0T(char *buf);
void P_BC0FL(char *buf);
void P_BC0TL(char *buf);
void P_TLBR(char *buf);
void P_TLBWI(char *buf);
void P_TLBWR(char *buf);
void P_TLBP(char *buf);
void P_ERET(char *buf);
void P_DI(char *buf);
void P_EI(char *buf);
//****************************************************************************
//** END OF COP0                                                            **
//****************************************************************************
//****************************************************************************
//** COP1 - Floating Point Unit (FPU)                                       **
//****************************************************************************
void P_MFC1(char *buf);
void P_CFC1(char *buf);
void P_MTC1(char *buf);
void P_CTC1(char *buf);
void P_BC1F(char *buf);
void P_BC1T(char *buf);
void P_BC1FL(char *buf);
void P_BC1TL(char *buf);
void P_ADD_S(char *buf);  
void P_SUB_S(char *buf);  
void P_MUL_S(char *buf);  
void P_DIV_S(char *buf);  
void P_SQRT_S(char *buf); 
void P_ABS_S(char *buf);  
void P_MOV_S(char *buf); 
void P_NEG_S(char *buf); 
void P_RSQRT_S(char *buf);  
void P_ADDA_S(char *buf); 
void P_SUBA_S(char *buf); 
void P_MULA_S(char *buf);
void P_MADD_S(char *buf); 
void P_MSUB_S(char *buf); 
void P_MADDA_S(char *buf); 
void P_MSUBA_S(char *buf);
void P_CVT_W(char *buf);
void P_MAX_S(char *buf);
void P_MIN_S(char *buf);
void P_C_F(char *buf);
void P_C_EQ(char *buf);
void P_C_LT(char *buf);
void P_C_LE(char *buf);
 void P_CVT_S(char *buf);
//****************************************************************************
//** END OF COP1                                                            **
//****************************************************************************
//****************************************************************************
//** COP2 - (VU0)                                                           **
//****************************************************************************
void P_QMFC2(char *buf); 
void P_CFC2(char *buf); 
void P_QMTC2(char *buf);
void P_CTC2(char *buf);  
void P_BC2F(char *buf);
void P_BC2T(char *buf);
void P_BC2FL(char *buf);
void P_BC2TL(char *buf);
//*****************SPECIAL 1 VUO TABLE*******************************
void P_VADDx(char *buf);       
void P_VADDy(char *buf);       
void P_VADDz(char *buf);       
void P_VADDw(char *buf);       
void P_VSUBx(char *buf);        
void P_VSUBy(char *buf);        
void P_VSUBz(char *buf);
void P_VSUBw(char *buf); 
void P_VMADDx(char *buf);
void P_VMADDy(char *buf);
void P_VMADDz(char *buf);
void P_VMADDw(char *buf);
void P_VMSUBx(char *buf);
void P_VMSUBy(char *buf);
void P_VMSUBz(char *buf);       
void P_VMSUBw(char *buf); 
void P_VMAXx(char *buf);       
void P_VMAXy(char *buf);       
void P_VMAXz(char *buf);       
void P_VMAXw(char *buf);       
void P_VMINIx(char *buf);       
void P_VMINIy(char *buf);       
void P_VMINIz(char *buf);       
void P_VMINIw(char *buf); 
void P_VMULx(char *buf);       
void P_VMULy(char *buf);       
void P_VMULz(char *buf);       
void P_VMULw(char *buf);       
void P_VMULq(char *buf);        
void P_VMAXi(char *buf);        
void P_VMULi(char *buf);        
void P_VMINIi(char *buf);
void P_VADDq(char *buf);
void P_VMADDq(char *buf);      
void P_VADDi(char *buf);       
void P_VMADDi(char *buf);      
void P_VSUBq(char *buf);        
void P_VMSUBq(char *buf);       
void P_VSUbi(char *buf);        
void P_VMSUBi(char *buf); 
void P_VADD(char *buf);        
void P_VMADD(char *buf);       
void P_VMUL(char *buf);        
void P_VMAX(char *buf);        
void P_VSUB(char *buf);         
void P_VMSUB(char *buf);       
void P_VOPMSUB(char *buf);      
void P_VMINI(char *buf);  
void P_VIADD(char *buf);       
void P_VISUB(char *buf);       
void P_VIADDI(char *buf);        
void P_VIAND(char *buf);        
void P_VIOR(char *buf);        
void P_VCALLMS(char *buf);     
void P_CALLMSR(char *buf);   
//***********************************END OF SPECIAL1 VU0 TABLE*****************************
//******************************SPECIAL2 VUO TABLE*****************************************
void P_VADDAx(char *buf);      
void P_VADDAy(char *buf);      
void P_VADDAz(char *buf);      
void P_VADDAw(char *buf);      
void P_VSUBAx(char *buf);      
void P_VSUBAy(char *buf);      
void P_VSUBAz(char *buf);      
void P_VSUBAw(char *buf);
void P_VMADDAx(char *buf);     
void P_VMADDAy(char *buf);     
void P_VMADDAz(char *buf);     
void P_VMADDAw(char *buf);     
void P_VMSUBAx(char *buf);     
void P_VMSUBAy(char *buf);     
void P_VMSUBAz(char *buf);     
void P_VMSUBAw(char *buf);
void P_VITOF0(char *buf);      
void P_VITOF4(char *buf);      
void P_VITOF12(char *buf);     
void P_VITOF15(char *buf);     
void P_VFTOI0(char *buf);      
void P_VFTOI4(char *buf);      
void P_VFTOI12(char *buf);     
void P_VFTOI15(char *buf);
void P_VMULAx(char *buf);      
void P_VMULAy(char *buf);      
void P_VMULAz(char *buf);      
void P_VMULAw(char *buf);      
void P_VMULAq(char *buf);      
void P_VABS(char *buf);        
void P_VMULAi(char *buf);      
void P_VCLIPw(char *buf);
void P_VADDAq(char *buf);      
void P_VMADDAq(char *buf);     
void P_VADDAi(char *buf);      
void P_VMADDAi(char *buf);     
void P_VSUBAq(char *buf);      
void P_VMSUBAq(char *buf);     
void P_VSUBAi(char *buf);      
void P_VMSUBAi(char *buf);
void P_VADDA(char *buf);       
void P_VMADDA(char *buf);      
void P_VMULA(char *buf);       
void P_VSUBA(char *buf);       
void P_VMSUBA(char *buf);      
void P_VOPMULA(char *buf);     
void P_VNOP(char *buf);   
void P_VMONE(char *buf);       
void P_VMR32(char *buf);       
void P_VLQI(char *buf);        
void P_VSQI(char *buf);        
void P_VLQD(char *buf);        
void P_VSQD(char *buf);   
void P_VDIV(char *buf);        
void P_VSQRT(char *buf);       
void P_VRSQRT(char *buf);      
void P_VWAITQ(char *buf);     
void P_VMTIR(char *buf);       
void P_VMFIR(char *buf);       
void P_VILWR(char *buf);       
void P_VISWR(char *buf);  
void P_VRNEXT(char *buf);      
void P_VRGET(char *buf);       
void P_VRINIT(char *buf);      
void P_VRXOR(char *buf);  
//************************************END OF SPECIAL2 VUO TABLE****************************     


/*
    CPU: Instructions encoded by opcode field.
    31---------26---------------------------------------------------0
    |  opcode   |                                                   |
    ------6----------------------------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
000 | *1    | *2    | J     | JAL   | BEQ   | BNE   | BLEZ  | BGTZ  |
001 | ADDI  | ADDIU | SLTI  | SLTIU | ANDI  | ORI   | XORI  | LUI   |
010 | *3    | *4    |  *5   | ---   | BEQL  | BNEL  | BLEZL | BGTZL |
011 | DADDI |DADDIU | LDL   | LDR   |  *6   |  ---  |  LQ   | SQ    |
100 | LB    | LH    | LWL   | LW    | LBU   | LHU   | LWR   | LWU   |
101 | SB    | SH    | SWL   | SW    | SDL   | SDR   | SWR   | CACHE |
110 | ---   | LWC1  | ---   | PREF  | ---   | ---   | LQC2  | LD    |
111 | ---   | SWC1  | ---   | ---   | ---   | ---   | SQC2  | SD    |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
     *1 = SPECIAL, see SPECIAL list    *2 = REGIMM, see REGIMM list
     *3 = COP0                         *4 = COP1
     *5 = COP2                         *6 = MMI table
*/
void (*OpcodePrintTable[64])(char *buf) = {
    P_SpecialOpcode, P_REGIMMOpcode, P_J,             P_JAL,           P_BEQ,          P_BNE,           P_BLEZ,  P_BGTZ,
    P_ADDI,          P_ADDIU,        P_SLTI,          P_SLTIU,         P_ANDI,         P_ORI,           P_XORI,  P_LUI,
    P_COP0,          P_COP1,         P_COP2,          P_UnknownOpcode, P_BEQL,         P_BNEL,          P_BLEZL, P_BGTZL,
    P_DADDI,         P_DADDIU,       P_LDL,           P_LDR,           P_MMI,          P_UnknownOpcode, P_LQ,    P_SQ,
    P_LB,            P_LH,           P_LWL,           P_LW,            P_LBU,          P_LHU,           P_LWR,   P_LWU,
    P_SB,            P_SH,           P_SWL,           P_SW,            P_SDL,          P_SDR,           P_SWR,   P_CACHE,
    P_UnknownOpcode, P_LWC1,         P_UnknownOpcode, P_PREF,          P_UnknownOpcode,P_UnknownOpcode, P_LQC2,  P_LD,
    P_UnknownOpcode, P_SWC1,         P_UnknownOpcode, P_UnknownOpcode, P_UnknownOpcode,P_UnknownOpcode, P_SQC2,  P_SD
};


	  /*
     SPECIAL: Instr. encoded by function field when opcode field = SPECIAL
    31---------26------------------------------------------5--------0
    | = SPECIAL |                                         | function|
    ------6----------------------------------------------------6-----
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
000 | SLL   | ---   | SRL   | SRA   | SLLV  |  ---  | SRLV  | SRAV  |
001 | JR    | JALR  | MOVZ  | MOVN  |SYSCALL| BREAK |  ---  | SYNC  |
010 | MFHI  | MTHI  | MFLO  | MTLO  | DSLLV |  ---  | DSRLV | DSRAV |
011 | MULT  | MULTU | DIV   | DIVU  | ----  |  ---  | ----  | ----- |
100 | ADD   | ADDU  | SUB   | SUBU  | AND   | OR    | XOR   | NOR   |
101 | MFSA  | MTSA  | SLT   | SLTU  | DADD  | DADDU | DSUB  | DSUBU |
110 | TGE   | TGEU  | TLT   | TLTU  | TEQ   |  ---  | TNE   |  ---  |
111 | DSLL  |  ---  | DSRL  | DSRA  |DSLL32 |  ---  |DSRL32 |DSRA32 |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
*/

void (*SpecialPrintTable[64])(char *buf) = {
    P_SLL,           P_UnknownOpcode, P_SRL,           P_SRA,           P_SLLV,    P_UnknownOpcode, P_SRLV,          P_SRAV,
    P_JR,            P_JALR,          P_MOVZ,          P_MOVN,          P_SYSCALL, P_BREAK,         P_UnknownOpcode, P_SYNC,
    P_MFHI,          P_MTHI,          P_MFLO,          P_MTLO,          P_DSLLV,   P_UnknownOpcode, P_DSRLV,         P_DSRAV,
    P_MULT,          P_MULTU,         P_DIV,           P_DIVU,          P_UnknownOpcode,P_UnknownOpcode,P_UnknownOpcode,P_UnknownOpcode,
    P_ADD,           P_ADDU,          P_SUB,           P_SUBU,          P_AND,     P_OR,            P_XOR,           P_NOR,
    P_MFSA ,         P_MTSA ,         P_SLT,           P_SLTU,          P_DADD,    P_DADDU,         P_DSUB,          P_DSUBU,
    P_TGE,           P_TGEU,          P_TLT,           P_TLTU,          P_TEQ,     P_UnknownOpcode, P_TNE,           P_UnknownOpcode,
    P_DSLL,          P_UnknownOpcode, P_DSRL,          P_DSRA,          P_DSLL32,  P_UnknownOpcode, P_DSRL32,        P_DSRA32
};

/*
    REGIMM: Instructions encoded by the rt field when opcode field = REGIMM.
    31---------26----------20-------16------------------------------0
    | = REGIMM  |          |   rt    |                              |
    ------6---------------------5------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
 00 | BLTZ  | BGEZ  | BLTZL | BGEZL |  ---  |  ---  |  ---  |  ---  |
 01 | TGEI  | TGEIU | TLTI  | TLTIU | TEQI  |  ---  | TNEI  |  ---  |
 10 | BLTZAL| BGEZAL|BLTZALL|BGEZALL|  ---  |  ---  |  ---  |  ---  |
 11 | MTSAB | MTSAH |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
*/
void (*REGIMMPrintTable[32])(char *buf) = {
    P_BLTZ,   P_BGEZ,   P_BLTZL,   P_BGEZL,   P_UnknownOpcode, P_UnknownOpcode, P_UnknownOpcode, P_UnknownOpcode,
    P_TGEI,   P_TGEIU,  P_TLTI,    P_TLTIU,   P_TEQI,          P_UnknownOpcode, P_TNEI,          P_UnknownOpcode,
    P_BLTZAL, P_BGEZAL, P_BLTZALL, P_BGEZALL, P_UnknownOpcode, P_UnknownOpcode, P_UnknownOpcode, P_UnknownOpcode,
    P_MTSAB,  P_MTSAH , P_UnknownOpcode, P_UnknownOpcode, P_UnknownOpcode, P_UnknownOpcode, P_UnknownOpcode, P_UnknownOpcode,
};
/*
    MMI: Instr. encoded by function field when opcode field = MMI
    31---------26------------------------------------------5--------0
    | = MMI     |                                         | function|
    ------6----------------------------------------------------6-----
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
000 | MADD  | MADDU |  ---  |  ---  | PLZCW |  ---  |  ---  |  ---  |
001 |  *1   |  *2   |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
010 | MFHI1 | MTHI1 | MFLO1 | MTLO1 |  ---  |  ---  |  ---  |  ---  |
011 | MULT1 | MULTU1| DIV1  | DIVU1 |  ---  |  ---  |  ---  |  ---  |
100 | MADD1 | MADDU1|  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
101 |  *3   |  *4   |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
110 | PMFHL | PMTHL |  ---  |  ---  | PSLLH |  ---  | PSRLH | PSRAH |
111 |  ---  |  ---  |  ---  |  ---  | PSLLW |  ---  | PSRLW | PSRAW |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|

     *1 = see MMI0 table    *2 = see MMI2 Table
     *3 = see MMI1 table    *4 = see MMI3 Table
*/
void (*MMIPrintTable[64])(char *buf) = {
    P_MADD,                    P_MADDU,                  P_MMI_Unknown,          P_MMI_Unknown,          P_PLZCW,          P_MMI_Unknown,       P_MMI_Unknown,          P_MMI_Unknown,
    P_MMI0,                    P_MMI2,                   P_MMI_Unknown,          P_MMI_Unknown,          P_MMI_Unknown,    P_MMI_Unknown,       P_MMI_Unknown,          P_MMI_Unknown,
    P_MFHI1,                   P_MTHI1,                  P_MFLO1,                P_MTLO1,                P_MMI_Unknown,    P_MMI_Unknown,       P_MMI_Unknown,          P_MMI_Unknown,
    P_MULT1,                   P_MULTU1,                 P_DIV1,                 P_DIVU1,                P_MMI_Unknown,    P_MMI_Unknown,       P_MMI_Unknown,          P_MMI_Unknown,
    P_MADD1,                   P_MADDU1,                 P_MMI_Unknown,          P_MMI_Unknown,          P_MMI_Unknown,    P_MMI_Unknown,       P_MMI_Unknown,          P_MMI_Unknown,
    P_MMI1 ,                   P_MMI3,                   P_MMI_Unknown,          P_MMI_Unknown,          P_MMI_Unknown,    P_MMI_Unknown,       P_MMI_Unknown,          P_MMI_Unknown,
    P_PMFHL,                   P_PMTHL,                  P_MMI_Unknown,          P_MMI_Unknown,          P_PSLLH,          P_MMI_Unknown,       P_PSRLH,                P_PSRAH,
    P_MMI_Unknown,             P_MMI_Unknown,            P_MMI_Unknown,          P_MMI_Unknown,          P_PSLLW,          P_MMI_Unknown,       P_PSRLW,                P_PSRAW,
};
/*
  MMI0: Instr. encoded by function field when opcode field = MMI & MMI0

    31---------26------------------------------10--------6-5--------0
    |          |                              |function  | MMI0    |
    ------6----------------------------------------------------6-----
    |--000--|--001--|--010--|--011--| lo
000 |PADDW  | PSUBW | PCGTW | PMAXW |
001 |PADDH  | PSUBH | PCGTH | PMAXH |
010 |PADDB  | PSUBB | PCGTB |  ---  |
011 | ---   | ---   |  ---  |  ---  |
100 |PADDSW |PSUBSW |PEXTLW | PPACW |
101 |PADDSH |PSUBSH |PEXTLH | PPACH |
110 |PADDSB |PSUBSB |PEXTLB | PPACB |
111 | ---   |  ---  | PEXT5 | PPAC5 |
 hi |-------|-------|-------|-------|
*/
void (*MMI0PrintTable[32])(char *buf) = { 
 P_PADDW,         P_PSUBW,         P_PCGTW,          P_PMAXW,       
 P_PADDH,         P_PSUBH,         P_PCGTH,          P_PMAXH,        
 P_PADDB,         P_PSUBB,         P_PCGTB,          P_MMI_Unknown,
 P_MMI_Unknown,   P_MMI_Unknown,   P_MMI_Unknown,    P_MMI_Unknown,
 P_PADDSW,        P_PSUBSW,        P_PEXTLW,         P_PPACW,        
 P_PADDSH,        P_PSUBSH,        P_PEXTLH,         P_PPACH,        
 P_PADDSB,        P_PSUBSB,        P_PEXTLB,         P_PPACB,        
 P_MMI_Unknown,   P_MMI_Unknown,   P_PEXT5,          P_PPAC5,        
};
/*
  MMI1: Instr. encoded by function field when opcode field = MMI & MMI1

    31---------26------------------------------------------5--------0
    |           |                               |function  | MMI1    |
    ------6----------------------------------------------------6-----
    |--000--|--001--|--010--|--011--| lo
000 |  ---  | PABSW | PCEQW | PMINW |
001 |PADSBH | PABSH | PCEQH | PMINH | 
010 |  ---  |  ---  | PCEQB |  ---  |
011 |  ---  |  ---  |  ---  |  ---  |
100 |PADDUW |PSUBUW |PEXTUW |  ---  |
101 |PADDUH |PSUBUH |PEXTUH |  ---  |
110 |PADDUB |PSUBUB |PEXTUB | QFSRV |
111 |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|
*/
void (*MMI1PrintTable[32])(char *buf) = { 
 P_MMI_Unknown, P_PABSW,         P_PCEQW,       P_PMINW, 
 P_PADSBH,      P_PABSH,         P_PCEQH,       P_PMINH, 
 P_MMI_Unknown, P_MMI_Unknown,   P_PCEQB,       P_MMI_Unknown, 
 P_MMI_Unknown, P_MMI_Unknown,   P_MMI_Unknown, P_MMI_Unknown, 
 P_PADDUW,      P_PSUBUW,        P_PEXTUW,      P_MMI_Unknown,  
 P_PADDUH,      P_PSUBUH,        P_PEXTUH,      P_MMI_Unknown, 
 P_PADDUB,      P_PSUBUB,        P_PEXTUB,      P_QFSRV, 
 P_MMI_Unknown, P_MMI_Unknown,   P_MMI_Unknown, P_MMI_Unknown, 
};

/*
  MMI2: Instr. encoded by function field when opcode field = MMI & MMI2

    31---------26------------------------------------------5--------0
    |           |                              |function   | MMI2    |
    ------6----------------------------------------------------6-----
    |--000--|--001--|--010--|--011--| lo
000 |PMADDW |  ---  |PSLLVW |PSRLVW |
001 |PMSUBW |  ---  |  ---  |  ---  | 
010 |PMFHI  |PMFLO  |PINTH  |  ---  |
011 |PMULTW |PDIVW  |PCPYLD |  ---  |
100 |PMADDH |PHMADH | PAND  |  PXOR |
101 |PMSUBH |PHMSBH |  ---  |  ---  |
110 | ---   |  ---  | PEXEH | PREVH |
111 |PMULTH |PDIVBW | PEXEW |PROT3W |
 hi |-------|-------|-------|-------|
*/
void (*MMI2PrintTable[32])(char *buf) = { 
 P_PMADDW,        P_MMI_Unknown,   P_PSLLVW,        P_PSRLVW, 
 P_PMSUBW,        P_MMI_Unknown,   P_MMI_Unknown,   P_MMI_Unknown,
 P_PMFHI,         P_PMFLO,         P_PINTH,         P_MMI_Unknown,
 P_PMULTW,        P_PDIVW,         P_PCPYLD,        P_MMI_Unknown,
 P_PMADDH,        P_PHMADH,        P_PAND,          P_PXOR, 
 P_PMSUBH,        P_PHMSBH,        P_MMI_Unknown,   P_MMI_Unknown, 
 P_MMI_Unknown,   P_MMI_Unknown,   P_PEXEH,         P_PREVH, 
 P_PMULTH,        P_PDIVBW,        P_PEXEW,         P_PROT3W, 
};
/*
  MMI3: Instr. encoded by function field when opcode field = MMI & MMI3
    31---------26------------------------------------------5--------0
    |           |                               |function  | MMI3   |
    ------6----------------------------------------------------6-----
    |--000--|--001--|--010--|--011--| lo
000 |PMADDUW|  ---  |  ---  |PSRAVW |
001 |  ---  |  ---  |  ---  |  ---  | 
010 |PMTHI  | PMTLO |PINTEH |  ---  |
011 |PMULTUW| PDIVUW|PCPYUD |  ---  |
100 |  ---  |  ---  |  POR  | PNOR  |
101 |  ---  |  ---  |  ---  |  ---  |
110 |  ---  |  ---  | PEXCH | PCPYH |
111 |  ---  |  ---  | PEXCW |  ---  |
 hi |-------|-------|-------|-------|
 */
void (*MMI3PrintTable[32])(char *buf) = { 
 P_PMADDUW,     P_MMI_Unknown, P_MMI_Unknown, P_PSRAVW, 
 P_MMI_Unknown, P_MMI_Unknown, P_MMI_Unknown, P_MMI_Unknown,
 P_PMTHI,       P_PMTLO,       P_PINTEH,      P_MMI_Unknown,
 P_PMULTUW,     P_PDIVUW,      P_PCPYUD,      P_MMI_Unknown,
 P_MMI_Unknown, P_MMI_Unknown, P_POR,         P_PNOR,  
 P_MMI_Unknown, P_MMI_Unknown, P_MMI_Unknown, P_MMI_Unknown,
 P_MMI_Unknown, P_MMI_Unknown, P_PEXCH,       P_PCPYH, 
 P_MMI_Unknown, P_MMI_Unknown, P_PEXCW,       P_MMI_Unknown,
};
/*
    COP0: Instructions encoded by the rs field when opcode = COP0.
    31--------26-25------21 ----------------------------------------0
    |  = COP0   |   fmt   |                                         |
    ------6----------5-----------------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
 00 | MFC0  |  ---  |  ---  |  ---  | MTC0  |  ---  |  ---  |  ---  |
 01 |  *1   |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 10 |  *2   |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 11 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
     *1=BC See BC0 list       *2 = TLB instr, see TLB list
*/
void (*COP0PrintTable[32])(char *buf) = {
    P_MFC0,         P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_MTC0,         P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown,
    P_COP0_BC0,     P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown,
    P_COP0_Func,    P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown,
    P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown,
};
/*
    BC0: Instructions encoded by the rt field when opcode = COP0 & rs field=BC0
    31--------26-25------21 ----------------------------------------0
    |  = COP0   |   fmt   |                                         |
    ------6----------5-----------------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
 00 | BC0F  | BC0T  | BC0FL | BC0TL |  ---  |  ---  |  ---  |  ---  |
 01 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 10 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 11 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
*/
void (*COP0BC0PrintTable[32])(char *buf) = {
    P_BC0F,         P_BC0T,         P_BC0FL,        P_BC0TL,   P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown,
    P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown,
    P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown,
    P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown,
};
/*
    C0=Instructions encode by function field when Opcode field=COP0 & rs field=C0
    31---------26------------------------------------------5--------0
    |           |                                         |         |
    ------6----------------------------------------------------6-----
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
000 | ---   |  TLBR | TLBWI |  ---  |  ---  |  ---  | TLBWR |  ---  |
001 | TLBP  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
010 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
011 | ERET  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
100 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
101 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
110 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
111 |  EI   |  DI   |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
*/
void (*COP0C0PrintTable[64])(char *buf) = {
    P_COP0_Unknown, P_TLBR,         P_TLBWI,        P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_TLBWR,        P_COP0_Unknown,
    P_TLBP,         P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown,
    P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown,
    P_ERET,         P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown,
    P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown,
    P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown,
    P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown,
    P_EI,           P_DI,           P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown, P_COP0_Unknown
};
/*
    COP1: Instructions encoded by the fmt field when opcode = COP1.
    31--------26-25------21 ----------------------------------------0
    |  = COP1   |   fmt   |                                         |
    ------6----------5-----------------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
 00 | MFC1  |  ---  | CFC1  |  ---  | MTC1  |  ---  | CTC1  |  ---  |
 01 | *1    |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 10 | *2    |  ---  |  ---  |  ---  | *3    |  ---  |  ---  |  ---  |
 11 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
     *1 = BC instructions, see BC1 list   *2 = S instr, see FPU list            
     *3 = W instr, see FPU list 
*/
void (*COP1PrintTable[32])(char *buf) = {
    P_MFC1,         P_COP1_Unknown, P_CFC1,         P_COP1_Unknown, P_MTC1,         P_COP1_Unknown, P_CTC1,         P_COP1_Unknown,
    P_COP1_BC1,     P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown,
    P_COP1_S,       P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_W,       P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown,
    P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown,
};
/*
    BC1: Instructions encoded by the rt field when opcode = COP1 & rs field=BC1
    31--------26-25------21 ----------------------------------------0
    |  = COP1   |   fmt   |                                         |
    ------6----------5-----------------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
 00 | BC1F  | BC1T  | BC1FL | BC1TL |  ---  |  ---  |  ---  |  ---  |
 01 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 10 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 11 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|   
*/
void (*COP1BC1PrintTable[32])(char *buf) = {
    P_BC1F,         P_BC1T,         P_BC1FL,        P_BC1TL,        P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown,
    P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown,
    P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown,
    P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown, P_COP1_Unknown,
};
/*
    FPU: Instructions encoded by the function field when opcode = COP1
         and rs = S
    31--------26-25------21 -------------------------------5--------0
    |  = COP1   |  = S    |                               | function|
    ------6----------5-----------------------------------------6-----
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
000 | ADD.S | SUB.S | MUL.S | DIV.S | SQRT.S| ABS.S | MOV.S | NEG.S |
001 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  | ---   |
010 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |RSQRT.S|  ---  |
011 | ADDA.S| SUBA.S| MULA.S|  ---  | MADD.S| MSUB.S|MADDA.S|MSUBA.S|
100 |  ---  | ---   |  ---  |  ---  | CVT.W |  ---  |  ---  |  ---  |
101 | MAX.S | MIN.S |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
110 | C.F   | ---   | C.EQ  |  ---  | C.LT  |  ---  |  C.LE |  ---  |
111 | ---   | ---   |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------| 
*/
void (*COP1SPrintTable[64])(char *buf) = {
P_ADD_S,       P_SUB_S,       P_MUL_S,       P_DIV_S,       P_SQRT_S,      P_ABS_S,       P_MOV_S,       P_NEG_S, 
P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,   
P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_RSQRT_S,     P_COP1_Unknown,  
P_ADDA_S,      P_SUBA_S,      P_MULA_S,      P_COP1_Unknown,P_MADD_S,      P_MSUB_S,      P_MADDA_S,     P_MSUBA_S,
P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_CVT_W,       P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown, 
P_MAX_S,       P_MIN_S,       P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown, 
P_C_F,         P_COP1_Unknown,P_C_EQ,        P_COP1_Unknown,P_C_LT,        P_COP1_Unknown,P_C_LE,        P_COP1_Unknown, 
P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown, 
};
/*
    FPU: Instructions encoded by the function field when opcode = COP1
         and rs = W
    31--------26-25------21 -------------------------------5--------0
    |  = COP1   |  = W    |                               | function|
    ------6----------5-----------------------------------------6-----
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
000 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
001 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
010 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
011 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
100 | CVT.S |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
101 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
110 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
111 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|       
*/  
void (*COP1WPrintTable[64])(char *buf) = { 
P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,   	
P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,   
P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,   
P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,   
P_CVT_S,       P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,   
P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,   
P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,   
P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,P_COP1_Unknown,   
};

//*************************************************************
//COP2 TABLES :) 
//*************************************************************
/* 
   COP2: Instructions encoded by the fmt field when opcode = COP2.
    31--------26-25------21 ----------------------------------------0
    |  = COP2   |   fmt   |                                         |
    ------6----------5-----------------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
 00 |  ---  | QMFC2 | CFC2  |  ---  |  ---  | QMTC2 | CTC2  |  ---  |
 01 | *1    |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 10 | *2    | *2    | *2    | *2    | *2    | *2    | *2    | *2    |
 11 | *2    | *2    | *2    | *2    | *2    | *2    | *2    | *2    |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
     *1 = BC instructions, see BC2 list   *2 =see special1 table
*/
void (*COP2PrintTable[32])(char *buf) = {
    P_COP2_Unknown, P_QMFC2,        P_CFC2,         P_COP2_Unknown, P_COP2_Unknown, P_QMTC2,        P_CTC2,         P_COP2_Unknown,
    P_COP2_BC2,     P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown,
    P_COP2_SPECIAL, P_COP2_SPECIAL, P_COP2_SPECIAL, P_COP2_SPECIAL, P_COP2_SPECIAL, P_COP2_SPECIAL, P_COP2_SPECIAL, P_COP2_SPECIAL,
	P_COP2_SPECIAL, P_COP2_SPECIAL, P_COP2_SPECIAL, P_COP2_SPECIAL, P_COP2_SPECIAL, P_COP2_SPECIAL, P_COP2_SPECIAL, P_COP2_SPECIAL,

    
};
/*
    BC2: Instructions encoded by the rt field when opcode = COP2 & rs field=BC1
    31--------26-25------21 ----------------------------------------0
    |  = COP2   |   rs=BC2|                                         |
    ------6----------5-----------------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
 00 | BC2F  | BC2T  | BC2FL | BC2TL |  ---  |  ---  |  ---  |  ---  |
 01 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 10 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 11 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|   
 */
void (*COP2BC2PrintTable[32])(char *buf) = {
    P_BC2F,         P_BC2T,         P_BC2FL,        P_BC2TL,        P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown,
    P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown,
    P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown,
    P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown, P_COP2_Unknown,
}; 
/*
    Special1 table : instructions encode by function field when opcode=COP2 & rs field=Special1
    31---------26---------------------------------------------------0
    |  =COP2   | rs=Special                                         |
    ------6----------------------------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
000 |VADDx  |VADDy  |VADDz  |VADDw  |VSUBx  |VSUBy  |VSUBz  |VSUBw  |
001 |VMADDx |VMADDy |VMADDz |VMADDw |VMSUBx |VMSUBy |VMSUBz |VMSUBw |
010 |VMAXx  |VMAXy  |VMAXz  |VMAXw  |VMINIx |VMINIy |VMINIz |VMINIw |
011 |VMULx  |VMULy  |VMULz  |VMULw  |VMULq  |VMAXi  |VMULi  |VMINIi |
100 |VADDq  |VMADDq |VADDi  |VMADDi |VSUBq  |VMSUBq |VSUbi  |VMSUBi |
101 |VADD   |VMADD  |VMUL   |VMAX   |VSUB   |VMSUB  |VOPMSUB|VMINI  |
110 |VIADD  |VISUB  |VIADDI |  ---  |VIAND  |VIOR   |  ---  |  ---  |
111 |VCALLMS|CALLMSR|  ---  |  ---  |  *1   |  *1   |  *1   |  *1   |
 hi |-------|-------|-------|-------|-------|-------|-------|-------| 
    *1=see special2 table  
*/
void (*COP2SPECIAL1PrintTable[64])(char *buf) = 
{ 
 P_VADDx,       P_VADDy,       P_VADDz,       P_VADDw,       P_VSUBx,        P_VSUBy,        P_VSUBz,        P_VSUBw,  
 P_VMADDx,      P_VMADDy,      P_VMADDz,      P_VMADDw,      P_VMSUBx,       P_VMSUBy,       P_VMSUBz,       P_VMSUBw, 
 P_VMAXx,       P_VMAXy,       P_VMAXz,       P_VMAXw,       P_VMINIx,       P_VMINIy,       P_VMINIz,       P_VMINIw, 
 P_VMULx,       P_VMULy,       P_VMULz,       P_VMULw,       P_VMULq,        P_VMAXi,        P_VMULi,        P_VMINIi,
 P_VADDq,       P_VMADDq,      P_VADDi,       P_VMADDi,      P_VSUBq,        P_VMSUBq,       P_VSUbi,        P_VMSUBi, 
 P_VADD,        P_VMADD,       P_VMUL,        P_VMAX,        P_VSUB,         P_VMSUB,        P_VOPMSUB,      P_VMINI,  
 P_VIADD,       P_VISUB,       P_VIADDI,      P_COP2_Unknown,P_VIAND,        P_VIOR,         P_COP2_Unknown, P_COP2_Unknown,
 P_VCALLMS,     P_CALLMSR,     P_COP2_Unknown,P_COP2_Unknown,P_COP2_SPECIAL2,P_COP2_SPECIAL2,P_COP2_SPECIAL2,P_COP2_SPECIAL2,  

};
/*
  Special2 table : instructions encode by function field when opcode=COp2 & rs field=Special2

     31---------26---------------------------------------------------0
     |  =COP2   | rs=Special2                                        |
     ------6----------------------------------------------------------
     |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
0000 |VADDAx |VADDAy |VADDAz |VADDAw |VSUBAx |VSUBAy |VSUBAz |VSUBAw |
0001 |VMADDAx|VMADDAy|VMADDAz|VMADDAw|VMSUBAx|VMSUBAy|VMSUBAz|VMSUBAw|
0010 |VITOF0 |VITOF4 |VITOF12|VITOF15|VFTOI0 |VFTOI4 |VFTOI12|VFTOI15|
0011 |VMULAx |VMULAy |VMULAz |VMULAw |VMULAq |VABS   |VMULAi |VCLIPw |
0100 |VADDAq |VMADDAq|VADDAi |VMADDAi|VSUBAq |VMSUBAq|VSUBAi |VMSUBAi|
0101 |VADDA  |VMADDA |VMULA  |  ---  |VSUBA  |VMSUBA |VOPMULA|VNOP   |
0110 |VMONE  |VMR32  |  ---  |  ---  |VLQI   |VSQI   |VLQD   |VSQD   |
0111 |VDIV   |VSQRT  |VRSQRT |VWAITQ |VMTIR  |VMFIR  |VILWR  |VISWR  |
1000 |VRNEXT |VRGET  |VRINIT |VRXOR  |  ---  |  ---  |  ---  |  ---  |
1001 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
1010 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
1011 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
1100 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
1101 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  | 
1110 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
1111 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi  |-------|-------|-------|-------|-------|-------|-------|-------| 
*/
void (*COP2SPECIAL2PrintTable[128])(char *buf) = 
{
 P_VADDAx      ,P_VADDAy      ,P_VADDAz      ,P_VADDAw      ,P_VSUBAx      ,P_VSUBAy      ,P_VSUBAz      ,P_VSUBAw,
 P_VMADDAx     ,P_VMADDAy     ,P_VMADDAz     ,P_VMADDAw     ,P_VMSUBAx     ,P_VMSUBAy     ,P_VMSUBAz     ,P_VMSUBAw,
 P_VITOF0      ,P_VITOF4      ,P_VITOF12     ,P_VITOF15     ,P_VFTOI0      ,P_VFTOI4      ,P_VFTOI12     ,P_VFTOI15,
 P_VMULAx      ,P_VMULAy      ,P_VMULAz      ,P_VMULAw      ,P_VMULAq      ,P_VABS        ,P_VMULAi      ,P_VCLIPw,
 P_VADDAq      ,P_VMADDAq     ,P_VADDAi      ,P_VMADDAi     ,P_VSUBAq      ,P_VMSUBAq     ,P_VSUBAi      ,P_VMSUBAi,
 P_VADDA       ,P_VMADDA      ,P_VMULA       ,P_COP2_Unknown,P_VSUBA       ,P_VMSUBA      ,P_VOPMULA     ,P_VNOP,   
 P_VMONE       ,P_VMR32       ,P_COP2_Unknown,P_COP2_Unknown,P_VLQI        ,P_VSQI        ,P_VLQD        ,P_VSQD,   
 P_VDIV        ,P_VSQRT       ,P_VRSQRT      ,P_VWAITQ      ,P_VMTIR       ,P_VMFIR       ,P_VILWR       ,P_VISWR,  
 P_VRNEXT      ,P_VRGET       ,P_VRINIT      ,P_VRXOR       ,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown, 
 P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,
 P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,
 P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,
 P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,
 P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,
 P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,
 P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,P_COP2_Unknown,
};

//**************************TABLES CALLS***********************

static char dbuf[1024];
static char obuf[1024];

char *disR5900Fasm(u32 code, u32 pc) {
	u32 scode = cpuRegs.code;
	opcode_addr = pc;
	cpuRegs.code = code;
	OpcodePrintTable[(code) >> 26](dbuf);

	sprintf(obuf, "%08X:\t%s", pc, dbuf);

	cpuRegs.code = scode;
	return obuf;
}

void P_SpecialOpcode(char *buf)
{
 SpecialPrintTable[DECODE_FUNCTION](buf);
}
void P_REGIMMOpcode(char *buf)
{
 REGIMMPrintTable[DECODE_RT](buf);
}

//***********COP0 TABLE CALLS********************************

void P_COP0(char *buf)
{
 COP0PrintTable[DECODE_RS](buf);
}
void P_COP0_BC0(char *buf)
{
 
 COP0BC0PrintTable[DECODE_C0BC](buf);
}
void P_COP0_Func(char *buf)
{
   
   COP0C0PrintTable[DECODE_FUNCTION](buf);
}

//*****************MMI TABLES CALLS**************************
void P_MMI(char *buf)
{
  
  MMIPrintTable[DECODE_FUNCTION](buf);
}
void P_MMI0(char *buf)
{
  MMI0PrintTable[DECODE_SA](buf);
}
void P_MMI1(char *buf)
{
  MMI1PrintTable[DECODE_SA](buf);
}
void P_MMI2(char *buf)
{
  MMI2PrintTable[DECODE_SA](buf);
}
void P_MMI3(char *buf)
{
  MMI3PrintTable[DECODE_SA](buf);
}
//****************END OF MMI TABLES CALLS**********************
//COP1 TABLECALLS*******************************************
void P_COP1(char *buf)
{
   
   COP1PrintTable[DECODE_RS](buf);
}
void P_COP1_BC1(char *buf)
{
 COP1BC1PrintTable[DECODE_C1BC](buf);
}
void P_COP1_S(char *buf)
{
   COP1SPrintTable[DECODE_FUNCTION](buf);
}
void P_COP1_W(char *buf)
{
   COP1WPrintTable[DECODE_FUNCTION](buf);
}
//**********************END OF COP1 TABLE CALLS

//*************************************************************
//************************COP2**********************************
void P_COP2(char *buf)
{
	
  COP2PrintTable[DECODE_RS](buf);
}
void P_COP2_BC2(char *buf)
{
 COP2BC2PrintTable[DECODE_C2BC](buf);
}
void P_COP2_SPECIAL(char *buf)
{
 COP2SPECIAL1PrintTable[DECODE_FUNCTION ](buf);

}
void P_COP2_SPECIAL2(char *buf)
{

	COP2SPECIAL2PrintTable[(cpuRegs.code & 0x3) |  ((cpuRegs.code >> 4) & 0x7c)](buf);

}

//**************************UNKNOWN****************************
void P_UnknownOpcode(char *buf)
{
  strcpy(buf, "?????");
}
void P_COP0_Unknown(char *buf)
{
  strcpy(buf, "COP0 ??");
}
void P_COP1_Unknown(char *buf)
{
  strcpy(buf, "COP1 ??");
}
void P_COP2_Unknown(char *buf)
{
	strcpy(buf,"COP2 ??");
}

void P_MMI_Unknown(char *buf)
{
	strcpy(buf,"MMI ??");
}



//*************************************************************

//*****************SOME DECODE STUFF***************************
#define dFindSym(i) { \
	char *str = disR5900GetSym(i); \
	if (str != NULL) sprintf(buf, "%s %s", buf, str); \
}

char *jump_decode(void)
{
    static char buf[256];
    unsigned long addr;
    addr = DECODE_JUMP;
    sprintf(buf, "0x%08X", addr);
    
	dFindSym(addr);
    return buf;
}

char *offset_decode(void)
{
    static char buf[256];
    unsigned long addr;
    addr = DECODE_OFFSET;
    sprintf(buf, "0x%08X", addr);
	dFindSym(addr);
    return buf;
}

//*********************END OF DECODE ROUTINES******************

//********************* Standard Opcodes***********************
void P_J(char *buf)      { sprintf(buf, "j\t%s",                  jump_decode());}
void P_JAL(char *buf)    { sprintf(buf, "jal\t%s",                jump_decode());} 
void P_BEQ(char *buf)    { sprintf(buf, "beq\t%s, %s, %s",        GPR_REG[DECODE_RS], GPR_REG[DECODE_RT], offset_decode()); }
void P_BNE(char *buf)    { sprintf(buf, "bne\t%s, %s, %s",        GPR_REG[DECODE_RS], GPR_REG[DECODE_RT], offset_decode()); }
void P_BLEZ(char *buf)   { sprintf(buf, "blez\t%s, %s",           GPR_REG[DECODE_RS], offset_decode()); }
void P_BGTZ(char *buf)   { sprintf(buf, "bgtz\t%s, %s",           GPR_REG[DECODE_RS], offset_decode()); }
void P_ADDI(char *buf)   { sprintf(buf, "addi\t%s, %s, 0x%04X",   GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED);}
void P_ADDIU(char *buf)  { sprintf(buf, "addiu\t%s, %s, 0x%04X",  GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED);}
void P_SLTI(char *buf)   { sprintf(buf, "slti\t%s, %s, 0x%04X",   GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED); }
void P_SLTIU(char *buf)  { sprintf(buf, "sltiu\t%s, %s, 0x%04X",  GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED); }
void P_ANDI(char *buf)   { sprintf(buf, "andi\t%s, %s, 0x%04X",   GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED);}
void P_ORI(char *buf)    { sprintf(buf, "ori\t%s, %s, 0x%04X",    GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED); }
void P_XORI(char *buf)   { sprintf(buf, "xori\t%s, %s, 0x%04X",   GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED); }
void P_LUI(char *buf)    { sprintf(buf, "lui\t%s, 0x%04X",        GPR_REG[DECODE_RT], DECODE_IMMED); }
void P_BEQL(char *buf)   { sprintf(buf, "beql\t%s, %s, %s",       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT], offset_decode()); }
void P_BNEL(char *buf)   { sprintf(buf, "bnel\t%s, %s, %s",       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT], offset_decode()); }
void P_BLEZL(char *buf)  { sprintf(buf, "blezl\t%s, %s",          GPR_REG[DECODE_RS], offset_decode()); }
void P_BGTZL(char *buf)  { sprintf(buf, "bgtzl\t%s, %s",          GPR_REG[DECODE_RS], offset_decode()); }
void P_DADDI(char *buf)  { sprintf(buf, "daddi\t%s, %s, 0x%04X",  GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED); }
void P_DADDIU(char *buf) { sprintf(buf, "daddiu\t%s, %s, 0x%04X", GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED); }
void P_LDL(char *buf)    { sprintf(buf, "ldl\t%s, 0x%04X(%s)",    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_LDR(char *buf)    { sprintf(buf, "ldr\t%s, 0x%04X(%s)",    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_LB(char *buf)     { sprintf(buf, "lb\t%s, 0x%04X(%s)",     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_LH(char *buf)     { sprintf(buf, "lh\t%s, 0x%04X(%s)",     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_LWL(char *buf)    { sprintf(buf, "lwl\t%s, 0x%04X(%s)",    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_LW(char *buf)     { sprintf(buf, "lw\t%s, 0x%04X(%s)",     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_LBU(char *buf)    { sprintf(buf, "lbu\t%s, 0x%04X(%s)",    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_LHU(char *buf)    { sprintf(buf, "lhu\t%s, 0x%04X(%s)",    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_LWR(char *buf)    { sprintf(buf, "lwr\t%s, 0x%04X(%s)",    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_LWU(char *buf)    { sprintf(buf, "lwu\t%s, 0x%04X(%s)",    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_SB(char *buf)     { sprintf(buf, "sb\t%s, 0x%04X(%s)",     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_SH(char *buf)     { sprintf(buf, "sh\t%s, 0x%04X(%s)",     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_SWL(char *buf)    { sprintf(buf, "swl\t%s, 0x%04X(%s)",    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_SW(char *buf)     { sprintf(buf, "sw\t%s, 0x%04X(%s)",     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_SDL(char *buf)    { sprintf(buf, "sdl\t%s, 0x%04X(%s)",    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_SDR(char *buf)    { sprintf(buf, "sdr\t%s, 0x%04X(%s)",    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_SWR(char *buf)    { sprintf(buf, "swr\t%s, 0x%04X(%s)",    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_LD(char *buf)     { sprintf(buf, "ld\t%s, 0x%04X(%s)",     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_SD(char *buf)     { sprintf(buf, "sd\t%s, 0x%04X(%s)",     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_LQ(char *buf)     { sprintf(buf, "lq\t%s, 0x%04X(%s)",     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_SQ(char *buf)     { sprintf(buf, "sq\t%s, 0x%04X(%s)",     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_SWC1(char *buf)   { sprintf(buf, "swc1\t%s, 0x%04X(%s)",   COP1_REG_FP[DECODE_FT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_SQC2(char *buf)   { sprintf(buf, "sqc2\t%s, 0x%04X(%s)",   COP2_REG_FP[DECODE_FT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_PREF(char *buf)   { strcpy(buf,  "pref ---");/*sprintf(buf, "PREF\t%s, 0x%04X(%s)",   GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[RS]); */}
void P_LWC1(char *buf)   { sprintf(buf, "lwc1\t%s, 0x%04X(%s)",   COP1_REG_FP[DECODE_FT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void P_LQC2(char *buf)   { sprintf(buf, "lqc2\t%s, 0x%04X(%s)",   COP2_REG_FP[DECODE_FT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
//********************END OF STANDARD OPCODES*************************

void P_SLL(char *buf)
{
   if (cpuRegs.code == 0x00000000)
        strcpy(buf, "nop");
    else
        sprintf(buf, "sll\t%s, %s, 0x%02X", GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA);
}

void P_SRL(char *buf)    { sprintf(buf, "srl\t%s, %s, 0x%02X", GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void P_SRA(char *buf)    { sprintf(buf, "sra\t%s, %s, 0x%02X", GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void P_SLLV(char *buf)   { sprintf(buf, "sllv\t%s, %s, %s",    GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], GPR_REG[DECODE_RS]); }
void P_SRLV(char *buf)   { sprintf(buf, "srlv\t%s, %s, %s",    GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], GPR_REG[DECODE_RS]);}
void P_SRAV(char *buf)   { sprintf(buf, "srav\t%s, %s, %s",    GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], GPR_REG[DECODE_RS]); }
void P_JR(char *buf)     { sprintf(buf, "jr\t%s",              GPR_REG[DECODE_RS]); }

void P_JALR(char *buf)
{
    int rd = DECODE_RD;

    if (rd == 31)
        sprintf(buf, "jalr\t%s", GPR_REG[DECODE_RS]);
    else
        sprintf(buf, "jalr\t%s, %s", GPR_REG[rd], GPR_REG[DECODE_RS]);
}


void P_SYNC(char *buf)    { sprintf(buf,  "SYNC");}
void P_MFHI(char *buf)    { sprintf(buf, "mfhi\t%s",          GPR_REG[DECODE_RD]); }
void P_MTHI(char *buf)    { sprintf(buf, "mthi\t%s",          GPR_REG[DECODE_RS]); }
void P_MFLO(char *buf)    { sprintf(buf, "mflo\t%s",          GPR_REG[DECODE_RD]); }
void P_MTLO(char *buf)    { sprintf(buf, "mtlo\t%s",          GPR_REG[DECODE_RS]); }
void P_DSLLV(char *buf)   { sprintf(buf, "dsllv\t%s, %s, %s", GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], GPR_REG[DECODE_RS]); }
void P_DSRLV(char *buf)   { sprintf(buf, "dsrlv\t%s, %s, %s", GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], GPR_REG[DECODE_RS]); }
void P_DSRAV(char *buf)   { sprintf(buf, "dsrav\t%s, %s, %s", GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], GPR_REG[DECODE_RS]); }
void P_MULT(char *buf)    { sprintf(buf, "mult\t%s, %s, %s", GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]);}	
void P_MULTU(char *buf)   { sprintf(buf, "multu\t%s, %s, %s", GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]);}
void P_DIV(char *buf)     { sprintf(buf, "div\t%s, %s",       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_DIVU(char *buf)    { sprintf(buf, "divu\t%s, %s",      GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_ADD(char *buf)     { sprintf(buf, "add\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_ADDU(char *buf)    { sprintf(buf, "addu\t%s, %s, %s",  GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_SUB(char *buf)     { sprintf(buf, "sub\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_SUBU(char *buf)    { sprintf(buf, "subu\t%s, %s, %s",  GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_AND(char *buf)     { sprintf(buf, "and\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_OR(char *buf)      { sprintf(buf, "or\t%s, %s, %s",    GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_XOR(char *buf)     { sprintf(buf, "xor\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_NOR(char *buf)     { sprintf(buf, "nor\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_SLT(char *buf)     { sprintf(buf, "slt\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_SLTU(char *buf)    { sprintf(buf, "sltu\t%s, %s, %s",  GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_DADD(char *buf)    { sprintf(buf, "dadd\t%s, %s, %s",  GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_DADDU(char *buf)   { sprintf(buf, "daddu\t%s, %s, %s", GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_DSUB(char *buf)    { sprintf(buf, "dsub\t%s, %s, %s",  GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_DSUBU(char *buf)   { sprintf(buf, "dsubu\t%s, %s, %s", GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_TGE(char *buf)     { sprintf(buf, "tge\t%s, %s",       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_TGEU(char *buf)    { sprintf(buf, "tgeu\t%s, %s",      GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_TLT(char *buf)     { sprintf(buf, "tlt\t%s, %s",       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_TLTU(char *buf)    { sprintf(buf, "tltu\t%s, %s",      GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_TEQ(char *buf)     { sprintf(buf, "teq\t%s, %s",       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_TNE(char *buf)     { sprintf(buf, "tne\t%s, %s",       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_DSLL(char *buf)    { sprintf(buf, "dsll\t%s, %s, 0x%02X",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void P_DSRL(char *buf)    { sprintf(buf, "dsrl\t%s, %s, 0x%02X",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void P_DSRA(char *buf)    { sprintf(buf, "dsra\t%s, %s, 0x%02X",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void P_DSLL32(char *buf)  { sprintf(buf, "dsll32\t%s, %s, 0x%02X", GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void P_DSRL32(char *buf)  { sprintf(buf, "dsrl32\t%s, %s, 0x%02X", GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void P_DSRA32(char *buf)  { sprintf(buf, "dsra32\t%s, %s, 0x%02X", GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void P_MOVZ(char *buf)    { sprintf(buf, "movz\t%s, %s, %s", GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_MOVN(char *buf)    { sprintf(buf, "movn\t%s, %s, %s", GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_MFSA(char *buf)    { sprintf(buf, "mfsa\t%s",          GPR_REG[DECODE_RD]);} 
void P_MTSA(char *buf)    { sprintf(buf, "mtsa\t%s",          GPR_REG[DECODE_RS]);}
//*** unsupport (yet) cpu opcodes
void P_SYSCALL(char *buf) { strcpy(buf,  "syscall ---");/*sprintf(buf, "syscall\t0x%05X",   DECODE_SYSCALL);*/}
void P_BREAK(char *buf)   { strcpy(buf,  "break   ---");/*sprintf(buf, "break\t0x%05X",     DECODE_BREAK); */}
void P_CACHE(char *buf)   { strcpy(buf,  "cache   ---");/*sprintf(buf, "cache\t%s, 0x%04X(%s)",  GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); */}
//************************REGIMM OPCODES***************************
void P_BLTZ(char *buf)    { sprintf(buf, "bltz\t%s, %s",     GPR_REG[DECODE_RS], offset_decode()); }
void P_BGEZ(char *buf)    { sprintf(buf, "bgez\t%s, %s",     GPR_REG[DECODE_RS], offset_decode()); }
void P_BLTZL(char *buf)   { sprintf(buf, "bltzl\t%s, %s",    GPR_REG[DECODE_RS], offset_decode()); }
void P_BGEZL(char *buf)   { sprintf(buf, "bgezl\t%s, %s",    GPR_REG[DECODE_RS], offset_decode()); }
void P_TGEI(char *buf)    { sprintf(buf, "tgei\t%s, 0x%04X", GPR_REG[DECODE_RS], DECODE_IMMED); }
void P_TGEIU(char *buf)   { sprintf(buf, "tgeiu\t%s,0x%04X", GPR_REG[DECODE_RS], DECODE_IMMED); }
void P_TLTI(char *buf)    { sprintf(buf, "tlti\t%s, 0x%04X", GPR_REG[DECODE_RS], DECODE_IMMED); }
void P_TLTIU(char *buf)   { sprintf(buf, "tltiu\t%s,0x%04X", GPR_REG[DECODE_RS], DECODE_IMMED); }
void P_TEQI(char *buf)    { sprintf(buf, "teqi\t%s, 0x%04X", GPR_REG[DECODE_RS], DECODE_IMMED); }
void P_TNEI(char *buf)    { sprintf(buf, "tnei\t%s, 0x%04X", GPR_REG[DECODE_RS], DECODE_IMMED); }
void P_BLTZAL(char *buf)  { sprintf(buf, "bltzal\t%s, %s",   GPR_REG[DECODE_RS], offset_decode()); }
void P_BGEZAL(char *buf)  { sprintf(buf, "bgezal\t%s, %s",   GPR_REG[DECODE_RS], offset_decode()); }
void P_BLTZALL(char *buf) { sprintf(buf, "bltzall\t%s, %s",  GPR_REG[DECODE_RS], offset_decode()); }
void P_BGEZALL(char *buf) { sprintf(buf, "bgezall\t%s, %s",  GPR_REG[DECODE_RS], offset_decode()); }
void P_MTSAB(char *buf)   { sprintf(buf, "mtsab\t%s, 0x%04X", GPR_REG[DECODE_RS], DECODE_IMMED);}
void P_MTSAH(char *buf)   { sprintf(buf, "mtsah\t%s, 0x%04X", GPR_REG[DECODE_RS], DECODE_IMMED);}


//***************************SPECIAL 2 CPU OPCODES*******************
const char* pmfhl_sub[] = { "lw", "uw", "slw", "lh", "sh" };

void P_MADD(char *buf)    { sprintf(buf, "madd\t%s, %s %s",        GPR_REG[DECODE_RD],GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_MADDU(char *buf)   { sprintf(buf, "maddu\t%s, %s %s",       GPR_REG[DECODE_RD],GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]);}
void P_PLZCW(char *buf)   { sprintf(buf, "plzcw\t%s, %s",          GPR_REG[DECODE_RD], GPR_REG[DECODE_RS]); }
void P_MADD1(char *buf)   { sprintf(buf, "madd1\t%s, %s %s",       GPR_REG[DECODE_RD],GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_MADDU1(char *buf)  { sprintf(buf, "maddu1\t%s, %s %s",      GPR_REG[DECODE_RD],GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_MFHI1(char *buf)   { sprintf(buf, "mfhi1\t%s",          GPR_REG[DECODE_RD]); }
void P_MTHI1(char *buf)   { sprintf(buf, "mthi1\t%s",          GPR_REG[DECODE_RS]); }
void P_MFLO1(char *buf)   { sprintf(buf, "mflo1\t%s",          GPR_REG[DECODE_RD]); }
void P_MTLO1(char *buf)   { sprintf(buf, "mtlo1\t%s",          GPR_REG[DECODE_RS]); }
void P_MULT1(char *buf)   { sprintf(buf, "mult1\t%s, %s, %s",        GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_MULTU1(char *buf)  { sprintf(buf, "multu1\t%s, %s, %s",        GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]);}
void P_DIV1(char *buf)    { sprintf(buf, "div1\t%s, %s",       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_DIVU1(char *buf)   { sprintf(buf, "divu1\t%s, %s",       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
//that have parametres that i haven't figure out how to display...
void P_PMFHL(char *buf)   { sprintf(buf, "pmfhl.%s \t%s",          pmfhl_sub[DECODE_SA], GPR_REG[DECODE_RD]); }
void P_PMTHL(char *buf)   { sprintf(buf, "pmthl.%s \t%s",          pmfhl_sub[DECODE_SA], GPR_REG[DECODE_RS]); }
void P_PSLLH(char *buf)   { sprintf(buf, "psllh   \t%s, %s, 0x%02X",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void P_PSRLH(char *buf)   { sprintf(buf, "psrlh   \t%s, %s, 0x%02X",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA);}
void P_PSRAH(char *buf)   { sprintf(buf, "psrah   \t%s, %s, 0x%02X",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA);}
void P_PSLLW(char *buf)   { sprintf(buf,  "psllw   \t%s, %s, 0x%02X",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA);}
void P_PSRLW(char *buf)   { sprintf(buf,  "psrlw   \t%s, %s, 0x%02X",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA);}
void P_PSRAW(char *buf)   { sprintf(buf,  "psraw   \t%s, %s, 0x%02X",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA);}
//***************************END OF SPECIAL OPCODES******************
//*************************MMI0 OPCODES************************

void P_PADDW(char *buf){  sprintf(buf,  "paddw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }  
void P_PSUBW(char *buf){  sprintf(buf,  "psubw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }  
void P_PCGTW(char *buf){  sprintf(buf,  "pcgtw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }  
void P_PMAXW(char *buf){  sprintf(buf,  "pmaxw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PADDH(char *buf){  sprintf(buf,  "paddh\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }  
void P_PSUBH(char *buf){  sprintf(buf,  "psubh\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }  
void P_PCGTH(char *buf){  sprintf(buf,  "pcgth\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }  
void P_PMAXH(char *buf){  sprintf(buf,  "pmaxh\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PADDB(char *buf){  sprintf(buf,  "paddb\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }  
void P_PSUBB(char *buf){  sprintf(buf,  "psubb\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }  
void P_PCGTB(char *buf){  sprintf(buf,  "pcgtb\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_PADDSW(char *buf){ sprintf(buf,  "paddsw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PSUBSW(char *buf){ sprintf(buf,  "psubsw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PEXTLW(char *buf){ sprintf(buf,  "pextlw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }  
void P_PPACW(char *buf) { sprintf(buf,  "ppacw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PADDSH(char *buf){ sprintf(buf,  "paddsh\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PSUBSH(char *buf){ sprintf(buf,  "psubsh\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_PEXTLH(char *buf){ sprintf(buf,  "pextlh\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }  
void P_PPACH(char *buf) { sprintf(buf,  "ppach\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PADDSB(char *buf){ sprintf(buf,  "paddsb\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PSUBSB(char *buf){ sprintf(buf,  "psubsb\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PEXTLB(char *buf){ sprintf(buf,  "pextlb\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }  
void P_PPACB(char *buf) { sprintf(buf,  "ppacb\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PEXT5(char *buf) { sprintf(buf,  "pext5\t%s, %s",      GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]); }  
void P_PPAC5(char *buf) { sprintf(buf,  "ppac5\t%s, %s",      GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]); } 
//**********END OF MMI0 OPCODES*********************************
//**********MMI1 OPCODES**************************************
void P_PABSW(char *buf){  sprintf(buf,  "pabsw%s, %s",      GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]); }
void P_PCEQW(char *buf){  sprintf(buf,  "pceqw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_PMINW(char *buf){  sprintf(buf,  "pminw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PADSBH(char *buf){ sprintf(buf,  "padsbh\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_PABSH(char *buf){  sprintf(buf,  "pabsh%s, %s",      GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]); }
void P_PCEQH(char *buf){  sprintf(buf,  "pceqh\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_PMINH(char *buf){  sprintf(buf,  "pminh\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }  
void P_PCEQB(char *buf){  sprintf(buf,  "pceqb\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PADDUW(char *buf){ sprintf(buf,  "padduw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_PSUBUW(char *buf){ sprintf(buf,  "psubuw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PEXTUW(char *buf){ sprintf(buf,  "pextuw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }  
void P_PADDUH(char *buf){ sprintf(buf,  "padduh\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_PSUBUH(char *buf){ sprintf(buf,  "psubuh\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PEXTUH(char *buf){ sprintf(buf,  "pextuh\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PADDUB(char *buf){ sprintf(buf,  "paddub\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_PSUBUB(char *buf){ sprintf(buf,  "psubub\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_PEXTUB(char *buf){ sprintf(buf,  "pextub\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_QFSRV(char *buf) { sprintf(buf,  "qfsrv\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
//********END OF MMI1 OPCODES***********************************
//*********MMI2 OPCODES***************************************
void P_PMADDW(char *buf){ sprintf(buf,  "pmaddw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PSLLVW(char *buf){ sprintf(buf,  "psllvw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PSRLVW(char *buf){ sprintf(buf,  "psrlvw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PMSUBW(char *buf){ sprintf(buf,  "msubw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PMFHI(char *buf){  sprintf(buf,  "pmfhi\t%s",          GPR_REG[DECODE_RD]); }
void P_PMFLO(char *buf){  sprintf(buf,  "pmflo\t%s",          GPR_REG[DECODE_RD]); } 
void P_PINTH(char *buf){  sprintf(buf,  "pinth\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PMULTW(char *buf){ sprintf(buf,  "pmultw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PDIVW(char *buf){  sprintf(buf,  "pdivw\t%s, %s",      GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PCPYLD(char *buf){ sprintf(buf,  "pcpyld\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PMADDH(char *buf){ sprintf(buf,  "pmaddh\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PHMADH(char *buf){ sprintf(buf,  "phmadh\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PAND(char *buf){   sprintf(buf,  "pand\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_PXOR(char *buf){   sprintf(buf,  "pxor\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_PMSUBH(char *buf){ sprintf(buf,  "pmsubh\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PHMSBH(char *buf){ sprintf(buf,  "phmsbh\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PEXEH(char *buf){  sprintf(buf,  "pexeh\t%s, %s",      GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]); } 
void P_PREVH(char *buf){  sprintf(buf,  "prevh\t%s, %s",      GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]); }  
void P_PMULTH(char *buf){ sprintf(buf,  "pmulth\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PDIVBW(char *buf){ sprintf(buf,  "pdivbw\t%s, %s",      GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PEXEW(char *buf){  sprintf(buf,  "pexew\t%s, %s",      GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]); } 
void P_PROT3W(char *buf){ sprintf(buf,  "prot3w\t%s, %s",      GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]); } 
//*****END OF MMI2 OPCODES***********************************
//*************************MMI3 OPCODES************************
void P_PMADDUW(char *buf){ sprintf(buf, "pmadduw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], GPR_REG[DECODE_RS]); }
void P_PSRAVW(char *buf){  sprintf(buf, "psravw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], GPR_REG[DECODE_RS]); }  
void P_PMTHI(char *buf){   sprintf(buf, "pmthi\t%s",          GPR_REG[DECODE_RS]); }
void P_PMTLO(char *buf){   sprintf(buf, "pmtlo\t%s",          GPR_REG[DECODE_RS]); }
void P_PINTEH(char *buf){  sprintf(buf, "pinteh\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PMULTUW(char *buf){ sprintf(buf, "pmultuw\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_PDIVUW(char *buf){  sprintf(buf, "pdivuw\t%s, %s",       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_PCPYUD(char *buf){  sprintf(buf, "pcpyud\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); } 
void P_POR(char *buf){     sprintf(buf, "por\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void P_PNOR(char *buf){    sprintf(buf, "pnor\t%s, %s, %s",   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }  
void P_PEXCH(char *buf){   sprintf(buf, "pexch\t%s, %s",       GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]);}
void P_PCPYH(char *buf){   sprintf(buf, "pcpyh\t%s, %s",       GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]);} 
void P_PEXCW(char *buf){   sprintf(buf, "pexcw\t%s, %s",       GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]);}
//**********************END OF MMI3 OPCODES******************** 
//****************************************************************************
//** COP0                                                                   **
//****************************************************************************
void P_MFC0(char *buf){  sprintf(buf, "mfc0\t%s, %s",  GPR_REG[DECODE_RT], COP0_REG[DECODE_FS]); }
void P_MTC0(char *buf){  sprintf(buf, "mtc0\t%s, %s",  GPR_REG[DECODE_RT], COP0_REG[DECODE_FS]); }
void P_BC0F(char *buf){  sprintf(buf, "bc0f\t%s",          offset_decode()); }
void P_BC0T(char *buf){  sprintf(buf, "bc0t\t%s",          offset_decode()); }
void P_BC0FL(char *buf){ sprintf(buf, "bc0fl\t%s",         offset_decode()); }
void P_BC0TL(char *buf){ sprintf(buf, "bc0tl\t%s",         offset_decode()); }
void P_TLBR(char *buf){  strcpy(buf,"tlbr");}
void P_TLBWI(char *buf){ strcpy(buf,"tlbwi");}
void P_TLBWR(char *buf){ strcpy(buf,"tlbwr");}
void P_TLBP(char *buf){  strcpy(buf,"tlbp");}
void P_ERET(char *buf){  strcpy(buf,"eret");}
void P_DI(char *buf){    strcpy(buf,"di");}
void P_EI(char *buf){    strcpy(buf,"ei");}
//****************************************************************************
//** END OF COP0                                                            **
//****************************************************************************
//****************************************************************************
//** COP1 - Floating Point Unit (FPU)                                       **
//****************************************************************************
void P_MFC1(char *buf){   sprintf(buf, "mfc1\t%s, %s",      GPR_REG[DECODE_RT], COP1_REG_FP[DECODE_FS]);  }
void P_CFC1(char *buf){   sprintf(buf, "cfc1\t%s, %s",      GPR_REG[DECODE_RT], COP1_REG_FCR[DECODE_FS]); }
void P_MTC1(char *buf){   sprintf(buf, "mtc1\t%s, %s",      GPR_REG[DECODE_RT], COP1_REG_FP[DECODE_FS]);  }
void P_CTC1(char *buf){   sprintf(buf, "ctc1\t%s, %s",      GPR_REG[DECODE_RT], COP1_REG_FCR[DECODE_FS]); }
void P_BC1F(char *buf){   sprintf(buf, "bc1f\t%s",          offset_decode()); }
void P_BC1T(char *buf){   sprintf(buf, "bc1t\t%s",          offset_decode()); }
void P_BC1FL(char *buf){  sprintf(buf, "bc1fl\t%s",         offset_decode()); }
void P_BC1TL(char *buf){  sprintf(buf, "bc1tl\t%s",         offset_decode()); }
void P_ADD_S(char *buf){  sprintf(buf, "add.s\t%s, %s, %s", COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]);}  
void P_SUB_S(char *buf){  sprintf(buf, "sub.s\t%s, %s, %s", COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]);}  
void P_MUL_S(char *buf){  sprintf(buf, "mul.s\t%s, %s, %s", COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]);}  
void P_DIV_S(char *buf){  sprintf(buf, "div.s\t%s, %s, %s", COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }  
void P_SQRT_S(char *buf){ sprintf(buf, "sqrt.s\t%s, %s",   COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FT]); } 
void P_ABS_S(char *buf){  sprintf(buf, "abs.s\t%s, %s",     COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS]); }  
void P_MOV_S(char *buf){  sprintf(buf, "mov.s\t%s, %s",     COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS]); } 
void P_NEG_S(char *buf){  sprintf(buf, "neg.s\t%s, %s",     COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS]);} 
void P_RSQRT_S(char *buf){sprintf(buf, "rsqrt.s\t%s, %s, %s", COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]);}  
void P_ADDA_S(char *buf){ sprintf(buf, "adda.s\t%s, %s",     COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); } 
void P_SUBA_S(char *buf){ sprintf(buf, "suba.s\t%s, %s",     COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); } 
void P_MULA_S(char *buf){ sprintf(buf, "mula.s\t%s, %s",   COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }
void P_MADD_S(char *buf){ sprintf(buf, "madd.s\t%s, %s, %s", COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); } 
void P_MSUB_S(char *buf){ sprintf(buf, "msub.s\t%s, %s, %s", COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); } 
void P_MADDA_S(char *buf){sprintf(buf, "madda.s\t%s, %s",   COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); } 
void P_MSUBA_S(char *buf){sprintf(buf, "msuba.s\t%s, %s",   COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }
void P_CVT_W(char *buf){  sprintf(buf, "cvt.w.s\t%s, %s",   COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS]); }
void P_MAX_S(char *buf){  sprintf(buf, "max.s\t%s, %s, %s", COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]);}
void P_MIN_S(char *buf){  sprintf(buf, "min.s\t%s, %s, %s", COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]);}
void P_C_F(char *buf){    sprintf(buf, "c.f.s\t%s, %s",     COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }
void P_C_EQ(char *buf){   sprintf(buf, "c.eq.s\t%s, %s",    COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }
void P_C_LT(char *buf){   sprintf(buf, "c.lt.s\t%s, %s",    COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }
void P_C_LE(char *buf){   sprintf(buf, "c.le.s\t%s, %s",    COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); } 
void P_CVT_S(char *buf){  sprintf(buf, "cvt.s.w\t%s, %s",   COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS]); }
//****************************************************************************
//** END OF COP1                                                            **
//****************************************************************************
//****************************************************************************
//** COP2 - (VU0)                                                           **
//****************************************************************************
void P_QMFC2(char *buf){   sprintf(buf, "qmfc2\t%s, %s",      GPR_REG[DECODE_RT], COP2_REG_FP[DECODE_FS]);  } 
void P_CFC2(char *buf){    sprintf(buf, "cfc2\t%s, %s",      GPR_REG[DECODE_RT], COP2_REG_CTL[DECODE_FS]); } 
void P_QMTC2(char *buf){   sprintf(buf, "qmtc2\t%s, %s",      GPR_REG[DECODE_RT], COP2_REG_FP[DECODE_FS]); } 
void P_CTC2(char *buf){    sprintf(buf, "ctc2\t%s, %s",      GPR_REG[DECODE_RT], COP2_REG_CTL[DECODE_FS]); }  
void P_BC2F(char *buf){    sprintf(buf, "bc2f\t%s",          offset_decode()); }
void P_BC2T(char *buf){    sprintf(buf, "bc2t\t%s",          offset_decode()); }
void P_BC2FL(char *buf){   sprintf(buf, "bc2fl\t%s",         offset_decode()); }
void P_BC2TL(char *buf){   sprintf(buf, "bc2tl\t%s",         offset_decode()); }
//******************************SPECIAL 1 VUO TABLE****************************************
#define _X ((cpuRegs.code>>24) & 1)
#define _Y ((cpuRegs.code>>23) & 1)
#define _Z ((cpuRegs.code>>22) & 1)
#define _W ((cpuRegs.code>>21) & 1)

const char *dest_string(void)
{
 static char str[5];
 int i;
 i = 0;
 if(_X) str[i++] = 'x';
 if(_Y) str[i++] = 'y';
 if(_Z) str[i++] = 'z';
 if(_W) str[i++] = 'w';
  str[i++] = 0;
 return (const char *)str;
}

char dest_fsf()
{
	const arr[4] = { 'x', 'y', 'z', 'w' };
	return arr[(cpuRegs.code>>21)&3];
}

char dest_ftf()
{
	const arr[4] = { 'x', 'y', 'z', 'w' };
	return arr[(cpuRegs.code>>23)&3];
}

void P_VADDx(char *buf){sprintf(buf, "vaddx.%s %s, %s, %sx", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VADDy(char *buf){sprintf(buf, "vaddy.%s %s, %s, %sy", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VADDz(char *buf){sprintf(buf, "vaddz.%s %s, %s, %sz", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VADDw(char *buf){sprintf(buf, "vaddw.%s %s, %s, %sw", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VSUBx(char *buf){sprintf(buf, "vsubx.%s %s, %s, %sx", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VSUBy(char *buf){sprintf(buf, "vsuby.%s %s, %s, %sy", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VSUBz(char *buf){sprintf(buf, "vsubz.%s %s, %s, %sz", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VSUBw(char *buf){sprintf(buf, "vsubw.%s %s, %s, %sw", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMADDx(char *buf){sprintf(buf, "vmaddx.%s %s, %s, %sx", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMADDy(char *buf){sprintf(buf, "vmaddy.%s %s, %s, %sy", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMADDz(char *buf){sprintf(buf, "vmaddz.%s %s, %s, %sz", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMADDw(char *buf){sprintf(buf, "vmaddw.%s %s, %s, %sw", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMSUBx(char *buf){sprintf(buf, "vmsubx.%s %s, %s, %sx", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMSUBy(char *buf){sprintf(buf, "vmsuby.%s %s, %s, %sy", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMSUBz(char *buf){sprintf(buf, "vmsubz.%s %s, %s, %sz", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMSUBw(char *buf){sprintf(buf, "vmsubw.%s %s, %s, %sw", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMAXx(char *buf){sprintf(buf, "vmaxx.%s %s, %s, %sx", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMAXy(char *buf){sprintf(buf, "vmaxy.%s %s, %s, %sy", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMAXz(char *buf){sprintf(buf, "vmaxz.%s %s, %s, %sz", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMAXw(char *buf){sprintf(buf, "vmaxw.%s %s, %s, %sw", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMINIx(char *buf){sprintf(buf, "vminix.%s %s, %s, %sx", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMINIy(char *buf){sprintf(buf, "vminiy.%s %s, %s, %sy", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); ;}
void P_VMINIz(char *buf){sprintf(buf, "vminiz.%s %s, %s, %sz", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMINIw(char *buf){sprintf(buf, "vminiw.%s %s, %s, %sw", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMULx(char *buf){sprintf(buf,"vmulx.%s %s,%s,%sx", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMULy(char *buf){sprintf(buf,"vmuly.%s %s,%s,%sy", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMULz(char *buf){sprintf(buf,"vmulz.%s %s,%s,%sz", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMULw(char *buf){sprintf(buf,"vmulw.%s %s,%s,%sw", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMULq(char *buf){sprintf(buf,"vmulq.%s %s,%s,Q",dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VMAXi(char *buf){sprintf(buf,"vmaxi.%s %s,%s,I",dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VMULi(char *buf){sprintf(buf,"vmuli.%s %s,%s,I",dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VMINIi(char *buf){sprintf(buf,"vminii.%s %s,%s,I",dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VADDq(char *buf){sprintf(buf,"vaddq.%s %s,%s,Q",dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VMADDq(char *buf){sprintf(buf,"vmaddq.%s %s,%s,Q",dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VADDi(char *buf){sprintf(buf,"vaddi.%s %s,%s,I",dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VMADDi(char *buf){sprintf(buf,"vmaddi.%s %s,%s,I",dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VSUBq(char *buf){sprintf(buf,"vsubq.%s %s,%s,Q",dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VMSUBq(char *buf){sprintf(buf,"vmsubq.%s %s,%s,Q",dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VSUbi(char *buf){sprintf(buf,"vsubi.%s %s,%s,I",dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VMSUBi(char *buf){sprintf(buf,"vmsubi.%s %s,%s,I",dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VADD(char *buf){sprintf(buf, "vadd.%s %s, %s, %s", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMADD(char *buf){sprintf(buf, "vmadd.%s %s, %s, %s", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMUL(char *buf){sprintf(buf, "vmul.%s %s, %s, %s", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMAX(char *buf){sprintf(buf, "vmax.%s %s, %s, %s", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VSUB(char *buf){sprintf(buf, "vsub.%s %s, %s, %s", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMSUB(char *buf){sprintf(buf, "vmsub.%s %s, %s, %s", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VOPMSUB(char *buf){sprintf(buf, "vopmsub.xyz %s, %s, %s", COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMINI(char *buf){sprintf(buf, "vmini.%s %s, %s, %s", dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VIADD(char *buf){sprintf(buf,"viadd %s, %s, %s", COP2_REG_CTL[DECODE_SA], COP2_REG_CTL[DECODE_FS], COP2_REG_CTL[DECODE_FT]);}
void P_VISUB(char *buf){sprintf(buf,"visub %s, %s, %s", COP2_REG_CTL[DECODE_SA], COP2_REG_CTL[DECODE_FS], COP2_REG_CTL[DECODE_FT]);}
void P_VIADDI(char *buf){sprintf(buf,"viaddi %s, %s, 0x%x", COP2_REG_CTL[DECODE_FT], COP2_REG_CTL[DECODE_FS], DECODE_SA);}
void P_VIAND(char *buf){sprintf(buf,"viand %s, %s, %s", COP2_REG_CTL[DECODE_SA], COP2_REG_CTL[DECODE_FS], COP2_REG_CTL[DECODE_FT]);}
void P_VIOR(char *buf){sprintf(buf,"vior %s, %s, %s", COP2_REG_CTL[DECODE_SA], COP2_REG_CTL[DECODE_FS], COP2_REG_CTL[DECODE_FT]);}
void P_VCALLMS(char *buf){strcpy(buf,"vcallms");}
void P_CALLMSR(char *buf){strcpy(buf,"callmsr");}
//***********************************END OF SPECIAL1 VU0 TABLE*****************************
//******************************SPECIAL2 VUO TABLE*****************************************
void P_VADDAx(char *buf){sprintf(buf,"vaddax.%s ACC,%s,%sx",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VADDAy(char *buf){sprintf(buf,"vadday.%s ACC,%s,%sy",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VADDAz(char *buf){sprintf(buf,"vaddaz.%s ACC,%s,%sz",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VADDAw(char *buf){sprintf(buf,"vaddaw.%s ACC,%s,%sw",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VSUBAx(char *buf){sprintf(buf,"vsubax.%s ACC,%s,%sx",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VSUBAy(char *buf){sprintf(buf,"vsubay.%s ACC,%s,%sy",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VSUBAz(char *buf){sprintf(buf,"vsubaz.%s ACC,%s,%sz",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VSUBAw(char *buf){sprintf(buf,"vsubaw.%s ACC,%s,%sw",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMADDAx(char *buf){sprintf(buf,"vmaddax.%s ACC,%s,%sx",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMADDAy(char *buf){sprintf(buf,"vmadday.%s ACC,%s,%sy",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMADDAz(char *buf){sprintf(buf,"vmaddaz.%s ACC,%s,%sz",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMADDAw(char *buf){sprintf(buf,"vmaddaw.%s ACC,%s,%sw",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMSUBAx(char *buf){sprintf(buf,"vmsubax.%s ACC,%s,%sx",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMSUBAy(char *buf){sprintf(buf,"vmsubay.%s ACC,%s,%sy",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMSUBAz(char *buf){sprintf(buf,"vmsubaz.%s ACC,%s,%sz",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMSUBAw(char *buf){sprintf(buf,"vmsubaw.%s ACC,%s,%sw",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VITOF0(char *buf){sprintf(buf, "vitof0.%s %s, %s", dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]); }
void P_VITOF4(char *buf){sprintf(buf, "vitof4.%s %s, %s", dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]); }
void P_VITOF12(char *buf){sprintf(buf, "vitof12.%s %s, %s", dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]); }
void P_VITOF15(char *buf){sprintf(buf, "vitof15.%s %s, %s", dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]); }
void P_VFTOI0(char *buf) {sprintf(buf, "vftoi0.%s %s, %s", dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]); }
void P_VFTOI4(char *buf) {sprintf(buf, "vftoi4.%s %s, %s", dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]); }
void P_VFTOI12(char *buf){sprintf(buf, "vftoi12.%s %s, %s", dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]); }
void P_VFTOI15(char *buf){sprintf(buf, "vftoi15.%s %s, %s", dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]); }
void P_VMULAx(char *buf){sprintf(buf,"vmulax.%s ACC,%s,%sx",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMULAy(char *buf){sprintf(buf,"vmulay.%s ACC,%s,%sy",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMULAz(char *buf){sprintf(buf,"vmulaz.%s ACC,%s,%sz",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMULAw(char *buf){sprintf(buf,"vmulaw.%s ACC,%s,%sw",dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMULAq(char *buf){sprintf(buf,"vmulaq.%s ACC %s, Q" ,dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VABS(char *buf){sprintf(buf, "vabs.%s %s, %s", dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]);}
void P_VMULAi(char *buf){sprintf(buf,"vmulaq.%s ACC %s, I" ,dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VCLIPw(char *buf){sprintf(buf,"vclip %sxyz, %sw", COP2_REG_FP[DECODE_FS], COP2_REG_FP[DECODE_FT]);}
void P_VADDAq(char *buf){sprintf(buf,"vaddaq.%s ACC %s, Q" ,dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VMADDAq(char *buf){sprintf(buf,"vmaddaq.%s ACC %s, Q" ,dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VADDAi(char *buf){sprintf(buf,"vaddai.%s ACC %s, I" ,dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VMADDAi(char *buf){sprintf(buf,"vmaddai.%s ACC %s, Q" ,dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VSUBAq(char *buf){sprintf(buf,"vsubaq.%s ACC %s, Q" ,dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VMSUBAq(char *buf){sprintf(buf,"vmsubaq.%s ACC %s, Q" ,dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VSUBAi(char *buf){sprintf(buf,"vsubai.%s ACC %s, I" ,dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VMSUBAi(char *buf){sprintf(buf,"vmsubai.%s ACC %s, I" ,dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VADDA(char *buf){sprintf(buf,"vadda.%s ACC %s, %s" ,dest_string(), COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMADDA(char *buf){sprintf(buf,"vmadda.%s ACC %s, %s" ,dest_string(), COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMULA(char *buf){sprintf(buf,"vmula.%s ACC %s, %s" ,dest_string(), COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VSUBA(char *buf){sprintf(buf,"vsuba.%s ACC %s, %s" ,dest_string(), COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMSUBA(char *buf){sprintf(buf,"vmsuba.%s ACC %s, %s" ,dest_string(), COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VOPMULA(char *buf){sprintf(buf,"vopmula.xyz %sxyz, %sxyz" ,COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VNOP(char *buf){strcpy(buf,"vnop");}
void P_VMONE(char *buf){sprintf(buf,"vmove.%s, %s, %s" ,dest_string(), COP2_REG_FP[DECODE_FT],COP2_REG_FP[DECODE_FS]); }
void P_VMR32(char *buf){sprintf(buf,"vmr32.%s, %s, %s" ,dest_string(), COP2_REG_FP[DECODE_FT],COP2_REG_FP[DECODE_FS]); }
void P_VLQI(char *buf){sprintf(buf,"vlqi %s%s, (%s++)", COP2_REG_FP[DECODE_FT], dest_string(), COP2_REG_CTL[DECODE_FS]);}
void P_VSQI(char *buf){sprintf(buf,"vsqi %s%s, (%s++)", COP2_REG_FP[DECODE_FS], dest_string(), COP2_REG_CTL[DECODE_FT]);}
void P_VLQD(char *buf){sprintf(buf,"vlqd %s%s, (--%s)", COP2_REG_FP[DECODE_FT], dest_string(), COP2_REG_CTL[DECODE_FS]);}
void P_VSQD(char *buf){sprintf(buf,"vsqd %s%s, (--%s)", COP2_REG_FP[DECODE_FS], dest_string(), COP2_REG_CTL[DECODE_FT]);}
void P_VDIV(char *buf){sprintf(buf,"vdiv Q, %s%c, %s%c", COP2_REG_FP[DECODE_FS], dest_fsf(), COP2_REG_FP[DECODE_FT], dest_ftf());}
void P_VSQRT(char *buf){sprintf(buf,"vsqrt Q, %s%c", COP2_REG_FP[DECODE_FT], dest_ftf());}
void P_VRSQRT(char *buf){sprintf(buf,"vrsqrt Q, %s%c, %s%c", COP2_REG_FP[DECODE_FS], dest_fsf(), COP2_REG_FP[DECODE_FT], dest_ftf());}
void P_VWAITQ(char *buf){sprintf(buf,"vwaitq");}
void P_VMTIR(char *buf){sprintf(buf,"vmtir %s, %s%c", COP2_REG_CTL[DECODE_FT], COP2_REG_FP[DECODE_FS], dest_fsf());}
void P_VMFIR(char *buf){sprintf(buf,"vmfir %s%c, %s", COP2_REG_FP[DECODE_FT], dest_string(), COP2_REG_CTL[DECODE_FS]);}
void P_VILWR(char *buf){sprintf(buf,"vilwr %s, (%s)%s", COP2_REG_CTL[DECODE_FT], COP2_REG_CTL[DECODE_FS], dest_string());}
void P_VISWR(char *buf){sprintf(buf,"viswr %s, (%s)%s", COP2_REG_CTL[DECODE_FT], COP2_REG_CTL[DECODE_FS], dest_string());}
void P_VRNEXT(char *buf){sprintf(buf,"vrnext %s%s, R", COP2_REG_CTL[DECODE_FT], dest_string());}
void P_VRGET(char *buf){sprintf(buf,"vrget %s%s, R", COP2_REG_CTL[DECODE_FT], dest_string());}
void P_VRINIT(char *buf){sprintf(buf,"vrinit R, %s%s", COP2_REG_CTL[DECODE_FS], dest_string());}
void P_VRXOR(char *buf){sprintf(buf,"vrxor R, %s%s", COP2_REG_CTL[DECODE_FS], dest_string());}
//************************************END OF SPECIAL2 VUO TABLE****************************     
