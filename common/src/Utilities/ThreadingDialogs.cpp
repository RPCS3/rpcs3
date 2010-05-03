/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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
#include "ThreadingDialogs.h"
#include "pxStaticText.h"


DEFINE_EVENT_TYPE(pxEvt_ThreadedTaskComplete);

// --------------------------------------------------------------------------------------
//  WaitForTaskDialog Implementations
// --------------------------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS(WaitForTaskDialog, wxDialogWithHelpers)

Threading::WaitForTaskDialog::WaitForTaskDialog( const wxString& title, const wxString& heading )
	: wxDialogWithHelpers( NULL, _("Waiting for tasks..."), wxVERTICAL )
	//, m_Timer(this)
{
	//m_sem	= sem;
	//m_mutex	= mutex;

	wxString m_title( title );
	wxString m_heading( heading );

	if( m_title.IsEmpty() )		m_title = _("Waiting for task...");
	if( m_heading.IsEmpty() )	m_heading = m_title;

	Connect( pxEvt_ThreadedTaskComplete, wxCommandEventHandler(WaitForTaskDialog::OnTaskComplete) );

	wxBoxSizer& paddedMsg( *new wxBoxSizer( wxHORIZONTAL ) );
	paddedMsg += 24;
	paddedMsg += Heading(m_heading);
	paddedMsg += 24;

	*this += 12;
	*this += paddedMsg;
	*this += 12;
	
	// TODO : Implement a cancel button.  Not quite sure the best way to do
	// that, since it requires a thread or event handler context, or something.

	//applyDlg += new wxButton( &applyDlg, wxID_CANCEL )	| pxCenter;
	//applyDlg += 6;

	//Connect( m_Timer.GetId(),	wxEVT_TIMER, wxTimerEventHandler(WaitForTaskDialog::OnTimer) );
	//m_Timer.Start( 200 );
	//GetSysExecutorThread().PostEvent( new SysExecEvent_ApplyPlugins( this, m_sync ) );
}

void Threading::WaitForTaskDialog::OnTaskComplete( wxCommandEvent& evt )
{
	evt.Skip();

	// Note: we don't throw exceptions from the pending task here.
	// Instead we wait until we exit the modal loop below -- this gives
	// the caller a chance to handle the exception themselves, and if
	// not the exception will still fall back on the standard app-level
	// exception handler.

	// (this also avoids any sticky business with the modal dialog not getting
	// closed out right due to stack unwinding skipping dialog closure crap)

	m_sync.WaitForResult_NoExceptions();
	EndModal( wxID_OK );
}

int Threading::WaitForTaskDialog::ShowModal()
{
	int result = _parent::ShowModal();
	m_sync.RethrowException();
	return result;
}
