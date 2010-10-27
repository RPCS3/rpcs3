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

#include "PageFaultSource.h"
#include "EventSource.inl"
#include "MemsetFast.inl"

template class EventSource< IEventListener_PageFault >;

SrcType_PageFault* Source_PageFault = NULL;

EventListener_PageFault::EventListener_PageFault()
{
	pxAssume(Source_PageFault);
	Source_PageFault->Add( *this );
}

EventListener_PageFault::~EventListener_PageFault() throw()
{
	if (Source_PageFault)
		Source_PageFault->Remove( *this );
}

void SrcType_PageFault::Dispatch( const PageFaultInfo& params )
{
	m_handled = false;
	_parent::Dispatch( params );
}

void SrcType_PageFault::_DispatchRaw( ListenerIterator iter, const ListenerIterator& iend, const PageFaultInfo& evt )
{
	do {
		(*iter)->DispatchEvent( evt, m_handled );
	} while( (++iter != iend) && !m_handled );
}

// --------------------------------------------------------------------------------------
//  BaseVirtualMemoryReserve  (implementations)
// --------------------------------------------------------------------------------------

BaseVirtualMemoryReserve::BaseVirtualMemoryReserve( const wxString& name )
	: Name( name )
{
	m_commited		= 0;
	m_reserved		= 0;
	m_baseptr		= NULL;
	m_block_size	= __pagesize;
	m_prot_mode		= PageAccess_None();
}

// Parameters:
//   upper_bounds - criteria that must be met for the allocation to be valid.
//     If the OS refuses to allocate the memory below the specified address, the
//     object will fail to initialize and an exception will be thrown.
void* BaseVirtualMemoryReserve::Reserve( uint size, uptr base, uptr upper_bounds )
{
	if (!pxAssertDev( m_baseptr == NULL, "(VirtualMemoryReserve) Invalid object state; object has already been reserved." ))
		return m_baseptr;

	m_reserved = (size + __pagesize-4) / __pagesize;
	uptr reserved_bytes = m_reserved * __pagesize;

	m_baseptr = (void*)HostSys::MmapReserve(base, reserved_bytes);

	if (!m_baseptr && (upper_bounds != 0 && (((uptr)m_baseptr + reserved_bytes) > upper_bounds)))
	{
		if (base)
		{
			DevCon.Warning( L"%s: address 0x%08x is unavailable; trying OS-selected address instead.", Name.c_str(), base );

			// Let's try again at an OS-picked memory area, and then hope it meets needed
			// boundschecking criteria below.
			SafeSysMunmap( m_baseptr, reserved_bytes );
			m_baseptr = (void*)HostSys::MmapReserve( NULL, reserved_bytes );
		}

		if ((upper_bounds != 0) && (((uptr)m_baseptr + reserved_bytes) > upper_bounds))
		{
			SafeSysMunmap( m_baseptr, reserved_bytes );
			// returns null, caller should throw an exception or handle appropriately.
		}
	}

	if (!m_baseptr) return NULL;
	
	DevCon.WriteLn( Color_Blue, L"%-32s @ 0x%08X -> 0x%08X [%umb]", Name.c_str(),
		m_baseptr, (uptr)m_baseptr+reserved_bytes, reserved_bytes / _1mb);

	/*if (m_def_commit)
	{
		const uint camt = m_def_commit * __pagesize;
		HostSys::MmapCommit(m_baseptr, camt);
		HostSys::MemProtect(m_baseptr, camt, m_prot_mode);

		u8* init = (u8*)m_baseptr;
		u8* endpos = init + camt;
		for( ; init<endpos; init += m_block_size*__pagesize )
		OnCommittedBlock(init);

		m_commited += m_def_commit * __pagesize;
	}*/
	
	return m_baseptr;
}

// Clears all committed blocks, restoring the allocation to a reserve only.
void BaseVirtualMemoryReserve::Reset()
{
	if (!m_commited) return;
	
	HostSys::MemProtect(m_baseptr, m_commited*__pagesize, PageAccess_None());
	HostSys::MmapReset(m_baseptr, m_commited*__pagesize);
	m_commited = 0;
}

void BaseVirtualMemoryReserve::Free()
{
	HostSys::Munmap((uptr)m_baseptr, m_reserved*__pagesize);
}

void BaseVirtualMemoryReserve::OnPageFaultEvent(const PageFaultInfo& info, bool& handled)
{
	uptr offset = (info.addr - (uptr)m_baseptr) / __pagesize;
	if (offset >= m_reserved) return;

	try	{

		if (!m_commited && m_def_commit)
		{
			const uint camt = m_def_commit * __pagesize;
			// first block being committed!  Commit the default requested
			// amount if its different from the blocksize.
			
			HostSys::MmapCommit(m_baseptr, camt);
			HostSys::MemProtect(m_baseptr, camt, m_prot_mode);

			u8* init = (u8*)m_baseptr;
			u8* endpos = init + camt;
			for( ; init<endpos; init += m_block_size*__pagesize )
				OnCommittedBlock(init);

			m_commited += m_def_commit;

			handled = true;
			return;
		}

		void* bleh = (u8*)m_baseptr + (offset * __pagesize);
	
		// Depending on the operating system, one or both of these could fail if the system
		// is low on either physical ram or virtual memory.
		HostSys::MmapCommit(bleh, m_block_size*__pagesize);
		HostSys::MemProtect(bleh, m_block_size*__pagesize, m_prot_mode);

		m_commited += m_block_size;
		OnCommittedBlock(bleh);

		handled = true;
	}
	catch (Exception::OutOfMemory& ex)
	{
		OnOutOfMemory( ex, (u8*)m_baseptr + (offset * __pagesize), handled );
	}
	#ifndef __WXMSW__
	// In windows we can let exceptions bubble out of the page fault handler.  SEH will more
	// or less handle them in a semi-expected way, and might even avoid a GPF long enough
	// for the system to log the error or something.
	
	// In Linux, however, the SIGNAL handler is very limited in what it can do, and not only
	// can't we let the C++ exception try to unwind the stack, we can't really log it either.
	// We can't issue a proper assertion (requires user popup).  We can't do jack or shit,
	// *unless* its attached to a debugger;  then we can, at a bare minimum, trap it.
	catch (Exception::BaseException& ex)
	{
		wxTrap();
		handled = false;
	}
	#endif
}


// --------------------------------------------------------------------------------------
//  SpatialArrayReserve  (implementations)
// --------------------------------------------------------------------------------------

void* SpatialArrayReserve::Reserve( uint size, uptr base, uptr upper_bounds )
{
	return __parent::Reserve( size, base, upper_bounds );
}

void SpatialArrayReserve::OnCommittedBlock( void* block )
{

}

void SpatialArrayReserve::OnOutOfMemory( const Exception::OutOfMemory& ex, void* blockptr, bool& handled )
{

}


// --------------------------------------------------------------------------------------
//  PageProtectionMode  (implementations)
// --------------------------------------------------------------------------------------
wxString PageProtectionMode::ToString() const
{
	wxString modeStr;

	if (m_read)		modeStr += L"Read";
	if (m_write)	modeStr += L"Write";
	if (m_exec)		modeStr += L"Exec";

	if (modeStr.IsEmpty()) return L"NoAccess";
	if (modeStr.Length() <= 5) modeStr += L"Only";

	return modeStr;
}
