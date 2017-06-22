#pragma once

#include "../Common/ShaderParam.h"
#include "GLHelpers.h"
#include <ostream>

std::string getFloatTypeNameImpl(size_t elementCount);
std::string getFunctionImpl(FUNCTION f);
std::string compareFunctionImpl(COMPARE f, const std::string &Op0, const std::string &Op1);
void insert_glsl_legacy_function(std::ostream& OS, gl::glsl::program_domain domain);
