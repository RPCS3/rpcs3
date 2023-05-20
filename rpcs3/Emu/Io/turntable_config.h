#pragma once

#include "Utilities/Config.h"
#include "pad_types.h"

#include <array>

enum class turntable_btn
{
	red,
	green,
	blue,
	dpad_up,
	dpad_down,
	dpad_left,
	dpad_right,
	start,
	select,
	square,
	circle,
	cross,
	triangle
};

struct cfg_turntable final : cfg::node
{
	cfg_turntable(node* owner, const std::string& name) : cfg::node(owner, name) {}

	cfg::_enum<pad_button> blue{ this, "Blue", pad_button::square };
	cfg::_enum<pad_button> green{ this, "Green", pad_button::cross };
	cfg::_enum<pad_button> red{ this, "Red", pad_button::circle };
	cfg::_enum<pad_button> dpad_up{ this, "D-Pad Up", pad_button::dpad_up };
	cfg::_enum<pad_button> dpad_down{ this, "D-Pad Down", pad_button::dpad_down };
	cfg::_enum<pad_button> dpad_left{ this, "D-Pad Left", pad_button::dpad_left };
	cfg::_enum<pad_button> dpad_right{ this, "D-Pad Right", pad_button::dpad_right };
	cfg::_enum<pad_button> start{ this, "Start", pad_button::start };
	cfg::_enum<pad_button> select{ this, "Select", pad_button::select };
	cfg::_enum<pad_button> square{ this, "Square", pad_button::R2 };
	cfg::_enum<pad_button> circle{ this, "Circle", pad_button::L1 };
	cfg::_enum<pad_button> cross{ this, "Cross", pad_button::R1 };
	cfg::_enum<pad_button> triangle{ this, "Triangle", pad_button::triangle };

	std::map<u32, std::map<u32, turntable_btn>> buttons;
	std::optional<turntable_btn> find_button(u32 offset, u32 keycode) const;
};

struct cfg_turntables final : cfg::node
{
	cfg_turntable player1{ this, "Player 1" };
	cfg_turntable player2{ this, "Player 2" };
	cfg_turntable player3{ this, "Player 3" };
	cfg_turntable player4{ this, "Player 4" };
	cfg_turntable player5{ this, "Player 5" };
	cfg_turntable player6{ this, "Player 6" };
	cfg_turntable player7{ this, "Player 7" };

	std::array<cfg_turntable*, 7> players{ &player1, &player2, &player3, &player4, &player5, &player6, &player7 }; // Thanks gcc!

	bool load();
	void save() const;
};
