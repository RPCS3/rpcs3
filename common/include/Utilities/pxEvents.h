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

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE( pxEvt_StartIdleEventTimer, -1 )
	DECLARE_EVENT_TYPE( pxEvt_DeleteObject, -1 )
	DECLARE_EVENT_TYPE( pxEvt_DeleteThread, -1 )
	DECLARE_EVENT_TYPE( pxEvt_InvokeAction, -1 )
	DECLARE_EVENT_TYPE( pxEvt_SynchronousCommand, -1 )
END_DECLARE_EVENT_TYPES()

typedef void FnType_Void();

// --------------------------------------------------------------------------------------
//  SynchronousActionState
// --------------------------------------------------------------------------------------
class SynchronousActionState
{
	DeclareNoncopyableObject( SynchronousActionState );

protected:
	bool						m_posted;
	Threading::Semaphore		m_sema;
	ScopedPtr<BaseException>	m_exception;

public:
	sptr						return_value;

	SynchronousActionState()
	{
		m_posted		= false;
		return_value	= 0;
	}

	virtual ~SynchronousActionState() throw()  {}

	void SetException( const BaseException& ex );
	void SetException( BaseException* ex );

	Threading::Semaphore& GetSemaphore() { return m_sema; }
	const Threading::Semaphore& GetSemaphore() const { return m_sema; }

	void RethrowException() const;
	int WaitForResult();
	int WaitForResult_NoExceptions();
	void PostResult( int res );
	void ClearResult();
	void PostResult();
};


// --------------------------------------------------------------------------------------
//  pxSimpleEvent  -  non-abstract implementation of wxEvent
// --------------------------------------------------------------------------------------
// Why?  I mean, why is wxEvent abstract?  Over-designed OO is such a bad habit.  And while
// I'm on my high horse (and it's so very high!), why use 'private'?  Ever?  Seriously, it
// sucks.  Stop using it, people.
//
class pxSimpleEvent : public wxEvent
{
	DECLARE_DYNAMIC_CLASS_NO_ASSIGN(pxSimpleEvent)

public:
	explicit pxSimpleEvent( int evtid=0 )
		: wxEvent(0, evtid)
	{ }

	pxSimpleEvent( wxWindowID winId, int evtid )
		: wxEvent(winId, evtid)
	{ }

	virtual wxEvent *Clone() const { return new pxSimpleEvent(*this); }
};

// --------------------------------------------------------------------------------------
//  pxActionEvent
// --------------------------------------------------------------------------------------
class pxActionEvent : public wxEvent
{
	DECLARE_DYNAMIC_CLASS_NO_ASSIGN(pxActionEvent)

protected:
	SynchronousActionState* m_state;

public:
	virtual ~pxActionEvent() throw() { }
	virtual pxActionEvent *Clone() const { return new pxActionEvent(*this); }

	explicit pxActionEvent( SynchronousActionState* sema=NULL, int msgtype=pxEvt_InvokeAction );
	explicit pxActionEvent( SynchronousActionState& sema, int msgtype=pxEvt_InvokeAction );
	pxActionEvent( const pxActionEvent& src );

	Threading::Semaphore* GetSemaphore() const { return m_state ? &m_state->GetSemaphore() : NULL; }

	const SynchronousActionState* GetSyncState() const { return m_state; }
	SynchronousActionState* GetSyncState() { return m_state; }

	void SetSyncState( SynchronousActionState* obj ) { m_state = obj; }
	void SetSyncState( SynchronousActionState& obj ) { m_state = &obj; }

	virtual void SetException( BaseException* ex );
	void SetException( const BaseException& ex );

	virtual void _DoInvokeEvent();
	
protected:
	// Extending classes should implement this method to perfoem whatever action it is
	// the event is supposed to do. :)  Thread affinity is garaunteed to be the Main/UI
	// thread, and exceptions will be handled automatically.
	//
	// Exception note: exceptions are passed back to the thread that posted the event
	// to the queue, when possible.  If the calling thread is not blocking for a result
	// from this event, then the exception will be posted to the Main/UI thread instead.
	virtual void InvokeEvent() {}
};


// --------------------------------------------------------------------------------------
//  pxExceptionEvent
// --------------------------------------------------------------------------------------
class pxExceptionEvent : public pxActionEvent
{
	typedef pxActionEvent _parent;

protected:
	BaseException*	m_except;

public:
	pxExceptionEvent( BaseException* ex=NULL )
	{
		m_except = ex;
	}

	pxExceptionEvent( const BaseException& ex );

	virtual ~pxExceptionEvent() throw()
	{
	}

	virtual pxExceptionEvent *Clone() const { return new pxExceptionEvent(*this); }

protected:
	void InvokeEvent();
};

// --------------------------------------------------------------------------------------
//  pxSynchronousCommandEvent
// --------------------------------------------------------------------------------------

class pxSynchronousCommandEvent : public wxCommandEvent
{
	DECLARE_DYNAMIC_CLASS_NO_ASSIGN(pxSynchronousCommandEvent)

protected:
	SynchronousActionState*	m_sync;
	wxEventType				m_realEvent;

public:
	virtual ~pxSynchronousCommandEvent() throw() { }
	virtual pxSynchronousCommandEvent *Clone() const { return new pxSynchronousCommandEvent(*this); }

	pxSynchronousCommandEvent(SynchronousActionState* sema=NULL, wxEventType commandType = wxEVT_NULL, int winid = 0);
	pxSynchronousCommandEvent(SynchronousActionState& sema, wxEventType commandType = wxEVT_NULL, int winid = 0);

	pxSynchronousCommandEvent(SynchronousActionState* sema, const wxCommandEvent& evt);
	pxSynchronousCommandEvent(SynchronousActionState& sema, const wxCommandEvent& evt);

	pxSynchronousCommandEvent(const pxSynchronousCommandEvent& src);

	Threading::Semaphore* GetSemaphore() { return m_sync ? &m_sync->GetSemaphore() : NULL; }
	wxEventType GetRealEventType() const { return m_realEvent; }

	void SetException( BaseException* ex );
	void SetException( const BaseException& ex );
};

// --------------------------------------------------------------------------------------
//  BaseMessageBoxEvent
// --------------------------------------------------------------------------------------
class BaseMessageBoxEvent : public pxActionEvent
{
	typedef pxActionEvent _parent;
	DECLARE_DYNAMIC_CLASS_NO_ASSIGN(BaseMessageBoxEvent)

protected:
	wxString			m_Content;

public:
	virtual ~BaseMessageBoxEvent() throw() { }
	virtual BaseMessageBoxEvent *Clone() const { return new BaseMessageBoxEvent(*this); }

	explicit BaseMessageBoxEvent( const wxString& content=wxEmptyString, SynchronousActionState* instdata=NULL );
	BaseMessageBoxEvent( const wxString& content, SynchronousActionState& instdata );
	BaseMessageBoxEvent( const BaseMessageBoxEvent& event );

protected:
	virtual void InvokeEvent();
	virtual int _DoDialog() const;
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

	pxMessageBoxEvent() {}
	pxMessageBoxEvent( const wxString& title, const wxString& content, const MsgButtons& buttons, SynchronousActionState& instdata );
	pxMessageBoxEvent( const wxString& title, const wxString& content, const MsgButtons& buttons, SynchronousActionState* instdata=NULL );
	pxMessageBoxEvent( const pxMessageBoxEvent& event );

protected:
	int _DoDialog() const;
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

	pxAssertionEvent( const wxString& content=wxEmptyString, const wxString& trace=wxEmptyString, SynchronousActionState* instdata=NULL );
	pxAssertionEvent( const wxString& content, const wxString& trace, SynchronousActionState& instdata );
	pxAssertionEvent( const pxAssertionEvent& event );

	pxAssertionEvent& SetStacktrace( const wxString& trace );

protected:
	int _DoDialog() const;
};


typedef void (wxEvtHandler::*pxSyncronousEventFunction)(pxSynchronousCommandEvent&);

#define pxSynchronousEventHandler(func) \
	(wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(pxSyncronousEventFunction, &func )
