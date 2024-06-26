#pragma once

#include "Utilities/Config.h"

// For simplicity's sake, there is only one config instead of 127 for MAX_MICE
struct mouse_config final : cfg::node
{
	mouse_config();

	const std::string cfg_name;

	cfg::string mouse_button_1{ this, "Button 1", "Mouse Left", true };
	cfg::string mouse_button_2{ this, "Button 2", "Mouse Right", true };
	cfg::string mouse_button_3{ this, "Button 3", "Mouse Middle", true };
	cfg::string mouse_button_4{ this, "Button 4", "", true };
	cfg::string mouse_button_5{ this, "Button 5", "", true };
	cfg::string mouse_button_6{ this, "Button 6", "", true };
	cfg::string mouse_button_7{ this, "Button 7", "", true };
	cfg::string mouse_button_8{ this, "Button 8", "", true };

	atomic_t<bool> reload_requested = true;

	bool exist() const;
	bool load();
	void save();

	cfg::string& get_button(int code);
};

extern mouse_config g_cfg_mouse;
