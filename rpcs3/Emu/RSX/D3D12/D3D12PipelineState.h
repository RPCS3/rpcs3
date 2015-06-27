#pragma once
#if defined (DX12_SUPPORT)

#include <d3d12.h>
#include <wrl/client.h>
#include "../Common/ProgramStateCache.h"
#include "D3D12VertexProgramDecompiler.h"
#include "D3D12FragmentProgramDecompiler.h"
#include "Utilities/File.h"



struct D3D12PipelineProperties
{
	D3D12_PRIMITIVE_TOPOLOGY_TYPE Topology;
	DXGI_FORMAT DepthStencilFormat;
	DXGI_FORMAT RenderTargetsFormat;
	std::vector<D3D12_INPUT_ELEMENT_DESC> IASet;
	D3D12_BLEND_DESC Blend;
	unsigned numMRT : 3;
	D3D12_DEPTH_STENCIL_DESC DepthStencil;
	D3D12_RASTERIZER_DESC Rasterization;

	bool operator==(const D3D12PipelineProperties &in) const
	{
		if (IASet.size() != in.IASet.size())
			return false;
		for (unsigned i = 0; i < IASet.size(); i++)
		{
			const D3D12_INPUT_ELEMENT_DESC &a = IASet[i], &b = in.IASet[i];
			if (a.AlignedByteOffset != b.AlignedByteOffset)
				return false;
			if (a.Format != b.Format)
				return false;
			if (a.InputSlot != b.InputSlot)
				return false;
			if (a.InstanceDataStepRate != b.InstanceDataStepRate)
				return false;
			if (a.SemanticIndex != b.SemanticIndex)
				return false;
		}

		if (memcmp(&DepthStencil, &in.DepthStencil, sizeof(D3D12_DEPTH_STENCIL_DESC)))
			return false;
		if (memcmp(&Blend, &in.Blend, sizeof(D3D12_BLEND_DESC)))
			return false;
		if (memcmp(&Rasterization, &in.Rasterization, sizeof(D3D12_RASTERIZER_DESC)))
			return false;
		return Topology == in.Topology && DepthStencilFormat == in.DepthStencilFormat && numMRT == in.numMRT && RenderTargetsFormat == in.RenderTargetsFormat;
	}
};

/** Storage for a shader
*   Embeds the D3DBlob
*/
struct Shader
{
public:
	enum class SHADER_TYPE
	{
		SHADER_TYPE_VERTEX,
		SHADER_TYPE_FRAGMENT
	};

	Shader() : bytecode(nullptr) {}
	~Shader() {}

	u32 id;
	Microsoft::WRL::ComPtr<ID3DBlob> bytecode;
	std::vector<size_t> FragmentConstantOffsetCache;
	size_t m_textureCount;

	/**
	* Decompile a fragment shader located in the PS3's Memory.  This function operates synchronously.
	* @param prog RSXShaderProgram specifying the location and size of the shader in memory
	*/
	//	void Decompile(RSXFragmentProgram& prog)

	/** Compile the decompiled fragment shader into a format we can use with OpenGL. */
	void Compile(const std::string &code, enum class SHADER_TYPE st);
};

struct D3D12Traits
{
	typedef Shader VertexProgramData;
	typedef Shader FragmentProgramData;
	typedef std::pair<ID3D12PipelineState *, size_t> PipelineData;
	typedef D3D12PipelineProperties PipelineProperties;
	typedef std::pair<ID3D12Device *, ID3D12RootSignature **> ExtraData;

	static
	void RecompileFragmentProgram(RSXFragmentProgram *RSXFP, FragmentProgramData& fragmentProgramData, size_t ID)
	{
		D3D12FragmentDecompiler FS(RSXFP->addr, RSXFP->size, RSXFP->ctrl);
		const std::string &shader = FS.Decompile();
		fragmentProgramData.Compile(shader, Shader::SHADER_TYPE::SHADER_TYPE_FRAGMENT);
		fragmentProgramData.m_textureCount = 0;
		for (const ParamType& PT : FS.m_parr.params[PF_PARAM_UNIFORM])
		{
			for (const ParamItem PI : PT.items)
			{
				if (PT.type == "sampler2D")
				{
					fragmentProgramData.m_textureCount++;
					continue;
				}
				size_t offset = atoi(PI.name.c_str() + 2);
				fragmentProgramData.FragmentConstantOffsetCache.push_back(offset);
			}
		}

		// TODO: This shouldn't use current dir
		std::string filename = "./FragmentProgram" + std::to_string(ID) + ".hlsl";
		fs::file(filename, o_write | o_create | o_trunc).write(shader.c_str(), shader.size());
		fragmentProgramData.id = (u32)ID;
	}

	static
	void RecompileVertexProgram(RSXVertexProgram *RSXVP, VertexProgramData& vertexProgramData, size_t ID)
	{
		D3D12VertexProgramDecompiler VS(RSXVP->data);
		std::string shaderCode = VS.Decompile();
		vertexProgramData.Compile(shaderCode, Shader::SHADER_TYPE::SHADER_TYPE_VERTEX);

		// TODO: This shouldn't use current dir
		std::string filename = "./VertexProgram" + std::to_string(ID) + ".hlsl";
		fs::file(filename, o_write | o_create | o_trunc).write(shaderCode.c_str(), shaderCode.size());
		vertexProgramData.id = (u32)ID;
	}

	static
	PipelineData *BuildProgram(VertexProgramData &vertexProgramData, FragmentProgramData &fragmentProgramData, const PipelineProperties &pipelineProperties, const ExtraData& extraData)
	{

		std::pair<ID3D12PipelineState *, size_t> *result = new std::pair<ID3D12PipelineState *, size_t>();
		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc = {};

		if (vertexProgramData.bytecode == nullptr)
			return nullptr;
		graphicPipelineStateDesc.VS.BytecodeLength = vertexProgramData.bytecode->GetBufferSize();
		graphicPipelineStateDesc.VS.pShaderBytecode = vertexProgramData.bytecode->GetBufferPointer();

		if (fragmentProgramData.bytecode == nullptr)
			return nullptr;
		graphicPipelineStateDesc.PS.BytecodeLength = fragmentProgramData.bytecode->GetBufferSize();
		graphicPipelineStateDesc.PS.pShaderBytecode = fragmentProgramData.bytecode->GetBufferPointer();

		graphicPipelineStateDesc.pRootSignature = extraData.second[fragmentProgramData.m_textureCount];
		result->second = fragmentProgramData.m_textureCount;

		graphicPipelineStateDesc.BlendState = pipelineProperties.Blend;
		graphicPipelineStateDesc.DepthStencilState = pipelineProperties.DepthStencil;
		graphicPipelineStateDesc.RasterizerState = pipelineProperties.Rasterization;
		graphicPipelineStateDesc.PrimitiveTopologyType = pipelineProperties.Topology;

		graphicPipelineStateDesc.NumRenderTargets = pipelineProperties.numMRT;
		for (unsigned i = 0; i < pipelineProperties.numMRT; i++)
			graphicPipelineStateDesc.RTVFormats[i] = pipelineProperties.RenderTargetsFormat;
		graphicPipelineStateDesc.DSVFormat = pipelineProperties.DepthStencilFormat;

		graphicPipelineStateDesc.InputLayout.pInputElementDescs = pipelineProperties.IASet.data();
		graphicPipelineStateDesc.InputLayout.NumElements = (UINT)pipelineProperties.IASet.size();
		graphicPipelineStateDesc.SampleDesc.Count = 1;
		graphicPipelineStateDesc.SampleMask = UINT_MAX;
		graphicPipelineStateDesc.NodeMask = 1;

		extraData.first->CreateGraphicsPipelineState(&graphicPipelineStateDesc, IID_PPV_ARGS(&result->first));
		return result;
	}

	static
	void DeleteProgram(PipelineData *ptr)
	{
		ptr->first->Release();
		delete ptr;
	}
};

class PipelineStateObjectCache : public ProgramStateCache<D3D12Traits>
{
};



#endif