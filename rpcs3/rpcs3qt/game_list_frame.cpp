#include "game_list_frame.h"

#include "settings_dialog.h"
#include "table_item_delegate.h"

#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Loader/PSF.h"
#include "Utilities/types.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <set>

#include <QDesktopServices>
#include <QDir>
#include <QHeaderView>
#include <QListView>
#include <QMenuBar>
#include <QMessageBox>
#include <QTimer>
#include <QUrl>
#include <QLabel>
#include <QScrollBar>

inline std::string sstr(const QString& _in) { return _in.toStdString(); }
inline QSize sizeFromSlider(const int& pos) { return gui::gl_icon_size_min + (gui::gl_icon_size_max - gui::gl_icon_size_min) * (pos / (float)gui::gl_max_slider_pos); }

game_list_frame::game_list_frame(std::shared_ptr<gui_settings> guiSettings, std::shared_ptr<emu_settings> emuSettings, QWidget *parent)
	: QDockWidget(tr("Game List"), parent), xgui_settings(guiSettings), xemu_settings(emuSettings)
{
	m_isListLayout    = xgui_settings->GetValue(gui::gl_listMode).toBool();
	m_icon_size_index = xgui_settings->GetValue(gui::gl_iconSize).toInt();
	m_Margin_Factor   = xgui_settings->GetValue(gui::gl_marginFactor).toReal();
	m_Text_Factor     = xgui_settings->GetValue(gui::gl_textFactor).toReal();
	m_showToolBar     = xgui_settings->GetValue(gui::gl_toolBarVisible).toBool();
	m_Icon_Color      = xgui_settings->GetValue(gui::gl_iconColor).value<QColor>();
	m_colSortOrder    = xgui_settings->GetValue(gui::gl_sortAsc).toBool() ? Qt::AscendingOrder : Qt::DescendingOrder;
	m_sortColumn      = xgui_settings->GetValue(gui::gl_sortCol).toInt();

	m_oldLayoutIsList = m_isListLayout;

	// Save factors for first setup
	xgui_settings->SetValue(gui::gl_iconColor, m_Icon_Color);
	xgui_settings->SetValue(gui::gl_marginFactor, m_Margin_Factor);
	xgui_settings->SetValue(gui::gl_textFactor, m_Text_Factor);
	xgui_settings->SetValue(gui::gl_toolBarVisible, m_showToolBar);

	m_Game_Dock = new QMainWindow(this);
	m_Game_Dock->setWindowFlags(Qt::Widget);

	// Set up toolbar
	m_Tool_Bar = new QToolBar(m_Game_Dock);
	m_Tool_Bar->setMovable(false);
	m_Tool_Bar->setVisible(m_showToolBar);
	m_Tool_Bar->setContextMenuPolicy(Qt::PreventContextMenu);

	// ToolBar Actions
	m_catActHDD = { new QAction(""), QIcon(":/Icons/hdd_blue.png"), QIcon(":/Icons/hdd_gray.png"), xgui_settings->GetValue(gui::cat_hdd_game).toBool() };
	m_catActHDD.action->setToolTip(tr("Show HDD Categories"));

	m_catActDisc = { new QAction(""), QIcon(":/Icons/disc_blue.png"), QIcon(":/Icons/disc_gray.png"), xgui_settings->GetValue(gui::cat_disc_game).toBool() };
	m_catActDisc.action->setToolTip(tr("Show Disc Categories"));

	m_catActHome = { new QAction(""), QIcon(":/Icons/home_blue.png"), QIcon(":/Icons/home_gray.png"), xgui_settings->GetValue(gui::cat_home).toBool() };
	m_catActHome.action->setToolTip(tr("Show Home Categories"));

	m_catActAudioVideo = { new QAction(""), QIcon(":/Icons/media_blue.png"), QIcon(":/Icons/media_gray.png"), xgui_settings->GetValue(gui::cat_audio_video).toBool() };
	m_catActAudioVideo.action->setToolTip(tr("Show Audio/Video Categories"));

	m_catActGameData = { new QAction(""), QIcon(":/Icons/data_blue.png"), QIcon(":/Icons/data_gray.png"), xgui_settings->GetValue(gui::cat_game_data).toBool() };
	m_catActGameData.action->setToolTip(tr("Show GameData Categories"));

	m_catActUnknown = { new QAction(""), QIcon(":/Icons/unknown_blue.png"), QIcon(":/Icons/unknown_gray.png"), xgui_settings->GetValue(gui::cat_unknown).toBool() };
	m_catActUnknown.action->setToolTip(tr("Show Unknown Categories"));

	m_catActOther = { new QAction(""), QIcon(":/Icons/other_blue.png"), QIcon(":/Icons/other_gray.png"), xgui_settings->GetValue(gui::cat_other).toBool() };
	m_catActOther.action->setToolTip(tr("Show Other Categories"));

	m_categoryButtons = { &m_catActHDD , &m_catActDisc, &m_catActHome, &m_catActAudioVideo, &m_catActGameData, &m_catActUnknown, &m_catActOther };

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
	m_modeActList.action->setToolTip(tr("Enable List Mode"));

	m_modeActGrid = { new QAction(""), QIcon(":/Icons/grid_blue.png"), QIcon(":/Icons/grid_gray.png") };
	m_modeActGrid.action->setToolTip(tr("Enable Grid Mode"));

	m_modeActs = new QActionGroup(m_Tool_Bar);
	m_modeActs->addAction(m_modeActList.action);
	m_modeActs->addAction(m_modeActGrid.action);

	// Search Bar
	m_Search_Bar = new QLineEdit(m_Tool_Bar);
	m_Search_Bar->setObjectName("tb_searchbar"); // used in default stylesheet
	m_Search_Bar->setPlaceholderText(tr("Search games ..."));
	m_Search_Bar->setMinimumWidth(m_Tool_Bar->height() * 5);
	m_Search_Bar->setFrame(false);

	// Icon Size Slider
	m_Slider_Size = new QSlider(Qt::Horizontal , m_Tool_Bar);
	m_Slider_Size->setRange(0, gui::gl_max_slider_pos);
	m_Slider_Size->setSliderPosition(m_icon_size_index);
	m_Slider_Size->setFixedWidth(m_Tool_Bar->height() * 3);

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

	RepaintToolBarIcons();

	bool showText = m_icon_size_index < gui::gl_max_slider_pos;
	m_Icon_Size = sizeFromSlider(m_icon_size_index);
	m_xgrid = new game_list_grid(m_Icon_Size, m_Icon_Color, m_Margin_Factor, m_Text_Factor, showText);

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
	m_gameList->verticalHeader()->setMinimumSectionSize(m_Icon_Size.height());
	m_gameList->verticalHeader()->setMaximumSectionSize(m_Icon_Size.height());
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

	connect(m_Search_Bar, &QLineEdit::textChanged, this, &game_list_frame::SetSearchText);

	connect(m_Slider_Size, &QSlider::valueChanged, this, &game_list_frame::RequestIconSizeActSet);
	connect(m_Slider_Size, &QSlider::sliderReleased, this, [&]
	{
		xgui_settings->SetValue(gui::gl_iconSize, m_Slider_Size->value());
	});
	connect(m_Slider_Size, &QSlider::actionTriggered, [&](int action)
	{
		if (action != QAbstractSlider::SliderNoAction && action != QAbstractSlider::SliderMove)
		{	// we only want to save on mouseclicks or slider release (the other connect handles this)
			Q_EMIT RequestSaveSliderPos(true); // actionTriggered happens before the value was changed
		}
	});

	connect(m_modeActs, &QActionGroup::triggered, [=](QAction* act)
	{
		Q_EMIT RequestListModeActSet(act == m_modeActList.action);
		m_modeActList.action->setIcon(m_isListLayout ? m_modeActList.colored : m_modeActList.gray);
		m_modeActGrid.action->setIcon(m_isListLayout ? m_modeActGrid.gray : m_modeActGrid.colored);
	});

	connect(m_categoryActs, &QActionGroup::triggered, [=](QAction* act)
	{
		Q_EMIT RequestCategoryActSet(m_categoryActs->actions().indexOf(act));
	});

	for (int col = 0; col < m_columnActs.count(); ++col)
	{
		m_columnActs[col]->setCheckable(true);

		connect(m_columnActs[col], &QAction::triggered, [this, col](bool val)
		{
			if (!val) // be sure to have at least one column left so you can call the context menu at all time
			{
				int c = 0;
				for (int i = 0; i < m_columnActs.count(); ++i)
				{
					if (xgui_settings->GetGamelistColVisibility(i)) { if (++c > 1) { break; } }
				}
				if (c < 2)
				{
					m_columnActs[col]->setChecked(true); // re-enable the checkbox if we don't change the actual state
					return;
				}
			}
			m_gameList->setColumnHidden(col, !val); // Negate because it's a set col hidden and we have menu say show.
			xgui_settings->SetGamelistColVisibility(col, val);
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

	m_Icon_Color = xgui_settings->GetValue(gui::gl_iconColor).value<QColor>();

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

// Filter for Categories
void game_list_frame::FilterData()
{
	for (auto& game : m_game_data)
	{
		bool match = false;
		const QString category = qstr(game.info.category);
		for (const auto& filter : m_categoryFilters)
		{
			if (category.contains(filter))
			{
				match = true;
				break;
			}
		}
		game.isVisible = match && SearchMatchesApp(game.info.name, game.info.serial);
	}
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

		// std::set is used to remove duplicates from the list
		for (const auto& dir : std::set<std::string>(std::make_move_iterator(path_list.begin()), std::make_move_iterator(path_list.end())))
		{
			const std::string sfb = dir + "/PS3_DISC.SFB";
			const std::string sfo = dir + (fs::is_file(sfb) ? "/PS3_GAME/PARAM.SFO" : "/PARAM.SFO");

			const fs::file sfo_file(sfo);
			if (!sfo_file)
			{
				continue;
			}

			const auto& psf = psf::load_object(sfo_file);

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

			if (game.icon_path.empty() || !img.load(qstr(game.icon_path)))
			{
				LOG_WARNING(GENERAL, "Could not load image from path %s", sstr(QDir(qstr(game.icon_path)).absolutePath()));
			}

			bool hasCustomConfig = fs::is_file(fs::get_config_dir() + "data/" + game.serial + "/config.yml");

			QPixmap pxmap = PaintedPixmap(img, hasCustomConfig);

			m_game_data.push_back({ game, m_game_compat->GetCompatibility(game.serial), img, pxmap, true, bootable, hasCustomConfig });
		}

		auto op = [](const GUI_GameInfo& game1, const GUI_GameInfo& game2)
		{
			return game1.info.name < game2.info.name;
		};

		// Sort by name at the very least.
		std::sort(m_game_data.begin(), m_game_data.end(), op);
	}

	// Fill Game List / Game Grid

	if (m_isListLayout)
	{
		int scroll_position = m_gameList->verticalScrollBar()->value();
		FilterData();
		int row = PopulateGameList();
		m_gameList->selectRow(row);
		SortGameList();

		if (scrollAfter)
		{
			m_gameList->scrollTo(m_gameList->currentIndex(), QAbstractItemView::PositionAtCenter);
		}
		else
		{
			m_gameList->verticalScrollBar()->setValue(std::min(m_gameList->verticalScrollBar()->maximum(), scroll_position));
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

		if (scrollAfter)
		{
			m_xgrid->scrollTo(m_xgrid->currentIndex());
		}
		else
		{
			m_xgrid->verticalScrollBar()->setValue(std::min(m_xgrid->verticalScrollBar()->maximum(), scroll_position));
		}
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
	
	if (1)
	{
		if (Boot(m_game_data[i].info))
		{
			LOG_SUCCESS(LOADER, "Boot from gamelist per doubleclick: done");
		}
	}
	else
	{
		open_dir(m_game_data[i].info.path);
	}
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
		globalPos = m_gameList->mapToGlobal(pos);
	}
	else
	{
		globalPos = m_xgrid->mapToGlobal(pos);
	}

	GameInfo currGame = m_game_data[row].info;

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
	QAction* deleteShadersCache = myMenu.addAction(tr("&Delete Shaders Cache"));
	QAction* deleteLLVMCache = myMenu.addAction(tr("&Delete LLVM Cache"));
	myMenu.addSeparator();
	QAction* openGameFolder = myMenu.addAction(tr("&Open Install Folder"));
	QAction* openConfig = myMenu.addAction(tr("&Open Config Folder"));
	myMenu.addSeparator();
	QAction* checkCompat = myMenu.addAction(tr("&Check Game Compatibility"));
	QAction* downloadCompat = myMenu.addAction(tr("&Download Compatibility Database"));

	connect(boot, &QAction::triggered, [=]
	{
		if (Boot(currGame))
		{
			LOG_SUCCESS(LOADER, "Boot from gamelist per Boot: done");
		}
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
	connect(removeGame, &QAction::triggered, [=]
	{
		if (QMessageBox::question(this, tr("Confirm Delete"), tr("Permanently delete files?")) == QMessageBox::Yes)
		{
			fs::remove_all(currGame.path);
			m_game_data.erase(m_game_data.begin() + row);
			Refresh();
		}
	});
	connect(removeConfig, &QAction::triggered, [=]()
	{
		RemoveCustomConfiguration(row);
		Refresh(true, false);
	});
	connect(deleteShadersCache, &QAction::triggered, [=]()
	{
		DeleteShadersCache(row);
	});

	connect(deleteLLVMCache, &QAction::triggered, [=]()
	{
		if (QMessageBox::question(this, tr("Confirm Delete"), tr("Delete LLVM cache?")) == QMessageBox::Yes)
		{
			const std::string config_base_dir = fs::get_config_dir() + "data/" + m_game_data[row].info.serial;

			for (auto&& subdir : fs::dir{config_base_dir})
			{
				if (!subdir.is_directory || subdir.name == "." || subdir.name == "..")
				{
					continue;
				}

				for (auto&& entry : fs::dir{config_base_dir + "/" + subdir.name})
				{
					if (entry.name.size() >= 4 && entry.name.compare(entry.name.size() - 4, 4, ".obj", 4) == 0)
					{
						fs::remove_file(config_base_dir + "/" + subdir.name + "/" + entry.name);
					}
				}
			}
		}
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

	myMenu.exec(globalPos);
}

bool game_list_frame::Boot(const GameInfo& game)
{
	Q_EMIT RequestIconPathSet(game.path);

	Emu.SetForceBoot(true);
	Emu.Stop();

	if (!Emu.BootGame(game.path))
	{
		QMessageBox::warning(this, tr("Warning!"), tr("Failed to boot ") + qstr(game.path));
		LOG_ERROR(LOADER, "Failed to boot %s", game.path);
		return false;
	}
	else
	{
		Q_EMIT RequestAddRecentGame(gui::Recent_Game(qstr(Emu.GetBoot()), qstr("[" + game.serial + "] " + game.name)));
		Refresh(true);
		return true;
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

void game_list_frame::DeleteShadersCache(int row)
{
	if (QMessageBox::question(this, tr("Confirm Delete"), tr("Delete shaders cache?")) != QMessageBox::Yes)
		return;

	const std::string config_base_dir = fs::get_config_dir() + "data/" + m_game_data[row].info.serial;

	if (fs::is_dir(config_base_dir))
	{
		fs::dir root = fs::dir(config_base_dir);
		fs::dir_entry tmp;

		while (root.read(tmp))
		{
			if (!fs::is_dir(config_base_dir + "/" + tmp.name))
				continue;

			const std::string shader_cache_name = config_base_dir + "/" + tmp.name + "/shaders_cache";
			if (fs::is_dir(shader_cache_name))
			{
				fs::remove_all(shader_cache_name, true);
			}
		}
	}
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

	if (m_Slider_Size->value() != sliderPos)
	{
		m_Slider_Size->setSliderPosition(sliderPos);
	}

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
			m_Icon_Color = gui::get_Label_Color("gamelist_icon_background_color");
		}
	}

	for (auto& game : m_game_data)
	{
		game.pxmap = PaintedPixmap(game.icon, game.hasCustomConfig);
	}

	Refresh();
}

int game_list_frame::GetSliderValue()
{
	return m_Slider_Size->value();
}

void game_list_frame::SetListMode(const bool& isList)
{
	m_oldLayoutIsList = m_isListLayout;
	m_isListLayout = isList;

	xgui_settings->SetValue(gui::gl_listMode, isList);

	m_categoryActs->setEnabled(isList);
	m_modeActList.action->setIcon(m_isListLayout ? m_modeActList.colored : m_modeActList.gray);
	m_modeActGrid.action->setIcon(m_isListLayout ? m_modeActGrid.gray : m_modeActGrid.colored);

	Refresh(true);

	m_Central_Widget->setCurrentWidget(m_isListLayout ? m_gameList : m_xgrid);
}

void game_list_frame::SetToolBarVisible(const bool& showToolBar)
{
	m_showToolBar = showToolBar;
	m_Tool_Bar->setVisible(showToolBar);
	xgui_settings->SetValue(gui::gl_toolBarVisible, showToolBar);
}
bool game_list_frame::GetToolBarVisible()
{
	return m_showToolBar;
}

void game_list_frame::SetCategoryActIcon(const int& id, const bool& active)
{
	m_categoryButtons.at(id)->action->setIcon(active ? m_categoryButtons.at(id)->colored : m_categoryButtons.at(id)->gray);
	m_categoryButtons.at(id)->isActive = active;
}

void game_list_frame::SetSearchText(const QString& text)
{
	m_searchText = text;
	Refresh();
}

void game_list_frame::RepaintToolBarIcons()
{
	QColor newColor;

	if (xgui_settings->GetValue(gui::m_enableUIColors).toBool())
	{
		newColor = xgui_settings->GetValue(gui::gl_toolIconColor).value<QColor>();
	}
	else
	{
		newColor = gui::get_Label_Color("gamelist_toolbar_icon_color");
	}

	auto icon = [&newColor](const QString& path, bool mask = false)
	{
		return gui_settings::colorizedIcon(QIcon(path), gui::gl_tool_icon_color, newColor, mask);
	};

	m_catActHDD.colored        = icon(":/Icons/hdd_blue.png", true);
	m_catActDisc.colored       = icon(":/Icons/disc_blue.png", true);
	m_catActHome.colored       = icon(":/Icons/home_blue.png");
	m_catActAudioVideo.colored = icon(":/Icons/media_blue.png", true);
	m_catActGameData.colored   = icon(":/Icons/data_blue.png", true);
	m_catActUnknown.colored    = icon(":/Icons/unknown_blue.png", true);
	m_catActOther.colored      = icon(":/Icons/other_blue.png");

	for (const auto& butt : m_categoryButtons)
	{
		butt->action->setIcon(butt->isActive ? butt->colored : butt->gray);
	}

	m_modeActList.colored = icon(":/Icons/list_blue.png");
	m_modeActList.action->setIcon(m_isListLayout ? m_modeActList.colored : m_modeActList.gray);

	m_modeActGrid.colored = icon(":/Icons/grid_blue.png");
	m_modeActGrid.action->setIcon(m_isListLayout ? m_modeActGrid.gray : m_modeActGrid.colored);

	m_Slider_Size->setStyleSheet(m_Slider_Size->styleSheet().append("QSlider::handle:horizontal{ background: rgba(%1, %2, %3, %4); }")
		.arg(newColor.red()).arg(newColor.green()).arg(newColor.blue()).arg(newColor.alpha()));
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
			Q_EMIT RequestSaveSliderPos(true);
			m_Slider_Size->setValue(m_Slider_Size->value() + value);
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
				Q_EMIT RequestSaveSliderPos(true);
				m_Slider_Size->setValue(m_Slider_Size->value() + 1);
				return true;
			}
			else if (keyEvent->key() == Qt::Key_Minus)
			{
				Q_EMIT RequestSaveSliderPos(true);
				m_Slider_Size->setValue(m_Slider_Size->value() - 1);
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

	auto l_GetItem = [](const std::string& text)
	{
		// force single line text ("hack" used instead of Qt shenanigans like Qt::TextSingleLine)
		QString formattedText = gui::get_Single_Line(qstr(text));

		QTableWidgetItem* curr = new QTableWidgetItem;
		curr->setFlags(curr->flags() & ~Qt::ItemIsEditable);
		curr->setText(formattedText);
		return curr;
	};

	int row = 0, index = -1;
	for (const auto& game : m_game_data)
	{
		index++;

		if (!game.isVisible)
		{
			continue;
		}

		// Icon
		QTableWidgetItem* icon_item = new QTableWidgetItem;
		icon_item->setFlags(icon_item->flags() & ~Qt::ItemIsEditable);
		icon_item->setData(Qt::DecorationRole, game.pxmap);
		icon_item->setData(Qt::UserRole, index);

		QTableWidgetItem* title_item = l_GetItem(game.info.name);
		if (game.hasCustomConfig)
		{
			title_item->setIcon(QIcon(":/Icons/cog_black.png"));
		}

		// Compatibility
		QTableWidgetItem* compat_item = new QTableWidgetItem;
		compat_item->setFlags(compat_item->flags() & ~Qt::ItemIsEditable);
		compat_item->setText(game.compat.text + (game.compat.date.isEmpty() ? "" : " (" + game.compat.date + ")"));
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
		m_gameList->setItem(row, gui::column_parental,   l_GetItem(GetStringFromU32(game.info.parental_lvl, parental::level)));
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

void game_list_frame::PopulateGameGrid(uint maxCols, const QSize& image_size, const QColor& image_color)
{
	uint r = 0;
	uint c = 0;

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
		if (category::CategoryInMap(m_game_data[i].info.category, category::cat_boot) && SearchMatchesApp(m_game_data[i].info.name, m_game_data[i].info.serial))
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
		QString title = gui::get_Single_Line(qstr(app.first->info.name));

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
		for (uint col = c; col < maxCols; ++col)
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

	// The index can be more than the size of m_game_data if you use the VFS to load a directory which has less games.
	if (m_oldLayoutIsList && m_gameList->selectedItems().count() && m_gameList->currentRow() < m_game_data.size())
	{
		selection = m_game_data.at(m_gameList->item(m_gameList->currentRow(), 0)->data(Qt::UserRole).toInt()).info.icon_path;
	}
	else if (!m_oldLayoutIsList && m_xgrid->selectedItems().count())
	{
		int ind = m_xgrid->currentItem()->data(Qt::UserRole).toInt();
		if (ind < m_game_data.size())
		{
			selection = m_game_data.at(ind).info.icon_path;
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
