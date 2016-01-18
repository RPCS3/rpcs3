#pragma once

#include <utility>
#include <d3d12.h>
#include "d3dx12.h"

#include "D3D12Formats.h"
#include "D3D12MemoryHelpers.h"
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

		template<typename T, typename U>
		void copy_pitched_src_to_dst(gsl::span<T> dest, gsl::span<const U> src, size_t src_pitch_in_bytes, size_t width, size_t height)
		{
			for (int row = 0; row < height; row++)
			{
				for (unsigned col = 0; col < width; col++)
					dest[col] = src[col];
				src = src.subspan(src_pitch_in_bytes / sizeof(U));
				dest = dest.subspan(width);
			}
		}

		size_t get_aligned_pitch(Surface_color_format format, u32 width)
		{
			switch (format)
			{
			case Surface_color_format::b8: return align(width, 256);
			case Surface_color_format::g8b8:
			case Surface_color_format::x1r5g5b5_o1r5g5b5:
			case Surface_color_format::x1r5g5b5_z1r5g5b5:
			case Surface_color_format::r5g6b5: return align(width * 2, 256);
			case Surface_color_format::a8b8g8r8:
			case Surface_color_format::x8b8g8r8_o8b8g8r8:
			case Surface_color_format::x8b8g8r8_z8b8g8r8:
			case Surface_color_format::x8r8g8b8_o8r8g8b8:
			case Surface_color_format::x8r8g8b8_z8r8g8b8:
			case Surface_color_format::x32:
			case Surface_color_format::a8r8g8b8: return align(width * 4, 256);
			case Surface_color_format::w16z16y16x16: return align(width * 8, 256);
			case Surface_color_format::w32z32y32x32: return align(width * 16, 256);
			}
			throw EXCEPTION("Unknow color surface format");
		}

		size_t get_packed_pitch(Surface_color_format format, u32 width)
		{
			switch (format)
			{
			case Surface_color_format::b8: return width;
			case Surface_color_format::g8b8:
			case Surface_color_format::x1r5g5b5_o1r5g5b5:
			case Surface_color_format::x1r5g5b5_z1r5g5b5:
			case Surface_color_format::r5g6b5: return width * 2;
			case Surface_color_format::a8b8g8r8:
			case Surface_color_format::x8b8g8r8_o8b8g8r8:
			case Surface_color_format::x8b8g8r8_z8b8g8r8:
			case Surface_color_format::x8r8g8b8_o8r8g8b8:
			case Surface_color_format::x8r8g8b8_z8r8g8b8:
			case Surface_color_format::x32:
			case Surface_color_format::a8r8g8b8: return width * 4;
			case Surface_color_format::w16z16y16x16: return width * 8;
			case Surface_color_format::w32z32y32x32: return width * 16;
			}
			throw EXCEPTION("Unknow color surface format");
		}
	}

	template<typename Traits>
	struct surface_store
	{
	private:
		using surface_storage_type = typename Traits::surface_storage_type;
		using surface_type = typename Traits::surface_type;
		using command_list_type = typename Traits::command_list_type;
		using download_buffer_object = typename Traits::download_buffer_object;

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

		template <typename... Args>
		std::array<std::vector<gsl::byte>, 4> get_render_targets_data(
			Surface_color_format surface_color_format, size_t width, size_t height,
			Args&& ...args
			)
		{
			std::array<download_buffer_object, 4> download_data = {};

			// Issue download commands
			for (int i = 0; i < 4; i++)
			{
				if (std::get<0>(m_bound_render_targets[i]) == 0)
					continue;

				surface_type surface_resource = std::get<1>(m_bound_render_targets[i]);
				download_data[i] = std::move(
						Traits::issue_download_command(surface_resource, surface_color_format, width, height, std::forward<Args&&>(args)...)
					);
			}

			std::array<std::vector<gsl::byte>, 4> result = {};

			// Sync and copy data
			for (int i = 0; i < 4; i++)
			{
				if (std::get<0>(m_bound_render_targets[i]) == 0)
					continue;

				gsl::span<const gsl::byte> raw_src = Traits::map_downloaded_buffer(download_data[i], std::forward<Args&&>(args)...);

				size_t src_pitch = get_aligned_pitch(surface_color_format, gsl::narrow<u32>(width));
				size_t dst_pitch = get_packed_pitch(surface_color_format, gsl::narrow<u32>(width));

				result[i].resize(dst_pitch * height);

				// Note: MSVC + GSL doesn't support span<byte> -> span<T> for non const span atm
				// thus manual conversion
				switch (surface_color_format)
				{
				case Surface_color_format::a8b8g8r8:
				case Surface_color_format::x8b8g8r8_o8b8g8r8:
				case Surface_color_format::x8b8g8r8_z8b8g8r8:
				case Surface_color_format::a8r8g8b8:
				case Surface_color_format::x8r8g8b8_o8r8g8b8:
				case Surface_color_format::x8r8g8b8_z8r8g8b8:
				case Surface_color_format::x32:
				{
					gsl::span<be_t<u32>> dst_span{ (be_t<u32>*)result[i].data(), gsl::narrow<int>(dst_pitch * width / sizeof(be_t<u32>)) };
					copy_pitched_src_to_dst(dst_span, gsl::as_span<const u32>(raw_src), src_pitch, width, height);
					break;
				}
				case Surface_color_format::b8:
				{
					gsl::span<u8> dst_span{ (u8*)result[i].data(), gsl::narrow<int>(dst_pitch * width / sizeof(u8)) };
					copy_pitched_src_to_dst(dst_span, gsl::as_span<const u8>(raw_src), src_pitch, width, height);
					break;
				}
				case Surface_color_format::g8b8:
				case Surface_color_format::r5g6b5:
				case Surface_color_format::x1r5g5b5_o1r5g5b5:
				case Surface_color_format::x1r5g5b5_z1r5g5b5:
				{
					gsl::span<be_t<u16>> dst_span{ (be_t<u16>*)result[i].data(), gsl::narrow<int>(dst_pitch * width / sizeof(be_t<u16>)) };
					copy_pitched_src_to_dst(dst_span, gsl::as_span<const u16>(raw_src), src_pitch, width, height);
					break;
				}
				// Note : may require some big endian swap
				case Surface_color_format::w32z32y32x32:
				{
					gsl::span<u128> dst_span{ (u128*)result[i].data(), gsl::narrow<int>(dst_pitch * width / sizeof(u128)) };
					copy_pitched_src_to_dst(dst_span, gsl::as_span<const u128>(raw_src), src_pitch, width, height);
					break;
				}
				case Surface_color_format::w16z16y16x16:
				{
					gsl::span<u64> dst_span{ (u64*)result[i].data(), gsl::narrow<int>(dst_pitch * width / sizeof(u64)) };
					copy_pitched_src_to_dst(dst_span, gsl::as_span<const u64>(raw_src), src_pitch, width, height);
					break;
				}

				}
				Traits::unmap_downloaded_buffer(download_data[i], std::forward<Args&&>(args)...);
			}
			return result;
		}

		template <typename... Args>
		std::array<std::vector<gsl::byte>, 2> get_depth_stencil_data(
			Surface_depth_format surface_depth_format, size_t width, size_t height,
			Args&& ...args
			)
		{
			std::array<std::vector<gsl::byte>, 2> result = {};
			if (std::get<0>(m_bound_depth_stencil) == 0)
				return result;
			size_t row_pitch = align(width * 4, 256);

			download_buffer_object stencil_data = {};
			download_buffer_object depth_data = Traits::issue_depth_download_command(std::get<1>(m_bound_depth_stencil), surface_depth_format, width, height, std::forward<Args&&>(args)...);
			if (surface_depth_format == Surface_depth_format::z24s8)
				stencil_data = std::move(Traits::issue_stencil_download_command(std::get<1>(m_bound_depth_stencil), width, height, std::forward<Args&&>(args)...));

			gsl::span<const gsl::byte> depth_buffer_raw_src = Traits::map_downloaded_buffer(depth_data, std::forward<Args&&>(args)...);
			if (surface_depth_format == Surface_depth_format::z16)
			{
				result[0].resize(width * height * 2);
				gsl::span<u16> dest{ (u16*)result[0].data(), gsl::narrow<int>(width * height) };
				copy_pitched_src_to_dst(dest, gsl::as_span<const u16>(depth_buffer_raw_src), row_pitch, width, height);
			}
			if (surface_depth_format == Surface_depth_format::z24s8)
			{
				result[0].resize(width * height * 4);
				gsl::span<u32> dest{ (u32*)result[0].data(), gsl::narrow<int>(width * height) };
				copy_pitched_src_to_dst(dest, gsl::as_span<const u32>(depth_buffer_raw_src), row_pitch, width, height);
			}
			Traits::unmap_downloaded_buffer(depth_data, std::forward<Args&&>(args)...);

			if (surface_depth_format == Surface_depth_format::z16)
				return result;

			gsl::span<const gsl::byte> stencil_buffer_raw_src = Traits::map_downloaded_buffer(stencil_data, std::forward<Args&&>(args)...);
			result[1].resize(width * height);
			gsl::span<u8> dest{ (u8*)result[1].data(), gsl::narrow<int>(width * height) };
			copy_pitched_src_to_dst(dest, gsl::as_span<const u8>(stencil_buffer_raw_src), align(width, 256), width, height);
			Traits::unmap_downloaded_buffer(stencil_data, std::forward<Args&&>(args)...);
			return result;
		}
	};
}

struct render_target_traits
{
	using surface_storage_type = ComPtr<ID3D12Resource>;
	using surface_type = ID3D12Resource*;
	using command_list_type = gsl::not_null<ID3D12GraphicsCommandList*>;
	using download_buffer_object = std::tuple<size_t, size_t, size_t, ComPtr<ID3D12Fence>, HANDLE>; // heap offset, size, last_put_pos, fence, handle

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

	static
	std::tuple<size_t, size_t, size_t, ComPtr<ID3D12Fence>, HANDLE> issue_download_command(
		gsl::not_null<ID3D12Resource*> rtt,
		Surface_color_format color_format, size_t width, size_t height,
		gsl::not_null<ID3D12Device*> device, gsl::not_null<ID3D12CommandQueue*> command_queue, data_heap &readback_heap, resource_storage &res_store
		)
	{
		ID3D12GraphicsCommandList* command_list = res_store.command_list.Get();
		DXGI_FORMAT dxgi_format = get_color_surface_format(color_format);
		size_t row_pitch = rsx::get_aligned_pitch(color_format, gsl::narrow<u32>(width));

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
		Surface_depth_format depth_format, size_t width, size_t height,
		gsl::not_null<ID3D12Device*> device, gsl::not_null<ID3D12CommandQueue*> command_queue, data_heap &readback_heap, resource_storage &res_store
			)
	{
		ID3D12GraphicsCommandList* command_list = res_store.command_list.Get();
		DXGI_FORMAT dxgi_format = (depth_format == Surface_depth_format::z24s8) ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_R16_TYPELESS;

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
			gsl::not_null<ID3D12Device*> device, gsl::not_null<ID3D12CommandQueue*> command_queue, data_heap &readback_heap, resource_storage &res_store
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
		gsl::not_null<ID3D12Device*> device, gsl::not_null<ID3D12CommandQueue*> command_queue, data_heap &readback_heap, resource_storage &res_store)
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
		return { mapped_buffer , gsl::narrow<int>(buffer_size) };
	}

	static
	void unmap_downloaded_buffer(const std::tuple<size_t, size_t, size_t, ComPtr<ID3D12Fence>, HANDLE> &sync_data,
		gsl::not_null<ID3D12Device*> device, gsl::not_null<ID3D12CommandQueue*> command_queue, data_heap &readback_heap, resource_storage &res_store)
	{
		readback_heap.unmap();
	}
};

struct render_targets : public rsx::surface_store<render_target_traits>
{
	INT g_descriptor_stride_rtv;

	D3D12_CPU_DESCRIPTOR_HANDLE current_rtts_handle;
	D3D12_CPU_DESCRIPTOR_HANDLE current_ds_handle;

	void init(ID3D12Device *device);
};

