#pragma once

#include <vector>
#include <string>

namespace countries
{
	struct country_code
	{
		std::string name;
		std::string ccode;
	};
	const std::array<country_code, 72> get_countries();
} // namespace countries

