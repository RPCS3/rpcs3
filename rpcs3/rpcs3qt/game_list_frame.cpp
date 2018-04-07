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

inline std::string sstr(const QString& _in) { return _in.toStdString(); }
inline QSize sizeFromSlider(const int& pos) { return gui::gl_icon_size_min + (gui::gl_icon_size_max - gui::gl_icon_size_min) * (pos / (float)gui::gl_max_slider_pos); }

game_list_frame::game_list_frame(std::shared_ptr<gui_settings> guiSettings, std::shared_ptr<emu_settings> emuSettings, QWidget *parent)
	: custom_dock_widget(tr("Game List"), parent), xgui_settings(guiSettings), xemu_settings(emuSettings)
{
	m_isListLayout    = xgui_settings->GetValue(gui::gl_listMode).toBool();
	m_Margin_Factor   = xgui_settings->GetValue(gui::gl_marginFactor).toReal();
	m_Text_Factor     = xgui_settings->GetValue(gui::gl_textFactor).toReal();
	m_Icon_Color      = xgui_settings->GetValue(gui::gl_iconColor).value<QColor>();
	m_colSortOrder    = xgui_settings->GetValue(gui::gl_sortAsc).toBool() ? Qt::AscendingOrder : Qt::DescendingOrder;
	m_sortColumn      = xgui_settings->GetValue(gui::gl_sortCol).toInt();
	m_hidden_list     = xgui_settings->GetValue(gui::gl_hidden_list).toStringList().toSet();

	m_oldLayoutIsList = m_isListLayout;

	// Save factors for first setup
	xgui_settings->SetValue(gui::gl_iconColor, m_Icon_Color);
	xgui_settings->SetValue(gui::gl_marginFactor, m_Margin_Factor);
	xgui_settings->SetValue(gui::gl_textFactor, m_Text_Factor);

	m_Game_Dock = new QMainWindow(this);
	m_Game_Dock->setWindowFlags(Qt::Widget);
	setWidget(m_Game_Dock);

	m_xgrid = new game_list_grid(QSize(), m_Icon_Color, m_Margin_Factor, m_Text_Factor, false);

	m_gameList = new game_list();
	m_gameList->setShowGrid(false);
	m_gameList->setItemDelegate(new table_item_delegate(this));
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
	m_gameList->setContextMenuPolicy(Qt::CustomContextMenu);
	m_gameList->setAlternatingRowColors(true);
	m_gameList->installEventFilter(this);

	m_gameList->setColumnCount(gui::column_count);
	m_gameList->setHorizontalHeaderItem(gui::column_icon,       new QTableWidgetItem(tr("Icon")));
	m_gameList->setHorizontalHeaderItem(gui::column_name,       new QTableWidgetItem(tr("Name")));
	m_gameList->setHorizontalHeaderItem(gui::column_serial,     new QTableWidgetItem(tr("Serial")));
	m_gameList->setHorizontalHeaderItem(gui::column_firmware,   new QTableWidgetItem(tr("Firmware")));
	m_gameList->setHorizontalHeaderItem(gui::column_version,    new QTableWidgetItem(tr("Version")));
	m_gameList->setHorizontalHeaderItem(gui::column_category,   new QTableWidgetItem(tr("Category")));
	m_gameList->setHorizontalHeaderItem(gui::column_path,       new QTableWidgetItem(tr("Path")));
	m_gameList->setHorizontalHeaderItem(gui::column_resolution, new QTableWidgetItem(tr("Supported Resolutions")));
	m_gameList->setHorizontalHeaderItem(gui::column_sound,      new QTableWidgetItem(tr("Sound Formats")));
	m_gameList->setHorizontalHeaderItem(gui::column_parental,   new QTableWidgetItem(tr("Parental Level")));
	m_gameList->setHorizontalHeaderItem(gui::column_compat,     new QTableWidgetItem(tr("Compatibility")));

	// since this won't work somehow: gameList->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	for (int i = 0; i < m_gameList->horizontalHeader()->count(); i++)
	{
		m_gameList->horizontalHeaderItem(i)->setTextAlignment(Qt::AlignLeft);
	}

	m_game_compat = std::make_unique<game_compatibility>(xgui_settings);

	m_Central_Widget = new QStackedWidget(this);
	m_Central_Widget->addWidget(m_gameList);
	m_Central_Widget->addWidget(m_xgrid);
	m_Central_Widget->setCurrentWidget(m_isListLayout ? m_gameList : m_xgrid);

	m_Game_Dock->setCentralWidget(m_Central_Widget);

	// Actions regarding showing/hiding columns
	QAction* showIconColAct          = new QAction(tr("Show Icons"), this);
	QAction* showNameColAct          = new QAction(tr("Show Names"), this);
	QAction* showSerialColAct        = new QAction(tr("Show Serials"), this);
	QAction* showFWColAct            = new QAction(tr("Show Firmwares"), this);
	QAction* showAppVersionColAct    = new QAction(tr("Show Versions"), this);
	QAction* showCategoryColAct      = new QAction(tr("Show Categories"), this);
	QAction* showPathColAct          = new QAction(tr("Show Paths"), this);
	QAction* showResolutionColAct    = new QAction(tr("Show Supported Resolutions"), this);
	QAction* showSoundFormatColAct   = new QAction(tr("Show Sound Formats"), this);
	QAction* showParentalLevelColAct = new QAction(tr("Show Parental Levels"), this);
	QAction* showCompatibilityAct    = new QAction(tr("Show Compatibilities"), this);

	m_columnActs = { showIconColAct, showNameColAct, showSerialColAct, showFWColAct, showAppVersionColAct, showCategoryColAct, showPathColAct,
		showResolutionColAct, showSoundFormatColAct, showParentalLevelColAct, showCompatibilityAct };

	// Events
	connect(m_gameList, &QTableWidget::customContextMenuRequested, this, &game_list_frame::ShowContextMenu);
	connect(m_gameList, &QTableWidget::doubleClicked, this, &game_list_frame::doubleClickedSlot);

	connect(m_gameList->horizontalHeader(), &QHeaderView::sectionClicked, this, &game_list_frame::OnColClicked);
	connect(m_gameList->horizontalHeader(), &QHeaderView::customContextMenuRequested, [=](const QPoint& pos)
	{
		QMenu* configure = new QMenu(this);
		configure->addActions(m_columnActs);
		configure->exec(mapToGlobal(pos));
	});

	connect(m_xgrid, &QTableWidget::doubleClicked, this, &game_list_frame::doubleClickedSlot);
	connect(m_xgrid, &QTableWidget::customContextMenuRequested, this, &game_list_frame::ShowContextMenu);

	connect(m_game_compat.get(), &game_compatibility::DownloadStarted, [=]()
	{
		for (auto& game : m_game_data)
		{
			game.compat = m_game_compat->GetStatusData("Download");
		}
		Refresh();
	});
	connect(m_game_compat.get(), &game_compatibility::DownloadFinished, [=]()
	{
		for (auto& game : m_game_data)
		{
			game.compat = m_game_compat->GetCompatibility(game.info.serial);
		}
		Refresh();
	});
	connect(m_game_compat.get(), &game_compatibility::DownloadError, [=](const QString& error)
	{
		for (auto& game : m_game_data)
		{
			game.compat = m_game_compat->GetCompatibility(game.info.serial);
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
					if (xgui_settings->GetGamelistColVisibility(i) && ++c > 1)
						break;
				}
				if (c < 2)
				{
					m_columnActs[col]->setChecked(true); // re-enable the checkbox if we don't change the actual state
					return;
				}
			}
			m_gameList->setColumnHidden(col, !checked); // Negate because it's a set col hidden and we have menu say show.
			xgui_settings->SetGamelistColVisibility(col, checked);

			if (checked) // handle hidden columns that have zero width after showing them (stuck between others)
			{
				if (m_gameList->columnWidth(col) <= m_gameList->horizontalHeader()->minimumSectionSize())
					m_gameList->setColumnWidth(col, m_gameList->horizontalHeader()->minimumSectionSize());
			}
		});
	}
}

void game_list_frame::LoadSettings()
{
	QByteArray state = xgui_settings->GetValue(gui::gl_state).toByteArray();

	if (state.isEmpty())
	{ // If no settings exist, go to default.
		if (m_gameList->rowCount() > 0)
		{
			m_gameList->verticalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
			m_gameList->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
			m_gameList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
		}
	}
	else
	{
		m_gameList->horizontalHeader()->restoreState(state);
	}

	for (int col = 0; col < m_columnActs.count(); ++col)
	{
		bool vis = xgui_settings->GetGamelistColVisibility(col);
		m_columnActs[col]->setChecked(vis);
		m_gameList->setColumnHidden(col, !vis);
	}

	m_gameList->horizontalHeader()->restoreState(m_gameList->horizontalHeader()->saveState());
	m_gameList->horizontalHeader()->stretchLastSection();

	m_colSortOrder = xgui_settings->GetValue(gui::gl_sortAsc).toBool() ? Qt::AscendingOrder : Qt::DescendingOrder;

	m_sortColumn = xgui_settings->GetValue(gui::gl_sortCol).toInt();

	m_categoryFilters = xgui_settings->GetGameListCategoryFilters();

	Refresh(true);
}

game_list_frame::~game_list_frame()
{
	SaveSettings();
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

	xgui_settings->SetValue(gui::gl_sortAsc, m_colSortOrder == Qt::AscendingOrder);
	xgui_settings->SetValue(gui::gl_sortCol, col);

	SortGameList();
}

// Get visibility of entries
bool game_list_frame::IsEntryVisible(const GUI_GameInfo& game)
{
	auto matches_category = [&]()
	{
		if (m_isListLayout)
			return m_categoryFilters.contains(qstr(game.info.category));
		return category::CategoryInMap(game.info.category, category::cat_boot);
	};
	bool is_visible = m_show_hidden || !m_hidden_list.contains(qstr(game.info.serial));
	return is_visible && matches_category() && SearchMatchesApp(game.info.name, game.info.serial);
}

void game_list_frame::SortGameList()
{
	m_gameList->sortByColumn(m_sortColumn, m_colSortOrder);
	m_gameList->verticalHeader()->setMinimumSectionSize(m_Icon_Size.height());
	m_gameList->verticalHeader()->setMaximumSectionSize(m_Icon_Size.height());
	m_gameList->resizeRowsToContents();
	m_gameList->resizeColumnToContents(0);
}

void game_list_frame::Refresh(const bool fromDrive, const bool scrollAfter)
{
	if (fromDrive)
	{
		// Load PSF

		m_game_data.clear();

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

			// Detect duplication
			if (!serial_cat[game.serial].emplace(game.category).second)
			{
				continue;
			}

			serials.insert(qstr(game.serial));

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

			m_game_data.push_back({ game, m_game_compat->GetCompatibility(game.serial), img, pxmap, hasCustomConfig });
		}
		catch (const std::exception& e)
		{
			LOG_FATAL(GENERAL, "Failed to update game list at %s\n%s thrown: %s", dir, typeid(e).name(), e.what());
			continue;
			// Blame MSVC for double }}
		}}

		auto op = [](const GUI_GameInfo& game1, const GUI_GameInfo& game2)
		{
			return qstr(game1.info.name).toLower() < qstr(game2.info.name).toLower();
		};

		// Sort by name at the very least.
		std::sort(m_game_data.begin(), m_game_data.end(), op);

		// clean up hidden games list
		m_hidden_list.intersect(serials);
		xgui_settings->SetValue(gui::gl_hidden_list, QStringList(m_hidden_list.toList()));
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
		connect(m_xgrid, &QTableWidget::doubleClicked, this, &game_list_frame::doubleClickedSlot);
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
		xgui_settings->SetGamelistColVisibility(col, m_columnActs[col]->isChecked());
	}
	xgui_settings->SetValue(gui::gl_sortCol, m_sortColumn);
	xgui_settings->SetValue(gui::gl_sortAsc, m_colSortOrder == Qt::AscendingOrder);

	xgui_settings->SetValue(gui::gl_state, m_gameList->horizontalHeader()->saveState());
}

static void open_dir(const std::string& spath)
{
	fs::create_dir(spath);
	QString path = qstr(spath);
	QDesktopServices::openUrl(QUrl("file:///" + path));
}

void game_list_frame::doubleClickedSlot(const QModelIndex& index)
{
	int i;

	if (m_isListLayout)
	{
		i = m_gameList->item(index.row(), gui::column_icon)->data(Qt::UserRole).toInt();
	}
	else
	{
		i = m_xgrid->item(index.row(), index.column())->data(Qt::ItemDataRole::UserRole).toInt();
	}

	LOG_NOTICE(LOADER, "Booting from gamelist per doubleclick...");
	Q_EMIT RequestBoot(m_game_data[i].info.path);
}

void game_list_frame::ShowContextMenu(const QPoint &pos)
{
	int index;

	if (m_isListLayout)
	{
		int row = m_gameList->indexAt(pos).row();
		QTableWidgetItem* item = m_gameList->item(row, gui::column_icon);
		if (item == nullptr) return;  // null happens if you are double clicking in dockwidget area on nothing.
		index = item->data(Qt::UserRole).toInt();
	}
	else
	{
		int row = m_xgrid->indexAt(pos).row();
		int col = m_xgrid->indexAt(pos).column();
		QTableWidgetItem* item = m_xgrid->item(row, col);
		if (item == nullptr) return;  // null happens if you are double clicking in dockwidget area on nothing.
		index = item->data(Qt::ItemDataRole::UserRole).toInt();
	}
	ShowSpecifiedContextMenu(pos, index);
}

void game_list_frame::ShowSpecifiedContextMenu(const QPoint &pos, int row)
{
	if (row == -1)
	{
		return; // invalid
	}

	QPoint globalPos;

	if (m_isListLayout)
	{
		globalPos = m_gameList->mapToGlobal(pos);
	}
	else
	{
		globalPos = m_xgrid->mapToGlobal(pos);
	}

	GameInfo currGame = m_game_data[row].info;
	const QString serial = qstr(currGame.serial);

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
	myMenu.addSeparator();
	QAction* openGameFolder = myMenu.addAction(tr("&Open Install Folder"));
	QAction* openConfig = myMenu.addAction(tr("&Open Config Folder"));
	myMenu.addSeparator();
	QAction* checkCompat = myMenu.addAction(tr("&Check Game Compatibility"));
	QAction* downloadCompat = myMenu.addAction(tr("&Download Compatibility Database"));

	const std::string config_base_dir = fs::get_config_dir() + "data/" + m_game_data[row].info.serial;

	connect(boot, &QAction::triggered, [=]
	{
		LOG_NOTICE(LOADER, "Booting from gamelist per context menu...");
		Q_EMIT RequestBoot(currGame.path);
	});
	connect(configure, &QAction::triggered, [=]
	{
		settings_dialog dlg(xgui_settings, xemu_settings, 0, this, &currGame);
		connect(&dlg, &QDialog::accepted, [this]
		{
			Refresh(true, false);
		});
		dlg.exec();
	});
	connect(hide_serial, &QAction::triggered, [=](bool checked)
	{
		if (checked)
			m_hidden_list.insert(serial);
		else
			m_hidden_list.remove(serial);

		xgui_settings->SetValue(gui::gl_hidden_list, QStringList(m_hidden_list.toList()));
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
		QMessageBox* mb = new QMessageBox(QMessageBox::Question, tr("Confirm %1 Removal").arg(qstr(currGame.category)), tr("Permanently remove %1 from drive?").arg(qstr(currGame.name)), QMessageBox::Yes | QMessageBox::No, this);
		mb->setCheckBox(new QCheckBox(tr("Remove caches and custom config")));
		mb->deleteLater();
		if (mb->exec() == QMessageBox::Yes)
		{
			if (mb->checkBox()->isChecked())
			{
				DeleteShadersCache(config_base_dir);
				DeleteLLVMCache(config_base_dir);
				RemoveCustomConfiguration(config_base_dir);
			}
			fs::remove_all(currGame.path);
			m_game_data.erase(m_game_data.begin() + row);
			Refresh();
			LOG_SUCCESS(GENERAL, "Removed %s %s in %s", currGame.category, currGame.name, currGame.path);
		}
	});
	connect(removeConfig, &QAction::triggered, [=]()
	{
		if (RemoveCustomConfiguration(config_base_dir, true))
			Refresh(true, false);
	});
	connect(deleteShadersCache, &QAction::triggered, [=]()
	{
		DeleteShadersCache(config_base_dir, true);
	});
	connect(deleteLLVMCache, &QAction::triggered, [=]()
	{
		DeleteLLVMCache(config_base_dir, true);
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
		QString link = "https://rpcs3.net/compatibility?g=" + qstr(currGame.serial);
		QDesktopServices::openUrl(QUrl(link));
	});
	connect(downloadCompat, &QAction::triggered, [=]
	{
		m_game_compat->RequestCompatibility(true);
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
	removeConfig->setEnabled(m_game_data[row].hasCustomConfig);

	// remove delete options if necessary
	if (!fs::is_dir(config_base_dir))
	{
		deleteShadersCache->setEnabled(false);
		deleteLLVMCache->setEnabled(false);
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

	fs::dir root = fs::dir(base_dir);
	fs::dir_entry tmp;

	while (root.read(tmp))
	{
		if (!fs::is_dir(base_dir + "/" + tmp.name))
			continue;

		const std::string shader_cache_name = base_dir + "/" + tmp.name + "/shaders_cache";
		if (fs::is_dir(shader_cache_name))
			fs::remove_all(shader_cache_name, true);
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

	for (auto&& subdir : fs::dir{ base_dir })
	{
		if (!subdir.is_directory || subdir.name == "." || subdir.name == "..")
			continue;

		const std::string dir = base_dir + "/" + subdir.name;

		for (auto&& entry : fs::dir{ dir })
		{
			if (entry.name.size() >= 4 && entry.name.compare(entry.name.size() - 4, 4, ".obj", 4) == 0)
				fs::remove_file(dir + "/" + entry.name);
		}
	}

	LOG_SUCCESS(GENERAL, "Removed llvm cache in %s", base_dir);
	return true;
}

QPixmap game_list_frame::PaintedPixmap(const QImage& img, bool paintConfigIcon)
{
	QImage scaled = QImage(m_Icon_Size, QImage::Format_ARGB32);
	scaled.fill(m_Icon_Color);

	QPainter painter(&scaled);

	if (!img.isNull())
	{
		painter.drawImage(QPoint(0, 0), img.scaled(m_Icon_Size, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
	}

	if (paintConfigIcon && !m_isListLayout)
	{
		int width = m_Icon_Size.width() * 0.2;
		QPoint origin = QPoint(m_Icon_Size.width() - width, 0);
		painter.drawImage(origin, QImage(":/Icons/cog_gray.png").scaled(QSize(width, width), Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
	}

	painter.end();

	return QPixmap::fromImage(scaled);
}

void game_list_frame::ResizeIcons(const int& sliderPos)
{
	m_icon_size_index = sliderPos;
	m_Icon_Size = sizeFromSlider(sliderPos);

	RepaintIcons();
}

void game_list_frame::RepaintIcons(const bool& fromSettings)
{
	if (fromSettings)
	{
		if (xgui_settings->GetValue(gui::m_enableUIColors).toBool())
		{
			m_Icon_Color = xgui_settings->GetValue(gui::gl_iconColor).value<QColor>();
		}
		else
		{
			m_Icon_Color = gui::utils::get_label_color("gamelist_icon_background_color");
		}
	}

	for (auto& game : m_game_data)
	{
		game.pxmap = PaintedPixmap(game.icon, game.hasCustomConfig);
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

	xgui_settings->SetValue(gui::gl_listMode, isList);

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

	auto l_GetItem = [](const std::string& text, int sort_role = Qt::DisplayRole, int sort_index = 0)
	{
		custom_table_widget_item* curr = new custom_table_widget_item;
		curr->setFlags(curr->flags() & ~Qt::ItemIsEditable);
		curr->setText(qstr(text).simplified()); // simplified() forces single line text
		if (sort_role != Qt::DisplayRole)
		{
			curr->setData(sort_role, sort_index, true);
		}
		return curr;
	};

	int row = 0, index = -1;
	for (const auto& game : m_game_data)
	{
		index++;

		if (!IsEntryVisible(game))
			continue;

		// Icon
		custom_table_widget_item* icon_item = new custom_table_widget_item;
		icon_item->setFlags(icon_item->flags() & ~Qt::ItemIsEditable);
		icon_item->setData(Qt::DecorationRole, game.pxmap);
		icon_item->setData(Qt::UserRole, index, true);

		// Title
		custom_table_widget_item* title_item = l_GetItem(game.info.name);
		if (game.hasCustomConfig)
		{
			title_item->setIcon(QIcon(":/Icons/cog_black.png"));
		}

		// Compatibility
		custom_table_widget_item* compat_item = new custom_table_widget_item;
		compat_item->setFlags(compat_item->flags() & ~Qt::ItemIsEditable);
		compat_item->setText(game.compat.text + (game.compat.date.isEmpty() ? "" : " (" + game.compat.date + ")"));
		compat_item->setData(Qt::UserRole, game.compat.index, true);
		compat_item->setToolTip(game.compat.tooltip);
		if (!game.compat.color.isEmpty())
		{
			compat_item->setData(Qt::DecorationRole, compat_pixmap(game.compat.color));
		}

		m_gameList->setItem(row, gui::column_icon,       icon_item);
		m_gameList->setItem(row, gui::column_name,       title_item);
		m_gameList->setItem(row, gui::column_serial,     l_GetItem(game.info.serial));
		m_gameList->setItem(row, gui::column_firmware,   l_GetItem(game.info.fw));
		m_gameList->setItem(row, gui::column_version,    l_GetItem(game.info.app_ver));
		m_gameList->setItem(row, gui::column_category,   l_GetItem(game.info.category));
		m_gameList->setItem(row, gui::column_path,       l_GetItem(game.info.path));
		m_gameList->setItem(row, gui::column_resolution, l_GetItem(GetStringFromU32(game.info.resolution, resolution::mode, true)));
		m_gameList->setItem(row, gui::column_sound,      l_GetItem(GetStringFromU32(game.info.sound_format, sound::format, true)));
		m_gameList->setItem(row, gui::column_parental,   l_GetItem(GetStringFromU32(game.info.parental_lvl, parental::level), Qt::UserRole, game.info.parental_lvl));
		m_gameList->setItem(row, gui::column_compat,     compat_item);

		if (selected_item == game.info.icon_path)
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

	// Get list of matching apps and their index
	QList<QPair<GUI_GameInfo*, int>> matching_apps;

	for (uint i = 0; i < m_game_data.size(); i++)
	{
		if (IsEntryVisible(m_game_data[i]))
		{
			matching_apps.append(QPair<GUI_GameInfo*, int>(&m_game_data[i], i));
		}
	}

	int entries = matching_apps.count();

	// Edge cases!
	if (entries == 0)
	{ // For whatever reason, 0%x is division by zero.  Absolute nonsense by definition of modulus.  But, I'll acquiesce.
		return;
	}
	if (maxCols == 0)
	{
		maxCols = 1;
	}
	if (maxCols > entries)
	{
		maxCols = entries;
	}

	int needsExtraRow = (entries % maxCols) != 0;
	int maxRows = needsExtraRow + entries / maxCols;
	m_xgrid->setRowCount(maxRows);
	m_xgrid->setColumnCount(maxCols);

	for (const auto& app : matching_apps)
	{
		const QString title = qstr(app.first->info.name).simplified(); // simplified() forces single line text

		m_xgrid->addItem(app.first->pxmap, title, app.second, r, c);

		if (selected_item == app.first->info.icon_path)
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
			emptyItem->setData(Qt::UserRole, -1);
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
		int ind = item->data(Qt::UserRole).toInt();

		if (ind < m_game_data.size())
			selection = m_game_data.at(ind).info.icon_path;
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
