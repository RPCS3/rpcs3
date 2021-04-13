#include "emu_settings.h"
#include "config_adapter.h"

#include <QMessageBox>
#include <QLineEdit>
#include <QTimer>
#include <QCalendarWidget>

#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/Cell/Modules/cellSysutil.h"

#include "util/yaml.hpp"
#include "Utilities/File.h"
#include "Utilities/Config.h"

LOG_CHANNEL(cfg_log, "CFG");

extern std::string g_cfg_defaults; //! Default settings grabbed from Utilities/Config.h

inline std::string sstr(const QString& _in) { return _in.toStdString(); }
inline std::string sstr(const QVariant& _in) { return sstr(_in.toString()); }

// Emit sorted YAML
namespace
{
	static NEVER_INLINE void emit_data(YAML::Emitter& out, const YAML::Node& node)
	{
		// TODO
		out << node;
	}

	// Incrementally load YAML
	static NEVER_INLINE void operator +=(YAML::Node& left, const YAML::Node& node)
	{
		if (node && !node.IsNull())
		{
			if (node.IsMap())
			{
				for (const auto& pair : node)
				{
					if (pair.first.IsScalar())
					{
						auto&& lhs = left[pair.first.Scalar()];
						lhs += pair.second;
					}
					else
					{
						// Exotic case (TODO: probably doesn't work)
						auto&& lhs = left[YAML::Clone(pair.first)];
						lhs += pair.second;
					}
				}
			}
			else if (node.IsScalar() || node.IsSequence())
			{
				// Scalars and sequences are replaced completely, but this may change in future.
				// This logic may be overwritten by custom demands of every specific cfg:: node.
				left = node;
			}
		}
	}
}

emu_settings::emu_settings()
	: QObject()
{
}

bool emu_settings::Init()
{
	m_render_creator = new render_creator(this);

	if (!m_render_creator)
	{
		fmt::throw_exception("emu_settings::emu_settings() render_creator is null");
	}

	if (m_render_creator->abort_requested)
	{
		return false;
	}

	// Make Vulkan default setting if it is supported
	if (m_render_creator->Vulkan.supported && !m_render_creator->Vulkan.adapters.empty())
	{
		const std::string adapter = sstr(m_render_creator->Vulkan.adapters.at(0));
		cfg_log.notice("Setting the default renderer to Vulkan. Default GPU: '%s')", adapter);
		Emu.SetDefaultRenderer(video_renderer::vulkan);
		Emu.SetDefaultGraphicsAdapter(adapter);
	}

	return true;
}

void emu_settings::LoadSettings(const std::string& title_id)
{
	m_title_id = title_id;

	// Create config path if necessary
	fs::create_path(title_id.empty() ? fs::get_config_dir() : Emulator::GetCustomConfigDir());

	// Load default config
	auto [default_config, default_error] = yaml_load(g_cfg_defaults);

	if (default_error.empty())
	{
		m_default_settings = default_config;
		m_current_settings = YAML::Clone(default_config);
	}
	else
	{
		cfg_log.fatal("Failed to load default config:\n%s", default_error);
		QMessageBox::critical(nullptr, tr("Config Error"), tr("Failed to load default config:\n%0")
			.arg(QString::fromStdString(default_error)), QMessageBox::Ok);
	}

	// Add global config
	const std::string global_config_path = fs::get_config_dir() + "config.yml";
	fs::file config(global_config_path, fs::read + fs::write + fs::create);
	auto [global_config, global_error] = yaml_load(config.to_string());
	config.close();

	if (global_error.empty())
	{
		m_current_settings += global_config;
	}
	else
	{
		cfg_log.fatal("Failed to load global config %s:\n%s", global_config_path, global_error);
		QMessageBox::critical(nullptr, tr("Config Error"), tr("Failed to load global config:\nFile: %0\nError: %1")
			.arg(QString::fromStdString(global_config_path)).arg(QString::fromStdString(global_error)), QMessageBox::Ok);
	}

	// Add game config
	if (!title_id.empty())
	{
		// Remove obsolete settings of the global config before adding the custom settings.
		// Otherwise we'll always trigger the "obsolete settings dialog" when editing custom configs.
		ValidateSettings(true);

		const std::string config_path_new = Emulator::GetCustomConfigPath(m_title_id);
		const std::string config_path_old = Emulator::GetCustomConfigPath(m_title_id, true);
		std::string custom_config_path;

		if (fs::is_file(config_path_new))
		{
			custom_config_path = config_path_new;
		}
		else if (fs::is_file(config_path_old))
		{
			custom_config_path = config_path_old;
		}

		if (!custom_config_path.empty())
		{
			if ((config = fs::file(custom_config_path, fs::read + fs::write)))
			{
				auto [custom_config, custom_error] = yaml_load(config.to_string());
				config.close();

				if (custom_error.empty())
				{
					m_current_settings += custom_config;
				}
				else
				{
					cfg_log.fatal("Failed to load custom config %s:\n%s", custom_config_path, custom_error);
					QMessageBox::critical(nullptr, tr("Config Error"), tr("Failed to load custom config:\nFile: %0\nError: %1")
						.arg(QString::fromStdString(custom_config_path)).arg(QString::fromStdString(custom_error)), QMessageBox::Ok);
				}
			}
		}
	}
}

bool emu_settings::ValidateSettings(bool cleanup)
{
	bool is_clean = true;

	std::function<void(int, YAML::Node&, std::vector<std::string>&, cfg::_base*)> search_level;
	search_level = [&search_level, &is_clean, &cleanup, this](int level, YAML::Node& yml_node, std::vector<std::string>& keys, cfg::_base* cfg_base)
	{
		if (!yml_node || !yml_node.IsMap())
		{
			return;
		}

		const int next_level = level + 1;

		for (const auto& yml_entry : yml_node)
		{
			const std::string key = yml_entry.first.Scalar();
			cfg::_base* cfg_node = nullptr;

			keys.resize(next_level);
			keys[level] = key;

			if (cfg_base && cfg_base->get_type() == cfg::type::node)
			{
				for (const auto& node : static_cast<const cfg::node*>(cfg_base)->get_nodes())
				{
					if (node->get_name() == keys[level])
					{
						cfg_node = node;
						break;
					}
				}
			}

			if (cfg_node)
			{
				YAML::Node next_node = yml_node[key];
				search_level(next_level, next_node, keys, cfg_node);
			}
			else
			{
				const auto get_full_key = [&keys](const std::string& seperator) -> std::string
				{
					std::string full_key;
					for (usz i = 0; i < keys.size(); i++)
					{
						full_key += keys[i];
						if (i < keys.size() - 1) full_key += seperator;
					}
					return full_key;
				};

				is_clean = false;

				if (cleanup)
				{
					if (!yml_node.remove(key))
					{
						cfg_log.error("Could not remove config entry: %s", get_full_key(": "));
						is_clean = true; // abort
						return;
					}

					// Let's only remove one entry at a time. I got some weird issues when doing all at once.
					return;
				}
				else
				{
					cfg_log.warning("Unknown config entry found: %s", get_full_key(": "));
				}
			}
		}
	};

	cfg_root root;
	std::vector<std::string> keys;

	do
	{
		is_clean = true;
		search_level(0, m_current_settings, keys, &root);
	}
	while (cleanup && !is_clean);

	return is_clean;
}

void emu_settings::SaveSettings() const
{
	YAML::Emitter out;
	emit_data(out, m_current_settings);

	std::string config_name;

	if (m_title_id.empty())
	{
		config_name = fs::get_config_dir() + "/config.yml";
	}
	else
	{
		config_name = Emulator::GetCustomConfigPath(m_title_id);
	}

	// Save config atomically
	fs::pending_file temp(config_name);
	temp.file.write(out.c_str(), out.size());
	temp.commit();

	// Check if the running config/title is the same as the edited config/title.
	if (config_name == g_cfg.name || m_title_id == Emu.GetTitleID())
	{
		// Update current config
		if (!g_cfg.from_string({out.c_str(), out.size()}, !Emu.IsStopped()))
		{
			cfg_log.fatal("Failed to update configuration");
		}
		else if (!Emu.IsStopped()) // Don't spam the log while emulation is stopped. The config will be logged on boot anyway.
		{
			cfg_log.notice("Updated configuration:\n%s\n", g_cfg.to_string());
		}
	}
}

void emu_settings::EnhanceComboBox(QComboBox* combobox, emu_settings_type type, bool is_ranged, bool use_max, int max, bool sorted)
{
	if (!combobox)
	{
		cfg_log.fatal("EnhanceComboBox '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	if (is_ranged)
	{
		if (sorted)
		{
			cfg_log.warning("EnhanceCombobox '%s': ignoring sorting request on ranged combo box", cfg_adapter::get_setting_name(type));
		}

		const QStringList range = GetSettingOptions(type);

		const int max_item = use_max ? max : range.last().toInt();

		for (int i = range.first().toInt(); i <= max_item; i++)
		{
			combobox->addItem(QString::number(i), i);
		}
	}
	else
	{
		const QStringList settings = GetSettingOptions(type);

		for (int i = 0; i < settings.count(); i++)
		{
			const QString localized_setting = GetLocalizedSetting(settings[i], type, combobox->count());
			combobox->addItem(localized_setting, QVariant({settings[i], i}));
		}

		if (sorted)
		{
			combobox->model()->sort(0, Qt::AscendingOrder);
		}
	}

	// Since the QComboBox has localised strings, we can't just findText / findData, so we need to manually iterate through it to find our index
	const auto find_index = [&](const QString& value)
	{
		for (int i = 0; i < combobox->count(); i++)
		{
			const QVariantList var_list = combobox->itemData(i).toList();
			ensure(var_list.size() == 2 && var_list[0].canConvert<QString>());

			if (value == var_list[0].toString())
			{
				return i;
			}
		}
		return -1;
	};

	const std::string selected = GetSetting(type);
	const QString selected_q = qstr(selected);
	int index;

	if (is_ranged)
	{
		index = combobox->findData(selected_q);
	}
	else
	{
		index = find_index(selected_q);
	}

	if (index == -1)
	{
		const std::string def = GetSettingDefault(type);
		cfg_log.error("EnhanceComboBox '%s' tried to set an invalid value: %s. Setting to default: %s", cfg_adapter::get_setting_name(type), selected, def);

		if (is_ranged)
		{
			index = combobox->findData(selected_q);
		}
		else
		{
			index = find_index(qstr(def));
		}

		m_broken_types.insert(type);
	}

	combobox->setCurrentIndex(index);

	connect(combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), combobox, [this, is_ranged, combobox, type](int index)
	{
		if (is_ranged)
		{
			SetSetting(type, sstr(combobox->itemData(index)));
		}
		else
		{
			const QVariantList var_list = combobox->itemData(index).toList();
			ensure(var_list.size() == 2 && var_list[0].canConvert<QString>());
			SetSetting(type, sstr(var_list[0]));
		}
	});
}

void emu_settings::EnhanceCheckBox(QCheckBox* checkbox, emu_settings_type type)
{
	if (!checkbox)
	{
		cfg_log.fatal("EnhanceCheckBox '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	std::string def = GetSettingDefault(type);
	std::transform(def.begin(), def.end(), def.begin(), ::tolower);

	if (def != "true" && def != "false")
	{
		cfg_log.fatal("EnhanceCheckBox '%s' was used with an invalid emu_settings_type", cfg_adapter::get_setting_name(type));
		return;
	}

	std::string selected = GetSetting(type);
	std::transform(selected.begin(), selected.end(), selected.begin(), ::tolower);

	if (selected == "true")
	{
		checkbox->setChecked(true);
	}
	else if (selected != "false")
	{
		cfg_log.error("EnhanceCheckBox '%s' tried to set an invalid value: %s. Setting to default: %s", cfg_adapter::get_setting_name(type), selected, def);
		checkbox->setChecked(def == "true");
		m_broken_types.insert(type);
	}

	connect(checkbox, &QCheckBox::stateChanged, [type, this](int val)
	{
		const std::string str = val != 0 ? "true" : "false";
		SetSetting(type, str);
	});
}

void emu_settings::EnhanceDateTimeEdit(QDateTimeEdit* date_time_edit, emu_settings_type type, const QString& format, bool use_calendar, bool as_offset_from_now, int offset_update_time)
{
	if (!date_time_edit)
	{
		cfg_log.fatal("EnhanceDateTimeEdit '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	date_time_edit->setDisplayFormat(format);
	date_time_edit->setCalendarPopup(use_calendar);

	if (as_offset_from_now)
	{
		// If using offset from now, then we disable the keyboard tracking to reduce the numebr of events that occur (since for each event we will lose focus)
		date_time_edit->setKeyboardTracking(false);

		bool ok_def = false, ok_min = false, ok_max = false;
		const QStringList range = GetSettingOptions(type);
		const s64 def = qstr(GetSettingDefault(type)).toLongLong(&ok_def);
		const s64 min = range.first().toLongLong(&ok_min);
		const s64 max = range.last().toLongLong(&ok_max);
		if (!ok_def || !ok_min || !ok_max)
		{
			cfg_log.fatal("EnhanceDateTimeEdit '%s' was used with an invalid emu_settings_type", cfg_adapter::get_setting_name(type));
			return;
		}

		bool ok_sel = false;
		s64 val = qstr(GetSetting(type)).toLongLong(&ok_sel);
		if (!ok_sel || val < min || val > max)
		{
			cfg_log.error("EnhanceDateTimeEdit '%s' tried to set an invalid value: %d. Setting to default: %d. Allowed range: [%d, %d]", cfg_adapter::get_setting_name(type), val, def, min, max);
			val = def;
			m_broken_types.insert(type);
			SetSetting(type, std::to_string(def));
		}

		// we'll capture the DateTime once, and apply the min/max and offset against it here.
		const QDateTime now = QDateTime::currentDateTime();

		// we set the allowed limits
		date_time_edit->setDateTimeRange(now.addSecs(min), now.addSecs(max));

		// we add the offset, and set the control to have this datetime value
		const QDateTime date_time = now.addSecs(val);
		date_time_edit->setDateTime(date_time);

		// if we have an invalid update time then we won't run the update timer
		if (offset_update_time > 0)
		{
			QTimer* console_time_update = new QTimer(date_time_edit);
			connect(console_time_update, &QTimer::timeout, [this, date_time_edit, min, max]()
			{
				if (!date_time_edit->hasFocus() && (!date_time_edit->calendarPopup() || !date_time_edit->calendarWidget()->hasFocus()))
				{
					const auto now   = QDateTime::currentDateTime();
					const s64 offset = qstr(GetSetting(emu_settings_type::ConsoleTimeOffset)).toLongLong();
					date_time_edit->setDateTime(now.addSecs(offset));
					date_time_edit->setDateTimeRange(now.addSecs(min), now.addSecs(max));
				}
			});

			console_time_update->start(offset_update_time);
		}
	}
	else
	{
		auto str                = qstr(GetSettingDefault(type));
		const QStringList range = GetSettingOptions(type);
		const auto def          = QDateTime::fromString(str, Qt::ISODate);
		const auto min          = QDateTime::fromString(range.first(), Qt::ISODate);
		const auto max          = QDateTime::fromString(range.last(), Qt::ISODate);
		if (!def.isValid() || !min.isValid() || !max.isValid())
		{
			cfg_log.fatal("EnhanceDateTimeEdit '%s' was used with an invalid emu_settings_type", cfg_adapter::get_setting_name(type));
			return;
		}

		str = qstr(GetSetting(type));
		auto val = QDateTime::fromString(str, Qt::ISODate);
		if (!val.isValid() || val < min || val > max)
		{
			cfg_log.error("EnhanceDateTimeEdit '%s' tried to set an invalid value: %s. Setting to default: %s Allowed range: [%s, %s]",
				cfg_adapter::get_setting_name(type), val.toString(Qt::ISODate).toStdString(), def.toString(Qt::ISODate).toStdString(),
				min.toString(Qt::ISODate).toStdString(), max.toString(Qt::ISODate).toStdString());
			val = def;
			m_broken_types.insert(type);
			SetSetting(type, sstr(def.toString(Qt::ISODate)));
		}

		// we set the allowed limits
		date_time_edit->setDateTimeRange(min, max);

		// set the date_time value to the control
		date_time_edit->setDateTime(val);
	}

	connect(date_time_edit, &QDateTimeEdit::dateTimeChanged, [date_time_edit, type, as_offset_from_now, this](const QDateTime& datetime)
	{
		if (as_offset_from_now)
		{
			// offset will be applied in seconds
			const s64 offset = QDateTime::currentDateTime().secsTo(datetime);
			SetSetting(type, std::to_string(offset));

			// HACK: We are only looking at whether the control has focus to prevent the time from updating dynamically, so we
			// clear the focus, so that this dynamic updating isn't suppressed.
			date_time_edit->clearFocus();
		}
		else
		{
			// date time will be written straight into settings
			SetSetting(type, sstr(datetime.toString(Qt::ISODate)));
		}
	});
}

void emu_settings::EnhanceSlider(QSlider* slider, emu_settings_type type)
{
	if (!slider)
	{
		cfg_log.fatal("EnhanceSlider '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	const QStringList range = GetSettingOptions(type);
	bool ok_def, ok_sel, ok_min, ok_max;

	const int def = qstr(GetSettingDefault(type)).toInt(&ok_def);
	const int min = range.first().toInt(&ok_min);
	const int max = range.last().toInt(&ok_max);

	if (!ok_def || !ok_min || !ok_max)
	{
		cfg_log.fatal("EnhanceSlider '%s' was used with an invalid emu_settings_type", cfg_adapter::get_setting_name(type));
		return;
	}

	const QString selected = qstr(GetSetting(type));
	int val = selected.toInt(&ok_sel);

	if (!ok_sel || val < min || val > max)
	{
		cfg_log.error("EnhanceSlider '%s' tried to set an invalid value: %d. Setting to default: %d. Allowed range: [%d, %d]", cfg_adapter::get_setting_name(type), val, def, min, max);
		val = def;
		m_broken_types.insert(type);
	}

	slider->setRange(min, max);
	slider->setValue(val);

	connect(slider, &QSlider::valueChanged, [type, this](int value)
	{
		SetSetting(type, sstr(value));
	});
}

void emu_settings::EnhanceSpinBox(QSpinBox* spinbox, emu_settings_type type, const QString& prefix, const QString& suffix)
{
	if (!spinbox)
	{
		cfg_log.fatal("EnhanceSpinBox '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	const QStringList range = GetSettingOptions(type);
	bool ok_def, ok_sel, ok_min, ok_max;

	const int def = qstr(GetSettingDefault(type)).toInt(&ok_def);
	const int min = range.first().toInt(&ok_min);
	const int max = range.last().toInt(&ok_max);

	if (!ok_def || !ok_min || !ok_max)
	{
		cfg_log.fatal("EnhanceSpinBox '%s' was used with an invalid type", cfg_adapter::get_setting_name(type));
		return;
	}

	const std::string selected = GetSetting(type);
	int val = qstr(selected).toInt(&ok_sel);

	if (!ok_sel || val < min || val > max)
	{
		cfg_log.error("EnhanceSpinBox '%s' tried to set an invalid value: %d. Setting to default: %d. Allowed range: [%d, %d]", cfg_adapter::get_setting_name(type), selected, def, min, max);
		val = def;
		m_broken_types.insert(type);
	}

	spinbox->setPrefix(prefix);
	spinbox->setSuffix(suffix);
	spinbox->setRange(min, max);
	spinbox->setValue(val);

	connect(spinbox, &QSpinBox::textChanged, [=, this](const QString&/* text*/)
	{
		SetSetting(type, sstr(spinbox->cleanText()));
	});
}

void emu_settings::EnhanceDoubleSpinBox(QDoubleSpinBox* spinbox, emu_settings_type type, const QString& prefix, const QString& suffix)
{
	if (!spinbox)
	{
		cfg_log.fatal("EnhanceDoubleSpinBox '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	const QStringList range = GetSettingOptions(type);
	bool ok_def, ok_sel, ok_min, ok_max;

	const double def = qstr(GetSettingDefault(type)).toDouble(&ok_def);
	const double min = range.first().toDouble(&ok_min);
	const double max = range.last().toDouble(&ok_max);

	if (!ok_def || !ok_min || !ok_max)
	{
		cfg_log.fatal("EnhanceDoubleSpinBox '%s' was used with an invalid type", cfg_adapter::get_setting_name(type));
		return;
	}

	const std::string selected = GetSetting(type);
	double val = qstr(selected).toDouble(&ok_sel);

	if (!ok_sel || val < min || val > max)
	{
		cfg_log.error("EnhanceDoubleSpinBox '%s' tried to set an invalid value: %f. Setting to default: %f. Allowed range: [%f, %f]", cfg_adapter::get_setting_name(type), val, def, min, max);
		val = def;
		m_broken_types.insert(type);
	}

	spinbox->setPrefix(prefix);
	spinbox->setSuffix(suffix);
	spinbox->setRange(min, max);
	spinbox->setValue(val);

	connect(spinbox, &QDoubleSpinBox::textChanged, [=, this](const QString&/* text*/)
	{
		SetSetting(type, sstr(spinbox->cleanText()));
	});
}

void emu_settings::EnhanceLineEdit(QLineEdit* edit, emu_settings_type type)
{
	if (!edit)
	{
		cfg_log.fatal("EnhanceEdit '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	const std::string set_text = GetSetting(type);
	edit->setText(qstr(set_text));

	connect(edit, &QLineEdit::textChanged, [type, this](const QString &text)
	{
		SetSetting(type, sstr(text));
	});
}

void emu_settings::EnhanceRadioButton(QButtonGroup* button_group, emu_settings_type type)
{
	if (!button_group)
	{
		cfg_log.fatal("EnhanceRadioButton '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	const QString selected    = qstr(GetSetting(type));
	const QString def         = qstr(GetSettingDefault(type));
	const QStringList options = GetSettingOptions(type);

	if (button_group->buttons().count() < options.size())
	{
		cfg_log.fatal("EnhanceRadioButton '%s': wrong button count", cfg_adapter::get_setting_name(type));
		return;
	}

	bool found = false;
	int def_pos = -1;

	for (int i = 0; i < options.count(); i++)
	{
		const QString localized_setting = GetLocalizedSetting(options[i], type, i);

		button_group->button(i)->setText(localized_setting);

		if (!found && options[i] == selected)
		{
			found = true;
			button_group->button(i)->setChecked(true);
		}
		else if (def_pos == -1 && options[i] == def)
		{
			def_pos = i;
		}

		connect(button_group->button(i), &QAbstractButton::clicked, [=, this]()
		{
			SetSetting(type, sstr(options[i]));
		});
	}

	if (!found)
	{
		ensure(def_pos >= 0);

		cfg_log.error("EnhanceRadioButton '%s' tried to set an invalid value: %s. Setting to default: %s.", cfg_adapter::get_setting_name(type), sstr(selected), sstr(def));
		m_broken_types.insert(type);

		// Select the default option on invalid setting string
		button_group->button(def_pos)->setChecked(true);
	}
}

std::vector<std::string> emu_settings::GetLibrariesControl()
{
	return m_current_settings["Core"]["Libraries Control"].as<std::vector<std::string>, std::initializer_list<std::string>>({});
}

void emu_settings::SaveSelectedLibraries(const std::vector<std::string>& libs)
{
	m_current_settings["Core"]["Libraries Control"] = libs;
}

QStringList emu_settings::GetSettingOptions(emu_settings_type type)
{
	return cfg_adapter::get_options(const_cast<cfg_location&&>(settings_location[type]));
}

std::string emu_settings::GetSettingDefault(emu_settings_type type) const
{
	if (const auto node = cfg_adapter::get_node(m_default_settings, settings_location[type]); node && node.IsScalar())
	{
		return node.Scalar();
	}

	cfg_log.fatal("GetSettingDefault(type=%d) could not retrieve the requested node", static_cast<int>(type));
	return "";
}

std::string emu_settings::GetSetting(emu_settings_type type) const
{
	if (const auto node = cfg_adapter::get_node(m_current_settings, settings_location[type]); node && node.IsScalar())
	{
		return node.Scalar();
	}

	cfg_log.fatal("GetSetting(type=%d) could not retrieve the requested node", static_cast<int>(type));
	return "";
}

void emu_settings::SetSetting(emu_settings_type type, const std::string& val) const
{
	cfg_adapter::get_node(m_current_settings, settings_location[type]) = val;
}

void emu_settings::OpenCorrectionDialog(QWidget* parent)
{
	if (!m_broken_types.empty() && QMessageBox::question(parent, tr("Fix invalid settings?"),
		tr(
			"Your config file contained one or more unrecognized values for settings.\n"
			"Their default value will be used until they are corrected.\n"
			"Consider that a correction might render them invalid for other versions of RPCS3.\n"
			"\n"
			"Do you wish to let the program correct them for you?\n"
			"This change will only be final when you save the config."
		),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
	{
		for (const auto& type : m_broken_types)
		{
			const std::string def = GetSettingDefault(type);
			const std::string old = GetSetting(type);
			SetSetting(type, def);
			cfg_log.success("The config entry '%s' was corrected from '%s' to '%s'", cfg_adapter::get_setting_name(type), old, def);
		}

		m_broken_types.clear();
		cfg_log.success("You need to save the settings in order to make these changes permanent!");
	}
}

QString emu_settings::GetLocalizedSetting(const QString& original, emu_settings_type type, int index) const
{
	switch (type)
	{
	case emu_settings_type::SPUBlockSize:
		switch (static_cast<spu_block_size_type>(index))
		{
		case spu_block_size_type::safe: return tr("Safe", "SPU block size");
		case spu_block_size_type::mega: return tr("Mega", "SPU block size");
		case spu_block_size_type::giga: return tr("Giga", "SPU block size");
		}
		break;
	case emu_settings_type::ThreadSchedulerMode:
		switch (static_cast<thread_scheduler_mode>(index))
		{
		case thread_scheduler_mode::old: return tr("RPCS3 Scheduler", "Thread Scheduler Mode");
		case thread_scheduler_mode::alt: return tr("RPCS3 Alternative Scheduler", "Thread Scheduler Mode");
		case thread_scheduler_mode::os: return tr("Operating System", "Thread Scheduler Mode");
		}
		break;
	case emu_settings_type::EnableTSX:
		switch (static_cast<tsx_usage>(index))
		{
		case tsx_usage::disabled: return tr("Disabled", "Enable TSX");
		case tsx_usage::enabled: return tr("Enabled", "Enable TSX");
		case tsx_usage::forced: return tr("Forced", "Enable TSX");
		}
		break;
	case emu_settings_type::Renderer:
		switch (static_cast<video_renderer>(index))
		{
		case video_renderer::null: return tr("Disable Video Output", "Video renderer");
		case video_renderer::opengl: return tr("OpenGL", "Video renderer");
		case video_renderer::vulkan: return tr("Vulkan", "Video renderer");
		}
		break;
	case emu_settings_type::ShaderMode:
		switch (static_cast<shader_mode>(index))
		{
		case shader_mode::recompiler: return tr("Legacy (single threaded)", "Shader Mode");
		case shader_mode::async_recompiler: return tr("Async (multi threaded)", "Shader Mode");
		case shader_mode::async_with_interpreter: return tr("Async with Shader Interpreter", "Shader Mode");
		case shader_mode::interpreter_only: return tr("Shader Interpreter only", "Shader Mode");
		}
		break;
	case emu_settings_type::FrameLimit:
		switch (static_cast<frame_limit_type>(index))
		{
		case frame_limit_type::none: return tr("Off", "Frame limit");
		case frame_limit_type::_59_94: return tr("59.94", "Frame limit");
		case frame_limit_type::_50: return tr("50", "Frame limit");
		case frame_limit_type::_60: return tr("60", "Frame limit");
		case frame_limit_type::_30: return tr("30", "Frame limit");
		case frame_limit_type::_auto: return tr("Auto", "Frame limit");
		}
		break;
	case emu_settings_type::MSAA:
		switch (static_cast<msaa_level>(index))
		{
		case msaa_level::none: return tr("Disabled", "MSAA");
		case msaa_level::_auto: return tr("Auto", "MSAA");
		}
		break;
	case emu_settings_type::AudioRenderer:
		switch (static_cast<audio_renderer>(index))
		{
		case audio_renderer::null: return tr("Disable Audio Output", "Audio renderer");
#ifdef _WIN32
		case audio_renderer::xaudio: return tr("XAudio2", "Audio renderer");
#endif
#ifdef HAVE_ALSA
		case audio_renderer::alsa: return tr("ALSA", "Audio renderer");
#endif
#ifdef HAVE_PULSE
		case audio_renderer::pulse: return tr("PulseAudio", "Audio renderer");
#endif
		case audio_renderer::openal: return tr("OpenAL", "Audio renderer");
#ifdef HAVE_FAUDIO
		case audio_renderer::faudio: return tr("FAudio", "Audio renderer");
#endif
		}
		break;
	case emu_settings_type::MicrophoneType:
		switch (static_cast<microphone_handler>(index))
		{
		case microphone_handler::null: return tr("Disabled", "Microphone handler");
		case microphone_handler::standard: return tr("Standard", "Microphone handler");
		case microphone_handler::singstar: return tr("SingStar", "Microphone handler");
		case microphone_handler::real_singstar: return tr("Real SingStar", "Microphone handler");
		case microphone_handler::rocksmith: return tr("Rocksmith", "Microphone handler");
		}
		break;
	case emu_settings_type::KeyboardHandler:
		switch (static_cast<keyboard_handler>(index))
		{
		case keyboard_handler::null: return tr("Null", "Keyboard handler");
		case keyboard_handler::basic: return tr("Basic", "Keyboard handler");
		}
		break;
	case emu_settings_type::MouseHandler:
		switch (static_cast<mouse_handler>(index))
		{
		case mouse_handler::null: return tr("Null", "Mouse handler");
		case mouse_handler::basic: return tr("Basic", "Mouse handler");
		}
		break;
	case emu_settings_type::CameraType:
		switch (static_cast<fake_camera_type>(index))
		{
		case fake_camera_type::unknown: return tr("Unknown", "Camera type");
		case fake_camera_type::eyetoy: return tr("EyeToy", "Camera type");
		case fake_camera_type::eyetoy2: return tr("PS Eye", "Camera type");
		case fake_camera_type::uvc1_1: return tr("UVC 1.1", "Camera type");
		}
		break;
	case emu_settings_type::Camera:
		switch (static_cast<camera_handler>(index))
		{
		case camera_handler::null: return tr("Null", "Camera handler");
		case camera_handler::fake: return tr("Fake", "Camera handler");
		}
		break;
	case emu_settings_type::Move:
		switch (static_cast<move_handler>(index))
		{
		case move_handler::null: return tr("Null", "Move handler");
		case move_handler::fake: return tr("Fake", "Move handler");
		case move_handler::mouse: return tr("Mouse", "Move handler");
		}
		break;
	case emu_settings_type::Buzz:
		switch (static_cast<buzz_handler>(index))
		{
		case buzz_handler::null: return tr("Null (use real Buzzers)", "Buzz handler");
		case buzz_handler::one_controller: return tr("1 controller (1-4 players)", "Buzz handler");
		case buzz_handler::two_controllers: return tr("2 controllers (5-7 players)", "Buzz handler");
		}
		break;
	case emu_settings_type::InternetStatus:
		switch (static_cast<np_internet_status>(index))
		{
		case np_internet_status::disabled: return tr("Disconnected", "Internet Status");
		case np_internet_status::enabled: return tr("Connected", "Internet Status");
		}
		break;
	case emu_settings_type::PSNStatus:
		switch (static_cast<np_psn_status>(index))
		{
		case np_psn_status::disabled: return tr("Disconnected", "PSN Status");
		case np_psn_status::fake: return tr("Simulated", "PSN Status");
		case np_psn_status::rpcn: return tr("RPCN", "PSN Status");
		}
		break;
	case emu_settings_type::SleepTimersAccuracy:
		switch (static_cast<sleep_timers_accuracy_level>(index))
		{
		case sleep_timers_accuracy_level::_as_host: return tr("As Host", "Sleep timers accuracy");
		case sleep_timers_accuracy_level::_usleep: return tr("Usleep Only", "Sleep timers accuracy");
		case sleep_timers_accuracy_level::_all_timers: return tr("All Timers", "Sleep timers accuracy");
		}
		break;
	case emu_settings_type::PerfOverlayDetailLevel:
		switch (static_cast<detail_level>(index))
		{
		case detail_level::none: return tr("None", "Detail Level");
		case detail_level::minimal: return tr("Minimal", "Detail Level");
		case detail_level::low: return tr("Low", "Detail Level");
		case detail_level::medium: return tr("Medium", "Detail Level");
		case detail_level::high: return tr("High", "Detail Level");
		}
		break;
	case emu_settings_type::PerfOverlayPosition:
		switch (static_cast<screen_quadrant>(index))
		{
		case screen_quadrant::top_left: return tr("Top Left", "Performance overlay position");
		case screen_quadrant::top_right: return tr("Top Right", "Performance overlay position");
		case screen_quadrant::bottom_left: return tr("Bottom Left", "Performance overlay position");
		case screen_quadrant::bottom_right: return tr("Bottom Right", "Performance overlay position");
		}
		break;
	case emu_settings_type::PPUDecoder:
		switch (static_cast<ppu_decoder_type>(index))
		{
		case ppu_decoder_type::precise: return tr("Interpreter (precise)", "PPU decoder");
		case ppu_decoder_type::fast: return tr("Interpreter (fast)", "PPU decoder");
		case ppu_decoder_type::llvm: return tr("Recompiler (LLVM)", "PPU decoder");
		}
		break;
	case emu_settings_type::SPUDecoder:
		switch (static_cast<spu_decoder_type>(index))
		{
		case spu_decoder_type::precise: return tr("Interpreter (precise)", "SPU decoder");
		case spu_decoder_type::fast: return tr("Interpreter (fast)", "SPU decoder");
		case spu_decoder_type::asmjit: return tr("Recompiler (ASMJIT)", "SPU decoder");
		case spu_decoder_type::llvm: return tr("Recompiler (LLVM)", "SPU decoder");
		}
		break;
	case emu_settings_type::EnterButtonAssignment:
		switch (static_cast<enter_button_assign>(index))
		{
		case enter_button_assign::circle: return tr("Enter with circle", "Enter button assignment");
		case enter_button_assign::cross: return tr("Enter with cross", "Enter button assignment");
		}
		break;
	case emu_settings_type::AudioChannels:
		switch (static_cast<audio_downmix>(index))
		{
		case audio_downmix::no_downmix: return tr("Surround 7.1", "Audio downmix");
		case audio_downmix::downmix_to_stereo: return tr("Downmix to Stereo", "Audio downmix");
		case audio_downmix::downmix_to_5_1: return tr("Downmix to 5.1", "Audio downmix");
		case audio_downmix::use_application_settings: return tr("Use application settings", "Audio downmix");
		}
		break;
	case emu_settings_type::LicenseArea:
		switch (static_cast<CellSysutilLicenseArea>(index))
		{
		case CellSysutilLicenseArea::CELL_SYSUTIL_LICENSE_AREA_J: return tr("Japan", "License Area");
		case CellSysutilLicenseArea::CELL_SYSUTIL_LICENSE_AREA_A: return tr("America", "License Area");
		case CellSysutilLicenseArea::CELL_SYSUTIL_LICENSE_AREA_E: return tr("Europe, Oceania, Middle East, Russia", "License Area");
		case CellSysutilLicenseArea::CELL_SYSUTIL_LICENSE_AREA_H: return tr("Southeast Asia", "License Area");
		case CellSysutilLicenseArea::CELL_SYSUTIL_LICENSE_AREA_K: return tr("Korea", "License Area");
		case CellSysutilLicenseArea::CELL_SYSUTIL_LICENSE_AREA_C: return tr("China", "License Area");
		case CellSysutilLicenseArea::CELL_SYSUTIL_LICENSE_AREA_OTHER: return tr("Other", "License Area");
		}
		break;
	default:
		break;
	}

	return original;
}
