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

// Micro VU recompiler! - author: cottonvibes(@gmail.com)

#include "PrecompiledHeader.h"
#include "Common.h"
#include "VU.h"
#include "GS.h"
#include "iR5900.h"
#include "R5900OpcodeTables.h"
#include "microVU.h"

//------------------------------------------------------------------
// Micro VU - Global Variables
//------------------------------------------------------------------

__aligned16 microVU microVU0;
__aligned16 microVU microVU1;

const __aligned(32) mVU_Globals mVUglob = {
	__four(0x7fffffff),		  // absclip
	__four(0x80000000),		  // signbit
	__four(0xff7fffff),		  // minvals 
	__four(0x7f7fffff),		  // maxvals
	__four(0x3f800000),		  // ONE!
	__four(0x3f490fdb),		  // PI4!
	__four(0x3f7ffff5),		  // T1
	__four(0xbeaaa61c),		  // T5
	__four(0x3e4c40a6),		  // T2
	__four(0xbe0e6c63),		  // T3
	__four(0x3dc577df),		  // T4
	__four(0xbd6501c4),		  // T6
	__four(0x3cb31652),		  // T7
	__four(0xbb84d7e7),		  // T8
	__four(0xbe2aaaa4),		  // S2 
	__four(0x3c08873e),		  // S3
	__four(0xb94fb21f),		  // S4
	__four(0x362e9c14),		  // S5
	__four(0x3e7fffa8),		  // E1
	__four(0x3d0007f4),		  // E2
	__four(0x3b29d3ff),		  // E3 
	__four(0x3933e553),		  // E4
	__four(0x36b63510),		  // E5
	__four(0x353961ac),		  // E6
	__four(16.0),			  // FTOI_4 
	__four(4096.0),			  // FTOI_12 
	__four(32768.0),		  // FTOI_15
	__four(0.0625f),		  // ITOF_4 
	__four(0.000244140625),	  // ITOF_12 
	__four(0.000030517578125) // ITOF_15
};

//------------------------------------------------------------------
// Micro VU - Main Functions
//------------------------------------------------------------------

_f void mVUthrowHardwareDeficiency(const wxChar* extFail, int vuIndex) {
	throw Exception::HardwareDeficiency(
		wxsFormat( L"microVU%d recompiler init failed: %s is not available.", vuIndex, extFail),
		wxsFormat(_("%s Extensions not found.  microVU requires a host CPU with MMX, SSE, and SSE2 extensions."), extFail )
	);
}

// Only run this once per VU! ;)
_f void mVUinit(VURegs* vuRegsPtr, int vuIndex) {

	if(!x86caps.hasMultimediaExtensions)		mVUthrowHardwareDeficiency( L"MMX", vuIndex );
	if(!x86caps.hasStreamingSIMDExtensions)		mVUthrowHardwareDeficiency( L"SSE", vuIndex );
	if(!x86caps.hasStreamingSIMD2Extensions)	mVUthrowHardwareDeficiency( L"SSE2", vuIndex );

	microVU* mVU = mVUx;
	memset(&mVU->prog, 0, sizeof(mVU->prog));

	mVU->regs			= vuRegsPtr;
	mVU->index			= vuIndex;
	mVU->cop2			= 0;
	mVU->vuMemSize		= (vuIndex ? 0x4000 : 0x1000);
	mVU->microMemSize	= (vuIndex ? 0x4000 : 0x1000);
	mVU->progSize		= (vuIndex ? 0x4000 : 0x1000) / 4;
	mVU->cache			= NULL;
	mVU->cacheSize		= mVUcacheSize;
	mVU->regAlloc		= new microRegAlloc(mVU->regs);

	// Give SysMmapEx a NULL and let the OS pick the memory for us: mVU can work with any
	// address the operating system gives us, and unlike the EE/IOP there's not much convenience
	// to debugging if the address is known anyway due to mVU's complex memory layouts (caching).

	mVU->cache = SysMmapEx(NULL, mVU->cacheSize + 0x1000, 0, (vuIndex ? "Micro VU1" : "Micro VU0"));
	if (!mVU->cache) throw Exception::OutOfMemory( "microVU Error: Failed to allocate recompiler memory!" );
	
	memset(mVU->cache, 0xcc, mVU->cacheSize + 0x1000);

	for (u32 i = 0; i < (mVU->progSize / 2); i++) {
		mVU->prog.prog[i].list = new deque<microProgram*>();
	}

	// Setup Entrance/Exit Points
	x86SetPtr(mVU->cache);
	mVUdispatcherA(mVU);
	mVUdispatcherB(mVU);
	mVUemitSearch();
	mVUreset(mVU);
}

// Resets Rec Data
_f void mVUreset(mV) {

	// Clear All Program Data
	//memset(&mVU->prog, 0, sizeof(mVU->prog));
	memset(&mVU->prog.lpState, 0, sizeof(mVU->prog.lpState));

	// Program Variables
	mVU->prog.cleared	=  1;
	mVU->prog.isSame	= -1;
	mVU->prog.cur		= NULL;
	mVU->prog.total		= -1;
	mVU->prog.curFrame	=  0;

	// Setup Dynarec Cache Limits for Each Program
	u8* z = (mVU->cache + 0x1000); // Dispatcher Code is in first page of cache
	mVU->prog.x86start	= z;
	mVU->prog.x86ptr	= z;
	mVU->prog.x86end	= (u8*)((uptr)z + (uptr)(mVU->cacheSize - (_1mb * 3))); // 3mb "Safe Zone"

	for (u32 i = 0; i < (mVU->progSize / 2); i++) {
		mVU->prog.prog[i].list->clear();
		mVU->prog.quick[i].block = NULL;
		mVU->prog.quick[i].prog  = NULL;
	}
}

// Free Allocated Resources
_f void mVUclose(mV) {

	if (mVU->cache) { HostSys::Munmap(mVU->cache, mVU->cacheSize); mVU->cache = NULL; }

	// Delete Programs and Block Managers
	for (u32 i = 0; i < (mVU->progSize / 2); i++) {
		deque<microProgram*>::iterator it = mVU->prog.prog[i].list->begin();
		for ( ; it != mVU->prog.prog[i].list->end(); it++) {
			safe_aligned_free(it[0]);
		}
		safe_delete(mVU->prog.prog[i].list);
	}

	safe_delete(mVU->regAlloc);
}

// Clears Block Data in specified range
_f void mVUclear(mV, u32 addr, u32 size) {
	if (!mVU->prog.cleared) {
		memzero(mVU->prog.lpState); // Clear pipeline state
		mVU->prog.cleared = 1;		// Next execution searches/creates a new microprogram
		for (u32 i = 0; i < (mVU->progSize / 2); i++) {
			mVU->prog.quick[i].block = NULL; // Clear current quick-reference block
			mVU->prog.quick[i].prog  = NULL; // Clear current quick-reference prog
		}
	}
}

//------------------------------------------------------------------
// Micro VU - Private Functions
//------------------------------------------------------------------

// Clears program data
_mVUt _f void mVUclearProg(microProgram& program, bool deleteBlocks) {
	microVU* mVU = mVUx;
	program.used		= 0;
	program.age			= isDead;
	program.frame		= mVU->prog.curFrame;
	program.startPC		= 0x7fffffff;
	for (int j = 0; j <= program.ranges.max; j++) {
		program.ranges.range[j][0]	= -1; // Set range to 
		program.ranges.range[j][1]	= -1; // indeterminable status
		program.ranges.total		= -1;
	}
	for (u32 i = 0; i < (mVU->progSize / 2); i++) {
		if (deleteBlocks) safe_delete(program.block[i]);
	}
}

// Creates a new Micro Program
_mVUt _f microProgram* mVUcreateProg(int startPC) {
	microVU* mVU = mVUx;
	microProgram* prog = (microProgram*)_aligned_malloc(sizeof(microProgram), 64);
	memzero_ptr<sizeof(microProgram)>(prog);
	mVUclearProg<vuIndex>(*prog);
	prog->age	  = isYoung;
	prog->used	  = 1;
	prog->idx	  = mVU->prog.total++;
	prog->startPC = startPC;
	mVUcacheProg<vuIndex>(*prog); // Cache Micro Program
	float cacheSize = (float)(u32)((u32)mVU->prog.x86end - (u32)mVU->prog.x86start);
	float cacheStat =((float)(u32)((u32)mVU->prog.x86ptr - (u32)mVU->prog.x86start)) / cacheSize * 100;
	ConsoleColors c = vuIndex ? Color_Orange : Color_Magenta;
	Console.WriteLn(c, "microVU%d: Cached MicroPrograms = [%03d] [PC=%04x] [List=%02d] (Cache = %f%%)", 
					vuIndex, mVU->prog.total, startPC, mVU->prog.prog[startPC].list->size()+1, cacheStat);
	return prog;
}

// Caches Micro Program
_mVUt _f void mVUcacheProg(microProgram& prog) {
	microVU* mVU = mVUx;
	if (!vuIndex) memcpy_const(prog.data, mVU->regs->Micro, 0x1000);
	else		  memcpy_const(prog.data, mVU->regs->Micro, 0x4000);
	mVUdumpProg(prog);
}

// Finds and Ages/Kills Programs if they haven't been used in a while.
_f void mVUvsyncUpdate(mV) {
	/*for (int i = 0; i <= mVU->prog.max; i++) {
		if (mVU->prog.prog[i].age == isDead) continue;
		if (mVU->prog.prog[i].used) {
			mVU->prog.prog[i].used  = 0;
			mVU->prog.prog[i].frame = mVU->prog.curFrame;
		}
		else { // Age Micro Program that wasn't used
			s32  diff  = mVU->prog.curFrame - mVU->prog.prog[i].frame;
			if	(diff >= (60 * 1)) {
				if (i == mVU->prog.cur) continue; // Don't Age/Kill last used program
				mVU->prog.total--;
				if (!mVU->index) mVUclearProg<0>(i);
				else			 mVUclearProg<1>(i);
				DevCon.WriteLn("microVU%d: Killing Dead Program [%03d]", mVU->index, i+1);
			}
			//elif(diff >= (60  *  1)) { mVU->prog.prog[i].age = isOld;  }
			//elif(diff >= (20  *  1)) { mVU->prog.prog[i].age = isAged; }
		}
	}
	mVU->prog.curFrame++;*/
}

_mVUt _f bool mVUcmpPartial(microProgram& prog) {
	microVU* mVU = mVUx;
	for (int i = 0; i <= prog.ranges.total; i++) {
		if((prog.ranges.range[i][0] < 0)
		|| (prog.ranges.range[i][1] < 0))  { DevCon.Error("microVU%d: Negative Range![%d][%d]", mVU->index, i, prog.ranges.total); }
		if (memcmp_mmx(cmpOffset(prog.data), cmpOffset(mVU->regs->Micro), ((prog.ranges.range[i][1] + 8) - prog.ranges.range[i][0]))) {
			return 0;
		}
	}
	return 1;
}

// Compare Cached microProgram to mVU->regs->Micro
_mVUt _f bool mVUcmpProg(microProgram& prog, const bool cmpWholeProg) {
	microVU* mVU = mVUx;
	if (prog.age == isDead) return 0;
	if ((cmpWholeProg && !memcmp_mmx((u8*)prog.data, mVU->regs->Micro, mVU->microMemSize))
	|| (!cmpWholeProg && mVUcmpPartial<vuIndex>(prog))) {
		mVU->prog.cleared = 0;
		mVU->prog.cur	  =&prog;
		mVU->prog.isSame  = cmpWholeProg ? 1 : -1;
		prog.used = 1;
		prog.age  = isYoung;
		return 1;
	}
	return 0;
}

// Searches for Cached Micro Program and sets prog.cur to it (returns entry-point to program)
_mVUt _f void* mVUsearchProg(u32 startPC, uptr pState) {
	microVU* mVU = mVUx;
	microProgramQuick& quick = mVU->prog.quick[startPC/8];
	microProgramList&  list  = mVU->prog.prog [startPC/8];
	if(!quick.prog) { // If null, we need to search for new program
		deque<microProgram*>::iterator it = list.list->begin();
		for ( ; it != list.list->end(); it++) {
			if (mVUcmpProg<vuIndex>(*it[0], 0)) { 
				quick.block = it[0]->block[startPC/8];
				quick.prog  = it[0];
				list.list->erase(it);
				list.list->push_front(quick.prog);
				return mVUentryGet(mVU, quick.block, startPC, pState);
			}
		}
		mVU->prog.cur = mVUcreateProg<vuIndex>(startPC/8); // If cleared and program not found, make a new program instance
		mVU->prog.cleared	=  0;
		mVU->prog.isSame	=  1;
		mVUcurProg.startPC  =  startPC / 8;
		void* entryPoint	=  mVUblockFetch(mVU, startPC, pState);
		quick.block			=  mVUcurProg.block[startPC/8];
		quick.prog			= &mVUcurProg;
		list.list->push_front(&mVUcurProg);
		return entryPoint;
	}
	mVU->prog.isSame	 = -1;
	mVU->prog.cur		 =  quick.prog;
	//mVU->prog.cur->used  =  1;
	//mVU->prog.cur->age   =  isYoung;
	return mVUentryGet(mVU, quick.block, startPC, pState); // If list.quick, then we've already found and recompiled the program ;)
}

//------------------------------------------------------------------
// recMicroVU0 / recMicroVU1
//------------------------------------------------------------------

static u32 mvu0_allocated = 0;
static u32 mvu1_allocated = 0;

recMicroVU0::recMicroVU0()		  { m_Idx = 0; IsInterpreter = false; }
recMicroVU1::recMicroVU1()		  { m_Idx = 1; IsInterpreter = false; }
void recMicroVU0::Vsync() throw() { mVUvsyncUpdate(&microVU0); }
void recMicroVU1::Vsync() throw() { mVUvsyncUpdate(&microVU1); }

void recMicroVU0::Allocate() {
	if(!m_AllocCount) {
		m_AllocCount++;
		if (AtomicExchange(mvu0_allocated, 1) == 0) 
			mVUinit(&VU0, 0);
	}
}
void recMicroVU1::Allocate() {
	if(!m_AllocCount) {
		m_AllocCount++;
		if (AtomicExchange(mvu1_allocated, 1) == 0) 
			mVUinit(&VU1, 1);
	}
}

void recMicroVU0::Shutdown() throw() {
	if (m_AllocCount > 0) {
		m_AllocCount--;
		if (AtomicExchange(mvu0_allocated, 0) == 1)
			mVUclose(&microVU0);
	}
}
void recMicroVU1::Shutdown() throw() {
	if (m_AllocCount > 0) {
		m_AllocCount--;
		if (AtomicExchange(mvu1_allocated, 0) == 1)
			mVUclose(&microVU1);
	}
}

void recMicroVU0::Reset() {
	if(!pxAssertDev(m_AllocCount, "MicroVU0 CPU Provider has not been allocated prior to reset!")) return;
	mVUreset(&microVU0);
}
void recMicroVU1::Reset() {
	if(!pxAssertDev(m_AllocCount, "MicroVU1 CPU Provider has not been allocated prior to reset!")) return;
	mVUreset(&microVU1);
}

void recMicroVU0::Execute(u32 cycles) {
	pxAssert(mvu0_allocated); // please allocate me first! :|

	if(!(VU0.VI[REG_VPU_STAT].UL & 1)) return;

	XMMRegisters::Freeze();
	// sometimes games spin on vu0, so be careful with this value
	// woody hangs if too high on sVU (untested on mVU)
	// Edit: Need to test this again, if anyone ever has a "Woody" game :p
	((mVUrecCall)microVU0.startFunct)(VU0.VI[REG_TPC].UL, cycles);
	XMMRegisters::Thaw();
}
void recMicroVU1::Execute(u32 cycles) {
	pxAssert(mvu1_allocated); // please allocate me first! :|

	if(!(VU0.VI[REG_VPU_STAT].UL & 0x100)) return;

	XMMRegisters::Freeze();
	((mVUrecCall)microVU1.startFunct)(VU1.VI[REG_TPC].UL, vu1RunCycles);
	XMMRegisters::Thaw();
}

void recMicroVU0::Clear(u32 addr, u32 size) {
	pxAssert(mvu0_allocated); // please allocate me first! :|
	mVUclear(&microVU0, addr, size); 
}
void recMicroVU1::Clear(u32 addr, u32 size) {
	pxAssert(mvu1_allocated); // please allocate me first! :|
	mVUclear(&microVU1, addr, size); 
}
