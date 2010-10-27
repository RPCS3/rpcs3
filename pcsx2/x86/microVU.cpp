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

// Micro VU recompiler! - author: cottonvibes(@gmail.com)

#include "PrecompiledHeader.h"
#include "Common.h"
#include "microVU.h"
#include "System/RecTypes.h"

// Include all the *.inl files (Needed because C++ sucks with templates and *.cpp files)
#include "microVU_Clamp.inl"
#include "microVU_Misc.inl"
#include "microVU_Log.inl"
#include "microVU_Analyze.inl"
#include "microVU_Alloc.inl"
#include "microVU_Upper.inl"
#include "microVU_Lower.inl"
#include "microVU_Tables.inl"
#include "microVU_Flags.inl"
#include "microVU_Branch.inl"
#include "microVU_Compile.inl"
#include "microVU_Execute.inl"
#include "microVU_Macro.inl"

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

static __fi void mVUthrowHardwareDeficiency(const wxChar* extFail, int vuIndex) {
	throw Exception::HardwareDeficiency()
		.SetDiagMsg(wxsFormat(L"microVU%d recompiler init failed: %s is not available.", vuIndex, extFail))
		.SetUserMsg(wxsFormat(_("%s Extensions not found.  microVU requires a host CPU with MMX, SSE, and SSE2 extensions."), extFail ));
}

// Only run this once per VU! ;)
void microVU::init(uint vuIndex) {

	if(!x86caps.hasMultimediaExtensions)		mVUthrowHardwareDeficiency( L"MMX", vuIndex );
	if(!x86caps.hasStreamingSIMDExtensions)		mVUthrowHardwareDeficiency( L"SSE", vuIndex );
	if(!x86caps.hasStreamingSIMD2Extensions)	mVUthrowHardwareDeficiency( L"SSE2", vuIndex );

	memzero(prog);

	index			= vuIndex;
	cop2			= 0;
	vuMemSize		= (index ? 0x4000 : 0x1000);
	microMemSize	= (index ? 0x4000 : 0x1000);
	progSize		= (index ? 0x4000 : 0x1000) / 4;
	progMemMask		= progSize-1;

	dispCache = SysMmapEx(0, mVUdispCacheSize, 0, (index ? "Micro VU1 Dispatcher" : "Micro VU0 Dispatcher"));
	if (!dispCache) throw Exception::OutOfMemory( index ? L"Micro VU1 Dispatcher" : L"Micro VU0 Dispatcher" );
	memset(dispCache, 0xcc, mVUdispCacheSize);

	cache_reserve = new RecompiledCodeReserve( pxsFmt("Micro VU%u Recompiler Cache", index) );
	cache = index ?
		(u8*)cache_reserve->Reserve( cacheSize, HostMemoryMap::mVU1rec ) :
		(u8*)cache_reserve->Reserve( cacheSize, HostMemoryMap::mVU0rec );

	if(!cache_reserve->IsOk())
		throw Exception::VirtualMemoryMapConflict().SetDiagMsg(pxsFmt( L"Micro VU%u Recompiler Cache", index ));

	ProfilerRegisterSource	(index ? "mVU1 Rec" : "mVU0 Rec", cache, cacheSize);

	regAlloc		= new microRegAlloc(index);
}

// Resets Rec Data
void microVU::reset() {

	x86SetPtr(dispCache);
	mVUdispatcherA(this);
	mVUdispatcherB(this);
		mVUemitSearch();

	// Clear All Program Data
	//memset(&prog, 0, sizeof(prog));
	memset(&prog.lpState, 0, sizeof(prog.lpState));

	if (IsDevBuild) { // Release builds shouldn't need this
		memset(cache, 0xcc, cacheSize);
	}

	// Program Variables
	prog.cleared	=  1;
	prog.isSame		= -1;
	prog.cur		= NULL;
	prog.total		=  0;
	prog.curFrame	=  0;

	// Setup Dynarec Cache Limits for Each Program
	u8* z = cache;
	prog.x86start	= z;
	prog.x86ptr		= z;
	prog.x86end		= z + (cacheSize - mVUcacheSafeZone);

	for (u32 i = 0; i < (progSize / 2); i++) {
		if (!prog.prog[i])
		{
			prog.prog[i] = new deque<microProgram*>();
			continue;
		}

		deque<microProgram*>::iterator it(prog.prog[i]->begin());
		for ( ; it != prog.prog[i]->end(); ++it) {
			if (index)	mVUdeleteProg<1>(it[0]);
			else		mVUdeleteProg<0>(it[0]);
		}
		prog.prog[i]->clear();
		prog.quick[i].block = NULL;
		prog.quick[i].prog  = NULL;
	}
}

// Free Allocated Resources
void microVU::close() {

	if (cache_reserve && cache_reserve->IsOk())
	{
		ProfilerTerminateSource	(index ? "mVU1 Rec" : "mVU0 Rec");
		safe_delete(cache_reserve);
	}

	SafeSysMunmap(dispCache, mVUdispCacheSize);

	// Delete Programs and Block Managers
	for (u32 i = 0; i < (progSize / 2); i++) {
		if (!prog.prog[i]) continue;
		deque<microProgram*>::iterator it(prog.prog[i]->begin());
		for ( ; it != prog.prog[i]->end(); ++it) {
			if (index)	mVUdeleteProg<1>(it[0]);
			else	    mVUdeleteProg<0>(it[0]);
		}
		safe_delete(prog.prog[i]);
	}
}

// Clears Block Data in specified range
static __fi void mVUclear(mV, u32 addr, u32 size) {
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

// Finds and Ages/Kills Programs if they haven't been used in a while.
static __fi void mVUvsyncUpdate(mV) {
	//mVU->prog.curFrame++;
}

// Deletes a program
_mVUt __fi void mVUdeleteProg(microProgram*& prog) {
	microVU* mVU = mVUx;
	for (u32 i = 0; i < (mVU->progSize / 2); i++) {
		safe_delete(prog->block[i]);
	}
	safe_delete(prog->ranges);
	safe_aligned_free(prog);
}

// Creates a new Micro Program
_mVUt __fi microProgram* mVUcreateProg(int startPC) {
	microVU* mVU = mVUx;
	microProgram* prog = (microProgram*)_aligned_malloc(sizeof(microProgram), 64);
	memzero_ptr<sizeof(microProgram)>(prog);
	prog->idx     = mVU->prog.total++;
	prog->ranges  = new deque<microRange>();
	prog->startPC = startPC;
	mVUcacheProg<vuIndex>(*prog); // Cache Micro Program
	double cacheSize = (double)((u32)mVU->prog.x86end - (u32)mVU->prog.x86start);
	double cacheUsed =((double)((u32)mVU->prog.x86ptr - (u32)mVU->prog.x86start)) / cacheSize * 100;
	ConsoleColors c = vuIndex ? Color_Orange : Color_Magenta;
	DevCon.WriteLn(c, "microVU%d: Cached MicroPrograms = [%03d] [PC=%04x] [List=%02d] (Cache = %3.3f%%)",
					vuIndex, prog->idx, startPC, mVU->prog.prog[startPC]->size()+1, cacheUsed);
	return prog;
}

// Caches Micro Program
_mVUt __fi void mVUcacheProg(microProgram& prog) {
	microVU* mVU = mVUx;
	if (!vuIndex) memcpy_const(prog.data, mVU->regs().Micro, 0x1000);
	else		  memcpy_const(prog.data, mVU->regs().Micro, 0x4000);
	mVUdumpProg(prog);
}

// Compare partial program by only checking compiled ranges...
_mVUt __fi bool mVUcmpPartial(microProgram& prog) {
	microVU* mVU = mVUx;
	deque<microRange>::const_iterator it(prog.ranges->begin());
	for ( ; it != prog.ranges->end(); ++it) {
		if((it[0].start<0)||(it[0].end<0))  { DevCon.Error("microVU%d: Negative Range![%d][%d]", mVU->index, it[0].start, it[0].end); }
		if (memcmp_mmx(cmpOffset(prog.data), cmpOffset(mVU->regs().Micro), ((it[0].end + 8)  -  it[0].start))) {
			return 0;
		}
	}
	return 1;
}

// Compare Cached microProgram to mVU->regs().Micro
_mVUt __fi bool mVUcmpProg(microProgram& prog, const bool cmpWholeProg) {
	microVU* mVU = mVUx;
	if ((cmpWholeProg && !memcmp_mmx((u8*)prog.data, mVU->regs().Micro, mVU->microMemSize))
	|| (!cmpWholeProg && mVUcmpPartial<vuIndex>(prog))) {
		mVU->prog.cleared =  0;
		mVU->prog.cur	  = &prog;
		mVU->prog.isSame  =  cmpWholeProg ? 1 : -1;
		return 1;
	}
	return 0;
}

// Searches for Cached Micro Program and sets prog.cur to it (returns entry-point to program)
_mVUt __fi void* mVUsearchProg(u32 startPC, uptr pState) {
	microVU* mVU = mVUx;
	microProgramQuick& quick = mVU->prog.quick[startPC/8];
	microProgramList*  list  = mVU->prog.prog [startPC/8];
	if(!quick.prog) { // If null, we need to search for new program
		deque<microProgram*>::iterator it(list->begin());
		for ( ; it != list->end(); ++it) {
			if (mVUcmpProg<vuIndex>(*it[0], 0)) {
				quick.block = it[0]->block[startPC/8];
				quick.prog  = it[0];
				list->erase(it);
				list->push_front(quick.prog);
				return mVUentryGet(mVU, quick.block, startPC, pState);
			}
		}

		// If cleared and program not found, make a new program instance
		mVU->prog.cleared	= 0;
		mVU->prog.isSame	= 1;
		mVU->prog.cur		= mVUcreateProg<vuIndex>(startPC/8);
		void* entryPoint	= mVUblockFetch(mVU, startPC, pState);
		quick.block			= mVU->prog.cur->block[startPC/8];
		quick.prog			= mVU->prog.cur;
		list->push_front(mVU->prog.cur);
		return entryPoint;
	}
	// If list.quick, then we've already found and recompiled the program ;)
	mVU->prog.isSame	 = -1;
	mVU->prog.cur		 =  quick.prog;
	return mVUentryGet(mVU, quick.block, startPC, pState);
}

//------------------------------------------------------------------
// recMicroVU0 / recMicroVU1
//------------------------------------------------------------------

recMicroVU0::recMicroVU0()		  { m_Idx = 0; IsInterpreter = false; }
recMicroVU1::recMicroVU1()		  { m_Idx = 1; IsInterpreter = false; }
void recMicroVU0::Vsync() throw() { mVUvsyncUpdate(&microVU0); }
void recMicroVU1::Vsync() throw() { mVUvsyncUpdate(&microVU1); }

void recMicroVU0::Reserve() {
	if (AtomicExchange(m_Reserved, 1) == 0)
		microVU0.init(0);
}
void recMicroVU1::Reserve() {
	if (AtomicExchange(m_Reserved, 1) == 0)
		microVU1.init(1);
}

void recMicroVU0::Shutdown() throw() {
	if (AtomicExchange(m_Reserved, 0) == 1)
		microVU0.close();
}
void recMicroVU1::Shutdown() throw() {
	if (AtomicExchange(m_Reserved, 0) == 1)
		microVU1.close();
}

void recMicroVU0::Reset() {
	if(!pxAssertDev(m_Reserved, "MicroVU0 CPU Provider has not been reserved prior to reset!")) return;
	microVU0.reset();
}
void recMicroVU1::Reset() {
	if(!pxAssertDev(m_Reserved, "MicroVU1 CPU Provider has not been reserved prior to reset!")) return;
	microVU1.reset();
}

void recMicroVU0::Execute(u32 cycles) {
	pxAssert(m_Reserved); // please allocate me first! :|

	if(!(VU0.VI[REG_VPU_STAT].UL & 1)) return;

	// Sometimes games spin on vu0, so be careful with this value
	// woody hangs if too high on sVU (untested on mVU)
	// Edit: Need to test this again, if anyone ever has a "Woody" game :p
	((mVUrecCall)microVU0.startFunct)(VU0.VI[REG_TPC].UL, cycles);
}
void recMicroVU1::Execute(u32 cycles) {
	pxAssert(m_Reserved); // please allocate me first! :|

	if(!(VU0.VI[REG_VPU_STAT].UL & 0x100)) return;
	((mVUrecCall)microVU1.startFunct)(VU1.VI[REG_TPC].UL, vu1RunCycles);
}

void recMicroVU0::Clear(u32 addr, u32 size) {
	pxAssert(m_Reserved); // please allocate me first! :|
	mVUclear(&microVU0, addr, size);
}
void recMicroVU1::Clear(u32 addr, u32 size) {
	pxAssert(m_Reserved); // please allocate me first! :|
	mVUclear(&microVU1, addr, size);
}

uint recMicroVU0::GetCacheReserve() const
{
	return microVU0.cacheSize / _1mb;
}
uint recMicroVU1::GetCacheReserve() const
{
	return microVU1.cacheSize / _1mb;
}

void recMicroVU0::SetCacheReserve( uint reserveInMegs ) const
{
	microVU0.cacheSize = reserveInMegs * _1mb;
}
void recMicroVU1::SetCacheReserve( uint reserveInMegs ) const
{
	microVU1.cacheSize = reserveInMegs * _1mb;
}
