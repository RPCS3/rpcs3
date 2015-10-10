#include "stdafx.h"
#include "emulator.h"

namespace rpcs3
{
	namespace emu
	{
		event<void> oninit;
		event<void> onstart;
		event<void> onstop;
		event<void> onpause;

		data_event<std::string> path_to_elf;
		data_event<std::string> virtual_path_to_elf;
	}
}