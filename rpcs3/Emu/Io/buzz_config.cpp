#include "stdafx.h"
#include "buzz_config.h"

LOG_CHANNEL(buzz_log, "BUZZ");

std::optional<buzz_btn> cfg_buzzer::find_button(u32 offset, u32 keycode) const
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

bool cfg_buzz::load()
{
	bool result = false;
	const std::string cfg_name = fs::get_config_dir() + "config/buzz.yml";
	buzz_log.notice("Loading buzz config: %s", cfg_name);

	from_default();

	for (cfg_buzzer* player : players)
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

	for (cfg_buzzer* player : players)
	{
		const auto set_button = [&player](pad_button pbtn, buzz_btn bbtn)
		{
			const u32 offset = pad_button_offset(pbtn);
			const u32 keycode = pad_button_keycode(pbtn);
			player->buttons[(offset >> 8) & 0xFF][keycode & 0xFF] = bbtn;
		};
		set_button(player->red, buzz_btn::red);
		set_button(player->yellow, buzz_btn::yellow);
		set_button(player->green, buzz_btn::green);
		set_button(player->orange, buzz_btn::orange);
		set_button(player->blue, buzz_btn::blue);
	}

	return result;
}

void cfg_buzz::save() const
{
	const std::string cfg_name = fs::get_config_dir() + "config/buzz.yml";
	buzz_log.notice("Saving buzz config to '%s'", cfg_name);

	if (!fs::create_path(fs::get_parent_dir(cfg_name)))
	{
		buzz_log.fatal("Failed to create path: %s (%s)", cfg_name, fs::g_tls_error);
	}

	fs::pending_file cfg_file(cfg_name);

	if (!cfg_file.file || (cfg_file.file.write(to_string()), !cfg_file.commit()))
	{
		buzz_log.error("Failed to save buzz config to '%s' (error=%s)", cfg_name, fs::g_tls_error);
	}
}
