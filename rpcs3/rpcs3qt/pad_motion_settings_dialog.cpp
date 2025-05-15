#include "stdafx.h"
#include "pad_motion_settings_dialog.h"

#include <QComboBox>
#include <thread>

LOG_CHANNEL(cfg_log, "CFG");

pad_motion_settings_dialog::pad_motion_settings_dialog(QDialog* parent, std::shared_ptr<PadHandlerBase> handler, cfg_player* cfg)
	: QDialog(parent)
	, ui(new Ui::pad_motion_settings_dialog)
	, m_handler(handler)
	, m_cfg(cfg)
{
	ui->setupUi(this);
	setModal(true);

	ensure(m_handler);
	ensure(m_cfg);

	cfg_pad& pad = m_cfg->config;

	m_preview_sliders = {{ ui->slider_x, ui->slider_y, ui->slider_z, ui->slider_g }};
	m_preview_labels = {{ ui->label_x, ui->label_y, ui->label_z, ui->label_g }};
	m_axis_names = {{ ui->combo_x, ui->combo_y, ui->combo_z, ui->combo_g }};
	m_mirrors = {{ ui->mirror_x, ui->mirror_y, ui->mirror_z, ui->mirror_g }};
	m_shifts = {{ ui->shift_x, ui->shift_y, ui->shift_z, ui->shift_g }};
	m_config_entries = {{ &pad.motion_sensor_x, &pad.motion_sensor_y, &pad.motion_sensor_z, &pad.motion_sensor_g }};

	for (usz i = 0; i < m_preview_sliders.size(); i++)
	{
		m_preview_sliders[i]->setRange(0, 1023);
		m_preview_labels[i]->setText("0");
	}

#if HAVE_LIBEVDEV
	const bool has_device_list = m_handler->m_type == pad_handler::evdev;
#else
	const bool has_device_list = false;
#endif

	if (has_device_list)
	{
		// Combobox: Motion Devices
		m_device_name = m_cfg->buddy_device.to_string();

		ui->cb_choose_device->addItem(tr("Disabled"), QVariant::fromValue(pad_device_info{}));

		const std::vector<pad_list_entry> device_list = m_handler->list_devices();
		for (const pad_list_entry& device : device_list)
		{
			if (device.is_buddy_only)
			{
				const QString device_name = QString::fromStdString(device.name);
				const QVariant user_data = QVariant::fromValue(pad_device_info{ device.name, device_name, true });

				ui->cb_choose_device->addItem(device_name, user_data);
			}
		}

		for (int i = 0; i < ui->cb_choose_device->count(); i++)
		{
			const QVariant user_data = ui->cb_choose_device->itemData(i);
			ensure(user_data.canConvert<pad_device_info>());

			if (const pad_device_info info = user_data.value<pad_device_info>(); info.name == m_device_name)
			{
				ui->cb_choose_device->setCurrentIndex(i);
				break;
			}
		}

		connect(ui->cb_choose_device, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &pad_motion_settings_dialog::change_device);

		// Combobox: Configure Axis
		m_motion_axis_list = m_handler->get_motion_axis_list();

		for (const auto& [code, axis] : m_motion_axis_list)
		{
			const QString q_axis = QString::fromStdString(axis);

			for (usz i = 0; i < m_axis_names.size(); i++)
			{
				m_axis_names[i]->addItem(q_axis, code);

				if (m_config_entries[i]->axis.to_string() == axis)
				{
					m_axis_names[i]->setCurrentIndex(m_axis_names[i]->findData(code));
				}
			}
		}

		for (usz i = 0; i < m_axis_names.size(); i++)
		{
			const cfg_sensor* config = m_config_entries[i];

			m_mirrors[i]->setChecked(config->mirrored.get());

			m_shifts[i]->setRange(config->shift.min, config->shift.max);
			m_shifts[i]->setValue(config->shift.get());

			connect(m_mirrors[i], &QCheckBox::checkStateChanged, this, [this, i](Qt::CheckState state)
			{
				std::lock_guard lock(m_config_mutex);
				m_config_entries[i]->mirrored.set(state != Qt::Unchecked);
			});

			connect(m_shifts[i], QOverload<int>::of(&QSpinBox::valueChanged), this, [this, i](int value)
			{
				std::lock_guard lock(m_config_mutex);
				m_config_entries[i]->shift.set(value);
			});

			connect(m_axis_names[i], QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, i](int index)
			{
				std::lock_guard lock(m_config_mutex);
				if (!m_config_entries[i]->axis.from_string(m_axis_names[i]->itemText(index).toStdString()))
				{
					cfg_log.error("Failed to convert motion axis string: %s", m_axis_names[i]->itemData(index).toString());
				}
			});
		}
	}
	else
	{
		m_device_name = m_cfg->device.to_string();
		ui->gb_device->setVisible(false);
		ui->cancelButton->setVisible(false);
		for (usz i = 0; i < m_axis_names.size(); i++)
		{
			m_axis_names[i]->setVisible(false);
			m_mirrors[i]->setVisible(false);
			m_shifts[i]->setVisible(false);
		}
	}

	// Use timer to display button input
	connect(&m_timer_input, &QTimer::timeout, this, [this]()
	{
		motion_callback_data data;
		{
			std::lock_guard lock(m_input_mutex);
			data = m_motion_callback_data;
			m_motion_callback_data.has_new_data = false;
		}

		if (data.has_new_data)
		{
			// Starting with 1 because the first entry is the Disabled entry.
			for (int i = 1; i < ui->cb_choose_device->count(); i++)
			{
				const QVariant user_data = ui->cb_choose_device->itemData(i);
				ensure(user_data.canConvert<pad_device_info>());

				if (const pad_device_info info = user_data.value<pad_device_info>(); info.name == data.pad_name)
				{
					switch_buddy_pad_info(i, info, data.success);
					break;
				}
			}

			for (usz i = 0; i < data.preview_values.size(); i++)
			{
				m_preview_sliders[i]->setValue(data.preview_values[i]);
				m_preview_labels[i]->setText(QString::number(data.preview_values[i]));
			}
		}
	});
	m_timer_input.start(10);

	// Use thread to get button input
	m_input_thread = std::make_unique<named_thread<std::function<void()>>>("UI Pad Motion Thread", [this]()
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

			std::array<AnalogSensor, 4> sensors{};
			{
				std::lock_guard lock(m_config_mutex);
				for (usz i = 0; i < sensors.size(); i++)
				{
					AnalogSensor& sensor = sensors[i];
					const cfg_sensor* config = m_config_entries[i];
					const std::string cfgname = config->axis.to_string();
					for (const auto& [code, name] : m_motion_axis_list)
					{
						if (cfgname == name)
						{
							sensor.m_keyCode = code;
							sensor.m_mirrored = config->mirrored.get();
							sensor.m_shift = config->shift.get();
							break;
						}
					}
				}
			}

			m_handler->get_motion_sensors(m_device_name,
				[this](std::string pad_name, motion_preview_values preview_values)
				{
					std::lock_guard lock(m_input_mutex);
					m_motion_callback_data.pad_name = std::move(pad_name);
					m_motion_callback_data.preview_values = std::move(preview_values);
					m_motion_callback_data.has_new_data = true;
					m_motion_callback_data.success = true;
				},
				[this](std::string pad_name, motion_preview_values preview_values)
				{
					std::lock_guard lock(m_input_mutex);
					m_motion_callback_data.pad_name = std::move(pad_name);
					m_motion_callback_data.preview_values = std::move(preview_values);
					m_motion_callback_data.has_new_data = true;
					m_motion_callback_data.success = false;
				},
				m_motion_callback_data.preview_values, sensors);
		}
	});
	start_input_thread();
}

pad_motion_settings_dialog::~pad_motion_settings_dialog()
{
	// Join thread
	m_input_thread_state = input_thread_state::pausing;
	m_input_thread.reset();
}

void pad_motion_settings_dialog::change_device(int index)
{
	if (index < 0)
		return;

	const QVariant user_data = ui->cb_choose_device->itemData(index);
	ensure(user_data.canConvert<pad_device_info>());

	const pad_device_info info = user_data.value<pad_device_info>();

	if (!m_cfg->buddy_device.from_string(info.name))
	{
		cfg_log.error("Failed to convert motion device string: %s", info.name);
	}

	m_device_name = m_cfg->buddy_device.to_string();
}

void pad_motion_settings_dialog::switch_buddy_pad_info(int index, pad_device_info info, bool is_connected)
{
	if (index >= 0 && info.is_connected != is_connected)
	{
		info.is_connected = is_connected;

		ui->cb_choose_device->setItemData(index, QVariant::fromValue(info));
		ui->cb_choose_device->setItemText(index, is_connected ? QString::fromStdString(info.name) : (QString::fromStdString(info.name) + Disconnected_suffix));
	}
}

void pad_motion_settings_dialog::start_input_thread()
{
	m_input_thread_state = input_thread_state::active;
}

void pad_motion_settings_dialog::pause_input_thread()
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
