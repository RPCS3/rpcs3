#include "stdafx.h"
#include "VKCommonDecompiler.h"

namespace vk
{
	static constexpr std::array<std::pair<std::string_view, int>, 18> varying_registers =
	{ {
		{ "tc0", 0 },
		{ "tc1", 1 },
		{ "tc2", 2 },
		{ "tc3", 3 },
		{ "tc4", 4 },
		{ "tc5", 5 },
		{ "tc6", 6 },
		{ "tc7", 7 },
		{ "tc8", 8 },
		{ "tc9", 9 },
		{ "diff_color", 10 },
		{ "diff_color1", 11 },
		{ "spec_color", 12 },
		{ "spec_color1", 13 },
		{ "fog_c", 14 },
		{ "fogc", 14 }
	} };

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
