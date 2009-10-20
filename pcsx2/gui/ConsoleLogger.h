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

#include "App.h"

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE(wxEVT_DockConsole, -1)
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

class ConsoleTestThread : public Threading::PersistentThread
{
	typedef PersistentThread _parent;

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
		virtual ~ColorArray();
		ColorArray( int fontsize=8 );

		void Create( int fontsize );
		void Cleanup();

		void SetFont( const wxFont& font );
		void SetFont( int fontsize );

		const wxTextAttr& operator[]( ConsoleColors coloridx ) const
		{
			return m_table[(int)coloridx];
		}
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
	wxTextCtrl&		m_TextCtrl;
	ColorArray		m_ColorTable;

	// this int throttles freeze/thaw of the display, by cycling from -2 to 4, roughly.
	// (negative values force thaw, positive values indicate thaw is disabled.  This is
	//  needed because the wxWidgets Thaw implementation uses a belated paint message,
	//  and if we Freeze on the very next queued message after thawing, the repaint
	//  never happens)
	int				m_ThawThrottle;		

	// If a freeze is executed, this is set true (without this, wx asserts)
	bool			m_ThawNeeded;

	// Set true when a Thaw message is sent (avoids cluttering the message pump with redundant
	// requests)
	bool			m_ThawPending;

	// ----------------------------------------------------------------------------
	//  Queue State Management Vars
	// ----------------------------------------------------------------------------

	// This is a counter of the total number of pending flushes across all threads.
	// If the value exceeds a threshold, threads begin throttling to avoid deadlocking
	// the GUI.
	volatile int			m_pendingFlushes;

	// This is a counter of the number of threads waiting for the Queue to flush.
	volatile int			m_WaitingThreadsForFlush;

	// Used by threads waiting on the queue to flush.
	Semaphore				m_sem_QueueFlushed;

	// Lock object for accessing or modifying the following three vars:
	//  m_QueueBuffer, m_QueueColorSelection, m_CurQueuePos
	MutexLockRecursive		m_QueueLock;
	
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
	ConsoleTestThread*		m_threadlogger;

public:
	// ctor & dtor
	ConsoleLogFrame( MainEmuFrame *pParent, const wxString& szTitle, ConLogConfig& options );
	virtual ~ConsoleLogFrame();

	virtual void DockedMove();

	// Retrieves the current configuration options settings for this box.
	// (settings change if the user moves the window or changes the font size)
	const ConLogConfig& GetConfig() const { return m_conf; }

	void Write( ConsoleColors color, const wxString& text );
	void Newline();

protected:
	// menu callbacks
	virtual void OnOpen (wxMenuEvent& event);
	virtual void OnClose(wxMenuEvent& event);
	virtual void OnSave (wxMenuEvent& event);
	virtual void OnClear(wxMenuEvent& event);

	void OnFontSize(wxMenuEvent& event);

	virtual void OnCloseWindow(wxCloseEvent& event);

	void OnSetTitle( wxCommandEvent& event );
	void OnDockedMove( wxCommandEvent& event );
	void OnIdleEvent( wxIdleEvent& event );
	void OnFlushEvent( wxCommandEvent& event );

	// common part of OnClose() and OnCloseWindow()
	virtual void DoClose();
	void DoFlushQueue();

	void OnMoveAround( wxMoveEvent& evt );
	void OnResize( wxSizeEvent& evt );
};

