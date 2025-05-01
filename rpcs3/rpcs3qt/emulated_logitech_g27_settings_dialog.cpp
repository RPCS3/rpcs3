#ifdef HAVE_SDL3

#include "stdafx.h"

#include "emulated_logitech_g27_settings_dialog.h"
#include "Emu/Io/LogitechG27.cpp"
#include "Input/sdl_instance.h"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QTimer>
#include <QSlider>

LOG_CHANNEL(logitech_g27_cfg_log, "LOGIG27");

#define DEFAULT_STATUS " "

struct joystick_state
{
	std::vector<int16_t> axes;
	std::vector<bool> buttons;
	std::vector<hat_component> hats;
};

class DeviceChoice : public QWidget
{

public:
	DeviceChoice(QWidget* parent, uint32_t device_type_id, const char* name)
		: QWidget(parent), m_device_type_id(device_type_id)
	{
		auto layout = new QHBoxLayout(this);

		QLabel* label = new QLabel(this);
		label->setText(QString(name));
		layout->addWidget(label);
		setLayout(layout);

		m_display_box = new QLabel(this);
		m_display_box->setTextFormat(Qt::RichText);
		m_display_box->setWordWrap(true);
		m_display_box->setFrameStyle(QFrame::Box);
		m_display_box->setMinimumWidth(100);
		layout->addWidget(m_display_box);

		update_display();
	}

	uint32_t get_device_type_id()
	{
		return m_device_type_id;
	}

	void set_device_type_id(uint32_t device_type_id)
	{
		this->m_device_type_id = device_type_id;
		update_display();
	}

private:
	uint32_t m_device_type_id = 0;
	QLabel* m_display_box = nullptr;
	void update_display()
	{
		char text_buf[32];
		sprintf(text_buf, "%04x:%04x", m_device_type_id >> 16, m_device_type_id & 0xFFFF);
		m_display_box->setText(QString(text_buf));
	}
};

class Mapping : public QGroupBox
{

public:
	Mapping(QWidget* parent, emulated_logitech_g27_settings_dialog* dialog, DeviceChoice* ffb_device, DeviceChoice* led_device, sdl_mapping mapping, bool is_axis, const char* name, bool flip_axis_display)
		: QGroupBox(parent), m_setting_dialog(dialog), m_ffb_device(ffb_device), m_led_device(led_device), m_mapping(mapping), m_is_axis(is_axis), m_name(name), m_flip_axis_display(flip_axis_display)
	{
		QVBoxLayout* layout = new QVBoxLayout(this);
		setLayout(layout);

		QWidget* horizontal_container = new QWidget(this);
		QHBoxLayout* horizontal_layout = new QHBoxLayout(horizontal_container);
		horizontal_container->setLayout(horizontal_layout);

		layout->addWidget(horizontal_container);

		QLabel* label = new QLabel(horizontal_container);
		label->setText(QString(name));

		m_display_box = new QLabel(horizontal_container);
		m_display_box->setTextFormat(Qt::RichText);
		m_display_box->setWordWrap(true);
		m_display_box->setFrameStyle(QFrame::Box);
		m_display_box->setMinimumWidth(150);

		m_ffb_set_button = new QPushButton(QString("FFB"), horizontal_container);
		m_led_set_button = new QPushButton(QString("LED"), horizontal_container);
		m_map_button = new QPushButton(QString("MAP"), horizontal_container);
		m_unmap_button = new QPushButton(QString("UNMAP"), horizontal_container);
		m_reverse_checkbox = new QCheckBox(QString("Reverse"), horizontal_container);

		if (!this->m_is_axis)
		{
			m_axis_status = nullptr;
			m_button_status = new QCheckBox(QString("Pressed"), horizontal_container);
			m_button_status->setDisabled(true);
		}
		else
		{
			m_button_status = nullptr;
			m_axis_status = new QSlider(Qt::Horizontal, this);
			m_axis_status->setDisabled(true);
			m_axis_status->setMinimum(-0x8000);
			m_axis_status->setMaximum(0x7FFF);
			m_axis_status->setValue(-0x8000);
		}

		update_display();

		horizontal_layout->addWidget(label);
		horizontal_layout->addWidget(m_display_box);
		if (!this->m_is_axis)
			horizontal_layout->addWidget(m_button_status);
		horizontal_layout->addWidget(m_ffb_set_button);
		horizontal_layout->addWidget(m_led_set_button);
		horizontal_layout->addWidget(m_map_button);
		horizontal_layout->addWidget(m_unmap_button);
		horizontal_layout->addWidget(m_reverse_checkbox);

		if (this->m_is_axis)
			layout->addWidget(m_axis_status);

		connect(m_ffb_set_button, &QPushButton::clicked, this, [this]()
			{
				this->m_ffb_device->set_device_type_id(this->m_mapping.device_type_id);
			});

		connect(m_led_set_button, &QPushButton::clicked, this, [this]()
			{
				this->m_led_device->set_device_type_id(this->m_mapping.device_type_id);
			});

		connect(m_map_button, &QPushButton::clicked, this, [this]()
			{
				this->m_mapping_in_progress = true;
				this->m_timeout_msec = 5500;
				this->m_setting_dialog->disable();
				this->m_last_joystick_states = this->m_setting_dialog->get_joystick_states();
			});

		connect(m_unmap_button, &QPushButton::clicked, this, [this]()
			{
				this->m_mapping.device_type_id = 0;
				update_display();
			});

		connect(m_reverse_checkbox, &QCheckBox::clicked, this, [this]()
			{
				this->m_mapping.reverse = this->m_reverse_checkbox->isChecked();
			});

		m_tick_timer = new QTimer(this);
		connect(m_tick_timer, &QTimer::timeout, this, [this]()
			{
				if (this->m_mapping_in_progress)
				{
					char text_buf[128];

					int timeout_sec = this->m_timeout_msec / 1000;

					const std::map<uint32_t, joystick_state>& new_joystick_states = this->m_setting_dialog->get_joystick_states();

					sprintf(text_buf, "Input %s for %s, timeout in %d %s", this->m_is_axis ? "axis" : "button/hat", this->m_name.c_str(), timeout_sec, timeout_sec >= 2 ? "seconds" : "second");
					this->m_setting_dialog->set_state_text(text_buf);

					for (auto new_joystick_state = new_joystick_states.begin(); new_joystick_state != new_joystick_states.end(); new_joystick_state++)
					{
						auto last_joystick_state = this->m_last_joystick_states.find(new_joystick_state->first);
						if (last_joystick_state == this->m_last_joystick_states.end())
						{
							continue;
						}

						if (this->m_is_axis)
						{
							const static int16_t axis_change_threshold = 0x7FFF / 5;
							if (last_joystick_state->second.axes.size() != new_joystick_state->second.axes.size())
							{
								logitech_g27_cfg_log.error("during input state change diff, number of axes on %04x:%04x changed", new_joystick_state->first >> 16, new_joystick_state->first & 0xFFFF);
								continue;
							}
							for (std::vector<int16_t>::size_type i = 0; i < new_joystick_state->second.axes.size(); i++)
							{
								int32_t diff = std::abs(last_joystick_state->second.axes[i] - new_joystick_state->second.axes[i]);
								if (diff > axis_change_threshold)
								{
									this->m_mapping_in_progress = false;
									this->m_setting_dialog->set_state_text(DEFAULT_STATUS);
									this->m_setting_dialog->enable();
									this->m_mapping.device_type_id = new_joystick_state->first;
									this->m_mapping.type = MAPPING_AXIS;
									this->m_mapping.id = i;
									this->m_mapping.hat = HAT_NONE;
									break;
								}
							}
						}
						else
						{
							if (last_joystick_state->second.buttons.size() != new_joystick_state->second.buttons.size())
							{
								logitech_g27_cfg_log.error("during input state change diff, number of buttons on %04x:%04x changed", new_joystick_state->first >> 16, new_joystick_state->first & 0xFFFF);
								continue;
							}
							if (last_joystick_state->second.hats.size() != new_joystick_state->second.hats.size())
							{
								logitech_g27_cfg_log.error("during input state change diff, number of hats on %04x:%04x changed", new_joystick_state->first >> 16, new_joystick_state->first & 0xFFFF);
								continue;
							}
							for (std::vector<int16_t>::size_type i = 0; i < new_joystick_state->second.buttons.size(); i++)
							{
								if (last_joystick_state->second.buttons[i] != new_joystick_state->second.buttons[i])
								{
									this->m_mapping_in_progress = false;
									this->m_setting_dialog->set_state_text(DEFAULT_STATUS);
									this->m_setting_dialog->enable();
									this->m_mapping.device_type_id = new_joystick_state->first;
									this->m_mapping.type = MAPPING_BUTTON;
									this->m_mapping.id = i;
									this->m_mapping.hat = HAT_NONE;
									break;
								}
							}
							for (std::vector<int16_t>::size_type i = 0; i < new_joystick_state->second.hats.size(); i++)
							{
								if (last_joystick_state->second.hats[i] != new_joystick_state->second.hats[i] && new_joystick_state->second.hats[i] != HAT_NONE)
								{
									this->m_mapping_in_progress = false;
									this->m_setting_dialog->set_state_text(DEFAULT_STATUS);
									this->m_setting_dialog->enable();
									this->m_mapping.device_type_id = new_joystick_state->first;
									this->m_mapping.type = MAPPING_HAT;
									this->m_mapping.id = i;
									this->m_mapping.hat = new_joystick_state->second.hats[i];
									break;
								}
							}
						}
					}

					this->m_timeout_msec = this->m_timeout_msec - 25;
					if (this->m_timeout_msec <= 0)
					{
						this->m_mapping_in_progress = false;
						this->m_setting_dialog->set_state_text(DEFAULT_STATUS);
						this->m_setting_dialog->enable();
					}
				}

				update_display();
			});
		m_tick_timer->start(25);
	}

	void disable()
	{
		m_ffb_set_button->setDisabled(true);
		m_led_set_button->setDisabled(true);
		m_map_button->setDisabled(true);
		m_unmap_button->setDisabled(true);
		m_reverse_checkbox->setDisabled(true);
	}

	void enable()
	{
		m_ffb_set_button->setEnabled(true);
		m_led_set_button->setEnabled(true);
		m_map_button->setEnabled(true);
		m_unmap_button->setEnabled(true);
		m_reverse_checkbox->setEnabled(true);
	}

	void set_mapping(const sdl_mapping& mapping)
	{
		this->m_mapping = mapping;
		update_display();
	}

	const sdl_mapping& get_mapping()
	{
		return m_mapping;
	}

private:
	emulated_logitech_g27_settings_dialog* m_setting_dialog = nullptr;
	DeviceChoice* m_ffb_device = nullptr;
	DeviceChoice* m_led_device = nullptr;
	sdl_mapping m_mapping = {0};
	bool m_is_axis = false;
	std::string m_name = "";
	bool m_flip_axis_display = false;

	QLabel* m_display_box = nullptr;
	QPushButton* m_ffb_set_button = nullptr;
	QPushButton* m_led_set_button = nullptr;
	QPushButton* m_map_button = nullptr;
	QPushButton* m_unmap_button = nullptr;
	QCheckBox* m_reverse_checkbox = nullptr;

	bool m_mapping_in_progress = false;
	int m_timeout_msec = 5500;
	QTimer* m_tick_timer = nullptr;
	std::map<uint32_t, joystick_state> m_last_joystick_states;

	QCheckBox* m_button_status = nullptr;
	QSlider* m_axis_status = nullptr;

	void update_display()
	{
		char text_buf[64];
		const char* type_string = nullptr;
		switch (m_mapping.type)
		{
		case MAPPING_BUTTON:
			type_string = "button";
			break;
		case MAPPING_HAT:
			type_string = "hat";
			break;
		case MAPPING_AXIS:
			type_string = "axis";
			break;
		}
		const char* hat_string = nullptr;
		switch (m_mapping.hat)
		{
		case HAT_UP:
			hat_string = "up";
			break;
		case HAT_DOWN:
			hat_string = "down";
			break;
		case HAT_LEFT:
			hat_string = "left";
			break;
		case HAT_RIGHT:
			hat_string = "right";
			break;
		case HAT_NONE:
			hat_string = "";
			break;
		}
		sprintf(text_buf, "%04x:%04x, %s %u %s", m_mapping.device_type_id >> 16, m_mapping.device_type_id & 0xFFFF, type_string, m_mapping.id, hat_string);
		m_display_box->setText(QString(text_buf));

		m_reverse_checkbox->setChecked(m_mapping.reverse);

		if (m_button_status)
			m_button_status->setChecked(m_mapping.reverse);

		if (m_axis_status)
		{
			int32_t axis_value = (-0x8000);
			if (m_mapping.reverse)
				axis_value = axis_value * (-1);
			if (m_flip_axis_display)
				axis_value = axis_value * (-1);
			if (axis_value > 0x7FFF)
				axis_value = 0x7FFF;
			if (axis_value < (-0x8000))
				axis_value = (-0x8000);
			m_axis_status->setValue(axis_value);
		}

		const std::map<uint32_t, joystick_state>& joystick_states = m_setting_dialog->get_joystick_states();
		auto joystick_state = joystick_states.find(m_mapping.device_type_id);

		if (joystick_state != joystick_states.end())
		{
			switch (m_mapping.type)
			{
			case MAPPING_BUTTON:
			{
				if (joystick_state->second.buttons.size() <= m_mapping.id)
					break;
				bool value = joystick_state->second.buttons[m_mapping.id];
				if (m_mapping.reverse)
					value = !value;
				m_button_status->setChecked(value);
				break;
			}
			case MAPPING_HAT:
			{
				if (joystick_state->second.hats.size() <= m_mapping.id)
					break;
				bool value = joystick_state->second.hats[m_mapping.id] == m_mapping.hat;
				if (m_mapping.reverse)
					value = !value;
				m_button_status->setChecked(value);
				break;
			}
			case MAPPING_AXIS:
			{
				if (joystick_state->second.axes.size() <= m_mapping.id)
					break;
				int32_t value = joystick_state->second.axes[m_mapping.id];
				if (m_mapping.reverse)
					value = value * (-1);
				if (m_flip_axis_display)
					value = value * (-1);
				if (value > 0x7FFF)
					value = 0x7FFF;
				else if (value < (-0x8000))
					value = (-0x8000);
				m_axis_status->setValue(value);
				break;
			}
			}
		}
	}
};

void emulated_logitech_g27_settings_dialog::save_ui_state_to_config()
{
#define SAVE_MAPPING(name)                                            \
	{                                                                 \
		const sdl_mapping& m = m_##name->get_mapping();               \
		g_cfg_logitech_g27.name.device_type_id.set(m.device_type_id); \
		g_cfg_logitech_g27.name.type.set(m.type);                     \
		g_cfg_logitech_g27.name.id.set(m.id);                         \
		g_cfg_logitech_g27.name.hat.set(m.hat);                       \
		g_cfg_logitech_g27.name.reverse.set(m.reverse);               \
	}

	SAVE_MAPPING(steering);
	SAVE_MAPPING(throttle);
	SAVE_MAPPING(brake);
	SAVE_MAPPING(clutch);
	SAVE_MAPPING(shift_up);
	SAVE_MAPPING(shift_down);

	SAVE_MAPPING(up);
	SAVE_MAPPING(down);
	SAVE_MAPPING(left);
	SAVE_MAPPING(right);

	SAVE_MAPPING(triangle);
	SAVE_MAPPING(cross);
	SAVE_MAPPING(square);
	SAVE_MAPPING(circle);

	SAVE_MAPPING(l2);
	SAVE_MAPPING(l3);
	SAVE_MAPPING(r2);
	SAVE_MAPPING(r3);

	SAVE_MAPPING(plus);
	SAVE_MAPPING(minus);

	SAVE_MAPPING(dial_clockwise);
	SAVE_MAPPING(dial_anticlockwise);

	SAVE_MAPPING(select);
	SAVE_MAPPING(pause);

	SAVE_MAPPING(shifter_1);
	SAVE_MAPPING(shifter_2);
	SAVE_MAPPING(shifter_3);
	SAVE_MAPPING(shifter_4);
	SAVE_MAPPING(shifter_5);
	SAVE_MAPPING(shifter_6);
	SAVE_MAPPING(shifter_r);

#undef SAVE_MAPPING

	g_cfg_logitech_g27.ffb_device_type_id.set(m_ffb_device->get_device_type_id());
	g_cfg_logitech_g27.led_device_type_id.set(m_led_device->get_device_type_id());

	g_cfg_logitech_g27.enabled.set(m_enabled->isChecked());
	g_cfg_logitech_g27.reverse_effects.set(m_reverse_effects->isChecked());
}

void emulated_logitech_g27_settings_dialog::load_ui_state_from_config()
{
#define LOAD_MAPPING(name)                                                                         \
	{                                                                                              \
		sdl_mapping m = {                                                                          \
			.device_type_id = static_cast<uint32_t>(g_cfg_logitech_g27.name.device_type_id.get()), \
			.type = static_cast<sdl_mapping_type>(g_cfg_logitech_g27.name.type.get()),             \
			.id = static_cast<uint8_t>(g_cfg_logitech_g27.name.id.get()),                          \
			.hat = static_cast<hat_component>(g_cfg_logitech_g27.name.hat.get()),                  \
			.reverse = g_cfg_logitech_g27.name.reverse.get(),                                      \
			.positive_axis = false};                                                               \
		m_##name->set_mapping(m);                                                                  \
	}

	LOAD_MAPPING(steering);
	LOAD_MAPPING(throttle);
	LOAD_MAPPING(brake);
	LOAD_MAPPING(clutch);
	LOAD_MAPPING(shift_up);
	LOAD_MAPPING(shift_down);

	LOAD_MAPPING(up);
	LOAD_MAPPING(down);
	LOAD_MAPPING(left);
	LOAD_MAPPING(right);

	LOAD_MAPPING(triangle);
	LOAD_MAPPING(cross);
	LOAD_MAPPING(square);
	LOAD_MAPPING(circle);

	LOAD_MAPPING(l2);
	LOAD_MAPPING(l3);
	LOAD_MAPPING(r2);
	LOAD_MAPPING(r3);

	LOAD_MAPPING(plus);
	LOAD_MAPPING(minus);

	LOAD_MAPPING(dial_clockwise);
	LOAD_MAPPING(dial_anticlockwise);

	LOAD_MAPPING(select);
	LOAD_MAPPING(pause);

	LOAD_MAPPING(shifter_1);
	LOAD_MAPPING(shifter_2);
	LOAD_MAPPING(shifter_3);
	LOAD_MAPPING(shifter_4);
	LOAD_MAPPING(shifter_5);
	LOAD_MAPPING(shifter_6);
	LOAD_MAPPING(shifter_r);

#undef LOAD_MAPPING

	m_ffb_device->set_device_type_id(static_cast<uint32_t>(g_cfg_logitech_g27.ffb_device_type_id.get()));
	m_led_device->set_device_type_id(static_cast<uint32_t>(g_cfg_logitech_g27.led_device_type_id.get()));

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
				g_cfg_logitech_g27.from_default();
				load_ui_state_from_config();
				g_cfg_logitech_g27.save();
			}
			else if (button == buttons->button(QDialogButtonBox::Cancel))
			{
				reject();
			}
		});

	QLabel* warning = new QLabel(QString("Warning: Force feedback output were meant for Logitech G27, on stronger wheels please adjust force strength accordingly in your wheel software."), this);
	warning->setStyleSheet("color: red;");
	v_layout->addWidget(warning);

	m_enabled = new QCheckBox(QString("Enabled (requires game restart)"), this);
	m_enabled->setChecked(g_cfg_logitech_g27.enabled.get());
	v_layout->addWidget(m_enabled);

	m_reverse_effects = new QCheckBox(QString("Reverse force feedback effects"), this);
	m_reverse_effects->setChecked(g_cfg_logitech_g27.reverse_effects.get());
	v_layout->addWidget(m_reverse_effects);

	m_state_text = new QLabel(QString(DEFAULT_STATUS), this);
	v_layout->addWidget(m_state_text);

	m_ffb_device = new DeviceChoice(this, g_cfg_logitech_g27.ffb_device_type_id.get(), "Force Feedback Device");
	m_led_device = new DeviceChoice(this, g_cfg_logitech_g27.led_device_type_id.get(), "LED Device");

	m_mapping_scroll_area = new QScrollArea(this);
	QWidget* mapping_widget = new QWidget(m_mapping_scroll_area);
	QVBoxLayout* mapping_layout = new QVBoxLayout(mapping_widget);
	mapping_widget->setLayout(mapping_layout);
	m_mapping_scroll_area->setWidget(mapping_widget);
	m_mapping_scroll_area->setWidgetResizable(true);
	m_mapping_scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_mapping_scroll_area->setMinimumHeight(400);
	m_mapping_scroll_area->setMinimumWidth(800);

	v_layout->addWidget(m_mapping_scroll_area);

#define ADD_MAPPING_SETTING(name, is_axis, display_name, flip_axis_display)                                                    \
	{                                                                                                                          \
		sdl_mapping m = {                                                                                                      \
			.device_type_id = static_cast<uint32_t>(g_cfg_logitech_g27.name.device_type_id.get()),                             \
			.type = static_cast<sdl_mapping_type>(g_cfg_logitech_g27.name.type.get()),                                         \
			.id = static_cast<uint8_t>(g_cfg_logitech_g27.name.id.get()),                                                      \
			.hat = static_cast<hat_component>(g_cfg_logitech_g27.name.hat.get()),                                              \
			.reverse = g_cfg_logitech_g27.name.reverse.get(),                                                                  \
			.positive_axis = false};                                                                                           \
		m_##name = new Mapping(mapping_widget, this, m_ffb_device, m_led_device, m, is_axis, display_name, flip_axis_display); \
		mapping_layout->addWidget(m_##name);                                                                                   \
	}

	QLabel* axis_label = new QLabel(QString("Axes:"), mapping_widget);
	mapping_layout->addWidget(axis_label);

	ADD_MAPPING_SETTING(steering, true, "Steering", false);
	ADD_MAPPING_SETTING(throttle, true, "Throttle", true);
	ADD_MAPPING_SETTING(brake, true, "Brake", true);
	ADD_MAPPING_SETTING(clutch, true, "Clutch", true);

	QLabel* button_label = new QLabel(QString("Buttons:"), mapping_widget);
	mapping_layout->addWidget(button_label);

	ADD_MAPPING_SETTING(shift_up, false, "Shift up", false);
	ADD_MAPPING_SETTING(shift_down, false, "Shift down", false);

	ADD_MAPPING_SETTING(up, false, "Up", false);
	ADD_MAPPING_SETTING(down, false, "Down", false);
	ADD_MAPPING_SETTING(left, false, "Left", false);
	ADD_MAPPING_SETTING(right, false, "Right", false);

	ADD_MAPPING_SETTING(triangle, false, "Triangle", false);
	ADD_MAPPING_SETTING(cross, false, "Cross", false);
	ADD_MAPPING_SETTING(square, false, "Square", false);
	ADD_MAPPING_SETTING(circle, false, "Circle", false);

	ADD_MAPPING_SETTING(l2, false, "L2", false);
	ADD_MAPPING_SETTING(l3, false, "L3", false);
	ADD_MAPPING_SETTING(r2, false, "R2", false);
	ADD_MAPPING_SETTING(r3, false, "R3", false);

	ADD_MAPPING_SETTING(plus, false, "L4", false);
	ADD_MAPPING_SETTING(minus, false, "L5", false);

	ADD_MAPPING_SETTING(dial_clockwise, false, "R4", false);
	ADD_MAPPING_SETTING(dial_anticlockwise, false, "R5", false);

	ADD_MAPPING_SETTING(select, false, "Select", false);
	ADD_MAPPING_SETTING(pause, false, "Start", false);

	ADD_MAPPING_SETTING(shifter_1, false, "Gear 1", false);
	ADD_MAPPING_SETTING(shifter_2, false, "Gear 2", false);
	ADD_MAPPING_SETTING(shifter_3, false, "Gear 3", false);
	ADD_MAPPING_SETTING(shifter_4, false, "Gear 4", false);
	ADD_MAPPING_SETTING(shifter_5, false, "Gear 5", false);
	ADD_MAPPING_SETTING(shifter_6, false, "Gear 6", false);
	ADD_MAPPING_SETTING(shifter_r, false, "Gear R", false);

#undef ADD_MAPPING_SETTING

	v_layout->addWidget(m_ffb_device);
	v_layout->addWidget(m_led_device);

	v_layout->addWidget(buttons);
	setLayout(v_layout);

	m_sdl_initialized = sdl_instance::get_instance().initialize();

	if (m_sdl_initialized)
		get_joystick_states();
}

emulated_logitech_g27_settings_dialog::~emulated_logitech_g27_settings_dialog()
{
	for (auto joystick_handle = m_joystick_handles.begin(); joystick_handle != m_joystick_handles.end(); joystick_handle++)
	{
		SDL_CloseJoystick(*joystick_handle);
	}
}

static inline hat_component get_sdl_hat_component(uint8_t sdl_hat)
{
	if (sdl_hat & SDL_HAT_UP)
	{
		return HAT_UP;
	}

	if (sdl_hat & SDL_HAT_DOWN)
	{
		return HAT_DOWN;
	}

	if (sdl_hat & SDL_HAT_LEFT)
	{
		return HAT_LEFT;
	}

	if (sdl_hat & SDL_HAT_RIGHT)
	{
		return HAT_RIGHT;
	}

	return HAT_NONE;
}

const std::map<uint32_t, joystick_state>& emulated_logitech_g27_settings_dialog::get_joystick_states()
{
	if (!m_sdl_initialized)
	{
		return m_last_joystick_states;
	}

	uint64_t now = SDL_GetTicks();

	if (SDL_GetTicks() - m_last_joystick_states_update < 25)
	{
		return m_last_joystick_states;
	}

	m_last_joystick_states_update = now;

	std::map<uint32_t, joystick_state> new_joystick_states;

	sdl_instance::get_instance().pump_events();

	int joystick_count;
	SDL_JoystickID* joystick_ids = SDL_GetJoysticks(&joystick_count);

	std::vector<SDL_Joystick*> new_joystick_handles;

	if (joystick_ids != nullptr)
	{
		for (int i = 0; i < joystick_count; i++)
		{
			SDL_Joystick* cur_joystick = SDL_OpenJoystick(joystick_ids[i]);
			if (cur_joystick == nullptr)
			{
				continue;
			}
			new_joystick_handles.push_back(cur_joystick);

			uint32_t device_type_id = (SDL_GetJoystickVendor(cur_joystick) << 16) | SDL_GetJoystickProduct(cur_joystick);

			auto cur_state = new_joystick_states.find(device_type_id);
			if (cur_state == new_joystick_states.end())
			{
				joystick_state s;
				int num_axes = SDL_GetNumJoystickAxes(cur_joystick);
				int num_buttons = SDL_GetNumJoystickButtons(cur_joystick);
				int num_hats = SDL_GetNumJoystickHats(cur_joystick);
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
					uint8_t sdl_hat = SDL_GetJoystickHat(cur_joystick, j);
					s.hats.push_back(get_sdl_hat_component(sdl_hat));
				}
				new_joystick_states[device_type_id] = s;
			}
			else
			{
				for (std::vector<int16_t>::size_type j = 0; j < cur_state->second.axes.size(); j++)
				{
					cur_state->second.axes[j] = (cur_state->second.axes[j] + SDL_GetJoystickAxis(cur_joystick, j)) / 2;
				}
				for (std::vector<bool>::size_type j = 0; j < cur_state->second.buttons.size(); j++)
				{
					cur_state->second.buttons[j] = cur_state->second.buttons[j] || SDL_GetJoystickButton(cur_joystick, j);
				}
				for (std::vector<hat_component>::size_type j = 0; j < cur_state->second.hats.size(); j++)
				{
					if (cur_state->second.hats[j] != HAT_NONE)
						continue;
					uint8_t sdl_hat = SDL_GetJoystickHat(cur_joystick, j);
					cur_state->second.hats[j] = get_sdl_hat_component(sdl_hat);
				}
			}
		}
	}

	for (auto joystick_handle = m_joystick_handles.begin(); joystick_handle != m_joystick_handles.end(); joystick_handle++)
	{
		SDL_CloseJoystick(*joystick_handle);
	}

	m_joystick_handles = new_joystick_handles;
	m_last_joystick_states = new_joystick_states;

	return m_last_joystick_states;
}

void emulated_logitech_g27_settings_dialog::set_state_text(const char* text)
{
	m_state_text->setText(QString(text));
}

void emulated_logitech_g27_settings_dialog::toggle_state(bool enable)
{

	int slider_position = m_mapping_scroll_area->verticalScrollBar()->sliderPosition();

#define TOGGLE_STATE(name)       \
	{                            \
		if (enable)              \
			m_##name->enable();  \
		else                     \
			m_##name->disable(); \
	}
	TOGGLE_STATE(steering);
	TOGGLE_STATE(throttle);
	TOGGLE_STATE(brake);
	TOGGLE_STATE(clutch);
	TOGGLE_STATE(shift_up);
	TOGGLE_STATE(shift_down);

	TOGGLE_STATE(up);
	TOGGLE_STATE(down);
	TOGGLE_STATE(left);
	TOGGLE_STATE(right);

	TOGGLE_STATE(triangle);
	TOGGLE_STATE(cross);
	TOGGLE_STATE(square);
	TOGGLE_STATE(circle);

	TOGGLE_STATE(l2);
	TOGGLE_STATE(l3);
	TOGGLE_STATE(r2);
	TOGGLE_STATE(r3);

	TOGGLE_STATE(plus);
	TOGGLE_STATE(minus);

	TOGGLE_STATE(dial_clockwise);
	TOGGLE_STATE(dial_anticlockwise);

	TOGGLE_STATE(select);
	TOGGLE_STATE(pause);

	TOGGLE_STATE(shifter_1);
	TOGGLE_STATE(shifter_2);
	TOGGLE_STATE(shifter_3);
	TOGGLE_STATE(shifter_4);
	TOGGLE_STATE(shifter_5);
	TOGGLE_STATE(shifter_6);
	TOGGLE_STATE(shifter_r);

#undef TOGGLE_STATE

	m_enabled->setEnabled(enable);
	m_reverse_effects->setEnabled(enable);

	m_mapping_scroll_area->verticalScrollBar()->setEnabled(enable);
	m_mapping_scroll_area->verticalScrollBar()->setSliderPosition(slider_position);
}

void emulated_logitech_g27_settings_dialog::enable()
{
	toggle_state(true);
}

void emulated_logitech_g27_settings_dialog::disable()
{
	toggle_state(false);
}

#endif
