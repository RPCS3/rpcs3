#pragma once

#include <QStringList>

#include "emu_settings_type.h"
#include "util/yaml.hpp"

// Helper methods to interact with YAML and the config settings.
namespace cfg_adapter
{
	YAML::Node get_node(const YAML::Node& node, cfg_location::const_iterator begin, cfg_location::const_iterator end);

	/** Syntactic sugar to get a setting at a given config location. */
	YAML::Node get_node(const YAML::Node& node, cfg_location loc);

	/** Returns possible options for values for some particular setting.*/
	QStringList get_options(cfg_location location);

	/** Returns dynamic property for some particular setting.*/
	bool get_is_dynamic(emu_settings_type type);

	/** Returns the string for a given setting.*/
	std::string get_setting_name(emu_settings_type type);
}
