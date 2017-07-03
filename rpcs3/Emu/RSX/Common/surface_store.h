#pragma once

#include "Utilities/GSL.h"
#include "../GCM.h"
#include <list>

namespace rsx
{
	namespace utility
	{
		std::vector<u8> get_rtt_indexes(surface_target color_target);
		size_t get_aligned_pitch(surface_color_format format, u32 width);
		size_t get_packed_pitch(surface_color_format format, u32 width);
	}

	/**
	 * Helper for surface (ie color and depth stencil render target) management.
	 * It handles surface creation and storage. Backend should only retrieve pointer to surface.
	 * It provides 2 methods get_texture_from_*_if_applicable that should be used when an app
	 * wants to sample a previous surface.
	 * Please note that the backend is still responsible for creating framebuffer/descriptors
	 * and need to inform surface_store everytime surface format/size/addresses change.
	 *
	 * Since it's a template it requires a trait with the followings:
	 * - type surface_storage_type which is a structure containing texture.
	 * - type surface_type which is a pointer to storage_type or a reference.
	 * - type command_list_type that can be void for backend without command list
	 * - type download_buffer_object used by issue_download_command and map_downloaded_buffer functions to handle sync
	 *
	 * - a member function static surface_type(const surface_storage_type&) that returns underlying surface pointer from a storage type.
	 * - 2 member functions static surface_storage_type create_new_surface(u32 address, Surface_color_format/Surface_depth_format format, size_t width, size_t height,...)
	 * used to create a new surface_storage_type holding surface from passed parameters.
	 * - a member function static prepare_rtt_for_drawing(command_list, surface_type) that makes a sampleable surface a color render target one.
	 * - a member function static prepare_rtt_for_drawing(command_list, surface_type) that makes a render target surface a sampleable one.
	 * - a member function static prepare_ds_for_drawing that does the same for depth stencil surface.
	 * - a member function static prepare_ds_for_sampling that does the same for depth stencil surface.
	 * - a member function static bool rtt_has_format_width_height(const surface_storage_type&, Surface_color_format surface_color_format, size_t width, size_t height)
	 * that checks if the given surface has the given format and size
	 * - a member function static bool ds_has_format_width_height that does the same for ds
	 * - a member function static download_buffer_object issue_download_command(surface_type, Surface_color_format color_format, size_t width, size_t height,...)
	 * that generates command to download the given surface to some mappable buffer.
	 * - a member function static issue_depth_download_command that does the same for depth surface
	 * - a member function static issue_stencil_download_command that does the same for stencil surface
	 * - a member function gsl::span<const gsl::byte> map_downloaded_buffer(download_buffer_object, ...) that maps a download_buffer_object
	 * - a member function static unmap_downloaded_buffer that unmaps it.
	 */
	template<typename Traits>
	struct surface_store
	{
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

	protected:
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
	protected:
		/**
		* If render target already exists at address, issue state change operation on cmdList.
		* Otherwise create one with width, height, clearColor info.
		* returns the corresponding render target resource.
		*/
		template <typename ...Args>
		gsl::not_null<surface_type> bind_address_as_render_targets(
			command_list_type command_list,
			u32 address,
			surface_color_format color_format, size_t width, size_t height,
			Args&&... extra_params)
		{
			auto It = m_render_targets_storage.find(address);
			// TODO: Fix corner cases
			// This doesn't take overlapping surface(s) into account.
			// Invalidated surface(s) should also copy their content to the new resources.
			if (It != m_render_targets_storage.end())
			{
				surface_storage_type &rtt = It->second;
				if (Traits::rtt_has_format_width_height(rtt, color_format, width, height))
				{
					Traits::prepare_rtt_for_drawing(command_list, Traits::get(rtt));
					return Traits::get(rtt);
				}
				invalidated_resources.push_back(std::move(rtt));
				m_render_targets_storage.erase(address);
			}

			m_render_targets_storage[address] = Traits::create_new_surface(address, color_format, width, height, std::forward<Args>(extra_params)...);
			return Traits::get(m_render_targets_storage[address]);
		}

		template <typename ...Args>
		gsl::not_null<surface_type> bind_address_as_depth_stencil(
			command_list_type command_list,
			u32 address,
			surface_depth_format depth_format, size_t width, size_t height,
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
				m_depth_stencil_storage.erase(address);
			}

			m_depth_stencil_storage[address] = Traits::create_new_surface(address, depth_format, width, height, std::forward<Args>(extra_params)...);
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
			const std::array<u32, 4> &surface_addresses, u32 address_z,
			Args&&... extra_params)
		{
			u32 clip_width = clip_horizontal_reg;
			u32 clip_height = clip_vertical_reg;
//			u32 clip_x = clip_horizontal_reg;
//			u32 clip_y = clip_vertical_reg;

			// Make previous RTTs sampleable
			for (std::tuple<u32, surface_type> &rtt : m_bound_render_targets)
			{
				if (std::get<1>(rtt) != nullptr)
					Traits::prepare_rtt_for_sampling(command_list, std::get<1>(rtt));
				rtt = std::make_tuple(0, nullptr);
			}

			// Create/Reuse requested rtts
			for (u8 surface_index : utility::get_rtt_indexes(set_surface_target))
			{
				if (surface_addresses[surface_index] == 0)
					continue;

				m_bound_render_targets[surface_index] = std::make_tuple(surface_addresses[surface_index],
					bind_address_as_render_targets(command_list, surface_addresses[surface_index], color_format, clip_width, clip_height, std::forward<Args>(extra_params)...));
			}

			// Same for depth buffer
			if (std::get<1>(m_bound_depth_stencil) != nullptr)
				Traits::prepare_ds_for_sampling(command_list, std::get<1>(m_bound_depth_stencil));
			
			m_bound_depth_stencil = std::make_tuple(0, nullptr);
			
			if (!address_z)
				return;

			m_bound_depth_stencil = std::make_tuple(address_z,
				bind_address_as_depth_stencil(command_list, address_z, depth_format, clip_width, clip_height, std::forward<Args>(extra_params)...));
		}

		/**
		 * Search for given address in stored color surface and returns it if size/format match.
		 * Return an empty surface_type otherwise.
		 */
		surface_type get_texture_from_render_target_if_applicable(u32 address)
		{
			// TODO: Handle texture that overlaps one (or several) surface.
			// Handle texture conversion
			// FIXME: Disgaea 3 loading screen seems to use a subset of a surface. It's not properly handled here.
			// Note: not const because conversions/resolve/... can happen
			auto It = m_render_targets_storage.find(address);
			if (It != m_render_targets_storage.end())
				return Traits::get(It->second);
			return surface_type();
		}

		/**
		* Search for given address in stored depth stencil surface and returns it if size/format match.
		* Return an empty surface_type otherwise.
		*/
		surface_type get_texture_from_depth_stencil_if_applicable(u32 address)
		{
			// TODO: Same as above although there wasn't any game using corner case for DS yet.
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
			std::array<download_buffer_object, 4> download_data = {};

			// Issue download commands
			for (int i = 0; i < 4; i++)
			{
				if (std::get<0>(m_bound_render_targets[i]) == 0)
					continue;

				surface_type surface_resource = std::get<1>(m_bound_render_targets[i]);
				download_data[i] = std::move(
					Traits::issue_download_command(surface_resource, color_format, width, height, std::forward<Args&&>(args)...)
					);
			}

			std::array<std::vector<gsl::byte>, 4> result = {};

			// Sync and copy data
			for (int i = 0; i < 4; i++)
			{
				if (std::get<0>(m_bound_render_targets[i]) == 0)
					continue;

				gsl::span<const gsl::byte> raw_src = Traits::map_downloaded_buffer(download_data[i], std::forward<Args&&>(args)...);

				size_t src_pitch = utility::get_aligned_pitch(color_format, ::narrow<u32>(width));
				size_t dst_pitch = utility::get_packed_pitch(color_format, ::narrow<u32>(width));

				result[i].resize(dst_pitch * height);

				// Note: MSVC + GSL doesn't support span<byte> -> span<T> for non const span atm
				// thus manual conversion
				switch (color_format)
				{
				case surface_color_format::a8b8g8r8:
				case surface_color_format::x8b8g8r8_o8b8g8r8:
				case surface_color_format::x8b8g8r8_z8b8g8r8:
				case surface_color_format::a8r8g8b8:
				case surface_color_format::x8r8g8b8_o8r8g8b8:
				case surface_color_format::x8r8g8b8_z8r8g8b8:
				case surface_color_format::x32:
				{
					gsl::span<be_t<u32>> dst_span{ (be_t<u32>*)result[i].data(), ::narrow<int>(dst_pitch * height / sizeof(be_t<u32>)) };
					copy_pitched_src_to_dst(dst_span, gsl::as_span<const u32>(raw_src), src_pitch, width, height);
					break;
				}
				case surface_color_format::b8:
				{
					gsl::span<u8> dst_span{ (u8*)result[i].data(), ::narrow<int>(dst_pitch * height / sizeof(u8)) };
					copy_pitched_src_to_dst(dst_span, gsl::as_span<const u8>(raw_src), src_pitch, width, height);
					break;
				}
				case surface_color_format::g8b8:
				case surface_color_format::r5g6b5:
				case surface_color_format::x1r5g5b5_o1r5g5b5:
				case surface_color_format::x1r5g5b5_z1r5g5b5:
				{
					gsl::span<be_t<u16>> dst_span{ (be_t<u16>*)result[i].data(), ::narrow<int>(dst_pitch * height / sizeof(be_t<u16>)) };
					copy_pitched_src_to_dst(dst_span, gsl::as_span<const u16>(raw_src), src_pitch, width, height);
					break;
				}
				// Note : may require some big endian swap
				case surface_color_format::w32z32y32x32:
				{
					gsl::span<u128> dst_span{ (u128*)result[i].data(), ::narrow<int>(dst_pitch * height / sizeof(u128)) };
					copy_pitched_src_to_dst(dst_span, gsl::as_span<const u128>(raw_src), src_pitch, width, height);
					break;
				}
				case surface_color_format::w16z16y16x16:
				{
					gsl::span<u64> dst_span{ (u64*)result[i].data(), ::narrow<int>(dst_pitch * height / sizeof(u64)) };
					copy_pitched_src_to_dst(dst_span, gsl::as_span<const u64>(raw_src), src_pitch, width, height);
					break;
				}

				}
				Traits::unmap_downloaded_buffer(download_data[i], std::forward<Args&&>(args)...);
			}
			return result;
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
			std::array<std::vector<gsl::byte>, 2> result = {};
			if (std::get<0>(m_bound_depth_stencil) == 0)
				return result;
			size_t row_pitch = align(width * 4, 256);

			download_buffer_object stencil_data = {};
			download_buffer_object depth_data = Traits::issue_depth_download_command(std::get<1>(m_bound_depth_stencil), depth_format, width, height, std::forward<Args&&>(args)...);
			if (depth_format == surface_depth_format::z24s8)
				stencil_data = std::move(Traits::issue_stencil_download_command(std::get<1>(m_bound_depth_stencil), width, height, std::forward<Args&&>(args)...));

			gsl::span<const gsl::byte> depth_buffer_raw_src = Traits::map_downloaded_buffer(depth_data, std::forward<Args&&>(args)...);
			if (depth_format == surface_depth_format::z16)
			{
				result[0].resize(width * height * 2);
				gsl::span<u16> dest{ (u16*)result[0].data(), ::narrow<int>(width * height) };
				copy_pitched_src_to_dst(dest, gsl::as_span<const u16>(depth_buffer_raw_src), row_pitch, width, height);
			}
			if (depth_format == surface_depth_format::z24s8)
			{
				result[0].resize(width * height * 4);
				gsl::span<u32> dest{ (u32*)result[0].data(), ::narrow<int>(width * height) };
				copy_pitched_src_to_dst(dest, gsl::as_span<const u32>(depth_buffer_raw_src), row_pitch, width, height);
			}
			Traits::unmap_downloaded_buffer(depth_data, std::forward<Args&&>(args)...);

			if (depth_format == surface_depth_format::z16)
				return result;

			gsl::span<const gsl::byte> stencil_buffer_raw_src = Traits::map_downloaded_buffer(stencil_data, std::forward<Args&&>(args)...);
			result[1].resize(width * height);
			gsl::span<u8> dest{ (u8*)result[1].data(), ::narrow<int>(width * height) };
			copy_pitched_src_to_dst(dest, gsl::as_span<const u8>(stencil_buffer_raw_src), align(width, 256), width, height);
			Traits::unmap_downloaded_buffer(stencil_data, std::forward<Args&&>(args)...);
			return result;
		}

		/**
		 * Invalidates cached surface data and marks surface contents as deleteable
		 * Called at the end of a frame (workaround, need to find the proper invalidate command)
		 */
		void invalidate_surface_cache_data(command_list_type command_list)
		{
			for (auto &rtt : m_render_targets_storage)
				Traits::invalidate_rtt_surface_contents(command_list, Traits::get(std::get<1>(rtt)));

			for (auto &ds : m_depth_stencil_storage)
				Traits::invalidate_depth_surface_contents(command_list, Traits::get(std::get<1>(ds)));
		}
	};
}