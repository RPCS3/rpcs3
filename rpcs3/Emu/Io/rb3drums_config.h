#pragma once

#include "Utilities/Config.h"

struct cfg_rb3drums final : cfg::node
{
	cfg_rb3drums();
	bool load();
	void save() const;

	cfg::uint<1, 100> pulse_ms{this, "Pulse width ms", 30, true};
	cfg::uint<1, 127> minimum_velocity{this, "Minimum velocity", 10, true};
	cfg::uint<1, 5000> combo_window_ms{this, "Combo window in milliseconds", 2000, true};
	cfg::string midi_overrides{this, "Midi id to note override", ""};
	cfg::string combo_start{this, "Combo Start", "HihatPedal,HihatPedal,HihatPedal,Snare"};
	cfg::string combo_select{this, "Combo Select", "HihatPedal,HihatPedal,HihatPedal,SnareRim"};
	cfg::string combo_toggle_hold_kick{this, "Combo Toggle Hold Kick", "HihatPedal,HihatPedal,HihatPedal,Kick"};

	const std::string path;
};

extern cfg_rb3drums g_cfg_rb3drums;
