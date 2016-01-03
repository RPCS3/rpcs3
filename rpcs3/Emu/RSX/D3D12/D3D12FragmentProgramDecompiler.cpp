#include "stdafx.h"
#include "stdafx_d3d12.h"
#ifdef _MSC_VER
#include "D3D12FragmentProgramDecompiler.h"
#include "D3D12CommonDecompiler.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

D3D12FragmentDecompiler::D3D12FragmentDecompiler(u32 addr, u32& size, u32 ctrl, const std::vector<texture_dimension> &texture_dimensions) :
	FragmentProgramDecompiler(addr, size, ctrl, texture_dimensions)
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
	OS << "cbuffer SCALE_OFFSET : register(b0)" << std::endl;
	OS << "{" << std::endl;
	OS << "	float4x4 scaleOffsetMat;" << std::endl;
	OS << "	int isAlphaTested;" << std::endl;
	OS << "	float alphaRef;" << std::endl;
	OS << "	int tex0_is_unorm;" << std::endl;
	OS << "	int tex1_is_unorm;" << std::endl;
	OS << "	int tex2_is_unorm;" << std::endl;
	OS << "	int tex3_is_unorm;" << std::endl;
	OS << "	int tex4_is_unorm;" << std::endl;
	OS << "	int tex5_is_unorm;" << std::endl;
	OS << "	int tex6_is_unorm;" << std::endl;
	OS << "	int tex7_is_unorm;" << std::endl;
	OS << "	int tex8_is_unorm;" << std::endl;
	OS << "	int tex9_is_unorm;" << std::endl;
	OS << "	int tex10_is_unorm;" << std::endl;
	OS << "	int tex11_is_unorm;" << std::endl;
	OS << "	int tex12_is_unorm;" << std::endl;
	OS << "	int tex13_is_unorm;" << std::endl;
	OS << "	int tex14_is_unorm;" << std::endl;
	OS << "	int tex15_is_unorm;" << std::endl;
	OS << "};" << std::endl;
}

void D3D12FragmentDecompiler::insertIntputs(std::stringstream & OS)
{
	OS << "struct PixelInput" << std::endl;
	OS << "{" << std::endl;
	OS << "	float4 Position : SV_POSITION;" << std::endl;
	OS << "	float4 diff_color : COLOR0;" << std::endl;
	OS << "	float4 spec_color : COLOR1;" << std::endl;
	OS << "	float4 dst_reg3 : COLOR2;" << std::endl;
	OS << "	float4 dst_reg4 : COLOR3;" << std::endl;
	OS << "	float fogc : FOG;" << std::endl;
	OS << "	float4 tc9 : TEXCOORD9;" << std::endl;
	OS << "	float4 tc0 : TEXCOORD0;" << std::endl;
	OS << "	float4 tc1 : TEXCOORD1;" << std::endl;
	OS << "	float4 tc2 : TEXCOORD2;" << std::endl;
	OS << "	float4 tc3 : TEXCOORD3;" << std::endl;
	OS << "	float4 tc4 : TEXCOORD4;" << std::endl;
	OS << "	float4 tc5 : TEXCOORD5;" << std::endl;
	OS << "	float4 tc6 : TEXCOORD6;" << std::endl;
	OS << "	float4 tc7 : TEXCOORD7;" << std::endl;
	OS << "	float4 tc8 : TEXCOORD8;" << std::endl;
	OS << "};" << std::endl;
}

void D3D12FragmentDecompiler::insertOutputs(std::stringstream & OS)
{
	OS << "struct PixelOutput" << std::endl;
	OS << "{" << std::endl;
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
			OS << "	" << "float4" << " " << table[i].first << " : SV_TARGET" << idx++ << ";" << std::endl;
	}
	if (m_ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT)
		OS << "	float depth : SV_Depth;" << std::endl;
	OS << "};" << std::endl;
}

void D3D12FragmentDecompiler::insertConstants(std::stringstream & OS)
{
	OS << "cbuffer CONSTANT : register(b2)" << std::endl;
	OS << "{" << std::endl;
	for (ParamType PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type == "sampler2D" || PT.type == "samplerCube")
			continue;
		for (ParamItem PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << ";" << std::endl;
	}
	OS << "};" << std::endl << std::endl;

	for (ParamType PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type == "sampler2D")
		{
			for (ParamItem PI : PT.items)
			{
				size_t textureIndex = atoi(PI.name.data() + 3);
				OS << "Texture2D " << PI.name << " : register(t" << textureIndex << ");" << std::endl;
				OS << "sampler " << PI.name << "sampler : register(s" << textureIndex << ");" << std::endl;
			}
		}
		else if (PT.type == "samplerCube")
		{
			for (ParamItem PI : PT.items)
			{
				size_t textureIndex = atoi(PI.name.data() + 3);
				OS << "TextureCube " << PI.name << " : register(t" << textureIndex << ");" << std::endl;
				OS << "sampler " << PI.name << "sampler : register(s" << textureIndex << ");" << std::endl;
			}
		}
	}
}

void D3D12FragmentDecompiler::insertMainStart(std::stringstream & OS)
{
	// "lib" function
	// 0.00001 is used as "some non zero very little number"
	OS << "float4 divsq_legacy(float4 num, float4 denum)\n";
	OS << "{\n";
	OS << "	return num / sqrt(max(denum.xxxx, 0.00001));\n";
	OS << "}\n";

	OS << "float4 rcp_legacy(float4 denum)\n";
	OS << "{\n";
	OS << "	return 1. / denum;\n";
	OS << "}\n";

	OS << "float4 rsq_legacy(float4 denum)\n";
	OS << "{\n";
	OS << "	return 1. / sqrt(max(denum, 0.00001));\n";
	OS << "}\n";


	const std::set<std::string> output_value =
	{
		"r0", "r1", "r2", "r3", "r4",
		"h0", "h4", "h6", "h8"
	};
	OS << "void ps_impl(PixelInput In, inout float4 r0, inout float4 h0, inout float4 r1, inout float4 r2, inout float4 h4, inout float4 r3, inout float4 h6, inout float4 r4, inout float4 h8)" << std::endl;
	OS << "{" << std::endl;
	for (ParamType PT : m_parr.params[PF_PARAM_IN])
	{
		for (ParamItem PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << " = In." << PI.name << ";" << std::endl;
	}
	// A bit unclean, but works.
	OS << "	" << "float4 gl_Position = In.Position;" << std::endl;
	// Declare output
	for (ParamType PT : m_parr.params[PF_PARAM_NONE])
	{
		for (ParamItem PI : PT.items)
			if (output_value.find(PI.name) == output_value.end())
				OS << "	" << PT.type << " " << PI.name << " = float4(0., 0., 0., 0.);" << std::endl;
	}
	// Declare texture coordinate scaling component (to handle unormalized texture coordinates)

	for (ParamType PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type != "sampler2D")
			continue;
		for (const ParamItem& PI : PT.items)
		{
			size_t textureIndex = atoi(PI.name.data() + 3);
			OS << "	float2  " << PI.name << "_dim;" << std::endl;
			OS << "	" << PI.name << ".GetDimensions(" << PI.name << "_dim.x, " << PI.name << "_dim.y);" << std::endl;
			OS << "	float2  " << PI.name << "_scale = (!!" << PI.name << "_is_unorm) ? float2(1., 1.) / " << PI.name << "_dim : float2(1., 1.);" << std::endl;
		}
	}
}

void D3D12FragmentDecompiler::insertMainEnd(std::stringstream & OS)
{
	OS << "}" << std::endl;
	OS << std::endl;
	OS << "PixelOutput main(PixelInput In)" << std::endl;
	OS << "{" << std::endl;
	OS << "	float4 r0 = float4(0., 0., 0., 0.);" << std::endl;
	OS << "	float4 r1 = float4(0., 0., 0., 0.);" << std::endl;
	OS << "	float4 r2 = float4(0., 0., 0., 0.);" << std::endl;
	OS << "	float4 r3 = float4(0., 0., 0., 0.);" << std::endl;
	OS << "	float4 r4 = float4(0., 0., 0., 0.);" << std::endl;
	OS << "	float4 h0 = float4(0., 0., 0., 0.);" << std::endl;
	OS << "	float4 h2 = float4(0., 0., 0., 0.);" << std::endl;
	OS << "	float4 h4 = float4(0., 0., 0., 0.);" << std::endl;
	OS << "	float4 h6 = float4(0., 0., 0., 0.);" << std::endl;
	OS << "	float4 h8 = float4(0., 0., 0., 0.);" << std::endl;
	OS << "	ps_impl(In, r0, h0, r1, r2, h4, r3, h6, r4, h8);" << std::endl;

	const std::pair<std::string, std::string> table[] =
	{
		{ "ocol0", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r0" : "h0" },
		{ "ocol1", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r2" : "h4" },
		{ "ocol2", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r3" : "h6" },
		{ "ocol3", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r4" : "h8" },
	};

	size_t num_output = 0;
	OS << "	PixelOutput Out = (PixelOutput)0;" << std::endl;
	for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "float4", table[i].second))
		{
			OS << "	Out." << table[i].first << " = " << table[i].second << ";" << std::endl;
			num_output++;
		}
	}
	if (m_ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT)
		OS << "	Out.depth = " << ((m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS) ? "r1.z;" : "h0.z;") << std::endl;
	// Shaders don't always output colors (for instance if they write to depth only)
	if (num_output > 0)
		OS << "	if (isAlphaTested && Out.ocol0.a <= alphaRef) discard;" << std::endl;
	OS << "	return Out;" << std::endl;
	OS << "}" << std::endl;
}
#endif
