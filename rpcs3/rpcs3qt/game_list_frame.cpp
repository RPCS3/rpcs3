#include "game_list_frame.h"
#include "qt_utils.h"
#include "settings_dialog.h"
#include "pad_settings_dialog.h"
#include "table_item_delegate.h"
#include "custom_table_widget_item.h"
#include "input_dialog.h"
#include "localized.h"
#include "progress_dialog.h"
#include "persistent_settings.h"
#include "emu_settings.h"
#include "gui_settings.h"
#include "game_list.h"
#include "game_list_grid.h"
#include "patch_manager_dialog.h"

#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/system_utils.hpp"
#include "Loader/PSF.h"
#include "util/types.hpp"
#include "Utilities/File.h"
#include "util/yaml.hpp"
#include "Input/pad_thread.h"

#include <algorithm>
#include <memory>
#include <set>
#include <regex>
#include <unordered_map>

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

	m_game_dock = new QMainWindow(this);
	m_game_dock->setWindowFlags(Qt::Widget);
	setWidget(m_game_dock);

	m_game_grid = new game_list_grid(QSize(), m_icon_color, m_margin_factor, m_text_factor, false);

	m_game_list = new game_list();
	m_game_list->setShowGrid(false);
	m_game_list->setItemDelegate(new table_item_delegate(this, true));
	m_game_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_game_list->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_game_list->setSelectionMode(QAbstractItemView::SingleSelection);
	m_game_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_game_list->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_game_list->verticalScrollBar()->installEventFilter(this);
	m_game_list->verticalScrollBar()->setSingleStep(20);
	m_game_list->horizontalScrollBar()->setSingleStep(20);
	m_game_list->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	m_game_list->verticalHeader()->setVisible(false);
	m_game_list->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	m_game_list->horizontalHeader()->setHighlightSections(false);
	m_game_list->horizontalHeader()->setSortIndicatorShown(true);
	m_game_list->horizontalHeader()->setStretchLastSection(true);
	m_game_list->horizontalHeader()->setDefaultSectionSize(150);
	m_game_list->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	m_game_list->setContextMenuPolicy(Qt::CustomContextMenu);
	m_game_list->setAlternatingRowColors(true);
	m_game_list->installEventFilter(this);
	m_game_list->setColumnCount(gui::column_count);
	m_game_list->setMouseTracking(true);

	m_game_compat = new game_compatibility(m_gui_settings, this);

	m_central_widget = new QStackedWidget(this);
	m_central_widget->addWidget(m_game_list);
	m_central_widget->addWidget(m_game_grid);
	m_central_widget->setCurrentWidget(m_is_list_layout ? m_game_list : m_game_grid);

	m_game_dock->setCentralWidget(m_central_widget);

	// Actions regarding showing/hiding columns
	auto add_column = [this](gui::game_list_columns col, const QString& header_text, const QString& action_text)
	{
		m_game_list->setHorizontalHeaderItem(col, new QTableWidgetItem(header_text));
		m_columnActs.append(new QAction(action_text, this));
	};

	add_column(gui::column_icon,       tr("Icon"),                  tr("Show Icons"));
	add_column(gui::column_name,       tr("Name"),                  tr("Show Names"));
	add_column(gui::column_serial,     tr("Serial"),                tr("Show Serials"));
	add_column(gui::column_firmware,   tr("Firmware"),              tr("Show Firmwares"));
	add_column(gui::column_version,    tr("Version"),               tr("Show Versions"));
	add_column(gui::column_category,   tr("Category"),              tr("Show Categories"));
	add_column(gui::column_path,       tr("Path"),                  tr("Show Paths"));
	add_column(gui::column_move,       tr("PlayStation Move"),      tr("Show PlayStation Move"));
	add_column(gui::column_resolution, tr("Supported Resolutions"), tr("Show Supported Resolutions"));
	add_column(gui::column_sound,      tr("Sound Formats"),         tr("Show Sound Formats"));
	add_column(gui::column_parental,   tr("Parental Level"),        tr("Show Parental Levels"));
	add_column(gui::column_last_play,  tr("Last Played"),           tr("Show Last Played"));
	add_column(gui::column_playtime,   tr("Time Played"),           tr("Show Time Played"));
	add_column(gui::column_compat,     tr("Compatibility"),         tr("Show Compatibility"));

	// Events
	connect(&m_refresh_watcher, &QFutureWatcher<void>::finished, this, &game_list_frame::OnRefreshFinished);
	connect(&m_refresh_watcher, &QFutureWatcher<void>::canceled, this, [this]()
	{
		if (m_repaint_watcher.isRunning())
		{
			m_repaint_watcher.cancel();
			m_repaint_watcher.waitForFinished();
		}

		m_path_list.clear();
		m_game_data.clear();
		m_serials.clear();
		m_games.pop_all();
	});
	connect(&m_repaint_watcher, &QFutureWatcher<movie_item*>::finished, this, &game_list_frame::OnRepaintFinished);
	connect(&m_repaint_watcher, &QFutureWatcher<movie_item*>::resultReadyAt, this, [this](int index)
	{
		if (!m_is_list_layout) return;
		if (movie_item* item = m_repaint_watcher.resultAt(index))
		{
			item->call_icon_func();
		}
	});

	connect(m_game_list, &QTableWidget::customContextMenuRequested, this, &game_list_frame::ShowContextMenu);
	connect(m_game_list, &QTableWidget::itemSelectionChanged, this, &game_list_frame::ItemSelectionChangedSlot);
	connect(m_game_list, &QTableWidget::itemDoubleClicked, this, &game_list_frame::doubleClickedSlot);

	connect(m_game_list->horizontalHeader(), &QHeaderView::sectionClicked, this, &game_list_frame::OnColClicked);
	connect(m_game_list->horizontalHeader(), &QHeaderView::customContextMenuRequested, [this](const QPoint& pos)
	{
		QMenu* configure = new QMenu(this);
		configure->addActions(m_columnActs);
		configure->exec(m_game_list->horizontalHeader()->viewport()->mapToGlobal(pos));
	});

	connect(m_game_grid, &QTableWidget::customContextMenuRequested, this, &game_list_frame::ShowContextMenu);
	connect(m_game_grid, &QTableWidget::itemSelectionChanged, this, &game_list_frame::ItemSelectionChangedSlot);
	connect(m_game_grid, &QTableWidget::itemDoubleClicked, this, &game_list_frame::doubleClickedSlot);

	connect(m_game_compat, &game_compatibility::DownloadStarted, [this]()
	{
		for (const auto& game : m_game_data)
		{
			game->compat = m_game_compat->GetStatusData("Download");
		}
		Refresh();
	});
	connect(m_game_compat, &game_compatibility::DownloadFinished, [this]()
	{
		for (const auto& game : m_game_data)
		{
			game->compat = m_game_compat->GetCompatibility(game->info.serial);
		}
		Refresh();
	});
	connect(m_game_compat, &game_compatibility::DownloadError, [this](const QString& error)
	{
		for (const auto& game : m_game_data)
		{
			game->compat = m_game_compat->GetCompatibility(game->info.serial);
		}
		Refresh();
		QMessageBox::warning(this, tr("Warning!"), tr("Failed to retrieve the online compatibility database!\nFalling back to local database.\n\n%0").arg(error));
	});

	for (int col = 0; col < m_columnActs.count(); ++col)
	{
		m_columnActs[col]->setCheckable(true);

		connect(m_columnActs[col], &QAction::triggered, [this, col](bool checked)
		{
			if (!checked) // be sure to have at least one column left so you can call the context menu at all time
			{
				int c = 0;
				for (int i = 0; i < m_columnActs.count(); ++i)
				{
					if (m_gui_settings->GetGamelistColVisibility(i) && ++c > 1)
						break;
				}
				if (c < 2)
				{
					m_columnActs[col]->setChecked(true); // re-enable the checkbox if we don't change the actual state
					return;
				}
			}
			m_game_list->setColumnHidden(col, !checked); // Negate because it's a set col hidden and we have menu say show.
			m_gui_settings->SetGamelistColVisibility(col, checked);

			if (checked) // handle hidden columns that have zero width after showing them (stuck between others)
			{
				FixNarrowColumns();
			}
		});
	}
}

void game_list_frame::LoadSettings()
{
	m_col_sort_order = m_gui_settings->GetValue(gui::gl_sortAsc).toBool() ? Qt::AscendingOrder : Qt::DescendingOrder;
	m_sort_column = m_gui_settings->GetValue(gui::gl_sortCol).toInt();
	m_category_filters = m_gui_settings->GetGameListCategoryFilters();
	m_draw_compat_status_to_grid = m_gui_settings->GetValue(gui::gl_draw_compat).toBool();
	m_show_custom_icons = m_gui_settings->GetValue(gui::gl_custom_icon).toBool();
	m_play_hover_movies = m_gui_settings->GetValue(gui::gl_hover_gifs).toBool();

	Refresh(true);

	const QByteArray state = m_gui_settings->GetValue(gui::gl_state).toByteArray();
	if (!m_game_list->horizontalHeader()->restoreState(state) && m_game_list->rowCount())
	{
		// If no settings exist, resize to contents.
		ResizeColumnsToContents();
	}

	for (int col = 0; col < m_columnActs.count(); ++col)
	{
		const bool vis = m_gui_settings->GetGamelistColVisibility(col);
		m_columnActs[col]->setChecked(vis);
		m_game_list->setColumnHidden(col, !vis);
	}

	SortGameList();
	FixNarrowColumns();

	m_game_list->horizontalHeader()->restoreState(m_game_list->horizontalHeader()->saveState());
}

game_list_frame::~game_list_frame()
{
	SaveSettings();
}

void game_list_frame::FixNarrowColumns() const
{
	qApp->processEvents();

	// handle columns (other than the icon column) that have zero width after showing them (stuck between others)
	for (int col = 1; col < m_columnActs.count(); ++col)
	{
		if (m_game_list->isColumnHidden(col))
		{
			continue;
		}

		if (m_game_list->columnWidth(col) <= m_game_list->horizontalHeader()->minimumSectionSize())
		{
			m_game_list->setColumnWidth(col, m_game_list->horizontalHeader()->minimumSectionSize());
		}
	}
}

void game_list_frame::ResizeColumnsToContents(int spacing) const
{
	if (!m_game_list)
	{
		return;
	}

	m_game_list->verticalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
	m_game_list->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);

	// Make non-icon columns slighty bigger for better visuals
	for (int i = 1; i < m_game_list->columnCount(); i++)
	{
		if (m_game_list->isColumnHidden(i))
		{
			continue;
		}

		const int size = m_game_list->horizontalHeader()->sectionSize(i) + spacing;
		m_game_list->horizontalHeader()->resizeSection(i, size);
	}
}

void game_list_frame::OnColClicked(int col)
{
	if (col == 0) return; // Don't "sort" icons.

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

	SortGameList();
}

// Get visibility of entries
bool game_list_frame::IsEntryVisible(const game_info& game)
{
	auto matches_category = [&]()
	{
		if (m_is_list_layout)
		{
			return m_category_filters.contains(qstr(game->info.category));
		}

		const auto cat_boot = Localized().category.cat_boot;
		return cat_boot.find(qstr(game->info.category)) != cat_boot.end();
	};

	const QString serial = qstr(game->info.serial);
	const bool is_visible = m_show_hidden || !m_hidden_list.contains(serial);
	return is_visible && matches_category() && SearchMatchesApp(qstr(game->info.name), serial);
}

void game_list_frame::SortGameList() const
{
	// Back-up old header sizes to handle unwanted column resize in case of zero search results
	QList<int> column_widths;
	const int old_row_count = m_game_list->rowCount();
	const int old_game_count = m_game_data.count();

	for (int i = 0; i < m_game_list->columnCount(); i++)
	{
		column_widths.append(m_game_list->columnWidth(i));
	}

	// Sorting resizes hidden columns, so unhide them as a workaround
	QList<int> columns_to_hide;

	for (int i = 0; i < m_game_list->columnCount(); i++)
	{
		if (m_game_list->isColumnHidden(i))
		{
			m_game_list->setColumnHidden(i, false);
			columns_to_hide << i;
		}
	}

	// Sort the list by column and sort order
	m_game_list->sortByColumn(m_sort_column, m_col_sort_order);

	// Hide columns again
	for (auto i : columns_to_hide)
	{
		m_game_list->setColumnHidden(i, true);
	}

	// Don't resize the columns if no game is shown to preserve the header settings
	if (!m_game_list->rowCount())
	{
		for (int i = 0; i < m_game_list->columnCount(); i++)
		{
			m_game_list->setColumnWidth(i, column_widths[i]);
		}

		m_game_list->horizontalHeader()->setSectionResizeMode(gui::column_icon, QHeaderView::Fixed);
		return;
	}

	// Fixate vertical header and row height
	m_game_list->verticalHeader()->setMinimumSectionSize(m_icon_size.height());
	m_game_list->verticalHeader()->setMaximumSectionSize(m_icon_size.height());
	m_game_list->resizeRowsToContents();

	// Resize columns if the game list was empty before
	if (!old_row_count && !old_game_count)
	{
		ResizeColumnsToContents();
	}
	else
	{
		m_game_list->resizeColumnToContents(gui::column_icon);
	}

	// Fixate icon column
	m_game_list->horizontalHeader()->setSectionResizeMode(gui::column_icon, QHeaderView::Fixed);

	// Shorten the last section to remove horizontal scrollbar if possible
	m_game_list->resizeColumnToContents(gui::column_count - 1);
}

QString game_list_frame::GetLastPlayedBySerial(const QString& serial) const
{
	return m_persistent_settings->GetLastPlayed(serial);
}

std::string game_list_frame::GetCacheDirBySerial(const std::string& serial)
{
	return rpcs3::utils::get_cache_dir() + serial;
}

std::string game_list_frame::GetDataDirBySerial(const std::string& serial)
{
	return fs::get_config_dir() + "data/" + serial;
}

void game_list_frame::Refresh(const bool from_drive, const bool scroll_after)
{
	if (m_repaint_watcher.isRunning())
	{
		m_repaint_watcher.cancel();
		m_repaint_watcher.waitForFinished();
	}

	if (m_refresh_watcher.isRunning())
	{
		if (from_drive)
		{
			m_refresh_watcher.cancel();
		}
		m_refresh_watcher.waitForFinished();
	}

	if (from_drive)
	{
		const Localized localized;

		m_path_list.clear();
		m_serials.clear();
		m_game_data.clear();
		m_notes.clear();
		m_games.pop_all();

		const std::string _hdd =  rpcs3::utils::get_hdd0_dir();

		const auto add_disc_dir = [&](const std::string& path)
		{
			for (const auto& entry : fs::dir(path))
			{
				if (!entry.is_directory || entry.name == "." || entry.name == "..")
				{
					continue;
				}

				if (entry.name == "PS3_GAME" || std::regex_match(entry.name, std::regex("^PS3_GM[[:digit:]]{2}$")))
				{
					m_path_list.emplace_back(path + "/" + entry.name);
				}
			}
		};

		const auto add_dir = [&](const std::string& path, bool is_disc)
		{
			for (const auto& entry : fs::dir(path))
			{
				if (!entry.is_directory || entry.name == "." || entry.name == "..")
				{
					continue;
				}

				const std::string entry_path = path + entry.name;

				if (fs::is_file(entry_path + "/PS3_DISC.SFB"))
				{
					if (!is_disc)
					{
						game_list_log.error("Invalid game path found in %s", entry_path);
					}
					else
					{
						add_disc_dir(entry_path);
					}
				}
				else
				{
					if (is_disc)
					{
						game_list_log.error("Invalid disc path found in %s", entry_path);
					}
					else
					{
						m_path_list.emplace_back(entry_path);
					}
				}
			}
		};

		add_dir(_hdd + "game/", false);
		add_dir(_hdd + "disc/", true);

		auto get_games = []() -> YAML::Node
		{
			if (const fs::file games = fs::file(fs::get_config_dir() + "/games.yml", fs::read + fs::create))
			{
				auto [result, error] = yaml_load(games.to_string());

				if (!error.empty())
				{
					game_list_log.error("Failed to load games.yml: %s", error);
					return {};
				}

				return result;
			}

			game_list_log.error("Failed to load games.yml, check permissions.");
			return {};
		};

		for (auto&& pair : get_games())
		{
			std::string game_dir = pair.second.Scalar();

			game_dir.resize(game_dir.find_last_not_of('/') + 1);

			if (fs::is_file(game_dir + "/PS3_DISC.SFB"))
			{
				// Check if a path loaded from games.yml is already registered in add_dir(_hdd + "disc/");
				if (game_dir.starts_with(_hdd))
				{
					std::string_view frag = std::string_view(game_dir).substr(_hdd.size());

					if (frag.starts_with("disc/"))
					{
						// Our path starts from _hdd + 'disc/'
						frag.remove_prefix(5);

						// Check if the remaining part is the only path component
						if (frag.find_first_of('/') + 1 == 0)
						{
							game_list_log.trace("Removed duplicate for %s: %s", pair.first.Scalar(), pair.second.Scalar());
							continue;
						}
					}
				}

				add_disc_dir(game_dir);
			}
			else
			{
				game_list_log.trace("Invalid disc path registered for %s: %s", pair.first.Scalar(), pair.second.Scalar());
			}
		}

		// Remove duplicates
		sort(m_path_list.begin(), m_path_list.end());
		m_path_list.erase(unique(m_path_list.begin(), m_path_list.end()), m_path_list.end());

		const std::string game_icon_path = m_play_hover_movies ? fs::get_config_dir() + "/Icons/game_icons/" : "";

		m_refresh_watcher.setFuture(QtConcurrent::map(m_path_list, [this, cat_unknown_localized = sstr(localized.category.unknown), cat_unknown = sstr(cat::cat_unknown), game_icon_path](const std::string& dir)
		{
			const Localized thread_localized;

			const std::string sfo_dir = rpcs3::utils::get_sfo_dir_from_game_path(dir);
			const psf::registry psf = psf::load_object(fs::file(sfo_dir + "/PARAM.SFO"));
			const std::string_view title_id = psf::get_string(psf, "TITLE_ID", "");

			if (title_id.empty())
			{
				// Do not care about invalid entries
				return;
			}

			GameInfo game;
			game.path         = dir;
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

			if (m_show_custom_icons)
			{
				game.icon_path = fs::get_config_dir() + "/Icons/game_icons/" + game.serial + "/ICON0.PNG";
			}

			if (!m_show_custom_icons || !fs::is_file(game.icon_path))
			{
				game.icon_path = sfo_dir + "/ICON0.PNG";
			}

			m_mutex_cat.lock();

			const QString serial = qstr(game.serial);

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

			auto qt_cat = qstr(game.category);

			if (const auto boot_cat = thread_localized.category.cat_boot.find(qt_cat); boot_cat != thread_localized.category.cat_boot.end())
			{
				qt_cat = boot_cat->second;
			}
			else if (const auto data_cat = thread_localized.category.cat_data.find(qt_cat); data_cat != thread_localized.category.cat_data.end())
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

			m_mutex_cat.unlock();

			const auto compat = m_game_compat->GetCompatibility(game.serial);
			const bool hasCustomConfig = fs::is_file(rpcs3::utils::get_custom_config_path(game.serial));
			const bool hasCustomPadConfig = fs::is_file(rpcs3::utils::get_custom_input_config_path(game.serial));
			const bool has_hover_gif = fs::is_file(game_icon_path + game.serial + "/hover.gif");

			m_games.push(std::make_shared<gui_game_info>(gui_game_info{game, qt_cat, compat, {}, {}, hasCustomConfig, hasCustomPadConfig, has_hover_gif, nullptr}));
		}));

		return;
	}

	// Fill Game List / Game Grid

	if (m_is_list_layout)
	{
		const int scroll_position = m_game_list->verticalScrollBar()->value();
		PopulateGameList();
		SortGameList();
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
		RepaintIcons();
	}
}

void game_list_frame::OnRefreshFinished()
{
	if (m_repaint_watcher.isRunning())
	{
		m_repaint_watcher.cancel();
		m_repaint_watcher.waitForFinished();
	}

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

	Refresh();
}

void game_list_frame::OnRepaintFinished()
{
	if (m_is_list_layout)
	{
		// Fixate vertical header and row height
		m_game_list->verticalHeader()->setMinimumSectionSize(m_icon_size.height());
		m_game_list->verticalHeader()->setMaximumSectionSize(m_icon_size.height());

		// Resize the icon column
		m_game_list->resizeColumnToContents(gui::column_icon);

		// Shorten the last section to remove horizontal scrollbar if possible
		m_game_list->resizeColumnToContents(gui::column_count - 1);
	}
	else
	{
		// The game grid needs to be recreated from scratch
		int games_per_row = 0;

		if (m_icon_size.width() > 0 && m_icon_size.height() > 0)
		{
			games_per_row = width() / (m_icon_size.width() + m_icon_size.width() * m_game_grid->getMarginFactor() * 2);
		}

		const int scroll_position = m_game_grid->verticalScrollBar()->value();
		PopulateGameGrid(games_per_row, m_icon_size, m_icon_color);
		connect(m_game_grid, &QTableWidget::customContextMenuRequested, this, &game_list_frame::ShowContextMenu);
		connect(m_game_grid, &QTableWidget::itemSelectionChanged, this, &game_list_frame::ItemSelectionChangedSlot);
		connect(m_game_grid, &QTableWidget::itemDoubleClicked, this, &game_list_frame::doubleClickedSlot);
		m_central_widget->addWidget(m_game_grid);
		m_central_widget->setCurrentWidget(m_game_grid);
		m_game_grid->verticalScrollBar()->setValue(scroll_position);
	}
}

void game_list_frame::ToggleCategoryFilter(const QStringList& categories, bool show)
{
	if (show)
	{
		m_category_filters.append(categories);
	}
	else
	{
		for (const auto& cat : categories)
		{
			m_category_filters.removeAll(cat);
		}
	}

	Refresh();
}

void game_list_frame::SaveSettings()
{
	for (int col = 0; col < m_columnActs.count(); ++col)
	{
		m_gui_settings->SetGamelistColVisibility(col, m_columnActs[col]->isChecked());
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

	const game_info game = GetGameInfoByMode(item);

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
		if (const auto item = m_game_list->item(m_game_list->currentRow(), gui::column_icon); item && item->isSelected())
		{
			game = GetGameInfoByMode(item);
		}
	}
	else if (const auto item = m_game_grid->currentItem(); item && item->isSelected())
	{
		game = GetGameInfoByMode(item);
	}

	Q_EMIT NotifyGameSelection(game);
}

void game_list_frame::ShowContextMenu(const QPoint &pos)
{
	QPoint global_pos;
	QTableWidgetItem* item;

	if (m_is_list_layout)
	{
		item = m_game_list->item(m_game_list->indexAt(pos).row(), gui::column_icon);
		global_pos = m_game_list->viewport()->mapToGlobal(pos);
	}
	else
	{
		const QModelIndex mi = m_game_grid->indexAt(pos);
		item = m_game_grid->item(mi.row(), mi.column());
		global_pos = m_game_grid->viewport()->mapToGlobal(pos);
	}

	game_info gameinfo = GetGameInfoFromItem(item);
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

	const bool is_current_running_game = (Emu.IsRunning() || Emu.IsPaused()) && current_game.serial == Emu.GetTitleID();

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
			Q_EMIT RequestBoot(gameinfo, cfg_keys::_default);
		});

		QAction* boot_manual = menu.addAction(is_current_running_game
			? tr("&Reboot with manually selected configuration")
			: tr("&Boot with manually selected configuration"));

		connect(boot_manual, &QAction::triggered, [this, gameinfo]
		{
			if (std::string file_path = sstr(QFileDialog::getOpenFileName(this, "Select Config File", "", tr("Config Files (*.yml);;All files (*.*)"))); !file_path.empty())
			{
				sys_log.notice("Booting from gamelist per context menu...");
				Q_EMIT RequestBoot(gameinfo, file_path);
			}
			else
			{
				sys_log.notice("Manual config selection aborted.");
			}
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
	QAction* create_ppu_cache = menu.addAction(tr("&Create PPU Cache"));
	menu.addSeparator();
	QAction* rename_title = menu.addAction(tr("&Rename In Game List"));
	QAction* hide_serial = menu.addAction(tr("&Hide From Game List"));
	hide_serial->setCheckable(true);
	hide_serial->setChecked(m_hidden_list.contains(serial));
	menu.addSeparator();
	QMenu* remove_menu = menu.addMenu(tr("&Remove"));
	QAction* remove_game = remove_menu->addAction(tr("&Remove %1").arg(gameinfo->localized_category));
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
	if (fs::is_dir(cache_base_dir))
	{
		remove_menu->addSeparator();
		QAction* remove_shaders_cache = remove_menu->addAction(tr("&Remove Shaders Cache"));
		connect(remove_shaders_cache, &QAction::triggered, [this, cache_base_dir]()
		{
			RemoveShadersCache(cache_base_dir, true);
		});
		QAction* remove_ppu_cache = remove_menu->addAction(tr("&Remove PPU Cache"));
		connect(remove_ppu_cache, &QAction::triggered, [this, cache_base_dir]()
		{
			RemovePPUCache(cache_base_dir, true);
		});
		QAction* remove_spu_cache = remove_menu->addAction(tr("&Remove SPU Cache"));
		connect(remove_spu_cache, &QAction::triggered, [this, cache_base_dir]()
		{
			RemoveSPUCache(cache_base_dir, true);
		});
		QAction* remove_all_caches = remove_menu->addAction(tr("&Remove All Caches"));
		connect(remove_all_caches, &QAction::triggered, [this, cache_base_dir]()
		{
			if (QMessageBox::question(this, tr("Confirm Removal"), tr("Remove all caches?")) != QMessageBox::Yes)
				return;

			RemoveShadersCache(cache_base_dir);
			RemovePPUCache(cache_base_dir);
			RemoveSPUCache(cache_base_dir);
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
						game_list_log.error("Could not remove old file: '%s'", sstr(game_icon_path), sstr(file.errorString()));
						QMessageBox::warning(this, tr("Warning!"), tr("Failed to remove the old file!"));
						return;
					}

					game_list_log.success("Removed file: '%s'", sstr(game_icon_path));
					if (action == icon_action::remove)
					{
						refresh = true;
					}
				}

				if (action != icon_action::remove)
				{
					if (!QFile::copy(icon_path, game_icon_path))
					{
						game_list_log.error("Could not import file '%s' to '%s'.", sstr(icon_path), sstr(game_icon_path));
						QMessageBox::warning(this, tr("Warning!"), tr("Failed to import the new file!"));
					}
					else
					{
						game_list_log.success("Imported file '%s' to '%s'", sstr(icon_path), sstr(game_icon_path));
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
		Q_EMIT RequestBoot(gameinfo, cfg_keys::global);
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
			CreatePPUCache(gameinfo);
		}
	});
	connect(remove_game, &QAction::triggered, this, [this, current_game, gameinfo, cache_base_dir, name]
	{
		if (current_game.path.empty())
		{
			game_list_log.fatal("Cannot remove game. Path is empty");
			return;
		}

		QMessageBox* mb = new QMessageBox(QMessageBox::Question, tr("Confirm %1 Removal").arg(gameinfo->localized_category), tr("Permanently remove %0 from drive?\nPath: %1").arg(name).arg(qstr(current_game.path)), QMessageBox::Yes | QMessageBox::No, this);
		mb->setCheckBox(new QCheckBox(tr("Remove caches and custom configs")));
		mb->deleteLater();
		if (mb->exec() == QMessageBox::Yes)
		{
			const bool remove_caches = mb->checkBox()->isChecked();
			if (fs::remove_all(current_game.path))
			{
				if (remove_caches)
				{
					RemoveShadersCache(cache_base_dir);
					RemovePPUCache(cache_base_dir);
					RemoveSPUCache(cache_base_dir);
					RemoveCustomConfiguration(current_game.serial);
					RemoveCustomPadConfiguration(current_game.serial);
				}
				m_game_data.erase(std::remove(m_game_data.begin(), m_game_data.end(), gameinfo), m_game_data.end());
				game_list_log.success("Removed %s %s in %s", sstr(gameinfo->localized_category), current_game.name, current_game.path);
				Refresh(true);
			}
			else
			{
				game_list_log.error("Failed to remove %s %s in %s (%s)", sstr(gameinfo->localized_category), current_game.name, current_game.path, fs::g_tls_error);
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
				games[game->info.serial].insert(game_list_frame::GetGameVersion(game));
			}
		}
		patch_manager_dialog patch_manager(m_gui_settings, games, gameinfo->info.serial, GetGameVersion(gameinfo), this);
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

	if (category == cat::cat_disc_game)
	{
		remove_game->setEnabled(false);
	}
	else if (category != cat::cat_hdd_game)
	{
		check_compat->setEnabled(false);
	}

	menu.exec(global_pos);
}

bool game_list_frame::CreatePPUCache(const std::string& path, const std::string& serial)
{
	Emu.SetForceBoot(true);
	Emu.Stop();
	Emu.SetForceBoot(true);

	if (const auto error = Emu.BootGame(path, serial, true); error != game_boot_result::no_errors)
	{
		game_list_log.error("Could not create PPU Cache for %s, error: %s", path, error);
		return false;
	}
	game_list_log.warning("Creating PPU Cache for %s", path);
	return true;
}

bool game_list_frame::CreatePPUCache(const game_info& game)
{
	return game && CreatePPUCache(game->info.path, game->info.serial);
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

	g_cfg_profile.load();
	g_cfg_profile.active_profiles.erase(title_id);
	g_cfg_profile.save();
	game_list_log.notice("Removed active pad profile entry for key '%s'", title_id);

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

	QDirIterator dir_iter(qstr(base_dir), filter, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

	while (dir_iter.hasNext())
	{
		const QString filepath = dir_iter.next();

		if (QDir(filepath).removeRecursively())
		{
			++caches_removed;
			game_list_log.notice("Removed shaders cache dir: %s", sstr(filepath));
		}
		else
		{
			game_list_log.warning("Could not completely remove shaders cache dir: %s", sstr(filepath));
		}

		++caches_total;
	}

	const bool success = caches_total == caches_removed;

	if (success)
		game_list_log.success("Removed shaders cache in %s", base_dir);
	else
		game_list_log.fatal("Only %d/%d shaders cache dirs could be removed in %s", caches_removed, caches_total, base_dir);

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

	QDirIterator dir_iter(qstr(base_dir), filter, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

	while (dir_iter.hasNext())
	{
		const QString filepath = dir_iter.next();

		if (QFile::remove(filepath))
		{
			++files_removed;
			game_list_log.notice("Removed PPU cache file: %s", sstr(filepath));
		}
		else
		{
			game_list_log.warning("Could not remove PPU cache file: %s", sstr(filepath));
		}

		++files_total;
	}

	const bool success = files_total == files_removed;

	if (success)
		game_list_log.success("Removed PPU cache in %s", base_dir);
	else
		game_list_log.fatal("Only %d/%d PPU cache files could be removed in %s", files_removed, files_total, base_dir);

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

	QDirIterator dir_iter(qstr(base_dir), filter, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

	while (dir_iter.hasNext())
	{
		const QString filepath = dir_iter.next();

		if (QFile::remove(filepath))
		{
			++files_removed;
			game_list_log.notice("Removed SPU cache file: %s", sstr(filepath));
		}
		else
		{
			game_list_log.warning("Could not remove SPU cache file: %s", sstr(filepath));
		}

		++files_total;
	}

	const bool success = files_total == files_removed;

	if (success)
		game_list_log.success("Removed SPU cache in %s", base_dir);
	else
		game_list_log.fatal("Only %d/%d SPU cache files could be removed in %s", files_removed, files_total, base_dir);

	return success;
}

void game_list_frame::BatchCreatePPUCaches()
{
	const std::string vsh_path = g_cfg.vfs.get_dev_flash() + "vsh/module/";
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

		if (CreatePPUCache(vsh_path) && wait_until_compiled())
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

		if (CreatePPUCache(game) && wait_until_compiled())
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
			Emu.Stop();
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

QPixmap game_list_frame::PaintedPixmap(const QPixmap& icon, bool paint_config_icon, bool paint_pad_config_icon, const QColor& compatibility_color) const
{
	const qreal device_pixel_ratio = devicePixelRatioF();
	QSize canvas_size(320, 176);
	QSize icon_size(icon.size());
	QPoint target_pos;

	if (!icon.isNull())
	{
		// Let's upscale the original icon to at least fit into the outer rect of the size of PS3's ICON0.PNG
		if (icon_size.width() < 320 || icon_size.height() < 176)
		{
			icon_size.scale(320, 176, Qt::KeepAspectRatio);
		}

		canvas_size = icon_size;

		// Calculate the centered size and position of the icon on our canvas.
		if (icon_size.width() != 320 || icon_size.height() != 176)
		{
			ensure(icon_size.height() > 0);
			constexpr double target_ratio = 320.0 / 176.0; // aspect ratio 20:11

			if ((icon_size.width() / static_cast<double>(icon_size.height())) > target_ratio)
			{
				canvas_size.setHeight(std::ceil(icon_size.width() / target_ratio));
			}
			else
			{
				canvas_size.setWidth(std::ceil(icon_size.height() * target_ratio));
			}

			target_pos.setX(std::max<int>(0, (canvas_size.width() - icon_size.width()) / 2.0));
			target_pos.setY(std::max<int>(0, (canvas_size.height() - icon_size.height()) / 2.0));
		}
	}

	// Create a canvas large enough to fit our entire scaled icon
	QPixmap canvas(canvas_size * device_pixel_ratio);
	canvas.setDevicePixelRatio(device_pixel_ratio);
	canvas.fill(m_icon_color);

	// Create a painter for our canvas
	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	// Draw the icon onto our canvas
	if (!icon.isNull())
	{
		painter.drawPixmap(target_pos.x(), target_pos.y(), icon_size.width(), icon_size.height(), icon);
	}

	// Draw config icons if necessary
	if (!m_is_list_layout && (paint_config_icon || paint_pad_config_icon))
	{
		const int width = canvas_size.width() * 0.2;
		const QPoint origin = QPoint(canvas_size.width() - width, 0);
		QString icon_path;

		if (paint_config_icon && paint_pad_config_icon)
		{
			icon_path = ":/Icons/combo_config_bordered.png";
		}
		else if (paint_config_icon)
		{
			icon_path = ":/Icons/custom_config.png";
		}
		else if (paint_pad_config_icon)
		{
			icon_path = ":/Icons/controllers.png";
		}

		QPixmap custom_config_icon(icon_path);
		custom_config_icon.setDevicePixelRatio(device_pixel_ratio);
		painter.drawPixmap(origin, custom_config_icon.scaled(QSize(width, width) * device_pixel_ratio, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
	}

	// Draw game compatibility icons if necessary
	if (compatibility_color.isValid())
	{
		const int size = canvas_size.height() * 0.2;
		const int spacing = canvas_size.height() * 0.05;
		QColor copyColor = QColor(compatibility_color);
		copyColor.setAlpha(215); // ~85% opacity
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setBrush(QBrush(copyColor));
		painter.setPen(QPen(Qt::black, std::max(canvas_size.width() / 320, canvas_size.height() / 176)));
		painter.drawEllipse(spacing, spacing, size, size);
	}

	// Finish the painting
	painter.end();

	// Scale and return our final image
	return canvas.scaled(m_icon_size * device_pixel_ratio, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
}

void game_list_frame::SetCustomConfigIcon(QTableWidgetItem* title_item, const game_info& game)
{
	if (!title_item || !game)
		return;

	static QIcon icon_combo_config_bordered(":/Icons/combo_config_bordered.png");
	static QIcon icon_custom_config(":/Icons/custom_config.png");
	static QIcon icon_controllers(":/Icons/controllers.png");

	if (game->hasCustomConfig && game->hasCustomPadConfig)
	{
		title_item->setIcon(icon_combo_config_bordered);
	}
	else if (game->hasCustomConfig)
	{
		title_item->setIcon(icon_custom_config);
	}
	else if (game->hasCustomPadConfig)
	{
		title_item->setIcon(icon_controllers);
	}
	else if (!title_item->icon().isNull())
	{
		title_item->setIcon({});
	}
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

	const QString q_serial = qstr(game->info.serial);

	for (int row = 0; row < m_game_list->rowCount(); ++row)
	{
		if (const auto item = m_game_list->item(row, gui::column_serial); item && item->text() == q_serial)
		{
			SetCustomConfigIcon(m_game_list->item(row, gui::column_name), game);
		}
	}

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
	if (m_repaint_watcher.isRunning())
	{
		m_repaint_watcher.cancel();
		m_repaint_watcher.waitForFinished();
	}

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
		QPixmap placeholder(m_icon_size);
		placeholder.fill(Qt::transparent);

		for (auto& game : m_game_data)
		{
			game->pxmap = placeholder;
			if (movie_item* item = game->item)
			{
				item->call_icon_func();
			}
		}

		// Fixate vertical header and row height
		m_game_list->verticalHeader()->setMinimumSectionSize(m_icon_size.height());
		m_game_list->verticalHeader()->setMaximumSectionSize(m_icon_size.height());

		// Resize the icon column
		m_game_list->resizeColumnToContents(gui::column_icon);

		// Shorten the last section to remove horizontal scrollbar if possible
		m_game_list->resizeColumnToContents(gui::column_count - 1);
	}

	const std::function func = [this](const game_info& game) -> movie_item*
	{
		if (game->icon.isNull() && (game->info.icon_path.empty() || !game->icon.load(qstr(game->info.icon_path))))
		{
			game_list_log.warning("Could not load image from path %s", sstr(QDir(qstr(game->info.icon_path)).absolutePath()));
		}
		const QColor color = getGridCompatibilityColor(game->compat.color);
		game->pxmap = PaintedPixmap(game->icon, game->hasCustomConfig, game->hasCustomPadConfig, color);
		return game->item;
	};
	m_repaint_watcher.setFuture(QtConcurrent::mapped(m_game_data, func));
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

	Refresh(true);

	m_central_widget->setCurrentWidget(m_is_list_layout ? m_game_list : m_game_grid);
}

void game_list_frame::SetSearchText(const QString& text)
{
	m_search_text = text;
	Refresh();
}

void game_list_frame::closeEvent(QCloseEvent *event)
{
	QDockWidget::closeEvent(event);
	Q_EMIT GameListFrameClosed();
}

void game_list_frame::resizeEvent(QResizeEvent *event)
{
	if (!m_is_list_layout)
	{
		Refresh(false, m_game_grid->selectedItems().count());
	}
	QDockWidget::resizeEvent(event);
}

bool game_list_frame::eventFilter(QObject *object, QEvent *event)
{
	// Zoom gamelist/gamegrid
	if (event->type() == QEvent::Wheel && (object == m_game_list->verticalScrollBar() || object == m_game_grid->verticalScrollBar()))
	{
		QWheelEvent *wheel_event = static_cast<QWheelEvent *>(event);

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
		QKeyEvent *key_event = static_cast<QKeyEvent *>(event);

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
				QTableWidgetItem* item;

				if (object == m_game_list)
					item = m_game_list->item(m_game_list->currentRow(), gui::column_icon);
				else
					item = m_game_grid->currentItem();

				if (!item || !item->isSelected())
					return false;

				const game_info gameinfo = GetGameInfoFromItem(item);

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
 Cleans and readds entries to table widget in UI.
*/
void game_list_frame::PopulateGameList()
{
	int selected_row = -1;

	const std::string selected_item = CurrentSelectionPath();

	// Release old data
	m_game_grid->clear_list();
	m_game_list->clear_list();

	m_game_list->setRowCount(m_game_data.size());

	// Default locale. Uses current Qt application language.
	const QLocale locale{};
	const Localized localized;

	const QString game_icon_path = m_play_hover_movies ? qstr(fs::get_config_dir() + "/Icons/game_icons/") : "";

	int row = 0;
	int index = -1;
	for (const auto& game : m_game_data)
	{
		index++;

		if (!IsEntryVisible(game))
		{
			game->item = nullptr;
			continue;
		}

		const QString serial = qstr(game->info.serial);
		const QString title = m_titles.value(serial, qstr(game->info.name));
		const QString notes = m_notes.value(serial);

		// Icon
		custom_table_widget_item* icon_item = new custom_table_widget_item;
		game->item = icon_item;

		icon_item->set_icon_func([this, icon_item, game](int)
		{
			ensure(icon_item && game);

			if (QMovie* movie = icon_item->movie(); movie && icon_item->get_active())
			{
				icon_item->setData(Qt::DecorationRole, movie->currentPixmap().scaled(m_icon_size, Qt::KeepAspectRatio));
			}
			else
			{
				icon_item->setData(Qt::DecorationRole, game->pxmap);

				if (!game->has_hover_gif)
				{
					game->pxmap = {};
				}

				if (movie)
				{
					movie->stop();
				}
			}
		});

		if (m_play_hover_movies && game->has_hover_gif)
		{
			icon_item->init_movie(game_icon_path % serial % "/hover.gif");
		}

		icon_item->setData(Qt::UserRole, index, true);
		icon_item->setData(gui::custom_roles::game_role, QVariant::fromValue(game));

		// Title
		custom_table_widget_item* title_item = new custom_table_widget_item(title);
		SetCustomConfigIcon(title_item, game);

		// Serial
		custom_table_widget_item* serial_item = new custom_table_widget_item(game->info.serial);

		if (!notes.isEmpty())
		{
			const QString tool_tip = tr("%0 [%1]\n\nNotes:\n%2").arg(title).arg(serial).arg(notes);
			title_item->setToolTip(tool_tip);
			serial_item->setToolTip(tool_tip);
		}

		// Move Support (http://www.psdevwiki.com/ps3/PARAM.SFO#ATTRIBUTE)
		const bool supports_move = game->info.attr & 0x800000;

		// Compatibility
		custom_table_widget_item* compat_item = new custom_table_widget_item;
		compat_item->setText(game->compat.text % (game->compat.date.isEmpty() ? QStringLiteral("") : " (" % game->compat.date % ")"));
		compat_item->setData(Qt::UserRole, game->compat.index, true);
		compat_item->setToolTip(game->compat.tooltip);
		if (!game->compat.color.isEmpty())
		{
			compat_item->setData(Qt::DecorationRole, compat_pixmap(game->compat.color, devicePixelRatioF() * 2));
		}

		// Version
		QString app_version = qstr(GetGameVersion(game));

		if (game->info.bootable && !game->compat.latest_version.isEmpty())
		{
			// If the app is bootable and the compat database contains info about the latest patch version:
			// add a hint for available software updates if the app version is unknown or lower than the latest version.
			if (app_version == localized.category.unknown || game->compat.latest_version.toDouble() > app_version.toDouble())
			{
				app_version = tr("%0 (Update available: %1)").arg(app_version, game->compat.latest_version);
			}
		}

		// Playtimes
		const quint64 elapsed_ms = m_persistent_settings->GetPlaytime(serial);

		// Last played (support outdated values)
		QDate last_played;
		const QString last_played_str = GetLastPlayedBySerial(serial);

		if (!last_played_str.isEmpty())
		{
			last_played = QDate::fromString(last_played_str, gui::persistent::last_played_date_format);

			if (!last_played.isValid())
			{
				last_played = QDate::fromString(last_played_str, gui::persistent::last_played_date_format_old);
			}
		}

		m_game_list->setItem(row, gui::column_icon,       icon_item);
		m_game_list->setItem(row, gui::column_name,       title_item);
		m_game_list->setItem(row, gui::column_serial,     serial_item);
		m_game_list->setItem(row, gui::column_firmware,   new custom_table_widget_item(game->info.fw));
		m_game_list->setItem(row, gui::column_version,    new custom_table_widget_item(app_version));
		m_game_list->setItem(row, gui::column_category,   new custom_table_widget_item(game->localized_category));
		m_game_list->setItem(row, gui::column_path,       new custom_table_widget_item(game->info.path));
		m_game_list->setItem(row, gui::column_move,       new custom_table_widget_item(sstr(supports_move ? tr("Supported") : tr("Not Supported")), Qt::UserRole, !supports_move));
		m_game_list->setItem(row, gui::column_resolution, new custom_table_widget_item(GetStringFromU32(game->info.resolution, localized.resolution.mode, true)));
		m_game_list->setItem(row, gui::column_sound,      new custom_table_widget_item(GetStringFromU32(game->info.sound_format, localized.sound.format, true)));
		m_game_list->setItem(row, gui::column_parental,   new custom_table_widget_item(GetStringFromU32(game->info.parental_lvl, localized.parental.level), Qt::UserRole, game->info.parental_lvl));
		m_game_list->setItem(row, gui::column_last_play,  new custom_table_widget_item(locale.toString(last_played, gui::persistent::last_played_date_format_new), Qt::UserRole, last_played));
		m_game_list->setItem(row, gui::column_playtime,   new custom_table_widget_item(elapsed_ms == 0 ? tr("Never played") : localized.GetVerboseTimeByMs(elapsed_ms), Qt::UserRole, elapsed_ms));
		m_game_list->setItem(row, gui::column_compat,     compat_item);

		if (selected_item == game->info.path + game->info.icon_path)
		{
			selected_row = row;
		}

		row++;
	}

	m_game_list->setRowCount(row);
	m_game_list->selectRow(selected_row);
}

void game_list_frame::PopulateGameGrid(int maxCols, const QSize& image_size, const QColor& image_color)
{
	int r = 0;
	int c = 0;

	const std::string selected_item = CurrentSelectionPath();

	// Release old data
	m_game_list->clear_list();
	m_game_grid->deleteLater();

	const bool show_text = m_icon_size_index > gui::gl_max_slider_pos * 2 / 5;

	if (m_icon_size_index < gui::gl_max_slider_pos * 2 / 3)
	{
		m_game_grid = new game_list_grid(image_size, image_color, m_margin_factor, m_text_factor * 2, show_text);
	}
	else
	{
		m_game_grid = new game_list_grid(image_size, image_color, m_margin_factor, m_text_factor, show_text);
	}

	// Get list of matching apps
	QList<game_info> matching_apps;

	for (const auto& app : m_game_data)
	{
		if (IsEntryVisible(app))
		{
			matching_apps.push_back(app);
		}
	}

	const int entries = matching_apps.count();

	// Edge cases!
	if (entries == 0)
	{ // For whatever reason, 0%x is division by zero. Absolute nonsense by definition of modulus. But, I'll acquiesce.
		return;
	}

	maxCols = std::clamp(maxCols, 1, entries);

	const int needs_extra_row = (entries % maxCols) != 0;
	const int max_rows = needs_extra_row + entries / maxCols;
	m_game_grid->setRowCount(max_rows);
	m_game_grid->setColumnCount(maxCols);

	const QString game_icon_path = m_play_hover_movies ? qstr(fs::get_config_dir() + "/Icons/game_icons/") : "";

	for (const auto& app : matching_apps)
	{
		const QString serial = qstr(app->info.serial);
		const QString title = m_titles.value(serial, qstr(app->info.name));
		const QString notes = m_notes.value(serial);

		movie_item* item = m_game_grid->addItem(app, title, (m_play_hover_movies && app->has_hover_gif) ? (game_icon_path % serial % "/hover.gif") : QStringLiteral(""), r, c);
		ensure(item);
		app->item = item;
		item->setData(gui::game_role, QVariant::fromValue(app));

		if (!notes.isEmpty())
		{
			item->setToolTip(tr("%0 [%1]\n\nNotes:\n%2").arg(title).arg(serial).arg(notes));
		}
		else
		{
			item->setToolTip(tr("%0 [%1]").arg(title).arg(serial));
		}

		if (selected_item == app->info.path + app->info.icon_path)
		{
			m_game_grid->setCurrentItem(item);
		}

		if (++c >= maxCols)
		{
			c = 0;
			r++;
		}
	}

	if (c != 0)
	{ // if left over games exist -- if empty entries exist
		for (int col = c; col < maxCols; ++col)
		{
			movie_item* empty_item = new movie_item();
			empty_item->setFlags(Qt::NoItemFlags);
			m_game_grid->setItem(r, col, empty_item);
		}
	}

	m_game_grid->resizeColumnsToContents();
	m_game_grid->resizeRowsToContents();
	m_game_grid->installEventFilter(this);
	m_game_grid->verticalScrollBar()->installEventFilter(this);
}

/**
* Returns false if the game should be hidden because it doesn't match search term in toolbar.
*/
bool game_list_frame::SearchMatchesApp(const QString& name, const QString& serial) const
{
	if (!m_search_text.isEmpty())
	{
		const QString search_text = m_search_text.toLower();
		return m_titles.value(serial, name).toLower().contains(search_text) || serial.toLower().contains(search_text);
	}
	return true;
}

std::string game_list_frame::CurrentSelectionPath()
{
	std::string selection;

	QTableWidgetItem* item = nullptr;

	if (m_old_layout_is_list)
	{
		if (!m_game_list->selectedItems().isEmpty())
		{
			item = m_game_list->item(m_game_list->currentRow(), 0);
		}
	}
	else if (m_game_grid)
	{
		if (!m_game_grid->selectedItems().isEmpty())
		{
			item = m_game_grid->currentItem();
		}
	}

	if (item)
	{
		if (const QVariant var = item->data(gui::game_role); var.canConvert<game_info>())
		{
			if (const game_info game = var.value<game_info>())
			{
				selection = game->info.path + game->info.icon_path;
			}
		}
	}

	m_old_layout_is_list = m_is_list_layout;

	return selection;
}

std::string game_list_frame::GetStringFromU32(const u32& key, const std::map<u32, QString>& map, bool combined)
{
	QStringList string;

	if (combined)
	{
		for (const auto& item : map)
		{
			if (key & item.first)
			{
				string << item.second;
			}
		}
	}
	else
	{
		if (map.find(key) != map.end())
		{
			string << map.at(key);
		}
	}

	if (string.isEmpty())
	{
		string << tr("Unknown");
	}

	return sstr(string.join(", "));
}

game_info game_list_frame::GetGameInfoByMode(const QTableWidgetItem* item) const
{
	if (!item)
	{
		return nullptr;
	}

	if (m_is_list_layout)
	{
		return GetGameInfoFromItem(m_game_list->item(item->row(), gui::column_icon));
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

QColor game_list_frame::getGridCompatibilityColor(const QString& string) const
{
	if (m_draw_compat_status_to_grid && !m_is_list_layout)
	{
		return QColor(string);
	}
	return QColor();
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

QList<game_info> game_list_frame::GetGameInfo() const
{
	return m_game_data;
}

std::string game_list_frame::GetGameVersion(const game_info& game)
{
	if (game->info.app_ver == sstr(Localized().category.unknown))
	{
		// Fall back to Disc/Pkg Revision
		return game->info.version;
	}

	return game->info.app_ver;
}
