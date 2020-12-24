#include "stdafx.h"
#include "basic_mouse_settings_dialog.h"
#include "localized_emu.h"
#include "Input/basic_mouse_handler.h"
#include "Emu/Io/mouse_config.h"
#include "Emu/Io/MouseHandler.h"
#include "util/asm.hpp"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

LOG_CHANNEL(cfg_log, "CFG");

enum button_role
{
	button_name = Qt::UserRole,
	button_code
};

basic_mouse_settings_dialog::basic_mouse_settings_dialog(QWidget* parent)
	: QDialog(parent)
{
	setObjectName("basic_mouse_settings_dialog");
	setWindowTitle(tr("Configure Basic Mouse Handler"));
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_StyledBackground);
	setModal(true);

	QVBoxLayout* v_layout = new QVBoxLayout(this);

	QDialogButtonBox* buttons = new QDialogButtonBox(this);
	buttons->setStandardButtons(QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Save | QDialogButtonBox::RestoreDefaults);

	connect(buttons, &QDialogButtonBox::clicked, this, [this, buttons](QAbstractButton* button)
	{
		if (button == buttons->button(QDialogButtonBox::Apply))
		{
			g_cfg_mouse.save();
		}
		else if (button == buttons->button(QDialogButtonBox::Save))
		{
			g_cfg_mouse.save();
			accept();
		}
		else if (button == buttons->button(QDialogButtonBox::RestoreDefaults))
		{
			if (QMessageBox::question(this, tr("Confirm Reset"), tr("Reset all settings?")) != QMessageBox::Yes)
				return;
			reset_config();
		}
		else if (button == buttons->button(QDialogButtonBox::Cancel))
		{
			reject();
		}
	});

	if (!g_cfg_mouse.load())
	{
		cfg_log.notice("Could not load basic mouse config. Using defaults.");
	}

	constexpr u32 button_count = 8;
	constexpr u32 max_items_per_column = 4;
	int rows = button_count;

	for (u32 cols = 1; utils::aligned_div(button_count, cols) > max_items_per_column;)
	{
		rows = utils::aligned_div(button_count, ++cols);
	}

	QWidget* widget = new QWidget(this);
	QGridLayout* grid_layout = new QGridLayout(this);

	for (int i = 0, row = 0, col = 0; i < static_cast<int>(button_count); i++, row++)
	{
		const int cell_code = get_mouse_button_code(i);
		const QString translated_cell_button = localized_emu::translated_mouse_button(cell_code);

		QHBoxLayout* h_layout = new QHBoxLayout(this);
		QGroupBox* gb = new QGroupBox(translated_cell_button, this);
		QComboBox* combo = new QComboBox;

		for (const auto& [name, btn] : qt_mouse_button_map)
		{
			const QString id = QString::fromStdString(name);
			const QString translated_id = id; // We could localize the id, but I am too lazy at the moment
			combo->addItem(translated_id);
			const int index = combo->findText(translated_id);
			combo->setItemData(index, id, button_role::button_name);
			combo->setItemData(index, cell_code, button_role::button_code);
		}

		const std::string saved_btn = g_cfg_mouse.get_button(cell_code);

		combo->setCurrentIndex(combo->findData(QString::fromStdString(saved_btn), button_role::button_name));

		connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, cell_code, combo](int index)
		{
			if (index < 0 || !combo)
				return;

			const QVariant data = combo->itemData(index, button_role::button_name);
			if (!data.isValid() || !data.canConvert<QString>())
				return;

			g_cfg_mouse.get_button(cell_code).from_string(data.toString().toStdString());
		});

		if (row >= rows)
		{
			row = 0;
			col++;
		}

		m_combos.push_back(combo);
		h_layout->addWidget(combo);
		gb->setLayout(h_layout);
		grid_layout->addWidget(gb, row, col);
	}

	widget->setLayout(grid_layout);

	v_layout->addWidget(widget);
	v_layout->addWidget(buttons);
	setLayout(v_layout);
}

void basic_mouse_settings_dialog::reset_config()
{
	g_cfg_mouse.from_default();

	for (QComboBox* combo : m_combos)
	{
		if (!combo)
			continue;

		const QVariant data = combo->itemData(0, button_role::button_code);
		if (!data.isValid() || !data.canConvert<int>())
			continue;

		const int cell_code = data.toInt();
		const QString def_btn_id = QString::fromStdString(g_cfg_mouse.get_button(cell_code).def);

		combo->setCurrentIndex(combo->findData(def_btn_id, button_role::button_name));
	}
}
