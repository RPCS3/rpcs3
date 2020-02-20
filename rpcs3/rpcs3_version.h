#pragma once
#include <string>
#include <cstdint>
#include <Utilities/version.h>

namespace rpcs3
{
	std::string_view get_branch();
	std::pair<std::string, std::string> get_commit_and_hash();
	const utils::version& get_version();
	std::string get_version_and_branch();
}
