#include "emu_settings.h"

#include "stdafx.h"
#include "Emu/System.h"
#include "Utilities/Config.h"

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

emu_settings::emu_settings(const std::string& path) : QObject()
{
	currentSettings = YAML::Load(g_cfg_defaults);

	// Create config path if necessary
	fs::create_path(fs::get_config_dir() + path);

	// Incrementally load config.yml
	config = fs::file(fs::get_config_dir() + path + "/config.yml", fs::read + fs::write + fs::create);

	if (config.size() == 0 && !path.empty()) // First time
	{
		config = fs::file(fs::get_config_dir() + "/config.yml", fs::read + fs::write + fs::create);
		currentSettings += YAML::Load(config.to_string());
	}
	else
	{
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
	
	// Save config
	config.seek(0);
	config.trunc(0);
	config.write(out.c_str(), out.size());
}

QComboBox* emu_settings::CreateEnhancedComboBox(SettingsType type, QWidget* parent)
{
	QComboBox* box = new QComboBox(parent);

	for (QString setting : GetSettingOptions(type))
	{
		box->addItem(tr(setting.toStdString().c_str()), QVariant(setting));
	}

	QString selected = qstr(GetSetting(type));
	int index = box->findData(selected);
	if (index == -1)
	{
		LOG_WARNING(GENERAL, "Current setting not found while creating combobox");
	}
	else
	{
		box->setCurrentIndex(index);
	}

	connect(box, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int index) {
		SetSetting(type, sstr(box->itemData(index)));
	});

	return box;
}

QCheckBox* emu_settings::CreateEnhancedCheckBox(SettingsType type, QWidget* parent)
{
	cfg_location loc = SettingsLoc[type];
	std::string name = loc[loc.size()-1];
	QCheckBox* settingsButton = new QCheckBox(tr(name.c_str()), parent);

	std::string currSet = GetSetting(type);
	if (currSet == "true")
	{
		settingsButton->setChecked(true);
	}
	else if (currSet != "false")
	{
		LOG_WARNING(GENERAL, "Passed in an invalid setting for creating enhanced checkbox");
	}
	connect(settingsButton, &QCheckBox::stateChanged, [=](int val) {
		std::string str = val != 0 ? "true" : "false";
		SetSetting(type, str);
	});
	return settingsButton;
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
	cfg_adapter::get_node(currentSettings, SettingsLoc[type])= val;
}
