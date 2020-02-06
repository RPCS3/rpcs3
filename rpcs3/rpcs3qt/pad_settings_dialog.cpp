#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QInputDialog>
#include <QMessageBox>
#include <QColorDialog>

#include "qt_utils.h"
#include "pad_settings_dialog.h"
#include "ui_pad_settings_dialog.h"
#include "tooltips.h"

#include "Emu/Io/Null/NullPadHandler.h"

#include "Input/keyboard_pad_handler.h"
#include "Input/ds3_pad_handler.h"
#include "Input/ds4_pad_handler.h"
#ifdef _WIN32
#include "Input/xinput_pad_handler.h"
#include "Input/mm_joystick_handler.h"
#endif
#ifdef HAVE_LIBEVDEV
#include "Input/evdev_joystick_handler.h"
#endif

LOG_CHANNEL(cfg_log, "CFG");

inline std::string sstr(const QString& _in) { return _in.toStdString(); }
constexpr auto qstr = QString::fromStdString;

inline bool CreateConfigFile(const QString& dir, const QString& name)
{
	if (!QDir().mkpath(dir))
	{
		cfg_log.error("Failed to create dir %s", sstr(dir));
		return false;
	}

	const QString filename = dir + name + ".yml";
	QFile new_file(filename);

	if (!new_file.open(QIODevice::WriteOnly))
	{
		cfg_log.error("Failed to create file %s", sstr(filename));
		return false;
	}

	new_file.close();
	return true;
}

pad_settings_dialog::pad_settings_dialog(QWidget *parent, const GameInfo *game)
	: QDialog(parent), ui(new Ui::pad_settings_dialog)
{
	ui->setupUi(this);

	// load input config
	g_cfg_input.from_default();

	if (game)
	{
		m_title_id = game->serial;
		g_cfg_input.load(game->serial);
		setWindowTitle(tr("Gamepad Settings: [%0] %1").arg(qstr(game->serial)).arg(qstr(game->name).simplified()));
	}
	else
	{
		g_cfg_input.load();
		setWindowTitle(tr("Gamepad Settings"));
	}

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
	for (size_t index = 0; index < str_inputs.size(); index++)
	{
		ui->chooseHandler->addItem(qstr(str_inputs[index]));
	}

	// Combobox: Input type
	connect(ui->chooseHandler, &QComboBox::currentTextChanged, this, &pad_settings_dialog::ChangeInputType);

	// Combobox: Devices
	connect(ui->chooseDevice, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index)
	{
		if (index < 0)
		{
			return;
		}
		const pad_device_info info = ui->chooseDevice->itemData(index).value<pad_device_info>();
		m_device_name = info.name;
		if (!g_cfg_input.player[m_tabs->currentIndex()]->device.from_string(m_device_name))
		{
			// Something went wrong
			cfg_log.error("Failed to convert device string: %s", m_device_name);
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
			cfg_log.error("Failed to convert profile string: %s", m_profile);
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
			if (CreateConfigFile(qstr(PadHandlerBase::get_config_dir(g_cfg_input.player[i]->handler, m_title_id)), friendlyName))
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

	ui->chooseClass->addItem(tr("Standard (Pad)"));
	ui->chooseClass->addItem(tr("Guitar"));
	ui->chooseClass->addItem(tr("Drum"));
	ui->chooseClass->addItem(tr("DJ"));
	ui->chooseClass->addItem(tr("Dance Mat"));
	ui->chooseClass->addItem(tr("Navigation"));

	// Initialize configurable buttons
	InitButtons();

	// Set up first tab
	OnTabChanged(0);

	// repaint controller image
	ui->l_controller->setPixmap(gui::utils::get_colorized_pixmap(*ui->l_controller->pixmap(), QColor(), gui::utils::get_label_color("l_controller"), false, true));

	// set tab layout constraint to the first tab
	m_tabs->widget(0)->layout()->setSizeConstraint(QLayout::SetFixedSize);

	layout()->setSizeConstraint(QLayout::SetFixedSize);

	show();

	RepaintPreviewLabel(ui->preview_stick_left, ui->slider_stick_left->value(), ui->slider_stick_left->size().width(), 0, 0);
	RepaintPreviewLabel(ui->preview_stick_right, ui->slider_stick_right->value(), ui->slider_stick_right->size().width(), 0, 0);
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

	m_padButtons->addButton(ui->b_led, button_ids::id_led);
	m_padButtons->addButton(ui->b_reset, button_ids::id_reset_parameters);
	m_padButtons->addButton(ui->b_blacklist, button_ids::id_blacklist);
	m_padButtons->addButton(ui->b_refresh, button_ids::id_refresh);
	m_padButtons->addButton(ui->b_addProfile, button_ids::id_add_profile);
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

		ui->chb_vibration_switch->isChecked() ? SetPadData(m_min_force, m_max_force)
		                                      : SetPadData(m_max_force, m_min_force);

		QTimer::singleShot(300, [this]()
		{
			SetPadData(m_min_force, m_min_force);
		});
	});

	connect(ui->chb_vibration_small, &QCheckBox::clicked, [this](bool checked)
	{
		if (!checked)
		{
			return;
		}

		ui->chb_vibration_switch->isChecked() ? SetPadData(m_max_force, m_min_force)
		                                      : SetPadData(m_min_force, m_max_force);

		QTimer::singleShot(300, [this]()
		{
			SetPadData(m_min_force, m_min_force);
		});
	});

	connect(ui->chb_vibration_switch, &QCheckBox::clicked, [this](bool checked)
	{
		checked ? SetPadData(m_min_force, m_max_force)
		        : SetPadData(m_max_force, m_min_force);

		QTimer::singleShot(200, [this, checked]()
		{
			checked ? SetPadData(m_max_force, m_min_force)
			        : SetPadData(m_min_force, m_max_force);

			QTimer::singleShot(200, [this]()
			{
				SetPadData(m_min_force, m_min_force);
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

	connect(ui->b_led, &QPushButton::clicked, [=]()
	{
		QColor led_color(m_handler_cfg.colorR, m_handler_cfg.colorG, m_handler_cfg.colorB);
		if (ui->b_led->property("led").canConvert<QColor>())
		{
			led_color = ui->b_led->property("led").value<QColor>();
		}
		QColorDialog dlg(led_color, this);
		dlg.setWindowTitle(tr("LED Color"));
		if (dlg.exec() == QColorDialog::Accepted)
		{
			const QColor newColor = dlg.selectedColor();
			m_handler->SetPadData(m_device_name, 0, 0, newColor.red(), newColor.green(), newColor.blue());
			ui->b_led->setIcon(gui::utils::get_colorized_icon(QIcon(":/Icons/controllers.png"), Qt::black, newColor));
			ui->b_led->setProperty("led", newColor);
		}
	});

	// Enable Button Remapping
	const auto& callback = [=](u16 val, std::string name, std::string pad_name, std::array<int, 6> preview_values)
	{
		SwitchPadInfo(pad_name, true);

		if (!m_enable_buttons && !m_timer.isActive())
		{
			SwitchButtons(true);
		}
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

		cfg_log.notice("get_next_button_press: %s device %s button %s pressed with value %d", m_handler->m_type, pad_name, name, val);
		if (m_button_id > button_ids::id_pad_begin && m_button_id < button_ids::id_pad_end)
		{
			m_cfg_entries[m_button_id].key  = name;
			m_cfg_entries[m_button_id].text = qstr(name);
			ReactivateButtons();
		}
	};

	// Disable Button Remapping
	const auto& fail_callback = [this](const std::string& pad_name)
	{
		SwitchPadInfo(pad_name, false);
		if (m_enable_buttons)
		{
			SwitchButtons(false);
		}
	};

	// Use timer to get button input
	connect(&m_timer_input, &QTimer::timeout, [this, callback, fail_callback]()
	{
		std::vector<std::string> buttons =
		{
			m_cfg_entries[button_ids::id_pad_l2].key, m_cfg_entries[button_ids::id_pad_r2].key, m_cfg_entries[button_ids::id_pad_lstick_left].key,
			m_cfg_entries[button_ids::id_pad_lstick_right].key, m_cfg_entries[button_ids::id_pad_lstick_down].key, m_cfg_entries[button_ids::id_pad_lstick_up].key,
			m_cfg_entries[button_ids::id_pad_rstick_left].key, m_cfg_entries[button_ids::id_pad_rstick_right].key, m_cfg_entries[button_ids::id_pad_rstick_down].key,
			m_cfg_entries[button_ids::id_pad_rstick_up].key
		};
		m_handler->get_next_button_press(m_device_name, callback, fail_callback, false, buttons);
	});

	// Use timer to refresh pad connection status
	connect(&m_timer_pad_refresh, &QTimer::timeout, [this]()
	{
		for (int i = 0; i < ui->chooseDevice->count(); i++)
		{
			if (!ui->chooseDevice->itemData(i).canConvert<pad_device_info>())
			{
				cfg_log.fatal("Cannot convert itemData for index %d and itemText %s", i, sstr(ui->chooseDevice->itemText(i)));
				continue;
			}
			const pad_device_info info = ui->chooseDevice->itemData(i).value<pad_device_info>();
			m_handler->get_next_button_press(info.name, [=](u16, std::string, std::string pad_name, std::array<int, 6>) { SwitchPadInfo(pad_name, true); }, [=](std::string pad_name) { SwitchPadInfo(pad_name, false); }, false);
		}
	});
}

void pad_settings_dialog::SetPadData(u32 large_motor, u32 small_motor)
{
	QColor led_color(m_handler_cfg.colorR, m_handler_cfg.colorG, m_handler_cfg.colorB);
	if (ui->b_led->property("led").canConvert<QColor>())
	{
		led_color = ui->b_led->property("led").value<QColor>();
	}
	m_handler->SetPadData(m_device_name, large_motor, small_motor, led_color.red(), led_color.green(), led_color.blue());
}

void pad_settings_dialog::SwitchPadInfo(const std::string& pad_name, bool is_connected)
{
	for (int i = 0; i < ui->chooseDevice->count(); i++)
	{
		const pad_device_info info = ui->chooseDevice->itemData(i).value<pad_device_info>();
		if (info.name == pad_name)
		{
			if (info.is_connected != is_connected)
			{
				ui->chooseDevice->setItemData(i, QVariant::fromValue(pad_device_info{ pad_name, is_connected }));
				ui->chooseDevice->setItemText(i, is_connected ? qstr(pad_name) : (qstr(pad_name) + Disconnected_suffix));
			}

			if (!is_connected && m_timer.isActive() && ui->chooseDevice->currentIndex() == i)
			{
				ReactivateButtons();
			}
			break;
		}
	}
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

	ui->chb_vibration_large->setChecked(static_cast<bool>(m_handler_cfg.enable_vibration_motor_large));
	ui->chb_vibration_small->setChecked(static_cast<bool>(m_handler_cfg.enable_vibration_motor_small));
	ui->chb_vibration_switch->setChecked(static_cast<bool>(m_handler_cfg.switch_vibration_motors));

	m_min_force = m_handler->vibration_min;
	m_max_force = m_handler->vibration_max;

	ui->chooseClass->setCurrentIndex(m_handler_cfg.device_class_type);

	// Enable Mouse Deadzones
	std::vector<std::string> mouse_dz_range_x = m_handler_cfg.mouse_deadzone_x.to_list();
	ui->mouse_dz_x->setRange(std::stoi(mouse_dz_range_x.front()), std::stoi(mouse_dz_range_x.back()));
	ui->mouse_dz_x->setValue(m_handler_cfg.mouse_deadzone_x);

	std::vector<std::string> mouse_dz_range_y = m_handler_cfg.mouse_deadzone_y.to_list();
	ui->mouse_dz_y->setRange(std::stoi(mouse_dz_range_y.front()), std::stoi(mouse_dz_range_y.back()));
	ui->mouse_dz_y->setValue(m_handler_cfg.mouse_deadzone_y);

	// Enable Mouse Acceleration
	std::vector<std::string> mouse_accel_range_x = m_handler_cfg.mouse_acceleration_x.to_list();
	ui->mouse_accel_x->setRange(std::stod(mouse_accel_range_x.front()) / 100.0, std::stod(mouse_accel_range_x.back()) / 100.0);
	ui->mouse_accel_x->setValue(m_handler_cfg.mouse_acceleration_x / 100.0);

	std::vector<std::string> mouse_accel_range_y = m_handler_cfg.mouse_acceleration_y.to_list();
	ui->mouse_accel_y->setRange(std::stod(mouse_accel_range_y.front()) / 100.0, std::stod(mouse_accel_range_y.back()) / 100.0);
	ui->mouse_accel_y->setValue(m_handler_cfg.mouse_acceleration_y / 100.0);

	// Enable Stick Lerp Factors
	std::vector<std::string> left_stick_lerp_range = m_handler_cfg.l_stick_lerp_factor.to_list();
	ui->left_stick_lerp->setRange(std::stod(left_stick_lerp_range.front()) / 100.0, std::stod(left_stick_lerp_range.back()) / 100.0);
	ui->left_stick_lerp->setValue(m_handler_cfg.l_stick_lerp_factor / 100.0);

	std::vector<std::string> right_stick_lerp_range = m_handler_cfg.r_stick_lerp_factor.to_list();
	ui->right_stick_lerp->setRange(std::stod(right_stick_lerp_range.front()) / 100.0, std::stod(right_stick_lerp_range.back()) / 100.0);
	ui->right_stick_lerp->setValue(m_handler_cfg.r_stick_lerp_factor / 100.0);

	// Enable Vibration Checkboxes
	m_enable_rumble = m_handler->has_rumble();

	// Enable Deadzone Settings
	m_enable_deadzones = m_handler->has_deadzones();

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

	// Enable and repaint the LED Button
	m_enable_led = m_handler->has_led();
	m_handler->SetPadData(m_device_name, 0, 0, m_handler_cfg.colorR, m_handler_cfg.colorG, m_handler_cfg.colorB);

	const QColor led_color(m_handler_cfg.colorR, m_handler_cfg.colorG, m_handler_cfg.colorB);
	ui->b_led->setIcon(gui::utils::get_colorized_icon(QIcon(":/Icons/controllers.png"), Qt::black, led_color));
	ui->b_led->setProperty("led", led_color);
	ui->gb_led->setVisible(m_enable_led);
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
		m_padButtons->button(m_button_id)->releaseMouse();
	}

	m_button_id = button_ids::id_pad_begin;
	UpdateLabel();
	SwitchButtons(true);

	for (auto but : m_padButtons->buttons())
	{
		but->setFocusPolicy(Qt::StrongFocus);
	}

	m_tabs->setFocusPolicy(Qt::TabFocus);

	ui->chooseProfile->setFocusPolicy(Qt::WheelFocus);
	ui->chooseHandler->setFocusPolicy(Qt::WheelFocus);
	ui->chooseDevice->setFocusPolicy(Qt::WheelFocus);
	ui->chooseClass->setFocusPolicy(Qt::WheelFocus);
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
		cfg_log.notice("Pad Settings: Handler Type: %d, Unknown button ID: %d", static_cast<int>(m_handler->m_type), m_button_id);
	}
	else
	{
		m_cfg_entries[m_button_id].key = (static_cast<keyboard_pad_handler*>(m_handler.get()))->GetKeyName(keyEvent);
		m_cfg_entries[m_button_id].text = qstr(m_cfg_entries[m_button_id].key);
	}

	ReactivateButtons();
}

void pad_settings_dialog::mouseReleaseEvent(QMouseEvent* event)
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
		cfg_log.notice("Pad Settings: Handler Type: %d, Unknown button ID: %d", static_cast<int>(m_handler->m_type), m_button_id);
	}
	else
	{
		m_cfg_entries[m_button_id].key = (static_cast<keyboard_pad_handler*>(m_handler.get()))->GetMouseName(event);
		m_cfg_entries[m_button_id].text = qstr(m_cfg_entries[m_button_id].key);
	}

	ReactivateButtons();
}

void pad_settings_dialog::wheelEvent(QWheelEvent *event)
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
		cfg_log.notice("Pad Settings: Handler Type: %d, Unknown button ID: %d", static_cast<int>(m_handler->m_type), m_button_id);
		return;
	}

	QPoint direction = event->angleDelta();
	if (direction.isNull())
	{
		// Scrolling started/ended event, no direction given
		return;
	}

	u32 key;
	if (const int x = direction.x())
	{
		bool to_left = event->inverted() ? x < 0 : x > 0;
		if (to_left)
		{
			key = mouse::wheel_left;
		}
		else
		{
			key = mouse::wheel_right;
		}
	}
	else
	{
		const int y = direction.y();
		bool to_up = event->inverted() ? y < 0 : y > 0;
		if (to_up)
		{
			key = mouse::wheel_up;
		}
		else
		{
			key = mouse::wheel_down;
		}
	}
	m_cfg_entries[m_button_id].key = (static_cast<keyboard_pad_handler*>(m_handler.get()))->GetMouseName(key);
	m_cfg_entries[m_button_id].text = qstr(m_cfg_entries[m_button_id].key);
	ReactivateButtons();
}

void pad_settings_dialog::mouseMoveEvent(QMouseEvent* /*event*/)
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
		cfg_log.notice("Pad Settings: Handler Type: %d, Unknown button ID: %d", static_cast<int>(m_handler->m_type), m_button_id);
	}
	else
	{
		QPoint mouse_pos = QCursor::pos();
		int delta_x = mouse_pos.x() - m_last_pos.x();
		int delta_y = mouse_pos.y() - m_last_pos.y();

		u32 key = 0;

		if (delta_x > 100)
		{
			key = mouse::move_right;
		}
		else if (delta_x < -100)
		{
			key = mouse::move_left;
		}
		else if (delta_y > 100)
		{
			key = mouse::move_down;
		}
		else if (delta_y < -100)
		{
			key = mouse::move_up;
		}

		if (key != 0)
		{
			m_cfg_entries[m_button_id].key = (static_cast<keyboard_pad_handler*>(m_handler.get()))->GetMouseName(key);
			m_cfg_entries[m_button_id].text = qstr(m_cfg_entries[m_button_id].key);
			ReactivateButtons();
		}
	}
}

bool pad_settings_dialog::eventFilter(QObject* object, QEvent* event)
{
	// Disabled buttons should not absorb mouseclicks
	if (event->type() == QEvent::MouseButtonRelease)
	{
		event->ignore();
	}
	if (event->type() == QEvent::MouseMove)
	{
		mouseMoveEvent(static_cast<QMouseEvent*>(event));
	}
	return QDialog::eventFilter(object, event);
}

void pad_settings_dialog::UpdateLabel(bool is_reset)
{
	if (is_reset)
	{
		if (m_handler->has_rumble())
		{
			ui->chb_vibration_large->setChecked(static_cast<bool>(m_handler_cfg.enable_vibration_motor_large));
			ui->chb_vibration_small->setChecked(static_cast<bool>(m_handler_cfg.enable_vibration_motor_small));
			ui->chb_vibration_switch->setChecked(static_cast<bool>(m_handler_cfg.switch_vibration_motors));
		}

		if (m_handler->has_deadzones())
		{
			ui->slider_trigger_left->setValue(m_handler_cfg.ltriggerthreshold);
			ui->slider_trigger_right->setValue(m_handler_cfg.rtriggerthreshold);
			ui->slider_stick_left->setValue(m_handler_cfg.lstickdeadzone);
			ui->slider_stick_right->setValue(m_handler_cfg.rstickdeadzone);
		}

		if (m_handler->has_led())
		{
			const QColor led_color(m_handler_cfg.colorR, m_handler_cfg.colorG, m_handler_cfg.colorB);
			ui->b_led->setProperty("led", led_color);
			ui->b_led->setIcon(gui::utils::get_colorized_icon(QIcon(":/Icons/controllers.png"), Qt::black, led_color));
			m_handler->SetPadData(m_device_name, 0, 0, m_handler_cfg.colorR, m_handler_cfg.colorG, m_handler_cfg.colorB);
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

	ui->chooseClass->setCurrentIndex(m_handler_cfg.device_class_type);
}

void pad_settings_dialog::SwitchButtons(bool is_enabled)
{
	m_enable_buttons = is_enabled;

	ui->gb_vibration->setEnabled(is_enabled && m_enable_rumble);
	ui->gb_sticks->setEnabled(is_enabled && m_enable_deadzones);
	ui->gb_triggers->setEnabled(is_enabled && m_enable_deadzones);
	ui->gb_led->setEnabled(is_enabled && m_enable_led);
	ui->gb_mouse_accel->setEnabled(is_enabled && m_handler->m_type == pad_handler::keyboard);
	ui->gb_mouse_dz->setEnabled(is_enabled && m_handler->m_type == pad_handler::keyboard);
	ui->gb_stick_lerp->setEnabled(is_enabled && m_handler->m_type == pad_handler::keyboard);
	ui->b_blacklist->setEnabled(is_enabled && m_handler->m_type != pad_handler::keyboard);

	for (int i = button_ids::id_pad_begin + 1; i < button_ids::id_pad_end; i++)
	{
		m_padButtons->button(i)->setEnabled(is_enabled);
	}
}

void pad_settings_dialog::OnPadButtonClicked(int id)
{
	switch (id)
	{
	case button_ids::id_led:
	case button_ids::id_pad_begin:
	case button_ids::id_pad_end:
	case button_ids::id_add_profile:
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
		m_handler->get_next_button_press(m_device_name, nullptr, nullptr, true);
		return;
	default:
		break;
	}

	for (auto but : m_padButtons->buttons())
	{
		but->setFocusPolicy(Qt::ClickFocus);
	}

	m_tabs->setFocusPolicy(Qt::ClickFocus);

	ui->chooseProfile->setFocusPolicy(Qt::ClickFocus);
	ui->chooseHandler->setFocusPolicy(Qt::ClickFocus);
	ui->chooseDevice->setFocusPolicy(Qt::ClickFocus);
	ui->chooseClass->setFocusPolicy(Qt::ClickFocus);

	m_last_pos = QCursor::pos();

	m_button_id = id;
	m_padButtons->button(m_button_id)->setText(tr("[ Waiting %1 ]").arg(MAX_SECONDS));
	m_padButtons->button(m_button_id)->setPalette(QPalette(Qt::blue));
	m_padButtons->button(m_button_id)->grabMouse();
	SwitchButtons(false); // disable all buttons, needed for using Space, Enter and other specific buttons
	m_timer.start(1000);
}

void pad_settings_dialog::OnTabChanged(int index)
{
	// TODO: Do not save yet! But keep all profile changes until the dialog was saved.
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
	case pad_handler::ds3:
		ret_handler = std::make_unique<ds3_pad_handler>();
		break;
	case pad_handler::ds4:
		ret_handler = std::make_unique<ds4_pad_handler>();
		break;
#ifdef _WIN32
	case pad_handler::xinput:
		ret_handler = std::make_unique<xinput_pad_handler>();
		break;
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
		cfg_log.error("Failed to convert input string: %s", handler);
		return;
	}

	ui->chooseDevice->clear();
	ui->chooseProfile->clear();

	// Get this player's current handler and it's currently available devices
	m_handler = GetHandler(g_cfg_input.player[player]->handler);
	const auto device_list = m_handler->ListDevices();

	// Localized tooltips
	Tooltips tooltips;

	// Change the description
	QString description;
	switch (m_handler->m_type)
	{
	case pad_handler::null:
		description = tooltips.gamepad_settings.null; break;
	case pad_handler::keyboard:
		description = tooltips.gamepad_settings.keyboard; break;
#ifdef _WIN32
	case pad_handler::xinput:
		description = tooltips.gamepad_settings.xinput; break;
	case pad_handler::mm:
		description = tooltips.gamepad_settings.mmjoy; break;
	case pad_handler::ds3:
		description = tooltips.gamepad_settings.ds3_windows; break;
	case pad_handler::ds4:
		description = tooltips.gamepad_settings.ds4_windows; break;
#elif __linux__
	case pad_handler::ds3:
		description = tooltips.gamepad_settings.ds3_linux; break;
	case pad_handler::ds4:
		description = tooltips.gamepad_settings.ds4_linux; break;
#else
	case pad_handler::ds3:
		description = tooltips.gamepad_settings.ds3_other; break;
	case pad_handler::ds4:
		description = tooltips.gamepad_settings.ds4_other; break;
#endif
#ifdef HAVE_LIBEVDEV
	case pad_handler::evdev:
		description = tooltips.gamepad_settings.evdev; break;
#endif
	default:
		description = "";
	}
	ui->l_description->setText(description);

	// change our contextual widgets
	ui->left_stack->setCurrentIndex((m_handler->m_type == pad_handler::keyboard) ? 1 : 0);
	ui->right_stack->setCurrentIndex((m_handler->m_type == pad_handler::keyboard) ? 1 : 0);

	// Refill the device combobox with currently available devices
	switch (m_handler->m_type)
	{
#ifdef _WIN32
	case pad_handler::xinput:
#endif
	case pad_handler::ds3:
	case pad_handler::ds4:
	{
		const QString name_string = qstr(m_handler->name_string());
		for (size_t i = 1; i <= m_handler->max_devices(); i++) // Controllers 1-n in GUI
		{
			const QString device_name = name_string + QString::number(i);
			ui->chooseDevice->addItem(device_name, QVariant::fromValue(pad_device_info{ sstr(device_name), true }));
		}
		force_enable = true;
		break;
	}
	default:
	{
		for (size_t i = 0; i < device_list.size(); i++)
		{
			ui->chooseDevice->addItem(qstr(device_list[i]), QVariant::fromValue(pad_device_info{ device_list[i], true }));
		}
		break;
	}
	}

	// Handle empty device list
	bool config_enabled = force_enable || (m_handler->m_type != pad_handler::null && ui->chooseDevice->count() > 0);
	ui->chooseDevice->setEnabled(config_enabled);
	ui->chooseClass->setEnabled(config_enabled);

	if (config_enabled)
	{
		for (int i = 0; i < ui->chooseDevice->count(); i++)
		{
			if (!ui->chooseDevice->itemData(i).canConvert<pad_device_info>())
			{
				cfg_log.fatal("Cannot convert itemData for index %d and itemText %s", i, sstr(ui->chooseDevice->itemText(i)));
				continue;
			}
			const pad_device_info info = ui->chooseDevice->itemData(i).value<pad_device_info>();
			m_handler->get_next_button_press(info.name, [=](u16, std::string, std::string pad_name, std::array<int, 6>) { SwitchPadInfo(pad_name, true); }, [=](std::string pad_name) { SwitchPadInfo(pad_name, false); }, false);
			if (info.name == device)
			{
				ui->chooseDevice->setCurrentIndex(i);
			}
		}

		QString profile_dir = qstr(PadHandlerBase::get_config_dir(m_handler->m_type, m_title_id));
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
	SwitchButtons(config_enabled && m_handler->m_type == pad_handler::keyboard);
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
	if (m_timer_pad_refresh.isActive())
	{
		m_timer_pad_refresh.stop();
	}

	// Change handler
	const std::string cfg_name = PadHandlerBase::get_config_dir(m_handler->m_type, m_title_id) + m_profile + ".yml";

	// Adjust to the different pad handlers
	switch (m_handler->m_type)
	{
	case pad_handler::null:
		static_cast<NullPadHandler*>(m_handler.get())->init_config(&m_handler_cfg, cfg_name);
		break;
	case pad_handler::keyboard:
		static_cast<keyboard_pad_handler*>(m_handler.get())->init_config(&m_handler_cfg, cfg_name);
		break;
	case pad_handler::ds3:
		static_cast<ds3_pad_handler*>(m_handler.get())->init_config(&m_handler_cfg, cfg_name);
		break;
	case pad_handler::ds4:
		static_cast<ds4_pad_handler*>(m_handler.get())->init_config(&m_handler_cfg, cfg_name);
		break;
#ifdef _WIN32
	case pad_handler::xinput:
		static_cast<xinput_pad_handler*>(m_handler.get())->init_config(&m_handler_cfg, cfg_name);
		break;
	case pad_handler::mm:
		static_cast<mm_joystick_handler*>(m_handler.get())->init_config(&m_handler_cfg, cfg_name);
		break;
#endif
#ifdef HAVE_LIBEVDEV
	case pad_handler::evdev:
		static_cast<evdev_joystick_handler*>(m_handler.get())->init_config(&m_handler_cfg, cfg_name);
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
		m_timer_pad_refresh.start(1000);
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

	if (m_handler->has_led() && ui->b_led->property("led").canConvert<QColor>())
	{
		const QColor led_color = ui->b_led->property("led").value<QColor>();
		m_handler_cfg.colorR.set(led_color.red());
		m_handler_cfg.colorG.set(led_color.green());
		m_handler_cfg.colorB.set(led_color.blue());
	}

	if (m_handler->m_type == pad_handler::keyboard)
	{
		m_handler_cfg.mouse_acceleration_x.set(ui->mouse_accel_x->value() * 100);
		m_handler_cfg.mouse_acceleration_y.set(ui->mouse_accel_y->value() * 100);
		m_handler_cfg.mouse_deadzone_x.set(ui->mouse_dz_x->value());
		m_handler_cfg.mouse_deadzone_y.set(ui->mouse_dz_y->value());
		m_handler_cfg.l_stick_lerp_factor.set(ui->left_stick_lerp->value() * 100);
		m_handler_cfg.r_stick_lerp_factor.set(ui->right_stick_lerp->value() * 100);
	}

	m_handler_cfg.device_class_type.set(ui->chooseClass->currentIndex());

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

	g_cfg_input.save(m_title_id);

	QDialog::accept();
}

void pad_settings_dialog::CancelExit()
{
	// Reloads config from file or defaults
	g_cfg_input.from_default();
	g_cfg_input.load();

	QDialog::reject();
}
