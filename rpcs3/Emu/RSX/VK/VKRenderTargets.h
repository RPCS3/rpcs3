#pragma once

#include "stdafx.h"
#include "VKHelpers.h"
#include "../GCM.h"
#include "../Common/surface_store.h"
#include "VKFormats.h"

namespace rsx
{
	struct vk_render_target_traits
	{
		using surface_storage_type = vk::texture ;
		using surface_type = vk::texture*;
		using command_list_type = vk::command_buffer*;
		using download_buffer_object = void*;

		static vk::texture create_new_surface(u32 address, surface_color_format format, size_t width, size_t height, vk::render_device &device, vk::command_buffer *cmd, const vk::gpu_formats_support &support)
		{
			VkFormat requested_format = vk::get_compatible_surface_format(format);
			
			vk::texture rtt;
			rtt.create(device, requested_format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_SAMPLED_BIT, width, height, 1, true);
			rtt.change_layout(*cmd, VK_IMAGE_LAYOUT_GENERAL);

			//Clear new surface
			VkClearColorValue clear_color;
			VkImageSubresourceRange range = vk::default_image_subresource_range();
		
			clear_color.float32[0] = 0.f;
			clear_color.float32[1] = 0.f;
			clear_color.float32[2] = 0.f;
			clear_color.float32[3] = 0.f;

			vkCmdClearColorImage(*cmd, rtt, rtt.get_layout(), &clear_color, 1, &range);
			rtt.change_layout(*cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			return rtt;
		}

		static vk::texture create_new_surface(u32 address, surface_depth_format format, size_t width, size_t height, vk::render_device &device, vk::command_buffer *cmd, const vk::gpu_formats_support &support)
		{
			VkFormat requested_format = vk::get_compatible_depth_surface_format(support, format);

			vk::texture rtt;
			rtt.create(device, requested_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT, width, height, 1, true);
			rtt.change_layout(*cmd, VK_IMAGE_LAYOUT_GENERAL);

			//Clear new surface..
			VkClearDepthStencilValue clear_depth = {};
			VkImageSubresourceRange range = {};
			range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			range.baseArrayLayer = 0;
			range.baseMipLevel = 0;
			range.layerCount = 1;
			range.levelCount = 1;

			if (requested_format != VK_FORMAT_D16_UNORM)
				range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

			clear_depth.depth = 1.f;
			clear_depth.stencil = 0;

			vkCmdClearDepthStencilImage(*cmd, rtt, rtt.get_layout(), &clear_depth, 1, &range);
			rtt.change_layout(*cmd, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			
			return rtt;
		}

		static void prepare_rtt_for_drawing(vk::command_buffer* pcmd, vk::texture *surface)
		{
			surface->change_layout(*pcmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		}

		static void prepare_rtt_for_sampling(vk::command_buffer* pcmd, vk::texture *surface)
		{
			surface->change_layout(*pcmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		static void prepare_ds_for_drawing(vk::command_buffer* pcmd, vk::texture *surface)
		{
			surface->change_layout(*pcmd, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		}

		static void prepare_ds_for_sampling(vk::command_buffer* pcmd, vk::texture *surface)
		{
			surface->change_layout(*pcmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		static bool rtt_has_format_width_height(const vk::texture &rtt, surface_color_format format, size_t width, size_t height)
		{
			VkFormat fmt = vk::get_compatible_surface_format(format);
			vk::texture &tex = const_cast<vk::texture&>(rtt);
			
			if (tex.get_format() == fmt &&
				tex.width() == width &&
				tex.height() == height)
				return true;

			return false;
		}

		static bool ds_has_format_width_height(const vk::texture &ds, surface_depth_format format, size_t width, size_t height)
		{
			// TODO: check format
			//VkFormat fmt = vk::get_compatible_depth_surface_format(format);
			vk::texture &tex = const_cast<vk::texture&>(ds);

			if (//tex.get_format() == fmt &&
				tex.width() == width &&
				tex.height() == height)
				return true;

			return false;
		}

		static download_buffer_object issue_download_command(surface_type, surface_color_format color_format, size_t width, size_t height, ...)
		{
			return nullptr;
		}

		static download_buffer_object issue_depth_download_command(surface_type, surface_depth_format depth_format, size_t width, size_t height, ...)
		{
			return nullptr;
		}

		static download_buffer_object issue_stencil_download_command(surface_type, surface_depth_format depth_format, size_t width, size_t height, ...)
		{
			return nullptr;
		}

		gsl::span<const gsl::byte> map_downloaded_buffer(download_buffer_object, ...)
		{
			return{ (gsl::byte*)nullptr, 0 };
		}

		static void unmap_downloaded_buffer(download_buffer_object, ...)
		{
		}

		static vk::texture *get(const vk::texture &tex)
		{
			return const_cast<vk::texture*>(&tex);
		}
	};

	struct vk_render_targets : public rsx::surface_store<vk_render_target_traits>
	{
	};
}
