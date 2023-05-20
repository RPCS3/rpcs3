#pragma once

#include "Utilities/Config.h"
#include "pad_types.h"

#include <array>

enum class ghltar_btn
{
	w1,
	w2,
	w3,
	b1,
	b2,
	b3,
	start,
	hero_power,
	ghtv,
	strum_down,
	strum_up,
	dpad_left,
	dpad_right
};

struct cfg_ghltar final : cfg::node
{
	cfg_ghltar(node* owner, const std::string& name) : cfg::node(owner, name) {}

	cfg::_enum<pad_button> w1{ this, "W1", pad_button::square };
	cfg::_enum<pad_button> w2{ this, "W2", pad_button::L1 };
	cfg::_enum<pad_button> w3{ this, "W3", pad_button::R1 };
	cfg::_enum<pad_button> b1{ this, "B1", pad_button::cross };
	cfg::_enum<pad_button> b2{ this, "B2", pad_button::circle };
	cfg::_enum<pad_button> b3{ this, "B3", pad_button::triangle };
	cfg::_enum<pad_button> start{ this, "Start", pad_button::start };
	cfg::_enum<pad_button> hero_power{ this, "Hero Power", pad_button::select };
	cfg::_enum<pad_button> ghtv{ this, "GHTV", pad_button::L3 };
	cfg::_enum<pad_button> strum_down{ this, "Strum Down", pad_button::dpad_down };
	cfg::_enum<pad_button> strum_up{ this, "Strum Up", pad_button::dpad_up };
	cfg::_enum<pad_button> dpad_left{ this, "D-Pad Left", pad_button::dpad_left };
	cfg::_enum<pad_button> dpad_right{ this, "D-Pad Right", pad_button::dpad_right };

	std::map<u32, std::map<u32, ghltar_btn>> buttons;
	std::optional<ghltar_btn> find_button(u32 offset, u32 keycode) const;
};

struct cfg_ghltars final : cfg::node
{
	cfg_ghltar player1{ this, "Player 1" };
	cfg_ghltar player2{ this, "Player 2" };
	cfg_ghltar player3{ this, "Player 3" };
	cfg_ghltar player4{ this, "Player 4" };
	cfg_ghltar player5{ this, "Player 5" };
	cfg_ghltar player6{ this, "Player 6" };
	cfg_ghltar player7{ this, "Player 7" };

	std::array<cfg_ghltar*, 7> players{ &player1, &player2, &player3, &player4, &player5, &player6, &player7 }; // Thanks gcc!

	bool load();
	void save() const;
};
