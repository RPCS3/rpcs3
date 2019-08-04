#pragma once

#define INCOMPLETE_SURFACE_CACHE_IMPL

#include <utility>
#include <d3d12.h>
#include "d3dx12.h"

#include "D3D12Formats.h"
#include "D3D12MemoryHelpers.h"

namespace rsx
{
namespace utility
{
	std::vector<u8> get_rtt_indexes(surface_target color_target);
	size_t get_aligned_pitch(surface_color_format format, u32 width);
	size_t get_packed_pitch(surface_color_format format, u32 width);
}

template<typename Traits>
struct surface_store_deprecated
{
	template<typename T, typename U>
	void copy_pitched_src_to_dst(gsl::span<T> dest, gsl::span<const U> src, size_t src_pitch_in_bytes, size_t width, size_t height)
	{
		for (unsigned row = 0; row < height; row++)
		{
			for (unsigned col = 0; col < width; col++)
				dest[col] = src[col];
			src = src.subspan(src_pitch_in_bytes / sizeof(U));
			dest = dest.subspan(width);
		}
	}

public:
	using surface_storage_type = typename Traits::surface_storage_type;
	using surface_type = typename Traits::surface_type;
	using command_list_type = typename Traits::command_list_type;
	using download_buffer_object = typename Traits::download_buffer_object;

protected:
	std::unordered_map<u32, surface_storage_type> m_render_targets_storage = {};
	std::unordered_map<u32, surface_storage_type> m_depth_stencil_storage = {};

public:
	std::pair<u8, u8> m_bound_render_targets_config = {};
	std::array<std::pair<u32, surface_type>, 4> m_bound_render_targets = {};
	std::pair<u32, surface_type> m_bound_depth_stencil = {};

	std::list<surface_storage_type> invalidated_resources;

	surface_store_deprecated() = default;
	~surface_store_deprecated() = default;
	surface_store_deprecated(const surface_store_deprecated&) = delete;

protected:
	/**
	* If render target already exists at address, issue state change operation on cmdList.
	* Otherwise create one with width, height, clearColor info.
	* returns the corresponding render target resource.
	*/
	template <typename ...Args>
	surface_type bind_address_as_render_targets(
		command_list_type command_list,
		u32 address,
		surface_color_format color_format,
		surface_antialiasing antialias,
		size_t width, size_t height, size_t pitch,
		Args&&... extra_params)
	{
		// Check if render target already exists
		auto It = m_render_targets_storage.find(address);
		if (It != m_render_targets_storage.end())
		{
			surface_storage_type &rtt = It->second;
			if (Traits::rtt_has_format_width_height(rtt, color_format, width, height))
			{
				Traits::prepare_rtt_for_drawing(command_list, Traits::get(rtt));
				return Traits::get(rtt);
			}

			invalidated_resources.push_back(std::move(rtt));
			m_render_targets_storage.erase(It);
		}

		m_render_targets_storage[address] = Traits::create_new_surface(address, color_format, width, height, pitch, std::forward<Args>(extra_params)...);
		return Traits::get(m_render_targets_storage[address]);
	}

	template <typename ...Args>
	surface_type bind_address_as_depth_stencil(
		command_list_type command_list,
		u32 address,
		surface_depth_format depth_format,
		surface_antialiasing antialias,
		size_t width, size_t height, size_t pitch,
		Args&&... extra_params)
	{
		auto It = m_depth_stencil_storage.find(address);
		if (It != m_depth_stencil_storage.end())
		{
			surface_storage_type &ds = It->second;
			if (Traits::ds_has_format_width_height(ds, depth_format, width, height))
			{
				Traits::prepare_ds_for_drawing(command_list, Traits::get(ds));
				return Traits::get(ds);
			}

			invalidated_resources.push_back(std::move(ds));
			m_depth_stencil_storage.erase(It);
		}

		m_depth_stencil_storage[address] = Traits::create_new_surface(address, depth_format, width, height, pitch, std::forward<Args>(extra_params)...);
		return Traits::get(m_depth_stencil_storage[address]);
	}
public:
	/**
		* Update bound color and depth surface.
		* Must be called everytime surface format, clip, or addresses changes.
		*/
	template <typename ...Args>
	void prepare_render_target(
		command_list_type command_list,
		surface_color_format color_format, surface_depth_format depth_format,
		u32 clip_horizontal_reg, u32 clip_vertical_reg,
		surface_target set_surface_target,
		surface_antialiasing antialias,
		const std::array<u32, 4> &surface_addresses, u32 address_z,
		const std::array<u32, 4> &surface_pitch, u32 zeta_pitch,
		Args&&... extra_params)
	{
		u32 clip_width = clip_horizontal_reg;
		u32 clip_height = clip_vertical_reg;

		// Make previous RTTs sampleable
		for (int i = m_bound_render_targets_config.first, count = 0;
			count < m_bound_render_targets_config.second;
			++i, ++count)
		{
			auto &rtt = m_bound_render_targets[i];
			Traits::prepare_rtt_for_sampling(command_list, std::get<1>(rtt));
			rtt = std::make_pair(0, nullptr);
		}

		const auto rtt_indices = utility::get_rtt_indexes(set_surface_target);
		if (LIKELY(!rtt_indices.empty()))
		{
			m_bound_render_targets_config = { rtt_indices.front(), 0 };

			// Create/Reuse requested rtts
			for (u8 surface_index : rtt_indices)
			{
				if (surface_addresses[surface_index] == 0)
					continue;

				m_bound_render_targets[surface_index] = std::make_pair(surface_addresses[surface_index],
					bind_address_as_render_targets(command_list, surface_addresses[surface_index], color_format, antialias,
						clip_width, clip_height, surface_pitch[surface_index], std::forward<Args>(extra_params)...));

				m_bound_render_targets_config.second++;
			}
		}
		else
		{
			m_bound_render_targets_config = { 0, 0 };
		}

		// Same for depth buffer
		if (std::get<1>(m_bound_depth_stencil) != nullptr)
			Traits::prepare_ds_for_sampling(command_list, std::get<1>(m_bound_depth_stencil));

		m_bound_depth_stencil = std::make_pair(0, nullptr);

		if (!address_z)
			return;

		m_bound_depth_stencil = std::make_pair(address_z,
			bind_address_as_depth_stencil(command_list, address_z, depth_format, antialias,
				clip_width, clip_height, zeta_pitch, std::forward<Args>(extra_params)...));
	}

	/**
		* Search for given address in stored color surface
		* Return an empty surface_type otherwise.
		*/
	surface_type get_texture_from_render_target_if_applicable(u32 address)
	{
		auto It = m_render_targets_storage.find(address);
		if (It != m_render_targets_storage.end())
			return Traits::get(It->second);
		return surface_type();
	}

	/**
	* Search for given address in stored depth stencil surface
	* Return an empty surface_type otherwise.
	*/
	surface_type get_texture_from_depth_stencil_if_applicable(u32 address)
	{
		auto It = m_depth_stencil_storage.find(address);
		if (It != m_depth_stencil_storage.end())
			return Traits::get(It->second);
		return surface_type();
	}

	/**
		* Get bound color surface raw data.
		*/
	template <typename... Args>
	std::array<std::vector<gsl::byte>, 4> get_render_targets_data(
		surface_color_format color_format, size_t width, size_t height,
		Args&& ...args
	)
	{
		return {};
	}

	/**
		* Get bound color surface raw data.
		*/
	template <typename... Args>
	std::array<std::vector<gsl::byte>, 2> get_depth_stencil_data(
		surface_depth_format depth_format, size_t width, size_t height,
		Args&& ...args
	)
	{
		return {};
	}
};

struct render_target_traits
{
	using surface_storage_type = ComPtr<ID3D12Resource>;
	using surface_type = ID3D12Resource*;
	using command_list_type = ID3D12GraphicsCommandList*;
	using download_buffer_object = std::tuple<size_t, size_t, size_t, ComPtr<ID3D12Fence>, HANDLE>; // heap offset, size, last_put_pos, fence, handle

	//TODO: Move this somewhere else
	bool depth_is_dirty = false;

	static
	ComPtr<ID3D12Resource> create_new_surface(
		u32 address,
		surface_color_format color_format, size_t width, size_t height, size_t /*pitch*/,
		ID3D12Device* device, const std::array<float, 4> &clear_color, float, u8)
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
	void prepare_rtt_for_drawing(
		ID3D12GraphicsCommandList* command_list,
		ID3D12Resource* rtt)
	{
		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rtt, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}

	static
	void prepare_rtt_for_sampling(
		ID3D12GraphicsCommandList* command_list,
		ID3D12Resource* rtt)
	{
		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rtt, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	static
	ComPtr<ID3D12Resource> create_new_surface(
		u32 address,
		surface_depth_format surfaceDepthFormat, size_t width, size_t height, size_t /*pitch*/,
		ID3D12Device* device, const std::array<float, 4>& , float clear_depth, u8 clear_stencil)
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
		ID3D12GraphicsCommandList* command_list,
		ID3D12Resource* ds)
	{
		// set the resource as depth write
		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ds, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	}

	static
	void prepare_ds_for_sampling(
		ID3D12GraphicsCommandList* command_list,
		ID3D12Resource* ds)
	{
		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ds, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

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
		ID3D12Resource* rtt,
		surface_color_format color_format, size_t width, size_t height,
		ID3D12Device* device, ID3D12CommandQueue* command_queue, d3d12_data_heap &readback_heap, resource_storage &res_store
		)
	{
		return {};
	}

	static
	std::tuple<size_t, size_t, size_t, ComPtr<ID3D12Fence>, HANDLE> issue_depth_download_command(
		ID3D12Resource* ds,
		surface_depth_format depth_format, size_t width, size_t height,
		ID3D12Device* device, ID3D12CommandQueue* command_queue, d3d12_data_heap &readback_heap, resource_storage &res_store
			)
	{
		return {};
	}

	static
		std::tuple<size_t, size_t, size_t, ComPtr<ID3D12Fence>, HANDLE> issue_stencil_download_command(
			ID3D12Resource* stencil,
			size_t width, size_t height,
			ID3D12Device* device, ID3D12CommandQueue* command_queue, d3d12_data_heap &readback_heap, resource_storage &res_store
			)
	{
		return {};
	}

	static
	gsl::span<const gsl::byte> map_downloaded_buffer(const std::tuple<size_t, size_t, size_t, ComPtr<ID3D12Fence>, HANDLE> &sync_data,
		ID3D12Device*, ID3D12CommandQueue*, d3d12_data_heap &readback_heap, resource_storage&)
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
		ID3D12Device*, ID3D12CommandQueue*, d3d12_data_heap &readback_heap, resource_storage&)
	{
		readback_heap.unmap();
	}

	static ID3D12Resource* get(const ComPtr<ID3D12Resource> &in)
	{
		return in.Get();
	}
};

struct render_targets : public rsx::surface_store_deprecated<render_target_traits>
{
	INT g_descriptor_stride_rtv;

	D3D12_CPU_DESCRIPTOR_HANDLE current_rtts_handle;
	D3D12_CPU_DESCRIPTOR_HANDLE current_ds_handle;

	void init(ID3D12Device *device);
};

}
