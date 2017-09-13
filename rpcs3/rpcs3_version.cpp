#include "stdafx.h"
#include "rpcs3_version.h"
#include "git-version.h"

char const* get_branch()
{
	return RPCS3_GIT_BRANCH;
}

namespace rpcs3
{
	const extern utils::version version{ 0, 0, 3, utils::version_type::alpha, 1, RPCS3_GIT_VERSION };
	const char* branch{get_branch()};
}
