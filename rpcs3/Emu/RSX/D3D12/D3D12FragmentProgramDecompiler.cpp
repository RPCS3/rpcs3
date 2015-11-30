#include "stdafx.h"
#include "stdafx_d3d12.h"
#ifdef _MSC_VER
#include "D3D12FragmentProgramDecompiler.h"
#include "D3D12CommonDecompiler.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

D3D12FragmentDecompiler::D3D12FragmentDecompiler(u32 addr, u32& size, u32 ctrl) :
	FragmentProgramDecompiler(addr, size, ctrl)
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

	for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "float4", table[i].second))
			OS << "	" << "float4" << " " << table[i].first << " : SV_TARGET" << i << ";" << std::endl;
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
		if (PT.type == "sampler2D")
			continue;
		for (ParamItem PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << ";" << std::endl;
	}
	OS << "};" << std::endl << std::endl;

	for (ParamType PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type != "sampler2D")
			continue;
		for (ParamItem PI : PT.items)
		{
			size_t textureIndex = atoi(PI.name.data() + 3);
			OS << "Texture2D " << PI.name << " : register(t" << textureIndex << ");" << std::endl;
			OS << "sampler " << PI.name << "sampler : register(s" << textureIndex << ");" << std::endl;
		}
	}
}

void D3D12FragmentDecompiler::insertMainStart(std::stringstream & OS)
{
	OS << "PixelOutput main(PixelInput In)" << std::endl;
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
	const std::pair<std::string, std::string> table[] =
	{
		{ "ocol0", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r0" : "h0" },
		{ "ocol1", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r2" : "h4" },
		{ "ocol2", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r3" : "h6" },
		{ "ocol3", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r4" : "h8" },
	};

	OS << "	PixelOutput Out;" << std::endl;
	size_t num_output = 0;
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
