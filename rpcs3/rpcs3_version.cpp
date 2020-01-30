#include "stdafx.h"
#include "rpcs3_version.h"
#include "git-version.h"
#include "Utilities/StrUtil.h"

namespace rpcs3
{
	std::string get_branch()
	{
		return RPCS3_GIT_BRANCH;
	}

	std::pair<std::string, std::string> get_commit_and_hash()
	{
		const auto commit_and_hash = fmt::split(RPCS3_GIT_VERSION, {"-"});
		if (commit_and_hash.size() != 2)
			return std::make_pair("0", "00000000");

		return std::make_pair(commit_and_hash[0], commit_and_hash[1]);
	}

	// TODO: Make this accessible from cmake and keep in sync with MACOSX_BUNDLE_BUNDLE_VERSION.
	// Currently accessible by Windows and Linux build scripts, see implementations when doing MACOSX
	const utils::version& get_version()
	{
		static constexpr utils::version version{ 0, 0, 8, utils::version_type::alpha, 1, RPCS3_GIT_VERSION };
		return version;
	}
}
