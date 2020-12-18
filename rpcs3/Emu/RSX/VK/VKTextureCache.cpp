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
			auto final_mapping = vk::map_dma(cmd, valid_range.start, section_length);

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
				vk::cs_shuffle_base *shuffle_kernel;

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

			auto mapping = vk::map_dma(cmd, valid_range.start, valid_range.length());
			region.bufferOffset = mapping.first;
			vkCmdCopyImageToBuffer(cmd, src->value, src->current_layout, mapping.second->value, 1, &region);
		}

		src->pop_layout(cmd);

		// Create event object for this transfer and queue signal op
		dma_fence = std::make_unique<vk::event>(*m_device);
		dma_fence->signal(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT);

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

	void texture_cache::copy_transfer_regions_impl(vk::command_buffer& cmd, vk::image* dst, const std::vector<copy_region_descriptor>& sections_to_transfer) const
	{
		const auto dst_aspect = dst->aspect();
		const auto dst_bpp = vk::get_format_texel_width(dst->format());

		for (const auto &section : sections_to_transfer)
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
}
