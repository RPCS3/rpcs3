#pragma once

#include "stdafx.h"
#include "VKHelpers.h"
#include "../GCM.h"
#include "../Common/surface_store.h"
#include "../Common/TextureUtils.h"
#include "VKFormats.h"
#include "../rsx_utils.h"

namespace vk
{
	struct render_target : public image, public rsx::ref_counted, public rsx::render_target_descriptor<vk::image*>
	{
		bool dirty = false;
		u16 native_pitch = 0;
		u16 rsx_pitch = 0;

		u16 surface_width = 0;
		u16 surface_height = 0;

		VkImageAspectFlags attachment_aspect_flag = VK_IMAGE_ASPECT_COLOR_BIT;
		std::unordered_map<u32, std::unique_ptr<vk::image_view>> views;

		render_target *old_contents = nullptr; //Data occupying the memory location that this surface is replacing
		u64 frame_tag = 0; //frame id when invalidated, 0 if not invalid

		render_target(vk::render_device &dev,
			uint32_t memory_type_index,
			uint32_t access_flags,
			VkImageType image_type,
			VkFormat format,
			uint32_t width, uint32_t height, uint32_t depth,
			uint32_t mipmaps, uint32_t layers,
			VkSampleCountFlagBits samples,
			VkImageLayout initial_layout,
			VkImageTiling tiling,
			VkImageUsageFlags usage,
			VkImageCreateFlags image_flags)

			:image(dev, memory_type_index, access_flags, image_type, format, width, height, depth,
					mipmaps, layers, samples, initial_layout, tiling, usage, image_flags)
		{}

		vk::image_view* get_view(u32 remap_encoding, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap)
		{
			auto found = views.find(remap_encoding);
			if (found != views.end())
			{
				return found->second.get();
			}

			VkComponentMapping real_mapping = vk::apply_swizzle_remap
			(
				{native_component_map.a, native_component_map.r, native_component_map.g, native_component_map.b },
				remap
			);

			auto view = std::make_unique<vk::image_view>(*vk::get_current_renderer(), value, VK_IMAGE_VIEW_TYPE_2D, info.format,
					real_mapping, vk::get_image_subresource_range(0, 0, 1, 1, attachment_aspect_flag & ~(VK_IMAGE_ASPECT_STENCIL_BIT)));

			auto result = view.get();
			views[remap_encoding] = std::move(view);
			return result;
		}

		vk::image* get_surface() override
		{
			return (vk::image*)this;
		}

		u16 get_surface_width() const override
		{
			return surface_width;
		}

		u16 get_surface_height() const override
		{
			return surface_height;
		}

		u16 get_rsx_pitch() const override
		{
			return rsx_pitch;
		}

		u16 get_native_pitch() const override
		{
			return native_pitch;
		}

		bool matches_dimensions(u16 _width, u16 _height) const
		{
			//Use foward scaling to account for rounding and clamping errors
			return (rsx::apply_resolution_scale(_width, true) == width()) && (rsx::apply_resolution_scale(_height, true) == height());
		}
	};

	struct framebuffer_holder: public vk::framebuffer, public rsx::ref_counted
	{
		framebuffer_holder(VkDevice dev,
			VkRenderPass pass,
			u32 width, u32 height,
			std::vector<std::unique_ptr<vk::image_view>> &&atts)

			: framebuffer(dev, pass, width, height, std::move(atts))
		{}
	};
}

namespace rsx
{
	struct vk_render_target_traits
	{
		using surface_storage_type = std::unique_ptr<vk::render_target>;
		using surface_type = vk::render_target*;
		using command_list_type = vk::command_buffer*;
		using download_buffer_object = void*;

		static std::unique_ptr<vk::render_target> create_new_surface(
			u32 /*address*/,
			surface_color_format format,
			size_t width, size_t height,
			vk::render_target* old_surface,
			vk::render_device &device, vk::command_buffer *cmd, const vk::gpu_formats_support &, const vk::memory_type_mapping &mem_mapping)
		{
			auto fmt = vk::get_compatible_surface_format(format);
			VkFormat requested_format = fmt.first;

			std::unique_ptr<vk::render_target> rtt;
			rtt.reset(new vk::render_target(device, mem_mapping.device_local,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D,
				requested_format,
				static_cast<uint32_t>(rsx::apply_resolution_scale((u16)width, true)), static_cast<uint32_t>(rsx::apply_resolution_scale((u16)height, true)), 1, 1, 1,
				VK_SAMPLE_COUNT_1_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
				0));

			change_image_layout(*cmd, rtt.get(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT));

			rtt->native_component_map = fmt.second;
			rtt->native_pitch = (u16)width * get_format_block_size_in_bytes(format);
			rtt->surface_width = (u16)width;
			rtt->surface_height = (u16)height;
			rtt->dirty = true;

			if (old_surface != nullptr && old_surface->info.format == requested_format)
				rtt->old_contents = old_surface;

			return rtt;
		}

		static std::unique_ptr<vk::render_target> create_new_surface(
			u32 /* address */,
			surface_depth_format format,
			size_t width, size_t height,
			vk::render_target* old_surface,
			vk::render_device &device, vk::command_buffer *cmd, const vk::gpu_formats_support &support, const vk::memory_type_mapping &mem_mapping)
		{
			VkFormat requested_format = vk::get_compatible_depth_surface_format(support, format);
			VkImageSubresourceRange range = vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_DEPTH_BIT);

			if (requested_format != VK_FORMAT_D16_UNORM)
				range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

			const auto scale = rsx::get_resolution_scale();

			std::unique_ptr<vk::render_target> ds;
			ds.reset(new vk::render_target(device, mem_mapping.device_local,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D,
				requested_format,
				static_cast<uint32_t>(rsx::apply_resolution_scale((u16)width, true)), static_cast<uint32_t>(rsx::apply_resolution_scale((u16)height, true)), 1, 1, 1,
				VK_SAMPLE_COUNT_1_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT| VK_IMAGE_USAGE_TRANSFER_SRC_BIT| VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
				0));

			ds->native_component_map = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };
			change_image_layout(*cmd, ds.get(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, range);

			ds->native_pitch = (u16)width * 2;
			if (format == rsx::surface_depth_format::z24s8)
				ds->native_pitch *= 2;

			ds->attachment_aspect_flag = range.aspectMask;
			ds->surface_width = (u16)width;
			ds->surface_height = (u16)height;
			ds->dirty = true;

			if (old_surface != nullptr && old_surface->info.format == requested_format)
				ds->old_contents = old_surface;

			return ds;
		}

		static
		void get_surface_info(vk::render_target *surface, rsx::surface_format_info *info)
		{
			info->rsx_pitch = surface->rsx_pitch;
			info->native_pitch = surface->native_pitch;
			info->surface_width = surface->get_surface_width();
			info->surface_height = surface->get_surface_height();
			info->bpp = static_cast<u8>(info->native_pitch / info->surface_width);
		}

		static void prepare_rtt_for_drawing(vk::command_buffer* pcmd, vk::render_target *surface)
		{
			VkImageSubresourceRange range = vk::get_image_subresource_range(0, 0, 1, 1, surface->attachment_aspect_flag);
			change_image_layout(*pcmd, surface, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, range);

			//Reset deref count
			surface->deref_count = 0;
			surface->frame_tag = 0;
		}

		static void prepare_rtt_for_sampling(vk::command_buffer* pcmd, vk::render_target *surface)
		{
			VkImageSubresourceRange range = vk::get_image_subresource_range(0, 0, 1, 1, surface->attachment_aspect_flag);
			change_image_layout(*pcmd, surface, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
		}

		static void prepare_ds_for_drawing(vk::command_buffer* pcmd, vk::render_target *surface)
		{
			VkImageSubresourceRange range = vk::get_image_subresource_range(0, 0, 1, 1, surface->attachment_aspect_flag);
			change_image_layout(*pcmd, surface, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, range);

			//Reset deref count
			surface->deref_count = 0;
			surface->frame_tag = 0;
		}

		static void prepare_ds_for_sampling(vk::command_buffer* pcmd, vk::render_target *surface)
		{
			VkImageSubresourceRange range = vk::get_image_subresource_range(0, 0, 1, 1, surface->attachment_aspect_flag);
			change_image_layout(*pcmd, surface, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
		}

		static void invalidate_rtt_surface_contents(vk::command_buffer* /*pcmd*/, vk::render_target *rtt, vk::render_target *old_surface, bool forced)
		{
			if (forced)
			{
				rtt->old_contents = old_surface;
				rtt->dirty = true;
			}
		}
		
		static void invalidate_depth_surface_contents(vk::command_buffer* /*pcmd*/, vk::render_target *ds, vk::render_target *old_surface, bool /*forced*/)
		{
			ds->dirty = true;
			ds->old_contents = old_surface;
		}

		static
		void notify_surface_invalidated(const std::unique_ptr<vk::render_target> &surface)
		{
			surface->frame_tag = vk::get_current_frame_id();
			if (!surface->frame_tag) surface->frame_tag = 1;
		}

		static bool rtt_has_format_width_height(const std::unique_ptr<vk::render_target> &rtt, surface_color_format format, size_t width, size_t height, bool check_refs=false)
		{
			if (check_refs && rtt->deref_count == 0) //Surface may still have read refs from data 'copy'
				return false;

			VkFormat fmt = vk::get_compatible_surface_format(format).first;

			if (rtt->info.format == fmt &&
				rtt->matches_dimensions((u16)width, (u16)height))
				return true;

			return false;
		}

		static bool ds_has_format_width_height(const std::unique_ptr<vk::render_target> &ds, surface_depth_format format, size_t width, size_t height, bool check_refs=false)
		{
			if (check_refs && ds->deref_count == 0) //Surface may still have read refs from data 'copy'
				return false;

			if (ds->matches_dimensions((u16)width, (u16)height))
			{
				//Check format
				switch (ds->info.format)
				{
				case VK_FORMAT_D16_UNORM:
					return format == surface_depth_format::z16;
				case VK_FORMAT_D24_UNORM_S8_UINT:
				case VK_FORMAT_D32_SFLOAT_S8_UINT:
					return format == surface_depth_format::z24s8;
				}
			}

			return false;
		}

		static download_buffer_object issue_download_command(surface_type, surface_color_format, size_t /*width*/, size_t /*height*/, ...)
		{
			return nullptr;
		}

		static download_buffer_object issue_depth_download_command(surface_type, surface_depth_format, size_t /*width*/, size_t /*height*/, ...)
		{
			return nullptr;
		}

		static download_buffer_object issue_stencil_download_command(surface_type, surface_depth_format, size_t /*width*/, size_t /*height*/, ...)
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

		static vk::render_target *get(const std::unique_ptr<vk::render_target> &tex)
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
			invalidated_resources.clear();
		}

		void free_invalidated()
		{
			const u64 last_finished_frame = vk::get_last_completed_frame_id();
			invalidated_resources.remove_if([&](std::unique_ptr<vk::render_target> &rtt)
			{
				verify(HERE), rtt->frame_tag != 0;

				if (rtt->deref_count >= 2 && rtt->frame_tag < last_finished_frame)
					return true;

				rtt->deref_count++;
				return false;
			});
		}
	};
}
