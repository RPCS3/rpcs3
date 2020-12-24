#pragma once

#include <QComboBox>
#include <QDialog>
#include <QTabWidget>

#include <vector>

class raw_mouse_settings_dialog : public QDialog
{
	Q_OBJECT

public:
	raw_mouse_settings_dialog(QWidget* parent = nullptr);

private:
	void add_tabs(QTabWidget* tabs);

	void reset_config();

	std::vector<std::vector<QComboBox*>> m_combos;
};
