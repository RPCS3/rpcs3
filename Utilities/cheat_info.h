#pragma once

#include "stdafx.h"

enum class cheat_type : u8
{
	unsigned_8_cheat,
	unsigned_16_cheat,
	unsigned_32_cheat,
	unsigned_64_cheat,
	signed_8_cheat,
	signed_16_cheat,
	signed_32_cheat,
	signed_64_cheat,
	max
};

constexpr u8 cheat_type_max = static_cast<u8>(cheat_type::max);

namespace cheat_key
{
	static const std::string cheats = "Cheats";
	static const std::string description = "Description";
	static const std::string title = "Title";
	static const std::string script = "Script";
	static const std::string type = "Type";
	static const std::string value = "Value";
	static const std::string apply_on_boot = "Apply on boot";
}

struct cheat_info
{
	std::string serial;
	std::string game;
	std::string description;
	cheat_type type = cheat_type::max;
	u32 offset{};
	union
	{
		u64 u;
		s64 s;
	} value{};
	std::string red_script;
	bool apply_on_boot = false;

	bool from_str(const std::string& cheat_line);
	std::string to_str() const;
};
