#pragma once

#include "util/types.hpp"
#include <string>
namespace countries
{
	struct country_code
	{
		std::string name;
		std::string ccode;
	};
	constexpr std::vector<country_code> get_countries();
} // namespace countries