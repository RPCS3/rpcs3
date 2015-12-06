#pragma once
#include <string>

namespace rpcs3
{
	namespace
	{
		constexpr int version_hi() { return 0; }
		constexpr int version_mid() { return 0; }
		constexpr int version_lo() { return 1; }

		constexpr const char* name()
		{
			return "rpcs3";
		}

		std::string verison()
		{
			return
				std::to_string(version_hi()) + "." +
				std::to_string(version_mid()) + "." +
				std::to_string(version_lo());
		}

		std::string name_with_version()
		{
			return std::string(name()) + " v" + verison();
		}
	}
}