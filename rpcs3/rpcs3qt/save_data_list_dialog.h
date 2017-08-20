#pragma once

// I just want the struct for the save data.
#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Memory/Memory.h"
#include "Emu/Cell/Modules/cellSaveData.h"

#include <QTableWidget>
#include <QDialog>
#include <QLabel>

//Display a list of SaveData. Would need to be initialized.
//Can also be used as a Save Data Chooser.
class save_data_list_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit save_data_list_dialog(const std::vector<SaveDataEntry>& entries, s32 focusedEntry, bool is_saving, QWidget* parent = nullptr);

	s32 GetSelection();
private Q_SLOTS:
	void OnEntryInfo();
private:
	void UpdateSelectionLabel(void);
	void UpdateList(void);
	void OnSort(int id);

	s32 m_selectedEntry;
	QLabel* selectedEntryLabel;

	QTableWidget* m_list;
	std::vector<SaveDataEntry> m_save_entries;

	int m_sortColumn;
	bool m_sortAscending;
};
