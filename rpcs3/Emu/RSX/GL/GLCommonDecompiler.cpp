#include "stdafx.h"
#include "GLCommonDecompiler.h"


namespace gl
{
	int get_varying_register_location(const std::string &var_name)
	{
		static const std::pair<std::string, int> reg_table[] =
		{
			{ "diff_color", 1 },
			{ "spec_color", 2 },
			{ "back_diff_color", 1 },
			{ "back_spec_color", 2 },
			{ "front_diff_color", 3 },
			{ "front_spec_color", 4 },
			{ "fog_c", 5 },
			{ "tc0", 6 },
			{ "tc1", 7 },
			{ "tc2", 8 },
			{ "tc3", 9 },
			{ "tc4", 10 },
			{ "tc5", 11 },
			{ "tc6", 12 },
			{ "tc7", 13 },
			{ "tc8", 14 },
			{ "tc9", 15 }
		};

		for (const auto& v: reg_table)
		{
			if (v.first == var_name)
				return v.second;
		}

		fmt::throw_exception("register named %s should not be declared!", var_name.c_str());
	}
}
