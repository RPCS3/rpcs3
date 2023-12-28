#pragma once
#include <string>
#include <Utilities/version.h>

namespace rpcs3
{
	std::string_view get_branch();
	std::string_view get_full_branch();
	std::pair<std::string, std::string> get_commit_and_hash();
	const ::utils::version& get_version();
	std::string get_version_and_branch();
	std::string get_verbose_version();
	bool is_release_build();
	bool is_local_build();
}
