#pragma once

#if defined(DX12_SUPPORT)
#include <d3d12.h>

struct RenderTargets
{
	std::unordered_map<u32, ID3D12Resource *> m_renderTargets;
	ID3D12Resource *m_currentlyBoundRenderTargets[4];
	u32 m_currentlyBoundRenderTargetsAddress[4];
	std::unordered_map<u32, ID3D12Resource *> m_depthStencil;
	ID3D12Resource *m_currentlyBoundDepthStencil;
	u32 m_currentlyBoundDepthStencilAddress;
	ID3D12DescriptorHeap *m_renderTargetsDescriptorsHeap;
	ID3D12DescriptorHeap *m_depthStencilDescriptorHeap;

	/**
	 * If render target already exists at address, issue state change operation on cmdList.
	 * Otherwise create one with width, height, clearColor info.
	 * returns the corresponding render target resource.
	 */
	ID3D12Resource *bindAddressAsRenderTargets(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, size_t slot, u32 address,
		size_t width, size_t height, float clearColorR, float clearColorG, float clearColorB, float clearColorA);

	ID3D12Resource *bindAddressAsDepthStencil(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, u32 address,
		size_t width, size_t height, u8 surfaceDepthFormat, float depthClear, u8 stencilClear);

	void Init(ID3D12Device *device);
	void Release();
};
#endif