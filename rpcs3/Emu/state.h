#pragma once
#include "config.h"

namespace rpcs3
{
	struct state_t
	{
		config_t config;

		std::string path_to_elf;
		std::string virtual_path_to_elf;
	};

	extern state_t state;
}
