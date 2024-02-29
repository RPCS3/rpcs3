#pragma once

#include "Utilities/Config.h"
#include "Utilities/mutex.h"

#include <array>

LOG_CHANNEL(cfg_log, "CFG");

struct raw_mouse_config : cfg::node
{
public:
	using cfg::node::node;

	cfg::_float<10, 1000> mouse_acceleration{ this, "Mouse Acceleration", 100.0f, true };
};

struct raw_mice_config : cfg::node
{
	raw_mice_config()
	{
		for (u32 i = 0; i < ::size32(players); i++)
		{
			players.at(i) = std::make_shared<raw_mouse_config>(this, fmt::format("Player %d", i + 1));
		}
	}

	shared_mutex m_mutex;
	static constexpr std::string_view cfg_id = "raw_mouse";
	std::array<std::shared_ptr<raw_mouse_config>, 4> players;

	bool load()
	{
		m_mutex.lock();

		bool result = false;
		const std::string cfg_name = fmt::format("%sconfig/%s.yml", fs::get_config_dir(), cfg_id);
		cfg_log.notice("Loading %s config: %s", cfg_id, cfg_name);

		from_default();

		if (fs::file cfg_file{ cfg_name, fs::read })
		{
			if (std::string content = cfg_file.to_string(); !content.empty())
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

	void save()
	{
		std::lock_guard lock(m_mutex);

		const std::string cfg_name = fmt::format("%sconfig/%s.yml", fs::get_config_dir(), cfg_id);
		cfg_log.notice("Saving %s config to '%s'", cfg_id, cfg_name);

		if (!fs::create_path(fs::get_parent_dir(cfg_name)))
		{
			cfg_log.fatal("Failed to create path: %s (%s)", cfg_name, fs::g_tls_error);
		}

		if (!cfg::node::save(cfg_name))
		{
			cfg_log.error("Failed to save %s config to '%s' (error=%s)", cfg_id, cfg_name, fs::g_tls_error);
		}
	}
};

extern raw_mice_config g_cfg_raw_mouse;
