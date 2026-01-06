#include "stdafx.h"
#include "game_list_context_menu.h"
#include "game_list_frame.h"
#include "gui_settings.h"
#include "category.h"
#include "input_dialog.h"
#include "qt_utils.h"
#include "shortcut_utils.h"
#include "settings_dialog.h"
#include "pad_settings_dialog.h"
#include "patch_manager_dialog.h"
#include "persistent_settings.h"

#include "Utilities/File.h"
#include "Emu/system_utils.hpp"

#include "QApplication"
#include "QClipboard"
#include "QDesktopServices"
#include "QFileDialog"
#include "QInputDialog"
#include "QMessageBox"

LOG_CHANNEL(game_list_log, "GameList");
LOG_CHANNEL(sys_log, "SYS");

std::string get_savestate_file(std::string_view title_id, std::string_view boot_pat, s64 rel_id, u64 aggregate_file_size = umax);

game_list_context_menu::game_list_context_menu(game_list_frame* frame)
	: QMenu(frame)
	, m_game_list_frame(ensure(frame))
	, m_game_list_actions(ensure(frame->actions()))
	, m_gui_settings(ensure(frame->get_gui_settings()))
	, m_emu_settings(ensure(frame->get_emu_settings()))
	, m_persistent_settings(ensure(frame->get_persistent_settings()))
{
}

game_list_context_menu::~game_list_context_menu()
{
}

void game_list_context_menu::show_menu(const std::vector<game_info>& games, const QPoint& global_pos)
{
	if (games.empty()) return;

	if (games.size() == 1)
	{
		show_single_selection_context_menu(games.front(), global_pos);
	}
	else
	{
		show_multi_selection_context_menu(games, global_pos);
	}
}

void game_list_context_menu::show_single_selection_context_menu(const game_info& gameinfo, const QPoint& global_pos)
{
	ensure(!!gameinfo);

	GameInfo current_game = gameinfo->info;
	const std::string serial = current_game.serial;
	const QString name = QString::fromStdString(current_game.name).simplified();
	const bool is_current_running_game = game_list_actions::IsGameRunning(serial);

	// Make Actions
	QAction* boot = new QAction(gameinfo->has_custom_config
		? (is_current_running_game
			? tr("&Reboot with Global Configuration")
			: tr("&Boot with Global Configuration"))
		: (is_current_running_game
			? tr("&Reboot")
			: tr("&Boot")));

	QFont font = boot->font();
	font.setBold(true);

	if (gameinfo->has_custom_config)
	{
		QAction* boot_custom = addAction(is_current_running_game
			? tr("&Reboot with Custom Configuration")
			: tr("&Boot with Custom Configuration"));
		boot_custom->setFont(font);
		connect(boot_custom, &QAction::triggered, m_game_list_frame, [this, gameinfo]
		{
			sys_log.notice("Booting from gamelist per context menu...");
			Q_EMIT m_game_list_frame->RequestBoot(gameinfo);
		});
	}
	else
	{
		boot->setFont(font);
	}

	addAction(boot);

	{
		QAction* boot_default = addAction(is_current_running_game
			? tr("&Reboot with Default Configuration")
			: tr("&Boot with Default Configuration"));

		connect(boot_default, &QAction::triggered, m_game_list_frame, [this, gameinfo]
		{
			sys_log.notice("Booting from gamelist per context menu...");
			Q_EMIT m_game_list_frame->RequestBoot(gameinfo, cfg_mode::default_config);
		});

		QAction* boot_manual = addAction(is_current_running_game
			? tr("&Reboot with Manually Selected Configuration")
			: tr("&Boot with Manually Selected Configuration"));

		connect(boot_manual, &QAction::triggered, m_game_list_frame, [this, gameinfo]
		{
			if (const std::string file_path = QFileDialog::getOpenFileName(m_game_list_frame, "Select Config File", "", tr("Config Files (*.yml);;All files (*.*)")).toStdString(); !file_path.empty())
			{
				sys_log.notice("Booting from gamelist per context menu...");
				Q_EMIT m_game_list_frame->RequestBoot(gameinfo, cfg_mode::custom_selection, file_path);
			}
			else
			{
				sys_log.notice("Manual config selection aborted.");
			}
		});
	}

	extern bool is_savestate_compatible(const std::string& filepath);

	if (const std::string sstate = get_savestate_file(serial, current_game.path, 1); is_savestate_compatible(sstate))
	{
		const bool has_ambiguity = !get_savestate_file(serial, current_game.path, 2).empty();

		QAction* boot_state = addAction(is_current_running_game
			? tr("&Reboot with last SaveState")
			: tr("&Boot with last SaveState"));

		connect(boot_state, &QAction::triggered, m_game_list_frame, [this, gameinfo, sstate]
		{
			sys_log.notice("Booting savestate from gamelist per context menu...");
			Q_EMIT m_game_list_frame->RequestBoot(gameinfo, cfg_mode::custom, "", sstate);
		});

		if (has_ambiguity)
		{
			QAction* choose_state = addAction(is_current_running_game
				? tr("&Choose SaveState to reboot")
				: tr("&Choose SaveState to boot"));

			connect(choose_state, &QAction::triggered, m_game_list_frame, [this, gameinfo]
			{
				// If there is any ambiguity, launch the savestate manager
				Q_EMIT m_game_list_frame->RequestSaveStateManager(gameinfo);
			});
		}
	}

	addSeparator();

	QAction* configure = addAction(gameinfo->has_custom_config
		? tr("&Change Custom Configuration")
		: tr("&Create Custom Configuration From Global Settings"));
	QAction* create_game_default_config = gameinfo->has_custom_config ? nullptr
		: addAction(tr("&Create Custom Configuration From Default Settings"));
	QAction* pad_configure = addAction(gameinfo->has_custom_pad_config
		? tr("&Change Custom Gamepad Configuration")
		: tr("&Create Custom Gamepad Configuration"));
	QAction* configure_patches = addAction(tr("&Manage Game Patches"));

	addSeparator();

	// Create LLVM cache
	QAction* create_cpu_cache = addAction(tr("&Create LLVM Cache"));

	// Remove menu
	QMenu* remove_menu = addMenu(tr("&Remove"));

	if (gameinfo->has_custom_config)
	{
		QAction* remove_custom_config = remove_menu->addAction(tr("&Remove Custom Configuration"));
		connect(remove_custom_config, &QAction::triggered, this, [this, serial, gameinfo]()
		{
			if (m_game_list_actions->RemoveCustomConfiguration(serial, gameinfo, true))
			{
				m_game_list_frame->ShowCustomConfigIcon(gameinfo);
			}
		});
	}
	if (gameinfo->has_custom_pad_config)
	{
		QAction* remove_custom_pad_config = remove_menu->addAction(tr("&Remove Custom Gamepad Configuration"));
		connect(remove_custom_pad_config, &QAction::triggered, this, [this, serial, gameinfo]()
		{
			if (m_game_list_actions->RemoveCustomPadConfiguration(serial, gameinfo, true))
			{
				m_game_list_frame->ShowCustomConfigIcon(gameinfo);
			}
		});
	}

	const std::string cache_base_dir = fs::get_path_if_dir(rpcs3::utils::get_cache_dir_by_serial(serial));
	const bool has_hdd1_cache_dir = !rpcs3::utils::get_dir_list(rpcs3::utils::get_hdd1_cache_dir(), serial).empty();
	const std::string savestates_dir = fs::get_path_if_dir(rpcs3::utils::get_savestates_dir(serial));

	if (!cache_base_dir.empty())
	{
		remove_menu->addSeparator();

		QAction* remove_shader_cache = remove_menu->addAction(tr("&Remove Shader Cache"));
		remove_shader_cache->setEnabled(!is_current_running_game);
		connect(remove_shader_cache, &QAction::triggered, this, [this, serial]()
		{
			m_game_list_actions->RemoveShaderCache(serial, true);
		});

		QAction* remove_ppu_cache = remove_menu->addAction(tr("&Remove PPU Cache"));
		remove_ppu_cache->setEnabled(!is_current_running_game);
		connect(remove_ppu_cache, &QAction::triggered, this, [this, serial]()
		{
			m_game_list_actions->RemovePPUCache(serial, true);
		});

		QAction* remove_spu_cache = remove_menu->addAction(tr("&Remove SPU Cache"));
		remove_spu_cache->setEnabled(!is_current_running_game);
		connect(remove_spu_cache, &QAction::triggered, this, [this, serial]()
		{
			m_game_list_actions->RemoveSPUCache(serial, true);
		});
	}

	if (has_hdd1_cache_dir)
	{
		QAction* remove_hdd1_cache = remove_menu->addAction(tr("&Remove HDD1 Cache"));
		remove_hdd1_cache->setEnabled(!is_current_running_game);
		connect(remove_hdd1_cache, &QAction::triggered, this, [this, serial]()
		{
			m_game_list_actions->RemoveHDD1Cache(serial, true);
		});
	}

	if (!cache_base_dir.empty() || has_hdd1_cache_dir)
	{
		QAction* remove_all_caches = remove_menu->addAction(tr("&Remove All Caches"));
		remove_all_caches->setEnabled(!is_current_running_game);
		connect(remove_all_caches, &QAction::triggered, this, [this, serial]()
		{
			m_game_list_actions->RemoveAllCaches(serial, true);
		});
	}

	if (!savestates_dir.empty())
	{
		remove_menu->addSeparator();

		QAction* remove_savestates = remove_menu->addAction(tr("&Remove Savestates"));
		remove_savestates->setEnabled(!is_current_running_game);
		connect(remove_savestates, &QAction::triggered, this, [this, serial]()
		{
			m_game_list_actions->SetContentList(game_list_actions::content_type::SAVESTATES, {});
			m_game_list_actions->RemoveContentList(serial, true);
		});
	}

	// Disable the Remove menu if empty
	remove_menu->setEnabled(!remove_menu->isEmpty());

	addSeparator();

	// Manage Game menu
	QMenu* manage_game_menu = addMenu(tr("&Manage Game"));

	// Create game shortcuts
	QAction* create_desktop_shortcut = manage_game_menu->addAction(tr("&Create Desktop Shortcut"));
	connect(create_desktop_shortcut, &QAction::triggered, this, [this, gameinfo]()
	{
		m_game_list_actions->CreateShortcuts({gameinfo}, {gui::utils::shortcut_location::desktop});
	});
#ifdef _WIN32
	QAction* create_start_menu_shortcut = manage_game_menu->addAction(tr("&Create Start Menu Shortcut"));
#elif defined(__APPLE__)
	QAction* create_start_menu_shortcut = manage_game_menu->addAction(tr("&Create Launchpad Shortcut"));
#else
	QAction* create_start_menu_shortcut = manage_game_menu->addAction(tr("&Create Application Menu Shortcut"));
#endif
	connect(create_start_menu_shortcut, &QAction::triggered, this, [this, gameinfo]()
	{
		m_game_list_actions->CreateShortcuts({gameinfo}, {gui::utils::shortcut_location::applications});
	});

	manage_game_menu->addSeparator();

	// Hide/rename game in game list
	QAction* hide_serial = manage_game_menu->addAction(tr("&Hide In Game List"));
	hide_serial->setCheckable(true);
	hide_serial->setChecked(m_game_list_frame->hidden_list().contains(QString::fromStdString(serial)));
	QAction* rename_title = manage_game_menu->addAction(tr("&Rename In Game List"));

	// Edit tooltip notes/reset time played
	QAction* edit_notes = manage_game_menu->addAction(tr("&Edit Tooltip Notes"));
	QAction* reset_time_played = manage_game_menu->addAction(tr("&Reset Time Played"));

	manage_game_menu->addSeparator();

	// Remove game
	QAction* remove_game = manage_game_menu->addAction(tr("&Remove %1").arg(gameinfo->localized_category));
	remove_game->setEnabled(!is_current_running_game);

	// Custom Images menu
	QMenu* icon_menu = addMenu(tr("&Custom Images"));
	const std::array<QAction*, 3> custom_icon_actions =
	{
		icon_menu->addAction(tr("&Import Custom Icon")),
		icon_menu->addAction(tr("&Replace Custom Icon")),
		icon_menu->addAction(tr("&Remove Custom Icon"))
	};
	icon_menu->addSeparator();
	const std::array<QAction*, 3> custom_gif_actions =
	{
		icon_menu->addAction(tr("&Import Hover Gif")),
		icon_menu->addAction(tr("&Replace Hover Gif")),
		icon_menu->addAction(tr("&Remove Hover Gif"))
	};
	icon_menu->addSeparator();
	const std::array<QAction*, 3> custom_shader_icon_actions =
	{
		icon_menu->addAction(tr("&Import Custom Shader Loading Background")),
		icon_menu->addAction(tr("&Replace Custom Shader Loading Background")),
		icon_menu->addAction(tr("&Remove Custom Shader Loading Background"))
	};

	if (const std::string custom_icon_dir_path = rpcs3::utils::get_icons_dir(serial);
		fs::create_path(custom_icon_dir_path))
	{
		enum class icon_action
		{
			add,
			replace,
			remove
		};
		enum class icon_type
		{
			game_list,
			hover_gif,
			shader_load
		};

		const auto handle_icon = [this, serial](const QString& game_icon_path, const QString& suffix, icon_action action, icon_type type)
		{
			QString icon_path;

			if (action != icon_action::remove)
			{
				QString msg;
				switch (type)
				{
				case icon_type::game_list:
					msg = tr("Select Custom Icon");
					break;
				case icon_type::hover_gif:
					msg = tr("Select Custom Hover Gif");
					break;
				case icon_type::shader_load:
					msg = tr("Select Custom Shader Loading Background");
					break;
				}
				icon_path = QFileDialog::getOpenFileName(m_game_list_frame, msg, "", tr("%0 (*.%0);;All files (*.*)").arg(suffix));
			}
			if (action == icon_action::remove || !icon_path.isEmpty())
			{
				bool refresh = false;

				QString msg;
				switch (type)
				{
				case icon_type::game_list:
					msg = tr("Remove Custom Icon of %0?").arg(QString::fromStdString(serial));
					break;
				case icon_type::hover_gif:
					msg = tr("Remove Custom Hover Gif of %0?").arg(QString::fromStdString(serial));
					break;
				case icon_type::shader_load:
					msg = tr("Remove Custom Shader Loading Background of %0?").arg(QString::fromStdString(serial));
					break;
				}

				if (action == icon_action::replace || (action == icon_action::remove &&
					QMessageBox::question(m_game_list_frame, tr("Confirm Removal"), msg) == QMessageBox::Yes))
				{
					if (QFile file(game_icon_path); file.exists() && !file.remove())
					{
						game_list_log.error("Could not remove old file: '%s'", game_icon_path, file.errorString());
						QMessageBox::warning(m_game_list_frame, tr("Warning!"), tr("Failed to remove the old file!"));
						return;
					}

					game_list_log.success("Removed file: '%s'", game_icon_path);
					if (action == icon_action::remove)
					{
						refresh = true;
					}
				}

				if (action != icon_action::remove)
				{
					if (!QFile::copy(icon_path, game_icon_path))
					{
						game_list_log.error("Could not import file '%s' to '%s'.", icon_path, game_icon_path);
						QMessageBox::warning(m_game_list_frame, tr("Warning!"), tr("Failed to import the new file!"));
					}
					else
					{
						game_list_log.success("Imported file '%s' to '%s'", icon_path, game_icon_path);
						refresh = true;
					}
				}

				if (refresh)
				{
					m_game_list_frame->Refresh(true);
				}
			}
		};

		const std::vector<std::tuple<icon_type, QString, QString, const std::array<QAction*, 3>&>> icon_map =
		{
			{icon_type::game_list, "/ICON0.PNG", "png", custom_icon_actions},
			{icon_type::hover_gif, "/hover.gif", "gif", custom_gif_actions},
			{icon_type::shader_load, "/PIC1.PNG", "png", custom_shader_icon_actions},
		};

		for (const auto& [type, icon_name, suffix, actions] : icon_map)
		{
			const QString icon_path = QString::fromStdString(custom_icon_dir_path) + icon_name;

			if (QFile::exists(icon_path))
			{
				actions[static_cast<int>(icon_action::add)]->setVisible(false);
				connect(actions[static_cast<int>(icon_action::replace)], &QAction::triggered, m_game_list_frame, [handle_icon, icon_path, t = type, s = suffix] { handle_icon(icon_path, s, icon_action::replace, t); });
				connect(actions[static_cast<int>(icon_action::remove)], &QAction::triggered, m_game_list_frame, [handle_icon, icon_path, t = type, s = suffix] { handle_icon(icon_path, s, icon_action::remove, t); });
			}
			else
			{
				connect(actions[static_cast<int>(icon_action::add)], &QAction::triggered, m_game_list_frame, [handle_icon, icon_path, t = type, s = suffix] { handle_icon(icon_path, s, icon_action::add, t); });
				actions[static_cast<int>(icon_action::replace)]->setVisible(false);
				actions[static_cast<int>(icon_action::remove)]->setEnabled(false);
			}
		}
	}
	else
	{
		game_list_log.error("Could not create path '%s'", custom_icon_dir_path);
		icon_menu->setEnabled(false);
	}

	// Open Folder menu
	QMenu* open_folder_menu = addMenu(tr("&Open Folder"));

	const bool is_disc_game = QString::fromStdString(current_game.category) == cat::cat_disc_game;
	const std::string data_dir = fs::get_path_if_dir(rpcs3::utils::get_data_dir(serial));
	const std::string captures_dir = fs::get_path_if_dir(rpcs3::utils::get_captures_dir());
	const std::string recordings_dir = fs::get_path_if_dir(rpcs3::utils::get_recordings_dir(serial));
	const std::string screenshots_dir = fs::get_path_if_dir(rpcs3::utils::get_screenshots_dir(serial));
	std::set<std::string> data_dir_list;

	if (is_disc_game)
	{
		QAction* open_disc_game_folder = open_folder_menu->addAction(tr("&Open Disc Game Folder"));
		connect(open_disc_game_folder, &QAction::triggered, this, [current_game]()
		{
			gui::utils::open_dir(current_game.path);
		});

		// It could be an empty list for a disc game
		data_dir_list = rpcs3::utils::get_dir_list(rpcs3::utils::get_hdd0_game_dir(), serial);
	}
	else
	{
		data_dir_list.insert(current_game.path);
	}

	if (!data_dir_list.empty()) // "true" if a path is present (it could be an empty list for a disc game)
	{
		QAction* open_data_folder = open_folder_menu->addAction(tr("&Open %0 Folder").arg(is_disc_game ? tr("Game Data") : gameinfo->localized_category));
		connect(open_data_folder, &QAction::triggered, this, [data_dir_list]()
		{
			for (const std::string& data_dir : data_dir_list)
			{
				gui::utils::open_dir(data_dir);
			}
		});
	}

	if (gameinfo->has_custom_config)
	{
		QAction* open_config_folder = open_folder_menu->addAction(tr("&Open Custom Config Folder"));
		connect(open_config_folder, &QAction::triggered, this, [serial]()
		{
			const std::string config_path = rpcs3::utils::get_custom_config_path(serial);

			if (fs::is_file(config_path))
				gui::utils::open_dir(config_path);
		});
	}

	// This is a debug feature, let's hide it by reusing debug tab protection
	if (m_gui_settings->GetValue(gui::m_showDebugTab).toBool() && !cache_base_dir.empty())
	{
		QAction* open_cache_folder = open_folder_menu->addAction(tr("&Open Cache Folder"));
		connect(open_cache_folder, &QAction::triggered, this, [cache_base_dir]()
		{
			gui::utils::open_dir(cache_base_dir);
		});
	}

	if (!data_dir.empty())
	{
		QAction* open_data_folder = open_folder_menu->addAction(tr("&Open Data Folder"));
		connect(open_data_folder, &QAction::triggered, this, [data_dir]()
		{
			gui::utils::open_dir(data_dir);
		});
	}

	if (!savestates_dir.empty())
	{
		QAction* open_savestates_folder = open_folder_menu->addAction(tr("&Open Savestates Folder"));
		connect(open_savestates_folder, &QAction::triggered, this, [savestates_dir]()
		{
			gui::utils::open_dir(savestates_dir);
		});
	}

	if (!captures_dir.empty())
	{
		QAction* open_captures_folder = open_folder_menu->addAction(tr("&Open Captures Folder"));
		connect(open_captures_folder, &QAction::triggered, this, [captures_dir]()
		{
			gui::utils::open_dir(captures_dir);
		});
	}

	if (!recordings_dir.empty())
	{
		QAction* open_recordings_folder = open_folder_menu->addAction(tr("&Open Recordings Folder"));
		connect(open_recordings_folder, &QAction::triggered, this, [recordings_dir]()
		{
			gui::utils::open_dir(recordings_dir);
		});
	}

	if (!screenshots_dir.empty())
	{
		QAction* open_screenshots_folder = open_folder_menu->addAction(tr("&Open Screenshots Folder"));
		connect(open_screenshots_folder, &QAction::triggered, this, [screenshots_dir]()
		{
			gui::utils::open_dir(screenshots_dir);
		});
	}

	// Copy Info menu
	QMenu* info_menu = addMenu(tr("&Copy Info"));
	QAction* copy_info = info_menu->addAction(tr("&Copy Name + Serial"));
	QAction* copy_name = info_menu->addAction(tr("&Copy Name"));
	QAction* copy_serial = info_menu->addAction(tr("&Copy Serial"));

	addSeparator();

	QAction* check_compat = addAction(tr("&Check Game Compatibility"));
	QAction* download_compat = addAction(tr("&Download Compatibility Database"));

	addSeparator();

	// Disk usage
	QAction* disk_usage = addAction(tr("&Disk Usage"));
	connect(disk_usage, &QAction::triggered, this, [this]()
	{
		m_game_list_actions->ShowDiskUsageDialog();
	});

	// Game info
	QAction* game_info = addAction(tr("&Game Info"));
	connect(game_info, &QAction::triggered, this, [this, gameinfo]()
	{
		m_game_list_actions->ShowGameInfoDialog({gameinfo});
	});

	connect(boot, &QAction::triggered, m_game_list_frame, [this, gameinfo]()
	{
		sys_log.notice("Booting from gamelist per context menu...");
		Q_EMIT m_game_list_frame->RequestBoot(gameinfo, cfg_mode::global);
	});

	auto configure_l = [this, current_game, gameinfo](bool create_cfg_from_global_cfg)
	{
		settings_dialog dlg(m_gui_settings, m_emu_settings, 0, m_game_list_frame, &current_game, create_cfg_from_global_cfg);

		connect(&dlg, &settings_dialog::EmuSettingsApplied, [this, gameinfo]()
		{
			if (!gameinfo->has_custom_config)
			{
				gameinfo->has_custom_config = true;
				m_game_list_frame->ShowCustomConfigIcon(gameinfo);
			}
			Q_EMIT m_game_list_frame->NotifyEmuSettingsChange();
		});

		dlg.exec();
	};

	if (create_game_default_config)
	{
		connect(configure, &QAction::triggered, m_game_list_frame, [configure_l]() { configure_l(true); });
		connect(create_game_default_config, &QAction::triggered, m_game_list_frame, [configure_l = std::move(configure_l)]() { configure_l(false); });
	}
	else
	{
		connect(configure, &QAction::triggered, m_game_list_frame, [configure_l = std::move(configure_l)]() { configure_l(true); });
	}

	connect(pad_configure, &QAction::triggered, m_game_list_frame, [this, current_game, gameinfo]()
	{
		pad_settings_dialog dlg(m_gui_settings, m_game_list_frame, &current_game);

		if (dlg.exec() == QDialog::Accepted && !gameinfo->has_custom_pad_config)
		{
			gameinfo->has_custom_pad_config = true;
			m_game_list_frame->ShowCustomConfigIcon(gameinfo);
		}
	});
	connect(hide_serial, &QAction::triggered, m_game_list_frame, [this, serial = QString::fromStdString(serial)](bool checked)
	{
		if (checked)
			m_game_list_frame->hidden_list().insert(serial);
		else
			m_game_list_frame->hidden_list().remove(serial);

		m_gui_settings->SetValue(gui::gl_hidden_list, QStringList(m_game_list_frame->hidden_list().values()));
		m_game_list_frame->Refresh();
	});
	connect(create_cpu_cache, &QAction::triggered, m_game_list_frame, [this, gameinfo]
	{
		if (m_gui_settings->GetBootConfirmation(m_game_list_frame))
		{
			m_game_list_actions->CreateCPUCaches(gameinfo);
		}
	});
	connect(remove_game, &QAction::triggered, this, [this, gameinfo]
	{
		m_game_list_actions->ShowRemoveGameDialog({gameinfo});
	});
	connect(configure_patches, &QAction::triggered, m_game_list_frame, [this, gameinfo]()
	{
		patch_manager_dialog patch_manager(m_gui_settings, m_game_list_frame->GetGameInfo(), gameinfo->info.serial, gameinfo->GetGameVersion(), m_game_list_frame);
		patch_manager.exec();
	});
	connect(check_compat, &QAction::triggered, this, [serial = QString::fromStdString(serial)]
	{
		const QString link = "https://rpcs3.net/compatibility?g=" + serial;
		QDesktopServices::openUrl(QUrl(link));
	});
	connect(download_compat, &QAction::triggered, m_game_list_frame, [this]
	{
		ensure(m_game_list_frame->GetGameCompatibility())->RequestCompatibility(true);
	});
	connect(rename_title, &QAction::triggered, m_game_list_frame, [this, name, serial = QString::fromStdString(serial), global_pos]
	{
		const QString custom_title = m_persistent_settings->GetValue(gui::persistent::titles, serial, "").toString();
		const QString old_title = custom_title.isEmpty() ? name : custom_title;

		input_dialog dlg(128, old_title, tr("Rename Title"), tr("%0\n%1\n\nYou can clear the line in order to use the original title.").arg(name).arg(serial), name, m_game_list_frame);
		dlg.move(global_pos);

		if (dlg.exec() == QDialog::Accepted)
		{
			const QString new_title = dlg.get_input_text().simplified();

			if (new_title.isEmpty() || new_title == name)
			{
				m_game_list_frame->titles().erase(serial);
				m_persistent_settings->RemoveValue(gui::persistent::titles, serial);
			}
			else
			{
				m_game_list_frame->titles().insert_or_assign(serial, new_title);
				m_persistent_settings->SetValue(gui::persistent::titles, serial, new_title);
			}
			m_game_list_frame->Refresh(true); // full refresh in order to reliably sort the list
		}
	});
	connect(edit_notes, &QAction::triggered, m_game_list_frame, [this, name, serial = QString::fromStdString(serial)]
	{
		bool accepted = false;
		const QString old_notes = m_persistent_settings->GetValue(gui::persistent::notes, serial, "").toString();
		const QString new_notes = QInputDialog::getMultiLineText(m_game_list_frame, tr("Edit Tooltip Notes"), tr("%0\n%1").arg(name).arg(serial), old_notes, &accepted);

		if (accepted)
		{
			if (new_notes.simplified().isEmpty())
			{
				m_game_list_frame->notes().erase(serial);
				m_persistent_settings->RemoveValue(gui::persistent::notes, serial);
			}
			else
			{
				m_game_list_frame->notes().insert_or_assign(serial, new_notes);
				m_persistent_settings->SetValue(gui::persistent::notes, serial, new_notes);
			}
			m_game_list_frame->Refresh();
		}
	});
	connect(reset_time_played, &QAction::triggered, m_game_list_frame, [this, name, serial = QString::fromStdString(serial)]
	{
		if (QMessageBox::question(m_game_list_frame, tr("Confirm Reset"), tr("Reset time played?\n\n%0 [%1]").arg(name).arg(serial)) == QMessageBox::Yes)
		{
			m_persistent_settings->SetPlaytime(serial, 0, false);
			m_persistent_settings->SetLastPlayed(serial, 0, true);
			m_game_list_frame->Refresh();
		}
	});
	connect(copy_info, &QAction::triggered, this, [name, serial = QString::fromStdString(serial)]
	{
		QApplication::clipboard()->setText(name % QStringLiteral(" [") % serial % QStringLiteral("]"));
	});
	connect(copy_name, &QAction::triggered, this, [name]
	{
		QApplication::clipboard()->setText(name);
	});
	connect(copy_serial, &QAction::triggered, this, [serial = QString::fromStdString(serial)]
	{
		QApplication::clipboard()->setText(serial);
	});

	// Disable options depending on software category
	const QString category = QString::fromStdString(current_game.category);

	if (category == cat::cat_ps3_os)
	{
		remove_game->setEnabled(false);
	}
	else if (category != cat::cat_disc_game && category != cat::cat_hdd_game)
	{
		check_compat->setEnabled(false);
	}

	exec(global_pos);
}

void game_list_context_menu::show_multi_selection_context_menu(const std::vector<game_info>& games, const QPoint& global_pos)
{
	ensure(!games.empty());

	// Create LLVM cache
	QAction* create_cpu_cache = addAction(tr("&Create LLVM Cache"));
	connect(create_cpu_cache, &QAction::triggered, this, [this, games]()
	{
		m_game_list_actions->BatchCreateCPUCaches(games, false, true);
	});

	// Remove menu
	QMenu* remove_menu = addMenu(tr("&Remove"));

	QAction* remove_custom_config = remove_menu->addAction(tr("&Remove Custom Configuration"));
	connect(remove_custom_config, &QAction::triggered, this, [this, games]()
	{
		m_game_list_actions->BatchRemoveCustomConfigurations(games, true);
	});

	QAction* remove_custom_pad_config = remove_menu->addAction(tr("&Remove Custom Gamepad Configuration"));
	connect(remove_custom_pad_config, &QAction::triggered, this, [this, games]()
	{
		m_game_list_actions->BatchRemoveCustomPadConfigurations(games, true);
	});

	remove_menu->addSeparator();

	QAction* remove_shader_cache = remove_menu->addAction(tr("&Remove Shader Cache"));
	connect(remove_shader_cache, &QAction::triggered, this, [this, games]()
	{
		m_game_list_actions->BatchRemoveShaderCaches(games, true);
	});

	QAction* remove_ppu_cache = remove_menu->addAction(tr("&Remove PPU Cache"));
	connect(remove_ppu_cache, &QAction::triggered, this, [this, games]()
	{
		m_game_list_actions->BatchRemovePPUCaches(games, true);
	});

	QAction* remove_spu_cache = remove_menu->addAction(tr("&Remove SPU Cache"));
	connect(remove_spu_cache, &QAction::triggered, this, [this, games]()
	{
		m_game_list_actions->BatchRemoveSPUCaches(games, true);
	});

	QAction* remove_hdd1_cache = remove_menu->addAction(tr("&Remove HDD1 Cache"));
	connect(remove_hdd1_cache, &QAction::triggered, this, [this, games]()
	{
		m_game_list_actions->BatchRemoveHDD1Caches(games, true);
	});

	QAction* remove_all_caches = remove_menu->addAction(tr("&Remove All Caches"));
	connect(remove_all_caches, &QAction::triggered, this, [this, games]()
	{
		m_game_list_actions->BatchRemoveAllCaches(games, true);
	});

	remove_menu->addSeparator();

	QAction* remove_savestates = remove_menu->addAction(tr("&Remove Savestates"));
	connect(remove_savestates, &QAction::triggered, this, [this, games]()
	{
		m_game_list_actions->SetContentList(game_list_actions::content_type::SAVESTATES, {});
		m_game_list_actions->BatchRemoveContentLists(games, true);
	});

	// Disable the Remove menu if empty
	remove_menu->setEnabled(!remove_menu->isEmpty());

	addSeparator();

	// Manage Game menu
	QMenu* manage_game_menu = addMenu(tr("&Manage Game"));

	// Create game shortcuts
	QAction* create_desktop_shortcut = manage_game_menu->addAction(tr("&Create Desktop Shortcut"));
	connect(create_desktop_shortcut, &QAction::triggered, m_game_list_frame, [this, games]()
	{
		if (QMessageBox::question(m_game_list_frame, tr("Confirm Creation"), tr("Create desktop shortcut?")) != QMessageBox::Yes)
			return;

		m_game_list_actions->CreateShortcuts(games, {gui::utils::shortcut_location::desktop});
	});

#ifdef _WIN32
	QAction* create_start_menu_shortcut = manage_game_menu->addAction(tr("&Create Start Menu Shortcut"));
#elif defined(__APPLE__)
	QAction* create_start_menu_shortcut = manage_game_menu->addAction(tr("&Create Launchpad Shortcut"));
#else
	QAction* create_start_menu_shortcut = manage_game_menu->addAction(tr("&Create Application Menu Shortcut"));
#endif
	connect(create_start_menu_shortcut, &QAction::triggered, m_game_list_frame, [this, games]()
	{
		if (QMessageBox::question(m_game_list_frame, tr("Confirm Creation"), tr("Create shortcut?")) != QMessageBox::Yes)
			return;

		m_game_list_actions->CreateShortcuts(games, {gui::utils::shortcut_location::applications});
	});

	manage_game_menu->addSeparator();

	// Hide game in game list
	QAction* hide_serial = manage_game_menu->addAction(tr("&Hide In Game List"));
	connect(hide_serial, &QAction::triggered, m_game_list_frame, [this, games]()
	{
		if (QMessageBox::question(m_game_list_frame, tr("Confirm Hiding"), tr("Hide in game list?")) != QMessageBox::Yes)
			return;

		for (const auto& game : games)
		{
			m_game_list_frame->hidden_list().insert(QString::fromStdString(game->info.serial));
		}

		m_gui_settings->SetValue(gui::gl_hidden_list, QStringList(m_game_list_frame->hidden_list().values()));
		m_game_list_frame->Refresh();
	});

	// Show game in game list
	QAction* show_serial = manage_game_menu->addAction(tr("&Show In Game List"));
	connect(show_serial, &QAction::triggered, m_game_list_frame, [this, games]()
	{
		for (const auto& game : games)
		{
			m_game_list_frame->hidden_list().remove(QString::fromStdString(game->info.serial));
		}

		m_gui_settings->SetValue(gui::gl_hidden_list, QStringList(m_game_list_frame->hidden_list().values()));
		m_game_list_frame->Refresh();
	});

	manage_game_menu->addSeparator();

	// Reset time played
	QAction* reset_time_played = manage_game_menu->addAction(tr("&Reset Time Played"));
	connect(reset_time_played, &QAction::triggered, m_game_list_frame, [this, games]()
	{
		if (QMessageBox::question(m_game_list_frame, tr("Confirm Reset"), tr("Reset time played?")) != QMessageBox::Yes)
			return;

		for (const auto& game : games)
		{
			const auto serial = QString::fromStdString(game->info.serial);

			m_persistent_settings->SetPlaytime(serial, 0, false);
			m_persistent_settings->SetLastPlayed(serial, 0, true);
		}

		m_game_list_frame->Refresh();
	});

	manage_game_menu->addSeparator();

	// Remove game
	QAction* remove_game = manage_game_menu->addAction(tr("&Remove Game"));
	connect(remove_game, &QAction::triggered, this, [this, games]()
	{
		m_game_list_actions->ShowRemoveGameDialog(games);
	});

	addSeparator();

	QAction* download_compat = addAction(tr("&Download Compatibility Database"));
	connect(download_compat, &QAction::triggered, m_game_list_frame, [this]
	{
		ensure(m_game_list_frame->GetGameCompatibility())->RequestCompatibility(true);
	});

	addSeparator();

	// Disk usage
	QAction* disk_usage = addAction(tr("&Disk Usage"));
	connect(disk_usage, &QAction::triggered, this, [this]()
	{
		m_game_list_actions->ShowDiskUsageDialog();
	});

	// Game info
	QAction* game_info = addAction(tr("&Game Info"));
	connect(game_info, &QAction::triggered, this, [this, games]()
	{
		m_game_list_actions->ShowGameInfoDialog(games);
	});

	exec(global_pos);
}
