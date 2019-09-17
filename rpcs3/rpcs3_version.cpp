#include "stdafx.h"
#include "rpcs3_version.h"
#include "git-version.h"

namespace rpcs3
{
	std::string get_branch()
	{
		return RPCS3_GIT_BRANCH;
	}

	// TODO: Make this accessible from cmake and keep in sync with MACOSX_BUNDLE_BUNDLE_VERSION.
	// Currently accessible by Windows and Linux build scripts, see implementations when doing MACOSX
	const extern utils::version version{ 0, 0, 7, utils::version_type::alpha, 1, RPCS3_GIT_VERSION };
}
