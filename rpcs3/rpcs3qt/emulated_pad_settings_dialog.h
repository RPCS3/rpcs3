#pragma once

#include <QComboBox>
#include <QDialog>
#include <QTabWidget>

#include <vector>

class emulated_pad_settings_dialog : public QDialog
{
	Q_OBJECT

public:
	enum class pad_type
	{
		buzz,
		turntable,
		ghltar,
		usio,
		gem,
		ds3gem,
		mousegem,
		guncon3,
		topshotelite,
		topshotfearmaster,
	};

	emulated_pad_settings_dialog(pad_type type, QWidget* parent = nullptr);

private:
	template <typename T>
	void add_tabs(QTabWidget* tabs);

	void load_config();
	void save_config();
	void reset_config();

	pad_type m_type;

	std::vector<std::vector<QComboBox*>> m_combos;
};
