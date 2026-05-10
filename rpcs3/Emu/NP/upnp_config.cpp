#include "stdafx.h"
#include "upnp_config.h"
#include "Utilities/File.h"

LOG_CHANNEL(upnp_cfg_log, "UPNP_CFG");

void cfg_upnp::load()
{
	const std::string path = cfg_upnp::get_path();

	fs::file cfg_file(path, fs::read);
	if (cfg_file)
	{
		upnp_cfg_log.notice("Loading UPNP config. Path: %s", path);
		from_string(cfg_file.to_string());
	}
	else
	{
		upnp_cfg_log.notice("UPNP config missing. Using default settings. Path: %s", path);
		from_default();
	}
}

void cfg_upnp::save() const
{
#ifdef _WIN32
	const std::string path_to_cfg = fs::get_config_dir(true);
	if (!fs::create_path(path_to_cfg))
	{
		upnp_cfg_log.error("Could not create path: %s", path_to_cfg);
	}
#endif

	const std::string path = cfg_upnp::get_path();

	if (!cfg::node::save(path))
	{
		upnp_cfg_log.error("Could not save config: %s (error=%s)", path, fs::g_tls_error);
	}
}

std::string cfg_upnp::get_device_url() const
{
	return device_url.to_string();
}

void cfg_upnp::set_device_url(std::string_view url)
{
	device_url.from_string(url);
}

std::string cfg_upnp::get_path()
{
	return fs::get_config_dir(true) + "upnp.yml";
}
