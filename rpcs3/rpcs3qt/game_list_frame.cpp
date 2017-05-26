#include "game_list_frame.h"

#include "settings_dialog.h"
#include "table_item_delegate.h"

#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Loader/PSF.h"
#include "Utilities/types.h"

#include <algorithm>
#include <memory>

#include <QDir>
#include <QHeaderView>
#include <QListView>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>

static const std::string m_class_name = "GameViewer";
inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }

// Auxiliary classes
class sortGameData
{
	int sortColumn;
	bool sortAscending;

public:
	sortGameData(u32 column, bool ascending) : sortColumn(column), sortAscending(ascending) {}
	bool operator()(const GameInfo& game1, const GameInfo& game2) const
	{
		// Note that the column index has to match the appropriate GameInfo member
		switch (sortColumn - 1) // skip *icon* column
		{
		case 0: return sortAscending ? (game1.name < game2.name) : (game1.name > game2.name);
		case 1: return sortAscending ? (game1.serial < game2.serial) : (game1.serial > game2.serial);
		case 2: return sortAscending ? (game1.fw < game2.fw) : (game1.fw > game2.fw);
		case 3: return sortAscending ? (game1.app_ver < game2.app_ver) : (game1.app_ver > game2.app_ver);
		case 4: return sortAscending ? (game1.category < game2.category) : (game1.category > game2.category);
		case 5: return sortAscending ? (game1.root < game2.root) : (game1.root > game2.root);
		default: return false;
		}
	}
};

game_list_frame::game_list_frame(std::shared_ptr<gui_settings> settings, Render_Creator r_Creator, QWidget *parent) 
	: QDockWidget(tr("Game List"), parent), xgui_settings(settings), m_Render_Creator(r_Creator)
{
	CreateActions();

	gameList = new QTableWidget(this);
	gameList->setShowGrid(false);
	gameList->setItemDelegate(new table_item_delegate(this));
	gameList->setSelectionBehavior(QAbstractItemView::SelectRows);
	gameList->setSelectionMode(QAbstractItemView::SingleSelection);
	gameList->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);	
	gameList->verticalHeader()->setMinimumSectionSize(m_Icon_Size.height());
	gameList->verticalHeader()->setMaximumSectionSize(m_Icon_Size.height());
	gameList->verticalHeader()->setVisible(false);
	gameList->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	gameList->setContextMenuPolicy(Qt::CustomContextMenu);

	gameList->setColumnCount(7);
	gameList->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("Icon")));
	gameList->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("Name")));
	gameList->setHorizontalHeaderItem(2, new QTableWidgetItem(tr("Serial")));
	gameList->setHorizontalHeaderItem(3, new QTableWidgetItem(tr("FW")));
	gameList->setHorizontalHeaderItem(4, new QTableWidgetItem(tr("App version")));
	gameList->setHorizontalHeaderItem(5, new QTableWidgetItem(tr("Category")));
	gameList->setHorizontalHeaderItem(6, new QTableWidgetItem(tr("Path")));

	setWidget(gameList);

	connect(gameList, &QTableWidget::customContextMenuRequested, this, &game_list_frame::ShowContextMenu);
	connect(gameList->horizontalHeader(), &QHeaderView::customContextMenuRequested, [=](const QPoint& pos){
		QMenu* configure = new QMenu(this);
		configure->addActions({ showIconColAct, showNameColAct, showSerialColAct, showFWColAct, showAppVersionColAct, showCategoryColAct, showPathColAct });
		configure->exec(mapToGlobal(pos));
	});
	connect(gameList, &QTableWidget::doubleClicked, this, &game_list_frame::doubleClickedSlot);
	connect(gameList->horizontalHeader(), &QHeaderView::sectionClicked, this, &game_list_frame::OnColClicked);

	Refresh(); // Data MUST be loaded so that first settings load will reset columns to correct width w/r to data.
	LoadSettings();
}

void game_list_frame::LoadSettings()
{
	QByteArray state = xgui_settings->GetGameListState();

	for (int col = 0; col < columnActs.length(); ++col)
	{
		bool vis = xgui_settings->GetGamelistColVisibility(col);
		columnActs[col]->setChecked(vis);
		gameList->setColumnHidden(col, !vis);
	}
	m_sortAscending = xgui_settings->GetGamelistSortAsc();
	m_sortColumn = xgui_settings->GetGamelistSortCol();

	categoryFilters = xgui_settings->GetGameListCategoryFilters();

	if (state.isEmpty())
	{ // If no settings exist, go to default.
		gameList->verticalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
		gameList->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
		gameList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
	}
	else
	{
		gameList->horizontalHeader()->restoreState(state);
	}

	Refresh();
}

game_list_frame::~game_list_frame()
{
	SaveSettings();
}

void game_list_frame::OnColClicked(int col)
{
	if (col == m_sortColumn)
	{
		m_sortAscending ^= true;
	}
	else
	{
		m_sortAscending = true;
	}
	m_sortColumn = col;

	xgui_settings->SetGamelistSortAsc(m_sortAscending);
	xgui_settings->SetGamelistSortCol(col);

	// Sort entries, update columns and refresh the panel
	Refresh();
}


void game_list_frame::LoadGames()
{
	m_games.clear();

	for (const auto& entry : fs::dir(Emu.GetGameDir()))
	{
		if (entry.is_directory)
		{
			m_games.push_back(entry.name);
		}
	}
}

void game_list_frame::LoadPSF()
{
	m_game_data.clear();

	const std::string& game_path = Emu.GetGameDir();

	for (u32 i = 0; i < m_games.size(); ++i)
	{
		const std::string& dir = game_path + m_games[i];
		const std::string& sfb = dir + "/PS3_DISC.SFB";
		const std::string& sfo = dir + (fs::is_file(sfb) ? "/PS3_GAME/PARAM.SFO" : "/PARAM.SFO");

		const fs::file sfo_file(sfo);
		if (!sfo_file)
		{
			continue;
		}

		const auto& psf = psf::load_object(sfo_file);

		GameInfo game;
		game.root = m_games[i];
		game.serial = psf::get_string(psf, "TITLE_ID", "");
		game.name = psf::get_string(psf, "TITLE", "unknown");
		game.app_ver = psf::get_string(psf, "APP_VER", "unknown");
		game.category = psf::get_string(psf, "CATEGORY", "unknown");
		game.fw = psf::get_string(psf, "PS3_SYSTEM_VER", "unknown");
		game.parental_lvl = psf::get_integer(psf, "PARENTAL_LEVEL");
		game.resolution = psf::get_integer(psf, "RESOLUTION");
		game.sound_format = psf::get_integer(psf, "SOUND_FORMAT");

		if (game.category == "HG")
		{
			game.category = sstr(category::hdd_Game);
			game.icon_path = dir + "/ICON0.PNG";
		}
		else if (game.category == "DG")
		{
			game.category = sstr(category::disc_Game);
			game.icon_path = dir + "/PS3_GAME/ICON0.PNG";
		}
		else if (game.category == "HM")
		{
			game.category = sstr(category::home);
			game.icon_path = dir + "/ICON0.PNG";
		}
		else if (game.category == "AV")
		{
			game.category = sstr(category::audio_Video);
			game.icon_path = dir + "/ICON0.PNG";
		}
		else if (game.category == "GD")
		{
			game.category = sstr(category::game_Data);
			game.icon_path = dir + "/ICON0.PNG";
		}

		m_game_data.push_back(game);
	}

	// Sort entries and update columns
	std::sort(m_game_data.begin(), m_game_data.end(), sortGameData(m_sortColumn, m_sortAscending));
	m_columns.Update(m_game_data);
}

void game_list_frame::ShowData()
{
	m_columns.ShowData(gameList);
}

// Filter for Categories
void game_list_frame::FilterData()
{
	for (int i = 0; i < gameList->rowCount(); ++i)
	{
		bool match = false;
		for (auto filter : categoryFilters)
		{
			for (int j = 0; j < gameList->columnCount(); ++j)
			{
				if (gameList->horizontalHeaderItem(j)->text() == tr("Category") && gameList->item(i, j)->text().contains(filter))
				{
					match = true;
					goto OutOfThis;
				}
			}
		}
		OutOfThis:
		gameList->setRowHidden(i, !match);
	}
}

void game_list_frame::Refresh()
{
	int row = gameList->currentRow();
	LoadGames();
	LoadPSF();
	ShowData();
	FilterData();
	gameList->selectRow(row);
}

void game_list_frame::ToggleCategoryFilter(QString category, bool show)
{
	if (show) { categoryFilters.append(category); } else { categoryFilters.removeAll(category); }
	Refresh();
}

void game_list_frame::SaveSettings()
{
	for (int col = 0; col < columnActs.length(); ++col)
	{
		xgui_settings->SetGamelistColVisibility(col, columnActs[col]->isChecked());
	}
	xgui_settings->SetGamelistSortCol(m_sortColumn);
	xgui_settings->SetGamelistSortAsc(m_sortAscending);

	xgui_settings->WriteGameListState(gameList->horizontalHeader()->saveState());
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
#elif __linux__
	process->start("xdg-open", QStringList() << path);
#endif
}

void game_list_frame::doubleClickedSlot(const QModelIndex& index)
{
	int i = index.row();
	QString category = qstr(m_game_data[i].category);

	// Boot these categories
	if (category == category::hdd_Game || category == category::disc_Game || category == category::audio_Video)
	{
		const std::string& path = Emu.GetGameDir() + m_game_data[i].root;
		emit RequestIconPathSet(path);

		Emu.Stop();

		if (!Emu.BootGame(path))
		{
			LOG_ERROR(LOADER, "Failed to boot /dev_hdd0/game/%s", m_game_data[i].root);
		}
	}
	else
	{
		open_dir(Emu.GetGameDir() + m_game_data[i].root);
	}
}

void game_list_frame::CreateActions()
{
	showIconColAct = new QAction(tr("Show Icons"), this);
	showNameColAct = new QAction(tr("Show Names"), this);
	showSerialColAct = new QAction(tr("Show Serials"), this);
	showFWColAct = new QAction(tr("Show FWs"), this);
	showAppVersionColAct = new QAction(tr("Show App Versions"), this);
	showCategoryColAct = new QAction(tr("Show Categories"), this);
	showPathColAct = new QAction(tr("Show Paths"), this);

	columnActs = { showIconColAct, showNameColAct, showSerialColAct, showFWColAct, showAppVersionColAct, showCategoryColAct, showPathColAct };

	for (int col = 0; col < columnActs.length(); ++col)
	{
		QAction* act = columnActs[col];

		act->setCheckable(true);
		act->setChecked(true);

		auto l_CallBack = [this, col](bool val) {
			gameList->setColumnHidden(col, !val); // Negate because it's a set col hidden and we have menu say show.
			xgui_settings->SetGamelistColVisibility(col, val);
		};

		connect(act, &QAction::triggered, l_CallBack);
	}
}

void game_list_frame::ShowContextMenu(const QPoint &pos) // this is a slot
{
	int row = gameList->indexAt(pos).row();
	
	if (row == -1)
	{
		return; // invalid
	}

	// for most widgets
	QPoint globalPos = gameList->mapToGlobal(pos);
	// for QAbstractScrollArea and derived classes you would use:
	// QPoint globalPos = myWidget->viewport()->mapToGlobal(pos);

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

	connect(boot, &QAction::triggered, [=]() {Boot(row); });
	connect(configure, &QAction::triggered, [=](){
		settings_dialog(xgui_settings, m_Render_Creator, this, &m_game_data[row]).exec();
	});
	connect(removeGame, &QAction::triggered, [=](){
		if (QMessageBox::question(this, tr("Confirm Delete"), tr("Permanently delete files?")) == QMessageBox::Yes)
			fs::remove_all(Emu.GetGameDir() + m_game_data[row].root);
		Refresh();
	});
	connect(removeConfig, &QAction::triggered, [=]() {RemoveCustomConfiguration(row); });
	connect(openGameFolder, &QAction::triggered, [=]() {open_dir(Emu.GetGameDir() + m_game_data[row].root); });
	connect(openConfig, &QAction::triggered, [=]() {open_dir(fs::get_config_dir() + "data/" + m_game_data[row].serial); });

	//Disable options depending on software category
	QString category = qstr(m_game_data[row].category);

	if (category == category::disc_Game)
	{
		removeGame->setEnabled(false);
	}
	else if (category == category::audio_Video)
	{
		configure->setEnabled(false);
		removeConfig->setEnabled(false);
		openConfig->setEnabled(false);
	}
	else if (category == category::home || category == category::game_Data)
	{
		boot->setEnabled(false), f.setBold(false), boot->setFont(f);
		configure->setEnabled(false);
		removeConfig->setEnabled(false);
		openConfig->setEnabled(false);
	}
	
	myMenu.exec(globalPos);
}

void game_list_frame::Boot(int row)
{
	const std::string& path = Emu.GetGameDir() + m_game_data[row].root;
	emit RequestIconPathSet(path);

	Emu.Stop();

	if (!Emu.BootGame(path))
	{
		QMessageBox::warning(this, tr("Warning!"), tr("Failed to boot ") + qstr(m_game_data[row].root));
		LOG_ERROR(LOADER, "Failed to boot /dev_hdd0/game/%s", m_game_data[row].root);
	}
}

void game_list_frame::RemoveCustomConfiguration(int row)
{
	const std::string config_path = fs::get_config_dir() + "data/" + m_game_data[row].serial + "/config.yml";

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

columns_arr::columns_arr(QSize icon_Size) : m_Icon_Size(icon_Size)
{
	m_img_list = new QList<QImage*>();

	m_columns.clear();
	m_columns.emplace_back(0, 90, "Icon");
	m_columns.emplace_back(1, 160, "Name");
	m_columns.emplace_back(2, 85, "Serial");
	m_columns.emplace_back(3, 55, "FW");
	m_columns.emplace_back(4, 55, "App version");
	m_columns.emplace_back(5, 75, "Category");
	m_columns.emplace_back(6, 160, "Path");
	m_col_icon = &m_columns[0];
	m_col_name = &m_columns[1];
	m_col_serial = &m_columns[2];
	m_col_fw = &m_columns[3];
	m_col_app_ver = &m_columns[4];
	m_col_category = &m_columns[5];
	m_col_path = &m_columns[6];
}

Column* columns_arr::GetColumnByPos(u32 pos)
{
	std::vector<Column *> columns;
	for (u32 pos = 0; pos<m_columns.size(); pos++)
	{
		for (u32 c = 0; c<m_columns.size(); ++c)
		{
			if (m_columns[c].pos != pos) continue;
			columns.push_back(&m_columns[c]);
		}
	}
	for (u32 c = 0; c<columns.size(); ++c)
	{
		if (!columns[c]->shown)
		{
			pos++;
			continue;
		}
		if (columns[c]->pos != pos) continue;
		return columns[c];
	}

	return NULL;
}

void columns_arr::Update(const std::vector<GameInfo>& game_data)
{
	m_img_list->clear();
	m_col_icon->data.clear();
	m_col_name->data.clear();
	m_col_serial->data.clear();
	m_col_fw->data.clear();
	m_col_app_ver->data.clear();
	m_col_category->data.clear();
	m_col_path->data.clear();
	m_icon_indexes.clear();

	if (m_columns.size() == 0) return;

	for (const auto& game : game_data)
	{
		m_col_icon->data.push_back(game.icon_path);
		m_col_name->data.push_back(game.name);
		m_col_serial->data.push_back(game.serial);
		m_col_fw->data.push_back(game.fw);
		m_col_app_ver->data.push_back(game.app_ver);
		m_col_category->data.push_back(game.category);
		m_col_path->data.push_back(game.root);
	}

	int c = 0;
	// load icons
	for (const auto& path : m_col_icon->data)
	{
		QImage* img = new QImage(m_Icon_Size, QImage::Format_ARGB32);
		if (!path.empty())
		{
			// Load image.
			bool success = img->load(qstr(path));
			if (success)
			{
				m_img_list->append(new QImage(img->scaled(m_Icon_Size, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation)));
			}
			else {
				// IIRC a load failure means blank image which is fine to have as a placeholder.
				QString abspath = QDir(qstr(path)).absolutePath();
				LOG_ERROR(HLE, "Count not load image from path %s", sstr(abspath));
				img->fill(QColor(0, 0, 0, 0));
				m_img_list->append(img);
			}
		}
		else
		{
			LOG_ERROR(HLE, "Count not load image from empty path");
			img->fill(QColor(0, 0, 0, 0));
			m_img_list->append(img);
		}
		m_icon_indexes.push_back(c);
		c++;
	}
}

void columns_arr::ShowData(QTableWidget* table)
{
	// Hack to delete everything without removing the headers.
	table->setRowCount(0);

	// Expect number of columns to be the same as number of icons.
	table->setRowCount(m_img_list->length());
	
	// Add icons.
	for (int r = 0; r < m_img_list->length(); ++r)
	{
		QTableWidgetItem* iconItem = new QTableWidgetItem;
		iconItem->setFlags(iconItem->flags() & ~Qt::ItemIsEditable);
		iconItem->setData(Qt::DecorationRole, QPixmap::fromImage(*m_img_list->at(m_icon_indexes[r])));
		table->setItem(r, 0, iconItem);
	}

	// Add the other data.
	for (int c = 1; c < table->columnCount(); ++c)
	{
		Column* col = GetColumnByPos(c);
	
		if (!col)
		{
			LOG_ERROR(HLE, "Columns loaded with error!");
			return;
		}
	
		int numRows = col->data.size();
		if (numRows != table->rowCount())
		{
			table->setRowCount(numRows);
			LOG_WARNING(HLE, "Warning. Columns are of different size: number of icons %d number wanted: %d", m_img_list->length(), numRows);
		}

		for (int r = 0; r<col->data.size(); ++r)
		{
			QTableWidgetItem* curr = new QTableWidgetItem;
			curr->setFlags(curr->flags() & ~Qt::ItemIsEditable);
			QString text = qstr(col->data[r]);
			curr->setText(text);
			table->setItem(r, c, curr);
		}
		table->resizeRowsToContents();
		table->resizeColumnToContents(0);
	}
}

void game_list_frame::closeEvent(QCloseEvent *event)
{
	QDockWidget::closeEvent(event);
	emit game_list_frameClosed();
}
