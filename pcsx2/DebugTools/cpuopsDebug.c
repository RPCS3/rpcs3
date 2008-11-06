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

#include <stdlib.h>
#include "Common.h"
#include "Debug.h"
#include "cpuopsDebug.h"

void UpdateR5900op()
{
  FILE *fp;
  fp=fopen("cpuops.txt", "wt");
  fprintf(fp, "------------\n");
  fprintf(fp, "--R5900ops--\n");
  fprintf(fp, "------------\n");
  if(L_ADD>0)       fprintf(fp, "Add     %d\n",                  L_ADD); 
  if(L_ADDI>0)      fprintf(fp, "Addi    %d\n",                  L_ADDI);
  if(L_ADDIU>0)     fprintf(fp, "Addiu   %d\n",                  L_ADDIU);
  if(L_ADDU>0)      fprintf(fp, "Addu    %d\n",                  L_ADDU);
  if(L_AND>0)       fprintf(fp, "And     %d\n",                  L_AND);
  if(L_ANDI>0)      fprintf(fp, "Andi    %d\n",                  L_ANDI);
  if(L_BEQ>0)       fprintf(fp, "Beq     %d\n",                  L_BEQ);
  if(L_BEQL>0)      fprintf(fp, "Beql    %d\n",                  L_BEQL);
  if(L_BGEZ>0)      fprintf(fp, "Bgez    %d\n",                  L_BGEZ);
  if(L_BGEZAL>0)    fprintf(fp, "Bgezal  %d\n",                  L_BGEZAL);
  if(L_BGEZALL>0)   fprintf(fp, "Bgezall %d\n",                  L_BGEZALL);
  if(L_BGEZL>0)     fprintf(fp, "Bgezl   %d\n",                  L_BGEZL);
  if( L_BGTZ>0)     fprintf(fp, "Bgtz    %d\n",                  L_BGTZ);
  if( L_BGTZL>0)    fprintf(fp, "Bgtzl   %d\n",                  L_BGTZL);
  if(L_BLEZ>0)      fprintf(fp, "Blez    %d\n",                  L_BLEZ);
  if(L_BLEZL>0)     fprintf(fp, "blezl   %d\n",                  L_BLEZL);
  if(L_BLTZ>0)      fprintf(fp, "bltz    %d\n",                  L_BLTZ);
  if(L_BLTZAL>0)    fprintf(fp, "bltzal  %d\n",                  L_BLTZAL);
  if(L_BLTZALL>0)   fprintf(fp, "bltzall %d\n",                  L_BLTZALL);
  if(L_BLTZL>0)     fprintf(fp, "bltzl   %d\n",                  L_BLTZL);
  if(L_BNE>0)       fprintf(fp, "bne     %d\n",                  L_BNE);
  if(L_BNEL>0)      fprintf(fp, "bnel    %d\n",                  L_BNEL);
  if(L_BREAK>0)     fprintf(fp, "break   %d\n",                  L_BREAK);
  if(L_CACHE>0)     fprintf(fp, "cache   %d\n",                  L_CACHE);
  if(L_DADD>0)      fprintf(fp, "dadd    %d\n",                  L_DADD);
  if(L_DADDI>0)     fprintf(fp, "daddi   %d\n",                  L_DADDI);
  if(L_DADDIU>0)    fprintf(fp, "daddiu  %d\n",                  L_DADDIU);
  if(L_DADDU>0)     fprintf(fp, "daddu   %d\n",                  L_DADDU);
  if(L_DIV>0)       fprintf(fp, "div     %d\n",                  L_DIV);
  if(L_DIVU>0)      fprintf(fp, "divu    %d\n",                  L_DIVU);
  if(L_DSLL>0)      fprintf(fp, "dsll    %d\n",                  L_DSLL);
  if(L_DSLL32>0)    fprintf(fp, "dsll32  %d\n",                  L_DSLL32);
  if(L_DSLLV>0)     fprintf(fp, "dsllv   %d\n",                  L_DSLLV);
  if(L_DSRA>0)      fprintf(fp, "dsra    %d\n",                  L_DSRA);
  if(L_DSRA32>0)    fprintf(fp, "dsra32  %d\n",                  L_DSRA32);
  if(L_DSRAV>0)     fprintf(fp, "dsrav   %d\n",                  L_DSRAV);
  if(L_DSRL>0)      fprintf(fp, "dsrl    %d\n",                  L_DSRL);
  if(L_DSRL32>0)    fprintf(fp, "dsr32   %d\n",                  L_DSRL32);
  if(L_DSRLV>0)     fprintf(fp, "dsrlv   %d\n",                  L_DSRLV);
  if(L_DSUB>0)      fprintf(fp, "dsub    %d\n",                  L_DSUB);
  if(L_DSUBU>0)     fprintf(fp, "dsubu   %d\n",                  L_DSUBU);
  if(L_J>0)         fprintf(fp, "j       %d\n",                  L_J);
  if(L_JAL>0)       fprintf(fp, "jal     %d\n",                  L_JAL);
  if(L_JALR>0)      fprintf(fp, "jalr    %d\n",                  L_JALR);
  if(L_JR>0)        fprintf(fp, "jr      %d\n",                  L_JR);
  if(L_LB>0)        fprintf(fp, "lb      %d\n",                  L_LB);
  if(L_LBU>0)       fprintf(fp, "lbu     %d\n",                  L_LBU);
  if(L_LD>0)        fprintf(fp, "ld      %d\n",                  L_LD);
  if(L_LDL>0)       fprintf(fp, "ldl     %d\n",                  L_LDL);
  if(L_LDR>0)       fprintf(fp, "ldr     %d\n",                  L_LDR);
  if(L_LH>0)        fprintf(fp, "lh      %d\n",                  L_LH);
  if(L_LHU>0)       fprintf(fp, "lhu     %d\n",                  L_LHU);
  if(L_LQ>0)        fprintf(fp, "lq      %d\n",                  L_LQ);
  if(L_LUI>0)       fprintf(fp, "lui     %d\n",                  L_LUI);
  if(L_LW>0)        fprintf(fp, "lw      %d\n",                  L_LW);
  if(L_LWL>0)       fprintf(fp, "lwl     %d\n",                  L_LWL);
  if(L_LWR>0)       fprintf(fp, "lwr     %d\n",                  L_LWR);
  if(L_LWU>0)       fprintf(fp, "lwu     %d\n",                  L_LWU);
  if(L_MFHI>0)      fprintf(fp, "mfhi    %d\n",                  L_MFHI);
  if(L_MFLO>0)      fprintf(fp, "mflo    %d\n",                  L_MFLO);
  if(L_MFSA>0)      fprintf(fp, "mfsa    %d\n",                  L_MFSA);
  if(L_MOVN>0)      fprintf(fp, "movn    %d\n",                  L_MOVN);
  if(L_MOVZ>0)      fprintf(fp, "movz    %d\n",                  L_MOVZ);
  if(L_MTHI>0)      fprintf(fp, "mthi    %d\n",                  L_MTHI);
  if(L_MTLO>0)      fprintf(fp, "mtlo    %d\n",                  L_MTLO);
  if(L_MTSA>0)      fprintf(fp, "mtsa    %d\n",                  L_MTSA);
  if(L_MTSAB>0)     fprintf(fp, "mtsab   %d\n",                  L_MTSAB);
  if(L_MTSAH>0)     fprintf(fp, "mtsah   %d\n",                  L_MTSAH);
  if(L_MULT>0)      fprintf(fp, "mult    %d\n",                  L_MULT);
  if(L_MULTU>0)     fprintf(fp, "multu   %d\n",                  L_MULTU);
  if(L_NOR>0)       fprintf(fp, "nor     %d\n",                  L_NOR);
  if(L_OR>0)        fprintf(fp, "or      %d\n",                  L_OR);
  if(L_ORI>0)       fprintf(fp, "ori     %d\n",                  L_ORI);
  if(L_PREF>0)      fprintf(fp, "pref    %d\n",                  L_PREF);
  if(L_SB>0)        fprintf(fp, "sb      %d\n",                  L_SB);
  if(L_SD>0)        fprintf(fp, "sd      %d\n",                  L_SD);
  if(L_SDL>0)       fprintf(fp, "sdl     %d\n",                  L_SDL);
  if(L_SDR>0)       fprintf(fp, "sdr     %d\n",                  L_SDR);
  if(L_SH>0)        fprintf(fp, "sh      %d\n",                  L_SH);
  if(L_SLL>0)       fprintf(fp, "sll     %d\n",                  L_SLL);
  if(L_SLLV>0)      fprintf(fp, "sllv    %d\n",                  L_SLLV);
  if(L_SLT>0)       fprintf(fp, "slt     %d\n",                  L_SLT);
  if(L_SLTI>0)      fprintf(fp, "slti    %d\n",                  L_SLTI);
  if(L_SLTIU>0)     fprintf(fp, "sltiu   %d\n",                  L_SLTIU);
  if(L_SLTU>0)      fprintf(fp, "sltu    %d\n",                  L_SLTU);
  if(L_SQ>0)        fprintf(fp, "sq      %d\n",                  L_SQ);
  if(L_SRA>0)       fprintf(fp, "sra     %d\n",                  L_SRA);
  if(L_SRAV>0)      fprintf(fp, "srav    %d\n",                  L_SRAV);
  if(L_SRL>0)       fprintf(fp, "srl     %d\n",                  L_SRL);
  if(L_SRLV>0)      fprintf(fp, "srlv    %d\n",                  L_SRLV);
  if(L_SUB>0)       fprintf(fp, "sub     %d\n",                  L_SUB);
  if(L_SUBU>0)      fprintf(fp, "subu    %d\n",                  L_SUBU);
  if(L_SW>0)        fprintf(fp, "sw      %d\n",                  L_SW);
  if(L_SWL>0)       fprintf(fp, "swl     %d\n",                  L_SWL);
  if(L_SWR>0)       fprintf(fp, "swr     %d\n",                  L_SWR);
  if(L_SYNC>0)      fprintf(fp, "sync    %d\n",                  L_SYNC);
  if(L_SYSCALL>0)   fprintf(fp, "syscall %d\n",                  L_SYSCALL);
  if(L_TEQ>0)       fprintf(fp, "teq     %d\n",                  L_TEQ);
  if(L_TEQI>0)      fprintf(fp, "teqi    %d\n",                  L_TEQI);
  if(L_TGE>0)       fprintf(fp, "tge     %d\n",                  L_TGE);
  if(L_TGEI>0)      fprintf(fp, "tgei    %d\n",                  L_TGEI);
  if(L_TGEIU>0)     fprintf(fp, "tgeiu   %d\n",                  L_TGEIU);
  if(L_TGEU>0)      fprintf(fp, "tgeu    %d\n",                  L_TGEU);
  if(L_TLT>0)       fprintf(fp, "tlt     %d\n",                  L_TLT);
  if(L_TLTI>0)      fprintf(fp, "tlti    %d\n",                  L_TLTI);
  if(L_TLTIU>0)     fprintf(fp, "tltiu   %d\n",                  L_TLTIU);
  if(L_TLTU>0)      fprintf(fp, "tltu    %d\n",                  L_TLTU);
  if(L_TNE>0)       fprintf(fp, "tne     %d\n",                  L_TNE);
  if(L_TNEI>0)      fprintf(fp, "tnei    %d\n",                  L_TNEI);
  if(L_XOR>0)       fprintf(fp, "xor     %d\n",                  L_XOR);
  if(L_XORI>0)      fprintf(fp, "xori    %d\n",                  L_XORI);
  fprintf(fp, "------------\n");
  fprintf(fp, "--MMI  ops--\n");
  fprintf(fp, "------------\n"); 
  if(L_DIV1>0)      fprintf(fp, "div1    %d\n",                  L_DIV1);                
  if(L_DIVU1>0)     fprintf(fp, "divu1   %d\n",                  L_DIVU1);                
  if(L_MADD>0)      fprintf(fp, "madd    %d\n",                  L_MADD);              
  if(L_MADD1>0)     fprintf(fp, "madd1   %d\n",                  L_MADD1);               
  if(L_MADDU>0)     fprintf(fp, "maddu   %d\n",                  L_MADDU);             
  if(L_MADDU1>0)    fprintf(fp, "maddu1  %d\n",                  L_MADDU1);            
  if(L_MFHI1>0)     fprintf(fp, "mfhi1   %d\n",                  L_MFHI1);           
  if(L_MFLO1>0)     fprintf(fp, "mflo1   %d\n",                  L_MFLO1);          
  if(L_MTHI1>0)     fprintf(fp, "mthi1   %d\n",                  L_MTHI1);          
  if(L_MTLO1>0)     fprintf(fp, "mtlo1   %d\n",                  L_MTLO1);          
  if(L_MULT1>0)     fprintf(fp, "mult1   %d\n",                  L_MULT1);         
  if(L_MULTU1>0)    fprintf(fp, "multu1  %d\n",                  L_MULTU1);           
  if(L_PABSH>0)     fprintf(fp, "pabsh   %d\n",                  L_PABSH);          
  if(L_PABSW>0)     fprintf(fp, "pabsw   %d\n",                  L_PABSW);          
  if(L_PADDB>0)     fprintf(fp, "paddb   %d\n",                  L_PADDB);           
  if(L_PADDH>0)     fprintf(fp, "paddh   %d\n",                  L_PADDH);           
  if(L_PADDSB>0)    fprintf(fp, "paddsb  %d\n",                  L_PADDSB);           
  if(L_PADDSH>0)    fprintf(fp, "paddsh  %d\n",                  L_PADDSH);       
  if(L_PADDSW>0)    fprintf(fp, "paddsw  %d\n",                  L_PADDSW);        
  if(L_PADDUB>0)    fprintf(fp, "paddub  %d\n",                  L_PADDUB);       
  if(L_PADDUH>0)    fprintf(fp, "padduh  %d\n",                  L_PADDUH);       
  if(L_PADDUW>0)    fprintf(fp, "padduw  %d\n",                  L_PADDUW);       
  if(L_PADDW>0)     fprintf(fp, "paddw   %d\n",                  L_PADDW);          
  if(L_PADSBH>0)    fprintf(fp, "padsbh  %d\n",                  L_PADSBH);         
  if(L_PAND>0)      fprintf(fp, "pand    %d\n",                  L_PAND);        
  if(L_PCEQB>0)     fprintf(fp, "pceqb   %d\n",                  L_PCEQB);          
  if(L_PCEQH>0)     fprintf(fp, "pceqh   %d\n",                  L_PCEQH);         
  if(L_PCEQW>0)     fprintf(fp, "pceqw   %d\n",                  L_PCEQW);         
  if(L_PCGTB>0)     fprintf(fp, "pcgtb   %d\n",                  L_PCGTB);         
  if(L_PCGTH>0)     fprintf(fp, "pcgth   %d\n",                  L_PCGTH);          
  if(L_PCGTW>0)     fprintf(fp, "pcgtw   %d\n",                  L_PCGTW);            
  if(L_PCPYH>0)     fprintf(fp, "pcpyh   %d\n",                  L_PCPYH);          
  if(L_PCPYLD>0)    fprintf(fp, "pcpyld  %d\n",                  L_PCPYLD);        
  if(L_PCPYUD>0)    fprintf(fp, "pcpyud  %d\n",                  L_PCPYUD);         
  if(L_PDIVBW>0)    fprintf(fp, "pdivbw  %d\n",                  L_PDIVBW);        
  if(L_PDIVUW>0)    fprintf(fp, "pdivuw  %d\n",                  L_PDIVUW);        
  if(L_PDIVW>0)     fprintf(fp, "pdivw   %d\n",                  L_PDIVW);         
  if(L_PEXCH>0)     fprintf(fp, "pexch   %d\n",                  L_PEXCH);
  if(L_PEXCW>0)     fprintf(fp, "pexcw   %d\n",                  L_PEXCW);   
  if(L_PEXEH>0)     fprintf(fp, "pexeh   %d\n",                  L_PEXEH);  
  if(L_PEXEW>0)     fprintf(fp, "pexew   %d\n",                  L_PEXEW); 
  if(L_PEXT5>0)     fprintf(fp, "pext5   %d\n",                  L_PEXT5);  
  if(L_PEXTLB>0)    fprintf(fp, "pextlb  %d\n",                  L_PEXTLB);  
  if(L_PEXTLH>0)    fprintf(fp, "pextlh  %d\n",                  L_PEXTLH); 
  if(L_PEXTLW>0)    fprintf(fp, "pextlw  %d\n",                  L_PEXTLW);  
  if(L_PEXTUB>0)    fprintf(fp, "pextub  %d\n",                  L_PEXTUB);
  if(L_PEXTUH>0)    fprintf(fp, "pextuh  %d\n",                  L_PEXTUH); 
  if(L_PEXTUW>0)    fprintf(fp, "pextuw  %d\n",                  L_PEXTUW);  
  if(L_PHMADH>0)    fprintf(fp, "phmadh  %d\n",                  L_PHMADH);
  if(L_PHMSBH>0)    fprintf(fp, "phmsbh  %d\n",                  L_PHMSBH); 
  if(L_PINTEH>0)    fprintf(fp, "pinteh  %d\n",                  L_PINTEH);    
  if(L_PINTH>0)     fprintf(fp, "pinth   %d\n",                  L_PINTH);   
  if(L_PLZCW>0)     fprintf(fp, "plzcw   %d\n",                  L_PLZCW);   
  if(L_PMADDH>0)    fprintf(fp, "pmaddh  %d\n",                  L_PMADDH);   
  if(L_PMADDUW>0)   fprintf(fp, "pmadduw %d\n",                  L_PMADDUW);  
  if(L_PMADDW>0)    fprintf(fp, "pmaddw  %d\n",                  L_PMADDW);  
  if(L_PMAXH>0)     fprintf(fp, "pmaxh   %d\n",                  L_PMAXH);  
  if(L_PMAXW>0)     fprintf(fp, "pmaxw   %d\n",                  L_PMAXW);  
  if(L_PMFHI>0)     fprintf(fp, "pmfhi   %d\n",                  L_PMFHI);  
  if(L_PMFHL>0)     fprintf(fp, "pmfhl   %d\n",                  L_PMFHL); 
  if(L_PMFLO>0)     fprintf(fp, "pmflo   %d\n",                  L_PMFLO); 
  if(L_PMINH>0)     fprintf(fp, "pminh   %d\n",                  L_PMINH); 
  if(L_PMINW>0)     fprintf(fp, "pminw   %d\n",                  L_PMINW); 
  if(L_PMSUBH>0)    fprintf(fp, "pmsubh  %d\n",                  L_PMSUBH); 
  if(L_PMSUBW>0)    fprintf(fp, "pmsubw  %d\n",                  L_PMSUBW); 
  if(L_PMTHI>0)     fprintf(fp, "pmthi   %d\n",                  L_PMTHI);
  if(L_PMTHL>0)     fprintf(fp, "pmthl   %d\n",                  L_PMTHL); 
  if(L_PMTLO>0)     fprintf(fp, "pmtlo   %d\n",                  L_PMTLO);
  if(L_PMULTH>0)    fprintf(fp, "pmulth  %d\n",                  L_PMULTH);         
  if(L_PMULTUW>0)   fprintf(fp, "pmultuw %d\n",                  L_PMULTUW);         
  if(L_PMULTW>0)    fprintf(fp, "pmultw  %d\n",                  L_PMULTW);         
  if(L_PNOR>0)      fprintf(fp, "pnor    %d\n",                  L_PNOR);           
  if(L_POR>0)       fprintf(fp, "por     %d\n",                  L_POR);         
  if(L_PPAC5>0)     fprintf(fp, "ppac5   %d\n",                  L_PPAC5);         
  if(L_PPACB>0)     fprintf(fp, "ppacb   %d\n",                  L_PPACB);         
  if(L_PPACH>0)     fprintf(fp, "ppach   %d\n",                  L_PPACH);         
  if(L_PPACW>0)     fprintf(fp, "ppacw   %d\n",                  L_PPACW);          
  if(L_PREVH>0)     fprintf(fp, "prevh   %d\n",                  L_PREVH);           
  if(L_PROT3W>0)    fprintf(fp, "prot3w  %d\n",                  L_PROT3W);          
  if(L_PSLLH>0)     fprintf(fp, "psllh   %d\n",                  L_PSLLH);       
  if(L_PSLLVW>0)    fprintf(fp, "psllvw  %d\n",                  L_PSLLVW);         
  if(L_PSLLW>0)     fprintf(fp, "psllw   %d\n",                  L_PSLLW);        
  if(L_PSRAH>0)     fprintf(fp, "psrah   %d\n",                  L_PSRAH);        
  if(L_PSRAVW>0)    fprintf(fp, "psravw  %d\n",                  L_PSRAVW);        
  if(L_PSRAW>0)     fprintf(fp, "psraw   %d\n",                  L_PSRAW);        
  if(L_PSRLH>0)     fprintf(fp, "psrlh   %d\n",                  L_PSRLH);        
  if(L_PSRLVW>0)    fprintf(fp, "psrlvw  %d\n",                  L_PSRLVW);        
  if(L_PSRLW>0)     fprintf(fp, "psrlw   %d\n",                  L_PSRLW);        
  if(L_PSUBB>0)     fprintf(fp, "psubb   %d\n",                  L_PSUBB);          
  if(L_PSUBH>0)     fprintf(fp, "psubh   %d\n",                  L_PSUBH);          
  if(L_PSUBSB>0)    fprintf(fp, "psubsb  %d\n",                  L_PSUBSB);        
  if(L_PSUBSH>0)    fprintf(fp, "psubsh  %d\n",                  L_PSUBSH);        
  if(L_PSUBSW>0)    fprintf(fp, "psubsw  %d\n",                  L_PSUBSW);       
  if(L_PSUBUB>0)    fprintf(fp, "psubub  %d\n",                  L_PSUBUB);       
  if(L_PSUBUH>0)    fprintf(fp, "psubuh  %d\n",                  L_PSUBUH);        
  if(L_PSUBUW>0)    fprintf(fp, "psubuw  %d\n",                  L_PSUBUW);         
  if(L_PSUBW>0)     fprintf(fp, "psubw   %d\n",                  L_PSUBW);         
  if(L_PXOR>0)      fprintf(fp, "pxor    %d\n",                  L_PXOR);       
  if(L_QFSRV>0)     fprintf(fp, "qfsrv   %d\n",                  L_QFSRV);       
  fprintf(fp, "------------\n");
  fprintf(fp, "--COP0 ops--\n");
  fprintf(fp, "------------\n");
  if(L_BC0F>0)      fprintf(fp, "bc0f    %d\n",                  L_BC0F);
  if(L_BC0FL>0)     fprintf(fp, "bc0fl   %d\n",                  L_BC0FL);
  if(L_BC0T>0)      fprintf(fp, "bc0t    %d\n",                  L_BC0T);
  if(L_BC0TL>0)     fprintf(fp, "bc0tl   %d\n",                  L_BC0TL);
  if(L_DI>0)        fprintf(fp, "di      %d\n",                  L_DI);
  if(L_EI>0)        fprintf(fp, "ei      %d\n",                  L_EI);
  if(L_ERET>0)      fprintf(fp, "eret    %d\n",                  L_ERET);
  if(L_MTC0>0)      fprintf(fp, "mtc0    %d\n",                  L_MTC0);
  if(L_TLBP>0)      fprintf(fp, "tlbp    %d\n",                  L_TLBP);
  if(L_TLBR>0)      fprintf(fp, "tlbr    %d\n",                  L_TLBR);
  if(L_TLBWI>0)     fprintf(fp, "tlbwi   %d\n",                  L_TLBWI);
  if(L_TLBWR>0)     fprintf(fp, "tlbwr   %d\n",                  L_TLBWR);
  if(L_MFC0>0)      fprintf(fp, "mfc0    %d\n",                  L_MFC0);
  fprintf(fp, "------------\n");
  fprintf(fp, "--COP1 ops--\n");
  fprintf(fp, "------------\n");
  if(L_LWC1>0)      fprintf(fp, "lwc1    %d\n",                  L_LWC1);
  if(L_SWC1>0)      fprintf(fp, "swc1    %d\n",                  L_SWC1);
  if(L_ABS_S>0)     fprintf(fp, "abs_s   %d\n",                  L_ABS_S);  
  if(L_ADD_S>0)     fprintf(fp, "add_s   %d\n",                  L_ADD_S);   
  if(L_ADDA_S>0)    fprintf(fp, "adda_s  %d\n",                  L_ADDA_S);   
  if(L_BC1F>0)      fprintf(fp, "bc1f    %d\n",                  L_BC1F);
  if(L_BC1FL>0)     fprintf(fp, "bc1fl   %d\n",                  L_BC1FL);
  if(L_BC1T>0)      fprintf(fp, "bc1t    %d\n",                  L_BC1T);
  if(L_BC1TL>0)     fprintf(fp, "bc1tl   %d\n",                  L_BC1TL);
  if(L_C_EQ>0)      fprintf(fp, "c_eq    %d\n",                  L_C_EQ);
  if(L_C_F>0)       fprintf(fp, "c_f     %d\n",                  L_C_F);
  if(L_C_LE>0)      fprintf(fp, "c_le    %d\n",                  L_C_LE);  
  if(L_C_LT>0)      fprintf(fp, "c_lt    %d\n",                  L_C_LT); 
  if(L_CFC1>0)      fprintf(fp, "cfc1    %d\n",                  L_CFC1);
  if(L_CTC1>0)      fprintf(fp, "ctc1    %d\n",                  L_CTC1);
  if(L_CVT_S>0)     fprintf(fp, "cvt_s   %d\n",                  L_CVT_S);
  if(L_CVT_W>0)     fprintf(fp, "cvt_w   %d\n",                  L_CVT_W);
  if(L_DIV_S>0)     fprintf(fp, "div_s   %d\n",                  L_DIV_S); 
  if(L_MADD_S>0)    fprintf(fp, "madd_s  %d\n",                  L_MADD_S);
  if(L_MADDA_S>0)   fprintf(fp, "madda_s %d\n",                  L_MADDA_S);   
  if(L_MAX_S>0)     fprintf(fp, "max_s   %d\n",                  L_MAX_S);
  if(L_MFC1>0)      fprintf(fp, "mfc1    %d\n",                  L_MFC1);
  if(L_MIN_S>0)     fprintf(fp, "min_s   %d\n",                  L_MIN_S);
  if(L_MOV_S>0)     fprintf(fp, "mov_s   %d\n",                  L_MOV_S);
  if(L_MSUB_S>0)    fprintf(fp, "msub_s  %d\n",                  L_MSUB_S);  
  if(L_MSUBA_S>0)   fprintf(fp, "msuba_s %d\n",                  L_MSUBA_S);
  if(L_MTC1>0)      fprintf(fp, "mtc1    %d\n",                  L_MTC1);
  if(L_MUL_S>0)     fprintf(fp, "mul_s   %d\n",                  L_MUL_S); 
  if(L_MULA_S>0)    fprintf(fp, "mula_s  %d\n",                  L_MULA_S);
  if(L_NEG_S>0)     fprintf(fp, "neg_s   %d\n",                  L_NEG_S); 
  if(L_RSQRT_S>0)   fprintf(fp, "rsqrt_s %d\n",                  L_RSQRT_S);  
  if(L_SQRT_S>0)    fprintf(fp, "sqrt_s  %d\n",                  L_SQRT_S); 
  if(L_SUB_S>0)     fprintf(fp, "sub_s   %d\n",                  L_SUB_S); 
  if(L_SUBA_S>0)    fprintf(fp, "suba_s  %d\n",                  L_SUBA_S); 
  fprintf(fp, "------------\n");
  fprintf(fp, "--COP2 ops--\n");
  fprintf(fp, "------------\n");
  if(L_LQC2>0)      fprintf(fp, "lqc2    %d\n",                  L_LQC2);
  if(L_SQC2>0)      fprintf(fp, "sqc2    %d\n",                  L_SQC2); 
  if(L_BC2F>0)      fprintf(fp, "bc2f    %d\n",                  L_BC2F);         
  if(L_BC2FL>0)     fprintf(fp, "bc2fl   %d\n",                  L_BC2FL);        
  if(L_BC2T>0)      fprintf(fp, "bc2t    %d\n",                  L_BC2T);        
  if(L_BC2TL>0)     fprintf(fp, "bc2tl   %d\n",                  L_BC2TL);        
  if(L_CFC2>0)      fprintf(fp, "cfc2    %d\n",                  L_CFC2);          
  if(L_CTC2>0)      fprintf(fp, "ctc2    %d\n",                  L_CTC2);           
  if(L_QMFC2>0)     fprintf(fp, "qmfc2   %d\n",                  L_QMFC2);         
  if(L_QMTC2>0)     fprintf(fp, "qmtc2   %d\n",                  L_QMTC2);        
  if(L_VABS>0)      fprintf(fp, "vabs    %d\n",                  L_VABS);                
  if(L_VADD>0)      fprintf(fp, "vadd    %d\n",                  L_VADD);                
  if(L_VADDA>0)     fprintf(fp, "vadda   %d\n",                  L_VADDA);               
  if(L_VADDAi>0)    fprintf(fp, "vaddai  %d\n",                  L_VADDAi);               
  if(L_VADDAq>0)    fprintf(fp, "vaddaq  %d\n",                  L_VADDAq);               
  if(L_VADDAw>0)    fprintf(fp, "vaddaw  %d\n",                  L_VADDAw);                   
  if(L_VADDAx>0)    fprintf(fp, "vaddax  %d\n",                  L_VADDAx);                  
  if(L_VADDAy>0)    fprintf(fp, "vadday  %d\n",                  L_VADDAy);                 
  if(L_VADDAz>0)    fprintf(fp, "vaddaz  %d\n",                  L_VADDAz);                 
  if(L_VADDi>0)     fprintf(fp, "vaddi   %d\n",                  L_VADDi);                 
  if(L_VADDq>0)     fprintf(fp, "vaddq   %d\n",                  L_VADDq);           
  if(L_VADDw>0)     fprintf(fp, "vaddw   %d\n",                  L_VADDw);                  
  if(L_VADDx>0)     fprintf(fp, "vaddx   %d\n",                  L_VADDx);                  
  if(L_VADDy>0)     fprintf(fp, "vaddy   %d\n",                  L_VADDy);                 
  if(L_VADDz>0)     fprintf(fp, "vaddz   %d\n",                  L_VADDz);                 
  if(L_VCALLMS>0)   fprintf(fp, "vcallms %d\n",                  L_VCALLMS);               
  if(L_VCALLMSR>0)  fprintf(fp, "vcallmsr %d\n",                  L_VCALLMSR);             
  if(L_VCLIPw>0)    fprintf(fp, "vclip   %d\n",                  L_VCLIPw);          
  if(L_VDIV>0)      fprintf(fp, "vdiv    %d\n",                  L_VDIV);                 
  if(L_VFTOI0>0)    fprintf(fp, "vftoi0  %d\n",                  L_VFTOI0);               
  if(L_VFTOI12>0)   fprintf(fp, "vftoi12 %d\n",                  L_VFTOI12);              
  if(L_VFTOI15>0)   fprintf(fp, "vftoi15 %d\n",                  L_VFTOI15);  
  if(L_VFTOI4>0)    fprintf(fp, "vftoi14 %d\n",                  L_VFTOI4);           
  if(L_VIADD>0)     fprintf(fp, "viadd   %d\n",                  L_VIADD);            
  if(L_VIADDI>0)    fprintf(fp, "viaddi  %d\n",                  L_VIADDI);           
  if(L_VIAND>0)     fprintf(fp, "viand   %d\n",                  L_VIAND);           
  if(L_VILWR>0)     fprintf(fp, "vilwr   %d\n",                  L_VILWR);            
  if(L_VIOR>0)      fprintf(fp, "vior    %d\n",                  L_VIOR);             
  if(L_VISUB>0)     fprintf(fp, "visub   %d\n",                  L_VISUB);             
  if(L_VISWR>0)     fprintf(fp, "viswr   %d\n",                  L_VISWR);        
  if(L_VITOF0>0)    fprintf(fp, "vitof0  %d\n",                  L_VITOF0);            
  if(L_VITOF12>0)   fprintf(fp, "vitof12 %d\n",                  L_VITOF12);           
  if(L_VITOF15>0)   fprintf(fp, "vitof15 %d\n",                  L_VITOF15);           
  if(L_VITOF4>0)    fprintf(fp, "vitof4  %d\n",                  L_VITOF4);            
  if(L_VLQD>0)      fprintf(fp, "vlqd    %d\n",                  L_VLQD);            
  if(L_VLQI>0)      fprintf(fp, "vlqi    %d\n",                  L_VLQI);             
  if(L_VMADD>0)     fprintf(fp, "vmadd   %d\n",                  L_VMADD);            
  if(L_VMADDA>0)    fprintf(fp, "vmadda  %d\n",                  L_VMADDA);           
  if(L_VMADDAi>0)   fprintf(fp, "vmaddai %d\n",                  L_VMADDAi);          
  if(L_VMADDAq>0)   fprintf(fp, "vmaddaq %d\n",                  L_VMADDAq);         
  if(L_VMADDAw>0)   fprintf(fp, "vmaddaw %d\n",                  L_VMADDAw);         
  if(L_VMADDAx>0)   fprintf(fp, "vmaddax %d\n",                  L_VMADDAx);           
  if(L_VMADDAy>0)   fprintf(fp, "vmadday %d\n",                  L_VMADDAy);          
  if(L_VMADDAz>0)   fprintf(fp, "vmaddaz %d\n",                  L_VMADDAz);           
  if(L_VMADDi>0)    fprintf(fp, "vmaddi  %d\n",                  L_VMADDi);         
  if(L_VMADDq>0)    fprintf(fp, "vmaddq  %d\n",                  L_VMADDq);         
  if(L_VMADDw>0)    fprintf(fp, "vmaddw  %d\n",                  L_VMADDw);     
  if(L_VMADDx>0)    fprintf(fp, "vmaddx  %d\n",                  L_VMADDx);      
  if(L_VMADDy>0)    fprintf(fp, "vmaddy  %d\n",                  L_VMADDy);       
  if(L_VMADDz>0)    fprintf(fp, "vmaddz  %d\n",                  L_VMADDz);       
  if(L_VMAX>0)      fprintf(fp, "vmax    %d\n",                  L_VMAX);            
  if(L_VMAXi>0)     fprintf(fp, "vmaxi   %d\n",                  L_VMAXi);            
  if(L_VMAXw>0)     fprintf(fp, "vmaxw   %d\n",                  L_VMAXw);           
  if(L_VMAXx>0)     fprintf(fp, "vmaxx   %d\n",                  L_VMAXx);           
  if(L_VMAXy>0)     fprintf(fp, "vmaxy   %d\n",                  L_VMAXy);         
  if(L_VMAXz>0)     fprintf(fp, "vmaxz   %d\n",                  L_VMAXz);         
  if(L_VMFIR>0)     fprintf(fp, "vmfir   %d\n",                  L_VMFIR);   
  if(L_VMINI>0)     fprintf(fp, "vmini   %d\n",                  L_VMINI);       
  if(L_VMINIi>0)    fprintf(fp, "vminii  %d\n",                  L_VMINIi);     
  if(L_VMINIw>0)    fprintf(fp, "vminiw  %d\n",                  L_VMINIw);    
  if(L_VMINIx>0)    fprintf(fp, "vminix  %d\n",                  L_VMINIx);          
  if(L_VMINIy>0)    fprintf(fp, "vminiy  %d\n",                  L_VMINIy);           
  if(L_VMINIz>0)    fprintf(fp, "vminiz  %d\n",                  L_VMINIz);           
  if(L_VMOVE>0)     fprintf(fp, "vmove   %d\n",                  L_VMOVE);          
  if(L_VMR32>0)     fprintf(fp, "vmr32   %d\n",                  L_VMR32);          
  if(L_VMSUB>0)     fprintf(fp, "vmsub   %d\n",                  L_VMSUB);           
  if(L_VMSUBA>0)    fprintf(fp, "vmsuba  %d\n",                  L_VMSUBA);           
  if(L_VMSUBAi>0)   fprintf(fp, "vmsubai %d\n",                  L_VMSUBAi);    
  if(L_VMSUBAq>0)   fprintf(fp, "vmsubaq %d\n",                  L_VMSUBAq);         
  if(L_VMSUBAw>0)   fprintf(fp, "vmsubaw %d\n",                  L_VMSUBAw);     
  if(L_VMSUBAx>0)   fprintf(fp, "vmsubax %d\n",                  L_VMSUBAx);         
  if(L_VMSUBAy>0)   fprintf(fp, "vmsubay %d\n",                  L_VMSUBAy);          
  if(L_VMSUBAz>0)   fprintf(fp, "vmsubaz %d\n",                  L_VMSUBAz);          
  if(L_VMSUBi>0)    fprintf(fp, "vmsubi  %d\n",                  L_VMSUBi);       
  if(L_VMSUBq>0)    fprintf(fp, "vmsubq  %d\n",                  L_VMSUBq);              
  if(L_VMSUBw>0)    fprintf(fp, "vmsubw  %d\n",                  L_VMSUBw);       
  if(L_VMSUBx>0)    fprintf(fp, "vmsubx  %d\n",                  L_VMSUBx);       
  if(L_VMSUBy>0)    fprintf(fp, "vmsuby  %d\n",                  L_VMSUBy);       
  if(L_VMSUBz>0)    fprintf(fp, "vmsubz  %d\n",                  L_VMSUBz);             
  if(L_VMTIR>0)     fprintf(fp, "vmtir   %d\n",                  L_VMTIR);              
  if(L_VMUL>0)      fprintf(fp, "vmul    %d\n",                  L_VMUL);               
  if(L_VMULA>0)     fprintf(fp, "vmula   %d\n",                  L_VMULA);              
  if(L_VMULAi>0)    fprintf(fp, "vmulai  %d\n",                  L_VMULAi);              
  if(L_VMULAq>0)    fprintf(fp, "vmulaq  %d\n",                  L_VMULAq);             
  if(L_VMULAw>0)    fprintf(fp, "vmulaw  %d\n",                  L_VMULAw);            
  if(L_VMULAx>0)    fprintf(fp, "vmulax  %d\n",                  L_VMULAx);           
  if(L_VMULAy>0)    fprintf(fp, "vmulay  %d\n",                  L_VMULAy);           
  if(L_VMULAz>0)    fprintf(fp, "vmulaz  %d\n",                  L_VMULAz);             
  if(L_VMULi>0)     fprintf(fp, "vmuli   %d\n",                   L_VMULi);                    
  if(L_VMULq>0)     fprintf(fp, "vmulq   %d\n",                   L_VMULq);                   
  if(L_VMULw>0)     fprintf(fp, "vmulw   %d\n",                   L_VMULw);                 
  if(L_VMULx>0)     fprintf(fp, "vmulx   %d\n",                   L_VMULx);                  
  if(L_VMULy>0)     fprintf(fp, "vmuly   %d\n",                   L_VMULy);                  
  if(L_VMULz>0)     fprintf(fp, "vmulz   %d\n",                   L_VMULz);                  
  if(L_VNOP>0)      fprintf(fp, "vnop    %d\n",                    L_VNOP);             
  if(L_VOPMSUB>0)   fprintf(fp, "vopmsub %d\n",                  L_VOPMSUB);                 
  if(L_VOPMULA>0)   fprintf(fp, "vopmula %d\n",                  L_VOPMULA);                
  if(L_VRGET>0)     fprintf(fp, "vrget   %d\n",                   L_VRGET);                 
  if(L_VRINIT>0)    fprintf(fp, "vrinit  %d\n",                   L_VRINIT);               
  if(L_VRNEXT>0)    fprintf(fp, "vrnext  %d\n",                   L_VRNEXT);               
  if(L_VRSQRT>0)    fprintf(fp, "vrsqrt  %d\n",                   L_VRSQRT);                
  if(L_VRXOR>0)     fprintf(fp, "vrxor   %d\n",                   L_VRXOR);           
  if(L_VSQD>0)      fprintf(fp, "vsqd    %d\n",                   L_VSQD);             
  if(L_VSQI>0)      fprintf(fp, "vsqi    %d\n",                   L_VSQI);                 
  if(L_VSQRT>0)     fprintf(fp, "vsqrt   %d\n",                   L_VSQRT );                
  if(L_VSUB>0)      fprintf(fp, "vsub    %d\n",                   L_VSUB);                   
  if(L_VSUBA>0)     fprintf(fp, "vsuba   %d\n",                   L_VSUBA);                
  if(L_VSUBAi>0)    fprintf(fp, "vsubai  %d\n",                   L_VSUBAi);                
  if(L_VSUBAq>0)    fprintf(fp, "vsubaq  %d\n",                   L_VSUBAq);                
  if(L_VSUBAw>0)    fprintf(fp, "vsubaw  %d\n",                   L_VSUBAw);          
  if(L_VSUBAx>0)    fprintf(fp, "vsubax  %d\n",                   L_VSUBAx);                 
  if(L_VSUBAy>0)    fprintf(fp, "vsubay  %d\n",                   L_VSUBAy);                 
  if(L_VSUBAz>0)    fprintf(fp, "vsubaz  %d\n",                   L_VSUBAz);                 
  if(L_VSUBi>0)     fprintf(fp, "vsubi   %d\n",                   L_VSUBi);                   
  if(L_VSUBq>0)     fprintf(fp, "vsubq   %d\n",                   L_VSUBq);                   
  if(L_VSUBw>0)     fprintf(fp, "vsubw   %d\n",                   L_VSUBw);            
  if(L_VSUBx>0)     fprintf(fp, "vsubx   %d\n",                   L_VSUBx);                   
  if(L_VSUBy>0)     fprintf(fp, "vsuby   %d\n",                   L_VSUBy);                    
  if(L_VSUBz>0)     fprintf(fp, "vsubz   %d\n",                   L_VSUBz);           
  if(L_VWAITQ>0)    fprintf(fp, "vwaitq  %d\n",                   L_VWAITQ);                 

  fclose(fp);

}











/***************/
//R5900
void LT_SPECIAL() {LT_SpecialPrintTable[_Funct_]();}
void LT_REGIMM()  {LT_REGIMMPrintTable[_Rt_]();    }
void LT_UnknownOpcode() {}
void LT_ADDI() 	{ L_ADDI++;}
void LT_ADDIU() { L_ADDIU++;}   
void LT_DADDI() { L_DADDI++;}   
void LT_DADDIU(){ L_DADDIU++;} 
void LT_ANDI()  { L_ANDI++;} 	
void LT_ORI()   { L_ORI++;}	   
void LT_XORI()  {L_XORI++;}
void LT_SLTI()  {L_SLTI++;}    
void LT_SLTIU() {L_SLTIU++;}   
void LT_ADD()   { L_ADD++;}
void LT_ADDU()  { L_ADDU++;} 	
void LT_DADD()  { L_DADD++;}     
void LT_DADDU() { L_DADDU++;}   
void LT_SUB()   { L_SUB++;}	  
void LT_SUBU()  { L_SUBU++;} 	
void LT_DSUB()  { L_DSUB++;} 	
void LT_DSUBU() { L_DSUBU++;}	
void LT_AND() 	{ L_AND++;}    
void LT_OR() 	{ L_OR++;}    
void LT_XOR()   { L_XOR++;}	   
void LT_NOR()   { L_NOR++;}	   
void LT_SLT()	{ L_SLT++;}	
void LT_SLTU()	{ L_SLTU++;}	
void LT_J()     { L_J++;} 
void LT_JAL()   { L_JAL++;}
void LT_JR()    { L_JR++;}
void LT_JALR()  { L_JALR++;}
void LT_DIV()   { L_DIV++;}
void LT_DIVU()  { L_DIVU++;}
void LT_MULT()  { L_MULT++;}
void LT_MULTU() { L_MULTU++;}
void LT_LUI()   { L_LUI++;}
void LT_MFHI()  { L_MFHI++;}
void LT_MFLO()  { L_MFLO++;}
void LT_MTHI()  { L_MTHI++;}
void LT_MTLO()  { L_MTLO++;}
void LT_SLL()   { L_SLL++;}
void LT_DSLL()  { L_DSLL++;} 
void LT_DSLL32(){ L_DSLL32++;}
void LT_SRA()   { L_SRA++;}
void LT_DSRA()  { L_DSRA++;}
void LT_DSRA32(){ L_DSRA32++;}
void LT_SRL()   { L_SRL++;}
void LT_DSRL()  { L_DSRL++;}
void LT_DSRL32(){ L_DSRL32++;}
void LT_SLLV()  { L_SLLV++;}
void LT_SRAV()  { L_SRAV++;}
void LT_SRLV()  { L_SRLV++;}
void LT_DSLLV() { L_DSLLV++;}
void LT_DSRAV() { L_DSRAV++;}
void LT_DSRLV() { L_DSRLV++;}
void LT_BEQ()   { L_BEQ++;}
void LT_BNE()   { L_BNE++;}
void LT_BGEZ()  { L_BGEZ++;} 
void LT_BGEZAL(){ L_BGEZAL++;}
void LT_BGTZ()  { L_BGTZ++;} 
void LT_BLEZ()  { L_BLEZ++;} 
void LT_BLTZ()  { L_BLTZ++;} 
void LT_BLTZAL(){ L_BLTZAL++;} 
void LT_BEQL()  { L_BEQL++;}  
void LT_BNEL()  { L_BNEL++;}  
void LT_BLEZL() { L_BLEZL++;}  
void LT_BGTZL() { L_BGTZL++;} 
void LT_BLTZL() { L_BLTZL++;}  
void LT_BGEZL() { L_BGEZL++;}  
void LT_BLTZALL(){ L_BLTZALL++;} 
void LT_BGEZALL(){ L_BGEZALL++;}
void LT_LB()    {L_LB++;}
void LT_LBU()   {L_LBU++;}
void LT_LH()    {L_LH++;}
void LT_LHU()   {L_LHU++;}
void LT_LW()    {L_LW++;}
void LT_LWU()   {L_LWU++;} 
void LT_LWL()   {L_LWL++;}
void LT_LWR()   {L_LWR++;}
void LT_LD()    {L_LD++;}
void LT_LDL()   {L_LDL++;}
void LT_LDR()   {L_LDR++;}
void LT_LQ()    {L_LQ++;}
void LT_SB()    {L_SB++;}
void LT_SH()    {L_SH++;}
void LT_SW()    {L_SW++;}
void LT_SWL()   {L_SWL++;}
void LT_SWR()   {L_SWR++;}
void LT_SD()    {L_SD++;}
void LT_SDL()   {L_SDL++;}
void LT_SDR()   {L_SDR++;}
void LT_SQ()    {L_SQ++;}
void LT_MOVZ()  {L_MOVZ++;}
void LT_MOVN()  {L_MOVN++;}
void LT_SYSCALL() {L_SYSCALL++;}
void LT_BREAK() {L_BREAK++;}
void LT_CACHE() {L_CACHE++;}		
void LT_MFSA()  {L_MFSA++;}
void LT_MTSA()  {L_MTSA++;}
void LT_SYNC()  {L_SYNC++;}
void LT_PREF()  {L_PREF++;}
void LT_TGE()   {L_TGE++;}
void LT_TGEU()  {L_TGEU++;}
void LT_TLT()   {L_TLT++;}
void LT_TLTU()  {L_TLTU++;}
void LT_TEQ()   {L_TEQ++;}
void LT_TNE()   {L_TNE++;}
void LT_TGEI()  {L_TGEI++;}
void LT_TGEIU() {L_TGEIU++;}
void LT_TLTI()  {L_TLTI++;}
void LT_TLTIU() {L_TLTIU++;}
void LT_TEQI()  {L_TEQI++;}
void LT_TNEI()  {L_TNEI++;}
void LT_MTSAB() {L_MTSAB++;}
void LT_MTSAH() {L_MTSAH++;}
//cop0
void LT_COP0(){ LT_COP0PrintTable[_Rs_]();}
void LT_COP0_BC0() {LT_COP0BC0PrintTable[(cpuRegs.code >> 16) & 0x03]();}
void LT_COP0_Func() { LT_COP0C0PrintTable[_Funct_](); }
void LT_COP0_Unknown() { }
void LT_MFC0() {L_MFC0++;}  
void LT_MTC0() {L_MTC0++;}
void LT_BC0F() {L_BC0F++;}
void LT_BC0T() {L_BC0T++;}
void LT_BC0FL(){L_BC0FL++;}
void LT_BC0TL(){L_BC0TL++;}
void LT_TLBR() {L_TLBR++;}
void LT_TLBWI(){L_TLBWI++;} 
void LT_TLBWR(){L_TLBWR++;}  
void LT_TLBP() {L_TLBP++;}
void LT_ERET() {L_ERET++;}
void LT_DI()   {L_DI++;}
void LT_EI()   {L_EI++;}
//mmi
void LT_MMI() {LT_MMIPrintTable[_Funct_]();}
void LT_MMI0() {LT_MMI0PrintTable[_Sa_]();}
void LT_MMI1() {LT_MMI1PrintTable[_Sa_]();}
void LT_MMI2() {LT_MMI2PrintTable[_Sa_]();}
void LT_MMI3() {LT_MMI3PrintTable[_Sa_]();}
void LT_MMI_Unknown() {}
void LT_MADD() {L_MADD++;}
void LT_MADDU(){L_MADDU++;}
void LT_PLZCW(){L_PLZCW++;}
void LT_MADD1(){L_MADD1++;}
void LT_MADDU1(){L_MADDU1++;}
void LT_MFHI1(){L_MFHI1++;}
void LT_MTHI1(){L_MTHI1++;}
void LT_MFLO1(){L_MFLO1++;}
void LT_MTLO1(){L_MTLO1++;}
void LT_MULT1(){L_MULT1++;}
void LT_MULTU1(){L_MULTU1++;}
void LT_DIV1(){L_DIV1++;}
void LT_DIVU1(){L_DIVU1++;}
void LT_PMFHL(){L_PMFHL++;}
void LT_PMTHL(){L_PMTHL++;}
void LT_PSLLH(){L_PSLLH++;}
void LT_PSRLH(){L_PSRLH++;}
void LT_PSRAH(){L_PSRAH++;}
void LT_PSLLW(){L_PSLLW++;}
void LT_PSRLW(){L_PSRLW++;}
void LT_PSRAW(){L_PSRAW++;}
void LT_PADDW(){L_PADDW++;}
void LT_PSUBW(){L_PSUBW++;}
void LT_PCGTW(){L_PCGTW++;}
void LT_PMAXW(){L_PMAXW++;}
void LT_PADDH(){L_PADDH++;}
void LT_PSUBH(){L_PSUBH++;}
void LT_PCGTH(){L_PCGTH++;}
void LT_PMAXH(){L_PMAXH++;}
void LT_PADDB(){L_PADDB++;}
void LT_PSUBB(){L_PSUBB++;}
void LT_PCGTB(){L_PCGTB++;}
void LT_PADDSW(){L_PADDSW++;}
void LT_PSUBSW(){L_PSUBSW++;}
void LT_PEXTLW(){L_PEXTLW++;}
void LT_PPACW(){L_PPACW++;}
void LT_PADDSH(){L_PADDSH++;}
void LT_PSUBSH(){L_PSUBSH++;}
void LT_PEXTLH(){L_PEXTLH++;}
void LT_PPACH(){L_PPACH++;}
void LT_PADDSB(){L_PADDSB++;}
void LT_PSUBSB(){L_PSUBSB++;}
void LT_PEXTLB(){L_PEXTLB++;}
void LT_PPACB(){L_PPACB++;}
void LT_PEXT5(){L_PEXT5++;} 
void LT_PPAC5(){L_PPAC5++;}
void LT_PABSW(){L_PABSW++;}
void LT_PCEQW(){L_PCEQW++;}
void LT_PMINW(){L_PMINW++;}
void LT_PADSBH(){L_PADSBH++;}
void LT_PABSH(){L_PABSH++;}
void LT_PCEQH(){L_PCEQH++;}
void LT_PMINH(){L_PMINH++;}
void LT_PCEQB(){L_PCEQB++;}
void LT_PADDUW(){L_PADDUW++;}
void LT_PSUBUW(){L_PSUBUW++;}
void LT_PEXTUW(){L_PEXTUW++;}
void LT_PADDUH(){L_PADDUH++;}
void LT_PSUBUH(){L_PSUBUH++;}
void LT_PEXTUH(){L_PEXTUH++;}
void LT_PADDUB(){L_PADDUB++;}
void LT_PSUBUB(){L_PSUBUB++;}
void LT_PEXTUB(){L_PEXTUB++;}
void LT_QFSRV(){L_QFSRV++;}
void LT_PMADDW(){L_PMADDW++;}
void LT_PSLLVW(){L_PSLLVW++;}
void LT_PSRLVW(){L_PSRLVW++;}
void LT_PMSUBW(){L_PMSUBW++;}
void LT_PMFHI(){L_PMFHI++;}
void LT_PMFLO(){L_PMFLO++;}
void LT_PINTH(){L_PINTH++;}
void LT_PMULTW(){L_PMULTW++;}
void LT_PDIVW(){L_PDIVW++;}
void LT_PCPYLD(){L_PCPYLD++;}
void LT_PMADDH(){L_PMADDH++;}
void LT_PHMADH(){L_PHMADH++;}
void LT_PAND(){L_PAND++;}
void LT_PXOR(){L_PXOR++;}
void LT_PMSUBH(){L_PMSUBH++;}
void LT_PHMSBH(){L_PHMSBH++;}
void LT_PEXEH(){L_PEXEH++;}
void LT_PREVH(){L_PREVH++;}
void LT_PMULTH(){L_PMULTH++;}
void LT_PDIVBW(){L_PDIVBW++;}
void LT_PEXEW(){L_PEXEW++;}
void LT_PROT3W(){L_PROT3W++;}
void LT_PMADDUW(){L_PMADDUW++;}
void LT_PSRAVW(){L_PSRAVW++;}
void LT_PMTHI(){L_MTHI++;}
void LT_PMTLO(){L_PMTLO++;}
void LT_PINTEH(){L_PINTEH++;}
void LT_PMULTUW(){L_PMULTUW++;}
void LT_PDIVUW(){L_PDIVUW++;}
void LT_PCPYUD(){L_PCPYUD++;}
void LT_POR(){L_POR++;}
void LT_PNOR(){L_PNOR++;}
void LT_PEXCH(){L_PEXCH++;}
void LT_PCPYH(){L_PCPYH++;}
void LT_PEXCW(){L_PEXCW++;}
//COP1
void LT_COP1() {LT_COP1PrintTable[_Rs_]();}
void LT_LWC1() {L_LWC1++;}
void LT_SWC1() {L_SWC1++;}
void LT_COP1_BC1() {LT_COP1BC1PrintTable[_Rt_]();}
void LT_COP1_S() {LT_COP1SPrintTable[_Funct_]();}
void LT_COP1_W() {LT_COP1WPrintTable[_Funct_]();}
void LT_COP1_Unknown() {}
void LT_MFC1(){L_MFC1++;}
void LT_CFC1(){L_CFC1++;}
void LT_MTC1(){L_MTC1++;}
void LT_CTC1(){L_CTC1++;}
void LT_BC1F(){L_BC1F++;}
void LT_BC1T(){L_BC1T++;}
void LT_BC1FL(){L_BC1FL++;}
void LT_BC1TL(){L_BC1TL++;}
void LT_ADD_S(){L_ADD_S++;}
void LT_SUB_S(){L_SUB_S++;}
void LT_MUL_S(){L_MUL_S++;}
void LT_DIV_S(){L_DIV_S++;}  
void LT_SQRT_S(){L_SQRT_S++;}
void LT_ABS_S(){L_ABS_S++;}
void LT_MOV_S(){L_MOV_S++;}
void LT_NEG_S(){L_NEG_S++;}
void LT_RSQRT_S(){L_RSQRT_S++;}
void LT_ADDA_S(){L_ADDA_S++;}
void LT_SUBA_S(){L_SUBA_S++;}
void LT_MULA_S(){L_MULA_S++;}
void LT_MADD_S(){L_MADD_S++;}
void LT_MSUB_S(){L_MSUB_S++;}
void LT_MADDA_S(){L_MADDA_S++;}
void LT_MSUBA_S(){L_MSUBA_S++;}
void LT_CVT_W(){L_CVT_W++;}
void LT_MAX_S(){L_MAX_S++;}
void LT_MIN_S(){L_MIN_S++;}
void LT_C_F(){L_C_F++;}
void LT_C_EQ(){L_C_EQ++;}
void LT_C_LT(){L_C_LT++;}
void LT_C_LE(){L_C_LE++;}
void LT_CVT_S(){L_CVT_S++;}
//cop2

void LT_LQC2() {L_LQC2++;}
void LT_SQC2() {L_SQC2++;}
void LT_COP2() {LT_COP2PrintTable[_Rs_]();}
void LT_COP2_BC2() {LT_COP2BC2PrintTable[_Rt_]();}
void LT_COP2_SPECIAL() { LT_COP2SPECIAL1PrintTable[_Funct_]();}
void LT_COP2_SPECIAL2() {LT_COP2SPECIAL2PrintTable[(cpuRegs.code & 0x3) | ((cpuRegs.code >> 4) & 0x7c)]();}
void LT_COP2_Unknown(){}
void LT_QMFC2(){L_QMFC2++;} 
void LT_CFC2(){L_CFC2++;}
void LT_QMTC2(){L_QMTC2++;}
void LT_CTC2(){L_CTC2++;}
void LT_BC2F(){L_BC2F++;}
void LT_BC2T(){L_BC2T++;}
void LT_BC2FL(){L_BC2FL++;}
void LT_BC2TL(){L_BC2TL++;}
void LT_VADDx(){L_VADDx++;}
void LT_VADDy(){L_VADDy++;}
void LT_VADDz(){L_VADDz++;}
void LT_VADDw(){L_VADDw++;}       
void LT_VSUBx(){L_VSUBx++;}        
void LT_VSUBy(){L_VSUBy++;}        
void LT_VSUBz(){L_VSUBz++;}
void LT_VSUBw(){L_VSUBw++;} 
void LT_VMADDx(){L_VMADDx++;}
void LT_VMADDy(){L_VMADDy++;}
void LT_VMADDz(){L_VMADDz++;}
void LT_VMADDw(){L_VMADDw++;}
void LT_VMSUBx(){L_VMSUBx++;}
void LT_VMSUBy(){L_VMSUBy++;}
void LT_VMSUBz(){L_VMSUBz++;}       
void LT_VMSUBw(){L_VMSUBw++;} 
void LT_VMAXx(){L_VMAXx++;}      
void LT_VMAXy(){L_VMAXy++;}       
void LT_VMAXz(){L_VMAXz++;}       
void LT_VMAXw(){L_VMAXw++;}      
void LT_VMINIx(){L_VMINIx++;}       
void LT_VMINIy(){L_VMINIy++;}      
void LT_VMINIz(){L_VMINIz++;}      
void LT_VMINIw(){L_VMINIw++;}
void LT_VMULx(){L_VMULx++;}       
void LT_VMULy(){L_VMULy++;}      
void LT_VMULz(){L_VMULz++;}       
void LT_VMULw(){L_VMULw++;}      
void LT_VMULq(){L_VMULq++;}        
void LT_VMAXi(){L_VMAXi++;}        
void LT_VMULi(){L_VMULi++;}       
void LT_VMINIi(){L_VMINIi++;}
void LT_VADDq(){L_VADDq++;}
void LT_VMADDq(){L_VMADDq++;}     
void LT_VADDi(){L_VADDi++;}      
void LT_VMADDi(){L_VMADDi++;}     
void LT_VSUBq(){L_VSUBq++;}       
void LT_VMSUBq(){L_VMSUBq++;}     
void LT_VSUBi(){L_VSUBi++;}       
void LT_VMSUBi(){L_VMSUBi++;} 
void LT_VADD(){L_VADD++;}      
void LT_VMADD(){L_VMADD++;}       
void LT_VMUL(){L_VMUL++;}      
void LT_VMAX(){L_VMAX++;}        
void LT_VSUB(){L_VSUB++;}         
void LT_VMSUB(){L_VMSUB++;}      
void LT_VOPMSUB(){L_VOPMSUB++;}      
void LT_VMINI(){L_VMINI++;}  
void LT_VIADD(){L_VIADD++;}       
void LT_VISUB(){L_VISUB++;}      
void LT_VIADDI(){L_VIADDI++;}       
void LT_VIAND(){L_VIAND++;}        
void LT_VIOR(){L_VIOR++;}        
void LT_VCALLMS(){L_VCALLMS++;}     
void LT_VCALLMSR(){L_VCALLMSR++;}  
void LT_VADDAx(){L_VADDAx++;}      
void LT_VADDAy(){L_VADDAy++;}      
void LT_VADDAz(){L_VADDAz++;}      
void LT_VADDAw(){L_VADDAw++;}      
void LT_VSUBAx(){L_VSUBAx++;}      
void LT_VSUBAy(){L_VSUBAy++;}      
void LT_VSUBAz(){L_VSUBAz++;}      
void LT_VSUBAw(){L_VSUBAw++;}
void LT_VMADDAx(){L_VMADDAx++;}     
void LT_VMADDAy(){L_VMADDAy++;}     
void LT_VMADDAz(){L_VMADDAz++;}     
void LT_VMADDAw(){L_VMADDAw++;}     
void LT_VMSUBAx(){L_VMSUBAx++;}     
void LT_VMSUBAy(){L_VMSUBAy++;}     
void LT_VMSUBAz(){L_VMSUBAz++;}    
void LT_VMSUBAw(){L_VMSUBAw++;}
void LT_VITOF0(){L_VITOF0++;}      
void LT_VITOF4(){L_VITOF4++;}      
void LT_VITOF12(){L_VITOF12++;}     
void LT_VITOF15(){L_VITOF15++;}     
void LT_VFTOI0(){L_VFTOI0++;}      
void LT_VFTOI4(){L_VFTOI4++;}      
void LT_VFTOI12(){L_VFTOI12++;}     
void LT_VFTOI15(){L_VFTOI15++;}
void LT_VMULAx(){L_VMULAx++;}      
void LT_VMULAy(){L_VMULAy++;}      
void LT_VMULAz(){L_VMULAz++;}      
void LT_VMULAw(){L_VMULAw++;}      
void LT_VMULAq(){L_VMULAq++;}      
void LT_VABS(){L_VABS++;}        
void LT_VMULAi(){L_VMULAi++;}      
void LT_VCLIPw(){L_VCLIPw++;}
void LT_VADDAq(){L_VADDAq++;}      
void LT_VMADDAq(){L_VMADDAq++;}     
void LT_VADDAi(){L_VADDAi++;}      
void LT_VMADDAi(){L_VMADDAi++;}     
void LT_VSUBAq(){L_VSUBAq++;}      
void LT_VMSUBAq(){L_VMSUBAq++;}     
void LT_VSUBAi(){L_VSUBAi++;}      
void LT_VMSUBAi(){L_VMSUBAi++;}
void LT_VADDA(){L_VADDA++;}      
void LT_VMADDA(){L_VMADDA++;}      
void LT_VMULA(){L_VMULA++;}       
void LT_VSUBA(){L_VSUBA++;}       
void LT_VMSUBA(){L_VMSUBA++;}      
void LT_VOPMULA(){L_VOPMULA++;}     
void LT_VNOP(){L_VNOP++;}   
void LT_VMOVE(){L_VMOVE++;}       
void LT_VMR32(){L_VMR32++;}       
void LT_VLQI(){L_VLQI++;}       
void LT_VSQI(){L_VSQI++;}        
void LT_VLQD(){L_VLQD++;}        
void LT_VSQD(){L_VSQD++;}   
void LT_VDIV(){L_VDIV++;}        
void LT_VSQRT(){L_VSQRT++;}       
void LT_VRSQRT(){L_VRSQRT++;}      
void LT_VWAITQ(){L_VWAITQ++;}     
void LT_VMTIR(){L_VMTIR++;}       
void LT_VMFIR(){L_VMFIR++;}       
void LT_VILWR(){L_VILWR++;}       
void LT_VISWR(){L_VISWR++;}  
void LT_VRNEXT(){L_VRNEXT++;}      
void LT_VRGET(){L_VRGET++;}       
void LT_VRINIT(){L_VRINIT++;}      
void LT_VRXOR(){L_VRXOR++;}

void (*LT_OpcodePrintTable[64])() = 
{
    LT_SPECIAL,       LT_REGIMM,       LT_J,             LT_JAL,           LT_BEQ,          LT_BNE,           LT_BLEZ,  LT_BGTZ,
    LT_ADDI,          LT_ADDIU,        LT_SLTI,          LT_SLTIU,         LT_ANDI,         LT_ORI,           LT_XORI,  LT_LUI,
    LT_COP0,          LT_COP1,         LT_COP2,          LT_UnknownOpcode, LT_BEQL,         LT_BNEL,          LT_BLEZL, LT_BGTZL,
    LT_DADDI,         LT_DADDIU,       LT_LDL,           LT_LDR,           LT_MMI,          LT_UnknownOpcode, LT_LQ,    LT_SQ,
    LT_LB,            LT_LH,           LT_LWL,           LT_LW,            LT_LBU,          LT_LHU,           LT_LWR,   LT_LWU,
    LT_SB,            LT_SH,           LT_SWL,           LT_SW,            LT_SDL,          LT_SDR,           LT_SWR,   LT_CACHE,
    LT_UnknownOpcode, LT_LWC1,         LT_UnknownOpcode, LT_PREF,          LT_UnknownOpcode,LT_UnknownOpcode, LT_LQC2,  LT_LD,
    LT_UnknownOpcode, LT_SWC1,         LT_UnknownOpcode, LT_UnknownOpcode, LT_UnknownOpcode,LT_UnknownOpcode, LT_SQC2,  LT_SD
};


void (*LT_SpecialPrintTable[64])() = 
{
    LT_SLL,           LT_UnknownOpcode, LT_SRL,           LT_SRA,           LT_SLLV,    LT_UnknownOpcode, LT_SRLV,          LT_SRAV,
    LT_JR,            LT_JALR,          LT_MOVZ,          LT_MOVN,          LT_SYSCALL, LT_BREAK,         LT_UnknownOpcode, LT_SYNC,
    LT_MFHI,          LT_MTHI,          LT_MFLO,          LT_MTLO,          LT_DSLLV,   LT_UnknownOpcode, LT_DSRLV,         LT_DSRAV,
    LT_MULT,          LT_MULTU,         LT_DIV,           LT_DIVU,          LT_UnknownOpcode,LT_UnknownOpcode,LT_UnknownOpcode,LT_UnknownOpcode,
    LT_ADD,           LT_ADDU,          LT_SUB,           LT_SUBU,          LT_AND,     LT_OR,            LT_XOR,           LT_NOR,
    LT_MFSA ,         LT_MTSA ,         LT_SLT,           LT_SLTU,          LT_DADD,    LT_DADDU,         LT_DSUB,          LT_DSUBU,
    LT_TGE,           LT_TGEU,          LT_TLT,           LT_TLTU,          LT_TEQ,     LT_UnknownOpcode, LT_TNE,           LT_UnknownOpcode,
    LT_DSLL,          LT_UnknownOpcode, LT_DSRL,          LT_DSRA,          LT_DSLL32,  LT_UnknownOpcode, LT_DSRL32,        LT_DSRA32
};

void (*LT_REGIMMPrintTable[32])() = {
    LT_BLTZ,   LT_BGEZ,   LT_BLTZL,            LT_BGEZL,         LT_UnknownOpcode, LT_UnknownOpcode, LT_UnknownOpcode, LT_UnknownOpcode,
    LT_TGEI,   LT_TGEIU,  LT_TLTI,             LT_TLTIU,         LT_TEQI,          LT_UnknownOpcode, LT_TNEI,          LT_UnknownOpcode,
    LT_BLTZAL, LT_BGEZAL, LT_BLTZALL,          LT_BGEZALL,       LT_UnknownOpcode, LT_UnknownOpcode, LT_UnknownOpcode, LT_UnknownOpcode,
    LT_MTSAB,  LT_MTSAH , LT_UnknownOpcode,    LT_UnknownOpcode, LT_UnknownOpcode, LT_UnknownOpcode, LT_UnknownOpcode, LT_UnknownOpcode,
};
void (*LT_MMIPrintTable[64])() = 
{
    LT_MADD,                    LT_MADDU,                  LT_MMI_Unknown,          LT_MMI_Unknown,          LT_PLZCW,            LT_MMI_Unknown,       LT_MMI_Unknown,          LT_MMI_Unknown,
    LT_MMI0,                    LT_MMI2,                   LT_MMI_Unknown,          LT_MMI_Unknown,          LT_MMI_Unknown,      LT_MMI_Unknown,       LT_MMI_Unknown,          LT_MMI_Unknown,
    LT_MFHI1,                   LT_MTHI1,                  LT_MFLO1,                LT_MTLO1,                LT_MMI_Unknown,      LT_MMI_Unknown,       LT_MMI_Unknown,          LT_MMI_Unknown,
    LT_MULT1,                   LT_MULTU1,                 LT_DIV1,                 LT_DIVU1,                LT_MMI_Unknown,      LT_MMI_Unknown,       LT_MMI_Unknown,          LT_MMI_Unknown,
    LT_MADD1,                   LT_MADDU1,                 LT_MMI_Unknown,          LT_MMI_Unknown,          LT_MMI_Unknown,      LT_MMI_Unknown,       LT_MMI_Unknown,          LT_MMI_Unknown,
    LT_MMI1 ,                   LT_MMI3,                   LT_MMI_Unknown,          LT_MMI_Unknown,          LT_MMI_Unknown,      LT_MMI_Unknown,       LT_MMI_Unknown,          LT_MMI_Unknown,
    LT_PMFHL,                   LT_PMTHL,                  LT_MMI_Unknown,          LT_MMI_Unknown,          LT_PSLLH,            LT_MMI_Unknown,       LT_PSRLH,                LT_PSRAH,
    LT_MMI_Unknown,             LT_MMI_Unknown,            LT_MMI_Unknown,          LT_MMI_Unknown,          LT_PSLLW,            LT_MMI_Unknown,       LT_PSRLW,                LT_PSRAW,
};

void (*LT_MMI0PrintTable[32])() = 
{ 
 LT_PADDW,         LT_PSUBW,         LT_PCGTW,          LT_PMAXW,       
 LT_PADDH,         LT_PSUBH,         LT_PCGTH,          LT_PMAXH,        
 LT_PADDB,         LT_PSUBB,         LT_PCGTB,          LT_MMI_Unknown,
 LT_MMI_Unknown,   LT_MMI_Unknown,   LT_MMI_Unknown,    LT_MMI_Unknown,
 LT_PADDSW,        LT_PSUBSW,        LT_PEXTLW,         LT_PPACW,        
 LT_PADDSH,        LT_PSUBSH,        LT_PEXTLH,         LT_PPACH,        
 LT_PADDSB,        LT_PSUBSB,        LT_PEXTLB,         LT_PPACB,        
 LT_MMI_Unknown,   LT_MMI_Unknown,   LT_PEXT5,          LT_PPAC5,        
};

void (*LT_MMI1PrintTable[32])() =
{ 
 LT_MMI_Unknown,   LT_PABSW,         LT_PCEQW,         LT_PMINW, 
 LT_PADSBH,        LT_PABSH,         LT_PCEQH,         LT_PMINH, 
 LT_MMI_Unknown,   LT_MMI_Unknown,   LT_PCEQB,         LT_MMI_Unknown, 
 LT_MMI_Unknown,   LT_MMI_Unknown,   LT_MMI_Unknown,   LT_MMI_Unknown, 
 LT_PADDUW,        LT_PSUBUW,        LT_PEXTUW,        LT_MMI_Unknown,  
 LT_PADDUH,        LT_PSUBUH,        LT_PEXTUH,        LT_MMI_Unknown, 
 LT_PADDUB,        LT_PSUBUB,        LT_PEXTUB,        LT_QFSRV, 
 LT_MMI_Unknown,   LT_MMI_Unknown,   LT_MMI_Unknown,   LT_MMI_Unknown, 
};


void (*LT_MMI2PrintTable[32])() = 
{ 
 LT_PMADDW,        LT_MMI_Unknown,   LT_PSLLVW,        LT_PSRLVW, 
 LT_PMSUBW,        LT_MMI_Unknown,   LT_MMI_Unknown,   LT_MMI_Unknown,
 LT_PMFHI,         LT_PMFLO,         LT_PINTH,         LT_MMI_Unknown,
 LT_PMULTW,        LT_PDIVW,         LT_PCPYLD,        LT_MMI_Unknown,
 LT_PMADDH,        LT_PHMADH,        LT_PAND,          LT_PXOR, 
 LT_PMSUBH,        LT_PHMSBH,        LT_MMI_Unknown,   LT_MMI_Unknown, 
 LT_MMI_Unknown,   LT_MMI_Unknown,   LT_PEXEH,         LT_PREVH, 
 LT_PMULTH,        LT_PDIVBW,        LT_PEXEW,         LT_PROT3W, 
};

void (*LT_MMI3PrintTable[32])() = 
{ 
 LT_PMADDUW,       LT_MMI_Unknown,   LT_MMI_Unknown,   LT_PSRAVW, 
 LT_MMI_Unknown,   LT_MMI_Unknown,   LT_MMI_Unknown,   LT_MMI_Unknown,
 LT_PMTHI,         LT_PMTLO,         LT_PINTEH,        LT_MMI_Unknown,
 LT_PMULTUW,       LT_PDIVUW,        LT_PCPYUD,        LT_MMI_Unknown,
 LT_MMI_Unknown,   LT_MMI_Unknown,   LT_POR,           LT_PNOR,  
 LT_MMI_Unknown,   LT_MMI_Unknown,   LT_MMI_Unknown,   LT_MMI_Unknown,
 LT_MMI_Unknown,   LT_MMI_Unknown,   LT_PEXCH,         LT_PCPYH, 
 LT_MMI_Unknown,   LT_MMI_Unknown,   LT_PEXCW,         LT_MMI_Unknown,
};

void (*LT_COP0PrintTable[32])() = 
{
    LT_MFC0,         LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_MTC0,         LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown,
    LT_COP0_BC0,     LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown,
    LT_COP0_Func,    LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown,
    LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown,
};

void (*LT_COP0BC0PrintTable[32])() = 
{
    LT_BC0F,         LT_BC0T,         LT_BC0FL,        LT_BC0TL,        LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown,
    LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown,
    LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown,
    LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown,
};

void (*LT_COP0C0PrintTable[64])() = {
    LT_COP0_Unknown, LT_TLBR,         LT_TLBWI,        LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_TLBWR,        LT_COP0_Unknown,
    LT_TLBP,         LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown,
    LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown,
    LT_ERET,         LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown,
    LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown,
    LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown,
    LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown,
    LT_EI,           LT_DI,           LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown, LT_COP0_Unknown
};


void (*LT_COP1PrintTable[32])() = {
    LT_MFC1,         LT_COP1_Unknown, LT_CFC1,         LT_COP1_Unknown, LT_MTC1,         LT_COP1_Unknown, LT_CTC1,         LT_COP1_Unknown,
    LT_COP1_BC1,     LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown,
    LT_COP1_S,       LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_W,       LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown,
    LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown,
};

void (*LT_COP1BC1PrintTable[32])() = {
    LT_BC1F,         LT_BC1T,         LT_BC1FL,        LT_BC1TL,        LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown,
    LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown,
    LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown,
    LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown, LT_COP1_Unknown,
};

void (*LT_COP1SPrintTable[64])() = {
LT_ADD_S,       LT_SUB_S,       LT_MUL_S,       LT_DIV_S,       LT_SQRT_S,      LT_ABS_S,       LT_MOV_S,       LT_NEG_S, 
LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,   
LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_RSQRT_S,     LT_COP1_Unknown,  
LT_ADDA_S,      LT_SUBA_S,      LT_MULA_S,      LT_COP1_Unknown,LT_MADD_S,      LT_MSUB_S,      LT_MADDA_S,     LT_MSUBA_S,
LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_CVT_W,       LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown, 
LT_MAX_S,       LT_MIN_S,       LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown, 
LT_C_F,         LT_COP1_Unknown,LT_C_EQ,        LT_COP1_Unknown,LT_C_LT,        LT_COP1_Unknown,LT_C_LE,        LT_COP1_Unknown, 
LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown, 
};
 
void (*LT_COP1WPrintTable[64])() = { 
LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,   	
LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,   
LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,   
LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,   
LT_CVT_S,       LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,   
LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,   
LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,   
LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,LT_COP1_Unknown,   
};

void (*LT_COP2PrintTable[32])() = {
    LT_COP2_Unknown, LT_QMFC2,        LT_CFC2,         LT_COP2_Unknown, LT_COP2_Unknown, LT_QMTC2,        LT_CTC2,         LT_COP2_Unknown,
    LT_COP2_BC2,     LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown,
    LT_COP2_SPECIAL, LT_COP2_SPECIAL, LT_COP2_SPECIAL, LT_COP2_SPECIAL, LT_COP2_SPECIAL, LT_COP2_SPECIAL, LT_COP2_SPECIAL, LT_COP2_SPECIAL,
	LT_COP2_SPECIAL, LT_COP2_SPECIAL, LT_COP2_SPECIAL, LT_COP2_SPECIAL, LT_COP2_SPECIAL, LT_COP2_SPECIAL, LT_COP2_SPECIAL, LT_COP2_SPECIAL,
};

void (*LT_COP2BC2PrintTable[32])() = {
    LT_BC2F,         LT_BC2T,         LT_BC2FL,        LT_BC2TL,        LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown,
    LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown,
    LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown,
    LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown, LT_COP2_Unknown,
}; 

void (*LT_COP2SPECIAL1PrintTable[64])() = 
{ 
 LT_VADDx,       LT_VADDy,       LT_VADDz,       LT_VADDw,       LT_VSUBx,        LT_VSUBy,        LT_VSUBz,        LT_VSUBw,  
 LT_VMADDx,      LT_VMADDy,      LT_VMADDz,      LT_VMADDw,      LT_VMSUBx,       LT_VMSUBy,       LT_VMSUBz,       LT_VMSUBw, 
 LT_VMAXx,       LT_VMAXy,       LT_VMAXz,       LT_VMAXw,       LT_VMINIx,       LT_VMINIy,       LT_VMINIz,       LT_VMINIw, 
 LT_VMULx,       LT_VMULy,       LT_VMULz,       LT_VMULw,       LT_VMULq,        LT_VMAXi,        LT_VMULi,        LT_VMINIi,
 LT_VADDq,       LT_VMADDq,      LT_VADDi,       LT_VMADDi,      LT_VSUBq,        LT_VMSUBq,       LT_VSUBi,        LT_VMSUBi, 
 LT_VADD,        LT_VMADD,       LT_VMUL,        LT_VMAX,        LT_VSUB,         LT_VMSUB,        LT_VOPMSUB,      LT_VMINI,  
 LT_VIADD,       LT_VISUB,       LT_VIADDI,      LT_COP2_Unknown,LT_VIAND,        LT_VIOR,         LT_COP2_Unknown, LT_COP2_Unknown,
 LT_VCALLMS,     LT_VCALLMSR,    LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_SPECIAL2,LT_COP2_SPECIAL2,LT_COP2_SPECIAL2,LT_COP2_SPECIAL2,  
};

void (*LT_COP2SPECIAL2PrintTable[128])() = 
{
 LT_VADDAx      ,LT_VADDAy      ,LT_VADDAz      ,LT_VADDAw      ,LT_VSUBAx      ,LT_VSUBAy      ,LT_VSUBAz      ,LT_VSUBAw,
 LT_VMADDAx     ,LT_VMADDAy     ,LT_VMADDAz     ,LT_VMADDAw     ,LT_VMSUBAx     ,LT_VMSUBAy     ,LT_VMSUBAz     ,LT_VMSUBAw,
 LT_VITOF0      ,LT_VITOF4      ,LT_VITOF12     ,LT_VITOF15     ,LT_VFTOI0      ,LT_VFTOI4      ,LT_VFTOI12     ,LT_VFTOI15,
 LT_VMULAx      ,LT_VMULAy      ,LT_VMULAz      ,LT_VMULAw      ,LT_VMULAq      ,LT_VABS        ,LT_VMULAi      ,LT_VCLIPw,
 LT_VADDAq      ,LT_VMADDAq     ,LT_VADDAi      ,LT_VMADDAi     ,LT_VSUBAq      ,LT_VMSUBAq     ,LT_VSUBAi      ,LT_VMSUBAi,
 LT_VADDA       ,LT_VMADDA      ,LT_VMULA       ,LT_COP2_Unknown,LT_VSUBA       ,LT_VMSUBA      ,LT_VOPMULA     ,LT_VNOP,   
 LT_VMOVE       ,LT_VMR32       ,LT_COP2_Unknown,LT_COP2_Unknown,LT_VLQI        ,LT_VSQI        ,LT_VLQD        ,LT_VSQD,   
 LT_VDIV        ,LT_VSQRT       ,LT_VRSQRT      ,LT_VWAITQ      ,LT_VMTIR       ,LT_VMFIR       ,LT_VILWR       ,LT_VISWR,  
 LT_VRNEXT      ,LT_VRGET       ,LT_VRINIT      ,LT_VRXOR       ,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown, 
 LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,
 LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,
 LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,
 LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,
 LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,
 LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,
 LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,LT_COP2_Unknown,
};
