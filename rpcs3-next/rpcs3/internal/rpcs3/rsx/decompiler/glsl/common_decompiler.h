#pragma once
#include "../Common/ShaderParam.h"

std::string getFloatTypeNameImpl(size_t elementCount);
std::string getFunctionImpl(FUNCTION f);
std::string compareFunctionImpl(COMPARE f, const std::string &Op0, const std::string &Op1);