#pragma once
#include <wx/listctrl.h>
#include "Emu/GameInfo.h"

struct Column
{
	u32 pos;
	u32 width;
	bool shown;
	wxArrayString data;

	const wxString name;
	const u32 def_pos;
	const u32 def_width;

	Column(const u32 _def_pos, const u32 _def_width, const wxString& _name)
		: def_pos(_def_pos)
		, def_width(_def_width)
		, pos(_def_pos)
		, width(_def_width)
		, shown(true)
		, name(_name)
	{
		data.Clear();
	}

};

struct ColumnsArr
{
	ArrayF<Column> m_columns;

	ColumnsArr()
	{
		Init();
	}

	ArrayF<Column>* GetSortedColumnsByPos()
	{
		static ArrayF<Column>& arr = *new ArrayF<Column>(); arr.ClearF();
		for(u32 pos=0; pos<m_columns.GetCount(); pos++)
		{
			for(u32 c=0; c<m_columns.GetCount(); ++c)
			{
				if(m_columns[c].pos != pos) continue;
				arr.Add(m_columns[c]);
			}
		}

		return &arr;
	}

	Column* GetColumnByPos(u32 pos)
	{
		ArrayF<Column>& columns = *GetSortedColumnsByPos();
		for(u32 c=0; c<columns.GetCount(); ++c)
		{
			if(!columns[c].shown)
			{
				pos++;
				continue;
			}
			if(columns[c].pos != pos) continue;
			return &columns[c];
		}

		return NULL;
	}

public:
	Column* m_col_name;
	Column* m_col_serial;
	Column* m_col_fw;
	Column* m_col_app_ver;
	Column* m_col_category;
	Column* m_col_path;

	void Init()
	{
		m_columns.Clear();

		#define ADD_COLUMN(x, w, n) x = new Column(m_columns.GetCount(), w, n);	m_columns.Add(x);
			ADD_COLUMN(m_col_name, 160, "Name");
			ADD_COLUMN(m_col_serial, 85, "Serial");
			ADD_COLUMN(m_col_fw, 55, "FW");
			ADD_COLUMN(m_col_app_ver, 55, "App version");
			ADD_COLUMN(m_col_category, 55, "Category");
			ADD_COLUMN(m_col_path, 160, "Path");
		#undef ADD_COLUMN
	}

	void Update(ArrayF<GameInfo>& game_data)
	{
		m_col_name->data.Clear();
		m_col_serial->data.Clear();
		m_col_fw->data.Clear();
		m_col_app_ver->data.Clear();
		m_col_category->data.Clear();
		m_col_path->data.Clear();

		if(m_columns.GetCount() == 0) return;

		for(uint i=0; i<game_data.GetCount(); ++i)
		{
			GameInfo& game = game_data[i];
			m_col_name->data.Add(game.name);
			m_col_serial->data.Add(game.serial);
			m_col_fw->data.Add(game.fw);
			m_col_app_ver->data.Add(game.app_ver);
			m_col_category->data.Add(game.category);
			m_col_path->data.Add(game.root);
		}
	}

	void Show(wxListView* list)
	{
		list->DeleteAllColumns();
		ArrayF<Column>& c_col = *GetSortedColumnsByPos();
		for(u32 i=0, c=0; i<c_col.GetCount(); ++i)
		{
			if(!c_col[i].shown) continue;
			list->InsertColumn(c++, c_col[i].name, 0, c_col[i].width);
		}
	}

	void ShowData(wxListView* list)
	{
		list->DeleteAllItems();
		for(int c=0; c<list->GetColumnCount(); ++c)
		{
			Column* col = GetColumnByPos(c);

			if(!col)
			{
				ConLog.Error("Columns loaded with error!");
				return;
			}

			for(u32 i=0; i<col->data.GetCount(); ++i)
			{
				if(list->GetItemCount() <= (int)i) list->InsertItem(i, wxEmptyString);
				list->SetItem(i, c, col->data[i]);
			}			
		}
	}

	void LoadSave(bool isLoad, const wxString& path, wxListView* list = NULL)
	{
		if(isLoad) Init();
		else if(list)
		{
			for(int c=0; c<list->GetColumnCount(); ++c)
			{
				Column* col = GetColumnByPos(c);
				if(col) col->width = list->GetColumnWidth(c);
			}
		}

	#define ADD_COLUMN(v, dv, t, n, isshown) \
		{ \
			IniEntry<t> ini; \
			ini.Init(m_columns[i].name + "_" + n, path); \
			if(isLoad) m_columns[i].v = ini.LoadValue(dv); \
			else if(isshown ? m_columns[i].shown : 1) \
			{ \
				ini.SetValue(m_columns[i].v); \
				ini.Save(); \
			} \
		}

		for(u32 i=0; i<m_columns.GetCount(); ++i)
		{
			ADD_COLUMN(pos, m_columns[i].def_pos, int, "position", 1);
			ADD_COLUMN(width, m_columns[i].def_width, int, "width", 1);
			ADD_COLUMN(shown, true, bool, "shown", 0);
		}

		if(isLoad)
		{
			//check for errors
			for(u32 c1=0; c1<m_columns.GetCount(); ++c1)
			{
				for(u32 c2=c1+1; c2<m_columns.GetCount(); ++c2)
				{
					if(m_columns[c1].pos == m_columns[c2].pos)
					{
						ConLog.Error("Columns loaded with error!");
						Init();
						return;
					}
				}
			}

			for(u32 p=0; p<m_columns.GetCount(); ++p)
			{
				bool ishas = false;
				for(u32 c=0; c<m_columns.GetCount(); ++c)
				{
					if(m_columns[c].pos != p) continue;
					ishas = true;
					break;
				}

				if(!ishas)
				{
					ConLog.Error("Columns loaded with error!");
					Init();
					return;
				}
			}
		}

	#undef ADD_COLUMN
	}
};

class GameViewer : public wxPanel
{
	wxListView* m_game_list;
	wxString m_path;
	wxArrayString m_games;
	ArrayF<GameInfo> m_game_data;
	ColumnsArr m_columns;

public:
	GameViewer(wxWindow* parent);
	~GameViewer();

	void DoResize(wxSize size);

	void LoadGames();
	void LoadPSF();
	void ShowData();

	void SaveSettings();
	void LoadSettings();

	void Refresh();

private:
	virtual void DClick(wxListEvent& event);
};