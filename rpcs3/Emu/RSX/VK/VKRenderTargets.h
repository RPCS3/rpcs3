#pragma once

#include "stdafx.h"
#include "VKHelpers.h"
#include "VKFormats.h"
#include "../GCM.h"
#include "../Common/surface_store.h"
#include "../Common/TextureUtils.h"
#include "../Common/texture_cache_utils.h"

namespace vk
{
	void resolve_image(vk::command_buffer& cmd, vk::viewable_image* dst, vk::viewable_image* src);
	void unresolve_image(vk::command_buffer& cmd, vk::viewable_image* dst, vk::viewable_image* src);

	class render_target : public viewable_image, public rsx::ref_counted, public rsx::render_target_descriptor<vk::viewable_image*>
	{
		// Get the linear resolve target bound to this surface. Initialize if none exists
		vk::viewable_image* get_resolve_target_safe(vk::command_buffer& cmd)
		{
			if (!resolve_surface)
			{
				// Create a resolve surface
				auto pdev = vk::get_current_renderer();
				const auto resolve_w = width() * samples_x;
				const auto resolve_h = height() * samples_y;

				VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				usage |= (this->info.usage & (VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT));

				resolve_surface.reset(new vk::viewable_image(
					*pdev,
					pdev->get_memory_mapping().device_local,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					VK_IMAGE_TYPE_2D,
					format(),
					resolve_w, resolve_h, 1, 1, 1,
					VK_SAMPLE_COUNT_1_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_TILING_OPTIMAL,
					usage,
					0));

				resolve_surface->native_component_map = native_component_map;
				resolve_surface->change_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
			}

			return resolve_surface.get();
		}

		// Resolve the planar MSAA data into a linear block
		void resolve(vk::command_buffer& cmd)
		{
			VkImageSubresourceRange range = { aspect(), 0, 1, 0, 1 };

			// NOTE: This surface can only be in the ATTACHMENT_OPTIMAL layout
			// The resolve surface can be in any type of access, but we have to assume it is likely in read-only mode like shader read-only

			if (!is_depth_surface()) [[likely]]
			{
				verify(HERE), current_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

				// This is the source; finish writing before reading
				vk::insert_image_memory_barrier(
					cmd, this->value,
					this->current_layout, VK_IMAGE_LAYOUT_GENERAL,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					range);

				// This is the target; finish reading before writing
				vk::insert_image_memory_barrier(
					cmd, resolve_surface->value,
					resolve_surface->current_layout, VK_IMAGE_LAYOUT_GENERAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_SHADER_READ_BIT,
					VK_ACCESS_SHADER_WRITE_BIT,
					range);

				this->current_layout = VK_IMAGE_LAYOUT_GENERAL;
				resolve_surface->current_layout = VK_IMAGE_LAYOUT_GENERAL;
			}
			else
			{
				this->push_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				resolve_surface->change_layout(cmd, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			}

			vk::resolve_image(cmd, resolve_surface.get(), this);

			if (!is_depth_surface()) [[likely]]
			{
				vk::insert_image_memory_barrier(
					cmd, this->value,
					this->current_layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					range);

				vk::insert_image_memory_barrier(
					cmd, resolve_surface->value,
					VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
					range);

				this->current_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				resolve_surface->current_layout = VK_IMAGE_LAYOUT_GENERAL;
			}
			else
			{
				this->pop_layout(cmd);
				resolve_surface->change_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
			}

			msaa_flags &= ~(rsx::surface_state_flags::require_resolve);
		}

		// Unresolve the linear data into planar MSAA data
		void unresolve(vk::command_buffer& cmd)
		{
			verify(HERE), !(msaa_flags & rsx::surface_state_flags::require_resolve);
			VkImageSubresourceRange range = { aspect(), 0, 1, 0, 1 };

			if (!is_depth_surface()) [[likely]]
			{
				verify(HERE), current_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

				// This is the dest; finish reading before writing
				vk::insert_image_memory_barrier(
					cmd, this->value,
					this->current_layout, VK_IMAGE_LAYOUT_GENERAL,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_ACCESS_SHADER_WRITE_BIT,
					range);

				// This is the source; finish writing before reading
				vk::insert_image_memory_barrier(
					cmd, resolve_surface->value,
					resolve_surface->current_layout, VK_IMAGE_LAYOUT_GENERAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					range);

				this->current_layout = VK_IMAGE_LAYOUT_GENERAL;
				resolve_surface->current_layout = VK_IMAGE_LAYOUT_GENERAL;
			}
			else
			{
				this->push_layout(cmd, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
				resolve_surface->change_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}

			vk::unresolve_image(cmd, this, resolve_surface.get());

			if (!is_depth_surface()) [[likely]]
			{
				vk::insert_image_memory_barrier(
					cmd, this->value,
					this->current_layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
					range);

				vk::insert_image_memory_barrier(
					cmd, resolve_surface->value,
					VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					range);

				this->current_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				resolve_surface->current_layout = VK_IMAGE_LAYOUT_GENERAL;
			}
			else
			{
				this->pop_layout(cmd);
				resolve_surface->change_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
			}

			msaa_flags &= ~(rsx::surface_state_flags::require_unresolve);
		}

		// Default-initialize memory without loading
		void clear_memory(vk::command_buffer& cmd, vk::image *surface)
		{
			const auto optimal_layout = (surface->current_layout == VK_IMAGE_LAYOUT_GENERAL) ?
				VK_IMAGE_LAYOUT_GENERAL :
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			surface->push_layout(cmd, optimal_layout);

			VkImageSubresourceRange range{ surface->aspect(), 0, 1, 0, 1 };
			if (surface->aspect() & VK_IMAGE_ASPECT_COLOR_BIT)
			{
				VkClearColorValue color = {{0.f, 0.f, 0.f, 1.f}};
				vkCmdClearColorImage(cmd, surface->value, surface->current_layout, &color, 1, &range);
			}
			else
			{
				VkClearDepthStencilValue clear{ 1.f, 255 };
				vkCmdClearDepthStencilImage(cmd, surface->value, surface->current_layout, &clear, 1, &range);
			}

			surface->pop_layout(cmd);

			if (surface == this)
			{
				state_flags &= ~rsx::surface_state_flags::erase_bkgnd;
			}
		}

		// Load memory from cell and use to initialize the surface
		void load_memory(vk::command_buffer& cmd)
		{
			auto& upload_heap = *vk::get_upload_heap();

			u32 gcm_format;
			if (is_depth_surface())
			{
				gcm_format = get_compatible_gcm_format(format_info.gcm_depth_format).first;
			}
			else
			{
				gcm_format = get_compatible_gcm_format(format_info.gcm_color_format).first;
			}

			rsx_subresource_layout subres{};
			subres.width_in_block = subres.width_in_texel = surface_width * samples_x;
			subres.height_in_block = subres.height_in_texel = surface_height * samples_y;
			subres.pitch_in_block = rsx_pitch / get_bpp();
			subres.depth = 1;
			subres.data = { vm::get_super_ptr<const std::byte>(base_addr), static_cast<gsl::span<const std::byte>::index_type>(rsx_pitch * surface_height * samples_y) };

			if (g_cfg.video.resolution_scale_percent == 100 && spp == 1) [[likely]]
			{
				push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				vk::copy_mipmaped_image_using_buffer(cmd, this, { subres }, gcm_format, false, 1, aspect(), upload_heap, rsx_pitch);
				pop_layout(cmd);
			}
			else
			{
				vk::image* content = nullptr;
				vk::image* final_dst = (samples() > 1) ? get_resolve_target_safe(cmd) : this;

				if (final_dst->width() == subres.width_in_block && final_dst->height() == subres.height_in_block)
				{
					// Possible if MSAA is enabled with 100% resolution scale or
					// surface dimensions are less than resolution scale threshold and no MSAA.
					// Writethrough.
					content = final_dst;
				}
				else
				{
					content = vk::get_typeless_helper(format(), subres.width_in_block, subres.height_in_block);
					content->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				}

				// Load Cell data into temp buffer
				vk::copy_mipmaped_image_using_buffer(cmd, content, { subres }, gcm_format, false, 1, aspect(), upload_heap, rsx_pitch);

				// Write into final image
				if (content != final_dst)
				{
					vk::copy_scaled_image(cmd, content->value, final_dst->value, content->current_layout, final_dst->current_layout,
						{ 0, 0, subres.width_in_block, subres.height_in_block }, { 0, 0, static_cast<s32>(final_dst->width()), static_cast<s32>(final_dst->height()) },
						1, aspect(), true, aspect() == VK_IMAGE_ASPECT_COLOR_BIT ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
						format(), format());
				}

				if (samples() > 1)
				{
					// Trigger unresolve
					msaa_flags = rsx::surface_state_flags::require_unresolve;
				}
			}

			state_flags &= ~rsx::surface_state_flags::erase_bkgnd;
		}

		void initialize_memory(vk::command_buffer& cmd, bool read_access)
		{
			const bool memory_load = is_depth_surface() ?
				!!g_cfg.video.read_depth_buffer :
				!!g_cfg.video.read_color_buffers;

			if (!memory_load)
			{
				clear_memory(cmd, this);

				if (read_access && samples() > 1)
				{
					// Only clear the resolve surface if reading from it, otherwise it's a waste
					clear_memory(cmd, get_resolve_target_safe(cmd));
				}

				msaa_flags = rsx::surface_state_flags::ready;
			}
			else
			{
				load_memory(cmd);
			}
		}

	public:
		u64 frame_tag = 0; // frame id when invalidated, 0 if not invalid
		using viewable_image::viewable_image;

		vk::viewable_image* get_surface(rsx::surface_access access_type) override
		{
			if (samples() == 1 || access_type == rsx::surface_access::write)
			{
				return this;
			}

			// A read barrier should have been called before this!
			verify("Read access without explicit barrier" HERE), resolve_surface, !(msaa_flags & rsx::surface_state_flags::require_resolve);
			return resolve_surface.get();
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

		image_view* get_view(u32 remap_encoding, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap,
			VkImageAspectFlags mask = VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT) override
		{
			if (remap_encoding == VK_REMAP_VIEW_MULTISAMPLED)
			{
				// Special remap flag, intercept here
				return vk::viewable_image::get_view(VK_REMAP_IDENTITY, remap, mask);
			}

			if (!resolve_surface) [[likely]]
			{
				return vk::viewable_image::get_view(remap_encoding, remap, mask);
			}
			else
			{
				return resolve_surface->get_view(remap_encoding, remap, mask);
			}
		}

		void memory_barrier(vk::command_buffer& cmd, rsx::surface_access access)
		{
			const bool read_access = (access != rsx::surface_access::write);
			const bool is_depth = is_depth_surface();

			if ((g_cfg.video.read_color_buffers && !is_depth) ||
				(g_cfg.video.read_depth_buffer && is_depth))
			{
				// TODO: Decide what to do when memory loads are disabled but the underlying has memory changed
				// NOTE: Assume test() is expensive when in a pinch
				if (last_use_tag && state_flags == rsx::surface_state_flags::ready && !test())
				{
					// TODO: Figure out why merely returning and failing the test does not work when reading (TLoU)
					// The result should have been the same either way
					state_flags |= rsx::surface_state_flags::erase_bkgnd;
				}
			}

			if (old_contents.empty()) [[likely]]
			{
				if (state_flags & rsx::surface_state_flags::erase_bkgnd)
				{
					// NOTE: This step CAN introduce MSAA flags!
					initialize_memory(cmd, read_access);

					verify(HERE), state_flags == rsx::surface_state_flags::ready;
					on_write(rsx::get_shared_tag(), static_cast<rsx::surface_state_flags>(msaa_flags));
				}

				if (msaa_flags & rsx::surface_state_flags::require_resolve)
				{
					if (read_access)
					{
						// Only do this step when read access is required
						get_resolve_target_safe(cmd);
						resolve(cmd);
					}
				}
				else if (msaa_flags & rsx::surface_state_flags::require_unresolve)
				{
					if (!read_access)
					{
						// Only do this step when it is needed to start rendering
						verify(HERE), resolve_surface;
						unresolve(cmd);
					}
				}

				return;
			}

			// Memory transfers
			vk::image *target_image = (samples() > 1) ? get_resolve_target_safe(cmd) : this;
			vk::blitter hw_blitter;
			const auto dst_bpp = get_bpp();

			unsigned first = prepare_rw_barrier_for_transfer(this);
			bool optimize_copy = true;
			bool any_valid_writes = false;
			u64  newest_tag = 0;

			for (auto i = first; i < old_contents.size(); ++i)
			{
				auto &section = old_contents[i];
				auto src_texture = static_cast<vk::render_target*>(section.source);
				src_texture->read_barrier(cmd);

				if (src_texture->test()) [[likely]]
				{
					any_valid_writes = true;
				}
				else
				{
					continue;
				}

				const auto src_bpp = src_texture->get_bpp();
				rsx::typeless_xfer typeless_info{};

				if (src_texture->info.format == info.format) [[likely]]
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
						typeless_info.src_native_format_override = static_cast<u32>(info.format);
						typeless_info.src_scaling_hint = f32(src_bpp) / dst_bpp;
					}
				}

				section.init_transfer(this);
				auto src_area = section.src_rect();
				auto dst_area = section.dst_rect();

				if (g_cfg.video.antialiasing_level != msaa_level::none)
				{
					src_texture->transform_pixels_to_samples(src_area);
					this->transform_pixels_to_samples(dst_area);
				}

				bool memory_load = true;
				if (dst_area.x1 == 0 && dst_area.y1 == 0 &&
					unsigned(dst_area.x2) == target_image->width() && unsigned(dst_area.y2) == target_image->height())
				{
					// Skip a bunch of useless work
					state_flags &= ~(rsx::surface_state_flags::erase_bkgnd);
					msaa_flags = rsx::surface_state_flags::ready;

					memory_load = false;
					stencil_init_flags = src_texture->stencil_init_flags;
				}
				else if (state_flags & rsx::surface_state_flags::erase_bkgnd)
				{
					// Might introduce MSAA flags
					initialize_memory(cmd, false);
					verify(HERE), state_flags == rsx::surface_state_flags::ready;
				}

				if (msaa_flags & rsx::surface_state_flags::require_resolve)
				{
					// Need to forward resolve this
					resolve(cmd);
				}

				hw_blitter.scale_image(
					cmd,
					src_texture->get_surface(rsx::surface_access::read),
					this->get_surface(rsx::surface_access::transfer),
					src_area,
					dst_area,
					/*linear?*/false, typeless_info);

				optimize_copy = optimize_copy && !memory_load;
				newest_tag = src_texture->last_use_tag;
			}

			if (!any_valid_writes) [[unlikely]]
			{
				rsx_log.warning("Surface at 0x%x inherited stale references", base_addr);

				clear_rw_barrier();
				shuffle_tag();

				if (!read_access)
				{
					// This will be modified either way
					state_flags |= rsx::surface_state_flags::erase_bkgnd;
					memory_barrier(cmd, access);
				}

				return;
			}

			// NOTE: Optimize flag relates to stencil resolve/unresolve for NVIDIA.
			on_write_copy(newest_tag, optimize_copy);

			if (!read_access && samples() > 1)
			{
				// Write barrier, must initialize
				unresolve(cmd);
			}
		}

		void read_barrier(vk::command_buffer& cmd) { memory_barrier(cmd, rsx::surface_access::read); }
		void write_barrier(vk::command_buffer& cmd) { memory_barrier(cmd, rsx::surface_access::write); }
	};

	static inline vk::render_target* as_rtt(vk::image* t)
	{
		return verify(HERE, dynamic_cast<vk::render_target*>(t));
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
			rsx::surface_antialiasing antialias,
			vk::render_device &device, vk::command_buffer& cmd)
		{
			const auto fmt = vk::get_compatible_surface_format(format);
			VkFormat requested_format = fmt.first;

			u8 samples;
			surface_sample_layout sample_layout;
			if (g_cfg.video.antialiasing_level == msaa_level::_auto)
			{
				samples = get_format_sample_count(antialias);
				sample_layout = surface_sample_layout::ps3;
			}
			else
			{
				samples = 1;
				sample_layout = surface_sample_layout::null;
			}

			VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			if (samples == 1) [[likely]]
			{
				usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			}
			else
			{
				usage_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
			}

			std::unique_ptr<vk::render_target> rtt;
			rtt = std::make_unique<vk::render_target>(device, device.get_memory_mapping().device_local,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D,
				requested_format,
				static_cast<uint32_t>(rsx::apply_resolution_scale(static_cast<u16>(width), true)), static_cast<uint32_t>(rsx::apply_resolution_scale(static_cast<u16>(height), true)), 1, 1, 1,
				static_cast<VkSampleCountFlagBits>(samples),
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL,
				usage_flags,
				0);

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
			surface_depth_format format,
			size_t width, size_t height, size_t pitch,
			rsx::surface_antialiasing antialias,
			vk::render_device &device, vk::command_buffer& cmd)
		{
			const VkFormat requested_format = vk::get_compatible_depth_surface_format(device.get_formats_support(), format);
			VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

			u8 samples;
			surface_sample_layout sample_layout;
			if (g_cfg.video.antialiasing_level == msaa_level::_auto)
			{
				samples = get_format_sample_count(antialias);
				sample_layout = surface_sample_layout::ps3;
			}
			else
			{
				samples = 1;
				sample_layout = surface_sample_layout::null;
			}

			if (samples == 1) [[likely]]
			{
				usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			}

			std::unique_ptr<vk::render_target> ds;
			ds = std::make_unique<vk::render_target>(device, device.get_memory_mapping().device_local,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D,
				requested_format,
				static_cast<uint32_t>(rsx::apply_resolution_scale(static_cast<u16>(width), true)), static_cast<uint32_t>(rsx::apply_resolution_scale(static_cast<u16>(height), true)), 1, 1, 1,
				static_cast<VkSampleCountFlagBits>(samples),
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL,
				usage_flags,
				0);

			ds->change_layout(cmd, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

			ds->set_format(format);
			ds->set_aa_mode(antialias);
			ds->sample_layout = sample_layout;
			ds->memory_usage_flags= rsx::surface_usage_flags::attachment;
			ds->state_flags = rsx::surface_state_flags::erase_bkgnd;
			ds->native_component_map = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };

			ds->native_pitch = static_cast<u16>(width) * 2 * ds->samples_x;
			if (format == rsx::surface_depth_format::z24s8)
				ds->native_pitch *= 2;

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
				const auto new_w = rsx::apply_resolution_scale(prev.width, true, ref->get_surface_width(rsx::surface_metrics::pixels));
				const auto new_h = rsx::apply_resolution_scale(prev.height, true, ref->get_surface_height(rsx::surface_metrics::pixels));

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
					ref->info.flags);

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
		}

		static bool is_compatible_surface(const vk::render_target* surface, const vk::render_target* ref, u16 width, u16 height, u8 sample_count)
		{
			return (surface->format() == ref->format() &&
					surface->get_spp() == sample_count &&
					surface->get_surface_width() >= width &&
					surface->get_surface_height() >= height);
		}

		static void prepare_surface_for_drawing(vk::command_buffer& cmd, vk::render_target *surface)
		{
			if (surface->aspect() == VK_IMAGE_ASPECT_COLOR_BIT)
			{
				surface->change_layout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			}
			else
			{
				surface->change_layout(cmd, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			}

			surface->frame_tag = 0;
			surface->memory_usage_flags |= rsx::surface_usage_flags::attachment;
		}

		static void prepare_surface_for_sampling(vk::command_buffer& /*cmd*/, vk::render_target* /*surface*/)
		{}

		static bool surface_is_pitch_compatible(const std::unique_ptr<vk::render_target> &surface, size_t pitch)
		{
			return surface->rsx_pitch == pitch;
		}

		static void invalidate_surface_contents(vk::command_buffer& /*cmd*/, vk::render_target *surface, u32 address, size_t pitch)
		{
			surface->rsx_pitch = static_cast<u16>(pitch);
			surface->queue_tag(address);
			surface->last_use_tag = 0;
			surface->stencil_init_flags = 0;
			surface->memory_usage_flags = rsx::surface_usage_flags::unknown;
		}

		static void notify_surface_invalidated(const std::unique_ptr<vk::render_target> &surface)
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

		static void notify_surface_reused(const std::unique_ptr<vk::render_target> &surface)
		{
			surface->state_flags |= rsx::surface_state_flags::erase_bkgnd;
			surface->add_ref();
		}

		static bool int_surface_matches_properties(
			const std::unique_ptr<vk::render_target> &surface,
			VkFormat format,
			size_t width, size_t height,
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
			const std::unique_ptr<vk::render_target> &surface,
			surface_color_format format,
			size_t width, size_t height,
			rsx::surface_antialiasing antialias,
			bool check_refs = false)
		{
			VkFormat vk_format = vk::get_compatible_surface_format(format).first;
			return int_surface_matches_properties(surface, vk_format, width, height, antialias, check_refs);
		}

		static bool surface_matches_properties(
			const std::unique_ptr<vk::render_target> &surface,
			surface_depth_format format,
			size_t width, size_t height,
			rsx::surface_antialiasing antialias,
			bool check_refs = false)
		{
			auto device = vk::get_current_renderer();
			VkFormat vk_format = vk::get_compatible_depth_surface_format(device->get_formats_support(), format);
			return int_surface_matches_properties(surface, vk_format, width, height, antialias, check_refs);
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
