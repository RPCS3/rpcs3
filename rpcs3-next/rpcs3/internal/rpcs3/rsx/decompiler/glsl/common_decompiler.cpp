#include "stdafx.h"
#include "GLCommonDecompiler.h"

std::string getFloatTypeNameImpl(size_t elementCount)
{
	switch (elementCount)
	{
	default:
		abort();
	case 1:
		return "float";
	case 2:
		return "vec2";
	case 3:
		return "vec3";
	case 4:
		return "vec4";
	}
}

std::string getFunctionImpl(FUNCTION f)
{
	switch (f)
	{
	default:
		abort();
	case FUNCTION::FUNCTION_DP2:
		return "vec4(dot($0.xy, $1.xy))";
	case FUNCTION::FUNCTION_DP2A:
		return "";
	case FUNCTION::FUNCTION_DP3:
		return "vec4(dot($0.xyz, $1.xyz))";
	case FUNCTION::FUNCTION_DP4:
		return "vec4(dot($0, $1))";
	case FUNCTION::FUNCTION_DPH:
		return "vec4(dot(vec4($0.xyz, 1.0), $1))";
	case FUNCTION::FUNCTION_SFL:
		return "vec4(0., 0., 0., 0.)";
	case FUNCTION::FUNCTION_STR:
		return "vec4(1., 1., 1., 1.)";
	case FUNCTION::FUNCTION_FRACT:
		return "fract($0)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE:
		return "texture($t, $0.xy)";
	case FUNCTION::FUNCTION_DFDX:
		return "dFdx($0)";
	case FUNCTION::FUNCTION_DFDY:
		return "dFdy($0)";
	}
}

std::string compareFunctionImpl(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	switch (f)
	{
	case COMPARE::FUNCTION_SEQ:
		return "equal(" + Op0 + ", " + Op1 + ")";
	case COMPARE::FUNCTION_SGE:
		return "greaterThanEqual(" + Op0 + ", " + Op1 + ")";
	case COMPARE::FUNCTION_SGT:
		return "greaterThan(" + Op0 + ", " + Op1 + ")";
	case COMPARE::FUNCTION_SLE:
		return "lessThanEqual(" + Op0 + ", " + Op1 + ")";
	case COMPARE::FUNCTION_SLT:
		return "lessThan(" + Op0 + ", " + Op1 + ")";
	case COMPARE::FUNCTION_SNE:
		return "notEqual(" + Op0 + ", " + Op1 + ")";
	}
}