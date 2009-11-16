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

#include <wx/textfile.h>

#ifdef __WXMSW__
#	include <wx/msw/wrapwin.h>		// needed for OutputDebugStirng
#endif

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE(wxEVT_LOG_Write, -1)
	DECLARE_EVENT_TYPE(wxEVT_LOG_Newline, -1)
	DECLARE_EVENT_TYPE(wxEVT_SetTitleText, -1)
	DECLARE_EVENT_TYPE(wxEVT_FlushQueue, -1)
END_DECLARE_EVENT_TYPES()

DEFINE_EVENT_TYPE(wxEVT_LOG_Write)
DEFINE_EVENT_TYPE(wxEVT_LOG_Newline)
DEFINE_EVENT_TYPE(wxEVT_SetTitleText)
DEFINE_EVENT_TYPE(wxEVT_DockConsole)
DEFINE_EVENT_TYPE(wxEVT_FlushQueue)

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
			Console.Warning( wxString(L"wx > ") + szString );
		break;
    }
}


// ----------------------------------------------------------------------------
void ConsoleTestThread::ExecuteTaskInThread()
{
	static int numtrack = 0;

	while( !m_done )
	{
		// Two lines, both formatted, and varied colors.  This makes for a fairly realistic
		// worst case scenario (without being entirely unrealistic).
		Console.WriteLn( L"This is a threaded logging test. Something bad could happen... %d", ++numtrack );
		Console.Warning( L"Testing high stress loads %s", L"(multi-color)" );
		Yield( 0 );
	}
}

// ----------------------------------------------------------------------------
// Pass an uninitialized file object. The function will ask the user for the
// filename and try to open it. It returns true on success (file was opened),
// false if file couldn't be opened/created and -1 if the file selection
// dialog was canceled.
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
	m_table( ConsoleColors_Count )
{
	Create( fontsize );
}

ConsoleLogFrame::ColorArray::~ColorArray()
{
	Cleanup();
}

void ConsoleLogFrame::ColorArray::Create( int fontsize )
{
	const wxFont fixed( fontsize, wxMODERN, wxNORMAL, wxNORMAL );
	const wxFont fixedB( fontsize, wxMODERN, wxNORMAL, wxBOLD );

	// Standard R, G, B format:
	new (&m_table[Color_Default])		wxTextAttr( wxColor(   0,   0,   0 ), wxNullColour, fixed );
	new (&m_table[Color_Black])			wxTextAttr( wxColor(   0,   0,   0 ), wxNullColour, fixed );
	new (&m_table[Color_Red])			wxTextAttr( wxColor( 128,   0,   0 ), wxNullColour, fixed );
	new (&m_table[Color_Green])			wxTextAttr( wxColor(   0, 128,   0 ), wxNullColour, fixed );
	new (&m_table[Color_Blue])			wxTextAttr( wxColor(   0,   0, 128 ), wxNullColour, fixed );
	new (&m_table[Color_Magenta]	)	wxTextAttr( wxColor( 160,   0, 160 ), wxNullColour, fixed );
	new (&m_table[Color_Orange])		wxTextAttr( wxColor( 160, 120,   0 ), wxNullColour, fixed );
	new (&m_table[Color_Gray])			wxTextAttr( wxColor( 108, 108, 108 ), wxNullColour, fixed );

	new (&m_table[Color_Cyan])			wxTextAttr( wxColor( 128, 180, 180 ), wxNullColour, fixed );
	new (&m_table[Color_Yellow])		wxTextAttr( wxColor( 180, 180, 128 ), wxNullColour, fixed );
	new (&m_table[Color_White])			wxTextAttr( wxColor( 160, 160, 160 ), wxNullColour, fixed );

	new (&m_table[Color_StrongBlack])	wxTextAttr( wxColor(   0,   0,   0 ), wxNullColour, fixedB );
	new (&m_table[Color_StrongRed])		wxTextAttr( wxColor( 128,   0,   0 ), wxNullColour, fixedB );
	new (&m_table[Color_StrongGreen])	wxTextAttr( wxColor(   0, 128,   0 ), wxNullColour, fixedB );
	new (&m_table[Color_StrongBlue])	wxTextAttr( wxColor(   0,   0, 128 ), wxNullColour, fixedB );
	new (&m_table[Color_StrongMagenta])	wxTextAttr( wxColor( 160,   0, 160 ), wxNullColour, fixedB );
	new (&m_table[Color_StrongOrange])	wxTextAttr( wxColor( 160, 120,   0 ), wxNullColour, fixedB );
    new (&m_table[Color_StrongGray])	wxTextAttr( wxColor( 108, 108, 108 ), wxNullColour, fixedB );

	new (&m_table[Color_StrongCyan])	wxTextAttr( wxColor( 128, 180, 180 ), wxNullColour, fixedB );
	new (&m_table[Color_StrongYellow])	wxTextAttr( wxColor( 180, 180, 128 ), wxNullColour, fixedB );
	new (&m_table[Color_StrongWhite])	wxTextAttr( wxColor( 160, 160, 160 ), wxNullColour, fixedB );
}

void ConsoleLogFrame::ColorArray::Cleanup()
{
	// The contents of m_table were created with placement new, and must be
	// disposed of manually:

	for( int i=0; i<ConsoleColors_Count; ++i )
		m_table[i].~wxTextAttr();
}

// fixme - not implemented yet.
void ConsoleLogFrame::ColorArray::SetFont( const wxFont& font )
{
	//for( int i=0; i<ConsoleColors_Count; ++i )
	//	m_table[i].SetFont( font );
}

void ConsoleLogFrame::ColorArray::SetFont( int fontsize )
{
	Cleanup();
	Create( fontsize );
}

enum MenuIDs_t
{
	MenuID_FontSize_Small = 0x10,
	MenuID_FontSize_Normal,
	MenuID_FontSize_Large,
	MenuID_FontSize_Huge,
};

// ------------------------------------------------------------------------
ConsoleLogFrame::ConsoleLogFrame( MainEmuFrame *parent, const wxString& title, AppConfig::ConsoleLogOptions& options )
	: wxFrame(parent, wxID_ANY, title)
	, m_conf( options )
	, m_TextCtrl( *new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxHSCROLL | wxTE_READONLY | wxTE_RICH2 ) )
	, m_ColorTable( options.FontSize )

	, m_QueueColorSection( L"ConsoleLog::QueueColorSection" )
	, m_QueueBuffer( L"ConsoleLog::QueueBuffer" )
	, m_CurQueuePos( false )

	, m_threadlogger( EnableThreadedLoggingTest ? new ConsoleTestThread() : NULL )
{
	m_pendingFlushes			= 0;
	m_WaitingThreadsForFlush	= 0;

	m_ThawThrottle				= 0;
	m_ThawNeeded				= false;
	m_ThawPending				= false;

	SetIcons( wxGetApp().GetIconBundle() );
		
	m_TextCtrl.SetBackgroundColour( wxColor( 230, 235, 242 ) );
	m_TextCtrl.SetDefaultStyle( m_ColorTable[DefaultConsoleColor] );

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

	SetSize( wxRect( options.DisplayPosition, options.DisplaySize ) );
	Show( options.Visible );

	// Bind Events:

	Connect( wxID_OPEN,  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ConsoleLogFrame::OnOpen)  );
	Connect( wxID_CLOSE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ConsoleLogFrame::OnClose) );
	Connect( wxID_SAVE,  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ConsoleLogFrame::OnSave)  );
	Connect( wxID_CLEAR, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ConsoleLogFrame::OnClear) );

	Connect( MenuID_FontSize_Small, MenuID_FontSize_Huge, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( ConsoleLogFrame::OnFontSize ) );

	Connect( wxEVT_CLOSE_WINDOW,	wxCloseEventHandler(ConsoleLogFrame::OnCloseWindow) );
	Connect( wxEVT_MOVE,			wxMoveEventHandler(ConsoleLogFrame::OnMoveAround) );
	Connect( wxEVT_SIZE,			wxSizeEventHandler(ConsoleLogFrame::OnResize) );

	Connect( wxEVT_SetTitleText,	wxCommandEventHandler(ConsoleLogFrame::OnSetTitle) );
	Connect( wxEVT_DockConsole,		wxCommandEventHandler(ConsoleLogFrame::OnDockedMove) );

	Connect( wxEVT_FlushQueue,		wxCommandEventHandler(ConsoleLogFrame::OnFlushEvent) );

	if( m_threadlogger != NULL )
		m_threadlogger->Start();
}

ConsoleLogFrame::~ConsoleLogFrame()
{
	wxGetApp().OnProgramLogClosed();
}

int m_pendingFlushes = 0;

// Implementation note:  Calls SetColor and Write( text ).  Override those virtuals
// and this one will magically follow suite. :)
void ConsoleLogFrame::Write( ConsoleColors color, const wxString& text )
{
	pthread_testcancel();

	ScopedLock lock( m_QueueLock );

	if( m_QueueColorSection.GetLength() == 0 )
	{
		pxAssertMsg( m_CurQueuePos == 0, "Queue's character position didn't get reset in sync with it's ColorSection table." );
	}

	if( (m_QueueColorSection.GetLength() == 0) || ((color != Color_Current) && (m_QueueColorSection.GetLast().color != color)) )
	{
		++m_CurQueuePos;		// Don't overwrite the NULL;
		m_QueueColorSection.Add( ColorSection(color, m_CurQueuePos) );
	}

	int endpos = m_CurQueuePos + text.Length();
	m_QueueBuffer.MakeRoomFor( endpos + 1 );		// and the null!!
	memcpy_fast( &m_QueueBuffer[m_CurQueuePos], text.c_str(), sizeof(wxChar) * text.Length() );
	m_CurQueuePos = endpos;

	// this NULL may be overwritten if the next message sent doesn't perform a color change.
	m_QueueBuffer[m_CurQueuePos] = 0;

	// Idle events don't always pass (wx blocks them when moving windows or using menus, for
	// example).  So let's hackfix it so that an alternate message is posted if the queue is
	// "piling up."

	if( m_pendingFlushes == 0 )
	{
		wxCommandEvent evt( wxEVT_FlushQueue );
		evt.SetInt( 0 );
		GetEventHandler()->AddPendingEvent( evt );
	}

	++m_pendingFlushes;

	if( m_pendingFlushes > 24 && !wxThread::IsMain() )
	{
		++m_WaitingThreadsForFlush;
		lock.Release();

		if( !m_sem_QueueFlushed.Wait( wxTimeSpan( 0,0,0,500 ) ) )
		{
			// Necessary since the main thread could grab the lock and process before
			// the above function actually returns (gotta love threading!)
			lock.Acquire();
			if( m_WaitingThreadsForFlush != 0 ) --m_WaitingThreadsForFlush;
		}
		else
		{
			// give gui thread time to repaint and handle other pending messages.
			// (those are prioritized lower than wxEvents, typically, which means we
			// can't post a ping event since it'll still just starve out paint msgs.)
			Sleep(1);
		}
	}
}

void ConsoleLogFrame::Newline()
{
	Write( Color_Current, L"\n" );
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

// Special event received from a window we're docked against.
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
		m_threadlogger = NULL;
		wxGetApp().OnProgramLogClosed();
		event.Skip();
	}
}

void ConsoleLogFrame::OnOpen(wxCommandEvent& WXUNUSED(event))
{
	Show(true);
}

void ConsoleLogFrame::OnClose( wxCommandEvent& event )
{
	DoClose();
}

void ConsoleLogFrame::OnSave(wxCommandEvent& WXUNUSED(event))
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

void ConsoleLogFrame::OnClear(wxCommandEvent& WXUNUSED(event))
{
    m_TextCtrl.Clear();
}

void ConsoleLogFrame::OnFontSize( wxCommandEvent& evt )
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
	m_TextCtrl.SetDefaultStyle( m_ColorTable[Color_White] );

	// TODO: Process the attributes of each character and upgrade the font size,
	// while still retaining color and bold settings...  (might be slow but then
	// it hardly matters being a once-in-a-bluemoon action).

}

// ----------------------------------------------------------------------------
//  Logging Events (typically received from Console class interfaces)
// ----------------------------------------------------------------------------

void ConsoleLogFrame::OnSetTitle( wxCommandEvent& event )
{
	SetTitle( event.GetString() );
}

void ConsoleLogFrame::OnFlushEvent( wxCommandEvent& evt )
{
	ScopedLock locker( m_QueueLock );

	if( m_CurQueuePos != 0 )
	{
		DoFlushQueue();

#ifdef __WXMSW__
		// This nicely sets the scroll position to the end of our log window, regardless of if
		// the textctrl has focus or not.  The wxWidgets AppendText() function uses EM_LINESCROLL
		// instead, which tends to be much faster for high-volume logs, but also ends up refreshing
		// the console in sloppy fashion for normal logging.

		// (both are needed, the WM_VSCROLL makes the scrolling smooth, and the EM_LINESCROLL avoids
		// weird errors when the buffer reaches "max" and starts clearing old history)

		::SendMessage((HWND)m_TextCtrl.GetHWND(), EM_LINESCROLL, 0, 0xfffffff);
		::SendMessage((HWND)m_TextCtrl.GetHWND(), WM_VSCROLL, SB_BOTTOM, (LPARAM)NULL);
#endif
		//m_TextCtrl.Thaw();
	}

	// Implementation note: I tried desperately to move this into wxEVT_IDLE, on the theory that
	// we don't actually want to wake up pending threads until after the GUI's finished all its
	// paperwork.  But wxEVT_IDLE doesn't work when you click menus or the title bar of a window,
	// making it pretty well annoyingly useless for just about anything. >_<

	// Workaround: I added a Sleep(1) to the DoWrite method to give the GUI some time to
	// do its paperwork.

	if( m_WaitingThreadsForFlush > 0 )
	{
		do {
			m_sem_QueueFlushed.Post();
		} while( --m_WaitingThreadsForFlush > 0 );

		int count = m_sem_QueueFlushed.Count();
		while( count < 0 ) m_sem_QueueFlushed.Post();
	}
}

void ConsoleLogFrame::DoFlushQueue()
{
	int len = m_QueueColorSection.GetLength();
	pxAssert( len != 0 );

	// Note, freezing/thawing actually seems to cause more overhead than it solves.
	// It might be useful if we're posting like dozens of messages, but in our case
	// we only post 1-4 typically, so better to leave things enabled.
	//m_TextCtrl.Freeze();

	// Manual InsertionPoint tracking avoids a lot of overhead in SetInsertionPointEnd()
	wxTextPos insertPoint = m_TextCtrl.GetLastPosition();

	// cap at 256k for now...
	// fixme - 256k runs well on win32 but appears to be very sluggish on linux (but that could
	// be a result of my using Xming + CoLinux).  Might need platform dependent defaults here. --air
	if( (insertPoint + m_CurQueuePos) > 0x40000 )
	{
		int toKeep = 0x40000 - m_CurQueuePos;
		if( toKeep <= 10 )
		{
			m_TextCtrl.Clear();
			insertPoint = 0;
		}
		else
		{
			int toRemove = 0x40000 - toKeep;
			if( toRemove < 0x10000 ) toRemove = 0x10000;
			m_TextCtrl.Remove( 0, toRemove );
			insertPoint -= toRemove;
		}
	}

	m_TextCtrl.SetInsertionPoint( insertPoint );

	for( int i=0; i<len; ++i )
	{
		if( m_QueueColorSection[i].color != Color_Current )
			m_TextCtrl.SetDefaultStyle( m_ColorTable[m_QueueColorSection[i].color] );

		const wxString passin( &m_QueueBuffer[m_QueueColorSection[i].startpoint] );

		m_TextCtrl.WriteText( passin );
		insertPoint += passin.Length();
	}

	// Some reports on Windows7 have corrupted cursor when using insertPoint (or
	// +1 / -1 ).  This works better for some reason:
	m_TextCtrl.SetInsertionPointEnd(); //( insertPoint );

	m_CurQueuePos = 0;
	m_QueueColorSection.Clear();
	m_pendingFlushes = 0;
}

ConsoleLogFrame* Pcsx2App::GetProgramLog()
{
	return m_ProgramLogBox;
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

static void __concall ConsoleToFile_Newline()
{
#ifdef __LINUX__
	if (g_Conf->EmuOptions.ConsoleToStdio) ConsoleWriter_Stdio.Newline();
#endif

#ifdef __LINUX__
	fputc( '\n', emuLog );
#else
	fputs( "\r\n", emuLog );
#endif
}

static void __concall ConsoleToFile_DoWrite( const wxString& fmt )
{
#ifdef __LINUX__
	if (g_Conf->EmuOptions.ConsoleToStdio) ConsoleWriter_Stdio.DoWrite(fmt);
#endif

	px_fputs( emuLog, fmt.ToUTF8() );
}

static void __concall ConsoleToFile_DoWriteLn( const wxString& fmt )
{
	ConsoleToFile_DoWrite( fmt );
	ConsoleToFile_Newline();

	if (emuLog != NULL) fflush( emuLog );
}

static void __concall ConsoleToFile_SetTitle( const wxString& title )
{
    ConsoleWriter_Stdio.SetTitle(title);
}

static void __concall ConsoleToFile_DoSetColor( ConsoleColors color )
{
    ConsoleWriter_Stdio.DoSetColor(color);
}

extern const IConsoleWriter	ConsoleWriter_File;
const IConsoleWriter    ConsoleWriter_File =
{
	ConsoleToFile_DoWrite,
	ConsoleToFile_DoWriteLn,
	ConsoleToFile_DoSetColor,

	ConsoleToFile_Newline,
	ConsoleToFile_SetTitle,
};

// --------------------------------------------------------------------------------------
//  ConsoleToWindow Implementations
// --------------------------------------------------------------------------------------
template< const IConsoleWriter& secondary >
static void __concall ConsoleToWindow_SetTitle( const wxString& title )
{
    secondary.SetTitle(title);
	wxCommandEvent evt( wxEVT_SetTitleText );
	evt.SetString( title );
	wxGetApp().ProgramLog_PostEvent( evt );
}

template< const IConsoleWriter& secondary >
static void __concall ConsoleToWindow_DoSetColor( ConsoleColors color )
{
    secondary.DoSetColor(color);
}

template< const IConsoleWriter& secondary >
static void __concall ConsoleToWindow_Newline()
{
	secondary.Newline();
	((Pcsx2App&)*wxTheApp).GetProgramLog()->Newline();
}

template< const IConsoleWriter& secondary >
static void __concall ConsoleToWindow_DoWrite( const wxString& fmt )
{
	if( secondary.DoWrite != NULL )
		secondary.DoWrite( fmt );
	((Pcsx2App&)*wxTheApp).GetProgramLog()->Write( Console.GetColor(), fmt );
}

template< const IConsoleWriter& secondary >
static void __concall ConsoleToWindow_DoWriteLn( const wxString& fmt )
{
	if( secondary.DoWriteLn != NULL )
		secondary.DoWriteLn( fmt );
	((Pcsx2App&)*wxTheApp).GetProgramLog()->Write( Console.GetColor(), fmt + L"\n" );
}

typedef void __concall DoWriteFn(const wxString&);

static const IConsoleWriter	ConsoleWriter_Window =
{
	ConsoleToWindow_DoWrite<ConsoleWriter_Null>,
	ConsoleToWindow_DoWriteLn<ConsoleWriter_Null>,
	ConsoleToWindow_DoSetColor<ConsoleWriter_Null>,

	ConsoleToWindow_Newline<ConsoleWriter_Null>,
	ConsoleToWindow_SetTitle<ConsoleWriter_Null>,
};	

static const IConsoleWriter	ConsoleWriter_WindowAndFile =
{
	ConsoleToWindow_DoWrite<ConsoleWriter_File>,
	ConsoleToWindow_DoWriteLn<ConsoleWriter_File>,
	ConsoleToWindow_DoSetColor<ConsoleWriter_File>,

	ConsoleToWindow_Newline<ConsoleWriter_File>,
	ConsoleToWindow_SetTitle<ConsoleWriter_File>,
};

void Pcsx2App::EnableAllLogging() const
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
