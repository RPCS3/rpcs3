#include <QCheckBox>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QInputDialog>
#include <QMessageBox>
#include <QSvgRenderer>

#include "qt_utils.h"
#include "pad_settings_dialog.h"
#include "pad_led_settings_dialog.h"
#include "ui_pad_settings_dialog.h"
#include "tooltips.h"
#include "gui_settings.h"

#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include "Emu/Io/Null/NullPadHandler.h"
#include "Utilities/File.h"

#include "Input/pad_thread.h"
#include "Input/product_info.h"
#include "Input/keyboard_pad_handler.h"

#include <thread>

LOG_CHANNEL(cfg_log, "CFG");

inline std::string sstr(const QString& _in) { return _in.toStdString(); }
constexpr auto qstr = QString::fromStdString;

cfg_profile g_cfg_profile;

inline bool CreateConfigFile(const QString& dir, const QString& name)
{
	if (!QDir().mkpath(dir))
	{
		cfg_log.fatal("Failed to create dir %s", sstr(dir));
		return false;
	}

	const QString filename = dir + name + ".yml";
	QFile new_file(filename);

	if (!new_file.open(QIODevice::WriteOnly))
	{
		cfg_log.fatal("Failed to create file %s", sstr(filename));
		return false;
	}

	new_file.close();
	return true;
}

pad_settings_dialog::pad_settings_dialog(std::shared_ptr<gui_settings> gui_settings, QWidget *parent, const GameInfo *game)
	: QDialog(parent)
	, ui(new Ui::pad_settings_dialog)
	, m_gui_settings(std::move(gui_settings))
{
	pad::set_enabled(false);

	ui->setupUi(this);

	if (game)
	{
		m_title_id = game->serial;
		setWindowTitle(tr("Gamepad Settings: [%0] %1").arg(qstr(game->serial)).arg(qstr(game->name).simplified()));
	}
	else
	{
		setWindowTitle(tr("Gamepad Settings"));
	}

	// Load profiles
	g_cfg_profile.load();

	if (m_title_id.empty())
	{
		const QString profile_dir = qstr(rpcs3::utils::get_input_config_dir(m_title_id));
		QStringList profiles = gui::utils::get_dir_entries(QDir(profile_dir), QStringList() << "*.yml");
		QString active_profile = qstr(g_cfg_profile.active_profiles.get_value(g_cfg_profile.global_key));

		if (!profiles.contains(active_profile))
		{
			const QString default_profile = qstr(g_cfg_profile.default_profile);

			if (!profiles.contains(default_profile) && CreateConfigFile(profile_dir, default_profile))
			{
				profiles.prepend(default_profile);
			}

			active_profile = default_profile;
		}

		for (const QString& profile : profiles)
		{
			ui->chooseProfile->addItem(profile);
		}

		ui->chooseProfile->setCurrentText(active_profile);
	}
	else
	{
		ui->chooseProfile->addItem(qstr(m_title_id));
		ui->gb_profiles->setEnabled(false);
	}

	// Create tab widget for 7 players
	for (int i = 1; i < 8; i++)
	{
		const QString tab_title = tr("Player %0").arg(i);

		if (i == 1)
		{
			ui->tabWidget->setTabText(0, tab_title);
		}
		else
		{
			ui->tabWidget->addTab(new QWidget, tab_title);
		}
	}

	// On tab change: move the layout to the new tab and refresh
	connect(ui->tabWidget, &QTabWidget::currentChanged, this, &pad_settings_dialog::OnTabChanged);

	// Combobox: Input type
	connect(ui->chooseHandler, &QComboBox::currentTextChanged, this, &pad_settings_dialog::ChangeHandler);

	// Combobox: Devices
	connect(ui->chooseDevice, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &pad_settings_dialog::ChangeDevice);

	// Combobox: Profiles
	connect(ui->chooseProfile, &QComboBox::currentTextChanged, this, &pad_settings_dialog::ChangeProfile);

	// Pushbutton: Add Profile
	connect(ui->b_addProfile, &QAbstractButton::clicked, this, &pad_settings_dialog::AddProfile);

	ui->buttonBox->button(QDialogButtonBox::Reset)->setText(tr("Filter Noise"));

	connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton* button)
	{
		if (button == ui->buttonBox->button(QDialogButtonBox::Save))
		{
			SaveExit();
		}
		else if (button == ui->buttonBox->button(QDialogButtonBox::Cancel))
		{
			CancelExit();
		}
		else if (button == ui->buttonBox->button(QDialogButtonBox::Reset))
		{
			OnPadButtonClicked(button_ids::id_blacklist);
		}
		else if (button == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults))
		{
			OnPadButtonClicked(button_ids::id_reset_parameters);
		}
	});

	// Refresh Button
	connect(ui->b_refresh, &QPushButton::clicked, this, &pad_settings_dialog::RefreshHandlers);

	ui->chooseClass->addItem(tr("Standard (Pad)")); // CELL_PAD_PCLASS_TYPE_STANDARD   = 0x00,
	ui->chooseClass->addItem(tr("Guitar"));         // CELL_PAD_PCLASS_TYPE_GUITAR     = 0x01,
	ui->chooseClass->addItem(tr("Drum"));           // CELL_PAD_PCLASS_TYPE_DRUM       = 0x02,
	ui->chooseClass->addItem(tr("DJ"));             // CELL_PAD_PCLASS_TYPE_DJ         = 0x03,
	ui->chooseClass->addItem(tr("Dance Mat"));      // CELL_PAD_PCLASS_TYPE_DANCEMAT   = 0x04,
	ui->chooseClass->addItem(tr("Navigation"));     // CELL_PAD_PCLASS_TYPE_NAVIGATION = 0x05,

	connect(ui->chooseClass, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &pad_settings_dialog::HandleDeviceClassChange);

	ui->chb_show_emulated_values->setChecked(m_gui_settings->GetValue(gui::pads_show_emulated).toBool());

	connect(ui->chb_show_emulated_values, &QCheckBox::clicked, [this](bool checked)
	{
		m_gui_settings->SetValue(gui::pads_show_emulated, checked);
		const auto& cfg = GetPlayerConfig();
		RepaintPreviewLabel(ui->preview_stick_left, ui->slider_stick_left->value(), ui->slider_stick_left->size().width(), m_lx, m_ly, cfg.lpadsquircling, cfg.lstickmultiplier / 100.0);
		RepaintPreviewLabel(ui->preview_stick_right, ui->slider_stick_right->value(), ui->slider_stick_right->size().width(), m_rx, m_ry, cfg.rpadsquircling, cfg.rstickmultiplier / 100.0);
	});

	ui->mouse_movement->addItem(tr("Relative"), static_cast<int>(mouse_movement_mode::relative));
	ui->mouse_movement->addItem(tr("Absolute"), static_cast<int>(mouse_movement_mode::absolute));

	// Initialize configurable buttons
	InitButtons();

	// Initialize tooltips
	SubscribeTooltips();

	// Repaint controller image
	QSvgRenderer renderer(QStringLiteral(":/Icons/DualShock_3.svg"));
	QPixmap controller_pixmap(renderer.defaultSize() * 10);
	controller_pixmap.fill(Qt::transparent);
	QPainter painter(&controller_pixmap);
	painter.setRenderHints(QPainter::TextAntialiasing | QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
	renderer.render(&painter, controller_pixmap.rect());
	ui->l_controller->setPixmap(gui::utils::get_colorized_pixmap(controller_pixmap, QColor(), gui::utils::get_label_color("l_controller"), false, true));

	// Show default widgets first in order to calculate the required size for the scroll area (see pad_settings_dialog::ResizeDialog)
	ui->left_stack->setCurrentIndex(0);
	ui->right_stack->setCurrentIndex(0);

	// Set up first tab
	OnTabChanged(0);
	ChangeProfile(ui->chooseProfile->currentText());
}

pad_settings_dialog::~pad_settings_dialog()
{
	if (m_input_thread)
	{
		m_input_thread_state = input_thread_state::pausing;
		auto& thread = *m_input_thread;
		thread = thread_state::aborting;
		thread();
	}

	m_gui_settings->SetValue(gui::pads_geometry, saveGeometry());

	if (!Emu.IsStopped())
	{
		pad::reset(Emu.GetTitleID());
	}

	pad::set_enabled(true);
}

void pad_settings_dialog::showEvent(QShowEvent* event)
{
	RepaintPreviewLabel(ui->preview_stick_left, ui->slider_stick_left->value(), ui->slider_stick_left->size().width(), 0, 0, 0, 0);
	RepaintPreviewLabel(ui->preview_stick_right, ui->slider_stick_right->value(), ui->slider_stick_right->size().width(), 0, 0, 0, 0);

	// Resize in order to fit into our scroll area
	if (!restoreGeometry(m_gui_settings->GetValue(gui::pads_geometry).toByteArray()))
	{
		ResizeDialog();
	}

	QDialog::showEvent(event);
}

void pad_settings_dialog::InitButtons()
{
	m_pad_buttons = new QButtonGroup(this);
	m_palette = ui->b_left->palette(); // save normal palette

	const auto insert_button = [this](int id, QPushButton* button)
	{
		m_pad_buttons->addButton(button, id);
		button->installEventFilter(this);
	};

	insert_button(button_ids::id_pad_lstick_left, ui->b_lstick_left);
	insert_button(button_ids::id_pad_lstick_down, ui->b_lstick_down);
	insert_button(button_ids::id_pad_lstick_right, ui->b_lstick_right);
	insert_button(button_ids::id_pad_lstick_up, ui->b_lstick_up);

	insert_button(button_ids::id_pad_left, ui->b_left);
	insert_button(button_ids::id_pad_down, ui->b_down);
	insert_button(button_ids::id_pad_right, ui->b_right);
	insert_button(button_ids::id_pad_up, ui->b_up);

	insert_button(button_ids::id_pad_l1, ui->b_shift_l1);
	insert_button(button_ids::id_pad_l2, ui->b_shift_l2);
	insert_button(button_ids::id_pad_l3, ui->b_shift_l3);

	insert_button(button_ids::id_pad_start, ui->b_start);
	insert_button(button_ids::id_pad_select, ui->b_select);
	insert_button(button_ids::id_pad_ps, ui->b_ps);

	insert_button(button_ids::id_pad_r1, ui->b_shift_r1);
	insert_button(button_ids::id_pad_r2, ui->b_shift_r2);
	insert_button(button_ids::id_pad_r3, ui->b_shift_r3);

	insert_button(button_ids::id_pad_square, ui->b_square);
	insert_button(button_ids::id_pad_cross, ui->b_cross);
	insert_button(button_ids::id_pad_circle, ui->b_circle);
	insert_button(button_ids::id_pad_triangle, ui->b_triangle);

	insert_button(button_ids::id_pad_rstick_left, ui->b_rstick_left);
	insert_button(button_ids::id_pad_rstick_down, ui->b_rstick_down);
	insert_button(button_ids::id_pad_rstick_right, ui->b_rstick_right);
	insert_button(button_ids::id_pad_rstick_up, ui->b_rstick_up);

	insert_button(button_ids::id_pressure_intensity, ui->b_pressure_intensity);

	m_pad_buttons->addButton(ui->b_refresh, button_ids::id_refresh);
	m_pad_buttons->addButton(ui->b_addProfile, button_ids::id_add_profile);

	connect(m_pad_buttons, &QButtonGroup::idClicked, this, &pad_settings_dialog::OnPadButtonClicked);

	connect(&m_timer, &QTimer::timeout, this, [this]()
	{
		if (--m_seconds <= 0)
		{
			ReactivateButtons();
			return;
		}
		m_pad_buttons->button(m_button_id)->setText(tr("[ Waiting %1 ]").arg(m_seconds));
	});

	connect(ui->chb_vibration_large, &QCheckBox::clicked, this, [this](bool checked)
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

	connect(ui->chb_vibration_small, &QCheckBox::clicked, this, [this](bool checked)
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

	connect(ui->chb_vibration_switch, &QCheckBox::clicked, this, [this](bool checked)
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

	connect(ui->slider_stick_left, &QSlider::valueChanged, this, [&](int value)
	{
		RepaintPreviewLabel(ui->preview_stick_left, value, ui->slider_stick_left->size().width(), m_lx, m_ly, ui->squircle_left->value(), ui->stick_multi_left->value());
	});

	connect(ui->slider_stick_right, &QSlider::valueChanged, this, [&](int value)
	{
		RepaintPreviewLabel(ui->preview_stick_right, value, ui->slider_stick_right->size().width(), m_rx, m_ry, ui->squircle_right->value(), ui->stick_multi_right->value());
	});

	// Open LED settings
	connect(ui->b_led_settings, &QPushButton::clicked, this, [this]()
	{
		// Allow LED battery indication while the dialog is open
		ensure(m_handler);
		const auto& cfg = GetPlayerConfig();
		SetPadData(0, 0, cfg.led_battery_indicator.get());
		pad_led_settings_dialog dialog(this, cfg.colorR, cfg.colorG, cfg.colorB, m_handler->has_rgb(), m_handler->has_battery(), cfg.led_low_battery_blink.get(), cfg.led_battery_indicator.get(), cfg.led_battery_indicator_brightness);
		connect(&dialog, &pad_led_settings_dialog::pass_led_settings, this, &pad_settings_dialog::apply_led_settings);
		dialog.exec();
		SetPadData(0, 0);
	});

	// Enable Button Remapping
	const auto callback = [this](u16 val, std::string name, std::string pad_name, u32 battery_level, pad_preview_values preview_values)
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

			if (m_lx != preview_values[2] || m_ly != preview_values[3])
			{
				m_lx = preview_values[2];
				m_ly = preview_values[3];
				RepaintPreviewLabel(ui->preview_stick_left, ui->slider_stick_left->value(), ui->slider_stick_left->size().width(), m_lx, m_ly, ui->squircle_left->value(), ui->stick_multi_left->value());
			}
			if (m_rx != preview_values[4] || m_ry != preview_values[5])
			{
				m_rx = preview_values[4];
				m_ry = preview_values[5];
				RepaintPreviewLabel(ui->preview_stick_right, ui->slider_stick_right->value(), ui->slider_stick_right->size().width(), m_rx, m_ry, ui->squircle_right->value(), ui->stick_multi_right->value());
			}
		}

		ui->pb_battery->setValue(m_enable_battery ? battery_level : 0);

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
	const auto fail_callback = [this](const std::string& pad_name)
	{
		SwitchPadInfo(pad_name, false);

		if (m_enable_buttons)
		{
			SwitchButtons(false);
		}

		ui->pb_battery->setValue(0);

		if (m_handler->has_deadzones())
		{
			ui->preview_trigger_left->setValue(0);
			ui->preview_trigger_right->setValue(0);

			if (m_lx != 0 || m_ly != 0)
			{
				m_lx = 0;
				m_ly = 0;
				RepaintPreviewLabel(ui->preview_stick_left, ui->slider_stick_left->value(), ui->slider_stick_left->size().width(), m_lx, m_ly, ui->squircle_left->value(), ui->stick_multi_left->value());
			}
			if (m_rx != 0 || m_ry != 0)
			{
				m_rx = 0;
				m_ry = 0;
				RepaintPreviewLabel(ui->preview_stick_right, ui->slider_stick_right->value(), ui->slider_stick_right->size().width(), m_rx, m_ry, ui->squircle_right->value(), ui->stick_multi_right->value());
			}
		}
	};

	// Use timer to display button input
	connect(&m_timer_input, &QTimer::timeout, this, [this, callback, fail_callback]()
	{
		input_callback_data data;
		{
			std::lock_guard lock(m_input_mutex);
			data = m_input_callback_data;
			m_input_callback_data.has_new_data = false;
		}

		if (data.has_new_data)
		{
			if (data.success)
			{
				callback(data.val, std::move(data.name), std::move(data.pad_name), data.battery_level, std::move(data.preview_values));
			}
			else
			{
				fail_callback(data.pad_name);
			}
		}
	});

	// Use timer to refresh pad connection status
	connect(&m_timer_pad_refresh, &QTimer::timeout, this, [this]()
	{
		for (int i = 0; i < ui->chooseDevice->count(); i++)
		{
			if (!ui->chooseDevice->itemData(i).canConvert<pad_device_info>())
			{
				cfg_log.fatal("Cannot convert itemData for index %d and itemText %s", i, sstr(ui->chooseDevice->itemText(i)));
				continue;
			}

			const pad_device_info info = ui->chooseDevice->itemData(i).value<pad_device_info>();

			std::lock_guard lock(m_handler_mutex);

			m_handler->get_next_button_press(info.name,
				[this](u16, std::string, std::string pad_name, u32, pad_preview_values) { SwitchPadInfo(pad_name, true); },
				[this](std::string pad_name) { SwitchPadInfo(pad_name, false); }, false);
		}
	});

	// Use thread to get button input
	m_input_thread = std::make_unique<named_thread<std::function<void()>>>("Pad Settings Thread", [this]()
	{
		while (thread_ctrl::state() != thread_state::aborting)
		{
			thread_ctrl::wait_for(1000);

			if (m_input_thread_state != input_thread_state::active)
			{
				if (m_input_thread_state == input_thread_state::pausing)
				{
					m_input_thread_state = input_thread_state::paused;
				}

				continue;
			}

			std::lock_guard lock(m_handler_mutex);

			const std::vector<std::string> buttons =
			{
				m_cfg_entries[button_ids::id_pad_l2].key, m_cfg_entries[button_ids::id_pad_r2].key, m_cfg_entries[button_ids::id_pad_lstick_left].key,
				m_cfg_entries[button_ids::id_pad_lstick_right].key, m_cfg_entries[button_ids::id_pad_lstick_down].key, m_cfg_entries[button_ids::id_pad_lstick_up].key,
				m_cfg_entries[button_ids::id_pad_rstick_left].key, m_cfg_entries[button_ids::id_pad_rstick_right].key, m_cfg_entries[button_ids::id_pad_rstick_down].key,
				m_cfg_entries[button_ids::id_pad_rstick_up].key
			};
			m_handler->get_next_button_press(m_device_name,
				[this](u16 val, std::string name, std::string pad_name, u32 battery_level, pad_preview_values preview_values)
				{
					std::lock_guard lock(m_input_mutex);
					m_input_callback_data.val = val;
					m_input_callback_data.name = std::move(name);
					m_input_callback_data.pad_name = std::move(pad_name);
					m_input_callback_data.battery_level = battery_level;
					m_input_callback_data.preview_values = std::move(preview_values);
					m_input_callback_data.has_new_data = true;
					m_input_callback_data.success = true;
				},
				[this](std::string pad_name)
				{
					std::lock_guard lock(m_input_mutex);
					m_input_callback_data.pad_name = std::move(pad_name);
					m_input_callback_data.has_new_data = true;
					m_input_callback_data.success = false;
				},
				false, buttons);
		}
	});
}

void pad_settings_dialog::SetPadData(u32 large_motor, u32 small_motor, bool led_battery_indicator)
{
	ensure(m_handler);
	const auto& cfg = GetPlayerConfig();

	std::lock_guard lock(m_handler_mutex);
	m_handler->SetPadData(m_device_name, GetPlayerIndex(), large_motor, small_motor, cfg.colorR, cfg.colorG, cfg.colorB, led_battery_indicator, cfg.led_battery_indicator_brightness);
}

// Slot to handle the data from a signal in the led settings dialog
void pad_settings_dialog::apply_led_settings(int colorR, int colorG, int colorB, bool led_low_battery_blink, bool led_battery_indicator, int led_battery_indicator_brightness)
{
	ensure(m_handler);
	auto& cfg = GetPlayerConfig();
	cfg.colorR.set(colorR);
	cfg.colorG.set(colorG);
	cfg.colorB.set(colorB);
	cfg.led_battery_indicator.set(led_battery_indicator);
	cfg.led_battery_indicator_brightness.set(led_battery_indicator_brightness);
	cfg.led_low_battery_blink.set(led_low_battery_blink);
	SetPadData(0, 0, led_battery_indicator);
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

	auto updateButton = [this](int id, QPushButton* button, cfg::string* cfg_text)
	{
		const QString text = qstr(*cfg_text);
		m_cfg_entries.insert(std::make_pair(id, pad_button{cfg_text, *cfg_text, text}));
		button->setText(text);
	};

	auto& cfg = GetPlayerConfig();

	updateButton(button_ids::id_pad_lstick_left, ui->b_lstick_left, &cfg.ls_left);
	updateButton(button_ids::id_pad_lstick_down, ui->b_lstick_down, &cfg.ls_down);
	updateButton(button_ids::id_pad_lstick_right, ui->b_lstick_right, &cfg.ls_right);
	updateButton(button_ids::id_pad_lstick_up, ui->b_lstick_up, &cfg.ls_up);

	updateButton(button_ids::id_pad_left, ui->b_left, &cfg.left);
	updateButton(button_ids::id_pad_down, ui->b_down, &cfg.down);
	updateButton(button_ids::id_pad_right, ui->b_right, &cfg.right);
	updateButton(button_ids::id_pad_up, ui->b_up, &cfg.up);

	updateButton(button_ids::id_pad_l1, ui->b_shift_l1, &cfg.l1);
	updateButton(button_ids::id_pad_l2, ui->b_shift_l2, &cfg.l2);
	updateButton(button_ids::id_pad_l3, ui->b_shift_l3, &cfg.l3);

	updateButton(button_ids::id_pad_start, ui->b_start, &cfg.start);
	updateButton(button_ids::id_pad_select, ui->b_select, &cfg.select);
	updateButton(button_ids::id_pad_ps, ui->b_ps, &cfg.ps);

	updateButton(button_ids::id_pad_r1, ui->b_shift_r1, &cfg.r1);
	updateButton(button_ids::id_pad_r2, ui->b_shift_r2, &cfg.r2);
	updateButton(button_ids::id_pad_r3, ui->b_shift_r3, &cfg.r3);

	updateButton(button_ids::id_pad_square, ui->b_square, &cfg.square);
	updateButton(button_ids::id_pad_cross, ui->b_cross, &cfg.cross);
	updateButton(button_ids::id_pad_circle, ui->b_circle, &cfg.circle);
	updateButton(button_ids::id_pad_triangle, ui->b_triangle, &cfg.triangle);

	updateButton(button_ids::id_pad_rstick_left, ui->b_rstick_left, &cfg.rs_left);
	updateButton(button_ids::id_pad_rstick_down, ui->b_rstick_down, &cfg.rs_down);
	updateButton(button_ids::id_pad_rstick_right, ui->b_rstick_right, &cfg.rs_right);
	updateButton(button_ids::id_pad_rstick_up, ui->b_rstick_up, &cfg.rs_up);

	updateButton(button_ids::id_pressure_intensity, ui->b_pressure_intensity, &cfg.pressure_intensity_button);

	UpdateLabels(true);
}

void pad_settings_dialog::ReactivateButtons()
{
	m_timer.stop();
	m_seconds = MAX_SECONDS;

	if (m_button_id == button_ids::id_pad_begin)
	{
		return;
	}

	if (m_pad_buttons->button(m_button_id))
	{
		m_pad_buttons->button(m_button_id)->setPalette(m_palette);
		m_pad_buttons->button(m_button_id)->releaseMouse();
	}

	m_button_id = button_ids::id_pad_begin;
	UpdateLabels();
	SwitchButtons(true);

	for (auto but : m_pad_buttons->buttons())
	{
		but->setFocusPolicy(Qt::StrongFocus);
	}

	for (auto but : ui->buttonBox->buttons())
	{
		but->setFocusPolicy(Qt::StrongFocus);
	}

	ui->tabWidget->setFocusPolicy(Qt::TabFocus);
	ui->scrollArea->setFocusPolicy(Qt::StrongFocus);

	ui->chooseProfile->setFocusPolicy(Qt::WheelFocus);
	ui->chooseHandler->setFocusPolicy(Qt::WheelFocus);
	ui->chooseDevice->setFocusPolicy(Qt::WheelFocus);
	ui->chooseClass->setFocusPolicy(Qt::WheelFocus);
	ui->chooseProduct->setFocusPolicy(Qt::WheelFocus);
}

void pad_settings_dialog::RepaintPreviewLabel(QLabel* l, int deadzone, int desired_width, int x, int y, int squircle, double multiplier) const
{
	desired_width = 100; // Let's keep a fixed size for these labels for now
	const int deadzone_max = m_handler ? m_handler->thumb_max : 255; // 255 used as fallback. The deadzone circle shall be small.

	constexpr qreal relative_size = 0.9;
	const qreal device_pixel_ratio = devicePixelRatioF();
	const qreal scaled_width = desired_width * device_pixel_ratio;
	const qreal origin = desired_width / 2.0;
	const qreal outer_circle_diameter = relative_size * desired_width;
	const qreal inner_circle_diameter = outer_circle_diameter * deadzone / deadzone_max;
	const qreal outer_circle_radius = outer_circle_diameter / 2.0;
	const qreal inner_circle_radius = inner_circle_diameter / 2.0;
	const qreal stick_x = std::clamp(outer_circle_radius * x * multiplier / deadzone_max, -outer_circle_radius, outer_circle_radius);
	const qreal stick_y = std::clamp(outer_circle_radius * -y * multiplier / deadzone_max, -outer_circle_radius, outer_circle_radius);

	const bool show_emulated_values = ui->chb_show_emulated_values->isChecked();

	[[maybe_unused]] qreal ingame_x = 0.0;
	[[maybe_unused]] qreal ingame_y = 0.0;

	// Set up the canvas for our work of art
	QPixmap pixmap(scaled_width, scaled_width);
	pixmap.setDevicePixelRatio(device_pixel_ratio);
	pixmap.fill(Qt::transparent);

	// Configure the painter and set its origin
	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setRenderHint(QPainter::TextAntialiasing, true);
	painter.translate(origin, origin);
	painter.setBrush(QBrush(Qt::white));

	if (show_emulated_values)
	{
		u16 real_x = 0;
		u16 real_y = 0;

		if (m_handler)
		{
			const int m_in = multiplier * 100.0;
			const u16 normal_x = m_handler->NormalizeStickInput(static_cast<u16>(std::abs(x)), deadzone, m_in, true);
			const u16 normal_y = m_handler->NormalizeStickInput(static_cast<u16>(std::abs(y)), deadzone, m_in, true);
			const s32 x_in = x >= 0 ? normal_x : 0 - normal_x;
			const s32 y_in = y >= 0 ? normal_y : 0 - normal_y;
			m_handler->convert_stick_values(real_x, real_y, x_in, y_in, deadzone, squircle);
		}

		constexpr qreal real_max = 126;
		ingame_x = std::clamp(outer_circle_radius * (static_cast<qreal>(real_x) - real_max) / real_max, -outer_circle_radius, outer_circle_radius);
		ingame_y = std::clamp(outer_circle_radius * -(static_cast<qreal>(real_y) - real_max) / real_max, -outer_circle_radius, outer_circle_radius);

		// Draw a black outer squircle that roughly represents the DS3's max values
		QPainterPath path;
		path.addRoundedRect(QRectF(-outer_circle_radius, -outer_circle_radius, outer_circle_diameter, outer_circle_diameter), 5, 5, Qt::SizeMode::RelativeSize);
		painter.setPen(QPen(Qt::black, 1.0));
		painter.drawPath(path);
	}

	// Draw a black outer circle that represents the maximum for the deadzone
	painter.setPen(QPen(Qt::black, 1.0));
	painter.drawEllipse(QRectF(-outer_circle_radius, -outer_circle_radius, outer_circle_diameter, outer_circle_diameter));

	// Draw a red inner circle that represents the current deadzone
	painter.setPen(QPen(Qt::red, 1.0));
	painter.drawEllipse(QRectF(-inner_circle_radius, -inner_circle_radius, inner_circle_diameter, inner_circle_diameter));

	// Draw a blue dot that represents the current stick orientation
	painter.setPen(QPen(Qt::blue, 2.0));
	painter.drawEllipse(QRectF(stick_x - 0.5, stick_y - 0.5, 1.0, 1.0));

	if (show_emulated_values)
	{
		// Draw a red dot that represents the current ingame stick orientation
		painter.setPen(QPen(Qt::red, 2.0));
		painter.drawEllipse(QRectF(ingame_x - 0.5, ingame_y - 0.5, 1.0, 1.0));
	}

	l->setPixmap(pixmap);
}

void pad_settings_dialog::keyPressEvent(QKeyEvent *keyEvent)
{
	if (m_button_id == button_ids::id_pad_begin)
	{
		// We are not remapping a button, so pass the event to the base class.
		QDialog::keyPressEvent(keyEvent);
		return;
	}

	if (m_handler->m_type != pad_handler::keyboard)
	{
		// Do nothing, we don't want to interfere with the ongoing remapping.
		return;
	}

	if (keyEvent->isAutoRepeat())
	{
		return;
	}

	if (m_button_id <= button_ids::id_pad_begin || m_button_id >= button_ids::id_pad_end)
	{
		cfg_log.error("Pad Settings: Handler Type: %d, Unknown button ID: %d", static_cast<int>(m_handler->m_type), m_button_id);
	}
	else
	{
		m_cfg_entries[m_button_id].key = keyboard_pad_handler::GetKeyName(keyEvent);
		m_cfg_entries[m_button_id].text = qstr(m_cfg_entries[m_button_id].key);
	}

	ReactivateButtons();
}

void pad_settings_dialog::mouseReleaseEvent(QMouseEvent* event)
{
	if (m_button_id == button_ids::id_pad_begin)
	{
		// We are not remapping a button, so pass the event to the base class.
		QDialog::mouseReleaseEvent(event);
		return;
	}

	if (m_handler->m_type != pad_handler::keyboard)
	{
		// Do nothing, we don't want to interfere with the ongoing remapping.
		return;
	}

	if (m_button_id <= button_ids::id_pad_begin || m_button_id >= button_ids::id_pad_end)
	{
		cfg_log.error("Pad Settings: Handler Type: %d, Unknown button ID: %d", static_cast<int>(m_handler->m_type), m_button_id);
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
	if (m_button_id == button_ids::id_pad_begin)
	{
		// We are not remapping a button, so pass the event to the base class.
		QDialog::wheelEvent(event);
		return;
	}

	if (m_handler->m_type != pad_handler::keyboard)
	{
		// Do nothing, we don't want to interfere with the ongoing remapping.
		return;
	}

	if (m_button_id <= button_ids::id_pad_begin || m_button_id >= button_ids::id_pad_end)
	{
		cfg_log.error("Pad Settings: Handler Type: %d, Unknown button ID: %d", static_cast<int>(m_handler->m_type), m_button_id);
		return;
	}

	const QPoint direction = event->angleDelta();

	if (direction.isNull())
	{
		// Scrolling started/ended event, no direction given
		return;
	}

	u32 key;

	if (const int x = direction.x())
	{
		if (event->inverted() ? x < 0 : x > 0)
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

		if (event->inverted() ? y < 0 : y > 0)
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

void pad_settings_dialog::mouseMoveEvent(QMouseEvent* event)
{
	if (m_button_id == button_ids::id_pad_begin)
	{
		// We are not remapping a button, so pass the event to the base class.
		QDialog::mouseMoveEvent(event);
		return;
	}

	if (m_handler->m_type != pad_handler::keyboard)
	{
		// Do nothing, we don't want to interfere with the ongoing remapping.
		return;
	}

	if (m_button_id <= button_ids::id_pad_begin || m_button_id >= button_ids::id_pad_end)
	{
		cfg_log.error("Pad Settings: Handler Type: %d, Unknown button ID: %d", static_cast<int>(m_handler->m_type), m_button_id);
	}
	else
	{
		const QPoint mouse_pos = QCursor::pos();
		const int delta_x = mouse_pos.x() - m_last_pos.x();
		const int delta_y = mouse_pos.y() - m_last_pos.y();

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
	switch (event->type())
	{
	case QEvent::MouseButtonRelease:
	{
		// Disabled buttons should not absorb mouseclicks
		event->ignore();
		break;
	}
	case QEvent::MouseMove:
	{
		mouseMoveEvent(static_cast<QMouseEvent*>(event));
		break;
	}
	case QEvent::Enter:
	{
		if (ui->l_description && m_descriptions.contains(object))
		{
			// Check for visibility when entering a widget (needed in case of overlapping widgets in a QStackedWidget for example)
			if (const auto widget = qobject_cast<QWidget*>(object); widget && widget->isVisible())
			{
				ui->l_description->setText(m_descriptions[object]);
			}
		}
		break;
	}
	case QEvent::Leave:
	{
		if (ui->l_description && m_descriptions.contains(object))
		{
			ui->l_description->setText(m_description);
		}
		break;
	}
	default:
	{
		break;
	}
	}

	return QDialog::eventFilter(object, event);
}

void pad_settings_dialog::UpdateLabels(bool is_reset)
{
	if (is_reset)
	{
		const cfg_pad& cfg = GetPlayerConfig();

		// Update device class
		ui->chooseClass->setCurrentIndex(cfg.device_class_type);

		// Trigger the change manually in case that the class dropdown didn't fire an event
		HandleDeviceClassChange(ui->chooseClass->currentIndex());

		const auto products = input::get_products_by_class(cfg.device_class_type);

		for (usz i = 0; i < products.size(); i++)
		{
			if (products[i].vendor_id == cfg.vendor_id && products[i].product_id == cfg.product_id)
			{
				ui->chooseProduct->setCurrentIndex(static_cast<int>(i));
				break;
			}
		}

		ui->chb_vibration_large->setChecked(static_cast<bool>(cfg.enable_vibration_motor_large));
		ui->chb_vibration_small->setChecked(static_cast<bool>(cfg.enable_vibration_motor_small));
		ui->chb_vibration_switch->setChecked(static_cast<bool>(cfg.switch_vibration_motors));

		// Update Trigger Thresholds
		ui->preview_trigger_left->setRange(0, m_handler->trigger_max);
		ui->slider_trigger_left->setRange(0, m_handler->trigger_max);
		ui->slider_trigger_left->setValue(cfg.ltriggerthreshold);

		ui->preview_trigger_right->setRange(0, m_handler->trigger_max);
		ui->slider_trigger_right->setRange(0, m_handler->trigger_max);
		ui->slider_trigger_right->setValue(cfg.rtriggerthreshold);

		// Update Stick Deadzones
		ui->slider_stick_left->setRange(0, m_handler->thumb_max);
		ui->slider_stick_left->setValue(cfg.lstickdeadzone);

		ui->slider_stick_right->setRange(0, m_handler->thumb_max);
		ui->slider_stick_right->setValue(cfg.rstickdeadzone);

		std::vector<std::string> range;

		// Update Mouse Movement Mode
		const int mouse_movement_index = ui->mouse_movement->findData(static_cast<int>(cfg.mouse_move_mode.get()));
		ensure(mouse_movement_index >= 0);
		ui->mouse_movement->setCurrentIndex(mouse_movement_index);

		// Update Mouse Deadzones
		range = cfg.mouse_deadzone_x.to_list();
		ui->mouse_dz_x->setRange(std::stoi(range.front()), std::stoi(range.back()));
		ui->mouse_dz_x->setValue(cfg.mouse_deadzone_x);

		range = cfg.mouse_deadzone_y.to_list();
		ui->mouse_dz_y->setRange(std::stoi(range.front()), std::stoi(range.back()));
		ui->mouse_dz_y->setValue(cfg.mouse_deadzone_y);

		// Update Mouse Acceleration
		range = cfg.mouse_acceleration_x.to_list();
		ui->mouse_accel_x->setRange(std::stod(range.front()) / 100.0, std::stod(range.back()) / 100.0);
		ui->mouse_accel_x->setValue(cfg.mouse_acceleration_x / 100.0);

		range = cfg.mouse_acceleration_y.to_list();
		ui->mouse_accel_y->setRange(std::stod(range.front()) / 100.0, std::stod(range.back()) / 100.0);
		ui->mouse_accel_y->setValue(cfg.mouse_acceleration_y / 100.0);

		// Update Stick Lerp Factors
		range = cfg.l_stick_lerp_factor.to_list();
		ui->left_stick_lerp->setRange(std::stod(range.front()) / 100.0, std::stod(range.back()) / 100.0);
		ui->left_stick_lerp->setValue(cfg.l_stick_lerp_factor / 100.0);

		range = cfg.r_stick_lerp_factor.to_list();
		ui->right_stick_lerp->setRange(std::stod(range.front()) / 100.0, std::stod(range.back()) / 100.0);
		ui->right_stick_lerp->setValue(cfg.r_stick_lerp_factor / 100.0);

		// Update Stick Multipliers
		range = cfg.lstickmultiplier.to_list();
		ui->stick_multi_left->setRange(std::stod(range.front()) / 100.0, std::stod(range.back()) / 100.0);
		ui->stick_multi_left->setValue(cfg.lstickmultiplier / 100.0);
		ui->kb_stick_multi_left->setRange(std::stod(range.front()) / 100.0, std::stod(range.back()) / 100.0);
		ui->kb_stick_multi_left->setValue(cfg.lstickmultiplier / 100.0);

		range = cfg.rstickmultiplier.to_list();
		ui->stick_multi_right->setRange(std::stod(range.front()) / 100.0, std::stod(range.back()) / 100.0);
		ui->stick_multi_right->setValue(cfg.rstickmultiplier / 100.0);
		ui->kb_stick_multi_right->setRange(std::stod(range.front()) / 100.0, std::stod(range.back()) / 100.0);
		ui->kb_stick_multi_right->setValue(cfg.rstickmultiplier / 100.0);

		// Update Squircle Factors
		range = cfg.lpadsquircling.to_list();
		ui->squircle_left->setRange(std::stoi(range.front()), std::stoi(range.back()));
		ui->squircle_left->setValue(cfg.lpadsquircling);

		range = cfg.rpadsquircling.to_list();
		ui->squircle_right->setRange(std::stoi(range.front()), std::stoi(range.back()));
		ui->squircle_right->setValue(cfg.rpadsquircling);

		RepaintPreviewLabel(ui->preview_stick_left, ui->slider_stick_left->value(), ui->slider_stick_left->size().width(), m_lx, m_ly, cfg.lpadsquircling, cfg.lstickmultiplier / 100.0);
		RepaintPreviewLabel(ui->preview_stick_right, ui->slider_stick_right->value(), ui->slider_stick_right->size().width(), m_rx, m_ry, cfg.rpadsquircling, cfg.rstickmultiplier / 100.0);

		// Update pressure sensitivity factors
		range = cfg.pressure_intensity.to_list();
		ui->sb_pressure_intensity->setRange(std::stoi(range.front()), std::stoi(range.back()));
		ui->sb_pressure_intensity->setValue(cfg.pressure_intensity);

		// Apply stored/default LED settings to the device
		m_enable_led = m_handler->has_led();
		SetPadData(0, 0);

		// Enable battery and LED group box
		m_enable_battery = m_handler->has_battery();
		ui->gb_battery->setVisible(m_enable_battery || m_enable_led);
	}

	for (auto& entry : m_cfg_entries)
	{
		if (is_reset)
		{
			entry.second.key = *entry.second.cfg_text;
			entry.second.text = qstr(entry.second.key);
		}

		// The button has to contain at least one character, because it would be square'ish otherwise
		m_pad_buttons->button(entry.first)->setText(entry.second.text.isEmpty() ? QStringLiteral("-") : entry.second.text);
	}
}

void pad_settings_dialog::SwitchButtons(bool is_enabled)
{
	m_enable_buttons = is_enabled;

	ui->chb_show_emulated_values->setEnabled(is_enabled);
	ui->stick_multi_left->setEnabled(is_enabled);
	ui->stick_multi_right->setEnabled(is_enabled);
	ui->kb_stick_multi_left->setEnabled(is_enabled);
	ui->kb_stick_multi_right->setEnabled(is_enabled);
	ui->squircle_left->setEnabled(is_enabled);
	ui->squircle_right->setEnabled(is_enabled);
	ui->gb_pressure_intensity->setEnabled(is_enabled && m_enable_pressure_intensity_button);
	ui->gb_vibration->setEnabled(is_enabled && m_enable_rumble);
	ui->gb_sticks->setEnabled(is_enabled && m_enable_deadzones);
	ui->gb_triggers->setEnabled(is_enabled && m_enable_deadzones);
	ui->gb_battery->setEnabled(is_enabled && (m_enable_battery || m_enable_led));
	ui->pb_battery->setEnabled(is_enabled && m_enable_battery);
	ui->b_led_settings->setEnabled(is_enabled && m_enable_led);
	ui->gb_mouse_movement->setEnabled(is_enabled && m_handler->m_type == pad_handler::keyboard);
	ui->gb_mouse_accel->setEnabled(is_enabled && m_handler->m_type == pad_handler::keyboard);
	ui->gb_mouse_dz->setEnabled(is_enabled && m_handler->m_type == pad_handler::keyboard);
	ui->gb_stick_lerp->setEnabled(is_enabled && m_handler->m_type == pad_handler::keyboard);
	ui->chooseClass->setEnabled(is_enabled && ui->chooseClass->count() > 0);
	ui->chooseProduct->setEnabled(is_enabled && ui->chooseProduct->count() > 0);
	ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(is_enabled && m_handler->m_type != pad_handler::keyboard);

	for (int i = button_ids::id_pad_begin + 1; i < button_ids::id_pad_end; i++)
	{
		m_pad_buttons->button(i)->setEnabled(is_enabled);
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
		GetPlayerConfig().from_default();
		UpdateLabels(true);
		return;
	case button_ids::id_blacklist:
	{
		std::lock_guard lock(m_handler_mutex);
		m_handler->get_next_button_press(m_device_name, nullptr, nullptr, true);
		return;
	}
	default:
		break;
	}

	for (auto but : m_pad_buttons->buttons())
	{
		but->setFocusPolicy(Qt::ClickFocus);
	}

	for (auto but : ui->buttonBox->buttons())
	{
		but->setFocusPolicy(Qt::ClickFocus);
	}

	ui->tabWidget->setFocusPolicy(Qt::ClickFocus);
	ui->scrollArea->setFocusPolicy(Qt::ClickFocus);

	ui->chooseProfile->setFocusPolicy(Qt::ClickFocus);
	ui->chooseHandler->setFocusPolicy(Qt::ClickFocus);
	ui->chooseDevice->setFocusPolicy(Qt::ClickFocus);
	ui->chooseClass->setFocusPolicy(Qt::ClickFocus);
	ui->chooseProduct->setFocusPolicy(Qt::ClickFocus);

	m_last_pos = QCursor::pos();

	m_button_id = id;
	m_pad_buttons->button(m_button_id)->setText(tr("[ Waiting %1 ]").arg(MAX_SECONDS));
	m_pad_buttons->button(m_button_id)->setPalette(QPalette(Qt::blue));
	m_pad_buttons->button(m_button_id)->grabMouse();
	SwitchButtons(false); // disable all buttons, needed for using Space, Enter and other specific buttons
	m_timer.start(1000);
}

void pad_settings_dialog::OnTabChanged(int index)
{
	// Apply current config
	ApplyCurrentPlayerConfig(index);

	// Move layout to the new tab
	ui->tabWidget->widget(index)->setLayout(ui->mainLayout);

	// Refresh handlers
	RefreshHandlers();
}

void pad_settings_dialog::ChangeHandler()
{
	// Pause input thread. This means we don't have to lock the handler mutex here.
	pause_input_thread();

	bool force_enable = false; // enable configs even with disconnected devices
	const u32 player = GetPlayerIndex();
	const bool is_ldd_pad = GetIsLddPad(player);

	std::string handler;
	std::string device;

	if (is_ldd_pad)
	{
		handler = fmt::format("%s", pad_handler::null);
	}
	else
	{
		handler = sstr(ui->chooseHandler->currentData().toString());
		device = g_cfg_input.player[player]->device.to_string();
	}
	
	auto& cfg = g_cfg_input.player[player]->config;

	// Change and get this player's current handler.
	if (auto& cfg_handler = g_cfg_input.player[player]->handler; handler != cfg_handler.to_string())
	{
		if (!cfg_handler.from_string(handler))
		{
			cfg_log.error("Failed to convert input string: %s", handler);
			return;
		}

		// Initialize the new pad config's defaults
		m_handler = pad_thread::GetHandler(g_cfg_input.player[player]->handler);
		pad_thread::InitPadConfig(cfg, cfg_handler, m_handler);
	}
	else
	{
		m_handler = pad_thread::GetHandler(g_cfg_input.player[player]->handler);
	}

	ensure(m_handler);

	// Get the handler's currently available devices.
	const auto device_list = m_handler->ListDevices();

	// Localized tooltips
	const Tooltips tooltips;

	// Change the description
	switch (m_handler->m_type)
	{
	case pad_handler::null:
		GetPlayerConfig().from_default();
		if (is_ldd_pad)
			m_description = tooltips.gamepad_settings.ldd_pad;
		else
			m_description = tooltips.gamepad_settings.null;
		break;
	case pad_handler::keyboard:
		m_description = tooltips.gamepad_settings.keyboard; break;
#ifdef _WIN32
	case pad_handler::xinput:
		m_description = tooltips.gamepad_settings.xinput; break;
	case pad_handler::mm:
		m_description = tooltips.gamepad_settings.mmjoy; break;
	case pad_handler::ds3:
		m_description = tooltips.gamepad_settings.ds3_windows; break;
	case pad_handler::ds4:
		m_description = tooltips.gamepad_settings.ds4_windows; break;
	case pad_handler::dualsense:
		m_description = tooltips.gamepad_settings.dualsense_windows; break;
#elif __linux__
	case pad_handler::ds3:
		m_description = tooltips.gamepad_settings.ds3_linux; break;
	case pad_handler::ds4:
		m_description = tooltips.gamepad_settings.ds4_linux; break;
	case pad_handler::dualsense:
		m_description = tooltips.gamepad_settings.dualsense_linux; break;
#else
	case pad_handler::ds3:
		m_description = tooltips.gamepad_settings.ds3_other; break;
	case pad_handler::ds4:
		m_description = tooltips.gamepad_settings.ds4_other; break;
	case pad_handler::dualsense:
		m_description = tooltips.gamepad_settings.dualsense_other; break;
#endif
#ifdef HAVE_LIBEVDEV
	case pad_handler::evdev:
		m_description = tooltips.gamepad_settings.evdev; break;
#endif
	}
	ui->l_description->setText(m_description);

	// Update parameters
	m_min_force = m_handler->vibration_min;
	m_max_force = m_handler->vibration_max;

	// Reset parameters
	m_lx = 0;
	m_ly = 0;
	m_rx = 0;
	m_ry = 0;

	// Enable Vibration Checkboxes
	m_enable_rumble = m_handler->has_rumble();

	// Enable Deadzone Settings
	m_enable_deadzones = m_handler->has_deadzones();

	// Enable Pressure Sensitivity Settings
	m_enable_pressure_intensity_button = m_handler->has_pressure_intensity_button();

	// Change our contextual widgets
	ui->left_stack->setCurrentIndex((m_handler->m_type == pad_handler::keyboard) ? 1 : 0);
	ui->right_stack->setCurrentIndex((m_handler->m_type == pad_handler::keyboard) ? 1 : 0);
	ui->gb_pressure_intensity->setVisible(m_handler->has_pressure_intensity_button());

	// Update device dropdown and block signals while doing so
	ui->chooseDevice->blockSignals(true);
	ui->chooseDevice->clear();

	// Refill the device combobox with currently available devices
	switch (m_handler->m_type)
	{
#ifdef _WIN32
	case pad_handler::xinput:
#endif
	case pad_handler::ds3:
	case pad_handler::ds4:
	case pad_handler::dualsense:
	{
		const QString name_string = qstr(m_handler->name_string());
		for (usz i = 1; i <= m_handler->max_devices(); i++) // Controllers 1-n in GUI
		{
			const QString device_name = name_string + QString::number(i);
			ui->chooseDevice->addItem(device_name, QVariant::fromValue(pad_device_info{ sstr(device_name), true }));
		}
		force_enable = true;
		break;
	}
	case pad_handler::null:
	{
		if (is_ldd_pad)
		{
			ui->chooseDevice->setPlaceholderText(tr("Custom Controller"));
			break;
		}
		[[fallthrough]];
	}
	default:
	{
		for (const auto& device_name : device_list)
		{
			ui->chooseDevice->addItem(qstr(device_name), QVariant::fromValue(pad_device_info{ device_name, true }));
		}
		break;
	}
	}

	// Re-enable signals for device dropdown
	ui->chooseDevice->blockSignals(false);

	// Handle empty device list
	bool config_enabled = force_enable || (m_handler->m_type != pad_handler::null && ui->chooseDevice->count() > 0);

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
			m_handler->get_next_button_press(info.name,
				[this](u16, std::string, std::string pad_name, u32, pad_preview_values) { SwitchPadInfo(pad_name, true); },
				[this](std::string pad_name) { SwitchPadInfo(pad_name, false); }, false);

			if (info.name == device)
			{
				ui->chooseDevice->setCurrentIndex(i);
			}
		}

		// Force Refresh
		ChangeDevice(ui->chooseDevice->currentIndex());
	}
	else
	{
		if (ui->chooseDevice->count() == 0)
		{
			ui->chooseDevice->setPlaceholderText(tr("No Device Detected"));
		}
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

	// Reload the buttons with the new handler
	ReloadButtons();

	// Enable configuration if possible
	SwitchButtons(config_enabled && m_handler->m_type == pad_handler::keyboard);

	ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(!is_ldd_pad);
	ui->chooseDevice->setEnabled(config_enabled && ui->chooseDevice->count() > 0);
	ui->chooseHandler->setEnabled(!is_ldd_pad && ui->chooseHandler->count() > 0);

	// Re-enable input timer
	if (ui->chooseDevice->isEnabled() && ui->chooseDevice->currentIndex() >= 0)
	{
		start_input_thread();
		m_timer_input.start(1);
		m_timer_pad_refresh.start(1000);
	}
}

void pad_settings_dialog::ChangeProfile(const QString& profile)
{
	if (profile.isEmpty())
		return;

	m_profile = sstr(profile);

	// Load in order to get the pad handlers
	if (!g_cfg_input.load(m_title_id, m_profile, true))
	{
		cfg_log.notice("Loaded empty pad config");
	}

	// Adjust to the different pad handlers
	for (usz i = 0; i < g_cfg_input.player.size(); i++)
	{
		std::shared_ptr<PadHandlerBase> handler;
		pad_thread::InitPadConfig(g_cfg_input.player[i]->config, g_cfg_input.player[i]->handler, handler);
	}

	// Reload with proper defaults
	if (!g_cfg_input.load(m_title_id, m_profile, true))
	{
		cfg_log.notice("Reloaded empty pad config");
	}

	const u32 player_id = GetPlayerIndex();
	const std::string handler = fmt::format("%s", g_cfg_input.player[player_id]->handler.get());

	if (const QString q_handler = qstr(handler); ui->chooseHandler->findText(q_handler) >= 0)
	{
		ui->chooseHandler->setCurrentText(q_handler);
	}
	else
	{
		cfg_log.fatal("Handler '%s' not found in handler dropdown.", handler);
	}

	// Force Refresh
	ChangeHandler();
}

void pad_settings_dialog::ChangeDevice(int index)
{
	if (index < 0)
		return;

	const pad_device_info info = ui->chooseDevice->itemData(index).value<pad_device_info>();
	m_device_name = info.name;
	if (!g_cfg_input.player[GetPlayerIndex()]->device.from_string(m_device_name))
	{
		// Something went wrong
		cfg_log.error("Failed to convert device string: %s", m_device_name);
	}
}

void pad_settings_dialog::HandleDeviceClassChange(int index) const
{
	if (index < 0)
	{
		return;
	}

	ui->chooseProduct->clear();

	for (const auto& product : input::get_products_by_class(index))
	{
		switch (product.type)
		{
		case input::product_type::playstation_3_controller:
		{
			ui->chooseProduct->addItem(tr("PS3 Controller", "PlayStation 3 Controller"), static_cast<int>(product.type));
			break;
		}
		case input::product_type::dance_dance_revolution_mat:
		{
			ui->chooseProduct->addItem(tr("Dance Dance Revolution", "Dance Dance Revolution Mat"), static_cast<int>(product.type));
			break;
		}
		case input::product_type::dj_hero_turntable:
		{
			ui->chooseProduct->addItem(tr("DJ Hero Turntable", "DJ Hero Turntable"), static_cast<int>(product.type));
			break;
		}
		case input::product_type::harmonix_rockband_drum_kit:
		{
			ui->chooseProduct->addItem(tr("Rock Band", "Harmonix Rock Band Drum Kit"), static_cast<int>(product.type));
			break;
		}
		case input::product_type::harmonix_rockband_drum_kit_2:
		{
			ui->chooseProduct->addItem(tr("Rock Band Pro", "Harmonix Rock Band Pro-Drum Kit"), static_cast<int>(product.type));
			break;
		}
		case input::product_type::harmonix_rockband_guitar:
		{
			ui->chooseProduct->addItem(tr("Rock Band", "Harmonix Rock Band Guitar"), static_cast<int>(product.type));
			break;
		}
		case input::product_type::red_octane_gh_drum_kit:
		{
			ui->chooseProduct->addItem(tr("Guitar Hero", "RedOctane Guitar Hero Drum Kit"), static_cast<int>(product.type));
			break;
		}
		case input::product_type::red_octane_gh_guitar:
		{
			ui->chooseProduct->addItem(tr("Guitar Hero", "RedOctane Guitar Hero Guitar"), static_cast<int>(product.type));
			break;
		}
		case input::product_type::rock_revolution_drum_kit:
		{
			ui->chooseProduct->addItem(tr("Rock Revolution", "Rock Revolution Drum Controller"), static_cast<int>(product.type));
			break;
		}
		}
	}
}

void pad_settings_dialog::AddProfile()
{
	QInputDialog* dialog = new QInputDialog(this);
	dialog->setWindowTitle(tr("Choose a unique name"));
	dialog->setLabelText(tr("Profile Name: "));
	dialog->setFixedSize(500, 100);

	while (dialog->exec() != QDialog::Rejected)
	{
		const QString profile_name = dialog->textValue();

		if (profile_name.isEmpty())
		{
			QMessageBox::warning(this, tr("Error"), tr("Name cannot be empty"));
			continue;
		}
		if (profile_name.contains("."))
		{
			QMessageBox::warning(this, tr("Error"), tr("Must choose a name without '.'"));
			continue;
		}
		if (ui->chooseProfile->findText(profile_name) != -1)
		{
			QMessageBox::warning(this, tr("Error"), tr("Please choose a non-existing name"));
			continue;
		}
		if (CreateConfigFile(qstr(rpcs3::utils::get_input_config_dir(m_title_id)), profile_name))
		{
			ui->chooseProfile->addItem(profile_name);
			ui->chooseProfile->setCurrentText(profile_name);
		}
		break;
	}
}

void pad_settings_dialog::RefreshHandlers()
{
	const u32 player_id = GetPlayerIndex();

	// Set the current input type from config. Disable signal to have ChangeHandler always executed exactly once
	ui->chooseHandler->blockSignals(true);
	ui->chooseHandler->clear();

	if (GetIsLddPad(player_id))
	{
		ui->chooseHandler->addItem(tr("Reserved"));
	}
	else
	{
		const std::vector<std::string> str_inputs = g_cfg_input.player[0]->handler.to_list();
		for (usz i = 0; i < str_inputs.size(); i++)
		{
			const QString item_data = qstr(str_inputs[i]);
			ui->chooseHandler->addItem(GetLocalizedPadHandler(item_data, static_cast<pad_handler>(i)), QVariant(item_data));
		}

		const auto& handler = g_cfg_input.player[player_id]->handler;
		ui->chooseHandler->setCurrentText(GetLocalizedPadHandler(qstr(handler.to_string()), handler));
	}

	ui->chooseHandler->blockSignals(false);

	// Force Change
	ChangeHandler();
}

void pad_settings_dialog::ApplyCurrentPlayerConfig(int new_player_id)
{
	if (!m_handler || new_player_id < 0 || static_cast<u32>(new_player_id) >= g_cfg_input.player.size())
	{
		return;
	}

	m_duplicate_buttons[m_last_player_id] = "";

	auto& player = g_cfg_input.player[m_last_player_id];
	m_last_player_id = new_player_id;

	// Check for duplicate button choices
	if (m_handler->m_type != pad_handler::null)
	{
		std::set<std::string> unique_keys;
		for (const auto& entry : m_cfg_entries)
		{
			// Let's ignore special keys, unless we're using a keyboard
			if (entry.first == button_ids::id_pressure_intensity && m_handler->m_type != pad_handler::keyboard)
				continue;

			if (const auto& [it, ok] = unique_keys.insert(entry.second.key); !ok)
			{
				m_duplicate_buttons[m_last_player_id] = entry.second.key;
				break;
			}
		}
	}

	// Apply buttons
	for (const auto& entry : m_cfg_entries)
	{
		entry.second.cfg_text->from_string(entry.second.key);
	}

	// Apply rest of config
	auto& cfg = player->config;

	if (m_handler->m_type == pad_handler::keyboard)
	{
		cfg.lstickmultiplier.set(ui->kb_stick_multi_left->value() * 100);
		cfg.rstickmultiplier.set(ui->kb_stick_multi_right->value() * 100);
	}
	else
	{
		cfg.lstickmultiplier.set(ui->stick_multi_left->value() * 100);
		cfg.rstickmultiplier.set(ui->stick_multi_right->value() * 100);
	}

	cfg.lpadsquircling.set(ui->squircle_left->value());
	cfg.rpadsquircling.set(ui->squircle_right->value());

	if (m_handler->has_rumble())
	{
		cfg.enable_vibration_motor_large.set(ui->chb_vibration_large->isChecked());
		cfg.enable_vibration_motor_small.set(ui->chb_vibration_small->isChecked());
		cfg.switch_vibration_motors.set(ui->chb_vibration_switch->isChecked());
	}

	if (m_handler->has_deadzones())
	{
		cfg.ltriggerthreshold.set(ui->slider_trigger_left->value());
		cfg.rtriggerthreshold.set(ui->slider_trigger_right->value());
		cfg.lstickdeadzone.set(ui->slider_stick_left->value());
		cfg.rstickdeadzone.set(ui->slider_stick_right->value());
	}

	if (m_handler->has_pressure_intensity_button())
	{
		cfg.pressure_intensity.set(ui->sb_pressure_intensity->value());
	}

	if (m_handler->m_type == pad_handler::keyboard)
	{
		const int mouse_move_mode = ui->mouse_movement->currentData().toInt();
		ensure(mouse_move_mode >= 0 && mouse_move_mode <= 1);
		cfg.mouse_move_mode.set(static_cast<mouse_movement_mode>(mouse_move_mode));
		cfg.mouse_acceleration_x.set(ui->mouse_accel_x->value() * 100);
		cfg.mouse_acceleration_y.set(ui->mouse_accel_y->value() * 100);
		cfg.mouse_deadzone_x.set(ui->mouse_dz_x->value());
		cfg.mouse_deadzone_y.set(ui->mouse_dz_y->value());
		cfg.l_stick_lerp_factor.set(ui->left_stick_lerp->value() * 100);
		cfg.r_stick_lerp_factor.set(ui->right_stick_lerp->value() * 100);
	}

	cfg.device_class_type.set(ui->chooseClass->currentIndex());

	const auto info = input::get_product_info(static_cast<input::product_type>(ui->chooseProduct->currentData().toInt()));

	cfg.vendor_id.set(info.vendor_id);
	cfg.product_id.set(info.product_id);
}

void pad_settings_dialog::SaveExit()
{
	ApplyCurrentPlayerConfig(m_last_player_id);

	for (const auto& [player_id, key] : m_duplicate_buttons)
	{
		if (!key.empty())
		{
			int result = QMessageBox::Yes;
			m_gui_settings->ShowConfirmationBox(
				tr("Warning!"),
				tr("The %0 button <b>%1</b> of <b>Player %2</b> was assigned at least twice.<br>Please consider adjusting the configuration.<br><br>Continue anyway?<br>")
					.arg(qstr(g_cfg_input.player[player_id]->handler.to_string())).arg(qstr(key)).arg(player_id + 1),
				gui::ib_same_buttons, &result, this);

			if (result == QMessageBox::No)
				return;

			break;
		}
	}

	const std::string profile_key = m_title_id.empty() ? g_cfg_profile.global_key : m_title_id;

	g_cfg_profile.active_profiles.set_value(profile_key, m_profile);
	g_cfg_profile.save();

	g_cfg_input.save(m_title_id, m_profile);

	QDialog::accept();
}

void pad_settings_dialog::CancelExit()
{
	// Reloads configs from file or defaults
	g_cfg_profile.load();
	g_cfg_input.from_default();

	QDialog::reject();
}

QString pad_settings_dialog::GetLocalizedPadHandler(const QString& original, pad_handler handler)
{
	switch (handler)
	{
		case pad_handler::null: return tr("Null");
		case pad_handler::keyboard: return tr("Keyboard");
		case pad_handler::ds3: return tr("DualShock 3");
		case pad_handler::ds4: return tr("DualShock 4");
		case pad_handler::dualsense: return tr("DualSense");
#ifdef _WIN32
		case pad_handler::xinput: return tr("XInput");
		case pad_handler::mm: return tr("MMJoystick");
#endif
#ifdef HAVE_LIBEVDEV
		case pad_handler::evdev: return tr("Evdev");
#endif
	}
	return original;
}

bool pad_settings_dialog::GetIsLddPad(u32 index) const
{
	// We only check for ldd pads if the current dialog may affect the running application.
	// To simplify this we include the global pad config indiscriminately as well as the relevant custom pad config.
	if (!Emu.IsStopped() && (m_title_id.empty() || m_title_id == Emu.GetTitleID()))
	{
		std::lock_guard lock(pad::g_pad_mutex);
		if (const auto handler = pad::get_current_handler(true))
		{
			ensure(index < handler->GetPads().size());

			if (const std::shared_ptr<Pad> pad = handler->GetPads().at(index))
			{
				return pad->ldd;
			}
		}
	}

	return false;
}

u32 pad_settings_dialog::GetPlayerIndex() const
{
	const int player_id = ui->tabWidget->currentIndex();
	ensure(player_id >= 0 && static_cast<u32>(player_id) < g_cfg_input.player.size());
	return static_cast<u32>(player_id);
}

cfg_pad& pad_settings_dialog::GetPlayerConfig() const
{
	return g_cfg_input.player[GetPlayerIndex()]->config;
}

void pad_settings_dialog::ResizeDialog()
{
	// Widgets
	const QSize buttons_size(0, ui->buttonBox->sizeHint().height());
	const QSize tabwidget_size = ui->tabWidget->sizeHint();

	// Spacing
	const int nr_of_spacings = 1; // Number of widgets - 1
	const QSize spacing_size(0, layout()->spacing() * nr_of_spacings);

	// Margins
	const auto margins = layout()->contentsMargins();
	const QSize margin_size(margins.left() + margins.right(), margins.top() + margins.bottom());

	resize(tabwidget_size + buttons_size + margin_size + spacing_size);
}

void pad_settings_dialog::SubscribeTooltip(QObject* object, const QString& tooltip)
{
	m_descriptions[object] = tooltip;
	object->installEventFilter(this);
}

void pad_settings_dialog::SubscribeTooltips()
{
	// Localized tooltips
	const Tooltips tooltips;

	SubscribeTooltip(ui->gb_pressure_intensity, tooltips.gamepad_settings.pressure_intensity);
	SubscribeTooltip(ui->gb_squircle, tooltips.gamepad_settings.squircle_factor);
	SubscribeTooltip(ui->gb_stick_multi, tooltips.gamepad_settings.stick_multiplier);
	SubscribeTooltip(ui->gb_kb_stick_multi, tooltips.gamepad_settings.stick_multiplier);
	SubscribeTooltip(ui->gb_vibration, tooltips.gamepad_settings.vibration);
	SubscribeTooltip(ui->gb_sticks, tooltips.gamepad_settings.stick_deadzones);
	SubscribeTooltip(ui->gb_stick_preview, tooltips.gamepad_settings.emulated_preview);
	SubscribeTooltip(ui->gb_triggers, tooltips.gamepad_settings.trigger_deadzones);
	SubscribeTooltip(ui->gb_stick_lerp, tooltips.gamepad_settings.stick_lerp);
	SubscribeTooltip(ui->gb_mouse_accel, tooltips.gamepad_settings.mouse_acceleration);
	SubscribeTooltip(ui->gb_mouse_dz, tooltips.gamepad_settings.mouse_deadzones);
	SubscribeTooltip(ui->gb_mouse_movement, tooltips.gamepad_settings.mouse_movement);
}

void pad_settings_dialog::start_input_thread()
{
	m_input_thread_state = input_thread_state::active;
}

void pad_settings_dialog::pause_input_thread()
{
	if (m_input_thread)
	{
		m_input_thread_state = input_thread_state::pausing;

		while (m_input_thread_state != input_thread_state::paused)
		{
			std::this_thread::sleep_for(1ms);
		}
	}
}
