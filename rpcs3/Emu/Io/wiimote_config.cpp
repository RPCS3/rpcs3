#include "stdafx.h"
#include "wiimote_config.h"
#include "Utilities/File.h"

LOG_CHANNEL(wiimote_log, "Wiimote");

template <>
void fmt_class_string<wiimote_button>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](wiimote_button value)
	{
		switch (value)
		{
		case wiimote_button::None: return "None";
		case wiimote_button::Left: return "Left";
		case wiimote_button::Right: return "Right";
		case wiimote_button::Down: return "Down";
		case wiimote_button::Up: return "Up";
		case wiimote_button::Plus: return "Plus";
		case wiimote_button::Two: return "Two";
		case wiimote_button::One: return "One";
		case wiimote_button::B: return "B";
		case wiimote_button::A: return "A";
		case wiimote_button::Minus: return "Minus";
		case wiimote_button::Home: return "Home";
		}
		return unknown;
	});
}

cfg_wiimote& get_wiimote_config()
{
	static cfg_wiimote instance;
	return instance;
}

cfg_wiimote::cfg_wiimote()
	: cfg::node()
	, path(fs::get_config_dir(true) + "wiimote.yml")
{
}

bool cfg_wiimote::load()
{
	if (fs::file f{path, fs::read})
	{
		return this->cfg::node::from_string(f.to_string());
	}
	return false;
}

void cfg_wiimote::save() const
{
	if (!this->cfg::node::save(path))
	{
		wiimote_log.error("Failed to save wiimote config to '%s'", path);
	}
}
