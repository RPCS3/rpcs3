#include "emu_settings.h"

#include "stdafx.h"
#include "Emu/System.h"
#include "Utilities/Config.h"

#include <QMessageBox>

#ifdef _MSC_VER
#include <Windows.h>
#undef GetHwnd
#include <d3d12.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#endif

#if defined(_WIN32) || defined(HAVE_VULKAN)
#include "Emu/RSX/VK/VKHelpers.h"
#endif

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
};

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
	// check for dx12 adapters
#ifdef _MSC_VER
	HMODULE D3D12Module = LoadLibrary(L"d3d12.dll");

	if (D3D12Module != NULL)
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter;
		Microsoft::WRL::ComPtr<IDXGIFactory1> dxgi_factory;
		if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory))))
		{
			PFN_D3D12_CREATE_DEVICE wrapD3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(D3D12Module, "D3D12CreateDevice");
			if (wrapD3D12CreateDevice != nullptr)
			{
				for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != dxgi_factory->EnumAdapters1(adapterIndex, pAdapter.ReleaseAndGetAddressOf()); ++adapterIndex)
				{
					if (SUCCEEDED(wrapD3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
					{
						//A device with D3D12 support found. Init data
						supportsD3D12 = true;

						DXGI_ADAPTER_DESC desc;
						if (SUCCEEDED(pAdapter->GetDesc(&desc)))
							D3D12Adapters.append(QString::fromWCharArray(desc.Description));
					}
				}
			}
		}
	}
#endif

#if defined(WIN32) || defined(HAVE_VULKAN)
	// check for vulkan adapters
	vk::context device_enum_context;
	u32 instance_handle = device_enum_context.createInstance("RPCS3", true);

	if (instance_handle > 0)
	{
		device_enum_context.makeCurrentInstance(instance_handle);
		std::vector<vk::physical_device>& gpus = device_enum_context.enumerateDevices();

		if (gpus.size() > 0)
		{
			//A device with vulkan support found. Init data
			supportsVulkan = true;

			for (auto& gpu : gpus)
			{
				vulkanAdapters.append(qstr(gpu.name()));
			}
		}
	}
#endif

	// Graphics Adapter
	D3D12 = Render_Info(name_D3D12, D3D12Adapters, supportsD3D12, emu_settings::D3D12Adapter);
	Vulkan = Render_Info(name_Vulkan, vulkanAdapters, supportsVulkan, emu_settings::VulkanAdapter);
	OpenGL = Render_Info(name_OpenGL);
	NullRender = Render_Info(name_Null);

	renderers = { &D3D12, &Vulkan, &OpenGL, &NullRender };
}

emu_settings::emu_settings() : QObject()
{
}

emu_settings::~emu_settings()
{
}

void emu_settings::LoadSettings(const std::string& path)
{
	m_path = path;

	// Create config path if necessary
	fs::create_path(fs::get_config_dir() + path);

	// Load default config
	m_defaultSettings = YAML::Load(g_cfg_defaults);
	m_currentSettings = YAML::Load(g_cfg_defaults);

	// Add global config
	fs::file config(fs::get_config_dir() + "/config.yml", fs::read + fs::write + fs::create);
	m_currentSettings += YAML::Load(config.to_string());
	config.close();

	// Add game config
	if (!path.empty() && fs::is_file(fs::get_config_dir() + path + "/config.yml"))
	{
		config = fs::file(fs::get_config_dir() + path + "/config.yml", fs::read + fs::write);
		m_currentSettings += YAML::Load(config.to_string());
		config.close();
	}
}

void emu_settings::SaveSettings()
{
	fs::file config;
	YAML::Emitter out;
	emitData(out, m_currentSettings);

	if (!m_path.empty())
	{
		config = fs::file(fs::get_config_dir() + m_path + "/config.yml", fs::read + fs::write + fs::create);
	}
	else
	{
		config = fs::file(fs::get_config_dir() + "/config.yml", fs::read + fs::write + fs::create);
	}

	// Save config
	config.seek(0);
	config.trunc(0);
	config.write(out.c_str(), out.size());
	config.close();
}

void emu_settings::EnhanceComboBox(QComboBox* combobox, SettingsType type, bool is_ranged, bool use_max, int max)
{
	if (!combobox)
	{
		LOG_FATAL(GENERAL, "EnhanceComboBox '%s' was used with an invalid object", GetSettingName(type));
		return;
	}

	if (is_ranged)
	{
		QStringList range = GetSettingOptions(type);

		int max_item = use_max ? max : range.last().toInt();

		for (int i = range.first().toInt(); i <= max_item; i++)
		{
			combobox->addItem(QString::number(i), QVariant(QString::number(i)));
		}
	}
	else
	{
		for (QString setting : GetSettingOptions(type))
		{
			combobox->addItem(tr(setting.toStdString().c_str()), QVariant(setting));
		}
	}

	std::string selected = GetSetting(type);
	int index = combobox->findData(qstr(selected));

	if (index == -1)
	{
		std::string def = GetSettingDefault(type);
		LOG_ERROR(GENERAL, "EnhanceComboBox '%s' tried to set an invalid value: %s. Setting to default: %s", GetSettingName(type), selected, def);
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
		LOG_FATAL(GENERAL, "EnhanceCheckBox '%s' was used with an invalid object", GetSettingName(type));
		return;
	}

	std::string def = GetSettingDefault(type);
	std::transform(def.begin(), def.end(), def.begin(), ::tolower);

	if (def != "true" && def != "false")
	{
		LOG_FATAL(GENERAL, "EnhanceCheckBox '%s' was used with an invalid SettingsType", GetSettingName(type));
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
		LOG_ERROR(GENERAL, "EnhanceCheckBox '%s' tried to set an invalid value: %s. Setting to default: %s", GetSettingName(type), selected, def);
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
		LOG_FATAL(GENERAL, "EnhanceSlider '%s' was used with an invalid object", GetSettingName(type));
		return;
	}

	QStringList range = GetSettingOptions(type);
	bool ok_def, ok_sel, ok_min, ok_max;

	int def = qstr(GetSettingDefault(type)).toInt(&ok_def);
	int min = range.first().toInt(&ok_min);
	int max = range.last().toInt(&ok_max);

	if (!ok_def || !ok_min || !ok_max)
	{
		LOG_FATAL(GENERAL, "EnhanceSlider '%s' was used with an invalid SettingsType", GetSettingName(type));
		return;
	}

	QString selected = qstr(GetSetting(type));
	int val = selected.toInt(&ok_sel);

	if (!ok_sel || val < min || val > max)
	{
		LOG_ERROR(GENERAL, "EnhanceSlider '%s' tried to set an invalid value: %d. Setting to default: %d. Allowed range: [%d, %d]", GetSettingName(type), val, def, min, max);
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
		LOG_FATAL(GENERAL, "EnhanceSpinBox '%s' was used with an invalid object", GetSettingName(type));
		return;
	}

	QStringList range = GetSettingOptions(type);
	bool ok_def, ok_sel, ok_min, ok_max;

	int def = qstr(GetSettingDefault(type)).toInt(&ok_def);
	int min = range.first().toInt(&ok_min);
	int max = range.last().toInt(&ok_max);

	if (!ok_def || !ok_min || !ok_max)
	{
		LOG_FATAL(GENERAL, "EnhanceSpinBox '%s' was used with an invalid type", GetSettingName(type));
		return;
	}

	std::string selected = GetSetting(type);
	int val = qstr(selected).toInt(&ok_sel);

	if (!ok_sel || val < min || val > max)
	{
		LOG_ERROR(GENERAL, "EnhanceSpinBox '%s' tried to set an invalid value: %d. Setting to default: %d. Allowed range: [%d, %d]", GetSettingName(type), selected, def, min, max);
		val = def;
		m_broken_types.insert(type);
	}

	spinbox->setPrefix(prefix);
	spinbox->setSuffix(suffix);
	spinbox->setRange(min, max);
	spinbox->setValue(val);

	connect(spinbox, QOverload<const QString &>::of(&QSpinBox::valueChanged), [=](const QString&/* value*/)
	{
		SetSetting(type, sstr(spinbox->cleanText()));
	});
}

void emu_settings::EnhanceDoubleSpinBox(QDoubleSpinBox* spinbox, SettingsType type, const QString& prefix, const QString& suffix)
{
	if (!spinbox)
	{
		LOG_FATAL(GENERAL, "EnhanceDoubleSpinBox '%s' was used with an invalid object", GetSettingName(type));
		return;
	}

	QStringList range = GetSettingOptions(type);
	bool ok_def, ok_sel, ok_min, ok_max;

	double def = qstr(GetSettingDefault(type)).toDouble(&ok_def);
	double min = range.first().toDouble(&ok_min);
	double max = range.last().toDouble(&ok_max);

	if (!ok_def || !ok_min || !ok_max)
	{
		LOG_FATAL(GENERAL, "EnhanceDoubleSpinBox '%s' was used with an invalid type", GetSettingName(type));
		return;
	}

	std::string selected = GetSetting(type);
	double val = qstr(selected).toDouble(&ok_sel);

	if (!ok_sel || val < min || val > max)
	{
		LOG_ERROR(GENERAL, "EnhanceDoubleSpinBox '%s' tried to set an invalid value: %f. Setting to default: %f. Allowed range: [%f, %f]", GetSettingName(type), val, def, min, max);
		val = def;
		m_broken_types.insert(type);
	}

	spinbox->setPrefix(prefix);
	spinbox->setSuffix(suffix);
	spinbox->setRange(min, max);
	spinbox->setValue(val);

	connect(spinbox, QOverload<const QString &>::of(&QDoubleSpinBox::valueChanged), [=](const QString&/* value*/)
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
			"Your config file contained one or more unrecognized settings.\n"
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
			SetSetting(type, def);
			LOG_SUCCESS(GENERAL, "The config entry '%s' was corrected from '%s' to '%s'", GetSettingName(type), GetSetting(type), def);
		}

		m_broken_types.clear();
		LOG_SUCCESS(GENERAL, "You need to save the settings in order to make these changes permanent!");
	}
}
