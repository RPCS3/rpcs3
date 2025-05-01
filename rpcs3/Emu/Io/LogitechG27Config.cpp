#include "stdafx.h"

#ifdef HAVE_SDL3

#include "Utilities/File.h"
#include "LogitechG27Config.h"

emulated_logitech_g27_config g_cfg_logitech_g27;

LOG_CHANNEL(cfg_log, "CFG");

void emulated_logitech_g27_config::from_default()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	cfg::node::from_default();
}

void emulated_logitech_g27_config::save()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	const std::string cfg_name = fmt::format("%s%s.yml", fs::get_config_dir(true), "LogitechG27");
	cfg_log.notice("Saving LogitechG27 config: %s", cfg_name);

	if (!fs::create_path(fs::get_parent_dir(cfg_name)))
	{
		cfg_log.fatal("Failed to create path: %s (%s)", cfg_name, fs::g_tls_error);
	}

	if (!cfg::node::save(cfg_name))
	{
		cfg_log.error("Failed to save LogitechG27 config to '%s' (error=%s)", cfg_name, fs::g_tls_error);
	}
}

bool emulated_logitech_g27_config::load()
{
	bool result = false;
	const std::string cfg_name = fmt::format("%s%s.yml", fs::get_config_dir(true), "LogitechG27");
	cfg_log.notice("Loading LogitechG27 config: %s", cfg_name);

	from_default();

	m_mutex.lock();

	if (fs::file cfg_file{cfg_name, fs::read})
	{
		if (const std::string content = cfg_file.to_string(); !content.empty())
		{
			result = from_string(content);
		}
		m_mutex.unlock();
	}
	else
	{
		m_mutex.unlock();
		save();
	}

	return result;
}

#endif
