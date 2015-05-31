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