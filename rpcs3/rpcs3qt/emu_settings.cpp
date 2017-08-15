#include "emu_settings.h"

#include "stdafx.h"
#include "Emu/System.h"
#include "Utilities/Config.h"

#ifdef _MSC_VER
#include <Windows.h>
#undef GetHwnd
#include <d3d12.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#endif

#if defined(_WIN32) || defined(__linux__)
#include "Emu/RSX/VK/VKHelpers.h"
#endif

extern std::string g_cfg_defaults; //! Default settings grabbed from Utilities/Config.h

inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }
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

Render_Creator::Render_Creator()
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

#if defined(WIN32) || defined(__linux__)
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
}

emu_settings::emu_settings(const std::string& path) : QObject()
{
	m_path = path;

	// Create config path if necessary
	fs::create_path(fs::get_config_dir() + path);

	// Load default config
	currentSettings = YAML::Load(g_cfg_defaults);

	// Add global config
	config = fs::file(fs::get_config_dir() + "/config.yml", fs::read + fs::write + fs::create);
	currentSettings += YAML::Load(config.to_string());

	// Add game config
	if (!path.empty() && fs::is_file(fs::get_config_dir() + path + "/config.yml"))
	{
		config = fs::file(fs::get_config_dir() + path + "/config.yml", fs::read + fs::write);
		currentSettings += YAML::Load(config.to_string());
	}
}

emu_settings::~emu_settings()
{
}

void emu_settings::SaveSettings()
 {
	YAML::Emitter out;
	emitData(out, currentSettings);

	if (!m_path.empty())
	{
		config = fs::file(fs::get_config_dir() + m_path + "/config.yml", fs::read + fs::write + fs::create);
	}

	// Save config
	config.seek(0);
	config.trunc(0);
	config.write(out.c_str(), out.size());
}

void emu_settings::EnhanceComboBox(QComboBox* combobox, SettingsType type, bool is_ranged)
{
	if (is_ranged)
	{
		QStringList range = GetSettingOptions(type);

		for (int i = range.first().toInt(); i <= range.last().toInt(); i++)
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

	QString selected = qstr(GetSetting(type));
	int index = combobox->findData(selected);
	if (index == -1)
	{
		LOG_WARNING(GENERAL, "Current setting not found while creating combobox");
	}
	else
	{
		combobox->setCurrentIndex(index);
	}

	connect(combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int index) {
		SetSetting(type, sstr(combobox->itemData(index)));
	});
}

void emu_settings::EnhanceCheckBox(QCheckBox* checkbox, SettingsType type)
{
	cfg_location loc = SettingsLoc[type];
	std::string name = loc[loc.size() - 1];

	std::string currSet = GetSetting(type);
	if (currSet == "true")
	{
		checkbox->setChecked(true);
	}
	else if (currSet != "false")
	{
		LOG_WARNING(GENERAL, "Passed in an invalid setting for creating enhanced checkbox");
	}
	connect(checkbox, &QCheckBox::stateChanged, [=](int val) {
		std::string str = val != 0 ? "true" : "false";
		SetSetting(type, str);
	});
}

std::vector<std::string> emu_settings::GetLoadedLibraries()
{
	return currentSettings["Core"]["Load libraries"].as<std::vector<std::string>, std::initializer_list<std::string>>({});
}

void emu_settings::SaveSelectedLibraries(const std::vector<std::string>& libs)
{
	currentSettings["Core"]["Load libraries"] = libs;
}

QStringList emu_settings::GetSettingOptions(SettingsType type) const
{
	return getOptions(const_cast<cfg_location&&>(SettingsLoc[type]));
}

std::string emu_settings::GetSetting(SettingsType type) const
{
	return cfg_adapter::get_node(currentSettings, SettingsLoc[type]).Scalar();
}

void emu_settings::SetSetting(SettingsType type, const std::string& val)
{
	cfg_adapter::get_node(currentSettings, SettingsLoc[type]) = val;
}
