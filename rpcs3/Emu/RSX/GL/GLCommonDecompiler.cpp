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
		return "textureLod($t, $0.x, $1.x)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_GRAD:
		return "textureGrad($t, $0.x, $1.x, $2.x)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D:
		return "texture($t, $0.xy * $t_coord_scale)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_PROJ:
		return "textureProj($t, $0 , $1.x)"; // Note: $1.x is bias
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_LOD:
		return "textureLod($t, $0.xy * $t_coord_scale, $1.x)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_GRAD:
		return "textureGrad($t, $0.xy * $t_coord_scale , $1.xy, $2.xy)";		
	case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE:
		return "texture($t, $0.xyz)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_PROJ:
		return "texture($t, ($0.xyz / $0.w))";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_LOD:
		return "textureLod($t, $0.xyz, $1.x)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_GRAD:
		return "textureGrad($t, $0.xyz, $1.xyz, $2.xyz)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D:
		return "texture($t, $0.xyz)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_PROJ:
		return "textureProj($t, $0.xyzw, $1.x)"; // Note: $1.x is bias
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_LOD:
		return "textureLod($t, $0.xyz, $1.x)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_GRAD:
		return "textureGrad($t, $0.xyz, $1.xyz, $2.xyz)";
	case FUNCTION::FUNCTION_DFDX:
		return "dFdx($0)";
	case FUNCTION::FUNCTION_DFDY:
		return "dFdy($0)";
	case FUNCTION::FUNCTION_VERTEX_TEXTURE_FETCH2D:
		return "textureLod($t, $0.xy, 0)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_DEPTH_RGBA:
		return "texture2DReconstruct($t, $0.xy * $t_coord_scale)";
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
	fmt::throw_exception("Unknown compare function" HERE);
}

void insert_glsl_legacy_function(std::ostream& OS, gl::glsl::program_domain domain)
{
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

	if (domain != gl::glsl::program_domain::glsl_fragment_program)
		return;

	//NOTE: We lose precision if we just store depth value into 8-bit textures i.e (depth, 0, 0)
	//NOTE2: After testing with GOW, the w component is either the original depth or wraps around to the x component
	//Since component.r == depth_value with some precision loss, just use the precise depth value for now (further testing needed)
	OS << "vec4 texture2DReconstruct(sampler2D tex, vec2 coord)\n";
	OS << "{\n";
	OS << "	float depth_value = texture(tex, coord.xy).r;\n";
	OS << "	uint value = uint(depth_value * 16777215);\n";
	OS << "	uint b = (value & 0xff);\n";
	OS << "	uint g = (value >> 8) & 0xff;\n";
	OS << "	uint r = (value >> 16) & 0xff;\n";
	OS << "	return vec4(float(r)/255., float(g)/255., float(b)/255., depth_value);\n";
	OS << "}\n\n";
}
