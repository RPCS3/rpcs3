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

using namespace Threading;
using namespace pxSizerFlags;

// Thread Safety: Must be called from the GUI thread ONLY.  Will assert otherwise.
//
// [TODO] Add support for icons?
//
static int pxMessageDialog( const wxString& caption, const wxString& content, const MsgButtons& buttons )
{
	if( !AffinityAssert_AllowFrom_MainUI() ) return wxID_CANCEL;

	// fixme: If the emulator is currently active and is running in exclusive mode (forced
	// fullscreen), then we need to either:
	//  1) Exit fullscreen mode before issuing the popup.
	//  2) Issue the popup with wxSTAY_ON_TOP specified so that the user will see it.
	//
	// And in either case the emulation should be paused/suspended for the user.

	wxDialogWithHelpers dialog( NULL, caption );
	dialog += dialog.Heading( content );
	return pxIssueConfirmation( dialog, buttons );
}

// --------------------------------------------------------------------------------------
//  BaseMessageBoxEvent Implementation
// --------------------------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS( BaseMessageBoxEvent, pxInvokeActionEvent )

BaseMessageBoxEvent::BaseMessageBoxEvent( const wxString& content, SynchronousActionState& instdata )
	: m_Content( content )
{
	m_state = &instdata;
}

BaseMessageBoxEvent::BaseMessageBoxEvent( const wxString& content, SynchronousActionState* instdata )
	: m_Content( content )
{
	m_state = instdata;
}

BaseMessageBoxEvent::BaseMessageBoxEvent( const BaseMessageBoxEvent& event )
	: m_Content( event.m_Content )
{
	m_state = event.m_state;
}

// Thread Safety: Must be called from the GUI thread ONLY.
void BaseMessageBoxEvent::InvokeEvent()
{
	int result = _DoDialog();
	if( m_state ) m_state->PostResult( result );
}

int BaseMessageBoxEvent::_DoDialog() const
{
	pxFailDev( "Abstract Base MessageBox Event." );
	return wxID_CANCEL;
}

// --------------------------------------------------------------------------------------
//  pxMessageBoxEvent Implementation
// --------------------------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS( pxMessageBoxEvent, BaseMessageBoxEvent )

pxMessageBoxEvent::pxMessageBoxEvent( const wxString& title, const wxString& content, const MsgButtons& buttons, SynchronousActionState& instdata )
	: BaseMessageBoxEvent( content, instdata )
	, m_Title( title )
	, m_Buttons( buttons )
{
}

pxMessageBoxEvent::pxMessageBoxEvent( const wxString& title, const wxString& content, const MsgButtons& buttons, SynchronousActionState* instdata )
	: BaseMessageBoxEvent( content, instdata )
	, m_Title( title )
	, m_Buttons( buttons )
{
}

pxMessageBoxEvent::pxMessageBoxEvent( const pxMessageBoxEvent& event )
	: BaseMessageBoxEvent( event )
	, m_Title( event.m_Title )
	, m_Buttons( event.m_Buttons )
{
}

int pxMessageBoxEvent::_DoDialog() const
{
	return pxMessageDialog( m_Title, m_Content, m_Buttons );
}

// --------------------------------------------------------------------------------------
//  pxAssertionEvent Implementation
// --------------------------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS( pxAssertionEvent, BaseMessageBoxEvent )

pxAssertionEvent::pxAssertionEvent( const wxString& content, const wxString& trace, SynchronousActionState& instdata )
	: BaseMessageBoxEvent( content, instdata )
	, m_Stacktrace( trace )
{
}

pxAssertionEvent::pxAssertionEvent( const wxString& content, const wxString& trace, SynchronousActionState* instdata )
	: BaseMessageBoxEvent( content, instdata )
	, m_Stacktrace( trace )
{
}

pxAssertionEvent::pxAssertionEvent( const pxAssertionEvent& event )
	: BaseMessageBoxEvent( event )
	, m_Stacktrace( event.m_Stacktrace )
{
}

pxAssertionEvent& pxAssertionEvent::SetStacktrace( const wxString& trace )
{
	m_Stacktrace = trace;
	return *this;
}

int pxAssertionEvent::_DoDialog() const
{
	return Dialogs::AssertionDialog( m_Content, m_Stacktrace ).ShowModal();
}

namespace Msgbox
{
	int ShowModal( BaseMessageBoxEvent& evt )
	{
		SynchronousActionState instdat;
		evt.SetSyncState( instdat );

		if( wxThread::IsMain() )
		{
			// main thread can handle the message immediately.
			wxGetApp().ProcessEvent( evt );
			return instdat.return_value;
		}
		else
		{
			// Not on main thread, must post the message there for handling instead:
			wxGetApp().AddPendingEvent( evt );
			return instdat.WaitForResult();		// Important! disable cancellation since we're using local stack vars.
		}
	}

	static int ShowModal( const wxString& title, const wxString& content, const MsgButtons& buttons )
	{
		pxMessageBoxEvent tevt( title, content, buttons );
		return ShowModal( tevt );
	}

	// Pops up an alert Dialog Box with a singular "OK" button.
	// Always returns false.
	bool Alert( const wxString& text, const wxString& caption, int icon )
	{
		ShowModal( caption, text, MsgButtons().OK() );
		return false;
	}

	// Pops up a dialog box with Ok/Cancel buttons.  Returns the result of the inquiry,
	// true if OK, false if cancel.
	bool OkCancel( const wxString& text, const wxString& caption, int icon )
	{
		MsgButtons buttons( MsgButtons().OKCancel() );

		if( wxThread::IsMain() )
		{
			return wxID_OK == pxMessageDialog( caption, text, buttons );
		}
		else
		{
			return wxID_OK == ShowModal( caption, text, buttons );
		}
	}

	bool YesNo( const wxString& text, const wxString& caption, int icon )
	{
		MsgButtons buttons( MsgButtons().YesNo() );

		if( wxThread::IsMain() )
		{
			return wxID_YES == pxMessageDialog( caption, text, buttons );
		}
		else
		{
			return wxID_YES == ShowModal( caption, text, buttons );
		}
	}

	int Assertion( const wxString& text, const wxString& stacktrace )
	{
		pxAssertionEvent tevt( text, stacktrace );
		return ShowModal( tevt );
	}
}
