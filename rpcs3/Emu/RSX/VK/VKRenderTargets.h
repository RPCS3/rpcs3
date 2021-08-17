#pragma once

#include "util/types.hpp"
#include "../Common/surface_store.h"

#include "VKFormats.h"
#include "VKHelpers.h"
#include "vkutils/barriers.h"
#include "vkutils/buffer_object.h"
#include "vkutils/data_heap.h"
#include "vkutils/device.h"
#include "vkutils/image.h"
#include "vkutils/scratch.h"

#include <span>

namespace vk
{
	void resolve_image(vk::command_buffer& cmd, vk::viewable_image* dst, vk::viewable_image* src);
	void unresolve_image(vk::command_buffer& cmd, vk::viewable_image* dst, vk::viewable_image* src);

	class render_target : public viewable_image, public rsx::ref_counted, public rsx::render_target_descriptor<vk::viewable_image*>
	{
		u64 cyclic_reference_sync_tag = 0;
		u64 write_barrier_sync_tag = 0;

		// Memory spilling support
		std::unique_ptr<vk::buffer> m_spilled_mem;

		// MSAA support:
		// Get the linear resolve target bound to this surface. Initialize if none exists
		vk::viewable_image* get_resolve_target_safe(vk::command_buffer& cmd);
		// Resolve the planar MSAA data into a linear block
		void resolve(vk::command_buffer& cmd);
		// Unresolve the linear data into planar MSAA data
		void unresolve(vk::command_buffer& cmd);

		// Memory management:
		// Default-initialize memory without loading
		void clear_memory(vk::command_buffer& cmd, vk::image* surface);
		// Load memory from cell and use to initialize the surface
		void load_memory(vk::command_buffer& cmd);
		// Generic - chooses whether to clear or load.
		void initialize_memory(vk::command_buffer& cmd, rsx::surface_access access);

		// Spill helpers
		// Re-initialize using spilled memory
		void unspill(vk::command_buffer& cmd);
		// Build spill transfer descriptors
		std::vector<VkBufferImageCopy> build_spill_transfer_descriptors(vk::image* target);

	public:
		u64 frame_tag = 0;              // frame id when invalidated, 0 if not invalid
		u64 last_rw_access_tag = 0;     // timestamp when this object was last used
		u64 spill_request_tag = 0;      // timestamp when spilling was requested
		bool is_bound = false;          // set when the surface is bound for rendering

		using viewable_image::viewable_image;

		vk::viewable_image* get_surface(rsx::surface_access access_type) override;
		bool is_depth_surface() const override;
		void release_ref(vk::viewable_image* t) const override;
		bool matches_dimensions(u16 _width, u16 _height) const;
		void reset_surface_counters();

		image_view* get_view(u32 remap_encoding, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap,
			VkImageAspectFlags mask = VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT) override;

		// Memory management
		bool spill(vk::command_buffer& cmd, std::vector<std::unique_ptr<vk::viewable_image>>& resolve_cache);

		// Synchronization
		void texture_barrier(vk::command_buffer& cmd);
		void memory_barrier(vk::command_buffer& cmd, rsx::surface_access access);
		void read_barrier(vk::command_buffer& cmd) { memory_barrier(cmd, rsx::surface_access::shader_read); }
		void write_barrier(vk::command_buffer& cmd) { memory_barrier(cmd, rsx::surface_access::shader_write); }
	};

	static inline vk::render_target* as_rtt(vk::image* t)
	{
		return ensure(dynamic_cast<vk::render_target*>(t));
	}

	struct surface_cache_traits
	{
		using surface_storage_type = std::unique_ptr<vk::render_target>;
		using surface_type = vk::render_target*;
		using command_list_type = vk::command_buffer&;
		using download_buffer_object = void*;
		using barrier_descriptor_t = rsx::deferred_clipped_region<vk::render_target*>;

		static VkFlags get_attachment_compression_usage_flags()
		{
			if (g_cfg.video.strict_rendering_mode)
			{
				return 0;
			}

			switch (vk::get_driver_vendor())
			{
			case driver_vendor::NVIDIA:
			case driver_vendor::INTEL:
			case driver_vendor::AMD: // TODO
				return 0;

			// Workaround to force transition to GENERAL to decompress.
			// Fixes corruption in FBO loops for ANV and RADV.
			case driver_vendor::ANV:
				return VK_IMAGE_USAGE_STORAGE_BIT;
			case driver_vendor::RADV:
				// Only needed for GFX10+
				return (vk::get_chip_family() >= chip_class::AMD_navi1x) ?
					VK_IMAGE_USAGE_STORAGE_BIT : 0;

			default:
				rsx_log.error("Unknown driver vendor!");
				return 0;
			}
		}

		static std::unique_ptr<vk::render_target> create_new_surface(
			u32 address,
			rsx::surface_color_format format,
			usz width, usz height, usz pitch,
			rsx::surface_antialiasing antialias,
			vk::render_device& device, vk::command_buffer& cmd)
		{
			const auto fmt = vk::get_compatible_surface_format(format);
			VkFormat requested_format = fmt.first;

			u8 samples;
			rsx::surface_sample_layout sample_layout;
			if (g_cfg.video.antialiasing_level == msaa_level::_auto)
			{
				samples = get_format_sample_count(antialias);
				sample_layout = rsx::surface_sample_layout::ps3;
			}
			else
			{
				samples = 1;
				sample_layout = rsx::surface_sample_layout::null;
			}

			VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			if (samples == 1) [[likely]]
			{
				usage_flags |= get_attachment_compression_usage_flags() | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			}
			else
			{
				usage_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
			}

			std::unique_ptr<vk::render_target> rtt;
			const auto [width_, height_] = rsx::apply_resolution_scale<true>(static_cast<u16>(width), static_cast<u16>(height));

			rtt = std::make_unique<vk::render_target>(device, device.get_memory_mapping().device_local,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D,
				requested_format,
				static_cast<u32>(width_), static_cast<u32>(height_), 1, 1, 1,
				static_cast<VkSampleCountFlagBits>(samples),
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL,
				usage_flags,
				0, VMM_ALLOCATION_POOL_SURFACE_CACHE, RSX_FORMAT_CLASS_COLOR);

			rtt->set_debug_name(fmt::format("RTV @0x%x", address));
			rtt->change_layout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			rtt->set_format(format);
			rtt->set_aa_mode(antialias);
			rtt->sample_layout = sample_layout;
			rtt->memory_usage_flags = rsx::surface_usage_flags::attachment;
			rtt->state_flags = rsx::surface_state_flags::erase_bkgnd;
			rtt->native_component_map = fmt.second;
			rtt->rsx_pitch = static_cast<u16>(pitch);
			rtt->native_pitch = static_cast<u16>(width) * get_format_block_size_in_bytes(format) * rtt->samples_x;
			rtt->surface_width = static_cast<u16>(width);
			rtt->surface_height = static_cast<u16>(height);
			rtt->queue_tag(address);

			rtt->add_ref();
			return rtt;
		}

		static std::unique_ptr<vk::render_target> create_new_surface(
			u32 address,
			rsx::surface_depth_format2 format,
			usz width, usz height, usz pitch,
			rsx::surface_antialiasing antialias,
			vk::render_device& device, vk::command_buffer& cmd)
		{
			const VkFormat requested_format = vk::get_compatible_depth_surface_format(device.get_formats_support(), format);
			VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

			u8 samples;
			rsx::surface_sample_layout sample_layout;
			if (g_cfg.video.antialiasing_level == msaa_level::_auto)
			{
				samples = get_format_sample_count(antialias);
				sample_layout = rsx::surface_sample_layout::ps3;
			}
			else
			{
				samples = 1;
				sample_layout = rsx::surface_sample_layout::null;
			}

			if (samples == 1) [[likely]]
			{
				usage_flags |= get_attachment_compression_usage_flags() | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			}

			std::unique_ptr<vk::render_target> ds;
			const auto [width_, height_] = rsx::apply_resolution_scale<true>(static_cast<u16>(width), static_cast<u16>(height));

			ds = std::make_unique<vk::render_target>(device, device.get_memory_mapping().device_local,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D,
				requested_format,
				static_cast<u32>(width_), static_cast<u32>(height_), 1, 1, 1,
				static_cast<VkSampleCountFlagBits>(samples),
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL,
				usage_flags,
				0, VMM_ALLOCATION_POOL_SURFACE_CACHE, rsx::classify_format(format));

			ds->set_debug_name(fmt::format("DSV @0x%x", address));
			ds->change_layout(cmd, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

			ds->set_format(format);
			ds->set_aa_mode(antialias);
			ds->sample_layout = sample_layout;
			ds->memory_usage_flags = rsx::surface_usage_flags::attachment;
			ds->state_flags = rsx::surface_state_flags::erase_bkgnd;
			ds->native_component_map = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };
			ds->native_pitch = static_cast<u16>(width) * get_format_block_size_in_bytes(format) * ds->samples_x;
			ds->rsx_pitch = static_cast<u16>(pitch);
			ds->surface_width = static_cast<u16>(width);
			ds->surface_height = static_cast<u16>(height);
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
				const auto [new_w, new_h] = rsx::apply_resolution_scale<true>(prev.width, prev.height,
					ref->get_surface_width(rsx::surface_metrics::pixels), ref->get_surface_height(rsx::surface_metrics::pixels));

				auto& dev = cmd.get_command_pool().get_owner();
				sink = std::make_unique<vk::render_target>(dev, dev.get_memory_mapping().device_local,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					VK_IMAGE_TYPE_2D,
					ref->format(),
					new_w, new_h, 1, 1, 1,
					static_cast<VkSampleCountFlagBits>(ref->samples()),
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_TILING_OPTIMAL,
					ref->info.usage,
					ref->info.flags,
					VMM_ALLOCATION_POOL_SURFACE_CACHE,
					ref->format_class());

				sink->add_ref();
				sink->set_spp(ref->get_spp());
				sink->format_info = ref->format_info;
				sink->memory_usage_flags = rsx::surface_usage_flags::storage;
				sink->state_flags = rsx::surface_state_flags::erase_bkgnd;
				sink->native_component_map = ref->native_component_map;
				sink->sample_layout = ref->sample_layout;
				sink->stencil_init_flags = ref->stencil_init_flags;
				sink->native_pitch = u16(prev.width * ref->get_bpp() * ref->samples_x);
				sink->rsx_pitch = ref->get_rsx_pitch();
				sink->surface_width = prev.width;
				sink->surface_height = prev.height;
				sink->queue_tag(address);

				const auto best_layout = (ref->info.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ?
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL :
					ref->current_layout;

				sink->change_layout(cmd, best_layout);
			}

			prev.target = sink.get();

			if (!sink->old_contents.empty())
			{
				// Deal with this, likely only needs to clear
				if (sink->surface_width > prev.width || sink->surface_height > prev.height)
				{
					sink->write_barrier(cmd);
				}
				else
				{
					sink->clear_rw_barrier();
				}
			}

			sink->rsx_pitch = ref->get_rsx_pitch();
			sink->set_old_contents_region(prev, false);
			sink->last_use_tag = ref->last_use_tag;
			sink->raster_type = ref->raster_type;     // Can't actually cut up swizzled data
		}

		static bool is_compatible_surface(const vk::render_target* surface, const vk::render_target* ref, u16 width, u16 height, u8 sample_count)
		{
			return (surface->format() == ref->format() &&
				surface->get_spp() == sample_count &&
				surface->get_surface_width() >= width &&
				surface->get_surface_height() >= height);
		}

		static void prepare_surface_for_drawing(vk::command_buffer& cmd, vk::render_target* surface)
		{
			// Special case barrier
			surface->memory_barrier(cmd, rsx::surface_access::gpu_reference);

			if (surface->aspect() == VK_IMAGE_ASPECT_COLOR_BIT)
			{
				surface->change_layout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			}
			else
			{
				surface->change_layout(cmd, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			}

			surface->reset_surface_counters();
			surface->memory_usage_flags |= rsx::surface_usage_flags::attachment;
			surface->is_bound = true;
		}

		static void prepare_surface_for_sampling(vk::command_buffer& /*cmd*/, vk::render_target* surface)
		{
			surface->is_bound = false;
		}

		static bool surface_is_pitch_compatible(const std::unique_ptr<vk::render_target>& surface, usz pitch)
		{
			return surface->rsx_pitch == pitch;
		}

		static void invalidate_surface_contents(vk::command_buffer& /*cmd*/, vk::render_target* surface, u32 address, usz pitch)
		{
			surface->rsx_pitch = static_cast<u16>(pitch);
			surface->queue_tag(address);
			surface->last_use_tag = 0;
			surface->stencil_init_flags = 0;
			surface->memory_usage_flags = rsx::surface_usage_flags::unknown;
			surface->raster_type = rsx::surface_raster_type::linear;
		}

		static void notify_surface_invalidated(const std::unique_ptr<vk::render_target>& surface)
		{
			surface->frame_tag = vk::get_current_frame_id();
			if (!surface->frame_tag) surface->frame_tag = 1;

			if (!surface->old_contents.empty())
			{
				// TODO: Retire the deferred writes
				surface->clear_rw_barrier();
			}

			surface->release();
		}

		static void notify_surface_persist(const std::unique_ptr<vk::render_target>& /*surface*/)
		{}

		static void notify_surface_reused(const std::unique_ptr<vk::render_target>& surface)
		{
			surface->state_flags |= rsx::surface_state_flags::erase_bkgnd;
			surface->add_ref();
		}

		static bool int_surface_matches_properties(
			const std::unique_ptr<vk::render_target>& surface,
			VkFormat format,
			usz width, usz height,
			rsx::surface_antialiasing antialias,
			bool check_refs)
		{
			if (check_refs && surface->has_refs())
			{
				// Surface may still have read refs from data 'copy'
				return false;
			}

			return (surface->info.format == format &&
				surface->get_spp() == get_format_sample_count(antialias) &&
				surface->matches_dimensions(static_cast<u16>(width), static_cast<u16>(height)));
		}

		static bool surface_matches_properties(
			const std::unique_ptr<vk::render_target>& surface,
			rsx::surface_color_format format,
			usz width, usz height,
			rsx::surface_antialiasing antialias,
			bool check_refs = false)
		{
			VkFormat vk_format = vk::get_compatible_surface_format(format).first;
			return int_surface_matches_properties(surface, vk_format, width, height, antialias, check_refs);
		}

		static bool surface_matches_properties(
			const std::unique_ptr<vk::render_target>& surface,
			rsx::surface_depth_format2 format,
			usz width, usz height,
			rsx::surface_antialiasing antialias,
			bool check_refs = false)
		{
			auto device = vk::get_current_renderer();
			VkFormat vk_format = vk::get_compatible_depth_surface_format(device->get_formats_support(), format);
			return int_surface_matches_properties(surface, vk_format, width, height, antialias, check_refs);
		}

		static vk::render_target* get(const std::unique_ptr<vk::render_target>& tex)
		{
			return tex.get();
		}
	};

	class surface_cache : public rsx::surface_store<vk::surface_cache_traits>
	{
	private:
		u64 get_surface_cache_memory_quota(u64 total_device_memory);

	public:
		void destroy();
		bool spill_unused_memory();
		bool is_overallocated();
		bool can_collapse_surface(const std::unique_ptr<vk::render_target>& surface, rsx::problem_severity severity) override;
		bool handle_memory_pressure(vk::command_buffer& cmd, rsx::problem_severity severity) override;
		void free_invalidated(vk::command_buffer& cmd, rsx::problem_severity memory_pressure);
	};
}
//h