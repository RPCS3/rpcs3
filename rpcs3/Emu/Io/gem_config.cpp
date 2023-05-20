#include "stdafx.h"
#include "gem_config.h"

LOG_CHANNEL(cellGem);

std::optional<gem_btn> cfg_gem::find_button(u32 offset, u32 keycode) const
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

bool cfg_gems::load()
{
	bool result = false;
	const std::string cfg_name = fs::get_config_dir() + "config/gem.yml";
	cellGem.notice("Loading gem config: %s", cfg_name);

	from_default();

	for (cfg_gem* player : players)
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

	for (cfg_gem* player : players)
	{
		const auto set_button = [&player](pad_button pbtn, gem_btn bbtn)
		{
			const u32 offset = pad_button_offset(pbtn);
			const u32 keycode = pad_button_keycode(pbtn);
			player->buttons[(offset >> 8) & 0xFF][keycode & 0xFF] = bbtn;
		};
		set_button(player->start, gem_btn::start);
		set_button(player->select, gem_btn::select);
		set_button(player->triangle, gem_btn::triangle);
		set_button(player->circle, gem_btn::circle);
		set_button(player->cross, gem_btn::cross);
		set_button(player->square, gem_btn::square);
		set_button(player->move, gem_btn::move);
		set_button(player->t, gem_btn::t);
	}

	return result;
}

void cfg_gems::save() const
{
	const std::string cfg_name = fs::get_config_dir() + "config/gem.yml";
	cellGem.notice("Saving gem config to '%s'", cfg_name);

	if (!fs::create_path(fs::get_parent_dir(cfg_name)))
	{
		cellGem.fatal("Failed to create path: %s (%s)", cfg_name, fs::g_tls_error);
	}

	fs::pending_file cfg_file(cfg_name);

	if (!cfg_file.file || (cfg_file.file.write(to_string()), !cfg_file.commit()))
	{
		cellGem.error("Failed to save gem config to '%s' (error=%s)", cfg_name, fs::g_tls_error);
	}
}
