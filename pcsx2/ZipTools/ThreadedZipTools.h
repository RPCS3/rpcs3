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
//#include <zlib/zlib.h>

using namespace Threading;

class IStreamWriter
{
public:
	virtual ~IStreamWriter() throw() {}

	virtual void Write( const void* data, size_t size )=0;
	virtual wxString GetStreamName() const=0;

	template< typename T >
	void Write( const T& data )
	{
		Write( &data, sizeof(data) );
	}
};

class IStreamReader
{
public:
	virtual ~IStreamReader() throw() {}

	virtual void Read( void* dest, size_t size )=0;
	virtual wxString GetStreamName() const=0;

	template< typename T >
	void Read( T& dest )
	{
		Read( &dest, sizeof(dest) );
	}
};

typedef void FnType_WriteCompressedHeader( IStreamWriter& thr );
typedef void FnType_ReadCompressedHeader( IStreamReader& thr );

// --------------------------------------------------------------------------------------
//  BaseCompressThread
// --------------------------------------------------------------------------------------
class BaseCompressThread
	: public pxThread
	, public IStreamWriter
{
	typedef pxThread _parent;

protected:
	FnType_WriteCompressedHeader*	m_WriteHeaderInThread;

	const wxString					m_filename;
	ScopedPtr< SafeArray< u8 > >	m_src_buffer;
	bool							m_PendingSaveFlag;

	BaseCompressThread( const wxString& file, SafeArray<u8>* srcdata, FnType_WriteCompressedHeader* writeHeader=NULL)
		: m_filename( file )
		, m_src_buffer( srcdata )
	{
		m_WriteHeaderInThread	= writeHeader;
		m_PendingSaveFlag		= false;
	}

	virtual ~BaseCompressThread() throw();
	
	void SetPendingSave();

public:
	wxString GetStreamName() const { return m_filename; }
};

// --------------------------------------------------------------------------------------
//   CompressThread_gzip
// --------------------------------------------------------------------------------------
class CompressThread_gzip : public BaseCompressThread
{
	typedef BaseCompressThread _parent;

protected:
	gzFile		m_gzfp;

public:
	CompressThread_gzip( const wxString& file, SafeArray<u8>* srcdata, FnType_WriteCompressedHeader* writeHeader=NULL );
	CompressThread_gzip( const wxString& file, ScopedPtr<SafeArray<u8> >& srcdata, FnType_WriteCompressedHeader* writeHeader=NULL );
	virtual ~CompressThread_gzip() throw();

protected:
	void Write( const void* data, size_t size );
	void ExecuteTaskInThread();
	void OnCleanupInThread();
};
