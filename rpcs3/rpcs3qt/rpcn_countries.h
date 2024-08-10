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
	const std::vector<country_code> get_countries();
} // namespace countries

