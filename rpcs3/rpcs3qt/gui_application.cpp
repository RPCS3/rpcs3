#include "gui_application.h"

#include "qt_utils.h"
#include "welcome_dialog.h"

#ifdef WITH_DISCORD_RPC
#include "_discord_utils.h"
#endif

#include "trophy_notification_helper.h"
#include "save_data_dialog.h"
#include "msg_dialog_frame.h"
#include "osk_dialog_frame.h"

gui_application::gui_application(int& argc, char** argv) : QApplication(argc, argv)
{
}

void gui_application::Init()
{
	setWindowIcon(QIcon(":/rpcs3.ico"));

	m_emu_settings.reset(new emu_settings());
	m_gui_settings.reset(new gui_settings());

	// Force init the emulator
	InitializeEmulator(m_gui_settings->GetCurrentUser().toStdString(), true);

	// Create the main window
	m_main_window = new main_window(m_gui_settings, m_emu_settings, nullptr);

	// Create callbacks from the emulator, which reference the handlers.
	InitializeCallbacks();

	// Create connects to propagate events throughout Gui.
	InitializeConnects();

	m_main_window->Init();

	if (m_gui_settings->GetValue(gui::ib_show_welcome).toBool())
	{
		welcome_dialog* welcome = new welcome_dialog();
		welcome->exec();
	}
#ifdef WITH_DISCORD_RPC
	// Discord Rich Presence Integration
	if (m_gui_settings->GetValue(gui::m_richPresence).toBool())
	{
		discord::initialize();
	}
#endif
}

void gui_application::InitializeConnects()
{
	connect(m_main_window, &main_window::RequestGlobalStylesheetChange, this, &gui_application::OnChangeStyleSheetRequest);

	connect(this, &gui_application::OnEmulatorRun, m_main_window, &main_window::OnEmuRun);
	connect(this, &gui_application::OnEmulatorStop, m_main_window, &main_window::OnEmuStop);
	connect(this, &gui_application::OnEmulatorPause, m_main_window, &main_window::OnEmuPause);
	connect(this, &gui_application::OnEmulatorResume, m_main_window, &main_window::OnEmuResume);
	connect(this, &gui_application::OnEmulatorReady, m_main_window, &main_window::OnEmuReady);

	qRegisterMetaType<std::function<void()>>("std::function<void()>");
	connect(this, &gui_application::RequestCallAfter, this, &gui_application::HandleCallAfter);
}

std::unique_ptr<gs_frame> gui_application::get_gs_frame()
{
	extern const std::unordered_map<video_resolution, std::pair<int, int>, value_hash<video_resolution>> g_video_out_resolution_map;

	const auto size = g_video_out_resolution_map.at(g_cfg.video.resolution);
	int w           = size.first;
	int h           = size.second;

	if (m_gui_settings->GetValue(gui::gs_resize).toBool())
	{
		w = m_gui_settings->GetValue(gui::gs_width).toInt();
		h = m_gui_settings->GetValue(gui::gs_height).toInt();
	}

	auto frame_geometry = gui::utils::create_centered_window_geometry(m_main_window->geometry(), w, h);

	gs_frame* frame;

	switch (video_renderer type = g_cfg.video.renderer)
	{
	case video_renderer::null:
	{
		frame = new gs_frame("Null", frame_geometry, m_main_window->GetAppIcon(), m_gui_settings);
		break;
	}
	case video_renderer::opengl:
	{
		frame = new gl_gs_frame(frame_geometry, m_main_window->GetAppIcon(), m_gui_settings);
		break;
	}
	case video_renderer::vulkan:
	{
		frame = new gs_frame("Vulkan", frame_geometry, m_main_window->GetAppIcon(), m_gui_settings);
		break;
	}
#ifdef _MSC_VER
	case video_renderer::dx12:
	{
		frame = new gs_frame("DirectX 12", frame_geometry, m_main_window->GetAppIcon(), m_gui_settings);
		break;
	}
#endif
	default: fmt::throw_exception("Invalid video renderer: %s" HERE, type);
	}

	m_game_window = frame;
	return std::unique_ptr<gs_frame>(frame);
}

/** RPCS3 emulator has functions it desires to call from the GUI at times. Initialize them in here. */
void gui_application::InitializeCallbacks()
{
	EmuCallbacks callbacks = CreateCallbacks();

	callbacks.exit = [this]()
	{
		quit();
	};
	callbacks.call_after = [=](std::function<void()> func)
	{
		RequestCallAfter(std::move(func));
	};

	callbacks.get_gs_frame    = [this]() -> std::unique_ptr<GSFrameBase> { return get_gs_frame(); };
	callbacks.get_msg_dialog  = [this]() -> std::shared_ptr<MsgDialogBase> { return std::make_shared<msg_dialog_frame>(m_main_window->windowHandle()); };
	callbacks.get_osk_dialog  = []() -> std::shared_ptr<OskDialogBase> { return std::make_shared<osk_dialog_frame>(); };
	callbacks.get_save_dialog = []() -> std::unique_ptr<SaveDialogBase> { return std::make_unique<save_data_dialog>(); };
	callbacks.get_trophy_notification_dialog = [this]() -> std::unique_ptr<TrophyNotificationBase> { return std::make_unique<trophy_notification_helper>(m_game_window); };

	callbacks.on_run    = [=]() { OnEmulatorRun(); };
	callbacks.on_pause  = [=]() { OnEmulatorPause(); };
	callbacks.on_resume = [=]() { OnEmulatorResume(); };
	callbacks.on_stop   = [=]() { OnEmulatorStop(); };
	callbacks.on_ready  = [=]() { OnEmulatorReady(); };

	callbacks.handle_taskbar_progress = [=](s32 type, s32 value)
	{
		if (m_game_window)
		{
			switch (type)
			{
			case 0: ((gs_frame*)m_game_window)->progress_reset(value); break;
			case 1: ((gs_frame*)m_game_window)->progress_increment(value); break;
			case 2: ((gs_frame*)m_game_window)->progress_set_limit(value); break;
			default: LOG_FATAL(GENERAL, "Unknown type in handle_taskbar_progress(type=%d, value=%d)", type, value); break;
			}
		}
	};

	Emu.SetCallbacks(std::move(callbacks));
}

/*
* Handle a request to change the stylesheet. May consider adding reporting of errors in future.
* Empty string means default.
*/
void gui_application::OnChangeStyleSheetRequest(const QString& path)
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
		"QLabel#gamelist_icon_background_color { color: rgba(240, 240, 240, 255); }"

		// save manager icon color
		"QLabel#save_manager_icon_background_color { color: rgba(240, 240, 240, 255); }"

		// trophy manager icon color
		"QLabel#trophy_manager_icon_background_color { color: rgba(240, 240, 240, 255); }"

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
	m_main_window->RepaintGui();
}

/**
 * Using connects avoids timers being unable to be used in a non-qt thread. So, even if this looks stupid to just call func, it's succinct.
 */
void gui_application::HandleCallAfter(const std::function<void()>& func)
{
	func();
}
