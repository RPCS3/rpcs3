#include "stdafx.h"
#include "stdafx_gui.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "GameViewer.h"
#include "Loader/PSF.h"
#include "SettingsDialog.h"

#include <algorithm>

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
		case 0: return sortAscending ? (game1.name < game2.name)         : (game1.name > game2.name);
		case 1: return sortAscending ? (game1.serial < game2.serial)     : (game1.serial > game2.serial);
		case 2: return sortAscending ? (game1.fw < game2.fw)             : (game1.fw > game2.fw);
		case 3: return sortAscending ? (game1.app_ver < game2.app_ver)   : (game1.app_ver > game2.app_ver);
		case 4: return sortAscending ? (game1.category < game2.category) : (game1.category > game2.category);
		case 5: return sortAscending ? (game1.root < game2.root)         : (game1.root > game2.root);
		default: return false;
		}
	}
};

// GameViewer functions
GameViewer::GameViewer(wxWindow* parent) : wxListView(parent)
{
	LoadSettings();
	m_columns.Show(this);

	m_sortColumn = 1;
	m_sortAscending = true;
	m_popup = new wxMenu();
	InitPopupMenu();

	Bind(wxEVT_LIST_ITEM_ACTIVATED, &GameViewer::DClick, this);
	Bind(wxEVT_LIST_COL_CLICK, &GameViewer::OnColClick, this);
	Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &GameViewer::RightClick, this);

	Refresh();
}

GameViewer::~GameViewer()
{
	SaveSettings();
}

void GameViewer::InitPopupMenu()
{
	wxMenuItem* boot_item = new wxMenuItem(m_popup, 0, _T("Boot"));
#if defined (_WIN32)
	// wxMenuItem::Set(Get)Font only available for the wxMSW port
	wxFont font = GetFont();
	font.SetWeight(wxFONTWEIGHT_BOLD);
	boot_item->SetFont(font);
#endif
	m_popup->Append(boot_item);
	m_popup->Append(1, _T("Configure"));
	m_popup->Append(2, _T("Remove Game"));

	Bind(wxEVT_MENU, &GameViewer::BootGame, this, 0);
	Bind(wxEVT_MENU, &GameViewer::ConfigureGame, this, 1);
	Bind(wxEVT_MENU, &GameViewer::RemoveGame, this, 2);
}

void GameViewer::DoResize(wxSize size)
{
	SetSize(size);
}

void GameViewer::OnColClick(wxListEvent& event)
{
	if (event.GetColumn() == m_sortColumn)
		m_sortAscending ^= true;
	else
		m_sortAscending = true;
	m_sortColumn = event.GetColumn();

	// Sort entries, update columns and refresh the panel
	std::sort(m_game_data.begin(), m_game_data.end(), sortGameData(m_sortColumn, m_sortAscending));
	m_columns.Update(m_game_data);
	ShowData();
}

void GameViewer::LoadGames()
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

void GameViewer::LoadPSF()
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
			continue;
		}
			
		m_game_data.push_back(game);
	}

	// Sort entries and update columns
	std::sort(m_game_data.begin(), m_game_data.end(), sortGameData(m_sortColumn, m_sortAscending));
	m_columns.Update(m_game_data);
}

void GameViewer::ShowData()
{
	m_columns.ShowData(this);
}

void GameViewer::Refresh()
{
	LoadGames();
	LoadPSF();
	ShowData();
}

void GameViewer::SaveSettings()
{
	m_columns.LoadSave(false, m_class_name, this);
}

void GameViewer::LoadSettings()
{
	m_columns.LoadSave(true, m_class_name);
}

void GameViewer::DClick(wxListEvent& event)
{
	long i = GetFirstSelected();
	if (i < 0) return;

	const std::string& path = Emu.GetGameDir() + m_game_data[i].root;

	Emu.Stop();

	if (!Emu.BootGame(path))
	{
		LOG_ERROR(LOADER, "Failed to boot /dev_hdd0/game/%s", m_game_data[i].root);
	}
}

void GameViewer::RightClick(wxListEvent& event)
{
	PopupMenu(m_popup, event.GetPoint());
}

void GameViewer::BootGame(wxCommandEvent& WXUNUSED(event))
{
	wxListEvent unused_event;
	DClick(unused_event);
}

void GameViewer::ConfigureGame(wxCommandEvent& WXUNUSED(event))
{
	long i = GetFirstSelected();
	if (i < 0) return;
	SettingsDialog(this, "data/" + m_game_data[i].serial);
}

void GameViewer::RemoveGame(wxCommandEvent& event)
{
	long i = GetFirstSelected();
	if (i < 0) return;

	if (wxMessageBox("Permanently delete game files?", "Confirm Delete", wxYES_NO | wxNO_DEFAULT) == wxYES)
	{
		fs::remove_all(Emu.GetGameDir() + fmt::ToUTF8(this->GetItemText(i, 6)));
	}

	Refresh();
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
	m_img_list = new wxImageList(80, 44);

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
		wxImage game_icon(80, 44);
		if (!path.empty())
		{
			wxLogNull logNo; // temporary disable wx warnings ("iCCP: known incorrect sRGB profile" spamming)
			if (game_icon.LoadFile(fmt::FromUTF8(path), wxBITMAP_TYPE_PNG))
			{
				game_icon.Rescale(80, 44, wxIMAGE_QUALITY_HIGH);
				m_icon_indexes.push_back(m_img_list->Add(game_icon));
				continue;
			}
			else
			{
				LOG_ERROR(GENERAL, "Error loading image %s", path);
			}
		}

		m_icon_indexes.push_back(-1);
	}
}

void ColumnsArr::Show(wxListView* list)
{
	list->DeleteAllColumns();
	list->SetImageList(m_img_list, wxIMAGE_LIST_SMALL);
	std::vector<Column *> c_col = GetSortedColumnsByPos();
	for (u32 i = 0, c = 0; i<c_col.size(); ++i)
	{
		if (!c_col[i]->shown) continue;
		list->InsertColumn(c++, fmt::FromUTF8(c_col[i]->name), 0, c_col[i]->width);
	}
}

void ColumnsArr::ShowData(wxListView* list)
{
	list->DeleteAllItems();
	list->SetImageList(m_img_list, wxIMAGE_LIST_SMALL);
	for (int c = 1; c<list->GetColumnCount(); ++c)
	{
		Column* col = GetColumnByPos(c);

		if (!col)
		{
			LOG_ERROR(HLE, "Columns loaded with error!");
			return;
		}

		for (u32 i = 0; i<col->data.size(); ++i)
		{
			if (list->GetItemCount() <= (int)i)
			{
				list->InsertItem(i, wxEmptyString);
				list->SetItemData(i, i);
			}
			list->SetItem(i, c, fmt::FromUTF8(col->data[i]));
			if (m_icon_indexes[i] >= 0)
				list->SetItemColumnImage(i, 0, m_icon_indexes[i]);
		}
	}
}

void ColumnsArr::LoadSave(bool isLoad, const std::string& path, wxListView* list)
{
	if (isLoad)
	{
		Init();
	}
	else if (list)
	{
		for (int c = 0; c < list->GetColumnCount(); ++c)
		{
			Column* col = GetColumnByPos(c);
			if (col)
				col->width = list->GetColumnWidth(c);
		}
	}

	auto&& cfg = g_gui_cfg["GameViewer"];

	for (auto& column : m_columns)
	{
		auto&& c_cfg = cfg[column.name];

		if (isLoad)
		{
			std::tie(column.pos, column.width) = c_cfg.as<std::pair<u32, u32>>(std::make_pair(column.def_pos, column.def_width));

			column.shown = true;
		}
		else //if (column.shown)
		{
			c_cfg = std::make_pair(column.pos, column.width);
		}
	}

	if (isLoad)
	{
		//check for errors
		for (u32 c1 = 0; c1 < m_columns.size(); ++c1)
		{
			for (u32 c2 = c1 + 1; c2 < m_columns.size(); ++c2)
			{
				if (m_columns[c1].pos == m_columns[c2].pos)
				{
					LOG_ERROR(HLE, "Columns loaded with error!");
					Init();
					return;
				}
			}
		}

		for (u32 p = 0; p < m_columns.size(); ++p)
		{
			bool ishas = false;
			for (u32 c = 0; c < m_columns.size(); ++c)
			{
				if (m_columns[c].pos != p)
					continue;

				ishas = true;
				break;
			}

			if (!ishas)
			{
				LOG_ERROR(HLE, "Columns loaded with error!");
				Init();
				return;
			}
		}
	}
	else
	{
		save_gui_cfg();
	}
}
