#include "stdafx.h"
#include "pad_config.h"
#include "Emu/system_utils.hpp"
#include "Emu/Io/PadHandler.h"

extern std::string g_input_config_override;

std::vector<std::vector<std::string>> cfg_pad::get_buttons(std::string_view str)
{
	if (str.empty())
		return {};

	// Handle special case: string contains separator itself as configured value (it's why I don't use fmt::split here)
	const auto split = [](std::string_view str, char sep)
	{
		std::vector<std::string> vec;
		bool was_sep = true;
		usz btn_start = 0ULL;
		usz i = 0ULL;

		for (; i < str.size(); i++)
		{
			const char c = str[i];

			if (c == sep)
			{
				if (!was_sep)
				{
					was_sep = true;
					vec.push_back(std::string(str.substr(btn_start, i - btn_start)));
					continue;
				}
			}

			if (was_sep)
			{
				was_sep = false;
				btn_start = i;
			}
		
			if (i == (str.size() - 1))
			{
				vec.push_back(std::string(str.substr(btn_start, i - btn_start + 1)));
			}
		}

		// Remove duplicates
		std::sort(vec.begin(), vec.end());
		vec.erase(std::unique(vec.begin(), vec.end()), vec.end());

		return vec;
	};

	std::vector<std::vector<std::string>> res;

	// Get all combos (seperated by ',')
	const std::vector<std::string> vec = split(str, ',');

	for (const std::string& combo : vec)
	{
		// Get all keys for this combo (seperated by '&')
		std::vector<std::string> keys = split(combo, '&');
		if (!keys.empty())
		{
			res.push_back(std::move(keys));
		}
	}

	return res;
}

std::string cfg_pad::get_buttons(std::vector<std::vector<std::string>> vec)
{
	std::vector<std::string> combos;

	// Remove duplicates
	std::sort(vec.begin(), vec.end());
	vec.erase(std::unique(vec.begin(), vec.end()), vec.end());

	for (std::vector<std::string>& combo : vec)
	{
		std::sort(combo.begin(), combo.end());
		combo.erase(std::unique(combo.begin(), combo.end()), combo.end());

		// Merge all keys for this combo (seperated by '&')
		combos.push_back(fmt::merge(combo, "&"));
	}

	// Merge combos (seperated by ',')
	return fmt::merge(combos, ",");
}

u8 cfg_pad::get_motor_speed(VibrateMotor& motor, f32 multiplier) const
{
	// If motor is small, use either 0 or 255.
	const u8 value = motor.is_large_motor ? motor.value : (motor.value > 0 ? 255 : 0);

	// Ignore lower range. Scale remaining range to full range.
	const f32 adjusted = PadHandlerBase::ScaledInput(value, static_cast<f32>(vibration_threshold.get()), 255.0f, 0.0f, 255.0f);

	// Apply multiplier
	motor.adjusted_value = static_cast<u8>(std::clamp(adjusted * multiplier, 0.0f, 255.0f));
	return motor.adjusted_value;
}

u8 cfg_pad::get_large_motor_speed(std::array<VibrateMotor, 2>& motors) const
{
	return get_motor_speed(motors[switch_vibration_motors ? 1 : 0], multiplier_vibration_motor_large / 100.0f);
}

u8 cfg_pad::get_small_motor_speed(std::array<VibrateMotor, 2>& motors) const
{
	return get_motor_speed(motors[switch_vibration_motors ? 0 : 1], multiplier_vibration_motor_small / 100.0f);
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
