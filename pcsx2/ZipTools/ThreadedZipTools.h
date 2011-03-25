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

#include "Utilities/PersistentThread.h"
#include "Utilities/pxStreams.h"
#include "wx/zipstrm.h"

using namespace Threading;

// --------------------------------------------------------------------------------------
//  ArchiveEntry
// --------------------------------------------------------------------------------------
class ArchiveEntry
{
protected:
	wxString	m_filename;
	uptr		m_dataidx;
	size_t		m_datasize;
	
public:
	ArchiveEntry( const wxString& filename=wxEmptyString )
		: m_filename( filename )
	{
		m_dataidx	= 0;
		m_datasize	= 0;
	}

	virtual ~ArchiveEntry() throw() {}

	ArchiveEntry& SetDataIndex( uptr idx )
	{
		m_dataidx = idx;
		return *this;
	}

	ArchiveEntry& SetDataSize( size_t size )
	{
		m_datasize = size;
		return *this;
	}

	wxString GetFilename() const
	{
		return m_filename;
	}
	
	uptr GetDataIndex() const
	{
		return m_dataidx;
	}

	uint GetDataSize() const
	{
		return m_datasize;
	}
};

typedef SafeArray< u8 > ArchiveDataBuffer;

// --------------------------------------------------------------------------------------
//  ArchiveEntryList
// --------------------------------------------------------------------------------------
class ArchiveEntryList
{
	DeclareNoncopyableObject( ArchiveEntryList );

protected:
	std::vector<ArchiveEntry>		m_list;
	ScopedPtr<ArchiveDataBuffer>	m_data;

public:
	virtual ~ArchiveEntryList() throw() {}

	ArchiveEntryList() {}

	ArchiveEntryList( ArchiveDataBuffer* data )
	{
		m_data = data;
	}

	ArchiveEntryList( ArchiveDataBuffer& data )
	{
		m_data = &data;
	}
	
	const VmStateBuffer* GetBuffer() const
	{
		return m_data;
	}

	VmStateBuffer* GetBuffer()
	{
		return m_data;
	}
	
	u8* GetPtr( uint idx )
	{
		return &(*m_data)[idx];
	}

	const u8* GetPtr( uint idx ) const
	{
		return &(*m_data)[idx];
	}

	ArchiveEntryList& Add( const ArchiveEntry& src )
	{
		m_list.push_back( src );
		return *this;
	}
	
	size_t GetLength() const
	{
		return m_list.size();
	}
	
	ArchiveEntry& operator[](uint idx)
	{
		return m_list[idx];
	}
	
	const ArchiveEntry& operator[](uint idx) const
	{
		return m_list[idx];
	}
};

// --------------------------------------------------------------------------------------
//  BaseCompressThread
// --------------------------------------------------------------------------------------
class BaseCompressThread
	: public pxThread
{
	typedef pxThread _parent;

protected:
	pxOutputStream*					m_gzfp;
	ArchiveEntryList*				m_src_list;
	bool							m_PendingSaveFlag;
	
	wxString						m_final_filename;

public:
	virtual ~BaseCompressThread() throw();

	BaseCompressThread& SetSource( ArchiveEntryList* srcdata )
	{
		m_src_list = srcdata;
		return *this;
	}

	BaseCompressThread& SetSource( ArchiveEntryList& srcdata )
	{
		m_src_list = &srcdata;
		return *this;
	}

	BaseCompressThread& SetOutStream( pxOutputStream* out )
	{
		m_gzfp = out;
		return *this;
	}

	BaseCompressThread& SetOutStream( pxOutputStream& out )
	{
		m_gzfp = &out;
		return *this;
	}

	BaseCompressThread& SetFinishedPath( const wxString& path )
	{
		m_final_filename = path;
		return *this;
	}

	wxString GetStreamName() const { return m_gzfp->GetStreamName(); }

	BaseCompressThread& SetTargetFilename(const wxString& filename)
	{
		m_final_filename = filename;
		return *this;
	}
	
protected:
	BaseCompressThread()
	{
		m_PendingSaveFlag	= false;
	}

	void SetPendingSave();
	void ExecuteTaskInThread();
	void OnCleanupInThread();
};
