#include "game_list_frame.h"
#include "game_list_context_menu.h"
#include "qt_utils.h"
#include "localized.h"
#include "progress_dialog.h"
#include "persistent_settings.h"
#include "emu_settings.h"
#include "gui_settings.h"
#include "gui_application.h"
#include "game_list_table.h"
#include "game_list_grid.h"
#include "game_list_grid_item.h"

#include "Emu/System.h"
#include "Emu/vfs_config.h"
#include "Emu/system_utils.hpp"
#include "Loader/PSF.h"
#include "Loader/ISO.h"
#include "util/types.hpp"
#include "Utilities/File.h"
#include "util/sysinfo.hpp"

#include <algorithm>
#include <memory>
#include <regex>
#include <unordered_set>

#include <QtConcurrent>
#include <QHeaderView>
#include <QMessageBox>
#include <QScrollBar>
#include <QApplication>

LOG_CHANNEL(game_list_log, "GameList");
LOG_CHANNEL(sys_log, "SYS");

game_list_frame::game_list_frame(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<emu_settings> emu_settings, std::shared_ptr<persistent_settings> persistent_settings, QWidget* parent)
	: custom_dock_widget(tr("Game List"), parent)
	, m_gui_settings(std::move(gui_settings))
	, m_emu_settings(std::move(emu_settings))
	, m_persistent_settings(std::move(persistent_settings))
{
	m_game_list_actions = std::make_shared<game_list_actions>(this, m_gui_settings);

	m_icon_size       = gui::gl_icon_size_min; // ensure a valid size
	m_is_list_layout  = m_gui_settings->GetValue(gui::gl_listMode).toBool();
	m_margin_factor   = m_gui_settings->GetValue(gui::gl_marginFactor).toReal();
	m_text_factor     = m_gui_settings->GetValue(gui::gl_textFactor).toReal();
	m_icon_color      = m_gui_settings->GetValue(gui::gl_iconColor).value<QColor>();
	m_col_sort_order  = m_gui_settings->GetValue(gui::gl_sortAsc).toBool() ? Qt::AscendingOrder : Qt::DescendingOrder;
	m_sort_column     = m_gui_settings->GetValue(gui::gl_sortCol).toInt();
	m_hidden_list     = gui::utils::list_to_set(m_gui_settings->GetValue(gui::gl_hidden_list).toStringList());

	m_old_layout_is_list = m_is_list_layout;

	// Save factors for first setup
	m_gui_settings->SetValue(gui::gl_iconColor, m_icon_color, false);
	m_gui_settings->SetValue(gui::gl_marginFactor, m_margin_factor, false);
	m_gui_settings->SetValue(gui::gl_textFactor, m_text_factor, true);

	m_game_dock = new QMainWindow(this);
	m_game_dock->setWindowFlags(Qt::Widget);
	setWidget(m_game_dock);

	m_game_grid = new game_list_grid();
	m_game_grid->installEventFilter(this);
	m_game_grid->scroll_area()->verticalScrollBar()->installEventFilter(this);

	m_game_list = new game_list_table(this, m_persistent_settings);
	m_game_list->installEventFilter(this);
	m_game_list->verticalScrollBar()->installEventFilter(this);

	m_game_compat = new game_compatibility(m_gui_settings, this);

	m_central_widget = new QStackedWidget(this);
	m_central_widget->addWidget(m_game_list);
	m_central_widget->addWidget(m_game_grid);

	if (m_is_list_layout)
	{
		m_central_widget->setCurrentWidget(m_game_list);
	}
	else
	{
		m_central_widget->setCurrentWidget(m_game_grid);
	}

	m_game_dock->setCentralWidget(m_central_widget);

	// Actions regarding showing/hiding columns
	const auto add_column = [this](gui::game_list_columns col)
	{
		const int column = static_cast<int>(col);
		m_game_list->setHorizontalHeaderItem(column, new QTableWidgetItem(get_header_text(column)));
		m_column_acts[column] = new QAction(get_action_text(column), this);
	};

	add_column(gui::game_list_columns::icon);
	add_column(gui::game_list_columns::name);
	add_column(gui::game_list_columns::serial);
	add_column(gui::game_list_columns::firmware);
	add_column(gui::game_list_columns::version);
	add_column(gui::game_list_columns::category);
	add_column(gui::game_list_columns::path);
	add_column(gui::game_list_columns::move);
	add_column(gui::game_list_columns::resolution);
	add_column(gui::game_list_columns::sound);
	add_column(gui::game_list_columns::parental);
	add_column(gui::game_list_columns::last_play);
	add_column(gui::game_list_columns::playtime);
	add_column(gui::game_list_columns::compat);
	add_column(gui::game_list_columns::dir_size);

	m_progress_dialog = new progress_dialog(tr("Loading games"), tr("Loading games, please wait..."), tr("Cancel"), 0, 0, false, this, Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
	m_progress_dialog->setMinimumDuration(200); // Only show the progress dialog after some time has passed

	// Events
	connect(m_progress_dialog, &QProgressDialog::canceled, this, [this]()
	{
		gui::utils::stop_future_watcher(m_parsing_watcher, true);
		gui::utils::stop_future_watcher(m_refresh_watcher, true);

		m_path_entries.clear();
		m_path_list.clear();
		m_serials.clear();
		m_game_data.clear();
		m_notes.clear();
		m_games.pop_all();
	});
	connect(&m_parsing_watcher, &QFutureWatcher<void>::finished, this, &game_list_frame::OnParsingFinished);
	connect(&m_parsing_watcher, &QFutureWatcher<void>::canceled, this, [this]()
	{
		WaitAndAbortSizeCalcThreads();
		WaitAndAbortRepaintThreads();

		m_path_entries.clear();
		m_path_list.clear();
		m_game_data.clear();
		m_serials.clear();
		m_games.pop_all();
	});
	connect(&m_refresh_watcher, &QFutureWatcher<void>::finished, this, &game_list_frame::OnRefreshFinished);
	connect(&m_refresh_watcher, &QFutureWatcher<void>::canceled, this, [this]()
	{
		WaitAndAbortSizeCalcThreads();
		WaitAndAbortRepaintThreads();

		m_path_entries.clear();
		m_path_list.clear();
		m_game_data.clear();
		m_serials.clear();
		m_games.pop_all();

		if (m_progress_dialog)
		{
			m_progress_dialog->accept();
		}
	});
	connect(&m_refresh_watcher, &QFutureWatcher<void>::progressRangeChanged, this, [this](int minimum, int maximum)
	{
		if (m_progress_dialog)
		{
			m_progress_dialog->SetRange(minimum, maximum);
		}
	});
	connect(&m_refresh_watcher, &QFutureWatcher<void>::progressValueChanged, this, [this](int value)
	{
		if (m_progress_dialog)
		{
			m_progress_dialog->SetValue(value);
		}
	});

	connect(m_game_list, &QTableWidget::customContextMenuRequested, this, &game_list_frame::ShowContextMenu);
	connect(m_game_list, &QTableWidget::itemSelectionChanged, this, &game_list_frame::ItemSelectionChangedSlot);
	connect(m_game_list, &QTableWidget::itemDoubleClicked, this, qOverload<QTableWidgetItem*>(&game_list_frame::doubleClickedSlot));

	connect(m_game_list->horizontalHeader(), &QHeaderView::sectionClicked, this, &game_list_frame::OnColClicked);

	connect(m_game_grid, &QWidget::customContextMenuRequested, this, &game_list_frame::ShowContextMenu);
	connect(m_game_grid, &game_list_grid::ItemSelectionChanged, this, &game_list_frame::NotifyGameSelection);
	connect(m_game_grid, &game_list_grid::ItemDoubleClicked, this, qOverload<const game_info&>(&game_list_frame::doubleClickedSlot));

	connect(m_game_compat, &game_compatibility::DownloadStarted, this, [this]()
	{
		for (const auto& game : m_game_data)
		{
			game->compat = m_game_compat->GetStatusData("Download");
		}
		Refresh();
	});
	connect(m_game_compat, &game_compatibility::DownloadFinished, this, &game_list_frame::OnCompatFinished);
	connect(m_game_compat, &game_compatibility::DownloadCanceled, this, &game_list_frame::OnCompatFinished);
	connect(m_game_compat, &game_compatibility::DownloadError, this, [this](const QString& error)
	{
		OnCompatFinished();
		QMessageBox::warning(this, tr("Warning!"), tr("Failed to retrieve the online compatibility database!\nFalling back to local database.\n\n%0").arg(error));
	});

	connect(m_game_list, &game_list::FocusToSearchBar, this, &game_list_frame::FocusToSearchBar);
	connect(m_game_grid, &game_list_grid::FocusToSearchBar, this, &game_list_frame::FocusToSearchBar);

	m_game_list->create_header_actions(m_column_acts,
		[this](int col) { return m_gui_settings->GetGamelistColVisibility(static_cast<gui::game_list_columns>(col)); },
		[this](int col, bool visible) { m_gui_settings->SetGamelistColVisibility(static_cast<gui::game_list_columns>(col), visible); });
}

void game_list_frame::LoadSettings()
{
	m_col_sort_order = m_gui_settings->GetValue(gui::gl_sortAsc).toBool() ? Qt::AscendingOrder : Qt::DescendingOrder;
	m_sort_column = m_gui_settings->GetValue(gui::gl_sortCol).toInt();
	m_category_filters = m_gui_settings->GetGameListCategoryFilters(true);
	m_grid_category_filters = m_gui_settings->GetGameListCategoryFilters(false);
	m_draw_compat_status_to_grid = m_gui_settings->GetValue(gui::gl_draw_compat).toBool();
	m_prefer_game_data_icons = m_gui_settings->GetValue(gui::gl_pref_gd_icon).toBool();
	m_show_custom_icons = m_gui_settings->GetValue(gui::gl_custom_icon).toBool();
	m_play_hover_movies = m_gui_settings->GetValue(gui::gl_hover_gifs).toBool();

	m_game_list->sync_header_actions(m_column_acts, [this](int col) { return m_gui_settings->GetGamelistColVisibility(static_cast<gui::game_list_columns>(col)); });
}

game_list_frame::~game_list_frame()
{
	WaitAndAbortSizeCalcThreads();
	WaitAndAbortRepaintThreads();
	gui::utils::stop_future_watcher(m_parsing_watcher, true);
	gui::utils::stop_future_watcher(m_refresh_watcher, true);
}

QString game_list_frame::get_header_text(int col) const
{
	switch (static_cast<gui::game_list_columns>(col))
	{
	case gui::game_list_columns::icon:       return tr("Icon");
	case gui::game_list_columns::name:       return tr("Name");
	case gui::game_list_columns::serial:     return tr("Serial");
	case gui::game_list_columns::firmware:   return tr("Firmware");
	case gui::game_list_columns::version:    return tr("Version");
	case gui::game_list_columns::category:   return tr("Category");
	case gui::game_list_columns::path:       return tr("Path");
	case gui::game_list_columns::move:       return tr("PlayStation Move");
	case gui::game_list_columns::resolution: return tr("Supported Resolutions");
	case gui::game_list_columns::sound:      return tr("Sound Formats");
	case gui::game_list_columns::parental:   return tr("Parental Level");
	case gui::game_list_columns::last_play:  return tr("Last Played");
	case gui::game_list_columns::playtime:   return tr("Time Played");
	case gui::game_list_columns::compat:     return tr("Compatibility");
	case gui::game_list_columns::dir_size:   return tr("Space On Disk");
	case gui::game_list_columns::count:      break;
	}
	return {};
}

QString game_list_frame::get_action_text(int col) const
{
	switch (static_cast<gui::game_list_columns>(col))
	{
	case gui::game_list_columns::icon:       return tr("Show Icons");
	case gui::game_list_columns::name:       return tr("Show Names");
	case gui::game_list_columns::serial:     return tr("Show Serials");
	case gui::game_list_columns::firmware:   return tr("Show Firmwares");
	case gui::game_list_columns::version:    return tr("Show Versions");
	case gui::game_list_columns::category:   return tr("Show Categories");
	case gui::game_list_columns::path:       return tr("Show Paths");
	case gui::game_list_columns::move:       return tr("Show PlayStation Move");
	case gui::game_list_columns::resolution: return tr("Show Supported Resolutions");
	case gui::game_list_columns::sound:      return tr("Show Sound Formats");
	case gui::game_list_columns::parental:   return tr("Show Parental Levels");
	case gui::game_list_columns::last_play:  return tr("Show Last Played");
	case gui::game_list_columns::playtime:   return tr("Show Time Played");
	case gui::game_list_columns::compat:     return tr("Show Compatibility");
	case gui::game_list_columns::dir_size:   return tr("Show Space On Disk");
	case gui::game_list_columns::count:      break;
	}
	return {};
}

void game_list_frame::OnColClicked(int col)
{
	if (col == static_cast<int>(gui::game_list_columns::icon)) return; // Don't "sort" icons.

	if (col == m_sort_column)
	{
		m_col_sort_order = (m_col_sort_order == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
	}
	else
	{
		m_col_sort_order = Qt::AscendingOrder;
	}
	m_sort_column = col;

	m_gui_settings->SetValue(gui::gl_sortAsc, m_col_sort_order == Qt::AscendingOrder, false);
	m_gui_settings->SetValue(gui::gl_sortCol, col, true);

	m_game_list->sort(m_game_data.size(), m_sort_column, m_col_sort_order);
}

// Get visibility of entries
bool game_list_frame::IsEntryVisible(const game_info& game, bool search_fallback) const
{
	const auto matches_category = [&]()
	{
		if (m_is_list_layout)
		{
			return m_category_filters.contains(QString::fromStdString(game->info.category));
		}

		return m_grid_category_filters.contains(QString::fromStdString(game->info.category));
	};

	const QString serial = QString::fromStdString(game->info.serial);
	const bool is_visible = m_show_hidden || !m_hidden_list.contains(serial);
	return is_visible && matches_category() && SearchMatchesApp(QString::fromStdString(game->info.name), serial, search_fallback);
}

void game_list_frame::push_path(const std::string& path, std::vector<std::string>& legit_paths)
{
	{
		std::lock_guard lock(m_path_mutex);
		if (!m_path_list.insert(path).second)
		{
			return;
		}
	}
	legit_paths.push_back(path);
}

void game_list_frame::Refresh(const bool from_drive, const std::vector<std::string>& serials_to_remove_from_yml, const bool scroll_after)
{
	if (from_drive)
	{
		WaitAndAbortSizeCalcThreads();
	}
	WaitAndAbortRepaintThreads();
	gui::utils::stop_future_watcher(m_parsing_watcher, from_drive);
	gui::utils::stop_future_watcher(m_refresh_watcher, from_drive);

	if (m_progress_dialog && m_progress_dialog->isVisible())
	{
		m_progress_dialog->SetValue(m_progress_dialog->maximum());
		m_progress_dialog->accept();
	}

	if (from_drive)
	{
		m_path_entries.clear();
		m_path_list.clear();
		m_serials.clear();
		m_game_data.clear();
		m_notes.clear();
		m_games.pop_all();

		if (m_progress_dialog)
		{
			m_progress_dialog->SetValue(0);
		}

		// Update headers
		for (int col = 0; col < m_game_list->horizontalHeader()->count(); col++)
		{
			if (auto item = m_game_list->horizontalHeaderItem(col))
			{
				item->setText(get_header_text(col));
			}
		}

		// Update actions
		for (auto& [col, action] : m_column_acts)
		{
			action->setText(get_action_text(col));
		}

		const std::string games_dir = rpcs3::utils::get_games_dir();

		// Remove the specified and detected serials (title id) belonging to "games_dir" folder only from the game list in memory
		// (not yet in "games.yml" file)
		Emu.RemoveGamesFromDir(games_dir, serials_to_remove_from_yml, false);

		// Scan auto-detection "games" folder adding the detected titles to the game list plus flushing also all the other changes in "games.yml" file
		if (const u32 games_added = Emu.AddGamesFromDir(games_dir); games_added != 0)
		{
			game_list_log.notice("Refresh added %d new entries found in %s", games_added, games_dir);
		}

		const std::string _hdd = Emu.GetCallbacks().resolve_path(rpcs3::utils::get_hdd0_dir()) + '/';

		m_parsing_watcher.setFuture(QtConcurrent::map(m_parsing_threads, [this, _hdd](int index)
		{
			if (index > 0)
			{
				game_list_log.error("Unexpected thread index: %d", index);
				return;
			}

			const auto add_dir = [this](const std::string& path, bool is_disc)
			{
				for (const auto& entry : fs::dir(path))
				{
					if (m_parsing_watcher.isCanceled())
					{
						break;
					}

					if (!entry.is_directory || entry.name == "." || entry.name == "..")
					{
						continue;
					}

					std::lock_guard lock(m_path_mutex);
					m_path_entries.emplace_back(path_entry{path + entry.name, is_disc, false});
				}
			};

			const std::string hdd0_game = _hdd + "game/";

			add_dir(hdd0_game, false);
			add_dir(_hdd + "disc/", true); // Deprecated

			for (const auto& [serial, path] : Emu.GetGamesConfig().get_games())
			{
				if (m_parsing_watcher.isCanceled())
				{
					break;
				}

				std::string game_dir = path;
				game_dir.resize(game_dir.find_last_not_of('/') + 1);

				if (game_dir.empty() || path.starts_with(hdd0_game))
				{
					continue;
				}

				// Don't use the C00 subdirectory in our game list
				if (game_dir.ends_with("/C00") || game_dir.ends_with("\\C00"))
				{
					game_dir = game_dir.substr(0, game_dir.size() - 4);
				}

				std::lock_guard lock(m_path_mutex);
				m_path_entries.emplace_back(path_entry{game_dir, false, true});
			}
		}));

		return;
	}

	// Fill Game List / Game Grid

	const std::set<std::string> selected_items = CurrentSelectionPaths();

	// Release old data
	for (const auto& game : m_game_data)
	{
		game->item = nullptr;
	}

	// Get list of matching apps
	std::vector<game_info> matching_apps;

	for (const auto& app : m_game_data)
	{
		if (IsEntryVisible(app))
		{
			matching_apps.push_back(app);
		}
	}

	// Fallback is not needed when at least one entry is visible
	if (matching_apps.empty())
	{
		for (const auto& app : m_game_data)
		{
			if (IsEntryVisible(app, true))
			{
				matching_apps.push_back(app);
			}
		}
	}

	if (m_is_list_layout)
	{
		m_game_grid->clear_list();
		const int scroll_position = m_game_list->verticalScrollBar()->value();
		m_game_list->populate(matching_apps, m_notes, m_titles, selected_items, m_play_hover_movies);
		m_game_list->sort(m_game_data.size(), m_sort_column, m_col_sort_order);
		RepaintIcons();

		if (scroll_after)
		{
			m_game_list->scrollTo(m_game_list->currentIndex(), QAbstractItemView::PositionAtCenter);
		}
		else
		{
			m_game_list->verticalScrollBar()->setValue(scroll_position);
		}
	}
	else
	{
		m_game_list->clear_list();
		m_game_grid->populate(matching_apps, m_notes, m_titles, selected_items, m_play_hover_movies);
		RepaintIcons();
	}
}

void game_list_frame::OnParsingFinished()
{
	const Localized localized;
	const std::string dev_flash = g_cfg_vfs.get_dev_flash();
	const std::string _hdd = rpcs3::utils::get_hdd0_dir();

	m_path_entries.emplace_back(path_entry{dev_flash + "vsh/module/vsh.self", false, false});

	// Remove duplicates
	sort(m_path_entries.begin(), m_path_entries.end(), [](const path_entry& l, const path_entry& r){return l.path < r.path;});
	m_path_entries.erase(unique(m_path_entries.begin(), m_path_entries.end(), [](const path_entry& l, const path_entry& r){return l.path == r.path;}), m_path_entries.end());

	const s32 language_index = gui_application::get_language_id();
	const std::string game_icon_path = fs::get_config_dir() + "/Icons/game_icons/";
	const std::string localized_title = fmt::format("TITLE_%02d", language_index);
	const std::string localized_icon = fmt::format("ICON0_%02d.PNG", language_index);
	const std::string localized_movie = fmt::format("ICON1_%02d.PAM", language_index);

	const auto add_game = [this, localized_title, localized_icon, localized_movie, dev_flash, cat_unknown_localized = localized.category.unknown.toStdString(), cat_unknown = cat::cat_unknown.toStdString(), game_icon_path, _hdd, play_hover_movies = m_play_hover_movies, show_custom_icons = m_show_custom_icons](const std::string& dir_or_elf)
	{
		std::unique_ptr<iso_archive> archive;
		if (is_file_iso(dir_or_elf))
		{
			archive = std::make_unique<iso_archive>(dir_or_elf);
		}

		const auto file_exists = [&archive](const std::string& path)
		{
			return archive ? archive->is_file(path) : fs::is_file(path);
		};

		gui_game_info game{};
		game.info.path = dir_or_elf;

		const Localized thread_localized;

		const std::string sfo_dir = archive ? "PS3_GAME" : rpcs3::utils::get_sfo_dir_from_game_path(dir_or_elf);
		const std::string sfo_path = sfo_dir + "/PARAM.SFO";

		const psf::registry psf = archive ? archive->open_psf(sfo_path) : psf::load_object(sfo_path);
		const std::string_view title_id = psf::get_string(psf, "TITLE_ID", "");

		if (title_id.empty())
		{
			if (!fs::is_file(dir_or_elf))
			{
				// Do not care about invalid entries
				return;
			}

			game.info.serial = dir_or_elf.substr(dir_or_elf.find_last_of(fs::delim) + 1);
			game.info.category = cat::cat_ps3_os.toStdString(); // Key for operating system executables
			game.info.version = utils::get_firmware_version();
			game.info.app_ver = game.info.version;
			game.info.fw = game.info.version;
			game.info.bootable = 1;
			game.info.icon_path = dev_flash + "vsh/resource/explore/icon/icon_home.png";

			if (dir_or_elf.starts_with(dev_flash))
			{
				std::string path_vfs = dir_or_elf.substr(dev_flash.size());

				if (const usz pos = path_vfs.find_first_not_of(fs::delim); pos != umax && pos != 0)
				{
					path_vfs = path_vfs.substr(pos);
				}

				if (const auto it = thread_localized.title.titles.find(path_vfs); it != thread_localized.title.titles.cend())
				{
					game.info.name = it->second.toStdString();
				}
			}

			if (game.info.name.empty())
			{
				game.info.name = game.info.serial;
			}
		}
		else
		{
			std::string_view name = psf::get_string(psf, localized_title);
			if (name.empty()) name = psf::get_string(psf, "TITLE", cat_unknown_localized);

			game.info.serial       = std::string(title_id);
			game.info.name         = std::string(name);
			game.info.app_ver      = std::string(psf::get_string(psf, "APP_VER", cat_unknown_localized));
			game.info.version      = std::string(psf::get_string(psf, "VERSION", cat_unknown_localized));
			game.info.category     = std::string(psf::get_string(psf, "CATEGORY", cat_unknown));
			game.info.fw           = std::string(psf::get_string(psf, "PS3_SYSTEM_VER", cat_unknown_localized));
			game.info.parental_lvl = psf::get_integer(psf, "PARENTAL_LEVEL", 0);
			game.info.resolution   = psf::get_integer(psf, "RESOLUTION", 0);
			game.info.sound_format = psf::get_integer(psf, "SOUND_FORMAT", 0);
			game.info.bootable     = psf::get_integer(psf, "BOOTABLE", 0);
			game.info.attr         = psf::get_integer(psf, "ATTRIBUTE", 0);
		}

		if (show_custom_icons)
		{
			if (std::string icon_path = game_icon_path + game.info.serial + "/ICON0.PNG"; fs::is_file(icon_path))
			{
				game.info.icon_path = std::move(icon_path);
				game.has_custom_icon = true;
			}
		}

		if (game.info.icon_path.empty())
		{
			if (std::string icon_path = sfo_dir + "/" + localized_icon; file_exists(icon_path))
			{
				game.info.icon_path = std::move(icon_path);
			}
			else
			{
				game.info.icon_path = sfo_dir + "/ICON0.PNG";
			}
			game.icon_in_archive = archive && archive->exists(game.info.icon_path);
		}

		if (std::string movie_path = game_icon_path + game.info.serial + "/hover.gif"; file_exists(movie_path))
		{
			game.info.movie_path = std::move(movie_path);
			game.has_hover_gif = true;
		}
		else if (std::string movie_path = sfo_dir + "/" + localized_movie; file_exists(movie_path))
		{
			game.info.movie_path = std::move(movie_path);
			game.has_hover_pam = true;
		}
		else if (std::string movie_path = sfo_dir + "/ICON1.PAM"; file_exists(movie_path))
		{
			game.info.movie_path = std::move(movie_path);
			game.has_hover_pam = true;
		}

		const QString serial = QString::fromStdString(game.info.serial);

		m_games_mutex.lock();

		// Read persistent_settings values
		const QString last_played = m_persistent_settings->GetValue(gui::persistent::last_played, serial, "").toString();
		const quint64 playtime    = m_persistent_settings->GetValue(gui::persistent::playtime, serial, 0).toULongLong();

		// Set persistent_settings values if values exist
		if (!last_played.isEmpty())
		{
			m_persistent_settings->SetLastPlayed(serial, last_played, false); // No need to sync here. It would slow down the refresh anyway.
		}
		if (playtime > 0)
		{
			m_persistent_settings->SetPlaytime(serial, playtime, false); // No need to sync here. It would slow down the refresh anyway.
		}

		m_serials.insert(serial);

		if (QString note = m_persistent_settings->GetValue(gui::persistent::notes, serial, "").toString(); !note.isEmpty())
		{
			m_notes.insert_or_assign(serial, std::move(note));
		}

		if (QString title = m_persistent_settings->GetValue(gui::persistent::titles, serial, "").toString().simplified(); !title.isEmpty())
		{
			m_titles.insert_or_assign(serial, std::move(title));
		}

		m_games_mutex.unlock();

		QString qt_cat = QString::fromStdString(game.info.category);

		if (const auto boot_cat = thread_localized.category.cat_boot.find(qt_cat); boot_cat != thread_localized.category.cat_boot.cend())
		{
			qt_cat = boot_cat->second;
		}
		else if (const auto data_cat = thread_localized.category.cat_data.find(qt_cat); data_cat != thread_localized.category.cat_data.cend())
		{
			qt_cat = data_cat->second;
		}
		else if (game.info.category == cat_unknown)
		{
			qt_cat = thread_localized.category.unknown;
		}
		else
		{
			qt_cat = thread_localized.category.other;
		}

		game.localized_category = std::move(qt_cat);
		game.compat = m_game_compat->GetCompatibility(game.info.serial);
		game.has_custom_config = fs::is_file(rpcs3::utils::get_custom_config_path(game.info.serial));
		game.has_custom_pad_config = fs::is_file(rpcs3::utils::get_custom_input_config_path(game.info.serial));

		m_games.push(std::make_shared<gui_game_info>(std::move(game)));
	};

	const auto add_disc_dir = [this](const std::string& path, std::vector<std::string>& legit_paths)
	{
		for (const auto& entry : fs::dir(path))
		{
			if (m_refresh_watcher.isCanceled())
			{
				break;
			}

			if (!entry.is_directory || entry.name == "." || entry.name == "..")
			{
				continue;
			}

			if (entry.name == "PS3_GAME" || std::regex_match(entry.name, std::regex("^PS3_GM[[:digit:]]{2}$")))
			{
				push_path(path + "/" + entry.name, legit_paths);
			}
		}
	};

	m_refresh_watcher.setFuture(QtConcurrent::map(m_path_entries, [this, _hdd, add_disc_dir, add_game](const path_entry& entry)
	{
		std::vector<std::string> legit_paths;

		if (entry.is_from_yml)
		{
			if (fs::is_file(entry.path + "/PARAM.SFO"))
			{
				push_path(entry.path, legit_paths);
			}
			else if (fs::is_file(entry.path + "/PS3_DISC.SFB"))
			{
				// Check if a path loaded from games.yml is already registered in add_dir(_hdd + "disc/");
				if (entry.path.starts_with(_hdd))
				{
					std::string_view frag = std::string_view(entry.path).substr(_hdd.size());

					if (frag.starts_with("disc/"))
					{
						// Our path starts from _hdd + 'disc/'
						frag.remove_prefix(5);

						// Check if the remaining part is the only path component
						if (frag.find_first_of('/') + 1 == 0)
						{
							game_list_log.trace("Removed duplicate: %s", entry.path);

							if (static std::unordered_set<std::string> warn_once_list; warn_once_list.emplace(entry.path).second)
							{
								game_list_log.todo("Game at '%s' is using deprecated directory '/dev_hdd0/disc/'.\nConsider moving into '%s'.", entry.path, rpcs3::utils::get_games_dir());
							}

							return;
						}
					}
				}

				add_disc_dir(entry.path, legit_paths);
			}
			else if (is_file_iso(entry.path))
			{
				push_path(entry.path, legit_paths);
			}
			else
			{
				game_list_log.trace("Invalid game path registered: %s", entry.path);
				return;
			}
		}
		else if (fs::is_file(entry.path + "/PS3_DISC.SFB"))
		{
			if (!entry.is_disc)
			{
				game_list_log.error("Invalid game path found in %s", entry.path);
				return;
			}

			add_disc_dir(entry.path, legit_paths);
		}
		else
		{
			if (entry.is_disc)
			{
				game_list_log.error("Invalid disc path found in %s", entry.path);
				return;
			}

			push_path(entry.path, legit_paths);
		}

		for (const std::string& path : legit_paths)
		{
			add_game(path);
		}
	}));
}

void game_list_frame::OnRefreshFinished()
{
	WaitAndAbortSizeCalcThreads();
	WaitAndAbortRepaintThreads();

	for (auto&& g : m_games.pop_all())
	{
		m_game_data.push_back(g);
	}

	const Localized localized;
	const std::string cat_unknown_localized = localized.category.unknown.toStdString();
	const s32 language_index = gui_application::get_language_id();
	const std::string localized_icon = fmt::format("ICON0_%02d.PNG", language_index);
	const std::string localized_movie = fmt::format("ICON1_%02d.PAM", language_index);

	// Try to update the app version for disc games if there is a patch
	// Also try to find updated game icons and movies
	for (const game_info& entry : m_game_data)
	{
		if (entry->info.category != "DG") continue;

		for (const auto& other : m_game_data)
		{
			if (other->info.category == "DG") continue;
			if (entry->info.serial != other->info.serial) continue;

			// The patch is game data and must have the same serial and an app version
			if (other->info.app_ver != cat_unknown_localized)
			{
				// Update the app version if it's higher than the disc's version (old games may not have an app version)
				if (entry->info.app_ver == cat_unknown_localized || rpcs3::utils::version_is_bigger(other->info.app_ver, entry->info.app_ver, entry->info.serial, false))
				{
					entry->info.app_ver = other->info.app_ver;
				}
				// Update the firmware version if possible and if it's higher than the disc's version
				if (other->info.fw != cat_unknown_localized && rpcs3::utils::version_is_bigger(other->info.fw, entry->info.fw, entry->info.serial, true))
				{
					entry->info.fw = other->info.fw;
				}
				// Update the parental level if possible and if it's higher than the disc's level
				if (other->info.parental_lvl != 0 && other->info.parental_lvl > entry->info.parental_lvl)
				{
					entry->info.parental_lvl = other->info.parental_lvl;
				}
			}

			// Let's fetch the game data icon if preferred or if the path was empty for some reason
			if ((m_prefer_game_data_icons && !entry->has_custom_icon) || entry->info.icon_path.empty())
			{
				if (std::string icon_path = other->info.path + "/" + localized_icon; fs::is_file(icon_path))
				{
					entry->info.icon_path = std::move(icon_path);
				}
				else if (std::string icon_path = other->info.path + "/ICON0.PNG"; fs::is_file(icon_path))
				{
					entry->info.icon_path = std::move(icon_path);
				}
			}

			// Let's fetch the game data movie if preferred or if the path was empty
			if (m_prefer_game_data_icons || entry->info.movie_path.empty())
			{
				if (std::string movie_path = other->info.path + "/" + localized_movie; fs::is_file(movie_path))
				{
					entry->info.movie_path = std::move(movie_path);
				}
				else if (std::string movie_path = other->info.path + "/ICON1.PAM"; fs::is_file(movie_path))
				{
					entry->info.movie_path = std::move(movie_path);
				}
			}
		}
	}

	// Sort by name at the very least.
	std::sort(m_game_data.begin(), m_game_data.end(), [&](const game_info& game1, const game_info& game2)
	{
		const QString serial1 = QString::fromStdString(game1->info.serial);
		const QString serial2 = QString::fromStdString(game2->info.serial);
		const QString& title1 = m_titles.contains(serial1) ? m_titles.at(serial1) : QString::fromStdString(game1->info.name);
		const QString& title2 = m_titles.contains(serial2) ? m_titles.at(serial2) : QString::fromStdString(game2->info.name);
		return title1.toLower() < title2.toLower();
	});

	// clean up hidden games list
	m_hidden_list.intersect(m_serials);
	m_gui_settings->SetValue(gui::gl_hidden_list, QStringList(m_hidden_list.values()));
	m_serials.clear();
	m_path_list.clear();
	m_path_entries.clear();

	Refresh();

	if (!std::exchange(m_initial_refresh_done, true))
	{
		m_game_list->restore_layout(m_gui_settings->GetValue(gui::gl_state).toByteArray());
		m_game_list->sync_header_actions(m_column_acts, [this](int col) { return m_gui_settings->GetGamelistColVisibility(static_cast<gui::game_list_columns>(col)); });
	}

	// Emit signal and remove slots
	Q_EMIT Refreshed();
	m_refresh_funcs_manage_type.reset();
	m_refresh_funcs_manage_type.emplace();
}

void game_list_frame::OnCompatFinished()
{
	for (const auto& game : m_game_data)
	{
		game->compat = m_game_compat->GetCompatibility(game->info.serial);
	}
	Refresh();
}

void game_list_frame::ToggleCategoryFilter(const QStringList& categories, bool show)
{
	QStringList& filters = m_is_list_layout ? m_category_filters : m_grid_category_filters;

	if (show)
	{
		filters.append(categories);
	}
	else
	{
		for (const QString& cat : categories)
		{
			filters.removeAll(cat);
		}
	}

	Refresh();
}

void game_list_frame::SaveSettings()
{
	for (const auto& [col, action] : m_column_acts)
	{
		m_gui_settings->SetGamelistColVisibility(static_cast<gui::game_list_columns>(col), action->isChecked());
	}
	m_gui_settings->SetValue(gui::gl_sortCol, m_sort_column, false);
	m_gui_settings->SetValue(gui::gl_sortAsc, m_col_sort_order == Qt::AscendingOrder, false);
	m_gui_settings->SetValue(gui::gl_state, m_game_list->horizontalHeader()->saveState(), true);
}

void game_list_frame::doubleClickedSlot(QTableWidgetItem* item)
{
	if (!item)
	{
		return;
	}

	doubleClickedSlot(GetGameInfoByMode(item));
}

void game_list_frame::doubleClickedSlot(const game_info& game)
{
	if (!game)
	{
		return;
	}

	sys_log.notice("Booting from gamelist per doubleclick...");
	Q_EMIT RequestBoot(game);
}

void game_list_frame::ItemSelectionChangedSlot()
{
	game_info game = nullptr;

	if (m_is_list_layout)
	{
		if (const auto item = m_game_list->item(m_game_list->currentRow(), static_cast<int>(gui::game_list_columns::icon)); item && item->isSelected())
		{
			game = GetGameInfoByMode(item);
		}
	}

	Q_EMIT NotifyGameSelection(game);
}

void game_list_frame::ShowContextMenu(const QPoint& pos)
{
	QPoint global_pos;
	std::vector<game_info> games;

	if (m_is_list_layout)
	{
		global_pos = m_game_list->viewport()->mapToGlobal(pos);

		for (const QTableWidgetItem* item : m_game_list->selectedItems())
		{
			if (!item || item->column() != static_cast<int>(gui::game_list_columns::icon))
				continue;

			if (game_info gameinfo = GetGameInfoFromItem(item))
				games.push_back(gameinfo);
		}
	}
	else if (m_game_grid)
	{
		global_pos = m_game_grid->mapToGlobal(pos);

		for (const flow_widget_item* selected_item : m_game_grid->selected_items())
		{
			if (const game_list_grid_item* item = static_cast<const game_list_grid_item*>(selected_item))
			{
				if (game_info gameinfo = item->game())
					games.push_back(gameinfo);
			}
		}
	}

	if (!games.empty())
	{
		game_list_context_menu menu(this);
		menu.show_menu(games, global_pos);
	}
}

void game_list_frame::ShowCustomConfigIcon(const game_info& game)
{
	if (!game)
	{
		return;
	}

	const std::string serial         = game->info.serial;
	const bool has_custom_config     = game->has_custom_config;
	const bool has_custom_pad_config = game->has_custom_pad_config;

	for (const auto& other_game : m_game_data)
	{
		if (other_game->info.serial == serial)
		{
			other_game->has_custom_config     = has_custom_config;
			other_game->has_custom_pad_config = has_custom_pad_config;
		}
	}

	m_game_list->set_custom_config_icon(game);

	RepaintIcons();
}

void game_list_frame::ResizeIcons(int slider_pos)
{
	m_icon_size_index = slider_pos;
	m_icon_size = gui_settings::SizeFromSlider(slider_pos);

	RepaintIcons();
}

void game_list_frame::RepaintIcons(bool from_settings)
{
	gui::utils::stop_future_watcher(m_parsing_watcher, false);
	gui::utils::stop_future_watcher(m_refresh_watcher, false);
	WaitAndAbortRepaintThreads();

	if (from_settings)
	{
		if (m_gui_settings->GetValue(gui::m_enableUIColors).toBool())
		{
			m_icon_color = m_gui_settings->GetValue(gui::gl_iconColor).value<QColor>();
		}
		else
		{
			m_icon_color = gui::utils::get_label_color("gamelist_icon_background_color", Qt::transparent, Qt::transparent);
		}
	}

	if (m_is_list_layout)
	{
		m_game_list->repaint_icons(m_game_data, m_icon_color, m_icon_size, devicePixelRatioF());
	}
	else
	{
		m_game_grid->set_draw_compat_status_to_grid(m_draw_compat_status_to_grid);
		m_game_grid->repaint_icons(m_game_data, m_icon_color, m_icon_size, devicePixelRatioF());
	}
}

void game_list_frame::SetShowHidden(bool show)
{
	m_show_hidden = show;
}

void game_list_frame::SetListMode(bool is_list)
{
	m_old_layout_is_list = m_is_list_layout;
	m_is_list_layout = is_list;

	m_gui_settings->SetValue(gui::gl_listMode, is_list);

	Refresh();

	if (m_is_list_layout)
	{
		m_central_widget->setCurrentWidget(m_game_list);
	}
	else
	{
		m_central_widget->setCurrentWidget(m_game_grid);
	}
}

void game_list_frame::SetSearchText(const QString& text)
{
	m_search_text = text;
	Refresh();
}

void game_list_frame::FocusAndSelectFirstEntryIfNoneIs()
{
	if (m_is_list_layout)
	{
		if (m_game_list)
		{
			m_game_list->FocusAndSelectFirstEntryIfNoneIs();
		}
	}
	else
	{
		if (m_game_grid)
		{
			m_game_grid->FocusAndSelectFirstEntryIfNoneIs();
		}
	}
}

void game_list_frame::closeEvent(QCloseEvent *event)
{
	SaveSettings();

	QDockWidget::closeEvent(event);
	Q_EMIT GameListFrameClosed();
}

bool game_list_frame::eventFilter(QObject *object, QEvent *event)
{
	// Zoom gamelist/gamegrid
	if (event->type() == QEvent::Wheel && (object == m_game_list->verticalScrollBar() || object == m_game_grid->scroll_area()->verticalScrollBar()))
	{
		QWheelEvent* wheel_event = static_cast<QWheelEvent*>(event);

		if (wheel_event->modifiers() & Qt::ControlModifier)
		{
			const QPoint num_steps = wheel_event->angleDelta() / 8 / 15; // http://doc.qt.io/qt-5/qwheelevent.html#pixelDelta
			const int value = num_steps.y();
			Q_EMIT RequestIconSizeChange(value);
			return true;
		}
	}
	else if (event->type() == QEvent::KeyPress && (object == m_game_list || object == m_game_grid))
	{
		QKeyEvent* key_event = static_cast<QKeyEvent*>(event);

		if (key_event->modifiers() & Qt::ControlModifier)
		{
			if (key_event->key() == Qt::Key_Plus)
			{
				Q_EMIT RequestIconSizeChange(1);
				return true;
			}
			if (key_event->key() == Qt::Key_Minus)
			{
				Q_EMIT RequestIconSizeChange(-1);
				return true;
			}
		}
		else if (!key_event->isAutoRepeat())
		{
			if (key_event->key() == Qt::Key_Enter || key_event->key() == Qt::Key_Return)
			{
				game_info gameinfo{};

				if (object == m_game_list)
				{
					const QTableWidgetItem* item = m_game_list->item(m_game_list->currentRow(), static_cast<int>(gui::game_list_columns::icon));

					if (item && item->isSelected())
					{
						gameinfo = GetGameInfoFromItem(item);
					}
				}
				else if (const auto items = m_game_grid->selected_items(); !items.empty())
				{
					if (const game_list_grid_item* item = static_cast<const game_list_grid_item*>(*items.begin()))
					{
						gameinfo = item->game();
					}
				}

				if (!gameinfo)
					return false;

				sys_log.notice("Booting from gamelist by pressing %s...", key_event->key() == Qt::Key_Enter ? "Enter" : "Return");
				Q_EMIT RequestBoot(gameinfo);

				return true;
			}
		}
	}

	return QDockWidget::eventFilter(object, event);
}

/**
* Returns false if the game should be hidden because it doesn't match search term in toolbar.
*/
bool game_list_frame::SearchMatchesApp(const QString& name, const QString& serial, bool fallback) const
{
	if (!m_search_text.isEmpty())
	{
		QString search_text = m_search_text.toLower();
		QString title_name;

		if (const auto it = m_titles.find(serial); it != m_titles.cend())
		{
			title_name = it->second.toLower();
		}
		else
		{
			title_name = name.toLower();
		}

		// Ignore trademarks when no search results have been yielded by unmodified search
		static const QRegularExpression s_ignored_on_fallback(reinterpret_cast<const char*>(u8"[:\\-®©™]+"));

		if (fallback)
		{
			search_text = search_text.simplified();
			title_name = title_name.simplified();

			QString title_name_replaced_trademarks_with_spaces = title_name;
			QString title_name_simplified = title_name;

			search_text.remove(s_ignored_on_fallback);
			title_name.remove(s_ignored_on_fallback);
			title_name_replaced_trademarks_with_spaces.replace(s_ignored_on_fallback, " ");

			// Before simplify to allow spaces in the beginning and end where ignored characters may have been
			if (title_name_replaced_trademarks_with_spaces.contains(search_text))
			{
				return true;
			}

			title_name_replaced_trademarks_with_spaces = title_name_replaced_trademarks_with_spaces.simplified();

			if (title_name_replaced_trademarks_with_spaces.contains(search_text))
			{
				return true;
			}

			// Initials-only search
			if (search_text.size() >= 2 && search_text.count(QRegularExpression(QStringLiteral("[a-z0-9]"))) >= 2 && !search_text.contains(QRegularExpression(QStringLiteral("[^a-z0-9 ]"))))
			{
				QString initials = QStringLiteral("\\b");

				for (auto it = search_text.begin(); it != search_text.end(); it++)
				{
					if (it->isSpace())
					{
						continue;
					}

					initials += *it;
					initials += QStringLiteral("\\w*\\b ");
				}

				initials += QChar('?');

				if (title_name_replaced_trademarks_with_spaces.contains(QRegularExpression(initials)))
				{
					return true;
				}
			}
		}

		return title_name.contains(search_text) || serial.toLower().contains(search_text);
	}
	return true;
}

std::set<std::string> game_list_frame::CurrentSelectionPaths()
{
	std::set<std::string> selection;

	if (m_old_layout_is_list)
	{
		for (const QTableWidgetItem* selected_item : m_game_list->selectedItems())
		{
			if (const QTableWidgetItem* item = m_game_list->item(selected_item->row(), 0))
			{
				if (const QVariant var = item->data(gui::game_role); var.canConvert<game_info>())
				{
					if (const game_info game = var.value<game_info>())
					{
						selection.insert(game->info.path + game->info.icon_path);
					}
				}
			}
		}
	}
	else if (m_game_grid)
	{
		for (const flow_widget_item* selected_item : m_game_grid->selected_items())
		{
			if (const game_list_grid_item* item = static_cast<const game_list_grid_item*>(selected_item))
			{
				if (const game_info& game = item->game())
				{
					selection.insert(game->info.path + game->info.icon_path);
				}
			}
		}
	}

	m_old_layout_is_list = m_is_list_layout;

	return selection;
}

game_info game_list_frame::GetGameInfoByMode(const QTableWidgetItem* item) const
{
	if (!item)
	{
		return nullptr;
	}

	if (m_is_list_layout)
	{
		return GetGameInfoFromItem(m_game_list->item(item->row(), static_cast<int>(gui::game_list_columns::icon)));
	}

	return GetGameInfoFromItem(item);
}

game_info game_list_frame::GetGameInfoFromItem(const QTableWidgetItem* item)
{
	if (!item)
	{
		return nullptr;
	}

	const QVariant var = item->data(gui::game_role);
	if (!var.canConvert<game_info>())
	{
		return nullptr;
	}

	return var.value<game_info>();
}

void game_list_frame::SetShowCompatibilityInGrid(bool show)
{
	m_draw_compat_status_to_grid = show;
	RepaintIcons();
	m_gui_settings->SetValue(gui::gl_draw_compat, show);
}

void game_list_frame::SetPreferGameDataIcons(bool enabled)
{
	if (m_prefer_game_data_icons != enabled)
	{
		m_prefer_game_data_icons = enabled;
		m_gui_settings->SetValue(gui::gl_pref_gd_icon, enabled);
		Refresh(true);
	}
}

void game_list_frame::SetShowCustomIcons(bool show)
{
	if (m_show_custom_icons != show)
	{
		m_show_custom_icons = show;
		m_gui_settings->SetValue(gui::gl_custom_icon, show);
		Refresh(true);
	}
}

void game_list_frame::SetPlayHoverGifs(bool play)
{
	if (m_play_hover_movies != play)
	{
		m_play_hover_movies = play;
		m_gui_settings->SetValue(gui::gl_hover_gifs, play);
		Refresh(true);
	}
}

void game_list_frame::WaitAndAbortRepaintThreads()
{
	for (const game_info& game : m_game_data)
	{
		if (game && game->item)
		{
			game->item->wait_for_icon_loading(true);
		}
	}
}

void game_list_frame::WaitAndAbortSizeCalcThreads()
{
	for (const game_info& game : m_game_data)
	{
		if (game && game->item)
		{
			game->item->wait_for_size_on_disk_loading(true);
		}
	}
}
