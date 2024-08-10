#pragma once

#include "util/types.hpp"
#include <string>
#include "stdafx.h"
#include "np_countries.h"

namespace countries
{
	const std::vector<country_code> get_countries()
	{
		std::vector<country_code> countries =
		{
				{"Japan", "jp"},
				{"United States", "us"},
				{"Argentina", "ar"},
				{"Australia", "au"},
				{"Austria", "at"},
				{"Bahrain", "bh"},
				{"Belgium", "be"},
				{"Bolivia", "bo"},
				{"Brazil", "br"},
				{"Bulgaria", "bg"},
				{"Canada", "ca"},
				{"Chile", "cl"},
				{"China", "cn"},
				{"Colombia", "co"},
				{"Costa Rica", "cr"},
				{"Croatia", "hr"},
				{"Cyprus", "cy"},
				{"Czech Republic", "cz"},
				{"Denmark", "dk"},
				{"Ecuador", "ec"},
				{"El Salvador", "sv"},
				{"Finland", "fi"},
				{"France", "fr"},
				{"Germany", "de"},
				{"Greece", "gr"},
				{"Guatemala", "gt"},
				{"Honduras", "hn"},
				{"Hong Kong", "hk"},
				{"Hungary", "hu"},
				{"Iceland", "is"},
				{"India", "in"},
				{"Indonesia", "id"},
				{"Ireland", "ie"},
				{"Israel", "il"},
				{"Italy", "it"},
				{"Korea", "kr"},
				{"Kuwait", "kw"},
				{"Lebanon", "lb"},
				{"Luxembourg", "lu"},
				{"Malaysia", "my"},
				{"Malta", "mt"},
				{"Mexico", "mx"},
				{"Netherlands", "nl"},
				{"New Zealand", "nz"},
				{"Nicaragua", "ni"},
				{"Norway", "no"},
				{"Oman", "om"},
				{"Panama", "pa"},
				{"Paraguay", "py"},
				{"Peru", "pe"},
				{"Philippines", "ph"},
				{"Poland", "pl"},
				{"Portugal", "pt"},
				{"Qatar", "qa"},
				{"Romania", "ro"},
				{"Russia", "ru"},
				{"Saudi Arabia", "sa"},
				{"Serbia", "rs"},
				{"Singapore", "sg"},
				{"Slovakia", "sk"},
				{"South Africa", "za"},
				{"Spain", "es"},
				{"Sweden", "se"},
				{"Switzerland", "ch"},
				{"Taiwan", "tw"},
				{"Thailand", "th"},
				{"Turkey", "tr"},
				{"Ukraine", "ua"},
				{"United Arab Emirates", "ae"},
				{"United Kingdom", "gb"},
				{"Uruguay", "uy"},
				{"Vietnam", "vn"}
		};

		return countries;
	}
} // namespace countries
