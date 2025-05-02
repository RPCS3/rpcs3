#include "stdafx.h"

#ifdef HAVE_SDL3

#include "emulated_logitech_g27_settings_dialog.h"
#include "Emu/Io/LogitechG27.h"
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
#include <QComboBox>

LOG_CHANNEL(logitech_g27_cfg_log, "LOGIG27");

static constexpr const char* DEFAULT_STATUS = " ";

enum mapping_device_choice
{
	CHOICE_NONE = -1,
	CHOICE_STEERING = 0,
	CHOICE_THROTTLE,
	CHOICE_BRAKE,
	CHOICE_CLUTCH,
	CHOICE_SHIFT_UP,
	CHOICE_SHIFT_DOWN,

	CHOICE_UP,
	CHOICE_DOWN,
	CHOICE_LEFT,
	CHOICE_RIGHT,

	CHOICE_TRIANGLE,
	CHOICE_CROSS,
	CHOICE_SQUARE,
	CHOICE_CIRCLE,

	CHOICE_L2,
	CHOICE_L3,
	CHOICE_R2,
	CHOICE_R3,

	CHOICE_PLUS,
	CHOICE_MINUS,

	CHOICE_DIAL_CLOCKWISE,
	CHOICE_DIAL_ANTICLOCKWISE,

	CHOICE_SELECT,
	CHOICE_PAUSE,

	CHOICE_SHIFTER_1,
	CHOICE_SHIFTER_2,
	CHOICE_SHIFTER_3,
	CHOICE_SHIFTER_4,
	CHOICE_SHIFTER_5,
	CHOICE_SHIFTER_6,
	CHOICE_SHIFTER_R
};

class DeviceChoice : public QWidget
{
public:
	DeviceChoice(QWidget* parent, const char* name)
		: QWidget(parent)
	{
		auto layout = new QHBoxLayout(this);
		setLayout(layout);

		QLabel* label = new QLabel(this);
		label->setText(QString(name));
		label->setMinimumWidth(400);
		layout->addWidget(label);

		m_dropdown = new QComboBox(this);
		layout->addWidget(m_dropdown);

		m_disable_button = new QPushButton(QString("DISABLE"), this);
		layout->addWidget(m_disable_button);

		connect(m_dropdown, &QComboBox::currentIndexChanged, this, [this]()
			{
				m_device_choice = static_cast<mapping_device_choice>(this->m_dropdown->currentIndex());
				update_display();
			});

		connect(m_disable_button, &QPushButton::clicked, this, [this]()
			{
				m_device_choice = CHOICE_NONE;
				update_display();
			});

		m_dropdown->setPlaceholderText(QString("-- Disabled --"));

		m_dropdown->addItem(QString("Steering"), QVariant(CHOICE_STEERING));
		m_dropdown->addItem(QString("Throttle"), QVariant(CHOICE_THROTTLE));
		m_dropdown->addItem(QString("Brake"), QVariant(CHOICE_BRAKE));
		m_dropdown->addItem(QString("Clutch"), QVariant(CHOICE_CLUTCH));
		m_dropdown->addItem(QString("Shift up"), QVariant(CHOICE_SHIFT_UP));
		m_dropdown->addItem(QString("Shift down"), QVariant(CHOICE_SHIFT_DOWN));

		m_dropdown->addItem(QString("Up"), QVariant(CHOICE_UP));
		m_dropdown->addItem(QString("Down"), QVariant(CHOICE_DOWN));
		m_dropdown->addItem(QString("Left"), QVariant(CHOICE_LEFT));
		m_dropdown->addItem(QString("Right"), QVariant(CHOICE_RIGHT));

		m_dropdown->addItem(QString("Triangle"), QVariant(CHOICE_TRIANGLE));
		m_dropdown->addItem(QString("Cross"), QVariant(CHOICE_CROSS));
		m_dropdown->addItem(QString("Square"), QVariant(CHOICE_SQUARE));
		m_dropdown->addItem(QString("Circle"), QVariant(CHOICE_CIRCLE));

		m_dropdown->addItem(QString("L2"), QVariant(CHOICE_L2));
		m_dropdown->addItem(QString("L3"), QVariant(CHOICE_L3));
		m_dropdown->addItem(QString("R2"), QVariant(CHOICE_R2));
		m_dropdown->addItem(QString("R3"), QVariant(CHOICE_R3));

		m_dropdown->addItem(QString("L4"), QVariant(CHOICE_PLUS));
		m_dropdown->addItem(QString("L5"), QVariant(CHOICE_MINUS));

		m_dropdown->addItem(QString("R4"), QVariant(CHOICE_DIAL_CLOCKWISE));
		m_dropdown->addItem(QString("R5"), QVariant(CHOICE_DIAL_ANTICLOCKWISE));

		m_dropdown->addItem(QString("Select"), QVariant(CHOICE_SELECT));
		m_dropdown->addItem(QString("Pause"), QVariant(CHOICE_PAUSE));

		m_dropdown->addItem(QString("Gear 1"), QVariant(CHOICE_SHIFTER_1));
		m_dropdown->addItem(QString("Gear 2"), QVariant(CHOICE_SHIFTER_2));
		m_dropdown->addItem(QString("Gear 3"), QVariant(CHOICE_SHIFTER_3));
		m_dropdown->addItem(QString("Gear 4"), QVariant(CHOICE_SHIFTER_4));
		m_dropdown->addItem(QString("Gear 5"), QVariant(CHOICE_SHIFTER_5));
		m_dropdown->addItem(QString("Gear 6"), QVariant(CHOICE_SHIFTER_6));
		m_dropdown->addItem(QString("Gear r"), QVariant(CHOICE_SHIFTER_R));

		update_display();
	}

	mapping_device_choice get_device_choice()
	{
		return m_device_choice;
	}

	void set_device_choice(mapping_device_choice choice)
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
	mapping_device_choice m_device_choice = CHOICE_NONE;
	QComboBox* m_dropdown = nullptr;
	QPushButton* m_disable_button = nullptr;

	void update_display()
	{
		m_dropdown->setCurrentIndex(static_cast<int>(m_device_choice));
	}
};

class Mapping : public QGroupBox
{
public:
	Mapping(QWidget* parent, emulated_logitech_g27_settings_dialog* dialog, bool is_axis, const char* name, bool flip_axis_display)
		: QGroupBox(parent), m_setting_dialog(dialog), m_is_axis(is_axis), m_name(name), m_flip_axis_display(flip_axis_display)
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
		horizontal_layout->addWidget(m_map_button);
		horizontal_layout->addWidget(m_unmap_button);
		horizontal_layout->addWidget(m_reverse_checkbox);

		if (this->m_is_axis)
			layout->addWidget(m_axis_status);

		connect(m_map_button, &QPushButton::clicked, this, [this]()
			{
				this->m_mapping_in_progress = true;
				this->m_timeout_msec = 5500;
				this->m_setting_dialog->set_enable(false);
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

					for (auto new_joystick_state : new_joystick_states)
					{
						auto last_joystick_state = this->m_last_joystick_states.find(new_joystick_state.first);
						if (last_joystick_state == this->m_last_joystick_states.end())
						{
							continue;
						}

						if (this->m_is_axis)
						{
							const static int16_t axis_change_threshold = 0x7FFF / 5;
							if (last_joystick_state->second.axes.size() != new_joystick_state.second.axes.size())
							{
								logitech_g27_cfg_log.error("during input state change diff, number of axes on %04x:%04x changed", new_joystick_state.first >> 16, new_joystick_state.first & 0xFFFF);
								continue;
							}
							for (std::vector<int16_t>::size_type i = 0; i < new_joystick_state.second.axes.size(); i++)
							{
								int32_t diff = std::abs(last_joystick_state->second.axes[i] - new_joystick_state.second.axes[i]);
								if (diff > axis_change_threshold)
								{
									this->m_mapping_in_progress = false;
									this->m_setting_dialog->set_state_text(DEFAULT_STATUS);
									this->m_setting_dialog->set_enable(true);
									this->m_mapping.device_type_id = new_joystick_state.first;
									this->m_mapping.type = MAPPING_AXIS;
									this->m_mapping.id = i;
									this->m_mapping.hat = HAT_NONE;
									break;
								}
							}
						}
						else
						{
							if (last_joystick_state->second.buttons.size() != new_joystick_state.second.buttons.size())
							{
								logitech_g27_cfg_log.error("during input state change diff, number of buttons on %04x:%04x changed", new_joystick_state.first >> 16, new_joystick_state.first & 0xFFFF);
								continue;
							}
							if (last_joystick_state->second.hats.size() != new_joystick_state.second.hats.size())
							{
								logitech_g27_cfg_log.error("during input state change diff, number of hats on %04x:%04x changed", new_joystick_state.first >> 16, new_joystick_state.first & 0xFFFF);
								continue;
							}
							for (std::vector<int16_t>::size_type i = 0; i < new_joystick_state.second.buttons.size(); i++)
							{
								if (last_joystick_state->second.buttons[i] != new_joystick_state.second.buttons[i])
								{
									this->m_mapping_in_progress = false;
									this->m_setting_dialog->set_state_text(DEFAULT_STATUS);
									this->m_setting_dialog->set_enable(true);
									this->m_mapping.device_type_id = new_joystick_state.first;
									this->m_mapping.type = MAPPING_BUTTON;
									this->m_mapping.id = i;
									this->m_mapping.hat = HAT_NONE;
									break;
								}
							}
							for (std::vector<int16_t>::size_type i = 0; i < new_joystick_state.second.hats.size(); i++)
							{
								if (last_joystick_state->second.hats[i] != new_joystick_state.second.hats[i] && new_joystick_state.second.hats[i] != HAT_NONE)
								{
									this->m_mapping_in_progress = false;
									this->m_setting_dialog->set_state_text(DEFAULT_STATUS);
									this->m_setting_dialog->set_enable(true);
									this->m_mapping.device_type_id = new_joystick_state.first;
									this->m_mapping.type = MAPPING_HAT;
									this->m_mapping.id = i;
									this->m_mapping.hat = new_joystick_state.second.hats[i];
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
						this->m_setting_dialog->set_enable(true);
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
#define SAVE_MAPPING(name, device_choice)                                \
	{                                                                    \
		const sdl_mapping& m = m_##name->get_mapping();                  \
		g_cfg_logitech_g27.name.device_type_id.set(m.device_type_id);    \
		g_cfg_logitech_g27.name.type.set(m.type);                        \
		g_cfg_logitech_g27.name.id.set(m.id);                            \
		g_cfg_logitech_g27.name.hat.set(m.hat);                          \
		g_cfg_logitech_g27.name.reverse.set(m.reverse);                  \
		if (m_ffb_device->get_device_choice() == device_choice)          \
		{                                                                \
			g_cfg_logitech_g27.ffb_device_type_id.set(m.device_type_id); \
		}                                                                \
		if (m_led_device->get_device_choice() == device_choice)          \
		{                                                                \
			g_cfg_logitech_g27.led_device_type_id.set(m.device_type_id); \
		}                                                                \
	}

	SAVE_MAPPING(steering, CHOICE_STEERING);
	SAVE_MAPPING(throttle, CHOICE_THROTTLE);
	SAVE_MAPPING(brake, CHOICE_BRAKE);
	SAVE_MAPPING(clutch, CHOICE_CLUTCH);
	SAVE_MAPPING(shift_up, CHOICE_SHIFT_UP);
	SAVE_MAPPING(shift_down, CHOICE_SHIFT_DOWN);

	SAVE_MAPPING(up, CHOICE_UP);
	SAVE_MAPPING(down, CHOICE_DOWN);
	SAVE_MAPPING(left, CHOICE_LEFT);
	SAVE_MAPPING(right, CHOICE_RIGHT);

	SAVE_MAPPING(triangle, CHOICE_TRIANGLE);
	SAVE_MAPPING(cross, CHOICE_CROSS);
	SAVE_MAPPING(square, CHOICE_SQUARE);
	SAVE_MAPPING(circle, CHOICE_CIRCLE);

	SAVE_MAPPING(l2, CHOICE_L2);
	SAVE_MAPPING(l3, CHOICE_L3);
	SAVE_MAPPING(r2, CHOICE_R2);
	SAVE_MAPPING(r3, CHOICE_R3);

	SAVE_MAPPING(plus, CHOICE_PLUS);
	SAVE_MAPPING(minus, CHOICE_MINUS);

	SAVE_MAPPING(dial_clockwise, CHOICE_DIAL_CLOCKWISE);
	SAVE_MAPPING(dial_anticlockwise, CHOICE_DIAL_ANTICLOCKWISE);

	SAVE_MAPPING(select, CHOICE_SELECT);
	SAVE_MAPPING(pause, CHOICE_PAUSE);

	SAVE_MAPPING(shifter_1, CHOICE_SHIFTER_1);
	SAVE_MAPPING(shifter_2, CHOICE_SHIFTER_2);
	SAVE_MAPPING(shifter_3, CHOICE_SHIFTER_3);
	SAVE_MAPPING(shifter_4, CHOICE_SHIFTER_4);
	SAVE_MAPPING(shifter_5, CHOICE_SHIFTER_5);
	SAVE_MAPPING(shifter_6, CHOICE_SHIFTER_6);
	SAVE_MAPPING(shifter_r, CHOICE_SHIFTER_R);

#undef SAVE_MAPPING

	g_cfg_logitech_g27.enabled.set(m_enabled->isChecked());
	g_cfg_logitech_g27.reverse_effects.set(m_reverse_effects->isChecked());

	if (m_ffb_device->get_device_choice() == CHOICE_NONE)
	{
		g_cfg_logitech_g27.ffb_device_type_id.set(0);
	}

	if (m_led_device->get_device_choice() == CHOICE_NONE)
	{
		g_cfg_logitech_g27.led_device_type_id.set(0);
	}
}

void emulated_logitech_g27_settings_dialog::load_ui_state_from_config()
{
#define LOAD_MAPPING(name, device_choice)                                                                                        \
	{                                                                                                                            \
		const sdl_mapping m = {                                                                                                  \
			.device_type_id = static_cast<uint32_t>(g_cfg_logitech_g27.name.device_type_id.get()),                               \
			.type = static_cast<sdl_mapping_type>(g_cfg_logitech_g27.name.type.get()),                                           \
			.id = static_cast<uint8_t>(g_cfg_logitech_g27.name.id.get()),                                                        \
			.hat = static_cast<hat_component>(g_cfg_logitech_g27.name.hat.get()),                                                \
			.reverse = g_cfg_logitech_g27.name.reverse.get(),                                                                    \
			.positive_axis = false};                                                                                             \
		m_##name->set_mapping(m);                                                                                                \
		if (g_cfg_logitech_g27.ffb_device_type_id.get() == m.device_type_id && m_ffb_device->get_device_choice() == CHOICE_NONE) \
		{                                                                                                                        \
			m_ffb_device->set_device_choice(device_choice);                                                                      \
		}                                                                                                                        \
		if (g_cfg_logitech_g27.led_device_type_id.get() == m.device_type_id && m_led_device->get_device_choice() == CHOICE_NONE) \
		{                                                                                                                        \
			m_led_device->set_device_choice(device_choice);                                                                      \
		}                                                                                                                        \
	}

	LOAD_MAPPING(steering, CHOICE_STEERING);
	LOAD_MAPPING(throttle, CHOICE_THROTTLE);
	LOAD_MAPPING(brake, CHOICE_BRAKE);
	LOAD_MAPPING(clutch, CHOICE_CLUTCH);
	LOAD_MAPPING(shift_up, CHOICE_SHIFT_UP);
	LOAD_MAPPING(shift_down, CHOICE_SHIFT_DOWN);

	LOAD_MAPPING(up, CHOICE_UP);
	LOAD_MAPPING(down, CHOICE_DOWN);
	LOAD_MAPPING(left, CHOICE_LEFT);
	LOAD_MAPPING(right, CHOICE_RIGHT);

	LOAD_MAPPING(triangle, CHOICE_TRIANGLE);
	LOAD_MAPPING(cross, CHOICE_CROSS);
	LOAD_MAPPING(square, CHOICE_SQUARE);
	LOAD_MAPPING(circle, CHOICE_CIRCLE);

	LOAD_MAPPING(l2, CHOICE_L2);
	LOAD_MAPPING(l3, CHOICE_L3);
	LOAD_MAPPING(r2, CHOICE_R2);
	LOAD_MAPPING(r3, CHOICE_R3);

	LOAD_MAPPING(plus, CHOICE_PLUS);
	LOAD_MAPPING(minus, CHOICE_MINUS);

	LOAD_MAPPING(dial_clockwise, CHOICE_DIAL_CLOCKWISE);
	LOAD_MAPPING(dial_anticlockwise, CHOICE_DIAL_ANTICLOCKWISE);

	LOAD_MAPPING(select, CHOICE_SELECT);
	LOAD_MAPPING(pause, CHOICE_PAUSE);

	LOAD_MAPPING(shifter_1, CHOICE_SHIFTER_1);
	LOAD_MAPPING(shifter_2, CHOICE_SHIFTER_2);
	LOAD_MAPPING(shifter_3, CHOICE_SHIFTER_3);
	LOAD_MAPPING(shifter_4, CHOICE_SHIFTER_4);
	LOAD_MAPPING(shifter_5, CHOICE_SHIFTER_5);
	LOAD_MAPPING(shifter_6, CHOICE_SHIFTER_6);
	LOAD_MAPPING(shifter_r, CHOICE_SHIFTER_R);

#undef LOAD_MAPPING

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
				g_cfg_logitech_g27.save();
			}
			else if (button == buttons->button(QDialogButtonBox::Cancel))
			{
				reject();
			}
		});

	QLabel* warning = new QLabel(QString("Warning: Force feedback output were meant for Logitech G27, on stronger wheels please adjust force strength accordingly in your wheel software."), this);
	warning->setStyleSheet("color: red;");
	warning->setWordWrap(true);
	v_layout->addWidget(warning);

	m_enabled = new QCheckBox(QString("Enabled (requires game restart)"), this);
	v_layout->addWidget(m_enabled);

	m_reverse_effects = new QCheckBox(QString("Reverse force feedback effects"), this);
	v_layout->addWidget(m_reverse_effects);

	m_state_text = new QLabel(QString(DEFAULT_STATUS), this);
	v_layout->addWidget(m_state_text);

	m_ffb_device = new DeviceChoice(this, "Use the device with the following mapping for force feedback:");
	m_led_device = new DeviceChoice(this, "Use the device with the following mapping for LED:");

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

	QLabel* axis_label = new QLabel(QString("Axes:"), mapping_widget);
	mapping_layout->addWidget(axis_label);

	auto add_mapping_setting = [mapping_widget, this, mapping_layout](Mapping*& target, bool is_axis, const char* display_name, bool flip_axis_display)
	{
		target = new Mapping(mapping_widget, this, is_axis, display_name, flip_axis_display);
		mapping_layout->addWidget(target);
	};

	add_mapping_setting(m_steering, true, "Steering", false);
	add_mapping_setting(m_throttle, true, "Throttle", true);
	add_mapping_setting(m_brake, true, "Brake", true);
	add_mapping_setting(m_clutch, true, "Clutch", true);

	QLabel* button_label = new QLabel(QString("Buttons:"), mapping_widget);
	mapping_layout->addWidget(button_label);

	add_mapping_setting(m_shift_up, false, "Shift up", false);
	add_mapping_setting(m_shift_down, false, "Shift down", false);

	add_mapping_setting(m_up, false, "Up", false);
	add_mapping_setting(m_down, false, "Down", false);
	add_mapping_setting(m_left, false, "Left", false);
	add_mapping_setting(m_right, false, "Right", false);

	add_mapping_setting(m_triangle, false, "Triangle", false);
	add_mapping_setting(m_cross, false, "Cross", false);
	add_mapping_setting(m_square, false, "Square", false);
	add_mapping_setting(m_circle, false, "Circle", false);

	add_mapping_setting(m_l2, false, "L2", false);
	add_mapping_setting(m_l3, false, "L3", false);
	add_mapping_setting(m_r2, false, "R2", false);
	add_mapping_setting(m_r3, false, "R3", false);

	add_mapping_setting(m_plus, false, "L4", false);
	add_mapping_setting(m_minus, false, "L5", false);

	add_mapping_setting(m_dial_clockwise, false, "R4", false);
	add_mapping_setting(m_dial_anticlockwise, false, "R5", false);

	add_mapping_setting(m_select, false, "Select", false);
	add_mapping_setting(m_pause, false, "Start", false);

	add_mapping_setting(m_shifter_1, false, "Gear 1", false);
	add_mapping_setting(m_shifter_2, false, "Gear 2", false);
	add_mapping_setting(m_shifter_3, false, "Gear 3", false);
	add_mapping_setting(m_shifter_4, false, "Gear 4", false);
	add_mapping_setting(m_shifter_5, false, "Gear 5", false);
	add_mapping_setting(m_shifter_6, false, "Gear 6", false);
	add_mapping_setting(m_shifter_r, false, "Gear R", false);

	v_layout->addSpacing(20);

	v_layout->addWidget(m_ffb_device);
	v_layout->addWidget(m_led_device);

	v_layout->addWidget(buttons);
	setLayout(v_layout);

	load_ui_state_from_config();

	m_sdl_initialized = sdl_instance::get_instance().initialize();

	if (m_sdl_initialized)
		get_joystick_states();
}

emulated_logitech_g27_settings_dialog::~emulated_logitech_g27_settings_dialog()
{
	for (auto joystick_handle : m_joystick_handles)
	{
		if (joystick_handle)
			SDL_CloseJoystick(reinterpret_cast<SDL_Joystick*>(joystick_handle));
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

	std::vector<void*> new_joystick_handles;

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

	for (auto joystick_handle : m_joystick_handles)
	{
		if (joystick_handle)
			SDL_CloseJoystick(reinterpret_cast<SDL_Joystick*>(joystick_handle));
	}

	m_joystick_handles = new_joystick_handles;
	m_last_joystick_states = new_joystick_states;

	return m_last_joystick_states;
}

void emulated_logitech_g27_settings_dialog::set_state_text(const char* text)
{
	m_state_text->setText(QString(text));
}

void emulated_logitech_g27_settings_dialog::set_enable(bool enable)
{
	const int slider_position = m_mapping_scroll_area->verticalScrollBar()->sliderPosition();

	m_steering->set_enable(enable);
	m_throttle->set_enable(enable);
	m_brake->set_enable(enable);
	m_clutch->set_enable(enable);
	m_shift_up->set_enable(enable);
	m_shift_down->set_enable(enable);

	m_up->set_enable(enable);
	m_down->set_enable(enable);
	m_left->set_enable(enable);
	m_right->set_enable(enable);

	m_triangle->set_enable(enable);
	m_cross->set_enable(enable);
	m_square->set_enable(enable);
	m_circle->set_enable(enable);

	m_l2->set_enable(enable);
	m_l3->set_enable(enable);
	m_r2->set_enable(enable);
	m_r3->set_enable(enable);

	m_plus->set_enable(enable);
	m_minus->set_enable(enable);

	m_dial_clockwise->set_enable(enable);
	m_dial_anticlockwise->set_enable(enable);

	m_select->set_enable(enable);
	m_pause->set_enable(enable);

	m_shifter_1->set_enable(enable);
	m_shifter_2->set_enable(enable);
	m_shifter_3->set_enable(enable);
	m_shifter_4->set_enable(enable);
	m_shifter_5->set_enable(enable);
	m_shifter_6->set_enable(enable);
	m_shifter_r->set_enable(enable);

	m_enabled->setEnabled(enable);
	m_reverse_effects->setEnabled(enable);

	m_ffb_device->set_enable(enable);
	m_led_device->set_enable(enable);

	m_mapping_scroll_area->verticalScrollBar()->setEnabled(enable);
	m_mapping_scroll_area->verticalScrollBar()->setSliderPosition(slider_position);
}

#else

// minimal symbols for sdl-less builds automoc
#include "emulated_logitech_g27_settings_dialog.h"

emulated_logitech_g27_settings_dialog::emulated_logitech_g27_settings_dialog(QWidget* parent)
	: QDialog(parent) {}
emulated_logitech_g27_settings_dialog::~emulated_logitech_g27_settings_dialog() {};

#endif
