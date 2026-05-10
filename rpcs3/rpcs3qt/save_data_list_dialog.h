#pragma once

#include "util/types.hpp"
#include "Emu/Cell/Modules/cellSaveData.h"
#include "Emu/RSX/Overlays/overlays.h"

#include <QTableWidget>
#include <QDialog>
#include <QLabel>

#include <vector>
#include <memory>

class gui_settings;
class persistent_settings;

//Display a list of SaveData. Would need to be initialized.
//Can also be used as a Save Data Chooser.
class save_data_list_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit save_data_list_dialog(const std::vector<SaveDataEntry>& entries, s32 focusedEntry, u32 op, vm::ptr<CellSaveDataListSet>, QWidget* parent = nullptr);

	s32 GetSelection() const;

protected:
	void mouseDoubleClickEvent(QMouseEvent* ev) override;

private Q_SLOTS:
	void OnEntryInfo();
	void OnSort(int logicalIndex);

private:
	void UpdateSelectionLabel();
	void UpdateList();

	s32 m_entry = rsx::overlays::user_interface::selection_code::new_save;
	QLabel* m_entry_label = nullptr;

	QTableWidget* m_list = nullptr;
	std::vector<SaveDataEntry> m_save_entries;

	std::shared_ptr<gui_settings> m_gui_settings;
	std::shared_ptr<persistent_settings> m_persistent_settings;

	int m_sort_column = 0;
	bool m_sort_ascending = true;
};
