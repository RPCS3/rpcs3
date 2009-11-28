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
#include "App.h"

DEFINE_EVENT_TYPE( pxEVT_MSGBOX );
DEFINE_EVENT_TYPE( pxEVT_CallStackBox );

using namespace Threading;

// Thread Safety: Must be called from the GUI thread ONLY.
static int pxMessageDialog( const wxString& content, const wxString& caption, long flags )
{
	if( IsDevBuild && !wxThread::IsMain() )
		throw Exception::InvalidOperation( "Function must be called by the main GUI thread only." );

	// fixme: If the emulator is currently active and is running in fullscreen mode, then we
	// need to either:
	//  1) Exit fullscreen mode before issuing the popup.
	//  2) Issue the popup with wxSTAY_ON_TOP specified so that the user will see it.
	//
	// And in either case the emulation should be paused/suspended for the user.

	return wxMessageDialog( NULL, content, caption, flags ).ShowModal();
}

// Thread Safety: Must be called from the GUI thread ONLY.
// fixme: this function should use a custom dialog box that has a wxTextCtrl for the callstack, and
// uses fixed-width (modern) fonts.
static int pxCallstackDialog( const wxString& content, const wxString& caption, long flags )
{
	if( IsDevBuild && !wxThread::IsMain() )
		throw Exception::InvalidOperation( "Function must be called by the main GUI thread only." );

	return wxMessageDialog( NULL, content, caption, flags ).ShowModal();
}

// --------------------------------------------------------------------------------------
//  pxMessageBoxEvent
// --------------------------------------------------------------------------------------
class pxMessageBoxEvent : public wxEvent
{
protected:
	MsgboxEventResult&	m_Instdata;
	wxString			m_Title;
	wxString			m_Content;
	long				m_Flags;

public:
	pxMessageBoxEvent()
		: wxEvent( 0, pxEVT_MSGBOX )
		, m_Instdata( *(MsgboxEventResult*)NULL )
		, m_Title()
		, m_Content()
	{
		m_Flags = 0;
	}

	pxMessageBoxEvent( MsgboxEventResult& instdata, const wxString& title, const wxString& content, long flags )
		: wxEvent( 0, pxEVT_MSGBOX )
		, m_Instdata( instdata )
		, m_Title( title )
		, m_Content( content )
	{
		m_Flags = flags;
	}

	pxMessageBoxEvent( const pxMessageBoxEvent& event )
		: wxEvent( event )
		, m_Instdata( event.m_Instdata )
		, m_Title( event.m_Title )
		, m_Content( event.m_Content )
	{
		m_Flags = event.m_Flags;
	}

	// Thread Safety: Must be called from the GUI thread ONLY.
	void DoTheDialog()
	{
		int result;

		if( m_id == pxEVT_MSGBOX )
			result = pxMessageDialog( m_Content, m_Title, m_Flags );
		else
			result = pxCallstackDialog( m_Content, m_Title, m_Flags );
		m_Instdata.result = result;
		m_Instdata.WaitForMe.Post();
	}

	virtual wxEvent *Clone() const { return new pxMessageBoxEvent(*this); }

private:
	DECLARE_DYNAMIC_CLASS_NO_ASSIGN(pxMessageBoxEvent)
};

IMPLEMENT_DYNAMIC_CLASS( pxMessageBoxEvent, wxEvent )

namespace Msgbox
{
	// parameters:
	//   flags - messagebox type flags, such as wxOK, wxCANCEL, etc.
	//
	static int ThreadedMessageBox( const wxString& content, const wxString& title, long flags, int boxType=pxEVT_MSGBOX )
	{
		// must pass the message to the main gui thread, and then stall this thread, to avoid
		// threaded chaos where our thread keeps running while the popup is awaiting input.

		MsgboxEventResult instdat;
		pxMessageBoxEvent tevt( instdat, title, content, flags );
		wxGetApp().AddPendingEvent( tevt );
		instdat.WaitForMe.WaitNoCancel();		// Important! disable cancellation since we're using local stack vars.
		return instdat.result;
	}

	void OnEvent( pxMessageBoxEvent& evt )
	{
		evt.DoTheDialog();
	}

	// Pops up an alert Dialog Box with a singular "OK" button.
	// Always returns false.
	bool Alert( const wxString& text, const wxString& caption, int icon )
	{
		icon |= wxOK;
		if( wxThread::IsMain() )
			pxMessageDialog( text, caption, icon );
		else
			ThreadedMessageBox( text, caption, icon );
		return false;
	}

	// Pops up a dialog box with Ok/Cancel buttons.  Returns the result of the inquiry,
	// true if OK, false if cancel.
	bool OkCancel( const wxString& text, const wxString& caption, int icon )
	{
		icon |= wxOK | wxCANCEL;
		if( wxThread::IsMain() )
		{
			return wxID_OK == pxMessageDialog( text, caption, icon );
		}
		else
		{
			return wxID_OK == ThreadedMessageBox( text, caption, icon );
		}
	}

	bool YesNo( const wxString& text, const wxString& caption, int icon )
	{
		icon |= wxYES_NO;
		if( wxThread::IsMain() )
		{
			return wxID_YES == pxMessageDialog( text, caption, icon );
		}
		else
		{
			return wxID_YES == ThreadedMessageBox( text, caption, icon );
		}
	}

	// [TODO] : This should probably be a fancier looking dialog box with the stacktrace
	// displayed inside a wxTextCtrl.
	static int CallStack( const wxString& errormsg, const wxString& stacktrace, const wxString& prompt, const wxString& caption, int buttons )
	{
		buttons |= wxICON_STOP;

		wxString text( errormsg + L"\n\n" + stacktrace + L"\n" + prompt );

		if( wxThread::IsMain() )
		{
			return pxCallstackDialog( text, caption, buttons );
		}
		else
		{
			return ThreadedMessageBox( text, caption, buttons, pxEVT_CallStackBox );
		}
	}

	int Assertion( const wxString& text, const wxString& stacktrace )
	{
		return CallStack( text, stacktrace,
			L"\nDo you want to stop the program?"
			L"\nOr press [Cancel] to suppress further assertions.",
			L"PCSX2 Assertion Failure",
			wxYES_NO | wxCANCEL
		);
	}

	void Except( const Exception::BaseException& src )
	{
		CallStack( src.FormatDisplayMessage(), src.FormatDiagnosticMessage(), wxEmptyString, L"PCSX2 Unhandled Exception", wxOK );
	}
}
