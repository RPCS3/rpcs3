#include "emu_settings.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include "stdafx.h"
#include "Emu/System.h"
#include "Utilities/Config.h"
#include "Utilities/Thread.h"
#include "Utilities/StrUtil.h"

#include <QMessageBox>

#if defined(_WIN32) || defined(HAVE_VULKAN)
#include "Emu/RSX/VK/VKHelpers.h"
#endif

#include "3rdparty/OpenAL/include/alext.h"

LOG_CHANNEL(cfg_log, "CFG");

extern std::string g_cfg_defaults; //! Default settings grabbed from Utilities/Config.h

inline std::string sstr(const QString& _in) { return _in.toStdString(); }
inline std::string sstr(const QVariant& _in) { return sstr(_in.toString()); }

// Emit sorted YAML
namespace
{
	static NEVER_INLINE void emitData(YAML::Emitter& out, const YAML::Node& node)
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

// Helper methods to interact with YAML and the config settings.
namespace cfg_adapter
{
	static cfg::_base& get_cfg(cfg::_base& root, const std::string& name)
	{
		if (root.get_type() == cfg::type::node)
		{
			for (const auto& pair : static_cast<cfg::node&>(root).get_nodes())
			{
				if (pair.first == name)
				{
					return *pair.second;
				}
			}
		}

		fmt::throw_exception("Node not found: %s", name);
	}

	static cfg::_base& get_cfg(cfg::_base& root, cfg_location::const_iterator begin, cfg_location::const_iterator end)
	{
		return begin == end ? root : get_cfg(get_cfg(root, *begin), begin + 1, end);
	}

	static YAML::Node get_node(const YAML::Node& node, cfg_location::const_iterator begin, cfg_location::const_iterator end)
	{
		return begin == end ? node : get_node(node[*begin], begin + 1, end); // TODO
	}

	/** Syntactic sugar to get a setting at a given config location. */
	static YAML::Node get_node(const YAML::Node& node, cfg_location loc)
	{
		return get_node(node, loc.cbegin(), loc.cend());
	}
}

/** Returns possible options for values for some particular setting.*/
static QStringList getOptions(cfg_location location)
{
	QStringList values;
	auto begin = location.cbegin();
	auto end = location.cend();
	for (const auto& v : cfg_adapter::get_cfg(g_cfg, begin, end).to_list())
	{
		values.append(qstr(v));
	}
	return values;
}

emu_settings::Render_Creator::Render_Creator()
{
#if defined(WIN32) || defined(HAVE_VULKAN)
	// Some drivers can get stuck when checking for vulkan-compatible gpus, f.ex. if they're waiting for one to get
	// plugged in. This whole contraption is for showing an error message in case that happens, so that user has
	// some idea about why the emulator window isn't showing up.

	static std::atomic<bool> was_called = false;
	if (was_called.exchange(true))
		fmt::throw_exception("Render_Creator cannot be created more than once" HERE);

	static std::mutex mtx;
	static std::condition_variable cond;
	static bool thread_running = true;
	static bool device_found = false;

	static QStringList compatible_gpus;

	std::thread enum_thread = std::thread([&]
	{
		thread_ctrl::set_native_priority(-1);

		vk::context device_enum_context;
		if (device_enum_context.createInstance("RPCS3", true))
		{
			device_enum_context.makeCurrentInstance();
			std::vector<vk::physical_device> &gpus = device_enum_context.enumerateDevices();

			if (!gpus.empty())
			{
				device_found = true;

				for (auto &gpu : gpus)
				{
					compatible_gpus.append(qstr(gpu.get_name()));
				}
			}
		}

		std::scoped_lock{mtx}, thread_running = false;
		cond.notify_all();
	});

	{
		std::unique_lock lck(mtx);
		cond.wait_for(lck, std::chrono::seconds(10), [&]{ return !thread_running; });
	}

	if (thread_running)
	{
		cfg_log.error("Vulkan device enumeration timed out");
		auto button = QMessageBox::critical(nullptr, tr("Vulkan Check Timeout"),
			tr("Querying for Vulkan-compatible devices is taking too long. This is usually caused by malfunctioning "
			"graphics drivers, reinstalling them could fix the issue.\n\n"
			"Selecting ignore starts the emulator without Vulkan support."),
			QMessageBox::Ignore | QMessageBox::Abort, QMessageBox::Abort);

		enum_thread.detach();
		if (button != QMessageBox::Ignore)
			std::exit(1);

		supportsVulkan = false;
	}
	else
	{
		supportsVulkan = device_found;
		vulkanAdapters = std::move(compatible_gpus);
		enum_thread.join();
	}
#endif

	// Graphics Adapter
	Vulkan = Render_Info(name_Vulkan, vulkanAdapters, supportsVulkan, emu_settings::VulkanAdapter, true);
	OpenGL = Render_Info(name_OpenGL);
	NullRender = Render_Info(name_Null);

	renderers = { &Vulkan, &OpenGL, &NullRender };
}

emu_settings::Microphone_Creator::Microphone_Creator()
{
	RefreshList();
}

void emu_settings::Microphone_Creator::RefreshList()
{
	microphones_list.clear();
	microphones_list.append(mic_none);

	if (alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT") == AL_TRUE)
	{
		const char *devices = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);

		while (*devices != 0)
		{
			microphones_list.append(qstr(devices));
			devices += strlen(devices) + 1;
		}
	}
	else
	{
		// Without enumeration we can only use one device
		microphones_list.append(qstr(alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER)));
	}
}

std::string emu_settings::Microphone_Creator::SetDevice(u32 num, QString& text)
{
	if (text == mic_none)
		sel_list[num-1] = "";
	else
		sel_list[num-1] = text.toStdString();

	const std::string final_list = sel_list[0] + "@@@" + sel_list[1] + "@@@" + sel_list[2] + "@@@" + sel_list[3] + "@@@";
	return final_list;
}

void emu_settings::Microphone_Creator::ParseDevices(std::string list)
{
	for (u32 index = 0; index < 4; index++)
	{
		sel_list[index] = "";
	}

	const auto devices_list = fmt::split(list, { "@@@" });
	for (u32 index = 0; index < std::min<u32>(4, ::size32(devices_list)); index++)
	{
		sel_list[index] = devices_list[index];
	}
}

emu_settings::emu_settings() : QObject()
{
}

emu_settings::~emu_settings()
{
}

void emu_settings::LoadSettings(const std::string& title_id)
{
	m_title_id = title_id;

	// Create config path if necessary
	fs::create_path(title_id.empty() ? fs::get_config_dir() : Emulator::GetCustomConfigDir());

	// Load default config
	m_defaultSettings = YAML::Load(g_cfg_defaults);
	m_currentSettings = YAML::Load(g_cfg_defaults);

	// Add global config
	fs::file config(fs::get_config_dir() + "/config.yml", fs::read + fs::write + fs::create);
	m_currentSettings += YAML::Load(config.to_string());
	config.close();

	// Add game config
	if (!title_id.empty())
	{
		const std::string config_path_new = Emulator::GetCustomConfigPath(m_title_id);
		const std::string config_path_old = Emulator::GetCustomConfigPath(m_title_id, true);

		if (fs::is_file(config_path_new))
		{
			config = fs::file(config_path_new, fs::read + fs::write);
			m_currentSettings += YAML::Load(config.to_string());
			config.close();
		}
		else if (fs::is_file(config_path_old))
		{
			config = fs::file(config_path_old, fs::read + fs::write);
			m_currentSettings += YAML::Load(config.to_string());
			config.close();
		}
	}
}

void emu_settings::SaveSettings()
{
	fs::file config;
	YAML::Emitter out;
	emitData(out, m_currentSettings);

	std::string config_name;

	if (m_title_id.empty())
	{
		config_name = fs::get_config_dir() + "/config.yml";
	}
	else
	{
		config_name = Emulator::GetCustomConfigPath(m_title_id);
	}

	config = fs::file(config_name, fs::read + fs::write + fs::create);

	// Save config
	config.seek(0);
	config.trunc(0);
	config.write(out.c_str(), out.size());

	// Check if the running config/title is the same as the edited config/title.
	if (config_name == g_cfg.name || m_title_id == Emu.GetTitleID())
	{
		// Update current config
		g_cfg.from_string(config.to_string(), true);

		if (!Emu.IsStopped()) // Don't spam the log while emulation is stopped. The config will be logged on boot anyway.
		{
			cfg_log.notice("Updated configuration:\n%s\n", g_cfg.to_string());
		}
	}

	config.close();
}

void emu_settings::EnhanceComboBox(QComboBox* combobox, SettingsType type, bool is_ranged, bool use_max, int max, bool sorted)
{
	if (!combobox)
	{
		cfg_log.fatal("EnhanceComboBox '%s' was used with an invalid object", GetSettingName(type));
		return;
	}

	if (is_ranged)
	{
		if (sorted)
		{
			cfg_log.warning("EnhanceCombobox '%s': ignoring sorting request on ranged combo box", GetSettingName(type));
		}

		QStringList range = GetSettingOptions(type);

		int max_item = use_max ? max : range.last().toInt();

		for (int i = range.first().toInt(); i <= max_item; i++)
		{
			combobox->addItem(QString::number(i), QVariant(QString::number(i)));
		}
	}
	else
	{
		QStringList settings = GetSettingOptions(type);

		if (sorted)
		{
			settings.sort();
		}

		for (QString setting : settings)
		{
			combobox->addItem(tr(setting.toStdString().c_str()), QVariant(setting));
		}
	}

	std::string selected = GetSetting(type);
	int index = combobox->findData(qstr(selected));

	if (index == -1)
	{
		std::string def = GetSettingDefault(type);
		cfg_log.fatal("EnhanceComboBox '%s' tried to set an invalid value: %s. Setting to default: %s", GetSettingName(type), selected, def);
		combobox->setCurrentIndex(combobox->findData(qstr(def)));
		m_broken_types.insert(type);
	}
	else
	{
		combobox->setCurrentIndex(index);
	}

	connect(combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int index)
	{
		SetSetting(type, sstr(combobox->itemData(index)));
	});
}

void emu_settings::EnhanceCheckBox(QCheckBox* checkbox, SettingsType type)
{
	if (!checkbox)
	{
		cfg_log.fatal("EnhanceCheckBox '%s' was used with an invalid object", GetSettingName(type));
		return;
	}

	std::string def = GetSettingDefault(type);
	std::transform(def.begin(), def.end(), def.begin(), ::tolower);

	if (def != "true" && def != "false")
	{
		cfg_log.fatal("EnhanceCheckBox '%s' was used with an invalid SettingsType", GetSettingName(type));
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
		cfg_log.fatal("EnhanceCheckBox '%s' tried to set an invalid value: %s. Setting to default: %s", GetSettingName(type), selected, def);
		checkbox->setChecked(def == "true");
		m_broken_types.insert(type);
	}

	connect(checkbox, &QCheckBox::stateChanged, [=](int val)
	{
		std::string str = val != 0 ? "true" : "false";
		SetSetting(type, str);
	});
}

void emu_settings::EnhanceSlider(QSlider* slider, SettingsType type)
{
	if (!slider)
	{
		cfg_log.fatal("EnhanceSlider '%s' was used with an invalid object", GetSettingName(type));
		return;
	}

	QStringList range = GetSettingOptions(type);
	bool ok_def, ok_sel, ok_min, ok_max;

	int def = qstr(GetSettingDefault(type)).toInt(&ok_def);
	int min = range.first().toInt(&ok_min);
	int max = range.last().toInt(&ok_max);

	if (!ok_def || !ok_min || !ok_max)
	{
		cfg_log.fatal("EnhanceSlider '%s' was used with an invalid SettingsType", GetSettingName(type));
		return;
	}

	QString selected = qstr(GetSetting(type));
	int val = selected.toInt(&ok_sel);

	if (!ok_sel || val < min || val > max)
	{
		cfg_log.fatal("EnhanceSlider '%s' tried to set an invalid value: %d. Setting to default: %d. Allowed range: [%d, %d]", GetSettingName(type), val, def, min, max);
		val = def;
		m_broken_types.insert(type);
	}

	slider->setRange(min, max);
	slider->setValue(val);

	connect(slider, &QSlider::valueChanged, [=](int value)
	{
		SetSetting(type, sstr(value));
	});
}

void emu_settings::EnhanceSpinBox(QSpinBox* spinbox, SettingsType type, const QString& prefix, const QString& suffix)
{
	if (!spinbox)
	{
		cfg_log.fatal("EnhanceSpinBox '%s' was used with an invalid object", GetSettingName(type));
		return;
	}

	QStringList range = GetSettingOptions(type);
	bool ok_def, ok_sel, ok_min, ok_max;

	int def = qstr(GetSettingDefault(type)).toInt(&ok_def);
	int min = range.first().toInt(&ok_min);
	int max = range.last().toInt(&ok_max);

	if (!ok_def || !ok_min || !ok_max)
	{
		cfg_log.fatal("EnhanceSpinBox '%s' was used with an invalid type", GetSettingName(type));
		return;
	}

	std::string selected = GetSetting(type);
	int val = qstr(selected).toInt(&ok_sel);

	if (!ok_sel || val < min || val > max)
	{
		cfg_log.fatal("EnhanceSpinBox '%s' tried to set an invalid value: %d. Setting to default: %d. Allowed range: [%d, %d]", GetSettingName(type), selected, def, min, max);
		val = def;
		m_broken_types.insert(type);
	}

	spinbox->setPrefix(prefix);
	spinbox->setSuffix(suffix);
	spinbox->setRange(min, max);
	spinbox->setValue(val);

#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	connect(spinbox, &QSpinBox::textChanged, [=](const QString&/* text*/)
#else
	connect(spinbox, QOverload<const QString &>::of(&QSpinBox::valueChanged), [=](const QString&/* value*/)
#endif
	{
		SetSetting(type, sstr(spinbox->cleanText()));
	});
}

void emu_settings::EnhanceDoubleSpinBox(QDoubleSpinBox* spinbox, SettingsType type, const QString& prefix, const QString& suffix)
{
	if (!spinbox)
	{
		cfg_log.fatal("EnhanceDoubleSpinBox '%s' was used with an invalid object", GetSettingName(type));
		return;
	}

	QStringList range = GetSettingOptions(type);
	bool ok_def, ok_sel, ok_min, ok_max;

	double def = qstr(GetSettingDefault(type)).toDouble(&ok_def);
	double min = range.first().toDouble(&ok_min);
	double max = range.last().toDouble(&ok_max);

	if (!ok_def || !ok_min || !ok_max)
	{
		cfg_log.fatal("EnhanceDoubleSpinBox '%s' was used with an invalid type", GetSettingName(type));
		return;
	}

	std::string selected = GetSetting(type);
	double val = qstr(selected).toDouble(&ok_sel);

	if (!ok_sel || val < min || val > max)
	{
		cfg_log.fatal("EnhanceDoubleSpinBox '%s' tried to set an invalid value: %f. Setting to default: %f. Allowed range: [%f, %f]", GetSettingName(type), val, def, min, max);
		val = def;
		m_broken_types.insert(type);
	}

	spinbox->setPrefix(prefix);
	spinbox->setSuffix(suffix);
	spinbox->setRange(min, max);
	spinbox->setValue(val);

#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	connect(spinbox, &QDoubleSpinBox::textChanged, [=](const QString&/* text*/)
#else
	connect(spinbox, QOverload<const QString &>::of(&QDoubleSpinBox::valueChanged), [=](const QString&/* value*/)
#endif
	{
		SetSetting(type, sstr(spinbox->cleanText()));
	});
}

std::vector<std::string> emu_settings::GetLoadedLibraries()
{
	return m_currentSettings["Core"]["Load libraries"].as<std::vector<std::string>, std::initializer_list<std::string>>({});
}

void emu_settings::SaveSelectedLibraries(const std::vector<std::string>& libs)
{
	m_currentSettings["Core"]["Load libraries"] = libs;
}

QStringList emu_settings::GetSettingOptions(SettingsType type) const
{
	return getOptions(const_cast<cfg_location&&>(SettingsLoc[type]));
}

std::string emu_settings::GetSettingName(SettingsType type) const
{
	cfg_location loc = SettingsLoc[type];
	return loc[loc.size() - 1];
}

std::string emu_settings::GetSettingDefault(SettingsType type) const
{
	return cfg_adapter::get_node(m_defaultSettings, SettingsLoc[type]).Scalar();
}

std::string emu_settings::GetSetting(SettingsType type) const
{
	return cfg_adapter::get_node(m_currentSettings, SettingsLoc[type]).Scalar();
}

void emu_settings::SetSetting(SettingsType type, const std::string& val)
{
	cfg_adapter::get_node(m_currentSettings, SettingsLoc[type]) = val;
}

void emu_settings::OpenCorrectionDialog(QWidget* parent)
{
	if (m_broken_types.size() && QMessageBox::question(parent, tr("Fix invalid settings?"),
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
			std::string def = GetSettingDefault(type);
			std::string old = GetSetting(type);
			SetSetting(type, def);
			cfg_log.success("The config entry '%s' was corrected from '%s' to '%s'", GetSettingName(type), old, def);
		}

		m_broken_types.clear();
		cfg_log.success("You need to save the settings in order to make these changes permanent!");
	}
}
