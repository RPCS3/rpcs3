#include "stdafx.h"
#include "GLCommonDecompiler.h"

namespace gl
{
	static constexpr std::array<std::pair<std::string_view, int>, 17> varying_registers =
	{{
		{"diff_color", 1},
		{"spec_color", 2},
		{"diff_color1", 3},
		{"spec_color1", 4},
		{"fogc", 5},
		{"fog_c", 5},
		{"tc0", 6},
		{"tc1", 7},
		{"tc2", 8},
		{"tc3", 9},
		{"tc4", 10},
		{"tc5", 11},
		{"tc6", 12},
		{"tc7", 13},
		{"tc8", 14},
		{"tc9", 15}
	 }};

	int get_varying_register_location(std::string_view varying_register_name)
	{
		for (const auto& varying_register : varying_registers)
		{
			if (varying_register.first == varying_register_name)
			{
				return varying_register.second;
			}
		}

		fmt::throw_exception("Unknown register name: %s", varying_register_name);
	}
}
