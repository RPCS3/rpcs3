#pragma once

#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/Cell/Modules/cellSaveData.h"
#include "gui_settings.h"

#include <QDialog>
#include <QTableWidget>

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
	explicit save_manager_dialog(std::shared_ptr<gui_settings> gui_settings, std::string dir = "", QWidget* parent = nullptr);
private Q_SLOTS:
	void OnEntryInfo();
	void OnEntryRemove();
	void OnEntriesRemove();
	void OnSort(int logicalIndex);
private:
	void Init(std::string dir);
	void UpdateList();

	void ShowContextMenu(const QPoint &pos);

	void closeEvent(QCloseEvent* event) override;

	QTableWidget* m_list;
	std::string m_dir;
	std::vector<SaveDataEntry> m_save_entries;

	std::shared_ptr<gui_settings> m_gui_settings;

	QMenu* m_sort_options;

	int m_sort_column;
	bool m_sort_ascending;
};
