#include "emu_settings.h"
#include "config_adapter.h"

#include <QMessageBox>
#include <QLineEdit>
#include <QTimer>
#include <QCalendarWidget>

#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/system_utils.hpp"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "Emu/Io/Keyboard.h"

#include "util/yaml.hpp"
#include "Utilities/File.h"
#include "Utilities/Config.h"

LOG_CHANNEL(cfg_log, "CFG");

extern std::string g_cfg_defaults; //! Default settings grabbed from Utilities/Config.h

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

	if (m_render_creator->abort_requested)
	{
		return false;
	}

	// Make Vulkan default setting if it is supported
	if (m_render_creator->Vulkan.supported && !m_render_creator->Vulkan.adapters.empty())
	{
		const std::string adapter = ::at32(m_render_creator->Vulkan.adapters, 0).toStdString();
		cfg_log.notice("Setting the default renderer to Vulkan. Default GPU: '%s'", adapter);
		Emu.SetDefaultRenderer(video_renderer::vulkan);
		Emu.SetDefaultGraphicsAdapter(adapter);
	}

	return true;
}

void emu_settings::LoadSettings(const std::string& title_id, bool create_config_from_global)
{
	m_title_id = title_id;

	// Create config path if necessary
	fs::create_path(title_id.empty() ? fs::get_config_dir(true) : rpcs3::utils::get_custom_config_dir());

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

	if (create_config_from_global)
	{
		// Add global config
		const std::string global_config_path = fs::get_config_dir(true) + "config.yml";
		fs::g_tls_error = fs::error::ok;
		fs::file config(global_config_path, fs::read + fs::create);
		auto [global_config, global_error] = yaml_load(config ? config.to_string() : "");

		if (config && global_error.empty())
		{
			m_current_settings += global_config;
		}
		else
		{
			config.close();
			cfg_log.fatal("Failed to load global config %s:\n%s (%s)", global_config_path, global_error, fs::g_tls_error);
			QMessageBox::critical(nullptr, tr("Config Error"), tr("Failed to load global config:\nFile: %0\nError: %1")
				.arg(QString::fromStdString(global_config_path)).arg(QString::fromStdString(global_error)), QMessageBox::Ok);
		}
	}

	// Add game config
	if (!title_id.empty())
	{
		// Remove obsolete settings of the global config before adding the custom settings.
		// Otherwise we'll always trigger the "obsolete settings dialog" when editing custom configs.
		ValidateSettings(true);

		std::string custom_config_path;

		if (std::string config_path = rpcs3::utils::get_custom_config_path(m_title_id); fs::is_file(config_path))
		{
			custom_config_path = std::move(config_path);
		}

		if (!custom_config_path.empty())
		{
			if (fs::file config{custom_config_path})
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
			else if (fs::g_tls_error != fs::error::noent)
			{
				cfg_log.fatal("Failed to load custom config %s (file error: %s)", custom_config_path, fs::g_tls_error);
					QMessageBox::critical(nullptr, tr("Config Error"), tr("Failed to load custom config:\nFile: %0\nError: %1")
						.arg(QString::fromStdString(custom_config_path)).arg(QString::fromStdString(fmt::format("%s", fs::g_tls_error))), QMessageBox::Ok);
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
			const std::string& key = yml_entry.first.Scalar();
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
				// Ignore every node in Log subsection
				if (level == 0 && cfg_node->get_name() == "Log")
				{
					continue;
				}

				YAML::Node next_node = yml_node[key];
				search_level(next_level, next_node, keys, cfg_node);
			}
			else
			{
				const auto get_full_key = [&keys](const std::string& separator) -> std::string
				{
					std::string full_key;
					for (usz i = 0; i < keys.size(); i++)
					{
						full_key += keys[i];
						if (i < keys.size() - 1) full_key += separator;
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

	std::unique_ptr<cfg_root> root = std::make_unique<cfg_root>();
	std::vector<std::string> keys;

	do
	{
		is_clean = true;
		search_level(0, m_current_settings, keys, root.get());
	}
	while (cleanup && !is_clean);

	return is_clean;
}

void emu_settings::RestoreDefaults()
{
	m_current_settings = YAML::Clone(m_default_settings);
	Q_EMIT RestoreDefaultsSignal();
}

void emu_settings::SaveSettings() const
{
	YAML::Emitter out;
	emit_data(out, m_current_settings);
	Emulator::SaveSettings(out.c_str(), m_title_id);
}

void emu_settings::EnhanceComboBox(QComboBox* combobox, emu_settings_type type, bool is_ranged, bool use_max, int max, bool sorted, bool strict)
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

		const QStringList range = GetQStringSettingOptions(type);
		ensure(!range.empty());

		const int max_item = use_max ? max : range.last().toInt();

		for (int i = range.first().toInt(); i <= max_item; i++)
		{
			combobox->addItem(QString::number(i), i);
		}
	}
	else
	{
		const QStringList settings = GetQStringSettingOptions(type);

		for (int i = 0; i < settings.count(); i++)
		{
			const QString localized_setting = GetLocalizedSetting(settings[i], type, combobox->count(), strict);
			combobox->addItem(localized_setting, QVariant({settings[i], i}));
		}

		if (sorted)
		{
			combobox->model()->sort(0, Qt::AscendingOrder);
		}
	}

	// Since the QComboBox has localised strings, we can't just findText / findData, so we need to manually iterate through it to find our index
	const auto find_index = [](QComboBox* combobox, const QString& value)
	{
		if (!combobox)
		{
			return -1;
		}

		for (int i = 0; i < combobox->count(); i++)
		{
			const QVariantList var_list = combobox->itemData(i).toList();
			if (var_list.size() != 2 || !var_list[0].canConvert<QString>())
			{
				fmt::throw_exception("Invalid data found in combobox entry %d (text='%s', listsize=%d, itemcount=%d)", i, combobox->itemText(i), var_list.size(), combobox->count());
			}

			if (value == var_list[0].toString())
			{
				return i;
			}
		}
		return -1;
	};

	const std::string def      = GetSettingDefault(type);
	const std::string selected = GetSetting(type);
	const QString selected_q = QString::fromStdString(selected);
	int index;

	if (is_ranged)
	{
		index = combobox->findData(selected_q);
	}
	else
	{
		index = find_index(combobox, selected_q);
	}

	if (index == -1)
	{
		cfg_log.error("EnhanceComboBox '%s' tried to set an invalid value: %s. Setting to default: %s", cfg_adapter::get_setting_name(type), selected, def);

		if (is_ranged)
		{
			index = combobox->findData(QString::fromStdString(def));
		}
		else
		{
			index = find_index(combobox, QString::fromStdString(def));
		}

		m_broken_types.insert(type);
	}

	combobox->setCurrentIndex(index);

	connect(combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), combobox, [this, is_ranged, combobox, type](int index)
	{
		if (index < 0) return;

		if (is_ranged)
		{
			SetSetting(type, combobox->itemData(index).toString().toStdString());
		}
		else
		{
			const QVariantList var_list = combobox->itemData(index).toList();
			if (var_list.size() != 2 || !var_list[0].canConvert<QString>())
			{
				fmt::throw_exception("Invalid data found in combobox entry %d (text='%s', listsize=%d, itemcount=%d)", index, combobox->itemText(index), var_list.size(), combobox->count());
			}
			SetSetting(type, var_list[0].toString().toStdString());
		}
	});

	connect(this, &emu_settings::RestoreDefaultsSignal, combobox, [def, combobox, is_ranged, find_index]()
	{
		if (is_ranged)
		{
			combobox->setCurrentIndex(combobox->findData(QString::fromStdString(def)));
		}
		else
		{
			combobox->setCurrentIndex(find_index(combobox, QString::fromStdString(def)));
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

	connect(checkbox, &QCheckBox::checkStateChanged, this, [type, this](Qt::CheckState val)
	{
		const std::string str = val != Qt::Unchecked ? "true" : "false";
		SetSetting(type, str);
	});

	connect(this, &emu_settings::RestoreDefaultsSignal, checkbox, [def, checkbox]()
	{
		checkbox->setChecked(def == "true");
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

		const QStringList range = GetQStringSettingOptions(type);
		ensure(!range.empty());

		bool ok_def = false, ok_min = false, ok_max = false;
		const s64 def = QString::fromStdString(GetSettingDefault(type)).toLongLong(&ok_def);
		const s64 min = range.first().toLongLong(&ok_min);
		const s64 max = range.last().toLongLong(&ok_max);
		if (!ok_def || !ok_min || !ok_max)
		{
			cfg_log.fatal("EnhanceDateTimeEdit '%s' was used with an invalid emu_settings_type", cfg_adapter::get_setting_name(type));
			return;
		}

		bool ok_sel = false;
		s64 val = QString::fromStdString(GetSetting(type)).toLongLong(&ok_sel);
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
			connect(console_time_update, &QTimer::timeout, date_time_edit, [this, date_time_edit, min, max]()
			{
				if (!date_time_edit->hasFocus() && (!date_time_edit->calendarPopup() || !date_time_edit->calendarWidget()->hasFocus()))
				{
					const QDateTime now = QDateTime::currentDateTime();
					const s64 offset = QString::fromStdString(GetSetting(emu_settings_type::ConsoleTimeOffset)).toLongLong();
					date_time_edit->setDateTime(now.addSecs(offset));
					date_time_edit->setDateTimeRange(now.addSecs(min), now.addSecs(max));
				}
			});

			console_time_update->start(offset_update_time);
		}

		connect(this, &emu_settings::RestoreDefaultsSignal, date_time_edit, [def, date_time_edit]()
		{
			date_time_edit->setDateTime(QDateTime::currentDateTime().addSecs(def));
		});
	}
	else
	{
		const QStringList range = GetQStringSettingOptions(type);
		ensure(!range.empty());

		QString str             = QString::fromStdString(GetSettingDefault(type));
		const QDateTime def     = QDateTime::fromString(str, Qt::ISODate);
		const QDateTime min     = QDateTime::fromString(range.first(), Qt::ISODate);
		const QDateTime max     = QDateTime::fromString(range.last(), Qt::ISODate);
		if (!def.isValid() || !min.isValid() || !max.isValid())
		{
			cfg_log.fatal("EnhanceDateTimeEdit '%s' was used with an invalid emu_settings_type", cfg_adapter::get_setting_name(type));
			return;
		}

		str = QString::fromStdString(GetSetting(type));
		QDateTime val = QDateTime::fromString(str, Qt::ISODate);
		if (!val.isValid() || val < min || val > max)
		{
			cfg_log.error("EnhanceDateTimeEdit '%s' tried to set an invalid value: %s. Setting to default: %s Allowed range: [%s, %s]",
				cfg_adapter::get_setting_name(type), val.toString(Qt::ISODate), def.toString(Qt::ISODate), min.toString(Qt::ISODate), max.toString(Qt::ISODate));
			val = def;
			m_broken_types.insert(type);
			SetSetting(type, def.toString(Qt::ISODate).toStdString());
		}

		// we set the allowed limits
		date_time_edit->setDateTimeRange(min, max);

		// set the date_time value to the control
		date_time_edit->setDateTime(val);

		connect(this, &emu_settings::RestoreDefaultsSignal, date_time_edit, [def, date_time_edit]()
		{
			date_time_edit->setDateTime(def);
		});
	}

	connect(date_time_edit, &QDateTimeEdit::dateTimeChanged, this, [date_time_edit, type, as_offset_from_now, this](const QDateTime& datetime)
	{
		if (as_offset_from_now)
		{
			// offset will be applied in seconds
			const s64 offset = QDateTime::currentDateTime().secsTo(datetime);
			SetSetting(type, std::to_string(offset));

			// HACK: We are only looking at whether the control has focus to prevent the time from updating dynamically, so we
			// clear the focus, so that this dynamic updating isn't suppressed.
			if (date_time_edit)
			{
				date_time_edit->clearFocus();
			}
		}
		else
		{
			// date time will be written straight into settings
			SetSetting(type, datetime.toString(Qt::ISODate).toStdString());
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

	const QStringList range = GetQStringSettingOptions(type);
	ensure(!range.empty());

	bool ok_def, ok_sel, ok_min, ok_max;

	const int def = QString::fromStdString(GetSettingDefault(type)).toInt(&ok_def);
	const int min = range.first().toInt(&ok_min);
	const int max = range.last().toInt(&ok_max);

	if (!ok_def || !ok_min || !ok_max)
	{
		cfg_log.fatal("EnhanceSlider '%s' was used with an invalid emu_settings_type", cfg_adapter::get_setting_name(type));
		return;
	}

	const QString selected = QString::fromStdString(GetSetting(type));
	int val = selected.toInt(&ok_sel);

	if (!ok_sel || val < min || val > max)
	{
		cfg_log.error("EnhanceSlider '%s' tried to set an invalid value: %d. Setting to default: %d. Allowed range: [%d, %d]", cfg_adapter::get_setting_name(type), val, def, min, max);
		val = def;
		m_broken_types.insert(type);
	}

	slider->setRange(min, max);
	slider->setValue(val);

	connect(slider, &QSlider::valueChanged, this, [type, this](int value)
	{
		SetSetting(type, QString::number(value).toStdString());
	});

	connect(this, &emu_settings::RestoreDefaultsSignal, slider, [def, slider]()
	{
		slider->setValue(def);
	});
}

void emu_settings::EnhanceSpinBox(QSpinBox* spinbox, emu_settings_type type, const QString& prefix, const QString& suffix)
{
	if (!spinbox)
	{
		cfg_log.fatal("EnhanceSpinBox '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	const QStringList range = GetQStringSettingOptions(type);
	ensure(!range.empty());

	bool ok_def, ok_sel, ok_min, ok_max;

	const int def = QString::fromStdString(GetSettingDefault(type)).toInt(&ok_def);
	const int min = range.first().toInt(&ok_min);
	const int max = range.last().toInt(&ok_max);

	if (!ok_def || !ok_min || !ok_max)
	{
		cfg_log.fatal("EnhanceSpinBox '%s' was used with an invalid type", cfg_adapter::get_setting_name(type));
		return;
	}

	const std::string selected = GetSetting(type);
	int val = QString::fromStdString(selected).toInt(&ok_sel);

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

	connect(spinbox, &QSpinBox::textChanged, this, [type, spinbox, this](const QString& /* text*/)
	{
		if (!spinbox) return;
		SetSetting(type, spinbox->cleanText().toStdString());
	});

	connect(this, &emu_settings::RestoreDefaultsSignal, spinbox, [def, spinbox]()
	{
		spinbox->setValue(def);
	});
}

void emu_settings::EnhanceDoubleSpinBox(QDoubleSpinBox* spinbox, emu_settings_type type, const QString& prefix, const QString& suffix)
{
	if (!spinbox)
	{
		cfg_log.fatal("EnhanceDoubleSpinBox '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	const std::vector<std::string> range = GetSettingOptions(type);
	ensure(!range.empty());

	const std::string def_s = GetSettingDefault(type);
	const std::string val_s = GetSetting(type);
	const std::string& min_s = range.front();
	const std::string& max_s = range.back();

	// cfg::_float range is in s32
	constexpr s32 min_value = ::std::numeric_limits<s32>::min();
	constexpr s32 max_value = ::std::numeric_limits<s32>::max();

	f64 val, def, min, max;
	const bool ok_sel = try_to_float(&val, val_s, min_value, max_value);
	const bool ok_def = try_to_float(&def, def_s, min_value, max_value);
	const bool ok_min = try_to_float(&min, min_s, min_value, max_value);
	const bool ok_max = try_to_float(&max, max_s, min_value, max_value);

	if (!ok_def || !ok_min || !ok_max)
	{
		cfg_log.fatal("EnhanceDoubleSpinBox '%s' was used with an invalid type. (val='%s', def='%s', min_s='%s', max_s='%s')", cfg_adapter::get_setting_name(type), val_s, def_s, min_s, max_s);
		return;
	}

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

	connect(spinbox, &QDoubleSpinBox::textChanged, this, [type, spinbox, this](const QString& /* text*/)
	{
		if (!spinbox) return;
		SetSetting(type, spinbox->cleanText().toStdString());
	});

	connect(this, &emu_settings::RestoreDefaultsSignal, spinbox, [def, spinbox]()
	{
		spinbox->setValue(def);
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
	edit->setText(QString::fromStdString(set_text));

	connect(edit, &QLineEdit::textChanged, this, [type, this](const QString &text)
	{
		const QString trimmed = text.trimmed();
		if (trimmed.size() != text.size())
		{
			cfg_log.warning("EnhanceLineEdit '%s' input was trimmed", cfg_adapter::get_setting_name(type));
		}
		SetSetting(type, trimmed.toStdString());
	});

	connect(this, &emu_settings::RestoreDefaultsSignal, edit, [this, edit, type]()
	{
		edit->setText(QString::fromStdString(GetSettingDefault(type)));
	});
}

void emu_settings::EnhanceRadioButton(QButtonGroup* button_group, emu_settings_type type)
{
	if (!button_group)
	{
		cfg_log.fatal("EnhanceRadioButton '%s' was used with an invalid object", cfg_adapter::get_setting_name(type));
		return;
	}

	const QString selected    = QString::fromStdString(GetSetting(type));
	const QString def         = QString::fromStdString(GetSettingDefault(type));
	const QStringList options = GetQStringSettingOptions(type);

	if (button_group->buttons().count() < options.size())
	{
		cfg_log.fatal("EnhanceRadioButton '%s': wrong button count", cfg_adapter::get_setting_name(type));
		return;
	}

	bool found = false;
	int def_pos = -1;

	for (int i = 0; i < options.count(); i++)
	{
		const QString& option = options[i];
		const QString localized_setting = GetLocalizedSetting(option, type, i, true);

		QAbstractButton* button = button_group->button(i);
		button->setText(localized_setting);

		if (!found && option == selected)
		{
			found = true;
			button->setChecked(true);
		}

		if (def_pos == -1 && option == def)
		{
			def_pos = i;
		}

		connect(button, &QAbstractButton::toggled, this, [this, type, val = option.toStdString()](bool checked)
		{
			if (checked)
			{
				SetSetting(type, val);
			}
		});
	}

	if (!found)
	{
		ensure(def_pos >= 0);

		cfg_log.error("EnhanceRadioButton '%s' tried to set an invalid value: %s. Setting to default: %s.", cfg_adapter::get_setting_name(type), selected, def);
		m_broken_types.insert(type);

		// Select the default option on invalid setting string
		button_group->button(def_pos)->setChecked(true);
	}

	connect(this, &emu_settings::RestoreDefaultsSignal, button_group, [button_group, def_pos]()
	{
		if (button_group && button_group->button(def_pos))
		{
			button_group->button(def_pos)->setChecked(true);
		}
	});
}

std::vector<std::string> emu_settings::GetLibrariesControl()
{
	return m_current_settings["Core"]["Libraries Control"].as<std::vector<std::string>, std::initializer_list<std::string>>({});
}

void emu_settings::SaveSelectedLibraries(const std::vector<std::string>& libs)
{
	m_current_settings["Core"]["Libraries Control"] = libs;
}

std::vector<std::string> emu_settings::GetSettingOptions(emu_settings_type type)
{
	return cfg_adapter::get_options(::at32(settings_location, type));
}

QStringList emu_settings::GetQStringSettingOptions(emu_settings_type type)
{
	QStringList values;
	for (const std::string& value : cfg_adapter::get_options(::at32(settings_location, type)))
	{
		values.append(QString::fromStdString(value));
	}
	return values;
}

std::string emu_settings::GetSettingDefault(emu_settings_type type) const
{
	if (const auto node = cfg_adapter::get_node(m_default_settings, ::at32(settings_location, type)); node && node.IsScalar())
	{
		return node.Scalar();
	}

	cfg_log.fatal("GetSettingDefault(type=%d) could not retrieve the requested node", static_cast<int>(type));
	return "";
}

std::string emu_settings::GetSetting(emu_settings_type type) const
{
	if (const auto node = cfg_adapter::get_node(m_current_settings, ::at32(settings_location, type)); node && node.IsScalar())
	{
		return node.Scalar();
	}

	cfg_log.fatal("GetSetting(type=%d) could not retrieve the requested node", static_cast<int>(type));
	return "";
}

void emu_settings::SetSetting(emu_settings_type type, const std::string& val) const
{
	cfg_adapter::get_node(m_current_settings, ::at32(settings_location, type)) = val;
}

emu_settings_type emu_settings::FindSettingsType(const cfg::_base* node) const
{
	// Add key and value to static map on first use
	static std::map<u32, emu_settings_type> id_to_type;
	static std::mutex mtx;
	std::lock_guard lock(mtx);

	if (!node) [[unlikely]]
	{
		// Provoke error. Don't use ensure or we will get a nullptr deref warning in VS
		return ::at32(id_to_type, umax);
	}

	std::vector<std::string> node_location;
	if (!id_to_type.contains(node->get_id()))
	{
		for (const cfg::_base* n = node; n; n = n->get_parent())
		{
			if (!n->get_name().empty())
			{
				node_location.push_back(n->get_name());
			}
		}

		std::reverse(node_location.begin(), node_location.end());

		for (const auto& [type, loc]: settings_location)
		{
			if (node_location.size() != loc.size())
			{
				continue;
			}

			bool is_match = true;
			for (usz i = 0; i < node_location.size(); i++)
			{
				if (node_location[i] != loc[i])
				{
					is_match = false;
					break;
				}
			}

			if (is_match && !id_to_type.try_emplace(node->get_id(), type).second)
			{
				cfg_log.error("'%s' already exists", loc.back());
			}
		}
	}

	if (!id_to_type.contains(node->get_id()))
	{
		fmt::throw_exception("Node '%s' not represented in emu_settings_type", node->get_name());
	}

	return ::at32(id_to_type, node->get_id());
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

QString emu_settings::GetLocalizedSetting(const QString& original, emu_settings_type type, int index, bool strict) const
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
	case emu_settings_type::Resolution:
		switch (static_cast<video_resolution>(index))
		{
		case video_resolution::_1080p: return tr("1080p", "Resolution");
		case video_resolution::_1080i: return tr("1080i", "Resolution");
		case video_resolution::_720p: return tr("720p", "Resolution");
		case video_resolution::_480p: return tr("480p", "Resolution");
		case video_resolution::_480i: return tr("480i", "Resolution");
		case video_resolution::_576p: return tr("576p", "Resolution");
		case video_resolution::_576i: return tr("576i", "Resolution");
		case video_resolution::_1600x1080p: return tr("1600x1080p", "Resolution");
		case video_resolution::_1440x1080p: return tr("1440x1080p", "Resolution");
		case video_resolution::_1280x1080p: return tr("1280x1080p", "Resolution");
		case video_resolution::_960x1080p: return tr("960x1080p", "Resolution");
		}
		break;
	case emu_settings_type::FrameLimit:
		switch (static_cast<frame_limit_type>(index))
		{
		case frame_limit_type::none: return tr("Off", "Frame limit");
		case frame_limit_type::_30: return tr("30", "Frame limit");
		case frame_limit_type::_50: return tr("50", "Frame limit");
		case frame_limit_type::_60: return tr("60", "Frame limit");
		case frame_limit_type::_120: return tr("120", "Frame limit");
		case frame_limit_type::display_rate: return tr("Display", "Frame limit");
		case frame_limit_type::_auto: return tr("Auto", "Frame limit");
		case frame_limit_type::_ps3: return tr("PS3 Native", "Frame limit");
		case frame_limit_type::infinite: return tr("Infinite", "Frame limit");
		}
		break;
	case emu_settings_type::MSAA:
		switch (static_cast<msaa_level>(index))
		{
		case msaa_level::none: return tr("Disabled", "MSAA");
		case msaa_level::_auto: return tr("Auto", "MSAA");
		}
		break;
	case emu_settings_type::ShaderPrecisionQuality:
		switch (static_cast<gpu_preset_level>(index))
		{
		case gpu_preset_level::_auto: return tr("Auto", "Shader Precision");
		case gpu_preset_level::ultra: return tr("Ultra", "Shader Precision");
		case gpu_preset_level::high: return tr("High", "Shader Precision");
		case gpu_preset_level::low: return tr("Low", "Shader Precision");
		}
		break;
	case emu_settings_type::OutputScalingMode:
		switch (static_cast<output_scaling_mode>(index))
		{
		case output_scaling_mode::nearest: return tr("Nearest", "Output Scaling Mode");
		case output_scaling_mode::bilinear: return tr("Bilinear", "Output Scaling Mode");
		case output_scaling_mode::fsr: return tr("FidelityFX Super Resolution 1", "Output Scaling Mode");
		}
		break;
	case emu_settings_type::AudioRenderer:
		switch (static_cast<audio_renderer>(index))
		{
		case audio_renderer::null: return tr("Disable Audio Output", "Audio renderer");
#ifdef _WIN32
		case audio_renderer::xaudio: return tr("XAudio2", "Audio renderer");
#endif
		case audio_renderer::cubeb: return tr("Cubeb", "Audio renderer");
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
		case mouse_handler::raw: return tr("Raw", "Mouse handler");
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
	case emu_settings_type::CameraFlip:
		switch (static_cast<camera_flip>(index))
		{
		case camera_flip::none: return tr("No", "Camera flip");
		case camera_flip::horizontal: return tr("Flip horizontally", "Camera flip");
		case camera_flip::vertical: return tr("Flip vertically", "Camera flip");
		case camera_flip::both: return tr("Flip both axes", "Camera flip");
		}
		break;
	case emu_settings_type::Camera:
		switch (static_cast<camera_handler>(index))
		{
		case camera_handler::null: return tr("Null", "Camera handler");
		case camera_handler::fake: return tr("Fake", "Camera handler");
		case camera_handler::qt: return tr("Qt", "Camera handler");
		}
		break;
	case emu_settings_type::MusicHandler:
		switch (static_cast<music_handler>(index))
		{
		case music_handler::null: return tr("Null", "Music handler");
		case music_handler::qt: return tr("Qt", "Music handler");
		}
		break;
	case emu_settings_type::PadHandlerMode:
		switch (static_cast<pad_handler_mode>(index))
		{
		case pad_handler_mode::single_threaded: return tr("Single-threaded", "Pad handler mode");
		case pad_handler_mode::multi_threaded: return tr("Multi-threaded", "Pad handler mode");
		}
		break;
	case emu_settings_type::Move:
		switch (static_cast<move_handler>(index))
		{
		case move_handler::null: return tr("Null", "Move handler");
		case move_handler::real: return tr("Real", "Move handler");
		case move_handler::fake: return tr("Fake", "Move handler");
		case move_handler::mouse: return tr("Mouse", "Move handler");
		case move_handler::raw_mouse: return tr("Raw Mouse", "Move handler");
#ifdef HAVE_LIBEVDEV
		case move_handler::gun: return tr("Gun", "Gun handler");
#endif
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
	case emu_settings_type::Turntable:
		switch (static_cast<turntable_handler>(index))
		{
		case turntable_handler::null: return tr("Null", "Turntable handler");
		case turntable_handler::one_controller: return tr("1 controller", "Turntable handler");
		case turntable_handler::two_controllers: return tr("2 controllers", "Turntable handler");
		}
		break;
	case emu_settings_type::GHLtar:
		switch (static_cast<ghltar_handler>(index))
		{
		case ghltar_handler::null: return tr("Null", "GHLtar handler");
		case ghltar_handler::one_controller: return tr("1 controller", "GHLtar handler");
		case ghltar_handler::two_controllers: return tr("2 controllers", "GHLtar handler");
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
		case np_psn_status::psn_fake: return tr("Simulated", "PSN Status");
		case np_psn_status::psn_rpcn: return tr("RPCN", "PSN Status");
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
	case emu_settings_type::FIFOAccuracy:
		switch (static_cast<rsx_fifo_mode>(index))
		{
		case rsx_fifo_mode::fast: return tr("Fast", "RSX FIFO Accuracy");
		case rsx_fifo_mode::atomic: return tr("Atomic", "RSX FIFO Accuracy");
		case rsx_fifo_mode::atomic_ordered: return tr("Ordered & Atomic", "RSX FIFO Accuracy");
		case rsx_fifo_mode::as_ps3: return tr("PS3", "RSX FIFO Accuracy");
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
	case emu_settings_type::PerfOverlayFramerateDetailLevel:
	case emu_settings_type::PerfOverlayFrametimeDetailLevel:
		switch (static_cast<perf_graph_detail_level>(index))
		{
		case perf_graph_detail_level::minimal: return tr("Minimal", "Perf Graph Detail Level");
		case perf_graph_detail_level::show_min_max: return tr("Show Min And Max", "Perf Graph Detail Level");
		case perf_graph_detail_level::show_one_percent_avg: return tr("Show 1% Low And Average", "Perf Graph Detail Level");
		case perf_graph_detail_level::show_all: return tr("Show All", "Perf Graph Detail Level");
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
		case ppu_decoder_type::_static: return tr("Interpreter (static)", "PPU decoder");
		case ppu_decoder_type::llvm: return tr("Recompiler (LLVM)", "PPU decoder");
		}
		break;
	case emu_settings_type::SPUDecoder:
		switch (static_cast<spu_decoder_type>(index))
		{
		case spu_decoder_type::_static: return tr("Interpreter (static)", "SPU decoder");
		case spu_decoder_type::dynamic: return tr("Interpreter (dynamic)", "SPU decoder");
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
	case emu_settings_type::AudioFormat:
		switch (static_cast<audio_format>(index))
		{
		case audio_format::stereo: return tr("Stereo", "Audio format");
		case audio_format::surround_5_1: return tr("Surround 5.1", "Audio format");
		case audio_format::surround_7_1: return tr("Surround 7.1", "Audio format");
		case audio_format::manual: return tr("Manual", "Audio format");
		case audio_format::automatic: return tr("Automatic", "Audio format");
		}
		break;
	case emu_settings_type::AudioFormats:
		switch (static_cast<audio_format_flag>(index))
		{
		case audio_format_flag::lpcm_2_48khz: return tr("Linear PCM 2 Ch. 48 kHz", "Audio format flag");
		case audio_format_flag::lpcm_5_1_48khz: return tr("Linear PCM 5.1 Ch. 48 kHz", "Audio format flag");
		case audio_format_flag::lpcm_7_1_48khz: return tr("Linear PCM 7.1 Ch. 48 kHz", "Audio format flag");
		case audio_format_flag::ac3: return tr("Dolby Digital 5.1 Ch.", "Audio format flag");
		case audio_format_flag::dts: return tr("DTS 5.1 Ch.", "Audio format flag");
		}
		break;
	case emu_settings_type::AudioProvider:
		switch (static_cast<audio_provider>(index))
		{
		case audio_provider::none: return tr("None", "Audio Provider");
		case audio_provider::cell_audio: return tr("CellAudio", "Audio Provider");
		case audio_provider::rsxaudio: return tr("RSXAudio", "Audio Provider");
		}
		break;
	case emu_settings_type::AudioAvport:
		switch (static_cast<audio_avport>(index))
		{
		case audio_avport::hdmi_0: return tr("HDMI 0", "Audio Avport");
		case audio_avport::hdmi_1: return tr("HDMI 1", "Audio Avport");
		case audio_avport::avmulti: return tr("AV multiout", "Audio Avport");
		case audio_avport::spdif_0: return tr("SPDIF 0", "Audio Avport");
		case audio_avport::spdif_1: return tr("SPDIF 1", "Audio Avport");
		}
		break;
	case emu_settings_type::AudioChannelLayout:
		switch (static_cast<audio_channel_layout>(index))
		{
		case audio_channel_layout::automatic:        return tr("Auto", "Audio Channel Layout");
		case audio_channel_layout::mono:             return tr("Mono", "Audio Channel Layout");
		case audio_channel_layout::stereo:           return tr("Stereo", "Audio Channel Layout");
		case audio_channel_layout::stereo_lfe:       return tr("Stereo LFE", "Audio Channel Layout");
		case audio_channel_layout::quadraphonic:     return tr("Quadraphonic", "Audio Channel Layout");
		case audio_channel_layout::quadraphonic_lfe: return tr("Quadraphonic LFE", "Audio Channel Layout");
		case audio_channel_layout::surround_5_1:     return tr("Surround 5.1", "Audio Channel Layout");
		case audio_channel_layout::surround_7_1:     return tr("Surround 7.1", "Audio Channel Layout");
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
	case emu_settings_type::VulkanAsyncSchedulerDriver:
		switch (static_cast<vk_gpu_scheduler_mode>(index))
		{
		case vk_gpu_scheduler_mode::safe: return tr("Safe", "Asynchronous Queue Scheduler");
		case vk_gpu_scheduler_mode::fast: return tr("Fast", "Asynchronous Queue Scheduler");
		}
		break;
	case emu_settings_type::DateFormat:
		switch (static_cast<date_format>(index))
		{
		case date_format::yyyymmdd: return tr("Year/Month/Day", "Date Format");
		case date_format::ddmmyyyy: return tr("Day/Month/Year", "Date Format");
		case date_format::mmddyyyy: return tr("Month/Day/Year", "Date Format");
		}
		break;
	case emu_settings_type::TimeFormat:
		switch (static_cast<time_format>(index))
		{
		case time_format::clock12: return tr("12-hour clock", "Time Format");
		case time_format::clock24: return tr("24-hour clock", "Time Format");
		}
		break;
	case emu_settings_type::Language:
		switch (static_cast<CellSysutilLang>(index))
		{
		case CELL_SYSUTIL_LANG_JAPANESE: return tr("Japanese", "System Language");
		case CELL_SYSUTIL_LANG_ENGLISH_US: return tr("English (US)", "System Language");
		case CELL_SYSUTIL_LANG_FRENCH: return tr("French", "System Language");
		case CELL_SYSUTIL_LANG_SPANISH: return tr("Spanish", "System Language");
		case CELL_SYSUTIL_LANG_GERMAN: return tr("German", "System Language");
		case CELL_SYSUTIL_LANG_ITALIAN: return tr("Italian", "System Language");
		case CELL_SYSUTIL_LANG_DUTCH: return tr("Dutch", "System Language");
		case CELL_SYSUTIL_LANG_PORTUGUESE_PT: return tr("Portuguese (Portugal)", "System Language");
		case CELL_SYSUTIL_LANG_RUSSIAN: return tr("Russian", "System Language");
		case CELL_SYSUTIL_LANG_KOREAN: return tr("Korean", "System Language");
		case CELL_SYSUTIL_LANG_CHINESE_T: return tr("Chinese (Traditional)", "System Language");
		case CELL_SYSUTIL_LANG_CHINESE_S: return tr("Chinese (Simplified)", "System Language");
		case CELL_SYSUTIL_LANG_FINNISH: return tr("Finnish", "System Language");
		case CELL_SYSUTIL_LANG_SWEDISH: return tr("Swedish", "System Language");
		case CELL_SYSUTIL_LANG_DANISH: return tr("Danish", "System Language");
		case CELL_SYSUTIL_LANG_NORWEGIAN: return tr("Norwegian", "System Language");
		case CELL_SYSUTIL_LANG_POLISH: return tr("Polish", "System Language");
		case CELL_SYSUTIL_LANG_ENGLISH_GB: return tr("English (UK)", "System Language");
		case CELL_SYSUTIL_LANG_PORTUGUESE_BR: return tr("Portuguese (Brazil)", "System Language");
		case CELL_SYSUTIL_LANG_TURKISH: return tr("Turkish", "System Language");
		default:
			break;
		}
		break;
	case emu_settings_type::KeyboardType:
		switch (static_cast<CellKbMappingType>(index))
		{
		case CELL_KB_MAPPING_101: return tr("English keyboard (US standard)", "Keyboard Type");
		case CELL_KB_MAPPING_106: return tr("Japanese keyboard", "Keyboard Type");
		case CELL_KB_MAPPING_106_KANA: return tr("Japanese keyboard (Kana state)", "Keyboard Type");
		case CELL_KB_MAPPING_GERMAN_GERMANY: return tr("German keyboard", "Keyboard Type");
		case CELL_KB_MAPPING_SPANISH_SPAIN: return tr("Spanish keyboard", "Keyboard Type");
		case CELL_KB_MAPPING_FRENCH_FRANCE: return tr("French keyboard", "Keyboard Type");
		case CELL_KB_MAPPING_ITALIAN_ITALY: return tr("Italian keyboard", "Keyboard Type");
		case CELL_KB_MAPPING_DUTCH_NETHERLANDS: return tr("Dutch keyboard", "Keyboard Type");
		case CELL_KB_MAPPING_PORTUGUESE_PORTUGAL: return tr("Portuguese keyboard (Portugal)", "Keyboard Type");
		case CELL_KB_MAPPING_RUSSIAN_RUSSIA: return tr("Russian keyboard", "Keyboard Type");
		case CELL_KB_MAPPING_ENGLISH_UK: return tr("English keyboard (UK standard)", "Keyboard Type");
		case CELL_KB_MAPPING_KOREAN_KOREA: return tr("Korean keyboard", "Keyboard Type");
		case CELL_KB_MAPPING_NORWEGIAN_NORWAY: return tr("Norwegian keyboard", "Keyboard Type");
		case CELL_KB_MAPPING_FINNISH_FINLAND: return tr("Finnish keyboard", "Keyboard Type");
		case CELL_KB_MAPPING_DANISH_DENMARK: return tr("Danish keyboard", "Keyboard Type");
		case CELL_KB_MAPPING_SWEDISH_SWEDEN: return tr("Swedish keyboard", "Keyboard Type");
		case CELL_KB_MAPPING_CHINESE_TRADITIONAL: return tr("Chinese keyboard (Traditional)", "Keyboard Type");
		case CELL_KB_MAPPING_CHINESE_SIMPLIFIED: return tr("Chinese keyboard (Simplified)", "Keyboard Type");
		case CELL_KB_MAPPING_SWISS_FRENCH_SWITZERLAND: return tr("French keyboard (Switzerland)", "Keyboard Type");
		case CELL_KB_MAPPING_SWISS_GERMAN_SWITZERLAND: return tr("German keyboard (Switzerland)", "Keyboard Type");
		case CELL_KB_MAPPING_CANADIAN_FRENCH_CANADA: return tr("French keyboard (Canada)", "Keyboard Type");
		case CELL_KB_MAPPING_BELGIAN_BELGIUM: return tr("French keyboard (Belgium)", "Keyboard Type");
		case CELL_KB_MAPPING_POLISH_POLAND: return tr("Polish keyboard", "Keyboard Type");
		case CELL_KB_MAPPING_PORTUGUESE_BRAZIL: return tr("Portuguese keyboard (Brazil)", "Keyboard Type");
		case CELL_KB_MAPPING_TURKISH_TURKEY: return tr("Turkish keyboard", "Keyboard Type");
		}
		break;
	case emu_settings_type::ExclusiveFullscreenMode:
		switch (static_cast<vk_exclusive_fs_mode>(index))
		{
		case vk_exclusive_fs_mode::unspecified: return tr("Automatic (Default)", "Exclusive Fullscreen Mode");
		case vk_exclusive_fs_mode::disable: return tr("Prefer borderless fullscreen", "Exclusive Fullscreen Mode");
		case vk_exclusive_fs_mode::enable: return tr("Prefer exclusive fullscreen", "Exclusive Fullscreen Mode");
		}
		break;
	case emu_settings_type::StereoRenderMode:
		switch (static_cast<stereo_render_mode_options>(index))
		{
		case stereo_render_mode_options::disabled: return tr("Disabled", "3D Display Mode");
		case stereo_render_mode_options::side_by_side: return tr("Side-by-side", "3D Display Mode");
		case stereo_render_mode_options::over_under: return tr("Over-under", "3D Display Mode");
		case stereo_render_mode_options::interlaced: return tr("Interlaced", "3D Display Mode");
		case stereo_render_mode_options::anaglyph_red_green: return tr("Anaglyph Red-Green", "3D Display Mode");
		case stereo_render_mode_options::anaglyph_red_blue: return tr("Anaglyph Red-Blue", "3D Display Mode");
		case stereo_render_mode_options::anaglyph_red_cyan: return tr("Anaglyph Red-Cyan", "3D Display Mode");
		case stereo_render_mode_options::anaglyph_magenta_cyan: return tr("Anaglyph Magenta-Cyan", "3D Display Mode");
		case stereo_render_mode_options::anaglyph_trioscopic: return tr("Anaglyph Green-Magenta (Trioscopic)", "3D Display Mode");
		case stereo_render_mode_options::anaglyph_amber_blue: return tr("Anaglyph Amber-Blue (ColorCode 3D)", "3D Display Mode");
		}
		break;
	case emu_settings_type::MidiDevices:
		switch (static_cast<midi_device_type>(index))
		{
		case midi_device_type::guitar: return tr("Guitar (17 frets)", "Midi Device Type");
		case midi_device_type::guitar_22fret: return tr("Guitar (22 frets)", "Midi Device Type");
		case midi_device_type::keyboard: return tr("Keyboard", "Midi Device Type");
		case midi_device_type::drums: return tr("Drums", "Midi Device Type");
		}
		break;
	case emu_settings_type::XFloatAccuracy:
		switch (static_cast<xfloat_accuracy>(index))
		{
		case xfloat_accuracy::accurate: return tr("Accurate XFloat");
		case xfloat_accuracy::approximate: return tr("Approximate XFloat");
		case xfloat_accuracy::relaxed: return tr("Relaxed XFloat");
		case xfloat_accuracy::inaccurate: return tr("Inaccurate XFloat");
		}
		break;
	default:
		break;
	}

	if (strict)
	{
		std::string type_string;
		if (const auto it = settings_location.find(type); it != settings_location.cend())
		{
			for (const char* loc : it->second)
			{
				if (!type_string.empty()) type_string += ": ";
				type_string += loc;
			}
		}
		fmt::throw_exception("Missing translation for emu setting (original=%s, type='%s'=%d, index=%d)", original, type_string.empty() ? "?" : type_string, static_cast<int>(type), index);
	}

	return original;
}

std::string emu_settings::GetLocalizedSetting(const std::string& original, emu_settings_type type, int index, bool strict) const
{
	return GetLocalizedSetting(QString::fromStdString(original), type, index, strict).toStdString();
}

std::string emu_settings::GetLocalizedSetting(const cfg::_base* node, u32 index) const
{
	const emu_settings_type type = FindSettingsType(node);
	const std::vector<std::string> settings = GetSettingOptions(type);
	return GetLocalizedSetting(::at32(settings, index), type, index, true);
}
