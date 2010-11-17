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
//  BaseCompressThread
// --------------------------------------------------------------------------------------
class BaseCompressThread
	: public pxThread
{
	typedef pxThread _parent;

protected:
	ScopedPtr< SafeArray< u8 > >	m_src_buffer;
	ScopedPtr< pxStreamWriter >		m_gzfp;
	bool							m_PendingSaveFlag;
	
	wxString						m_final_filename;

	BaseCompressThread( SafeArray<u8>* srcdata, pxStreamWriter* outarchive)
		: m_src_buffer( srcdata )
	{
		m_gzfp				= outarchive;
		m_PendingSaveFlag	= false;
	}

	BaseCompressThread( SafeArray<u8>* srcdata, ScopedPtr<pxStreamWriter>& outarchive)
		: m_src_buffer( srcdata )
	{
		m_gzfp				= outarchive.DetachPtr();
		m_PendingSaveFlag	= false;
	}
	
	virtual ~BaseCompressThread() throw();
	
	void SetPendingSave();
	void ExecuteTaskInThread();
	void OnCleanupInThread();
	
public:
	wxString GetStreamName() const { return m_gzfp->GetStreamName(); }

	BaseCompressThread& SetTargetFilename(const wxString& filename)
	{
		m_final_filename = filename;
		return *this;
	}
};
