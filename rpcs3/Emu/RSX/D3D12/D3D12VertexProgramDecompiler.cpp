#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "D3D12VertexProgramDecompiler.h"
#include "D3D12CommonDecompiler.h"
#include "Utilities/Log.h"
#include "Emu/System.h"


std::string D3D12VertexProgramDecompiler::getFloatTypeName(size_t elementCount)
{
	return getFloatTypeNameImp(elementCount);
}

std::string D3D12VertexProgramDecompiler::getFunction(enum class FUNCTION f)
{
	return getFunctionImp(f);
}

std::string D3D12VertexProgramDecompiler::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	return compareFunctionImp(f, Op0, Op1);
}

void D3D12VertexProgramDecompiler::insertHeader(std::stringstream &OS)
{
	OS << "cbuffer SCALE_OFFSET : register(b0)" << std::endl;
	OS << "{" << std::endl;
	OS << "	float4x4 scaleOffsetMat;" << std::endl;
	OS << "	int isAlphaTested;" << std::endl;
	OS << "	float alphaRef;" << std::endl;
	OS << "};" << std::endl;
}

void D3D12VertexProgramDecompiler::insertInputs(std::stringstream & OS, const std::vector<ParamType>& inputs)
{
	OS << "struct VertexInput" << std::endl;
	OS << "{" << std::endl;
	for (const ParamType PT : inputs)
	{
		for (const ParamItem &PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << ": TEXCOORD" << PI.location << ";" << std::endl;
	}
	OS << "};" << std::endl;
}

void D3D12VertexProgramDecompiler::insertConstants(std::stringstream & OS, const std::vector<ParamType> & constants)
{
	OS << "cbuffer CONSTANT_BUFFER : register(b1)" << std::endl;
	OS << "{" << std::endl;
	for (const ParamType PT : constants)
	{
		for (const ParamItem &PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << ";" << std::endl;
	}
	OS << "};" << std::endl;
}

void D3D12VertexProgramDecompiler::insertOutputs(std::stringstream & OS, const std::vector<ParamType> & outputs)
{
	OS << "struct PixelInput" << std::endl;
	OS << "{" << std::endl;
	OS << "	float4 dst_reg0 : SV_POSITION;" << std::endl;
	OS << "	float4 dst_reg1 : COLOR0;" << std::endl;
	OS << "	float4 dst_reg2 : COLOR1;" << std::endl;
	OS << "	float4 dst_reg3 : COLOR2;" << std::endl;
	OS << "	float4 dst_reg4 : COLOR3;" << std::endl;
	OS << "	float dst_reg5 : FOG;" << std::endl;
	OS << "	float4 dst_reg6 : COLOR4;" << std::endl;
	OS << "	float4 dst_reg7 : TEXCOORD0;" << std::endl;
	OS << "	float4 dst_reg8 : TEXCOORD1;" << std::endl;
	OS << "	float4 dst_reg9 : TEXCOORD2;" << std::endl;
	OS << "	float4 dst_reg10 : TEXCOORD3;" << std::endl;
	OS << "	float4 dst_reg11 : TEXCOORD4;" << std::endl;
	OS << "	float4 dst_reg12 : TEXCOORD5;" << std::endl;
	OS << "	float4 dst_reg13 : TEXCOORD6;" << std::endl;
	OS << "	float4 dst_reg14 : TEXCOORD7;" << std::endl;
	OS << "	float4 dst_reg15 : TEXCOORD8;" << std::endl;
	OS << "};" << std::endl;
}

struct reg_info
{
	std::string name;
	bool need_declare;
	std::string src_reg;
	std::string src_reg_mask;
	bool need_cast;
};

static const reg_info reg_table[] =
{
	{ "gl_Position", false, "dst_reg0", "", false },
	{ "diff_color", true, "dst_reg1", "", false },
	{ "spec_color", true, "dst_reg2", "", false },
	{ "front_diff_color", true, "dst_reg3", "", false },
	{ "front_spec_color", true, "dst_reg4", "", false },
	{ "fogc", true, "dst_reg5", ".x", true },
	{ "gl_ClipDistance[0]", false, "dst_reg5", ".y", false },
	{ "gl_ClipDistance[1]", false, "dst_reg5", ".z", false },
	{ "gl_ClipDistance[2]", false, "dst_reg5", ".w", false },
	{ "gl_PointSize", false, "dst_reg6", ".x", false },
	{ "gl_ClipDistance[3]", false, "dst_reg6", ".y", false },
	{ "gl_ClipDistance[4]", false, "dst_reg6", ".z", false },
	{ "gl_ClipDistance[5]", false, "dst_reg6", ".w", false },
	{ "tc0", true, "dst_reg7", "", false },
	{ "tc1", true, "dst_reg8", "", false },
	{ "tc2", true, "dst_reg9", "", false },
	{ "tc3", true, "dst_reg10", "", false },
	{ "tc4", true, "dst_reg11", "", false },
	{ "tc5", true, "dst_reg12", "", false },
	{ "tc6", true, "dst_reg13", "", false },
	{ "tc7", true, "dst_reg14", "", false },
	{ "tc8", true, "dst_reg15", "", false },
	{ "tc9", true, "dst_reg6", "", false }  // In this line, dst_reg6 is correct since dst_reg goes from 0 to 15.
};

void D3D12VertexProgramDecompiler::insertMainStart(std::stringstream & OS)
{
	OS << "PixelInput main(VertexInput In)" << std::endl;
	OS << "{" << std::endl;

	// Declare inside main function
	for (const ParamType PT : m_parr.params[PF_PARAM_NONE])
	{
		for (const ParamItem &PI : PT.items)
		{
			OS << "	" << PT.type << " " << PI.name;
			if (!PI.value.empty())
				OS << " = " << PI.value;
			OS << ";" << std::endl;
		}
	}

	for (const ParamType PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem &PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << " = In." << PI.name << ";" << std::endl;
	}
}


void D3D12VertexProgramDecompiler::insertMainEnd(std::stringstream & OS)
{
	OS << "	PixelInput Out;" << std::endl;
	// Declare inside main function
	for (auto &i : reg_table)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "float4", i.src_reg))
			OS << "	Out." << i.src_reg << " = " << i.src_reg << ";" << std::endl;
	}
	OS << "	Out.dst_reg0 = mul(Out.dst_reg0, scaleOffsetMat);" << std::endl;
	OS << "	return Out;" << std::endl;
	OS << "}" << std::endl;
}

D3D12VertexProgramDecompiler::D3D12VertexProgramDecompiler(std::vector<u32>& data) :
	VertexProgramDecompiler(data)
{
}

#endif
