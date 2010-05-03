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


#include "PrecompiledHeader.h"

#ifdef __LINUX__
#include <cstdarg>
#endif

#include "Debug.h"
#include "R5900.h"
#include "DisASM.h"
#include "R5900OpcodeTables.h"

unsigned long opcode_addr;

using namespace std;

namespace R5900
{

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
static const char * const GPR_REG[32] = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};
static const char * const COP0_REG[32] ={
	"Index","Random","EntryLo0","EntryLo1","Context","PageMask",
	"Wired","C0r7","BadVaddr","Count","EntryHi","Compare","Status",
	"Cause","EPC","PRId","Config","C0r17","C0r18","C0r19","C0r20",
	"C0r21","C0r22","C0r23","Debug","Perf","C0r26","C0r27","TagLo",
	"TagHi","ErrorPC","C0r31"
};
//floating point cop1 Floating point reg
static const char * const COP1_REG_FP[32] ={
 	"f00","f01","f02","f03","f04","f05","f06","f07",
	"f08","f09","f10","f11","f12","f13","f14","f15",
	"f16","f17","f18","f19","f20","f21","f21","f23",
	"f24","f25","f26","f27","f28","f29","f30","f31"
};
//floating point cop1 control registers
static const char * const COP1_REG_FCR[32] ={
 	"fcr00","fcr01","fcr02","fcr03","fcr04","fcr05","fcr06","fcr07",
	"fcr08","fcr09","fcr10","fcr11","fcr12","fcr13","fcr14","fcr15",
	"fcr16","fcr17","fcr18","fcr19","fcr20","fcr21","fcr21","fcr23",
	"fcr24","fcr25","fcr26","fcr27","fcr28","fcr29","fcr30","fcr31"
};

//floating point cop2 reg
static const char * const COP2_REG_FP[32] ={
	"vf00","vf01","vf02","vf03","vf04","vf05","vf06","vf07",
	"vf08","vf09","vf10","vf11","vf12","vf13","vf14","vf15",
	"vf16","vf17","vf18","vf19","vf20","vf21","vf21","vf23",
	"vf24","vf25","vf26","vf27","vf28","vf29","vf30","vf31"
};
//cop2 control registers

static const char * const COP2_REG_CTL[32] ={
	"vi00","vi01","vi02","vi03","vi04","vi05","vi06","vi07",
	"vi08","vi09","vi10","vi11","vi12","vi13","vi14","vi15",
	"Status","MACflag","ClipFlag","c2c19","R","I","Q","c2c23",
	"c2c24","c2c25","TPC","CMSAR0","FBRST","VPU-STAT","c2c30","CMSAR1"
};

void P_COP2_Unknown( string& output );
void P_COP2_SPECIAL2( string& output );
void P_COP2_SPECIAL( string& output );
void P_COP2_BC2( string& output );

//****************************************************************************
//** COP2 - (VU0)                                                           **
//****************************************************************************
void P_QMFC2( string& output );
void P_CFC2( string& output );
void P_QMTC2( string& output );
void P_CTC2( string& output );
void P_BC2F( string& output );
void P_BC2T( string& output );
void P_BC2FL( string& output );
void P_BC2TL( string& output );
//*****************SPECIAL 1 VUO TABLE*******************************
void P_VADDx( string& output );
void P_VADDy( string& output );
void P_VADDz( string& output );
void P_VADDw( string& output );
void P_VSUBx( string& output );
void P_VSUBy( string& output );
void P_VSUBz( string& output );
void P_VSUBw( string& output );
void P_VMADDx( string& output );
void P_VMADDy( string& output );
void P_VMADDz( string& output );
void P_VMADDw( string& output );
void P_VMSUBx( string& output );
void P_VMSUBy( string& output );
void P_VMSUBz( string& output );
void P_VMSUBw( string& output );
void P_VMAXx( string& output );
void P_VMAXy( string& output );
void P_VMAXz( string& output );
void P_VMAXw( string& output );
void P_VMINIx( string& output );
void P_VMINIy( string& output );
void P_VMINIz( string& output );
void P_VMINIw( string& output );
void P_VMULx( string& output );
void P_VMULy( string& output );
void P_VMULz( string& output );
void P_VMULw( string& output );
void P_VMULq( string& output );
void P_VMAXi( string& output );
void P_VMULi( string& output );
void P_VMINIi( string& output );
void P_VADDq( string& output );
void P_VMADDq( string& output );
void P_VADDi( string& output );
void P_VMADDi( string& output );
void P_VSUBq( string& output );
void P_VMSUBq( string& output );
void P_VSUbi( string& output );
void P_VMSUBi( string& output );
void P_VADD( string& output );
void P_VMADD( string& output );
void P_VMUL( string& output );
void P_VMAX( string& output );
void P_VSUB( string& output );
void P_VMSUB( string& output );
void P_VOPMSUB( string& output );
void P_VMINI( string& output );
void P_VIADD( string& output );
void P_VISUB( string& output );
void P_VIADDI( string& output );
void P_VIAND( string& output );
void P_VIOR( string& output );
void P_VCALLMS( string& output );
void P_CALLMSR( string& output );
//***********************************END OF SPECIAL1 VU0 TABLE*****************************
//******************************SPECIAL2 VUO TABLE*****************************************
void P_VADDAx( string& output );
void P_VADDAy( string& output );
void P_VADDAz( string& output );
void P_VADDAw( string& output );
void P_VSUBAx( string& output );
void P_VSUBAy( string& output );
void P_VSUBAz( string& output );
void P_VSUBAw( string& output );
void P_VMADDAx( string& output );
void P_VMADDAy( string& output );
void P_VMADDAz( string& output );
void P_VMADDAw( string& output );
void P_VMSUBAx( string& output );
void P_VMSUBAy( string& output );
void P_VMSUBAz( string& output );
void P_VMSUBAw( string& output );
void P_VITOF0( string& output );
void P_VITOF4( string& output );
void P_VITOF12( string& output );
void P_VITOF15( string& output );
void P_VFTOI0( string& output );
void P_VFTOI4( string& output );
void P_VFTOI12( string& output );
void P_VFTOI15( string& output );
void P_VMULAx( string& output );
void P_VMULAy( string& output );
void P_VMULAz( string& output );
void P_VMULAw( string& output );
void P_VMULAq( string& output );
void P_VABS( string& output );
void P_VMULAi( string& output );
void P_VCLIPw( string& output );
void P_VADDAq( string& output );
void P_VMADDAq( string& output );
void P_VADDAi( string& output );
void P_VMADDAi( string& output );
void P_VSUBAq( string& output );
void P_VMSUBAq( string& output );
void P_VSUBAi( string& output );
void P_VMSUBAi( string& output );
void P_VADDA( string& output );
void P_VMADDA( string& output );
void P_VMULA( string& output );
void P_VSUBA( string& output );
void P_VMSUBA( string& output );
void P_VOPMULA( string& output );
void P_VNOP( string& output );
void P_VMONE( string& output );
void P_VMR32( string& output );
void P_VLQI( string& output );
void P_VSQI( string& output );
void P_VLQD( string& output );
void P_VSQD( string& output );
void P_VDIV( string& output );
void P_VSQRT( string& output );
void P_VRSQRT( string& output );
void P_VWAITQ( string& output );
void P_VMTIR( string& output );
void P_VMFIR( string& output );
void P_VILWR( string& output );
void P_VISWR( string& output );
void P_VRNEXT( string& output );
void P_VRGET( string& output );
void P_VRINIT( string& output );
void P_VRXOR( string& output );
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

//*************************************************************
// COP2 TABLES :)   [VU0 as a Co-Processor to the EE]
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
void (*COP2PrintTable[32])( string& output ) = {
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
void (*COP2BC2PrintTable[32])( string& output ) = {
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
void (*COP2SPECIAL1PrintTable[64])( string& output ) =
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
void (*COP2SPECIAL2PrintTable[128])( string& output ) =
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


void disR5900Fasm( string& output, u32 code, u32 pc )
{
	string dbuf;
	char obuf[48];

	const u32 scode = cpuRegs.code;
	opcode_addr = pc;
	cpuRegs.code = code;

	sprintf(obuf, "%08X:\t", pc );
	output.assign( obuf );
	GetCurrentInstruction().disasm( output );

	cpuRegs.code = scode;
}

//*************************************************************
//************************COP2**********************************
void P_COP2_BC2( string& output )
{
	COP2BC2PrintTable[DECODE_C2BC]( output );
}
void P_COP2_SPECIAL( string& output )
{
	COP2SPECIAL1PrintTable[DECODE_FUNCTION ]( output );
}
void P_COP2_SPECIAL2( string& output )
{
	COP2SPECIAL2PrintTable[(cpuRegs.code & 0x3) | ((cpuRegs.code >> 4) & 0x7c)]( output );
}

//**************************UNKNOWN****************************
void P_COP2_Unknown( string& output )
{
	output += "COP2 ??";
}


//*************************************************************

//*****************SOME DECODE STUFF***************************

void label_decode( string& output, u32 addr )
{
	string buf;
	ssprintf(buf, "0x%08X", addr);
	const char* label = disR5900GetSym( addr );

	if( label != NULL )
	{
		output += label;
		output += ' ';
	}

	output += buf;
}

void jump_decode( string& output )
{
    label_decode( output, DECODE_JUMP );
}

void offset_decode( string& output )
{
	label_decode( output, DECODE_OFFSET );
}

//*********************END OF DECODE ROUTINES******************

namespace OpcodeDisasm
{

void COP2( string& output )
{
	COP2PrintTable[DECODE_RS]( output );
}

// Unkown Opcode!
void Unknown( string& output )
{
	output += "?????";
}

void MMI_Unknown( string& output )
{
	output += "MMI ??";
}

void COP0_Unknown( string& output )
{
	output += "COP0 ??";
}

void COP1_Unknown( string& output )
{
	output += "FPU ??";
}

// sap!  it stands for string append.  It's not a friendly name but for now it makes
// the copy-paste marathon of code below more readable!
#define _sap( str ) ssappendf( output, str,

//********************* Standard Opcodes***********************
void J( string& output )      { output += "j\t";        jump_decode(output);}
void JAL( string& output )    { output += "jal\t";      jump_decode(output);}
void BEQ( string& output )    { _sap("beq\t%s, %s, ")          GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); offset_decode(output); }
void BNE( string& output )    { _sap("bne\t%s, %s, ")          GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); offset_decode(output); }
void BLEZ( string& output )   { _sap("blez\t%s, ")             GPR_REG[DECODE_RS]); offset_decode(output); }
void BGTZ( string& output )   { _sap("bgtz\t%s, ")             GPR_REG[DECODE_RS]); offset_decode(output); }
void ADDI( string& output )   { _sap("addi\t%s, %s, 0x%04X")   GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED);}
void ADDIU( string& output )  { _sap("addiu\t%s, %s, 0x%04X")  GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED);}
void SLTI( string& output )   { _sap("slti\t%s, %s, 0x%04X")   GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED); }
void SLTIU( string& output )  { _sap("sltiu\t%s, %s, 0x%04X")  GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED); }
void ANDI( string& output )   { _sap("andi\t%s, %s, 0x%04X")   GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED);}
void ORI( string& output )    { _sap("ori\t%s, %s, 0x%04X")    GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED); }
void XORI( string& output )   { _sap("xori\t%s, %s, 0x%04X")   GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED); }
void LUI( string& output )    { _sap("lui\t%s, 0x%04X")        GPR_REG[DECODE_RT], DECODE_IMMED); }
void BEQL( string& output )   { _sap("beql\t%s, %s, ")       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); offset_decode(output); }
void BNEL( string& output )   { _sap("bnel\t%s, %s, ")       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); offset_decode(output); }
void BLEZL( string& output )  { _sap("blezl\t%s, ")            GPR_REG[DECODE_RS]); offset_decode(output); }
void BGTZL( string& output )  { _sap("bgtzl\t%s, ")            GPR_REG[DECODE_RS]); offset_decode(output); }
void DADDI( string& output )  { _sap("daddi\t%s, %s, 0x%04X")  GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED); }
void DADDIU( string& output ) { _sap("daddiu\t%s, %s, 0x%04X") GPR_REG[DECODE_RT], GPR_REG[DECODE_RS], DECODE_IMMED); }
void LDL( string& output )    { _sap("ldl\t%s, 0x%04X(%s)")    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void LDR( string& output )    { _sap("ldr\t%s, 0x%04X(%s)")    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void LB( string& output )     { _sap("lb\t%s, 0x%04X(%s)")     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void LH( string& output )     { _sap("lh\t%s, 0x%04X(%s)")     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void LWL( string& output )    { _sap("lwl\t%s, 0x%04X(%s)")    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void LW( string& output )     { _sap("lw\t%s, 0x%04X(%s)")     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void LBU( string& output )    { _sap("lbu\t%s, 0x%04X(%s)")    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void LHU( string& output )    { _sap("lhu\t%s, 0x%04X(%s)")    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void LWR( string& output )    { _sap("lwr\t%s, 0x%04X(%s)")    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void LWU( string& output )    { _sap("lwu\t%s, 0x%04X(%s)")    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void SB( string& output )     { _sap("sb\t%s, 0x%04X(%s)")     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void SH( string& output )     { _sap("sh\t%s, 0x%04X(%s)")     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void SWL( string& output )    { _sap("swl\t%s, 0x%04X(%s)")    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void SW( string& output )     { _sap("sw\t%s, 0x%04X(%s)")     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void SDL( string& output )    { _sap("sdl\t%s, 0x%04X(%s)")    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void SDR( string& output )    { _sap("sdr\t%s, 0x%04X(%s)")    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void SWR( string& output )    { _sap("swr\t%s, 0x%04X(%s)")    GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void LD( string& output )     { _sap("ld\t%s, 0x%04X(%s)")     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void SD( string& output )     { _sap("sd\t%s, 0x%04X(%s)")     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void LQ( string& output )     { _sap("lq\t%s, 0x%04X(%s)")     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void SQ( string& output )     { _sap("sq\t%s, 0x%04X(%s)")     GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void SWC1( string& output )   { _sap("swc1\t%s, 0x%04X(%s)")   COP1_REG_FP[DECODE_FT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void SQC2( string& output )   { _sap("sqc2\t%s, 0x%04X(%s)")   COP2_REG_FP[DECODE_FT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void PREF( string& output )   { output += "pref ---"; /*_sap("PREF\t%s, 0x%04X(%s)")   GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[RS]); */}
void LWC1( string& output )   { _sap("lwc1\t%s, 0x%04X(%s)")   COP1_REG_FP[DECODE_FT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
void LQC2( string& output )   { _sap("lqc2\t%s, 0x%04X(%s)")   COP2_REG_FP[DECODE_FT], DECODE_IMMED, GPR_REG[DECODE_RS]); }
//********************END OF STANDARD OPCODES*************************

void SLL( string& output )
{
   if (cpuRegs.code == 0x00000000)
        output += "nop";
    else
        _sap("sll\t%s, %s, 0x%02X") GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA);
}

void SRL( string& output )    { _sap("srl\t%s, %s, 0x%02X") GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void SRA( string& output )    { _sap("sra\t%s, %s, 0x%02X") GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void SLLV( string& output )   { _sap("sllv\t%s, %s, %s")    GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], GPR_REG[DECODE_RS]); }
void SRLV( string& output )   { _sap("srlv\t%s, %s, %s")    GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], GPR_REG[DECODE_RS]);}
void SRAV( string& output )   { _sap("srav\t%s, %s, %s")    GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], GPR_REG[DECODE_RS]); }
void JR( string& output )     { _sap("jr\t%s")              GPR_REG[DECODE_RS]); }

void JALR( string& output )
{
    int rd = DECODE_RD;

    if (rd == 31)
        _sap("jalr\t%s") GPR_REG[DECODE_RS]);
    else
        _sap("jalr\t%s, %s") GPR_REG[rd], GPR_REG[DECODE_RS]);
}


void SYNC( string& output )    { output += "SYNC"; }
void MFHI( string& output )    { _sap("mfhi\t%s")          GPR_REG[DECODE_RD]); }
void MTHI( string& output )    { _sap("mthi\t%s")          GPR_REG[DECODE_RS]); }
void MFLO( string& output )    { _sap("mflo\t%s")          GPR_REG[DECODE_RD]); }
void MTLO( string& output )    { _sap("mtlo\t%s")          GPR_REG[DECODE_RS]); }
void DSLLV( string& output )   { _sap("dsllv\t%s, %s, %s") GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], GPR_REG[DECODE_RS]); }
void DSRLV( string& output )   { _sap("dsrlv\t%s, %s, %s") GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], GPR_REG[DECODE_RS]); }
void DSRAV( string& output )   { _sap("dsrav\t%s, %s, %s") GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], GPR_REG[DECODE_RS]); }
void MULT( string& output )    { _sap("mult\t%s, %s, %s")  GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]);}
void MULTU( string& output )   { _sap("multu\t%s, %s, %s") GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]);}
void DIV( string& output )     { _sap("div\t%s, %s")       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void DIVU( string& output )    { _sap("divu\t%s, %s")      GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void ADD( string& output )     { _sap("add\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void ADDU( string& output )    { _sap("addu\t%s, %s, %s")  GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void SUB( string& output )     { _sap("sub\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void SUBU( string& output )    { _sap("subu\t%s, %s, %s")  GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void AND( string& output )     { _sap("and\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void OR( string& output )      { _sap("or\t%s, %s, %s")    GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void XOR( string& output )     { _sap("xor\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void NOR( string& output )     { _sap("nor\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void SLT( string& output )     { _sap("slt\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void SLTU( string& output )    { _sap("sltu\t%s, %s, %s")  GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void DADD( string& output )    { _sap("dadd\t%s, %s, %s")  GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void DADDU( string& output )   { _sap("daddu\t%s, %s, %s") GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void DSUB( string& output )    { _sap("dsub\t%s, %s, %s")  GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void DSUBU( string& output )   { _sap("dsubu\t%s, %s, %s") GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void TGE( string& output )     { _sap("tge\t%s, %s")       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void TGEU( string& output )    { _sap("tgeu\t%s, %s")      GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void TLT( string& output )     { _sap("tlt\t%s, %s")       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void TLTU( string& output )    { _sap("tltu\t%s, %s")      GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void TEQ( string& output )     { _sap("teq\t%s, %s")       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void TNE( string& output )     { _sap("tne\t%s, %s")       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void DSLL( string& output )    { _sap("dsll\t%s, %s, 0x%02X")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void DSRL( string& output )    { _sap("dsrl\t%s, %s, 0x%02X")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void DSRA( string& output )    { _sap("dsra\t%s, %s, 0x%02X")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void DSLL32( string& output )  { _sap("dsll32\t%s, %s, 0x%02X") GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void DSRL32( string& output )  { _sap("dsrl32\t%s, %s, 0x%02X") GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void DSRA32( string& output )  { _sap("dsra32\t%s, %s, 0x%02X") GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void MOVZ( string& output )    { _sap("movz\t%s, %s, %s") GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void MOVN( string& output )    { _sap("movn\t%s, %s, %s") GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void MFSA( string& output )    { _sap("mfsa\t%s")          GPR_REG[DECODE_RD]);}
void MTSA( string& output )    { _sap("mtsa\t%s")          GPR_REG[DECODE_RS]);}
//*** unsupport (yet) cpu opcodes
void SYSCALL( string& output ) { output +="syscall ---";/*_sap("syscall\t0x%05X")   DECODE_SYSCALL);*/}
void BREAK( string& output )   { output += "break   ---";/*_sap("break\t0x%05X")     DECODE_BREAK); */}
void CACHE( string& output )   { output += "cache   ---";/*_sap("cache\t%s, 0x%04X(%s)")  GPR_REG[DECODE_RT], DECODE_IMMED, GPR_REG[DECODE_RS]); */}
//************************REGIMM OPCODES***************************
void BLTZ( string& output )    { _sap("bltz\t%s, ")       GPR_REG[DECODE_RS]); offset_decode(output); }
void BGEZ( string& output )    { _sap("bgez\t%s, ")       GPR_REG[DECODE_RS]); offset_decode(output); }
void BLTZL( string& output )   { _sap("bltzl\t%s, ")      GPR_REG[DECODE_RS]); offset_decode(output); }
void BGEZL( string& output )   { _sap("bgezl\t%s, ")      GPR_REG[DECODE_RS]); offset_decode(output); }
void TGEI( string& output )    { _sap("tgei\t%s, 0x%04X") GPR_REG[DECODE_RS], DECODE_IMMED); }
void TGEIU( string& output )   { _sap("tgeiu\t%s,0x%04X") GPR_REG[DECODE_RS], DECODE_IMMED); }
void TLTI( string& output )    { _sap("tlti\t%s, 0x%04X") GPR_REG[DECODE_RS], DECODE_IMMED); }
void TLTIU( string& output )   { _sap("tltiu\t%s,0x%04X") GPR_REG[DECODE_RS], DECODE_IMMED); }
void TEQI( string& output )    { _sap("teqi\t%s, 0x%04X") GPR_REG[DECODE_RS], DECODE_IMMED); }
void TNEI( string& output )    { _sap("tnei\t%s, 0x%04X") GPR_REG[DECODE_RS], DECODE_IMMED); }
void BLTZAL( string& output )  { _sap("bltzal\t%s, ")     GPR_REG[DECODE_RS]); offset_decode(output); }
void BGEZAL( string& output )  { _sap("bgezal\t%s, ")     GPR_REG[DECODE_RS]); offset_decode(output); }
void BLTZALL( string& output ) { _sap("bltzall\t%s, ")    GPR_REG[DECODE_RS]); offset_decode(output); }
void BGEZALL( string& output ) { _sap("bgezall\t%s, ")    GPR_REG[DECODE_RS]); offset_decode(output); }
void MTSAB( string& output )   { _sap("mtsab\t%s, 0x%04X") GPR_REG[DECODE_RS], DECODE_IMMED);}
void MTSAH( string& output )   { _sap("mtsah\t%s, 0x%04X") GPR_REG[DECODE_RS], DECODE_IMMED);}


//***************************SPECIAL 2 CPU OPCODES*******************
const char* pmfhl_sub[] = { "lw", "uw", "slw", "lh", "sh" };

void MADD( string& output )    { _sap("madd\t%s, %s %s")        GPR_REG[DECODE_RD],GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void MADDU( string& output )   { _sap("maddu\t%s, %s %s")       GPR_REG[DECODE_RD],GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]);}
void PLZCW( string& output )   { _sap("plzcw\t%s, %s")          GPR_REG[DECODE_RD], GPR_REG[DECODE_RS]); }
void MADD1( string& output )   { _sap("madd1\t%s, %s %s")       GPR_REG[DECODE_RD],GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void MADDU1( string& output )  { _sap("maddu1\t%s, %s %s")      GPR_REG[DECODE_RD],GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void MFHI1( string& output )   { _sap("mfhi1\t%s")          GPR_REG[DECODE_RD]); }
void MTHI1( string& output )   { _sap("mthi1\t%s")          GPR_REG[DECODE_RS]); }
void MFLO1( string& output )   { _sap("mflo1\t%s")          GPR_REG[DECODE_RD]); }
void MTLO1( string& output )   { _sap("mtlo1\t%s")          GPR_REG[DECODE_RS]); }
void MULT1( string& output )   { _sap("mult1\t%s, %s, %s")        GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void MULTU1( string& output )  { _sap("multu1\t%s, %s, %s")        GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]);}
void DIV1( string& output )    { _sap("div1\t%s, %s")       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void DIVU1( string& output )   { _sap("divu1\t%s, %s")       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
//that have parametres that i haven't figure out how to display...
void PMFHL( string& output )   { _sap("pmfhl.%s \t%s")          pmfhl_sub[DECODE_SA], GPR_REG[DECODE_RD]); }
void PMTHL( string& output )   { _sap("pmthl.%s \t%s")          pmfhl_sub[DECODE_SA], GPR_REG[DECODE_RS]); }
void PSLLH( string& output )   { _sap("psllh   \t%s, %s, 0x%02X")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA); }
void PSRLH( string& output )   { _sap("psrlh   \t%s, %s, 0x%02X")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA);}
void PSRAH( string& output )   { _sap("psrah   \t%s, %s, 0x%02X")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA);}
void PSLLW( string& output )   { _sap( "psllw   \t%s, %s, 0x%02X")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA);}
void PSRLW( string& output )   { _sap( "psrlw   \t%s, %s, 0x%02X")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA);}
void PSRAW( string& output )   { _sap( "psraw   \t%s, %s, 0x%02X")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], DECODE_SA);}
//***************************END OF SPECIAL OPCODES******************
//*************************MMI0 OPCODES************************

void PADDW( string& output ){  _sap( "paddw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PSUBW( string& output ){  _sap( "psubw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PCGTW( string& output ){  _sap( "pcgtw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PMAXW( string& output ){  _sap( "pmaxw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PADDH( string& output ){  _sap( "paddh\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PSUBH( string& output ){  _sap( "psubh\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PCGTH( string& output ){  _sap( "pcgth\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PMAXH( string& output ){  _sap( "pmaxh\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PADDB( string& output ){  _sap( "paddb\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PSUBB( string& output ){  _sap( "psubb\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PCGTB( string& output ){  _sap( "pcgtb\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PADDSW( string& output ){ _sap( "paddsw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PSUBSW( string& output ){ _sap( "psubsw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PEXTLW( string& output ){ _sap( "pextlw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PPACW( string& output ) { _sap( "ppacw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PADDSH( string& output ){ _sap( "paddsh\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PSUBSH( string& output ){ _sap( "psubsh\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PEXTLH( string& output ){ _sap( "pextlh\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PPACH( string& output ) { _sap( "ppach\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PADDSB( string& output ){ _sap( "paddsb\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PSUBSB( string& output ){ _sap( "psubsb\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PEXTLB( string& output ){ _sap( "pextlb\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PPACB( string& output ) { _sap( "ppacb\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PEXT5( string& output ) { _sap( "pext5\t%s, %s")      GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]); }
void PPAC5( string& output ) { _sap( "ppac5\t%s, %s")      GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]); }
//**********END OF MMI0 OPCODES*********************************
//**********MMI1 OPCODES**************************************
void PABSW( string& output ){  _sap( "pabsw%s, %s")      GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]); }
void PCEQW( string& output ){  _sap( "pceqw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PMINW( string& output ){  _sap( "pminw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PADSBH( string& output ){ _sap( "padsbh\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PABSH( string& output ){  _sap( "pabsh%s, %s")      GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]); }
void PCEQH( string& output ){  _sap( "pceqh\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PMINH( string& output ){  _sap( "pminh\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PCEQB( string& output ){  _sap( "pceqb\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PADDUW( string& output ){ _sap( "padduw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PSUBUW( string& output ){ _sap( "psubuw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PEXTUW( string& output ){ _sap( "pextuw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PADDUH( string& output ){ _sap( "padduh\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PSUBUH( string& output ){ _sap( "psubuh\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PEXTUH( string& output ){ _sap( "pextuh\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PADDUB( string& output ){ _sap( "paddub\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PSUBUB( string& output ){ _sap( "psubub\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PEXTUB( string& output ){ _sap( "pextub\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void QFSRV( string& output ) { _sap( "qfsrv\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
//********END OF MMI1 OPCODES***********************************
//*********MMI2 OPCODES***************************************
void PMADDW( string& output ){ _sap( "pmaddw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PSLLVW( string& output ){ _sap( "psllvw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PSRLVW( string& output ){ _sap( "psrlvw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PMSUBW( string& output ){ _sap( "msubw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PMFHI( string& output ){  _sap( "pmfhi\t%s")          GPR_REG[DECODE_RD]); }
void PMFLO( string& output ){  _sap( "pmflo\t%s")          GPR_REG[DECODE_RD]); }
void PINTH( string& output ){  _sap( "pinth\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PMULTW( string& output ){ _sap( "pmultw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PDIVW( string& output ){  _sap( "pdivw\t%s, %s")      GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PCPYLD( string& output ){ _sap( "pcpyld\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PMADDH( string& output ){ _sap( "pmaddh\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PHMADH( string& output ){ _sap( "phmadh\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PAND( string& output ){   _sap( "pand\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PXOR( string& output ){   _sap( "pxor\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PMSUBH( string& output ){ _sap( "pmsubh\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PHMSBH( string& output ){ _sap( "phmsbh\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PEXEH( string& output ){  _sap( "pexeh\t%s, %s")      GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]); }
void PREVH( string& output ){  _sap( "prevh\t%s, %s")      GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]); }
void PMULTH( string& output ){ _sap( "pmulth\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PDIVBW( string& output ){ _sap( "pdivbw\t%s, %s")      GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PEXEW( string& output ){  _sap( "pexew\t%s, %s")      GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]); }
void PROT3W( string& output ){ _sap( "prot3w\t%s, %s")      GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]); }
//*****END OF MMI2 OPCODES***********************************
//*************************MMI3 OPCODES************************
void PMADDUW( string& output ){ _sap("pmadduw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], GPR_REG[DECODE_RS]); }
void PSRAVW( string& output ){  _sap("psravw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RT], GPR_REG[DECODE_RS]); }
void PMTHI( string& output ){   _sap("pmthi\t%s")          GPR_REG[DECODE_RS]); }
void PMTLO( string& output ){   _sap("pmtlo\t%s")          GPR_REG[DECODE_RS]); }
void PINTEH( string& output ){  _sap("pinteh\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PMULTUW( string& output ){ _sap("pmultuw\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PDIVUW( string& output ){  _sap("pdivuw\t%s, %s")       GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PCPYUD( string& output ){  _sap("pcpyud\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void POR( string& output ){     _sap("por\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PNOR( string& output ){    _sap("pnor\t%s, %s, %s")   GPR_REG[DECODE_RD], GPR_REG[DECODE_RS], GPR_REG[DECODE_RT]); }
void PEXCH( string& output ){   _sap("pexch\t%s, %s")       GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]);}
void PCPYH( string& output ){   _sap("pcpyh\t%s, %s")       GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]);}
void PEXCW( string& output ){   _sap("pexcw\t%s, %s")       GPR_REG[DECODE_RD], GPR_REG[DECODE_RT]);}
//**********************END OF MMI3 OPCODES********************

//****************************************************************************
//** COP0                                                                   **
//****************************************************************************
void MFC0( string& output ){  _sap("mfc0\t%s, %s")  GPR_REG[DECODE_RT], COP0_REG[DECODE_FS]); }
void MTC0( string& output ){  _sap("mtc0\t%s, %s")  GPR_REG[DECODE_RT], COP0_REG[DECODE_FS]); }
void BC0F( string& output ){  output += "bc0f\t";       offset_decode(output); }
void BC0T( string& output ){  output += "bc0t\t";       offset_decode(output); }
void BC0FL( string& output ){ output += "bc0fl\t";      offset_decode(output); }
void BC0TL( string& output ){ output += "bc0tl\t";      offset_decode(output); }
void TLBR( string& output ){  output += "tlbr";}
void TLBWI( string& output ){ output += "tlbwi";}
void TLBWR( string& output ){ output += "tlbwr";}
void TLBP( string& output ){  output += "tlbp";}
void ERET( string& output ){  output += "eret";}
void DI( string& output ){    output += "di";}
void EI( string& output ){    output += "ei";}
//****************************************************************************
//** END OF COP0                                                            **
//****************************************************************************
//****************************************************************************
//** COP1 - Floating Point Unit (FPU)                                       **
//****************************************************************************
void MFC1( string& output ){   _sap("mfc1\t%s, %s")      GPR_REG[DECODE_RT], COP1_REG_FP[DECODE_FS]);  }
void CFC1( string& output ){   _sap("cfc1\t%s, %s")      GPR_REG[DECODE_RT], COP1_REG_FCR[DECODE_FS]); }
void MTC1( string& output ){   _sap("mtc1\t%s, %s")      GPR_REG[DECODE_RT], COP1_REG_FP[DECODE_FS]);  }
void CTC1( string& output ){   _sap("ctc1\t%s, %s")      GPR_REG[DECODE_RT], COP1_REG_FCR[DECODE_FS]); }
void BC1F( string& output ){   output += "bc1f\t";      offset_decode(output); }
void BC1T( string& output ){   output += "bc1t\t";      offset_decode(output); }
void BC1FL( string& output ){  output += "bc1fl\t";     offset_decode(output); }
void BC1TL( string& output ){  output += "bc1tl\t";     offset_decode(output); }
void ADD_S( string& output ){  _sap("add.s\t%s, %s, %s") COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]);}
void SUB_S( string& output ){  _sap("sub.s\t%s, %s, %s") COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]);}
void MUL_S( string& output ){  _sap("mul.s\t%s, %s, %s") COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]);}
void DIV_S( string& output ){  _sap("div.s\t%s, %s, %s") COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }
void SQRT_S( string& output ){ _sap("sqrt.s\t%s, %s")   COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FT]); }
void ABS_S( string& output ){  _sap("abs.s\t%s, %s")     COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS]); }
void MOV_S( string& output ){  _sap("mov.s\t%s, %s")     COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS]); }
void NEG_S( string& output ){  _sap("neg.s\t%s, %s")     COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS]);}
void RSQRT_S( string& output ){_sap("rsqrt.s\t%s, %s, %s") COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]);}
void ADDA_S( string& output ){ _sap("adda.s\t%s, %s")     COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }
void SUBA_S( string& output ){ _sap("suba.s\t%s, %s")     COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }
void MULA_S( string& output ){ _sap("mula.s\t%s, %s")   COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }
void MADD_S( string& output ){ _sap("madd.s\t%s, %s, %s") COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }
void MSUB_S( string& output ){ _sap("msub.s\t%s, %s, %s") COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }
void MADDA_S( string& output ){_sap("madda.s\t%s, %s")   COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }
void MSUBA_S( string& output ){_sap("msuba.s\t%s, %s")   COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }
void CVT_W( string& output ){  _sap("cvt.w.s\t%s, %s")   COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS]); }
void MAX_S( string& output ){  _sap("max.s\t%s, %s, %s") COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]);}
void MIN_S( string& output ){  _sap("min.s\t%s, %s, %s") COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]);}
void C_F( string& output ){    _sap("c.f.s\t%s, %s")     COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }
void C_EQ( string& output ){   _sap("c.eq.s\t%s, %s")    COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }
void C_LT( string& output ){   _sap("c.lt.s\t%s, %s")    COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }
void C_LE( string& output ){   _sap("c.le.s\t%s, %s")    COP1_REG_FP[DECODE_FS], COP1_REG_FP[DECODE_FT]); }
void CVT_S( string& output ){  _sap("cvt.s.w\t%s, %s")   COP1_REG_FP[DECODE_FD], COP1_REG_FP[DECODE_FS]); }
//****************************************************************************
//** END OF COP1                                                            **
//****************************************************************************

}	// End namespace R5900::OpcodeDisasm

//****************************************************************************
//** COP2 - (VU0)                                                           **
//****************************************************************************
void P_QMFC2( string& output ){   _sap("qmfc2\t%s, %s")      GPR_REG[DECODE_RT], COP2_REG_FP[DECODE_FS]);  }
void P_CFC2( string& output ){    _sap("cfc2\t%s, %s")      GPR_REG[DECODE_RT], COP2_REG_CTL[DECODE_FS]); }
void P_QMTC2( string& output ){   _sap("qmtc2\t%s, %s")      GPR_REG[DECODE_RT], COP2_REG_FP[DECODE_FS]); }
void P_CTC2( string& output ){    _sap("ctc2\t%s, %s")      GPR_REG[DECODE_RT], COP2_REG_CTL[DECODE_FS]); }
void P_BC2F( string& output ){    output += "bc2f\t";      offset_decode(output); }
void P_BC2T( string& output ){    output += "bc2t\t";      offset_decode(output); }
void P_BC2FL( string& output ){   output += "bc2fl\t";     offset_decode(output); }
void P_BC2TL( string& output ){   output += "bc2tl\t";     offset_decode(output); }
//******************************SPECIAL 1 VUO TABLE****************************************
#define _X ((cpuRegs.code>>24) & 1)
#define _Y ((cpuRegs.code>>23) & 1)
#define _Z ((cpuRegs.code>>22) & 1)
#define _W ((cpuRegs.code>>21) & 1)

const char *dest_string(void)
{
	static char str[5];
	int i = 0;

	if(_X) str[i++] = 'x';
	if(_Y) str[i++] = 'y';
	if(_Z) str[i++] = 'z';
	if(_W) str[i++] = 'w';
	str[i++] = 0;

	return (const char *)str;
}

char dest_fsf()
{
	const char arr[4] = { 'x', 'y', 'z', 'w' };
	return arr[(cpuRegs.code>>21)&3];
}

char dest_ftf()
{
	const char arr[4] = { 'x', 'y', 'z', 'w' };
	return arr[(cpuRegs.code>>23)&3];
}

void P_VADDx( string& output ){_sap("vaddx.%s %s, %s, %sx") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VADDy( string& output ){_sap("vaddy.%s %s, %s, %sy") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VADDz( string& output ){_sap("vaddz.%s %s, %s, %sz") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VADDw( string& output ){_sap("vaddw.%s %s, %s, %sw") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VSUBx( string& output ){_sap("vsubx.%s %s, %s, %sx") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VSUBy( string& output ){_sap("vsuby.%s %s, %s, %sy") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VSUBz( string& output ){_sap("vsubz.%s %s, %s, %sz") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VSUBw( string& output ){_sap("vsubw.%s %s, %s, %sw") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMADDx( string& output ){_sap("vmaddx.%s %s, %s, %sx") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMADDy( string& output ){_sap("vmaddy.%s %s, %s, %sy") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMADDz( string& output ){_sap("vmaddz.%s %s, %s, %sz") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMADDw( string& output ){_sap("vmaddw.%s %s, %s, %sw") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMSUBx( string& output ){_sap("vmsubx.%s %s, %s, %sx") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMSUBy( string& output ){_sap("vmsuby.%s %s, %s, %sy") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMSUBz( string& output ){_sap("vmsubz.%s %s, %s, %sz") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMSUBw( string& output ){_sap("vmsubw.%s %s, %s, %sw") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMAXx( string& output ){_sap("vmaxx.%s %s, %s, %sx") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMAXy( string& output ){_sap("vmaxy.%s %s, %s, %sy") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMAXz( string& output ){_sap("vmaxz.%s %s, %s, %sz") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMAXw( string& output ){_sap("vmaxw.%s %s, %s, %sw") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMINIx( string& output ){_sap("vminix.%s %s, %s, %sx") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMINIy( string& output ){_sap("vminiy.%s %s, %s, %sy") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); ;}
void P_VMINIz( string& output ){_sap("vminiz.%s %s, %s, %sz") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMINIw( string& output ){_sap("vminiw.%s %s, %s, %sw") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMULx( string& output ){_sap("vmulx.%s %s,%s,%sx") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMULy( string& output ){_sap("vmuly.%s %s,%s,%sy") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMULz( string& output ){_sap("vmulz.%s %s,%s,%sz") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMULw( string& output ){_sap("vmulw.%s %s,%s,%sw") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMULq( string& output ){_sap("vmulq.%s %s,%s,Q") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VMAXi( string& output ){_sap("vmaxi.%s %s,%s,I") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VMULi( string& output ){_sap("vmuli.%s %s,%s,I") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VMINIi( string& output ){_sap("vminii.%s %s,%s,I") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VADDq( string& output ){_sap("vaddq.%s %s,%s,Q") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VMADDq( string& output ){_sap("vmaddq.%s %s,%s,Q") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VADDi( string& output ){_sap("vaddi.%s %s,%s,I") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VMADDi( string& output ){_sap("vmaddi.%s %s,%s,I") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VSUBq( string& output ){_sap("vsubq.%s %s,%s,Q") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VMSUBq( string& output ){_sap("vmsubq.%s %s,%s,Q") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VSUbi( string& output ){_sap("vsubi.%s %s,%s,I") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VMSUBi( string& output ){_sap("vmsubi.%s %s,%s,I") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS]); }
void P_VADD( string& output ){_sap("vadd.%s %s, %s, %s") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMADD( string& output ){_sap("vmadd.%s %s, %s, %s") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMUL( string& output ){_sap("vmul.%s %s, %s, %s") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMAX( string& output ){_sap("vmax.%s %s, %s, %s") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VSUB( string& output ){_sap("vsub.%s %s, %s, %s") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMSUB( string& output ){_sap("vmsub.%s %s, %s, %s") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VOPMSUB( string& output ){_sap("vopmsub.xyz %s, %s, %s") COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMINI( string& output ){_sap("vmini.%s %s, %s, %s") dest_string(),COP2_REG_FP[DECODE_FD], COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VIADD( string& output ){_sap("viadd %s, %s, %s") COP2_REG_CTL[DECODE_SA], COP2_REG_CTL[DECODE_FS], COP2_REG_CTL[DECODE_FT]);}
void P_VISUB( string& output ){_sap("visub %s, %s, %s") COP2_REG_CTL[DECODE_SA], COP2_REG_CTL[DECODE_FS], COP2_REG_CTL[DECODE_FT]);}
void P_VIADDI( string& output ){_sap("viaddi %s, %s, 0x%x") COP2_REG_CTL[DECODE_FT], COP2_REG_CTL[DECODE_FS], DECODE_SA);}
void P_VIAND( string& output ){_sap("viand %s, %s, %s") COP2_REG_CTL[DECODE_SA], COP2_REG_CTL[DECODE_FS], COP2_REG_CTL[DECODE_FT]);}
void P_VIOR( string& output ){_sap("vior %s, %s, %s") COP2_REG_CTL[DECODE_SA], COP2_REG_CTL[DECODE_FS], COP2_REG_CTL[DECODE_FT]);}
void P_VCALLMS( string& output ){output += "vcallms";}
void P_CALLMSR( string& output ){output += "callmsr";}
//***********************************END OF SPECIAL1 VU0 TABLE*****************************
//******************************SPECIAL2 VUO TABLE*****************************************
void P_VADDAx( string& output ){_sap("vaddax.%s ACC,%s,%sx") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VADDAy( string& output ){_sap("vadday.%s ACC,%s,%sy") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VADDAz( string& output ){_sap("vaddaz.%s ACC,%s,%sz") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VADDAw( string& output ){_sap("vaddaw.%s ACC,%s,%sw") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VSUBAx( string& output ){_sap("vsubax.%s ACC,%s,%sx") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VSUBAy( string& output ){_sap("vsubay.%s ACC,%s,%sy") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VSUBAz( string& output ){_sap("vsubaz.%s ACC,%s,%sz") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VSUBAw( string& output ){_sap("vsubaw.%s ACC,%s,%sw") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMADDAx( string& output ){_sap("vmaddax.%s ACC,%s,%sx") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMADDAy( string& output ){_sap("vmadday.%s ACC,%s,%sy") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMADDAz( string& output ){_sap("vmaddaz.%s ACC,%s,%sz") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMADDAw( string& output ){_sap("vmaddaw.%s ACC,%s,%sw") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMSUBAx( string& output ){_sap("vmsubax.%s ACC,%s,%sx") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMSUBAy( string& output ){_sap("vmsubay.%s ACC,%s,%sy") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMSUBAz( string& output ){_sap("vmsubaz.%s ACC,%s,%sz") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMSUBAw( string& output ){_sap("vmsubaw.%s ACC,%s,%sw") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VITOF0( string& output ){_sap("vitof0.%s %s, %s") dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]); }
void P_VITOF4( string& output ){_sap("vitof4.%s %s, %s") dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]); }
void P_VITOF12( string& output ){_sap("vitof12.%s %s, %s") dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]); }
void P_VITOF15( string& output ){_sap("vitof15.%s %s, %s") dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]); }
void P_VFTOI0( string& output ) {_sap("vftoi0.%s %s, %s") dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]); }
void P_VFTOI4( string& output ) {_sap("vftoi4.%s %s, %s") dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]); }
void P_VFTOI12( string& output ){_sap("vftoi12.%s %s, %s") dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]); }
void P_VFTOI15( string& output ){_sap("vftoi15.%s %s, %s") dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]); }
void P_VMULAx( string& output ){_sap("vmulax.%s ACC,%s,%sx") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMULAy( string& output ){_sap("vmulay.%s ACC,%s,%sy") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMULAz( string& output ){_sap("vmulaz.%s ACC,%s,%sz") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMULAw( string& output ){_sap("vmulaw.%s ACC,%s,%sw") dest_string(),COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]);}
void P_VMULAq( string& output ){_sap("vmulaq.%s ACC %s, Q") dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VABS( string& output ){_sap("vabs.%s %s, %s") dest_string(),COP2_REG_FP[DECODE_FT], COP2_REG_FP[DECODE_FS]);}
void P_VMULAi( string& output ){_sap("vmulaq.%s ACC %s, I") dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VCLIPw( string& output ){_sap("vclip %sxyz, %sw") COP2_REG_FP[DECODE_FS], COP2_REG_FP[DECODE_FT]);}
void P_VADDAq( string& output ){_sap("vaddaq.%s ACC %s, Q") dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VMADDAq( string& output ){_sap("vmaddaq.%s ACC %s, Q") dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VADDAi( string& output ){_sap("vaddai.%s ACC %s, I") dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VMADDAi( string& output ){_sap("vmaddai.%s ACC %s, Q") dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VSUBAq( string& output ){_sap("vsubaq.%s ACC %s, Q") dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VMSUBAq( string& output ){_sap("vmsubaq.%s ACC %s, Q") dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VSUBAi( string& output ){_sap("vsubai.%s ACC %s, I") dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VMSUBAi( string& output ){_sap("vmsubai.%s ACC %s, I") dest_string(), COP2_REG_FP[DECODE_FS]); }
void P_VADDA( string& output ){_sap("vadda.%s ACC %s, %s") dest_string(), COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMADDA( string& output ){_sap("vmadda.%s ACC %s, %s") dest_string(), COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMULA( string& output ){_sap("vmula.%s ACC %s, %s") dest_string(), COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VSUBA( string& output ){_sap("vsuba.%s ACC %s, %s") dest_string(), COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VMSUBA( string& output ){_sap("vmsuba.%s ACC %s, %s") dest_string(), COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VOPMULA( string& output ){_sap("vopmula.xyz %sxyz, %sxyz") COP2_REG_FP[DECODE_FS],COP2_REG_FP[DECODE_FT]); }
void P_VNOP( string& output ){output += "vnop";}
void P_VMONE( string& output ){_sap("vmove.%s, %s, %s") dest_string(), COP2_REG_FP[DECODE_FT],COP2_REG_FP[DECODE_FS]); }
void P_VMR32( string& output ){_sap("vmr32.%s, %s, %s") dest_string(), COP2_REG_FP[DECODE_FT],COP2_REG_FP[DECODE_FS]); }
void P_VLQI( string& output ){_sap("vlqi %s%s, (%s++)") COP2_REG_FP[DECODE_FT], dest_string(), COP2_REG_CTL[DECODE_FS]);}
void P_VSQI( string& output ){_sap("vsqi %s%s, (%s++)") COP2_REG_FP[DECODE_FS], dest_string(), COP2_REG_CTL[DECODE_FT]);}
void P_VLQD( string& output ){_sap("vlqd %s%s, (--%s)") COP2_REG_FP[DECODE_FT], dest_string(), COP2_REG_CTL[DECODE_FS]);}
void P_VSQD( string& output ){_sap("vsqd %s%s, (--%s)") COP2_REG_FP[DECODE_FS], dest_string(), COP2_REG_CTL[DECODE_FT]);}
void P_VDIV( string& output ){_sap("vdiv Q, %s%c, %s%c") COP2_REG_FP[DECODE_FS], dest_fsf(), COP2_REG_FP[DECODE_FT], dest_ftf());}
void P_VSQRT( string& output ){_sap("vsqrt Q, %s%c") COP2_REG_FP[DECODE_FT], dest_ftf());}
void P_VRSQRT( string& output ){_sap("vrsqrt Q, %s%c, %s%c") COP2_REG_FP[DECODE_FS], dest_fsf(), COP2_REG_FP[DECODE_FT], dest_ftf());}
void P_VWAITQ( string& output ){output += "vwaitq";}
void P_VMTIR( string& output ){_sap("vmtir %s, %s%c") COP2_REG_CTL[DECODE_FT], COP2_REG_FP[DECODE_FS], dest_fsf());}
void P_VMFIR( string& output ){_sap("vmfir %s%c, %s") COP2_REG_FP[DECODE_FT], dest_string(), COP2_REG_CTL[DECODE_FS]);}
void P_VILWR( string& output ){_sap("vilwr %s, (%s)%s") COP2_REG_CTL[DECODE_FT], COP2_REG_CTL[DECODE_FS], dest_string());}
void P_VISWR( string& output ){_sap("viswr %s, (%s)%s") COP2_REG_CTL[DECODE_FT], COP2_REG_CTL[DECODE_FS], dest_string());}
void P_VRNEXT( string& output ){_sap("vrnext %s%s, R") COP2_REG_CTL[DECODE_FT], dest_string());}
void P_VRGET( string& output ){_sap("vrget %s%s, R") COP2_REG_CTL[DECODE_FT], dest_string());}
void P_VRINIT( string& output ){_sap("vrinit R, %s%s") COP2_REG_CTL[DECODE_FS], dest_string());}
void P_VRXOR( string& output ){_sap("vrxor R, %s%s") COP2_REG_CTL[DECODE_FS], dest_string());}
//************************************END OF SPECIAL2 VUO TABLE****************************

}
