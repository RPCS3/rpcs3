#pragma once

#include "util/types.hpp"
#include "../Common/surface_store.h"

#include "VKFormats.h"
#include "VKHelpers.h"
#include "vkutils/barriers.h"
#include "vkutils/buffer_object.h"
#include "vkutils/device.h"
#include "vkutils/image.h"
#include "vkutils/scratch.h"

namespace vk
{
	namespace surface_cache_utils
	{
		void dispose(vk::buffer* buf);
	}

	void resolve_image(vk::command_buffer& cmd, vk::viewable_image* dst, vk::viewable_image* src);
	void unresolve_image(vk::command_buffer& cmd, vk::viewable_image* dst, vk::viewable_image* src);

	class image_reference_sync_barrier
	{
		u32 m_texture_barrier_count = 0;
		u32 m_draw_barrier_count = 0;
		bool m_allow_skip_barrier = true;

	public:
		void on_insert_texture_barrier()
		{
			m_texture_barrier_count++;
			m_allow_skip_barrier = false;
		}

		void on_insert_draw_barrier()
		{
			// Account for corner case where the same texture can be bound to more than 1 slot
			m_draw_barrier_count = std::max(m_draw_barrier_count + 1, m_texture_barrier_count);
		}

		void allow_skip()
		{
			m_allow_skip_barrier = true;
		}

		void reset()
		{
			m_texture_barrier_count = m_draw_barrier_count = 0ull;
			m_allow_skip_barrier = false;
		}

		bool can_skip() const
		{
			return m_allow_skip_barrier;
		}

		bool is_enabled() const
		{
			return !!m_texture_barrier_count;
		}

		bool requires_post_loop_barrier() const
		{
			return is_enabled() && m_texture_barrier_count < m_draw_barrier_count;
		}
	};

	class render_target : public viewable_image, public rsx::render_target_descriptor<vk::viewable_image*>
	{
		// Cyclic reference hazard tracking
		image_reference_sync_barrier m_cyclic_ref_tracker;

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
		bool matches_dimensions(u16 _width, u16 _height) const;
		void reset_surface_counters();

		image_view* get_view(const rsx::texture_channel_remap_t& remap,
			VkImageAspectFlags mask = VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT) override;

		// Memory management
		bool spill(vk::command_buffer& cmd, std::vector<std::unique_ptr<vk::viewable_image>>& resolve_cache);

		// Synchronization
		void texture_barrier(vk::command_buffer& cmd);
		void post_texture_barrier(vk::command_buffer& cmd);
		void memory_barrier(vk::command_buffer& cmd, rsx::surface_access access);
		void read_barrier(vk::command_buffer& cmd) { memory_barrier(cmd, rsx::surface_access::shader_read); }
		void write_barrier(vk::command_buffer& cmd) { memory_barrier(cmd, rsx::surface_access::shader_write); }
	};

	static inline vk::render_target* as_rtt(vk::image* t)
	{
		return ensure(dynamic_cast<vk::render_target*>(t));
	}

	static inline const vk::render_target* as_rtt(const vk::image* t)
	{
		return ensure(dynamic_cast<const vk::render_target*>(t));
	}

	struct surface_cache_traits
	{
		using surface_storage_type = std::unique_ptr<vk::render_target>;
		using surface_type = vk::render_target*;
		using buffer_object_storage_type = std::unique_ptr<vk::buffer>;
		using buffer_object_type = vk::buffer*;
		using command_list_type = vk::command_buffer&;
		using download_buffer_object = void*;
		using barrier_descriptor_t = rsx::deferred_clipped_region<vk::render_target*>;

		static std::pair<VkImageUsageFlags, VkImageCreateFlags> get_attachment_create_flags(VkFormat format, [[maybe_unused]] u8 samples)
		{
			if (g_cfg.video.strict_rendering_mode)
			{
				return {};
			}

			// If we have driver support for FBO loops, set the usage flag for it.
			if (vk::get_current_renderer()->get_framebuffer_loops_support())
			{
				return { VK_IMAGE_USAGE_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT };
			}

			// Workarounds to force transition to GENERAL to decompress.
			// Fixes corruption in FBO loops for ANV and RADV.
			switch (vk::get_driver_vendor())
			{
			case driver_vendor::ANV:
				if (const auto format_features = vk::get_current_renderer()->get_format_properties(format);
					format_features.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)
				{
					// Only set if supported by hw
					return { VK_IMAGE_USAGE_STORAGE_BIT, 0 };
				}
				break;
			case driver_vendor::AMD:
			case driver_vendor::RADV:
				if (vk::get_chip_family() >= chip_class::AMD_navi1x)
				{
					// Only needed for GFX10+
					return { 0, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT };
				}
				break;
			default:
				rsx_log.error("Unknown driver vendor!");
				[[ fallthrough ]];
			case driver_vendor::NVIDIA:
			case driver_vendor::INTEL:
			case driver_vendor::MVK:
			case driver_vendor::DOZEN:
			case driver_vendor::LAVAPIPE:
			case driver_vendor::V3DV:
			case driver_vendor::HONEYKRISP:
			case driver_vendor::PANVK:
			case driver_vendor::ARM_MALI:
				break;
			}

			return {};
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

			auto [usage_flags, create_flags] = get_attachment_create_flags(requested_format, samples);
			usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

			if (samples == 1) [[likely]]
			{
				usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
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
				create_flags,
				VMM_ALLOCATION_POOL_SURFACE_CACHE,
				RSX_FORMAT_CLASS_COLOR);

			rtt->set_debug_name(fmt::format("RTV @0x%x, fmt=0x%x", address, static_cast<int>(format)));
			rtt->change_layout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			rtt->set_format(format);
			rtt->set_aa_mode(antialias);
			rtt->sample_layout = sample_layout;
			rtt->memory_usage_flags = rsx::surface_usage_flags::attachment;
			rtt->state_flags = rsx::surface_state_flags::erase_bkgnd;
			rtt->native_component_map = fmt.second;
			rtt->rsx_pitch = static_cast<u32>(pitch);
			rtt->native_pitch = static_cast<u32>(width) * get_format_block_size_in_bytes(format) * rtt->samples_x;
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

			auto [usage_flags, create_flags] = get_attachment_create_flags(requested_format, samples);
			usage_flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

			if (samples == 1) [[likely]]
			{
				usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
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
				create_flags,
				VMM_ALLOCATION_POOL_SURFACE_CACHE,
				rsx::classify_format(format));

			ds->set_debug_name(fmt::format("DSV @0x%x", address));
			ds->change_layout(cmd, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

			ds->set_format(format);
			ds->set_aa_mode(antialias);
			ds->sample_layout = sample_layout;
			ds->memory_usage_flags = rsx::surface_usage_flags::attachment;
			ds->state_flags = rsx::surface_state_flags::erase_bkgnd;
			ds->native_component_map = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };
			ds->native_pitch = static_cast<u32>(width) * get_format_block_size_in_bytes(format) * ds->samples_x;
			ds->rsx_pitch = static_cast<u32>(pitch);
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
					ref->get_surface_width<rsx::surface_metrics::pixels>(), ref->get_surface_height<rsx::surface_metrics::pixels>());

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
				sink->native_pitch = static_cast<u32>(prev.width) * ref->get_bpp() * ref->samples_x;
				sink->rsx_pitch = ref->get_rsx_pitch();
				sink->surface_width = prev.width;
				sink->surface_height = prev.height;
				sink->queue_tag(address);

				const auto best_layout = (ref->info.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ?
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL :
					ref->current_layout;

				sink->change_layout(cmd, best_layout);
			}

			sink->on_clone_from(ref);

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

			prev.target = sink.get();
			sink->set_old_contents_region(prev, false);
		}

		static std::unique_ptr<vk::render_target> convert_pitch(
			vk::command_buffer& /*cmd*/,
			std::unique_ptr<vk::render_target>& src,
			usz /*out_pitch*/)
		{
			// TODO
			src->state_flags = rsx::surface_state_flags::erase_bkgnd;
			return {};
		}

		static bool is_compatible_surface(const vk::render_target* surface, const vk::render_target* ref, u16 width, u16 height, u8 sample_count)
		{
			return (surface->format() == ref->format() &&
				surface->get_spp() == sample_count &&
				surface->get_surface_width() == width &&
				surface->get_surface_height() == height);
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

		static void int_invalidate_surface_contents(
			vk::command_buffer& /*cmd*/,
			vk::render_target* surface,
			u32 address,
			usz pitch)
		{
			surface->rsx_pitch = static_cast<u32>(pitch);
			surface->queue_tag(address);
			surface->last_use_tag = 0;
			surface->stencil_init_flags = 0;
			surface->memory_usage_flags = rsx::surface_usage_flags::unknown;
			surface->raster_type = rsx::surface_raster_type::linear;
		}

		static void invalidate_surface_contents(
			vk::command_buffer& cmd,
			vk::render_target* surface,
			rsx::surface_color_format format,
			u32 address,
			usz pitch)
		{
			const auto fmt = vk::get_compatible_surface_format(format);
			surface->set_format(format);
			surface->set_native_component_layout(fmt.second);
			surface->set_debug_name(fmt::format("RTV @0x%x, fmt=0x%x", address, static_cast<int>(format)));

			int_invalidate_surface_contents(cmd, surface, address, pitch);
		}

		static void invalidate_surface_contents(
			vk::command_buffer& cmd,
			vk::render_target* surface,
			rsx::surface_depth_format2 format,
			u32 address,
			usz pitch)
		{
			surface->set_format(format);
			surface->set_debug_name(fmt::format("DSV @0x%x", address));

			int_invalidate_surface_contents(cmd, surface, address, pitch);
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

		static void spill_buffer(std::unique_ptr<vk::buffer>& /*bo*/)
		{
			// TODO
		}

		static void unspill_buffer(std::unique_ptr<vk::buffer>& /*bo*/)
		{
			// TODO
		}

		static void write_render_target_to_memory(
			vk::command_buffer& cmd,
			vk::buffer* bo,
			vk::render_target* surface,
			u64 dst_offset_in_buffer,
			u64 src_offset_in_buffer,
			u64 max_copy_length)
		{
			surface->read_barrier(cmd);
			vk::image* source = surface->get_surface(rsx::surface_access::transfer_read);
			const bool is_scaled = surface->width() != surface->surface_width;
			if (is_scaled)
			{
				const areai src_rect = { 0, 0, static_cast<int>(source->width()), static_cast<int>(source->height()) };
				const areai dst_rect = { 0, 0, surface->get_surface_width<rsx::surface_metrics::samples, int>(), surface->get_surface_height<rsx::surface_metrics::samples, int>() };

				auto scratch = vk::get_typeless_helper(source->format(), source->format_class(), dst_rect.x2, dst_rect.y2);
				vk::copy_scaled_image(cmd, source, scratch, src_rect, dst_rect, 1, true, VK_FILTER_NEAREST);

				source = scratch;
			}

			auto dest = bo;
			const auto transfer_size = surface->get_memory_range().length();
			if (transfer_size > max_copy_length || src_offset_in_buffer || surface->is_depth_surface())
			{
				auto scratch = vk::get_scratch_buffer(cmd, transfer_size * 4);
				dest = scratch;
			}

			VkBufferImageCopy region =
			{
				.bufferOffset = (dest == bo) ? dst_offset_in_buffer : 0,
				.bufferRowLength = surface->rsx_pitch / surface->get_bpp(),
				.bufferImageHeight = 0,
				.imageSubresource = { source->aspect(), 0, 0, 1 },
				.imageOffset = {},
				.imageExtent = {
					.width = source->width(),
					.height = source->height(),
					.depth = 1
				}
			};

			// inject post-transfer barrier
			image_readback_options_t options{};
			options.sync_region =
			{
				.offset = src_offset_in_buffer,
				.length = max_copy_length
			};
			vk::copy_image_to_buffer(cmd, source, dest, region, options);

			if (dest != bo)
			{
				VkBufferCopy copy = { src_offset_in_buffer, dst_offset_in_buffer, max_copy_length };
				vkCmdCopyBuffer(cmd, dest->value, bo->value, 1, &copy);

				vk::insert_buffer_memory_barrier(cmd,
					bo->value, dst_offset_in_buffer, max_copy_length,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
			}
		}

		template <int BlockSize>
		static vk::buffer* merge_bo_list(vk::command_buffer& cmd, std::vector<vk::buffer*>& list)
		{
			u32 required_bo_size = 0;
			for (auto& bo : list)
			{
				required_bo_size += (bo ? bo->size() : BlockSize);
			}

			// Create dst
			auto& dev = cmd.get_command_pool().get_owner();
			auto dst = new vk::buffer(dev,
				required_bo_size,
				dev.get_memory_mapping().device_local, 0,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				0, VMM_ALLOCATION_POOL_SURFACE_CACHE);

			// TODO: Initialize the buffer with system RAM contents

			// Copy all the data over from the sub-blocks
			u32 offset = 0;
			for (auto& bo : list)
			{
				if (!bo)
				{
					offset += BlockSize;
					continue;
				}

				VkBufferCopy copy = { 0, offset, ::size32(*bo) };
				offset += ::size32(*bo);
				vkCmdCopyBuffer(cmd, bo->value, dst->value, 1, &copy);

				// Cleanup
				vk::surface_cache_utils::dispose(bo);
			}

			return dst;
		}

		template <typename T>
		static T* get(const std::unique_ptr<T>& obj)
		{
			return obj.get();
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
		void trim(vk::command_buffer& cmd, rsx::problem_severity memory_pressure);
	};
}
