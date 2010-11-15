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

// [TODO] Rename this file to VirtualMemory.h !!

#pragma once

// =====================================================================================================
//  Cross-Platform Memory Protection (Used by VTLB, Recompilers and Texture caches)
// =====================================================================================================
// Win32 platforms use the SEH model: __try {}  __except {}
// Linux platforms use the POSIX Signals model: sigaction()
// [TODO] OS-X (Darwin) platforms should use the Mach exception model (not implemented)

#include "EventSource.h"

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
//  EventListener_PageFault / EventListenerHelper_PageFault
// --------------------------------------------------------------------------------------
class EventListener_PageFault : public IEventListener_PageFault
{
public:
	EventListener_PageFault();
	virtual ~EventListener_PageFault() throw();
};

template< typename TypeToDispatchTo >
class EventListenerHelper_PageFault : public EventListener_PageFault
{
public:
	TypeToDispatchTo*	Owner;

public:
	EventListenerHelper_PageFault( TypeToDispatchTo& dispatchTo )
	{
		Owner = &dispatchTo;
	}

	EventListenerHelper_PageFault( TypeToDispatchTo* dispatchTo )
	{
		Owner = dispatchTo;
	}

	virtual ~EventListenerHelper_PageFault() throw() {}

protected:
	virtual void OnPageFaultEvent( const PageFaultInfo& info, bool& handled )
	{
		OnPageFaultEvent( info, handled );
	}

};

// --------------------------------------------------------------------------------------
//  SrcType_PageFault
// --------------------------------------------------------------------------------------
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
//  VirtualMemoryReserve
// --------------------------------------------------------------------------------------
class VirtualMemoryReserve
{
	DeclareNoncopyableObject( VirtualMemoryReserve );

public:
	wxString Name;

protected:
	// Default size of the reserve, in bytes.  Can be specified when the object is contructed.
	// Is used as the reserve size when Reserve() is called, unless an override is specified
	// in the Reserve parameters.
	size_t	m_defsize;

	void*	m_baseptr;

	// reserved memory (in pages).
	uptr	m_pages_reserved;

	// Protection mode to be applied to committed blocks.
	PageProtectionMode m_prot_mode;

	// Records the number of pages committed to memory.
	// (metric for analysis of buffer usage)
	uptr	m_pages_commited;

public:
	VirtualMemoryReserve( const wxString& name, size_t size = 0 );
	virtual ~VirtualMemoryReserve() throw()
	{
		Release();
	}

	virtual void* Reserve( size_t size = 0, uptr base = 0, uptr upper_bounds = 0 );
	virtual void* ReserveAt( uptr base = 0, uptr upper_bounds = 0 )
	{
		return Reserve(m_defsize, base, upper_bounds);
	}

	virtual void Reset();
	virtual void Release();
	virtual bool TryResize( uint newsize );
	virtual bool Commit();

	bool IsOk() const { return m_baseptr !=  NULL; }
	wxString GetName() const { return Name; }

	uptr GetReserveSizeInBytes() const	{ return m_pages_reserved * __pagesize; }
	uptr GetReserveSizeInPages() const	{ return m_pages_reserved; }
	uint GetCommittedPageCount() const	{ return m_pages_commited; }
	uint GetCommittedBytes() const		{ return m_pages_commited * __pagesize; }

	u8* GetPtr()					{ return (u8*)m_baseptr; }
	const u8* GetPtr() const		{ return (u8*)m_baseptr; }
	u8* GetPtrEnd()					{ return (u8*)m_baseptr + (m_pages_reserved * __pagesize); }
	const u8* GetPtrEnd() const		{ return (u8*)m_baseptr + (m_pages_reserved * __pagesize); }

	VirtualMemoryReserve& SetBaseAddr( uptr newaddr );
	VirtualMemoryReserve& SetPageAccessOnCommit( const PageProtectionMode& mode );

	operator void*()				{ return m_baseptr; }
	operator const void*() const	{ return m_baseptr; }

	operator u8*()					{ return (u8*)m_baseptr; }
	operator const u8*() const		{ return (u8*)m_baseptr; }

	u8& operator[](uint idx)
	{
		pxAssume(idx < (m_pages_reserved * __pagesize));
		return *((u8*)m_baseptr + idx);
	}

	const u8& operator[](uint idx) const
	{
		pxAssume(idx < (m_pages_reserved * __pagesize));
		return *((u8*)m_baseptr + idx);
	}

};

// --------------------------------------------------------------------------------------
//  BaseVmReserveListener
// --------------------------------------------------------------------------------------
class BaseVmReserveListener : public VirtualMemoryReserve
{
	DeclareNoncopyableObject( BaseVmReserveListener );

	typedef VirtualMemoryReserve _parent;

protected:
	EventListenerHelper_PageFault<BaseVmReserveListener> m_pagefault_listener;

	// Incremental size by which the buffer grows (in pages)
	uptr	m_blocksize;

public:
	BaseVmReserveListener( const wxString& name, size_t size = 0 );
	virtual ~BaseVmReserveListener() throw() { }

	operator void*()				{ return m_baseptr; }
	operator const void*() const	{ return m_baseptr; }

	operator u8*()					{ return (u8*)m_baseptr; }
	operator const u8*() const		{ return (u8*)m_baseptr; }

	using _parent::operator[];

protected:
	void OnPageFaultEvent( const PageFaultInfo& info, bool& handled );

	// This function is called from OnPageFaultEvent after the address has been translated
	// and confirmed to apply to this reserved area in question.  OnPageFaultEvent contains 
	// a try/catch exception handler, which ensures "reasonable" error response behavior if
	// this function throws exceptions.
	//
	// Important: This method is called from the context of an exception/signal handler.  On
	// Windows this isn't a big deal (most operations are ok).  On Linux, however, logging
	// and other facilities are probably not a good idea.
	virtual void DoCommitAndProtect( uptr offset )=0;

	// This function is called for every committed block.
	virtual void OnCommittedBlock( void* block )=0;

	virtual void CommitBlocks( uptr page, uint blocks );
};

// --------------------------------------------------------------------------------------
//  SpatialArrayReserve
// --------------------------------------------------------------------------------------
// A spatial array is one where large areas of the memory reserve will remain unused during
// process execution.  Only areas put to use will be committed to virtual memory.
//
// Spatial array efficiency depends heavily on selecting the right parameters for the array's
// primary intended use.  Memory in a spatial array is arranged by blocks, with each block
// containing some number of pages (pages are 4096 bytes each on most platforms).  When the
// array is accessed, the entire block containing the addressed memory will be committed at
// once.  Blocks can be a single page in size (4096 bytes), though this is highly discouraged
// due to overhead and fragmentation penalties.
//
// Balancing block sizes:
//   Larger blocks are good for reducing memory fragmentation and block-tracking overhead, but
//   can also result in a lot of otherwise unused memory being committed to memory.  Smaller
//   blocks are good for arrays that will tend toward more sequential behavior, as they reduce
//   the amount of unused memory being committed.  However, since every block requires a
//   tracking entry, assigning small blocks to a very large array can result in quite a bit of
//   unwanted overhead.  Furthermore, if the array is accessed randomly, system physical memory
//   will become very fragmented, which will also hurt performance.
//
// By default, the base block size is based on a heuristic that balances the size of the spatial
// array reserve against a best-guess performance profile for the target platform.
//
class SpatialArrayReserve : public BaseVmReserveListener
{
	typedef BaseVmReserveListener _parent;

protected:
	uint			m_numblocks;

	// Array of block bits, each bit indicating if the block has been committed to memory
	// or not.  The array length is typically determined via ((numblocks+7) / 8), though the
	// actual array size may be larger in order to accommodate 32-bit or 128-bit accelerated
	// operations.
	ScopedAlignedAlloc<u8,16>	m_blockbits;

public:
	SpatialArrayReserve( const wxString& name );

	virtual void* Reserve( size_t size = 0, uptr base = 0, uptr upper_bounds = 0 );
	virtual void Reset();
	virtual bool TryResize( uint newsize );

	void OnCommittedBlock( void* block );
	
	SpatialArrayReserve& SetBlockCount( uint blocks );
	SpatialArrayReserve& SetBlockSizeInPages( uint bytes );
	uint SetBlockSize( uint bytes );

	operator void*()				{ return m_baseptr; }
	operator const void*() const	{ return m_baseptr; }

	operator u8*()				{ return (u8*)m_baseptr; }
	operator const u8*() const	{ return (u8*)m_baseptr; }
	
	using _parent::operator[];

protected:
	void DoCommitAndProtect( uptr page );
	uint _calcBlockBitArrayLength() const;
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

extern void pxInstallSignalHandler();
extern void _platform_InstallSignalHandler();

extern SrcType_PageFault* Source_PageFault;

