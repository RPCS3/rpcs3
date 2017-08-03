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
			u32 mipmaps, VkImageAspectFlagBits aspect)
	{
		VkImageSubresourceLayers a_src = {}, a_dst = {};
		a_src.aspectMask = aspect;
		a_src.baseArrayLayer = 0;
		a_src.layerCount = 1;
		a_src.mipLevel = 0;

		a_dst = a_src;

		//TODO: Use an array of offsets/dimensions for mipmapped blits (mipmap count > 1) since subimages will have different dimensions

		VkImageBlit rgn = {};
		rgn.srcOffsets[0] = { (int32_t)src_x_offset, (int32_t)src_y_offset, 0 };
		rgn.srcOffsets[1] = { (int32_t)src_width, (int32_t)src_height, 1 };
		rgn.dstOffsets[0] = { (int32_t)dst_x_offset, (int32_t)dst_y_offset, 0 };
		rgn.dstOffsets[1] = { (int32_t)(dst_width + dst_x_offset), (int32_t)(dst_height + dst_y_offset), 1 };
		rgn.dstSubresource = a_dst;
		rgn.srcSubresource = a_src;

		if (srcLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			change_image_layout(cmd, src, srcLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vk::get_image_subresource_range(0, 0, 1, 1, aspect));

		if (dstLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			change_image_layout(cmd, dst, dstLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vk::get_image_subresource_range(0, 0, 1, 1, aspect));

		for (u32 mip_level = 0; mip_level < mipmaps; ++mip_level)
		{
			vkCmdBlitImage(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &rgn, VK_FILTER_LINEAR);

			rgn.srcSubresource.mipLevel++;
			rgn.dstSubresource.mipLevel++;
		}

		if (srcLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			change_image_layout(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcLayout, vk::get_image_subresource_range(0, 0, 1, 1, aspect));

		if (dstLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			change_image_layout(cmd, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstLayout, vk::get_image_subresource_range(0, 0, 1, 1, aspect));
	}

	void copy_texture(VkCommandBuffer cmd, texture &src, texture &dst, VkImageLayout srcLayout, VkImageLayout dstLayout, u32 width, u32 height, u32 mipmaps, VkImageAspectFlagBits aspect)
	{
		VkImage isrc = (VkImage)src;
		VkImage idst = (VkImage)dst;
		
		copy_image(cmd, isrc, idst, srcLayout, dstLayout, width, height, mipmaps, aspect);
	}

	void copy_mipmaped_image_using_buffer(VkCommandBuffer cmd, VkImage dst_image,
		const std::vector<rsx_subresource_layout>& subresource_layout, int format, bool is_swizzled, u16 mipmap_count,
		vk::vk_data_heap &upload_heap, vk::buffer* upload_buffer)
	{
		u32 mipmap_level = 0;
		u32 block_in_pixel = get_format_block_size_in_texel(format);
		u8 block_size_in_bytes = get_format_block_size_in_bytes(format);
		for (const rsx_subresource_layout &layout : subresource_layout)
		{
			u32 row_pitch = align(layout.width_in_block * block_size_in_bytes, 256);
			u32 image_linear_size = row_pitch * layout.height_in_block * layout.depth;
			size_t offset_in_buffer = upload_heap.alloc<512>(image_linear_size);

			void *mapped_buffer = upload_buffer->map(offset_in_buffer, image_linear_size);
			gsl::span<gsl::byte> mapped{ (gsl::byte*)mapped_buffer, ::narrow<int>(image_linear_size) };
			upload_texture_subresource(mapped, layout, format, is_swizzled, 256);
			upload_buffer->unmap();

			VkBufferImageCopy copy_info = {};
			copy_info.bufferOffset = offset_in_buffer;
			copy_info.imageExtent.height = layout.height_in_block * block_in_pixel;
			copy_info.imageExtent.width = layout.width_in_block * block_in_pixel;
			copy_info.imageExtent.depth = layout.depth;
			copy_info.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copy_info.imageSubresource.layerCount = 1;
			copy_info.imageSubresource.baseArrayLayer = mipmap_level / mipmap_count;
			copy_info.imageSubresource.mipLevel = mipmap_level % mipmap_count;
			copy_info.bufferRowLength = block_in_pixel * row_pitch / block_size_in_bytes;

			vkCmdCopyBufferToImage(cmd, upload_buffer->value, dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_info);
			mipmap_level++;
		}
	}

	texture::texture(vk::swap_chain_image &img)
	{
		m_image_contents = img;
		m_view = img;
		
		//We did not create this object, do not allow internal modification!
		owner = nullptr;
	}

	void texture::create(vk::render_device &device, VkFormat format, VkImageType image_type, VkImageViewType view_type, VkImageCreateFlags image_flags, VkImageUsageFlags usage, VkImageTiling tiling, u32 width, u32 height, u32 mipmaps, bool gpu_only, VkComponentMapping swizzle)
	{
		owner = &device;

		//First create the image
		VkImageCreateInfo image_info = {};

		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.imageType = image_type;
		image_info.format = format;
		image_info.extent = { width, height, 1 };
		image_info.mipLevels = mipmaps;
		image_info.arrayLayers = (image_flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)? 6: 1;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_info.tiling = tiling;
		image_info.usage = usage;
		image_info.flags = image_flags;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		CHECK_RESULT(vkCreateImage(device, &image_info, nullptr, &m_image_contents));

		vkGetImageMemoryRequirements(device, m_image_contents, &m_memory_layout);
		vram_allocation.allocate_from_pool(device, m_memory_layout.size, !gpu_only, m_memory_layout.memoryTypeBits);

		CHECK_RESULT(vkBindImageMemory(device, m_image_contents, vram_allocation, 0));

		VkImageViewCreateInfo view_info = {};
		view_info.format = format;
		view_info.image = m_image_contents;
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.viewType = view_type;
		view_info.components = swizzle;
		view_info.subresourceRange = get_image_subresource_range(0, 0, 1, mipmaps, VK_IMAGE_ASPECT_COLOR_BIT);

		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			m_image_aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		CHECK_RESULT(vkCreateImageView(device, &view_info, nullptr, &m_view));

		m_width = width;
		m_height = height;
		m_mipmaps = mipmaps;
		m_internal_format = format;
		m_flags = usage;
		m_view_type = view_type;
		m_usage = usage;
		m_tiling = tiling;

		ready = true;
	}

	void texture::create(vk::render_device &device, VkFormat format, VkImageUsageFlags usage, VkImageTiling tiling, u32 width, u32 height, u32 mipmaps, bool gpu_only, VkComponentMapping swizzle)
	{
		create(device, format, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, 0, usage, tiling, width, height, mipmaps, gpu_only, swizzle);
	}

	void texture::create(vk::render_device &device, VkFormat format, VkImageUsageFlags usage, u32 width, u32 height, u32 mipmaps, bool gpu_only, VkComponentMapping swizzle)
	{
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;

		/* The spec mandates checking against all usage bits for support in either linear or optimal tiling modes.
		 * Ideally, no assumptions should be made, but for simplification, we'll assume optimal mode suppoorts everything
		 */

		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(device.gpu(), format, &props);

		bool linear_is_supported = true;

		if (!!(usage & VK_IMAGE_USAGE_SAMPLED_BIT))
		{
			if (!(props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
				linear_is_supported = false;
		}

		if (linear_is_supported && !!(usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
		{
			if (!(props.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))
				linear_is_supported = false;
		}

		if (linear_is_supported && !!(usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
			if (!(props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
				linear_is_supported = false;
		}

		if (linear_is_supported && !!(usage & VK_IMAGE_USAGE_STORAGE_BIT))
		{
			if (!(props.linearTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
				linear_is_supported = false;
		}

		if (linear_is_supported)
			tiling = VK_IMAGE_TILING_LINEAR;
		else
			usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		create(device, format, usage, tiling, width, height, mipmaps, gpu_only, swizzle);
	}

	void texture::init(rsx::fragment_texture& tex, vk::command_buffer &cmd, bool ignore_checks)
	{
		VkImageViewType best_type = VK_IMAGE_VIEW_TYPE_2D;
		
		if (tex.cubemap() && m_view_type != VK_IMAGE_VIEW_TYPE_CUBE)
		{
			vk::render_device &dev = (*owner);
			VkFormat format = m_internal_format;
			VkImageUsageFlags usage = m_usage;
			VkImageTiling tiling = m_tiling;

			destroy();
			create(dev, format, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, usage, tiling, tex.width(), tex.height(), tex.get_exact_mipmap_count(), false, default_component_map());
		}

		if (!tex.cubemap() && tex.depth() > 1 && m_view_type != VK_IMAGE_VIEW_TYPE_3D)
		{
			best_type = VK_IMAGE_VIEW_TYPE_3D;

			vk::render_device &dev = (*owner);
			VkFormat format = m_internal_format;
			VkImageUsageFlags usage = m_usage;
			VkImageTiling tiling = m_tiling;

			destroy();
			create(dev, format, VK_IMAGE_TYPE_3D, VK_IMAGE_VIEW_TYPE_3D, 0, usage, tiling, tex.width(), tex.height(), tex.get_exact_mipmap_count(), false, default_component_map());
		}

		VkImageSubresource subres = {};
		subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subres.mipLevel = 0;
		subres.arrayLayer = 0;

		u8 *data;

		VkFormatProperties props;
		vk::physical_device dev = owner->gpu();
		vkGetPhysicalDeviceFormatProperties(dev, m_internal_format, &props);

		if (ignore_checks || props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
		{
			std::vector<std::pair<u16, VkSubresourceLayout>> layout_alignment(tex.get_exact_mipmap_count());

			for (u32 i = 0; i < tex.get_exact_mipmap_count(); ++i)
			{
				layout_alignment[i].first = 4096;
				vkGetImageSubresourceLayout((*owner), m_image_contents, &subres, &layout_alignment[i].second);

				if (m_view_type == VK_IMAGE_VIEW_TYPE_CUBE)
					layout_alignment[i].second.size *= 6;
				
				while (layout_alignment[i].first > 1)
				{
					//Test if is wholly divisible by alignment..
					if (!(layout_alignment[i].second.rowPitch & (layout_alignment[i].first - 1)))
						break;

					layout_alignment[i].first >>= 1;
				}

				subres.mipLevel++;
			}

			if (tex.get_exact_mipmap_count() == 1)
			{
				u64 buffer_size = get_placed_texture_storage_size(tex, layout_alignment[0].first, layout_alignment[0].first);
				if (buffer_size != layout_alignment[0].second.size)
				{
					if (buffer_size > layout_alignment[0].second.size)
					{
						LOG_ERROR(RSX, "Layout->pitch = %d, size=%d, height=%d", layout_alignment[0].second.rowPitch, layout_alignment[0].second.size, tex.height());
						LOG_ERROR(RSX, "Computed alignment would have been %d, which yielded a size of %d", layout_alignment[0].first, buffer_size);
						LOG_ERROR(RSX, "Retrying...");

						//layout_alignment[0].first >>= 1;
						buffer_size = get_placed_texture_storage_size(tex, layout_alignment[0].first, layout_alignment[0].first);

						if (buffer_size != layout_alignment[0].second.size)
							fmt::throw_exception("Bad texture alignment computation!" HERE);
					}
					else
					{
						LOG_ERROR(RSX, "Bad texture alignment computation: expected size=%d bytes, computed=%d bytes, alignment=%d, hw pitch=%d",
							layout_alignment[0].second.size, buffer_size, layout_alignment[0].first, layout_alignment[0].second.rowPitch);
					}
				}

				CHECK_RESULT(vkMapMemory((*owner), vram_allocation, 0, m_memory_layout.size, 0, (void**)&data));
				gsl::span<gsl::byte> mapped{ (gsl::byte*)(data + layout_alignment[0].second.offset), ::narrow<int>(layout_alignment[0].second.size) };

				const std::vector<rsx_subresource_layout> &subresources_layout = get_subresources_layout(tex);
				for (const rsx_subresource_layout &layout : subresources_layout)
				{
					upload_texture_subresource(mapped, layout, tex.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN), !(tex.format() & CELL_GCM_TEXTURE_LN), layout_alignment[0].first);
				}
				vkUnmapMemory((*owner), vram_allocation);
			}
			else
			{
				auto &layer_props = layout_alignment[layout_alignment.size() - 1].second;
				u64 max_size = layer_props.offset + layer_props.size;

				if (m_memory_layout.size < max_size)
				{
					fmt::throw_exception("Failed to upload texture. Invalid memory block size." HERE);
				}

				int index= 0;
				std::vector<std::pair<u64, u32>> layout_offset_info(tex.get_exact_mipmap_count());
				
				for (auto &mip_info : layout_offset_info)
				{
					auto &alignment = layout_alignment[index].first;
					auto &layout = layout_alignment[index++].second;
					
					mip_info = std::make_pair(layout.offset, (u32)layout.rowPitch);
				}

				CHECK_RESULT(vkMapMemory((*owner), vram_allocation, 0, m_memory_layout.size, 0, (void**)&data));
				gsl::span<gsl::byte> mapped{ (gsl::byte*)(data), ::narrow<int>(m_memory_layout.size) };

				const std::vector<rsx_subresource_layout> &subresources_layout = get_subresources_layout(tex);
				size_t idx = 0;
				for (const rsx_subresource_layout &layout : subresources_layout)
				{
					const auto &dst_layout = layout_offset_info[idx++];
					upload_texture_subresource(mapped.subspan(dst_layout.first), layout, tex.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN), !(tex.format() & CELL_GCM_TEXTURE_LN), dst_layout.second);
				}
				vkUnmapMemory((*owner), vram_allocation);
			}
		}
		else if (!ignore_checks)
		{
			if (!staging_texture)
			{
				staging_texture = new texture();
				staging_texture->create((*owner), m_internal_format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_LINEAR, m_width, m_height, tex.get_exact_mipmap_count(), false, default_component_map());
			}

			staging_texture->init(tex, cmd, true);
			staging_texture->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			ready = false;
		}
	}

	void texture::flush(vk::command_buffer &cmd)
	{
		if (!ready)
		{
			vk::copy_texture(cmd, *staging_texture, *this, staging_texture->get_layout(), m_layout, m_width, m_height, m_mipmaps, m_image_aspect);
			ready = true;
		}
	}

	void texture::init_debug()
	{
		void *data;
		CHECK_RESULT(vkMapMemory((*owner), vram_allocation, 0, m_memory_layout.size, 0, (void**)&data));

		memset(data, 0xFF, m_memory_layout.size);
		vkUnmapMemory((*owner), vram_allocation);
	}

	void texture::change_layout(vk::command_buffer &cmd, VkImageLayout new_layout)
	{
		if (m_layout == new_layout) return;

		vk::change_image_layout(cmd, m_image_contents, m_layout, new_layout, vk::get_image_subresource_range(0, 0, 1, 1, m_image_aspect));
		m_layout = new_layout;
	}

	VkImageLayout texture::get_layout()
	{
		return m_layout;
	}

	const u32 texture::width()
	{
		return m_width;
	}

	const u32 texture::height()
	{
		return m_height;
	}

	const u16 texture::mipmaps()
	{
		return m_mipmaps;
	}

	void texture::destroy()
	{
		if (!owner) return;

		//Destroy all objects managed by this object
		vkDestroyImageView((*owner), m_view, nullptr);
		vkDestroyImage((*owner), m_image_contents, nullptr);

		vram_allocation.destroy();

		owner = nullptr;
		m_view = nullptr;
		m_image_contents = nullptr;

		if (staging_texture)
		{
			staging_texture->destroy();
			delete staging_texture;
			staging_texture = nullptr;
		}
	}

	const VkFormat texture::get_format()
	{
		return m_internal_format;
	}

	texture::operator VkImage()
	{
		return m_image_contents;
	}

	texture::operator VkImageView()
	{
		return m_view;
	}

}
