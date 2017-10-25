#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QAction>

#include "pad_settings_dialog.h"
#include "ui_pad_settings_dialog.h"

inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }
constexpr auto qstr = QString::fromStdString;

pad_settings_dialog::pad_settings_dialog(pad_config* pad_cfg, const std::string& device, PadHandlerBase& handler, QWidget *parent)
	: QDialog(parent), ui(new Ui::pad_settings_dialog), m_handler_cfg(pad_cfg), m_device_name(device), m_handler(&handler)
{
	m_handler_cfg->load();

	ui->setupUi(this);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->b_cancel->setDefault(true);
	connect(ui->b_cancel, &QAbstractButton::clicked, this, &QWidget::close);

	m_padButtons = new QButtonGroup(this);
	m_palette = ui->b_left->palette(); // save normal palette

	ui->chb_vibration_large->setChecked((bool)m_handler_cfg->enable_vibration_motor_large);
	ui->chb_vibration_small->setChecked((bool)m_handler_cfg->enable_vibration_motor_small);
	ui->chb_vibration_switch->setChecked((bool)m_handler_cfg->switch_vibration_motors);

	// Adjust to the different pad handlers
	if (m_handler_cfg->cfg_type == "keyboard")
	{
		setWindowTitle(tr("Configure Keyboard"));
		m_handler_type = HANDLER_TYPE_KEYBOARD;
	}
	else if (m_handler_cfg->cfg_type == "xinput")
	{
		setWindowTitle(tr("Configure XInput"));
		m_handler_type = HANDLER_TYPE_XINPUT;
	}
	else if (m_handler_cfg->cfg_type == "ds4")
	{
		setWindowTitle(tr("Configure DS4"));
		m_handler_type = HANDLER_TYPE_DS4;
	}
	else if (m_handler_cfg->cfg_type == "mmjoystick")
	{
		setWindowTitle(tr("Configure MMJoystick"));
		m_handler_type = HANDLER_TYPE_MMJOYSTICK;
	}
	else if (m_handler_cfg->cfg_type == "evdev")
	{
		setWindowTitle(tr("Configure evdev"));
		m_handler_type = HANDLER_TYPE_EVDEV;
	}

	// Enable Button Remapping
	if (m_handler->has_config())
	{
		// Use timer to get button input
		const auto& callback = [=](u32 button, std::string name)
		{
			LOG_NOTICE(GENERAL, "Pad Settings: %s button %s pressed (0x%x)", m_handler_cfg->cfg_type, name, button);
			m_cfg_entries[m_button_id].key = std::to_string(button);
			m_cfg_entries[m_button_id].text = qstr(name);
			ReactivateButtons();
		};

		connect(&m_timer_input, &QTimer::timeout, [=]()
		{
			if (m_button_id > id_pad_begin && m_button_id < id_pad_end)
			{
				m_handler->GetNextButtonPress(m_device_name, callback);
			}
		});

		m_timer_input.start(10);
	};

	// Enable Vibration Checkboxes
	if (m_handler->has_rumble())
	{
		const s32 min_force = m_handler->VIBRATION_MIN;
		const s32 max_force = m_handler->VIBRATION_MAX;

		ui->chb_vibration_large->setEnabled(true);
		ui->chb_vibration_small->setEnabled(true);
		ui->chb_vibration_switch->setEnabled(true);

		connect(ui->chb_vibration_large, &QCheckBox::clicked, [=](bool checked)
		{
			if (!checked) return;

			ui->chb_vibration_switch->isChecked() ? m_handler->TestVibration(m_device_name, min_force, max_force)
				: m_handler->TestVibration(m_device_name, max_force, min_force);

			QTimer::singleShot(300, [=]()
			{
				m_handler->TestVibration(m_device_name, min_force, min_force);
			});
		});

		connect(ui->chb_vibration_small, &QCheckBox::clicked, [=](bool checked)
		{
			if (!checked) return;

			ui->chb_vibration_switch->isChecked() ? m_handler->TestVibration(m_device_name, max_force, min_force)
				: m_handler->TestVibration(m_device_name, min_force, max_force);

			QTimer::singleShot(300, [=]()
			{
				m_handler->TestVibration(m_device_name, min_force, min_force);
			});
		});

		connect(ui->chb_vibration_switch, &QCheckBox::clicked, [=](bool checked)
		{
			checked ? m_handler->TestVibration(m_device_name, min_force, max_force)
				: m_handler->TestVibration(m_device_name, max_force, min_force);

			QTimer::singleShot(200, [=]()
			{
				checked ? m_handler->TestVibration(m_device_name, max_force, min_force)
					: m_handler->TestVibration(m_device_name, min_force, max_force);

				QTimer::singleShot(200, [=]()
				{
					m_handler->TestVibration(m_device_name, min_force, min_force);
				});
			});
		});
	}

	// Enable Deadzone Settings
	if (m_handler->has_deadzones())
	{
		auto initSlider = [=](QSlider* slider, const s32& value, const s32& min, const s32& max)
		{
			slider->setEnabled(true);
			slider->setRange(min, max);
			slider->setValue(value);
		};

		// Enable Trigger Thresholds
		initSlider(ui->slider_trigger_left, m_handler_cfg->ltriggerthreshold, m_handler->TRIGGER_MIN, m_handler->TRIGGER_MAX);
		initSlider(ui->slider_trigger_right, m_handler_cfg->rtriggerthreshold, m_handler->TRIGGER_MIN, m_handler->TRIGGER_MAX);

		// Enable Stick Deadzones
		initSlider(ui->slider_stick_left, m_handler_cfg->lstickdeadzone, m_handler->THUMB_MIN, m_handler->THUMB_MAX);
		initSlider(ui->slider_stick_right, m_handler_cfg->rstickdeadzone, m_handler->THUMB_MIN, m_handler->THUMB_MAX);
	}

	auto insertButton = [this](int id, QPushButton* button, cfg::int32* cfg_id)
	{
		QString name = GetButtonName(*cfg_id);
		m_cfg_entries.insert(std::make_pair(id, PAD_BUTTON{ cfg_id, std::to_string(*cfg_id), name }));
		m_padButtons->addButton(button, id);
		button->setText(name);
	};

	insertButton(id_pad_lstick_left,  ui->b_lstick_left,  &m_handler_cfg->left_stick_left);  
	insertButton(id_pad_lstick_down,  ui->b_lstick_down,  &m_handler_cfg->left_stick_down);
	insertButton(id_pad_lstick_right, ui->b_lstick_right, &m_handler_cfg->left_stick_right);
	insertButton(id_pad_lstick_up,    ui->b_lstick_up,    &m_handler_cfg->left_stick_up);

	insertButton(id_pad_left,  ui->b_left,  &m_handler_cfg->left);
	insertButton(id_pad_down,  ui->b_down,  &m_handler_cfg->down);
	insertButton(id_pad_right, ui->b_right, &m_handler_cfg->right);
	insertButton(id_pad_up,    ui->b_up,    &m_handler_cfg->up);

	insertButton(id_pad_l1, ui->b_shift_l1, &m_handler_cfg->l1);
	insertButton(id_pad_l2, ui->b_shift_l2, &m_handler_cfg->l2);
	insertButton(id_pad_l3, ui->b_shift_l3, &m_handler_cfg->l3);

	insertButton(id_pad_start,  ui->b_start,  &m_handler_cfg->start);
	insertButton(id_pad_select, ui->b_select, &m_handler_cfg->select);

	insertButton(id_pad_r1, ui->b_shift_r1, &m_handler_cfg->r1);
	insertButton(id_pad_r2, ui->b_shift_r2, &m_handler_cfg->r2);
	insertButton(id_pad_r3, ui->b_shift_r3, &m_handler_cfg->r3);

	insertButton(id_pad_square,   ui->b_square,   &m_handler_cfg->square);
	insertButton(id_pad_cross,    ui->b_cross,    &m_handler_cfg->cross);
	insertButton(id_pad_circle,   ui->b_circle,   &m_handler_cfg->circle);
	insertButton(id_pad_triangle, ui->b_triangle, &m_handler_cfg->triangle);

	insertButton(id_pad_rstick_left,  ui->b_rstick_left,  &m_handler_cfg->right_stick_left);
	insertButton(id_pad_rstick_down,  ui->b_rstick_down,  &m_handler_cfg->right_stick_down);
	insertButton(id_pad_rstick_right, ui->b_rstick_right, &m_handler_cfg->right_stick_right);
	insertButton(id_pad_rstick_up,    ui->b_rstick_up,    &m_handler_cfg->right_stick_up);

	m_padButtons->addButton(ui->b_reset, id_reset_parameters);
	m_padButtons->addButton(ui->b_ok, id_ok);
	m_padButtons->addButton(ui->b_cancel, id_cancel);

	connect(m_padButtons, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &pad_settings_dialog::OnPadButtonClicked);

	connect(&m_timer, &QTimer::timeout, [&]()
	{
		if (--m_seconds <= 0)
		{
			ReactivateButtons();
			return;
		}
		m_padButtons->button(m_button_id)->setText(tr("[ Waiting %1 ]").arg(m_seconds));
	});

	UpdateLabel();

	gui_settings settings(this);

	// repaint and resize controller image
	ui->l_controller->setPixmap(settings.colorizedPixmap(*ui->l_controller->pixmap(), QColor(), gui::get_Label_Color("l_controller"), false, true));
	ui->l_controller->setMaximumSize(ui->gb_description->sizeHint().width(), ui->l_controller->maximumHeight() * ui->gb_description->sizeHint().width() / ui->l_controller->maximumWidth());

	layout()->setSizeConstraint(QLayout::SetFixedSize);
}

pad_settings_dialog::~pad_settings_dialog()
{
	delete ui;
}

void pad_settings_dialog::ReactivateButtons()
{
	m_timer.stop();
	m_seconds = MAX_SECONDS;

	if (m_padButtons->button(m_button_id))
	{
		m_padButtons->button(m_button_id)->setPalette(m_palette);
	}

	m_button_id = id_pad_begin;
	UpdateLabel();
	SwitchButtons(true);
}

QString pad_settings_dialog::GetButtonName(const u32& code)
{
	switch (m_handler_type)
	{
	case HANDLER_TYPE_KEYBOARD:
		return GetKeyName(code);
	case HANDLER_TYPE_DS4:
	case HANDLER_TYPE_XINPUT:
	case HANDLER_TYPE_EVDEV:
		return qstr(m_handler->GetButtonName(code));
	case HANDLER_TYPE_MMJOYSTICK:
	default:
		LOG_ERROR(HLE, "Pad Settings: Handler Type: %d, Unknown button code: %d", static_cast<int>(m_handler_type), code);
		return "FAIL";
	}
}

void pad_settings_dialog::keyPressEvent(QKeyEvent *keyEvent)
{
	if (m_handler_type != HANDLER_TYPE_KEYBOARD)
	{
		return;
	}

	if (m_button_id == id_pad_begin)
	{
		return;
	}

	if (m_button_id <= id_pad_begin || m_button_id >= id_pad_end)
	{
		LOG_ERROR(HLE, "Pad Settings: Handler Type: %d, Unknown button ID: %d", static_cast<int>(m_handler_type), m_button_id);
	}
	else
	{
		m_cfg_entries[m_button_id].key = std::to_string(keyEvent->key());
		m_cfg_entries[m_button_id].text = GetKeyName(keyEvent->key());
	}

	ReactivateButtons();
}

void pad_settings_dialog::UpdateLabel(bool is_reset)
{
	if (is_reset)
	{
		ui->chb_vibration_large->setChecked((bool)m_handler_cfg->enable_vibration_motor_large);
		ui->chb_vibration_small->setChecked((bool)m_handler_cfg->enable_vibration_motor_small);
		ui->chb_vibration_switch->setChecked((bool)m_handler_cfg->switch_vibration_motors);

		ui->slider_trigger_left->setValue(m_handler_cfg->ltriggerthreshold);
		ui->slider_trigger_right->setValue(m_handler_cfg->rtriggerthreshold);
		ui->slider_stick_left->setValue(m_handler_cfg->lstickdeadzone);
		ui->slider_stick_right->setValue(m_handler_cfg->rstickdeadzone);
	}

	for (auto& entry : m_cfg_entries)
	{
		if (is_reset)
		{
			entry.second.text = GetButtonName(*entry.second.cfg_id);
			entry.second.key = std::to_string(*entry.second.cfg_id);
		}

		m_padButtons->button(entry.first)->setText(entry.second.text);
	}
}

void pad_settings_dialog::SwitchButtons(bool is_enabled)
{
	for (const auto& button : m_padButtons->buttons())
	{
		button->setEnabled(is_enabled);
	}
}

void pad_settings_dialog::SaveConfig()
{
	for (const auto& entry : m_cfg_entries)
	{
		entry.second.cfg_id->from_string(entry.second.key);
	}
	m_handler_cfg->enable_vibration_motor_large.set(ui->chb_vibration_large->isChecked());
	m_handler_cfg->enable_vibration_motor_small.set(ui->chb_vibration_small->isChecked());
	m_handler_cfg->switch_vibration_motors.set(ui->chb_vibration_switch->isChecked());
	m_handler_cfg->ltriggerthreshold.set(ui->slider_trigger_left->value());
	m_handler_cfg->rtriggerthreshold.set(ui->slider_trigger_right->value());
	m_handler_cfg->lstickdeadzone.set(ui->slider_stick_left->value());
	m_handler_cfg->rstickdeadzone.set(ui->slider_stick_right->value());
	m_handler_cfg->save();
}

const QString pad_settings_dialog::GetKeyName(const u32& keyCode)
{
	//TODO what about numpad?
	return QKeySequence(keyCode).toString();
}

void pad_settings_dialog::OnPadButtonClicked(int id)
{
	switch (id)
	{
	case id_pad_begin:
	case id_pad_end:
	case id_cancel:
		return;
	case id_reset_parameters:
		m_handler_cfg->from_default();
		UpdateLabel(true);
		return;
	case id_ok:
		SaveConfig();
		QDialog::accept();
		return;
	default:
		break;
	}

	m_button_id = id;
	m_padButtons->button(m_button_id)->setText(tr("[ Waiting %1 ]").arg(MAX_SECONDS));
	m_padButtons->button(m_button_id)->setPalette(QPalette(Qt::blue));
	SwitchButtons(false); // disable all buttons, needed for using Space, Enter and other specific buttons
	m_timer.start(1000);
}
