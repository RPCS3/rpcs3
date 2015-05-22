#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "D3D12FragmentProgramDecompiler.h"

#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

D3D12FragmentDecompiler::D3D12FragmentDecompiler(u32 addr, u32& size, u32 ctrl) :
	FragmentProgramDecompiler(addr, size, ctrl)
{

}

std::string D3D12FragmentDecompiler::getFloatTypeName(size_t elementCount)
{
	switch (elementCount)
	{
	default:
		abort();
	case 1:
		return "float";
	case 2:
		return "float2";
	case 3:
		return "float3";
	case 4:
		return "float4";
	}
}

std::string D3D12FragmentDecompiler::getFunction(enum class FUNCTION f)
{
	switch (f)
	{
	default:
		abort();
	case FUNCTION::FUNCTION_DP2:
		return "dot($0.xy, $1.xy).xxxx";
	case FUNCTION::FUNCTION_DP2A:
		return "(dot($0.xy, $1.xy) + $2.x).xxxx";
	case FUNCTION::FUNCTION_DP3:
		return "dot($0.xyz, $1.xyz).xxxx";
	case FUNCTION::FUNCTION_DP4:
		return "dot($0, $1).xxxx";
	case FUNCTION::FUNCTION_SFL:
		return "float4(0., 0., 0., 0.)";
	case FUNCTION::FUNCTION_STR:
		return "float4(1., 1., 1., 1.)";
	case FUNCTION::FUNCTION_FRACT:
		return "frac($0)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE:
		return "$t.Sample($tsampler, $0.xy)";
	case FUNCTION::FUNCTION_DFDX:
		return "ddx($0)";
	case FUNCTION::FUNCTION_DFDY:
		return "ddy($0)";
	}
}

std::string D3D12FragmentDecompiler::saturate(const std::string & code)
{
	return "saturate(" + code + ")";
}

std::string D3D12FragmentDecompiler::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	switch (f)
	{
	default:
		abort();
	case COMPARE::FUNCTION_SEQ:
		return "(" + Op0 + " == " + Op1 + ")";
	case COMPARE::FUNCTION_SGE:
		return "(" + Op0 + " >= " + Op1 +")";
	case COMPARE::FUNCTION_SGT:
		return "(" + Op0 + " > " + Op1 + ")";
	case COMPARE::FUNCTION_SLE:
		return "(" + Op0 + " <= " + Op1 + ")";
	case COMPARE::FUNCTION_SLT:
		return "(" + Op0 + " < " + Op1 + ")";
	case COMPARE::FUNCTION_SNE:
		return "(" + Op0 + " != " + Op1 + ")";
	}
}

void D3D12FragmentDecompiler::insertHeader(std::stringstream & OS)
{
	OS << "// Header" << std::endl;
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
	OS << "	float4 dummy : COLOR4;" << std::endl;
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
		{ "ocol0", "r0" },
		{ "ocol1", "r2" },
		{ "ocol2", "r3" },
		{ "ocol3", "r4" },
	};

	for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "float4", table[i].second))
			OS << "	" << "float4" << " " << table[i].first << " : SV_TARGET" << i << ";" << std::endl;
	}
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
	size_t textureIndex = 0;
	for (ParamType PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type != "sampler2D")
			continue;
		for (ParamItem PI : PT.items)
		{
			OS << "Texture2D " << PI.name << " : register(t" << textureIndex << ");" << std::endl;
			OS << "sampler " << PI.name << "sampler : register(s" << textureIndex << ");" << std::endl;
			textureIndex++;
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
	// Declare output
	for (ParamType PT : m_parr.params[PF_PARAM_NONE])
	{
		for (ParamItem PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << " = float4(0., 0., 0., 0.);" << std::endl;
	}
}

void D3D12FragmentDecompiler::insertMainEnd(std::stringstream & OS)
{
	const std::pair<std::string, std::string> table[] =
	{
		{ "ocol0", "r0" },
		{ "ocol1", "r2" },
		{ "ocol2", "r3" },
		{ "ocol3", "r4" },
	};

	OS << "	PixelOutput Out;" << std::endl;
	for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "float4", table[i].second))
			OS << "	Out." << table[i].first << " = " << table[i].second << ";" << std::endl;
	}
	OS << "	return Out;" << std::endl;
	OS << "}" << std::endl;
}

#endif