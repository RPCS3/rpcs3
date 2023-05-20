#pragma once

#include "Utilities/Config.h"
#include "pad_types.h"

#include <array>

enum class buzz_btn
{
	red,
	yellow,
	green,
	orange,
	blue
};

struct cfg_buzzer final : cfg::node
{
	cfg_buzzer(node* owner, const std::string& name) : cfg::node(owner, name) {}

	cfg::_enum<pad_button> red{ this, "Red", pad_button::R1 };
	cfg::_enum<pad_button> yellow{ this, "Yellow", pad_button::cross };
	cfg::_enum<pad_button> green{ this, "Green", pad_button::circle };
	cfg::_enum<pad_button> orange{ this, "Orange", pad_button::square };
	cfg::_enum<pad_button> blue{ this, "Blue", pad_button::triangle };

	std::map<u32, std::map<u32, buzz_btn>> buttons;
	std::optional<buzz_btn> find_button(u32 offset, u32 keycode) const;
};

struct cfg_buzz final : cfg::node
{
	cfg_buzzer player1{ this, "Player 1" };
	cfg_buzzer player2{ this, "Player 2" };
	cfg_buzzer player3{ this, "Player 3" };
	cfg_buzzer player4{ this, "Player 4" };
	cfg_buzzer player5{ this, "Player 5" };
	cfg_buzzer player6{ this, "Player 6" };
	cfg_buzzer player7{ this, "Player 7" };

	std::array<cfg_buzzer*, 7> players{ &player1, &player2, &player3, &player4, &player5, &player6, &player7 }; // Thanks gcc!

	bool load();
	void save() const;
};
