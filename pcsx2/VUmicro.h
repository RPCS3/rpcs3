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

#pragma once

#include "VU.h"
#include "VUops.h"
#include "R5900.h"

static const uint VU0_MEMSIZE = 0x1000;
static const uint VU0_PROGSIZE = 0x1000;
static const uint VU1_MEMSIZE = 0x4000;
static const uint VU1_PROGSIZE = 0x4000;

static const uint VU0_MEMMASK = VU0_MEMSIZE-1;
static const uint VU0_PROGMASK = VU0_PROGSIZE-1;
static const uint VU1_MEMMASK = VU1_MEMSIZE-1;
static const uint VU1_PROGMASK = VU1_PROGSIZE-1;

#define vuRunCycles  (512*12)  // Cycles to run ExecuteBlockJIT() for (called from within recs)
#define vu0RunCycles (512*12)  // Cycles to run vu0 for whenever ExecuteBlock() is called
#define vu1RunCycles (3000000) // mVU1 uses this for inf loop detection on dev builds

// --------------------------------------------------------------------------------------
//  BaseCpuProvider
// --------------------------------------------------------------------------------------
//
// Design Note: This class is only partial C++ style.  It still relies on Alloc and Shutdown
// calls for memory and resource management.  This is because the underlying implementations
// of our CPU emulators don't have properly encapsulated objects yet -- if we allocate ram
// in a constructor, it won't get free'd if an exception occurs during object construction.
// Once we've resolved all the 'dangling pointers' and stuff in the recompilers, Alloc
// and Shutdown can be removed in favor of constructor/destructor syntax.
//
class BaseCpuProvider
{
protected:
	// allocation counter for multiple init/shutdown calls
	// (most or all implementations will need this!)
	int		m_AllocCount;

public:
	// this boolean indicates to some generic logging facilities if the VU's registers
	// are valid for logging or not. (see DisVU1Micro.cpp, etc)  [kinda hacky, might
	// be removed in the future]
	bool	IsInterpreter;

public:
	BaseCpuProvider()
	{
		m_AllocCount   = 0;
	}

	virtual ~BaseCpuProvider() throw()
	{
		if( m_AllocCount != 0 )
			Console.Warning( "Cleanup miscount detected on CPU provider.  Count=%d", m_AllocCount );
	}

	virtual const char* GetShortName() const=0;
	virtual wxString GetLongName() const=0;

	// returns the number of bytes committed to the working caches for this CPU
	// provider (typically this refers to recompiled code caches, but could also refer
	// to other optional growable allocations).
	virtual size_t GetCommittedCache() const
	{
		return 0;
	}

	virtual void Allocate()=0;
	virtual void Shutdown()=0;
	virtual void Reset()=0;
	virtual void Execute(u32 cycles)=0;
	virtual void ExecuteBlock(bool startUp)=0;

	virtual void Step()=0;
	virtual void Clear(u32 Addr, u32 Size)=0;

	// C++ Calling Conventions are unstable, and some compilers don't even allow us to take the
	// address of C++ methods.  We need to use a wrapper function to invoke the ExecuteBlock from
	// recompiled code.
	static void __fastcall ExecuteBlockJIT( BaseCpuProvider* cpu )
	{
		cpu->Execute(1024);
	}
};

// --------------------------------------------------------------------------------------
//  BaseVUmicroCPU
// --------------------------------------------------------------------------------------
// Layer class for possible future implementation (currently is nothing more than a type-safe
// type define).
//
class BaseVUmicroCPU : public BaseCpuProvider {
public:
	int m_Idx;
	u32 m_lastEEcycles;

	BaseVUmicroCPU() {
		m_Idx		   = 0;
		m_lastEEcycles = 0;
	}
	virtual ~BaseVUmicroCPU() throw() {}

	// Called by the PS2 VM's event manager for every internal vertical sync (occurs at either
	// 50hz (pal) or 59.94hz (NTSC).
	//
	// Exceptions:
	//   This method is not allowed to throw exceptions, since exceptions may not propagate
	//   safely from the context of recompiled code stackframes.
	//
	// Thread Affinity:
	//   Called from the EEcore thread.  No locking is performed, so any necessary locks must
	//   be implemented by the CPU provider manually.
	//
	virtual void Vsync() throw() { }

	virtual void Step() {
		// Ideally this would fall back on interpretation for executing single instructions
		// for all CPU types, but due to VU complexities and large discrepancies between
		// clamping in recs and ints, it's not really worth bothering with yet.
	}

	// Execute VU for the number of VU cycles (recs might go over 0~30 cycles)
	// virtual void Execute(u32 cycles)=0;

	// Executes a Block based on static preset cycles OR
	// Executes a Block based on EE delta time (see VUmicro.cpp)
	virtual void ExecuteBlock(bool startUp=0);

	static void __fastcall ExecuteBlockJIT(BaseVUmicroCPU* cpu);
};


// --------------------------------------------------------------------------------------
//  InterpVU0 / InterpVU1
// --------------------------------------------------------------------------------------
class InterpVU0 : public BaseVUmicroCPU
{
public:
	InterpVU0();
	virtual ~InterpVU0() throw() { Shutdown(); }

	const char* GetShortName() const	{ return "intVU0"; }
	wxString GetLongName() const		{ return L"VU0 Interpreter"; }

	void Allocate() { }
	void Shutdown() throw() { }
	void Reset() { }

	void Step();
	void Execute(u32 cycles);
	void Clear(u32 addr, u32 size) {}
};

class InterpVU1 : public BaseVUmicroCPU
{
public:
	InterpVU1();
	virtual ~InterpVU1() throw() { Shutdown(); }

	const char* GetShortName() const	{ return "intVU1"; }
	wxString GetLongName() const		{ return L"VU1 Interpreter"; }

	void Allocate() { }
	void Shutdown() throw() { }
	void Reset() { }

	void Step();
	void Execute(u32 cycles);
	void Clear(u32 addr, u32 size) {}
};

// --------------------------------------------------------------------------------------
//  recMicroVU0 / recMicroVU1
// --------------------------------------------------------------------------------------
class recMicroVU0 : public BaseVUmicroCPU
{
public:
	recMicroVU0();
	virtual ~recMicroVU0() throw()  { Shutdown(); }

	const char* GetShortName() const	{ return "mVU0"; }
	wxString GetLongName() const		{ return L"microVU0 Recompiler"; }

	void Allocate();
	void Shutdown() throw();

	void Reset();
	void Execute(u32 cycles);
	void Clear(u32 addr, u32 size);
	void Vsync() throw();
};

class recMicroVU1 : public BaseVUmicroCPU
{
public:
	recMicroVU1();
	virtual ~recMicroVU1() throw() { Shutdown(); }

	const char* GetShortName() const	{ return "mVU1"; }
	wxString GetLongName() const		{ return L"microVU1 Recompiler"; }

	void Allocate();
	void Shutdown() throw();
	void Reset();
	void Execute(u32 cycles);
	void Clear(u32 addr, u32 size);
	void Vsync() throw();
};

// --------------------------------------------------------------------------------------
//  recSuperVU0 / recSuperVU1
// --------------------------------------------------------------------------------------

class recSuperVU0 : public BaseVUmicroCPU
{
public:
	recSuperVU0();

	const char* GetShortName() const	{ return "sVU0"; }
	wxString GetLongName() const		{ return L"SuperVU0 Recompiler"; }

	void Allocate();
	void Shutdown() throw();
	void Reset();
	void Execute(u32 cycles);
	void Clear(u32 Addr, u32 Size);
};

class recSuperVU1 : public BaseVUmicroCPU
{
public:
	recSuperVU1();

	const char* GetShortName() const	{ return "sVU1"; }
	wxString GetLongName() const		{ return L"SuperVU1 Recompiler"; }

	void Allocate();
	void Shutdown() throw();
	void Reset();
	void Execute(u32 cycles);
	void Clear(u32 Addr, u32 Size);
};

extern BaseVUmicroCPU* CpuVU0;
extern BaseVUmicroCPU* CpuVU1;


extern void vuMicroMemAlloc();
extern void vuMicroMemShutdown();
extern void vuMicroMemReset();

// VU0
extern void vu0ResetRegs();
extern void __fastcall vu0ExecMicro(u32 addr);
extern void vu0Exec(VURegs* VU);
extern void vu0Finish();
extern void iDumpVU0Registers();

// VU1
extern void vu1Finish();
extern void vu1ResetRegs();
extern void __fastcall vu1ExecMicro(u32 addr);
extern void vu1Exec(VURegs* VU);
extern void iDumpVU1Registers();

#ifdef VUM_LOG

#define IdebugUPPER(VU) \
	VUM_LOG("%s", dis##VU##MicroUF(VU.code, VU.VI[REG_TPC].UL));
#define IdebugLOWER(VU) \
	VUM_LOG("%s", dis##VU##MicroLF(VU.code, VU.VI[REG_TPC].UL));
#define _vuExecMicroDebug(VU) \
	VUM_LOG("_vuExecMicro: %8.8x", VU.VI[REG_TPC].UL);

#else

#define IdebugUPPER(VU)
#define IdebugLOWER(VU)
#define _vuExecMicroDebug(VU)

#endif
