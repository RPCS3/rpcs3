/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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

#include "Debug.h"
#include "R5900.h"
#include "VU.h"

const char * const disRNameCP2f[] = {
	"VF00", "VF01", "VF02", "VF03", "VF04", "VF05", "VF06", "VF07",
	"VF08", "VF09", "VF10", "VF11", "VF12", "VF13", "VF14", "VF15",
	"VF16", "VF17", "VF18", "VF19", "VF20", "VF21", "VF22", "VF23",
	"VF24", "VF25", "VF26", "VF27", "VF28", "VF29", "VF30", "VF31"};

const char * const disRNameCP2i[] = {
	"VI00",   "VI01",  "VI02", "VI03",   "VI04",  "VI05",     "VI06",  "VI07",
	"VI08",   "VI09",  "VI10", "VI11",   "VI12",  "VI13",     "VI14",  "VI15",
	"Status", "MAC",   "Clip", "*RES*",  "R",     "I",        "Q",     "*RES*",
	"*RES*",  "*RES*", "TPC",  "CMSAR0", "FBRST", "VPU-STAT", "*RES*", "CMSAR1"};

const char * const CP2VFnames[] = { "x", "y", "z", "w" };

namespace R5900
{

// Names of registers
const char * const disRNameGPR[] = {
	"r0", "at", "v0", "v1", "a0", "a1","a2", "a3",
	"t0", "t1", "t2", "t3", "t4", "t5","t6", "t7",
	"s0", "s1", "s2", "s3", "s4", "s5","s6", "s7",
	"t8", "t9", "k0", "k1", "gp", "sp","fp", "ra", "hi", "lo"}; // lo,hi used in rec

const char * const disRNameCP0[] = {
	"Index"     , "Random"    , "EntryLo0" , "EntryLo1", "Context" , "PageMask"  , "Wired"     , "*RES*",
	"BadVAddr"  , "Count"     , "EntryHi"  , "Compare" , "Status"  , "Cause"     , "ExceptPC"  , "PRevID",
	"Config"    , "LLAddr"    , "WatchLo"  , "WatchHi" , "*RES*"   , "*RES*"     , "*RES*"     , "Debug",
	"DEPC"      , "PerfCnt"   , "ErrCtl"   , "CacheErr", "TagLo"   , "TagHi"     , "ErrorEPC"  , "DESAVE"};

const char * const disRNameCP1[] = {
	"FPR0" , "FPR1" , "FPR2" , "FPR3" , "FPR4" , "FPR5" , "FPR6" , "FPR7",
	"FPR8" , "FPR9" , "FPR10", "FPR11", "FPR12", "FPR13", "FPR14", "FPR15",
	"FPR16", "FPR17", "FPR18", "FPR19", "FPR20", "FPR21", "FPR22", "FPR23",
	"FPR24", "FPR25", "FPR26", "FPR27", "FPR28", "FPR29", "FPR30", "FPR31"};

const char * const disRNameCP1c[] = {
	"FRevID", "*RES*", "*RES*", "*RES*", "*RES*", "*RES*", "*RES*", "*RES*",
	"*RES*",  "*RES*", "*RES*", "*RES*", "*RES*", "*RES*", "*RES*", "*RES*",
	"*RES*",  "*RES*", "*RES*", "*RES*", "*RES*", "*RES*", "*RES*", "*RES*",
	"*RES*",  "*RES*", "*RES*", "*RES*", "*RES*", "*RES*", "*RES*", "FStatus"};

// Type definition of our functions
#define DisFInterface  (string& output, u32 code)
#define DisFInterfaceT (string&, u32)
#define DisFInterfaceN (output, code)

typedef void (*TdisR5900F)DisFInterface;

// These macros are used to assemble the disassembler functions
#define MakeDisF(fn, b) \
	void fn DisFInterface { \
		ssprintf(output, "(%8.8x) ", code); \
		b; \
	}

#undef _Target_
#undef _Branch_
#undef _Funct_
#undef _Rd_
#undef _Rt_
#undef _Rs_
#undef _Sa_
#undef _Im_

#define _Funct_  ((code      ) & 0x3F) // The funct part of the instruction register 
#define _Rd_     ((code >> 11) & 0x1F) // The rd part of the instruction register 
#define _Rt_     ((code >> 16) & 0x1F) // The rt part of the instruction register 
#define _Rs_     ((code >> 21) & 0x1F) // The rs part of the instruction register 
#define _Sa_     ((code >>  6) & 0x1F) // The sa part of the instruction register
#define _Im_     ( code & 0xFFFF)      // The immediate part of the instruction register


#define _rRs_   cpuRegs.GPR.r[_Rs_].UL[1], cpuRegs.GPR.r[_Rs_].UL[0]   // Rs register
#define _rRt_   cpuRegs.GPR.r[_Rt_].UL[1], cpuRegs.GPR.r[_Rt_].UL[0]   // Rt register
#define _rRd_   cpuRegs.GPR.r[_Rd_].UL[1], cpuRegs.GPR.r[_Rd_].UL[0]   // Rd register
#define _rSa_   cpuRegs.GPR.r[_Sa_].UL[1], cpuRegs.GPR.r[_Sa_].UL[0]   // Sa register

#define _rFs_   cpuRegs.CP0.r[_Rd_]   // Fs register

#define _rRs32_   cpuRegs.GPR.r[_Rs_].UL[0]   // Rs register
#define _rRt32_   cpuRegs.GPR.r[_Rt_].UL[0]   // Rt register
#define _rRd32_   cpuRegs.GPR.r[_Rd_].UL[0]   // Rd register
#define _rSa32_   cpuRegs.GPR.r[_Sa_].UL[0]   // Sa register


#define _nRs_ _rRs_, disRNameGPR[_Rs_]
#define _nRt_ _rRt_, disRNameGPR[_Rt_]
#define _nRd_ _rRd_, disRNameGPR[_Rd_]
#define _nSa_ _rSa_, disRNameGPR[_Sa_]
#define _nRd0_ _rFs_, disRNameCP0[_Rd_]

#define _nRs32_ _rRs32_, disRNameGPR[_Rs_]
#define _nRt32_ _rRt32_, disRNameGPR[_Rt_]
#define _nRd32_ _rRd32_, disRNameGPR[_Rd_]
#define _nSa32_ _rSa32_, disRNameGPR[_Sa_]

#define _I_       _Im_, _Im_
#define _Target_  ((cpuRegs.pc & 0xf0000000) + ((code & 0x03ffffff) * 4))
#define _Branch_  (cpuRegs.pc + 4 + ((short)_Im_ * 4))
#define _OfB_     _Im_, _nRs_

#define _Fsf_ ((code >> 21) & 0x03)
#define _Ftf_ ((code >> 23) & 0x03)

// sap!  it stands for string append.  It's not a friendly name but for now it makes
// the copy-paste marathon of code below more readable!
#define _sap( str ) ssappendf( output, str, 

#define dName(i)	_sap("%-7s\t") i);
#define dGPR128(i)	_sap("%8.8x_%8.8x_%8.8x_%8.8x (%s),") cpuRegs.GPR.r[i].UL[3], cpuRegs.GPR.r[i].UL[2], cpuRegs.GPR.r[i].UL[1], cpuRegs.GPR.r[i].UL[0], disRNameGPR[i])
#define dGPR64(i)	_sap("%8.8x_%8.8x (%s),") cpuRegs.GPR.r[i].UL[1], cpuRegs.GPR.r[i].UL[0], disRNameGPR[i]) 
#define dGPR64U(i)	_sap("%8.8x_%8.8x (%s),") cpuRegs.GPR.r[i].UL[3], cpuRegs.GPR.r[i].UL[2], disRNameGPR[i])
#define dGPR32(i)	_sap("%8.8x (%s),") cpuRegs.GPR.r[i].UL[0], disRNameGPR[i])

#define dCP032(i)	_sap("%8.8x (%s),") cpuRegs.CP0.r[i], disRNameCP0[i])

#define dCP132(i)	_sap("%f (%s),") fpuRegs.fpr[i].f, disRNameCP1[i])
#define dCP1c32(i)	_sap("%8.8x (%s),") fpuRegs.fprc[i], disRNameCP1c[i])
#define dCP1acc()	_sap("%f (ACC),") fpuRegs.ACC.f)

#define dCP2128f(i)		_sap("w=%f z=%f y=%f x=%f (%s),") VU0.VF[i].f.w, VU0.VF[i].f.z, VU0.VF[i].f.y, VU0.VF[i].f.x, disRNameCP2f[i])
#define dCP232x(i)		_sap("x=%f (%s),") VU0.VF[i].f.x, disRNameCP2f[i])
#define dCP232y(i)		_sap("y=%f (%s),") VU0.VF[i].f.y, disRNameCP2f[i])
#define dCP232z(i)		_sap("z=%f (%s),") VU0.VF[i].f.z, disRNameCP2f[i])
#define dCP232w(i)		_sap("w=%f (%s),") VU0.VF[i].f.w, disRNameCP2f[i])
#define dCP2ACCf()		_sap("w=%f z=%f y=%f x=%f (ACC),") VU0.ACC.f.w, VU0.ACC.f.z, VU0.ACC.f.y, VU0.ACC.f.x)
#define dCP232i(i)		_sap("%8.8x (%s),") VU0.VI[i].UL, disRNameCP2i[i])
#define dCP232iF(i)		_sap("%f (%s),") VU0.VI[i].F, disRNameCP2i[i])
#define dCP232f(i, j)	_sap("Q %s=%f (%s),") CP2VFnames[j], VU0.VF[i].F[j], disRNameCP2f[i])

#define dHI64()		_sap("%8.8x_%8.8x (%s),") cpuRegs.HI.UL[1], cpuRegs.HI.UL[0], "hi")
#define dLO64()		_sap("%8.8x_%8.8x (%s),") cpuRegs.LO.UL[1], cpuRegs.LO.UL[0], "lo")
#define dImm()		_sap("%4.4x (%d),") _Im_, _Im_)
#define dTarget()	_sap("%8.8x,") _Target_)
#define dSa()		_sap("%2.2x (%d),") _Sa_, _Sa_)
#define dSa32()		_sap("%2.2x (%d),") _Sa_+32, _Sa_+32)
#define dOfB()		_sap("%4.4x (%8.8x (%s)),") _Im_, cpuRegs.GPR.r[_Rs_].UL[0], disRNameGPR[_Rs_])
#define dOffset()	_sap("%8.8x,") _Branch_)
#define dCode()		_sap("%8.8x,") (code >> 6) & 0xffffff)
#define dSaR()		_sap("%8.8x,") cpuRegs.sa)

struct sSymbol {
	u32 addr;
	char name[64];
};

static sSymbol *dSyms = NULL;
static int nSymAlloc = 0;
static int nSyms = 0;

void disR5900AddSym(u32 addr, const char *name) {

    if( !pxAssertDev(strlen(name) < sizeof(dSyms->name),
		wxsFormat(L"String length out of bounds on debug symbol. Allowed=%d, Symbol=%d", sizeof(dSyms->name)-1, strlen(name)))
	) return;

	if( nSyms+1 >= nSymAlloc )
	{
		// Realloc by threshold block sizes.
		nSymAlloc += 64 + (nSyms / 4);
		dSyms = (sSymbol*)realloc(dSyms, sizeof(sSymbol) * (nSymAlloc));
	}

	if (dSyms == NULL) return;
	dSyms[nSyms].addr = addr;
	strncpy(dSyms[nSyms].name, name, 64);
	nSyms++;
}

void disR5900FreeSyms() {
	if (dSyms != NULL) { free(dSyms); dSyms = NULL; }
	nSymAlloc = 0;
	nSyms = 0;
}

const char *disR5900GetSym(u32 addr) {
	int i;

	if (dSyms == NULL) return NULL;
	for (i=0; i<nSyms; i++)
		if (dSyms[i].addr == addr) return dSyms[i].name;

	return NULL;
}

const char *disR5900GetUpperSym(u32 addr) {
	u32 laddr;
	int i, j=-1;

	if (dSyms == NULL) return NULL;
	for (i=0, laddr=0; i<nSyms; i++) {
		if (dSyms[i].addr < addr && dSyms[i].addr > laddr) {
			laddr = dSyms[i].addr;
			j = i;
		}
	}
	if (j == -1) return NULL;
	return dSyms[j].name;
}

void dFindSym( string& output, u32 addr )
{
	const char* label = disR5900GetSym( addr );
	if( label != NULL )
		output.append( label );
}

#define dAppendSym(addr) dFindSym( output, addr )

/*********************************************************
* Arithmetic with immediate operand                      *
* Format:  OP rt, rs, immediate                          *
*********************************************************/
MakeDisF(disADDI,		dName("ADDI");   dGPR64(_Rt_); dGPR32(_Rs_); dImm();)
MakeDisF(disADDIU,		dName("ADDIU");  dGPR64(_Rt_); dGPR32(_Rs_); dImm();)
MakeDisF(disANDI,		dName("ANDI");   dGPR64(_Rt_); dGPR64(_Rs_); dImm();)
MakeDisF(disORI,		dName("ORI");    dGPR64(_Rt_); dGPR64(_Rs_); dImm();)
MakeDisF(disSLTI,		dName("SLTI");   dGPR64(_Rt_); dGPR64(_Rs_); dImm();)
MakeDisF(disSLTIU,		dName("SLTIU");  dGPR64(_Rt_); dGPR64(_Rs_); dImm();)
MakeDisF(disXORI,		dName("XORI");   dGPR64(_Rt_); dGPR64(_Rs_); dImm();)
MakeDisF(disDADDI,      dName("DADDI");  dGPR64(_Rt_); dGPR64(_Rs_); dImm();)
MakeDisF(disDADDIU,     dName("DADDIU"); dGPR64(_Rt_); dGPR64(_Rs_); dImm();)

/*********************************************************
* Register arithmetic                                    *
* Format:  OP rd, rs, rt                                 *
*********************************************************/
MakeDisF(disADD,		dName("ADD");   dGPR64(_Rd_); dGPR32(_Rs_); dGPR32(_Rt_);)
MakeDisF(disADDU,		dName("ADDU");  dGPR64(_Rd_); dGPR32(_Rs_); dGPR32(_Rt_);)
MakeDisF(disDADD,		dName("DADD");  dGPR64(_Rd_); dGPR64(_Rs_); dGPR64(_Rt_);)
MakeDisF(disDADDU,		dName("DADDU"); dGPR64(_Rd_); dGPR64(_Rs_); dGPR64(_Rt_);)
MakeDisF(disSUB,		dName("SUB");   dGPR64(_Rd_); dGPR32(_Rs_); dGPR32(_Rt_);)
MakeDisF(disSUBU,		dName("SUBU");  dGPR64(_Rd_); dGPR32(_Rs_); dGPR32(_Rt_);)
MakeDisF(disDSUB,		dName("DSUB");  dGPR64(_Rd_); dGPR64(_Rs_); dGPR64(_Rt_);)
MakeDisF(disDSUBU,		dName("DSDBU"); dGPR64(_Rd_); dGPR64(_Rs_); dGPR64(_Rt_);)
MakeDisF(disAND,		dName("AND");   dGPR64(_Rd_); dGPR64(_Rs_); dGPR64(_Rt_);)
MakeDisF(disOR,		    dName("OR");    dGPR64(_Rd_); dGPR64(_Rs_); dGPR64(_Rt_);)
MakeDisF(disXOR,		dName("XOR");   dGPR64(_Rd_); dGPR64(_Rs_); dGPR64(_Rt_);)
MakeDisF(disNOR,		dName("NOR");   dGPR64(_Rd_); dGPR64(_Rs_); dGPR64(_Rt_);)
MakeDisF(disSLT,		dName("SLT");   dGPR64(_Rd_); dGPR64(_Rs_); dGPR64(_Rt_);) 
MakeDisF(disSLTU,		dName("SLTU");  dGPR64(_Rd_); dGPR64(_Rs_); dGPR64(_Rt_);)

/*********************************************************
* Jump to target                                         *
* Format:  OP target                                     *
*********************************************************/
MakeDisF(disJ,			dName("J");   dTarget(); dAppendSym(_Target_);)
MakeDisF(disJAL,		dName("JAL"); dTarget(); dGPR32(31); dAppendSym(_Target_);)

/*********************************************************
* Register jump                                          *
* Format:  OP rs, rd                                     *
*********************************************************/
MakeDisF(disJR,			dName("JR");   dGPR32(_Rs_); dAppendSym(cpuRegs.GPR.r[_Rs_].UL[0]);)
MakeDisF(disJALR,		dName("JALR"); dGPR32(_Rs_); dGPR32(_Rd_); dAppendSym(cpuRegs.GPR.r[_Rs_].UL[0]);)

/*********************************************************
* Register mult/div & Register trap logic                *
* Format:  OP rs, rt                                     *
*********************************************************/
MakeDisF(disDIV,		dName("DIV");   dGPR32(_Rs_); dGPR32(_Rt_);)
MakeDisF(disDIVU,		dName("DIVU");  dGPR32(_Rs_); dGPR32(_Rt_);)
MakeDisF(disMULT,		dName("MULT");  dGPR32(_Rs_); dGPR32(_Rt_); dGPR32(_Rd_);)
MakeDisF(disMULTU,		dName("MULTU"); dGPR32(_Rs_); dGPR32(_Rt_); dGPR32(_Rd_);)

/*********************************************************
* Load higher 16 bits of the first word in GPR with imm  *
* Format:  OP rt, immediate                              *
*********************************************************/
MakeDisF(disLUI,		dName("LUI"); dGPR64(_Rt_); dImm();)

/*********************************************************
* Move from HI/LO to GPR                                 *
* Format:  OP rd                                         *
*********************************************************/
MakeDisF(disMFHI,		dName("MFHI"); dGPR64(_Rd_); dHI64();)
MakeDisF(disMFLO,		dName("MFLO"); dGPR64(_Rd_); dLO64();)

/*********************************************************
* Move to GPR to HI/LO & Register jump                   *
* Format:  OP rs                                         *
*********************************************************/
MakeDisF(disMTHI,		dName("MTHI"); dHI64(); dGPR64(_Rs_);)
MakeDisF(disMTLO,		dName("MTLO"); dLO64(); dGPR64(_Rs_);)

/*********************************************************
* Shift arithmetic with constant shift                   *
* Format:  OP rd, rt, sa                                 *
*********************************************************/
MakeDisF(disSLL,		if (code) { dName("SLL");   dGPR64(_Rd_); dGPR32(_Rt_); dSa(); } else { dName("NOP"); })
MakeDisF(disDSLL,		dName("DSLL");   dGPR64(_Rd_); dGPR64(_Rt_); dSa();)
MakeDisF(disDSLL32,		dName("DSLL32"); dGPR64(_Rd_); dGPR64(_Rt_); dSa32();)
MakeDisF(disSRA,		dName("SRA");    dGPR64(_Rd_); dGPR32(_Rt_); dSa();)
MakeDisF(disDSRA,		dName("DSRA");   dGPR64(_Rd_); dGPR64(_Rt_); dSa();)
MakeDisF(disDSRA32,		dName("DSRA32"); dGPR64(_Rd_); dGPR64(_Rt_); dSa32();)
MakeDisF(disSRL,		dName("SRL");    dGPR64(_Rd_); dGPR32(_Rt_); dSa();)
MakeDisF(disDSRL,		dName("DSRL");   dGPR64(_Rd_); dGPR64(_Rt_); dSa();)
MakeDisF(disDSRL32,		dName("DSRL32"); dGPR64(_Rd_); dGPR64(_Rt_); dSa32();)

/*********************************************************
* Shift arithmetic with variant register shift           *
* Format:  OP rd, rt, rs                                 *
*********************************************************/
MakeDisF(disSLLV,		dName("SLLV");  dGPR64(_Rd_); dGPR32(_Rt_); dGPR32(_Rs_);)
MakeDisF(disDSLLV,		dName("DSLLV"); dGPR64(_Rd_); dGPR64(_Rt_); dGPR32(_Rs_);)
MakeDisF(disSRAV,		dName("SRAV");  dGPR64(_Rd_); dGPR32(_Rt_); dGPR32(_Rs_);)
MakeDisF(disDSRAV,		dName("DSRAV"); dGPR64(_Rd_); dGPR64(_Rt_); dGPR32(_Rs_);)
MakeDisF(disSRLV,		dName("SRLV");  dGPR64(_Rd_); dGPR32(_Rt_); dGPR32(_Rs_);)
MakeDisF(disDSRLV,		dName("DSRLV"); dGPR64(_Rd_); dGPR64(_Rt_); dGPR32(_Rs_);)

/*********************************************************
* Load and store for GPR                                 *
* Format:  OP rt, offset(base)                           *
*********************************************************/
MakeDisF(disLB,			dName("LB");  dGPR64(_Rt_);  dOfB();)
MakeDisF(disLBU,		dName("LBU"); dGPR64(_Rt_);  dOfB();)
MakeDisF(disLH,			dName("LH");  dGPR64(_Rt_);  dOfB();)
MakeDisF(disLHU,		dName("LHU"); dGPR64(_Rt_);  dOfB();)
MakeDisF(disLW,			dName("LW");  dGPR64(_Rt_);  dOfB();)
MakeDisF(disLWU,		dName("LWU"); dGPR64(_Rt_);  dOfB();)
MakeDisF(disLWL,		dName("LWL"); dGPR64(_Rt_);  dOfB();)
MakeDisF(disLWR,		dName("LWR"); dGPR64(_Rt_);  dOfB();)
MakeDisF(disLD,			dName("LD");  dGPR64(_Rt_);  dOfB();)
MakeDisF(disLDL,		dName("LDL"); dGPR64(_Rt_);  dOfB();)
MakeDisF(disLDR,		dName("LDR"); dGPR64(_Rt_);  dOfB();)
MakeDisF(disLQ,			dName("LQ");  dGPR128(_Rt_); dOfB();)
MakeDisF(disSB,			dName("SB");  dGPR64(_Rt_);  dOfB();)
MakeDisF(disSH,			dName("SH");  dGPR64(_Rt_);  dOfB();)
MakeDisF(disSW,			dName("SW");  dGPR64(_Rt_);  dOfB();)
MakeDisF(disSWL,		dName("SWL"); dGPR64(_Rt_);  dOfB();)
MakeDisF(disSWR,		dName("SWR"); dGPR64(_Rt_);  dOfB();)
MakeDisF(disSD,			dName("SD");  dGPR64(_Rt_);  dOfB();)
MakeDisF(disSDL,		dName("SDL"); dGPR64(_Rt_);  dOfB();)
MakeDisF(disSDR,		dName("SDR"); dGPR64(_Rt_);  dOfB();)
MakeDisF(disSQ,			dName("SQ");  dGPR128(_Rt_); dOfB();)

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, rt, offset                             *
*********************************************************/
MakeDisF(disBEQ,		dName("BEQ"); dGPR64(_Rs_); dGPR64(_Rt_); dOffset();)
MakeDisF(disBNE,		dName("BNE"); dGPR64(_Rs_); dGPR64(_Rt_); dOffset();)

/*********************************************************
* Moves between GPR and COPx                             *
* Format:  OP rt, rd                                     *
*********************************************************/
MakeDisF(disMFC0,		dName("MFC0"); dGPR32(_Rt_); dCP032(_Rd_);)
MakeDisF(disMTC0,		dName("MTC0"); dCP032(_Rd_); dGPR32(_Rt_);)

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, offset                                 *
*********************************************************/

MakeDisF(disBGEZ, 		dName("BGEZ");   dGPR64(_Rs_); dOffset();)
MakeDisF(disBGEZAL, 	dName("BGEZAL"); dGPR64(_Rs_); dOffset();)
MakeDisF(disBGTZ, 		dName("BGTZ");   dGPR64(_Rs_); dOffset();)   
MakeDisF(disBLEZ, 		dName("BLEZ");   dGPR64(_Rs_); dOffset();)  
MakeDisF(disBLTZ, 		dName("BLTZ");   dGPR64(_Rs_); dOffset();)    
MakeDisF(disBLTZAL,     dName("BLTZAL"); dGPR64(_Rs_); dOffset();) 


/*********************************************************
* Register branch logic  Likely                          *
* Format:  OP rs, offset                                 *
*********************************************************/


MakeDisF(disBEQL, 		dName("BEQL");      dGPR64(_Rs_); dGPR64(_Rt_); dOffset();)  
MakeDisF(disBNEL, 		dName("BNEL");      dGPR64(_Rs_); dGPR64(_Rt_); dOffset();)   
MakeDisF(disBLEZL, 		dName("BLEZL");     dGPR64(_Rs_); dOffset();)  
MakeDisF(disBGTZL, 		dName("BGTZL");     dGPR64(_Rs_); dOffset();) 
MakeDisF(disBLTZL, 		dName("BLTZL");     dGPR64(_Rs_); dOffset();) 
MakeDisF(disBGEZL, 		dName("BGEZL");     dGPR64(_Rs_); dOffset();)  
MakeDisF(disBLTZALL,    dName("BLTZALL");   dGPR64(_Rs_); dOffset();)
MakeDisF(disBGEZALL, 	dName("BGEZALL");   dGPR64(_Rs_); dOffset();)


/*********************************************************
*   COP0 opcodes                                         *
*                                                        *
*********************************************************/

MakeDisF(disBC0F,       dName("BC0F");   dOffset();)
MakeDisF(disBC0T,       dName("BC0T");   dOffset();)
MakeDisF(disBC0FL,      dName("BC0FL");  dOffset();)
MakeDisF(disBC0TL,      dName("BC0TL");  dOffset();)

MakeDisF(disTLBR,		dName("TLBR");)
MakeDisF(disTLBWI,		dName("TLBWI");)
MakeDisF(disTLBWR,		dName("TLBWR");)
MakeDisF(disTLBP,		dName("TLBP");)
MakeDisF(disERET,		dName("ERET");)
MakeDisF(disEI,			dName("EI");)
MakeDisF(disDI,			dName("DI");)

/*********************************************************
*   COP1 opcodes                                         *
*                                                        *
*********************************************************/

#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_

MakeDisF(disMFC1,		dName("MFC1"); dGPR64(_Rt_); dCP132(_Fs_);)
MakeDisF(disCFC1,		dName("CFC1"); dGPR64(_Rt_); dCP1c32(_Fs_);)
MakeDisF(disMTC1,		dName("MTC1"); dCP132(_Fs_); dGPR64(_Rt_);)
MakeDisF(disCTC1,		dName("CTC1"); dCP1c32(_Fs_); dGPR64(_Rt_);)

MakeDisF(disBC1F,		dName("BC1F");)
MakeDisF(disBC1T,		dName("BC1T");)
MakeDisF(disBC1FL,		dName("BC1FL");)
MakeDisF(disBC1TL,		dName("BC1TL");)

MakeDisF(disADDs,		dName("ADDs");   dCP132(_Fd_); dCP132(_Fs_); dCP132(_Ft_);)
MakeDisF(disSUBs,		dName("SUBs");   dCP132(_Fd_); dCP132(_Fs_); dCP132(_Ft_);)
MakeDisF(disMULs,		dName("MULs");   dCP132(_Fd_); dCP132(_Fs_); dCP132(_Ft_);)
MakeDisF(disDIVs,		dName("DIVs");   dCP132(_Fd_); dCP132(_Fs_); dCP132(_Ft_);)
MakeDisF(disSQRTs,		dName("SQRTs");  dCP132(_Fd_); dCP132(_Ft_);)
MakeDisF(disABSs,		dName("ABSs");   dCP132(_Fd_); dCP132(_Fs_);)
MakeDisF(disMOVs,		dName("MOVs");   dCP132(_Fd_); dCP132(_Fs_);)
MakeDisF(disNEGs,		dName("NEGs");   dCP132(_Fd_); dCP132(_Fs_);)
MakeDisF(disRSQRTs,		dName("RSQRTs"); dCP132(_Fd_); dCP132(_Fs_); dCP132(_Ft_);)
MakeDisF(disADDAs,		dName("ADDAs");  dCP1acc();    dCP132(_Fs_); dCP132(_Ft_);)
MakeDisF(disSUBAs,		dName("SUBAs");  dCP1acc();    dCP132(_Fs_); dCP132(_Ft_);)
MakeDisF(disMULAs,		dName("MULAs");  dCP1acc();    dCP132(_Fs_); dCP132(_Ft_);)
MakeDisF(disMADDs,		dName("MADDs");  dCP132(_Fd_); dCP1acc();    dCP132(_Fs_); dCP132(_Ft_);)
MakeDisF(disMSUBs,		dName("MSUBs");  dCP132(_Fd_); dCP1acc();    dCP132(_Fs_); dCP132(_Ft_);)
MakeDisF(disMADDAs,		dName("MADDAs"); dCP1acc();    dCP132(_Fs_); dCP132(_Ft_);)
MakeDisF(disMSUBAs,		dName("MSUBAs"); dCP1acc();    dCP132(_Fs_); dCP132(_Ft_);)
MakeDisF(disCVTWs,		dName("CVTWs");  dCP132(_Fd_); dCP132(_Fs_);)
MakeDisF(disMAXs,		dName("MAXs");   dCP132(_Fd_); dCP132(_Fs_); dCP132(_Ft_);)
MakeDisF(disMINs,		dName("MINs");   dCP132(_Fd_); dCP132(_Fs_); dCP132(_Ft_);)
MakeDisF(disCFs,		dName("CFs");    dCP132(_Fs_); dCP132(_Ft_);)
MakeDisF(disCEQs,		dName("CEQs");   dCP132(_Fs_); dCP132(_Ft_);)
MakeDisF(disCLTs,		dName("CLTs");   dCP132(_Fs_); dCP132(_Ft_);)
MakeDisF(disCLEs,		dName("CLEs");   dCP132(_Fs_); dCP132(_Ft_);)

MakeDisF(disCVTSw,		dName("CVTSw"); dCP132(_Fd_); dCP132(_Fs_);)

/*********************************************************
* Load and store for COP1                                *
* Format:  OP rt, offset(base)                           *
*********************************************************/

MakeDisF(disLWC1,		dName("LWC1"); dCP132(_Ft_); dOffset();)
MakeDisF(disSWC1,		dName("SWC1"); dCP132(_Ft_); dOffset();)

/*********************************************************
* Conditional Move                                       *
* Format:  OP rd, rs, rt                                 *
*********************************************************/

MakeDisF(disMOVZ,		dName("MOVZ"); dGPR64(_Rd_); dGPR64(_Rs_); dGPR64(_Rt_);)
MakeDisF(disMOVN,		dName("MOVN"); dGPR64(_Rd_); dGPR64(_Rs_); dGPR64(_Rt_);)

/*********************************************************
*   MMI opcodes                                         *
*                                                        *
*********************************************************/

MakeDisF(disMULT1,		dName("MULT1");)
MakeDisF(disMULTU1,		dName("MULTU1");)

/*********************************************************
*   MMI0 opcodes                                         *
*                                                        *
*********************************************************/

MakeDisF(disPADDW,		dName("PADDW");)
MakeDisF(disPADDH,		dName("PADDH");)
MakeDisF(disPADDB,		dName("PADDB");)

MakeDisF(disPADDSW,		dName("PADDSW");)
MakeDisF(disPADDSH,		dName("PADDSH");)
MakeDisF(disPADDSB,		dName("PADDSB");)

MakeDisF(disPSUBW,		dName("PSUBW");)
MakeDisF(disPSUBH,		dName("PSUBH");)
MakeDisF(disPSUBB,		dName("PSUBB");)

MakeDisF(disPSUBSW,		dName("PSUBSW");)
MakeDisF(disPSUBSH,		dName("PSUBSH");)
MakeDisF(disPSUBSB,		dName("PSUBSB");)

MakeDisF(disPCGTW,		dName("PCGTW");)
MakeDisF(disPCGTH,		dName("PCGTH");)
MakeDisF(disPCGTB,		dName("PCGTB");)

MakeDisF(disPMAXW,		dName("PMAXW");)
MakeDisF(disPMAXH,		dName("PMAXH");)

MakeDisF(disPEXTLW,		dName("PEXTLW"); dGPR128(_Rd_); dGPR64(_Rs_); dGPR64(_Rt_);)
MakeDisF(disPEXTLH,		dName("PEXTLH"); dGPR128(_Rd_); dGPR64(_Rs_); dGPR64(_Rt_);)
MakeDisF(disPEXTLB,		dName("PEXTLB");)
MakeDisF(disPEXTS,		dName("PEXTS");)

MakeDisF(disPPACW,		dName("PPACW");)
MakeDisF(disPPACH,		dName("PPACH");)
MakeDisF(disPPACB,		dName("PPACB");)
MakeDisF(disPPACS,		dName("PPACS");)

/*********************************************************
*   MMI1 opcodes                                         *
*                                                        *
*********************************************************/

MakeDisF(disPADSBH,		dName("PADSBH");)

MakeDisF(disPABSW,		dName("PABSW");)
MakeDisF(disPABSH,		dName("PABSH");)

MakeDisF(disPCEQW,		dName("PCEQW");)
MakeDisF(disPCEQH,		dName("PCEQH");)
MakeDisF(disPCEQB,		dName("PCEQB");)

MakeDisF(disPMINW,		dName("PMINW");)
MakeDisF(disPMINH,		dName("PMINH");)

MakeDisF(disPADDUW,		dName("PADDUW");)
MakeDisF(disPADDUH,		dName("PADDUH");)
MakeDisF(disPADDUB,		dName("PADDUB");)

MakeDisF(disPSUBUW,		dName("PSUBUW");)
MakeDisF(disPSUBUH,		dName("PSUBUH");)
MakeDisF(disPSUBUB,		dName("PSUBUB");)

MakeDisF(disPEXTUW,		dName("PEXTUW"); dGPR128(_Rd_); dGPR64U(_Rs_); dGPR64U(_Rt_);)
MakeDisF(disPEXTUH,		dName("PEXTUH"); dGPR128(_Rd_); dGPR64U(_Rs_); dGPR64U(_Rt_);)
MakeDisF(disPEXTUB,		dName("PEXTUB");)

MakeDisF(disQFSRV,		dName("QFSRV");)

/*********************************************************
*   MMI2 opcodes                                         *
*                                                        *
*********************************************************/

MakeDisF(disPMADDW,		dName("PMADDW");)
MakeDisF(disPMADDH,		dName("PMADDH");)

MakeDisF(disPSLLVW,		dName("PSLLVW");)
MakeDisF(disPSRLVW,		dName("PSRLVW");)

MakeDisF(disPMFHI,		dName("PMFHI");)
MakeDisF(disPMFLO,		dName("PMFLO");)

MakeDisF(disPINTH,		dName("PINTH");)

MakeDisF(disPMULTW,		dName("PMULTW");)
MakeDisF(disPMULTH,		dName("PMULTH");)

MakeDisF(disPDIVW,		dName("PDIVW");)
MakeDisF(disPDIVH,		dName("PDIVH");)

MakeDisF(disPCPYLD,		dName("PCPYLD"); dGPR128(_Rd_); dGPR128(_Rs_); dGPR128(_Rt_);)

MakeDisF(disPAND,		dName("PAND"); dGPR128(_Rd_); dGPR128(_Rs_); dGPR128(_Rt_);)
MakeDisF(disPXOR,		dName("PXOR"); dGPR128(_Rd_); dGPR128(_Rs_); dGPR128(_Rt_);)

MakeDisF(disPMSUBW,		dName("PMSUBW");)
MakeDisF(disPMSUBH,		dName("PMSUBH");)

MakeDisF(disPHMADH,		dName("PHMADH");)
MakeDisF(disPHMSBH,		dName("PHMSBH");)

MakeDisF(disPEXEW,		dName("PEXEW");)
MakeDisF(disPEXEH,		dName("PEXEH");)

MakeDisF(disPREVH,		dName("PREVH");)

MakeDisF(disPDIVBW,		dName("PDIVBW");)

MakeDisF(disPROT3W,		dName("PROT3W");)

/*********************************************************
*   MMI3 opcodes                                         *
*                                                        *
*********************************************************/

MakeDisF(disPMADDUW,	dName("PMADDUW");)

MakeDisF(disPSRAVW,		dName("PSRAVW");)

MakeDisF(disPMTHI,		dName("PMTHI");)
MakeDisF(disPMTLO,		dName("PMTLO");)

MakeDisF(disPINTEH,		dName("PINTEH");)

MakeDisF(disPMULTUW,	dName("PMULTUW");)
MakeDisF(disPDIVUW,		dName("PDIVUW");)

MakeDisF(disPCPYUD,		dName("PCPYUD"); dGPR128(_Rd_); dGPR128(_Rt_); dGPR128(_Rs_);)

MakeDisF(disPOR,		dName("POR"); dGPR128(_Rd_); dGPR128(_Rs_); dGPR128(_Rt_);)
MakeDisF(disPNOR,		dName("PNOR"); dGPR128(_Rd_); dGPR128(_Rs_); dGPR128(_Rt_);)

MakeDisF(disPEXCH,		dName("PEXCH");)
MakeDisF(disPEXCW,		dName("PEXCW");)

MakeDisF(disPCPYH,		dName("PCPYH"); dGPR128(_Rd_); dGPR128(_Rt_);)

/*********************************************************
*   COP2 opcodes                                         *
*                                                        *
*********************************************************/

#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_

#define _X code>>24
#define _Y code>>23
#define _Z code>>22
#define _W code>>21

MakeDisF(disLQC2,		dName("LQC2"); dCP2128f(_Rt_); dOfB();)  
MakeDisF(disSQC2,		dName("SQC2"); dCP2128f(_Rt_); dOfB();)  

MakeDisF(disQMFC2,		dName("QMFC2");)  
MakeDisF(disQMTC2,		dName("QMTC2");)  
MakeDisF(disCFC2,		dName("CFC2"); dGPR32(_Rt_); dCP232i(_Fs_);)  
MakeDisF(disCTC2,		dName("CTC2"); dCP232i(_Fs_); dGPR32(_Rt_);)  

MakeDisF(disBC2F,		dName("BC2F");)
MakeDisF(disBC2T,		dName("BC2T");)
MakeDisF(disBC2FL,		dName("BC2FL");)
MakeDisF(disBC2TL,		dName("BC2TL");)

// SPEC1
MakeDisF(disVADD,		dName("VADD");)
MakeDisF(disVADDx,		dName("VADDx"); dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);)
MakeDisF(disVADDy,		dName("VADDy"); dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);)
MakeDisF(disVADDz,		dName("VADDz"); dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);)
MakeDisF(disVADDw,		dName("VADDw"); dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);)
MakeDisF(disVADDq,		dName("VADDq"); dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_Q);)
MakeDisF(disVADDi,		dName("VADDi"); dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_I);)
MakeDisF(disVSUB,		dName("VSUB");)
MakeDisF(disVSUBx,		dName("VSUBx"); dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);)
MakeDisF(disVSUBy,		dName("VSUBy"); dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);)
MakeDisF(disVSUBz,		dName("VSUBz"); dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);)
MakeDisF(disVSUBw,		dName("VSUBw"); dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);)
MakeDisF(disVSUBq,		dName("VSUBq");)
MakeDisF(disVSUBi,		dName("VSUBi");)
MakeDisF(disVMADD,		dName("VMADD");)
MakeDisF(disVMADDx,		dName("VMADDx"); dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232x(_Ft_);)
MakeDisF(disVMADDy,		dName("VMADDy"); dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232y(_Ft_);)
MakeDisF(disVMADDz,		dName("VMADDz"); dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232z(_Ft_);)
MakeDisF(disVMADDw,		dName("VMADDw"); dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232w(_Ft_);)
MakeDisF(disVMADDq,		dName("VMADDq");)
MakeDisF(disVMADDi,		dName("VMADDi");)
MakeDisF(disVMSUB,		dName("VMSUB");)
MakeDisF(disVMSUBx,		dName("VMSUBx");)
MakeDisF(disVMSUBy,		dName("VMSUBy");)
MakeDisF(disVMSUBz,		dName("VMSUBz");)
MakeDisF(disVMSUBw,		dName("VMSUBw");)
MakeDisF(disVMSUBq,		dName("VMSUBq");)
MakeDisF(disVMSUBi,		dName("VMSUBi");)
MakeDisF(disVMAX,		dName("VMAX");)
MakeDisF(disVMAXx,		dName("VMAXx");)
MakeDisF(disVMAXy,		dName("VMAXy");)
MakeDisF(disVMAXz,		dName("VMAXz");)
MakeDisF(disVMAXw,		dName("VMAXw");)
MakeDisF(disVMAXi,		dName("VMAXi");)
MakeDisF(disVMINI,		dName("VMINI");)
MakeDisF(disVMINIx,		dName("VMINIx");)
MakeDisF(disVMINIy,		dName("VMINIy");)
MakeDisF(disVMINIz,		dName("VMINIz");)
MakeDisF(disVMINIw,		dName("VMINIw");)
MakeDisF(disVMINIi,		dName("VMINIi");)
MakeDisF(disVMUL,		dName("VMUL");)
MakeDisF(disVMULx,		dName("VMULx");)
MakeDisF(disVMULy,		dName("VMULy");)
MakeDisF(disVMULz,		dName("VMULz");)
MakeDisF(disVMULw,		dName("VMULw");)
MakeDisF(disVMULq,		dName("VMULq");)
MakeDisF(disVMULi,		dName("VMULi");)
MakeDisF(disVIADD,		dName("VIADD");)
MakeDisF(disVIADDI,		dName("VIADDI");)
MakeDisF(disVISUB,		dName("VISUB");)
MakeDisF(disVIAND,		dName("VIAND");)
MakeDisF(disVIOR,		dName("VIOR");)
MakeDisF(disVOPMSUB,	dName("VOPMSUB");)
MakeDisF(disVCALLMS,	dName("VCALLMS");)
MakeDisF(disVCALLMSR,	dName("VCALLMSR");)

// SPEC2
MakeDisF(disVADDA,		dName("VADDA");)
MakeDisF(disVADDAx,		dName("VADDAx");)
MakeDisF(disVADDAy,		dName("VADDAy");)
MakeDisF(disVADDAz,		dName("VADDAz");)
MakeDisF(disVADDAw,		dName("VADDAw");)
MakeDisF(disVADDAq,		dName("VADDAq");)
MakeDisF(disVADDAi,		dName("VADDAi");)
MakeDisF(disVMADDA,		dName("VMADDA");)
MakeDisF(disVMADDAx,	dName("VMADDAx"); dCP2ACCf(); dCP2128f(_Fs_); dCP232x(_Ft_);)
MakeDisF(disVMADDAy,	dName("VMADDAy"); dCP2ACCf(); dCP2128f(_Fs_); dCP232y(_Ft_);)
MakeDisF(disVMADDAz,	dName("VMADDAz"); dCP2ACCf(); dCP2128f(_Fs_); dCP232z(_Ft_);)
MakeDisF(disVMADDAw,	dName("VMADDAw"); dCP2ACCf(); dCP2128f(_Fs_); dCP232w(_Ft_);)
MakeDisF(disVMADDAq,	dName("VMADDAq");)
MakeDisF(disVMADDAi,	dName("VMADDAi");)
MakeDisF(disVSUBAx,		dName("VSUBAx");)
MakeDisF(disVSUBAy,		dName("VSUBAy");)
MakeDisF(disVSUBAz,		dName("VSUBAz");)
MakeDisF(disVSUBAw,		dName("VSUBAw");)
MakeDisF(disVMSUBAx,	dName("VMSUBAx");)
MakeDisF(disVMSUBAy,	dName("VMSUBAy");)
MakeDisF(disVMSUBAz,	dName("VMSUBAz");)
MakeDisF(disVMSUBAw,	dName("VMSUBAw");)
MakeDisF(disVITOF0,		dName("VITOF0");)
MakeDisF(disVITOF4,		dName("VITOF4");)
MakeDisF(disVITOF12,	dName("VITOF12");)
MakeDisF(disVITOF15,	dName("VITOF15");)
MakeDisF(disVFTOI0,		dName("VFTOI0");)
MakeDisF(disVFTOI4,		dName("VFTOI4");)
MakeDisF(disVFTOI12,	dName("VFTOI12");)
MakeDisF(disVFTOI15,	dName("VFTOI15");)
MakeDisF(disVMULA,		dName("VMULA");)
MakeDisF(disVMULAx,		dName("VMULAx"); dCP2ACCf(); dCP2128f(_Fs_); dCP232x(_Ft_);)
MakeDisF(disVMULAy,		dName("VMULAy");)
MakeDisF(disVMULAz,		dName("VMULAz");)
MakeDisF(disVMULAw,		dName("VMULAw");)
MakeDisF(disVMOVE,		dName("VMOVE"); dCP2128f(_Ft_); dCP2128f(_Fs_);)
MakeDisF(disVMR32,		dName("VMR32");)
MakeDisF(disVDIV,		dName("VDIV");)
MakeDisF(disVSQRT,		dName("VSQRT"); dCP232f(_Ft_, _Ftf_);)
MakeDisF(disVRSQRT,		dName("VRSQRT");)
MakeDisF(disVRNEXT,		dName("VRNEXT");)
MakeDisF(disVRGET,		dName("VRGET");)
MakeDisF(disVRINIT,		dName("VRINIT");)
MakeDisF(disVRXOR,		dName("VRXOR");)
MakeDisF(disVWAITQ,		dName("VWAITQ");)

/*********************************************************
* Special purpose instructions                           *
* Format:  OP                                            *
*********************************************************/

MakeDisF(disSYNC,		dName("SYNC");)  
MakeDisF(disBREAK,		dName("BREAK");) 
MakeDisF(disSYSCALL,	dName("SYSCALL"); dCode();)
MakeDisF(disCACHE,		ssappendf(output, "%-7s, %x,", "CACHE", _Rt_); dOfB();)
MakeDisF(disPREF,		dName("PREF");) 

MakeDisF(disMFSA,		dName("MFSA"); dGPR64(_Rd_); dSaR();)   
MakeDisF(disMTSA,		dName("MTSA"); dGPR64(_Rs_); dSaR();)   

MakeDisF(disMTSAB,      dName("MTSAB");dGPR64(_Rs_); dImm();)
MakeDisF(disMTSAH,      dName("MTSAH");dGPR64(_Rs_); dImm();)

MakeDisF(disTGE,	    dName("TGE");  dGPR64(_Rs_); dGPR64(_Rt_);)
MakeDisF(disTGEU,	    dName("TGEU"); dGPR64(_Rs_); dGPR64(_Rt_);)
MakeDisF(disTLT,	    dName("TLT");  dGPR64(_Rs_); dGPR64(_Rt_);)
MakeDisF(disTLTU,	    dName("TLTU"); dGPR64(_Rs_); dGPR64(_Rt_);) 
MakeDisF(disTEQ,		dName("TEQ");  dGPR64(_Rs_); dGPR64(_Rt_);) 
MakeDisF(disTNE,	    dName("TNE");  dGPR64(_Rs_); dGPR64(_Rt_);)

MakeDisF(disTGEI,	    dName("TGEI");  dGPR64(_Rs_); dImm();)
MakeDisF(disTGEIU,	    dName("TGEIU"); dGPR64(_Rs_); dImm();) 
MakeDisF(disTLTI,	    dName("TLTI");  dGPR64(_Rs_); dImm();) 
MakeDisF(disTLTIU,	    dName("TLTIU"); dGPR64(_Rs_); dImm();)  
MakeDisF(disTEQI,	    dName("TEQI");  dGPR64(_Rs_); dImm();)  
MakeDisF(disTNEI,	    dName("TNEI");  dGPR64(_Rs_); dImm();)

/*********************************************************
* Unknown instruction (would generate an exception)      *
* Format:  ?                                             *
*********************************************************/
static MakeDisF(disNULL,		dName("*** Bad OP ***");)

TdisR5900F disR5900_MMI0[] = { // Subset of disMMI0
    disPADDW,  disPSUBW,  disPCGTW,  disPMAXW,
	disPADDH,  disPSUBH,  disPCGTH,  disPMAXH,
    disPADDB,  disPSUBB,  disPCGTB,  disNULL, 
	disNULL,   disNULL,   disNULL,   disNULL,
    disPADDSW, disPSUBSW, disPEXTLW, disPPACW, 
	disPADDSH, disPSUBSH, disPEXTLH, disPPACH,
    disPADDSB, disPSUBSB, disPEXTLB, disPPACB, 
	disNULL,   disNULL,   disPEXTS,  disPPACS};

static void disMMI0( string& output, u32 code )
{
	disR5900_MMI0[_Sa_]( output, code );
}

TdisR5900F disR5900_MMI1[] = { // Subset of disMMI1
    disNULL,   disPABSW,  disPCEQW,  disPMINW, 
	disPADSBH, disPABSH,  disPCEQH,  disPMINH,
    disNULL,   disNULL,   disPCEQB,  disNULL, 
	disNULL,   disNULL,   disNULL,   disNULL,
    disPADDUW, disPSUBUW, disPEXTUW, disNULL, 
	disPADDUH, disPSUBUH, disPEXTUH, disNULL,
    disPADDUB, disPSUBUB, disPEXTUB, disQFSRV, 
	disNULL,   disNULL,   disNULL,   disNULL};

static void disMMI1( string& output, u32 code )
{
	disR5900_MMI1[_Sa_]( output, code );
}

TdisR5900F disR5900_MMI2[] = { // Subset of disMMI2
    disPMADDW, disNULL,   disPSLLVW, disPSRLVW, 
	disPMSUBW, disNULL,   disNULL,   disNULL,
    disPMFHI,  disPMFLO,  disPINTH,  disNULL, 
	disPMULTW, disPDIVW,  disPCPYLD, disNULL,
    disPMADDH, disPHMADH, disPAND,   disPXOR, 
	disPMSUBH, disPHMSBH, disNULL,   disNULL,
    disNULL,   disNULL,   disPEXEH,  disPREVH, 
	disPMULTH, disPDIVBW, disPEXEW,  disPROT3W};

static void disMMI2( string& output, u32 code )
{
	disR5900_MMI2[_Sa_]( output, code );
}

TdisR5900F disR5900_MMI3[] = { // Subset of disMMI3
    disPMADDUW, disNULL,   disNULL,   disPSRAVW, 
	disNULL,    disNULL,   disNULL,   disNULL,
    disPMTHI,   disPMTLO,  disPINTEH, disNULL, 
	disPMULTUW, disPDIVUW, disPCPYUD, disNULL,
    disNULL,    disNULL,   disPOR,    disPNOR, 
	disNULL,    disNULL,   disNULL,   disNULL,
    disNULL,    disNULL,   disPEXCH,  disPCPYH, 
	disNULL,    disNULL,   disPEXCW,  disNULL};

static void disMMI3( string& output, u32 code )
{
	disR5900_MMI3[_Sa_]( output, code );
}

TdisR5900F disR5900_MMI[] = { // Subset of disMMI
    disNULL,  disNULL,   disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
    disMMI0,  disMMI2,   disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
    disNULL,  disNULL,   disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
    disMULT1, disMULTU1, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
    disNULL,  disNULL,   disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
    disMMI1,  disMMI3,   disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
    disNULL,  disNULL,   disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
    disNULL,  disNULL,   disNULL, disNULL, disNULL, disNULL, disNULL, disNULL};

static void disMMI( string& output, u32 code )
{
	disR5900_MMI[_Funct_]( output, code );
}


TdisR5900F disR5900_COP0_BC0[] = { //subset of disCOP0 BC
    disBC0F, disBC0T, disBC0FL, disBC0TL, disNULL, disNULL, disNULL, disNULL,
    disNULL, disNULL, disNULL , disNULL , disNULL, disNULL, disNULL, disNULL,
    disNULL, disNULL, disNULL , disNULL , disNULL, disNULL, disNULL, disNULL,
    disNULL, disNULL, disNULL , disNULL , disNULL, disNULL, disNULL, disNULL,
};

static void disCOP0_BC0( string& output, u32 code )
{
	disR5900_COP0_BC0[_Rt_]( output, code );
}

TdisR5900F disR5900_COP0_Func[] = { //subset of disCOP0 Function
    disNULL, disTLBR, disTLBWI, disNULL, disNULL, disNULL, disTLBWR, disNULL,
    disTLBP, disNULL, disNULL , disNULL, disNULL, disNULL, disNULL , disNULL,
    disNULL, disNULL, disNULL , disNULL, disNULL, disNULL, disNULL , disNULL,
    disERET, disNULL, disNULL , disNULL, disNULL, disNULL, disNULL , disNULL,
    disNULL, disNULL, disNULL , disNULL, disNULL, disNULL, disNULL , disNULL,
    disNULL, disNULL, disNULL , disNULL, disNULL, disNULL, disNULL , disNULL,
    disNULL, disNULL, disNULL , disNULL, disNULL, disNULL, disNULL , disNULL,
    disEI  , disDI  , disNULL , disNULL, disNULL, disNULL, disNULL , disNULL
};
static void disCOP0_Func( string& output, u32 code )
{
	disR5900_COP0_Func[_Funct_]( output, code );
}

TdisR5900F disR5900_COP0[] = { // Subset of disCOP0
    disMFC0,      disNULL, disNULL, disNULL, disMTC0, disNULL, disNULL, disNULL,
    disCOP0_BC0,  disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
    disCOP0_Func, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
    disNULL,      disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL};

static void disCOP0( string& output, u32 code )
{
	disR5900_COP0[_Rs_]( output, code );
}

TdisR5900F disR5900_COP1_S[] = { //subset of disCOP1 S
    disADDs,  disSUBs,  disMULs,  disDIVs, disSQRTs, disABSs,  disMOVs,   disNEGs,
    disNULL,  disNULL,  disNULL,  disNULL, disNULL,  disNULL,  disNULL,   disNULL,
    disNULL,  disNULL,  disNULL,  disNULL, disNULL,  disNULL,  disRSQRTs, disNULL,
    disADDAs, disSUBAs, disMULAs, disNULL, disMADDs, disMSUBs, disMADDAs, disMSUBAs,
    disNULL,  disNULL,  disNULL,  disNULL, disCVTWs, disNULL,  disNULL,   disNULL,
    disMINs,  disMAXs,  disNULL,  disNULL, disNULL,  disNULL,  disNULL,   disNULL,
    disCFs,   disNULL,  disCEQs,  disNULL, disCLTs,  disNULL,  disCLEs,   disNULL,
    disNULL,  disNULL,  disNULL,  disNULL, disNULL,  disNULL,  disNULL,   disNULL,
};

static void disCOP1_S( string& output, u32 code )
{
	disR5900_COP1_S[_Funct_]( output, code );
}

TdisR5900F disR5900_COP1_W[] = { //subset of disCOP1 W
    disNULL,  disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
    disNULL,  disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
    disNULL,  disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
    disNULL,  disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
    disCVTSw, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
    disNULL,  disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
    disNULL,  disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
    disNULL,  disNULL, disNULL, disNULL, disNULL, disNULL, disNULL, disNULL,
};

static void disCOP1_W( string& output, u32 code )
{
	disR5900_COP1_W[_Funct_]( output, code );
}

TdisR5900F disR5900_COP1_BC1[] = { //subset of disCOP1 BC
    disBC1F, disBC1T, disBC1FL, disBC1TL, disNULL, disNULL, disNULL, disNULL,
    disNULL, disNULL, disNULL , disNULL , disNULL, disNULL, disNULL, disNULL,
    disNULL, disNULL, disNULL , disNULL , disNULL, disNULL, disNULL, disNULL,
    disNULL, disNULL, disNULL , disNULL , disNULL, disNULL, disNULL, disNULL,
};

static void disCOP1_BC1( string& output, u32 code )
{
	disR5900_COP1_BC1[_Rt_]( output, code );
}

TdisR5900F disR5900_COP1[] = { // Subset of disCOP1
    disMFC1,     disNULL, disCFC1, disNULL, disMTC1,   disNULL, disCTC1, disNULL,
    disCOP1_BC1, disNULL, disNULL, disNULL, disNULL,   disNULL, disNULL, disNULL,
    disCOP1_S,   disNULL, disNULL, disNULL, disCOP1_W, disNULL, disNULL, disNULL,
    disNULL,     disNULL, disNULL, disNULL, disNULL,   disNULL, disNULL, disNULL};

static void disCOP1( string& output, u32 code )
{
	disR5900_COP1[_Rs_]( output, code );
}

TdisR5900F disR5900_COP2_SPEC2[] = { //subset of disCOP2 SPEC2
    disVADDAx,  disVADDAy,  disVADDAz,  disVADDAw,  disVSUBAx,  disVSUBAy,  disVSUBAz,  disVSUBAw,
    disVMADDAx, disVMADDAy, disVMADDAz, disVMADDAw, disVMSUBAx, disVMSUBAy, disVMSUBAz, disVMSUBAw,
    disVITOF0,  disVITOF4,  disVITOF12, disVITOF15, disVFTOI0, disVFTOI4, disVFTOI12, disVFTOI15,
    disVMULAx,  disVMULAy,  disVMULAz,  disVMULAw,  disNULL, disNULL, disNULL, disNULL,
    disVADDAq,  disVMADDAq, disVADDAi,  disVMADDAi, disNULL, disNULL, disNULL, disNULL,
    disVADDA,   disVMADDA,  disVMULA,   disNULL,    disNULL, disNULL, disNULL, disNULL,
    disVMOVE,   disVMR32,   disNULL,    disNULL,    disNULL, disNULL, disNULL, disNULL,
    disVDIV,    disVSQRT,   disVRSQRT,  disVWAITQ,  disNULL, disNULL, disNULL, disNULL,
    disVRNEXT,  disVRGET,   disVRINIT,  disVRXOR,   disNULL, disNULL, disNULL, disNULL,
    disNULL,    disNULL,    disNULL,    disNULL,    disNULL, disNULL, disNULL, disNULL,
    disNULL,    disNULL,    disNULL,    disNULL,    disNULL, disNULL, disNULL, disNULL,
    disNULL,    disNULL,    disNULL,    disNULL,    disNULL, disNULL, disNULL, disNULL,
    disNULL,    disNULL,    disNULL,    disNULL,    disNULL, disNULL, disNULL, disNULL,
    disNULL,    disNULL,    disNULL,    disNULL,    disNULL, disNULL, disNULL, disNULL,
    disNULL,    disNULL,    disNULL,    disNULL,    disNULL, disNULL, disNULL, disNULL,
    disNULL,    disNULL,    disNULL,    disNULL,    disNULL, disNULL, disNULL, disNULL,
};

static void disCOP2_SPEC2( string& output, u32 code )
{
	disR5900_COP2_SPEC2[(code & 0x3) | ((code >> 4) & 0x7c)]( output, code );
}

TdisR5900F disR5900_COP2_SPEC1[] = { //subset of disCOP2 SPEC1
    disVADDx,   disVADDy,    disVADDz,  disVADDw,  disVSUBx,        disVSUBy,        disVSUBz,         disVSUBw,
    disVMADDx,  disVMADDy,   disVMADDz, disVMADDw, disVMSUBx,       disVMSUBy,       disVMSUBz,        disVMSUBw,
    disVMAXx,   disVMAXy,    disVMAXz,  disVMAXw,  disVMINIx,       disVMINIy,       disVMINIz,        disVMINIw,
    disVMULx,   disVMULy,    disVMULz,  disVMULw,  disVMULq,        disVMAXi,        disVMULi,         disVMINIi,
    disVADDq,   disVMADDq,   disVADDi,  disVMADDi, disVSUBq,        disVMSUBq,       disVSUBi,         disVMSUBi,
    disVADD,    disVMADD,    disVMUL,   disVMAX,   disVSUB,         disVMSUB,        disVOPMSUB,       disVMINI,
    disVIADD,   disVISUB,    disVIADDI, disNULL,   disVIAND,        disVIOR,         disNULL,          disNULL,
    disVCALLMS, disVCALLMSR, disNULL,   disNULL,   disCOP2_SPEC2,   disCOP2_SPEC2,   disCOP2_SPEC2,    disCOP2_SPEC2,
};

static void disCOP2_SPEC1( string& output, u32 code )
{
	disR5900_COP2_SPEC1[_Funct_]( output, code );
}

TdisR5900F disR5900_COP2_BC2[] = { //subset of disCOP2 BC
    disBC2F, disBC2T, disBC2FL, disBC2TL, disNULL, disNULL, disNULL, disNULL,
    disNULL, disNULL, disNULL , disNULL , disNULL, disNULL, disNULL, disNULL,
    disNULL, disNULL, disNULL , disNULL , disNULL, disNULL, disNULL, disNULL,
    disNULL, disNULL, disNULL , disNULL , disNULL, disNULL, disNULL, disNULL,
};

static void disCOP2_BC2( string& output, u32 code )
{
	disR5900_COP2_BC2[_Rt_]( output, code );
}

TdisR5900F disR5900_COP2[] = { // Subset of disCOP2
	disNULL,       disQMFC2,      disCFC2,       disNULL,       disNULL,       disQMTC2,      disCTC2,       disNULL,
	disCOP2_BC2,   disNULL,       disNULL,       disNULL,       disNULL,       disNULL,       disNULL,       disNULL,
	disCOP2_SPEC1, disCOP2_SPEC1, disCOP2_SPEC1, disCOP2_SPEC1, disCOP2_SPEC1, disCOP2_SPEC1, disCOP2_SPEC1, disCOP2_SPEC1,
	disCOP2_SPEC1, disCOP2_SPEC1, disCOP2_SPEC1, disCOP2_SPEC1, disCOP2_SPEC1, disCOP2_SPEC1, disCOP2_SPEC1, disCOP2_SPEC1};

static void disCOP2( string& output, u32 code )
{
	disR5900_COP2[_Rs_]( output, code );
}

TdisR5900F disR5900_REGIMM[] = { // Subset of disREGIMM
    disBLTZ,   disBGEZ,   disBLTZL,   disBGEZL,   disNULL, disNULL, disNULL, disNULL,
    disTGEI,   disTGEIU,  disTLTI,    disTLTIU,   disTEQI, disNULL, disTNEI, disNULL,
    disBLTZAL, disBGEZAL, disBLTZALL, disBGEZALL, disNULL, disNULL, disNULL, disNULL,
    disMTSAB,  disMTSAH , disNULL,    disNULL,    disNULL, disNULL, disNULL, disNULL};

static void disREGIMM( string& output, u32 code )
{
	disR5900_REGIMM[_Rt_]( output, code );
}

TdisR5900F disR5900_SPECIAL[] = {
    disSLL,    disNULL, disSRL,    disSRA,   disSLLV,    disNULL, disSRLV,  disSRAV,
    disJR,     disJALR, disMOVZ,   disMOVN,  disSYSCALL, disBREAK,disNULL,  disSYNC,
    disMFHI,   disMTHI, disMFLO,   disMTLO,  disDSLLV,   disNULL, disDSRLV, disDSRAV,
    disMULT,   disMULTU,disDIV,    disDIVU,  disNULL,    disNULL, disNULL,  disNULL,
    disADD,    disADDU, disSUB,    disSUBU,  disAND,     disOR,   disXOR,   disNOR,
    disMFSA ,  disMTSA, disSLT,    disSLTU,  disDADD,    disDADDU,disDSUB,  disDSUBU,
    disTGE,    disTGEU, disTLT,    disTLTU,  disTEQ,     disNULL, disTNE,   disNULL,
    disDSLL,   disNULL, disDSRL,   disDSRA,  disDSLL32,  disNULL, disDSRL32,disDSRA32 };

static void disSPECIAL( string& output, u32 code )
{
	disR5900_SPECIAL[_Funct_]( output, code );
}

TdisR5900F disR5900[] = {
    disSPECIAL, disREGIMM, disJ   , disJAL  , disBEQ , disBNE , disBLEZ , disBGTZ ,
    disADDI   , disADDIU , disSLTI, disSLTIU, disANDI, disORI , disXORI , disLUI  ,
    disCOP0   , disCOP1  , disCOP2, disNULL , disBEQL, disBNEL, disBLEZL, disBGTZL,
    disDADDI  , disDADDIU, disLDL , disLDR  , disMMI , disNULL, disLQ   , disSQ   ,
    disLB     , disLH    , disLWL , disLW   , disLBU , disLHU , disLWR  , disLWU  ,
    disSB     , disSH    , disSWL , disSW   , disSDL , disSDR , disSWR  , disCACHE,
    disNULL   , disLWC1  , disNULL, disPREF , disNULL, disNULL, disLQC2 , disLD   ,
    disNULL   , disSWC1  , disNULL, disNULL  , disNULL, disNULL, disSQC2 , disSD  };

void disR5900F( string& output, u32 code )
{
	disR5900[code >> 26]( output, code );
}

// returns a string representation of the cpuRegs current instruction.
// The return value of this method is *not* thread safe!
const string& DisR5900CurrentState::getString()
{
	result.clear();
	disR5900F( result, cpuRegs.code );
	return result;
}

const char* DisR5900CurrentState::getCString()
{
	result.clear();
	disR5900F( result, cpuRegs.code );
	return result.c_str();
}

DisR5900CurrentState disR5900Current;

}
