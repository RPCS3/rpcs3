#pragma once

#include "../Common/ShaderParam.h"
#include "../Common/GLSLCommon.h"

std::string getFloatTypeNameImp(size_t elementCount);
std::string getFunctionImp(FUNCTION f);
std::string compareFunctionImp(COMPARE f, const std::string &Op0, const std::string &Op1);

void insert_d3d12_legacy_function(std::ostream&, bool is_fragment_program);
