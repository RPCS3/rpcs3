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

#include "PrecompiledHeader.h"

#include "Debug.h"
#include "R3000A.h"
#include "DisASM.h"

namespace R3000A {

unsigned long IOP_opcode_addr;

const char *GPR_IOP_REG[32] = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};
const char *COP0_IOP_REG[32] ={
	"Index","Random","EntryLo0","EntryLo1","Context","PageMask",
	"Wired","C0r7","BadVaddr","Count","EntryHi","Compare","Status",
	"Cause","EPC","PRId","Config","C0r17","C0r18","C0r19","C0r20",
	"C0r21","C0r22","C0r23","Debug","Perf","C0r26","C0r27","TagLo",
	"TagHi","ErrorPC","C0r31"
};
void IOPD_SPECIAL(char *buf);
void IOPD_REGIMM(char *buf); 
void IOPD_J(char *buf);   
void IOPD_JAL(char *buf);  
void IOPD_BEQ(char *buf); 
void IOPD_BNE(char *buf); 
void IOPD_BLEZ(char *buf);
void IOPD_BGTZ(char *buf);
void IOPD_ADDI(char *buf);   
void IOPD_ADDIU(char *buf); 
void IOPD_SLTI(char *buf);
void IOPD_SLTIU(char *buf);
void IOPD_ANDI(char *buf);
void IOPD_ORI(char *buf); 
void IOPD_XORI(char *buf);
void IOPD_LUI(char *buf); 
void IOPD_COP0(char *buf);  
void IOPD_COP2(char *buf);		
void IOPD_LB(char *buf);    
void IOPD_LH(char *buf);    
void IOPD_LWL(char *buf); 
void IOPD_LW(char *buf);   
void IOPD_LBU(char *buf); 
void IOPD_LHU(char *buf); 
void IOPD_LWR(char *buf); 
void IOPD_SB(char *buf);    
void IOPD_SH(char *buf);   
void IOPD_SWL(char *buf); 
void IOPD_SW(char *buf);  
void IOPD_SWR(char *buf); 
void IOPD_LWC2(char *buf);
void IOPD_SWC2(char *buf);

void IOPD_SLL(char *buf);
void IOPD_SRL(char *buf); 
void IOPD_SRA(char *buf);
void IOPD_SLLV(char *buf); 
void IOPD_SRLV(char *buf);
void IOPD_SRAV(char *buf);
void IOPD_JR(char *buf);  
void IOPD_JALR(char *buf); 
void IOPD_SYSCALL(char *buf);
void IOPD_BREAK(char *buf);
void IOPD_MFHI(char *buf);
void IOPD_MTHI(char *buf); 
void IOPD_MFLO(char *buf);
void IOPD_MTLO(char *buf);
void IOPD_MULT(char *buf);
void IOPD_MULTU(char *buf);
void IOPD_DIV(char *buf); 
void IOPD_DIVU(char *buf);
void IOPD_ADD(char *buf); 
void IOPD_ADDU(char *buf); 
void IOPD_SUB(char *buf); 
void IOPD_SUBU(char *buf);
void IOPD_AND(char *buf);    
void IOPD_OR(char *buf);   
void IOPD_XOR(char *buf); 
void IOPD_NOR(char *buf); 
void IOPD_SLT(char *buf); 
void IOPD_SLTU(char *buf);


void IOPD_BLTZ(char *buf); 
void IOPD_BGEZ(char *buf); 
void IOPD_BLTZAL(char *buf);
void IOPD_BGEZAL(char *buf);
 


void IOPD_MFC0(char *buf);
void IOPD_CFC0(char *buf);
void IOPD_MTC0(char *buf);
void IOPD_CTC0(char *buf);
void IOPD_RFE(char *buf); 



void IOPD_BASIC(char *buf);
void IOPD_RTPS(char *buf);
void IOPD_NCLIP(char *buf);
void IOPD_OP(char *buf); 
void IOPD_DPCS(char *buf); 
void IOPD_INTPL(char *buf);
void IOPD_MVMVA(char *buf);
void IOPD_NCDS(char *buf);
void IOPD_CDP(char *buf); 
void IOPD_NCDT(char *buf);  
void IOPD_NCCS(char *buf);
void IOPD_CC(char *buf); 
void IOPD_NCS(char *buf); 
void IOPD_NCT(char *buf); 
void IOPD_SQR(char *buf); 
void IOPD_DCPL(char *buf); 
void IOPD_DPCT(char *buf); 
void IOPD_AVSZ3(char *buf);
void IOPD_AVSZ4(char *buf); 
void IOPD_RTPT(char *buf); 
void IOPD_GPF(char *buf); 
void IOPD_GPL(char *buf); 
void IOPD_NCCT(char *buf);  



void IOPD_MFC2(char *buf);
void IOPD_CFC2(char *buf);
void IOPD_MTC2(char *buf);
void IOPD_CTC2(char *buf);
void IOPD_NULL(char *buf);



 void (*IOP_DEBUG_BSC[64])(char *buf) = {
	IOPD_SPECIAL, IOPD_REGIMM, IOPD_J   , IOPD_JAL  , IOPD_BEQ , IOPD_BNE , IOPD_BLEZ, IOPD_BGTZ,
	IOPD_ADDI   , IOPD_ADDIU , IOPD_SLTI, IOPD_SLTIU, IOPD_ANDI, IOPD_ORI , IOPD_XORI, IOPD_LUI ,
	IOPD_COP0   , IOPD_NULL  , IOPD_COP2, IOPD_NULL , IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL,
	IOPD_NULL   , IOPD_NULL  , IOPD_NULL, IOPD_NULL , IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL,
	IOPD_LB     , IOPD_LH    , IOPD_LWL , IOPD_LW   , IOPD_LBU , IOPD_LHU , IOPD_LWR , IOPD_NULL,
	IOPD_SB     , IOPD_SH    , IOPD_SWL , IOPD_SW   , IOPD_NULL, IOPD_NULL, IOPD_SWR , IOPD_NULL, 
	IOPD_NULL   , IOPD_NULL  , IOPD_LWC2, IOPD_NULL , IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL,
	IOPD_NULL   , IOPD_NULL  , IOPD_SWC2, IOPD_NULL , IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL 
};


void (*IOP_DEBUG_SPC[64])(char *buf) = {
	IOPD_SLL , IOPD_NULL , IOPD_SRL , IOPD_SRA , IOPD_SLLV   , IOPD_NULL , IOPD_SRLV, IOPD_SRAV,
	IOPD_JR  , IOPD_JALR , IOPD_NULL, IOPD_NULL, IOPD_SYSCALL, IOPD_BREAK, IOPD_NULL, IOPD_NULL,
	IOPD_MFHI, IOPD_MTHI , IOPD_MFLO, IOPD_MTLO, IOPD_NULL   , IOPD_NULL , IOPD_NULL, IOPD_NULL,
	IOPD_MULT, IOPD_MULTU, IOPD_DIV , IOPD_DIVU, IOPD_NULL   , IOPD_NULL , IOPD_NULL, IOPD_NULL,
	IOPD_ADD , IOPD_ADDU , IOPD_SUB , IOPD_SUBU, IOPD_AND    , IOPD_OR   , IOPD_XOR , IOPD_NOR ,
	IOPD_NULL, IOPD_NULL , IOPD_SLT , IOPD_SLTU, IOPD_NULL   , IOPD_NULL , IOPD_NULL, IOPD_NULL,
	IOPD_NULL, IOPD_NULL , IOPD_NULL, IOPD_NULL, IOPD_NULL   , IOPD_NULL , IOPD_NULL, IOPD_NULL,
	IOPD_NULL, IOPD_NULL , IOPD_NULL, IOPD_NULL, IOPD_NULL   , IOPD_NULL , IOPD_NULL, IOPD_NULL
};

void (*IOP_DEBUG_REG[32])(char *buf) = {
	IOPD_BLTZ  , IOPD_BGEZ  , IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL,
	IOPD_NULL  , IOPD_NULL  , IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL,
	IOPD_BLTZAL, IOPD_BGEZAL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL,
	IOPD_NULL  , IOPD_NULL  , IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL
};

void (*IOP_DEBUG_CP0[32])(char *buf) = {
	IOPD_MFC0, IOPD_NULL, IOPD_CFC0, IOPD_NULL, IOPD_MTC0, IOPD_NULL, IOPD_CTC0, IOPD_NULL,
	IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL,
	IOPD_RFE , IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL,
	IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL
};

void (*IOP_DEBUG_CP2[64])(char *buf) = {
	IOPD_BASIC, IOPD_RTPS , IOPD_NULL , IOPD_NULL, IOPD_NULL, IOPD_NULL , IOPD_NCLIP, IOPD_NULL, 
	IOPD_NULL , IOPD_NULL , IOPD_NULL , IOPD_NULL, IOPD_OP  , IOPD_NULL , IOPD_NULL , IOPD_NULL, 
	IOPD_DPCS , IOPD_INTPL, IOPD_MVMVA, IOPD_NCDS, IOPD_CDP , IOPD_NULL , IOPD_NCDT , IOPD_NULL, 
	IOPD_NULL , IOPD_NULL , IOPD_NULL , IOPD_NCCS, IOPD_CC  , IOPD_NULL , IOPD_NCS  , IOPD_NULL, 
	IOPD_NCT  , IOPD_NULL , IOPD_NULL , IOPD_NULL, IOPD_NULL, IOPD_NULL , IOPD_NULL , IOPD_NULL, 
	IOPD_SQR  , IOPD_DCPL , IOPD_DPCT , IOPD_NULL, IOPD_NULL, IOPD_AVSZ3, IOPD_AVSZ4, IOPD_NULL, 
	IOPD_RTPT , IOPD_NULL , IOPD_NULL , IOPD_NULL, IOPD_NULL, IOPD_NULL , IOPD_NULL , IOPD_NULL, 
	IOPD_NULL , IOPD_NULL , IOPD_NULL , IOPD_NULL, IOPD_NULL, IOPD_GPF  , IOPD_GPL  , IOPD_NCCT  
};

void (*IOP_DEBUG_CP2BSC[32])(char *buf) = {
	IOPD_MFC2, IOPD_NULL, IOPD_CFC2, IOPD_NULL, IOPD_MTC2, IOPD_NULL, IOPD_CTC2, IOPD_NULL,
	IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL,
	IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL,
	IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL, IOPD_NULL
};

static char dbuf2[1024];
static char obuf2[1024];

char *disR3000Fasm(u32 code, u32 pc) {
	u32 scode = psxRegs.code;
	IOP_opcode_addr = pc;
	psxRegs.code = code;
	IOP_DEBUG_BSC[(code) >> 26](dbuf2);

	sprintf(obuf2, "%08lX:\t%s", pc, dbuf2);

	psxRegs.code = scode;
	return obuf2;
}
char *IOP_jump_decode(void)
{
    static char buf[256];
    unsigned long addr;
    addr = (IOP_opcode_addr & 0xf0000000)|((psxRegs.code&0x3ffffff)<<2);
    sprintf(buf, "0x%08lX", addr);
    return buf;
}
char *IOP_offset_decode(void)
{
    static char buf[256];
    unsigned long addr;
    addr = ((((short)( psxRegs.code & 0xFFFF) * 4) + IOP_opcode_addr + 4));
    sprintf(buf, "0x%08lX", addr);
    return buf;
}
//basic table
void IOPD_SPECIAL(char *buf){IOP_DEBUG_SPC[((psxRegs.code) & 0x3F)](buf);}
void IOPD_REGIMM(char *buf){IOP_DEBUG_REG[DECODE_RT_IOP](buf);} 
void IOPD_J(char *buf)  { sprintf(buf, "j\t%s",                   IOP_jump_decode());}   
void IOPD_JAL(char *buf){sprintf(buf, "jal\t%s",                  IOP_jump_decode());}  
void IOPD_BEQ(char *buf){sprintf(buf, "beq\t%s, %s, %s",          GPR_IOP_REG[DECODE_RS_IOP], GPR_IOP_REG[DECODE_RT_IOP], IOP_offset_decode()); } 
void IOPD_BNE(char *buf){sprintf(buf, "bne\t%s, %s, %s",          GPR_IOP_REG[DECODE_RS_IOP], GPR_IOP_REG[DECODE_RT_IOP], IOP_offset_decode()); } 
void IOPD_BLEZ(char *buf){sprintf(buf, "blez\t%s, %s",            GPR_IOP_REG[DECODE_RS_IOP], IOP_offset_decode()); }
void IOPD_BGTZ(char *buf){sprintf(buf, "bgtz\t%s, %s",            GPR_IOP_REG[DECODE_RS_IOP], IOP_offset_decode()); }
void IOPD_ADDI(char *buf){sprintf(buf, "addi\t%s, %s, 0x%04lX",   GPR_IOP_REG[DECODE_RT_IOP], GPR_IOP_REG[DECODE_RS_IOP], DECODE_IMMED_IOP);}   
void IOPD_ADDIU(char *buf){sprintf(buf, "addiu\t%s, %s, 0x%04lX", GPR_IOP_REG[DECODE_RT_IOP], GPR_IOP_REG[DECODE_RS_IOP], DECODE_IMMED_IOP);} 
void IOPD_SLTI(char *buf){sprintf(buf, "slti\t%s, %s, 0x%04lX",   GPR_IOP_REG[DECODE_RT_IOP], GPR_IOP_REG[DECODE_RS_IOP], DECODE_IMMED_IOP);}
void IOPD_SLTIU(char *buf){sprintf(buf, "sltiu\t%s, %s, 0x%04lX", GPR_IOP_REG[DECODE_RT_IOP], GPR_IOP_REG[DECODE_RS_IOP], DECODE_IMMED_IOP);}
void IOPD_ANDI(char *buf){sprintf(buf, "andi\t%s, %s, 0x%04lX",   GPR_IOP_REG[DECODE_RT_IOP], GPR_IOP_REG[DECODE_RS_IOP], DECODE_IMMED_IOP);}
void IOPD_ORI(char *buf){sprintf(buf, "ori\t%s, %s, 0x%04lX",     GPR_IOP_REG[DECODE_RT_IOP], GPR_IOP_REG[DECODE_RS_IOP], DECODE_IMMED_IOP); }
void IOPD_XORI(char *buf){sprintf(buf, "xori\t%s, %s, 0x%04lX",   GPR_IOP_REG[DECODE_RT_IOP], GPR_IOP_REG[DECODE_RS_IOP], DECODE_IMMED_IOP); }
void IOPD_LUI(char *buf){sprintf(buf, "lui\t%s, 0x%04lX",         GPR_IOP_REG[DECODE_RT_IOP], DECODE_IMMED_IOP); } 
void IOPD_COP0(char *buf){IOP_DEBUG_CP0[DECODE_RS_IOP](buf);}  
void IOPD_COP2(char *buf){IOP_DEBUG_CP2[((psxRegs.code) & 0x3F)](buf);}		
void IOPD_LB(char *buf){sprintf(buf, "lb\t%s, 0x%04lX(%s)",     GPR_IOP_REG[DECODE_RT_IOP], DECODE_IMMED_IOP, GPR_IOP_REG[DECODE_RS_IOP]); }    
void IOPD_LH(char *buf){sprintf(buf, "lh\t%s, 0x%04lX(%s)",     GPR_IOP_REG[DECODE_RT_IOP], DECODE_IMMED_IOP, GPR_IOP_REG[DECODE_RS_IOP]); }    
void IOPD_LWL(char *buf){sprintf(buf, "lwl\t%s, 0x%04lX(%s)",   GPR_IOP_REG[DECODE_RT_IOP], DECODE_IMMED_IOP, GPR_IOP_REG[DECODE_RS_IOP]); } 
void IOPD_LW(char *buf){sprintf(buf, "lw\t%s, 0x%04lX(%s)",     GPR_IOP_REG[DECODE_RT_IOP], DECODE_IMMED_IOP, GPR_IOP_REG[DECODE_RS_IOP]); }   
void IOPD_LBU(char *buf){sprintf(buf, "lbu\t%s, 0x%04lX(%s)",   GPR_IOP_REG[DECODE_RT_IOP], DECODE_IMMED_IOP, GPR_IOP_REG[DECODE_RS_IOP]); } 
void IOPD_LHU(char *buf){sprintf(buf, "lhu\t%s, 0x%04lX(%s)",   GPR_IOP_REG[DECODE_RT_IOP], DECODE_IMMED_IOP, GPR_IOP_REG[DECODE_RS_IOP]); } 
void IOPD_LWR(char *buf){sprintf(buf, "lwr\t%s, 0x%04lX(%s)",   GPR_IOP_REG[DECODE_RT_IOP], DECODE_IMMED_IOP, GPR_IOP_REG[DECODE_RS_IOP]);} 
void IOPD_SB(char *buf){sprintf(buf, "sb\t%s, 0x%04lX(%s)",     GPR_IOP_REG[DECODE_RT_IOP], DECODE_IMMED_IOP, GPR_IOP_REG[DECODE_RS_IOP]);}    
void IOPD_SH(char *buf){sprintf(buf, "sh\t%s, 0x%04lX(%s)",     GPR_IOP_REG[DECODE_RT_IOP], DECODE_IMMED_IOP, GPR_IOP_REG[DECODE_RS_IOP]); }   
void IOPD_SWL(char *buf){sprintf(buf, "swl\t%s, 0x%04lX(%s)",   GPR_IOP_REG[DECODE_RT_IOP], DECODE_IMMED_IOP, GPR_IOP_REG[DECODE_RS_IOP]); } 
void IOPD_SW(char *buf){sprintf(buf, "sw\t%s, 0x%04lX(%s)",     GPR_IOP_REG[DECODE_RT_IOP], DECODE_IMMED_IOP, GPR_IOP_REG[DECODE_RS_IOP]); }  
void IOPD_SWR(char *buf){sprintf(buf, "swr\t%s, 0x%04lX(%s)",   GPR_IOP_REG[DECODE_RT_IOP], DECODE_IMMED_IOP, GPR_IOP_REG[DECODE_RS_IOP]);} 
void IOPD_LWC2(char *buf){strcpy(buf, "lwc2");}
void IOPD_SWC2(char *buf){strcpy(buf, "swc2");}
//special table
void IOPD_SLL(char *buf)
{
   if (psxRegs.code == 0x00000000)
        strcpy(buf, "nop");
    else
        sprintf(buf, "sll\t%s, %s, 0x%02lX", GPR_IOP_REG[DECODE_RD_IOP], GPR_IOP_REG[DECODE_RT_IOP], DECODE_SA_IOP);
}
void IOPD_SRL(char *buf){sprintf(buf, "srl\t%s, %s, 0x%02lX", GPR_IOP_REG[DECODE_RD_IOP], GPR_IOP_REG[DECODE_RT_IOP], DECODE_SA_IOP); } 
void IOPD_SRA(char *buf){sprintf(buf, "sra\t%s, %s, 0x%02lX", GPR_IOP_REG[DECODE_RD_IOP], GPR_IOP_REG[DECODE_RT_IOP], DECODE_SA_IOP);}
void IOPD_SLLV(char *buf){sprintf(buf, "sllv\t%s, %s, %s",    GPR_IOP_REG[DECODE_RD_IOP], GPR_IOP_REG[DECODE_RT_IOP], GPR_IOP_REG[DECODE_RS_IOP]); } 
void IOPD_SRLV(char *buf){sprintf(buf, "srlv\t%s, %s, %s",    GPR_IOP_REG[DECODE_RD_IOP], GPR_IOP_REG[DECODE_RT_IOP], GPR_IOP_REG[DECODE_RS_IOP]);}
void IOPD_SRAV(char *buf){sprintf(buf, "srav\t%s, %s, %s",    GPR_IOP_REG[DECODE_RD_IOP], GPR_IOP_REG[DECODE_RT_IOP], GPR_IOP_REG[DECODE_RS_IOP]); }
void IOPD_JR(char *buf){sprintf(buf, "jr\t%s",                GPR_IOP_REG[DECODE_RS_IOP]);}  
void IOPD_JALR(char *buf)
{
    int rd = DECODE_RD_IOP;

    if (rd == 31)
        sprintf(buf, "jalr\t%s", GPR_IOP_REG[DECODE_RS_IOP]);
    else
        sprintf(buf, "jalr\t%s, %s", GPR_IOP_REG[rd], GPR_IOP_REG[DECODE_RS_IOP]);
}

void IOPD_SYSCALL(char *buf){strcpy(buf, "syscall");}
void IOPD_BREAK(char *buf){strcpy(buf, "break");}
void IOPD_MFHI(char *buf){sprintf(buf, "mfhi\t%s",          GPR_IOP_REG[DECODE_RD_IOP]); }
void IOPD_MTHI(char *buf){sprintf(buf, "mthi\t%s",          GPR_IOP_REG[DECODE_RS_IOP]); } 
void IOPD_MFLO(char *buf){sprintf(buf, "mflo\t%s",          GPR_IOP_REG[DECODE_RD_IOP]); }
void IOPD_MTLO(char *buf){sprintf(buf, "mtlo\t%s",          GPR_IOP_REG[DECODE_RS_IOP]); }
void IOPD_MULT(char *buf){sprintf(buf, "mult\t%s, %s",      GPR_IOP_REG[DECODE_RS_IOP], GPR_IOP_REG[DECODE_RT_IOP]);}
void IOPD_MULTU(char *buf){sprintf(buf, "multu\t%s, %s",    GPR_IOP_REG[DECODE_RS_IOP], GPR_IOP_REG[DECODE_RT_IOP]);}
void IOPD_DIV(char *buf){sprintf(buf, "div\t%s, %s",        GPR_IOP_REG[DECODE_RS_IOP], GPR_IOP_REG[DECODE_RT_IOP]);} 
void IOPD_DIVU(char *buf){sprintf(buf, "divu\t%s, %s",      GPR_IOP_REG[DECODE_RS_IOP], GPR_IOP_REG[DECODE_RT_IOP]); }

void IOPD_ADD(char *buf)     { sprintf(buf, "add\t%s, %s, %s",   GPR_IOP_REG[DECODE_RD_IOP], GPR_IOP_REG[DECODE_RS_IOP], GPR_IOP_REG[DECODE_RT_IOP]);  }
void IOPD_ADDU(char *buf)    { sprintf(buf, "addu\t%s, %s, %s",  GPR_IOP_REG[DECODE_RD_IOP], GPR_IOP_REG[DECODE_RS_IOP], GPR_IOP_REG[DECODE_RT_IOP]);  }
void IOPD_SUB(char *buf)     { sprintf(buf, "sub\t%s, %s, %s",   GPR_IOP_REG[DECODE_RD_IOP], GPR_IOP_REG[DECODE_RS_IOP], GPR_IOP_REG[DECODE_RT_IOP]);  }
void IOPD_SUBU(char *buf)    { sprintf(buf, "subu\t%s, %s, %s",  GPR_IOP_REG[DECODE_RD_IOP], GPR_IOP_REG[DECODE_RS_IOP], GPR_IOP_REG[DECODE_RT_IOP]);  }
void IOPD_AND(char *buf)     { sprintf(buf, "and\t%s, %s, %s",   GPR_IOP_REG[DECODE_RD_IOP], GPR_IOP_REG[DECODE_RS_IOP], GPR_IOP_REG[DECODE_RT_IOP]);  }
void IOPD_OR(char *buf)      { sprintf(buf, "or\t%s, %s, %s",    GPR_IOP_REG[DECODE_RD_IOP], GPR_IOP_REG[DECODE_RS_IOP], GPR_IOP_REG[DECODE_RT_IOP]);  }
void IOPD_XOR(char *buf)     { sprintf(buf, "xor\t%s, %s, %s",   GPR_IOP_REG[DECODE_RD_IOP], GPR_IOP_REG[DECODE_RS_IOP], GPR_IOP_REG[DECODE_RT_IOP]);  }
void IOPD_NOR(char *buf)     { sprintf(buf, "nor\t%s, %s, %s",   GPR_IOP_REG[DECODE_RD_IOP], GPR_IOP_REG[DECODE_RS_IOP], GPR_IOP_REG[DECODE_RT_IOP]);  }
void IOPD_SLT(char *buf)     { sprintf(buf, "slt\t%s, %s, %s",   GPR_IOP_REG[DECODE_RD_IOP], GPR_IOP_REG[DECODE_RS_IOP], GPR_IOP_REG[DECODE_RT_IOP]);  }
void IOPD_SLTU(char *buf)    { sprintf(buf, "sltu\t%s, %s, %s",  GPR_IOP_REG[DECODE_RD_IOP], GPR_IOP_REG[DECODE_RS_IOP], GPR_IOP_REG[DECODE_RT_IOP]);  }
//regimm

void IOPD_BLTZ(char *buf)    { sprintf(buf, "bltz\t%s, %s",     GPR_IOP_REG[DECODE_RS_IOP], IOP_offset_decode()); }
void IOPD_BGEZ(char *buf)    { sprintf(buf, "bgez\t%s, %s",     GPR_IOP_REG[DECODE_RS_IOP], IOP_offset_decode()); }
void IOPD_BLTZAL(char *buf)  { sprintf(buf, "bltzal\t%s, %s",   GPR_IOP_REG[DECODE_RS_IOP], IOP_offset_decode()); }
void IOPD_BGEZAL(char *buf)  { sprintf(buf, "bgezal\t%s, %s",   GPR_IOP_REG[DECODE_RS_IOP], IOP_offset_decode()); }

//cop0

void IOPD_MFC0(char *buf){  sprintf(buf, "mfc0\t%s, %s",  GPR_IOP_REG[DECODE_RT_IOP], COP0_IOP_REG[DECODE_FS_IOP]); }
void IOPD_MTC0(char *buf){  sprintf(buf, "mtc0\t%s, %s",  GPR_IOP_REG[DECODE_RT_IOP], COP0_IOP_REG[DECODE_FS_IOP]); }
void IOPD_CFC0(char *buf){  sprintf(buf, "cfc0\t%s, %s",  GPR_IOP_REG[DECODE_RT_IOP], COP0_IOP_REG[DECODE_FS_IOP]); }
void IOPD_CTC0(char *buf){  sprintf(buf, "ctc0\t%s, %s",  GPR_IOP_REG[DECODE_RT_IOP], COP0_IOP_REG[DECODE_FS_IOP]); }
void IOPD_RFE(char *buf){strcpy(buf, "rfe");} 
//cop2
void IOPD_BASIC(char *buf){IOP_DEBUG_CP2BSC[DECODE_RS_IOP](buf);}
void IOPD_RTPS(char *buf){strcpy(buf, "rtps");}
void IOPD_NCLIP(char *buf){strcpy(buf, "nclip");}
void IOPD_OP(char *buf){strcpy(buf, "op");}
void IOPD_DPCS(char *buf){strcpy(buf, "dpcs");} 
void IOPD_INTPL(char *buf){strcpy(buf, "intpl");}
void IOPD_MVMVA(char *buf){strcpy(buf, "mvmva");}
void IOPD_NCDS(char *buf){strcpy(buf, "ncds");}
void IOPD_CDP(char *buf){strcpy(buf, "cdp");} 
void IOPD_NCDT(char *buf){strcpy(buf, "ncdt");}  
void IOPD_NCCS(char *buf){strcpy(buf, "nccs");}
void IOPD_CC(char *buf){strcpy(buf, "cc");} 
void IOPD_NCS(char *buf){strcpy(buf, "ncs");} 
void IOPD_NCT(char *buf){strcpy(buf, "nct");} 
void IOPD_SQR(char *buf){strcpy(buf, "sqr");} 
void IOPD_DCPL(char *buf){strcpy(buf, "dcpl");} 
void IOPD_DPCT(char *buf){strcpy(buf, "dpct");}
void IOPD_AVSZ3(char *buf){strcpy(buf, "avsz3");}
void IOPD_AVSZ4(char *buf){strcpy(buf, "avsz4");} 
void IOPD_RTPT(char *buf){strcpy(buf, "rtpt");}
void IOPD_GPF(char *buf){strcpy(buf, "gpf");} 
void IOPD_GPL(char *buf){strcpy(buf, "gpl");}
void IOPD_NCCT(char *buf){strcpy(buf, "ncct");}  
//cop2 basic
void IOPD_MFC2(char *buf){strcpy(buf, "mfc2");}
void IOPD_CFC2(char *buf){strcpy(buf, "cfc2");}
void IOPD_MTC2(char *buf){strcpy(buf, "mtc2");}
void IOPD_CTC2(char *buf){strcpy(buf, "ctc2");}
//null
void IOPD_NULL(char *buf){strcpy(buf, "????");}

}		// end Namespace R3000A