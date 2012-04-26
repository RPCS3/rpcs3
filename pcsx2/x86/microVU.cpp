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
#include "microVU.h"

//------------------------------------------------------------------
// Micro VU - Main Functions
//------------------------------------------------------------------

static __fi void mVUthrowHardwareDeficiency(const wxChar* extFail, int vuIndex) {
	throw Exception::HardwareDeficiency()
		.SetDiagMsg(pxsFmt(L"microVU%d recompiler init failed: %s is not available.", vuIndex, extFail))
		.SetUserMsg(pxsFmt(_("%s Extensions not found.  microVU requires a host CPU with MMX, SSE, and SSE2 extensions."), extFail));
}

void mVUreserveCache(microVU& mVU) {

	mVU.cache_reserve = new RecompiledCodeReserve(pxsFmt("Micro VU%u Recompiler Cache", mVU.index));
	mVU.cache_reserve->SetProfilerName(pxsFmt("mVU%urec", mVU.index));
	
	mVU.cache = mVU.index ?
		(u8*)mVU.cache_reserve->Reserve(mVU.cacheSize * _1mb, HostMemoryMap::mVU1rec):
		(u8*)mVU.cache_reserve->Reserve(mVU.cacheSize * _1mb, HostMemoryMap::mVU0rec);

	mVU.cache_reserve->ThrowIfNotOk();
}

// Only run this once per VU! ;)
void mVUinit(microVU& mVU, uint vuIndex) {

	if(!x86caps.hasMultimediaExtensions)	 mVUthrowHardwareDeficiency( L"MMX",  vuIndex );
	if(!x86caps.hasStreamingSIMDExtensions)	 mVUthrowHardwareDeficiency( L"SSE",  vuIndex );
	if(!x86caps.hasStreamingSIMD2Extensions) mVUthrowHardwareDeficiency( L"SSE2", vuIndex );

	memzero(mVU.prog);

	mVU.index			=  vuIndex;
	mVU.cop2			=  0;
	mVU.vuMemSize		= (mVU.index ? 0x4000 : 0x1000);
	mVU.microMemSize	= (mVU.index ? 0x4000 : 0x1000);
	mVU.progSize		= (mVU.index ? 0x4000 : 0x1000) / 4;
	mVU.progMemMask		=  mVU.progSize-1;
	mVU.cacheSize		=  vuIndex ? mVU1cacheReserve : mVU0cacheReserve;
	mVU.cache			= NULL;
	mVU.dispCache		= NULL;
	mVU.startFunct		= NULL;
	mVU.exitFunct		= NULL;

	mVUreserveCache(mVU);

	mVU.dispCache = SysMmapEx(0, mVUdispCacheSize, 0,(mVU.index ?  "Micro VU1 Dispatcher" :  "Micro VU0 Dispatcher"));
	if (!mVU.dispCache) throw Exception::OutOfMemory (mVU.index ? L"Micro VU1 Dispatcher" : L"Micro VU0 Dispatcher");
	memset(mVU.dispCache, 0xcc, mVUdispCacheSize);

	mVU.regAlloc = new microRegAlloc(mVU.index);
}

// Resets Rec Data
void mVUreset(microVU& mVU, bool resetReserve) {

	// Restore reserve to uncommitted state
	if (resetReserve) mVU.cache_reserve->Reset();
	
	x86SetPtr(mVU.dispCache);
	mVUdispatcherA(mVU);
	mVUdispatcherB(mVU);
	mVUdispatcherC(mVU);
	mVUdispatcherD(mVU);
	mVUemitSearch();

	// Clear All Program Data
	//memset(&mVU.prog, 0, sizeof(mVU.prog));
	memset(&mVU.prog.lpState, 0, sizeof(mVU.prog.lpState));
	mVU.profiler.Reset(mVU.index);

	// Program Variables
	mVU.prog.cleared	=  1;
	mVU.prog.isSame		= -1;
	mVU.prog.cur		= NULL;
	mVU.prog.total		=  0;
	mVU.prog.curFrame	=  0;

	// Setup Dynarec Cache Limits for Each Program
	u8* z = mVU.cache;
	mVU.prog.x86start	= z;
	mVU.prog.x86ptr		= z;
	mVU.prog.x86end		= z + ((mVU.cacheSize - mVUcacheSafeZone) * _1mb);
	//memset(mVU.prog.x86start, 0xcc, mVU.cacheSize*_1mb);

	for(u32 i = 0; i < (mVU.progSize / 2); i++) {
		if(!mVU.prog.prog[i]) {
			mVU.prog.prog[i] = new deque<microProgram*>();
			continue;
		}
		deque<microProgram*>::iterator it(mVU.prog.prog[i]->begin());
		for ( ; it != mVU.prog.prog[i]->end(); ++it) {
			mVUdeleteProg(mVU, it[0]);
		}
		mVU.prog.prog[i]->clear();
		mVU.prog.quick[i].block = NULL;
		mVU.prog.quick[i].prog  = NULL;
	}
}

// Free Allocated Resources
void mVUclose(microVU& mVU) {

	safe_delete  (mVU.cache_reserve);
	SafeSysMunmap(mVU.dispCache, mVUdispCacheSize);

	// Delete Programs and Block Managers
	for (u32 i = 0; i < (mVU.progSize / 2); i++) {
		if (!mVU.prog.prog[i]) continue;
		deque<microProgram*>::iterator it(mVU.prog.prog[i]->begin());
		for ( ; it != mVU.prog.prog[i]->end(); ++it) {
			mVUdeleteProg(mVU, it[0]);
		}
		safe_delete(mVU.prog.prog[i]);
	}
}

// Clears Block Data in specified range
__fi void mVUclear(mV, u32 addr, u32 size) {
	if(!mVU.prog.cleared) {
		mVU.prog.cleared = 1;		// Next execution searches/creates a new microprogram
		memzero(mVU.prog.lpState); // Clear pipeline state
		for(u32 i = 0; i < (mVU.progSize / 2); i++) {
			mVU.prog.quick[i].block = NULL; // Clear current quick-reference block
			mVU.prog.quick[i].prog  = NULL; // Clear current quick-reference prog
		}
	}
}

//------------------------------------------------------------------
// Micro VU - Private Functions
//------------------------------------------------------------------

// Finds and Ages/Kills Programs if they haven't been used in a while.
__ri void mVUvsyncUpdate(mV) {
	//mVU.prog.curFrame++;
}

// Deletes a program
__ri void mVUdeleteProg(microVU& mVU, microProgram*& prog) {
	for (u32 i = 0; i < (mVU.progSize / 2); i++) {
		safe_delete(prog->block[i]);
	}
	safe_delete(prog->ranges);
	safe_aligned_free(prog);
}

// Creates a new Micro Program
__ri microProgram* mVUcreateProg(microVU& mVU, int startPC) {
	microProgram* prog = (microProgram*)_aligned_malloc(sizeof(microProgram), 64);
	memzero_ptr<sizeof(microProgram)>(prog);
	prog->idx     = mVU.prog.total++;
	prog->ranges  = new deque<microRange>();
	prog->startPC = startPC;
	mVUcacheProg(mVU, *prog); // Cache Micro Program
	double cacheSize = (double)((u32)mVU.prog.x86end - (u32)mVU.prog.x86start);
	double cacheUsed =((double)((u32)mVU.prog.x86ptr - (u32)mVU.prog.x86start)) / (double)_1mb;
	double cachePerc =((double)((u32)mVU.prog.x86ptr - (u32)mVU.prog.x86start)) / cacheSize * 100;
	ConsoleColors c = mVU.index ? Color_Orange : Color_Magenta;
	DevCon.WriteLn(c, "microVU%d: Cached Prog = [%03d] [PC=%04x] [List=%02d] (Cache=%3.3f%%) [%3.1fmb]",
				   mVU.index, prog->idx, startPC*8, mVU.prog.prog[startPC]->size()+1, cachePerc, cacheUsed);
	return prog;
}

// Caches Micro Program
__ri void mVUcacheProg(microVU& mVU, microProgram& prog) {
	if (!mVU.index)	memcpy_const(prog.data, mVU.regs().Micro, 0x1000);
	else			memcpy_const(prog.data, mVU.regs().Micro, 0x4000);
	mVUdumpProg(mVU, prog);
}

// Generate Hash for partial program based on compiled ranges...
u64 mVUrangesHash(microVU& mVU, microProgram& prog) {
	u32 hash[2] = {0, 0};
	deque<microRange>::const_iterator it(prog.ranges->begin());
	for ( ; it != prog.ranges->end(); ++it) {
		if((it[0].start<0)||(it[0].end<0))  { DevCon.Error("microVU%d: Negative Range![%d][%d]", mVU.index, it[0].start, it[0].end); }
		for(int i = it[0].start/4; i < it[0].end/4; i++) {
			hash[0] -= prog.data[i];
			hash[1] ^= prog.data[i];
		}
	}
	return *(u64*)hash;
}

// Prints the ratio of unique programs to total programs
void mVUprintUniqueRatio(microVU& mVU) {
	vector<u64> v;
	for(u32 pc = 0; pc < mProgSize/2; pc++) {
		microProgramList* list = mVU.prog.prog[pc];
		if (!list) continue;
		deque<microProgram*>::iterator it(list->begin());
		for ( ; it != list->end(); ++it) {
			v.push_back(mVUrangesHash(mVU, *it[0]));
		}
	}
	u32 total = v.size();
	sortVector(v);
	makeUnique(v);
	if (!total) return;
	DevCon.WriteLn("%d / %d [%3.1f%%]", v.size(), total, 100.-(double)v.size()/(double)total*100.);
}

// Compare partial program by only checking compiled ranges...
__ri bool mVUcmpPartial(microVU& mVU, microProgram& prog) {
	deque<microRange>::const_iterator it(prog.ranges->begin());
	for ( ; it != prog.ranges->end(); ++it) {
		if((it[0].start<0)||(it[0].end<0))  { DevCon.Error("microVU%d: Negative Range![%d][%d]", mVU.index, it[0].start, it[0].end); }
		if (memcmp_mmx(cmpOffset(prog.data), cmpOffset(mVU.regs().Micro), ((it[0].end + 8)  -  it[0].start))) {
			return 0;
		}
	}
	return 1;
}

// Compare Cached microProgram to mVU.regs().Micro
__fi bool mVUcmpProg(microVU& mVU, microProgram& prog, const bool cmpWholeProg) {
	if ((cmpWholeProg && !memcmp_mmx((u8*)prog.data, mVU.regs().Micro, mVU.microMemSize))
	|| (!cmpWholeProg && mVUcmpPartial(mVU, prog))) {
		mVU.prog.cleared =  0;
		mVU.prog.cur	 = &prog;
		mVU.prog.isSame  =  cmpWholeProg ? 1 : -1;
		return 1;
	}
	return 0;
}

// Searches for Cached Micro Program and sets prog.cur to it (returns entry-point to program)
_mVUt __fi void* mVUsearchProg(u32 startPC, uptr pState) {
	microVU& mVU = mVUx;
	microProgramQuick& quick = mVU.prog.quick[startPC/8];
	microProgramList*  list  = mVU.prog.prog [startPC/8];
	if(!quick.prog) { // If null, we need to search for new program
		deque<microProgram*>::iterator it(list->begin());
		for ( ; it != list->end(); ++it) {
			if (mVUcmpProg(mVU, *it[0], 0)) {
				quick.block = it[0]->block[startPC/8];
				quick.prog  = it[0];
				list->erase(it);
				list->push_front(quick.prog);
				return mVUentryGet(mVU, quick.block, startPC, pState);
			}
		}

		// If cleared and program not found, make a new program instance
		mVU.prog.cleared	= 0;
		mVU.prog.isSame		= 1;
		mVU.prog.cur		= mVUcreateProg(mVU,  startPC/8);
		void* entryPoint	= mVUblockFetch(mVU,  startPC, pState);
		quick.block			= mVU.prog.cur->block[startPC/8];
		quick.prog			= mVU.prog.cur;
		list->push_front(mVU.prog.cur);
		//mVUprintUniqueRatio(mVU);
		return entryPoint;
	}
	// If list.quick, then we've already found and recompiled the program ;)
	mVU.prog.isSame	= -1;
	mVU.prog.cur	=  quick.prog;
	return mVUentryGet(mVU, quick.block, startPC, pState);
}

//------------------------------------------------------------------
// recMicroVU0 / recMicroVU1
//------------------------------------------------------------------
recMicroVU0::recMicroVU0()		  { m_Idx = 0; IsInterpreter = false; }
recMicroVU1::recMicroVU1()		  { m_Idx = 1; IsInterpreter = false; }
void recMicroVU0::Vsync() throw() { mVUvsyncUpdate(microVU0); }
void recMicroVU1::Vsync() throw() { mVUvsyncUpdate(microVU1); }

void recMicroVU0::Reserve() {
	if (AtomicExchange(m_Reserved, 1) == 0)
		mVUinit(microVU0, 0);
}
void recMicroVU1::Reserve() {
	if (AtomicExchange(m_Reserved, 1) == 0) {
		mVUinit(microVU1, 1);
		vu1Thread.Start();
	}
}

void recMicroVU0::Shutdown() throw() {
	if (AtomicExchange(m_Reserved, 0) == 1)
		mVUclose(microVU0);
}
void recMicroVU1::Shutdown() throw() {
	if (AtomicExchange(m_Reserved, 0) == 1) {
		vu1Thread.WaitVU();
		mVUclose(microVU1);
	}
}

void recMicroVU0::Reset() {
	if(!pxAssertDev(m_Reserved, "MicroVU0 CPU Provider has not been reserved prior to reset!")) return;
	mVUreset(microVU0, true);
}
void recMicroVU1::Reset() {
	if(!pxAssertDev(m_Reserved, "MicroVU1 CPU Provider has not been reserved prior to reset!")) return;
	vu1Thread.WaitVU();
	mVUreset(microVU1, true);
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

	if (!THREAD_VU1) {
		if(!(VU0.VI[REG_VPU_STAT].UL & 0x100)) return;
	}
	((mVUrecCall)microVU1.startFunct)(VU1.VI[REG_TPC].UL, cycles);
}

void recMicroVU0::Clear(u32 addr, u32 size) {
	pxAssert(m_Reserved); // please allocate me first! :|
	mVUclear(microVU0, addr, size);
}
void recMicroVU1::Clear(u32 addr, u32 size) {
	pxAssert(m_Reserved); // please allocate me first! :|
	mVUclear(microVU1, addr, size);
}

uint recMicroVU0::GetCacheReserve() const {
	return microVU0.cacheSize;
}
uint recMicroVU1::GetCacheReserve() const {
	return microVU1.cacheSize;
}

void recMicroVU0::SetCacheReserve(uint reserveInMegs) const {
	DevCon.WriteLn("microVU0: Changing cache size [%dmb]", reserveInMegs);
	microVU0.cacheSize = min(reserveInMegs, mVU0cacheReserve);
	safe_delete(microVU0.cache_reserve); // I assume this unmaps the memory
	mVUreserveCache(microVU0); // Need rec-reset after this
}
void recMicroVU1::SetCacheReserve(uint reserveInMegs) const {
	DevCon.WriteLn("microVU1: Changing cache size [%dmb]", reserveInMegs);
	microVU1.cacheSize = min(reserveInMegs, mVU1cacheReserve);
	safe_delete(microVU1.cache_reserve); // I assume this unmaps the memory
	mVUreserveCache(microVU1); // Need rec-reset after this
}

void recMicroVU1::ResumeXGkick() {
	pxAssert(m_Reserved); // please allocate me first! :|

	if(!(VU0.VI[REG_VPU_STAT].UL & 0x100)) return;
	((mVUrecCallXG)microVU1.startFunctXG)();
}
