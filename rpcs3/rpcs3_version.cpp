#include "stdafx.h"
#include "rpcs3_version.h"
#include "git-version.h"

namespace rpcs3
{
	const utils::version& version() {
		const static utils::version version{ 0, 0, 2, utils::version_type::alpha, 1, RPCS3_GIT_VERSION };

		return version;
	}
	
}
