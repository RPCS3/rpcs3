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
#include "stylesheets.h"

#include <QScreen>

#include <clocale>

gui_application::gui_application(int& argc, char** argv) : QApplication(argc, argv)
{
}

gui_application::~gui_application()
{
#ifdef WITH_DISCORD_RPC
	discord::shutdown();
#endif
}

void gui_application::Init()
{
	setWindowIcon(QIcon(":/rpcs3.ico"));

	m_emu_settings.reset(new emu_settings());
	m_gui_settings.reset(new gui_settings());

	// Force init the emulator
	InitializeEmulator(m_gui_settings->GetCurrentUser().toStdString(), true);

	// Create the main window
	if (m_show_gui)
	{
		m_main_window = new main_window(m_gui_settings, m_emu_settings, nullptr);
	}

	// Create callbacks from the emulator, which reference the handlers.
	InitializeCallbacks();

	// Create connects to propagate events throughout Gui.
	InitializeConnects();

	// As per QT recommendations to avoid conflicts for POSIX functions
	std::setlocale(LC_NUMERIC, "C");

	if (m_main_window)
	{
		m_main_window->Init();
	}

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
	connect(this, &gui_application::OnEmulatorRun, this, &gui_application::StartPlaytime);
	connect(this, &gui_application::OnEmulatorStop, this, &gui_application::StopPlaytime);
	connect(this, &gui_application::OnEmulatorPause, this, &gui_application::StopPlaytime);
	connect(this, &gui_application::OnEmulatorResume, this, &gui_application::StartPlaytime);

	if (m_main_window)
	{
		connect(m_main_window, &main_window::RequestGlobalStylesheetChange, this, &gui_application::OnChangeStyleSheetRequest);

		connect(this, &gui_application::OnEmulatorRun, m_main_window, &main_window::OnEmuRun);
		connect(this, &gui_application::OnEmulatorStop, m_main_window, &main_window::OnEmuStop);
		connect(this, &gui_application::OnEmulatorPause, m_main_window, &main_window::OnEmuPause);
		connect(this, &gui_application::OnEmulatorResume, m_main_window, &main_window::OnEmuResume);
		connect(this, &gui_application::OnEmulatorReady, m_main_window, &main_window::OnEmuReady);
	}

#ifdef WITH_DISCORD_RPC
	connect(this, &gui_application::OnEmulatorRun, [this]()
	{
		// Discord Rich Presence Integration
		if (m_gui_settings->GetValue(gui::m_richPresence).toBool())
		{
			discord::update_presence(Emu.GetTitleID(), Emu.GetTitle());
		}
	});
	connect(this, &gui_application::OnEmulatorStop, [this]()
	{
		// Discord Rich Presence Integration
		if (m_gui_settings->GetValue(gui::m_richPresence).toBool())
		{
			discord::update_presence(m_gui_settings->GetValue(gui::m_discordState).toString().toStdString());
		}
	});
#endif

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

	const auto screen_geometry = m_main_window ? m_main_window->geometry() : primaryScreen()->geometry();
	const auto frame_geometry  = gui::utils::create_centered_window_geometry(screen_geometry, w, h);
	const auto app_icon = m_main_window ? m_main_window->GetAppIcon() : gui::utils::get_app_icon_from_path(Emu.GetBoot(), Emu.GetTitleID());

	gs_frame* frame;

	switch (video_renderer type = g_cfg.video.renderer)
	{
	case video_renderer::null:
	{
		frame = new gs_frame("Null", frame_geometry, app_icon, m_gui_settings);
		break;
	}
	case video_renderer::opengl:
	{
		frame = new gl_gs_frame(frame_geometry, app_icon, m_gui_settings);
		break;
	}
	case video_renderer::vulkan:
	{
		frame = new gs_frame("Vulkan", frame_geometry, app_icon, m_gui_settings);
		break;
	}
	default: fmt::throw_exception("Invalid video renderer: %s" HERE, type);
	}

	m_game_window = frame;

	return std::unique_ptr<gs_frame>(frame);
}

/** RPCS3 emulator has functions it desires to call from the GUI at times. Initialize them in here. */
void gui_application::InitializeCallbacks()
{
	EmuCallbacks callbacks = CreateCallbacks();

	callbacks.exit = [this](bool force_quit)
	{
		// Close rpcs3 if closed in no-gui mode
		if (force_quit || !m_main_window)
		{
			quit();
		}
	};
	callbacks.call_after = [=](std::function<void()> func)
	{
		RequestCallAfter(std::move(func));
	};

	callbacks.get_gs_frame    = [this]() -> std::unique_ptr<GSFrameBase> { return get_gs_frame(); };
	callbacks.get_msg_dialog  = [this]() -> std::shared_ptr<MsgDialogBase> { return std::make_shared<msg_dialog_frame>(); };
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

void gui_application::StartPlaytime()
{
	const QString serial = qstr(Emu.GetTitleID());
	m_gui_settings->SetLastPlayed(serial, QDate::currentDate().toString("MMMM d yyyy"));
	m_timer_playtime.start();
}

void gui_application::StopPlaytime()
{
	if (!m_timer_playtime.isValid())
		return;

	const QString serial = qstr(Emu.GetTitleID());
	const qint64 playtime = m_gui_settings->GetPlaytime(serial) + m_timer_playtime.elapsed();
	m_gui_settings->SetPlaytime(serial, playtime);
	m_gui_settings->SetLastPlayed(serial, QDate::currentDate().toString("MMMM d yyyy"));
	m_timer_playtime.invalidate();
}

/*
* Handle a request to change the stylesheet. May consider adding reporting of errors in future.
* Empty string means default.
*/
void gui_application::OnChangeStyleSheetRequest(const QString& path)
{
	// skip stylesheets on first repaint if a style was set from command line
	if (m_use_cli_style && gui::stylesheet.isEmpty())
	{
		gui::stylesheet = styleSheet().isEmpty() ? "/* style set by command line arg */" : styleSheet();

		if (m_main_window)
		{
			m_main_window->RepaintGui();
		}

		return;
	}

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
		setStyleSheet(gui::stylesheets::default_style_sheet);
	}
	else if (path == "-")
	{
		setStyleSheet("/* none */");
	}
	else if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QString config_dir = qstr(fs::get_config_dir());

		// Remove old fonts
		QFontDatabase::removeAllApplicationFonts();

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
		setStyleSheet(gui::stylesheets::default_style_sheet);
	}

	gui::stylesheet = styleSheet();

	if (m_main_window)
	{
		m_main_window->RepaintGui();
	}
}

/**
 * Using connects avoids timers being unable to be used in a non-qt thread. So, even if this looks stupid to just call func, it's succinct.
 */
void gui_application::HandleCallAfter(const std::function<void()>& func)
{
	func();
}
