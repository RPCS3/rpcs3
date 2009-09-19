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


// Holds instruction tables for the r5900 recompiler

#include "PrecompiledHeader.h"

#include "Common.h"
#include "Memory.h"
#include "R5900OpcodeTables.h"
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
#include "iCOP0.h"

// Use this to call into interpreter functions that require an immediate branchtest
// to be done afterward (anything that throws an exception or enables interrupts, etc).
void recBranchCall( void (*func)() )
{
	// In order to make sure a branch test is performed, the nextBranchCycle is set
	// to the current cpu cycle.

	MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
	MOV32MtoR( EAX, (uptr)&cpuRegs.cycle );
	MOV32ItoM( (uptr)&cpuRegs.pc, pc );
	MOV32RtoM( (uptr)&g_nextBranchCycle, EAX );

	// Might as well flush everything -- it'll all get flushed when the
	// recompiler inserts the branchtest anyway.
	iFlushCall(FLUSH_EVERYTHING);
	CALLFunc( (uptr)func );
	branch = 2;
}

void recCall( void (*func)(), int delreg )
{
	MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (uptr)&cpuRegs.pc, pc );

	iFlushCall(FLUSH_EVERYTHING);
	if( delreg > 0 ) _deleteEEreg(delreg, 0);
	CALLFunc( (uptr)func );
}

using namespace R5900::Dynarec::OpcodeImpl;


////////////////////////////////////////////////
// Back-Prob Function Tables - Gathering Info //
////////////////////////////////////////////////
BSCPropagate::BSCPropagate( EEINST& previous, EEINST& pinstance ) :
	prev( previous )
,	pinst( pinstance )
{
}

void BSCPropagate::rpropSetRead( int reg, int mask )
{
	if( !(pinst.regs[reg] & EEINST_USED) )
		pinst.regs[reg] |= EEINST_LASTUSE;
	prev.regs[reg] |= EEINST_LIVE0|(mask)|EEINST_USED;
	pinst.regs[reg] |= EEINST_USED;
	if( reg ) pinst.info = ((mask)&(EEINST_MMX|EEINST_XMM));
	_recFillRegister(pinst, XMMTYPE_GPRREG, reg, 0);
}

template< int live >
void BSCPropagate::rpropSetWrite0( int reg, int mask )
{
	prev.regs[reg] &= ~((mask)|live|EEINST_XMM|EEINST_MMX);
	if( !(pinst.regs[reg] & EEINST_USED) )
		pinst.regs[reg] |= EEINST_LASTUSE;
	pinst.regs[reg] |= EEINST_USED;
	prev.regs[reg] |= EEINST_USED;
	_recFillRegister(pinst, XMMTYPE_GPRREG, reg, 1);
}

__forceinline void BSCPropagate::rpropSetWrite( int reg, int mask )
{
	rpropSetWrite0<EEINST_LIVE0>( reg, mask );
}

__forceinline void BSCPropagate::rpropSetFast( int write1, int read1, int read2, int mask )
{
	if( write1 ) { rpropSetWrite(write1,mask); }
	if( read1 ) { rpropSetRead(read1,mask); }
	if( read2 ) { rpropSetRead(read2,mask); }
}

template< int lo, int hi >
void BSCPropagate::rpropSetLOHI( int write1, int read1, int read2, int mask )
{
	if( write1 ) { rpropSetWrite(write1,mask); }
	if( lo & MODE_WRITE ) { rpropSetWrite(XMMGPR_LO,mask); }
	if( hi & MODE_WRITE ) { rpropSetWrite(XMMGPR_HI,mask); }
	if( read1 ) { rpropSetRead(read1,mask); }
	if( read2 ) { rpropSetRead(read2,mask); }
	if( lo & MODE_READ ) { rpropSetRead(XMMGPR_LO,mask); }
	if( hi & MODE_READ ) { rpropSetRead(XMMGPR_HI,mask); }
}

// FPU regs
void BSCPropagate::rpropSetFPURead( int reg, int mask )
{
	if( !(pinst.fpuregs[reg] & EEINST_USED) )
		pinst.fpuregs[reg] |= EEINST_LASTUSE;
	prev.fpuregs[reg] |= EEINST_LIVE0|(mask)|EEINST_USED;
	pinst.fpuregs[reg] |= EEINST_USED;
	if( reg ) pinst.info = ((mask)&(EEINST_MMX|EEINST_XMM));
	if( reg == XMMFPU_ACC ) _recFillRegister(pinst, XMMTYPE_FPACC, 0, 0);
	else _recFillRegister(pinst, XMMTYPE_FPREG, reg, 0);
}

template< int live >
void BSCPropagate::rpropSetFPUWrite0( int reg, int mask )
{
	prev.fpuregs[reg] &= ~((mask)|live|EEINST_XMM|EEINST_MMX);
	if( !(pinst.fpuregs[reg] & EEINST_USED) )
		pinst.fpuregs[reg] |= EEINST_LASTUSE;
	pinst.fpuregs[reg] |= EEINST_USED;
	prev.fpuregs[reg] |= EEINST_USED;
	if( reg == XMMFPU_ACC ) _recFillRegister(pinst, XMMTYPE_FPACC, 0, 1);
	else _recFillRegister(pinst, XMMTYPE_FPREG, reg, 1);
}

__forceinline void BSCPropagate::rpropSetFPUWrite( int reg, int mask )
{
	rpropSetFPUWrite0<EEINST_LIVE0>( reg, mask );
}


#define EEINST_REALXMM EEINST_XMM

//SLL,  NULL,  SRL,  SRA,  SLLV,    NULL,  SRLV,   SRAV,
//JR,   JALR,  MOVZ, MOVN, SYSCALL, BREAK, NULL,   SYNC,
//MFHI, MTHI,  MFLO, MTLO, DSLLV,   NULL,  DSRLV,  DSRAV,
//MULT, MULTU, DIV,  DIVU, NULL,    NULL,  NULL,   NULL,
//ADD,  ADDU,  SUB,  SUBU, AND,     OR,    XOR,    NOR,
//MFSA, MTSA,  SLT,  SLTU, DADD,    DADDU, DSUB,   DSUBU,
//TGE,  TGEU,  TLT,  TLTU, TEQ,     NULL,  TNE,    NULL,
//DSLL, NULL,  DSRL, DSRA, DSLL32,  NULL,  DSRL32, DSRA32

__forceinline void BSCPropagate::rpropSPECIAL()
{
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
			pinst.info |= EEINST_MMX;
			_recFillRegister(pinst, XMMTYPE_GPRREG, _Rd_, 1);
			break;

		case 12: // syscall
		case 13: // break
			_recClearInst(&prev);
			prev.info = 0;
			break;
		case 15: // sync
			break;

		case 16: // mfhi
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(XMMGPR_HI, (pinst.regs[_Rd_]&(EEINST_MMX|EEINST_REALXMM))|EEINST_LIVE1);
			break;
		case 17: // mthi
			rpropSetWrite(XMMGPR_HI, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1);
			break;
		case 18: // mflo
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(XMMGPR_LO, (pinst.regs[_Rd_]&(EEINST_MMX|EEINST_REALXMM))|EEINST_LIVE1);
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

		//case 24: // mult
		//	// can do unsigned mult only if HI isn't used

		//	//using this allocation for temp causes the emu to crash
		//	//temp = (pinst.regs[XMMGPR_HI]&(EEINST_LIVE0|EEINST_LIVE1))?0:EEINST_MMX;
		//	temp = 0;
		//	rpropSetWrite(XMMGPR_LO, EEINST_LIVE1);
		//	rpropSetWrite(XMMGPR_HI, EEINST_LIVE1);
		//	rpropSetWrite(_Rd_, EEINST_LIVE1);
		//
		//	// fixme - temp is always 0, so I doubt the next three lines are right. (arcum42)

		//	// Yep, its wrong. Using always 0 causes the wrong damage calculations in Soul Nomad.
		//	rpropSetRead(_Rs_, temp);
		//	rpropSetRead(_Rt_, temp);
		//	pinst.info |= temp;
		//	break;

		//case 25: // multu
		//	rpropSetWrite(XMMGPR_LO, EEINST_LIVE1);
		//	rpropSetWrite(XMMGPR_HI, EEINST_LIVE1);
		//	rpropSetWrite(_Rd_, EEINST_LIVE1);
		//	rpropSetRead(_Rs_, EEINST_MMX);
		//	rpropSetRead(_Rt_, EEINST_MMX);
		//	pinst.info |= EEINST_MMX;
		//	break;

		
		case 24: // mult
		case 25: // multu
			rpropSetWrite(XMMGPR_LO, EEINST_LIVE1);
			rpropSetWrite(XMMGPR_HI, EEINST_LIVE1);
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_USED);  //EEINST_MMX crashes on init, EEINST_XMM fails on Tales of Abyss, so EEINST_USED (rama)
			rpropSetRead(_Rt_, EEINST_USED);
			// did we ever try EEINST_LIVE1 or 0 in the place of EEINST_USED? I think of those might be more correct here (Air)

			pinst.info |= EEINST_USED;
			break;

		case 26: // div
		case 27: // divu
			rpropSetWrite(XMMGPR_LO, EEINST_LIVE1);
			rpropSetWrite(XMMGPR_HI, EEINST_LIVE1);
			rpropSetRead(_Rs_, 0);
			rpropSetRead(_Rt_, 0);
			//pinst.info |= EEINST_REALXMM|EEINST_MMX;
			break;

		case 32: // add
		case 33: // addu
		case 34: // sub
		case 35: // subu
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			if( _Rs_ ) rpropSetRead(_Rs_, EEINST_LIVE1);
			if( _Rt_ ) rpropSetRead(_Rt_, EEINST_LIVE1);
			pinst.info |= EEINST_MMX;
			break;

		case 36: // and
		case 37: // or
		case 38: // xor
		case 39: // nor
			// if rd == rs or rt, keep live1
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rs_, (pinst.regs[_Rd_]&EEINST_LIVE1)|EEINST_MMX);
			rpropSetRead(_Rt_, (pinst.regs[_Rd_]&EEINST_LIVE1)|EEINST_MMX);
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
			pinst.info |= EEINST_MMX;
			break;

		case 43: // sltu
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1);
			rpropSetRead(_Rt_, EEINST_LIVE1);
			pinst.info |= EEINST_MMX;
			break;

		case 44: // dadd
		case 45: // daddu
		case 46: // dsub
		case 47: // dsubu
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			if( _Rs_ == 0 || _Rt_ == 0 ) {
				// just a copy, so don't force mmx
				rpropSetRead(_Rs_, (pinst.regs[_Rd_]&EEINST_LIVE1));
				rpropSetRead(_Rt_, (pinst.regs[_Rd_]&EEINST_LIVE1));
			}
			else {
				rpropSetRead(_Rs_, (pinst.regs[_Rd_]&EEINST_LIVE1)|EEINST_MMX);
				rpropSetRead(_Rt_, (pinst.regs[_Rd_]&EEINST_LIVE1)|EEINST_MMX);
			}
			pinst.info |= EEINST_MMX;
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
			rpropSetRead(_Rt_, EEINST_LIVE1|EEINST_MMX);
			pinst.info |= EEINST_MMX;
			break;

		case 60: // dsll32
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rt_, EEINST_MMX);
			pinst.info |= EEINST_MMX;
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
__forceinline void BSCPropagate::rpropREGIMM()
{
	switch(_Rt_) {
		case 0: // bltz
		case 1: // bgez
			rpropSetRead(_Rs_, EEINST_LIVE1);
			pinst.info |= EEINST_MMX;
			pinst.info |= EEINST_REALXMM;
			break;

		case 2: // bltzl
		case 3: // bgezl
			// reset since don't know which path to go
			_recClearInst(&prev);
			prev.info = 0;
			rpropSetRead(_Rs_, EEINST_LIVE1);
			pinst.info |= EEINST_MMX;
			pinst.info |= EEINST_REALXMM;
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
			_recClearInst(&prev);
			prev.info = 0;
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
__forceinline void BSCPropagate::rpropCP0()
{
	switch(_Rs_) {
		case 0: // mfc0
			rpropSetWrite(_Rt_, EEINST_LIVE1|EEINST_REALXMM);
			break;
		case 4:
			rpropSetRead(_Rt_, EEINST_LIVE1|EEINST_REALXMM);
			break;
		case 8: // cop0bc0
			_recClearInst(&prev);
			prev.info = 0;
			break;
		case 16: // cop0c0
			_recClearInst(&prev);
			prev.info = 0;
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
__forceinline void BSCPropagate::rpropCP1()
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
			_recClearInst(&prev);
			prev.info = 0;
			break;
		case 16:
			// floating point ops
			pinst.info |= EEINST_REALXMM;
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

__forceinline void BSCPropagate::rpropCP2()
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
			pinst.info |= EEINSTINFO_COP2;
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
__forceinline void BSCPropagate::rpropMMI()
{
	switch(cpuRegs.code&0x3f) {
		case 0: // madd
		case 1: // maddu
			rpropSetLOHI<MODE_READ|MODE_WRITE, MODE_READ|MODE_WRITE>(_Rd_, _Rs_, _Rt_, EEINST_LIVE1);
			break;
		case 4: // plzcw
			rpropSetFast(_Rd_, _Rs_, 0, EEINST_LIVE1);
			break;
		case 8: rpropMMI0(); break;
		case 9: rpropMMI2(); break;

		case 16: // mfhi1
		{
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			int temp = ((pinst.regs[_Rd_]&(EEINST_MMX|EEINST_REALXMM))?EEINST_MMX:EEINST_REALXMM);
			rpropSetRead(XMMGPR_HI, temp|EEINST_LIVE2);
			break;
		}
		case 17: // mthi1
			rpropSetWrite0<0>(XMMGPR_HI, EEINST_LIVE2);
			rpropSetRead(_Rs_, EEINST_LIVE1);
			break;
		case 18: // mflo1
		{
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			int temp = ((pinst.regs[_Rd_]&(EEINST_MMX|EEINST_REALXMM))?EEINST_MMX:EEINST_REALXMM);
			rpropSetRead(XMMGPR_LO, temp|EEINST_LIVE2);
			break;
		}
		case 19: // mtlo1
			rpropSetWrite0<0>(XMMGPR_LO, EEINST_LIVE2);
			rpropSetRead(_Rs_, EEINST_LIVE1);
			break;

		case 24: // mult1
		{
			int temp = (pinst.regs[XMMGPR_HI]&(EEINST_LIVE2))?0:EEINST_MMX;
			rpropSetWrite0<0>(XMMGPR_LO, EEINST_LIVE2);
			rpropSetWrite0<0>(XMMGPR_HI, EEINST_LIVE2);
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rs_, temp);
			rpropSetRead(_Rt_, temp);
			pinst.info |= temp;
			break;
		}
		case 25: // multu1
			rpropSetWrite0<0>(XMMGPR_LO, EEINST_LIVE2);
			rpropSetWrite0<0>(XMMGPR_HI, EEINST_LIVE2);
			rpropSetWrite(_Rd_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_MMX);
			rpropSetRead(_Rt_, EEINST_MMX);
			pinst.info |= EEINST_MMX;
			break;

		case 26: // div1
		case 27: // divu1
			rpropSetWrite0<0>(XMMGPR_LO, EEINST_LIVE2);
			rpropSetWrite0<0>(XMMGPR_HI, EEINST_LIVE2);
			rpropSetRead(_Rs_, 0);
			rpropSetRead(_Rt_, 0);
			//pinst.info |= EEINST_REALXMM|EEINST_MMX;
			break;

		case 32: // madd1
		case 33: // maddu1
			rpropSetWrite0<0>(XMMGPR_LO, EEINST_LIVE2);
			rpropSetWrite0<0>(XMMGPR_HI, EEINST_LIVE2);
			rpropSetFast(_Rd_, _Rs_, _Rt_, EEINST_LIVE1);
			rpropSetRead(XMMGPR_LO, EEINST_LIVE2);
			rpropSetRead(XMMGPR_HI, EEINST_LIVE2);
			break;

		case 40: rpropMMI1(); break;
		case 41: rpropMMI3(); break;

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
__forceinline void BSCPropagate::rpropMMI0()
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

__forceinline void BSCPropagate::rpropMMI1()
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

__forceinline void BSCPropagate::rpropMMI2()
{
	switch((cpuRegs.code>>6)&0x1f) {
		case 0: // pmaddw
		case 4: // pmsubw
			rpropSetLOHI<MODE_READ|MODE_WRITE, MODE_READ|MODE_WRITE>
				(_Rd_, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1);
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
			rpropSetLOHI<MODE_WRITE, MODE_WRITE>
				(_Rd_, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1);
			break;
		case 13: // pdivw
			rpropSetLOHI<MODE_WRITE, MODE_WRITE>
				(0, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1);
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
			rpropSetLOHI<MODE_READ|MODE_WRITE, MODE_READ|MODE_WRITE>
				(_Rd_, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM);
			break;

		case 26: // pexeh
		case 27: // prevh
		case 30: // pexew
		case 31: // prot3w
			rpropSetWrite(_Rd_, EEINST_LIVE2|EEINST_LIVE1);
			rpropSetRead(_Rt_, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM);
			break;

		case 28: // pmulth
			rpropSetLOHI<MODE_WRITE, MODE_WRITE>
				(_Rd_, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM);
			break;
		case 29: // pdivbw
			rpropSetLOHI<MODE_WRITE, MODE_WRITE>
				(_Rd_, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM);
			break;

		default:
			rpropSetFast(_Rd_, _Rs_, _Rt_, EEINST_LIVE1|EEINST_LIVE2|EEINST_REALXMM);
			break;
	}
}

__forceinline void BSCPropagate::rpropMMI3()
{
	switch((cpuRegs.code>>6)&0x1f) {
		case 0: // pmadduw
			rpropSetLOHI<MODE_READ|MODE_WRITE, MODE_READ|MODE_WRITE>
				(_Rd_, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM );
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
			rpropSetLOHI<MODE_WRITE, MODE_WRITE>
				(_Rd_, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1|EEINST_REALXMM);
			break;
		case 13: // pdivuw
			rpropSetLOHI<MODE_WRITE, MODE_WRITE>
				(0, _Rs_, _Rt_, EEINST_LIVE2|EEINST_LIVE1);
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

//SPECIAL, REGIMM, J,    JAL,   BEQ,  BNE,  BLEZ,  BGTZ,
//ADDI,    ADDIU,  SLTI, SLTIU, ANDI, ORI,  XORI,  LUI,
//COP0,    COP1,   COP2, NULL,  BEQL, BNEL, BLEZL, BGTZL,
//DADDI,   DADDIU, LDL,  LDR,   MMI,  NULL, LQ,    SQ,
//LB,      LH,     LWL,  LW,    LBU,  LHU,  LWR,   LWU,
//SB,      SH,     SWL,  SW,    SDL,  SDR,  SWR,   CACHE,
//NULL,    LWC1,   NULL, PREF,  NULL, NULL, LQC2,  LD,
//NULL,    SWC1,   NULL, NULL,  NULL, NULL, SQC2,  SD
void BSCPropagate::rprop()
{
	switch(cpuRegs.code >> 26) {
		case 0: rpropSPECIAL(); break;
		case 1: rpropREGIMM(); break;
		case 2: // j
			break;
		case 3: // jal
			rpropSetWrite(31, EEINST_LIVE1);
			break;
		case 4: // beq
		case 5: // bne
			rpropSetRead(_Rs_, EEINST_LIVE1);
			rpropSetRead(_Rt_, EEINST_LIVE1);
			pinst.info |= EEINST_REALXMM|EEINST_MMX;
			break;

		case 20: // beql
		case 21: // bnel
			// reset since don't know which path to go
			_recClearInst(&prev);
			prev.info = 0;
			rpropSetRead(_Rs_, EEINST_LIVE1);
			rpropSetRead(_Rt_, EEINST_LIVE1);
			pinst.info |= EEINST_REALXMM|EEINST_MMX;
			break;

		case 6: // blez
		case 7: // bgtz
			rpropSetRead(_Rs_, EEINST_LIVE1);
			break;

		case 22: // blezl
		case 23: // bgtzl
			// reset since don't know which path to go
			_recClearInst(&prev);
			prev.info = 0;
			rpropSetRead(_Rs_, EEINST_LIVE1);
			break;

		case 24: // daddi
		case 25: // daddiu
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1 | (_Rs_!=0?EEINST_MMX:0)); // This looks like what ZeroFrog wanted; ToDo: Needs checking! (cottonvibes)
			break;

		case 8: // addi
		case 9: // addiu
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1);
			pinst.info |= EEINST_MMX;
			break;

		case 10: // slti
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1);
			pinst.info |= EEINST_MMX;
			break;
		case 11: // sltiu
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1);
			pinst.info |= EEINST_MMX;
			break;

		case 12: // andi
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rs_, (_Rs_!=_Rt_?EEINST_MMX:0));
			pinst.info |= EEINST_MMX;
			break;
		case 13: // ori
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1|(_Rs_!=_Rt_?EEINST_MMX:0));
			pinst.info |= EEINST_MMX;
			break;
		case 14: // xori
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			rpropSetRead(_Rs_, EEINST_LIVE1|(_Rs_!=_Rt_?EEINST_MMX:0));
			pinst.info |= EEINST_MMX;
			break;

		case 15: // lui
			rpropSetWrite(_Rt_, EEINST_LIVE1);
			break;

		case 16: rpropCP0(); break;
		case 17: rpropCP1(); break;
		case 18: rpropCP2(); break;

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

		case 28: rpropMMI(); break;

		case 30: // lq
			rpropSetWrite(_Rt_, EEINST_LIVE1|EEINST_LIVE2);
			rpropSetRead(_Rs_, 0);
			pinst.info |= EEINST_MMX;
			pinst.info |= EEINST_REALXMM;
			break;

		case 31: // sq
			rpropSetRead(_Rt_, EEINST_LIVE1|EEINST_LIVE2|EEINST_REALXMM);
			rpropSetRead(_Rs_, 0);
			pinst.info |= EEINST_MMX;
			pinst.info |= EEINST_REALXMM;
			break;

		// 4 byte stores
		case 40: case 41: case 42: case 43: case 46:
			rpropSetRead(_Rt_, 0);
			rpropSetRead(_Rs_, 0);
			pinst.info |= EEINST_MMX;
			pinst.info |= EEINST_REALXMM;
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
}	// End namespace OpcodeImpl
