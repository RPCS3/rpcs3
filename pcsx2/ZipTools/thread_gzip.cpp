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
#include "wx/wfstream.h"


BaseCompressThread::~BaseCompressThread() throw()
{
	_parent::Cancel();
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

void BaseCompressThread::ExecuteTaskInThread()
{
	// TODO : Add an API to PersistentThread for this! :)  --air
	//SetThreadPriority( THREAD_PRIORITY_BELOW_NORMAL );

	// Notes:
	//  * Safeguard against corruption by writing to a temp file, and then copying the final
	//    result over the original.

	if( !m_src_list ) return;
	SetPendingSave();
	
	Yield( 3 );

	uint listlen = m_src_list->GetLength();
	for( uint i=0; i<listlen; ++i )
	{
		const ArchiveEntry& entry = (*m_src_list)[i];
		if (!entry.GetDataSize()) continue;

		wxArchiveOutputStream& woot = *(wxArchiveOutputStream*)m_gzfp->GetWxStreamBase();
		woot.PutNextEntry( entry.GetFilename() );

		static const uint BlockSize = 0x64000;
		uint curidx = 0;

		do {
			uint thisBlockSize = std::min( BlockSize, entry.GetDataSize() - curidx );
			m_gzfp->Write(m_src_list->GetPtr( entry.GetDataIndex() + curidx ), thisBlockSize);
			curidx += thisBlockSize;
			Yield( 2 );
		} while( curidx < entry.GetDataSize() );
		
		woot.CloseEntry();
	}

	m_gzfp->Close();

	if( !wxRenameFile( m_gzfp->GetStreamName(), m_final_filename, true ) )
		throw Exception::BadStream( m_final_filename )
		.SetDiagMsg(L"Failed to move or copy the temporary archive to the destination filename.")
		.SetUserMsg(_("The savestate was not properly saved. The temporary file was created successfully but could not be moved to its final resting place."));

	Console.WriteLn( "(gzipThread) Data saved to disk without error." );
}

void BaseCompressThread::OnCleanupInThread()
{
	_parent::OnCleanupInThread();
	wxGetApp().DeleteThread( this );

	safe_delete(m_gzfp);
	safe_delete(m_src_list);
}

