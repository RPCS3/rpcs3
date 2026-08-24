#pragma once
#include "util/yaml.hpp"
#include "Utilities/Config.h"
#include "util/logs.hpp"

struct cfg_ra : cfg::node
{
	cfg::_bool enabled{ this, "Enabled", false };
	cfg::_bool hardcore{ this, "Hardcore", false };
	cfg::string username{ this, "Username", "" };
	cfg::string token{ this, "Token", "" };

	static std::string get_path();
	void load();
	void save() const;
};

extern cfg_ra g_cfg_ra;
