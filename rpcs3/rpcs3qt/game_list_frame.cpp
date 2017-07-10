#include "game_list_frame.h"

#include "settings_dialog.h"
#include "table_item_delegate.h"

#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Loader/PSF.h"
#include "Utilities/types.h"

#include <algorithm>
#include <memory>

#include <QDesktopServices>
#include <QDir>
#include <QHeaderView>
#include <QListView>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>
#include <QUrl>
#include <QLabel>

static const std::string m_class_name = "GameViewer";
inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }

game_list_frame::game_list_frame(std::shared_ptr<gui_settings> settings, const Render_Creator& r_Creator, QWidget *parent)
	: QDockWidget(tr("Game List"), parent), xgui_settings(settings), m_Render_Creator(r_Creator)
{
	m_isListLayout = xgui_settings->GetValue(GUI::gl_listMode).toBool();
	m_Icon_Size_Str = xgui_settings->GetValue(GUI::gl_iconSize).toString();
	m_Margin_Factor = xgui_settings->GetValue(GUI::gl_marginFactor).toReal();
	m_Text_Factor = xgui_settings->GetValue(GUI::gl_textFactor).toReal();
	m_showToolBar = xgui_settings->GetValue(GUI::gl_toolBarVisible).toBool();
	m_Icon_Color = xgui_settings->GetValue(GUI::gl_iconColor).value<QColor>();

	m_oldLayoutIsList = m_isListLayout;

	// get icon size from list
	int icon_size_index = 0;
	for (int i = 0; i < GUI::gl_icon_size.count(); i++)
	{
		if (GUI::gl_icon_size.at(i).first == m_Icon_Size_Str)
		{
			m_Icon_Size = GUI::gl_icon_size.at(i).second;
			icon_size_index = i;
			break;
		}
	}

	// Save factors for first setup
	xgui_settings->SetValue(GUI::gl_iconColor, m_Icon_Color);
	xgui_settings->SetValue(GUI::gl_marginFactor, m_Margin_Factor);
	xgui_settings->SetValue(GUI::gl_textFactor, m_Text_Factor);
	xgui_settings->SetValue(GUI::gl_toolBarVisible, m_showToolBar);

	m_Game_Dock = new QMainWindow(this);
	m_Game_Dock->setWindowFlags(Qt::Widget);

	// Set up toolbar
	m_Tool_Bar = new QToolBar(m_Game_Dock);
	m_Tool_Bar->setMovable(false);
	m_Tool_Bar->setVisible(m_showToolBar);
	m_Tool_Bar->setContextMenuPolicy(Qt::PreventContextMenu);

	// ToolBar Actions
	m_catActHDD = { new QAction(""), QIcon(":/Icons/hdd_blue.png"), QIcon(":/Icons/hdd_gray.png") };
	m_catActHDD.action->setIcon(xgui_settings->GetValue(GUI::cat_hdd_game).toBool() ? m_catActHDD.colored : m_catActHDD.gray);
	m_catActHDD.action->setToolTip(tr("Show HDD Categories"));

	m_catActDisc = { new QAction(""), QIcon(":/Icons/disc_blue.png"), QIcon(":/Icons/disc_gray.png") };
	m_catActDisc.action->setIcon(xgui_settings->GetValue(GUI::cat_disc_game).toBool() ? m_catActDisc.colored : m_catActDisc.gray);
	m_catActDisc.action->setToolTip(tr("Show Disc Categories"));

	m_catActHome = { new QAction(""), QIcon(":/Icons/home_blue.png"), QIcon(":/Icons/home_gray.png") };
	m_catActHome.action->setIcon(xgui_settings->GetValue(GUI::cat_home).toBool() ? m_catActHome.colored : m_catActHome.gray);
	m_catActHome.action->setToolTip(tr("Show Home Categories"));

	m_catActAudioVideo = { new QAction(""), QIcon(":/Icons/media_blue.png"), QIcon(":/Icons/media_gray.png") };
	m_catActAudioVideo.action->setIcon(xgui_settings->GetValue(GUI::cat_audio_video).toBool() ? m_catActAudioVideo.colored : m_catActAudioVideo.gray);
	m_catActAudioVideo.action->setToolTip(tr("Show Audio/Video Categories"));

	m_catActGameData = { new QAction(""), QIcon(":/Icons/data_blue.png"), QIcon(":/Icons/data_gray.png") };
	m_catActGameData.action->setIcon(xgui_settings->GetValue(GUI::cat_game_data).toBool() ? m_catActGameData.colored : m_catActGameData.gray);
	m_catActGameData.action->setToolTip(tr("Show GameData Categories"));

	m_catActUnknown = { new QAction(""), QIcon(":/Icons/unknown_blue.png"), QIcon(":/Icons/unknown_gray.png") };
	m_catActUnknown.action->setIcon(xgui_settings->GetValue(GUI::cat_unknown).toBool() ? m_catActUnknown.colored : m_catActUnknown.gray);
	m_catActUnknown.action->setToolTip(tr("Show Unknown Categories"));

	m_catActOther = { new QAction(""), QIcon(":/Icons/other_blue.png"), QIcon(":/Icons/other_gray.png") };
	m_catActOther.action->setIcon(xgui_settings->GetValue(GUI::cat_other).toBool() ? m_catActOther.colored : m_catActOther.gray);
	m_catActOther.action->setToolTip(tr("Show Other Categories"));

	m_categoryButtons = { m_catActHDD , m_catActDisc, m_catActHome, m_catActAudioVideo, m_catActGameData, m_catActUnknown, m_catActOther };

	m_categoryActs = new QActionGroup(m_Tool_Bar);
	m_categoryActs->addAction(m_catActHDD.action);
	m_categoryActs->addAction(m_catActDisc.action);
	m_categoryActs->addAction(m_catActHome.action);
	m_categoryActs->addAction(m_catActAudioVideo.action);
	m_categoryActs->addAction(m_catActGameData.action);
	m_categoryActs->addAction(m_catActUnknown.action);
	m_categoryActs->addAction(m_catActOther.action);
	m_categoryActs->setEnabled(m_isListLayout);

	m_modeActList = { new QAction(""), QIcon(":/Icons/list_blue.png"), QIcon(":/Icons/list_gray.png") };
	m_modeActList.action->setIcon(m_isListLayout ? m_modeActList.colored : m_modeActList.gray);
	m_modeActList.action->setToolTip(tr("Enable List Mode"));

	m_modeActGrid = { new QAction(""), QIcon(":/Icons/grid_blue.png"), QIcon(":/Icons/grid_gray.png") };
	m_modeActGrid.action->setIcon(m_isListLayout ? m_modeActGrid.gray : m_modeActGrid.colored);
	m_modeActGrid.action->setToolTip(tr("Enable Grid Mode"));

	m_modeActs = new QActionGroup(m_Tool_Bar);
	m_modeActs->addAction(m_modeActList.action);
	m_modeActs->addAction(m_modeActGrid.action);

	// Search Bar
	m_Search_Bar = new QLineEdit(m_Tool_Bar);
	m_Search_Bar->setPlaceholderText(tr("Search games ..."));
	connect(m_Search_Bar, &QLineEdit::textChanged, [this](const QString& text) {
		m_searchText = text;
		Refresh();
	});

	// Icon Size Slider
	m_Slider_Size = new QSlider(Qt::Horizontal , m_Tool_Bar);
	m_Slider_Size->setRange(0, GUI::gl_icon_size.size() - 1);
	m_Slider_Size->setSliderPosition(icon_size_index);
	m_Slider_Size->setFixedWidth(100);

	m_Tool_Bar->addWidget(m_Search_Bar);
	m_Tool_Bar->addWidget(new QLabel("       "));
	m_Tool_Bar->addSeparator();
	m_Tool_Bar->addWidget(new QLabel("       "));
	m_Tool_Bar->addActions(m_categoryActs->actions());
	m_Tool_Bar->addWidget(new QLabel("       "));
	m_Tool_Bar->addSeparator();
	m_Tool_Bar->addWidget(new QLabel(tr("       View Mode  ")));
	m_Tool_Bar->addAction(m_modeActList.action);
	m_Tool_Bar->addAction(m_modeActGrid.action);
	m_Tool_Bar->addWidget(new QLabel(tr("       ")));
	m_Tool_Bar->addSeparator();
	m_Tool_Bar->addWidget(new QLabel(tr("       Tiny  "))); // Can this be any easier?
	m_Tool_Bar->addWidget(m_Slider_Size);
	m_Tool_Bar->addWidget(new QLabel(tr("  Large       ")));

	m_Game_Dock->addToolBar(m_Tool_Bar);
	setWidget(m_Game_Dock);

	bool showText = (m_Icon_Size_Str != GUI::gl_icon_key_small && m_Icon_Size_Str != GUI::gl_icon_key_tiny);
	m_xgrid = new game_list_grid(m_Icon_Size, m_Icon_Color, m_Margin_Factor, m_Text_Factor, showText);

	gameList = new QTableWidget();
	gameList->setShowGrid(false);
	gameList->setItemDelegate(new table_item_delegate(this));
	gameList->setSelectionBehavior(QAbstractItemView::SelectRows);
	gameList->setSelectionMode(QAbstractItemView::SingleSelection);
	gameList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	gameList->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);	
	gameList->verticalHeader()->setMinimumSectionSize(m_Icon_Size.height());
	gameList->verticalHeader()->setMaximumSectionSize(m_Icon_Size.height());
	gameList->verticalHeader()->setVisible(false);
	gameList->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	gameList->horizontalHeader()->setHighlightSections(false);
	gameList->horizontalHeader()->setSortIndicatorShown(true);
	gameList->horizontalHeader()->setStretchLastSection(true);
	gameList->horizontalHeader()->setDefaultSectionSize(150);
	gameList->setContextMenuPolicy(Qt::CustomContextMenu);
	gameList->setAlternatingRowColors(true);

	gameList->setColumnCount(10);
	gameList->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("Icon")));
	gameList->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("Name")));
	gameList->setHorizontalHeaderItem(2, new QTableWidgetItem(tr("Serial")));
	gameList->setHorizontalHeaderItem(3, new QTableWidgetItem(tr("Firmware")));
	gameList->setHorizontalHeaderItem(4, new QTableWidgetItem(tr("Version")));
	gameList->setHorizontalHeaderItem(5, new QTableWidgetItem(tr("Category")));
	gameList->setHorizontalHeaderItem(6, new QTableWidgetItem(tr("Path")));
	gameList->setHorizontalHeaderItem(7, new QTableWidgetItem(tr("Supported Resolutions")));
	gameList->setHorizontalHeaderItem(8, new QTableWidgetItem(tr("Sound Formats")));
	gameList->setHorizontalHeaderItem(9, new QTableWidgetItem(tr("Parental Level")));

	// since this won't work somehow: gameList->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);	
	for (int i = 0; i < gameList->horizontalHeader()->count(); i++)
	{
		gameList->horizontalHeaderItem(i)->setTextAlignment(Qt::AlignLeft);
	}

	m_Central_Widget = new QStackedWidget(this);
	m_Central_Widget->addWidget(gameList);
	m_Central_Widget->addWidget(m_xgrid);
	m_Central_Widget->setCurrentWidget(m_isListLayout ? gameList : m_xgrid);

	m_Game_Dock->setCentralWidget(m_Central_Widget);

	// Actions
	showIconColAct = new QAction(tr("Show Icons"), this);
	showNameColAct = new QAction(tr("Show Names"), this);
	showSerialColAct = new QAction(tr("Show Serials"), this);
	showFWColAct = new QAction(tr("Show Firmwares"), this);
	showAppVersionColAct = new QAction(tr("Show Versions"), this);
	showCategoryColAct = new QAction(tr("Show Categories"), this);
	showPathColAct = new QAction(tr("Show Paths"), this);
	showResolutionColAct = new QAction(tr("Show Supported Resolutions"), this);
	showSoundFormatColAct = new QAction(tr("Show Sound Formats"), this);
	showParentalLevelColAct = new QAction(tr("Show Parental Levels"), this);

	columnActs = { showIconColAct, showNameColAct, showSerialColAct, showFWColAct, showAppVersionColAct, showCategoryColAct, showPathColAct,
		showResolutionColAct, showSoundFormatColAct, showParentalLevelColAct };

	// Events
	connect(gameList, &QTableWidget::customContextMenuRequested, this, &game_list_frame::ShowContextMenu);
	connect(gameList->horizontalHeader(), &QHeaderView::customContextMenuRequested, [=](const QPoint& pos) {
		QMenu* configure = new QMenu(this);
		configure->addActions(columnActs);
		configure->exec(mapToGlobal(pos));
	});
	connect(gameList, &QTableWidget::doubleClicked, this, &game_list_frame::doubleClickedSlot);
	connect(gameList->horizontalHeader(), &QHeaderView::sectionClicked, this, &game_list_frame::OnColClicked);

	connect(m_xgrid, &QTableWidget::doubleClicked, this, &game_list_frame::doubleClickedSlot);
	connect(m_xgrid, &QTableWidget::customContextMenuRequested, this, &game_list_frame::ShowContextMenu);

	connect(m_Slider_Size, &QSlider::valueChanged, [=](int value) { RequestIconSizeActSet(value); });

	connect(m_modeActs, &QActionGroup::triggered, [=](QAction* act) {
		RequestListModeActSet(act == m_modeActList.action);
		m_modeActList.action->setIcon(m_isListLayout ? m_modeActList.colored : m_modeActList.gray);
		m_modeActGrid.action->setIcon(m_isListLayout ? m_modeActGrid.gray : m_modeActGrid.colored);
	});

	connect(m_categoryActs, &QActionGroup::triggered, [=](QAction* act) {
		RequestCategoryActSet(m_categoryActs->actions().indexOf(act));
	});

	for (int col = 0; col < columnActs.count(); ++col)
	{
		columnActs[col]->setCheckable(true);

		auto l_CallBack = [this, col](bool val) {
			if (!val) // be sure to have at least one column left so you can call the context menu at all time
			{
				int c = 0;
				for (int i = 0; i < columnActs.count(); ++i)
				{
					if (xgui_settings->GetGamelistColVisibility(i)) { if (++c > 1) { break; } }
				}
				if (c < 2)
				{
					columnActs[col]->setChecked(true); // re-enable the checkbox if we don't change the actual state
					return;
				}
			}
			gameList->setColumnHidden(col, !val); // Negate because it's a set col hidden and we have menu say show.
			xgui_settings->SetGamelistColVisibility(col, val);
		};

		connect(columnActs[col], &QAction::triggered, l_CallBack);
	}
}

void game_list_frame::LoadSettings()
{
	QByteArray state = xgui_settings->GetValue(GUI::gl_state).toByteArray();

	if (state.isEmpty())
	{ // If no settings exist, go to default.
		if (gameList->rowCount() > 0)
		{
			gameList->verticalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
			gameList->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
			gameList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
		}
	}
	else
	{
		gameList->horizontalHeader()->restoreState(state);
	}

	for (int col = 0; col < columnActs.count(); ++col)
	{
		bool vis = xgui_settings->GetGamelistColVisibility(col);
		columnActs[col]->setChecked(vis);
		gameList->setColumnHidden(col, !vis);
	}

	gameList->horizontalHeader()->restoreState(gameList->horizontalHeader()->saveState());
	gameList->horizontalHeader()->stretchLastSection();

	m_colSortOrder = xgui_settings->GetValue(GUI::gl_sortAsc).toBool() ? Qt::AscendingOrder : Qt::DescendingOrder;

	m_sortColumn = xgui_settings->GetValue(GUI::gl_sortCol).toInt();

	m_Icon_Color = xgui_settings->GetValue(GUI::gl_iconColor).value<QColor>();

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

	xgui_settings->SetValue(GUI::gl_sortAsc, m_colSortOrder == Qt::AscendingOrder);
	xgui_settings->SetValue(GUI::gl_sortCol, col);

	gameList->sortByColumn(m_sortColumn, m_colSortOrder);
}

// Filter for Categories
void game_list_frame::FilterData()
{
	for (int i = 0; i < gameList->rowCount(); ++i)
	{
		bool match = false;
		const QString category = qstr(m_game_data[i].info.category);
		for (const auto& filter : m_categoryFilters)
		{
			if (category.contains(filter))
			{
				match = true;
				break;
			}
		}
		gameList->setRowHidden(i, !match || !SearchMatchesApp(m_game_data[i].info.name, m_game_data[i].info.serial));
	}
}

void game_list_frame::Refresh(bool fromDrive)
{
	if (fromDrive)
	{
		// Load PSF

		m_game_data.clear();

		const std::string& game_path = Emu.GetGameDir();

		for (const auto& entry : fs::dir(Emu.GetGameDir()))
		{
			if (!entry.is_directory)
			{
				continue;
			}

			const std::string& dir = game_path + entry.name;
			const std::string& sfb = dir + "/PS3_DISC.SFB";
			const std::string& sfo = dir + (fs::is_file(sfb) ? "/PS3_GAME/PARAM.SFO" : "/PARAM.SFO");

			const fs::file sfo_file(sfo);
			if (!sfo_file)
			{
				continue;
			}

			const auto& psf = psf::load_object(sfo_file);

			GameInfo game;
			game.root         = entry.name;
			game.serial       = psf::get_string(psf, "TITLE_ID", "");
			game.name         = psf::get_string(psf, "TITLE", sstr(category::unknown));
			game.app_ver      = psf::get_string(psf, "APP_VER", sstr(category::unknown));
			game.category     = psf::get_string(psf, "CATEGORY", sstr(category::unknown));
			game.fw           = psf::get_string(psf, "PS3_SYSTEM_VER", sstr(category::unknown));
			game.parental_lvl = psf::get_integer(psf, "PARENTAL_LEVEL");
			game.resolution   = psf::get_integer(psf, "RESOLUTION");
			game.sound_format = psf::get_integer(psf, "SOUND_FORMAT");

			bool bootable = false;
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
				bootable = true;
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
			QPixmap pxmap;

			if (!game.icon_path.empty() && img.load(qstr(game.icon_path)))
			{
				QImage scaled = QImage(m_Icon_Size, QImage::Format_ARGB32);
				scaled.fill(m_Icon_Color);
				QPainter painter(&scaled);
				painter.drawImage(QPoint(0,0), img.scaled(m_Icon_Size, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
				painter.end();
				pxmap = QPixmap::fromImage(scaled);
			}
			else
			{
				img = QImage(m_Icon_Size, QImage::Format_ARGB32);
				QString abspath = QDir(qstr(game.icon_path)).absolutePath();
				LOG_ERROR(GENERAL, "Could not load image from path %s", sstr(abspath));
				img.fill(QColor(0, 0, 0, 0));
				pxmap = QPixmap::fromImage(img);
			}

			m_game_data.push_back({ game, img, pxmap, bootable });
		}

		auto op = [](const GUI_GameInfo& game1, const GUI_GameInfo& game2) {
			return game1.info.name < game2.info.name;
		};

		// Sort by name at the very least.
		std::sort(m_game_data.begin(), m_game_data.end(), op);
	}

	// Fill Game List / Game Grid

	if (m_isListLayout)
	{
		int row = PopulateGameList();
		FilterData();
		gameList->selectRow(row);
		gameList->sortByColumn(m_sortColumn, m_colSortOrder);
		gameList->verticalHeader()->setMinimumSectionSize(m_Icon_Size.height());
		gameList->verticalHeader()->setMaximumSectionSize(m_Icon_Size.height());
		gameList->resizeRowsToContents();
		gameList->resizeColumnToContents(0);
		gameList->scrollTo(gameList->currentIndex());
	}
	else
	{
		if (m_Icon_Size.width() > 0 && m_Icon_Size.height() > 0)
		{
			m_games_per_row = width() / (m_Icon_Size.width() + m_Icon_Size.width() * m_xgrid->getMarginFactor() * 2);
		}
		else
		{
			m_games_per_row = 0;
		}

		PopulateGameGrid(m_games_per_row, m_Icon_Size, m_Icon_Color);
		connect(m_xgrid, &QTableWidget::doubleClicked, this, &game_list_frame::doubleClickedSlot);
		connect(m_xgrid, &QTableWidget::customContextMenuRequested, this, &game_list_frame::ShowContextMenu);
		m_Central_Widget->addWidget(m_xgrid);
		m_Central_Widget->setCurrentWidget(m_xgrid);
		m_xgrid->scrollTo(m_xgrid->currentIndex());
	}
}

void game_list_frame::ToggleCategoryFilter(const QStringList& categories, bool show)
{
	if (show) { m_categoryFilters.append(categories); }
	else { for (const auto& cat : categories) m_categoryFilters.removeAll(cat); }
	Refresh();
}

void game_list_frame::SaveSettings()
{
	for (int col = 0; col < columnActs.count(); ++col)
	{
		xgui_settings->SetGamelistColVisibility(col, columnActs[col]->isChecked());
	}
	xgui_settings->SetValue(GUI::gl_sortCol, m_sortColumn);
	xgui_settings->SetValue(GUI::gl_sortAsc, m_colSortOrder == Qt::AscendingOrder);

	xgui_settings->SetValue(GUI::gl_state, gameList->horizontalHeader()->saveState());
}

static void open_dir(const std::string& spath)
{
	fs::create_dir(spath);
	QString path = qstr(spath);
	QProcess* process = new QProcess();

#ifdef _WIN32
	std::string command = "explorer";
	std::replace(path.begin(), path.end(), '/', '\\');
	process->start("explorer", QStringList() << path);
#elif __APPLE__
	process->start("open", QStringList() << path);
#else
	process->start("xdg-open", QStringList() << path);
#endif
}

void game_list_frame::doubleClickedSlot(const QModelIndex& index)
{
	int i;

	if (m_isListLayout)
	{
		i = gameList->item(index.row(), 0)->data(Qt::UserRole).toInt();
	}
	else
	{
		i = m_xgrid->item(index.row(), index.column())->data(Qt::ItemDataRole::UserRole).toInt();
	}
	
	// enable boot for bootable categories only
	if (m_game_data[i].bootable)
	{
		const std::string& path = Emu.GetGameDir() + m_game_data[i].info.root;
		RequestIconPathSet(path);
	
		Emu.Stop();
	
		if (!Emu.BootGame(path))
		{
			LOG_ERROR(LOADER, "Failed to boot /dev_hdd0/game/%s", m_game_data[i].info.root);
		}
		else
		{
			LOG_SUCCESS(LOADER, "Boot from gamelist per doubleclick: done");
			RequestAddRecentGame(q_string_pair(qstr(path), qstr("[" + m_game_data[i].info.serial + "] " + m_game_data[i].info.name)));
		}
	}
	else
	{
		open_dir(Emu.GetGameDir() + m_game_data[i].info.root);
	}
}

void game_list_frame::ShowContextMenu(const QPoint &pos)
{
	int index;

	if (m_isListLayout)
	{
		int row = gameList->indexAt(pos).row();
		QTableWidgetItem* item = gameList->item(row, 0);
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
		if (index == -1) return; // empty item shouldn't have context menu
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
		globalPos = gameList->mapToGlobal(pos);
	}
	else
	{
		globalPos = m_xgrid->mapToGlobal(pos);
	}

	QMenu myMenu;

	// Make Actions
	QAction* boot = myMenu.addAction(tr("&Boot"));
	QFont f = boot->font();
	f.setBold(true);
	boot->setFont(f);
	QAction* configure = myMenu.addAction(tr("&Configure"));
	myMenu.addSeparator();
	QAction* removeGame = myMenu.addAction(tr("&Remove"));
	QAction* removeConfig = myMenu.addAction(tr("&Remove Custom Configuration"));
	myMenu.addSeparator();
	QAction* openGameFolder = myMenu.addAction(tr("&Open Install Folder"));
	QAction* openConfig = myMenu.addAction(tr("&Open Config Folder"));
	myMenu.addSeparator();
	QAction* checkCompat = myMenu.addAction(tr("&Check Game Compatibility"));

	connect(boot, &QAction::triggered, [=]() {Boot(row); });
	connect(configure, &QAction::triggered, [=]() {
		settings_dialog (xgui_settings, m_Render_Creator, 0, this, &m_game_data[row].info).exec();
	});
	connect(removeGame, &QAction::triggered, [=]() {
		if (QMessageBox::question(this, tr("Confirm Delete"), tr("Permanently delete files?")) == QMessageBox::Yes)
		{
			fs::remove_all(Emu.GetGameDir() + m_game_data[row].info.root);
			m_game_data.erase(m_game_data.begin() + row);
			Refresh();
		}
	});
	connect(removeConfig, &QAction::triggered, [=]() {RemoveCustomConfiguration(row); });
	connect(openGameFolder, &QAction::triggered, [=]() {open_dir(Emu.GetGameDir() + m_game_data[row].info.root); });
	connect(openConfig, &QAction::triggered, [=]() {open_dir(fs::get_config_dir() + "data/" + m_game_data[row].info.serial); });
	connect(checkCompat, &QAction::triggered, [=]() {
		QString serial = qstr(m_game_data[row].info.serial);
		QString link = "https://rpcs3.net/compatibility?g=" + serial;
		QDesktopServices::openUrl(QUrl(link));
	});

	//Disable options depending on software category
	QString category = qstr(m_game_data[row].info.category);

	if (category == category::disc_Game)
	{
		removeGame->setEnabled(false);
	}
	else if (!m_game_data[row].bootable)
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

	myMenu.exec(globalPos);
}

void game_list_frame::Boot(int row)
{
	const std::string& path = Emu.GetGameDir() + m_game_data[row].info.root;
	RequestIconPathSet(path);

	Emu.Stop();

	if (!Emu.BootGame(path))
	{
		QMessageBox::warning(this, tr("Warning!"), tr("Failed to boot ") + qstr(m_game_data[row].info.root));
		LOG_ERROR(LOADER, "Failed to boot /dev_hdd0/game/%s", m_game_data[row].info.root);
	}
	else
	{
		LOG_SUCCESS(LOADER, "Boot from gamelist per Boot: done");
		RequestAddRecentGame(q_string_pair(qstr(path), qstr("[" + m_game_data[row].info.serial + "] " + m_game_data[row].info.name)));
	}
}

void game_list_frame::RemoveCustomConfiguration(int row)
{
	const std::string config_path = fs::get_config_dir() + "data/" + m_game_data[row].info.serial + "/config.yml";

	if (fs::is_file(config_path))
	{
		if (QMessageBox::question(this, tr("Confirm Delete"), tr("Delete custom game configuration?")) == QMessageBox::Yes)
		{
			if (fs::remove_file(config_path))
			{
				LOG_SUCCESS(GENERAL, "Removed configuration file: %s", config_path);
			}
			else
			{
				QMessageBox::warning(this, tr("Warning!"), tr("Failed to delete configuration file!"));
				LOG_FATAL(GENERAL, "Failed to delete configuration file: %s\nError: %s", config_path, fs::g_tls_error);
			}
		}
	}
	else
	{
		QMessageBox::warning(this, tr("Warning!"), tr("No custom configuration found!"));
		LOG_ERROR(GENERAL, "Configuration file not found: %s", config_path);
	}
}

void game_list_frame::ResizeIcons(const QString& sizeStr, const QSize& size, const int& index)
{
	m_Icon_Size_Str = sizeStr;
	m_Icon_Size = size;

	if (m_Slider_Size->value() != index)
	{
		m_Slider_Size->setSliderPosition(index);
	}

	xgui_settings->SetValue(GUI::gl_iconSize, m_Icon_Size_Str);

	for (size_t i = 0; i < m_game_data.size(); i++)
	{
		QImage scaled = QImage(m_Icon_Size, QImage::Format_ARGB32);
		scaled.fill(m_Icon_Color);
		QPainter painter(&scaled);
		painter.drawImage(QPoint(0, 0), m_game_data[i].icon.scaled(m_Icon_Size, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
		painter.end();
		m_game_data[i].pxmap = QPixmap::fromImage(scaled);
	}

	Refresh();
}

void game_list_frame::SetListMode(const bool& isList)
{
	m_oldLayoutIsList = m_isListLayout;
	m_isListLayout = isList;

	xgui_settings->SetValue(GUI::gl_listMode, isList);

	m_categoryActs->setEnabled(isList);
	m_modeActList.action->setIcon(m_isListLayout ? m_modeActList.colored : m_modeActList.gray);
	m_modeActGrid.action->setIcon(m_isListLayout ? m_modeActGrid.gray : m_modeActGrid.colored);

	Refresh();

	m_Central_Widget->setCurrentWidget(m_isListLayout ? gameList : m_xgrid);
}

void game_list_frame::SetToolBarVisible(const bool& showToolBar)
{
	m_showToolBar = showToolBar;
	m_Tool_Bar->setVisible(showToolBar);
	xgui_settings->SetValue(GUI::gl_toolBarVisible, showToolBar);
}
bool game_list_frame::GetToolBarVisible()
{
	return m_showToolBar;
}

void game_list_frame::SetCategoryActIcon(const int& id, const bool& active)
{
	m_categoryButtons.at(id).action->setIcon(active ? m_categoryButtons.at(id).colored : m_categoryButtons.at(id).gray);
}

void game_list_frame::SetSearchText(const QString& text)
{
	m_searchText = text;
	Refresh();
}

void game_list_frame::closeEvent(QCloseEvent *event)
{
	QDockWidget::closeEvent(event);
	game_list_frameClosed();
}

void game_list_frame::resizeEvent(QResizeEvent *event)
{
	if (!m_isListLayout)
	{
		Refresh();
	}
	QDockWidget::resizeEvent(event);
}

/**
 Cleans and readds entries to table widget in UI.
*/
int game_list_frame::PopulateGameList()
{
	int result = -1;

	std::string selected_item = CurrentSelectionIconPath();

	// Hack to delete everything without removing the headers.
	gameList->setRowCount(0);

	gameList->setRowCount(m_game_data.size());

	auto l_GetItem = [](const std::string& text)
	{
		QTableWidgetItem* curr = new QTableWidgetItem;
		curr->setFlags(curr->flags() & ~Qt::ItemIsEditable);
		curr->setText(qstr(text));
		return curr;
	};

	int row = 0;
	for (const GUI_GameInfo& game : m_game_data)
	{
		// Icon
		QTableWidgetItem* iconItem = new QTableWidgetItem;
		iconItem->setFlags(iconItem->flags() & ~Qt::ItemIsEditable);
		iconItem->setData(Qt::DecorationRole, game.pxmap);
		iconItem->setData(Qt::UserRole, row);
		gameList->setItem(row, 0, iconItem);

		gameList->setItem(row, 1, l_GetItem(game.info.name));
		gameList->setItem(row, 2, l_GetItem(game.info.serial));
		gameList->setItem(row, 3, l_GetItem(game.info.fw));
		gameList->setItem(row, 4, l_GetItem(game.info.app_ver));
		gameList->setItem(row, 5, l_GetItem(game.info.category));
		gameList->setItem(row, 6, l_GetItem(game.info.root));
		gameList->setItem(row, 7, l_GetItem(GetStringFromU32(game.info.resolution, resolution::mode, true)));
		gameList->setItem(row, 8, l_GetItem(GetStringFromU32(game.info.sound_format, sound::format, true)));
		gameList->setItem(row, 9, l_GetItem(GetStringFromU32(game.info.parental_lvl, parental::level)));

		if (selected_item == game.info.icon_path) result = row;

		row++;
	}

	return result;
}

void game_list_frame::PopulateGameGrid(uint maxCols, const QSize& image_size, const QColor& image_color)
{
	uint r = 0;
	uint c = 0;

	std::string selected_item = CurrentSelectionIconPath();

	delete m_xgrid;

	bool showText = m_Icon_Size_Str != GUI::gl_icon_key_small && m_Icon_Size_Str != GUI::gl_icon_key_tiny;

	if (m_Icon_Size_Str == GUI::gl_icon_key_medium)
	{
		m_xgrid = new game_list_grid(image_size, image_color, m_Margin_Factor, m_Text_Factor * 2, showText);
	}
	else
	{
		m_xgrid = new game_list_grid(image_size, image_color, m_Margin_Factor, m_Text_Factor, showText);
	}

	// Get number of things that'll be in grid and precompute grid size.
	int entries = 0;
	for (const GUI_GameInfo& game : m_game_data)
	{
		if (qstr(game.info.category) == category::disc_Game || qstr(game.info.category) == category::hdd_Game)
		{
			if (SearchMatchesApp(game.info.name, game.info.serial) == false)
			{
				continue;
			}
			++entries;
		}
	}

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

	for (uint i = 0; i < m_game_data.size(); i++)
	{
		if (SearchMatchesApp(m_game_data[i].info.name, m_game_data[i].info.serial) == false)
		{
			continue;
		}

		QString category = qstr(m_game_data[i].info.category);

		if (category == category::hdd_Game || category == category::disc_Game)
		{
			m_xgrid->addItem(m_game_data[i].pxmap, qstr(m_game_data[i].info.name), i, r, c);

			if (selected_item == m_game_data[i].info.icon_path) m_xgrid->setCurrentItem(m_xgrid->item(r, c));;

			if (++c >= maxCols)
			{
				c = 0;
				r++;
			}
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
}

/**
* Returns false if the game should be hidden because it doesn't match search term in toolbar.
*/
bool game_list_frame::SearchMatchesApp(const std::string& name, const std::string& serial)
{
	if (!m_searchText.isEmpty())
	{
		QString searchText = m_searchText.toLower();
		return qstr(name).toLower().contains(searchText) || qstr(serial).toLower().contains(searchText);
	}
	return true;
}

std::string game_list_frame::CurrentSelectionIconPath()
{
	std::string selection = "";

	if (m_oldLayoutIsList && gameList->currentRow() >= 0)
	{
		selection = m_game_data.at(gameList->item(gameList->currentRow(), 0)->data(Qt::UserRole).toInt()).info.icon_path;
	}
	else if (!m_oldLayoutIsList && m_xgrid->currentItem() != nullptr)
	{
		selection = m_game_data.at(m_xgrid->currentItem()->data(Qt::UserRole).toInt()).info.icon_path;
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
