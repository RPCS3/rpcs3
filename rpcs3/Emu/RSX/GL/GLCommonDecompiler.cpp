#include "stdafx.h"
#include "GLCommonDecompiler.h"


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
