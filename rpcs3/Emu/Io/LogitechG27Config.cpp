#include "stdafx.h"

#ifdef HAVE_SDL3

#include "Utilities/File.h"
#include "LogitechG27Config.h"

emulated_logitech_g27_config g_cfg_logitech_g27;

LOG_CHANNEL(cfg_log, "CFG");

emulated_logitech_g27_config::emulated_logitech_g27_config()
	: m_path(fmt::format("%s%s.yml", fs::get_config_dir(true), "LogitechG27"))
{
}

void emulated_logitech_g27_config::reset()
{
	const std::lock_guard<std::mutex> lock(m_mutex);
	cfg::node::from_default();
}

void emulated_logitech_g27_config::save(bool lock_mutex)
{
	std::unique_lock lock(m_mutex, std::defer_lock);
	if (lock_mutex)
	{
		lock.lock();
	}
	cfg_log.notice("Saving LogitechG27 config: '%s'", m_path);

	if (!fs::create_path(fs::get_parent_dir(m_path)))
	{
		cfg_log.fatal("Failed to create path: '%s' (%s)", m_path, fs::g_tls_error);
	}

	if (!cfg::node::save(m_path))
	{
		cfg_log.error("Failed to save LogitechG27 config to '%s' (error=%s)", m_path, fs::g_tls_error);
	}
}

bool emulated_logitech_g27_config::load()
{
	const std::lock_guard lock(m_mutex);

	cfg_log.notice("Loading LogitechG27 config: %s", m_path);

	from_default();

	if (fs::file cfg_file{m_path, fs::read})
	{
		if (const std::string content = cfg_file.to_string(); !content.empty())
		{
			return from_string(content);
		}
	}
	else
	{
		save(false);
	}

	return true;
}

#endif
