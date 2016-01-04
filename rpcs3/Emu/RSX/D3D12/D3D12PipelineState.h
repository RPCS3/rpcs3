#pragma once

#include "D3D12Utils.h"
#include "../Common/ProgramStateCache.h"
#include "D3D12VertexProgramDecompiler.h"
#include "D3D12FragmentProgramDecompiler.h"

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
	D3D12_INDEX_BUFFER_STRIP_CUT_VALUE CutValue;

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

template<typename T>
size_t hashStructContent(const T& structure)
{
	char *data = (char*)(&structure);
	size_t result = 0;
	for (unsigned i = 0; i < sizeof(T); i++)
		result ^= std::hash<char>()(data[i]);
	return result;
}

namespace std
{
	template <>
	struct hash<D3D12PipelineProperties> {
		size_t operator()(const D3D12PipelineProperties &pipelineProperties) const {
			size_t seed = hash<unsigned>()(pipelineProperties.DepthStencilFormat) ^
				(hash<unsigned>()(pipelineProperties.RenderTargetsFormat) << 2) ^
				(hash<unsigned>()(pipelineProperties.Topology) << 2) ^
				(hash<unsigned>()(pipelineProperties.numMRT) << 4);
			seed ^= hashStructContent(pipelineProperties.Blend);
			seed ^= hashStructContent(pipelineProperties.DepthStencil);
			seed ^= hashStructContent(pipelineProperties.Rasterization);
			return hash<size_t>()(seed);
		}
	};
}

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
	ComPtr<ID3DBlob> bytecode;
	// For debugging
	std::string content;
	std::vector<size_t> vertex_shader_inputs;
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

static
bool has_attribute(size_t attribute, const std::vector<D3D12_INPUT_ELEMENT_DESC> &desc)
{
	for (const auto &attribute_desc : desc)
	{
		if (attribute_desc.SemanticIndex == attribute)
			return true;
	}
	return false;
}

static
std::vector<D3D12_INPUT_ELEMENT_DESC> completes_IA_desc(const std::vector<D3D12_INPUT_ELEMENT_DESC> &desc, const std::vector<size_t> &inputs)
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> result(desc);
	for (size_t attribute : inputs)
	{
		if (has_attribute(attribute, desc))
			continue;
		D3D12_INPUT_ELEMENT_DESC extra_ia_desc = {};
		extra_ia_desc.SemanticIndex = (UINT)attribute;
		extra_ia_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		extra_ia_desc.SemanticName = "TEXCOORD";
		extra_ia_desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		result.push_back(extra_ia_desc);
	}
	return result;
}

struct D3D12Traits
{
	typedef Shader VertexProgramData;
	typedef Shader FragmentProgramData;
	typedef std::tuple<ID3D12PipelineState *, std::vector<size_t>, size_t> PipelineData;
	typedef D3D12PipelineProperties PipelineProperties;
	typedef std::pair<ID3D12Device *, ComPtr<ID3D12RootSignature> *> ExtraData;

	static
	void RecompileFragmentProgram(RSXFragmentProgram *RSXFP, FragmentProgramData& fragmentProgramData, size_t ID)
	{
		D3D12FragmentDecompiler FS(RSXFP->addr, RSXFP->size, RSXFP->ctrl, RSXFP->texture_dimensions);
		const std::string &shader = FS.Decompile();
		fragmentProgramData.Compile(shader, Shader::SHADER_TYPE::SHADER_TYPE_FRAGMENT);
		fragmentProgramData.m_textureCount = 0;
		for (const ParamType& PT : FS.m_parr.params[PF_PARAM_UNIFORM])
		{
			for (const ParamItem PI : PT.items)
			{
				if (PT.type == "sampler2D" || PT.type == "samplerCube")
				{
					size_t texture_unit = atoi(PI.name.c_str() + 3);
					fragmentProgramData.m_textureCount = std::max(texture_unit + 1, fragmentProgramData.m_textureCount);
					continue;
				}
				size_t offset = atoi(PI.name.c_str() + 2);
				fragmentProgramData.FragmentConstantOffsetCache.push_back(offset);
			}
		}

		fs::file(fs::get_config_dir() + "FragmentProgram" + std::to_string(ID) + ".hlsl", fom::rewrite) << shader;
		fragmentProgramData.id = (u32)ID;
	}

	static
	void RecompileVertexProgram(RSXVertexProgram *RSXVP, VertexProgramData& vertexProgramData, size_t ID)
	{
		D3D12VertexProgramDecompiler VS(RSXVP->data);
		std::string shaderCode = VS.Decompile();
		vertexProgramData.Compile(shaderCode, Shader::SHADER_TYPE::SHADER_TYPE_VERTEX);
		vertexProgramData.vertex_shader_inputs = VS.input_slots;
		fs::file(fs::get_config_dir() + "VertexProgram" + std::to_string(ID) + ".hlsl", fom::rewrite) << shaderCode;
		vertexProgramData.id = (u32)ID;
	}

	static
	PipelineData *BuildProgram(VertexProgramData &vertexProgramData, FragmentProgramData &fragmentProgramData, const PipelineProperties &pipelineProperties, const ExtraData& extraData)
	{

		std::tuple<ID3D12PipelineState *, std::vector<size_t>, size_t> *result = new std::tuple<ID3D12PipelineState *, std::vector<size_t>, size_t>();
		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc = {};

		if (vertexProgramData.bytecode == nullptr)
			return nullptr;
		graphicPipelineStateDesc.VS.BytecodeLength = vertexProgramData.bytecode->GetBufferSize();
		graphicPipelineStateDesc.VS.pShaderBytecode = vertexProgramData.bytecode->GetBufferPointer();

		if (fragmentProgramData.bytecode == nullptr)
			return nullptr;
		graphicPipelineStateDesc.PS.BytecodeLength = fragmentProgramData.bytecode->GetBufferSize();
		graphicPipelineStateDesc.PS.pShaderBytecode = fragmentProgramData.bytecode->GetBufferPointer();

		graphicPipelineStateDesc.pRootSignature = extraData.second[fragmentProgramData.m_textureCount].Get();
		std::get<2>(*result) = fragmentProgramData.m_textureCount;

		graphicPipelineStateDesc.BlendState = pipelineProperties.Blend;
		graphicPipelineStateDesc.DepthStencilState = pipelineProperties.DepthStencil;
		graphicPipelineStateDesc.RasterizerState = pipelineProperties.Rasterization;
		graphicPipelineStateDesc.PrimitiveTopologyType = pipelineProperties.Topology;

		graphicPipelineStateDesc.NumRenderTargets = pipelineProperties.numMRT;
		for (unsigned i = 0; i < pipelineProperties.numMRT; i++)
			graphicPipelineStateDesc.RTVFormats[i] = pipelineProperties.RenderTargetsFormat;
		graphicPipelineStateDesc.DSVFormat = pipelineProperties.DepthStencilFormat;

		const std::vector<D3D12_INPUT_ELEMENT_DESC> &completed_IA_desc = completes_IA_desc(pipelineProperties.IASet, vertexProgramData.vertex_shader_inputs);

		graphicPipelineStateDesc.InputLayout.pInputElementDescs = completed_IA_desc.data();
		graphicPipelineStateDesc.InputLayout.NumElements = (UINT)completed_IA_desc.size();
		graphicPipelineStateDesc.SampleDesc.Count = 1;
		graphicPipelineStateDesc.SampleMask = UINT_MAX;
		graphicPipelineStateDesc.NodeMask = 1;

		graphicPipelineStateDesc.IBStripCutValue = pipelineProperties.CutValue;

		CHECK_HRESULT(extraData.first->CreateGraphicsPipelineState(&graphicPipelineStateDesc, IID_PPV_ARGS(&std::get<0>(*result))));
		std::get<1>(*result) = vertexProgramData.vertex_shader_inputs;

		std::wstring name = L"PSO_" + std::to_wstring(vertexProgramData.id) + L"_" + std::to_wstring(fragmentProgramData.id);
		std::get<0>(*result)->SetName(name.c_str());
		return result;
	}

	static
	void DeleteProgram(PipelineData *ptr)
	{
		std::get<0>(*ptr)->Release();
		delete ptr;
	}
};

class PipelineStateObjectCache : public ProgramStateCache<D3D12Traits>
{
};
