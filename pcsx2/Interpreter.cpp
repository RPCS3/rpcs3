/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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

#include "Common.h"
#include "R5900.h"
#include "R5900OpcodeTables.h"

#include <float.h>

using namespace R5900;		// for OPCODE and OpcodeImpl

extern int vu0branch, vu1branch;

static int branch2 = 0;
static u32 cpuBlockCycles = 0;		// 3 bit fixed point version of cycle count
static std::string disOut;

// These macros are used to assemble the repassembler functions

static void debugI()
{
	if( !IsDevBuild ) return;
	if( cpuRegs.GPR.n.r0.UD[0] || cpuRegs.GPR.n.r0.UD[1] ) Console::Error("R0 is not zero!!!!");
}

//long int runs=0;

static void execI()
{
	cpuRegs.code = memRead32( cpuRegs.pc );
	if( IsDebugBuild )
		debugI();

	const OPCODE& opcode = GetCurrentInstruction();
	//use this to find out what opcodes your game uses. very slow! (rama)
	//runs++;
	//if (runs > 1599999999){ //leave some time to startup the testgame
	//	if (opcode.Name[0] == 'L') { //find all opcodes beginning with "L"
	//		Console::WriteLn ("Load %s", params opcode.Name);
	//	}
	//}

	// Another method of instruction dumping:
	/*if( cpuRegs.cycle > 0x4f24d714 )
	{
		//CPU_LOG( "%s", disR5900Current.getCString());
		disOut.clear();
		opcode.disasm( disOut );
		disOut += '\n';
		CPU_LOG( disOut.c_str() );
	}*/


	cpuBlockCycles += opcode.cycles;
	cpuRegs.pc += 4;

	opcode.interpret();
}

static bool EventRaised = false;

static __forceinline void _doBranch_shared(u32 tar)
{
	branch2 = cpuRegs.branch = 1;
	execI();

	// branch being 0 means an exception was thrown, since only the exception
	// handler should ever clear it.

	if( cpuRegs.branch != 0 )
	{
		cpuRegs.pc = tar;
		cpuRegs.branch = 0;
	}
}

static void __fastcall doBranch( u32 target )
{
	_doBranch_shared( target );
	cpuRegs.cycle += cpuBlockCycles >> 3;
	cpuBlockCycles &= (1<<3)-1;
	EventRaised |= intEventTest();
}

void __fastcall intDoBranch(u32 target)
{
	//Console::WriteLn("Interpreter Branch ");
	_doBranch_shared( target );

	if( Cpu == &intCpu )
	{
		cpuRegs.cycle += cpuBlockCycles >> 3;
		cpuBlockCycles &= (1<<3)-1;
		EventRaised |= intEventTest();
	}
}

void intSetBranch() {
	branch2 = /*cpuRegs.branch =*/ 1;
}

////////////////////////////////////////////////////////////////////
// R5900 Branching Instructions!
// These are the interpreter versions of the branch instructions.  Unlike other
// types of interpreter instructions which can be called safely from the recompilers,
// these instructions are not "recSafe" because they may not invoke the
// necessary branch test logic that the recs need to maintain sync with the
// cpuRegs.pc and delaySlot instruction and such.

namespace R5900 {
namespace Interpreter {
namespace OpcodeImpl {

/*********************************************************
* Jump to target                                         *
* Format:  OP target                                     *
*********************************************************/

void J()   {
	doBranch(_JumpTarget_);
}

void JAL() {
	_SetLink(31); doBranch(_JumpTarget_);
}

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, rt, offset                             *
*********************************************************/
#define RepBranchi32(op) \
	if (cpuRegs.GPR.r[_Rs_].SD[0] op cpuRegs.GPR.r[_Rt_].SD[0]) doBranch(_BranchTarget_); \
	else intEventTest();


void BEQ() {	RepBranchi32(==) }  // Branch if Rs == Rt
void BNE() {	RepBranchi32(!=) }  // Branch if Rs != Rt

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, offset                                 *
*********************************************************/
#define RepZBranchi32(op) \
	if(cpuRegs.GPR.r[_Rs_].SD[0] op 0) { \
		doBranch(_BranchTarget_); \
	}

#define RepZBranchLinki32(op) \
	_SetLink(31); \
	if(cpuRegs.GPR.r[_Rs_].SD[0] op 0) { \
		doBranch(_BranchTarget_); \
	}

void BGEZ()   { RepZBranchi32(>=) }      // Branch if Rs >= 0
void BGEZAL() { RepZBranchLinki32(>=) }  // Branch if Rs >= 0 and link
void BGTZ()   { RepZBranchi32(>) }       // Branch if Rs >  0
void BLEZ()   { RepZBranchi32(<=) }      // Branch if Rs <= 0
void BLTZ()   { RepZBranchi32(<) }       // Branch if Rs <  0
void BLTZAL() { RepZBranchLinki32(<) }   // Branch if Rs <  0 and link


/*********************************************************
* Register branch logic  Likely                          *
* Format:  OP rs, offset                                 *
*********************************************************/
#define RepZBranchi32Likely(op) \
	if(cpuRegs.GPR.r[_Rs_].SD[0] op 0) { \
		doBranch(_BranchTarget_); \
	} else { cpuRegs.pc +=4; intEventTest(); }

#define RepZBranchLinki32Likely(op) \
	_SetLink(31); \
	if(cpuRegs.GPR.r[_Rs_].SD[0] op 0) { \
		doBranch(_BranchTarget_); \
	} else { cpuRegs.pc +=4; intEventTest(); }

#define RepBranchi32Likely(op) \
	if(cpuRegs.GPR.r[_Rs_].SD[0] op cpuRegs.GPR.r[_Rt_].SD[0]) { \
		doBranch(_BranchTarget_); \
	} else { cpuRegs.pc +=4; intEventTest(); }


void BEQL()    {  RepBranchi32Likely(==)      }  // Branch if Rs == Rt
void BNEL()    {  RepBranchi32Likely(!=)      }  // Branch if Rs != Rt
void BLEZL()   {  RepZBranchi32Likely(<=)     }  // Branch if Rs <= 0
void BGTZL()   {  RepZBranchi32Likely(>)      }  // Branch if Rs >  0
void BLTZL()   {  RepZBranchi32Likely(<)      }  // Branch if Rs <  0
void BGEZL()   {  RepZBranchi32Likely(>=)     }  // Branch if Rs >= 0
void BLTZALL() {  RepZBranchLinki32Likely(<)  }  // Branch if Rs <  0 and link
void BGEZALL() {  RepZBranchLinki32Likely(>=) }  // Branch if Rs >= 0 and link

/*********************************************************
* Register jump                                          *
* Format:  OP rs, rd                                     *
*********************************************************/
void JR()   { 
	doBranch(cpuRegs.GPR.r[_Rs_].UL[0]); 
}

void JALR() { 
	u32 temp = cpuRegs.GPR.r[_Rs_].UL[0];
	if (_Rd_) { _SetLink(_Rd_); }
	doBranch(temp);
}

} } }		// end namespace R5900::Interpreter::OpcodeImpl

////////////////////////////////////////////////////////

void intAlloc()
{
	 // fixme : detect cpu for use the optimize asm code
}

void intReset()
{
	cpuRegs.branch = 0;
	branch2 = 0;
}

bool intEventTest()
{
	// Perform counters, ints, and IOP updates:
	return _cpuBranchTest_Shared();
}

void intExecute()
{
	g_EEFreezeRegs = false;

	// Mem protection should be handled by the caller here so that it can be
	// done in a more optimized fashion.

	EventRaised = false;

	while( !EventRaised )
	{
		execI();
	}
}

static void intExecuteBlock()
{
	g_EEFreezeRegs = false;

	branch2 = 0;
	while (!branch2) execI();
}

static void intStep()
{
	g_EEFreezeRegs = false;
	execI();
}

static void intClear(u32 Addr, u32 Size)
{
}

static void intShutdown() {
}

R5900cpu intCpu = {
	intAlloc,
	intReset,
	intStep,
	intExecute,
	intExecuteBlock,
	intClear,
	intShutdown
};
