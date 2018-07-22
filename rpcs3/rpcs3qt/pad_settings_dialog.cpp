#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QInputDialog>
#include <QMessageBox>

#include "qt_utils.h"
#include "pad_settings_dialog.h"
#include "ui_pad_settings_dialog.h"

#include "Emu/Io/Null/NullPadHandler.h"

#include "keyboard_pad_handler.h"
#include "ds4_pad_handler.h"
#ifdef _WIN32
#include "xinput_pad_handler.h"
#endif
#ifdef _MSC_VER
#include "mm_joystick_handler.h"
#endif
#ifdef HAVE_LIBEVDEV
#include "evdev_joystick_handler.h"
#endif

inline std::string sstr(const QString& _in) { return _in.toStdString(); }
constexpr auto qstr = QString::fromStdString;

inline bool CreateConfigFile(const QString& dir, const QString& name)
{
	QString input_dir = qstr(fs::get_config_dir()) + "/InputConfigs/";
	if (!QDir().mkdir(input_dir) && !QDir().exists(input_dir))
	{
		LOG_ERROR(GENERAL, "Failed to create dir %s", sstr(input_dir));
		return false;
	}
	if (!QDir().mkdir(dir) && !QDir().exists(dir))
	{
		LOG_ERROR(GENERAL, "Failed to create dir %s", sstr(dir));
		return false;
	}

	QString filename = dir + name + ".yml";
	QFile new_file(filename);

	if (!new_file.open(QIODevice::WriteOnly))
	{
		LOG_ERROR(GENERAL, "Failed to create file %s", sstr(filename));
		return false;
	}

	new_file.close();
	return true;
};

pad_settings_dialog::pad_settings_dialog(QWidget *parent)
	: QDialog(parent), ui(new Ui::pad_settings_dialog)
{
	ui->setupUi(this);

	setWindowTitle(tr("Gamepads Settings"));

	// load input config
	g_cfg_input.from_default();
	g_cfg_input.load();

	// Create tab widget for 7 players
	m_tabs = new QTabWidget;
	for (int i = 1; i < 8; i++)
	{
		QWidget* tab = new QWidget;
		m_tabs->addTab(tab, tr("Player %0").arg(i));
	}

	// on tab change: move the layout to the new tab and refresh
	connect(m_tabs, &QTabWidget::currentChanged, this, &pad_settings_dialog::OnTabChanged);

	// Set tab widget as layout
	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addWidget(m_tabs);
	setLayout(mainLayout);

	// Fill input type combobox
	std::vector<std::string> str_inputs = g_cfg_input.player[0]->handler.to_list();
	for (int index = 0; index < str_inputs.size(); index++)
	{
		ui->chooseHandler->addItem(qstr(str_inputs[index]));
	}

	// Combobox: Input type
	connect(ui->chooseHandler, &QComboBox::currentTextChanged, this, &pad_settings_dialog::ChangeInputType);

	// Combobox: Devices
	connect(ui->chooseDevice, &QComboBox::currentTextChanged, [this](const QString& dev)
	{
		if (dev.isEmpty())
		{
			return;
		}
		m_device_name = sstr(dev);
		if (!g_cfg_input.player[m_tabs->currentIndex()]->device.from_string(m_device_name))
		{
			// Something went wrong
			LOG_ERROR(GENERAL, "Failed to convert device string: %s", m_device_name);
			return;
		}
	});

	// Combobox: Profiles
	connect(ui->chooseProfile, &QComboBox::currentTextChanged, [this](const QString& prof)
	{
		if (prof.isEmpty())
		{
			return;
		}
		m_profile = sstr(prof);
		if (!g_cfg_input.player[m_tabs->currentIndex()]->profile.from_string(m_profile))
		{
			// Something went wrong
			LOG_ERROR(GENERAL, "Failed to convert profile string: %s", m_profile);
			return;
		}
		ChangeProfile();
	});

	// Pushbutton: Add Profile
	connect(ui->b_addProfile, &QAbstractButton::clicked, [=]
	{
		const int i = m_tabs->currentIndex();

		QInputDialog* dialog = new QInputDialog(this);
		dialog->setWindowTitle(tr("Choose a unique name"));
		dialog->setLabelText(tr("Profile Name: "));
		dialog->setFixedSize(500, 100);

		while (dialog->exec() != QDialog::Rejected)
		{
			QString friendlyName = dialog->textValue();
			if (friendlyName.isEmpty())
			{
				QMessageBox::warning(this, tr("Error"), tr("Name cannot be empty"));
				continue;
			}
			if (friendlyName.contains("."))
			{
				QMessageBox::warning(this, tr("Error"), tr("Must choose a name without '.'"));
				continue;
			}
			if (ui->chooseProfile->findText(friendlyName) != -1)
			{
				QMessageBox::warning(this, tr("Error"), tr("Please choose a non-existing name"));
				continue;
			}
			if (CreateConfigFile(qstr(PadHandlerBase::get_config_dir(g_cfg_input.player[i]->handler)), friendlyName))
			{
				ui->chooseProfile->addItem(friendlyName);
				ui->chooseProfile->setCurrentText(friendlyName);
			}
			break;
		}
	});

	// Cancel Button
	connect(ui->b_cancel, &QAbstractButton::clicked, this, &pad_settings_dialog::CancelExit);

	// Save Button
	connect(ui->b_ok, &QAbstractButton::clicked, this, &pad_settings_dialog::SaveExit);

	// Refresh Button
	connect(ui->b_refresh, &QPushButton::clicked, this, &pad_settings_dialog::RefreshInputTypes);

	// Initialize configurable buttons
	InitButtons();

	// Set up first tab
	OnTabChanged(0);

	// repaint and resize controller image
	ui->l_controller->setPixmap(gui::utils::get_colorized_pixmap(*ui->l_controller->pixmap(), QColor(), gui::utils::get_label_color("l_controller"), false, true));
	ui->l_controller->setMaximumSize(ui->gb_description->sizeHint().width(), ui->l_controller->maximumHeight() * ui->gb_description->sizeHint().width() / ui->l_controller->maximumWidth());

	// set tab layout constraint to the first tab
	m_tabs->widget(0)->layout()->setSizeConstraint(QLayout::SetFixedSize);

	layout()->setSizeConstraint(QLayout::SetFixedSize);
}

pad_settings_dialog::~pad_settings_dialog()
{
	delete ui;
}

void pad_settings_dialog::InitButtons()
{
	m_padButtons = new QButtonGroup(this);
	m_palette = ui->b_left->palette(); // save normal palette

	auto insertButton = [this](int id, QPushButton* button)
	{
		m_padButtons->addButton(button, id);
		button->installEventFilter(this);
	};

	insertButton(button_ids::id_pad_lstick_left, ui->b_lstick_left);
	insertButton(button_ids::id_pad_lstick_down, ui->b_lstick_down);
	insertButton(button_ids::id_pad_lstick_right, ui->b_lstick_right);
	insertButton(button_ids::id_pad_lstick_up, ui->b_lstick_up);

	insertButton(button_ids::id_pad_left, ui->b_left);
	insertButton(button_ids::id_pad_down, ui->b_down);
	insertButton(button_ids::id_pad_right, ui->b_right);
	insertButton(button_ids::id_pad_up, ui->b_up);

	insertButton(button_ids::id_pad_l1, ui->b_shift_l1);
	insertButton(button_ids::id_pad_l2, ui->b_shift_l2);
	insertButton(button_ids::id_pad_l3, ui->b_shift_l3);

	insertButton(button_ids::id_pad_start, ui->b_start);
	insertButton(button_ids::id_pad_select, ui->b_select);
	insertButton(button_ids::id_pad_ps, ui->b_ps);

	insertButton(button_ids::id_pad_r1, ui->b_shift_r1);
	insertButton(button_ids::id_pad_r2, ui->b_shift_r2);
	insertButton(button_ids::id_pad_r3, ui->b_shift_r3);

	insertButton(button_ids::id_pad_square, ui->b_square);
	insertButton(button_ids::id_pad_cross, ui->b_cross);
	insertButton(button_ids::id_pad_circle, ui->b_circle);
	insertButton(button_ids::id_pad_triangle, ui->b_triangle);

	insertButton(button_ids::id_pad_rstick_left, ui->b_rstick_left);
	insertButton(button_ids::id_pad_rstick_down, ui->b_rstick_down);
	insertButton(button_ids::id_pad_rstick_right, ui->b_rstick_right);
	insertButton(button_ids::id_pad_rstick_up, ui->b_rstick_up);

	m_padButtons->addButton(ui->b_reset, button_ids::id_reset_parameters);
	m_padButtons->addButton(ui->b_blacklist, button_ids::id_blacklist);
	m_padButtons->addButton(ui->b_refresh, button_ids::id_refresh);
	m_padButtons->addButton(ui->b_ok, button_ids::id_ok);
	m_padButtons->addButton(ui->b_cancel, button_ids::id_cancel);

	connect(m_padButtons, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &pad_settings_dialog::OnPadButtonClicked);

	connect(&m_timer, &QTimer::timeout, [this]()
	{
		if (--m_seconds <= 0)
		{
			ReactivateButtons();
			return;
		}
		m_padButtons->button(m_button_id)->setText(tr("[ Waiting %1 ]").arg(m_seconds));
	});

	connect(ui->chb_vibration_large, &QCheckBox::clicked, [this](bool checked)
	{
		if (!checked)
		{
			return;
		}

		ui->chb_vibration_switch->isChecked() ? m_handler->TestVibration(m_device_name, m_min_force, m_max_force)
		                                      : m_handler->TestVibration(m_device_name, m_max_force, m_min_force);

		QTimer::singleShot(300, [this]()
		{
			m_handler->TestVibration(m_device_name, m_min_force, m_min_force);
		});
	});

	connect(ui->chb_vibration_small, &QCheckBox::clicked, [this](bool checked)
	{
		if (!checked)
		{
			return;
		}

		ui->chb_vibration_switch->isChecked() ? m_handler->TestVibration(m_device_name, m_max_force, m_min_force)
		                                      : m_handler->TestVibration(m_device_name, m_min_force, m_max_force);

		QTimer::singleShot(300, [this]()
		{
			m_handler->TestVibration(m_device_name, m_min_force, m_min_force);
		});
	});

	connect(ui->chb_vibration_switch, &QCheckBox::clicked, [this](bool checked)
	{
		checked ? m_handler->TestVibration(m_device_name, m_min_force, m_max_force)
		        : m_handler->TestVibration(m_device_name, m_max_force, m_min_force);

		QTimer::singleShot(200, [this, checked]()
		{
			checked ? m_handler->TestVibration(m_device_name, m_max_force, m_min_force)
			        : m_handler->TestVibration(m_device_name, m_min_force, m_max_force);

			QTimer::singleShot(200, [this]()
			{
				m_handler->TestVibration(m_device_name, m_min_force, m_min_force);
			});
		});
	});

	connect(ui->slider_stick_left, &QSlider::valueChanged, [&](int value)
	{
		RepaintPreviewLabel(ui->preview_stick_left, value, ui->slider_stick_left->size().width(), lx, ly);
	});

	connect(ui->slider_stick_right, &QSlider::valueChanged, [&](int value)
	{
		RepaintPreviewLabel(ui->preview_stick_right, value, ui->slider_stick_right->size().width(), rx, ry);
	});

	// Enable Button Remapping
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

		if (val <= 0)
		{
			return;
		}

		LOG_NOTICE(HLE, "GetNextButtonPress: %s button %s pressed with value %d", m_handler->m_type, name, val);
		if (m_button_id > button_ids::id_pad_begin && m_button_id < button_ids::id_pad_end)
		{
			m_cfg_entries[m_button_id].key  = name;
			m_cfg_entries[m_button_id].text = qstr(name);
			ReactivateButtons();
		}
	};

	// Use timer to get button input
	connect(&m_timer_input, &QTimer::timeout, [this, callback]()
	{
		std::vector<std::string> buttons =
		{
			m_cfg_entries[button_ids::id_pad_l2].key, m_cfg_entries[button_ids::id_pad_r2].key, m_cfg_entries[button_ids::id_pad_lstick_left].key,
			m_cfg_entries[button_ids::id_pad_lstick_right].key, m_cfg_entries[button_ids::id_pad_lstick_down].key, m_cfg_entries[button_ids::id_pad_lstick_up].key,
			m_cfg_entries[button_ids::id_pad_rstick_left].key, m_cfg_entries[button_ids::id_pad_rstick_right].key, m_cfg_entries[button_ids::id_pad_rstick_down].key,
			m_cfg_entries[button_ids::id_pad_rstick_up].key
		};
		m_handler->GetNextButtonPress(m_device_name, callback, false, buttons);
	});
}

void pad_settings_dialog::ReloadButtons()
{
	m_cfg_entries.clear();

	auto updateButton = [this](int id, QPushButton* button, cfg::string* cfg_name)
	{
		const QString name = qstr(*cfg_name);
		m_cfg_entries.insert(std::make_pair(id, pad_button{cfg_name, *cfg_name, name}));
		button->setText(name);
	};

	updateButton(button_ids::id_pad_lstick_left, ui->b_lstick_left, &m_handler_cfg.ls_left);
	updateButton(button_ids::id_pad_lstick_down, ui->b_lstick_down, &m_handler_cfg.ls_down);
	updateButton(button_ids::id_pad_lstick_right, ui->b_lstick_right, &m_handler_cfg.ls_right);
	updateButton(button_ids::id_pad_lstick_up, ui->b_lstick_up, &m_handler_cfg.ls_up);

	updateButton(button_ids::id_pad_left, ui->b_left, &m_handler_cfg.left);
	updateButton(button_ids::id_pad_down, ui->b_down, &m_handler_cfg.down);
	updateButton(button_ids::id_pad_right, ui->b_right, &m_handler_cfg.right);
	updateButton(button_ids::id_pad_up, ui->b_up, &m_handler_cfg.up);

	updateButton(button_ids::id_pad_l1, ui->b_shift_l1, &m_handler_cfg.l1);
	updateButton(button_ids::id_pad_l2, ui->b_shift_l2, &m_handler_cfg.l2);
	updateButton(button_ids::id_pad_l3, ui->b_shift_l3, &m_handler_cfg.l3);

	updateButton(button_ids::id_pad_start, ui->b_start, &m_handler_cfg.start);
	updateButton(button_ids::id_pad_select, ui->b_select, &m_handler_cfg.select);
	updateButton(button_ids::id_pad_ps, ui->b_ps, &m_handler_cfg.ps);

	updateButton(button_ids::id_pad_r1, ui->b_shift_r1, &m_handler_cfg.r1);
	updateButton(button_ids::id_pad_r2, ui->b_shift_r2, &m_handler_cfg.r2);
	updateButton(button_ids::id_pad_r3, ui->b_shift_r3, &m_handler_cfg.r3);

	updateButton(button_ids::id_pad_square, ui->b_square, &m_handler_cfg.square);
	updateButton(button_ids::id_pad_cross, ui->b_cross, &m_handler_cfg.cross);
	updateButton(button_ids::id_pad_circle, ui->b_circle, &m_handler_cfg.circle);
	updateButton(button_ids::id_pad_triangle, ui->b_triangle, &m_handler_cfg.triangle);

	updateButton(button_ids::id_pad_rstick_left, ui->b_rstick_left, &m_handler_cfg.rs_left);
	updateButton(button_ids::id_pad_rstick_down, ui->b_rstick_down, &m_handler_cfg.rs_down);
	updateButton(button_ids::id_pad_rstick_right, ui->b_rstick_right, &m_handler_cfg.rs_right);
	updateButton(button_ids::id_pad_rstick_up, ui->b_rstick_up, &m_handler_cfg.rs_up);

	// Enable Vibration Checkboxes
	ui->gb_vibration->setEnabled(m_handler->has_rumble());

	ui->chb_vibration_large->setChecked((bool)m_handler_cfg.enable_vibration_motor_large);
	ui->chb_vibration_small->setChecked((bool)m_handler_cfg.enable_vibration_motor_small);
	ui->chb_vibration_switch->setChecked((bool)m_handler_cfg.switch_vibration_motors);

	m_min_force = m_handler->vibration_min;
	m_max_force = m_handler->vibration_max;

	// Enable Deadzone Settings
	const bool enable_deadzones = m_handler->has_deadzones();

	ui->gb_sticks->setEnabled(enable_deadzones);
	ui->gb_triggers->setEnabled(enable_deadzones);

	// Enable Trigger Thresholds
	ui->slider_trigger_left->setRange(0, m_handler->trigger_max);
	ui->slider_trigger_left->setValue(m_handler_cfg.ltriggerthreshold);

	ui->slider_trigger_right->setRange(0, m_handler->trigger_max);
	ui->slider_trigger_right->setValue(m_handler_cfg.rtriggerthreshold);

	ui->preview_trigger_left->setRange(0, m_handler->trigger_max);
	ui->preview_trigger_right->setRange(0, m_handler->trigger_max);

	// Enable Stick Deadzones
	ui->slider_stick_left->setRange(0, m_handler->thumb_max);
	ui->slider_stick_left->setValue(m_handler_cfg.lstickdeadzone);

	ui->slider_stick_right->setRange(0, m_handler->thumb_max);
	ui->slider_stick_right->setValue(m_handler_cfg.rstickdeadzone);

	RepaintPreviewLabel(ui->preview_stick_left, ui->slider_stick_left->value(), ui->slider_stick_left->size().width(), lx, ly);
	RepaintPreviewLabel(ui->preview_stick_right, ui->slider_stick_right->value(), ui->slider_stick_right->size().width(), rx, ry);
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
	if (m_handler->m_type != pad_handler::keyboard)
	{
		return;
	}

	if (m_button_id == button_ids::id_pad_begin)
	{
		return;
	}

	if (m_button_id <= button_ids::id_pad_begin || m_button_id >= button_ids::id_pad_end)
	{
		LOG_NOTICE(HLE, "Pad Settings: Handler Type: %d, Unknown button ID: %d", static_cast<int>(m_handler->m_type), m_button_id);
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
	if (m_handler->m_type != pad_handler::keyboard)
	{
		return;
	}

	if (m_button_id == button_ids::id_pad_begin)
	{
		return;
	}

	if (m_button_id <= button_ids::id_pad_begin || m_button_id >= button_ids::id_pad_end)
	{
		LOG_NOTICE(HLE, "Pad Settings: Handler Type: %d, Unknown button ID: %d", static_cast<int>(m_handler->m_type), m_button_id);
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
	{
		event->ignore();
	}
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

void pad_settings_dialog::OnPadButtonClicked(int id)
{
	switch (id)
	{
	case button_ids::id_pad_begin:
	case button_ids::id_pad_end:
	case button_ids::id_refresh:
	case button_ids::id_ok:
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

void pad_settings_dialog::OnTabChanged(int index)
{
	// Save old profile
	SaveProfile();

	// Move layout to the new tab
	m_tabs->widget(index)->setLayout(ui->mainLayout);

	// Refresh handlers
	RefreshInputTypes();
}

std::shared_ptr<PadHandlerBase> pad_settings_dialog::GetHandler(pad_handler type)
{
	std::shared_ptr<PadHandlerBase> ret_handler;

	switch (type)
	{
	case pad_handler::null:
		ret_handler = std::make_unique<NullPadHandler>();
		break;
	case pad_handler::keyboard:
		ret_handler = std::make_unique<keyboard_pad_handler>();
		break;
	case pad_handler::ds4:
		ret_handler = std::make_unique<ds4_pad_handler>();
		break;
#ifdef _MSC_VER
	case pad_handler::xinput:
		ret_handler = std::make_unique<xinput_pad_handler>();
		break;
#endif
#ifdef _WIN32
	case pad_handler::mm:
		ret_handler = std::make_unique<mm_joystick_handler>();
		break;
#endif
#ifdef HAVE_LIBEVDEV
	case pad_handler::evdev:
		ret_handler = std::make_unique<evdev_joystick_handler>();
		break;
#endif
	}

	return ret_handler;
}

void pad_settings_dialog::ChangeInputType()
{
	bool force_enable = false; // enable configs even with disconnected devices
	const int player = m_tabs->currentIndex();

	const std::string handler = sstr(ui->chooseHandler->currentText());
	const std::string device = g_cfg_input.player[player]->device.to_string();
	const std::string profile = g_cfg_input.player[player]->profile.to_string();

	// Change this player's current handler
	if (!g_cfg_input.player[player]->handler.from_string(handler))
	{
		// Something went wrong
		LOG_ERROR(GENERAL, "Failed to convert input string:%s", handler);
		return;
	}

	ui->chooseDevice->clear();
	ui->chooseProfile->clear();

	// Get this player's current handler and it's currently available devices
	m_handler = GetHandler(g_cfg_input.player[player]->handler);
	const std::vector<std::string> list_devices = m_handler->ListDevices();

	// Refill the device combobox with currently available devices
	switch (m_handler->m_type)
	{
#ifdef _MSC_VER
	case pad_handler::xinput:
	{
		const QString name_string = qstr(m_handler->name_string());
		for (int i = 0; i < m_handler->max_devices(); i++)
		{
			ui->chooseDevice->addItem(name_string + QString::number(i), i);
		}
		force_enable = true;
		break;
	}
#endif
	default:
	{
		for (int i = 0; i < list_devices.size(); i++)
		{
			ui->chooseDevice->addItem(qstr(list_devices[i]), i);
		}
		break;
	}
	}

	// Handle empty device list
	bool config_enabled = force_enable || (m_handler->m_type != pad_handler::null && list_devices.size() > 0);
	ui->chooseDevice->setEnabled(config_enabled);

	if (config_enabled)
	{
		ui->chooseDevice->setCurrentText(qstr(device));

		QString profile_dir = qstr(PadHandlerBase::get_config_dir(m_handler->m_type));
		QStringList profiles = gui::utils::get_dir_entries(QDir(profile_dir), QStringList() << "*.yml");

		if (profiles.isEmpty())
		{
			QString def_name = "Default Profile";
			if (CreateConfigFile(profile_dir, def_name))
			{
				ui->chooseProfile->addItem(def_name);
			}
			else
			{
				config_enabled = false;
			}
		}
		else
		{
			for (const auto& prof : profiles)
			{
				ui->chooseProfile->addItem(prof);
			}
			ui->chooseProfile->setCurrentText(qstr(profile));
		}
	}
	else
	{
		ui->chooseProfile->addItem(tr("No Profiles"));
		ui->chooseDevice->addItem(tr("No Device Detected"), -1);
	}

	// enable configuration and profile list if possible
	SwitchButtons(config_enabled);
	ui->b_addProfile->setEnabled(config_enabled);
	ui->chooseProfile->setEnabled(config_enabled);
}

void pad_settings_dialog::ChangeProfile()
{
	if (!m_handler)
	{
		return;
	}

	// Handle running timers
	if (m_timer.isActive())
	{
		ReactivateButtons();
	}
	if (m_timer_input.isActive())
	{
		m_timer_input.stop();
	}

	// Change handler
	const std::string cfg_name = PadHandlerBase::get_config_dir(m_handler->m_type) + m_profile + ".yml";

	// Adjust to the different pad handlers
	switch (m_handler->m_type)
	{
	case pad_handler::null:
		((NullPadHandler*)m_handler.get())->init_config(&m_handler_cfg, cfg_name);
		break;
	case pad_handler::keyboard:
		ui->b_blacklist->setEnabled(false);
		((keyboard_pad_handler*)m_handler.get())->init_config(&m_handler_cfg, cfg_name);
		break;
	case pad_handler::ds4:
		((ds4_pad_handler*)m_handler.get())->init_config(&m_handler_cfg, cfg_name);
		break;
#ifdef _MSC_VER
	case pad_handler::xinput:
		((xinput_pad_handler*)m_handler.get())->init_config(&m_handler_cfg, cfg_name);
		break;
#endif
#ifdef _WIN32
	case pad_handler::mm:
		((mm_joystick_handler*)m_handler.get())->init_config(&m_handler_cfg, cfg_name);
		break;
#endif
#ifdef HAVE_LIBEVDEV
	case pad_handler::evdev:
		((evdev_joystick_handler*)m_handler.get())->init_config(&m_handler_cfg, cfg_name);
		break;
#endif
	default:
		break;
	}

	// Load new config
	m_handler_cfg.load();

	// Reload the buttons with the new handler and profile
	ReloadButtons();

	// Reenable input timer
	if (ui->chooseDevice->isEnabled() && ui->chooseDevice->currentIndex() >= 0)
	{
		m_timer_input.start(1);
	}
}

void pad_settings_dialog::RefreshInputTypes()
{
	// Set the current input type from config. Disable signal to have ChangeInputType always executed exactly once
	ui->chooseHandler->blockSignals(true);
	ui->chooseHandler->setCurrentText(qstr(g_cfg_input.player[m_tabs->currentIndex()]->handler.to_string()));
	ui->chooseHandler->blockSignals(false);

	// Force Change
	ChangeInputType();
}

void pad_settings_dialog::SaveProfile()
{
	if (!m_handler || !ui->chooseProfile->isEnabled() || ui->chooseProfile->currentIndex() < 0)
	{
		return;
	}

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

void pad_settings_dialog::SaveExit()
{
	SaveProfile();

	// Check for invalid selection
	if (!ui->chooseDevice->isEnabled() || ui->chooseDevice->currentIndex() < 0)
	{
		const int i = m_tabs->currentIndex();

		g_cfg_input.player[i]->handler.from_default();
		g_cfg_input.player[i]->device.from_default();
		g_cfg_input.player[i]->profile.from_default();
	}

	g_cfg_input.save();

	QDialog::accept();
}

void pad_settings_dialog::CancelExit()
{
	// Reloads config from file or defaults
	g_cfg_input.from_default();
	g_cfg_input.load();

	QDialog::reject();
}
