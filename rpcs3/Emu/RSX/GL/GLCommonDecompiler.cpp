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
		return "vec4(dot($0.xy, $1.xy) + $2.x)";
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
	case FUNCTION::FUNCTION_REFL:
		return "vec4($0 - 2.0 * (dot($0, $1)) * $1)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D:
		return "texture($t, $0.x)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_PROJ:
		return "textureProj($t, $0.x, $1.x)"; // Note: $1.x is bias
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_LOD:
		return "textureLod($t, $0.x, $1)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_GRAD:
		return "textureGrad($t, $0.x, $1.x, $2.y)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D:
		return "texture($t, $0.xy * $t_coord_scale)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_PROJ:
		return "textureProj($t, $0.xyz * vec3($t_coord_scale, 1.) , $1.x)"; // Note: $1.x is bias
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_LOD:
		return "textureLod($t, $0.xy * $t_coord_scale, $1.x)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_GRAD:
		return "textureGrad($t, $0.xyz * vec3($t_coord_scale, 1.) , $1.x, $2.y)";		
	case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE:
		return "texture($t, $0.xyz)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_PROJ:
		return "textureProj($t, $0.xyzw, $1.x)"; // Note: $1.x is bias
	case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_LOD:
		return "textureLod($t, $0.xyz, $1.x)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_GRAD:
		return "textureGrad($t, $0.xyzw, $1.x, $2.y)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D:
		return "texture($t, $0.xyz)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_PROJ:
		return "textureProj($t, $0.xyzw, $1.x)"; // Note: $1.x is bias
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_LOD:
		return "textureLod($t, $0.xyz, $1.x)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_GRAD:
		return "textureGrad($t, $0.xyzw, $1.x, $2.y)";
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
	throw EXCEPTION("Unknow compare function");
}

void insert_glsl_legacy_function(std::ostream& OS)
{
	OS << "vec4 divsq_legacy(vec4 num, vec4 denum)\n";
	OS << "{\n";
	OS << "	return num / sqrt(max(denum.xxxx, 1.E-10));\n";
	OS << "}\n";

	OS << "vec4 rcp_legacy(vec4 denum)\n";
	OS << "{\n";
	OS << "	return 1. / denum;\n";
	OS << "}\n";

	OS << "vec4 rsq_legacy(vec4 val)\n";
	OS << "{\n";
	OS << "	return float(1.0 / sqrt(max(val.x, 1.E-10))).xxxx;\n";
	OS << "}\n\n";

	OS << "vec4 log2_legacy(vec4 val)\n";
	OS << "{\n";
	OS << "	return log2(max(val.x, 1.E-10)).xxxx;\n";
	OS << "}\n\n";

	OS << "vec4 lit_legacy(vec4 val)";
	OS << "{\n";
	OS << "	vec4 clamped_val = val;\n";
	OS << "	clamped_val.x = max(val.x, 0);\n";
	OS << "	clamped_val.y = max(val.y, 0);\n";
	OS << "	vec4 result;\n";
	OS << "	result.x = 1.0;\n";
	OS << "	result.w = 1.;\n";
	OS << "	result.y = clamped_val.x;\n";
	OS << "	result.z = clamped_val.x > 0.0 ? exp(clamped_val.w * log(max(clamped_val.y, 1.E-10))) : 0.0;\n";
	OS << "	return result;\n";
	OS << "}\n\n";
}
