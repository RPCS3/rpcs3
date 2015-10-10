#pragma once
#include "Utilities/event.h"

namespace rpcs3
{
	namespace emu
	{
		extern event<void> oninit;
		extern event<void> onstart;
		extern event<void> onstop;
		extern event<void> onpause;

		extern data_event<std::string> path_to_elf;
		extern data_event<std::string> virtual_path_to_elf;
	}
}