/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of te License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"

#include "App.h"
#include "SaveState.h"
#include "ThreadedZipTools.h"
#include "Utilities/SafeArray.inl"


BaseCompressThread::~BaseCompressThread() throw()
{
	if( m_PendingSaveFlag )
	{
		wxGetApp().ClearPendingSave();
		m_PendingSaveFlag = false;
	}
}

void BaseCompressThread::SetPendingSave()
{
	wxGetApp().StartPendingSave();
	m_PendingSaveFlag = true;
}

CompressThread_gzip::CompressThread_gzip( const wxString& file, SafeArray<u8>* srcdata, FnType_WriteCompressedHeader* writeheader )
	: BaseCompressThread( file, srcdata, writeheader )
{
	m_gzfp	= NULL;
}

// Believe it or not, the space in > > is required.
CompressThread_gzip::CompressThread_gzip( const wxString& file, ScopedPtr< SafeArray<u8> >& srcdata, FnType_WriteCompressedHeader* writeheader )
	: BaseCompressThread( file, srcdata.DetachPtr(), writeheader )
{
	m_gzfp	= NULL;
}

CompressThread_gzip::~CompressThread_gzip() throw()
{
	_parent::Cancel();

	if( m_gzfp ) gzclose( m_gzfp );
}

void CompressThread_gzip::Write( const void* data, size_t size )
{
	if( gzwrite( m_gzfp, data, size ) == 0 )
		throw Exception::BadStream( m_filename ).SetDiagMsg(L"Write to zip file failed.");
}

void CompressThread_gzip::ExecuteTaskInThread()
{
	// TODO : Add an API to PersistentThread for this! :)  --air
	//SetThreadPriority( THREAD_PRIORITY_BELOW_NORMAL );

	if( !m_src_buffer ) return;

	SetPendingSave();

	Yield( 3 );

	// Safeguard against corruption by writing to a temp file, and then
	// copying the final result over the original:
	
	wxString tempfile( m_filename + L".tmp" );

	if( !(m_gzfp = gzopen(tempfile.ToUTF8(), "wb")) )
		throw Exception::CannotCreateStream( m_filename );

	gzsetparams(m_gzfp, Z_BEST_SPEED, Z_FILTERED); // Best speed at good compression

#if defined(ZLIB_VERNUM) && (ZLIB_VERNUM >= 0x1240)
	gzbuffer(m_gzfp, 0x100000); // 1mb buffer size for less file fragments (Windows/NTFS)
#endif

	if( m_WriteHeaderInThread )
		m_WriteHeaderInThread( *this );

	static const int BlockSize = 0x64000;
	int curidx = 0;

	do {
		int thisBlockSize = std::min( BlockSize, m_src_buffer->GetSizeInBytes() - curidx );
		if( gzwrite( m_gzfp, m_src_buffer->GetPtr(curidx), thisBlockSize ) < thisBlockSize )
			throw Exception::BadStream( m_filename );
		curidx += thisBlockSize;
		Yield( 2 );
	} while( curidx < m_src_buffer->GetSizeInBytes() );

	gzclose( m_gzfp );
	m_gzfp = NULL;

	if( !wxRenameFile( tempfile, m_filename, true ) )
		throw Exception::BadStream( m_filename )
			.SetDiagMsg(L"Failed to move or copy the temporary archive to the destination filename.")
			.SetUserMsg(_("The savestate was not properly saved. The temporary file was created successfully but could not be moved to its final resting place."));

	Console.WriteLn( "(gzipThread) Data saved to disk without error." );
}

void CompressThread_gzip::OnCleanupInThread()
{
	_parent::OnCleanupInThread();
	wxGetApp().DeleteThread( this );
}

