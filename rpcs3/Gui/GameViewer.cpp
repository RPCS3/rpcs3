#include "stdafx_gui.h"
#include "Utilities/AutoPause.h"
#include "Utilities/Log.h"
#include "Utilities/rFile.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsDir.h"
#include "Emu/FS/vfsFile.h"
#include "GameViewer.h"
#include "Loader/PSF.h"
#include <wx/dir.h>

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

class WxDirDeleteTraverser : public wxDirTraverser
{
public:
	virtual wxDirTraverseResult OnFile(const wxString& filename)
	{
		if (!wxRemoveFile(filename)){
			LOG_ERROR(HLE, "Couldn't delete File: %s", fmt::ToUTF8(filename).c_str());
		}
		return wxDIR_CONTINUE;
	}
	virtual wxDirTraverseResult OnDir(const wxString& dirname)
	{
		wxDir dir(dirname);
		dir.Traverse(*this);
		if (!wxRmDir(dirname)){
			//this get triggered a few times while clearing folders
			//but if this gets reimplented we should probably warn
			//if directories can't be removed
		}
		return wxDIR_CONTINUE;
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
	m_popup->Append(0, _T("Remove Game"));

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
	vfsDir dir(m_path);
	LOG_NOTICE(HLE, "path: %s", m_path.c_str());
	if(!dir.IsOpened()) return;

	m_games.clear();

	for(const DirEntryInfo* info : dir)
	{
		if(info->flags & DirEntry_TypeDir)
		{
			m_games.push_back(info->name);
		}
	}
	dir.Close();

	//ConLog.Write("path: %s", m_path.wx_str());
	//ConLog.Write("folders count: %d", m_games.GetCount());
}

void GameViewer::LoadPSF()
{
	m_game_data.clear();
	for(uint i=0; i<m_games.size(); ++i)
	{
		vfsFile f;
		std::string sfo;
		std::string sfb;

		sfb = m_path + m_games[i] + "/PS3_DISC.SFB"; 

		if (!f.Open(sfb))
			sfo = m_path + m_games[i] + "/PARAM.SFO";
		else
			sfo = m_path + m_games[i] + "/PS3_GAME/PARAM.SFO";

		if(!f.Open(sfo))
			continue;

		PSFLoader psf(f);
		if(!psf.Load(false))
			continue;

		GameInfo game;
		game.icon_path = wxGetCwd() + m_path + m_games[i] + "/ICON0.PNG";
		game.root = m_games[i];
		game.serial = psf.GetString("TITLE_ID");
		game.name = psf.GetString("TITLE");
		game.app_ver = psf.GetString("APP_VER");
		game.category = psf.GetString("CATEGORY");
		game.fw = psf.GetString("PS3_SYSTEM_VER");
		game.parental_lvl = psf.GetInteger("PARENTAL_LEVEL");
		game.resolution = psf.GetInteger("RESOLUTION");
		game.sound_format = psf.GetInteger("SOUND_FORMAT");
		
		if(game.serial.length() == 9)
			game.serial = game.serial.substr(0, 4) + "-" + game.serial.substr(4, 5);

		if (game.category.substr(0, 2) == "HG")
			game.category = "HDD Game";
		else if (game.category.substr(0, 2) == "DG")
			game.category = "Disc Game";
			
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
	if(i < 0) return;

	const std::string& path = m_path + m_game_data[i].root;

	Emu.Stop();

	Debug::AutoPause::getInstance().Reload();

	Emu.GetVFS().Init("/");
	std::string local_path;
	if (Emu.GetVFS().GetDevice(path, local_path) && !Emu.BootGame(local_path)) {
		LOG_ERROR(HLE, "Boot error: elf not found! [%s]", path.c_str());
		return;
	}

	if (Ini.HLEAlwaysStart.GetValue() && Emu.IsReady()) {
		Emu.Run();
	}
}

void GameViewer::RightClick(wxListEvent& event)
{
	m_popup->Destroy(m_popup->FindItemByPosition(0));
	
	wxMenuItem *pMenuItemA = m_popup->Append(event.GetIndex(), _T("Remove Game"));
	Bind(wxEVT_MENU, &GameViewer::RemoveGame, this, event.GetIndex());
	PopupMenu(m_popup, event.GetPoint());
}

void GameViewer::RemoveGame(wxCommandEvent& event)
{
	wxString GameName = this->GetItemText(event.GetId(), 5);

	// TODO: VFS is only available at emulation time, this is a temporary solution to locate the game
	Emu.GetVFS().Init(m_path);

	vfsDir dir(m_path);	
	if (!dir.IsOpened())
		return;
	const std::string sPath = dir.GetPath().erase(0, 1);
	const std::string sGameFolder = GameName.mb_str().data();
	const std::string localPath = sPath + sGameFolder;

	Emu.GetVFS().UnMountAll();

	if (!rExists(localPath))
		return;
	//TODO: Replace wxWidgetsSpecific filesystem stuff?
	WxDirDeleteTraverser deleter;
	wxDir localDir(localPath);
	localDir.Traverse(deleter);

	Refresh();
}
