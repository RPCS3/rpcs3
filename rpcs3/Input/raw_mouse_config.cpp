#include "stdafx.h"
#include "raw_mouse_config.h"
#include "Emu/Io/MouseHandler.h"

std::string mouse_button_id(int code)
{
	switch (code)
	{
	case CELL_MOUSE_BUTTON_1: return "Button 1";
	case CELL_MOUSE_BUTTON_2: return "Button 2";
	case CELL_MOUSE_BUTTON_3: return "Button 3";
	case CELL_MOUSE_BUTTON_4: return "Button 4";
	case CELL_MOUSE_BUTTON_5: return "Button 5";
	case CELL_MOUSE_BUTTON_6: return "Button 6";
	case CELL_MOUSE_BUTTON_7: return "Button 7";
	case CELL_MOUSE_BUTTON_8: return "Button 8";
	}
	return "";
}

cfg::string& raw_mouse_config::get_button_by_index(int index)
{
	switch (index)
	{
	case 0: return mouse_button_1;
	case 1: return mouse_button_2;
	case 2: return mouse_button_3;
	case 3: return mouse_button_4;
	case 4: return mouse_button_5;
	case 5: return mouse_button_6;
	case 6: return mouse_button_7;
	case 7: return mouse_button_8;
	default: fmt::throw_exception("Invalid index %d", index);
	}
}

cfg::string& raw_mouse_config::get_button(int code)
{
	switch (code)
	{
	case CELL_MOUSE_BUTTON_1: return mouse_button_1;
	case CELL_MOUSE_BUTTON_2: return mouse_button_2;
	case CELL_MOUSE_BUTTON_3: return mouse_button_3;
	case CELL_MOUSE_BUTTON_4: return mouse_button_4;
	case CELL_MOUSE_BUTTON_5: return mouse_button_5;
	case CELL_MOUSE_BUTTON_6: return mouse_button_6;
	case CELL_MOUSE_BUTTON_7: return mouse_button_7;
	case CELL_MOUSE_BUTTON_8: return mouse_button_8;
	default: fmt::throw_exception("Invalid code %d", code);
	}
}

raw_mice_config::raw_mice_config()
{
	for (u32 i = 0; i < ::size32(players); i++)
	{
		players.at(i) = std::make_shared<raw_mouse_config>(this, fmt::format("Player %d", i + 1));
	}
}

bool raw_mice_config::load()
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

void raw_mice_config::save()
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
