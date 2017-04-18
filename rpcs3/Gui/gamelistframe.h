#ifndef GAMELISTFRAME_H
#define GAMELISTFRAME_H

#include <QDockWidget>
#include <QTableWidget>
#include <QListView>
#include <QListWidget>

#include "Utilities\types.h"
#include "Emu\GameInfo.h"

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

	QListWidget* m_img_list;
	std::vector<int> m_icon_indexes;
	
	void Init();
	
	void Update(const std::vector<GameInfo>& game_data);
	
	void Show(QDockWidget* list);
	
	void ShowData(QDockWidget* list);
	
	void LoadSave(bool isLoad, const std::string& path, QDockWidget* list = NULL);
};

class GameListFrame : public QDockWidget {
	Q_OBJECT

	int m_sortColumn;
	bool m_sortAscending;
	std::vector<std::string> m_games;
	std::vector<GameInfo> m_game_data;
	ColumnsArr m_columns;

public:
	explicit GameListFrame(QWidget *parent = 0);
	~GameListFrame();

private slots:
	void Boot();
	void Configure();
	void RemoveGame();
	void RemoveCustomConfiguration();
	void OpenGameFolder();
	void OpenConfigFolder();

signals:
	void GameListFrameClosed();
protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event);
private:
	QTableWidget *gameList;
	void ShowContextMenu(QPoint &pos);
	void doubleClickedSlot(QModelIndex index);
	void CreateActions();
	void LoadGames();
	void LoadPSF();
	void ShowData();
	void Refresh();
	void SaveSettings();
	void LoadSettings();

	QAction *bootAct;
	QAction *configureAct;
	QAction *removeGameAct;
	QAction *removeCustomConfigurationAct;
	QAction *openGameFolderAct;
	QAction *openConfigFolderAct;
};

#endif // GAMELISTFRAME_H
