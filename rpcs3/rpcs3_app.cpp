#include "rpcs3_app.h"

#include "rpcs3qt/welcome_dialog.h"

#include "Emu/System.h"
#include "rpcs3qt/gs_frame.h"
#include "rpcs3qt/gl_gs_frame.h"

#include "Emu/Cell/Modules/cellSaveData.h"
#include "rpcs3qt/save_data_dialog.h"

#include "rpcs3qt/msg_dialog_frame.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"

#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "basic_keyboard_handler.h"

#include "Emu/Io/Null/NullMouseHandler.h"
#include "basic_mouse_handler.h"

#include "Emu/Io/Null/NullPadHandler.h"
#include "keyboard_pad_handler.h"
#include "ds4_pad_handler.h"
#ifdef _MSC_VER
#include "xinput_pad_handler.h"
#endif
#ifdef _WIN32
#include "mm_joystick_handler.h"
#endif


#include "Emu/RSX/Null/NullGSRender.h"
#include "Emu/RSX/GL/GLGSRender.h"
#include "Emu/Audio/Null/NullAudioThread.h"
#include "Emu/Audio/AL/OpenALThread.h"
#ifdef _MSC_VER
#include "Emu/RSX/D3D12/D3D12GSRender.h"
#endif
#if defined(_WIN32) || defined(__linux__)
#include "Emu/RSX/VK/VKGSRender.h"
#endif
#ifdef _WIN32
#include "Emu/Audio/XAudio2/XAudio2Thread.h"
#endif
#ifdef __linux__
#include "Emu/Audio/ALSA/ALSAThread.h"
#endif

// For now, a trivial constructor/destructor.  May add command line usage later.
rpcs3_app::rpcs3_app(int& argc, char** argv) : QApplication(argc, argv)
{
}

void rpcs3_app::Init()
{
	Emu.Init();

	guiSettings.reset(new gui_settings());

	// Create the main window
	RPCS3MainWin = new main_window(guiSettings, nullptr);

	// Reset the pads -- see the method for why this is currently needed.
	ResetPads();

	// Create callbacks from the emulator, which reference the handlers.
	InitializeCallbacks();

	// Create connects to propagate events throughout Gui.
	InitializeConnects();

	RPCS3MainWin->Init();

	setApplicationName("RPCS3");
	RPCS3MainWin->show();

	// Create the thumbnail toolbar after the main_window is created
	RPCS3MainWin->CreateThumbnailToolbar();

	if (guiSettings->GetValue(GUI::ib_show_welcome).toBool())
	{
		welcome_dialog* welcome = new welcome_dialog();
		welcome->exec();
	}
}

/** RPCS3 emulator has functions it desires to call from the GUI at times. Initialize them in here.
*/
void rpcs3_app::InitializeCallbacks()
{
	EmuCallbacks callbacks;

	callbacks.exit = [this]()
	{
		quit();
	};
	callbacks.call_after = [=](std::function<void()> func)
	{	
		RequestCallAfter(std::move(func));
	};

	callbacks.process_events = [this]()
	{
		RPCS3MainWin->update();
		processEvents();
	};

	callbacks.get_kb_handler = [=]() -> std::shared_ptr<KeyboardHandlerBase>
	{
		switch (keyboard_handler type = g_cfg.io.keyboard)
		{
		case keyboard_handler::null: return std::make_shared<NullKeyboardHandler>();
		case keyboard_handler::basic: return  m_basicKeyboardHandler;
		default: fmt::throw_exception("Invalid keyboard handler: %s", type);
		}
	};

	callbacks.get_mouse_handler = [=]() -> std::shared_ptr<MouseHandlerBase>
	{
		switch (mouse_handler type = g_cfg.io.mouse)
		{
		case mouse_handler::null: return std::make_shared<NullMouseHandler>();
		case mouse_handler::basic: return m_basicMouseHandler;
		default: fmt::throw_exception("Invalid mouse handler: %s", type);
		}
	};

	callbacks.get_pad_handler = [this]() -> std::shared_ptr<PadHandlerBase>
	{
		switch (pad_handler type = g_cfg.io.pad)
		{
		case pad_handler::null: return std::make_shared<NullPadHandler>();
		case pad_handler::keyboard: return m_keyboardPadHandler;
		case pad_handler::ds4: return std::make_shared<ds4_pad_handler>();
#ifdef _MSC_VER
		case pad_handler::xinput: return std::make_shared<xinput_pad_handler>();
#endif
#ifdef _WIN32
		case pad_handler::mm: return std::make_shared<mm_joystick_handler>();
#endif
		default: fmt::throw_exception("Invalid pad handler: %s", type);
		}
	};

	callbacks.get_gs_frame = [this]() -> std::unique_ptr<GSFrameBase>
	{
		extern const std::unordered_map<video_resolution, std::pair<int, int>, value_hash<video_resolution>> g_video_out_resolution_map;

		const auto size = g_video_out_resolution_map.at(g_cfg.video.resolution);
		int w = size.first;
		int h = size.second;

		if (guiSettings->GetValue(GUI::gs_resize).toBool())
		{
			w = guiSettings->GetValue(GUI::gs_width).toInt();
			h = guiSettings->GetValue(GUI::gs_height).toInt();
		}

		switch (video_renderer type = g_cfg.video.renderer)
		{
		case video_renderer::null: return std::make_unique<gs_frame>("Null", w, h, RPCS3MainWin->GetAppIcon());
		case video_renderer::opengl: return std::make_unique<gl_gs_frame>(w, h, RPCS3MainWin->GetAppIcon());
		case video_renderer::vulkan: return std::make_unique<gs_frame>("Vulkan", w, h, RPCS3MainWin->GetAppIcon());
#ifdef _MSC_VER
		case video_renderer::dx12: return std::make_unique<gs_frame>("DirectX 12", w, h, RPCS3MainWin->GetAppIcon());
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
#if defined(_WIN32) || defined(__linux__)
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

	callbacks.get_msg_dialog = [=]() -> std::shared_ptr<MsgDialogBase>
	{
		return std::make_shared<msg_dialog_frame>(RPCS3MainWin->windowHandle());
	};

	callbacks.get_save_dialog = [=]() -> std::unique_ptr<SaveDialogBase>
	{
		return std::make_unique<save_data_dialog>();
	};

	callbacks.on_run = [=]() { OnEmulatorRun(); };
	callbacks.on_pause = [=]() { OnEmulatorPause(); };
	callbacks.on_resume = [=]() { OnEmulatorResume(); };
	callbacks.on_stop = [=]() { OnEmulatorStop(); };
	callbacks.on_ready = [=]() { OnEmulatorReady(); };

	Emu.SetCallbacks(std::move(callbacks));
}

/*
 * Initialize connects here. These are used to connect things between UI elements that require the intervention of rpcs3_app.
 */
void rpcs3_app::InitializeConnects()
{
	connect(RPCS3MainWin, &main_window::RequestGlobalStylesheetChange, this, &rpcs3_app::OnChangeStyleSheetRequest);

	qRegisterMetaType <std::function<void()>>("std::function<void()>");
	connect(this, &rpcs3_app::RequestCallAfter, this, &rpcs3_app::HandleCallAfter);

	connect(this, &rpcs3_app::OnEmulatorRun, RPCS3MainWin, &main_window::OnEmuRun);
	connect(this, &rpcs3_app::OnEmulatorStop, this, &rpcs3_app::ResetPads);
	connect(this, &rpcs3_app::OnEmulatorStop, RPCS3MainWin, &main_window::OnEmuStop);
	connect(this, &rpcs3_app::OnEmulatorPause, RPCS3MainWin, &main_window::OnEmuPause);
	connect(this, &rpcs3_app::OnEmulatorResume, RPCS3MainWin, &main_window::OnEmuResume);
	connect(this, &rpcs3_app::OnEmulatorReady, RPCS3MainWin, &main_window::OnEmuReady);
}

/*
* Handle a request to change the stylesheet. May consider adding reporting of errors in future.
* Empty string means default.
*/
void rpcs3_app::OnChangeStyleSheetRequest(const QString& sheetFilePath)
{
	QFile file(sheetFilePath);
	if (sheetFilePath == "")
	{
		setStyleSheet("");
	}
	else if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		setStyleSheet(file.readAll());
		file.close();
	}
}

/**
 * Using connects avoids timers being unable to be used in a non-qt thread.  So, even if this looks stupid to just call func, it's succinct.
*/
void rpcs3_app::HandleCallAfter(const std::function<void()>& func)
{
	func();
}

/**
 * We need to make this in the main thread to receive events from the main thread.
 * This leads to the tricky situation.  Creating it while booting leads to deadlock with a blocking connection.
 * So, I need to make them before, but when?
 * I opted to reset them when the Emu stops and on first init. Potentially a race condition on restart? Never encountered issues.
 * The other tricky issue is that I don't want Init to be called twice on the same object. Reseting the pointer on emu stop should handle this as well!
*/
void rpcs3_app::ResetPads()
{
	m_basicKeyboardHandler.reset(new basic_keyboard_handler(this, this));
	m_basicMouseHandler.reset(new basic_mouse_handler(this, this));
	m_keyboardPadHandler.reset(new keyboard_pad_handler(this, this));
}
