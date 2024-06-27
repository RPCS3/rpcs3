#include "stdafx.h"
#include "mouse_config.h"
#include "MouseHandler.h"
#include "Utilities/File.h"

mouse_config::mouse_config()
#ifdef _WIN32
	: cfg_name(fs::get_config_dir() + "config/config_mouse.yml")
#else
	: cfg_name(fs::get_config_dir() + "config_mouse.yml")
#endif
{
}

bool mouse_config::exist() const
{
	return fs::is_file(cfg_name);
}

bool mouse_config::load()
{
	if (fs::file cfg_file{cfg_name, fs::read})
	{
		return from_string(cfg_file.to_string());
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
