#pragma once

#include "Emu/GameInfo.h"

struct Column
{
	u32 pos;
	u32 width;
	bool shown;
	std::vector<std::string> data;

	const std::string name;
	const u32 def_pos;
	const u32 def_width;

	Column(const u32 _def_pos, const u32 _def_width, const std::string& _name)
		: def_pos(_def_pos)
		, def_width(_def_width)
		, pos(_def_pos)
		, width(_def_width)
		, shown(true)
		, name(_name)
	{
		data.clear();
	}

};

struct ColumnsArr
{
	std::vector<Column> m_columns;

	ColumnsArr();

	std::vector<Column*> GetSortedColumnsByPos();

	Column* GetColumnByPos(u32 pos);

public:
	Column* m_col_icon;
	Column* m_col_name;
	Column* m_col_serial;
	Column* m_col_fw;
	Column* m_col_app_ver;
	Column* m_col_category;
	Column* m_col_path;

	wxImageList* m_img_list;
	std::vector<int> m_icon_indexes;

	void Init();

	void Update(const std::vector<GameInfo>& game_data);

	void Show(wxListView* list);

	void ShowData(wxListView* list);

	void LoadSave(bool isLoad, const std::string& path, wxListView* list = NULL);
};

class GameViewer : public wxListView
{
	int m_sortColumn;
	bool m_sortAscending;
	std::vector<std::string> m_games;
	std::vector<GameInfo> m_game_data;
	ColumnsArr m_columns;
	wxMenu* m_popup;

public:
	GameViewer(wxWindow* parent);
	~GameViewer();

	void InitPopupMenu();

	void DoResize(wxSize size);

	void LoadGames();
	void LoadPSF();
	void ShowData();

	void SaveSettings();
	void LoadSettings();

	void Refresh();
	void BootGame(wxCommandEvent& event);
	void ConfigureGame(wxCommandEvent& event);
	void RemoveGame(wxCommandEvent& event);
	void RemoveGameConfig(wxCommandEvent& event);
	void OpenGameFolder(wxCommandEvent& event);
	void OpenConfigFolder(wxCommandEvent& event);

private:
	virtual void DClick(wxListEvent& event);
	virtual void OnColClick(wxListEvent& event);
	virtual void RightClick(wxListEvent& event);
};
