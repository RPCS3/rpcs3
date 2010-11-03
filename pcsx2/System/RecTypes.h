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

#include "Utilities/PageFaultSource.h"

// --------------------------------------------------------------------------------------
//  RecompiledCodeReserve
// --------------------------------------------------------------------------------------
// A recompiled code reserve is a simple sequential-growth block of memory which is auto-
// cleared to INT 3 (0xcc) as needed.  When using this class, care should be take to re-
// implement the provided OnOutOfMemory handler so that it clears other recompiled memory
// reserves that are known to be attached to the process.
//
class RecompiledCodeReserve : public BaseVirtualMemoryReserve
{
	typedef BaseVirtualMemoryReserve __parent;

protected:
	// Specifies the number of blocks that should be committed automatically when the
	// reserve is created.  Typically this chunk is larger than the block size, and
	// should be based on whatever typical overhead is needed for basic block use.
	uint		m_def_commit;

	wxString	m_profiler_name;
	bool		m_profiler_registered;

public:
	RecompiledCodeReserve( const wxString& name, uint defCommit = 0 );
	virtual ~RecompiledCodeReserve() throw();

	virtual void* Reserve( uint size, uptr base=0, uptr upper_bounds=0 );
	virtual void OnCommittedBlock( void* block );
	virtual void OnOutOfMemory( const Exception::OutOfMemory& ex, void* blockptr, bool& handled );

	virtual RecompiledCodeReserve& SetProfilerName( const wxString& shortname );
	virtual RecompiledCodeReserve& SetProfilerName( const char* shortname )
	{
		return SetProfilerName( fromUTF8(shortname) );
	}

	operator void*()				{ return m_baseptr; }
	operator const void*() const	{ return m_baseptr; }

	operator u8*()				{ return (u8*)m_baseptr; }
	operator const u8*() const	{ return (u8*)m_baseptr; }

protected:
	void ResetProcessReserves() const;
	void DoCommitAndProtect( uptr page );

	void _registerProfiler();
	void _termProfiler();
	
	uint _calcDefaultCommitInBlocks() const;
};
