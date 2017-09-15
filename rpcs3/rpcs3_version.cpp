#include "stdafx.h"
#include "rpcs3_version.h"
#include "git-version.h"

namespace rpcs3
{
	std::string get_branch()
	{
		return RPCS3_GIT_BRANCH;
	}

	const extern utils::version version{ 0, 0, 3, utils::version_type::alpha, 1, RPCS3_GIT_VERSION };
}
