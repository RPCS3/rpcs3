#include "stdafx.h"

#ifdef HAVE_SDL3

#include "emulated_logitech_g27_settings_dialog.h"
#include "Emu/Io/LogitechG27.h"
#include "Input/sdl_instance.h"
#include "qt_utils.h"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QTimer>
#include <QSlider>
#include <QComboBox>

LOG_CHANNEL(logitech_g27_cfg_log, "LOGIG27");

static const QString DEFAULT_STATUS = " ";

enum class mapping_device
{
	NONE = -1,

	// Axis
	STEERING = 0,
	THROTTLE,
	BRAKE,
	CLUTCH,

	// Buttons
	FIRST_BUTTON,
	SHIFT_UP = FIRST_BUTTON,
	SHIFT_DOWN,

	UP,
	DOWN,
	LEFT,
	RIGHT,

	TRIANGLE,
	CROSS,
	SQUARE,
	CIRCLE,

	L2,
	L3,
	R2,
	R3,

	PLUS,
	MINUS,

	DIAL_CLOCKWISE,
	DIAL_ANTICLOCKWISE,

	SELECT,
	PAUSE,

	SHIFTER_1,
	SHIFTER_2,
	SHIFTER_3,
	SHIFTER_4,
	SHIFTER_5,
	SHIFTER_6,
	SHIFTER_R,
	SHIFTER_PRESS,

	// Enum count
	COUNT
};

QString device_name(mapping_device dev)
{
	switch (dev)
	{
	case mapping_device::NONE: return "";
	case mapping_device::STEERING: return QObject::tr("Steering");
	case mapping_device::THROTTLE: return QObject::tr("Throttle");
	case mapping_device::BRAKE: return QObject::tr("Brake");
	case mapping_device::CLUTCH: return QObject::tr("Clutch");
	case mapping_device::SHIFT_UP: return QObject::tr("Shift up");
	case mapping_device::SHIFT_DOWN: return QObject::tr("Shift down");
	case mapping_device::UP: return QObject::tr("Up");
	case mapping_device::DOWN: return QObject::tr("Down");
	case mapping_device::LEFT: return QObject::tr("Left");
	case mapping_device::RIGHT: return QObject::tr("Right");
	case mapping_device::TRIANGLE: return QObject::tr("Triangle");
	case mapping_device::CROSS: return QObject::tr("Cross");
	case mapping_device::SQUARE: return QObject::tr("Square");
	case mapping_device::CIRCLE: return QObject::tr("Circle");
	case mapping_device::L2: return QObject::tr("L2");
	case mapping_device::L3: return QObject::tr("L3");
	case mapping_device::R2: return QObject::tr("R2");
	case mapping_device::R3: return QObject::tr("R3");
	case mapping_device::PLUS: return QObject::tr("L4");
	case mapping_device::MINUS: return QObject::tr("L5");
	case mapping_device::DIAL_CLOCKWISE: return QObject::tr("R4");
	case mapping_device::DIAL_ANTICLOCKWISE: return QObject::tr("R5");
	case mapping_device::SELECT: return QObject::tr("Select");
	case mapping_device::PAUSE: return QObject::tr("Pause");
	case mapping_device::SHIFTER_1: return QObject::tr("Gear 1");
	case mapping_device::SHIFTER_2: return QObject::tr("Gear 2");
	case mapping_device::SHIFTER_3: return QObject::tr("Gear 3");
	case mapping_device::SHIFTER_4: return QObject::tr("Gear 4");
	case mapping_device::SHIFTER_5: return QObject::tr("Gear 5");
	case mapping_device::SHIFTER_6: return QObject::tr("Gear 6");
	case mapping_device::SHIFTER_R: return QObject::tr("Gear R");
	case mapping_device::SHIFTER_PRESS: return QObject::tr("Shifter press");
	case mapping_device::COUNT: return "";
	}
	return "";
}

emulated_logitech_g27_mapping& device_cfg(mapping_device dev)
{
	auto& cfg = g_cfg_logitech_g27;

	switch (dev)
	{
	case mapping_device::STEERING: return cfg.steering;
	case mapping_device::THROTTLE: return cfg.throttle;
	case mapping_device::BRAKE: return cfg.brake;
	case mapping_device::CLUTCH: return cfg.clutch;
	case mapping_device::SHIFT_UP: return cfg.shift_up;
	case mapping_device::SHIFT_DOWN: return cfg.shift_down;
	case mapping_device::UP: return cfg.up;
	case mapping_device::DOWN: return cfg.down;
	case mapping_device::LEFT: return cfg.left;
	case mapping_device::RIGHT: return cfg.right;
	case mapping_device::TRIANGLE: return cfg.triangle;
	case mapping_device::CROSS: return cfg.cross;
	case mapping_device::SQUARE: return cfg.square;
	case mapping_device::CIRCLE: return cfg.circle;
	case mapping_device::L2: return cfg.l2;
	case mapping_device::L3: return cfg.l3;
	case mapping_device::R2: return cfg.r2;
	case mapping_device::R3: return cfg.r3;
	case mapping_device::PLUS: return cfg.plus;
	case mapping_device::MINUS: return cfg.minus;
	case mapping_device::DIAL_CLOCKWISE: return cfg.dial_clockwise;
	case mapping_device::DIAL_ANTICLOCKWISE: return cfg.dial_anticlockwise;
	case mapping_device::SELECT: return cfg.select;
	case mapping_device::PAUSE: return cfg.pause;
	case mapping_device::SHIFTER_1: return cfg.shifter_1;
	case mapping_device::SHIFTER_2: return cfg.shifter_2;
	case mapping_device::SHIFTER_3: return cfg.shifter_3;
	case mapping_device::SHIFTER_4: return cfg.shifter_4;
	case mapping_device::SHIFTER_5: return cfg.shifter_5;
	case mapping_device::SHIFTER_6: return cfg.shifter_6;
	case mapping_device::SHIFTER_R: return cfg.shifter_r;
	case mapping_device::SHIFTER_PRESS: return cfg.shifter_press;
	default: fmt::throw_exception("Unexpected mapping_device %d", static_cast<int>(dev));
	}
}

class DeviceChoice : public QWidget
{
public:
	DeviceChoice(QWidget* parent, const QString& name)
		: QWidget(parent)
	{
		auto layout = new QHBoxLayout(this);
		setLayout(layout);

		QLabel* label = new QLabel(this);
		label->setText(name);
		label->setMinimumWidth(400);
		layout->addWidget(label);

		m_dropdown = new QComboBox(this);
		layout->addWidget(m_dropdown);

		m_disable_button = new QPushButton(tr("DISABLE"), this);
		layout->addWidget(m_disable_button);

		connect(m_dropdown, &QComboBox::currentIndexChanged, this, [this](int index)
		{
			if (index < 0) return;

			const QVariant var = m_dropdown->currentData();
			if (!var.canConvert<int>()) return;

			m_device_choice = static_cast<mapping_device>(var.toInt());
			update_display();
		});

		connect(m_disable_button, &QPushButton::clicked, this, [this]()
		{
			m_device_choice = mapping_device::NONE;
			update_display();
		});

		m_dropdown->setPlaceholderText(tr("-- Disabled --"));

		for (int i = 0; i < static_cast<int>(mapping_device::COUNT); i++)
		{
			const mapping_device dev = static_cast<mapping_device>(i);
			m_dropdown->addItem(device_name(dev), i);
		}

		update_display();
	}

	mapping_device get_device_choice() const
	{
		return m_device_choice;
	}

	void set_device_choice(mapping_device choice)
	{
		m_device_choice = choice;
		update_display();
	}

	void set_enable(bool enable)
	{
		m_dropdown->setEnabled(enable);
		m_disable_button->setEnabled(enable);
	}

private:
	void update_display()
	{
		m_dropdown->setCurrentIndex(static_cast<int>(m_device_choice));
	}

	mapping_device m_device_choice = mapping_device::NONE;
	QComboBox* m_dropdown = nullptr;
	QPushButton* m_disable_button = nullptr;
};

class Mapping : public QGroupBox
{
public:
	Mapping(QWidget* parent, emulated_logitech_g27_settings_dialog* dialog, bool is_axis, const QString& name, bool flip_axis_display)
		: QGroupBox(parent), m_setting_dialog(dialog), m_is_axis(is_axis), m_name(name), m_flip_axis_display(flip_axis_display)
	{
		QVBoxLayout* layout = new QVBoxLayout(this);
		setLayout(layout);

		QWidget* horizontal_container = new QWidget(this);
		QHBoxLayout* horizontal_layout = new QHBoxLayout(horizontal_container);
		horizontal_container->setLayout(horizontal_layout);

		layout->addWidget(horizontal_container);

		QLabel* label = new QLabel(horizontal_container);
		label->setText(name);

		m_display_box = new QLabel(horizontal_container);
		m_display_box->setTextFormat(Qt::RichText);
		m_display_box->setWordWrap(true);
		m_display_box->setFrameStyle(QFrame::Box);
		m_display_box->setMinimumWidth(225);

		m_map_button = new QPushButton(tr("MAP"), horizontal_container);
		m_unmap_button = new QPushButton(tr("UNMAP"), horizontal_container);
		m_reverse_checkbox = new QCheckBox(tr("Reverse"), horizontal_container);

		if (!m_is_axis)
		{
			m_button_status = new QCheckBox(tr("Pressed"), horizontal_container);
			m_button_status->setDisabled(true);
		}
		else
		{
			m_axis_status = new QSlider(Qt::Horizontal, this);
			m_axis_status->setDisabled(true);
			m_axis_status->setMinimum(-0x8000);
			m_axis_status->setMaximum(0x7FFF);
			m_axis_status->setValue(-0x8000);
		}

		update_display();

		horizontal_layout->addWidget(label, 1);
		horizontal_layout->addWidget(m_display_box, 2);
		if (m_button_status)
			horizontal_layout->addWidget(m_button_status, 1);
		else
			horizontal_layout->addStretch(1); // For a more consistent layout
		horizontal_layout->addWidget(m_map_button, 1);
		horizontal_layout->addWidget(m_unmap_button, 1);
		horizontal_layout->addWidget(m_reverse_checkbox, 1);

		if (m_axis_status)
			layout->addWidget(m_axis_status);

		connect(m_map_button, &QPushButton::clicked, this, [this]()
		{
			m_mapping_in_progress = true;
			m_timeout_msec = 5500;
			m_setting_dialog->set_enable(false);
			m_last_joystick_states = m_setting_dialog->get_joystick_states();
		});

		connect(m_unmap_button, &QPushButton::clicked, this, [this]()
		{
			m_mapping.device_type_id = 0;
			update_display();
		});

		connect(m_reverse_checkbox, &QCheckBox::clicked, this, [this]()
		{
			m_mapping.reverse = m_reverse_checkbox->isChecked();
		});

		m_tick_timer = new QTimer(this);
		connect(m_tick_timer, &QTimer::timeout, this, [this]()
		{
			if (m_mapping_in_progress)
			{
				const int timeout_sec = m_timeout_msec / 1000;
				const std::map<u64, joystick_state>& new_joystick_states = m_setting_dialog->get_joystick_states();

				m_setting_dialog->set_state_text(tr("Input %0 for %1, timeout in %2 %3").arg(m_is_axis ? tr("axis") : tr("button/hat")).arg(m_name).arg(timeout_sec).arg(timeout_sec >= 2 ? tr("seconds") : tr("second")));

				for (auto& [device_type_id, new_joystick_state] : new_joystick_states)
				{
					const auto last_joystick_state = m_last_joystick_states.find(device_type_id);
					if (last_joystick_state == m_last_joystick_states.cend())
					{
						continue;
					}

					if (m_is_axis)
					{
						constexpr s16 axis_change_threshold = 0x7FFF / 5;
						if (last_joystick_state->second.axes.size() != new_joystick_state.axes.size())
						{
							logitech_g27_cfg_log.error("During input state change diff, number of axes on %04x:%04x changed", (device_type_id >> 16) & 0xFFFF, device_type_id & 0xFFFF);
							continue;
						}
						for (usz i = 0; i < new_joystick_state.axes.size(); i++)
						{
							const s32 diff = std::abs(last_joystick_state->second.axes[i] - new_joystick_state.axes[i]);
							if (diff > axis_change_threshold)
							{
								m_mapping_in_progress = false;
								m_setting_dialog->set_state_text(DEFAULT_STATUS);
								m_setting_dialog->set_enable(true);
								m_mapping.device_type_id = device_type_id;
								m_mapping.type = sdl_mapping_type::axis;
								m_mapping.id = i;
								m_mapping.hat = hat_component::none;
								break;
							}
						}
					}
					else
					{
						if (last_joystick_state->second.buttons.size() != new_joystick_state.buttons.size())
						{
							logitech_g27_cfg_log.error("during input state change diff, number of buttons on %04x:%04x changed", (device_type_id >> 16) & 0xFFFF, device_type_id & 0xFFFF);
							continue;
						}
						if (last_joystick_state->second.hats.size() != new_joystick_state.hats.size())
						{
							logitech_g27_cfg_log.error("during input state change diff, number of hats on %04x:%04x changed", (device_type_id >> 16) & 0xFFFF, device_type_id & 0xFFFF);
							continue;
						}
						for (usz i = 0; i < new_joystick_state.buttons.size(); i++)
						{
							if (last_joystick_state->second.buttons[i] != new_joystick_state.buttons[i])
							{
								m_mapping_in_progress = false;
								m_setting_dialog->set_state_text(DEFAULT_STATUS);
								m_setting_dialog->set_enable(true);
								m_mapping.device_type_id = device_type_id;
								m_mapping.type = sdl_mapping_type::button;
								m_mapping.id = i;
								m_mapping.hat = hat_component::none;
								break;
							}
						}
						for (usz i = 0; i < new_joystick_state.hats.size(); i++)
						{
							if (last_joystick_state->second.hats[i] != new_joystick_state.hats[i] && new_joystick_state.hats[i] != hat_component::none)
							{
								m_mapping_in_progress = false;
								m_setting_dialog->set_state_text(DEFAULT_STATUS);
								m_setting_dialog->set_enable(true);
								m_mapping.device_type_id = device_type_id;
								m_mapping.type = sdl_mapping_type::hat;
								m_mapping.id = i;
								m_mapping.hat = new_joystick_state.hats[i];
								break;
							}
						}
					}
				}

				m_timeout_msec -= 25;
				if (m_timeout_msec <= 0)
				{
					m_mapping_in_progress = false;
					m_setting_dialog->set_state_text(DEFAULT_STATUS);
					m_setting_dialog->set_enable(true);
				}
			}

			update_display();
		});
		m_tick_timer->start(25);
	}

	void set_enable(bool enable)
	{
		m_map_button->setEnabled(enable);
		m_unmap_button->setEnabled(enable);
		m_reverse_checkbox->setEnabled(enable);
	}

	void set_mapping(const sdl_mapping& mapping)
	{
		m_mapping = mapping;
		update_display();
	}

	const sdl_mapping& get_mapping() const
	{
		return m_mapping;
	}

private:
	emulated_logitech_g27_settings_dialog* m_setting_dialog = nullptr;
	DeviceChoice* m_ffb_device = nullptr;
	DeviceChoice* m_led_device = nullptr;
	sdl_mapping m_mapping{};
	bool m_is_axis = false;
	QString m_name;
	bool m_flip_axis_display = false;

	QLabel* m_display_box = nullptr;
	QPushButton* m_map_button = nullptr;
	QPushButton* m_unmap_button = nullptr;
	QCheckBox* m_reverse_checkbox = nullptr;

	bool m_mapping_in_progress = false;
	int m_timeout_msec = 5500;
	QTimer* m_tick_timer = nullptr;
	std::map<u64, joystick_state> m_last_joystick_states;

	QCheckBox* m_button_status = nullptr;
	QSlider* m_axis_status = nullptr;

	void update_display()
	{
		const std::string text = fmt::format("%04x:%04x (0x%08x), %s %u %s", (m_mapping.device_type_id >> 16) & 0xFFFF, m_mapping.device_type_id & 0xFFFF, m_mapping.device_type_id >> 32, m_mapping.type, m_mapping.id, m_mapping.hat);
		m_display_box->setText(QString::fromStdString(text));

		m_reverse_checkbox->setChecked(m_mapping.reverse);

		if (m_button_status)
			m_button_status->setChecked(m_mapping.reverse);

		if (m_axis_status)
		{
			s32 axis_value = -0x8000;
			if (m_mapping.reverse)
				axis_value *= -1;
			if (m_flip_axis_display)
				axis_value *= -1;
			m_axis_status->setValue(std::clamp(axis_value, -0x8000, 0x7FFF));
		}

		const std::map<u64, joystick_state>& joystick_states = m_setting_dialog->get_joystick_states();
		auto joystick_state = joystick_states.find(m_mapping.device_type_id);

		if (joystick_state != joystick_states.end())
		{
			switch (m_mapping.type)
			{
			case sdl_mapping_type::button:
			{
				if (joystick_state->second.buttons.size() <= m_mapping.id)
					break;
				bool value = joystick_state->second.buttons[m_mapping.id];
				if (m_mapping.reverse)
					value = !value;
				m_button_status->setChecked(value);
				break;
			}
			case sdl_mapping_type::hat:
			{
				if (joystick_state->second.hats.size() <= m_mapping.id)
					break;
				bool value = joystick_state->second.hats[m_mapping.id] == m_mapping.hat;
				if (m_mapping.reverse)
					value = !value;
				m_button_status->setChecked(value);
				break;
			}
			case sdl_mapping_type::axis:
			{
				if (joystick_state->second.axes.size() <= m_mapping.id)
					break;
				s32 value = joystick_state->second.axes[m_mapping.id];
				if (m_mapping.reverse)
					value *= -1;
				if (m_flip_axis_display)
					value *= -1;
				m_axis_status->setValue(std::clamp(value, -0x8000, 0x7FFF));
				break;
			}
			}
		}
	}
};

void emulated_logitech_g27_settings_dialog::save_ui_state_to_config()
{
	const auto save_mapping = [this](mapping_device device_choice)
	{
		emulated_logitech_g27_mapping& mapping = device_cfg(device_choice);
		const sdl_mapping& m = ::at32(m_mappings, device_choice)->get_mapping();
		mapping.device_type_id.set(m.device_type_id);
		mapping.type.set(m.type);
		mapping.id.set(m.id);
		mapping.hat.set(m.hat);
		mapping.reverse.set(m.reverse);

		if (m_ffb_device->get_device_choice() == device_choice)
		{
			g_cfg_logitech_g27.ffb_device_type_id.set(m.device_type_id);
		}
		if (m_led_device->get_device_choice() == device_choice)
		{
			g_cfg_logitech_g27.led_device_type_id.set(m.device_type_id);
		}
	};

	for (int i = 0; i < static_cast<int>(mapping_device::COUNT); i++)
	{
		save_mapping(static_cast<mapping_device>(i));
	}

	g_cfg_logitech_g27.enabled.set(m_enabled->isChecked());
	g_cfg_logitech_g27.reverse_effects.set(m_reverse_effects->isChecked());

	if (m_ffb_device->get_device_choice() == mapping_device::NONE)
	{
		g_cfg_logitech_g27.ffb_device_type_id.set(0);
	}

	if (m_led_device->get_device_choice() == mapping_device::NONE)
	{
		g_cfg_logitech_g27.led_device_type_id.set(0);
	}
}

void emulated_logitech_g27_settings_dialog::load_ui_state_from_config()
{
	const auto load_mapping = [this](mapping_device device_choice)
	{
		const emulated_logitech_g27_mapping& mapping = device_cfg(device_choice);
		const sdl_mapping m =
		{
			.device_type_id = mapping.device_type_id.get(),
			.type = mapping.type.get(),
			.id = mapping.id.get(),
			.hat = mapping.hat.get(),
			.reverse = mapping.reverse.get(),
			.positive_axis = false
		};

		::at32(m_mappings, device_choice)->set_mapping(m);

		const u64 ffb_device_type_id = g_cfg_logitech_g27.ffb_device_type_id.get();
		const u64 led_device_type_id = g_cfg_logitech_g27.led_device_type_id.get();

		if (ffb_device_type_id == m.device_type_id && m_ffb_device->get_device_choice() == mapping_device::NONE)
		{
			m_ffb_device->set_device_choice(device_choice);
		}
		if (led_device_type_id == m.device_type_id && m_led_device->get_device_choice() == mapping_device::NONE)
		{
			m_led_device->set_device_choice(device_choice);
		}
	};

	for (int i = 0; i < static_cast<int>(mapping_device::COUNT); i++)
	{
		load_mapping(static_cast<mapping_device>(i));
	}

	m_enabled->setChecked(g_cfg_logitech_g27.enabled.get());
	m_reverse_effects->setChecked(g_cfg_logitech_g27.reverse_effects.get());
}

emulated_logitech_g27_settings_dialog::emulated_logitech_g27_settings_dialog(QWidget* parent)
	: QDialog(parent)
{
	setObjectName("emulated_logitech_g27_settings_dialog");
	setWindowTitle(tr("Configure Emulated Logitech G27 Wheel"));
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_StyledBackground);
	setModal(true);

	QVBoxLayout* v_layout = new QVBoxLayout(this);

	QDialogButtonBox* buttons = new QDialogButtonBox(this);
	buttons->setStandardButtons(QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Save | QDialogButtonBox::RestoreDefaults);

	g_cfg_logitech_g27.load();

	connect(buttons, &QDialogButtonBox::clicked, this, [this, buttons](QAbstractButton* button)
	{
		if (button == buttons->button(QDialogButtonBox::Apply))
		{
			save_ui_state_to_config();
			g_cfg_logitech_g27.save();
			load_ui_state_from_config();
		}
		else if (button == buttons->button(QDialogButtonBox::Save))
		{
			save_ui_state_to_config();
			g_cfg_logitech_g27.save();
			accept();
		}
		else if (button == buttons->button(QDialogButtonBox::RestoreDefaults))
		{
			if (QMessageBox::question(this, tr("Confirm Reset"), tr("Reset all?")) != QMessageBox::Yes)
				return;
			g_cfg_logitech_g27.reset();
			load_ui_state_from_config();
		}
		else if (button == buttons->button(QDialogButtonBox::Cancel))
		{
			reject();
		}
	});

	QLabel* warning = new QLabel(tr("Warning: Force feedback output were meant for Logitech G27, on stronger wheels please adjust force strength accordingly in your wheel software."), this);
	warning->setStyleSheet(QString("color: %0;").arg(gui::utils::get_label_color("emulated_logitech_g27_warning_label", Qt::red, Qt::red).name()));
	warning->setWordWrap(true);
	v_layout->addWidget(warning);

	QLabel* mapping_note = new QLabel(tr("Note: Please DO NOT map your wheel onto gamepads, only map it here. If your wheel was mapped onto gamepads, go to gamepad settings and unmap it. If you used vJoy to map your wheel onto a gamepad before for RPCS3, undo that."), this);
	mapping_note->setWordWrap(true);
	v_layout->addWidget(mapping_note);

	m_enabled = new QCheckBox(tr("Enabled (requires game restart)"), this);
	v_layout->addWidget(m_enabled);

	m_reverse_effects = new QCheckBox(tr("Reverse force feedback effects"), this);
	v_layout->addWidget(m_reverse_effects);

	m_state_text = new QLabel(DEFAULT_STATUS, this);
	v_layout->addWidget(m_state_text);

	m_ffb_device = new DeviceChoice(this, tr("Use the device with the following mapping for force feedback:"));
	m_led_device = new DeviceChoice(this, tr("Use the device with the following mapping for LED:"));

	m_mapping_scroll_area = new QScrollArea(this);
	QWidget* mapping_widget = new QWidget(m_mapping_scroll_area);
	QVBoxLayout* mapping_layout = new QVBoxLayout(mapping_widget);
	mapping_widget->setLayout(mapping_layout);
	m_mapping_scroll_area->setWidget(mapping_widget);
	m_mapping_scroll_area->setWidgetResizable(true);
	m_mapping_scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_mapping_scroll_area->setMinimumHeight(400);
	m_mapping_scroll_area->setMinimumWidth(700);

	v_layout->addWidget(m_mapping_scroll_area);

	QLabel* axis_label = new QLabel(tr("Axes:"), mapping_widget);
	mapping_layout->addWidget(axis_label);

	const auto add_axis = [this, mapping_widget, mapping_layout](mapping_device dev, bool flip_axis_display)
	{
		m_mappings[dev] = new Mapping(mapping_widget, this, true, device_name(dev), flip_axis_display);
		mapping_layout->addWidget(m_mappings[dev]);
	};

	const auto add_button = [this, mapping_widget, mapping_layout](mapping_device dev)
	{
		m_mappings[dev] = new Mapping(mapping_widget, this, false, device_name(dev), false);
		mapping_layout->addWidget(m_mappings[dev]);
	};

	add_axis(mapping_device::STEERING, false);
	add_axis(mapping_device::THROTTLE, true);
	add_axis(mapping_device::BRAKE, true);
	add_axis(mapping_device::CLUTCH, true);

	QLabel* button_label = new QLabel(tr("Buttons:"), mapping_widget);
	mapping_layout->addWidget(button_label);

	for (int i = static_cast<int>(mapping_device::FIRST_BUTTON); i < static_cast<int>(mapping_device::COUNT); i++)
	{
		add_button(static_cast<mapping_device>(i));
	}

	v_layout->addSpacing(20);

	v_layout->addWidget(m_ffb_device);
	v_layout->addWidget(m_led_device);

	v_layout->addWidget(buttons);
	setLayout(v_layout);

	m_sdl_initialized = sdl_instance::get_instance().initialize();

	load_ui_state_from_config();

	if (m_sdl_initialized)
		get_joystick_states();
}

emulated_logitech_g27_settings_dialog::~emulated_logitech_g27_settings_dialog()
{
	for (SDL_Joystick* joystick_handle : m_joystick_handles)
	{
		if (joystick_handle)
			SDL_CloseJoystick(joystick_handle);
	}
}

static inline hat_component get_sdl_hat_component(u8 sdl_hat)
{
	if (sdl_hat & SDL_HAT_UP)
	{
		return hat_component::up;
	}

	if (sdl_hat & SDL_HAT_DOWN)
	{
		return hat_component::down;
	}

	if (sdl_hat & SDL_HAT_LEFT)
	{
		return hat_component::left;
	}

	if (sdl_hat & SDL_HAT_RIGHT)
	{
		return hat_component::right;
	}

	return hat_component::none;
}

const std::map<u64, joystick_state>& emulated_logitech_g27_settings_dialog::get_joystick_states()
{
	if (!m_sdl_initialized)
	{
		return m_last_joystick_states;
	}

	const u64 now = SDL_GetTicks();

	if (now - m_last_joystick_states_update < 25)
	{
		return m_last_joystick_states;
	}

	m_last_joystick_states_update = now;

	std::map<u64, joystick_state> new_joystick_states;
	std::vector<SDL_Joystick*> new_joystick_handles;

	sdl_instance::get_instance().pump_events();

	int joystick_count = 0;
	if (SDL_JoystickID* joystick_ids = SDL_GetJoysticks(&joystick_count))
	{
		for (int i = 0; i < joystick_count; i++)
		{
			SDL_Joystick* cur_joystick = SDL_OpenJoystick(joystick_ids[i]);
			if (!cur_joystick)
			{
				continue;
			}
			new_joystick_handles.push_back(cur_joystick);

			const int num_axes = SDL_GetNumJoystickAxes(cur_joystick);
			const int num_buttons = SDL_GetNumJoystickButtons(cur_joystick);
			const int num_hats = SDL_GetNumJoystickHats(cur_joystick);
			const emulated_g27_device_type_id device_type_id_struct =
			{
				.product_id = static_cast<u64>(SDL_GetJoystickProduct(cur_joystick)),
				.vendor_id = static_cast<u64>(SDL_GetJoystickVendor(cur_joystick)),
				.num_axes = static_cast<u64>(num_axes),
				.num_hats = static_cast<u64>(num_hats),
				.num_buttons = static_cast<u64>(num_buttons)
			};
			const u64 device_type_id = device_type_id_struct.as_u64();

			auto cur_state = new_joystick_states.find(device_type_id);
			if (cur_state == new_joystick_states.end())
			{
				joystick_state s {};
				for (int j = 0; j < num_axes; j++)
				{
					s.axes.push_back(SDL_GetJoystickAxis(cur_joystick, j));
				}
				for (int j = 0; j < num_buttons; j++)
				{
					s.buttons.push_back(SDL_GetJoystickButton(cur_joystick, j));
				}
				for (int j = 0; j < num_hats; j++)
				{
					const u8 sdl_hat = SDL_GetJoystickHat(cur_joystick, j);
					s.hats.push_back(get_sdl_hat_component(sdl_hat));
				}
				new_joystick_states[device_type_id] = std::move(s);
			}
			else
			{
				for (s32 j = 0; j < static_cast<s32>(cur_state->second.axes.size()); j++)
				{
					cur_state->second.axes[j] = (cur_state->second.axes[j] + SDL_GetJoystickAxis(cur_joystick, j)) / 2;
				}
				for (s32 j = 0; j < static_cast<s32>(cur_state->second.buttons.size()); j++)
				{
					cur_state->second.buttons[j] = cur_state->second.buttons[j] || SDL_GetJoystickButton(cur_joystick, j);
				}
				for (s32 j = 0; j < static_cast<s32>(cur_state->second.hats.size()); j++)
				{
					if (cur_state->second.hats[j] != hat_component::none)
						continue;
					const u8 sdl_hat = SDL_GetJoystickHat(cur_joystick, j);
					cur_state->second.hats[j] = get_sdl_hat_component(sdl_hat);
				}
			}
		}
	}

	for (SDL_Joystick* joystick_handle : m_joystick_handles)
	{
		if (joystick_handle)
			SDL_CloseJoystick(joystick_handle);
	}

	m_joystick_handles = new_joystick_handles;
	m_last_joystick_states = new_joystick_states;

	return m_last_joystick_states;
}

void emulated_logitech_g27_settings_dialog::set_state_text(const QString& text)
{
	m_state_text->setText(text);
}

void emulated_logitech_g27_settings_dialog::set_enable(bool enable)
{
	const int slider_position = m_mapping_scroll_area->verticalScrollBar()->sliderPosition();

	for (auto& [dev, mapping] : m_mappings)
	{
		mapping->set_enable(enable);
	}

	m_enabled->setEnabled(enable);
	m_reverse_effects->setEnabled(enable);

	m_ffb_device->set_enable(enable);
	m_led_device->set_enable(enable);

	m_mapping_scroll_area->verticalScrollBar()->setEnabled(enable);
	m_mapping_scroll_area->verticalScrollBar()->setSliderPosition(slider_position);
}

#endif
