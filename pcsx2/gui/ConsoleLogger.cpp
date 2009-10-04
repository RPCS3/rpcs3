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
#include "MainFrame.h"
#include "ConsoleLogger.h"
#include "Utilities/Console.h"
#include "DebugTools/Debug.h"

#include <wx/file.h>
#include <wx/textfile.h>

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE(wxEVT_LOG_Write, -1)
	DECLARE_EVENT_TYPE(wxEVT_LOG_Newline, -1)
	DECLARE_EVENT_TYPE(wxEVT_SetTitleText, -1)
	DECLARE_EVENT_TYPE(wxEVT_SemaphoreWait, -1)
END_DECLARE_EVENT_TYPES()

DEFINE_EVENT_TYPE(wxEVT_LOG_Write)
DEFINE_EVENT_TYPE(wxEVT_LOG_Newline)
DEFINE_EVENT_TYPE(wxEVT_SetTitleText)
DEFINE_EVENT_TYPE(wxEVT_DockConsole)
DEFINE_EVENT_TYPE(wxEVT_SemaphoreWait)

// C++ requires abstract destructors to exist, even thought hey're abstract.
PipeRedirectionBase::~PipeRedirectionBase() throw() {}

// ----------------------------------------------------------------------------
//
void pxLogConsole::DoLog( wxLogLevel level, const wxChar *szString, time_t t )
{
	switch ( level )
	{
		case wxLOG_Trace:
		case wxLOG_Debug:
			if( IsDebugBuild )
			{
				wxString str;
				TimeStamp( &str );
				str += szString;

				#if defined(__WXMSW__) && !defined(__WXMICROWIN__)
					// don't prepend debug/trace here: it goes to the
					// debug window anyhow
					str += wxT("\r\n");
					OutputDebugString(str);
				#else
					// send them to stderr
					wxFprintf(stderr, wxT("[%s] %s\n"),
							  level == wxLOG_Trace ? wxT("Trace")
												   : wxT("Debug"),
							  str.c_str());
					fflush(stderr);
				#endif
			}
		break;

		case wxLOG_FatalError:
			// This one is unused by wx, and unused by PCSX2 (we prefer exceptions, thanks).
			pxFailDev( "Stop using FatalError and use assertions or exceptions instead." );
		break;

		case wxLOG_Status:
			// Also unsed by wx, and unused by PCSX2 also (we prefer direct API calls to our main window!)
			pxFailDev( "Stop using wxLogStatus just access the Pcsx2App functions directly instead." );
		break;

		case wxLOG_Info:
			if ( !GetVerbose() ) return;
			// fallthrough!

		case wxLOG_Message:
			Console.WriteLn( wxString(L"wx > ") + szString );
		break;

		case wxLOG_Error:
			Console.Error( wxString(L"wx > ") + szString );
		break;

		case wxLOG_Warning:
			Console.Notice( wxString(L"wx > ") + szString );
		break;
    }
}


// ----------------------------------------------------------------------------
void ConsoleTestThread::ExecuteTask()
{
	static int numtrack = 0;

	while( !m_done )
	{
		// Two lines, both formatted, and varied colors.  This makes for a fairly realistic
		// worst case scenario (without being entirely unrealistic).
		Console.WriteLn( wxsFormat( L"This is a threaded logging test. Something bad could happen... %d", ++numtrack ) );
		Console.Status( wxsFormat( L"Testing high stress loads %s", L"(multi-color)" ) );
		Sleep( 0 );
	}
}

// ----------------------------------------------------------------------------
// pass an uninitialized file object, the function will ask the user for the
// filename and try to open it, returns true on success (file was opened),
// false if file couldn't be opened/created and -1 if the file selection
// dialog was canceled
//
static bool OpenLogFile(wxFile& file, wxString& filename, wxWindow *parent)
{
    filename = wxSaveFileSelector(L"log", L"txt", L"log.txt", parent);
    if ( !filename ) return false; // canceled

    if( wxFile::Exists(filename) )
    {
        bool bAppend = false;
        wxString strMsg;
        strMsg.Printf(L"Append log to file '%s' (choosing [No] will overwrite it)?",
                      filename.c_str());

        switch ( wxMessageBox(strMsg, L"Question", wxICON_QUESTION | wxYES_NO | wxCANCEL) )
		{
			case wxYES:
				bAppend = true;
			break;

			case wxNO:
				bAppend = false;
			break;

			case wxCANCEL:
				return false;

			default:
				wxFAIL_MSG( L"invalid message box return value" );
        }

		return ( bAppend ) ?
			file.Open(filename, wxFile::write_append) :
            file.Create(filename, true /* overwrite */);
    }

	return file.Create(filename);
}

// ------------------------------------------------------------------------
// fontsize - size of the font specified in points.
//   (actual font used is the system-selected fixed-width font)
//
ConsoleLogFrame::ColorArray::ColorArray( int fontsize ) :
	m_table( 8 )
{
	Create( fontsize );
}

ConsoleLogFrame::ColorArray::~ColorArray()
{
	Cleanup();
}

void ConsoleLogFrame::ColorArray::Create( int fontsize )
{
	wxFont fixed( fontsize, wxMODERN, wxNORMAL, wxNORMAL );
	wxFont fixedB( fontsize, wxMODERN, wxNORMAL, wxBOLD );

	// Standard R, G, B format:
	new (&m_table[Color_Black])		wxTextAttr( wxColor(   0,   0,   0 ), wxNullColour, fixed );
	new (&m_table[Color_Red])		wxTextAttr( wxColor( 128,   0,   0 ), wxNullColour, fixedB );
	new (&m_table[Color_Green])		wxTextAttr( wxColor(   0, 128,   0 ), wxNullColour, fixed );
	new (&m_table[Color_Blue])		wxTextAttr( wxColor(   0,   0, 128 ), wxNullColour, fixed );
	new (&m_table[Color_Yellow])	wxTextAttr( wxColor( 160, 160,   0 ), wxNullColour, fixedB );
	new (&m_table[Color_Cyan])		wxTextAttr( wxColor(   0, 140, 140 ), wxNullColour, fixed );
	new (&m_table[Color_Magenta])	wxTextAttr( wxColor( 160,   0, 160 ), wxNullColour, fixed );
	new (&m_table[Color_White])		wxTextAttr( wxColor( 128, 128, 128 ), wxNullColour, fixed );
}

void ConsoleLogFrame::ColorArray::Cleanup()
{
	// The contents of m_table were created with placement new, and must be
	// disposed of manually:

	for( int i=0; i<8; ++i )
		m_table[i].~wxTextAttr();
}

// fixme - not implemented yet.
void ConsoleLogFrame::ColorArray::SetFont( const wxFont& font )
{
	//for( int i=0; i<8; ++i )
	//	m_table[i].SetFont( font );
}

void ConsoleLogFrame::ColorArray::SetFont( int fontsize )
{
	Cleanup();
	Create( fontsize );
}

static const ConsoleColors DefaultConsoleColor = Color_White;

enum MenuIDs_t
{
	MenuID_FontSize_Small = 0x10,
	MenuID_FontSize_Normal,
	MenuID_FontSize_Large,
	MenuID_FontSize_Huge,
};

// ------------------------------------------------------------------------
ConsoleLogFrame::ConsoleLogFrame( MainEmuFrame *parent, const wxString& title, AppConfig::ConsoleLogOptions& options ) :
	wxFrame(parent, wxID_ANY, title)
,	m_conf( options )
,	m_TextCtrl( *new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxHSCROLL | wxTE_READONLY | wxTE_RICH2 ) )
,	m_ColorTable( options.FontSize )
,	m_curcolor( DefaultConsoleColor )
,	m_msgcounter( 0 )
,	m_threadlogger( EnableThreadedLoggingTest ? new ConsoleTestThread() : NULL )
{
	m_TextCtrl.SetBackgroundColour( wxColor( 230, 235, 242 ) );

    // create Log menu (contains most options)
	wxMenuBar *pMenuBar = new wxMenuBar();
	wxMenu& menuLog = *new wxMenu();
	menuLog.Append(wxID_SAVE,  _("&Save..."),	_("Save log contents to file"));
	menuLog.Append(wxID_CLEAR, _("C&lear"),		_("Clear the log window contents"));
	menuLog.AppendSeparator();
	menuLog.Append(wxID_CLOSE, _("&Close"),		_("Close this log window; contents are preserved"));

	// create Appearance menu and submenus

	wxMenu& menuFontSizes = *new wxMenu();
	menuFontSizes.Append( MenuID_FontSize_Small,	_("Small"),	_("Fits a lot of log in a microcosmically small area."),
		wxITEM_RADIO )->Check( options.FontSize == 7 );
	menuFontSizes.Append( MenuID_FontSize_Normal,	_("Normal"),_("It's what I use (the programmer guy)."),
		wxITEM_RADIO )->Check( options.FontSize == 8 );
	menuFontSizes.Append( MenuID_FontSize_Large,	_("Large"),	_("Its nice and readable."),
		wxITEM_RADIO )->Check( options.FontSize == 10 );
	menuFontSizes.Append( MenuID_FontSize_Huge,		_("Huge"),	_("In case you have a really high res display."),
		wxITEM_RADIO )->Check( options.FontSize == 12 );

	wxMenu& menuAppear = *new wxMenu();
	menuAppear.Append( wxID_ANY, _("Always on Top"),
		_("When checked the log window will be visible over other foreground windows."), wxITEM_CHECK );
	menuAppear.Append( wxID_ANY, _("Font Size"), &menuFontSizes );

	pMenuBar->Append(&menuLog,		_("&Log"));
	pMenuBar->Append(&menuAppear,	_("&Appearance"));
	SetMenuBar(pMenuBar);

	// status bar for menu prompts
	CreateStatusBar();
	ClearColor();

	SetSize( wxRect( options.DisplayPosition, options.DisplaySize ) );
	Show( options.Visible );

	// Bind Events:

	Connect( wxID_OPEN,  wxEVT_COMMAND_MENU_SELECTED, wxMenuEventHandler(ConsoleLogFrame::OnOpen)  );
	Connect( wxID_CLOSE, wxEVT_COMMAND_MENU_SELECTED, wxMenuEventHandler(ConsoleLogFrame::OnClose) );
	Connect( wxID_SAVE,  wxEVT_COMMAND_MENU_SELECTED, wxMenuEventHandler(ConsoleLogFrame::OnSave)  );
	Connect( wxID_CLEAR, wxEVT_COMMAND_MENU_SELECTED, wxMenuEventHandler(ConsoleLogFrame::OnClear) );

	Connect( MenuID_FontSize_Small, MenuID_FontSize_Huge, wxEVT_COMMAND_MENU_SELECTED, wxMenuEventHandler( ConsoleLogFrame::OnFontSize ) );

	Connect( wxEVT_CLOSE_WINDOW,	wxCloseEventHandler(ConsoleLogFrame::OnCloseWindow) );
	Connect( wxEVT_MOVE,			wxMoveEventHandler(ConsoleLogFrame::OnMoveAround) );
	Connect( wxEVT_SIZE,			wxSizeEventHandler(ConsoleLogFrame::OnResize) );

	Connect( wxEVT_LOG_Write,		wxCommandEventHandler(ConsoleLogFrame::OnWrite) );
	Connect( wxEVT_LOG_Newline,		wxCommandEventHandler(ConsoleLogFrame::OnNewline) );
	Connect( wxEVT_SetTitleText,	wxCommandEventHandler(ConsoleLogFrame::OnSetTitle) );
	Connect( wxEVT_DockConsole,		wxCommandEventHandler(ConsoleLogFrame::OnDockedMove) );
	Connect( wxEVT_SemaphoreWait,	wxCommandEventHandler(ConsoleLogFrame::OnSemaphoreWait) );

	if( m_threadlogger != NULL )
		m_threadlogger->Start();
}

ConsoleLogFrame::~ConsoleLogFrame()
{
	safe_delete( m_threadlogger );
	wxGetApp().OnProgramLogClosed();
}

void ConsoleLogFrame::SetColor( ConsoleColors color )
{
	if( color != m_curcolor )
		m_TextCtrl.SetDefaultStyle( m_ColorTable[m_curcolor=color] );
}

void ConsoleLogFrame::ClearColor()
{
	if( DefaultConsoleColor != m_curcolor )
		m_TextCtrl.SetDefaultStyle( m_ColorTable[m_curcolor=DefaultConsoleColor] );
}

void ConsoleLogFrame::Write( const wxString& text )
{
	// remove selection (WriteText is in fact ReplaceSelection)
	// TODO : Optimize this to only replace selection if some selection
	//   messages have been received since the last write.

#ifdef __WXMSW__
	wxTextPos nLen = m_TextCtrl.GetLastPosition();
	m_TextCtrl.SetSelection(nLen, nLen);
#endif

	m_TextCtrl.AppendText( text );

	// cap at 256k for now...
	// fixme - 256k runs well on win32 but appears to be very sluggish on linux.  Might
	// need platform dependent defaults here. - air
	if( m_TextCtrl.GetLastPosition() > 0x40000 )
	{
		m_TextCtrl.Remove( 0, 0x10000 );
	}
}

// Implementation note:  Calls SetColor and Write( text ).  Override those virtuals
// and this one will magically follow suite. :)
void ConsoleLogFrame::Write( ConsoleColors color, const wxString& text )
{
	SetColor( color );
	Write( text );
}

void ConsoleLogFrame::Newline()
{
	Write( L"\n" );
}

void ConsoleLogFrame::DoClose()
{
    // instead of closing just hide the window to be able to Show() it later
    Show(false);
	if( wxWindow* main = GetParent() )
		wxStaticCast( main, MainEmuFrame )->OnLogBoxHidden();
}

void ConsoleLogFrame::DockedMove()
{
	SetPosition( m_conf.DisplayPosition );
}

// =================================================================================
//  Section : Event Handlers
//    * Misc Window Events (Move, Resize,etc)
//    * Menu Events
//    * Logging Events
// =================================================================================

// Special event recieved from a window we're docked against.
void ConsoleLogFrame::OnDockedMove( wxCommandEvent& event )
{
	DockedMove();
}

void ConsoleLogFrame::OnMoveAround( wxMoveEvent& evt )
{
	// Docking check!  If the window position is within some amount
	// of the main window, enable docking.

	if( wxWindow* main = GetParent() )
	{
		wxPoint topright( main->GetRect().GetTopRight() );
		wxRect snapzone( topright - wxSize( 8,8 ), wxSize( 16,16 ) );

		m_conf.AutoDock = snapzone.Contains( GetPosition() );
		//Console.WriteLn( "DockCheck: %d", g_Conf->ConLogBox.AutoDock );
		if( m_conf.AutoDock )
		{
			SetPosition( topright + wxSize( 1,0 ) );
			m_conf.AutoDock = true;
		}
	}
	m_conf.DisplayPosition = GetPosition();
	evt.Skip();
}

void ConsoleLogFrame::OnResize( wxSizeEvent& evt )
{
	m_conf.DisplaySize = GetSize();
	evt.Skip();
}

void ConsoleLogFrame::OnCloseWindow(wxCloseEvent& event)
{
	if( event.CanVeto() )
		DoClose();
	else
	{
		safe_delete( m_threadlogger );
		wxGetApp().OnProgramLogClosed();
		event.Skip();
	}
}

void ConsoleLogFrame::OnOpen(wxMenuEvent& WXUNUSED(event))
{
	Show(true);
}

void ConsoleLogFrame::OnClose( wxMenuEvent& event )
{
	DoClose();
}

void ConsoleLogFrame::OnSave(wxMenuEvent& WXUNUSED(event))
{
    wxString filename;
    wxFile file;
    bool rc = OpenLogFile( file, filename, this );
    if ( !rc )
    {
        // canceled
        return;
    }

    // retrieve text and save it
    // -------------------------
    int nLines = m_TextCtrl.GetNumberOfLines();
    for ( int nLine = 0; nLine < nLines; nLine++ )
    {
		if( !file.Write(m_TextCtrl.GetLineText(nLine) + wxTextFile::GetEOL()) )
		{
			wxLogError( L"Can't save log contents to file." );
			return;
		}
	}

	wxLogStatus(this, L"Log saved to the file '%s'.", filename.c_str());
}

void ConsoleLogFrame::OnClear(wxMenuEvent& WXUNUSED(event))
{
    m_TextCtrl.Clear();
}

void ConsoleLogFrame::OnFontSize( wxMenuEvent& evt )
{
	int ptsize = 8;
	switch( evt.GetId() )
	{
		case MenuID_FontSize_Small:		ptsize = 7; break;
		case MenuID_FontSize_Normal:	ptsize = 8; break;
		case MenuID_FontSize_Large:		ptsize = 10; break;
		case MenuID_FontSize_Huge:		ptsize = 12; break;
	}

	if( ptsize == m_conf.FontSize ) return;

	m_conf.FontSize = ptsize;
	m_ColorTable.SetFont( ptsize );
	m_TextCtrl.SetDefaultStyle( m_ColorTable[m_curcolor] );

	// TODO: Process the attributes of each character and upgrade the font size,
	// while still retaining color and bold settings...  (might be slow but then
	// it hardly matters being a once-in-a-bluemoon action).

}

// ----------------------------------------------------------------------------
//  Logging Events (typically received from Console class interfaces)
// ----------------------------------------------------------------------------

void ConsoleLogFrame::OnWrite( wxCommandEvent& event )
{
	Write( (ConsoleColors)event.GetExtraLong(), event.GetString() );
	DoMessage();
}

void ConsoleLogFrame::OnNewline( wxCommandEvent& event )
{
	Newline();
	DoMessage();
}

void ConsoleLogFrame::OnSetTitle( wxCommandEvent& event )
{
	SetTitle( event.GetString() );
}

void ConsoleLogFrame::OnSemaphoreWait( wxCommandEvent& event )
{
	m_semaphore.Post();
}

// ------------------------------------------------------------------------
// Deadlock protection: High volume logs will over-tax our message pump and cause the
// GUI to become inaccessible.  The cool solution would be a threaded log window, but wx
// is entirely un-safe for that kind of threading.  So instead I use a message counter
// that stalls non-GUI threads when they attempt to over-tax an already burdened log.
// If too many messages get queued up, non-gui threads are stalled to allow the gui to
// catch up.
void ConsoleLogFrame::CountMessage()
{
	long result = _InterlockedIncrement( &m_msgcounter );

	if( result > 0x20 )		// 0x20 -- arbitrary value that seems to work well (tested on P4 and C2D)
	{
		if( !wxThread::IsMain() )
		{
			// Append an event that'll post up our semaphore.  It'll get run "in
			// order" which means when it posts all queued messages will have been
			// processed.

			wxCommandEvent evt( wxEVT_SemaphoreWait );
			GetEventHandler()->AddPendingEvent( evt );
			m_semaphore.Wait();
		}
	}
}

// Thread Safety note: This function expects to be called from the Main GUI thread
// only.  If called from a thread other than Main, it will generate an assertion failure.
//
void ConsoleLogFrame::DoMessage()
{
	AllowFromMainThreadOnly();

	int cur = _InterlockedDecrement( &m_msgcounter );

	// We need to freeze the control if there are more than 2 pending messages,
	// otherwise the redraw of the console will prevent it from ever being able to
	// catch up with the rate the queue is being filled, and the whole app could
	// deadlock. >_<

	if( m_TextCtrl.IsFrozen() )
	{
		if( cur < 1 )
			m_TextCtrl.Thaw();
	}
	else if( cur >= 3 )
	{
		m_TextCtrl.Freeze();
	}
}


ConsoleLogFrame* Pcsx2App::GetProgramLog()
{
	return m_ProgramLogBox;
}

void Pcsx2App::ProgramLog_CountMsg()
{
	// New console log object model makes this check obsolete:
	//if( m_ProgramLogBox == NULL ) return;
	m_ProgramLogBox->CountMessage();
}

void Pcsx2App::ProgramLog_PostEvent( wxEvent& evt )
{
	// New console log object model makes this check obsolete:
	//if( m_ProgramLogBox == NULL ) return;
	m_ProgramLogBox->GetEventHandler()->AddPendingEvent( evt );
}

// --------------------------------------------------------------------------------------
//  ConsoleImpl_ToFile
// --------------------------------------------------------------------------------------
static void __concall _immediate_logger( const char* src )
{
#ifdef __LINUX__
	fputs( src, stdout );
#endif
	px_fputs( emuLog, src );
}

static void __concall ConsoleToFile_Newline()
{
#ifdef __LINUX__
	fputc( '\n', stdout );
	fputc( '\n', emuLog );
#else
	fputs( "\r\n", emuLog );
#endif
}

static void __concall ConsoleToFile_DoWrite( const wxString& fmt )
{
	_immediate_logger( toUTF8(fmt) );
}

static void __concall ConsoleToFile_DoWriteLn( const wxString& fmt )
{
	_immediate_logger( toUTF8(fmt) );
	ConsoleToFile_Newline();

	if( emuLog != NULL )
		fflush( emuLog );
}

extern const IConsoleWriter	ConsoleWriter_File;
const IConsoleWriter	ConsoleWriter_File =
{
	ConsoleToFile_DoWrite,
	ConsoleToFile_DoWriteLn,
	ConsoleToFile_Newline,

	ConsoleWriter_Null.SetTitle,
	ConsoleWriter_Null.SetColor,
	ConsoleWriter_Null.ClearColor,
};

// thread-local console color storage.
static __threadlocal ConsoleColors th_CurrentColor = DefaultConsoleColor;

// --------------------------------------------------------------------------------------
//  ConsoleToWindow Implementations
// --------------------------------------------------------------------------------------
static void __concall ConsoleToWindow_SetTitle( const wxString& title )
{
	wxCommandEvent evt( wxEVT_SetTitleText );
	evt.SetString( title );
	wxGetApp().ProgramLog_PostEvent( evt );
}

static void __concall ConsoleToWindow_SetColor( ConsoleColors color )
{
	th_CurrentColor = color;
}

static void __concall ConsoleToWindow_ClearColor()
{
	th_CurrentColor = DefaultConsoleColor;
}

template< const IConsoleWriter& secondary >
static void __concall ConsoleToWindow_Newline()
{
	secondary.Newline();

	wxCommandEvent evt( wxEVT_LOG_Newline );
	((Pcsx2App&)*wxTheApp).ProgramLog_PostEvent( evt );
	((Pcsx2App&)*wxTheApp).ProgramLog_CountMsg();
}

template< const IConsoleWriter& secondary >
static void __concall ConsoleToWindow_DoWrite( const wxString& fmt )
{
	secondary.DoWrite( fmt );

	wxCommandEvent evt( wxEVT_LOG_Write );
	evt.SetString( fmt );
	evt.SetExtraLong( th_CurrentColor );
	((Pcsx2App&)*wxTheApp).ProgramLog_PostEvent( evt );
	((Pcsx2App&)*wxTheApp).ProgramLog_CountMsg();
}

template< const IConsoleWriter& secondary >
static void __concall ConsoleToWindow_DoWriteLn( const wxString& fmt )
{
	secondary.DoWriteLn( fmt );

	// Implementation note: I've duplicated Write+Newline behavior here to avoid polluting
	// the message pump with lots of erroneous messages (Newlines can be bound into Write message).

	wxCommandEvent evt( wxEVT_LOG_Write );
	evt.SetString( fmt + L"\n" );
	evt.SetExtraLong( th_CurrentColor );
	((Pcsx2App&)*wxTheApp).ProgramLog_PostEvent( evt );
	((Pcsx2App&)*wxTheApp).ProgramLog_CountMsg();
}

typedef void __concall DoWriteFn(const wxString&);

static const IConsoleWriter	ConsoleWriter_Window =
{
	ConsoleToWindow_DoWrite<ConsoleWriter_Null>,
	ConsoleToWindow_DoWriteLn<ConsoleWriter_Null>,
	ConsoleToWindow_Newline<ConsoleWriter_Null>,

	ConsoleToWindow_SetTitle,
	ConsoleToWindow_SetColor,
	ConsoleToWindow_ClearColor,
};

static const IConsoleWriter	ConsoleWriter_WindowAndFile =
{
	ConsoleToWindow_DoWrite<ConsoleWriter_File>,
	ConsoleToWindow_DoWriteLn<ConsoleWriter_File>,
	ConsoleToWindow_Newline<ConsoleWriter_File>,

	ConsoleToWindow_SetTitle,
	ConsoleToWindow_SetColor,
	ConsoleToWindow_ClearColor,
};

void Pcsx2App::EnableConsoleLogging() const
{
	if( emuLog )
		Console_SetActiveHandler( (m_ProgramLogBox!=NULL) ? (IConsoleWriter&)ConsoleWriter_WindowAndFile : (IConsoleWriter&)ConsoleWriter_File );
	else
		Console_SetActiveHandler( (m_ProgramLogBox!=NULL) ? (IConsoleWriter&)ConsoleWriter_Window : (IConsoleWriter&)ConsoleWriter_Buffered );
}

// Used to disable the emuLog disk logger, typically used when disabling or re-initializing the
// emuLog file handle.  Call SetConsoleLogging to re-enable the disk logger when finished.
void Pcsx2App::DisableDiskLogging() const
{
	Console_SetActiveHandler( (m_ProgramLogBox!=NULL) ? (IConsoleWriter&)ConsoleWriter_Window : (IConsoleWriter&)ConsoleWriter_Buffered );

	// Semi-hack: It's possible, however very unlikely, that a secondary thread could attempt
	// to write to the logfile just before we disable logging, and would thus have a pending write
	// operation to emuLog file handle at the same time we're trying to re-initialize it.  The CRT
	// has some guards of its own, and PCSX2 itself typically suspends the "log happy" threads
	// when changing settings, so the chance for problems is low.  We minimize it further here
	// by sleeping off 5ms, which should allow any pending log-to-disk events to finish up.
	//
	// (the most ideal solution would be a mutex lock in the Disk logger itself, but for now I
	//  am going to try and keep the logger lock-free and use this semi-hack instead).

	Threading::Sleep( 5 );
}

void Pcsx2App::DisableWindowLogging() const
{
	Console_SetActiveHandler( (emuLog!=NULL) ? (IConsoleWriter&)ConsoleWriter_File : (IConsoleWriter&)ConsoleWriter_Buffered );
	Threading::Sleep( 5 );
}

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

class pxMessageBoxEvent : public wxEvent
{
protected:
	MsgboxEventResult&	m_Instdata;
	wxString			m_Title;
	wxString			m_Content;
	long				m_Flags;

public:
	pxMessageBoxEvent() :
		wxEvent( 0, pxEVT_MSGBOX )
	,	m_Instdata( *(MsgboxEventResult*)NULL )
	,	m_Title()
	,	m_Content()
	,	m_Flags( 0 )
	{
	}

	pxMessageBoxEvent( MsgboxEventResult& instdata, const wxString& title, const wxString& content, long flags ) :
		wxEvent( 0, pxEVT_MSGBOX )
	,	m_Instdata( instdata )
	,	m_Title( title )
	,	m_Content( content )
	,	m_Flags( flags )
	{
	}

	pxMessageBoxEvent( const pxMessageBoxEvent& event ) :
		wxEvent( event )
	,	m_Instdata( event.m_Instdata )
	,	m_Title( event.m_Title )
	,	m_Content( event.m_Content )
	,	m_Flags( event.m_Flags )
	{
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
