#include "stdafx.h"
#include "recording_config.h"
#include <charconv>

LOG_CHANNEL(cfg_log, "CFG");

cfg_recording g_cfg_recording;

cfg_recording::cfg_recording()
	: cfg::node()
#ifdef _WIN32
	, path(fs::get_config_dir() + "config/recording.yml")
#else
	, path(fs::get_config_dir() + "recording.yml")
#endif
{
}

bool cfg_recording::load()
{
	cfg_log.notice("Loading recording config from '%s'", path);

	if (fs::file cfg_file{path, fs::read})
	{
		return from_string(cfg_file.to_string());
	}

	cfg_log.notice("Recording config missing. Using default settings. Path: %s", path);
	from_default();
	save();
	return false;
}

void cfg_recording::save() const
{
	cfg_log.notice("Saving recording config to '%s'", path);

	fs::pending_file cfg_file(path);

	if (!cfg_file.file || (cfg_file.file.write(to_string()), !cfg_file.commit()))
	{
		cfg_log.error("Failed to save recording config to '%s' (error=%s)", path, fs::g_tls_error);
	}
}
