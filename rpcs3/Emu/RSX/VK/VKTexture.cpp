#include "stdafx.h"
#include "VKHelpers.h"
#include "../GCM.h"
#include "../RSXThread.h"
#include "../RSXTexture.h"
#include "../rsx_utils.h"
#include "VKFormats.h"

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

	void copy_image(VkCommandBuffer cmd, VkImage &src, VkImage &dst, VkImageLayout srcLayout, VkImageLayout dstLayout, u32 width, u32 height, u32 mipmaps, VkImageAspectFlagBits aspect)
	{
		VkImageSubresourceLayers a_src = {}, a_dst = {};
		a_src.aspectMask = aspect;
		a_src.baseArrayLayer = 0;
		a_src.layerCount = 1;
		a_src.mipLevel = 0;

		a_dst = a_src;

		VkImageCopy rgn = {};
		rgn.extent.depth = 1;
		rgn.extent.width = width;
		rgn.extent.height = height;
		rgn.dstOffset = { 0, 0, 0 };
		rgn.srcOffset = { 0, 0, 0 };
		rgn.srcSubresource = a_src;
		rgn.dstSubresource = a_dst;

		if (srcLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			change_image_layout(cmd, src, srcLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vk::get_image_subresource_range(0, 0, 1, 1, aspect));

		if (dstLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			change_image_layout(cmd, dst, dstLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vk::get_image_subresource_range(0, 0, 1, 1, aspect));

		for (u32 mip_level = 0; mip_level < mipmaps; ++mip_level)
		{
			vkCmdCopyImage(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &rgn);

			rgn.srcSubresource.mipLevel++;
			rgn.dstSubresource.mipLevel++;
		}

		if (srcLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			change_image_layout(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcLayout, vk::get_image_subresource_range(0, 0, 1, 1, aspect));

		if (dstLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			change_image_layout(cmd, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstLayout, vk::get_image_subresource_range(0, 0, 1, 1, aspect));
	}

	void copy_scaled_image(VkCommandBuffer cmd,
			VkImage & src, VkImage & dst,
			VkImageLayout srcLayout, VkImageLayout dstLayout,
			u32 src_x_offset, u32 src_y_offset, u32 src_width, u32 src_height,
			u32 dst_x_offset, u32 dst_y_offset, u32 dst_width, u32 dst_height,
			u32 mipmaps, VkImageAspectFlagBits aspect, bool compatible_formats)
	{
		VkImageSubresourceLayers a_src = {}, a_dst = {};
		a_src.aspectMask = aspect;
		a_src.baseArrayLayer = 0;
		a_src.layerCount = 1;
		a_src.mipLevel = 0;

		a_dst = a_src;

		//TODO: Use an array of offsets/dimensions for mipmapped blits (mipmap count > 1) since subimages will have different dimensions
		if (srcLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			change_image_layout(cmd, src, srcLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vk::get_image_subresource_range(0, 0, 1, 1, aspect));

		if (dstLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			change_image_layout(cmd, dst, dstLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vk::get_image_subresource_range(0, 0, 1, 1, aspect));

		if (src_width != dst_width || src_height != dst_height || mipmaps > 1 || !compatible_formats)
		{
			if ((aspect & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0)
			{
				//Most depth/stencil formats cannot be scaled using hw blit
				LOG_ERROR(RSX, "Cannot perform scaled blit for depth/stencil images");
				return;
			}

			VkImageBlit rgn = {};
			rgn.srcOffsets[0] = { (int32_t)src_x_offset, (int32_t)src_y_offset, 0 };
			rgn.srcOffsets[1] = { (int32_t)(src_width + src_x_offset), (int32_t)(src_height + src_y_offset), 1 };
			rgn.dstOffsets[0] = { (int32_t)dst_x_offset, (int32_t)dst_y_offset, 0 };
			rgn.dstOffsets[1] = { (int32_t)(dst_width + dst_x_offset), (int32_t)(dst_height + dst_y_offset), 1 };
			rgn.dstSubresource = a_dst;
			rgn.srcSubresource = a_src;

			for (u32 mip_level = 0; mip_level < mipmaps; ++mip_level)
			{
				vkCmdBlitImage(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &rgn, VK_FILTER_LINEAR);

				rgn.srcSubresource.mipLevel++;
				rgn.dstSubresource.mipLevel++;
			}
		}
		else
		{
			VkImageCopy copy_rgn;
			copy_rgn.srcOffset = { (int32_t)src_x_offset, (int32_t)src_y_offset, 0 };
			copy_rgn.dstOffset = { (int32_t)dst_x_offset, (int32_t)dst_y_offset, 0 };
			copy_rgn.dstSubresource = { (VkImageAspectFlags)aspect, 0, 0, 1 };
			copy_rgn.srcSubresource = { (VkImageAspectFlags)aspect, 0, 0, 1 };
			copy_rgn.extent = { src_width, src_height, 1 };

			vkCmdCopyImage(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_rgn);
		}

		if (srcLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			change_image_layout(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcLayout, vk::get_image_subresource_range(0, 0, 1, 1, aspect));

		if (dstLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			change_image_layout(cmd, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstLayout, vk::get_image_subresource_range(0, 0, 1, 1, aspect));
	}

	void copy_mipmaped_image_using_buffer(VkCommandBuffer cmd, VkImage dst_image,
		const std::vector<rsx_subresource_layout>& subresource_layout, int format, bool is_swizzled, u16 mipmap_count,
		VkImageAspectFlags flags, vk::vk_data_heap &upload_heap)
	{
		u32 mipmap_level = 0;
		u32 block_in_pixel = get_format_block_size_in_texel(format);
		u8 block_size_in_bytes = get_format_block_size_in_bytes(format);
		for (const rsx_subresource_layout &layout : subresource_layout)
		{
			u32 row_pitch = align(layout.width_in_block * block_size_in_bytes, 256);
			u32 image_linear_size = row_pitch * layout.height_in_block * layout.depth;
			size_t offset_in_buffer = upload_heap.alloc<512>(image_linear_size);

			void *mapped_buffer = upload_heap.map(offset_in_buffer, image_linear_size);
			gsl::span<gsl::byte> mapped{ (gsl::byte*)mapped_buffer, ::narrow<int>(image_linear_size) };
			upload_texture_subresource(mapped, layout, format, is_swizzled, 256);
			upload_heap.unmap();

			VkBufferImageCopy copy_info = {};
			copy_info.bufferOffset = offset_in_buffer;
			copy_info.imageExtent.height = layout.height_in_block * block_in_pixel;
			copy_info.imageExtent.width = layout.width_in_block * block_in_pixel;
			copy_info.imageExtent.depth = layout.depth;
			copy_info.imageSubresource.aspectMask = flags;
			copy_info.imageSubresource.layerCount = 1;
			copy_info.imageSubresource.baseArrayLayer = mipmap_level / mipmap_count;
			copy_info.imageSubresource.mipLevel = mipmap_level % mipmap_count;
			copy_info.bufferRowLength = block_in_pixel * row_pitch / block_size_in_bytes;

			vkCmdCopyBufferToImage(cmd, upload_heap.heap->value, dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_info);
			mipmap_level++;
		}
	}
}
