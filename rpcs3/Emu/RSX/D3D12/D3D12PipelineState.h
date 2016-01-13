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

	Shader() = default;
	~Shader() = default;
	Shader(const Shader &) = delete;

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
	using vertex_program_type = Shader;
	using fragment_program_type  = Shader;
	using pipeline_storage_type = std::tuple<ComPtr<ID3D12PipelineState>, std::vector<size_t>, size_t>;
	using pipeline_properties  = D3D12PipelineProperties;

	static
	void recompile_fragment_program(const RSXFragmentProgram &RSXFP, fragment_program_type& fragmentProgramData, size_t ID)
	{
		u32 size;
		D3D12FragmentDecompiler FS(RSXFP, size);
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

		fs::file(fs::get_config_dir() + "FragmentProgram" + std::to_string(ID) + ".hlsl", fom::rewrite).write(shader);
		fragmentProgramData.id = (u32)ID;
	}

	static
	void recompile_vertex_program(const RSXVertexProgram &RSXVP, vertex_program_type& vertexProgramData, size_t ID)
	{
		D3D12VertexProgramDecompiler VS(RSXVP);
		std::string shaderCode = VS.Decompile();
		vertexProgramData.Compile(shaderCode, Shader::SHADER_TYPE::SHADER_TYPE_VERTEX);
		vertexProgramData.vertex_shader_inputs = VS.input_slots;
		fs::file(fs::get_config_dir() + "VertexProgram" + std::to_string(ID) + ".hlsl", fom::rewrite).write(shaderCode);
		vertexProgramData.id = (u32)ID;
	}

	static
		pipeline_storage_type build_pipeline(
			const vertex_program_type &vertexProgramData, const fragment_program_type &fragmentProgramData, const pipeline_properties &pipelineProperties,
			ID3D12Device *device, gsl::span<ComPtr<ID3D12RootSignature>, 17> root_signatures)
	{
		std::tuple<ID3D12PipelineState *, std::vector<size_t>, size_t> result = {};
		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc = {};

		if (vertexProgramData.bytecode == nullptr)
			throw new EXCEPTION("Vertex program compilation failure");
		graphicPipelineStateDesc.VS.BytecodeLength = vertexProgramData.bytecode->GetBufferSize();
		graphicPipelineStateDesc.VS.pShaderBytecode = vertexProgramData.bytecode->GetBufferPointer();

		if (fragmentProgramData.bytecode == nullptr)
			throw new EXCEPTION("fragment program compilation failure");
		graphicPipelineStateDesc.PS.BytecodeLength = fragmentProgramData.bytecode->GetBufferSize();
		graphicPipelineStateDesc.PS.pShaderBytecode = fragmentProgramData.bytecode->GetBufferPointer();

		graphicPipelineStateDesc.pRootSignature = root_signatures[fragmentProgramData.m_textureCount].Get();

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

		ComPtr<ID3D12PipelineState> pso;
		CHECK_HRESULT(device->CreateGraphicsPipelineState(&graphicPipelineStateDesc, IID_PPV_ARGS(pso.GetAddressOf())));

		std::wstring name = L"PSO_" + std::to_wstring(vertexProgramData.id) + L"_" + std::to_wstring(fragmentProgramData.id);
		pso->SetName(name.c_str());
		return std::make_tuple(pso, vertexProgramData.vertex_shader_inputs, fragmentProgramData.m_textureCount);
	}
};

class PipelineStateObjectCache : public program_state_cache<D3D12Traits>
{
};
