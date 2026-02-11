#include "stdafx.h"
#include "wiimote_config.h"
#include "Utilities/File.h"

LOG_CHANNEL(wiimote_log, "Wiimote");

cfg_wiimote& get_wiimote_config()
{
	static cfg_wiimote instance;
	return instance;
}

cfg_wiimote::cfg_wiimote()
	: cfg::node()
	, path(fs::get_config_dir(true) + "wiimote.yml")
{
}

bool cfg_wiimote::load()
{
	if (fs::file f{path, fs::read})
	{
		return from_string(f.to_string());
	}
	return false;
}

void cfg_wiimote::save() const
{
	if (!cfg::node::save(path))
	{
		wiimote_log.error("Failed to save wiimote config to '%s'", path);
	}
}
