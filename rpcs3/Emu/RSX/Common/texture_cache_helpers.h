#pragma once

#include "../rsx_utils.h"
#include "TextureUtils.h"

namespace rsx
{
	// Defines pixel operation to be performed on a surface before it is ready for use
	enum surface_transform : u32
	{
		identity = 0,                // Nothing
		coordinate_transform = 1     // Incoming source coordinates may generated based on the format of the secondary (dest) surface. Recalculate them before use.
	};

	template<typename image_resource_type>
	struct copy_region_descriptor_base
	{
		image_resource_type src;
		flags32_t xform;
		u32 base_addr;
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
		mipmap_gather,            // Provided list of sections to be reassembled as mipmap levels of the same texture
		blit_image_static,        // Variant of the copy command that does scaling instead of copying
	};

	struct image_section_attributes_t
	{
		u32 address;
		u32 gcm_format;
		u32 pitch;
		u16 width;
		u16 height;
		u16 depth;
		u16 mipmaps;
		u16 slice_h;
		u8  bpp;
		bool swizzled;
		bool edge_clamped;
	};

	struct blit_op_result
	{
		bool succeeded = false;
		u32 real_dst_address = 0;
		u32 real_dst_size = 0;

		blit_op_result(bool success) : succeeded(success)
		{}

		inline address_range32 to_address_range() const
		{
			return address_range32::start_length(real_dst_address, real_dst_size);
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
		static inline bool force_strict_fbo_sampling(u8 samples)
		{
			if (g_cfg.video.strict_rendering_mode)
			{
				// Strict mode. All access is strict.
				return true;
			}

			if (g_cfg.video.antialiasing_level == msaa_level::none)
			{
				// MSAA disabled. All access is fast.
				return false;
			}

			// Strict access if MSAA only.
			return samples > 1 && !!g_cfg.video.force_hw_MSAA_resolve;
		}

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

		static inline u32 get_sized_blit_format(bool is_32_bit, bool depth_format, bool /*is_format_convert*/)
		{
			if (is_32_bit)
			{
				return (!depth_format) ? CELL_GCM_TEXTURE_A8R8G8B8 : CELL_GCM_TEXTURE_DEPTH24_D8;
			}
			else
			{
				return (!depth_format) ? CELL_GCM_TEXTURE_R5G6B5 : CELL_GCM_TEXTURE_DEPTH16;
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
			address_range32 dst_range,
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

					const u32 bpp = g_fxo->get<rsx::avconf>().get_bpp();

					const u32 pitch = buffer.pitch ? +buffer.pitch : bpp * buffer.width;
					if (pitch != dst_pitch)
					{
						continue;
					}

					const auto buffer_range = address_range32::start_length(rsx::get_address(buffer.offset, CELL_GCM_LOCATION_LOCAL), pitch * (buffer.height - 1) + (buffer.width * bpp));
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

		template<typename commandbuffer_type, typename section_storage_type, typename copy_region_type, typename surface_store_list_type>
		void gather_texture_slices(
			commandbuffer_type& cmd,
			std::vector<copy_region_type>& out,
			const surface_store_list_type& fbos,
			const std::vector<section_storage_type*>& local,
			const image_section_attributes_t& attr,
			u16 count, bool /*is_depth*/)
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

				std::sort(sort_list.begin(), sort_list.end(), FN(x.tag < y.tag));
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

				const auto surface_width = section.surface->template get_surface_width<rsx::surface_metrics::pixels>();
				const auto surface_height = section.surface->template get_surface_height<rsx::surface_metrics::pixels>();
				const auto [src_width, src_height] = rsx::apply_resolution_scale<true>(section.src_area.width, h, surface_width, surface_height);
				const auto [dst_width, dst_height] = rsx::apply_resolution_scale<true>(section.dst_area.width, h, attr.width, attr.height);

				std::tie(src_x, src_y) = rsx::apply_resolution_scale<false>(src_x, src_y, surface_width, surface_height);
				std::tie(dst_x, dst_y) = rsx::apply_resolution_scale<false>(dst_x, dst_y, attr.width, attr.height);

				section.surface->memory_barrier(cmd, rsx::surface_access::transfer_read);

				out.push_back
				({
					.src = section.surface->get_surface(rsx::surface_access::transfer_read),
					.xform = surface_transform::identity,
					.base_addr = section.base_address,
					.level = 0,
					.src_x = static_cast<u16>(src_x),
					.src_y = static_cast<u16>(src_y),
					.dst_x = static_cast<u16>(dst_x),
					.dst_y = static_cast<u16>(dst_y),
					.dst_z = slice,
					.src_w = src_width,
					.src_h = src_height,
					.dst_w = dst_width,
					.dst_h = dst_height
				});
			};

			auto add_local_resource = [&](auto& section, u32 address, u16 slice, bool scaling = true)
			{
				// Intersect this resource with the original one.
				// Note that intersection takes place in a normalized coordinate space (bpp = 1)
				const u32 section_bpp = get_format_block_size_in_bytes(section->get_gcm_format());
				const u32 normalized_section_width = (section->get_width() * section_bpp);
				const u32 normalized_attr_width = (attr.width * attr.bpp);

				auto [src_offset, dst_offset, dimensions] = rsx::intersect_region(
					section->get_section_base(), normalized_section_width, section->get_height(), /* parent region (extractee) */
					address, normalized_attr_width, attr.slice_h, /* child region (extracted) */
					attr.pitch);

				if (!dimensions.width || !dimensions.height)
				{
					// Out of bounds, invalid intersection
					return;
				}

				// The intersection takes place in a normalized coordinate space. Now we convert back to domain-specific
				src_offset.x /= section_bpp;
				dst_offset.x /= attr.bpp;
				const size2u dst_size = { dimensions.width / attr.bpp, dimensions.height };
				const size2u src_size = { dimensions.width / section_bpp, dimensions.height };

				const u32 dst_slice_begin = slice * attr.slice_h;      // Output slice low watermark
				const u32 dst_slice_end = dst_slice_begin + attr.height;   // Output slice high watermark

				const auto dst_y = dst_offset.y;
				const auto dst_h = dst_size.height;

				const auto write_section_end = dst_y + dst_h;
				if (dst_y >= dst_slice_end || write_section_end <= dst_slice_begin)
				{
					// Belongs to a different slice
					return;
				}

				const u16 dst_w = static_cast<u16>(dst_size.width);
				const u16 src_w = static_cast<u16>(src_size.width);
				const u16 height = std::min(dst_slice_end, write_section_end) - dst_y;

				if (scaling)
				{
					// Since output is upscaled, also upscale on dst
					const auto [_dst_x, _dst_y] = rsx::apply_resolution_scale<false>(static_cast<u16>(dst_offset.x), static_cast<u16>(dst_y - dst_slice_begin), attr.width, attr.height);
					const auto [_dst_w, _dst_h] = rsx::apply_resolution_scale<true>(dst_w, height, attr.width, attr.height);

					out.push_back
					({
						.src = section->get_raw_texture(),
						.xform = surface_transform::identity,
						.level = 0,
						.src_x = static_cast<u16>(src_offset.x),   // src.x
						.src_y = static_cast<u16>(src_offset.y),   // src.y
						.dst_x = _dst_x,                           // dst.x
						.dst_y = _dst_y,                           // dst.y
						.dst_z = slice,
						.src_w = src_w,
						.src_h = height,
						.dst_w = _dst_w,
						.dst_h = _dst_h
					});
				}
				else
				{
					out.push_back
					({
						.src = section->get_raw_texture(),
						.xform = surface_transform::identity,
						.level = 0,
						.src_x = static_cast<u16>(src_offset.x),         // src.x
						.src_y = static_cast<u16>(src_offset.y),         // src.y
						.dst_x = static_cast<u16>(dst_offset.x),         // dst.x
						.dst_y = static_cast<u16>(dst_y - dst_slice_begin),  // dst.y
						.dst_z = 0,
						.src_w = src_w,
						.src_h = height,
						.dst_w = dst_w,
						.dst_h = height
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
					// TODO: Gather remaining sides from the texture cache or upload from cpu (too slow?)
					rsx_log.warning("Could not gather all required slices for cubemap/3d generation");
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

			const auto surface_width = texptr->template get_surface_width<rsx::surface_metrics::samples>();
			const auto surface_height = texptr->template get_surface_height<rsx::surface_metrics::samples>();

			switch (extended_dimension)
			{
			case rsx::texture_dimension_extended::texture_dimension_1d:
				return (surface_width >= attr.width);
			case rsx::texture_dimension_extended::texture_dimension_2d:
				return (surface_width >= attr.width && surface_height >= attr.height);
			case rsx::texture_dimension_extended::texture_dimension_3d:
				return (surface_width >= attr.width && surface_height >= u32{attr.slice_h} * attr.depth);
			case rsx::texture_dimension_extended::texture_dimension_cubemap:
				return (surface_width == attr.height && surface_width >= attr.width && surface_height >= (u32{attr.slice_h} * 6));
			}

			return false;
		}

		template <typename sampled_image_descriptor>
		void calculate_sample_clip_parameters(
			sampled_image_descriptor& desc,
			const position2i& offset,
			const size2i& desired_dimensions,
			const size2i& actual_dimensions)
		{
			// Back up the transformation before we destructively modify it.
			desc.push_texcoord_xform();

			desc.texcoord_xform.scale[0] *= f32(desired_dimensions.width) / actual_dimensions.width;
			desc.texcoord_xform.scale[1] *= f32(desired_dimensions.height) / actual_dimensions.height;
			desc.texcoord_xform.bias[0] += f32(offset.x) / actual_dimensions.width;
			desc.texcoord_xform.bias[1] += f32(offset.y) / actual_dimensions.height;
			desc.texcoord_xform.clamp_min[0] = (offset.x + 0.49999f) / actual_dimensions.width;
			desc.texcoord_xform.clamp_min[1] = (offset.y + 0.49999f) / actual_dimensions.height;
			desc.texcoord_xform.clamp_max[0] = (offset.x + desired_dimensions.width - 0.50001f) / actual_dimensions.width;
			desc.texcoord_xform.clamp_max[1] = (offset.y + desired_dimensions.height - 0.50001f) / actual_dimensions.height;
			desc.texcoord_xform.clamp = true;
		}

		template <typename sampled_image_descriptor>
		void convert_image_copy_to_clip_descriptor(
			sampled_image_descriptor& desc,
			const position2i& offset,
			const size2i& desired_dimensions,
			const size2i& actual_dimensions,
			const texture_channel_remap_t& decoded_remap,
			bool cyclic_reference)
		{
			desc.image_handle = desc.external_subresource_desc.as_viewable()->get_view(decoded_remap);
			desc.ref_address = desc.external_subresource_desc.external_ref_addr;
			desc.is_cyclic_reference = cyclic_reference;
			desc.samples = desc.external_subresource_desc.external_handle->samples();
			desc.external_subresource_desc = {};

			calculate_sample_clip_parameters(desc, offset, desired_dimensions, actual_dimensions);
		}

		template <typename sampled_image_descriptor>
		void convert_image_blit_to_clip_descriptor(
			sampled_image_descriptor& desc,
			const texture_channel_remap_t& decoded_remap,
			bool cyclic_reference)
		{
			// Our "desired" output is the source window, and the "actual" output is the real size
			const auto& section = desc.external_subresource_desc.sections_to_copy[0];

			// Apply AA correct factor
			auto surface_width = section.src->width();
			auto surface_height = section.src->height();
			switch (section.src->samples())
			{
			case 1:
				break;
			case 2:
				surface_width *= 2;
				break;
			case 4:
				surface_width *= 2;
				surface_height *= 2;
				break;
			default:
				fmt::throw_exception("Unsupported MSAA configuration");
			}

			// First, we convert this descriptor to a copy descriptor
			desc.external_subresource_desc.external_handle = section.src;
			desc.external_subresource_desc.external_ref_addr = section.base_addr;

			// Now apply conversion
			convert_image_copy_to_clip_descriptor(
				desc,
				position2i(section.src_x, section.src_y),
				size2i(section.src_w, section.src_h),
				size2i(surface_width, surface_height),
				decoded_remap,
				cyclic_reference);
		}

		template <typename sampled_image_descriptor, typename commandbuffer_type, typename render_target_type>
		sampled_image_descriptor process_framebuffer_resource_fast(commandbuffer_type& cmd,
			render_target_type texptr,
			const image_section_attributes_t& attr,
			const size3f& scale,
			texture_dimension_extended extended_dimension,
			const texture_channel_remap_t& decoded_remap,
			bool surface_is_rop_target,
			bool force_convert)
		{
			const auto surface_width = texptr->template get_surface_width<rsx::surface_metrics::samples>();
			const auto surface_height = texptr->template get_surface_height<rsx::surface_metrics::samples>();

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
				if (force_convert || gcm_format_is_depth)
				{
					// If force_convert is set, we already know there is no simple workaround. Bitcast will be forced to resolve the issue.
					// If the existing texture is a color texture but depth readout is requested, force bitcast
					// Note that if only reading the depth value was needed from a depth surface, it would have been sampled as color due to Z comparison.
					is_depth = gcm_format_is_depth;
					force_convert = true;
				}
				else
				{
					// Existing texture is a depth texture, but RSX wants a color texture.
					// Change the RSX request to a compatible depth texture to give same results in shader.
					ensure(is_depth);
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

				// A GPU operation must be performed on the data before sampling. Implies transfer_read access.
				bool requires_processing = force_convert;
				// A GPU clip operation may be performed by combining texture coordinate scaling with a clamp.
				bool requires_clip = false;

				rsx::surface_access access_type = rsx::surface_access::shader_read;

				if (attr.width != surface_width || attr.height != surface_height)
				{
					// If we can get away with clip only, do it
					if (attr.edge_clamped)
					{
						requires_clip = true;
					}
					else
					{
						requires_processing = true;
					}
				}

				if (surface_is_rop_target && texture_cache_helpers::force_strict_fbo_sampling(texptr->samples()))
				{
					// Framebuffer feedback avoidance. For MSAA, we do not need to make copies; just use the resolve target
					if (texptr->samples() == 1)
					{
						requires_processing = true;
					}
					else if (!requires_processing)
					{
						// Select resolve target instead of MSAA image
						access_type = rsx::surface_access::transfer_read;
					}
				}

				if (requires_processing)
				{
					const auto format_class = (force_convert) ? classify_format(attr2.gcm_format) : texptr->format_class();
					const auto command = surface_is_rop_target ? deferred_request_command::copy_image_dynamic : deferred_request_command::copy_image_static;

					texptr->memory_barrier(cmd, rsx::surface_access::transfer_read);
					return { texptr->get_surface(rsx::surface_access::transfer_read), command, attr2, {},
							texture_upload_context::framebuffer_storage, format_class, scale,
							extended_dimension, decoded_remap };
				}

				texptr->memory_barrier(cmd, access_type);
				auto viewed_surface = texptr->get_surface(access_type);
				sampled_image_descriptor result = { viewed_surface->get_view(decoded_remap), texture_upload_context::framebuffer_storage,
						texptr->format_class(), scale, rsx::texture_dimension_extended::texture_dimension_2d, surface_is_rop_target, viewed_surface->samples() };

				if (requires_clip)
				{
					calculate_sample_clip_parameters(result, position2i(0, 0), size2i(attr.width, attr.height), size2i(surface_width, surface_height));
				}

				return result;
			}

			texptr->memory_barrier(cmd, rsx::surface_access::transfer_read);

			if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_3d)
			{
				return{ texptr->get_surface(rsx::surface_access::transfer_read), deferred_request_command::_3d_unwrap,
						attr2, {},
						texture_upload_context::framebuffer_storage, texptr->format_class(), scale,
						rsx::texture_dimension_extended::texture_dimension_3d, decoded_remap };
			}

			ensure(extended_dimension == rsx::texture_dimension_extended::texture_dimension_cubemap);

			return{ texptr->get_surface(rsx::surface_access::transfer_read), deferred_request_command::cubemap_unwrap,
					attr2, {},
					texture_upload_context::framebuffer_storage, texptr->format_class(), scale,
					rsx::texture_dimension_extended::texture_dimension_cubemap, decoded_remap };
		}

		template <typename sampled_image_descriptor, typename commandbuffer_type, typename surface_store_list_type, typename section_storage_type>
		sampled_image_descriptor merge_cache_resources(
			commandbuffer_type& cmd,
			const surface_store_list_type& fbos, const std::vector<section_storage_type*>& local,
			const image_section_attributes_t& attr,
			const size3f& scale,
			texture_dimension_extended extended_dimension,
			const texture_channel_remap_t& decoded_remap,
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
			const auto upload_context = (fbos.empty()) ? texture_upload_context::shader_read : texture_upload_context::framebuffer_storage;

			if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_cubemap)
			{
				attr2.width = scaled_w;
				attr2.height = scaled_h;

				sampled_image_descriptor desc = { nullptr, deferred_request_command::cubemap_gather,
						attr2, {},
						upload_context, format_class, scale,
						rsx::texture_dimension_extended::texture_dimension_cubemap, decoded_remap };

				gather_texture_slices(cmd, desc.external_subresource_desc.sections_to_copy, fbos, local, attr, 6, is_depth);
				return desc;
			}
			else if (extended_dimension == rsx::texture_dimension_extended::texture_dimension_3d && attr.depth > 1)
			{
				attr2.width = scaled_w;
				attr2.height = scaled_h;

				sampled_image_descriptor desc = { nullptr, deferred_request_command::_3d_gather,
					attr2, {},
					upload_context, format_class, scale,
					rsx::texture_dimension_extended::texture_dimension_3d, decoded_remap };

				gather_texture_slices(cmd, desc.external_subresource_desc.sections_to_copy, fbos, local, attr, attr.depth, is_depth);
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
					attr2, {}, upload_context, format_class,
					scale, rsx::texture_dimension_extended::texture_dimension_2d, decoded_remap };

			gather_texture_slices(cmd, result.external_subresource_desc.sections_to_copy, fbos, local, attr, 1, is_depth);
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
				copy_region_descriptor_type mip
				{
					.src = level.image_handle->image(),
					.xform = surface_transform::coordinate_transform,
					.level = mipmap_level,
					.dst_w = attr.width,
					.dst_h = attr.height
				};

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
					copy_region_descriptor_type mip
					{
						.src = level.external_subresource_desc.external_handle,
						.xform = surface_transform::coordinate_transform,
						.level = mipmap_level,

						// NOTE: gather_texture_slices pre-applies resolution scaling
						.src_x = level.external_subresource_desc.x,
						.src_y = level.external_subresource_desc.y,
						.src_w = level.external_subresource_desc.width,
						.src_h = level.external_subresource_desc.height,

						.dst_w = attr.width,
						.dst_h = attr.height
					};

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
