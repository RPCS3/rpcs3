#include "game_list_frame.h"
#include "qt_utils.h"
#include "settings_dialog.h"
#include "pad_settings_dialog.h"
#include "custom_table_widget_item.h"
#include "input_dialog.h"
#include "localized.h"
#include "progress_dialog.h"
#include "persistent_settings.h"
#include "emu_settings.h"
#include "gui_settings.h"
#include "game_list_delegate.h"
#include "game_list_table.h"
#include "game_list_grid.h"
#include "game_list_grid_item.h"
#include "patch_manager_dialog.h"

#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "Emu/vfs_config.h"
#include "Emu/system_utils.hpp"
#include "Loader/PSF.h"
#include "util/types.hpp"
#include "Utilities/File.h"
#include "Utilities/mutex.h"
#include "util/yaml.hpp"
#include "util/sysinfo.hpp"
#include "Input/pad_thread.h"

#include <algorithm>
#include <memory>
#include <set>
#include <regex>
#include <unordered_map>
#include <unordered_set>

#include <QtConcurrent>
#include <QDesktopServices>
#include <QHeaderView>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollBar>
#include <QInputDialog>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>

LOG_CHANNEL(game_list_log, "GameList");
LOG_CHANNEL(sys_log, "SYS");

extern atomic_t<bool> g_system_progress_canceled;

std::string get_savestate_file(std::string_view title_id, std::string_view boot_pat, s64 abs_id, s64 rel_id);

inline std::string sstr(const QString& _in) { return _in.toStdString(); }

game_list_frame::game_list_frame(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<emu_settings> emu_settings, std::shared_ptr<persistent_settings> persistent_settings, QWidget* parent)
	: custom_dock_widget(tr("Game List"), parent)
	, m_gui_settings(std::move(gui_settings))
	, m_emu_settings(std::move(emu_settings))
	, m_persistent_settings(std::move(persistent_settings))
{
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
	m_gui_settings->SetValue(gui::gl_iconColor, m_icon_color);
	m_gui_settings->SetValue(gui::gl_marginFactor, m_margin_factor);
	m_gui_settings->SetValue(gui::gl_textFactor, m_text_factor);

	// Only show the progress dialog after some time has passed
	m_progress_dialog_timer = new QTimer(this);
	m_progress_dialog_timer->setSingleShot(true);
	m_progress_dialog_timer->setInterval(200);
	connect(m_progress_dialog_timer, &QTimer::timeout, this, [this]()
	{
		if (m_progress_dialog)
		{
			m_progress_dialog->show();
		}
	});

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
	auto add_column = [this](gui::game_list_columns col, const QString& header_text, const QString& action_text)
	{
		m_game_list->setHorizontalHeaderItem(static_cast<int>(col), new QTableWidgetItem(header_text));
		m_columnActs.append(new QAction(action_text, this));
	};

	add_column(gui::game_list_columns::icon,       tr("Icon"),                  tr("Show Icons"));
	add_column(gui::game_list_columns::name,       tr("Name"),                  tr("Show Names"));
	add_column(gui::game_list_columns::serial,     tr("Serial"),                tr("Show Serials"));
	add_column(gui::game_list_columns::firmware,   tr("Firmware"),              tr("Show Firmwares"));
	add_column(gui::game_list_columns::version,    tr("Version"),               tr("Show Versions"));
	add_column(gui::game_list_columns::category,   tr("Category"),              tr("Show Categories"));
	add_column(gui::game_list_columns::path,       tr("Path"),                  tr("Show Paths"));
	add_column(gui::game_list_columns::move,       tr("PlayStation Move"),      tr("Show PlayStation Move"));
	add_column(gui::game_list_columns::resolution, tr("Supported Resolutions"), tr("Show Supported Resolutions"));
	add_column(gui::game_list_columns::sound,      tr("Sound Formats"),         tr("Show Sound Formats"));
	add_column(gui::game_list_columns::parental,   tr("Parental Level"),        tr("Show Parental Levels"));
	add_column(gui::game_list_columns::last_play,  tr("Last Played"),           tr("Show Last Played"));
	add_column(gui::game_list_columns::playtime,   tr("Time Played"),           tr("Show Time Played"));
	add_column(gui::game_list_columns::compat,     tr("Compatibility"),         tr("Show Compatibility"));
	add_column(gui::game_list_columns::dir_size,   tr("Space On Disk"),         tr("Show Space On Disk"));

	// Events
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
	});

	connect(m_game_list, &QTableWidget::customContextMenuRequested, this, &game_list_frame::ShowContextMenu);
	connect(m_game_list, &QTableWidget::itemSelectionChanged, this, &game_list_frame::ItemSelectionChangedSlot);
	connect(m_game_list, &QTableWidget::itemDoubleClicked, this, QOverload<QTableWidgetItem*>::of(&game_list_frame::doubleClickedSlot));

	connect(m_game_list->horizontalHeader(), &QHeaderView::sectionClicked, this, &game_list_frame::OnColClicked);

	connect(m_game_grid, &QWidget::customContextMenuRequested, this, &game_list_frame::ShowContextMenu);
	connect(m_game_grid, &game_list_grid::ItemSelectionChanged, this, &game_list_frame::NotifyGameSelection);
	connect(m_game_grid, &game_list_grid::ItemDoubleClicked, this, QOverload<const game_info&>::of(&game_list_frame::doubleClickedSlot));

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

	m_game_list->create_header_actions(m_columnActs,
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
	m_show_custom_icons = m_gui_settings->GetValue(gui::gl_custom_icon).toBool();
	m_play_hover_movies = m_gui_settings->GetValue(gui::gl_hover_gifs).toBool();

	for (int col = 0; col < m_columnActs.count(); ++col)
	{
		const bool vis = m_gui_settings->GetGamelistColVisibility(static_cast<gui::game_list_columns>(col));
		m_columnActs[col]->setChecked(vis);
		m_game_list->setColumnHidden(col, !vis);
	}
}

game_list_frame::~game_list_frame()
{
	WaitAndAbortSizeCalcThreads();
	WaitAndAbortRepaintThreads();
	gui::utils::stop_future_watcher(m_parsing_watcher, true);
	gui::utils::stop_future_watcher(m_refresh_watcher, true);
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

	m_gui_settings->SetValue(gui::gl_sortAsc, m_col_sort_order == Qt::AscendingOrder);
	m_gui_settings->SetValue(gui::gl_sortCol, col);

	m_game_list->sort(m_game_data.count(), m_sort_column, m_col_sort_order);
}

// Get visibility of entries
bool game_list_frame::IsEntryVisible(const game_info& game, bool search_fallback) const
{
	const auto matches_category = [&]()
	{
		if (m_is_list_layout)
		{
			return m_category_filters.contains(qstr(game->info.category));
		}

		return m_grid_category_filters.contains(qstr(game->info.category));
	};

	const QString serial = qstr(game->info.serial);
	const bool is_visible = m_show_hidden || !m_hidden_list.contains(serial);
	return is_visible && matches_category() && SearchMatchesApp(qstr(game->info.name), serial, search_fallback);
}

std::string game_list_frame::GetCacheDirBySerial(const std::string& serial)
{
	return rpcs3::utils::get_cache_dir() + serial;
}

std::string game_list_frame::GetDataDirBySerial(const std::string& serial)
{
	return fs::get_config_dir() + "data/" + serial;
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

void game_list_frame::Refresh(const bool from_drive, const bool scroll_after)
{
	if (from_drive)
	{
		WaitAndAbortSizeCalcThreads();
	}
	WaitAndAbortRepaintThreads();
	gui::utils::stop_future_watcher(m_parsing_watcher, from_drive);
	gui::utils::stop_future_watcher(m_refresh_watcher, from_drive);

	if (m_progress_dialog_timer)
	{
		m_progress_dialog_timer->stop();
	}

	if (m_progress_dialog)
	{
		m_progress_dialog->accept();
		m_progress_dialog->deleteLater();
		m_progress_dialog = nullptr;
	}

	if (from_drive)
	{
		m_path_entries.clear();
		m_path_list.clear();
		m_serials.clear();
		m_game_data.clear();
		m_notes.clear();
		m_games.pop_all();

		m_progress_dialog = new progress_dialog(tr("Loading games"), tr("Loading games, please wait..."), tr("Cancel"), 0, 0, true, this, Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
		connect(&m_refresh_watcher, &QFutureWatcher<void>::progressRangeChanged, m_progress_dialog, &QProgressDialog::setRange);
		connect(&m_refresh_watcher, &QFutureWatcher<void>::progressValueChanged, m_progress_dialog, &QProgressDialog::setValue);
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

			if (m_progress_dialog_timer)
			{
				m_progress_dialog_timer->stop();
			}

			m_progress_dialog->deleteLater();
			m_progress_dialog = nullptr;
		});

		if (m_progress_dialog_timer)
		{
			m_progress_dialog_timer->start();
		}

		Emu.AddGamesFromDir(g_cfg_vfs.get(g_cfg_vfs.games_dir, rpcs3::utils::get_emu_dir()));

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
					if (!entry.is_directory || entry.name == "." || entry.name == "..")
					{
						continue;
					}

					QApplication::processEvents();
					std::lock_guard lock(m_path_mutex);
					m_path_entries.emplace_back(path_entry{path + entry.name, is_disc, false});
				}
			};

			const std::string hdd0_game = _hdd + "game/";

			add_dir(hdd0_game, false);
			add_dir(_hdd + "disc/", true); // Deprecated

			for (const auto& [serial, path] : Emu.GetGamesConfig().get_games())
			{
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

				QApplication::processEvents();
				std::lock_guard lock(m_path_mutex);
				m_path_entries.emplace_back(path_entry{game_dir, false, true});
			}
		}));

		return;
	}

	// Fill Game List / Game Grid

	const std::string selected_item = CurrentSelectionPath();

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
		m_game_list->populate(matching_apps, m_notes, m_titles, selected_item, m_play_hover_movies);
		m_game_list->sort(m_game_data.count(), m_sort_column, m_col_sort_order);
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
		m_game_grid->populate(matching_apps, m_notes, m_titles, selected_item, m_play_hover_movies);
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

	const std::string game_icon_path = m_play_hover_movies ? fs::get_config_dir() + "/Icons/game_icons/" : "";

	const auto add_game = [this, dev_flash, cat_unknown_localized = sstr(localized.category.unknown), cat_unknown = sstr(cat::cat_unknown), game_icon_path](const std::string& dir_or_elf)
	{
		GameInfo game{};
		game.path = dir_or_elf;

		const Localized thread_localized;

		const std::string sfo_dir = rpcs3::utils::get_sfo_dir_from_game_path(dir_or_elf);
		const psf::registry psf = psf::load_object(sfo_dir + "/PARAM.SFO");
		const std::string_view title_id = psf::get_string(psf, "TITLE_ID", "");

		if (title_id.empty())
		{
			if (!fs::is_file(dir_or_elf))
			{
				// Do not care about invalid entries
				return;
			}

			game.serial = dir_or_elf.substr(dir_or_elf.find_last_of(fs::delim) + 1);
			game.category = cat::cat_ps3_os.toStdString(); // Key for operating system executables
			game.version = utils::get_firmware_version();
			game.fw = game.version;
			game.bootable = 1;
			game.icon_path = dev_flash + "vsh/resource/explore/icon/icon_home.png";

			if (dir_or_elf.starts_with(dev_flash))
			{
				std::string path_vfs = dir_or_elf.substr(dev_flash.size());

				if (const usz pos = path_vfs.find_first_not_of(fs::delim); pos != umax && pos != 0)
				{
					path_vfs = path_vfs.substr(pos);
				}

				if (const auto it = thread_localized.title.titles.find(path_vfs); it != thread_localized.title.titles.cend())
				{
					game.name = it->second.toStdString();
				}
			}

			if (game.name == "Unknown")
			{
				game.name = game.serial;
			}
		}
		else
		{
			game.serial       = std::string(title_id);
			game.name         = std::string(psf::get_string(psf, "TITLE", cat_unknown_localized));
			game.app_ver      = std::string(psf::get_string(psf, "APP_VER", cat_unknown_localized));
			game.version      = std::string(psf::get_string(psf, "VERSION", cat_unknown_localized));
			game.category     = std::string(psf::get_string(psf, "CATEGORY", cat_unknown));
			game.fw           = std::string(psf::get_string(psf, "PS3_SYSTEM_VER", cat_unknown_localized));
			game.parental_lvl = psf::get_integer(psf, "PARENTAL_LEVEL", 0);
			game.resolution   = psf::get_integer(psf, "RESOLUTION", 0);
			game.sound_format = psf::get_integer(psf, "SOUND_FORMAT", 0);
			game.bootable     = psf::get_integer(psf, "BOOTABLE", 0);
			game.attr         = psf::get_integer(psf, "ATTRIBUTE", 0);
			game.icon_path    = sfo_dir + "/ICON0.PNG";

			if (game.category == "DG")
			{
				std::string latest_icon = rpcs3::utils::get_hdd0_dir() + "game/" + game.serial + "/ICON0.PNG";
				if (fs::is_file(latest_icon))
				{
					game.icon_path = std::move(latest_icon);
				}
			}
		}

		if (m_show_custom_icons)
		{
			if (std::string icon_path = fs::get_config_dir() + "/Icons/game_icons/" + game.serial + "/ICON0.PNG"; fs::is_file(icon_path))
			{
				game.icon_path = std::move(icon_path);
			}
		}

		const QString serial = qstr(game.serial);

		m_games_mutex.lock();

		// Read persistent_settings values
		const QString note  = m_persistent_settings->GetValue(gui::persistent::notes, serial, "").toString();
		const QString title = m_persistent_settings->GetValue(gui::persistent::titles, serial, "").toString().simplified();
		QString last_played = m_persistent_settings->GetValue(gui::persistent::last_played, serial, "").toString();
		quint64 playtime    = m_persistent_settings->GetValue(gui::persistent::playtime, serial, 0).toULongLong();

		// Set persistent_settings values if values exist
		if (!last_played.isEmpty())
		{
			m_persistent_settings->SetLastPlayed(serial, last_played);
		}
		if (playtime > 0)
		{
			m_persistent_settings->SetPlaytime(serial, playtime);
		}

		m_serials.insert(serial);

		if (!note.isEmpty())
		{
			m_notes.insert(serial, note);
		}

		if (!title.isEmpty())
		{
			m_titles.insert(serial, title);
		}

		m_games_mutex.unlock();

		QString qt_cat = qstr(game.category);

		if (const auto boot_cat = thread_localized.category.cat_boot.find(qt_cat); boot_cat != thread_localized.category.cat_boot.cend())
		{
			qt_cat = boot_cat->second;
		}
		else if (const auto data_cat = thread_localized.category.cat_data.find(qt_cat); data_cat != thread_localized.category.cat_data.cend())
		{
			qt_cat = data_cat->second;
		}
		else if (game.category == cat_unknown)
		{
			qt_cat = thread_localized.category.unknown;
		}
		else
		{
			qt_cat = thread_localized.category.other;
		}

		gui_game_info info{};
		info.info = std::move(game);
		info.localized_category = std::move(qt_cat);
		info.compat = m_game_compat->GetCompatibility(info.info.serial);
		info.hasCustomConfig = fs::is_file(rpcs3::utils::get_custom_config_path(info.info.serial));
		info.hasCustomPadConfig = fs::is_file(rpcs3::utils::get_custom_input_config_path(info.info.serial));
		info.has_hover_gif = fs::is_file(game_icon_path + info.info.serial + "/hover.gif");

		m_games.push(std::make_shared<gui_game_info>(std::move(info)));
	};

	const auto add_disc_dir = [this](const std::string& path, std::vector<std::string>& legit_paths)
	{
		for (const auto& entry : fs::dir(path))
		{
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
								game_list_log.todo("Game at '%s' is using deprecated directory '/dev_hdd0/disc/'.\nConsider moving into '%s'.", entry.path, g_cfg_vfs.get(g_cfg_vfs.games_dir, rpcs3::utils::get_emu_dir()));
							}

							return;
						}
					}
				}

				add_disc_dir(entry.path, legit_paths);
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
	const std::string cat_unknown_localized = sstr(localized.category.unknown);

	// Try to update the app version for disc games if there is a patch
	for (const auto& entry : m_game_data)
	{
		if (entry->info.category == "DG")
		{
			for (const auto& other : m_game_data)
			{
				// The patch is game data and must have the same serial and an app version
				static constexpr auto version_is_bigger = [](const std::string& v0, const std::string& v1, const std::string& serial, bool is_fw)
				{
					std::add_pointer_t<char> ev0, ev1;
					const double ver0 = std::strtod(v0.c_str(), &ev0);
					const double ver1 = std::strtod(v1.c_str(), &ev1);

					if (v0.c_str() + v0.size() == ev0 && v1.c_str() + v1.size() == ev1)
					{
						return ver0 > ver1;
					}

					game_list_log.error("Failed to update the displayed %s numbers for title ID %s\n'%s'-'%s'", is_fw ? "firmware version" : "version", serial, v0, v1);
					return false;
				};

				if (entry->info.serial == other->info.serial && other->info.category != "DG" && other->info.app_ver != cat_unknown_localized)
				{
					// Update the app version if it's higher than the disc's version (old games may not have an app version)
					if (entry->info.app_ver == cat_unknown_localized || version_is_bigger(other->info.app_ver, entry->info.app_ver, entry->info.serial, true))
					{
						entry->info.app_ver = other->info.app_ver;
					}
					// Update the firmware version if possible and if it's higher than the disc's version
					if (other->info.fw != cat_unknown_localized && version_is_bigger(other->info.fw, entry->info.fw, entry->info.serial, false))
					{
						entry->info.fw = other->info.fw;
					}
					// Update the parental level if possible and if it's higher than the disc's level
					if (other->info.parental_lvl != 0 && other->info.parental_lvl > entry->info.parental_lvl)
					{
						entry->info.parental_lvl = other->info.parental_lvl;
					}
				}
			}
		}
	}

	// Sort by name at the very least.
	std::sort(m_game_data.begin(), m_game_data.end(), [&](const game_info& game1, const game_info& game2)
	{
		const QString title1 = m_titles.value(qstr(game1->info.serial), qstr(game1->info.name));
		const QString title2 = m_titles.value(qstr(game2->info.serial), qstr(game2->info.name));
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
	}

	if (m_progress_dialog_timer)
	{
		m_progress_dialog_timer->stop();
	}

	if (m_progress_dialog)
	{
		m_progress_dialog->accept();
		m_progress_dialog->deleteLater();
		m_progress_dialog = nullptr;
	}
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
	for (int col = 0; col < m_columnActs.count(); ++col)
	{
		m_gui_settings->SetGamelistColVisibility(static_cast<gui::game_list_columns>(col), m_columnActs[col]->isChecked());
	}
	m_gui_settings->SetValue(gui::gl_sortCol, m_sort_column);
	m_gui_settings->SetValue(gui::gl_sortAsc, m_col_sort_order == Qt::AscendingOrder);
	m_gui_settings->SetValue(gui::gl_state, m_game_list->horizontalHeader()->saveState());
}

void game_list_frame::doubleClickedSlot(QTableWidgetItem *item)
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

void game_list_frame::CreateShortcuts(const game_info& gameinfo, const std::set<gui::utils::shortcut_location>& locations)
{
	if (locations.empty())
	{
		game_list_log.error("Failed to create shortcuts for %s. No locations selected.", qstr(gameinfo->info.name).simplified());
		return;
	}

	std::string gameid_token_value;

	const std::string dev_flash = g_cfg_vfs.get_dev_flash();

	if (gameinfo->info.category == "DG" && !fs::is_file(rpcs3::utils::get_hdd0_dir() + "/game/" + gameinfo->info.serial + "/USRDIR/EBOOT.BIN"))
	{
		const usz ps3_game_dir_pos = fs::get_parent_dir(gameinfo->info.path).size();
		std::string relative_boot_dir = gameinfo->info.path.substr(ps3_game_dir_pos);

		if (usz char_pos = relative_boot_dir.find_first_not_of(fs::delim); char_pos != umax)
		{
			relative_boot_dir = relative_boot_dir.substr(char_pos);
		}
		else
		{
			relative_boot_dir.clear();
		}

		if (!relative_boot_dir.empty())
		{
			if (relative_boot_dir != "PS3_GAME")
			{
				gameid_token_value = gameinfo->info.serial + "/" + relative_boot_dir;
			}
			else
			{
				gameid_token_value = gameinfo->info.serial;
			}
		}
	}
	else
	{
		gameid_token_value = gameinfo->info.serial;
	}

#ifdef __linux__
	const std::string target_cli_args = gameinfo->info.path.starts_with(dev_flash) ? fmt::format("--no-gui \"%%%%RPCS3_VFS%%%%:dev_flash/%s\"", gameinfo->info.path.substr(dev_flash.size()))
											: fmt::format("--no-gui \"%%%%RPCS3_GAMEID%%%%:%s\"", gameid_token_value);
#else
	const std::string target_cli_args = gameinfo->info.path.starts_with(dev_flash) ? fmt::format("--no-gui \"%%RPCS3_VFS%%:dev_flash/%s\"", gameinfo->info.path.substr(dev_flash.size()))
											: fmt::format("--no-gui \"%%RPCS3_GAMEID%%:%s\"", gameid_token_value);
#endif
	const std::string target_icon_dir = fmt::format("%sIcons/game_icons/%s/", fs::get_config_dir(), gameinfo->info.serial);

	if (!fs::create_path(target_icon_dir))
	{
		game_list_log.error("Failed to create shortcut path %s (%s)", qstr(gameinfo->info.name).simplified(), target_icon_dir, fs::g_tls_error);
		return;
	}

	bool success = true;

	for (const gui::utils::shortcut_location& location : locations)
	{
		std::string destination;

		switch (location)
		{
		case gui::utils::shortcut_location::desktop:
			destination = "desktop";
			break;
		case gui::utils::shortcut_location::applications:
			destination = "application menu";
			break;
#ifdef _WIN32
		case gui::utils::shortcut_location::rpcs3_shortcuts:
			destination = "/games/shortcuts/";
			break;
#endif
		}

		if (!gameid_token_value.empty() && gui::utils::create_shortcut(gameinfo->info.name, target_cli_args, gameinfo->info.name, gameinfo->info.icon_path, target_icon_dir, location))
		{
			game_list_log.success("Created %s shortcut for %s", destination, qstr(gameinfo->info.name).simplified());
		}
		else
		{
			game_list_log.error("Failed to create %s shortcut for %s", destination, qstr(gameinfo->info.name).simplified());
			success = false;
		}
	}

#ifdef _WIN32
	if (locations.size() > 1 || !locations.contains(gui::utils::shortcut_location::rpcs3_shortcuts))
#endif
	{
		if (success)
		{
			QMessageBox::information(this, tr("Success!"), tr("Successfully created shortcut(s)."));
		}
		else
		{
			QMessageBox::warning(this, tr("Warning!"), tr("Failed to create shortcut(s)!"));
		}
	}
}

void game_list_frame::ShowContextMenu(const QPoint &pos)
{
	QPoint global_pos;
	game_info gameinfo;

	if (m_is_list_layout)
	{
		QTableWidgetItem* item = m_game_list->item(m_game_list->indexAt(pos).row(), static_cast<int>(gui::game_list_columns::icon));
		global_pos = m_game_list->viewport()->mapToGlobal(pos);
		gameinfo = GetGameInfoFromItem(item);
	}
	else if (game_list_grid_item* item = static_cast<game_list_grid_item*>(m_game_grid->selected_item()))
	{
		gameinfo = item->game();
		global_pos = m_game_grid->mapToGlobal(pos);
	}

	if (!gameinfo)
	{
		return;
	}

	GameInfo current_game = gameinfo->info;
	const QString serial = qstr(current_game.serial);
	const QString name = qstr(current_game.name).simplified();

	const std::string cache_base_dir = GetCacheDirBySerial(current_game.serial);
	const std::string data_base_dir  = GetDataDirBySerial(current_game.serial);

	// Make Actions
	QMenu menu;

	static const auto is_game_running = [](const std::string& serial)
	{
		return Emu.GetStatus(false) != system_state::stopped && serial == Emu.GetTitleID();
	};

	const bool is_current_running_game = is_game_running(current_game.serial);

	QAction* boot = new QAction(gameinfo->hasCustomConfig
		? (is_current_running_game
			? tr("&Reboot with global configuration")
			: tr("&Boot with global configuration"))
		: (is_current_running_game
			? tr("&Reboot")
			: tr("&Boot")));

	QFont font = boot->font();
	font.setBold(true);

	if (gameinfo->hasCustomConfig)
	{
		QAction* boot_custom = menu.addAction(is_current_running_game
			? tr("&Reboot with custom configuration")
			: tr("&Boot with custom configuration"));
		boot_custom->setFont(font);
		connect(boot_custom, &QAction::triggered, [this, gameinfo]
		{
			sys_log.notice("Booting from gamelist per context menu...");
			Q_EMIT RequestBoot(gameinfo);
		});
	}
	else
	{
		boot->setFont(font);
	}

	menu.addAction(boot);

	{
		QAction* boot_default = menu.addAction(is_current_running_game
			? tr("&Reboot with default configuration")
			: tr("&Boot with default configuration"));

		connect(boot_default, &QAction::triggered, [this, gameinfo]
		{
			sys_log.notice("Booting from gamelist per context menu...");
			Q_EMIT RequestBoot(gameinfo, cfg_mode::default_config);
		});

		QAction* boot_manual = menu.addAction(is_current_running_game
			? tr("&Reboot with manually selected configuration")
			: tr("&Boot with manually selected configuration"));

		connect(boot_manual, &QAction::triggered, [this, gameinfo]
		{
			if (std::string file_path = sstr(QFileDialog::getOpenFileName(this, "Select Config File", "", tr("Config Files (*.yml);;All files (*.*)"))); !file_path.empty())
			{
				sys_log.notice("Booting from gamelist per context menu...");
				Q_EMIT RequestBoot(gameinfo, cfg_mode::custom_selection, file_path);
			}
			else
			{
				sys_log.notice("Manual config selection aborted.");
			}
		});
	}

	extern bool is_savestate_compatible(fs::file&& file);

	if (const std::string sstate = get_savestate_file(current_game.serial, current_game.path, 0, 0); is_savestate_compatible(fs::file(sstate)))
	{
		QAction* boot_state = menu.addAction(is_current_running_game
			? tr("&Reboot with savestate")
			: tr("&Boot with savestate"));
		connect(boot_state, &QAction::triggered, [this, gameinfo, sstate]
		{
			sys_log.notice("Booting savestate from gamelist per context menu...");
			Q_EMIT RequestBoot(gameinfo, cfg_mode::custom, "", sstate);
		});
	}

	menu.addSeparator();

	QAction* configure = menu.addAction(gameinfo->hasCustomConfig
		? tr("&Change Custom Configuration")
		: tr("&Create Custom Configuration"));
	QAction* pad_configure = menu.addAction(gameinfo->hasCustomPadConfig
		? tr("&Change Custom Gamepad Configuration")
		: tr("&Create Custom Gamepad Configuration"));
	QAction* configure_patches = menu.addAction(tr("&Manage Game Patches"));
	QAction* create_ppu_cache = menu.addAction(tr("&Create PPU/SPU Cache"));

	menu.addSeparator();

	QMenu* shortcut_menu = menu.addMenu(tr("&Create Shortcut"));
	QAction* create_desktop_shortcut = shortcut_menu->addAction(tr("&Create Desktop Shortcut"));
	connect(create_desktop_shortcut, &QAction::triggered, this, [this, gameinfo](){ CreateShortcuts(gameinfo, { gui::utils::shortcut_location::desktop }); });
#ifdef _WIN32
	QAction* create_start_menu_shortcut = shortcut_menu->addAction(tr("&Create Start Menu Shortcut"));
#elif defined(__APPLE__)
	QAction* create_start_menu_shortcut = shortcut_menu->addAction(tr("&Create Launchpad Shortcut"));
#else
	QAction* create_start_menu_shortcut = shortcut_menu->addAction(tr("&Create Application Menu Shortcut"));
#endif
	connect(create_start_menu_shortcut, &QAction::triggered, this, [this, gameinfo](){ CreateShortcuts(gameinfo, { gui::utils::shortcut_location::applications }); });

	menu.addSeparator();

	QAction* rename_title = menu.addAction(tr("&Rename In Game List"));
	QAction* hide_serial = menu.addAction(tr("&Hide From Game List"));
	hide_serial->setCheckable(true);
	hide_serial->setChecked(m_hidden_list.contains(serial));
	menu.addSeparator();
	QMenu* remove_menu = menu.addMenu(tr("&Remove"));
	QAction* remove_game = remove_menu->addAction(tr("&Remove %1").arg(gameinfo->localized_category));
	remove_game->setEnabled(!is_current_running_game);
	if (gameinfo->hasCustomConfig)
	{
		QAction* remove_custom_config = remove_menu->addAction(tr("&Remove Custom Configuration"));
		connect(remove_custom_config, &QAction::triggered, [this, current_game, gameinfo]()
		{
			if (RemoveCustomConfiguration(current_game.serial, gameinfo, true))
			{
				ShowCustomConfigIcon(gameinfo);
			}
		});
	}
	if (gameinfo->hasCustomPadConfig)
	{
		QAction* remove_custom_pad_config = remove_menu->addAction(tr("&Remove Custom Gamepad Configuration"));
		connect(remove_custom_pad_config, &QAction::triggered, [this, current_game, gameinfo]()
		{
			if (RemoveCustomPadConfiguration(current_game.serial, gameinfo, true))
			{
				ShowCustomConfigIcon(gameinfo);
			}
		});
	}

	const bool has_cache_dir = fs::is_dir(cache_base_dir);

	if (has_cache_dir)
	{
		remove_menu->addSeparator();
		QAction* remove_shaders_cache = remove_menu->addAction(tr("&Remove Shaders Cache"));
		remove_shaders_cache->setEnabled(!is_current_running_game);
		connect(remove_shaders_cache, &QAction::triggered, [this, cache_base_dir]()
		{
			RemoveShadersCache(cache_base_dir, true);
		});
		QAction* remove_ppu_cache = remove_menu->addAction(tr("&Remove PPU Cache"));
		remove_ppu_cache->setEnabled(!is_current_running_game);
		connect(remove_ppu_cache, &QAction::triggered, [this, cache_base_dir]()
		{
			RemovePPUCache(cache_base_dir, true);
		});
		QAction* remove_spu_cache = remove_menu->addAction(tr("&Remove SPU Cache"));
		remove_spu_cache->setEnabled(!is_current_running_game);
		connect(remove_spu_cache, &QAction::triggered, [this, cache_base_dir]()
		{
			RemoveSPUCache(cache_base_dir, true);
		});
	}

	bool has_hdd1_cache = false;
	const std::string hdd1 = rpcs3::utils::get_hdd1_dir() + "/caches/";

	for (const auto& entry : fs::dir(hdd1))
	{
		if (entry.is_directory && entry.name.starts_with(current_game.serial))
		{
			has_hdd1_cache = true;
			break;
		}
	}

	if (has_hdd1_cache)
	{
		QAction* remove_hdd1_cache = remove_menu->addAction(tr("&Remove HDD1 Cache"));
		remove_hdd1_cache->setEnabled(!is_current_running_game);
		connect(remove_hdd1_cache, &QAction::triggered, [this, hdd1, serial = current_game.serial]()
		{
			RemoveHDD1Cache(hdd1, serial, true);
		});
	}

	if (has_cache_dir || has_hdd1_cache)
	{
		QAction* remove_all_caches = remove_menu->addAction(tr("&Remove All Caches"));
		remove_all_caches->setEnabled(!is_current_running_game);
		connect(remove_all_caches, &QAction::triggered, [this, current_game, cache_base_dir, hdd1]()
		{
			if (is_game_running(current_game.serial))
				return;

			if (QMessageBox::question(this, tr("Confirm Removal"), tr("Remove all caches?")) != QMessageBox::Yes)
				return;

			if (fs::remove_all(cache_base_dir))
				game_list_log.success("Removed cache directory: '%s'", cache_base_dir);
			else
				game_list_log.error("Could not remove cache directory: '%s' (%s)", cache_base_dir, fs::g_tls_error);

			RemoveHDD1Cache(hdd1, current_game.serial);
		});
	}

	menu.addSeparator();
	QAction* open_game_folder = menu.addAction(tr("&Open Install Folder"));
	if (gameinfo->hasCustomConfig)
	{
		QAction* open_config_dir = menu.addAction(tr("&Open Custom Config Folder"));
		connect(open_config_dir, &QAction::triggered, [current_game]()
		{
			const std::string config_path = rpcs3::utils::get_custom_config_path(current_game.serial);

			if (fs::is_file(config_path))
				gui::utils::open_dir(config_path);
		});
	}
	if (fs::is_dir(data_base_dir))
	{
		QAction* open_data_dir = menu.addAction(tr("&Open Data Folder"));
		connect(open_data_dir, &QAction::triggered, [data_base_dir]()
		{
			gui::utils::open_dir(data_base_dir);
		});
	}
	menu.addSeparator();
	QAction* check_compat = menu.addAction(tr("&Check Game Compatibility"));
	QAction* download_compat = menu.addAction(tr("&Download Compatibility Database"));
	menu.addSeparator();
	QAction* edit_notes = menu.addAction(tr("&Edit Tooltip Notes"));
	QAction* reset_time_played = menu.addAction(tr("&Reset Time Played"));

	QMenu* icon_menu = menu.addMenu(tr("&Custom Images"));
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

	if (const std::string custom_icon_dir_path = fs::get_config_dir() + "/Icons/game_icons/" + current_game.serial;
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
				icon_path = QFileDialog::getOpenFileName(this, msg, "", tr("%0 (*.%0);;All files (*.*)").arg(suffix));
			}
			if (action == icon_action::remove || !icon_path.isEmpty())
			{
				bool refresh = false;

				QString msg;
				switch (type)
				{
				case icon_type::game_list:
					msg = tr("Remove Custom Icon of %0?").arg(serial);
					break;
				case icon_type::hover_gif:
					msg = tr("Remove Custom Hover Gif of %0?").arg(serial);
					break;
				case icon_type::shader_load:
					msg = tr("Remove Custom Shader Loading Background of %0?").arg(serial);
					break;
				}

				if (action == icon_action::replace || (action == icon_action::remove &&
					QMessageBox::question(this, tr("Confirm Removal"), msg) == QMessageBox::Yes))
				{
					if (QFile file(game_icon_path); file.exists() && !file.remove())
					{
						game_list_log.error("Could not remove old file: '%s'", game_icon_path, file.errorString());
						QMessageBox::warning(this, tr("Warning!"), tr("Failed to remove the old file!"));
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
						QMessageBox::warning(this, tr("Warning!"), tr("Failed to import the new file!"));
					}
					else
					{
						game_list_log.success("Imported file '%s' to '%s'", icon_path, game_icon_path);
						refresh = true;
					}
				}

				if (refresh)
				{
					Refresh(true);
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
			const QString icon_path = qstr(custom_icon_dir_path) + icon_name;

			if (QFile::exists(icon_path))
			{
				actions[static_cast<int>(icon_action::add)]->setVisible(false);
				connect(actions[static_cast<int>(icon_action::replace)], &QAction::triggered, this, [handle_icon, icon_path, t = type, s = suffix] { handle_icon(icon_path, s, icon_action::replace, t); });
				connect(actions[static_cast<int>(icon_action::remove)], &QAction::triggered, this, [handle_icon, icon_path, t = type, s = suffix] { handle_icon(icon_path, s, icon_action::remove, t); });
			}
			else
			{
				connect(actions[static_cast<int>(icon_action::add)], &QAction::triggered, this, [handle_icon, icon_path, t = type, s = suffix] { handle_icon(icon_path, s, icon_action::add, t); });
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

	QMenu* info_menu = menu.addMenu(tr("&Copy Info"));
	QAction* copy_info = info_menu->addAction(tr("&Copy Name + Serial"));
	QAction* copy_name = info_menu->addAction(tr("&Copy Name"));
	QAction* copy_serial = info_menu->addAction(tr("&Copy Serial"));

	connect(boot, &QAction::triggered, this, [this, gameinfo]()
	{
		sys_log.notice("Booting from gamelist per context menu...");
		Q_EMIT RequestBoot(gameinfo, cfg_mode::global);
	});
	connect(configure, &QAction::triggered, this, [this, current_game, gameinfo]()
	{
		settings_dialog dlg(m_gui_settings, m_emu_settings, 0, this, &current_game);
		connect(&dlg, &settings_dialog::EmuSettingsApplied, [this, gameinfo]()
		{
			if (!gameinfo->hasCustomConfig)
			{
				gameinfo->hasCustomConfig = true;
				ShowCustomConfigIcon(gameinfo);
			}
			Q_EMIT NotifyEmuSettingsChange();
		});
		dlg.exec();
	});
	connect(pad_configure, &QAction::triggered, this, [this, current_game, gameinfo]()
	{
		pad_settings_dialog dlg(m_gui_settings, this, &current_game);

		if (dlg.exec() == QDialog::Accepted && !gameinfo->hasCustomPadConfig)
		{
			gameinfo->hasCustomPadConfig = true;
			ShowCustomConfigIcon(gameinfo);
		}
	});
	connect(hide_serial, &QAction::triggered, this, [serial, this](bool checked)
	{
		if (checked)
			m_hidden_list.insert(serial);
		else
			m_hidden_list.remove(serial);

		m_gui_settings->SetValue(gui::gl_hidden_list, QStringList(m_hidden_list.values()));
		Refresh();
	});
	connect(create_ppu_cache, &QAction::triggered, this, [gameinfo, this]
	{
		if (m_gui_settings->GetBootConfirmation(this))
		{
			CreateCPUCaches(gameinfo);
		}
	});
	connect(remove_game, &QAction::triggered, this, [this, current_game, gameinfo, cache_base_dir, name]
	{
		if (current_game.path.empty())
		{
			game_list_log.fatal("Cannot remove game. Path is empty");
			return;
		}

		if (is_game_running(current_game.serial))
		{
			QMessageBox::critical(this, tr("Cannot Remove Game"), tr("The PS3 application is still running, it cannot be removed!"));
			return;
		}

		QString size_information;

		if (current_game.size_on_disk != umax)
		{
			fs::device_stat stat{};
			if (fs::statfs(current_game.path, stat))
			{
				size_information = tr("Game Directory Size: %0\nCurrent Free Disk Space: %1\n\n").arg(gui::utils::format_byte_size(current_game.size_on_disk)).arg(gui::utils::format_byte_size(stat.avail_free));
			}
		}

		QMessageBox mb(QMessageBox::Question, tr("Confirm %1 Removal").arg(gameinfo->localized_category), tr("Permanently remove %0 from drive?\n%1Path: %2").arg(name).arg(size_information).arg(qstr(current_game.path)), QMessageBox::Yes | QMessageBox::No, this);
		mb.setCheckBox(new QCheckBox(tr("Remove caches and custom configs")));

		if (mb.exec() == QMessageBox::Yes)
		{
			const bool remove_caches = mb.checkBox()->isChecked();
			if (fs::remove_all(current_game.path))
			{
				if (remove_caches)
				{
					if (fs::is_dir(cache_base_dir))
					{
						if (fs::remove_all(cache_base_dir))
							game_list_log.notice("Removed cache directory: '%s'", cache_base_dir);
						else
							game_list_log.error("Could not remove cache directory: '%s' (%s)", cache_base_dir, fs::g_tls_error);
					}

					RemoveCustomConfiguration(current_game.serial);
					RemoveCustomPadConfiguration(current_game.serial);
				}
				m_game_data.erase(std::remove(m_game_data.begin(), m_game_data.end(), gameinfo), m_game_data.end());
				game_list_log.success("Removed %s %s in %s", gameinfo->localized_category, current_game.name, current_game.path);
				Refresh(true);
			}
			else
			{
				game_list_log.error("Failed to remove %s %s in %s (%s)", gameinfo->localized_category, current_game.name, current_game.path, fs::g_tls_error);
				QMessageBox::critical(this, tr("Failure!"), remove_caches
					? tr("Failed to remove %0 from drive!\nPath: %1\nCaches and custom configs have been left intact.").arg(name).arg(qstr(current_game.path))
					: tr("Failed to remove %0 from drive!\nPath: %1").arg(name).arg(qstr(current_game.path)));
			}
		}
	});
	connect(configure_patches, &QAction::triggered, this, [this, gameinfo]()
	{
		std::unordered_map<std::string, std::set<std::string>> games;
		for (const auto& game : m_game_data)
		{
			if (game)
			{
				games[game->info.serial].insert(game_list::GetGameVersion(game));
			}
		}
		patch_manager_dialog patch_manager(m_gui_settings, games, gameinfo->info.serial, game_list::GetGameVersion(gameinfo), this);
		patch_manager.exec();
	});
	connect(open_game_folder, &QAction::triggered, this, [current_game]()
	{
		gui::utils::open_dir(current_game.path);
	});
	connect(check_compat, &QAction::triggered, this, [serial]
	{
		const QString link = "https://rpcs3.net/compatibility?g=" + serial;
		QDesktopServices::openUrl(QUrl(link));
	});
	connect(download_compat, &QAction::triggered, this, [this]
	{
		m_game_compat->RequestCompatibility(true);
	});
	connect(rename_title, &QAction::triggered, this, [this, name, serial, global_pos]
	{
		const QString custom_title = m_persistent_settings->GetValue(gui::persistent::titles, serial, "").toString();
		const QString old_title = custom_title.isEmpty() ? name : custom_title;

		input_dialog dlg(128, old_title, tr("Rename Title"), tr("%0\n%1\n\nYou can clear the line in order to use the original title.").arg(name).arg(serial), name, this);
		dlg.move(global_pos);

		if (dlg.exec() == QDialog::Accepted)
		{
			const QString new_title = dlg.get_input_text().simplified();

			if (new_title.isEmpty() || new_title == name)
			{
				m_titles.remove(serial);
				m_persistent_settings->RemoveValue(gui::persistent::titles, serial);
			}
			else
			{
				m_titles.insert(serial, new_title);
				m_persistent_settings->SetValue(gui::persistent::titles, serial, new_title);
			}
			Refresh(true); // full refresh in order to reliably sort the list
		}
	});
	connect(edit_notes, &QAction::triggered, this, [this, name, serial]
	{
		bool accepted;
		const QString old_notes = m_persistent_settings->GetValue(gui::persistent::notes, serial, "").toString();
		const QString new_notes = QInputDialog::getMultiLineText(this, tr("Edit Tooltip Notes"), tr("%0\n%1").arg(name).arg(serial), old_notes, &accepted);

		if (accepted)
		{
			if (new_notes.simplified().isEmpty())
			{
				m_notes.remove(serial);
				m_persistent_settings->RemoveValue(gui::persistent::notes, serial);
			}
			else
			{
				m_notes.insert(serial, new_notes);
				m_persistent_settings->SetValue(gui::persistent::notes, serial, new_notes);
			}
			Refresh();
		}
	});
	connect(reset_time_played, &QAction::triggered, this, [this, name, serial]
	{
		if (QMessageBox::question(this, tr("Confirm Reset"), tr("Reset time played?\n\n%0 [%1]").arg(name).arg(serial)) == QMessageBox::Yes)
		{
			m_persistent_settings->SetPlaytime(serial, 0);
			m_persistent_settings->SetLastPlayed(serial, 0);
			m_persistent_settings->sync();
			Refresh();
		}
	});
	connect(copy_info, &QAction::triggered, this, [name, serial]
	{
		QApplication::clipboard()->setText(name % QStringLiteral(" [") % serial % QStringLiteral("]"));
	});
	connect(copy_name, &QAction::triggered, this, [name]
	{
		QApplication::clipboard()->setText(name);
	});
	connect(copy_serial, &QAction::triggered, this, [serial]
	{
		QApplication::clipboard()->setText(serial);
	});

	// Disable options depending on software category
	const QString category = qstr(current_game.category);

	if (category == cat::cat_disc_game || category == cat::cat_ps3_os)
	{
		remove_game->setEnabled(false);
	}
	else if (category != cat::cat_hdd_game)
	{
		check_compat->setEnabled(false);
	}

	menu.exec(global_pos);
}

bool game_list_frame::CreateCPUCaches(const std::string& path, const std::string& serial)
{
	Emu.GracefulShutdown(false);
	Emu.SetForceBoot(true);

	if (const auto error = Emu.BootGame(fs::is_file(path) ? fs::get_parent_dir(path) : path, serial, true); error != game_boot_result::no_errors)
	{
		game_list_log.error("Could not create PPU and SPU caches for %s, error: %s", path, error);
		return false;
	}

	game_list_log.warning("Creating PPU/SPU Caches for %s", path);
	return true;
}

bool game_list_frame::CreateCPUCaches(const game_info& game)
{
	return game && CreateCPUCaches(game->info.path, game->info.serial);
}

bool game_list_frame::RemoveCustomConfiguration(const std::string& title_id, const game_info& game, bool is_interactive)
{
	const std::string path = rpcs3::utils::get_custom_config_path(title_id);

	if (!fs::is_file(path))
		return true;

	if (is_interactive && QMessageBox::question(this, tr("Confirm Removal"), tr("Remove custom game configuration?")) != QMessageBox::Yes)
		return true;

	bool result = true;

	if (fs::is_file(path))
	{
		if (fs::remove_file(path))
		{
			if (game)
			{
				game->hasCustomConfig = false;
			}
			game_list_log.success("Removed configuration file: %s", path);
		}
		else
		{
			game_list_log.fatal("Failed to remove configuration file: %s\nError: %s", path, fs::g_tls_error);
			result = false;
		}
	}

	if (is_interactive && !result)
	{
		QMessageBox::warning(this, tr("Warning!"), tr("Failed to remove configuration file!"));
	}

	return result;
}

bool game_list_frame::RemoveCustomPadConfiguration(const std::string& title_id, const game_info& game, bool is_interactive)
{
	if (title_id.empty())
		return true;

	const std::string config_dir = rpcs3::utils::get_input_config_dir(title_id);

	if (!fs::is_dir(config_dir))
		return true;

	if (is_interactive && QMessageBox::question(this, tr("Confirm Removal"), (!Emu.IsStopped() && Emu.GetTitleID() == title_id)
		? tr("Remove custom pad configuration?\nYour configuration will revert to the global pad settings.")
		: tr("Remove custom pad configuration?")) != QMessageBox::Yes)
		return true;

	g_cfg_input_configs.load();
	g_cfg_input_configs.active_configs.erase(title_id);
	g_cfg_input_configs.save();
	game_list_log.notice("Removed active input configuration entry for key '%s'", title_id);

	if (QDir(qstr(config_dir)).removeRecursively())
	{
		if (game)
		{
			game->hasCustomPadConfig = false;
		}
		if (!Emu.IsStopped() && Emu.GetTitleID() == title_id)
		{
			pad::set_enabled(false);
			pad::reset(title_id);
			pad::set_enabled(true);
		}
		game_list_log.notice("Removed pad configuration directory: %s", config_dir);
		return true;
	}

	if (is_interactive)
	{
		QMessageBox::warning(this, tr("Warning!"), tr("Failed to completely remove pad configuration directory!"));
		game_list_log.fatal("Failed to completely remove pad configuration directory: %s\nError: %s", config_dir, fs::g_tls_error);
	}
	return false;
}

bool game_list_frame::RemoveShadersCache(const std::string& base_dir, bool is_interactive)
{
	if (!fs::is_dir(base_dir))
		return true;

	if (is_interactive && QMessageBox::question(this, tr("Confirm Removal"), tr("Remove shaders cache?")) != QMessageBox::Yes)
		return true;

	u32 caches_removed = 0;
	u32 caches_total   = 0;

	const QStringList filter{ QStringLiteral("shaders_cache") };
	const QString q_base_dir = qstr(base_dir);

	QDirIterator dir_iter(q_base_dir, filter, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

	while (dir_iter.hasNext())
	{
		const QString filepath = dir_iter.next();

		if (QDir(filepath).removeRecursively())
		{
			++caches_removed;
			game_list_log.notice("Removed shaders cache dir: %s", filepath);
		}
		else
		{
			game_list_log.warning("Could not completely remove shaders cache dir: %s", filepath);
		}

		++caches_total;
	}

	const bool success = caches_total == caches_removed;

	if (success)
		game_list_log.success("Removed shaders cache in %s", base_dir);
	else
		game_list_log.fatal("Only %d/%d shaders cache dirs could be removed in %s", caches_removed, caches_total, base_dir);

	if (QDir(q_base_dir).isEmpty())
	{
		if (fs::remove_dir(base_dir))
			game_list_log.notice("Removed empty shader cache directory: %s", base_dir);
		else
			game_list_log.error("Could not remove empty shader cache directory: '%s' (%s)", base_dir, fs::g_tls_error);
	}

	return success;
}

bool game_list_frame::RemovePPUCache(const std::string& base_dir, bool is_interactive)
{
	if (!fs::is_dir(base_dir))
		return true;

	if (is_interactive && QMessageBox::question(this, tr("Confirm Removal"), tr("Remove PPU cache?")) != QMessageBox::Yes)
		return true;

	u32 files_removed = 0;
	u32 files_total = 0;

	const QStringList filter{ QStringLiteral("v*.obj"), QStringLiteral("v*.obj.gz") };
	const QString q_base_dir = qstr(base_dir);

	QDirIterator dir_iter(q_base_dir, filter, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

	while (dir_iter.hasNext())
	{
		const QString filepath = dir_iter.next();

		if (QFile::remove(filepath))
		{
			++files_removed;
			game_list_log.notice("Removed PPU cache file: %s", filepath);
		}
		else
		{
			game_list_log.warning("Could not remove PPU cache file: %s", filepath);
		}

		++files_total;
	}

	const bool success = files_total == files_removed;

	if (success)
		game_list_log.success("Removed PPU cache in %s", base_dir);
	else
		game_list_log.fatal("Only %d/%d PPU cache files could be removed in %s", files_removed, files_total, base_dir);

	if (QDir(q_base_dir).isEmpty())
	{
		if (fs::remove_dir(base_dir))
			game_list_log.notice("Removed empty PPU cache directory: %s", base_dir);
		else
			game_list_log.error("Could not remove empty PPU cache directory: '%s' (%s)", base_dir, fs::g_tls_error);
	}

	return success;
}

bool game_list_frame::RemoveSPUCache(const std::string& base_dir, bool is_interactive)
{
	if (!fs::is_dir(base_dir))
		return true;

	if (is_interactive && QMessageBox::question(this, tr("Confirm Removal"), tr("Remove SPU cache?")) != QMessageBox::Yes)
		return true;

	u32 files_removed = 0;
	u32 files_total = 0;

	const QStringList filter{ QStringLiteral("spu*.dat"), QStringLiteral("spu*.dat.gz"), QStringLiteral("spu*.obj"), QStringLiteral("spu*.obj.gz") };
	const QString q_base_dir = qstr(base_dir);

	QDirIterator dir_iter(q_base_dir, filter, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

	while (dir_iter.hasNext())
	{
		const QString filepath = dir_iter.next();

		if (QFile::remove(filepath))
		{
			++files_removed;
			game_list_log.notice("Removed SPU cache file: %s", filepath);
		}
		else
		{
			game_list_log.warning("Could not remove SPU cache file: %s", filepath);
		}

		++files_total;
	}

	const bool success = files_total == files_removed;

	if (success)
		game_list_log.success("Removed SPU cache in %s", base_dir);
	else
		game_list_log.fatal("Only %d/%d SPU cache files could be removed in %s", files_removed, files_total, base_dir);

	if (QDir(q_base_dir).isEmpty())
	{
		if (fs::remove_dir(base_dir))
			game_list_log.notice("Removed empty SPU cache directory: %s", base_dir);
		else
			game_list_log.error("Could not remove empty SPU cache directory: '%s' (%s)", base_dir, fs::g_tls_error);
	}

	return success;
}

void game_list_frame::RemoveHDD1Cache(const std::string& base_dir, const std::string& title_id, bool is_interactive)
{
	if (!fs::is_dir(base_dir))
		return;

	if (is_interactive && QMessageBox::question(this, tr("Confirm Removal"), tr("Remove HDD1 cache?")) != QMessageBox::Yes)
		return;

	u32 dirs_removed = 0;
	u32 dirs_total = 0;

	const QString q_base_dir = qstr(base_dir);

	const QStringList filter{ qstr(title_id + "_*") };

	QDirIterator dir_iter(q_base_dir, filter, QDir::Dirs | QDir::NoDotAndDotDot);

	while (dir_iter.hasNext())
	{
		const QString filepath = dir_iter.next();

		if (fs::remove_all(filepath.toStdString()))
		{
			++dirs_removed;
			game_list_log.notice("Removed HDD1 cache directory: %s", filepath);
		}
		else
		{
			game_list_log.warning("Could not remove HDD1 cache directory: %s", filepath);
		}

		++dirs_total;
	}

	const bool success = dirs_removed == dirs_total;

	if (success)
		game_list_log.success("Removed HDD1 cache in %s (%s)", base_dir, title_id);
	else
		game_list_log.fatal("Only %d/%d HDD1 cache directories could be removed in %s (%s)", dirs_removed, dirs_total, base_dir, title_id);
}

void game_list_frame::BatchCreateCPUCaches()
{
	const std::string vsh_path = g_cfg_vfs.get_dev_flash() + "vsh/module/";
	const bool vsh_exists = fs::is_file(vsh_path + "vsh.self");
	const u32 total = m_game_data.size() + (vsh_exists ? 1 : 0);

	if (total == 0)
	{
		QMessageBox::information(this, tr("PPU Cache Batch Creation"), tr("No titles found"), QMessageBox::Ok);
		return;
	}

	if (!m_gui_settings->GetBootConfirmation(this))
	{
		return;
	}

	const QString main_label = tr("Creating all PPU caches");

	progress_dialog* pdlg = new progress_dialog(tr("PPU Cache Batch Creation"), main_label, tr("Cancel"), 0, total, true, this);
	pdlg->setAutoClose(false);
	pdlg->setAutoReset(false);
	pdlg->show();
	QApplication::processEvents();

	u32 created = 0;

	const auto wait_until_compiled = [pdlg]() -> bool
	{
		while (!Emu.IsStopped())
		{
			if (pdlg->wasCanceled())
			{
				return false;
			}
			QApplication::processEvents();
		}
		return true;
	};

	if (vsh_exists)
	{
		pdlg->setLabelText(tr("%0\nProgress: %1/%2. Compiling caches for VSH...", "Second line after main label").arg(main_label).arg(created).arg(total));
		QApplication::processEvents();

		if (CreateCPUCaches(vsh_path) && wait_until_compiled())
		{
			pdlg->SetValue(++created);
		}
	}

	for (const auto& game : m_game_data)
	{
		if (pdlg->wasCanceled() || g_system_progress_canceled)
		{
			break;
		}

		pdlg->setLabelText(tr("%0\nProgress: %1/%2. Compiling caches for %3...", "Second line after main label").arg(main_label).arg(created).arg(total).arg(qstr(game->info.serial)));
		QApplication::processEvents();

		if (CreateCPUCaches(game) && wait_until_compiled())
		{
			pdlg->SetValue(++created);
		}
	}

	if (pdlg->wasCanceled() || g_system_progress_canceled)
	{
		game_list_log.notice("PPU Cache Batch Creation was canceled");

		if (!Emu.IsStopped())
		{
			QApplication::processEvents();
			Emu.GracefulShutdown(false);
		}

		if (!pdlg->wasCanceled())
		{
			pdlg->close();
		}
		return;
	}

	pdlg->setLabelText(tr("Created PPU Caches for %n title(s)", "", created));
	pdlg->setCancelButtonText(tr("OK"));
	QApplication::beep();
}

void game_list_frame::BatchRemovePPUCaches()
{
	if (Emu.GetStatus(false) != system_state::stopped)
	{
		return;
	}

	std::set<std::string> serials;
	for (const auto& game : m_game_data)
	{
		serials.emplace(game->info.serial);
	}
	const u32 total = ::size32(serials);

	if (total == 0)
	{
		QMessageBox::information(this, tr("PPU Cache Batch Removal"), tr("No files found"), QMessageBox::Ok);
		return;
	}

	progress_dialog* pdlg = new progress_dialog(tr("PPU Cache Batch Removal"), tr("Removing all PPU caches"), tr("Cancel"), 0, total, true, this);
	pdlg->setAutoClose(false);
	pdlg->setAutoReset(false);
	pdlg->show();

	u32 removed = 0;
	for (const auto& serial : serials)
	{
		if (pdlg->wasCanceled())
		{
			game_list_log.notice("PPU Cache Batch Removal was canceled");
			break;
		}
		QApplication::processEvents();

		if (RemovePPUCache(GetCacheDirBySerial(serial)))
		{
			pdlg->SetValue(++removed);
		}
	}

	pdlg->setLabelText(tr("%0/%1 caches cleared").arg(removed).arg(total));
	pdlg->setCancelButtonText(tr("OK"));
	QApplication::beep();
}

void game_list_frame::BatchRemoveSPUCaches()
{
	if (Emu.GetStatus(false) != system_state::stopped)
	{
		return;
	}

	std::set<std::string> serials;
	for (const auto& game : m_game_data)
	{
		serials.emplace(game->info.serial);
	}
	const u32 total = ::size32(serials);

	if (total == 0)
	{
		QMessageBox::information(this, tr("SPU Cache Batch Removal"), tr("No files found"), QMessageBox::Ok);
		return;
	}

	progress_dialog* pdlg = new progress_dialog(tr("SPU Cache Batch Removal"), tr("Removing all SPU caches"), tr("Cancel"), 0, total, true, this);
	pdlg->setAutoClose(false);
	pdlg->setAutoReset(false);
	pdlg->show();

	u32 removed = 0;
	for (const auto& serial : serials)
	{
		if (pdlg->wasCanceled())
		{
			game_list_log.notice("SPU Cache Batch Removal was canceled. %d/%d folders cleared", removed, total);
			break;
		}
		QApplication::processEvents();

		if (RemoveSPUCache(GetCacheDirBySerial(serial)))
		{
			pdlg->SetValue(++removed);
		}
	}

	pdlg->setLabelText(tr("%0/%1 caches cleared").arg(removed).arg(total));
	pdlg->setCancelButtonText(tr("OK"));
	QApplication::beep();
}

void game_list_frame::BatchRemoveCustomConfigurations()
{
	std::set<std::string> serials;
	for (const auto& game : m_game_data)
	{
		if (game->hasCustomConfig && !serials.count(game->info.serial))
		{
			serials.emplace(game->info.serial);
		}
	}
	const u32 total = ::size32(serials);

	if (total == 0)
	{
		QMessageBox::information(this, tr("Custom Configuration Batch Removal"), tr("No files found"), QMessageBox::Ok);
		return;
	}

	progress_dialog* pdlg = new progress_dialog(tr("Custom Configuration Batch Removal"), tr("Removing all custom configurations"), tr("Cancel"), 0, total, true, this);
	pdlg->setAutoClose(false);
	pdlg->setAutoReset(false);
	pdlg->show();

	u32 removed = 0;
	for (const auto& serial : serials)
	{
		if (pdlg->wasCanceled())
		{
			game_list_log.notice("Custom Configuration Batch Removal was canceled. %d/%d custom configurations cleared", removed, total);
			break;
		}
		QApplication::processEvents();

		if (RemoveCustomConfiguration(serial))
		{
			pdlg->SetValue(++removed);
		}
	}

	pdlg->setLabelText(tr("%0/%1 custom configurations cleared").arg(removed).arg(total));
	pdlg->setCancelButtonText(tr("OK"));
	QApplication::beep();
	Refresh(true);
}

void game_list_frame::BatchRemoveCustomPadConfigurations()
{
	std::set<std::string> serials;
	for (const auto& game : m_game_data)
	{
		if (game->hasCustomPadConfig && !serials.count(game->info.serial))
		{
			serials.emplace(game->info.serial);
		}
	}
	const u32 total = ::size32(serials);

	if (total == 0)
	{
		QMessageBox::information(this, tr("Custom Pad Configuration Batch Removal"), tr("No files found"), QMessageBox::Ok);
		return;
	}

	progress_dialog* pdlg = new progress_dialog(tr("Custom Pad Configuration Batch Removal"), tr("Removing all custom pad configurations"), tr("Cancel"), 0, total, true, this);
	pdlg->setAutoClose(false);
	pdlg->setAutoReset(false);
	pdlg->show();

	u32 removed = 0;
	for (const auto& serial : serials)
	{
		if (pdlg->wasCanceled())
		{
			game_list_log.notice("Custom Pad Configuration Batch Removal was canceled. %d/%d custom pad configurations cleared", removed, total);
			break;
		}
		QApplication::processEvents();

		if (RemoveCustomPadConfiguration(serial))
		{
			pdlg->SetValue(++removed);
		}
	}

	pdlg->setLabelText(tr("%0/%1 custom pad configurations cleared").arg(removed).arg(total));
	pdlg->setCancelButtonText(tr("OK"));
	QApplication::beep();
	Refresh(true);
}

void game_list_frame::BatchRemoveShaderCaches()
{
	if (Emu.GetStatus(false) != system_state::stopped)
	{
		return;
	}

	std::set<std::string> serials;
	for (const auto& game : m_game_data)
	{
		serials.emplace(game->info.serial);
	}
	const u32 total = ::size32(serials);

	if (total == 0)
	{
		QMessageBox::information(this, tr("Shader Cache Batch Removal"), tr("No files found"), QMessageBox::Ok);
		return;
	}

	progress_dialog* pdlg = new progress_dialog(tr("Shader Cache Batch Removal"), tr("Removing all shader caches"), tr("Cancel"), 0, total, true, this);
	pdlg->setAutoClose(false);
	pdlg->setAutoReset(false);
	pdlg->show();

	u32 removed = 0;
	for (const auto& serial : serials)
	{
		if (pdlg->wasCanceled())
		{
			game_list_log.notice("Shader Cache Batch Removal was canceled");
			break;
		}
		QApplication::processEvents();

		if (RemoveShadersCache(GetCacheDirBySerial(serial)))
		{
			pdlg->SetValue(++removed);
		}
	}

	pdlg->setLabelText(tr("%0/%1 shader caches cleared").arg(removed).arg(total));
	pdlg->setCancelButtonText(tr("OK"));
	QApplication::beep();
}

void game_list_frame::ShowCustomConfigIcon(const game_info& game)
{
	if (!game)
	{
		return;
	}

	const std::string serial         = game->info.serial;
	const bool has_custom_config     = game->hasCustomConfig;
	const bool has_custom_pad_config = game->hasCustomPadConfig;

	for (const auto& other_game : m_game_data)
	{
		if (other_game->info.serial == serial)
		{
			other_game->hasCustomConfig    = has_custom_config;
			other_game->hasCustomPadConfig = has_custom_pad_config;
		}
	}

	m_game_list->set_custom_config_icon(game);

	RepaintIcons();
}

void game_list_frame::ResizeIcons(const int& slider_pos)
{
	m_icon_size_index = slider_pos;
	m_icon_size = gui_settings::SizeFromSlider(slider_pos);

	RepaintIcons();
}

void game_list_frame::RepaintIcons(const bool& from_settings)
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
			m_icon_color = gui::utils::get_label_color("gamelist_icon_background_color");
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

void game_list_frame::SetListMode(const bool& is_list)
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
	m_gui_settings->sync();

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
					QTableWidgetItem* item = m_game_list->item(m_game_list->currentRow(), static_cast<int>(gui::game_list_columns::icon));

					if (!item || !item->isSelected())
						return false;

					gameinfo = GetGameInfoFromItem(item);
				}
				else if (game_list_grid_item* item = static_cast<game_list_grid_item*>(m_game_grid->selected_item()))
				{
					gameinfo = item->game();
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
		QString title_name = m_titles.value(serial, name).toLower();

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

std::string game_list_frame::CurrentSelectionPath()
{
	std::string selection;

	game_info game{};

	if (m_old_layout_is_list)
	{
		if (!m_game_list->selectedItems().isEmpty())
		{
			if (QTableWidgetItem* item = m_game_list->item(m_game_list->currentRow(), 0))
			{
				if (const QVariant var = item->data(gui::game_role); var.canConvert<game_info>())
				{
					game = var.value<game_info>();
				}
			}
		}
	}
	else if (m_game_grid)
	{
		if (game_list_grid_item* item = static_cast<game_list_grid_item*>(m_game_grid->selected_item()))
		{
			game = item->game();
		}
	}

	if (game)
	{
		selection = game->info.path + game->info.icon_path;
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

const QList<game_info>& game_list_frame::GetGameInfo() const
{
	return m_game_data;
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
