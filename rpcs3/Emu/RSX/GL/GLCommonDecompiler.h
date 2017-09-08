#pragma once

#include "../Common/ShaderParam.h"
#include "../Common/GLSLCommon.h"
#include <ostream>

namespace gl
{
	std::string getFunctionImpl(FUNCTION f);
	int get_varying_register_location(const std::string &var_name);
}
