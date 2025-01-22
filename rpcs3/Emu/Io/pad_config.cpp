#include "stdafx.h"
#include "pad_config.h"
#include "Emu/system_utils.hpp"

LOG_CHANNEL(input_log, "Input");

extern std::string g_input_config_override;

std::vector<std::string> cfg_pad::get_buttons(const std::string& str)
{
	std::vector<std::string> vec = fmt::split(str, {","});

	// Handle special case: string contains separator itself as configured value
	if (str == "," || str.find(",,") != umax)
	{
		vec.push_back(",");
	}

	// Remove duplicates
	std::sort(vec.begin(), vec.end());
	vec.erase(std::unique(vec.begin(), vec.end()), vec.end());

	return vec;
}

std::string cfg_pad::get_buttons(std::vector<std::string> vec)
{
	// Remove duplicates
	std::sort(vec.begin(), vec.end());
	vec.erase(std::unique(vec.begin(), vec.end()), vec.end());

	return fmt::merge(vec, ",");
}

u8 cfg_pad::get_large_motor_speed(const std::array<VibrateMotor, 2>& motor_speed) const
{
	const u8 idx = switch_vibration_motors ? 1 : 0;
	const f32 multiplier = multiplier_vibration_motor_large / 100.0f;
	return static_cast<u8>(std::clamp(motor_speed[idx].m_value * multiplier, 0.0f, 255.0f));
}

u8 cfg_pad::get_small_motor_speed(const std::array<VibrateMotor, 2>& motor_speed) const
{
	const u8 idx = switch_vibration_motors ? 0 : 1;
	const f32 multiplier = multiplier_vibration_motor_small / 100.0f;
	return static_cast<u8>(std::clamp(motor_speed[idx].m_value * multiplier, 0.0f, 255.0f));
}

bool cfg_input::load(const std::string& title_id, const std::string& config_file, bool strict)
{
	input_log.notice("Loading pad config (title_id='%s', config_file='%s', strict=%d)", title_id, config_file, strict);

	std::string cfg_name;

	// Check configuration override first
	if (!strict && !g_input_config_override.empty())
	{
		cfg_name = rpcs3::utils::get_input_config_dir() + g_input_config_override + ".yml";
	}

	// Check custom config next
	if (!title_id.empty() && !fs::is_file(cfg_name))
	{
		cfg_name = rpcs3::utils::get_custom_input_config_path(title_id);
	}

	// Check active global configuration next
	if ((title_id.empty() || !strict) && !config_file.empty() && !fs::is_file(cfg_name))
	{
		cfg_name = rpcs3::utils::get_input_config_dir() + config_file + ".yml";
	}

	// Fallback to default configuration
	if (!strict && !fs::is_file(cfg_name))
	{
		cfg_name = rpcs3::utils::get_input_config_dir() + g_cfg_input_configs.default_config + ".yml";
	}

	from_default();

	if (fs::file cfg_file{ cfg_name, fs::read })
	{
		input_log.notice("Loading input configuration: '%s'", cfg_name);

		if (const std::string content = cfg_file.to_string(); !content.empty())
		{
			return from_string(content);
		}
	}

	// Add keyboard by default
	input_log.notice("Input configuration empty. Adding default keyboard pad handler");
	player[0]->handler.from_string(fmt::format("%s", pad_handler::keyboard));
	player[0]->device.from_string(pad::keyboard_device_name.data());
	player[0]->buddy_device.from_string(""sv);

	return false;
}

void cfg_input::save(const std::string& title_id, const std::string& config_file) const
{
	std::string cfg_name;

	if (title_id.empty())
	{
		cfg_name = rpcs3::utils::get_input_config_dir() + config_file + ".yml";
		input_log.notice("Saving input configuration '%s' to '%s'", config_file, cfg_name);
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

	if (!cfg::node::save(cfg_name))
	{
		input_log.error("Failed to save pad config to '%s' (error=%s)", cfg_name, fs::g_tls_error);
	}
}

cfg_input_configurations::cfg_input_configurations()
	: path(rpcs3::utils::get_input_config_root() + "/active_input_configurations.yml")
{
}

bool cfg_input_configurations::load()
{
	if (fs::file cfg_file{ path, fs::read })
	{
		return from_string(cfg_file.to_string());
	}

	from_default();
	return false;
}

void cfg_input_configurations::save() const
{
	input_log.notice("Saving input configurations config to '%s'", path);

	if (!cfg::node::save(path))
	{
		input_log.error("Failed to save input configurations config to '%s' (error=%s)", path, fs::g_tls_error);
	}
}
