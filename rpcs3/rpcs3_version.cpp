#include "stdafx.h"
#include "rpcs3_version.h"
#include "git-version.h"

namespace rpcs3
{
	std::string get_branch()
	{
		return RPCS3_GIT_BRANCH;
	}

	//TODO: Make this accessible from cmake and keep in sync with MACOSX_BUNDLE_BUNDLE_VERSION.
	const extern utils::version version{ 0, 0, 6, utils::version_type::alpha, 1, RPCS3_GIT_VERSION };
}
