#include "stdafx.h"
#include "stdafx_gui.h"
#include "rpcs3.h"

#include "Utilities/Config.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Gui/ConLogFrame.h"
#include "Emu/GameInfo.h"

#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "BasicKeyboardHandler.h"

#include "Emu/Io/Null/NullMouseHandler.h"
#include "BasicMouseHandler.h"

#include "Emu/Io/Null/NullPadHandler.h"
#include "KeyboardPadHandler.h"
#ifdef _MSC_VER
#include "XInputPadHandler.h"
#include "MMJoystickHandler.h"
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
#endif
#ifdef _WIN32
#include "Emu/RSX/VK/VKGSRender.h"
#include "Emu/Audio/XAudio2/XAudio2Thread.h"
#include <wx/msw/wrapwin.h>
#endif

#ifdef __unix__
#include <X11/Xlib.h>
#endif

// GUI config
YAML::Node g_gui_cfg;

// GUI config file
static fs::file s_gui_cfg;

void save_gui_cfg()
{
	YAML::Emitter out;
	out.SetSeqFormat(YAML::Flow);
	out << g_gui_cfg;

	// Save to file
	s_gui_cfg.seek(0);
	s_gui_cfg.trunc(0);
	s_gui_cfg.write(out.c_str(), out.size());
}

IMPLEMENT_APP(Rpcs3App)
Rpcs3App* TheApp;

cfg::map_entry<std::function<std::shared_ptr<KeyboardHandlerBase>()>> g_cfg_kb_handler(cfg::root.io, "Keyboard",
{
	{ "Null", &std::make_shared<NullKeyboardHandler> },
	{ "Basic", &std::make_shared<BasicKeyboardHandler> },
});

cfg::map_entry<std::function<std::shared_ptr<MouseHandlerBase>()>> g_cfg_mouse_handler(cfg::root.io, "Mouse",
{
	{ "Null", &std::make_shared<NullMouseHandler> },
	{ "Basic", &std::make_shared<BasicMouseHandler> },
});

cfg::map_entry<std::function<std::shared_ptr<PadHandlerBase>()>> g_cfg_pad_handler(cfg::root.io, "Controller", "Keyboard",
{
	{ "Null", &std::make_shared<NullPadHandler> },
	{ "Keyboard", &std::make_shared<KeyboardPadHandler> },
#ifdef _MSC_VER
	{ "XInput", &std::make_shared<XInputPadHandler> },
	{ "MMJoystick", &std::make_shared<MMJoystickHandler>},
#endif
});

cfg::map_entry<std::function<std::shared_ptr<GSRender>()>> g_cfg_gs_render(cfg::root.video, "Rendering API", "OpenGL",
{
	{ "Null", &std::make_shared<NullGSRender> },
	{ "OpenGL", &std::make_shared<GLGSRender> },
#ifdef _MSC_VER
	{ "DirectX 12", &std::make_shared<D3D12GSRender> },
#endif
#ifdef _WIN32
	{ "Vulkan", &std::make_shared<VKGSRender> },
#endif
});

cfg::map_entry<std::function<std::shared_ptr<AudioThread>()>> g_cfg_audio_render(cfg::root.audio, "Renderer", 1,
{
	{ "Null", &std::make_shared<NullAudioThread> },
#ifdef _WIN32
	{ "XAudio2", &std::make_shared<XAudio2Thread> },
#endif
	{ "OpenAL", &std::make_shared<OpenALThread> },
});

extern cfg::bool_entry g_cfg_autostart;
extern cfg::bool_entry g_cfg_autoexit;

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

	s_gui_cfg.open(fs::get_config_dir() + "/config_gui.yml", fs::read + fs::write + fs::create);
	g_gui_cfg = YAML::Load(s_gui_cfg.to_string());

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

	callbacks.exit = [this]()
	{
		wxGetApp().Exit();
	};

	callbacks.get_kb_handler = []{ return g_cfg_kb_handler.get()(); };

	callbacks.get_mouse_handler = []{ return g_cfg_mouse_handler.get()(); };

	callbacks.get_pad_handler = []{ return g_cfg_pad_handler.get()(); };

	callbacks.get_gs_frame = [](frame_type type, int w, int h) -> std::unique_ptr<GSFrameBase>
	{
		switch (type)
		{
		case frame_type::OpenGL: return std::make_unique<GLGSFrame>(w, h);
		case frame_type::DX12: return std::make_unique<GSFrame>("DirectX 12", w, h);
		case frame_type::Null: return std::make_unique<GSFrame>("Null", w, h);
		case frame_type::Vulkan: return std::make_unique<GSFrame>("Vulkan", w, h);
		}

		fmt::throw_exception("Invalid frame type (0x%x)" HERE, (int)type);
	};

	callbacks.get_gs_render = []{ return g_cfg_gs_render.get()(); };

	callbacks.get_audio = []{ return g_cfg_audio_render.get()(); };

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
	SetAppName("RPCS3");
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
		if (parser.GetParamCount() != 1)
		{
			wxLogDebug("A (S)ELF file needs to be given in test mode, exiting.");
			this->Exit();
		}

		// TODO: clean implementation
		g_cfg_autostart = true;
		g_cfg_autoexit = true;
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
	Emu.Stop();
	wxApp::Exit();
}

Rpcs3App::Rpcs3App()
{
#ifdef _WIN32
	timeBeginPeriod(1);

	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSAStartup(wVersionRequested, &wsaData);

	std::atexit([]
	{
		timeEndPeriod(1);
		WSACleanup();
	});
#endif

#if defined(__unix__) && !defined(__APPLE__)
	XInitThreads();
#endif
}
