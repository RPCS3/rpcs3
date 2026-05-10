#pragma once

#include <array>
#include <string_view>

namespace countries
{
	struct country_code
	{
		std::string_view name;
		std::string_view ccode;
	};
	extern const std::array<country_code, 72> g_countries;
} // namespace countries

