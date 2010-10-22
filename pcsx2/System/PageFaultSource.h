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

// =====================================================================================================
//  Cross-Platform Memory Protection (Used by VTLB, Recompilers and Texture caches)
// =====================================================================================================
// Win32 platforms use the SEH model: __try {}  __except {}
// Linux platforms use the POSIX Signals model: sigaction()

#include "Utilities/EventSource.h"

struct PageFaultInfo
{
	uptr	addr;

	PageFaultInfo( uptr address )
	{
		addr = address;
	}
};

// --------------------------------------------------------------------------------------
//  IEventListener_PageFault
// --------------------------------------------------------------------------------------
class IEventListener_PageFault : public IEventDispatcher<PageFaultInfo>
{
public:
	typedef PageFaultInfo EvtParams;

public:
	virtual ~IEventListener_PageFault() throw() {}

	virtual void DispatchEvent( const PageFaultInfo& evtinfo, bool& handled )
	{
		OnPageFaultEvent( evtinfo, handled );
	}

	virtual void DispatchEvent( const PageFaultInfo& evtinfo )
	{
		pxFailRel( "Don't call me, damnit.  Use DispatchException instead." );
	}

protected:
	virtual void OnPageFaultEvent( const PageFaultInfo& evtinfo, bool& handled ) {}
};

// --------------------------------------------------------------------------------------
//  EventListener_PageFault
// --------------------------------------------------------------------------------------
class EventListener_PageFault : public IEventListener_PageFault
{
public:
	EventListener_PageFault();
	virtual ~EventListener_PageFault() throw();
};

class SrcType_PageFault : public EventSource<IEventListener_PageFault>
{
protected:
	typedef EventSource<IEventListener_PageFault> _parent;

protected:
	bool	m_handled;

public:
	SrcType_PageFault() {}
	virtual ~SrcType_PageFault() throw() { }

	bool WasHandled() const { return m_handled; }
	virtual void Dispatch( const PageFaultInfo& params );

protected:
	virtual void _DispatchRaw( ListenerIterator iter, const ListenerIterator& iend, const PageFaultInfo& evt );
};

// --------------------------------------------------------------------------------------
//  BaseVirtualMemoryReserve   (WIP!!)
// --------------------------------------------------------------------------------------
class BaseVirtualMemoryReserve : public EventListener_PageFault
{
	DeclareNoncopyableObject( BaseVirtualMemoryReserve );

public:
	wxString Name;

protected:
	void*	m_baseptr;

	// reserved memory (in pages).
	uptr	m_reserved;

	// Incremental size by which the buffer grows (in pages)
	uptr	m_block_size;

	// Protection mode to be applied to committed blocks.
	PageProtectionMode m_prot_mode;

	// Specifies the number of blocks that should be committed automatically when the
	// reserve is created.  Typically this chunk is larger than the block size, and
	// should be based on whatever typical overhead is needed for basic block use.
	uint	m_def_commit;

	// Records the number of pages committed to memory.
	// (metric for analysis of buffer usage)
	uptr	m_commited;

public:
	BaseVirtualMemoryReserve( const wxString& name );
	virtual ~BaseVirtualMemoryReserve() throw()
	{
		Free();
	}

	void* Reserve( uint size, uptr base = 0, uptr upper_bounds = 0 );
	void Reset();
	void Free();
	
	uptr GetReserveSizeInBytes() const { return m_reserved * __pagesize; }
	uptr GetReserveSizeInPages() const { return m_reserved; }

	u8* GetPtr()					{ return (u8*)m_baseptr; }
	const u8* GetPtr() const		{ return (u8*)m_baseptr; }

	u8* GetPtrEnd()					{ return (u8*)m_baseptr + (m_reserved * __pagesize); }
	const u8* GetPtrEnd() const		{ return (u8*)m_baseptr + (m_reserved * __pagesize); }

	operator void*()				{ return m_baseptr; }
	operator const void*() const	{ return m_baseptr; }

	operator u8*()					{ return (u8*)m_baseptr; }
	operator const u8*() const		{ return (u8*)m_baseptr; }

protected:
	void OnPageFaultEvent( const PageFaultInfo& info, bool& handled );

	virtual void OnCommittedBlock( void* block )=0;
	virtual void OnOutOfMemory( const Exception::OutOfMemory& ex, void* blockptr, bool& handled )
	{
		throw;
	}
};

// --------------------------------------------------------------------------------------
//  RecompiledCodeReserve
// --------------------------------------------------------------------------------------
class RecompiledCodeReserve : public BaseVirtualMemoryReserve
{
protected:

public:
	RecompiledCodeReserve( const wxString& name, uint defCommit = 0 );
	void OnCommittedBlock( void* block );
	void OnOutOfMemory( const Exception::OutOfMemory& ex, void* blockptr, bool& handled );

	operator void*()				{ return m_baseptr; }
	operator const void*() const	{ return m_baseptr; }

	operator u8*()				{ return (u8*)m_baseptr; }
	operator const u8*() const	{ return (u8*)m_baseptr; }
};

#ifdef __LINUX__

#	define PCSX2_PAGEFAULT_PROTECT
#	define PCSX2_PAGEFAULT_EXCEPT

#elif defined( _WIN32 )

struct _EXCEPTION_POINTERS;
extern int SysPageFaultExceptionFilter(struct _EXCEPTION_POINTERS* eps);

#	define PCSX2_PAGEFAULT_PROTECT		__try
#	define PCSX2_PAGEFAULT_EXCEPT		__except(SysPageFaultExceptionFilter(GetExceptionInformation())) {}

#else
#	error PCSX2 - Unsupported operating system platform.
#endif


extern void InstallSignalHandler();

extern SrcType_PageFault Source_PageFault;

