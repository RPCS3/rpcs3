#ifdef _MSC_VER
#include "stdafx.h"
#include "stdafx_d3d12.h"
#include "D3D12FragmentProgramDecompiler.h"
#include "D3D12CommonDecompiler.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include <set>

D3D12FragmentDecompiler::D3D12FragmentDecompiler(const RSXFragmentProgram &prog, u32& size) :
	FragmentProgramDecompiler(prog, size)
{

}

std::string D3D12FragmentDecompiler::getFloatTypeName(size_t elementCount)
{
	return getFloatTypeNameImp(elementCount);
}

std::string D3D12FragmentDecompiler::getFunction(enum class FUNCTION f)
{
	return getFunctionImp(f);
}

std::string D3D12FragmentDecompiler::saturate(const std::string & code)
{
	return "saturate(" + code + ")";
}

std::string D3D12FragmentDecompiler::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	return compareFunctionImp(f, Op0, Op1);
}

void D3D12FragmentDecompiler::insertHeader(std::stringstream & OS)
{
	OS << "#define floatBitsToUint asuint\n";
	OS << "#define uintBitsToFloat asfloat\n\n";

	OS << "cbuffer SCALE_OFFSET : register(b0)\n";
	OS << "{\n";
	OS << "	float4x4 scaleOffsetMat;\n";
	OS << "	int4 userClipEnabled[2];\n";
	OS << "	float4 userClipFactor[2];\n";
	OS << "	float fog_param0;\n";
	OS << "	float fog_param1;\n";
	OS << "	uint alpha_test;\n";
	OS << "	float alpha_ref;\n";
	OS << "	uint alpha_func;\n";
	OS << "	uint fog_mode;\n";
	OS << "	float wpos_scale;\n";
	OS << "	float wpos_bias;\n";
	OS << "	float4 texture_parameters[16];\n";
	OS << "};\n";
}

void D3D12FragmentDecompiler::insertIntputs(std::stringstream & OS)
{
	OS << "struct PixelInput\n";
	OS << "{\n";
	OS << "	float4 Position : SV_POSITION;\n";
	OS << "	float4 diff_color : COLOR0;\n";
	OS << "	float4 spec_color : COLOR1;\n";
	OS << "	float4 dst_reg3 : COLOR2;\n";
	OS << "	float4 dst_reg4 : COLOR3;\n";
	OS << "	float4 fogc : FOG;\n";
	OS << "	float4 tc9 : TEXCOORD9;\n";
	OS << "	float4 tc0 : TEXCOORD0;\n";
	OS << "	float4 tc1 : TEXCOORD1;\n";
	OS << "	float4 tc2 : TEXCOORD2;\n";
	OS << "	float4 tc3 : TEXCOORD3;\n";
	OS << "	float4 tc4 : TEXCOORD4;\n";
	OS << "	float4 tc5 : TEXCOORD5;\n";
	OS << "	float4 tc6 : TEXCOORD6;\n";
	OS << "	float4 tc7 : TEXCOORD7;\n";
	OS << "	float4 tc8 : TEXCOORD8;\n";
	OS << "	float4 dst_userClip0 : SV_ClipDistance0;\n";
	OS << "	float4 dst_userClip1 : SV_ClipDistance1;\n";
	OS << "};\n";
}

void D3D12FragmentDecompiler::insertOutputs(std::stringstream & OS)
{
	OS << "struct PixelOutput\n";
	OS << "{\n";
	const std::pair<std::string, std::string> table[] =
	{
		{ "ocol0", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r0" : "h0" },
		{ "ocol1", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r2" : "h4" },
		{ "ocol2", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r3" : "h6" },
		{ "ocol3", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r4" : "h8" },
	};
	size_t idx = 0;
	for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "float4", table[i].second))
			OS << "	" << "float4" << " " << table[i].first << " : SV_TARGET" << idx++ << ";\n";
	}
	if (m_ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT)
		OS << "	float depth : SV_Depth;\n";
	OS << "};\n";
}

void D3D12FragmentDecompiler::insertConstants(std::stringstream & OS)
{
	OS << "cbuffer CONSTANT : register(b2)\n";
	OS << "{\n";
	for (const ParamType &PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type == "sampler1D" || PT.type == "sampler2D" || PT.type == "samplerCube" || PT.type == "sampler3D")
			continue;
		for (const ParamItem &PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << ";\n";
	}
	OS << "};\n\n";

	for (const ParamType &PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type == "sampler1D")
		{
			for (const ParamItem &PI : PT.items)
			{
				size_t textureIndex = atoi(PI.name.data() + 3);
				OS << "Texture1D " << PI.name << " : register(t" << textureIndex << ");\n";
				OS << "sampler " << PI.name << "sampler : register(s" << textureIndex << ");\n";
			}
		}
		else if (PT.type == "sampler2D")
		{
			for (const ParamItem &PI : PT.items)
			{
				size_t textureIndex = atoi(PI.name.data() + 3);
				OS << "Texture2D " << PI.name << " : register(t" << textureIndex << ");\n";
				OS << "sampler " << PI.name << "sampler : register(s" << textureIndex << ");\n";
			}
		}
		else if (PT.type == "sampler3D")
		{
			for (const ParamItem &PI : PT.items)
			{
				size_t textureIndex = atoi(PI.name.data() + 3);
				OS << "Texture3D " << PI.name << " : register(t" << textureIndex << ");\n";
				OS << "sampler " << PI.name << "sampler : register(s" << textureIndex << ");\n";
			}
		}
		else if (PT.type == "samplerCube")
		{
			for (const ParamItem &PI : PT.items)
			{
				size_t textureIndex = atoi(PI.name.data() + 3);
				OS << "TextureCube " << PI.name << " : register(t" << textureIndex << ");\n";
				OS << "sampler " << PI.name << "sampler : register(s" << textureIndex << ");\n";
			}
		}
	}
}

namespace
{
	std::string insert_texture_fetch(const RSXFragmentProgram& prog, int index)
	{
		std::string tex_name = "tex" + std::to_string(index) + ".Sample";
		std::string sampler_name = "tex" + std::to_string(index) + "sampler";
		std::string coord_name = "In.tc" + std::to_string(index);

		switch (prog.get_texture_dimension(index))
		{
		case rsx::texture_dimension_extended::texture_dimension_1d: return " " + tex_name + " (" + sampler_name + ", " + coord_name + ".x) ";
		case rsx::texture_dimension_extended::texture_dimension_2d: return " " + tex_name + " (" + sampler_name + ", " + coord_name + ".xy) ";
		case rsx::texture_dimension_extended::texture_dimension_3d:
		case rsx::texture_dimension_extended::texture_dimension_cubemap: return " " + tex_name + " (" + sampler_name + ", " + coord_name + ".xyz) ";
		}

		fmt::throw_exception("Invalid texture dimension %d" HERE, (u32)prog.get_texture_dimension(index));
	}
}

void D3D12FragmentDecompiler::insertGlobalFunctions(std::stringstream &OS)
{
	insert_d3d12_legacy_function(OS, true);
}

void D3D12FragmentDecompiler::insertMainStart(std::stringstream & OS)
{
	for (const ParamType &PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem &PI : PT.items)
		{
			if (PI.name == "fogc")
			{
				program_common::insert_fog_declaration(OS, "float4", "fogc", true);
				break;
			}
		}
	}

	const std::set<std::string> output_value =
	{
		"r0", "r1", "r2", "r3", "r4",
		"h0", "h2", "h4", "h6", "h8"
	};
	OS << "void ps_impl(bool is_front_face, PixelInput In, inout float4 r0, inout float4 h0, inout float4 r1, inout float4 h2, inout float4 r2, inout float4 h4, inout float4 r3, inout float4 h6, inout float4 r4, inout float4 h8)\n";
	OS << "{\n";
	for (const ParamType &PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem &PI : PT.items)
		{
			if (m_prog.front_back_color_enabled)
			{
				if (PI.name == "spec_color" && m_prog.back_color_specular_output)
				{
					OS << "	float4 spec_color = is_front_face ? In.dst_reg4 : In.spec_color;\n";
					continue;
				}
				if (PI.name == "diff_color" && m_prog.back_color_diffuse_output)
				{
					OS << "	float4 diff_color = is_front_face ? In.dst_reg3 : In.diff_color;\n";
					continue;
				}
			}
			if (PI.name == "fogc")
			{
				OS << "	float4 fogc = fetch_fog_value(fog_mode, In.fogc);\n";
				continue;
			}
			if (PI.name == "ssa")
				continue;
			OS << "	" << PT.type << " " << PI.name << " = In." << PI.name << ";\n";
		}
	}

	//NOTE: Framebuffer scaling not actually supported. wpos_scale is used to reconstruct the true window height
	OS << "	float4 wpos = In.Position;\n";
	OS << " float4 res_scale = abs(1.f / wpos_scale);\n";
	OS << "	if (wpos_scale < 0) wpos.y = (wpos_bias * res_scale) - wpos.y;\n";
	OS << "	float4 ssa = is_front_face ? float4(1., 1., 1., 1.) : float4(-1., -1., -1., -1.);\n";

	// Declare output
	for (const ParamType &PT : m_parr.params[PF_PARAM_NONE])
	{
		for (const ParamItem &PI : PT.items)
			if (output_value.find(PI.name) == output_value.end())
				OS << "	" << PT.type << " " << PI.name << " = float4(0., 0., 0., 0.);\n";
	}
	// Declare texture coordinate scaling component (to handle unormalized texture coordinates)

	for (const ParamType &PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type != "sampler2D")
			continue;
		for (const ParamItem& PI : PT.items)
		{
			size_t textureIndex = atoi(PI.name.data() + 3);
			bool is_unorm = !!(m_prog.unnormalized_coords & (1 << textureIndex));
			if (!is_unorm)
			{
				OS << "	float2  " << PI.name << "_scale = float2(1., 1.);\n";
				continue;
			}

			OS << "	float2  " << PI.name << "_dim;\n";
			OS << "	" << PI.name << ".GetDimensions(" << PI.name << "_dim.x, " << PI.name << "_dim.y);\n";
			OS << "	float2  " << PI.name << "_scale = texture_parameters[" << textureIndex << "] / " << PI.name << "_dim;\n";
		}
	}
}

void D3D12FragmentDecompiler::insertMainEnd(std::stringstream & OS)
{
	OS << "}\n";
	OS << "\n";
	OS << "PixelOutput main(PixelInput In, bool is_front_face : SV_IsFrontFace)\n";
	OS << "{\n";
	OS << "	float4 r0 = float4(0., 0., 0., 0.);\n";
	OS << "	float4 r1 = float4(0., 0., 0., 0.);\n";
	OS << "	float4 r2 = float4(0., 0., 0., 0.);\n";
	OS << "	float4 r3 = float4(0., 0., 0., 0.);\n";
	OS << "	float4 r4 = float4(0., 0., 0., 0.);\n";
	OS << "	float4 h0 = float4(0., 0., 0., 0.);\n";
	OS << "	float4 h2 = float4(0., 0., 0., 0.);\n";
	OS << "	float4 h4 = float4(0., 0., 0., 0.);\n";
	OS << "	float4 h6 = float4(0., 0., 0., 0.);\n";
	OS << "	float4 h8 = float4(0., 0., 0., 0.);\n";
	OS << "	ps_impl(is_front_face, In, r0, h0, r1, h2, r2, h4, r3, h6, r4, h8);\n";

	const std::pair<std::string, std::string> table[] =
	{
		{ "ocol0", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r0" : "h0" },
		{ "ocol1", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r2" : "h4" },
		{ "ocol2", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r3" : "h6" },
		{ "ocol3", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r4" : "h8" },
	};

	std::string first_output_name;
	OS << "	PixelOutput Out = (PixelOutput)0;\n";
	for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "float4", table[i].second))
		{
			OS << "	Out." << table[i].first << " = " << table[i].second << ";\n";
			if (first_output_name.empty()) first_output_name = table[i].first;
		}
	}
	if (m_ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", "r1"))
		{
			/**
			 * Note: Naruto Shippuden : Ultimate Ninja Storm 2 sets CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS in a shader
			 * but it writes depth in r1.z and not h2.z.
			 * Maybe there's a different flag for depth ?
			 */
			 //		OS << "	Out.depth = " << ((m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS) ? "r1.z;" : "h2.z;") << "\n";
			OS << "	Out.depth = r1.z;\n";
		}
		else
		{
			//Input not declared. Leave commented to assist in debugging the shader
			OS << "	//Out.depth = r1.z;\n";
		}
	}
	// Shaders don't always output colors (for instance if they write to depth only)
	if (!first_output_name.empty())
	{
		auto make_comparison_test = [](rsx::comparison_function compare_func, const std::string &test, const std::string &a, const std::string &b) -> std::string
		{
			std::string compare;
			switch (compare_func)
			{
			case rsx::comparison_function::equal:            compare = " == "; break;
			case rsx::comparison_function::not_equal:        compare = " != "; break;
			case rsx::comparison_function::less_or_equal:    compare = " <= "; break;
			case rsx::comparison_function::less:             compare = " < ";  break;
			case rsx::comparison_function::greater:          compare = " > ";  break;
			case rsx::comparison_function::greater_or_equal: compare = " >= "; break;
			default:
				return "";
			}

			return "	if (" + test + "!(" + a + compare + b + ")) discard;\n";
		};

		for (u8 index = 0; index < 16; ++index)
		{
			if (m_prog.textures_alpha_kill[index])
			{
				const std::string texture_name = "tex" + std::to_string(index);
				if (m_parr.HasParamTypeless(PF_PARAM_UNIFORM, texture_name))
				{
					std::string fetch_texture = insert_texture_fetch(m_prog, index) + ".a";
					OS << make_comparison_test((rsx::comparison_function)m_prog.textures_zfunc[index], "", "0", fetch_texture);
				}
			}
		}

		OS << "	if (alpha_test != 0 && !comparison_passes(Out." << first_output_name << ".a, alpha_ref, alpha_func)) discard;\n";
		
	}
	OS << "	return Out;\n";
	OS << "}\n";
}
#endif
