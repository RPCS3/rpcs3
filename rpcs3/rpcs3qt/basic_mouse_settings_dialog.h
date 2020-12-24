#pragma once

#include <QComboBox>
#include <QDialog>

#include <vector>

class basic_mouse_settings_dialog : public QDialog
{
	Q_OBJECT

public:
	basic_mouse_settings_dialog(QWidget* parent = nullptr);

private:
	void reset_config();

	std::vector<QComboBox*> m_combos;
};
