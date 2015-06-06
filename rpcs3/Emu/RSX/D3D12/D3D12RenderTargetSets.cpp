#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "D3D12RenderTargetSets.h"
#include "rpcs3/Ini.h"
#include "Utilities/rPlatform.h" // only for rImage
#include "Utilities/File.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/RSX/GSRender.h"

#include "D3D12.h"
#include "D3D12GSRender.h"

void D3D12GSRender::InitDrawBuffers()
{
	// FBO location has changed, previous data might be copied
	u32 address_a = GetAddress(m_surface_offset_a, m_context_dma_color_a - 0xfeed0000);
	u32 address_b = GetAddress(m_surface_offset_b, m_context_dma_color_b - 0xfeed0000);
	u32 address_c = GetAddress(m_surface_offset_c, m_context_dma_color_c - 0xfeed0000);
	u32 address_d = GetAddress(m_surface_offset_d, m_context_dma_color_d - 0xfeed0000);
	u32 address_z = GetAddress(m_surface_offset_z, m_context_dma_z - 0xfeed0000);

	ID3D12GraphicsCommandList *copycmdlist;
	check(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, getCurrentResourceStorage().m_commandAllocator, nullptr, IID_PPV_ARGS(&copycmdlist)));
	getCurrentResourceStorage().m_inflightCommandList.push_back(copycmdlist);

	// Make previous RTTs sampleable
	for (unsigned i = 0; i < 4; i++)
	{
		if (m_rtts.m_currentlyBoundRenderTargets[i] == nullptr)
			continue;
		copycmdlist->ResourceBarrier(1, &getResourceBarrierTransition(m_rtts.m_currentlyBoundRenderTargets[i], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	}
	// Same for depth buffer
	if (m_rtts.m_currentlyBoundDepthStencil != nullptr)
		copycmdlist->ResourceBarrier(1, &getResourceBarrierTransition(m_rtts.m_currentlyBoundDepthStencil, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));

	memset(m_rtts.m_currentlyBoundRenderTargetsAddress, 0, 4 * sizeof(u32));
	memset(m_rtts.m_currentlyBoundRenderTargets, 0, 4 * sizeof(ID3D12Resource *));
	m_rtts.m_currentlyBoundDepthStencil = nullptr;
	m_rtts.m_currentlyBoundDepthStencilAddress = 0;


	D3D12_CPU_DESCRIPTOR_HANDLE Handle = m_rtts.m_renderTargetsDescriptorsHeap->GetCPUDescriptorHandleForHeapStart();
	size_t g_RTTIncrement = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	switch (m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_0:
	{
		ID3D12Resource *rttA = m_rtts.bindAddressAsRenderTargets(m_device, copycmdlist, 0, address_a, m_surface_clip_w, m_surface_clip_h,
			m_clear_surface_color_r / 255.0f, m_clear_surface_color_g / 255.0f, m_clear_surface_color_b / 255.0f, m_clear_surface_color_a / 255.0f);
		D3D12_RENDER_TARGET_VIEW_DESC rttViewDesc = {};
		rttViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rttViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateRenderTargetView(rttA, &rttViewDesc, Handle);
		break;
	}
	case CELL_GCM_SURFACE_TARGET_1:
	{
		ID3D12Resource *rttB = m_rtts.bindAddressAsRenderTargets(m_device, copycmdlist, 0, address_b, m_surface_clip_w, m_surface_clip_h,
			m_clear_surface_color_r / 255.0f, m_clear_surface_color_g / 255.0f, m_clear_surface_color_b / 255.0f, m_clear_surface_color_a / 255.0f);
		D3D12_RENDER_TARGET_VIEW_DESC rttViewDesc = {};
		rttViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rttViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateRenderTargetView(rttB, &rttViewDesc, Handle);
		break;
	}
	case CELL_GCM_SURFACE_TARGET_MRT1:
	{
		ID3D12Resource *rttA = m_rtts.bindAddressAsRenderTargets(m_device, copycmdlist, 0, address_a, m_surface_clip_w, m_surface_clip_h,
			m_clear_surface_color_r / 255.0f, m_clear_surface_color_g / 255.0f, m_clear_surface_color_b / 255.0f, m_clear_surface_color_a / 255.0f);
		D3D12_RENDER_TARGET_VIEW_DESC rttViewDesc = {};
		rttViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rttViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateRenderTargetView(rttA, &rttViewDesc, Handle);
		Handle.ptr += g_RTTIncrement;
		ID3D12Resource *rttB = m_rtts.bindAddressAsRenderTargets(m_device, copycmdlist, 1, address_b, m_surface_clip_w, m_surface_clip_h,
			m_clear_surface_color_r / 255.0f, m_clear_surface_color_g / 255.0f, m_clear_surface_color_b / 255.0f, m_clear_surface_color_a / 255.0f);
		rttViewDesc = {};
		rttViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rttViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateRenderTargetView(rttB, &rttViewDesc, Handle);
	}
	break;
	case CELL_GCM_SURFACE_TARGET_MRT2:
	{
		ID3D12Resource *rttA = m_rtts.bindAddressAsRenderTargets(m_device, copycmdlist, 0, address_a, m_surface_clip_w, m_surface_clip_h,
			m_clear_surface_color_r / 255.0f, m_clear_surface_color_g / 255.0f, m_clear_surface_color_b / 255.0f, m_clear_surface_color_a / 255.0f);
		D3D12_RENDER_TARGET_VIEW_DESC rttViewDesc = {};
		rttViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rttViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateRenderTargetView(rttA, &rttViewDesc, Handle);
		Handle.ptr += g_RTTIncrement;
		ID3D12Resource *rttB = m_rtts.bindAddressAsRenderTargets(m_device, copycmdlist, 1, address_b, m_surface_clip_w, m_surface_clip_h,
			m_clear_surface_color_r / 255.0f, m_clear_surface_color_g / 255.0f, m_clear_surface_color_b / 255.0f, m_clear_surface_color_a / 255.0f);
		rttViewDesc = {};
		rttViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rttViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateRenderTargetView(rttB, &rttViewDesc, Handle);
		Handle.ptr += g_RTTIncrement;
		ID3D12Resource *rttC = m_rtts.bindAddressAsRenderTargets(m_device, copycmdlist, 2, address_c, m_surface_clip_w, m_surface_clip_h,
			m_clear_surface_color_r / 255.0f, m_clear_surface_color_g / 255.0f, m_clear_surface_color_b / 255.0f, m_clear_surface_color_a / 255.0f);
		rttViewDesc = {};
		rttViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rttViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateRenderTargetView(rttC, &rttViewDesc, Handle);
		break;
	}
	case CELL_GCM_SURFACE_TARGET_MRT3:
	{
		ID3D12Resource *rttA = m_rtts.bindAddressAsRenderTargets(m_device, copycmdlist, 0, address_a, m_surface_clip_w, m_surface_clip_h,
			m_clear_surface_color_r / 255.0f, m_clear_surface_color_g / 255.0f, m_clear_surface_color_b / 255.0f, m_clear_surface_color_a / 255.0f);
		D3D12_RENDER_TARGET_VIEW_DESC rttViewDesc = {};
		rttViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rttViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateRenderTargetView(rttA, &rttViewDesc, Handle);
		Handle.ptr += g_RTTIncrement;
		ID3D12Resource *rttB = m_rtts.bindAddressAsRenderTargets(m_device, copycmdlist, 1, address_b, m_surface_clip_w, m_surface_clip_h,
			m_clear_surface_color_r / 255.0f, m_clear_surface_color_g / 255.0f, m_clear_surface_color_b / 255.0f, m_clear_surface_color_a / 255.0f);
		rttViewDesc = {};
		rttViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rttViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateRenderTargetView(rttB, &rttViewDesc, Handle);
		Handle.ptr += g_RTTIncrement;
		ID3D12Resource *rttC = m_rtts.bindAddressAsRenderTargets(m_device, copycmdlist, 2, address_c, m_surface_clip_w, m_surface_clip_h,
			m_clear_surface_color_r / 255.0f, m_clear_surface_color_g / 255.0f, m_clear_surface_color_b / 255.0f, m_clear_surface_color_a / 255.0f);
		rttViewDesc = {};
		rttViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rttViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateRenderTargetView(rttC, &rttViewDesc, Handle);
		Handle.ptr += g_RTTIncrement;
		ID3D12Resource *rttD = m_rtts.bindAddressAsRenderTargets(m_device, copycmdlist, 3, address_d, m_surface_clip_w, m_surface_clip_h,
			m_clear_surface_color_r / 255.0f, m_clear_surface_color_g / 255.0f, m_clear_surface_color_b / 255.0f, m_clear_surface_color_a / 255.0f);
		rttViewDesc = {};
		rttViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rttViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateRenderTargetView(rttD, &rttViewDesc, Handle);
		break;
	}
	}

	ID3D12Resource *ds = m_rtts.bindAddressAsDepthStencil(m_device, copycmdlist, address_z, m_surface_clip_w, m_surface_clip_h, m_surface_depth_format, 1., 0);

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
	switch (m_surface_depth_format)
	{
	case 0:
		break;
	case CELL_GCM_SURFACE_Z16:
		depthStencilViewDesc.Format = DXGI_FORMAT_D16_UNORM;
		break;
	case CELL_GCM_SURFACE_Z24S8:
		depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		break;
	default:
		LOG_ERROR(RSX, "Bad depth format! (%d)", m_surface_depth_format);
		assert(0);
	}
	depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	m_device->CreateDepthStencilView(ds, &depthStencilViewDesc, m_rtts.m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	check(copycmdlist->Close());
	m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)&copycmdlist);
}

ID3D12Resource *RenderTargets::bindAddressAsRenderTargets(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, size_t slot, u32 address,
	size_t width, size_t height, float clearColorR, float clearColorG, float clearColorB, float clearColorA)
{
	ID3D12Resource* rtt;
	auto It = m_renderTargets.find(address);
	// TODO: Check if sizes match
	if (It != m_renderTargets.end())
	{
		rtt = It->second;
		cmdList->ResourceBarrier(1, &getResourceBarrierTransition(rtt, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}
	else
	{
		LOG_WARNING(RSX, "Creating RTT");
		D3D12_CLEAR_VALUE clearColorValue = {};
		clearColorValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		clearColorValue.Color[0] = clearColorR;
		clearColorValue.Color[1] = clearColorG;
		clearColorValue.Color[2] = clearColorB;
		clearColorValue.Color[3] = clearColorA;

		D3D12_HEAP_PROPERTIES heapProp = {};
		heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC resourceDesc = getTexture2DResourceDesc(width, height, DXGI_FORMAT_R8G8B8A8_UNORM);
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&clearColorValue,
			IID_PPV_ARGS(&rtt)
			);
		m_renderTargets[address] = rtt;
	}
	m_currentlyBoundRenderTargetsAddress[slot] = address;
	m_currentlyBoundRenderTargets[slot] = rtt;
	return rtt;
}

ID3D12Resource * RenderTargets::bindAddressAsDepthStencil(ID3D12Device * device, ID3D12GraphicsCommandList * cmdList, u32 address, size_t width, size_t height, u8 surfaceDepthFormat, float depthClear, u8 stencilClear)
{
	ID3D12Resource* ds;
	auto It = m_depthStencil.find(address);
	// TODO: Check if sizes and surface depth format match

	if (It != m_depthStencil.end())
	{
		ds = It->second;
		cmdList->ResourceBarrier(1, &getResourceBarrierTransition(ds, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	}
	else
	{
		D3D12_CLEAR_VALUE clearDepthValue = {};
		clearDepthValue.DepthStencil.Depth = depthClear;

		D3D12_HEAP_PROPERTIES heapProp = {};
		heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

		DXGI_FORMAT dxgiFormat;
		switch (surfaceDepthFormat)
		{
		case 0:
		break;
		case CELL_GCM_SURFACE_Z16:
			dxgiFormat = DXGI_FORMAT_R16_TYPELESS;
			clearDepthValue.Format = DXGI_FORMAT_D16_UNORM;
		break;
		case CELL_GCM_SURFACE_Z24S8:
			dxgiFormat = DXGI_FORMAT_R24G8_TYPELESS;
			clearDepthValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		break;
		default:
		LOG_ERROR(RSX, "Bad depth format! (%d)", surfaceDepthFormat);
		assert(0);
		}

		D3D12_RESOURCE_DESC resourceDesc = getTexture2DResourceDesc(width, height, dxgiFormat);
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearDepthValue,
		IID_PPV_ARGS(&ds)
		);
		m_depthStencil[address] = ds;
	}
	m_currentlyBoundDepthStencil = ds;
	m_currentlyBoundDepthStencilAddress = address;
	return ds;
}

void RenderTargets::Init(ID3D12Device *device)//, u8 surfaceDepthFormat, size_t width, size_t height, float clearColor[4], float clearDepth)
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.NumDescriptors = 1;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_depthStencilDescriptorHeap));

	descriptorHeapDesc.NumDescriptors = 4;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_renderTargetsDescriptorsHeap));

	memset(m_currentlyBoundRenderTargetsAddress, 0, 4 * sizeof(u32));
	memset(m_currentlyBoundRenderTargets, 0, 4 * sizeof(ID3D12Resource*));
	m_currentlyBoundDepthStencil = nullptr;
	m_currentlyBoundDepthStencilAddress = 0;
}

void RenderTargets::Release()
{
	for (auto tmp : m_renderTargets)
		tmp.second->Release();
	m_depthStencilDescriptorHeap->Release();
	m_renderTargetsDescriptorsHeap->Release();
	for (auto tmp : m_depthStencil)
		tmp.second->Release();
}
#endif