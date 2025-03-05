#include "stdafx.h"
#include "cheat_info.h"
#include "StrUtil.h"

LOG_CHANNEL(log_cheat, "Cheat");

template <>
void fmt_class_string<cheat_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](cheat_type value)
	{
		switch (value)
		{
		case cheat_type::unsigned_8_cheat: return "Unsigned 8 bits";
		case cheat_type::unsigned_16_cheat: return "Unsigned 16 bits";
		case cheat_type::unsigned_32_cheat: return "Unsigned 32 bits";
		case cheat_type::unsigned_64_cheat: return "Unsigned 64 bits";
		case cheat_type::signed_8_cheat: return "Signed 8 bits";
		case cheat_type::signed_16_cheat: return "Signed 16 bits";
		case cheat_type::signed_32_cheat: return "Signed 32 bits";
		case cheat_type::signed_64_cheat: return "Signed 64 bits";
		case cheat_type::float_32_cheat: return "Float 32 bits";
		case cheat_type::max: break;
		}

		return unknown;
	});
}

bool cheat_info::from_str(const std::string& cheat_line)
{
	auto cheat_vec = fmt::split(cheat_line, {"@@@"}, false);

	s64 val64 = 0;
	if (cheat_vec.size() != 5 || !try_to_int64(&val64, cheat_vec[2], 0, cheat_type_max - 1))
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
