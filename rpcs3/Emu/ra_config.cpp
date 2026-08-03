#include "stdafx.h"
#include "ra_config.h"
#include "Utilities/File.h"

LOG_CHANNEL(ra_cfg_log, "RA");

cfg_ra g_cfg_ra;

std::string cfg_ra::get_path()
{
	return fs::get_config_dir(true) + "ra.yml";
}

void cfg_ra::load()
{
	const std::string path = cfg_ra::get_path();
	if (fs::file cfg_file(path, fs::read); cfg_file)
		from_string(cfg_file.to_string());
	else
		from_default();
}

void cfg_ra::save() const
{
	const std::string path = cfg_ra::get_path();
	if (!cfg::node::save(path))
		ra_cfg_log.error("Could not save config: %s", path, fs::g_tls_error);
}
