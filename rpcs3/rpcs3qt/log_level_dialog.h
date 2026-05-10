#pragma once

#include <QDialog>
#include <QTableWidget>

class emu_settings;

class log_level_dialog : public QDialog
{
public:
	log_level_dialog(QWidget* parent, std::shared_ptr<emu_settings> emu_settings);
	virtual ~log_level_dialog();

private:
	void reload_page();

	std::shared_ptr<emu_settings> m_emu_settings;
	QTableWidget* m_table = nullptr;
};
