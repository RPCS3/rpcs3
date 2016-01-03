#include "stdafx.h"
#include "stdafx_gui.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/state.h"
#include "rpcs3.h"
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
#ifdef _MSC_VER
#include "Emu/Io/XInput/XInputPadHandler.h"
#endif

#include "Emu/RSX/Null/NullGSRender.h"
#include "Emu/RSX/GL/GLGSRender.h"

#include "Gui/MsgDialog.h"
#include "Gui/SaveDataDialog.h"

#include "Gui/GLGSFrame.h"

#include "Emu/RSX/Null/NullGSRender.h"
#include "Emu/RSX/GL/GLGSRender.h"
#include "Emu/Audio/Null/NullAudioThread.h"
#include "Emu/Audio/AL/OpenALThread.h"
#ifdef _MSC_VER
#include "Emu/RSX/D3D12/D3D12GSRender.h"
#include "Emu/Audio/XAudio2/XAudio2Thread.h"
#endif

#ifdef _WIN32
#include <wx/msw/wrapwin.h>
#endif

#ifdef __unix__
#include <X11/Xlib.h>
#endif

wxDEFINE_EVENT(wxEVT_DBG_COMMAND, wxCommandEvent);

IMPLEMENT_APP(Rpcs3App)
Rpcs3App* TheApp;

bool Rpcs3App::OnInit()
{
	static const wxCmdLineEntryDesc desc[]
	{
		{ wxCMD_LINE_SWITCH, "h", "help", "Command line options:\nh (help): Help and commands\nt (test): For directly executing a (S)ELF", wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
		{ wxCMD_LINE_SWITCH, "t", "test", "Run in test mode on (S)ELF", wxCMD_LINE_VAL_NONE },
		{ wxCMD_LINE_PARAM, NULL, NULL, "(S)ELF", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
		{ wxCMD_LINE_NONE }
	};
	
	parser.SetDesc(desc);
	parser.SetCmdLine(argc, argv);

	if (parser.Parse())
	{
		// help was given, terminating
		this->Exit();
	}

	EmuCallbacks callbacks;

	callbacks.call_after = [](std::function<void()> func)
	{
		wxGetApp().CallAfter(std::move(func));
	};

	callbacks.process_events = [this]()
	{
		m_MainFrame->Update();
		wxGetApp().ProcessPendingEvents();
	};

	callbacks.send_dbg_command = [](DbgCommand id, CPUThread* t)
	{
		wxGetApp().SendDbgCommand(id, t);
	};

	callbacks.get_kb_handler = []() -> std::unique_ptr<KeyboardHandlerBase>
	{
		switch (auto mode = rpcs3::config.io.keyboard_handler_mode.value())
		{
		case io_handler_mode::null: return std::make_unique<NullKeyboardHandler>();
		case io_handler_mode::windows: return std::make_unique<WindowsKeyboardHandler>();
		default: throw EXCEPTION("Invalid Keyboard Handler Mode %d", +(u32)mode);
		}
	};

	callbacks.get_mouse_handler = []() -> std::unique_ptr<MouseHandlerBase>
	{
		switch (auto mode = rpcs3::config.io.mouse_handler_mode.value())
		{
		case io_handler_mode::null: return std::make_unique<NullMouseHandler>();
		case io_handler_mode::windows: return std::make_unique<WindowsMouseHandler>();
		default: throw EXCEPTION("Invalid Mouse Handler Mode %d", +(u32)mode);
		}
	};

	callbacks.get_pad_handler = []() -> std::unique_ptr<PadHandlerBase>
	{
		switch (auto mode = rpcs3::config.io.pad_handler_mode.value())
		{
		case io_handler_mode::null: return std::make_unique<NullPadHandler>();
		case io_handler_mode::windows: return std::make_unique<WindowsPadHandler>();
#ifdef _MSC_VER
		case io_handler_mode::xinput: return std::make_unique<XInputPadHandler>();
#endif
		default: throw EXCEPTION("Invalid Pad Handler Mode %d", (int)mode);
		}
	};

	callbacks.get_gs_frame = [](frame_type type) -> std::unique_ptr<GSFrameBase>
	{
		switch (type)
		{
		case frame_type::OpenGL: return std::make_unique<GLGSFrame>();
		case frame_type::DX12: return std::make_unique<GSFrame>("DirectX 12");
		case frame_type::Null: return std::make_unique<GSFrame>("Null");
		}

		throw EXCEPTION("Invalid Frame Type");
	};

	callbacks.get_gs_render = []() -> std::shared_ptr<GSRender>
	{
		switch (auto mode = rpcs3::state.config.rsx.renderer.value())
		{
		case rsx_renderer_type::Null: return std::make_shared<NullGSRender>();
		case rsx_renderer_type::OpenGL: return std::make_shared<GLGSRender>();
#ifdef _MSC_VER
		case rsx_renderer_type::DX12: return std::make_shared<D3D12GSRender>();
#endif
		default: throw EXCEPTION("Invalid GS Renderer %d", (int)mode);
		}
	};

	callbacks.get_audio = []() -> std::shared_ptr<AudioThread>
	{
		switch (rpcs3::state.config.audio.out.value())
		{
		default:
		case audio_output_type::Null: return std::make_shared<NullAudioThread>();
		case audio_output_type::OpenAL: return std::make_shared<OpenALThread>();
#ifdef _MSC_VER
		case audio_output_type::XAudio2: return std::make_shared<XAudio2Thread>();
#endif
		}
	};

	callbacks.get_msg_dialog = []() -> std::shared_ptr<MsgDialogBase>
	{
		return std::make_shared<MsgDialogFrame>();
	};

	callbacks.get_save_dialog = []() -> std::unique_ptr<SaveDialogBase>
	{
		return std::make_unique<SaveDialogFrame>();
	};

	Emu.SetCallbacks(std::move(callbacks));

	TheApp = this;
	SetAppName(_PRGNAME_);
	wxInitAllImageHandlers();

	Emu.Init();

	m_MainFrame = new MainFrame();
	SetTopWindow(m_MainFrame);
	m_MainFrame->Show();
	m_MainFrame->DoSettings(true);

	OnArguments(parser);

	return true;
}

void Rpcs3App::OnArguments(const wxCmdLineParser& parser)
{
	// Usage:
	//   rpcs3-*.exe               Initializes RPCS3
	//   rpcs3-*.exe [(S)ELF]      Initializes RPCS3, then loads and runs the specified (S)ELF file.

	if (parser.FoundSwitch("t"))
	{
		HLEExitOnStop = rpcs3::config.misc.exit_on_stop.value();
		rpcs3::config.misc.exit_on_stop = true;
		if (parser.GetParamCount() != 1)
		{
			wxLogDebug(wxT("A (S)ELF file needs to be given in test mode, exiting."));
			this->Exit();
		}
	}
	
	if (parser.GetParamCount() > 0)
	{
		Emu.SetPath(fmt::ToUTF8(parser.GetParam(0)));
		Emu.Load();
		Emu.Run();
	}
}

void Rpcs3App::Exit()
{
	if (parser.FoundSwitch("t"))
	{
		rpcs3::config.misc.exit_on_stop = HLEExitOnStop;
	}

	Emu.Stop();
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
#ifdef _WIN32
	timeBeginPeriod(1);

	std::atexit([]
	{
		timeEndPeriod(1);
	});
#endif

#if defined(__unix__) && !defined(__APPLE__)
	XInitThreads();
#endif
}
