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
		using surface_storage_type = std::unique_ptr<vk::image>;
		using surface_type = vk::image*;
		using command_list_type = vk::command_buffer*;
		using download_buffer_object = void*;

		static std::unique_ptr<vk::image> create_new_surface(u32 address, surface_color_format format, size_t width, size_t height, vk::render_device &device, vk::command_buffer *cmd, const vk::gpu_formats_support &support, const vk::memory_type_mapping &mem_mapping)
		{
			auto fmt = vk::get_compatible_surface_format(format);
			VkFormat requested_format = fmt.first;

			std::unique_ptr<vk::image> rtt;
			rtt.reset(new vk::image(device, mem_mapping.device_local,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D,
				requested_format,
				static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1, 1, 1,
				VK_SAMPLE_COUNT_1_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
				0));
			change_image_layout(*cmd, rtt->value, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT));
			//Clear new surface
			VkClearColorValue clear_color;
			VkImageSubresourceRange range = vk::get_image_subresource_range(0,0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT);

			clear_color.float32[0] = 0.f;
			clear_color.float32[1] = 0.f;
			clear_color.float32[2] = 0.f;
			clear_color.float32[3] = 0.f;

			vkCmdClearColorImage(*cmd, rtt->value, VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1, &range);
			change_image_layout(*cmd, rtt->value, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT));

			rtt->native_layout = fmt.second;
			return rtt;
		}

		static std::unique_ptr<vk::image> create_new_surface(u32 address, surface_depth_format format, size_t width, size_t height, vk::render_device &device, vk::command_buffer *cmd, const vk::gpu_formats_support &support, const vk::memory_type_mapping &mem_mapping)
		{
			VkFormat requested_format = vk::get_compatible_depth_surface_format(support, format);
			VkImageSubresourceRange range = vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_DEPTH_BIT);

			if (requested_format != VK_FORMAT_D16_UNORM)
				range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

			std::unique_ptr<vk::image> ds;
			ds.reset(new vk::image(device, mem_mapping.device_local,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D,
				requested_format,
				static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1, 1, 1,
				VK_SAMPLE_COUNT_1_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0));

			change_image_layout(*cmd, ds->value, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, range);

			//Clear new surface..
			VkClearDepthStencilValue clear_depth = {};


			clear_depth.depth = 1.f;
			clear_depth.stencil = 0;

			vkCmdClearDepthStencilImage(*cmd, ds->value, VK_IMAGE_LAYOUT_GENERAL, &clear_depth, 1, &range);
			change_image_layout(*cmd, ds->value, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, range);

			return ds;
		}

		static void prepare_rtt_for_drawing(vk::command_buffer* pcmd, vk::image *surface)
		{
//			surface->change_layout(*pcmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		}

		static void prepare_rtt_for_sampling(vk::command_buffer* pcmd, vk::image *surface)
		{
//			surface->change_layout(*pcmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		static void prepare_ds_for_drawing(vk::command_buffer* pcmd, vk::image *surface)
		{
//			surface->change_layout(*pcmd, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		}

		static void prepare_ds_for_sampling(vk::command_buffer* pcmd, vk::image *surface)
		{
//			surface->change_layout(*pcmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		static bool rtt_has_format_width_height(const std::unique_ptr<vk::image> &rtt, surface_color_format format, size_t width, size_t height)
		{
			VkFormat fmt = vk::get_compatible_surface_format(format).first;

			if (rtt->info.format == fmt &&
				rtt->info.extent.width == width &&
				rtt->info.extent.height == height)
				return true;

			return false;
		}

		static bool ds_has_format_width_height(const std::unique_ptr<vk::image> &ds, surface_depth_format format, size_t width, size_t height)
		{
			// TODO: check format
			//VkFormat fmt = vk::get_compatible_depth_surface_format(format);

			if (//tex.get_format() == fmt &&
				ds->info.extent.width == width &&
				ds->info.extent.height == height)
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

		static vk::image *get(const std::unique_ptr<vk::image> &tex)
		{
			return tex.get();
		}
	};

	struct vk_render_targets : public rsx::surface_store<vk_render_target_traits>
	{

		void destroy()
		{
			m_render_targets_storage.clear();
			m_depth_stencil_storage.clear();
		}
	};
}
