#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "D3D12GSRender.h"
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <thread>
#include <chrono>

PFN_D3D12_CREATE_DEVICE wrapD3D12CreateDevice;
PFN_D3D12_GET_DEBUG_INTERFACE wrapD3D12GetDebugInterface;
PFN_D3D12_SERIALIZE_ROOT_SIGNATURE wrapD3D12SerializeRootSignature;

static HMODULE D3D12Module;

static void loadD3D12FunctionPointers()
{
	D3D12Module = LoadLibrary(L"d3d12.dll");
	wrapD3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(D3D12Module, "D3D12CreateDevice");
	wrapD3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(D3D12Module, "D3D12GetDebugInterface");
	wrapD3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(D3D12Module, "D3D12SerializeRootSignature");
}

static void unloadD3D12FunctionPointers()
{
	FreeLibrary(D3D12Module);
}

GetGSFrameCb2 GetGSFrame = nullptr;

void SetGetD3DGSFrameCallback(GetGSFrameCb2 value)
{
	GetGSFrame = value;
}

GarbageCollectionThread::GarbageCollectionThread()
{
	m_askForTermination = false;
	m_worker = std::thread([this]() {
		std::unique_lock<std::mutex> lock(m_mutex);
		while (!m_askForTermination)
		{
			if (!lock)
			{
				lock.lock();
				continue;
			}

			if (!m_queue.empty())
			{
				auto func = std::move(m_queue.front());

				m_queue.pop();

				if (lock) lock.unlock();

				func();

				continue;
			}
			cv.wait(lock);
		}
	});
}

GarbageCollectionThread::~GarbageCollectionThread()
{
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_askForTermination = true;
		cv.notify_one();
	}
	m_worker.join();
}

void GarbageCollectionThread::pushWork(std::function<void()>&& f)
{
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_queue.push(f);
	}
	cv.notify_one();
}

void GarbageCollectionThread::waitForCompletion()
{
	pushWork([]() {});
	while (true)
	{
		std::this_thread::yield();
		std::unique_lock<std::mutex> lock(m_mutex);
		if (m_queue.empty())
			return;
	}
}

void D3D12GSRender::ResourceStorage::Reset()
{
	m_constantsBufferIndex = 0;
	m_currentScaleOffsetBufferIndex = 0;
	m_currentTextureIndex = 0;
	m_currentSamplerIndex = 0;
	m_samplerDescriptorHeapIndex = 0;

	m_commandAllocator->Reset();
	m_nextAvailableCommandListIndex = 0;
	setNewCommandList();

	m_singleFrameLifetimeResources.clear();
}

void D3D12GSRender::ResourceStorage::setNewCommandList()
{
	if (m_availableCommandLists.size() > m_nextAvailableCommandListIndex)
	{
		m_currentCommandList = m_availableCommandLists[m_nextAvailableCommandListIndex].Get();
		m_currentCommandList->Reset(m_commandAllocator.Get(), nullptr);
	}
	else
	{
		ComPtr<ID3D12GraphicsCommandList> commandList;
		ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(commandList.GetAddressOf())));
		m_availableCommandLists.push_back(commandList);
		m_currentCommandList = m_availableCommandLists.back().Get();
	}
	m_nextAvailableCommandListIndex++;
}

void D3D12GSRender::ResourceStorage::Init(ID3D12Device *device)
{
	m_device = device;
	m_RAMFramebuffer = nullptr;
	// Create a global command allocator
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocator.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NumDescriptors = 10000; // For safety
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_constantsBufferDescriptorsHeap)));

	descriptorHeapDesc = {};
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NumDescriptors = 10000; // For safety
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_scaleOffsetDescriptorHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC textureDescriptorDesc = {};
	textureDescriptorDesc.NumDescriptors = 10000; // For safety
	textureDescriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	textureDescriptorDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&textureDescriptorDesc, IID_PPV_ARGS(&m_textureDescriptorsHeap)));

	textureDescriptorDesc.NumDescriptors = 2048; // For safety
	textureDescriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	ThrowIfFailed(device->CreateDescriptorHeap(&textureDescriptorDesc, IID_PPV_ARGS(&m_samplerDescriptorHeap[0])));
	ThrowIfFailed(device->CreateDescriptorHeap(&textureDescriptorDesc, IID_PPV_ARGS(&m_samplerDescriptorHeap[1])));

	m_frameFinishedHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	m_fenceValue = 0;
	ThrowIfFailed(device->CreateFence(m_fenceValue++, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_frameFinishedFence.GetAddressOf())));
	m_isUseable = true;
}

void D3D12GSRender::ResourceStorage::WaitAndClean(const std::vector<ID3D12Resource *> &dirtyTextures)
{
	WaitForSingleObjectEx(m_frameFinishedHandle, INFINITE, FALSE);

	Reset();

	for (auto tmp : dirtyTextures)
		tmp->Release();

	m_RAMFramebuffer = nullptr;
	m_isUseable.store(true, std::memory_order_release);
}

void D3D12GSRender::ResourceStorage::Release()
{
	// NOTE: Should be released only after gfx pipeline last command has been finished.
	m_availableCommandLists.clear();
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

bool D3D12GSRender::invalidateTexture(u32 addr)
{
	bool handled = false;
	auto It = m_protectedTextures.begin(), E = m_protectedTextures.end();
	for (; It != E;)
	{
		auto currentIt = It;
		++It;
		auto protectedTexture = *currentIt;
		u32 protectedRangeStart = std::get<1>(protectedTexture), protectedRangeSize = std::get<2>(protectedTexture);
		if (addr - protectedRangeStart < protectedRangeSize)
		{
			std::lock_guard<std::mutex> lock(mut);
			u32 texadrr = std::get<0>(protectedTexture);
			ID3D12Resource *texToErase = m_texturesCache[texadrr];
			m_texturesCache.erase(texadrr);
			m_texToClean.push_back(texToErase);

			vm::page_protect(protectedRangeStart, protectedRangeSize, 0, vm::page_writable, 0);
			m_protectedTextures.erase(currentIt);
			handled = true;
		}
	}
	return handled;
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
	: GSRender(), m_D3D12Lib(), m_PSO(nullptr)
{
	gfxHandler = [this](u32 addr) {
		bool result = invalidateTexture(addr);
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
	IDXGIAdapter* adaptater = nullptr;
	switch (Ini.GSD3DAdaptater.GetValue())
	{
	case 0: // WARP
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adaptater)));
		break;
	case 1: // Default
		dxgiFactory->EnumAdapters(0, &adaptater);
		break;
	default: // Adaptater 0, 1, ...
		dxgiFactory->EnumAdapters(Ini.GSD3DAdaptater.GetValue() - 2,&adaptater);
		break;
	}
	ThrowIfFailed(wrapD3D12CreateDevice(adaptater, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));

	// Queues
	D3D12_COMMAND_QUEUE_DESC copyQueueDesc = {}, graphicQueueDesc = {};
	copyQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	graphicQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(m_device->CreateCommandQueue(&graphicQueueDesc, IID_PPV_ARGS(m_commandQueueGraphic.GetAddressOf())));

	g_descriptorStrideSRVCBVUAV = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	g_descriptorStrideDSV = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	g_descriptorStrideRTV = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	g_descriptorStrideSamplers = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

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

	ThrowIfFailed(dxgiFactory->CreateSwapChain(m_commandQueueGraphic.Get(), &swapChain, (IDXGISwapChain**)m_swapChain.GetAddressOf()));
	m_swapChain->GetBuffer(0, IID_PPV_ARGS(&m_backBuffer[0]));
	m_swapChain->GetBuffer(1, IID_PPV_ARGS(&m_backBuffer[1]));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
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
		ThrowIfFailed(wrapD3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob));

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
	m_outputScalingPass.Init(m_device.Get());

	D3D12_HEAP_PROPERTIES hp = {};
	hp.Type = D3D12_HEAP_TYPE_DEFAULT;
	ThrowIfFailed(
		m_device->CreateCommittedResource(
			&hp,
			D3D12_HEAP_FLAG_NONE,
			&getTexture2DResourceDesc(2, 2, DXGI_FORMAT_R8G8B8A8_UNORM, 1),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_dummyTexture))
			);

	m_readbackResources.Init(m_device.Get(), 1024 * 1024 * 128, D3D12_HEAP_TYPE_READBACK, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
	m_UAVHeap.Init(m_device.Get(), 1024 * 1024 * 128, D3D12_HEAP_TYPE_DEFAULT, D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES);

	m_rtts.Init(m_device.Get());

	m_constantsData.Init(m_device.Get(), 1024 * 1024 * 64, D3D12_HEAP_TYPE_UPLOAD, D3D12_HEAP_FLAG_NONE);
	m_vertexIndexData.Init(m_device.Get(), 1024 * 1024 * 384, D3D12_HEAP_TYPE_UPLOAD, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
	m_textureUploadData.Init(m_device.Get(), 1024 * 1024 * 256, D3D12_HEAP_TYPE_UPLOAD, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
}

D3D12GSRender::~D3D12GSRender()
{
	while (!getCurrentResourceStorage().m_isUseable.load(std::memory_order_acquire) &&
		!getNonCurrentResourceStorage().m_isUseable.load(std::memory_order_acquire))
		std::this_thread::yield();

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
	for (auto &tmp : m_texToClean)
		tmp->Release();
	for (auto &tmp : m_texturesCache)
		tmp.second->Release();
	m_outputScalingPass.Release();
}

void D3D12GSRender::Close()
{
	if (joinable())
	{
		join();
	}

	if (m_frame->IsShown())
	{
		m_frame->Hide();
	}
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

void D3D12GSRender::Clear(u32 cmd)
{
	assert(cmd == NV4097_CLEAR_SURFACE);

	PrepareRenderTargets(getCurrentResourceStorage().m_currentCommandList);

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
	{
		u32 max_depth_value = m_surface_depth_format == CELL_GCM_SURFACE_Z16 ? 0x0000ffff : 0x00ffffff;
		getCurrentResourceStorage().m_currentCommandList->ClearDepthStencilView(m_rtts.m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, m_clear_surface_z / (float)max_depth_value, 0, 0, nullptr);
	}

	if (m_clear_surface_mask & 0x2)
		getCurrentResourceStorage().m_currentCommandList->ClearDepthStencilView(m_rtts.m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_STENCIL, 0.f, m_clear_surface_s, 0, nullptr);

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
				getCurrentResourceStorage().m_currentCommandList->ClearRenderTargetView(getCPUDescriptorHandle(m_rtts.m_renderTargetsDescriptorsHeap, 0), clearColor, 0, nullptr);
				break;
			case CELL_GCM_SURFACE_TARGET_MRT1:
				getCurrentResourceStorage().m_currentCommandList->ClearRenderTargetView(getCPUDescriptorHandle(m_rtts.m_renderTargetsDescriptorsHeap, 0), clearColor, 0, nullptr);
				getCurrentResourceStorage().m_currentCommandList->ClearRenderTargetView(getCPUDescriptorHandle(m_rtts.m_renderTargetsDescriptorsHeap, g_descriptorStrideRTV), clearColor, 0, nullptr);
				break;
			case CELL_GCM_SURFACE_TARGET_MRT2:
				getCurrentResourceStorage().m_currentCommandList->ClearRenderTargetView(getCPUDescriptorHandle(m_rtts.m_renderTargetsDescriptorsHeap, 0), clearColor, 0, nullptr);
				getCurrentResourceStorage().m_currentCommandList->ClearRenderTargetView(getCPUDescriptorHandle(m_rtts.m_renderTargetsDescriptorsHeap, g_descriptorStrideRTV), clearColor, 0, nullptr);
				handle.ptr += g_RTTIncrement;
				getCurrentResourceStorage().m_currentCommandList->ClearRenderTargetView(getCPUDescriptorHandle(m_rtts.m_renderTargetsDescriptorsHeap, 2 * g_descriptorStrideRTV), clearColor, 0, nullptr);
				break;
			case CELL_GCM_SURFACE_TARGET_MRT3:
				getCurrentResourceStorage().m_currentCommandList->ClearRenderTargetView(getCPUDescriptorHandle(m_rtts.m_renderTargetsDescriptorsHeap, 0), clearColor, 0, nullptr);
				handle.ptr += g_RTTIncrement;
				getCurrentResourceStorage().m_currentCommandList->ClearRenderTargetView(getCPUDescriptorHandle(m_rtts.m_renderTargetsDescriptorsHeap, g_descriptorStrideRTV), clearColor, 0, nullptr);
				handle.ptr += g_RTTIncrement;
				getCurrentResourceStorage().m_currentCommandList->ClearRenderTargetView(getCPUDescriptorHandle(m_rtts.m_renderTargetsDescriptorsHeap, 2 * g_descriptorStrideRTV), clearColor, 0, nullptr);
				handle.ptr += g_RTTIncrement;
				getCurrentResourceStorage().m_currentCommandList->ClearRenderTargetView(getCPUDescriptorHandle(m_rtts.m_renderTargetsDescriptorsHeap, 3 * g_descriptorStrideRTV), clearColor, 0, nullptr);
				break;
			default:
				LOG_ERROR(RSX, "Bad surface color target: %d", m_surface_color_target);
		}
	}
}

void D3D12GSRender::Draw()
{
	PrepareRenderTargets(getCurrentResourceStorage().m_currentCommandList);

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

	std::chrono::time_point<std::chrono::system_clock> startVertexTime = std::chrono::system_clock::now();
	if (m_indexed_array.m_count || m_draw_array_count)
	{
		const std::vector<D3D12_VERTEX_BUFFER_VIEW> &vertexBufferViews = UploadVertexBuffers(m_indexed_array.m_count ? true : false);
		const D3D12_INDEX_BUFFER_VIEW &indexBufferView = uploadIndexBuffers(m_indexed_array.m_count ? true : false);
		getCurrentResourceStorage().m_currentCommandList->IASetVertexBuffers(0, (UINT)vertexBufferViews.size(), vertexBufferViews.data());
		if (m_renderingInfo.m_indexed)
			getCurrentResourceStorage().m_currentCommandList->IASetIndexBuffer(&indexBufferView);
	}
	std::chrono::time_point<std::chrono::system_clock> endVertexTime = std::chrono::system_clock::now();
	m_timers.m_vertexUploadDuration += std::chrono::duration_cast<std::chrono::microseconds>(endVertexTime - startVertexTime).count();

	if (!LoadProgram())
	{
		LOG_ERROR(RSX, "LoadProgram failed.");
		Emu.Pause();
		return;
	}

	getCurrentResourceStorage().m_currentCommandList->SetGraphicsRootSignature(m_rootSignatures[m_PSO->second].Get());
	getCurrentResourceStorage().m_currentCommandList->OMSetStencilRef(m_stencil_func_ref);

	// Constants
	setScaleOffset();
	getCurrentResourceStorage().m_currentCommandList->SetDescriptorHeaps(1, getCurrentResourceStorage().m_scaleOffsetDescriptorHeap.GetAddressOf());
	getCurrentResourceStorage().m_currentCommandList->SetGraphicsRootDescriptorTable(0,
		getGPUDescriptorHandle(getCurrentResourceStorage().m_scaleOffsetDescriptorHeap.Get(),
			getCurrentResourceStorage().m_currentScaleOffsetBufferIndex * g_descriptorStrideSRVCBVUAV)
		);
	getCurrentResourceStorage().m_currentScaleOffsetBufferIndex++;

	size_t currentBufferIndex = getCurrentResourceStorage().m_constantsBufferIndex;
	FillVertexShaderConstantsBuffer();
	getCurrentResourceStorage().m_constantsBufferIndex++;
	FillPixelShaderConstantsBuffer();
	getCurrentResourceStorage().m_constantsBufferIndex++;

	getCurrentResourceStorage().m_currentCommandList->SetDescriptorHeaps(1, getCurrentResourceStorage().m_constantsBufferDescriptorsHeap.GetAddressOf());
	getCurrentResourceStorage().m_currentCommandList->SetGraphicsRootDescriptorTable(1,
		getGPUDescriptorHandle(getCurrentResourceStorage().m_constantsBufferDescriptorsHeap.Get(),
			currentBufferIndex * g_descriptorStrideSRVCBVUAV)
		);
	getCurrentResourceStorage().m_currentCommandList->SetPipelineState(m_PSO->first);

	if (m_PSO->second > 0)
	{
		std::chrono::time_point<std::chrono::system_clock> startTextureTime = std::chrono::system_clock::now();
		size_t usedTexture = UploadTextures(getCurrentResourceStorage().m_currentCommandList);

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
				getCPUDescriptorHandle(getCurrentResourceStorage().m_textureDescriptorsHeap.Get(),
					(getCurrentResourceStorage().m_currentTextureIndex + usedTexture) * g_descriptorStrideSRVCBVUAV)
				);

			D3D12_SAMPLER_DESC samplerDesc = {};
			samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			m_device->CreateSampler(&samplerDesc,
				getCPUDescriptorHandle(getCurrentResourceStorage().m_samplerDescriptorHeap[getCurrentResourceStorage().m_samplerDescriptorHeapIndex].Get(),
					(getCurrentResourceStorage().m_currentSamplerIndex + usedTexture) * g_descriptorStrideSamplers)
				);
		}

		getCurrentResourceStorage().m_currentCommandList->SetDescriptorHeaps(1, getCurrentResourceStorage().m_textureDescriptorsHeap.GetAddressOf());
		getCurrentResourceStorage().m_currentCommandList->SetGraphicsRootDescriptorTable(2,
			getGPUDescriptorHandle(getCurrentResourceStorage().m_textureDescriptorsHeap.Get(),
				getCurrentResourceStorage().m_currentTextureIndex * g_descriptorStrideSRVCBVUAV)
			);

		getCurrentResourceStorage().m_currentCommandList->SetDescriptorHeaps(1, getCurrentResourceStorage().m_samplerDescriptorHeap[getCurrentResourceStorage().m_samplerDescriptorHeapIndex].GetAddressOf());
		getCurrentResourceStorage().m_currentCommandList->SetGraphicsRootDescriptorTable(3,
			getGPUDescriptorHandle(getCurrentResourceStorage().m_samplerDescriptorHeap[getCurrentResourceStorage().m_samplerDescriptorHeapIndex].Get(),
				getCurrentResourceStorage().m_currentTextureIndex * g_descriptorStrideSamplers)
			);

		getCurrentResourceStorage().m_currentTextureIndex += usedTexture;
		getCurrentResourceStorage().m_currentSamplerIndex += usedTexture;
		std::chrono::time_point<std::chrono::system_clock> endTextureTime = std::chrono::system_clock::now();
		m_timers.m_textureUploadDuration += std::chrono::duration_cast<std::chrono::microseconds>(endTextureTime - startTextureTime).count();
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

	getCurrentResourceStorage().m_currentCommandList->OMSetRenderTargets((UINT)numRTT, &m_rtts.m_renderTargetsDescriptorsHeap->GetCPUDescriptorHandleForHeapStart(), true,
		&getCPUDescriptorHandle(m_rtts.m_depthStencilDescriptorHeap, 0));

	D3D12_VIEWPORT viewport =
	{
		0.f,
		0.f,
		(float)m_surface_clip_w,
		(float)m_surface_clip_h,
		-1.f,
		1.f
	};
	getCurrentResourceStorage().m_currentCommandList->RSSetViewports(1, &viewport);

	D3D12_RECT box =
	{
		0,
		0,
		(LONG)m_surface_clip_w,
		(LONG)m_surface_clip_h,
	};
	getCurrentResourceStorage().m_currentCommandList->RSSetScissorRects(1, &box);

	switch (m_draw_mode - 1)
	{
	case GL_POINTS:
		getCurrentResourceStorage().m_currentCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		break;
	case GL_LINES:
		getCurrentResourceStorage().m_currentCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		break;
	case GL_LINE_LOOP:
		getCurrentResourceStorage().m_currentCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ);
		break;
	case GL_LINE_STRIP:
		getCurrentResourceStorage().m_currentCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
		break;
	case GL_TRIANGLES:
		getCurrentResourceStorage().m_currentCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		break;
	case GL_TRIANGLE_STRIP:
		getCurrentResourceStorage().m_currentCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		break;
	case GL_TRIANGLE_FAN:
	case GL_QUADS:
		getCurrentResourceStorage().m_currentCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		break;
	case GL_QUAD_STRIP:
	case GL_POLYGON:
	default:
		getCurrentResourceStorage().m_currentCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		LOG_ERROR(RSX, "Unsupported primitive type");
		break;
	}

	if (m_renderingInfo.m_indexed)
		getCurrentResourceStorage().m_currentCommandList->DrawIndexedInstanced((UINT)m_renderingInfo.m_count, 1, 0, (UINT)m_renderingInfo.m_baseVertex, 0);
	else
		getCurrentResourceStorage().m_currentCommandList->DrawInstanced((UINT)m_renderingInfo.m_count, 1, (UINT)m_renderingInfo.m_baseVertex, 0);

	m_indexed_array.Reset();
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

void D3D12GSRender::Flip()
{
	ID3D12Resource *resourceToFlip;
	float viewport_w, viewport_h;

	if (!isFlipSurfaceInLocalMemory(m_surface_color_target))
	{
		ResourceStorage &storage = getCurrentResourceStorage();
		assert(storage.m_RAMFramebuffer == nullptr);

		D3D12_HEAP_PROPERTIES heapProp = {};
		heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

		size_t w = 0, h = 0, rowPitch = 0;

		ID3D12Resource *stagingTexture;
		if (m_read_buffer)
		{
			CellGcmDisplayInfo* buffers = vm::get_ptr<CellGcmDisplayInfo>(m_gcm_buffers_addr);
			u32 addr = GetAddress(buffers[m_gcm_current_buffer].offset, CELL_GCM_LOCATION_LOCAL);
			w = buffers[m_gcm_current_buffer].width;
			h = buffers[m_gcm_current_buffer].height;
			u8 *src_buffer = vm::get_ptr<u8>(addr);

			rowPitch = align(w * 4, 256);
			size_t textureSize = rowPitch * h; // * 4 for mipmap levels
			assert(m_textureUploadData.canAlloc(textureSize));
			size_t heapOffset = m_textureUploadData.alloc(textureSize);

			ThrowIfFailed(m_device->CreatePlacedResource(
				m_textureUploadData.m_heap,
				heapOffset,
				&getBufferResourceDesc(textureSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&stagingTexture)
				));
			getCurrentResourceStorage().m_singleFrameLifetimeResources.push_back(stagingTexture);

			void *dstBuffer;
			ThrowIfFailed(stagingTexture->Map(0, nullptr, &dstBuffer));
			for (unsigned row = 0; row < h; row++)
				memcpy((char*)dstBuffer + row * rowPitch, (char*)src_buffer + row * w * 4, w * 4);
			stagingTexture->Unmap(0, nullptr);
		}

		ThrowIfFailed(
			m_device->CreateCommittedResource(
				&heapProp,
				D3D12_HEAP_FLAG_NONE,
				&getTexture2DResourceDesc(w, h, DXGI_FORMAT_R8G8B8A8_UNORM, 1),
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(storage.m_RAMFramebuffer.GetAddressOf())
				)
			);
		D3D12_TEXTURE_COPY_LOCATION src = {}, dst = {};
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.pResource = storage.m_RAMFramebuffer.Get();
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.pResource = stagingTexture;
		src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		src.PlacedFootprint.Footprint.Width = (UINT)w;
		src.PlacedFootprint.Footprint.Height = (UINT)h;
		src.PlacedFootprint.Footprint.Depth = (UINT)1;
		src.PlacedFootprint.Footprint.RowPitch = (UINT)rowPitch;
		getCurrentResourceStorage().m_currentCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		getCurrentResourceStorage().m_currentCommandList->ResourceBarrier(1, &getResourceBarrierTransition(storage.m_RAMFramebuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
		resourceToFlip = storage.m_RAMFramebuffer.Get();
		viewport_w = (float)w, viewport_h = (float)h;
	}
	else
	{
		if (m_rtts.m_currentlyBoundRenderTargets[0] != nullptr)
			getCurrentResourceStorage().m_currentCommandList->ResourceBarrier(1, &getResourceBarrierTransition(m_rtts.m_currentlyBoundRenderTargets[0], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
		resourceToFlip = m_rtts.m_currentlyBoundRenderTargets[0];
	}

	getCurrentResourceStorage().m_currentCommandList->ResourceBarrier(1, &getResourceBarrierTransition(m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	D3D12_VIEWPORT viewport =
	{
		0.f,
		0.f,
		(float)m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()]->GetDesc().Width,
		(float)m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()]->GetDesc().Height,
		0.f,
		1.f
	};
	getCurrentResourceStorage().m_currentCommandList->RSSetViewports(1, &viewport);

	D3D12_RECT box =
	{
		0,
		0,
		(LONG)m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()]->GetDesc().Width,
		(LONG)m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()]->GetDesc().Height,
	};
	getCurrentResourceStorage().m_currentCommandList->RSSetScissorRects(1, &box);
	getCurrentResourceStorage().m_currentCommandList->SetGraphicsRootSignature(m_outputScalingPass.m_rootSignature);
	getCurrentResourceStorage().m_currentCommandList->SetPipelineState(m_outputScalingPass.m_PSO);
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
	CPUHandle = m_outputScalingPass.m_textureDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	CPUHandle.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * m_swapChain->GetCurrentBackBufferIndex();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	// FIXME: Not always true
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	if (isFlipSurfaceInLocalMemory(m_surface_color_target))
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	else
		srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3,
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0
			);
	m_device->CreateShaderResourceView(resourceToFlip, &srvDesc, CPUHandle);

	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	CPUHandle = m_outputScalingPass.m_samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	CPUHandle.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) * m_swapChain->GetCurrentBackBufferIndex();
	m_device->CreateSampler(&samplerDesc, CPUHandle);

	D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
	GPUHandle = m_outputScalingPass.m_textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	GPUHandle.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * m_swapChain->GetCurrentBackBufferIndex();
	getCurrentResourceStorage().m_currentCommandList->SetDescriptorHeaps(1, &m_outputScalingPass.m_textureDescriptorHeap);
	getCurrentResourceStorage().m_currentCommandList->SetGraphicsRootDescriptorTable(0, GPUHandle);
	GPUHandle = m_outputScalingPass.m_samplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	GPUHandle.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) * m_swapChain->GetCurrentBackBufferIndex();
	getCurrentResourceStorage().m_currentCommandList->SetDescriptorHeaps(1, &m_outputScalingPass.m_samplerDescriptorHeap);
	getCurrentResourceStorage().m_currentCommandList->SetGraphicsRootDescriptorTable(1, GPUHandle);

	CPUHandle = m_backbufferAsRendertarget[m_swapChain->GetCurrentBackBufferIndex()]->GetCPUDescriptorHandleForHeapStart();
	getCurrentResourceStorage().m_currentCommandList->OMSetRenderTargets(1, &CPUHandle, true, nullptr);
	D3D12_VERTEX_BUFFER_VIEW vbv = {};
	vbv.BufferLocation = m_outputScalingPass.m_vertexBuffer->GetGPUVirtualAddress();
	vbv.StrideInBytes = 4 * sizeof(float);
	vbv.SizeInBytes = 16 * sizeof(float);
	getCurrentResourceStorage().m_currentCommandList->IASetVertexBuffers(0, 1, &vbv);
	getCurrentResourceStorage().m_currentCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	if (m_rtts.m_currentlyBoundRenderTargets[0] != nullptr)
		getCurrentResourceStorage().m_currentCommandList->DrawInstanced(4, 1, 0, 0);

	getCurrentResourceStorage().m_currentCommandList->ResourceBarrier(1, &getResourceBarrierTransition(m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	if (isFlipSurfaceInLocalMemory(m_surface_color_target) && m_rtts.m_currentlyBoundRenderTargets[0] != nullptr)
		getCurrentResourceStorage().m_currentCommandList->ResourceBarrier(1, &getResourceBarrierTransition(m_rtts.m_currentlyBoundRenderTargets[0], D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	ThrowIfFailed(getCurrentResourceStorage().m_currentCommandList->Close());
	m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)&(getCurrentResourceStorage().m_currentCommandList));

	ThrowIfFailed(m_swapChain->Present(Ini.GSVSyncEnable.GetValue() ? 1 : 0, 0));
	// Add an event signaling queue completion

	ResourceStorage &storage = getNonCurrentResourceStorage();

	storage.m_frameFinishedFence->SetEventOnCompletion(storage.m_fenceValue, storage.m_frameFinishedHandle);
	m_commandQueueGraphic->Signal(storage.m_frameFinishedFence.Get(), storage.m_fenceValue);
	storage.m_fenceValue++;

	// Flush
	m_texturesRTTs.clear();
	m_vertexCache.clear();
	m_vertexConstants.clear();


	// Get the put pos - 1. This way after cleaning we can set the get ptr to
	// this value, allowing heap to proceed even if we cleant before allocating
	// a new value (that's the reason of the -1)
	size_t newGetPosConstantsHeap = m_constantsData.getCurrentPutPosMinusOne();
	size_t newGetPosVertexIndexHeap = m_vertexIndexData.getCurrentPutPosMinusOne();
	size_t newGetPosTextureUploadHeap = m_textureUploadData.getCurrentPutPosMinusOne();
	size_t newGetPosReadbackHeap = m_readbackResources.getCurrentPutPosMinusOne();
	size_t newGetPosUAVHeap = m_UAVHeap.getCurrentPutPosMinusOne();

	std::lock_guard<std::mutex> lock(mut);
	std::vector<ID3D12Resource *> textoclean = m_texToClean;
	m_texToClean.clear();

	storage.m_isUseable.store(false);

	m_GC.pushWork([&,
		textoclean,
		newGetPosConstantsHeap,
		newGetPosVertexIndexHeap,
		newGetPosTextureUploadHeap,
		newGetPosReadbackHeap,
		newGetPosUAVHeap]()
	{
		storage.WaitAndClean(textoclean);
		m_constantsData.m_getPos.store(newGetPosConstantsHeap, std::memory_order_release);
		m_vertexIndexData.m_getPos.store(newGetPosVertexIndexHeap, std::memory_order_release);
		m_textureUploadData.m_getPos.store(newGetPosTextureUploadHeap, std::memory_order_release);
		m_readbackResources.m_getPos.store(newGetPosReadbackHeap, std::memory_order_release);
		m_UAVHeap.m_getPos.store(newGetPosUAVHeap, std::memory_order_release);
	});

	while (!getCurrentResourceStorage().m_isUseable.load(std::memory_order_acquire))
		std::this_thread::yield();
	m_frame->Flip(nullptr);

	ResetTimer();
}

void D3D12GSRender::ResetTimer()
{
	m_timers.m_textureUploadDuration = 0;
	m_timers.m_vertexUploadDuration = 0;
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
	ID3D12Resource *Result;
	size_t w = m_surface_clip_w, h = m_surface_clip_h;
	DXGI_FORMAT dxgiFormat;
	size_t rowPitch;
	switch (m_surface_color_format)
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

	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_READBACK;
	D3D12_RESOURCE_DESC resdesc = getBufferResourceDesc(rowPitch * h);

	size_t sizeInByte = rowPitch * h;
	assert(m_readbackResources.canAlloc(sizeInByte));
	size_t heapOffset = m_readbackResources.alloc(sizeInByte);

	resdesc = getBufferResourceDesc(sizeInByte);
	ThrowIfFailed(
		m_device->CreatePlacedResource(
			m_readbackResources.m_heap,
			heapOffset,
			&resdesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&Result)
			)
		);
	getCurrentResourceStorage().m_singleFrameLifetimeResources.push_back(Result);

	cmdlist->ResourceBarrier(1, &getResourceBarrierTransition(RTT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

	D3D12_TEXTURE_COPY_LOCATION dst = {}, src = {};
	src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	src.pResource = RTT;
	dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	dst.pResource = Result;
	dst.PlacedFootprint.Offset = 0;
	dst.PlacedFootprint.Footprint.Depth = 1;
	dst.PlacedFootprint.Footprint.Format = dxgiFormat;
	dst.PlacedFootprint.Footprint.Height = (UINT)h;
	dst.PlacedFootprint.Footprint.Width = (UINT)w;
	dst.PlacedFootprint.Footprint.RowPitch = (UINT)rowPitch;
	cmdlist->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	cmdlist->ResourceBarrier(1, &getResourceBarrierTransition(RTT, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
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

void D3D12GSRender::semaphorePGRAPHTextureReadRelease(u32 offset, u32 value)
{
	semaphorePGRAPHBackendRelease(offset, value);
}

void D3D12GSRender::semaphorePGRAPHBackendRelease(u32 offset, u32 value)
{
	// Add all buffer write
	// Cell can't make any assumption about readyness of color/depth buffer
	// Except when a semaphore is written by RSX


	ID3D12Fence *fence;
	ThrowIfFailed(
		m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))
		);
	HANDLE handle = CreateEvent(0, FALSE, FALSE, 0);
	fence->SetEventOnCompletion(1, handle);

	ComPtr<ID3D12Resource> writeDest, depthConverted;
	ID3D12DescriptorHeap *descriptorHeap;
	size_t depthRowPitch = m_surface_clip_w;
	depthRowPitch = (depthRowPitch + 255) & ~255;

	bool needTransfer = (m_set_context_dma_z && Ini.GSDumpDepthBuffer.GetValue()) ||
		((m_set_context_dma_color_a || m_set_context_dma_color_b || m_set_context_dma_color_c || m_set_context_dma_color_d) && Ini.GSDumpColorBuffers.GetValue());

	if (m_set_context_dma_z && Ini.GSDumpDepthBuffer.GetValue())
	{
		D3D12_HEAP_PROPERTIES heapProp = {};
		heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_RESOURCE_DESC resdesc = getTexture2DResourceDesc(m_surface_clip_w, m_surface_clip_h, DXGI_FORMAT_R8_UNORM, 1);
		resdesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		size_t sizeInByte = m_surface_clip_w * m_surface_clip_h * 2;
		assert(m_UAVHeap.canAlloc(sizeInByte));
		size_t heapOffset = m_UAVHeap.alloc(sizeInByte);

		ThrowIfFailed(
			m_device->CreatePlacedResource(
				m_UAVHeap.m_heap,
				heapOffset,
				&resdesc,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				nullptr,
				IID_PPV_ARGS(depthConverted.GetAddressOf())
				)
			);
		getCurrentResourceStorage().m_singleFrameLifetimeResources.push_back(depthConverted);

		sizeInByte = depthRowPitch * m_surface_clip_h;
		assert(m_readbackResources.canAlloc(sizeInByte));
		heapOffset = m_readbackResources.alloc(sizeInByte);

		resdesc = getBufferResourceDesc(sizeInByte);
		ThrowIfFailed(
			m_device->CreatePlacedResource(
				m_readbackResources.m_heap,
				heapOffset,
				&resdesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(writeDest.GetAddressOf())
				)
			);
		getCurrentResourceStorage().m_singleFrameLifetimeResources.push_back(writeDest);

		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
		descriptorHeapDesc.NumDescriptors = 2;
		descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(
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
		m_device->CreateUnorderedAccessView(depthConverted.Get(), nullptr, &uavDesc, Handle);

		// Convert
		getCurrentResourceStorage().m_currentCommandList->ResourceBarrier(1, &getResourceBarrierTransition(m_rtts.m_currentlyBoundDepthStencil, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));

		getCurrentResourceStorage().m_currentCommandList->SetPipelineState(m_convertPSO);
		getCurrentResourceStorage().m_currentCommandList->SetComputeRootSignature(m_convertRootSignature);
		getCurrentResourceStorage().m_currentCommandList->SetDescriptorHeaps(1, &descriptorHeap);
		getCurrentResourceStorage().m_currentCommandList->SetComputeRootDescriptorTable(0, descriptorHeap->GetGPUDescriptorHandleForHeapStart());
		getCurrentResourceStorage().m_currentCommandList->Dispatch(m_surface_clip_w / 8, m_surface_clip_h / 8, 1);

		// Flush UAV
		D3D12_RESOURCE_BARRIER uavbarrier = {};
		uavbarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavbarrier.UAV.pResource = depthConverted.Get();

		D3D12_RESOURCE_BARRIER barriers[] =
		{
			getResourceBarrierTransition(m_rtts.m_currentlyBoundDepthStencil, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE),
			uavbarrier,
		};
		getCurrentResourceStorage().m_currentCommandList->ResourceBarrier(2, barriers);
		getCurrentResourceStorage().m_currentCommandList->ResourceBarrier(1, &getResourceBarrierTransition(depthConverted.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	}

	if (m_set_context_dma_z && Ini.GSDumpDepthBuffer.GetValue())
	{
		// Copy
		D3D12_TEXTURE_COPY_LOCATION dst = {}, src = {};
		src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.pResource = depthConverted.Get();
		dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		dst.pResource = writeDest.Get();
		dst.PlacedFootprint.Offset = 0;
		dst.PlacedFootprint.Footprint.Depth = 1;
		dst.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8_UNORM;
		dst.PlacedFootprint.Footprint.Height = m_surface_clip_h;
		dst.PlacedFootprint.Footprint.Width = m_surface_clip_w;
		dst.PlacedFootprint.Footprint.RowPitch = (UINT)depthRowPitch;
		getCurrentResourceStorage().m_currentCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		invalidateTexture(GetAddress(m_surface_offset_z, m_context_dma_z - 0xfeed0000));
	}

	ID3D12Resource *rtt0, *rtt1, *rtt2, *rtt3;
	if (Ini.GSDumpColorBuffers.GetValue())
	{
		switch (m_surface_color_target)
		{
		case CELL_GCM_SURFACE_TARGET_NONE:
			break;

		case CELL_GCM_SURFACE_TARGET_0:
			if (m_context_dma_color_a) rtt0 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[0], getCurrentResourceStorage().m_currentCommandList);
			break;

		case CELL_GCM_SURFACE_TARGET_1:
			if (m_context_dma_color_b) rtt1 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[0], getCurrentResourceStorage().m_currentCommandList);
			break;

		case CELL_GCM_SURFACE_TARGET_MRT1:
			if (m_context_dma_color_a) rtt0 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[0], getCurrentResourceStorage().m_currentCommandList);
			if (m_context_dma_color_b) rtt1 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[1], getCurrentResourceStorage().m_currentCommandList);
			break;

		case CELL_GCM_SURFACE_TARGET_MRT2:
			if (m_context_dma_color_a) rtt0 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[0], getCurrentResourceStorage().m_currentCommandList);
			if (m_context_dma_color_b) rtt1 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[1], getCurrentResourceStorage().m_currentCommandList);
			if (m_context_dma_color_c) rtt2 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[2], getCurrentResourceStorage().m_currentCommandList);
			break;

		case CELL_GCM_SURFACE_TARGET_MRT3:
			if (m_context_dma_color_a) rtt0 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[0], getCurrentResourceStorage().m_currentCommandList);
			if (m_context_dma_color_b) rtt1 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[1], getCurrentResourceStorage().m_currentCommandList);
			if (m_context_dma_color_c) rtt2 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[2], getCurrentResourceStorage().m_currentCommandList);
			if (m_context_dma_color_d) rtt3 = writeColorBuffer(m_rtts.m_currentlyBoundRenderTargets[3], getCurrentResourceStorage().m_currentCommandList);
			break;
		}

		if (m_context_dma_color_a) invalidateTexture(GetAddress(m_surface_offset_a, m_context_dma_color_a - 0xfeed0000));
		if (m_context_dma_color_b) invalidateTexture(GetAddress(m_surface_offset_b, m_context_dma_color_b - 0xfeed0000));
		if (m_context_dma_color_c) invalidateTexture(GetAddress(m_surface_offset_c, m_context_dma_color_c - 0xfeed0000));
		if (m_context_dma_color_d) invalidateTexture(GetAddress(m_surface_offset_d, m_context_dma_color_d - 0xfeed0000));
	}
	if (needTransfer)
	{
		ThrowIfFailed(getCurrentResourceStorage().m_currentCommandList->Close());
		m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)&(getCurrentResourceStorage().m_currentCommandList));
		getCurrentResourceStorage().setNewCommandList();
	}

	//Wait for result
	m_commandQueueGraphic->Signal(fence, 1);

	m_GC.pushWork([=]() {
		WaitForSingleObject(handle, INFINITE);
		CloseHandle(handle);
		fence->Release();

		if (m_set_context_dma_z && Ini.GSDumpDepthBuffer.GetValue())
		{
			u32 address = GetAddress(m_surface_offset_z, m_context_dma_z - 0xfeed0000);
			auto ptr = vm::get_ptr<void>(address);
			char *ptrAsChar = (char*)ptr;
			unsigned char *writeDestPtr;
			ThrowIfFailed(writeDest->Map(0, nullptr, (void**)&writeDestPtr));
			// TODO : this should be done by the gpu
			for (unsigned row = 0; row < m_surface_clip_h; row++)
			{
				for (unsigned i = 0; i < m_surface_clip_w; i++)
				{
					unsigned char c = writeDestPtr[row * depthRowPitch + i];
					ptrAsChar[4 * (row * m_surface_clip_w + i)] = c;
					ptrAsChar[4 * (row * m_surface_clip_w + i) + 1] = c;
					ptrAsChar[4 * (row * m_surface_clip_w + i) + 2] = c;
					ptrAsChar[4 * (row * m_surface_clip_w + i) + 3] = c;
				}
			}
			descriptorHeap->Release();
		}

		size_t srcPitch, dstPitch;
		switch (m_surface_color_format)
		{
		case CELL_GCM_SURFACE_A8R8G8B8:
			srcPitch = align(m_surface_clip_w * 4, 256);
			dstPitch = m_surface_clip_w * 4;
			break;
		case CELL_GCM_SURFACE_F_W16Z16Y16X16:
			srcPitch = align(m_surface_clip_w * 8, 256);
			dstPitch = m_surface_clip_w * 8;
			break;
		}

		if (Ini.GSDumpColorBuffers.GetValue())
		{
			switch (m_surface_color_target)
			{
			case CELL_GCM_SURFACE_TARGET_NONE:
				break;

			case CELL_GCM_SURFACE_TARGET_0:
			{
				u32 address = GetAddress(m_surface_offset_a, m_context_dma_color_a - 0xfeed0000);
				void *dstAddress = vm::get_ptr<void>(address);
				copyToCellRamAndRelease(dstAddress, rtt0, srcPitch, dstPitch, m_surface_clip_w, m_surface_clip_h);
			}
			break;

			case CELL_GCM_SURFACE_TARGET_1:
			{
				u32 address = GetAddress(m_surface_offset_b, m_context_dma_color_b - 0xfeed0000);
				void *dstAddress = vm::get_ptr<void>(address);
				copyToCellRamAndRelease(dstAddress, rtt1, srcPitch, dstPitch, m_surface_clip_w, m_surface_clip_h);
			}
			break;

			case CELL_GCM_SURFACE_TARGET_MRT1:
			{
				u32 address = GetAddress(m_surface_offset_a, m_context_dma_color_a - 0xfeed0000);
				void *dstAddress = vm::get_ptr<void>(address);
				copyToCellRamAndRelease(dstAddress, rtt0, srcPitch, dstPitch, m_surface_clip_w, m_surface_clip_h);
				address = GetAddress(m_surface_offset_b, m_context_dma_color_b - 0xfeed0000);
				dstAddress = vm::get_ptr<void>(address);
				copyToCellRamAndRelease(dstAddress, rtt1, srcPitch, dstPitch, m_surface_clip_w, m_surface_clip_h);
			}
			break;

			case CELL_GCM_SURFACE_TARGET_MRT2:
			{
				u32 address = GetAddress(m_surface_offset_a, m_context_dma_color_a - 0xfeed0000);
				void *dstAddress = vm::get_ptr<void>(address);
				copyToCellRamAndRelease(dstAddress, rtt0, srcPitch, dstPitch, m_surface_clip_w, m_surface_clip_h);
				address = GetAddress(m_surface_offset_b, m_context_dma_color_b - 0xfeed0000);
				dstAddress = vm::get_ptr<void>(address);
				copyToCellRamAndRelease(dstAddress, rtt1, srcPitch, dstPitch, m_surface_clip_w, m_surface_clip_h);
				address = GetAddress(m_surface_offset_c, m_context_dma_color_c - 0xfeed0000);
				dstAddress = vm::get_ptr<void>(address);
				copyToCellRamAndRelease(dstAddress, rtt2, srcPitch, dstPitch, m_surface_clip_w, m_surface_clip_h);
			}
			break;

			case CELL_GCM_SURFACE_TARGET_MRT3:
			{
				u32 address = GetAddress(m_surface_offset_a, m_context_dma_color_a - 0xfeed0000);
				void *dstAddress = vm::get_ptr<void>(address);
				copyToCellRamAndRelease(dstAddress, rtt0, srcPitch, dstPitch, m_surface_clip_w, m_surface_clip_h);
				address = GetAddress(m_surface_offset_b, m_context_dma_color_b - 0xfeed0000);
				dstAddress = vm::get_ptr<void>(address);
				copyToCellRamAndRelease(dstAddress, rtt1, srcPitch, dstPitch, m_surface_clip_w, m_surface_clip_h);
				address = GetAddress(m_surface_offset_c, m_context_dma_color_c - 0xfeed0000);
				dstAddress = vm::get_ptr<void>(address);
				copyToCellRamAndRelease(dstAddress, rtt2, srcPitch, dstPitch, m_surface_clip_w, m_surface_clip_h);
				address = GetAddress(m_surface_offset_d, m_context_dma_color_d - 0xfeed0000);
				dstAddress = vm::get_ptr<void>(address);
				copyToCellRamAndRelease(dstAddress, rtt3, srcPitch, dstPitch, m_surface_clip_w, m_surface_clip_h);
			}
			break;
			}
		}

		vm::write32(m_label_addr + offset, value);
	});

	m_GC.waitForCompletion();
}

void D3D12GSRender::semaphorePFIFOAcquire(u32 offset, u32 value)
{
	const std::chrono::time_point<std::chrono::system_clock> enterWait = std::chrono::system_clock::now();
	while (true)
	{
		volatile u32 val = vm::read32(m_label_addr + offset);
		if (val == value) break;
		std::chrono::time_point<std::chrono::system_clock> waitPoint = std::chrono::system_clock::now();
		long long elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(waitPoint - enterWait).count();
		if (elapsedTime > 0)
			LOG_ERROR(RSX, "Has wait for more than a second for semaphore acquire");
		std::this_thread::yield();
	}
}
#endif
