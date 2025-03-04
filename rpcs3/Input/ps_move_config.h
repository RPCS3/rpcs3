#pragma once

#include "Utilities/Config.h"

#include <array>

struct cfg_ps_move final : cfg::node
{
	cfg_ps_move() : cfg::node() {}
	cfg_ps_move(cfg::node* owner, std::string name) : cfg::node(owner, std::move(name)) {}

	cfg::uint<0, 255> r{ this, "Color R", 0, true };
	cfg::uint<0, 255> g{ this, "Color G", 0, true };
	cfg::uint<0, 255> b{ this, "Color B", 0, true };
	cfg::uint<0, 359> hue{ this, "Hue", 0, true };
	cfg::uint<0, 180> hue_threshold{ this, "Hue Threshold", 10, true };
	cfg::uint<0, 255> saturation_threshold{ this, "Saturation Threshold", 10, true };
};

struct cfg_ps_moves final : cfg::node
{
	cfg_ps_moves();

	cfg_ps_move move1{ this, "PS Move 1" };
	cfg_ps_move move2{ this, "PS Move 2" };
	cfg_ps_move move3{ this, "PS Move 3" };
	cfg_ps_move move4{ this, "PS Move 4" };

	cfg::_float<0, 50> min_radius{ this, "Minimum Radius", 1.0f, true }; // Percentage of image width
	cfg::_float<0, 50> max_radius{ this, "Maximum Radius", 10.0f, true }; // Percentage of image width

	std::array<cfg_ps_move*, 4> move{ &move1, &move2, &move3, &move4 };

	const std::string path;

	bool load();
	void save() const;
};

extern cfg_ps_moves g_cfg_move;
