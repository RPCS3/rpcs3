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

#include "R3000A.h"
#include "Debug.h"

namespace R3000A
{
	static char ostr[1024];

// Names of registers
	const char * const disRNameGPR[] = {
		"r0", "at", "v0", "v1", "a0", "a1","a2", "a3",
		"t0", "t1", "t2", "t3", "t4", "t5","t6", "t7",
		"s0", "s1", "s2", "s3", "s4", "s5","s6", "s7",
		"t8", "t9", "k0", "k1", "gp", "sp","fp", "ra"};

	const char * const disRNameCP0[] = {
		"Index"     , "Random"    , "EntryLo0", "EntryLo1", "Context" , "PageMask"  , "Wired"     , "*Check me*",
		"BadVAddr"  , "Count"     , "EntryHi" , "Compare" , "Status"  , "Cause"     , "ExceptPC"  , "PRevID"    ,
		"Config"    , "LLAddr"    , "WatchLo" , "WatchHi" , "XContext", "*RES*"     , "*RES*"     , "*RES*"     ,
		"*RES*"     , "*RES* "    , "PErr"    , "CacheErr", "TagLo"   , "TagHi"     , "ErrorEPC"  , "*RES*"     };

// Type definition of our functions

typedef char* (*TdisR3000AF)(u32 code, u32 pc);

// These macros are used to assemble the disassembler functions
#define MakeDisFg(fn, b) char* fn(u32 code, u32 pc) { b; return ostr; }
#define MakeDisF(fn, b) \
	static char* fn(u32 code, u32 pc) { \
		sprintf (ostr, "%8.8lx %8.8lx:", pc, code); \
		b; /*ostr[(strlen(ostr) - 1)] = 0;*/ return ostr; \
	}


#undef _Funct_
#undef _Rd_
#undef _Rt_
#undef _Rs_
#undef _Sa_
#undef _Im_
#undef _Target_

#define _Funct_  ((code      ) & 0x3F) // The funct part of the instruction register
#define _Rd_     ((code >> 11) & 0x1F) // The rd part of the instruction register
#define _Rt_     ((code >> 16) & 0x1F) // The rt part of the instruction register
#define _Rs_     ((code >> 21) & 0x1F) // The rs part of the instruction register
#define _Sa_     ((code >>  6) & 0x1F) // The sa part of the instruction register
#define _Im_     ( code & 0xFFFF)      // The immediate part of the instruction register

#define _Target_  ((pc & 0xf0000000) + ((code & 0x03ffffff) * 4))
#define _Branch_  (pc + 4 + ((short)_Im_ * 4))
#define _OfB_     _Im_, _nRs_

#define dName(i)	sprintf(ostr, "%s %-7s,", ostr, i)
#define dGPR(i)		sprintf(ostr, "%s %8.8lx (%s),", ostr, psxRegs.GPR.r[i], disRNameGPR[i])
#define dCP0(i)		sprintf(ostr, "%s %8.8lx (%s),", ostr, psxRegs.CP0.r[i], disRNameCP0[i])
#define dHI()		sprintf(ostr, "%s %8.8lx (%s),", ostr, psxRegs.GPR.n.hi, "hi")
#define dLO()		sprintf(ostr, "%s %8.8lx (%s),", ostr, psxRegs.GPR.n.lo, "lo")
#define dImm()		sprintf(ostr, "%s %4.4lx (%ld),", ostr, _Im_, _Im_)
#define dTarget()	sprintf(ostr, "%s %8.8lx,", ostr, _Target_)
#define dSa()		sprintf(ostr, "%s %2.2lx (%ld),", ostr, _Sa_, _Sa_)
#define dOfB()		sprintf(ostr, "%s %4.4lx (%8.8lx (%s)),", ostr, _Im_, psxRegs.GPR.r[_Rs_], disRNameGPR[_Rs_])
#define dOffset()	sprintf(ostr, "%s %8.8lx,", ostr, _Branch_)
#define dCode()		sprintf(ostr, "%s %8.8lx,", ostr, (code >> 6) & 0xffffff)

/*********************************************************
* Arithmetic with immediate operand                      *
* Format:  OP rt, rs, immediate                          *
*********************************************************/
MakeDisF(disADDI,		dName("ADDI");  dGPR(_Rt_); dGPR(_Rs_); dImm();)
MakeDisF(disADDIU,		dName("ADDIU"); dGPR(_Rt_); dGPR(_Rs_); dImm();)
MakeDisF(disANDI,		dName("ANDI");  dGPR(_Rt_); dGPR(_Rs_); dImm();)
MakeDisF(disORI,		dName("ORI");   dGPR(_Rt_); dGPR(_Rs_); dImm();)
MakeDisF(disSLTI,		dName("SLTI");  dGPR(_Rt_); dGPR(_Rs_); dImm();)
MakeDisF(disSLTIU,		dName("SLTIU"); dGPR(_Rt_); dGPR(_Rs_); dImm();)
MakeDisF(disXORI,		dName("XORI");  dGPR(_Rt_); dGPR(_Rs_); dImm();)

/*********************************************************
* Register arithmetic                                    *
* Format:  OP rd, rs, rt                                 *
*********************************************************/
MakeDisF(disADD,		dName("ADD");  dGPR(_Rd_); dGPR(_Rs_); dGPR(_Rt_);)
MakeDisF(disADDU,		dName("ADDU"); dGPR(_Rd_); dGPR(_Rs_); dGPR(_Rt_);)
MakeDisF(disAND,		dName("AND");  dGPR(_Rd_); dGPR(_Rs_); dGPR(_Rt_);)
MakeDisF(disNOR,		dName("NOR");  dGPR(_Rd_); dGPR(_Rs_); dGPR(_Rt_);)
MakeDisF(disOR,			dName("OR");   dGPR(_Rd_); dGPR(_Rs_); dGPR(_Rt_);)
MakeDisF(disSLT,		dName("SLT");  dGPR(_Rd_); dGPR(_Rs_); dGPR(_Rt_);)
MakeDisF(disSLTU,		dName("SLTU"); dGPR(_Rd_); dGPR(_Rs_); dGPR(_Rt_);)
MakeDisF(disSUB,		dName("SUB");  dGPR(_Rd_); dGPR(_Rs_); dGPR(_Rt_);)
MakeDisF(disSUBU,		dName("SUBU"); dGPR(_Rd_); dGPR(_Rs_); dGPR(_Rt_);)
MakeDisF(disXOR,		dName("XOR");  dGPR(_Rd_); dGPR(_Rs_); dGPR(_Rt_);)

/*********************************************************
* Register arithmetic & Register trap logic              *
* Format:  OP rs, rt                                     *
*********************************************************/
MakeDisF(disDIV,		dName("DIV");   dGPR(_Rs_); dGPR(_Rt_);)
MakeDisF(disDIVU,		dName("DIVU");  dGPR(_Rs_); dGPR(_Rt_);)
MakeDisF(disMULT,		dName("MULT");  dGPR(_Rs_); dGPR(_Rt_);)
MakeDisF(disMULTU,		dName("MULTU"); dGPR(_Rs_); dGPR(_Rt_);)

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, offset                                 *
*********************************************************/
MakeDisF(disBGEZ,		dName("BGEZ");   dGPR(_Rs_); dOffset();)
MakeDisF(disBGEZAL,		dName("BGEZAL"); dGPR(_Rs_); dOffset();)
MakeDisF(disBGTZ,		dName("BGTZ");   dGPR(_Rs_); dOffset();)
MakeDisF(disBLEZ,		dName("BLEZ");   dGPR(_Rs_); dOffset();)
MakeDisF(disBLTZ,		dName("BLTZ");   dGPR(_Rs_); dOffset();)
MakeDisF(disBLTZAL,		dName("BLTZAL"); dGPR(_Rs_); dOffset();)

/*********************************************************
* Shift arithmetic with constant shift                   *
* Format:  OP rd, rt, sa                                 *
*********************************************************/
MakeDisF(disSLL,		if (code) { dName("SLL"); dGPR(_Rd_); dGPR(_Rt_); dSa(); } else { dName("NOP"); })
MakeDisF(disSRA,		dName("SRA"); dGPR(_Rd_); dGPR(_Rt_); dSa();)
MakeDisF(disSRL,		dName("SRL"); dGPR(_Rd_); dGPR(_Rt_); dSa();)

/*********************************************************
* Shift arithmetic with variant register shift           *
* Format:  OP rd, rt, rs                                 *
*********************************************************/
MakeDisF(disSLLV,		dName("SLLV");  dGPR(_Rd_); dGPR(_Rt_); dGPR(_Rs_);)
MakeDisF(disSRAV,		dName("SRAV");  dGPR(_Rd_); dGPR(_Rt_); dGPR(_Rs_);)
MakeDisF(disSRLV,		dName("SRLV");  dGPR(_Rd_); dGPR(_Rt_); dGPR(_Rs_);)

/*********************************************************
* Load higher 16 bits of the first word in GPR with imm  *
* Format:  OP rt, immediate                              *
*********************************************************/
MakeDisF(disLUI,		dName("LUI"); dGPR(_Rt_); dImm();)

/*********************************************************
* Move from HI/LO to GPR                                 *
* Format:  OP rd                                         *
*********************************************************/
MakeDisF(disMFHI,		dName("MFHI"); dGPR(_Rd_); dHI();)
MakeDisF(disMFLO,		dName("MFLO"); dGPR(_Rd_); dLO();)

/*********************************************************
* Move from GPR to HI/LO                                 *
* Format:  OP rd                                         *
*********************************************************/
MakeDisF(disMTHI,		dName("MTHI"); dHI(); dGPR(_Rs_);)
MakeDisF(disMTLO,		dName("MTLO"); dLO(); dGPR(_Rs_);)

/*********************************************************
* Special purpose instructions                           *
* Format:  OP                                            *
*********************************************************/
MakeDisF(disBREAK,		dName("BREAK"))
MakeDisF(disRFE,		dName("RFE"))
MakeDisF(disSYSCALL,	dName("SYSCALL"))



MakeDisF(disRTPS,		dName("RTPS"))
MakeDisF(disOP  ,		dName("OP"))
MakeDisF(disNCLIP,		dName("NCLIP"))
MakeDisF(disDPCS,		dName("DPCS"))
MakeDisF(disINTPL,		dName("INTPL"))
MakeDisF(disMVMVA,		dName("MVMVA"))
MakeDisF(disNCDS ,		dName("NCDS"))
MakeDisF(disCDP ,		dName("CDP"))
MakeDisF(disNCDT ,		dName("NCDT"))
MakeDisF(disNCCS ,		dName("NCCS"))
MakeDisF(disCC  ,		dName("CC"))
MakeDisF(disNCS ,		dName("NCS"))
MakeDisF(disNCT  ,		dName("NCT"))
MakeDisF(disSQR  ,		dName("SQR"))
MakeDisF(disDCPL ,		dName("DCPL"))
MakeDisF(disDPCT ,		dName("DPCT"))
MakeDisF(disAVSZ3,		dName("AVSZ3"))
MakeDisF(disAVSZ4,		dName("AVSZ4"))
MakeDisF(disRTPT ,		dName("RTPT"))
MakeDisF(disGPF  ,		dName("GPF"))
MakeDisF(disGPL  ,		dName("GPL"))
MakeDisF(disNCCT ,		dName("NCCT"))

MakeDisF(disMFC2,		dName("MFC2"); dGPR(_Rt_);)
MakeDisF(disCFC2,		dName("CFC2"); dGPR(_Rt_);)
MakeDisF(disMTC2,		dName("MTC2"))
MakeDisF(disCTC2,		dName("CTC2"))

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, rt, offset                             *
*********************************************************/
MakeDisF(disBEQ,		dName("BEQ"); dGPR(_Rs_); dGPR(_Rt_); dOffset();)
MakeDisF(disBNE,		dName("BNE"); dGPR(_Rs_); dGPR(_Rt_); dOffset();)

/*********************************************************
* Jump to target                                         *
* Format:  OP target                                     *
*********************************************************/
MakeDisF(disJ,			dName("J");   dTarget();)
MakeDisF(disJAL,		dName("JAL"); dTarget(); dGPR(31);)

/*********************************************************
* Register jump                                          *
* Format:  OP rs, rd                                     *
*********************************************************/
MakeDisF(disJR,			dName("JR");   dGPR(_Rs_);)
MakeDisF(disJALR,		dName("JALR"); dGPR(_Rs_); dGPR(_Rd_))

/*********************************************************
* Load and store for GPR                                 *
* Format:  OP rt, offset(base)                           *
*********************************************************/
MakeDisF(disLB,			dName("LB");    dGPR(_Rt_);  dOfB();)
MakeDisF(disLBU,		dName("LBU");   dGPR(_Rt_);  dOfB();)
MakeDisF(disLH,			dName("LH");    dGPR(_Rt_);  dOfB();)
MakeDisF(disLHU,		dName("LHU");   dGPR(_Rt_);  dOfB();)
MakeDisF(disLW,			dName("LW");    dGPR(_Rt_);  dOfB();)
MakeDisF(disLWL,		dName("LWL");   dGPR(_Rt_);  dOfB();)
MakeDisF(disLWR,		dName("LWR");   dGPR(_Rt_);  dOfB();)
MakeDisF(disLWC2,		dName("LWC2");  dGPR(_Rt_);  dOfB();)
MakeDisF(disSB,			dName("SB");    dGPR(_Rt_);  dOfB();)
MakeDisF(disSH,			dName("SH");    dGPR(_Rt_);  dOfB();)
MakeDisF(disSW,			dName("SW");    dGPR(_Rt_);  dOfB();)
MakeDisF(disSWL,		dName("SWL");   dGPR(_Rt_);  dOfB();)
MakeDisF(disSWR,		dName("SWR");   dGPR(_Rt_);  dOfB();)
MakeDisF(disSWC2,		dName("SWC2");  dGPR(_Rt_);  dOfB();)

/*********************************************************
* Moves between GPR and COPx                             *
* Format:  OP rt, fs                                     *
*********************************************************/
MakeDisF(disMFC0,		dName("MFC0"); dGPR(_Rt_); dCP0(_Rd_);)
MakeDisF(disMTC0,		dName("MTC0"); dCP0(_Rd_); dGPR(_Rt_);)
MakeDisF(disCFC0,		dName("CFC0"); dGPR(_Rt_); dCP0(_Rd_);)
MakeDisF(disCTC0,		dName("CTC0"); dCP0(_Rd_); dGPR(_Rt_);)

/*********************************************************
* Unknow instruction (would generate an exception)       *
* Format:  ?                                             *
*********************************************************/
MakeDisF(disNULL,		dName("*** Bad OP ***");)


TdisR3000AF disR3000A_SPECIAL[] = { // Subset of disSPECIAL
	disSLL , disNULL , disSRL , disSRA , disSLLV   , disNULL  , disSRLV  , disSRAV ,
	disJR  , disJALR , disNULL, disNULL, disSYSCALL, disBREAK , disNULL  , disNULL ,
	disMFHI, disMTHI , disMFLO, disMTLO, disNULL   , disNULL  , disNULL  , disNULL ,
	disMULT, disMULTU, disDIV , disDIVU, disNULL   , disNULL  , disNULL  , disNULL ,
	disADD , disADDU , disSUB , disSUBU, disAND    , disOR    , disXOR   , disNOR  ,
	disNULL, disNULL , disSLT , disSLTU, disNULL   , disNULL  , disNULL  , disNULL ,
	disNULL, disNULL , disNULL, disNULL, disNULL   , disNULL  , disNULL   , disNULL ,
	disNULL, disNULL , disNULL, disNULL, disNULL   , disNULL  , disNULL  , disNULL};

MakeDisF(disSPECIAL,	disR3000A_SPECIAL[_Funct_](code, pc))

TdisR3000AF disR3000A_BCOND[] = { // Subset of disBCOND
	disBLTZ  , disBGEZ  , disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
	disNULL  , disNULL  , disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
	disBLTZAL, disBGEZAL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
	disNULL  , disNULL  , disNULL, disNULL, disNULL, disNULL, disNULL, disNULL};

MakeDisF(disBCOND,	disR3000A_BCOND[_Rt_](code, pc))

TdisR3000AF disR3000A_COP0[] = { // Subset of disCOP0
	disMFC0, disNULL, disCFC0, disNULL, disMTC0, disNULL, disCTC0, disNULL,
	disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
	disRFE , disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
	disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL};

MakeDisF(disCOP0,		disR3000A_COP0[_Rs_](code, pc))

TdisR3000AF disR3000A_BASIC[] = { // Subset of disBASIC (based on rs)
	disMFC2, disNULL, disCFC2, disNULL, disMTC2, disNULL, disCTC2, disNULL,
	disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
	disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
	disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL};

MakeDisF(disBASIC,		disR3000A_BASIC[_Rs_](code, pc))

TdisR3000AF disR3000A_COP2[] = { // Subset of disR3000F_COP2 (based on funct)
	disBASIC, disRTPS , disNULL , disNULL , disNULL, disNULL , disNCLIP, disNULL,
	disNULL , disNULL , disNULL , disNULL , disOP  , disNULL , disNULL , disNULL,
	disDPCS , disINTPL, disMVMVA, disNCDS , disCDP , disNULL , disNCDT , disNULL,
	disNULL , disNULL , disNULL , disNCCS , disCC  , disNULL , disNCS  , disNULL,
	disNCT  , disNULL , disNULL , disNULL , disNULL, disNULL , disNULL , disNULL,
	disSQR  , disDCPL , disDPCT , disNULL , disNULL, disAVSZ3, disAVSZ4, disNULL,
	disRTPT , disNULL , disNULL , disNULL , disNULL, disNULL , disNULL , disNULL,
	disNULL , disNULL , disNULL , disNULL , disNULL, disGPF  , disGPL  , disNCCT   };

MakeDisF(disCOP2,		disR3000A_COP2[_Funct_](code, pc))

TdisR3000AF disR3000A[] = {
	disSPECIAL    , disBCOND     , disJ       , disJAL  , disBEQ , disBNE , disBLEZ , disBGTZ ,
	disADDI       , disADDIU     , disSLTI    , disSLTIU, disANDI, disORI , disXORI , disLUI  ,
	disCOP0       , disNULL      , disCOP2    , disNULL , disNULL, disNULL, disNULL , disNULL ,
	disNULL       , disNULL      , disNULL    , disNULL , disNULL, disNULL, disNULL , disNULL ,
	disLB         , disLH        , disLWL     , disLW   , disLBU , disLHU , disLWR  , disNULL ,
	disSB         , disSH        , disSWL     , disSW   , disNULL, disNULL, disSWR  , disNULL ,
	disNULL       , disNULL      , disLWC2    , disNULL , disNULL, disNULL, disNULL , disNULL ,
	disNULL       , disNULL      , disSWC2    , disNULL  , disNULL, disNULL, disNULL , disNULL };

MakeDisFg(disR3000AF,	disR3000A[code >> 26](code, pc))

}
