#include "stdafx.h"
#include "stdafx_gui.h"
#include "Utilities/AutoPause.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsDir.h"
#include "Emu/FS/vfsFile.h"
#include "GameViewer.h"
#include "Loader/PSF.h"
#include "SettingsDialog.h"

static const std::string m_class_name = "Game viewer";

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
	m_path = "/dev_hdd0/game/";
	m_popup = new wxMenu();

	Bind(wxEVT_LIST_ITEM_ACTIVATED, &GameViewer::DClick, this);
	Bind(wxEVT_LIST_COL_CLICK, &GameViewer::OnColClick, this);
	Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &GameViewer::RightClick, this);

	Refresh();
}

GameViewer::~GameViewer()
{
	SaveSettings();
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

	for (const auto info : vfsDir(m_path))
	{
		if(info->flags & DirEntry_TypeDir)
		{
			m_games.push_back(info->name);
		}
	}
}

void GameViewer::LoadPSF()
{
	m_game_data.clear();
	for (u32 i = 0; i < m_games.size(); ++i)
	{
		const std::string sfb = m_path + m_games[i] + "/PS3_DISC.SFB"; 
		const std::string sfo = m_path + m_games[i] + (Emu.GetVFS().ExistsFile(sfb) ? "/PS3_GAME/PARAM.SFO" : "/PARAM.SFO");

		if (!Emu.GetVFS().ExistsFile(sfo))
		{
			continue;
		}

		vfsFile f;

		if (!f.Open(sfo))
		{
			continue;
		}

		const PSFLoader psf(f);

		if (!psf)
		{
			continue;
		}

		// get local path from VFS...
		std::string local_path;
		Emu.GetVFS().GetDevice(m_path, local_path);

		GameInfo game;
		game.root = m_games[i];
		game.serial = psf.GetString("TITLE_ID");
		game.name = psf.GetString("TITLE");
		game.app_ver = psf.GetString("APP_VER");
		game.category = psf.GetString("CATEGORY");
		game.fw = psf.GetString("PS3_SYSTEM_VER");
		game.parental_lvl = psf.GetInteger("PARENTAL_LEVEL");
		game.resolution = psf.GetInteger("RESOLUTION");
		game.sound_format = psf.GetInteger("SOUND_FORMAT");
		
		if (game.serial.length() == 9)
		{
			game.serial = game.serial.substr(0, 4) + "-" + game.serial.substr(4, 5);
		}

		if (game.category.substr(0, 2) == "HG")
		{
			game.category = "HDD Game";
			game.icon_path = local_path + "/" + m_games[i] + "/ICON0.PNG";
		}
		else if (game.category.substr(0, 2) == "DG")
		{
			game.category = "Disc Game";
			game.icon_path = local_path + "/" + m_games[i] + "/PS3_GAME/ICON0.PNG";
		}
		else if (game.category.substr(0, 2) == "HM")
		{
			game.category = "Home";
			game.icon_path = local_path + "/" + m_games[i] + "/ICON0.PNG";
		}
		else if (game.category.substr(0, 2) == "AV")
		{
			game.category = "Audio/Video";
			game.icon_path = local_path + "/" + m_games[i] + "/ICON0.PNG";
		}
		else if (game.category.substr(0, 2) == "GD")
		{
			game.category = "Game Data";
			game.icon_path = local_path + "/" + m_games[i] + "/ICON0.PNG";
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
	Emu.GetVFS().Init("/");
	LoadGames();
	LoadPSF();
	ShowData();
	Emu.GetVFS().UnMountAll();
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

	const std::string& path = m_path + m_game_data[i].root;

	Emu.Stop();

	Debug::AutoPause::getInstance().Reload();

	Emu.GetVFS().Init("/");
	std::string local_path;
	if (Emu.GetVFS().GetDevice(path, local_path) && !Emu.BootGame(local_path))
	{
		LOG_ERROR(HLE, "Boot error: elf not found! [%s]", path.c_str());
		return;
	}

	if (rpcs3::config.misc.always_start.value() && Emu.IsReady())
	{
		Emu.Run();
	}
}

void GameViewer::RightClick(wxListEvent& event)
{
	for (wxMenuItem *item : m_popup->GetMenuItems()) {
		m_popup->Destroy(item);
	}
	
	wxMenuItem* boot_item = new wxMenuItem(m_popup, 0, _T("Boot"));
#if defined (_WIN32)
	// wxMenuItem::Set(Get)Font only available for the wxMSW port
	wxFont font = GetFont();
	font.SetWeight(wxFONTWEIGHT_BOLD);
	boot_item->SetFont(font);
#endif
	m_popup->Append(boot_item);
	m_popup->Append(1, _T("Configure"));
	m_popup->Append(2, _T("Remove game"));

	Bind(wxEVT_MENU, &GameViewer::BootGame, this, 0);
	Bind(wxEVT_MENU, &GameViewer::ConfigureGame, this, 1);
	Bind(wxEVT_MENU, &GameViewer::RemoveGame, this, 2);

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

	Emu.CreateConfig(m_game_data[i].serial);
	rpcs3::config_t custom_config { fs::get_config_dir() + "data/" + m_game_data[i].serial + "/settings.ini" };
	custom_config.load();
	LOG_NOTICE(LOADER, "Configure: '%s'", custom_config.path().c_str());
	SettingsDialog(this, &custom_config);
}

void GameViewer::RemoveGame(wxCommandEvent& event)
{
	long i = GetFirstSelected();
	if (i < 0) return;
	
	Emu.GetVFS().Init("/");
	Emu.GetVFS().DeleteAll(m_path + "/" + this->GetItemText(i, 6).ToStdString());
	Emu.GetVFS().UnMountAll();

	Refresh();
}
