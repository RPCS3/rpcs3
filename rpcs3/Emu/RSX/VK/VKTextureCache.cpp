#include "stdafx.h"
#include "VKGSRender.h"
#include "VKTextureCache.h"
#include "VKCompute.h"

#include "util/asm.hpp"

namespace vk
{
	void cached_texture_section::dma_transfer(vk::command_buffer& cmd, vk::image* src, const areai& src_area, const utils::address_range& valid_range, u32 pitch)
	{
		ensure(src->samples() == 1);

		if (!m_device)
		{
			m_device = &cmd.get_command_pool().get_owner();
		}

		if (dma_fence)
		{
			// NOTE: This can be reached if previously synchronized, or a special path happens.
			// If a hard flush occurred while this surface was flush_always the cache would have reset its protection afterwards.
			// DMA resource would still be present but already used to flush previously.
			vk::get_resource_manager()->dispose(dma_fence);
		}

		if (vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		src->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		const auto internal_bpp = vk::get_format_texel_width(src->format());
		const auto transfer_width = static_cast<u32>(src_area.width());
		const auto transfer_height = static_cast<u32>(src_area.height());
		real_pitch = internal_bpp * transfer_width;
		rsx_pitch = pitch;

		const bool require_format_conversion = !!(src->aspect() & VK_IMAGE_ASPECT_STENCIL_BIT) || src->format() == VK_FORMAT_D32_SFLOAT;
		if (require_format_conversion || pack_unpack_swap_bytes)
		{
			const auto section_length = valid_range.length();
			const auto transfer_pitch = real_pitch;
			const auto task_length = transfer_pitch * src_area.height();

			auto working_buffer = vk::get_scratch_buffer(task_length);
			auto final_mapping = vk::map_dma(valid_range.start, section_length);

			VkBufferImageCopy region = {};
			region.imageSubresource = { src->aspect(), 0, 0, 1 };
			region.imageOffset = { src_area.x1, src_area.y1, 0 };
			region.imageExtent = { transfer_width, transfer_height, 1 };
			vk::copy_image_to_buffer(cmd, src, working_buffer, region, (require_format_conversion && pack_unpack_swap_bytes));

			// NOTE: For depth/stencil formats, copying to buffer and byteswap are combined into one step above
			if (pack_unpack_swap_bytes && !require_format_conversion)
			{
				const auto texel_layout = vk::get_format_element_size(src->format());
				const auto elem_size = texel_layout.first;
				vk::cs_shuffle_base* shuffle_kernel;

				if (elem_size == 2)
				{
					shuffle_kernel = vk::get_compute_task<vk::cs_shuffle_16>();
				}
				else if (elem_size == 4)
				{
					shuffle_kernel = vk::get_compute_task<vk::cs_shuffle_32>();
				}
				else
				{
					ensure(get_context() == rsx::texture_upload_context::dma);
					shuffle_kernel = nullptr;
				}

				if (shuffle_kernel)
				{
					vk::insert_buffer_memory_barrier(cmd, working_buffer->value, 0, task_length,
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

					shuffle_kernel->run(cmd, working_buffer, task_length);

					vk::insert_buffer_memory_barrier(cmd, working_buffer->value, 0, task_length,
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
						VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
				}
			}

			if (rsx_pitch == real_pitch) [[likely]]
			{
				VkBufferCopy copy = {};
				copy.dstOffset = final_mapping.first;
				copy.size = section_length;
				vkCmdCopyBuffer(cmd, working_buffer->value, final_mapping.second->value, 1, &copy);
			}
			else
			{
				if (context != rsx::texture_upload_context::dma)
				{
					// Partial load for the bits outside the existing image
					// NOTE: A true DMA section would have been prepped beforehand
					// TODO: Parial range load/flush
					vk::load_dma(valid_range.start, section_length);
				}

				std::vector<VkBufferCopy> copy;
				copy.reserve(transfer_height);

				u32 dst_offset = final_mapping.first;
				u32 src_offset = 0;

				for (unsigned row = 0; row < transfer_height; ++row)
				{
					copy.push_back({ src_offset, dst_offset, transfer_pitch });
					src_offset += real_pitch;
					dst_offset += rsx_pitch;
				}

				vkCmdCopyBuffer(cmd, working_buffer->value, final_mapping.second->value, transfer_height, copy.data());
			}
		}
		else
		{
			VkBufferImageCopy region = {};
			region.bufferRowLength = (rsx_pitch / internal_bpp);
			region.imageSubresource = { src->aspect(), 0, 0, 1 };
			region.imageOffset = { src_area.x1, src_area.y1, 0 };
			region.imageExtent = { transfer_width, transfer_height, 1 };

			auto mapping = vk::map_dma(valid_range.start, valid_range.length());
			region.bufferOffset = mapping.first;
			vkCmdCopyImageToBuffer(cmd, src->value, src->current_layout, mapping.second->value, 1, &region);
		}

		src->pop_layout(cmd);

		// Create event object for this transfer and queue signal op
		dma_fence = std::make_unique<vk::event>(*m_device, sync_domain::any);
		dma_fence->signal(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);

		// Set cb flag for queued dma operations
		cmd.set_flag(vk::command_buffer::cb_has_dma_transfer);

		if (get_context() == rsx::texture_upload_context::dma)
		{
			// Save readback hint in case transformation is required later
			switch (internal_bpp)
			{
			case 2:
				gcm_format = CELL_GCM_TEXTURE_R5G6B5;
				break;
			case 4:
			default:
				gcm_format = CELL_GCM_TEXTURE_A8R8G8B8;
				break;
			}
		}

		synchronized = true;
		sync_timestamp = get_system_time();
	}

	void texture_cache::on_section_destroyed(cached_texture_section& tex)
	{
		if (tex.is_managed())
		{
			vk::get_resource_manager()->dispose(tex.get_texture());
		}
	}

	void texture_cache::clear()
	{
		baseclass::clear();

		m_temporary_storage.clear();
		m_temporary_memory_size = 0;
	}

	void texture_cache::copy_transfer_regions_impl(vk::command_buffer& cmd, vk::image* dst, const std::vector<copy_region_descriptor>& sections_to_transfer) const
	{
		const auto dst_aspect = dst->aspect();
		const auto dst_bpp = vk::get_format_texel_width(dst->format());

		for (const auto& section : sections_to_transfer)
		{
			if (!section.src)
				continue;

			const bool typeless = section.src->aspect() != dst_aspect ||
				!formats_are_bitcast_compatible(dst, section.src);

			// Avoid inserting unnecessary barrier GENERAL->TRANSFER_SRC->GENERAL in active render targets
			const auto preferred_layout = (section.src->current_layout != VK_IMAGE_LAYOUT_GENERAL) ?
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;

			section.src->push_layout(cmd, preferred_layout);

			auto src_image = section.src;
			auto src_x = section.src_x;
			auto src_y = section.src_y;
			auto src_w = section.src_w;
			auto src_h = section.src_h;

			rsx::flags32_t transform = section.xform;
			if (section.xform == rsx::surface_transform::coordinate_transform)
			{
				// Dimensions were given in 'dst' space. Work out the real source coordinates
				const auto src_bpp = vk::get_format_texel_width(section.src->format());
				src_x = (src_x * dst_bpp) / src_bpp;
				src_w = utils::aligned_div<u16>(src_w * dst_bpp, src_bpp);

				transform &= ~(rsx::surface_transform::coordinate_transform);
			}

			if (auto surface = dynamic_cast<vk::render_target*>(section.src))
			{
				surface->transform_samples_to_pixels(src_x, src_w, src_y, src_h);
			}

			if (typeless) [[unlikely]]
			{
				const auto src_bpp = vk::get_format_texel_width(section.src->format());
				const u16 convert_w = u16(src_w * src_bpp) / dst_bpp;
				const u16 convert_x = u16(src_x * src_bpp) / dst_bpp;

				if (convert_w == section.dst_w && src_h == section.dst_h &&
					transform == rsx::surface_transform::identity &&
					section.level == 0 && section.dst_z == 0)
				{
					// Optimization to avoid double transfer
					// TODO: Handle level and layer offsets
					const areai src_rect = coordi{{ src_x, src_y }, { src_w, src_h }};
					const areai dst_rect = coordi{{ section.dst_x, section.dst_y }, { section.dst_w, section.dst_h }};
					vk::copy_image_typeless(cmd, section.src, dst, src_rect, dst_rect, 1);

					section.src->pop_layout(cmd);
					continue;
				}

				src_image = vk::get_typeless_helper(dst->format(), dst->format_class(), convert_x + convert_w, src_y + src_h);
				src_image->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

				const areai src_rect = coordi{{ src_x, src_y }, { src_w, src_h }};
				const areai dst_rect = coordi{{ convert_x, src_y }, { convert_w, src_h }};
				vk::copy_image_typeless(cmd, section.src, src_image, src_rect, dst_rect, 1);
				src_image->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

				src_x = convert_x;
				src_w = convert_w;
			}

			ensure(src_image->current_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL || src_image->current_layout == VK_IMAGE_LAYOUT_GENERAL);

			// Final aspect mask of the 'final' transfer source
			const auto new_src_aspect = src_image->aspect();

			if (src_w == section.dst_w && src_h == section.dst_h && transform == rsx::surface_transform::identity) [[likely]]
			{
				VkImageCopy copy_rgn;
				copy_rgn.srcOffset = { src_x, src_y, 0 };
				copy_rgn.dstOffset = { section.dst_x, section.dst_y, 0 };
				copy_rgn.dstSubresource = { dst_aspect, 0, 0, 1 };
				copy_rgn.srcSubresource = { new_src_aspect, 0, 0, 1 };
				copy_rgn.extent = { src_w, src_h, 1 };

				if (dst->info.imageType == VK_IMAGE_TYPE_3D)
				{
					copy_rgn.dstOffset.z = section.dst_z;
				}
				else
				{
					copy_rgn.dstSubresource.baseArrayLayer = section.dst_z;
					copy_rgn.dstSubresource.mipLevel = section.level;
				}

				vkCmdCopyImage(cmd, src_image->value, src_image->current_layout, dst->value, dst->current_layout, 1, &copy_rgn);
			}
			else
			{
				ensure(section.dst_z == 0);

				u16 dst_x = section.dst_x, dst_y = section.dst_y;
				vk::image* _dst;

				if (src_image->info.format == dst->info.format && section.level == 0) [[likely]]
				{
					_dst = dst;
				}
				else
				{
					// Either a bitcast is required or a scale+copy to mipmap level
					_dst = vk::get_typeless_helper(src_image->format(), src_image->format_class(), dst->width(), dst->height() * 2);
					_dst->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				}

				if (transform == rsx::surface_transform::identity)
				{
					vk::copy_scaled_image(cmd, src_image, _dst,
						coordi{ { src_x, src_y }, { src_w, src_h } },
						coordi{ { section.dst_x, section.dst_y }, { section.dst_w, section.dst_h } },
						1, src_image->format() == _dst->format(),
						VK_FILTER_NEAREST);
				}
				else if (transform == rsx::surface_transform::argb_to_bgra)
				{
					VkBufferImageCopy copy{};
					copy.imageExtent = { src_w, src_h, 1 };
					copy.imageOffset = { src_x, src_y, 0 };
					copy.imageSubresource = { src_image->aspect(), 0, 0, 1 };

					const auto mem_length = src_w * src_h * dst_bpp;
					auto scratch_buf = vk::get_scratch_buffer(mem_length);
					vkCmdCopyImageToBuffer(cmd, src_image->value, src_image->current_layout, scratch_buf->value, 1, &copy);

					vk::insert_buffer_memory_barrier(cmd, scratch_buf->value, 0, mem_length, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

					auto shuffle_kernel = vk::get_compute_task<vk::cs_shuffle_32>();
					shuffle_kernel->run(cmd, scratch_buf, mem_length);

					vk::insert_buffer_memory_barrier(cmd, scratch_buf->value, 0, mem_length, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
						VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);

					auto tmp = vk::get_typeless_helper(src_image->format(), src_image->format_class(), section.dst_x + section.dst_w, section.dst_y + section.dst_h);
					tmp->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

					copy.imageOffset = { 0, 0, 0 };
					vkCmdCopyBufferToImage(cmd, scratch_buf->value, tmp->value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

					dst_x = 0;
					dst_y = 0;

					if (src_w != section.dst_w || src_h != section.dst_h)
					{
						// Optionally scale if needed
						if (tmp == _dst) [[unlikely]]
						{
							dst_y = src_h;
						}

						vk::copy_scaled_image(cmd, tmp, _dst,
							areai{ 0, 0, src_w, static_cast<s32>(src_h) },
							coordi{ { dst_x, dst_y }, { section.dst_w, section.dst_h } },
							1, tmp->info.format == _dst->info.format,
							VK_FILTER_NEAREST);
					}
					else
					{
						_dst = tmp;
					}
				}
				else
				{
					fmt::throw_exception("Unreachable");
				}

				if (_dst != dst) [[unlikely]]
				{
					// Casting comes after the scaling!
					VkImageCopy copy_rgn;
					copy_rgn.srcOffset = { s32(dst_x), s32(dst_y), 0 };
					copy_rgn.dstOffset = { section.dst_x, section.dst_y, 0 };
					copy_rgn.dstSubresource = { dst_aspect, section.level, 0, 1 };
					copy_rgn.srcSubresource = { _dst->aspect(), 0, 0, 1 };
					copy_rgn.extent = { section.dst_w, section.dst_h, 1 };

					_dst->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
					vkCmdCopyImage(cmd, _dst->value, _dst->current_layout, dst->value, dst->current_layout, 1, &copy_rgn);
				}
			}

			section.src->pop_layout(cmd);
		}
	}

	VkComponentMapping texture_cache::apply_component_mapping_flags(u32 gcm_format, rsx::component_order flags, const rsx::texture_channel_remap_t& remap_vector) const
	{
		switch (gcm_format)
		{
		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		case CELL_GCM_TEXTURE_DEPTH16:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
			// Dont bother letting this propagate
			return{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };
		default:
			break;
		}

		VkComponentMapping mapping = {};
		switch (flags)
		{
		case rsx::component_order::default_:
		{
			mapping = vk::apply_swizzle_remap(vk::get_component_mapping(gcm_format), remap_vector);
			break;
		}
		case rsx::component_order::native:
		{
			mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			break;
		}
		case rsx::component_order::swapped_native:
		{
			mapping = { VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B };
			break;
		}
		default:
			break;
		}

		return mapping;
	}

	vk::image* texture_cache::get_template_from_collection_impl(const std::vector<copy_region_descriptor>& sections_to_transfer) const
	{
		if (sections_to_transfer.size() == 1) [[likely]]
		{
			return sections_to_transfer.front().src;
		}

		vk::image* result = nullptr;
		for (const auto& section : sections_to_transfer)
		{
			if (!section.src)
				continue;

			if (!result)
			{
				result = section.src;
			}
			else
			{
				if (section.src->native_component_map.a != result->native_component_map.a ||
					section.src->native_component_map.r != result->native_component_map.r ||
					section.src->native_component_map.g != result->native_component_map.g ||
					section.src->native_component_map.b != result->native_component_map.b)
				{
					// TODO
					// This requires a far more complex setup as its not always possible to mix and match without compute assistance
					return nullptr;
				}
			}
		}

		return result;
	}

	std::unique_ptr<vk::viewable_image> texture_cache::find_temporary_image(VkFormat format, u16 w, u16 h, u16 d, u8 mipmaps)
	{
		for (auto& e : m_temporary_storage)
		{
			if (e.can_reuse && e.matches(format, w, h, d, mipmaps, 0))
			{
				m_temporary_memory_size -= e.block_size;
				e.block_size = 0;
				e.can_reuse = false;
				return std::move(e.combined_image);
			}
		}

		return {};
	}

	std::unique_ptr<vk::viewable_image> texture_cache::find_temporary_cubemap(VkFormat format, u16 size)
	{
		for (auto& e : m_temporary_storage)
		{
			if (e.can_reuse && e.matches(format, size, size, 1, 1, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT))
			{
				m_temporary_memory_size -= e.block_size;
				e.block_size = 0;
				e.can_reuse = false;
				return std::move(e.combined_image);
			}
		}

		return {};
	}

	vk::image_view* texture_cache::create_temporary_subresource_view_impl(vk::command_buffer& cmd, vk::image* source, VkImageType image_type, VkImageViewType view_type,
		u32 gcm_format, u16 x, u16 y, u16 w, u16 h, u16 d, u8 mips, const rsx::texture_channel_remap_t& remap_vector, bool copy)
	{
		std::unique_ptr<vk::viewable_image> image;

		VkImageCreateFlags image_flags = (view_type == VK_IMAGE_VIEW_TYPE_CUBE) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
		VkFormat dst_format = vk::get_compatible_sampler_format(m_formats_support, gcm_format);
		u16 layers = 1;

		if (!image_flags) [[likely]]
		{
			image = find_temporary_image(dst_format, w, h, 1, mips);
		}
		else
		{
			image = find_temporary_cubemap(dst_format, w);
			layers = 6;
		}

		if (!image)
		{
			image = std::make_unique<vk::viewable_image>(*vk::get_current_renderer(), m_memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				image_type,
				dst_format,
				w, h, d, mips, layers, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, image_flags,
				VMM_ALLOCATION_POOL_TEXTURE_CACHE, rsx::classify_format(gcm_format));
		}

		// This method is almost exclusively used to work on framebuffer resources
		// Keep the original swizzle layout unless there is data format conversion
		VkComponentMapping view_swizzle;
		if (!source || dst_format != source->info.format)
		{
			// This is a data cast operation
			// Use native mapping for the new type
			// TODO: Also simulate the readback+reupload step (very tricky)
			const auto remap = get_component_mapping(gcm_format);
			view_swizzle = { remap[1], remap[2], remap[3], remap[0] };
		}
		else
		{
			view_swizzle = source->native_component_map;
		}

		image->set_native_component_layout(view_swizzle);
		auto view = image->get_view(rsx::get_remap_encoding(remap_vector), remap_vector);

		if (copy)
		{
			std::vector<copy_region_descriptor> region =
			{ {
				source,
				rsx::surface_transform::coordinate_transform,
				0,
				x, y, 0, 0, 0,
				w, h, w, h
			} };

			vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			copy_transfer_regions_impl(cmd, image.get(), region);
			vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		const u32 resource_memory = w * h * 4; //Rough approximate
		m_temporary_storage.emplace_back(image);
		m_temporary_storage.back().block_size = resource_memory;
		m_temporary_memory_size += resource_memory;

		return view;
	}

	vk::image_view* texture_cache::create_temporary_subresource_view(vk::command_buffer& cmd, vk::image* source, u32 gcm_format,
		u16 x, u16 y, u16 w, u16 h, const rsx::texture_channel_remap_t& remap_vector)
	{
		return create_temporary_subresource_view_impl(cmd, source, source->info.imageType, VK_IMAGE_VIEW_TYPE_2D,
			gcm_format, x, y, w, h, 1, 1, remap_vector, true);
	}

	vk::image_view* texture_cache::create_temporary_subresource_view(vk::command_buffer& cmd, vk::image** source, u32 gcm_format,
		u16 x, u16 y, u16 w, u16 h, const rsx::texture_channel_remap_t& remap_vector)
	{
		return create_temporary_subresource_view(cmd, *source, gcm_format, x, y, w, h, remap_vector);
	}

	vk::image_view* texture_cache::generate_cubemap_from_images(vk::command_buffer& cmd, u32 gcm_format, u16 size,
		const std::vector<copy_region_descriptor>& sections_to_copy, const rsx::texture_channel_remap_t& remap_vector)
	{
		auto _template = get_template_from_collection_impl(sections_to_copy);
		auto result = create_temporary_subresource_view_impl(cmd, _template, VK_IMAGE_TYPE_2D,
			VK_IMAGE_VIEW_TYPE_CUBE, gcm_format, 0, 0, size, size, 1, 1, remap_vector, false);

		const auto image = result->image();
		VkImageAspectFlags dst_aspect = vk::get_aspect_flags(result->info.format);
		VkImageSubresourceRange dst_range = { dst_aspect, 0, 1, 0, 6 };
		vk::change_image_layout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_range);

		if (!(dst_aspect & VK_IMAGE_ASPECT_DEPTH_BIT))
		{
			VkClearColorValue clear = {};
			vkCmdClearColorImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
		}
		else
		{
			VkClearDepthStencilValue clear = { 1.f, 0 };
			vkCmdClearDepthStencilImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
		}

		copy_transfer_regions_impl(cmd, image, sections_to_copy);

		vk::change_image_layout(cmd, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, dst_range);
		return result;
	}

	vk::image_view* texture_cache::generate_3d_from_2d_images(vk::command_buffer& cmd, u32 gcm_format, u16 width, u16 height, u16 depth,
		const std::vector<copy_region_descriptor>& sections_to_copy, const rsx::texture_channel_remap_t& remap_vector)
	{
		auto _template = get_template_from_collection_impl(sections_to_copy);
		auto result = create_temporary_subresource_view_impl(cmd, _template, VK_IMAGE_TYPE_3D,
			VK_IMAGE_VIEW_TYPE_3D, gcm_format, 0, 0, width, height, depth, 1, remap_vector, false);

		const auto image = result->image();
		VkImageAspectFlags dst_aspect = vk::get_aspect_flags(result->info.format);
		VkImageSubresourceRange dst_range = { dst_aspect, 0, 1, 0, 1 };
		vk::change_image_layout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_range);

		if (!(dst_aspect & VK_IMAGE_ASPECT_DEPTH_BIT))
		{
			VkClearColorValue clear = {};
			vkCmdClearColorImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
		}
		else
		{
			VkClearDepthStencilValue clear = { 1.f, 0 };
			vkCmdClearDepthStencilImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
		}

		copy_transfer_regions_impl(cmd, image, sections_to_copy);

		vk::change_image_layout(cmd, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, dst_range);
		return result;
	}

	vk::image_view* texture_cache::generate_atlas_from_images(vk::command_buffer& cmd, u32 gcm_format, u16 width, u16 height,
		const std::vector<copy_region_descriptor>& sections_to_copy, const rsx::texture_channel_remap_t& remap_vector)
	{
		auto _template = get_template_from_collection_impl(sections_to_copy);
		auto result = create_temporary_subresource_view_impl(cmd, _template, VK_IMAGE_TYPE_2D,
			VK_IMAGE_VIEW_TYPE_2D, gcm_format, 0, 0, width, height, 1, 1, remap_vector, false);

		const auto image = result->image();
		VkImageAspectFlags dst_aspect = vk::get_aspect_flags(result->info.format);
		VkImageSubresourceRange dst_range = { dst_aspect, 0, 1, 0, 1 };
		vk::change_image_layout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_range);

		if (!(dst_aspect & VK_IMAGE_ASPECT_DEPTH_BIT))
		{
			VkClearColorValue clear = {};
			vkCmdClearColorImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
		}
		else
		{
			VkClearDepthStencilValue clear = { 1.f, 0 };
			vkCmdClearDepthStencilImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
		}

		copy_transfer_regions_impl(cmd, image, sections_to_copy);

		vk::change_image_layout(cmd, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, dst_range);
		return result;
	}

	vk::image_view* texture_cache::generate_2d_mipmaps_from_images(vk::command_buffer& cmd, u32 gcm_format, u16 width, u16 height,
		const std::vector<copy_region_descriptor>& sections_to_copy, const rsx::texture_channel_remap_t& remap_vector)
	{
		const auto mipmaps = ::narrow<u8>(sections_to_copy.size());
		auto _template = get_template_from_collection_impl(sections_to_copy);
		auto result = create_temporary_subresource_view_impl(cmd, _template, VK_IMAGE_TYPE_2D,
			VK_IMAGE_VIEW_TYPE_2D, gcm_format, 0, 0, width, height, 1, mipmaps, remap_vector, false);

		const auto image = result->image();
		VkImageAspectFlags dst_aspect = vk::get_aspect_flags(result->info.format);
		VkImageSubresourceRange dst_range = { dst_aspect, 0, mipmaps, 0, 1 };
		vk::change_image_layout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_range);

		if (!(dst_aspect & VK_IMAGE_ASPECT_DEPTH_BIT))
		{
			VkClearColorValue clear = {};
			vkCmdClearColorImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
		}
		else
		{
			VkClearDepthStencilValue clear = { 1.f, 0 };
			vkCmdClearDepthStencilImage(cmd, image->value, image->current_layout, &clear, 1, &dst_range);
		}

		copy_transfer_regions_impl(cmd, image, sections_to_copy);

		vk::change_image_layout(cmd, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, dst_range);
		return result;
	}

	void texture_cache::release_temporary_subresource(vk::image_view* view)
	{
		auto handle = dynamic_cast<vk::viewable_image*>(view->image());
		for (auto& e : m_temporary_storage)
		{
			if (e.combined_image.get() == handle)
			{
				e.can_reuse = true;
				return;
			}
		}
	}

	void texture_cache::update_image_contents(vk::command_buffer& cmd, vk::image_view* dst_view, vk::image* src, u16 width, u16 height)
	{
		std::vector<copy_region_descriptor> region =
		{ {
			src,
			rsx::surface_transform::identity,
			0,
			0, 0, 0, 0, 0,
			width, height, width, height
		} };

		auto dst = dst_view->image();
		dst->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copy_transfer_regions_impl(cmd, dst, region);
		dst->pop_layout(cmd);
	}

	cached_texture_section* texture_cache::create_new_texture(vk::command_buffer& cmd, const utils::address_range& rsx_range, u16 width, u16 height, u16 depth, u16 mipmaps, u16 pitch,
		u32 gcm_format, rsx::texture_upload_context context, rsx::texture_dimension_extended type, bool swizzled, rsx::component_order swizzle_flags, rsx::flags32_t flags)
	{
		const auto section_depth = depth;

		// Define desirable attributes based on type
		VkImageType image_type;
		VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		u8 layer = 0;

		switch (type)
		{
		case rsx::texture_dimension_extended::texture_dimension_1d:
			image_type = VK_IMAGE_TYPE_1D;
			height = 1;
			depth = 1;
			layer = 1;
			break;
		case rsx::texture_dimension_extended::texture_dimension_2d:
			image_type = VK_IMAGE_TYPE_2D;
			depth = 1;
			layer = 1;
			break;
		case rsx::texture_dimension_extended::texture_dimension_cubemap:
			image_type = VK_IMAGE_TYPE_2D;
			depth = 1;
			layer = 6;
			break;
		case rsx::texture_dimension_extended::texture_dimension_3d:
			image_type = VK_IMAGE_TYPE_3D;
			layer = 1;
			break;
		default:
			fmt::throw_exception("Unreachable");
		}

		// Check what actually exists at that address
		const rsx::image_section_attributes_t search_desc = { .gcm_format = gcm_format, .width = width, .height = height, .depth = section_depth, .mipmaps = mipmaps };
		const bool allow_dirty = (context != rsx::texture_upload_context::framebuffer_storage);
		cached_texture_section& region = *find_cached_texture(rsx_range, search_desc, true, true, allow_dirty);
		ensure(!region.is_locked());

		vk::viewable_image* image = nullptr;
		if (region.exists())
		{
			image = dynamic_cast<vk::viewable_image*>(region.get_raw_texture());
			if (!image || region.get_image_type() != type || image->depth() != depth) // TODO
			{
				// Incompatible view/type
				region.destroy();
				image = nullptr;
			}
			else
			{
				ensure(region.is_managed());

				// Reuse
				region.set_rsx_pitch(pitch);

				if (flags & texture_create_flags::initialize_image_contents)
				{
					// Wipe memory
					image->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

					VkImageSubresourceRange range{ image->aspect(), 0, image->mipmaps(), 0, image->layers() };
					if (image->aspect() & VK_IMAGE_ASPECT_COLOR_BIT)
					{
						VkClearColorValue color = { {0.f, 0.f, 0.f, 1.f} };
						vkCmdClearColorImage(cmd, image->value, image->current_layout, &color, 1, &range);
					}
					else
					{
						VkClearDepthStencilValue clear{ 1.f, 255 };
						vkCmdClearDepthStencilImage(cmd, image->value, image->current_layout, &clear, 1, &range);
					}
				}
			}
		}

		if (!image)
		{
			const bool is_cubemap = type == rsx::texture_dimension_extended::texture_dimension_cubemap;
			const VkFormat vk_format = get_compatible_sampler_format(m_formats_support, gcm_format);

			image = new vk::viewable_image(*m_device,
				m_memory_types.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				image_type, vk_format,
				width, height, depth, mipmaps, layer, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, usage_flags, is_cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0,
				VMM_ALLOCATION_POOL_TEXTURE_CACHE, rsx::classify_format(gcm_format));

			// New section, we must prepare it
			region.reset(rsx_range);
			region.set_gcm_format(gcm_format);
			region.set_image_type(type);
			region.create(width, height, section_depth, mipmaps, image, pitch, true, gcm_format);
		}

		region.set_view_flags(swizzle_flags);
		region.set_context(context);
		region.set_swizzled(swizzled);
		region.set_dirty(false);

		image->native_component_map = apply_component_mapping_flags(gcm_format, swizzle_flags, rsx::default_remap_vector);

		// Its not necessary to lock blit dst textures as they are just reused as necessary
		switch (context)
		{
		case rsx::texture_upload_context::shader_read:
		case rsx::texture_upload_context::blit_engine_src:
			region.protect(utils::protection::ro);
			read_only_range = region.get_min_max(read_only_range, rsx::section_bounds::locked_range);
			break;
		case rsx::texture_upload_context::blit_engine_dst:
			region.set_unpack_swap_bytes(true);
			no_access_range = region.get_min_max(no_access_range, rsx::section_bounds::locked_range);
			break;
		case rsx::texture_upload_context::dma:
		case rsx::texture_upload_context::framebuffer_storage:
			// Should not be initialized with this method
		default:
			fmt::throw_exception("Unexpected upload context 0x%x", u32(context));
		}

		update_cache_tag();
		return &region;
	}

	cached_texture_section* texture_cache::create_nul_section(vk::command_buffer& /*cmd*/, const utils::address_range& rsx_range, bool memory_load)
	{
		auto& region = *find_cached_texture(rsx_range, { .gcm_format = RSX_GCM_FORMAT_IGNORED }, true, false, false);
		ensure(!region.is_locked());

		// Prepare section
		region.reset(rsx_range);
		region.set_context(rsx::texture_upload_context::dma);
		region.set_dirty(false);
		region.set_unpack_swap_bytes(true);

		if (memory_load)
		{
			vk::map_dma(rsx_range.start, rsx_range.length());
			vk::load_dma(rsx_range.start, rsx_range.length());
		}

		no_access_range = region.get_min_max(no_access_range, rsx::section_bounds::locked_range);
		update_cache_tag();
		return &region;
	}

	cached_texture_section* texture_cache::upload_image_from_cpu(vk::command_buffer& cmd, const utils::address_range& rsx_range, u16 width, u16 height, u16 depth, u16 mipmaps, u16 pitch, u32 gcm_format,
		rsx::texture_upload_context context, const std::vector<rsx::subresource_layout>& subresource_layout, rsx::texture_dimension_extended type, bool swizzled)
	{
		if (context != rsx::texture_upload_context::shader_read)
		{
			if (vk::is_renderpass_open(cmd))
			{
				vk::end_renderpass(cmd);
			}
		}
		auto section = create_new_texture(cmd, rsx_range, width, height, depth, mipmaps, pitch, gcm_format, context, type, swizzled,
			rsx::component_order::default_, 0);

		auto image = section->get_raw_texture();
		image->set_debug_name(fmt::format("Raw Texture @0x%x", rsx_range.start));

		vk::enter_uninterruptible();

		bool input_swizzled = swizzled;
		if (context == rsx::texture_upload_context::blit_engine_src)
		{
			// Swizzling is ignored for blit engine copy and emulated using remapping
			input_swizzled = false;
		}

		rsx::flags32_t upload_command_flags = initialize_image_layout |
			(rsx::get_current_renderer()->get_backend_config().supports_asynchronous_compute ? upload_contents_async : upload_contents_inline);

		vk::upload_image(cmd, image, subresource_layout, gcm_format, input_swizzled, mipmaps, image->aspect(),
			*m_texture_upload_heap, upload_heap_align_default, upload_command_flags);

		vk::leave_uninterruptible();

		if (context != rsx::texture_upload_context::shader_read)
		{
			// Insert appropriate barrier depending on use. Shader read resources should be lazy-initialized before consuming.
			// TODO: All texture resources should be initialized on use, this is wasteful

			VkImageLayout preferred_layout;
			switch (context)
			{
			default:
				preferred_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				break;
			case rsx::texture_upload_context::blit_engine_dst:
				preferred_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				break;
			case rsx::texture_upload_context::blit_engine_src:
				preferred_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				break;
			}

			if (preferred_layout != image->current_layout)
			{
				image->change_layout(cmd, preferred_layout);
			}
			else
			{
				// Insert ordering barrier
				ensure(preferred_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				insert_image_memory_barrier(cmd, image->value, image->current_layout, preferred_layout,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
					{ image->aspect(), 0, image->mipmaps(), 0, image->layers() });
			}
		}

		section->last_write_tag = rsx::get_shared_tag();
		return section;
	}

	void texture_cache::set_component_order(cached_texture_section& section, u32 gcm_format, rsx::component_order expected_flags)
	{
		if (expected_flags == section.get_view_flags())
			return;

		const VkComponentMapping mapping = apply_component_mapping_flags(gcm_format, expected_flags, rsx::default_remap_vector);
		auto image = static_cast<vk::viewable_image*>(section.get_raw_texture());

		ensure(image);
		image->set_native_component_layout(mapping);

		section.set_view_flags(expected_flags);
	}

	void texture_cache::insert_texture_barrier(vk::command_buffer& cmd, vk::image* tex, bool strong_ordering)
	{
		if (!strong_ordering && tex->current_layout == VK_IMAGE_LAYOUT_GENERAL)
		{
			// A previous barrier already exists, do nothing
			return;
		}

		vk::as_rtt(tex)->texture_barrier(cmd);
	}

	bool texture_cache::render_target_format_is_compatible(vk::image* tex, u32 gcm_format)
	{
		auto vk_format = tex->info.format;
		switch (gcm_format)
		{
		default:
			//TODO
			warn_once("Format incompatibility detected, reporting failure to force data copy (VK_FORMAT=0x%X, GCM_FORMAT=0x%X)", static_cast<u32>(vk_format), gcm_format);
			return false;
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
			return (vk_format == VK_FORMAT_R16G16B16A16_SFLOAT);
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
			return (vk_format == VK_FORMAT_R32G32B32A32_SFLOAT);
		case CELL_GCM_TEXTURE_X32_FLOAT:
			return (vk_format == VK_FORMAT_R32_SFLOAT);
		case CELL_GCM_TEXTURE_R5G6B5:
			return (vk_format == VK_FORMAT_R5G6B5_UNORM_PACK16);
		case CELL_GCM_TEXTURE_A8R8G8B8:
		case CELL_GCM_TEXTURE_D8R8G8B8:
			return (vk_format == VK_FORMAT_B8G8R8A8_UNORM || vk_format == VK_FORMAT_D24_UNORM_S8_UINT || vk_format == VK_FORMAT_D32_SFLOAT_S8_UINT);
		case CELL_GCM_TEXTURE_B8:
			return (vk_format == VK_FORMAT_R8_UNORM);
		case CELL_GCM_TEXTURE_G8B8:
			return (vk_format == VK_FORMAT_R8G8_UNORM);
		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
			return (vk_format == VK_FORMAT_D24_UNORM_S8_UINT || vk_format == VK_FORMAT_D32_SFLOAT_S8_UINT);
		case CELL_GCM_TEXTURE_X16:
		case CELL_GCM_TEXTURE_DEPTH16:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
			return (vk_format == VK_FORMAT_D16_UNORM || vk_format == VK_FORMAT_D32_SFLOAT);
		}
	}

	void texture_cache::prepare_for_dma_transfers(vk::command_buffer& cmd)
	{
		if (!cmd.is_recording())
		{
			cmd.begin();
		}
	}

	void texture_cache::cleanup_after_dma_transfers(vk::command_buffer& cmd)
	{
		bool occlusion_query_active = !!(cmd.flags & vk::command_buffer::cb_has_open_query);
		if (occlusion_query_active)
		{
			// We really stepped in it
			vk::do_query_cleanup(cmd);
		}

		// End recording
		cmd.end();

		if (cmd.access_hint != vk::command_buffer::access_type_hint::all)
		{
			// Flush any pending async jobs in case of blockers
			// TODO: Context-level manager should handle this logic
			g_fxo->get<async_scheduler_thread>().flush(VK_TRUE);

			// Primary access command queue, must restart it after
			vk::fence submit_fence(*m_device);
			cmd.submit(m_submit_queue, VK_NULL_HANDLE, VK_NULL_HANDLE, &submit_fence, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_TRUE);

			vk::wait_for_fence(&submit_fence, GENERAL_WAIT_TIMEOUT);

			CHECK_RESULT(vkResetCommandBuffer(cmd, 0));
			cmd.begin();
		}
		else
		{
			// Auxilliary command queue with auto-restart capability
			cmd.submit(m_submit_queue, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_TRUE);
		}

		ensure(cmd.flags == 0);

		if (occlusion_query_active)
		{
			ensure(cmd.is_recording());
			cmd.flags |= vk::command_buffer::cb_load_occluson_task;
		}
	}

	void texture_cache::initialize(vk::render_device& device, VkQueue submit_queue, vk::data_heap& upload_heap)
	{
		m_device = &device;
		m_memory_types = device.get_memory_mapping();
		m_formats_support = device.get_formats_support();
		m_submit_queue = submit_queue;
		m_texture_upload_heap = &upload_heap;
	}

	void texture_cache::destroy()
	{
		clear();
	}

	bool texture_cache::is_depth_texture(u32 rsx_address, u32 rsx_size)
	{
		reader_lock lock(m_cache_mutex);

		auto& block = m_storage.block_for(rsx_address);

		if (block.get_locked_count() == 0)
			return false;

		for (auto& tex : block)
		{
			if (tex.is_dirty())
				continue;

			if (!tex.overlaps(rsx_address, rsx::section_bounds::full_range))
				continue;

			if ((rsx_address + rsx_size - tex.get_section_base()) <= tex.get_section_size())
			{
				switch (tex.get_format())
				{
				case VK_FORMAT_D16_UNORM:
				case VK_FORMAT_D32_SFLOAT:
				case VK_FORMAT_D32_SFLOAT_S8_UINT:
				case VK_FORMAT_D24_UNORM_S8_UINT:
					return true;
				default:
					return false;
				}
			}
		}

		//Unreachable; silence compiler warning anyway
		return false;
	}

	bool texture_cache::handle_memory_pressure(rsx::problem_severity severity)
	{
		bool any_released = baseclass::handle_memory_pressure(severity);

		if (severity <= rsx::problem_severity::low || !m_temporary_memory_size)
		{
			// Nothing left to do
			return any_released;
		}

		constexpr u64 _1M = 0x100000;
		if (severity <= rsx::problem_severity::moderate && m_temporary_memory_size < (64 * _1M))
		{
			// Some memory is consumed by the temporary resources, but no need to panic just yet
			return any_released;
		}

		// Nuke temporary resources. They will still be visible to the GPU.
		auto gc = vk::get_resource_manager();
		u64 actual_released_memory = 0;

		for (auto& entry : m_temporary_storage)
		{
			actual_released_memory += entry.combined_image->memory->size();
			gc->dispose(entry.combined_image);
			m_temporary_memory_size -= entry.block_size;
		}

		ensure(m_temporary_memory_size == 0);
		m_temporary_storage.clear();
		m_temporary_subresource_cache.clear();

		rsx_log.warning("Texture cache released %lluM of temporary resources.", (actual_released_memory / _1M));
		return any_released || (actual_released_memory > 0);
	}

	void texture_cache::on_frame_end()
	{
		trim_sections();

		if (m_storage.m_unreleased_texture_objects >= m_max_zombie_objects ||
			m_temporary_memory_size > 0x4000000) //If already holding over 64M in discardable memory, be frugal with memory resources
		{
			purge_unreleased_sections();
		}

		const u64 last_complete_frame = vk::get_last_completed_frame_id();
		m_temporary_storage.remove_if([&](const temporary_storage& o)
		{
			if (!o.block_size || o.test(last_complete_frame))
			{
				m_temporary_memory_size -= o.block_size;
				return true;
			}
			return false;
		});

		m_temporary_subresource_cache.clear();
		reset_frame_statistics();

		baseclass::on_frame_end();
	}

	vk::image* texture_cache::upload_image_simple(vk::command_buffer& cmd, VkFormat format, u32 address, u32 width, u32 height, u32 pitch)
	{
		bool linear_format_supported = false;

		switch (format)
		{
		case VK_FORMAT_B8G8R8A8_UNORM:
			linear_format_supported = m_formats_support.bgra8_linear;
			break;
		case VK_FORMAT_R8G8B8A8_UNORM:
			linear_format_supported = m_formats_support.argb8_linear;
			break;
		default:
			rsx_log.error("Unsupported VkFormat 0x%x", static_cast<u32>(format));
			return nullptr;
		}

		if (!linear_format_supported)
		{
			return nullptr;
		}

		// Uploads a linear memory range as a BGRA8 texture
		auto image = std::make_unique<vk::viewable_image>(*m_device, m_memory_types.host_visible_coherent,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_IMAGE_TYPE_2D,
			format,
			width, height, 1, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED,
			VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0,
			VMM_ALLOCATION_POOL_UNDEFINED);

		VkImageSubresource subresource{};
		subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		VkSubresourceLayout layout{};
		vkGetImageSubresourceLayout(*m_device, image->value, &subresource, &layout);

		void* mem = image->memory->map(0, layout.rowPitch * height);

		auto src = vm::_ptr<const char>(address);
		auto dst = static_cast<char*>(mem);

		// TODO: SSE optimization
		for (u32 row = 0; row < height; ++row)
		{
			auto casted_src = reinterpret_cast<const be_t<u32>*>(src);
			auto casted_dst = reinterpret_cast<u32*>(dst);

			for (u32 col = 0; col < width; ++col)
				casted_dst[col] = casted_src[col];

			src += pitch;
			dst += layout.rowPitch;
		}

		image->memory->unmap();

		vk::change_image_layout(cmd, image.get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		auto result = image.get();
		const u32 resource_memory = width * height * 4; //Rough approximate
		m_temporary_storage.emplace_back(image);
		m_temporary_storage.back().block_size = resource_memory;
		m_temporary_memory_size += resource_memory;

		return result;
	}

	bool texture_cache::blit(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate, vk::surface_cache& m_rtts, vk::command_buffer& cmd)
	{
		blitter helper;
		auto reply = upload_scaled_image(src, dst, interpolate, cmd, m_rtts, helper);

		if (reply.succeeded)
		{
			if (reply.real_dst_size)
			{
				flush_if_cache_miss_likely(cmd, reply.to_address_range());
			}

			return true;
		}

		return false;
	}

	u32 texture_cache::get_unreleased_textures_count() const
	{
		return baseclass::get_unreleased_textures_count() + ::size32(m_temporary_storage);
	}

	u32 texture_cache::get_temporary_memory_in_use() const
	{
		return m_temporary_memory_size;
	}

	bool texture_cache::is_overallocated() const
	{
		const auto total_device_memory = m_device->get_memory_mapping().device_local_total_bytes / 0x100000;
		u64 quota = 0;

		if (total_device_memory >= 1024)
		{
			quota = std::min(3072ull, (total_device_memory * 40) / 100);
		}
		else if (total_device_memory >= 768)
		{
			quota = 256;
		}
		else
		{
			quota = std::min(128ull, total_device_memory / 2);
		}

		quota *= 0x100000;

		if (const u64 texture_cache_pool_usage = vmm_get_application_pool_usage(VMM_ALLOCATION_POOL_TEXTURE_CACHE);
			texture_cache_pool_usage > quota)
		{
			rsx_log.warning("Texture cache is using %lluM of memory which exceeds the allocation quota of %lluM",
				texture_cache_pool_usage, quota);
			return true;
		}

		return false;
	}
}
