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

D3D12RenderTargetSets::D3D12RenderTargetSets(ID3D12Device *device, u8 surfaceDepthFormat, size_t width, size_t height, float clearColor[4], float clearDepth)
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.NumDescriptors = 1;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_depthStencilDescriptorHeap));

	descriptorHeapDesc.NumDescriptors = 4;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_rttDescriptorHeap));

	D3D12_CLEAR_VALUE clearDepthValue = {};
	clearDepthValue.DepthStencil.Depth = clearDepth;

	// Every resource are committed for simplicity, later we could use heap
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	resourceDesc.Width = (UINT)width;
	resourceDesc.Height = (UINT)height;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.DepthOrArraySize = 1;

	switch (surfaceDepthFormat)
	{
	case 0:
		break;
	case CELL_GCM_SURFACE_Z16:
		resourceDesc.Format = DXGI_FORMAT_R16_TYPELESS;
		clearDepthValue.Format = DXGI_FORMAT_D16_UNORM;
		break;
	case CELL_GCM_SURFACE_Z24S8:
		resourceDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		clearDepthValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		break;
	default:
		LOG_ERROR(RSX, "Bad depth format! (%d)", surfaceDepthFormat);
		assert(0);
	}

	device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearDepthValue,
		IID_PPV_ARGS(&m_depthStencilTexture)
		);
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
	switch (surfaceDepthFormat)
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
		LOG_ERROR(RSX, "Bad depth format! (%d)", surfaceDepthFormat);
		assert(0);
	}
	depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(m_depthStencilTexture, &depthStencilViewDesc, m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_CLEAR_VALUE clearColorValue = {};
	clearColorValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clearColorValue.Color[0] = clearColor[0];
	clearColorValue.Color[1] = clearColor[1];
	clearColorValue.Color[2] = clearColor[2];
	clearColorValue.Color[3] = clearColor[3];
	g_RTTIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE Handle = m_rttDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (int i = 0; i < 4; ++i)
	{
		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		resourceDesc.Width = (UINT)width;
		resourceDesc.Height = (UINT)height;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		resourceDesc.SampleDesc.Count = 1;

		device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&clearColorValue,
			IID_PPV_ARGS(&m_rtts[i])
			);

		D3D12_RENDER_TARGET_VIEW_DESC rttViewDesc = {};
		rttViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rttViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

		device->CreateRenderTargetView(m_rtts[i], &rttViewDesc, Handle);
		Handle.ptr += g_RTTIncrement;
	}

	/*if (!m_set_surface_clip_horizontal)
	{
		m_surface_clip_x = 0;
		m_surface_clip_w = RSXThread::m_width;
	}

	if (!m_set_surface_clip_vertical)
	{
		m_surface_clip_y = 0;
		m_surface_clip_h = RSXThread::m_height;
	}*/
}

D3D12RenderTargetSets::~D3D12RenderTargetSets()
{
	for (unsigned i = 0; i < 4; i++)
		m_rtts[i]->Release();
	m_rttDescriptorHeap->Release();
	m_depthStencilTexture->Release();
	m_depthStencilDescriptorHeap->Release();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12RenderTargetSets::getRTTCPUHandle(u8 baseFBO) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE Handle = m_rttDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	Handle.ptr += baseFBO * g_RTTIncrement;
	return Handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12RenderTargetSets::getDSVCPUHandle() const
{
	return m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}
ID3D12Resource * D3D12RenderTargetSets::getRenderTargetTexture(u8 Id) const
{
	return m_rtts[Id];
}
ID3D12Resource * D3D12RenderTargetSets::getDepthStencilTexture() const
{
	return m_depthStencilTexture;
}
#endif