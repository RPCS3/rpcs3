#pragma once

#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/Cell/Modules/cellSaveData.h"

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
	explicit save_manager_dialog(std::string dir = "", QWidget* parent = nullptr);
private Q_SLOTS:
	void OnEntryInfo();
	void OnEntryRemove();
private:
	void Init(std::string dir);
	void UpdateList();

	void OnSort(int id);

	void ShowContextMenu(const QPoint &pos);

	QTableWidget* m_list;
	std::string m_dir;
	std::vector<SaveDataEntry> m_save_entries;

	QMenu* m_sort_options;

	int m_sortColumn;
	bool m_sortAscending;

	QAction* saveIDAct;
	QAction* titleAct;
	QAction* subtitleAct;
	QAction* removeAct;
	QAction* infoAct;
};
