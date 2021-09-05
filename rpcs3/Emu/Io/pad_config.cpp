#include "stdafx.h"
#include "pad_config.h"
#include "Emu/system_utils.hpp"

LOG_CHANNEL(input_log, "Input");

bool cfg_input::load(const std::string& title_id, const std::string& profile, bool strict)
{
	input_log.notice("Loading pad config (title_id='%s', profile='%s', strict=%d)", title_id, profile, strict);

	std::string cfg_name;

	// Check custom config first
	if (!title_id.empty())
	{
		cfg_name = rpcs3::utils::get_custom_input_config_path(title_id);
	}

	// Check active global profile next
	if ((title_id.empty() || !strict) && !fs::is_file(cfg_name))
	{
		cfg_name = rpcs3::utils::get_input_config_dir() + profile + ".yml";
	}

	// Fallback to default profile
	if (!strict && !fs::is_file(cfg_name))
	{
		cfg_name = rpcs3::utils::get_input_config_dir() + g_cfg_profile.default_profile + ".yml";
	}

	from_default();

	if (fs::file cfg_file{ cfg_name, fs::read })
	{
		input_log.notice("Loading pad profile: '%s'", cfg_name);

		if (std::string content = cfg_file.to_string(); !content.empty())
		{
			return from_string(content);
		}
	}

	// Add keyboard by default
	input_log.notice("Pad profile empty. Adding default keyboard pad handler");
	player[0]->handler.from_string(fmt::format("%s", pad_handler::keyboard));
	player[0]->device.from_string(pad::keyboard_device_name.data());

	return false;
}

void cfg_input::save(const std::string& title_id, const std::string& profile) const
{
	std::string cfg_name;

	if (title_id.empty())
	{
		cfg_name = rpcs3::utils::get_input_config_dir() + profile + ".yml";
		input_log.notice("Saving pad config profile '%s' to '%s'", profile, cfg_name);
	}
	else
	{
		cfg_name = rpcs3::utils::get_custom_input_config_path(title_id);
		input_log.notice("Saving custom pad config for '%s' to '%s'", title_id, cfg_name);
	}

	if (!fs::create_path(fs::get_parent_dir(cfg_name)))
	{
		input_log.fatal("Failed to create path: %s (%s)", cfg_name, fs::g_tls_error);
	}

	if (auto cfg_file = fs::file(cfg_name, fs::rewrite))
	{
		cfg_file.write(to_string());
	}
	else
	{
		input_log.error("Failed to save pad config to '%s'", cfg_name);
	}
}

cfg_profile::cfg_profile()
	: path(rpcs3::utils::get_input_config_root() + "/active_profiles.yml")
{
}

bool cfg_profile::load()
{
	if (fs::file cfg_file{ path, fs::read })
	{
		return from_string(cfg_file.to_string());
	}

	from_default();
	return false;
}

void cfg_profile::save() const
{
	input_log.notice("Saving pad profile config to '%s'", path);

	if (auto cfg_file = fs::file(path, fs::rewrite))
	{
		cfg_file.write(to_string());
	}
	else
	{
		input_log.error("Failed to save pad profile config to '%s'", path);
	}
}
