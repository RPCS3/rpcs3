#include "stdafx_gui.h"
#include "Emu/System.h"
#include "rpcs3.h"
#include "Ini.h"
#include "Gui/ConLogFrame.h"
#include "Emu/GameInfo.h"

#include "Emu/Io/Keyboard.h"
#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "Emu/Io/Windows/WindowsKeyboardHandler.h"

#include "Emu/Io/Mouse.h"
#include "Emu/Io/Null/NullMouseHandler.h"
#include "Emu/Io/Windows/WindowsMouseHandler.h"

#include "Emu/Io/Pad.h"
#include "Emu/Io/Null/NullPadHandler.h"
#include "Emu/Io/Windows/WindowsPadHandler.h"
#if defined(_WIN32)
#include "Emu/Io/XInput/XInputPadHandler.h"
#endif

#include "Gui/GLGSFrame.h"
#include <wx/stdpaths.h>

#ifdef _WIN32
#include <wx/msw/wrapwin.h>
#endif

#ifdef __UNIX__
#include <X11/Xlib.h>
#endif

wxDEFINE_EVENT(wxEVT_DBG_COMMAND, wxCommandEvent);

IMPLEMENT_APP(Rpcs3App)
Rpcs3App* TheApp;

bool Rpcs3App::OnInit()
{
	SetSendDbgCommandCallback([](DbgCommand id, CPUThread* t)
	{
		wxGetApp().SendDbgCommand(id, t);
	});
	SetCallAfterCallback([](std::function<void()> func)
	{
		wxGetApp().CallAfter(func);
	});
	SetGetKeyboardHandlerCountCallback([]()
	{
		return 2;
	});
	SetGetKeyboardHandlerCallback([](int i) -> KeyboardHandlerBase*
	{
		switch (i)
		{
		case 0:
			return new NullKeyboardHandler();
			break;
		case 1:
			return new WindowsKeyboardHandler();
			break;
		default:
			return new NullKeyboardHandler();
		}
	});
	SetGetMouseHandlerCountCallback([]()
	{
		return 2;
	});
	SetGetMouseHandlerCallback([](int i) -> MouseHandlerBase*
	{
		switch (i)
		{
		case 0:
			return new NullMouseHandler();
			break;
		case 1:
			return new WindowsMouseHandler();
			break;
		default:
			return new NullMouseHandler();
		}
	});
	SetGetPadHandlerCountCallback([]()
	{
#if defined(_WIN32)
		return 3;
#else
		return 2;
#endif
	});
	SetGetPadHandlerCallback([](int i) -> PadHandlerBase*
	{
		switch (i)
		{
		case 0:
			return new NullPadHandler();
			break;
		case 1:
			return new WindowsPadHandler();
			break;
#if defined(_WIN32)
		case 2:
			return new XInputPadHandler();
			break;
#endif
		default:
			return new NullPadHandler();
		}
	});
	SetGetGSFrameCallback([]() -> GSFrameBase*
	{
		return new GLGSFrame();
	});

	TheApp = this;
	SetAppName(_PRGNAME_);
	wxInitAllImageHandlers();

	// RPCS3 assumes the current working directory is the folder where it is contained, so we make sure this is true
	const wxString executablePath = wxStandardPaths::Get().GetExecutablePath();
	wxSetWorkingDirectory(wxPathOnly(executablePath));

	main_thread = std::this_thread::get_id();

	Ini.Load();
	m_MainFrame = new MainFrame();
	SetTopWindow(m_MainFrame);
	Emu.Init();

	m_MainFrame->Show();
	m_MainFrame->DoSettings(true);

	OnArguments();

	return true;
}

void Rpcs3App::OnArguments()
{
	// Usage:
	//   rpcs3-*.exe               Initializes RPCS3
	//   rpcs3-*.exe [(S)ELF]      Initializes RPCS3, then loads and runs the specified (S)ELF file.

	if (Rpcs3App::argc > 1)
	{
		// Force this value to be true
		Ini.HLEExitOnStop.SetValue(true);

		Emu.SetPath(fmt::ToUTF8(argv[1]));
		Emu.Load();
		Emu.Run();
	}
}

void Rpcs3App::Exit()
{
	Emu.Stop();
	Ini.Save();

	wxApp::Exit();
}

void Rpcs3App::SendDbgCommand(DbgCommand id, CPUThread* thr)
{
	wxCommandEvent event(wxEVT_DBG_COMMAND, id);
	event.SetClientData(thr);
	AddPendingEvent(event);
}

Rpcs3App::Rpcs3App()
{
	#if defined(__UNIX__) && !defined(__APPLE__)
	XInitThreads();
	#endif
}

GameInfo CurGameInfo;
