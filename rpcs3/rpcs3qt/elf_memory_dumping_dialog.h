#pragma once

#include "util/types.hpp"

#include <QListWidget>
#include <QLineEdit>
#include <QDialog>

#include <memory>

class gui_settings;

class elf_memory_dumping_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit elf_memory_dumping_dialog(u32 ppu_debugger_pc, std::shared_ptr<gui_settings> _gui_settings, QWidget* parent = nullptr);

protected:
	void add_new_segment();
	void remove_segment();
	void save_to_file();

	std::shared_ptr<gui_settings> m_gui_settings;

	// UI variables needed in higher scope
	QListWidget* m_seg_list = nullptr;

	QLineEdit* m_ls_address_input = nullptr;
	QLineEdit* m_segment_size_input = nullptr;
	QLineEdit* m_ppu_address_input = nullptr;
	QLineEdit* m_segment_flags_input = nullptr;
};
