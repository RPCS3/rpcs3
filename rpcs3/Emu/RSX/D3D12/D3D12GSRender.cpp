#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "D3D12GSRender.h"
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <thread>
#include <chrono>

GetGSFrameCb2 GetGSFrame = nullptr;

void SetGetD3DGSFrameCallback(GetGSFrameCb2 value)
{
	GetGSFrame = value;
}

void DataHeap::Init(ID3D12Device *device, size_t heapSize, D3D12_HEAP_TYPE type, D3D12_HEAP_FLAGS flags)
{
	m_size = heapSize;
	D3D12_HEAP_DESC heapDesc = {};
	heapDesc.SizeInBytes = m_size;
	heapDesc.Properties.Type = type;
	heapDesc.Flags = flags;
	check(device->CreateHeap(&heapDesc, IID_PPV_ARGS(&m_heap)));
	m_putPos = 0;
	m_getPos = m_size - 1;
}


bool DataHeap::canAlloc(size_t size)
{
	size_t putPos = m_putPos, getPos = m_getPos;
	size_t allocSize = powerOf2Align(size, 65536);
	if (putPos + allocSize < m_size)
	{
		// range before get
		if (putPos + allocSize < getPos)
			return true;
		// range after get
		if (putPos > getPos)
			return true;
		return false;
	}
	else
	{
		// ..]....[..get..
		if (putPos < getPos)
			return false;
		// ..get..]...[...
		// Actually all resources extending beyond heap space starts at 0
		if (allocSize > getPos)
			return false;
		return true;
	}
}

size_t DataHeap::alloc(size_t size)
{
	assert(canAlloc(size));
	size_t putPos = m_putPos;
	if (putPos + size < m_size)
	{
		m_putPos += powerOf2Align(size, 65536);
		return putPos;
	}
	else
	{
		m_putPos = powerOf2Align(size, 65536);
		return 0;
	}
}

void DataHeap::Release()
{
	m_heap->Release();
	for (auto tmp : m_resourceStoredSinceLastSync)
	{
		std::get<2>(tmp)->Release();
	}
}

void D3D12GSRender::ResourceStorage::Reset()
{
	m_constantsBufferIndex = 0;
	m_currentScaleOffsetBufferIndex = 0;
	m_currentTextureIndex = 0;
	m_frameFinishedFence = nullptr;
	m_frameFinishedHandle = 0;

	for (auto tmp : m_inUseConstantsBuffers)
		std::get<2>(tmp)->Release();
	for (auto tmp : m_inUseVertexIndexBuffers)
		std::get<2>(tmp)->Release();
	for (auto tmp : m_inUseTextureUploadBuffers)
		std::get<2>(tmp)->Release();
	for (auto tmp : m_inUseTexture2D)
		std::get<2>(tmp)->Release();

	m_commandAllocator->Reset();
	m_textureUploadCommandAllocator->Reset();
	m_downloadCommandAllocator->Reset();
	for (ID3D12GraphicsCommandList *gfxCommandList : m_inflightCommandList)
		gfxCommandList->Release();
	m_inflightCommandList.clear();
	for (ID3D12Resource *vertexBuffer : m_inflightResources)
		vertexBuffer->Release();
	m_inflightResources.clear();
}

void D3D12GSRender::ResourceStorage::Init(ID3D12Device *device)
{
	m_frameFinishedHandle = 0;
	// Create a global command allocator
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_textureUploadCommandAllocator));
	check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&m_downloadCommandAllocator)));

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NumDescriptors = 10000; // For safety
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	check(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_constantsBufferDescriptorsHeap)));


	descriptorHeapDesc = {};
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NumDescriptors = 10000; // For safety
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	check(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_scaleOffsetDescriptorHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC textureDescriptorDesc = {};
	textureDescriptorDesc.NumDescriptors = 10000; // For safety
	textureDescriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	textureDescriptorDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	check(device->CreateDescriptorHeap(&textureDescriptorDesc, IID_PPV_ARGS(&m_textureDescriptorsHeap)));

	textureDescriptorDesc.NumDescriptors = 2048; // For safety
	textureDescriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	check(device->CreateDescriptorHeap(&textureDescriptorDesc, IID_PPV_ARGS(&m_samplerDescriptorHeap)));
}

void D3D12GSRender::ResourceStorage::Release()
{
	// NOTE: Should be released only if no command are in flight !
	for (auto tmp : m_inUseConstantsBuffers)
		std::get<2>(tmp)->Release();
	for (auto tmp : m_inUseVertexIndexBuffers)
		std::get<2>(tmp)->Release();
	for (auto tmp : m_inUseTextureUploadBuffers)
		std::get<2>(tmp)->Release();
	for (auto tmp : m_inUseTexture2D)
		std::get<2>(tmp)->Release();

	m_constantsBufferDescriptorsHeap->Release();
	m_scaleOffsetDescriptorHeap->Release();
	for (auto tmp : m_inflightResources)
		tmp->Release();
	m_textureDescriptorsHeap->Release();
	m_samplerDescriptorHeap->Release();
	for (auto tmp : m_inflightCommandList)
		tmp->Release();
	m_commandAllocator->Release();
	m_textureUploadCommandAllocator->Release();
	m_downloadCommandAllocator->Release();
	if (m_frameFinishedHandle)
		CloseHandle(m_frameFinishedHandle);
	if (m_frameFinishedFence)
		m_frameFinishedFence->Release();
}

// 32 bits float to U8 unorm CS
#define STRINGIFY(x) #x
const char *shaderCode = STRINGIFY(
	Texture2D<float> InputTexture : register(t0); \n
	RWTexture2D<float> OutputTexture : register(u0);\n

	[numthreads(8, 8, 1)]\n
	void main(uint3 Id : SV_DispatchThreadID)\n
{ \n
	OutputTexture[Id.xy] = InputTexture.Load(uint3(Id.xy, 0));\n
}
);

/**
 * returns bytecode and root signature of a Compute Shader converting texture from 
 * one format to another
 */
static
std::pair<ID3DBlob *, ID3DBlob *> compileF32toU8CS()
{
	ID3DBlob *bytecode;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3DCompile(shaderCode, strlen(shaderCode), "test", nullptr, nullptr, "main", "cs_5_0", 0, 0, &bytecode, errorBlob.GetAddressOf());
	if (hr != S_OK)
	{
		const char *tmp = (const char*)errorBlob->GetBufferPointer();
		LOG_ERROR(RSX, tmp);
	}
	D3D12_DESCRIPTOR_RANGE descriptorRange[2] = {};
	// Textures
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[1].BaseShaderRegister = 0;
	descriptorRange[1].NumDescriptors = 1;
	descriptorRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	descriptorRange[1].OffsetInDescriptorsFromTableStart = 1;
	D3D12_ROOT_PARAMETER RP[2] = {};
	RP[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RP[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	RP[0].DescriptorTable.pDescriptorRanges = &descriptorRange[0];
	RP[0].DescriptorTable.NumDescriptorRanges = 2;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = 1;
	rootSignatureDesc.pParameters = RP;

	ID3DBlob *rootSignatureBlob;

	hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob);
	if (hr != S_OK)
	{
		const char *tmp = (const char*)errorBlob->GetBufferPointer();
		LOG_ERROR(RSX, tmp);
	}

	return std::make_pair(bytecode, rootSignatureBlob);
}

D3D12GSRender::D3D12GSRender()
	: GSRender(), m_PSO(nullptr)
{
	if (Ini.GSDebugOutputEnable.GetValue())
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
		D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
		debugInterface->EnableDebugLayer();
	}

	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	check(CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory)));
	// Create adapter
	IDXGIAdapter* adaptater = nullptr;
	switch (Ini.GSD3DAdaptater.GetValue())
	{
	case 0: // WARP
		check(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adaptater)));
		break;
	case 1: // Default
		dxgiFactory->EnumAdapters(0, &adaptater);
		break;
	default: // Adaptater 0, 1, ...
		dxgiFactory->EnumAdapters(Ini.GSD3DAdaptater.GetValue() - 2,&adaptater);
		break;
	}
	check(D3D12CreateDevice(adaptater, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));

	// Queues
	D3D12_COMMAND_QUEUE_DESC copyQueueDesc = {}, graphicQueueDesc = {};
	copyQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	graphicQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	check(m_device->CreateCommandQueue(&copyQueueDesc, IID_PPV_ARGS(&m_commandQueueCopy)));
	check(m_device->CreateCommandQueue(&graphicQueueDesc, IID_PPV_ARGS(&m_commandQueueGraphic)));

	m_frame = GetGSFrame();
	DXGI_ADAPTER_DESC adaptaterDesc;
	adaptater->GetDesc(&adaptaterDesc);
	m_frame->SetAdaptaterName(adaptaterDesc.Description);

	// Create swap chain and put them in a descriptor heap as rendertarget
	DXGI_SWAP_CHAIN_DESC swapChain = {};
	swapChain.BufferCount = 2;
	swapChain.Windowed = true;
	swapChain.OutputWindow = m_frame->getHandle();
	swapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChain.SampleDesc.Count = 1;
	swapChain.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	check(dxgiFactory->CreateSwapChain(m_commandQueueGraphic, &swapChain, (IDXGISwapChain**)&m_swapChain));
	m_swapChain->GetBuffer(0, IID_PPV_ARGS(&m_backBuffer[0]));
	m_swapChain->GetBuffer(1, IID_PPV_ARGS(&m_backBuffer[1]));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	D3D12_RENDER_TARGET_VIEW_DESC rttDesc = {};
	rttDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rttDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_backbufferAsRendertarget[0]));
	m_device->CreateRenderTargetView(m_backBuffer[0], &rttDesc, m_backbufferAsRendertarget[0]->GetCPUDescriptorHandleForHeapStart());
	m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_backbufferAsRendertarget[1]));
	m_device->CreateRenderTargetView(m_backBuffer[1], &rttDesc, m_backbufferAsRendertarget[1]->GetCPUDescriptorHandleForHeapStart());

	// Common root signatures
	for (unsigned textureCount = 0; textureCount < 17; textureCount++)
	{
		D3D12_DESCRIPTOR_RANGE descriptorRange[4] = {};
		// Scale Offset data
		descriptorRange[0].BaseShaderRegister = 0;
		descriptorRange[0].NumDescriptors = 1;
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		// Constants
		descriptorRange[1].BaseShaderRegister = 1;
		descriptorRange[1].NumDescriptors = 2;
		descriptorRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		// Textures
		descriptorRange[2].BaseShaderRegister = 0;
		descriptorRange[2].NumDescriptors = textureCount;
		descriptorRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		// Samplers
		descriptorRange[3].BaseShaderRegister = 0;
		descriptorRange[3].NumDescriptors = textureCount;
		descriptorRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		D3D12_ROOT_PARAMETER RP[4] = {};
		RP[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		RP[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		RP[0].DescriptorTable.pDescriptorRanges = &descriptorRange[0];
		RP[0].DescriptorTable.NumDescriptorRanges = 1;
		RP[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		RP[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		RP[1].DescriptorTable.pDescriptorRanges = &descriptorRange[1];
		RP[1].DescriptorTable.NumDescriptorRanges = 1;
		RP[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		RP[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		RP[2].DescriptorTable.pDescriptorRanges = &descriptorRange[2];
		RP[2].DescriptorTable.NumDescriptorRanges = 1;
		RP[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		RP[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		RP[3].DescriptorTable.pDescriptorRanges = &descriptorRange[3];
		RP[3].DescriptorTable.NumDescriptorRanges = 1;

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSignatureDesc.NumParameters = (textureCount > 0) ? 4 : 2;
		rootSignatureDesc.pParameters = RP;

		Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
		check(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob));

		m_device->CreateRootSignature(0,
			rootSignatureBlob->GetBufferPointer(),
			rootSignatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&m_rootSignatures[textureCount]));
	}

	m_perFrameStorage[0].Init(m_device);
	m_perFrameStorage[0].Reset();
	m_perFrameStorage[1].Init(m_device);
	m_perFrameStorage[1].Reset();

	vertexConstantShadowCopy = new float[512 * 4];

	// Convert shader
	auto p = compileF32toU8CS();
	check(
		m_device->CreateRootSignature(0, p.second->GetBufferPointer(), p.second->GetBufferSize(), IID_PPV_ARGS(&m_convertRootSignature))
		);

	D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc = {};
	computePipelineStateDesc.CS.BytecodeLength = p.first->GetBufferSize();
	computePipelineStateDesc.CS.pShaderBytecode = p.first->GetBufferPointer();
	computePipelineStateDesc.pRootSignature = m_convertRootSignature;

	check(
		m_device->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&m_convertPSO))
		);

	p.first->Release();
	p.second->Release();

	D3D12_HEAP_PROPERTIES hp = {};
	hp.Type = D3D12_HEAP_TYPE_DEFAULT;
	check(
		m_device->CreateCommittedResource(
			&hp,
			D3D12_HEAP_FLAG_NONE,
			&getTexture2DResourceDesc(2, 2, DXGI_FORMAT_R8G8B8A8_UNORM),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_dummyTexture))
			);

	D3D12_HEAP_DESC hd = {};
	hd.SizeInBytes = 1024 * 1024 * 128;
	hd.Properties.Type = D3D12_HEAP_TYPE_READBACK;
	hd.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	check(m_device->CreateHeap(&hd, IID_PPV_ARGS(&m_readbackResources.m_heap)));
	m_readbackResources.m_putPos = 0;
	m_readbackResources.m_getPos = 1024 * 1024 * 128 - 1;

	hd.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
	hd.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
	check(m_device->CreateHeap(&hd, IID_PPV_ARGS(&m_UAVHeap.m_heap)));
	m_UAVHeap.m_putPos = 0;
	m_UAVHeap.m_getPos = 1024 * 1024 * 128 - 1;

	m_rtts.Init(m_device);

	m_constantsData.Init(m_device, 1024 * 1024 * 128, D3D12_HEAP_TYPE_UPLOAD, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
	m_vertexIndexData.Init(m_device, 1024 * 1024 * 128, D3D12_HEAP_TYPE_UPLOAD, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
	m_textureUploadData.Init(m_device, 1024 * 1024 * 256, D3D12_HEAP_TYPE_UPLOAD, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
	m_textureData.Init(m_device, 1024 * 1024 * 512, D3D12_HEAP_TYPE_DEFAULT, D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES);
}

D3D12GSRender::~D3D12GSRender()
{
	m_constantsData.Release();
	m_vertexIndexData.Release();
	m_textureUploadData.Release();
	m_textureData.Release();
	m_UAVHeap.m_heap->Release();
	m_readbackResources.m_heap->Release();
	m_texturesRTTs.clear();
	m_dummyTexture->Release();
	m_convertPSO->Release();
	m_convertRootSignature->Release();
	m_perFrameStorage[0].Release();
	m_perFrameStorage[1].Release();
	m_commandQueueGraphic->Release();
	m_commandQueueCopy->Release();
	m_backbufferAsRendertarget[0]->Release();
	m_backBuffer[0]->Release();
	m_backbufferAsRendertarget[1]->Release();
	m_backBuffer[1]->Release();
	m_rtts.Release();
	for (unsigned i = 0; i < 17; i++)
		m_rootSignatures[i]->Release();
	m_swapChain->Release();
	m_device->Release();
	delete[] vertexConstantShadowCopy;
}

void D3D12GSRender::Close()
{
	Stop();
	m_frame->Hide();
}

void D3D12GSRender::OnInit()
{
	m_frame->Show();
}

void D3D12GSRender::OnInitThread()
{
}

void D3D12GSRender::OnExitThread()
{
}

void D3D12GSRender::OnReset()
{
}

void D3D12GSRender::ExecCMD(u32 cmd)
{
	assert(cmd == NV4097_CLEAR_SURFACE);

	InitDrawBuffers();

	ID3D12GraphicsCommandList *commandList;
	check(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, getCurrentResourceStorage().m_commandAllocator, nullptr, IID_PPV_ARGS(&commandList)));
	getCurrentResourceStorage().m_inflightCommandList.push_back(commandList);

/*	if (m_set_color_mask)
	{
		glColorMask(m_color_mask_r, m_color_mask_g, m_color_mask_b, m_color_mask_a);
		checkForGlError("glColorMask");
	}

	if (m_set_scissor_horizontal && m_set_scissor_vertical)
	{
		glScissor(m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h);
		checkForGlError("glScissor");
	}*/

	// TODO: Merge depth and stencil clear when possible
	if (m_clear_surface_mask & 0x1)
		commandList->ClearDepthStencilView(m_rtts.m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, m_clear_surface_z / (float)0xffffff, 0, 0, nullptr);

	if (m_clear_surface_mask & 0x2)
		commandList->ClearDepthStencilView(m_rtts.m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_STENCIL, 0.f, m_clear_surface_s, 0, nullptr);

	if (m_clear_surface_mask & 0xF0)
	{
		float clearColor[] =
		{
			m_clear_surface_color_r / 255.0f,
			m_clear_surface_color_g / 255.0f,
			m_clear_surface_color_b / 255.0f,
			m_clear_surface_color_a / 255.0f
		};

		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtts.m_renderTargetsDescriptorsHeap->GetCPUDescriptorHandleForHeapStart();
		size_t g_RTTIncrement = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		switch (m_surface_color_target)
		{
			case CELL_GCM_SURFACE_TARGET_NONE: break;

			case CELL_GCM_SURFACE_TARGET_0:
			case CELL_GCM_SURFACE_TARGET_1:
				commandList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
				break;
			case CELL_GCM_SURFACE_TARGET_MRT1:
				commandList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
				handle.ptr += g_RTTIncrement;
				commandList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
				break;
			case CELL_GCM_SURFACE_TARGET_MRT2:
				commandList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
				handle.ptr += g_RTTIncrement;
				commandList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
				handle.ptr += g_RTTIncrement;
				commandList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
				break;
			case CELL_GCM_SURFACE_TARGET_MRT3:
				commandList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
				handle.ptr += g_RTTIncrement;
				commandList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
				handle.ptr += g_RTTIncrement;
				commandList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
				handle.ptr += g_RTTIncrement;
				commandList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
				break;
			default:
				LOG_ERROR(RSX, "Bad surface color target: %d", m_surface_color_target);
		}
	}

	check(commandList->Close());
	m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**) &commandList);
}

static D3D12_BLEND_OP getBlendOp(u16 op)
{
	switch (op)
	{
	case CELL_GCM_FUNC_ADD: return D3D12_BLEND_OP_ADD;
	case CELL_GCM_FUNC_SUBTRACT: return D3D12_BLEND_OP_SUBTRACT;
	case CELL_GCM_FUNC_REVERSE_SUBTRACT: return D3D12_BLEND_OP_REV_SUBTRACT;
	case CELL_GCM_MIN: return D3D12_BLEND_OP_MIN;
	case CELL_GCM_MAX: return D3D12_BLEND_OP_MAX;
	case CELL_GCM_FUNC_ADD_SIGNED:
	case CELL_GCM_FUNC_REVERSE_ADD_SIGNED:
	case CELL_GCM_FUNC_REVERSE_SUBTRACT_SIGNED:
		LOG_WARNING(RSX, "Unsupported Blend Op %d", op);
	}
}

static D3D12_BLEND getBlendFactor(u16 factor)
{
	switch (factor)
	{
	case CELL_GCM_ZERO: return D3D12_BLEND_ZERO;
	case CELL_GCM_ONE: return D3D12_BLEND_ONE;
	case CELL_GCM_SRC_COLOR: return D3D12_BLEND_SRC_COLOR;
	case CELL_GCM_ONE_MINUS_SRC_COLOR: return D3D12_BLEND_INV_SRC_COLOR;
	case CELL_GCM_SRC_ALPHA: return D3D12_BLEND_SRC_ALPHA;
	case CELL_GCM_ONE_MINUS_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
	case CELL_GCM_DST_ALPHA: return D3D12_BLEND_DEST_ALPHA;
	case CELL_GCM_ONE_MINUS_DST_ALPHA: return D3D12_BLEND_INV_DEST_ALPHA;
	case CELL_GCM_DST_COLOR: return D3D12_BLEND_DEST_COLOR;
	case CELL_GCM_ONE_MINUS_DST_COLOR: return D3D12_BLEND_INV_DEST_COLOR;
	case CELL_GCM_SRC_ALPHA_SATURATE: return D3D12_BLEND_SRC_ALPHA_SAT;
	case CELL_GCM_CONSTANT_COLOR:
	case CELL_GCM_ONE_MINUS_CONSTANT_COLOR:
	case CELL_GCM_CONSTANT_ALPHA:
	case CELL_GCM_ONE_MINUS_CONSTANT_ALPHA:
		LOG_WARNING(RSX, "Unsupported Blend Factor %d", factor);
	}
}

static D3D12_LOGIC_OP getLogicOp(u32 op)
{
	switch (op)
	{
	default: LOG_WARNING(RSX, "Unsupported Logic Op %d", op);
	case CELL_GCM_CLEAR: return D3D12_LOGIC_OP_CLEAR;
	case CELL_GCM_AND: return D3D12_LOGIC_OP_AND;
	case CELL_GCM_AND_REVERSE: return D3D12_LOGIC_OP_AND_REVERSE;
	case CELL_GCM_COPY: return D3D12_LOGIC_OP_COPY;
	case CELL_GCM_AND_INVERTED: return D3D12_LOGIC_OP_AND_INVERTED;
	case CELL_GCM_NOOP: return D3D12_LOGIC_OP_NOOP;
	case CELL_GCM_XOR: return D3D12_LOGIC_OP_XOR;
	case CELL_GCM_OR: return D3D12_LOGIC_OP_OR;
	case CELL_GCM_NOR: return D3D12_LOGIC_OP_NOR;
	case CELL_GCM_EQUIV: return D3D12_LOGIC_OP_EQUIV;
	case CELL_GCM_INVERT: return D3D12_LOGIC_OP_INVERT;
	case CELL_GCM_OR_REVERSE: return D3D12_LOGIC_OP_OR_REVERSE;
	case CELL_GCM_COPY_INVERTED: return D3D12_LOGIC_OP_COPY_INVERTED;
	case CELL_GCM_OR_INVERTED: return D3D12_LOGIC_OP_OR_INVERTED;
	case CELL_GCM_NAND: return D3D12_LOGIC_OP_NAND;
	}
}

static D3D12_STENCIL_OP getStencilOp(u32 op)
{
	switch (op)
	{
	case GL_KEEP:
		return D3D12_STENCIL_OP_KEEP;
	case GL_ZERO:
		return D3D12_STENCIL_OP_ZERO;
	case GL_REPLACE:
		return D3D12_STENCIL_OP_REPLACE;
	case GL_INCR:
		return D3D12_STENCIL_OP_INCR;
	case GL_DECR:
		return D3D12_STENCIL_OP_DECR;
	case GL_INVERT:
		return D3D12_STENCIL_OP_INVERT;
	}
}

static D3D12_COMPARISON_FUNC getStencilFunc(u32 op)
{
	switch (op)
	{
	case GL_NEVER:
		return D3D12_COMPARISON_FUNC_NEVER;
	case GL_LESS:
		return D3D12_COMPARISON_FUNC_LESS;
	case GL_LEQUAL:
		return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case GL_GREATER:
		return D3D12_COMPARISON_FUNC_GREATER;
	case GL_GEQUAL:
		return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case GL_EQUAL:
		return D3D12_COMPARISON_FUNC_EQUAL;
	case GL_NOTEQUAL:
		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case GL_ALWAYS:
		return D3D12_COMPARISON_FUNC_ALWAYS;
	}
}

bool D3D12GSRender::LoadProgram()
{
	if (!m_cur_fragment_prog)
	{
		LOG_WARNING(RSX, "LoadProgram: m_cur_shader_prog == NULL");
		return false;
	}

	m_cur_fragment_prog->ctrl = m_shader_ctrl;

	if (!m_cur_vertex_prog)
	{
		LOG_WARNING(RSX, "LoadProgram: m_cur_vertex_prog == NULL");
		return false;
	}

	D3D12PipelineProperties prop = {};
	switch (m_draw_mode - 1)
	{
	case GL_POINTS:
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		break;
	case GL_LINES:
	case GL_LINE_LOOP:
	case GL_LINE_STRIP:
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	case GL_TRIANGLES:
	case GL_TRIANGLE_STRIP:
	case GL_TRIANGLE_FAN:
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	case GL_QUADS:
	case GL_QUAD_STRIP:
	case GL_POLYGON:
	default:
//		LOG_ERROR(RSX, "Unsupported primitive type");
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	}

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
	prop.Blend = CD3D12_BLEND_DESC;

	if (m_set_blend)
	{
		prop.Blend.RenderTarget[0].BlendEnable = true;
	}

	if (m_set_blend_equation)
	{
		prop.Blend.RenderTarget[0].BlendOp = getBlendOp(m_blend_equation_rgb);
		prop.Blend.RenderTarget[0].BlendOpAlpha = getBlendOp(m_blend_equation_alpha);
	}

	if (m_set_blend_sfactor && m_set_blend_dfactor)
	{
		prop.Blend.RenderTarget[0].SrcBlend = getBlendFactor(m_blend_sfactor_rgb);
		prop.Blend.RenderTarget[0].DestBlend = getBlendFactor(m_blend_dfactor_rgb);
		prop.Blend.RenderTarget[0].SrcBlendAlpha = getBlendFactor(m_blend_sfactor_alpha);
		prop.Blend.RenderTarget[0].DestBlendAlpha = getBlendFactor(m_blend_dfactor_alpha);
	}

	if (m_set_logic_op)
	{
		prop.Blend.RenderTarget[0].LogicOpEnable = true;
		prop.Blend.RenderTarget[0].LogicOp = getLogicOp(m_logic_op);
	}

	if (m_set_blend_color)
	{
//		glBlendColor(m_blend_color_r, m_blend_color_g, m_blend_color_b, m_blend_color_a);
//		checkForGlError("glBlendColor");
	}

	switch (m_surface_depth_format)
	{
	case 0:
		break;
	case CELL_GCM_SURFACE_Z16:
		prop.DepthStencilFormat = DXGI_FORMAT_D16_UNORM;
		break;
	case CELL_GCM_SURFACE_Z24S8:
		prop.DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		break;
	default:
		LOG_ERROR(RSX, "Bad depth format! (%d)", m_surface_depth_format);
		assert(0);
	}

	switch (m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_0:
	case CELL_GCM_SURFACE_TARGET_1:
		prop.numMRT = 1;
		break;
	case CELL_GCM_SURFACE_TARGET_MRT1:
		prop.numMRT = 2;
		break;
	case CELL_GCM_SURFACE_TARGET_MRT2:
		prop.numMRT = 3;
		break;
	case CELL_GCM_SURFACE_TARGET_MRT3:
		prop.numMRT = 4;
		break;
	default:
		LOG_ERROR(RSX, "Bad surface color target: %d", m_surface_color_target);
	}

	prop.DepthStencil.DepthEnable = m_set_depth_test;
	prop.DepthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	prop.DepthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	prop.DepthStencil.StencilEnable = m_set_stencil_test;
	prop.DepthStencil.StencilReadMask = m_stencil_func_mask;
	prop.DepthStencil.StencilWriteMask = m_set_stencil_mask;
	prop.DepthStencil.FrontFace.StencilPassOp = getStencilOp(m_stencil_zpass);
	prop.DepthStencil.FrontFace.StencilDepthFailOp = getStencilOp(m_stencil_zfail);
	prop.DepthStencil.FrontFace.StencilFailOp = getStencilOp(m_stencil_fail);
	prop.DepthStencil.FrontFace.StencilFunc = getStencilFunc(m_stencil_func);

	if (m_set_two_sided_stencil_test_enable)
	{
		prop.DepthStencil.BackFace.StencilFailOp = getStencilOp(m_stencil_fail);
		prop.DepthStencil.BackFace.StencilFunc = getStencilFunc(m_stencil_func);
		prop.DepthStencil.BackFace.StencilPassOp = getStencilOp(m_stencil_zpass);
		prop.DepthStencil.BackFace.StencilDepthFailOp = getStencilOp(m_stencil_zfail);
	}
	else
	{
		prop.DepthStencil.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
		prop.DepthStencil.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		prop.DepthStencil.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		prop.DepthStencil.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	}


	prop.IASet = m_IASet;

	m_PSO = m_cachePSO.getGraphicPipelineState(m_cur_vertex_prog, m_cur_fragment_prog, prop, std::make_pair(m_device, m_rootSignatures));
	return m_PSO != nullptr;
}

void D3D12GSRender::ExecCMD()
{
	InitDrawBuffers();

	// Init vertex count
	// TODO: Very hackish, clean this
	if (m_indexed_array.m_count)
	{
		for (u32 i = 0; i < m_vertex_count; ++i)
		{
			if (!m_vertex_data[i].IsEnabled()) continue;
			if (!m_vertex_data[i].addr) continue;

			const u32 tsize = m_vertex_data[i].GetTypeSize();
			m_vertex_data[i].data.resize((m_indexed_array.index_min + m_indexed_array.index_max - m_indexed_array.index_min + 1) * tsize * m_vertex_data[i].size);
		}
	}
	else
	{
		for (u32 i = 0; i < m_vertex_count; ++i)
		{
			if (!m_vertex_data[i].IsEnabled()) continue;
			if (!m_vertex_data[i].addr) continue;

			const u32 tsize = m_vertex_data[i].GetTypeSize();
			m_vertex_data[i].data.resize((m_draw_array_first + m_draw_array_count) * tsize * m_vertex_data[i].size);
		}
	}

	ID3D12GraphicsCommandList *commandList;
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, getCurrentResourceStorage().m_commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	getCurrentResourceStorage().m_inflightCommandList.push_back(commandList);

	if (m_indexed_array.m_count || m_draw_array_count)
	{
		const std::pair<std::vector<D3D12_VERTEX_BUFFER_VIEW>, D3D12_INDEX_BUFFER_VIEW> &vertexIndexBufferViews = EnableVertexData(m_indexed_array.m_count ? true : false);
		commandList->IASetVertexBuffers(0, (UINT)vertexIndexBufferViews.first.size(), vertexIndexBufferViews.first.data());
		if (m_forcedIndexBuffer || m_indexed_array.m_count)
			commandList->IASetIndexBuffer(&vertexIndexBufferViews.second);
	}

	if (!LoadProgram())
	{
		LOG_ERROR(RSX, "LoadProgram failed.");
		Emu.Pause();
		return;
	}

	commandList->SetGraphicsRootSignature(m_rootSignatures[m_PSO->second]);
	commandList->OMSetStencilRef(m_stencil_func_ref);

	// Constants
	setScaleOffset();
	commandList->SetDescriptorHeaps(1, &getCurrentResourceStorage().m_scaleOffsetDescriptorHeap);
	D3D12_GPU_DESCRIPTOR_HANDLE Handle = getCurrentResourceStorage().m_scaleOffsetDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	Handle.ptr += getCurrentResourceStorage().m_currentScaleOffsetBufferIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	commandList->SetGraphicsRootDescriptorTable(0, Handle);
	getCurrentResourceStorage().m_currentScaleOffsetBufferIndex++;

	size_t currentBufferIndex = getCurrentResourceStorage().m_constantsBufferIndex;
	FillVertexShaderConstantsBuffer();
	getCurrentResourceStorage().m_constantsBufferIndex++;
	FillPixelShaderConstantsBuffer();
	getCurrentResourceStorage().m_constantsBufferIndex++;

	commandList->SetDescriptorHeaps(1, &getCurrentResourceStorage().m_constantsBufferDescriptorsHeap);
	Handle = getCurrentResourceStorage().m_constantsBufferDescriptorsHeap->GetGPUDescriptorHandleForHeapStart();
	Handle.ptr += currentBufferIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	commandList->SetGraphicsRootDescriptorTable(1, Handle);
	commandList->SetPipelineState(m_PSO->first);

	if (m_PSO->second > 0)
	{
		size_t usedTexture = UploadTextures();

		Handle = getCurrentResourceStorage().m_textureDescriptorsHeap->GetGPUDescriptorHandleForHeapStart();
		Handle.ptr += getCurrentResourceStorage().m_currentTextureIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		commandList->SetDescriptorHeaps(1, &getCurrentResourceStorage().m_textureDescriptorsHeap);
		commandList->SetGraphicsRootDescriptorTable(2, Handle);

		Handle = getCurrentResourceStorage().m_samplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		Handle.ptr += getCurrentResourceStorage().m_currentTextureIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		commandList->SetDescriptorHeaps(1, &getCurrentResourceStorage().m_samplerDescriptorHeap);
		commandList->SetGraphicsRootDescriptorTable(3, Handle);

		getCurrentResourceStorage().m_currentTextureIndex += usedTexture;
	}

	size_t numRTT;
	switch (m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_NONE: break;
	case CELL_GCM_SURFACE_TARGET_0:
	case CELL_GCM_SURFACE_TARGET_1:
		numRTT = 1;
		break;
	case CELL_GCM_SURFACE_TARGET_MRT1:
		numRTT = 2;
		break;
	case CELL_GCM_SURFACE_TARGET_MRT2:
		numRTT = 3;
		break;
	case CELL_GCM_SURFACE_TARGET_MRT3:
		numRTT = 4;
		break;
	default:
		LOG_ERROR(RSX, "Bad surface color target: %d", m_surface_color_target);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE *DepthStencilHandle = &m_rtts.m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	commandList->OMSetRenderTargets((UINT)numRTT, &m_rtts.m_renderTargetsDescriptorsHeap->GetCPUDescriptorHandleForHeapStart(), true, DepthStencilHandle);

	D3D12_VIEWPORT viewport =
	{
		0.f,
		0.f,
		(float)RSXThread::m_width,
		(float)RSXThread::m_height,
		-1.f,
		1.f
	};
	commandList->RSSetViewports(1, &viewport);
	D3D12_RECT box =
	{
		0, 0,
		(LONG)RSXThread::m_width, (LONG)RSXThread::m_height,
	};
	commandList->RSSetScissorRects(1, &box);

	bool requireIndexBuffer = false;
	switch (m_draw_mode - 1)
	{
	case GL_POINTS:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		break;
	case GL_LINES:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		break;
	case GL_LINE_LOOP:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ);
		break;
	case GL_LINE_STRIP:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
		break;
	case GL_TRIANGLES:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		break;
	case GL_TRIANGLE_STRIP:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		break;
	case GL_TRIANGLE_FAN:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ);
		break;
	case GL_QUADS:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		requireIndexBuffer = true;
	case GL_QUAD_STRIP:
	case GL_POLYGON:
	default:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//		LOG_ERROR(RSX, "Unsupported primitive type");
		break;
	}

	// Indexed quad
	if (m_forcedIndexBuffer && m_indexed_array.m_count)
		commandList->DrawIndexedInstanced((UINT)indexCount, 1, 0, 0, 0);
	// Non indexed quad
	else if (m_forcedIndexBuffer && !m_indexed_array.m_count)
		commandList->DrawIndexedInstanced((UINT)indexCount, 1, 0, (UINT)m_draw_array_first, 0);
	// Indexed triangles
	else if (m_indexed_array.m_count)
		commandList->DrawIndexedInstanced((UINT)m_indexed_array.m_data.size() / 4, 1, 0, (UINT)m_draw_array_first, 0);
	else if (m_draw_array_count)
		commandList->DrawInstanced(m_draw_array_count, 1, m_draw_array_first, 0);

	check(commandList->Close());
	m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)&commandList);
	m_indexed_array.Reset();
	WriteDepthBuffer();
}

void D3D12GSRender::Flip()
{
	ID3D12GraphicsCommandList *commandList;
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, getCurrentResourceStorage().m_commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	getCurrentResourceStorage().m_inflightCommandList.push_back(commandList);

	switch (m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_0:
	case CELL_GCM_SURFACE_TARGET_1:
	case CELL_GCM_SURFACE_TARGET_MRT1:
	case CELL_GCM_SURFACE_TARGET_MRT2:
	case CELL_GCM_SURFACE_TARGET_MRT3:
	{
		D3D12_RESOURCE_BARRIER barriers[2] = {};
		barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[0].Transition.pResource = m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()];
		barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

		barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[1].Transition.pResource = m_rtts.m_currentlyBoundRenderTargets[0];
		barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

		commandList->ResourceBarrier(2, barriers);
		D3D12_TEXTURE_COPY_LOCATION src = {}, dst = {};
		src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.SubresourceIndex = 0, dst.SubresourceIndex = 0;
		src.pResource = m_rtts.m_currentlyBoundRenderTargets[0], dst.pResource = m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()];
		D3D12_BOX box = { 0, 0, 0, RSXThread::m_width, RSXThread::m_height, 1 };
		commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, &box);

		barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
		barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		commandList->ResourceBarrier(2, barriers);
		commandList->Close();
		m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)&commandList);
	}
	}

	check(m_swapChain->Present(Ini.GSVSyncEnable.GetValue() ? 1 : 0, 0));
	// Add an event signaling queue completion

	m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&getNonCurrentResourceStorage().m_frameFinishedFence));
	getNonCurrentResourceStorage().m_frameFinishedHandle = CreateEvent(0, 0, 0, 0);
	getNonCurrentResourceStorage().m_frameFinishedFence->SetEventOnCompletion(1, getNonCurrentResourceStorage().m_frameFinishedHandle);
	m_commandQueueGraphic->Signal(getNonCurrentResourceStorage().m_frameFinishedFence, 1);

	// Flush
	m_texturesCache.clear();
	m_texturesRTTs.clear();

	if (getCurrentResourceStorage().m_frameFinishedHandle)
	{
		WaitForSingleObject(getCurrentResourceStorage().m_frameFinishedHandle, INFINITE);
		CloseHandle(getCurrentResourceStorage().m_frameFinishedHandle);
		getCurrentResourceStorage().m_frameFinishedFence->Release();

		for (auto tmp : getCurrentResourceStorage().m_inUseConstantsBuffers)
			m_constantsData.m_getPos = std::get<0>(tmp);
		for (auto tmp : getCurrentResourceStorage().m_inUseVertexIndexBuffers)
			m_vertexIndexData.m_getPos = std::get<0>(tmp);
		for (auto tmp : getCurrentResourceStorage().m_inUseTextureUploadBuffers)
			m_textureUploadData.m_getPos = std::get<0>(tmp);
		for (auto tmp : getCurrentResourceStorage().m_inUseTexture2D)
			m_textureData.m_getPos = std::get<0>(tmp);
		getCurrentResourceStorage().Reset();
	}

	getCurrentResourceStorage().m_inUseConstantsBuffers = m_constantsData.m_resourceStoredSinceLastSync;
	m_constantsData.m_resourceStoredSinceLastSync.clear();
	getCurrentResourceStorage().m_inUseVertexIndexBuffers = m_vertexIndexData.m_resourceStoredSinceLastSync;
	m_vertexIndexData.m_resourceStoredSinceLastSync.clear();
	getCurrentResourceStorage().m_inUseTextureUploadBuffers = m_textureUploadData.m_resourceStoredSinceLastSync;
	m_textureUploadData.m_resourceStoredSinceLastSync.clear();
	getCurrentResourceStorage().m_inUseTexture2D = m_textureData.m_resourceStoredSinceLastSync;
	m_textureData.m_resourceStoredSinceLastSync.clear();

	m_frame->Flip(nullptr);
}

D3D12GSRender::ResourceStorage& D3D12GSRender::getCurrentResourceStorage()
{
	return m_perFrameStorage[m_swapChain->GetCurrentBackBufferIndex()];
}

D3D12GSRender::ResourceStorage& D3D12GSRender::getNonCurrentResourceStorage()
{
	return m_perFrameStorage[1 - m_swapChain->GetCurrentBackBufferIndex()];
}


void D3D12GSRender::WriteDepthBuffer()
{
}

ID3D12Resource * D3D12GSRender::writeColorBuffer(ID3D12Resource * RTT, ID3D12GraphicsCommandList * cmdlist)
{
	ID3D12Resource *Result;
	size_t rowPitch = RSXThread::m_width * 4;
	rowPitch = (rowPitch + 255) & ~255;

	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_READBACK;
	D3D12_RESOURCE_DESC resdesc = getBufferResourceDesc(rowPitch * RSXThread::m_height);

	size_t heapOffset = powerOf2Align(m_readbackResources.m_putPos.load(), 65536);
	size_t sizeInByte = rowPitch * RSXThread::m_height;

	if (heapOffset + sizeInByte >= 1024 * 1024 * 128) // If it will be stored past heap size
		heapOffset = 0;

	resdesc = getBufferResourceDesc(sizeInByte);

	check(
		m_device->CreatePlacedResource(
			m_readbackResources.m_heap,
			heapOffset,
			&resdesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&Result)
			)
		);

	cmdlist->ResourceBarrier(1, &getResourceBarrierTransition(RTT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

	D3D12_TEXTURE_COPY_LOCATION dst = {}, src = {};
	src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	src.pResource = RTT;
	dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	dst.pResource = Result;
	dst.PlacedFootprint.Offset = 0;
	dst.PlacedFootprint.Footprint.Depth = 1;
	dst.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dst.PlacedFootprint.Footprint.Height = (UINT)RSXThread::m_height;
	dst.PlacedFootprint.Footprint.Width = (UINT)RSXThread::m_width;
	dst.PlacedFootprint.Footprint.RowPitch = (UINT)rowPitch;
	cmdlist->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	cmdlist->ResourceBarrier(1, &getResourceBarrierTransition(RTT, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
	return Result;
}

static
void copyToCellRamAndRelease(void *dstAddress, ID3D12Resource *res, size_t rowPitch, size_t width, size_t height)
{
	void *srcBuffer;
	check(res->Map(0, nullptr, &srcBuffer));
	for (unsigned row = 0; row < height; row++)
		memcpy((char*)dstAddress + row * width * 4, (char*)srcBuffer + row * rowPitch, width * 4);
	res->Unmap(0, nullptr);
	res->Release();
}


void D3D12GSRender::semaphorePGRAPHBackendRelease(u32 offset, u32 value)
{
	// Add all buffer write
	// Cell can't make any assumption about readyness of color/depth buffer
	// Except when a semaphore is written by RSX


/*	if (!Ini.GSDumpDepthBuffer.GetValue())
		return;*/

	ID3D12Fence *fence;
	check(
		m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))
		);
	HANDLE handle = CreateEvent(0, FALSE, FALSE, 0);
	fence->SetEventOnCompletion(1, handle);

	ID3D12Resource *writeDest, *depthConverted;
	ID3D12GraphicsCommandList *convertCommandList;
	ID3D12DescriptorHeap *descriptorHeap;
	size_t depthRowPitch = RSXThread::m_width;
	depthRowPitch = (depthRowPitch + 255) & ~255;

	bool needTransfer = m_set_context_dma_z || m_set_context_dma_color_a || m_set_context_dma_color_b || m_set_context_dma_color_c || m_set_context_dma_color_d;

	if (m_set_context_dma_z)
	{
		D3D12_HEAP_PROPERTIES heapProp = {};
		heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_RESOURCE_DESC resdesc = getTexture2DResourceDesc(RSXThread::m_width, RSXThread::m_height, DXGI_FORMAT_R8_UNORM);
		resdesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		size_t heapOffset = m_readbackResources.m_putPos.load();
		heapOffset = powerOf2Align(heapOffset, 65536);
		size_t sizeInByte = RSXThread::m_width * RSXThread::m_height;
		if (heapOffset + sizeInByte >= 1024 * 1024 * 128) // If it will be stored past heap size
			heapOffset = 0;

		check(
			m_device->CreatePlacedResource(
				m_UAVHeap.m_heap,
				heapOffset,
				&resdesc,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				nullptr,
				IID_PPV_ARGS(&depthConverted)
				)
			);
		m_UAVHeap.m_putPos.store(heapOffset + sizeInByte);

		heapOffset = m_readbackResources.m_putPos.load();
		heapOffset = powerOf2Align(heapOffset, 65536);
		sizeInByte = depthRowPitch * RSXThread::m_height;

		if (heapOffset + sizeInByte >= 1024 * 1024 * 128) // If it will be stored past heap size
			heapOffset = 0;
		resdesc = getBufferResourceDesc(sizeInByte);

		check(
			m_device->CreatePlacedResource(
				m_readbackResources.m_heap,
				heapOffset,
				&resdesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&writeDest)
				)
			);
		m_readbackResources.m_putPos.store(heapOffset + sizeInByte);

		check(
			m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, getCurrentResourceStorage().m_commandAllocator, nullptr, IID_PPV_ARGS(&convertCommandList))
			);

		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
		descriptorHeapDesc.NumDescriptors = 2;
		descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		check(
			m_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap))
			);
		D3D12_CPU_DESCRIPTOR_HANDLE Handle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		switch (m_surface_depth_format)
		{
		case 0:
			break;
		case CELL_GCM_SURFACE_Z16:
			srvDesc.Format = DXGI_FORMAT_R16_UNORM;
			break;
		case CELL_GCM_SURFACE_Z24S8:
			srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			break;
		default:
			LOG_ERROR(RSX, "Bad depth format! (%d)", m_surface_depth_format);
			assert(0);
		}
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		m_device->CreateShaderResourceView(m_rtts.m_currentlyBoundDepthStencil, &srvDesc, Handle);
		Handle.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R8_UNORM;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		m_device->CreateUnorderedAccessView(depthConverted, nullptr, &uavDesc, Handle);

		// Convert
		convertCommandList->ResourceBarrier(1, &getResourceBarrierTransition(m_rtts.m_currentlyBoundDepthStencil, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));

		convertCommandList->SetPipelineState(m_convertPSO);
		convertCommandList->SetComputeRootSignature(m_convertRootSignature);
		convertCommandList->SetDescriptorHeaps(1, &descriptorHeap);
		convertCommandList->SetComputeRootDescriptorTable(0, descriptorHeap->GetGPUDescriptorHandleForHeapStart());
		convertCommandList->Dispatch(RSXThread::m_width / 8, RSXThread::m_height / 8, 1);

		// Flush UAV
		D3D12_RESOURCE_BARRIER uavbarrier = {};
		uavbarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavbarrier.UAV.pResource = depthConverted;

		D3D12_RESOURCE_BARRIER barriers[] =
		{
			getResourceBarrierTransition(m_rtts.m_currentlyBoundDepthStencil, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE),
			uavbarrier,
		};
		convertCommandList->ResourceBarrier(2, barriers);
		convertCommandList->ResourceBarrier(1, &getResourceBarrierTransition(depthConverted, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));

		convertCommandList->Close();
		m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)&convertCommandList);
	}

	ID3D12GraphicsCommandList *downloadCommandList;
	if (needTransfer)
	{
		check(
			m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, getCurrentResourceStorage().m_commandAllocator, nullptr, IID_PPV_ARGS(&downloadCommandList))
			);
	}

	if (m_set_context_dma_z)
	{
		// Copy
		D3D12_TEXTURE_COPY_LOCATION dst = {}, src = {};
		src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.pResource = depthConverted;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		dst.pResource = writeDest;
		dst.PlacedFootprint.Offset = 0;
		dst.PlacedFootprint.Footprint.Depth = 1;
		dst.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8_UNORM;
		dst.PlacedFootprint.Footprint.Height = RSXThread::m_height;
		dst.PlacedFootprint.Footprint.Width = RSXThread::m_width;
		dst.PlacedFootprint.Footprint.RowPitch = (UINT)depthRowPitch;
		downloadCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	}

	ID3D12Resource *rtt0, *rtt1, *rtt2, *rtt3;
	switch (m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_NONE:
		break;

	case CELL_GCM_SURFACE_TARGET_0:
		if (m_context_dma_color_a) rtt0 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[0], downloadCommandList);
		break;

	case CELL_GCM_SURFACE_TARGET_1:
		if (m_context_dma_color_b) rtt1 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[0], downloadCommandList);
		break;

	case CELL_GCM_SURFACE_TARGET_MRT1:
		if (m_context_dma_color_a) rtt0 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[0], downloadCommandList);
		if (m_context_dma_color_b) rtt1 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[1], downloadCommandList);
		break;

	case CELL_GCM_SURFACE_TARGET_MRT2:
		if (m_context_dma_color_a) rtt0 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[0], downloadCommandList);
		if (m_context_dma_color_b) rtt1 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[1], downloadCommandList);
		if (m_context_dma_color_c) rtt2 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[2], downloadCommandList);
		break;

	case CELL_GCM_SURFACE_TARGET_MRT3:
		if (m_context_dma_color_a) rtt0 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[0], downloadCommandList);
		if (m_context_dma_color_b) rtt1 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[1], downloadCommandList);
		if (m_context_dma_color_c) rtt2 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[2], downloadCommandList);
		if (m_context_dma_color_d) rtt3 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[3], downloadCommandList);
		break;
	}
	if (needTransfer)
	{
		downloadCommandList->Close();
		m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)&downloadCommandList);
	}

	//Wait for result
	m_commandQueueGraphic->Signal(fence, 1);

	std::thread valueChangerThread([=]() {
		WaitForSingleObject(handle, INFINITE);
		CloseHandle(handle);
		fence->Release();

		if (m_set_context_dma_z)
		{
			u32 address = GetAddress(m_surface_offset_z, m_context_dma_z - 0xfeed0000);
			auto ptr = vm::get_ptr<void>(address);
			char *ptrAsChar = (char*)ptr;
			unsigned char *writeDestPtr;
			check(writeDest->Map(0, nullptr, (void**)&writeDestPtr));
			// TODO : this should be done by the gpu
			for (unsigned row = 0; row < RSXThread::m_height; row++)
			{
				for (unsigned i = 0; i < RSXThread::m_width; i++)
				{
					unsigned char c = writeDestPtr[row * depthRowPitch + i];
					ptrAsChar[4 * (row * RSXThread::m_width + i)] = c;
					ptrAsChar[4 * (row * RSXThread::m_width + i) + 1] = c;
					ptrAsChar[4 * (row * RSXThread::m_width + i) + 2] = c;
					ptrAsChar[4 * (row * RSXThread::m_width + i) + 3] = c;
				}
			}
			writeDest->Release();
			depthConverted->Release();
			descriptorHeap->Release();
			convertCommandList->Release();
		}

		size_t colorRowPitch = RSXThread::m_width * 4;
		colorRowPitch = (colorRowPitch + 255) & ~255;

		switch (m_surface_color_target)
		{
		case CELL_GCM_SURFACE_TARGET_NONE:
			break;

		case CELL_GCM_SURFACE_TARGET_0:
		{
			u32 address = GetAddress(m_surface_offset_a, m_context_dma_color_a - 0xfeed0000);
			void *dstAddress = vm::get_ptr<void>(address);
			copyToCellRamAndRelease(dstAddress, rtt0, colorRowPitch, RSXThread::m_width, RSXThread::m_height);
		}
			break;

		case CELL_GCM_SURFACE_TARGET_1:
		{
			u32 address = GetAddress(m_surface_offset_b, m_context_dma_color_b - 0xfeed0000);
			void *dstAddress = vm::get_ptr<void>(address);
			copyToCellRamAndRelease(dstAddress, rtt1, colorRowPitch, RSXThread::m_width, RSXThread::m_height);
		}
			break;

		case CELL_GCM_SURFACE_TARGET_MRT1:
		{
			u32 address = GetAddress(m_surface_offset_a, m_context_dma_color_a - 0xfeed0000);
			void *dstAddress = vm::get_ptr<void>(address);
			copyToCellRamAndRelease(dstAddress, rtt0, colorRowPitch, RSXThread::m_width, RSXThread::m_height);
			address = GetAddress(m_surface_offset_b, m_context_dma_color_b - 0xfeed0000);
			dstAddress = vm::get_ptr<void>(address);
			copyToCellRamAndRelease(dstAddress, rtt1, colorRowPitch, RSXThread::m_width, RSXThread::m_height);
		}
			break;

		case CELL_GCM_SURFACE_TARGET_MRT2:
		{
			u32 address = GetAddress(m_surface_offset_a, m_context_dma_color_a - 0xfeed0000);
			void *dstAddress = vm::get_ptr<void>(address);
			copyToCellRamAndRelease(dstAddress, rtt0, colorRowPitch, RSXThread::m_width, RSXThread::m_height);
			address = GetAddress(m_surface_offset_b, m_context_dma_color_b - 0xfeed0000);
			dstAddress = vm::get_ptr<void>(address);
			copyToCellRamAndRelease(dstAddress, rtt1, colorRowPitch, RSXThread::m_width, RSXThread::m_height);
			address = GetAddress(m_surface_offset_c, m_context_dma_color_c - 0xfeed0000);
			dstAddress = vm::get_ptr<void>(address);
			copyToCellRamAndRelease(dstAddress, rtt2, colorRowPitch, RSXThread::m_width, RSXThread::m_height);
		}
			break;

		case CELL_GCM_SURFACE_TARGET_MRT3:
		{
			u32 address = GetAddress(m_surface_offset_a, m_context_dma_color_a - 0xfeed0000);
			void *dstAddress = vm::get_ptr<void>(address);
			copyToCellRamAndRelease(dstAddress, rtt0, colorRowPitch, RSXThread::m_width, RSXThread::m_height);
			address = GetAddress(m_surface_offset_b, m_context_dma_color_b - 0xfeed0000);
			dstAddress = vm::get_ptr<void>(address);
			copyToCellRamAndRelease(dstAddress, rtt1, colorRowPitch, RSXThread::m_width, RSXThread::m_height);
			address = GetAddress(m_surface_offset_c, m_context_dma_color_c - 0xfeed0000);
			dstAddress = vm::get_ptr<void>(address);
			copyToCellRamAndRelease(dstAddress, rtt2, colorRowPitch, RSXThread::m_width, RSXThread::m_height);
			address = GetAddress(m_surface_offset_d, m_context_dma_color_d - 0xfeed0000);
			dstAddress = vm::get_ptr<void>(address);
			copyToCellRamAndRelease(dstAddress, rtt3, colorRowPitch, RSXThread::m_width, RSXThread::m_height);
		}
			break;
		}

		if (needTransfer)
			downloadCommandList->Release();

		vm::write32(m_label_addr + offset, value);
	});
	valueChangerThread.detach();
}

void D3D12GSRender::semaphorePFIFOAcquire(u32 offset, u32 value)
{
	ID3D12Fence *fence;
	check(
		m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))
		);
	m_commandQueueGraphic->Wait(fence, 1);

	std::thread valueChangerThread([=]() {
		while (true)
		{
			u32 val = vm::read32(m_label_addr + offset);
			if (val == value) break;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		fence->Signal(1);
		fence->Release();
	}
	);
	valueChangerThread.detach();
}
#endif
