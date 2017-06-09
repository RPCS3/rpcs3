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
};

class game_list_frame : public QDockWidget {
	Q_OBJECT

public:
	explicit game_list_frame(std::shared_ptr<gui_settings> settings, Render_Creator r_Creator, QWidget *parent = nullptr);
	~game_list_frame();
	void Refresh(const bool fromDrive);
	void ToggleCategoryFilter(QString category, bool show);

	/** Loads from settings. Public so that main frame can easily reset these settings if needed. */
	void LoadSettings();

	/** Saves settings. Public so that main frame can save this when a caching of column widths is needed for settings backup */
	void SaveSettings();

public slots:
	/** Resize Gamelist Icons to size */
	void ResizeIcons(QSize size);
	void SetListMode(bool isList);

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
protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event);
	void resizeEvent(QResizeEvent *event);
private:
	game_list_grid* MakeGrid(uint maxCols, const QSize& image_size, bool fromDrive);
	void LoadGames();
	void LoadPSF();
	void FilterData();

	void PopulateUI();
	QImage* GetImage(const std::string& path, const QSize& size);

	// Which widget we are displaying depends on if we are in grid or list mode.
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

	// TODO: Reorganize this into a sensible order for private variables.
	std::shared_ptr<gui_settings> xgui_settings;

	int m_sortColumn;
	Qt::SortOrder m_colSortOrder;
	bool m_isListLayout = true;
	std::vector<std::string> m_games;
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
