#pragma once

#include "Utilities/Config.h"
#include "Utilities/mutex.h"

#include <array>

LOG_CHANNEL(cfg_log, "CFG");

std::string mouse_button_id(int code);

struct raw_mouse_config : cfg::node
{
public:
	using cfg::node::node;

	cfg::string device{this, "Device", ""};

	cfg::_float<10, 1000> mouse_acceleration{ this, "Mouse Acceleration", 100.0f, true };

	cfg::string mouse_button_1{ this, "Button 1", "Button 1", true };
	cfg::string mouse_button_2{ this, "Button 2", "Button 2", true };
	cfg::string mouse_button_3{ this, "Button 3", "Button 3", true };
	cfg::string mouse_button_4{ this, "Button 4", "Button 4", true };
	cfg::string mouse_button_5{ this, "Button 5", "Button 5", true };
	cfg::string mouse_button_6{ this, "Button 6", "", true };
	cfg::string mouse_button_7{ this, "Button 7", "", true };
	cfg::string mouse_button_8{ this, "Button 8", "", true };

	cfg::string& get_button_by_index(int index);
	cfg::string& get_button(int code);
};

struct raw_mice_config : cfg::node
{
	raw_mice_config();

	shared_mutex m_mutex;
	static constexpr std::string_view cfg_id = "raw_mouse";
	std::array<std::shared_ptr<raw_mouse_config>, 4> players;
	atomic_t<bool> reload_requested = false;

	bool load();
	void save();
};

extern raw_mice_config g_cfg_raw_mouse;
