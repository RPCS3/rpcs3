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

	template <typename surface_type>
	struct surface_subresource_storage
	{
		surface_type surface = nullptr;
		u32 base_address = 0;

		u16 x = 0;
		u16 y = 0;
		u16 w = 0;
		u16 h = 0;

		bool is_bound = false;
		bool is_depth_surface = false;
		bool is_clipped = false;

		surface_subresource_storage() {}

		surface_subresource_storage(u32 addr, surface_type src, u16 X, u16 Y, u16 W, u16 H, bool _Bound, bool _Depth, bool _Clipped = false)
			: base_address(addr), surface(src), x(X), y(Y), w(W), h(H), is_bound(_Bound), is_depth_surface(_Depth), is_clipped(_Clipped)
		{}
	};

	template <typename surface_type>
	struct surface_overlap_info_t
	{
		surface_type surface = nullptr;
		bool is_depth = false;

		u16 src_x = 0;
		u16 src_y = 0;
		u16 dst_x = 0;
		u16 dst_y = 0;
		u16 width = 0;
		u16 height = 0;
	};

	struct surface_format_info
	{
		u32 surface_width;
		u32 surface_height;
		u16 native_pitch;
		u16 rsx_pitch;
		u8 bpp;
	};

	template <typename image_storage_type>
	struct render_target_descriptor
	{
		bool dirty = false;
		image_storage_type old_contents = nullptr;
		rsx::surface_antialiasing read_aa_mode = rsx::surface_antialiasing::center_1_sample;

		GcmTileInfo *tile = nullptr;
		rsx::surface_antialiasing write_aa_mode = rsx::surface_antialiasing::center_1_sample;

		virtual image_storage_type get_surface() = 0;
		virtual u16 get_surface_width() const = 0;
		virtual u16 get_surface_height() const = 0;
		virtual u16 get_rsx_pitch() const = 0;
		virtual u16 get_native_pitch() const = 0;

		void save_aa_mode()
		{
			read_aa_mode = write_aa_mode;
			write_aa_mode = rsx::surface_antialiasing::center_1_sample;
		}

		void reset_aa_mode()
		{
			write_aa_mode = read_aa_mode = rsx::surface_antialiasing::center_1_sample;
		}

		void on_write()
		{
			read_aa_mode = write_aa_mode;
			dirty = false;
			old_contents = nullptr;
		}
	};

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
		using surface_subresource = surface_subresource_storage<surface_type>;
		using surface_overlap_info = surface_overlap_info_t<surface_type>;

		std::unordered_map<u32, surface_storage_type> m_render_targets_storage = {};
		std::unordered_map<u32, surface_storage_type> m_depth_stencil_storage = {};

	public:
		std::array<std::tuple<u32, surface_type>, 4> m_bound_render_targets = {};
		std::tuple<u32, surface_type> m_bound_depth_stencil = {};

		std::list<surface_storage_type> invalidated_resources;
		u64 cache_tag = 0ull;
		u64 write_tag = 0ull;

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
			// TODO: Fix corner cases
			// This doesn't take overlapping surface(s) into account.
			surface_storage_type old_surface_storage;
			surface_storage_type new_surface_storage;
			surface_type old_surface = nullptr;
			surface_type new_surface = nullptr;
			surface_type convert_surface = nullptr;

			// Remove any depth surfaces occupying this memory address (TODO: Discard all overlapping range)
			auto aliased_depth_surface = m_depth_stencil_storage.find(address);
			if (aliased_depth_surface != m_depth_stencil_storage.end())
			{
				Traits::notify_surface_invalidated(aliased_depth_surface->second);
				convert_surface = Traits::get(aliased_depth_surface->second);
				invalidated_resources.push_back(std::move(aliased_depth_surface->second));
				m_depth_stencil_storage.erase(aliased_depth_surface);
			}

			auto It = m_render_targets_storage.find(address);
			if (It != m_render_targets_storage.end())
			{
				surface_storage_type &rtt = It->second;
				if (Traits::rtt_has_format_width_height(rtt, color_format, width, height))
				{
					Traits::notify_surface_persist(rtt);
					Traits::prepare_rtt_for_drawing(command_list, Traits::get(rtt));
					return Traits::get(rtt);
				}

				old_surface = Traits::get(rtt);
				old_surface_storage = std::move(rtt);
				m_render_targets_storage.erase(address);
			}

			// Select source of original data if any
			auto contents_to_copy = old_surface == nullptr ? convert_surface : old_surface;

			// Search invalidated resources for a suitable surface
			for (auto It = invalidated_resources.begin(); It != invalidated_resources.end(); It++)
			{
				auto &rtt = *It;
				if (Traits::rtt_has_format_width_height(rtt, color_format, width, height, true))
				{
					new_surface_storage = std::move(rtt);

					if (old_surface)
					{
						//Exchange this surface with the invalidated one
						Traits::notify_surface_invalidated(old_surface_storage);
						rtt = std::move(old_surface_storage);
					}
					else
						//rtt is now empty - erase it
						invalidated_resources.erase(It);

					new_surface = Traits::get(new_surface_storage);
					Traits::invalidate_surface_contents(command_list, new_surface, contents_to_copy);
					Traits::prepare_rtt_for_drawing(command_list, new_surface);
					break;
				}
			}

			if (old_surface != nullptr && new_surface == nullptr)
			{
				//This was already determined to be invalid and is excluded from testing above
				Traits::notify_surface_invalidated(old_surface_storage);
				invalidated_resources.push_back(std::move(old_surface_storage));
			}

			if (new_surface != nullptr)
			{
				//New surface was found among existing surfaces
				m_render_targets_storage[address] = std::move(new_surface_storage);
				return new_surface;
			}

			m_render_targets_storage[address] = Traits::create_new_surface(address, color_format, width, height, contents_to_copy, std::forward<Args>(extra_params)...);
			return Traits::get(m_render_targets_storage[address]);
		}

		template <typename ...Args>
		gsl::not_null<surface_type> bind_address_as_depth_stencil(
			command_list_type command_list,
			u32 address,
			surface_depth_format depth_format, size_t width, size_t height,
			Args&&... extra_params)
		{
			surface_storage_type old_surface_storage;
			surface_storage_type new_surface_storage;
			surface_type old_surface = nullptr;
			surface_type new_surface = nullptr;
			surface_type convert_surface = nullptr;

			// Remove any color surfaces occupying this memory range (TODO: Discard all overlapping surfaces)
			auto aliased_rtt_surface = m_render_targets_storage.find(address);
			if (aliased_rtt_surface != m_render_targets_storage.end())
			{
				Traits::notify_surface_invalidated(aliased_rtt_surface->second);
				convert_surface = Traits::get(aliased_rtt_surface->second);
				invalidated_resources.push_back(std::move(aliased_rtt_surface->second));
				m_render_targets_storage.erase(aliased_rtt_surface);
			}

			auto It = m_depth_stencil_storage.find(address);
			if (It != m_depth_stencil_storage.end())
			{
				surface_storage_type &ds = It->second;
				if (Traits::ds_has_format_width_height(ds, depth_format, width, height))
				{
					Traits::notify_surface_persist(ds);
					Traits::prepare_ds_for_drawing(command_list, Traits::get(ds));
					return Traits::get(ds);
				}

				old_surface = Traits::get(ds);
				old_surface_storage = std::move(ds);
				m_depth_stencil_storage.erase(address);
			}

			// Select source of original data if any
			auto contents_to_copy = old_surface == nullptr ? convert_surface : old_surface;

			//Search invalidated resources for a suitable surface
			for (auto It = invalidated_resources.begin(); It != invalidated_resources.end(); It++)
			{
				auto &ds = *It;
				if (Traits::ds_has_format_width_height(ds, depth_format, width, height, true))
				{
					new_surface_storage = std::move(ds);

					if (old_surface)
					{
						//Exchange this surface with the invalidated one
						Traits::notify_surface_invalidated(old_surface_storage);
						ds = std::move(old_surface_storage);
					}
					else
						invalidated_resources.erase(It);

					new_surface = Traits::get(new_surface_storage);
					Traits::prepare_ds_for_drawing(command_list, new_surface);
					Traits::invalidate_surface_contents(command_list, new_surface, contents_to_copy);
					break;
				}
			}

			if (old_surface != nullptr && new_surface == nullptr)
			{
				//This was already determined to be invalid and is excluded from testing above
				Traits::notify_surface_invalidated(old_surface_storage);
				invalidated_resources.push_back(std::move(old_surface_storage));
			}

			if (new_surface != nullptr)
			{
				//New surface was found among existing surfaces
				m_depth_stencil_storage[address] = std::move(new_surface_storage);
				return new_surface;
			}

			m_depth_stencil_storage[address] = Traits::create_new_surface(address, depth_format, width, height, contents_to_copy, std::forward<Args>(extra_params)...);
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

			cache_tag++;

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
		 * Moves a single surface from surface storage to invalidated surface store.
		 * Can be triggered by the texture cache's blit functionality when formats do not match
		 */
		void invalidate_single_surface(surface_type surface, bool depth)
		{
			if (!depth)
			{
				for (auto It = m_render_targets_storage.begin(); It != m_render_targets_storage.end(); It++)
				{
					const auto address = It->first;
					const auto ref = Traits::get(It->second);

					if (surface == ref)
					{
						Traits::notify_surface_invalidated(It->second);
						invalidated_resources.push_back(std::move(It->second));
						m_render_targets_storage.erase(It);

						cache_tag++;
						return;
					}
				}
			}
			else
			{
				for (auto It = m_depth_stencil_storage.begin(); It != m_depth_stencil_storage.end(); It++)
				{
					const auto address = It->first;
					const auto ref = Traits::get(It->second);

					if (surface == ref)
					{
						Traits::notify_surface_invalidated(It->second);
						invalidated_resources.push_back(std::move(It->second));
						m_depth_stencil_storage.erase(It);

						cache_tag++;
						return;
					}
				}
			}
		}

		/**
		 * Invalidates surface that exists at an address
		 */
		void invalidate_surface_address(u32 addr, bool depth)
		{
			if (address_is_bound(addr, depth))
			{
				LOG_ERROR(RSX, "Cannot invalidate a currently bound render target!");
				return;
			}

			if (!depth)
			{
				auto It = m_render_targets_storage.find(addr);
				if (It != m_render_targets_storage.end())
				{
					Traits::notify_surface_invalidated(It->second);
					invalidated_resources.push_back(std::move(It->second));
					m_render_targets_storage.erase(It);

					cache_tag++;
					return;
				}
			}
			else
			{
				auto It = m_depth_stencil_storage.find(addr);
				if (It != m_depth_stencil_storage.end())
				{
					Traits::notify_surface_invalidated(It->second);
					invalidated_resources.push_back(std::move(It->second));
					m_depth_stencil_storage.erase(It);

					cache_tag++;
					return;
				}
			}
		}

		/**
		 * Clipping and fitting lookup functions
		 * surface_overlaps - returns true if surface overlaps a given surface address and returns the relative x and y position of the surface address within the surface
		 * address_is_bound - returns true if the surface at a given address is actively bound
		 * get_surface_subresource_if_available - returns a section descriptor that allows to crop surfaces stored in memory
		 */
		bool surface_overlaps_address(surface_type surface, u32 surface_address, u32 texaddr, u16 *x, u16 *y)
		{
			bool is_subslice = false;
			u16  x_offset = 0;
			u16  y_offset = 0;

			if (surface_address > texaddr)
				return false;

			const u32 offset = texaddr - surface_address;
			if (offset == 0)
			{
				*x = 0;
				*y = 0;
				return true;
			}
			else
			{
				surface_format_info info;
				Traits::get_surface_info(surface, &info);

				bool doubled_x = false;
				bool doubled_y = false;

				switch (surface->read_aa_mode)
				{
				case rsx::surface_antialiasing::square_rotated_4_samples:
				case rsx::surface_antialiasing::square_centered_4_samples:
					doubled_y = true;
					//fall through
				case rsx::surface_antialiasing::diagonal_centered_2_samples:
					doubled_x = true;
					break;
				}

				u32 range = info.rsx_pitch * info.surface_height;
				if (doubled_y) range <<= 1;

				if (offset < range)
				{
					y_offset = (offset / info.rsx_pitch);
					x_offset = (offset % info.rsx_pitch) / info.bpp;

					if (doubled_x) x_offset /= 2;
					if (doubled_y) y_offset /= 2;

					is_subslice = true;
				}
			}

			if (is_subslice)
			{
				*x = x_offset;
				*y = y_offset;

				return true;
			}

			return false;
		}

		//Fast hit test
		inline bool surface_overlaps_address_fast(surface_type surface, u32 surface_address, u32 texaddr)
		{
			if (surface_address > texaddr)
				return false;

			const u32 offset = texaddr - surface_address;
			const u32 range = surface->get_rsx_pitch() * surface->get_surface_height();

			return (offset < range);
		}

		bool address_is_bound(u32 address, bool is_depth) const
		{
			if (is_depth)
			{
				const u32 bound_depth_address = std::get<0>(m_bound_depth_stencil);
				return (bound_depth_address == address);
			}

			for (auto &surface : m_bound_render_targets)
			{
				const u32 bound_address = std::get<0>(surface);
				if (bound_address == address)
					return true;
			}

			return false;
		}

		inline bool region_fits(u16 region_width, u16 region_height, u16 x_offset, u16 y_offset, u16 width, u16 height) const
		{
			if ((x_offset + width) > region_width) return false;
			if ((y_offset + height) > region_height) return false;

			return true;
		}

		surface_subresource get_surface_subresource_if_applicable(u32 texaddr, u16 requested_width, u16 requested_height, u16 requested_pitch,
			bool crop = false, bool ignore_depth_formats = false, bool ignore_color_formats = false)
		{
			auto test_surface = [&](surface_type surface, u32 this_address, u16 &x_offset, u16 &y_offset, u16 &w, u16 &h, bool &clipped)
			{
				if (surface_overlaps_address(surface, this_address, texaddr, &x_offset, &y_offset))
				{
					surface_format_info info;
					Traits::get_surface_info(surface, &info);

					u16 real_width = requested_width;
					u16 real_height = requested_height;

					switch (surface->read_aa_mode)
					{
					case rsx::surface_antialiasing::diagonal_centered_2_samples:
						real_width /= 2;
						break;
					case rsx::surface_antialiasing::square_centered_4_samples:
					case rsx::surface_antialiasing::square_rotated_4_samples:
						real_width /= 2;
						real_height /= 2;
						break;
					}

					if (region_fits(info.surface_width, info.surface_height, x_offset, y_offset, real_width, real_height))
					{
						w = real_width;
						h = real_height;
						clipped = false;

						return true;
					}
					else if (crop && info.surface_width > x_offset && info.surface_height > y_offset)
					{
						//Forcefully fit the requested region by clipping and scaling
						u16 remaining_width = info.surface_width - x_offset;
						u16 remaining_height = info.surface_height - y_offset;

						w = std::min(real_width, remaining_width);
						h = std::min(real_height, remaining_height);
						clipped = true;

						return true;
					}
				}

				return false;
			};

			surface_type surface = nullptr;
			bool clipped = false;
			u16  x_offset = 0;
			u16  y_offset = 0;
			u16  w;
			u16  h;

			if (!ignore_color_formats)
			{
				for (auto &tex_info : m_render_targets_storage)
				{
					const u32 this_address = std::get<0>(tex_info);
					if (texaddr < this_address)
						continue;

					surface = std::get<1>(tex_info).get();
					if (surface->get_rsx_pitch() != requested_pitch)
						continue;

					if (requested_width == 0 || requested_height == 0)
					{
						if (!surface_overlaps_address_fast(surface, this_address, texaddr))
							continue;
						else
							return{ this_address, surface, 0, 0, 0, 0, false, false, false };
					}

					if (test_surface(surface, this_address, x_offset, y_offset, w, h, clipped))
						return{ this_address, surface, x_offset, y_offset, w, h, address_is_bound(this_address, false), false, clipped };
				}
			}

			if (!ignore_depth_formats)
			{
				//Check depth surfaces for overlap
				for (auto &tex_info : m_depth_stencil_storage)
				{
					const u32 this_address = std::get<0>(tex_info);
					if (texaddr < this_address)
						continue;

					surface = std::get<1>(tex_info).get();
					if (surface->get_rsx_pitch() != requested_pitch)
						continue;

					if (requested_width == 0 || requested_height == 0)
					{
						if (!surface_overlaps_address_fast(surface, this_address, texaddr))
							continue;
						else
							return{ this_address, surface, 0, 0, 0, 0, false, true, false };
					}

					if (test_surface(surface, this_address, x_offset, y_offset, w, h, clipped))
						return{ this_address, surface, x_offset, y_offset, w, h, address_is_bound(this_address, true), true, clipped };
				}
			}

			return{};
		}

		std::vector<surface_overlap_info> get_merged_texture_memory_region(u32 texaddr, u32 required_width, u32 required_height, u32 required_pitch, u32 bpp)
		{
			std::vector<surface_overlap_info> result;
			const u32 limit = texaddr + (required_pitch * required_height);

			auto process_list_function = [&](std::unordered_map<u32, surface_storage_type>& data, bool is_depth)
			{
				for (auto &tex_info : data)
				{
					auto this_address = std::get<0>(tex_info);
					if (this_address >= limit)
						continue;

					auto surface = std::get<1>(tex_info).get();
					const auto pitch = surface->get_rsx_pitch();
					if (pitch != required_pitch)
						continue;

					const auto texture_size = pitch * surface->get_surface_height();
					if ((this_address + texture_size) <= texaddr)
						continue;

					surface_overlap_info info;
					info.surface = surface;
					info.is_depth = is_depth;

					if (this_address < texaddr)
					{
						auto offset = texaddr - this_address;
						info.src_y = (offset / required_pitch);
						info.src_x = (offset % required_pitch) / bpp;
						info.dst_x = 0;
						info.dst_y = 0;
						info.width = std::min<u32>(required_width, surface->get_surface_width() - info.src_x);
						info.height = std::min<u32>(required_height, surface->get_surface_height() - info.src_y);
					}
					else
					{
						auto offset = this_address - texaddr;
						info.src_x = 0;
						info.src_y = 0;
						info.dst_y = (offset / required_pitch);
						info.dst_x = (offset % required_pitch) / bpp;
						info.width = std::min<u32>(surface->get_surface_width(), required_width - info.dst_x);
						info.height = std::min<u32>(surface->get_surface_height(), required_height - info.dst_y);
					}

					result.push_back(info);
				}
			};

			process_list_function(m_render_targets_storage, false);
			process_list_function(m_depth_stencil_storage, true);
			return result;
		}

		void on_write()
		{
			if (write_tag == cache_tag)
				return;

			for (auto &rtt : m_bound_render_targets)
			{
				if (auto surface = std::get<1>(rtt))
				{
					surface->on_write();
				}
			}

			if (auto ds = std::get<1>(m_bound_depth_stencil))
			{
				ds->on_write();
			}

			write_tag = cache_tag;
		}
	};
}
