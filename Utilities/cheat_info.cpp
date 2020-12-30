#include "stdafx.h"
#include "cheat_info.h"
#include "Config.h"
#include "StrUtil.h"

LOG_CHANNEL(log_cheat, "Cheat");

bool cheat_info::from_str(const std::string& cheat_line)
{
	auto cheat_vec = fmt::split(cheat_line, {"@@@"}, false);

	s64 val64 = 0;
	if (cheat_vec.size() != 5 || !cfg::try_to_int64(&val64, cheat_vec[2], 0, cheat_type_max - 1))
	{
		log_cheat.fatal("Failed to parse cheat line");
		return false;
	}

	game        = cheat_vec[0];
	description = cheat_vec[1];
	type        = cheat_type{::narrow<u8>(val64)};
	offset      = std::stoul(cheat_vec[3]);
	red_script  = cheat_vec[4];

	return true;
}

std::string cheat_info::to_str() const
{
	std::string cheat_str = game + "@@@" + description + "@@@" + std::to_string(static_cast<u8>(type)) + "@@@" + std::to_string(offset) + "@@@" + red_script + "@@@";
	return cheat_str;
}
