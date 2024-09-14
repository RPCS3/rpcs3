#pragma once

#include "Utilities/Config.h"

struct cfg_rb3drums final : cfg::node
{
	cfg_rb3drums();
	bool load();
	void save();

	cfg::uint<1, 100> pulse_ms{this, "Pulse width ms", 30, true};
	cfg::uint<1, 127> minimum_velocity{this, "Minimum velocity", 10, true};
	cfg::uint<1, 5000> combo_window_ms{this, "Combo window in milliseconds", 2000, true};
	cfg::_bool stagger_cymbals{this, "Stagger cymbal hits", true, true};
	cfg::string midi_overrides{this, "Midi id to note override", "", true};
	cfg::string combo_start{this, "Combo Start", "HihatPedal,HihatPedal,HihatPedal,Snare", true};
	cfg::string combo_select{this, "Combo Select", "HihatPedal,HihatPedal,HihatPedal,SnareRim", true};
	cfg::string combo_toggle_hold_kick{this, "Combo Toggle Hold Kick", "HihatPedal,HihatPedal,HihatPedal,Kick", true};
	cfg::uint<0, 255> midi_cc_status{this, "Midi CC status", 0xB0, true};
	cfg::uint<0, 127> midi_cc_number{this, "Midi CC control number", 4, true};
	cfg::uint<0, 127> midi_cc_threshold{this, "Midi CC threshold", 64, true};
	cfg::_bool midi_cc_invert_threshold{this, "Midi CC invert threshold", false, true};

	const std::string path;

	atomic_t<bool> reload_requested = false;
};

extern cfg_rb3drums g_cfg_rb3drums;
