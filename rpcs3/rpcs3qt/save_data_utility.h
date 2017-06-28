#pragma once

// I just want the struct for the save data.
#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/Cell/Modules/cellSaveData.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QTableWidget>
#include <QLabel>
#include <QHeaderView>
#include <QMenu>

//TODO: Implement function calls related to Save Data List.
//Those function calls may be needed to use this GUI.
//Currently this is only a stub.

//A stub for the sorting.
enum
{
	SAVE_DATA_LIST_SORT_BY_USERID
};

//Used to display the information of a savedata.
//Not sure about what information should be displayed.
class save_data_info_dialog :public QDialog
{
	Q_OBJECT

public:
	explicit save_data_info_dialog(const SaveDataEntry& save, QWidget* parent = nullptr);
private:
	void UpdateData();

	SaveDataEntry m_entry;
	QTableWidget* m_list;
};

//Simple way to show up the sort menu and other operations
//Like what you get when press Triangle on SaveData.
class save_data_manage_dialog :public QDialog
{
	Q_OBJECT

public:
	explicit save_data_manage_dialog(unsigned int* sort_type, SaveDataEntry& save, QWidget* parent = nullptr);
private Q_SLOTS:
	void OnInfo();
	void OnCopy();
	void OnDelete();
	void OnApplySort();
private:
	SaveDataEntry m_entry;

	QComboBox* m_sort_options;
	unsigned int* m_sort_type;
};

//Display a list of SaveData. Would need to be initialized.
//Can also be used as a Save Data Chooser.
class save_data_list_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit save_data_list_dialog(const std::vector<SaveDataEntry>& data, bool enable_manage, QWidget* parent = nullptr);

	s32 m_selectedEntry = -1;
private Q_SLOTS:
	void OnSelect();
	void OnManage();
	void OnEntryCopy();
	void OnEntryRemove();
	void OnEntryInfo();
	void ShowContextMenu(const QPoint &pos);
private:
	void LoadEntries(void);
	void UpdateList(void);
	void OnSort(int id);

	QTableWidget* m_list;
	std::vector<SaveDataEntry> m_save_entries;

	QMenu* m_sort_options;
	unsigned int m_sort_type;
	unsigned int m_sort_type_count;

	int m_sortColumn;
	bool m_sortAscending;

	QAction* userIDAct;
	QAction* titleAct;
	QAction* subtitleAct;
	QAction* copyAct;
	QAction* removeAct;
	QAction* infoAct;
};
