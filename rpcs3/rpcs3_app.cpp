#include "rpcs3_app.h"

#include "rpcs3qt/qt_utils.h"

#include "rpcs3qt/welcome_dialog.h"

#include "Emu/System.h"
#include "rpcs3qt/gs_frame.h"
#include "rpcs3qt/gl_gs_frame.h"

#include "rpcs3qt/trophy_notification_helper.h"
#include "rpcs3qt/save_data_dialog.h"
#include "rpcs3qt/msg_dialog_frame.h"

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
#ifdef HAVE_LIBEVDEV
#include "evdev_joystick_handler.h"
#endif

#include "pad_thread.h"

#include "Emu/RSX/Null/NullGSRender.h"
#include "Emu/RSX/GL/GLGSRender.h"
#include "Emu/Audio/Null/NullAudioThread.h"
#include "Emu/Audio/AL/OpenALThread.h"
#ifdef _MSC_VER
#include "Emu/RSX/D3D12/D3D12GSRender.h"
#endif
#if defined(_WIN32) || defined(HAVE_VULKAN)
#include "Emu/RSX/VK/VKGSRender.h"
#endif
#ifdef _WIN32
#include "Emu/Audio/XAudio2/XAudio2Thread.h"
#endif
#ifdef HAVE_ALSA
#include "Emu/Audio/ALSA/ALSAThread.h"
#endif
#ifdef HAVE_PULSE
#include "Emu/Audio/Pulse/PulseThread.h"
#endif

// For now, a trivial constructor/destructor.  May add command line usage later.
rpcs3_app::rpcs3_app(int& argc, char** argv) : QApplication(argc, argv)
{
}

void rpcs3_app::Init()
{
	setApplicationName("RPCS3");
	setWindowIcon(QIcon(":/rpcs3.ico"));

	Emu.Init();

	guiSettings.reset(new gui_settings());
	emuSettings.reset(new emu_settings());

	// Create the main window
	RPCS3MainWin = new main_window(guiSettings, emuSettings, nullptr);

	// Create callbacks from the emulator, which reference the handlers.
	InitializeCallbacks();

	// Create connects to propagate events throughout Gui.
	InitializeConnects();

	RPCS3MainWin->Init();

	RPCS3MainWin->show();

	// Create the thumbnail toolbar after the main_window is created
	RPCS3MainWin->CreateThumbnailToolbar();

	if (guiSettings->GetValue(gui::ib_show_welcome).toBool())
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
		case keyboard_handler::basic:
		{
			basic_keyboard_handler* ret = new basic_keyboard_handler();
			ret->moveToThread(thread());
			ret->SetTargetWindow(gameWindow);
			return std::shared_ptr<KeyboardHandlerBase>(ret);
		}
		default: fmt::throw_exception("Invalid keyboard handler: %s", type);
		}
	};

	callbacks.get_mouse_handler = [=]() -> std::shared_ptr<MouseHandlerBase>
	{
		switch (mouse_handler type = g_cfg.io.mouse)
		{
		case mouse_handler::null: return std::make_shared<NullMouseHandler>();
		case mouse_handler::basic:
		{
			basic_mouse_handler* ret = new basic_mouse_handler();
			ret->moveToThread(thread());
			ret->SetTargetWindow(gameWindow);
			return std::shared_ptr<MouseHandlerBase>(ret);
		}
		default: fmt::throw_exception("Invalid mouse handler: %s", type);
		}
	};

	callbacks.get_pad_handler = [this]() -> std::shared_ptr<pad_thread>
	{
		return std::make_shared<pad_thread>(thread(), gameWindow);
	};

	callbacks.get_gs_frame = [this]() -> std::unique_ptr<GSFrameBase>
	{
		extern const std::unordered_map<video_resolution, std::pair<int, int>, value_hash<video_resolution>> g_video_out_resolution_map;

		const auto size = g_video_out_resolution_map.at(g_cfg.video.resolution);
		int w = size.first;
		int h = size.second;

		if (guiSettings->GetValue(gui::gs_resize).toBool())
		{
			w = guiSettings->GetValue(gui::gs_width).toInt();
			h = guiSettings->GetValue(gui::gs_height).toInt();
		}

		bool disableMouse = guiSettings->GetValue(gui::gs_disableMouse).toBool();
		auto frame_geometry = gui::utils::create_centered_window_geometry(RPCS3MainWin->geometry(), w, h);

		gs_frame* frame;

		switch (video_renderer type = g_cfg.video.renderer)
		{
		case video_renderer::null:
		{
			frame = new gs_frame("Null", frame_geometry, RPCS3MainWin->GetAppIcon(), disableMouse);
			break;
		}
		case video_renderer::opengl:
		{
			frame = new gl_gs_frame(frame_geometry, RPCS3MainWin->GetAppIcon(), disableMouse);
			break;
		}
		case video_renderer::vulkan:
		{
			frame = new gs_frame("Vulkan", frame_geometry, RPCS3MainWin->GetAppIcon(), disableMouse);
			break;
		}
#ifdef _MSC_VER
		case video_renderer::dx12:
		{
			frame = new gs_frame("DirectX 12", frame_geometry, RPCS3MainWin->GetAppIcon(), disableMouse);
			break;
		}
#endif
		default:
			fmt::throw_exception("Invalid video renderer: %s" HERE, type);
		}

		gameWindow = frame;
		return std::unique_ptr<gs_frame>(frame);
	};

	callbacks.get_gs_render = []() -> std::shared_ptr<GSRender>
	{
		switch (video_renderer type = g_cfg.video.renderer)
		{
		case video_renderer::null: return std::make_shared<NullGSRender>();
		case video_renderer::opengl: return std::make_shared<GLGSRender>();
#if defined(_WIN32) || defined(HAVE_VULKAN)
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
#endif
#ifdef HAVE_ALSA
		case audio_renderer::alsa: return std::make_shared<ALSAThread>();
#endif
#ifdef HAVE_PULSE
		case audio_renderer::pulse: return std::make_shared<PulseThread>();
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

	callbacks.get_trophy_notification_dialog = [=]() -> std::unique_ptr<TrophyNotificationBase>
	{
		return std::make_unique<trophy_notification_helper>(gameWindow);
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
		auto rgba = [](const QColor& c, int v = 0)
		{
			return QString("rgba(%1, %2, %3, %4);").arg(c.red() + v).arg(c.green() + v).arg(c.blue() + v).arg(c.alpha() + v);
		};

		// toolbar color stylesheet
		QString rgba_tool_bar = rgba(gui::mw_tool_bar_color);
		QString style_toolbar = QString
		(
			"QLineEdit#mw_searchbar { margin-left:14px; background-color: " + rgba_tool_bar + " }"
			"QToolBar#mw_toolbar { background-color: " + rgba_tool_bar + " }"
			"QToolBar#mw_toolbar QSlider { background-color: " + rgba_tool_bar + " }"
			"QToolBar#mw_toolbar::separator { background-color: " + rgba(gui::mw_tool_bar_color, -20) + " width: 1px; margin-top: 2px; margin-bottom: 2px; }"
		);

		// toolbar icon color stylesheet
		QString style_toolbar_icons = QString
		(
			"QLabel#toolbar_icon_color { color: " + rgba(gui::mw_tool_icon_color) + " }"
		);

		// thumbnail icon color stylesheet
		QString style_thumbnail_icons = QString
		(
			"QLabel#thumbnail_icon_color { color: " + rgba(gui::mw_thumb_icon_color) + " }"
		);

		// gamelist toolbar stylesheet
		QString style_gamelist_toolbar = QString
		(
			"QLineEdit#tb_searchbar { background: transparent; }"
			"QLabel#gamelist_toolbar_icon_color { color: " + rgba(gui::gl_tool_icon_color) + " }"
		);

		// gamelist icon color stylesheet
		QString style_gamelist_icons = QString
		(
			"QLabel#gamelist_icon_background_color { color: " + rgba(gui::gl_icon_color) + " }"
		);

		// log stylesheet
		QString style_log = QString
		(
			"QTextEdit#tty_frame { background-color: #ffffff; }"
			"QLabel#tty_text { color: #000000; }"
			"QTextEdit#log_frame { background-color: #ffffff; }"
			"QLabel#log_level_always { color: #107896; }"
			"QLabel#log_level_fatal { color: #ff00ff; }"
			"QLabel#log_level_error { color: #C02F1D; }"
			"QLabel#log_level_todo { color: #ff6000; }"
			"QLabel#log_level_success { color: #008000; }"
			"QLabel#log_level_warning { color: #BA8745; }"
			"QLabel#log_level_notice { color: #000000; }"
			"QLabel#log_level_trace { color: #808080; }"
			"QLabel#log_stack { color: #000000; }"
		);

		// other objects' stylesheet
		QString style_rest = QString
		(
			"QWidget#header_section { background-color: #ffffff; }"
			"QDialog#kernel_explorer { background-color: rgba(240, 240, 240, 255); }"
			"QDialog#memory_viewer { background-color: rgba(240, 240, 240, 255); }"
			"QLabel#memory_viewer_address_panel { color: rgba(75, 135, 150, 255); background-color: rgba(240, 240, 240, 255); }"
			"QLabel#memory_viewer_hex_panel { color: #000000; background-color: rgba(240, 240, 240, 255); }"
			"QLabel#memory_viewer_ascii_panel { color: #000000; background-color: rgba(240, 240, 240, 255); }"
			"QLabel#debugger_frame_breakpoint { color: #000000; background-color: #ffff00; }"
			"QLabel#debugger_frame_pc { color: #000000; background-color: #00ff00; }"
			"QLabel#rsx_debugger_display_buffer { background-color: rgba(240, 240, 240, 255); }"
			"QLabel#l_controller { color: #434343; }"
			"QLabel#gamegrid_font { font-weight: 600; font-size: 8pt; font-family: Lucida Grande; color: rgba(51, 51, 51, 255); }"
		);

		setStyleSheet(style_toolbar + style_toolbar_icons + style_thumbnail_icons + style_gamelist_toolbar + style_gamelist_icons + style_log + style_rest);
	}
	else if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		setStyleSheet(file.readAll());
		file.close();
	}
	gui::stylesheet = styleSheet();
	RPCS3MainWin->RepaintGui();
}

/**
 * Using connects avoids timers being unable to be used in a non-qt thread.  So, even if this looks stupid to just call func, it's succinct.
*/
void rpcs3_app::HandleCallAfter(const std::function<void()>& func)
{
	func();
}
