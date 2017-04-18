#include "gamelistframe.h"
#include "stdafx.h"
#include "Emu\Memory\Memory.h"
#include "Emu\System.h"
#include "Loader\PSF.h"
#include "settingsDialog.h"
#include "Utilities/types.h"

#include <algorithm>
#include <QMenuBar>
#include <QProcess>
#include <QMessageBox>
#include <QDir>

static const std::string m_class_name = "GameViewer";

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

GameListFrame::GameListFrame(QWidget *parent) : QDockWidget(tr("Game List"), parent)
{
	gameList = new QTableWidget(this);
	gameList->setSelectionBehavior(QAbstractItemView::SelectRows);
	gameList->setColumnCount(7);
	gameList->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("Icon")));
	gameList->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("Name")));
	gameList->setHorizontalHeaderItem(2, new QTableWidgetItem(tr("Serial")));
	gameList->setHorizontalHeaderItem(3, new QTableWidgetItem(tr("FW")));
	gameList->setHorizontalHeaderItem(4, new QTableWidgetItem(tr("App version")));
	gameList->setHorizontalHeaderItem(5, new QTableWidgetItem(tr("Category")));
	gameList->setHorizontalHeaderItem(6, new QTableWidgetItem(tr("Path")));

	setWidget(gameList);

	gameList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(gameList, &QTableWidget::customContextMenuRequested, this, &GameListFrame::ShowContextMenu);
	connect(gameList, &QTableWidget::doubleClicked, this, &GameListFrame::doubleClickedSlot);
}

GameListFrame::~GameListFrame()
{

}

//void GameListFrame::OnColClicked(QEvent& event)
//{
//	if (event.GetColumn() == m_sortColumn)
//		m_sortAscending ^= true;
//	else
//		m_sortAscending = true;
//	m_sortColumn = event.GetColumn();
//
//	// Sort entries, update columns and refresh the panel
//	std::sort(m_game_data.begin(), m_game_data.end(), sortGameData(m_sortColumn, m_sortAscending));
//	m_columns.Update(m_game_data);
//	ShowData();
//}


void GameListFrame::LoadGames()
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

void GameListFrame::LoadPSF()
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
			game.category = "HDD Game";
			game.icon_path = dir + "/ICON0.PNG";
		}
		else if (game.category == "DG")
		{
			game.category = "Disc Game";
			game.icon_path = dir + "/PS3_GAME/ICON0.PNG";
		}
		else if (game.category == "HM")
		{
			game.category = "Home";
			game.icon_path = dir + "/ICON0.PNG";
		}
		else if (game.category == "AV")
		{
			game.category = "Audio/Video";
			game.icon_path = dir + "/ICON0.PNG";
		}
		else if (game.category == "GD")
		{
			game.category = "Game Data";
			game.icon_path = dir + "/ICON0.PNG";
		}

		m_game_data.push_back(game);
	}

	// Sort entries and update columns
	std::sort(m_game_data.begin(), m_game_data.end(), sortGameData(m_sortColumn, m_sortAscending));
	m_columns.Update(m_game_data);
}

void GameListFrame::ShowData()
{
	m_columns.ShowData(this);
}

void GameListFrame::Refresh()
{
	LoadGames();
	LoadPSF();
	ShowData();
}

void GameListFrame::SaveSettings()
{
	m_columns.LoadSave(false, m_class_name, this);
}

void GameListFrame::LoadSettings()
{
	m_columns.LoadSave(true, m_class_name);
}

void GameListFrame::doubleClickedSlot(const QModelIndex& index)
{
	int i = index.row();
	const std::string& path = Emu.GetGameDir() + m_game_data[i].root;

	Emu.Stop();

	if (!Emu.BootGame(path))
	{
		LOG_ERROR(LOADER, "Failed to boot /dev_hdd0/game/%s", m_game_data[i].root);
	}
}

void GameListFrame::ShowContextMenu(const QPoint &pos) // this is a slot
{
	QModelIndex index = gameList->indexAt(pos);
	// for most widgets
	QPoint globalPos = gameList->mapToGlobal(pos);
	// for QAbstractScrollArea and derived classes you would use:
	// QPoint globalPos = myWidget->viewport()->mapToGlobal(pos);

	QMenu myMenu;
	QFont f = myMenu.menuAction()->font();
	f.setBold(true);
	myMenu.menuAction()->setFont(f);
	myMenu.addAction(bootAct);
	f.setBold(false);
	myMenu.menuAction()->setFont(f);
	myMenu.addAction(configureAct);
	myMenu.addSeparator();
	myMenu.addAction(removeGameAct);
	myMenu.addAction(removeCustomConfigurationAct);
	myMenu.addSeparator();
	myMenu.addAction(openGameFolderAct);
	myMenu.addAction(openConfigFolderAct);

	if (index.isValid()) QAction* selectedItem = myMenu.exec(globalPos);
}

void GameListFrame::Boot()
{
	for (int i = 0; i < gameList->selectedItems().size(); ++i) {
		// Get curent item on selected row
		QTableWidgetItem *item = gameList->takeItem(gameList->currentRow(), gameList->currentColumn());
		const std::string& path = Emu.GetGameDir() + m_game_data[i].root;

		Emu.Stop();

		if (!Emu.BootGame(path))
		{
			LOG_ERROR(LOADER, "Failed to boot /dev_hdd0/game/%s", m_game_data[i].root);
		}
	}
}

void GameListFrame::Configure()
{
	for (int i = 0; i < gameList->selectedItems().size(); ++i) {
		// Get curent item on selected row
		QTableWidgetItem *item = gameList->takeItem(gameList->currentRow(), gameList->currentColumn());
		SettingsDialog(this, "data/" + m_game_data[i].serial);
	}
}

void GameListFrame::RemoveGame()
{
	for (int i = 0; i < gameList->selectedItems().size(); ++i) {
		// Get curent item on selected row
		QTableWidgetItem *item = gameList->takeItem(gameList->currentRow(), 6); // 6 should be path
		if (QMessageBox::question(this , tr("Confirm Delete"), tr("Permanently delete game files?")) == QMessageBox::Yes)
		{
			fs::remove_all(Emu.GetGameDir() + static_cast<std::string>(item->text().toUtf8()));
		}
	}
	Refresh();
}

void GameListFrame::RemoveCustomConfiguration()
{

	for (int i = 0; i < gameList->selectedItems().size(); ++i) {
		// Get curent item on selected row
		QTableWidgetItem *item = gameList->takeItem(gameList->currentRow(), 6); // 6 should be path
		const std::string config_path = fs::get_config_dir() + "data/" + m_game_data[i].serial + "/config.yml";

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
					LOG_FATAL(GENERAL, "Failed to delete configuration file: %s\nError: %s", config_path, fs::g_tls_error);
				}
			}
		}
		else
		{
			LOG_ERROR(GENERAL, "Configuration file not found: %s", config_path);
		}

	}
}

static void open_dir(const std::string& spath)
{
	fs::create_dir(spath);
	QString path = QString::fromStdString(spath);
	QProcess process;

#ifdef _WIN32
	std::string command = "explorer";
	std::replace(path.begin(), path.end(), '/', '\\');
	process.start("explorer", QStringList() << path);
#elif __APPLE__
	process.start("open", QStringList() << path);
#elif __linux__
	process.start("xdg-open", QStringList() << path);
#endif
}

void GameListFrame::OpenGameFolder()
{
	for (int i = 0; i < gameList->selectedItems().size(); ++i) {
		// Get curent item on selected row
		QTableWidgetItem *item = gameList->takeItem(gameList->currentRow(), gameList->currentColumn());
		open_dir(Emu.GetGameDir() + m_game_data[i].root);
	}
}

void GameListFrame::OpenConfigFolder()
{
	for (int i = 0; i < gameList->selectedItems().size(); ++i) {
		// Get curent item on selected row
		QTableWidgetItem *item = gameList->takeItem(gameList->currentRow(), gameList->currentColumn());
		open_dir(fs::get_config_dir() + "data/" + m_game_data[i].serial);
	}
}

ColumnsArr::ColumnsArr()
{
	Init();
}

std::vector<Column*> ColumnsArr::GetSortedColumnsByPos()
{
	std::vector<Column*> arr;
	for (u32 pos = 0; pos<m_columns.size(); pos++)
	{
		for (u32 c = 0; c<m_columns.size(); ++c)
		{
			if (m_columns[c].pos != pos) continue;
			arr.push_back(&m_columns[c]);
		}
	}

	return arr;
}

Column* ColumnsArr::GetColumnByPos(u32 pos)
{
	std::vector<Column *> columns = GetSortedColumnsByPos();
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

void ColumnsArr::Init()
{
	m_img_list = new QListWidget();
	m_img_list->setIconSize(QSize(80, 44));
	//m_img_list->setViewMode(QListWidget::IconMode); // Do we really want icon mode? I'm going to override that when I add the images anyhow probably

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

void ColumnsArr::Update(const std::vector<GameInfo>& game_data)
{
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

	// load icons
	for (const auto& path : m_col_icon->data)
	{
		QImage game_icon(80, 44, QImage::Format_RGB32);
		if (!path.empty())
		{
			QListWidgetItem *thumbnail = new QListWidgetItem;
			thumbnail->setFlags(thumbnail->flags() & ~Qt::ItemIsEditable);

			// Load image.
			QImage* img = new QImage;
			bool success = img->load(QString::fromStdString(path));
			if (success)
			{
				thumbnail->setData(Qt::DecorationRole, QPixmap::fromImage(*img));
				m_img_list->addItem(thumbnail);
			}
			else {
				QString abspath = QDir(QString::fromStdString(path)).absolutePath();
				LOG_ERROR(HLE, "Count not load image from path %s", abspath.toStdString());
				m_img_list->addItem("ImgLoadFailed");
			}
		}

		m_icon_indexes.push_back(-1);
	}
}

void ColumnsArr::Show(QDockWidget* list)
{
	//list->DeleteAllColumns();
	//list->SetImageList(m_img_list, wxIMAGE_LIST_SMALL);
	//std::vector<Column *> c_col = GetSortedColumnsByPos();
	//for (u32 i = 0, c = 0; i<c_col.size(); ++i)
	//{
	//	if (!c_col[i]->shown) continue;
	//	list->InsertColumn(c++, fmt::FromUTF8(c_col[i]->name), 0, c_col[i]->width);
	//}
}

void ColumnsArr::ShowData(QDockWidget* list)
{
	//list->DeleteAllItems();
	//list->SetImageList(m_img_list, wxIMAGE_LIST_SMALL);
	//for (int c = 1; c<list->model()->columnCount(); ++c)
	//{
	//	Column* col = GetColumnByPos(c);
	//
	//	if (!col)
	//	{
	//		LOG_ERROR(HLE, "Columns loaded with error!");
	//		return;
	//	}
	//
	//	for (u32 i = 0; i<col->data.size(); ++i)
	//	{
	//		if (list->model()->rowCount() <= (int)i)
	//		{
	//			list->InsertItem(i, QString());
	//			list->SetItemData(i, i);
	//		}
	//		list->SetItem(i, c, fmt::FromUTF8(col->data[i]));
	//		if (m_icon_indexes[i] >= 0)
	//			list->SetItemColumnImage(i, 0, m_icon_indexes[i]);
	//	}
	//}
}

void ColumnsArr::LoadSave(bool isLoad, const std::string& path, QDockWidget* list)
{
	if (isLoad)
	{
		Init();
	}
	//else if (list)
	//{
	//	for (int c = 0; c < list->model()->columnCount(); ++c)
	//	{
	//		Column* col = GetColumnByPos(c);
	//		if (col)
	//			col->width = list->getContentsMargins(c);
	//	}
	//}
	//
	//auto&& cfg = g_gui_cfg["GameViewer"];
	//
	//for (auto& column : m_columns)
	//{
	//	auto&& c_cfg = cfg[column.name];
	//
	//	if (isLoad)
	//	{
	//		std::tie(column.pos, column.width) = c_cfg.as<std::pair<u32, u32>>(std::make_pair(column.def_pos, column.def_width));
	//
	//		column.shown = true;
	//	}
	//	else //if (column.shown)
	//	{
	//		c_cfg = std::make_pair(column.pos, column.width);
	//	}
	//}
	//
	//if (isLoad)
	//{
	//	//check for errors
	//	for (u32 c1 = 0; c1 < m_columns.size(); ++c1)
	//	{
	//		for (u32 c2 = c1 + 1; c2 < m_columns.size(); ++c2)
	//		{
	//			if (m_columns[c1].pos == m_columns[c2].pos)
	//			{
	//				LOG_ERROR(HLE, "Columns loaded with error!");
	//				Init();
	//				return;
	//			}
	//		}
	//	}
	//
	//	for (u32 p = 0; p < m_columns.size(); ++p)
	//	{
	//		bool ishas = false;
	//		for (u32 c = 0; c < m_columns.size(); ++c)
	//		{
	//			if (m_columns[c].pos != p)
	//				continue;
	//
	//			ishas = true;
	//			break;
	//		}
	//
	//		if (!ishas)
	//		{
	//			LOG_ERROR(HLE, "Columns loaded with error!");
	//			Init();
	//			return;
	//		}
	//	}
	//}
	//else
	//{
	//	save_gui_cfg();
	//}
}

void GameListFrame::CreateActions()
{
	bootAct = new QAction(tr("&Boot"), this);
	connect(bootAct, &QAction::triggered, this, &GameListFrame::Boot);

	configureAct = new QAction(tr("&Configure"), this);
	connect(configureAct, &QAction::triggered, this, &GameListFrame::Configure);

	removeGameAct = new QAction(tr("&Remove Game"), this);
	connect(removeGameAct, &QAction::triggered, this, &GameListFrame::RemoveGame);

	removeCustomConfigurationAct = new QAction(tr("&Remove Custom Configuration"), this);
	connect(removeCustomConfigurationAct, &QAction::triggered, this, &GameListFrame::RemoveCustomConfiguration);

	openGameFolderAct = new QAction(tr("&Open Game Folder"), this);
	connect(openGameFolderAct, &QAction::triggered, this, &GameListFrame::OpenGameFolder);

	openConfigFolderAct = new QAction(tr("&Open Config Folder"), this);
	connect(openConfigFolderAct, &QAction::triggered, this, &GameListFrame::OpenConfigFolder);
}

void GameListFrame::closeEvent(QCloseEvent *event)
{
	QDockWidget::closeEvent(event);
	emit GameListFrameClosed();
}
