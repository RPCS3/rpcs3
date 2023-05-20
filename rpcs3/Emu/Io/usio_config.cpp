#include "stdafx.h"
#include "usio_config.h"

LOG_CHANNEL(usio_log, "USIO");

std::optional<usio_btn> cfg_usio::find_button(u32 offset, u32 keycode) const
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

bool cfg_usios::load()
{
	bool result = false;
	const std::string cfg_name = fs::get_config_dir() + "config/usio.yml";
	usio_log.notice("Loading usio config: %s", cfg_name);

	from_default();

	for (cfg_usio* player : players)
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

	for (cfg_usio* player : players)
	{
		const auto set_button = [&player](pad_button pbtn, usio_btn bbtn)
		{
			const u32 offset = pad_button_offset(pbtn);
			const u32 keycode = pad_button_keycode(pbtn);
			player->buttons[(offset >> 8) & 0xFF][keycode & 0xFF] = bbtn;
		};
		set_button(player->test, usio_btn::test);
		set_button(player->coin, usio_btn::coin);
		set_button(player->enter, usio_btn::enter);
		set_button(player->up, usio_btn::up);
		set_button(player->down, usio_btn::down);
		set_button(player->service, usio_btn::service);
		set_button(player->strong_hit_side_left, usio_btn::strong_hit_side_left);
		set_button(player->strong_hit_side_right, usio_btn::strong_hit_side_right);
		set_button(player->strong_hit_center_left, usio_btn::strong_hit_center_left);
		set_button(player->strong_hit_center_right, usio_btn::strong_hit_center_right);
		set_button(player->small_hit_side_left, usio_btn::small_hit_side_left);
		set_button(player->small_hit_side_right, usio_btn::small_hit_side_right);
		set_button(player->small_hit_center_left, usio_btn::small_hit_center_left);
		set_button(player->small_hit_center_right, usio_btn::small_hit_center_right);
	}

	return result;
}

void cfg_usios::save() const
{
	const std::string cfg_name = fs::get_config_dir() + "config/usio.yml";
	usio_log.notice("Saving usio config to '%s'", cfg_name);

	if (!fs::create_path(fs::get_parent_dir(cfg_name)))
	{
		usio_log.fatal("Failed to create path: %s (%s)", cfg_name, fs::g_tls_error);
	}

	fs::pending_file cfg_file(cfg_name);

	if (!cfg_file.file || (cfg_file.file.write(to_string()), !cfg_file.commit()))
	{
		usio_log.error("Failed to save usio config to '%s' (error=%s)", cfg_name, fs::g_tls_error);
	}
}
