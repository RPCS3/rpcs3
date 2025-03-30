#pragma once

#include "game_list.h"

#include "Emu/Cell/Modules/cellSaveData.h"

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QPixmap>

class gui_settings;
class persistent_settings;
class video_label;

class save_manager_dialog : public QDialog
{
	Q_OBJECT
public:
	/**
	* Class which will handle the managing of saves from all games.
	* You may think I should just modify save_data_list_dialog.  But, that wouldn't be ideal long term since that class will be refactored into an overlay.
	* Plus, there's the added complexity of an additional way in which the dialog will spawn differently.
	* There'll be some duplicated code.  But, in the future, there'll be no duplicated code. So, I don't care.
	*/
	explicit save_manager_dialog(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<persistent_settings> persistent_settings, std::string dir = "", QWidget* parent = nullptr);
public Q_SLOTS:
	void HandleRepaintUiRequest();
private Q_SLOTS:
	void OnEntryRemove(int row, bool user_interaction);
	void OnEntriesRemove();
	void OnSort(int logicalIndex);
	void SetIconSize(int size);
	void UpdateDetails();
	void text_changed(const QString& text);

Q_SIGNALS:
	void IconReady(int index, const QPixmap& new_icon);

private:
	void Init();
	void UpdateList();
	void UpdateIcons();
	void ShowContextMenu(const QPoint& pos);
	void WaitForRepaintThreads(bool abort);

	void closeEvent(QCloseEvent* event) override;

	std::vector<SaveDataEntry> GetSaveEntries(const std::string& base_dir);

	game_list* m_list = nullptr;
	std::string m_dir;
	std::vector<SaveDataEntry> m_save_entries;

	std::shared_ptr<gui_settings> m_gui_settings;
	std::shared_ptr<persistent_settings> m_persistent_settings;

	int m_sort_column = 1;
	bool m_sort_ascending = true;
	QSize m_icon_size;
	QColor m_icon_color;

	video_label* m_details_icon = nullptr;
	QLabel* m_details_title = nullptr;
	QLabel* m_details_subtitle = nullptr;
	QLabel* m_details_modified = nullptr;
	QLabel* m_details_details = nullptr;
	QLabel* m_details_note = nullptr;

	QPushButton* m_button_delete = nullptr;
	QPushButton* m_button_folder = nullptr;
};
