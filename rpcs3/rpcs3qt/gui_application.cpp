#include "gui_application.h"

#include "qt_utils.h"
#include "welcome_dialog.h"
#include "main_window.h"
#include "emu_settings.h"
#include "gui_settings.h"
#include "persistent_settings.h"
#include "gs_frame.h"
#include "gl_gs_frame.h"
#include "localized_emu.h"
#include "qt_camera_handler.h"
#include "qt_music_handler.h"
#include "rpcs3_version.h"

#ifdef WITH_DISCORD_RPC
#include "_discord_utils.h"
#endif

#include "Emu/Io/Null/null_camera_handler.h"
#include "Emu/Io/Null/null_music_handler.h"
#include "Emu/vfs_config.h"
#include "trophy_notification_helper.h"
#include "save_data_dialog.h"
#include "msg_dialog_frame.h"
#include "osk_dialog_frame.h"
#include "recvmessage_dialog_frame.h"
#include "sendmessage_dialog_frame.h"
#include "stylesheets.h"

#include <QScreen>
#include <QFontDatabase>
#include <QLayout>
#include <QLibraryInfo>
#include <QDirIterator>
#include <QFileInfo>
#include <QSound>
#include <QMessageBox>
#include <QTextDocument>

#include <clocale>

#include "Emu/RSX/Null/NullGSRender.h"
#include "Emu/RSX/GL/GLGSRender.h"

#if defined(HAVE_VULKAN)
#include "Emu/RSX/VK/VKGSRender.h"
#endif

LOG_CHANNEL(gui_log, "GUI");

gui_application::gui_application(int& argc, char** argv) : QApplication(argc, argv)
{
}

gui_application::~gui_application()
{
#ifdef WITH_DISCORD_RPC
	discord::shutdown();
#endif
}

bool gui_application::Init()
{
#ifndef __APPLE__
	setWindowIcon(QIcon(":/rpcs3.ico"));
#endif

	if (!rpcs3::is_release_build() && !rpcs3::is_local_build())
	{
		const std::string_view branch_name = rpcs3::get_full_branch();
		gui_log.warning("Experimental Build Warning! Build origin: %s", branch_name);

		QMessageBox msg;
		msg.setWindowTitle(tr("Experimental Build Warning"));
		msg.setIcon(QMessageBox::Critical);
		msg.setTextFormat(Qt::RichText);
		msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msg.setDefaultButton(QMessageBox::No);
		msg.setText(tr(
			R"(
				<p style="white-space: nowrap;">
					Please understand that this build is not an official RPCS3 release.<br>
					This build contains changes that may break games, or even <b>damage</b> your data.<br>
					We recommend to download and use the official build from the <a %0 href='https://rpcs3.net/download'>RPCS3 website</a>.<br><br>
					Build origin: %1<br>
					Do you wish to use this build anyway?
				</p>
			)"
		).arg(gui::utils::get_link_style()).arg(Qt::convertFromPlainText(branch_name.data())));
		msg.layout()->setSizeConstraint(QLayout::SetFixedSize);

		if (msg.exec() == QMessageBox::No)
		{
			return false;
		}
	}

	m_emu_settings.reset(new emu_settings());
	m_gui_settings.reset(new gui_settings());
	m_persistent_settings.reset(new persistent_settings());

	if (!m_emu_settings->Init())
	{
		return false;
	}

	// The user might be set by cli arg. If not, set another user.
	if (m_active_user.empty())
	{
		// Get active user with standard user as fallback
		m_active_user = m_persistent_settings->GetCurrentUser("00000001").toStdString();
	}

	// Force init the emulator
	InitializeEmulator(m_active_user, m_show_gui);

	// Create the main window
	if (m_show_gui)
	{
		m_main_window = new main_window(m_gui_settings, m_emu_settings, m_persistent_settings, nullptr);

		const auto codes    = GetAvailableLanguageCodes();
		const auto language = m_gui_settings->GetValue(gui::loc_language).toString();
		const auto index    = codes.indexOf(language);

		LoadLanguage(index < 0 ? QLocale(QLocale::English).bcp47Name() : ::at32(codes, index));
	}

	// Create callbacks from the emulator, which reference the handlers.
	InitializeCallbacks();

	// Create connects to propagate events throughout Gui.
	InitializeConnects();

	if (m_gui_settings->GetValue(gui::ib_show_welcome).toBool())
	{
		welcome_dialog* welcome = new welcome_dialog(m_gui_settings);
		welcome->exec();
	}

	// Check maxfiles
	if (utils::get_maxfiles() < 4096)
	{
		QMessageBox::warning(nullptr,
							 tr("Warning"),
							 tr("The current limit of maximum file descriptors is too low.\n"
								"Some games will crash.\n"
								"\n"
								"Please increase the limit before running RPCS3."));
	}

	if (m_main_window && !m_main_window->Init(m_with_cli_boot))
	{
		return false;
	}

#ifdef WITH_DISCORD_RPC
	// Discord Rich Presence Integration
	if (m_gui_settings->GetValue(gui::m_richPresence).toBool())
	{
		discord::initialize();
	}
#endif

	return true;
}

void gui_application::SwitchTranslator(QTranslator& translator, const QString& filename, const QString& language_code)
{
	// remove the old translator
	removeTranslator(&translator);

	const QString lang_path = QLibraryInfo::location(QLibraryInfo::TranslationsPath) + QStringLiteral("/");
	const QString file_path = lang_path + filename;

	if (QFileInfo(file_path).isFile())
	{
		// load the new translator
		if (translator.load(file_path))
		{
			installTranslator(&translator);
		}
	}
	else if (const QString default_code = QLocale(QLocale::English).bcp47Name(); language_code != default_code)
	{
		// show error, but ignore default case "en", since it is handled in source code
		gui_log.error("No translation file found in: %s", file_path.toStdString());

		// reset current language to default "en"
		m_language_code = default_code;
	}
}

void gui_application::LoadLanguage(const QString& language_code)
{
	if (m_language_code == language_code)
	{
		return;
	}

	m_language_code = language_code;

	const QLocale locale      = QLocale(language_code);
	const QString locale_name = QLocale::languageToString(locale.language());

	QLocale::setDefault(locale);

	// Idk if this is overruled by the QLocale default, so I'll change it here just to be sure.
	// As per QT recommendations to avoid conflicts for POSIX functions
	std::setlocale(LC_NUMERIC, "C");

	SwitchTranslator(m_translator, QStringLiteral("rpcs3_%1.qm").arg(language_code), language_code);

	if (m_main_window)
	{
		const QString default_code = QLocale(QLocale::English).bcp47Name();
		QStringList language_codes = GetAvailableLanguageCodes();

		if (!language_codes.contains(default_code))
		{
			language_codes.prepend(default_code);
		}

		m_main_window->RetranslateUI(language_codes, m_language_code);
	}

	m_gui_settings->SetValue(gui::loc_language, m_language_code);

	gui_log.notice("Current language changed to %s (%s)", locale_name.toStdString(), language_code.toStdString());
}

QStringList gui_application::GetAvailableLanguageCodes()
{
	QStringList language_codes;

	const QString language_path = QLibraryInfo::location(QLibraryInfo::TranslationsPath);

	if (QFileInfo(language_path).isDir())
	{
		const QDir dir(language_path);
		const QStringList filenames = dir.entryList(QStringList("rpcs3_*.qm"));

		for (const QString& filename : filenames)
		{
			QString language_code = filename;                        // "rpcs3_en.qm"
			language_code.truncate(language_code.lastIndexOf('.'));  // "rpcs3_en"
			language_code.remove(0, language_code.indexOf('_') + 1); // "en"

			if (language_codes.contains(language_code))
			{
				gui_log.error("Found duplicate language '%s' (%s)", language_code.toStdString(), filename.toStdString());
			}
			else
			{
				language_codes << language_code;
			}
		}
	}

	return language_codes;
}

void gui_application::InitializeConnects()
{
	connect(&m_timer, &QTimer::timeout, this, &gui_application::UpdatePlaytime);
	connect(this, &gui_application::OnEmulatorRun, this, &gui_application::StartPlaytime);
	connect(this, &gui_application::OnEmulatorStop, this, &gui_application::StopPlaytime);
	connect(this, &gui_application::OnEmulatorPause, this, &gui_application::StopPlaytime);
	connect(this, &gui_application::OnEmulatorResume, this, &gui_application::StartPlaytime);

	if (m_main_window)
	{
		connect(m_main_window, &main_window::RequestLanguageChange, this, &gui_application::LoadLanguage);
		connect(m_main_window, &main_window::RequestGlobalStylesheetChange, this, &gui_application::OnChangeStyleSheetRequest);
		connect(m_main_window, &main_window::NotifyEmuSettingsChange, this, [this](){ OnEmuSettingsChange(); });

		connect(this, &gui_application::OnEmulatorRun, m_main_window, &main_window::OnEmuRun);
		connect(this, &gui_application::OnEmulatorStop, m_main_window, &main_window::OnEmuStop);
		connect(this, &gui_application::OnEmulatorPause, m_main_window, &main_window::OnEmuPause);
		connect(this, &gui_application::OnEmulatorResume, m_main_window, &main_window::OnEmuResume);
		connect(this, &gui_application::OnEmulatorReady, m_main_window, &main_window::OnEmuReady);
		connect(this, &gui_application::OnEnableDiscEject, m_main_window, &main_window::OnEnableDiscEject);
		connect(this, &gui_application::OnEnableDiscInsert, m_main_window, &main_window::OnEnableDiscInsert);
	}

#ifdef WITH_DISCORD_RPC
	connect(this, &gui_application::OnEmulatorRun, [this](bool /*start_playtime*/)
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
	connect(this, &gui_application::RequestCallFromMainThread, this, &gui_application::CallFromMainThread);
}

std::unique_ptr<gs_frame> gui_application::get_gs_frame()
{
	extern const std::unordered_map<video_resolution, std::pair<int, int>, value_hash<video_resolution>> g_video_out_resolution_map;

	auto [w, h] = ::at32(g_video_out_resolution_map, g_cfg.video.resolution);

	if (m_gui_settings->GetValue(gui::gs_resize).toBool())
	{
		if (m_gui_settings->GetValue(gui::gs_resize_manual).toBool())
		{
			w = m_gui_settings->GetValue(gui::gs_width).toInt();
			h = m_gui_settings->GetValue(gui::gs_height).toInt();
		}
		else
		{
			const qreal device_pixel_ratio = devicePixelRatio();
			w /= device_pixel_ratio;
			h /= device_pixel_ratio;
		}
	}

	QScreen* screen = nullptr;
	QRect base_geometry{};

	// Use screen index set by CLI argument
	int screen_index = m_game_screen_index;

	// In no-gui mode: use last used screen if no CLI index was set
	if (screen_index < 0 && !m_main_window)
	{
		screen_index = m_gui_settings->GetValue(gui::gs_screen).toInt();
	}

	// Try to find the specified screen
	if (screen_index >= 0)
	{
		const QList<QScreen*> available_screens = screens();

		if (screen_index < available_screens.count())
		{
			screen = ::at32(available_screens, screen_index);

			if (screen)
			{
				base_geometry = screen->geometry();
			}
		}

		if (!screen)
		{
			gui_log.error("The selected game screen with index %d is not available (available screens: %d)", screen_index, available_screens.count());
		}
	}

	// Fallback to the screen of the main window. Use the primary screen as last resort.
	if (!screen)
	{
		screen = m_main_window ? m_main_window->screen() : primaryScreen();
		base_geometry = m_main_window ? m_main_window->frameGeometry() : primaryScreen()->geometry();
	}

	const QRect frame_geometry = gui::utils::create_centered_window_geometry(screen, base_geometry, w, h);
	const QIcon app_icon = m_main_window ? m_main_window->GetAppIcon() : gui::utils::get_app_icon_from_path(Emu.GetBoot(), Emu.GetTitleID());

	gs_frame* frame = nullptr;

	switch (g_cfg.video.renderer.get())
	{
	case video_renderer::opengl:
	{
		frame = new gl_gs_frame(screen, frame_geometry, app_icon, m_gui_settings, m_start_games_fullscreen);
		break;
	}
	case video_renderer::null:
	case video_renderer::vulkan:
	{
		frame = new gs_frame(screen, frame_geometry, app_icon, m_gui_settings, m_start_games_fullscreen);
		break;
	}
	}

	m_game_window = frame;

	return std::unique_ptr<gs_frame>(frame);
}

/** RPCS3 emulator has functions it desires to call from the GUI at times. Initialize them in here. */
void gui_application::InitializeCallbacks()
{
	EmuCallbacks callbacks = CreateCallbacks();

	callbacks.try_to_quit = [this](bool force_quit, std::function<void()> on_exit) -> bool
	{
		// Close rpcs3 if closed in no-gui mode
		if (force_quit || !m_main_window)
		{
			if (on_exit)
			{
				on_exit();
			}

			if (m_main_window)
			{
				// Close main window in order to save its window state
				m_main_window->close();
			}
			quit();
			return true;
		}

		return false;
	};
	callbacks.call_from_main_thread = [this](std::function<void()> func, atomic_t<bool>* wake_up)
	{
		RequestCallFromMainThread(std::move(func), wake_up);
	};

	callbacks.init_gs_render = [](utils::serial* ar)
	{
		switch (g_cfg.video.renderer.get())
		{
		case video_renderer::null:
		{
			g_fxo->init<rsx::thread, named_thread<NullGSRender>>(ar);
			break;
		}
		case video_renderer::opengl:
		{
#if not defined(__APPLE__)
			g_fxo->init<rsx::thread, named_thread<GLGSRender>>(ar);
#endif
			break;
		}
		case video_renderer::vulkan:
		{
#if defined(HAVE_VULKAN)
			g_fxo->init<rsx::thread, named_thread<VKGSRender>>(ar);
#endif
			break;
		}
		}
	};

	callbacks.get_camera_handler = []() -> std::shared_ptr<camera_handler_base>
	{
		switch (g_cfg.io.camera.get())
		{
		case camera_handler::null:
		case camera_handler::fake:
		{
			return std::make_shared<null_camera_handler>();
		}
		case camera_handler::qt:
		{
			return std::make_shared<qt_camera_handler>();
		}
		}
		return nullptr;
	};

	callbacks.get_music_handler = []() -> std::shared_ptr<music_handler_base>
	{
		switch (g_cfg.audio.music.get())
		{
		case music_handler::null:
		{
			return std::make_shared<null_music_handler>();
		}
		case music_handler::qt:
		{
			return std::make_shared<qt_music_handler>();
		}
		}
		return nullptr;
	};

	callbacks.get_gs_frame    = [this]() -> std::unique_ptr<GSFrameBase> { return get_gs_frame(); };
	callbacks.get_msg_dialog  = [this]() -> std::shared_ptr<MsgDialogBase> { return m_show_gui ? std::make_shared<msg_dialog_frame>() : nullptr; };
	callbacks.get_osk_dialog  = [this]() -> std::shared_ptr<OskDialogBase> { return m_show_gui ? std::make_shared<osk_dialog_frame>() : nullptr; };
	callbacks.get_save_dialog = []() -> std::unique_ptr<SaveDialogBase> { return std::make_unique<save_data_dialog>(); };
	callbacks.get_sendmessage_dialog = [this]() -> std::shared_ptr<SendMessageDialogBase> { return std::make_shared<sendmessage_dialog_frame>(); };
	callbacks.get_recvmessage_dialog = [this]() -> std::shared_ptr<RecvMessageDialogBase> { return std::make_shared<recvmessage_dialog_frame>(); };
	callbacks.get_trophy_notification_dialog = [this]() -> std::unique_ptr<TrophyNotificationBase> { return std::make_unique<trophy_notification_helper>(m_game_window); };

	callbacks.on_run    = [this](bool start_playtime) { OnEmulatorRun(start_playtime); };
	callbacks.on_pause  = [this]() { OnEmulatorPause(); };
	callbacks.on_resume = [this]() { OnEmulatorResume(true); };
	callbacks.on_stop   = [this]() { OnEmulatorStop(); };
	callbacks.on_ready  = [this]() { OnEmulatorReady(); };

	callbacks.enable_disc_eject  = [this](bool enabled)
	{
		Emu.CallFromMainThread([this, enabled]()
		{
			OnEnableDiscEject(enabled);
		});
	};
	callbacks.enable_disc_insert = [this](bool enabled)
	{
		Emu.CallFromMainThread([this, enabled]()
		{
			OnEnableDiscInsert(enabled);
		});
	};

	callbacks.on_missing_fw = [this]()
	{
		if (!m_main_window) return false;
		return m_main_window->OnMissingFw();
	};

	callbacks.handle_taskbar_progress = [this](s32 type, s32 value)
	{
		if (m_game_window)
		{
			switch (type)
			{
			case 0: static_cast<gs_frame*>(m_game_window)->progress_reset(value); break;
			case 1: static_cast<gs_frame*>(m_game_window)->progress_increment(value); break;
			case 2: static_cast<gs_frame*>(m_game_window)->progress_set_limit(value); break;
			case 3: static_cast<gs_frame*>(m_game_window)->progress_set_value(value); break;
			default: gui_log.fatal("Unknown type in handle_taskbar_progress(type=%d, value=%d)", type, value); break;
			}
		}
	};

	callbacks.get_localized_string = [](localized_string_id id, const char* args) -> std::string
	{
		return localized_emu::get_string(id, args);
	};

	callbacks.get_localized_u32string = [](localized_string_id id, const char* args) -> std::u32string
	{
		return localized_emu::get_u32string(id, args);
	};

	callbacks.play_sound = [](const std::string& path)
	{
		Emu.CallFromMainThread([path]()
		{
			if (fs::is_file(path))
			{
				QSound::play(qstr(path));
			}
		});
	};

	if (m_show_gui) // If this is false, we already have a fallback in the main_application.
	{
		callbacks.on_install_pkgs = [this](const std::vector<std::string>& pkgs)
		{
			ensure(m_main_window);
			ensure(!pkgs.empty());
			QStringList pkg_list;
			for (const std::string& pkg : pkgs)
			{
				pkg_list << QString::fromStdString(pkg);
			}
			return m_main_window->InstallPackages(pkg_list, true);
		};
	}

	Emu.SetCallbacks(std::move(callbacks));
}

void gui_application::StartPlaytime(bool start_playtime = true)
{
	if (!start_playtime)
	{
		return;
	}

	const QString serial = qstr(Emu.GetTitleID());
	if (serial.isEmpty())
	{
		return;
	}

	m_persistent_settings->SetLastPlayed(serial, QDateTime::currentDateTime().toString(gui::persistent::last_played_date_format));
	m_timer_playtime.start();
	m_timer.start(10000); // Update every 10 seconds in case the emulation crashes
}

void gui_application::UpdatePlaytime()
{
	if (!m_timer_playtime.isValid())
	{
		m_timer.stop();
		return;
	}

	const QString serial = qstr(Emu.GetTitleID());
	if (serial.isEmpty())
	{
		m_timer_playtime.invalidate();
		m_timer.stop();
		return;
	}

	m_persistent_settings->AddPlaytime(serial, m_timer_playtime.restart());
	m_persistent_settings->SetLastPlayed(serial, QDateTime::currentDateTime().toString(gui::persistent::last_played_date_format));
}

void gui_application::StopPlaytime()
{
	m_timer.stop();

	if (!m_timer_playtime.isValid())
		return;

	const QString serial = qstr(Emu.GetTitleID());
	if (serial.isEmpty())
	{
		m_timer_playtime.invalidate();
		return;
	}

	m_persistent_settings->AddPlaytime(serial, m_timer_playtime.restart());
	m_persistent_settings->SetLastPlayed(serial, QDateTime::currentDateTime().toString(gui::persistent::last_played_date_format));
	m_timer_playtime.invalidate();
}

/*
* Handle a request to change the stylesheet based on the current entry in the settings.
*/
void gui_application::OnChangeStyleSheetRequest()
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

	// Remove old fonts
	QFontDatabase::removeAllApplicationFonts();

	const QString stylesheet_name = m_gui_settings->GetValue(gui::m_currentStylesheet).toString();

	if (stylesheet_name.isEmpty() || stylesheet_name == gui::DefaultStylesheet)
	{
		setStyleSheet(gui::stylesheets::default_style_sheet);
	}
	else if (stylesheet_name == gui::NoStylesheet)
	{
		setStyleSheet("/* none */");
	}
	else
	{
		QString stylesheet_path;
		QString stylesheet_dir;
		QList<QDir> locs;
		locs << m_gui_settings->GetSettingsDir();

#if !defined(_WIN32)
#ifdef __APPLE__
		locs << QCoreApplication::applicationDirPath() + "/../Resources/GuiConfigs/";
#else
#ifdef DATADIR
		const QString data_dir = (DATADIR);
		locs << data_dir + "/GuiConfigs/";
#endif
		locs << QCoreApplication::applicationDirPath() + "/../share/rpcs3/GuiConfigs/";
#endif
		locs << QCoreApplication::applicationDirPath() + "/GuiConfigs/";
#endif

		for (auto&& loc : locs)
		{
			QFileInfo file_info(loc.absoluteFilePath(stylesheet_name + QStringLiteral(".qss")));
			if (file_info.exists())
			{
				loc.cdUp();
				stylesheet_dir  = loc.absolutePath();
				stylesheet_path = file_info.absoluteFilePath();
				break;
			}
		}

		if (QFile file(stylesheet_path); !stylesheet_path.isEmpty() && file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			const QString config_dir = qstr(fs::get_config_dir());

			// Add PS3 fonts
			QDirIterator ps3_font_it(qstr(g_cfg_vfs.get_dev_flash() + "data/font/"), QStringList() << "*.ttf", QDir::Files, QDirIterator::Subdirectories);
			while (ps3_font_it.hasNext())
				QFontDatabase::addApplicationFont(ps3_font_it.next());

			// Add custom fonts
			QDirIterator custom_font_it(config_dir + "fonts/", QStringList() << "*.ttf", QDir::Files, QDirIterator::Subdirectories);
			while (custom_font_it.hasNext())
				QFontDatabase::addApplicationFont(custom_font_it.next());

			// Replace relative paths with absolute paths. Since relative paths should always be the same, we can just use simple string replacement.
			// Another option would be to use QDir::setCurrent, but that changes current working directory for the whole process (We don't want that).
			QString stylesheet = file.readAll();
			stylesheet.replace(QStringLiteral("url(\"GuiConfigs/"), QStringLiteral("url(\"") + stylesheet_dir + QStringLiteral("/GuiConfigs/"));
			setStyleSheet(stylesheet);
			file.close();
		}
		else
		{
			gui_log.error("Could not find stylesheet '%s'. Using default.", stylesheet_name.toStdString());
			setStyleSheet(gui::stylesheets::default_style_sheet);
		}
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
void gui_application::CallFromMainThread(const std::function<void()>& func, atomic_t<bool>* wake_up)
{
	func();

	if (wake_up)
	{
		*wake_up = true;
		wake_up->notify_one();
	}
}
