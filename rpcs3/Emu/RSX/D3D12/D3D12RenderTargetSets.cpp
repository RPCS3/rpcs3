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

void D3D12GSRender::PrepareRenderTargets(ID3D12GraphicsCommandList *copycmdlist)
{
	u32 surface_format = rsx::method_registers[NV4097_SET_SURFACE_FORMAT];

	u32 clip_horizontal = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL];
	u32 clip_vertical = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL];

	u32 clip_width = clip_horizontal >> 16;
	u32 clip_height = clip_vertical >> 16;
	u32 clip_x = clip_horizontal;
	u32 clip_y = clip_vertical;

	if (m_surface.format != surface_format)
	{
		m_surface.unpack(surface_format);
		m_surface.width = clip_width;
		m_surface.height = clip_height;
	}

	// Exit early if there is no rtt changes
	if ((m_previous_address_a == rsx::method_registers[NV4097_SET_SURFACE_COLOR_AOFFSET]) &&
		(m_previous_address_b == rsx::method_registers[NV4097_SET_SURFACE_COLOR_BOFFSET]) &&
		(m_previous_address_c == rsx::method_registers[NV4097_SET_SURFACE_COLOR_COFFSET]) &&
		(m_previous_address_d == rsx::method_registers[NV4097_SET_SURFACE_COLOR_DOFFSET]) &&
		(m_previous_address_z == rsx::method_registers[NV4097_SET_SURFACE_ZETA_OFFSET]))
		return;

	m_previous_address_a = rsx::method_registers[NV4097_SET_SURFACE_COLOR_AOFFSET];
	m_previous_address_b = rsx::method_registers[NV4097_SET_SURFACE_COLOR_BOFFSET];
	m_previous_address_c = rsx::method_registers[NV4097_SET_SURFACE_COLOR_COFFSET];
	m_previous_address_d = rsx::method_registers[NV4097_SET_SURFACE_COLOR_DOFFSET];
	m_previous_address_z = rsx::method_registers[NV4097_SET_SURFACE_ZETA_OFFSET];

	u32 m_context_dma_color_a = rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_A];
	u32 m_context_dma_color_b = rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_B];
	u32 m_context_dma_color_c = rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_C];
	u32 m_context_dma_color_d = rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_D];
	u32 m_context_dma_z = rsx::method_registers[NV4097_SET_CONTEXT_DMA_ZETA];

	// FBO location has changed, previous data might be copied
	u32 address_a = m_context_dma_color_a ? rsx::get_address(m_previous_address_a, m_context_dma_color_a - 0xfeed0000) : 0;
	u32 address_b = m_context_dma_color_b ? rsx::get_address(m_previous_address_b, m_context_dma_color_b - 0xfeed0000) : 0;
	u32 address_c = m_context_dma_color_c ? rsx::get_address(m_previous_address_c, m_context_dma_color_c - 0xfeed0000) : 0;
	u32 address_d = m_context_dma_color_d ? rsx::get_address(m_previous_address_d, m_context_dma_color_d - 0xfeed0000) : 0;
	u32 address_z = m_context_dma_z ? rsx::get_address(m_previous_address_z, m_context_dma_z - 0xfeed0000) : 0;

	// Make previous RTTs sampleable
	for (unsigned i = 0; i < 4; i++)
	{
		if (m_rtts.m_currentlyBoundRenderTargets[i] == nullptr)
			continue;
		copycmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rtts.m_currentlyBoundRenderTargets[i], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	}
	// Same for depth buffer
	if (m_rtts.m_currentlyBoundDepthStencil != nullptr)
		copycmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rtts.m_currentlyBoundDepthStencil, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));

	memset(m_rtts.m_currentlyBoundRenderTargetsAddress, 0, 4 * sizeof(u32));
	memset(m_rtts.m_currentlyBoundRenderTargets, 0, 4 * sizeof(ID3D12Resource *));
	m_rtts.m_currentlyBoundDepthStencil = nullptr;
	m_rtts.m_currentlyBoundDepthStencilAddress = 0;


	D3D12_CPU_DESCRIPTOR_HANDLE Handle = m_rtts.m_renderTargetsDescriptorsHeap->GetCPUDescriptorHandleForHeapStart();
	size_t g_RTTIncrement = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	DXGI_FORMAT dxgiFormat;
	switch (m_surface.color_format)
	{
	case CELL_GCM_SURFACE_A8R8G8B8:
		dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case CELL_GCM_SURFACE_F_W16Z16Y16X16:
		dxgiFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		break;
	}
	D3D12_RENDER_TARGET_VIEW_DESC rttViewDesc = {};
	rttViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rttViewDesc.Format = dxgiFormat;

	u32 clear_color = rsx::method_registers[NV4097_SET_COLOR_CLEAR_VALUE];
	u8 clear_a = clear_color >> 24;
	u8 clear_r = clear_color >> 16;
	u8 clear_g = clear_color >> 8;
	u8 clear_b = clear_color;
	std::array<float, 4> clearColor =
	{
		clear_r / 255.0f,
		clear_g / 255.0f,
		clear_b / 255.0f,
		clear_a / 255.0f
	};

	switch (rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET])
	{
	case CELL_GCM_SURFACE_TARGET_0:
	{
		ID3D12Resource *rttA = m_rtts.bindAddressAsRenderTargets(m_device.Get(), copycmdlist, 0, address_a, clip_width, clip_height, m_surface.color_format,
			clearColor);
		m_device->CreateRenderTargetView(rttA, &rttViewDesc, Handle);
		break;
	}
	case CELL_GCM_SURFACE_TARGET_1:
	{
		ID3D12Resource *rttB = m_rtts.bindAddressAsRenderTargets(m_device.Get(), copycmdlist, 0, address_b, clip_width, clip_height, m_surface.color_format,
			clearColor);
		m_device->CreateRenderTargetView(rttB, &rttViewDesc, Handle);
		break;
	}
	case CELL_GCM_SURFACE_TARGET_MRT1:
	{
		ID3D12Resource *rttA = m_rtts.bindAddressAsRenderTargets(m_device.Get(), copycmdlist, 0, address_a, clip_width, clip_height, m_surface.color_format,
			clearColor);
		m_device->CreateRenderTargetView(rttA, &rttViewDesc, Handle);
		Handle.ptr += g_RTTIncrement;
		ID3D12Resource *rttB = m_rtts.bindAddressAsRenderTargets(m_device.Get(), copycmdlist, 1, address_b, clip_width, clip_height, m_surface.color_format,
			clearColor);
		m_device->CreateRenderTargetView(rttB, &rttViewDesc, Handle);
	}
	break;
	case CELL_GCM_SURFACE_TARGET_MRT2:
	{
		ID3D12Resource *rttA = m_rtts.bindAddressAsRenderTargets(m_device.Get(), copycmdlist, 0, address_a, clip_width, clip_height, m_surface.color_format,
			clearColor);
		m_device->CreateRenderTargetView(rttA, &rttViewDesc, Handle);
		Handle.ptr += g_RTTIncrement;
		ID3D12Resource *rttB = m_rtts.bindAddressAsRenderTargets(m_device.Get(), copycmdlist, 1, address_b, clip_width, clip_height, m_surface.color_format,
			clearColor);
		m_device->CreateRenderTargetView(rttB, &rttViewDesc, Handle);
		Handle.ptr += g_RTTIncrement;
		ID3D12Resource *rttC = m_rtts.bindAddressAsRenderTargets(m_device.Get(), copycmdlist, 2, address_c, clip_width, clip_height, m_surface.color_format,
			clearColor);
		m_device->CreateRenderTargetView(rttC, &rttViewDesc, Handle);
		break;
	}
	case CELL_GCM_SURFACE_TARGET_MRT3:
	{
		ID3D12Resource *rttA = m_rtts.bindAddressAsRenderTargets(m_device.Get(), copycmdlist, 0, address_a, clip_width, clip_height, m_surface.color_format,
			clearColor);
		m_device->CreateRenderTargetView(rttA, &rttViewDesc, Handle);
		Handle.ptr += g_RTTIncrement;
		ID3D12Resource *rttB = m_rtts.bindAddressAsRenderTargets(m_device.Get(), copycmdlist, 1, address_b, clip_width, clip_height, m_surface.color_format,
			clearColor);
		m_device->CreateRenderTargetView(rttB, &rttViewDesc, Handle);
		Handle.ptr += g_RTTIncrement;
		ID3D12Resource *rttC = m_rtts.bindAddressAsRenderTargets(m_device.Get(), copycmdlist, 2, address_c, clip_width, clip_height, m_surface.color_format,
			clearColor);
		m_device->CreateRenderTargetView(rttC, &rttViewDesc, Handle);
		Handle.ptr += g_RTTIncrement;
		ID3D12Resource *rttD = m_rtts.bindAddressAsRenderTargets(m_device.Get(), copycmdlist, 3, address_d, clip_width, clip_height, m_surface.color_format,
			clearColor);
		m_device->CreateRenderTargetView(rttD, &rttViewDesc, Handle);
		break;
	}
	}

	ID3D12Resource *ds = m_rtts.bindAddressAsDepthStencil(m_device.Get(), copycmdlist, address_z, clip_width, clip_height, m_surface.depth_format, 1., 0);

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
	switch (m_surface.depth_format)
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
		LOG_ERROR(RSX, "Bad depth format! (%d)", m_surface.depth_format);
		assert(0);
	}
	depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	m_device->CreateDepthStencilView(ds, &depthStencilViewDesc, m_rtts.m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

ID3D12Resource *RenderTargets::bindAddressAsRenderTargets(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, size_t slot, u32 address,
	size_t width, size_t height, u8 surfaceColorFormat, const std::array<float, 4> &clearColor)
{
	ID3D12Resource* rtt;
	auto It = m_renderTargets.find(address);
	// TODO: Check if sizes match
	if (It != m_renderTargets.end())
	{
		rtt = It->second;
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rtt, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}
	else
	{
		LOG_WARNING(RSX, "Creating RTT");
		DXGI_FORMAT dxgiFormat;
		switch (surfaceColorFormat)
		{
		case CELL_GCM_SURFACE_A8R8G8B8:
			dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			break;
		case CELL_GCM_SURFACE_F_W16Z16Y16X16:
			dxgiFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
			break;
		}
		D3D12_CLEAR_VALUE clearColorValue = {};
		clearColorValue.Format = dxgiFormat;
		clearColorValue.Color[0] = clearColor[0];
		clearColorValue.Color[1] = clearColor[1];
		clearColorValue.Color[2] = clearColor[2];
		clearColorValue.Color[3] = clearColor[3];

		device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(dxgiFormat, (UINT)width, (UINT)height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
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
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ds, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));
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

		device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(dxgiFormat, (UINT)width, (UINT)height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
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