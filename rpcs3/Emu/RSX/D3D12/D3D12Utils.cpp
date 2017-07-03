#ifdef _MSC_VER
#include "stdafx.h"
#include "stdafx_d3d12.h"
#include "D3D12GSRender.h"
#include "d3dx12.h"
#define STRINGIFY(x) #x

extern PFN_D3D12_SERIALIZE_ROOT_SIGNATURE wrapD3D12SerializeRootSignature;
extern pD3DCompile wrapD3DCompile;

void D3D12GSRender::shader::init(ID3D12Device *device, ID3D12CommandQueue *gfx_command_queue)
{
	const char *fsCode = STRINGIFY(
		Texture2D InputTexture : register(t0); \n
		sampler bilinearSampler : register(s0); \n

	struct PixelInput \n
	{ \n
		float4 Pos : SV_POSITION; \n
		float2 TexCoords : TEXCOORDS0; \n
	}; \n

		float4 main(PixelInput In) : SV_TARGET \n
	{ \n
		return InputTexture.Sample(bilinearSampler, In.TexCoords); \n
	}
	);

	Microsoft::WRL::ComPtr<ID3DBlob> fsBytecode;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	CHECK_HRESULT(wrapD3DCompile(fsCode, strlen(fsCode), "test", nullptr, nullptr, "main", "ps_5_0", 0, 0, &fsBytecode, errorBlob.GetAddressOf()));

	const char *vsCode = STRINGIFY(
	struct VertexInput \n
	{ \n
		float2 Pos : POSITION; \n
		float2 TexCoords : TEXCOORDS0; \n
	}; \n

	struct PixelInput \n
	{ \n
		float4 Pos : SV_POSITION; \n
		float2 TexCoords : TEXCOORDS0; \n
	}; \n

		PixelInput main(VertexInput In) \n
	{ \n
		PixelInput Out; \n
		Out.Pos = float4(In.Pos, 0., 1.); \n
		Out.TexCoords = In.TexCoords; \n
		return Out; \n
	}
	);

	Microsoft::WRL::ComPtr<ID3DBlob> vsBytecode;
	CHECK_HRESULT(wrapD3DCompile(vsCode, strlen(vsCode), "test", nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBytecode, errorBlob.GetAddressOf()));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.PS.BytecodeLength = fsBytecode->GetBufferSize();
	psoDesc.PS.pShaderBytecode = fsBytecode->GetBufferPointer();
	psoDesc.VS.BytecodeLength = vsBytecode->GetBufferSize();
	psoDesc.VS.pShaderBytecode = vsBytecode->GetBufferPointer();
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = UINT_MAX;

	D3D12_INPUT_ELEMENT_DESC IADesc[2] = {};
	IADesc[0].SemanticName = "POSITION";
	IADesc[0].Format = DXGI_FORMAT_R32G32_FLOAT;
	IADesc[1].SemanticName = "TEXCOORDS";
	IADesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	IADesc[1].AlignedByteOffset = 2 * sizeof(float);

	psoDesc.InputLayout.NumElements = 2;
	psoDesc.InputLayout.pInputElementDescs = IADesc;

	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

	D3D12_DESCRIPTOR_RANGE descriptorRange[2] = {};
	// Textures
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[1].BaseShaderRegister = 0;
	descriptorRange[1].NumDescriptors = 1;
	descriptorRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	D3D12_ROOT_PARAMETER RP[2] = {};
	RP[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RP[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	RP[0].DescriptorTable.pDescriptorRanges = &descriptorRange[0];
	RP[0].DescriptorTable.NumDescriptorRanges = 1;
	RP[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RP[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	RP[1].DescriptorTable.pDescriptorRanges = &descriptorRange[1];
	RP[1].DescriptorTable.NumDescriptorRanges = 1;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.NumParameters = 2;
	rootSignatureDesc.pParameters = RP;

	Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;

	CHECK_HRESULT(wrapD3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob));
	CHECK_HRESULT(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&root_signature)));

	psoDesc.pRootSignature = root_signature;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	CHECK_HRESULT(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));

	D3D12_DESCRIPTOR_HEAP_DESC textureHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV , 2, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };
	CHECK_HRESULT(
		device->CreateDescriptorHeap(&textureHeapDesc, IID_PPV_ARGS(&texture_descriptor_heap))
		);
	D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER , 2, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };
	CHECK_HRESULT(
		device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&sampler_descriptor_heap))
		);

	ComPtr<ID3D12Fence> fence;
	CHECK_HRESULT(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));
	HANDLE handle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	fence->SetEventOnCompletion(1, handle);

	float quadVertex[16] = {
		-1., -1., 0., 1.,
		-1., 1., 0., 0.,
		1., -1., 1., 1.,
		1., 1., 1., 0.,
	};

	ComPtr<ID3D12CommandAllocator> cmdlistAllocator;
	CHECK_HRESULT(
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(cmdlistAllocator.GetAddressOf()))
			);
	ComPtr<ID3D12GraphicsCommandList> cmdList;
	CHECK_HRESULT(
		device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdlistAllocator.Get(),nullptr, IID_PPV_ARGS(cmdList.GetAddressOf()))
			);
	ComPtr<ID3D12Resource> intermediateBuffer;
	CHECK_HRESULT(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(16 * sizeof(float)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(intermediateBuffer.GetAddressOf())
		));

	CHECK_HRESULT(
		device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(16 * sizeof(float)),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&vertex_buffer)
			));

	D3D12_SUBRESOURCE_DATA vertexData = { reinterpret_cast<BYTE*>(quadVertex), 16 * sizeof(float), 1 };

	UpdateSubresources(cmdList.Get(), vertex_buffer, intermediateBuffer.Get(), 0, 0, 1, &vertexData);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertex_buffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	CHECK_HRESULT(cmdList->Close());

	gfx_command_queue->ExecuteCommandLists(1, CommandListCast(cmdList.GetAddressOf()));

	// Now wait until upload has completed
	gfx_command_queue->Signal(fence.Get(), 1);
	WaitForSingleObjectEx(handle, INFINITE, FALSE);
	CloseHandle(handle);
}
#endif
