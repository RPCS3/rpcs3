#pragma once
#include "../Common/ShaderParam.h"

std::string getFloatTypeNameImp(size_t elementCount);
std::string getFunctionImp(FUNCTION f);
std::string compareFunctionImp(COMPARE f, const std::string &Op0, const std::string &Op1);