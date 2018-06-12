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

	VkImageAspectFlags get_aspect_flags(VkFormat format)
	{
		switch (format)
		{
		default:
			return VK_IMAGE_ASPECT_COLOR_BIT;
		case VK_FORMAT_D16_UNORM:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}

	void copy_image(VkCommandBuffer cmd, VkImage &src, VkImage &dst, VkImageLayout srcLayout, VkImageLayout dstLayout,
			const areai& src_rect, const areai& dst_rect, u32 mipmaps, VkImageAspectFlags src_aspect, VkImageAspectFlags dst_aspect,
			VkImageAspectFlags src_transfer_mask, VkImageAspectFlags dst_transfer_mask)
	{
		// NOTE: src_aspect should match dst_aspect according to spec but drivers seem to work just fine with the mismatch
		// TODO: Implement separate pixel transfer for drivers that refuse this workaround

		VkImageSubresourceLayers a_src = {}, a_dst = {};
		a_src.aspectMask = src_aspect & src_transfer_mask;
		a_src.baseArrayLayer = 0;
		a_src.layerCount = 1;
		a_src.mipLevel = 0;

		a_dst = a_src;
		a_dst.aspectMask = dst_aspect & dst_transfer_mask;

		VkImageCopy rgn = {};
		rgn.extent.depth = 1;
		rgn.extent.width = u32(src_rect.x2 - src_rect.x1);
		rgn.extent.height = u32(src_rect.y2 - src_rect.y1);
		rgn.dstOffset = { dst_rect.x1, dst_rect.y1, 0 };
		rgn.srcOffset = { src_rect.x1, src_rect.y1, 0 };
		rgn.srcSubresource = a_src;
		rgn.dstSubresource = a_dst;

		auto preferred_src_format = (src == dst) ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		auto preferred_dst_format = (src == dst) ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		if (srcLayout != preferred_src_format)
			change_image_layout(cmd, src, srcLayout, preferred_src_format, vk::get_image_subresource_range(0, 0, 1, 1, src_aspect));

		if (dstLayout != preferred_dst_format)
			change_image_layout(cmd, dst, dstLayout, preferred_dst_format, vk::get_image_subresource_range(0, 0, 1, 1, dst_aspect));

		for (u32 mip_level = 0; mip_level < mipmaps; ++mip_level)
		{
			vkCmdCopyImage(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &rgn);

			rgn.srcSubresource.mipLevel++;
			rgn.dstSubresource.mipLevel++;
		}

		if (srcLayout != preferred_src_format)
			change_image_layout(cmd, src, preferred_src_format, srcLayout, vk::get_image_subresource_range(0, 0, 1, 1, src_aspect));

		if (dstLayout != preferred_dst_format)
			change_image_layout(cmd, dst, preferred_dst_format, dstLayout, vk::get_image_subresource_range(0, 0, 1, 1, dst_aspect));
	}

	void copy_scaled_image(VkCommandBuffer cmd,
			VkImage & src, VkImage & dst,
			VkImageLayout srcLayout, VkImageLayout dstLayout,
			u32 src_x_offset, u32 src_y_offset, u32 src_width, u32 src_height,
			u32 dst_x_offset, u32 dst_y_offset, u32 dst_width, u32 dst_height,
			u32 mipmaps, VkImageAspectFlags aspect, bool compatible_formats,
			VkFilter filter, VkFormat src_format, VkFormat dst_format)
	{
		VkImageSubresourceLayers a_src = {}, a_dst = {};
		a_src.aspectMask = aspect;
		a_src.baseArrayLayer = 0;
		a_src.layerCount = 1;
		a_src.mipLevel = 0;

		a_dst = a_src;

		auto preferred_src_format = (src == dst) ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		auto preferred_dst_format = (src == dst) ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		//TODO: Use an array of offsets/dimensions for mipmapped blits (mipmap count > 1) since subimages will have different dimensions
		if (srcLayout != preferred_src_format)
			change_image_layout(cmd, src, srcLayout, preferred_src_format, vk::get_image_subresource_range(0, 0, 1, 1, aspect));

		if (dstLayout != preferred_dst_format)
			change_image_layout(cmd, dst, dstLayout, preferred_dst_format, vk::get_image_subresource_range(0, 0, 1, 1, aspect));

		if (compatible_formats && src_width == dst_width && src_height != dst_height)
		{
			VkImageCopy copy_rgn;
			copy_rgn.srcOffset = { (int32_t)src_x_offset, (int32_t)src_y_offset, 0 };
			copy_rgn.dstOffset = { (int32_t)dst_x_offset, (int32_t)dst_y_offset, 0 };
			copy_rgn.dstSubresource = { (VkImageAspectFlags)aspect, 0, 0, 1 };
			copy_rgn.srcSubresource = { (VkImageAspectFlags)aspect, 0, 0, 1 };
			copy_rgn.extent = { src_width, src_height, 1 };

			vkCmdCopyImage(cmd, src, preferred_src_format, dst, preferred_dst_format, 1, &copy_rgn);
		}
		else if ((aspect & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0)
		{
			//Most depth/stencil formats cannot be scaled using hw blit
			if (src_format == VK_FORMAT_UNDEFINED || dst_width > 4096 || (src_height + dst_height) > 4096)
			{
				LOG_ERROR(RSX, "Could not blit depth/stencil image. src_fmt=0x%x, src=%dx%d, dst=%dx%d",
					(u32)src_format, src_width, src_height, dst_width, dst_height);
			}
			else
			{
				auto stretch_image_typeless = [&cmd, preferred_src_format, preferred_dst_format](VkImage src, VkImage dst, VkImage typeless,
						const areai& src_rect, const areai& dst_rect, VkImageAspectFlags aspect, VkImageAspectFlags transfer_flags = 0xFF)
				{
					const u32 src_w = u32(src_rect.x2 - src_rect.x1);
					const u32 src_h = u32(src_rect.y2 - src_rect.y1);
					const u32 dst_w = u32(dst_rect.x2 - dst_rect.x1);
					const u32 dst_h = u32(dst_rect.y2 - dst_rect.y1);

					// Drivers are not very accepting of aspect COLOR -> aspect DEPTH or aspect STENCIL separately
					// However, this works okay for D24S8 (nvidia-only format)
					// To work around the problem we use the non-existent DEPTH/STENCIL aspect of the color texture instead (AMD only)
					VkImageAspectFlags typeless_aspect;
					const bool single_aspect = (transfer_flags == VK_IMAGE_ASPECT_DEPTH_BIT || transfer_flags == VK_IMAGE_ASPECT_STENCIL_BIT);

					switch (vk::get_driver_vendor())
					{
					case driver_vendor::AMD:
						// This workaround allows proper transfer of stencil data
						typeless_aspect = aspect;
						break;
					case driver_vendor::NVIDIA:
						// This workaround allows only transfer of depth data, stencil is ignored (D32S8 only)
						// However, transfer from r32 to d24s8 in color->depth_stencil works
						typeless_aspect = (single_aspect)? aspect : VK_IMAGE_ASPECT_COLOR_BIT;
						break;
					case driver_vendor::RADV:
						// This workaround allows only transfer of depth data, stencil is ignored (D32S8 only)
					default:
						typeless_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
						break;
					}

					//1. Copy unscaled to typeless surface
					copy_image(cmd, src, typeless, preferred_src_format, VK_IMAGE_LAYOUT_GENERAL,
						src_rect, { 0, 0, (s32)src_w, (s32)src_h }, 1, aspect, typeless_aspect, transfer_flags, 0xFF);

					//2. Blit typeless surface to self
					copy_scaled_image(cmd, typeless, typeless, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
						0, 0, src_w, src_h, 0, src_h, dst_w, dst_h, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_NEAREST);

					//3. Copy back the aspect bits
					copy_image(cmd, typeless, dst, VK_IMAGE_LAYOUT_GENERAL, preferred_dst_format,
						{0, (s32)src_h, (s32)dst_w, s32(src_h + dst_h) }, dst_rect, 1, typeless_aspect, aspect, 0xFF, transfer_flags);
				};

				areai src_rect = { (s32)src_x_offset, (s32)src_y_offset, s32(src_x_offset + src_width), s32(src_y_offset + src_height) };
				areai dst_rect = { (s32)dst_x_offset, (s32)dst_y_offset, s32(dst_x_offset + dst_width), s32(dst_y_offset + dst_height) };

				switch (src_format)
				{
				case VK_FORMAT_D16_UNORM:
				{
					auto typeless = vk::get_typeless_helper(VK_FORMAT_R16_UNORM);
					change_image_layout(cmd, typeless, VK_IMAGE_LAYOUT_GENERAL);
					stretch_image_typeless(src, dst, typeless->value, src_rect, dst_rect, VK_IMAGE_ASPECT_DEPTH_BIT);
					break;
				}
				case VK_FORMAT_D24_UNORM_S8_UINT:
				{
					auto typeless = vk::get_typeless_helper(VK_FORMAT_B8G8R8A8_UNORM);
					change_image_layout(cmd, typeless, VK_IMAGE_LAYOUT_GENERAL);
					stretch_image_typeless(src, dst, typeless->value, src_rect, dst_rect, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
					break;
				}
				case VK_FORMAT_D32_SFLOAT_S8_UINT:
				{
					// NOTE: Typeless transfer (Depth/Stencil->Equivalent Color->Depth/Stencil) of single aspects does not work on AMD when done from a non-depth texture
					// Since the typeless transfer itself violates spec, the only way to make it work is to use a D32S8 intermediate
					// Copy from src->intermediate then intermediate->dst for each aspect separately

					auto typeless_depth = vk::get_typeless_helper(VK_FORMAT_R32_SFLOAT);
					auto typeless_stencil = vk::get_typeless_helper(VK_FORMAT_R8_UINT);
					change_image_layout(cmd, typeless_depth, VK_IMAGE_LAYOUT_GENERAL);
					change_image_layout(cmd, typeless_stencil, VK_IMAGE_LAYOUT_GENERAL);

					auto intermediate = vk::get_typeless_helper(VK_FORMAT_D32_SFLOAT_S8_UINT);
					change_image_layout(cmd, intermediate, preferred_dst_format);

					const areai intermediate_rect = { 0, 0, (s32)dst_width, (s32)dst_height };
					const VkImageAspectFlags depth_stencil = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

					// Blit DEPTH aspect
					stretch_image_typeless(src, intermediate->value, typeless_depth->value, src_rect, intermediate_rect, depth_stencil, VK_IMAGE_ASPECT_DEPTH_BIT);
					copy_image(cmd, intermediate->value, dst, preferred_dst_format, preferred_dst_format, intermediate_rect, dst_rect, 1, depth_stencil, depth_stencil, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

					// Blit STENCIL aspect
					stretch_image_typeless(src, intermediate->value, typeless_stencil->value, src_rect, intermediate_rect, depth_stencil, VK_IMAGE_ASPECT_STENCIL_BIT);
					copy_image(cmd, intermediate->value, dst, preferred_dst_format, preferred_dst_format, intermediate_rect, dst_rect, 1, depth_stencil, depth_stencil, VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_ASPECT_STENCIL_BIT);
					break;
				}
				}
			}
		}
		else
		{
			VkImageBlit rgn = {};
			rgn.srcOffsets[0] = { (int32_t)src_x_offset, (int32_t)src_y_offset, 0 };
			rgn.srcOffsets[1] = { (int32_t)(src_width + src_x_offset), (int32_t)(src_height + src_y_offset), 1 };
			rgn.dstOffsets[0] = { (int32_t)dst_x_offset, (int32_t)dst_y_offset, 0 };
			rgn.dstOffsets[1] = { (int32_t)(dst_width + dst_x_offset), (int32_t)(dst_height + dst_y_offset), 1 };
			rgn.dstSubresource = a_dst;
			rgn.srcSubresource = a_src;

			for (u32 mip_level = 0; mip_level < mipmaps; ++mip_level)
			{
				vkCmdBlitImage(cmd, src, preferred_src_format, dst, preferred_dst_format, 1, &rgn, filter);

				rgn.srcSubresource.mipLevel++;
				rgn.dstSubresource.mipLevel++;
			}
		}

		if (srcLayout != preferred_src_format)
			change_image_layout(cmd, src, preferred_src_format, srcLayout, vk::get_image_subresource_range(0, 0, 1, 1, aspect));

		if (dstLayout != preferred_dst_format)
			change_image_layout(cmd, dst, preferred_dst_format, dstLayout, vk::get_image_subresource_range(0, 0, 1, 1, aspect));
	}

	void copy_mipmaped_image_using_buffer(VkCommandBuffer cmd, vk::image* dst_image,
		const std::vector<rsx_subresource_layout>& subresource_layout, int format, bool is_swizzled, u16 mipmap_count,
		VkImageAspectFlags flags, vk::vk_data_heap &upload_heap)
	{
		u32 mipmap_level = 0;
		u32 block_in_pixel = get_format_block_size_in_texel(format);
		u8 block_size_in_bytes = get_format_block_size_in_bytes(format);
		std::vector<u8> staging_buffer;

		//TODO: Depth and stencil transfer together
		flags &= ~(VK_IMAGE_ASPECT_STENCIL_BIT);

		for (const rsx_subresource_layout &layout : subresource_layout)
		{
			u32 row_pitch = align(layout.width_in_block * block_size_in_bytes, 256);
			u32 image_linear_size = row_pitch * layout.height_in_block * layout.depth;

			//Map with extra padding bytes in case of realignment
			size_t offset_in_buffer = upload_heap.alloc<512>(image_linear_size + 8);
			void *mapped_buffer = upload_heap.map(offset_in_buffer, image_linear_size + 8);
			void *dst = mapped_buffer;

			bool use_staging = false;
			if (dst_image->info.format == VK_FORMAT_D24_UNORM_S8_UINT ||
				dst_image->info.format == VK_FORMAT_D32_SFLOAT_S8_UINT)
			{
				//Misalign intentionally to skip the first stencil byte in D24S8 data
				//Ensures the real depth data is dword aligned

				if (dst_image->info.format == VK_FORMAT_D32_SFLOAT_S8_UINT)
				{
					//Emulate D24x8 passthrough to D32 format
					//Reads from GPU managed memory are slow at best and at worst unreliable
					use_staging = true;
					staging_buffer.resize(image_linear_size + 8);
					dst = staging_buffer.data() + 4 - 1;
				}
				else
				{
					//Skip leading dword when writing to texture
					offset_in_buffer += 4;
					dst = (char*)(mapped_buffer) + 4 - 1;
				}
			}

			gsl::span<gsl::byte> mapped{ (gsl::byte*)dst, ::narrow<int>(image_linear_size) };
			upload_texture_subresource(mapped, layout, format, is_swizzled, false, 256);

			if (use_staging)
			{
				if (dst_image->info.format == VK_FORMAT_D32_SFLOAT_S8_UINT)
				{
					//Map depth component from D24x8 to a f32 depth value
					//NOTE: One byte (contains first S8 value) is skipped
					rsx::convert_le_d24x8_to_le_f32(mapped_buffer, (char*)dst + 1, image_linear_size >> 2, 1);
				}
				else //unused
				{
					//Copy emulated data back to the target buffer
					memcpy(mapped_buffer, dst, image_linear_size);
				}
			}

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

			vkCmdCopyBufferToImage(cmd, upload_heap.heap->value, dst_image->value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_info);
			mipmap_level++;
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
				LOG_ERROR(RSX, "Unknown remap lookup value %d", remap_vector.second[channel]);
			}
		}

		return{ final_mapping[1], final_mapping[2], final_mapping[3], final_mapping[0] };
	}
}
