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
#include "App.h"
#include "Dialogs/ModalPopups.h"

using namespace pxSizerFlags;
using namespace Threading;

// NOTE: Currently unused module.  Stuck Threads are not detected or handled at this time,
// though I would like to have something in place in the distant future. --air


Dialogs::StuckThreadDialog::StuckThreadDialog( wxWindow* parent, StuckThreadActionType action, PersistentThread& stuck_thread )
	: wxDialogWithHelpers( parent, _("PCSX2 Thread is not responding"), wxVERTICAL )
{
	stuck_thread.AddListener( this );

	*this += Heading( wxsFormat(
		pxE( ".Panel:StuckThread:Heading",
			L"The thread '%s' is not responding.  It could be deadlocked, or it might "
			L"just be running *really* slowly."
		),
		stuck_thread.GetName().data()
	) );


	*this += Heading(
		L"\nDo you want to stop the program [Yes/No]?"
		L"\nOr press [Ignore] to suppress further assertions."
	);

	*this += new ModalButtonPanel( this, MsgButtons().Cancel().Custom(L"Wait") ) | StdCenter();

	if( wxWindow* idyes = FindWindowById( wxID_YES ) )
		idyes->SetFocus();
}

void Dialogs::StuckThreadDialog::OnThreadCleanup()
{
}
