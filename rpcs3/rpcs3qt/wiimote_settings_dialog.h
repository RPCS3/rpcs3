#pragma once

#include <QDialog>
#include "ui_wiimote_settings_dialog.h"

class wiimote_settings_dialog : public QDialog
{
	Q_OBJECT
public:
	explicit wiimote_settings_dialog(QWidget* parent = nullptr);
	~wiimote_settings_dialog();

private:
	std::unique_ptr<Ui::wiimote_settings_dialog> ui;
	std::vector<QComboBox*> m_boxes;
	void update_list();
	void update_state();
	void populate_mappings();
	void apply_mappings();
	void restore_defaults();
};
