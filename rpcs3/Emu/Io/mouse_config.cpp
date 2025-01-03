#include "stdafx.h"
#include "mouse_config.h"
#include "MouseHandler.h"
#include "Utilities/File.h"

mouse_config::mouse_config()
	: cfg_name(fs::get_config_dir(true) + "config_mouse.yml")
{
}

bool mouse_config::exist() const
{
	return fs::is_file(cfg_name);
}

bool mouse_config::load()
{
	g_cfg_mouse.from_default();

	if (fs::file cfg_file{cfg_name, fs::read})
	{
		if (const std::string content = cfg_file.to_string(); !content.empty())
		{
			return from_string(content);
		}
	}

	return false;
}

void mouse_config::save()
{
	fs::pending_file file(cfg_name);

	if (file.file)
	{
		file.file.write(to_string());
		file.commit();
	}

	reload_requested = true;
}

cfg::string& mouse_config::get_button(int code)
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
