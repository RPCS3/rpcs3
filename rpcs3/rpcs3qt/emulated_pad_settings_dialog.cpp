#include "stdafx.h"
#include "emulated_pad_settings_dialog.h"
#include "localized_emu.h"
#include "Input/raw_mouse_config.h"
#include "Emu/Io/mouse_config.h"
#include "Emu/Io/buzz_config.h"
#include "Emu/Io/gem_config.h"
#include "Emu/Io/ghltar_config.h"
#include "Emu/Io/guncon3_config.h"
#include "Emu/Io/topshotelite_config.h"
#include "Emu/Io/topshotfearmaster_config.h"
#include "Emu/Io/turntable_config.h"
#include "Emu/Io/usio_config.h"
#include "util/asm.hpp"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
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

	QDialogButtonBox* buttons = new QDialogButtonBox(this);
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
			// Restore config
			load_config();
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
	case emulated_pad_settings_dialog::pad_type::gem:
		setWindowTitle(tr("Configure Emulated PS Move (Real)"));
		add_tabs<gem_btn>(tabs);
		break;
	case emulated_pad_settings_dialog::pad_type::ds3gem:
		setWindowTitle(tr("Configure Emulated PS Move (Fake)"));
		add_tabs<gem_btn>(tabs);
		break;
	case emulated_pad_settings_dialog::pad_type::mousegem:
		setWindowTitle(tr("Configure Emulated PS Move (Mouse)"));
		add_tabs<gem_btn>(tabs);
		break;
	case emulated_pad_settings_dialog::pad_type::guncon3:
		setWindowTitle(tr("Configure Emulated GunCon 3"));
		add_tabs<guncon3_btn>(tabs);
		break;
	case emulated_pad_settings_dialog::pad_type::topshotelite:
		setWindowTitle(tr("Configure Emulated Top Shot Elite"));
		add_tabs<topshotelite_btn>(tabs);
		break;
	case emulated_pad_settings_dialog::pad_type::topshotfearmaster:
		setWindowTitle(tr("Configure Emulated Top Shot Fearmaster"));
		add_tabs<topshotfearmaster_btn>(tabs);
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

	std::set<int> ignored_values;

	const auto remove_value = [&ignored_values](int value)
	{
		ignored_values.insert(static_cast<int>(value));
	};

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
	case pad_type::gem:
		players = g_cfg_gem_real.players.size();

		// Ignore combo, x and y axis
		remove_value(static_cast<int>(gem_btn::x_axis));
		remove_value(static_cast<int>(gem_btn::y_axis));
		for (int i = static_cast<int>(gem_btn::combo_begin); i <= static_cast<int>(gem_btn::combo_end); i++)
		{
			remove_value(i);
		}
		break;
	case pad_type::ds3gem:
		players = g_cfg_gem_fake.players.size();

		// Ignore combo
		for (int i = static_cast<int>(gem_btn::combo_begin); i <= static_cast<int>(gem_btn::combo_end); i++)
		{
			remove_value(i);
		}
		break;
	case pad_type::mousegem:
		players = g_cfg_gem_mouse.players.size();

		// Ignore x and y axis
		remove_value(static_cast<int>(gem_btn::x_axis));
		remove_value(static_cast<int>(gem_btn::y_axis));
		break;
	case pad_type::guncon3:
		players = g_cfg_guncon3.players.size();
		break;
	case pad_type::topshotelite:
		players = g_cfg_topshotelite.players.size();
		break;
	case pad_type::topshotfearmaster:
		players = g_cfg_topshotfearmaster.players.size();
		break;
	}

	constexpr u32 max_items_per_column = 6;
	const int count = static_cast<int>(T::count) - static_cast<int>(ignored_values.size());
	int rows = count;

	for (u32 cols = 1; utils::aligned_div(static_cast<u32>(count), cols) > max_items_per_column;)
	{
		rows = utils::aligned_div(static_cast<u32>(count), ++cols);
	}

	m_combos.resize(players);

	const bool show_mouse_legend = m_type == pad_type::mousegem;

	if (show_mouse_legend)
	{
		if (!g_cfg_mouse.load())
		{
			cfg_log.notice("Could not restore mouse config. Using defaults.");
		}

		if (!g_cfg_raw_mouse.load())
		{
			cfg_log.notice("Could not restore raw mouse config. Using defaults.");
		}
	}

	for (usz player = 0; player < players; player++)
	{
		// Create grid with all buttons
		QGridLayout* grid_layout = new QGridLayout(this);

		for (int i = 0, row = 0, col = 0; i < static_cast<int>(T::count); i++)
		{
			if (ignored_values.contains(i)) continue;

			const T id = static_cast<T>(i);
			const QString name = QString::fromStdString(fmt::format("%s", id));

			QHBoxLayout* h_layout = new QHBoxLayout(this);
			QGroupBox* gb = new QGroupBox(name, this);
			QComboBox* combo = new QComboBox;

			if constexpr (std::is_same_v<T, gem_btn>)
			{
				const gem_btn btn = static_cast<gem_btn>(i);
				if (btn >= gem_btn::combo_begin && btn <= gem_btn::combo_end)
				{
					gb->setToolTip(tr("Press the \"Combo\" button in combination with any of the other combo buttons to trigger their related PS Move button.\n"
					                  "This can be useful if your device does not have enough regular buttons."));
				}
			}

			// Add empty value
			combo->addItem("");
			const int index = combo->findText("");
			combo->setItemData(index, static_cast<int>(pad_button::pad_button_max_enum), button_role::button);
			combo->setItemData(index, i, button_role::emulated_button);

			if (m_type != pad_type::mousegem)
			{
				for (int p = 0; p < static_cast<int>(pad_button::pad_button_max_enum); p++)
				{
					const QString translated = localized_emu::translated_pad_button(static_cast<pad_button>(p));
					combo->addItem(translated);
					const int index = combo->findText(translated);
					combo->setItemData(index, p, button_role::button);
					combo->setItemData(index, i, button_role::emulated_button);
				}
			}

			if (std::is_same_v<T, guncon3_btn> || std::is_same_v<T, topshotelite_btn> || std::is_same_v<T, topshotfearmaster_btn> || m_type == pad_type::mousegem)
			{
				for (int p = static_cast<int>(pad_button::mouse_button_1); p <= static_cast<int>(pad_button::mouse_button_8); p++)
				{
					const QString translated = localized_emu::translated_pad_button(static_cast<pad_button>(p));
					combo->addItem(translated);
					const int index = combo->findText(translated);
					combo->setItemData(index, p, button_role::button);
					combo->setItemData(index, i, button_role::emulated_button);
				}
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
			case pad_type::gem:
				saved_btn_id = ::at32(g_cfg_gem_real.players, player)->get_pad_button(static_cast<gem_btn>(id));
				break;
			case pad_type::ds3gem:
				saved_btn_id = ::at32(g_cfg_gem_fake.players, player)->get_pad_button(static_cast<gem_btn>(id));
				break;
			case pad_type::mousegem:
				saved_btn_id = ::at32(g_cfg_gem_mouse.players, player)->get_pad_button(static_cast<gem_btn>(id));
				break;
			case pad_type::guncon3:
				saved_btn_id = ::at32(g_cfg_guncon3.players, player)->get_pad_button(static_cast<guncon3_btn>(id));
				break;
			case pad_type::topshotelite:
				saved_btn_id = ::at32(g_cfg_topshotelite.players, player)->get_pad_button(static_cast<topshotelite_btn>(id));
				break;
			case pad_type::topshotfearmaster:
				saved_btn_id = ::at32(g_cfg_topshotfearmaster.players, player)->get_pad_button(static_cast<topshotfearmaster_btn>(id));
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
				case pad_type::gem:
					::at32(g_cfg_gem_real.players, player)->set_button(static_cast<gem_btn>(id), btn_id);
					break;
				case pad_type::ds3gem:
					::at32(g_cfg_gem_fake.players, player)->set_button(static_cast<gem_btn>(id), btn_id);
					break;
				case pad_type::mousegem:
					::at32(g_cfg_gem_mouse.players, player)->set_button(static_cast<gem_btn>(id), btn_id);
					break;
				case pad_type::guncon3:
					::at32(g_cfg_guncon3.players, player)->set_button(static_cast<guncon3_btn>(id), btn_id);
					break;
				case pad_type::topshotelite:
					::at32(g_cfg_topshotelite.players, player)->set_button(static_cast<topshotelite_btn>(id), btn_id);
					break;
				case pad_type::topshotfearmaster:
					::at32(g_cfg_topshotfearmaster.players, player)->set_button(static_cast<topshotfearmaster_btn>(id), btn_id);
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

			row++;
		}

		QVBoxLayout* v_layout = new QVBoxLayout(this);

		// Create a legend of the current mouse settings
		if (show_mouse_legend)
		{
			QHBoxLayout* legend_layout = new QHBoxLayout(this);
			if (player == 0)
			{
				std::string basic_mouse_settings;
				fmt::append(basic_mouse_settings, "1: %s\n", g_cfg_mouse.mouse_button_1.to_string());
				fmt::append(basic_mouse_settings, "2: %s\n", g_cfg_mouse.mouse_button_2.to_string());
				fmt::append(basic_mouse_settings, "3: %s\n", g_cfg_mouse.mouse_button_3.to_string());
				fmt::append(basic_mouse_settings, "4: %s\n", g_cfg_mouse.mouse_button_4.to_string());
				fmt::append(basic_mouse_settings, "5: %s\n", g_cfg_mouse.mouse_button_5.to_string());
				fmt::append(basic_mouse_settings, "6: %s\n", g_cfg_mouse.mouse_button_6.to_string());
				fmt::append(basic_mouse_settings, "7: %s\n", g_cfg_mouse.mouse_button_7.to_string());
				fmt::append(basic_mouse_settings, "8: %s",   g_cfg_mouse.mouse_button_8.to_string());

				QGroupBox* gb_legend_basic = new QGroupBox(tr("Current Basic Mouse Config"), this);
				QVBoxLayout* gb_legend_basic_layout = new QVBoxLayout(this);
				gb_legend_basic_layout->addWidget(new QLabel(QString::fromStdString(basic_mouse_settings), this));
				gb_legend_basic->setLayout(gb_legend_basic_layout);
				legend_layout->addWidget(gb_legend_basic);
			}
			{
				std::string raw_mouse_settings;
				const auto& raw_cfg = *ensure(::at32(g_cfg_raw_mouse.players, player));
				fmt::append(raw_mouse_settings, "1: %s\n", raw_mouse_config::get_button_name(raw_cfg.mouse_button_1.to_string()));
				fmt::append(raw_mouse_settings, "2: %s\n", raw_mouse_config::get_button_name(raw_cfg.mouse_button_2.to_string()));
				fmt::append(raw_mouse_settings, "3: %s\n", raw_mouse_config::get_button_name(raw_cfg.mouse_button_3.to_string()));
				fmt::append(raw_mouse_settings, "4: %s\n", raw_mouse_config::get_button_name(raw_cfg.mouse_button_4.to_string()));
				fmt::append(raw_mouse_settings, "5: %s\n", raw_mouse_config::get_button_name(raw_cfg.mouse_button_5.to_string()));
				fmt::append(raw_mouse_settings, "6: %s\n", raw_mouse_config::get_button_name(raw_cfg.mouse_button_6.to_string()));
				fmt::append(raw_mouse_settings, "7: %s\n", raw_mouse_config::get_button_name(raw_cfg.mouse_button_7.to_string()));
				fmt::append(raw_mouse_settings, "8: %s",   raw_mouse_config::get_button_name(raw_cfg.mouse_button_8.to_string()));

				QGroupBox* gb_legend_raw = new QGroupBox(tr("Current Raw Mouse Config"), this);
				QVBoxLayout* gb_legend_raw_layout = new QVBoxLayout(this);
				gb_legend_raw_layout->addWidget(new QLabel(QString::fromStdString(raw_mouse_settings), this));
				gb_legend_raw->setLayout(gb_legend_raw_layout);
				legend_layout->addWidget(gb_legend_raw);
			}
			v_layout->addLayout(legend_layout);
		}

		v_layout->addLayout(grid_layout);

		QWidget* widget = new QWidget(this);
		widget->setLayout(v_layout);

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
	case emulated_pad_settings_dialog::pad_type::gem:
		if (!g_cfg_gem_real.load())
		{
			cfg_log.notice("Could not load gem config. Using defaults.");
		}
		break;
	case emulated_pad_settings_dialog::pad_type::ds3gem:
		if (!g_cfg_gem_fake.load())
		{
			cfg_log.notice("Could not load fake gem config. Using defaults.");
		}
		break;
	case emulated_pad_settings_dialog::pad_type::mousegem:
		if (!g_cfg_gem_mouse.load())
		{
			cfg_log.notice("Could not load mouse gem config. Using defaults.");
		}
		break;
	case emulated_pad_settings_dialog::pad_type::guncon3:
		if (!g_cfg_guncon3.load())
		{
			cfg_log.notice("Could not load guncon3 config. Using defaults.");
		}
		break;
	case emulated_pad_settings_dialog::pad_type::topshotelite:
		if (!g_cfg_topshotelite.load())
		{
			cfg_log.notice("Could not load topshotelite config. Using defaults.");
		}
		break;
	case emulated_pad_settings_dialog::pad_type::topshotfearmaster:
		if (!g_cfg_topshotfearmaster.load())
		{
			cfg_log.notice("Could not load topshotfearmaster config. Using defaults.");
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
	case emulated_pad_settings_dialog::pad_type::gem:
		g_cfg_gem_real.save();
		break;
	case emulated_pad_settings_dialog::pad_type::ds3gem:
		g_cfg_gem_fake.save();
		break;
	case emulated_pad_settings_dialog::pad_type::mousegem:
		g_cfg_gem_mouse.save();
		break;
	case emulated_pad_settings_dialog::pad_type::guncon3:
		g_cfg_guncon3.save();
		break;
	case emulated_pad_settings_dialog::pad_type::topshotelite:
		g_cfg_topshotelite.save();
		break;
	case emulated_pad_settings_dialog::pad_type::topshotfearmaster:
		g_cfg_topshotfearmaster.save();
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
	case emulated_pad_settings_dialog::pad_type::gem:
		g_cfg_gem_real.from_default();
		break;
	case emulated_pad_settings_dialog::pad_type::ds3gem:
		g_cfg_gem_fake.from_default();
		break;
	case emulated_pad_settings_dialog::pad_type::mousegem:
		g_cfg_gem_mouse.from_default();
		break;
	case emulated_pad_settings_dialog::pad_type::guncon3:
		g_cfg_guncon3.from_default();
		break;
	case emulated_pad_settings_dialog::pad_type::topshotelite:
		g_cfg_topshotelite.from_default();
		break;
	case emulated_pad_settings_dialog::pad_type::topshotfearmaster:
		g_cfg_topshotfearmaster.from_default();
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
			case pad_type::gem:
				def_btn_id = ::at32(g_cfg_gem_real.players, player)->default_pad_button(static_cast<gem_btn>(data.toInt()));
				break;
			case pad_type::ds3gem:
				def_btn_id = ::at32(g_cfg_gem_fake.players, player)->default_pad_button(static_cast<gem_btn>(data.toInt()));
				break;
			case pad_type::mousegem:
				def_btn_id = ::at32(g_cfg_gem_mouse.players, player)->default_pad_button(static_cast<gem_btn>(data.toInt()));
				break;
			case pad_type::guncon3:
				def_btn_id = ::at32(g_cfg_guncon3.players, player)->default_pad_button(static_cast<guncon3_btn>(data.toInt()));
				break;
			case pad_type::topshotelite:
				def_btn_id = ::at32(g_cfg_topshotelite.players, player)->default_pad_button(static_cast<topshotelite_btn>(data.toInt()));
				break;
			case pad_type::topshotfearmaster:
				def_btn_id = ::at32(g_cfg_topshotfearmaster.players, player)->default_pad_button(static_cast<topshotfearmaster_btn>(data.toInt()));
				break;
			}

			combo->setCurrentIndex(combo->findData(static_cast<int>(def_btn_id)));
		}
	}
}
