#include "game_list_frame.h"
#include "qt_utils.h"
#include "settings_dialog.h"
#include "pad_settings_dialog.h"
#include "table_item_delegate.h"
#include "custom_table_widget_item.h"
#include "input_dialog.h"
#include "localized.h"

#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "Loader/PSF.h"
#include "Utilities/types.h"
#include "Utilities/lockless.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <set>
#include <regex>

#include <QtConcurrent>
#include <QDesktopServices>
#include <QHeaderView>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollBar>
#include <QInputDialog>
#include <QToolTip>
#include <QApplication>
#include <QClipboard>

LOG_CHANNEL(game_list_log, "GameList");
LOG_CHANNEL(sys_log, "SYS");

inline std::string sstr(const QString& _in) { return _in.toStdString(); }

game_list_frame::game_list_frame(std::shared_ptr<gui_settings> guiSettings, std::shared_ptr<emu_settings> emuSettings, std::shared_ptr<persistent_settings> persistent_settings, QWidget *parent)
	: custom_dock_widget(tr("Game List"), parent)
	, m_gui_settings(guiSettings)
	, m_emu_settings(emuSettings)
	, m_persistent_settings(persistent_settings)
{
	m_isListLayout    = m_gui_settings->GetValue(gui::gl_listMode).toBool();
	m_Margin_Factor   = m_gui_settings->GetValue(gui::gl_marginFactor).toReal();
	m_Text_Factor     = m_gui_settings->GetValue(gui::gl_textFactor).toReal();
	m_Icon_Color      = m_gui_settings->GetValue(gui::gl_iconColor).value<QColor>();
	m_colSortOrder    = m_gui_settings->GetValue(gui::gl_sortAsc).toBool() ? Qt::AscendingOrder : Qt::DescendingOrder;
	m_sortColumn      = m_gui_settings->GetValue(gui::gl_sortCol).toInt();
	m_hidden_list     = gui::utils::list_to_set(m_gui_settings->GetValue(gui::gl_hidden_list).toStringList());

	m_oldLayoutIsList = m_isListLayout;

	// Save factors for first setup
	m_gui_settings->SetValue(gui::gl_iconColor, m_Icon_Color);
	m_gui_settings->SetValue(gui::gl_marginFactor, m_Margin_Factor);
	m_gui_settings->SetValue(gui::gl_textFactor, m_Text_Factor);

	m_Game_Dock = new QMainWindow(this);
	m_Game_Dock->setWindowFlags(Qt::Widget);
	setWidget(m_Game_Dock);

	m_xgrid = new game_list_grid(QSize(), m_Icon_Color, m_Margin_Factor, m_Text_Factor, false);

	m_gameList = new game_list();
	m_gameList->setShowGrid(false);
	m_gameList->setItemDelegate(new table_item_delegate(this, true));
	m_gameList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_gameList->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_gameList->setSelectionMode(QAbstractItemView::SingleSelection);
	m_gameList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_gameList->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_gameList->verticalScrollBar()->installEventFilter(this);
	m_gameList->verticalScrollBar()->setSingleStep(20);
	m_gameList->horizontalScrollBar()->setSingleStep(20);
	m_gameList->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	m_gameList->verticalHeader()->setVisible(false);
	m_gameList->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	m_gameList->horizontalHeader()->setHighlightSections(false);
	m_gameList->horizontalHeader()->setSortIndicatorShown(true);
	m_gameList->horizontalHeader()->setStretchLastSection(true);
	m_gameList->horizontalHeader()->setDefaultSectionSize(150);
	m_gameList->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	m_gameList->setContextMenuPolicy(Qt::CustomContextMenu);
	m_gameList->setAlternatingRowColors(true);
	m_gameList->installEventFilter(this);
	m_gameList->setColumnCount(gui::column_count);

	m_game_compat = std::make_unique<game_compatibility>(m_gui_settings);

	m_Central_Widget = new QStackedWidget(this);
	m_Central_Widget->addWidget(m_gameList);
	m_Central_Widget->addWidget(m_xgrid);
	m_Central_Widget->setCurrentWidget(m_isListLayout ? m_gameList : m_xgrid);

	m_Game_Dock->setCentralWidget(m_Central_Widget);

	// Actions regarding showing/hiding columns
	auto AddColumn = [this](gui::game_list_columns col, const QString& header_text, const QString& action_text)
	{
		m_gameList->setHorizontalHeaderItem(col, new QTableWidgetItem(header_text));
		m_columnActs.append(new QAction(action_text, this));
	};

	AddColumn(gui::column_icon,       tr("Icon"),                  tr("Show Icons"));
	AddColumn(gui::column_name,       tr("Name"),                  tr("Show Names"));
	AddColumn(gui::column_serial,     tr("Serial"),                tr("Show Serials"));
	AddColumn(gui::column_firmware,   tr("Firmware"),              tr("Show Firmwares"));
	AddColumn(gui::column_version,    tr("Version"),               tr("Show Versions"));
	AddColumn(gui::column_category,   tr("Category"),              tr("Show Categories"));
	AddColumn(gui::column_path,       tr("Path"),                  tr("Show Paths"));
	AddColumn(gui::column_move,       tr("PlayStation Move"),      tr("Show PlayStation Move"));
	AddColumn(gui::column_resolution, tr("Supported Resolutions"), tr("Show Supported Resolutions"));
	AddColumn(gui::column_sound,      tr("Sound Formats"),         tr("Show Sound Formats"));
	AddColumn(gui::column_parental,   tr("Parental Level"),        tr("Show Parental Levels"));
	AddColumn(gui::column_last_play,  tr("Last Played"),           tr("Show Last Played"));
	AddColumn(gui::column_playtime,   tr("Time Played"),           tr("Show Time Played"));
	AddColumn(gui::column_compat,     tr("Compatibility"),         tr("Show Compatibility"));

	// Events
	connect(m_gameList, &QTableWidget::customContextMenuRequested, this, &game_list_frame::ShowContextMenu);
	connect(m_gameList, &QTableWidget::itemDoubleClicked, this, &game_list_frame::doubleClickedSlot);

	connect(m_gameList->horizontalHeader(), &QHeaderView::sectionClicked, this, &game_list_frame::OnColClicked);
	connect(m_gameList->horizontalHeader(), &QHeaderView::customContextMenuRequested, [=](const QPoint& pos)
	{
		QMenu* configure = new QMenu(this);
		configure->addActions(m_columnActs);
		configure->exec(m_gameList->horizontalHeader()->viewport()->mapToGlobal(pos));
	});

	connect(m_xgrid, &QTableWidget::itemDoubleClicked, this, &game_list_frame::doubleClickedSlot);
	connect(m_xgrid, &QTableWidget::customContextMenuRequested, this, &game_list_frame::ShowContextMenu);

	connect(m_game_compat.get(), &game_compatibility::DownloadStarted, [=]()
	{
		for (const auto& game : m_game_data)
		{
			game->compat = m_game_compat->GetStatusData("Download");
		}
		Refresh();
	});
	connect(m_game_compat.get(), &game_compatibility::DownloadFinished, [=]()
	{
		for (const auto& game : m_game_data)
		{
			game->compat = m_game_compat->GetCompatibility(game->info.serial);
		}
		Refresh();
	});
	connect(m_game_compat.get(), &game_compatibility::DownloadError, [=](const QString& error)
	{
		for (const auto& game : m_game_data)
		{
			game->compat = m_game_compat->GetCompatibility(game->info.serial);
		}
		Refresh();
		QMessageBox::warning(this, tr("Warning!"), tr("Failed to retrieve the online compatibility database!\nFalling back to local database.\n\n") + tr(qPrintable(error)));
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
			m_gameList->setColumnHidden(col, !checked); // Negate because it's a set col hidden and we have menu say show.
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
	m_colSortOrder = m_gui_settings->GetValue(gui::gl_sortAsc).toBool() ? Qt::AscendingOrder : Qt::DescendingOrder;
	m_sortColumn = m_gui_settings->GetValue(gui::gl_sortCol).toInt();
	m_categoryFilters = m_gui_settings->GetGameListCategoryFilters();
	m_drawCompatStatusToGrid = m_gui_settings->GetValue(gui::gl_draw_compat).toBool();

	Refresh(true);

	QByteArray state = m_gui_settings->GetValue(gui::gl_state).toByteArray();
	if (!m_gameList->horizontalHeader()->restoreState(state) && m_gameList->rowCount())
	{
		// If no settings exist, resize to contents.
		ResizeColumnsToContents();
	}

	for (int col = 0; col < m_columnActs.count(); ++col)
	{
		bool vis = m_gui_settings->GetGamelistColVisibility(col);
		m_columnActs[col]->setChecked(vis);
		m_gameList->setColumnHidden(col, !vis);
	}

	SortGameList();
	FixNarrowColumns();

	m_gameList->horizontalHeader()->restoreState(m_gameList->horizontalHeader()->saveState());
}

game_list_frame::~game_list_frame()
{
	SaveSettings();
}

void game_list_frame::FixNarrowColumns()
{
	qApp->processEvents();

	// handle columns (other than the icon column) that have zero width after showing them (stuck between others)
	for (int col = 1; col < m_columnActs.count(); ++col)
	{
		if (m_gameList->isColumnHidden(col))
		{
			continue;
		}

		if (m_gameList->columnWidth(col) <= m_gameList->horizontalHeader()->minimumSectionSize())
		{
			m_gameList->setColumnWidth(col, m_gameList->horizontalHeader()->minimumSectionSize());
		}
	}
}

void game_list_frame::ResizeColumnsToContents(int spacing)
{
	if (!m_gameList)
	{
		return;
	}

	m_gameList->verticalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
	m_gameList->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);

	// Make non-icon columns slighty bigger for better visuals
	for (int i = 1; i < m_gameList->columnCount(); i++)
	{
		if (m_gameList->isColumnHidden(i))
		{
			continue;
		}

		int size = m_gameList->horizontalHeader()->sectionSize(i) + spacing;
		m_gameList->horizontalHeader()->resizeSection(i, size);
	}
}

void game_list_frame::OnColClicked(int col)
{
	if (col == 0) return; // Don't "sort" icons.

	if (col == m_sortColumn)
	{
		m_colSortOrder = (m_colSortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
	}
	else
	{
		m_colSortOrder = Qt::AscendingOrder;
	}
	m_sortColumn = col;

	m_gui_settings->SetValue(gui::gl_sortAsc, m_colSortOrder == Qt::AscendingOrder);
	m_gui_settings->SetValue(gui::gl_sortCol, col);

	SortGameList();
}

// Get visibility of entries
bool game_list_frame::IsEntryVisible(const game_info& game)
{
	auto matches_category = [&]()
	{
		if (m_isListLayout)
		{
			return m_categoryFilters.contains(qstr(game->info.category));
		}

		const auto cat_boot = Localized().category.cat_boot;
		return cat_boot.find(qstr(game->info.category)) != cat_boot.end();
	};

	const QString serial = qstr(game->info.serial);
	bool is_visible = m_show_hidden || !m_hidden_list.contains(serial);
	return is_visible && matches_category() && SearchMatchesApp(qstr(game->info.name), serial);
}

void game_list_frame::SortGameList()
{
	// Back-up old header sizes to handle unwanted column resize in case of zero search results
	QList<int> column_widths;
	int old_row_count = m_gameList->rowCount();
	int old_game_count = m_game_data.count();

	for (int i = 0; i < m_gameList->columnCount(); i++)
	{
		column_widths.append(m_gameList->columnWidth(i));
	}

	// Sorting resizes hidden columns, so unhide them as a workaround
	QList<int> columns_to_hide;

	for (int i = 0; i < m_gameList->columnCount(); i++)
	{
		if (m_gameList->isColumnHidden(i))
		{
			m_gameList->setColumnHidden(i, false);
			columns_to_hide << i;
		}
	}

	// Sort the list by column and sort order
	m_gameList->sortByColumn(m_sortColumn, m_colSortOrder);

	// Hide columns again
	for (auto i : columns_to_hide)
	{
		m_gameList->setColumnHidden(i, true);
	}

	// Don't resize the columns if no game is shown to preserve the header settings
	if (!m_gameList->rowCount())
	{
		for (int i = 0; i < m_gameList->columnCount(); i++)
		{
			m_gameList->setColumnWidth(i, column_widths[i]);
		}

		m_gameList->horizontalHeader()->setSectionResizeMode(gui::column_icon, QHeaderView::Fixed);
		return;
	}

	// Fixate vertical header and row height
	m_gameList->verticalHeader()->setMinimumSectionSize(m_Icon_Size.height());
	m_gameList->verticalHeader()->setMaximumSectionSize(m_Icon_Size.height());
	m_gameList->resizeRowsToContents();

	// Resize columns if the game list was empty before
	if (!old_row_count && !old_game_count)
	{
		ResizeColumnsToContents();
	}
	else
	{
		m_gameList->resizeColumnToContents(gui::column_icon);
	}

	// Fixate icon column
	m_gameList->horizontalHeader()->setSectionResizeMode(gui::column_icon, QHeaderView::Fixed);

	// Shorten the last section to remove horizontal scrollbar if possible
	m_gameList->resizeColumnToContents(gui::column_count - 1);
}

QString game_list_frame::GetLastPlayedBySerial(const QString& serial)
{
	return m_persistent_settings->GetLastPlayed(serial);
}

QString game_list_frame::GetPlayTimeByMs(int elapsed_ms)
{
	if (elapsed_ms <= 0)
	{
		return "";
	}

	const qint64 elapsed_seconds = (elapsed_ms / 1000) + ((elapsed_ms % 1000) > 0 ? 1 : 0);
	const qint64 hours_played    = elapsed_seconds / 3600;
	const qint64 minutes_played  = (elapsed_seconds % 3600) / 60;
	const qint64 seconds_played  = (elapsed_seconds % 3600) % 60;

	// For anyone who was wondering why there need to be so many cases:
	// 1. Using variables won't work for future localization due to varying sentence structure in different languages.
	// 2. The provided Qt functionality only works if localization is already enabled
	// 3. The provided Qt functionality only works for single variables

	if (hours_played <= 0)
	{
		if (minutes_played <= 0)
		{
			if (seconds_played == 1)
			{
				return tr("%0 second").arg(seconds_played);
			}
			return tr("%0 seconds").arg(seconds_played);
		}

		if (seconds_played <= 0)
		{
			if (minutes_played == 1)
			{
				return tr("%0 minute").arg(minutes_played);
			}
			return tr("%0 minutes").arg(minutes_played);
		}
		if (minutes_played == 1 && seconds_played == 1)
		{
			return tr("%0 minute and %1 second").arg(minutes_played).arg(seconds_played);
		}
		if (minutes_played == 1)
		{
			return tr("%0 minute and %1 seconds").arg(minutes_played).arg(seconds_played);
		}
		if (seconds_played == 1)
		{
			return tr("%0 minutes and %1 second").arg(minutes_played).arg(seconds_played);
		}
		return tr("%0 minutes and %1 seconds").arg(minutes_played).arg(seconds_played);
	}

	if (minutes_played <= 0)
	{
		if (hours_played == 1)
		{
			return tr("%0 hour").arg(hours_played);
		}
		return tr("%0 hours").arg(hours_played);
	}
	if (hours_played == 1 && minutes_played == 1)
	{
		return tr("%0 hour and %1 minute").arg(hours_played).arg(minutes_played);
	}
	if (hours_played == 1)
	{
		return tr("%0 hour and %1 minutes").arg(hours_played).arg(minutes_played);
	}
	if (minutes_played == 1)
	{
		return tr("%0 hours and %1 minute").arg(hours_played).arg(minutes_played);
	}
	return tr("%0 hours and %1 minutes").arg(hours_played).arg(minutes_played);
}

std::string game_list_frame::GetCacheDirBySerial(const std::string& serial)
{
	return fs::get_cache_dir() + "cache/" + serial;
}

std::string game_list_frame::GetDataDirBySerial(const std::string& serial)
{
	return fs::get_config_dir() + "data/" + serial;
}

void game_list_frame::Refresh(const bool fromDrive, const bool scrollAfter)
{
	if (fromDrive)
	{
		const Localized localized;

		// Load PSF

		m_game_data.clear();
		m_notes.clear();

		const std::string _hdd = Emulator::GetHddDir();
		const std::string cat_unknown = sstr(category::cat_unknown);
		const std::string cat_unknown_localized = sstr(localized.category.unknown);

		std::vector<std::string> path_list;

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
					path_list.emplace_back(path + "/" + entry.name);
				}
			}
		};

		const auto add_dir = [&](const std::string& path)
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
					add_disc_dir(entry_path);
				}
				else
				{
					path_list.emplace_back(entry_path);
				}
			}
		};

		add_dir(_hdd + "game/");
		add_dir(_hdd + "disc/");

		for (auto pair : YAML::Load(fs::file{fs::get_config_dir() + "/games.yml", fs::read + fs::create}.to_string()))
		{
			std::string game_dir = pair.second.Scalar();
			game_dir.resize(game_dir.find_last_not_of('/') + 1);

			if (fs::is_file(game_dir + "/PS3_DISC.SFB"))
			{
				add_disc_dir(game_dir);
			}
			else
			{
				path_list.push_back(game_dir);
			}
		}

		// Used to remove duplications from the list (serial -> set of cat names)
		std::map<std::string, std::set<std::string>> serial_cat_name;

		QSet<QString> serials;

		QMutex mutex_cat;

		QList<size_t> indices;
		for (size_t i = 0; i < path_list.size(); ++i)
			indices.append(i);

		lf_queue<game_info> games;

		QtConcurrent::blockingMap(indices, [&](size_t& i)
		{
			const std::string dir = path_list[i];

			const Localized thread_localized;

			try
			{
				const std::string sfo_dir = Emulator::GetSfoDirFromGamePath(dir, Emu.GetUsr());
				const fs::file sfo_file(sfo_dir + "/PARAM.SFO");
				if (!sfo_file)
				{
					return;
				}

				const auto psf = psf::load_object(sfo_file);

				GameInfo game;
				game.path         = dir;
				game.serial       = psf::get_string(psf, "TITLE_ID", "");
				game.name         = psf::get_string(psf, "TITLE", cat_unknown_localized);
				game.app_ver      = psf::get_string(psf, "APP_VER", cat_unknown_localized);
				game.version      = psf::get_string(psf, "VERSION", cat_unknown_localized);
				game.category     = psf::get_string(psf, "CATEGORY", cat_unknown);
				game.fw           = psf::get_string(psf, "PS3_SYSTEM_VER", cat_unknown_localized);
				game.parental_lvl = psf::get_integer(psf, "PARENTAL_LEVEL", 0);
				game.resolution   = psf::get_integer(psf, "RESOLUTION", 0);
				game.sound_format = psf::get_integer(psf, "SOUND_FORMAT", 0);
				game.bootable     = psf::get_integer(psf, "BOOTABLE", 0);
				game.attr         = psf::get_integer(psf, "ATTRIBUTE", 0);

				mutex_cat.lock();

				// Detect duplication
				if (!serial_cat_name[game.serial].emplace(game.category + game.name).second)
				{
					mutex_cat.unlock();
					return;
				}

				const QString serial = qstr(game.serial);
				const QString note = m_gui_settings->GetValue(gui::notes, serial, "").toString();
				const QString title = m_gui_settings->GetValue(gui::titles, serial, "").toString().simplified();

				// Read persistent_settings values
				QString last_played = m_persistent_settings->GetValue(gui::persistent::last_played, serial, "").toString();
				int playtime        = m_persistent_settings->GetValue(gui::persistent::playtime, serial, 0).toInt();

				// Read deprecated gui_setting values first for backwards compatibility (older than January 12th 2020).
				// Restrict this to empty persistent settings to keep continuity.
				if (last_played.isEmpty())
				{
					last_played = m_gui_settings->GetValue(gui::persistent::last_played, serial, "").toString();
				}
				if (playtime <= 0)
				{
					playtime = m_gui_settings->GetValue(gui::persistent::playtime, serial, 0).toInt();
				}

				// Set persistent_settings values if values exist
				if (!last_played.isEmpty())
				{
					m_persistent_settings->SetLastPlayed(serial, last_played);
				}
				if (playtime > 0)
				{
					m_persistent_settings->SetPlaytime(serial, playtime);
				}

				serials.insert(serial);

				if (!note.isEmpty())
				{
					m_notes.insert(serial, note);
				}

				if (!title.isEmpty())
				{
					m_titles.insert(serial, title);
				}

				auto qt_cat = qstr(game.category);
				auto cat = thread_localized.category.cat_boot.find(qt_cat);
				if (cat != thread_localized.category.cat_boot.end())
				{
					game.icon_path = sfo_dir + "/ICON0.PNG";
					qt_cat = cat->second;
				}
				else if ((cat = thread_localized.category.cat_data.find(qt_cat)) != thread_localized.category.cat_data.end())
				{
					game.icon_path = sfo_dir + "/ICON0.PNG";
					qt_cat = cat->second;
				}
				else if (game.category == cat_unknown)
				{
					game.icon_path = sfo_dir + "/ICON0.PNG";
					qt_cat = localized.category.unknown;
				}
				else
				{
					game.icon_path = sfo_dir + "/ICON0.PNG";
					qt_cat = localized.category.other;
				}

				mutex_cat.unlock();

				// Load ICON0.PNG
				QPixmap icon;

				if (game.icon_path.empty() || !icon.load(qstr(game.icon_path)))
				{
					game_list_log.warning("Could not load image from path %s", sstr(QDir(qstr(game.icon_path)).absolutePath()));
				}

				const auto compat = m_game_compat->GetCompatibility(game.serial);

				const bool hasCustomConfig = fs::is_file(Emulator::GetCustomConfigPath(game.serial)) || fs::is_file(Emulator::GetCustomConfigPath(game.serial, true));
				const bool hasCustomPadConfig = fs::is_file(Emulator::GetCustomInputConfigPath(game.serial));

				const QColor color = getGridCompatibilityColor(compat.color);
				const QPixmap pxmap = PaintedPixmap(icon, hasCustomConfig, hasCustomPadConfig, color);

				games.push(std::make_shared<gui_game_info>(gui_game_info{game, qt_cat, compat, icon, pxmap, hasCustomConfig, hasCustomPadConfig}));
			}
			catch (const std::exception& e)
			{
				game_list_log.fatal("Failed to update game list at %s\n%s thrown: %s", dir, typeid(e).name(), e.what());
				return;
			}
		});

		for (auto&& g : games.pop_all())
		{
			m_game_data.push_back(std::move(g));
		}

		// Try to update the app version for disc games if there is a patch
		for (const auto& entry : m_game_data)
		{
			if (entry->info.category == "DG")
			{
				for (const auto& other : m_game_data)
				{
					// The patch is game data and must have the same serial and an app version
					if (entry->info.serial == other->info.serial && other->info.category == "GD" && other->info.app_ver != cat_unknown)
					{
						try
						{
							// Update the app version if it's higher than the disc's version (old games may not have an app version)
							if (entry->info.app_ver == cat_unknown || std::stod(other->info.app_ver) > std::stod(entry->info.app_ver))
							{
								entry->info.app_ver = other->info.app_ver;
							}
							// Update the firmware version if possible and if it's higher than the disc's version
							if (other->info.fw != cat_unknown && std::stod(other->info.fw) > std::stod(entry->info.fw))
							{
								entry->info.fw = other->info.fw;
							}
							// Update the parental level if possible and if it's higher than the disc's level
							if (other->info.parental_lvl != 0 && other->info.parental_lvl > entry->info.parental_lvl)
							{
								entry->info.parental_lvl = other->info.parental_lvl;
							}
						}
						catch (const std::exception& e)
						{
							game_list_log.error("Failed to update the displayed version numbers for title ID %s\n%s thrown: %s", entry->info.serial, typeid(e).name(), e.what());
						}

						const std::string key = "GD" + other->info.name;
						serial_cat_name[other->info.serial].erase(key);
						if (!serial_cat_name[other->info.serial].count(key))
						{
							break;
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
		m_hidden_list.intersect(serials);
		m_gui_settings->SetValue(gui::gl_hidden_list, QStringList(m_hidden_list.values()));
	}

	// Fill Game List / Game Grid

	if (m_isListLayout)
	{
		int scroll_position = m_gameList->verticalScrollBar()->value();
		int row = PopulateGameList();
		m_gameList->selectRow(row);
		SortGameList();

		if (scrollAfter)
		{
			m_gameList->scrollTo(m_gameList->currentIndex(), QAbstractItemView::PositionAtCenter);
		}
		else
		{
			m_gameList->verticalScrollBar()->setValue(scroll_position);
		}
	}
	else
	{
		int games_per_row = 0;

		if (m_Icon_Size.width() > 0 && m_Icon_Size.height() > 0)
		{
			games_per_row = width() / (m_Icon_Size.width() + m_Icon_Size.width() * m_xgrid->getMarginFactor() * 2);
		}

		const int scroll_position = m_xgrid->verticalScrollBar()->value();
		PopulateGameGrid(games_per_row, m_Icon_Size, m_Icon_Color);
		connect(m_xgrid, &QTableWidget::itemDoubleClicked, this, &game_list_frame::doubleClickedSlot);
		connect(m_xgrid, &QTableWidget::customContextMenuRequested, this, &game_list_frame::ShowContextMenu);
		m_Central_Widget->addWidget(m_xgrid);
		m_Central_Widget->setCurrentWidget(m_xgrid);
		m_xgrid->verticalScrollBar()->setValue(scroll_position);
	}
}

void game_list_frame::ToggleCategoryFilter(const QStringList& categories, bool show)
{
	if (show)
	{
		m_categoryFilters.append(categories);
	}
	else
	{
		for (const auto& cat : categories)
		{
			m_categoryFilters.removeAll(cat);
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
	m_gui_settings->SetValue(gui::gl_sortCol, m_sortColumn);
	m_gui_settings->SetValue(gui::gl_sortAsc, m_colSortOrder == Qt::AscendingOrder);

	m_gui_settings->SetValue(gui::gl_state, m_gameList->horizontalHeader()->saveState());
}

static void open_dir(const std::string& spath)
{
	fs::create_dir(spath);
	const QString path = qstr(spath);

	if (fs::is_file(spath))
	{
		// open directory and select file
		// https://stackoverflow.com/questions/3490336/how-to-reveal-in-finder-or-show-in-explorer-with-qt
#ifdef _WIN32
		QProcess::startDetached("explorer.exe", { "/select,", QDir::toNativeSeparators(path) });
#elif defined(__APPLE__)
		QProcess::execute("/usr/bin/osascript", { "-e", "tell application \"Finder\" to reveal POSIX file \"" + path + "\"" });
		QProcess::execute("/usr/bin/osascript", { "-e", "tell application \"Finder\" to activate" });
#else
		// open parent directory
		QDesktopServices::openUrl(QUrl("file:///" + qstr(fs::get_parent_dir(spath))));
#endif
		return;
	}

	QDesktopServices::openUrl(QUrl("file:///" + path));
}

void game_list_frame::doubleClickedSlot(QTableWidgetItem *item)
{
	if (item == nullptr)
	{
		return;
	}

	game_info game;

	if (m_isListLayout)
	{
		game = GetGameInfoFromItem(m_gameList->item(item->row(), gui::column_icon));
	}
	else
	{
		game = GetGameInfoFromItem(item);
	}

	if (!game)
	{
		return;
	}

	sys_log.notice("Booting from gamelist per doubleclick...");
	Q_EMIT RequestBoot(game);
}

void game_list_frame::ShowContextMenu(const QPoint &pos)
{
	QPoint globalPos;
	QTableWidgetItem* item;

	if (m_isListLayout)
	{
		item = m_gameList->item(m_gameList->indexAt(pos).row(), gui::column_icon);
		globalPos = m_gameList->viewport()->mapToGlobal(pos);
	}
	else
	{
		QModelIndex mi = m_xgrid->indexAt(pos);
		item = m_xgrid->item(mi.row(), mi.column());
		globalPos = m_xgrid->viewport()->mapToGlobal(pos);
	}

	game_info gameinfo = GetGameInfoFromItem(item);
	if (!gameinfo)
	{
		return;
	}

	GameInfo currGame = gameinfo->info;
	const QString serial = qstr(currGame.serial);
	const QString name = qstr(currGame.name).simplified();

	const std::string cache_base_dir = GetCacheDirBySerial(currGame.serial);
	const std::string data_base_dir  = GetDataDirBySerial(currGame.serial);

	// Make Actions
	QMenu myMenu;

	const bool is_current_running_game = (Emu.IsRunning() || Emu.IsPaused()) && currGame.serial == Emu.GetTitleID();

	QAction* boot = new QAction(gameinfo->hasCustomConfig ? tr(is_current_running_game ? "&Reboot with global configuration" : "&Boot with global configuration") : tr("&Boot"));
	QFont f = boot->font();
	f.setBold(true);

	if (gameinfo->hasCustomConfig)
	{
		QAction* boot_custom = myMenu.addAction(tr(is_current_running_game ? "&Reboot with custom configuration" : "&Boot with custom configuration"));
		boot_custom->setFont(f);
		connect(boot_custom, &QAction::triggered, [=]
		{
			sys_log.notice("Booting from gamelist per context menu...");
			Q_EMIT RequestBoot(gameinfo);
		});
	}
	else
	{
		boot->setFont(f);
	}

	myMenu.addAction(boot);
	myMenu.addSeparator();

	QAction* configure = myMenu.addAction(gameinfo->hasCustomConfig ? tr("&Change Custom Configuration") : tr("&Create Custom Configuration"));
	QAction* pad_configure = myMenu.addAction(gameinfo->hasCustomPadConfig ? tr("&Change Custom Gamepad Configuration") : tr("&Create Custom Gamepad Configuration"));
	QAction* createPPUCache = myMenu.addAction(tr("&Create PPU Cache"));
	myMenu.addSeparator();
	QAction* renameTitle = myMenu.addAction(tr("&Rename In Game List"));
	QAction* hide_serial = myMenu.addAction(tr("&Hide From Game List"));
	hide_serial->setCheckable(true);
	hide_serial->setChecked(m_hidden_list.contains(serial));
	myMenu.addSeparator();
	QMenu* remove_menu = myMenu.addMenu(tr("&Remove"));
	QAction* removeGame = remove_menu->addAction(tr("&Remove %1").arg(gameinfo->localized_category));
	if (gameinfo->hasCustomConfig)
	{
		QAction* remove_custom_config = remove_menu->addAction(tr("&Remove Custom Configuration"));
		connect(remove_custom_config, &QAction::triggered, [=]()
		{
			if (RemoveCustomConfiguration(currGame.serial, gameinfo, true))
			{
				ShowCustomConfigIcon(gameinfo);
			}
		});
	}
	if (gameinfo->hasCustomPadConfig)
	{
		QAction* remove_custom_pad_config = remove_menu->addAction(tr("&Remove Custom Gamepad Configuration"));
		connect(remove_custom_pad_config, &QAction::triggered, [=]()
		{
			if (RemoveCustomPadConfiguration(currGame.serial, gameinfo, true))
			{
				ShowCustomConfigIcon(gameinfo);
			}
		});
	}
	if (fs::is_dir(cache_base_dir))
	{
		remove_menu->addSeparator();
		QAction* removeShadersCache = remove_menu->addAction(tr("&Remove Shaders Cache"));
		connect(removeShadersCache, &QAction::triggered, [=]()
		{
			RemoveShadersCache(cache_base_dir, true);
		});
		QAction* removePPUCache = remove_menu->addAction(tr("&Remove PPU Cache"));
		connect(removePPUCache, &QAction::triggered, [=]()
		{
			RemovePPUCache(cache_base_dir, true);
		});
		QAction* removeSPUCache = remove_menu->addAction(tr("&Remove SPU Cache"));
		connect(removeSPUCache, &QAction::triggered, [=]()
		{
			RemoveSPUCache(cache_base_dir, true);
		});
		QAction* removeAllCaches = remove_menu->addAction(tr("&Remove All Caches"));
		connect(removeAllCaches, &QAction::triggered, [=]()
		{
			if (QMessageBox::question(this, tr("Confirm Removal"), tr("Remove all caches?")) != QMessageBox::Yes)
				return;

			RemoveShadersCache(cache_base_dir);
			RemovePPUCache(cache_base_dir);
			RemoveSPUCache(cache_base_dir);
		});
	}
	myMenu.addSeparator();
	QAction* openGameFolder = myMenu.addAction(tr("&Open Install Folder"));
	if (gameinfo->hasCustomConfig)
	{
		QAction* open_config_dir = myMenu.addAction(tr("&Open Custom Config Folder"));
		connect(open_config_dir, &QAction::triggered, [=]()
		{
			const std::string new_config_path = Emulator::GetCustomConfigPath(currGame.serial);

			if (fs::is_file(new_config_path))
				open_dir(new_config_path);

			const std::string old_config_path = Emulator::GetCustomConfigPath(currGame.serial, true);

			if (fs::is_file(old_config_path))
				open_dir(old_config_path);
		});
	}
	if (fs::is_dir(data_base_dir))
	{
		QAction* open_data_dir = myMenu.addAction(tr("&Open Data Folder"));
		connect(open_data_dir, &QAction::triggered, [=]()
		{
			open_dir(data_base_dir);
		});
	}
	myMenu.addSeparator();
	QAction* checkCompat = myMenu.addAction(tr("&Check Game Compatibility"));
	QAction* downloadCompat = myMenu.addAction(tr("&Download Compatibility Database"));
	myMenu.addSeparator();
	QAction* editNotes = myMenu.addAction(tr("&Edit Tooltip Notes"));
	QMenu* info_menu = myMenu.addMenu(tr("&Copy Info"));
	QAction* copy_info = info_menu->addAction(tr("&Copy Name + Serial"));
	QAction* copy_name = info_menu->addAction(tr("&Copy Name"));
	QAction* copy_serial = info_menu->addAction(tr("&Copy Serial"));

	connect(boot, &QAction::triggered, [=]
	{
		sys_log.notice("Booting from gamelist per context menu...");
		Q_EMIT RequestBoot(gameinfo, gameinfo->hasCustomConfig);
	});
	connect(configure, &QAction::triggered, [=]
	{
		settings_dialog dlg(m_gui_settings, m_emu_settings, 0, this, &currGame);
		if (dlg.exec() == QDialog::Accepted)
		{
			if (!gameinfo->hasCustomConfig)
			{
				gameinfo->hasCustomConfig = true;
				ShowCustomConfigIcon(gameinfo);
			}
			Q_EMIT NotifyEmuSettingsChange();
		}
	});
	connect(pad_configure, &QAction::triggered, [=]
	{
		if (!Emu.IsStopped())
		{
			Emu.GetCallbacks().enable_pads(false);
		}
		pad_settings_dialog dlg(this, &currGame);
		connect(&dlg, &QDialog::finished, [this](int/* result*/)
		{
			if (Emu.IsStopped())
			{
				return;
			}
			Emu.GetCallbacks().reset_pads(Emu.GetTitleID());
		});
		if (dlg.exec() == QDialog::Accepted && !gameinfo->hasCustomPadConfig)
		{
			gameinfo->hasCustomPadConfig = true;
			ShowCustomConfigIcon(gameinfo);
		}
		if (!Emu.IsStopped())
		{
			Emu.GetCallbacks().enable_pads(true);
		}
	});
	connect(hide_serial, &QAction::triggered, [=](bool checked)
	{
		if (checked)
			m_hidden_list.insert(serial);
		else
			m_hidden_list.remove(serial);

		m_gui_settings->SetValue(gui::gl_hidden_list, QStringList(m_hidden_list.values()));
		Refresh();
	});
	connect(createPPUCache, &QAction::triggered, [=]
	{
		CreatePPUCache(gameinfo);
	});
	connect(removeGame, &QAction::triggered, [=]
	{
		if (currGame.path.empty())
		{
			game_list_log.fatal("Cannot remove game. Path is empty");
			return;
		}

		QMessageBox* mb = new QMessageBox(QMessageBox::Question, tr("Confirm %1 Removal").arg(gameinfo->localized_category), tr("Permanently remove %0 from drive?\nPath: %1").arg(name).arg(qstr(currGame.path)), QMessageBox::Yes | QMessageBox::No, this);
		mb->setCheckBox(new QCheckBox(tr("Remove caches and custom configs")));
		mb->deleteLater();
		if (mb->exec() == QMessageBox::Yes)
		{
			const bool remove_caches = mb->checkBox()->isChecked();
			if (fs::remove_all(currGame.path))
			{
				if (remove_caches)
				{
					RemoveShadersCache(cache_base_dir);
					RemovePPUCache(cache_base_dir);
					RemoveSPUCache(cache_base_dir);
					RemoveCustomConfiguration(currGame.serial);
					RemoveCustomPadConfiguration(currGame.serial);
				}
				m_game_data.erase(std::remove(m_game_data.begin(), m_game_data.end(), gameinfo), m_game_data.end());
				game_list_log.success("Removed %s %s in %s", sstr(gameinfo->localized_category), currGame.name, currGame.path);
				Refresh(true);
			}
			else
			{
				game_list_log.error("Failed to remove %s %s in %s (%s)", sstr(gameinfo->localized_category), currGame.name, currGame.path, fs::g_tls_error);
				QMessageBox::critical(this, tr("Failure!"), tr(remove_caches ? "Failed to remove %0 from drive!\nPath: %1\nCaches and custom configs have been left intact." : "Failed to remove %0 from drive!\nPath: %1").arg(name).arg(qstr(currGame.path)));
			}
		}
	});
	connect(openGameFolder, &QAction::triggered, [=]()
	{
		open_dir(currGame.path);
	});
	connect(checkCompat, &QAction::triggered, [=]
	{
		QString link = "https://rpcs3.net/compatibility?g=" + serial;
		QDesktopServices::openUrl(QUrl(link));
	});
	connect(downloadCompat, &QAction::triggered, [=]
	{
		m_game_compat->RequestCompatibility(true);
	});
	connect(renameTitle, &QAction::triggered, [=]
	{
		const QString custom_title = m_gui_settings->GetValue(gui::titles, serial, "").toString();
		const QString old_title = custom_title.isEmpty() ? name : custom_title;
		QString new_title;

		input_dialog dlg(128, old_title, tr("Rename Title"), tr("%0\n%1\n\nYou can clear the line in order to use the original title.").arg(name).arg(serial), name, this);
		dlg.move(globalPos);
		connect(&dlg, &input_dialog::text_changed, this, [&new_title](const QString& text)
		{
			new_title = text.simplified();
		});

		if (dlg.exec() == QDialog::Accepted)
		{
			if (new_title.isEmpty() || new_title == name)
			{
				m_titles.remove(serial);
				m_gui_settings->RemoveValue(gui::titles, serial);
			}
			else
			{
				m_titles.insert(serial, new_title);
				m_gui_settings->SetValue(gui::titles, serial, new_title);
			}
			Refresh(true); // full refresh in order to reliably sort the list
		}
	});
	connect(editNotes, &QAction::triggered, [=]
	{
		bool accepted;
		const QString old_notes = m_gui_settings->GetValue(gui::notes, serial, "").toString();
		const QString new_notes = QInputDialog::getMultiLineText(this, tr("Edit Tooltip Notes"), tr("%0\n%1").arg(name).arg(serial), old_notes, &accepted);

		if (accepted)
		{
			if (new_notes.simplified().isEmpty())
			{
				m_notes.remove(serial);
				m_gui_settings->RemoveValue(gui::notes, serial);
			}
			else
			{
				m_notes.insert(serial, new_notes);
				m_gui_settings->SetValue(gui::notes, serial, new_notes);
			}
			Refresh();
		}
	});
	connect(copy_info, &QAction::triggered, [=]
	{
		QApplication::clipboard()->setText(name + " [" + serial + "]");
	});
	connect(copy_name, &QAction::triggered, [=]
	{
		QApplication::clipboard()->setText(name);
	});
	connect(copy_serial, &QAction::triggered, [=]
	{
		QApplication::clipboard()->setText(serial);
	});

	// Disable options depending on software category
	const QString category = qstr(currGame.category);

	if (category == cat_disc_game)
	{
		removeGame->setEnabled(false);
	}
	else if (category != cat_hdd_game)
	{
		checkCompat->setEnabled(false);
	}

	myMenu.exec(globalPos);
}

bool game_list_frame::CreatePPUCache(const game_info& game)
{
	Emu.SetForceBoot(true);
	Emu.Stop();
	Emu.SetForceBoot(true);
	const bool success = Emu.BootGame(game->info.path, game->info.serial, true);

	if (success)
	{
		game_list_log.warning("Creating PPU Cache for %s", game->info.path);
	}
	else
	{
		game_list_log.error("Could not create PPU Cache for %s", game->info.path);
	}
	return success;
}

bool game_list_frame::RemoveCustomConfiguration(const std::string& title_id, game_info game, bool is_interactive)
{
	const std::string config_path_new = Emulator::GetCustomConfigPath(title_id);
	const std::string config_path_old = Emulator::GetCustomConfigPath(title_id, true);

	if (!fs::is_file(config_path_new) && !fs::is_file(config_path_old))
		return true;

	if (is_interactive && QMessageBox::question(this, tr("Confirm Removal"), tr("Remove custom game configuration?")) != QMessageBox::Yes)
		return true;

	bool result = true;

	for (const std::string& path : { config_path_new, config_path_old })
	{
		if (!fs::is_file(path))
		{
			continue;
		}
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

bool game_list_frame::RemoveCustomPadConfiguration(const std::string& title_id, game_info game, bool is_interactive)
{
	const std::string config_dir = Emulator::GetCustomInputConfigDir(title_id);

	if (!fs::is_dir(config_dir))
		return true;

	if (is_interactive && QMessageBox::question(this, tr("Confirm Removal"), (!Emu.IsStopped() && Emu.GetTitleID() == title_id)
		? tr("Remove custom pad configuration?\nYour configuration will revert to the global pad settings.")
		: tr("Remove custom pad configuration?")) != QMessageBox::Yes)
		return true;

	if (QDir(qstr(config_dir)).removeRecursively())
	{
		if (game)
		{
			game->hasCustomPadConfig = false;
		}
		if (!Emu.IsStopped() && Emu.GetTitleID() == title_id)
		{
			Emu.GetCallbacks().enable_pads(false);
			Emu.GetCallbacks().reset_pads(title_id);
			Emu.GetCallbacks().enable_pads(true);
		}
		game_list_log.notice("Removed pad configuration directory: %s", config_dir);
		return true;
	}
	else if (is_interactive)
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
	const u32 total = m_game_data.size();

	if (total == 0)
	{
		QMessageBox::information(this, tr("PPU Cache Batch Creation"), tr("No titles found"), QMessageBox::Ok);
		return;
	}

	progress_dialog* pdlg = new progress_dialog(tr("PPU Cache Batch Creation"), tr("Creating all PPU caches"), tr("Cancel"), 0, total, true, this);
	pdlg->setAutoClose(false);
	pdlg->setAutoReset(false);
	pdlg->show();

	u32 created = 0;
	for (const auto& game : m_game_data)
	{
		if (pdlg->wasCanceled())
		{
			game_list_log.notice("PPU Cache Batch Creation was canceled");
			break;
		}
		QApplication::processEvents();

		if (CreatePPUCache(game))
		{
			while (!Emu.IsStopped())
			{
				QApplication::processEvents();
			}
			pdlg->SetValue(++created);
		}
	}

	pdlg->setLabelText(tr("Created PPU Caches for %0 titles").arg(created));
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

	progress_dialog* pdlg = new progress_dialog(tr("SPU Cache Batch Removal"), tr("Removing all SPU caches"), tr("Cancel"), 0, total, this);
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

QPixmap game_list_frame::PaintedPixmap(const QPixmap& icon, bool paint_config_icon, bool paint_pad_config_icon, const QColor& compatibility_color)
{
	const qreal device_pixel_ratio = devicePixelRatioF();
	const QSize original_size = icon.size();

	QPixmap canvas = QPixmap(original_size * device_pixel_ratio);
	canvas.setDevicePixelRatio(device_pixel_ratio);
	canvas.fill(m_Icon_Color);

	QPainter painter(&canvas);

	if (!icon.isNull())
	{
		painter.drawPixmap(QPoint(0, 0), icon);
	}

	if (!m_isListLayout && (paint_config_icon || paint_pad_config_icon))
	{
		const int width = original_size.width() * 0.2;
		const QPoint origin = QPoint(original_size.width() - width, 0);
		QString icon_path;

		if (paint_config_icon && paint_pad_config_icon)
		{
			icon_path = ":/Icons/combo_config_bordered.png";
		}
		else if (paint_config_icon)
		{
			icon_path = ":/Icons/custom_config_2.png";
		}
		else if (paint_pad_config_icon)
		{
			icon_path = ":/Icons/controllers_2.png";
		}

		QPixmap custom_config_icon(icon_path);
		custom_config_icon.setDevicePixelRatio(device_pixel_ratio);
		painter.drawPixmap(origin, custom_config_icon.scaled(QSize(width, width) * device_pixel_ratio, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
	}

	if (compatibility_color.isValid())
	{
		const int size = original_size.height() * 0.2;
		const int spacing = original_size.height() * 0.05;
		QColor copyColor = QColor(compatibility_color);
		copyColor.setAlpha(215); // ~85% opacity
		painter.setBrush(QBrush(copyColor));
		painter.drawEllipse(spacing, spacing, size, size);
	}

	painter.end();

	return canvas.scaled(m_Icon_Size * device_pixel_ratio, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
}

void game_list_frame::ShowCustomConfigIcon(game_info game)
{
	if (!game)
	{
		return;
	}

	const std::string serial      = game->info.serial;
	const bool hasCustomConfig    = game->hasCustomConfig;
	const bool hasCustomPadConfig = game->hasCustomPadConfig;

	for (auto other_game : m_game_data)
	{
		if (other_game->info.serial == serial)
		{
			other_game->hasCustomConfig    = hasCustomConfig;
			other_game->hasCustomPadConfig = hasCustomPadConfig;
		}
	}

	RepaintIcons();
}

void game_list_frame::ResizeIcons(const int& sliderPos)
{
	m_icon_size_index = sliderPos;
	m_Icon_Size = gui_settings::SizeFromSlider(sliderPos);

	RepaintIcons();
}

void game_list_frame::RepaintIcons(const bool& fromSettings)
{
	if (fromSettings)
	{
		if (m_gui_settings->GetValue(gui::m_enableUIColors).toBool())
		{
			m_Icon_Color = m_gui_settings->GetValue(gui::gl_iconColor).value<QColor>();
		}
		else
		{
			m_Icon_Color = gui::utils::get_label_color("gamelist_icon_background_color");
		}
	}

	QList<int> indices;
	for (int i = 0; i < m_game_data.size(); ++i)
		indices.append(i);

	QtConcurrent::blockingMap(indices, [this](int& i)
	{
		auto game = m_game_data[i];
		const QColor color = getGridCompatibilityColor(game->compat.color);
		game->pxmap = PaintedPixmap(game->icon, game->hasCustomConfig, game->hasCustomPadConfig, color);
	});

	Refresh();
}

void game_list_frame::SetShowHidden(bool show)
{
	m_show_hidden = show;
}

void game_list_frame::SetListMode(const bool& isList)
{
	m_oldLayoutIsList = m_isListLayout;
	m_isListLayout = isList;

	m_gui_settings->SetValue(gui::gl_listMode, isList);

	Refresh(true);

	m_Central_Widget->setCurrentWidget(m_isListLayout ? m_gameList : m_xgrid);
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
	if (!m_isListLayout)
	{
		Refresh(false, m_xgrid->selectedItems().count());
	}
	QDockWidget::resizeEvent(event);
}

bool game_list_frame::eventFilter(QObject *object, QEvent *event)
{
	// Zoom gamelist/gamegrid
	if (event->type() == QEvent::Wheel && (object == m_gameList->verticalScrollBar() || object == m_xgrid->verticalScrollBar()))
	{
		QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);

		if (wheelEvent->modifiers() & Qt::ControlModifier)
		{
			QPoint numSteps = wheelEvent->angleDelta() / 8 / 15;	// http://doc.qt.io/qt-5/qwheelevent.html#pixelDelta
			const int value = numSteps.y();
			Q_EMIT RequestIconSizeChange(value);
			return true;
		}
	}
	else if (event->type() == QEvent::KeyPress && (object == m_gameList || object == m_xgrid))
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

		if (keyEvent->modifiers() & Qt::ControlModifier)
		{
			if (keyEvent->key() == Qt::Key_Plus)
			{
				Q_EMIT RequestIconSizeChange(1);
				return true;
			}
			else if (keyEvent->key() == Qt::Key_Minus)
			{
				Q_EMIT RequestIconSizeChange(-1);
				return true;
			}
		}
		else
		{
			if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
			{
				QTableWidgetItem* item;

				if (object == m_gameList)
					item = m_gameList->item(m_gameList->currentRow(), gui::column_icon);
				else
					item = m_xgrid->currentItem();

				if (!item || !item->isSelected())
					return false;

				game_info gameinfo = GetGameInfoFromItem(item);

				if (!gameinfo)
					return false;

				sys_log.notice("Booting from gamelist by pressing %s...", keyEvent->key() == Qt::Key_Enter ? "Enter" : "Return");
				Q_EMIT RequestBoot(gameinfo);

				return true;
			}
		}
	}
	else if (event->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
		QTableWidgetItem* item;

		if (m_isListLayout)
		{
			item = m_gameList->itemAt(helpEvent->globalPos());
		}
		else
		{
			item = m_xgrid->itemAt(helpEvent->globalPos());
		}

		if (item && !item->toolTip().isEmpty() && (!m_isListLayout || item->column() == gui::column_name || item->column() == gui::column_serial))
		{
			QToolTip::showText(helpEvent->globalPos(), item->toolTip());
		}
		else
		{
			QToolTip::hideText();
			event->ignore();
		}

		return true;
	}

	return QDockWidget::eventFilter(object, event);
}

/**
 Cleans and readds entries to table widget in UI.
*/
int game_list_frame::PopulateGameList()
{
	int result = -1;

	std::string selected_item = CurrentSelectionIconPath();

	m_gameList->clearContents();
	m_gameList->setRowCount(m_game_data.size());

	// Default locale. Uses current Qt application language.
	const QLocale locale{};
	const Localized localized;

	int row = 0, index = -1;
	for (const auto& game : m_game_data)
	{
		index++;

		if (!IsEntryVisible(game))
			continue;

		const QString serial = qstr(game->info.serial);
		const QString title = m_titles.value(serial, qstr(game->info.name));
		const QString notes = m_notes.value(serial);

		// Icon
		custom_table_widget_item* icon_item = new custom_table_widget_item;
		icon_item->setData(Qt::DecorationRole, game->pxmap);
		icon_item->setData(Qt::UserRole, index, true);
		icon_item->setData(gui::game_role, QVariant::fromValue(game));

		// Title
		custom_table_widget_item* title_item = new custom_table_widget_item(title);
		if (game->hasCustomConfig && game->hasCustomPadConfig)
		{
			title_item->setIcon(QIcon(":/Icons/combo_config_bordered.png"));
		}
		else if (game->hasCustomConfig)
		{
			title_item->setIcon(QIcon(":/Icons/custom_config.png"));
		}
		else if (game->hasCustomPadConfig)
		{
			title_item->setIcon(QIcon(":/Icons/controllers.png"));
		}

		// Serial
		custom_table_widget_item* serial_item = new custom_table_widget_item(game->info.serial);

		if (!notes.isEmpty())
		{
			const QString tool_tip = tr("%0 [%1]\n\nNotes:\n%2").arg(title).arg(serial).arg(notes);
			title_item->setToolTip(tool_tip);
			serial_item->setToolTip(tool_tip);
		}

		// Move Support (http://www.psdevwiki.com/ps3/PARAM.SFO#ATTRIBUTE)
		bool supports_move = game->info.attr & 0x800000;

		// Compatibility
		custom_table_widget_item* compat_item = new custom_table_widget_item;
		compat_item->setText(game->compat.text + (game->compat.date.isEmpty() ? "" : " (" + game->compat.date + ")"));
		compat_item->setData(Qt::UserRole, game->compat.index, true);
		compat_item->setToolTip(game->compat.tooltip);
		if (!game->compat.color.isEmpty())
		{
			compat_item->setData(Qt::DecorationRole, compat_pixmap(game->compat.color, devicePixelRatioF() * 2));
		}

		// Version
		QString app_version = qstr(game->info.app_ver);

		if (app_version == localized.category.unknown)
		{
			// Fall back to Disc/Pkg Revision
			app_version = qstr(game->info.version);
		}

		if (!game->compat.version.isEmpty() && (app_version == localized.category.unknown || game->compat.version.toDouble() > app_version.toDouble()))
		{
			app_version = tr("%0 (Update available: %1)").arg(app_version, game->compat.version);
		}

		// Playtimes
		const qint64 elapsed_ms = m_persistent_settings->GetPlaytime(serial);

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

		m_gameList->setItem(row, gui::column_icon,       icon_item);
		m_gameList->setItem(row, gui::column_name,       title_item);
		m_gameList->setItem(row, gui::column_serial,     serial_item);
		m_gameList->setItem(row, gui::column_firmware,   new custom_table_widget_item(game->info.fw));
		m_gameList->setItem(row, gui::column_version,    new custom_table_widget_item(app_version));
		m_gameList->setItem(row, gui::column_category,   new custom_table_widget_item(game->localized_category));
		m_gameList->setItem(row, gui::column_path,       new custom_table_widget_item(game->info.path));
		m_gameList->setItem(row, gui::column_move,       new custom_table_widget_item(sstr(supports_move ? tr("Supported") : tr("Not Supported")), Qt::UserRole, !supports_move));
		m_gameList->setItem(row, gui::column_resolution, new custom_table_widget_item(GetStringFromU32(game->info.resolution, localized.resolution.mode, true)));
		m_gameList->setItem(row, gui::column_sound,      new custom_table_widget_item(GetStringFromU32(game->info.sound_format, localized.sound.format, true)));
		m_gameList->setItem(row, gui::column_parental,   new custom_table_widget_item(GetStringFromU32(game->info.parental_lvl, localized.parental.level), Qt::UserRole, game->info.parental_lvl));
		m_gameList->setItem(row, gui::column_last_play,  new custom_table_widget_item(locale.toString(last_played, gui::persistent::last_played_date_format_new), Qt::UserRole, last_played));
		m_gameList->setItem(row, gui::column_playtime,   new custom_table_widget_item(GetPlayTimeByMs(elapsed_ms), Qt::UserRole, elapsed_ms));
		m_gameList->setItem(row, gui::column_compat,     compat_item);

		if (selected_item == game->info.icon_path)
		{
			result = row;
		}

		row++;
	}

	m_gameList->setRowCount(row);

	return result;
}

void game_list_frame::PopulateGameGrid(int maxCols, const QSize& image_size, const QColor& image_color)
{
	int r = 0;
	int c = 0;

	const std::string selected_item = CurrentSelectionIconPath();

	m_xgrid->deleteLater();

	const bool showText = m_icon_size_index > gui::gl_max_slider_pos * 2 / 5;

	if (m_icon_size_index < gui::gl_max_slider_pos * 2 / 3)
	{
		m_xgrid = new game_list_grid(image_size, image_color, m_Margin_Factor, m_Text_Factor * 2, showText);
	}
	else
	{
		m_xgrid = new game_list_grid(image_size, image_color, m_Margin_Factor, m_Text_Factor, showText);
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

	const int needsExtraRow = (entries % maxCols) != 0;
	const int maxRows = needsExtraRow + entries / maxCols;
	m_xgrid->setRowCount(maxRows);
	m_xgrid->setColumnCount(maxCols);

	for (const auto& app : matching_apps)
	{
		const QString serial = qstr(app->info.serial);
		const QString title = m_titles.value(serial, qstr(app->info.name));
		const QString notes = m_notes.value(serial);

		m_xgrid->addItem(app->pxmap, title, r, c);
		m_xgrid->item(r, c)->setData(gui::game_role, QVariant::fromValue(app));

		if (!notes.isEmpty())
		{
			m_xgrid->item(r, c)->setToolTip(tr("%0 [%1]\n\nNotes:\n%2").arg(title).arg(serial).arg(notes));
		}
		else
		{
			m_xgrid->item(r, c)->setToolTip(tr("%0 [%1]").arg(title).arg(serial));
		}

		if (selected_item == app->info.icon_path)
		{
			m_xgrid->setCurrentItem(m_xgrid->item(r, c));
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
			QTableWidgetItem* emptyItem = new QTableWidgetItem();
			emptyItem->setFlags(Qt::NoItemFlags);
			m_xgrid->setItem(r, col, emptyItem);
		}
	}

	m_xgrid->resizeColumnsToContents();
	m_xgrid->resizeRowsToContents();
	m_xgrid->installEventFilter(this);
	m_xgrid->verticalScrollBar()->installEventFilter(this);
}

/**
* Returns false if the game should be hidden because it doesn't match search term in toolbar.
*/
bool game_list_frame::SearchMatchesApp(const QString& name, const QString& serial) const
{
	if (!m_search_text.isEmpty())
	{
		const QString searchText = m_search_text.toLower();
		return m_titles.value(serial, name).toLower().contains(searchText) || serial.toLower().contains(searchText);
	}
	return true;
}

std::string game_list_frame::CurrentSelectionIconPath()
{
	std::string selection = "";

	if (m_gameList->selectedItems().count())
	{
		QTableWidgetItem* item = m_oldLayoutIsList ? m_gameList->item(m_gameList->currentRow(), 0) : m_xgrid->currentItem();
		QVariant var = item->data(gui::game_role);

		if (var.canConvert<game_info>())
		{
			auto game = var.value<game_info>();
			if (game)
			{
				selection = game->info.icon_path;
			}
		}
	}

	m_oldLayoutIsList = m_isListLayout;

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

game_info game_list_frame::GetGameInfoFromItem(QTableWidgetItem* item)
{
	if (item == nullptr)
	{
		return nullptr;
	}

	QVariant var = item->data(gui::game_role);
	if (!var.canConvert<game_info>())
	{
		return nullptr;
	}

	return var.value<game_info>();
}

QColor game_list_frame::getGridCompatibilityColor(const QString& string)
{
	if (m_drawCompatStatusToGrid && !m_isListLayout)
	{
		return QColor(string);
	}
	return QColor();
}

void game_list_frame::SetShowCompatibilityInGrid(bool show)
{
	m_drawCompatStatusToGrid = show;
	RepaintIcons();
	m_gui_settings->SetValue(gui::gl_draw_compat, show);
}
