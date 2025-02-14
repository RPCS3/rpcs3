#include "stdafx.h"
#include "rb3drums_config.h"

LOG_CHANNEL(cfg_log, "CFG");

cfg_rb3drums g_cfg_rb3drums;

cfg_rb3drums::cfg_rb3drums()
	: cfg::node()
	, path(fs::get_config_dir(true) + "rb3drums.yml")
{
}

bool cfg_rb3drums::load()
{
	cfg_log.notice("Loading rb3drums config from '%s'", path);

	if (fs::file cfg_file{path, fs::read})
	{
		return from_string(cfg_file.to_string());
	}

	cfg_log.notice("No rb3drums config found. Using default settings. Path: %s", path);
	from_default();
	save();
	return false;
}

void cfg_rb3drums::save()
{
	cfg_log.notice("Saving rb3drums config to '%s'", path);

	if (!cfg::node::save(path))
	{
		cfg_log.error("Failed to save rb3drums config to '%s' (error=%s)", path, fs::g_tls_error);
	}

	reload_requested = true;
}
