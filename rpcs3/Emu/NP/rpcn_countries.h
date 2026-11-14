#pragma once

#include <array>
#include <string_view>

#include "util/types.hpp"

namespace countries
{
	inline static const char POINT = '.';
	inline static const char COMMA = ',';
	inline static const char SPACE = ' ';
	inline static const char APOST = '\'';
	inline static const char EMPTY = '\0';

	struct country_code
	{
		std::string_view name;
		std::string_view ccode;
	};
	extern const std::array<country_code, 72> g_countries;

	struct currency_info
	{
		char cur_code[4];
		char symbol[5];
		char thousands;
		char decimals;
		u32 num_decimals;
		bool symbol_before;
	};
	currency_info get_currency_info(std::string_view ccode);
} // namespace countries

