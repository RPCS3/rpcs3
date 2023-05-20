#include "stdafx.h"
#include "turntable_config.h"

LOG_CHANNEL(turntable_log, "TURN");

std::optional<turntable_btn> cfg_turntable::find_button(u32 offset, u32 keycode) const
{
	if (const auto it = buttons.find(offset); it != buttons.cend())
	{
		if (const auto it2 = it->second.find(keycode); it2 != it->second.cend())
		{
			return it2->second;
		}
	}

	return std::nullopt;
}

bool cfg_turntables::load()
{
	bool result = false;
	const std::string cfg_name = fs::get_config_dir() + "config/turntable.yml";
	turntable_log.notice("Loading turntable config: %s", cfg_name);

	from_default();

	for (cfg_turntable* player : players)
	{
		player->buttons.clear();
	}

	if (fs::file cfg_file{ cfg_name, fs::read })
	{
		if (std::string content = cfg_file.to_string(); !content.empty())
		{
			result = from_string(content);
		}
	}
	else
	{
		save();
	}

	for (cfg_turntable* player : players)
	{
		const auto set_button = [&player](pad_button pbtn, turntable_btn bbtn)
		{
			const u32 offset = pad_button_offset(pbtn);
			const u32 keycode = pad_button_keycode(pbtn);
			player->buttons[(offset >> 8) & 0xFF][keycode & 0xFF] = bbtn;
		};
		set_button(player->red, turntable_btn::red);
		set_button(player->green, turntable_btn::green);
		set_button(player->blue, turntable_btn::blue);
		set_button(player->dpad_up, turntable_btn::dpad_up);
		set_button(player->dpad_down, turntable_btn::dpad_down);
		set_button(player->dpad_left, turntable_btn::dpad_left);
		set_button(player->dpad_right, turntable_btn::dpad_right);
		set_button(player->start, turntable_btn::start);
		set_button(player->select, turntable_btn::select);
		set_button(player->square, turntable_btn::square);
		set_button(player->circle, turntable_btn::circle);
		set_button(player->cross, turntable_btn::cross);
		set_button(player->triangle, turntable_btn::triangle);
	}

	return result;
}

void cfg_turntables::save() const
{
	const std::string cfg_name = fs::get_config_dir() + "config/turntable.yml";
	turntable_log.notice("Saving turntable config to '%s'", cfg_name);

	if (!fs::create_path(fs::get_parent_dir(cfg_name)))
	{
		turntable_log.fatal("Failed to create path: %s (%s)", cfg_name, fs::g_tls_error);
	}

	fs::pending_file cfg_file(cfg_name);

	if (!cfg_file.file || (cfg_file.file.write(to_string()), !cfg_file.commit()))
	{
		turntable_log.error("Failed to save turntable config to '%s' (error=%s)", cfg_name, fs::g_tls_error);
	}
}
