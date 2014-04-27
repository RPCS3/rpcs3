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

#include "App.h"

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE(pxEvt_DockConsole, -1)
END_DECLARE_EVENT_TYPES()

static const bool EnableThreadedLoggingTest = false; //true;

class LogWriteEvent;

// --------------------------------------------------------------------------------------
//  PipeRedirectionBase
// --------------------------------------------------------------------------------------
// Implementations for this class are found in Win/Lnx specific modules.  Class creation
// should be done using NewPipeRedir() only (hence the protected constructor in this class).
//
class PipeRedirectionBase
{
	DeclareNoncopyableObject( PipeRedirectionBase );

public:
	virtual ~PipeRedirectionBase() throw()=0;	// abstract destructor, forces abstract class behavior

protected:
	PipeRedirectionBase() {}
};

extern PipeRedirectionBase* NewPipeRedir( FILE* stdstream );

// --------------------------------------------------------------------------------------
//  pxLogConsole
// --------------------------------------------------------------------------------------
// This is a custom logging facility that pipes wxLog messages to our very own console
// log window.  Useful for catching and redirecting wx's internal logs (although like
// 3/4ths of them are worthless and we would probably rather ignore them anyway).
//
class pxLogConsole : public wxLog
{
public:
	pxLogConsole() {}

protected:
	virtual void DoLog(wxLogLevel level, const wxChar *szString, time_t t);
};


// --------------------------------------------------------------------------------------
//  ConsoleThreadTest -- useful class for unit testing the thread safety and general performance
//  of the console logger.
// --------------------------------------------------------------------------------------

class ConsoleTestThread : public Threading::pxThread
{
	typedef pxThread _parent;

protected:
	volatile bool m_done;
	void ExecuteTaskInThread();

public:
	ConsoleTestThread() :
		m_done( false )
	{
	}

	~ConsoleTestThread() throw()
	{
		m_done = true;
	}
};

// --------------------------------------------------------------------------------------
//  pxLogTextCtrl
// --------------------------------------------------------------------------------------
class pxLogTextCtrl : public wxTextCtrl,
	public EventListener_CoreThread,
	public EventListener_Plugins
{
protected:
	ScopedPtr<ScopedCoreThreadPause> m_IsPaused;
	bool m_FreezeWrites;

public:
	pxLogTextCtrl(wxWindow* parent);
	virtual ~pxLogTextCtrl() throw();

	bool HasWriteLock() const { return m_FreezeWrites; }
	void ConcludeIssue();

#ifdef __WXMSW__
	virtual void WriteText(const wxString& text);
#endif

protected:
	virtual void OnThumbTrack(wxScrollWinEvent& event);
	virtual void OnThumbRelease(wxScrollWinEvent& event);
	virtual void OnResize( wxSizeEvent& evt );

	void DispatchEvent( const CoreThreadStatus& status );
	void DispatchEvent( const PluginEventType& evt );
};

// --------------------------------------------------------------------------------------
//  ConsoleLogFrame  --  Because the one built in wx is poop.
// --------------------------------------------------------------------------------------
class ConsoleLogFrame : public wxFrame
{
	DeclareNoncopyableObject(ConsoleLogFrame);

public:
	typedef AppConfig::ConsoleLogOptions ConLogConfig;

protected:
	class ColorArray
	{
		DeclareNoncopyableObject(ColorArray);

	protected:
		SafeArray<wxTextAttr>	m_table;
		wxTextAttr				m_color_default;

	public:
		virtual ~ColorArray() throw();
		ColorArray( int fontsize=8 );

		void Create( int fontsize );
		void Cleanup();

		void SetFont( const wxFont& font );
		void SetFont( int fontsize );

		const wxTextAttr& operator[]( ConsoleColors coloridx ) const
		{
			return m_table[(int)coloridx];
		}
		
		void SetColorScheme_Dark();
		void SetColorScheme_Light();
	};

	class ColorSection
	{
	public:
		ConsoleColors	color;
		int				startpoint;

		ColorSection() {}
		ColorSection( ConsoleColors _color, int msgptr ) : color(_color), startpoint(msgptr) { }
	};

protected:
	ConLogConfig&	m_conf;
	pxLogTextCtrl&	m_TextCtrl;
	wxTimer			m_timer_FlushLimiter;
	wxTimer			m_timer_FlushUnlocker;
	ColorArray		m_ColorTable;

	int				m_flushevent_counter;
	bool			m_FlushRefreshLocked;
	
	// ----------------------------------------------------------------------------
	//  Queue State Management Vars
	// ----------------------------------------------------------------------------

	// Boolean indicating if a flush message is already in the Main message queue.  Used
	// to prevent spamming the main thread with redundant messages.
	volatile bool			m_pendingFlushMsg;

	// This is a counter of the number of threads waiting for the Queue to flush.
	volatile int			m_WaitingThreadsForFlush;

	// Indicates to the main thread if a child thread is actively writing to the log.  If
	// true the main thread will sleep briefly to allow the child a chance to accumulate
	// more messages (helps avoid rapid successive flushes on high volume logging).
	volatile bool			m_ThreadedLogInQueue;

	// Used by threads waiting on the queue to flush.
	Semaphore				m_sem_QueueFlushed;

	// Lock object for accessing or modifying the following three vars:
	//  m_QueueBuffer, m_QueueColorSelection, m_CurQueuePos
	Mutex					m_mtx_Queue;

	// Describes a series of colored text sections in the m_QueueBuffer.
	SafeList<ColorSection>	m_QueueColorSection;

	// Series of Null-terminated strings, each one has a corresponding entry in
	// m_QueueColorSelection.
	SafeArray<wxChar>		m_QueueBuffer;

	// Current write position into the m_QueueBuffer;
	int						m_CurQueuePos;

	// Threaded log spammer, useful for testing console logging performance.
	// (alternatively you can enable Disasm logging in any recompiler and achieve
	// a similar effect)
	ScopedPtr<ConsoleTestThread>	m_threadlogger;

	// ----------------------------------------------------------------------------
	//  Window and Menu Object Handles
	// ----------------------------------------------------------------------------

	ScopedArray<wxMenuItem*>	m_sourceChecks;

public:
	// ctor & dtor
	ConsoleLogFrame( MainEmuFrame *pParent, const wxString& szTitle, ConLogConfig& options );
	virtual ~ConsoleLogFrame();

	virtual void DockedMove();

	// Retrieves the current configuration options settings for this box.
	// (settings change if the user moves the window or changes the font size)
	const ConLogConfig& GetConfig() const { return m_conf; }

	bool Write( ConsoleColors color, const wxString& text );
	bool Newline();

protected:
	// menu callbacks
	void OnOpen (wxCommandEvent& event);
	void OnClose(wxCommandEvent& event);
	void OnSave (wxCommandEvent& event);
	void OnClear(wxCommandEvent& event);

	void OnEnableAllLogging(wxCommandEvent& event);
	void OnDisableAllLogging(wxCommandEvent& event);

	void OnToggleTheme(wxCommandEvent& event);
	void OnFontSize(wxCommandEvent& event);
	void OnToggleSource(wxCommandEvent& event);
	void OnToggleCDVDInfo(wxCommandEvent& event);

	virtual void OnCloseWindow(wxCloseEvent& event);

	void OnSetTitle( wxCommandEvent& event );
	void OnDockedMove( wxCommandEvent& event );
	void OnFlushUnlockerTimer( wxTimerEvent& evt );
	void OnFlushEvent( wxCommandEvent& event );

	void DoFlushQueue();
	void DoFlushEvent( bool isPending );

	void OnMoveAround( wxMoveEvent& evt );
	void OnResize( wxSizeEvent& evt );
	void OnActivate( wxActivateEvent& evt );
	
	void OnLoggingChanged();
};
