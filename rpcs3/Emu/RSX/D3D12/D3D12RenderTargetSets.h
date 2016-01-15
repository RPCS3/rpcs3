#pragma once

#include <utility>
#include <d3d12.h>
#include "d3dx12.h"

#include "D3D12Formats.h"
#include <gsl.h>

namespace rsx
{
	namespace
	{
		std::vector<u8> get_rtt_indexes(Surface_target color_target)
		{
			switch (color_target)
			{
			case Surface_target::none: return{};
			case Surface_target::surface_a: return{ 0 };
			case Surface_target::surface_b: return{ 1 };
			case Surface_target::surfaces_a_b: return{ 0, 1 };
			case Surface_target::surfaces_a_b_c: return{ 0, 1, 2 };
			case Surface_target::surfaces_a_b_c_d: return{ 0, 1, 2, 3 };
			}
			throw EXCEPTION("Wrong color_target");
		}
	}

	template<typename Traits>
	struct surface_store
	{
	private:
		using surface_storage_type = typename Traits::surface_storage_type;
		using surface_type = typename Traits::surface_type;
		using command_list_type = typename Traits::command_list_type;

		std::unordered_map<u32, surface_storage_type> m_render_targets_storage = {};
		std::unordered_map<u32, surface_storage_type> m_depth_stencil_storage = {};

	public:
		std::array<std::tuple<u32, surface_type>, 4> m_bound_render_targets = {};
		std::tuple<u32, surface_type> m_bound_depth_stencil = {};

		std::list<surface_storage_type> invalidated_resources;

		surface_store() = default;
		~surface_store() = default;
		surface_store(const surface_store&) = delete;
	private:
		/**
		* If render target already exists at address, issue state change operation on cmdList.
		* Otherwise create one with width, height, clearColor info.
		* returns the corresponding render target resource.
		*/
		template <typename ...Args>
		gsl::not_null<surface_type> bind_address_as_render_targets(
			command_list_type command_list,
			u32 address,
			Surface_color_format surface_color_format, size_t width, size_t height,
			Args&&... extra_params)
		{
			auto It = m_render_targets_storage.find(address);
			// TODO: Fix corner cases
			// This doesn't take overlapping surface(s) into account.
			// Invalidated surface(s) should also copy their content to the new resources.
			if (It != m_render_targets_storage.end())
			{
				surface_storage_type &rtt = It->second;
				if (Traits::rtt_has_format_width_height(rtt, surface_color_format, width, height))
				{
					Traits::prepare_rtt_for_drawing(command_list, rtt.Get());
					return rtt.Get();
				}
				invalidated_resources.push_back(std::move(rtt));
				m_render_targets_storage.erase(address);
			}

			m_render_targets_storage[address] = Traits::create_new_surface(address, surface_color_format, width, height, std::forward<Args>(extra_params)...);
			return m_render_targets_storage[address].Get();
		}

		template <typename ...Args>
		gsl::not_null<surface_type> bind_address_as_depth_stencil(
			command_list_type command_list,
			u32 address,
			Surface_depth_format surface_depth_format, size_t width, size_t height,
			Args&&... extra_params)
		{
			auto It = m_depth_stencil_storage.find(address);
			if (It != m_depth_stencil_storage.end())
			{
				surface_storage_type &ds = It->second;
				if (Traits::ds_has_format_width_height(ds, surface_depth_format, width, height))
				{
					Traits::prepare_ds_for_drawing(command_list, ds.Get());
					return ds.Get();
				}
				invalidated_resources.push_back(std::move(ds));
				m_depth_stencil_storage.erase(address);
			}

			m_depth_stencil_storage[address] = Traits::create_new_surface(address, surface_depth_format, width, height, std::forward<Args>(extra_params)...);
			return m_depth_stencil_storage[address].Get();
		}
	public:
		template <typename ...Args>
		void prepare_render_target(
			command_list_type command_list,
			u32 set_surface_format_reg,
			u32 clip_horizontal_reg, u32 clip_vertical_reg,
			Surface_target set_surface_target,
			const std::array<u32, 4> &surface_addresses, u32 address_z,
			Args&&... extra_params)
		{
			u32 clip_width = clip_horizontal_reg >> 16;
			u32 clip_height = clip_vertical_reg >> 16;
			u32 clip_x = clip_horizontal_reg;
			u32 clip_y = clip_vertical_reg;

			rsx::surface_info surface = {};
			surface.unpack(set_surface_format_reg);

			// Make previous RTTs sampleable
			for (std::tuple<u32, surface_type> &rtt : m_bound_render_targets)
			{
				if (std::get<1>(rtt) != nullptr)
					Traits::prepare_rtt_for_sampling(command_list, std::get<1>(rtt));
				rtt = std::make_tuple(0, nullptr);
			}

			// Create/Reuse requested rtts
			for (u8 surface_index : get_rtt_indexes(set_surface_target))
			{
				if (surface_addresses[surface_index] == 0)
					continue;

				m_bound_render_targets[surface_index] = std::make_tuple(surface_addresses[surface_index],
					bind_address_as_render_targets(command_list, surface_addresses[surface_index], surface.color_format, clip_width, clip_height, std::forward<Args>(extra_params)...));
			}

			// Same for depth buffer
			if (std::get<1>(m_bound_depth_stencil) != nullptr)
				Traits::prepare_ds_for_sampling(command_list, std::get<1>(m_bound_depth_stencil));
			m_bound_depth_stencil = std::make_tuple(0, nullptr);
			if (!address_z)
				return;
			m_bound_depth_stencil = std::make_tuple(address_z,
				bind_address_as_depth_stencil(command_list, address_z, surface.depth_format, clip_width, clip_height, std::forward<Args>(extra_params)...));
		}

		surface_type get_texture_from_render_target_if_applicable(u32 address)
		{
			// TODO: Handle texture that overlaps one (or several) surface.
			// Handle texture conversion
			// FIXME: Disgaea 3 loading screen seems to use a subset of a surface. It's not properly handled here.
			// Note: not const because conversions/resolve/... can happen
			auto It = m_render_targets_storage.find(address);
			if (It != m_render_targets_storage.end())
				return It->second.Get();
			return surface_type();
		}

		surface_type get_texture_from_depth_stencil_if_applicable(u32 address)
		{
			// TODO: Same as above although there wasn't any game using corner case for DS yet.
			auto It = m_depth_stencil_storage.find(address);
			if (It != m_depth_stencil_storage.end())
				return It->second.Get();
			return surface_type();
		}
	};
}

struct render_target_traits
{
	using surface_storage_type = ComPtr<ID3D12Resource>;
	using surface_type = ID3D12Resource*;
	using command_list_type = gsl::not_null<ID3D12GraphicsCommandList*>;

	static
	ComPtr<ID3D12Resource> create_new_surface(
		u32 address,
		Surface_color_format surface_color_format, size_t width, size_t height,
		gsl::not_null<ID3D12Device*> device, const std::array<float, 4> &clear_color, float, u8)
	{
		DXGI_FORMAT dxgi_format = get_color_surface_format(surface_color_format);
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
		Surface_depth_format surfaceDepthFormat, size_t width, size_t height,
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
	bool rtt_has_format_width_height(const ComPtr<ID3D12Resource> &rtt, Surface_color_format surface_color_format, size_t width, size_t height)
	{
		DXGI_FORMAT dxgi_format = get_color_surface_format(surface_color_format);
		return rtt->GetDesc().Format == dxgi_format && rtt->GetDesc().Width == width && rtt->GetDesc().Height == height;
	}

	static
	bool ds_has_format_width_height(const ComPtr<ID3D12Resource> &rtt, Surface_depth_format surface_depth_stencil_format, size_t width, size_t height)
	{
		//TODO: Check format
		return rtt->GetDesc().Width == width && rtt->GetDesc().Height == height;
	}
};

struct render_targets : public rsx::surface_store<render_target_traits>
{
	INT g_descriptor_stride_rtv;

	D3D12_CPU_DESCRIPTOR_HANDLE current_rtts_handle;
	D3D12_CPU_DESCRIPTOR_HANDLE current_ds_handle;

	void init(ID3D12Device *device);
};

