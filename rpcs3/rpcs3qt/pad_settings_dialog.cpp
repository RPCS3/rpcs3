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
#include "pad_motion_settings_dialog.h"
#include "ui_pad_settings_dialog.h"
#include "tooltips.h"
#include "gui_settings.h"

#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include "Utilities/File.h"

#include "Input/pad_thread.h"
#include "Input/gui_pad_thread.h"
#include "Input/product_info.h"
#include "Input/keyboard_pad_handler.h"

#include <thread>

LOG_CHANNEL(cfg_log, "CFG");

cfg_input_configurations g_cfg_input_configs;

inline bool CreateConfigFile(const QString& dir, const QString& name)
{
	if (!QDir().mkpath(dir))
	{
		cfg_log.fatal("Failed to create dir %s", dir);
		return false;
	}

	const QString filename = dir + name + ".yml";
	QFile new_file(filename);

	if (!new_file.open(QIODevice::WriteOnly))
	{
		cfg_log.fatal("Failed to create file %s", filename);
		return false;
	}

	new_file.close();
	return true;
}

void pad_settings_dialog::pad_button::insert_key(const std::string& key, bool append_key)
{
	std::vector<std::string> buttons;
	if (append_key)
	{
		buttons = cfg_pad::get_buttons(keys);
	}
	buttons.push_back(key);

	keys = cfg_pad::get_buttons(std::move(buttons));
	text = QString::fromStdString(keys).replace(",", ", ");
}

pad_settings_dialog::pad_settings_dialog(std::shared_ptr<gui_settings> gui_settings, QWidget* parent, const GameInfo* game)
	: QDialog(parent)
	, ui(new Ui::pad_settings_dialog)
	, m_gui_settings(std::move(gui_settings))
{
	pad::set_enabled(false);

	ui->setupUi(this);

	if (game)
	{
		m_title_id = game->serial;
		setWindowTitle(tr("Gamepad Settings: [%0] %1").arg(QString::fromStdString(game->serial)).arg(QString::fromStdString(game->name).simplified()));
	}
	else
	{
		setWindowTitle(tr("Gamepad Settings"));
	}

	// Load input configs
	g_cfg_input_configs.load();

	if (m_title_id.empty())
	{
		const auto [config_files, active_config_file] = get_config_files();

		for (const QString& profile : config_files)
		{
			ui->chooseConfig->addItem(profile);
		}

		ui->chooseConfig->setCurrentText(active_config_file);
	}
	else
	{
		ui->chooseConfig->addItem(QString::fromStdString(m_title_id));
		ui->gb_config_files->setEnabled(false);
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

	// Combobox: Configs
	connect(ui->chooseConfig, &QComboBox::currentTextChanged, this, &pad_settings_dialog::ChangeConfig);

	// Pushbutton: Add config file
	connect(ui->b_addConfig, &QAbstractButton::clicked, this, &pad_settings_dialog::AddConfigFile);

	// Pushbutton: Remove config file
	connect(ui->b_remConfig, &QAbstractButton::clicked, this, &pad_settings_dialog::RemoveConfigFile);

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

	ui->chooseClass->addItem(tr("Standard (Pad)"),     u32{CELL_PAD_PCLASS_TYPE_STANDARD});
	ui->chooseClass->addItem(tr("Guitar"),             u32{CELL_PAD_PCLASS_TYPE_GUITAR});
	ui->chooseClass->addItem(tr("Drum"),               u32{CELL_PAD_PCLASS_TYPE_DRUM});
	ui->chooseClass->addItem(tr("DJ"),                 u32{CELL_PAD_PCLASS_TYPE_DJ});
	ui->chooseClass->addItem(tr("Dance Mat"),          u32{CELL_PAD_PCLASS_TYPE_DANCEMAT});
	ui->chooseClass->addItem(tr("PS Move Navigation"), u32{CELL_PAD_PCLASS_TYPE_NAVIGATION});
	ui->chooseClass->addItem(tr("Skateboard"),         u32{CELL_PAD_PCLASS_TYPE_SKATEBOARD});
	ui->chooseClass->addItem(tr("GunCon 3"),           u32{CELL_PAD_FAKE_TYPE_GUNCON3});
	ui->chooseClass->addItem(tr("Top Shot Elite"),     u32{CELL_PAD_FAKE_TYPE_TOP_SHOT_ELITE});
	ui->chooseClass->addItem(tr("Top Shot Fearmaster"),u32{CELL_PAD_FAKE_TYPE_TOP_SHOT_FEARMASTER});
	ui->chooseClass->addItem(tr("uDraw GameTablet"),   u32{CELL_PAD_FAKE_TYPE_GAMETABLET});
	ui->chooseClass->addItem(tr("Copilot for Player 1"), u32{CELL_PAD_FAKE_TYPE_COPILOT_1});
	ui->chooseClass->addItem(tr("Copilot for Player 2"), u32{CELL_PAD_FAKE_TYPE_COPILOT_2});
	ui->chooseClass->addItem(tr("Copilot for Player 3"), u32{CELL_PAD_FAKE_TYPE_COPILOT_3});
	ui->chooseClass->addItem(tr("Copilot for Player 4"), u32{CELL_PAD_FAKE_TYPE_COPILOT_4});
	ui->chooseClass->addItem(tr("Copilot for Player 5"), u32{CELL_PAD_FAKE_TYPE_COPILOT_5});
	ui->chooseClass->addItem(tr("Copilot for Player 6"), u32{CELL_PAD_FAKE_TYPE_COPILOT_6});
	ui->chooseClass->addItem(tr("Copilot for Player 7"), u32{CELL_PAD_FAKE_TYPE_COPILOT_7});

	connect(ui->chooseClass, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index)
	{
		if (index < 0) return;
		HandleDeviceClassChange(ui->chooseClass->currentData().toUInt());
	});

	ui->chb_show_emulated_values->setChecked(m_gui_settings->GetValue(gui::pads_show_emulated).toBool());

	connect(ui->chb_show_emulated_values, &QCheckBox::clicked, [this](bool checked)
	{
		m_gui_settings->SetValue(gui::pads_show_emulated, checked);
		const cfg_pad& cfg = GetPlayerConfig();
		RepaintPreviewLabel(ui->preview_stick_left, ui->slider_stick_left->value(), ui->anti_deadzone_slider_stick_left->value(), ui->slider_stick_left->size().width(), m_lx, m_ly, cfg.lpadsquircling, cfg.lstickmultiplier / 100.0);
		RepaintPreviewLabel(ui->preview_stick_right, ui->slider_stick_right->value(), ui->anti_deadzone_slider_stick_right->value(), ui->slider_stick_right->size().width(), m_rx, m_ry, cfg.rpadsquircling, cfg.rstickmultiplier / 100.0);
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
	const QColor color = gui::utils::get_foreground_color();
	ui->l_controller->setPixmap(gui::utils::get_colorized_pixmap(controller_pixmap, QColor(), gui::utils::get_label_color("l_controller", color, color), false, true));

	// Show default widgets first in order to calculate the required size for the scroll area (see pad_settings_dialog::ResizeDialog)
	ui->left_stack->setCurrentIndex(0);
	ui->right_stack->setCurrentIndex(0);

	// Set up first tab
	OnTabChanged(0);
	ChangeConfig(ui->chooseConfig->currentText());
}

void pad_settings_dialog::closeEvent(QCloseEvent* event)
{
	m_gui_settings->SetValue(gui::pads_geometry, saveGeometry());

	QDialog::closeEvent(event);
}

pad_settings_dialog::~pad_settings_dialog()
{
	if (m_input_thread)
	{
		m_input_thread_state = input_thread_state::pausing;
		*m_input_thread = thread_state::finished;
	}

	gui_pad_thread::reset();

	if (!Emu.IsStopped())
	{
		pad::reset(Emu.GetTitleID());
	}

	pad::set_enabled(true);
}

void pad_settings_dialog::showEvent(QShowEvent* event)
{
	RepaintPreviewLabel(ui->preview_stick_left, ui->slider_stick_left->value(), ui->anti_deadzone_slider_stick_left->value(), ui->slider_stick_left->size().width(), 0, 0, 0, 0);
	RepaintPreviewLabel(ui->preview_stick_right, ui->slider_stick_right->value(), ui->anti_deadzone_slider_stick_right->value(), ui->slider_stick_right->size().width(), 0, 0, 0, 0);

	// Resize in order to fit into our scroll area
	if (!restoreGeometry(m_gui_settings->GetValue(gui::pads_geometry).toByteArray()))
	{
		ResizeDialog();
	}

	QDialog::showEvent(event);
}

std::pair<QStringList, QString> pad_settings_dialog::get_config_files()
{
	const QString input_config_dir = QString::fromStdString(rpcs3::utils::get_input_config_dir(m_title_id));
	QStringList config_files = gui::utils::get_dir_entries(QDir(input_config_dir), QStringList() << "*.yml");
	QString active_config_file = QString::fromStdString(g_cfg_input_configs.active_configs.get_value(g_cfg_input_configs.global_key));

	if (!config_files.contains(active_config_file))
	{
		const QString default_config_file = QString::fromStdString(g_cfg_input_configs.default_config);

		if (!config_files.contains(default_config_file) && CreateConfigFile(input_config_dir, default_config_file))
		{
			config_files.prepend(default_config_file);
		}

		active_config_file = default_config_file;
	}

	return std::make_pair<QStringList, QString>(std::move(config_files), std::move(active_config_file));
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
	insert_button(button_ids::id_analog_limiter, ui->b_analog_limiter);
	insert_button(button_ids::id_orientation_reset, ui->b_orientation_reset);

	m_pad_buttons->addButton(ui->b_refresh, button_ids::id_refresh);
	m_pad_buttons->addButton(ui->b_addConfig, button_ids::id_add_config_file);
	m_pad_buttons->addButton(ui->b_remConfig, button_ids::id_remove_config_file);

	connect(m_pad_buttons, &QButtonGroup::idClicked, this, &pad_settings_dialog::OnPadButtonClicked);

	connect(&m_remap_timer, &QTimer::timeout, this, [this]()
	{
		if (--m_seconds <= 0)
		{
			ReactivateButtons();
			return;
		}
		if (auto button = m_pad_buttons->button(m_button_id))
		{
			button->setText(tr("[ Waiting %1 ]").arg(m_seconds));
		}
	});

	connect(ui->sb_vibration_large, &QSpinBox::valueChanged, this, [this](int value)
	{
		const u8 force = static_cast<u8>(std::clamp(m_max_force * (value / 100.0f), 0.0f, 255.0f));
		ui->chb_vibration_switch->isChecked() ? SetPadData(m_min_force, force)
		                                      : SetPadData(force, m_min_force);

		QTimer::singleShot(300, this, [this]()
		{
			SetPadData(m_min_force, m_min_force);
		});
	});

	connect(ui->sb_vibration_small, &QSpinBox::valueChanged, this, [this](int value)
	{
		const u8 force = static_cast<u8>(std::clamp(m_max_force * (value / 100.0f), 0.0f, 255.0f));
		ui->chb_vibration_switch->isChecked() ? SetPadData(force, m_min_force)
		                                      : SetPadData(m_min_force, force);

		QTimer::singleShot(300, this, [this]()
		{
			SetPadData(m_min_force, m_min_force);
		});
	});

	connect(ui->chb_vibration_switch, &QCheckBox::clicked, this, [this](bool checked)
	{
		checked ? SetPadData(m_min_force, m_max_force)
		        : SetPadData(m_max_force, m_min_force);

		QTimer::singleShot(200, this, [this, checked]()
		{
			checked ? SetPadData(m_max_force, m_min_force)
			        : SetPadData(m_min_force, m_max_force);

			QTimer::singleShot(200, this, [this]()
			{
				SetPadData(m_min_force, m_min_force);
			});
		});
	});

	connect(ui->slider_stick_left, &QSlider::valueChanged, this, [&](int value)
	{
		RepaintPreviewLabel(ui->preview_stick_left, value, ui->anti_deadzone_slider_stick_left->value(), ui->slider_stick_left->size().width(), m_lx, m_ly, ui->squircle_left->value(), ui->stick_multi_left->value());
	});

	connect(ui->slider_stick_right, &QSlider::valueChanged, this, [&](int value)
	{
		RepaintPreviewLabel(ui->preview_stick_right, value, ui->anti_deadzone_slider_stick_right->value(), ui->slider_stick_right->size().width(), m_rx, m_ry, ui->squircle_right->value(), ui->stick_multi_right->value());
	});

	connect(ui->anti_deadzone_slider_stick_left, &QSlider::valueChanged, this, [&](int value)
	{
		RepaintPreviewLabel(ui->preview_stick_left, ui->slider_stick_left->value(), value, ui->slider_stick_left->size().width(), m_lx, m_ly, ui->squircle_left->value(), ui->stick_multi_left->value());
	});

	connect(ui->anti_deadzone_slider_stick_right, &QSlider::valueChanged, this, [&](int value)
	{
		RepaintPreviewLabel(ui->preview_stick_right, ui->slider_stick_right->value(), value, ui->slider_stick_right->size().width(), m_rx, m_ry, ui->squircle_right->value(), ui->stick_multi_right->value());
	});

	// Open LED settings
	connect(ui->b_led_settings, &QPushButton::clicked, this, [this]()
	{
		// Allow LED battery indication while the dialog is open
		ensure(m_handler);
		const cfg_pad& cfg = GetPlayerConfig();
		SetPadData(0, 0, cfg.led_battery_indicator.get());
		pad_led_settings_dialog dialog(this, cfg.colorR, cfg.colorG, cfg.colorB, m_handler->has_rgb(), m_handler->has_player_led(), cfg.player_led_enabled.get(), m_handler->has_battery(), m_handler->has_battery_led(), cfg.led_low_battery_blink.get(), cfg.led_battery_indicator.get(), cfg.led_battery_indicator_brightness);
		connect(&dialog, &pad_led_settings_dialog::pass_led_settings, this, [this](const pad_led_settings_dialog::led_settings& settings)
		{
			ensure(m_handler);
			cfg_pad& cfg = GetPlayerConfig();
			cfg.colorR.set(settings.color_r);
			cfg.colorG.set(settings.color_g);
			cfg.colorB.set(settings.color_b);
			cfg.led_battery_indicator.set(settings.battery_indicator);
			cfg.led_battery_indicator_brightness.set(settings.battery_indicator_brightness);
			cfg.led_low_battery_blink.set(settings.low_battery_blink);
			cfg.player_led_enabled.set(settings.player_led_enabled);
			SetPadData(0, 0, settings.battery_indicator);
		});
		dialog.exec();
		SetPadData(0, 0);
	});

	// Open Motion settings
	connect(ui->b_motion_controls, &QPushButton::clicked, this, [this]()
	{
		if (m_timer_input.isActive())
		{
			m_timer_input.stop();
		}
		if (m_timer_pad_refresh.isActive())
		{
			m_timer_pad_refresh.stop();
		}
		pause_input_thread();

		pad_motion_settings_dialog dialog(this, m_handler, g_cfg_input.player[GetPlayerIndex()]);
		dialog.exec();

		if (ui->chooseDevice->isEnabled() && ui->chooseDevice->currentIndex() >= 0)
		{
			start_input_thread();
			m_timer_input.start(10);
			m_timer_pad_refresh.start(1000);
		}
	});

	// Use timer to display button input
	connect(&m_timer_input, &QTimer::timeout, this, [this]()
	{
		input_callback_data data;
		{
			std::lock_guard lock(m_input_mutex);
			data = m_input_callback_data;
			m_input_callback_data.values.clear();
			m_input_callback_data.has_new_data = false;
		}

		if (!data.has_new_data)
		{
			return;
		}

		const auto update_preview = [this](const std::string& pad_name, bool is_connected, int battery_level, int trigger_left, int trigger_right, int lx, int ly, int rx, int ry, const pad_capabilities& capabilities)
		{
			SwitchPadInfo(pad_name, is_connected);

			if ((!is_connected || !m_remap_timer.isActive()) && (
				is_connected != m_enable_buttons ||
				(is_connected && (
					!capabilities.has_pressure_sensitivity != m_enable_pressure_intensity_button ||
					capabilities.has_rumble != m_enable_rumble ||
					capabilities.has_battery_led != m_enable_battery_led ||
					(capabilities.has_led || capabilities.has_mono_led) != m_enable_led ||
					(capabilities.has_accel || capabilities.has_gyro) != m_enable_motion))))
			{
				if (is_connected)
				{
					m_enable_pressure_intensity_button = !capabilities.has_pressure_sensitivity;
					m_enable_rumble = capabilities.has_rumble;
					m_enable_battery_led = capabilities.has_battery_led;
					m_enable_led = capabilities.has_led || capabilities.has_mono_led;
					m_enable_motion = capabilities.has_accel || capabilities.has_gyro;
				}

				SwitchButtons(is_connected);
			}

			ui->pb_battery->setValue(m_enable_battery ? battery_level : 0);

			if (m_handler->has_deadzones())
			{
				ui->preview_trigger_left->setValue(trigger_left);
				ui->preview_trigger_right->setValue(trigger_right);

				if (m_lx != lx || m_ly != ly)
				{
					m_lx = lx;
					m_ly = ly;
					RepaintPreviewLabel(ui->preview_stick_left, ui->slider_stick_left->value(), ui->anti_deadzone_slider_stick_left->value(), ui->slider_stick_left->size().width(), m_lx, m_ly, ui->squircle_left->value(), ui->stick_multi_left->value());
				}
				if (m_rx != rx || m_ry != ry)
				{
					m_rx = rx;
					m_ry = ry;
					RepaintPreviewLabel(ui->preview_stick_right, ui->slider_stick_right->value(), ui->anti_deadzone_slider_stick_right->value(), ui->slider_stick_right->size().width(), m_rx, m_ry, ui->squircle_right->value(), ui->stick_multi_right->value());
				}
			}
		};

		if (data.status == PadHandlerBase::connection::disconnected)
		{
			// Disable Button Remapping
			update_preview(data.pad_name, false, 0, 0, 0, 0, 0, 0, 0, data.capabilities);
			return;
		}

		// Enable Button Remapping
		update_preview(data.pad_name, true, data.battery_level, data.preview_values[0], data.preview_values[1], data.preview_values[2], data.preview_values[3], data.preview_values[4], data.preview_values[5], data.capabilities);

		// Handle Button Presses
		for (const input_callback_data::input_values& values : data.values)
		{
			if (values.val <= 0) continue;

			cfg_log.notice("get_next_button_press: %s device %s button %s pressed with value %d", m_handler->m_type, data.pad_name, values.button_name, values.val);

			if (m_button_id > button_ids::id_pad_begin && m_button_id < button_ids::id_pad_end && m_button_id == values.button_id)
			{
				m_cfg_entries[m_button_id].insert_key(values.button_name, m_enable_multi_binding);
				ReactivateButtons();
			}
		}
	});

	// Use timer to refresh pad connection status
	connect(&m_timer_pad_refresh, &QTimer::timeout, this, &pad_settings_dialog::RefreshPads);

	// Use thread to get button input
	m_input_thread = std::make_unique<named_thread<std::function<void()>>>("Pad Settings Thread", [this]()
	{
		u32 button_id = button_ids::id_pad_begin; // Used to check if this is the first call during a remap

		while (thread_ctrl::state() != thread_state::aborting)
		{
			thread_ctrl::wait_for(1000);

			if (m_input_thread_state != input_thread_state::active)
			{
				if (m_input_thread_state == input_thread_state::pausing)
				{
					std::lock_guard lock(m_input_mutex);
					m_input_callback_data = {};
					m_input_thread_state = input_thread_state::paused;
				}

				continue;
			}

			std::lock_guard lock(m_handler_mutex);

			const std::vector<std::string> buttons =
			{
				m_cfg_entries[button_ids::id_pad_l2].keys,
				m_cfg_entries[button_ids::id_pad_r2].keys,
				m_cfg_entries[button_ids::id_pad_lstick_left].keys,
				m_cfg_entries[button_ids::id_pad_lstick_right].keys,
				m_cfg_entries[button_ids::id_pad_lstick_down].keys,
				m_cfg_entries[button_ids::id_pad_lstick_up].keys,
				m_cfg_entries[button_ids::id_pad_rstick_left].keys,
				m_cfg_entries[button_ids::id_pad_rstick_right].keys,
				m_cfg_entries[button_ids::id_pad_rstick_down].keys,
				m_cfg_entries[button_ids::id_pad_rstick_up].keys
			};

			// Check if this is the first call during a remap
			const u32 new_button_id = m_button_id;
			const bool is_mapping = new_button_id > button_ids::id_pad_begin && new_button_id < button_ids::id_pad_end;
			const bool first_call = std::exchange(button_id, new_button_id) != button_id && is_mapping;
			const PadHandlerBase::gui_call_type call_type = first_call ? PadHandlerBase::gui_call_type::reset_input : PadHandlerBase::gui_call_type::normal;

			const PadHandlerBase::connection status = m_handler->get_next_button_press(m_device_name,
				[this, button_id](u16 val, std::string button_name, std::string pad_name, u32 battery_level, pad_preview_values preview_values, pad_capabilities capabilities)
				{
					std::lock_guard lock(m_input_mutex);
					if (m_input_callback_data.pad_name != pad_name)
					{
						m_input_callback_data = {};
						m_input_callback_data.pad_name = std::move(pad_name);
					}
					m_input_callback_data.battery_level = battery_level;
					m_input_callback_data.preview_values = std::move(preview_values);
					m_input_callback_data.capabilities = std::move(capabilities);
					m_input_callback_data.has_new_data = true;
					m_input_callback_data.status = PadHandlerBase::connection::connected;
					if (val > 0)
					{
						m_input_callback_data.values.push_back(input_callback_data::input_values
						{
							.button_name = std::move(button_name),
							.button_id = button_id,
							.val = val,
						});
					}
				},
				[this](std::string pad_name)
				{
					std::lock_guard lock(m_input_mutex);
					m_input_callback_data = {};
					m_input_callback_data.pad_name = std::move(pad_name);
					m_input_callback_data.has_new_data = true;
				},
				call_type, buttons);

			if (status == PadHandlerBase::connection::no_data)
			{
				std::lock_guard lock(m_input_mutex);
				if (m_input_callback_data.pad_name != m_device_name)
				{
					m_input_callback_data = {};
					m_input_callback_data.pad_name = m_device_name;
				}
				m_input_callback_data.has_new_data = true;
				m_input_callback_data.status = status;
			}
		}
	});
}

void pad_settings_dialog::RefreshPads()
{
	for (int i = 0; i < ui->chooseDevice->count(); i++)
	{
		pad_device_info info = get_pad_info(ui->chooseDevice, i);

		if (info.name.empty())
		{
			continue;
		}

		std::lock_guard lock(m_handler_mutex);
		const PadHandlerBase::connection status = m_handler->get_next_button_press(info.name, nullptr, nullptr, PadHandlerBase::gui_call_type::get_connection, {});
		switch_pad_info(i, info, status != PadHandlerBase::connection::disconnected);
	}
}

void pad_settings_dialog::SetPadData(u8 large_motor, u8 small_motor, bool led_battery_indicator)
{
	const cfg_pad& cfg = GetPlayerConfig();

	std::lock_guard lock(m_handler_mutex);
	ensure(m_handler);
	m_handler->SetPadData(m_device_name, GetPlayerIndex(), large_motor, small_motor, cfg.colorR, cfg.colorG, cfg.colorB, cfg.player_led_enabled.get(), led_battery_indicator, cfg.led_battery_indicator_brightness);
}

pad_device_info pad_settings_dialog::get_pad_info(QComboBox* combo, int index)
{
	if (!combo || index < 0)
	{
		cfg_log.fatal("get_pad_info: Invalid combo box or index (combo=%d, index=%d)", !!combo, index);
		return {};
	}

	const QVariant user_data = combo->itemData(index);

	if (!user_data.canConvert<pad_device_info>())
	{
		cfg_log.fatal("get_pad_info: Cannot convert itemData for index %d and itemText %s", index, combo->itemText(index));
		return {};
	}

	return user_data.value<pad_device_info>();
}

void pad_settings_dialog::switch_pad_info(int index, pad_device_info info, bool is_connected)
{
	if (index >= 0 && info.is_connected != is_connected)
	{
		info.is_connected = is_connected;

		ui->chooseDevice->setItemData(index, QVariant::fromValue(info));
		ui->chooseDevice->setItemText(index, is_connected ? info.localized_name : (info.localized_name + Disconnected_suffix));
	}

	if (!is_connected && m_remap_timer.isActive() && ui->chooseDevice->currentIndex() == index)
	{
		ReactivateButtons();
	}
}

void pad_settings_dialog::SwitchPadInfo(const std::string& pad_name, bool is_connected)
{
	for (int i = 0; i < ui->chooseDevice->count(); i++)
	{
		if (pad_device_info info = get_pad_info(ui->chooseDevice, i); info.name == pad_name)
		{
			switch_pad_info(i, info, is_connected);
			break;
		}
	}
}

void pad_settings_dialog::ReloadButtons()
{
	m_cfg_entries.clear();

	auto updateButton = [this](int id, QPushButton* button, cfg::string* cfg_text)
	{
		const QString text = QString::fromStdString(*cfg_text);
		m_cfg_entries.insert(std::make_pair(id, pad_button{cfg_text, *cfg_text, text}));
		button->setText(text);
	};

	cfg_pad& cfg = GetPlayerConfig();

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
	updateButton(button_ids::id_analog_limiter, ui->b_analog_limiter, &cfg.analog_limiter_button);
	updateButton(button_ids::id_orientation_reset, ui->b_orientation_reset, &cfg.orientation_reset_button);

	UpdateLabels(true);
}

void pad_settings_dialog::ReactivateButtons()
{
	m_remap_timer.stop();
	m_seconds = MAX_SECONDS;
	m_enable_multi_binding = false;

	if (m_button_id == button_ids::id_pad_begin)
	{
		return;
	}

	if (auto button = m_pad_buttons->button(m_button_id))
	{
		button->setPalette(m_palette);
		button->releaseMouse();
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

	ui->chooseConfig->setFocusPolicy(Qt::WheelFocus);
	ui->chooseHandler->setFocusPolicy(Qt::WheelFocus);
	ui->chooseDevice->setFocusPolicy(Qt::WheelFocus);
	ui->chooseClass->setFocusPolicy(Qt::WheelFocus);
	ui->chooseProduct->setFocusPolicy(Qt::WheelFocus);
}

void pad_settings_dialog::RepaintPreviewLabel(QLabel* label, int deadzone, int anti_deadzone, int desired_width, int x, int y, int squircle, double multiplier) const
{
	desired_width = 100; // Let's keep a fixed size for these labels for now
	const qreal deadzone_max = m_handler ? m_handler->thumb_max : 255; // 255 used as fallback. The deadzone circle shall be small.

	constexpr qreal relative_size = 0.9;
	const qreal device_pixel_ratio = devicePixelRatioF();
	const qreal scaled_width = desired_width * device_pixel_ratio;
	const qreal origin = desired_width / 2.0;
	const qreal outer_circle_diameter = relative_size * desired_width;
	const qreal deadzone_circle_diameter = outer_circle_diameter * deadzone / deadzone_max;
	const qreal anti_deadzone_circle_diameter = outer_circle_diameter * anti_deadzone / deadzone_max;
	const qreal outer_circle_radius = outer_circle_diameter / 2.0;
	const qreal deadzone_circle_radius = deadzone_circle_diameter / 2.0;
	const qreal anti_deadzone_circle_radius = anti_deadzone_circle_diameter / 2.0;
	const qreal stick_x = std::clamp(outer_circle_radius * x * multiplier / deadzone_max, -outer_circle_radius, outer_circle_radius);
	const qreal stick_y = std::clamp(outer_circle_radius * -y * multiplier / deadzone_max, -outer_circle_radius, outer_circle_radius);

	const bool show_emulated_values = ui->chb_show_emulated_values->isChecked();

	qreal ingame_x = 0.0;
	qreal ingame_y = 0.0;

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
			m_handler->convert_stick_values(real_x, real_y, x_in, y_in, deadzone, anti_deadzone, squircle);
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

	painter.setBrush(QBrush(Qt::transparent));

	// Draw a red inner circle that represents the current deadzone
	painter.setPen(QPen(Qt::red, 1.0));
	painter.drawEllipse(QRectF(-deadzone_circle_radius, -deadzone_circle_radius, deadzone_circle_diameter, deadzone_circle_diameter));

	// Draw a green inner circle that represents the current anti-deadzone
	painter.setPen(QPen(Qt::green, 1.0));
	painter.drawEllipse(QRectF(-anti_deadzone_circle_radius, -anti_deadzone_circle_radius, anti_deadzone_circle_diameter, anti_deadzone_circle_diameter));

	// Draw a blue dot that represents the current stick orientation
	painter.setPen(QPen(Qt::blue, 2.0));
	painter.drawEllipse(QRectF(stick_x - 0.5, stick_y - 0.5, 1.0, 1.0));

	if (show_emulated_values)
	{
		// Draw a red dot that represents the current ingame stick orientation
		painter.setPen(QPen(Qt::red, 2.0));
		painter.drawEllipse(QRectF(ingame_x - 0.5, ingame_y - 0.5, 1.0, 1.0));
	}

	painter.end();

	label->setPixmap(pixmap);
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
		cfg_log.error("Pad Settings: Handler Type: %d, Unknown button ID: %d", static_cast<int>(m_handler->m_type), m_button_id.load());
	}
	else
	{
		m_cfg_entries[m_button_id].insert_key(keyboard_pad_handler::GetKeyName(keyEvent, false), m_enable_multi_binding);
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
		cfg_log.error("Pad Settings: Handler Type: %d, Unknown button ID: %d", static_cast<int>(m_handler->m_type), m_button_id.load());
	}
	else
	{
		m_cfg_entries[m_button_id].insert_key((static_cast<keyboard_pad_handler*>(m_handler.get()))->GetMouseName(event), m_enable_multi_binding);
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
		cfg_log.error("Pad Settings: Handler Type: %d, Unknown button ID: %d", static_cast<int>(m_handler->m_type), m_button_id.load());
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

	m_cfg_entries[m_button_id].insert_key((static_cast<keyboard_pad_handler*>(m_handler.get()))->GetMouseName(key), m_enable_multi_binding);
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
		cfg_log.error("Pad Settings: Handler Type: %d, Unknown button ID: %d", static_cast<int>(m_handler->m_type), m_button_id.load());
	}
	else
	{
		constexpr int delta_threshold = 20;
		const QPoint mouse_pos = QCursor::pos();
		const int delta_x = mouse_pos.x() - m_last_pos.x();
		const int delta_y = mouse_pos.y() - m_last_pos.y();

		u32 key = 0;

		if (delta_x > delta_threshold)
		{
			key = mouse::move_right;
		}
		else if (delta_x < -delta_threshold)
		{
			key = mouse::move_left;
		}
		else if (delta_y > delta_threshold)
		{
			key = mouse::move_down;
		}
		else if (delta_y < -delta_threshold)
		{
			key = mouse::move_up;
		}

		if (key != 0)
		{
			m_cfg_entries[m_button_id].insert_key((static_cast<keyboard_pad_handler*>(m_handler.get()))->GetMouseName(key), m_enable_multi_binding);
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
		// On right click clear binding if we are not remapping pad button
		if (m_button_id == button_ids::id_pad_begin)
		{
			QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
			if (const auto button = qobject_cast<QPushButton*>(object); button && button->isEnabled() && mouse_event->button() == Qt::RightButton)
			{
				if (const int button_id = m_pad_buttons->id(button); m_cfg_entries.contains(button_id))
				{
					pad_button& button = m_cfg_entries[button_id];
					button.keys.clear();
					button.text.clear();
					UpdateLabels();

					return true;
				}
			}
		}

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
		const int index = ui->chooseClass->findData(cfg.device_class_type.get());
		ui->chooseClass->setCurrentIndex(index);

		// Trigger the change manually in case that the class dropdown didn't fire an event
		HandleDeviceClassChange(cfg.device_class_type);

		const auto products = input::get_products_by_class(cfg.device_class_type);

		for (usz i = 0; i < products.size(); i++)
		{
			if (products[i].vendor_id == cfg.vendor_id && products[i].product_id == cfg.product_id)
			{
				ui->chooseProduct->setCurrentIndex(static_cast<int>(i));
				break;
			}
		}

		ui->sb_vibration_large->setRange(cfg.multiplier_vibration_motor_large.min, cfg.multiplier_vibration_motor_large.max);
		ui->sb_vibration_large->setValue(cfg.multiplier_vibration_motor_large.get());

		ui->sb_vibration_small->setRange(cfg.multiplier_vibration_motor_small.min, cfg.multiplier_vibration_motor_small.max);
		ui->sb_vibration_small->setValue(cfg.multiplier_vibration_motor_small.get());

		ui->sb_vibration_threshold->setRange(cfg.vibration_threshold.min, cfg.vibration_threshold.max);
		ui->sb_vibration_threshold->setValue(cfg.vibration_threshold.get());

		ui->chb_vibration_switch->setChecked(cfg.switch_vibration_motors.get());

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

		ui->anti_deadzone_slider_stick_left->setRange(0, m_handler->thumb_max);
		ui->anti_deadzone_slider_stick_left->setValue(cfg.lstick_anti_deadzone);

		ui->anti_deadzone_slider_stick_right->setRange(0, m_handler->thumb_max);
		ui->anti_deadzone_slider_stick_right->setValue(cfg.rstick_anti_deadzone);

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

		range = cfg.rstickmultiplier.to_list();
		ui->stick_multi_right->setRange(std::stod(range.front()) / 100.0, std::stod(range.back()) / 100.0);
		ui->stick_multi_right->setValue(cfg.rstickmultiplier / 100.0);

		// Update Squircle Factors
		range = cfg.lpadsquircling.to_list();
		ui->squircle_left->setRange(std::stoi(range.front()), std::stoi(range.back()));
		ui->squircle_left->setValue(cfg.lpadsquircling);

		range = cfg.rpadsquircling.to_list();
		ui->squircle_right->setRange(std::stoi(range.front()), std::stoi(range.back()));
		ui->squircle_right->setValue(cfg.rpadsquircling);

		RepaintPreviewLabel(ui->preview_stick_left, ui->slider_stick_left->value(), ui->anti_deadzone_slider_stick_left->value(), ui->slider_stick_left->size().width(), m_lx, m_ly, cfg.lpadsquircling, cfg.lstickmultiplier / 100.0);
		RepaintPreviewLabel(ui->preview_stick_right, ui->slider_stick_right->value(), ui->anti_deadzone_slider_stick_right->value(), ui->slider_stick_right->size().width(), m_rx, m_ry, cfg.rpadsquircling, cfg.rstickmultiplier / 100.0);

		// Update orientation toggle
		ui->cb_orientation_toggle->setChecked(cfg.orientation_enabled.get());

		// Update analog limiter toggle mode
		ui->cb_analog_limiter_toggle_mode->setChecked(cfg.analog_limiter_toggle_mode.get());

		// Update pressure sensitivity factors
		range = cfg.pressure_intensity.to_list();
		ui->sb_pressure_intensity->setRange(std::stoi(range.front()), std::stoi(range.back()));
		ui->sb_pressure_intensity->setValue(cfg.pressure_intensity);

		// Update pressure sensitivity toggle mode
		ui->cb_pressure_intensity_toggle_mode->setChecked(cfg.pressure_intensity_toggle_mode.get());

		// Update pressure sensitivity deadzone
		range = cfg.pressure_intensity_deadzone.to_list();
		ui->pressure_intensity_deadzone->setRange(std::stoi(range.front()), std::stoi(range.back()));
		ui->pressure_intensity_deadzone->setValue(cfg.pressure_intensity_deadzone.get());

		// Apply stored/default LED settings to the device
		SetPadData(0, 0);

		// Enable battery and LED group box
		ui->gb_battery->setVisible(m_enable_battery || m_enable_led);
	}

	for (auto& [id, button] : m_cfg_entries)
	{
		if (is_reset)
		{
			button.keys = *button.cfg_text;
			button.text = QString::fromStdString(button.keys);
		}

		// The button has to contain at least one character, because it would be square'ish otherwise
		if (auto btn = m_pad_buttons->button(id))
		{
			btn->setText(button.text.isEmpty() ? QStringLiteral("-") : button.text);
		}
	}
}

void pad_settings_dialog::SwitchButtons(bool is_enabled)
{
	m_enable_buttons = is_enabled;

	ui->chb_show_emulated_values->setEnabled(is_enabled);
	ui->stick_multi_left->setEnabled(is_enabled);
	ui->stick_multi_right->setEnabled(is_enabled);
	ui->squircle_left->setEnabled(is_enabled);
	ui->squircle_right->setEnabled(is_enabled);
	ui->gb_pressure_intensity_deadzone->setEnabled(is_enabled);
	ui->gb_pressure_intensity->setEnabled(is_enabled && m_enable_pressure_intensity_button);
	ui->gb_analog_limiter->setEnabled(is_enabled && m_enable_analog_limiter_button);
	ui->gb_orientation_reset->setEnabled(is_enabled && m_enable_orientation_reset_button);
	ui->gb_vibration->setEnabled(is_enabled && m_enable_rumble);
	ui->gb_motion_controls->setEnabled(is_enabled && m_enable_motion);
	ui->gb_stick_deadzones->setEnabled(is_enabled && m_enable_deadzones);
	ui->gb_stick_anti_deadzones->setEnabled(is_enabled && m_enable_deadzones);
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
		if (auto button = m_pad_buttons->button(i))
		{
			button->setEnabled(is_enabled);
		}
	}
}

void pad_settings_dialog::OnPadButtonClicked(int id)
{
	switch (id)
	{
	case button_ids::id_led:
	case button_ids::id_pad_begin:
	case button_ids::id_pad_end:
	case button_ids::id_add_config_file:
	case button_ids::id_remove_config_file:
	case button_ids::id_refresh:
		return;
	case button_ids::id_reset_parameters:
		ReactivateButtons();
		GetPlayerConfig().from_default();
		UpdateLabels(true);
		return;
	case button_ids::id_blacklist:
	{
		std::lock_guard lock(m_handler_mutex);
		[[maybe_unused]] const PadHandlerBase::connection status = m_handler->get_next_button_press(m_device_name, nullptr, nullptr, PadHandlerBase::gui_call_type::blacklist, {});
		return;
	}
	default:
		break;
	}

	// On shift+click or shift+space enable multi key binding
	if (QApplication::keyboardModifiers() & Qt::KeyboardModifier::ShiftModifier)
	{
		m_enable_multi_binding = true;
	}

	// On alt+click or alt+space allow to handle triggers as the entire stick axis
	m_handler->set_trigger_recognition_mode((QApplication::keyboardModifiers() & Qt::KeyboardModifier::AltModifier) ? PadHandlerBase::trigger_recognition_mode::two_directional : PadHandlerBase::trigger_recognition_mode::one_directional);

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

	ui->chooseConfig->setFocusPolicy(Qt::ClickFocus);
	ui->chooseHandler->setFocusPolicy(Qt::ClickFocus);
	ui->chooseDevice->setFocusPolicy(Qt::ClickFocus);
	ui->chooseClass->setFocusPolicy(Qt::ClickFocus);
	ui->chooseProduct->setFocusPolicy(Qt::ClickFocus);

	m_last_pos = QCursor::pos();

	m_button_id = id;
	if (auto button = m_pad_buttons->button(m_button_id))
	{
		button->setText(tr("[ Waiting %1 ]").arg(MAX_SECONDS));
		button->setPalette(QPalette(Qt::blue));
		button->grabMouse();
	}
	SwitchButtons(false); // disable all buttons, needed for using Space, Enter and other specific buttons
	m_remap_timer.start(1000);
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
	cfg_player* player_config = g_cfg_input.player[player];

	std::string handler;
	std::string device;
	std::string buddy_device;

	if (is_ldd_pad)
	{
		handler = fmt::format("%s", pad_handler::null);
	}
	else
	{
		handler = ui->chooseHandler->currentData().toString().toStdString();
		device = player_config->device.to_string();
		buddy_device = player_config->buddy_device.to_string();
	}

	cfg_pad& cfg = player_config->config;

	// Change and get this player's current handler.
	if (auto& cfg_handler = player_config->handler; handler != cfg_handler.to_string())
	{
		if (!cfg_handler.from_string(handler))
		{
			cfg_log.error("Failed to convert input string: %s", handler);
			return;
		}

		// Initialize the new pad config's defaults
		m_handler = pad_thread::GetHandler(player_config->handler);
		pad_thread::InitPadConfig(cfg, cfg_handler, m_handler);
	}
	else
	{
		m_handler = pad_thread::GetHandler(player_config->handler);
	}

	ensure(m_handler);

	// Get the handler's currently available devices.
	const std::vector<pad_list_entry> device_list = m_handler->list_devices();

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
	case pad_handler::keyboard: m_description = tooltips.gamepad_settings.keyboard; break;
	case pad_handler::skateboard: m_description = tooltips.gamepad_settings.skateboard; break;
	case pad_handler::move: m_description = tooltips.gamepad_settings.move; break;
#ifdef _WIN32
	case pad_handler::xinput: m_description = tooltips.gamepad_settings.xinput; break;
	case pad_handler::mm: m_description = tooltips.gamepad_settings.mmjoy; break;
	case pad_handler::ds3: m_description = tooltips.gamepad_settings.ds3_windows; break;
	case pad_handler::ds4: m_description = tooltips.gamepad_settings.ds4_windows; break;
	case pad_handler::dualsense: m_description = tooltips.gamepad_settings.dualsense_windows; break;
#elif __linux__
	case pad_handler::ds3: m_description = tooltips.gamepad_settings.ds3_linux; break;
	case pad_handler::ds4: m_description = tooltips.gamepad_settings.ds4_linux; break;
	case pad_handler::dualsense: m_description = tooltips.gamepad_settings.dualsense_linux; break;
#else
	case pad_handler::ds3: m_description = tooltips.gamepad_settings.ds3_other; break;
	case pad_handler::ds4: m_description = tooltips.gamepad_settings.ds4_other; break;
	case pad_handler::dualsense: m_description = tooltips.gamepad_settings.dualsense_other; break;
#endif
#ifdef HAVE_SDL3
	case pad_handler::sdl: m_description = tooltips.gamepad_settings.sdl; break;
#endif
#ifdef HAVE_LIBEVDEV
	case pad_handler::evdev: m_description = tooltips.gamepad_settings.evdev; break;
#endif
	}
	ui->l_description->setText(m_description);

	// Reset parameters
	m_lx = 0;
	m_ly = 0;
	m_rx = 0;
	m_ry = 0;

	// Enable Capabilities
	m_enable_led = m_handler->has_led();
	m_enable_battery_led = m_handler->has_battery_led();
	m_enable_battery = m_handler->has_battery();
	m_enable_rumble = m_handler->has_rumble();
	m_enable_motion = m_handler->has_motion();
	m_enable_deadzones = m_handler->has_deadzones();
	m_enable_pressure_intensity_button = m_handler->has_pressure_intensity_button();
	m_enable_analog_limiter_button = m_handler->has_analog_limiter_button();
	m_enable_orientation_reset_button = m_handler->has_orientation();

	// Change our contextual widgets
	ui->left_stack->setCurrentIndex((m_handler->m_type == pad_handler::keyboard) ? 1 : 0);
	ui->right_stack->setCurrentIndex((m_handler->m_type == pad_handler::keyboard) ? 1 : 0);
	ui->gb_pressure_intensity->setVisible(m_handler->has_pressure_intensity_button());
	ui->gb_analog_limiter->setVisible(m_handler->has_analog_limiter_button());
	ui->gb_orientation_reset->setVisible(m_handler->has_orientation());

	// Update device dropdown and block signals while doing so
	ui->chooseDevice->blockSignals(true);
	ui->chooseDevice->clear();

	// Refill the device combobox with currently available devices
	switch (m_handler->m_type)
	{
#ifdef _WIN32
	case pad_handler::xinput:
	case pad_handler::mm:
#endif
	case pad_handler::ds3:
	case pad_handler::ds4:
	case pad_handler::dualsense:
	case pad_handler::skateboard:
	case pad_handler::move:
	{
		const QString name_string = QString::fromStdString(m_handler->name_string());
		for (usz i = 1; i <= m_handler->max_devices(); i++) // Controllers 1-n in GUI
		{
			const QString device_name = name_string + QString::number(i);
			const QString device_name_localized = GetLocalizedPadName(m_handler->m_type, device_name, i);
			ui->chooseDevice->addItem(device_name_localized, QVariant::fromValue(pad_device_info{ device_name.toStdString(), device_name_localized, true }));
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
		for (usz i = 0; i < device_list.size(); i++)
		{
			const pad_list_entry& device = ::at32(device_list, i);

			if (!device.is_buddy_only)
			{
				const QString device_name_localized = GetLocalizedPadName(m_handler->m_type, QString::fromStdString(device.name), i);
				const QVariant user_data = QVariant::fromValue(pad_device_info{ device.name, device_name_localized, true });

				ui->chooseDevice->addItem(device_name_localized, user_data);
			}
		}
		break;
	}
	}

	// Re-enable signals for device dropdown
	ui->chooseDevice->blockSignals(false);

	// Handle empty device list
	const bool config_enabled = force_enable || (m_handler->m_type != pad_handler::null && ui->chooseDevice->count() > 0);

	if (config_enabled)
	{
		RefreshPads();

		for (int i = 0; i < ui->chooseDevice->count(); i++)
		{
			if (pad_device_info info = get_pad_info(ui->chooseDevice, i); info.name == device)
			{
				ui->chooseDevice->setCurrentIndex(i);
				break;
			}
		}

		if (ui->chooseDevice->currentIndex() < 0 && ui->chooseDevice->count() > 0)
		{
			ui->chooseDevice->setCurrentIndex(0);
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

		// Keep the configured device name
		m_device_name = GetDeviceName();
	}

	// Handle running timers
	if (m_remap_timer.isActive())
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
		m_timer_input.start(10);
		m_timer_pad_refresh.start(1000);
	}
}

void pad_settings_dialog::ChangeConfig(const QString& config_file)
{
	if (config_file.isEmpty())
		return;

	m_config_file = config_file.toStdString();

	ui->b_remConfig->setEnabled(m_title_id.empty() && m_config_file != g_cfg_input_configs.default_config);

	// Load in order to get the pad handlers
	if (!g_cfg_input.load(m_title_id, m_config_file, true))
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
	if (!g_cfg_input.load(m_title_id, m_config_file, true))
	{
		cfg_log.notice("Reloaded empty pad config");
	}

	const u32 player_id = GetPlayerIndex();
	const QString q_handler = QString::fromStdString(g_cfg_input.player[player_id]->handler.to_string());

	if (const int index = ui->chooseHandler->findData(q_handler); index >= 0)
	{
		ui->chooseHandler->setCurrentIndex(index);
	}
	else
	{
		cfg_log.error("Handler '%s' not found in handler dropdown.", q_handler);
	}

	// Force Refresh
	ChangeHandler();
}

void pad_settings_dialog::ChangeDevice(int index)
{
	if (index < 0)
		return;

	const QVariant user_data = ui->chooseDevice->itemData(index);

	if (!user_data.canConvert<pad_device_info>())
	{
		cfg_log.fatal("ChangeDevice: Cannot convert itemData for index %d and itemText %s", index, ui->chooseDevice->itemText(index));
		return;
	}

	const pad_device_info info = user_data.value<pad_device_info>();
	SetDeviceName(info.name);
}

void pad_settings_dialog::HandleDeviceClassChange(u32 class_id) const
{
	ui->chooseProduct->clear();

	for (const input::product_info& product : input::get_products_by_class(class_id))
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
		case input::product_type::ps_move_navigation:
		{
			ui->chooseProduct->addItem(tr("PS Move Navigation", "PS Move Navigation Controller"), static_cast<int>(product.type));
			break;
		}
		case input::product_type::ride_skateboard:
		{
			ui->chooseProduct->addItem(tr("RIDE Skateboard", "Tony Hawk RIDE Skateboard Controller"), static_cast<int>(product.type));
			break;
		}
		case input::product_type::guncon_3:
		{
			ui->chooseProduct->addItem(tr("GunCon 3", "GunCon 3 Controller"), static_cast<int>(product.type));
			break;
		}
		case input::product_type::top_shot_elite:
		{
			ui->chooseProduct->addItem(tr("Top Shot Elite", "Top Shot Elite Controller"), static_cast<int>(product.type));
			break;
		}
		case input::product_type::top_shot_fearmaster:
		{
			ui->chooseProduct->addItem(tr("Top Shot Fearmaster", "Top Shot Fearmaster Controller"), static_cast<int>(product.type));
			break;
		}
		case input::product_type::udraw_gametablet:
		{
			ui->chooseProduct->addItem(tr("uDraw GameTablet", "uDraw GameTablet Controller"), static_cast<int>(product.type));
			break;
		}
		}
	}
}

void pad_settings_dialog::AddConfigFile()
{
	QInputDialog* dialog = new QInputDialog(this);
	dialog->setWindowTitle(tr("Choose a unique name"));
	dialog->setLabelText(tr("Configuration Name: "));
	dialog->setFixedSize(500, 100);

	while (dialog->exec() != QDialog::Rejected)
	{
		const QString config_name = dialog->textValue();

		if (config_name.isEmpty())
		{
			QMessageBox::warning(this, tr("Error"), tr("Name cannot be empty"));
			continue;
		}
		if (config_name.contains("."))
		{
			QMessageBox::warning(this, tr("Error"), tr("Must choose a name without '.'"));
			continue;
		}
		if (ui->chooseConfig->findText(config_name) != -1)
		{
			QMessageBox::warning(this, tr("Error"), tr("Please choose a non-existing name"));
			continue;
		}
		if (CreateConfigFile(QString::fromStdString(rpcs3::utils::get_input_config_dir(m_title_id)), config_name))
		{
			ui->chooseConfig->addItem(config_name);
			ui->chooseConfig->setCurrentText(config_name);
		}
		break;
	}
}

void pad_settings_dialog::RemoveConfigFile()
{
	const std::string config_to_remove = m_config_file;
	const QString q_config_to_remove = QString::fromStdString(config_to_remove);

	if (config_to_remove == g_cfg_input_configs.default_config)
	{
		QMessageBox::warning(this, tr("Warning!"), tr("Can't remove default configuration '%0'.").arg(q_config_to_remove));
		return;
	}

	if (QMessageBox::question(this, tr("Remove Configuration?"), tr("Do you really want to remove the configuration '%0'?").arg(q_config_to_remove)) != QMessageBox::StandardButton::Yes)
	{
		return;
	}

	const std::string filepath = fmt::format("%s%s.yml", rpcs3::utils::get_input_config_dir(m_title_id), config_to_remove);

	if (!fs::remove_file(filepath))
	{
		QMessageBox::warning(this, tr("Warning!"), tr("Failed to remove '%0'.").arg(QString::fromStdString(filepath)));
		return;
	}

	const auto [config_files, active_config_file] = get_config_files();

	ui->chooseConfig->setCurrentText(active_config_file);
	ui->chooseConfig->removeItem(ui->chooseConfig->findText(q_config_to_remove));

	// Save new config if we removed the currently saved config
	if (active_config_file == q_config_to_remove)
	{
		save(false);
	}

	QMessageBox::information(this, tr("Removed Configuration"), tr("Removed configuration '%0'.\nThe selected configuration is now '%1'.").arg(q_config_to_remove).arg(active_config_file));
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
			const QString item_data = QString::fromStdString(str_inputs[i]);
			ui->chooseHandler->addItem(GetLocalizedPadHandler(item_data, static_cast<pad_handler>(i)), QVariant(item_data));
		}

		const QString item_data = QString::fromStdString(g_cfg_input.player[player_id]->handler.to_string());
		ui->chooseHandler->setCurrentIndex(ui->chooseHandler->findData(QVariant(item_data)));
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

	m_duplicate_buttons[m_last_player_id].clear();

	auto& player = g_cfg_input.player[m_last_player_id];
	m_last_player_id = new_player_id;

	// Check for duplicate button choices
	if (m_handler->m_type != pad_handler::null)
	{
		std::set<std::string> unique_keys;
		for (const auto& [id, button] : m_cfg_entries)
		{
			// Let's ignore special keys, unless we're using a keyboard
			if (m_handler->m_type != pad_handler::keyboard &&
				(id == button_ids::id_pressure_intensity || id == button_ids::id_analog_limiter || id == button_ids::id_orientation_reset))
			{
				continue;
			}

			for (const std::string& key : cfg_pad::get_buttons(button.keys))
			{
				if (const auto& [it, ok] = unique_keys.insert(key); !ok)
				{
					m_duplicate_buttons[m_last_player_id] = key;
					break;
				}
			}
		}
	}

	// Apply buttons
	for (const auto& entry : m_cfg_entries)
	{
		entry.second.cfg_text->from_string(entry.second.keys);
	}

	// Apply rest of config
	auto& cfg = player->config;

	cfg.lstickmultiplier.set(ui->stick_multi_left->value() * 100);
	cfg.rstickmultiplier.set(ui->stick_multi_right->value() * 100);

	cfg.lpadsquircling.set(ui->squircle_left->value());
	cfg.rpadsquircling.set(ui->squircle_right->value());

	if (m_handler->has_rumble())
	{
		cfg.multiplier_vibration_motor_large.set(ui->sb_vibration_large->value());
		cfg.multiplier_vibration_motor_small.set(ui->sb_vibration_small->value());
		cfg.vibration_threshold.set(ui->sb_vibration_threshold->value());
		cfg.switch_vibration_motors.set(ui->chb_vibration_switch->isChecked());
	}

	if (m_handler->has_deadzones())
	{
		cfg.ltriggerthreshold.set(ui->slider_trigger_left->value());
		cfg.rtriggerthreshold.set(ui->slider_trigger_right->value());
		cfg.lstickdeadzone.set(ui->slider_stick_left->value());
		cfg.rstickdeadzone.set(ui->slider_stick_right->value());
		cfg.lstick_anti_deadzone.set(ui->anti_deadzone_slider_stick_left->value());
		cfg.rstick_anti_deadzone.set(ui->anti_deadzone_slider_stick_right->value());
	}

	if (m_handler->has_analog_limiter_button())
	{
		cfg.analog_limiter_toggle_mode.set(ui->cb_analog_limiter_toggle_mode->isChecked());
	}

	if (m_handler->has_pressure_intensity_button())
	{
		cfg.pressure_intensity.set(ui->sb_pressure_intensity->value());
		cfg.pressure_intensity_toggle_mode.set(ui->cb_pressure_intensity_toggle_mode->isChecked());
	}

	if (m_handler->has_orientation())
	{
		cfg.orientation_enabled.set(ui->cb_orientation_toggle->isChecked());
	}

	cfg.pressure_intensity_deadzone.set(ui->pressure_intensity_deadzone->value());

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

	cfg.device_class_type.set(ui->chooseClass->currentData().toUInt());

	const auto info = input::get_product_info(static_cast<input::product_type>(ui->chooseProduct->currentData().toInt()));

	cfg.vendor_id.set(info.vendor_id);
	cfg.product_id.set(info.product_id);
}

void pad_settings_dialog::save(bool check_duplicates)
{
	ApplyCurrentPlayerConfig(m_last_player_id);

	if (check_duplicates)
	{
		for (const auto& [player_id, key] : m_duplicate_buttons)
		{
			if (!key.empty())
			{
				int result = QMessageBox::Yes;
				m_gui_settings->ShowConfirmationBox(
					tr("Warning!"),
					tr("The %0 button <b>%1</b> of <b>Player %2</b> was assigned at least twice.<br>Please consider adjusting the configuration.<br><br>Continue anyway?<br>")
						.arg(QString::fromStdString(g_cfg_input.player[player_id]->handler.to_string()))
						.arg(QString::fromStdString(key))
						.arg(player_id + 1),
					gui::ib_same_buttons, &result, this);

				if (result == QMessageBox::No)
					return;

				break;
			}
		}
	}

	const std::string config_file_key = m_title_id.empty() ? g_cfg_input_configs.global_key : m_title_id;

	g_cfg_input_configs.active_configs.set_value(config_file_key, m_config_file);
	g_cfg_input_configs.save();

	g_cfg_input.save(m_title_id, m_config_file);
}

void pad_settings_dialog::SaveExit()
{
	save(true);

	QDialog::accept();
}

void pad_settings_dialog::CancelExit()
{
	// Reloads configs from file or defaults
	g_cfg_input_configs.load();
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
		case pad_handler::skateboard: return tr("Skateboard");
		case pad_handler::move: return tr("PS Move");
#ifdef _WIN32
		case pad_handler::xinput: return tr("XInput");
		case pad_handler::mm: return tr("MMJoystick");
#endif
#ifdef HAVE_SDL3
		case pad_handler::sdl: return tr("SDL");
#endif
#ifdef HAVE_LIBEVDEV
		case pad_handler::evdev: return tr("Evdev");
#endif
	}
	return original;
}

QString pad_settings_dialog::GetLocalizedPadName(pad_handler handler, const QString& original, usz index)
{
	switch (handler)
	{
		case pad_handler::null: return tr("Default Null Device");
		case pad_handler::keyboard: return tr("Keyboard");
		case pad_handler::ds3: return tr("DS3 Pad #%0").arg(index);
		case pad_handler::ds4: return tr("DS4 Pad #%0").arg(index);
		case pad_handler::dualsense: return tr("DualSense Pad #%0").arg(index);
		case pad_handler::skateboard: return tr("Skateboard #%0").arg(index);
		case pad_handler::move: return tr("PS Move #%0").arg(index);
#ifdef _WIN32
		case pad_handler::xinput: return tr("XInput Pad #%0").arg(index);
		case pad_handler::mm: return tr("Joystick #%0").arg(index);
#endif
#ifdef HAVE_SDL3
		case pad_handler::sdl: break; // Localization not feasible. Names differ for each device.
#endif
#ifdef HAVE_LIBEVDEV
		case pad_handler::evdev: break; // Localization not feasible. Names differ for each device.
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
		if (const auto handler = pad::get_pad_thread(true))
		{
			ensure(index < handler->GetPads().size());

			if (const std::shared_ptr<Pad> pad = ::at32(handler->GetPads(), index))
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

std::string pad_settings_dialog::GetDeviceName() const
{
	return g_cfg_input.player[GetPlayerIndex()]->device.to_string();
}

void pad_settings_dialog::SetDeviceName(const std::string& name)
{
	m_device_name = name;

	if (!g_cfg_input.player[GetPlayerIndex()]->device.from_string(m_device_name))
	{
		cfg_log.error("Failed to convert device string: %s", m_device_name);
	}
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
	ensure(!!object);
	m_descriptions[object] = tooltip;
	object->installEventFilter(this);
}

void pad_settings_dialog::SubscribeTooltips()
{
	// Localized tooltips
	const Tooltips tooltips;

	SubscribeTooltip(ui->gb_orientation_reset, tooltips.gamepad_settings.orientation_reset);
	SubscribeTooltip(ui->gb_analog_limiter, tooltips.gamepad_settings.analog_limiter);
	SubscribeTooltip(ui->gb_pressure_intensity, tooltips.gamepad_settings.pressure_intensity);
	SubscribeTooltip(ui->gb_pressure_intensity_deadzone, tooltips.gamepad_settings.pressure_deadzone);
	SubscribeTooltip(ui->gb_squircle, tooltips.gamepad_settings.squircle_factor);
	SubscribeTooltip(ui->gb_stick_multi, tooltips.gamepad_settings.stick_multiplier);
	SubscribeTooltip(ui->gb_vibration, tooltips.gamepad_settings.vibration);
	SubscribeTooltip(ui->gb_motion_controls, tooltips.gamepad_settings.motion_controls);
	SubscribeTooltip(ui->gb_stick_deadzones, tooltips.gamepad_settings.stick_deadzones);
	SubscribeTooltip(ui->gb_stick_anti_deadzones, tooltips.gamepad_settings.stick_deadzones);
	SubscribeTooltip(ui->gb_stick_preview, tooltips.gamepad_settings.emulated_preview);
	SubscribeTooltip(ui->gb_triggers, tooltips.gamepad_settings.trigger_deadzones);
	SubscribeTooltip(ui->gb_stick_lerp, tooltips.gamepad_settings.stick_lerp);
	SubscribeTooltip(ui->gb_mouse_accel, tooltips.gamepad_settings.mouse_acceleration);
	SubscribeTooltip(ui->gb_mouse_dz, tooltips.gamepad_settings.mouse_deadzones);
	SubscribeTooltip(ui->gb_mouse_movement, tooltips.gamepad_settings.mouse_movement);
	
	for (int i = button_ids::id_pad_begin + 1; i < button_ids::id_pad_end; i++)
	{
		SubscribeTooltip(m_pad_buttons->button(i), tooltips.gamepad_settings.button_assignment);
	}
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
