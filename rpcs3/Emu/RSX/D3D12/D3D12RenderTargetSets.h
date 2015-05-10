#pragma once

#if defined(DX12_SUPPORT)
#include <d3d12.h>

/**
 * Class that embeds a RenderTargetDescriptor view and eventually a DepthStencil Descriptor View.
 * Used to imitate OpenGL FrameBuffer concept.
 */
class D3D12RenderTargetSets
{
	size_t g_RTTIncrement;
	ID3D12Resource *m_depthStencilTexture;
	ID3D12Resource *m_rtts[4];
	ID3D12DescriptorHeap *m_rttDescriptorHeap;
	ID3D12DescriptorHeap *m_depthStencilDescriptorHeap;
public:
	D3D12RenderTargetSets(ID3D12Device *device, u8 surfaceDepthFormat, size_t width, size_t height);
	~D3D12RenderTargetSets();
	/**
	 * Return the base descriptor address for the give surface target.
	 * All rtt's view descriptor are contigous.
	 */
	D3D12_CPU_DESCRIPTOR_HANDLE getRTTCPUHandle(u8 baseFBO) const;
	D3D12_CPU_DESCRIPTOR_HANDLE getDSVCPUHandle() const;
	ID3D12Resource *getRenderTargetTexture(u8 Id) const;
};
#endif