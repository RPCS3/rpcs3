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
#include "IopCommon.h"

using namespace R3000A;

// Used to flag delay slot instructions when throwig exceptions.
bool iopIsDelaySlot = false;

static bool branch2 = 0;
static u32 branchPC;

static void doBranch(s32 tar);	// forward declared prototype

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, offset                                 *
*********************************************************/

void psxBGEZ()         // Branch if Rs >= 0
{
	if (_i32(_rRs_) >= 0) doBranch(_BranchTarget_);
}

void psxBGEZAL()   // Branch if Rs >= 0 and link
{
	if (_i32(_rRs_) >= 0)
	{
		_SetLink(31);
		doBranch(_BranchTarget_);
	}
}

void psxBGTZ()          // Branch if Rs >  0
{
	if (_i32(_rRs_) > 0) doBranch(_BranchTarget_);
}

void psxBLEZ()         // Branch if Rs <= 0
{
	if (_i32(_rRs_) <= 0) doBranch(_BranchTarget_);
}
void psxBLTZ()          // Branch if Rs <  0
{
	if (_i32(_rRs_) < 0) doBranch(_BranchTarget_);
}

void psxBLTZAL()    // Branch if Rs <  0 and link
{
	if (_i32(_rRs_) < 0)
		{
			_SetLink(31);
			doBranch(_BranchTarget_);
		}
}

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, rt, offset                             *
*********************************************************/

void psxBEQ()   // Branch if Rs == Rt
{
	if (_i32(_rRs_) == _i32(_rRt_)) doBranch(_BranchTarget_);
}

void psxBNE()   // Branch if Rs != Rt
{
	if (_i32(_rRs_) != _i32(_rRt_)) doBranch(_BranchTarget_);
}

/*********************************************************
* Jump to target                                         *
* Format:  OP target                                     *
*********************************************************/
void psxJ()
{
	// check for iop module import table magic
	u32 delayslot = iopMemRead32(psxRegs.pc);
	if (delayslot >> 16 == 0x2400 && irxImportExec(irxImportLibname(psxRegs.pc), delayslot & 0xffff))
		return;

	doBranch(_JumpTarget_);
}

void psxJAL()
{
	_SetLink(31);
	doBranch(_JumpTarget_);
}

/*********************************************************
* Register jump                                          *
* Format:  OP rs, rd                                     *
*********************************************************/
void psxJR()
{
	doBranch(_u32(_rRs_));
}

void psxJALR()
{
	if (_Rd_)
	{
		_SetLink(_Rd_);
	}
	doBranch(_u32(_rRs_));
}

///////////////////////////////////////////
// These macros are used to assemble the repassembler functions

static __fi void execI()
{
	psxRegs.code = iopMemRead32(psxRegs.pc);

		PSXCPU_LOG("%s", disR3000AF(psxRegs.code, psxRegs.pc));

	psxRegs.pc+= 4;
	psxRegs.cycle++;
	iopCycleEE-=8;

	psxBSC[psxRegs.code >> 26]();
}


static void doBranch(s32 tar) {
	branch2 = iopIsDelaySlot = true;
	branchPC = tar;
	execI();
	PSXCPU_LOG( "\n" );
	iopIsDelaySlot = false;
	psxRegs.pc = branchPC;

	iopEventTest();
}

static void intReserve() {
}

static void intAlloc() {
}

static void intReset() {
	intAlloc();
}

static void intExecute() {
	for (;;) execI();
}

static s32 intExecuteBlock( s32 eeCycles )
{
	iopBreak = 0;
	iopCycleEE = eeCycles;

	while (iopCycleEE > 0){
		branch2 = 0;
		while (!branch2) {
			execI();
        }
	}
	return iopBreak + iopCycleEE;
}

static void intClear(u32 Addr, u32 Size) {
}

static void intShutdown() {
}

R3000Acpu psxInt = {
	intReserve,
	intReset,
	intExecute,
	intExecuteBlock,
	intClear,
	intShutdown
};
