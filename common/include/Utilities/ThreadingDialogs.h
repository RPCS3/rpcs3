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

#include "Threading.h"
#include "wxAppWithHelpers.h"

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE(pxEvt_ThreadedTaskComplete, -1)
END_DECLARE_EVENT_TYPES()

namespace Threading
{
	// --------------------------------------------------------------------------------------
	//  WaitForTaskDialog
	// --------------------------------------------------------------------------------------
	// This dialog is displayed whenever the main thread is recursively waiting on multiple
	// mutexes or semaphores.  wxwidgets does not support recursive yielding to pending events
	// but it *does* support opening a modal dialog, which disables the interface (preventing
	// the user from starting additional actions), and processes messages (allowing the system
	// to continue to manage threads and process logging).
	//
	class WaitForTaskDialog : public wxDialogWithHelpers
	{
		DECLARE_DYNAMIC_CLASS_NO_COPY(WaitForTaskDialog)

		typedef wxDialogWithHelpers _parent;

	protected:
		SynchronousActionState	m_sync;

	public:
		WaitForTaskDialog( const wxString& title=wxEmptyString, const wxString& heading=wxEmptyString );
		virtual ~WaitForTaskDialog() throw() {}
		virtual int ShowModal();

	protected:
		void OnTaskComplete( wxCommandEvent& evt );
		//void OnTimer( wxTimerEvent& evt );
	};

}
