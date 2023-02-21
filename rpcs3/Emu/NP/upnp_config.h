#pragma once

#include "Utilities/Config.h"

struct cfg_upnp : cfg::node
{
	cfg::string device_url{this, "DeviceUrl", ""};

	void load();
	void save() const;

	std::string get_device_url() const;

	void set_device_url(std::string_view url);

private:
	static std::string get_path();
};
