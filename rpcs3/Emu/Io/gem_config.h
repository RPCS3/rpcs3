#pragma once

#include "Utilities/Config.h"
#include "pad_types.h"

#include <array>

enum class gem_btn
{
	start,
	select,
	triangle,
	circle,
	cross,
	square,
	move,
	t,
};

struct cfg_gem final : cfg::node
{
	cfg_gem(node* owner, const std::string& name) : cfg::node(owner, name) {}

	cfg::_enum<pad_button> start{ this, "Start", pad_button::start };
	cfg::_enum<pad_button> select{ this, "Select", pad_button::select };
	cfg::_enum<pad_button> triangle{ this, "Triangle", pad_button::triangle };
	cfg::_enum<pad_button> circle{ this, "Circle", pad_button::circle };
	cfg::_enum<pad_button> cross{ this, "Cross", pad_button::cross };
	cfg::_enum<pad_button> square{ this, "Square", pad_button::square };
	cfg::_enum<pad_button> move{ this, "Move", pad_button::R1 };
	cfg::_enum<pad_button> t{ this, "T", pad_button::R2 };

	std::map<u32, std::map<u32, gem_btn>> buttons;
	std::optional<gem_btn> find_button(u32 offset, u32 keycode) const;
};

struct cfg_gems final : cfg::node
{
	cfg_gem player1{ this, "Player 1" };
	cfg_gem player2{ this, "Player 2" };
	cfg_gem player3{ this, "Player 3" };
	cfg_gem player4{ this, "Player 4" };
	cfg_gem player5{ this, "Player 5" };
	cfg_gem player6{ this, "Player 6" };
	cfg_gem player7{ this, "Player 7" };

	std::array<cfg_gem*, 7> players{ &player1, &player2, &player3, &player4, &player5, &player6, &player7 }; // Thanks gcc!

	bool load();
	void save() const;
};
