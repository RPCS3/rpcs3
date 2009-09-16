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


BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE(wxEVT_DockConsole, -1)
END_DECLARE_EVENT_TYPES()

static const bool EnableThreadedLoggingTest = false; //true;

using namespace Threading;

class MainEmuFrame;
class LogWriteEvent;


//////////////////////////////////////////////////////////////////////////////////////////
// pxLogConsole
// This is a custom logging facility that pipes wxLog messages to our very own console
// log window.
//
class pxLogConsole : public wxLog
{
public:
    pxLogConsole() {}

protected:
    virtual void DoLog(wxLogLevel level, const wxChar *szString, time_t t);
};

//////////////////////////////////////////////////////////////////////////////////////////
// ConsoleThreadTest -- useful class for unit testing the thread safety and general performance
// of the console logger.
//
class ConsoleTestThread : public PersistentThread
{
protected:
	volatile bool m_done;
	sptr ExecuteTask();

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


//////////////////////////////////////////////////////////////////////////////////////////
//
class ConsoleLogFrame : public wxFrame
{
	DeclareNoncopyableObject(ConsoleLogFrame)

public:
	typedef AppConfig::ConsoleLogOptions ConLogConfig;

protected:
	class ColorArray
	{
		DeclareNoncopyableObject(ColorArray)

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

		const wxTextAttr& operator[]( Console::Colors coloridx ) const
		{
			return m_table[(int)coloridx];
		}
	};

protected:
	ConLogConfig&	m_conf;
	wxTextCtrl&		m_TextCtrl;
	ColorArray		m_ColorTable;
	Console::Colors	m_curcolor;
	volatile long	m_msgcounter;		// used to track queued messages and throttle load placed on the gui message pump

	Semaphore		m_semaphore;

	// Threaded log spammer, useful for testing console logging performance.
	ConsoleTestThread* m_threadlogger;

public:
	// ctor & dtor
	ConsoleLogFrame( MainEmuFrame *pParent, const wxString& szTitle, ConLogConfig& options );
	virtual ~ConsoleLogFrame();

	virtual void Write( const wxString& text );
	virtual void SetColor( Console::Colors color );
	virtual void ClearColor();
	virtual void DockedMove();

	// Retrieves the current configuration options settings for this box.
	// (settings change if the user moves the window or changes the font size)
	const ConLogConfig& GetConfig() const { return m_conf; }

	void Write( Console::Colors color, const wxString& text );
	void Newline();
	void CountMessage();
	void DoMessage();

protected:

	// menu callbacks
	virtual void OnOpen (wxMenuEvent& event);
	virtual void OnClose(wxMenuEvent& event);
	virtual void OnSave (wxMenuEvent& event);
	virtual void OnClear(wxMenuEvent& event);

	void OnFontSize(wxMenuEvent& event);

	virtual void OnCloseWindow(wxCloseEvent& event);

	void OnWrite( wxCommandEvent& event );
	void OnNewline( wxCommandEvent& event );
	void OnSetTitle( wxCommandEvent& event );
	void OnDockedMove( wxCommandEvent& event );
	void OnSemaphoreWait( wxCommandEvent& event );

	// common part of OnClose() and OnCloseWindow()
	virtual void DoClose();

	void OnMoveAround( wxMoveEvent& evt );
	void OnResize( wxSizeEvent& evt );
};

