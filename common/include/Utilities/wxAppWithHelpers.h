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

#pragma once

#include <wx/wx.h>

#include "Threading.h"
#include "wxGuiTools.h"

using namespace Threading;

class pxPingEvent;
class pxMessageBoxEvent;

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE( pxEvt_Ping, -1 )
	DECLARE_EVENT_TYPE( pxEvt_IdleEventQueue, -1 )
	DECLARE_EVENT_TYPE( pxEvt_MessageBox, -1 )
	DECLARE_EVENT_TYPE( pxEvt_DeleteObject, -1 )
	//DECLARE_EVENT_TYPE( pxEvt_Assertion, -1 )
END_DECLARE_EVENT_TYPES()

struct MsgboxEventResult
{
	Semaphore	WaitForMe;
	int			result;

	MsgboxEventResult()
	{
		result = 0;
	}
};

// --------------------------------------------------------------------------------------
//  MsgButtons
// --------------------------------------------------------------------------------------
class MsgButtons
{
protected:
	BITFIELD32()
		bool
			m_OK		:1,
			m_Cancel	:1,
			m_Yes		:1,
			m_No		:1,
			m_AllowToAll:1,
			m_Apply		:1,
			m_Abort		:1,
			m_Retry		:1,
			m_Ignore	:1,
			m_Reset		:1,
			m_Close		:1;
	BITFIELD_END

	wxString m_CustomLabel;

public:
	MsgButtons() { bitset = 0; }

	MsgButtons& OK()		{ m_OK			= true; return *this; }
	MsgButtons& Cancel()	{ m_Cancel		= true; return *this; }
	MsgButtons& Apply()		{ m_Apply		= true; return *this; }
	MsgButtons& Yes()		{ m_Yes			= true; return *this; }
	MsgButtons& No()		{ m_No			= true; return *this; }
	MsgButtons& ToAll()		{ m_AllowToAll	= true; return *this; }

	MsgButtons& Abort()		{ m_Abort		= true; return *this; }
	MsgButtons& Retry()		{ m_Retry		= true; return *this; }
	MsgButtons& Ignore()	{ m_Ignore		= true; return *this; }
	MsgButtons& Reset()		{ m_Reset		= true; return *this; }
	MsgButtons& Close()		{ m_Close		= true; return *this; }

	MsgButtons& Custom( const wxString& label)
	{
		m_CustomLabel = label;
		return *this;
	}

	MsgButtons& OKCancel()	{ m_OK = m_Cancel = true; return *this; }
	MsgButtons& YesNo()		{ m_Yes = m_No = true; return *this; }

	bool HasOK() const		{ return m_OK; }
	bool HasCancel() const	{ return m_Cancel; }
	bool HasApply() const	{ return m_Apply; }
	bool HasYes() const		{ return m_Yes; }
	bool HasNo() const		{ return m_No; }
	bool AllowsToAll() const{ return m_AllowToAll; }

	bool HasAbort() const	{ return m_Abort; }
	bool HasRetry() const	{ return m_Retry; }
	bool HasIgnore() const	{ return m_Ignore; }
	bool HasReset() const	{ return m_Reset; }
	bool HasClose() const	{ return m_Close; }

	bool HasCustom() const	{ return !m_CustomLabel.IsEmpty(); }
	const wxString& GetCustomLabel() const { return m_CustomLabel; }

	bool Allows( wxWindowID id ) const;
	void SetBestFocus( wxWindow* dialog ) const;
	void SetBestFocus( wxWindow& dialog ) const;

	bool operator ==( const MsgButtons& right ) const
	{
		return OpEqu( bitset );
	}

	bool operator !=( const MsgButtons& right ) const
	{
		return !OpEqu( bitset );
	}
};

// --------------------------------------------------------------------------------------
//  ModalButtonPanel
// --------------------------------------------------------------------------------------
class ModalButtonPanel : public wxPanelWithHelpers
{
public:
	ModalButtonPanel( wxWindow* window, const MsgButtons& buttons );
	virtual ~ModalButtonPanel() throw() { }

	virtual void AddActionButton( wxWindowID id );
	virtual void AddCustomButton( wxWindowID id, const wxString& label );

	virtual void OnActionButtonClicked( wxCommandEvent& evt );
};

// --------------------------------------------------------------------------------------
//  BaseMessageBoxEvent
// --------------------------------------------------------------------------------------
class BaseMessageBoxEvent : public wxEvent
{
	DECLARE_DYNAMIC_CLASS_NO_ASSIGN(BaseMessageBoxEvent)

protected:
	MsgboxEventResult*	m_Instdata;
	wxString			m_Content;

public:
	virtual ~BaseMessageBoxEvent() throw() { }
	virtual BaseMessageBoxEvent *Clone() const { return new BaseMessageBoxEvent(*this); }

	explicit BaseMessageBoxEvent( int msgtype=pxEvt_MessageBox, const wxString& content=wxEmptyString );
	BaseMessageBoxEvent( MsgboxEventResult& instdata, const wxString& content );
	BaseMessageBoxEvent( const wxString& content );
	BaseMessageBoxEvent( const BaseMessageBoxEvent& event );

	BaseMessageBoxEvent& SetInstData( MsgboxEventResult& instdata );

	virtual void IssueDialog();

protected:
	virtual int _DoDialog() const;
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
	virtual ~pxMessageBoxEvent() throw() { }
	virtual pxMessageBoxEvent *Clone() const { return new pxMessageBoxEvent(*this); }

	explicit pxMessageBoxEvent( int msgtype=pxEvt_MessageBox );

	pxMessageBoxEvent( MsgboxEventResult& instdata, const wxString& title, const wxString& content, const MsgButtons& buttons );
	pxMessageBoxEvent( const wxString& title, const wxString& content, const MsgButtons& buttons );
	pxMessageBoxEvent( const pxMessageBoxEvent& event );

	pxMessageBoxEvent& SetInstData( MsgboxEventResult& instdata );

protected:
	virtual int _DoDialog() const;
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
	virtual ~pxAssertionEvent() throw() { }
	virtual pxAssertionEvent *Clone() const { return new pxAssertionEvent(*this); }

	pxAssertionEvent();
	pxAssertionEvent( MsgboxEventResult& instdata, const wxString& content, const wxString& trace );
	pxAssertionEvent( const wxString& content, const wxString& trace );
	pxAssertionEvent( const pxAssertionEvent& event );

	pxAssertionEvent& SetInstData( MsgboxEventResult& instdata );
	pxAssertionEvent& SetStacktrace( const wxString& trace );

protected:
	virtual int _DoDialog() const;

};

// --------------------------------------------------------------------------------------
//  pxStuckThreadEvent
// --------------------------------------------------------------------------------------
class pxStuckThreadEvent : public BaseMessageBoxEvent
{
	typedef BaseMessageBoxEvent _parent;
	DECLARE_DYNAMIC_CLASS_NO_ASSIGN( pxStuckThreadEvent )

protected:
	Threading::PersistentThread&	m_Thread;

public:
	virtual ~pxStuckThreadEvent() throw() { }
	virtual pxStuckThreadEvent *Clone() const { return new pxStuckThreadEvent(*this); }

	pxStuckThreadEvent();
	pxStuckThreadEvent( PersistentThread& thr );
	pxStuckThreadEvent( MsgboxEventResult& instdata, PersistentThread& thr );
	pxStuckThreadEvent( const pxStuckThreadEvent& src);

protected:
	virtual int _DoDialog() const;
};

// --------------------------------------------------------------------------------------
//  pxPingEvent
// --------------------------------------------------------------------------------------
class pxPingEvent : public wxEvent
{
	DECLARE_DYNAMIC_CLASS_NO_ASSIGN(pxPingEvent)

protected:
	Semaphore*	m_PostBack;

public:
	virtual ~pxPingEvent() throw() { }
	virtual pxPingEvent *Clone() const { return new pxPingEvent(*this); }

	explicit pxPingEvent( int msgtype, Semaphore* sema=NULL );
	explicit pxPingEvent( Semaphore* sema=NULL );
	pxPingEvent( const pxPingEvent& src );

	Semaphore* GetSemaphore() { return m_PostBack; }
};

typedef void FnType_VoidMethod();

// --------------------------------------------------------------------------------------
//  wxAppWithHelpers
// --------------------------------------------------------------------------------------
class wxAppWithHelpers : public wxApp
{
	typedef wxApp _parent;

	DECLARE_DYNAMIC_CLASS(wxAppWithHelpers)

protected:
	std::vector<Semaphore*>			m_PingWhenIdle;
	std::vector<IDeletableObject*>	m_DeleteWhenIdle;
	std::vector<wxEvent*>			m_IdleEventQueue;
	Threading::Mutex				m_DeleteIdleLock;
	wxTimer							m_PingTimer;
	wxTimer							m_IdleEventTimer;

public:
	wxAppWithHelpers();
	virtual ~wxAppWithHelpers() {}

	void CleanUp();

	void DeleteObject( IDeletableObject& obj );
	void DeleteObject( IDeletableObject* obj )
	{
		if( obj == NULL ) return;
		DeleteObject( *obj );
	}

	void PostCommand( void* clientData, int evtType, int intParam=0, long longParam=0, const wxString& stringParam=wxEmptyString );
	void PostCommand( int evtType, int intParam=0, long longParam=0, const wxString& stringParam=wxEmptyString );
	void PostMethod( FnType_VoidMethod* method );

	void Ping();
	bool OnInit();
	//int  OnExit();

protected:
	void IdleEventDispatcher( const char* action );
	void PingDispatcher( const char* action );
	void DeletionDispatcher();

	void OnIdleEvent( wxIdleEvent& evt );
	void OnPingEvent( pxPingEvent& evt );
	void OnAddEventToIdleQueue( wxEvent& evt );
	void OnPingTimeout( wxTimerEvent& evt );
	void OnIdleEventTimeout( wxTimerEvent& evt );
	void OnMessageBox( BaseMessageBoxEvent& evt );
	void OnDeleteObject( wxCommandEvent& evt );
};

namespace Msgbox
{
	extern int	ShowModal( BaseMessageBoxEvent& evt );
}
