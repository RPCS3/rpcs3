#include "stdafx.h"
#include "cheat_info.h"
#include "Config.h"
#include "StrUtil.h"

LOG_CHANNEL(log_cheat, "Cheat");

bool cheat_info::from_str(const std::string& cheat_line)
{
	auto cheat_vec = fmt::split(cheat_line, {"@@@"}, false);

	s64 val64 = 0;
	if (cheat_vec.size() != 7 || !cfg::try_to_int64(&val64, cheat_vec[3], 0, cheat_type_max - 1))
	{
		log_cheat.fatal("Failed to parse cheat line");
		return false;
	}

	serial        = cheat_vec[0];
	game          = cheat_vec[1];
	description   = cheat_vec[2];
	type          = cheat_type{::narrow<u8>(val64)};
	offset        = std::stoul(cheat_vec[4]);
	switch (type)
	{
	case cheat_type::unsigned_8_cheat:
	case cheat_type::unsigned_16_cheat:
	case cheat_type::unsigned_32_cheat:
	case cheat_type::unsigned_64_cheat:
		value.u = std::stoul(cheat_vec[5]);
		break;
	case cheat_type::signed_8_cheat:
	case cheat_type::signed_16_cheat:
	case cheat_type::signed_32_cheat:
	case cheat_type::signed_64_cheat:
		value.s = std::stol(cheat_vec[5]);
		break;
	default:
		value = {};
		break;
	}
	red_script    = cheat_vec[6];
	apply_on_boot = cheat_vec[7] == "true";

	return true;
}

std::string cheat_info::to_str() const
{
	static const std::string del = "@@@";
	std::string cheat_str =
		serial + del +
		game + del +
		description + del +
		std::to_string(static_cast<u8>(type)) + del +
		std::to_string(offset) + del;

	switch (type)
	{
	case cheat_type::unsigned_8_cheat:
	case cheat_type::unsigned_16_cheat:
	case cheat_type::unsigned_32_cheat:
	case cheat_type::unsigned_64_cheat:
		cheat_str += std::to_string(value.u) + del;
		break;
	case cheat_type::signed_8_cheat:
	case cheat_type::signed_16_cheat:
	case cheat_type::signed_32_cheat:
	case cheat_type::signed_64_cheat:
		cheat_str += std::to_string(value.s) + del;
		break;
	default:
		cheat_str += "0" + del;
		break;
	}

	cheat_str +=
		red_script + del +
		(apply_on_boot ? "true" : "false") + del;

	return cheat_str;
}
