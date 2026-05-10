#pragma once

#include "localized_string_id.h"
#include "Utilities/StrFmt.h"

std::string get_localized_string(localized_string_id id, const char* args = "");
std::u32string get_localized_u32string(localized_string_id id, const char* args = "");

template <typename CharT, usz N, typename... Args>
requires (sizeof...(Args) > 0)
std::string get_localized_string(localized_string_id id, const CharT(&fmt)[N], const Args&... args)
{
	return get_localized_string(id, fmt::format(fmt, args...).c_str());
}

struct localized_string
{
	template <typename CharT, usz N, typename... Args>
	requires (sizeof...(Args) > 0)
	localized_string(localized_string_id _id, const CharT(&fmt)[N], const Args&... args)
		: id(_id), str(get_localized_string(id, fmt, args...))
	{
	}

	localized_string_id id = localized_string_id::INVALID;
	std::string str;
};
