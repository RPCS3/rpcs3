#pragma once
#include <string>
#include <cstdint>
#include <Utilities/version.h>

namespace rpcs3
{
	//warning this solves the order of initializatiob problem but not the order of deinitalization
	//see https://isocpp.org/wiki/faq/ctors#static-init-order for more info
	const utils::version& version();
}
