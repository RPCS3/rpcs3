#pragma once

#include "Utilities/Config.h"
#include "Utilities/mutex.h"

#include <array>

#ifdef _WIN32
#include <windows.h>
#endif

static const std::map<std::string_view, int> raw_mouse_button_map
{
	{ "", 0 },
#ifdef _WIN32
	{ "Button 1", RI_MOUSE_BUTTON_1_UP },
	{ "Button 2", RI_MOUSE_BUTTON_2_UP },
	{ "Button 3", RI_MOUSE_BUTTON_3_UP },
	{ "Button 4", RI_MOUSE_BUTTON_4_UP },
	{ "Button 5", RI_MOUSE_BUTTON_5_UP },
#endif
};

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

	static constexpr std::string_view key_prefix = "Key ";

	static std::string get_button_name(std::string_view value);
	static std::string get_button_name(s32 button_code);
	static std::string get_key_name(s32 scan_code);
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
