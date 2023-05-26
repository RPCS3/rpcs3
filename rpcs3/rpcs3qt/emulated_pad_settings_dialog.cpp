#include "stdafx.h"
#include "emulated_pad_settings_dialog.h"
#include "localized_emu.h"
#include "Emu/Io/buzz_config.h"
#include "Emu/Io/gem_config.h"
#include "Emu/Io/ghltar_config.h"
#include "Emu/Io/turntable_config.h"
#include "Emu/Io/usio_config.h"
#include "util/asm.hpp"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

enum button_role
{
	button = Qt::UserRole,
	emulated_button
};

emulated_pad_settings_dialog::emulated_pad_settings_dialog(pad_type type, QWidget* parent)
	: QDialog(parent), m_type(type)
{
	setObjectName("emulated_pad_settings_dialog");
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_StyledBackground);
	setModal(true);

	QVBoxLayout* v_layout = new QVBoxLayout(this);

	QTabWidget* tabs = new QTabWidget();
	tabs->setUsesScrollButtons(false);

	QDialogButtonBox* buttons =new QDialogButtonBox(this);
	buttons->setStandardButtons(QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Save | QDialogButtonBox::RestoreDefaults);

	connect(buttons, &QDialogButtonBox::clicked, this, [this, buttons](QAbstractButton* button)
	{
		if (button == buttons->button(QDialogButtonBox::Apply))
		{
			save_config();
		}
		else if (button == buttons->button(QDialogButtonBox::Save))
		{
			save_config();
			accept();
		}
		else if (button == buttons->button(QDialogButtonBox::RestoreDefaults))
		{
			if (QMessageBox::question(this, tr("Confirm Reset"), tr("Reset all buttons of all players?")) != QMessageBox::Yes)
				return;
			reset_config();
		}
		else if (button == buttons->button(QDialogButtonBox::Cancel))
		{
			reject();
		}
	});

	load_config();

	switch (m_type)
	{
	case emulated_pad_settings_dialog::pad_type::buzz:
		setWindowTitle(tr("Configure Emulated Buzz"));
		add_tabs<buzz_btn>(tabs);
		break;
	case emulated_pad_settings_dialog::pad_type::turntable:
		setWindowTitle(tr("Configure Emulated Turntable"));
		add_tabs<turntable_btn>(tabs);
		break;
	case emulated_pad_settings_dialog::pad_type::ghltar:
		setWindowTitle(tr("Configure Emulated GHLtar"));
		add_tabs<ghltar_btn>(tabs);
		break;
	case emulated_pad_settings_dialog::pad_type::usio:
		setWindowTitle(tr("Configure Emulated USIO"));
		add_tabs<usio_btn>(tabs);
		break;
	case emulated_pad_settings_dialog::pad_type::ds3gem:
		setWindowTitle(tr("Configure Emulated PS Move (Fake)"));
		add_tabs<gem_btn>(tabs);
		break;
	}

	v_layout->addWidget(tabs);
	v_layout->addWidget(buttons);
	setLayout(v_layout);
}

template <typename T>
void emulated_pad_settings_dialog::add_tabs(QTabWidget* tabs)
{
	ensure(!!tabs);

	constexpr u32 max_items_per_column = 6;
	int rows = static_cast<int>(T::count);

	for (u32 cols = 1; utils::aligned_div(static_cast<u32>(T::count), cols) > max_items_per_column;)
	{
		rows = utils::aligned_div(static_cast<u32>(T::count), ++cols);
	}

	usz players = 0;
	switch (m_type)
	{
	case pad_type::buzz:
		players = g_cfg_buzz.players.size();
		break;
	case pad_type::turntable:
		players = g_cfg_turntable.players.size();
		break;
	case pad_type::ghltar:
		players = g_cfg_ghltar.players.size();
		break;
	case pad_type::usio:
		players = g_cfg_usio.players.size();
		break;
	case pad_type::ds3gem:
		players = g_cfg_gem.players.size();
		break;
	}

	m_combos.resize(players);

	for (usz player = 0; player < players; player++)
	{
		QWidget* widget = new QWidget(this);
		QGridLayout* grid_layout = new QGridLayout(this);

		for (int i = 0, row = 0, col = 0; i < static_cast<int>(T::count); i++, row++)
		{
			const T id = static_cast<T>(i);
			const QString name = QString::fromStdString(fmt::format("%s", id));

			QHBoxLayout* h_layout = new QHBoxLayout(this);
			QGroupBox* gb = new QGroupBox(name, this);
			QComboBox* combo = new QComboBox;

			for (int p = 0; p < static_cast<int>(pad_button::pad_button_max_enum); p++)
			{
				const QString translated = localized_emu::translated_pad_button(static_cast<pad_button>(p));
				combo->addItem(translated);
				const int index = combo->findText(translated);
				combo->setItemData(index, p, button_role::button);
				combo->setItemData(index, i, button_role::emulated_button);
			}

			pad_button saved_btn_id = pad_button::pad_button_max_enum;
			switch (m_type)
			{
			case pad_type::buzz:
				saved_btn_id = ::at32(g_cfg_buzz.players, player)->get_pad_button(static_cast<buzz_btn>(id));
				break;
			case pad_type::turntable:
				saved_btn_id = ::at32(g_cfg_turntable.players, player)->get_pad_button(static_cast<turntable_btn>(id));
				break;
			case pad_type::ghltar:
				saved_btn_id = ::at32(g_cfg_ghltar.players, player)->get_pad_button(static_cast<ghltar_btn>(id));
				break;
			case pad_type::usio:
				saved_btn_id = ::at32(g_cfg_usio.players, player)->get_pad_button(static_cast<usio_btn>(id));
				break;
			case pad_type::ds3gem:
				saved_btn_id = ::at32(g_cfg_gem.players, player)->get_pad_button(static_cast<gem_btn>(id));
				break;
			}

			combo->setCurrentIndex(combo->findData(static_cast<int>(saved_btn_id)));

			connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, player, id, combo](int index)
			{
				if (index < 0 || !combo)
					return;

				const QVariant data = combo->itemData(index, button_role::button);
				if (!data.isValid() || !data.canConvert<int>())
					return;

				const pad_button btn_id = static_cast<pad_button>(data.toInt());

				switch (m_type)
				{
				case pad_type::buzz:
					::at32(g_cfg_buzz.players, player)->set_button(static_cast<buzz_btn>(id), btn_id);
					break;
				case pad_type::turntable:
					::at32(g_cfg_turntable.players, player)->set_button(static_cast<turntable_btn>(id), btn_id);
					break;
				case pad_type::ghltar:
					::at32(g_cfg_ghltar.players, player)->set_button(static_cast<ghltar_btn>(id), btn_id);
					break;
				case pad_type::usio:
					::at32(g_cfg_usio.players, player)->set_button(static_cast<usio_btn>(id), btn_id);
					break;
				case pad_type::ds3gem:
					::at32(g_cfg_gem.players, player)->set_button(static_cast<gem_btn>(id), btn_id);
					break;
				}
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

		widget->setLayout(grid_layout);
		tabs->addTab(widget, tr("Player %0").arg(player + 1));
	}
}

void emulated_pad_settings_dialog::load_config()
{
	switch (m_type)
	{
	case emulated_pad_settings_dialog::pad_type::buzz:
		if (!g_cfg_buzz.load())
		{
			cfg_log.notice("Could not load buzz config. Using defaults.");
		}
		break;
	case emulated_pad_settings_dialog::pad_type::turntable:
		if (!g_cfg_turntable.load())
		{
			cfg_log.notice("Could not load turntable config. Using defaults.");
		}
		break;
	case emulated_pad_settings_dialog::pad_type::ghltar:
		if (!g_cfg_ghltar.load())
		{
			cfg_log.notice("Could not load ghltar config. Using defaults.");
		}
		break;
	case emulated_pad_settings_dialog::pad_type::usio:
		if (!g_cfg_usio.load())
		{
			cfg_log.notice("Could not load usio config. Using defaults.");
		}
		break;
	case emulated_pad_settings_dialog::pad_type::ds3gem:
		if (!g_cfg_gem.load())
		{
			cfg_log.notice("Could not load gem config. Using defaults.");
		}
		break;
	}
}

void emulated_pad_settings_dialog::save_config()
{
	switch (m_type)
	{
	case emulated_pad_settings_dialog::pad_type::buzz:
		g_cfg_buzz.save();
		break;
	case emulated_pad_settings_dialog::pad_type::turntable:
		g_cfg_turntable.save();
		break;
	case emulated_pad_settings_dialog::pad_type::ghltar:
		g_cfg_ghltar.save();
		break;
	case emulated_pad_settings_dialog::pad_type::usio:
		g_cfg_usio.save();
		break;
	case emulated_pad_settings_dialog::pad_type::ds3gem:
		g_cfg_gem.save();
		break;
	}
}

void emulated_pad_settings_dialog::reset_config()
{
	switch (m_type)
	{
	case emulated_pad_settings_dialog::pad_type::buzz:
		g_cfg_buzz.from_default();
		break;
	case emulated_pad_settings_dialog::pad_type::turntable:
		g_cfg_turntable.from_default();
		break;
	case emulated_pad_settings_dialog::pad_type::ghltar:
		g_cfg_ghltar.from_default();
		break;
	case emulated_pad_settings_dialog::pad_type::usio:
		g_cfg_usio.from_default();
		break;
	case emulated_pad_settings_dialog::pad_type::ds3gem:
		g_cfg_gem.from_default();
		break;
	}

	for (usz player = 0; player < m_combos.size(); player++)
	{
		for (QComboBox* combo : m_combos.at(player))
		{
			if (!combo)
				continue;

			const QVariant data = combo->itemData(0, button_role::emulated_button);
			if (!data.isValid() || !data.canConvert<int>())
				continue;

			pad_button def_btn_id = pad_button::pad_button_max_enum;
			switch (m_type)
			{
			case pad_type::buzz:
				def_btn_id = ::at32(g_cfg_buzz.players, player)->default_pad_button(static_cast<buzz_btn>(data.toInt()));
				break;
			case pad_type::turntable:
				def_btn_id = ::at32(g_cfg_turntable.players, player)->default_pad_button(static_cast<turntable_btn>(data.toInt()));
				break;
			case pad_type::ghltar:
				def_btn_id = ::at32(g_cfg_ghltar.players, player)->default_pad_button(static_cast<ghltar_btn>(data.toInt()));
				break;
			case pad_type::usio:
				def_btn_id = ::at32(g_cfg_usio.players, player)->default_pad_button(static_cast<usio_btn>(data.toInt()));
				break;
			case pad_type::ds3gem:
				def_btn_id = ::at32(g_cfg_gem.players, player)->default_pad_button(static_cast<gem_btn>(data.toInt()));
				break;
			}

			combo->setCurrentIndex(combo->findData(static_cast<int>(def_btn_id)));
		}
	}
}
