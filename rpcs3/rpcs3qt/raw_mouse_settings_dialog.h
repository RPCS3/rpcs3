#pragma once

#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QTabWidget>

#include <vector>

class raw_mouse_settings_dialog : public QDialog
{
	Q_OBJECT

public:
	raw_mouse_settings_dialog(QWidget* parent = nullptr);
	virtual ~raw_mouse_settings_dialog();

private:
	void add_tabs(QTabWidget* tabs);

	void reset_config();

	std::vector<QComboBox*> m_device_combos;
	std::vector<QDoubleSpinBox*> m_accel_spin_boxes;
	std::vector<std::vector<QComboBox*>> m_combos;
};
