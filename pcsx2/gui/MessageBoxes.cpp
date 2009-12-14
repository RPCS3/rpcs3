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
#include "Dialogs/ModalPopups.h"

DEFINE_EVENT_TYPE( pxEVT_MSGBOX );
DEFINE_EVENT_TYPE( pxEVT_ASSERTION );

using namespace Threading;
using namespace pxSizerFlags;

// Thread Safety: Must be called from the GUI thread ONLY.  Will assert otherwise.
//
// [TODO] Add support for icons?
//
static int pxMessageDialog( const wxString& caption, const wxString& content, const MsgButtons& buttons )
{
	if( !AffinityAssert_AllowFromMain() ) return wxID_CANCEL;

	// fixme: If the emulator is currently active and is running in exclusive mode (forced
	// fullscreen), then we need to either:
	//  1) Exit fullscreen mode before issuing the popup.
	//  2) Issue the popup with wxSTAY_ON_TOP specified so that the user will see it.
	//
	// And in either case the emulation should be paused/suspended for the user.

	wxDialogWithHelpers dialog( NULL, caption, wxVERTICAL );
	dialog += dialog.Heading( content );
	return pxIssueConfirmation( dialog, buttons );
}

class BaseMessageBoxEvent : public wxEvent
{
	DECLARE_DYNAMIC_CLASS_NO_ASSIGN(BaseMessageBoxEvent)

protected:
	MsgboxEventResult*	m_Instdata;
	wxString			m_Content;

public:
	explicit BaseMessageBoxEvent( int msgtype=pxEVT_MSGBOX, const wxString& content=wxEmptyString )
		: wxEvent( 0, msgtype )
		, m_Content( content )
	{
		m_Instdata = NULL;
	}

	virtual ~BaseMessageBoxEvent() throw() { }
	virtual BaseMessageBoxEvent *Clone() const { return new BaseMessageBoxEvent(*this); }

	BaseMessageBoxEvent( MsgboxEventResult& instdata, const wxString& content )
		: wxEvent( 0, pxEVT_MSGBOX )
		, m_Instdata( &instdata )
		, m_Content( content )
	{
	}

	BaseMessageBoxEvent( const wxString& content )
		: wxEvent( 0, pxEVT_MSGBOX )
		, m_Instdata( NULL )
		, m_Content( content )
	{
	}

	BaseMessageBoxEvent( const BaseMessageBoxEvent& event )
		: wxEvent( event )
		, m_Instdata( event.m_Instdata )
		, m_Content( event.m_Content )
	{
	}

	BaseMessageBoxEvent& SetInstData( MsgboxEventResult& instdata )
	{
		m_Instdata = &instdata;
		return *this;
	}

	// Thread Safety: Must be called from the GUI thread ONLY.
	virtual void IssueDialog()
	{
		AffinityAssert_AllowFromMain();

		int result = _DoDialog();

		if( m_Instdata != NULL )
		{
			m_Instdata->result = result;
			m_Instdata->WaitForMe.Post();
		}
	}

protected:
	virtual int _DoDialog() const
	{
		pxFailDev( "Abstract Base MessageBox Event." );
		return wxID_CANCEL;
	}
};

// --------------------------------------------------------------------------------------
//  pxMessageBoxEvent
// --------------------------------------------------------------------------------------
// This event type is used to transfer message boxes to the main UI thread, and return the
// result of the box.  It's the only way a message box can be issued from non-main threads
// with complete safety in wx2.8.
//
// For simplicity sake this message box only supports two basic designs.  The main design
// is a generic message box with confirmation buttons of your choosing.  Additionally you
// can specify a "scrollableContent" text string, which is added into a read-only richtext
// control similar to the console logs and such.
//
// Future consideration: If wxWidgets 3.0 has improved thread safety, then it should probably
// be reasonable for it to work with a more flexable model where the dialog can be created
// on a child thread, passed to the main thread, where ShowModal() is run (keeping the nested
// message pumps on the main thread where they belong).  But so far this is not possible,
// because of various subtle issues in wx2.8 design.
//
class pxMessageBoxEvent : public BaseMessageBoxEvent
{
	typedef BaseMessageBoxEvent _parent;
	DECLARE_DYNAMIC_CLASS_NO_ASSIGN(pxMessageBoxEvent)

protected:
	wxString			m_Title;
	MsgButtons			m_Buttons;
	
public:
	pxMessageBoxEvent( int msgtype=pxEVT_MSGBOX )
		: BaseMessageBoxEvent( msgtype )
	{
	}

	virtual ~pxMessageBoxEvent() throw() { }
	virtual pxMessageBoxEvent *Clone() const { return new pxMessageBoxEvent(*this); }

	pxMessageBoxEvent( MsgboxEventResult& instdata, const wxString& title, const wxString& content, const MsgButtons& buttons )
		: BaseMessageBoxEvent( instdata, content )
		, m_Title( title )
		, m_Buttons( buttons )
	{
	}

	pxMessageBoxEvent( const wxString& title, const wxString& content, const MsgButtons& buttons )
		: BaseMessageBoxEvent( content )
		, m_Title( title )
		, m_Buttons( buttons )
	{
	}

	pxMessageBoxEvent( const pxMessageBoxEvent& event )
		: BaseMessageBoxEvent( event )
		, m_Title( event.m_Title )
		, m_Buttons( event.m_Buttons )
	{
	}

	pxMessageBoxEvent& SetInstData( MsgboxEventResult& instdata )
	{
		_parent::SetInstData( instdata );
		return *this;
	}

protected:
	virtual int _DoDialog() const
	{
		return pxMessageDialog( m_Content, m_Title, m_Buttons );
	}
};

// --------------------------------------------------------------------------------------
//  pxAssertionEvent
// --------------------------------------------------------------------------------------
class pxAssertionEvent : public BaseMessageBoxEvent
{
	typedef BaseMessageBoxEvent _parent;
	DECLARE_DYNAMIC_CLASS_NO_ASSIGN( pxAssertionEvent )

protected:
	wxString	m_Stacktrace;

public:
	pxAssertionEvent()
		: BaseMessageBoxEvent( pxEVT_ASSERTION )
	{
	}
	
	virtual ~pxAssertionEvent() throw() { }

	virtual pxAssertionEvent *Clone() const { return new pxAssertionEvent(*this); }

	pxAssertionEvent( MsgboxEventResult& instdata, const wxString& content, const wxString& trace )
		: BaseMessageBoxEvent( pxEVT_ASSERTION )
		, m_Stacktrace( trace )
	{
	}

	pxAssertionEvent( const wxString& content, const wxString& trace )
		: BaseMessageBoxEvent( pxEVT_ASSERTION, content )
		, m_Stacktrace( trace )
	{
	}

	pxAssertionEvent( const pxAssertionEvent& event )
		: BaseMessageBoxEvent( event )
		, m_Stacktrace( event.m_Stacktrace )
	{
	}

	pxAssertionEvent& SetInstData( MsgboxEventResult& instdata )
	{
		_parent::SetInstData( instdata );
		return *this;
	}

	pxAssertionEvent& SetStacktrace( const wxString& trace )
	{
		m_Stacktrace = trace;
		return *this;
	}

protected:
	virtual int _DoDialog() const
	{
		return Dialogs::AssertionDialog( m_Content, m_Stacktrace ).ShowModal();
	}
};

IMPLEMENT_DYNAMIC_CLASS( BaseMessageBoxEvent, wxEvent )
IMPLEMENT_DYNAMIC_CLASS( pxMessageBoxEvent, BaseMessageBoxEvent )
IMPLEMENT_DYNAMIC_CLASS( pxAssertionEvent, BaseMessageBoxEvent )

namespace Msgbox
{
	static int ThreadedMessageBox( BaseMessageBoxEvent& evt )
	{
		MsgboxEventResult instdat;
		evt.SetInstData( instdat );

		if( wxThread::IsMain() )
		{
			// main thread can handle the message immediately.
			wxGetApp().ProcessEvent( evt );
		}
		else
		{
			// Not on main thread, must post the message there for handling instead:
			wxGetApp().AddPendingEvent( evt );
			instdat.WaitForMe.WaitNoCancel();		// Important! disable cancellation since we're using local stack vars.
		}
		return instdat.result;
	}

	static int ThreadedMessageBox( const wxString& title, const wxString& content, const MsgButtons& buttons )
	{
		// must pass the message to the main gui thread, and then stall this thread, to avoid
		// threaded chaos where our thread keeps running while the popup is awaiting input.

		MsgboxEventResult instdat;
		pxMessageBoxEvent tevt( instdat, title, content, buttons );
		wxGetApp().AddPendingEvent( tevt );
		instdat.WaitForMe.WaitNoCancel();		// Important! disable cancellation since we're using local stack vars.
		return instdat.result;
	}

	// Pops up an alert Dialog Box with a singular "OK" button.
	// Always returns false.
	bool Alert( const wxString& text, const wxString& caption, int icon )
	{
		MsgButtons buttons( MsgButtons().OK() );

		if( wxThread::IsMain() )
			pxMessageDialog( caption, text, buttons );
		else
			ThreadedMessageBox( caption, text, buttons );
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
			return wxID_OK == ThreadedMessageBox( caption, text, buttons );
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
			return wxID_YES == ThreadedMessageBox( caption, text, buttons );
		}
	}

	int Assertion( const wxString& text, const wxString& stacktrace )
	{
		pxAssertionEvent tevt( text, stacktrace );
		return ThreadedMessageBox( tevt );
	}
}

void Pcsx2App::OnMessageBox( pxMessageBoxEvent& evt )
{
	evt.IssueDialog();
}
