#pragma once

#include "Loader/TROPUSR.h"

#include <QWidget>
#include <QComboBox>
#include <QFutureWatcher>
#include <QLabel>
#include <QPixmap>
#include <QTableWidget>
#include <QSlider>
#include <QSplitter>

#include <memory>
#include <mutex>
#include <unordered_map>

class game_list;
class gui_settings;
class TROPUSRLoader;

struct GameTrophiesData
{
	std::unique_ptr<TROPUSRLoader> trop_usr;
	trophy_xml_document trop_config; // I'd like to use unique but the protocol inside of the function passes around shared pointers..
	std::unordered_map<int, QPixmap> trophy_images; // Cache trophy images to avoid loading from disk as much as possible.
	std::unordered_map<int, QString> trophy_image_paths;
	std::string game_name;
	std::string path;
};

class trophy_manager_dialog : public QWidget
{
	Q_OBJECT

public:
	explicit trophy_manager_dialog(std::shared_ptr<gui_settings> gui_settings);
	~trophy_manager_dialog() override;
	void RepaintUI(bool restore_layout = true);

public Q_SLOTS:
	void HandleRepaintUiRequest();

private Q_SLOTS:
	void ResizeGameIcons();
	void ResizeTrophyIcons();
	void ApplyFilter();
	void ShowTrophyTableContextMenu(const QPoint& pos);
	void ShowGameTableContextMenu(const QPoint& pos);

Q_SIGNALS:
	void GameIconReady(int index, const QPixmap& pixmap);
	void TrophyIconReady(int index, const QPixmap& pixmap);

private:
	/** Loads a trophy folder.
	Returns true if successful.  Does not attempt to install if failure occurs, like sceNpTrophy.
	*/
	bool LoadTrophyFolderToDB(const std::string& trop_name);

	/** Populate the trophy database (multithreaded). */
	void StartTrophyLoadThreads();

	/** Fills game table with information.
	Takes results from LoadTrophyFolderToDB and puts it into the UI.
	*/
	void PopulateGameTable();

	/** Fills trophy table with information.
	Takes results from LoadTrophyFolderToDB and puts it into the UI.
	*/
	void PopulateTrophyTable();

	void ReadjustGameTable() const;
	void ReadjustTrophyTable() const;

	void WaitAndAbortGameRepaintThreads();
	void WaitAndAbortTrophyRepaintThreads();

	void closeEvent(QCloseEvent *event) override;
	bool eventFilter(QObject *object, QEvent *event) override;

	static QDateTime TickToDateTime(u64 tick);
	static u64 DateTimeToTick(QDateTime date_time);

	std::shared_ptr<gui_settings> m_gui_settings;

	std::vector<std::unique_ptr<GameTrophiesData>> m_trophies_db; //! Holds all the trophy information.
	std::mutex m_trophies_db_mtx;
	QComboBox* m_game_combo; //! Lets you choose a game
	QLabel* m_game_progress; //! Shows you the current game's progress
	QSplitter* m_splitter; //! Contains the game and trophy tables
	game_list* m_trophy_table; //! UI element to display trophy stuff.
	game_list* m_game_table; //! UI element to display games.

	QList<QAction*> m_trophy_column_acts;
	QList<QAction*> m_game_column_acts;

	bool m_show_hidden_trophies = false;
	bool m_show_unlocked_trophies = true;
	bool m_show_locked_trophies = true;
	bool m_show_bronze_trophies = true;
	bool m_show_silver_trophies = true;
	bool m_show_gold_trophies = true;
	bool m_show_platinum_trophies = true;
	std::string m_trophy_dir;

	int m_icon_height = 75;
	bool m_save_icon_height = false;
	QSlider* m_icon_slider = nullptr;

	int m_game_icon_size_index = 25;
	QSize m_game_icon_size = QSize(m_game_icon_size_index, m_game_icon_size_index);
	bool m_save_game_icon_size = false;
	QSlider* m_game_icon_slider = nullptr;
	QColor m_game_icon_color;
};
