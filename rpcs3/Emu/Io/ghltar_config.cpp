#include "stdafx.h"
#include "ghltar_config.h"

LOG_CHANNEL(ghltar_log, "GHLTAR");

std::optional<ghltar_btn> cfg_ghltar::find_button(u32 offset, u32 keycode) const
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

bool cfg_ghltars::load()
{
	bool result = false;
	const std::string cfg_name = fs::get_config_dir() + "config/ghltar.yml";
	ghltar_log.notice("Loading ghltar config: %s", cfg_name);

	from_default();

	for (cfg_ghltar* player : players)
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

	for (cfg_ghltar* player : players)
	{
		const auto set_button = [&player](pad_button pbtn, ghltar_btn bbtn)
		{
			const u32 offset = pad_button_offset(pbtn);
			const u32 keycode = pad_button_keycode(pbtn);
			player->buttons[(offset >> 8) & 0xFF][keycode & 0xFF] = bbtn;
		};
		set_button(player->w1, ghltar_btn::w1);
		set_button(player->w2, ghltar_btn::w2);
		set_button(player->w3, ghltar_btn::w3);
		set_button(player->b1, ghltar_btn::b1);
		set_button(player->b2, ghltar_btn::b2);
		set_button(player->b3, ghltar_btn::b3);
		set_button(player->start, ghltar_btn::start);
		set_button(player->hero_power, ghltar_btn::hero_power);
		set_button(player->ghtv, ghltar_btn::ghtv);
		set_button(player->strum_down, ghltar_btn::strum_down);
		set_button(player->strum_up, ghltar_btn::strum_up);
		set_button(player->dpad_left, ghltar_btn::dpad_left);
		set_button(player->dpad_right, ghltar_btn::dpad_right);
	}

	return result;
}

void cfg_ghltars::save() const
{
	const std::string cfg_name = fs::get_config_dir() + "config/ghltar.yml";
	ghltar_log.notice("Saving ghltar config to '%s'", cfg_name);

	if (!fs::create_path(fs::get_parent_dir(cfg_name)))
	{
		ghltar_log.fatal("Failed to create path: %s (%s)", cfg_name, fs::g_tls_error);
	}

	fs::pending_file cfg_file(cfg_name);

	if (!cfg_file.file || (cfg_file.file.write(to_string()), !cfg_file.commit()))
	{
		ghltar_log.error("Failed to save ghltar config to '%s' (error=%s)", cfg_name, fs::g_tls_error);
	}
}
