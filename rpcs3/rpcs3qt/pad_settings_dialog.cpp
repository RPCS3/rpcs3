#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QAction>
#include <QPainter>

#include "pad_settings_dialog.h"
#include "ui_pad_settings_dialog.h"

inline std::string sstr(const QString& _in) { return _in.toStdString(); }
constexpr auto qstr = QString::fromStdString;

pad_settings_dialog::pad_settings_dialog(const std::string& device, const std::string& profile, std::shared_ptr<PadHandlerBase> handler, QWidget *parent)
	: QDialog(parent), ui(new Ui::pad_settings_dialog), m_device_name(device), m_handler(handler), m_handler_type(handler->m_type)
{
	ui->setupUi(this);

	ui->b_cancel->setDefault(true);
	connect(ui->b_cancel, &QAbstractButton::clicked, this, &QWidget::close);

	m_padButtons = new QButtonGroup(this);
	m_palette = ui->b_left->palette(); // save normal palette

	std::string cfg_name = PadHandlerBase::get_config_dir(m_handler_type) + profile + ".yml";

	// Adjust to the different pad handlers
	if (m_handler_type == pad_handler::keyboard)
	{
		setWindowTitle(tr("Configure Keyboard"));
		ui->b_blacklist->setEnabled(false);
		((keyboard_pad_handler*)m_handler.get())->init_config(&m_handler_cfg, cfg_name);
	}
	else if (m_handler_type == pad_handler::ds4)
	{
		setWindowTitle(tr("Configure DS4"));
		((ds4_pad_handler*)m_handler.get())->init_config(&m_handler_cfg, cfg_name);
	}
#ifdef _MSC_VER
	else if (m_handler_type == pad_handler::xinput)
	{
		setWindowTitle(tr("Configure XInput"));
		((xinput_pad_handler*)m_handler.get())->init_config(&m_handler_cfg, cfg_name);
	}
#endif
#ifdef _WIN32
	else if (m_handler_type == pad_handler::mm)
	{
		setWindowTitle(tr("Configure MMJoystick"));
		((mm_joystick_handler*)m_handler.get())->init_config(&m_handler_cfg, cfg_name);
	}
#endif
#ifdef HAVE_LIBEVDEV
	else if (m_handler_type == pad_handler::evdev)
	{
		setWindowTitle(tr("Configure evdev"));
		((evdev_joystick_handler*)m_handler.get())->init_config(&m_handler_cfg, cfg_name);
	}
#endif

	m_handler_cfg.load();

	ui->chb_vibration_large->setChecked((bool)m_handler_cfg.enable_vibration_motor_large);
	ui->chb_vibration_small->setChecked((bool)m_handler_cfg.enable_vibration_motor_small);
	ui->chb_vibration_switch->setChecked((bool)m_handler_cfg.switch_vibration_motors);

	// Enable Button Remapping
	if (m_handler->has_config())
	{
		// Use timer to get button input
		const auto& callback = [=](u16 val, std::string name, int preview_values[6])
		{
			if (m_handler->has_deadzones())
			{
				ui->preview_trigger_left->setValue(preview_values[0]);
				ui->preview_trigger_right->setValue(preview_values[1]);

				if (lx != preview_values[2] || ly != preview_values[3])
				{
					lx = preview_values[2], ly = preview_values[3];
					RepaintPreviewLabel(ui->preview_stick_left, ui->slider_stick_left->value(), ui->slider_stick_left->size().width(), lx, ly);
				}
				if (rx != preview_values[4] || ry != preview_values[5])
				{
					rx = preview_values[4], ry = preview_values[5];
					RepaintPreviewLabel(ui->preview_stick_right, ui->slider_stick_right->value(), ui->slider_stick_right->size().width(), rx, ry);
				}
			}

			if (val <= 0) return;

			LOG_NOTICE(HLE, "GetNextButtonPress: %s button %s pressed with value %d", m_handler_type, name, val);
			if (m_button_id > button_ids::id_pad_begin && m_button_id < button_ids::id_pad_end)
			{
				m_cfg_entries[m_button_id].key = name;
				m_cfg_entries[m_button_id].text = qstr(name);
				ReactivateButtons();
			}
		};

		connect(&m_timer_input, &QTimer::timeout, [=]()
		{
			std::vector<std::string> buttons =
			{
				m_cfg_entries[button_ids::id_pad_l2].key,
				m_cfg_entries[button_ids::id_pad_r2].key,
				m_cfg_entries[button_ids::id_pad_lstick_left].key,
				m_cfg_entries[button_ids::id_pad_lstick_right].key,
				m_cfg_entries[button_ids::id_pad_lstick_down].key,
				m_cfg_entries[button_ids::id_pad_lstick_up].key,
				m_cfg_entries[button_ids::id_pad_rstick_left].key,
				m_cfg_entries[button_ids::id_pad_rstick_right].key,
				m_cfg_entries[button_ids::id_pad_rstick_down].key,
				m_cfg_entries[button_ids::id_pad_rstick_up].key
			};
			m_handler->GetNextButtonPress(m_device_name, callback, false, buttons);
		});

		m_timer_input.start(1);
	};

	// Enable Vibration Checkboxes
	if (m_handler->has_rumble())
	{
		const s32 min_force = m_handler->vibration_min;
		const s32 max_force = m_handler->vibration_max;

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
	else
	{
		ui->verticalLayout_left->removeWidget(ui->gb_vibration);
		delete ui->gb_vibration;
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
		initSlider(ui->slider_trigger_left, m_handler_cfg.ltriggerthreshold, 0, m_handler->trigger_max);
		initSlider(ui->slider_trigger_right, m_handler_cfg.rtriggerthreshold, 0, m_handler->trigger_max);
		ui->preview_trigger_left->setRange(0, m_handler->trigger_max);
		ui->preview_trigger_right->setRange(0, m_handler->trigger_max);

		// Enable Stick Deadzones
		initSlider(ui->slider_stick_left, m_handler_cfg.lstickdeadzone, 0, m_handler->thumb_max);
		initSlider(ui->slider_stick_right, m_handler_cfg.rstickdeadzone, 0, m_handler->thumb_max);

		RepaintPreviewLabel(ui->preview_stick_left, ui->slider_stick_left->value(), ui->slider_stick_left->size().width(), lx, ly);
		connect(ui->slider_stick_left, &QSlider::valueChanged, [&](int value)
		{
			RepaintPreviewLabel(ui->preview_stick_left, value, ui->slider_stick_left->size().width(), lx, ly);
		});

		RepaintPreviewLabel(ui->preview_stick_right, ui->slider_stick_right->value(), ui->slider_stick_right->size().width(), rx, ry);
		connect(ui->slider_stick_right, &QSlider::valueChanged, [&](int value)
		{
			RepaintPreviewLabel(ui->preview_stick_right, value, ui->slider_stick_right->size().width(), rx, ry);
		});
	}
	else
	{
		ui->verticalLayout_right->removeWidget(ui->gb_sticks);
		ui->verticalLayout_left->removeWidget(ui->gb_triggers);

		delete ui->gb_sticks;
		delete ui->gb_triggers;
	}

	auto insertButton = [this](int id, QPushButton* button, cfg::string* cfg_name)
	{
		QString name = qstr(*cfg_name);
		m_cfg_entries.insert(std::make_pair(id, pad_button{ cfg_name, *cfg_name, name }));
		m_padButtons->addButton(button, id);
		button->setText(name);
		button->installEventFilter(this);
	};

	insertButton(button_ids::id_pad_lstick_left,  ui->b_lstick_left,  &m_handler_cfg.ls_left);
	insertButton(button_ids::id_pad_lstick_down,  ui->b_lstick_down,  &m_handler_cfg.ls_down);
	insertButton(button_ids::id_pad_lstick_right, ui->b_lstick_right, &m_handler_cfg.ls_right);
	insertButton(button_ids::id_pad_lstick_up,    ui->b_lstick_up,    &m_handler_cfg.ls_up);

	insertButton(button_ids::id_pad_left,  ui->b_left,  &m_handler_cfg.left);
	insertButton(button_ids::id_pad_down,  ui->b_down,  &m_handler_cfg.down);
	insertButton(button_ids::id_pad_right, ui->b_right, &m_handler_cfg.right);
	insertButton(button_ids::id_pad_up,    ui->b_up,    &m_handler_cfg.up);

	insertButton(button_ids::id_pad_l1, ui->b_shift_l1, &m_handler_cfg.l1);
	insertButton(button_ids::id_pad_l2, ui->b_shift_l2, &m_handler_cfg.l2);
	insertButton(button_ids::id_pad_l3, ui->b_shift_l3, &m_handler_cfg.l3);

	insertButton(button_ids::id_pad_start,  ui->b_start,  &m_handler_cfg.start);
	insertButton(button_ids::id_pad_select, ui->b_select, &m_handler_cfg.select);
	insertButton(button_ids::id_pad_ps,     ui->b_ps,     &m_handler_cfg.ps);

	insertButton(button_ids::id_pad_r1, ui->b_shift_r1, &m_handler_cfg.r1);
	insertButton(button_ids::id_pad_r2, ui->b_shift_r2, &m_handler_cfg.r2);
	insertButton(button_ids::id_pad_r3, ui->b_shift_r3, &m_handler_cfg.r3);

	insertButton(button_ids::id_pad_square,   ui->b_square,   &m_handler_cfg.square);
	insertButton(button_ids::id_pad_cross,    ui->b_cross,    &m_handler_cfg.cross);
	insertButton(button_ids::id_pad_circle,   ui->b_circle,   &m_handler_cfg.circle);
	insertButton(button_ids::id_pad_triangle, ui->b_triangle, &m_handler_cfg.triangle);

	insertButton(button_ids::id_pad_rstick_left,  ui->b_rstick_left,  &m_handler_cfg.rs_left);
	insertButton(button_ids::id_pad_rstick_down,  ui->b_rstick_down,  &m_handler_cfg.rs_down);
	insertButton(button_ids::id_pad_rstick_right, ui->b_rstick_right, &m_handler_cfg.rs_right);
	insertButton(button_ids::id_pad_rstick_up,    ui->b_rstick_up,    &m_handler_cfg.rs_up);

	m_padButtons->addButton(ui->b_reset,     button_ids::id_reset_parameters);
	m_padButtons->addButton(ui->b_blacklist, button_ids::id_blacklist);
	m_padButtons->addButton(ui->b_ok,        button_ids::id_ok);
	m_padButtons->addButton(ui->b_cancel,    button_ids::id_cancel);

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

	if (m_button_id == button_ids::id_pad_begin)
	{
		return;
	}

	if (m_padButtons->button(m_button_id))
	{
		m_padButtons->button(m_button_id)->setPalette(m_palette);
	}

	m_button_id = button_ids::id_pad_begin;
	UpdateLabel();
	SwitchButtons(true);

	for (auto but : m_padButtons->buttons())
	{
		but->setFocusPolicy(Qt::StrongFocus);
	}
}

void pad_settings_dialog::RepaintPreviewLabel(QLabel* l, int dz, int w, int x, int y)
{
	int max = m_handler->thumb_max;
	int origin = w * 0.1;
	int width = w * 0.8;
	int dz_width = width * dz / max;
	int dz_origin = (w - dz_width) / 2;

	x = (w + (x * width / max)) / 2;
	y = (w + (y * -1 * width / max)) / 2;

	QPixmap pm(w, w);
	pm.fill(Qt::transparent);
	QPainter p(&pm);
	p.setRenderHint(QPainter::Antialiasing, true);
	QPen pen(Qt::black, 2);
	p.setPen(pen);
	QBrush brush(Qt::white);
	p.setBrush(brush);
	p.drawEllipse(origin, origin, width, width);
	pen = QPen(Qt::red, 2);
	p.setPen(pen);
	p.drawEllipse(dz_origin, dz_origin, dz_width, dz_width);
	pen = QPen(Qt::blue, 2);
	p.setPen(pen);
	p.drawEllipse(x, y, 1, 1);
	l->setPixmap(pm);
}

void pad_settings_dialog::keyPressEvent(QKeyEvent *keyEvent)
{
	if (m_handler_type != pad_handler::keyboard)
	{
		return;
	}

	if (m_button_id == button_ids::id_pad_begin)
	{
		return;
	}

	if (m_button_id <= button_ids::id_pad_begin || m_button_id >= button_ids::id_pad_end)
	{
		LOG_NOTICE(HLE, "Pad Settings: Handler Type: %d, Unknown button ID: %d", static_cast<int>(m_handler_type), m_button_id);
	}
	else
	{
		m_cfg_entries[m_button_id].key = ((keyboard_pad_handler*)m_handler.get())->GetKeyName(keyEvent);
		m_cfg_entries[m_button_id].text = qstr(m_cfg_entries[m_button_id].key);
	}

	ReactivateButtons();
}

void pad_settings_dialog::mousePressEvent(QMouseEvent* event)
{
	if (m_handler_type != pad_handler::keyboard)
	{
		return;
	}

	if (m_button_id == button_ids::id_pad_begin)
	{
		return;
	}

	if (m_button_id <= button_ids::id_pad_begin || m_button_id >= button_ids::id_pad_end)
	{
		LOG_NOTICE(HLE, "Pad Settings: Handler Type: %d, Unknown button ID: %d", static_cast<int>(m_handler_type), m_button_id);
	}
	else
	{
		m_cfg_entries[m_button_id].key = ((keyboard_pad_handler*)m_handler.get())->GetMouseName(event);
		m_cfg_entries[m_button_id].text = qstr(m_cfg_entries[m_button_id].key);
	}

	ReactivateButtons();
}

bool pad_settings_dialog::eventFilter(QObject* object, QEvent* event)
{
	// Disabled buttons should not absorb mouseclicks
	if (event->type() == QEvent::MouseButtonPress)
		event->ignore();
	return QDialog::eventFilter(object, event);
}

void pad_settings_dialog::UpdateLabel(bool is_reset)
{
	if (is_reset)
	{
		if (m_handler->has_rumble())
		{
			ui->chb_vibration_large->setChecked((bool)m_handler_cfg.enable_vibration_motor_large);
			ui->chb_vibration_small->setChecked((bool)m_handler_cfg.enable_vibration_motor_small);
			ui->chb_vibration_switch->setChecked((bool)m_handler_cfg.switch_vibration_motors);
		}

		if (m_handler->has_deadzones())
		{
			ui->slider_trigger_left->setValue(m_handler_cfg.ltriggerthreshold);
			ui->slider_trigger_right->setValue(m_handler_cfg.rtriggerthreshold);
			ui->slider_stick_left->setValue(m_handler_cfg.lstickdeadzone);
			ui->slider_stick_right->setValue(m_handler_cfg.rstickdeadzone);
		}
	}

	for (auto& entry : m_cfg_entries)
	{
		if (is_reset)
		{
			entry.second.key = *entry.second.cfg_name;
			entry.second.text = qstr(entry.second.key);
		}

		m_padButtons->button(entry.first)->setText(entry.second.text);
	}
}

void pad_settings_dialog::SwitchButtons(bool is_enabled)
{
	for (int i = button_ids::id_pad_begin + 1; i < button_ids::id_pad_end; i++)
	{
		m_padButtons->button(i)->setEnabled(is_enabled);
	}
}

void pad_settings_dialog::SaveConfig()
{
	for (const auto& entry : m_cfg_entries)
	{
		entry.second.cfg_name->from_string(entry.second.key);
	}

	if (m_handler->has_rumble())
	{
		m_handler_cfg.enable_vibration_motor_large.set(ui->chb_vibration_large->isChecked());
		m_handler_cfg.enable_vibration_motor_small.set(ui->chb_vibration_small->isChecked());
		m_handler_cfg.switch_vibration_motors.set(ui->chb_vibration_switch->isChecked());
	}

	if (m_handler->has_deadzones())
	{
		m_handler_cfg.ltriggerthreshold.set(ui->slider_trigger_left->value());
		m_handler_cfg.rtriggerthreshold.set(ui->slider_trigger_right->value());
		m_handler_cfg.lstickdeadzone.set(ui->slider_stick_left->value());
		m_handler_cfg.rstickdeadzone.set(ui->slider_stick_right->value());
	}

	m_handler_cfg.save();
}

void pad_settings_dialog::OnPadButtonClicked(int id)
{
	switch (id)
	{
	case button_ids::id_pad_begin:
	case button_ids::id_pad_end:
	case button_ids::id_cancel:
		return;
	case button_ids::id_reset_parameters:
		ReactivateButtons();
		m_handler_cfg.from_default();
		UpdateLabel(true);
		return;
	case button_ids::id_blacklist:
		m_handler->GetNextButtonPress(m_device_name, nullptr, true);
		return;
	case button_ids::id_ok:
		SaveConfig();
		QDialog::accept();
		return;
	default:
		break;
	}

	for (auto but : m_padButtons->buttons())
	{
		but->setFocusPolicy(Qt::ClickFocus);
	}

	m_button_id = id;
	m_padButtons->button(m_button_id)->setText(tr("[ Waiting %1 ]").arg(MAX_SECONDS));
	m_padButtons->button(m_button_id)->setPalette(QPalette(Qt::blue));
	SwitchButtons(false); // disable all buttons, needed for using Space, Enter and other specific buttons
	m_timer.start(1000);
}
