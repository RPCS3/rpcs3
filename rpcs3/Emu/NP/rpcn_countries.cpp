#include "stdafx.h"
#include "rpcn_countries.h"

namespace countries
{
	const std::array<country_code, 72> g_countries =
		{
			country_code{"Japan", "jp"},
			country_code{"United States", "us"},
			country_code{"Argentina", "ar"},
			country_code{"Australia", "au"},
			country_code{"Austria", "at"},
			country_code{"Bahrain", "bh"},
			country_code{"Belgium", "be"},
			country_code{"Bolivia", "bo"},
			country_code{"Brazil", "br"},
			country_code{"Bulgaria", "bg"},
			country_code{"Canada", "ca"},
			country_code{"Chile", "cl"},
			country_code{"China", "cn"},
			country_code{"Colombia", "co"},
			country_code{"Costa Rica", "cr"},
			country_code{"Croatia", "hr"},
			country_code{"Cyprus", "cy"},
			country_code{"Czech Republic", "cz"},
			country_code{"Denmark", "dk"},
			country_code{"Ecuador", "ec"},
			country_code{"El Salvador", "sv"},
			country_code{"Finland", "fi"},
			country_code{"France", "fr"},
			country_code{"Germany", "de"},
			country_code{"Greece", "gr"},
			country_code{"Guatemala", "gt"},
			country_code{"Honduras", "hn"},
			country_code{"Hong Kong", "hk"},
			country_code{"Hungary", "hu"},
			country_code{"Iceland", "is"},
			country_code{"India", "in"},
			country_code{"Indonesia", "id"},
			country_code{"Ireland", "ie"},
			country_code{"Israel", "il"},
			country_code{"Italy", "it"},
			country_code{"Korea", "kr"},
			country_code{"Kuwait", "kw"},
			country_code{"Lebanon", "lb"},
			country_code{"Luxembourg", "lu"},
			country_code{"Malaysia", "my"},
			country_code{"Malta", "mt"},
			country_code{"Mexico", "mx"},
			country_code{"Netherlands", "nl"},
			country_code{"New Zealand", "nz"},
			country_code{"Nicaragua", "ni"},
			country_code{"Norway", "no"},
			country_code{"Oman", "om"},
			country_code{"Panama", "pa"},
			country_code{"Paraguay", "py"},
			country_code{"Peru", "pe"},
			country_code{"Philippines", "ph"},
			country_code{"Poland", "pl"},
			country_code{"Portugal", "pt"},
			country_code{"Qatar", "qa"},
			country_code{"Romania", "ro"},
			country_code{"Russia", "ru"},
			country_code{"Saudi Arabia", "sa"},
			country_code{"Serbia", "rs"},
			country_code{"Singapore", "sg"},
			country_code{"Slovakia", "sk"},
			country_code{"South Africa", "za"},
			country_code{"Spain", "es"},
			country_code{"Sweden", "se"},
			country_code{"Switzerland", "ch"},
			country_code{"Taiwan", "tw"},
			country_code{"Thailand", "th"},
			country_code{"Turkey", "tr"},
			country_code{"Ukraine", "ua"},
			country_code{"United Arab Emirates", "ae"},
			country_code{"United Kingdom", "gb"},
			country_code{"Uruguay", "uy"},
			country_code{"Vietnam", "vn"}
		};

	inline static const std::unordered_map<std::string_view, currency_info> currency_table{
		{"us", currency_info{"USD", "$",   COMMA, POINT, 2, true}},		// US Dollar
		{"eu", currency_info{"EUR", "€",   POINT, COMMA, 2, false}},	// Euros
		{"gb", currency_info{"GBP", "£",   COMMA, POINT, 2, true}},		// UK
		{"jp", currency_info{"JPY", "¥",   COMMA, EMPTY, 0, true}},		// Japan
		{"au", currency_info{"AUD", "$",   COMMA, POINT, 2, true}},		// Australia
		{"bg", currency_info{"BGN", "лв",  SPACE, COMMA, 2, false}},	// Bulgaria
		{"br", currency_info{"BRL", "R$",  POINT, COMMA, 2, true}},		// Brazil
		{"ca", currency_info{"CAD", "$",   COMMA, POINT, 2, true}},		// Canada
		{"hr", currency_info{"HRK", "kn",  POINT, COMMA, 2, false}},	// Croatia
		{"cz", currency_info{"CZK", "Kč",  SPACE, COMMA, 2, false}},	// Czech Republic
		{"dk", currency_info{"DKK", "kr",  POINT, COMMA, 2, false}},	// Denmark
		{"hu", currency_info{"HUF", "Ft",  SPACE, COMMA, 2, false}},	// Hungary
		{"in", currency_info{"INR", "₹",   COMMA, POINT, 2, true}},		// India
		{"il", currency_info{"ILS", "₪",   COMMA, POINT, 2, true}},		// Israel
		{"nz", currency_info{"NZD", "$",   COMMA, POINT, 2, true}},		// New Zealand
		{"no", currency_info{"NOK", "kr",  SPACE, COMMA, 2, false}},	// Norway
		{"pl", currency_info{"PLN", "zł",  SPACE, COMMA, 2, false}},	// Poland
		{"ro", currency_info{"RON", "lei", POINT, COMMA, 2, false}},	// Romania
		{"ru", currency_info{"RUB", "₽",   SPACE, COMMA, 2, false}},	// Russia
		{"za", currency_info{"ZAR", "R",   SPACE, COMMA, 2, false}},	// South Africa
		{"se", currency_info{"SEK", "kr",  SPACE, COMMA, 2, false}},	// Sweden
		{"ch", currency_info{"CHF", "CHF", APOST, POINT, 2, true}},     // Switzerland
		{"tr", currency_info{"TRY", "₺",   POINT, COMMA, 2, false}},	// Turkey
		{"ua", currency_info{"UAH", "₴",   SPACE, COMMA, 2, false}},	// Ukraine
		{"hk", currency_info{"HKD", "$",   COMMA, POINT, 2, true}},		// Hong Kong
		{"id", currency_info{"IDR", "Rp",  POINT, EMPTY, 0, true}},		// Indonesia
		{"kr", currency_info{"KRW", "₩",   COMMA, EMPTY, 0, true}},		// Korea
		{"my", currency_info{"MYR", "RM",  COMMA, POINT, 2, true}},		// Malaysia
		{"sg", currency_info{"SGD", "$",   COMMA, POINT, 2, true}},		// Singapore
		{"tw", currency_info{"TWD", "NT$", COMMA, POINT, 2, true}},		// Taiwan
		{"th", currency_info{"THB", "฿",   COMMA, POINT, 2, true}}		// Thailand
	};

	currency_info get_currency_info(std::string_view ccode)
	{
		// US. Dollar
		if (ccode == "us" || ccode == "ar" || ccode == "cl" || ccode == "co" || ccode == "mx" || ccode == "bh" ||
			ccode == "kw" || ccode == "lb" || ccode == "om" || ccode == "qa" || ccode == "sa" || ccode == "ae")
			return currency_table.at("us");

		// Euros
		if (ccode == "at" || ccode == "be" || ccode == "cy" || ccode == "fi" || ccode == "fr" || ccode == "de" || ccode == "gr" || ccode == "is" ||
			ccode == "ie" || ccode == "it" || ccode == "lu" || ccode == "mt" || ccode == "nl" || ccode == "pt" || ccode == "sk" || ccode == "es")
			return currency_table.at("eu");

		if (const auto it = currency_table.find(ccode); it != currency_table.end())
		{
			return it->second;
		}

		// fallback to USD
		return currency_info{"USD", "$", COMMA, POINT, 2, true};
	}
} // namespace countries

