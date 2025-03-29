#pragma once

#include "gui_game_info.h"

#include <QWidget>
#include <QComboBox>
#include <QSplitter>

#include <mutex>

class game_list;
class gui_settings;

class savestate_manager_dialog : public QWidget
{
	Q_OBJECT

public:
	explicit savestate_manager_dialog(std::shared_ptr<gui_settings> gui_settings, const std::vector<game_info>& games);
	~savestate_manager_dialog() override;
	void RepaintUI(bool restore_layout = true);

public Q_SLOTS:
	void HandleRepaintUiRequest();

private Q_SLOTS:
	void ResizeGameIcons();
	void ShowSavestateTableContextMenu(const QPoint& pos);
	void ShowGameTableContextMenu(const QPoint& pos);

Q_SIGNALS:
	void GameIconReady(int index, const QPixmap& pixmap);
	void RequestBoot(const std::string& path);

private:
	struct savestate_data
	{
		QString name;
		QString path;
		QDateTime date;
		bool is_compatible = false;
	};

	struct game_savestates_data
	{
		std::vector<savestate_data> savestates;
		std::string title_id;
		std::string game_name;
		std::string game_icon_path;
		std::string dir_path;
	};

	bool LoadSavestateFolderToDB(std::unique_ptr<game_savestates_data>&& game_savestates);
	void StartSavestateLoadThreads();

	void PopulateGameTable();
	void PopulateSavestateTable();

	void ReadjustGameTable() const;
	void ReadjustSavestateTable() const;

	void WaitAndAbortGameRepaintThreads();

	void closeEvent(QCloseEvent *event) override;
	bool eventFilter(QObject *object, QEvent *event) override;

	std::shared_ptr<gui_settings> m_gui_settings;

	std::vector<game_info> m_game_info;
	std::vector<std::unique_ptr<game_savestates_data>> m_savestate_db; //! Holds all the savestate information.
	std::mutex m_savestate_db_mtx;
	QComboBox* m_game_combo; //! Lets you choose a game
	QSplitter* m_splitter; //! Contains the game and savestate tables
	game_list* m_savestate_table; //! UI element to display savestate stuff.
	game_list* m_game_table; //! UI element to display games.

	QList<QAction*> m_savestate_column_acts;
	QList<QAction*> m_game_column_acts;

	int m_game_icon_size_index = 25;
	QSize m_game_icon_size = QSize(m_game_icon_size_index, m_game_icon_size_index);
	bool m_save_game_icon_size = false;
	QSlider* m_game_icon_slider = nullptr;
	QColor m_game_icon_color;
};
