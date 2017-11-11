#pragma once

#include <utility>
#include <d3d12.h>
#include "d3dx12.h"

#include "D3D12Formats.h"
#include "D3D12MemoryHelpers.h"
#include "../Common/surface_store.h"

namespace rsx
{

struct render_target_traits
{
	using surface_storage_type = ComPtr<ID3D12Resource>;
	using surface_type = ID3D12Resource*;
	using command_list_type = gsl::not_null<ID3D12GraphicsCommandList*>;
	using download_buffer_object = std::tuple<size_t, size_t, size_t, ComPtr<ID3D12Fence>, HANDLE>; // heap offset, size, last_put_pos, fence, handle

	//TODO: Move this somewhere else
	bool depth_is_dirty = false;

	static
	ComPtr<ID3D12Resource> create_new_surface(
		u32 address,
		surface_color_format color_format, size_t width, size_t height,
		ID3D12Resource* /*old*/,
		gsl::not_null<ID3D12Device*> device, const std::array<float, 4> &clear_color, float, u8)
	{
		DXGI_FORMAT dxgi_format = get_color_surface_format(color_format);
		ComPtr<ID3D12Resource> rtt;
		LOG_WARNING(RSX, "Creating RTT");

		D3D12_CLEAR_VALUE clear_color_value = {};
		clear_color_value.Format = dxgi_format;
		clear_color_value.Color[0] = clear_color[0];
		clear_color_value.Color[1] = clear_color[1];
		clear_color_value.Color[2] = clear_color[2];
		clear_color_value.Color[3] = clear_color[3];

		device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(dxgi_format, (UINT)width, (UINT)height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&clear_color_value,
			IID_PPV_ARGS(rtt.GetAddressOf())
			);

		std::wstring name = L"rtt_@" + std::to_wstring(address);
		rtt->SetName(name.c_str());

		return rtt;
	}

	static
	void get_surface_info(ID3D12Resource *surface, rsx::surface_format_info *info)
	{
		//TODO
		auto desc = surface->GetDesc();
		info->rsx_pitch = static_cast<u16>(desc.Width);
		info->native_pitch = static_cast<u16>(desc.Width);
		info->surface_width = static_cast<u32>(desc.Width);
		info->surface_height = static_cast<u32>(desc.Height);
		info->bpp = 1;
	}

	static
	void prepare_rtt_for_drawing(
		gsl::not_null<ID3D12GraphicsCommandList*> command_list,
		ID3D12Resource* rtt)
	{
		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rtt, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}

	static
	void prepare_rtt_for_sampling(
		gsl::not_null<ID3D12GraphicsCommandList*> command_list,
		ID3D12Resource* rtt)
	{
		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rtt, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	static
	ComPtr<ID3D12Resource> create_new_surface(
		u32 address,
		surface_depth_format surfaceDepthFormat, size_t width, size_t height,
		ID3D12Resource* /*old*/,
		gsl::not_null<ID3D12Device*> device, const std::array<float, 4>& , float clear_depth, u8 clear_stencil)
	{
		D3D12_CLEAR_VALUE clear_depth_value = {};
		clear_depth_value.DepthStencil.Depth = clear_depth;
		clear_depth_value.DepthStencil.Stencil = clear_stencil;

		DXGI_FORMAT dxgi_format = get_depth_stencil_typeless_surface_format(surfaceDepthFormat);
		clear_depth_value.Format = get_depth_stencil_surface_clear_format(surfaceDepthFormat);

		ComPtr<ID3D12Resource> new_depth_stencil;
		device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(dxgi_format, (UINT)width, (UINT)height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clear_depth_value,
			IID_PPV_ARGS(new_depth_stencil.GetAddressOf())
			);
		std::wstring name = L"ds_@" + std::to_wstring(address);
		new_depth_stencil->SetName(name.c_str());

		return new_depth_stencil;
	}

	static
	void prepare_ds_for_drawing(
		gsl::not_null<ID3D12GraphicsCommandList*> command_list,
		ID3D12Resource* ds)
	{
		// set the resource as depth write
		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ds, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	}

	static
	void prepare_ds_for_sampling(
		gsl::not_null<ID3D12GraphicsCommandList*> command_list,
		ID3D12Resource* ds)
	{
		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ds, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	static
	void invalidate_rtt_surface_contents(
		gsl::not_null<ID3D12GraphicsCommandList*>,
		ID3D12Resource*, ID3D12Resource*, bool)
	{}

	static
	void invalidate_depth_surface_contents(
		gsl::not_null<ID3D12GraphicsCommandList*>,
		ID3D12Resource*, ID3D12Resource*, bool)
	{
		//TODO
	}

	static
	void notify_surface_invalidated(const ComPtr<ID3D12Resource>&)
	{}

	static
	bool rtt_has_format_width_height(const ComPtr<ID3D12Resource> &rtt, surface_color_format surface_color_format, size_t width, size_t height, bool=false)
	{
		DXGI_FORMAT dxgi_format = get_color_surface_format(surface_color_format);
		return rtt->GetDesc().Format == dxgi_format && rtt->GetDesc().Width == width && rtt->GetDesc().Height == height;
	}

	static
	bool ds_has_format_width_height(const ComPtr<ID3D12Resource> &rtt, surface_depth_format, size_t width, size_t height, bool=false)
	{
		//TODO: Check format
		return rtt->GetDesc().Width == width && rtt->GetDesc().Height == height;
	}

	static
	std::tuple<size_t, size_t, size_t, ComPtr<ID3D12Fence>, HANDLE> issue_download_command(
		gsl::not_null<ID3D12Resource*> rtt,
		surface_color_format color_format, size_t width, size_t height,
		gsl::not_null<ID3D12Device*> device, gsl::not_null<ID3D12CommandQueue*> command_queue, d3d12_data_heap &readback_heap, resource_storage &res_store
		)
	{
		ID3D12GraphicsCommandList* command_list = res_store.command_list.Get();
		DXGI_FORMAT dxgi_format = get_color_surface_format(color_format);
		size_t row_pitch = rsx::utility::get_aligned_pitch(color_format, ::narrow<u32>(width));

		size_t buffer_size = row_pitch * height;
		size_t heap_offset = readback_heap.alloc<D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rtt, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

		command_list->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(readback_heap.get_heap(), { heap_offset,{ dxgi_format, (UINT)width, (UINT)height, 1, (UINT)row_pitch } }), 0, 0, 0,
			&CD3DX12_TEXTURE_COPY_LOCATION(rtt, 0), nullptr);
		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rtt, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

		CHECK_HRESULT(command_list->Close());
		command_queue->ExecuteCommandLists(1, (ID3D12CommandList**)res_store.command_list.GetAddressOf());
		res_store.set_new_command_list();

		ComPtr<ID3D12Fence> fence;
		CHECK_HRESULT(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));
		HANDLE handle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		fence->SetEventOnCompletion(1, handle);
		command_queue->Signal(fence.Get(), 1);

		return std::make_tuple(heap_offset, buffer_size, readback_heap.get_current_put_pos_minus_one(), fence, handle);
	}

	static
	std::tuple<size_t, size_t, size_t, ComPtr<ID3D12Fence>, HANDLE> issue_depth_download_command(
		gsl::not_null<ID3D12Resource*> ds,
		surface_depth_format depth_format, size_t width, size_t height,
		gsl::not_null<ID3D12Device*> device, gsl::not_null<ID3D12CommandQueue*> command_queue, d3d12_data_heap &readback_heap, resource_storage &res_store
			)
	{
		ID3D12GraphicsCommandList* command_list = res_store.command_list.Get();
		DXGI_FORMAT dxgi_format = (depth_format == surface_depth_format::z24s8) ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_R16_TYPELESS;

		size_t row_pitch = align(width * 4, 256);
		size_t buffer_size = row_pitch * height;
		size_t heap_offset = readback_heap.alloc<D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ds, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_SOURCE));

		command_list->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(readback_heap.get_heap(), { heap_offset,{ dxgi_format, (UINT)width, (UINT)height, 1, (UINT)row_pitch } }), 0, 0, 0,
			&CD3DX12_TEXTURE_COPY_LOCATION(ds, 0), nullptr);
		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ds, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));

		CHECK_HRESULT(command_list->Close());
		command_queue->ExecuteCommandLists(1, (ID3D12CommandList**)res_store.command_list.GetAddressOf());
		res_store.set_new_command_list();

		ComPtr<ID3D12Fence> fence;
		CHECK_HRESULT(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));
		HANDLE handle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		fence->SetEventOnCompletion(1, handle);
		command_queue->Signal(fence.Get(), 1);

		return std::make_tuple(heap_offset, buffer_size, readback_heap.get_current_put_pos_minus_one(), fence, handle);
	}

	static
		std::tuple<size_t, size_t, size_t, ComPtr<ID3D12Fence>, HANDLE> issue_stencil_download_command(
			gsl::not_null<ID3D12Resource*> stencil,
			size_t width, size_t height,
			gsl::not_null<ID3D12Device*> device, gsl::not_null<ID3D12CommandQueue*> command_queue, d3d12_data_heap &readback_heap, resource_storage &res_store
			)
	{
		ID3D12GraphicsCommandList* command_list = res_store.command_list.Get();

		size_t row_pitch = align(width, 256);
		size_t buffer_size = row_pitch * height;
		size_t heap_offset = readback_heap.alloc<D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(stencil, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_SOURCE));

		command_list->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(readback_heap.get_heap(), { heap_offset,{ DXGI_FORMAT_R8_TYPELESS, (UINT)width, (UINT)height, 1, (UINT)row_pitch } }), 0, 0, 0,
			&CD3DX12_TEXTURE_COPY_LOCATION(stencil, 1), nullptr);
		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(stencil, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));

		CHECK_HRESULT(command_list->Close());
		command_queue->ExecuteCommandLists(1, (ID3D12CommandList**)res_store.command_list.GetAddressOf());
		res_store.set_new_command_list();

		ComPtr<ID3D12Fence> fence;
		CHECK_HRESULT(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));
		HANDLE handle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		fence->SetEventOnCompletion(1, handle);
		command_queue->Signal(fence.Get(), 1);

		return std::make_tuple(heap_offset, buffer_size, readback_heap.get_current_put_pos_minus_one(), fence, handle);
	}

	static
	gsl::span<const gsl::byte> map_downloaded_buffer(const std::tuple<size_t, size_t, size_t, ComPtr<ID3D12Fence>, HANDLE> &sync_data,
		gsl::not_null<ID3D12Device*>, gsl::not_null<ID3D12CommandQueue*>, d3d12_data_heap &readback_heap, resource_storage&)
	{
		size_t offset;
		size_t buffer_size;
		size_t current_put_pos_minus_one;
		HANDLE handle;
		std::tie(offset, buffer_size, current_put_pos_minus_one, std::ignore, handle) = sync_data;
		WaitForSingleObjectEx(handle, INFINITE, FALSE);
		CloseHandle(handle);

		readback_heap.m_get_pos = current_put_pos_minus_one;
		const gsl::byte *mapped_buffer = readback_heap.map<const gsl::byte>(CD3DX12_RANGE(offset, offset + buffer_size));
		return { mapped_buffer , ::narrow<int>(buffer_size) };
	}

	static
	void unmap_downloaded_buffer(const std::tuple<size_t, size_t, size_t, ComPtr<ID3D12Fence>, HANDLE> &,
		gsl::not_null<ID3D12Device*>, gsl::not_null<ID3D12CommandQueue*>, d3d12_data_heap &readback_heap, resource_storage&)
	{
		readback_heap.unmap();
	}

	static ID3D12Resource* get(const ComPtr<ID3D12Resource> &in)
	{
		return in.Get();
	}
};

struct render_targets : public rsx::surface_store<render_target_traits>
{
	INT g_descriptor_stride_rtv;

	D3D12_CPU_DESCRIPTOR_HANDLE current_rtts_handle;
	D3D12_CPU_DESCRIPTOR_HANDLE current_ds_handle;

	void init(ID3D12Device *device);
};

}
