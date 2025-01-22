#include "stdafx.h"
#include "raw_mouse_settings_dialog.h"
#include "localized_emu.h"
#include "gui_application.h"
#include "Input/raw_mouse_config.h"
#include "Input/raw_mouse_handler.h"
#include "util/asm.hpp"

#include <QAbstractItemView>
#include <QGroupBox>
#include <QMessageBox>
#include <QVBoxLayout>

LOG_CHANNEL(cfg_log, "CFG");

constexpr u32 button_count = 8;

raw_mouse_settings_dialog::raw_mouse_settings_dialog(QWidget* parent)
	: QDialog(parent)
{
	setObjectName("raw_mouse_settings_dialog");
	setWindowTitle(tr("Configure Raw Mouse Handler"));
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_StyledBackground);
	setModal(true);

	QVBoxLayout* v_layout = new QVBoxLayout(this);

	m_tab_widget = new QTabWidget();
	m_tab_widget->setUsesScrollButtons(false);

	m_button_box = new QDialogButtonBox(this);
	m_button_box->setStandardButtons(QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Save | QDialogButtonBox::RestoreDefaults);

	connect(m_button_box, &QDialogButtonBox::clicked, this, [this](QAbstractButton* button)
	{
		if (button == m_button_box->button(QDialogButtonBox::Apply))
		{
			g_cfg_raw_mouse.save();
		}
		else if (button == m_button_box->button(QDialogButtonBox::Save))
		{
			g_cfg_raw_mouse.save();
			accept();
		}
		else if (button == m_button_box->button(QDialogButtonBox::RestoreDefaults))
		{
			if (QMessageBox::question(this, tr("Confirm Reset"), tr("Reset settings of all players?")) != QMessageBox::Yes)
				return;
			reset_config();
		}
		else if (button == m_button_box->button(QDialogButtonBox::Cancel))
		{
			// Restore config
			if (!g_cfg_raw_mouse.load())
			{
				cfg_log.notice("Could not restore raw mouse config. Using defaults.");
			}
			reject();
		}
	});

	if (!g_cfg_raw_mouse.load())
	{
		cfg_log.notice("Could not load raw mouse config. Using defaults.");
	}

	constexpr u32 max_devices = 16;

	g_raw_mouse_handler = std::make_unique<raw_mouse_handler>();
	g_raw_mouse_handler->set_is_for_gui(true);
	g_raw_mouse_handler->Init(std::max(max_devices, ::size32(g_cfg_raw_mouse.players)));
	g_raw_mouse_handler->set_mouse_press_callback([this](const std::string& device_name, s32 button_code, bool pressed)
	{
		mouse_press(device_name, button_code, pressed);
	});
	g_raw_mouse_handler->set_key_press_callback([this](const std::string& device_name, s32 scan_code, bool pressed)
	{
		key_press(device_name, scan_code, pressed);
	});

	m_buttons = new QButtonGroup(this);
	connect(m_buttons, &QButtonGroup::idClicked, this, &raw_mouse_settings_dialog::on_button_click);

	connect(&m_update_timer, &QTimer::timeout, this, &raw_mouse_settings_dialog::on_enumeration);

	connect(&m_remap_timer, &QTimer::timeout, this, [this]()
	{
		auto button = m_buttons->button(m_button_id);

		if (--m_seconds <= 0)
		{
			if (button)
			{
				if (const int button_id = m_buttons->id(button); button_id >= 0)
				{
					auto& config = ::at32(g_cfg_raw_mouse.players, m_tab_widget->currentIndex());
					const std::string name = raw_mouse_config::get_button_name(config->get_button_by_index(button_id % button_count).to_string());
					button->setText(name.empty() ? QStringLiteral("-") : QString::fromStdString(name));
				}
			}
			reactivate_buttons();
			return;
		}
		if (button)
		{
			button->setText(tr("[ Waiting %1 ]").arg(m_seconds));
		}
	});

	connect(&m_mouse_release_timer, &QTimer::timeout, this, [this]()
	{
		m_mouse_release_timer.stop();
		m_disable_mouse_release_event = false;
	});

	add_tabs(m_tab_widget);

	v_layout->addWidget(m_tab_widget);
	v_layout->addWidget(m_button_box);
	setLayout(v_layout);

	m_palette = m_push_buttons[0][CELL_MOUSE_BUTTON_1]->palette(); // save normal palette

	connect(m_tab_widget, &QTabWidget::currentChanged, this, [this](int index)
	{
		handle_device_change(get_current_device_name(index));
	});

	on_enumeration();

	handle_device_change(get_current_device_name(0));

	m_update_timer.start(1000ms);
}

raw_mouse_settings_dialog::~raw_mouse_settings_dialog()
{
	g_raw_mouse_handler.reset();
}

void raw_mouse_settings_dialog::update_combo_box(usz player)
{
	auto& config = ::at32(g_cfg_raw_mouse.players, player);
	const std::string device_name = config->device.to_string();
	const QString current_device = QString::fromStdString(device_name);
	bool found_device = false;

	QComboBox* combo = ::at32(m_device_combos, player);

	combo->blockSignals(true);
	combo->clear();
	combo->addItem(tr("Disabled"), QString());

	{
		std::lock_guard lock(g_raw_mouse_handler->m_raw_mutex);

		const auto& mice = g_raw_mouse_handler->get_mice();

		for (const auto& [handle, mouse] : mice)
		{
			const QString name = QString::fromStdString(mouse.device_name());
			const QString& pretty_name = name; // Same ugly device path for now
			combo->addItem(pretty_name, name);

			if (current_device == name)
			{
				found_device = true;
			}
		}
	}

	// Keep configured device in list even if it is not connected
	if (!found_device && !current_device.isEmpty())
	{
		const QString& pretty_name = current_device; // Same ugly device path for now
		combo->addItem(pretty_name, current_device);
	}

	// Select index
	combo->setCurrentIndex(std::max(0, combo->findData(current_device)));
	combo->blockSignals(false);

	if (static_cast<int>(player) == m_tab_widget->currentIndex())
	{
		handle_device_change(device_name);
	}
}

void raw_mouse_settings_dialog::add_tabs(QTabWidget* tabs)
{
	ensure(!!tabs);

	constexpr u32 max_items_per_column = 6;
	int rows = button_count;

	for (u32 cols = 1; utils::aligned_div(button_count, cols) > max_items_per_column;)
	{
		rows = utils::aligned_div(button_count, ++cols);
	}

	constexpr usz players = g_cfg_raw_mouse.players.size();

	m_push_buttons.resize(players);

	ensure(g_raw_mouse_handler);

	const auto insert_button = [this](int id, QPushButton* button)
	{
		m_buttons->addButton(button, id);
		button->installEventFilter(this);
	};

	for (usz player = 0; player < players; player++)
	{
		QWidget* widget = new QWidget(this);
		QGridLayout* grid_layout = new QGridLayout(this);

		auto& config = ::at32(g_cfg_raw_mouse.players, player);

		QHBoxLayout* h_layout = new QHBoxLayout(this);
		QGroupBox* gb = new QGroupBox(tr("Device"), this);
		QComboBox* combo = new QComboBox(this);
		m_device_combos.push_back(combo);

		update_combo_box(player);

		connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, player, combo](int index)
		{
			if (index < 0 || !combo)
				return;

			const QVariant data = combo->itemData(index);
			if (!data.isValid() || !data.canConvert<QString>())
				return;

			const std::string device_name = data.toString().toStdString();

			auto& config = ::at32(g_cfg_raw_mouse.players, player)->device;
			config.from_string(device_name);

			handle_device_change(device_name);
		});

		h_layout->addWidget(combo);
		gb->setLayout(h_layout);
		grid_layout->addWidget(gb, 0, 0, 1, 2);

		const int first_row = grid_layout->rowCount();

		for (int i = 0, row = first_row, col = 0; i < static_cast<int>(button_count); i++, row++)
		{
			const int cell_code = get_mouse_button_code(i);
			const QString translated_cell_button = localized_emu::translated_mouse_button(cell_code);

			QHBoxLayout* h_layout = new QHBoxLayout(this);
			QGroupBox* gb = new QGroupBox(translated_cell_button, this);
			QPushButton* pb = new QPushButton;

			insert_button(static_cast<int>(player * button_count + i), pb);

			const std::string saved_btn = raw_mouse_config::get_button_name(config->get_button(cell_code).to_string());

			pb->setText(saved_btn.empty() ? QStringLiteral("-") : QString::fromStdString(saved_btn));

			if (row >= rows + first_row)
			{
				row = first_row;
				col++;
			}

			m_push_buttons[player][cell_code] = pb;
			h_layout->addWidget(pb);
			gb->setLayout(h_layout);
			grid_layout->addWidget(gb, row, col);
		}

		h_layout = new QHBoxLayout(this);
		gb = new QGroupBox(tr("Mouse Acceleration"), this);
		QDoubleSpinBox* mouse_acceleration_spin_box = new QDoubleSpinBox(this);
		m_accel_spin_boxes.push_back(mouse_acceleration_spin_box);
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

void raw_mouse_settings_dialog::on_enumeration()
{
	if (!g_raw_mouse_handler)
	{
		return;
	}

	const int player = m_tab_widget->currentIndex();
	QComboBox* combo = ::at32(m_device_combos, player);

	// Don't enumerate while the current dropdown is open
	if (combo && combo->view()->isVisible())
	{
		return;
	}

	g_raw_mouse_handler->update_devices();

	// Update all dropdowns
	for (usz player = 0; player < g_cfg_raw_mouse.players.size(); player++)
	{
		update_combo_box(player);
	}
}

void raw_mouse_settings_dialog::reset_config()
{
	g_cfg_raw_mouse.from_default();

	for (usz player = 0; player < m_push_buttons.size(); player++)
	{
		auto& config = ::at32(g_cfg_raw_mouse.players, player);

		for (auto& [cell_code, pb] : m_push_buttons[player])
		{
			if (!pb)
				continue;

			const std::string text = raw_mouse_config::get_button_name(config->get_button(cell_code).def);
			pb->setText(text.empty() ? QStringLiteral("-") : QString::fromStdString(text));
		}

		if (QComboBox* combo = ::at32(m_device_combos, player))
		{
			combo->setCurrentIndex(combo->findData(QString::fromStdString(config->device.to_string())));
		}

		if (QDoubleSpinBox* sb = ::at32(m_accel_spin_boxes, player))
		{
			sb->setValue(config->mouse_acceleration.get() / 100.0);
		}
	}
}

void raw_mouse_settings_dialog::mouse_press(const std::string& device_name, s32 button_code, bool pressed)
{
	if (m_button_id < 0 || pressed) // Let's only react to mouse releases
	{
		return;
	}

	const int player = m_tab_widget->currentIndex();
	const std::string current_device_name = get_current_device_name(player);

	if (device_name != current_device_name)
	{
		return;
	}

	const std::string button_name = raw_mouse_config::get_button_name(button_code);

	auto& config = ::at32(g_cfg_raw_mouse.players, player);
	config->get_button_by_index(m_button_id % button_count).from_string(button_name);

	if (auto button = m_buttons->button(m_button_id))
	{
		button->setText(QString::fromStdString(button_name));
	}

	reactivate_buttons();
}

void raw_mouse_settings_dialog::key_press(const std::string& device_name, s32 scan_code, bool pressed)
{
	if (m_button_id < 0 || !pressed) // Let's only react to key presses
	{
		return;
	}

	const int player = m_tab_widget->currentIndex();
	const std::string current_device_name = get_current_device_name(player);

	if (device_name != current_device_name)
	{
		return;
	}

	auto& config = ::at32(g_cfg_raw_mouse.players, player);
	config->get_button_by_index(m_button_id % button_count).from_string(fmt::format("%s%d", raw_mouse_config::key_prefix, scan_code));

	if (auto button = m_buttons->button(m_button_id))
	{
		button->setText(QString::fromStdString(raw_mouse_config::get_key_name(scan_code)));
	}

	reactivate_buttons();
}

void raw_mouse_settings_dialog::handle_device_change(const std::string& device_name)
{
	if (is_device_active(device_name))
	{
		reactivate_buttons();
	}
	else
	{
		for (auto but : m_buttons->buttons())
		{
			but->setEnabled(false);
		}
	}
}

bool raw_mouse_settings_dialog::is_device_active(const std::string& device_name)
{
	if (!g_raw_mouse_handler || device_name.empty())
	{
		return false;
	}

	std::lock_guard lock(g_raw_mouse_handler->m_raw_mutex);

	const auto& mice = g_raw_mouse_handler->get_mice();

	return std::any_of(mice.cbegin(), mice.cend(), [&device_name](const auto& entry){ return entry.second.device_name() == device_name; });
}

std::string raw_mouse_settings_dialog::get_current_device_name(int player)
{
	if (player < 0)
		return {};

	const QVariant data = ::at32(m_device_combos, player)->currentData();

	if (!data.canConvert<QString>())
		return {};

	return data.toString().toStdString();
}

bool raw_mouse_settings_dialog::eventFilter(QObject* object, QEvent* event)
{
	switch (event->type())
	{
	case QEvent::MouseButtonRelease:
	{
		// On right click clear binding if we are not remapping pad button
		if (m_button_id < 0 && !m_disable_mouse_release_event)
		{
			QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
			if (const auto button = qobject_cast<QPushButton*>(object); button && button->isEnabled() && mouse_event->button() == Qt::RightButton)
			{
				if (const int button_id = m_buttons->id(button); button_id >= 0)
				{
					button->setText(QStringLiteral("-"));
					auto& config = ::at32(g_cfg_raw_mouse.players, m_tab_widget->currentIndex());
					config->get_button_by_index(button_id % button_count).from_string("");
					return true;
				}
			}
		}

		// Disabled buttons should not absorb mouseclicks
		event->ignore();
		break;
	}
	default:
	{
		break;
	}
	}

	return QDialog::eventFilter(object, event);
}

void raw_mouse_settings_dialog::on_button_click(int id)
{
	if (id < 0)
	{
		return;
	}

	m_update_timer.stop();

	for (auto sb : m_accel_spin_boxes)
	{
		sb->setEnabled(false);
		sb->setFocusPolicy(Qt::ClickFocus);
	}

	for (auto cb : m_device_combos)
	{
		cb->setEnabled(false);
		cb->setFocusPolicy(Qt::ClickFocus);
	}

	for (auto but : m_buttons->buttons())
	{
		but->setEnabled(false);
		but->setFocusPolicy(Qt::ClickFocus);
	}

	m_button_box->setEnabled(false);
	for (auto but : m_button_box->buttons())
	{
		but->setFocusPolicy(Qt::ClickFocus);
	}

	m_button_id = id;
	if (auto button = m_buttons->button(m_button_id))
	{
		button->setText(tr("[ Waiting %1 ]").arg(MAX_SECONDS));
		button->setPalette(QPalette(Qt::blue));
		button->grabMouse();
	}

	m_tab_widget->setEnabled(false);

	m_remap_timer.start(1000);

	// We need to disable the mouse release event filter or we will clear the button if the raw mouse does a right click
	m_mouse_release_timer.stop();
	m_disable_mouse_release_event = true;
}

void raw_mouse_settings_dialog::reactivate_buttons()
{
	m_remap_timer.stop();
	m_seconds = MAX_SECONDS;

	if (m_button_id >= 0)
	{
		if (auto button = m_buttons->button(m_button_id))
		{
			button->setPalette(m_palette);
			button->releaseMouse();
		}

		m_button_id = -1;
	}

	// Enable all buttons
	m_button_box->setEnabled(true);

	for (auto but : m_button_box->buttons())
	{
		but->setFocusPolicy(Qt::StrongFocus);
	}

	for (auto sb : m_accel_spin_boxes)
	{
		sb->setEnabled(true);
		sb->setFocusPolicy(Qt::StrongFocus);
	}

	for (auto cb : m_device_combos)
	{
		cb->setEnabled(true);
		cb->setFocusPolicy(Qt::StrongFocus);
	}

	for (auto but : m_buttons->buttons())
	{
		but->setEnabled(true);
		but->setFocusPolicy(Qt::StrongFocus);
	}

	m_tab_widget->setEnabled(true);

	m_mouse_release_timer.start(100ms);
	m_update_timer.start(1000ms);
}
