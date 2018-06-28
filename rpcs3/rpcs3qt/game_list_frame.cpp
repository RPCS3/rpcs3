#include "game_list_frame.h"
#include "qt_utils.h"
#include "settings_dialog.h"
#include "table_item_delegate.h"
#include "custom_table_widget_item.h"

#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Loader/PSF.h"
#include "Utilities/types.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <set>

#include <QDesktopServices>
#include <QHeaderView>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollBar>
#include <QInputDialog>
#include <QToolTip>
#include <QApplication>
#include <QClipboard>

inline std::string sstr(const QString& _in) { return _in.toStdString(); }

game_list_frame::game_list_frame(std::shared_ptr<gui_settings> guiSettings, std::shared_ptr<emu_settings> emuSettings, QWidget *parent)
	: custom_dock_widget(tr("Game List"), parent), m_gui_settings(guiSettings), m_emu_settings(emuSettings)
{
	m_isListLayout    = m_gui_settings->GetValue(gui::gl_listMode).toBool();
	m_Margin_Factor   = m_gui_settings->GetValue(gui::gl_marginFactor).toReal();
	m_Text_Factor     = m_gui_settings->GetValue(gui::gl_textFactor).toReal();
	m_Icon_Color      = m_gui_settings->GetValue(gui::gl_iconColor).value<QColor>();
	m_colSortOrder    = m_gui_settings->GetValue(gui::gl_sortAsc).toBool() ? Qt::AscendingOrder : Qt::DescendingOrder;
	m_sortColumn      = m_gui_settings->GetValue(gui::gl_sortCol).toInt();
	m_hidden_list     = m_gui_settings->GetValue(gui::gl_hidden_list).toStringList().toSet();

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
	m_gameList->setItemDelegate(new table_item_delegate(this));
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
	AddColumn(gui::column_compat,     tr("Compatibility"),         tr("Show Compatibility"));

	// Events
	connect(m_gameList, &QTableWidget::customContextMenuRequested, this, &game_list_frame::ShowContextMenu);
	connect(m_gameList, &QTableWidget::itemDoubleClicked, this, &game_list_frame::doubleClickedSlot);

	connect(m_gameList->horizontalHeader(), &QHeaderView::sectionClicked, this, &game_list_frame::OnColClicked);
	connect(m_gameList->horizontalHeader(), &QHeaderView::customContextMenuRequested, [=](const QPoint& pos)
	{
		QMenu* configure = new QMenu(this);
		configure->addActions(m_columnActs);
		configure->exec(mapToGlobal(pos));
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
		return category::CategoryInMap(game->info.category, category::cat_boot);
	};

	bool is_visible = m_show_hidden || !m_hidden_list.contains(qstr(game->info.serial));
	return is_visible && matches_category() && SearchMatchesApp(game->info.name, game->info.serial);
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

void game_list_frame::Refresh(const bool fromDrive, const bool scrollAfter)
{
	if (fromDrive)
	{
		// Load PSF

		m_game_data.clear();
		m_notes.clear();

		const std::string _hdd = Emu.GetHddDir();

		std::vector<std::string> path_list;

		const auto add_dir = [&](const std::string& path)
		{
			for (const auto& entry : fs::dir(path))
			{
				if (entry.is_directory)
				{
					path_list.emplace_back(path + entry.name);
				}
			}
		};

		add_dir(_hdd + "game/");
		add_dir(_hdd + "disc/");

		for (auto pair : YAML::Load(fs::file{fs::get_config_dir() + "/games.yml", fs::read + fs::create}.to_string()))
		{
			path_list.push_back(pair.second.Scalar());
			path_list.back().resize(path_list.back().find_last_not_of('/') + 1);
		}

		// Used to remove duplications from the list (serial -> set of cats)
		std::map<std::string, std::set<std::string>> serial_cat;

		QSet<QString> serials;

		for (const auto& dir : path_list) { try
		{
			const std::string sfb = dir + "/PS3_DISC.SFB";
			const std::string sfo = dir + (fs::is_file(sfb) ? "/PS3_GAME/PARAM.SFO" : "/PARAM.SFO");

			const fs::file sfo_file(sfo);
			if (!sfo_file)
			{
				continue;
			}

			const auto psf = psf::load_object(sfo_file);

			GameInfo game;
			game.path         = dir;
			game.serial       = psf::get_string(psf, "TITLE_ID", "");
			game.name         = psf::get_string(psf, "TITLE", sstr(category::unknown));
			game.app_ver      = psf::get_string(psf, "APP_VER", sstr(category::unknown));
			game.category     = psf::get_string(psf, "CATEGORY", sstr(category::unknown));
			game.fw           = psf::get_string(psf, "PS3_SYSTEM_VER", sstr(category::unknown));
			game.parental_lvl = psf::get_integer(psf, "PARENTAL_LEVEL");
			game.resolution   = psf::get_integer(psf, "RESOLUTION");
			game.sound_format = psf::get_integer(psf, "SOUND_FORMAT");
			game.bootable     = psf::get_integer(psf, "BOOTABLE", 0);
			game.attr         = psf::get_integer(psf, "ATTRIBUTE", 0);

			// Detect duplication
			if (!serial_cat[game.serial].emplace(game.category).second)
			{
				continue;
			}

			QString serial = qstr(game.serial);
			m_notes[serial] = m_gui_settings->GetValue(gui::notes, serial, "").toString();
			serials.insert(serial);

			auto cat = category::cat_boot.find(game.category);
			if (cat != category::cat_boot.end())
			{
				if (game.category == "DG")
				{
					game.icon_path = dir + "/PS3_GAME/ICON0.PNG";
				}
				else
				{
					game.icon_path = dir + "/ICON0.PNG";
				}

				game.category = sstr(cat->second);
			}
			else if ((cat = category::cat_data.find(game.category)) != category::cat_data.end())
			{
				game.icon_path = dir + "/ICON0.PNG";
				game.category = sstr(cat->second);
			}
			else if (game.category == sstr(category::unknown))
			{
				game.icon_path = dir + "/ICON0.PNG";
			}
			else
			{
				game.icon_path = dir + "/ICON0.PNG";
				game.category = sstr(category::other);
			}

			// Load Image
			QImage img;

			if (game.icon_path.empty() || !img.load(qstr(game.icon_path)))
			{
				LOG_WARNING(GENERAL, "Could not load image from path %s", sstr(QDir(qstr(game.icon_path)).absolutePath()));
			}

			bool hasCustomConfig = fs::is_file(fs::get_config_dir() + "data/" + game.serial + "/config.yml");

			QPixmap pxmap = PaintedPixmap(img, hasCustomConfig);

			m_game_data.push_back(game_info(new gui_game_info{ game, m_game_compat->GetCompatibility(game.serial), img, pxmap, hasCustomConfig }));
		}
		catch (const std::exception& e)
		{
			LOG_FATAL(GENERAL, "Failed to update game list at %s\n%s thrown: %s", dir, typeid(e).name(), e.what());
			continue;
			// Blame MSVC for double }}
		}}

		auto op = [](const game_info& game1, const game_info& game2)
		{
			return qstr(game1->info.name).toLower() < qstr(game2->info.name).toLower();
		};

		// Sort by name at the very least.
		std::sort(m_game_data.begin(), m_game_data.end(), op);

		// clean up hidden games list
		m_hidden_list.intersect(serials);
		m_gui_settings->SetValue(gui::gl_hidden_list, QStringList(m_hidden_list.toList()));
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

		int scroll_position = m_xgrid->verticalScrollBar()->value();
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
	QString path = qstr(spath);
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

	if (game.get() == nullptr)
	{
		return;
	}

	LOG_NOTICE(LOADER, "Booting from gamelist per doubleclick...");
	Q_EMIT RequestBoot(game->info.path);
}

void game_list_frame::ShowContextMenu(const QPoint &pos)
{
	QPoint globalPos;
	QTableWidgetItem* item;

	if (m_isListLayout)
	{
		item = m_gameList->item(m_gameList->indexAt(pos).row(), gui::column_icon);
		globalPos = m_gameList->mapToGlobal(pos);
	}
	else
	{
		QModelIndex mi = m_xgrid->indexAt(pos);
		item = m_xgrid->item(mi.row(), mi.column());
		globalPos = m_xgrid->mapToGlobal(pos);
	}

	game_info gameinfo = GetGameInfoFromItem(item);
	if (gameinfo.get() == nullptr)
	{
		return;
	}

	GameInfo currGame = gameinfo->info;
	const QString serial = qstr(currGame.serial);
	const QString name = qstr(currGame.name).simplified();

	// Make Actions
	QMenu myMenu;
	QAction* boot = myMenu.addAction(tr("&Boot"));
	QFont f = boot->font();
	f.setBold(true);
	boot->setFont(f);
	QAction* configure = myMenu.addAction(tr("&Configure"));
	QAction* createLLVMCache = myMenu.addAction(tr("&Create LLVM Cache"));
	myMenu.addSeparator();
	QAction* hide_serial = myMenu.addAction(tr("&Hide From Game List"));
	hide_serial->setCheckable(true);
	hide_serial->setChecked(m_hidden_list.contains(serial));
	myMenu.addSeparator();
	QAction* removeGame = myMenu.addAction(tr("&Remove %1").arg(qstr(currGame.category)));
	QAction* removeConfig = myMenu.addAction(tr("&Remove Custom Configuration"));
	QAction* deleteShadersCache = myMenu.addAction(tr("&Delete Shaders Cache"));
	QAction* deleteLLVMCache = myMenu.addAction(tr("&Delete LLVM Cache"));
	QAction* deleteSPUCache = myMenu.addAction(tr("&Delete SPU Cache"));
	myMenu.addSeparator();
	QAction* openGameFolder = myMenu.addAction(tr("&Open Install Folder"));
	QAction* openConfig = myMenu.addAction(tr("&Open Config Folder"));
	myMenu.addSeparator();
	QAction* checkCompat = myMenu.addAction(tr("&Check Game Compatibility"));
	QAction* downloadCompat = myMenu.addAction(tr("&Download Compatibility Database"));
	myMenu.addSeparator();
	QAction* editNotes = myMenu.addAction(tr("&Edit Tooltip Notes"));
	QMenu* info_menu = myMenu.addMenu(tr("&Copy Info"));
	QAction* copy_info = info_menu->addAction(tr("&Copy Name + Serial"));
	QAction* copy_name = info_menu->addAction(tr("&Copy Name"));
	QAction* copy_serial = info_menu->addAction(tr("&Copy Serial"));

	const std::string config_base_dir = fs::get_config_dir() + "data/" + currGame.serial;

	connect(boot, &QAction::triggered, [=]
	{
		LOG_NOTICE(LOADER, "Booting from gamelist per context menu...");
		Q_EMIT RequestBoot(currGame.path);
	});
	connect(configure, &QAction::triggered, [=]
	{
		settings_dialog dlg(m_gui_settings, m_emu_settings, 0, this, &currGame);
		if (dlg.exec() == QDialog::Accepted && !gameinfo->hasCustomConfig)
		{
			ShowCustomConfigIcon(item, true);
		}
	});
	connect(hide_serial, &QAction::triggered, [=](bool checked)
	{
		if (checked)
			m_hidden_list.insert(serial);
		else
			m_hidden_list.remove(serial);

		m_gui_settings->SetValue(gui::gl_hidden_list, QStringList(m_hidden_list.toList()));
		Refresh();
	});
	connect(createLLVMCache, &QAction::triggered, [=]
	{
		Emu.SetForceBoot(true);
		Emu.Stop();
		Emu.SetForceBoot(true);
		Emu.BootGame(currGame.path, true);
	});
	connect(removeGame, &QAction::triggered, [=]
	{
		if (currGame.path.empty())
		{
			LOG_FATAL(GENERAL, "Cannot delete game. Path is empty");
			return;
		}

		QMessageBox* mb = new QMessageBox(QMessageBox::Question, tr("Confirm %1 Removal").arg(qstr(currGame.category)), tr("Permanently remove %0 from drive?\nPath: %1").arg(name).arg(qstr(currGame.path)), QMessageBox::Yes | QMessageBox::No, this);
		mb->setCheckBox(new QCheckBox(tr("Remove caches and custom config")));
		mb->deleteLater();
		if (mb->exec() == QMessageBox::Yes)
		{
			if (mb->checkBox()->isChecked())
			{
				DeleteShadersCache(config_base_dir);
				DeleteLLVMCache(config_base_dir);
				DeleteSPUCache(config_base_dir);
				RemoveCustomConfiguration(config_base_dir);
			}
			fs::remove_all(currGame.path);
			m_game_data.erase(std::remove(m_game_data.begin(), m_game_data.end(), gameinfo), m_game_data.end());
			if (m_isListLayout)
			{
				m_gameList->removeRow(m_gameList->currentItem()->row());
			}
			else
			{
				Refresh();
			}
			LOG_SUCCESS(GENERAL, "Removed %s %s in %s", currGame.category, currGame.name, currGame.path);
		}
	});
	connect(removeConfig, &QAction::triggered, [=]()
	{
		if (RemoveCustomConfiguration(config_base_dir, true))
		{
			ShowCustomConfigIcon(item, false);
		}
	});
	connect(deleteShadersCache, &QAction::triggered, [=]()
	{
		DeleteShadersCache(config_base_dir, true);
	});
	connect(deleteLLVMCache, &QAction::triggered, [=]()
	{
		DeleteLLVMCache(config_base_dir, true);
	});
	connect(deleteSPUCache, &QAction::triggered, [=]()
	{
		DeleteSPUCache(config_base_dir, true);
	});
	connect(openGameFolder, &QAction::triggered, [=]()
	{
		open_dir(currGame.path);
	});
	connect(openConfig, &QAction::triggered, [=]()
	{
		open_dir(fs::get_config_dir() + "data/" + currGame.serial);
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
	connect(editNotes, &QAction::triggered, [=]
	{
		bool accepted;
		const QString old_notes = m_gui_settings->GetValue(gui::notes, serial, "").toString();
		const QString new_notes = QInputDialog::getMultiLineText(this, tr("Edit Tooltip Notes"), QString("%0\n%1").arg(name).arg(serial), old_notes, &accepted);

		if (accepted)
		{
			m_notes[serial] = new_notes;
			m_gui_settings->SetValue(gui::notes, serial, new_notes);
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

	//Disable options depending on software category
	QString category = qstr(currGame.category);

	if (category == category::disc_Game)
	{
		removeGame->setEnabled(false);
	}
	else if (0)
	{
		boot->setEnabled(false), f.setBold(false), boot->setFont(f);
		configure->setEnabled(false);
		removeConfig->setEnabled(false);
		openConfig->setEnabled(false);
		checkCompat->setEnabled(false);
	}
	else if (category != category::hdd_Game)
	{
		checkCompat->setEnabled(false);
	}

	// Disable removeconfig if no config exists.
	removeConfig->setEnabled(gameinfo->hasCustomConfig);

	// remove delete options if necessary
	if (!fs::is_dir(config_base_dir))
	{
		deleteShadersCache->setEnabled(false);
		deleteLLVMCache->setEnabled(false);
		deleteSPUCache->setEnabled(false);
	}

	myMenu.exec(globalPos);
}

bool game_list_frame::RemoveCustomConfiguration(const std::string& base_dir, bool is_interactive)
{
	const std::string config_path = base_dir + "/config.yml";

	if (!fs::is_file(config_path))
		return false;

	if (is_interactive && QMessageBox::question(this, tr("Confirm Delete"), tr("Delete custom game configuration?")) != QMessageBox::Yes)
		return false;

	if (fs::remove_file(config_path))
	{
		LOG_SUCCESS(GENERAL, "Removed configuration file: %s", config_path);
		return true;
	}
	else
	{
		QMessageBox::warning(this, tr("Warning!"), tr("Failed to delete configuration file!"));
		LOG_FATAL(GENERAL, "Failed to delete configuration file: %s\nError: %s", config_path, fs::g_tls_error);
		return false;
	}
}

bool game_list_frame::DeleteShadersCache(const std::string& base_dir, bool is_interactive)
{
	if (!fs::is_dir(base_dir))
		return false;

	if (is_interactive && QMessageBox::question(this, tr("Confirm Delete"), tr("Delete shaders cache?")) != QMessageBox::Yes)
		return false;

	QDirIterator dir_iter(qstr(base_dir), QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
	while (dir_iter.hasNext())
	{
		const QString filepath = dir_iter.next();

		if (dir_iter.fileName() == "shaders_cache")
		{
			if (QDir(filepath).removeRecursively())
				LOG_NOTICE(GENERAL, "Removed shaders cache dir: %s", sstr(filepath));
			else
				LOG_WARNING(GENERAL, "Could not remove shaders cache file: %s", sstr(filepath));
		}
	}

	LOG_SUCCESS(GENERAL, "Removed shaders cache in %s", base_dir);
	return true;
}

bool game_list_frame::DeleteLLVMCache(const std::string& base_dir, bool is_interactive)
{
	if (!fs::is_dir(base_dir))
		return false;

	if (is_interactive && QMessageBox::question(this, tr("Confirm Delete"), tr("Delete LLVM cache?")) != QMessageBox::Yes)
		return false;

	QDirIterator dir_iter(qstr(base_dir), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
	while (dir_iter.hasNext())
	{
		const QString filepath = dir_iter.next();

		if (dir_iter.fileInfo().absoluteFilePath().endsWith(".obj", Qt::CaseInsensitive))
		{
			if (QFile::remove(filepath))
				LOG_NOTICE(GENERAL, "Removed LLVM cache file: %s", sstr(filepath));
			else
				LOG_WARNING(GENERAL, "Could not remove LLVM cache file: %s", sstr(filepath));
		}
	}

	LOG_SUCCESS(GENERAL, "Removed LLVM cache in %s", base_dir);
	return true;
}

bool game_list_frame::DeleteSPUCache(const std::string& base_dir, bool is_interactive)
{
	if (!fs::is_dir(base_dir))
		return false;

	if (is_interactive && QMessageBox::question(this, tr("Confirm Delete"), tr("Delete SPU cache?")) != QMessageBox::Yes)
		return false;

	QDirIterator dir_iter(qstr(base_dir), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
	while (dir_iter.hasNext())
	{
		const QString filepath = dir_iter.next();

		if (dir_iter.fileInfo().absoluteFilePath().endsWith(".dat", Qt::CaseInsensitive))
		{
			if (QFile::remove(filepath))
				LOG_NOTICE(GENERAL, "Removed SPU cache file: %s", sstr(filepath));
			else
				LOG_WARNING(GENERAL, "Could not remove SPU cache file: %s", sstr(filepath));
		}
	}

	LOG_SUCCESS(GENERAL, "Removed SPU cache in %s", base_dir);
	return true;
}

QPixmap game_list_frame::PaintedPixmap(const QImage& img, bool paint_config_icon)
{
	const QSize original_size = img.size();

	QImage image = QImage(original_size, QImage::Format_ARGB32);
	image.fill(m_Icon_Color);

	QPainter painter(&image);

	if (!img.isNull())
	{
		painter.drawImage(QPoint(0, 0), img);
	}

	if (paint_config_icon && !m_isListLayout)
	{
		const int width = original_size.width() * 0.2;
		const QPoint origin = QPoint(original_size.width() - width, 0);
		painter.drawImage(origin, QImage(":/Icons/custom_config_2.png").scaled(QSize(width, width), Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
	}

	painter.end();

	return QPixmap::fromImage(image.scaled(m_Icon_Size, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
}

void game_list_frame::ShowCustomConfigIcon(QTableWidgetItem* item, bool enabled)
{
	auto game = GetGameInfoFromItem(item);
	if (game == nullptr)
	{
		return;
	}

	game->hasCustomConfig = enabled;
	game->pxmap = PaintedPixmap(game->icon, enabled);

	if (!m_isListLayout)
	{
		int r = m_xgrid->currentItem()->row(), c = m_xgrid->currentItem()->column();
		m_xgrid->addItem(game->pxmap, qstr(game->info.name).simplified(), r, c);
		m_xgrid->item(r, c)->setData(gui::game_role, QVariant::fromValue(game));
	}
	else if (enabled)
	{
		m_gameList->item(item->row(), gui::column_name)->setIcon(QIcon(":/Icons/custom_config.png"));
	}
	else
	{
		m_gameList->setItem(item->row(), gui::column_name, new custom_table_widget_item(game->info.name));
	}
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

	for (auto& game : m_game_data)
	{
		game->pxmap = PaintedPixmap(game->icon, game->hasCustomConfig);
	}

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

				if (gameinfo.get() == nullptr)
					return false;

				LOG_NOTICE(LOADER, "Booting from gamelist by pressing %s...", keyEvent->key() == Qt::Key_Enter ? "Enter" : "Return");
				Q_EMIT RequestBoot(gameinfo->info.path);

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
	m_gameList->setRowCount((int)m_game_data.size());

	int row = 0, index = -1;
	for (const auto& game : m_game_data)
	{
		index++;

		if (!IsEntryVisible(game))
			continue;

		const QString name = qstr(game->info.name).simplified();
		const QString serial = qstr(game->info.serial);
		const QString notes = m_notes[serial];

		// Icon
		custom_table_widget_item* icon_item = new custom_table_widget_item;
		icon_item->setData(Qt::DecorationRole, game->pxmap);
		icon_item->setData(Qt::UserRole, index, true);
		icon_item->setData(gui::game_role, QVariant::fromValue(game));

		// Title
		custom_table_widget_item* title_item = new custom_table_widget_item(game->info.name);
		if (game->hasCustomConfig)
		{
			title_item->setIcon(QIcon(":/Icons/custom_config.png"));
		}

		// Serial
		custom_table_widget_item* serial_item = new custom_table_widget_item(game->info.serial);

		if (!notes.isEmpty())
		{
			const QString tool_tip = tr("%0 [%1]\n\nNotes:\n%2").arg(name).arg(serial).arg(notes);
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
			compat_item->setData(Qt::DecorationRole, compat_pixmap(game->compat.color));
		}

		m_gameList->setItem(row, gui::column_icon,       icon_item);
		m_gameList->setItem(row, gui::column_name,       title_item);
		m_gameList->setItem(row, gui::column_serial,     serial_item);
		m_gameList->setItem(row, gui::column_firmware,   new custom_table_widget_item(game->info.fw));
		m_gameList->setItem(row, gui::column_version,    new custom_table_widget_item(game->info.app_ver));
		m_gameList->setItem(row, gui::column_category,   new custom_table_widget_item(game->info.category));
		m_gameList->setItem(row, gui::column_path,       new custom_table_widget_item(game->info.path));
		m_gameList->setItem(row, gui::column_move,       new custom_table_widget_item(sstr(supports_move ? tr("Supported") : tr("Not Supported")), Qt::UserRole, !supports_move));
		m_gameList->setItem(row, gui::column_resolution, new custom_table_widget_item(GetStringFromU32(game->info.resolution, resolution::mode, true)));
		m_gameList->setItem(row, gui::column_sound,      new custom_table_widget_item(GetStringFromU32(game->info.sound_format, sound::format, true)));
		m_gameList->setItem(row, gui::column_parental,   new custom_table_widget_item(GetStringFromU32(game->info.parental_lvl, parental::level), Qt::UserRole, game->info.parental_lvl));
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

	std::string selected_item = CurrentSelectionIconPath();

	m_xgrid->deleteLater();

	bool showText = m_icon_size_index > gui::gl_max_slider_pos * 2 / 5;

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

	int entries = matching_apps.count();

	// Edge cases!
	if (entries == 0)
	{ // For whatever reason, 0%x is division by zero.  Absolute nonsense by definition of modulus.  But, I'll acquiesce.
		return;
	}

	maxCols = std::clamp(maxCols, 1, entries);

	int needsExtraRow = (entries % maxCols) != 0;
	int maxRows = needsExtraRow + entries / maxCols;
	m_xgrid->setRowCount(maxRows);
	m_xgrid->setColumnCount(maxCols);

	QString title, serial, notes;

	for (const auto& app : matching_apps)
	{
		title = qstr(app->info.name).simplified(); // simplified() forces single line text
		serial = qstr(app->info.serial);
		notes = m_notes[serial];

		m_xgrid->addItem(app->pxmap, title,  r, c);
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
bool game_list_frame::SearchMatchesApp(const std::string& name, const std::string& serial)
{
	if (!m_search_text.isEmpty())
	{
		QString searchText = m_search_text.toLower();
		return qstr(name).toLower().contains(searchText) || qstr(serial).toLower().contains(searchText);
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
