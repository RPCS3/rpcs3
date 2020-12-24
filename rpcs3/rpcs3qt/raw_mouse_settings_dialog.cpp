#include "stdafx.h"
#include "raw_mouse_settings_dialog.h"
#include "localized_emu.h"
#include "Input/raw_mouse_config.h"
#include "Input/raw_mouse_handler.h"
#include "util/asm.hpp"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

enum button_role
{
	button_name = Qt::UserRole,
	button_code
};

raw_mouse_settings_dialog::raw_mouse_settings_dialog(QWidget* parent)
	: QDialog(parent)
{
	setObjectName("raw_mouse_settings_dialog");
	setWindowTitle(tr("Configure Raw Mouse Handler"));
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_StyledBackground);
	setModal(true);

	QVBoxLayout* v_layout = new QVBoxLayout(this);

	QTabWidget* tabs = new QTabWidget();
	tabs->setUsesScrollButtons(false);

	QDialogButtonBox* buttons = new QDialogButtonBox(this);
	buttons->setStandardButtons(QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Save | QDialogButtonBox::RestoreDefaults);

	connect(buttons, &QDialogButtonBox::clicked, this, [this, buttons](QAbstractButton* button)
	{
		if (button == buttons->button(QDialogButtonBox::Apply))
		{
			g_cfg_raw_mouse.save();
		}
		else if (button == buttons->button(QDialogButtonBox::Save))
		{
			g_cfg_raw_mouse.save();
			accept();
		}
		else if (button == buttons->button(QDialogButtonBox::RestoreDefaults))
		{
			if (QMessageBox::question(this, tr("Confirm Reset"), tr("Reset settings of all players?")) != QMessageBox::Yes)
				return;
			reset_config();
		}
		else if (button == buttons->button(QDialogButtonBox::Cancel))
		{
			reject();
		}
	});

	if (!g_cfg_raw_mouse.load())
	{
		cfg_log.notice("Could not load raw mouse config. Using defaults.");
	}

	add_tabs(tabs);

	v_layout->addWidget(tabs);
	v_layout->addWidget(buttons);
	setLayout(v_layout);
}

void raw_mouse_settings_dialog::add_tabs(QTabWidget* tabs)
{
	ensure(!!tabs);

	constexpr u32 button_count = 8;
	constexpr u32 max_items_per_column = 6;
	int rows = button_count;

	for (u32 cols = 1; utils::aligned_div(button_count, cols) > max_items_per_column;)
	{
		rows = utils::aligned_div(button_count, ++cols);
	}

	const usz players = g_cfg_raw_mouse.players.size();

	m_combos.resize(players);

	for (usz player = 0; player < players; player++)
	{
		QWidget* widget = new QWidget(this);
		QGridLayout* grid_layout = new QGridLayout(this);

		auto& config = ::at32(g_cfg_raw_mouse.players, player);

		for (int i = 0, row = 0, col = 0; i < static_cast<int>(button_count); i++, row++)
		{
			const int cell_code = get_mouse_button_code(i);
			const QString translated_cell_button = localized_emu::translated_mouse_button(cell_code);

			QHBoxLayout* h_layout = new QHBoxLayout(this);
			QGroupBox* gb = new QGroupBox(translated_cell_button, this);
			QComboBox* combo = new QComboBox;

			for (const auto& [name, btn] : raw_mouse_button_map)
			{
				const QString id = QString::fromStdString(name);
				const QString translated_id = id; // We could localize the id, but I am too lazy at the moment
				combo->addItem(translated_id);
				const int index = combo->findText(translated_id);
				combo->setItemData(index, id, button_role::button_name);
				combo->setItemData(index, cell_code, button_role::button_code);
			}

			const std::string saved_btn = config->get_button(cell_code);

			combo->setCurrentIndex(combo->findData(QString::fromStdString(saved_btn), button_role::button_name));

			connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, player, cell_code, combo](int index)
			{
				if (index < 0 || !combo)
					return;

				const QVariant data = combo->itemData(index, button_role::button_name);
				if (!data.isValid() || !data.canConvert<QString>())
					return;

				::at32(g_cfg_raw_mouse.players, player)->get_button(cell_code).from_string(data.toString().toStdString());
			});

			if (row >= rows)
			{
				row = 0;
				col++;
			}

			::at32(m_combos, player).push_back(combo);
			h_layout->addWidget(combo);
			gb->setLayout(h_layout);
			grid_layout->addWidget(gb, row, col);
		}

		QHBoxLayout* h_layout = new QHBoxLayout(this);
		QGroupBox* gb = new QGroupBox(tr("Mouse Acceleration"), this);
		QDoubleSpinBox* mouse_acceleration_spin_box = new QDoubleSpinBox(this);
		mouse_acceleration_spin_box->setRange(0.1, 10.0);
		mouse_acceleration_spin_box->setValue(config->mouse_acceleration.get() / 100.0);
		connect(mouse_acceleration_spin_box, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [player](double value)
		{
			auto& config = ::at32(g_cfg_raw_mouse.players, player)->mouse_acceleration;
			config.set(std::clamp(value * 100.0, config.min, config.max));
		});

		h_layout->addWidget(mouse_acceleration_spin_box);
		gb->setLayout(h_layout);
		grid_layout->addWidget(gb, grid_layout->rowCount(), 0);

		widget->setLayout(grid_layout);
		tabs->addTab(widget, tr("Player %0").arg(player + 1));
	}
}

void raw_mouse_settings_dialog::reset_config()
{
	g_cfg_raw_mouse.from_default();

	for (usz player = 0; player < m_combos.size(); player++)
	{
		auto& config = ::at32(g_cfg_raw_mouse.players, player);

		for (QComboBox* combo : m_combos.at(player))
		{
			if (!combo)
				continue;

			const QVariant data = combo->itemData(0, button_role::button_code);
			if (!data.isValid() || !data.canConvert<int>())
				continue;

			const int cell_code = data.toInt();
			const QString def_btn_id = QString::fromStdString(config->get_button(cell_code).def);

			combo->setCurrentIndex(combo->findData(def_btn_id, button_role::button_name));
		}
	}
}
