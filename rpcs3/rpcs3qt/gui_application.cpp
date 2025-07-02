#include "stdafx.h"
#include "gui_application.h"

#include "qt_utils.h"
#include "permissions.h"
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
#include "display_sleep_control.h"

#ifdef WITH_DISCORD_RPC
#include "_discord_utils.h"
#endif

#include "Emu/Audio/audio_utils.h"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "Emu/Io/Null/null_camera_handler.h"
#include "Emu/Io/Null/null_music_handler.h"
#include "Emu/vfs_config.h"
#include "util/init_mutex.hpp"
#include "util/console.h"
#include "qt_video_source.h"
#include "trophy_notification_helper.h"
#include "save_data_dialog.h"
#include "msg_dialog_frame.h"
#include "osk_dialog_frame.h"
#include "recvmessage_dialog_frame.h"
#include "sendmessage_dialog_frame.h"
#include "stylesheets.h"
#include "progress_dialog.h"

#include <QScreen>
#include <QFontDatabase>
#include <QLayout>
#include <QLibraryInfo>
#include <QDirIterator>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextDocument>
#include <QStyleFactory>
#include <QStyleHints>

#include <clocale>

#include "Emu/RSX/Null/NullGSRender.h"
#include "Emu/RSX/GL/GLGSRender.h"

#if defined(HAVE_VULKAN)
#include "Emu/RSX/VK/VKGSRender.h"
#endif

#ifdef _WIN32
#include <Usbiodef.h>
#include <Dbt.h>

#include "Emu/Cell/lv2/sys_usbd.h"
#endif

LOG_CHANNEL(gui_log, "GUI");

std::unique_ptr<raw_mouse_handler> g_raw_mouse_handler;

s32 gui_application::m_language_id = static_cast<s32>(CELL_SYSUTIL_LANG_ENGLISH_US);

[[noreturn]] void report_fatal_error(std::string_view text, bool is_html = false, bool include_help_text = true);

gui_application::gui_application(int& argc, char** argv) : QApplication(argc, argv)
{
	std::setlocale(LC_NUMERIC, "C"); // On linux Qt changes to system locale while initializing QCoreApplication
}

gui_application::~gui_application()
{
#ifdef WITH_DISCORD_RPC
	discord::shutdown();
#endif
#ifdef _WIN32
	unregister_device_notification();
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
		msg.setWindowModality(Qt::WindowModal);
		msg.setWindowTitle(tr("Experimental Build Warning"));
		msg.setIcon(QMessageBox::Critical);
		msg.setTextFormat(Qt::RichText);
		msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msg.setDefaultButton(QMessageBox::No);
		msg.setText(gui::utils::make_paragraph(tr(
			"Please understand that this build is not an official RPCS3 release.\n"
			"This build contains changes that may break games, or even <b>damage</b> your data.\n"
			"We recommend to download and use the official build from the %0.\n"
			"\n"
			"Build origin: %1\n"
			"Do you wish to use this build anyway?")
			.arg(gui::utils::make_link(tr("RPCS3 website"), "https://rpcs3.net/download"))
			.arg(Qt::convertFromPlainText(branch_name.data()))));
		msg.layout()->setSizeConstraint(QLayout::SetFixedSize);

		if (msg.exec() == QMessageBox::No)
		{
			return false;
		}
	}

	m_emu_settings = std::make_shared<emu_settings>();
	m_gui_settings = std::make_shared<gui_settings>();
	m_persistent_settings = std::make_shared<persistent_settings>();

	if (!m_emu_settings->Init())
	{
		return false;
	}

	if (m_gui_settings->GetValue(gui::m_attachCommandLine).toBool())
	{
		utils::attach_console(utils::console_stream::std_err, true);
	}
	else
	{
		m_gui_settings->SetValue(gui::m_attachCommandLine, false);
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
		welcome_dialog* welcome = new welcome_dialog(m_gui_settings, false);

		if (welcome->exec() == QDialog::Rejected)
		{
			// If the agreement on RPCS3's usage conditions was not accepted by the user, ask the main window to gracefully terminate
			return false;
		}
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

	// Install native event filter
#ifdef _WIN32 // Currently only needed for raw mouse input on windows
	installNativeEventFilter(&m_native_event_filter);

	if (m_main_window)
	{
		register_device_notification(m_main_window->winId());
	}
#endif

	return true;
}

void gui_application::SwitchTranslator(QTranslator& translator, const QString& filename, const QString& language_code)
{
	// remove the old translator
	removeTranslator(&translator);

	const QString lang_path = QLibraryInfo::path(QLibraryInfo::TranslationsPath) + QStringLiteral("/");
	const QString file_path = lang_path + filename;

	if (QFileInfo(file_path).isFile())
	{
		// load the new translator
		if (translator.load(file_path))
		{
			installTranslator(&translator);
		}
	}
	else if (QString default_code = QLocale(QLocale::English).bcp47Name(); language_code != default_code)
	{
		// show error, but ignore default case "en", since it is handled in source code
		gui_log.error("No translation file found in: %s", file_path);

		// reset current language to default "en"
		set_language_code(std::move(default_code));
	}
}

void gui_application::LoadLanguage(const QString& language_code)
{
	if (m_language_code == language_code)
	{
		return;
	}

	set_language_code(language_code);

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

	gui_log.notice("Current language changed to %s (%s)", locale_name, language_code);
}

QStringList gui_application::GetAvailableLanguageCodes()
{
	QStringList language_codes;

	const QString language_path = QLibraryInfo::path(QLibraryInfo::TranslationsPath);

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
				gui_log.error("Found duplicate language '%s' (%s)", language_code, filename);
			}
			else
			{
				language_codes << language_code;
			}
		}
	}

	return language_codes;
}

void gui_application::set_language_code(QString language_code)
{
	m_language_code = language_code;

	// Transform language code to lowercase and use '-'
	language_code = language_code.toLower().replace("_", "-");

	// Try to find the CELL language ID for this language code
	static const std::map<QString, CellSysutilLang> language_ids = {
		{"ja", CELL_SYSUTIL_LANG_JAPANESE },
		{"en", CELL_SYSUTIL_LANG_ENGLISH_US },
		{"en-us", CELL_SYSUTIL_LANG_ENGLISH_US },
		{"en-gb", CELL_SYSUTIL_LANG_ENGLISH_GB },
		{"fr", CELL_SYSUTIL_LANG_FRENCH },
		{"es", CELL_SYSUTIL_LANG_SPANISH },
		{"de", CELL_SYSUTIL_LANG_GERMAN },
		{"it", CELL_SYSUTIL_LANG_ITALIAN },
		{"nl", CELL_SYSUTIL_LANG_DUTCH },
		{"pt", CELL_SYSUTIL_LANG_PORTUGUESE_PT },
		{"pt-pt", CELL_SYSUTIL_LANG_PORTUGUESE_PT },
		{"pt-br", CELL_SYSUTIL_LANG_PORTUGUESE_BR },
		{"ru", CELL_SYSUTIL_LANG_RUSSIAN },
		{"ko", CELL_SYSUTIL_LANG_KOREAN },
		{"zh", CELL_SYSUTIL_LANG_CHINESE_T },
		{"zh-hant", CELL_SYSUTIL_LANG_CHINESE_T },
		{"zh-hans", CELL_SYSUTIL_LANG_CHINESE_S },
		{"fi", CELL_SYSUTIL_LANG_FINNISH },
		{"sv", CELL_SYSUTIL_LANG_SWEDISH },
		{"da", CELL_SYSUTIL_LANG_DANISH },
		{"no", CELL_SYSUTIL_LANG_NORWEGIAN },
		{"nn", CELL_SYSUTIL_LANG_NORWEGIAN },
		{"nb", CELL_SYSUTIL_LANG_NORWEGIAN },
		{"pl", CELL_SYSUTIL_LANG_POLISH },
		{"tr", CELL_SYSUTIL_LANG_TURKISH },
	};

	// Check direct match first
	const auto it = language_ids.find(language_code);
	if (it != language_ids.cend())
	{
		m_language_id = static_cast<s32>(it->second);
		return;
	}

	// Try to find closest match
	for (const auto& [code, id] : language_ids)
	{
		if (language_code.startsWith(code))
		{
			m_language_id = static_cast<s32>(id);
			return;
		}
	}

	// Fallback to English (US)
	m_language_id = static_cast<s32>(CELL_SYSUTIL_LANG_ENGLISH_US);
}

s32 gui_application::get_language_id()
{
	return m_language_id;
}

void gui_application::InitializeConnects()
{
	connect(&m_timer, &QTimer::timeout, this, &gui_application::UpdatePlaytime);
	connect(this, &gui_application::OnEmulatorRun, this, &gui_application::StartPlaytime);
	connect(this, &gui_application::OnEmulatorStop, this, &gui_application::StopPlaytime);
	connect(this, &gui_application::OnEmulatorPause, this, &gui_application::StopPlaytime);
	connect(this, &gui_application::OnEmulatorResume, this, &gui_application::StartPlaytime);
	connect(this, &QGuiApplication::applicationStateChanged, this, &gui_application::OnAppStateChanged);

	if (m_main_window)
	{
		connect(m_main_window, &main_window::RequestLanguageChange, this, &gui_application::LoadLanguage);
		connect(m_main_window, &main_window::RequestGlobalStylesheetChange, this, &gui_application::OnChangeStyleSheetRequest);
		connect(m_main_window, &main_window::NotifyEmuSettingsChange, this, [this](){ OnEmuSettingsChange(); });
		connect(m_main_window, &main_window::NotifyShortcutHandlers, this, &gui_application::OnShortcutChange);

		connect(this, &gui_application::OnEmulatorRun, m_main_window, &main_window::OnEmuRun);
		connect(this, &gui_application::OnEmulatorStop, m_main_window, &main_window::OnEmuStop);
		connect(this, &gui_application::OnEmulatorPause, m_main_window, &main_window::OnEmuPause);
		connect(this, &gui_application::OnEmulatorResume, m_main_window, &main_window::OnEmuResume);
		connect(this, &gui_application::OnEmulatorReady, m_main_window, &main_window::OnEmuReady);
		connect(this, &gui_application::OnEnableDiscEject, m_main_window, &main_window::OnEnableDiscEject);
		connect(this, &gui_application::OnEnableDiscInsert, m_main_window, &main_window::OnEnableDiscInsert);

		connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this](){ OnChangeStyleSheetRequest(); });
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
	// Load AppIcon
	const QIcon app_icon = m_main_window ? m_main_window->GetAppIcon() : gui::utils::get_app_icon_from_path(Emu.GetBoot(), Emu.GetTitleID());

	if (m_game_window)
	{
		// Check if the continuous mode is enabled. We reset the mode after each use in order to ensure that it is only used when explicitly needed.
		const bool continuous_mode_enabled = Emu.ContinuousModeEnabled(true);

		// Make sure we run the same config
		const bool is_same_renderer = m_game_window->renderer() == g_cfg.video.renderer;

		if (is_same_renderer && (Emu.IsChildProcess() || continuous_mode_enabled))
		{
			gui_log.notice("gui_application: Re-using old game window (IsChildProcess=%d, ContinuousModeEnabled=%d)", Emu.IsChildProcess(), continuous_mode_enabled);

			if (!app_icon.isNull())
			{
				m_game_window->setIcon(app_icon);
			}
			return std::unique_ptr<gs_frame>(m_game_window);
		}

		// Clean-up old game window. This should only happen if the renderer changed or there was an unexpected error during boot.
		Emu.GetCallbacks().close_gs_frame();
	}

	gui_log.notice("gui_application: Creating new game window");

	extern const std::unordered_map<video_resolution, std::pair<int, int>, value_hash<video_resolution>> g_video_out_resolution_map;

	auto [w, h] = ::at32(g_video_out_resolution_map, g_cfg.video.resolution);

	const bool resize_game_window = m_gui_settings->GetValue(gui::gs_resize).toBool();

	if (resize_game_window)
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
	const int last_screen_index = m_gui_settings->GetValue(gui::gs_screen).toInt();

	// Use last used screen if no CLI index was set
	if (screen_index < 0)
	{
		screen_index = last_screen_index;
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

	// Use saved geometry if possible. Ignore this if the last used screen is different than the requested screen.
	QRect frame_geometry = screen_index != last_screen_index ? QRect{} : m_gui_settings->GetValue(gui::gs_geometry).value<QRect>();

	if (frame_geometry.isNull() || frame_geometry.isEmpty())
	{
		// Center above main window or inside screen if the saved geometry is invalid
		frame_geometry = gui::utils::create_centered_window_geometry(screen, base_geometry, w, h);
	}
	else if (resize_game_window)
	{
		// Apply size override to our saved geometry if needed
		frame_geometry.setSize(QSize(w, h));
	}

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
	ensure(m_game_window);

#ifdef _WIN32
	if (!m_show_gui)
	{
		register_device_notification(m_game_window->winId());
	}
#endif

	connect(m_game_window, &gs_frame::destroyed, this, [this]()
	{
		gui_log.notice("gui_application: Deleting old game window");
		m_game_window = nullptr;

#ifdef _WIN32
		if (!m_show_gui)
		{
			unregister_device_notification();
		}
#endif
	});

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

			gui_log.notice("Quitting gui application");
			quit();
			return true;
		}

		return false;
	};
	callbacks.call_from_main_thread = [this](std::function<void()> func, atomic_t<u32>* wake_up)
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

	callbacks.close_gs_frame  = [this]()
	{
		if (m_game_window)
		{
			gui_log.warning("gui_application: Closing old game window");
			m_game_window->ignore_stop_events();
			delete m_game_window;
			m_game_window = nullptr;
		}
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
		if (m_main_window)
		{
			m_main_window->OnMissingFw();
		}
	};

	callbacks.handle_taskbar_progress = [this](s32 type, s32 value)
	{
		if (m_game_window)
		{
			switch (type)
			{
			case 0: m_game_window->progress_reset(value); break;
			case 1: m_game_window->progress_increment(value); break;
			case 2: m_game_window->progress_set_limit(value); break;
			case 3: m_game_window->progress_set_value(value); break;
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

	callbacks.get_localized_setting = [this](const cfg::_base* node, u32 enum_index) -> std::string
	{
		ensure(!!m_emu_settings);
		return m_emu_settings->GetLocalizedSetting(node, enum_index);
	};

	callbacks.play_sound = [this](const std::string& path)
	{
		Emu.CallFromMainThread([this, path]()
		{
			if (fs::is_file(path))
			{
				// Allow to play 3 sound effects at the same time
				while (m_sound_effects.size() >= 3)
				{
					m_sound_effects.pop_front();
				}

				// Create a new sound effect. Re-using the same object seems to be broken for some users starting with Qt 6.6.3.
				std::unique_ptr<QSoundEffect> sound_effect = std::make_unique<QSoundEffect>();
				sound_effect->setSource(QUrl::fromLocalFile(QString::fromStdString(path)));
				sound_effect->setVolume(audio::get_volume());
				sound_effect->play();

				m_sound_effects.push_back(std::move(sound_effect));
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

	callbacks.on_emulation_stop_no_response = [this](std::shared_ptr<atomic_t<bool>> closed_successfully, int seconds_waiting_already)
	{
		const std::string terminate_message = tr("Stopping emulator took too long."
			"\nSome thread has probably deadlocked. Aborting.").toStdString();

		if (!closed_successfully)
		{
			report_fatal_error(terminate_message);
		}

		Emu.CallFromMainThread([this, closed_successfully, seconds_waiting_already, terminate_message]
		{
			const auto seconds = std::make_shared<int>(seconds_waiting_already);

			QMessageBox* mb = new QMessageBox();
			mb->setWindowTitle(tr("PS3 Game/Application Is Unresponsive"));
			mb->setIcon(QMessageBox::Critical);
			mb->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			mb->setDefaultButton(QMessageBox::No);
			mb->button(QMessageBox::Yes)->setText(tr("Terminate RPCS3"));
			mb->button(QMessageBox::No)->setText(tr("Keep Waiting"));

			QString text_base = tr("Waiting for %0 second(s) already to stop emulation without success."
			                       "\nKeep waiting or terminate RPCS3 unsafely at your own risk?");

			mb->setText(text_base.arg(10));
			mb->layout()->setSizeConstraint(QLayout::SetFixedSize);
			mb->setAttribute(Qt::WA_DeleteOnClose);

			QTimer* update_timer = new QTimer(mb);

			connect(update_timer, &QTimer::timeout, [mb, seconds, text_base, closed_successfully]()
			{
				*seconds += 1;
				mb->setText(text_base.arg(*seconds));

				if (*closed_successfully)
				{
					mb->reject();
				}
			});

			connect(mb, &QDialog::accepted, mb, [closed_successfully, terminate_message]
			{
				if (!*closed_successfully)
				{
					report_fatal_error(terminate_message);
				}
			});

			mb->open();
			update_timer->start(1000);
		});
	};

	callbacks.on_save_state_progress = [this](std::shared_ptr<atomic_t<bool>> closed_successfully, stx::shared_ptr<utils::serial> ar_ptr, stx::atomic_ptr<std::string>* code_location, std::shared_ptr<void> init_mtx)
	{
		Emu.CallFromMainThread([this, closed_successfully, ar_ptr, code_location, init_mtx]
		{
			const auto half_seconds = std::make_shared<int>(1);

			progress_dialog* pdlg = new progress_dialog(tr("Creating Save-State / Do Not Close RPCS3"), tr("Please wait..."), tr("Hide Progress"), 0, 100, true, m_main_window);
			pdlg->setAutoReset(false);
			pdlg->setAutoClose(true);
			pdlg->show();

			QString text_base = tr("%0 written, %1 second(s) passed%2");

			pdlg->setLabelText(text_base.arg("0B").arg(1).arg(""));
			pdlg->setAttribute(Qt::WA_DeleteOnClose);

			QTimer* update_timer = new QTimer(pdlg);

			connect(update_timer, &QTimer::timeout, [pdlg, ar_ptr, half_seconds, text_base, closed_successfully
				, code_location, init_mtx, old_written = usz{0}, repeat_count = u32{0}]() mutable
			{
				std::string verbose_message;
				usz bytes_written = 0;

				while (true)
				{
					auto mtx = static_cast<stx::init_mutex*>(init_mtx.get());
					auto init = mtx->access();

					if (!init)
					{
						// Try to wait for the abort process to complete
						auto fake_reset = mtx->reset();
						if (!fake_reset)
						{
							// End of emulation termination
							pdlg->reject();
							return;
						}

						fake_reset.set_init();

						// Now ar_ptr contains a null file descriptor
						continue;
					}

					if (auto str_ptr = code_location->load())
					{
						verbose_message = "\n" + *str_ptr;
					}

					bytes_written = ar_ptr->is_writing() ? std::max<usz>(ar_ptr->get_size(), old_written) : old_written;
					break;
				}

				*half_seconds += 1;

				if (old_written == bytes_written)
				{
					if (repeat_count == 60)
					{
						if (verbose_message.empty())
						{
							verbose_message += "\n";
						}
						else
						{
							verbose_message += ". ";
						}

						verbose_message += tr("If Stuck, Report To Developers").toStdString();
					}
					else
					{
						repeat_count++;
					}
				}
				else
				{
					repeat_count = 0;
				}

				old_written = bytes_written;

				pdlg->setLabelText(text_base.arg(gui::utils::format_byte_size(bytes_written)).arg(*half_seconds / 2).arg(QString::fromStdString(verbose_message)));

				// 300MB -> 50%, 600MB -> 75%, 1200MB -> 87.5% etc
				const int percent = std::clamp(static_cast<int>(100. - 100. / std::pow(2., std::fmax(0.01, bytes_written * 1. / (300 * 1024 * 1024)))), 2, 100);

				// Add a third of the remaining progress when the keyword is found
				pdlg->setValue(verbose_message.find("Finalizing") != umax ? 100 - ((100 - percent) * 2 / 3) : percent);

				if (*closed_successfully)
				{
					pdlg->reject();
				}
			});

			pdlg->open();
			update_timer->start(500);
		});
	};

	callbacks.add_breakpoint = [this](u32 addr)
	{
		Emu.BlockingCallFromMainThread([this, addr]()
		{
			m_main_window->OnAddBreakpoint(addr);
		});
	};

	callbacks.display_sleep_control_supported = [](){ return display_sleep_control_supported(); };
	callbacks.enable_display_sleep = [](bool enabled){ enable_display_sleep(enabled); };

	callbacks.check_microphone_permissions = []()
	{
		Emu.BlockingCallFromMainThread([]()
		{
			gui::utils::check_microphone_permission();
		});
	};

	callbacks.make_video_source = [](){ return std::make_unique<qt_video_source_wrapper>(); };

	Emu.SetCallbacks(std::move(callbacks));
}

void gui_application::StartPlaytime(bool start_playtime = true)
{
	if (!start_playtime)
	{
		return;
	}

	const QString serial = QString::fromStdString(Emu.GetTitleID());
	if (serial.isEmpty())
	{
		return;
	}

	m_persistent_settings->SetLastPlayed(serial, QDateTime::currentDateTime().toString(gui::persistent::last_played_date_format), true);
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

	const QString serial = QString::fromStdString(Emu.GetTitleID());
	if (serial.isEmpty())
	{
		m_timer_playtime.invalidate();
		m_timer.stop();
		return;
	}

	m_persistent_settings->AddPlaytime(serial, m_timer_playtime.restart(), false);
	m_persistent_settings->SetLastPlayed(serial, QDateTime::currentDateTime().toString(gui::persistent::last_played_date_format), true);
}

void gui_application::StopPlaytime()
{
	m_timer.stop();

	if (!m_timer_playtime.isValid())
		return;

	const QString serial = QString::fromStdString(Emu.GetTitleID());
	if (serial.isEmpty())
	{
		m_timer_playtime.invalidate();
		return;
	}

	m_persistent_settings->AddPlaytime(serial, m_timer_playtime.restart(), false);
	m_persistent_settings->SetLastPlayed(serial, QDateTime::currentDateTime().toString(gui::persistent::last_played_date_format), true);
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

	// Determine default style
	if (m_default_style.isEmpty())
	{
#ifdef _WIN32
		// On windows, the custom stylesheets don't seem to work properly unless we use the windowsvista style as default
		if (QStyleFactory::keys().contains("windowsvista"))
		{
			m_default_style = "windowsvista";
			gui_log.notice("Using '%s' as default style", m_default_style);
		}
#endif

		// Use the initial style as default style
		if (const QStyle* style = m_default_style.isEmpty() ? QApplication::style() : nullptr)
		{
			m_default_style = style->name();
			gui_log.notice("Determined '%s' as default style", m_default_style);
		}

		// Fallback to the first style, which is supposed to be the default style according to the Qt docs.
		if (m_default_style.isEmpty())
		{
			if (const QStringList styles = QStyleFactory::keys(); !styles.empty())
			{
				m_default_style = styles.front();
				gui_log.notice("Determined '%s' as default style (first style available)", m_default_style);
			}
		}
	}

	// Reset style to default before doing anything else, or we will get unexpected effects in custom stylesheets.
	if (QStyle* style = QStyleFactory::create(m_default_style))
	{
		setStyle(style);
	}

	const auto match_native_style = [&stylesheet_name]() -> QString
	{
		// Search for "native (<style>)"
		static const QRegularExpression expr(gui::NativeStylesheet + " \\((?<style>.*)\\)");
		const QRegularExpressionMatch match = expr.match(stylesheet_name);

		if (match.hasMatch())
		{
			return match.captured("style");
		}

		return {};
	};

	gui_log.notice("Changing stylesheet to '%s'", stylesheet_name);
	gui::custom_stylesheet_active = false;

	if (stylesheet_name.isEmpty() || stylesheet_name == gui::DefaultStylesheet)
	{
		gui_log.notice("Using default stylesheet");
		setStyleSheet(gui::stylesheets::default_style_sheet);
		gui::custom_stylesheet_active = true;
	}
	else if (stylesheet_name == gui::NoStylesheet)
	{
		gui_log.notice("Using empty style");
		setStyleSheet("/* none */");
	}
	else if (const QString native_style = match_native_style(); !native_style.isEmpty())
	{
		if (QStyle* style = QStyleFactory::create(native_style))
		{
			gui_log.notice("Using native style '%s'", native_style);
			setStyleSheet("/* none */");
			setStyle(style);
		}
		else
		{
			gui_log.error("Failed to set stylesheet: Native style '%s' not available", native_style);
		}
	}
	else
	{
		QString stylesheet_path;
		QString stylesheet_dir;
		std::vector<QDir> locs;
		locs.push_back(m_gui_settings->GetSettingsDir());

#if !defined(_WIN32)
#ifdef __APPLE__
		locs.push_back(QCoreApplication::applicationDirPath() + "/../Resources/GuiConfigs/");
#else
#ifdef DATADIR
		const QString data_dir = (DATADIR);
		locs.push_back(data_dir + "/GuiConfigs/");
#endif
		locs.push_back(QCoreApplication::applicationDirPath() + "/../share/rpcs3/GuiConfigs/");
#endif
		locs.push_back(QCoreApplication::applicationDirPath() + "/GuiConfigs/");
#endif

		for (QDir& loc : locs)
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
			const QString config_dir = QString::fromStdString(fs::get_config_dir());

			// Add PS3 fonts
			QDirIterator ps3_font_it(QString::fromStdString(g_cfg_vfs.get_dev_flash() + "data/font/"), QStringList() << "*.ttf", QDir::Files, QDirIterator::Subdirectories);
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
			gui_log.error("Could not find stylesheet '%s'. Using default.", stylesheet_name);
			setStyleSheet(gui::stylesheets::default_style_sheet);
		}

		gui::custom_stylesheet_active = true;
	}

	gui::stylesheet = styleSheet();

	if (m_main_window)
	{
		m_main_window->RepaintGui();
	}
}

void gui_application::OnShortcutChange()
{
	if (m_game_window)
	{
		m_game_window->update_shortcuts();
	}
}

/**
 * Using connects avoids timers being unable to be used in a non-qt thread. So, even if this looks stupid to just call func, it's succinct.
 */
void gui_application::CallFromMainThread(const std::function<void()>& func, atomic_t<u32>* wake_up)
{
	func();

	if (wake_up)
	{
		*wake_up = true;
		wake_up->notify_one();
	}
}

void gui_application::OnAppStateChanged(Qt::ApplicationState state)
{
	// Invalidate previous delayed pause call (even when the setting is off because it is dynamic)
	m_pause_delayed_tag++;

	if (!g_cfg.misc.autopause)
	{
		return;
	}

	const auto emu_state = Emu.GetStatus();
	const bool is_active = state & Qt::ApplicationActive;

	if (emu_state != system_state::paused && emu_state != system_state::running)
	{
		return;
	}

	const bool is_paused = emu_state == system_state::paused;

	if (is_active != is_paused)
	{
		// Nothing to do (either paused and this is focus-out event or running and this is a focus-in event)
		// Invalidate data
		m_is_pause_on_focus_loss_active = false;
		m_emu_focus_out_emulation_id = Emulator::stop_counter_t{};
		return;
	}

	if (is_paused)
	{
		// Check if Emu.Resume() or Emu.Kill() has not been called since
		if (m_is_pause_on_focus_loss_active && m_pause_amend_time_on_focus_loss == Emu.GetPauseTime() && m_emu_focus_out_emulation_id == Emu.GetEmulationIdentifier())
		{
			m_is_pause_on_focus_loss_active = false;
			Emu.Resume();
		}

		return;
	}

	// Gather validation data
	m_emu_focus_out_emulation_id = Emu.GetEmulationIdentifier();

	auto pause_callback = [this, delayed_tag = m_pause_delayed_tag]()
	{
		// Check if Emu.Kill() has not been called since
		if (applicationState() != Qt::ApplicationActive && Emu.IsRunning() &&
		    m_emu_focus_out_emulation_id == Emu.GetEmulationIdentifier() &&
		    delayed_tag == m_pause_delayed_tag &&
			!m_is_pause_on_focus_loss_active)
		{
			if (Emu.Pause())
			{
				// Gather validation data
				m_pause_amend_time_on_focus_loss = Emu.GetPauseTime();
				m_emu_focus_out_emulation_id = Emu.GetEmulationIdentifier();
				m_is_pause_on_focus_loss_active = true;
			}
		}
	};

	if (state == Qt::ApplicationSuspended)
	{
		// Must be invoked now (otherwise it may not happen later)
		pause_callback();
		return;
	}

	// Delay pause so it won't immediately pause the emulated application
	QTimer::singleShot(1000, this, pause_callback);
}

bool gui_application::native_event_filter::nativeEventFilter([[maybe_unused]] const QByteArray& eventType, [[maybe_unused]] void* message, [[maybe_unused]] qintptr* result)
{
#ifdef _WIN32
	if (!Emu.IsRunning() && !Emu.IsStarting() && !g_raw_mouse_handler)
	{
		return false;
	}

	if (eventType == "windows_generic_MSG")
	{
		if (MSG* msg = static_cast<MSG*>(message); msg && (msg->message == WM_INPUT || msg->message == WM_KEYDOWN || msg->message == WM_KEYUP || msg->message == WM_DEVICECHANGE))
		{
			if (msg->message == WM_DEVICECHANGE && (msg->wParam == DBT_DEVICEARRIVAL || msg->wParam == DBT_DEVICEREMOVECOMPLETE))
			{
				if (Emu.IsRunning() || Emu.IsStarting())
				{
					handle_hotplug_event(msg->wParam == DBT_DEVICEARRIVAL);
				}
				return false;
			}

			if (auto* handler = g_fxo->try_get<MouseHandlerBase>(); handler && handler->type == mouse_handler::raw)
			{
				static_cast<raw_mouse_handler*>(handler)->handle_native_event(*msg);
			}

			if (g_raw_mouse_handler)
			{
				g_raw_mouse_handler->handle_native_event(*msg);
			}
		}
	}
#endif

	return false;
}

#ifdef _WIN32
void gui_application::register_device_notification(WId window_id)
{
	if (m_device_notification_handle) return;

	gui_log.notice("Registering device notifications...");

	// Enable usb device hotplug events
	// Currently only needed for hotplug on windows, as libusb handles other platforms
	DEV_BROADCAST_DEVICEINTERFACE notification_filter {};
	notification_filter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	notification_filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	notification_filter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;

	m_device_notification_handle = RegisterDeviceNotification(reinterpret_cast<HWND>(window_id), &notification_filter, DEVICE_NOTIFY_WINDOW_HANDLE);
	if (!m_device_notification_handle )
	{
		gui_log.error("RegisterDeviceNotification() failed: %s", fmt::win_error{GetLastError(), nullptr});
	}
}

void gui_application::unregister_device_notification()
{
	if (m_device_notification_handle)
	{
		gui_log.notice("Unregistering device notifications...");

		if (!UnregisterDeviceNotification(m_device_notification_handle))
		{
			gui_log.error("UnregisterDeviceNotification() failed: %s", fmt::win_error{GetLastError(), nullptr});
		}

		m_device_notification_handle = {};
	}
}
#endif
