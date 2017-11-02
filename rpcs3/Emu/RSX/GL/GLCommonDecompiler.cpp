#include "stdafx.h"
#include "GLCommonDecompiler.h"


namespace gl
{
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
			return "texture($t, $0.xy * texture_parameters[$_i].xy)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_PROJ:
			return "textureProj($t, $0 , $1.x)"; // Note: $1.x is bias
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_LOD:
			return "textureLod($t, $0.xy * texture_parameters[$_i].xy, $1.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_GRAD:
			return "textureGrad($t, $0.xy * texture_parameters[$_i].xy , $1.xy, $2.xy)";
		case FUNCTION::FUNCTION_TEXTURE_SHADOW2D:
			return "texture($t, $0.xyz)";
		case FUNCTION::FUNCTION_TEXTURE_SHADOW2D_PROJ:
			return "textureProj($t, $0, $1.x)"; // Note: $1.x is bias
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
			return "texture2DReconstruct($t, $0.xy * texture_parameters[$_i].xy)";
		}
	}

	int get_varying_register_location(const std::string &var_name)
	{
		static const std::pair<std::string, int> reg_table[] =
		{
			{ "diff_color", 1 },
			{ "spec_color", 2 },
			{ "back_diff_color", 1 },
			{ "back_spec_color", 2 },
			{ "front_diff_color", 3 },
			{ "front_spec_color", 4 },
			{ "fog_c", 5 },
			{ "tc0", 6 },
			{ "tc1", 7 },
			{ "tc2", 8 },
			{ "tc3", 9 },
			{ "tc4", 10 },
			{ "tc5", 11 },
			{ "tc6", 12 },
			{ "tc7", 13 },
			{ "tc8", 14 },
			{ "tc9", 15 }
		};

		for (const auto& v: reg_table)
		{
			if (v.first == var_name)
				return v.second;
		}

		fmt::throw_exception("register named %s should not be declared!", var_name.c_str());
	}
}
