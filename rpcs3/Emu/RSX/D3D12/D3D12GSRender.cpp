#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "D3D12GSRender.h"
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <thread>
#include <chrono>
#include "d3dx12.h"
#include <d3d11on12.h>
#include "Emu/state.h"

PFN_D3D12_CREATE_DEVICE wrapD3D12CreateDevice;
PFN_D3D12_GET_DEBUG_INTERFACE wrapD3D12GetDebugInterface;
PFN_D3D12_SERIALIZE_ROOT_SIGNATURE wrapD3D12SerializeRootSignature;
PFN_D3D11ON12_CREATE_DEVICE wrapD3D11On12CreateDevice;

static HMODULE D3D12Module;
static HMODULE D3D11Module;

static void loadD3D12FunctionPointers()
{
	D3D12Module = LoadLibrary(L"d3d12.dll");
	wrapD3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(D3D12Module, "D3D12CreateDevice");
	wrapD3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(D3D12Module, "D3D12GetDebugInterface");
	wrapD3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(D3D12Module, "D3D12SerializeRootSignature");
	D3D11Module = LoadLibrary(L"d3d11.dll");
	wrapD3D11On12CreateDevice = (PFN_D3D11ON12_CREATE_DEVICE)GetProcAddress(D3D11Module, "D3D11On12CreateDevice");
}

static void unloadD3D12FunctionPointers()
{
	FreeLibrary(D3D12Module);
	FreeLibrary(D3D11Module);
}

void D3D12GSRender::ResourceStorage::Reset()
{
	m_descriptorsHeapIndex = 0;
	m_currentSamplerIndex = 0;
	m_samplerDescriptorHeapIndex = 0;

	ThrowIfFailed(m_commandAllocator->Reset());
	setNewCommandList();

	m_singleFrameLifetimeResources.clear();
}

void D3D12GSRender::ResourceStorage::setNewCommandList()
{
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
}

void D3D12GSRender::ResourceStorage::Init(ID3D12Device *device)
{
	m_inUse = false;
	m_device = device;
	m_RAMFramebuffer = nullptr;
	// Create a global command allocator
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocator.GetAddressOf())));

	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(m_commandList.GetAddressOf())));
	ThrowIfFailed(m_commandList->Close());

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 10000, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };
	ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_descriptorsHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER , 2048, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };
	ThrowIfFailed(device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_samplerDescriptorHeap[0])));
	ThrowIfFailed(device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_samplerDescriptorHeap[1])));

	m_frameFinishedHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	m_fenceValue = 0;
	ThrowIfFailed(device->CreateFence(m_fenceValue++, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_frameFinishedFence.GetAddressOf())));
}

void D3D12GSRender::ResourceStorage::WaitAndClean()
{
	if (m_inUse)
		WaitForSingleObjectEx(m_frameFinishedHandle, INFINITE, FALSE);
	else
		ThrowIfFailed(m_commandList->Close());

	Reset();

	m_dirtyTextures.clear();

	m_RAMFramebuffer = nullptr;
}

void D3D12GSRender::ResourceStorage::Release()
{
	m_dirtyTextures.clear();
	// NOTE: Should be released only after gfx pipeline last command has been finished.
	CloseHandle(m_frameFinishedHandle);
}


void D3D12GSRender::Shader::Release()
{
	m_PSO->Release();
	m_rootSignature->Release();
	m_vertexBuffer->Release();
	m_textureDescriptorHeap->Release();
	m_samplerDescriptorHeap->Release();
}

extern std::function<bool(u32 addr)> gfxHandler;

bool D3D12GSRender::invalidateAddress(u32 addr)
{
	bool result = false;
	result |= m_textureCache.invalidateAddress(addr);
	return result;
}

D3D12DLLManagement::D3D12DLLManagement()
{
	loadD3D12FunctionPointers();
}

D3D12DLLManagement::~D3D12DLLManagement()
{
	unloadD3D12FunctionPointers();
}

D3D12GSRender::D3D12GSRender()
	: GSRender(frame_type::DX12), m_D3D12Lib(), m_PSO(nullptr)
{
	m_previous_address_a = 0;
	m_previous_address_b = 0;
	m_previous_address_c = 0;
	m_previous_address_d = 0;
	m_previous_address_z = 0;
	gfxHandler = [this](u32 addr) {
		bool result = invalidateAddress(addr);
		if (result)
				LOG_WARNING(RSX, "Reporting Cell writing to %x", addr);
		return result;
	};
	if (Ini.GSDebugOutputEnable.GetValue())
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
		wrapD3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
		debugInterface->EnableDebugLayer();
	}

	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory)));
	// Create adapter
	ComPtr<IDXGIAdapter> adaptater = nullptr;
	ThrowIfFailed(dxgiFactory->EnumAdapters(Ini.GSD3DAdaptater.GetValue(), adaptater.GetAddressOf()));
	ThrowIfFailed(wrapD3D12CreateDevice(adaptater.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));

	// Queues
	D3D12_COMMAND_QUEUE_DESC copyQueueDesc = {}, graphicQueueDesc = {};
	copyQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	graphicQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(m_device->CreateCommandQueue(&graphicQueueDesc, IID_PPV_ARGS(m_commandQueueGraphic.GetAddressOf())));

	g_descriptorStrideSRVCBVUAV = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	g_descriptorStrideDSV = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	g_descriptorStrideRTV = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	g_descriptorStrideSamplers = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	// Create swap chain and put them in a descriptor heap as rendertarget
	DXGI_SWAP_CHAIN_DESC swapChain = {};
	swapChain.BufferCount = 2;
	swapChain.Windowed = true;
	swapChain.OutputWindow = (HWND)m_frame->handle();
	swapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChain.SampleDesc.Count = 1;
	swapChain.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	ThrowIfFailed(dxgiFactory->CreateSwapChain(m_commandQueueGraphic.Get(), &swapChain, (IDXGISwapChain**)m_swapChain.GetAddressOf()));
	m_swapChain->GetBuffer(0, IID_PPV_ARGS(&m_backBuffer[0]));
	m_swapChain->GetBuffer(1, IID_PPV_ARGS(&m_backBuffer[1]));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1};
	D3D12_RENDER_TARGET_VIEW_DESC rttDesc = {};
	rttDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rttDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_backbufferAsRendertarget[0]));
	m_device->CreateRenderTargetView(m_backBuffer[0].Get(), &rttDesc, m_backbufferAsRendertarget[0]->GetCPUDescriptorHandleForHeapStart());
	m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_backbufferAsRendertarget[1]));
	m_device->CreateRenderTargetView(m_backBuffer[1].Get(), &rttDesc, m_backbufferAsRendertarget[1]->GetCPUDescriptorHandleForHeapStart());

	// Common root signatures
	for (unsigned textureCount = 0; textureCount < 17; textureCount++)
	{
		CD3DX12_DESCRIPTOR_RANGE descriptorRange[] =
		{
			// Scale Offset data
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0),
			// Constants
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 1),
			// Textures
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, textureCount, 0),
			// Samplers
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, textureCount, 0),
		};
		CD3DX12_ROOT_PARAMETER RP[2];
		RP[0].InitAsDescriptorTable((textureCount > 0) ? 3 : 2, &descriptorRange[0]);
		RP[1].InitAsDescriptorTable(1, &descriptorRange[3]);

		Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
		ThrowIfFailed(wrapD3D12SerializeRootSignature(
			&CD3DX12_ROOT_SIGNATURE_DESC((textureCount > 0) ? 2 : 1, RP, 0, 0, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),
			D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob));

		m_device->CreateRootSignature(0,
			rootSignatureBlob->GetBufferPointer(),
			rootSignatureBlob->GetBufferSize(),
			IID_PPV_ARGS(m_rootSignatures[textureCount].GetAddressOf()));
	}

	m_perFrameStorage[0].Init(m_device.Get());
	m_perFrameStorage[0].Reset();
	m_perFrameStorage[1].Init(m_device.Get());
	m_perFrameStorage[1].Reset();

	initConvertShader();
	m_outputScalingPass.Init(m_device.Get(), m_commandQueueGraphic.Get());

	ThrowIfFailed(
		m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 2, 2, 1, 1),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_dummyTexture))
			);

	m_readbackResources.Init(m_device.Get(), 1024 * 1024 * 128, D3D12_HEAP_TYPE_READBACK, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
	m_UAVHeap.Init(m_device.Get(), 1024 * 1024 * 128, D3D12_HEAP_TYPE_DEFAULT, D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES);

	m_rtts.Init(m_device.Get());

	m_constantsData.Init(m_device.Get(), 1024 * 1024 * 64, D3D12_HEAP_TYPE_UPLOAD, D3D12_HEAP_FLAG_NONE);
	m_vertexIndexData.Init(m_device.Get(), 1024 * 1024 * 384, D3D12_HEAP_TYPE_UPLOAD, D3D12_HEAP_FLAG_NONE);
	m_textureUploadData.Init(m_device.Get(), 1024 * 1024 * 256, D3D12_HEAP_TYPE_UPLOAD, D3D12_HEAP_FLAG_NONE);

	if (Ini.GSOverlay.GetValue())
		InitD2DStructures();
}

D3D12GSRender::~D3D12GSRender()
{
	// wait until queue has completed
	ComPtr<ID3D12Fence> fence;
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));
	HANDLE handle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	fence->SetEventOnCompletion(1, handle);

	m_commandQueueGraphic->Signal(fence.Get(), 1);
	WaitForSingleObjectEx(handle, INFINITE, FALSE);
	CloseHandle(handle);

	{
		m_textureCache.unprotedAll();
	}

	gfxHandler = [this](u32) { return false; };
	m_constantsData.Release();
	m_vertexIndexData.Release();
	m_textureUploadData.Release();
	m_UAVHeap.m_heap->Release();
	m_readbackResources.m_heap->Release();
	m_texturesRTTs.clear();
	m_dummyTexture->Release();
	m_convertPSO->Release();
	m_convertRootSignature->Release();
	m_perFrameStorage[0].Release();
	m_perFrameStorage[1].Release();
	m_rtts.Release();
	m_outputScalingPass.Release();

	ReleaseD2DStructures();
}

void D3D12GSRender::onexit_thread()
{
}

bool D3D12GSRender::domethod(u32 cmd, u32 arg)
{
	switch (cmd)
	{
	case NV4097_CLEAR_SURFACE:
		clear_surface(arg);
		return true;
	case NV4097_TEXTURE_READ_SEMAPHORE_RELEASE:
		semaphore_PGRAPH_texture_read_release();
		return false; //call rsx::thread method implementation
	case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE:
		semaphore_PGRAPH_backend_release();
		return false; //call rsx::thread method implementation

	default:
		return false;
	}
}

void D3D12GSRender::clear_surface(u32 arg)
{
	std::chrono::time_point<std::chrono::system_clock> startDuration = std::chrono::system_clock::now();

	std::chrono::time_point<std::chrono::system_clock> rttDurationStart = std::chrono::system_clock::now();
	PrepareRenderTargets(getCurrentResourceStorage().m_commandList.Get());

	std::chrono::time_point<std::chrono::system_clock> rttDurationEnd = std::chrono::system_clock::now();
	m_timers.m_rttDuration += std::chrono::duration_cast<std::chrono::microseconds>(rttDurationEnd - rttDurationStart).count();

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
	if (arg & 0x1)
	{
		u32 clear_depth = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] >> 8;
		u32 max_depth_value = m_surface.depth_format == CELL_GCM_SURFACE_Z16 ? 0x0000ffff : 0x00ffffff;
		getCurrentResourceStorage().m_commandList->ClearDepthStencilView(m_rtts.m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, clear_depth / (float)max_depth_value, 0, 0, nullptr);
	}

	if (arg & 0x2)
	{
		u8 clear_stencil = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] & 0xff;
		getCurrentResourceStorage().m_commandList->ClearDepthStencilView(m_rtts.m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_STENCIL, 0.f, clear_stencil, 0, nullptr);
	}

	if (arg & 0xF0)
	{
		u32 clear_color = rsx::method_registers[NV4097_SET_COLOR_CLEAR_VALUE];
		u8 clear_a = clear_color >> 24;
		u8 clear_r = clear_color >> 16;
		u8 clear_g = clear_color >> 8;
		u8 clear_b = clear_color;
		float clearColor[] =
		{
			clear_r / 255.0f,
			clear_g / 255.0f,
			clear_b / 255.0f,
			clear_a / 255.0f
		};

		size_t g_RTTIncrement = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		switch (u32 color_target = rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET])
		{
			case CELL_GCM_SURFACE_TARGET_NONE: break;

			case CELL_GCM_SURFACE_TARGET_0:
			case CELL_GCM_SURFACE_TARGET_1:
				getCurrentResourceStorage().m_commandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtts.m_renderTargetsDescriptorsHeap->GetCPUDescriptorHandleForHeapStart()), clearColor, 0, nullptr);
				break;
			case CELL_GCM_SURFACE_TARGET_MRT1:
				getCurrentResourceStorage().m_commandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtts.m_renderTargetsDescriptorsHeap->GetCPUDescriptorHandleForHeapStart()), clearColor, 0, nullptr);
				getCurrentResourceStorage().m_commandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtts.m_renderTargetsDescriptorsHeap->GetCPUDescriptorHandleForHeapStart()).Offset(1, g_descriptorStrideRTV), clearColor, 0, nullptr);
				break;
			case CELL_GCM_SURFACE_TARGET_MRT2:
				getCurrentResourceStorage().m_commandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtts.m_renderTargetsDescriptorsHeap->GetCPUDescriptorHandleForHeapStart()), clearColor, 0, nullptr);
				getCurrentResourceStorage().m_commandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtts.m_renderTargetsDescriptorsHeap->GetCPUDescriptorHandleForHeapStart()).Offset(1, g_descriptorStrideRTV), clearColor, 0, nullptr);
				getCurrentResourceStorage().m_commandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtts.m_renderTargetsDescriptorsHeap->GetCPUDescriptorHandleForHeapStart()).Offset(2, g_descriptorStrideRTV), clearColor, 0, nullptr);
				break;
			case CELL_GCM_SURFACE_TARGET_MRT3:
				getCurrentResourceStorage().m_commandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtts.m_renderTargetsDescriptorsHeap->GetCPUDescriptorHandleForHeapStart()), clearColor, 0, nullptr);
				getCurrentResourceStorage().m_commandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtts.m_renderTargetsDescriptorsHeap->GetCPUDescriptorHandleForHeapStart()).Offset(1, g_descriptorStrideRTV), clearColor, 0, nullptr);
				getCurrentResourceStorage().m_commandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtts.m_renderTargetsDescriptorsHeap->GetCPUDescriptorHandleForHeapStart()).Offset(2, g_descriptorStrideRTV), clearColor, 0, nullptr);
				getCurrentResourceStorage().m_commandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtts.m_renderTargetsDescriptorsHeap->GetCPUDescriptorHandleForHeapStart()).Offset(3, g_descriptorStrideRTV), clearColor, 0, nullptr);
				break;
			default:
				LOG_ERROR(RSX, "Bad surface color target: %d", color_target);
		}
	}

	std::chrono::time_point<std::chrono::system_clock> endDuration = std::chrono::system_clock::now();
	m_timers.m_drawCallDuration += std::chrono::duration_cast<std::chrono::microseconds>(endDuration - startDuration).count();
	m_timers.m_drawCallCount++;

	if (Ini.GSDebugOutputEnable.GetValue())
	{
		ThrowIfFailed(getCurrentResourceStorage().m_commandList->Close());
		m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)getCurrentResourceStorage().m_commandList.GetAddressOf());
		getCurrentResourceStorage().setNewCommandList();
	}
}

void D3D12GSRender::end()
{
	std::chrono::time_point<std::chrono::system_clock> startDuration = std::chrono::system_clock::now();

	std::chrono::time_point<std::chrono::system_clock> rttDurationStart = std::chrono::system_clock::now();
	PrepareRenderTargets(getCurrentResourceStorage().m_commandList.Get());

	std::chrono::time_point<std::chrono::system_clock> rttDurationEnd = std::chrono::system_clock::now();
	m_timers.m_rttDuration += std::chrono::duration_cast<std::chrono::microseconds>(rttDurationEnd - rttDurationStart).count();

	std::chrono::time_point<std::chrono::system_clock> vertexIndexDurationStart = std::chrono::system_clock::now();

	if (!vertex_index_array.empty() || vertex_draw_count)
	{
		const std::vector<D3D12_VERTEX_BUFFER_VIEW> &vertexBufferViews = UploadVertexBuffers(!vertex_index_array.empty());
		const D3D12_INDEX_BUFFER_VIEW &indexBufferView = uploadIndexBuffers(!vertex_index_array.empty());
		getCurrentResourceStorage().m_commandList->IASetVertexBuffers(0, (UINT)vertexBufferViews.size(), vertexBufferViews.data());
		if (m_renderingInfo.m_indexed)
			getCurrentResourceStorage().m_commandList->IASetIndexBuffer(&indexBufferView);
	}

	std::chrono::time_point<std::chrono::system_clock> vertexIndexDurationEnd = std::chrono::system_clock::now();
	m_timers.m_vertexIndexDuration += std::chrono::duration_cast<std::chrono::microseconds>(vertexIndexDurationEnd - vertexIndexDurationStart).count();

	std::chrono::time_point<std::chrono::system_clock> programLoadStart = std::chrono::system_clock::now();
	if (!LoadProgram())
	{
		LOG_ERROR(RSX, "LoadProgram failed.");
		Emu.Pause();
		return;
	}
	std::chrono::time_point<std::chrono::system_clock> programLoadEnd = std::chrono::system_clock::now();
	m_timers.m_programLoadDuration += std::chrono::duration_cast<std::chrono::microseconds>(programLoadEnd - programLoadStart).count();

	getCurrentResourceStorage().m_commandList->SetGraphicsRootSignature(m_rootSignatures[m_PSO->second].Get());
	getCurrentResourceStorage().m_commandList->OMSetStencilRef(rsx::method_registers[NV4097_SET_STENCIL_FUNC_REF]);

	std::chrono::time_point<std::chrono::system_clock> constantsDurationStart = std::chrono::system_clock::now();

	size_t currentDescriptorIndex = getCurrentResourceStorage().m_descriptorsHeapIndex;
	// Constants
	setScaleOffset(currentDescriptorIndex);
	FillVertexShaderConstantsBuffer(currentDescriptorIndex + 1);
	FillPixelShaderConstantsBuffer(currentDescriptorIndex + 2);

	std::chrono::time_point<std::chrono::system_clock> constantsDurationEnd = std::chrono::system_clock::now();
	m_timers.m_constantsDuration += std::chrono::duration_cast<std::chrono::microseconds>(constantsDurationEnd - constantsDurationStart).count();

	getCurrentResourceStorage().m_commandList->SetPipelineState(m_PSO->first);

	std::chrono::time_point<std::chrono::system_clock> textureDurationStart = std::chrono::system_clock::now();
	if (m_PSO->second > 0)
	{
		size_t usedTexture = UploadTextures(getCurrentResourceStorage().m_commandList.Get(), currentDescriptorIndex + 3);

		// Fill empty slots
		for (; usedTexture < m_PSO->second; usedTexture++)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0);
			m_device->CreateShaderResourceView(m_dummyTexture, &srvDesc,
				CD3DX12_CPU_DESCRIPTOR_HANDLE(getCurrentResourceStorage().m_descriptorsHeap->GetCPUDescriptorHandleForHeapStart())
				.Offset((INT)currentDescriptorIndex + 3 + (INT)usedTexture, g_descriptorStrideSRVCBVUAV)
				);

			D3D12_SAMPLER_DESC samplerDesc = {};
			samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			m_device->CreateSampler(&samplerDesc,
				CD3DX12_CPU_DESCRIPTOR_HANDLE(getCurrentResourceStorage().m_samplerDescriptorHeap[getCurrentResourceStorage().m_samplerDescriptorHeapIndex]->GetCPUDescriptorHandleForHeapStart())
				.Offset((INT)getCurrentResourceStorage().m_currentSamplerIndex + (INT)usedTexture, g_descriptorStrideSamplers)
				);
		}

		ID3D12DescriptorHeap *descriptors[] =
		{
			getCurrentResourceStorage().m_descriptorsHeap.Get(),
			getCurrentResourceStorage().m_samplerDescriptorHeap[getCurrentResourceStorage().m_samplerDescriptorHeapIndex].Get(),
		};
		getCurrentResourceStorage().m_commandList->SetDescriptorHeaps(2, descriptors);

		getCurrentResourceStorage().m_commandList->SetGraphicsRootDescriptorTable(0,
			CD3DX12_GPU_DESCRIPTOR_HANDLE(getCurrentResourceStorage().m_descriptorsHeap->GetGPUDescriptorHandleForHeapStart())
			.Offset((INT)currentDescriptorIndex, g_descriptorStrideSRVCBVUAV)
			);
		getCurrentResourceStorage().m_commandList->SetGraphicsRootDescriptorTable(1,
			CD3DX12_GPU_DESCRIPTOR_HANDLE(getCurrentResourceStorage().m_samplerDescriptorHeap[getCurrentResourceStorage().m_samplerDescriptorHeapIndex]->GetGPUDescriptorHandleForHeapStart())
			.Offset((INT)getCurrentResourceStorage().m_currentSamplerIndex, g_descriptorStrideSamplers)
			);

		getCurrentResourceStorage().m_currentSamplerIndex += usedTexture;
		getCurrentResourceStorage().m_descriptorsHeapIndex += usedTexture + 3;
	}
	else
	{
		getCurrentResourceStorage().m_commandList->SetDescriptorHeaps(1, getCurrentResourceStorage().m_descriptorsHeap.GetAddressOf());
		getCurrentResourceStorage().m_commandList->SetGraphicsRootDescriptorTable(0,
			CD3DX12_GPU_DESCRIPTOR_HANDLE(getCurrentResourceStorage().m_descriptorsHeap->GetGPUDescriptorHandleForHeapStart())
			.Offset((INT)currentDescriptorIndex, g_descriptorStrideSRVCBVUAV)
			);
		getCurrentResourceStorage().m_descriptorsHeapIndex += 3;
	}

	std::chrono::time_point<std::chrono::system_clock> textureDurationEnd = std::chrono::system_clock::now();
	m_timers.m_textureDuration += std::chrono::duration_cast<std::chrono::microseconds>(textureDurationEnd - textureDurationStart).count();

	size_t numRTT;
	switch (u32 color_target = rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET])
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
		LOG_ERROR(RSX, "Bad surface color target: %d", color_target);
	}

	getCurrentResourceStorage().m_commandList->OMSetRenderTargets((UINT)numRTT, &m_rtts.m_renderTargetsDescriptorsHeap->GetCPUDescriptorHandleForHeapStart(), true,
		&CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtts.m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart()));

	int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
	int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;

	D3D12_VIEWPORT viewport =
	{
		0.f,
		0.f,
		(float)clip_w,
		(float)clip_h,
		-1.f,
		1.f
	};
	getCurrentResourceStorage().m_commandList->RSSetViewports(1, &viewport);

	D3D12_RECT box =
	{
		0,
		0,
		(LONG)clip_w,
		(LONG)clip_h,
	};
	getCurrentResourceStorage().m_commandList->RSSetScissorRects(1, &box);

	switch (draw_mode - 1)
	{
	case GL_POINTS:
		getCurrentResourceStorage().m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		break;
	case GL_LINES:
		getCurrentResourceStorage().m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		break;
	case GL_LINE_LOOP:
		getCurrentResourceStorage().m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ);
		break;
	case GL_LINE_STRIP:
		getCurrentResourceStorage().m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
		break;
	case GL_TRIANGLES:
		getCurrentResourceStorage().m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		break;
	case GL_TRIANGLE_STRIP:
		getCurrentResourceStorage().m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		break;
	case GL_TRIANGLE_FAN:
	case GL_QUADS:
		getCurrentResourceStorage().m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		break;
	case GL_QUAD_STRIP:
	case GL_POLYGON:
	default:
		getCurrentResourceStorage().m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		LOG_ERROR(RSX, "Unsupported primitive type");
		break;
	}

	if (m_renderingInfo.m_indexed)
		getCurrentResourceStorage().m_commandList->DrawIndexedInstanced((UINT)m_renderingInfo.m_count, 1, 0, (UINT)m_renderingInfo.m_baseVertex, 0);
	else
		getCurrentResourceStorage().m_commandList->DrawInstanced((UINT)m_renderingInfo.m_count, 1, (UINT)m_renderingInfo.m_baseVertex, 0);

	vertex_index_array.clear();
	std::chrono::time_point<std::chrono::system_clock> endDuration = std::chrono::system_clock::now();
	m_timers.m_drawCallDuration += std::chrono::duration_cast<std::chrono::microseconds>(endDuration - startDuration).count();
	m_timers.m_drawCallCount++;

	if (Ini.GSDebugOutputEnable.GetValue())
	{
		ThrowIfFailed(getCurrentResourceStorage().m_commandList->Close());
		m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)getCurrentResourceStorage().m_commandList.GetAddressOf());
		getCurrentResourceStorage().setNewCommandList();
	}

	thread::end();
}

static bool
isFlipSurfaceInLocalMemory(u32 surfaceColorTarget)
{
	switch (surfaceColorTarget)
	{
	case CELL_GCM_SURFACE_TARGET_0:
	case CELL_GCM_SURFACE_TARGET_1:
	case CELL_GCM_SURFACE_TARGET_MRT1:
	case CELL_GCM_SURFACE_TARGET_MRT2:
	case CELL_GCM_SURFACE_TARGET_MRT3:
		return true;
	case CELL_GCM_SURFACE_TARGET_NONE:
	default:
		return false;
	}
}

void D3D12GSRender::flip(int buffer)
{
	ID3D12Resource *resourceToFlip;
	float viewport_w, viewport_h;

	if (!isFlipSurfaceInLocalMemory(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]))
	{
		ResourceStorage &storage = getCurrentResourceStorage();
		assert(storage.m_RAMFramebuffer == nullptr);

		size_t w = 0, h = 0, rowPitch = 0;

		size_t offset = 0;
		if (false)
		{
			CellGcmDisplayInfo* buffers;// = vm::ps3::_ptr<CellGcmDisplayInfo>(m_gcm_buffers_addr);
			u32 addr = rsx::get_address(gcm_buffers[gcm_current_buffer].offset, CELL_GCM_LOCATION_LOCAL);
			w = gcm_buffers[gcm_current_buffer].width;
			h = gcm_buffers[gcm_current_buffer].height;
			u8 *src_buffer = vm::ps3::_ptr<u8>(addr);

			rowPitch = align(w * 4, 256);
			size_t textureSize = rowPitch * h; // * 4 for mipmap levels
			assert(m_textureUploadData.canAlloc(textureSize));
			size_t heapOffset = m_textureUploadData.alloc(textureSize);

			void *buffer;
			ThrowIfFailed(m_textureUploadData.m_heap->Map(0, &CD3DX12_RANGE(heapOffset, heapOffset + textureSize), &buffer));
			void *dstBuffer = (char*)buffer + heapOffset;
			for (unsigned row = 0; row < h; row++)
				memcpy((char*)dstBuffer + row * rowPitch, (char*)src_buffer + row * w * 4, w * 4);
			m_textureUploadData.m_heap->Unmap(0, &CD3DX12_RANGE(heapOffset, heapOffset + textureSize));
			offset = heapOffset;
		}

		ThrowIfFailed(
			m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)w, (UINT)h, 1, 1),
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(storage.m_RAMFramebuffer.GetAddressOf())
				)
			);
		getCurrentResourceStorage().m_commandList->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(storage.m_RAMFramebuffer.Get(), 0), 0, 0, 0,
			&CD3DX12_TEXTURE_COPY_LOCATION(m_textureUploadData.m_heap, { offset, { DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)w, (UINT)h, 1, (UINT)rowPitch} }), nullptr);

		getCurrentResourceStorage().m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(storage.m_RAMFramebuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
		resourceToFlip = storage.m_RAMFramebuffer.Get();
		viewport_w = (float)w, viewport_h = (float)h;
	}
	else
	{
		if (m_rtts.m_currentlyBoundRenderTargets[0] != nullptr)
			getCurrentResourceStorage().m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rtts.m_currentlyBoundRenderTargets[0], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
		resourceToFlip = m_rtts.m_currentlyBoundRenderTargets[0];
	}

	getCurrentResourceStorage().m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	D3D12_VIEWPORT viewport =
	{
		0.f,
		0.f,
		(float)m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()]->GetDesc().Width,
		(float)m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()]->GetDesc().Height,
		0.f,
		1.f
	};
	getCurrentResourceStorage().m_commandList->RSSetViewports(1, &viewport);

	D3D12_RECT box =
	{
		0,
		0,
		(LONG)m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()]->GetDesc().Width,
		(LONG)m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()]->GetDesc().Height,
	};
	getCurrentResourceStorage().m_commandList->RSSetScissorRects(1, &box);
	getCurrentResourceStorage().m_commandList->SetGraphicsRootSignature(m_outputScalingPass.m_rootSignature);
	getCurrentResourceStorage().m_commandList->SetPipelineState(m_outputScalingPass.m_PSO);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	// FIXME: Not always true
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	if (isFlipSurfaceInLocalMemory(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]))
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	else
		srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3,
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0
			);
	m_device->CreateShaderResourceView(resourceToFlip, &srvDesc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(m_outputScalingPass.m_textureDescriptorHeap->GetCPUDescriptorHandleForHeapStart()).Offset(m_swapChain->GetCurrentBackBufferIndex(), g_descriptorStrideSRVCBVUAV));

	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	m_device->CreateSampler(&samplerDesc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(m_outputScalingPass.m_samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart()).Offset(m_swapChain->GetCurrentBackBufferIndex(), g_descriptorStrideSamplers));

	getCurrentResourceStorage().m_commandList->SetDescriptorHeaps(1, &m_outputScalingPass.m_textureDescriptorHeap);
	getCurrentResourceStorage().m_commandList->SetGraphicsRootDescriptorTable(0,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(m_outputScalingPass.m_textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart()).Offset(m_swapChain->GetCurrentBackBufferIndex(), g_descriptorStrideSRVCBVUAV));
	getCurrentResourceStorage().m_commandList->SetDescriptorHeaps(1, &m_outputScalingPass.m_samplerDescriptorHeap);
	getCurrentResourceStorage().m_commandList->SetGraphicsRootDescriptorTable(1, 
		CD3DX12_GPU_DESCRIPTOR_HANDLE(m_outputScalingPass.m_samplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart()).Offset(m_swapChain->GetCurrentBackBufferIndex(), g_descriptorStrideSamplers));

	getCurrentResourceStorage().m_commandList->OMSetRenderTargets(1,
		&CD3DX12_CPU_DESCRIPTOR_HANDLE(m_backbufferAsRendertarget[m_swapChain->GetCurrentBackBufferIndex()]->GetCPUDescriptorHandleForHeapStart()),
		true, nullptr);
	D3D12_VERTEX_BUFFER_VIEW vbv = {};
	vbv.BufferLocation = m_outputScalingPass.m_vertexBuffer->GetGPUVirtualAddress();
	vbv.StrideInBytes = 4 * sizeof(float);
	vbv.SizeInBytes = 16 * sizeof(float);
	getCurrentResourceStorage().m_commandList->IASetVertexBuffers(0, 1, &vbv);
	getCurrentResourceStorage().m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	if (m_rtts.m_currentlyBoundRenderTargets[0] != nullptr)
		getCurrentResourceStorage().m_commandList->DrawInstanced(4, 1, 0, 0);

	if (!Ini.GSOverlay.GetValue())
		getCurrentResourceStorage().m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	if (isFlipSurfaceInLocalMemory(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]) && m_rtts.m_currentlyBoundRenderTargets[0] != nullptr)
		getCurrentResourceStorage().m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rtts.m_currentlyBoundRenderTargets[0], D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	ThrowIfFailed(getCurrentResourceStorage().m_commandList->Close());
	m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)getCurrentResourceStorage().m_commandList.GetAddressOf());

	if(Ini.GSOverlay.GetValue())
		renderOverlay();

	ResetTimer();

	std::chrono::time_point<std::chrono::system_clock> flipStart = std::chrono::system_clock::now();

	ThrowIfFailed(m_swapChain->Present(Ini.GSVSyncEnable.GetValue() ? 1 : 0, 0));
	// Add an event signaling queue completion

	ResourceStorage &storage = getNonCurrentResourceStorage();

	m_commandQueueGraphic->Signal(storage.m_frameFinishedFence.Get(), storage.m_fenceValue);
	storage.m_frameFinishedFence->SetEventOnCompletion(storage.m_fenceValue, storage.m_frameFinishedHandle);
	storage.m_fenceValue++;

	storage.m_inUse = true;

	// Get the put pos - 1. This way after cleaning we can set the get ptr to
	// this value, allowing heap to proceed even if we cleant before allocating
	// a new value (that's the reason of the -1)
	storage.m_getPosConstantsHeap = m_constantsData.getCurrentPutPosMinusOne();
	storage.m_getPosVertexIndexHeap = m_vertexIndexData.getCurrentPutPosMinusOne();
	storage.m_getPosTextureUploadHeap = m_textureUploadData.getCurrentPutPosMinusOne();
	storage.m_getPosReadbackHeap = m_readbackResources.getCurrentPutPosMinusOne();
	storage.m_getPosUAVHeap = m_UAVHeap.getCurrentPutPosMinusOne();

	// Flush
	local_transform_constants.clear();
	m_texturesRTTs.clear();

	// Now get ready for next frame
	ResourceStorage &newStorage = getCurrentResourceStorage();

	newStorage.WaitAndClean();
	if (newStorage.m_inUse)
	{
		m_constantsData.m_getPos = newStorage.m_getPosConstantsHeap;
		m_vertexIndexData.m_getPos = newStorage.m_getPosVertexIndexHeap;
		m_textureUploadData.m_getPos = newStorage.m_getPosTextureUploadHeap;
		m_readbackResources.m_getPos = newStorage.m_getPosReadbackHeap;
		m_UAVHeap.m_getPos = newStorage.m_getPosUAVHeap;
	}

	m_frame->flip(nullptr);


	std::chrono::time_point<std::chrono::system_clock> flipEnd = std::chrono::system_clock::now();
	m_timers.m_flipDuration += std::chrono::duration_cast<std::chrono::microseconds>(flipEnd - flipStart).count();
}

void D3D12GSRender::ResetTimer()
{
	m_timers.m_drawCallCount = 0;
	m_timers.m_drawCallDuration = 0;
	m_timers.m_rttDuration = 0;
	m_timers.m_vertexIndexDuration = 0;
	m_timers.m_bufferUploadSize = 0;
	m_timers.m_programLoadDuration = 0;
	m_timers.m_constantsDuration = 0;
	m_timers.m_textureDuration = 0;
	m_timers.m_flipDuration = 0;
}

D3D12GSRender::ResourceStorage& D3D12GSRender::getCurrentResourceStorage()
{
	return m_perFrameStorage[m_swapChain->GetCurrentBackBufferIndex()];
}

D3D12GSRender::ResourceStorage& D3D12GSRender::getNonCurrentResourceStorage()
{
	return m_perFrameStorage[1 - m_swapChain->GetCurrentBackBufferIndex()];
}

ID3D12Resource * D3D12GSRender::writeColorBuffer(ID3D12Resource * RTT, ID3D12GraphicsCommandList * cmdlist)
{
	int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
	int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;
	ID3D12Resource *Result;
	size_t w = clip_w, h = clip_h;
	DXGI_FORMAT dxgiFormat;
	size_t rowPitch;
	switch (m_surface.color_format)
	{
	case CELL_GCM_SURFACE_A8R8G8B8:
		dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		rowPitch = align(w * 4, 256);
		break;
	case CELL_GCM_SURFACE_F_W16Z16Y16X16:
		dxgiFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		rowPitch = align(w * 8, 256);
		break;
	}

	size_t sizeInByte = rowPitch * h;
	assert(m_readbackResources.canAlloc(sizeInByte));
	size_t heapOffset = m_readbackResources.alloc(sizeInByte);

	ThrowIfFailed(
		m_device->CreatePlacedResource(
			m_readbackResources.m_heap,
			heapOffset,
			&CD3DX12_RESOURCE_DESC::Buffer(rowPitch * h),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&Result)
			)
		);
	getCurrentResourceStorage().m_singleFrameLifetimeResources.push_back(Result);

	cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(RTT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

	cmdlist->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(Result, { 0, {dxgiFormat, (UINT)h, (UINT)w, 1, (UINT)rowPitch } }), 0, 0, 0,
		&CD3DX12_TEXTURE_COPY_LOCATION(RTT, 0), nullptr);
	cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(RTT, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
	return Result;
}

static
void copyToCellRamAndRelease(void *dstAddress, ID3D12Resource *res, size_t dstPitch, size_t srcPitch, size_t width, size_t height)
{
	void *srcBuffer;
	ThrowIfFailed(res->Map(0, nullptr, &srcBuffer));
	for (unsigned row = 0; row < height; row++)
		memcpy((char*)dstAddress + row * dstPitch, (char*)srcBuffer + row * srcPitch, srcPitch);
	res->Unmap(0, nullptr);
	res->Release();
}

void D3D12GSRender::semaphore_PGRAPH_texture_read_release()
{
	semaphore_PGRAPH_backend_release();
}

void D3D12GSRender::semaphore_PGRAPH_backend_release()
{
	// Add all buffer write
	// Cell can't make any assumption about readyness of color/depth buffer
	// Except when a semaphore is written by RSX
	int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
	int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;

	ComPtr<ID3D12Fence> fence;
	ThrowIfFailed(
		m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()))
		);
	HANDLE handle = CreateEvent(0, FALSE, FALSE, 0);
	fence->SetEventOnCompletion(1, handle);

	ComPtr<ID3D12Resource> writeDest, depthConverted;
	ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	size_t depthRowPitch = clip_w;
	depthRowPitch = (depthRowPitch + 255) & ~255;

	u32 m_context_dma_color_a = rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_A];
	u32 m_context_dma_color_b = rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_B];
	u32 m_context_dma_color_c = rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_C];
	u32 m_context_dma_color_d = rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_D];
	u32 m_context_dma_z = rsx::method_registers[NV4097_SET_CONTEXT_DMA_ZETA];

	bool needTransfer = (m_context_dma_z && rpcs3::state.config.rsx.opengl.write_depth_buffer) ||
		((m_context_dma_color_a || m_context_dma_color_b || m_context_dma_color_c || m_context_dma_color_d) && rpcs3::state.config.rsx.opengl.write_color_buffers);

	if (m_context_dma_z && rpcs3::state.config.rsx.opengl.write_depth_buffer)
	{
		size_t sizeInByte = clip_w * clip_h * 2;
		assert(m_UAVHeap.canAlloc(sizeInByte));
		size_t heapOffset = m_UAVHeap.alloc(sizeInByte);

		ThrowIfFailed(
			m_device->CreatePlacedResource(
				m_UAVHeap.m_heap,
				heapOffset,
				&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UNORM, clip_w, clip_h, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				nullptr,
				IID_PPV_ARGS(depthConverted.GetAddressOf())
				)
			);

		sizeInByte = depthRowPitch * clip_h;
		assert(m_readbackResources.canAlloc(sizeInByte));
		heapOffset = m_readbackResources.alloc(sizeInByte);

		ThrowIfFailed(
			m_device->CreatePlacedResource(
				m_readbackResources.m_heap,
				heapOffset,
				&CD3DX12_RESOURCE_DESC::Buffer(sizeInByte),
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(writeDest.GetAddressOf())
				)
			);

		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV , 2, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };
		ThrowIfFailed(
			m_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(descriptorHeap.GetAddressOf()))
			);
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		switch (m_surface.depth_format)
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
			LOG_ERROR(RSX, "Bad depth format! (%d)", m_surface.depth_format);
			assert(0);
		}
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		m_device->CreateShaderResourceView(m_rtts.m_currentlyBoundDepthStencil, &srvDesc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptorHeap->GetCPUDescriptorHandleForHeapStart()));
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R8_UNORM;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		m_device->CreateUnorderedAccessView(depthConverted.Get(), nullptr, &uavDesc, 
			CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptorHeap->GetCPUDescriptorHandleForHeapStart()).Offset(1, g_descriptorStrideSRVCBVUAV));

		// Convert
		getCurrentResourceStorage().m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rtts.m_currentlyBoundDepthStencil, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));

		getCurrentResourceStorage().m_commandList->SetPipelineState(m_convertPSO);
		getCurrentResourceStorage().m_commandList->SetComputeRootSignature(m_convertRootSignature);
		getCurrentResourceStorage().m_commandList->SetDescriptorHeaps(1, descriptorHeap.GetAddressOf());
		getCurrentResourceStorage().m_commandList->SetComputeRootDescriptorTable(0, descriptorHeap->GetGPUDescriptorHandleForHeapStart());
		getCurrentResourceStorage().m_commandList->Dispatch(clip_w / 8, clip_h / 8, 1);

		D3D12_RESOURCE_BARRIER barriers[] =
		{
			CD3DX12_RESOURCE_BARRIER::Transition(m_rtts.m_currentlyBoundDepthStencil, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE),
			CD3DX12_RESOURCE_BARRIER::UAV(depthConverted.Get()),
		};
		getCurrentResourceStorage().m_commandList->ResourceBarrier(2, barriers);
		getCurrentResourceStorage().m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(depthConverted.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
		getCurrentResourceStorage().m_commandList->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(writeDest.Get(), { 0, { DXGI_FORMAT_R8_UNORM, (UINT)clip_w, (UINT)clip_h, 1, (UINT)depthRowPitch } }), 0, 0, 0,
			&CD3DX12_TEXTURE_COPY_LOCATION(depthConverted.Get(), 0), nullptr);

		invalidateAddress(rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_ZETA_OFFSET], m_context_dma_z - 0xfeed0000));
	}

	ID3D12Resource *rtt0, *rtt1, *rtt2, *rtt3;
	if (rpcs3::state.config.rsx.opengl.write_color_buffers)
	{
		switch (rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET])
		{
		case CELL_GCM_SURFACE_TARGET_NONE:
			break;

		case CELL_GCM_SURFACE_TARGET_0:
			if (m_context_dma_color_a) rtt0 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[0], getCurrentResourceStorage().m_commandList.Get());
			break;

		case CELL_GCM_SURFACE_TARGET_1:
			if (m_context_dma_color_b) rtt1 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[0], getCurrentResourceStorage().m_commandList.Get());
			break;

		case CELL_GCM_SURFACE_TARGET_MRT1:
			if (m_context_dma_color_a) rtt0 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[0], getCurrentResourceStorage().m_commandList.Get());
			if (m_context_dma_color_b) rtt1 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[1], getCurrentResourceStorage().m_commandList.Get());
			break;

		case CELL_GCM_SURFACE_TARGET_MRT2:
			if (m_context_dma_color_a) rtt0 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[0], getCurrentResourceStorage().m_commandList.Get());
			if (m_context_dma_color_b) rtt1 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[1], getCurrentResourceStorage().m_commandList.Get());
			if (m_context_dma_color_c) rtt2 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[2], getCurrentResourceStorage().m_commandList.Get());
			break;

		case CELL_GCM_SURFACE_TARGET_MRT3:
			if (m_context_dma_color_a) rtt0 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[0], getCurrentResourceStorage().m_commandList.Get());
			if (m_context_dma_color_b) rtt1 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[1], getCurrentResourceStorage().m_commandList.Get());
			if (m_context_dma_color_c) rtt2 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[2], getCurrentResourceStorage().m_commandList.Get());
			if (m_context_dma_color_d) rtt3 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[3], getCurrentResourceStorage().m_commandList.Get());
			break;
		}

		if (m_context_dma_color_a) invalidateAddress(rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_COLOR_AOFFSET], m_context_dma_color_a - 0xfeed0000));
		if (m_context_dma_color_b) invalidateAddress(rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_COLOR_BOFFSET], m_context_dma_color_b - 0xfeed0000));
		if (m_context_dma_color_c) invalidateAddress(rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_COLOR_COFFSET], m_context_dma_color_c - 0xfeed0000));
		if (m_context_dma_color_d) invalidateAddress(rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_COLOR_DOFFSET], m_context_dma_color_d - 0xfeed0000));
	}
	if (needTransfer)
	{
		ThrowIfFailed(getCurrentResourceStorage().m_commandList->Close());
		m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)getCurrentResourceStorage().m_commandList.GetAddressOf());
		getCurrentResourceStorage().setNewCommandList();
	}

	//Wait for result
	m_commandQueueGraphic->Signal(fence.Get(), 1);
	WaitForSingleObject(handle, INFINITE);
	CloseHandle(handle);

	if (m_context_dma_z && rpcs3::state.config.rsx.opengl.write_depth_buffer)
	{
		u32 address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_ZETA_OFFSET], m_context_dma_z - 0xfeed0000);
		auto ptr = vm::base(address);
		char *ptrAsChar = (char*)ptr;
		unsigned char *writeDestPtr;
		ThrowIfFailed(writeDest->Map(0, nullptr, (void**)&writeDestPtr));

		for (unsigned row = 0; row < (unsigned)clip_h; row++)
		{
			for (unsigned i = 0; i < (unsigned)clip_w; i++)
			{
				unsigned char c = writeDestPtr[row * depthRowPitch + i];
				ptrAsChar[4 * (row * clip_w + i)] = c;
				ptrAsChar[4 * (row * clip_w + i) + 1] = c;
				ptrAsChar[4 * (row * clip_w + i) + 2] = c;
				ptrAsChar[4 * (row * clip_w + i) + 3] = c;
			}
		}
	}

	size_t srcPitch, dstPitch;
	switch (m_surface.color_format)
	{
	case CELL_GCM_SURFACE_A8R8G8B8:
		srcPitch = align(clip_w * 4, 256);
		dstPitch = clip_w * 4;
		break;
	case CELL_GCM_SURFACE_F_W16Z16Y16X16:
		srcPitch = align(clip_w * 8, 256);
		dstPitch = clip_w * 8;
		break;
	}

	if (rpcs3::state.config.rsx.opengl.write_color_buffers)
	{
		switch (rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET])
		{
		case CELL_GCM_SURFACE_TARGET_NONE:
			break;
		case CELL_GCM_SURFACE_TARGET_0:
		{
			u32 address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_COLOR_AOFFSET], m_context_dma_color_a - 0xfeed0000);
			copyToCellRamAndRelease(vm::base(address), rtt0, srcPitch, dstPitch, clip_w, clip_h);
		}
		break;
		case CELL_GCM_SURFACE_TARGET_1:
		{
			u32 address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_COLOR_BOFFSET], m_context_dma_color_b - 0xfeed0000);
			copyToCellRamAndRelease(vm::base(address), rtt1, srcPitch, dstPitch, clip_w, clip_h);
		}
		break;
		case CELL_GCM_SURFACE_TARGET_MRT1:
		{
			u32 address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_COLOR_AOFFSET], m_context_dma_color_a - 0xfeed0000);
			copyToCellRamAndRelease(vm::base(address), rtt0, srcPitch, dstPitch, clip_w, clip_h);
			address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_COLOR_BOFFSET], m_context_dma_color_b - 0xfeed0000);
			copyToCellRamAndRelease(vm::base(address), rtt1, srcPitch, dstPitch, clip_w, clip_h);
		}
		break;
		case CELL_GCM_SURFACE_TARGET_MRT2:
		{
			u32 address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_COLOR_AOFFSET], m_context_dma_color_a - 0xfeed0000);
			copyToCellRamAndRelease(vm::base(address), rtt0, srcPitch, dstPitch, clip_w, clip_h);
			address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_COLOR_BOFFSET], m_context_dma_color_b - 0xfeed0000);
			copyToCellRamAndRelease(vm::base(address), rtt1, srcPitch, dstPitch, clip_w, clip_h);
			address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_COLOR_COFFSET], m_context_dma_color_c - 0xfeed0000);
			copyToCellRamAndRelease(vm::base(address), rtt2, srcPitch, dstPitch, clip_w, clip_h);
		}
		break;
		case CELL_GCM_SURFACE_TARGET_MRT3:
		{
			u32 address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_COLOR_AOFFSET], m_context_dma_color_a - 0xfeed0000);
			copyToCellRamAndRelease(vm::base(address), rtt0, srcPitch, dstPitch, clip_w, clip_h);
			address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_COLOR_BOFFSET], m_context_dma_color_b - 0xfeed0000);
			copyToCellRamAndRelease(vm::base(address), rtt1, srcPitch, dstPitch, clip_w, clip_h);
			address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_COLOR_COFFSET], m_context_dma_color_c - 0xfeed0000);
			copyToCellRamAndRelease(vm::base(address), rtt2, srcPitch, dstPitch, clip_w, clip_h);
			address = rsx::get_address(rsx::method_registers[NV4097_SET_SURFACE_COLOR_DOFFSET], m_context_dma_color_d - 0xfeed0000);
			copyToCellRamAndRelease(vm::base(address), rtt3, srcPitch, dstPitch, clip_w, clip_h);
		}
		break;
		}
	}
}
#endif
