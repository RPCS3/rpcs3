#include "stdafx.h"
#include "rpcs3_version.h"
#include "git-version.h"

namespace rpcs3
{
	const utils::version version = utils::version{ 0, 0, 1 }
		.type(utils::version_type::pre_alpha)
		.postfix(RPCS3_GIT_VERSION);
}
