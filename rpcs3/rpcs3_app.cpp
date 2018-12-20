#include "rpcs3_app.h"

#include "rpcs3qt/qt_utils.h"

#include "rpcs3qt/welcome_dialog.h"

#ifdef WITH_DISCORD_RPC
#include "rpcs3qt/_discord_utils.h"
#endif

#include "Emu/System.h"
#include "rpcs3qt/gs_frame.h"
#include "rpcs3qt/gl_gs_frame.h"

#include "rpcs3qt/trophy_notification_helper.h"
#include "rpcs3qt/save_data_dialog.h"

#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "basic_keyboard_handler.h"

#include "Emu/Io/Null/NullMouseHandler.h"
#include "basic_mouse_handler.h"

#include "Emu/Io/Null/NullPadHandler.h"
#include "keyboard_pad_handler.h"
#include "ds4_pad_handler.h"
#ifdef _WIN32
#include "xinput_pad_handler.h"
#include "mm_joystick_handler.h"
#endif
#ifdef HAVE_LIBEVDEV
#include "evdev_joystick_handler.h"
#endif

#include "pad_thread.h"

#include "Emu/RSX/Null/NullGSRender.h"
#include "Emu/RSX/GL/GLGSRender.h"
#include "Emu/Audio/Null/NullAudioThread.h"
//#include "Emu/Audio/AL/OpenALThread.h"
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

#ifdef _WIN32
#include "Utilities/dynamic_library.h"
DYNAMIC_IMPORT("ntdll.dll", NtQueryTimerResolution, NTSTATUS(PULONG MinimumResolution, PULONG MaximumResolution, PULONG CurrentResolution));
DYNAMIC_IMPORT("ntdll.dll", NtSetTimerResolution, NTSTATUS(ULONG DesiredResolution, BOOLEAN SetResolution, PULONG CurrentResolution));
#endif

// For now, a trivial constructor/destructor.  May add command line usage later.
rpcs3_app::rpcs3_app(int& argc, char** argv) : QApplication(argc, argv)
{
}

void rpcs3_app::Init()
{
	setApplicationName("RPCS3");
	setWindowIcon(QIcon(":/rpcs3.ico"));

	guiSettings.reset(new gui_settings());
	emuSettings.reset(new emu_settings());

	// Force init the emulator
	InitializeEmulator(guiSettings->GetCurrentUser().toStdString(), true);

	// Create the main window
	RPCS3MainWin = new main_window(guiSettings, emuSettings, nullptr);

	// Create callbacks from the emulator, which reference the handlers.
	InitializeCallbacks();

	// Create connects to propagate events throughout Gui.
	InitializeConnects();

	RPCS3MainWin->Init();

	if (guiSettings->GetValue(gui::ib_show_welcome).toBool())
	{
		welcome_dialog* welcome = new welcome_dialog();
		welcome->exec();
	}
#ifdef WITH_DISCORD_RPC
	// Discord Rich Presence Integration
	if (guiSettings->GetValue(gui::m_richPresence).toBool())
	{
		discord::initialize();
	}
#endif

#ifdef _WIN32
	// Set 0.5 msec timer resolution for best performance
	// - As QT5 timers (QTimer) sets the timer resolution to 1 msec, override it here.
	// - Don't bother "unsetting" the timer resolution after the emulator stops as QT5 will still require the timer resolution to be set to 1 msec.
	ULONG min_res, max_res, orig_res, new_res;
	if (NtQueryTimerResolution(&min_res, &max_res, &orig_res) == 0)
	{
		NtSetTimerResolution(max_res, TRUE, &new_res);
	}
#endif
}

/** Emu.Init() wrapper for user manager */
bool rpcs3_app::InitializeEmulator(const std::string& user, bool force_init)
{
	// try to set a new user
	const bool user_was_set = Emu.SetUsr(user);

	// only init the emulation if forced or a user was set
	if (user_was_set || force_init)
	{
		Emu.Init();
	}

	return user_was_set;
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

	callbacks.reset_pads = [this]()
	{
		pad::get_current_handler()->Reset();
	};
	callbacks.enable_pads = [this](bool enable)
	{
		pad::get_current_handler()->SetEnabled(enable);
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

		auto frame_geometry = gui::utils::create_centered_window_geometry(RPCS3MainWin->geometry(), w, h);

		gs_frame* frame;

		switch (video_renderer type = g_cfg.video.renderer)
		{
		case video_renderer::null:
		{
			frame = new gs_frame("Null", frame_geometry, RPCS3MainWin->GetAppIcon(), guiSettings);
			break;
		}
		case video_renderer::opengl:
		{
			frame = new gl_gs_frame(frame_geometry, RPCS3MainWin->GetAppIcon(), guiSettings);
			break;
		}
		case video_renderer::vulkan:
		{
			frame = new gs_frame("Vulkan", frame_geometry, RPCS3MainWin->GetAppIcon(), guiSettings);
			break;
		}
#ifdef _MSC_VER
		case video_renderer::dx12:
		{
			frame = new gs_frame("DirectX 12", frame_geometry, RPCS3MainWin->GetAppIcon(), guiSettings);
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
		case video_renderer::null: return std::make_shared<named_thread<NullGSRender>>("rsx::thread");
		case video_renderer::opengl: return std::make_shared<named_thread<GLGSRender>>("rsx::thread");
#if defined(_WIN32) || defined(HAVE_VULKAN)
		case video_renderer::vulkan: return std::make_shared<named_thread<VKGSRender>>("rsx::thread");
#endif
#ifdef _MSC_VER
		case video_renderer::dx12: return std::make_shared<named_thread<D3D12GSRender>>("rsx::thread");
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

		//case audio_renderer::openal: return std::make_shared<OpenALThread>();
		default: fmt::throw_exception("Invalid audio renderer: %s" HERE, type);
		}
	};

	callbacks.get_msg_dialog = [=]() -> std::shared_ptr<MsgDialogBase>
	{
		return std::make_shared<msg_dialog_frame>(RPCS3MainWin->windowHandle());
	};

	callbacks.get_osk_dialog = [=]() -> std::shared_ptr<OskDialogBase>
	{
		return std::make_shared<osk_dialog_frame>();
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

	callbacks.handle_taskbar_progress = [=](s32 type, s32 value)
	{
		if (gameWindow)
		{
			switch (type)
			{
			case 0:
				((gs_frame*)gameWindow)->progress_reset(value);
				break;
			case 1:
				((gs_frame*)gameWindow)->progress_increment(value);
				break;
			case 2:
				((gs_frame*)gameWindow)->progress_set_limit(value);
				break;
			default:
				LOG_FATAL(GENERAL, "Unknown type in handle_taskbar_progress(type=%d, value=%d)", type, value);
				break;
			}
		}
	};

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
void rpcs3_app::OnChangeStyleSheetRequest(const QString& path)
{
	QString style_sheet
	(
		// main window toolbar search
		"QLineEdit#mw_searchbar { padding: 0 1em; background: #fdfdfd; selection-background-color: #148aff; margin: .8em; color:#000000; }"

		// main window toolbar slider
		"QSlider#sizeSlider { color: #505050; background: #F0F0F0; }"
		"QSlider#sizeSlider::handle:horizontal { border: 0em smooth rgba(227, 227, 227, 255); border-radius: .58em; background: #404040; width: 1.2em; margin: -.5em 0; }"
		"QSlider#sizeSlider::groove:horizontal { border-radius: .15em; background: #5b5b5b; height: .3em; }"

		// main window toolbar
		"QToolBar#mw_toolbar { background-color: #F0F0F0; border: none; }"
		"QToolBar#mw_toolbar::separator { background-color: rgba(207, 207, 207, 235); width: 0.125em; margin-top: 0.250em; margin-bottom: 0.250em; }"

		// main window toolbar icon color
		"QLabel#toolbar_icon_color { color: #5b5b5b; }"

		// thumbnail icon color
		"QLabel#thumbnail_icon_color { color: rgba(0, 100, 231, 255); }"

		// game list icon color
		"QLabel#gamelist_icon_background_color { color: rgba(36, 36, 36, 255); }"

		// tables
		"QTableWidget { alternate-background-color: #f2f2f2; background-color: #fff; border: none; }"
		"QTableWidget#game_grid { alternate-background-color: #f2f2f2; background-color: #fff; font-weight: 600; font-size: 8pt; font-family: Lucida Grande; color: rgba(51, 51, 51, 255); border: 0em solid white; }"
		"QTableView::item { border-left: 0.063em solid white; border-right: 0.063em solid white; padding-left:0.313em; }"
		"QTableView::item:selected { background-color: #148aff; color: #fff; }"
		"QTableView#game_grid::item:hover:!selected { background-color: #94c9ff; color: #fff; }"
		"QTableView#game_grid::item:hover:selected { background-color: #007fff; color: #fff; }"

		// table headers
#if (QT_VERSION < QT_VERSION_CHECK(5,11,0))
		"QHeaderView::section { padding: .5em; border: 0.063em solid #ffffff; }"
		"QHeaderView::section:hover { background: #e3e3e3; padding: .5em; border: 0.063em solid #ffffff; }"
#else
		"QHeaderView::section { padding-left: .5em; padding-right: .5em; padding-top: .4em; padding-bottom: -.1em; border: 0.063em solid #ffffff; }"
		"QHeaderView::section:hover { background: #e3e3e3; padding-left: .5em; padding-right: .5em; padding-top: .4em; padding-bottom: -.1em; border: 0.063em solid #ffffff; }"
#endif

		// dock widget
		"QDockWidget{ background: transparent; color: black; }"
		"[floating=\"true\"]{ background: white; }"
		"QDockWidget::title{ background: #e3e3e3; border: none; padding-top: 0.2em; padding-left: 0.2em; }"
		"QDockWidget::close-button, QDockWidget::float-button{ background-color: #e3e3e3; }"

		// log frame tty
		"QTextEdit#tty_frame { background-color: #ffffff; }"
		"QLabel#tty_text { color: #000000; }"

		// log frame log
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

		// about dialog
		"QWidget#header_section { background-color: #ffffff; }"

		// kernel explorer
		"QDialog#kernel_explorer { background-color: rgba(240, 240, 240, 255); }"

		// memory viewer
		"QDialog#memory_viewer { background-color: rgba(240, 240, 240, 255); }"
		"QLabel#memory_viewer_address_panel { color: rgba(75, 135, 150, 255); background-color: rgba(240, 240, 240, 255); }"
		"QLabel#memory_viewer_hex_panel { color: #000000; background-color: rgba(240, 240, 240, 255); }"
		"QLabel#memory_viewer_ascii_panel { color: #000000; background-color: rgba(240, 240, 240, 255); }"

		// debugger frame
		"QLabel#debugger_frame_breakpoint { color: #000000; background-color: #ffff00; }"
		"QLabel#debugger_frame_pc { color: #000000; background-color: #00ff00; }"

		// rsx debugger
		"QLabel#rsx_debugger_display_buffer { background-color: rgba(240, 240, 240, 255); }"

		// pad settings
		"QLabel#l_controller { color: #434343; }"
	);

	QFile file(path);

	// If we can't open the file, try the /share or /Resources folder
#if !defined(_WIN32)
#ifdef __APPLE__
	QString share_dir = QCoreApplication::applicationDirPath() + "/../Resources/";
#else
	QString share_dir = QCoreApplication::applicationDirPath() + "/../share/rpcs3/";
#endif
	QFile share_file(share_dir + "GuiConfigs/" + QFileInfo(file.fileName()).fileName());
#endif

	if (path == "")
	{
		setStyleSheet(style_sheet);
	}
	else if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QString config_dir = qstr(fs::get_config_dir());

		// Add PS3 fonts
		QDirIterator ps3_font_it(qstr(g_cfg.vfs.get_dev_flash() + "data/font/"), QStringList() << "*.ttf", QDir::Files, QDirIterator::Subdirectories);
		while (ps3_font_it.hasNext())
			QFontDatabase::addApplicationFont(ps3_font_it.next());

		// Add custom fonts
		QDirIterator custom_font_it(config_dir + "fonts/", QStringList() << "*.ttf", QDir::Files, QDirIterator::Subdirectories);
		while (custom_font_it.hasNext())
			QFontDatabase::addApplicationFont(custom_font_it.next());

		// Set root for stylesheets
		QDir::setCurrent(config_dir);
		setStyleSheet(file.readAll());
		file.close();
	}
#if !defined(_WIN32)
	else if (share_file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QDir::setCurrent(share_dir);
		setStyleSheet(share_file.readAll());
		share_file.close();
	}
#endif
	else
	{
		setStyleSheet(style_sheet);
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
