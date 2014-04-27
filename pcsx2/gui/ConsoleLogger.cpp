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
#include "MainFrame.h"
#include "ConsoleLogger.h"
#include "MSWstuff.h"

#include "Utilities/Console.h"
#include "Utilities/IniInterface.h"
#include "Utilities/SafeArray.inl"
#include "DebugTools/Debug.h"

#include <wx/textfile.h>

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE(pxEvt_LogWrite, -1)
	DECLARE_EVENT_TYPE(pxEvt_SetTitleText, -1)
	DECLARE_EVENT_TYPE(pxEvt_FlushQueue, -1)
END_DECLARE_EVENT_TYPES()

DEFINE_EVENT_TYPE(pxEvt_LogWrite)
DEFINE_EVENT_TYPE(pxEvt_SetTitleText)
DEFINE_EVENT_TYPE(pxEvt_DockConsole)
DEFINE_EVENT_TYPE(pxEvt_FlushQueue)

// C++ requires abstract destructors to exist, even though they're abstract.
PipeRedirectionBase::~PipeRedirectionBase() throw() {}

// ----------------------------------------------------------------------------
//
void pxLogConsole::DoLog( wxLogLevel level, const wxChar *szString, time_t t )
{
	switch ( level )
	{
		case wxLOG_Trace:
		case wxLOG_Debug:
		{
			wxString str;
			TimeStamp( &str );
			MSW_OutputDebugString( str + szString + L"\n" );
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
			Console.WriteLn( L"[wx] %s", szString );
		break;

		case wxLOG_Error:
			Console.Error( L"[wx] %s", szString );
		break;

		case wxLOG_Warning:
			Console.Warning( L"[wx] %s", szString );
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

		switch ( Msgbox::ShowModal( _("Save log question"), strMsg, MsgButtons().YesNo().Cancel() ) )
		{
			case wxID_YES:
				bAppend = true;
			break;

			case wxID_NO:
				bAppend = false;
			break;

			case wxID_CANCEL:
				return false;

			default:
				pxFailDev( "invalid message box return value" );
		}

		return ( bAppend ) ?
			file.Open(filename, wxFile::write_append) :
			file.Create(filename, true /* overwrite */);
	}

	return file.Create(filename);
}

// --------------------------------------------------------------------------------------
//  ConsoleLogFrame::ColorArray  (implementations)
// --------------------------------------------------------------------------------------

// fontsize - size of the font specified in points.
//   (actual font used is the system-selected fixed-width font)
//
ConsoleLogFrame::ColorArray::ColorArray( int fontsize )
	: m_table( ConsoleColors_Count )
{
	Create( fontsize );
}

ConsoleLogFrame::ColorArray::~ColorArray() throw()
{
	Cleanup();
}

void ConsoleLogFrame::ColorArray::Create( int fontsize )
{
	const wxFont fixed( pxGetFixedFont( fontsize ) );
	const wxFont fixedB( pxGetFixedFont( fontsize+1, wxBOLD ) );

	//const wxFont fixed( fontsize, wxMODERN, wxNORMAL, wxNORMAL );
	//const wxFont fixedB( fontsize, wxMODERN, wxNORMAL, wxBOLD );

	// Standard R, G, B format:
	new (&m_table[Color_Default])		wxTextAttr( wxNullColour, wxNullColour, fixed );
	new (&m_table[Color_Black])			wxTextAttr( wxNullColour, wxNullColour, fixed );
	new (&m_table[Color_Red])			wxTextAttr( wxNullColour, wxNullColour, fixed );
	new (&m_table[Color_Green])			wxTextAttr( wxNullColour, wxNullColour, fixed );
	new (&m_table[Color_Blue])			wxTextAttr( wxNullColour, wxNullColour, fixed );
	new (&m_table[Color_Magenta])		wxTextAttr( wxNullColour, wxNullColour, fixed );
	new (&m_table[Color_Orange])		wxTextAttr( wxNullColour, wxNullColour, fixed );
	new (&m_table[Color_Gray])			wxTextAttr( wxNullColour, wxNullColour, fixed );

	new (&m_table[Color_Cyan])			wxTextAttr( wxNullColour, wxNullColour, fixed );
	new (&m_table[Color_Yellow])		wxTextAttr( wxNullColour, wxNullColour, fixed );
	new (&m_table[Color_White])			wxTextAttr( wxNullColour, wxNullColour, fixed );

	new (&m_table[Color_StrongBlack])	wxTextAttr( wxNullColour, wxNullColour, fixedB );
	new (&m_table[Color_StrongRed])		wxTextAttr( wxNullColour, wxNullColour, fixedB );
	new (&m_table[Color_StrongGreen])	wxTextAttr( wxNullColour, wxNullColour, fixedB );
	new (&m_table[Color_StrongBlue])	wxTextAttr( wxNullColour, wxNullColour, fixedB );
	new (&m_table[Color_StrongMagenta])	wxTextAttr( wxNullColour, wxNullColour, fixedB );
	new (&m_table[Color_StrongOrange])	wxTextAttr( wxNullColour, wxNullColour, fixedB );
	new (&m_table[Color_StrongGray])	wxTextAttr( wxNullColour, wxNullColour, fixedB );

	new (&m_table[Color_StrongCyan])	wxTextAttr( wxNullColour, wxNullColour, fixedB );
	new (&m_table[Color_StrongYellow])	wxTextAttr( wxNullColour, wxNullColour, fixedB );
	new (&m_table[Color_StrongWhite])	wxTextAttr( wxNullColour, wxNullColour, fixedB );

	SetColorScheme_Light();
}

void ConsoleLogFrame::ColorArray::SetColorScheme_Dark()
{
	m_table[Color_Default]		.SetTextColour(wxColor( 208, 208, 208 ));
	m_table[Color_Black]		.SetTextColour(wxColor( 255, 255, 255 ));
	m_table[Color_Red]			.SetTextColour(wxColor( 180,   0,   0 ));
	m_table[Color_Green]		.SetTextColour(wxColor(   0, 160,   0 ));
	m_table[Color_Blue]			.SetTextColour(wxColor(  32,  32, 204 ));
	m_table[Color_Magenta]		.SetTextColour(wxColor( 160,   0, 160 ));
	m_table[Color_Orange]		.SetTextColour(wxColor( 160, 120,   0 ));
	m_table[Color_Gray]			.SetTextColour(wxColor( 128, 128, 128 ));

	m_table[Color_Cyan]			.SetTextColour(wxColor( 128, 180, 180 ));
	m_table[Color_Yellow]		.SetTextColour(wxColor( 180, 180, 128 ));
	m_table[Color_White]		.SetTextColour(wxColor( 160, 160, 160 ));

	m_table[Color_StrongBlack]	.SetTextColour(wxColor( 255, 255, 255 ));
	m_table[Color_StrongRed]	.SetTextColour(wxColor( 180,   0,   0 ));
	m_table[Color_StrongGreen]	.SetTextColour(wxColor(   0, 160,   0 ));
	m_table[Color_StrongBlue]	.SetTextColour(wxColor(  32,  32, 204 ));
	m_table[Color_StrongMagenta].SetTextColour(wxColor( 160,   0, 160 ));
	m_table[Color_StrongOrange]	.SetTextColour(wxColor( 160, 120,   0 ));
	m_table[Color_StrongGray]	.SetTextColour(wxColor( 128, 128, 128 ));

	m_table[Color_StrongCyan]	.SetTextColour(wxColor( 128, 180, 180 ));
	m_table[Color_StrongYellow]	.SetTextColour(wxColor( 180, 180, 128 ));
	m_table[Color_StrongWhite]	.SetTextColour(wxColor( 160, 160, 160 ));
}

void ConsoleLogFrame::ColorArray::SetColorScheme_Light()
{
	m_table[Color_Default]		.SetTextColour(wxColor(   0,   0,   0 ));
	m_table[Color_Black]		.SetTextColour(wxColor(   0,   0,   0 ));
	m_table[Color_Red]			.SetTextColour(wxColor( 128,   0,   0 ));
	m_table[Color_Green]		.SetTextColour(wxColor(   0, 128,   0 ));
	m_table[Color_Blue]			.SetTextColour(wxColor(   0,   0, 128 ));
	m_table[Color_Magenta]		.SetTextColour(wxColor( 160,   0, 160 ));
	m_table[Color_Orange]		.SetTextColour(wxColor( 160, 120,   0 ));
	m_table[Color_Gray]			.SetTextColour(wxColor( 108, 108, 108 ));

	m_table[Color_Cyan]			.SetTextColour(wxColor( 128, 180, 180 ));
	m_table[Color_Yellow]		.SetTextColour(wxColor( 180, 180, 128 ));
	m_table[Color_White]		.SetTextColour(wxColor( 160, 160, 160 ));

	m_table[Color_StrongBlack]	.SetTextColour(wxColor(   0,   0,   0 ));
	m_table[Color_StrongRed]	.SetTextColour(wxColor( 128,   0,   0 ));
	m_table[Color_StrongGreen]	.SetTextColour(wxColor(   0, 128,   0 ));
	m_table[Color_StrongBlue]	.SetTextColour(wxColor(   0,   0, 128 ));
	m_table[Color_StrongMagenta].SetTextColour(wxColor( 160,   0, 160 ));
	m_table[Color_StrongOrange]	.SetTextColour(wxColor( 160, 120,   0 ));
	m_table[Color_StrongGray]	.SetTextColour(wxColor( 108, 108, 108 ));

	m_table[Color_StrongCyan]	.SetTextColour(wxColor( 128, 180, 180 ));
	m_table[Color_StrongYellow]	.SetTextColour(wxColor( 180, 180, 128 ));
	m_table[Color_StrongWhite]	.SetTextColour(wxColor( 160, 160, 160 ));
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
	MenuId_FontSize_Small = 0x10,
	MenuId_FontSize_Normal,
	MenuId_FontSize_Large,
	MenuId_FontSize_Huge,

	MenuId_ColorScheme_Light = 0x20,
	MenuId_ColorScheme_Dark,

	MenuId_LogSource_EnableAll = 0x30,
	MenuId_LogSource_DisableAll,
	MenuId_LogSource_Devel,
	MenuId_LogSource_CDVD_Info,

	MenuId_LogSource_Start = 0x100
};

#define pxTheApp ((Pcsx2App&)*wxTheApp)

// --------------------------------------------------------------------------------------
//  ScopedLogLock  (implementations)
// --------------------------------------------------------------------------------------
class ScopedLogLock : public ScopedLock
{
public:
	ConsoleLogFrame*	WindowPtr;

public:
	ScopedLogLock()
		: ScopedLock( wxThread::IsMain() ? NULL : &pxTheApp.GetProgramLogLock() )
	{
		WindowPtr = pxTheApp.m_ptr_ProgramLog;
	}

	virtual ~ScopedLogLock() throw() {}

	bool HasWindow() const
	{
		return WindowPtr != NULL;
	}

	ConsoleLogFrame& GetWindow() const
	{
		return *WindowPtr;
	}
};

// WARNING ConsoleLogSources & ConLogDefaults must have the same size
static ConsoleLogSource* const ConLogSources[] =
{
	(ConsoleLogSource*)&SysConsole.eeConsole,
	(ConsoleLogSource*)&SysConsole.iopConsole,
	(ConsoleLogSource*)&SysConsole.eeRecPerf,
	NULL,
	(ConsoleLogSource*)&SysConsole.ELF,
	NULL,
	(ConsoleLogSource*)&pxConLog_Event,
	(ConsoleLogSource*)&pxConLog_Thread,
};

// WARNING ConsoleLogSources & ConLogDefaults must have the same size
static const bool ConLogDefaults[] =
{
	true,
	true,
	false,
	true,
	false,
	false,
	false,
	false
};

void ConLog_LoadSaveSettings( IniInterface& ini )
{
	ScopedIniGroup path(ini, L"ConsoleLogSources");

	ini.Entry( L"Devel", DevConWriterEnabled, false );

	uint srcnt = ArraySize(ConLogSources);
	for (uint i=0; i<srcnt; ++i)
	{
		if (ConsoleLogSource* log = ConLogSources[i])
		{
			ini.Entry( log->GetCategory() + L"." + log->GetShortName(), log->Enabled, ConLogDefaults[i] );
		}
	}
}


// --------------------------------------------------------------------------------------
//  ConsoleLogFrame  (implementations)
// --------------------------------------------------------------------------------------
ConsoleLogFrame::ConsoleLogFrame( MainEmuFrame *parent, const wxString& title, AppConfig::ConsoleLogOptions& options )
	: wxFrame(parent, wxID_ANY, title)
	, m_conf( options )
	, m_TextCtrl( *new pxLogTextCtrl(this) )
	, m_timer_FlushUnlocker( this )
	, m_ColorTable( options.FontSize )

	, m_QueueColorSection( L"ConsoleLog::QueueColorSection" )
	, m_QueueBuffer( L"ConsoleLog::QueueBuffer" )
	, m_threadlogger( EnableThreadedLoggingTest ? new ConsoleTestThread() : NULL )
{
	m_CurQueuePos				= 0;
	m_WaitingThreadsForFlush	= 0;
	m_pendingFlushMsg			= false;
	m_FlushRefreshLocked		= false;

	// create Log menu (contains most options)
	wxMenuBar *pMenuBar = new wxMenuBar();
	SetMenuBar( pMenuBar );
	SetIcons( wxGetApp().GetIconBundle() );

	if (0==m_conf.Theme.CmpNoCase(L"Dark"))
	{
		m_ColorTable.SetColorScheme_Dark();
		m_TextCtrl.SetBackgroundColour( wxColor( 0, 0, 0 ) );
	}
	else //if ((0==m_conf.Theme.CmpNoCase("Default")) || (0==m_conf.Theme.CmpNoCase("Light")))
	{
		m_ColorTable.SetColorScheme_Light();
		m_TextCtrl.SetBackgroundColour( wxColor( 230, 235, 242 ) );
	}

	m_TextCtrl.SetDefaultStyle( m_ColorTable[DefaultConsoleColor] );

	// SetDefaultStyle only sets the style of text in the control.  We need to
	// also set the font of the control, so that sizing logic knows what font we use:
	m_TextCtrl.SetFont( m_ColorTable[DefaultConsoleColor].GetFont() );

	wxMenu& menuLog		(*new wxMenu());
	wxMenu& menuAppear	(*new wxMenu());
	wxMenu& menuSources	(*new wxMenu());
	wxMenu& menuFontSizes( menuAppear );

	// create Appearance menu and submenus

	menuFontSizes.Append( MenuId_FontSize_Small,	_("Small"),	_t("Fits a lot of log in a microcosmically small area."),
		wxITEM_RADIO )->Check( options.FontSize == 7 );
	menuFontSizes.Append( MenuId_FontSize_Normal,	_("Normal"),_t("It's what I use (the programmer guy)."),
		wxITEM_RADIO )->Check( options.FontSize == 8 );
	menuFontSizes.Append( MenuId_FontSize_Large,	_("Large"),	_t("Its nice and readable."),
		wxITEM_RADIO )->Check( options.FontSize == 10 );
	menuFontSizes.Append( MenuId_FontSize_Huge,		_("Huge"),	_t("In case you have a really high res display."),
		wxITEM_RADIO )->Check( options.FontSize == 12 );

	menuFontSizes.AppendSeparator();
	menuFontSizes.Append( MenuId_ColorScheme_Light,	_("Light theme"),	_t("Default soft-tone color scheme."), wxITEM_RADIO );
	menuFontSizes.Append( MenuId_ColorScheme_Dark,	_("Dark theme"),	_t("Classic black color scheme for people who enjoy having text seared into their optic nerves."), wxITEM_RADIO );

	menuAppear.AppendSeparator();
	menuAppear.Append( wxID_ANY, _("Always on Top"),
		_t("When checked the log window will be visible over other foreground windows."), wxITEM_CHECK );

	menuLog.Append(wxID_SAVE,	_("&Save..."),		_("Save log contents to file"));
	menuLog.Append(wxID_CLEAR,	_("C&lear"),		_("Clear the log window contents"));
	menuLog.AppendSeparator();
	menuLog.AppendSubMenu( &menuAppear, _("Appearance") );
	menuLog.AppendSeparator();
	menuLog.Append(wxID_CLOSE,	_("&Close"),		_("Close this log window; contents are preserved"));

	// Source Selection/Toggle menu

	menuSources.Append( MenuId_LogSource_Devel, _("Dev/Verbose"), _("Shows PCSX2 developer logs"), wxITEM_CHECK );
	menuSources.Append( MenuId_LogSource_CDVD_Info, _("CDVD reads"), _("Shows disk read activity"), wxITEM_CHECK );
	
	menuSources.AppendSeparator();

	uint srcnt = ArraySize(ConLogSources);
	for (uint i=0; i<srcnt; ++i)
	{
		if (const BaseTraceLogSource* log = ConLogSources[i])
		{
			menuSources.Append( MenuId_LogSource_Start+i, log->GetName(), log->GetDescription(), wxITEM_CHECK );
			Connect( MenuId_LogSource_Start+i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ConsoleLogFrame::OnToggleSource));
		}
		else
			menuSources.AppendSeparator();
	}

	menuSources.AppendSeparator();
	menuSources.Append( MenuId_LogSource_EnableAll,		_("Enable all"),	_("Enables all log source filters.") );
	menuSources.Append( MenuId_LogSource_DisableAll,	_("Disable all"),	_("Disables all log source filters.") );

	pMenuBar->Append(&menuLog,		_("&Log"));
	pMenuBar->Append(&menuSources,	_("&Sources"));

	// status bar for menu prompts
	CreateStatusBar();

	SetSize( wxRect( options.DisplayPosition, options.DisplaySize ) );
	Show( options.Visible );

	// Bind Events:

	Connect( wxID_OPEN,  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ConsoleLogFrame::OnOpen)  );
	Connect( wxID_CLOSE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ConsoleLogFrame::OnClose) );
	Connect( wxID_SAVE,  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ConsoleLogFrame::OnSave)  );
	Connect( wxID_CLEAR, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ConsoleLogFrame::OnClear) );

	Connect( MenuId_FontSize_Small,		MenuId_FontSize_Huge,		wxEVT_COMMAND_MENU_SELECTED,	wxCommandEventHandler( ConsoleLogFrame::OnFontSize ) );
	Connect( MenuId_ColorScheme_Light,	MenuId_ColorScheme_Dark,	wxEVT_COMMAND_MENU_SELECTED,	wxCommandEventHandler( ConsoleLogFrame::OnToggleTheme ) );

	Connect( MenuId_LogSource_Devel,		wxEVT_COMMAND_MENU_SELECTED,	wxCommandEventHandler( ConsoleLogFrame::OnToggleSource ) );
	Connect( MenuId_LogSource_CDVD_Info,	wxEVT_COMMAND_MENU_SELECTED,	wxCommandEventHandler( ConsoleLogFrame::OnToggleCDVDInfo ) );
	Connect( MenuId_LogSource_EnableAll,	wxEVT_COMMAND_MENU_SELECTED,	wxCommandEventHandler( ConsoleLogFrame::OnEnableAllLogging ) );
	Connect( MenuId_LogSource_DisableAll,	wxEVT_COMMAND_MENU_SELECTED,	wxCommandEventHandler( ConsoleLogFrame::OnDisableAllLogging ) );

	Connect( wxEVT_CLOSE_WINDOW,	wxCloseEventHandler			(ConsoleLogFrame::OnCloseWindow) );
	Connect( wxEVT_MOVE,			wxMoveEventHandler			(ConsoleLogFrame::OnMoveAround) );
	Connect( wxEVT_SIZE,			wxSizeEventHandler			(ConsoleLogFrame::OnResize) );
	Connect( wxEVT_ACTIVATE,		wxActivateEventHandler		(ConsoleLogFrame::OnActivate) );

	Connect( pxEvt_SetTitleText,	wxCommandEventHandler	(ConsoleLogFrame::OnSetTitle) );
	Connect( pxEvt_DockConsole,		wxCommandEventHandler	(ConsoleLogFrame::OnDockedMove) );
	Connect( pxEvt_FlushQueue,		wxCommandEventHandler	(ConsoleLogFrame::OnFlushEvent) );

	Connect( m_timer_FlushUnlocker.GetId(),	wxEVT_TIMER,	wxTimerEventHandler	(ConsoleLogFrame::OnFlushUnlockerTimer) );

	if( m_threadlogger != NULL )
		m_threadlogger->Start();

	OnLoggingChanged();

	if (0==m_conf.Theme.CmpNoCase(L"Dark"))
	{
		pMenuBar->Check(MenuId_ColorScheme_Dark, true);
	}
	else //if ((0==m_conf.Theme.CmpNoCase("Default")) || (0==m_conf.Theme.CmpNoCase("Light")))
	{
		pMenuBar->Check(MenuId_ColorScheme_Light, true);
	}
}

ConsoleLogFrame::~ConsoleLogFrame()
{
	ScopedLogLock locker;
	wxGetApp().OnProgramLogClosed( GetId() );
}

void ConsoleLogFrame::OnEnableAllLogging(wxCommandEvent& evt)
{
	uint srcnt = ArraySize(ConLogSources);
	for (uint i=0; i<srcnt; ++i)
	{
		if (ConsoleLogSource* log = ConLogSources[i])
			log->Enabled = true;
	}

	OnLoggingChanged();
	evt.Skip();
}

void ConsoleLogFrame::OnDisableAllLogging(wxCommandEvent& evt)
{
	uint srcnt = ArraySize(ConLogSources);
	for (uint i=0; i<srcnt; ++i)
	{
		if (ConsoleLogSource* log = ConLogSources[i])
			log->Enabled = false;
	}

	OnLoggingChanged();
	evt.Skip();
}

void ConsoleLogFrame::OnLoggingChanged()
{
	if (!GetMenuBar()) return;

	if( wxMenuItem* item = GetMenuBar()->FindItem(MenuId_LogSource_Devel) )
		item->Check( DevConWriterEnabled );

	if( wxMenuItem* item = GetMenuBar()->FindItem(MenuId_LogSource_CDVD_Info) )
		item->Check( g_Conf->EmuOptions.CdvdVerboseReads );

	uint srcnt = ArraySize(ConLogSources);
	for (uint i=0; i<srcnt; ++i)
	{
		if (const ConsoleLogSource* log = ConLogSources[i])
		{
			GetMenuBar()->Check( MenuId_LogSource_Start+i, log->IsActive() );
		}
	}
}

// Implementation note:  Calls SetColor and Write( text ).  Override those virtuals
// and this one will magically follow suite. :)
bool ConsoleLogFrame::Write( ConsoleColors color, const wxString& text )
{
	pthread_testcancel();

	ScopedLock lock( m_mtx_Queue );

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

	if( !m_pendingFlushMsg )
	{
		m_pendingFlushMsg = true;

		// wxWidgets may have aggressive locks on event processing, so best to release
		// our own mutex lest wx get hung for an extended period of time and cause all
		// of our own stuff to get sluggish.
		lock.Release();

		wxCommandEvent evt( pxEvt_FlushQueue );
		evt.SetInt( 0 );
		if( wxThread::IsMain() )
		{
			OnFlushEvent( evt );
			return false;
		}
		else
			GetEventHandler()->AddPendingEvent( evt );

		lock.Acquire();
	}

	if( !wxThread::IsMain() )
	{
		// Too many color changes causes huge slowdowns when decorating the rich textview, so
		// include a secondary check to avoid having a colorful log spam killing gui responsiveness.

		if( m_CurQueuePos > 0x100000 || m_QueueColorSection.GetLength() > 256 )
		{
			++m_WaitingThreadsForFlush;
			lock.Release();

			// Note: if the queue flushes, we need to return TRUE, so that our thread sleeps
			// until the main thread has had a chance to repaint the console window contents.
			// [TODO] : It'd be a lot better if the console window repaint released the lock
			//  once its task were complete, but thats been problematic, so for now this hack is
			//  what we get.
			if( m_sem_QueueFlushed.Wait( wxTimeSpan( 0,0,0,250 ) ) ) return true;

			// If we're here it means QueueFlush wait timed out, so remove us from the waiting
			// threads count. This way no thread permanently deadlocks against the console
			// logger.  They just run quite slow, but should remain responsive to user input.
			lock.Acquire();
			if( m_WaitingThreadsForFlush != 0 ) --m_WaitingThreadsForFlush;
		}
	}

	return false;
}

bool ConsoleLogFrame::Newline()
{
	return Write( Color_Current, L"\n" );
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
	if( IsBeingDeleted() || !IsVisible() || IsIconized() ) return;

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

// ----------------------------------------------------------------------------
// OnFocus / OnActivate : Special implementation to "connect" the console log window
// with the main frame window.  When one is clicked, the other is assured to be brought
// to the foreground with it.  (Currently only MSW only, as wxWidgets appears to have no
// equivalent to this). We don't bother with OnFocus here because it doesn't propagate
// up the window hierarchy anyway, so it always gets swallowed by the text control.
// But no matter: the console doesn't have the same problem as the Main Window of missing
// the initial activation event.

/*void ConsoleLogFrame::OnFocus( wxFocusEvent& evt )
{
	if( MainEmuFrame* mainframe = GetMainFramePtr() )
		MSW_SetWindowAfter( mainframe->GetHandle(), GetHandle() );

	evt.Skip();
}*/

void ConsoleLogFrame::OnActivate( wxActivateEvent& evt )
{
	if( MainEmuFrame* mainframe = GetMainFramePtr() )
		MSW_SetWindowAfter( mainframe->GetHandle(), GetHandle() );

	evt.Skip();
}
// ----------------------------------------------------------------------------

void ConsoleLogFrame::OnCloseWindow(wxCloseEvent& event)
{
	if( event.CanVeto() )
	{
		// instead of closing just hide the window to be able to Show() it later
		Show( false );

		// Can't do this via a Connect() on the MainFrame because Close events are not commands,
		// and thus do not propagate up/down the event chain.
		if( wxWindow* main = GetParent() )
			wxStaticCast( main, MainEmuFrame )->OnLogBoxHidden();
	}
	else
	{
		// This is sent when the app is exiting typically, so do a full close.
		m_threadlogger = NULL;
		wxGetApp().OnProgramLogClosed( GetId() );
		event.Skip();
	}
}

void ConsoleLogFrame::OnOpen(wxCommandEvent& WXUNUSED(event))
{
	Show(true);
}

void ConsoleLogFrame::OnClose( wxCommandEvent& event )
{
	Close( false );
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
}

void ConsoleLogFrame::OnClear(wxCommandEvent& WXUNUSED(event))
{
	m_TextCtrl.Clear();
}

void ConsoleLogFrame::OnToggleCDVDInfo( wxCommandEvent& evt )
{
	evt.Skip();

	if (!GetMenuBar()) return;

	if (evt.GetId() == MenuId_LogSource_CDVD_Info)
	{
		if( wxMenuItem* item = GetMenuBar()->FindItem(evt.GetId()) )
		{
			g_Conf->EmuOptions.CdvdVerboseReads = item->IsChecked();
			const_cast<Pcsx2Config&>(EmuConfig).CdvdVerboseReads = g_Conf->EmuOptions.CdvdVerboseReads;		// read-only in core thread, so it's safe to modify.
		}
	}
}

void ConsoleLogFrame::OnToggleSource( wxCommandEvent& evt )
{
	evt.Skip();

	if (!GetMenuBar()) return;

	if (evt.GetId() == MenuId_LogSource_Devel)
	{
		if( wxMenuItem* item = GetMenuBar()->FindItem(evt.GetId()) )
			DevConWriterEnabled = item->IsChecked();

		return;
	}

	uint srcid = evt.GetId() - MenuId_LogSource_Start;

	if (!pxAssertDev( ArraySize(ConLogSources) > srcid, "Invalid source log index (out of bounds)" )) return;
	if (!pxAssertDev( ConLogSources[srcid] != NULL, "Invalid source log index (NULL pointer [separator])" )) return;

	if( wxMenuItem* item = GetMenuBar()->FindItem(evt.GetId()) )
	{
		pxAssertDev( item->IsCheckable(), "Uncheckable log source menu item?  Seems fishy!" );
		ConLogSources[srcid]->Enabled = item->IsChecked();
	}
}

void ConsoleLogFrame::OnToggleTheme( wxCommandEvent& evt )
{
	evt.Skip();

	const wxChar* newTheme = L"Default";

	switch( evt.GetId() )
	{
		case MenuId_ColorScheme_Light:
			newTheme = L"Default";
			m_ColorTable.SetColorScheme_Light();
			m_TextCtrl.SetBackgroundColour( wxColor( 230, 235, 242 ) );
		break;

		case MenuId_ColorScheme_Dark:
			newTheme = L"Dark";
			m_ColorTable.SetColorScheme_Dark();
			m_TextCtrl.SetBackgroundColour( wxColor( 24, 24, 24 ) );
		break;
	}

	if (0 == m_conf.Theme.CmpNoCase(newTheme)) return;
	m_conf.Theme = newTheme;

	m_ColorTable.SetFont( m_conf.FontSize );
	m_TextCtrl.SetDefaultStyle( m_ColorTable[Color_White] );
}

void ConsoleLogFrame::OnFontSize( wxCommandEvent& evt )
{
	evt.Skip();

	int ptsize = 8;
	switch( evt.GetId() )
	{
		case MenuId_FontSize_Small:		ptsize = 7; break;
		case MenuId_FontSize_Normal:	ptsize = 8; break;
		case MenuId_FontSize_Large:		ptsize = 10; break;
		case MenuId_FontSize_Huge:		ptsize = 12; break;
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

void ConsoleLogFrame::DoFlushEvent( bool isPending )
{
	// recursion guard needed due to Mutex lock/acquire code below, which can end up yielding
	// to the gui and attempting to process more messages (which in turn would result in re-
	// entering this handler).

	static int recursion_counter = 0;
	RecursionGuard recguard( recursion_counter );
	if( recguard.IsReentrant() ) return;

	ScopedLock locker( m_mtx_Queue );

	if( m_CurQueuePos != 0 )
	{
		DoFlushQueue();
	}

	// Implementation note: I tried desperately to move this into wxEVT_IDLE, on the theory that
	// we don't actually want to wake up pending threads until after the GUI's finished all its
	// paperwork.  But wxEVT_IDLE doesn't work when you click menus or the title bar of a window,
	// making it pretty well annoyingly useless for just about anything. >_<

	if( m_WaitingThreadsForFlush > 0 )
	{
		do {
			m_sem_QueueFlushed.Post();
		} while( --m_WaitingThreadsForFlush > 0 );

		int count = m_sem_QueueFlushed.Count();
		while( count < 0 ) m_sem_QueueFlushed.Post();
	}

	m_pendingFlushMsg = isPending;
}

void ConsoleLogFrame::OnFlushUnlockerTimer( wxTimerEvent& )
{
	m_FlushRefreshLocked = false;
	DoFlushEvent( false );
}

void ConsoleLogFrame::OnFlushEvent( wxCommandEvent& )
{
	if( m_FlushRefreshLocked ) return;

	DoFlushEvent( true );

	m_FlushRefreshLocked = true;
	m_timer_FlushUnlocker.Start( 100, true );
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

	// cap at 512k for now...
	// fixme - 512k runs well on win32 but appears to be very sluggish on linux (but that could
	// be a result of my using Xming + CoLinux).  Might need platform dependent defaults here. --air

	static const int BufferSize = 0x80000;
	if( (insertPoint + m_CurQueuePos) > BufferSize )
	{
		int toKeep = BufferSize - m_CurQueuePos;
		if( toKeep <= 10 )
		{
			m_TextCtrl.Clear();
			insertPoint = 0;
		}
		else
		{
			int toRemove = BufferSize - toKeep;
			if( toRemove < BufferSize / 4 ) toRemove = BufferSize;
			m_TextCtrl.Remove( 0, toRemove );
			insertPoint -= toRemove;
		}
	}

	m_TextCtrl.SetInsertionPoint( insertPoint );

	// fixme : Writing a lot of colored logs to the console can be quite slow when "spamming"
	// is happening, due to the overhead of SetDefaultStyle and WriteText calls.  I'm not sure
	// if there's a better way to go about this?  Using Freeze/Thaw helps a little bit, but it's
	// still magnitudes slower than dumping a straight run. --air

	if( len > 64 ) m_TextCtrl.Freeze();

	for( int i=0; i<len; ++i )
	{
		if( m_QueueColorSection[i].color != Color_Current )
			m_TextCtrl.SetDefaultStyle( m_ColorTable[m_QueueColorSection[i].color] );

		m_TextCtrl.WriteText( &m_QueueBuffer[m_QueueColorSection[i].startpoint] );
	}

	if( len > 64 ) m_TextCtrl.Thaw();

	m_TextCtrl.ConcludeIssue();

	m_QueueColorSection.Clear();
	m_CurQueuePos		= 0;
}

ConsoleLogFrame* Pcsx2App::GetProgramLog()
{
	return (ConsoleLogFrame*) wxWindow::FindWindowById( m_id_ProgramLogBox );
}

const ConsoleLogFrame* Pcsx2App::GetProgramLog() const
{
	return (const ConsoleLogFrame*) wxWindow::FindWindowById( m_id_ProgramLogBox );
}

void Pcsx2App::ProgramLog_PostEvent( wxEvent& evt )
{
	if( ConsoleLogFrame* proglog = GetProgramLog() )
		proglog->GetEventHandler()->AddPendingEvent( evt );
}

// --------------------------------------------------------------------------------------
//  ConsoleImpl_ToFile
// --------------------------------------------------------------------------------------

static void __concall ConsoleToFile_Newline()
{
#ifdef __LINUX__
	if ((g_Conf) && (g_Conf->EmuOptions.ConsoleToStdio)) ConsoleWriter_Stdout.Newline();
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
	if ((g_Conf) && (g_Conf->EmuOptions.ConsoleToStdio)) ConsoleWriter_Stdout.WriteRaw(fmt);
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
	ConsoleWriter_Stdout.SetTitle(title);
}

static void __concall ConsoleToFile_DoSetColor( ConsoleColors color )
{
	ConsoleWriter_Stdout.DoSetColor(color);
}

extern const IConsoleWriter	ConsoleWriter_File;
const IConsoleWriter	ConsoleWriter_File =
{
	ConsoleToFile_DoWrite,
	ConsoleToFile_DoWriteLn,
	ConsoleToFile_DoSetColor,

	ConsoleToFile_DoWrite,
	ConsoleToFile_Newline,
	ConsoleToFile_SetTitle,

	0
};

Mutex& Pcsx2App::GetProgramLogLock()
{
	return m_mtx_ProgramLog;
}

// --------------------------------------------------------------------------------------
//  ConsoleToWindow Implementations
// --------------------------------------------------------------------------------------
template< const IConsoleWriter& secondary >
static void __concall ConsoleToWindow_SetTitle( const wxString& title )
{
	secondary.SetTitle(title);
	wxCommandEvent evt( pxEvt_SetTitleText );
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

	ScopedLogLock locker;
	bool needsSleep = locker.WindowPtr && locker.WindowPtr->Newline();
	locker.Release();
	if( needsSleep ) wxGetApp().Ping();
}

template< const IConsoleWriter& secondary >
static void __concall ConsoleToWindow_DoWrite( const wxString& fmt )
{
	if( secondary.WriteRaw != NULL )
		secondary.WriteRaw( fmt );

	ScopedLogLock locker;
	bool needsSleep = locker.WindowPtr && locker.WindowPtr->Write( Console.GetColor(), fmt );
	locker.Release();
	if( needsSleep ) wxGetApp().Ping();
}

template< const IConsoleWriter& secondary >
static void __concall ConsoleToWindow_DoWriteLn( const wxString& fmt )
{
	if( secondary.DoWriteLn != NULL )
		secondary.DoWriteLn( fmt );

	ScopedLogLock locker;
	bool needsSleep = locker.WindowPtr && locker.WindowPtr->Write( Console.GetColor(), fmt + L'\n' );
	locker.Release();
	if( needsSleep ) wxGetApp().Ping();
}

typedef void __concall DoWriteFn(const wxString&);

static const IConsoleWriter	ConsoleWriter_Window =
{
	ConsoleToWindow_DoWrite<ConsoleWriter_Stdout>,
	ConsoleToWindow_DoWriteLn<ConsoleWriter_Stdout>,
	ConsoleToWindow_DoSetColor<ConsoleWriter_Stdout>,

	ConsoleToWindow_DoWrite<ConsoleWriter_Stdout>,
	ConsoleToWindow_Newline<ConsoleWriter_Stdout>,
	ConsoleToWindow_SetTitle<ConsoleWriter_Stdout>,

	0
};

static const IConsoleWriter	ConsoleWriter_WindowAndFile =
{
	ConsoleToWindow_DoWrite<ConsoleWriter_File>,
	ConsoleToWindow_DoWriteLn<ConsoleWriter_File>,
	ConsoleToWindow_DoSetColor<ConsoleWriter_File>,

	ConsoleToWindow_DoWrite<ConsoleWriter_File>,
	ConsoleToWindow_Newline<ConsoleWriter_File>,
	ConsoleToWindow_SetTitle<ConsoleWriter_File>,

	0
};

void Pcsx2App::EnableAllLogging()
{
	AffinityAssert_AllowFrom_MainUI();

	const bool logBoxOpen = (m_ptr_ProgramLog != NULL);
	const IConsoleWriter* newHandler = NULL;

	if( emuLog )
	{
		if( !m_StdoutRedirHandle ) m_StdoutRedirHandle = NewPipeRedir(stdout);
		if( !m_StderrRedirHandle ) m_StderrRedirHandle = NewPipeRedir(stderr);
		newHandler = logBoxOpen ? (IConsoleWriter*)&ConsoleWriter_WindowAndFile : (IConsoleWriter*)&ConsoleWriter_File;
	}
	else
	{
		if( logBoxOpen )
		{
			if( !m_StdoutRedirHandle ) m_StdoutRedirHandle = NewPipeRedir(stdout);
			if( !m_StderrRedirHandle ) m_StderrRedirHandle = NewPipeRedir(stderr);
			newHandler = &ConsoleWriter_Window;
		}
		else
			newHandler = &ConsoleWriter_Stdout;
	}
	Console_SetActiveHandler( *newHandler );
}

// Used to disable the emuLog disk logger, typically used when disabling or re-initializing the
// emuLog file handle.  Call SetConsoleLogging to re-enable the disk logger when finished.
void Pcsx2App::DisableDiskLogging() const
{
	AffinityAssert_AllowFrom_MainUI();

	const bool logBoxOpen = (GetProgramLog() != NULL);
	Console_SetActiveHandler( logBoxOpen ? (IConsoleWriter&)ConsoleWriter_Window : (IConsoleWriter&)ConsoleWriter_Stdout );

	// Semi-hack: It's possible, however very unlikely, that a secondary thread could attempt
	// to write to the logfile just before we disable logging, and would thus have a pending write
	// operation to emuLog file handle at the same time we're trying to re-initialize it.  The CRT
	// has some guards of its own, and PCSX2 itself typically suspends the "log happy" threads
	// when changing settings, so the chance for problems is low.  We minimize it further here
	// by sleeping off 5ms, which should allow any pending log-to-disk events to finish up.
	//
	// (the most correct solution would be a mutex lock in the Disk logger itself, but for now I
	//  am going to try and keep the logger lock-free and use this semi-hack instead).

	Threading::Sleep( 5 );
}

void Pcsx2App::DisableWindowLogging() const
{
	AffinityAssert_AllowFrom_MainUI();
	Console_SetActiveHandler( (emuLog!=NULL) ? (IConsoleWriter&)ConsoleWriter_File : (IConsoleWriter&)ConsoleWriter_Stdout );
}
