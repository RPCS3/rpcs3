#include "stdafx.h"
#include "pad_config.h"
#include "Emu/system_utils.hpp"
#include "Emu/Io/PadHandler.h"

extern std::string g_input_config_override;

std::vector<pad::combo> cfg_pad::get_combos(std::string_view button_string)
{
	if (button_string.empty())
		return {};

	// Handle special case: string contains separator itself as configured value (it's why I don't use fmt::split here)
	const auto split = [](std::string_view str, char sep)
	{
		std::set<std::string> buttons;
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
					buttons.insert(std::string(str.substr(btn_start, i - btn_start)));
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
				buttons.insert(std::string(str.substr(btn_start, i - btn_start + 1)));
			}
		}

		return buttons;
	};

	std::vector<pad::combo> combos;

	// Get all combos (seperated by ',')
	const std::set<std::string> combo_strings = split(button_string, ',');

	for (const std::string& combo_string : combo_strings)
	{
		// Get all keys for this combo (seperated by '&')
		std::set<std::string> combo = split(combo_string, '&');
		if (!combo.empty())
		{
			combos.push_back(pad::combo{std::move(combo)});
		}
	}

	return combos;
}

std::string cfg_pad::get_button_string(std::vector<pad::combo>& combos)
{
	std::vector<std::string> combo_strings;

	// Remove duplicates
	std::sort(combos.begin(), combos.end());
	combos.erase(std::unique(combos.begin(), combos.end()), combos.end());

	for (const pad::combo& combo : combos)
	{
		// Merge all keys for this combo (seperated by '&')
		combo_strings.push_back(combo.to_string());
	}

	// Merge combos (seperated by ',')
	return fmt::merge(combo_strings, ",");
}

std::string cfg_pad::make_button_string(const std::unordered_map<u32, std::string>& button_list, const std::vector<std::set<u32>>& button_combos)
{
	std::vector<pad::combo> combos;

	for (const std::set<u32>& button_combo : button_combos)
	{
		if (button_combo.empty()) continue;

		pad::combo combo {};

		for (u32 button : button_combo)
		{
			combo.add_button(::at32(button_list, button));
		}

		combos.push_back(std::move(combo));
	}

	return get_button_string(combos);
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

// Check if a config node still contains nothing but the current defaults
static bool is_default_config(const cfg::node& node, const cfg::_base* ignored_0 = nullptr, const cfg::_base* ignored_1 = nullptr)
{
	for (const cfg::_base* entry : node.get_nodes())
	{
		if (entry == ignored_0 || entry == ignored_1)
		{
			continue;
		}

		if (entry->get_type() == cfg::type::node)
		{
			if (!is_default_config(static_cast<const cfg::node&>(*entry)))
			{
				return false;
			}
		}
		else if (entry->to_string() != entry->def_to_string())
		{
			return false;
		}
	}

	return true;
}

// Copy all values of a config node to another node of the exact same type
static void copy_config(cfg::node& dst, const cfg::node& src)
{
	const std::vector<cfg::_base*>& dst_entries = dst.get_nodes();
	const std::vector<cfg::_base*>& src_entries = src.get_nodes();

	ensure(dst_entries.size() == src_entries.size());

	for (usz i = 0; i < src_entries.size(); i++)
	{
		cfg::_base* dst_entry = dst_entries[i];
		const cfg::_base* src_entry = src_entries[i];

		ensure(dst_entry->get_type() == src_entry->get_type());

		if (src_entry->get_type() == cfg::type::node)
		{
			copy_config(static_cast<cfg::node&>(*dst_entry), static_cast<const cfg::node&>(*src_entry));
		}
		else
		{
			dst_entry->from_string(src_entry->to_string());
		}
	}
}

cfg_saved_config* cfg_saved_configs::find(pad_handler type) const
{
	for (const std::unique_ptr<cfg_saved_config>& config : m_configs)
	{
		if (config->type == type)
		{
			return config.get();
		}
	}

	return nullptr;
}

cfg_saved_config& cfg_saved_configs::add(pad_handler type)
{
	if (cfg_saved_config* config = find(type))
	{
		return *config;
	}

	// The new config registers itself as a child node of this node
	return *m_configs.emplace_back(std::make_unique<cfg_saved_config>(this, fmt::format("%s", type), type));
}

void cfg_saved_configs::remove(pad_handler type)
{
	for (auto it = m_configs.cbegin(); it != m_configs.cend(); it++)
	{
		if ((*it)->type == type)
		{
			remove_node(it->get());
			m_configs.erase(it);
			return;
		}
	}
}

void cfg_saved_configs::from_default()
{
	for (const std::unique_ptr<cfg_saved_config>& config : m_configs)
	{
		remove_node(config.get());
	}

	m_configs.clear();
}

cfg::_base* cfg_saved_configs::create_node(const std::string& name)
{
	u64 type{};

	if (!cfg::try_to_enum_value(&type, &fmt_class_string<pad_handler>::format, name, get_name()))
	{
		return nullptr;
	}

	const pad_handler handler_type = static_cast<pad_handler>(type);

	// There is never a saved config for the null handler
	if (handler_type == pad_handler::null)
	{
		return nullptr;
	}

	return &add(handler_type);
}

void cfg_player::backup_config(std::string_view default_device)
{
	// There is nothing worth saving for the null handler
	if (handler.get() == pad_handler::null)
	{
		return;
	}

	const std::string current_device = device.to_string();
	const std::string current_buddy_device = buddy_device.to_string();

	// A device the user picked has to be kept even if nothing else was configured
	const bool has_default_device = current_device.empty() || current_device == default_device || current_device == device.def;
	const bool has_default_buddy_device = current_buddy_device.empty() || current_buddy_device == buddy_device.def;

	// Don't waste any space on a config that the user never touched.
	// The device identity says nothing about that: the pad dialog always writes the ids of a concrete product,
	// and the standard device class only contains one, so picking another one always changes the class as well.
	if (has_default_device && has_default_buddy_device && is_default_config(config, &config.vendor_id, &config.product_id))
	{
		saved_configs.remove(handler.get());
		return;
	}

	input_log.notice("Saving input configuration of handler '%s' for later use", handler.to_string());

	cfg_saved_config& saved_config = saved_configs.add(handler.get());
	saved_config.device.from_string(current_device);
	saved_config.buddy_device.from_string(current_buddy_device);
	copy_config(saved_config.config, config);
}

bool cfg_player::restore_config(pad_handler type)
{
	cfg_saved_config* saved_config = saved_configs.find(type);

	if (!saved_config)
	{
		return false;
	}

	input_log.notice("Restoring saved input configuration of handler '%s'", type);

	device.from_string(saved_config->device.to_string());
	buddy_device.from_string(saved_config->buddy_device.to_string());
	copy_config(config, saved_config->config);

	// The saved config is active again, so we don't need the backup anymore
	saved_configs.remove(type);

	return true;
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
