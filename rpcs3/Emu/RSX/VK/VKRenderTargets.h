#pragma once

#include "stdafx.h"
#include "VKHelpers.h"
#include "VKFormats.h"
#include "../GCM.h"
#include "../Common/surface_store.h"
#include "../Common/TextureUtils.h"
#include "../Common/texture_cache_utils.h"
#include "../rsx_utils.h"

namespace vk
{
	struct render_target : public viewable_image, public rsx::ref_counted, public rsx::render_target_descriptor<vk::viewable_image*>
	{
		u16 native_pitch = 0;
		u16 rsx_pitch = 0;

		u16 surface_width = 0;
		u16 surface_height = 0;

		u64 frame_tag = 0; // frame id when invalidated, 0 if not invalid

		using viewable_image::viewable_image;

		vk::viewable_image* get_surface() override
		{
			return (vk::viewable_image*)this;
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

		bool is_depth_surface() const override
		{
			return !!(aspect() & VK_IMAGE_ASPECT_DEPTH_BIT);
		}

		void release_ref(vk::viewable_image* t) const override
		{
			static_cast<vk::render_target*>(t)->release();
		}

		bool matches_dimensions(u16 _width, u16 _height) const
		{
			//Use forward scaling to account for rounding and clamping errors
			return (rsx::apply_resolution_scale(_width, true) == width()) && (rsx::apply_resolution_scale(_height, true) == height());
		}

		void memory_barrier(vk::command_buffer& cmd, bool force_init = false)
		{
			// Helper to optionally clear/initialize memory contents depending on barrier type
			auto clear_surface_impl = [&]()
			{
				push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				VkImageSubresourceRange range{ aspect(), 0, 1, 0, 1 };

				if (aspect() & VK_IMAGE_ASPECT_COLOR_BIT)
				{
					VkClearColorValue color{};
					vkCmdClearColorImage(cmd, value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range);
				}
				else
				{
					VkClearDepthStencilValue clear{ 1.f, 255 };
					vkCmdClearDepthStencilImage(cmd, value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range);
				}

				pop_layout(cmd);
				state_flags &= ~rsx::surface_state_flags::erase_bkgnd;
			};

			auto null_transfer_impl = [&]()
			{
				if (dirty() && (force_init || state_flags & rsx::surface_state_flags::erase_bkgnd))
				{
					// Initialize memory contents if we did not find anything usable
					// TODO: Properly sync with Cell
					clear_surface_impl();
					on_write();
				}
				else
				{
					verify(HERE), state_flags == rsx::surface_state_flags::ready;
				}
			};

			if (!old_contents)
			{
				null_transfer_impl();
				return;
			}

			auto src_texture = static_cast<vk::render_target*>(old_contents.source);
			if (!rsx::pitch_compatible(this, src_texture))
			{
				LOG_TRACE(RSX, "Pitch mismatch, could not transfer inherited memory");

				clear_rw_barrier();
				return;
			}

			const auto src_bpp = src_texture->get_bpp();
			const auto dst_bpp = get_bpp();
			rsx::typeless_xfer typeless_info{};

			if (src_texture->info.format == info.format)
			{
				verify(HERE), src_bpp == dst_bpp;
			}
			else
			{
				if (!formats_are_bitcast_compatible(format(), src_texture->format()) ||
					src_texture->aspect() != aspect())
				{
					typeless_info.src_is_typeless = true;
					typeless_info.src_context = rsx::texture_upload_context::framebuffer_storage;
					typeless_info.src_native_format_override = (u32)info.format;
					typeless_info.src_is_depth = src_texture->is_depth_surface();
					typeless_info.src_scaling_hint = f32(src_bpp) / dst_bpp;
				}
			}

			vk::blitter hw_blitter;
			old_contents.init_transfer(this);

			if (state_flags & rsx::surface_state_flags::erase_bkgnd)
			{
				const auto area = old_contents.dst_rect();
				if (area.x1 > 0 || area.y1 > 0 || unsigned(area.x2) < width() || unsigned(area.y2) < height())
				{
					clear_surface_impl();
				}
				else
				{
					state_flags &= ~rsx::surface_state_flags::erase_bkgnd;
				}
			}

			hw_blitter.scale_image(cmd, old_contents.source, this,
				old_contents.src_rect(),
				old_contents.dst_rect(),
				/*linear?*/false, /*depth?(unused)*/false, typeless_info);

			on_write();
		}

		void read_barrier(vk::command_buffer& cmd) { memory_barrier(cmd, true); }
		void write_barrier(vk::command_buffer& cmd) { memory_barrier(cmd, false); }
	};

	static inline vk::render_target* as_rtt(vk::image* t)
	{
		return static_cast<vk::render_target*>(t);
	}
}

namespace rsx
{
	struct vk_render_target_traits
	{
		using surface_storage_type = std::unique_ptr<vk::render_target>;
		using surface_type = vk::render_target*;
		using command_list_type = vk::command_buffer&;
		using download_buffer_object = void*;
		using barrier_descriptor_t = rsx::deferred_clipped_region<vk::render_target*>;

		static std::unique_ptr<vk::render_target> create_new_surface(
			u32 address,
			surface_color_format format,
			size_t width, size_t height, size_t pitch,
			vk::render_device &device, vk::command_buffer& cmd)
		{
			auto fmt = vk::get_compatible_surface_format(format);
			VkFormat requested_format = fmt.first;

			std::unique_ptr<vk::render_target> rtt;
			rtt = std::make_unique<vk::render_target>(device, device.get_memory_mapping().device_local,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D,
				requested_format,
				static_cast<uint32_t>(rsx::apply_resolution_scale((u16)width, true)), static_cast<uint32_t>(rsx::apply_resolution_scale((u16)height, true)), 1, 1, 1,
				VK_SAMPLE_COUNT_1_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
				0);

			change_image_layout(cmd, rtt.get(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT));

			rtt->set_format(format);
			rtt->memory_usage_flags = rsx::surface_usage_flags::attachment;
			rtt->state_flags = rsx::surface_state_flags::erase_bkgnd;
			rtt->native_component_map = fmt.second;
			rtt->rsx_pitch = (u16)pitch;
			rtt->native_pitch = (u16)width * get_format_block_size_in_bytes(format);
			rtt->surface_width = (u16)width;
			rtt->surface_height = (u16)height;
			rtt->queue_tag(address);

			rtt->add_ref();
			return rtt;
		}

		static std::unique_ptr<vk::render_target> create_new_surface(
			u32 address,
			surface_depth_format format,
			size_t width, size_t height, size_t pitch,
			vk::render_device &device, vk::command_buffer& cmd)
		{
			VkFormat requested_format = vk::get_compatible_depth_surface_format(device.get_formats_support(), format);
			VkImageSubresourceRange range = vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_DEPTH_BIT);

			if (requested_format != VK_FORMAT_D16_UNORM)
				range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

			const auto scale = rsx::get_resolution_scale();

			std::unique_ptr<vk::render_target> ds;
			ds = std::make_unique<vk::render_target>(device, device.get_memory_mapping().device_local,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D,
				requested_format,
				static_cast<uint32_t>(rsx::apply_resolution_scale((u16)width, true)), static_cast<uint32_t>(rsx::apply_resolution_scale((u16)height, true)), 1, 1, 1,
				VK_SAMPLE_COUNT_1_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT| VK_IMAGE_USAGE_TRANSFER_SRC_BIT| VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
				0);


			ds->set_format(format);
			ds->memory_usage_flags= rsx::surface_usage_flags::attachment;
			ds->state_flags = rsx::surface_state_flags::erase_bkgnd;
			ds->native_component_map = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };

			change_image_layout(cmd, ds.get(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, range);

			ds->native_pitch = (u16)width * 2;
			if (format == rsx::surface_depth_format::z24s8)
				ds->native_pitch *= 2;

			ds->rsx_pitch = (u16)pitch;
			ds->surface_width = (u16)width;
			ds->surface_height = (u16)height;
			ds->queue_tag(address);

			ds->add_ref();
			return ds;
		}

		static void clone_surface(
			vk::command_buffer& cmd,
			std::unique_ptr<vk::render_target>& sink, vk::render_target* ref,
			u32 address, barrier_descriptor_t& prev)
		{
			if (!sink)
			{
				const auto new_w = rsx::apply_resolution_scale(prev.width, true, ref->get_surface_width());
				const auto new_h = rsx::apply_resolution_scale(prev.height, true, ref->get_surface_height());

				auto& dev = cmd.get_command_pool().get_owner();
				sink = std::make_unique<vk::render_target>(dev, dev.get_memory_mapping().device_local,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					VK_IMAGE_TYPE_2D,
					ref->format(),
					new_w, new_h, 1, 1, 1,
					VK_SAMPLE_COUNT_1_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_TILING_OPTIMAL,
					ref->info.usage,
					ref->info.flags);

				sink->add_ref();
				sink->format_info = ref->format_info;
				sink->memory_usage_flags = rsx::surface_usage_flags::storage;
				sink->state_flags = rsx::surface_state_flags::erase_bkgnd;
				sink->native_component_map = ref->native_component_map;
				sink->native_pitch = u16(prev.width * ref->get_bpp());
				sink->surface_width = prev.width;
				sink->surface_height = prev.height;
				sink->queue_tag(address);

				change_image_layout(cmd, sink.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}

			prev.target = sink.get();

			sink->rsx_pitch = ref->get_rsx_pitch();
			sink->sync_tag();
			sink->set_old_contents_region(prev, false);
			sink->last_use_tag = ref->last_use_tag;
		}

		static bool is_compatible_surface(const vk::render_target* surface, const vk::render_target* ref, u16 width, u16 height, u8 /*sample_count*/)
		{
			return (surface->format() == ref->format() &&
					surface->get_surface_width() >= width &&
					surface->get_surface_height() >= height);
		}

		static void get_surface_info(vk::render_target *surface, rsx::surface_format_info *info)
		{
			info->rsx_pitch = surface->rsx_pitch;
			info->native_pitch = surface->native_pitch;
			info->surface_width = surface->get_surface_width();
			info->surface_height = surface->get_surface_height();
			info->bpp = surface->get_bpp();
		}

		static void prepare_rtt_for_drawing(vk::command_buffer& cmd, vk::render_target *surface)
		{
			surface->change_layout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			surface->frame_tag = 0;
			surface->memory_usage_flags |= rsx::surface_usage_flags::attachment;
		}

		static void prepare_rtt_for_sampling(vk::command_buffer& cmd, vk::render_target *surface)
		{
			surface->change_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		static void prepare_ds_for_drawing(vk::command_buffer& cmd, vk::render_target *surface)
		{
			surface->change_layout(cmd, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			surface->frame_tag = 0;
			surface->memory_usage_flags |= rsx::surface_usage_flags::attachment;
		}

		static void prepare_ds_for_sampling(vk::command_buffer& cmd, vk::render_target *surface)
		{
			surface->change_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		static bool surface_is_pitch_compatible(const std::unique_ptr<vk::render_target> &surface, size_t pitch)
		{
			return surface->rsx_pitch == pitch;
		}

		static void invalidate_surface_contents(vk::command_buffer& /*cmd*/, vk::render_target *surface, u32 address, size_t pitch)
		{
			surface->rsx_pitch = (u16)pitch;
			surface->reset_aa_mode();
			surface->queue_tag(address);
			surface->last_use_tag = 0;
			surface->memory_usage_flags = rsx::surface_usage_flags::unknown;
		}

		static void notify_surface_invalidated(const std::unique_ptr<vk::render_target> &surface)
		{
			surface->frame_tag = vk::get_current_frame_id();
			if (!surface->frame_tag) surface->frame_tag = 1;

			if (surface->old_contents)
			{
				// TODO: Retire the deferred writes
				surface->clear_rw_barrier();
			}

			surface->release();
		}

		static void notify_surface_persist(const std::unique_ptr<vk::render_target> &surface)
		{
			surface->save_aa_mode();
		}

		static void notify_surface_reused(const std::unique_ptr<vk::render_target> &surface)
		{
			surface->state_flags |= rsx::surface_state_flags::erase_bkgnd;
			surface->add_ref();
		}

		static bool rtt_has_format_width_height(const std::unique_ptr<vk::render_target> &rtt, surface_color_format format, size_t width, size_t height, bool check_refs=false)
		{
			if (check_refs && rtt->has_refs()) // Surface may still have read refs from data 'copy'
				return false;

			VkFormat fmt = vk::get_compatible_surface_format(format).first;

			if (rtt->info.format == fmt &&
				rtt->matches_dimensions((u16)width, (u16)height))
				return true;

			return false;
		}

		static bool ds_has_format_width_height(const std::unique_ptr<vk::render_target> &ds, surface_depth_format format, size_t width, size_t height, bool check_refs=false)
		{
			if (check_refs && ds->has_refs()) //Surface may still have read refs from data 'copy'
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
			return {};
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
			invalidate_all();
			invalidated_resources.clear();
		}

		void free_invalidated()
		{
			const u64 last_finished_frame = vk::get_last_completed_frame_id();
			invalidated_resources.remove_if([&](std::unique_ptr<vk::render_target> &rtt)
			{
				verify(HERE), rtt->frame_tag != 0;

				if (rtt->unused_check_count() >= 2 && rtt->frame_tag < last_finished_frame)
					return true;

				return false;
			});
		}
	};
}
