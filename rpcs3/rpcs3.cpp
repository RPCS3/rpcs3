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
#include "DS4PadHandler.h"
#ifdef _MSC_VER
#include "XInputPadHandler.h"
#endif
#ifdef _WIN32
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
#ifdef __linux__
#include "Emu/Audio/ALSA/ALSAThread.h"
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

	callbacks.get_kb_handler = []() -> std::shared_ptr<KeyboardHandlerBase>
	{
		switch (keyboard_handler type = g_cfg.io.keyboard)
		{
		case keyboard_handler::null: return std::make_shared<NullKeyboardHandler>();
		case keyboard_handler::basic: return std::make_shared<BasicKeyboardHandler>();
		default: fmt::throw_exception("Invalid keyboard handler: %s", type);
		}
	};

	callbacks.get_mouse_handler = []() -> std::shared_ptr<MouseHandlerBase>
	{
		switch (mouse_handler type = g_cfg.io.mouse)
		{
		case mouse_handler::null: return std::make_shared<NullMouseHandler>();
		case mouse_handler::basic: return std::make_shared<BasicMouseHandler>();
		default: fmt::throw_exception("Invalid mouse handler: %s", type);
		}
	};

	callbacks.get_pad_handler = []() -> std::shared_ptr<PadHandlerBase>
	{
		switch (pad_handler type = g_cfg.io.pad)
		{
		case pad_handler::null: return std::make_shared<NullPadHandler>();
		case pad_handler::keyboard: return std::make_shared<KeyboardPadHandler>();
		case pad_handler::ds4: return std::make_shared<DS4PadHandler>();
#ifdef _MSC_VER
		case pad_handler::xinput: return std::make_shared<XInputPadHandler>();
#endif
#ifdef _WIN32
		case pad_handler::mm: return std::make_shared<MMJoystickHandler>();
#endif
		default: fmt::throw_exception("Invalid pad handler: %s", type);
		}
	};

	callbacks.get_gs_frame = []() -> std::unique_ptr<GSFrameBase>
	{
		extern const std::unordered_map<video_resolution, std::pair<int, int>, value_hash<video_resolution>> g_video_out_resolution_map;

		const auto size = g_video_out_resolution_map.at(g_cfg.video.resolution);

		switch (video_renderer type = g_cfg.video.renderer)
		{
		case video_renderer::null: return std::make_unique<GSFrame>("Null", size.first, size.second);
		case video_renderer::opengl: return std::make_unique<GLGSFrame>(size.first, size.second);
#ifdef _WIN32
		case video_renderer::vulkan: return std::make_unique<GSFrame>("Vulkan", size.first, size.second);
#endif
#ifdef _MSC_VER
		case video_renderer::dx12: return std::make_unique<GSFrame>("DirectX 12", size.first, size.second);
#endif
		default: fmt::throw_exception("Invalid video renderer: %s" HERE, type);
		}
	};

	callbacks.get_gs_render = []() -> std::shared_ptr<GSRender>
	{
		switch (video_renderer type = g_cfg.video.renderer)
		{
		case video_renderer::null: return std::make_shared<NullGSRender>();
		case video_renderer::opengl: return std::make_shared<GLGSRender>();
#ifdef _WIN32
		case video_renderer::vulkan: return std::make_shared<VKGSRender>();
#endif
#ifdef _MSC_VER
		case video_renderer::dx12: return std::make_shared<D3D12GSRender>();
#endif
		default: fmt::throw_exception("Invalid video renderer: %s" HERE, type);
		}
	};

	callbacks.get_audio = []() -> std::shared_ptr<AudioThread>
	{
		switch (audio_renderer type = g_cfg.audio.renderer)
		{
		case audio_renderer::null: return std::make_shared<NullAudioThread>();
#ifdef _WIN32
		case audio_renderer::xaudio: return std::make_shared<XAudio2Thread>();
#elif __linux__
		case audio_renderer::alsa: return std::make_shared<ALSAThread>();
#endif
		case audio_renderer::openal: return std::make_shared<OpenALThread>();
		default: fmt::throw_exception("Invalid audio renderer: %s" HERE, type);
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
		g_cfg.misc.autostart.from_string("true");
		g_cfg.misc.autoexit.from_string("true");
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
