#include "stdafx.h"
#include "pad_config.h"
#include "Emu/System.h"

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
	else
	{
		// Add keyboard by default
		player[0]->handler.from_string(fmt::format("%s", pad_handler::keyboard));
		player[0]->device.from_string(pad::keyboard_device_name.data());
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


bool pad_config::exist() const
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

void pad_config::save() const
{
	fs::file(cfg_name, fs::rewrite).write(to_string());
}
