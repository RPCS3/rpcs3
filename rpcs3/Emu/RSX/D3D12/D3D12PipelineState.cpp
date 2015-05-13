#include "stdafx.h"
#if defined (DX12_SUPPORT)

#include "D3D12PipelineState.h"
#include "D3D12ProgramDisassembler.h"
#include "Emu/Memory/vm.h"
#include "Utilities/Log.h"
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <unordered_map>

#pragma comment (lib, "d3dcompiler.lib")


size_t getFPBinarySize(void *ptr)
{
	const qword *instBuffer = (const qword*)ptr;
	size_t instIndex = 0;
	while (true)
	{
		const qword& inst = instBuffer[instIndex];
		bool end = (inst.word[0] >> 8) & 0x1;
		if (end)
			return (instIndex + 1) * 4;
		instIndex++;
	}
}


PipelineStateObjectCache::PipelineStateObjectCache() : m_currentShaderId(0)
{}

PipelineStateObjectCache::~PipelineStateObjectCache()
{
	for (auto pair : m_cachePSO)
		pair.second->Release();
}

bool PipelineStateObjectCache::SearchFp(const RSXFragmentProgram& rsx_fp, Shader& shader)
{
	binary2FS::const_iterator It = m_cacheFS.find(vm::get_ptr<void>(rsx_fp.addr));
	if (It != m_cacheFS.end())
	{
		shader = It->second;
		return true;
	}
	return false;
}

bool PipelineStateObjectCache::SearchVp(const RSXVertexProgram& rsx_vp, Shader& shader)
{
	binary2VS::const_iterator It = m_cacheVS.find((void*)rsx_vp.data.data());
	if (It != m_cacheVS.end())
	{
		shader = It->second;
		return true;
	}
	return false;
}

ID3D12PipelineState *PipelineStateObjectCache::GetProg(const PSOKey &key) const
{
	std::unordered_map<PSOKey, ID3D12PipelineState *, PSOKeyHash, PSOKeyCompare>::const_iterator It = m_cachePSO.find(key);
	if (It == m_cachePSO.end())
		return nullptr;
	return It->second;
}

void PipelineStateObjectCache::AddVertexProgram(Shader& vp, RSXVertexProgram& rsx_vp)
{
	size_t actualVPSize = rsx_vp.data.size() * 4;
	void *fpShadowCopy = malloc(actualVPSize);
	memcpy(fpShadowCopy, rsx_vp.data.data(), actualVPSize);
	vp.Id = (u32)m_currentShaderId++;
	m_cacheVS.insert(std::make_pair(fpShadowCopy, vp));
}

void PipelineStateObjectCache::AddFragmentProgram(Shader& fp, RSXFragmentProgram& rsx_fp)
{
	size_t actualFPSize = getFPBinarySize(vm::get_ptr<u8>(rsx_fp.addr));
	void *fpShadowCopy = malloc(actualFPSize);
	memcpy(fpShadowCopy, vm::get_ptr<u8>(rsx_fp.addr), actualFPSize);
	fp.Id = (u32)m_currentShaderId++;
	m_cacheFS.insert(std::make_pair(fpShadowCopy, fp));
}

void PipelineStateObjectCache::Add(ID3D12PipelineState *prog, const PSOKey& PSOKey)
{
	m_cachePSO.insert(std::make_pair(PSOKey, prog));
}

ID3D12PipelineState *PipelineStateObjectCache::getGraphicPipelineState(
	ID3D12Device *device,
	ID3D12RootSignature *rootSignature,
	RSXVertexProgram *vertexShader,
	RSXFragmentProgram *fragmentShader,
	const PipelineProperties &pipelineProperties,
	const std::vector<D3D12_INPUT_ELEMENT_DESC> &IASet)
{
	ID3D12PipelineState *result = nullptr;
	Shader m_vertex_prog, m_fragment_prog;
	bool m_fp_buf_num = SearchFp(*fragmentShader, m_fragment_prog);
	bool m_vp_buf_num = SearchVp(*vertexShader, m_vertex_prog);

	if (!m_fp_buf_num)
	{
		LOG_WARNING(RSX, "FP not found in buffer!");
		//		Decompile(*fragmentShader);
		m_fragment_prog.Compile(SHADER_TYPE::SHADER_TYPE_FRAGMENT);
		AddFragmentProgram(m_fragment_prog, *fragmentShader);

		// TODO: This shouldn't use current dir
		//fs::file("./FragmentProgram.txt", o_write | o_create | o_trunc).write(m_fragment_prog.shader.c_str(), m_fragment_prog.shader.size());
	}

	if (!m_vp_buf_num)
	{
		LOG_WARNING(RSX, "VP not found in buffer!");
		//		m_vertex_prog.Decompile(*vertexShader);
		m_vertex_prog.Compile(SHADER_TYPE::SHADER_TYPE_VERTEX);
		AddVertexProgram(m_vertex_prog, *vertexShader);

		// TODO: This shouldn't use current dir
//		fs::file("./VertexProgram.txt", o_write | o_create | o_trunc).write(m_vertex_prog.shader.c_str(), m_vertex_prog.shader.size());
	}

	if (m_fp_buf_num && m_vp_buf_num)
	{
		result = GetProg({ m_vertex_prog.Id, m_fragment_prog.Id, pipelineProperties });
	}

	if (result != nullptr)
	{
		return result;
		/*		// RSX Debugger: Check if this program was modified and update it
				if (Ini.GSLogPrograms.GetValue())
				{
					for (auto& program : m_debug_programs)
					{
						if (program.id == m_program.id && program.modified)
						{
							// TODO: This isn't working perfectly. Is there any better/shorter way to update the program
							m_vertex_prog.shader = program.vp_shader;
							m_fragment_prog.shader = program.fp_shader;
							m_vertex_prog.Wait();
							m_vertex_prog.Compile();
							checkForGlError("m_vertex_prog.Compile");
							m_fragment_prog.Wait();
							m_fragment_prog.Compile();
							checkForGlError("m_fragment_prog.Compile");
							glAttachShader(m_program.id, m_vertex_prog.id);
							glAttachShader(m_program.id, m_fragment_prog.id);
							glLinkProgram(m_program.id);
							checkForGlError("glLinkProgram");
							glDetachShader(m_program.id, m_vertex_prog.id);
							glDetachShader(m_program.id, m_fragment_prog.id);
							program.vp_id = m_vertex_prog.id;
							program.fp_id = m_fragment_prog.id;
							program.modified = false;
						}
					}
				}
				m_program.Use();*/
	}
	else
	{
		LOG_WARNING(RSX, "Add program :");
		LOG_WARNING(RSX, "*** vp id = %d", m_vertex_prog.Id);
		LOG_WARNING(RSX, "*** fp id = %d", m_fragment_prog.Id);

		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc = {};

		if (m_vertex_prog.bytecode != nullptr)
		{
			graphicPipelineStateDesc.VS.BytecodeLength = m_vertex_prog.bytecode->GetBufferSize();
			graphicPipelineStateDesc.VS.pShaderBytecode = m_vertex_prog.bytecode->GetBufferPointer();
		}
		if (m_fragment_prog.bytecode != nullptr)
		{
			graphicPipelineStateDesc.PS.BytecodeLength = m_fragment_prog.bytecode->GetBufferSize();
			graphicPipelineStateDesc.PS.pShaderBytecode = m_fragment_prog.bytecode->GetBufferPointer();
		}

		graphicPipelineStateDesc.pRootSignature = rootSignature;

		// Sensible default value
		static D3D12_RASTERIZER_DESC CD3D12_RASTERIZER_DESC =
		{
			D3D12_FILL_MODE_SOLID,
			D3D12_CULL_MODE_BACK,
			FALSE,
			D3D12_DEFAULT_DEPTH_BIAS,
			D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
			D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
			TRUE,
			FALSE,
			FALSE,
			0,
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
		};

		static D3D12_DEPTH_STENCIL_DESC CD3D12_DEPTH_STENCIL_DESC =
		{
			TRUE,
			D3D12_DEPTH_WRITE_MASK_ALL,
			D3D12_COMPARISON_FUNC_LESS_EQUAL,
			FALSE,
			D3D12_DEFAULT_STENCIL_READ_MASK,
			D3D12_DEFAULT_STENCIL_WRITE_MASK,
		};

		static D3D12_BLEND_DESC CD3D12_BLEND_DESC =
		{
			FALSE,
			FALSE,
			{
				FALSE,FALSE,
				D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
				D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
				D3D12_LOGIC_OP_NOOP,
				D3D12_COLOR_WRITE_ENABLE_ALL,
			}
		};

		graphicPipelineStateDesc.BlendState = CD3D12_BLEND_DESC;
		graphicPipelineStateDesc.DepthStencilState = CD3D12_DEPTH_STENCIL_DESC;
		graphicPipelineStateDesc.RasterizerState = CD3D12_RASTERIZER_DESC;
		graphicPipelineStateDesc.PrimitiveTopologyType = pipelineProperties.Topology;

		graphicPipelineStateDesc.NumRenderTargets = 1;
		graphicPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		graphicPipelineStateDesc.DSVFormat = DXGI_FORMAT_D16_UNORM;

		graphicPipelineStateDesc.InputLayout.pInputElementDescs = IASet.data();
		graphicPipelineStateDesc.InputLayout.NumElements = (UINT)IASet.size();
		graphicPipelineStateDesc.SampleDesc.Count = 1;
		graphicPipelineStateDesc.SampleMask = UINT_MAX;
		graphicPipelineStateDesc.NodeMask = 1;

		device->CreateGraphicsPipelineState(&graphicPipelineStateDesc, IID_PPV_ARGS(&result));
		Add(result, {m_vertex_prog.Id, m_fragment_prog.Id, pipelineProperties });

		// RSX Debugger
		/*if (Ini.GSLogPrograms.GetValue())
		{
			RSXDebuggerProgram program;
			program.id = m_program.id;
			program.vp_id = m_vertex_prog.id;
			program.fp_id = m_fragment_prog.id;
			program.vp_shader = m_vertex_prog.shader;
			program.fp_shader = m_fragment_prog.shader;
			m_debug_programs.push_back(program);
		}*/
	}
	return result;
}

#define TO_STRING(x) #x

void Shader::Compile(SHADER_TYPE st)
{
	static const char VSstring[] = TO_STRING(
		cbuffer SCALE_OFFSET : register(b0)
		{
			float4x4 scaleOffsetMat;
		};

		cbuffer CONSTANT : register(b1)
		{
			float4 vc[468];
		};

		struct vertex {
			float4 pos : TEXCOORD0;
			float4 color : TEXCOORD3;
		};

		struct pixel {
			float4 pos : SV_POSITION;
			float4 color : TEXCOORD0;
		};

		pixel main(vertex In)
		{
			pixel Out;
			float4 pos = In.pos;
			pos.w = dot(pos, vc[259]);
			pos.z = dot(pos, vc[258]);
			pos.y = dot(pos, vc[257]);
			pos.x = dot(pos, vc[256]);
			pos.z = -pos.z;
			Out.pos = mul(pos, scaleOffsetMat);
			Out.color = In.color;
			return Out;
		});

	static const char FSstring[] = TO_STRING(
		struct pixel {
			float4 pos : SV_POSITION;
			float4 color : TEXCOORD0;
		};
		float4 main(pixel In) : SV_TARGET
		{
		return In.color;
		});

	HRESULT hr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	switch (st)
	{
	case SHADER_TYPE::SHADER_TYPE_VERTEX:
		hr = D3DCompile(VSstring, sizeof(VSstring), "test", nullptr, nullptr, "main", "vs_5_0", 0, 0, &bytecode, errorBlob.GetAddressOf());
		if (hr != S_OK)
			LOG_ERROR(RSX, "VS build failed:%s", errorBlob->GetBufferPointer());
		break;
	case SHADER_TYPE::SHADER_TYPE_FRAGMENT:
		hr = D3DCompile(FSstring, sizeof(FSstring), "test", nullptr, nullptr, "main", "ps_5_0", 0, 0, &bytecode, errorBlob.GetAddressOf());
		if (hr != S_OK)
			LOG_ERROR(RSX, "FS build failed:%s", errorBlob->GetBufferPointer());
		break;
	}
}


#endif