#pragma once

#include "../Common/ShaderParam.h"
#include "../Common/GLSLCommon.h"
#include <ostream>

namespace gl
{
	int get_varying_register_location(const std::string &var_name);
}
