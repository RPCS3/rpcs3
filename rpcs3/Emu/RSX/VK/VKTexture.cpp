﻿#include "stdafx.h"
#include "VKHelpers.h"
#include "../GCM.h"
#include "../RSXThread.h"
#include "../RSXTexture.h"
#include "../rsx_utils.h"
#include "VKFormats.h"
#include "VKCompute.h"
#include "VKRenderPass.h"
#include "VKRenderTargets.h"

namespace vk
{
	VkComponentMapping default_component_map()
	{
		VkComponentMapping result = {};
		result.a = VK_COMPONENT_SWIZZLE_A;
		result.r = VK_COMPONENT_SWIZZLE_R;
		result.g = VK_COMPONENT_SWIZZLE_G;
		result.b = VK_COMPONENT_SWIZZLE_B;

		return result;
	}

	VkImageSubresource default_image_subresource()
	{
		VkImageSubresource subres = {};
		subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subres.mipLevel = 0;
		subres.arrayLayer = 0;

		return subres;
	}

	VkImageSubresourceRange get_image_subresource_range(uint32_t base_layer, uint32_t base_mip, uint32_t layer_count, uint32_t level_count, VkImageAspectFlags aspect)
	{
		VkImageSubresourceRange subres = {};
		subres.aspectMask = aspect;
		subres.baseArrayLayer = base_layer;
		subres.baseMipLevel = base_mip;
		subres.layerCount = layer_count;
		subres.levelCount = level_count;

		return subres;
	}

	VkImageAspectFlags get_aspect_flags(VkFormat format)
	{
		switch (format)
		{
		default:
			return VK_IMAGE_ASPECT_COLOR_BIT;
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D32_SFLOAT:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}

	void copy_image_to_buffer(VkCommandBuffer cmd, const vk::image* src, const vk::buffer* dst, const VkBufferImageCopy& region, bool swap_bytes)
	{
		// Always validate
		verify("Invalid image layout!" HERE),
			src->current_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL || src->current_layout == VK_IMAGE_LAYOUT_GENERAL;

		if (vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		switch (src->format())
		{
		default:
		{
			verify("Implicit byteswap option not supported for speficied format" HERE), !swap_bytes;
			vkCmdCopyImageToBuffer(cmd, src->value, src->current_layout, dst->value, 1, &region);
			break;
		}
		case VK_FORMAT_D32_SFLOAT:
		{
			rsx_log.error("Unsupported transfer (D16_FLOAT)"); // Need real games to test this.
			verify(HERE), region.imageSubresource.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT;

			const u32 out_w = region.bufferRowLength ? region.bufferRowLength : region.imageExtent.width;
			const u32 out_h = region.bufferImageHeight ? region.bufferImageHeight : region.imageExtent.height;
			const u32 packed32_length = out_w * out_h * 4;
			const u32 packed16_length = out_w * out_h * 2;

			const auto allocation_end = region.bufferOffset + packed32_length + packed16_length;
			verify(HERE), dst->size() >= allocation_end;

			const auto data_offset = u32(region.bufferOffset);
			const auto z32_offset = align<u32>(data_offset + packed16_length, 256);

			// 1. Copy the depth to buffer
			VkBufferImageCopy region2;
			region2 = region;
			region2.bufferOffset = z32_offset;
			vkCmdCopyImageToBuffer(cmd, src->value, src->current_layout, dst->value, 1, &region2);

			// 2. Pre-compute barrier
			vk::insert_buffer_memory_barrier(cmd, dst->value, z32_offset, packed32_length,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

			// 3. Do conversion with byteswap [D32->D16F]
			if (!swap_bytes) [[likely]]
			{
				auto job = vk::get_compute_task<vk::cs_fconvert_task<f32, f16>>();
				job->run(cmd, dst, z32_offset, packed32_length, data_offset);
			}
			else
			{
				auto job = vk::get_compute_task<vk::cs_fconvert_task<f32, f16, false, true>>();
				job->run(cmd, dst, z32_offset, packed32_length, data_offset);
			}

			// 4. Post-compute barrier
			vk::insert_buffer_memory_barrier(cmd, dst->value, region.bufferOffset, packed16_length,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
			break;
		}
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
		{
			verify(HERE), region.imageSubresource.aspectMask == (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

			const u32 out_w = region.bufferRowLength? region.bufferRowLength : region.imageExtent.width;
			const u32 out_h = region.bufferImageHeight? region.bufferImageHeight : region.imageExtent.height;
			const u32 packed_length = out_w * out_h * 4;
			const u32 in_depth_size = packed_length;
			const u32 in_stencil_size = out_w * out_h;

			const auto allocation_end = region.bufferOffset + packed_length + in_depth_size + in_stencil_size;
			verify(HERE), dst->size() >= allocation_end;

			const auto data_offset = u32(region.bufferOffset);
			const auto z_offset = align<u32>(data_offset + packed_length, 256);
			const auto s_offset = align<u32>(z_offset + in_depth_size, 256);

			// 1. Copy the depth and stencil blocks to separate banks
			VkBufferImageCopy sub_regions[2];
			sub_regions[0] = sub_regions[1] = region;
			sub_regions[0].bufferOffset = z_offset;
			sub_regions[0].imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			sub_regions[1].bufferOffset = s_offset;
			sub_regions[1].imageSubresource.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
			vkCmdCopyImageToBuffer(cmd, src->value, src->current_layout, dst->value, 2, sub_regions);

			// 2. Interleave the separated data blocks with a compute job
			vk::cs_interleave_task *job;
			if (!swap_bytes) [[likely]]
			{
				if (src->format() == VK_FORMAT_D24_UNORM_S8_UINT)
				{
					job = vk::get_compute_task<vk::cs_gather_d24x8<false>>();
				}
				else if (src->format_class() == RSX_FORMAT_CLASS_DEPTH24_FLOAT_X8_PACK32)
				{
					job = vk::get_compute_task<vk::cs_gather_d32x8<false, true>>();
				}
				else
				{
					job = vk::get_compute_task<vk::cs_gather_d32x8<false>>();
				}
			}
			else
			{
				if (src->format() == VK_FORMAT_D24_UNORM_S8_UINT)
				{
					job = vk::get_compute_task<vk::cs_gather_d24x8<true>>();
				}
				else if (src->format_class() == RSX_FORMAT_CLASS_DEPTH24_FLOAT_X8_PACK32)
				{
					job = vk::get_compute_task<vk::cs_gather_d32x8<true, true>>();
				}
				else
				{
					job = vk::get_compute_task<vk::cs_gather_d32x8<true>>();
				}
			}

			vk::insert_buffer_memory_barrier(cmd, dst->value, z_offset, in_depth_size + in_stencil_size,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

			job->run(cmd, dst, data_offset, packed_length, z_offset, s_offset);

			vk::insert_buffer_memory_barrier(cmd, dst->value, region.bufferOffset, packed_length,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
			break;
		}
		}
	}

	void copy_buffer_to_image(VkCommandBuffer cmd, const vk::buffer* src, const vk::image* dst, const VkBufferImageCopy& region)
	{
		// Always validate
		verify("Invalid image layout!" HERE),
			dst->current_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL || dst->current_layout == VK_IMAGE_LAYOUT_GENERAL;

		if (vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		switch (dst->format())
		{
		default:
		{
			vkCmdCopyBufferToImage(cmd, src->value, dst->value, dst->current_layout, 1, &region);
			break;
		}
		case VK_FORMAT_D32_SFLOAT:
		{
			rsx_log.error("Unsupported transfer (D16_FLOAT)");
			verify(HERE), region.imageSubresource.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT;

			const u32 out_w = region.bufferRowLength ? region.bufferRowLength : region.imageExtent.width;
			const u32 out_h = region.bufferImageHeight ? region.bufferImageHeight : region.imageExtent.height;
			const u32 packed32_length = out_w * out_h * 4;
			const u32 packed16_length = out_w * out_h * 2;

			const auto allocation_end = region.bufferOffset + packed32_length + packed16_length;
			verify(HERE), src->size() >= allocation_end;

			const auto data_offset = u32(region.bufferOffset);
			const auto z32_offset = align<u32>(data_offset + packed16_length, 256);

			// 1. Pre-compute barrier
			vk::insert_buffer_memory_barrier(cmd, src->value, z32_offset, packed32_length,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

			// 2. Do conversion with byteswap [D16F->D32F]
			auto job = vk::get_compute_task<vk::cs_fconvert_task<f16, f32>>();
			job->run(cmd, src, data_offset, packed16_length, z32_offset);

			// 4. Post-compute barrier
			vk::insert_buffer_memory_barrier(cmd, src->value, z32_offset, packed32_length,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);

			// 5. Copy the depth data to image
			VkBufferImageCopy region2 = region;
			region2.bufferOffset = z32_offset;
			vkCmdCopyBufferToImage(cmd, src->value, dst->value, dst->current_layout, 1, &region2);
			break;
		}
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
		{
			const u32 out_w = region.bufferRowLength? region.bufferRowLength : region.imageExtent.width;
			const u32 out_h = region.bufferImageHeight? region.bufferImageHeight : region.imageExtent.height;
			const u32 packed_length = out_w * out_h * 4;
			const u32 in_depth_size = packed_length;
			const u32 in_stencil_size = out_w * out_h;

			const auto allocation_end = region.bufferOffset + packed_length + in_depth_size + in_stencil_size;
			verify("Out of memory (compute heap). Lower your resolution scale setting." HERE), src->size() >= allocation_end;

			const auto data_offset = u32(region.bufferOffset);
			const auto z_offset = align<u32>(data_offset + packed_length, 256);
			const auto s_offset = align<u32>(z_offset + in_depth_size, 256);

			// Zero out the stencil block
			vkCmdFillBuffer(cmd, src->value, s_offset, in_stencil_size, 0);

			vk::insert_buffer_memory_barrier(cmd, src->value, s_offset, in_stencil_size,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

			// 1. Scatter the interleaved data into separate depth and stencil blocks
			vk::cs_interleave_task *job;
			if (dst->format() == VK_FORMAT_D24_UNORM_S8_UINT)
			{
				job = vk::get_compute_task<vk::cs_scatter_d24x8>();
			}
			else if (dst->format_class() == RSX_FORMAT_CLASS_DEPTH24_FLOAT_X8_PACK32)
			{
				job = vk::get_compute_task<vk::cs_scatter_d32x8<true>>();
			}
			else
			{
				job = vk::get_compute_task<vk::cs_scatter_d32x8<false>>();
			}

			job->run(cmd, src, data_offset, packed_length, z_offset, s_offset);

			vk::insert_buffer_memory_barrier(cmd, src->value, z_offset, in_depth_size + in_stencil_size,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);

			// 2. Copy the separated blocks into the target
			VkBufferImageCopy sub_regions[2];
			sub_regions[0] = sub_regions[1] = region;
			sub_regions[0].bufferOffset = z_offset;
			sub_regions[0].imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			sub_regions[1].bufferOffset = s_offset;
			sub_regions[1].imageSubresource.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
			vkCmdCopyBufferToImage(cmd, src->value, dst->value, dst->current_layout, 2, sub_regions);
			break;
		}
		}
	}

	void copy_image_typeless(const vk::command_buffer& cmd, vk::image* src, vk::image* dst, const areai& src_rect, const areai& dst_rect,
		u32 mipmaps, VkImageAspectFlags src_transfer_mask, VkImageAspectFlags dst_transfer_mask)
	{
		if (src->format() == dst->format())
		{
			if (src->format_class() == dst->format_class())
			{
				rsx_log.error("[Performance warning] Image copy requested incorrectly for matching formats.");
				copy_image(cmd, src, dst, src_rect, dst_rect, mipmaps, src_transfer_mask, dst_transfer_mask);
				return;
			}
			else
			{
				// Should only happen for DEPTH_FLOAT <-> DEPTH_UINT at this time
				const u32 mask = src->format_class() | dst->format_class();
				if (mask != (RSX_FORMAT_CLASS_DEPTH24_FLOAT_X8_PACK32 | RSX_FORMAT_CLASS_DEPTH24_UNORM_X8_PACK32))
				{
					rsx_log.error("Unexpected (and possibly incorrect) typeless transfer setup.");
				}
			}
		}

		if (vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		if (src != dst) [[likely]]
		{
			src->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			dst->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		}
		else
		{
			src->push_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
		}

		VkBufferImageCopy src_copy{}, dst_copy{};
		src_copy.imageExtent = { u32(src_rect.x2 - src_rect.x1), u32(src_rect.y2 - src_rect.y1), 1 };
		src_copy.imageOffset = { src_rect.x1, src_rect.y1, 0 };
		src_copy.imageSubresource = { src->aspect() & src_transfer_mask, 0, 0, 1 };

		dst_copy.imageExtent = { u32(dst_rect.x2 - dst_rect.x1), u32(dst_rect.y2 - dst_rect.y1), 1 };
		dst_copy.imageOffset = { dst_rect.x1, dst_rect.y1, 0 };
		dst_copy.imageSubresource = { dst->aspect() & dst_transfer_mask, 0, 0, 1 };

		const auto src_texel_size = vk::get_format_texel_width(src->info.format);
		const auto src_length = src_texel_size * src_copy.imageExtent.width * src_copy.imageExtent.height;
		u32 min_scratch_size = src_length;

		// Check for DS manipulation which will affect scratch memory requirements
		if (const VkFlags combined_aspect =  src->aspect() | dst->aspect();
			(combined_aspect & VK_IMAGE_ASPECT_STENCIL_BIT) != 0)
		{
			// At least one depth-stencil merge/extract required; requirements change to 2(w*h*bpp) + (w*h)
			min_scratch_size = (src_length * 2) + (src_length / src_texel_size);
		}

		// Initialize scratch memory
		auto scratch_buf = vk::get_scratch_buffer(min_scratch_size);

		for (u32 mip_level = 0; mip_level < mipmaps; ++mip_level)
		{
			vk::copy_image_to_buffer(cmd, src, scratch_buf, src_copy);

			auto src_convert = get_format_convert_flags(src->info.format);
			auto dst_convert = get_format_convert_flags(dst->info.format);

			if (src_convert.first || dst_convert.first)
			{
				if (src_convert.first == dst_convert.first &&
					src_convert.second == dst_convert.second)
				{
					// NOP, the two operations will cancel out
				}
				else
				{
					insert_buffer_memory_barrier(cmd, scratch_buf->value, 0, src_length, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

					vk::cs_shuffle_base *shuffle_kernel = nullptr;
					if (src_convert.first && dst_convert.first)
					{
						shuffle_kernel = vk::get_compute_task<vk::cs_shuffle_32_16>();
					}
					else
					{
						const auto block_size = src_convert.first ? src_convert.second : dst_convert.second;
						if (block_size == 4)
						{
							shuffle_kernel = vk::get_compute_task<vk::cs_shuffle_32>();
						}
						else if (block_size == 2)
						{
							shuffle_kernel = vk::get_compute_task<vk::cs_shuffle_16>();
						}
						else
						{
							fmt::throw_exception("Unreachable" HERE);
						}
					}

					shuffle_kernel->run(cmd, scratch_buf, src_length);

					insert_buffer_memory_barrier(cmd, scratch_buf->value, 0, src_length, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
						VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
				}
			}

			vk::copy_buffer_to_image(cmd, scratch_buf, dst, dst_copy);

			src_copy.imageSubresource.mipLevel++;
			dst_copy.imageSubresource.mipLevel++;
		}

		src->pop_layout(cmd);
		if (src != dst) dst->pop_layout(cmd);
	}

	void copy_image(const vk::command_buffer& cmd, vk::image* src, vk::image* dst,
			const areai& src_rect, const areai& dst_rect, u32 mipmaps,
			VkImageAspectFlags src_transfer_mask, VkImageAspectFlags dst_transfer_mask)
	{
		// NOTE: src_aspect should match dst_aspect according to spec but some drivers seem to work just fine with the mismatch
		if (const u32 aspect_bridge = (src->aspect() | dst->aspect());
			(aspect_bridge & VK_IMAGE_ASPECT_COLOR_BIT) == 0 &&
			src->format() != dst->format())
		{
			// Copying between two depth formats must match exactly or crashes will happen
			rsx_log.error("[Performance warning] Image copy was requested incorrectly for mismatched depth formats");
			copy_image_typeless(cmd, src, dst, src_rect, dst_rect, mipmaps);
			return;
		}

		VkImageSubresourceLayers a_src = {}, a_dst = {};
		a_src.aspectMask = src->aspect() & src_transfer_mask;
		a_src.baseArrayLayer = 0;
		a_src.layerCount = 1;
		a_src.mipLevel = 0;

		a_dst = a_src;
		a_dst.aspectMask = dst->aspect() & dst_transfer_mask;

		VkImageCopy rgn = {};
		rgn.extent.depth = 1;
		rgn.extent.width = u32(src_rect.x2 - src_rect.x1);
		rgn.extent.height = u32(src_rect.y2 - src_rect.y1);
		rgn.dstOffset = { dst_rect.x1, dst_rect.y1, 0 };
		rgn.srcOffset = { src_rect.x1, src_rect.y1, 0 };
		rgn.srcSubresource = a_src;
		rgn.dstSubresource = a_dst;

		if (vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		if (src != dst)
		{
			src->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			dst->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		}
		else
		{
			src->push_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
		}

		for (u32 mip_level = 0; mip_level < mipmaps; ++mip_level)
		{
			vkCmdCopyImage(cmd, src->value, src->current_layout, dst->value, dst->current_layout, 1, &rgn);

			rgn.srcSubresource.mipLevel++;
			rgn.dstSubresource.mipLevel++;
		}

		src->pop_layout(cmd);
		if (src != dst) dst->pop_layout(cmd);
	}

	void copy_scaled_image(const vk::command_buffer& cmd,
			vk::image* src, vk::image* dst,
			const areai& src_rect, const areai& dst_rect, u32 mipmaps,
			bool compatible_formats, VkFilter filter)
	{
		VkImageSubresourceLayers a_src = {}, a_dst = {};
		a_src.aspectMask = src->aspect();
		a_src.baseArrayLayer = 0;
		a_src.layerCount = 1;
		a_src.mipLevel = 0;

		a_dst = a_src;

		if (vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		//TODO: Use an array of offsets/dimensions for mipmapped blits (mipmap count > 1) since subimages will have different dimensions
		if (src != dst)
		{
			src->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			dst->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		}
		else
		{
			src->push_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
		}

		if (compatible_formats && !src_rect.is_flipped() && !dst_rect.is_flipped() &&
			src_rect.width() == dst_rect.width() && src_rect.height() == dst_rect.height())
		{
			VkImageCopy copy_rgn;
			copy_rgn.srcOffset = { src_rect.x1, src_rect.y1, 0 };
			copy_rgn.dstOffset = { dst_rect.x1, dst_rect.y1, 0 };
			copy_rgn.dstSubresource = { dst->aspect(), 0, 0, 1 };
			copy_rgn.srcSubresource = { src->aspect(), 0, 0, 1 };
			copy_rgn.extent = { static_cast<u32>(src_rect.width()), static_cast<u32>(src_rect.height()), 1 };

			vkCmdCopyImage(cmd, src->value, src->current_layout, dst->value, dst->current_layout, 1, &copy_rgn);
		}
		else if ((src->aspect() & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0)
		{
			//Most depth/stencil formats cannot be scaled using hw blit
			if (src->format() != dst->format())
			{
				// Can happen because of depth float mismatch. Format width should be equal RSX-side
				auto typeless = vk::get_typeless_helper(dst->format(), dst->format_class(), src_rect.width(), src_rect.height());
				copy_image_typeless(cmd, src, typeless, src_rect, src_rect, mipmaps);
				copy_scaled_image(cmd, typeless, dst, src_rect, dst_rect, mipmaps, true, filter);
			}
			else
			{
				verify(HERE), !dst_rect.is_flipped();

				auto stretch_image_typeless_unsafe = [&cmd, filter](vk::image* src, vk::image* dst, vk::image* typeless,
						const areai& src_rect, const areai& dst_rect, VkImageAspectFlags aspect, VkImageAspectFlags transfer_flags = 0xFF)
				{
					const auto src_w = src_rect.width();
					const auto src_h = src_rect.height();
					const auto dst_w = dst_rect.width();
					const auto dst_h = dst_rect.height();

					// Drivers are not very accepting of aspect COLOR -> aspect DEPTH or aspect STENCIL separately
					// However, this works okay for D24S8 (nvidia-only format)
					// NOTE: Tranfers of single aspect D/S from Nvidia's D24S8 is very slow

					//1. Copy unscaled to typeless surface
					copy_image(cmd, src, typeless, src_rect, { 0, 0, src_w, src_h }, 1, transfer_flags, 0xFF);

					//2. Blit typeless surface to self
					copy_scaled_image(cmd, typeless, typeless, { 0, 0, src_w, src_h }, { 0, src_h, dst_w, (src_h + dst_h) }, 1, true, filter);

					//3. Copy back the aspect bits
					copy_image(cmd, typeless, dst, {0, src_h, dst_w, (src_h + dst_h) }, dst_rect, 1, 0xFF, transfer_flags);
				};

				auto stretch_image_typeless_safe = [&cmd, filter](vk::image* src, vk::image* dst, vk::image* typeless,
					const areai& src_rect, const areai& dst_rect, VkImageAspectFlags aspect, VkImageAspectFlags transfer_flags = 0xFF)
				{
					const auto src_w = src_rect.width();
					const auto src_h = src_rect.height();
					const auto dst_w = dst_rect.width();
					const auto dst_h = dst_rect.height();

					auto scratch_buf = vk::get_scratch_buffer();

					//1. Copy unscaled to typeless surface
					VkBufferImageCopy info{};
					info.imageOffset = { std::min(src_rect.x1, src_rect.x2), std::min(src_rect.y1, src_rect.y2), 0 };
					info.imageExtent = { static_cast<u32>(src_w), static_cast<u32>(src_h), 1 };
					info.imageSubresource = { aspect & transfer_flags, 0, 0, 1 };

					vkCmdCopyImageToBuffer(cmd, src->value, src->current_layout, scratch_buf->value, 1, &info);
					insert_buffer_memory_barrier(cmd, scratch_buf->value, 0, VK_WHOLE_SIZE, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);

					info.imageOffset = {};
					info.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
					vkCmdCopyBufferToImage(cmd, scratch_buf->value, typeless->value, VK_IMAGE_LAYOUT_GENERAL, 1, &info);

					//2. Blit typeless surface to self and apply transform if necessary
					areai src_rect2 = { 0, 0, src_w, src_h };
					if (src_rect.x1 > src_rect.x2) src_rect2.flip_horizontal();
					if (src_rect.y1 > src_rect.y2) src_rect2.flip_vertical();

					insert_image_memory_barrier(cmd, typeless->value, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
						VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
						{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

					copy_scaled_image(cmd, typeless, typeless, src_rect2, { 0, src_h, dst_w, (src_h + dst_h) }, 1, true, filter);

					insert_image_memory_barrier(cmd, typeless->value, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
						VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
						{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

					//3. Copy back the aspect bits
					info.imageExtent = { static_cast<u32>(dst_w), static_cast<u32>(dst_h), 1 };
					info.imageOffset = { 0, src_h, 0 };

					vkCmdCopyImageToBuffer(cmd, typeless->value, VK_IMAGE_LAYOUT_GENERAL, scratch_buf->value, 1, &info);
					insert_buffer_memory_barrier(cmd, scratch_buf->value, 0, VK_WHOLE_SIZE, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);

					info.imageOffset = { dst_rect.x1, dst_rect.y1, 0 };
					info.imageSubresource = { aspect & transfer_flags, 0, 0, 1 };
					vkCmdCopyBufferToImage(cmd, scratch_buf->value, dst->value, dst->current_layout, 1, &info);
				};

				const u32 typeless_w = std::max(dst_rect.width(), src_rect.width());
				const u32 typeless_h = src_rect.height() + dst_rect.height();

				switch (src->format())
				{
				case VK_FORMAT_D16_UNORM:
				{
					auto typeless = vk::get_typeless_helper(VK_FORMAT_R16_UNORM, RSX_FORMAT_CLASS_COLOR, typeless_w, typeless_h);
					change_image_layout(cmd, typeless, VK_IMAGE_LAYOUT_GENERAL);
					stretch_image_typeless_unsafe(src, dst, typeless, src_rect, dst_rect, VK_IMAGE_ASPECT_DEPTH_BIT);
					break;
				}
				case VK_FORMAT_D32_SFLOAT:
				{
					auto typeless = vk::get_typeless_helper(VK_FORMAT_R32_SFLOAT, RSX_FORMAT_CLASS_COLOR, typeless_w, typeless_h);
					change_image_layout(cmd, typeless, VK_IMAGE_LAYOUT_GENERAL);
					stretch_image_typeless_unsafe(src, dst, typeless, src_rect, dst_rect, VK_IMAGE_ASPECT_DEPTH_BIT);
					break;
				}
				case VK_FORMAT_D24_UNORM_S8_UINT:
				{
					const VkImageAspectFlags depth_stencil = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
					if (const auto chip_family = vk::get_chip_family();
						chip_family > vk::chip_class::NV_generic && chip_family < vk::chip_class::NV_turing)
					{
						auto typeless = vk::get_typeless_helper(VK_FORMAT_B8G8R8A8_UNORM, RSX_FORMAT_CLASS_COLOR, typeless_w, typeless_h);
						change_image_layout(cmd, typeless, VK_IMAGE_LAYOUT_GENERAL);
						stretch_image_typeless_unsafe(src, dst, typeless, src_rect, dst_rect, depth_stencil);
					}
					else
					{
						auto typeless_depth = vk::get_typeless_helper(VK_FORMAT_B8G8R8A8_UNORM, RSX_FORMAT_CLASS_COLOR, typeless_w, typeless_h);
						auto typeless_stencil = vk::get_typeless_helper(VK_FORMAT_R8_UNORM, RSX_FORMAT_CLASS_COLOR, typeless_w, typeless_h);
						change_image_layout(cmd, typeless_depth, VK_IMAGE_LAYOUT_GENERAL);
						change_image_layout(cmd, typeless_stencil, VK_IMAGE_LAYOUT_GENERAL);

						stretch_image_typeless_safe(src, dst, typeless_depth, src_rect, dst_rect, depth_stencil, VK_IMAGE_ASPECT_DEPTH_BIT);
						stretch_image_typeless_safe(src, dst, typeless_stencil, src_rect, dst_rect, depth_stencil, VK_IMAGE_ASPECT_STENCIL_BIT);
					}
					break;
				}
				case VK_FORMAT_D32_SFLOAT_S8_UINT:
				{
					// NOTE: Typeless transfer (Depth/Stencil->Equivalent Color->Depth/Stencil) of single aspects does not work on AMD when done from a non-depth texture
					// Since the typeless transfer itself violates spec, the only way to make it work is to use a D32S8 intermediate
					// Copy from src->intermediate then intermediate->dst for each aspect separately

					// NOTE: While it may seem intuitive to use R32_SFLOAT as the carrier for the depth aspect, this does not work properly
					// Floating point interpolation is non-linear from a bit-by-bit perspective and generates undesirable effects

					auto typeless_depth = vk::get_typeless_helper(VK_FORMAT_B8G8R8A8_UNORM, RSX_FORMAT_CLASS_COLOR, typeless_w, typeless_h);
					auto typeless_stencil = vk::get_typeless_helper(VK_FORMAT_R8_UNORM, RSX_FORMAT_CLASS_COLOR, typeless_w, typeless_h);
					change_image_layout(cmd, typeless_depth, VK_IMAGE_LAYOUT_GENERAL);
					change_image_layout(cmd, typeless_stencil, VK_IMAGE_LAYOUT_GENERAL);

					const VkImageAspectFlags depth_stencil = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
					stretch_image_typeless_safe(src, dst, typeless_depth, src_rect, dst_rect, depth_stencil, VK_IMAGE_ASPECT_DEPTH_BIT);
					stretch_image_typeless_safe(src, dst, typeless_stencil, src_rect, dst_rect, depth_stencil, VK_IMAGE_ASPECT_STENCIL_BIT);
					break;
				}
				default:
					fmt::throw_exception("Unreachable" HERE);
					break;
				}
			}
		}
		else
		{
			VkImageBlit rgn = {};
			rgn.srcOffsets[0] = { src_rect.x1, src_rect.y1, 0 };
			rgn.srcOffsets[1] = { src_rect.x2, src_rect.y2, 1 };
			rgn.dstOffsets[0] = { dst_rect.x1, dst_rect.y1, 0 };
			rgn.dstOffsets[1] = { dst_rect.x2, dst_rect.y2, 1 };
			rgn.dstSubresource = a_dst;
			rgn.srcSubresource = a_src;

			for (u32 mip_level = 0; mip_level < mipmaps; ++mip_level)
			{
				vkCmdBlitImage(cmd, src->value, src->current_layout, dst->value, dst->current_layout, 1, &rgn, filter);

				rgn.srcSubresource.mipLevel++;
				rgn.dstSubresource.mipLevel++;
			}
		}

		src->pop_layout(cmd);
		if (src != dst) dst->pop_layout(cmd);
	}

	void gpu_deswizzle_sections_impl(VkCommandBuffer cmd, vk::buffer* scratch_buf, u32 dst_offset, int word_size, int word_count, bool swap_bytes, std::vector<VkBufferImageCopy>& sections)
	{
		// NOTE: This has to be done individually for every LOD
		vk::cs_deswizzle_base* job = nullptr;
		const auto block_size = (word_size * word_count);

		verify(HERE), word_size == 4 || word_size == 2;

		if (!swap_bytes)
		{
			if (word_size == 4)
			{
				switch (block_size)
				{
				case 4:
					job = vk::get_compute_task<cs_deswizzle_3d<u32, u32, false>>();
					break;
				case 8:
					job = vk::get_compute_task<cs_deswizzle_3d<u64, u32, false>>();
					break;
				case 16:
					job = vk::get_compute_task<cs_deswizzle_3d<u128, u32, false>>();
					break;
				}
			}
			else
			{
				switch (block_size)
				{
				case 4:
					job = vk::get_compute_task<cs_deswizzle_3d<u32, u16, false>>();
					break;
				case 8:
					job = vk::get_compute_task<cs_deswizzle_3d<u64, u16, false>>();
					break;
				}
			}
		}
		else
		{
			if (word_size == 4)
			{
				switch (block_size)
				{
				case 4:
					job = vk::get_compute_task<cs_deswizzle_3d<u32, u32, true>>();
					break;
				case 8:
					job = vk::get_compute_task<cs_deswizzle_3d<u64, u32, true>>();
					break;
				case 16:
					job = vk::get_compute_task<cs_deswizzle_3d<u128, u32, true>>();
					break;
				}
			}
			else
			{
				switch (block_size)
				{
				case 4:
					job = vk::get_compute_task<cs_deswizzle_3d<u32, u16, true>>();
					break;
				case 8:
					job = vk::get_compute_task<cs_deswizzle_3d<u64, u16, true>>();
					break;
				}
			}
		}

		verify(HERE), job;

		auto next_layer = sections.front().imageSubresource.baseArrayLayer;
		auto next_level = sections.front().imageSubresource.mipLevel;
		unsigned base = 0;
		unsigned lods = 0;

		std::vector<std::pair<unsigned, unsigned>> packets;
		for (unsigned i = 0; i < sections.size(); ++i)
		{
			verify(HERE), sections[i].bufferRowLength;

			const auto layer = sections[i].imageSubresource.baseArrayLayer;
			const auto level = sections[i].imageSubresource.mipLevel;

			if (layer == next_layer &&
				level == next_level)
			{
				next_level++;
				lods++;
				continue;
			}

			packets.emplace_back(base, lods);
			next_layer = layer;
			next_level = 1;
			base = i;
			lods = 1;
		}

		if (packets.empty() ||
			(packets.back().first + packets.back().second) < sections.size())
		{
			packets.emplace_back(base, lods);
		}

		for (const auto &packet : packets)
		{
			const auto& section = sections[packet.first];
			const auto src_offset = section.bufferOffset;

			// Align output to 128-byte boundary to keep some drivers happy
			dst_offset = align(dst_offset, 128);

			u32 data_length = 0;
			for (unsigned i = 0, j = packet.first; i < packet.second; ++i, ++j)
			{
				const u32 packed_size = sections[j].imageExtent.width * sections[j].imageExtent.height * sections[j].imageExtent.depth * block_size;
				sections[j].bufferOffset = dst_offset;
				dst_offset += packed_size;
				data_length += packed_size;
			}

			const u32 buf_off32 = static_cast<u32>(section.bufferOffset);
			const u32 src_off32 = static_cast<u32>(src_offset);

			job->run(cmd, scratch_buf, buf_off32, scratch_buf, src_off32, data_length,
				section.imageExtent.width, section.imageExtent.height, section.imageExtent.depth, packet.second);
		}

		verify(HERE), dst_offset <= scratch_buf->size();
	}

	void copy_mipmaped_image_using_buffer(VkCommandBuffer cmd, vk::image* dst_image,
		const std::vector<rsx::subresource_layout>& subresource_layout, int format, bool is_swizzled, u16 mipmap_count,
		VkImageAspectFlags flags, vk::data_heap &upload_heap, u32 heap_align)
	{
		const bool requires_depth_processing = (dst_image->aspect() & VK_IMAGE_ASPECT_STENCIL_BIT) || (format == CELL_GCM_TEXTURE_DEPTH16_FLOAT);
		u32 block_in_pixel = rsx::get_format_block_size_in_texel(format);
		u8  block_size_in_bytes = rsx::get_format_block_size_in_bytes(format);

		rsx::texture_uploader_capabilities caps{ true, false, true, heap_align };
		rsx::texture_memory_info opt{};
		bool check_caps = true;

		vk::buffer* scratch_buf = nullptr;
		u32 scratch_offset = 0;
		u32 row_pitch, image_linear_size;

		std::vector<VkBufferImageCopy> copy_regions;
		std::vector<VkBufferCopy> buffer_copies;
		copy_regions.reserve(subresource_layout.size());

		if (vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		for (const rsx::subresource_layout &layout : subresource_layout)
		{
			if (!heap_align) [[likely]]
			{
				if (!layout.border) [[likely]]
				{
					row_pitch = (layout.pitch_in_block * block_size_in_bytes);
				}
				else
				{
					// Skip the border texels if possible. Padding is undesirable for GPU deswizzle
					row_pitch = (layout.width_in_block * block_size_in_bytes);
				}

				caps.alignment = row_pitch;
			}
			else
			{
				row_pitch = rsx::align2(layout.width_in_block * block_size_in_bytes, heap_align);
				verify(HERE), row_pitch == heap_align;
			}

			image_linear_size = row_pitch * layout.height_in_block * layout.depth;

			// Map with extra padding bytes in case of realignment
			size_t offset_in_buffer = upload_heap.alloc<512>(image_linear_size + 8);
			void* mapped_buffer = upload_heap.map(offset_in_buffer, image_linear_size + 8);

			// Only do GPU-side conversion if occupancy is good
			if (check_caps)
			{
				caps.supports_byteswap = (image_linear_size >= 1024);
				caps.supports_hw_deswizzle = caps.supports_byteswap;
				check_caps = false;
			}

			gsl::span<std::byte> mapped{ static_cast<std::byte*>(mapped_buffer), image_linear_size };
			opt = upload_texture_subresource(mapped, layout, format, is_swizzled, caps);
			upload_heap.unmap();

			copy_regions.push_back({});
			auto& copy_info = copy_regions.back();
			copy_info.bufferOffset = offset_in_buffer;
			copy_info.imageExtent.height = layout.height_in_texel;
			copy_info.imageExtent.width = layout.width_in_texel;
			copy_info.imageExtent.depth = layout.depth;
			copy_info.imageSubresource.aspectMask = flags;
			copy_info.imageSubresource.layerCount = 1;
			copy_info.imageSubresource.baseArrayLayer = layout.layer;
			copy_info.imageSubresource.mipLevel = layout.level;
			copy_info.bufferRowLength = std::max<u32>(block_in_pixel * row_pitch / block_size_in_bytes, layout.width_in_texel);

			if (opt.require_swap || opt.require_deswizzle || requires_depth_processing)
			{
				if (!scratch_buf)
				{
					scratch_buf = vk::get_scratch_buffer(image_linear_size * 2);
					buffer_copies.reserve(subresource_layout.size());
				}

				if (layout.level == 0)
				{
					// Align mip0 on a 128-byte boundary
					scratch_offset = align(scratch_offset, 128);
				}

				// Copy from upload heap to scratch mem
				buffer_copies.push_back({});
				auto& copy = buffer_copies.back();
				copy.srcOffset = offset_in_buffer;
				copy.dstOffset = scratch_offset;
				copy.size = image_linear_size;

				// Point data source to scratch mem
				copy_info.bufferOffset = scratch_offset;

				scratch_offset += image_linear_size;
				verify("Out of scratch memory" HERE), (scratch_offset + image_linear_size) <= scratch_buf->size();
			}
		}

		if (opt.require_swap || opt.require_deswizzle || requires_depth_processing)
		{
			verify(HERE), scratch_buf;
			vkCmdCopyBuffer(cmd, upload_heap.heap->value, scratch_buf->value, static_cast<u32>(buffer_copies.size()), buffer_copies.data());

			insert_buffer_memory_barrier(cmd, scratch_buf->value, 0, scratch_offset, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
		}

		// Swap and swizzle if requested
		if (opt.require_deswizzle)
		{
			gpu_deswizzle_sections_impl(cmd, scratch_buf, scratch_offset, opt.element_size, opt.block_length, opt.require_swap, copy_regions);
		}
		else if (opt.require_swap)
		{
			if (opt.element_size == 4)
			{
				vk::get_compute_task<vk::cs_shuffle_32>()->run(cmd, scratch_buf, scratch_offset);
			}
			else if (opt.element_size == 2)
			{
				vk::get_compute_task<vk::cs_shuffle_16>()->run(cmd, scratch_buf, scratch_offset);
			}
			else
			{
				fmt::throw_exception("Unreachable" HERE);
			}
		}

		// CopyBufferToImage routines
		if (requires_depth_processing)
		{
			// Upload in reverse to avoid polluting data in lower space
			for (auto rIt = copy_regions.crbegin(); rIt != copy_regions.crend(); ++rIt)
			{
				vk::copy_buffer_to_image(cmd, scratch_buf, dst_image, *rIt);
			}
		}
		else if (scratch_buf)
		{
			verify(HERE), opt.require_deswizzle || opt.require_swap;

			const auto block_start = copy_regions.front().bufferOffset;
			insert_buffer_memory_barrier(cmd, scratch_buf->value, block_start, scratch_offset, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);

			vkCmdCopyBufferToImage(cmd, scratch_buf->value, dst_image->value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<u32>(copy_regions.size()), copy_regions.data());
		}
		else
		{
			vkCmdCopyBufferToImage(cmd, upload_heap.heap->value, dst_image->value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<u32>(copy_regions.size()), copy_regions.data());
		}
	}

	VkComponentMapping apply_swizzle_remap(const std::array<VkComponentSwizzle, 4>& base_remap, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap_vector)
	{
		VkComponentSwizzle final_mapping[4] = {};

		for (u8 channel = 0; channel < 4; ++channel)
		{
			switch (remap_vector.second[channel])
			{
			case CELL_GCM_TEXTURE_REMAP_ONE:
				final_mapping[channel] = VK_COMPONENT_SWIZZLE_ONE;
				break;
			case CELL_GCM_TEXTURE_REMAP_ZERO:
				final_mapping[channel] = VK_COMPONENT_SWIZZLE_ZERO;
				break;
			case CELL_GCM_TEXTURE_REMAP_REMAP:
				final_mapping[channel] = base_remap[remap_vector.first[channel]];
				break;
			default:
				rsx_log.error("Unknown remap lookup value %d", remap_vector.second[channel]);
			}
		}

		return{ final_mapping[1], final_mapping[2], final_mapping[3], final_mapping[0] };
	}

	void blitter::scale_image(vk::command_buffer& cmd, vk::image* src, vk::image* dst, areai src_area, areai dst_area, bool interpolate, const rsx::typeless_xfer& xfer_info)
	{
		vk::image* real_src = src;
		vk::image* real_dst = dst;

		// Optimization pass; check for pass-through data transfer
		if (!xfer_info.flip_horizontal && !xfer_info.flip_vertical && src_area.height() == dst_area.height())
		{
			auto src_w = src_area.width();
			auto dst_w = dst_area.width();

			if (xfer_info.src_is_typeless) src_w = static_cast<int>(src_w * xfer_info.src_scaling_hint);
			if (xfer_info.dst_is_typeless) dst_w = static_cast<int>(dst_w * xfer_info.dst_scaling_hint);

			if (src_w == dst_w)
			{
				// Final dimensions are a match
				if (xfer_info.src_is_typeless || xfer_info.dst_is_typeless)
				{
					vk::copy_image_typeless(cmd, src, dst, src_area, dst_area, 1);
				}
				else
				{
					copy_image(cmd, src, dst, src_area, dst_area, 1);
				}

				return;
			}
		}

		if (xfer_info.src_is_typeless)
		{
			const auto format = xfer_info.src_native_format_override ?
				VkFormat(xfer_info.src_native_format_override) :
				vk::get_compatible_sampler_format(vk::get_current_renderer()->get_formats_support(), xfer_info.src_gcm_format);

			if (format != src->format())
			{
				// Normalize input region (memory optimization)
				const auto old_src_area = src_area;
				src_area.y2 -= src_area.y1;
				src_area.y1 = 0;
				src_area.x2 = static_cast<int>(src_area.width() * xfer_info.src_scaling_hint);
				src_area.x1 = 0;

				// Transfer bits from src to typeless src
				real_src = vk::get_typeless_helper(format, rsx::classify_format(xfer_info.src_gcm_format), src_area.width(), src_area.height());
				real_src->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				vk::copy_image_typeless(cmd, src, real_src, old_src_area, src_area, 1);
			}
		}

		// Save output region descriptor
		const auto old_dst_area = dst_area;

		if (xfer_info.dst_is_typeless)
		{
			const auto format = xfer_info.dst_native_format_override ?
				VkFormat(xfer_info.dst_native_format_override) :
				vk::get_compatible_sampler_format(vk::get_current_renderer()->get_formats_support(), xfer_info.dst_gcm_format);

			if (format != dst->format())
			{
				// Normalize output region (memory optimization)
				dst_area.y2 -= dst_area.y1;
				dst_area.y1 = 0;
				dst_area.x2 = static_cast<int>(dst_area.width() * xfer_info.dst_scaling_hint);
				dst_area.x1 = 0;

				// Account for possibility where SRC is typeless and DST is typeless and both map to the same format
				auto required_height = dst_area.height();
				if (real_src != src && real_src->format() == format)
				{
					required_height += src_area.height();

					// Move the dst area just below the src area
					dst_area.y1 += src_area.y2;
					dst_area.y2 += src_area.y2;
				}

				real_dst = vk::get_typeless_helper(format, rsx::classify_format(xfer_info.dst_gcm_format), dst_area.width(), required_height);
			}
		}

		// Prepare typeless resources for the operation if needed
		if (real_src != src)
		{
			const auto layout = ((real_src == real_dst) ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			real_src->change_layout(cmd, layout);
		}

		if (real_dst != dst && real_dst != real_src)
		{
			real_dst->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		}

		// Checks
		if (src_area.x2 <= src_area.x1 || src_area.y2 <= src_area.y1 || dst_area.x2 <= dst_area.x1 || dst_area.y2 <= dst_area.y1)
		{
			rsx_log.error("Blit request consists of an empty region descriptor!");
			return;
		}

		if (src_area.x1 < 0 || src_area.x2 > static_cast<s32>(real_src->width()) || src_area.y1 < 0 || src_area.y2 > static_cast<s32>(real_src->height()))
		{
			rsx_log.error("Blit request denied because the source region does not fit!");
			return;
		}

		if (dst_area.x1 < 0 || dst_area.x2 > static_cast<s32>(real_dst->width()) || dst_area.y1 < 0 || dst_area.y2 > static_cast<s32>(real_dst->height()))
		{
			rsx_log.error("Blit request denied because the destination region does not fit!");
			return;
		}

		if (xfer_info.flip_horizontal)
		{
			src_area.flip_horizontal();
		}

		if (xfer_info.flip_vertical)
		{
			src_area.flip_vertical();
		}

		verify("Incompatible source and destination format!" HERE), real_src->aspect() == real_dst->aspect();

		copy_scaled_image(cmd, real_src, real_dst, src_area, dst_area, 1,
			formats_are_bitcast_compatible(real_src, real_dst),
			interpolate ? VK_FILTER_LINEAR : VK_FILTER_NEAREST);

		if (real_dst != dst)
		{
			vk::copy_image_typeless(cmd, real_dst, dst, dst_area, old_dst_area, 1);
		}
	}
}
