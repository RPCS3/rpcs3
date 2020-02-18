#include "stdafx.h"
#include "pad_config.h"
#include "Emu/System.h"

template <>
void fmt_class_string<pad_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](pad_handler value)
	{
		switch (value)
		{
		case pad_handler::null: return "Null";
		case pad_handler::keyboard: return "Keyboard";
		case pad_handler::ds3: return "DualShock 3";
		case pad_handler::ds4: return "DualShock 4";
#ifdef _WIN32
		case pad_handler::xinput: return "XInput";
		case pad_handler::mm: return "MMJoystick";
#endif
#ifdef HAVE_LIBEVDEV
		case pad_handler::evdev: return "Evdev";
#endif
		}

		return unknown;
	});
}

bool cfg_input::load(const std::string& title_id)
{
	cfg_name = Emulator::GetCustomInputConfigPath(title_id);

	if (!fs::is_file(cfg_name))
	{
		cfg_name = fs::get_config_dir() + "/config_input.yml";
	}

	if (fs::file cfg_file{ cfg_name, fs::read })
	{
		return from_string(cfg_file.to_string());
	}
	return false;
}

void cfg_input::save(const std::string& title_id)
{
	if (title_id.empty())
	{
		cfg_name = fs::get_config_dir() + "/config_input.yml";
	}
	else
	{
		cfg_name = Emulator::GetCustomInputConfigPath(title_id);
	}
	fs::file(cfg_name, fs::rewrite).write(to_string());
}


bool pad_config::exist()
{
	return fs::is_file(cfg_name);
}

bool pad_config::load()
{
	if (fs::file cfg_file{ cfg_name, fs::read })
	{
		return from_string(cfg_file.to_string());
	}

	return false;
}

void pad_config::save()
{
	fs::file(cfg_name, fs::rewrite).write(to_string());
}
