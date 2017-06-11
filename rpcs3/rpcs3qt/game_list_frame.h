#ifndef GAMELISTFRAME_H
#define GAMELISTFRAME_H

#include "stdafx.h"
#include "Emu/GameInfo.h"

#include "game_list_grid.h"
#include "gui_settings.h"
#include "emu_settings.h"

#include <QDockWidget>
#include <QList>
#include <QTableWidget>
#include <QMainWindow>
#include <QToolBar>
#include <QLineEdit>
#include <QStackedWidget>

#include <memory>

namespace category
{
	const QString hdd_Game    = QObject::tr("HDD Game");
	const QString disc_Game   = QObject::tr("Disc Game");
	const QString home        = QObject::tr("Home");
	const QString audio_Video = QObject::tr("Audio/Video");
	const QString game_Data   = QObject::tr("Game Data");
	const QString unknown     = QObject::tr("Unknown");
}

/* Having the icons associated with the game info simplifies logic internally */
typedef struct GUI_GameInfo
{
	GameInfo info;
	QImage icon;
	QPixmap pxmap;
};

typedef struct Tool_Bar_Button
{
	QAction* action;
	QIcon colored;
	QIcon gray;
};

class game_list_frame : public QDockWidget {
	Q_OBJECT

public:
	explicit game_list_frame(std::shared_ptr<gui_settings> settings, Render_Creator r_Creator, QWidget *parent = nullptr);
	~game_list_frame();
	void Refresh(const bool fromDrive = false);
	void ToggleCategoryFilter(QString category, bool show);

	/** Loads from settings. Public so that main frame can easily reset these settings if needed. */
	void LoadSettings();

	/** Saves settings. Public so that main frame can save this when a caching of column widths is needed for settings backup */
	void SaveSettings();

public slots:
	/** Resize Gamelist Icons to size */
	void ResizeIcons(const QSize& size, const int& idx);
	void SetListMode(const bool& isList);
	void SetToolBarVisible(const bool& showToolBar);
	void SetCategoryActIcon(const int& id, const bool& active);

private slots:
	void Boot(int row);
	void RemoveCustomConfiguration(int row);
	void OnColClicked(int col);

	void ShowContextMenu(const QPoint &pos);
	void ShowSpecifiedContextMenu(const QPoint &pos, int index); // Different name because the notation for overloaded connects is messy
	void doubleClickedSlot(const QModelIndex& index);
signals:
	void game_list_frameClosed();
	void RequestIconPathSet(const std::string path);
	void RequestAddRecentGame(const q_string_pair& entry);
	void RequestIconSizeActSet(const int& idx);
	void RequestListModeActSet(const bool& isList);
	void RequestCategoryActSet(const int& id);
protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event);
	void resizeEvent(QResizeEvent *event);
private:
	game_list_grid* MakeGrid(uint maxCols, const QSize& image_size);
	void FilterData();

	void PopulateGameList();
	bool SearchMatchesApp(const std::string& name, const std::string& serial);

	// Which widget we are displaying depends on if we are in grid or list mode.
	QMainWindow* m_Game_Dock;
	QStackedWidget* m_Central_Widget;
	QToolBar* m_Tool_Bar;
	QLineEdit* m_Search_Bar;
	QSlider* m_Slider_Size;
	QTableWidget *gameList;
	std::unique_ptr<game_list_grid> m_xgrid;

	// Actions regarding showing/hiding columns
	QAction* showIconColAct;
	QAction* showNameColAct;
	QAction* showSerialColAct;
	QAction* showFWColAct;
	QAction* showAppVersionColAct;
	QAction* showCategoryColAct;
	QAction* showPathColAct;

	QList<QAction*> columnActs;

	// Actions regarding showing/hiding categories
	Tool_Bar_Button m_catActHDD;
	Tool_Bar_Button m_catActDisc;
	Tool_Bar_Button m_catActHome;
	Tool_Bar_Button m_catActGameData;
	Tool_Bar_Button m_catActAudioVideo;
	Tool_Bar_Button m_catActUnknown;

	QList<Tool_Bar_Button> m_categoryButtons;

	QActionGroup* m_categoryActs;

	// Actions regarding switching list modes
	Tool_Bar_Button m_modeActList;
	Tool_Bar_Button m_modeActGrid;

	QActionGroup* m_modeActs;

	// TODO: Reorganize this into a sensible order for private variables.
	std::shared_ptr<gui_settings> xgui_settings;

	int m_sortColumn;
	Qt::SortOrder m_colSortOrder;
	bool m_isListLayout = true;
	bool m_showToolBar = true;
	std::vector<GUI_GameInfo> m_game_data;
	QSize m_Icon_Size;
	QString m_Icon_Size_Str;
	qreal m_Margin_Factor;
	qreal m_Text_Factor;
	QStringList m_categoryFilters;
	Render_Creator m_Render_Creator;

	uint m_games_per_row = 0;
};

#endif // GAMELISTFRAME_H
