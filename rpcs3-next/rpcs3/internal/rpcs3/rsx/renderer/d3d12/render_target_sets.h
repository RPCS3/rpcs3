#pragma once

#include <d3d12.h>

struct render_targets
{
	INT g_descriptor_stride_rtv;
	std::unordered_map<u32, ComPtr<ID3D12Resource> > render_targets_storage;
	ID3D12Resource *bound_render_targets[4];
	u32 bound_render_targets_address[4];
	std::unordered_map<u32, ComPtr<ID3D12Resource> > depth_stencil_storage;
	ID3D12Resource *bound_depth_stencil;
	u32 bound_depth_stencil_address;

	size_t bind_render_targets(ID3D12Device *, u32 color_format, D3D12_CPU_DESCRIPTOR_HANDLE);
	size_t bind_depth_stencil(ID3D12Device *, u32 depth_format, D3D12_CPU_DESCRIPTOR_HANDLE);

	/**
	 * If render target already exists at address, issue state change operation on cmdList.
	 * Otherwise create one with width, height, clearColor info.
	 * returns the corresponding render target resource.
	 */
	ID3D12Resource *bind_address_as_render_targets(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, u32 address,
		size_t width, size_t height, u8 surfaceColorFormat, const std::array<float, 4> &clearColor, ComPtr<ID3D12Resource> &dirtyDS);

	ID3D12Resource *bind_address_as_depth_stencil(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, u32 address,
		size_t width, size_t height, u8 surfaceDepthFormat, float depthClear, u8 stencilClear, ComPtr<ID3D12Resource> &dirtyDS);

	void init(ID3D12Device *device);
};

