#pragma once

#include "../rsx_utils.h"
#include "TextureUtils.h"

namespace rsx
{
	using texture_channel_remap_t = std::pair<std::array<u8, 4>, std::array<u8, 4>>;

	// Defines pixel operation to be performed on a surface before it is ready for use
	enum surface_transform : u32
	{
		identity = 0,                // Nothing
		argb_to_bgra = 1,            // Swap ARGB to BGRA (endian swap)
		coordinate_transform = 2     // Incoming source coordinates may generated based on the format of the secondary (dest) surface. Recalculate them before use.
	};

	template<typename image_resource_type>
	struct copy_region_descriptor_base
	{
		image_resource_type src;
		flags32_t xform;
		u8  level;
		u16 src_x;
		u16 src_y;
		u16 dst_x;
		u16 dst_y;
		u16 dst_z;
		u16 src_w;
		u16 src_h;
		u16 dst_w;
		u16 dst_h;
	};

	// Deferred texture processing commands
	enum class deferred_request_command : u32
	{
		nop = 0,                  // Nothing
		copy_image_static,        // Copy image and cache the results
		copy_image_dynamic,       // Copy image but do not cache the results
		cubemap_gather,           // Provided list of sections generates a cubemap
		cubemap_unwrap,           // One large texture provided to be partitioned into a cubemap
		atlas_gather,             // Provided list of sections generates a texture atlas
		_3d_gather,               // Provided list of sections generates a 3D array
		_3d_unwrap,               // One large texture provided to be partitioned into a 3D array
		mipmap_gather             // Provided list of sections to be reassembled as mipmap levels of the same texture
	};

	struct image_section_attributes_t
	{
		u32 address;
		u32 gcm_format;
		u16 width;
		u16 height;
		u16 depth;
		u16 mipmaps;
		u16 pitch;
		u16 slice_h;
		u8  bpp;
		bool swizzled;
	};

	struct blit_op_result
	{
		bool succeeded = false;
		u32 real_dst_address = 0;
		u32 real_dst_size = 0;

		blit_op_result(bool success) : succeeded(success)
		{}

		inline address_range to_address_range() const
		{
			return address_range::start_length(real_dst_address, real_dst_size);
		}
	};

	struct blit_target_properties
	{
		bool use_dma_region;
		u32 offset;
		u32 width;
		u32 height;
	};

	struct texture_cache_search_options
	{
		u8 lookup_mask = 0xff;
		bool is_compressed_format = false;
		bool skip_texture_barriers = false;
		bool skip_texture_merge = false;
		bool prefer_surface_cache = false;
	};

	namespace texture_cache_helpers
	{
		static inline bool is_gcm_depth_format(u32 format)
		{
			switch (format)
			{
			case CELL_GCM_TEXTURE_DEPTH16:
			case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
			case CELL_GCM_TEXTURE_DEPTH24_D8:
			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
				return true;
			default:
				return false;
			}
		}

		static inline u32 get_compatible_depth_format(u32 gcm_format)
		{
			switch (gcm_format)
			{
			case CELL_GCM_TEXTURE_DEPTH24_D8:
			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
			case CELL_GCM_TEXTURE_DEPTH16:
			case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
				return gcm_format;
			case CELL_GCM_TEXTURE_A8R8G8B8:
				return CELL_GCM_TEXTURE_DEPTH24_D8;
			case CELL_GCM_TEXTURE_X16:
				//case CELL_GCM_TEXTURE_A4R4G4B4:
				//case CELL_GCM_TEXTURE_G8B8:
				//case CELL_GCM_TEXTURE_A1R5G5B5:
				//case CELL_GCM_TEXTURE_R5G5B5A1:
				//case CELL_GCM_TEXTURE_R5G6B5:
				//case CELL_GCM_TEXTURE_R6G5B5:
				return CELL_GCM_TEXTURE_DEPTH16;
			}

			rsx_log.error("Unsupported depth conversion (0x%X)", gcm_format);
			return gcm_format;
		}

		static inline u32 get_sized_blit_format(bool is_32_bit, bool depth_format, bool format_conversion)
		{
			if (format_conversion)
			{
				return (is_32_bit) ? CELL_GCM_TEXTURE_A8R8G8B8 : CELL_GCM_TEXTURE_R5G6B5;
			}
			else if (is_32_bit)
			{
				return (!depth_format) ? CELL_GCM_TEXTURE_A8R8G8B8 : CELL_GCM_TEXTURE_DEPTH24_D8;
			}
			else
			{
				return (!depth_format) ? CELL_GCM_TEXTURE_X16 : CELL_GCM_TEXTURE_DEPTH16;
			}
		}

		static inline bool is_compressed_gcm_format(u32 format)
		{
			switch (format)
			{
			default:
				return false;
			case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
			case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
			case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
			case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
			case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
			case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
				return true;
			}
		}

		static inline blit_target_properties get_optimal_blit_target_properties(
			bool src_is_render_target,
			address_range dst_range,
			u32 dst_pitch,
			const sizeu src_dimensions,
			const sizeu dst_dimensions)
		{
			if (get_location(dst_range.start) == CELL_GCM_LOCATION_LOCAL)
			{
				// Check if this is a blit to the output buffer
				// TODO: This can be used to implement reference tracking to possibly avoid downscaling
				const auto renderer = rsx::get_current_renderer();
				for (u32 i = 0; i < renderer->display_buffers_count; ++i)
				{
					const auto& buffer = renderer->display_buffers[i];

					if (!buffer.valid())
					{
						continue;
					}

					const u32 bpp = g_fxo->get<rsx::avconf>()->get_bpp();

					const u32 pitch = buffer.pitch ? +buffer.pitch : bpp * buffer.width;
					if (pitch != dst_pitch)
					{
						continue;
					}

					const auto buffer_range = address_range::start_length(rsx::get_address(buffer.offset, CELL_GCM_LOCATION_LOCAL), pitch * (buffer.height - 1) + (buffer.width * bpp));
					if (dst_range.inside(buffer_range))
					{
						// Match found
						return { false, buffer_range.start, buffer.width, buffer.height };
					}

					if (dst_range.overlaps(buffer_range)) [[unlikely]]
					{
						// The range clips the destination but does not fit inside it
						// Use DMA stream to optimize the flush that is likely to happen when flipping
						return { true };
					}
				}
			}

			if (src_is_render_target)
			{
				// Attempt to optimize...
				if (dst_dimensions.width == 1280 || dst_dimensions.width == 2560) [[likely]]
				{
					// Optimizations table based on common width/height pairings. If we guess wrong, the upload resolver will fix it anyway
					// TODO: Add more entries based on empirical data
					const auto optimal_height = std::max(dst_dimensions.height, 720u);
					return { false, 0, dst_dimensions.width, optimal_height };
				}

				if (dst_dimensions.width == src_dimensions.width)
				{
					const auto optimal_height = std::max(dst_dimensions.height, src_dimensions.height);
					return { false, 0, dst_dimensions.width, optimal_height };
				}
			}

			return { false, 0, dst_dimensions.width, dst_dimensions.height };
		}

		template<typename section_storage_type, typename copy_region_type, typename surface_store_list_type>
		void gather_texture_slices(
			std::vector<copy_region_type>& out,
			const surface_store_list_type& fbos,
			const std::vector<section_storage_type*>& local,
			const image_section_attributes_t& attr,
			u16 count, bool is_depth)
		{
			// Need to preserve sorting order
			struct sort_helper
			{
				u64 tag;   // Timestamp
				u32 list;  // List source, 0 = fbo, 1 = local
				u32 index; // Index in list
			};

			std::vector<sort_helper> sort_list;

			if (!fbos.empty() && !local.empty())
			{
				// Generate sorting tree if both resources are available and overlapping
				sort_list.reserve(fbos.size() + local.size());

				for (u32 index = 0; index < fbos.size(); ++index)
				{
					sort_list.push_back({ fbos[index].surface->last_use_tag, 0, index });
				}

				for (u32 index = 0; index < local.size(); ++index)
				{
					if (local[index]->get_context() != rsx::texture_upload_context::blit_engine_dst)
						continue;

					sort_list.push_back({ local[index]->last_write_tag, 1, index });
				}

				std::sort(sort_list.begin(), sort_list.end(), [](const auto& a, const auto& b)
				{
					return (a.tag < b.tag);
				});
			}

			auto add_rtt_resource = [&](auto& section, u16 slice)
			{
				const u32 slice_begin = (slice * attr.slice_h);
				const u32 slice_end = (slice_begin + attr.height);

				const u32 section_end = section.dst_area.y + section.dst_area.height;
				if (section.dst_area.y >= slice_end || section_end <= slice_begin)
				{
					// Belongs to a different slice
					return;
				}

				// How much of this slice to read?
				int rebased = int(section.dst_area.y) - slice_begin;
				auto src_x = section.src_area.x;
				auto dst_x = section.dst_area.x;
				auto src_y = section.src_area.y;
				auto dst_y = section.dst_area.y;

				if (rebased < 0)
				{
					const u16 delta = u16(-rebased);
					src_y += delta;
					dst_y += delta;

					ensure(dst_y == slice_begin);
				}

				ensure(dst_y >= slice_begin);

				const auto h = std::min(section_end, slice_end) - dst_y;
				dst_y = (dst_y - slice_begin);

				const auto surface_width = section.surface->get_surface_width(rsx::surface_metrics::pixels);
				const auto surface_height = section.surface->get_surface_height(rsx::surface_metrics::pixels);
				const auto [src_width, src_height] = rsx::apply_resolution_scale<true>(section.src_area.width, h, surface_width, surface_height);
				const auto [dst_width, dst_height] = rsx::apply_resolution_scale<true>(section.dst_area.width, h, attr.width, attr.height);

				std::tie(src_x, src_y) = rsx::apply_resolution_scale<false>(src_x, src_y, surface_width, surface_height);
				std::tie(dst_x, dst_y) = rsx::apply_resolution_scale<false>(dst_x, dst_y, attr.width, attr.height);

				out.push_back
				({
					section.surface->get_surface(rsx::surface_access::read),
					surface_transform::identity,
					0,
					static_cast<u16>(src_x),
					static_cast<u16>(src_y),
					static_cast<u16>(dst_x),
					static_cast<u16>(dst_y),
					slice,
					src_width, src_height,
					dst_width, dst_height
					});
			};

			auto add_local_resource = [&](auto& section, u32 address, u16 slice, bool scaling = true)
			{
				// Intersect this resource with the original one
				const auto section_bpp = get_format_block_size_in_bytes(section->get_gcm_format());
				const auto normalized_width = (section->get_width() * section_bpp) / attr.bpp;

				const auto clipped = rsx::intersect_region(
					section->get_section_base(), normalized_width, section->get_height(), section_bpp, /* parent region (extractee) */
					address, attr.width, attr.slice_h, attr.bpp, /* child region (extracted) */
					attr.pitch);

				// Rect intersection test
				// TODO: Make the intersection code cleaner with proper 2D regions
				if (std::get<0>(clipped).x >= section->get_width())
				{
					// Overlap lies outside the image area!
					return;
				}

				const u32 slice_begin = slice * attr.slice_h;
				const u32 slice_end = slice_begin + attr.height;

				const auto dst_y = std::get<1>(clipped).y;
				const auto dst_h = std::get<2>(clipped).height;

				const auto section_end = dst_y + dst_h;
				if (dst_y >= slice_end || section_end <= slice_begin)
				{
					// Belongs to a different slice
					return;
				}

				const u16 dst_w = static_cast<u16>(std::get<2>(clipped).width);
				const u16 src_w = static_cast<u16>(dst_w * attr.bpp) / section_bpp;
				const u16 height = static_cast<u16>(dst_h);

				if (scaling)
				{
					// Since output is upscaled, also upscale on dst
					const auto [_dst_x, _dst_y] = rsx::apply_resolution_scale<false>(static_cast<u16>(std::get<1>(clipped).x), static_cast<u16>(dst_y - slice_begin), attr.width, attr.height);
					const auto [_dst_w, _dst_h] = rsx::apply_resolution_scale<true>(dst_w, height, attr.width, attr.height);

					out.push_back
					({
						section->get_raw_texture(),
						surface_transform::identity,
						0,
						static_cast<u16>(std::get<0>(clipped).x),   // src.x
						static_cast<u16>(std::get<0>(clipped).y),   // src.y
						_dst_x,                                     // dst.x
						_dst_y,                                     // dst.y
						slice,
						src_w,
						height,
						_dst_w,
						_dst_h,
						});
				}
				else
				{
					out.push_back
					({
						section->get_raw_texture(),
						surface_transform::identity,
						0,
						static_cast<u16>(std::get<0>(clipped).x),   // src.x
						static_cast<u16>(std::get<0>(clipped).y),   // src.y
						static_cast<u16>(std::get<1>(clipped).x),   // dst.x
						static_cast<u16>(dst_y - slice_begin),      // dst.y
						0,
						src_w,
						height,
						dst_w,
						height,
						});
				}
			};

			u32 current_address = attr.address;
			//u16 current_src_offset = 0;
			//u16 current_dst_offset = 0;
			u32 slice_size = (attr.pitch * attr.slice_h);

			out.reserve(count);
			u16 found_slices = 0;

			for (u16 slice = 0; slice < count; ++slice)
			{
				auto num_surface = out.size();

				if (local.empty()) [[likely]]
				{
					for (auto& section : fbos)
					{
						add_rtt_resource(section, slice);
					}
				}
				else if (fbos.empty())
				{
					for (auto& section : local)
					{
						add_local_resource(section, current_address, slice, false);
					}
				}
				else
				{
					for (const auto& e : sort_list)
					{
						if (e.list == 0)
						{
							add_rtt_resource(fbos[e.index], slice);
						}
						else
						{
							add_local_resource(local[e.index], current_address, slice);
						}
					}
				}

				current_address += slice_size;
				if (out.size() != num_surface)
				{
					found_slices++;
				}
			}

			if (found_slices < count)
			{
				if (found_slices > 0)
				{
					//TODO: Gather remaining sides from the texture cache or upload from cpu (too slow?)
					rsx_log.error("Could not gather all required slices for cubemap/3d generation");
				}
				else
				{
					rsx_log.warning("Could not gather textures into an atlas; using CPU fallback...");
				}
			}
		}

		template<typename render_target_type>
		bool check_framebuffer_resource(
			render_target_type texptr,
			const image_section_attributes_t& attr,
			texture_dimension_extended extended_dimension)
		{
			if (!rsx::pitch_compatible(texptr, attr.pitch, attr.height))
			{
				return false;
			}

			const auto surface_width = texptr->get_surface_width(rsx::surface_metrics::samples);
			const auto surface_height = texptr->get_surface_height(rsx::surface_metrics::samples);

			switch (extended_dimension)
			{
			case rsx::texture_dimension_extended::texture_dimension_1d:
				return (surface_width >= attr.width);
			case rsx::texture_dimension_extended::texture_dimension_2d:
				return (surface_width >= attr.width && surface_height >= attr.height);
			case rsx::texture_dimension_extended::texture_dimension_3d:
				return (surface_width >= attr.width && surface_height >= (attr.slice_h * attr.depth));
			case rsx::texture_dimension_extended::texture_dimension_cubemap:
				return (surface_width == attr.height && surface_width >= attr.width && surface_height >= (attr.slice_h * 6));
			}

			return false;
		}

		template <typename sampled_image_descriptor, typename commandbuffer_type, typename render_target_type>
		sampled_image_descriptor process_framebuffer_resource_fast(commandbuffer_type& cmd,
			render_target_type texptr,
			const image_section_attributes_t& attr,
			const size2f& scale,
			texture_dimension_extended extended_dimension,
			u32 encoded_remap, const texture_channel_remap_t& decoded_remap,
			bool surface_is_rop_target,
			bool force_convert)
		{
			texptr->read_barrier(cmd);

			const auto surface_width = texptr->get_surface_width(rsx::surface_metrics::samples);
			const auto surface_height = texptr->get_surface_height(rsx::surface_metrics::samples);

			bool is_depth = texptr->is_depth_surface();
			auto attr2 = attr;

			if (rsx::get_resolution_scale_percent() != 100)
			{
				const auto [scaled_w, scaled_h] = rsx::apply_resolution_scale<true>(attr.width, attr.height, surface_width, surface_height);
				const auto [unused, scaled_slice_h] = rsx::apply_resolution_scale<false>(RSX_SURFACE_DIMENSION_IGNORED, attr.slice_h, surface_width, surface_height);
				attr2.width = scaled_w;
				attr2.height = scaled_h;
				attr2.slice_h = scaled_slice_h;
			}

			if (const bool gcm_format_is_depth = is_gcm_depth_format(attr2.gcm_format);
				gcm_format_is_depth != is_depth)
			{
				if (force_convert)
				{
					is_depth = gcm_format_is_depth;
				}
				else
				{
					attr2.gcm_format = get_compatible_depth_format(attr2.gcm_format);
				}

				// Always make sure the conflict is resolved!
				ensure(is_gcm_depth_format(attr2.gcm_format) == is_depth);
			}

			if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_2d ||
				extended_dimension == rsx::texture_dimension_extended::texture_dimension_1d) [[likely]]
			{
				if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_1d)
				{
					ensure(attr.height == 1);
				}

				if ((surface_is_rop_target && g_cfg.video.strict_rendering_mode) ||
					attr.width < surface_width ||
					attr.height < surface_height ||
					force_convert)
				{
					const auto format_class = (force_convert) ? classify_format(attr2.gcm_format) : texptr->format_class();
					const auto command = surface_is_rop_target ? deferred_request_command::copy_image_dynamic : deferred_request_command::copy_image_static;

					return { texptr->get_surface(rsx::surface_access::read), command, attr2, {},
							texture_upload_context::framebuffer_storage, format_class, scale,
							extended_dimension, decoded_remap };
				}

				return{ texptr->get_view(encoded_remap, decoded_remap), texture_upload_context::framebuffer_storage,
						texptr->format_class(), scale, rsx::texture_dimension_extended::texture_dimension_2d, surface_is_rop_target };
			}

			if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_3d)
			{
				return{ texptr->get_surface(rsx::surface_access::read), deferred_request_command::_3d_unwrap,
						attr2, {},
						texture_upload_context::framebuffer_storage, texptr->format_class(), scale,
						rsx::texture_dimension_extended::texture_dimension_3d, decoded_remap };
			}

			ensure(extended_dimension == rsx::texture_dimension_extended::texture_dimension_cubemap);

			return{ texptr->get_surface(rsx::surface_access::read), deferred_request_command::cubemap_unwrap,
					attr2, {},
					texture_upload_context::framebuffer_storage, texptr->format_class(), scale,
					rsx::texture_dimension_extended::texture_dimension_cubemap, decoded_remap };
		}

		template <typename sampled_image_descriptor, typename surface_store_list_type, typename section_storage_type>
		sampled_image_descriptor merge_cache_resources(
			const surface_store_list_type& fbos, const std::vector<section_storage_type*>& local,
			const image_section_attributes_t& attr,
			const size2f& scale,
			texture_dimension_extended extended_dimension,
			u32 encoded_remap, const texture_channel_remap_t& decoded_remap,
			int select_hint = -1)
		{
			ensure((select_hint & 0x1) == select_hint);

			bool is_depth = (select_hint == 0) ? fbos.back().is_depth : local.back()->is_depth_texture();
			bool aspect_mismatch = false;
			auto attr2 = attr;

			// Check for mixed sources with aspect mismatch
			// NOTE: If the last texture is a perfect match, this method would not have been called which means at least one transfer has to occur
			if ((fbos.size() + local.size()) > 1) [[unlikely]]
			{
				for (const auto& tex : local)
				{
					if (tex->is_depth_texture() != is_depth)
					{
						aspect_mismatch = true;
						break;
					}
				}

				if (!aspect_mismatch) [[likely]]
				{
					for (const auto& surface : fbos)
					{
						if (surface.is_depth != is_depth)
						{
							aspect_mismatch = true;
							break;
						}
					}
				}
			}

			if (aspect_mismatch)
			{
				// Override with the requested format
				is_depth = is_gcm_depth_format(attr.gcm_format);
			}
			else if (is_depth)
			{
				// Depth format textures were found. Check if the data can be bitcast without conversion.
				if (const auto suggested_format = get_compatible_depth_format(attr.gcm_format);
					!is_gcm_depth_format(suggested_format))
				{
					// Requested format cannot be directly read from a depth texture.
					// Typeless conversion will be performed to make data accessible.
					is_depth = false;
				}
				else
				{
					// Replace request format with one that is compatible with existing data.
					attr2.gcm_format = suggested_format;
				}
			}

			// If this method was called, there is no easy solution, likely means atlas gather is needed
			const auto [scaled_w, scaled_h] = rsx::apply_resolution_scale(attr2.width, attr2.height);
			const auto format_class = classify_format(attr2.gcm_format);

			if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_cubemap)
			{
				attr2.width = scaled_w;
				attr2.height = scaled_h;
				attr2.depth = 1;

				sampled_image_descriptor desc = { nullptr, deferred_request_command::cubemap_gather,
						attr2, {},
						texture_upload_context::framebuffer_storage, format_class, scale,
						rsx::texture_dimension_extended::texture_dimension_cubemap, decoded_remap };

				gather_texture_slices(desc.external_subresource_desc.sections_to_copy, fbos, local, attr, 6, is_depth);
				return desc;
			}
			else if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_3d && attr.depth > 1)
			{
				attr2.width = scaled_w;
				attr2.height = scaled_h;

				sampled_image_descriptor desc = { nullptr, deferred_request_command::_3d_gather,
					attr2, {},
					texture_upload_context::framebuffer_storage, format_class, scale,
					rsx::texture_dimension_extended::texture_dimension_3d, decoded_remap };

				gather_texture_slices(desc.external_subresource_desc.sections_to_copy, fbos, local, attr, attr.depth, is_depth);
				return desc;
			}

			if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_1d)
			{
				ensure(attr.height == 1);
			}

			if (!fbos.empty())
			{
				attr2.width = scaled_w;
				attr2.height = scaled_h;
			}

			sampled_image_descriptor result = { nullptr, deferred_request_command::atlas_gather,
					attr2, {}, texture_upload_context::framebuffer_storage, format_class,
					scale, rsx::texture_dimension_extended::texture_dimension_2d, decoded_remap };

			gather_texture_slices(result.external_subresource_desc.sections_to_copy, fbos, local, attr, 1, is_depth);
			result.simplify();
			return result;
		}

		template<typename sampled_image_descriptor, typename copy_region_descriptor_type>
		bool append_mipmap_level(
			std::vector<copy_region_descriptor_type>& sections,   // Destination list
			const sampled_image_descriptor& level,                // Descriptor for the image level being checked
			const image_section_attributes_t& attr,               // Attributes of image level
			u8 mipmap_level,                                      // Level index
			bool apply_upscaling,                                 // Whether to upscale the results or not
			const image_section_attributes_t& level0_attr)        // Attributes of the first mipmap level
		{
			if (level.image_handle)
			{
				copy_region_descriptor_type mip{};
				mip.src = level.image_handle->image();
				mip.xform = surface_transform::coordinate_transform;
				mip.level = mipmap_level;
				mip.dst_w = attr.width;
				mip.dst_h = attr.height;

				// "Fast" framebuffer results are a perfect match for attr so we do not store transfer sizes
				// Calculate transfer dimensions from attr
				if (level.upload_context == rsx::texture_upload_context::framebuffer_storage) [[likely]]
				{
					std::tie(mip.src_w, mip.src_h) = rsx::apply_resolution_scale<true>(attr.width, attr.height);
				}
				else
				{
					mip.src_w = attr.width;
					mip.src_h = attr.height;
				}

				sections.push_back(mip);
			}
			else
			{
				switch (level.external_subresource_desc.op)
				{
				case deferred_request_command::copy_image_dynamic:
				case deferred_request_command::copy_image_static:
				{
					copy_region_descriptor_type mip{};
					mip.src = level.external_subresource_desc.external_handle;
					mip.xform = surface_transform::coordinate_transform;
					mip.level = mipmap_level;
					mip.dst_w = attr.width;
					mip.dst_h = attr.height;

					// NOTE: gather_texture_slices pre-applies resolution scaling
					mip.src_x = level.external_subresource_desc.x;
					mip.src_y = level.external_subresource_desc.y;
					mip.src_w = level.external_subresource_desc.width;
					mip.src_h = level.external_subresource_desc.height;

					sections.push_back(mip);
					break;
				}
				default:
				{
					// TODO
					return false;
				}
				}
			}

			// Check for upscaling if requested
			if (apply_upscaling)
			{
				auto& mip = sections.back();
				std::tie(mip.dst_w, mip.dst_h) = rsx::apply_resolution_scale<true>(mip.dst_w, mip.dst_h, level0_attr.width, level0_attr.height);
			}

			return true;
		}
	};
}
