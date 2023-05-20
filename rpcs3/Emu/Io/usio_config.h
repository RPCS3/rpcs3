#pragma once

#include "Utilities/Config.h"
#include "pad_types.h"

#include <array>

enum class usio_btn
{
	test,
	coin,
	enter,
	up,
	down,
	service,
	strong_hit_side_left,
	strong_hit_side_right,
	strong_hit_center_left,
	strong_hit_center_right,
	small_hit_side_left,
	small_hit_side_right,
	small_hit_center_left,
	small_hit_center_right
};

struct cfg_usio final : cfg::node
{
	cfg_usio(node* owner, const std::string& name) : cfg::node(owner, name) {}

	cfg::_enum<pad_button> test{ this, "Test", pad_button::select };
	cfg::_enum<pad_button> coin{ this, "Coin", pad_button::dpad_left };
	cfg::_enum<pad_button> enter{ this, "Enter", pad_button::start };
	cfg::_enum<pad_button> up{ this, "Up", pad_button::dpad_up };
	cfg::_enum<pad_button> down{ this, "Down", pad_button::dpad_down };
	cfg::_enum<pad_button> service{ this, "Service", pad_button::dpad_right };
	cfg::_enum<pad_button> strong_hit_side_left{ this, "Strong Hit Side Left", pad_button::square };
	cfg::_enum<pad_button> strong_hit_side_right{ this, "Strong Hit Side Right", pad_button::circle };
	cfg::_enum<pad_button> strong_hit_center_left{ this, "Strong Hit Center Left", pad_button::triangle };
	cfg::_enum<pad_button> strong_hit_center_right{ this, "Strong Hit Center Right", pad_button::cross };
	cfg::_enum<pad_button> small_hit_side_left{ this, "Small Hit Side Left", pad_button::L2 };
	cfg::_enum<pad_button> small_hit_side_right{ this, "Small Hit Side Right", pad_button::R2 };
	cfg::_enum<pad_button> small_hit_center_left{ this, "Small Hit Center Left", pad_button::L1 };
	cfg::_enum<pad_button> small_hit_center_right{ this, "Small Hit Center Right", pad_button::R1 };

	std::map<u32, std::map<u32, usio_btn>> buttons;
	std::optional<usio_btn> find_button(u32 offset, u32 keycode) const;
};

struct cfg_usios final : cfg::node
{
	cfg_usio player1{ this, "Player 1" };
	cfg_usio player2{ this, "Player 2" };
	cfg_usio player3{ this, "Player 3" };
	cfg_usio player4{ this, "Player 4" };
	cfg_usio player5{ this, "Player 5" };
	cfg_usio player6{ this, "Player 6" };
	cfg_usio player7{ this, "Player 7" };

	std::array<cfg_usio*, 7> players{ &player1, &player2, &player3, &player4, &player5, &player6, &player7 }; // Thanks gcc!

	bool load();
	void save() const;
};
